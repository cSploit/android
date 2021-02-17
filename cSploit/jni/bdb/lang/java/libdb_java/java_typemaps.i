/* Typemaps */
%define JAVA_TYPEMAP(_ctype, _jtype, _jnitype)
%typemap(jstype) _ctype #_jtype
%typemap(jtype) _ctype #_jtype
%typemap(jni) _ctype #_jnitype
%typemap(out) _ctype %{ $result = (_jnitype)$1; %}
%typemap(javain) _ctype "$javainput"
%typemap(javaout) _ctype { return $jnicall; }
%enddef

JAVA_TYPEMAP(int32_t, int, jint)
JAVA_TYPEMAP(u_int32_t, int, jint)
JAVA_TYPEMAP(u_int32_t pagesize, long, jlong)
JAVA_TYPEMAP(long, long, jlong)
JAVA_TYPEMAP(db_seq_t, long, jlong)
JAVA_TYPEMAP(db_size_t, long, jlong)
JAVA_TYPEMAP(db_ssize_t, long, jlong)
JAVA_TYPEMAP(pid_t, long, jlong)
JAVA_TYPEMAP(roff_t, long, jlong)
#ifndef SWIGJAVA
JAVA_TYPEMAP(db_threadid_t, long, jlong)
#endif
JAVA_TYPEMAP(db_timeout_t, long, jlong)
JAVA_TYPEMAP(size_t, long, jlong)
JAVA_TYPEMAP(db_ret_t, void, void)
%typemap(javaout) db_ret_t { $jnicall; }
%typemap(out) db_ret_t ""

JAVA_TYPEMAP(int_bool, boolean, jboolean)
%typemap(in) int_bool %{ $1 = ($input == JNI_TRUE); %}
%typemap(out) int_bool %{ $result = ($1) ? JNI_TRUE : JNI_FALSE; %}

/* Fake typedefs for SWIG */
typedef        int db_ret_t;    /* An int that is mapped to a void */
typedef        int int_bool;    /* An int that is mapped to a boolean */

%{
typedef int db_ret_t;
typedef int int_bool;

struct __db_lk_conflicts {
       u_int8_t *lk_conflicts;
       int lk_modes;
};

struct __db_out_stream {
       void *handle;
       int (*callback) __P((void *, const void *));
};

struct __db_repmgr_site_address {
	const char *host;
	u_int port;
};

struct __db_repmgr_sites {
       DB_REPMGR_SITE *sites;
       u_int32_t nsites;
};

#define        Db __db
#define        Dbc __dbc
#define        Dbt __db_dbt
#define        DbChannel __db_channel
#define        DbEnv __db_env
#define        DbLock __db_lock_u
#define        DbLogc __db_log_cursor
#define        DbLsn __db_lsn
#define        DbMpoolFile __db_mpoolfile
#define        DbSequence __db_sequence
#define        DbSite __db_site
#define        DbStream __db_stream
#define        DbTxn __db_txn

/* Suppress a compilation warning for an unused symbol */
void *unused = (void *)SWIG_JavaThrowException;
%}

/* Dbt handling */
JAVA_TYPEMAP(DBT *, com.sleepycat.db.DatabaseEntry, jobject)

%{
typedef struct __dbt_locked {
	JNIEnv *jenv;
	jobject jdbt;
	DBT dbt;
	jobject jdata_nio;
	jbyteArray jarr;
	jint offset;
	int reuse;
	u_int32_t orig_size;
	jsize array_len;
} DBT_LOCKED;

struct __dbt_arr {
	DBT *arr_ptr;
	int len;
};

static int __dbj_dbt_memcopy(DBT *dbt, u_int32_t offset, void *buf, u_int32_t size, u_int32_t flags) {
	DBT_LOCKED *ldbt = dbt->app_data;
	JNIEnv *jenv = ldbt->jenv;

	if (size == 0)
		return (0);
	else if (!F_ISSET(dbt, DB_DBT_USERCOPY)) {
		/*
		  * For simplicity, the Java API calls this function directly,
		  * so it needs to work with regular DBTs.
		  */
		switch (flags) {
		case DB_USERCOPY_GETDATA:
			memcpy(buf, (u_int8_t *)dbt->data + offset, size);
			return (0);
		case DB_USERCOPY_SETDATA:
			memcpy((u_int8_t *)dbt->data + offset, buf, size);
			return (0);
		default:
			return (EINVAL);
		}
	}

	switch (flags) {
	case DB_USERCOPY_GETDATA:
		(*jenv)->GetByteArrayRegion(jenv, ldbt->jarr, ldbt->offset +
					    offset, size, buf);
		break;
	case DB_USERCOPY_SETDATA:
		/*
		 * Check whether this is the first time through the callback by relying
		 * on the offset being zero.
		 */
		if (offset == 0 && (!ldbt->reuse ||
		    (jsize)(ldbt->offset + dbt->size) > ldbt->array_len)) {
			if (ldbt->jarr != NULL)
				(*jenv)->DeleteLocalRef(jenv, ldbt->jarr);
			ldbt->jarr = (*jenv)->NewByteArray(jenv, (jsize)dbt->size);
			if (ldbt->jarr == NULL)
				return (ENOMEM);
			(*jenv)->SetObjectField(jenv, ldbt->jdbt, dbt_data_fid, ldbt->jarr);
			/* We've allocated a new array, start from the beginning. */
			ldbt->offset = 0;
		}
		(*jenv)->SetByteArrayRegion(jenv, ldbt->jarr, ldbt->offset +
					    offset, size, buf);
		break;
	default:
		return (EINVAL);
	}
	return ((*jenv)->ExceptionOccurred(jenv) ? EINVAL : 0);
}

static void __dbj_dbt_copyout(
    JNIEnv *jenv, const DBT *dbt, jbyteArray *jarr, jobject jdbt)
{
	jbyteArray newarr = (*jenv)->NewByteArray(jenv, (jsize)dbt->size);
	if (newarr == NULL)
		return; /* An exception is pending */
	(*jenv)->SetByteArrayRegion(jenv, newarr, 0, (jsize)dbt->size,
	    (jbyte *)dbt->data);
	(*jenv)->SetObjectField(jenv, jdbt, dbt_data_fid, newarr);
	(*jenv)->SetIntField(jenv, jdbt, dbt_offset_fid, 0);
	(*jenv)->SetIntField(jenv, jdbt, dbt_size_fid, (jint)dbt->size);
	if (jarr != NULL)
		*jarr = newarr;
	else
		(*jenv)->DeleteLocalRef(jenv, newarr);
}

static int __dbj_dbt_copyin(
    JNIEnv *jenv, DBT_LOCKED *ldbt, DBT **dbtp, jobject jdbt, int allow_null)
{
	DBT *dbt;
	jlong capacity;

	memset(ldbt, 0, sizeof (*ldbt));
	ldbt->jenv = jenv;
	ldbt->jdbt = jdbt;

	if (jdbt == NULL) {
		if (allow_null) {
			*dbtp = NULL;
			return (0);
		} else {
			return (__dbj_throw(jenv, EINVAL,
			    "DatabaseEntry must not be null", NULL, NULL));
		}
	}

	dbt = &ldbt->dbt;
	if (dbtp != NULL)
		*dbtp = dbt;

	ldbt->jdata_nio = (*jenv)->GetObjectField(jenv, jdbt, dbt_data_nio_fid);
	if (ldbt->jdata_nio != NULL)
		F_SET(dbt, DB_DBT_USERMEM);
	else
		ldbt->jarr = (jbyteArray)(*jenv)->GetObjectField(jenv, jdbt, dbt_data_fid);
	ldbt->offset = (*jenv)->GetIntField(jenv, jdbt, dbt_offset_fid);
	dbt->size = (*jenv)->GetIntField(jenv, jdbt, dbt_size_fid);
	ldbt->orig_size = dbt->size;
	dbt->flags = (*jenv)->GetIntField(jenv, jdbt, dbt_flags_fid);

	if (F_ISSET(dbt, DB_DBT_USERMEM))
		dbt->ulen = (*jenv)->GetIntField(jenv, jdbt, dbt_ulen_fid);
	if (F_ISSET(dbt, DB_DBT_PARTIAL)) {
		dbt->dlen = (*jenv)->GetIntField(jenv, jdbt, dbt_dlen_fid);
		dbt->doff = (*jenv)->GetIntField(jenv, jdbt, dbt_doff_fid);

		if ((jint)dbt->doff < 0)
			return (__dbj_throw(jenv, EINVAL, "DatabaseEntry doff illegal",
			    NULL, NULL));
	}

	/*
	 * We don't support DB_DBT_REALLOC - map anything that's not USERMEM to
	 * MALLOC.
	 */
	if (!F_ISSET(dbt, DB_DBT_USERMEM)) {
		ldbt->reuse = !F_ISSET(dbt, DB_DBT_MALLOC);
		F_CLR(dbt, DB_DBT_MALLOC | DB_DBT_REALLOC);
	}

	/* Verify parameters before allocating or locking data. */
	if (ldbt->jdata_nio != NULL) {
		capacity = (*jenv)->GetDirectBufferCapacity(jenv,
				ldbt->jdata_nio);
		if (capacity > (jlong)UINT32_MAX)
			return (__dbj_throw(jenv, EINVAL,
			    "DirectBuffer may not be larger than 4GB",
			    NULL, NULL));
		ldbt->array_len = (u_int32_t)capacity;
	} else if (ldbt->jarr == NULL) {
		/*
		 * Some code makes the assumption that if a DBT's size or ulen
		 * is non-zero, there is data to copy from dbt->data.
		 *
		 * Clean up the dbt fields so we don't run into trouble.
		 * (Note that doff, dlen, and flags all may contain
		 * meaningful values.)
		 */
		dbt->data = NULL;
		ldbt->array_len = ldbt->offset = dbt->size = dbt->ulen = 0;
	} else
		ldbt->array_len = (*jenv)->GetArrayLength(jenv, ldbt->jarr);

	if (F_ISSET(dbt, DB_DBT_USERMEM)) {
		if (ldbt->offset < 0)
			return (__dbj_throw(jenv, EINVAL,
			    "offset cannot be negative",
			    NULL, NULL));
		if (dbt->size > dbt->ulen)
			return (__dbj_throw(jenv, EINVAL,
			    "size must be less than or equal to ulen",
			    NULL, NULL));
		if ((jsize)(ldbt->offset + dbt->ulen) > ldbt->array_len)
			return (__dbj_throw(jenv, EINVAL,
			    "offset + ulen greater than array length",
			    NULL, NULL));
	}

	if (ldbt->jdata_nio) {
		dbt->data = (*jenv)->GetDirectBufferAddress(jenv,
				ldbt->jdata_nio);
		dbt->data = (u_int8_t *)dbt->data + ldbt->offset;
	} else if (F_ISSET(dbt, DB_DBT_USERMEM)) {
		if (ldbt->jarr != NULL &&
		    (dbt->data = (*jenv)->GetByteArrayElements(jenv,
		    ldbt->jarr, NULL)) == NULL)
			return (EINVAL); /* an exception will be pending */
		dbt->data = (u_int8_t *)dbt->data + ldbt->offset;
	} else
		F_SET(dbt, DB_DBT_USERCOPY);
	dbt->app_data = ldbt;

	return (0);
}

static void __dbj_dbt_release(
    JNIEnv *jenv, jobject jdbt, DBT *dbt, DBT_LOCKED *ldbt) {
	jthrowable t;

	if (dbt == NULL)
		return;

	if (dbt->size != ldbt->orig_size)
		(*jenv)->SetIntField(jenv, jdbt, dbt_size_fid, (jint)dbt->size);

	if (F_ISSET(dbt, DB_DBT_USERMEM)) {
		if (ldbt->jarr != NULL)
			(*jenv)->ReleaseByteArrayElements(jenv, ldbt->jarr,
			    (jbyte *)dbt->data - ldbt->offset, 0);

		if (dbt->size > dbt->ulen &&
		    (t = (*jenv)->ExceptionOccurred(jenv)) != NULL &&
		    (*jenv)->IsInstanceOf(jenv, t, memex_class)) {
			(*jenv)->CallNonvirtualVoidMethod(jenv, t, memex_class,
			    memex_update_method, jdbt);
			/*
			 * We have to rethrow the exception because calling
			 * into Java clears it.
			 */
			(*jenv)->Throw(jenv, t);
		}
	}
}
%}

%typemap(in) DBT * (DBT_LOCKED ldbt) %{
	if (__dbj_dbt_copyin(jenv, &ldbt, &$1, $input, 0) != 0) {
		return $null; /* An exception will be pending. */
	}%}

/* Special cases for DBTs that may be null: DbEnv.rep_start, Db.compact */
%typemap(in) DBT *data_or_null (DBT_LOCKED ldbt) %{
	if (__dbj_dbt_copyin(jenv, &ldbt, &$1, $input, 1) != 0) {
		return $null; /* An exception will be pending. */
	}%}

%apply DBT *data_or_null {DBT *cdata, DBT *start, DBT *stop, DBT *end, DBT *db_put_data};

%typemap(freearg) DBT * %{ __dbj_dbt_release(jenv, $input, $1, &ldbt$argnum); %}

/* Special case for Db.set_partition. */
%typemap(in) DBT *keys { 
	DBT_LOCKED lresult;
	DBT *dbt;
	void *ptr, *retdata;
	int cnt, i, ret;
	u_int32_t retlen;

	if ($input == NULL)
		$1 = NULL;
	else {
		/* Copy the DBT from Java to C. */
		if ((ret = __dbj_dbt_copyin(jenv,
		    &lresult, &dbt, $input, 0)) != 0)
			/* An exception will be pending. */
			return $null;

		/* Get the number of DBT from the bulk buffer. */
		DB_MULTIPLE_INIT(ptr, dbt);
		DB_MULTIPLE_NEXT(ptr, dbt, retdata, retlen);
		cnt = 0;
		while (ptr != NULL) {
			cnt++;
			DB_MULTIPLE_NEXT(ptr, dbt, retdata, retlen);
		}

		/* Make an array of DBTs. */
		if ((ret = __os_malloc(NULL,
		    (cnt + 1) * sizeof(DBT), &$1)) != 0) {
			__dbj_throw(jenv, ret, NULL, NULL, NULL);
			goto err;
		}
		memset($1, 0, (cnt + 1) * sizeof(DBT));
		/*
		 * Save the key array size in the last entry so that it can
		 * be used later when free the key array and mark its flags
		 * as DB_DBT_BULK which is an internal flag that can't be
		 * passed by users. The last entry will not be read by the C
		 * call __partition_set since it only reads the first cnt
		 * DBTs from the key array.
		 */
		$1[cnt].size = sizeof(int);
		$1[cnt].data = &cnt;
		$1[cnt].flags = DB_DBT_BULK;
		DB_MULTIPLE_INIT(ptr, dbt);
		for (i = 0; i < cnt; i++) {
			DB_MULTIPLE_NEXT(ptr, dbt, retdata, retlen);
			if (retlen != 0) {
				/*
				 * If there's data, we need to take a copy
				 * of it.
				 */
				$1[i].size = retlen;
				if ((ret = __os_malloc(NULL,
				    $1[i].size, &$1[i].data)) != 0) {
					__dbj_throw(jenv,
					    ret, NULL, NULL, NULL);
					goto err;
				}
				memcpy($1[i].data, retdata, retlen);
			}
		}

err:		if (ret != 0) {
			if ($1 != NULL) {
				for (i = 0; i < cnt; i++)
					__os_free(NULL, $1[i].data);
			__os_free(NULL, $1);
			}
			return $null; 
		}
	}
}

%typemap(freearg) DBT *keys %{
{
	int cnt, i;

	if ($1 != NULL) {
		/* Get the array size. */
		cnt = 0;
		while ($1[cnt].flags != DB_DBT_BULK)
			cnt++;
		for (i = 0; i < cnt; i++)
			__os_free(NULL, $1[i].data);
		__os_free(NULL, $1);
	}
}
%}

JAVA_TYPEMAP(struct __dbt_arr, com.sleepycat.db.DatabaseEntry, jobject)
%typemap(out) struct __dbt_arr {
	DBT *dbt;
	void *ptr;
	int i, ret;
	u_int32_t buflen;

	if ($1.arr_ptr == NULL || $1.len <= 0)
		$result = NULL;
	else {
		/* Allocate the bulk buffer and make the bulk DBT. */
		for (i = 0, buflen = 0; i < $1.len; i++)
			buflen += $1.arr_ptr[i].size * sizeof(u_int32_t);
		buflen += ($1.len * 2 + 1) * sizeof(u_int32_t);
		if ((ret = __os_malloc(NULL, sizeof(DBT), &dbt)) != 0) {
			__dbj_throw(jenv, ret, NULL, NULL, NULL);
			goto err;
		}
		if ((ret = __os_malloc(NULL, buflen, &dbt->data)) != 0) {
			__dbj_throw(jenv, ret, NULL, NULL, NULL);
			goto err;
		}
		dbt->size = dbt->ulen = buflen;
		DB_MULTIPLE_INIT(ptr, dbt);
		for (i = 0; i < $1.len; i++)
			DB_MULTIPLE_WRITE_NEXT(ptr, dbt,
			    $1.arr_ptr[i].data, $1.arr_ptr[i].size);

		/* Map a bulk DBT into a DatabaseEntry. */
		$result = (*jenv)->NewObject(jenv, dbt_class, dbt_construct);
		__dbj_dbt_copyout(jenv, dbt, NULL, $result);
		if ($result == NULL)
			goto err;

err:		if (dbt != NULL) {
			if (dbt->data != NULL)
				__os_free(NULL, dbt->data);
			__os_free(NULL, dbt);
		}
		if (ret != 0 || $result == NULL)
			return $null;
	}
}

/* DB_TXN_TOKEN handling */
JAVA_TYPEMAP(DB_TXN_TOKEN *, byte[], jobject)

%typemap(check) DB_TXN_TOKEN * %{
       if ($1 == NULL) {
               __dbj_throw(jenv, EINVAL, "null txn commit token", NULL, NULL);
               return $null;
       }
%}


%typemap(in) DB_TXN_TOKEN * (DB_TXN_TOKEN token) %{
       if ($input == NULL) {
               $1 = NULL;
       } else {
               $1 = &token;
               (*jenv)->GetByteArrayRegion(jenv, (jbyteArray)$input, 0, DB_TXN_TOKEN_SIZE, $1->buf);
       }
%}

%typemap(out) DB_TXN_TOKEN * %{
       if ($input != NULL) {
               (*jenv)->SetByteArrayRegion(jenv, (jbyteArray)$input, 0, DB_TXN_TOKEN_SIZE, $1->buf);
       }
%}

/* DbLsn handling */
JAVA_TYPEMAP(DB_LSN *, com.sleepycat.db.LogSequenceNumber, jobject)

%typemap(check) DB_LSN *lsn_or_null ""

%typemap(check) DB_LSN * %{
	if ($1 == NULL) {
		__dbj_throw(jenv, EINVAL, "null LogSequenceNumber", NULL, NULL);
		return $null;
	}
%}

%typemap(in) DB_LSN * (DB_LSN lsn) %{
	if ($input == NULL) {
		$1 = NULL;
	} else {
		$1 = &lsn;
		$1->file = (*jenv)->GetIntField(jenv, $input, dblsn_file_fid);
		$1->offset = (*jenv)->GetIntField(jenv, $input,
		    dblsn_offset_fid);
	}
%}

%typemap(freearg) DB_LSN * %{
	if ($input != NULL) {
		(*jenv)->SetIntField(jenv, $input, dblsn_file_fid, $1->file);
		(*jenv)->SetIntField(jenv, $input,
		    dblsn_offset_fid, $1->offset);
	}
%}

/* Various typemaps */
JAVA_TYPEMAP(time_t, long, jlong)
JAVA_TYPEMAP(time_t *, long, jlong)
%typemap(in) time_t * (time_t time) %{
	time = (time_t)$input;
	$1 = &time;
%}

%typemap(in) time_t %{
        $1 = $input;
%}

JAVA_TYPEMAP(u_int, long, jlong)
JAVA_TYPEMAP(u_int *, long, jlong)
%typemap(in) u_int * (u_int value) %{
        value = (u_int)$input;
        $1 = &value;
%}

%typemap(in) u_int %{
        $1 = $input;
%}

JAVA_TYPEMAP(DB_KEY_RANGE *, com.sleepycat.db.KeyRange, jobject)
%typemap(in) DB_KEY_RANGE * (DB_KEY_RANGE range) {
	$1 = &range;
}
%typemap(argout) DB_KEY_RANGE * {
	(*jenv)->SetDoubleField(jenv, $input, kr_less_fid, $1->less);
	(*jenv)->SetDoubleField(jenv, $input, kr_equal_fid, $1->equal);
	(*jenv)->SetDoubleField(jenv, $input, kr_greater_fid, $1->greater);
}

JAVA_TYPEMAP(DBC **, Dbc[], jobjectArray)
%typemap(in) DBC ** {
	int i, count, ret;

	count = (*jenv)->GetArrayLength(jenv, $input);
	if ((ret = __os_malloc(NULL, (count + 1) * sizeof(DBC *), &$1)) != 0) {
		__dbj_throw(jenv, ret, NULL, NULL, DB2JDBENV);
		return $null;
	}
	for (i = 0; i < count; i++) {
		jobject jobj = (*jenv)->GetObjectArrayElement(jenv, $input, i);
		/*
		 * A null in the array is treated as an endpoint.
		 */
		if (jobj == NULL) {
			$1[i] = NULL;
			break;
		} else {
			jlong jptr = (*jenv)->GetLongField(jenv, jobj,
			    dbc_cptr_fid);
			$1[i] = *(DBC **)(void *)&jptr;
		}
	}
	$1[count] = NULL;
}

%typemap(freearg) DBC ** %{
	__os_free(NULL, $1);
%}

JAVA_TYPEMAP(u_int8_t *gid, byte[], jbyteArray)
%typemap(check) u_int8_t *gid %{
	if ((*jenv)->GetArrayLength(jenv, $input) < DB_GID_SIZE) {
		__dbj_throw(jenv, EINVAL,
		    "DbTxn.prepare gid array must be >= 128 bytes", NULL,
		    TXN2JDBENV);
		return $null;
	}
%}

%typemap(in) u_int8_t *gid %{
	$1 = (u_int8_t *)(*jenv)->GetByteArrayElements(jenv, $input, NULL);
%}

%typemap(freearg) u_int8_t *gid %{
	(*jenv)->ReleaseByteArrayElements(jenv, $input, (jbyte *)$1, 0);
%}

%define STRING_ARRAY_OUT
	int i, len;

	len = 0;
	while ($1[len] != NULL)
		len++;
	if (($result = (*jenv)->NewObjectArray(jenv, (jsize)len, string_class,
	    NULL)) == NULL)
		return $null; /* an exception is pending */
	for (i = 0; i < len; i++) {
		jstring str = (*jenv)->NewStringUTF(jenv, $1[i]);
		(*jenv)->SetObjectArrayElement(jenv, $result, (jsize)i, str);
	}
%enddef

JAVA_TYPEMAP(char **, String[], jobjectArray)
%typemap(out) const char ** {
	if ($1 != NULL) {
		STRING_ARRAY_OUT
	}
}
%typemap(out) char ** {
	if ($1 != NULL) {
		STRING_ARRAY_OUT
		__os_ufree(NULL, $1);
	}
}

JAVA_TYPEMAP(char **hostp, String, jobjectArray)

JAVA_TYPEMAP(struct __db_lk_conflicts, byte[][], jobjectArray)
%typemap(in) struct __db_lk_conflicts {
	int i, len, ret;
	size_t bytesize;

	len = $1.lk_modes = (*jenv)->GetArrayLength(jenv, $input);
	bytesize = sizeof(u_char) * len * len;

	if ((ret = __os_malloc(NULL, bytesize, &$1.lk_conflicts)) != 0) {
		__dbj_throw(jenv, ret, NULL, NULL, JDBENV);
		return $null;
	}

	for (i = 0; i < len; i++) {
		jobject sub_array = (*jenv)->GetObjectArrayElement(jenv,
		    $input, i);
		(*jenv)->GetByteArrayRegion(jenv,(jbyteArray)sub_array, 0, len,
		    (jbyte *)&$1.lk_conflicts[i * len]);
	}
}

%typemap(freearg) struct __db_lk_conflicts %{
	__os_free(NULL, $1.lk_conflicts);
%}

%typemap(out) struct __db_lk_conflicts {
	int i;
	jbyteArray bytes;

	$result = (*jenv)->NewObjectArray(jenv,
	    (jsize)$1.lk_modes, bytearray_class, NULL);
	if ($result == NULL)
		return $null; /* an exception is pending */
	for (i = 0; i < $1.lk_modes; i++) {
		bytes = (*jenv)->NewByteArray(jenv, (jsize)$1.lk_modes);
		if (bytes == NULL)
			return $null; /* an exception is pending */
		(*jenv)->SetByteArrayRegion(jenv, bytes, 0, (jsize)$1.lk_modes,
		    (jbyte *)($1.lk_conflicts + i * $1.lk_modes));
		(*jenv)->SetObjectArrayElement(jenv, $result, (jsize)i, bytes);
	}
}

%{
struct __dbj_verify_data {
	JNIEnv *jenv;
	jobject streamobj;
	jbyteArray bytes;
	int nbytes;
};

static int __dbj_verify_callback(void *handle, const void *str_arg) {
	char *str;
	struct __dbj_verify_data *vd;
	int len;
	JNIEnv *jenv;

	str = (char *)str_arg;
	vd = (struct __dbj_verify_data *)handle;
	jenv = vd->jenv;
	len = (int)strlen(str) + 1;
	if (len > vd->nbytes) {
		vd->nbytes = len;
		if (vd->bytes != NULL)
			(*jenv)->DeleteLocalRef(jenv, vd->bytes);
		if ((vd->bytes = (*jenv)->NewByteArray(jenv, (jsize)len))
		    == NULL)
			return (ENOMEM);
	}

	if (vd->bytes != NULL) {
		(*jenv)->SetByteArrayRegion(jenv, vd->bytes, 0, (jsize)len,
		    (jbyte*)str);
		(*jenv)->CallVoidMethod(jenv, vd->streamobj,
		    outputstream_write_method, vd->bytes, 0, len - 1);
	}

	if ((*jenv)->ExceptionOccurred(jenv) != NULL)
		return (EIO);

	return (0);
}
%}

JAVA_TYPEMAP(struct __db_out_stream, java.io.OutputStream, jobject)
%typemap(in) struct __db_out_stream (struct __dbj_verify_data data) {
	data.jenv = jenv;
	data.streamobj = $input;
	data.bytes = NULL;
	data.nbytes = 0;
	$1.handle = &data;
	$1.callback = __dbj_verify_callback;
}

JAVA_TYPEMAP(DB_PREPLIST *, com.sleepycat.db.PreparedTransaction[],
    jobjectArray)
%typemap(out) DB_PREPLIST * {
	int i, len;

	if ($1 == NULL)
		$result = NULL;
	else {
		len = 0;
		while ($1[len].txn != NULL)
			len++;
		$result = (*jenv)->NewObjectArray(jenv, (jsize)len, dbpreplist_class,
		    NULL);
		if ($result == NULL)
			return $null; /* an exception is pending */
		for (i = 0; i < len; i++) {
			jobject jtxn = (*jenv)->NewObject(jenv, dbtxn_class,
			    dbtxn_construct, $1[i].txn, JNI_FALSE);
			jobject bytearr = (*jenv)->NewByteArray(jenv,
			    (jsize)sizeof($1[i].gid));
			jobject obj = (*jenv)->NewObject(jenv, dbpreplist_class,
			    dbpreplist_construct, jtxn, bytearr);
	
			if (jtxn == NULL || bytearr == NULL || obj == NULL)
				return $null; /* An exception is pending */

			(*jenv)->SetByteArrayRegion(jenv, bytearr, 0,
			    (jsize)sizeof($1[i].gid), (jbyte *)$1[i].gid);
			(*jenv)->SetObjectArrayElement(jenv, $result, i, obj);
		}
		__os_ufree(NULL, $1);
	}
}

JAVA_TYPEMAP(DB_LOCKREQ *, com.sleepycat.db.LockRequest[], jobjectArray)

%native(DbEnv_lock_vec) void DbEnv_lock_vec(DB_ENV *dbenv, u_int32_t locker,
    u_int32_t flags, DB_LOCKREQ *list, int offset, int nlist);
%{
SWIGEXPORT void JNICALL
Java_com_sleepycat_db_internal_db_1javaJNI_DbEnv_1lock_1vec(JNIEnv *jenv,
    jclass jcls, jlong jdbenvp, jobject jdbenv, jint locker, jint flags,
    jobjectArray list, jint offset, jint count) {
	DB_ENV *dbenv;
	DB_LOCKREQ *lockreq;
	DB_LOCKREQ *prereq;	/* preprocessed requests */
	DB_LOCKREQ *failedreq;
	DB_LOCK *lockp;
	DBT_LOCKED *locked_dbts;
	DBT *obj;
	ENV *env;
	int alloc_err, i, ret;
	size_t bytesize, ldbtsize;
	jobject jlockreq;
	db_lockop_t op;
	jobject jobj, jlock;
	jlong jlockp;
	int completed;

	/*       
	 * We can't easily #include "dbinc/db_ext.h" because of name
	 * clashes, so we declare this explicitly.
	 */     
	extern int __dbt_usercopy __P((ENV *, DBT *));
	extern void __dbt_userfree __P((ENV *, DBT *, DBT *, DBT *));

	COMPQUIET(jcls, NULL);
	dbenv = *(DB_ENV **)(void *)&jdbenvp;
	env = dbenv->env;
	locked_dbts = NULL;
	lockreq = NULL;

	if (dbenv == NULL) {
		__dbj_throw(jenv, EINVAL, "null object", NULL, jdbenv);
		return;
	}

	if ((*jenv)->GetArrayLength(jenv, list) < offset + count) {
		__dbj_throw(jenv, EINVAL,
		    "DbEnv.lock_vec array not large enough", NULL, jdbenv);
		return;
	}

	bytesize = sizeof(DB_LOCKREQ) * count;
	if ((ret = __os_malloc(env, bytesize, &lockreq)) != 0) {
		__dbj_throw(jenv, ret, NULL, NULL, jdbenv);
		return;
	}
	memset(lockreq, 0, bytesize);

	ldbtsize = sizeof(DBT_LOCKED) * count;
	if ((ret = __os_malloc(env, ldbtsize, &locked_dbts)) != 0) {
		__dbj_throw(jenv, ret, NULL, NULL, jdbenv);
		goto err;
	}
	memset(locked_dbts, 0, ldbtsize);
	prereq = &lockreq[0];

	/* fill in the lockreq array */
	for (i = 0, prereq = &lockreq[0]; i < count; i++, prereq++) {
		jlockreq = (*jenv)->GetObjectArrayElement(jenv, list,
		    offset + i);
		if (jlockreq == NULL) {
			__dbj_throw(jenv, EINVAL,
			    "DbEnv.lock_vec list entry is null", NULL, jdbenv);
			goto err;
		}
		op = (db_lockop_t)(*jenv)->GetIntField(
                    jenv, jlockreq, lockreq_op_fid);
		prereq->op = op;

		switch (op) {
		case DB_LOCK_GET_TIMEOUT:
			/* Needed: mode, timeout, obj.  Returned: lock. */
			prereq->op = (db_lockop_t)(*jenv)->GetIntField(
			    jenv, jlockreq, lockreq_timeout_fid);
			/* FALLTHROUGH */
		case DB_LOCK_GET:
			/* Needed: mode, obj.  Returned: lock. */
			prereq->mode = (db_lockmode_t)(*jenv)->GetIntField(
			    jenv, jlockreq, lockreq_modeflag_fid);
			jobj = (*jenv)->GetObjectField(jenv, jlockreq,
			    lockreq_obj_fid);
			if ((ret = __dbj_dbt_copyin(jenv,
			    &locked_dbts[i], &obj, jobj, 0)) != 0 ||
			    (ret = __dbt_usercopy(env, obj)) != 0)
				goto err;
			prereq->obj = obj;
			break;
		case DB_LOCK_PUT:
			/* Needed: lock.  Ignored: mode, obj. */
			jlock = (*jenv)->GetObjectField(jenv, jlockreq,
				lockreq_lock_fid);
			if (jlock == NULL ||
			    (jlockp = (*jenv)->GetLongField(jenv, jlock,
			    lock_cptr_fid)) == 0L) {
				__dbj_throw(jenv, EINVAL,
				    "LockRequest lock field is NULL", NULL,
				    jdbenv);
				goto err;
			}
			lockp = *(DB_LOCK **)(void *)&jlockp;
			prereq->lock = *lockp;
			break;
		case DB_LOCK_PUT_ALL:
		case DB_LOCK_TIMEOUT:
			/* Needed: (none).  Ignored: lock, mode, obj. */
			break;
		case DB_LOCK_PUT_OBJ:
			/* Needed: obj.  Ignored: lock, mode. */
			jobj = (*jenv)->GetObjectField(jenv, jlockreq,
			    lockreq_obj_fid);
			if ((ret = __dbj_dbt_copyin(jenv,
			    &locked_dbts[i], &obj, jobj, 0)) != 0 ||
			    (ret = __dbt_usercopy(env, obj)) != 0)
				goto err;
			prereq->obj = obj;
			break;
		default:
			__dbj_throw(jenv, EINVAL,
			    "DbEnv.lock_vec bad op value", NULL, jdbenv);
			goto err;
		}
	}

	ret = dbenv->lock_vec(dbenv, (u_int32_t)locker, (u_int32_t)flags,
	    lockreq, count, &failedreq);
	if (ret == 0)
		completed = count;
	else
		completed = (int)(failedreq - lockreq);

	/* do post processing for any and all requests that completed */
	for (i = 0; i < completed; i++) {
		op = lockreq[i].op;
		if (op == DB_LOCK_PUT) {
			/*
			 * After a successful put, the DbLock can no longer be
			 * used, so we release the storage related to it.
			 */
			jlockreq = (*jenv)->GetObjectArrayElement(jenv,
			    list, i + offset);
			jlock = (*jenv)->GetObjectField(jenv, jlockreq,
			    lockreq_lock_fid);
			jlockp = (*jenv)->GetLongField(jenv, jlock,
			    lock_cptr_fid);
			lockp = *(DB_LOCK **)(void *)&jlockp;
			__os_free(NULL, lockp);
			(*jenv)->SetLongField(jenv, jlock, lock_cptr_fid,
			    (jlong)0);
		} else if (op == DB_LOCK_GET_TIMEOUT || op == DB_LOCK_GET) {
			/*
			 * Store the lock that was obtained.  We need to create
			 * storage for it since the lockreq array only exists
			 * during this method call.
			 */
			if ((alloc_err =
			    __os_malloc(env, sizeof(DB_LOCK), &lockp)) != 0) {
				__dbj_throw(jenv, alloc_err, NULL, NULL,
				    jdbenv);
				goto err;
			}

			*lockp = lockreq[i].lock;
			*(DB_LOCK **)(void *)&jlockp = lockp;

			jlockreq = (*jenv)->GetObjectArrayElement(jenv,
			    list, i + offset);
			jlock = (*jenv)->NewObject(jenv, lock_class,
			    lock_construct, jlockp, JNI_TRUE);
			if (jlock == NULL)
				goto err; /* An exception is pending */
			(*jenv)->SetLongField(jenv, jlock, lock_cptr_fid,
			    jlockp);
			(*jenv)->SetObjectField(jenv, jlockreq,
			    lockreq_lock_fid, jlock);
		}
	}

	/* If one of the locks was not granted, build the exception now. */
	if (ret == DB_LOCK_NOTGRANTED && i < count) {
		jlockreq = (*jenv)->GetObjectArrayElement(jenv, list,
		    i + offset);
		jobj = (*jenv)->GetObjectField(jenv, jlockreq, lockreq_obj_fid);
		jlock = (*jenv)->GetObjectField(jenv, jlockreq,
		    lockreq_lock_fid);
		(*jenv)->Throw(jenv,
		    (*jenv)->NewObject(jenv, lockex_class, lockex_construct,
		    (*jenv)->NewStringUTF(jenv, "DbEnv.lock_vec incomplete"),
		    lockreq[i].op, lockreq[i].mode, jobj, jlock, i, jdbenv));
	} else if (ret != 0)
		__dbj_throw(jenv, ret, NULL, NULL, jdbenv);

err:	for (i = 0, prereq = &lockreq[0]; i < count; i++, prereq++)
		if (prereq->op == DB_LOCK_GET_TIMEOUT ||
		    prereq->op == DB_LOCK_GET ||
		    prereq->op == DB_LOCK_PUT_OBJ) {
			jlockreq = (*jenv)->GetObjectArrayElement(jenv,
			    list, i + offset);
			jobj = (*jenv)->GetObjectField(jenv,
			    jlockreq, lockreq_obj_fid);
			__dbt_userfree(env, prereq->obj, NULL, NULL);
			__dbj_dbt_release(jenv, jobj, prereq->obj, &locked_dbts[i]);
	}
	if (locked_dbts != NULL)
		__os_free(env, locked_dbts);
	if (lockreq != NULL)
		__os_free(env, lockreq);
}
%}

%native(DbTxn_commit) void DbTxn_commit(DB_TXN *txn, u_int32_t flags);
%{
SWIGEXPORT void JNICALL
Java_com_sleepycat_db_internal_db_1javaJNI_DbTxn_1commit(JNIEnv *jenv,
    jclass jcls, jlong jarg1, jobject jarg1_, jint jarg2) {
  struct DbTxn *txn = (struct DbTxn *) 0 ;
  DB_ENV *dbenv = (DB_ENV *) 0 ;
  DB_REP_STAT *rep_stat = (DB_REP_STAT *) 0 ;
  ENV *env = (ENV *) 0 ;
  u_int32_t flags;
  DB_TXN_TOKEN token;
  db_ret_t result;
  db_ret_t result1;
  db_ret_t result2;
  int is_nested, is_logging_enabled, is_rep_client, commit_token_enabled;
  
  (void)jcls;
  txn = *(struct DbTxn **)&jarg1; 
  flags = (u_int32_t)jarg2; 
  
  if (jarg1 == 0) {
     __dbj_throw(jenv, EINVAL, "call on closed handle", NULL, NULL);
  return ;
  }

  /*
   * The Java API uses set_commit_token in a different way to the C API.
   * In Java a commit token is always generated, unless doing so would
   * generate an error from the C API.
   * The checks below are based on those in place in the implementation
   * of set_commit_token in the C API. It is invalid to set a commit
   * token for a subtransaction, or if logging is disabled, or on a rep
   * client node.
   */
  env = txn->mgrp->env;
  dbenv = env->dbenv;
  is_nested = (txn->parent != NULL);
  is_logging_enabled = env->lg_handle != NULL;
  /*
   * It is an illegal configuration to enable Java and disable statistics
   * or enable the small build, so Java should always have access to the
   * stat function.
   */
  is_rep_client = 0;
  if (env->rep_handle != NULL && env->rep_handle->region != NULL) {
    result2 = dbenv->rep_stat(dbenv, &rep_stat, 0);
    if (DB_RETOK_STD(result2)) {
      is_rep_client = (rep_stat->st_status == DB_REP_CLIENT);
      free(rep_stat);
    }
  }
  commit_token_enabled = (!is_nested && is_logging_enabled && !is_rep_client);

  if (commit_token_enabled) {
    result1 = (db_ret_t)txn->set_commit_token(txn, &token);
  }

  result = (db_ret_t)txn->commit(txn, flags);
  if (!DB_RETOK_STD(result)) {
     __dbj_throw(jenv, result, NULL, NULL, NULL);
  }

  /* 
   * Set the commit token in the Java Transaction class object only if 
   * the Core has generated a valid token for this transaction
   */
  if (commit_token_enabled && DB_RETOK_STD(result1)) {
     jbyteArray newarr = (*jenv)->NewByteArray(jenv, (jsize)DB_TXN_TOKEN_SIZE);
     if (newarr == NULL) {
        return; /* An exception is pending */
     }
     (*jenv)->SetByteArrayRegion(jenv, newarr, 0, (jsize)DB_TXN_TOKEN_SIZE,
        (jbyte *)&token);
     (*jenv)->SetObjectField(jenv, jarg1_, txn_commit_token, newarr);
  }
}
%}

JAVA_TYPEMAP(struct __db_repmgr_site_address,
    com.sleepycat.db.ReplicationHostAddress, jobject)
%typemap(out) struct __db_repmgr_site_address
{
	jstring addr_host;
	if ($1.host == NULL)
		return $null;
	addr_host = (*jenv)->NewStringUTF(jenv, $1.host);
	if (addr_host == NULL)
		return $null; /* An exception is pending. */
	$result = (*jenv)->NewObject(jenv,
	    rephost_class, rephost_construct, addr_host, $1.port);
	if ($result == NULL)
		return $null; /* An exception is pending */
}

JAVA_TYPEMAP(struct __db_repmgr_sites,
    com.sleepycat.db.ReplicationManagerSiteInfo[], jobjectArray)
%typemap(out) struct __db_repmgr_sites
{
	int i, len;
	jobject jrep_addr, jrep_info;

	len = $1.nsites;
	$result = (*jenv)->NewObjectArray(jenv, (jsize)len, repmgr_siteinfo_class,
	    NULL);
	if ($result == NULL)
		return $null; /* an exception is pending */
	for (i = 0; i < len; i++) {
		jstring addr_host = (*jenv)->NewStringUTF(jenv, $1.sites[i].host);
		if (addr_host == NULL)
			return $null; /* An exception is pending */
		jrep_addr = (*jenv)->NewObject(jenv,
		    rephost_class, rephost_construct, addr_host, $1.sites[i].port);
		if (jrep_addr == NULL)
			return $null; /* An exception is pending */

		jrep_info = (*jenv)->NewObject(jenv,
		    repmgr_siteinfo_class, repmgr_siteinfo_construct, jrep_addr, $1.sites[i].eid);
		if (jrep_info == NULL)
			return $null; /* An exception is pending */
		(*jenv)->SetIntField(jenv, jrep_info, repmgr_siteinfo_flags_fid,
		    $1.sites[i].flags);
		(*jenv)->SetIntField(jenv, jrep_info, repmgr_siteinfo_status_fid,
		    $1.sites[i].status);

		(*jenv)->SetObjectArrayElement(jenv, $result, i, jrep_info);
	}
	__os_ufree(NULL, $1.sites);
}

JAVA_TYPEMAP(void *, Object, jobject)

JAVA_TYPEMAP(DBT *chan_msgs, com.sleepycat.db.DatabaseEntry[], jobjectArray)
%typemap(in) DBT *chan_msgs {
	DBT_LOCKED lresult;
	int count, i, ret;

	count = (*jenv)->GetArrayLength(jenv, $input);
	if ((ret = __os_malloc(NULL, count * sizeof(DBT), &$1)) != 0) {
		__dbj_throw(jenv, ret, NULL, NULL, NULL);
		return $null;
	}
	memset($1, 0, count * sizeof(DBT));
	for (i = 0; i < count; i++) {
		jobject jresult = (*jenv)->GetObjectArrayElement(jenv, $input, i);
		if ((ret =
		    __dbj_dbt_copyin(jenv, &lresult, NULL, jresult, 0)) != 0) {
			return $null; /* An exception will be pending. */
		}
		if (lresult.dbt.size != 0) {
			/* If there's data, we need to take a copy of it.  */
			$1[i].size = lresult.dbt.size;
			if ((ret = __os_malloc(
			    NULL, $1[i].size, $1[i].data)) != 0) {
				__dbj_throw(jenv, ret, NULL, NULL, NULL);	
				return $null;
			}

			if ((ret = __dbj_dbt_memcopy(&lresult.dbt, 0,
			    $1[i].data, $1[i].size, DB_USERCOPY_GETDATA)) != 0) {
				__dbj_throw(jenv, ret, NULL, NULL, NULL);
				return $null;
			}

			__dbj_dbt_release(jenv, jresult, &lresult.dbt, &lresult);
			(*jenv)->DeleteLocalRef(jenv, lresult.jarr);
		}
		(*jenv)->DeleteLocalRef(jenv, jresult);
	}
}

%typemap(freearg) DBT *chan_msgs %{
{
	int count, i;

	count = (*jenv)->GetArrayLength(jenv, $input);
	for (i = 0; i < count; i++)
		__os_free(NULL, $1[i].data);
	__os_free(NULL, $1);
}
%}

