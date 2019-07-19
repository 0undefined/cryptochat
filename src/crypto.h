#include <openssl/rsa.h>

RSA *gen_rsa_keypair();
char *get_pub_key(RSA *in);
char *get_pri_key(RSA *in);
