
typedef struct Tree {
	struct Tree *left, *right;
	int type; // type of node
	int depth;
} Tree;

enum {
	TREE_TYPE_NONE,
	TREE_TYPE_GOTO,
};

typedef Tree *(AppendFunc)(Tree *, Tree *);

// Forward declare functions
AppendFunc appendNode, appendGotoNode;

// Specific appends for node types.
AppendFunc *appends[] = {
	[TREE_TYPE_NONE] = appendNode,
	[TREE_TYPE_GOTO] = appendGotoNode
};

// Default append function
// Returns null on failure, c on empty slot fill.
Tree *appendNode(Tree *t, Tree *c) {
	if (t->left == NULL || t->right == NULL) {
		if (t->left == NULL) {
			t->left = c;
		} else {
			t->right = c;
		}
		return c;
	}
	return NULL;
}

Tree *appendGotoNode(Tree *t, Tree *c) {
	printf("woot");
	return NULL;
}

Tree *appendTree(Tree *t, Tree *c) {
	assert(t != NULL && c != NULL);
	AppendFunc *a = appends[t->type];
	if (a != NULL) {
		return appends[t->type](t, c);
	} else {
		// Default append function
		return NULL;
	}
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
