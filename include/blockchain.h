/* ============================================================
 * blockchain.h
 * Blockchain-Based Library Book Lending Tracker
 *
 * Author: Deng Mayen Deng Akol
 * Assignment 1 - Individual Assignment
 * ============================================================ */

#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <time.h>
#include <stddef.h>

/* ---------- Size limits ---------- */
#define MAX_BOOKS        200
#define MAX_MEMBERS      200
#define MAX_CHAIN_BLOCKS 2000

#define BOOK_ID_LEN      20
#define TITLE_LEN        80
#define AUTHOR_LEN       50
#define MEMBER_ID_LEN    20
#define NAME_LEN         50
#define COURSE_LEN       10
#define ACTION_LEN       10
#define HASH_LEN         65   /* 64 hex chars + '\0' */
#define SIG_LEN          72   /* max DER-encoded ECDSA P-256 signature */

#define BOOKS_FILE       "data/books.txt"
#define MEMBERS_FILE     "data/members.txt"
#define CHAIN_FILE       "data/chain.dat"
#define PRIVATE_KEY_FILE "keys/ec_private.pem"
#define PUBLIC_KEY_FILE  "keys/ec_public.pem"

/* ---------- Registry structs (per spec) ---------- */
typedef struct {
    char book_id[BOOK_ID_LEN];
    char title[TITLE_LEN];
    char author[AUTHOR_LEN];
} Book;

typedef struct {
    char member_id[MEMBER_ID_LEN];
    char full_name[NAME_LEN];
    char course_code[COURSE_LEN];
} Member;

/* ---------- Block struct (per spec) ----------
 * Note: sig_len is an implementation addition (not in the
 * original spec table) because ECDSA/DER signatures are
 * variable-length; we must know how many of the SIG_LEN bytes
 * in `signature` are meaningful in order to verify them later.
 * This is documented in the technical report.
 */
typedef struct Block {
    int            index;
    time_t         timestamp;
    char           book_id[BOOK_ID_LEN];
    char           book_title[TITLE_LEN];
    char           member_id[MEMBER_ID_LEN];
    char           member_name[NAME_LEN];
    char           action[ACTION_LEN];        /* BORROWED / RETURNED / OVERDUE */
    char           previous_hash[HASH_LEN];
    unsigned char  signature[SIG_LEN];
    unsigned int   sig_len;                    /* actual signature length */
    char           hash[HASH_LEN];
} Block;

/* ---------- Blockchain container ---------- */
typedef struct {
    Block blocks[MAX_CHAIN_BLOCKS];
    int   length;
} Blockchain;

/* ---------- Registry API (registry.c) ---------- */
int  load_books(const char *filename, Book books[], int max_books);
int  load_members(const char *filename, Member members[], int max_members);
int  find_book(const Book books[], int count, const char *book_id);
int  find_member(const Member members[], int count, const char *member_id);

/* ---------- Crypto API (crypto_utils.c) ---------- */
int  crypto_init_keys(void);
void sha256_hex(const unsigned char *data, size_t len, char out_hex[HASH_LEN]);
int  ecdsa_sign(const unsigned char *data, size_t len,
                 unsigned char *sig_out, unsigned int *sig_len_out);
int  ecdsa_verify(const unsigned char *data, size_t len,
                   const unsigned char *sig, unsigned int sig_len);
void crypto_cleanup(void);

/* ---------- Blockchain API (blockchain.c) ---------- */
void chain_init(Blockchain *chain);
void chain_create_genesis(Blockchain *chain);
void block_serialize(const Block *b, char *buffer, size_t buffer_size);
void block_compute_hash(Block *b);
int  block_sign(Block *b);
int  block_verify_signature(const Block *b);

int  chain_add_borrow(Blockchain *chain, const Book books[], int book_count,
                       const Member members[], int member_count,
                       const char *book_id, const char *member_id);
int  chain_add_return(Blockchain *chain, const char *book_id);

int  chain_validate(const Blockchain *chain, int verbose);
void chain_print_records(const Blockchain *chain);
int  chain_find_last_borrow_index(const Blockchain *chain, const char *book_id);

int  chain_save(const Blockchain *chain, const char *filename);
int  chain_load(Blockchain *chain, const char *filename);

void chain_tamper_demo(Blockchain *chain);

#endif /* BLOCKCHAIN_H */
