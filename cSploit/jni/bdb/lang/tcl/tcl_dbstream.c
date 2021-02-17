/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
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
 * Prototypes for procedures defined later in this file:
 */
static int tcl_DbstreamRead __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB_STREAM *));
static int tcl_DbstreamWrite __P((Tcl_Interp *,
    int, Tcl_Obj * CONST*, DB_STREAM *));

/*
 * PUBLIC: int dbstream_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
 *
 * dbstream_cmd --
 *	Implements the database stream command.
 */
int
dbstream_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* Database stream handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *dbscmds[] = {
		"close",
		"read",
		"size",
		"write",
		NULL
	};
	enum dbscmds {
		DBSTREAMCLOSE,
		DBSTREAMREAD,
		DBSTREAMSIZE,
		DBSTREAMWRITE
	};
	DB_STREAM *dbs;
	Tcl_Obj *res;
	DBTCL_INFO *dbip;
	db_off_t size;
	int cmdindex, result, ret;

	Tcl_ResetResult(interp);
	dbs = (DB_STREAM *)clientData;
	dbip = _PtrToInfo((void *)dbs);
	size = 0;
	ret = 0;
	result = TCL_OK;

	if (objc <= 1) {
		Tcl_WrongNumArgs(interp, 1, objv, "command cmdargs");
		return (TCL_ERROR);
	}
	if (dbs == NULL) {
		Tcl_SetResult(interp, "NULL dbstream pointer", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (dbip == NULL) {
		Tcl_SetResult(interp, "NULL dbstream info pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the berkdbcmds
	 * defined above.
	 */
	if (Tcl_GetIndexFromObj(interp, objv[1], dbscmds, "command",
	    TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));
	switch ((enum dbscmds)cmdindex) {
	case DBSTREAMCLOSE:
		/* No args for this.  Error if there are some. */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbs->close(dbs, 0);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "dbstream size")) == TCL_OK) {
			(void)Tcl_DeleteCommand(interp, dbip->i_name);
			_DeleteInfo(dbip);
		}
		break;
	case DBSTREAMSIZE:
		/* No args for this.  Error if there are some. */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbs->size(dbs, &size, 0);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "dbstream size")) == TCL_OK) {
			res = Tcl_NewWideIntObj((Tcl_WideInt)size);
			Tcl_SetObjResult(interp, res);
		}
		break;
	case DBSTREAMREAD:
		result = tcl_DbstreamRead(interp, objc, objv, dbs);
		break;
	case DBSTREAMWRITE:
		result = tcl_DbstreamWrite(interp, objc, objv, dbs);
		break;
	}
	return (result);
}

/*
 * tcl_DbstreamRead --
 */
static int
tcl_DbstreamRead(interp, objc, objv, dbs)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_STREAM *dbs;			/* Database stream pointer */
{
	static const char *dbsreadopts[] = {
		"-offset",
		"-size",
		NULL
	};
	enum dbsreadopts {
		DBSTREAMREAD_OFFSET,
		DBSTREAMREAD_SIZE
	};
	Tcl_Obj *res;
	Tcl_WideInt offtmp;
	DBT data;
	db_off_t offset;
	u_int32_t size;
	int i, optindex, result, ret;

	offtmp = 0;
	memset(&data, 0, sizeof(DBT));
	offset = 0;
	size = 0;
	result = TCL_OK;

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-args?");
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the options
	 * defined above.
	 */
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbsreadopts,
		    "option", TCL_EXACT, &optindex) != TCL_OK) {
			/*
			 * Reset the result so we don't get
			 * an errant error message if there is another error.
			 */
			if (IS_HELP(objv[i]) == TCL_OK) {
				result = TCL_OK;
				goto out;
			}
			Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum dbsreadopts)optindex) {
		case DBSTREAMREAD_OFFSET:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-offset offset?");
				result = TCL_ERROR;
				goto out;
			}
			if ((result = Tcl_GetWideIntFromObj(interp,
			    objv[i++], &offtmp)) != TCL_OK)
				goto out;
			offset = (db_off_t)offtmp;
			break;
		case DBSTREAMREAD_SIZE:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-size bytes?");
				result = TCL_ERROR;
				goto out;
			}
			if ((result = _GetUInt32(interp,
			    objv[i++], &size)) != TCL_OK)
				goto out;
			break;
		}
	}
	_debug_check();
	data.flags = DB_DBT_MALLOC;
	ret = dbs->read(dbs, &data, offset, size, 0);
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "dbstream read")) == TCL_OK) {
		res = NewStringObj(data.data, data.size);
		Tcl_SetObjResult(interp, res);
	}
out:	if (data.data != NULL)
		__os_ufree(dbs->dbc->env, data.data);
	return (result);
}

/*
 * tcl_DbstreamWrite --
 */
static int
tcl_DbstreamWrite(interp, objc, objv, dbs)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_STREAM *dbs;			/* Database stream pointer */
{
	static const char *dbsreadopts[] = {
		"-offset",
		NULL
	};
	enum dbsreadopts {
		DBSTREAMWRITE_OFFSET
	};
	Tcl_WideInt offtmp;
	DBT data;
	db_off_t offset;
	int freedata, i, optindex, result, ret;

	offtmp = 0;
	memset(&data, 0, sizeof(DBT));
	offset = 0;
	freedata = 0;
	result = TCL_OK;

	if (objc != 3 && objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-args? data");
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the options
	 * defined above.
	 */
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], dbsreadopts,
		    "option", TCL_EXACT, &optindex) != TCL_OK) {
			/*
			 * Reset the result so we don't get
			 * an errant error message if there is another error.
			 */
			if (IS_HELP(objv[i]) == TCL_OK) {
				result = TCL_OK;
				goto out;
			}
			Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum dbsreadopts)optindex) {
		case DBSTREAMWRITE_OFFSET:
			if (i >= objc || objc != 5) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-offset offset? data");
				result = TCL_ERROR;
				goto out;
			}
			if ((result = Tcl_GetWideIntFromObj(interp,
			    objv[i++], &offtmp)) != TCL_OK)
				goto out;
			offset = (db_off_t)offtmp;
			break;
		}
	}
	_debug_check();
	ret = _CopyObjBytes(interp, objv[objc-1],
	    &data.data, &data.size, &freedata);
	if (ret != 0) {
		result = _ReturnSetup(interp, ret,
		    DB_RETOK_STD(ret), "dbstream write");
		goto out;
	}
	ret = dbs->write(dbs, &data, offset, 0);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "dbstream write");
		
out:	if (freedata && data.data != NULL)
		__os_free(NULL, data.data);
	return (result);
}
