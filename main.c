
/*
*
* fort
* A FORTRAN 66 interpreter.
*
* Copyright (c) 2015 Connor Taffe <cpaynetaffe@gmail.com>
* Licensed under the MIT license <https://opensource.org/licenses/MIT>
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

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
	assert(key != NULL);
	while (*key) {
		hash ^= *key;
		hash *= 1099511628211u;
		key++;
	}
	return hash;
}

void putHashTable(HashTable *h, struct HashTableEntry e) {
	uint64_t j, i = hash(e.key) % h->nbuckets;
	j = i;
	while (h->buckets[i].key != NULL) {
		i = (i + 1) % h->nbuckets;
		if (j == i) {
			// Have looked over entire table
			// Expand table and rehash appropriately.
			h->nbuckets *= 2;
			struct HashTableEntry *en = h->buckets;
			h->buckets = calloc(sizeof(struct HashTableEntry), h->nbuckets);
			// Rehash every entry
			for (uint64_t i = 0; i < h->nbuckets/2; i++) {
				if (en[i].key != NULL) {
					putHashTable(h, en[i]);
				}
			}
			free(en);
			// Defer putting to new invocation of putHashTable
			return putHashTable(h, e);
		}
	}
	h->buckets[i] = e;
}

struct HashTableEntry fetchHashTable(HashTable *h, char *key) {
	uint64_t j, i = hash(key) % h->nbuckets;
	j = i;
	while (strcmp(h->buckets[j].key, key) != 0) {
		i = (i + 1) % h->nbuckets;
		if (j == i) {
			// Have looked over entire table
			return (struct HashTableEntry) {};
		}
	}
	return h->buckets[i];
}

int main() {
	enum { hashTableSize = 2 };
	HashTable h = {
		.buckets = calloc(sizeof(struct HashTableEntry), hashTableSize),
		.nbuckets = hashTableSize
	};

	for (;;) {
		char *key = calloc(sizeof(char), 100), *val = calloc(sizeof(char), 100);
		int nitem = scanf("%s%s", key, val);
		if (nitem != 2) {
			free(key);
			free(val);
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
			free(h.buckets[i].key);
			free(h.buckets[i].value);
		}
	}
	free(h.buckets);
}
