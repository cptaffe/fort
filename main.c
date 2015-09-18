
/*
*
* fort
* A FORTRAN interpreter.
*
* Copyright (c) 2015 Connor Taffe <cpaynetaffe@gmail.com>
* Licensed under the MIT license <https://opensource.org/licenses/MIT>
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Secion 3, Program Form
// Categories dealing with characters:
enum {
	CHAR_DIGITS,      // Section 3.1.1
	CHAR_LETTERS,     // Section 3.1.2
	// Section 3.1.3, Alphanumeric, is CHAR_DIGITS | CHAR_LETTERS
	CHAR_SPECIAL      // Section 3.1.4
};

char *chars[] = {
	[CHAR_DIGITS] = "0123456789",
	[CHAR_LETTERS] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
	[CHAR_SPECIAL] = " =+-*/(),.$"
};

struct HashTableEntry {
	char *key;
	void *value;
};

typedef struct {
	struct HashTableEntry *buckets;
	int nbuckets;
} HashTable;

uint64_t hash(char *key) {
	// FNV-1a algorithm
	uint64_t hash = 14695981039346656037u;
	while (*key) {
		hash ^= *key;
		hash *= 1099511628211u;
		key++;
	}
	return hash;
}

void putHashTable(HashTable *h, struct HashTableEntry e) {
	h->buckets[hash(e.key) % h->nbuckets] = e;
}

struct HashTableEntry fetchHashTable(HashTable *h, char *key) {
	return h->buckets[hash(key) % h->nbuckets];
}

int main() {
	enum { hashTableSize = 0x10000 };
	HashTable h = {
		.buckets = calloc(sizeof(struct HashTableEntry), hashTableSize),
		.nbuckets = hashTableSize
	};

	for (;;) {
		char *key = calloc(sizeof(char), 100), *val = calloc(sizeof(char), 100);
		int nitem = scanf("%s%s", key, val);
		if (nitem != 2) {
			printf("%d\n", nitem);
			break;
		}
		putHashTable(&h, (struct HashTableEntry){
			.key = key,
			.value = (void *) val
		});
	}

	for (int i = 0; i < h.nbuckets; i++) {
		if (h.buckets[i].key) {
			printf("%d: %s, %s\n", i, h.buckets[i].key, (char *) h.buckets[i].value);
		}
	}
}
