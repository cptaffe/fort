
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

/*
 * Secion 3, Program Form
 * Categories dealing with characters
 */
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

bool charIn(char c, int i) {
	char *digits = chars[i];
	while (*digits) {
		if (c == *digits) {
			return true;
		}
		digits++;
	}
	return false;
}

bool charDigit(char c) {
	return charIn(c, CHAR_DIGITS);
}

bool charLetter(char c) {
	return charIn(c, CHAR_LETTERS);
}

bool charAlphanumeric(char c) {
	return charIn(c, CHAR_DIGITS) | charIn(c, CHAR_LETTERS);
}

bool charSpecial(char c) {
	return charIn(c, CHAR_SPECIAL);
}

enum {
	// Keywords
	_KEYWORD_BEGIN,
	KEYWORD_END,
	KEYWORD_GOTO,
	KEYWORD_IF,
	KEYWORD_CALL,
	KEYWORD_RETURN,
	KEYWORD_CONTINUE,
	KEYWORD_DO,
	_KEYWORD_END,

	// Operators
	_KEYWORD_OP_BEGIN,

	// Arithmetic Expression Operators (Section 6.1)
	_KEYWORD_OP_ARITHMETIC_BEGIN,
	KEYWORD_OP_ADD,
	KEYWORD_OP_SUB,
	KEYWORD_OP_MUL,
	KEYWORD_OP_DIV,
	KEYWORD_OP_EXP,
	_KEYWORD_OP_ARITHMETIC_END,

	// Relational Expression Operators (Section 6.2)
	_KEYWORD_OP_RELATIONAL_BEGIN,
	KEYWORD_OP_LT,
	KEYWORD_OP_LE,
	KEYWORD_OP_EQ,
	KEYWORD_OP_NE,
	KEYWORD_OP_GT,
	KEYWORD_OP_GE,
	_KEYWORD_OP_RELATIONAL_END,

	// Logical Expression Operators (Section 6.3)
	_KEYWORD_OP_LOGICAL_BEGIN,
	KEYWORD_OP_OR,
	KEYWORD_OP_AND,
	KEYWORD_OP_NOT,
	_KEYWORD_OP_LOGICAL_END,

	_KEYWORD_OP_END,
};

char *keywords[] = {
	[KEYWORD_END] = "END",
	[KEYWORD_GOTO] = "GOTO",
	[KEYWORD_IF] = "IF",
	[KEYWORD_CALL] = "CALL",
	[KEYWORD_RETURN] = "RETURN",
	[KEYWORD_CONTINUE] = "CONTINUE",
	[KEYWORD_DO] = "DO",

	// Arithmetic
	[KEYWORD_OP_ADD] = "+",
	[KEYWORD_OP_SUB] = "-",
	[KEYWORD_OP_MUL] = "*",
	[KEYWORD_OP_DIV] = "/",
	[KEYWORD_OP_EXP] = "**",

	// Relational
	[KEYWORD_OP_LT] = ".LT.",
	[KEYWORD_OP_LE] = ".LE.",
	[KEYWORD_OP_EQ] = ".EQ.",
	[KEYWORD_OP_NE] = ".NE.",
	[KEYWORD_OP_GT] = ".GT.",
	[KEYWORD_OP_GE] = ".GE.",

	// Logical
	[KEYWORD_OP_OR] = ".OR.",
	[KEYWORD_OP_AND] = ".AND.",
	[KEYWORD_OP_NOT] = ".NOT."
};

enum {
	MAX_LINE_LENGTH = 72,
	MAX_SYMBOLIC_NAME_LENGTH = 6,
	PREFIX_LENGTH = 6
};

enum {
	LEXICAL_TOKEN_ERROR,
	LEXICAL_TOKEN_COMMENT,
	LEXICAL_TOKEN_END,
	LEXICAL_TOKEN_LABEL,
	LEXICAL_TOKEN_IDENT,
	LEXICAL_TOKEN_KEYWORD,
};

struct LexicalToken {
	int type;
	char *str;
	int col, line;
};

typedef struct {
	FILE *input;
	char *buf;
	int i, sz;
	int col, line;
	void *func;

	struct LexicalToken *toks;
	int toki, ntoks;
} Lexer;

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

struct LexicalToken *lexerEmit(Lexer *l, int type) {
	// Construct token
	struct LexicalToken t = {
		.str = l->buf,
		.type = type,
		.col = l->col,
		.line = l->line
	};
	t.str[l->i] = '\0'; // null terminate

	// Reset lexer
	l->i = 0;
	l->sz = 5;
	l->buf = calloc(sizeof(char), l->sz);

	// Append to toks queue
	if (l->toki >= l->ntoks) {
		l->ntoks = (l->ntoks+1)*2;
		l->toks = calloc(sizeof(struct LexicalToken), l->ntoks);
	}
	l->toks[l->toki] = t;
	return &l->toks[l->toki++];
}

void lexicalTokenPprint(struct LexicalToken *tok) {
	printf("%d:%d (%d) '%s'\n", tok->line+1, tok->col+1, tok->type, tok->str);
}

void error(Lexer *l, char *msg, char *sec) {
	struct LexicalToken *t = lexerEmit(l, LEXICAL_TOKEN_ERROR);
	free(t->str);
	if (sec != NULL) {
		asprintf(&t->str, "Error: %s. (ASA FORTRAN S. %s)", msg, sec);
	} else {
		asprintf(&t->str, "Error: %s.", msg);
	}
}

// Error: Exceeds maximum line length
void errorMaxLineLengthExceeded(Lexer *l, const char *line_type) {
	char *str;
	asprintf(&str, "%s exceeds maximum line length (%d)", line_type, MAX_LINE_LENGTH);
	error(l, str, "3.2");
	free(str);
}

// Error: Early program termination
void errorEarlyProgramTermination(Lexer *l, const char *line_type) {
	char *str;
	asprintf(&str, "%s terminated program, expected End Line.", line_type);
	error(l, str, "3.2.2");
	free(str);
}

/*
 * Section 3.2 Lines
 */
enum {
	LINE_ERROR,
	LINE_COMMENT,        // Section 3.2.1
	LINE_INITIAL_OR_END, // Section 3.2.2, 3.2.3
	LINE_INITIAL,        // Section 3.2.3
	LINE_CONTINUATION    // Section 3.2.4
};

void *stateStart(Lexer *l);

void *stateCommentLine(Lexer *l) {
	int c;
	/*
	 * Read through the rest of the line;
	 * comment terminates on \n.
	 * Watch for early program terminations (EOF).
	 * Error on maximum line length exceedance.
	 */
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
		lexerEmit(l, LEXICAL_TOKEN_COMMENT);
		return stateStart;
	}
}

/*
 * GOTO statement (Section 7.1.2.1)
 * Unconditional, Assigned, and Computed.
 */
void *stateGoto(Lexer *l) {
	int c;
	while ((c = lexerNext(l)) != EOF
		&& c != '\n'
		&& l->col < MAX_LINE_LENGTH) {
		if (charDigit(c)) {
			// Beginning of Unconditional (Section 7.1.2.1.1) or
			// Assigned (Section 7.1.2.1.2)
			// Scan Label
			return NULL;
		} else if (c == '(') {
			// Beginning of Computed (Section 7.1.2.1.3)
			return NULL;
		} else if (c != ' ') {
			char *str;
			asprintf(&str, "Unexpected character '%c' in GO TO Statement", c);
			error(l, str, "7.1.2.1");
			free(str);
			return NULL;
		}
	}
	if (c == EOF) {
		errorEarlyProgramTermination(l, "GO TO Statement");
		return NULL;
	}
	return NULL;
}

void *stateLine(Lexer *l);

/*
 * Symbolic Names (Section 3.5, 10.1-10.1.10)
 *
 * Maximum of MAX_SYMBOLIC_NAME_LENGTH characters long.
 */
void *stateIdentOrKeyword(Lexer *l) {
	// One character already lexed.
	int c, seen = 0;
	char ident[7] = {};
	while ((c = lexerPeek(l)) != EOF
		&& c != '\n'
		&& l->col < MAX_LINE_LENGTH) {
		if (charLetter(c)) {
			// Record identifier
			ident[seen] = c;
			seen++;
			if (seen > MAX_SYMBOLIC_NAME_LENGTH) {
				char *str;
				asprintf(&str, "Identifier exceeds maximum Symbolic Name length (%d)", MAX_SYMBOLIC_NAME_LENGTH);
				error(l, str, "3.5");
				free(str);
				return NULL;
			}
		} else if (c != ' ') {
			// Junk characters, signals end of number, break.
			break;
		}
		lexerNext(l);
	}
	if (c == EOF) {
		errorEarlyProgramTermination(l, "Symbolic Name");
		return NULL;
	} else {
		for (int i = _KEYWORD_BEGIN+1; i < _KEYWORD_END; i++) {
			if (strcmp(ident, keywords[i]) == 0) {
				lexerEmit(l, LEXICAL_TOKEN_KEYWORD);
				// Lex appropriate statement.
				if (i == KEYWORD_DO) {
					return NULL;
				} else if (i == KEYWORD_IF) {
					return NULL;
				} else if (i == KEYWORD_END) {
					return NULL;
				} else if (i == KEYWORD_CALL) {
					return NULL;
				} else if (i == KEYWORD_GOTO) {
					return stateGoto;
				}
				// Unreachable
				assert(false);
			}
		}
		// Lexed up to non-digit.
		lexerEmit(l, LEXICAL_TOKEN_IDENT);
		return stateLine;
	}
	return NULL;
}

/*
 * Combined End (Section 3.2.2), Initial (Section 3.2.3),
 * and Continuation (Section 3.2.4) lines.
 *
 * End and Continuation lines cannot have labels (handled later).
 */
void *stateLine(Lexer *l) {
	int c;
	while ((c = lexerPeek(l)) != EOF
		&& c != '\n'
		&& l->col < MAX_LINE_LENGTH) {
		// Lex line and stuff
		if (charLetter(c)) {
			return stateIdentOrKeyword;
		} else {
			error(l, "Unknown Character in Line", "3.1");
			return NULL;
		}
		lexerNext(l);
	}
	if (l->col == MAX_LINE_LENGTH) {
		// Error: Line exceeds maximum length
		errorMaxLineLengthExceeded(l, "End Line");
	}
	return NULL;
}

/*
 * Statement Label (Section 3.4)
 *
 * Optionally prefixes a line, but not an End Line
 * or Continuation Line.
 *
 * Used as reference points for GOTO, DO, etc.
 */
void *statePrefixLabel(Lexer *l) {
	int c;
	while ((c = lexerNext(l)) != EOF
		&& c != '\n'
		&& ((l->col < 5 && charDigit(c))
			// column 5, '0' or ' ' only
			|| (l->col == 5 && c == '0')
			|| c == ' ')
		&& l->col < PREFIX_LENGTH) {}
	if (c == EOF) {
		errorEarlyProgramTermination(l, "Label in Line Prefix");
		return NULL;
	} else if (c == '\n') {
		error(l, "Line terminated before Line Prefix ended", "3.2.2");
		return NULL;
	} else if (l->col != PREFIX_LENGTH) {
		error(l, "Malformed Label", "3.4");
		return NULL;
	}
	// Emit label
	lexerEmit(l, LEXICAL_TOKEN_LABEL);
	// Only Initial lines can have labels.
	return stateLine;
}

/*
 * Lines (Section 3.2)
 * Columns 1-6 of a FORTRAN Line.
 */
void *statePrefix(Lexer *l) {
	int c = lexerNext(l);
	if (c == 'C') {
		// Comment
		return stateCommentLine;
	}
	while (c != EOF && c != '\n' && l->col < PREFIX_LENGTH) {
		// Label or solid blanks
		if (charDigit(c)) {
			// Found a Label
			return statePrefixLabel;
		} else if (c == ' ') {
			// Blank, ignored
		} else {
			char *str;
			asprintf(&str, "Unexpected character '%c' in Line Prefix", c);
			error(l, str, "3.2");
			free(str);
			return NULL;
		}
		c = lexerNext(l);
	}
	if (c == EOF) {
		errorEarlyProgramTermination(l, "Line Prefix");
		return NULL;
	} else if (c == '\n') {
		if (l->i == 1) {
			// Empty line, dump it and move on.
			l->i = 0;
			return stateStart;
		} else {
			error(l, "Truncated Line Prefix", "3.2");
			return NULL;
		}
	} else {
		// Initial or End line.
		return stateLine;
	}
}

void *stateStart(Lexer *l) {
	return statePrefix;
}

struct LexicalToken *lexerStateMachine(Lexer *l) {
	while (l->func != NULL && l->toki == 0) {
		l->func = ((void*(*)(Lexer*)) l->func)(l);
		while (l->toki) {
			l->toki--;
			lexicalTokenPprint(&l->toks[l->toki]);
		}
	}
	if (l->toki > 0) {
		l->toki--;
		return &l->toks[l->toki];
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
		.input = stdin,
		.func = stateStart
	};

	struct LexicalToken *t;
	while ((t = lexerStateMachine(&l))) {
		lexicalTokenPprint(t);
	}

	free(l.buf);
	free(h.buckets);
}
