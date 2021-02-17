#include "defs.h"

static int nvectors;
static int nentries;
static Yshort **froms;
static Yshort **tos;
static Yshort *conflicts, nconflicts;
static Yshort *tally;
static Yshort *width;
static Yshort *state_count;
static Yshort *order;
static Yshort *base;
static Yshort *pos;
static int maxtable;
static Yshort *table;
static Yshort *check;
static int lowzero;
static int high;


void output()
{
    free_itemsets();
    free_shifts();
    free_reductions();
    output_stored_text();
    output_defines();
    output_rule_data();
    output_yydefred();
    output_actions();
    free_parser();
    output_debug();
    output_stype();
    if (rflag) write_section("tables");
    write_section("header");
    output_trailing_text();
    write_section("body");
    output_semantic_actions();
    write_section("trailer");
}


void output_rule_data()
{
    register int i;
    register int j;

    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yylhs[] = {%42d,",
	    symbol_value[start_symbol]);

    j = 10;
    for (i = 3; i < nrules; i++)
    {
	if (j >= 10)
	{
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1;
	}
        else
	    ++j;

        fprintf(output_file, "%5d,", symbol_value[rlhs[i]]);
    }
    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");

    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yylen[] = {%42d,", 2);

    j = 10;
    for (i = 3; i < nrules; i++)
    {
	if (j >= 10)
	{
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1;
	}
	else
	  j++;

        fprintf(output_file, "%5d,", rrhs[i + 1] - rrhs[i] - 1);
    }
    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
}


void output_yydefred()
{
    register int i, j;

    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yydefred[] = {%39d,",
	    (defred[0] ? defred[0] - 2 : 0));

    j = 10;
    for (i = 1; i < nstates; i++)
    {
	if (j < 10)
	    ++j;
	else
	{
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1;
	}

	fprintf(output_file, "%5d,", (defred[i] ? defred[i] - 2 : 0));
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
}


void output_actions()
{
    nvectors = 3*nstates + nvars;

    froms = NEW2(nvectors, Yshort *);
    tos = NEW2(nvectors, Yshort *);
    tally = NEW2(nvectors, Yshort);
    width = NEW2(nvectors, Yshort);
    if (SRtotal+RRtotal)
	conflicts = NEW2(4*(SRtotal+RRtotal), Yshort);
    else
	conflicts = 0;
    nconflicts = 0;

    token_actions();
    FREE(lookaheads);
    FREE(LA);
    FREE(LAruleno);
    FREE(accessing_symbol);

    goto_actions();
    FREE(goto_map + ntokens);
    FREE(from_state);
    FREE(to_state);

    sort_actions();
    pack_table();
    output_base();
    output_table();
    output_check();
    output_ctable();
}

int find_conflict_base(int cbase)
{
    int i,j;

    for (i=0; i<cbase; i++) {
	for (j=0; j+cbase < nconflicts; j++) {
	    if (conflicts[i+j] != conflicts[cbase+j])
		break; }
	if (j+cbase >= nconflicts)
	    return i; }
    return cbase;
}

void token_actions()
{
    register int i, j;
    register int shiftcount, reducecount, conflictcount, csym, cbase;
    register int max, min;
    register Yshort *actionrow, *r, *s;
    register action *p;

    actionrow = NEW2(3*ntokens, Yshort);
    for (i = 0; i < nstates; ++i) {
	if (parser[i]) {
	    for (j = 0; j < 3*ntokens; ++j)
	    actionrow[j] = 0;

	    shiftcount = 0;
	    reducecount = 0;
	    conflictcount = 0;
	    csym = -1;
	    cbase = nconflicts;
	    for (p = parser[i]; p; p = p->next) {
		if (csym != -1 && csym != p->symbol) {
		    conflictcount++;
		    conflicts[nconflicts++] = -1;
		    j = find_conflict_base(cbase);
		    actionrow[csym + 2*ntokens] = j + 1;
		    if (j == cbase) {
			cbase = nconflicts; }
		    else {
			if (conflicts[cbase] == -1) cbase++;
			nconflicts = cbase; }
		    csym = -1; }
		if (p->suppressed == 0) {
		    if (p->action_code == SHIFT) {
			++shiftcount;
			actionrow[p->symbol] = p->number; }
		    else if (p->action_code == REDUCE &&
			     p->number != defred[i]) {
			++reducecount;
			actionrow[p->symbol + ntokens] = p->number; } }
		else if (p->suppressed == 1) {
		    csym = p->symbol;
		    if (p->action_code == SHIFT) {
			conflicts[nconflicts++] = p->number; }
		    else if (p->action_code == REDUCE &&
			     p->number != defred[i]) {
			if (cbase == nconflicts) {
			    if (cbase) cbase--;
			    else       conflicts[nconflicts++] = -1; }
			conflicts[nconflicts++] = p->number - 2; } } }
	    if (csym != -1) {
		conflictcount++;
		conflicts[nconflicts++] = -1;
		j = find_conflict_base(cbase);
		actionrow[csym + 2*ntokens] = j + 1;
		if (j == cbase) {
		    cbase = nconflicts; }
		else {
		    if (conflicts[cbase] == -1) cbase++;
		    nconflicts = cbase; } }

	    tally[i] = shiftcount;
	    tally[nstates+i] = reducecount;
	    tally[2*nstates+i] = conflictcount;
	    width[i] = 0;
	    width[nstates+i] = 0;
	    width[2*nstates+i] = 0;
	    if (shiftcount > 0) {
		froms[i] = r = NEW2(shiftcount, Yshort);
		tos[i] = s = NEW2(shiftcount, Yshort);
		min = MAXSHORT;
		max = 0;
		for (j = 0; j < ntokens; ++j) {
		    if (actionrow[j]) {
			if (min > symbol_value[j])
			    min = symbol_value[j];
			if (max < symbol_value[j])
			    max = symbol_value[j];
			*r++ = symbol_value[j];
			*s++ = actionrow[j]; } }
		width[i] = max - min + 1; }
	    if (reducecount > 0) {
		froms[nstates+i] = r = NEW2(reducecount, Yshort);
		tos[nstates+i] = s = NEW2(reducecount, Yshort);
		min = MAXSHORT;
		max = 0;
		for (j = 0; j < ntokens; ++j) {
		    if (actionrow[ntokens+j]) {
			if (min > symbol_value[j])
			    min = symbol_value[j];
			if (max < symbol_value[j])
			    max = symbol_value[j];
			*r++ = symbol_value[j];
			*s++ = actionrow[ntokens+j] - 2; } }
		width[nstates+i] = max - min + 1; }
	    if (conflictcount > 0) {
		froms[2*nstates+i] = r = NEW2(conflictcount, Yshort);
		tos[2*nstates+i] = s = NEW2(conflictcount, Yshort);
		min = MAXSHORT;
		max = 0;
		for (j = 0; j < ntokens; ++j) {
		    if (actionrow[2*ntokens+j]) {
			if (min > symbol_value[j])
			    min = symbol_value[j];
			if (max < symbol_value[j])
			    max = symbol_value[j];
			*r++ = symbol_value[j];
			*s++ = actionrow[2*ntokens+j] - 1; } }
		width[2*nstates+i] = max - min + 1; } } }
    FREE(actionrow);
}

void goto_actions()
{
    register int i, j, k;

    state_count = NEW2(nstates, Yshort);

    k = default_goto(start_symbol + 1);
    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yydgoto[] = {%40d,", k);
    save_column(start_symbol + 1, k);

    j = 10;
    for (i = start_symbol + 2; i < nsyms; i++)
    {
	if (j >= 10)
	{
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1;
	}
	else
	    ++j;

	k = default_goto(i);
	fprintf(output_file, "%5d,", k);
	save_column(i, k);
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    FREE(state_count);
}

int default_goto(int symbol)
{
    register int i;
    register int m;
    register int n;
    register int default_state;
    register int max;

    m = goto_map[symbol];
    n = goto_map[symbol + 1];

    if (m == n) return (0);

    for (i = 0; i < nstates; i++)
	state_count[i] = 0;

    for (i = m; i < n; i++)
	state_count[to_state[i]]++;

    max = 0;
    default_state = 0;
    for (i = 0; i < nstates; i++)
    {
	if (state_count[i] > max)
	{
	    max = state_count[i];
	    default_state = i;
	}
    }

    return (default_state);
}



void save_column(int symbol, int default_state)
{
    register int i;
    register int m;
    register int n;
    register Yshort *sp;
    register Yshort *sp1;
    register Yshort *sp2;
    register int count;
    register int symno;

    m = goto_map[symbol];
    n = goto_map[symbol + 1];

    count = 0;
    for (i = m; i < n; i++)
    {
	if (to_state[i] != default_state)
	    ++count;
    }
    if (count == 0) return;

    symno = symbol_value[symbol] + 3*nstates;

    froms[symno] = sp1 = sp = NEW2(count, Yshort);
    tos[symno] = sp2 = NEW2(count, Yshort);

    for (i = m; i < n; i++)
    {
	if (to_state[i] != default_state)
	{
	    *sp1++ = from_state[i];
	    *sp2++ = to_state[i];
	}
    }

    tally[symno] = count;
    width[symno] = sp1[-1] - sp[0] + 1;
}

void sort_actions()
{
  register int i;
  register int j;
  register int k;
  register int t;
  register int w;

  order = NEW2(nvectors, Yshort);
  nentries = 0;

  for (i = 0; i < nvectors; i++)
    {
      if (tally[i] > 0)
	{
	  t = tally[i];
	  w = width[i];
	  j = nentries - 1;

	  while (j >= 0 && (width[order[j]] < w))
	    j--;

	  while (j >= 0 && (width[order[j]] == w) && (tally[order[j]] < t))
	    j--;

	  for (k = nentries - 1; k > j; k--)
	    order[k + 1] = order[k];

	  order[j + 1] = i;
	  nentries++;
	}
    }
}


void pack_table()
{
    register int i;
    register int place;
    register int state;

    base = NEW2(nvectors, Yshort);
    pos = NEW2(nentries, Yshort);

    maxtable = 1000;
    table = NEW2(maxtable, Yshort);
    check = NEW2(maxtable, Yshort);

    lowzero = 0;
    high = 0;

    for (i = 0; i < maxtable; i++)
	check[i] = -1;

    for (i = 0; i < nentries; i++)
    {
	state = matching_vector(i);

	if (state < 0)
	    place = pack_vector(i);
	else
	    place = base[state];

	pos[i] = place;
	base[order[i]] = place;
    }

    for (i = 0; i < nvectors; i++)
    {
	if (froms[i])
	    FREE(froms[i]);
	if (tos[i])
	    FREE(tos[i]);
    }

    FREE(froms);
    FREE(tos);
    FREE(tally);
    FREE(width);
    FREE(pos);
}


/*  The function matching_vector determines if the vector specified by	*/
/*  the input parameter matches a previously considered	vector.  The	*/
/*  test at the start of the function checks if the vector represents	*/
/*  a row of shifts over terminal symbols or a row of reductions, or a	*/
/*  column of shifts over a nonterminal symbol.  Berkeley Yacc does not	*/
/*  check if a column of shifts over a nonterminal symbols matches a	*/
/*  previously considered vector.  Because of the nature of LR parsing	*/
/*  tables, no two columns can match.  Therefore, the only possible	*/
/*  match would be between a row and a column.  Such matches are	*/
/*  unlikely.  Therefore, to save time, no attempt is made to see if a	*/
/*  column matches a previously considered vector.			*/
/*									*/
/*  Matching_vector is poorly designed.  The test could easily be made	*/
/*  faster.  Also, it depends on the vectors being in a specific	*/
/*  order.								*/
/*  Not really any point in checking for matching conflicts -- it is    */
/*  extremely unlikely to occur, and conflicts are (hopefully) rare.    */

int matching_vector(int vector)
{
    register int i;
    register int j;
    register int k;
    register int t;
    register int w;
    register int match;
    register int prev;

    i = order[vector];
    if (i >= 2*nstates)
	return (-1);

    t = tally[i];
    w = width[i];

    for (prev = vector - 1; prev >= 0; prev--)
    {
	j = order[prev];
	if (width[j] != w || tally[j] != t)
	    return (-1);

	match = 1;
	for (k = 0; match && k < t; k++)
	{
	    if (tos[j][k] != tos[i][k] || froms[j][k] != froms[i][k])
		match = 0;
	}

	if (match)
	    return (j);
    }

    return (-1);
}



int pack_vector(int vector)
{
    register int i, j, k, l;
    register int t;
    register int loc;
    register int ok;
    register Yshort *from;
    register Yshort *to;
    int newmax;

    i = order[vector];
    t = tally[i];
    assert(t);

    from = froms[i];
    to = tos[i];

    j = lowzero - from[0];
    for (k = 1; k < t; ++k)
	if (lowzero - from[k] > j)
	    j = lowzero - from[k];
    for (;; ++j)
    {
	if (j == 0)
	    continue;
	ok = 1;
	for (k = 0; ok && k < t; k++)
	{
	    loc = j + from[k];
	    if (loc >= maxtable)
	    {
		if (loc >= MAXTABLE)
		    fatal("maximum table size exceeded");

		newmax = maxtable;
		do { newmax += 200; } while (newmax <= loc);
		table = (Yshort *) REALLOC(table, newmax*sizeof(Yshort));
		if (table == 0) no_space();
		check = (Yshort *) REALLOC(check, newmax*sizeof(Yshort));
		if (check == 0) no_space();
		for (l  = maxtable; l < newmax; ++l)
		{
		    table[l] = 0;
		    check[l] = -1;
		}
		maxtable = newmax;
	    }

	    if (check[loc] != -1)
		ok = 0;
	}
	for (k = 0; ok && k < vector; k++)
	{
	    if (pos[k] == j)
		ok = 0;
	}
	if (ok)
	{
	    for (k = 0; k < t; k++)
	    {
		loc = j + from[k];
		table[loc] = to[k];
		check[loc] = from[k];
		if (loc > high) high = loc;
	    }

	    while (check[lowzero] != -1)
		++lowzero;

	    return (j);
	}
    }
}



void output_base()
{
    register int i, j;

    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yysindex[] = {%39d,", base[0]);
    j = 10;
    for (i = 1; i < nstates; i++) {
	if (j >= 10) {
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1; }
	else
	    ++j;
	fprintf(output_file, "%5d,", base[i]); }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yyrindex[] = {%39d,", base[nstates]);
    j = 10;
    for (i = nstates + 1; i < 2*nstates; i++) {
	if (j >= 10) {
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1; }
	else
	    ++j;
	fprintf(output_file, "%5d,", base[i]); }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yycindex[] = {%39d,", base[2*nstates]);
    j = 10;
    for (i = 2*nstates + 1; i < 3*nstates; i++) {
	if (j >= 10) {
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1; }
	else
	    ++j;
	fprintf(output_file, "%5d,", base[i]); }


    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yygindex[] = {%39d,",
	    base[3*nstates]);
    j = 10;
    for (i = 3*nstates + 1; i < nvectors - 1; i++) {
	if (j >= 10) {
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1; }
	else
	    ++j;
	fprintf(output_file, "%5d,", base[i]); }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    FREE(base);
}



void output_table()
{
    register int i;
    register int j;

    ++outline;

    fprintf(stderr, "YYTABLESIZE: %d\n", high);
    if(high >= MAXSHORT) {
      fprintf(stderr, "Table is longer than %d elements. It's not gonna fly.\n", MAXSHORT);
      exit(1);
    }

    fprintf(code_file, "#define YYTABLESIZE %d\n", high);
    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yytable[] = {%40d,", table[0]);

    j = 10;
    for (i = 1; i <= high; i++)
    {
	if (j >= 10)
	{
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1;
	}
	else
	    ++j;

	fprintf(output_file, "%5d,", table[i]);
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    FREE(table);
}



void output_check()
{
    register int i;
    register int j;

    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yycheck[] = {%40d,", check[0]);

    j = 10;
    for (i = 1; i <= high; i++)
    {
	if (j >= 10)
	{
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1;
	}
	else
	    ++j;

	fprintf(output_file, "%5d,", check[i]);
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    FREE(check);
}

void output_ctable()
{
    register int i;
    register int j;

    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "int yyctable[] = {%39d,", conflicts ?
	    conflicts[0] : 0);

    j = 10;
    for (i = 1; i < nconflicts; i++)
    {
	if (j >= 10)
	{
	    if (!rflag) ++outline;
	    putc('\n', output_file);
	    j = 1;
	}
	else
	    ++j;

	fprintf(output_file, "%5d,", conflicts[i]);
    }
    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    if (conflicts)
	FREE(conflicts);
}


int is_C_identifier(char *name)
{
    register char *s;
    register int c;

    s = name;
    c = *s;
    if (c == '"')
    {
	c = *++s;
	if (!isalpha(c) && c != '_' && c != '$')
	    return (0);
	while ((c = *++s) != '"')
	{
	    if (!isalnum(c) && c != '_' && c != '$')
		return (0);
	}
	return (1);
    }

    if (!isalpha(c) && c != '_' && c != '$')
	return (0);
    while ((c = *++s))
    {
	if (!isalnum(c) && c != '_' && c != '$')
	    return (0);
    }
    return (1);
}


void output_defines()
{
    register int c, i;
    register char *s;
    FILE *dc_file;

    /* ASF: changed to separate (_yacc_defines_keywords and _yacc_defines_yystype instead of
       _yacc_defines_h_) keyword definitions of YYSTYPE because keywords may conflict with other
       things */

    if(dflag) {
      /***
      fprintf(defines_file, "#ifndef _yacc_defines_h_\n");
      fprintf(defines_file, "#define _yacc_defines_h_\n\n");
      ***/
      fprintf(defines_file, "#ifndef _yacc_defines_keywords\n");
      fprintf(defines_file, "#define _yacc_defines_keywords\n\n");
    }

    /* VM: Print to either code file or defines file but not to both */
    dc_file = dflag ? defines_file : code_file;

    for (i = 2; i < ntokens; ++i)
    {
	s = symbol_name[i];
	if (is_C_identifier(s))
	{
	    fprintf(dc_file, "#define ");
	    c = *s;
	    if (c == '"')
	    {
		while ((c = *++s) != '"')
		{
		    putc(c, dc_file);
		}
	    }
	    else
	    {
		do
		{
		    putc(c, dc_file);
		}
		while ((c = *++s));
	    }
	    ++outline;
	    fprintf(dc_file, " %d\n", symbol_value[i]);
	}
    }

    ++outline;
    fprintf(dc_file, "#define YYERRCODE %d\n", symbol_value[1]);

    if(dflag) {
      fprintf(defines_file, "\n#endif /* _yacc_defines_keywords */\n\n");
    }

    if (dflag && unionized)
    {
    fprintf(defines_file, "#ifndef _yacc_defines_yystype\n");
    fprintf(defines_file, "#define _yacc_defines_yystype\n");

	fclose(union_file);
	union_file = fopen(union_file_name, "r");
	if (union_file == NULL) open_error(union_file_name);
	while ((c = getc(union_file)) != EOF) {
	  putc(c, defines_file);
	}
	/* ASF: we define it on the Parser class
	   fprintf(defines_file, "extern YYSTYPE yylval;\n"); */

    fprintf(defines_file, "\n#endif /* _yacc_defines_yystype */\n\n");
    }

    /***
    if(dflag) {
      fprintf(defines_file, "\n#endif\n");
    }
    ***/
}


void output_stored_text()
{
    register int c;
    register FILE *in, *out;
    register int state;	/* 0=middle of line, 1=start of line, 2=seen '#' */

    state = 1;
    fclose(text_file);
    text_file = fopen(text_file_name, "r");
    if (text_file == NULL)
	open_error(text_file_name);
    in = text_file;
    if ((c = getc(in)) == EOF)
	return;
    out = code_file;
    do {
	if (c == '\n') {
	    ++outline;
	    if (state == 2) {
		fprintf(out, line_format+1, outline + 1, code_file_name);
		state = 1;
		continue; }
	    state = 1; }
	else if (state == 1 && c == '#')
	    state = 2;
	else
	    state = 0;
	putc(c, out);
    } while ((c = getc(in)) != EOF);
    if (!lflag)
	fprintf(out, line_format, ++outline + 1, code_file_name);
}


void output_debug()
{
    register int i, j, k, max;
    char **symnam, *s;

    ++outline;
    fprintf(code_file, "#define YYFINAL %d\n", final_state);
    outline += 3;
    fprintf(code_file, "#ifndef YYDEBUG\n#define YYDEBUG %d\n#endif\n",
	    tflag);
    if (rflag)
	fprintf(output_file, "#ifndef YYDEBUG\n#define YYDEBUG %d\n#endif\n",
		tflag);

    max = 0;
    for (i = 2; i < ntokens; ++i)
	if (symbol_value[i] > max)
	    max = symbol_value[i];
    ++outline;
    fprintf(code_file, "#define YYMAXTOKEN %d\n", max);

    symnam = (char **) MALLOC((max+1)*sizeof(char *));
    if (symnam == 0) no_space();

    /* Note that it is  not necessary to initialize the element		*/
    /* symnam[max].							*/
    for (i = 0; i < max; ++i)
	symnam[i] = 0;
    for (i = ntokens - 1; i >= 2; --i)
	symnam[symbol_value[i]] = symbol_name[i];
    symnam[0] = "end-of-file";

    if (!rflag) ++outline;
    fprintf(output_file, "#if YYDEBUG\n");
    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "const char *yyname[] = {");
    j = 80;
    for (i = 0; i <= max; ++i)
    {
	if ((s = symnam[i]))
	{
	    if (s[0] == '"')
	    {
		k = 7;
		while (*++s != '"')
		{
		    ++k;
		    if (*s == '\\')
		    {
			k += 2;
			if (*++s == '\\')
			    ++k;
		    }
		}
		j += k;
		if (j > 80)
		{
		    if (!rflag) ++outline;
		    putc('\n', output_file);
		    j = k;
		}
		fprintf(output_file, "\"\\\"");
		s = symnam[i];
		while (*++s != '"')
		{
		    if (*s == '\\')
		    {
			fprintf(output_file, "\\\\");
			if (*++s == '\\')
			    fprintf(output_file, "\\\\");
			else
			    putc(*s, output_file);
		    }
		    else
			putc(*s, output_file);
		}
		fprintf(output_file, "\\\"\",");
	    }
	    else if (s[0] == '\'')
	    {
		if (s[1] == '"')
		{
		    j += 7;
		    if (j > 80)
		    {
			if (!rflag) ++outline;
			putc('\n', output_file);
			j = 7;
		    }
		    fprintf(output_file, "\"'\\\"'\",");
		}
		else
		{
		    k = 5;
		    while (*++s != '\'')
		    {
			++k;
			if (*s == '\\')
			{
			    k += 2;
			    if (*++s == '\\')
				++k;
			}
		    }
		    j += k;
		    if (j > 80)
		    {
			if (!rflag) ++outline;
			putc('\n', output_file);
			j = k;
		    }
		    fprintf(output_file, "\"'");
		    s = symnam[i];
		    while (*++s != '\'')
		    {
			if (*s == '\\')
			{
			    fprintf(output_file, "\\\\");
			    if (*++s == '\\')
				fprintf(output_file, "\\\\");
			    else
				putc(*s, output_file);
			}
			else
			    putc(*s, output_file);
		    }
		    fprintf(output_file, "'\",");
		}
	    }
	    else
	    {
		k = strlen(s) + 3;
		j += k;
		if (j > 80)
		{
		    if (!rflag) ++outline;
		    putc('\n', output_file);
		    j = k;
		}
		putc('"', output_file);
		do { putc(*s, output_file); } while (*++s);
		fprintf(output_file, "\",");
	    }
	}
	else
	{
	    j += 2;
	    if (j > 80)
	    {
		if (!rflag) ++outline;
		putc('\n', output_file);
		j = 2;
	    }
	    fprintf(output_file, "0,");
	}
    }
    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    FREE(symnam);

    if (!rflag) ++outline;
    if (!rflag)
	fprintf(output_file, "static ");
    fprintf(output_file, "const char *yyrule[] = {\n");
    for (i = 2; i < nrules; ++i)
    {
	fprintf(output_file, "\"%s :", symbol_name[rlhs[i]]);
	for (j = rrhs[i]; ritem[j] > 0; ++j)
	{
	    s = symbol_name[ritem[j]];
	    if (s[0] == '"')
	    {
		fprintf(output_file, " \\\"");
		while (*++s != '"')
		{
		    if (*s == '\\')
		    {
			if (s[1] == '\\')
			    fprintf(output_file, "\\\\\\\\");
			else
			    fprintf(output_file, "\\\\%c", s[1]);
			++s;
		    }
		    else
			putc(*s, output_file);
		}
		fprintf(output_file, "\\\"");
	    }
	    else if (s[0] == '\'')
	    {
		if (s[1] == '"')
		    fprintf(output_file, " '\\\"'");
		else if (s[1] == '\\')
		{
		    if (s[2] == '\\')
			fprintf(output_file, " '\\\\\\\\");
		    else
			fprintf(output_file, " '\\\\%c", s[2]);
		    s += 2;
		    while (*++s != '\'')
			putc(*s, output_file);
		    putc('\'', output_file);
		}
		else
		    fprintf(output_file, " '%c'", s[1]);
	    }
	    else
		fprintf(output_file, " %s", s);
	}
	if (!rflag) ++outline;
	fprintf(output_file, "\",\n");
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "};\n#endif\n");
}


void output_stype()
{
    if (!unionized && ntags == 0)
    {
	outline += 3;
	fprintf(code_file, "#ifndef YYSTYPE\ntypedef int YYSTYPE;\n#endif\n");
    }
}


void output_trailing_text()
{
    register int c, last;
    register FILE *in, *out;

    if (line == 0)
	return;

    in = input_file;
    out = code_file;
    c = *cptr;
    if (c == '\n')
    {
	++lineno;
	if ((c = getc(in)) == EOF)
	    return;
	if (!lflag)
	{
	    ++outline;
	    fprintf(out, line_format, lineno, (inc_file?inc_file_name:input_file_name));
	}
	if (c == '\n')
	    ++outline;
	putc(c, out);
	last = c;
    }
    else
    {
	if (!lflag)
	{
	    ++outline;
	    fprintf(out, line_format, lineno, (inc_file?inc_file_name:input_file_name));
	}
	do { putc(c, out); } while ((c = *++cptr) != '\n');
	++outline;
	putc('\n', out);
	last = '\n';
    }

    while ((c = getc(in)) != EOF)
    {
	if (c == '\n')
	    ++outline;
	putc(c, out);
	last = c;
    }

    if (last != '\n')
    {
	++outline;
	putc('\n', out);
    }
    if (!lflag)
	fprintf(out, line_format, ++outline + 1, code_file_name);
}


void output_semantic_actions()
{
    register int c, last;
    register FILE *out;
    register int state;	/* 0=middle of line, 1=start of line, 2=seen '#' */

    state = 1;
    fclose(action_file);
    action_file = fopen(action_file_name, "r");
    if (action_file == NULL)
	open_error(action_file_name);

    if ((c = getc(action_file)) == EOF)
	return;

    out = code_file;
    do {
	last = c;
	if (c == '\n') {
	    ++outline;
	    if (state == 2) {
		fprintf(out, line_format+1, outline + 1, code_file_name);
		state = 1;
		continue; }
	    state = 1; }
	else if (state == 1 && c == '#')
	    state = 2;
	else
	    state = 0;
	putc(c, out);
    } while ((c = getc(action_file)) != EOF);

    if (last != '\n')
    {
	++outline;
	putc('\n', out);
    }

    if (!lflag)
	fprintf(out, line_format, ++outline + 1, code_file_name);
}


void free_itemsets()
{
    register core *cp, *next;

    FREE(state_table);
    for (cp = first_state; cp; cp = next)
    {
	next = cp->next;
	FREE(cp);
    }
}


void free_shifts()
{
    register shifts *sp, *next;

    FREE(shift_table);
    for (sp = first_shift; sp; sp = next)
    {
	next = sp->next;
	FREE(sp);
    }
}



void free_reductions()
{
    register reductions *rp, *next;

    FREE(reduction_table);
    for (rp = first_reduction; rp; rp = next)
    {
	next = rp->next;
	FREE(rp);
    }
}



void write_section(char *section_name)
{
    char **section;
    FILE *fp;
    int i;
    struct section *sl;

    for(sl=&section_list[0]; sl->name; sl++) {
      if(strcmp(sl->name,section_name)==0) {
	break;
      }
    }
    if(sl->name==0) {
      fprintf(stderr, "Cannot find section '%s' in your skeleton file\n", section_name);
      exit(1);
    }

    section =  sl->ptr;
    fp = code_file;
    for (i = lflag ? 1 : 0; section[i]; ++i)
    {
	++outline;
	fprintf(fp, "%s\n", section[i]);
    }
}
