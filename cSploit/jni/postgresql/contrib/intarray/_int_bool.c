/*
 * contrib/intarray/_int_bool.c
 */
#include "postgres.h"

#include "miscadmin.h"
#include "utils/builtins.h"

#include "_int.h"

PG_FUNCTION_INFO_V1(bqarr_in);
PG_FUNCTION_INFO_V1(bqarr_out);
Datum		bqarr_in(PG_FUNCTION_ARGS);
Datum		bqarr_out(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(boolop);
Datum		boolop(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(rboolop);
Datum		rboolop(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(querytree);
Datum		querytree(PG_FUNCTION_ARGS);


/* parser's states */
#define WAITOPERAND 1
#define WAITENDOPERAND	2
#define WAITOPERATOR	3

/*
 * node of query tree, also used
 * for storing polish notation in parser
 */
typedef struct NODE
{
	int4		type;
	int4		val;
	struct NODE *next;
} NODE;

typedef struct
{
	char	   *buf;
	int4		state;
	int4		count;
	/* reverse polish notation in list (for temporary usage) */
	NODE	   *str;
	/* number in str */
	int4		num;
} WORKSTATE;

/*
 * get token from query string
 */
static int4
gettoken(WORKSTATE *state, int4 *val)
{
	char		nnn[16];
	int			innn;

	*val = 0;					/* default result */

	innn = 0;
	while (1)
	{
		if (innn >= sizeof(nnn))
			return ERR;			/* buffer overrun => syntax error */
		switch (state->state)
		{
			case WAITOPERAND:
				innn = 0;
				if ((*(state->buf) >= '0' && *(state->buf) <= '9') ||
					*(state->buf) == '-')
				{
					state->state = WAITENDOPERAND;
					nnn[innn++] = *(state->buf);
				}
				else if (*(state->buf) == '!')
				{
					(state->buf)++;
					*val = (int4) '!';
					return OPR;
				}
				else if (*(state->buf) == '(')
				{
					state->count++;
					(state->buf)++;
					return OPEN;
				}
				else if (*(state->buf) != ' ')
					return ERR;
				break;
			case WAITENDOPERAND:
				if (*(state->buf) >= '0' && *(state->buf) <= '9')
				{
					nnn[innn++] = *(state->buf);
				}
				else
				{
					long		lval;

					nnn[innn] = '\0';
					errno = 0;
					lval = strtol(nnn, NULL, 0);
					*val = (int4) lval;
					if (errno != 0 || (long) *val != lval)
						return ERR;
					state->state = WAITOPERATOR;
					return (state->count && *(state->buf) == '\0')
						? ERR : VAL;
				}
				break;
			case WAITOPERATOR:
				if (*(state->buf) == '&' || *(state->buf) == '|')
				{
					state->state = WAITOPERAND;
					*val = (int4) *(state->buf);
					(state->buf)++;
					return OPR;
				}
				else if (*(state->buf) == ')')
				{
					(state->buf)++;
					state->count--;
					return (state->count < 0) ? ERR : CLOSE;
				}
				else if (*(state->buf) == '\0')
					return (state->count) ? ERR : END;
				else if (*(state->buf) != ' ')
					return ERR;
				break;
			default:
				return ERR;
				break;
		}
		(state->buf)++;
	}
	return END;
}

/*
 * push new one in polish notation reverse view
 */
static void
pushquery(WORKSTATE *state, int4 type, int4 val)
{
	NODE	   *tmp = (NODE *) palloc(sizeof(NODE));

	tmp->type = type;
	tmp->val = val;
	tmp->next = state->str;
	state->str = tmp;
	state->num++;
}

#define STACKDEPTH	16

/*
 * make polish notation of query
 */
static int4
makepol(WORKSTATE *state)
{
	int4		val,
				type;
	int4		stack[STACKDEPTH];
	int4		lenstack = 0;

	/* since this function recurses, it could be driven to stack overflow */
	check_stack_depth();

	while ((type = gettoken(state, &val)) != END)
	{
		switch (type)
		{
			case VAL:
				pushquery(state, type, val);
				while (lenstack && (stack[lenstack - 1] == (int4) '&' ||
									stack[lenstack - 1] == (int4) '!'))
				{
					lenstack--;
					pushquery(state, OPR, stack[lenstack]);
				}
				break;
			case OPR:
				if (lenstack && val == (int4) '|')
					pushquery(state, OPR, val);
				else
				{
					if (lenstack == STACKDEPTH)
						ereport(ERROR,
								(errcode(ERRCODE_STATEMENT_TOO_COMPLEX),
								 errmsg("statement too complex")));
					stack[lenstack] = val;
					lenstack++;
				}
				break;
			case OPEN:
				if (makepol(state) == ERR)
					return ERR;
				while (lenstack && (stack[lenstack - 1] == (int4) '&' ||
									stack[lenstack - 1] == (int4) '!'))
				{
					lenstack--;
					pushquery(state, OPR, stack[lenstack]);
				}
				break;
			case CLOSE:
				while (lenstack)
				{
					lenstack--;
					pushquery(state, OPR, stack[lenstack]);
				};
				return END;
				break;
			case ERR:
			default:
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("syntax error")));
				return ERR;

		}
	}

	while (lenstack)
	{
		lenstack--;
		pushquery(state, OPR, stack[lenstack]);
	};
	return END;
}

typedef struct
{
	int4	   *arrb;
	int4	   *arre;
} CHKVAL;

/*
 * is there value 'val' in (sorted) array or not ?
 */
static bool
checkcondition_arr(void *checkval, ITEM *item)
{
	int4	   *StopLow = ((CHKVAL *) checkval)->arrb;
	int4	   *StopHigh = ((CHKVAL *) checkval)->arre;
	int4	   *StopMiddle;

	/* Loop invariant: StopLow <= val < StopHigh */

	while (StopLow < StopHigh)
	{
		StopMiddle = StopLow + (StopHigh - StopLow) / 2;
		if (*StopMiddle == item->val)
			return (true);
		else if (*StopMiddle < item->val)
			StopLow = StopMiddle + 1;
		else
			StopHigh = StopMiddle;
	}
	return false;
}

static bool
checkcondition_bit(void *checkval, ITEM *item)
{
	return GETBIT(checkval, HASHVAL(item->val));
}

/*
 * evaluate boolean expression, using chkcond() to test the primitive cases
 */
static bool
execute(ITEM *curitem, void *checkval, bool calcnot,
		bool (*chkcond) (void *checkval, ITEM *item))
{
	/* since this function recurses, it could be driven to stack overflow */
	check_stack_depth();

	if (curitem->type == VAL)
		return (*chkcond) (checkval, curitem);
	else if (curitem->val == (int4) '!')
	{
		return (calcnot) ?
			((execute(curitem - 1, checkval, calcnot, chkcond)) ? false : true)
			: true;
	}
	else if (curitem->val == (int4) '&')
	{
		if (execute(curitem + curitem->left, checkval, calcnot, chkcond))
			return execute(curitem - 1, checkval, calcnot, chkcond);
		else
			return false;
	}
	else
	{							/* |-operator */
		if (execute(curitem + curitem->left, checkval, calcnot, chkcond))
			return true;
		else
			return execute(curitem - 1, checkval, calcnot, chkcond);
	}
	return false;
}

/*
 * signconsistent & execconsistent called by *_consistent
 */
bool
signconsistent(QUERYTYPE *query, BITVEC sign, bool calcnot)
{
	return execute(GETQUERY(query) + query->size - 1,
				   (void *) sign, calcnot,
				   checkcondition_bit);
}

/* Array must be sorted! */
bool
execconsistent(QUERYTYPE *query, ArrayType *array, bool calcnot)
{
	CHKVAL		chkval;

	CHECKARRVALID(array);
	chkval.arrb = ARRPTR(array);
	chkval.arre = chkval.arrb + ARRNELEMS(array);
	return execute(GETQUERY(query) + query->size - 1,
				   (void *) &chkval, calcnot,
				   checkcondition_arr);
}

typedef struct
{
	ITEM	   *first;
	bool	   *mapped_check;
} GinChkVal;

static bool
checkcondition_gin(void *checkval, ITEM *item)
{
	GinChkVal  *gcv = (GinChkVal *) checkval;

	return gcv->mapped_check[item - gcv->first];
}

bool
gin_bool_consistent(QUERYTYPE *query, bool *check)
{
	GinChkVal	gcv;
	ITEM	   *items = GETQUERY(query);
	int			i,
				j = 0;

	if (query->size <= 0)
		return FALSE;

	/*
	 * Set up data for checkcondition_gin.	This must agree with the query
	 * extraction code in ginint4_queryextract.
	 */
	gcv.first = items;
	gcv.mapped_check = (bool *) palloc(sizeof(bool) * query->size);
	for (i = 0; i < query->size; i++)
	{
		if (items[i].type == VAL)
			gcv.mapped_check[i] = check[j++];
	}

	return execute(GETQUERY(query) + query->size - 1,
				   (void *) &gcv, true,
				   checkcondition_gin);
}

static bool
contains_required_value(ITEM *curitem)
{
	/* since this function recurses, it could be driven to stack overflow */
	check_stack_depth();

	if (curitem->type == VAL)
		return true;
	else if (curitem->val == (int4) '!')
	{
		/*
		 * Assume anything under a NOT is non-required.  For some cases with
		 * nested NOTs, we could prove there's a required value, but it seems
		 * unlikely to be worth the trouble.
		 */
		return false;
	}
	else if (curitem->val == (int4) '&')
	{
		/* If either side has a required value, we're good */
		if (contains_required_value(curitem + curitem->left))
			return true;
		else
			return contains_required_value(curitem - 1);
	}
	else
	{							/* |-operator */
		/* Both sides must have required values */
		if (contains_required_value(curitem + curitem->left))
			return contains_required_value(curitem - 1);
		else
			return false;
	}
	return false;
}

bool
query_has_required_values(QUERYTYPE *query)
{
	if (query->size <= 0)
		return false;
	return contains_required_value(GETQUERY(query) + query->size - 1);
}

/*
 * boolean operations
 */
Datum
rboolop(PG_FUNCTION_ARGS)
{
	/* just reverse the operands */
	return DirectFunctionCall2(boolop,
							   PG_GETARG_DATUM(1),
							   PG_GETARG_DATUM(0));
}

Datum
boolop(PG_FUNCTION_ARGS)
{
	ArrayType  *val = PG_GETARG_ARRAYTYPE_P_COPY(0);
	QUERYTYPE  *query = PG_GETARG_QUERYTYPE_P(1);
	CHKVAL		chkval;
	bool		result;

	CHECKARRVALID(val);
	PREPAREARR(val);
	chkval.arrb = ARRPTR(val);
	chkval.arre = chkval.arrb + ARRNELEMS(val);
	result = execute(GETQUERY(query) + query->size - 1,
					 &chkval, true,
					 checkcondition_arr);
	pfree(val);

	PG_FREE_IF_COPY(query, 1);
	PG_RETURN_BOOL(result);
}

static void
findoprnd(ITEM *ptr, int4 *pos)
{
#ifdef BS_DEBUG
	elog(DEBUG3, (ptr[*pos].type == OPR) ?
		 "%d  %c" : "%d  %d", *pos, ptr[*pos].val);
#endif
	if (ptr[*pos].type == VAL)
	{
		ptr[*pos].left = 0;
		(*pos)--;
	}
	else if (ptr[*pos].val == (int4) '!')
	{
		ptr[*pos].left = -1;
		(*pos)--;
		findoprnd(ptr, pos);
	}
	else
	{
		ITEM	   *curitem = &ptr[*pos];
		int4		tmp = *pos;

		(*pos)--;
		findoprnd(ptr, pos);
		curitem->left = *pos - tmp;
		findoprnd(ptr, pos);
	}
}


/*
 * input
 */
Datum
bqarr_in(PG_FUNCTION_ARGS)
{
	char	   *buf = (char *) PG_GETARG_POINTER(0);
	WORKSTATE	state;
	int4		i;
	QUERYTYPE  *query;
	int4		commonlen;
	ITEM	   *ptr;
	NODE	   *tmp;
	int4		pos = 0;

#ifdef BS_DEBUG
	StringInfoData pbuf;
#endif

	state.buf = buf;
	state.state = WAITOPERAND;
	state.count = 0;
	state.num = 0;
	state.str = NULL;

	/* make polish notation (postfix, but in reverse order) */
	makepol(&state);
	if (!state.num)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("empty query")));

	commonlen = COMPUTESIZE(state.num);
	query = (QUERYTYPE *) palloc(commonlen);
	SET_VARSIZE(query, commonlen);
	query->size = state.num;
	ptr = GETQUERY(query);

	for (i = state.num - 1; i >= 0; i--)
	{
		ptr[i].type = state.str->type;
		ptr[i].val = state.str->val;
		tmp = state.str->next;
		pfree(state.str);
		state.str = tmp;
	}

	pos = query->size - 1;
	findoprnd(ptr, &pos);
#ifdef BS_DEBUG
	initStringInfo(&pbuf);
	for (i = 0; i < query->size; i++)
	{
		if (ptr[i].type == OPR)
			appendStringInfo(&pbuf, "%c(%d) ", ptr[i].val, ptr[i].left);
		else
			appendStringInfo(&pbuf, "%d ", ptr[i].val);
	}
	elog(DEBUG3, "POR: %s", pbuf.data);
	pfree(pbuf.data);
#endif

	PG_RETURN_POINTER(query);
}


/*
 * out function
 */
typedef struct
{
	ITEM	   *curpol;
	char	   *buf;
	char	   *cur;
	int4		buflen;
} INFIX;

#define RESIZEBUF(inf,addsize) while( ( (inf)->cur - (inf)->buf ) + (addsize) + 1 >= (inf)->buflen ) { \
	int4 len = inf->cur - inf->buf; \
	inf->buflen *= 2; \
	inf->buf = (char*) repalloc( (void*)inf->buf, inf->buflen ); \
	inf->cur = inf->buf + len; \
}

static void
infix(INFIX *in, bool first)
{
	if (in->curpol->type == VAL)
	{
		RESIZEBUF(in, 11);
		sprintf(in->cur, "%d", in->curpol->val);
		in->cur = strchr(in->cur, '\0');
		in->curpol--;
	}
	else if (in->curpol->val == (int4) '!')
	{
		bool		isopr = false;

		RESIZEBUF(in, 1);
		*(in->cur) = '!';
		in->cur++;
		*(in->cur) = '\0';
		in->curpol--;
		if (in->curpol->type == OPR)
		{
			isopr = true;
			RESIZEBUF(in, 2);
			sprintf(in->cur, "( ");
			in->cur = strchr(in->cur, '\0');
		}
		infix(in, isopr);
		if (isopr)
		{
			RESIZEBUF(in, 2);
			sprintf(in->cur, " )");
			in->cur = strchr(in->cur, '\0');
		}
	}
	else
	{
		int4		op = in->curpol->val;
		INFIX		nrm;

		in->curpol--;
		if (op == (int4) '|' && !first)
		{
			RESIZEBUF(in, 2);
			sprintf(in->cur, "( ");
			in->cur = strchr(in->cur, '\0');
		}

		nrm.curpol = in->curpol;
		nrm.buflen = 16;
		nrm.cur = nrm.buf = (char *) palloc(sizeof(char) * nrm.buflen);

		/* get right operand */
		infix(&nrm, false);

		/* get & print left operand */
		in->curpol = nrm.curpol;
		infix(in, false);

		/* print operator & right operand */
		RESIZEBUF(in, 3 + (nrm.cur - nrm.buf));
		sprintf(in->cur, " %c %s", op, nrm.buf);
		in->cur = strchr(in->cur, '\0');
		pfree(nrm.buf);

		if (op == (int4) '|' && !first)
		{
			RESIZEBUF(in, 2);
			sprintf(in->cur, " )");
			in->cur = strchr(in->cur, '\0');
		}
	}
}


Datum
bqarr_out(PG_FUNCTION_ARGS)
{
	QUERYTYPE  *query = PG_GETARG_QUERYTYPE_P(0);
	INFIX		nrm;

	if (query->size == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("empty query")));

	nrm.curpol = GETQUERY(query) + query->size - 1;
	nrm.buflen = 32;
	nrm.cur = nrm.buf = (char *) palloc(sizeof(char) * nrm.buflen);
	*(nrm.cur) = '\0';
	infix(&nrm, true);

	PG_FREE_IF_COPY(query, 0);
	PG_RETURN_POINTER(nrm.buf);
}


/* Useless old "debugging" function for a fundamentally wrong algorithm */
Datum
querytree(PG_FUNCTION_ARGS)
{
	elog(ERROR, "querytree is no longer implemented");
	PG_RETURN_NULL();
}
