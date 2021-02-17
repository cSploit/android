#ifndef _DB_STL_COMMON_H
#define _DB_STL_COMMON_H

#ifdef DBSTL_DEBUG_LEAK
#include "vld.h"
#endif

#include <assert.h>

#include "db_cxx.h"

// In release builds, the native assert will be disabled so we
// can't use it in dbstl in cases where we rely on the expression being
// evaluated to change the state of the application.
//
#if !defined(DEBUG) && !defined(_DEBUG)
#undef dbstl_assert
#define dbstl_assert(expression)
#else
#undef dbstl_assert
#define dbstl_assert(expression) do {			\
	if (!(expression)) {				\
		FailedAssertionException ex(__FILE__, __LINE__, #expression);\
		throw ex; } } while (0)
#endif

#if defined( DB_WIN32) || defined(_WIN32)
#include <windows.h>
#include <tchar.h>
#else
#define TCHAR char
#define _T(e) (e)
#define _ftprintf fprintf
#define _snprintf snprintf
#define _tcschr strchr
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcslen strlen
#define _tgetopt getopt
#define _tmain main
#define _tprintf printf
#define _ttoi atoi
#endif

#undef SIZE_T_MAX
// The max value for size_t variables, one fourth of 2 powers 32.
#define SIZE_T_MAX 1073741824

// Macro for HAVE_WSTRING (detected by configure)
#define	HAVE_WSTRING	1

// Thread local storage modifier declaration.
#define	TLS_DECL_MODIFIER	__declspec(thread)
#define	TLS_DEFN_MODIFIER	__declspec(thread)

#if !defined(TLS_DECL_MODIFIER) && !defined(HAVE_PTHREAD_TLS)
#error "No appropriate TLS modifier defined."
#endif

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// C++ compiler portability control macro definitions.
// If a C++ compiler does not support the following capabilities, disabling
// these flags will remove usage of the feature from DB STL.
// Where possible a DB STL has implemented work-arounds for the missing
// functionality.
//
#define HAVE_EXPLICIT_KEYWORD			1
#define HAVE_NAMESPACE				1
#define HAVE_TYPENAME				1

// Platform specific compiler capability configuration.
#ifdef WIN32
#define CLS_SCOPE(clstmpl_name)
#else

// C++ standard: It is not possible to define a full specialized version of
// a member function of a class template inside the class body. It needs to
// be defined outside the class template, and must be defined in the namespace
// scope.
#define CLS_SCOPE(clstmpl_name) clstmpl_name::
#define NO_IN_CLASS_FULL_SPECIALIZATION  1
#define NO_MEMBER_FUNCTION_PARTIAL_SPECIALIZATION 1
#endif

#if HAVE_NAMESPACE
#define START_NS(nsname) namespace nsname {
#define END_NS }
#else
#define START_NS(nsname) struct nsname {
#define END_NS };
#endif

#if HAVE_EXPLICIT_KEYWORD
#define EXPLICIT explicit
#else
#define EXPLICIT
#endif

#if HAVE_TYPENAME
#define Typename typename
#else
#define Typename class
#endif

//////////////////////////////////////////////////////////////////////////
// End of compiler portability control macro definitions.
////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// Iterator status macro definitions.
//
#define INVALID_ITERATOR_POSITION -1 // Iterator goes out of valid range.
#define INVALID_ITERATOR_CURSOR -2 // The iterator's dbc cursor is invalid.
#define ITERATOR_DUP_ERROR -3 // Failed to duplicate a cursor.

// Current cursor's key or data dbt has no data.
#define INVALID_KEY_DATA -4
#define EMPTY_DBT_DATA -5 // Current cursor's pointed data dbt has no data.
#define ITERATOR_AT_END -6
#define CURSOR_NOT_OPEN -7

///////////////////////////////////////////////////////////////////////
// End of iterator status macro definitions.
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// Helper macros definitions.
//
// Use BDBOP and BDBOP2 to wrap Berkeley DB calls. The macros validate the 
// return value. On failure, the wrappers clean up, and generate the
// expected exception.
//
#define BDBOP(bdb_call, ret) do { 					\
	if ((ret = (bdb_call)) != 0) throw_bdb_exception(#bdb_call, ret);\
	} while(0)
#define BDBOP2(bdb_call, ret, cleanup) do {				\
	if ((ret = (bdb_call)) != 0) { (cleanup); 			\
		throw_bdb_exception(#bdb_call, ret);}			\
	} while (0)
// Do not throw the exception if bdb_call returned a specified error number.
#define BDBOP3(bdb_call, ret, exception, cleanup) do {			\
	if (((ret = (bdb_call)) != 0) && (ret & exception) == 0) {	\
		(cleanup); throw_bdb_exception(#bdb_call, ret);}	\
	} while (0)

#define THROW(exception_type, arg_list) do {		\
	exception_type ex arg_list; throw ex; } while (0)

#define THROW0(exception_type)	do {			\
	exception_type ex; throw ex; } while (0)

#define INVALID_INDEX ((index_type)-1)
#define INVALID_DLEN ((u_int32_t)-1)

#define DBSTL_MAX_DATA_BUF_LEN 1024 * 4096
#define DBSTL_MAX_KEY_BUF_LEN 1024 * 4096
#define DBSTL_MAX_MTX_ENV_MUTEX 4096 * 4
#define DBSTL_BULK_BUF_SIZE 256 * 1024

#define COMPARE_CHECK(obj) if (this == &obj) return true;
#define ASSIGNMENT_PREDCOND(obj) if (this == &obj) return obj;
//////////////////////////////////////////////////////////////////
// End of helper macro definitions.
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//
// Public global function declarations.
// These functions are open/public functionalities of dbstl for
// dbstl users to call.
//
START_NS(dbstl)
// _exported is a macro we employ from db_cxx.h of Berkeley DB C++
// API. If we want to export the symbols it decorates on Windows,
// we must define the macro "DB_CREATE_DLL", as is defined in dbstl
// project property.
/// \defgroup dbstl_global_functions dbstl global public functions
//@{

/// \name Functions to close database/environments.
/// Normally you don't have to close any database 
/// or environment handles, they will be closed automatically.
/// Though you still have the following API to close them.
//@{ 
/// Close pdb regardless of reference count. You must make sure pdb
/// is not used by others before calling this method.
/// You can close the underlying database of a container and assign 
/// another database with right configurations to it, if the configuration
/// is not suitable for the container, there will be an 
/// InvalidArgumentException type of exception thrown.
/// You can't use the container after you called close_db and before setting 
/// another valid database handle to the container via 
/// db_container::set_db_handle() function.
/// \param pdb The database handle to close.
_exported void close_db(Db *pdb);

/// Close all open database handles regardless of reference count.
/// You can't use any container after you called close_all_dbs and 
/// before setting another valid database handle to the 
/// container via db_container::set_db_handle() function.
/// \sa close_db(Db *);
_exported void close_all_dbs();

/// \brief Close specified database environment handle regardless of reference 
/// count. 
///
/// Make sure the environment is not used by any other databases.
/// \param pdbenv The database environment handle to close.
_exported void close_db_env(DbEnv *pdbenv);

/// \brief Close all open database environment handles regardless of
/// reference count.
///
/// You can't use the container after you called close_db and before setting
/// another valid database handle to the container via 
/// db_container::set_db_handle() function. \sa close_db_env(DbEnv *);
_exported void close_all_db_envs();
//@}

/// \name Transaction control global functions.
/// dbstl transaction API. You should call these API rather than DB C/C++
/// API to use Berkeley DB transaction features.
//@{ 
/// Begin a new transaction from the specified environment "env". 
/// This function is called by dbstl user to begin an external transaction.
/// The "flags" parameter is passed to DbEnv::txn_begin(). 
/// If a transaction created from 
/// the same database environment already exists and is unresolved,
/// the new transaction is started as a child transaction of that transaction,
/// and thus you can't specify the parent transaction.
/// \param env The environment to start a transaction from.
/// \param flags It is set to DbEnv::txn_begin() function.
/// \return The newly created transaction.
///
_exported DbTxn* begin_txn(u_int32_t flags, DbEnv *env);

/// Commit current transaction opened in the environment "env".
/// This function is called by user to commit an external explicit transaction.
/// \param env The environment whose current transaction is to be committed.
/// \param flags It is set to DbTxn::commit() funcion.
/// \sa commit_txn(DbEnv *, DbTxn *, u_int32_t);
///
_exported void commit_txn(DbEnv *env, u_int32_t flags = 0);

/// Commit a specified transaction and all its child transactions.
/// \param env The environment where txn is started from.
/// \param txn The transaction to commit, can be a parent transaction of a 
/// nested transaction group, all un-aborted child transactions of 
/// it will be committed. 
/// \param flags It is passed to each DbTxn::commit() call.
/// \sa commit_txn(DbEnv *, u_int32_t);
_exported void commit_txn(DbEnv *env, DbTxn *txn, u_int32_t flags = 0);

/// Abort current transaction of environment "env". This function is called by
/// dbstl user to abort an outside explicit transaction.
/// \param env The environment whose current transaction is to be aborted.
/// \sa abort_txn(DbEnv *, DbTxn *);
_exported void abort_txn(DbEnv *env);

/// Abort specified transaction "txn" and all its child transactions. 
/// That is, "txn" can be a parent transaction of a nested transaction group.
/// \param env The environment where txn is started from.
/// \param txn The transaction to abort, can be a parent transaction of a 
/// nested transaction group, all child transactions of it will be aborted.
/// \sa abort_txn(DbEnv *);
///
_exported void abort_txn(DbEnv *env, DbTxn *txn);

/// Get current transaction of environment "env".
/// \param env The environment whose current transaction we want to get.
/// \return Current transaction of env.
_exported DbTxn* current_txn(DbEnv *env);

/// Set environment env's current transaction handle to be newtxn. The original
/// transaction handle returned without aborting or commiting. This function
/// is used for users to use one transaction among multiple threads.
/// \param env The environment whose current transaction to replace.
/// \param newtxn The new transaction to be as the current transaction of env.
/// \return The old current transaction of env. It is not resolved.
_exported DbTxn* set_current_txn_handle(DbEnv *env, DbTxn *newtxn);
//@} 

/// \name Functions to open and register database/environment handles.
//@{ 
/// Register a Db handle "pdb1". This handle and handles opened in it will be
/// closed by ResourceManager, so application code must not try to close or
/// delete it. Users can do enough configuration before opening the Db then
/// register it via this function.
/// All database handles should be registered via this function in each 
/// thread using the handle. The only exception is the database handle opened
/// by dbstl::open_db should not be registered in the thread of the 
/// dbstl::open_db call.
/// \param pdb1 The database handle to register into dbstl for current thread.
///
_exported void register_db(Db *pdb1);

/// Register a DbEnv handle env1, this handle and handles opened in it will be
/// closed by ResourceManager. Application code must not try to close or delete
/// it. Users can do enough config before opening the DbEnv and then register
/// it via this function.
/// All environment handles should be registered via this function in each 
/// thread using the handle. The only exception is the environment handle 
/// opened by dbstl::open_db_env should not be registered in the thread of 
/// the dbstl::open_db_env call.
/// \param env1 The environment to register into dbstl for current thread.
///
_exported void register_db_env(DbEnv *env1);

/// Helper function to open a database and register it into dbstl for the 
/// calling thread.
/// Users still need to register it in any other thread using it if it
/// is shared by multiple threads, via register_db() function.
/// Users don't need to delete or free the memory of the returned object, 
/// dbstl will take care of that.
/// When you don't use dbstl::open_db() but explicitly call DB C++ API to
/// open a database, you must new the Db object, rather than create it 
/// on stack, and you must delete the Db object by yourself.
/// \param penv The environment to open the database from.
/// \param cflags The create flags passed to Db class constructor.
/// \param filename The database file name, passed to Db::open.
/// \param dbname The database name, passed to Db::open.
/// \param dbtype The database type, passed to Db::open.
/// \param oflags The database open flags, passed to Db::open.
/// \param mode The database open mode, passed to Db::open.
/// \param txn The transaction to open the database from, passed to Db::open.
/// \param set_flags The flags to be set to the created database handle.
/// \return The opened database handle.
/// \sa register_db(Db *);
/// \sa open_db_env;
///
_exported Db* open_db (DbEnv *penv, const char *filename, DBTYPE dbtype,
    u_int32_t oflags, u_int32_t set_flags, int mode = 0644, DbTxn *txn = NULL,
    u_int32_t cflags = 0, const char* dbname = NULL);

/// Helper function to open an environment and register it into dbstl for the
/// calling thread. Users still need to register it in any other thread if it
/// is shared by multiple threads, via register_db_env() function above.
/// Users don't need to delete or free the memory of the returned object, 
/// dbstl will take care of that.
/// 
/// When you don't use dbstl::open_env() but explicitly call DB C++ API to
/// open an environment, you must new the DbEnv object, rather than create it
/// on stack, and you must delete the DbEnv object by yourself.
/// \param env_home Environment home directory, it must exist. Passed to 
/// DbEnv::open.
/// \param cflags DbEnv constructor creation flags, passed to DbEnv::DbEnv.
/// \param set_flags Flags to set to the created environment before opening it.
/// \param oflags Environment open flags, passed to DbEnv::open.
/// \param mode Environment region files mode, passed to DbEnv::open.
/// \param cachesize Environment cache size, by default 4M bytes.
/// \return The opened database environment handle.
/// \sa register_db_env(DbEnv *); 
/// \sa open_db;
///
_exported DbEnv* open_env(const char *env_home, u_int32_t set_flags,
    u_int32_t oflags = DB_CREATE | DB_INIT_MPOOL,
    u_int32_t cachesize = 4 * 1024 * 1024,
    int mode = 0644,
    u_int32_t cflags = 0/* Flags for DbEnv constructor. */);
//@}

/// @name Mutex API based on Berkeley DB mutex.
/// These functions are in-process mutex support which uses Berkeley DB 
/// mutex mechanisms. You can call these functions to do portable 
/// synchronization for your code.
//@{
/// Allocate a Berkeley DB mutex.
/// \return Berkeley DB mutex handle.
_exported db_mutex_t alloc_mutex();
/// Lock a mutex, wait if it is held by another thread.
/// \param mtx The mutex handle to lock.
/// \return 0 if succeed, non-zero otherwise, call db_strerror to get message.
_exported int lock_mutex(db_mutex_t mtx);
/// Unlock a mutex, and return immediately.
/// \param mtx The mutex handle to unlock.
/// \return 0 if succeed, non-zero otherwise, call db_strerror to get message.
_exported int unlock_mutex(db_mutex_t mtx);
/// Free a mutex, and return immediately.
/// \param mtx The mutex handle to free.
/// \return 0 if succeed, non-zero otherwise, call db_strerror to get message.
_exported void free_mutex(db_mutex_t mtx);
//@}

/// Close cursors opened in dbp1.
/// \param dbp1 The database handle whose active cursors to close.
/// \return The number of cursors closed by this call.
_exported size_t close_db_cursors(Db* dbp1);

/// \name Other global functions.
//@{
/// If there are multiple threads within a process that make use of dbstl, then
/// this function should be called in a single thread mutual exclusively before
/// any use of dbstl in a process; Otherwise, you don't need to call it, but
/// are allowed to call it anyway.
_exported void dbstl_startup();

/// This function releases any memory allocated in the heap by code of dbstl, 
/// and close all DB handles in the right order.
/// So you can only call dbstl_exit() right before the entire process exits.
/// It will release any memory allocated by dbstl that have to live during
/// the entire process lifetime.
_exported void dbstl_exit();

/// This function release all DB handles in the right order. The environment
/// and database handles are only closed when they are not used by other 
/// threads, otherwise the reference cout is decremented.
_exported void dbstl_thread_exit();

/// Operators to compare two Dbt objects.
/// \param d1 Dbt object to compare.
/// \param d2 Dbt object to compare.
_exported bool operator==(const Dbt&d1, const Dbt&d2);
/// Operators to compare two DBT objects.
/// \param d1 DBT object to compare.
/// \param d2 DBT object to compare.
_exported bool operator==(const DBT&d1, const DBT&d2);

/// If exisiting random temporary database name generation mechanism is still
/// causing name clashes, users can set this global suffix number which will
/// be append to each temporary database file name and incremented after each
/// append, and by default it is 0.
/// \param num Starting number to append to each temporary db file name.
_exported void set_global_dbfile_suffix_number(u_int32_t num);
//@}

//@} // dbstl_global_functions

// Internally used memory allocation functions, they will throw an exception
// of NotEnoughMemoryException if can't allocate memory.
_exported void * DbstlReAlloc(void *ptr, size_t size);
_exported void * DbstlMalloc(size_t size);

_exported u_int32_t hash_default(Db * /*dbp*/, const void *key, u_int32_t len);

// Default string manipulation callbacks.
_exported u_int32_t dbstl_strlen(const char *str);
_exported void dbstl_strcpy(char *dest, const char *src, size_t num);
_exported int dbstl_strncmp(const char *s1, const char *s2, size_t num);
_exported int dbstl_strcmp(const char *s1, const char *s2);
_exported int dbstl_wcscmp(const wchar_t *s1, const wchar_t *s2);
_exported int dbstl_wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t num);
_exported u_int32_t dbstl_wcslen(const wchar_t *str);
_exported void dbstl_wcscpy(wchar_t *dest, const wchar_t *src, size_t num);

END_NS

//////////////////////////////////////////////////////////////////
// End of public global function declarations.
//////////////////////////////////////////////////////////////////

#endif /* !_DB_STL_COMMON_H */
