# Blockchain-Based Library Book Lending Tracker

Individual Assignment 1 - Deng Mayen Deng Akol
African Leadership University, Software Engineering

## Required libraries and dependencies

- GCC with C11 support
- OpenSSL 3.x development headers (libssl-dev) - used for SHA-256
  hashing and ECDSA digital signatures, via OpenSSL's EVP API
- make (not strictly required, but the build is set up to use it)

No other third-party libraries are used. Everything else is standard C.

### Installing on Ubuntu / Debian / WSL

sudo apt-get update
sudo apt-get install -y build-essential libssl-dev

### Installing on macOS (Homebrew)

brew install openssl
export CPPFLAGS="-I$(brew --prefix openssl)/include"
export LDFLAGS="-L$(brew --prefix openssl)/lib"

## Compilation instructions

From the project root, just run:

make

This compiles all four source files (main.c, blockchain.c,
crypto_utils.c, registry.c) with -Wall -Wextra -std=c11 and links
against OpenSSL (-lssl -lcrypto). It produces a binary called
library_tracker in the project root.

If you want to compile by hand instead of using make:

gcc -Wall -Wextra -std=c11 -Iinclude -c src/main.c -o build/main.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/blockchain.c -o build/blockchain.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/crypto_utils.c -o build/crypto_utils.o
gcc -Wall -Wextra -std=c11 -Iinclude -c src/registry.c -o build/registry.o
gcc build/main.o build/blockchain.o build/crypto_utils.o build/registry.o -o library_tracker -lssl -lcrypto

To rebuild from a clean state (also wipes the saved chain and keys):

make clean
make

## How to build and run the application

Build it with make as shown above, then run:

./library_tracker

or:

make run

On the very first run, the program:
1. Loads and checks books.txt and members.txt
2. Generates an EC (P-256) keypair and saves it under keys/, since
   none exists yet
3. Creates a new chain with a genesis block, since data/chain.dat
   doesn't exist yet

On every run after that, it loads the existing chain from
data/chain.dat and reuses the existing keys, so records and
signatures made in earlier sessions stay intact.

You'll see a menu:

1. Borrow a book
2. Return a book
3. View all records
4. Validate the chain
5. List books
6. List members
7. Tamper detection demo (runs on a copy, doesn't touch your saved data)
0. Save and exit

Borrowing or returning a book asks for a Book ID and/or Member ID.
Typing an ID that isn't in the registry gives you an error and
nothing gets added to the chain.

## Folder layout

library-blockchain/
- Makefile
- README.md
- include/blockchain.h
- src/main.c
- src/blockchain.c
- src/crypto_utils.c
- src/registry.c
- data/books.txt
- data/members.txt
- data/chain.dat        (created at runtime)
- keys/                 (created at runtime)
- docs/system_design.md
