/* ============================================================
 * crypto_utils.c
 * SHA-256 hashing and ECDSA digital signatures (OpenSSL 3.x, EVP API)
 *
 * Keys are generated once (P-256 curve) and persisted under keys/
 * so that signatures remain verifiable across separate program runs.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "../include/blockchain.h"

/* Kept for the lifetime of the process once loaded/generated */
static EVP_PKEY *g_keypair = NULL;

/* ---------- SHA-256 ---------- */

void sha256_hex(const unsigned char *data, size_t len, char out_hex[HASH_LEN]) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    for (unsigned int i = 0; i < digest_len; i++) {
        snprintf(out_hex + (i * 2), 3, "%02x", digest[i]);
    }
    out_hex[digest_len * 2] = '\0';
}

/* ---------- Key management ---------- */

static int generate_and_save_keypair(void) {
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL);
    if (!pctx) return -1;

    if (EVP_PKEY_keygen_init(pctx) <= 0) { EVP_PKEY_CTX_free(pctx); return -1; }
    if (EVP_PKEY_CTX_set_group_name(pctx, "prime256v1") <= 0) {
        EVP_PKEY_CTX_free(pctx);
        return -1;
    }

    EVP_PKEY *pkey = NULL;
    if (EVP_PKEY_keygen(pctx, &pkey) <= 0) { EVP_PKEY_CTX_free(pctx); return -1; }
    EVP_PKEY_CTX_free(pctx);

    FILE *priv = fopen(PRIVATE_KEY_FILE, "w");
    if (!priv) { EVP_PKEY_free(pkey); return -1; }
    PEM_write_PrivateKey(priv, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(priv);

    FILE *pub = fopen(PUBLIC_KEY_FILE, "w");
    if (!pub) { EVP_PKEY_free(pkey); return -1; }
    PEM_write_PUBKEY(pub, pkey);
    fclose(pub);

    g_keypair = pkey;
    return 0;
}

static int load_existing_keypair(void) {
    FILE *priv = fopen(PRIVATE_KEY_FILE, "r");
    if (!priv) return -1;

    EVP_PKEY *pkey = PEM_read_PrivateKey(priv, NULL, NULL, NULL);
    fclose(priv);

    if (!pkey) return -1;

    g_keypair = pkey;
    return 0;
}

/* Ensures a signing keypair exists (loads it if already generated,
 * otherwise creates a fresh P-256 EC keypair and persists it). */
int crypto_init_keys(void) {
    if (load_existing_keypair() == 0) {
        return 0;
    }
    return generate_and_save_keypair();
}

void crypto_cleanup(void) {
    if (g_keypair) {
        EVP_PKEY_free(g_keypair);
        g_keypair = NULL;
    }
}

/* ---------- ECDSA sign / verify ---------- */

int ecdsa_sign(const unsigned char *data, size_t len,
               unsigned char *sig_out, unsigned int *sig_len_out) {
    if (!g_keypair) return -1;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) return -1;

    size_t sig_len = SIG_LEN;
    int ok = 0;

    if (EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, g_keypair) == 1) {
        if (EVP_DigestSignUpdate(mdctx, data, len) == 1) {
            if (EVP_DigestSignFinal(mdctx, sig_out, &sig_len) == 1) {
                *sig_len_out = (unsigned int)sig_len;
                ok = 1;
            }
        }
    }

    EVP_MD_CTX_free(mdctx);
    return ok ? 0 : -1;
}

int ecdsa_verify(const unsigned char *data, size_t len,
                  const unsigned char *sig, unsigned int sig_len) {
    if (!g_keypair) return 0;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) return 0;

    int result = 0;

    if (EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, g_keypair) == 1) {
        int rc = EVP_DigestVerifyUpdate(mdctx, data, len);
        if (rc == 1) {
            rc = EVP_DigestVerifyFinal(mdctx, sig, sig_len);
            result = (rc == 1);
        }
    }

    EVP_MD_CTX_free(mdctx);
    return result;
}
