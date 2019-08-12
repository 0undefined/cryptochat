#include "main.h"

char *log_buffer[1024];

static volatile unsigned char is_running = 0xff;
unsigned char TUI = 0x00;
enum MODES TUI_MODE = MODE_COMMAND;
WINDOW *cmd_win;
WINDOW *log_win;
struct tm tm;

void update_cmd_mode(enum MODES new_mode) {
    TUI_MODE = new_mode;
    wattron(cmd_win, COLOR_PAIR(1));
    switch(new_mode) {
        case MODE_CHAT:
            mvwprintw(cmd_win, 0, 0, "#");
            break;
        case MODE_VISUAL:
            mvwprintw(cmd_win, 0, 0, "?");
            break;
        case MODE_SEARCH:
            mvwprintw(cmd_win, 0, 0, "/");
            break;
        case MODE_COMMAND:
        default:
            mvwprintw(cmd_win, 0, 0, ":");
            break;
    }
    wattroff(cmd_win, COLOR_PAIR(1));
    wrefresh(cmd_win);
}

void update_cmd_buffer(char *new_buf) {
    mvwprintw(cmd_win, 0, 1, "%s", new_buf);
    wrefresh(cmd_win);
}

void clear_cmd_buffer() {
    mvwprintw(cmd_win, 0, 0, "%*c", COLS, ' ');
    update_cmd_mode(TUI_MODE);
}

void add_log_error(char *log_msg, ...) {
    time_t T = time(NULL);
    tm = *localtime(&T);
    char buffer[1024];

    va_list args;
    va_start(args, log_msg);

    vsnprintf(buffer, sizeof(buffer), log_msg, args);

    va_end(args);


    if(TUI) {
        wprintw(log_win, "[%02d:%02d] ", tm.tm_hour, tm.tm_min);
        wattron(log_win, COLOR_PAIR(2));
        wprintw(log_win, "! ");
        wattroff(log_win, COLOR_PAIR(2));
        wprintw(log_win, "%s\n", buffer);
#ifdef DEBUG
        void **tracebuf = (void*)malloc(sizeof(void*)*8);
        int stacksize = backtrace(tracebuf, 8);
        char **tracestr = backtrace_symbols(tracebuf, stacksize);
        wprintw(log_win, "    > stacktrace:\n");
        for(int i = 0; i < stacksize; i++) wprintw(log_win, "    > %s\n", tracestr[i]);
        free(tracestr);
        free(tracebuf);
#endif
        wrefresh(log_win);
    } else {
        fprintf(stderr, "[%02d:%02d] ! %s\n", tm.tm_hour, tm.tm_min, buffer);
    }
}
void add_log(char *log_msg, ...) {
    time_t T = time(NULL);
    tm = *localtime(&T);
    char buffer[1024];

    va_list args;
    va_start(args, log_msg);

    vsnprintf(buffer, sizeof(buffer), log_msg, args);

    va_end(args);

    if(TUI) {
        wprintw(log_win, "[%02d:%02d] %s\n", tm.tm_hour, tm.tm_min, buffer);
        wrefresh(log_win);
    } else {
        printf("[%02d:%02d] %s\n", tm.tm_hour, tm.tm_min, buffer);
    }
}

void interrupt_handler(int signal) {
    if(signal == SIGINT) {
        is_running = 0x00;
    }
    printf("exiting.\n");
    if(TUI) { endwin(); }
    exit(EXIT_SUCCESS);
}

int command_recognizer(char *cmd);
WINDOW *create_newwin(int height, int width, int starty, int startx);

int main(int argc, char* argv[]) {
    signal(SIGINT, interrupt_handler);

    // Declare defaults
    int   Connections = 256;
    char *IDFile      = "id_rsa";
    int   ListenPort  = DEFAULT_PORT;

    // Arguements may change those
    int opt;
    while((opt = getopt(argc, argv, "c:i:p:tvh?")) != -1) {
        switch(opt) {
            case 'c':
                {
                    int tmp = atoi(optarg);
                    if(tmp != 0) Connections = tmp;
                    else {
                        fprintf(stderr, "! Error (%d): %s\n", errno, strerror(errno));
                    }
                }
                break;
            case 'i':
                IDFile = (char*)malloc(sizeof(char) * strlen(optarg));
                strcpy(IDFile, optarg);
                break;
            case 'p':
                {
                    int tmp = atoi(optarg);
                    if(tmp != 0) ListenPort = tmp;
                    else {
                        fprintf(stderr, "! Error (%d): %s\n", errno, strerror(errno));
                    }
                }
                break;
            case 't':
                TUI = 0xff;
                break;
            case 'v':
                printf("Verbose flag currently does nothing\n");
                break; // NOT IMPLEMENTED
            case 'h':
            case '?':
            default:
                printf(help_text);
                exit(EXIT_FAILURE);
        }
    }

    //RSA *keypair = gen_rsa_keypair();
    //printf("%s\n%s\n", get_pub_key(keypair), get_pri_key(keypair));

    if(TUI) { // Init ncurses
        setlocale(LC_CTYPE, "");
        initscr();
        raw();
        noecho();
        keypad(stdscr, TRUE);
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_RED,   COLOR_BLACK);
        refresh();
        log_win = create_newwin(LINES, COLS-1, 0, 0);
        scrollok(log_win, TRUE);
        cmd_win = create_newwin(1, COLS-1, LINES-1, 0);
        update_cmd_mode(MODE_COMMAND);
    }

    init_listener(ListenPort, Connections);

    size_t bufsize = sizeof(char) * 1024;
    char *line_buffer = (char*)malloc(bufsize);
    ssize_t bytes_read = 0;

    if(TUI) {

        int in;
        int col = 0;

        add_log("ready");
        wmove(cmd_win, 0, 1+col);
        wrefresh(cmd_win);

        while((in = getch()) != '') {
            // Check if the user tries to change mode from current mode
            enum MODES oldmode = TUI_MODE;
            if(col == 0) {
                for(int i = 0; i < (int)strlen(mode_mappings); i++) {
                    if(in == mode_mappings[i] && (enum MODES)i != TUI_MODE) {
                        update_cmd_mode((enum MODES)i);
                    }
                }
                if(oldmode == TUI_MODE && in != KEY_BACKSPACE) { // No change to mode was made
                    if(!isspace(in)) {
                    line_buffer[col] = in;
                    line_buffer[++col] = '\0';
                    }
                }
                else {
                    in = ' ';
                }
            }
            else {
                switch(in) {
                    case KEY_BACKSPACE:
                        if(col > 0) {
                            line_buffer[--col] = '\0';
                            in = ' ';
                            clear_cmd_buffer();
                            update_cmd_buffer(line_buffer);
                        }
                        break;
                    case '\n':
                    case '\r':
                    case KEY_ENTER:
                        if(col > 0) {
                            command_recognizer(line_buffer);
                            for(int i = 0; i < (int)strlen(line_buffer); i++) line_buffer[i] = '\0';
                            clear_cmd_buffer();
                            col = 0;
                        }
                        in = ' ';
                        break;
                    default:
                        line_buffer[col] = in;
                        line_buffer[++col] = '\0';
                        break;
                }
            }

            waddch(cmd_win, (char)in);

            mvwprintw(cmd_win, 0, COLS-12, "% 4d,%d", col, strlen(line_buffer));
            wmove(cmd_win, 0, 1+col);
            wrefresh(cmd_win);

            //update_cmd_buffer(line_buffer);
        }

        add_log("exiting... [press any key]");
        getch();
    } else {
        while((bytes_read = getline(&line_buffer, &bufsize, stdin)) != -1) {
            if(bytes_read != 0) {
                line_buffer[bytes_read-1] = '\0';
                command_recognizer(line_buffer);
            }
        }
    }
    free(line_buffer);

    if(TUI) {
        endwin();
    }
    return 0;
}

int command_recognizer(char *cmd) {
    char *first = strtok(cmd, " ");
    if(first == NULL) return -1;
    if(strcmp(first, "quit") == 0) {
        if(TUI) {
            endwin();
        }
        printf("\n");
        exit(EXIT_SUCCESS);
    } else if(strcmp(first, "connect") == 0) {
        // expect 1-2 more arguments
        char *host = strtok(NULL, " ");
        int port = DEFAULT_PORT;
        if(host == NULL) {
            add_log(HELP_CONNECT);
            return -1;
        }
        if(strcmp(host, "help") == 0) {
            add_log(HELP_CONNECT);
            return 0;
        }
        char *port_str = strtok(NULL, " ");
        // Check if non-proper syntax is used, host port
        if(port_str != NULL) {
            port = atoi(port_str);
            if(port == 0) {
                add_log("Port cannot be '%s'", port_str);
                return -1;
            }
        }
        // check if the proper syntax is used ie. host:port or no port specified at all.
        else {
            host = strtok(host, ":");
            port_str = strtok(NULL, ":");
            if(port_str != NULL) {
                port = atoi(port_str);
                if(port == 0) {
                    add_log("Port cannot be '%s'", port_str);
                    return -1;
                }
            }

        }
        add_log("Connecting to %s at port %d...", host, port);
        connect_host(host, port);
    } else if(strcmp(first, "help") == 0) {
        add_log(HELP_COMMANDS);
    }
    return 0;
}

WINDOW *create_newwin(int height, int width, int starty, int startx) {
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    //box(local_win, 0 , 0);
    /* 0, 0 gives default characters
     * for the vertical and horizontal
     * lines      */
    wrefresh(local_win);

    return local_win;
}
