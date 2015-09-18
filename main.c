
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

#include "hash.c"

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

typedef struct {
	FILE *input;
	char *buf;
	int i, sz;
	int col, line;
} Lexer;

enum {
	LEXICAL_TOKEN_ERROR,
	LEXICAL_TOKEN_COMMENT
};

struct LexicalToken {
	int type;
	char *str;
	int col, line;
};

int lexerNext(Lexer *l) {
	if (l->i == l->sz) {
		l->sz = (l->sz+1) * 2;
		l->buf = realloc(l->buf, l->sz);
	}
	l->buf[l->i] = fgetc(l->input);
	if (l->buf[l->i] == '\n') {
		l->line++;
		l->col = 0;
	} else {
		l->col++;
	}
	return l->buf[l->i++];
}

int lexerPeek(Lexer *l) {
	int c = fgetc(l->input);
	ungetc(c, l->input);
	return c;
}

struct LexicalToken lexerEmit(Lexer *l) {
	struct LexicalToken t = {
		.str = l->buf,
		.col = l->col,
		.line = l->line
	};
	l->i = 0;
	l->sz = 5;
	l->buf = calloc(sizeof(char), l->sz);
	return t;
}

void lexicalTokenPprint(struct LexicalToken *tok) {
	printf("%d:%d '%s'\n", tok->line+1, tok->col+1, tok->str);
}

void *stateStart(Lexer *l);

void *stateCommentLine(Lexer *l) {
	int c;
	// Must be on 0th column, must be comment line.
	assert(l->col == 0);
	assert(lexerNext(l) == 'C');

	while((c = lexerNext(l)) != EOF && c != '\n' && l->col < 72) {}
	if (l->col == 72) {
		// Exceeds maximum line length (72), error.
		struct LexicalToken t = lexerEmit(l);
		t.type = LEXICAL_TOKEN_ERROR;
		free(t.str);
		t.str = "error: comment line exceeded maximum line length (72).";
		lexicalTokenPprint(&t);
		return NULL;
	}
	// Emit comment line.
	struct LexicalToken *t = calloc(sizeof(struct LexicalToken), 1);
	*t = lexerEmit(l);
	lexicalTokenPprint(t);
	free(t);
	return stateStart;
}

void *stateStart(Lexer *l) {
	int c;
	assert(l->col == 0); // Must be on 0th column
	c = lexerPeek(l);
	if (c == 'C') {
		// Beginning of comment line.
		return (void*) stateCommentLine;
	}
	return NULL;
}

int main() {
	HashTable h;
	Lexer l;
	void *func = stateStart;
	enum { hashTableSize = 2 };
	h = (HashTable) {
		.buckets = calloc(sizeof(struct HashTableEntry), hashTableSize),
		.nbuckets = hashTableSize
	};
	l = (Lexer) {
		.input = stdin
	};

	while (func != NULL) {
		func = ((void*(*)(Lexer*)) func)(&l);
	}
	free(l.buf);
	free(h.buckets);
}
