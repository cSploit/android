#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if HAVE_SQLITE2
#include "sqlite.h"
#endif

#if HAVE_SQLITE3
#include "sqlite3.h"
#undef  HAVE_SQLITE_COMPILE
#define HAVE_SQLITE_COMPILE 1
#undef  HAVE_SQLITE_PROGRESS_HANDLER
#define HAVE_SQLITE_PROGRESS_HANDLER 1
#undef  HAVE_SQLITE_TRACE
#define HAVE_SQLITE_TRACE 1
#if !HAVE_SQLITE3_MALLOC
#define sqlite3_malloc malloc
#define sqlite3_free   free
#endif
#if !HAVE_SQLITE3_BIND_PARAMETER_COUNT
#define sqlite3_bind_parameter_count(dummy) (1000)
#endif
#endif

#if HAVE_SQLITE2 && HAVE_SQLITE3
#define HAVE_BOTH_SQLITE 1
#endif

#ifndef HAVE_SQLITE3_SHARED_CACHE
#define HAVE_SQLITE3_SHARED_CACHE 0
#endif

#include "sqlite_jni.h"

#if defined(_WIN32) || !defined(CANT_PASS_VALIST_AS_CHARPTR)
#define MAX_PARAMS 256
#else
#define MAX_PARAMS 32
#endif

/* free memory proc */

typedef void (freemem)(void *);

/* internal handle for SQLite database */

typedef struct {
    void *sqlite;		/* SQLite handle */
#if HAVE_BOTH_SQLITE
    int is3;			/* True for SQLITE3 handle */
#endif
    int ver;			/* version code */
    jobject bh;			/* BusyHandler object */
    jobject cb;			/* Callback object */
    jobject ai;			/* Authorizer object */
    jobject tr;			/* Trace object */
    jobject pr;			/* Profile object */
    jobject ph;			/* ProgressHandler object */
    JNIEnv *env;		/* Java environment for callbacks */
    int row1;			/* true while processing first row */
    int haveutf;		/* true for SQLite UTF-8 support */
    jstring enc;		/* encoding or 0 */
    struct hfunc *funcs;	/* SQLite user defined function handles */
#if HAVE_SQLITE_COMPILE
    struct hvm *vms;		/* Compiled SQLite VMs */
#endif
#if HAVE_SQLITE3
    sqlite3_stmt *stmt;		/* For callback() */
#endif
#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
    struct hbl *blobs;		/* SQLite3 blob handles */
#endif
#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
  struct hbk *backups;		/* SQLite3 backup handles */
#endif
} handle;

/* internal handle for SQLite user defined function */

typedef struct hfunc {
    struct hfunc *next;		/* next function */
#if HAVE_BOTH_SQLITE
    int is3;			/* True for SQLITE3 handle */
#endif
    jobject fc;			/* FunctionContext object */
    jobject fi;			/* Function object */
    jobject db;			/* Database object */
    handle *h;			/* SQLite database handle */
    void *sf;			/* SQLite function handle */
    JNIEnv *env;		/* Java environment for callbacks */
} hfunc;

#if HAVE_SQLITE_COMPILE
/* internal handle for SQLite VM (sqlite_compile()) */

typedef struct hvm {
    struct hvm *next;		/* next vm handle */
#if HAVE_BOTH_SQLITE
    int is3;			/* True for SQLITE3 handle */
#endif
    void *vm;			/* SQLite 2/3 VM/statement */
    char *tail;			/* tail SQL string */
    int tail_len;		/* only for SQLite3/prepare */
    handle *h;			/* SQLite database handle */
    handle hh;			/* fake SQLite database handle */
} hvm;
#endif

#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
/* internal handle for sqlite3_blob */

typedef struct hbl {
    struct hbl *next;		/* next blob handle */
    sqlite3_blob *blob;		/* SQLite3 blob */
    handle *h;			/* SQLite database handle */
} hbl;
#endif

#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
/* internal handle for sqlite3_backup */

typedef struct hbk {
    struct hbk *next;		/* next blob handle */
    sqlite3_backup *bkup;	/* SQLite3 backup handle */
    handle *h;			/* SQLite database handle (source) */
} hbk;
#endif

/* ISO to/from UTF-8 translation */

typedef struct {
    char *result;		/* translated C string result */
    char *tofree;		/* memory to be free'd, or 0 */
    jstring jstr;		/* resulting Java string or 0 */
} transstr;

/* static cached weak class refs, field and method ids */

static jclass C_java_lang_String = 0;

static jfieldID F_SQLite_Database_handle = 0;
static jfieldID F_SQLite_Database_error_code = 0;
static jfieldID F_SQLite_FunctionContext_handle = 0;
static jfieldID F_SQLite_Vm_handle = 0;
static jfieldID F_SQLite_Vm_error_code = 0;
static jfieldID F_SQLite_Stmt_handle = 0;
static jfieldID F_SQLite_Stmt_error_code = 0;
static jfieldID F_SQLite_Blob_handle = 0;
static jfieldID F_SQLite_Blob_size = 0;
static jfieldID F_SQLite_Backup_handle = 0;

static jmethodID M_java_lang_String_getBytes = 0;
static jmethodID M_java_lang_String_getBytes2 = 0;
static jmethodID M_java_lang_String_initBytes = 0;
static jmethodID M_java_lang_String_initBytes2 = 0;

static const char xdigits[] = "0123456789ABCDEF";

static void
seterr(JNIEnv *env, jobject obj, int err)
{
    jvalue v;

    v.j = 0;
    v.i = (jint) err;
    (*env)->SetIntField(env, obj, F_SQLite_Database_error_code, v.i);
}

#if HAVE_SQLITE_COMPILE
static void
setvmerr(JNIEnv *env, jobject obj, int err)
{
    jvalue v;

    v.j = 0;
    v.i = (jint) err;
    (*env)->SetIntField(env, obj, F_SQLite_Vm_error_code, v.i);
}

#if HAVE_SQLITE3
static void
setstmterr(JNIEnv *env, jobject obj, int err)
{
    jvalue v;

    v.j = 0;
    v.i = (jint) err;
    (*env)->SetIntField(env, obj, F_SQLite_Stmt_error_code, v.i);
}

static int
jstrlen(const jchar *jstr)
{
    int len = 0;

    if (jstr) {
	while (*jstr++) {
	    len++;
	}
    }
    return len;
}
#endif
#endif

static void *
gethandle(JNIEnv *env, jobject obj)
{
    jvalue v;

    v.j = (*env)->GetLongField(env, obj, F_SQLite_Database_handle);
    return (void *) v.l;
}

#if HAVE_SQLITE_COMPILE
static void *
gethvm(JNIEnv *env, jobject obj)
{
    jvalue v;

    v.j = (*env)->GetLongField(env, obj, F_SQLite_Vm_handle);
    return (void *) v.l;
}

#if HAVE_SQLITE3
static void *
gethstmt(JNIEnv *env, jobject obj)
{
    jvalue v;

    v.j = (*env)->GetLongField(env, obj, F_SQLite_Stmt_handle);
    return (void *) v.l;
}
#endif
#endif

#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
static void *
gethbl(JNIEnv *env, jobject obj)
{
    jvalue v;

    v.j = (*env)->GetLongField(env, obj, F_SQLite_Blob_handle);
    return (void *) v.l;
}
#endif

#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
static void *
gethbk(JNIEnv *env, jobject obj)
{
    jvalue v;

    v.j = (*env)->GetLongField(env, obj, F_SQLite_Backup_handle);
    return (void *) v.l;
}
#endif

static void
delglobrefp(JNIEnv *env, jobject *obj)
{
    if (*obj) {
	(*env)->DeleteGlobalRef(env, *obj);
	*obj = 0;
    }
}

static jobject
globrefpop(JNIEnv *env, jobject *obj)
{
    jobject ret = 0;

    if (*obj) {
	ret = *obj;
	*obj = 0;
    }
    return ret;
}

static void
globrefset(JNIEnv *env, jobject obj, jobject *ref)
{
    if (ref) {
	if (obj) {	
	    *ref = (*env)->NewGlobalRef(env, obj);
	} else {
	    *ref = 0;
	}
    }
}

static void
freep(char **strp)
{
    if (strp && *strp) {
	free(*strp);
	*strp = 0;
    }
}

static void
throwex(JNIEnv *env, const char *msg)
{
    jclass except = (*env)->FindClass(env, "SQLite/Exception");

    (*env)->ExceptionClear(env);
    if (except) {
	(*env)->ThrowNew(env, except, msg);
    }
}

static void
throwoom(JNIEnv *env, const char *msg)
{
    jclass except = (*env)->FindClass(env, "java/lang/OutOfMemoryError");

    (*env)->ExceptionClear(env);
    if (except) {
	(*env)->ThrowNew(env, except, msg);
    }
}

static void
throwclosed(JNIEnv *env)
{
    throwex(env, "database already closed");
}

#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
static void
throwioex(JNIEnv *env, const char *msg)
{
    jclass except = (*env)->FindClass(env, "java/io/IOException");

    (*env)->ExceptionClear(env);
    if (except) {
	(*env)->ThrowNew(env, except, msg);
    }
}
#endif

static void
transfree(transstr *dest)
{
    dest->result = 0;
    freep(&dest->tofree);
}

static char *
trans2iso(JNIEnv *env, int haveutf, jstring enc, jstring src,
	  transstr *dest)
{
    jbyteArray bytes = 0;
    jthrowable exc;

    dest->result = 0;
    dest->tofree = 0;
    if (haveutf) {
#ifndef JNI_VERSION_1_2
	const char *utf = (*env)->GetStringUTFChars(env, src, 0);

	dest->result = dest->tofree = malloc(strlen(utf) + 1);
#else
	jsize utflen = (*env)->GetStringUTFLength(env, src);
	jsize uclen = (*env)->GetStringLength(env, src);

	dest->result = dest->tofree = malloc(utflen + 1);
#endif
	if (!dest->tofree) {
	    throwoom(env, "string translation failed");
	    return dest->result;
	}
#ifndef JNI_VERSION_1_2
	strcpy(dest->result, utf);
	(*env)->ReleaseStringUTFChars(env, src, utf);
#else
	(*env)->GetStringUTFRegion(env, src, 0, uclen, dest->result);
	dest->result[utflen] = '\0';
#endif
	return dest->result;
    }
    if (enc) {
	bytes = (*env)->CallObjectMethod(env, src,
					 M_java_lang_String_getBytes2, enc);
    } else {
	bytes = (*env)->CallObjectMethod(env, src,
					 M_java_lang_String_getBytes);
    }
    exc = (*env)->ExceptionOccurred(env);
    if (!exc) {
	jint len = (*env)->GetArrayLength(env, bytes);
	dest->tofree = malloc(len + 1);
	if (!dest->tofree) {
	    throwoom(env, "string translation failed");
	    return dest->result;
	}
	dest->result = dest->tofree;	
	(*env)->GetByteArrayRegion(env, bytes, 0, len, (jbyte *) dest->result);
	dest->result[len] = '\0';
    } else {
	(*env)->DeleteLocalRef(env, exc);
    }
    return dest->result;
}

static jstring
trans2utf(JNIEnv *env, int haveutf, jstring enc, const char *src,
	  transstr *dest)
{
    jbyteArray bytes = 0;
    int len;

    dest->result = 0;
    dest->tofree = 0;
    dest->jstr = 0;
    if (!src) {
	return dest->jstr;
    }
    if (haveutf) {
	dest->jstr = (*env)->NewStringUTF(env, src);
	return dest->jstr;
    }
    len = strlen(src);
    bytes = (*env)->NewByteArray(env, len);
    if (bytes) {
	(*env)->SetByteArrayRegion(env, bytes, 0, len, (jbyte *) src);
	if (enc) {
	    dest->jstr =
		(*env)->NewObject(env, C_java_lang_String,
				  M_java_lang_String_initBytes2, bytes, enc);
	} else {
	    dest->jstr =
		(*env)->NewObject(env, C_java_lang_String,
				  M_java_lang_String_initBytes, bytes);
	}
	(*env)->DeleteLocalRef(env, bytes);
	return dest->jstr;
    }
    throwoom(env, "string translation failed");
    return dest->jstr;
}

#if HAVE_SQLITE2
static int
busyhandler(void *udata, const char *table, int count)
{
    handle *h = (handle *) udata;
    JNIEnv *env = h->env;
    int ret = 0;

    if (env && h->bh) {
	transstr tabstr;
	jclass cls = (*env)->GetObjectClass(env, h->bh);
	jmethodID mid = (*env)->GetMethodID(env, cls, "busy",
					    "(Ljava/lang/String;I)Z");

	if (mid == 0) {
	    (*env)->DeleteLocalRef(env, cls);
	    return ret;
	}
	trans2utf(env, h->haveutf, h->enc, table, &tabstr);
	ret = (*env)->CallBooleanMethod(env, h->bh, mid, tabstr.jstr,
					(jint) count)
	      != JNI_FALSE;
	(*env)->DeleteLocalRef(env, tabstr.jstr);
	(*env)->DeleteLocalRef(env, cls);
    }
    return ret;
}
#endif

#if HAVE_SQLITE3
static int
busyhandler3(void *udata, int count)
{
    handle *h = (handle *) udata;
    JNIEnv *env = h->env;
    int ret = 0;

    if (env && h->bh) {
	jclass cls = (*env)->GetObjectClass(env, h->bh);
	jmethodID mid = (*env)->GetMethodID(env, cls, "busy",
					    "(Ljava/lang/String;I)Z");

	if (mid == 0) {
	    (*env)->DeleteLocalRef(env, cls);
	    return ret;
	}
	ret = (*env)->CallBooleanMethod(env, h->bh, mid, 0, (jint) count)
	    != JNI_FALSE;
	(*env)->DeleteLocalRef(env, cls);
    }
    return ret;
}
#endif

static int
progresshandler(void *udata)
{
    handle *h = (handle *) udata;
    JNIEnv *env = h->env;
    int ret = 0;

    if (env && h->ph) {
	jclass cls = (*env)->GetObjectClass(env, h->ph);
	jmethodID mid = (*env)->GetMethodID(env, cls, "progress", "()Z");

	if (mid == 0) {
	    (*env)->DeleteLocalRef(env, cls);
	    return ret;
	}
	ret = (*env)->CallBooleanMethod(env, h->ph, mid) != JNI_TRUE;
	(*env)->DeleteLocalRef(env, cls);
    }
    return ret;
}

static int
callback(void *udata, int ncol, char **data, char **cols)
{
    handle *h = (handle *) udata;
    JNIEnv *env = h->env;

    if (env && h->cb) {
	jthrowable exc;
	jclass cls = (*env)->GetObjectClass(env, h->cb);
	jmethodID mid;
	jobjectArray arr = 0;
	jint i;

	if (h->row1) {
	    mid = (*env)->GetMethodID(env, cls, "columns",
				      "([Ljava/lang/String;)V");

	    if (mid) {
		arr = (*env)->NewObjectArray(env, ncol, C_java_lang_String, 0);
		for (i = 0; i < ncol; i++) {
		    if (cols[i]) {
			transstr col;

			trans2utf(env, h->haveutf, h->enc, cols[i], &col);
			(*env)->SetObjectArrayElement(env, arr, i, col.jstr);
			exc = (*env)->ExceptionOccurred(env);
			if (exc) {
			    (*env)->DeleteLocalRef(env, exc);
			    return 1;
			}
			(*env)->DeleteLocalRef(env, col.jstr);
		    }
		}
		h->row1 = 0;
		(*env)->CallVoidMethod(env, h->cb, mid, arr);
		exc = (*env)->ExceptionOccurred(env);
		if (exc) {
		    (*env)->DeleteLocalRef(env, exc);
		    return 1;
		}
		(*env)->DeleteLocalRef(env, arr);
	    }
#if HAVE_BOTH_SQLITE
	    if (h->is3) {
		mid = (*env)->GetMethodID(env, cls, "types",
					  "([Ljava/lang/String;)V");

		if (mid && h->stmt) {
		    arr = (*env)->NewObjectArray(env, ncol,
						 C_java_lang_String, 0);
		    for (i = 0; i < ncol; i++) {
			const char *ctype =
			    sqlite3_column_decltype(h->stmt, i);

			if (!ctype) {
			    switch (sqlite3_column_type(h->stmt, i)) {
			    case SQLITE_INTEGER: ctype = "integer"; break;
			    case SQLITE_FLOAT:   ctype = "double";  break;
			    default:
#if defined(SQLITE_TEXT) && defined(SQLITE3_TEXT) && (SQLITE_TEXT != SQLITE3_TEXT)
			    case SQLITE_TEXT:
#else
#ifdef SQLITE3_TEXT
			    case SQLITE3_TEXT:
#endif
#endif
						 ctype = "text";    break;
			    case SQLITE_BLOB:    ctype = "blob";    break;
			    case SQLITE_NULL:    ctype = "null";    break;
			    }
			}
			if (ctype) {
			    transstr ty;

			    trans2utf(env, 1, 0, ctype, &ty);
			    (*env)->SetObjectArrayElement(env, arr, i,
							  ty.jstr);
			    exc = (*env)->ExceptionOccurred(env);
			    if (exc) {
				(*env)->DeleteLocalRef(env, exc);
				return 1;
			    }
			    (*env)->DeleteLocalRef(env, ty.jstr);
			}
		    }
		    (*env)->CallVoidMethod(env, h->cb, mid, arr);
		    exc = (*env)->ExceptionOccurred(env);
		    if (exc) {
			(*env)->DeleteLocalRef(env, exc);
			return 1;
		    }
		    (*env)->DeleteLocalRef(env, arr);
		}
	    } else {
		if (h->ver >= 0x020506 && cols[ncol]) {
		    mid = (*env)->GetMethodID(env, cls, "types",
					      "([Ljava/lang/String;)V");

		    if (mid) {
			arr = (*env)->NewObjectArray(env, ncol,
						     C_java_lang_String, 0);
			for (i = 0; i < ncol; i++) {
			    if (cols[i + ncol]) {
				transstr ty;

				trans2utf(env, h->haveutf, h->enc,
					  cols[i + ncol], &ty);
				(*env)->SetObjectArrayElement(env, arr, i,
							      ty.jstr);
				exc = (*env)->ExceptionOccurred(env);
				if (exc) {
				    (*env)->DeleteLocalRef(env, exc);
				    return 1;
				}
				(*env)->DeleteLocalRef(env, ty.jstr);
			    }
			}
			(*env)->CallVoidMethod(env, h->cb, mid, arr);
			exc = (*env)->ExceptionOccurred(env);
			if (exc) {
			    (*env)->DeleteLocalRef(env, exc);
			    return 1;
			}
			(*env)->DeleteLocalRef(env, arr);
		    }
		}
	    }
#else
#if HAVE_SQLITE2
	    if (h->ver >= 0x020506 && cols[ncol]) {
		mid = (*env)->GetMethodID(env, cls, "types",
					  "([Ljava/lang/String;)V");

		if (mid) {
		    arr = (*env)->NewObjectArray(env, ncol,
						 C_java_lang_String, 0);
		    for (i = 0; i < ncol; i++) {
			if (cols[i + ncol]) {
			    transstr ty;

			    trans2utf(env, h->haveutf, h->enc,
				      cols[i + ncol], &ty);
			    (*env)->SetObjectArrayElement(env, arr, i,
							  ty.jstr);
			    exc = (*env)->ExceptionOccurred(env);
			    if (exc) {
				(*env)->DeleteLocalRef(env, exc);
				return 1;
			    }
			    (*env)->DeleteLocalRef(env, ty.jstr);
			}
		    }
		    (*env)->CallVoidMethod(env, h->cb, mid, arr);
		    exc = (*env)->ExceptionOccurred(env);
		    if (exc) {
			(*env)->DeleteLocalRef(env, exc);
			return 1;
		    }
		    (*env)->DeleteLocalRef(env, arr);
		}
	    }
#endif
#if HAVE_SQLITE3
	    mid = (*env)->GetMethodID(env, cls, "types",
				      "([Ljava/lang/String;)V");

	    if (mid && h->stmt) {
		arr = (*env)->NewObjectArray(env, ncol,
					     C_java_lang_String, 0);
		for (i = 0; i < ncol; i++) {
		    const char *ctype = sqlite3_column_decltype(h->stmt, i);

		    if (!ctype) {
			switch (sqlite3_column_type(h->stmt, i)) {
			case SQLITE_INTEGER: ctype = "integer"; break;
			case SQLITE_FLOAT:   ctype = "double";  break;
			default:
#if defined(SQLITE_TEXT) && defined(SQLITE3_TEXT) && (SQLITE_TEXT != SQLITE3_TEXT)
			case SQLITE_TEXT:
#else
#ifdef SQLITE3_TEXT
			case SQLITE3_TEXT:
#endif
#endif
					     ctype = "text";    break;
			case SQLITE_BLOB:    ctype = "blob";    break;
			case SQLITE_NULL:    ctype = "null";    break;
			}
		    }
		    if (ctype) {
			transstr ty;

			trans2utf(env, 1, 0, ctype, &ty);
			(*env)->SetObjectArrayElement(env, arr, i, ty.jstr);
			exc = (*env)->ExceptionOccurred(env);
			if (exc) {
			    (*env)->DeleteLocalRef(env, exc);
			    return 1;
			}
			(*env)->DeleteLocalRef(env, ty.jstr);
		    }
		}
		(*env)->CallVoidMethod(env, h->cb, mid, arr);
		exc = (*env)->ExceptionOccurred(env);
		if (exc) {
		    (*env)->DeleteLocalRef(env, exc);
		    return 1;
		}
		(*env)->DeleteLocalRef(env, arr);
	    }
#endif
#endif
	}
	if (data) {
	    mid = (*env)->GetMethodID(env, cls, "newrow",
				      "([Ljava/lang/String;)Z");
	    if (mid) {
		jboolean rc;

		arr = (*env)->NewObjectArray(env, ncol, C_java_lang_String, 0);
		for (i = 0; arr && i < ncol; i++) {
		    if (data[i]) {
			transstr dats;

			trans2utf(env, h->haveutf, h->enc, data[i], &dats);
			(*env)->SetObjectArrayElement(env, arr, i, dats.jstr);
			exc = (*env)->ExceptionOccurred(env);
			if (exc) {
			    (*env)->DeleteLocalRef(env, exc);
			    return 1;
			}
			(*env)->DeleteLocalRef(env, dats.jstr);
		    }
		}
		rc = (*env)->CallBooleanMethod(env, h->cb, mid, arr);
		exc = (*env)->ExceptionOccurred(env);
		if (exc) {
		    (*env)->DeleteLocalRef(env, exc);
		    return 1;
		}
		if (arr) {
		    (*env)->DeleteLocalRef(env, arr);
		}
		(*env)->DeleteLocalRef(env, cls);
		return rc != JNI_FALSE;
	    }
	}
    }
    return 0;
}

static void
doclose(JNIEnv *env, jobject obj, int final)
{
    handle *h = gethandle(env, obj);

    if (h) {
	hfunc *f;
#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
	hbl *bl;
#endif
#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
	hbk *bk;
#endif
#if HAVE_SQLITE_COMPILE
	hvm *v;

	while ((v = h->vms)) {
	    h->vms = v->next;
	    v->next = 0;
	    v->h = 0;
	    if (v->vm) {
#if HAVE_BOTH_SQLITE
		if (h->is3) {
		    sqlite3_finalize((sqlite3_stmt *) v->vm);
		} else {
		    sqlite_finalize((sqlite_vm *) v->vm, 0);
		}
#else
#if HAVE_SQLITE2
		sqlite_finalize((sqlite_vm *) v->vm, 0);
#endif
#if HAVE_SQLITE3
		sqlite3_finalize((sqlite3_stmt *) v->vm);
#endif
#endif
		v->vm = 0;
	    }
	}
#endif
	if (h->sqlite) {
#if HAVE_BOTH_SQLITE
	    if (h->is3) {
		sqlite3_close((sqlite3 *) h->sqlite);
	    } else {
		sqlite_close((sqlite *) h->sqlite);
	    }
#else
#if HAVE_SQLITE2
	    sqlite_close((sqlite *) h->sqlite);
#endif
#if HAVE_SQLITE3
	    sqlite3_close((sqlite3 *) h->sqlite);
#endif
#endif
	    h->sqlite = 0;
	}
	while ((f = h->funcs)) {
	    h->funcs = f->next;
	    f->h = 0;
	    f->sf = 0;
	    f->env = 0;
	    if (f->fc) {
		(*env)->SetLongField(env, f->fc,
				     F_SQLite_FunctionContext_handle, 0);
	    }
	    delglobrefp(env, &f->db);
	    delglobrefp(env, &f->fi);
	    delglobrefp(env, &f->fc);
	    free(f);
	}
#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
	while ((bl = h->blobs)) {
	    h->blobs = bl->next;
	    bl->next = 0;
	    bl->h = 0;
	    if (bl->blob) {
		sqlite3_blob_close(bl->blob);
	    }
	    bl->blob = 0;
	}
#endif
#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
	while ((bk = h->backups)) {
	    h->backups = bk->next;
	    bk->next = 0;
	    bk->h = 0;
	    if (bk->bkup) {
		sqlite3_backup_finish(bk->bkup);
	    }
	    bk->bkup = 0;
	}
#endif
	delglobrefp(env, &h->bh);
	delglobrefp(env, &h->cb);
	delglobrefp(env, &h->ai);
	delglobrefp(env, &h->tr);
	delglobrefp(env, &h->ph);
	delglobrefp(env, &h->enc);
	free(h);
	(*env)->SetLongField(env, obj, F_SQLite_Database_handle, 0);
	return;
    }
    if (!final) {
	throwclosed(env);
    }
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1close(JNIEnv *env, jobject obj)
{
    doclose(env, obj, 0);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1finalize(JNIEnv *env, jobject obj)
{
    doclose(env, obj, 1);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1busy_1timeout(JNIEnv *env, jobject obj, jint ms)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    sqlite3_busy_timeout((sqlite3 * ) h->sqlite, ms);
	} else {
	    sqlite_busy_timeout((sqlite *) h->sqlite, ms);
	}
#else
#if HAVE_SQLITE2
	sqlite_busy_timeout((sqlite *) h->sqlite, ms);
#endif
#if HAVE_SQLITE3
	sqlite3_busy_timeout((sqlite3 * ) h->sqlite, ms);
#endif
#endif
	return;
    }
    throwclosed(env);
}

JNIEXPORT jstring JNICALL
Java_SQLite_Database_version(JNIEnv *env, jclass cls)
{
    /* CHECK THIS */
#if HAVE_BOTH_SQLITE
    return (*env)->NewStringUTF(env, sqlite_libversion());
#else
#if HAVE_SQLITE2
    return (*env)->NewStringUTF(env, sqlite_libversion());
#else
    return (*env)->NewStringUTF(env, sqlite3_libversion());
#endif
#endif
}

JNIEXPORT jstring JNICALL
Java_SQLite_Database_dbversion(JNIEnv *env, jobject obj)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    return (*env)->NewStringUTF(env, sqlite3_libversion());
	} else {
	    return (*env)->NewStringUTF(env, sqlite_libversion());
	}
#else
#if HAVE_SQLITE2
	return (*env)->NewStringUTF(env, sqlite_libversion());
#else
	return (*env)->NewStringUTF(env, sqlite3_libversion());
#endif
#endif
    }
    return (*env)->NewStringUTF(env, "unknown");
}

JNIEXPORT jlong JNICALL
Java_SQLite_Database__1last_1insert_1rowid(JNIEnv *env, jobject obj)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    return (jlong) sqlite3_last_insert_rowid((sqlite3 *) h->sqlite);
	} else {
	    return (jlong) sqlite_last_insert_rowid((sqlite *) h->sqlite);
	}
#else
#if HAVE_SQLITE2
	return (jlong) sqlite_last_insert_rowid((sqlite *) h->sqlite);
#endif
#if HAVE_SQLITE3
	return (jlong) sqlite3_last_insert_rowid((sqlite3 *) h->sqlite);
#endif
#endif
    }
    throwclosed(env);
    return (jlong) 0;
}

JNIEXPORT jlong JNICALL
Java_SQLite_Database__1changes(JNIEnv *env, jobject obj)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    return (jlong) sqlite3_changes((sqlite3 *) h->sqlite);
	} else {
	    return (jlong) sqlite_changes((sqlite *) h->sqlite);
	}
#else
#if HAVE_SQLITE2
	return (jlong) sqlite_changes((sqlite *) h->sqlite);
#endif
#if HAVE_SQLITE3
	return (jlong) sqlite3_changes((sqlite3 *) h->sqlite);
#endif
#endif
    }
    throwclosed(env);
    return (jlong) 0;
}

JNIEXPORT jboolean JNICALL
Java_SQLite_Database__1complete(JNIEnv *env, jclass cls, jstring sql)
{
    transstr sqlstr;
    jboolean result;

    if (!sql) {
	return JNI_FALSE;
    }
#if HAVE_BOTH_SQLITE || HAVE_SQLITE3
    /* CHECK THIS */
    trans2iso(env, 1, 0, sql, &sqlstr);
    result = sqlite3_complete(sqlstr.result) ? JNI_TRUE : JNI_FALSE;
#else
    trans2iso(env, strcmp(sqlite_libencoding(), "UTF-8") == 0, 0,
	      sql, &sqlstr);
    result = sqlite_complete(sqlstr.result) ? JNI_TRUE : JNI_FALSE;
#endif
    transfree(&sqlstr);
    return result;
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1interrupt(JNIEnv *env, jobject obj)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    sqlite3_interrupt((sqlite3 *) h->sqlite);
	} else {
	    sqlite_interrupt((sqlite *) h->sqlite);
	}
#else
#if HAVE_SQLITE2
	sqlite_interrupt((sqlite *) h->sqlite);
#endif
#if HAVE_SQLITE3
	sqlite3_interrupt((sqlite3 *) h->sqlite);
#endif
#endif
	return;
    }
    throwclosed(env);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1open4(JNIEnv *env, jobject obj, jstring file, jint mode,
			     jstring vfs, jboolean ver2)
{
    handle *h = gethandle(env, obj);
    jthrowable exc;
    char *err = 0;
    transstr filename;
    int maj, min, lev;
#if HAVE_SQLITE3_OPEN_V2
    transstr vfsname;

    vfsname.result = 0;
    vfsname.tofree = 0;
    vfsname.jstr = 0;
#endif

    if (h) {
	if (h->sqlite) {
#if HAVE_BOTH_SQLITE
	    if (h->is3) {
		sqlite3_close((sqlite3 *) h->sqlite);
	    } else {
		sqlite_close((sqlite *) h->sqlite);
	    }
	    h->is3 = 0;
#else
#if HAVE_SQLITE2
	    sqlite_close((sqlite *) h->sqlite);
#endif
#if HAVE_SQLITE3
	    sqlite3_close((sqlite3 *) h->sqlite);
#endif
#endif
	    h->sqlite = 0;
	}
    } else {
	h = malloc(sizeof (handle));
	if (!h) {
	    throwoom(env, "unable to get SQLite handle");
	    return;
	}
	h->sqlite = 0;
	h->bh = h->cb = h->ai = h->tr = h->pr = h->ph = 0;
	/* CHECK THIS */
#if HAVE_BOTH_SQLITE
	h->is3 = 0;
	h->stmt = 0;
	h->haveutf = 1;
#else
#if HAVE_SQLITE2
	h->haveutf = strcmp(sqlite_libencoding(), "UTF-8") == 0;
#endif
#if HAVE_SQLITE3
	h->stmt = 0;
	h->haveutf = 1;
#endif
#endif
	h->enc = 0;
	h->funcs = 0;
	h->ver = 0;
#if HAVE_SQLITE_COMPILE
	h->vms = 0;
#endif
#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
	h->blobs = 0;
#endif
#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
	h->backups = 0;
#endif
    }
    h->env = 0;
    if (!file) {
	throwex(env, err ? err : "invalid file name");
	return;
    }
    trans2iso(env, h->haveutf, h->enc, file, &filename);
    exc = (*env)->ExceptionOccurred(env);
    if (exc) {
	(*env)->DeleteLocalRef(env, exc);
	return;
    }
#if HAVE_SQLITE3_OPEN_V2
    if (vfs) {
	trans2iso(env, 1, h->enc, vfs, &vfsname);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    transfree(&filename);
	    (*env)->DeleteLocalRef(env, exc);
	    return;
	}
    }
#endif
#if HAVE_BOTH_SQLITE
    {
	FILE *f = fopen(filename.result, "rb");
	int c_0 = EOF;

	if (f) {
	    c_0 = fgetc(f);
	    fclose(f);
	}
	if (c_0 != '*' && ver2 == JNI_FALSE) {
#if HAVE_SQLITE3_OPEN_V2
	    int rc = sqlite3_open_v2(filename.result, (sqlite3 **) &h->sqlite,
				     (int) mode, vfsname.result);
#else
	    int rc = sqlite3_open(filename.result, (sqlite3 **) &h->sqlite);
#endif

	    if (rc == SQLITE_OK) {
		h->is3 = 1;
	    } else if (h->sqlite) {
		sqlite3_close((sqlite3 *) h->sqlite);
		h->sqlite = 0;
	    }
	} else {
	    h->sqlite = (void *) sqlite_open(filename.result,
					     (int) mode, &err);
	}
    }
#else
#if HAVE_SQLITE2
    h->sqlite = (void *) sqlite_open(filename.result, (int) mode, &err);
#endif
#if HAVE_SQLITE3
#if HAVE_SQLITE3_OPEN_V2
    if (sqlite3_open_v2(filename.result, (sqlite3 **) &h->sqlite,
			(int) mode, vfsname.result) != SQLITE_OK)
#else
    if (sqlite3_open(filename.result, (sqlite3 **) &h->sqlite) != SQLITE_OK)
#endif
    {
	if (h->sqlite) {
	    sqlite3_close((sqlite3 *) h->sqlite);
	    h->sqlite = 0;
	}
    }
#endif
#endif
    transfree(&filename);
#if HAVE_SQLITE3_OPEN_V2
    transfree(&vfsname);
#endif
    exc = (*env)->ExceptionOccurred(env);
    if (exc) {
	(*env)->DeleteLocalRef(env, exc);
#if HAVE_SQLITE2
	if (err) {
	    sqlite_freemem(err);
	}
#endif
	if (h->sqlite) {
#if HAVE_BOTH_SQLITE
	    if (h->is3) {
		sqlite3_close((sqlite3 *) h->sqlite);
		h->is3 = 0;
	    } else {
		sqlite_close((sqlite *) h->sqlite);
	    }
#else
#if HAVE_SQLITE2
	    sqlite_close((sqlite *) h->sqlite);
#endif
#if HAVE_SQLITE3
	    sqlite3_close((sqlite3 *) h->sqlite);
#endif
#endif
	}
	h->sqlite = 0;
	return;
    }
    if (h->sqlite) {
	jvalue v;

	v.j = 0;
	v.l = (jobject) h;
	(*env)->SetLongField(env, obj, F_SQLite_Database_handle, v.j);
#if HAVE_SQLITE2
	if (err) {
	    sqlite_freemem(err);
	}
#endif
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    sscanf(sqlite3_libversion(), "%d.%d.%d", &maj, &min, &lev);
#if HAVE_SQLITE3_LOAD_EXTENSION
	    sqlite3_enable_load_extension((sqlite3 *) h->sqlite, 1);
#endif
	} else {
	    sscanf(sqlite_libversion(), "%d.%d.%d", &maj, &min, &lev);
	}
#else
#if HAVE_SQLITE2
	sscanf(sqlite_libversion(), "%d.%d.%d", &maj, &min, &lev);
#endif
#if HAVE_SQLITE3
	sscanf(sqlite3_libversion(), "%d.%d.%d", &maj, &min, &lev);
#if HAVE_SQLITE3_LOAD_EXTENSION
	sqlite3_enable_load_extension((sqlite3 *) h->sqlite, 1);
#endif
#endif
#endif
	h->ver = ((maj & 0xFF) << 16) | ((min & 0xFF) << 8) | (lev & 0xFF);
	return;
    }
    throwex(env, err ? err : "unknown error in open");
#if HAVE_SQLITE2
    if (err) {
	sqlite_freemem(err);
    }
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1open(JNIEnv *env, jobject obj, jstring file, jint mode)
{
    Java_SQLite_Database__1open4(env, obj, file, mode, 0, 0);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1open_1aux_1file(JNIEnv *env, jobject obj, jstring file)
{
    handle *h = gethandle(env, obj);
#if HAVE_SQLITE_OPEN_AUX_FILE
    jthrowable exc;
    char *err = 0;
    transstr filename;
    int ret;
#endif

    if (h && h->sqlite) {
#if HAVE_SQLITE_OPEN_AUX_FILE
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    throwex(env, "unsupported");
	}
#endif
	trans2iso(env, h->haveutf, h->enc, file, &filename);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    (*env)->DeleteLocalRef(env, exc);
	    return;
	}
	ret = sqlite_open_aux_file((sqlite *) h->sqlite,
				   filename.result, &err);
	transfree(&filename);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    (*env)->DeleteLocalRef(env, exc);
	    if (err) {
		sqlite_freemem(err);
	    }
	    return;
	}
	if (ret != SQLITE_OK) {
	    throwex(env, err ? err : sqlite_error_string(ret));
	}
	if (err) {
	    sqlite_freemem(err);
	}
#else
	throwex(env, "unsupported");
#endif
	return;
    }
    throwclosed(env);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1busy_1handler(JNIEnv *env, jobject obj, jobject bh)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
	delglobrefp(env, &h->bh);
	globrefset(env, bh, &h->bh);
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    sqlite3_busy_handler((sqlite3 *) h->sqlite, busyhandler3, h);
	} else {
	    sqlite_busy_handler((sqlite *) h->sqlite, busyhandler, h);
	}
#else
#if HAVE_SQLITE2
	sqlite_busy_handler((sqlite *) h->sqlite, busyhandler, h);
#endif
#if HAVE_SQLITE3
	sqlite3_busy_handler((sqlite3 *) h->sqlite, busyhandler3, h);
#endif
#endif
	return;
    }
    throwclosed(env);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1exec__Ljava_lang_String_2LSQLite_Callback_2
    (JNIEnv *env, jobject obj, jstring sql, jobject cb)
{
    handle *h = gethandle(env, obj);
    freemem *freeproc;

    if (!sql) {
	throwex(env, "invalid SQL statement");
	return;
    }
    if (h) {
	if (h->sqlite) {
	    jthrowable exc;
	    int rc;
	    char *err = 0;
	    transstr sqlstr;
	    jobject oldcb = globrefpop(env, &h->cb);

	    globrefset(env, cb, &h->cb);
	    h->env = env;
	    h->row1 = 1;
	    trans2iso(env, h->haveutf, h->enc, sql, &sqlstr);
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		return;
	    }
#if HAVE_BOTH_SQLITE
	    if (h->is3) {
		rc = sqlite3_exec((sqlite3 *) h->sqlite, sqlstr.result,
				  callback, h, &err);
		freeproc = (freemem *) sqlite3_free;
	    } else {
		rc = sqlite_exec((sqlite *) h->sqlite, sqlstr.result,
				 callback, h, &err);
		freeproc = (freemem *) sqlite_freemem;
	    }
#else
#if HAVE_SQLITE2
	    rc = sqlite_exec((sqlite *) h->sqlite, sqlstr.result,
			     callback, h, &err);
	    freeproc = (freemem *) sqlite_freemem;
#endif
#if HAVE_SQLITE3
	    rc = sqlite3_exec((sqlite3 *) h->sqlite, sqlstr.result,
			      callback, h, &err);
	    freeproc = (freemem *) sqlite3_free;
#endif
#endif
	    transfree(&sqlstr);
	    exc = (*env)->ExceptionOccurred(env);
	    delglobrefp(env, &h->cb);
	    h->cb = oldcb;
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		if (err) {
		    freeproc(err);
		}
		return;
	    }
	    if (rc != SQLITE_OK) {
		char msg[128];

		seterr(env, obj, rc);
		if (!err) {
		    sprintf(msg, "error %d in sqlite*_exec", rc);
		}
		throwex(env, err ? err : msg);
	    }
	    if (err) {
		freeproc(err);
	    }
	    return;
	}
    }
    throwclosed(env);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1exec__Ljava_lang_String_2LSQLite_Callback_2_3Ljava_lang_String_2
    (JNIEnv *env, jobject obj, jstring sql, jobject cb, jobjectArray args)
{
    handle *h = gethandle(env, obj);
    freemem *freeproc = 0;

    if (!sql) {
	throwex(env, "invalid SQL statement");
	return;
    }
    if (h) {
	if (h->sqlite) {
	    jthrowable exc;
	    int rc = SQLITE_ERROR, nargs, i;
	    char *err = 0, *p;
	    const char *str = (*env)->GetStringUTFChars(env, sql, 0);
	    transstr sqlstr;
	    struct args {
		char *arg;
		jobject obj;
		transstr trans;
	    } *argv = 0;
	    char **cargv = 0;
	    jobject oldcb = globrefpop(env, &h->cb);

	    globrefset(env, cb, &h->cb);
	    p = (char *) str;
	    nargs = 0;
	    while (*p) {
		if (*p == '%') {
		    ++p;
		    if (*p == 'q' || *p == 's') {
			nargs++;
			if (nargs > MAX_PARAMS) {
			    (*env)->ReleaseStringUTFChars(env, sql, str);
			    delglobrefp(env, &h->cb);
			    h->cb = oldcb;
			    throwex(env, "too much SQL parameters");
			    return;
			}
		    } else if (h->ver >= 0x020500 && *p == 'Q') {
			nargs++;
			if (nargs > MAX_PARAMS) {
			    (*env)->ReleaseStringUTFChars(env, sql, str);
			    delglobrefp(env, &h->cb);
			    h->cb = oldcb;
			    throwex(env, "too much SQL parameters");
			    return;
			}
		    } else if (*p != '%') {
			(*env)->ReleaseStringUTFChars(env, sql, str);
			delglobrefp(env, &h->cb);
			h->cb = oldcb;
			throwex(env, "bad % specification in query");
			return;
		    }
		}
		++p;
	    }
	    cargv = malloc((sizeof (*argv) + sizeof (char *))
			   * MAX_PARAMS);
	    if (!cargv) {
		(*env)->ReleaseStringUTFChars(env, sql, str);
		delglobrefp(env, &h->cb);
		h->cb = oldcb;
		throwoom(env, "unable to allocate arg vector");
		return;
	    }
	    argv = (struct args *) (cargv + MAX_PARAMS);
	    for (i = 0; i < MAX_PARAMS; i++) {
		cargv[i] = 0;
		argv[i].arg = 0;
		argv[i].obj = 0;
		argv[i].trans.result = argv[i].trans.tofree = 0;
	    }
	    exc = 0;
	    for (i = 0; i < nargs; i++) {
		jobject so = (*env)->GetObjectArrayElement(env, args, i);

		exc = (*env)->ExceptionOccurred(env);
		if (exc) {
		    (*env)->DeleteLocalRef(env, exc);
		    break;
		}
		if (so) {
		    argv[i].obj = so;
		    argv[i].arg = cargv[i] =
			trans2iso(env, h->haveutf, h->enc, argv[i].obj,
				  &argv[i].trans);
		}
	    }
	    if (exc) {
		for (i = 0; i < nargs; i++) {
		    if (argv[i].obj) {
			transfree(&argv[i].trans);
		    }
		}
		freep((char **) &cargv);
		(*env)->ReleaseStringUTFChars(env, sql, str);
		delglobrefp(env, &h->cb);
		h->cb = oldcb;
		return;
	    }
	    h->env = env;
	    h->row1 = 1;
	    trans2iso(env, h->haveutf, h->enc, sql, &sqlstr);
	    exc = (*env)->ExceptionOccurred(env);
	    if (!exc) {
#if HAVE_BOTH_SQLITE
		if (h->is3) {
#if defined(_WIN32) || !defined(CANT_PASS_VALIST_AS_CHARPTR)
		    char *s = sqlite3_vmprintf(sqlstr.result, (char *) cargv);
#else
		    char *s = sqlite3_mprintf(sqlstr.result,
					      cargv[0], cargv[1],
					      cargv[2], cargv[3],
					      cargv[4], cargv[5],
					      cargv[6], cargv[7],
					      cargv[8], cargv[9],
					      cargv[10], cargv[11],
					      cargv[12], cargv[13],
					      cargv[14], cargv[15],
					      cargv[16], cargv[17],
					      cargv[18], cargv[19],
					      cargv[20], cargv[21],
					      cargv[22], cargv[23],
					      cargv[24], cargv[25],
					      cargv[26], cargv[27],
					      cargv[28], cargv[29],
					      cargv[30], cargv[31]);
#endif

		    if (s) {
			rc = sqlite3_exec((sqlite3 *) h->sqlite, s, callback,
					  h, &err);
			sqlite3_free(s);
		    } else {
			rc = SQLITE_NOMEM;
		    }
		    freeproc = (freemem *) sqlite3_free;
		} else {
#if defined(_WIN32) || !defined(CANT_PASS_VALIST_AS_CHARPTR)
		    rc = sqlite_exec_vprintf((sqlite *) h->sqlite,
					     sqlstr.result, callback, h, &err,
					     (char *) cargv);
#else
		    rc = sqlite_exec_printf((sqlite *) h->sqlite,
					    sqlstr.result, callback,
					    h, &err,
					    cargv[0], cargv[1],
					    cargv[2], cargv[3],
					    cargv[4], cargv[5],
					    cargv[6], cargv[7],
					    cargv[8], cargv[9],
					    cargv[10], cargv[11],
					    cargv[12], cargv[13],
					    cargv[14], cargv[15],
					    cargv[16], cargv[17],
					    cargv[18], cargv[19],
					    cargv[20], cargv[21],
					    cargv[22], cargv[23],
					    cargv[24], cargv[25],
					    cargv[26], cargv[27],
					    cargv[28], cargv[29],
					    cargv[30], cargv[31]);
#endif
		    freeproc = (freemem *) sqlite_freemem;
		}
#else
#if HAVE_SQLITE2
#if defined(_WIN32) || !defined(CANT_PASS_VALIST_AS_CHARPTR)
		rc = sqlite_exec_vprintf((sqlite *) h->sqlite, sqlstr.result,
					 callback, h, &err, (char *) cargv);
#else
		rc = sqlite_exec_printf((sqlite *) h->sqlite, sqlstr.result,
					callback, h, &err,
					cargv[0], cargv[1],
					cargv[2], cargv[3],
					cargv[4], cargv[5],
					cargv[6], cargv[7],
					cargv[8], cargv[9],
					cargv[10], cargv[11],
					cargv[12], cargv[13],
					cargv[14], cargv[15],
					cargv[16], cargv[17],
					cargv[18], cargv[19],
					cargv[20], cargv[21],
					cargv[22], cargv[23],
					cargv[24], cargv[25],
					cargv[26], cargv[27],
					cargv[28], cargv[29],
					cargv[30], cargv[31]);
#endif
		freeproc = (freemem *) sqlite_freemem;
#endif
#if HAVE_SQLITE3
#if defined(_WIN32) || !defined(CANT_PASS_VALIST_AS_CHARPTR)
		char *s = sqlite3_vmprintf(sqlstr.result, (char *) cargv);
#else
		char *s = sqlite3_mprintf(sqlstr.result,
					  cargv[0], cargv[1],
					  cargv[2], cargv[3],
					  cargv[4], cargv[5],
					  cargv[6], cargv[7],
					  cargv[8], cargv[9],
					  cargv[10], cargv[11],
					  cargv[12], cargv[13],
					  cargv[14], cargv[15],
					  cargv[16], cargv[17],
					  cargv[18], cargv[19],
					  cargv[20], cargv[21],
					  cargv[22], cargv[23],
					  cargv[24], cargv[25],
					  cargv[26], cargv[27],
					  cargv[28], cargv[29],
					  cargv[30], cargv[31]);
#endif

		if (s) {
		    rc = sqlite3_exec((sqlite3 *) h->sqlite, s, callback,
				      h, &err);
		    sqlite3_free(s);
		} else {
		    rc = SQLITE_NOMEM;
		}
		freeproc = (freemem *) sqlite3_free;
#endif
#endif
		exc = (*env)->ExceptionOccurred(env);
	    }
	    for (i = 0; i < nargs; i++) {
		if (argv[i].obj) {
		    transfree(&argv[i].trans);
		}
	    }
	    transfree(&sqlstr);
	    (*env)->ReleaseStringUTFChars(env, sql, str);
	    freep((char **) &cargv);
	    delglobrefp(env, &h->cb);
	    h->cb = oldcb;
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		if (err && freeproc) {
		    freeproc(err);
		}
		return;
	    }
	    if (rc != SQLITE_OK) {
		char msg[128];

		seterr(env, obj, rc);
		if (!err) {
		    sprintf(msg, "error %d in sqlite*_exec", rc);
		}
		throwex(env, err ? err : msg);
	    }
	    if (err && freeproc) {
		freeproc(err);
	    }
	    return;
	}
    }
    throwclosed(env);
}

static hfunc *
getfunc(JNIEnv *env, jobject obj)
{
    jvalue v;

    v.j = (*env)->GetLongField(env, obj, F_SQLite_FunctionContext_handle);
    return (hfunc *) v.l;
}

#if HAVE_SQLITE2
static void
call_common(sqlite_func *sf, int isstep, int nargs, const char **args)
{
    hfunc *f = (hfunc *) sqlite_user_data(sf);

    if (f && f->env && f->fi) {
	JNIEnv *env = f->env;
	jclass cls = (*env)->GetObjectClass(env, f->fi);
	jmethodID mid =
	    (*env)->GetMethodID(env, cls,
				isstep ? "step" : "function",
				"(LSQLite/FunctionContext;[Ljava/lang/String;)V");
	jobjectArray arr;
	int i;

	if (mid == 0) {
	    (*env)->DeleteLocalRef(env, cls);
	    return;
	}
	arr = (*env)->NewObjectArray(env, nargs, C_java_lang_String, 0);
	for (i = 0; i < nargs; i++) {
	    if (args[i]) {
		transstr arg;
		jthrowable exc;

		trans2utf(env, f->h->haveutf, f->h->enc, args[i], &arg);
		(*env)->SetObjectArrayElement(env, arr, i, arg.jstr);
		exc = (*env)->ExceptionOccurred(env);
		if (exc) {
		    (*env)->DeleteLocalRef(env, exc);
		    return;
		}
		(*env)->DeleteLocalRef(env, arg.jstr);
	    }
	}
	f->sf = sf;
	(*env)->CallVoidMethod(env, f->fi, mid, f->fc, arr);
	(*env)->DeleteLocalRef(env, arr);
	(*env)->DeleteLocalRef(env, cls);
    }
}

static void
call_func(sqlite_func *sf, int nargs, const char **args)
{
    call_common(sf, 0, nargs, args);
}

static void
call_step(sqlite_func *sf, int nargs, const char **args)
{
    call_common(sf, 1, nargs, args);
}

static void
call_final(sqlite_func *sf)
{
    hfunc *f = (hfunc *) sqlite_user_data(sf);

    if (f && f->env && f->fi) {
	JNIEnv *env = f->env;
	jclass cls = (*env)->GetObjectClass(env, f->fi);
	jmethodID mid = (*env)->GetMethodID(env, cls, "last_step",
					    "(LSQLite/FunctionContext;)V");
	if (mid == 0) {
	    (*env)->DeleteLocalRef(env, cls);
	    return;
	}
	f->sf = sf;
	(*env)->CallVoidMethod(env, f->fi, mid, f->fc);
	(*env)->DeleteLocalRef(env, cls);
    }
}
#endif

#if HAVE_SQLITE3
static void
call3_common(sqlite3_context *sf, int isstep, int nargs, sqlite3_value **args)
{
    hfunc *f = (hfunc *) sqlite3_user_data(sf);

    if (f && f->env && f->fi) {
	JNIEnv *env = f->env;
	jclass cls = (*env)->GetObjectClass(env, f->fi);
	jmethodID mid =
	    (*env)->GetMethodID(env, cls,
				isstep ? "step" : "function",
				"(LSQLite/FunctionContext;[Ljava/lang/String;)V");
	jobjectArray arr;
	int i;

	if (mid == 0) {
	    (*env)->DeleteLocalRef(env, cls);
	    return;
	}
	arr = (*env)->NewObjectArray(env, nargs, C_java_lang_String, 0);
	for (i = 0; i < nargs; i++) {
	    if (args[i]) {
		transstr arg;
		jthrowable exc;

		trans2utf(env, 1, 0, (char *) sqlite3_value_text(args[i]),
			  &arg);
		(*env)->SetObjectArrayElement(env, arr, i, arg.jstr);
		exc = (*env)->ExceptionOccurred(env);
		if (exc) {
		    (*env)->DeleteLocalRef(env, exc);
		    return;
		}
		(*env)->DeleteLocalRef(env, arg.jstr);
	    }
	}
	f->sf = sf;
	(*env)->CallVoidMethod(env, f->fi, mid, f->fc, arr);
	(*env)->DeleteLocalRef(env, arr);
	(*env)->DeleteLocalRef(env, cls);
    }
}

static void
call3_func(sqlite3_context *sf, int nargs, sqlite3_value **args)
{
    call3_common(sf, 0, nargs, args);
}

static void
call3_step(sqlite3_context *sf, int nargs, sqlite3_value **args)
{
    call3_common(sf, 1, nargs, args);
}

static void
call3_final(sqlite3_context *sf)
{
    hfunc *f = (hfunc *) sqlite3_user_data(sf);

    if (f && f->env && f->fi) {
	JNIEnv *env = f->env;
	jclass cls = (*env)->GetObjectClass(env, f->fi);
	jmethodID mid = (*env)->GetMethodID(env, cls, "last_step",
					    "(LSQLite/FunctionContext;)V");
	if (mid == 0) {
	    (*env)->DeleteLocalRef(env, cls);
	    return;
	}
	f->sf = sf;
	(*env)->CallVoidMethod(env, f->fi, mid, f->fc);
	(*env)->DeleteLocalRef(env, cls);
    }
}
#endif

static void
mkfunc_common(JNIEnv *env, int isagg, jobject obj, jstring name,
	      jint nargs, jobject fi)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
	jclass cls = (*env)->FindClass(env, "SQLite/FunctionContext");
	jobject fc;
	hfunc *f;
	int ret;
	transstr namestr;
	jvalue v;
	jthrowable exc;

	fc = (*env)->AllocObject(env, cls);
	if (!fi) {
	    throwex(env, "null SQLite.Function not allowed");
	    return;
	}
	f = malloc(sizeof (hfunc));
	if (!f) {
	    throwoom(env, "unable to get SQLite.FunctionContext handle");
	    return;
	}
	globrefset(env, fc, &f->fc);
	globrefset(env, fi, &f->fi);
	globrefset(env, obj, &f->db);
	f->h = h;
	f->next = h->funcs;
	h->funcs = f;
	f->sf = 0;
	f->env = env;
	v.j = 0;
	v.l = (jobject) f;
	(*env)->SetLongField(env, f->fc, F_SQLite_FunctionContext_handle, v.j);
	trans2iso(env, h->haveutf, h->enc, name, &namestr);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    (*env)->DeleteLocalRef(env, exc);
	    return;
	}
#if HAVE_BOTH_SQLITE
	f->is3 = h->is3;
	if (h->is3) {
	    ret = sqlite3_create_function((sqlite3 *) h->sqlite,
					  namestr.result,
					  (int) nargs,
					  SQLITE_UTF8, f,
					  isagg ? NULL : call3_func,
					  isagg ? call3_step : NULL,
					  isagg ? call3_final : NULL);

	} else {
	    if (isagg) {
		ret = sqlite_create_aggregate((sqlite *) h->sqlite,
					      namestr.result,
					      (int) nargs,
					      call_step, call_final, f);
	    } else {
		ret = sqlite_create_function((sqlite *) h->sqlite,
					     namestr.result,
					     (int) nargs,
					     call_func, f);
	    }
	}
#else
#if HAVE_SQLITE2
	if (isagg) {
	    ret = sqlite_create_aggregate((sqlite *) h->sqlite, namestr.result,
					  (int) nargs,
					  call_step, call_final, f);
	} else {
	    ret = sqlite_create_function((sqlite *) h->sqlite, namestr.result,
					 (int) nargs,
					 call_func, f);
	}
#endif
#if HAVE_SQLITE3
	ret = sqlite3_create_function((sqlite3 *) h->sqlite,
				      namestr.result,
				      (int) nargs,
				      SQLITE_UTF8, f,
				      isagg ? NULL : call3_func,
				      isagg ? call3_step : NULL,
				      isagg ? call3_final : NULL);
#endif
#endif
	transfree(&namestr);
	if (ret != SQLITE_OK) {
	    throwex(env, "error creating function/aggregate");
	}
	return;
    }
    throwclosed(env);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1create_1aggregate(JNIEnv *env, jobject obj,
					 jstring name, jint nargs, jobject fi)
{
    mkfunc_common(env, 1, obj, name, nargs, fi);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1create_1function(JNIEnv *env, jobject obj,
					jstring name, jint nargs, jobject fi)
{
    mkfunc_common(env, 0, obj, name, nargs, fi);
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1function_1type(JNIEnv *env, jobject obj,
				      jstring name, jint type)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    return;
	}
#endif
#if HAVE_SQLITE2
#if HAVE_SQLITE_FUNCTION_TYPE
	{
	    int ret;
	    transstr namestr;
	    jthrowable exc;

	    trans2iso(env, h->haveutf, h->enc, name, &namestr);
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		return;
	    }
	    ret = sqlite_function_type(h->sqlite, namestr.result, (int) type);
	    transfree(&namestr);
	    if (ret != SQLITE_OK) {
		throwex(env, sqlite_error_string(ret));
	    }
	}
#endif
#endif
	return;
    }
    throwclosed(env);
}

JNIEXPORT jint JNICALL
Java_SQLite_FunctionContext_count(JNIEnv *env, jobject obj)
{
    hfunc *f = getfunc(env, obj);
    jint r = 0;

    if (f && f->sf) {
#if HAVE_SQLITE_BOTH
	if (f->is3) {
	    r = (jint) sqlite3_aggregate_count((sqlite3_context *) f->sf);
	} else {
	    r = (jint) sqlite_aggregate_count((sqlite_func *) f->sf);
	}
#else
#if HAVE_SQLITE2
	r = (jint) sqlite_aggregate_count((sqlite_func *) f->sf);
#endif
#if HAVE_SQLITE3
	r = (jint) sqlite3_aggregate_count((sqlite3_context *) f->sf);
#endif
#endif
    }
    return r;
}

JNIEXPORT void JNICALL
Java_SQLite_FunctionContext_set_1error(JNIEnv *env, jobject obj, jstring err)
{
    hfunc *f = getfunc(env, obj);

    if (f && f->sf) {
#if HAVE_BOTH_SQLITE
	if (!f->is3) {
	    transstr errstr;
	    jthrowable exc;

	    trans2iso(env, f->h->haveutf, f->h->enc, err, &errstr);
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		return;
	    }
	    sqlite_set_result_error((sqlite_func *) f->sf,
				    errstr.result, -1);
	    transfree(&errstr);
	} else if (err) {
	    jsize len = (*env)->GetStringLength(env, err) * sizeof (jchar);
	    const jchar *str = (*env)->GetStringChars(env, err, 0);

	    sqlite3_result_error16((sqlite3_context *) f->sf, str, len);
	    (*env)->ReleaseStringChars(env, err, str);
	} else {
	    sqlite3_result_error((sqlite3_context *) f->sf,
				 "null error text", -1);
	}
#else
#if HAVE_SQLITE2
	transstr errstr;
	jthrowable exc;

	trans2iso(env, f->h->haveutf, f->h->enc, err, &errstr);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    (*env)->DeleteLocalRef(env, exc);
	    return;
	}
	sqlite_set_result_error((sqlite_func *) f->sf, errstr.result, -1);
	transfree(&errstr);
#endif
#if HAVE_SQLITE3
	if (err) {
	    jsize len = (*env)->GetStringLength(env, err) * sizeof (jchar);
	    const jchar *str = (*env)->GetStringChars(env, err, 0);

	    sqlite3_result_error16((sqlite3_context *) f->sf, str, len);
	    (*env)->ReleaseStringChars(env, err, str);
	} else {
	    sqlite3_result_error((sqlite3_context *) f->sf,
				 "null error text", -1);
	}
#endif
#endif
    }
}

JNIEXPORT void JNICALL
Java_SQLite_FunctionContext_set_1result__D(JNIEnv *env, jobject obj, jdouble d)
{
    hfunc *f = getfunc(env, obj);

    if (f && f->sf) {
#if HAVE_BOTH_SQLITE
	if (f->is3) {
	    sqlite3_result_double((sqlite3_context *) f->sf, (double) d);
	} else {
	    sqlite_set_result_double((sqlite_func *) f->sf, (double) d);
	}
#else
#if HAVE_SQLITE2
	sqlite_set_result_double((sqlite_func *) f->sf, (double) d);
#endif
#if HAVE_SQLITE3
	sqlite3_result_double((sqlite3_context *) f->sf, (double) d);
#endif
#endif
    }
}

JNIEXPORT void JNICALL
Java_SQLite_FunctionContext_set_1result__I(JNIEnv *env, jobject obj, jint i)
{
    hfunc *f = getfunc(env, obj);

    if (f && f->sf) {
#if HAVE_BOTH_SQLITE
	if (f->is3) {
	    sqlite3_result_int((sqlite3_context *) f->sf, (int) i);
	} else {
	    sqlite_set_result_int((sqlite_func *) f->sf, (int) i);
	}
#else
#if HAVE_SQLITE2
	sqlite_set_result_int((sqlite_func *) f->sf, (int) i);
#endif
#if HAVE_SQLITE3
	sqlite3_result_int((sqlite3_context *) f->sf, (int) i);
#endif
#endif
    }
}

JNIEXPORT void JNICALL
Java_SQLite_FunctionContext_set_1result__Ljava_lang_String_2(JNIEnv *env,
							     jobject obj,
							     jstring ret)
{
    hfunc *f = getfunc(env, obj);

    if (f && f->sf) {
#if HAVE_BOTH_SQLITE
	if (!f->is3) {
	    transstr retstr;
	    jthrowable exc;

	    trans2iso(env, f->h->haveutf, f->h->enc, ret, &retstr);
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		return;
	    }
	    sqlite_set_result_string((sqlite_func *) f->sf,
				     retstr.result, -1);
	    transfree(&retstr);
	} else if (ret) {
	    jsize len = (*env)->GetStringLength(env, ret) * sizeof (jchar);
	    const jchar *str = (*env)->GetStringChars(env, ret, 0);

	    sqlite3_result_text16((sqlite3_context *) f->sf, str, len,
				  SQLITE_TRANSIENT);
	    (*env)->ReleaseStringChars(env, ret, str);
	} else {
	    sqlite3_result_null((sqlite3_context *) f->sf);
	}
#else
#if HAVE_SQLITE2
	transstr retstr;
	jthrowable exc;

	trans2iso(env, f->h->haveutf, f->h->enc, ret, &retstr);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    (*env)->DeleteLocalRef(env, exc);
	    return;
	}
	sqlite_set_result_string((sqlite_func *) f->sf, retstr.result, -1);
	transfree(&retstr);
#endif
#if HAVE_SQLITE3
	if (ret) {
	    jsize len = (*env)->GetStringLength(env, ret) * sizeof (jchar);
	    const jchar *str = (*env)->GetStringChars(env, ret, 0);

	    sqlite3_result_text16((sqlite3_context *) f->sf, str, len,
				  SQLITE_TRANSIENT);
	    (*env)->ReleaseStringChars(env, ret, str);
	} else {
	    sqlite3_result_null((sqlite3_context *) f->sf);
	}
#endif
#endif
    }
}

JNIEXPORT void JNICALL
Java_SQLite_FunctionContext_set_1result___3B(JNIEnv *env, jobject obj,
					     jbyteArray b)
{
#if HAVE_SQLITE3
    hfunc *f = getfunc(env, obj);

    if (f && f->sf) {
#if HAVE_BOTH_SQLITE
	if (!f->is3) {
	    /* silently ignored */
	    return;
	}
#endif
	if (b) {
	    jsize len;
	    jbyte *data;

	    len = (*env)->GetArrayLength(env, b);
	    data = (*env)->GetByteArrayElements(env, b, 0);
	    sqlite3_result_blob((sqlite3_context *) f->sf,
				data, len, SQLITE_TRANSIENT);
	    (*env)->ReleaseByteArrayElements(env, b, data, 0);
	} else {
	    sqlite3_result_null((sqlite3_context *) f->sf);
	}
    }
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_FunctionContext_set_1result_1zeroblob(JNIEnv *env, jobject obj,
						  jint n)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_RESULT_ZEROBLOB
    hfunc *f = getfunc(env, obj);

    if (f && f->sf) {
#if HAVE_BOTH_SQLITE
	if (!f->is3) {
	    /* silently ignored */
	    return;
	}
#endif
	sqlite3_result_zeroblob((sqlite3_context *) f->sf, n);
    }
#endif
}

JNIEXPORT jstring JNICALL
Java_SQLite_Database_error_1string(JNIEnv *env, jclass c, jint err)
{
#if HAVE_SQLITE2
    return (*env)->NewStringUTF(env, sqlite_error_string((int) err));
#else
    return (*env)->NewStringUTF(env, "unkown error");
#endif
}

JNIEXPORT jstring JNICALL
Java_SQLite_Database__1errmsg(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (!h->is3) {
	    return 0;
	}
#endif
	return (*env)->NewStringUTF(env,
				    sqlite3_errmsg((sqlite3 *) h->sqlite));
    }
#endif
    return 0;
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1set_1encoding(JNIEnv *env, jobject obj, jstring enc)
{
    handle *h = gethandle(env, obj);

    if (h && !h->haveutf) {
#if HAVE_BOTH_SQLITE
	if (!h->is3) {
	    delglobrefp(env, &h->enc);
	    h->enc = enc;
	    globrefset(env, enc, &h->enc);
	}
#else
#if HAVE_SQLITE2
	delglobrefp(env, &h->enc);
	h->enc = enc;
	globrefset(env, enc, &h->enc);
#endif
#endif
    }
}

#if HAVE_SQLITE_SET_AUTHORIZER
static int
doauth(void *arg, int what, const char *arg1, const char *arg2,
       const char *arg3, const char *arg4)
{
    handle *h = (handle *) arg;
    JNIEnv *env = h->env;

    if (env && h->ai) {
	jthrowable exc;
	jclass cls = (*env)->GetObjectClass(env, h->ai);
	jmethodID mid;
	jint i = what;

	mid = (*env)->GetMethodID(env, cls, "authorize",
				  "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
	if (mid) {
	    jstring s1 = 0, s2 = 0, s3 = 0, s4 = 0;
	    transstr tr;

	    if (arg1) {
		trans2utf(env, h->haveutf, h->enc, arg1, &tr);
		s1 = tr.jstr;
	    }
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		return SQLITE_DENY;
	    }
	    if (arg2) {
		trans2utf(env, h->haveutf, h->enc, arg2, &tr);
		s2 = tr.jstr;
	    }
	    if (arg3) {
		trans2utf(env, h->haveutf, h->enc, arg3, &tr);
		s3 = tr.jstr;
	    }
	    if (arg4) {
		trans2utf(env, h->haveutf, h->enc, arg4, &tr);
		s4 = tr.jstr;
	    }
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		return SQLITE_DENY;
	    }
	    i = (*env)->CallIntMethod(env, h->ai, mid, i, s1, s2, s3, s4);
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		return SQLITE_DENY;
	    }
	    (*env)->DeleteLocalRef(env, s4);
	    (*env)->DeleteLocalRef(env, s3);
	    (*env)->DeleteLocalRef(env, s2);
	    (*env)->DeleteLocalRef(env, s1);
	    if (i != SQLITE_OK && i != SQLITE_IGNORE) {
		i = SQLITE_DENY;
	    }
	    return (int) i;
	}
    }
    return SQLITE_DENY;
}
#endif

JNIEXPORT void JNICALL
Java_SQLite_Database__1set_1authorizer(JNIEnv *env, jobject obj, jobject auth)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
	delglobrefp(env, &h->ai);
	globrefset(env, auth, &h->ai);
#if HAVE_SQLITE_SET_AUTHORIZER
	h->env = env;
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    sqlite3_set_authorizer((sqlite3 *) h->sqlite,
				   h->ai ? doauth : 0, h);
	} else {
	    sqlite_set_authorizer((sqlite *) h->sqlite,
				  h->ai ? doauth : 0, h);
	}
#else
#if HAVE_SQLITE2
	sqlite_set_authorizer((sqlite *) h->sqlite, h->ai ? doauth : 0, h);
#endif
#if HAVE_SQLITE3
	sqlite3_set_authorizer((sqlite3 *) h->sqlite, h->ai ? doauth : 0, h);
#endif
#endif
#endif
	return;
    }
    throwclosed(env);
}

#if HAVE_SQLITE_TRACE
static void
dotrace(void *arg, const char *msg)
{
    handle *h = (handle *) arg;
    JNIEnv *env = h->env;

    if (env && h->tr && msg) {
	jthrowable exc;
	jclass cls = (*env)->GetObjectClass(env, h->tr);
	jmethodID mid;

	mid = (*env)->GetMethodID(env, cls, "trace", "(Ljava/lang/String;)V");
	if (mid) {
	    transstr tr;

	    trans2utf(env, h->haveutf, h->enc, msg, &tr);
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		(*env)->ExceptionClear(env);
		return;
	    }
	    (*env)->CallVoidMethod(env, h->tr, mid, tr.jstr);
	    (*env)->ExceptionClear(env);
	    (*env)->DeleteLocalRef(env, tr.jstr);
	    return;
	}
    }
    return;
}
#endif

JNIEXPORT void JNICALL
Java_SQLite_Database__1trace(JNIEnv *env, jobject obj, jobject tr)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
	delglobrefp(env, &h->tr);
	globrefset(env, tr, &h->tr);
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    sqlite3_trace((sqlite3 *) h->sqlite, h->tr ? dotrace : 0, h);
	} else {
#if HAVE_SQLITE_TRACE
	    sqlite_trace((sqlite *) h->sqlite, h->tr ? dotrace : 0, h);
#endif
	}
#else
#if HAVE_SQLITE2
#if HAVE_SQLITE_TRACE
	sqlite_trace((sqlite *) h->sqlite, h->tr ? dotrace : 0, h);
#endif
#endif
#if HAVE_SQLITE3
	sqlite3_trace((sqlite3 *) h->sqlite, h->tr ? dotrace : 0, h);
#endif
#endif
	return;
    }
    throwclosed(env);
}

#if HAVE_SQLITE_COMPILE
static void
dovmfinal(JNIEnv *env, jobject obj, int final)
{
    hvm *v = gethvm(env, obj);

    if (v) {
	if (v->h) {
	    handle *h = v->h;
	    hvm *vv, **vvp;

	    vvp = &h->vms;
	    vv = *vvp;
	    while (vv) {
		if (vv == v) {
		    *vvp = vv->next;
		    break;
		}
		vvp = &vv->next;
		vv = *vvp;
	    }
	}
	if (v->vm) {
#if HAVE_BOTH_SQLITE
	    if (v->is3) {
		sqlite3_finalize((sqlite3_stmt *) v->vm);
	    } else {
		sqlite_finalize((sqlite_vm *) v->vm, 0);
	    }
#else
#if HAVE_SQLITE2
	    sqlite_finalize((sqlite_vm *) v->vm, 0);
#endif
#if HAVE_SQLITE3
	    sqlite3_finalize((sqlite3_stmt *) v->vm);
#endif
#endif
	    v->vm = 0;
	}
	free(v);
	(*env)->SetLongField(env, obj, F_SQLite_Vm_handle, 0);
	return;
    }
    if (!final) {
	throwex(env, "vm already closed");
    }
}
#endif

#if HAVE_SQLITE3
static void
dostmtfinal(JNIEnv *env, jobject obj)
{
    hvm *v = gethstmt(env, obj);

    if (v) {
	if (v->h) {
	    handle *h = v->h;
	    hvm *vv, **vvp;

	    vvp = &h->vms;
	    vv = *vvp;
	    while (vv) {
		if (vv == v) {
		    *vvp = vv->next;
		    break;
		}
		vvp = &vv->next;
		vv = *vvp;
	    }
	}
	if (v->vm) {
	    sqlite3_finalize((sqlite3_stmt *) v->vm);
	}
	v->vm = 0;
	free(v);
	(*env)->SetLongField(env, obj, F_SQLite_Stmt_handle, 0);
    }
}
#endif

#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
static void
doblobfinal(JNIEnv *env, jobject obj)
{
    hbl *bl = gethbl(env, obj);

    if (bl) {
	if (bl->h) {
	    handle *h = bl->h;
	    hbl *blc, **blp;

	    blp = &h->blobs;
	    blc = *blp;
	    while (blc) {
		if (blc == bl) {
		    *blp = blc->next;
		    break;
		}
		blp = &blc->next;
		blc = *blp;
	    }
	}
	if (bl->blob) {
	    sqlite3_blob_close(bl->blob);
	}
	bl->blob = 0;
	free(bl);
	(*env)->SetLongField(env, obj, F_SQLite_Blob_handle, 0);
	(*env)->SetIntField(env, obj, F_SQLite_Blob_size, 0);
    }
}
#endif

JNIEXPORT void JNICALL
Java_SQLite_Vm_stop(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE_COMPILE
    dovmfinal(env, obj, 0);
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Vm_finalize(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE_COMPILE
    dovmfinal(env, obj, 1);
#endif
}

#if HAVE_SQLITE_COMPILE
#if HAVE_SQLITE3
static void
free_tab(void *mem)
{
    char **p = (char **) mem;
    int i, n;

    if (!p) {
	return;
    }
    p -= 1;
    mem = (void *) p;
    n = ((int *) p)[0];
    p += n * 2 + 2 + 1;
    for (i = 0; i < n; i++) {
	if (p[i]) {
	    free(p[i]);
	}
    }
    free(mem);
}
#endif
#endif

JNIEXPORT jboolean JNICALL
Java_SQLite_Vm_step(JNIEnv *env, jobject obj, jobject cb)
{
#if HAVE_SQLITE_COMPILE
    hvm *v = gethvm(env, obj);

    if (v && v->vm && v->h) {
	jthrowable exc;
	int ret;
#if HAVE_SQLITE2
	int tmp;
#endif
	long ncol = 0;
#if HAVE_SQLITE3
	freemem *freeproc = 0;
	const char **blob = 0;
#endif
	const char **data = 0, **cols = 0;

	v->h->env = env;
#if HAVE_BOTH_SQLITE
	if (v->is3) {
	    ret = sqlite3_step((sqlite3_stmt *) v->vm);
	    if (ret == SQLITE_DONE && v->hh.row1) {
		ncol = sqlite3_column_count((sqlite3_stmt *) v->vm);
		if (ncol > 0) {
		    data = calloc(ncol * 3 + 3 + 1, sizeof (char *));
		    if (data) {
			data[0] = (const char *) ncol;
			++data;
			cols = data + ncol + 1;
			blob = cols + ncol + 1;
			freeproc = free_tab;
		    } else {
			ret = SQLITE_NOMEM;
		    }
		}
		if (ret != SQLITE_NOMEM) {
		    int i;

		    for (i = 0; i < ncol; i++) {
			cols[i] =
			    sqlite3_column_name((sqlite3_stmt *) v->vm, i);
		    }
		}
	    } else if (ret == SQLITE_ROW) {
		ncol = sqlite3_data_count((sqlite3_stmt *) v->vm);
		if (ncol > 0) {
		    data = calloc(ncol * 3 + 3 + 1, sizeof (char *));
		    if (data) {
			data[0] = (const char *) ncol;
			++data;
			cols = data + ncol + 1;
			blob = cols + ncol + 1;
			freeproc = free_tab;
		    } else {
			ret = SQLITE_NOMEM;
		    }
		}
		if (ret != SQLITE_NOMEM) {
		    int i;

		    for (i = 0; i < ncol; i++) {
			cols[i] =
			    sqlite3_column_name((sqlite3_stmt *) v->vm, i);
			if (sqlite3_column_type((sqlite3_stmt *) v->vm, i)
			    == SQLITE_BLOB) {
			    unsigned char *src = (unsigned char *)
				sqlite3_column_blob((sqlite3_stmt *) v->vm, i);
			    int n =
				sqlite3_column_bytes((sqlite3_stmt *) v->vm,
						     i);

			    if (src) {
				data[i] = malloc(n * 2 + 4);
				if (data[i]) {
				    int k;
				    char *p = (char *) data[i];

				    blob[i] = data[i];
				    *p++ = 'X';
				    *p++ = '\'';
				    for (k = 0; k < n; k++) {
					*p++ = xdigits[src[k] >> 4];
					*p++ = xdigits[src[k] & 0x0F];
				    }
				    *p++ = '\'';
				    *p++ = '\0';
				}
			    }
			} else {
			    data[i] = (const char *)
				sqlite3_column_text((sqlite3_stmt *) v->vm, i);
			}
		    }
		}
	    }
	} else {
	    tmp = 0;
	    ret = sqlite_step((sqlite_vm *) v->vm, &tmp, &data, &cols);
	    ncol = tmp;
	}
#else
#if HAVE_SQLITE2
	tmp = 0;
	ret = sqlite_step((sqlite_vm *) v->vm, &tmp, &data, &cols);
	ncol = tmp;
#endif
#if HAVE_SQLITE3
	ret = sqlite3_step((sqlite3_stmt *) v->vm);
	if (ret == SQLITE_DONE && v->hh.row1) {
	    ncol = sqlite3_column_count((sqlite3_stmt *) v->vm);
	    if (ncol > 0) {
		data = calloc(ncol * 3 + 3 + 1, sizeof (char *));
		if (data) {
		    data[0] = (const char *) ncol;
		    ++data;
		    cols = data + ncol + 1;
		    blob = cols + ncol + 1;
		    freeproc = free_tab;
		} else {
		    ret = SQLITE_NOMEM;
		}
	    }
	    if (ret != SQLITE_NOMEM) {
		int i;

		for (i = 0; i < ncol; i++) {
		    cols[i] =
			sqlite3_column_name((sqlite3_stmt *) v->vm, i);
		}
	    }
	} else if (ret == SQLITE_ROW) {
	    ncol = sqlite3_data_count((sqlite3_stmt *) v->vm);
	    if (ncol > 0) {
		data = calloc(ncol * 3 + 3 + 1, sizeof (char *));
		if (data) {
		    data[0] = (const char *) ncol;
		    ++data;
		    cols = data + ncol + 1;
		    blob = cols + ncol + 1;
		    freeproc = free_tab;
		} else {
		    ret = SQLITE_NOMEM;
		}
	    }
	    if (ret != SQLITE_NOMEM) {
		int i;

		for (i = 0; i < ncol; i++) {
		    cols[i] = sqlite3_column_name((sqlite3_stmt *) v->vm, i);
		    if (sqlite3_column_type((sqlite3_stmt *) v->vm, i)
			== SQLITE_BLOB) {
			unsigned char *src = (unsigned char *)
			    sqlite3_column_blob((sqlite3_stmt *) v->vm, i);
			int n =
			    sqlite3_column_bytes((sqlite3_stmt *) v->vm, i);

			if (src) {
			    data[i] = malloc(n * 2 + 4);
			    if (data[i]) {
				int k;
				char *p = (char *) data[i];

				blob[i] = data[i];
				*p++ = 'X';
				*p++ = '\'';
				for (k = 0; k < n; k++) {
				    *p++ = xdigits[src[k] >> 4];
				    *p++ = xdigits[src[k] & 0x0F];
				}
				*p++ = '\'';
				*p++ = '\0';
			    }
			}
		    } else {
			data[i] = (char *)
			    sqlite3_column_text((sqlite3_stmt *) v->vm, i);
		    }
		}
	    }
	}
#endif
#endif
	if (ret == SQLITE_ROW) {
	    v->hh.cb = cb;
	    v->hh.env = env;
#if HAVE_BOTH_SQLITE
	    if (v->is3) {
		v->hh.stmt = (sqlite3_stmt *) v->vm;
	    }
#else
#if HAVE_SQLITE3
	    v->hh.stmt = (sqlite3_stmt *) v->vm;
#endif
#endif
	    callback((void *) &v->hh, ncol, (char **) data, (char **) cols);
#if HAVE_SQLITE3
	    if (data && freeproc) {
		freeproc((void *) data);
	    }
#endif
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		goto dofin;
	    }
	    return JNI_TRUE;
	} else if (ret == SQLITE_DONE) {
dofin:
	    if (v->hh.row1 && cols) {
		v->hh.cb = cb;
		v->hh.env = env;
#if HAVE_BOTH_SQLITE
		if (v->is3) {
		    v->hh.stmt = (sqlite3_stmt *) v->vm;
		}
#else
#if HAVE_SQLITE3
		v->hh.stmt = (sqlite3_stmt *) v->vm;
#endif
#endif
		callback((void *) &v->hh, ncol, (char **) 0, (char **) cols);
#if HAVE_SQLITE3
		if (data && freeproc) {
		    freeproc((void *) data);
		}
#endif
		exc = (*env)->ExceptionOccurred(env);
		if (exc) {
		    (*env)->DeleteLocalRef(env, exc);
		}
	    }
#if HAVE_BOTH_SQLITE
	    if (v->is3) {
		sqlite3_finalize((sqlite3_stmt *) v->vm);
	    } else {
		sqlite_finalize((sqlite_vm *) v->vm, 0);
	    }
#else
#if HAVE_SQLITE2
	    sqlite_finalize((sqlite_vm *) v->vm, 0);
#endif
#if HAVE_SQLITE3
	    sqlite3_finalize((sqlite3_stmt *) v->vm);
#endif
#endif
	    v->vm = 0;
	    return JNI_FALSE;
	}
#if HAVE_BOTH_SQLITE
	if (v->is3) {
	    sqlite3_finalize((sqlite3_stmt *) v->vm);
	} else {
	    sqlite_finalize((sqlite_vm *) v->vm, 0);
	}
#else
#if HAVE_SQLITE2
	sqlite_finalize((sqlite_vm *) v->vm, 0);
#endif
#if HAVE_SQLITE3
	sqlite3_finalize((sqlite3_stmt *) v->vm);
#endif
#endif
	setvmerr(env, obj, ret);
	v->vm = 0;
	throwex(env, "error in step");
	return JNI_FALSE;
    }
    throwex(env, "vm already closed");
#else
    throwex(env, "unsupported");
#endif
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_SQLite_Vm_compile(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE_COMPILE
    hvm *v = gethvm(env, obj);
    void *svm = 0;
    char *err = 0;
#ifdef HAVE_SQLITE2
    char *errfr = 0;
#endif
    const char *tail;
    int ret;

    if (v && v->vm) {
#if HAVE_BOTH_SQLITE
	if (v->is3) {
	    sqlite3_finalize((sqlite3_stmt *) v->vm);
	} else {
	    sqlite_finalize((sqlite_vm *) v->vm, 0);
	}
#else
#if HAVE_SQLITE2
	sqlite_finalize((sqlite_vm *) v->vm, 0);
#endif
#if HAVE_SQLITE3
	sqlite3_finalize((sqlite3_stmt *) v->vm);
#endif
#endif
	v->vm = 0;
    }
    if (v && v->h && v->h->sqlite) {
	if (!v->tail) {
	    return JNI_FALSE;
	}
	v->h->env = env;
#if HAVE_BOTH_SQLITE
	if (v->is3) {
#if HAVE_SQLITE3_PREPARE_V2
	    ret = sqlite3_prepare_v2((sqlite3 *) v->h->sqlite, v->tail, -1,
				     (sqlite3_stmt **) &svm, &tail);
#else
	    ret = sqlite3_prepare((sqlite3 *) v->h->sqlite, v->tail, -1,
				  (sqlite3_stmt **) &svm, &tail);
#endif
	    if (ret != SQLITE_OK) {
		if (svm) {
		    sqlite3_finalize((sqlite3_stmt *) svm);
		    svm = 0;
		}
		err = (char *) sqlite3_errmsg((sqlite3 *) v->h->sqlite);
	    }
	} else {
	    ret = sqlite_compile((sqlite *) v->h->sqlite, v->tail,
				 &tail, (sqlite_vm **) &svm, &errfr);
	    if (ret != SQLITE_OK) {
		err = errfr;
		if (svm) {
		    sqlite_finalize((sqlite_vm *) svm, 0);
		    svm = 0;
		}
	    }
	}
#else
#if HAVE_SQLITE2
	ret = sqlite_compile((sqlite *) v->h->sqlite, v->tail,
			     &tail, (sqlite_vm **) &svm, &errfr);
	if (ret != SQLITE_OK) {
	    err = errfr;
	    if (svm) {
		sqlite_finalize((sqlite_vm *) svm, 0);
		svm = 0;
	    }
	}
#endif
#if HAVE_SQLITE3
#if HAVE_SQLITE3_PREPARE_V2
	ret = sqlite3_prepare_v2((sqlite3 *) v->h->sqlite,
				 v->tail, -1, (sqlite3_stmt **) &svm, &tail);
#else
	ret = sqlite3_prepare((sqlite3 *) v->h->sqlite,
			      v->tail, -1, (sqlite3_stmt **) &svm, &tail);
#endif
	if (ret != SQLITE_OK) {
	    if (svm) {
		sqlite3_finalize((sqlite3_stmt *) svm);
		svm = 0;
	    }
	    err = (char *) sqlite3_errmsg((sqlite3 *) v->h->sqlite);
	}
#endif
#endif
	if (ret != SQLITE_OK) {
	    setvmerr(env, obj, ret);
	    v->tail = 0;
	    throwex(env, err ? err : "error in compile/prepare");
#if HAVE_SQLITE2
	    if (errfr) {
		sqlite_freemem(errfr);
	    }
#endif
	    return JNI_FALSE;
	}
#if HAVE_SQLITE2
	if (errfr) {
	    sqlite_freemem(errfr);
	}
#endif
	if (!svm) {
	    v->tail = 0;
	    return JNI_FALSE;
	}
	v->vm = svm;
	v->tail = (char *) tail;
	v->hh.row1 = 1;
	return JNI_TRUE;
    }
    throwex(env, "vm already closed");
#else
    throwex(env, "unsupported");
#endif
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_SQLite_Database_vm_1compile(JNIEnv *env, jobject obj, jstring sql,
				 jobject vm)
{
#if HAVE_SQLITE_COMPILE
    handle *h = gethandle(env, obj);
    void *svm = 0;
    hvm *v;
    char *err = 0;
#if HAVE_SQLITE2
    char *errfr = 0;
#endif
    const char *tail;
    transstr tr;
    jvalue vv;
    int ret;
    jthrowable exc;

    if (!h) {
	throwclosed(env);
	return;
    }
    if (!vm) {
	throwex(env, "null vm");
	return;
    }
    if (!sql) {
	throwex(env, "null sql");
	return;
    }
    trans2iso(env, h->haveutf, h->enc, sql, &tr);
    exc = (*env)->ExceptionOccurred(env);
    if (exc) {
	(*env)->DeleteLocalRef(env, exc);
	return;
    }
    h->env = env;
#if HAVE_BOTH_SQLITE
    if (h->is3) {
#if HAVE_SQLITE3_PREPARE_V2
	ret = sqlite3_prepare_v2((sqlite3 *) h->sqlite, tr.result, -1,
				 (sqlite3_stmt **) &svm, &tail);
#else
	ret = sqlite3_prepare((sqlite3 *) h->sqlite, tr.result, -1,
			      (sqlite3_stmt **) &svm, &tail);
#endif
	if (ret != SQLITE_OK) {
	    if (svm) {
		sqlite3_finalize((sqlite3_stmt *) svm);
		svm = 0;
	    }
	    err = (char *) sqlite3_errmsg((sqlite3 *) h->sqlite);
	}
    } else {
	ret = sqlite_compile((sqlite *) h->sqlite, tr.result, &tail,
			     (sqlite_vm **) &svm, &errfr);
	if (ret != SQLITE_OK) {
	    err = errfr;
	    if (svm) {
		sqlite_finalize((sqlite_vm *) svm, 0);
	    }
	}
    }
#else
#if HAVE_SQLITE2
    ret = sqlite_compile((sqlite *) h->sqlite, tr.result, &tail,
			 (sqlite_vm **) &svm, &errfr);
    if (ret != SQLITE_OK) {
	err = errfr;
	if (svm) {
	    sqlite_finalize((sqlite_vm *) svm, 0);
	    svm = 0;
	}
    }
#endif
#if HAVE_SQLITE3
#if HAVE_SQLITE3_PREPARE_V2
    ret = sqlite3_prepare_v2((sqlite3 *) h->sqlite, tr.result, -1,
			     (sqlite3_stmt **) &svm, &tail);
#else
    ret = sqlite3_prepare((sqlite3 *) h->sqlite, tr.result, -1,
			  (sqlite3_stmt **) &svm, &tail);
#endif
    if (ret != SQLITE_OK) {
	if (svm) {
	    sqlite3_finalize((sqlite3_stmt *) svm);
	    svm = 0;
	}
	err = (char *) sqlite3_errmsg((sqlite3 *) h->sqlite);
    }
#endif
#endif
    if (ret != SQLITE_OK) {
	transfree(&tr);
	setvmerr(env, vm, ret);
	throwex(env, err ? err : "error in prepare/compile");
#if HAVE_SQLITE2
	if (errfr) {
	    sqlite_freemem(errfr);
	}
#endif
	return;
    }
#if HAVE_SQLITE2
    if (errfr) {
	sqlite_freemem(errfr);
    }
#endif
    if (!svm) {
	transfree(&tr);
	return;
    }
    v = malloc(sizeof (hvm) + strlen(tail) + 1);
    if (!v) {
	transfree(&tr);
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    sqlite3_finalize((sqlite3_stmt *) svm);
	} else {
	    sqlite_finalize((sqlite_vm *) svm, 0);
	}
#else
#if HAVE_SQLITE2
	sqlite_finalize((sqlite_vm *) svm, 0);
#endif
#if HAVE_SQLITE3
	sqlite3_finalize((sqlite3_stmt *) svm);
#endif
#endif
	throwoom(env, "unable to get SQLite handle");
	return;
    }
    v->next = h->vms;
    h->vms = v;
    v->vm = svm;
    v->h = h;
    v->tail = (char *) (v + 1);
#if HAVE_BOTH_SQLITE
    v->is3 = v->hh.is3 = h->is3;
#endif
    strcpy(v->tail, tail);
    v->hh.sqlite = 0;
    v->hh.haveutf = h->haveutf;
    v->hh.ver = h->ver;
    v->hh.bh = v->hh.cb = v->hh.ai = v->hh.tr = v->hh.ph = 0;
    v->hh.row1 = 1;
    v->hh.enc = h->enc;
    v->hh.funcs = 0;
    v->hh.vms = 0;
    v->hh.env = 0;
    vv.j = 0;
    vv.l = (jobject) v;
    (*env)->SetLongField(env, vm, F_SQLite_Vm_handle, vv.j);
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Database_vm_1compile_1args(JNIEnv *env,
				       jobject obj, jstring sql,
				       jobject vm, jobjectArray args)
{
#if HAVE_SQLITE_COMPILE
#if HAVE_SQLITE3
    handle *h = gethandle(env, obj);
#endif

#if HAVE_BOTH_SQLITE
    if (h && !h->is3) {
	throwex(env, "unsupported");
	return;
    }
#else
#if HAVE_SQLITE2
    throwex(env, "unsupported");
#endif
#endif
#if HAVE_SQLITE3 
    if (!h || !h->sqlite) {
	throwclosed(env);
	return;
    }
    if (!vm) {
	throwex(env, "null vm");
	return;
    }
    if (!sql) {
	throwex(env, "null sql");
	return;
    } else {
	void *svm = 0;
	hvm *v;
	jvalue vv;
	jthrowable exc;
	int rc = SQLITE_ERROR, nargs, i;
	char *p;
	const char *str = (*env)->GetStringUTFChars(env, sql, 0);
	const char *tail;
	transstr sqlstr;
	struct args {
	    char *arg;
	    jobject obj;
	    transstr trans;
	} *argv = 0;
	char **cargv = 0;

	p = (char *) str;
	nargs = 0;
	while (*p) {
	    if (*p == '%') {
		++p;
		if (*p == 'q' || *p == 'Q' || *p == 's') {
		    nargs++;
		    if (nargs > MAX_PARAMS) {
			(*env)->ReleaseStringUTFChars(env, sql, str);
			throwex(env, "too much SQL parameters");
			return;
		    }
		} else if (*p != '%') {
		    (*env)->ReleaseStringUTFChars(env, sql, str);
		    throwex(env, "bad % specification in query");
		    return;
		}
	    }
	    ++p;
	}
	cargv = malloc((sizeof (*argv) + sizeof (char *)) * MAX_PARAMS);
	if (!cargv) {
	    (*env)->ReleaseStringUTFChars(env, sql, str);
	    throwoom(env, "unable to allocate arg vector");
	    return;
	}
	argv = (struct args *) (cargv + MAX_PARAMS);
	for (i = 0; i < MAX_PARAMS; i++) {
	    cargv[i] = 0;
	    argv[i].arg = 0;
	    argv[i].obj = 0;
	    argv[i].trans.result = argv[i].trans.tofree = 0;
	}
	exc = 0;
	for (i = 0; i < nargs; i++) {
	    jobject so = (*env)->GetObjectArrayElement(env, args, i);

	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		break;
	    }
	    if (so) {
		argv[i].obj = so;
		argv[i].arg = cargv[i] =
		    trans2iso(env, 1, 0, argv[i].obj, &argv[i].trans);
	    }
	}
	if (exc) {
	    for (i = 0; i < nargs; i++) {
		if (argv[i].obj) {
		    transfree(&argv[i].trans);
		}
	    }
	    freep((char **) &cargv);
	    (*env)->ReleaseStringUTFChars(env, sql, str);
	    return;
	}
	h->row1 = 1;
	trans2iso(env, 1, 0, sql, &sqlstr);
	exc = (*env)->ExceptionOccurred(env);
	if (!exc) {
#if defined(_WIN32) || !defined(CANT_PASS_VALIST_AS_CHARPTR)
	    char *s = sqlite3_vmprintf(sqlstr.result, (char *) cargv);
#else
	    char *s = sqlite3_mprintf(sqlstr.result,
				      cargv[0], cargv[1],
				      cargv[2], cargv[3],
				      cargv[4], cargv[5],
				      cargv[6], cargv[7],
				      cargv[8], cargv[9],
				      cargv[10], cargv[11],
				      cargv[12], cargv[13],
				      cargv[14], cargv[15],
				      cargv[16], cargv[17],
				      cargv[18], cargv[19],
				      cargv[20], cargv[21],
				      cargv[22], cargv[23],
				      cargv[24], cargv[25],
				      cargv[26], cargv[27],
				      cargv[28], cargv[29],
				      cargv[30], cargv[31]);
#endif
	    if (!s) {
		rc = SQLITE_NOMEM;
	    } else {
#if HAVE_SQLITE3_PREPARE_V2
		rc = sqlite3_prepare_v2((sqlite3 *) h->sqlite, s, -1,
					(sqlite3_stmt **) &svm, &tail);
#else
		rc = sqlite3_prepare((sqlite3 *) h->sqlite, s, -1,
				      (sqlite3_stmt **) &svm, &tail);
#endif
		if (rc != SQLITE_OK) {
		    if (svm) {
			sqlite3_finalize((sqlite3_stmt *) svm);
			svm = 0;
		    }
		}
	    }
	    if (rc != SQLITE_OK) {
		sqlite3_free(s);
		for (i = 0; i < nargs; i++) {
		    if (argv[i].obj) {
			transfree(&argv[i].trans);
		    }
		}
		freep((char **) &cargv);
		transfree(&sqlstr);
		(*env)->ReleaseStringUTFChars(env, sql, str);
		setvmerr(env, vm, rc);
		throwex(env, "error in prepare");
		return;
	    }
	    v = malloc(sizeof (hvm) + strlen(tail) + 1);
	    if (!v) {
		sqlite3_free(s);
		for (i = 0; i < nargs; i++) {
		    if (argv[i].obj) {
			transfree(&argv[i].trans);
		    }
		}
		freep((char **) &cargv);
		transfree(&sqlstr);
		(*env)->ReleaseStringUTFChars(env, sql, str);
		sqlite3_finalize((sqlite3_stmt *) svm);
		setvmerr(env, vm, SQLITE_NOMEM);
		throwoom(env, "unable to get SQLite handle");
		return;
	    }
	    v->next = h->vms;
	    h->vms = v;
	    v->vm = svm;
	    v->h = h;
	    v->tail = (char *) (v + 1);
#if HAVE_BOTH_SQLITE
	    v->is3 = v->hh.is3 = h->is3;
#endif
	    strcpy(v->tail, tail);
	    sqlite3_free(s);
	    v->hh.sqlite = 0;
	    v->hh.haveutf = h->haveutf;
	    v->hh.ver = h->ver;
	    v->hh.bh = v->hh.cb = v->hh.ai = v->hh.tr = v->hh.ph = 0;
	    v->hh.row1 = 1;
	    v->hh.enc = h->enc;
	    v->hh.funcs = 0;
	    v->hh.vms = 0;
	    v->hh.env = 0;
	    vv.j = 0;
	    vv.l = (jobject) v;
	    (*env)->SetLongField(env, vm, F_SQLite_Vm_handle, vv.j);
	}
	for (i = 0; i < nargs; i++) {
	    if (argv[i].obj) {
		transfree(&argv[i].trans);
	    }
	}
	freep((char **) &cargv);
	transfree(&sqlstr);
	(*env)->ReleaseStringUTFChars(env, sql, str);
	if (exc) {
	    (*env)->DeleteLocalRef(env, exc);
	}
    }
#endif
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_FunctionContext_internal_1init(JNIEnv *env, jclass cls)
{
    F_SQLite_FunctionContext_handle =
	(*env)->GetFieldID(env, cls, "handle", "J");
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1progress_1handler(JNIEnv *env, jobject obj, jint n,
					 jobject ph)
{
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
	/* CHECK THIS */
#if HAVE_SQLITE_PROGRESS_HANDLER
	delglobrefp(env, &h->ph);
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    if (ph) {
		globrefset(env, ph, &h->ph);
		sqlite3_progress_handler((sqlite3 *) h->sqlite,
					 n, progresshandler, h);
	    } else {
		sqlite3_progress_handler((sqlite3 *) h->sqlite,
					 0, 0, 0);
	    }
	} else {
	    if (ph) {
		globrefset(env, ph, &h->ph);
		sqlite_progress_handler((sqlite *) h->sqlite,
					n, progresshandler, h);
	    } else {
		sqlite_progress_handler((sqlite *) h->sqlite,
					0, 0, 0);
	    }
	}
#else
#if HAVE_SQLITE2
	if (ph) {
	    globrefset(env, ph, &h->ph);
	    sqlite_progress_handler((sqlite *) h->sqlite,
				    n, progresshandler, h);
	} else {
	    sqlite_progress_handler((sqlite *) h->sqlite,
				    0, 0, 0);
	}
#endif
#if HAVE_SQLITE3
	if (ph) {
	    globrefset(env, ph, &h->ph);
	    sqlite3_progress_handler((sqlite3 *) h->sqlite,
				     n, progresshandler, h);
	} else {
	    sqlite3_progress_handler((sqlite3 *) h->sqlite,
				     0, 0, 0);
	}
#endif
#endif
	return;
#else
	throwex(env, "unsupported");
	return;
#endif
    }
    throwclosed(env);
}

JNIEXPORT jboolean JNICALL
Java_SQLite_Database_is3(JNIEnv *env, jobject obj)
{
#if HAVE_BOTH_SQLITE
    handle *h = gethandle(env, obj);

    if (h) {
	return h->is3 ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
#else
#if HAVE_SQLITE2
    return JNI_FALSE;
#endif
#if HAVE_SQLITE3
    return JNI_TRUE;
#endif
#endif
}

JNIEXPORT jboolean JNICALL
Java_SQLite_Stmt_prepare(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3
    hvm *v = gethstmt(env, obj);
    void *svm = 0;
    char *tail;
    int ret;

    if (v && v->vm) {
	sqlite3_finalize((sqlite3_stmt *) v->vm);
	v->vm = 0;
    }
    if (v && v->h && v->h->sqlite) {
	if (!v->tail) {
	    return JNI_FALSE;
	}
	v->h->env = env;
#if HAVE_SQLITE3_PREPARE16_V2
	ret = sqlite3_prepare16_v2((sqlite3 *) v->h->sqlite,
				   v->tail, -1, (sqlite3_stmt **) &svm,
				   (const void **) &tail);
#else
	ret = sqlite3_prepare16((sqlite3 *) v->h->sqlite,
				v->tail, -1, (sqlite3_stmt **) &svm,
				(const void **) &tail);
#endif
	if (ret != SQLITE_OK) {
	    if (svm) {
		sqlite3_finalize((sqlite3_stmt *) svm);
		svm = 0;
	    }
	}
	if (ret != SQLITE_OK) {
	    const char *err = sqlite3_errmsg(v->h->sqlite);

	    setstmterr(env, obj, ret);
	    v->tail = 0;
	    throwex(env, err ? err : "error in compile/prepare");
	    return JNI_FALSE;
	}
	if (!svm) {
	    v->tail = 0;
	    return JNI_FALSE;
	}
	v->vm = svm;
	v->tail = (char *) tail;
	v->hh.row1 = 1;
	return JNI_TRUE;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_SQLite_Database_stmt_1prepare(JNIEnv *env, jobject obj, jstring sql,
				   jobject stmt)
{
#if HAVE_SQLITE3
    handle *h = gethandle(env, obj);
    void *svm = 0;
    hvm *v;
    jvalue vv;
    jsize len16;
    const jchar *sql16, *tail = 0;
    int ret;

    if (!h) {
	throwclosed(env);
	return;
    }
    if (!stmt) {
	throwex(env, "null stmt");
	return;
    }
    if (!sql) {
	throwex(env, "null sql");
	return;
    }
#ifdef HAVE_BOTH_SQLITE
    if (!h->is3) {
	throwex(env, "only on SQLite3 database");
	return;
    }
#endif
    len16 = (*env)->GetStringLength(env, sql) * sizeof (jchar);
    if (len16 < 1) {
	return;
    }
    h->env = env;
    sql16 = (*env)->GetStringChars(env, sql, 0);
#if HAVE_SQLITE3_PREPARE16_V2
    ret = sqlite3_prepare16_v2((sqlite3 *) h->sqlite, sql16, len16,
			       (sqlite3_stmt **) &svm, (const void **) &tail);
#else
    ret = sqlite3_prepare16((sqlite3 *) h->sqlite, sql16, len16,
			    (sqlite3_stmt **) &svm, (const void **) &tail);
#endif
    if (ret != SQLITE_OK) {
	if (svm) {
	    sqlite3_finalize((sqlite3_stmt *) svm);
	    svm = 0;
	}
    }
    if (ret != SQLITE_OK) {
	const char *err = sqlite3_errmsg(h->sqlite);

	(*env)->ReleaseStringChars(env, sql, sql16);
	setstmterr(env, stmt, ret);
	throwex(env, err ? err : "error in prepare");
	return;
    }
    if (!svm) {
	(*env)->ReleaseStringChars(env, sql, sql16);
	return;
    }
    len16 = len16 + sizeof (jchar) - ((char *) tail - (char *) sql16);
    if (len16 < sizeof (jchar)) {
	len16 = sizeof (jchar);
    }
    v = malloc(sizeof (hvm) + len16);
    if (!v) {
	(*env)->ReleaseStringChars(env, sql, sql16);
	sqlite3_finalize((sqlite3_stmt *) svm);
	throwoom(env, "unable to get SQLite handle");
	return;
    }
    v->next = h->vms;
    h->vms = v;
    v->vm = svm;
    v->h = h;
    v->tail = (char *) (v + 1);
#if HAVE_BOTH_SQLITE
    v->is3 = v->hh.is3 = 1;
#endif
    memcpy(v->tail, tail, len16);
    len16 /= sizeof (jchar);
    ((jchar *) v->tail)[len16 - 1] = 0;
    (*env)->ReleaseStringChars(env, sql, sql16);
    v->hh.sqlite = 0;
    v->hh.haveutf = h->haveutf;
    v->hh.ver = h->ver;
    v->hh.bh = v->hh.cb = v->hh.ai = v->hh.tr = v->hh.ph = 0;
    v->hh.row1 = 1;
    v->hh.enc = h->enc;
    v->hh.funcs = 0;
    v->hh.vms = 0;
    v->hh.env = 0;
    vv.j = 0;
    vv.l = (jobject) v;
    (*env)->SetLongField(env, stmt, F_SQLite_Stmt_handle, vv.j);
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT jboolean JNICALL
Java_SQLite_Stmt_step(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ret;

	ret = sqlite3_step((sqlite3_stmt *) v->vm);
	if (ret == SQLITE_ROW) {
	    return JNI_TRUE;
	}
	if (ret != SQLITE_DONE) {
	    const char *err = sqlite3_errmsg(v->h->sqlite);

	    setstmterr(env, obj, ret);
	    throwex(env, err ? err : "error in step");
	}
	return JNI_FALSE;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_close(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ret;

	ret = sqlite3_finalize((sqlite3_stmt *) v->vm);
	v->vm = 0;
	if (ret != SQLITE_OK) {
	    const char *err = sqlite3_errmsg(v->h->sqlite);

	    setstmterr(env, obj, ret);
	    throwex(env, err ? err : "error in close");
	}
	return;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return;
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_reset(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	sqlite3_reset((sqlite3_stmt *) v->vm);
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_clear_1bindings(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_CLEAR_BINDINGS
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	sqlite3_clear_bindings((sqlite3_stmt *) v->vm);
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_bind__II(JNIEnv *env, jobject obj, jint pos, jint val)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int npar = sqlite3_bind_parameter_count((sqlite3_stmt *) v->vm);
	int ret;

	if (pos < 1 || pos > npar) {
	    throwex(env, "parameter position out of bounds");
	    return;
	}
	ret = sqlite3_bind_int((sqlite3_stmt *) v->vm, pos, val);
	if (ret != SQLITE_OK) {
	    setstmterr(env, obj, ret);
	    throwex(env, "bind failed");
	}
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_bind__IJ(JNIEnv *env, jobject obj, jint pos, jlong val)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int npar = sqlite3_bind_parameter_count((sqlite3_stmt *) v->vm);
	int ret;

	if (pos < 1 || pos > npar) {
	    throwex(env, "parameter position out of bounds");
	    return;
	}
	ret = sqlite3_bind_int64((sqlite3_stmt *) v->vm, pos, val);
	if (ret != SQLITE_OK) {
	    setstmterr(env, obj, ret);
	    throwex(env, "bind failed");
	}
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_bind__ID(JNIEnv *env, jobject obj, jint pos, jdouble val)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int npar = sqlite3_bind_parameter_count((sqlite3_stmt *) v->vm);
	int ret;

	if (pos < 1 || pos > npar) {
	    throwex(env, "parameter position out of bounds");
	    return;
	}
	ret = sqlite3_bind_double((sqlite3_stmt *) v->vm, pos, val);
	if (ret != SQLITE_OK) {
	    setstmterr(env, obj, ret);
	    throwex(env, "bind failed");
	}
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_bind__I_3B(JNIEnv *env, jobject obj, jint pos, jbyteArray val)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int npar = sqlite3_bind_parameter_count((sqlite3_stmt *) v->vm);
	int ret;
	jint len;
	char *data = 0;

	if (pos < 1 || pos > npar) {
	    throwex(env, "parameter position out of bounds");
	    return;
	}
	if (val) {
	    len = (*env)->GetArrayLength(env, val);
	    if (len > 0) {
		data = sqlite3_malloc(len);
		if (!data) {
		    throwoom(env, "unable to get blob parameter");
		    return;
		}
		(*env)->GetByteArrayRegion(env, val, 0, len, (jbyte *) data);
		ret = sqlite3_bind_blob((sqlite3_stmt *) v->vm,
					pos, data, len, sqlite3_free);
	    } else {
		ret = sqlite3_bind_blob((sqlite3_stmt *) v->vm,
					pos, "", 0, SQLITE_STATIC);
	    }
	} else {
	    ret = sqlite3_bind_null((sqlite3_stmt *) v->vm, pos);
	}
	if (ret != SQLITE_OK) {
	    if (data) {
		sqlite3_free(data);
	    }
	    setstmterr(env, obj, ret);
	    throwex(env, "bind failed");
	}
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_bind__ILjava_lang_String_2(JNIEnv *env, jobject obj,
					    jint pos, jstring val)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int npar = sqlite3_bind_parameter_count((sqlite3_stmt *) v->vm);
	int ret;
	jsize len, count;
	char *data = 0;

	if (pos < 1 || pos > npar) {
	    throwex(env, "parameter position out of bounds");
	    return;
	}
	if (val) {
	    count = (*env)->GetStringLength(env, val);
	    len = count * sizeof (jchar);
	    if (len > 0) {
#ifndef JNI_VERSION_1_2
		const jchar *ch;
#endif
		data = sqlite3_malloc(len);
		if (!data) {
		    throwoom(env, "unable to get blob parameter");
		    return;
		}
#ifndef JNI_VERSION_1_2
		ch = (*env)->GetStringChars(env, val, 0);
		memcpy(data, ch, len);
		(*env)->ReleaseStringChars(env, val, ch);
#else
		(*env)->GetStringRegion(env, val, 0, count, (jchar *) data);
#endif
		ret = sqlite3_bind_text16((sqlite3_stmt *) v->vm,
					  pos, data, len, sqlite3_free);
	    } else {
		ret = sqlite3_bind_text16((sqlite3_stmt *) v->vm, pos, "", 0,
					  SQLITE_STATIC);
	    }
	} else {
	    ret = sqlite3_bind_null((sqlite3_stmt *) v->vm, pos);
	}
	if (ret != SQLITE_OK) {
	    if (data) {
		sqlite3_free(data);
	    }
	    setstmterr(env, obj, ret);
	    throwex(env, "bind failed");
	}
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_bind__I(JNIEnv *env, jobject obj, jint pos)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int npar = sqlite3_bind_parameter_count((sqlite3_stmt *) v->vm);
	int ret;

	if (pos < 1 || pos > npar) {
	    throwex(env, "parameter position out of bounds");
	    return;
	}
	ret = sqlite3_bind_null((sqlite3_stmt *) v->vm, pos);
	if (ret != SQLITE_OK) {
	    setstmterr(env, obj, ret);
	    throwex(env, "bind failed");
	}
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_bind_1zeroblob(JNIEnv *env, jobject obj, jint pos, jint len)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_BIND_ZEROBLOB
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int npar = sqlite3_bind_parameter_count((sqlite3_stmt *) v->vm);
	int ret;

	if (pos < 1 || pos > npar) {
	    throwex(env, "parameter position out of bounds");
	    return;
	}
	ret = sqlite3_bind_zeroblob((sqlite3_stmt *) v->vm, pos, len);
	if (ret != SQLITE_OK) {
	    setstmterr(env, obj, ret);
	    throwex(env, "bind failed");
	}
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT jint JNICALL
Java_SQLite_Stmt_bind_1parameter_1count(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	return sqlite3_bind_parameter_count((sqlite3_stmt *) v->vm);
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jstring JNICALL
Java_SQLite_Stmt_bind_1parameter_1name(JNIEnv *env, jobject obj, jint pos)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_BIND_PARAMETER_NAME
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int npar = sqlite3_bind_parameter_count((sqlite3_stmt *) v->vm);
	const char *name;

	if (pos < 1 || pos > npar) {
	    throwex(env, "parameter position out of bounds");
	    return 0;
	}
	name = sqlite3_bind_parameter_name((sqlite3_stmt *) v->vm, pos);
	if (name) {
	    return (*env)->NewStringUTF(env, name);
	}
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_SQLite_Stmt_bind_1parameter_1index(JNIEnv *env, jobject obj,
					jstring name)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_BIND_PARAMETER_INDEX
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int pos;
	const char *n;
	transstr namestr;
	jthrowable exc;

	n = trans2iso(env, 1, 0, name, &namestr);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    (*env)->DeleteLocalRef(env, exc);
	    return -1;
	}
	pos = sqlite3_bind_parameter_index((sqlite3_stmt *) v->vm, n);
	transfree(&namestr);
	return pos;
    } else {
	throwex(env, "stmt already closed");
    }
#else
    throwex(env, "unsupported");
#endif
    return -1;
}

JNIEXPORT jint JNICALL
Java_SQLite_Stmt_column_1int(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_data_count((sqlite3_stmt *) v->vm);

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	return sqlite3_column_int((sqlite3_stmt *) v->vm, col);
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jlong JNICALL
Java_SQLite_Stmt_column_1long(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_data_count((sqlite3_stmt *) v->vm);

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	return sqlite3_column_int64((sqlite3_stmt *) v->vm, col);
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jdouble JNICALL
Java_SQLite_Stmt_column_1double(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_data_count((sqlite3_stmt *) v->vm);

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	return sqlite3_column_double((sqlite3_stmt *) v->vm, col);
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jbyteArray JNICALL
Java_SQLite_Stmt_column_1bytes(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_data_count((sqlite3_stmt *) v->vm);
	int nbytes;
	const jbyte *data;
	jbyteArray b = 0;

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	data = sqlite3_column_blob((sqlite3_stmt *) v->vm, col);
	if (data) {
	    nbytes = sqlite3_column_bytes((sqlite3_stmt *) v->vm, col);
	} else {
	    return 0;
	}
	b = (*env)->NewByteArray(env, nbytes);
	if (!b) {
	    throwoom(env, "unable to get blob column data");
	    return 0;
	}
	(*env)->SetByteArrayRegion(env, b, 0, nbytes, data);
	return b;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jstring JNICALL
Java_SQLite_Stmt_column_1string(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_data_count((sqlite3_stmt *) v->vm);
	int nbytes;
	const jchar *data;
	jstring b = 0;

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	data = sqlite3_column_text16((sqlite3_stmt *) v->vm, col);
	if (data) {
	    nbytes = sqlite3_column_bytes16((sqlite3_stmt *) v->vm, col);
	} else {
	    return 0;
	}
	nbytes /= sizeof (jchar);
	b = (*env)->NewString(env, data, nbytes);
	if (!b) {
	    throwoom(env, "unable to get string column data");
	    return 0;
	}
	return b;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_SQLite_Stmt_column_1type(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_data_count((sqlite3_stmt *) v->vm);

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	return sqlite3_column_type((sqlite3_stmt *) v->vm, col);
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_SQLite_Stmt_column_1count(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	return sqlite3_column_count((sqlite3_stmt *) v->vm);
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jstring JNICALL
Java_SQLite_Stmt_column_1table_1name(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_COLUMN_TABLE_NAME16
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_column_count((sqlite3_stmt *) v->vm);
	const jchar *str;

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	str = sqlite3_column_table_name16((sqlite3_stmt *) v->vm, col);
	if (str) {
	    return (*env)->NewString(env, str, jstrlen(str));
	}
	return 0;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jstring JNICALL
Java_SQLite_Stmt_column_1database_1name(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_COLUMN_DATABASE_NAME16
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_column_count((sqlite3_stmt *) v->vm);
	const jchar *str;

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	str = sqlite3_column_database_name16((sqlite3_stmt *) v->vm, col);
	if (str) {
	    return (*env)->NewString(env, str, jstrlen(str));
	}
	return 0;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jstring JNICALL
Java_SQLite_Stmt_column_1decltype(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_column_count((sqlite3_stmt *) v->vm);
	const jchar *str;

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	str = sqlite3_column_decltype16((sqlite3_stmt *) v->vm, col);
	if (str) {
	    return (*env)->NewString(env, str, jstrlen(str));
	}
	return 0;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jstring JNICALL
Java_SQLite_Stmt_column_1name(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_column_count((sqlite3_stmt *) v->vm);
	const jchar *str;

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	str = sqlite3_column_name16((sqlite3_stmt *) v->vm, col);
	if (str) {
	    return (*env)->NewString(env, str, jstrlen(str));
	}
	return 0;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jstring JNICALL
Java_SQLite_Stmt_column_1origin_1name(JNIEnv *env, jobject obj, jint col)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_COLUMN_ORIGIN_NAME16
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	int ncol = sqlite3_column_count((sqlite3_stmt *) v->vm);
	const jchar *str;

	if (col < 0 || col >= ncol) {
	    throwex(env, "column out of bounds");
	    return 0;
	}
	str = sqlite3_column_origin_name16((sqlite3_stmt *) v->vm, col);
	if (str) {
	    return (*env)->NewString(env, str, jstrlen(str));
	}
	return 0;
    }
    throwex(env, "stmt already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_SQLite_Stmt_status(JNIEnv *env, jobject obj, jint op, jboolean flg)
{
    jint count = 0;
#if HAVE_SQLITE3 && HAVE_SQLITE3_STMT_STATUS
    hvm *v = gethstmt(env, obj);

    if (v && v->vm && v->h) {
	count = sqlite3_stmt_status((sqlite3_stmt *) v->vm, op,
				    flg == JNI_TRUE);
    }
#endif
    return count;
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_finalize(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE_COMPILE
    dostmtfinal(env, obj);
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1open_1blob(JNIEnv *env, jobject obj,
				  jstring dbname, jstring table,
				  jstring column, jlong row,
				  jboolean rw, jobject blobj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
    handle *h = gethandle(env, obj);
    hbl *bl;
    jthrowable exc;
    transstr dbn, tbl, col;
    sqlite3_blob *blob;
    jvalue vv;
    int ret;

    if (!blobj) {
	throwex(env, "null blob");
	return;
    }
#if HAVE_BOTH_SQLITE
    if (!h->is3) {
	throwex(env, "not a SQLite 3 database");
	return;
    }
#endif
    if (h && h->sqlite) {
	trans2iso(env, h->haveutf, h->enc, dbname, &dbn);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    (*env)->DeleteLocalRef(env, exc);
	    return;
	}
	trans2iso(env, h->haveutf, h->enc, table, &tbl);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    transfree(&dbn);
	    (*env)->DeleteLocalRef(env, exc);
	    return;
	}
	trans2iso(env, h->haveutf, h->enc, column, &col);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    transfree(&tbl);
	    transfree(&dbn);
	    (*env)->DeleteLocalRef(env, exc);
	    return;
	}
	ret = sqlite3_blob_open(h->sqlite,
				dbn.result, tbl.result, col.result,
				row, rw, &blob);
	transfree(&col);
	transfree(&tbl);
	transfree(&dbn);
	if (ret != SQLITE_OK) {
	    const char *err = sqlite3_errmsg(h->sqlite);

	    seterr(env, obj, ret);
	    throwex(env, err ? err : "error in blob open");
	    return;
	}
	bl = malloc(sizeof (hbl));
	if (!bl) {
	    sqlite3_blob_close(blob);
	    throwoom(env, "unable to get SQLite blob handle");
	    return;
	}
	bl->next = h->blobs;
	h->blobs = bl;
	bl->blob = blob;
	bl->h = h;
	vv.j = 0;
	vv.l = (jobject) bl;
	(*env)->SetLongField(env, blobj, F_SQLite_Blob_handle, vv.j);
	(*env)->SetIntField(env, blobj, F_SQLite_Blob_size,
			    sqlite3_blob_bytes(blob));
	return;
    }
    throwex(env, "not an open database");
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT jint JNICALL
Java_SQLite_Blob_write(JNIEnv *env , jobject obj, jbyteArray b, jint off,
		       jint pos, jint len)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
    hbl *bl = gethbl(env, obj);

    if (bl && bl->h && bl->blob) {
	jbyte *buf;
	jthrowable exc;
	int ret;

	if (len <= 0) {
	    return 0;
	}
	buf = malloc(len);
	if (!buf) {
	    throwoom(env, "out of buffer space for blob");
	    return 0;
	}
	(*env)->GetByteArrayRegion(env, b, off, len, buf);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    free(buf);
	    return 0;
	}
	ret = sqlite3_blob_write(bl->blob, buf, len, pos);
	free(buf);
	if (ret != SQLITE_OK) {
	    throwioex(env, "blob write error");
	    return 0;
	}
	return len;
    }
    throwex(env, "blob already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT jint JNICALL
Java_SQLite_Blob_read(JNIEnv *env , jobject obj, jbyteArray b, jint off,
		      jint pos, jint len)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
    hbl *bl = gethbl(env, obj);

    if (bl && bl->h && bl->blob) {
	jbyte *buf;
	jthrowable exc;
	int ret;

	if (len <= 0) {
	    return 0;
	}
	buf = malloc(len);
	if (!buf) {
	    throwoom(env, "out of buffer space for blob");
	    return 0;
	}
	ret = sqlite3_blob_read(bl->blob, buf, len, pos);
	if (ret != SQLITE_OK) {
	    free(buf);
	    throwioex(env, "blob read error");
	    return 0;
	}
	(*env)->SetByteArrayRegion(env, b, off, len, buf);
	free(buf);
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
	    return 0;
	}
	return len;
    }
    throwex(env, "blob already closed");
#else
    throwex(env, "unsupported");
#endif
    return 0;
}

JNIEXPORT void JNICALL
Java_SQLite_Blob_close(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
    doblobfinal(env, obj);
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Blob_finalize(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_INCRBLOBIO
    doblobfinal(env, obj);
#endif
}

JNIEXPORT void
JNICALL Java_SQLite_Database__1key(JNIEnv *env, jobject obj, jbyteArray key)
{
    jsize len;
    jbyte *data;
#if HAVE_SQLITE3_KEY
    handle *h = gethandle(env, obj);
#endif

    len = (*env)->GetArrayLength(env, key);
    data = (*env)->GetByteArrayElements(env, key, 0);
    if (len == 0) {
	data = 0;
    }
    if (!data) {
	len = 0;
    }
#if HAVE_SQLITE3_KEY
    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (!h->is3) {
	    if (data) {
		memset(data, 0, len);
	    }
	    throwex(env, "unsupported");
	}
#endif
	sqlite3_key((sqlite3 *) h->sqlite, data, len);
	if (data) {
	    memset(data, 0, len);
	}
    } else {
	if (data) {
	    memset(data, 0, len);
	}
	throwclosed(env);
    }
#else
    if (data) {
	memset(data, 0, len);
    }
    /* no error */
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Database__1rekey(JNIEnv *env, jobject obj, jbyteArray key)
{
    jsize len;
    jbyte *data;
#if HAVE_SQLITE3_KEY
    handle *h = gethandle(env, obj);
#endif

    len = (*env)->GetArrayLength(env, key);
    data = (*env)->GetByteArrayElements(env, key, 0);
    if (len == 0) {
	data = 0;
    }
    if (!data) {
	len = 0;
    }
#if HAVE_SQLITE3_KEY
    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (!h->is3) {
	    if (data) {
		memset(data, 0, len);
	    }
	    throwex(env, "unsupported");
	}
#endif
	sqlite3_rekey((sqlite3 *) h->sqlite, data, len);
	if (data) {
	    memset(data, 0, len);
	}
    } else {
	if (data) {
	    memset(data, 0, len);
	}
	throwclosed(env);
    }
#else
    if (data) {
	memset(data, 0, len);
    }
    throwex(env, "unsupported");
#endif
}

JNIEXPORT jboolean JNICALL
Java_SQLite_Database__1enable_1shared_1cache(JNIEnv *env, jclass cls,
					     jboolean onoff)
{
#if HAVE_SQLITE3_SHARED_CACHE
    return (sqlite3_enable_shared_cache(onoff == JNI_TRUE) == SQLITE_OK) ?
	   JNI_TRUE : JNI_FALSE;
#else
    return JNI_FALSE;
#endif
}


JNIEXPORT void JNICALL
Java_SQLite_Database__1backup(JNIEnv *env, jclass cls, jobject bkupj,
			      jobject dest, jstring destName,
			      jobject src, jstring srcName)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
    handle *hsrc = gethandle(env, src);
    handle *hdest = gethandle(env, dest);
    hbk *bk;
    jthrowable exc;
    transstr dbns, dbnd;
    sqlite3_backup *bkup;
    jvalue vv;

    if (!bkupj) {
	throwex(env, "null backup");
	return;
    }
    if (!hsrc) {
	throwex(env, "no source database");
	return;
    }
    if (!hdest) {
	throwex(env, "no destination database");
	return;
    }
#if HAVE_BOTH_SQLITE
    if (!hsrc->is3 || !hdest->is3) {
	throwex(env, "not a SQLite 3 database");
	return;
    }
#endif
    if (!hsrc->sqlite) {
	throwex(env, "source database not open");
	return;
    }
    if (!hdest->sqlite) {
	throwex(env, "destination database not open");
	return;
    }
    trans2iso(env, hdest->haveutf, hdest->enc, destName, &dbnd);
    exc = (*env)->ExceptionOccurred(env);
    if (exc) {
	(*env)->DeleteLocalRef(env, exc);
	return;
    }
    trans2iso(env, hsrc->haveutf, hsrc->enc, srcName, &dbns);
    exc = (*env)->ExceptionOccurred(env);
    if (exc) {
	transfree(&dbnd);
	(*env)->DeleteLocalRef(env, exc);
	return;
    }
    bkup = sqlite3_backup_init((sqlite3 *) hdest->sqlite, dbnd.result,
			       (sqlite3 *) hsrc->sqlite, dbns.result);
    transfree(&dbnd);
    transfree(&dbns);
    if (!bkup) {
	const char *err = sqlite3_errmsg((sqlite3 *) hdest->sqlite);

	seterr(env, src, sqlite3_errcode((sqlite3 *) hdest->sqlite));
	throwex(env, err ? err : "error in backup init");
	return;
    }
    bk = malloc(sizeof (hbk));
    if (!bk) {
	sqlite3_backup_finish(bkup);
	throwoom(env, "unable to get SQLite backup handle");
	return;
    }
    bk->next = hsrc->backups;
    hsrc->backups = bk;
    bk->bkup = bkup;
    bk->h = hsrc;
    vv.j = 0;
    vv.l = (jobject) bk;
    (*env)->SetLongField(env, bkupj, F_SQLite_Backup_handle, vv.j);
    return;
#else
    throwex(env, "unsupported");
#endif
}

JNIEXPORT void JNICALL
Java_SQLite_Backup__1finalize(JNIEnv *env, jobject obj)
{
#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
    hbk *bk = gethbk(env, obj);
    int ret = SQLITE_OK;
    char *err = 0;

    if (bk) {
	if (bk->h) {
	    handle *h = bk->h;
	    hbk *bkc, **bkp;

	    bkp = &h->backups;
	    bkc = *bkp;
	    while (bkc) {
		if (bkc == bk) {
		    *bkp = bkc->next;
		    break;
		}
		bkp = &bkc->next;
		bkc = *bkp;
	    }
	}
	if (bk->bkup) {
	    ret = sqlite3_backup_finish(bk->bkup);
	    if (ret != SQLITE_OK && bk->h) {
		err = (char *) sqlite3_errmsg((sqlite3 *) bk->h->sqlite);
	    }
	}
	bk->bkup = 0;
	free(bk);
	(*env)->SetLongField(env, obj, F_SQLite_Backup_handle, 0);
	if (ret != SQLITE_OK) {
	    throwex(env, err ? err : "unknown error");
	}
    }
#endif
}

JNIEXPORT jboolean JNICALL
Java_SQLite_Backup__1step(JNIEnv *env, jobject obj, jint n)
{
    jboolean result = JNI_TRUE;
#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
    hbk *bk = gethbk(env, obj);
    int ret;

    if (bk) {
	if (bk->bkup) {
	    ret = sqlite3_backup_step(bk->bkup, (int) n);
	    switch (ret) {
	    case SQLITE_DONE:
		break;
	    case SQLITE_LOCKED:
	    case SQLITE_BUSY:
	    case SQLITE_OK:
		result = JNI_FALSE;
		break;
	    default:
		result = JNI_FALSE;
		throwex(env, "backup step failed");
		break;
	    }
	}
    } else {
	throwex(env, "stale backup object");
    }
#else
    throwex(env, "unsupported");
#endif
    return result;
}

JNIEXPORT jint JNICALL
Java_SQLite_Backup__1remaining(JNIEnv *env, jobject obj)
{
    jint result = 0;
#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
    hbk *bk = gethbk(env, obj);

    if (bk) {
	if (bk->bkup) {
	    result = sqlite3_backup_remaining(bk->bkup);
	}
    }
#else
    throwex(env, "unsupported");
#endif
    return result;
}

JNIEXPORT jint JNICALL
Java_SQLite_Backup__1pagecount(JNIEnv *env, jobject obj)
{
    jint result = 0;
#if HAVE_SQLITE3 && HAVE_SQLITE3_BACKUPAPI
    hbk *bk = gethbk(env, obj);

    if (bk) {
	if (bk->bkup) {
	    result = sqlite3_backup_pagecount(bk->bkup);
	}
    }
#else
    throwex(env, "unsupported");
#endif
    return result;
}

#if HAVE_SQLITE3_PROFILE
static void
doprofile(void *arg, const char *msg, sqlite_uint64 est)
{
    handle *h = (handle *) arg;
    JNIEnv *env = h->env;

    if (env && h->pr && msg) {
	jthrowable exc;
	jclass cls = (*env)->GetObjectClass(env, h->pr);
	jmethodID mid;

	mid = (*env)->GetMethodID(env, cls, "profile",
				  "(Ljava/lang/String;J)V");
	if (mid) {
	    transstr tr;
#if _MSC_VER && (_MSC_VER < 1300)
	    jlong ms = est / (3600i64 * 24i64 * 1000i64);
#else
	    jlong ms = est / (3600LL * 24LL * 1000LL);
#endif

	    trans2utf(env, h->haveutf, h->enc, msg, &tr);
	    exc = (*env)->ExceptionOccurred(env);
	    if (exc) {
		(*env)->DeleteLocalRef(env, exc);
		(*env)->ExceptionClear(env);
		return;
	    }
	    (*env)->CallVoidMethod(env, h->pr, mid, tr.jstr, ms);
	    (*env)->ExceptionClear(env);
	    (*env)->DeleteLocalRef(env, tr.jstr);
	    return;
	}
    }
    return;
}
#endif

JNIEXPORT void JNICALL
Java_SQLite_Database__1profile(JNIEnv *env, jobject obj, jobject tr)
{
#if HAVE_SQLITE3_PROFILE
    handle *h = gethandle(env, obj);

    if (h && h->sqlite) {
	delglobrefp(env, &h->pr);
	globrefset(env, tr, &h->pr);
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    sqlite3_profile((sqlite3 *) h->sqlite, h->pr ? doprofile : 0, h);
	}
#else
#if HAVE_SQLITE3
	sqlite3_profile((sqlite3 *) h->sqlite, h->pr ? doprofile : 0, h);
#endif
#endif
    }
#endif
}

JNIEXPORT jint JNICALL
Java_SQLite_Database__1status(JNIEnv *env, jclass cls, jint op,
			      jintArray info, jboolean flag)
{
    jint ret = SQLITE_ERROR;
#if HAVE_SQLITE3_STATUS
    int data[2] = { 0, 0 };
    jint jdata[2];
#if HAVE_SQLITE3
    ret = sqlite3_status(op, &data[0], &data[1], flag);
    if (ret == SQLITE_OK) {
	jdata[0] = data[0];
	jdata[1] = data[1];
	(*env)->SetIntArrayRegion(env, info, 0, 2, jdata);
    }
#endif
#endif
    return ret;
}

JNIEXPORT jint JNICALL
Java_SQLite_Database__1db_1status(JNIEnv *env, jobject obj, jint op,
				  jintArray info, jboolean flag)
{
    jint ret = SQLITE_ERROR;
#if HAVE_SQLITE3_DB_STATUS
    handle *h = gethandle(env, obj);
    int data[2] = { 0, 0 };
    jint jdata[2];

    if (h && h->sqlite) {
#if HAVE_BOTH_SQLITE
	if (h->is3) {
	    ret = sqlite3_db_status((sqlite3 *) h->sqlite, op, &data[0],
				    &data[1], flag);
	}
#else
#if HAVE_SQLITE3
	ret = sqlite3_db_status((sqlite3 *) h->sqlite, op, &data[0],
				&data[1], flag);
#endif
#endif
	if (ret == SQLITE_OK) {
	    jdata[0] = data[0];
	    jdata[1] = data[1];
	    (*env)->SetIntArrayRegion(env, info, 0, 2, jdata);
	}
    }
#endif
    return ret;
}

JNIEXPORT void JNICALL
Java_SQLite_Stmt_internal_1init(JNIEnv *env, jclass cls)
{
    F_SQLite_Stmt_handle =
	(*env)->GetFieldID(env, cls, "handle", "J");
    F_SQLite_Stmt_error_code =
	(*env)->GetFieldID(env, cls, "error_code", "I");
}

JNIEXPORT void JNICALL
Java_SQLite_Vm_internal_1init(JNIEnv *env, jclass cls)
{
    F_SQLite_Vm_handle =
	(*env)->GetFieldID(env, cls, "handle", "J");
    F_SQLite_Vm_error_code =
	(*env)->GetFieldID(env, cls, "error_code", "I");
}

JNIEXPORT void JNICALL
Java_SQLite_Blob_internal_1init(JNIEnv *env, jclass cls)
{
    F_SQLite_Blob_handle =
	(*env)->GetFieldID(env, cls, "handle", "J");
    F_SQLite_Blob_size =
	(*env)->GetFieldID(env, cls, "size", "I");
}

JNIEXPORT void JNICALL
Java_SQLite_Backup_internal_1init(JNIEnv *env, jclass cls)
{
    F_SQLite_Backup_handle =
	(*env)->GetFieldID(env, cls, "handle", "J");
}

JNIEXPORT void JNICALL
Java_SQLite_Database_internal_1init(JNIEnv *env, jclass cls)
{
#if defined(DONT_USE_JNI_ONLOAD) || !defined(JNI_VERSION_1_2)
    while (C_java_lang_String == 0) {
	jclass jls = (*env)->FindClass(env, "java/lang/String");

	C_java_lang_String = (*env)->NewGlobalRef(env, jls);
    }
#endif
    F_SQLite_Database_handle =
	(*env)->GetFieldID(env, cls, "handle", "J");
    F_SQLite_Database_error_code =
	(*env)->GetFieldID(env, cls, "error_code", "I");
    M_java_lang_String_getBytes =
	(*env)->GetMethodID(env, C_java_lang_String, "getBytes", "()[B");
    M_java_lang_String_getBytes2 =
	(*env)->GetMethodID(env, C_java_lang_String, "getBytes",
			    "(Ljava/lang/String;)[B");
    M_java_lang_String_initBytes =
	(*env)->GetMethodID(env, C_java_lang_String, "<init>", "([B)V");
    M_java_lang_String_initBytes2 =
	(*env)->GetMethodID(env, C_java_lang_String, "<init>",
			    "([BLjava/lang/String;)V");
}

#if !defined(DONT_USE_JNI_ONLOAD) && defined(JNI_VERSION_1_2)
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env;
    jclass cls;

#ifndef _WIN32
#if HAVE_SQLITE2
    if (strcmp(sqlite_libencoding(), "UTF-8") != 0) {
	fprintf(stderr, "WARNING: using non-UTF SQLite2 engine\n");
    }
#endif
#endif
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_2)) {
	return JNI_ERR;
    }
    cls = (*env)->FindClass(env, "java/lang/String");
    if (!cls) {
	return JNI_ERR;
    }
    C_java_lang_String = (*env)->NewGlobalRef(env, cls);
    return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *vm, void *reserved)
{
    JNIEnv *env;

    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_2)) {
	return;
    }
    if (C_java_lang_String) {
	(*env)->DeleteGlobalRef(env, C_java_lang_String);
	C_java_lang_String = 0;
    }
}
#endif
