/*-------------------------------------------------------------------------
 *
 * regexp.c
 *	  Postgres' interface to the regular expression package.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/regexp.c
 *
 *		Alistair Crooks added the code for the regex caching
 *		agc - cached the regular expressions used - there's a good chance
 *		that we'll get a hit, so this saves a compile step for every
 *		attempted match. I haven't actually measured the speed improvement,
 *		but it `looks' a lot quicker visually when watching regression
 *		test output.
 *
 *		agc - incorporated Keith Bostic's Berkeley regex code into
 *		the tree for all ports. To distinguish this regex code from any that
 *		is existent on a platform, I've prepended the string "pg_" to
 *		the functions regcomp, regerror, regexec and regfree.
 *		Fixed a bug that was originally a typo by me, where `i' was used
 *		instead of `oldest' when compiling regular expressions - benign
 *		results mostly, although occasionally it bit you...
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "catalog/pg_type.h"
#include "funcapi.h"
#include "regex/regex.h"
#include "utils/builtins.h"
#include "utils/guc.h"

#define PG_GETARG_TEXT_PP_IF_EXISTS(_n) \
	(PG_NARGS() > (_n) ? PG_GETARG_TEXT_PP(_n) : NULL)


/* all the options of interest for regex functions */
typedef struct pg_re_flags
{
	int			cflags;			/* compile flags for Spencer's regex code */
	bool		glob;			/* do it globally (for each occurrence) */
} pg_re_flags;

/* cross-call state for regexp_matches(), also regexp_split() */
typedef struct regexp_matches_ctx
{
	text	   *orig_str;		/* data string in original TEXT form */
	int			nmatches;		/* number of places where pattern matched */
	int			npatterns;		/* number of capturing subpatterns */
	/* We store start char index and end+1 char index for each match */
	/* so the number of entries in match_locs is nmatches * npatterns * 2 */
	int		   *match_locs;		/* 0-based character indexes */
	int			next_match;		/* 0-based index of next match to process */
	/* workspace for build_regexp_matches_result() */
	Datum	   *elems;			/* has npatterns elements */
	bool	   *nulls;			/* has npatterns elements */
} regexp_matches_ctx;

/*
 * We cache precompiled regular expressions using a "self organizing list"
 * structure, in which recently-used items tend to be near the front.
 * Whenever we use an entry, it's moved up to the front of the list.
 * Over time, an item's average position corresponds to its frequency of use.
 *
 * When we first create an entry, it's inserted at the front of
 * the array, dropping the entry at the end of the array if necessary to
 * make room.  (This might seem to be weighting the new entry too heavily,
 * but if we insert new entries further back, we'll be unable to adjust to
 * a sudden shift in the query mix where we are presented with MAX_CACHED_RES
 * never-before-seen items used circularly.  We ought to be able to handle
 * that case, so we have to insert at the front.)
 *
 * Knuth mentions a variant strategy in which a used item is moved up just
 * one place in the list.  Although he says this uses fewer comparisons on
 * average, it seems not to adapt very well to the situation where you have
 * both some reusable patterns and a steady stream of non-reusable patterns.
 * A reusable pattern that isn't used at least as often as non-reusable
 * patterns are seen will "fail to keep up" and will drop off the end of the
 * cache.  With move-to-front, a reusable pattern is guaranteed to stay in
 * the cache as long as it's used at least once in every MAX_CACHED_RES uses.
 */

/* this is the maximum number of cached regular expressions */
#ifndef MAX_CACHED_RES
#define MAX_CACHED_RES	32
#endif

/* this structure describes one cached regular expression */
typedef struct cached_re_str
{
	char	   *cre_pat;		/* original RE (not null terminated!) */
	int			cre_pat_len;	/* length of original RE, in bytes */
	int			cre_flags;		/* compile flags: extended,icase etc */
	Oid			cre_collation;	/* collation to use */
	regex_t		cre_re;			/* the compiled regular expression */
} cached_re_str;

static int	num_res = 0;		/* # of cached re's */
static cached_re_str re_array[MAX_CACHED_RES];	/* cached re's */


/* Local functions */
static regexp_matches_ctx *setup_regexp_matches(text *orig_str, text *pattern,
					 text *flags,
					 Oid collation,
					 bool force_glob,
					 bool use_subpatterns,
					 bool ignore_degenerate);
static void cleanup_regexp_matches(regexp_matches_ctx *matchctx);
static ArrayType *build_regexp_matches_result(regexp_matches_ctx *matchctx);
static Datum build_regexp_split_result(regexp_matches_ctx *splitctx);


/*
 * RE_compile_and_cache - compile a RE, caching if possible
 *
 * Returns regex_t *
 *
 *	text_re --- the pattern, expressed as a TEXT object
 *	cflags --- compile options for the pattern
 *	collation --- collation to use for LC_CTYPE-dependent behavior
 *
 * Pattern is given in the database encoding.  We internally convert to
 * an array of pg_wchar, which is what Spencer's regex package wants.
 */
static regex_t *
RE_compile_and_cache(text *text_re, int cflags, Oid collation)
{
	int			text_re_len = VARSIZE_ANY_EXHDR(text_re);
	char	   *text_re_val = VARDATA_ANY(text_re);
	pg_wchar   *pattern;
	int			pattern_len;
	int			i;
	int			regcomp_result;
	cached_re_str re_temp;
	char		errMsg[100];

	/*
	 * Look for a match among previously compiled REs.	Since the data
	 * structure is self-organizing with most-used entries at the front, our
	 * search strategy can just be to scan from the front.
	 */
	for (i = 0; i < num_res; i++)
	{
		if (re_array[i].cre_pat_len == text_re_len &&
			re_array[i].cre_flags == cflags &&
			re_array[i].cre_collation == collation &&
			memcmp(re_array[i].cre_pat, text_re_val, text_re_len) == 0)
		{
			/*
			 * Found a match; move it to front if not there already.
			 */
			if (i > 0)
			{
				re_temp = re_array[i];
				memmove(&re_array[1], &re_array[0], i * sizeof(cached_re_str));
				re_array[0] = re_temp;
			}

			return &re_array[0].cre_re;
		}
	}

	/*
	 * Couldn't find it, so try to compile the new RE.  To avoid leaking
	 * resources on failure, we build into the re_temp local.
	 */

	/* Convert pattern string to wide characters */
	pattern = (pg_wchar *) palloc((text_re_len + 1) * sizeof(pg_wchar));
	pattern_len = pg_mb2wchar_with_len(text_re_val,
									   pattern,
									   text_re_len);

	regcomp_result = pg_regcomp(&re_temp.cre_re,
								pattern,
								pattern_len,
								cflags,
								collation);

	pfree(pattern);

	if (regcomp_result != REG_OKAY)
	{
		/* re didn't compile (no need for pg_regfree, if so) */
		pg_regerror(regcomp_result, &re_temp.cre_re, errMsg, sizeof(errMsg));
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_REGULAR_EXPRESSION),
				 errmsg("invalid regular expression: %s", errMsg)));
	}

	/*
	 * We use malloc/free for the cre_pat field because the storage has to
	 * persist across transactions, and because we want to get control back on
	 * out-of-memory.  The Max() is because some malloc implementations return
	 * NULL for malloc(0).
	 */
	re_temp.cre_pat = malloc(Max(text_re_len, 1));
	if (re_temp.cre_pat == NULL)
	{
		pg_regfree(&re_temp.cre_re);
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));
	}
	memcpy(re_temp.cre_pat, text_re_val, text_re_len);
	re_temp.cre_pat_len = text_re_len;
	re_temp.cre_flags = cflags;
	re_temp.cre_collation = collation;

	/*
	 * Okay, we have a valid new item in re_temp; insert it into the storage
	 * array.  Discard last entry if needed.
	 */
	if (num_res >= MAX_CACHED_RES)
	{
		--num_res;
		Assert(num_res < MAX_CACHED_RES);
		pg_regfree(&re_array[num_res].cre_re);
		free(re_array[num_res].cre_pat);
	}

	if (num_res > 0)
		memmove(&re_array[1], &re_array[0], num_res * sizeof(cached_re_str));

	re_array[0] = re_temp;
	num_res++;

	return &re_array[0].cre_re;
}

/*
 * RE_wchar_execute - execute a RE on pg_wchar data
 *
 * Returns TRUE on match, FALSE on no match
 *
 *	re --- the compiled pattern as returned by RE_compile_and_cache
 *	data --- the data to match against (need not be null-terminated)
 *	data_len --- the length of the data string
 *	start_search -- the offset in the data to start searching
 *	nmatch, pmatch	--- optional return area for match details
 *
 * Data is given as array of pg_wchar which is what Spencer's regex package
 * wants.
 */
static bool
RE_wchar_execute(regex_t *re, pg_wchar *data, int data_len,
				 int start_search, int nmatch, regmatch_t *pmatch)
{
	int			regexec_result;
	char		errMsg[100];

	/* Perform RE match and return result */
	regexec_result = pg_regexec(re,
								data,
								data_len,
								start_search,
								NULL,	/* no details */
								nmatch,
								pmatch,
								0);

	if (regexec_result != REG_OKAY && regexec_result != REG_NOMATCH)
	{
		/* re failed??? */
		pg_regerror(regexec_result, re, errMsg, sizeof(errMsg));
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_REGULAR_EXPRESSION),
				 errmsg("regular expression failed: %s", errMsg)));
	}

	return (regexec_result == REG_OKAY);
}

/*
 * RE_execute - execute a RE
 *
 * Returns TRUE on match, FALSE on no match
 *
 *	re --- the compiled pattern as returned by RE_compile_and_cache
 *	dat --- the data to match against (need not be null-terminated)
 *	dat_len --- the length of the data string
 *	nmatch, pmatch	--- optional return area for match details
 *
 * Data is given in the database encoding.	We internally
 * convert to array of pg_wchar which is what Spencer's regex package wants.
 */
static bool
RE_execute(regex_t *re, char *dat, int dat_len,
		   int nmatch, regmatch_t *pmatch)
{
	pg_wchar   *data;
	int			data_len;
	bool		match;

	/* Convert data string to wide characters */
	data = (pg_wchar *) palloc((dat_len + 1) * sizeof(pg_wchar));
	data_len = pg_mb2wchar_with_len(dat, data, dat_len);

	/* Perform RE match and return result */
	match = RE_wchar_execute(re, data, data_len, 0, nmatch, pmatch);

	pfree(data);
	return match;
}

/*
 * RE_compile_and_execute - compile and execute a RE
 *
 * Returns TRUE on match, FALSE on no match
 *
 *	text_re --- the pattern, expressed as a TEXT object
 *	dat --- the data to match against (need not be null-terminated)
 *	dat_len --- the length of the data string
 *	cflags --- compile options for the pattern
 *	collation --- collation to use for LC_CTYPE-dependent behavior
 *	nmatch, pmatch	--- optional return area for match details
 *
 * Both pattern and data are given in the database encoding.  We internally
 * convert to array of pg_wchar which is what Spencer's regex package wants.
 */
static bool
RE_compile_and_execute(text *text_re, char *dat, int dat_len,
					   int cflags, Oid collation,
					   int nmatch, regmatch_t *pmatch)
{
	regex_t    *re;

	/* Compile RE */
	re = RE_compile_and_cache(text_re, cflags, collation);

	return RE_execute(re, dat, dat_len, nmatch, pmatch);
}


/*
 * parse_re_flags - parse the options argument of regexp_matches and friends
 *
 *	flags --- output argument, filled with desired options
 *	opts --- TEXT object, or NULL for defaults
 *
 * This accepts all the options allowed by any of the callers; callers that
 * don't want some have to reject them after the fact.
 */
static void
parse_re_flags(pg_re_flags *flags, text *opts)
{
	/* regex flavor is always folded into the compile flags */
	flags->cflags = REG_ADVANCED;
	flags->glob = false;

	if (opts)
	{
		char	   *opt_p = VARDATA_ANY(opts);
		int			opt_len = VARSIZE_ANY_EXHDR(opts);
		int			i;

		for (i = 0; i < opt_len; i++)
		{
			switch (opt_p[i])
			{
				case 'g':
					flags->glob = true;
					break;
				case 'b':		/* BREs (but why???) */
					flags->cflags &= ~(REG_ADVANCED | REG_EXTENDED | REG_QUOTE);
					break;
				case 'c':		/* case sensitive */
					flags->cflags &= ~REG_ICASE;
					break;
				case 'e':		/* plain EREs */
					flags->cflags |= REG_EXTENDED;
					flags->cflags &= ~(REG_ADVANCED | REG_QUOTE);
					break;
				case 'i':		/* case insensitive */
					flags->cflags |= REG_ICASE;
					break;
				case 'm':		/* Perloid synonym for n */
				case 'n':		/* \n affects ^ $ . [^ */
					flags->cflags |= REG_NEWLINE;
					break;
				case 'p':		/* ~Perl, \n affects . [^ */
					flags->cflags |= REG_NLSTOP;
					flags->cflags &= ~REG_NLANCH;
					break;
				case 'q':		/* literal string */
					flags->cflags |= REG_QUOTE;
					flags->cflags &= ~(REG_ADVANCED | REG_EXTENDED);
					break;
				case 's':		/* single line, \n ordinary */
					flags->cflags &= ~REG_NEWLINE;
					break;
				case 't':		/* tight syntax */
					flags->cflags &= ~REG_EXPANDED;
					break;
				case 'w':		/* weird, \n affects ^ $ only */
					flags->cflags &= ~REG_NLSTOP;
					flags->cflags |= REG_NLANCH;
					break;
				case 'x':		/* expanded syntax */
					flags->cflags |= REG_EXPANDED;
					break;
				default:
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("invalid regexp option: \"%c\"",
									opt_p[i])));
					break;
			}
		}
	}
}


/*
 *	interface routines called by the function manager
 */

Datum
nameregexeq(PG_FUNCTION_ARGS)
{
	Name		n = PG_GETARG_NAME(0);
	text	   *p = PG_GETARG_TEXT_PP(1);

	PG_RETURN_BOOL(RE_compile_and_execute(p,
										  NameStr(*n),
										  strlen(NameStr(*n)),
										  REG_ADVANCED,
										  PG_GET_COLLATION(),
										  0, NULL));
}

Datum
nameregexne(PG_FUNCTION_ARGS)
{
	Name		n = PG_GETARG_NAME(0);
	text	   *p = PG_GETARG_TEXT_PP(1);

	PG_RETURN_BOOL(!RE_compile_and_execute(p,
										   NameStr(*n),
										   strlen(NameStr(*n)),
										   REG_ADVANCED,
										   PG_GET_COLLATION(),
										   0, NULL));
}

Datum
textregexeq(PG_FUNCTION_ARGS)
{
	text	   *s = PG_GETARG_TEXT_PP(0);
	text	   *p = PG_GETARG_TEXT_PP(1);

	PG_RETURN_BOOL(RE_compile_and_execute(p,
										  VARDATA_ANY(s),
										  VARSIZE_ANY_EXHDR(s),
										  REG_ADVANCED,
										  PG_GET_COLLATION(),
										  0, NULL));
}

Datum
textregexne(PG_FUNCTION_ARGS)
{
	text	   *s = PG_GETARG_TEXT_PP(0);
	text	   *p = PG_GETARG_TEXT_PP(1);

	PG_RETURN_BOOL(!RE_compile_and_execute(p,
										   VARDATA_ANY(s),
										   VARSIZE_ANY_EXHDR(s),
										   REG_ADVANCED,
										   PG_GET_COLLATION(),
										   0, NULL));
}


/*
 *	routines that use the regexp stuff, but ignore the case.
 *	for this, we use the REG_ICASE flag to pg_regcomp
 */


Datum
nameicregexeq(PG_FUNCTION_ARGS)
{
	Name		n = PG_GETARG_NAME(0);
	text	   *p = PG_GETARG_TEXT_PP(1);

	PG_RETURN_BOOL(RE_compile_and_execute(p,
										  NameStr(*n),
										  strlen(NameStr(*n)),
										  REG_ADVANCED | REG_ICASE,
										  PG_GET_COLLATION(),
										  0, NULL));
}

Datum
nameicregexne(PG_FUNCTION_ARGS)
{
	Name		n = PG_GETARG_NAME(0);
	text	   *p = PG_GETARG_TEXT_PP(1);

	PG_RETURN_BOOL(!RE_compile_and_execute(p,
										   NameStr(*n),
										   strlen(NameStr(*n)),
										   REG_ADVANCED | REG_ICASE,
										   PG_GET_COLLATION(),
										   0, NULL));
}

Datum
texticregexeq(PG_FUNCTION_ARGS)
{
	text	   *s = PG_GETARG_TEXT_PP(0);
	text	   *p = PG_GETARG_TEXT_PP(1);

	PG_RETURN_BOOL(RE_compile_and_execute(p,
										  VARDATA_ANY(s),
										  VARSIZE_ANY_EXHDR(s),
										  REG_ADVANCED | REG_ICASE,
										  PG_GET_COLLATION(),
										  0, NULL));
}

Datum
texticregexne(PG_FUNCTION_ARGS)
{
	text	   *s = PG_GETARG_TEXT_PP(0);
	text	   *p = PG_GETARG_TEXT_PP(1);

	PG_RETURN_BOOL(!RE_compile_and_execute(p,
										   VARDATA_ANY(s),
										   VARSIZE_ANY_EXHDR(s),
										   REG_ADVANCED | REG_ICASE,
										   PG_GET_COLLATION(),
										   0, NULL));
}


/*
 * textregexsubstr()
 *		Return a substring matched by a regular expression.
 */
Datum
textregexsubstr(PG_FUNCTION_ARGS)
{
	text	   *s = PG_GETARG_TEXT_PP(0);
	text	   *p = PG_GETARG_TEXT_PP(1);
	regex_t    *re;
	regmatch_t	pmatch[2];
	int			so,
				eo;

	/* Compile RE */
	re = RE_compile_and_cache(p, REG_ADVANCED, PG_GET_COLLATION());

	/*
	 * We pass two regmatch_t structs to get info about the overall match and
	 * the match for the first parenthesized subexpression (if any). If there
	 * is a parenthesized subexpression, we return what it matched; else
	 * return what the whole regexp matched.
	 */
	if (!RE_execute(re,
					VARDATA_ANY(s), VARSIZE_ANY_EXHDR(s),
					2, pmatch))
		PG_RETURN_NULL();		/* definitely no match */

	if (re->re_nsub > 0)
	{
		/* has parenthesized subexpressions, use the first one */
		so = pmatch[1].rm_so;
		eo = pmatch[1].rm_eo;
	}
	else
	{
		/* no parenthesized subexpression, use whole match */
		so = pmatch[0].rm_so;
		eo = pmatch[0].rm_eo;
	}

	/*
	 * It is possible to have a match to the whole pattern but no match for a
	 * subexpression; for example 'foo(bar)?' is considered to match 'foo' but
	 * there is no subexpression match.  So this extra test for match failure
	 * is not redundant.
	 */
	if (so < 0 || eo < 0)
		PG_RETURN_NULL();

	return DirectFunctionCall3(text_substr,
							   PointerGetDatum(s),
							   Int32GetDatum(so + 1),
							   Int32GetDatum(eo - so));
}

/*
 * textregexreplace_noopt()
 *		Return a string matched by a regular expression, with replacement.
 *
 * This version doesn't have an option argument: we default to case
 * sensitive match, replace the first instance only.
 */
Datum
textregexreplace_noopt(PG_FUNCTION_ARGS)
{
	text	   *s = PG_GETARG_TEXT_PP(0);
	text	   *p = PG_GETARG_TEXT_PP(1);
	text	   *r = PG_GETARG_TEXT_PP(2);
	regex_t    *re;

	re = RE_compile_and_cache(p, REG_ADVANCED, PG_GET_COLLATION());

	PG_RETURN_TEXT_P(replace_text_regexp(s, (void *) re, r, false));
}

/*
 * textregexreplace()
 *		Return a string matched by a regular expression, with replacement.
 */
Datum
textregexreplace(PG_FUNCTION_ARGS)
{
	text	   *s = PG_GETARG_TEXT_PP(0);
	text	   *p = PG_GETARG_TEXT_PP(1);
	text	   *r = PG_GETARG_TEXT_PP(2);
	text	   *opt = PG_GETARG_TEXT_PP(3);
	regex_t    *re;
	pg_re_flags flags;

	parse_re_flags(&flags, opt);

	re = RE_compile_and_cache(p, flags.cflags, PG_GET_COLLATION());

	PG_RETURN_TEXT_P(replace_text_regexp(s, (void *) re, r, flags.glob));
}

/*
 * similar_escape()
 * Convert a SQL:2008 regexp pattern to POSIX style, so it can be used by
 * our regexp engine.
 */
Datum
similar_escape(PG_FUNCTION_ARGS)
{
	text	   *pat_text;
	text	   *esc_text;
	text	   *result;
	char	   *p,
			   *e,
			   *r;
	int			plen,
				elen;
	bool		afterescape = false;
	bool		incharclass = false;
	int			nquotes = 0;

	/* This function is not strict, so must test explicitly */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pat_text = PG_GETARG_TEXT_PP(0);
	p = VARDATA_ANY(pat_text);
	plen = VARSIZE_ANY_EXHDR(pat_text);
	if (PG_ARGISNULL(1))
	{
		/* No ESCAPE clause provided; default to backslash as escape */
		e = "\\";
		elen = 1;
	}
	else
	{
		esc_text = PG_GETARG_TEXT_PP(1);
		e = VARDATA_ANY(esc_text);
		elen = VARSIZE_ANY_EXHDR(esc_text);
		if (elen == 0)
			e = NULL;			/* no escape character */
		else if (elen != 1)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_ESCAPE_SEQUENCE),
					 errmsg("invalid escape string"),
				  errhint("Escape string must be empty or one character.")));
	}

	/*----------
	 * We surround the transformed input string with
	 *			^(?: ... )$
	 * which requires some explanation.  We need "^" and "$" to force
	 * the pattern to match the entire input string as per SQL99 spec.
	 * The "(?:" and ")" are a non-capturing set of parens; we have to have
	 * parens in case the string contains "|", else the "^" and "$" will
	 * be bound into the first and last alternatives which is not what we
	 * want, and the parens must be non capturing because we don't want them
	 * to count when selecting output for SUBSTRING.
	 *----------
	 */

	/*
	 * We need room for the prefix/postfix plus as many as 3 output bytes per
	 * input byte; since the input is at most 1GB this can't overflow
	 */
	result = (text *) palloc(VARHDRSZ + 6 + 3 * plen);
	r = VARDATA(result);

	*r++ = '^';
	*r++ = '(';
	*r++ = '?';
	*r++ = ':';

	while (plen > 0)
	{
		char		pchar = *p;

		if (afterescape)
		{
			if (pchar == '"' && !incharclass)	/* for SUBSTRING patterns */
				*r++ = ((nquotes++ % 2) == 0) ? '(' : ')';
			else
			{
				*r++ = '\\';
				*r++ = pchar;
			}
			afterescape = false;
		}
		else if (e && pchar == *e)
		{
			/* SQL99 escape character; do not send to output */
			afterescape = true;
		}
		else if (incharclass)
		{
			if (pchar == '\\')
				*r++ = '\\';
			*r++ = pchar;
			if (pchar == ']')
				incharclass = false;
		}
		else if (pchar == '[')
		{
			*r++ = pchar;
			incharclass = true;
		}
		else if (pchar == '%')
		{
			*r++ = '.';
			*r++ = '*';
		}
		else if (pchar == '_')
			*r++ = '.';
		else if (pchar == '(')
		{
			/* convert to non-capturing parenthesis */
			*r++ = '(';
			*r++ = '?';
			*r++ = ':';
		}
		else if (pchar == '\\' || pchar == '.' ||
				 pchar == '^' || pchar == '$')
		{
			*r++ = '\\';
			*r++ = pchar;
		}
		else
			*r++ = pchar;
		p++, plen--;
	}

	*r++ = ')';
	*r++ = '$';

	SET_VARSIZE(result, r - ((char *) result));

	PG_RETURN_TEXT_P(result);
}

/*
 * regexp_matches()
 *		Return a table of matches of a pattern within a string.
 */
Datum
regexp_matches(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	regexp_matches_ctx *matchctx;

	if (SRF_IS_FIRSTCALL())
	{
		text	   *pattern = PG_GETARG_TEXT_PP(1);
		text	   *flags = PG_GETARG_TEXT_PP_IF_EXISTS(2);
		MemoryContext oldcontext;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* be sure to copy the input string into the multi-call ctx */
		matchctx = setup_regexp_matches(PG_GETARG_TEXT_P_COPY(0), pattern,
										flags,
										PG_GET_COLLATION(),
										false, true, false);

		/* Pre-create workspace that build_regexp_matches_result needs */
		matchctx->elems = (Datum *) palloc(sizeof(Datum) * matchctx->npatterns);
		matchctx->nulls = (bool *) palloc(sizeof(bool) * matchctx->npatterns);

		MemoryContextSwitchTo(oldcontext);
		funcctx->user_fctx = (void *) matchctx;
	}

	funcctx = SRF_PERCALL_SETUP();
	matchctx = (regexp_matches_ctx *) funcctx->user_fctx;

	if (matchctx->next_match < matchctx->nmatches)
	{
		ArrayType  *result_ary;

		result_ary = build_regexp_matches_result(matchctx);
		matchctx->next_match++;
		SRF_RETURN_NEXT(funcctx, PointerGetDatum(result_ary));
	}

	/* release space in multi-call ctx to avoid intraquery memory leak */
	cleanup_regexp_matches(matchctx);

	SRF_RETURN_DONE(funcctx);
}

/* This is separate to keep the opr_sanity regression test from complaining */
Datum
regexp_matches_no_flags(PG_FUNCTION_ARGS)
{
	return regexp_matches(fcinfo);
}

/*
 * setup_regexp_matches --- do the initial matching for regexp_matches()
 *		or regexp_split()
 *
 * To avoid having to re-find the compiled pattern on each call, we do
 * all the matching in one swoop.  The returned regexp_matches_ctx contains
 * the locations of all the substrings matching the pattern.
 *
 * The three bool parameters have only two patterns (one for each caller)
 * but it seems clearer to distinguish the functionality this way than to
 * key it all off one "is_split" flag.
 */
static regexp_matches_ctx *
setup_regexp_matches(text *orig_str, text *pattern, text *flags,
					 Oid collation,
					 bool force_glob, bool use_subpatterns,
					 bool ignore_degenerate)
{
	regexp_matches_ctx *matchctx = palloc0(sizeof(regexp_matches_ctx));
	int			orig_len;
	pg_wchar   *wide_str;
	int			wide_len;
	pg_re_flags re_flags;
	regex_t    *cpattern;
	regmatch_t *pmatch;
	int			pmatch_len;
	int			array_len;
	int			array_idx;
	int			prev_match_end;
	int			start_search;

	/* save original string --- we'll extract result substrings from it */
	matchctx->orig_str = orig_str;

	/* convert string to pg_wchar form for matching */
	orig_len = VARSIZE_ANY_EXHDR(orig_str);
	wide_str = (pg_wchar *) palloc(sizeof(pg_wchar) * (orig_len + 1));
	wide_len = pg_mb2wchar_with_len(VARDATA_ANY(orig_str), wide_str, orig_len);

	/* determine options */
	parse_re_flags(&re_flags, flags);
	if (force_glob)
	{
		/* user mustn't specify 'g' for regexp_split */
		if (re_flags.glob)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("regexp_split does not support the global option")));
		/* but we find all the matches anyway */
		re_flags.glob = true;
	}

	/* set up the compiled pattern */
	cpattern = RE_compile_and_cache(pattern, re_flags.cflags, collation);

	/* do we want to remember subpatterns? */
	if (use_subpatterns && cpattern->re_nsub > 0)
	{
		matchctx->npatterns = cpattern->re_nsub;
		pmatch_len = cpattern->re_nsub + 1;
	}
	else
	{
		use_subpatterns = false;
		matchctx->npatterns = 1;
		pmatch_len = 1;
	}

	/* temporary output space for RE package */
	pmatch = palloc(sizeof(regmatch_t) * pmatch_len);

	/* the real output space (grown dynamically if needed) */
	array_len = re_flags.glob ? 256 : 32;
	matchctx->match_locs = (int *) palloc(sizeof(int) * array_len);
	array_idx = 0;

	/* search for the pattern, perhaps repeatedly */
	prev_match_end = 0;
	start_search = 0;
	while (RE_wchar_execute(cpattern, wide_str, wide_len, start_search,
							pmatch_len, pmatch))
	{
		/*
		 * If requested, ignore degenerate matches, which are zero-length
		 * matches occurring at the start or end of a string or just after a
		 * previous match.
		 */
		if (!ignore_degenerate ||
			(pmatch[0].rm_so < wide_len &&
			 pmatch[0].rm_eo > prev_match_end))
		{
			/* enlarge output space if needed */
			while (array_idx + matchctx->npatterns * 2 > array_len)
			{
				array_len *= 2;
				matchctx->match_locs = (int *) repalloc(matchctx->match_locs,
													sizeof(int) * array_len);
			}

			/* save this match's locations */
			if (use_subpatterns)
			{
				int			i;

				for (i = 1; i <= matchctx->npatterns; i++)
				{
					matchctx->match_locs[array_idx++] = pmatch[i].rm_so;
					matchctx->match_locs[array_idx++] = pmatch[i].rm_eo;
				}
			}
			else
			{
				matchctx->match_locs[array_idx++] = pmatch[0].rm_so;
				matchctx->match_locs[array_idx++] = pmatch[0].rm_eo;
			}
			matchctx->nmatches++;
		}
		prev_match_end = pmatch[0].rm_eo;

		/* if not glob, stop after one match */
		if (!re_flags.glob)
			break;

		/*
		 * Advance search position.  Normally we start the next search at the
		 * end of the previous match; but if the match was of zero length, we
		 * have to advance by one character, or we'd just find the same match
		 * again.
		 */
		start_search = prev_match_end;
		if (pmatch[0].rm_so == pmatch[0].rm_eo)
			start_search++;
		if (start_search > wide_len)
			break;
	}

	/* Clean up temp storage */
	pfree(wide_str);
	pfree(pmatch);

	return matchctx;
}

/*
 * cleanup_regexp_matches - release memory of a regexp_matches_ctx
 */
static void
cleanup_regexp_matches(regexp_matches_ctx *matchctx)
{
	pfree(matchctx->orig_str);
	pfree(matchctx->match_locs);
	if (matchctx->elems)
		pfree(matchctx->elems);
	if (matchctx->nulls)
		pfree(matchctx->nulls);
	pfree(matchctx);
}

/*
 * build_regexp_matches_result - build output array for current match
 */
static ArrayType *
build_regexp_matches_result(regexp_matches_ctx *matchctx)
{
	Datum	   *elems = matchctx->elems;
	bool	   *nulls = matchctx->nulls;
	int			dims[1];
	int			lbs[1];
	int			loc;
	int			i;

	/* Extract matching substrings from the original string */
	loc = matchctx->next_match * matchctx->npatterns * 2;
	for (i = 0; i < matchctx->npatterns; i++)
	{
		int			so = matchctx->match_locs[loc++];
		int			eo = matchctx->match_locs[loc++];

		if (so < 0 || eo < 0)
		{
			elems[i] = (Datum) 0;
			nulls[i] = true;
		}
		else
		{
			elems[i] = DirectFunctionCall3(text_substr,
										 PointerGetDatum(matchctx->orig_str),
										   Int32GetDatum(so + 1),
										   Int32GetDatum(eo - so));
			nulls[i] = false;
		}
	}

	/* And form an array */
	dims[0] = matchctx->npatterns;
	lbs[0] = 1;
	/* XXX: this hardcodes assumptions about the text type */
	return construct_md_array(elems, nulls, 1, dims, lbs,
							  TEXTOID, -1, false, 'i');
}

/*
 * regexp_split_to_table()
 *		Split the string at matches of the pattern, returning the
 *		split-out substrings as a table.
 */
Datum
regexp_split_to_table(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	regexp_matches_ctx *splitctx;

	if (SRF_IS_FIRSTCALL())
	{
		text	   *pattern = PG_GETARG_TEXT_PP(1);
		text	   *flags = PG_GETARG_TEXT_PP_IF_EXISTS(2);
		MemoryContext oldcontext;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* be sure to copy the input string into the multi-call ctx */
		splitctx = setup_regexp_matches(PG_GETARG_TEXT_P_COPY(0), pattern,
										flags,
										PG_GET_COLLATION(),
										true, false, true);

		MemoryContextSwitchTo(oldcontext);
		funcctx->user_fctx = (void *) splitctx;
	}

	funcctx = SRF_PERCALL_SETUP();
	splitctx = (regexp_matches_ctx *) funcctx->user_fctx;

	if (splitctx->next_match <= splitctx->nmatches)
	{
		Datum		result = build_regexp_split_result(splitctx);

		splitctx->next_match++;
		SRF_RETURN_NEXT(funcctx, result);
	}

	/* release space in multi-call ctx to avoid intraquery memory leak */
	cleanup_regexp_matches(splitctx);

	SRF_RETURN_DONE(funcctx);
}

/* This is separate to keep the opr_sanity regression test from complaining */
Datum
regexp_split_to_table_no_flags(PG_FUNCTION_ARGS)
{
	return regexp_split_to_table(fcinfo);
}

/*
 * regexp_split_to_array()
 *		Split the string at matches of the pattern, returning the
 *		split-out substrings as an array.
 */
Datum
regexp_split_to_array(PG_FUNCTION_ARGS)
{
	ArrayBuildState *astate = NULL;
	regexp_matches_ctx *splitctx;

	splitctx = setup_regexp_matches(PG_GETARG_TEXT_PP(0),
									PG_GETARG_TEXT_PP(1),
									PG_GETARG_TEXT_PP_IF_EXISTS(2),
									PG_GET_COLLATION(),
									true, false, true);

	while (splitctx->next_match <= splitctx->nmatches)
	{
		astate = accumArrayResult(astate,
								  build_regexp_split_result(splitctx),
								  false,
								  TEXTOID,
								  CurrentMemoryContext);
		splitctx->next_match++;
	}

	/*
	 * We don't call cleanup_regexp_matches here; it would try to pfree the
	 * input string, which we didn't copy.  The space is not in a long-lived
	 * memory context anyway.
	 */

	PG_RETURN_ARRAYTYPE_P(makeArrayResult(astate, CurrentMemoryContext));
}

/* This is separate to keep the opr_sanity regression test from complaining */
Datum
regexp_split_to_array_no_flags(PG_FUNCTION_ARGS)
{
	return regexp_split_to_array(fcinfo);
}

/*
 * build_regexp_split_result - build output string for current match
 *
 * We return the string between the current match and the previous one,
 * or the string after the last match when next_match == nmatches.
 */
static Datum
build_regexp_split_result(regexp_matches_ctx *splitctx)
{
	int			startpos;
	int			endpos;

	if (splitctx->next_match > 0)
		startpos = splitctx->match_locs[splitctx->next_match * 2 - 1];
	else
		startpos = 0;
	if (startpos < 0)
		elog(ERROR, "invalid match ending position");

	if (splitctx->next_match < splitctx->nmatches)
	{
		endpos = splitctx->match_locs[splitctx->next_match * 2];
		if (endpos < startpos)
			elog(ERROR, "invalid match starting position");
		return DirectFunctionCall3(text_substr,
								   PointerGetDatum(splitctx->orig_str),
								   Int32GetDatum(startpos + 1),
								   Int32GetDatum(endpos - startpos));
	}
	else
	{
		/* no more matches, return rest of string */
		return DirectFunctionCall2(text_substr_no_len,
								   PointerGetDatum(splitctx->orig_str),
								   Int32GetDatum(startpos + 1));
	}
}

/*
 * regexp_fixed_prefix - extract fixed prefix, if any, for a regexp
 *
 * The result is NULL if there is no fixed prefix, else a palloc'd string.
 * If it is an exact match, not just a prefix, *exact is returned as TRUE.
 */
char *
regexp_fixed_prefix(text *text_re, bool case_insensitive, Oid collation,
					bool *exact)
{
	char	   *result;
	regex_t    *re;
	int			cflags;
	int			re_result;
	pg_wchar   *str;
	size_t		slen;
	size_t		maxlen;
	char		errMsg[100];

	*exact = false;				/* default result */

	/* Compile RE */
	cflags = REG_ADVANCED;
	if (case_insensitive)
		cflags |= REG_ICASE;

	re = RE_compile_and_cache(text_re, cflags, collation);

	/* Examine it to see if there's a fixed prefix */
	re_result = pg_regprefix(re, &str, &slen);

	switch (re_result)
	{
		case REG_NOMATCH:
			return NULL;

		case REG_PREFIX:
			/* continue with wchar conversion */
			break;

		case REG_EXACT:
			*exact = true;
			/* continue with wchar conversion */
			break;

		default:
			/* re failed??? */
			pg_regerror(re_result, re, errMsg, sizeof(errMsg));
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_REGULAR_EXPRESSION),
					 errmsg("regular expression failed: %s", errMsg)));
			break;
	}

	/* Convert pg_wchar result back to database encoding */
	maxlen = pg_database_encoding_max_length() * slen + 1;
	result = (char *) palloc(maxlen);
	slen = pg_wchar2mb_with_len(str, result, slen);
	Assert(slen < maxlen);

	free(str);

	return result;
}
