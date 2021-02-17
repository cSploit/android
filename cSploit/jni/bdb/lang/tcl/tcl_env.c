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
#include "dbinc/lock.h"
#include "dbinc/txn.h"
#include "dbinc/tcl_db.h"

/*
 * Prototypes for procedures defined later in this file:
 */
static void _EnvInfoDelete __P((Tcl_Interp *, DBTCL_INFO *));
static int  env_DbRemove __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
static int  env_DbRename __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
static int  env_EventInfo __P((Tcl_Interp *,
	int, Tcl_Obj * CONST*, DB_ENV *, DBTCL_INFO *));
static int  env_GetFlags __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
static int  env_GetOpenFlag
		__P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
static int  env_GetLockDetect
		__P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
static int  env_GetTimeout __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
static int  env_GetVerbose __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));

/*
 * PUBLIC: int env_Cmd __P((ClientData, Tcl_Interp *, int, Tcl_Obj * CONST*));
 *
 * env_Cmd --
 *	Implements the "env" command.
 */
int
env_Cmd(clientData, interp, objc, objv)
	ClientData clientData;		/* Env handle */
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *envcmds[] = {
#ifdef CONFIG_TEST
		"attributes",
		"backup",
		"dbbackup",
		"errfile",
		"errpfx",
		"event_info",
		"failchk",
		"id_reset",
		"lock_detect",
		"lock_id",
		"lock_id_free",
		"lock_id_set",
		"lock_get",
		"lock_set_priority",
		"lock_stat",
		"lock_stat_print",
		"lock_timeout",
		"lock_vec",
		"log_archive",
		"log_compare",
		"log_config",
		"log_cursor",
		"log_file",
		"log_flush",
		"log_get",
		"log_get_config",
		"log_put",
		"log_stat",
		"log_stat_print",
		"log_verify",
		"lsn_reset",
		"mpool",
		"mpool_stat",
		"mpool_stat_print",
		"mpool_sync",
		"mpool_trickle",
		"msgfile",
		"mutex",
		"mutex_free",
		"mutex_get_align",
		"mutex_get_incr",
		"mutex_get_init",
		"mutex_get_max",
		"mutex_get_tas_spins",
		"mutex_lock",
		"mutex_set_tas_spins",
		"mutex_stat",
		"mutex_stat_print",
		"mutex_unlock",
		"rep_config",
		"rep_elect",
		"rep_flush",
		"rep_get_clockskew",
		"rep_get_config",
		"rep_get_limit",
		"rep_get_nsites",
		"rep_get_priority",
		"rep_get_request",
		"rep_get_timeout",
		"rep_lease",
		"rep_limit",
		"rep_nsites",
		"rep_process_message",
		"rep_request",
		"rep_start",
		"rep_stat",
		"rep_stat_print",
		"rep_sync",
		"rep_transport",
		"repmgr",
		"repmgr_get_ack_policy",
		"repmgr_get_inqueue_max",
		"repmgr_get_local_site",
		"repmgr_site_list",
		"repmgr_stat",
		"repmgr_stat_print",
		"set_flags",
		"set_lg_max",
		"stat_print",
		"test",
		"txn_applied",
		"txn_id_set",
		"txn_recover",
		"txn_stat",
		"txn_stat_print",
		"txn_timeout",
		"verbose",
#endif
		"cdsgroup",
		"close",
		"dbremove",
		"dbrename",
		"get_blob_dir",
		"get_blob_threshold",
		"get_cachesize",
		"get_cache_max",
		"get_create_dir",
		"get_data_dirs",
		"get_encrypt_flags",
		"get_errpfx",
		"get_flags",
		"get_home",
		"get_lg_bsize",
		"get_lg_dir",
		"get_lg_filemode",
		"get_lg_max",
		"get_lg_regionmax",
		"get_lk_detect",
		"get_lk_init_lockers",
		"get_lk_init_locks",
		"get_lk_init_logid",
		"get_lk_init_objects",
		"get_lk_init_thread",
		"get_lk_max_lockers",
		"get_lk_max_locks",
		"get_lk_max_objects",
		"get_lk_partitions",
		"get_lk_tablesize",
		"get_memory_max",
		"get_metadata_dir",
		"get_mp_max_openfd",
		"get_mp_max_write",
		"get_mp_mmapsize",
		"get_mp_mtxcount",
		"get_mp_pagesize",
		"get_mp_tablesize",
		"get_open_flags",
		"get_shm_key",
		"get_tas_spins",
		"get_thread_count",
		"get_timeout",
		"get_tmp_dir",
		"get_tx_init",
		"get_tx_max",
		"get_tx_timestamp",
		"get_verbose",
		"resize_cache",
		"set_blob_threshold",
		"set_data_dir",
		"set_maxcache",
		"txn",
		"txn_checkpoint",
		NULL
	};
	enum envcmds {
#ifdef CONFIG_TEST
		ENVATTR,
		ENVBACKUP,
		ENVDBBACKUP,
		ENVERRFILE,
		ENVERRPFX,
		ENVEVENTINFO,
		ENVFAILCHK,
		ENVIDRESET,
		ENVLKDETECT,
		ENVLKID,
		ENVLKFREEID,
		ENVLKSETID,
		ENVLKGET,
		ENVLKPRI,
		ENVLKSTAT,
		ENVLKSTATPRT,
		ENVLKTIMEOUT,
		ENVLKVEC,
		ENVLOGARCH,
		ENVLOGCMP,
		ENVLOGCONFIG,
		ENVLOGCURSOR,
		ENVLOGFILE,
		ENVLOGFLUSH,
		ENVLOGGET,
		ENVLOGGETCONFIG,
		ENVLOGPUT,
		ENVLOGSTAT,
		ENVLOGSTATPRT,
		ENVLOGVERIFY,
		ENVLSNRESET,
		ENVMP,
		ENVMPSTAT,
		ENVMPSTATPRT,
		ENVMPSYNC,
		ENVTRICKLE,
		ENVMSGFILE,
		ENVMUTEX,
		ENVMUTFREE,
		ENVMUTGETALIGN,
		ENVMUTGETINCR,
		ENVMUTGETINIT,
		ENVMUTGETMAX,
		ENVMUTGETTASSPINS,
		ENVMUTLOCK,
		ENVMUTSETTASSPINS,
		ENVMUTSTAT,
		ENVMUTSTATPRT,
		ENVMUTUNLOCK,
		ENVREPCONFIG,
		ENVREPELECT,
		ENVREPFLUSH,
		ENVREPGETCLOCKSKEW,
		ENVREPGETCONFIG,
		ENVREPGETLIMIT,
		ENVREPGETNSITES,
		ENVREPGETPRIORITY,
		ENVREPGETREQUEST,
		ENVREPGETTIMEOUT,
		ENVREPLEASE,
		ENVREPLIMIT,
		ENVREPNSITES,
		ENVREPPROCMESS,
		ENVREPREQUEST,
		ENVREPSTART,
		ENVREPSTAT,
		ENVREPSTATPRT,
		ENVREPSYNC,
		ENVREPTRANSPORT,
		ENVREPMGR,
		ENVREPMGRGETACK,
		ENVREPMGRGETINQUEUE,
		ENVREPMGRGETLOCAL,
		ENVREPMGRSITELIST,
		ENVREPMGRSTAT,
		ENVREPMGRSTATPRT,
		ENVSETFLAGS,
		ENVSETLOGMAX,
		ENVSTATPRT,
		ENVTEST,
		ENVTXNAPPLIED,
		ENVTXNSETID,
		ENVTXNRECOVER,
		ENVTXNSTAT,
		ENVTXNSTATPRT,
		ENVTXNTIMEOUT,
		ENVVERB,
#endif
		ENVCDSGROUP,
		ENVCLOSE,
		ENVDBREMOVE,
		ENVDBRENAME,
		ENVGETBLOBDIR,
		ENVGETBLOBTHRESHOLD,
		ENVGETCACHESIZE,
		ENVGETCACHEMAX,
		ENVGETCREATEDIR,
		ENVGETDATADIRS,
		ENVGETENCRYPTFLAGS,
		ENVGETERRPFX,
		ENVGETFLAGS,
		ENVGETHOME,
		ENVGETLGBSIZE,
		ENVGETLGDIR,
		ENVGETLGFILEMODE,
		ENVGETLGMAX,
		ENVGETLGREGIONMAX,
		ENVGETLKDETECT,
		ENVGETLKINITLOCKERS,
		ENVGETLKINITLOCKS,
		ENVGETLKINITLOGID,
		ENVGETLKINITOBJECTS,
		ENVGETLKINITTHREAD,
		ENVGETLKMAXLOCKERS,
		ENVGETLKMAXLOCKS,
		ENVGETLKMAXOBJECTS,
		ENVGETLKPARTITIONS,
		ENVGETLKTABLESIZE,
		ENVGETMEMORYMAX,
		ENVGETMETADATADIR,
		ENVGETMPMAXOPENFD,
		ENVGETMPMAXWRITE,
		ENVGETMPMMAPSIZE,
		ENVGETMPMTXCOUNT,
		ENVGETMPPAGESIZE,
		ENVGETMPTABLESIZE,
		ENVGETOPENFLAG,
		ENVGETSHMKEY,
		ENVGETTASSPINS,
		ENVGETTHREADCOUNT,
		ENVGETTIMEOUT,
		ENVGETTMPDIR,
		ENVGETTXINIT,
		ENVGETTXMAX,
		ENVGETTXTIMESTAMP,
		ENVGETVERBOSE,
		ENVRESIZECACHE,
		ENVSETBLOBTHRESHOLD,
		ENVSETDATADIR,
		ENVSETMAXCACHE,
		ENVTXN,
		ENVTXNCKP
	};
	DBTCL_INFO *envip;
	DB_ENV *dbenv;
	Tcl_Obj **listobjv, *myobjv[3], *res;
	db_timeout_t timeout;
	size_t size;
	time_t timeval;
	u_int32_t bytes, gbytes, value;
	long shm_key;
	int cmdindex, i, intvalue, listobjc, ncache, result, ret;
	const char *strval, **dirs;
	char *strarg, newname[MSG_SIZE];
#ifdef CONFIG_TEST
	DBTCL_INFO *logcip;
	DB_LOGC *logc;
	u_int32_t lockid;
	long newval, otherval;
#endif

	Tcl_ResetResult(interp);
	dbenv = (DB_ENV *)clientData;
	envip = _PtrToInfo((void *)dbenv);
	result = TCL_OK;
	memset(newname, 0, MSG_SIZE);

	if (objc <= 1) {
		Tcl_WrongNumArgs(interp, 1, objv, "command cmdargs");
		return (TCL_ERROR);
	}
	if (dbenv == NULL) {
		Tcl_SetResult(interp, "NULL env pointer", TCL_STATIC);
		return (TCL_ERROR);
	}
	if (envip == NULL) {
		Tcl_SetResult(interp, "NULL env info pointer", TCL_STATIC);
		return (TCL_ERROR);
	}

	/*
	 * Get the command name index from the object based on the berkdbcmds
	 * defined above.
	 */
	if (Tcl_GetIndexFromObj(interp, objv[1], envcmds, "command",
	    TCL_EXACT, &cmdindex) != TCL_OK)
		return (IS_HELP(objv[1]));
	res = NULL;
	switch ((enum envcmds)cmdindex) {
#ifdef CONFIG_TEST
	case ENVBACKUP:
		result = tcl_EnvBackup(interp, objc, objv, dbenv);
		break;
	case ENVDBBACKUP:
		result = tcl_EnvDbBackup(interp, objc, objv, dbenv);
		break;
	case ENVEVENTINFO:
		result = env_EventInfo(interp, objc, objv, dbenv, envip);
		break;
	case ENVFAILCHK:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbenv->failchk(dbenv, 0);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "failchk");
		break;
	case ENVIDRESET:
		result = tcl_EnvIdReset(interp, objc, objv, dbenv);
		break;
	case ENVLSNRESET:
		result = tcl_EnvLsnReset(interp, objc, objv, dbenv);
		break;
	case ENVLKDETECT:
		result = tcl_LockDetect(interp, objc, objv, dbenv);
		break;
	case ENVLKSTAT:
		result = tcl_LockStat(interp, objc, objv, dbenv);
		break;
	case ENVLKSTATPRT:
		result = tcl_LockStatPrint(interp, objc, objv, dbenv);
		break;
	case ENVLKTIMEOUT:
		result = tcl_LockTimeout(interp, objc, objv, dbenv);
		break;
	case ENVLKID:
		/*
		 * No args for this.  Error if there are some.
		 */
		if (objc > 2) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		_debug_check();
		ret = dbenv->lock_id(dbenv, &lockid);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "lock_id");
		if (result == TCL_OK)
			res = Tcl_NewWideIntObj((Tcl_WideInt)lockid);
		break;
	case ENVLKFREEID:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 3, objv, NULL);
			return (TCL_ERROR);
		}
		result = Tcl_GetLongFromObj(interp, objv[2], &newval);
		if (result != TCL_OK)
			return (result);
		ret = dbenv->lock_id_free(dbenv, (u_int32_t)newval);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "lock id_free");
		break;
	case ENVLKSETID:
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 4, objv, "current max");
			return (TCL_ERROR);
		}
		result = Tcl_GetLongFromObj(interp, objv[2], &newval);
		if (result != TCL_OK)
			return (result);
		result = Tcl_GetLongFromObj(interp, objv[3], &otherval);
		if (result != TCL_OK)
			return (result);
		ret = __lock_id_set(dbenv->env,
		    (u_int32_t)newval, (u_int32_t)otherval);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "lock id_free");
		break;
	case ENVLKPRI:
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 4, objv, NULL);
			return (TCL_ERROR);
		}
		result = _GetUInt32(interp, objv[2], &lockid);
		if (result != TCL_OK)
			return (result);
		result = _GetUInt32(interp, objv[3], &value);
		if (result != TCL_OK)
			return (result);
		if (dbenv->env->lk_handle == NULL) {
			Tcl_SetResult(interp,
			    "env not configured for locking", NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->set_lk_priority(dbenv, lockid, value);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "lock set priority");
		break;
	case ENVLKGET:
		result = tcl_LockGet(interp, objc, objv, dbenv);
		break;
	case ENVLKVEC:
		result = tcl_LockVec(interp, objc, objv, dbenv);
		break;
	case ENVLOGARCH:
		result = tcl_LogArchive(interp, objc, objv, dbenv);
		break;
	case ENVLOGCMP:
		result = tcl_LogCompare(interp, objc, objv);
		break;
	case ENVLOGCONFIG:
		/*
		 * Two args for this.  Error if different.
		 */
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 2, objv, "opt on|off");
			return (TCL_ERROR);
		}
		result = tcl_LogConfig(interp, dbenv, objv[2], objv[3]);
		break;
	case ENVLOGCURSOR:
		snprintf(newname, sizeof(newname),
		    "%s.logc%d", envip->i_name, envip->i_envlogcid);
		logcip = _NewInfo(interp, NULL, newname, I_LOGC);
		if (logcip != NULL) {
			ret = dbenv->log_cursor(dbenv, &logc, 0);
			if (ret == 0) {
				result = TCL_OK;
				envip->i_envlogcid++;
				/*
				 * We do NOT want to set i_parent to
				 * envip here because log cursors are
				 * not "tied" to the env.  That is, they
				 * are NOT closed if the env is closed.
				 */
				(void)Tcl_CreateObjCommand(interp, newname,
				    (Tcl_ObjCmdProc *)logc_Cmd,
				    (ClientData)logc, NULL);
				res = NewStringObj(newname, strlen(newname));
				_SetInfoData(logcip, logc);
			} else {
				_DeleteInfo(logcip);
				result = _ErrorSetup(interp, ret, "log cursor");
			}
		} else {
			Tcl_SetResult(interp,
			    "Could not set up info", TCL_STATIC);
			result = TCL_ERROR;
		}
		break;
	case ENVLOGFILE:
		result = tcl_LogFile(interp, objc, objv, dbenv);
		break;
	case ENVLOGFLUSH:
		result = tcl_LogFlush(interp, objc, objv, dbenv);
		break;
	case ENVLOGGET:
		result = tcl_LogGet(interp, objc, objv, dbenv);
		break;
	case ENVLOGGETCONFIG:
		/*
		 * Two args for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		result = tcl_LogGetConfig(interp, dbenv, objv[2]);
		break;
	case ENVLOGPUT:
		result = tcl_LogPut(interp, objc, objv, dbenv);
		break;
	case ENVLOGSTAT:
		result = tcl_LogStat(interp, objc, objv, dbenv);
		break;
	case ENVLOGSTATPRT:
		result = tcl_LogStatPrint(interp, objc, objv, dbenv);
		break;
	case ENVLOGVERIFY:
		result = tcl_LogVerify(interp, objc, objv, dbenv);
		break;
	case ENVMPSTAT:
		result = tcl_MpStat(interp, objc, objv, dbenv);
		break;
	case ENVMPSTATPRT:
		result = tcl_MpStatPrint(interp, objc, objv, dbenv);
		break;
	case ENVMPSYNC:
		result = tcl_MpSync(interp, objc, objv, dbenv);
		break;
	case ENVTRICKLE:
		result = tcl_MpTrickle(interp, objc, objv, dbenv);
		break;
	case ENVMP:
		result = tcl_Mp(interp, objc, objv, dbenv, envip);
		break;
	case ENVMUTEX:
		result = tcl_Mutex(interp, objc, objv, dbenv);
		break;
	case ENVMUTFREE:
		result = tcl_MutFree(interp, objc, objv, dbenv);
		break;
	case ENVMUTGETALIGN:
		result = tcl_MutGet(interp, dbenv, DBTCL_MUT_ALIGN);
		break;
	case ENVMUTGETINCR:
		result = tcl_MutGet(interp, dbenv, DBTCL_MUT_INCR);
		break;
	case ENVMUTGETINIT:
		result = tcl_MutGet(interp, dbenv, DBTCL_MUT_INIT);
		break;
	case ENVMUTGETMAX:
		result = tcl_MutGet(interp, dbenv, DBTCL_MUT_MAX);
		break;
	case ENVMUTGETTASSPINS:
		result = tcl_MutGet(interp, dbenv, DBTCL_MUT_TAS);
		break;
	case ENVMUTLOCK:
		result = tcl_MutLock(interp, objc, objv, dbenv);
		break;
	case ENVMUTSETTASSPINS:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		result = tcl_MutSet(interp, objv[2], dbenv, DBTCL_MUT_TAS);
		break;
	case ENVMUTSTAT:
		result = tcl_MutStat(interp, objc, objv, dbenv);
		break;
	case ENVMUTSTATPRT:
		result = tcl_MutStatPrint(interp, objc, objv, dbenv);
		break;
	case ENVMUTUNLOCK:
		result = tcl_MutUnlock(interp, objc, objv, dbenv);
		break;
	case ENVREPCONFIG:
		/*
		 * Two args for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		result = tcl_RepConfig(interp, dbenv, objv[2]);
		break;
	case ENVREPELECT:
		result = tcl_RepElect(interp, objc, objv, dbenv);
		break;
	case ENVREPFLUSH:
		result = tcl_RepFlush(interp, objc, objv, dbenv);
		break;
	case ENVREPGETCLOCKSKEW:
		result = tcl_RepGetTwo(interp, dbenv, DBTCL_GETCLOCK);
		break;
	case ENVREPGETCONFIG:
		/*
		 * Two args for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		result = tcl_RepGetConfig(interp, dbenv, objv[2]);
		break;
	case ENVREPGETLIMIT:
		result = tcl_RepGetTwo(interp, dbenv, DBTCL_GETLIMIT);
		break;
	case ENVREPGETNSITES:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->rep_get_nsites(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env rep_get_nsites")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVREPGETPRIORITY:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->rep_get_priority(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env rep_get_priority")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVREPGETREQUEST:
		result = tcl_RepGetTwo(interp, dbenv, DBTCL_GETREQ);
		break;
	case ENVREPGETTIMEOUT:
		/*
		 * Two args for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		result = tcl_RepGetTimeout(interp, dbenv, objv[2]);
		break;
	case ENVREPLEASE:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		result = Tcl_ListObjGetElements(interp, objv[2],
		    &listobjc, &listobjv);
		if (result == TCL_OK)
			result = tcl_RepLease(interp,
			    listobjc, listobjv, dbenv);
		break;
	case ENVREPLIMIT:
		result = tcl_RepLimit(interp, (objc-2), &objv[2], dbenv);
		break;
	case ENVREPNSITES:
		result = tcl_RepNSites(interp, objc, objv, dbenv);
		break;
	case ENVREPPROCMESS:
		result = tcl_RepProcessMessage(interp, objc, objv, dbenv);
		break;
	case ENVREPREQUEST:
		result = tcl_RepRequest(interp, (objc-2), &objv[2], dbenv);
		break;
	case ENVREPSTART:
		result = tcl_RepStart(interp, objc, objv, dbenv);
		break;
	case ENVREPSTAT:
		result = tcl_RepStat(interp, objc, objv, dbenv);
		break;
	case ENVREPSTATPRT:
		result = tcl_RepStatPrint(interp, objc, objv, dbenv);
		break;
	case ENVREPSYNC:
		result = tcl_RepSync(interp, objc, objv, dbenv);
		break;
	case ENVREPTRANSPORT:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		result = Tcl_ListObjGetElements(interp, objv[2],
		    &listobjc, &listobjv);
		if (result == TCL_OK)
			result = tcl_RepTransport(interp,
			    listobjc, listobjv, dbenv, envip);
		break;
	case ENVREPMGR:
		result = tcl_RepMgr(interp, objc, objv, dbenv);
		break;
	case ENVREPMGRGETACK:
		result = tcl_RepGetAckPolicy(interp, objc, objv, dbenv);
		break;
	case ENVREPMGRGETINQUEUE:
		result = tcl_RepGetTwo(interp, dbenv, DBTCL_GETINQUEUE);
		break;
	case ENVREPMGRGETLOCAL:
		result = tcl_RepGetLocalSite(interp, objc, objv, dbenv);
		break;	
	case ENVREPMGRSITELIST:
		result = tcl_RepMgrSiteList(interp, objc, objv, dbenv);
		break;
	case ENVREPMGRSTAT:
		result = tcl_RepMgrStat(interp, objc, objv, dbenv);
		break;
	case ENVREPMGRSTATPRT:
		result = tcl_RepMgrStatPrint(interp, objc, objv, dbenv);
		break;
	case ENVSETLOGMAX:
		result = tcl_LogSetMax(interp, dbenv, objv[2], NULL, NULL);
		break;
	case ENVTXNAPPLIED:
		result = tcl_RepApplied(interp, objc, objv, dbenv);
		break;
	case ENVTXNSETID:
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 4, objv, "current max");
			return (TCL_ERROR);
		}
		result = Tcl_GetLongFromObj(interp, objv[2], &newval);
		if (result != TCL_OK)
			return (result);
		result = Tcl_GetLongFromObj(interp, objv[3], &otherval);
		if (result != TCL_OK)
			return (result);
		ret = __txn_id_set(dbenv->env,
		    (u_int32_t)newval, (u_int32_t)otherval);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "txn setid");
		break;
	case ENVTXNRECOVER:
		result = tcl_TxnRecover(interp, objc, objv, dbenv, envip);
		break;
	case ENVTXNSTAT:
		result = tcl_TxnStat(interp, objc, objv, dbenv);
		break;
	case ENVTXNSTATPRT:
		result = tcl_TxnStatPrint(interp, objc, objv, dbenv);
		break;
	case ENVTXNTIMEOUT:
		result = tcl_TxnTimeout(interp, objc, objv, dbenv);
		break;
	case ENVATTR:
		result = tcl_EnvAttr(interp, objc, objv, dbenv);
		break;
	case ENVERRFILE:
		/*
		 * One args for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "errfile");
			return (TCL_ERROR);
		}
		strarg = Tcl_GetStringFromObj(objv[2], NULL);
		tcl_EnvSetErrfile(interp, dbenv, envip, strarg);
		result = TCL_OK;
		break;
	case ENVERRPFX:
		/*
		 * One args for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "pfx");
			return (TCL_ERROR);
		}
		strarg = Tcl_GetStringFromObj(objv[2], NULL);
		result = tcl_EnvSetErrpfx(interp, dbenv, envip, strarg);
		break;
	case ENVMSGFILE:
		/*
		 * One args for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "msgfile");
			return (TCL_ERROR);
		}
		strarg = Tcl_GetStringFromObj(objv[2], NULL);
		tcl_EnvSetMsgfile(interp, dbenv, envip, strarg);
		result = TCL_OK;
		break;
	case ENVSETFLAGS:
		/*
		 * Two args for this.  Error if different.
		 */
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 2, objv, "which on|off");
			return (TCL_ERROR);
		}
		result = tcl_EnvSetFlags(interp, dbenv, objv[2], objv[3]);
		break;
	case ENVSTATPRT:
		result = tcl_EnvStatPrint(interp, objc, objv, dbenv);
		break;
	case ENVTEST:
		result = tcl_EnvTest(interp, objc, objv, dbenv);
		break;
	case ENVVERB:
		/*
		 * Two args for this.  Error if different.
		 */
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 2, objv, NULL);
			return (TCL_ERROR);
		}
		result = tcl_EnvVerbose(interp, dbenv, objv[2], objv[3]);
		break;
#endif
	case ENVCDSGROUP:
		result = tcl_CDSGroup(interp, objc, objv, dbenv, envip);
		break;
	case ENVCLOSE:
		result = tcl_EnvClose(interp, objc, objv, dbenv, envip);
		break;
	case ENVDBREMOVE:
		result = env_DbRemove(interp, objc, objv, dbenv);
		break;
	case ENVDBRENAME:
		result = env_DbRename(interp, objc, objv, dbenv);
		break;
	case ENVGETBLOBDIR:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_blob_dir(dbenv, &strval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_blob_dir")) == TCL_OK)
			res = NewStringObj(strval,
			    strval != NULL ? strlen(strval) : 0);
		break;
	case ENVGETBLOBTHRESHOLD:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_blob_threshold(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_blob_threshold")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETCACHESIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_cachesize(dbenv, &gbytes, &bytes, &ncache);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_cachesize")) == TCL_OK) {
			myobjv[0] = Tcl_NewLongObj((long)gbytes);
			myobjv[1] = Tcl_NewLongObj((long)bytes);
			myobjv[2] = Tcl_NewLongObj((long)ncache);
			res = Tcl_NewListObj(3, myobjv);
		}
		break;
	case ENVGETCACHEMAX:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_cache_max(dbenv, &gbytes, &bytes);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_cache_max")) == TCL_OK) {
			myobjv[0] = Tcl_NewLongObj((long)gbytes);
			myobjv[1] = Tcl_NewLongObj((long)bytes);
			res = Tcl_NewListObj(2, myobjv);
		}
		break;
	case ENVGETCREATEDIR:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_create_dir(dbenv, &strval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_create_dir")) == TCL_OK)
			res = NewStringObj(strval,
			    strval != NULL ? strlen(strval) : 0);
		break;
	case ENVGETDATADIRS:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_data_dirs(dbenv, &dirs);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_data_dirs")) == TCL_OK) {
			res = Tcl_NewListObj(0, NULL);
			for (i = 0; result == TCL_OK && dirs[i] != NULL; i++)
				result = Tcl_ListObjAppendElement(interp, res,
				    NewStringObj(dirs[i],
				    dirs[i]  != NULL ? strlen(dirs[i]) : 0));
		}
		break;
	case ENVGETENCRYPTFLAGS:
		result = tcl_EnvGetEncryptFlags(interp, objc, objv, dbenv);
		break;
	case ENVGETERRPFX:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		dbenv->get_errpfx(dbenv, &strval);
		res = NewStringObj(strval, strlen(strval));
		break;
	case ENVGETFLAGS:
		result = env_GetFlags(interp, objc, objv, dbenv);
		break;
	case ENVGETHOME:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_home(dbenv, &strval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_home")) == TCL_OK)
			res = NewStringObj(strval, strlen(strval));
		break;
	case ENVGETLGBSIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lg_bsize(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lg_bsize")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLGDIR:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lg_dir(dbenv, &strval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lg_dir")) == TCL_OK)
			res = NewStringObj(strval, strlen(strval));
		break;
	case ENVGETLGFILEMODE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lg_filemode(dbenv, &intvalue);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lg_filemode")) == TCL_OK)
			res = Tcl_NewLongObj((long)intvalue);
		break;
	case ENVGETLGMAX:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lg_max(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lg_max")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLGREGIONMAX:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lg_regionmax(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lg_regionmax")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKDETECT:
		result = env_GetLockDetect(interp, objc, objv, dbenv);
		break;
	case ENVGETLKINITLOCKERS:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_memory_init(dbenv, DB_MEM_LOCKER, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_init_lockers")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKINITLOCKS:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_memory_init(dbenv, DB_MEM_LOCK, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_init_locks")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKINITLOGID:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_memory_init(dbenv, DB_MEM_LOGID, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_init_logid")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKINITOBJECTS:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_memory_init(dbenv, DB_MEM_LOCKOBJECT, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_init_objects")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKINITTHREAD:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_memory_init(dbenv, DB_MEM_THREAD, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_init_thread")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKMAXLOCKERS:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lk_max_lockers(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_max_lockers")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKMAXLOCKS:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lk_max_locks(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_max_locks")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKMAXOBJECTS:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lk_max_objects(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_max_objects")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKPARTITIONS:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lk_partitions(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_partitions")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETLKTABLESIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_lk_tablesize(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_lk_tablesize")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETMEMORYMAX:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_memory_max(dbenv, &gbytes, &bytes);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_memory_max")) == TCL_OK) {
			myobjv[0] = Tcl_NewLongObj((long)gbytes);
			myobjv[1] = Tcl_NewLongObj((long)bytes);
			res = Tcl_NewListObj(2, myobjv);
		}
		break;
	case ENVGETMETADATADIR:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_metadata_dir(dbenv, &strval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_metadata_dir")) == TCL_OK)
			res = NewStringObj(strval, strlen(strval));
		break;
	case ENVGETMPMAXOPENFD:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_mp_max_openfd(dbenv, &intvalue);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_mp_max_openfd")) == TCL_OK)
			res = Tcl_NewIntObj(intvalue);
		break;
	case ENVGETMPMAXWRITE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_mp_max_write(dbenv, &intvalue, &timeout);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_mp_max_write")) == TCL_OK) {
			myobjv[0] = Tcl_NewIntObj(intvalue);
			myobjv[1] = Tcl_NewIntObj((int)timeout);
			res = Tcl_NewListObj(2, myobjv);
		}
		break;
	case ENVGETMPMMAPSIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_mp_mmapsize(dbenv, &size);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_mp_mmapsize")) == TCL_OK)
			res = Tcl_NewLongObj((long)size);
		break;
	case ENVGETMPMTXCOUNT:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_mp_mtxcount(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_mp_mtxcount")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETMPPAGESIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_mp_pagesize(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_mp_pagesize")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETMPTABLESIZE:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_mp_tablesize(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_mp_tablesize")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETOPENFLAG:
		result = env_GetOpenFlag(interp, objc, objv, dbenv);
		break;
	case ENVGETSHMKEY:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_shm_key(dbenv, &shm_key);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env shm_key")) == TCL_OK)
			res = Tcl_NewLongObj(shm_key);
		break;
	case ENVGETTASSPINS:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->mutex_get_tas_spins(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_tas_spins")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETTHREADCOUNT:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_thread_count(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_thread_count")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETTIMEOUT:
		result = env_GetTimeout(interp, objc, objv, dbenv);
		break;
	case ENVGETTMPDIR:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_tmp_dir(dbenv, &strval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_tmp_dir")) == TCL_OK)
			res = NewStringObj(strval, strlen(strval));
		break;
	case ENVGETTXINIT:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_memory_init(dbenv, DB_MEM_TRANSACTION, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_tx_init")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETTXMAX:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_tx_max(dbenv, &value);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_tx_max")) == TCL_OK)
			res = Tcl_NewLongObj((long)value);
		break;
	case ENVGETTXTIMESTAMP:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 1, objv, NULL);
			return (TCL_ERROR);
		}
		ret = dbenv->get_tx_timestamp(dbenv, &timeval);
		if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env get_tx_timestamp")) == TCL_OK)
			res = Tcl_NewLongObj((long)timeval);
		break;
	case ENVGETVERBOSE:
		result = env_GetVerbose(interp, objc, objv, dbenv);
		break;
	case ENVRESIZECACHE:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv,
			    "?-resize_cache {gbytes bytes}?");
			result = TCL_ERROR;
			break;
		}
		if ((result = Tcl_ListObjGetElements(
		    interp, objv[2], &listobjc, &listobjv)) != TCL_OK)
			break;
		if (listobjc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv,
			    "?-resize_cache {gbytes bytes}?");
			result = TCL_ERROR;
			break;
		}
		result = _GetUInt32(interp, listobjv[0], &gbytes);
		if (result != TCL_OK)
			break;
		result = _GetUInt32(interp, listobjv[1], &bytes);
		if (result != TCL_OK)
			break;
		ret = dbenv->set_cachesize(dbenv, gbytes, bytes, 0);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "resize_cache");
		break;
	case ENVSETBLOBTHRESHOLD:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv,
			    "?-set_blob_threshold bytes?");
			result = TCL_ERROR;
			break;
		}
		result = _GetUInt32(interp, objv[2], &bytes);
		ret = dbenv->set_blob_threshold(dbenv, bytes, 0);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env set_blob_threshold");
		break;
	case ENVSETDATADIR:
		/*
		 * One args for this.  Error if different.
		 */
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "pfx");
			return (TCL_ERROR);
		}
		strarg = Tcl_GetStringFromObj(objv[2], NULL);
		ret = dbenv->set_data_dir(dbenv, strarg);
		return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "env set data dir"));
	case ENVSETMAXCACHE:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv,
			    "?-set_max_cache {gbytes bytes}?");
			result = TCL_ERROR;
			break;
		}
		if ((result = Tcl_ListObjGetElements(
		    interp, objv[2], &listobjc, &listobjv)) != TCL_OK)
			break;
		if (listobjc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv,
			    "?-set_max_cache {gbytes bytes}?");
			result = TCL_ERROR;
			break;
		}
		result = _GetUInt32(interp, listobjv[0], &gbytes);
		if (result != TCL_OK)
			break;
		result = _GetUInt32(interp, listobjv[1], &bytes);
		if (result != TCL_OK)
			break;
		ret = dbenv->set_cache_max(dbenv, gbytes, bytes);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "set_max_cache");
		break;
	case ENVTXN:
		result = tcl_Txn(interp, objc, objv, dbenv, envip);
		break;
	case ENVTXNCKP:
		result = tcl_TxnCheckpoint(interp, objc, objv, dbenv);
		break;
	}
	/*
	 * Only set result if we have a res.  Otherwise, lower
	 * functions have already done so.
	 */
	if (result == TCL_OK && res)
		Tcl_SetObjResult(interp, res);
	return (result);
}

/*
 * PUBLIC: int tcl_EnvRemove __P((Tcl_Interp *, int, Tcl_Obj * CONST*));
 *
 * tcl_EnvRemove --
 */
int
tcl_EnvRemove(interp, objc, objv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
{
	static const char *envremopts[] = {
#ifdef CONFIG_TEST
		"-overwrite",
#endif
		"-data_dir",
		"-encryptaes",
		"-encryptany",
		"-force",
		"-home",
		"-log_dir",
		"-metadata_dir",
		"-tmp_dir",
		"-use_environ",
		"-use_environ_root",
		NULL
	};
	enum envremopts {
#ifdef CONFIG_TEST
		ENVREM_OVERWRITE,
#endif
		ENVREM_DATADIR,
		ENVREM_ENCRYPT_AES,
		ENVREM_ENCRYPT_ANY,
		ENVREM_FORCE,
		ENVREM_HOME,
		ENVREM_LOGDIR,
		ENVREM_METADATADIR,
		ENVREM_TMPDIR,
		ENVREM_USE_ENVIRON,
		ENVREM_USE_ENVIRON_ROOT
	};
	DB_ENV *dbenv;
	u_int32_t cflag, enc_flag, flag, forceflag, sflag;
	int i, optindex, result, ret;
	char *datadir, *home, *logdir, *mddir, *passwd, *tmpdir;

	result = TCL_OK;
	cflag = flag = forceflag = sflag = 0;
	home = NULL;
	passwd = NULL;
	datadir = logdir = mddir = tmpdir = NULL;
	enc_flag = 0;

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?args?");
		return (TCL_ERROR);
	}

	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], envremopts, "option",
		    TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			goto error;
		}
		i++;
		switch ((enum envremopts)optindex) {
		case ENVREM_ENCRYPT_AES:
			/* Make sure we have an arg to check against! */
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-encryptaes passwd?");
				result = TCL_ERROR;
				break;
			}
			passwd = Tcl_GetStringFromObj(objv[i++], NULL);
			enc_flag = DB_ENCRYPT_AES;
			break;
		case ENVREM_ENCRYPT_ANY:
			/* Make sure we have an arg to check against! */
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-encryptany passwd?");
				result = TCL_ERROR;
				break;
			}
			passwd = Tcl_GetStringFromObj(objv[i++], NULL);
			enc_flag = 0;
			break;
		case ENVREM_FORCE:
			forceflag |= DB_FORCE;
			break;
		case ENVREM_HOME:
			/* Make sure we have an arg to check against! */
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "?-home dir?");
				result = TCL_ERROR;
				break;
			}
			home = Tcl_GetStringFromObj(objv[i++], NULL);
			break;
#ifdef CONFIG_TEST
		case ENVREM_OVERWRITE:
			sflag |= DB_OVERWRITE;
			break;
#endif
		case ENVREM_USE_ENVIRON:
			flag |= DB_USE_ENVIRON;
			break;
		case ENVREM_USE_ENVIRON_ROOT:
			flag |= DB_USE_ENVIRON_ROOT;
			break;
		case ENVREM_DATADIR:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "-data_dir dir");
				result = TCL_ERROR;
				break;
			}
			datadir = Tcl_GetStringFromObj(objv[i++], NULL);
			break;
		case ENVREM_LOGDIR:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "-log_dir dir");
				result = TCL_ERROR;
				break;
			}
			logdir = Tcl_GetStringFromObj(objv[i++], NULL);
			break;
		case ENVREM_METADATADIR:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "-metadata_dir dir");
				result = TCL_ERROR;
				break;
			}
			mddir = Tcl_GetStringFromObj(objv[i++], NULL);
			break;
		case ENVREM_TMPDIR:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv,
				    "-tmp_dir dir");
				result = TCL_ERROR;
				break;
			}
			tmpdir = Tcl_GetStringFromObj(objv[i++], NULL);
			break;
		}
		/*
		 * If, at any time, parsing the args we get an error,
		 * bail out and return.
		 */
		if (result != TCL_OK)
			goto error;
	}

	if ((ret = db_env_create(&dbenv, cflag)) != 0) {
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db_env_create");
		goto error;
	}
	if (datadir != NULL) {
		_debug_check();
		ret = dbenv->set_data_dir(dbenv, datadir);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "set_data_dir");
		if (result != TCL_OK)
			goto error;
	}
	if (logdir != NULL) {
		_debug_check();
		ret = dbenv->set_lg_dir(dbenv, logdir);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "set_log_dir");
		if (result != TCL_OK)
			goto error;
	}
	if (mddir != NULL) {
		_debug_check();
		ret = dbenv->set_metadata_dir(dbenv, mddir);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "set_metadata_dir");
		if (result != TCL_OK)
			goto error;
	}
	if (tmpdir != NULL) {
		_debug_check();
		ret = dbenv->set_tmp_dir(dbenv, tmpdir);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "set_tmp_dir");
		if (result != TCL_OK)
			goto error;
	}
	if (passwd != NULL) {
		ret = dbenv->set_encrypt(dbenv, passwd, enc_flag);
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "set_encrypt");
	}
	if (sflag != 0 &&
	    (ret = dbenv->set_flags(dbenv, sflag, 1)) != 0) {
		_debug_check();
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "set_flags");
		if (result != TCL_OK)
			goto error;
	}
	dbenv->set_errpfx(dbenv, "EnvRemove");
	dbenv->set_errcall(dbenv, _ErrorFunc);

	flag |= forceflag;
	/*
	 * When we get here we have parsed all the args.  Now remove
	 * the environment.
	 */
	_debug_check();
	ret = dbenv->remove(dbenv, home, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env remove");
error:
	return (result);
}

static void
_EnvInfoDelete(interp, envip)
	Tcl_Interp *interp;		/* Tcl Interpreter */
	DBTCL_INFO *envip;		/* Info for env */
{
	DBTCL_INFO *nextp, *p;

	/*
	 * Before we can delete the environment info, we must close
	 * any open subsystems in this env.  We will:
	 * 1.  Abort any transactions (which aborts any nested txns).
	 * 2.  Close any mpools (which will put any pages itself).
	 * 3.  Put any locks and close log cursors.
	 * 4.  Close the error file.
	 */
	for (p = LIST_FIRST(&__db_infohead); p != NULL; p = nextp) {
		/*
		 * Check if this info structure "belongs" to this
		 * env.  If so, remove its commands and info structure.
		 * We do not close/abort/whatever here, because we
		 * don't want to replicate DB behavior.
		 *
		 * NOTE:  Only those types that can nest need to be
		 * itemized in the switch below.  That is txns and mps.
		 * Other types like log cursors and locks will just
		 * get cleaned up here.
		 */
		if (p->i_parent == envip) {
			switch (p->i_type) {
			case I_TXN:
				_TxnInfoDelete(interp, p);
				break;
			case I_MP:
				_MpInfoDelete(interp, p);
				break;
			case I_AUX:
			case I_DB:
			case I_DBC:
			case I_DBSTREAM:
			case I_ENV:
			case I_LOCK:
			case I_LOGC:
			case I_NDBM:
			case I_PG:
			case I_SEQ:
				Tcl_SetResult(interp,
				    "_EnvInfoDelete: bad info type",
				    TCL_STATIC);
				break;
			}
			nextp = LIST_NEXT(p, entries);
			(void)Tcl_DeleteCommand(interp, p->i_name);
			_DeleteInfo(p);
		} else
			nextp = LIST_NEXT(p, entries);
	}
	(void)Tcl_DeleteCommand(interp, envip->i_name);
	_DeleteInfo(envip);
}

/*
 * PUBLIC: int tcl_EnvClose __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *, DBTCL_INFO *));
 *
 * tcl_EnvClose --
 *	Implements the DB_ENV->close command.
 */
int
tcl_EnvClose(interp, objc, objv, dbenv, envip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* arg count */
	Tcl_Obj * CONST* objv;		/* args */
	DB_ENV *dbenv;			/* Database pointer */
	DBTCL_INFO *envip;		/* Info pointer */
{
	static const char *closeoptions[] = {
		"-forcesync",
		NULL
	};
	enum closeoptions {
		FORCESYNC
	};
	int i, result, ret, t_ret;
	u_int32_t flags;

	result = TCL_OK;
	flags = 0;
	Tcl_SetResult(interp, "0", TCL_STATIC);
	if (objc > 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-forcesync?");
		return (TCL_ERROR);
	} else if (objc == 3) {
		/*
		 * If there is an arg, make sure it is the right one.
		 */
		if (Tcl_GetIndexFromObj(interp, objv[2], closeoptions, "option",
		    TCL_EXACT, &i) != TCL_OK)
			return (IS_HELP(objv[2]));

		switch ((enum closeoptions)i) {
			case FORCESYNC:
				flags |= DB_FORCESYNC;
				break;
		}
	}

	/*
	 * Any transactions will be aborted, and an mpools
	 * closed automatically.  We must delete any txn
	 * and mp widgets we have here too for this env.
	 * NOTE: envip is freed when we come back from
	 * this function.  Set it to NULL to make sure no
	 * one tries to use it later.
	 */
	ret = __mutex_free(dbenv->env, &envip->i_mutex);
	_debug_check();
	if ((t_ret = dbenv->close(dbenv, flags)) != 0 && ret == 0)
		ret = t_ret;
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "env close");
	_EnvInfoDelete(interp, envip);
	envip = NULL;
	return (result);
}

#ifdef CONFIG_TEST
/*
 * PUBLIC: int tcl_EnvBackup __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *));
 *
 * tcl_EnvBackup --
 *	Implements the ENV->backup command.
 */
int
tcl_EnvBackup(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* arg count */
	Tcl_Obj * CONST* objv;		/* args */
	DB_ENV *dbenv;			/* Database pointer */
{
	static const char *buwhich[] = {
		"-clean",
		"-create",
		"-excl",
		"-files",
		"-no_logs",
		"-single_dir",
		"-update",
		"-verbose",
		NULL
	};
	enum buwhich {
		BKUPCLEAN,
		BKUPCREATE,
		BKUPEXCL,
		BKUPFILES,
		BKUPNOLOGS,
		BKUPSINGLEDIR,
		BKUPUPDATE,
		BKUPVERBOSE
	};
	int i, optindex, result, ret;
	u_int32_t flags;
	char *targetdir;

	result = TCL_OK;
	flags = 0;
	i = 2;
	Tcl_SetResult(interp, "0", TCL_STATIC);
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?args? target");
		return (TCL_ERROR);
	} 
	while (i < (objc - 1)) {
		if (Tcl_GetIndexFromObj(interp, objv[i], buwhich, "option",
		    TCL_EXACT, &optindex) != TCL_OK)
			return (IS_HELP(objv[i]));
		i++;
		switch ((enum buwhich)optindex) {
		case BKUPCLEAN:
			flags |= DB_BACKUP_CLEAN;
			break;
		case BKUPCREATE:
			flags |= DB_CREATE;
			break;
		case BKUPEXCL:
			flags |= DB_EXCL;
			break;
		case BKUPFILES:
			flags |= DB_BACKUP_FILES;
			break;
		case BKUPNOLOGS:
			flags |= DB_BACKUP_NO_LOGS;
			break;
		case BKUPSINGLEDIR:
			flags |= DB_BACKUP_SINGLE_DIR;
			break;
		case BKUPUPDATE:
			flags |= DB_BACKUP_UPDATE;
			break;
		case BKUPVERBOSE:
			flags |= DB_VERB_BACKUP;
			break;
		}
	}
	targetdir = Tcl_GetStringFromObj(objv[i], NULL);
	ret = dbenv->backup(dbenv, targetdir, flags);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "backup");
	return (result);
}

/*
 * PUBLIC: int tcl_EnvDbBackup __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *));
 *
 * tcl_EnvDbBackup --
 *	Implements the ENV->dbbackup command.
 */
int
tcl_EnvDbBackup(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* arg count */
	Tcl_Obj * CONST* objv;		/* args */
	DB_ENV *dbenv;			/* Database pointer */
{
	static const char *dbbuwhich[] = {
		"-excl",
		NULL
	};
	enum dbbuwhich {
		DBBKUPEXCL
	};
	int i, optindex, result, ret;
	u_int32_t flags;
	char *srcfile, *targetdir;

	result = TCL_OK;
	flags = 0;
	i = 2;
	Tcl_SetResult(interp, "0", TCL_STATIC);
	if (objc < 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "?args? file targetdir");
		return (TCL_ERROR);
	} else if (objc > 4) {
		/*
		 * If there is an arg, make sure it is the right one.
		 */
		if (Tcl_GetIndexFromObj(interp, objv[2], dbbuwhich, "option",
		    TCL_EXACT, &optindex) != TCL_OK)
			return (IS_HELP(objv[2]));
		switch ((enum dbbuwhich)optindex) {
		case DBBKUPEXCL:
			flags |= DB_EXCL;
			break;
		}
		i = 4;
	}
	srcfile = Tcl_GetStringFromObj(objv[i++], NULL);
	targetdir = Tcl_GetStringFromObj(objv[i], NULL);
	ret = dbenv->dbbackup(dbenv, srcfile, targetdir, flags);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "dbbackup");
	return (result);
}

/*
 * PUBLIC: int tcl_EnvIdReset __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *));
 *
 * tcl_EnvIdReset --
 *	Implements the ENV->fileid_reset command.
 */
int
tcl_EnvIdReset(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* arg count */
	Tcl_Obj * CONST* objv;		/* args */
	DB_ENV *dbenv;			/* Database pointer */
{
	static const char *idwhich[] = {
		"-encrypt",
		NULL
	};
	enum idwhich {
		IDENCRYPT
	};
	int enc, i, result, ret;
	u_int32_t flags;
	char *file;

	result = TCL_OK;
	flags = 0;
	i = 2;
	Tcl_SetResult(interp, "0", TCL_STATIC);
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-encrypt? filename");
		return (TCL_ERROR);
	} else if (objc > 3) {
		/*
		 * If there is an arg, make sure it is the right one.
		 */
		if (Tcl_GetIndexFromObj(interp, objv[2], idwhich, "option",
		    TCL_EXACT, &enc) != TCL_OK)
			return (IS_HELP(objv[2]));
		switch ((enum idwhich)enc) {
		case IDENCRYPT:
			flags |= DB_ENCRYPT;
			break;
		}
		i = 3;
	}
	file = Tcl_GetStringFromObj(objv[i], NULL);
	ret = dbenv->fileid_reset(dbenv, file, flags);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "fileid reset");
	return (result);
}

/*
 * PUBLIC: int tcl_EnvLsnReset __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:    DB_ENV *));
 *
 * tcl_EnvLsnReset --
 *	Implements the ENV->lsn_reset command.
 */
int
tcl_EnvLsnReset(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* arg count */
	Tcl_Obj * CONST* objv;		/* args */
	DB_ENV *dbenv;			/* Database pointer */
{
	static const char *lsnwhich[] = {
		"-encrypt",
		NULL
	};
	enum lsnwhich {
		IDENCRYPT
	};
	int enc, i, result, ret;
	u_int32_t flags;
	char *file;

	result = TCL_OK;
	flags = 0;
	i = 2;
	Tcl_SetResult(interp, "0", TCL_STATIC);
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-encrypt? filename");
		return (TCL_ERROR);
	} else if (objc > 3) {
		/*
		 * If there is an arg, make sure it is the right one.
		 */
		if (Tcl_GetIndexFromObj(interp, objv[2], lsnwhich, "option",
		    TCL_EXACT, &enc) != TCL_OK)
			return (IS_HELP(objv[2]));

		switch ((enum lsnwhich)enc) {
		case IDENCRYPT:
			flags |= DB_ENCRYPT;
			break;
		}
		i = 3;
	}
	file = Tcl_GetStringFromObj(objv[i], NULL);
	ret = dbenv->lsn_reset(dbenv, file, flags);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret), "lsn reset");
	return (result);
}

/*
 * PUBLIC: int tcl_EnvVerbose __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *,
 * PUBLIC:    Tcl_Obj *));
 *
 * tcl_EnvVerbose --
 */
int
tcl_EnvVerbose(interp, dbenv, which, onoff)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Env pointer */
	Tcl_Obj *which;			/* Which subsystem */
	Tcl_Obj *onoff;			/* On or off */
{
	static const char *verbwhich[] = {
		"backup",
		"deadlock",
		"fileops",
		"fileops_all",
		"mvcc",
		"recovery",
		"register",
		"rep",
		"rep_elect",
		"rep_lease",
		"rep_misc",
		"rep_msgs",
		"rep_sync",
		"rep_system",
		"rep_test",
		"repmgr_connfail",
		"repmgr_misc",
		"wait",
		NULL
	};
	enum verbwhich {
		ENVVERB_BACKUP,
		ENVVERB_DEADLOCK,
		ENVVERB_FILEOPS,
		ENVVERB_FILEOPS_ALL,
		ENVVERB_MVCC,
		ENVVERB_RECOVERY,
		ENVVERB_REGISTER,
		ENVVERB_REPLICATION,
		ENVVERB_REP_ELECT,
		ENVVERB_REP_LEASE,
		ENVVERB_REP_MISC,
		ENVVERB_REP_MSGS,
		ENVVERB_REP_SYNC,
		ENVVERB_REP_SYSTEM,
		ENVVERB_REP_TEST,
		ENVVERB_REPMGR_CONNFAIL,
		ENVVERB_REPMGR_MISC,
		ENVVERB_WAITSFOR
	};
	static const char *verbonoff[] = {
		"off",
		"on",
		NULL
	};
	enum verbonoff {
		ENVVERB_OFF,
		ENVVERB_ON
	};
	int on, optindex, ret;
	u_int32_t wh;

	if (Tcl_GetIndexFromObj(interp, which, verbwhich, "option",
	    TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(which));

	switch ((enum verbwhich)optindex) {
	case ENVVERB_BACKUP:
		wh = DB_VERB_BACKUP;
		break;
	case ENVVERB_DEADLOCK:
		wh = DB_VERB_DEADLOCK;
		break;
	case ENVVERB_FILEOPS:
		wh = DB_VERB_FILEOPS;
		break;
	case ENVVERB_FILEOPS_ALL:
		wh = DB_VERB_FILEOPS_ALL;
		break;
	case ENVVERB_MVCC:
		wh = DB_VERB_MVCC;
		break;
	case ENVVERB_RECOVERY:
		wh = DB_VERB_RECOVERY;
		break;
	case ENVVERB_REGISTER:
		wh = DB_VERB_REGISTER;
		break;
	case ENVVERB_REPLICATION:
		wh = DB_VERB_REPLICATION;
		break;
	case ENVVERB_REP_ELECT:
		wh = DB_VERB_REP_ELECT;
		break;
	case ENVVERB_REP_LEASE:
		wh = DB_VERB_REP_LEASE;
		break;
	case ENVVERB_REP_MISC:
		wh = DB_VERB_REP_MISC;
		break;
	case ENVVERB_REP_MSGS:
		wh = DB_VERB_REP_MSGS;
		break;
	case ENVVERB_REP_SYNC:
		wh = DB_VERB_REP_SYNC;
		break;
	case ENVVERB_REP_SYSTEM:
		wh = DB_VERB_REP_SYSTEM;
		break;
	case ENVVERB_REP_TEST:
		wh = DB_VERB_REP_TEST;
		break;
	case ENVVERB_REPMGR_CONNFAIL:
		wh = DB_VERB_REPMGR_CONNFAIL;
		break;
	case ENVVERB_REPMGR_MISC:
		wh = DB_VERB_REPMGR_MISC;
		break;
	case ENVVERB_WAITSFOR:
		wh = DB_VERB_WAITSFOR;
		break;
	default:
		return (TCL_ERROR);
	}
	if (Tcl_GetIndexFromObj(interp, onoff, verbonoff, "option",
	    TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(onoff));
	switch ((enum verbonoff)optindex) {
	case ENVVERB_OFF:
		on = 0;
		break;
	case ENVVERB_ON:
		on = 1;
		break;
	default:
		return (TCL_ERROR);
	}
	ret = dbenv->set_verbose(dbenv, wh, on);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env set verbose"));
}
#endif

#ifdef CONFIG_TEST
/*
 * PUBLIC: int tcl_EnvAttr __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
 *
 * tcl_EnvAttr --
 *	Return a list of the env's attributes
 */
int
tcl_EnvAttr(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Env pointer */
{
	ENV *env;
	Tcl_Obj *myobj, *retlist;
	int result;

	env = dbenv->env;
	result = TCL_OK;

	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return (TCL_ERROR);
	}
	retlist = Tcl_NewListObj(0, NULL);
	/*
	 * XXX
	 * We peek at the ENV to determine what subsystems we have available
	 * in this environment.
	 */
	myobj = NewStringObj("-home", strlen("-home"));
	if ((result = Tcl_ListObjAppendElement(interp,
	    retlist, myobj)) != TCL_OK)
		goto err;
	myobj = NewStringObj(env->db_home, strlen(env->db_home));
	if ((result = Tcl_ListObjAppendElement(interp,
	    retlist, myobj)) != TCL_OK)
		goto err;
	if (CDB_LOCKING(env)) {
		myobj = NewStringObj("-cdb", strlen("-cdb"));
		if ((result = Tcl_ListObjAppendElement(interp,
		    retlist, myobj)) != TCL_OK)
			goto err;
	}
	if (CRYPTO_ON(env)) {
		myobj = NewStringObj("-crypto", strlen("-crypto"));
		if ((result = Tcl_ListObjAppendElement(interp,
		    retlist, myobj)) != TCL_OK)
			goto err;
	}
	if (LOCKING_ON(env)) {
		myobj = NewStringObj("-lock", strlen("-lock"));
		if ((result = Tcl_ListObjAppendElement(interp,
		    retlist, myobj)) != TCL_OK)
			goto err;
	}
	if (LOGGING_ON(env)) {
		myobj = NewStringObj("-log", strlen("-log"));
		if ((result = Tcl_ListObjAppendElement(interp,
		    retlist, myobj)) != TCL_OK)
			goto err;
	}
	if (MPOOL_ON(env)) {
		myobj = NewStringObj("-mpool", strlen("-mpool"));
		if ((result = Tcl_ListObjAppendElement(interp,
		    retlist, myobj)) != TCL_OK)
			goto err;
	}
	if (REP_ON(env)) {
		myobj = NewStringObj("-rep", strlen("-rep"));
		if ((result = Tcl_ListObjAppendElement(interp,
		    retlist, myobj)) != TCL_OK)
			goto err;
	}
	if (TXN_ON(env)) {
		myobj = NewStringObj("-txn", strlen("-txn"));
		if ((result = Tcl_ListObjAppendElement(interp,
		    retlist, myobj)) != TCL_OK)
			goto err;
	}
	Tcl_SetObjResult(interp, retlist);
err:
	return (result);
}

/*
 * env_EventInfo --
 *	Implements the ENV->event_info command.
 */
static int
env_EventInfo(interp, objc, objv, dbenv, ip)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
	DBTCL_INFO *ip;
{
	static const char *envinfo_option_names[] = {
		"-clear",
		NULL
	};
	enum envinfo_options {
		ENVINFCLEAR
	};
	DB_LSN *lsn;
	Tcl_Obj *lsnlist[2], *myobjv[2], *one_event, *res;
	int clear, enc, i, ret, t_ret;
	u_int32_t bit_flag;

	static const struct {
		u_int32_t flag;
		char *name;
	} event_names[] = {
		{ DB_EVENT_PANIC, "panic" },
		{ DB_EVENT_REG_ALIVE, "reg_alive" },
		{ DB_EVENT_REG_PANIC, "reg_panic" },
		{ DB_EVENT_REP_AUTOTAKEOVER_FAILED, "autotakeover_failed" },
		{ DB_EVENT_REP_CLIENT, "client" },
		{ DB_EVENT_REP_CONNECT_BROKEN, "connection_broken" },
		{ DB_EVENT_REP_CONNECT_ESTD, "connection_established" },
		{ DB_EVENT_REP_CONNECT_TRY_FAILED, "connection_retry_failed" },
		{ DB_EVENT_REP_DUPMASTER, "dupmaster" },
		{ DB_EVENT_REP_ELECTED, "elected" },
		{ DB_EVENT_REP_ELECTION_FAILED, "election_failed" },
		{ DB_EVENT_REP_JOIN_FAILURE, "join_failure" },
		{ DB_EVENT_REP_LOCAL_SITE_REMOVED, "local_site_removed" },
		{ DB_EVENT_REP_MASTER, "master" },
		{ DB_EVENT_REP_MASTER_FAILURE, "master_failure" },
		{ DB_EVENT_REP_NEWMASTER, "newmaster" },
		{ DB_EVENT_REP_PERM_FAILED, "perm_failed" },
		{ DB_EVENT_REP_SITE_ADDED, "site_added" },
		{ DB_EVENT_REP_SITE_REMOVED, "site_removed" },
		{ DB_EVENT_REP_STARTUPDONE, "startupdone" },
		{ DB_EVENT_REP_WOULD_ROLLBACK, "would_rollback" },
		{ DB_EVENT_WRITE_FAILED, "write_failed" },
		{ DB_EVENT_NO_SUCH_EVENT, NULL }
	};
	/*
	 * Note that when this list grows to more than 32 event types, the code
	 * below (the shift operation) will be broken.
	 */ 

	if (objc > 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-clear?");
		return (TCL_ERROR);
	}
	clear = 0;
	if (objc == 3) {
		if (Tcl_GetIndexFromObj(interp, objv[2], envinfo_option_names,
		    "option", TCL_EXACT, &enc) != TCL_OK)
			return (IS_HELP(objv[2]));
		switch ((enum envinfo_options)enc) {
		case ENVINFCLEAR:
			clear = 1;
			break;
		}
	}

	if(ip->i_event_info == NULL) {
		/* Script needs "-event" in "berkdb env" cmd. */
		Tcl_SetResult(interp,
		    "event collection not enabled on this env", TCL_STATIC);
		return (TCL_ERROR);
	}

	res = Tcl_NewListObj(0, NULL);
	if ((ret = tcl_LockMutex(dbenv, ip->i_mutex)) != 0)
		return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
			"mutex lock"));
	ret = TCL_OK;
	for (i = 0; event_names[i].flag != DB_EVENT_NO_SUCH_EVENT; i++) {
		bit_flag = 1 << event_names[i].flag;
		if (FLD_ISSET(ip->i_event_info->events, bit_flag)) {
			/*
			 * Build the 2-element list "one_event", consisting of
			 * the event name and the event info.  The event info is
			 * sometimes more than a scalar value: in those cases,
			 * reuse the "myobjv" array to build the sub-list first.
			 * For this reason, set up the event info first
			 * (myobjv[1]), and only when that is done set up the
			 * event name (myobjv[0])
			 */ 
			switch (event_names[i].flag) {
			case DB_EVENT_PANIC:
				myobjv[1] = Tcl_NewIntObj(ip->
				    i_event_info->panic_error);
				break;
			case DB_EVENT_REG_ALIVE:
				myobjv[1] = Tcl_NewLongObj((long)ip->
				    i_event_info->attached_process);
				break;
			case DB_EVENT_REP_CONNECT_BROKEN:
				myobjv[0] = Tcl_NewIntObj(ip->
				    i_event_info->conn_broken_info.eid);
				myobjv[1] = Tcl_NewIntObj(ip->
				    i_event_info->conn_broken_info.error);
				myobjv[1] = Tcl_NewListObj(2, myobjv);
				break;
			case DB_EVENT_REP_CONNECT_ESTD:
				myobjv[1] = Tcl_NewIntObj(ip->
				    i_event_info->connected_eid);
				break;
			case DB_EVENT_REP_CONNECT_TRY_FAILED:
				myobjv[0] = Tcl_NewIntObj(ip->
				    i_event_info->conn_failed_try_info.eid);
				myobjv[1] = Tcl_NewIntObj(ip->
				    i_event_info->conn_failed_try_info.error);
				myobjv[1] = Tcl_NewListObj(2, myobjv);
				break;
			case DB_EVENT_REP_NEWMASTER:
				myobjv[1] = Tcl_NewIntObj(ip->
				    i_event_info->newmaster_eid);
				break;
			case DB_EVENT_REP_SITE_ADDED:
				myobjv[1] = Tcl_NewIntObj(ip->
				    i_event_info->added_eid);
				break;
			case DB_EVENT_REP_SITE_REMOVED:
				myobjv[1] = Tcl_NewIntObj(ip->
				    i_event_info->removed_eid);
				break;
			case DB_EVENT_REP_WOULD_ROLLBACK:
				lsn = &ip->i_event_info->sync_point;
				lsnlist[0] = Tcl_NewLongObj((long)lsn->file);
				lsnlist[1] = Tcl_NewLongObj((long)lsn->offset);
				myobjv[1] = Tcl_NewListObj(2, lsnlist);
				break;
			default:
				myobjv[1] = NewStringObj("", 0);
				break;
			}
			myobjv[0] = NewStringObj(event_names[i].name,
			    strlen(event_names[i].name));

			one_event = Tcl_NewListObj(2, myobjv);
			if ((ret = Tcl_ListObjAppendElement(interp,
			    res, one_event)) != TCL_OK)
				break;
		}
	}

	/*
	 * Here, either everything is OK, or Tcl_ListObjAppendElement failed,
	 * above.  Regardless, we need to make sure we unlock the mutex.  Then,
	 * if either operation generated an error, return it, giving precedence
	 * to the earlier-occurring one.
	 */
	t_ret = tcl_UnlockMutex(dbenv, ip->i_mutex);
	if (ret != TCL_OK)
		return (ret);
	if (t_ret != 0)
		return (_ReturnSetup(interp, t_ret, DB_RETOK_STD(t_ret),
			"mutex unlock"));
	Tcl_SetObjResult(interp, res);

	if (clear)
		ip->i_event_info->events = 0;
	return (TCL_OK);
}

/*
 * PUBLIC: int tcl_EnvSetFlags __P((Tcl_Interp *, DB_ENV *, Tcl_Obj *,
 * PUBLIC:    Tcl_Obj *));
 *
 * tcl_EnvSetFlags --
 *	Set flags in an env.
 */
int
tcl_EnvSetFlags(interp, dbenv, which, onoff)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Env pointer */
	Tcl_Obj *which;			/* Which subsystem */
	Tcl_Obj *onoff;			/* On or off */
{
	static const char *sfwhich[] = {
		"-auto_commit",
		"-direct_db",
		"-dsync_db",
		"-multiversion",
		"-nolock",
		"-nommap",
		"-nopanic",
		"-nosync",
		"-overwrite",
		"-panic",
		"-snapshot",
		"-time_notgranted",
		"-wrnosync",
		"-hotbackup_in_progress",
		NULL
	};
	enum sfwhich {
		ENVSF_AUTOCOMMIT,
		ENVSF_DIRECTDB,
		ENVSF_DSYNCDB,
		ENVSF_MULTIVERSION,
		ENVSF_NOLOCK,
		ENVSF_NOMMAP,
		ENVSF_NOPANIC,
		ENVSF_NOSYNC,
		ENVSF_OVERWRITE,
		ENVSF_PANIC,
		ENVSF_SNAPSHOT,
		ENVSF_TIME_NOTGRANTED,
		ENVSF_WRNOSYNC,
		ENVSF_HOTBACKUP_IN_PROGRESS
	};
	static const char *sfonoff[] = {
		"off",
		"on",
		NULL
	};
	enum sfonoff {
		ENVSF_OFF,
		ENVSF_ON
	};
	int on, optindex, ret;
	u_int32_t wh;

	if (Tcl_GetIndexFromObj(interp, which, sfwhich, "option",
	    TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(which));

	switch ((enum sfwhich)optindex) {
	case ENVSF_AUTOCOMMIT:
		wh = DB_AUTO_COMMIT;
		break;
	case ENVSF_DIRECTDB:
		wh = DB_DIRECT_DB;
		break;
	case ENVSF_DSYNCDB:
		wh = DB_DSYNC_DB;
		break;
	case ENVSF_MULTIVERSION:
		wh = DB_MULTIVERSION;
		break;
	case ENVSF_NOLOCK:
		wh = DB_NOLOCKING;
		break;
	case ENVSF_NOMMAP:
		wh = DB_NOMMAP;
		break;
	case ENVSF_NOSYNC:
		wh = DB_TXN_NOSYNC;
		break;
	case ENVSF_NOPANIC:
		wh = DB_NOPANIC;
		break;
	case ENVSF_PANIC:
		wh = DB_PANIC_ENVIRONMENT;
		break;
	case ENVSF_OVERWRITE:
		wh = DB_OVERWRITE;
		break;
	case ENVSF_SNAPSHOT:
		wh = DB_TXN_SNAPSHOT;
		break;
	case ENVSF_TIME_NOTGRANTED:
		wh = DB_TIME_NOTGRANTED;
		break;
	case ENVSF_WRNOSYNC:
		wh = DB_TXN_WRITE_NOSYNC;
		break;
	case ENVSF_HOTBACKUP_IN_PROGRESS:
		wh = DB_HOTBACKUP_IN_PROGRESS;
		break;
	default:
		return (TCL_ERROR);
	}
	if (Tcl_GetIndexFromObj(interp, onoff, sfonoff, "option",
	    TCL_EXACT, &optindex) != TCL_OK)
		return (IS_HELP(onoff));
	switch ((enum sfonoff)optindex) {
	case ENVSF_OFF:
		on = 0;
		break;
	case ENVSF_ON:
		on = 1;
		break;
	default:
		return (TCL_ERROR);
	}
	ret = dbenv->set_flags(dbenv, wh, on);
	return (_ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env set flags"));
}

/*
 * tcl_EnvTest --
 *	The "$env test ..." command is a sort of catch-all for any sort of
 * desired test hook manipulation.  The "abort", "check" and "copy" subcommands
 * all set one or another certain location in the DB_ENV handle to a specific
 * value.  (In the case of "check", the value is an integer passed in with the
 * command itself.  For the other two, the "value" is a predefined enum
 * constant, specified by name.)
 *	The "$env test force ..." subcommand invokes other, more arbitrary
 * manipulations.
 *	Although these functions may not all seem closely related, putting them
 * all under the name "test" has the aesthetic appeal of keeping the rest of the
 * API clean.
 *
 * PUBLIC: int tcl_EnvTest __P((Tcl_Interp *, int, Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_EnvTest(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Env pointer */
{
	static const char *envtestcmd[] = {
		"abort",
		"check",
		"copy",
		"force",
		NULL
	};
	enum envtestcmd {
		ENVTEST_ABORT,
		ENVTEST_CHECK,
		ENVTEST_COPY,
		ENVTEST_FORCE
	};
	static const char *envtestat[] = {
		"electinit",
		"electvote1",
		"no_pages",
		"none",
		"predestroy",
		"preopen",
		"postdestroy",
		"postlog",
		"postlogmeta",
		"postopen",
		"postsync",
		"repmgr_perm",
		"subdb_lock",
		NULL
	};
	enum envtestat {
		ENVTEST_ELECTINIT,
		ENVTEST_ELECTVOTE1,
		ENVTEST_NO_PAGES,
		ENVTEST_NONE,
		ENVTEST_PREDESTROY,
		ENVTEST_PREOPEN,
		ENVTEST_POSTDESTROY,
		ENVTEST_POSTLOG,
		ENVTEST_POSTLOGMETA,
		ENVTEST_POSTOPEN,
		ENVTEST_POSTSYNC,
		ENVTEST_REPMGR_PERM,
		ENVTEST_SUBDB_LOCKS
	};
	static const char *envtestforce[] = {
		"noarchive_timeout",
		NULL
	};
	enum envtestforce {
		ENVTEST_NOARCHIVE_TIMEOUT
	};
	ENV *env;
	int *loc, optindex, result, testval;

	env = dbenv->env;
	result = TCL_OK;
	loc = NULL;

	if (objc != 4) {
		Tcl_WrongNumArgs(interp,
		    2, objv, "abort|check|copy|force <args>");
		return (TCL_ERROR);
	}

	/*
	 * This must be the "check", "copy" or "abort" portion of the command.
	 */
	if (Tcl_GetIndexFromObj(interp, objv[2], envtestcmd, "command",
	    TCL_EXACT, &optindex) != TCL_OK) {
		result = IS_HELP(objv[2]);
		return (result);
	}
	switch ((enum envtestcmd)optindex) {
	case ENVTEST_ABORT:
		loc = &env->test_abort;
		break;
	case ENVTEST_CHECK:
		loc = &env->test_check;
		if (Tcl_GetIntFromObj(interp, objv[3], &testval) != TCL_OK) {
			result = IS_HELP(objv[3]);
			return (result);
		}
		goto done;
	case ENVTEST_COPY:
		loc = &env->test_copy;
		break;
	case ENVTEST_FORCE:
		if (Tcl_GetIndexFromObj(interp, objv[3], envtestforce, "arg",
			TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[3]);
			return (result);
		}
		/*
		 * In the future we might add more, and then we'd use a switch
		 * statement.
		 */
		DB_ASSERT(env,
		    (enum envtestforce)optindex == ENVTEST_NOARCHIVE_TIMEOUT);
		return (tcl_RepNoarchiveTimeout(interp, dbenv));
	default:
		Tcl_SetResult(interp, "Illegal store location", TCL_STATIC);
		return (TCL_ERROR);
	}

	/*
	 * This must be the location portion of the command.
	 */
	if (Tcl_GetIndexFromObj(interp, objv[3], envtestat, "location",
	    TCL_EXACT, &optindex) != TCL_OK) {
		result = IS_HELP(objv[3]);
		return (result);
	}
	switch ((enum envtestat)optindex) {
	case ENVTEST_ELECTINIT:
		DB_ASSERT(env, loc == &env->test_abort);
		testval = DB_TEST_ELECTINIT;
		break;
	case ENVTEST_ELECTVOTE1:
		DB_ASSERT(env, loc == &env->test_abort);
		testval = DB_TEST_ELECTVOTE1;
		break;
	case ENVTEST_NO_PAGES:
		DB_ASSERT(env, loc == &env->test_abort);
		testval = DB_TEST_NO_PAGES;
		break;
	case ENVTEST_NONE:
		testval = 0;
		break;
	case ENVTEST_PREOPEN:
		testval = DB_TEST_PREOPEN;
		break;
	case ENVTEST_PREDESTROY:
		testval = DB_TEST_PREDESTROY;
		break;
	case ENVTEST_POSTLOG:
		testval = DB_TEST_POSTLOG;
		break;
	case ENVTEST_POSTLOGMETA:
		testval = DB_TEST_POSTLOGMETA;
		break;
	case ENVTEST_POSTOPEN:
		testval = DB_TEST_POSTOPEN;
		break;
	case ENVTEST_POSTDESTROY:
		testval = DB_TEST_POSTDESTROY;
		break;
	case ENVTEST_POSTSYNC:
		testval = DB_TEST_POSTSYNC;
		break;
	case ENVTEST_REPMGR_PERM:
		DB_ASSERT(env, loc == &env->test_abort);
		testval = DB_TEST_REPMGR_PERM;
		break;
	case ENVTEST_SUBDB_LOCKS:
		DB_ASSERT(env, loc == &env->test_abort);
		testval = DB_TEST_SUBDB_LOCKS;
		break;
	default:
		Tcl_SetResult(interp, "Illegal test location", TCL_STATIC);
		return (TCL_ERROR);
	}
done:
	*loc = testval;
	Tcl_SetResult(interp, "0", TCL_STATIC);
	return (result);
}
#endif

/*
 * env_DbRemove --
 *	Implements the ENV->dbremove command.
 */
static int
env_DbRemove(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	static const char *envdbrem[] = {
		"-auto_commit",
		"-notdurable",
		"-txn",
		"--",
		NULL
	};
	enum envdbrem {
		TCL_EDBREM_COMMIT,
		TCL_EDBREM_NOTDURABLE,
		TCL_EDBREM_TXN,
		TCL_EDBREM_ENDARG
	};
	DB_TXN *txn;
	u_int32_t flag;
	int endarg, i, optindex, result, ret, subdblen;
	u_char *subdbtmp;
	char *arg, *db, *dbr, *subdb, *subdbr, msg[MSG_SIZE];
	size_t nlen;

	txn = NULL;
	result = TCL_OK;
	subdbtmp = NULL;
	db = dbr = subdb = subdbr = NULL;
	endarg = subdblen = 0;
	flag = 0;
	nlen = 0;

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?args? filename ?database?");
		return (TCL_ERROR);
	}

	/*
	 * We must first parse for the environment flag, since that
	 * is needed for db_create.  Then create the db handle.
	 */
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], envdbrem,
		    "option", TCL_EXACT, &optindex) != TCL_OK) {
			arg = Tcl_GetStringFromObj(objv[i], NULL);
			if (arg[0] == '-') {
				result = IS_HELP(objv[i]);
				goto error;
			} else
				Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum envdbrem)optindex) {
		case TCL_EDBREM_COMMIT:
			flag |= DB_AUTO_COMMIT;
			break;
		case TCL_EDBREM_TXN:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "env dbremove: Invalid txn %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				return (TCL_ERROR);
			}
			break;
		case TCL_EDBREM_ENDARG:
			endarg = 1;
			break;
		case TCL_EDBREM_NOTDURABLE:
			flag |= DB_TXN_NOT_DURABLE;
			break;
		}
		/*
		 * If, at any time, parsing the args we get an error,
		 * bail out and return.
		 */
		if (result != TCL_OK)
			goto error;
		if (endarg)
			break;
	}
	if (result != TCL_OK)
		goto error;
	/*
	 * Any args we have left, (better be 1 or 2 left) are
	 * file names. If there is 1, a db name, if 2 a db and subdb name.
	 */
	if ((i != (objc - 1)) || (i != (objc - 2))) {
		/*
		 * Dbs must be NULL terminated file names, but subdbs can
		 * be anything.  Use Strings for the db name and byte
		 * arrays for the subdb.
		 */
		db = Tcl_GetStringFromObj(objv[i++], NULL);
		if (strcmp(db, "") == 0)
			db = NULL;
		if (i != objc) {
			subdbtmp =
			    Tcl_GetByteArrayFromObj(objv[i++], &subdblen);
			if ((ret = __os_malloc(
			    dbenv->env, (size_t)subdblen + 1, &subdb)) != 0) {
				Tcl_SetResult(interp,
				    db_strerror(ret), TCL_STATIC);
				return (0);
			}
			memcpy(subdb, subdbtmp, (size_t)subdblen);
			subdb[subdblen] = '\0';
		}
	} else {
		Tcl_WrongNumArgs(interp, 2, objv, "?args? filename ?database?");
		result = TCL_ERROR;
		goto error;
	}
	ret = dbenv->dbremove(dbenv, txn, db, subdb, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env dbremove");

	/* 
	 * If we are heap, we have auxiliary dbs.  However in order to tell if
	 * we are heap we have to open the db to get its type.  So we will just
	 * try the dbremove and ignore ENOENT.
	 */
	if ((db != NULL || subdb != NULL) && ret == 0) {
		/* Set up file name for associated recno db for heap. */
		if (db != NULL) {
			nlen = strlen(db);
			if ((ret = __os_malloc(
			    dbenv->env, nlen + 2, &dbr)) != 0) {
				Tcl_SetResult(
				    interp, db_strerror(ret), TCL_STATIC);
				return (0);
			}
			memcpy(dbr, db, nlen);
			dbr[nlen] = '1';
			dbr[nlen+1] = '\0';
		}

		if (subdb != NULL) {
			if ((ret = __os_malloc(
			    dbenv->env, (size_t)subdblen + 2, &subdbr)) != 0) {
				Tcl_SetResult(
				    interp, db_strerror(ret), TCL_STATIC);
				return (0);
			}
			memcpy(subdbr, subdb, (size_t)subdblen);
			subdbr[subdblen] = '1';
			subdbr[subdblen+1] = '\0';
		}


		ret = dbenv->dbremove(dbenv, txn, dbr, subdbr, flag);
		if (ret == ENOENT)
			ret = 0;
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db remove associated recno");
		if (ret)
			goto error;

		/* Remove the btree which is doing the RID to recno mapping*/
		if (dbr != NULL)
			dbr[nlen] = '2';
		if (subdbr != NULL)
			subdbr[subdblen] = '2';
		ret = dbenv->dbremove(dbenv, txn, dbr, subdbr, flag);
		if (ret == ENOENT)
			ret = 0;
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db remove associated btree");
	}
error:
	if (subdb)
		__os_free(dbenv->env, subdb);
	if (subdbr)
		__os_free(dbenv->env, subdbr);
	if (dbr)
		__os_free(dbenv->env, dbr);
	return (result);
}

/*
 * env_DbRename --
 *	Implements the ENV->dbrename command.
 */
static int
env_DbRename(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	static const char *envdbmv[] = {
		"-auto_commit",
		"-txn",
		"--",
		NULL
	};
	enum envdbmv {
		TCL_EDBMV_COMMIT,
		TCL_EDBMV_TXN,
		TCL_EDBMV_ENDARG
	};
	DB_TXN *txn;
	u_int32_t flag;
	int endarg, i, newlen, optindex, result, ret, subdblen;
	u_char *subdbtmp;
	char *arg, *db, *dbr, *newname, *newnamer, *subdb, *subdbr;
	char msg[MSG_SIZE];
	size_t nlen;

	txn = NULL;
	result = TCL_OK;
	subdbtmp = NULL;
	db = dbr = newname = newnamer = subdb = subdbr = NULL;
	endarg = subdblen = 0;
	flag = 0;
	nlen = 0;

	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 3, objv,
		    "?args? filename ?database? ?newname?");
		return (TCL_ERROR);
	}

	/*
	 * We must first parse for the environment flag, since that
	 * is needed for db_create.  Then create the db handle.
	 */
	i = 2;
	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], envdbmv,
		    "option", TCL_EXACT, &optindex) != TCL_OK) {
			arg = Tcl_GetStringFromObj(objv[i], NULL);
			if (arg[0] == '-') {
				result = IS_HELP(objv[i]);
				goto error;
			} else
				Tcl_ResetResult(interp);
			break;
		}
		i++;
		switch ((enum envdbmv)optindex) {
		case TCL_EDBMV_COMMIT:
			flag |= DB_AUTO_COMMIT;
			break;
		case TCL_EDBMV_TXN:
			if (i >= objc) {
				Tcl_WrongNumArgs(interp, 2, objv, "?-txn id?");
				result = TCL_ERROR;
				break;
			}
			arg = Tcl_GetStringFromObj(objv[i++], NULL);
			txn = NAME_TO_TXN(arg);
			if (txn == NULL) {
				snprintf(msg, MSG_SIZE,
				    "env dbrename: Invalid txn %s\n", arg);
				Tcl_SetResult(interp, msg, TCL_VOLATILE);
				return (TCL_ERROR);
			}
			break;
		case TCL_EDBMV_ENDARG:
			endarg = 1;
			break;
		}
		/*
		 * If, at any time, parsing the args we get an error,
		 * bail out and return.
		 */
		if (result != TCL_OK)
			goto error;
		if (endarg)
			break;
	}
	if (result != TCL_OK)
		goto error;
	/*
	 * Any args we have left, (better be 2 or 3 left) are
	 * file names. If there is 2, a db name, if 3 a db and subdb name.
	 */
	if ((i != (objc - 2)) || (i != (objc - 3))) {
		/*
		 * Dbs must be NULL terminated file names, but subdbs can
		 * be anything.  Use Strings for the db name and byte
		 * arrays for the subdb.
		 */
		db = Tcl_GetStringFromObj(objv[i++], NULL);
		if (strcmp(db, "") == 0)
			db = NULL;
		if (i == objc - 2) {
			subdbtmp =
			    Tcl_GetByteArrayFromObj(objv[i++], &subdblen);
			if ((ret = __os_malloc(
			    dbenv->env, (size_t)subdblen + 1, &subdb)) != 0) {
				Tcl_SetResult(interp,
				    db_strerror(ret), TCL_STATIC);
				return (0);
			}
			memcpy(subdb, subdbtmp, (size_t)subdblen);
			subdb[subdblen] = '\0';
		}
		subdbtmp = Tcl_GetByteArrayFromObj(objv[i++], &newlen);
		if ((ret = __os_malloc(
		    dbenv->env, (size_t)newlen + 1, &newname)) != 0) {
			Tcl_SetResult(interp,
			    db_strerror(ret), TCL_STATIC);
			return (0);
		}
		memcpy(newname, subdbtmp, (size_t)newlen);
		newname[newlen] = '\0';
	} else {
		Tcl_WrongNumArgs(interp, 3, objv,
		    "?args? filename ?database? ?newname?");
		result = TCL_ERROR;
		goto error;
	}
	ret = dbenv->dbrename(dbenv, txn, db, subdb, newname, flag);
	result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env dbrename");

	/* 
	 * If we are heap, we have auxiliary dbs.  However in order to tell if
	 * we are heap we have to open the db to get its type.  So we will just
	 * try the dbrename and ignore ENOENT.
	 */
	if ((db != NULL || subdb != NULL) && ret == 0) {
		/* Set up file name for associated recno db for heap. */
		if (db != NULL) {
			nlen = strlen(db);
			if ((ret = __os_malloc(
			    dbenv->env, nlen + 2, &dbr)) != 0) {
				Tcl_SetResult(
				    interp, db_strerror(ret), TCL_STATIC);
				return (0);
			}
			memcpy(dbr, db, nlen);
			dbr[nlen] = '1';
			dbr[nlen+1] = '\0';
		}

		if (subdb != NULL) {
			if ((ret = __os_malloc(
			    dbenv->env, (size_t)subdblen + 2, &subdbr)) != 0) {
				Tcl_SetResult(
				    interp, db_strerror(ret), TCL_STATIC);
				return (0);
			}
			memcpy(subdbr, subdb, (size_t)subdblen);
			subdbr[subdblen] = '1';
			subdbr[subdblen+1] = '\0';
		}

		if ((ret = __os_malloc(dbenv->env, 
		    (size_t)newlen + 2, &newnamer)) != 0) {
			Tcl_SetResult(interp, db_strerror(ret), TCL_STATIC);
			return (0);
		}
		memcpy(newnamer, newname, (size_t)newlen);
		newnamer[newlen] = '1';
		newnamer[newlen+1] = '\0';

		ret = dbenv->dbrename(dbenv, txn, dbr, subdbr, newnamer, flag);
		if (ret == ENOENT)
			ret = 0;
		
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db rename associated recno");
		if (ret != 0)
			goto error;

		/* Rename the btree which is doing the RID to recno mapping*/
		if (dbr != NULL)
			dbr[nlen] = '2';
		if (subdbr != NULL)
			subdbr[subdblen] = '2';
		newnamer[newlen] = '2';
		ret = dbenv->dbrename(dbenv, txn, dbr, subdbr, newnamer, flag);
		if (ret == ENOENT)
			ret = 0;
		result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
		    "db rename associated btree");
	}


error:
	if (subdb)
		__os_free(dbenv->env, subdb);
	if (newname)
		__os_free(dbenv->env, newname);
	if (dbr)
		__os_free(dbenv->env, dbr);
	if (subdbr)
		__os_free(dbenv->env, subdbr);
	if (newnamer)
		__os_free(dbenv->env, newnamer);
	return (result);
}

/*
 * env_GetFlags --
 *	Implements the ENV->get_flags command.
 */
static int
env_GetFlags(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	int i, ret, result;
	u_int32_t flags;
	char buf[512];
	Tcl_Obj *res;

	static const struct {
		u_int32_t flag;
		char *arg;
	} open_flags[] = {
		{ DB_AUTO_COMMIT, "-auto_commit" },
		{ DB_CDB_ALLDB, "-cdb_alldb" },
		{ DB_DIRECT_DB, "-direct_db" },
		{ DB_DSYNC_DB, "-dsync_db" },
		{ DB_MULTIVERSION, "-multiversion" },
		{ DB_NOLOCKING, "-nolock" },
		{ DB_NOMMAP, "-nommap" },
		{ DB_NOPANIC, "-nopanic" },
		{ DB_OVERWRITE, "-overwrite" },
		{ DB_PANIC_ENVIRONMENT, "-panic" },
		{ DB_REGION_INIT, "-region_init" },
		{ DB_TIME_NOTGRANTED, "-time_notgranted" },
		{ DB_TXN_NOSYNC, "-nosync" },
		{ DB_TXN_NOWAIT, "-nowait" },
		{ DB_TXN_SNAPSHOT, "-snapshot" },
		{ DB_TXN_WRITE_NOSYNC, "-wrnosync" },
		{ DB_YIELDCPU, "-yield" },
		{ DB_HOTBACKUP_IN_PROGRESS, "-hotbackup_in_progress" },
		{ 0, NULL }
	};

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}

	ret = dbenv->get_flags(dbenv, &flags);
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env get_flags")) == TCL_OK) {
		buf[0] = '\0';

		for (i = 0; open_flags[i].flag != 0; i++)
			if (LF_ISSET(open_flags[i].flag)) {
				if (strlen(buf) > 0)
					(void)strncat(buf, " ", sizeof(buf));
				(void)strncat(
				    buf, open_flags[i].arg, sizeof(buf) - 1);
			}

		res = NewStringObj(buf, strlen(buf));
		Tcl_SetObjResult(interp, res);
	}

	return (result);
}

/*
 * env_GetOpenFlag --
 *	Implements the ENV->get_open_flags command.
 */
static int
env_GetOpenFlag(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	int i, ret, result;
	u_int32_t flags;
	char buf[512];
	Tcl_Obj *res;

	static const struct {
		u_int32_t flag;
		char *arg;
	} open_flags[] = {
		{ DB_CREATE, "-create" },
		{ DB_FAILCHK, "-failchk" },
		{ DB_INIT_CDB, "-cdb" },
		{ DB_INIT_LOCK, "-lock" },
		{ DB_INIT_LOG, "-log" },
		{ DB_INIT_MPOOL, "-mpool" },
		{ DB_INIT_REP, "-rep" },
		{ DB_INIT_TXN, "-txn" },
		{ DB_LOCKDOWN, "-lockdown" },
		{ DB_PRIVATE, "-private" },
		{ DB_RECOVER, "-recover" },
		{ DB_RECOVER_FATAL, "-recover_fatal" },
		{ DB_REGISTER, "-register" },
		{ DB_FAILCHK, "-failchk" },
		{ DB_SYSTEM_MEM, "-system_mem" },
		{ DB_THREAD, "-thread" },
		{ DB_USE_ENVIRON, "-use_environ" },
		{ DB_USE_ENVIRON_ROOT, "-use_environ_root" },
		{ 0, NULL }
	};

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}

	ret = dbenv->get_open_flags(dbenv, &flags);
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env get_open_flags")) == TCL_OK) {
		buf[0] = '\0';

		for (i = 0; open_flags[i].flag != 0; i++)
			if (LF_ISSET(open_flags[i].flag)) {
				if (strlen(buf) > 0)
					(void)strncat(buf, " ", sizeof(buf));
				(void)strncat(
				    buf, open_flags[i].arg, sizeof(buf) - 1);
			}

		res = NewStringObj(buf, strlen(buf));
		Tcl_SetObjResult(interp, res);
	}

	return (result);
}

/*
 * PUBLIC: int tcl_EnvGetEncryptFlags __P((Tcl_Interp *, int, Tcl_Obj * CONST*,
 * PUBLIC:      DB_ENV *));
 *
 * tcl_EnvGetEncryptFlags --
 *	Implements the ENV->get_encrypt_flags command.
 */
int
tcl_EnvGetEncryptFlags(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Database pointer */
{
	int i, ret, result;
	u_int32_t flags;
	char buf[512];
	Tcl_Obj *res;

	static const struct {
		u_int32_t flag;
		char *arg;
	} encrypt_flags[] = {
		{ DB_ENCRYPT_AES, "-encryptaes" },
		{ 0, NULL }
	};

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}

	ret = dbenv->get_encrypt_flags(dbenv, &flags);
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env get_encrypt_flags")) == TCL_OK) {
		buf[0] = '\0';

		for (i = 0; encrypt_flags[i].flag != 0; i++)
			if (LF_ISSET(encrypt_flags[i].flag)) {
				if (strlen(buf) > 0)
					(void)strncat(buf, " ", sizeof(buf));
				(void)strncat(
				    buf, encrypt_flags[i].arg, sizeof(buf) -1);
			}

		res = NewStringObj(buf, strlen(buf));
		Tcl_SetObjResult(interp, res);
	}

	return (result);
}

/*
 * env_GetLockDetect --
 *	Implements the ENV->get_lk_detect command.
 */
static int
env_GetLockDetect(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	int i, ret, result;
	u_int32_t lk_detect;
	const char *answer;
	Tcl_Obj *res;
	static const struct {
		u_int32_t flag;
		char *name;
	} lk_detect_returns[] = {
		{ DB_LOCK_DEFAULT, "default" },
		{ DB_LOCK_EXPIRE, "expire" },
		{ DB_LOCK_MAXLOCKS, "maxlocks" },
		{ DB_LOCK_MAXWRITE, "maxwrite" },
		{ DB_LOCK_MINLOCKS, "minlocks" },
		{ DB_LOCK_MINWRITE, "minwrite" },
		{ DB_LOCK_OLDEST, "oldest" },
		{ DB_LOCK_RANDOM, "random" },
		{ DB_LOCK_YOUNGEST, "youngest" },
		{ 0, NULL }
	};

	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}
	ret = dbenv->get_lk_detect(dbenv, &lk_detect);
	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env get_lk_detect")) == TCL_OK) {
		answer = "unknown";
		for (i = 0; lk_detect_returns[i].flag != 0; i++)
			if (lk_detect == lk_detect_returns[i].flag)
				answer = lk_detect_returns[i].name;

		res = NewStringObj(answer, strlen(answer));
		Tcl_SetObjResult(interp, res);
	}

	return (result);
}

/*
 * env_GetTimeout --
 *	Implements the ENV->get_timeout command.
 */
static int
env_GetTimeout(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	static const struct {
		u_int32_t flag;
		char *arg;
	} timeout_flags[] = {
		{ DB_SET_LOCK_TIMEOUT, "lock" },
		{ DB_SET_REG_TIMEOUT, "reg" },
		{ DB_SET_TXN_TIMEOUT, "txn" },
		{ 0, NULL }
	};
	Tcl_Obj *res;
	db_timeout_t timeout;
	u_int32_t which;
	int i, ret, result;
	const char *arg;

	COMPQUIET(timeout, 0);

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}

	arg = Tcl_GetStringFromObj(objv[2], NULL);
	which = 0;
	for (i = 0; timeout_flags[i].flag != 0; i++)
		if (strcmp(arg, timeout_flags[i].arg) == 0)
			which = timeout_flags[i].flag;
	if (which == 0) {
		ret = EINVAL;
		goto err;
	}

	ret = dbenv->get_timeout(dbenv, &timeout, which);
err:	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env get_timeout")) == TCL_OK) {
		res = Tcl_NewLongObj((long)timeout);
		Tcl_SetObjResult(interp, res);
	}

	return (result);
}

/*
 * env_GetVerbose --
 *	Implements the ENV->get_open_flags command.
 */
static int
env_GetVerbose(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;
{
	static const struct {
		u_int32_t flag;
		char *arg;
	} verbose_flags[] = {
		{ DB_VERB_BACKUP, "backup" },
		{ DB_VERB_DEADLOCK, "deadlock" },
		{ DB_VERB_FILEOPS, "fileops" },
		{ DB_VERB_FILEOPS_ALL, "fileops_all" },
		{ DB_VERB_RECOVERY, "recovery" },
		{ DB_VERB_REGISTER, "register" },
		{ DB_VERB_REPLICATION, "rep" },
		{ DB_VERB_REP_ELECT, "rep_elect" },
		{ DB_VERB_REP_LEASE, "rep_lease" },
		{ DB_VERB_REP_MISC, "rep_misc" },
		{ DB_VERB_REP_MSGS, "rep_msgs" },
		{ DB_VERB_REP_SYNC, "rep_sync" },
		{ DB_VERB_REP_SYSTEM, "rep_system" },
		{ DB_VERB_REP_TEST, "rep_test" },
		{ DB_VERB_REPMGR_CONNFAIL, "repmgr_connfail" },
		{ DB_VERB_REPMGR_MISC, "repmgr_misc" },
		{ DB_VERB_WAITSFOR, "wait" },
		{ 0, NULL }
	};
	Tcl_Obj *res;
	u_int32_t which;
	int i, onoff, ret, result;
	const char *arg, *answer;

	COMPQUIET(onoff, 0);

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, NULL);
		return (TCL_ERROR);
	}

	arg = Tcl_GetStringFromObj(objv[2], NULL);
	which = 0;
	for (i = 0; verbose_flags[i].flag != 0; i++)
		if (strcmp(arg, verbose_flags[i].arg) == 0)
			which = verbose_flags[i].flag;
	if (which == 0) {
		ret = EINVAL;
		goto err;
	}

	ret = dbenv->get_verbose(dbenv, which, &onoff);
err:	if ((result = _ReturnSetup(interp, ret, DB_RETOK_STD(ret),
	    "env get_verbose")) == 0) {
		answer = onoff ? "on" : "off";
		res = NewStringObj(answer, strlen(answer));
		Tcl_SetObjResult(interp, res);
	}

	return (result);
}

/*
 * PUBLIC: void tcl_EnvSetErrfile __P((Tcl_Interp *, DB_ENV *, DBTCL_INFO *,
 * PUBLIC:    char *));
 *
 * tcl_EnvSetErrfile --
 *	Implements the ENV->set_errfile command.
 */
void
tcl_EnvSetErrfile(interp, dbenv, ip, errf)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Database pointer */
	DBTCL_INFO *ip;			/* Our internal info */
	char *errf;
{
	COMPQUIET(interp, NULL);
	/*
	 * If the user already set one, free it.
	 */
	if (ip->i_err != NULL && ip->i_err != stdout &&
	    ip->i_err != stderr)
		(void)fclose(ip->i_err);
	if (strcmp(errf, "/dev/stdout") == 0)
		ip->i_err = stdout;
	else if (strcmp(errf, "/dev/stderr") == 0)
		ip->i_err = stderr;
	else
		ip->i_err = fopen(errf, "a");
	if (ip->i_err != NULL)
		dbenv->set_errfile(dbenv, ip->i_err);
}

/*
 * PUBLIC: void tcl_EnvSetMsgfile __P((Tcl_Interp *, DB_ENV *, DBTCL_INFO *,
 * PUBLIC:    char *));
 *
 * tcl_EnvSetMsgfile --
 *	Implements the ENV->set_msgfile command.
 */
void
tcl_EnvSetMsgfile(interp, dbenv, ip, msgf)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Database pointer */
	DBTCL_INFO *ip;			/* Our internal info */
	char *msgf;
{
	COMPQUIET(interp, NULL);
	/*
	 * If the user already set one, free it.
	 */
	if (ip->i_msg != NULL && ip->i_msg != stdout &&
	    ip->i_msg != stderr)
		(void)fclose(ip->i_msg);
	if (strcmp(msgf, "/dev/stdout") == 0)
		ip->i_msg = stdout;
	else if (strcmp(msgf, "/dev/stderr") == 0)
		ip->i_msg = stderr;
	else
		ip->i_msg = fopen(msgf, "a");
	if (ip->i_msg != NULL)
		dbenv->set_msgfile(dbenv, ip->i_msg);
}

/*
 * PUBLIC: int tcl_EnvSetErrpfx __P((Tcl_Interp *, DB_ENV *, DBTCL_INFO *,
 * PUBLIC:    char *));
 *
 * tcl_EnvSetErrpfx --
 *	Implements the ENV->set_errpfx command.
 */
int
tcl_EnvSetErrpfx(interp, dbenv, ip, pfx)
	Tcl_Interp *interp;		/* Interpreter */
	DB_ENV *dbenv;			/* Database pointer */
	DBTCL_INFO *ip;			/* Our internal info */
	char *pfx;
{
	int result, ret;

	/*
	 * Assume success.  The only thing that can fail is
	 * the __os_strdup.
	 */
	result = TCL_OK;
	Tcl_SetResult(interp, "0", TCL_STATIC);
	/*
	 * If the user already set one, free it.
	 */
	if (ip->i_errpfx != NULL) {
		dbenv->set_errpfx(dbenv, NULL);
		__os_free(dbenv->env, ip->i_errpfx);
		ip->i_errpfx = NULL;
	}
	if ((ret = __os_strdup(dbenv->env, pfx, &ip->i_errpfx)) != 0) {
		result = _ReturnSetup(interp, ret,
		    DB_RETOK_STD(ret), "__os_strdup");
		ip->i_errpfx = NULL;
	}
	if (ip->i_errpfx != NULL)
		dbenv->set_errpfx(dbenv, ip->i_errpfx);
	return (result);
}

/*
 * tcl_EnvStatPrint --
 *
 * PUBLIC: int tcl_EnvStatPrint __P((Tcl_Interp *, int,
 * PUBLIC:    Tcl_Obj * CONST*, DB_ENV *));
 */
int
tcl_EnvStatPrint(interp, objc, objv, dbenv)
	Tcl_Interp *interp;		/* Interpreter */
	int objc;			/* How many arguments? */
	Tcl_Obj *CONST objv[];		/* The argument objects */
	DB_ENV *dbenv;			/* Environment pointer */
{	
	static const char *envstatprtopts[] = {
		"-all",
		"-clear",
		"-subsystem",
		 NULL
	};
	enum envstatprtopts {
		ENVSTATPRTALL,
		ENVSTATPRTCLEAR,
		ENVSTATPRTSUB
	};
	u_int32_t flag;
	int i, optindex, result, ret;

	result = TCL_OK;
	flag = 0;
	i = 2;

	while (i < objc) {
		if (Tcl_GetIndexFromObj(interp, objv[i], envstatprtopts, 
		    "option", TCL_EXACT, &optindex) != TCL_OK) {
			result = IS_HELP(objv[i]);
			goto error;
		}
		i++;
		switch ((enum envstatprtopts)optindex) {
		case ENVSTATPRTALL:
			flag |= DB_STAT_ALL;
			break;
		case ENVSTATPRTCLEAR:
			flag |= DB_STAT_CLEAR;
			break;
		case ENVSTATPRTSUB:
			flag |= DB_STAT_SUBSYSTEM;
			break;
		}
		if (result != TCL_OK)
			break;
	}
	if (result != TCL_OK)
		goto error;

	_debug_check();
	ret = dbenv->stat_print(dbenv, flag);
	result = _ReturnSetup(interp, 
	    ret, DB_RETOK_STD(ret), "dbenv stat_print");
error:
	return (result);

}
