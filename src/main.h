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

#ifdef DEBUG
#include <execinfo.h>
#endif

#include "network.h"

#define HELP_CONNECT "HOST[:PORT]"
#define HELP_COMMANDS "Commands:\n"\
  "          help\n" \
  "          quit\n" \
  "          connect"

enum MODES {
    MODE_COMMAND = 0,
    MODE_CHAT    = 1,
    MODE_VISUAL  = 2,
    MODE_SEARCH  = 3,
};

char mode_mappings[] = {
    ':', // 0
    '#', // 1
    '?', // 2
    '/', // 3
    '\0', // Allows strlen i guess or something
};

const char *help_text =
"Usage:\n"
"  cchat [OPTIONS]\n\n"
"Options:\n"
"  -h, -?   Print this help message\n"
"  -c NUM   Allows NUM amount of connections simultanously before ignoring\n"
"           incoming connections \x1b[31mNOT IMPLEMENTED\x1b[0m\n"
"  -i ID    Use ID-file for public/private key encryption \x1b[31mNOT IMPLEMENTED\x1b[0m\n"
"  -p PORT  Listen on port PORT\n"
"  -t       Enable TUI\n"
"  -v       Verbose \x1b[31mNOT IMPLEMENTED\x1b[0m\n";
