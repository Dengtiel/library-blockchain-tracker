/* ============================================================
 * main.c
 * CLI entry point for the Blockchain-Based Library Lending Tracker.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "../include/blockchain.h"

static Book books[MAX_BOOKS];
static Member members[MAX_MEMBERS];
static int book_count = 0;
static int member_count = 0;
static Blockchain chain;

static void print_menu(void) {
    printf("\n==================== Library Lending Tracker ====================\n");
    printf("1. Borrow a book\n");
    printf("2. Return a book\n");
    printf("3. View all lending records\n");
    printf("4. Validate blockchain integrity\n");
    printf("5. List registered books\n");
    printf("6. List registered members\n");
    printf("7. Demonstrate tamper detection\n");
    printf("0. Save & Exit\n");
    printf("===================================================================\n");
    printf("Choose an option: ");
}

static void read_line(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    } else {
        buf[0] = '\0';
    }
}

static void list_books(void) {
    printf("\n%-8s %-30s %-20s\n", "ID", "TITLE", "AUTHOR");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < book_count; i++) {
        printf("%-8s %-30.30s %-20.20s\n", books[i].book_id, books[i].title, books[i].author);
    }
}

static void list_members(void) {
    printf("\n%-8s %-25s %-10s\n", "ID", "NAME", "COURSE");
    printf("--------------------------------------------------------\n");
    for (int i = 0; i < member_count; i++) {
        printf("%-8s %-25.25s %-10s\n", members[i].member_id, members[i].full_name, members[i].course_code);
    }
}

int main(void) {
    printf("Blockchain-Based Library Book Lending Tracker\n");
    printf("Initializing...\n");

    /* 1. Load registries */
    book_count = load_books(BOOKS_FILE, books, MAX_BOOKS);
    member_count = load_members(MEMBERS_FILE, members, MAX_MEMBERS);

    if (book_count == -1 || member_count == -1) {
        printf("FATAL: Registries must be present and non-empty before the system can start.\n");
        return 1;
    }
    printf("Loaded %d book(s) and %d member(s) from registry files.\n", book_count, member_count);

    /* 2. Init crypto keys (generate once, reuse thereafter) */
    if (crypto_init_keys() != 0) {
        printf("FATAL: Could not initialize cryptographic keys.\n");
        return 1;
    }

    /* 3. Load or create the chain */
    if (chain_load(&chain, CHAIN_FILE) != 0) {
        printf("No existing chain found. Creating a new chain with a genesis block.\n");
        chain_init(&chain);
        chain_create_genesis(&chain);
    } else {
        printf("Loaded existing chain with %d block(s) from '%s'.\n", chain.length, CHAIN_FILE);
    }

    /* 4. CLI loop */
    char choice[16];
    char book_id[BOOK_ID_LEN];
    char member_id[MEMBER_ID_LEN];

    int running = 1;
    while (running) {
        print_menu();
        read_line(choice, sizeof(choice));

        if (strcmp(choice, "1") == 0) {
            printf("Enter Book ID: ");
            read_line(book_id, sizeof(book_id));
            printf("Enter Member ID: ");
            read_line(member_id, sizeof(member_id));
            chain_add_borrow(&chain, books, book_count, members, member_count, book_id, member_id);

        } else if (strcmp(choice, "2") == 0) {
            printf("Enter Book ID: ");
            read_line(book_id, sizeof(book_id));
            chain_add_return(&chain, book_id);

        } else if (strcmp(choice, "3") == 0) {
            chain_print_records(&chain);

        } else if (strcmp(choice, "4") == 0) {
            int valid = chain_validate(&chain, 1);
            printf("\nOverall chain status: %s\n", valid ? "VALID" : "TAMPERED / INVALID");

        } else if (strcmp(choice, "5") == 0) {
            list_books();

        } else if (strcmp(choice, "6") == 0) {
            list_members();

        } else if (strcmp(choice, "7") == 0) {
            chain_tamper_demo(&chain);

        } else if (strcmp(choice, "0") == 0) {
            running = 0;

        } else {
            printf("Invalid option. Please try again.\n");
        }
    }

    /* 5. Persist and clean up */
    if (chain_save(&chain, CHAIN_FILE) == 0) {
        printf("Chain saved to '%s'. Goodbye!\n", CHAIN_FILE);
    } else {
        printf("WARNING: Could not save chain.\n");
    }

    crypto_cleanup();
    return 0;
}
