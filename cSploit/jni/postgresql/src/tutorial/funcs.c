/* src/tutorial/funcs.c */

/******************************************************************************
  These are user-defined functions that can be bound to a Postgres backend
  and called by Postgres to execute SQL functions of the same name.

  The calling format for these functions is defined by the CREATE FUNCTION
  SQL statement that binds them to the backend.

  NOTE: this file shows examples of "old style" function call conventions.
  See funcs_new.c for examples of "new style".
*****************************************************************************/

#include "postgres.h"			/* general Postgres declarations */

#include "executor/executor.h"	/* for GetAttributeByName() */
#include "utils/geo_decls.h"	/* for point type */

PG_MODULE_MAGIC;


/* These prototypes just prevent possible warnings from gcc. */

int			add_one(int arg);
float8	   *add_one_float8(float8 *arg);
Point	   *makepoint(Point *pointx, Point *pointy);
text	   *copytext(text *t);
text	   *concat_text(text *arg1, text *arg2);
bool c_overpaid(HeapTupleHeader t,	/* the current instance of EMP */
		   int32 limit);


/* By Value */

int
add_one(int arg)
{
	return arg + 1;
}

/* By Reference, Fixed Length */

float8 *
add_one_float8(float8 *arg)
{
	float8	   *result = (float8 *) palloc(sizeof(float8));

	*result = *arg + 1.0;

	return result;
}

Point *
makepoint(Point *pointx, Point *pointy)
{
	Point	   *new_point = (Point *) palloc(sizeof(Point));

	new_point->x = pointx->x;
	new_point->y = pointy->y;

	return new_point;
}

/* By Reference, Variable Length */

text *
copytext(text *t)
{
	/*
	 * VARSIZE is the total size of the struct in bytes.
	 */
	text	   *new_t = (text *) palloc(VARSIZE(t));

	SET_VARSIZE(new_t, VARSIZE(t));

	/*
	 * VARDATA is a pointer to the data region of the struct.
	 */
	memcpy((void *) VARDATA(new_t),		/* destination */
		   (void *) VARDATA(t), /* source */
		   VARSIZE(t) - VARHDRSZ);		/* how many bytes */
	return new_t;
}

text *
concat_text(text *arg1, text *arg2)
{
	int32		arg1_size = VARSIZE(arg1) - VARHDRSZ;
	int32		arg2_size = VARSIZE(arg2) - VARHDRSZ;
	int32		new_text_size = arg1_size + arg2_size + VARHDRSZ;
	text	   *new_text = (text *) palloc(new_text_size);

	SET_VARSIZE(new_text, new_text_size);
	memcpy(VARDATA(new_text), VARDATA(arg1), arg1_size);
	memcpy(VARDATA(new_text) + arg1_size, VARDATA(arg2), arg2_size);
	return new_text;
}

/* Composite types */

bool
c_overpaid(HeapTupleHeader t,	/* the current instance of EMP */
		   int32 limit)
{
	bool		isnull;
	int32		salary;

	salary = DatumGetInt32(GetAttributeByName(t, "salary", &isnull));
	if (isnull)
		return false;
	return salary > limit;
}
