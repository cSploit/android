/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import java.lang.IllegalArgumentException;
import java.lang.NullPointerException;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;
import com.sleepycat.db.internal.DbSite;
import com.sleepycat.db.ReplicationManagerSite;

/**
A database environment.  Environments include support for some or
all of caching, locking, logging and transactions.
<p>
To open an existing environment with default attributes the application
may use a default environment configuration object or null:
<p>
<blockquote><pre>
    // Open an environment handle with default attributes.
    Environment env = new Environment(home, new EnvironmentConfig());
</pre></blockquote>
<p>
or
<p>
<blockquote><pre>
    Environment env = new Environment(home, null);
</pre></blockquote>
<p>
Note that many Environment objects may access a single environment.
<p>
To create an environment or customize attributes, the application should
customize the configuration class. For example:
<p>
<blockquote><pre>
    EnvironmentConfig envConfig = new EnvironmentConfig();
    envConfig.setTransactional(true);
    envConfig.setAllowCreate(true);
    envConfig.setCacheSize(1000000);
    <p>
    Environment newlyCreatedEnv = new Environment(home, envConfig);
</pre></blockquote>
<p>
Environment handles are free-threaded unless {@link com.sleepycat.db.EnvironmentConfig#setThreaded EnvironmentConfig.setThreaded} is called to disable this before the environment is opened.
<p>
An <em>environment handle</em> is an Environment instance.  More than
one Environment instance may be created for the same physical directory,
which is the same as saying that more than one Environment handle may
be open at one time for a given environment.
<p>
The Environment handle should not be closed while any other handle
remains open that is using it as a reference (for example,
{@link com.sleepycat.db.Database Database} or {@link com.sleepycat.db.Transaction Transaction}.  Once {@link com.sleepycat.db.Environment#close Environment.close}
is called, this object may not be accessed again, regardless of
whether or not it throws an exception.
*/
public class Environment {
    private DbEnv dbenv;
    private int autoCommitFlag;

    /* package */
    Environment(final DbEnv dbenv)
        throws DatabaseException {

        this.dbenv = dbenv;
        dbenv.wrapper = this;
    }

    /**
    Create a database environment handle.
    <p>
    @param home
    The database environment's home directory.
    The environment variable <code>DB_HOME</code> may be used as
    the path of the database home.
    For more information on <code>envHome</code> and filename
    resolution in general, see
    <a href="{@docRoot}/../programmer_reference/env_naming.html" target="_top">File Naming</a>.
    <p>
    @param config The database environment attributes.  If null, default attributes are used.
    <p>
    <p>
    @throws IllegalArgumentException if an invalid parameter was specified.
    <p>
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public Environment(final java.io.File home, EnvironmentConfig config)
        throws DatabaseException, java.io.FileNotFoundException {

        this(EnvironmentConfig.checkNull(config).openEnvironment(home));
        this.autoCommitFlag =
            ((dbenv.get_open_flags() & DbConstants.DB_INIT_TXN) == 0) ? 0 :
                DbConstants.DB_AUTO_COMMIT;
    }

    /**
    Close the database environment, freeing any allocated resources and
    closing any underlying subsystems.
    <p>
    When you call this method, all open database and cursor handles are closed automatically, and should not be reused. 
    <p>
    The {@link com.sleepycat.db.Environment Environment} handle should not be closed while any other
    handle that refers to it is not yet closed; for example, database
    environment handles must not be closed while 
    transactions in the environment have not yet been committed
    or aborted.  Specifically, this includes 
{@link com.sleepycat.db.Transaction Transaction}, and {@link com.sleepycat.db.LogCursor LogCursor}
    handles.
    <p>
    In multiple threads of control, each thread of control opens a database environment and the database handles within it. When you close each database handle using this method, by default, the database is not synchronized. To synchronize all open databases ensure that the last environment 
    object is closed using the Environment.CloseForceSync() method. 
	When the close operation fails, the method returns a non-zero error value for the 
	first instance of such error, and continues to close the rest of the environment objects. 
    <p>
    Where the environment was initialized with a locking subsystem,
    closing the environment does not release any locks still held by the
    closing process, providing functionality for long-lived locks.
    <p>
    Where the environment was initialized with a transaction subsystem,
    closing the environment aborts any unresolved transactions.
    Applications should not depend on this behavior for transactions
    involving databases; all such transactions should be explicitly
    resolved.  The problem with depending on this semantic is that
    aborting an unresolved transaction involving database operations
    requires a database handle.  Because the database handles should
    have been closed before closing the environment, it will not be
    possible to abort the transaction, and recovery will have to be run
    on the database environment before further operations are done.
    <p>
    Where log cursors were created, closing the environment does not
    imply closing those cursors.
    <p>
    In multithreaded applications, only a single thread may call this
    method.
    <p>
    After this method has been called, regardless of its return, the
    {@link com.sleepycat.db.Environment Environment} handle may not be accessed again.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void close()
        throws DatabaseException {

        dbenv.close(0);
    }
    /**
    Close the database environment, freeing any allocated resources and
    closing any underlying subsystems. 
    <p>
    This function has verify similar
    behavior as Environment.close(), except the following:
    When each open database handle is closed, the database is synchronized.
    */
    public void closeForceSync()
        throws DatabaseException {

        dbenv.close(DbConstants.DB_FORCESYNC);
    }

    /* package */
    DbEnv unwrap() {
        return dbenv;
    }

    /**
    Destroy a database environment.
    <p>
    If the environment is not in use, the environment regions, including
    any backing files, are removed.  Any log or database files and the
    environment directory itself are not removed.
    <p>
    If there are processes currently using the database environment,
    this method will fail without further action (unless the force
    argument is true, in which case the environment will be removed,
    regardless of any processes still using it).
    <p>
    The result of attempting to forcibly destroy the environment when
    it is in use is unspecified.  Processes using an environment often
    maintain open file descriptors for shared regions within it.  On
    UNIX systems, the environment removal will usually succeed, and
    processes that have already joined the region will continue to run
    in that region without change.  However, processes attempting to
    join the environment will either fail or create new regions.  On
    other systems in which the unlink system call will fail if any
    process has an open file descriptor for the file (for example
    Windows/NT), the region removal will fail.
    <p>
    Calling this method should not be necessary for most applications
    because the environment is cleaned up as part of normal
    database recovery procedures. However, applications may want to call
    this method as part of application shut down to free up system
    resources.  For example, if system shared memory was used to back
    the database environment, it may be useful to call this method in
    order to release system shared memory segments that have been
    allocated.  Or, on architectures in which mutexes require allocation
    of underlying system resources, it may be useful to call
    this method in order to release those resources.  Alternatively, if
    recovery is not required because no database state is maintained
    across failures, and no system resources need to be released, it is
    possible to clean up an environment by simply removing all the
    Berkeley DB files in the database environment's directories.
    <p>
    In multithreaded applications, only a single thread may call this
    method.
    <p>
    After this method has been called, regardless of its return, the
    {@link com.sleepycat.db.Environment Environment} handle may not be 
    accessed again.
    <p>
    @param home
    The database environment to be removed.
    On Windows platforms, this argument will be interpreted as a UTF-8
    string, which is equivalent to ASCII for Latin characters.
    <p>
    @param force
    The environment is removed, regardless of any processes that may
    still using it, and no locks are acquired during this process.
    (Generally, the force argument is specified only when applications
    were unable to shut down cleanly, and there is a risk that an
    application may have died holding a Berkeley DB mutex or lock.
    <p>
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public static void remove(final java.io.File home,
                              final boolean force,
                              EnvironmentConfig config)
        throws DatabaseException, java.io.FileNotFoundException {

        config = EnvironmentConfig.checkNull(config);
        int flags = force ? DbConstants.DB_FORCE : 0;
        flags |= config.getUseEnvironment() ?
            DbConstants.DB_USE_ENVIRON : 0;
        flags |= config.getUseEnvironmentRoot() ?
            DbConstants.DB_USE_ENVIRON_ROOT : 0;
        final DbEnv dbenv = config.createEnvironment();
        dbenv.remove((home == null) ? null : home.toString(), flags);
    }

    /**
    Change the settings in an existing environment handle.
    <p>
    @param config The database environment attributes.  If null, default attributes are used.
    <p>
    <p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setConfig(final EnvironmentConfig config)
        throws DatabaseException {

        config.configureEnvironment(dbenv, new EnvironmentConfig(dbenv));
    }

    /**
    Return this object's configuration.
    <p>
    @return
    This object's configuration.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public EnvironmentConfig getConfig()
        throws DatabaseException {

        return new EnvironmentConfig(dbenv);
    }

    /* Manage databases. */
    /**
    Open a database.
<p>
The database is represented by the file and database parameters.
<p>
The currently supported database file formats (or <em>access
methods</em>) are Btree, Hash, Queue, and Recno.  The Btree format is a
representation of a sorted, balanced tree structure.  The Hash format
is an extensible, dynamic hashing scheme.  The Queue format supports
fast access to fixed-length records accessed sequentially or by logical
record number.  The Recno format supports fixed- or variable-length
records, accessed sequentially or by logical record number, and
optionally backed by a flat text file.
<p>
Storage and retrieval are based on key/data pairs; see {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}
for more information.
<p>
Opening a database is a relatively expensive operation, and maintaining
a set of open databases will normally be preferable to repeatedly
opening and closing the database for each new query.
<p>
In-memory databases never intended to be preserved on disk may be
created by setting both the fileName and databaseName parameters to
null.  Note that in-memory databases can only ever be shared by sharing
the single database handle that created them, in circumstances where
doing so is safe.  The environment variable <code>TMPDIR</code> may
be used as a directory in which to create temporary backing files.
<p>
@param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
Note that transactionally protected operations on a Database handle
require that the Database handle itself be transactionally protected
during its open, either with a non-null transaction handle, or by calling
{@link com.sleepycat.db.DatabaseConfig#setTransactional DatabaseConfig.setTransactional} on the configuration object.
<p>
@param fileName
The name of an underlying file that will be used to back the database.
On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
<p>
@param databaseName
An optional parameter that allows applications to have multiple
databases in a single file.  Although no databaseName parameter needs
to be specified, it is an error to attempt to open a second database in
a physical file that was not initially created using a databaseName
parameter.  Further, the databaseName parameter is not supported by the
Queue format.
<p>
@param config The database open attributes.  If null, default attributes are used.
    */
    public Database openDatabase(final Transaction txn,
                                 final String fileName,
                                 final String databaseName,
                                 DatabaseConfig config)
        throws DatabaseException, java.io.FileNotFoundException {

        return new Database(
            DatabaseConfig.checkNull(config).openDatabase(dbenv,
                (txn == null) ? null : txn.txn,
                fileName, databaseName));
    }

    /**
    Open a database.
<p>
The database is represented by the file and database parameters.
<p>
The currently supported database file formats (or <em>access
methods</em>) are Btree, Hash, Queue, and Recno.  The Btree format is a
representation of a sorted, balanced tree structure.  The Hash format
is an extensible, dynamic hashing scheme.  The Queue format supports
fast access to fixed-length records accessed sequentially or by logical
record number.  The Recno format supports fixed- or variable-length
records, accessed sequentially or by logical record number, and
optionally backed by a flat text file.
<p>
Storage and retrieval are based on key/data pairs; see {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}
for more information.
<p>
Opening a database is a relatively expensive operation, and maintaining
a set of open databases will normally be preferable to repeatedly
opening and closing the database for each new query.
<p>
In-memory databases never intended to be preserved on disk may be
created by setting both the fileName and databaseName parameters to
null.  Note that in-memory databases can only ever be shared by sharing
the single database handle that created them, in circumstances where
doing so is safe.  The environment variable <code>TMPDIR</code> may
be used as a directory in which to create temporary backing files.
<p>
@param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
Note that transactionally protected operations on a Database handle
require that the Database handle itself be transactionally protected
during its open, either with a non-null transaction handle, or by calling
{@link com.sleepycat.db.DatabaseConfig#setTransactional DatabaseConfig.setTransactional} on the configuration object.
<p>
@param fileName
The name of an underlying file that will be used to back the database.
On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
<p>
@param databaseName
An optional parameter that allows applications to have multiple
databases in a single file.  Although no databaseName parameter needs
to be specified, it is an error to attempt to open a second database in
a physical file that was not initially created using a databaseName
parameter.  Further, the databaseName parameter is not supported by the
Queue format.
<p>
@param primaryDatabase
a database handle for the primary database that is to be indexed.
<p>
@param config The secondary database open attributes.  If null, default attributes are used.
    */
    public SecondaryDatabase openSecondaryDatabase(
            final Transaction txn,
            final String fileName,
            final String databaseName,
            final Database primaryDatabase,
            SecondaryConfig config)
        throws DatabaseException, java.io.FileNotFoundException {

        return new SecondaryDatabase(
            SecondaryConfig.checkNull(config).openSecondaryDatabase(
                dbenv, (txn == null) ? null : txn.txn,
                fileName, databaseName, primaryDatabase.db),
            primaryDatabase);
    }

    /**
    <p>
Remove the database specified by the fileName and databaseName parameters.
<p>
If no database is specified, the underlying file specified is removed, incidentally removing all of the databases it contained.
<p>
Applications should never remove databases with open {@link com.sleepycat.db.Database Database}
handles, or in the case of removing a file, when any database in the
file has an open handle.  For example, some architectures do not permit
the removal of files with open system handles.  On these architectures,
attempts to remove databases currently in use by any thread of control
in the system may fail.
<p>
The
environment variable DB_HOME may be used as the path of the database
environment home.
<p>
This method is affected by any database directory specified with
{@link com.sleepycat.db.EnvironmentConfig#addDataDir EnvironmentConfig.addDataDir}, or by setting the "set_data_dir"
string in the database environment's DB_CONFIG file.
<p>
The {@link com.sleepycat.db.Database Database} handle may not be accessed
again after this method is called, regardless of this method's success
or failure.
<p>
@param txn
If the operation is part of an application-specified transaction, the txn
parameter is a Transaction object returned from the
{@link com.sleepycat.db.Environment#beginTransaction Environment.beginTransaction} method; if the operation is part of a Berkeley DB
Concurrent Data Store group, the txn parameter is a Transaction object returned
from the {@link com.sleepycat.db.Environment#beginCDSGroup Environment.beginCDSGroup} method; otherwise null.
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
<p>
@param fileName
The physical file which contains the database to be removed.
On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
<p>
@param databaseName
The database to be removed.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public void removeDatabase(final Transaction txn,
                               final String fileName,
                               final String databaseName)
        throws DatabaseException, java.io.FileNotFoundException {

        dbenv.dbremove((txn == null) ? null : txn.txn,
            fileName, databaseName,
            (txn == null) ? autoCommitFlag : 0);
    }

    /**
    <p>
Rename a database.
<p>
If no database name is specified, the underlying file specified is
renamed, incidentally renaming all of the databases it contains.
<p>
Applications should never rename databases that are currently in use.
If an underlying file is being renamed and logging is currently enabled
in the database environment, no database in the file may be open when
this method is called.  In particular, some architectures do not permit
renaming files with open handles.  On these architectures, attempts to
rename databases that are currently in use by any thread of control in
the system may fail.
<p>
The
environment variable DB_HOME may be used as the path of the database
environment home.
<p>
This method is affected by any database directory specified with
{@link com.sleepycat.db.EnvironmentConfig#addDataDir EnvironmentConfig.addDataDir}, or by setting the "set_data_dir"
string in the database environment's DB_CONFIG file.
<p>
The {@link com.sleepycat.db.Database Database} handle may not be accessed
again after this method is called, regardless of this method's success
or failure.
<p>
@param txn
If the operation is part of an application-specified transaction, the txn
parameter is a Transaction object returned from the
{@link com.sleepycat.db.Environment#beginTransaction Environment.beginTransaction} method; if the operation is part of a Berkeley DB
Concurrent Data Store group, the txn parameter is a Transaction object returned
from the {@link com.sleepycat.db.Environment#beginCDSGroup Environment.beginCDSGroup} method; otherwise null.
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
<p>
@param fileName
The physical file which contains the database to be renamed.
On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
<p>
@param databaseName
The database to be renamed.
<p>
@param newName
The new name of the database or file.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public void renameDatabase(final Transaction txn,
                               final String fileName,
                               final String databaseName,
                               final String newName)
        throws DatabaseException, java.io.FileNotFoundException {

        dbenv.dbrename((txn == null) ? null : txn.txn,
            fileName, databaseName, newName,
            (txn == null) ? autoCommitFlag : 0);
    }

    /**
    Return the database environment home directory. This directory is normally
    identified in the {@link com.sleepycat.db.Environment} constructor.
    <p>
    @return The database environment home directory.
    */
    public java.io.File getHome()
        throws DatabaseException {

        String home = dbenv.get_home();
        return (home == null) ? null : new java.io.File(home);
    }

    /* Cache management. */
    /**
    Ensure that a specified percent of the pages in the shared memory
    pool are clean, by writing dirty pages to their backing files.
    <p>
    The purpose of this method is to enable a memory pool manager to ensure
    that a page is always available for reading in new information
    without having to wait for a write.
    <p>
    @param percent
    The percent of the pages in the cache that should be clean.
    <p>
    @return
    The number of pages that were written to reach the specified
    percentage.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public int trickleCacheWrite(int percent)
        throws DatabaseException {

        return dbenv.memp_trickle(percent);
    }

    /* Locking */
    /**
    Run one iteration of the deadlock detector.
    <p>
    The deadlock detector traverses the lock table and marks one of the
    participating lock requesters for rejection in each deadlock it finds.
    <p>
    @param mode
    Which lock request(s) to reject.
    <p>
    @return
    The number of lock requests that were rejected.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public int detectDeadlocks(LockDetectMode mode)
        throws DatabaseException {

        return dbenv.lock_detect(0, mode.getFlag());
    }

    /**
    Acquire a lock from the lock table returning information about it in the Lock parameter.
    <p>
    @param locker
    An unsigned 32-bit integer quantity representing the entity
    requesting the lock.
    <p>
    @param mode
    The lock mode.
    <p>
    @param noWait
    If a lock cannot be granted because the requested lock conflicts
    with an existing lock, throw a {@link com.sleepycat.db.LockNotGrantedException LockNotGrantedException}
    immediately instead of waiting for the lock to become available.
    <p>
    @param object
    An untyped byte string that specifies the object to be locked.
    Applications using the locking subsystem directly while also doing
    locking via the Berkeley DB access methods must take care not to
    inadvertently lock objects that happen to be equal to the unique
    file IDs used to lock files.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public Lock getLock(int locker,
                        boolean noWait,
                        DatabaseEntry object,
                        LockRequestMode mode)
        throws DatabaseException {

        return Lock.wrap(
            dbenv.lock_get(locker, noWait ? DbConstants.DB_LOCK_NOWAIT : 0,
                object, mode.getFlag()));
    }

    /**
    Release a lock.
    <p>
    @param lock
    The lock to be released.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void putLock(Lock lock)
        throws DatabaseException {

        dbenv.lock_put(lock.unwrap());
    }

    /**
    Allocate a locker ID.
    <p>
    The locker ID is guaranteed to be unique for the database environment.
    <p>
    Call {@link com.sleepycat.db.Environment#freeLockerID Environment.freeLockerID} to return the locker ID to
    the environment when it is no longer needed.
    <p>
    @return
    A locker ID.
    */
    public int createLockerID()
        throws DatabaseException {

        return dbenv.lock_id();
    }

    /**
    Free a locker ID.
    <p>
    @param id
    The locker id to be freed.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void freeLockerID(int id)
        throws DatabaseException {

        dbenv.lock_id_free(id);
    }

    /**
    Return the deadlock priority for the given locker.
    <p>
    @param id
    The locker id
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public int getLockerPriority(int id) throws DatabaseException {
        return dbenv.get_lk_priority(id);
    }

    /**
    Assign a deadlock priority to a locker.  This value is used when resolving deadlocks. The deadlock resolution algorithm will reject a
    lock request from a locker with a lower priority before a request from a locker with a higher
    priority.
    By default, all lockers are created with a priority of 100. 
    <p>
    @param id
    The locker id to configure
    @param priority
    The priority to assign to the locker.
    <p>
    @throws DatabaseException if a failure occurs
    */
    public void setLockerPriority(int id, int priority)
	throws DatabaseException {
        dbenv.set_lk_priority(id, priority);
    }

    /**
    Atomically obtain and release one or more locks from the lock table.
    This method is intended to support acquisition or trading of
    multiple locks under one lock table semaphore, as is needed for lock
    coupling or in multigranularity locking for lock escalation.
    <p>
    If any of the requested locks cannot be acquired, or any of the locks to
    be released cannot be released, the operations before the failing
    operation are guaranteed to have completed successfully, and
    the method throws an exception.
    <p>
    @param noWait
    If a lock cannot be granted because the requested lock conflicts
    with an existing lock, throw a {@link com.sleepycat.db.LockNotGrantedException LockNotGrantedException}
    immediately instead of waiting for the lock to become available.
    The index of the request that was not granted will be returned by
    {@link com.sleepycat.db.LockNotGrantedException#getIndex LockNotGrantedException.getIndex}.
    <p>
    @param locker
    An unsigned 32-bit integer quantity representing the entity
    requesting the lock.
    <p>
    @param list
    An array of {@link com.sleepycat.db.LockRequest LockRequest} objects, listing the requested lock
    operations.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void lockVector(int locker, boolean noWait, LockRequest[] list)
        throws DatabaseException {

        dbenv.lock_vec(locker, noWait ? DbConstants.DB_LOCK_NOWAIT : 0,
            list, 0, list.length);
    }

    /* Logging */
    /**
    Return a log cursor.
    <p>
    @return
    A log cursor.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public LogCursor openLogCursor()
        throws DatabaseException {

        return LogCursor.wrap(dbenv.log_cursor(0));
    }

    /**
    Return the name of the log file that contains the log record
    specified by a LogSequenceNumber object.
    <p>
    This mapping of LogSequenceNumber objects to files is needed for
    database administration.  For example, a transaction manager
    typically records the earliest LogSequenceNumber object needed for
    restart, and the database administrator may want to archive log
    files to tape when they contain only log records before the earliest
    one needed for restart.
    <p>
    @param lsn
    The LogSequenceNumber object for which a filename is wanted.
    <p>
    @return
    The name of the log file that contains the log record specified by a
    LogSequenceNumber object.
    <p>
    <p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public String getLogFileName(LogSequenceNumber lsn)
        throws DatabaseException {

        return dbenv.log_file(lsn);
    }

    /**
    Verify integrity of the log records of an environment and write both error and 
    normal messages to the  environment's error report destination.
    This method does not perform the locking function, even in Berkeley DB environments 
    that are configured with a locking subsystem. 
    Because this function does not access any database files, you can call it even 
    when the environment has other threads of control attached and running
    <p>
    @param config
    The LogVerifyConfig object which contains configurations for the log 
    verification.
    <p>
    @return
    If the verification succeeds, return 0; otherwise non-zero. 
    Error, warning and report messages are written to the environment's 
    error report destination.
    */
    public int logVerify(LogVerifyConfig config) {

        String env_home, dbfile, dbname;
        int caf, efile, eoffset, flags, ret, stfile, stoffset, verbose;
        long etime, stime;
        LogSequenceNumber endlsn, startlsn;

        caf = ret = verbose = 0;
        env_home = config.getEnvHome();
        dbfile = config.getDbFile();
        dbname = config.getDbName();
        stime = 0;
        etime = 0;
        if (config.getContinueAfterFail())
            caf = 1;
        if (config.getVerbose())
            verbose = 1;
        if (config.getStartTime() != null)
            stime = config.getStartTime().getTime() / 1000;
        if (config.getEndTime() != null)
            etime = config.getEndTime().getTime() / 1000;
        startlsn = config.getStartLsn();
        endlsn = config.getEndLsn();
        efile = eoffset = stfile = stoffset = 0;
        if (endlsn != null)
            efile = endlsn.getFile();
        if (endlsn != null)
            eoffset = endlsn.getOffset();
        if (startlsn != null)
            stfile = startlsn.getFile();
        if (startlsn != null)
            stoffset = startlsn.getOffset();
        try {
            ret = dbenv.log_verify(env_home, config.getCacheSize(), 
                dbfile, dbname, stime, etime, 
                stfile, stoffset, efile, eoffset, caf, verbose);
        } catch (Exception ex) {  }
        return (ret);
    }
    /* Replication support */
    /**
    Configure the database environment as a client or master in a group
    of replicated database environments.  This method is not called by most replication applications. 
    It should only be called by Base API applications implementing their 
    own network transport layer, explicitly holding replication group elections and 
    handling replication messages outside of the Replication Manager framework.
    <p>
    Replication master
    environments are the only database environments where replicated
    databases may be modified.  Replication client environments are
    read-only as long as they are clients.  Replication client
    environments may be upgraded to be replication master environments
    in the case that the current master fails or there is no master
    present. If master leases are in use, this method cannot be used to 
    appoint a master, and should only be used to configure a database 
    environment as a master as the result of an election.
    <p>
    The enclosing database environment must already have been configured
    to send replication messages by calling 
    {@link com.sleepycat.db.EnvironmentConfig#setReplicationTransport EnvironmentConfig.setReplicationTransport}.
    <p>
    @param cdata
    An opaque data item that is sent over the communication infrastructure
    when the client or master comes online.  If no such information is
    useful, cdata should be null.
    <p>
    @param master
    Configure the environment as a replication master.  If false, the
    environment will be configured as as a replication client.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void startReplication(DatabaseEntry cdata, boolean master)
        throws DatabaseException {

        dbenv.rep_start(cdata,
            master ? DbConstants.DB_REP_MASTER : DbConstants.DB_REP_CLIENT);
    }

    /**
    Hold an election for the master of a replication group. This method is not called 
    by most replication applications. It should only be called by Base API applications 
    implementing their own network transport layer, explicitly holding replication group 
    elections and handling replication messages outside of the Replication Manager framework.
    <p>
    If the election is successful, the new master's ID may be the ID of the
    previous master, or the ID of the current environment.  The application
    is responsible for adjusting its usage of the other environments in the
    replication group, including directing all database updates to the newly
    selected master, in accordance with the results of this election.
    <p>
    The thread of control that calls this method must not be the thread
    of control that processes incoming messages; processing the incoming
    messages is necessary to successfully complete an election.
    <p>
    Before calling this method do the following:
    <ul>
    <li>open the database environment.
    <li>configure the database environment to send replication messages.
    <li>configure the database environment as a client or a master.
    </ul>
    <h3>How Elections are Held</h3>
    <p>
    Elections are done in two parts: first, replication sites collect information from the 
    other replication sites they know about, and second, replication sites cast their votes for a new master.
     The second phase is triggered by one of two things: either the replication site gets election 
     information from nsites sites, or the election timeout expires. Once the second phase is triggered, 
     the replication site will cast a vote for the new master of its choice if, and only if, 
     the site has election information from at least nvotes sites. If a site receives nvotes votes 
     for it to become the new master, then it will become the new master.
     We recommend nvotes be set to at least:
	<p>
    (sites participating in the election / 2) + 1
	<p>
to ensure there are never more than two masters active at the same time even in the case of a network partition. 
When a network partitions, the side of the partition with more than half the environments will elect 
a new master and continue, while the environments communicating with fewer than half of the 
environments will fail to find a new master, as no site can get nvotes votes.
We recommend nsites be set to:
<p>
number of sites in the replication group - 1
<p>
when choosing a new master after a current master fails. This allows the group to reach a consensus without having to wait for the timeout to expire.
<p>
When choosing a master from among a group of client sites all restarting at the same time, it makes more sense to set nsites to the total number of sites in the group, since there is no known missing site. Furthermore, in order to ensure the best choice from among sites that may take longer to boot than the local site, setting nvotes also to this same total number of sites will guarantee that every site in the group is considered. Alternatively, using the special timeout for full elections allows full participation on restart but allows election of a master if one site does not reboot and rejoin the group in a reasonable amount of time. (See the Elections section in the Berkeley DB Programmer's Reference Guide for more information.)
<p>
Setting nsites to lower values can increase the speed of an election, but can also result in election failure, and is usually not recommended.  
    <p>
    
    @param nsites
    The number of environments that the application believes are in the
    replication group.  This number is used by Berkeley DB to avoid
    having two masters active simultaneously, even in the case of a
    network partition.  During an election, a new master cannot be
    elected unless more than half of nsites agree on the new master.
    Thus, in the face of a network partition, the side of the partition
    with more than half the environments will elect a new master and
    continue, while the environments communicating with fewer than half
    the other environments will fail to find a new master.
    <p>
    @param nvotes
    The number of votes required by the application to successfully
    elect a new master.  It must be a positive integer, no greater than
    nsites, or 0 if the election should use a simple majority of the
    nsites value as the requirement.  A warning is given if half or
    fewer votes are required to win an election as that can potentially
    lead to multiple masters in the face of a network partition.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void electReplicationMaster(int nsites, int nvotes)
        throws DatabaseException {
        dbenv.rep_elect(nsites, nvotes, 0 /* unused flags */);
    }

    /**
    Internal method: re-push the last log record to all clients, in case they've
    lost messages and don't know it.
    <p>
    This method may not be called before the database environment is opened.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    **/
    public void flushReplication()
        throws DatabaseException {

        dbenv.rep_flush();
    }

    /**
    Process an incoming replication message sent by a member of the
    replication group to the local database environment. This method is not called 
    by most replication applications. It should only be called by Base API applications 
    implementing their own network transport layer, explicitly holding replication 
    group elections and handling replication messages outside of the Replication Manager framework.
    <p>
    For implementation reasons, all incoming replication messages must
    be processed using the same {@link com.sleepycat.db.Environment Environment} handle.  It is not
    required that a single thread of control process all messages, only
    that all threads of control processing messages use the same handle.
    <p>
    Before calling this method, the enclosing database environment must already 
    have been opened and must already have been configured to send replication messages.
    <p>
    @param control
    A copy of the control parameter specified by Berkeley DB on the
    sending environment.
    <p>
    @param envid
    The local identifier that corresponds to the environment that sent
    the message to be processed.
    <p>
    @param rec
    A copy of the rec parameter specified by Berkeley DB on the sending
    environment.
    <p>
    @return
    A {@link com.sleepycat.db.ReplicationStatus ReplicationStatus} object.
    */
    public ReplicationStatus processReplicationMessage(DatabaseEntry control,
                                                       DatabaseEntry rec,
                                                       int envid)
        throws DatabaseException {
        // Create a new entry so that rec isn't overwritten
        final DatabaseEntry cdata =
            new DatabaseEntry(rec.getData(), rec.getOffset(), rec.getSize());
        final LogSequenceNumber lsn = new LogSequenceNumber();
        final int ret =
            dbenv.rep_process_message(control, cdata, envid, lsn);
        return ReplicationStatus.getStatus(ret, cdata, envid, lsn);
    }

    /**
    Configure the replication subsystem.
    <p>
    The database environment's replication subsystem may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "rep_set_config", one or more whitespace characters, and     the method configuration parameter as a string; for example,
    "rep_set_config REP_CONF_NOWAIT".
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param config
    A replication feature to be configured.
    @param onoff
    If true, the feature is enabled, otherwise it is disabled.
    **/
    public void setReplicationConfig(ReplicationConfig config, boolean onoff)
        throws DatabaseException {

        dbenv.rep_set_config(config.getFlag(), onoff);
    }

    /**
    Get the configuration of the replication subsystem.
    This method may be called at any time during the life of the application.
    @return
    Whether the specified feature is enabled or disabled.
    **/
    public boolean getReplicationConfig(ReplicationConfig config)
        throws DatabaseException {

        return dbenv.rep_get_config(config.getFlag());
    }

    /**
    Get the number of sites in a replication group.
    Applications using the replication manager API may only call this method
    after replication has been started.
    @return
    The number of sites in the replication group
    @throws
    DatabaseException if a failure occurs
    **/
    public int getReplicationNumSites()
        throws DatabaseException {
        return dbenv.rep_get_nsites();
    }

    /**
    Sets the timeout applied to the specified timeout type.
    This method may be called at any time during the life of the application.
    <p>
    The database environment's replication subsystem may also be configured 
    using the environment's DB_CONFIG file. The syntax of the entry in that 
    file is a single line with the string "rep_set_timeout", one or more whitespace characters, 
    and the which parameter specified as a string and the timeout specified as two parts.
    Because the DB_CONFIG file is read when the database environment is opened, 
    it will silently overrule configuration done before that time.
    @param type
    The type of timeout to set.
    <p>
    @param replicationTimeout
    The time in microseconds of the desired timeout.
    **/
    public void setReplicationTimeout(
        final ReplicationTimeoutType type, final int replicationTimeout)
        throws DatabaseException {

        dbenv.rep_set_timeout(type.getId(), replicationTimeout);
    }

    /**
    Gets the timeout applied to the specified timeout type.
    @param type
    The type of timeout to retrieve.
    <p>
    @return
    The timeout applied to the specified timout type, in microseconds.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    **/
    public int getReplicationTimeout(final ReplicationTimeoutType type)
        throws DatabaseException {

        return dbenv.rep_get_timeout(type.getId());
    }

    /**
    Forces master synchronization to begin for this client.  This method is the other
    half of setting {@link ReplicationConfig#DELAYCLIENT} with
    {@link #setReplicationConfig}.
    <p>
    When a new master is elected and the application has configured delayed
    synchronization, the application must choose when to perform
    synchronization by using this method.  Otherwise the client will remain
    unsynchronized and will ignore all new incoming log messages.
    This method may be called any time after the client application learns 
    that the new master has been established.
    <p>
    Before calling this method, the enclosing database environment must 
    already have been opened and must already have been configured to send replication messages. 
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    **/
    public void syncReplication() throws DatabaseException {
        dbenv.rep_sync(0);
    }

    /* Replication Manager interface */
    /**
    Starts the replication manager.
    <p>
    The replication manager is implemented inside the Berkeley DB library,
    it is designed to manage a replication group. This includes network
    transport, all replication message processing and acknowledgment, and
    group elections.
    <p>
    For more information on building replication manager applications,
    please see the "Replication Manager Getting Started Guide" included in
    the Berkeley DB documentation.
    <p>
    This method may not be called before the database environment is opened.
    @param nthreads
    Specify the number of threads of control created and dedicated to
    processing replication messages. In addition to these message processing
    threads, the replication manager creates and manages a few of its own
    threads of control.
    <p>
    @param policy
    The policy defines the startup characteristics of a replication group.
    See {@link com.sleepycat.db.ReplicationManagerStartPolicy ReplicationManagerStartPolicy} for more information.
    **/
    public void replicationManagerStart(
        int nthreads, ReplicationManagerStartPolicy policy)
        throws DatabaseException {

        dbenv.repmgr_start(nthreads, policy.getId());
    }

    /**
    Return a handle for the local replication site.
    */
    public ReplicationManagerSite getReplicationManagerLocalSite()
        throws DatabaseException {

        DbSite site = dbenv.repmgr_local_site();
        return (site == null) ? null :
            new ReplicationManagerSite(site);
    }

    /**
    Return an array of all the sites known to the replication manager.
    This method may only be called after replication has been started
    using the {@link com.sleepycat.db.Environment#replicationManagerStart replicationManagerStart} method.
    */
    public ReplicationManagerSiteInfo[] getReplicationManagerSiteList() 
        throws DatabaseException {

        return dbenv.repmgr_site_list();
    }

    /**
    Return a site known to the replication manager by its eid.
    <p>
    @param eid
    */
    public ReplicationManagerSite getReplicationManagerSite(int eid) 
        throws DatabaseException {

        return new ReplicationManagerSite(dbenv.repmgr_site_by_eid(eid));
    }

    /**
    Return a site in the replication manager by its host and port.
    <p>
    @param host
    @param port
    */
    public ReplicationManagerSite getReplicationManagerSite(
        String host, long port)
        throws DatabaseException {

        return new ReplicationManagerSite(dbenv.repmgr_site(host, port));
    }

    /**
    Create a channel.
    <p>
    @param eid
    */
    public ReplicationChannel openChannel(int eid)
        throws DatabaseException {

        return new ReplicationChannel(dbenv.repmgr_channel(eid, 0));
    }

    /**
    Set the message dispatch function.
    <p>
    @param dispatch
    Application-specific function used to handle messages sent over Replication
    Manager message channels.
    @param flags
    This flag is DB_REPMGR_NEED_RESPONSE if the message requires a response.
    Otherwise, it is 0. 
    */
    public void setReplicationManagerMessageDispatch(
        ReplicationManagerMessageDispatch dispatch, int flags) 
        throws DatabaseException {

        dbenv.repmgr_set_dispatch(dispatch, flags);
    }

    /* Statistics */
    /**
    Returns the memory pool (that is, the buffer cache) subsystem statistics. 
    <p>
    @return the memory pool (that is, the buffer cache) subsystem statistics.
    */
    public CacheStats getCacheStats(StatsConfig config)
        throws DatabaseException {

        return dbenv.memp_stat(StatsConfig.checkNull(config).getFlags());
    }

    /**
    Return statistics for individual files in the cache.
    <p>
    @return statistics for individual files in the cache.
    */
    public CacheFileStats[] getCacheFileStats(StatsConfig config)
        throws DatabaseException {

        return dbenv.memp_fstat(StatsConfig.checkNull(config).getFlags());
    }

    /**
    Return the database environment's logging statistics.
    <p>
    @param config The statistics attributes.  If null, default attributes are used.
    <p>
    @return
    The database environment's logging statistics.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public LogStats getLogStats(StatsConfig config)
        throws DatabaseException {

        return dbenv.log_stat(StatsConfig.checkNull(config).getFlags());
    }

    /**
    Return the database environment's replication statistics.
    <p>
    @param config The statistics attributes.  If null, default attributes are used.
    <p>
    @return
    The database environment's replication statistics.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public ReplicationStats getReplicationStats(StatsConfig config)
        throws DatabaseException {

        return dbenv.rep_stat(StatsConfig.checkNull(config).getFlags());
    }

    /**
    Return the database environment's replication manager statistics.
    <p>
    @param config The statistics attributes.  If null, default attributes are used.
    <p>
    @return
    The database environment's replication manager statistics.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public ReplicationManagerStats getReplicationManagerStats(
        StatsConfig config)
        throws DatabaseException {

        return dbenv.repmgr_stat(StatsConfig.checkNull(config).getFlags());
    }

    /**
    Return the database environment's locking statistics.
    <p>
    @param config The locking statistics attributes.  If null, default attributes are used.
    <p>
    @return
    The database environment's locking statistics.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public LockStats getLockStats(StatsConfig config)
        throws DatabaseException {

        return dbenv.lock_stat(StatsConfig.checkNull(config).getFlags());
    }

    /**
    Return the database environment's mutex statistics.
    <p>
    @param config The statistics attributes.  If null, default attributes are used.
    <p>
    @return
    The database environment's mutex statistics.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public MutexStats getMutexStats(StatsConfig config)
        throws DatabaseException {

        return dbenv.mutex_stat(StatsConfig.checkNull(config).getFlags());
    }

    /**
    Return the database environment's transactional statistics.
    <p>
    @param config The transactional statistics attributes.  If null, default attributes are used.
    <p>
    @return
    The database environment's transactional statistics.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public TransactionStats getTransactionStats(StatsConfig config)
        throws DatabaseException {

        return dbenv.txn_stat(StatsConfig.checkNull(config).getFlags());
    }

    /* Transaction management */
    /**
    Allocate a locker ID in an environment configured for Berkeley DB
    Concurrent Data Store applications.   Returns a {@link Transaction} object
    that uniquely identifies the locker ID.  Calling the {@link
    Transaction#commit} method will discard the allocated locker ID.
    <p>
    See
    <a href="{@docRoot}/../programmer_reference/cam.html#cam_intro" target="_top">Berkeley DB Concurrent Data Store applications</a>
    for more information about when this is required.
    This method may be called at any time during the life of the application.
    <p>
    @return
    A transaction handle that wraps a CDS locker ID.
    */
    public Transaction beginCDSGroup() throws DatabaseException {

        return new Transaction(dbenv.cdsgroup_begin());
    }

    /**
    Create a new transaction in the database environment.
    <p>
    Transactions may only span threads if they do so serially; that is,
    each transaction must be active in only a single thread of control
    at a time.
    <p>
    This restriction holds for parents of nested transactions as well;
    no two children may be concurrently active in more than one thread
    of control at any one time.
    <p>
    Cursors may not span transactions; that is, each cursor must be opened
    and closed within a single transaction.
    <p>
    A parent transaction may not issue any Berkeley DB operations --
    except for {@link com.sleepycat.db.Environment#beginTransaction Environment.beginTransaction},
    {@link com.sleepycat.db.Transaction#abort Transaction.abort} and {@link com.sleepycat.db.Transaction#commit Transaction.commit} --
    while it has active child transactions (child transactions that have
    not yet been committed or aborted).
    <p>
    @param parent
    If the parent parameter is non-null, the new transaction will be a
    nested transaction, with the transaction indicated by parent as its
    parent.  Transactions may be nested to any level.  In the presence
    of distributed transactions and two-phase commit, only the parental
    transaction, that is a transaction without a parent specified,
    should be passed as an parameter to {@link com.sleepycat.db.Transaction#prepare Transaction.prepare}.
    <p>
    @param config
    The transaction attributes.  If null, default attributes are used.
    <p>
    @return
    The newly created transaction's handle.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public Transaction beginTransaction(final Transaction parent,
                                        TransactionConfig config)
        throws DatabaseException {

        return new Transaction(
            TransactionConfig.checkNull(config).beginTransaction(dbenv,
                (parent == null) ? null : parent.txn));
    }

    /**
    Synchronously checkpoint the database environment.
    <p>
    <p>
    @param config
    The checkpoint attributes.  If null, default attributes are used.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public void checkpoint(CheckpointConfig config)
        throws DatabaseException {

        CheckpointConfig.checkNull(config).runCheckpoint(dbenv);
    }

    /**
    Flush log records to stable storage.
    <p>
    @param lsn
    All log records with LogSequenceNumber values less than or equal to
    the lsn parameter are written to stable storage.  If lsn is null,
    all records in the log are flushed.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void logFlush(LogSequenceNumber lsn)
        throws DatabaseException {

        dbenv.log_flush(lsn);
    }

    /**
    Append a record to the log.
    <p>
    @param data
    The record to append to the log.
    <p>
    The caller is responsible for providing any necessary structure to
    data.  (For example, in a write-ahead logging protocol, the
    application must understand what part of data is an operation code,
    what part is redo information, and what part is undo information.
    In addition, most transaction managers will store in data the
    LogSequenceNumber of the previous log record for the same
    transaction, to support chaining back through the transaction's log
    records during undo.)
    <p>
    @param flush
    The log is forced to disk after this record is written, guaranteeing
    that all records with LogSequenceNumber values less than or equal
    to the one being "put" are on disk before this method returns.
    <p>
    @return
    The LogSequenceNumber of the put record.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public LogSequenceNumber logPut(DatabaseEntry data, boolean flush)
        throws DatabaseException {

        final LogSequenceNumber lsn = new LogSequenceNumber();
        dbenv.log_put(lsn, data, flush ? DbConstants.DB_FLUSH : 0);
        return lsn;
    }

    /**
    Append an informational message to the Berkeley DB database environment log files.
    <p>
    This method allows applications to include information in
    the database environment log files, for later review using the
    <a href="{@docRoot}/../api_reference/C/db_printlog.html" target="_top">db_printlog</a>
     utility.  This method is intended for debugging and performance tuning.
     <p>
    @param txn
    If the logged message refers to an application-specified transaction,
    the <code>txn</code> parameter is a transaction handle, otherwise
    <code>null</code>.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    **/
    public void logPrint(Transaction txn, String message)
        throws DatabaseException {

        dbenv.log_print((txn == null) ? null : txn.txn, message);
    }

    /**
    Return an array of log files.
    <p>
    When Replication Manager is in use, log archiving is performed in a
    replication group-aware manner such that the log file status of other sites
    in the group is considered to determine if a log file is in use. 
    <p>
    Log cursor handles (returned by the
    {@link com.sleepycat.db.Environment#openLogCursor openLogCursor() method} may
    have open file descriptors for log files in the database environment. Also,
    the Berkeley DB interfaces to the database environment logging subsystem (for
    example, {@link com.sleepycat.db.Environment#logPut logPut} and
    {@link com.sleepycat.db.Transaction#abort Transaction.abort} may allocate log
    cursors and have open file descriptors for log files as well. On operating
    systems where filesystem related system calls (for example, rename and unlink
    on Windows/NT) can fail if a process has an open file descriptor for the
    affected file, attempting to move or remove returned log files may fail. All
    Berkeley DB internal use of log cursors operates on active log files only and
    furthermore, is short-lived in nature. So, an application seeing such a
    failure should be restructured to close any open log cursors it may have, and
    otherwise to retry the operation until it succeeds. (Although the latter is
    not likely to be necessary; it is hard to imagine a reason to move or rename
    a log file in which transactions are being logged or aborted.)
    <p>
    See
    <a href="{@docRoot}/../api_reference/C/db_archive.html" target="_top">db_archive</a>
    for more information on database archival procedures. 
    <p>
    @param includeInUse if true, all log files, regardless of whether or not they
    are in use, are returned.  Otherwise, the log files that are no longer in use
    (for example, that are no longer involved in active transactions), and that
    may safely be archived for catastrophic recovery and then removed from the
    system.
    <p>
    @return An array of log files
    */
    public java.io.File[] getArchiveLogFiles(boolean includeInUse)
        throws DatabaseException {

        final String[] logNames = dbenv.log_archive(DbConstants.DB_ARCH_ABS |
                (includeInUse ? DbConstants.DB_ARCH_LOG : 0));
        final int len = (logNames == null) ? 0 : logNames.length;
        final java.io.File[] logFiles = new java.io.File[len];
        for (int i = 0; i < len; i++)
            logFiles[i] = new java.io.File(logNames[i]);
        return logFiles;
    }

    /**
    Return the database files that need to be archived in order to recover the
    database from catastrophic failure. If any of the database files have not
    been accessed during the lifetime of the current log files, they will not be
    returned. It is also possible that some of the files referred to by the log
    have since been deleted from the system.
    <p>
    When Replication Manager is in use, log archiving is performed in a
    replication group-aware manner such that the log file status of other sites
    in the group is considered to determine if a log file is in use. 
    <p>
    See
    <a href="{@docRoot}/../api_reference/C/db_archive.html" target="_top">db_archive</a>
    for more information on database archival procedures. 
    <p>
    @return An array of database files
    */
    public java.io.File[] getArchiveDatabases()
        throws DatabaseException {

        final String home = dbenv.get_home();
        final String[] dbNames = dbenv.log_archive(DbConstants.DB_ARCH_DATA);
        final int len = (dbNames == null) ? 0 : dbNames.length;
        final java.io.File[] dbFiles = new java.io.File[len];
        for (int i = 0; i < len; i++)
            dbFiles[i] = new java.io.File(home, dbNames[i]);
        return dbFiles;
    }

    /**
    Remove log files that are no longer needed.
    <p>
    Automatic log file removal is likely to make catastrophic recovery
    impossible.
    <p>
    When Replication Manager is in use, log archiving is performed in a
    replication group-aware manner such that the log file status of other sites
    in the group is considered to determine if a log file is in use.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void removeOldLogFiles()
        throws DatabaseException {

        dbenv.log_archive(DbConstants.DB_ARCH_REMOVE);
    }

    /**
    Environment recovery restores transactions that were prepared, but not yet
    resolved at the time of the system shut down or crash, to their state prior
    to the shut down or crash, including any locks previously held. This method
    returns a list of those prepared transactions.
    <p>
    This method should only be called after the environment has been recovered.
    <p>
    Multiple threads of control may call this method, but only one thread of
    control may resolve each returned transaction, that is, only one thread of
    control may call 
    {@link com.sleepycat.db.Transaction#commit Transaction.commit()} or 
    {@link com.sleepycat.db.Transaction#abort Transaction.abort()} each returned
    transaction. Callers must call 
    {@link com.sleepycat.db.Transaction#discard Transaction.discard()} to discard
    each transaction they do not resolve.
    <p>
    @return a list of {@link com.sleepycat.db.PreparedTransaction transactions}
    that must be resolved by the application (committed, aborted or discarded).
    */
    public PreparedTransaction[] recover(final int count,
                                         final boolean continued)
        throws DatabaseException {

        return dbenv.txn_recover(count,
            continued ? DbConstants.DB_NEXT : DbConstants.DB_FIRST);
    }

    /**
    Allows database files to be copied, and then the copy used in the same
    database environment as the original.
    <p>
    All databases contain an ID string used to identify the database in the
    database environment cache.  If a physical database file is copied, and
    used in the same environment as another file with the same ID strings,
    corruption can occur.  This method creates new ID strings for all of
    the databases in the physical file.
    <p>
    This method modifies the physical file, in-place.
    Applications should not reset IDs in files that are currently in use.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param filename
    The name of the physical file in which the LSNs are to be cleared.
    @param encrypted
    Whether the file contains encrypted databases.
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void resetFileID(final String filename, boolean encrypted)
        throws DatabaseException {

        dbenv.fileid_reset(filename, encrypted ? DbConstants.DB_ENCRYPT : 0);
    }

    /**
    Allows database files to be moved from one transactional database
    environment to another.
    <p>
    Database pages in transactional database environments contain references
    to the environment's log files (that is, log sequence numbers, or LSNs).
    Copying or moving a database file from one database environment to
    another, and then modifying it, can result in data corruption if the
    LSNs are not first cleared.
    <p>
    Note that LSNs should be reset before moving or copying the database
    file into a new database environment, rather than moving or copying the
    database file and then resetting the LSNs.  Berkeley DB has consistency checks
    that may be triggered if an application calls this method
    on a database in a new environment when the database LSNs still reflect
    the old environment.
    <p>
    This method modifies the physical file, in-place.
    Applications should not reset LSNs in files that are currently in use.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param filename
    The name of the physical file in which the LSNs are to be cleared.
    @param encrypted
    Whether the file contains encrypted databases.
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void resetLogSequenceNumber(final String filename, boolean encrypted)
        throws DatabaseException {

        dbenv.lsn_reset(filename, encrypted ? DbConstants.DB_ENCRYPT : 0);
    }

    /* Panic the environment, or stop a panic. */
    /**
    Set the panic state for the database environment.
    Database environments in a panic state normally refuse all attempts to
    call library functions, throwing a 
    {@link com.sleepycat.db.RunRecoveryException RunRecoveryException}.
    <p>
    This method configures a database environment, including all threads of
    control accessing the database environment, not only the operations
    performed using a specified 
    {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param onoff
    If true, set the panic state for the database environment.
    */
    public void panic(boolean onoff)
        throws DatabaseException {

        dbenv.set_flags(DbConstants.DB_PANIC_ENVIRONMENT, onoff);
    }

    /**
     Perform a hot back up of the open environment. 
     <p>
     All files used by the environment are backed up, so long as the normal
     rules for file placement are followed. For information on how files
     are normally placed relative to the environment directory, see the
     <a href="{@docRoot}/../programmer_reference/env_naming.html" target="_top">
         Berkeley DB File Naming
     </a> section in the <i>Berkeley DB Reference Guide</i>.
     <p>
     By default, data directories and the log directory specified relative
     to the home directory will be recreated relative to the target
     directory. If absolute path names are used, then 
     use the {@link com.sleepycat.db.BackupOptions#setSingleDir BackupOptions.setSingleDir}
     method.
     <p>
     This method provides the same functionality as the 
     <a href="{@docRoot}/../api_reference/C/db_hotbackup.html" target="_top">db_hotbackup</a> 
     utility.  However, this method does not perform the housekeeping
     actions performed by that utility. In particular, you may
     want to run a checkpoint before calling this method. To run a
     checkpoint, use the 
     {@link com.sleepycat.db.Environment#checkpoint Environment.checkpoint}
     method. For more information on checkpoints, see the
     <a href="{@docRoot}/../programmer_reference/transapp_checkpoint.html" target="_top">Checkpoint</a> 
     section in the Berkeley DB Reference Guide.
     <p>
     To back up a single database file within the environment, use the
    {@link com.sleepycat.db.Environment#backupDatabase Environment.backupDatabase}
    method.
    <p>
    In addition to the configuration options available using the
    {@link com.sleepycat.db.BackupOptions BackupOptions}
    class, additional tuning modifications can be made using the
    {@link com.sleepycat.db.EnvironmentConfig#setBackupReadCount EnvironmentConfig.setBackupReadCount},
    {@link com.sleepycat.db.EnvironmentConfig#setBackupReadSleep EnvironmentConfig.setBackupReadSleep},
    {@link com.sleepycat.db.EnvironmentConfig#setBackupSize EnvironmentConfig.setBackupSize}, and
    {@link com.sleepycat.db.EnvironmentConfig#setBackupWriteDirect EnvironmentConfig.setBackupWriteDirect}
    methods. Alternatively, you can write your own custom hot back up facility using the 
    {@link com.sleepycat.db.BackupHandler BackupHandler} interface.
    <p>
    This method may only be called after the environment has been opened.
    <p>
    @param target
    Identifies the directory in which the back up will be placed. Any
    subdirectories required to contain the back up must be placed relative
    to this directory. Note that if a
    {@link com.sleepycat.db.BackupHandler BackupHandler} is configured for the
    environment, then the
    value specified to this parameter is passed on to the 
    {@link com.sleepycat.db.BackupHandler#open BackupHandler.open}
    method.
    If this parameter is null, then the target must be specified
    to the 
    {@link com.sleepycat.db.BackupHandler#open BackupHandler.open}
    method.
    <p>
    This directory, and any required subdirectories, will be created for
    you if you specify true for the
    {@link com.sleepycat.db.BackupOptions#setAllowCreate BackupOptions.setAllowCreate}
    method.  Otherwise, if the target does not exist, this method 
    throws a 
    {@link com.sleepycat.db.DatabaseException DatabaseException} exception.
    <p>
    @param opt
    The {@link com.sleepycat.db.BackupOptions BackupOptions} instance used to
    configure the hot back up.
    <p>
    @throws DatabaseException if a failure occurs at any point during the back up.
    */
    public void backup(String target, BackupOptions opt) 
        throws DatabaseException {
        dbenv.backup(target, opt.getFlags());
    }

    /**
     Perform a hot back up of a single database file contained within the environment.
     <p>
     To back up the entire environment, use the
    {@link com.sleepycat.db.Environment#backup Environment.backup}
    method.
    <p>
    You can make some tuning modifications to the backup process using the
    {@link com.sleepycat.db.EnvironmentConfig#setBackupReadCount EnvironmentConfig.setBackupReadCount},
    {@link com.sleepycat.db.EnvironmentConfig#setBackupReadSleep EnvironmentConfig.setBackupReadSleep},
    {@link com.sleepycat.db.EnvironmentConfig#setBackupSize EnvironmentConfig.setBackupSize}, and
    {@link com.sleepycat.db.EnvironmentConfig#setBackupWriteDirect EnvironmentConfig.setBackupWriteDirect}
    methods. Alternatively, you can write your own custom hot back up facility using the 
    {@link com.sleepycat.db.BackupHandler BackupHandler} interface.
    <p>
    This method may only be called after the environment has been opened.
    <p>
    @param dbfile
    The database file that you want to back up.
    <p>
    @param target
    Identifies the directory in which the back up will be placed. 
    The target must exist; otherwise this method throws a
    {@link com.sleepycat.db.DatabaseException DatabaseException} exception.
    Note that if a
    {@link com.sleepycat.db.BackupHandler BackupHandler} is configured for the
    environment, then the
    value specified to this parameter is passed on to the 
    {@link com.sleepycat.db.BackupHandler#open BackupHandler.open}
    method.
    If this parameter is null, then the target must be specified
    to the 
    {@link com.sleepycat.db.BackupHandler#open BackupHandler.open}
    method.
    <p>
    @param exclusiveCreate
    If true, then if the target file exists, this method throws a
    {@link com.sleepycat.db.DatabaseException DatabaseException} exception.
    <p>
    @throws DatabaseException if a failure occurs at any point during the back up.
    */
    public void backupDatabase(String dbfile, String target, boolean exclusiveCreate)
        throws DatabaseException {
        int flags = 0;
        flags |= exclusiveCreate ? DbConstants.DB_EXCL : 0;

        dbenv.dbbackup(dbfile, target, flags);
    }

    /* Version information */
    /**
Return the release version information, suitable for display.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The release version information, suitable for display.
    */
    public static String getVersionString() {
        return DbEnv.get_version_string();
    }

    /**
Return the full release version information, suitable for display.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The full release version information, suitable for display.
    */
    public static String getVersionFullString() {
        return DbEnv.get_version_full_string();
    }

    /**
Return the release's Oracle family number.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The release's Oracle family number.
    */
    public static int getVersionFamily() {
        return DbEnv.get_version_family();
    }

    /**
Return the release's Oracle release number.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The release's Oracle release number.
    */
    public static int getVersionRelease() {
        return DbEnv.get_version_release();
    }

    /**
Return the release major number.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The release major number.
    */
    public static int getVersionMajor() {
        return DbEnv.get_version_major();
    }

    /**
Ensure that all modified pages in the cache are flushed to their backing files.
<p>
Pages in the cache that cannot be immediately written back to disk (for example
pages that are currently in use by another thread of control) are waited for
and written to disk as soon as it is possible to do so.
<p>
@param logSequenceNumber
The purpose of the logSequenceNumber parameter is to enable a transaction
manager to ensure, as part of a checkpoint, that all pages modified by a
certain time have been written to disk.
<p>
All modified pages with a log sequence number less than the logSequenceNumber
parameter are written to disk. If logSequenceNumber is null, all modified
pages in the cache are written to disk.
    */
    public void syncCache(LogSequenceNumber logSequenceNumber) 
        throws DatabaseException {
        dbenv.memp_sync(logSequenceNumber);
    }

    /**
Return whether the transaction referred to by the commit token "token" has
been applied at the local replication environment.
<p>
This method may not be called before the database environment is opened.
<p>
@param maxwait
The maximum time to wait for the transaction to arrive by replication,
expressed in microseconds.  To check the status of the transaction
without waiting, the timeout may be specified as 0.
<p>
@return
TransactionStatus indicating the status of the applicaton of the transaction.
    */
    public TransactionStatus isTransactionApplied(byte[] token, int maxwait)
        throws DatabaseException {

        if (token == null) {
            throw new NullPointerException("token is null");
        }
        if (token.length != DbConstants.DB_TXN_TOKEN_SIZE) {
            throw new IllegalArgumentException("token must be 20 bytes");
        }

        return TransactionStatus.fromInt(dbenv.txn_applied(token, maxwait, 0));
    }

    /**
Return the release minor number.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The release minor number.
    */
    public static int getVersionMinor() {
        return DbEnv.get_version_minor();
    }

    /**
Return the release patch number.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The release patch number.
    */
    public static int getVersionPatch() {
        return DbEnv.get_version_patch();
    }

    /**
    A special identifier for eid in 
    {@link com.sleepycat.db.Environment#openChannel Environment.openChannel}
    to create channel to send messages only to the master site.
    */
    public static final int EID_MASTER = DbConstants.DB_EID_MASTER;

    /**
    Set the blob threshold size.
    <p>
    @param value
    The blob threshold size.
    <p>
    Any data item that is equal to or larger in size than the
    threshold value will automatically be stored as a blob file.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public void setBlobThreshold(int value) throws DatabaseException {
        dbenv.set_blob_threshold(value, 0);
    }
}
