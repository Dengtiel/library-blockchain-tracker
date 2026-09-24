/* ============================================================
 * blockchain.c
 * Core blockchain logic: genesis block, borrowing/returning,
 * hashing, signing, chain validation, persistence, tamper demo.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/blockchain.h"

#define GENESIS_HASH "0000000000000000000000000000000000000000000000000000000000000"

void chain_init(Blockchain *chain) {
    chain->length = 0;
    memset(chain->blocks, 0, sizeof(chain->blocks));
}

/* Serialize all block fields (except the hash itself) into a single
 * buffer so it can be hashed and signed consistently. */
void block_serialize(const Block *b, char *buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size,
             "%d|%ld|%s|%s|%s|%s|%s|%s",
             b->index,
             (long)b->timestamp,
             b->book_id,
             b->book_title,
             b->member_id,
             b->member_name,
             b->action,
             b->previous_hash);
}

void block_compute_hash(Block *b) {
    char buffer[512];
    block_serialize(b, buffer, sizeof(buffer));
    sha256_hex((unsigned char *)buffer, strlen(buffer), b->hash);
}

int block_sign(Block *b) {
    char buffer[512];
    block_serialize(b, buffer, sizeof(buffer));
    return ecdsa_sign((unsigned char *)buffer, strlen(buffer), b->signature, &b->sig_len);
}

int block_verify_signature(const Block *b) {
    char buffer[512];
    block_serialize(b, buffer, sizeof(buffer));
    return ecdsa_verify((unsigned char *)buffer, strlen(buffer), b->signature, b->sig_len);
}

void chain_create_genesis(Blockchain *chain) {
    Block genesis;
    memset(&genesis, 0, sizeof(Block));

    genesis.index = 0;
    genesis.timestamp = time(NULL);
    snprintf(genesis.book_id, BOOK_ID_LEN, "N/A");
    snprintf(genesis.book_title, TITLE_LEN, "GENESIS BLOCK");
    snprintf(genesis.member_id, MEMBER_ID_LEN, "N/A");
    snprintf(genesis.member_name, NAME_LEN, "N/A");
    snprintf(genesis.action, ACTION_LEN, "GENESIS");
    snprintf(genesis.previous_hash, HASH_LEN, "%s", GENESIS_HASH);

    block_sign(&genesis);
    block_compute_hash(&genesis);

    chain->blocks[0] = genesis;
    chain->length = 1;
}

/* Find the index of the most recent BORROWED block for a given book_id
 * that has not already been matched by a later RETURNED block.
 * Returns the block index, or -1 if the book is not currently on loan. */
int chain_find_last_borrow_index(const Blockchain *chain, const char *book_id) {
    int last_borrow = -1;

    for (int i = 0; i < chain->length; i++) {
        if (strcmp(chain->blocks[i].book_id, book_id) != 0) continue;

        if (strcmp(chain->blocks[i].action, "BORROWED") == 0) {
            last_borrow = i;
        } else if (strcmp(chain->blocks[i].action, "RETURNED") == 0) {
            last_borrow = -1; /* the most recent borrow has been closed */
        }
    }

    return last_borrow;
}

int chain_add_borrow(Blockchain *chain, const Book books[], int book_count,
                      const Member members[], int member_count,
                      const char *book_id, const char *member_id) {
    int bi = find_book(books, book_count, book_id);
    int mi = find_member(members, member_count, member_id);

    if (bi == -1 || mi == -1) {
        printf("ERROR: Book or Member not found\n");
        return -1;
    }

    if (chain_find_last_borrow_index(chain, book_id) != -1) {
        printf("ERROR: Book '%s' is already on loan and has not been returned.\n", book_id);
        return -1;
    }

    if (chain->length >= MAX_CHAIN_BLOCKS) {
        printf("ERROR: Blockchain is full.\n");
        return -1;
    }

    Block b;
    memset(&b, 0, sizeof(Block));

    b.index = chain->length;
    b.timestamp = time(NULL);
    snprintf(b.book_id, BOOK_ID_LEN, "%s", books[bi].book_id);
    snprintf(b.book_title, TITLE_LEN, "%s", books[bi].title);
    snprintf(b.member_id, MEMBER_ID_LEN, "%s", members[mi].member_id);
    snprintf(b.member_name, NAME_LEN, "%s", members[mi].full_name);
    snprintf(b.action, ACTION_LEN, "BORROWED");
    snprintf(b.previous_hash, HASH_LEN, "%s", chain->blocks[chain->length - 1].hash);

    if (block_sign(&b) != 0) {
        printf("ERROR: Failed to sign block.\n");
        return -1;
    }
    block_compute_hash(&b);

    chain->blocks[chain->length] = b;
    chain->length++;

    printf("SUCCESS: '%s' borrowed by %s (%s). Block #%d added.\n",
           books[bi].title, members[mi].full_name, member_id, b.index);
    return 0;
}

int chain_add_return(Blockchain *chain, const char *book_id) {
    int borrow_idx = chain_find_last_borrow_index(chain, book_id);

    if (borrow_idx == -1) {
        printf("ERROR: No active loan found for book '%s' "
               "(never borrowed, invalid ID, or already returned).\n", book_id);
        return -1;
    }

    if (chain->length >= MAX_CHAIN_BLOCKS) {
        printf("ERROR: Blockchain is full.\n");
        return -1;
    }

    Block *origin = &chain->blocks[borrow_idx];

    Block b;
    memset(&b, 0, sizeof(Block));

    b.index = chain->length;
    b.timestamp = time(NULL);
    snprintf(b.book_id, BOOK_ID_LEN, "%s", origin->book_id);
    snprintf(b.book_title, TITLE_LEN, "%s", origin->book_title);
    snprintf(b.member_id, MEMBER_ID_LEN, "%s", origin->member_id);
    snprintf(b.member_name, NAME_LEN, "%s", origin->member_name);
    snprintf(b.action, ACTION_LEN, "RETURNED");
    snprintf(b.previous_hash, HASH_LEN, "%s", chain->blocks[chain->length - 1].hash);

    if (block_sign(&b) != 0) {
        printf("ERROR: Failed to sign block.\n");
        return -1;
    }
    block_compute_hash(&b);

    chain->blocks[chain->length] = b;
    chain->length++;

    printf("SUCCESS: '%s' returned by %s. Block #%d added.\n",
           origin->book_title, origin->member_name, b.index);
    return 0;
}

/* Recomputes each block's hash from its fields and confirms:
 *  (a) the stored hash matches the recomputed hash (no tampering)
 *  (b) previous_hash correctly links to the prior block
 *  (c) the digital signature verifies
 * Returns 1 if the whole chain is valid, 0 otherwise. */
int chain_validate(const Blockchain *chain, int verbose) {
    int valid = 1;

    for (int i = 0; i < chain->length; i++) {
        const Block *b = &chain->blocks[i];
        char recomputed[HASH_LEN];

        char buffer[512];
        block_serialize(b, buffer, sizeof(buffer));
        sha256_hex((unsigned char *)buffer, strlen(buffer), recomputed);

        int hash_ok = (strcmp(recomputed, b->hash) == 0);
        int link_ok = 1;
        if (i > 0) {
            link_ok = (strcmp(b->previous_hash, chain->blocks[i - 1].hash) == 0);
        }
        int sig_ok = block_verify_signature(b);

        if (!hash_ok || !link_ok || !sig_ok) {
            valid = 0;
        }

        if (verbose) {
            printf("Block #%d: hash=%s link=%s signature=%s\n",
                   i,
                   hash_ok ? "OK" : "TAMPERED",
                   link_ok ? "OK" : "BROKEN",
                   sig_ok ? "VALID" : "INVALID");
        }
    }

    return valid;
}

static const char *format_time(time_t t) {
    static char buf[32];
    struct tm *tm_info = localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    return buf;
}

void chain_print_records(const Blockchain *chain) {
    printf("\n%-4s %-10s %-22s %-20s %-9s %-20s %-8s\n",
           "IDX", "ACTION", "BOOK", "MEMBER", "ID", "TIMESTAMP", "SIG");
    printf("--------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < chain->length; i++) {
        const Block *b = &chain->blocks[i];
        int sig_ok = block_verify_signature(b);

        printf("%-4d %-10s %-22.22s %-20.20s %-9s %-20s %-8s\n",
               b->index,
               b->action,
               b->book_title,
               b->member_name,
               b->member_id,
               format_time(b->timestamp),
               sig_ok ? "VALID" : "INVALID");
    }
    printf("\n");
}

/* ---------- Persistence ---------- */

int chain_save(const Blockchain *chain, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "ERROR: Could not open '%s' for writing.\n", filename);
        return -1;
    }

    fwrite(&chain->length, sizeof(int), 1, fp);
    fwrite(chain->blocks, sizeof(Block), (size_t)chain->length, fp);

    fclose(fp);
    return 0;
}

int chain_load(Blockchain *chain, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return -1; /* no saved chain yet -- not an error on first run */
    }

    chain_init(chain);

    if (fread(&chain->length, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        chain_init(chain);
        return -1;
    }

    if (chain->length < 0 || chain->length > MAX_CHAIN_BLOCKS) {
        fclose(fp);
        chain_init(chain);
        return -1;
    }

    size_t read_count = fread(chain->blocks, sizeof(Block), (size_t)chain->length, fp);
    fclose(fp);

    if ((int)read_count != chain->length) {
        chain_init(chain);
        return -1;
    }

    return 0;
}

/* Demonstrates tamper detection by directly modifying the data field
 * of a past block WITHOUT recomputing its hash or signature -- exactly
 * what an attacker (e.g. a librarian editing a record) would attempt.
 * Operates on an in-memory COPY of the chain so the real, persisted
 * ledger is never actually corrupted by running this demo. */
void chain_tamper_demo(Blockchain *chain) {
    if (chain->length < 2) {
        printf("Not enough blocks to demonstrate tampering. Borrow/return a book first.\n");
        return;
    }

    Blockchain copy = *chain; /* struct copy -- the original `chain` is untouched */
    int target = 1; /* first real transaction block, after genesis */

    printf("\nBEFORE TAMPERING:\n");
    chain_validate(&copy, 1);

    char original_action[ACTION_LEN];
    snprintf(original_action, ACTION_LEN, "%s", copy.blocks[target].action);

    printf("\n>> Attacker silently changes Block #%d's action from '%s' to 'RETURNED' "
           "without re-signing or re-hashing (demo copy only -- your saved ledger is untouched)...\n",
           target, original_action);
    snprintf(copy.blocks[target].action, ACTION_LEN, "RETURNED");

    printf("\nAFTER TAMPERING:\n");
    int still_valid = chain_validate(&copy, 1);

    if (!still_valid) {
        printf("\n*** TAMPER DETECTED: Chain validation FAILED. ***\n"
               "The recomputed hash of Block #%d no longer matches its stored hash,\n"
               "and its digital signature no longer verifies against the modified data.\n", target);
    } else {
        printf("\nUnexpected: tampering was not detected.\n");
    }
}
