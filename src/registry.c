/* ============================================================
 * registry.c
 * Loading and lookup of the Book and Member registries.
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "../include/blockchain.h"

/* Trim trailing \n / \r from a line read with fgets */
static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

/* Load books.txt (format: book_id,title,author) into an array of Book.
 * Returns the number of books loaded, or -1 on file error. */
int load_books(const char *filename, Book books[], int max_books) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: Could not open book registry file '%s'\n", filename);
        return -1;
    }

    char line[300];
    int count = 0;

    while (fgets(line, sizeof(line), fp) && count < max_books) {
        trim_newline(line);
        if (strlen(line) == 0) continue; /* skip blank lines */

        char *id = strtok(line, ",");
        char *title = strtok(NULL, ",");
        char *author = strtok(NULL, ",");

        if (!id || !title || !author) {
            fprintf(stderr, "WARNING: Skipping malformed line in %s: %s\n", filename, line);
            continue;
        }

        snprintf(books[count].book_id, BOOK_ID_LEN, "%s", id);
        snprintf(books[count].title, TITLE_LEN, "%s", title);
        snprintf(books[count].author, AUTHOR_LEN, "%s", author);
        count++;
    }

    fclose(fp);

    if (count == 0) {
        fprintf(stderr, "ERROR: Book registry file '%s' is empty or invalid.\n", filename);
        return -1;
    }

    return count;
}

/* Load members.txt (format: member_id,full_name,course_code) into an array of Member.
 * Returns the number of members loaded, or -1 on file error. */
int load_members(const char *filename, Member members[], int max_members) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: Could not open member registry file '%s'\n", filename);
        return -1;
    }

    char line[300];
    int count = 0;

    while (fgets(line, sizeof(line), fp) && count < max_members) {
        trim_newline(line);
        if (strlen(line) == 0) continue;

        char *id = strtok(line, ",");
        char *name = strtok(NULL, ",");
        char *course = strtok(NULL, ",");

        if (!id || !name || !course) {
            fprintf(stderr, "WARNING: Skipping malformed line in %s: %s\n", filename, line);
            continue;
        }

        snprintf(members[count].member_id, MEMBER_ID_LEN, "%s", id);
        snprintf(members[count].full_name, NAME_LEN, "%s", name);
        snprintf(members[count].course_code, COURSE_LEN, "%s", course);
        count++;
    }

    fclose(fp);

    if (count == 0) {
        fprintf(stderr, "ERROR: Member registry file '%s' is empty or invalid.\n", filename);
        return -1;
    }

    return count;
}

/* Return the array index of a book_id, or -1 if not found */
int find_book(const Book books[], int count, const char *book_id) {
    for (int i = 0; i < count; i++) {
        if (strcmp(books[i].book_id, book_id) == 0) {
            return i;
        }
    }
    return -1;
}

/* Return the array index of a member_id, or -1 if not found */
int find_member(const Member members[], int count, const char *member_id) {
    for (int i = 0; i < count; i++) {
        if (strcmp(members[i].member_id, member_id) == 0) {
            return i;
        }
    }
    return -1;
}
