// Library inclusions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <locale.h>
#include <ncurses.h>
#include <wchar.h>
#include <time.h>

// Debugging inclusion
#ifdef DEBUG
#include <execinfo.h>
#endif

// Homebrewn libraries
#include "network.h"
#include "commands.h"
#include "ascii.h"

// Definitions
#define ROOT_COMMAND_COUNT 6

#define HELP_COMMANDS   "Commands:\n" \
    "          help\n"                \
    "          quit\n"                \
    "          connect\n"             \
    "          message\n"             \
    "          whisper"

#define HELP_ECHO       "e[cho] [text]\n      echos some text?"
#define HELP_HELP       "h[elp] [command]\n       Get help about command"
#define HELP_QUIT       "q[uit]\n       quits the program"
#define HELP_CONNECT    "c[onnect] [HOST[:PORT]]\n       connects to HOST through PORT"
#define HELP_MSG        "m[sg|essage] [message]\n       Send a message to all connected peers"
#define HELP_WHISPER    "w[hisper] [recipient] [message]\n       Send a message to a specified peer"

#define MAX(a,b) (a > b ? a : b)
#define MIN(a,b) (a < b ? a : b)

enum MODES {
    MODE_COMMAND = 0,
    MODE_CHAT    = 1,
    MODE_VISUAL  = 2,
    MODE_SEARCH  = 3,
};

unsigned char V = 0;
struct command *commands;
static volatile unsigned char is_running = 0xff;
enum MODES TUI_MODE = MODE_COMMAND;
WINDOW *cmd_win;
WINDOW *log_win;
struct tm tm;

char *log_buffer[1024];

char mode_mappings[] = {
    ':', // 0
    '#', // 1
    '?', // 2
    '/', // 3
    '\0', // Allows strlen i guess or something
};

#define HELP_TEXT \
    "Usage:\n" \
    "  cchat [OPTIONS]\n\n" \
    "Options:\n" \
    "  -h, -?   Print this help message\n" \
    "  -c NUM   Allows NUM amount of connections simultanously before ignoring\n" \
    "           incoming connections \x1b[31mNOT IMPLEMENTED\x1b[0m\n" \
    "  -i ID    Use ID-file for public/private key encryption \x1b[31mNOT IMPLEMENTED\x1b[0m\n" \
    "  -p PORT  Listen on port PORT\n" \
    "  -v       Verbose \x1b[31mNOT IMPLEMENTED\x1b[0m\n"

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
    for(int i = 0; i < stacksize; i++) wprintw(log_win, "        > %s\n", tracestr[i]);
    free(tracestr);
    free(tracebuf);
#endif
    wrefresh(log_win);
}

void add_log(char *log_msg, ...) {
    time_t T = time(NULL);
    tm = *localtime(&T);
    char buffer[1024];

    va_list args;
    va_start(args, log_msg);

    vsnprintf(buffer, sizeof(buffer), log_msg, args);

    va_end(args);

    wprintw(log_win, "[%02d:%02d] %s\n", tm.tm_hour, tm.tm_min, buffer);
    wrefresh(log_win);
}

void interrupt_handler(int signal) {
    if(signal == SIGINT) is_running = 0x00;
    endwin();
    exit(EXIT_SUCCESS);
}

int _echo(char *argv) {
    if(argv == NULL) return 1;
    else if(argv[0] == '\0') return 1;

    size_t index = strlen(argv);
    char *fst = (char*)malloc(sizeof(char)*index+(1024-index));
    strcpy(fst, argv);

    while((argv = strtok(NULL,"")) != NULL) {
        *(fst+strlen(fst)) = ' ';

        strcpy(fst+strlen(fst), argv);
    }

    add_log("%s", fst);

    free(fst);

    return 0;
}

int _quit(char *_) {
    _=_; // Ignore arg
    cleanup();
    endwin();
    exit(EXIT_SUCCESS);
    return 0;
}

int _help(char *command) {
    if(command != NULL && strlen(command) != 0) {
        for(int i = 0; i < ROOT_COMMAND_COUNT; i++) {
            if(strcmp(command, commands[i].longhand) == 0) {
                add_log("Usage: %s", commands[i].help);
                break;
            }
            if(i == ROOT_COMMAND_COUNT - 1) add_log(HELP_COMMANDS);
        }
    } else {
        add_log(HELP_COMMANDS);
    }
    return 0;
}

int _connect(char *args) {
    char *host = strtok(args, " ");
    int port   = DEFAULT_PORT;
    if(host == NULL) {
        add_log(HELP_CONNECT);
        return -1;
    }
    char *port_str = strtok(NULL, " ");
    // Check if non-proper syntax is used, host port
    if(port_str != NULL) {
        port = atoi(port_str);
        if(port == 0) {
            add_log("Port cannot be '%s'", port_str);
            return -1;
        }
    } else {
        // check if the proper syntax is used ie. host:port or no port specified at all.
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
    return 0;
}

int command_recognizer(char *cmd) {
    //size_t cmdlen = strlen(cmd);
    char *first = strtok(cmd, " ");
    if(first == NULL) return -1;
    if(strlen(first) == 0) {
        add_log_error("I honestly don't know how you submitted an empty command.");
    }
    if(strlen(first) == 1) {
        add_log("shorthand recognized");
        char shorthand = first[0];
        for(int i = 0; i < ROOT_COMMAND_COUNT; i++) {
            if(shorthand == commands[i].shorthand) {
                int ret = commands[i].fun(cmd + strlen(first) + 1);
                if(ret != 0) {
                    add_log_error("Usage: %s", (char*)commands[i].help);
                }
                break;
            }
            if(i == ROOT_COMMAND_COUNT - 1) add_log(HELP_COMMANDS);
        }
    } else {
        for(int i = 0; i < ROOT_COMMAND_COUNT; i++) {
            if(strcmp(first, commands[i].longhand) == 0) {

                char *arg = strtok(NULL, " ");

                int ret = commands[i].fun(arg);
                if(ret != 0) {
                    add_log_error("Usage: %s", (char*)commands[i].help);
                }
                break;
            }
            if(i == ROOT_COMMAND_COUNT - 1) add_log(HELP_COMMANDS);
        }
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
