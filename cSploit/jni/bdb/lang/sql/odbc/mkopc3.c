/*
 * Make opcodes.[ch] from standard input and parse.h.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct tk {
    struct tk *next, *other;
    char *name;
    int val;
    int nopush;
};

static int
opsort(const void *a, const void *b)
{
    struct tk *ta = (struct tk *) a;
    struct tk *tb = (struct tk *) b;

    return ta->val - tb->val;
}

static void
append(struct tk **head, struct tk *tk)
{
    struct tk *h = *head;

    tk->next = NULL;
    if (h != NULL) {
	while (h->next != NULL) {
	    h = h->next;
	}
	h->next = tk;
    } else {
	*head = tk;
    }
}

static int nopush[10];

int
main(int argc, char **argv)
{
    FILE *c_file, *h_file, *p_file, *a_file;
    char line[512];
    char used[1024];
    int count = 0, last_val, max_val = -1, nop = 0;
    struct tk *tk = NULL;
    struct tk *op = NULL, *opa = NULL;

    memset(used, 0, sizeof (used));
    p_file = fopen("parse.h", "r");
    c_file = fopen("opcodes.c", "w");
    h_file = fopen("opcodes.h", "w");
    if (p_file == NULL) {
	fprintf(stderr, "unable to open parse.h\n");
	exit(1);
    }
    if (c_file == NULL || h_file == NULL) {
	fprintf(stderr, "unable to open output file(s)\n");
	exit(1);
    }
    while (fgets(line, sizeof (line), p_file) != NULL) {
	if (strncmp(line, "#define TK_", 11) == 0) {
	    char name[64];
	    int val;

	    if (sscanf(line, "#define %s%d", name, &val) == 2) {
		struct tk *tkn = malloc(sizeof (struct tk) + strlen(name) + 1);

		tkn->next = tk;
		tkn->other = NULL;
		tkn->name = (char *) (tkn + 1);
		tkn->val = val;
		tkn->nopush = 0;
		strcpy(tkn->name, name);
		tk = tkn;
		if (val > max_val) {
		    max_val = val;
		}
	    }
	}
    }
    fclose(p_file);
    a_file = fopen("addopcodes.awk", "r");
    if (a_file) {
	p_file = fopen("parse.h", "a");
	while (fgets(line, sizeof (line), a_file) != NULL) {
	    if (strstr(line, "printf") && strstr(line, "#define TK_")) {
		char *name = NULL, *p;

		for (p = line, count = 0; p != NULL && count < 3; count++) {
		    p = strchr(p, '"');
		    if (p) {
			++p;
		    }
		}
		if (p != NULL) {
		    char *q = strchr(p, '"');

		    if (q != NULL) {
			name = p;
			*q = '\0';
		    }
		}
		if (name != NULL) {
		    struct tk *tkn = malloc(sizeof (struct tk) +
					    strlen(name) + 3 + 1);

		    fprintf(p_file, "#define TK_%-27s %3d\n", name, ++max_val);
		    tkn->next = tk;
		    tkn->other = NULL;
		    tkn->name = (char *) (tkn + 1);
		    tkn->val = max_val;
		    tkn->nopush = 0;
		    strcpy(tkn->name, "TK_");
		    strcat(tkn->name, name);
		    tk = tkn;
		}
	    }
	}
	fclose(a_file);
	fclose(p_file);
    }
    while (fgets(line, sizeof (line), stdin) != NULL) {
	if (strncmp(line, "case OP_", 7) == 0) {
	    char *p, *q, *qq;
	    struct tk *opn;

	    p = line + 5;
	    q = strchr(p, ':');
	    if (q == NULL) {
		continue;
	    }
	    *q = '\0';
	    opn = malloc(sizeof (struct tk) + strlen(p) + 1);
	    opn->next = opn->other = NULL;
	    opn->name = (char *) (opn + 1);
	    opn->val = -1;
	    opn->nopush = 0;
	    strcpy(opn->name, p);
	    append(&op, opn);
	    qq = strstr(q + 1, "same as TK_");
	    if (qq != NULL) {
		qq = strstr(qq, "TK_");
		q = qq + 3;
	    }
	    if (qq != NULL) {
		struct tk *tkp;

		for (tkp = tk; tkp != NULL; tkp = tkp->next) {
		    if (memcmp(qq, tkp->name, strlen(tkp->name)) == 0) {
			opn->other = tkp;
			opn->val = tkp->val;
			used[opn->val] = 1;
			break;
		    }
		}
	    }
	    qq = strstr(q + 1, "no-push");
	    if (qq != NULL) {
		opn->nopush = 1;
	    }
	}
    }
    if (op != NULL) {
	struct tk *opp;

	for (opp = op; opp != NULL; opp = opp->next) {
	    if (opp->val < 0) {
		++count;
		while (used[count]) {
		    count++;
		}
		opp->val = count;
	    }
	    ++nop;
	}
	opa = malloc(sizeof (struct tk) * nop);
	nop = 0;
	for (opp = op; opp != NULL; opp = opp->next) {
	    opa[nop++] = *opp;
	}
	qsort(opa, nop, sizeof (struct tk), opsort);
    }
    fprintf(c_file, "/* Automatically generated file. Do not edit */\n");
    fprintf(c_file, "char *sqliteOpcodeNames[] = {\n");
    fprintf(h_file, "/* Automatically generated file. Do not edit */\n");
    for (count = 0, last_val = -1; count < nop; count++) {
	while (++last_val < opa[count].val) {
	    fprintf(c_file, "  \"???\",\n");
	}
	fprintf(c_file, "  \"%s\",\n", opa[count].name + 3);
	fprintf(h_file, "#define %-30s %3d",
		opa[count].name, opa[count].val);
	if (opa[count].other != NULL) {
	    fprintf(h_file, "    /* same as %s */", opa[count].other->name);
	}
	fprintf(h_file, "\n");
    }
    fprintf(c_file, "};\n");
    for (count = 0; count < nop; count++) {
	if (opa[count].nopush) {
	    int bit = opa[count].val % 16;
	    int off = opa[count].val / 16;
	    nopush[off] |= 1 << bit;
	}
    }
    for (count = 0; count < 10; count++) {
	fprintf(h_file, "#define NOPUSH_MASK_%d %d\n", count, nopush[count]);
    }
    fclose(h_file);
    fclose(c_file);
    return 0;
}
