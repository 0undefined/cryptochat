#include "main.h"


// Insert 'c' at col in buffer, increment col and buffer
int add_char(char *buffer[], const int bufsize, int *col, int *buflen, char c) {
    if(*col >= bufsize) return -1;

    if(*col == *buflen) { // Append 'c' to buffer
        (*buffer)[*col] = c;
        (*buffer)[*col+1] = '\0';
        (*col)++;
        (*buflen)++;

    } else { // We are inserting a char inside the text
        const size_t diffsize = (size_t)(*buflen - *col);
        char *tmp = (char*)malloc(sizeof(char)*diffsize);
        strncpy(tmp, (*buffer)+(*col), diffsize+1);

        (*buffer)[*col] = c;
        //(*buffer)[*col+1] = '\0';
        (*col)++;
        (*buflen)++;

        strncpy((*buffer)+(*col), tmp, diffsize+1);
        free(tmp);
    }
    return *buflen;
}

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
                V = 0xff;
                //printf("Verbose flag currently does nothing\n");
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

    commands[0] = (struct command){ COMMAND_ECHO,       'e', "echo",    HELP_ECHO,      _echo };
    commands[1] = (struct command){ COMMAND_HELP,       'h', "help",    HELP_HELP,      _help };
    commands[2] = (struct command){ COMMAND_QUIT,       'q', "quit",    HELP_QUIT,      _quit };
    commands[3] = (struct command){ COMMAND_CONNECT,    'c', "connect", HELP_CONNECT,   _connect };
    commands[4] = (struct command){ COMMAND_MSG,        'm', "message", HELP_MSG,       _echo };
    commands[5] = (struct command){ COMMAND_WHISPER,    'w', "whisper", HELP_WHISPER,   _echo };

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
    char *line_buffer = (char*)calloc(1024, sizeof(char));//(char*)malloc(bufsize);
    //ssize_t bytes_read = 0;

    int in;
    int col = 0;
    int buflen = 0;

    add_log("ready");
    wmove(cmd_win, 0, 1+col);
    wrefresh(cmd_win);


    while((in = getch()) != EOF) {
        // Check if the user tries to change mode from current mode
        //enum MODES oldmode = TUI_MODE;
        if(col == 0) {
            for(int i = 0; i < (int)strlen(mode_mappings); i++) {
                if(in == mode_mappings[i] && (enum MODES)i != TUI_MODE) {
                    update_cmd_mode((enum MODES)i);
                    goto skip;
                }
            }
        }
        switch(in) {
            case KEY_BACKSPACE:
                if(col > 0) {
                    line_buffer[--col] = '\0';
                    buflen--;
                    //in = ' ';
                    clear_cmd_buffer();
                    update_cmd_buffer(line_buffer);
                }
                in = ' ';
                break;
                //case ASCII_DELETE:
                //    if(buflen > 0) {
                //    if(col == buflen 0) {
                //        line_buffer[col] = '\0';
                //        in = ' ';
                //        clear_cmd_buffer();
                //        update_cmd_buffer(line_buffer);
                //        buflen--;
                //    }
                //    }
                //    break;
            case ASCII_LF:
            case ASCII_CR:
            case KEY_ENTER:
                if(col > 0) {
                    command_recognizer(line_buffer);
                    for(long unsigned i = 0; i < 1024; i++) line_buffer[i] = '\0';
                    clear_cmd_buffer();
                    col    = 0;
                    buflen = 0;
                }
                //in = ' ';
                break;
            case KEY_LEFT:
                col = MAX(0, col-1);
                break;
            case KEY_RIGHT:
                col = MIN(buflen, col+1);
                break;
            default:
                //if(in > 31 && in < 127) {}
                add_char(&line_buffer, bufsize, &col, &buflen, in);
                //line_buffer[col] = in;
                //line_buffer[++col] = '\0';
                //buflen++;
                break;
        }

        update_cmd_buffer(line_buffer);

        skip: {
          mvwprintw(cmd_win, 0, COLS-12, "% 4d,%d,%d", col, buflen, strlen(line_buffer));
          wmove(cmd_win, 0, 1+col);
          wrefresh(cmd_win);
        };

        //update_cmd_buffer(line_buffer);
    }

    add_log("exiting... [press any key]");
    getch();
    free(line_buffer);

    endwin();
    return 0;
}

