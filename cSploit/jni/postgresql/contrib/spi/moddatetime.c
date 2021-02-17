/*
moddatetime.c

contrib/spi/moddatetime.c

What is this?
It is a function to be called from a trigger for the purpose of updating
a modification datetime stamp in a record when that record is UPDATEd.

Credits
This is 95%+ based on autoinc.c, which I used as a starting point as I do
not really know what I am doing.  I also had help from
Jan Wieck <jwieck@debis.com> who told me about the timestamp_in("now") function.
OH, me, I'm Terry Mackintosh <terry@terrym.com>
*/
#include "postgres.h"

#include "catalog/pg_type.h"
#include "executor/spi.h"
#include "commands/trigger.h"

PG_MODULE_MAGIC;

extern Datum moddatetime(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(moddatetime);

Datum
moddatetime(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	Trigger    *trigger;		/* to get trigger name */
	int			nargs;			/* # of arguments */
	int			attnum;			/* positional number of field to change */
	Oid			atttypid;		/* type OID of field to change */
	Datum		newdt;			/* The current datetime. */
	char	  **args;			/* arguments */
	char	   *relname;		/* triggered relation name */
	Relation	rel;			/* triggered relation */
	HeapTuple	rettuple = NULL;
	TupleDesc	tupdesc;		/* tuple description */

	if (!CALLED_AS_TRIGGER(fcinfo))
		/* internal error */
		elog(ERROR, "moddatetime: not fired by trigger manager");

	if (!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		/* internal error */
		elog(ERROR, "moddatetime: must be fired for row");

	if (!TRIGGER_FIRED_BEFORE(trigdata->tg_event))
		/* internal error */
		elog(ERROR, "moddatetime: must be fired before event");

	if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
		/* internal error */
		elog(ERROR, "moddatetime: cannot process INSERT events");
	else if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		rettuple = trigdata->tg_newtuple;
	else
		/* internal error */
		elog(ERROR, "moddatetime: cannot process DELETE events");

	rel = trigdata->tg_relation;
	relname = SPI_getrelname(rel);

	trigger = trigdata->tg_trigger;

	nargs = trigger->tgnargs;

	if (nargs != 1)
		/* internal error */
		elog(ERROR, "moddatetime (%s): A single argument was expected", relname);

	args = trigger->tgargs;
	/* must be the field layout? */
	tupdesc = rel->rd_att;

	/*
	 * This gets the position in the tuple of the field we want. args[0] being
	 * the name of the field to update, as passed in from the trigger.
	 */
	attnum = SPI_fnumber(tupdesc, args[0]);

	/*
	 * This is where we check to see if the field we are supposed to update
	 * even exists. The above function must return -1 if name not found?
	 */
	if (attnum < 0)
		ereport(ERROR,
				(errcode(ERRCODE_TRIGGERED_ACTION_EXCEPTION),
				 errmsg("\"%s\" has no attribute \"%s\"",
						relname, args[0])));

	/*
	 * Check the target field has an allowed type, and get the current
	 * datetime as a value of that type.
	 */
	atttypid = SPI_gettypeid(tupdesc, attnum);
	if (atttypid == TIMESTAMPOID)
		newdt = DirectFunctionCall3(timestamp_in,
									CStringGetDatum("now"),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
	else if (atttypid == TIMESTAMPTZOID)
		newdt = DirectFunctionCall3(timestamptz_in,
									CStringGetDatum("now"),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_TRIGGERED_ACTION_EXCEPTION),
				 errmsg("attribute \"%s\" of \"%s\" must be type TIMESTAMP or TIMESTAMPTZ",
						args[0], relname)));
		newdt = (Datum) 0;		/* keep compiler quiet */
	}

/* 1 is the number of items in the arrays attnum and newdt.
	attnum is the positional number of the field to be updated.
	newdt is the new datetime stamp.
	NOTE that attnum and newdt are not arrays, but then a 1 element array
	is not an array any more then they are.  Thus, they can be considered a
	one element array.
*/
	rettuple = SPI_modifytuple(rel, rettuple, 1, &attnum, &newdt, NULL);

	if (rettuple == NULL)
		/* internal error */
		elog(ERROR, "moddatetime (%s): %d returned by SPI_modifytuple",
			 relname, SPI_result);

/* Clean up */
	pfree(relname);

	return PointerGetDatum(rettuple);
}
