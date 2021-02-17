/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#ifdef HAVE_SYSTEM_INCLUDE_FILES
#include <tcl.h>
#endif
#include "dbinc/tcl_db.h"

/*
 * bdb_RandCommand --
 *	Implements rand* functions.
 *
 * PUBLIC: int bdb_RandCommand __P((Tcl_Interp *, int, Tcl_Obj * CONST*));
 */
int
bdb_RandCommand(interp, objc, objv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *rcmds[] = {
		"rand",	"random_int",	"srand",
		NULL
	};
	enum rcmds {
		RRAND, RRAND_INT, RSRAND
	};
	Tcl_Obj *res;
	int cmdindex, hi, lo, result, ret;

	result = TCL_OK;
	/*
	 * Get the command name index from the object based on the cmds
	 * defined above.  This SHOULD NOT fail because we already checked
	 * in the 'berkdb' command.
	 */
	if (Tcl_GetIndexFromObj(interp,
	    objv[1], rcmds, "command", TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));

	res = NULL;
	switch ((enum rcmds)cmdindex) {
	case RRAND:
		/*
		 * Must be 0 args.  Error if different.
		 */
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
#ifdef	HAVE_RANDOM
		ret = random();
#else
		ret = rand();
#endif
		res = Tcl_NewIntObj(ret);
		break;
	case RRAND_INT:
		/*
		 * Must be 4 args.  Error if different.
		 */
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 2, objv, "lo hi");
			return (TCL_ERROR);
		}
		if ((result =
		    Tcl_GetIntFromObj(interp, objv[2], &lo)) != TCL_OK)
			return (result);
		if ((result =
		    Tcl_GetIntFromObj(interp, objv[3], &hi)) != TCL_OK)
			return (result);
		if (lo < 0 || hi < 0) {
			Tcl_SetResult(interp,
			    "Range value less than 0", TCL_STATIC);
			return (TCL_ERROR);
		}

		_debug_check();
#ifdef	HAVE_RANDOM
		ret = lo + random() % ((hi - lo) + 1);
#else
		ret = lo + rand() % ((hi - lo) + 1);
#endif
		res = Tcl_NewIntObj(ret);
		break;
	case RSRAND:
		/*
		 * Must be 1 arg.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "seed");
			return (TCL_ERROR);
		}
		if ((result =
		    Tcl_GetIntFromObj(interp, objv[2], &lo)) == TCL_OK) {
#ifdef	HAVE_RANDOM
			srandom((u_int)lo);
#else
			srand((u_int)lo);
#endif
			res = Tcl_NewIntObj(0);
		}
		break;
	}

	/*
	 * Only set result if we have a res.  Otherwise, lower functions have
	 * already done so.
	 */
	if (result == TCL_OK && res)
		Tcl_SetObjResult(interp, res);
	return (result);
}

/*
 * PUBLIC: int tcl_LockMutex __P((DB_ENV *, db_mutex_t));
 */
int
tcl_LockMutex(dbenv, mutex)
	DB_ENV *dbenv;
	db_mutex_t mutex;
{
	/*
	 * Why such a seemingly ridiculously trivial function?  MUTEX_LOCK can't
	 * be invoked in a void function.  The behavior of the macro could be
	 * unwrapped and duplicated in line; but by the time you account for
	 * HAVE_MUTEX_SUPPORT, checking for MUTEX_INVALID, etc., you've created
	 * a maintenance burden, and it's just not worth it.
	 */ 
	MUTEX_LOCK(dbenv->env, mutex);
	return (0);
}

/*
 * PUBLIC: int tcl_UnlockMutex __P((DB_ENV *, db_mutex_t));
 */
int
tcl_UnlockMutex(dbenv, mutex)
	DB_ENV *dbenv;
	db_mutex_t mutex;
{
	MUTEX_UNLOCK(dbenv->env, mutex);
	return (0);
}
