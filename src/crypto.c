#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

// Debugging build doesn't need a large `e'
#ifdef DEBUG
#define BITS 1024
#else
#define BITS 1024 * 8
#endif

RSA *gen_rsa_keypair() {
    int ret = 0;

    RSA     *rsa = NULL;
    BIGNUM  *bne = NULL;

    unsigned long e = RSA_F4;

    bne = BN_new();
    ret = BN_set_word(bne, e);
    if(ret != 1) {
        BN_free(bne);
        return NULL;
    };

    rsa = RSA_new();
    ret = RSA_generate_key_ex(rsa, BITS, bne, NULL);
    if(ret != 1) {
        RSA_free(rsa);
        BN_free(bne);
        return NULL;
    }

    return rsa;
}

char *get_pub_key(RSA *in) {
    size_t pub_len;
    char  *pub_key;

    BIO     *bp_public  = NULL;

    bp_public = BIO_new(BIO_s_mem());
    if(PEM_write_bio_RSAPublicKey(bp_public, in) != 1) {
        BIO_free_all(bp_public);
        return NULL;
    }

    pub_len = BIO_pending(bp_public);
    pub_key = (char*)malloc(pub_len + 1);
    BIO_read(bp_public, pub_key, pub_len);

    pub_key[pub_len] = '\0';

    BIO_free_all(bp_public);
    return pub_key;
}

char *get_pri_key(RSA *in) {
    size_t pri_len;
    char  *pri_key;

    BIO     *bp_private  = NULL;

    bp_private = BIO_new(BIO_s_mem());
    if(PEM_write_bio_RSAPrivateKey(bp_private, in, NULL, NULL, 0, NULL, NULL) != 1) {
        BIO_free_all(bp_private);
        return NULL;
    }

    pri_len = BIO_pending(bp_private);
    pri_key = (char*)malloc(pri_len + 1);
    BIO_read(bp_private, pri_key, pri_len);

    pri_key[pri_len] = '\0';

    BIO_free_all(bp_private);
    return pri_key;
}
