#include "main.h"


int main(int argc, char* argv[]) {
    signal(SIGINT, interrupt_handler);

    // Declare defaults
    int   Connections = 256;
    char *IDFile      = "id_rsa";
    int   ListenPort  = DEFAULT_PORT;

    // Arguements may change those
    int opt;
    while((opt = getopt(argc, argv, "c:i:p:vh?")) != -1) {
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
            case 'v':
                printf("Verbose flag currently does nothing\n");
                break; // NOT IMPLEMENTED
            case 'h':
            case '?':
            default:
                printf(HELP_TEXT);
                exit(EXIT_FAILURE);
        }
    }

    //RSA *keypair = gen_rsa_keypair();
    //printf("%s\n%s\n", get_pub_key(keypair), get_pri_key(keypair));

    commands = (struct command*)malloc(sizeof(struct command)*ROOT_COMMAND_COUNT);

    commands[0] = (struct command){ COMMAND_ECHO, 'e', "echo", HELP_ECHO, _echo };
    commands[1] = (struct command){ COMMAND_HELP, 'h', "help", HELP_COMMANDS, _help };
    commands[2] = (struct command){ COMMAND_QUIT, 'q', "quit", HELP_QUIT, _quit };
    commands[3] = (struct command){ COMMAND_CONNECT, 'c', "connect", HELP_CONNECT, _connect };

    setlocale(LC_CTYPE, "");
    initscr(); raw(); noecho();
    keypad(stdscr, TRUE);

    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED,   COLOR_BLACK);

    refresh();
    log_win = create_newwin(LINES, COLS-1, 0, 0);
    scrollok(log_win, TRUE);
    cmd_win = create_newwin(1, COLS-1, LINES-1, 0);
    update_cmd_mode(MODE_COMMAND);

    init_listener(ListenPort, Connections);

    size_t bufsize = sizeof(char) * 1024;
    char *line_buffer = (char*)malloc(bufsize);
    //ssize_t bytes_read = 0;

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
                    for(long unsigned i = 0; i < sizeof(char)*1024; i++) line_buffer[i] = '\0';
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
    free(line_buffer);

    endwin();
    return 0;
}

