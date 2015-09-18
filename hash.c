
/*
 * hash
 * HashTable for storing string indexed values.
 * Put and Fetch operations allow the storage
 * and retrieval (w/ removal) of values.
 * Hashing is done using the FNV-1a algorithm.
 */

struct HashTableEntry {
	char *key;
	void *value;
};

typedef struct {
	struct HashTableEntry *buckets;
	int nbuckets;
} HashTable;

static uint64_t hash(char *key) {
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

void hashTablePut(HashTable *h, struct HashTableEntry e) {
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
					hashTablePut(h, en[i]);
				}
			}
			free(en);
			// Defer putting to new invocation of putHashTable
			return hashTablePut(h, e);
		}
	}
	h->buckets[i] = e;
}

struct HashTableEntry hashTableFetch(HashTable *h, char *key) {
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
