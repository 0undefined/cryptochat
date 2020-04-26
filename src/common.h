#ifndef COMMON_H
#define COMMON_H

#define PROGNAME            "crypto_chat"

#define VERSION_MAJOR       0
#define VERSION_MINOR       1
#define VERSION_PATCH       0

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define CTRL(x)           ((x) & 0x1F)

#define isspace(a) (a == ' ' || a == '\n' || a == '\t' || a == '\r')

#endif
