/* Callbacks */
%define JAVA_CALLBACK(_sig, _jclass, _name)
JAVA_TYPEMAP(_sig, _jclass, jboolean)
%typemap(jtype) _sig "boolean"
%typemap(javain) _sig %{ (_name##_handler = $javainput) != null %}

/*
 * The Java object is stored in the Db or DbEnv class.
 * Here we only care whether it is non-NULL.
 */
%typemap(in) _sig %{
	$1 = ($input == JNI_TRUE) ? __dbj_##_name : NULL;
%}
%enddef

%{
static void __dbj_error(const DB_ENV *dbenv,
    const char *prefix, const char *msg)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);
	jobject jmsg;

	COMPQUIET(prefix, NULL);

	if (jdbenv != NULL){
		jmsg = (*jenv)->NewStringUTF(jenv, msg);
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv, dbenv_class,
		    errcall_method, jmsg);
		(*jenv)->DeleteLocalRef(jenv, jmsg);
	}

	if (detach)
		__dbj_detach();
}

static void __dbj_env_feedback(DB_ENV *dbenv, int opcode, int percent)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);

	if (jdbenv != NULL)
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv, dbenv_class,
		    env_feedback_method, opcode, percent);

	if (detach)
		__dbj_detach();
}

static void __dbj_message(const DB_ENV *dbenv, const char *msg)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);
	jobject jmsg;

	if (jdbenv != NULL){
		jmsg = (*jenv)->NewStringUTF(jenv, msg);
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv, dbenv_class,
		    msgcall_method, jmsg);
		(*jenv)->DeleteLocalRef(jenv, jmsg);
	}

	if (detach)
		__dbj_detach();
}

static void __dbj_repmgr_msg_dispatch(DB_ENV *dbenv, DB_CHANNEL *chan, DBT *msgs, u_int32_t nmsgs, u_int32_t flags)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);
	jobject jdbt;
	jobjectArray jmsgs;
	jbyteArray jdbtarr;
	u_int32_t i;

	if (jdbenv == NULL)
		return;
	
	jmsgs = (*jenv)->NewObjectArray(jenv, nmsgs, dbt_class, NULL);
	if (jmsgs == NULL)
		goto err;
	for (i = 0; i < nmsgs; i++) {
		jdbt = (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
		__dbj_dbt_copyout(jenv, &(msgs[i]), &jdbtarr, jdbt);
		if (jdbt == NULL) {
			/* An exception is pending. */
			goto err;
		}
		(*jenv)->SetObjectArrayElement(jenv, jmsgs, i, jdbt);
	}
	
	(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv, dbenv_class, repmgr_msg_dispatch_method, chan, jmsgs, flags);
	
	(*jenv)->DeleteLocalRef(jenv, jdbt);
	(*jenv)->DeleteLocalRef(jenv, jdbtarr);
	(*jenv)->DeleteLocalRef(jenv, jmsgs);

err:	if (detach)
		__dbj_detach();
}

static void __dbj_panic(DB_ENV *dbenv, int ret)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);

	if (jdbenv != NULL)
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv, dbenv_class,
		    paniccall_method,
		    __dbj_get_except(jenv, ret, NULL, NULL, jdbenv));

	if (detach)
		__dbj_detach();
}

static int __dbj_app_dispatch(DB_ENV *dbenv,
    DBT *dbt, DB_LSN *lsn, db_recops recops)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);
	jobject jdbt, jlsn;
	jbyteArray jdbtarr;
	int ret;

	if (jdbenv == NULL) {
		ret = EINVAL;
		goto err;
	}

	jdbt = (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
	__dbj_dbt_copyout(jenv, dbt, &jdbtarr, jdbt);
	if (jdbt == NULL) {
		ret = ENOMEM; /* An exception is pending */
		goto err;
	}

	jlsn = (lsn == NULL) ? NULL : __dbj_wrap_DB_LSN(jenv, lsn);

	ret = (*jenv)->CallNonvirtualIntMethod(jenv, jdbenv, dbenv_class,
	    app_dispatch_method, jdbt, jlsn, recops);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
	}

	(*jenv)->DeleteLocalRef(jenv, jdbtarr);
	(*jenv)->DeleteLocalRef(jenv, jdbt);
	if (jlsn != NULL)
		(*jenv)->DeleteLocalRef(jenv, jlsn);

err:	if (detach)
		__dbj_detach();
	return (ret);
}

static void __dbj_event_notify(DB_ENV *dbenv, u_int32_t event_id, void * info)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);

	if (jdbenv == NULL)
		goto done;

	switch (event_id) {
	case DB_EVENT_PANIC:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, panic_event_notify_method);
		break;
	case DB_EVENT_REP_AUTOTAKEOVER_FAILED:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_autotakeover_failed_event_notify_method);
		break;
	case DB_EVENT_REP_CLIENT:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_client_event_notify_method);
		break;
	case DB_EVENT_REP_CONNECT_BROKEN:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_connect_broken_event_notify_method);
		break;
	case DB_EVENT_REP_CONNECT_ESTD:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_connect_established_event_notify_method);
		break;
	case DB_EVENT_REP_CONNECT_TRY_FAILED:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_connect_try_failed_event_notify_method);
		break;
	case DB_EVENT_REP_DUPMASTER:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_dupmaster_event_notify_method);
		break;
	case DB_EVENT_REP_ELECTED:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_elected_event_notify_method);
		break;
	case DB_EVENT_REP_ELECTION_FAILED:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_election_failed_event_notify_method);
		break;
	case DB_EVENT_REP_INIT_DONE:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_init_done_event_notify_method);
		break;
	case DB_EVENT_REP_JOIN_FAILURE:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_join_failure_event_notify_method);
		break;
	case DB_EVENT_REP_LOCAL_SITE_REMOVED:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_local_site_removed_notify_method);
		break;
	case DB_EVENT_REP_MASTER:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_master_event_notify_method);
		break;
	case DB_EVENT_REP_MASTER_FAILURE:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_master_failure_event_notify_method);
		break;
	case DB_EVENT_REP_NEWMASTER:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_new_master_event_notify_method, 
		    *(int*)info);
		break;
	case DB_EVENT_REP_PERM_FAILED:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_perm_failed_event_notify_method);
		break;
	case DB_EVENT_REP_SITE_ADDED:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_site_added_event_notify_method);
		break;
	case DB_EVENT_REP_SITE_REMOVED:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_site_removed_event_notify_method);
		break;
	case DB_EVENT_REP_STARTUPDONE:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, rep_startup_done_event_notify_method);
		break;
	case DB_EVENT_WRITE_FAILED:
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdbenv,
		    dbenv_class, write_failed_event_notify_method, 
		    *(int*)info);
		break;
	default:
                dbenv->errx(dbenv, "Unhandled event callback in the Java API");
                DB_ASSERT(dbenv->env, 0);
	}

done:	if (detach)
		__dbj_detach();
}

static int __dbj_rep_transport(DB_ENV *dbenv,
    const DBT *control, const DBT *rec, const DB_LSN *lsn, int envid,
    u_int32_t flags)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);
	jobject jcontrol, jrec, jlsn;
	jbyteArray jcontrolarr, jrecarr;
	int ret;

	if (jdbenv == NULL) {
		ret = EINVAL;
		goto err;
	}

	jcontrol = (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
	jrec = (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
	if (jcontrol == NULL || jrec == NULL) {
		ret = ENOMEM; /* An exception is pending */
		goto err;
	}

	__dbj_dbt_copyout(jenv, control, &jcontrolarr, jcontrol);
	__dbj_dbt_copyout(jenv, rec, &jrecarr, jrec);
	jlsn = (lsn == NULL) ? NULL : __dbj_wrap_DB_LSN(jenv, (DB_LSN *)lsn);

	if (jcontrolarr == NULL || jrecarr == NULL) {
		ret = ENOMEM; /* An exception is pending */
		goto err;
	}

	ret = (*jenv)->CallNonvirtualIntMethod(jenv, jdbenv, dbenv_class,
	    rep_transport_method, jcontrol, jrec, jlsn, envid, flags);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
	}

	(*jenv)->DeleteLocalRef(jenv, jrecarr);
	(*jenv)->DeleteLocalRef(jenv, jcontrolarr);
	(*jenv)->DeleteLocalRef(jenv, jrec);
	(*jenv)->DeleteLocalRef(jenv, jcontrol);
	if (jlsn != NULL)
		(*jenv)->DeleteLocalRef(jenv, jlsn);

err:	if (detach)
		__dbj_detach();
	return (ret);
}

static int __dbj_foreignkey_nullify(DB *db, 
    const DBT *key, DBT *data, const DBT *skey, int *changed)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	DBT_LOCKED lresult;
	jobject jdb = (jobject)DB_INTERNAL(db);
	jobject jkey, jdata, jskey;
	jbyteArray jkeyarr, jdataarr, jskeyarr;
	jboolean jresult;
	int ret;

	if (jdb == NULL) {
		ret = EINVAL;
		goto err;
	}

	jkey = (key->app_data != NULL) ?
	    ((DBT_LOCKED *)key->app_data)->jdbt :
	    (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
	jdata = (data->app_data != NULL) ?
	    ((DBT_LOCKED *)data->app_data)->jdbt :
	    (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
	jskey = (skey->app_data != NULL) ?
	    ((DBT_LOCKED *)skey->app_data)->jdbt :
	    (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
	if (jkey == NULL || jdata == NULL || jskey == NULL) {
		ret = ENOMEM; /* An exception is pending */
		goto err;
	}

	if (key->app_data == NULL) {
		__dbj_dbt_copyout(jenv, key, &jkeyarr, jkey);
		if (jkeyarr == NULL) {
			ret = ENOMEM; /* An exception is pending */
			goto err;
		}
	}
	if (data->app_data == NULL) {
		__dbj_dbt_copyout(jenv, data, &jdataarr, jdata);
		if (jdataarr == NULL) {
			ret = ENOMEM; /* An exception is pending */
			goto err;
		}
	}
	if (skey->app_data == NULL) {
		__dbj_dbt_copyout(jenv, skey, &jskeyarr, jskey);
		if (jskeyarr == NULL) {
			ret = ENOMEM; /* An exception is pending */
			goto err;
		}
	}

	jresult = (*jenv)->CallNonvirtualBooleanMethod(jenv, jdb, db_class, foreignkey_nullify_method, jkey, jdata, jskey);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
		goto err;
	}

	if (jresult == JNI_FALSE)
		*changed = ret = 0;
	else{
		*changed = 1;
		/* copy jdata into data */
		if ((ret = __dbj_dbt_copyin(jenv, &lresult, NULL, jdata, 0)) != 0)
			goto err;
		if (lresult.dbt.size != 0){
			data->size = lresult.dbt.size;
			if ((ret = __os_umalloc(
			    NULL, data->size, &data->data)) != 0)
				goto err;
			if ((ret = __dbj_dbt_memcopy(&lresult.dbt, 0, 
			    data->data, data->size, DB_USERCOPY_GETDATA)) != 0)
				goto err;
			__dbj_dbt_release(jenv, jdata,  &lresult.dbt, &lresult);
			(*jenv)->DeleteLocalRef(jenv, lresult.jarr);
			F_SET(data, DB_DBT_APPMALLOC);
		} 
	}			

err:	if (key->app_data == NULL) {
		(*jenv)->DeleteLocalRef(jenv, jkeyarr);
		(*jenv)->DeleteLocalRef(jenv, jkey);
	}
	if (data->app_data == NULL) {
		(*jenv)->DeleteLocalRef(jenv, jdataarr);
		(*jenv)->DeleteLocalRef(jenv, jdata);
	}

	if (detach)
		__dbj_detach();
	return ret;
}

static int __dbj_seckey_create(DB *db,
    const DBT *key, const DBT *data, DBT *result)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);
	jobject jkey, jdata, jresult;
	jobjectArray jskeys;
	jsize i, num_skeys;
	jbyteArray jkeyarr, jdataarr;
	DBT_LOCKED lresult;
	DBT *tresult;
	int ret;

	if (jdb == NULL) {
		ret = EINVAL;
		goto err;
	}

	jkey = (key->app_data != NULL) ?
	    ((DBT_LOCKED *)key->app_data)->jdbt :
	    (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
	jdata = (data->app_data != NULL) ?
	    ((DBT_LOCKED *)data->app_data)->jdbt :
	    (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
	if (jkey == NULL || jdata == NULL) {
		ret = ENOMEM; /* An exception is pending */
		goto err;
	}

	if (key->app_data == NULL) {
		__dbj_dbt_copyout(jenv, key, &jkeyarr, jkey);
		if (jkeyarr == NULL) {
			ret = ENOMEM; /* An exception is pending */
			goto err;
		}
	}
	if (data->app_data == NULL) {
		__dbj_dbt_copyout(jenv, data, &jdataarr, jdata);
		if (jdataarr == NULL) {
			ret = ENOMEM; /* An exception is pending */
			goto err;
		}
	}

	jskeys = (jobjectArray)(*jenv)->CallNonvirtualObjectMethod(jenv,
	    jdb, db_class, seckey_create_method, jkey, jdata);

	if (jskeys == NULL ||
	    (num_skeys = (*jenv)->GetArrayLength(jenv, jskeys)) == 0) {
		ret = DB_DONOTINDEX;
		goto err;
	} else if (num_skeys == 1) {
		memset(result, 0, sizeof (DBT));
		tresult = result;
	} else {
		if ((ret = __os_umalloc(db->env,
		    num_skeys * sizeof (DBT), &result->data)) != 0)
			goto err;
		memset(result->data, 0, num_skeys * sizeof (DBT));
		result->size = num_skeys;
		F_SET(result, DB_DBT_APPMALLOC | DB_DBT_MULTIPLE);
		tresult = (DBT *)result->data;
	}

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
		goto err;
	}

	for (i = 0; i < num_skeys; i++, tresult++) {
		jresult = (*jenv)->GetObjectArrayElement(jenv, jskeys, i);

		if ((ret =
		    __dbj_dbt_copyin(jenv, &lresult, NULL, jresult, 0)) != 0)
			goto err;

		if (lresult.dbt.size != 0) {
			/* If there's data, we need to take a copy of it.  */
			tresult->size = lresult.dbt.size;
			if ((ret = __os_umalloc(NULL,
			    tresult->size, &tresult->data)) != 0)
				goto err;
			if ((ret = __dbj_dbt_memcopy(&lresult.dbt, 0,
			    tresult->data, tresult->size,
			    DB_USERCOPY_GETDATA)) != 0)
				goto err;
			__dbj_dbt_release(jenv,
			    jresult, &lresult.dbt, &lresult);
			(*jenv)->DeleteLocalRef(jenv, lresult.jarr);
			F_SET(tresult, DB_DBT_APPMALLOC);
		}

		(*jenv)->DeleteLocalRef(jenv, jresult);
	}

err:	if (key->app_data == NULL) {
		(*jenv)->DeleteLocalRef(jenv, jkeyarr);
		(*jenv)->DeleteLocalRef(jenv, jkey);
	}
	if (data->app_data == NULL) {
		(*jenv)->DeleteLocalRef(jenv, jdataarr);
		(*jenv)->DeleteLocalRef(jenv, jdata);
	}
	if (jskeys != NULL) {
		(*jenv)->DeleteLocalRef(jenv, jskeys);
	}

	if (detach)
		__dbj_detach();
	return (ret);
}

static int __dbj_append_recno(DB *db, DBT *dbt, db_recno_t recno)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);
	jobject jdbt;
	DBT_LOCKED lresult;
	jbyteArray jdbtarr;
	int ret;

	if (jdb == NULL) {
		ret = EINVAL;
		goto err;
	}

	/*
	 * The dbt we're passed will be from the application, but we can't
	 * just reuse it, since we will have already taken a copy of the data.
	 * Make a new DatabaseEntry object here for the callback.
	 */
	jdbt = (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
	if (jdbt == NULL) {
		ret = ENOMEM; /* An exception is pending */
		goto err;
	}

	__dbj_dbt_copyout(jenv, dbt, &jdbtarr, jdbt);
	if (jdbtarr == NULL) {
		ret = ENOMEM; /* An exception is pending */
		goto err;
	}

	ret = 0;
	(*jenv)->CallNonvirtualVoidMethod(jenv, jdb, db_class,
	    append_recno_method, jdbt, recno);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
		goto err;
	}

	ret = __dbj_dbt_copyin(jenv, &lresult, NULL, jdbt, 0);
	memset(dbt, 0, sizeof (DBT));

	if (ret == 0 && lresult.dbt.size != 0) {
		/* If there's data, we need to take a copy of it.  */
		dbt->size = lresult.dbt.size;
		if ((ret =
		    __os_umalloc(NULL, dbt->size, &dbt->data)) != 0)
			goto err;
		if ((ret = __dbj_dbt_memcopy(&lresult.dbt, 0,
		    dbt->data, dbt->size,
		    DB_USERCOPY_GETDATA)) != 0)
			goto err;
		__dbj_dbt_release(jenv, jdbt, &lresult.dbt, &lresult);
		(*jenv)->DeleteLocalRef(jenv, lresult.jarr);
		F_SET(dbt, DB_DBT_APPMALLOC);
	}

err:	(*jenv)->DeleteLocalRef(jenv, jdbtarr);
	(*jenv)->DeleteLocalRef(jenv, jdbt);

	if (detach)
		__dbj_detach();
	return (ret);
}

/*
 * Shared by __dbj_bt_compare and __dbj_h_compare
 */
static int __dbj_am_compare(DB *db, const DBT *dbt1, const DBT *dbt2,
    jmethodID compare_method)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);
	jbyteArray jdbtarr1, jdbtarr2;
	int ret;

	if (jdb == NULL) {
		ret = EINVAL;
		goto err;
	}

	if (dbt1->app_data != NULL)
		jdbtarr1 = ((DBT_LOCKED *)dbt1->app_data)->jarr;
	else {
		jdbtarr1 = (*jenv)->NewByteArray(jenv, (jsize)dbt1->size);
		if (jdbtarr1 == NULL) {
			ret = ENOMEM;
			goto err;
		}
		(*jenv)->SetByteArrayRegion(jenv, jdbtarr1, 0,
		    (jsize)dbt1->size, (jbyte *)dbt1->data);
	}

	if (dbt2->app_data != NULL)
		jdbtarr2 = ((DBT_LOCKED *)dbt2->app_data)->jarr;
	else {
		jdbtarr2 = (*jenv)->NewByteArray(jenv, (jsize)dbt2->size);
		if (jdbtarr2 == NULL) {
			ret = ENOMEM;
			goto err;
		}
		(*jenv)->SetByteArrayRegion(jenv, jdbtarr2, 0,
		    (jsize)dbt2->size, (jbyte *)dbt2->data);
	}

	ret = (int)(*jenv)->CallNonvirtualIntMethod(jenv, jdb, db_class,
	    compare_method, jdbtarr1, jdbtarr2);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
	}

err:	if (dbt1->app_data == NULL)
		(*jenv)->DeleteLocalRef(jenv, jdbtarr1);
	if (dbt2->app_data == NULL)
		(*jenv)->DeleteLocalRef(jenv, jdbtarr2);

	if (detach)
		__dbj_detach();
	return (ret);
}

static int __dbj_bt_compare(DB *db,
    const DBT *dbt1, const DBT *dbt2, size_t *locp)
{
	if (locp != NULL)
		locp = NULL;
	return __dbj_am_compare(db, dbt1, dbt2, bt_compare_method);
}

#define DBT_COPYOUT(num) do {						\
	if (dbt##num->app_data != NULL)					\
		jdbt##num = ((DBT_LOCKED *)dbt##num->app_data)->jdbt;	\
	else {								\
		if ((jdbt##num = (*jenv)->NewObject(			\
		    jenv, dbt_class, dbt_construct)) == NULL) {		\
			ret = ENOMEM; /* An exception is pending */	\
			goto err;					\
		}							\
		__dbj_dbt_copyout(jenv, dbt##num, &jdbtarr##num, jdbt##num);\
		if (jdbtarr##num == NULL) {				\
			ret = ENOMEM; /* An exception is pending */	\
			goto err;					\
		}							\
	}								\
} while (0)

#define DBT_COPIED_FREE(num) do {					\
	if (dbt##num->app_data == NULL) {				\
		(*jenv)->DeleteLocalRef(jenv, jdbtarr##num);		\
		(*jenv)->DeleteLocalRef(jenv, jdbt##num);		\
	}								\
} while (0)

#define DBT_COPYIN_DATA(num) do {					\
	ret = __dbj_dbt_copyin(jenv, &lresult, NULL, jdbt##num, 0);	\
	/* Re-initialize the output DBT. Preserve app_data. */		\
	dbt##num->data = NULL;						\
	dbt##num->size = 0;						\
	dbt##num->ulen = 0;						\
	dbt##num->dlen = 0;						\
	dbt##num->doff = 0;						\
	dbt##num->flags = 0;						\
	if (ret == 0 && lresult.dbt.size != 0) {			\
		/* If there's data, we need to take a copy of it.  */	\
		dbt##num->size = lresult.dbt.size;			\
		if ((ret = __os_umalloc(				\
		    NULL, dbt##num->size, &dbt##num->data)) != 0)	\
			goto err;					\
		if ((ret = __dbj_dbt_memcopy(&lresult.dbt, 0,		\
		    dbt##num->data, dbt##num->size,			\
		    DB_USERCOPY_GETDATA)) != 0)				\
			goto err;					\
		__dbj_dbt_release(jenv, jdbt##num, &lresult.dbt, &lresult);\
		(*jenv)->DeleteLocalRef(jenv, lresult.jarr);		\
		F_SET(dbt##num, DB_DBT_APPMALLOC);			\
	}								\
} while (0)

static int __dbj_bt_compress(DB *db, const DBT *dbt1, const DBT *dbt2,
    const DBT *dbt3, const DBT *dbt4, DBT *dbt5)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);
	jobject jdbt1, jdbt2, jdbt3, jdbt4, jdbt5;
	jbyteArray jdbtarr1, jdbtarr2, jdbtarr3, jdbtarr4, jdbtarr5;
	DBT_LOCKED lresult;
	int ret;

	if (jdb == NULL) {
		ret = EINVAL;
		goto err;
	}

	jdbt1 = jdbt2 = jdbt3 = jdbt4 = jdbt5 = NULL;

	DBT_COPYOUT(1);
	DBT_COPYOUT(2);
	DBT_COPYOUT(3);
	DBT_COPYOUT(4);
	DBT_COPYOUT(5);

	ret = (int)(*jenv)->CallNonvirtualIntMethod(jenv, jdb, db_class,
	    bt_compress_method, jdbt1, jdbt2, jdbt3, jdbt4, jdbt5);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
		goto err;
	}

	DBT_COPYIN_DATA(5);

err:	DBT_COPIED_FREE(1);
	DBT_COPIED_FREE(2);
	DBT_COPIED_FREE(3);
	DBT_COPIED_FREE(4);
	DBT_COPIED_FREE(5);
	if (detach)
		__dbj_detach();
	return (ret);
}

static int __dbj_bt_decompress(DB *db, const DBT *dbt1, const DBT *dbt2,
    DBT *dbt3, DBT *dbt4, DBT *dbt5)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);
	jobject jdbt1, jdbt2, jdbt3, jdbt4, jdbt5;
	jbyteArray jdbtarr1, jdbtarr2, jdbtarr3, jdbtarr4, jdbtarr5;
	DBT_LOCKED lresult;
	int ret;

	jdbt1 = jdbt2 = jdbt3 = jdbt4 = jdbt5 = NULL;

	if (jdb == NULL) {
		ret = EINVAL;
		goto err;
	}

	DBT_COPYOUT(1);
	DBT_COPYOUT(2);
	DBT_COPYOUT(3);
	DBT_COPYOUT(4);
	DBT_COPYOUT(5);

	ret = (int)(*jenv)->CallNonvirtualIntMethod(jenv, jdb, db_class,
	    bt_decompress_method, jdbt1, jdbt2, jdbt3, jdbt4, jdbt5);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
		goto err;
	}

	DBT_COPYIN_DATA(3);
	DBT_COPYIN_DATA(4);
	DBT_COPYIN_DATA(5);

err:	DBT_COPIED_FREE(1);
	DBT_COPIED_FREE(2);
	DBT_COPIED_FREE(3);
	DBT_COPIED_FREE(4);
	DBT_COPIED_FREE(5);
	if (detach)
		__dbj_detach();
	return (ret);
}

static size_t __dbj_bt_prefix(DB *db, const DBT *dbt1, const DBT *dbt2)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);
	jobject jdbt1, jdbt2;
	jbyteArray jdbtarr1, jdbtarr2;
	int ret;

	if (jdb == NULL) {
		ret = EINVAL;
		goto err;
	}

	if (dbt1->app_data != NULL)
		jdbt1 = ((DBT_LOCKED *)dbt1->app_data)->jdbt;
	else {
		if ((jdbt1 =
		    (*jenv)->NewObject(jenv, dbt_class, dbt_construct)) == NULL) {
			ret = ENOMEM; /* An exception is pending */
			goto err;
		}
		__dbj_dbt_copyout(jenv, dbt1, &jdbtarr1, jdbt1);
		if (jdbtarr1 == NULL) {
			ret = ENOMEM; /* An exception is pending */
			goto err;
		}
	}

	if (dbt2->app_data != NULL)
		jdbt2 = ((DBT_LOCKED *)dbt2->app_data)->jdbt;
	else {
		if ((jdbt2 =
		    (*jenv)->NewObject(jenv, dbt_class, dbt_construct)) == NULL) {
			ret = ENOMEM; /* An exception is pending */
			goto err;
		}
		__dbj_dbt_copyout(jenv, dbt2, &jdbtarr2, jdbt2);
		if (jdbtarr2 == NULL) {
			ret = ENOMEM; /* An exception is pending */
			goto err;
		}
	}

	ret = (int)(*jenv)->CallNonvirtualIntMethod(jenv, jdb, db_class,
	    bt_prefix_method, jdbt1, jdbt2);

err:	if (dbt1->app_data == NULL) {
		(*jenv)->DeleteLocalRef(jenv, jdbtarr1);
		(*jenv)->DeleteLocalRef(jenv, jdbt1);
	}
	if (dbt2->app_data == NULL) {
		(*jenv)->DeleteLocalRef(jenv, jdbtarr2);
		(*jenv)->DeleteLocalRef(jenv, jdbt2);
	}

	if (detach)
		__dbj_detach();
	return (ret);
}

static int __dbj_dup_compare(DB *db,
    const DBT *dbt1, const DBT *dbt2, size_t *locp)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);
	jbyteArray jdbtarr1, jdbtarr2;
	int ret;

	if (locp != NULL)
		locp = NULL;
	if (jdb == NULL)
		return (EINVAL);

	jdbtarr1 = (*jenv)->NewByteArray(jenv, (jsize)dbt1->size);
	if (jdbtarr1 == NULL)
		return (ENOMEM);
	(*jenv)->SetByteArrayRegion(jenv, jdbtarr1, 0, (jsize)dbt1->size,
	    (jbyte *)dbt1->data);

	jdbtarr2 = (*jenv)->NewByteArray(jenv, (jsize)dbt2->size);
	if (jdbtarr2 == NULL)
		return (ENOMEM);
	(*jenv)->SetByteArrayRegion(jenv, jdbtarr2, 0, (jsize)dbt2->size,
	    (jbyte *)dbt2->data);

	ret = (int)(*jenv)->CallNonvirtualIntMethod(jenv, jdb, db_class,
	    dup_compare_method, jdbtarr1, jdbtarr2);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
	}

	(*jenv)->DeleteLocalRef(jenv, jdbtarr2);
	(*jenv)->DeleteLocalRef(jenv, jdbtarr1);

	if (detach)
		__dbj_detach();
	return (ret);
}

static void __dbj_db_feedback(DB *db, int opcode, int percent)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);

	if (jdb != NULL)
		(*jenv)->CallNonvirtualVoidMethod(jenv, jdb, db_class,
		    db_feedback_method, opcode, percent);

	if (detach)
		__dbj_detach();
}

static int __dbj_h_compare(DB *db,
    const DBT *dbt1, const DBT *dbt2, size_t *locp)
{
	if (locp != NULL)
		locp = NULL;
	return __dbj_am_compare(db, dbt1, dbt2, h_compare_method);
}

static u_int32_t __dbj_h_hash(DB *db, const void *data, u_int32_t len)
{
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);
	jbyteArray jarr = (*jenv)->NewByteArray(jenv, (jsize)len);
	int ret;

	if (jdb == NULL)
		return (EINVAL);

	if ((jarr = (*jenv)->NewByteArray(jenv, (jsize)len)) == NULL)
		return (ENOMEM); /* An exception is pending */

	(*jenv)->SetByteArrayRegion(jenv, jarr, 0, (jsize)len, (jbyte *)data);

	ret = (int)(*jenv)->CallNonvirtualIntMethod(jenv, jdb, db_class,
	    h_hash_method, jarr, len);

	(*jenv)->DeleteLocalRef(jenv, jarr);

	if (detach)
		__dbj_detach();
	return (ret);
}

static u_int32_t __dbj_partition(DB *db, DBT *dbt1)
{
	int detach, ret;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdb = (jobject)DB_INTERNAL(db);
	jobject jdbt1 = NULL;
	jbyteArray jdbtarr1;
	DBT_LOCKED lresult;

	DBT_COPYOUT(1);

	ret = (int)(*jenv)->CallNonvirtualIntMethod(jenv, jdb, db_class,
	    partition_method, jdbt1);
	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
		goto err;
	}

	DBT_COPYIN_DATA(1);
err:	DBT_COPIED_FREE(1);

	if (detach)
		__dbj_detach();
	return (ret);
}

static int __dbj_backup_close(DB_ENV *dbenv, const char *dbname, void *handle) {
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);
	jobject jdbname;
	int ret;

	COMPQUIET(handle, NULL);

	if (jdbenv == NULL) {
		ret = EINVAL;
		goto err;
	}

	jdbname = (*jenv)->NewStringUTF(jenv, dbname);
	ret = (*jenv)->CallNonvirtualIntMethod(jenv, jdbenv, dbenv_class, backup_close_method, jdbname);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
	}

err:	if (detach)
		__dbj_detach();
	return (ret);
}

static int __dbj_backup_open(DB_ENV *dbenv, const char *target, const char *dbname, void **handle) {
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);
	jobject jtarget, jdbname;
	int ret;

	COMPQUIET(handle, NULL);

	if (jdbenv == NULL) {
		ret = EINVAL;
		goto err;
	}

	jtarget = (*jenv)->NewStringUTF(jenv, target);
	jdbname = (*jenv)->NewStringUTF(jenv, dbname);
	ret = (*jenv)->CallNonvirtualIntMethod(jenv, jdbenv, dbenv_class, backup_open_method, jtarget, jdbname);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
	}

err:	if (detach)
		__dbj_detach();
	return (ret);
}

static int __dbj_backup_write(DB_ENV *dbenv, u_int32_t file_pos_gbytes, u_int32_t file_pos_bytes, u_int32_t size, u_int8_t *buf, void *handle) {
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);
	jbyteArray jarr;
	int ret;

	COMPQUIET(handle, NULL);

	if (jdbenv == NULL) {
		ret = EINVAL;
		goto err;
	}

	if ((jarr = (*jenv)->NewByteArray(jenv, (jsize)size)) == NULL) {
		/* An exception is pending. */
		ret = ENOMEM;
		goto err;
	}
	(*jenv)->SetByteArrayRegion(jenv, jarr, 0, (jsize)size, (jbyte *)buf);
	
	ret = (*jenv)->CallNonvirtualIntMethod(jenv, jdbenv, dbenv_class, backup_write_method, file_pos_gbytes, file_pos_bytes, size, jarr);
	(*jenv)->DeleteLocalRef(jenv, jarr);
	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
	}

err:	if (detach)
		__dbj_detach();
	return (ret);
}

static int __dbj_rep_view(DB_ENV *dbenv, const char *name, int *result, u_int32_t flags) {
	int detach;
	JNIEnv *jenv = __dbj_get_jnienv(&detach);
	jobject jdbenv = (jobject)DB_ENV_INTERNAL(dbenv);
	jobject jname;
	jboolean jresult;
	int ret;

	if (jdbenv == NULL) {
		ret = EINVAL;
		goto err;
	}

	jname = (*jenv)->NewStringUTF(jenv, name);

	jresult = (*jenv)->CallNonvirtualBooleanMethod(jenv, jdbenv, dbenv_class, rep_view_method, jname, flags);

	if ((*jenv)->ExceptionOccurred(jenv)) {
		/* The exception will be thrown, so this could be any error. */
		ret = EINVAL;
		goto err;
	}

	ret = 0;
	if (jresult == JNI_FALSE)
		*result = 0;
	else
		*result = 1;

err:	if (detach)
		__dbj_detach();
	return (ret);
}
%}

JAVA_CALLBACK(int (*backup_close_fcn)(DB_ENV *,
    const char *, void *), com.sleepycat.db.BackupHandler, backup_close)
JAVA_CALLBACK(int (*backup_open_fcn)(DB_ENV *,
    const char *, const char *, void **), com.sleepycat.db.BackupHandler, backup_open)
JAVA_CALLBACK(int (*backup_write_fcn)(DB_ENV *, u_int32_t,
    u_int32_t, u_int32_t, u_int8_t *, void *), com.sleepycat.db.BackupHandler, backup_write)
JAVA_CALLBACK(void (*db_errcall_fcn)(const DB_ENV *,
    const char *, const char *), com.sleepycat.db.ErrorHandler, error)
JAVA_CALLBACK(void (*env_feedback_fcn)(DB_ENV *, int, int),
    com.sleepycat.db.FeedbackHandler, env_feedback)
JAVA_CALLBACK(void (*db_msgcall_fcn)(const DB_ENV *, const char *),
    com.sleepycat.db.MessageHandler, message)
JAVA_CALLBACK(void (*db_panic_fcn)(DB_ENV *, int),
    com.sleepycat.db.PanicHandler, panic)
JAVA_CALLBACK(void (*event_notify)(DB_ENV *, u_int32_t, void *),
    com.sleepycat.db.EventHandler, event_notify)
JAVA_CALLBACK(int (*tx_recover)(DB_ENV *, DBT *, DB_LSN *, db_recops),
    com.sleepycat.db.LogRecordHandler, app_dispatch)
JAVA_CALLBACK(int (*send)(DB_ENV *, const DBT *, const DBT *,
			       const DB_LSN *, int, u_int32_t),
    com.sleepycat.db.ReplicationTransport, rep_transport)
JAVA_CALLBACK(void (*dispatch)(DB_ENV *, DB_CHANNEL *, DBT *, u_int32_t, u_int32_t),
    com.sleepycat.db.ReplicationManagerMessageDispatch, repmgr_msg_dispatch)

/*
 * Db.associate is a special case, because the handler must be set in the
 * secondary DB - that's what we have in the callback.  In addition, there
 * are two flavors of callback (single key and multi-key), so we need to
 * check for both types when working out whether the C callback should
 * be NULL.  Note that this implies that the multi-key callback will be set
 * on the secondary database *before* associate is called.
 */
JAVA_CALLBACK(int (*callback)(DB *, const DBT *, const DBT *, DBT *),
    com.sleepycat.db.SecondaryKeyCreator, seckey_create)
%typemap(javain) int (*callback)(DB *, const DBT *, const DBT *, DBT *)
    %{ (secondary.seckey_create_handler = $javainput) != null ||
	(secondary.secmultikey_create_handler != null) %}
JAVA_CALLBACK(int (*callback)(DB *, const DBT *, DBT *, const DBT *, int *),
    com.sleepycat.db.ForeignKeyNullifier, foreignkey_nullify)
%typemap(javain) int (*callback)(DB *, const DBT *, DBT *, const DBT *, int *)
    %{ (primary.foreignkey_nullify_handler = $javainput) != null ||
	(primary.foreignmultikey_nullify_handler != null) %}

JAVA_CALLBACK(int (*db_append_recno_fcn)(DB *, DBT *, db_recno_t),
    com.sleepycat.db.RecordNumberAppender, append_recno)
JAVA_CALLBACK(int (*bt_compare_fcn)(DB *, const DBT *, const DBT *, size_t *),
    java.util.Comparator, bt_compare)
JAVA_CALLBACK(int (*bt_compress_fcn)(DB *, const DBT *, const DBT *,
    const DBT *, const DBT *, DBT *), 
    com.sleepycat.db.BtreeCompressor, bt_compress)
JAVA_CALLBACK(int (*bt_decompress_fcn)(DB *, const DBT *, const DBT *,
    DBT *, DBT *, DBT *), com.sleepycat.db.BtreeCompressor, bt_decompress)
JAVA_CALLBACK(u_int32_t (*db_partition_fcn)(DB *, DBT *),
    com.sleepycat.db.PartitionHandler, partition)
JAVA_CALLBACK(size_t (*bt_prefix_fcn)(DB *, const DBT *, const DBT *),
    com.sleepycat.db.BtreePrefixCalculator, bt_prefix)
JAVA_CALLBACK(int (*dup_compare_fcn)(DB *, const DBT *, const DBT *, size_t *),
    java.util.Comparator, dup_compare)
JAVA_CALLBACK(void (*db_feedback_fcn)(DB *, int, int),
    com.sleepycat.db.FeedbackHandler, db_feedback)
JAVA_CALLBACK(int (*h_compare_fcn)(DB *, const DBT *, const DBT *, size_t *),
    java.util.Comparator, h_compare)
JAVA_CALLBACK(u_int32_t (*h_hash_fcn)(DB *, const void *, u_int32_t),
    com.sleepycat.db.Hasher, h_hash)
JAVA_CALLBACK(int (*rep_view_fcn)(DB_ENV *, const char *, int *, u_int32_t),
    com.sleepycat.db.ReplicationViewHandler, rep_view);
