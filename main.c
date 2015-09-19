
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
#include <ctype.h>

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

enum {
	KEYWORD_END
};

char *keywords[] = {
	[KEYWORD_END] = "END"
};

enum {
	MAX_LINE_LENGTH = 72
};

typedef struct {
	FILE *input;
	char *buf;
	int i, sz;
	int col, line;
} Lexer;

enum {
	LEXICAL_TOKEN_ERROR,
	LEXICAL_TOKEN_COMMENT,
	LEXICAL_TOKEN_END,
	LEXICAL_TOKEN_LABEL
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
	t.str[l->i] = '\0'; // null terminate
	l->i = 0;
	l->sz = 5;
	l->buf = calloc(sizeof(char), l->sz);
	return t;
}

void lexicalTokenPprint(struct LexicalToken *tok) {
	printf("%d:%d '%s'\n", tok->line+1, tok->col+1, tok->str);
}

void error(Lexer *l, char *msg) {
	struct LexicalToken t = lexerEmit(l);
	t.type = LEXICAL_TOKEN_ERROR;
	free(t.str);
	t.str = msg;
	lexicalTokenPprint(&t);
}

// Error: Exceeds maximum line length
void errorMaxLineLengthExceeded(Lexer *l, const char *line_type) {
	char *str;
	asprintf(&str, "error: %s exceeds maximum line length (%d).", line_type, MAX_LINE_LENGTH);
	error(l, str);
	free(str);
}

// Error: Early program termination
void errorEarlyProgramTermination(Lexer *l, const char *line_type) {
	char *str;
	asprintf(&str, "error: %s terminated program, expected end line.", line_type);
	error(l, str);
	free(str);
}

// Section 3.2 Lines
enum {
	LINE_ERROR,
	LINE_COMMENT,        // Section 3.2.1
	LINE_INITIAL_OR_END, // Section 3.2.2, 3.2.3
	LINE_INITIAL,        // Section 3.2.3
	LINE_CONTINUATION    // Section 3.2.4
};

enum {
	PREFIX_LENGTH = 6
};

void *stateStart(Lexer *l);

void *stateCommentLine(Lexer *l) {
	int c;
	// Read through the rest of the line;
	// comment terminates on \n.
	// Watch for early program terminations (EOF).
	// Error on maximum line length exceedance.
	while((c = lexerNext(l)) != EOF && c != '\n' && l->col < MAX_LINE_LENGTH) {}
	if (l->col == MAX_LINE_LENGTH) {
		// Error: Exceeds maximum line length
		errorMaxLineLengthExceeded(l, "comment line");
		return NULL;
	} else if (c == EOF) {
		// Error: Early program termination
		errorEarlyProgramTermination(l, "comment line");
		return NULL;
	} else {
		// Emit comment line.
		struct LexicalToken *t = calloc(sizeof(struct LexicalToken), 1);
		*t = lexerEmit(l);
		lexicalTokenPprint(t);
		free(t->str);
		free(t);
		return stateStart;
	}
}

void *stateEndLine(Lexer *l) {
	int c;
	char *keyword = keywords[KEYWORD_END];
	while ((c = lexerNext(l)) != EOF && c != '\n' && l->col < MAX_LINE_LENGTH) {
		if (*keyword && c == *keyword) {
			keyword++;
		} else if (c != ' ') {
			// unexpected character in end line
			char *str;
			asprintf(&str, "error: Unexpected character in end line, expected '%c'.", *keyword);
			error(l, str);
			free(str);
			return NULL;
		}
	}
	if (l->col == MAX_LINE_LENGTH) {
		// Error: Line exceeds maximum length
		errorMaxLineLengthExceeded(l, "end line");
	} else if (*keyword) {
		// Error: Didn't scan entire keyword
		char *str;
		asprintf(&str, "error: Expected keyword '%s', missing.", keyword);
		error(l, str);
		free(str);
		return NULL;
	}
	struct LexicalToken *t = calloc(sizeof(struct LexicalToken), 1);
	*t = lexerEmit(l);
	t->type = LEXICAL_TOKEN_END;
	lexicalTokenPprint(t);
	free(t->str);
	free(t);
	return NULL;
}

void *statePrefixLabel(Lexer *l) {
	int c;
	while ((c = lexerNext(l)) != EOF
		&& c != '\n'
		&& ((l->col < 5 && isdigit(c))
			// column 5, '0' or ' ' only
			|| (l->col == 5 && c == '0')
			|| c == ' ')
		&& l->col < PREFIX_LENGTH) {}
	if (c == EOF) {
		errorEarlyProgramTermination(l, "prefix label");
		return NULL;
	} else if (c == '\n') {
		error(l, "error: Line terminated before prefix ended.");
		return NULL;
	} else if (l->col != PREFIX_LENGTH) {
		error(l, "error: Malformed label.");
		return NULL;
	}
	// Emit label
	struct LexicalToken *t = calloc(sizeof(struct LexicalToken), 1);
	*t = lexerEmit(l);
	t->type = LEXICAL_TOKEN_LABEL;
	lexicalTokenPprint(t);
	free(t->str);
	free(t);
	// Only Initial lines can have labels.
	return NULL;
}

void *statePrefix(Lexer *l) {
	int c = lexerNext(l);
	if (c == 'C') {
		// Comment
		return stateCommentLine;
	}
	do {
		// Label or solid blanks
		if (isdigit(c)) {
			// Found a Label
			return statePrefixLabel;
		} else if (c == ' ') {
			// Blank, ignored
		} else {
			char *str;
			asprintf(&str, "error: Unexpected character '%c' in prefix.", c);
			error(l, str);
			free(str);
			return NULL;
		}
	} while((c = lexerNext(l)) != EOF && c != '\n' && l->col < PREFIX_LENGTH);
	if (c == EOF) {
		errorEarlyProgramTermination(l, "line prefix");
		return NULL;
	} else if (c == '\n') {
		if (l->i == 1) {
			// Empty line, dump it and move on.
			l->i = 0;
			return stateStart;
		} else {
			error(l, "error: Truncated prefix.");
			return NULL;
		}
	} else {
		// Initial or End line.
		return NULL;
	}
}

void *stateStart(Lexer *l) {
	return statePrefix;
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
