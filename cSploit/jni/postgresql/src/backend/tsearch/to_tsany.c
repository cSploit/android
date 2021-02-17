/*-------------------------------------------------------------------------
 *
 * to_tsany.c
 *		to_ts* function definitions
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/tsearch/to_tsany.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "catalog/namespace.h"
#include "tsearch/ts_cache.h"
#include "tsearch/ts_utils.h"
#include "utils/builtins.h"
#include "utils/syscache.h"


Datum
get_current_ts_config(PG_FUNCTION_ARGS)
{
	PG_RETURN_OID(getTSCurrentConfig(true));
}

/*
 * to_tsvector
 */
static int
compareWORD(const void *a, const void *b)
{
	int			res;

	res = tsCompareString(
						  ((ParsedWord *) a)->word, ((ParsedWord *) a)->len,
						  ((ParsedWord *) b)->word, ((ParsedWord *) b)->len,
						  false);

	if (res == 0)
	{
		if (((ParsedWord *) a)->pos.pos == ((ParsedWord *) b)->pos.pos)
			return 0;

		res = (((ParsedWord *) a)->pos.pos > ((ParsedWord *) b)->pos.pos) ? 1 : -1;
	}

	return res;
}

static int
uniqueWORD(ParsedWord *a, int4 l)
{
	ParsedWord *ptr,
			   *res;
	int			tmppos;

	if (l == 1)
	{
		tmppos = LIMITPOS(a->pos.pos);
		a->alen = 2;
		a->pos.apos = (uint16 *) palloc(sizeof(uint16) * a->alen);
		a->pos.apos[0] = 1;
		a->pos.apos[1] = tmppos;
		return l;
	}

	res = a;
	ptr = a + 1;

	/*
	 * Sort words with its positions
	 */
	qsort((void *) a, l, sizeof(ParsedWord), compareWORD);

	/*
	 * Initialize first word and its first position
	 */
	tmppos = LIMITPOS(a->pos.pos);
	a->alen = 2;
	a->pos.apos = (uint16 *) palloc(sizeof(uint16) * a->alen);
	a->pos.apos[0] = 1;
	a->pos.apos[1] = tmppos;

	/*
	 * Summarize position information for each word
	 */
	while (ptr - a < l)
	{
		if (!(ptr->len == res->len &&
			  strncmp(ptr->word, res->word, res->len) == 0))
		{
			/*
			 * Got a new word, so put it in result
			 */
			res++;
			res->len = ptr->len;
			res->word = ptr->word;
			tmppos = LIMITPOS(ptr->pos.pos);
			res->alen = 2;
			res->pos.apos = (uint16 *) palloc(sizeof(uint16) * res->alen);
			res->pos.apos[0] = 1;
			res->pos.apos[1] = tmppos;
		}
		else
		{
			/*
			 * The word already exists, so adjust position information. But
			 * before we should check size of position's array, max allowed
			 * value for position and uniqueness of position
			 */
			pfree(ptr->word);
			if (res->pos.apos[0] < MAXNUMPOS - 1 && res->pos.apos[res->pos.apos[0]] != MAXENTRYPOS - 1 &&
				res->pos.apos[res->pos.apos[0]] != LIMITPOS(ptr->pos.pos))
			{
				if (res->pos.apos[0] + 1 >= res->alen)
				{
					res->alen *= 2;
					res->pos.apos = (uint16 *) repalloc(res->pos.apos, sizeof(uint16) * res->alen);
				}
				if (res->pos.apos[0] == 0 || res->pos.apos[res->pos.apos[0]] != LIMITPOS(ptr->pos.pos))
				{
					res->pos.apos[res->pos.apos[0] + 1] = LIMITPOS(ptr->pos.pos);
					res->pos.apos[0]++;
				}
			}
		}
		ptr++;
	}

	return res + 1 - a;
}

/*
 * make value of tsvector, given parsed text
 */
TSVector
make_tsvector(ParsedText *prs)
{
	int			i,
				j,
				lenstr = 0,
				totallen;
	TSVector	in;
	WordEntry  *ptr;
	char	   *str;
	int			stroff;

	prs->curwords = uniqueWORD(prs->words, prs->curwords);
	for (i = 0; i < prs->curwords; i++)
	{
		lenstr += prs->words[i].len;
		if (prs->words[i].alen)
		{
			lenstr = SHORTALIGN(lenstr);
			lenstr += sizeof(uint16) + prs->words[i].pos.apos[0] * sizeof(WordEntryPos);
		}
	}

	if (lenstr > MAXSTRPOS)
		ereport(ERROR,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
				 errmsg("string is too long for tsvector (%d bytes, max %d bytes)", lenstr, MAXSTRPOS)));

	totallen = CALCDATASIZE(prs->curwords, lenstr);
	in = (TSVector) palloc0(totallen);
	SET_VARSIZE(in, totallen);
	in->size = prs->curwords;

	ptr = ARRPTR(in);
	str = STRPTR(in);
	stroff = 0;
	for (i = 0; i < prs->curwords; i++)
	{
		ptr->len = prs->words[i].len;
		ptr->pos = stroff;
		memcpy(str + stroff, prs->words[i].word, prs->words[i].len);
		stroff += prs->words[i].len;
		pfree(prs->words[i].word);
		if (prs->words[i].alen)
		{
			int			k = prs->words[i].pos.apos[0];
			WordEntryPos *wptr;

			if (k > 0xFFFF)
				elog(ERROR, "positions array too long");

			ptr->haspos = 1;
			stroff = SHORTALIGN(stroff);
			*(uint16 *) (str + stroff) = (uint16) k;
			wptr = POSDATAPTR(in, ptr);
			for (j = 0; j < k; j++)
			{
				WEP_SETWEIGHT(wptr[j], 0);
				WEP_SETPOS(wptr[j], prs->words[i].pos.apos[j + 1]);
			}
			stroff += sizeof(uint16) + k * sizeof(WordEntryPos);
			pfree(prs->words[i].pos.apos);
		}
		else
			ptr->haspos = 0;
		ptr++;
	}
	pfree(prs->words);
	return in;
}

Datum
to_tsvector_byid(PG_FUNCTION_ARGS)
{
	Oid			cfgId = PG_GETARG_OID(0);
	text	   *in = PG_GETARG_TEXT_P(1);
	ParsedText	prs;
	TSVector	out;

	prs.lenwords = (VARSIZE(in) - VARHDRSZ) / 6;		/* just estimation of
														 * word's number */
	if (prs.lenwords == 0)
		prs.lenwords = 2;
	prs.curwords = 0;
	prs.pos = 0;
	prs.words = (ParsedWord *) palloc(sizeof(ParsedWord) * prs.lenwords);

	parsetext(cfgId, &prs, VARDATA(in), VARSIZE(in) - VARHDRSZ);
	PG_FREE_IF_COPY(in, 1);

	if (prs.curwords)
		out = make_tsvector(&prs);
	else
	{
		pfree(prs.words);
		out = palloc(CALCDATASIZE(0, 0));
		SET_VARSIZE(out, CALCDATASIZE(0, 0));
		out->size = 0;
	}

	PG_RETURN_POINTER(out);
}

Datum
to_tsvector(PG_FUNCTION_ARGS)
{
	text	   *in = PG_GETARG_TEXT_P(0);
	Oid			cfgId;

	cfgId = getTSCurrentConfig(true);
	PG_RETURN_DATUM(DirectFunctionCall2(to_tsvector_byid,
										ObjectIdGetDatum(cfgId),
										PointerGetDatum(in)));
}

/*
 * to_tsquery
 */


/*
 * This function is used for morph parsing.
 *
 * The value is passed to parsetext which will call the right dictionary to
 * lexize the word. If it turns out to be a stopword, we push a QI_VALSTOP
 * to the stack.
 *
 * All words belonging to the same variant are pushed as an ANDed list,
 * and different variants are ORred together.
 */
static void
pushval_morph(Datum opaque, TSQueryParserState state, char *strval, int lenval, int2 weight, bool prefix)
{
	int4		count = 0;
	ParsedText	prs;
	uint32		variant,
				pos,
				cntvar = 0,
				cntpos = 0,
				cnt = 0;
	Oid			cfg_id = DatumGetObjectId(opaque);		/* the input is actually
														 * an Oid, not a pointer */

	prs.lenwords = 4;
	prs.curwords = 0;
	prs.pos = 0;
	prs.words = (ParsedWord *) palloc(sizeof(ParsedWord) * prs.lenwords);

	parsetext(cfg_id, &prs, strval, lenval);

	if (prs.curwords > 0)
	{

		while (count < prs.curwords)
		{
			pos = prs.words[count].pos.pos;
			cntvar = 0;
			while (count < prs.curwords && pos == prs.words[count].pos.pos)
			{
				variant = prs.words[count].nvariant;

				cnt = 0;
				while (count < prs.curwords && pos == prs.words[count].pos.pos && variant == prs.words[count].nvariant)
				{

					pushValue(state, prs.words[count].word, prs.words[count].len, weight,
							  ((prs.words[count].flags & TSL_PREFIX) || prefix) ? true : false);
					pfree(prs.words[count].word);
					if (cnt)
						pushOperator(state, OP_AND);
					cnt++;
					count++;
				}

				if (cntvar)
					pushOperator(state, OP_OR);
				cntvar++;
			}

			if (cntpos)
				pushOperator(state, OP_AND);

			cntpos++;
		}

		pfree(prs.words);

	}
	else
		pushStop(state);
}

Datum
to_tsquery_byid(PG_FUNCTION_ARGS)
{
	Oid			cfgid = PG_GETARG_OID(0);
	text	   *in = PG_GETARG_TEXT_P(1);
	TSQuery		query;
	QueryItem  *res;
	int4		len;

	query = parse_tsquery(text_to_cstring(in), pushval_morph, ObjectIdGetDatum(cfgid), false);

	if (query->size == 0)
		PG_RETURN_TSQUERY(query);

	/* clean out any stopword placeholders from the tree */
	res = clean_fakeval(GETQUERY(query), &len);
	if (!res)
	{
		SET_VARSIZE(query, HDRSIZETQ);
		query->size = 0;
		PG_RETURN_POINTER(query);
	}
	memcpy((void *) GETQUERY(query), (void *) res, len * sizeof(QueryItem));

	/*
	 * Removing the stopword placeholders might've resulted in fewer
	 * QueryItems. If so, move the operands up accordingly.
	 */
	if (len != query->size)
	{
		char	   *oldoperand = GETOPERAND(query);
		int4		lenoperand = VARSIZE(query) - (oldoperand - (char *) query);

		Assert(len < query->size);

		query->size = len;
		memmove((void *) GETOPERAND(query), oldoperand, VARSIZE(query) - (oldoperand - (char *) query));
		SET_VARSIZE(query, COMPUTESIZE(len, lenoperand));
	}

	pfree(res);
	PG_RETURN_TSQUERY(query);
}

Datum
to_tsquery(PG_FUNCTION_ARGS)
{
	text	   *in = PG_GETARG_TEXT_P(0);
	Oid			cfgId;

	cfgId = getTSCurrentConfig(true);
	PG_RETURN_DATUM(DirectFunctionCall2(to_tsquery_byid,
										ObjectIdGetDatum(cfgId),
										PointerGetDatum(in)));
}

Datum
plainto_tsquery_byid(PG_FUNCTION_ARGS)
{
	Oid			cfgid = PG_GETARG_OID(0);
	text	   *in = PG_GETARG_TEXT_P(1);
	TSQuery		query;
	QueryItem  *res;
	int4		len;

	query = parse_tsquery(text_to_cstring(in), pushval_morph, ObjectIdGetDatum(cfgid), true);

	if (query->size == 0)
		PG_RETURN_TSQUERY(query);

	res = clean_fakeval(GETQUERY(query), &len);
	if (!res)
	{
		SET_VARSIZE(query, HDRSIZETQ);
		query->size = 0;
		PG_RETURN_POINTER(query);
	}
	memcpy((void *) GETQUERY(query), (void *) res, len * sizeof(QueryItem));

	if (len != query->size)
	{
		char	   *oldoperand = GETOPERAND(query);
		int4		lenoperand = VARSIZE(query) - (oldoperand - (char *) query);

		Assert(len < query->size);

		query->size = len;
		memcpy((void *) GETOPERAND(query), oldoperand, lenoperand);
		SET_VARSIZE(query, COMPUTESIZE(len, lenoperand));
	}

	pfree(res);
	PG_RETURN_POINTER(query);
}

Datum
plainto_tsquery(PG_FUNCTION_ARGS)
{
	text	   *in = PG_GETARG_TEXT_P(0);
	Oid			cfgId;

	cfgId = getTSCurrentConfig(true);
	PG_RETURN_DATUM(DirectFunctionCall2(plainto_tsquery_byid,
										ObjectIdGetDatum(cfgId),
										PointerGetDatum(in)));
}
