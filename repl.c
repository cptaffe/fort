
typedef struct {
	FILE *input;
	FILE *output;
} Repl;

static void prompt(Repl *r) {
	const char *p = ">>> ";
	fprintf(r->output, "%s", p);
	fflush(r->output);
}

void initRepl(Repl *r) {
	prompt(r);
}

int nextRepl(Repl *r, bool prompOnNewline) {
	int c = fgetc(r->input);
	if (prompOnNewline && c == '\n') {
		prompt(r);
	}
	return c;
}

void backRepl(Repl *r, int c) {
	ungetc(c, r->input);
}
