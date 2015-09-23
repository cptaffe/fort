
typedef struct Tree {
	struct Tree *left, *right;
	int type; // type of node
	int depth;
} Tree;

enum {
	TREE_TYPE_GOTO,
};

Tree *appendGotoNode(Tree *t, Tree *c) {
	printf("woot");
	return NULL;
}

// Specific appends for node types.
Tree *(*appends[])(Tree *, Tree *) = {
	[TREE_TYPE_GOTO] = appendGotoNode
};

Tree *appendTree(Tree *t, Tree *c) {
	assert(t != NULL && c != NULL);
	return appends[t->type](t, c);
}

typedef struct {
	Lexer *l;
	Tree *root;
} Parser;

void parserParse(Parser *p) {
	struct LexicalToken *t;
	while ((t = lexerStateMachine(p->l))) {
		lexicalTokenPprint(t);
	}
}
