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

#ifdef CONFIG_TEST
typedef struct {
	const char *name;
	u_int32_t value;
} NAMEMAP;

static const NAMEMAP rep_ack_policies[] = {
	{"all",			DB_REPMGR_ACKS_ALL},
	{"allavailable",	DB_REPMGR_ACKS_ALL_AVAILABLE},
	{"allpeers",		DB_REPMGR_ACKS_ALL_PEERS},
	{"none",		DB_REPMGR_ACKS_NONE},
	{"one",			DB_REPMGR_ACKS_ONE},
	{"onepeer",		DB_REPMGR_ACKS_ONE_PEER},
	{"quorum",		DB_REPMGR_ACKS_QUORUM},
	{NULL,			0}
};

static const NAMEMAP rep_config_types[] = {
	{"autoinit",		DB_REP_CONF_AUTOINIT},
	{"autorollback",	DB_REP_CONF_AUTOROLLBACK},
	{"bulk",		DB_REP_CONF_BULK},
	{"delayclient",		DB_REP_CONF_DELAYCLIENT},
	{"inmem",		DB_REP_CONF_INMEM},
	{"lease",		DB_REP_CONF_LEASE},
	{"mgr2sitestrict",	DB_REPMGR_CONF_2SITE_STRICT},
	{"mgrelections",	DB_REPMGR_CONF_ELECTIONS},
	{"nowait",		DB_REP_CONF_NOWAIT},
	{NULL,			0}
};

static const NAMEMAP rep_timeout_types[] = {
	{"ack",			DB_REP_ACK_TIMEOUT},
	{"checkpoint_delay",	DB_REP_CHECKPOINT_DELAY},
	{"connection_retry",	DB_REP_CONNECTION_RETRY},
	{"election",		DB_REP_ELECTION_TIMEOUT},
	{"election_retry",	DB_REP_ELECTION_RETRY},
	{"full_election",	DB_REP_FULL_ELECTION_TIMEOUT},
	{"heartbeat_monitor",	DB_REP_HEARTBEAT_MONITOR},
	{"heartbeat_send",	DB_REP_HEARTBEAT_SEND},
	{"lease",		DB_REP_LEASE_TIMEOUT},
	{NULL,			0}
};

static int tcl_RepNumberToName __P((const NAMEMAP *, u_int32_t, const char **));

/*
 * tcl_RepConfig --
 *	Call DB_ENV->rep_set_config().
 *
 * PUBLIC: int tcl_RepConfig
 * PUBLIC:     __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
 */
int
tcl_RepConfig(interp, dbenv, list)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Environment pointer */
	Tcl_Obj *list;			/* {which on|off} */
{
	static const char *confonoff[] = {
		"off",
		"on",
		NULL
	};
	enum confonoff {
		REPCONF_OFF,
		REPCONF_ON
	};
	Tcl_Obj **myobjv, *onoff, *which;
	int myobjc, on, optindex, result, ret;
	u_int32_t wh;

	result = Tcl_ListObjGetElements(interp, list, &myobjc, &myobjv);
	if (result != TCL_OK)
		return (result);
	which = myobjv[0];
	onoff = myobjv[1];
	if (Tcl_GetIndexFromObjStruct(interp, which,
		&rep_config_types[0].name, sizeof(NAMEMAP),
		"config type", TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(which));

	wh = rep_config_types[optindex].value;
	if (Tcl_GetIndexFromObj(interp, onoff, confonoff, "option",
	    TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(onoff));
	switch ((enum confonoff)optindex) {
	case REPCONF_OFF:
		on = 0;
		break;
	case REPCONF_ON:
		on = 1;
		break;
	default:
		return (TCL_ERROR);
	}
	ret = dbenv->rep_set_config(dbenv, wh, on);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env rep_config"));
}

/*
 * tcl_RepGetTwo --
 *	Call replication getters that return 2 values.
 *
 * PUBLIC: int tcl_RepGetTwo
 * PUBLIC:     __P((Tcl_Interp *, DB_ENV *, int));
 */
int
tcl_RepGetTwo(interp, dbenv, op)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Environment pointer */
	int op;				/* which getter */
{
	Tcl_Obj *myobjv[2], *res;
	u_int32_t val1, val2;
	int myobjc, result, ret;

	ret = 0;
	val1 = val2 = 0;
	switch (op) {
	case DBTCL_GETCLOCK:
		ret = dbenv->rep_get_clockskew(dbenv, &val1, &val2);
		break;
	case DBTCL_GETINQUEUE:
		ret = dbenv->repmgr_get_incoming_queue_max(dbenv,
		    &val1, &val2);
		break;
	case DBTCL_GETLIMIT:
		ret = dbenv->rep_get_limit(dbenv, &val1, &val2);
		break;
	case DBTCL_GETREQ:
		ret = dbenv->rep_get_request(dbenv, &val1, &val2);
		break;
	default:
		return (TCL_ERROR);
	}
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env rep_get")) == TCL_OK) {
		myobjc = 2;
		myobjv[0] = Tcl_NewLongObj((long)val1);
		myobjv[1] = Tcl_NewLongObj((long)val2);
		res = Tcl_NewListObj(myobjc, myobjv);
		Tcl_SetObjResult(interp, res);
	}
	return (result);
}

/*
 * tcl_RepGetConfig --
 *
 * PUBLIC: int tcl_RepGetConfig
 * PUBLIC:     __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
 */
int
tcl_RepGetConfig(interp, dbenv, which)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Environment pointer */
	Tcl_Obj *which;			/* which flag */
{
	Tcl_Obj *res;
	int on, optindex, result, ret;
	u_int32_t wh;

	if (Tcl_GetIndexFromObjStruct(interp, which,
	    &rep_config_types[0].name, sizeof(NAMEMAP),
	    "config type", TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(which));
	wh = rep_config_types[optindex].value;

	ret = dbenv->rep_get_config(dbenv, wh, &on);
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env rep_config")) == TCL_OK) {
		res = Tcl_NewIntObj(on);
		Tcl_SetObjResult(interp, res);
	}
	return (result);
}

/*
 * tcl_RepGetTimeout --
 *	Get various replication timeout values.
 *
 * PUBLIC: int tcl_RepGetTimeout
 * PUBLIC:     __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *));
 */
int
tcl_RepGetTimeout(interp, dbenv, which)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Environment pointer */
	Tcl_Obj *which;			/* which flag */
{
	Tcl_Obj *res;
	int optindex, result, ret, wh;
	u_int32_t to;

	if (Tcl_GetIndexFromObjStruct(interp, which,
		&rep_timeout_types[0].name, sizeof(NAMEMAP),
		"timeout type", TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(which));
	wh = (int)rep_timeout_types[optindex].value;
	ret = dbenv->rep_get_timeout(dbenv, wh, &to);
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env rep_get_timeout")) == TCL_OK) {
		res = Tcl_NewLongObj((long)to);
		Tcl_SetObjResult(interp, res);
	}
	return (result);
}

/*
 * tcl_RepGetAckPolicy
 *	Get Replication Manager acknowledgement policy 
 *
 * PUBLIC: int tcl_RepGetAckPolicy
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepGetAckPolicy(interp, objc, objv, dbenv)
	Tcl_Interp *interp;
	int objc;
	Tcl_Obj *CONST objv[];
	DB_ENV *dbenv;
{
	const char *name;
	int policy, ret;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "");
		return (TCL_ERROR);
	}

	if ((ret = dbenv->repmgr_get_ack_policy(dbenv, &policy)) != 0 ||
	    (ret = tcl_RepNumberToName(rep_ack_policies,
	    (u_int32_t)policy, &name)) != 0)
		return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			"env repmgr_get_ack_policy"));

	Tcl_SetObjResult(interp, NewStringObj(name, strlen(name)));
	return (TCL_OK);
}

/*
 * tcl_RepGetLocalSite
 *	Get local site address.
 *
 * PUBLIC: int tcl_RepGetLocalSite
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepGetLocalSite(interp, objc, objv, dbenv)
	Tcl_Interp *interp;
	int objc;
	Tcl_Obj *CONST objv[];
	DB_ENV *dbenv;
{
	Tcl_Obj *myobjv[2];
	DB_SITE *dbsite;
	const char *host;
	int ret, t_ret;
	u_int port;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "");
		return (TCL_ERROR);
	}

	if ((ret = dbenv->repmgr_local_site(dbenv, &dbsite)) == 0) {
		if ((ret = dbsite->get_address(dbsite, &host, &port)) == 0) {
			myobjv[0] = NewStringObj(host, strlen(host));
			myobjv[1] = Tcl_NewIntObj((int)port);
			Tcl_SetObjResult(interp, Tcl_NewListObj(2, myobjv));
		}
		if ((t_ret = dbsite->close(dbsite)) != 0 && ret == 0)
			ret = t_ret;
	}
	return (ret == 0 ? TCL_OK :
	    _ReturnSetup(interp, ret,
	    DB_RETOK_REPMGR_LOCALSITE(ret), "env repmgr_local_site"));
}




/*
 * tcl_RepNumberToName
 *	Map a #define'd int value to the corresponding name.
 */ 
static int
tcl_RepNumberToName(map, value, namep)
	const NAMEMAP *map;
	u_int32_t value;
	const char **namep;
{
	while (map->name) {
		if (map->value == value) {
			*namep = map->name;
			return (0);
		}
		map++;
	}
	return (DB_NOTFOUND);
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepElect --
 *	Call DB_ENV->rep_elect().
 *
 * PUBLIC: int tcl_RepElect
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepElect(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	int result, ret;
	u_int32_t full_timeout, nsites, nvotes, pri, timeout;

	if (objc != 6 && objc != 7) {
		Tcl_WrongNumArgs(interp, 2, objv,
		    "nsites nvotes pri timeout [full_timeout]");
		return (TCL_ERROR);
	}

	if ((result = _GetUInt32(interp, objv[2], &nsites)) != TCL_OK)
		return (result);
	if ((result = _GetUInt32(interp, objv[3], &nvotes)) != TCL_OK)
		return (result);
	if ((result = _GetUInt32(interp, objv[4], &pri)) != TCL_OK)
		return (result);
	if ((result = _GetUInt32(interp, objv[5], &timeout)) != TCL_OK)
		return (result);
	full_timeout = 0;
	if (objc == 7)
		if ((result = _GetUInt32(interp, objv[6], &full_timeout))
		    != TCL_OK)
			return (result);

	_debug_check();

	if ((ret = dbenv->rep_set_priority(dbenv, pri)) != 0)
		return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "env rep_elect (rep_set_priority)"));
	if ((ret = dbenv->rep_set_timeout(dbenv, DB_REP_ELECTION_TIMEOUT,
	    timeout)) != 0)
		return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "env rep_elect (rep_set_timeout)"));

	if (full_timeout != 0 && (ret = dbenv->rep_set_timeout(dbenv,
	    DB_REP_FULL_ELECTION_TIMEOUT, full_timeout)) != 0)
		return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "env rep_elect (rep_set_timeout)"));

	ret = dbenv->rep_elect(dbenv, nsites, nvotes, 0);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env rep_elect"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepFlush --
 *	Call DB_ENV->rep_flush().
 *
 * PUBLIC: int tcl_RepFlush
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepFlush(interp, objc, objv, dbenv)
	Tcl_Interp *interp;
	int objc;
	Tcl_Obj *CONST objv[];
	DB_ENV *dbenv;
{
	int ret;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "");
		return TCL_ERROR;
	}

	_debug_check();
	ret = dbenv->rep_flush(dbenv);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env rep_flush"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepSync --
 *	Call DB_ENV->rep_sync().
 *
 * PUBLIC: int tcl_RepSync
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepSync(interp, objc, objv, dbenv)
	Tcl_Interp *interp;
	int objc;
	Tcl_Obj *CONST objv[];
	DB_ENV *dbenv;
{
	int ret;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "");
		return TCL_ERROR;
	}

	_debug_check();
	ret = dbenv->rep_sync(dbenv, 0);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env rep_sync"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepLease --
 *	Call DB_ENV->rep_set_lease().
 *
 * PUBLIC: int tcl_RepLease  __P((Tcl_Interp *, int, Tcl_Obj * CONST *,
 * PUBLIC:    DB_ENV *));
 */
int
tcl_RepLease(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	u_int32_t clock_fast, clock_slow, timeout;
	int result, ret;

	COMPQUIET(clock_fast, 0);
	COMPQUIET(clock_slow, 0);

	if (objc != 3 && objc != 1) {
		Tcl_WrongNumArgs(interp, 1, objv, "{timeout fast slow}");
		return (TCL_ERROR);
	}

	if ((result = _GetUInt32(interp, objv[0], &timeout)) != TCL_OK)
		return (result);
	if (objc == 3) {
		if ((result = _GetUInt32(interp, objv[1], &clock_fast))
		    != TCL_OK)
			return (result);
		if ((result = _GetUInt32(interp, objv[2], &clock_slow))
		    != TCL_OK)
			return (result);
	}
	ret = dbenv->rep_set_timeout(dbenv, DB_REP_LEASE_TIMEOUT,
	    (db_timeout_t)timeout);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "rep_set_timeout");
	ret = dbenv->rep_set_config(dbenv, DB_REP_CONF_LEASE, 1);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "rep_set_config");
	if (result != TCL_OK)
		return (result);
	if (objc == 3)
		ret = dbenv->rep_set_clockskew(dbenv, clock_fast, clock_slow);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env rep_set_lease"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepInmemFiles --
 *	Set in-memory replication, which must be done before opening
 *	environment.
 *
 * PUBLIC: int tcl_RepInmemFiles  __P((Tcl_Interp *, DB_ENV *));
 */
int
tcl_RepInmemFiles(interp, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;
{
	int ret;

	ret = dbenv->rep_set_config(dbenv, DB_REP_CONF_INMEM, 1);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "rep_set_config"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepLimit --
 *	Call DB_ENV->rep_set_limit().
 *
 * PUBLIC: int tcl_RepLimit
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepLimit(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	int result, ret;
	u_int32_t bytes, gbytes;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "gbytes bytes");
		return (TCL_ERROR);
	}

	if ((result = _GetUInt32(interp, objv[0], &gbytes)) != TCL_OK)
		return (result);
	if ((result = _GetUInt32(interp, objv[1], &bytes)) != TCL_OK)
		return (result);

	_debug_check();
	if ((ret = dbenv->rep_set_limit(dbenv, gbytes, bytes)) != 0)
		return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env set_rep_limit"));

	return (_ReturnSetup(interp,
	    ret, DB_RETOK_STD(ret), "env set_rep_limit"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepNSites --
 *	Call DB_ENV->rep_set_nsites().
 *
 * PUBLIC: int tcl_RepNSites
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepNSites(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	int result, ret;
	u_int32_t nsites;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "nsites");
		return (TCL_ERROR);
	}

	if ((result = _GetUInt32(interp, objv[2], &nsites)) != TCL_OK)
		return (result);

	_debug_check();
	ret = dbenv->rep_set_nsites(dbenv, nsites);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env rep_nsites"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepRequest --
 *	Call DB_ENV->rep_set_request().
 *
 * PUBLIC: int tcl_RepRequest
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepRequest(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	int result, ret;
	long min, max;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "min max");
		return (TCL_ERROR);
	}

	if ((result = Tcl_GetLongFromObj(interp, objv[0], &min)) != TCL_OK)
		return (result);
	if ((result = Tcl_GetLongFromObj(interp, objv[1], &max)) != TCL_OK)
		return (result);

	_debug_check();
	if ((ret = dbenv->rep_set_request(dbenv, (db_timeout_t)min,
	    (db_timeout_t)max)) != 0)
		return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env rep_request"));

	return (_ReturnSetup(interp,
	    ret, DB_RETOK_STD(ret), "env rep_request"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepNoarchiveTimeout --
 *	Reset the master update timer, to allow immediate log archiving.
 *
 * PUBLIC: int tcl_RepNoarchiveTimeout
 * PUBLIC:     __P((Tcl_Interp *, DB_ENV *));
 */
int
tcl_RepNoarchiveTimeout(interp, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Environment pointer */
{
	ENV *env;
	REGENV *renv;
	REGINFO *infop;

	env = dbenv->env;

	_debug_check();
	infop = env->reginfo;
	renv = infop->primary;
	REP_SYSTEM_LOCK(env);
	F_CLR(renv, DB_REGENV_REPLOCKED);
	renv->op_timestamp = 0;
	REP_SYSTEM_UNLOCK(env);

	return (_ReturnSetup(interp,
	    0, DB_RETOK_STD(0), "env test force noarchive_timeout"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepTransport --
 *	Call DB_ENV->rep_set_transport().
 *
 * PUBLIC: int tcl_RepTransport  __P((Tcl_Interp *, int, Tcl_Obj * CONST *,
 * PUBLIC:    DB_ENV *, DBTCL_INFO *));
 *
 *	Note that this normally can/should be achieved as an argument to
 * berkdb env, but we need to test changing the transport function on
 * the fly.
 */
int
tcl_RepTransport(interp, objc, objv, dbenv, ip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
	DBTCL_INFO *ip;
{
	int intarg, result, ret;

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "{id transport_func}");
		return (TCL_ERROR);
	}

	/*
	 * Store the objects containing the machine ID
	 * and the procedure name.  We don't need to crack
	 * the send procedure out now, but we do convert the
	 * machine ID to an int, since rep_set_transport needs
	 * it.  Even so, it'll be easier later to deal with
	 * the Tcl_Obj *, so we save that, not the int.
	 *
	 * Note that we Tcl_IncrRefCount both objects
	 * independently;  Tcl is free to discard the list
	 * that they're bundled into.
	 */

	/*
	 * Check that the machine ID is an int.  Note that
	 * we do want to use GetIntFromObj;  the machine
	 * ID is explicitly an int, not a u_int32_t.
	 */
	if (ip->i_rep_eid != NULL) {
		Tcl_DecrRefCount(ip->i_rep_eid);
	}
	ip->i_rep_eid = objv[0];
	Tcl_IncrRefCount(ip->i_rep_eid);
	result = Tcl_GetIntFromObj(interp,
	    ip->i_rep_eid, &intarg);
	if (result != TCL_OK)
		return (result);

	if (ip->i_rep_send != NULL) {
		Tcl_DecrRefCount(ip->i_rep_send);
	}
	ip->i_rep_send = objv[1];
	Tcl_IncrRefCount(ip->i_rep_send);
	_debug_check();
	ret = dbenv->rep_set_transport(dbenv, intarg, tcl_rep_send);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env rep_transport"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepStart --
 *	Call DB_ENV->rep_start().
 *
 * PUBLIC: int tcl_RepStart
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 *
 *	Note that this normally can/should be achieved as an argument to
 * berkdb env, but we need to test forcible upgrading of clients, which
 * involves calling this on an open environment handle.
 */
int
tcl_RepStart(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	static const char *tclrpstrt[] = {
		"-client",
		"-master",
		NULL
	};
	enum tclrpstrt {
		TCL_RPSTRT_CLIENT,
		TCL_RPSTRT_MASTER
	};
	char *arg;
	int i, optindex, ret;
	u_int32_t flag;

	flag = 0;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "[-master/-client]");
		return (TCL_ERROR);
	}

	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], tclrpstrt,
		    "option", TCL_EXACT, &optindex) != TCL_OK) {
			arg = Tcl_GetStringFromObj(objv[i], NULL);
			if (arg[0] == '-')
				return (IS_HELP(objv[i]));
			else
				Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum tclrpstrt)optindex) {
		case TCL_RPSTRT_CLIENT:
			flag = DB_REP_CLIENT;
			break;
		case TCL_RPSTRT_MASTER:
			flag = DB_REP_MASTER;
			break;
		}
	}

	_debug_check();
	ret = dbenv->rep_start(dbenv, NULL, flag);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env rep_start"));
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepProcessMessage --
 *	Call DB_ENV->rep_process_message().
 *
 * PUBLIC: int tcl_RepProcessMessage
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepProcessMessage(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	DBT control, rec;
	DB_LSN permlsn;
	Tcl_Obj *lsnlist, *myobjv[2], *res;
	void *ctmp, *rtmp;
	char *msg;
	int eid;
	int freectl, freerec, myobjc, result, ret;

	if (objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv, "id control rec");
		return (TCL_ERROR);
	}
	freectl = freerec = 0;

	memset(&control, 0, sizeof(control));
	memset(&rec, 0, sizeof(rec));

	if ((result = Tcl_GetIntFromObj(interp, objv[2], &eid)) != TCL_OK)
		return (result);

	ret = _CopyObjBytes(interp, objv[3], &ctmp,
	    &control.size, &freectl);
	if (ret != 0) {
		result = _ReturnSetup(interp, ret,
		    DB_RETOK_REPPMSG(ret), "rep_proc_msg");
		return (result);
	}
	control.data = ctmp;
	ret = _CopyObjBytes(interp, objv[4], &rtmp,
	    &rec.size, &freerec);
	if (ret != 0) {
		result = _ReturnSetup(interp, ret,
		    DB_RETOK_REPPMSG(ret), "rep_proc_msg");
		goto out;
	}
	rec.data = rtmp;
	_debug_check();
	ret = dbenv->rep_process_message(dbenv, &control, &rec, eid, &permlsn);
	/*
	 * !!!
	 * The TCL API diverges from the C++/Java APIs here.  For us, it
	 * is OK to get DUPMASTER and HOLDELECTION for testing purposes.
	 */
	result = _ReturnSetup(interp, ret,
	    DB_RETOK_REPPMSG(ret) || ret == DB_REP_DUPMASTER ||
	    ret == DB_REP_HOLDELECTION,
	    "env rep_process_message");

	if (result != TCL_OK)
		goto out;

	/*
	 * We have a valid return.  We need to return a variety of information.
	 * It will be one of the following:
	 * {0 0} -  Make a 0 return a list for consistent return structure.
	 * {DUPMASTER 0} -  DUPMASTER, no other info needed.
	 * {HOLDELECTION 0} -  HOLDELECTION, no other info needed.
	 * {NEWMASTER #} - NEWMASTER and its ID.
	 * {NEWSITE 0} - NEWSITE, no other info needed.
	 * {IGNORE {LSN list}} - IGNORE and this msg's LSN.
	 * {ISPERM {LSN list}} - ISPERM and the perm LSN.
	 * {NOTPERM {LSN list}} - NOTPERM and this msg's LSN.
	 */
	myobjc = 2;
	switch (ret) {
	case 0:
		myobjv[0] = Tcl_NewIntObj(0);
		myobjv[1] = Tcl_NewIntObj(0);
		break;
	case DB_REP_DUPMASTER:
		myobjv[0] = Tcl_NewByteArrayObj(
		    (u_char *)"DUPMASTER", (int)strlen("DUPMASTER"));
		myobjv[1] = Tcl_NewIntObj(0);
		break;
	case DB_REP_HOLDELECTION:
		myobjv[0] = Tcl_NewByteArrayObj(
		    (u_char *)"HOLDELECTION", (int)strlen("HOLDELECTION"));
		myobjv[1] = Tcl_NewIntObj(0);
		break;
	case DB_REP_IGNORE:
		myobjv[0] = Tcl_NewLongObj((long)permlsn.file);
		myobjv[1] = Tcl_NewLongObj((long)permlsn.offset);
		lsnlist = Tcl_NewListObj(myobjc, myobjv);
		myobjv[0] = Tcl_NewByteArrayObj(
		    (u_char *)"IGNORE", (int)strlen("IGNORE"));
		myobjv[1] = lsnlist;
		break;
	case DB_REP_ISPERM:
		myobjv[0] = Tcl_NewLongObj((long)permlsn.file);
		myobjv[1] = Tcl_NewLongObj((long)permlsn.offset);
		lsnlist = Tcl_NewListObj(myobjc, myobjv);
		myobjv[0] = Tcl_NewByteArrayObj(
		    (u_char *)"ISPERM", (int)strlen("ISPERM"));
		myobjv[1] = lsnlist;
		break;
	case DB_REP_NEWSITE:
		myobjv[0] = Tcl_NewByteArrayObj(
		    (u_char *)"NEWSITE", (int)strlen("NEWSITE"));
		myobjv[1] = Tcl_NewIntObj(0);
		break;
	case DB_REP_NOTPERM:
		myobjv[0] = Tcl_NewLongObj((long)permlsn.file);
		myobjv[1] = Tcl_NewLongObj((long)permlsn.offset);
		lsnlist = Tcl_NewListObj(myobjc, myobjv);
		myobjv[0] = Tcl_NewByteArrayObj(
		    (u_char *)"NOTPERM", (int)strlen("NOTPERM"));
		myobjv[1] = lsnlist;
		break;
	default:
		msg = db_strerror(ret);
		Tcl_AppendResult(interp, msg, NULL);
		Tcl_SetErrorCode(interp, "BerkeleyDB", msg, NULL);
		result = TCL_ERROR;
		goto out;
	}
	res = Tcl_NewListObj(myobjc, myobjv);
	if (res != NULL)
		Tcl_SetObjResult(interp, res);
out:
	if (freectl)
		__os_free(NULL, ctmp);
	if (freerec)
		__os_free(NULL, rtmp);

	return (result);
}
#endif

#ifdef CONFIG_TEST
/*
 * tcl_RepStat --
 *	Call DB_ENV->rep_stat().
 *
 * PUBLIC: int tcl_RepStat
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepStat(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	DB_REP_STAT *sp;
	Tcl_Obj *myobjv[2], *res, *thislist, *lsnlist;
	u_int32_t flag;
	int myobjc, result, ret;
	char *arg, *role;

	flag = 0;
	result = TCL_OK;

	if (objc > 3) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return (TCL_ERROR);
	}
	if (objc == 3) {
		arg = Tcl_GetStringFromObj(objv[2], NULL);
		if (strcmp(arg, "-clear") == 0)
			flag = DB_STAT_CLEAR;
		else {
			Tcl_SetResult(interp,
			    "db stat: unknown arg", TCL_STATIC);
			return (TCL_ERROR);
		}
	}

	_debug_check();
	ret = dbenv->rep_stat(dbenv, &sp, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "rep stat");
	if (result == TCL_ERROR)
		return (result);

	/*
	 * Have our stats, now construct the name value
	 * list pairs and free up the memory.
	 */
	res = Tcl_NewObj();
	/*
	 * MAKE_STAT_* assumes 'res' and 'error' label.
	 */
	if (sp->st_status == DB_REP_MASTER)
		role = "master";
	else if (sp->st_status == DB_REP_CLIENT)
		role = "client";
	else
		role = "none";
	MAKE_STAT_STRLIST("Role", role);

	MAKE_STAT_LSN("Next LSN expected", &sp->st_next_lsn);
	MAKE_STAT_LSN("First missed LSN", &sp->st_waiting_lsn);
	MAKE_STAT_LSN("Maximum permanent LSN", &sp->st_max_perm_lsn);
	MAKE_WSTAT_LIST("Bulk buffer fills", sp->st_bulk_fills);
	MAKE_WSTAT_LIST("Bulk buffer overflows", sp->st_bulk_overflows);
	MAKE_WSTAT_LIST("Bulk records stored", sp->st_bulk_records);
	MAKE_WSTAT_LIST("Bulk buffer transfers", sp->st_bulk_transfers);
	MAKE_WSTAT_LIST("Client service requests", sp->st_client_svc_req);
	MAKE_WSTAT_LIST("Client service req misses", sp->st_client_svc_miss);
	MAKE_WSTAT_LIST("Client rerequests", sp->st_client_rerequests);
	MAKE_STAT_LIST("Duplicate master conditions", sp->st_dupmasters);
	MAKE_STAT_LIST("Environment ID", sp->st_env_id);
	MAKE_STAT_LIST("Environment priority", sp->st_env_priority);
	MAKE_STAT_LIST("Generation number", sp->st_gen);
	MAKE_STAT_LIST("Election generation number", sp->st_egen);
	MAKE_STAT_LIST("Startup complete", sp->st_startup_complete);
	MAKE_STAT_LIST("Is view", sp->st_view);
	MAKE_WSTAT_LIST("Lease messages sent", sp->st_lease_sends);
	MAKE_WSTAT_LIST("Lease checks", sp->st_lease_chk);
	MAKE_WSTAT_LIST("Lease check invalid", sp->st_lease_chk_misses);
	MAKE_WSTAT_LIST("Lease check refresh", sp->st_lease_chk_refresh);
	MAKE_WSTAT_LIST("Duplicate log records received", sp->st_log_duplicated);
	MAKE_WSTAT_LIST("Current log records queued", sp->st_log_queued);
	MAKE_WSTAT_LIST("Maximum log records queued", sp->st_log_queued_max);
	MAKE_WSTAT_LIST("Total log records queued", sp->st_log_queued_total);
	MAKE_WSTAT_LIST("Log records received", sp->st_log_records);
	MAKE_WSTAT_LIST("Log records requested", sp->st_log_requested);
	MAKE_STAT_LIST("Master environment ID", sp->st_master);
	MAKE_WSTAT_LIST("Master changes", sp->st_master_changes);
	MAKE_STAT_LIST("Messages with bad generation number",
	    sp->st_msgs_badgen);
	MAKE_WSTAT_LIST("Messages processed", sp->st_msgs_processed);
	MAKE_WSTAT_LIST("Messages ignored for recovery", sp->st_msgs_recover);
	MAKE_WSTAT_LIST("Message send failures", sp->st_msgs_send_failures);
	MAKE_WSTAT_LIST("Messages sent", sp->st_msgs_sent);
	MAKE_WSTAT_LIST("New site messages", sp->st_newsites);
	MAKE_STAT_LIST("Number of sites in replication group", sp->st_nsites);
	MAKE_WSTAT_LIST("Transmission limited", sp->st_nthrottles);
	MAKE_WSTAT_LIST("Outdated conditions", sp->st_outdated);
	MAKE_WSTAT_LIST("Transactions applied", sp->st_txns_applied);
	MAKE_STAT_LIST("Next page expected", sp->st_next_pg);
	MAKE_WSTAT_LIST("First missed page", sp->st_waiting_pg);
	MAKE_WSTAT_LIST("Duplicate pages received", sp->st_pg_duplicated);
	MAKE_WSTAT_LIST("Pages received", sp->st_pg_records);
	MAKE_WSTAT_LIST("Pages requested", sp->st_pg_requested);
	MAKE_WSTAT_LIST("Elections held", sp->st_elections);
	MAKE_WSTAT_LIST("Elections won", sp->st_elections_won);
	MAKE_STAT_LIST("Election phase", sp->st_election_status);
	MAKE_STAT_LIST("Election winner", sp->st_election_cur_winner);
	MAKE_STAT_LIST("Election winner generation number",
	    sp->st_election_gen);
	MAKE_STAT_LIST("Election data generation number", 
	    sp->st_election_datagen);
	MAKE_STAT_LSN("Election max LSN", &sp->st_election_lsn);
	MAKE_STAT_LIST("Election sites", sp->st_election_nsites);
	MAKE_STAT_LIST("Election nvotes", sp->st_election_nvotes);
	MAKE_STAT_LIST("Election priority", sp->st_election_priority);
	MAKE_STAT_LIST("Election tiebreaker", sp->st_election_tiebreaker);
	MAKE_STAT_LIST("Election votes", sp->st_election_votes);
	MAKE_STAT_LIST("Election seconds", sp->st_election_sec);
	MAKE_STAT_LIST("Election usecs", sp->st_election_usec);
	MAKE_STAT_LIST("Start-sync operations delayed",
	    sp->st_startsync_delayed);
	MAKE_STAT_LIST("Maximum lease seconds", sp->st_max_lease_sec);
	MAKE_STAT_LIST("Maximum lease usecs", sp->st_max_lease_usec);
	MAKE_STAT_LIST("File fail cleanups done", sp->st_filefail_cleanups);
	MAKE_WSTAT_LIST("Future duplicated log records", sp->st_log_futuredup);

	Tcl_SetObjResult(interp, res);
error:
	__os_ufree(dbenv->env, sp);
	return (result);
}

/*
 * tcl_RepStatPrint --
 *
 * PUBLIC: int tcl_RepStatPrint __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_RepStatPrint(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{	
	static const char *repstatprtopts[] = {
		"-all",
		"-clear",
		 NULL
	};
	enum repstatprtopts {
		REPSTATPRTALL,
		REPSTATPRTCLEAR
	};
	u_int32_t flag;
	int i, optindex, result, ret;

	result = TCL_OK;
	flag = 0;
	i = 2;

	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], repstatprtopts, 
		    "option", TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			goto error;
		}
		i++;
		switch ((enum repstatprtopts)optindex) {
		case REPSTATPRTALL:
			flag |= DB_STAT_ALL;
			break;
		case REPSTATPRTCLEAR:
			flag |= DB_STAT_CLEAR;
			break;
		}
		if (result != TCL_OK)
			break;
	}
	if (result != TCL_OK)
		goto error;

	_debug_check();
	ret = dbenv->rep_stat_print(dbenv, flag);
	result = _ReturnSetup(interp, 
	    ret, DB_RETOK_STD(ret), "dbenv rep_stat_print");
error:
	return (result);

}

/*
 * tcl_RepMgr --
 *	Configure and start the Replication Manager.
 *
 * PUBLIC: int tcl_RepMgr
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepMgr(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{
	static const char *rmgr[] = {
		"-ack",
		"-inqueue",
		"-local",
		"-msgth",
		"-pri",
		"-remote",
		"-remove",
		"-start",
		"-timeout",
		NULL
	};
	enum rmgr {
		RMGR_ACK,
		RMGR_INQUEUE,
		RMGR_LOCAL,
		RMGR_MSGTH,
		RMGR_PRI,
		RMGR_REMOTE,
		RMGR_REMOVE,
		RMGR_START,
		RMGR_TIMEOUT
	};
	Tcl_Obj **myobjv;
	DB_SITE *dbsite;
	long to;
	int ack, creator, i, j, legacy, myobjc, optindex;
	int peer, result, ret, totype, t_ret;
	u_int32_t msgth, start_flag, uintarg, uintarg2;
	char *arg;

	result = TCL_OK;
	ack = ret = totype = 0;
	msgth = 1;
	start_flag = 0;

	if (objc <= 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?args?");
		return (TCL_ERROR);
	}
	/*
	 * Get the command name index from the object based on the bdbcmds
	 * defined above.
	 */
	i = 2;
	while (i < objc) {
		Tcl_ResetResult(interp);
		if (Tcl_GetIndexFromObj(interp, objv[i], rmgr, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			goto error;
		}
		i++;
		switch ((enum rmgr)optindex) {
		case RMGR_ACK:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-ack policy?");
				result = TCL_ERROR;
				break;
			}


			if (Tcl_GetIndexFromObjStruct(interp, objv[i++],
				&rep_ack_policies[0].name, sizeof(NAMEMAP),
				"ack policy", TCL_EXACT, &optindex) != TCL_OK) {
				result = TCL_ERROR;
				break;
			}
			ack = (int)rep_ack_policies[optindex].value;

			_debug_check();
			ret = dbenv->repmgr_set_ack_policy(dbenv, ack);
			result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "ack");
			break;
		case RMGR_INQUEUE:
			result = Tcl_ListObjGetElements(interp, objv[i],
			    &myobjc, &myobjv);
			if (result == TCL_OK)
				i++;
			else
				break;
			if (myobjc != 2) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-inqueue {msgs bulkmsgs}?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(myobjv[0], NULL);
			if ((result = _GetUInt32(interp, myobjv[0], &uintarg))
			    != TCL_OK)
				break;
			if ((result = _GetUInt32(interp, myobjv[1], &uintarg2))
			    != TCL_OK)
				break;
			_debug_check();
			ret = dbenv->repmgr_set_incoming_queue_max(dbenv,
			    uintarg, uintarg2);
			result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "repmgr_set_incoming_queue_max");
			break;
		case RMGR_LOCAL:
			result = Tcl_ListObjGetElements(interp, objv[i],
			    &myobjc, &myobjv);
			if (result == TCL_OK)
				i++;
			else
				break;
			if (myobjc < 2 || myobjc > 4) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-local {host port [creator][legacy]}?");
				result = TCL_ERROR;
				break;
			}
			creator = legacy = 0;
			for (j = 2; j < myobjc; j++) {
				arg = Tcl_GetString(myobjv[j]);
				if (strcmp(arg, "creator") == 0)
					creator = 1;
				else if (strcmp(arg, "legacy") == 0)
					legacy = 1;
				else {
					Tcl_AddErrorInfo(interp,
					    "local: illegal flag");
					result = TCL_ERROR;
					break;
				}
			}
			/* The "arg" is host name, and "uintarg" is port. */
			arg = Tcl_GetStringFromObj(myobjv[0], NULL);
			if ((result = _GetUInt32(interp, myobjv[1], &uintarg))
			    != TCL_OK)
				break;
			_debug_check();
			if ((ret = dbenv->repmgr_site(dbenv,
			    arg, uintarg, &dbsite, 0)) == 0) {
				ret = dbsite->set_config(dbsite,
				    DB_LOCAL_SITE, 1);
				if (ret == 0 && creator)
					ret = dbsite->set_config(dbsite,
					    DB_GROUP_CREATOR, 1);
				if (ret == 0 && legacy)
					ret = dbsite->set_config(dbsite,
					    DB_LEGACY, 1);
				if ((t_ret = dbsite->close(dbsite)) != 0 &&
				    ret == 0)
					ret = t_ret;
			}
			result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "repmgr_site");
			break;
		case RMGR_MSGTH:
			if (i >= objc) {
				Tcl_WrongNumArgs(
				    interp, 2, objv, "?-msgth nth?");
				result = TCL_ERROR;
				break;
			}
			result = _GetUInt32(interp, objv[i++], &msgth);
			break;
		case RMGR_PRI:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-pri priority?");
				result = TCL_ERROR;
				break;
			}
			result = _GetUInt32(interp, objv[i++], &uintarg);
			if (result == TCL_OK) {
				_debug_check();
				ret = dbenv->
				    rep_set_priority(dbenv, uintarg);
			}
			result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "rep_set_priority");
			break;
		case RMGR_REMOTE:
			result = Tcl_ListObjGetElements(interp, objv[i],
			    &myobjc, &myobjv);
			if (result == TCL_OK)
				i++;
			else
				break;
			if (myobjc < 2 || myobjc > 4) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-remote {host port [peer][legacy]}?");
				result = TCL_ERROR;
				break;
			}

			/*
			 * Get the flags first so we can reuse 'arg'.
			 */
			legacy = peer = 0;
			for (j = 2; j < myobjc; j++) {
				arg = Tcl_GetString(myobjv[j]);
				if (strcmp(arg, "legacy") == 0)
					legacy = 1;
				else if (strcmp(arg, "peer") == 0)
					peer = 1;
				else {
					Tcl_AddErrorInfo(interp,
					    "remote: illegal flag");
					result = TCL_ERROR;
					break;
				}
			}
			arg = Tcl_GetString(myobjv[0]);
			if ((result = _GetUInt32(interp, myobjv[1], &uintarg))
			    != TCL_OK)
				break;
			_debug_check();

			/* For now, all "-remote" sites become helpers. */
			if ((ret = dbenv->repmgr_site(dbenv,
			    arg, uintarg, &dbsite, 0)) == 0) {
				ret = dbsite->set_config(dbsite,
				    DB_BOOTSTRAP_HELPER, 1);
				if (ret == 0 && legacy)
					ret = dbsite->set_config(dbsite,
					    DB_LEGACY, 1);
				if (ret == 0 && peer)
					ret = dbsite->set_config(dbsite,
					    DB_REPMGR_PEER, 1);
				if ((t_ret = dbsite->close(dbsite)) != 0 &&
				    ret == 0)
					ret = t_ret;
			}
			result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "repmgr_site");
			break;
		case RMGR_REMOVE:
			result = Tcl_ListObjGetElements(interp, objv[i],
			    &myobjc, &myobjv);
			if (result == TCL_OK)
				i++;
			else
				break;
			if (myobjc != 2) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-remove {host port}?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetString(myobjv[0]);
			if ((result = _GetUInt32(interp, myobjv[1], &uintarg))
			    != TCL_OK)
				break;
			_debug_check();
			if ((ret = dbenv->repmgr_site(dbenv,
			    arg, uintarg, &dbsite, 0)) == 0)
				ret = dbsite->remove(dbsite);
			result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "repmgr_site->remove");
			break;
		case RMGR_START:
			if (i >= objc) {
				Tcl_WrongNumArgs(
				    interp, 2, objv, "?-start state?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			if (strcmp(arg, "master") == 0)
				start_flag = DB_REP_MASTER;
			else if (strcmp(arg, "client") == 0)
				start_flag = DB_REP_CLIENT;
			else if (strcmp(arg, "elect") == 0)
				start_flag = DB_REP_ELECTION;
			else {
				Tcl_AddErrorInfo(
				    interp, "start: illegal state");
				result = TCL_ERROR;
				break;
			}
			/*
			 * Some config functions need to be called
			 * before repmgr_start.  So finish parsing all
			 * the args and call repmgr_start at the end.
			 */
			break;
		case RMGR_TIMEOUT:
			result = Tcl_ListObjGetElements(interp, objv[i],
			    &myobjc, &myobjv);
			if (result == TCL_OK)
				i++;
			else
				break;
			if (myobjc != 2) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-timeout {type to}?");
				result = TCL_ERROR;
				break;
			}
			if (Tcl_GetIndexFromObjStruct(interp, myobjv[0],
				&rep_timeout_types[0].name, sizeof(NAMEMAP),
				"timeout type", TCL_EXACT, &optindex) != TCL_OK) {
				result = TCL_ERROR;
				break;
			}
			totype = (int)rep_timeout_types[optindex].value;
			if ((result = Tcl_GetLongFromObj(
			   interp, myobjv[1], &to)) != TCL_OK)
				break;
			_debug_check();
			ret = dbenv->rep_set_timeout(dbenv, totype,
			    (db_timeout_t)to);
			result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			    "rep_set_timeout");
			break;
		}
		/*
		 * If, at any time, parsing the args we get an error,
		 * bail out and return.
		 */
		if (result != TCL_OK)
			goto error;
	}
	/*
	 * Only call repmgr_start if needed.  The user may use this
	 * call just to reconfigure, change policy, etc.
	 */
	if (start_flag != 0 && result == TCL_OK) {
		_debug_check();
		ret = dbenv->repmgr_start(dbenv, (int)msgth, start_flag);
		result = _ReturnSetup(
		    interp, ret, DB_RETOK_REPMGR_START(ret), "repmgr_start");
	}
error:
	return (result);
}

/*
 * tcl_RepMgrSiteList --
 *	Call DB_ENV->repmgr_site_list().
 *
 * PUBLIC: int tcl_RepMgrSiteList
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepMgrSiteList(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	DB_REPMGR_SITE *sp;
	Tcl_Obj *myobjv[6], *res, *thislist;
	u_int count, i;
	char *pr, *st, *vw;
	int myobjc, result, ret;

	result = TCL_OK;

	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return (TCL_ERROR);
	}

	_debug_check();
	ret = dbenv->repmgr_site_list(dbenv, &count, &sp);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "repmgr sitelist");
	if (result == TCL_ERROR)
		return (result);

	/*
	 * Have our sites, now construct the {eid host port status peer}
	 * tuples and free up the memory.
	 */
	res = Tcl_NewObj();

	for (i = 0; i < count; ++i) {
		/*
		 * MAKE_SITE_LIST assumes 'res' and 'error' label.
		 */
		if (sp[i].status == DB_REPMGR_CONNECTED)
			st = "connected";
		else if (sp[i].status == DB_REPMGR_DISCONNECTED)
			st = "disconnected";
		else
			st = "unknown";
		pr = F_ISSET(&sp[i], DB_REPMGR_ISPEER) ? "peer" : "non-peer";
		vw = F_ISSET(&sp[i], DB_REPMGR_ISVIEW) ? "view" : "participant";
		MAKE_SITE_LIST(sp[i].eid, sp[i].host, sp[i].port, st, pr, vw);
	}

	Tcl_SetObjResult(interp, res);
error:
	__os_ufree(dbenv->env, sp);
	return (result);
}

/*
 * tcl_RepMgrStat --
 *	Call DB_ENV->repmgr_stat().
 *
 * PUBLIC: int tcl_RepMgrStat
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepMgrStat(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	DB_REPMGR_STAT *sp;
	Tcl_Obj *res;
	u_int32_t flag;
	int result, ret;
	char *arg;

	flag = 0;
	result = TCL_OK;

	if (objc > 3) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return (TCL_ERROR);
	}
	if (objc == 3) {
		arg = Tcl_GetStringFromObj(objv[2], NULL);
		if (strcmp(arg, "-clear") == 0)
			flag = DB_STAT_CLEAR;
		else {
			Tcl_SetResult(interp,
			    "db stat: unknown arg", TCL_STATIC);
			return (TCL_ERROR);
		}
	}

	_debug_check();
	ret = dbenv->repmgr_stat(dbenv, &sp, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "repmgr stat");
	if (result == TCL_ERROR)
		return (result);

	/*
	 * Have our stats, now construct the name value
	 * list pairs and free up the memory.
	 */
	res = Tcl_NewObj();
	/*
	 * MAKE_STAT_* assumes 'res' and 'error' label.
	 */
	MAKE_WSTAT_LIST("Acknowledgement failures", sp->st_perm_failed);
	MAKE_WSTAT_LIST("Messages delayed", sp->st_msgs_queued);
	MAKE_WSTAT_LIST("Messages discarded", sp->st_msgs_dropped);
	MAKE_WSTAT_LIST("Connections dropped", sp->st_connection_drop);
	MAKE_WSTAT_LIST("Failed re-connects", sp->st_connect_fail);
	MAKE_STAT_LIST("Election threads", sp->st_elect_threads);
	MAKE_STAT_LIST("Max elect threads", sp->st_max_elect_threads);
	MAKE_STAT_LIST("Total sites", sp->st_site_total);
	MAKE_STAT_LIST("View sites", sp->st_site_views);
	MAKE_STAT_LIST("Participant sites", sp->st_site_participants);
	MAKE_WSTAT_LIST("Automatic replication process takeovers",
	    sp->st_takeovers);
	MAKE_STAT_LIST("Incoming queue size", sp->st_incoming_queue_size);

	Tcl_SetObjResult(interp, res);
error:
	__os_ufree(dbenv->env, sp);
	return (result);
}

/*
 * tcl_RepMgrStatPrint --
 *
 * PUBLIC: int tcl_RepMgrStatPrint __P((Tcl_Interp *, int,
 * PUBLIC:     Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_RepMgrStatPrint(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{	
	static const char *repmgrstatprtopts[] = {
		"-all",
		"-clear",
		 NULL
	};
	enum repmgrstatprtopts {
		REPMGRSTATPRTALL,
		REPMGRSTATPRTCLEAR
	};
	u_int32_t flag;
	int i, optindex, result, ret;

	result = TCL_OK;
	flag = 0;
	i = 2;

	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], repmgrstatprtopts, 
		    "option", TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			goto error;
		}
		i++;
		switch ((enum repmgrstatprtopts)optindex) {
		case REPMGRSTATPRTALL:
			flag |= DB_STAT_ALL;
			break;
		case REPMGRSTATPRTCLEAR:
			flag |= DB_STAT_CLEAR;
			break;
		}
		if (result != TCL_OK)
			break;
	}
	if (result != TCL_OK)
		goto error;

	_debug_check();
	ret = dbenv->repmgr_stat_print(dbenv, flag);
	result = _ReturnSetup(interp, 
	    ret, DB_RETOK_STD(ret), "dbenv repmgr_stat_print");
error:
	return (result);

}

/*
 * tcl_RepApplied -
 *
 * PUBLIC: int tcl_RepApplied
 * PUBLIC:     __P((Tcl_Interp *, int, Tcl_Obj * CONST *, DB_ENV *));
 */
int
tcl_RepApplied(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	static const char *repapplied_option_names[] = {
		"-timeout",
		NULL
	};
	enum envinfo_options {
		REPAPPLIEDTIMEOUT
	};
	unsigned char *arg;
	char msg[MSG_SIZE];
	db_timeout_t timeout;
	int i, len, ptr, result, ret;

	if (objc != 3 && objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv,
		    "?-timeout t? token");
		return (TCL_ERROR);
	}
	timeout = 0;
	i = 2;
	if (objc == 5) {
		if (Tcl_GetIndexFromObj(interp, objv[i],
		    repapplied_option_names, "option", TCL_EXACT, &ptr)
		    != TCL_OK)
			return (IS_HELP(objv[i]));
		i++;
		switch ((enum envinfo_options)ptr) {
		case REPAPPLIEDTIMEOUT:
			result = _GetUInt32(interp, objv[i++], &timeout);
			if (result != TCL_OK)
				return (result);
			break;
		}
	}

	arg = Tcl_GetByteArrayFromObj(objv[i], &len);
	if (len != DB_TXN_TOKEN_SIZE) {
		Tcl_SetErrorCode(interp, "BerkeleyDB",
		    "Commit token is the wrong size", NULL);

		snprintf(msg, MSG_SIZE,
		    "Bad commit token size %lu, should be %lu",
		    (u_long)len, (u_long)DB_TXN_TOKEN_SIZE);
		Tcl_SetResult(interp, msg, TCL_VOLATILE);
		return (TCL_ERROR);
	}		

	_debug_check();
	ret = dbenv->txn_applied(dbenv, (DB_TXN_TOKEN*)arg, timeout, 0);
	result = _ReturnSetup(interp, ret , DB_RETOK_TXNAPPLIED(ret),
	    "txn_applied");
	return (result);
}
#endif
