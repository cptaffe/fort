
/*
 *
 * fort
 * A FORTRAN 66 interpreter.
 *
 * Copyright (c) 2015 Connor Taffe <cpaynetaffe@gmail.com>
 * Licensed under the MIT license <https://opensource.org/licenses/MIT>
 *
 */

#define _GNU_SOURCE // asprintf is useful
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "hash.c"
#include "repl.c"
#include "lex.c"

int main() {
	HashTable h;
	Lexer l;
	Repl r;
	void *func = stateStart;
	enum { hashTableSize = 2 };
	h = (HashTable) {
		.buckets = calloc(sizeof(struct HashTableEntry), hashTableSize),
		.nbuckets = hashTableSize
	};
	r = (Repl) {
		.input = stdin,
		.output = stdout
	};
	initRepl(&r);
	l = (Lexer) {
		.repl = &r,
		.func = stateStart
	};

	struct LexicalToken *t;
	while ((t = lexerStateMachine(&l))) {
		lexicalTokenPprint(t);
	}

	free(l.buf);
	free(h.buckets);
}
