/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;
import com.sleepycat.db.internal.DbSite;

/**
Specifies the attributes of an environment.
<p>
To change the default settings for a database environment, an application
creates a configuration object, customizes settings and uses it for
environment construction. The set methods of this class validate the
configuration values when the method is invoked.  An
IllegalArgumentException is thrown if the value is not valid for that
attribute.
<p>
All commonly used environment attributes have convenience setter/getter
methods defined in this class.  For example, to change the default
transaction timeout setting for an environment, the application should
do the following:
<p>
<blockquote><pre>
    // customize an environment configuration
    EnvironmentConfig envConfig = new EnvironmentConfig();
    envConfig.setTxnTimeout(10000);  // will throw if timeout value is invalid
    // Open the environment.
    Environment myEnvironment = new Environment(home, envConfig);
</pre></blockquote>
Environment configuration follows this order of precedence:
<ol>
<li>Configuration parameters specified in &lt;environment
home&gt;/je.properties take first precedence.
<li>Configuration parameters set in the EnvironmentConfig object used
at Environment construction are next.
<li>Any configuration parameters not set by the application are set to
system defaults.
</ol>
<p>
An EnvironmentConfig can be used to specify both mutable and immutable
environment properties.  Immutable properties may be specified when the
first Environment handle (instance) is opened for a given physical
environment.  When more handles are opened for the same environment, the
following rules apply:
<p>
<ol>
<li>Immutable properties must equal the original values specified when
constructing an Environment handle for an already open environment.  When a
mismatch occurs, an exception is thrown.
<li>Mutable properties are ignored when constructing an Environment
handle for an already open environment.
</ol>
<p>
After an Environment has been constructed, its mutable properties may
be changed using
{@link Environment#setConfig}.
<p>
*/
public class EnvironmentConfig implements Cloneable {
    /*
     * For internal use, to allow null as a valid value for
     * the config parameter.
     */
    public static final EnvironmentConfig DEFAULT = new EnvironmentConfig();

    /* package */
    static EnvironmentConfig checkNull(EnvironmentConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    /* Parameters */
    private int mode = 0644;
    private int backup_read_count = 0;
    private int backup_read_sleep = 0;
    private int backup_size = 0;
    private boolean backup_write_direct = false;
    private int write_direct = 0;
    private java.io.File blobDir = null;
    private int blobThreshold = 0;
    private int cacheCount = 0;
    private long cacheSize = 0L;
    private long cacheMax = 0L;
    private java.io.File createDir = null;
    private java.util.Vector dataDirs = new java.util.Vector();
    private int envid = 0;
    private String errorPrefix = null;
    private java.io.OutputStream errorStream = null;
    private java.io.OutputStream messageStream = null;
    private byte[][] lockConflicts = null;
    private LockDetectMode lockDetectMode = LockDetectMode.NONE;
    private int initMutexes = 0;
    private int lockTableSize = 0;
    private int maxLocks = 0;
    private int maxLockers = 0;
    private int maxLockObjects = 0;
    private int maxLogFileSize = 0;
    private int logBufferSize = 0;
    private java.io.File logDirectory = null;
    private int logFileMode = 0;
    private int logRegionSize = 0;
    private int maxMutexes = 0;
    private int maxOpenFiles = 0;
    private int maxWrite = 0;
    private long maxWriteSleep = 0L;
    private java.io.File metadataDir = null;
    private int mutexAlignment = 0;
    private int mutexIncrement = 0;
    private int mutexTestAndSetSpins = 0;
    private long mmapSize = 0L;
    private int mpPageSize = 0;
    private int mpTableSize = 0;
    private int partitionLocks = 0;
    private String password = null;
    private long regionMemoryMax = 0L;
    private int replicationClockskewFast = 0;
    private int replicationClockskewSlow = 0;
    private long replicationLimit = 0L;
    private int replicationNumSites = 0;
    private int replicationPriority = DbConstants.DB_REP_DEFAULT_PRIORITY;
    private int replicationRequestMin = 0;
    private int replicationRequestMax = 0;
    private long segmentId = 0L;
    private long lockTimeout = 0L;
    private int txnMaxActive = 0;
    private long txnTimeout = 0L;
    private java.util.Date txnTimestamp = null;
    private java.io.File temporaryDirectory = null;
    private ReplicationManagerAckPolicy repmgrAckPolicy =
        ReplicationManagerAckPolicy.QUORUM;
    private java.util.Vector repmgrSitesConfig = new java.util.Vector();

    /* Initial region resource allocation. */
    private int initResourceLocks = 0;
    private int initResourceLockObjects = 0;
    private int initResourceLockers = 0;
    private int initResourceLogIds = 0;
    private int initResourceTransactions = 0;
    private int initResourceThreads = 0;

    /* Open flags */
    private boolean allowCreate = false;
    private boolean initializeCache = false;
    private boolean initializeCDB = false;
    private boolean initializeLocking = false;
    private boolean initializeLogging = false;
    private boolean initializeReplication = false;
    private boolean joinEnvironment = false;
    private boolean lockDown = false;
    private boolean isPrivate = false;
    private boolean register = false;
    private boolean runRecovery = false;
    private boolean runFatalRecovery = false;
    private boolean systemMemory = false;
    private boolean threaded = true;  // Handles are threaded by default in Java
    private boolean transactional = false;
    private boolean useEnvironment = false;
    private boolean useEnvironmentRoot = false;

    /* Flags */
    private boolean cdbLockAllDatabases = false;
    private boolean directDatabaseIO = false;
    private boolean directLogIO = false;
    private boolean dsyncDatabases = false;
    private boolean dsyncLog = false;
    private boolean hotbackupInProgress = false;
    private boolean initializeRegions = false;
    private boolean logAutoRemove = false;
    private boolean logBlobContent = false;
    private boolean logInMemory = false;
    private boolean logZero = false;
    private boolean multiversion = false;
    private boolean noLocking = false;
    private boolean noMMap = false;
    private boolean noPanic = false;
    private boolean overwrite = false;
    private boolean replicationInMemory = false;
    private boolean txnNoSync = false;
    private boolean txnNoWait = false;
    private boolean txnNotDurable = false;
    private boolean txnSnapshot = false;
    private boolean txnWriteNoSync = false;
    private boolean yieldCPU = false;

    /* Verbose Flags */
    private boolean verboseBackup = false;
    private boolean verboseDeadlock = false;
    private boolean verboseFileops = false;
    private boolean verboseFileopsAll = false;
    private boolean verboseRecovery = false;
    private boolean verboseRegister = false;
    private boolean verboseReplication = false;
    private boolean verboseReplicationElection = false;
    private boolean verboseReplicationLease = false;
    private boolean verboseReplicationMisc = false;
    private boolean verboseReplicationMsgs = false;
    private boolean verboseReplicationSync = false;
    private boolean verboseReplicationSystem = true;
    private boolean verboseReplicationTest = false;
    private boolean verboseRepmgrConnfail = false;
    private boolean verboseRepmgrMisc = false;
    private boolean verboseWaitsFor = false;

    /* Callbacks */
    private BackupHandler backupHandler = null;
    private ErrorHandler errorHandler = null;
    private FeedbackHandler feedbackHandler = null;
    private LogRecordHandler logRecordHandler = null;
    private EventHandler eventHandler = null;
    private MessageHandler messageHandler = null;
    private PanicHandler panicHandler = null;
    private boolean repViewIsSet = false;
    private ReplicationViewHandler replicationViewHandler = null;
    private ReplicationTransport replicationTransport = null;

    /**
    Create an EnvironmentConfig initialized with the system default settings.
    */
    public EnvironmentConfig() {
    }

    /**
    Configure the database environment to create any underlying files,
    as necessary.
    <p>
    @param allowCreate
    If true, configure the database environment to create any underlying
    files, as necessary.
    */
    public void setAllowCreate(final boolean allowCreate) {
        this.allowCreate = allowCreate;
    }

    /**
Return true if the database environment is configured to create any
    underlying files, as necessary.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured to create any
    underlying files, as necessary.
    */
    public boolean getAllowCreate() {
        return allowCreate;
    }

    /**
    Sets the path of a directory where blobs are stored.
    <p>
    The blobs of each {@link com.sleepycat.db.Database Database} opened
    within this {@link com.sleepycat.db.Environment Environment} are
    stored under this directory.
    <p>
    This path can not be set after opening the environment.
    <p>
    @param dir
    The path of a directory where blobs are stored.
    */
    public void setBlobDir(java.io.File dir) {
        this.blobDir = dir;
    }

    /**
    Returns the path of a directory where blobs are stored.
    <p>
    The blobs of each {@link com.sleepycat.db.Database Database} opened
    within this {@link com.sleepycat.db.Environment Environment} are
    stored under this directory.
    <p>
    @return
    The path of a directory where blobs are stored.
    */
    public java.io.File getBlobDir() {
        return blobDir;
    }

    /**
    Set the default blob threshold for databases opened in this environment.
    The blob threshold is the size in bytes which is used to determine when
    a data item will be stored as a blob.
    <p>
    Any data item that is equal to or larger in size than the
    threshold value will automatically be stored as a blob.
    <p>
    It is illegal to enable blob in the environment if any of
    {@link com.sleepycat.db.EnvironmentConfig#setTxnSnapshot EnvironmentConfig.setTxnSnapshot},
    {@link com.sleepycat.db.EnvironmentConfig#setInitializeReplication EnvironmentConfig.setInitializeReplication},
    and {@link com.sleepycat.db.EnvironmentConfig#setMultiversion EnvironmentConfig.setMultiversion}
    is called with true value.
    <p>
    This threshold value can be set any time before and after opening the
    environment.
    <p>
    @param value
    The size in bytes which is used to determine when a data item will
    be stored as a blob. If 0, databases opened in the environment will default
    to never using blob.
    */
    public void setBlobThreshold(int value) {
        this.blobThreshold = value;
    }

    /**
    Return the environment wide default blob threshold value. The blob
    threshold is the size in bytes which is used to determine when a data item
    will be stored as a blob.
    <p>
    @return
    The blob threshold value in bytes beyond which data items are
    stored as blobs. If 0, databases opened in the environment will default to
    never using blobs.
    */
    public int getBlobThreshold() {
        return blobThreshold;
    }

    /**
    Set the size of the shared memory buffer pool, that is, the size of the
cache.
<p>
The cache should be the size of the normal working data set of the
application, with some small amount of additional memory for unusual
situations.  (Note: the working set is not the same as the number of
pages accessed simultaneously, and is usually much larger.)
<p>
The default cache size is 256KB, and may not be specified as less than
20KB.  Any cache size less than 500MB is automatically increased by 25%
to account for buffer pool overhead; cache sizes larger than 500MB are
used as specified.  The current maximum size of a single cache is 4GB.
(All sizes are in powers-of-two, that is, 256KB is 2^18 not 256,000.)
<p>
The database environment's cache size may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_cachesize", one or more whitespace characters, and the cache size specified in three parts: the gigabytes of cache, the
additional bytes of cache, and the number of caches, also separated by
whitespace characters.  For example, "set_cachesize 2 524288000 3" would
create a 2.5GB logical cache, split between three physical caches.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
<p>
This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
<p>
This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
<p>
This method may be called at any time during the life of the application.
<p>
@param cacheSize
The size of the shared memory buffer pool, that is, the size of the
cache.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setCacheSize(final long cacheSize) {
        this.cacheSize = cacheSize;
    }

    /**
Return the size of the shared memory buffer pool, that is, the cache.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The size of the shared memory buffer pool, that is, the cache.
    */
    public long getCacheSize() {
        return cacheSize;
    }

    /**
    Set the maximum cache size in bytes. The specified size is rounded to the
    nearest multiple of the cache region size, which is the initial cache size
    divded by the number of regions specified to {@link #setCacheCount}. If no
    value is specified, it defaults to the initial cache size.
    */
    public void setCacheMax(final long cacheMax) {
        this.cacheMax = cacheMax;
    }

    /**
Return the maximum size of the cache.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The maximum size of the cache.
    */
    public long getCacheMax() {
        return cacheMax;
    }

    /**
    Set the number of shared memory buffer pools, that is, the number of
caches.
<p>
It is possible to specify caches larger than 4GB and/or large enough
they cannot be allocated contiguously on some architectures.  For
example, some releases of Solaris limit the amount of memory that may
be allocated contiguously by a process.  This method allows applications
to break the cache broken up into a number of  equally sized, separate
pieces of memory.
<p>
<p>
The database environment's cache size may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_cachesize", one or more whitespace characters, and the cache size specified in three parts: the gigabytes of cache, the
additional bytes of cache, and the number of caches, also separated by
whitespace characters.  For example, "set_cachesize 2 524288000 3" would
create a 2.5GB logical cache, split between three physical caches.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
<p>
This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
<p>
This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
<p>
This method may be called at any time during the life of the application.
<p>
@param cacheCount
The number of shared memory buffer pools, that is, the number of caches.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setCacheCount(final int cacheCount) {
        this.cacheCount = cacheCount;
    }

    /**
Return the number of shared memory buffer pools, that is, the number
    of cache regions.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The number of shared memory buffer pools, that is, the number
    of cache regions.
    */
    public int getCacheCount() {
        return cacheCount;
    }

    /**
    Configure Concurrent Data Store applications to perform locking on
    an environment-wide basis rather than on a per-database basis.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may not be called after the
environment has been opened.
    <p>
    @param cdbLockAllDatabases
    If true, configure Concurrent Data Store applications to perform
    locking on an environment-wide basis rather than on a per-database
    basis.
    */
    public void setCDBLockAllDatabases(final boolean cdbLockAllDatabases) {
        this.cdbLockAllDatabases = cdbLockAllDatabases;
    }

    /**
Return true if the Concurrent Data Store applications are configured to
    perform locking on an environment-wide basis rather than on a
    per-database basis.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the Concurrent Data Store applications are configured to
    perform locking on an environment-wide basis rather than on a
    per-database basis.
    */
    public boolean getCDBLockAllDatabases() {
        return cdbLockAllDatabases;
    }

    /**
    Sets the path of a directory to be used as the location to create the
access method database files. When the open function is used to create a file
it will be created relative to this path.
    */
    public void setCreateDir(java.io.File dir) {
        createDir = dir;
    }

    /**
    Returns the path of a directory to be used as the location to create the
access method database files.
@return
The path of a directory to be used as the location to create the access method 
database files.
    */
    public java.io.File getCreateDir() {
        return createDir;
    }

    /**
    Set the path of a directory to be used as the location of the access
    method database files.
    <p>
    Paths specified to {@link com.sleepycat.db.Environment#openDatabase Environment.openDatabase} and
    {@link com.sleepycat.db.Environment#openSecondaryDatabase Environment.openSecondaryDatabase} will be searched
    relative to this path.  Paths set using this method are additive, and
    specifying more than one will result in each specified directory
    being searched for database files.  If any directories are
    specified, created database files will always be created in the
    first path specified.
    <p>
    If no database directories are specified, database files must be named
    either by absolute paths or relative to the environment home directory.
    <p>
    The database environment's data directories may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_data_dir", one or more whitespace characters, and the directory name.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, the
information specified to this method must be consistent with the
existing environment or corruption can occur.
    <p>
    @param dataDir
    A directory to be used as a location for database files.
    On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
    */
    public void addDataDir(final java.io.File dataDir) {
        this.dataDirs.add(dataDir);
    }

    /** @deprecated replaced by {@link #addDataDir(java.io.File)} */
    public void addDataDir(final String dataDir) {
        this.addDataDir(new java.io.File(dataDir));
    }

    /**
Return the array of data directories.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The array of data directories.
    */
    public java.io.File[] getDataDirs() {
        final java.io.File[] dirs = new java.io.File[dataDirs.size()];
        dataDirs.copyInto(dirs);
        return dirs;
    }

    /**
    Configure the database environment to not buffer database files.
    <p>
    This is intended to avoid to avoid double caching.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param directDatabaseIO
    If true, configure the database environment to not buffer database files.
    */
    public void setDirectDatabaseIO(final boolean directDatabaseIO) {
        this.directDatabaseIO = directDatabaseIO;
    }

    /**
Return true if the database environment has been configured to not buffer
    database files.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment has been configured to not buffer
    database files.
    */
    public boolean getDirectDatabaseIO() {
        return directDatabaseIO;
    }

    /**
    Configure the database environment to not buffer log files.
    <p>
    This is intended to avoid to avoid double caching.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param directLogIO
    If true, configure the database environment to not buffer log files.
    */
    public void setDirectLogIO(final boolean directLogIO) {
        this.directLogIO = directLogIO;
    }

    /**
Return true if the database environment has been configured to not buffer
    log files.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment has been configured to not buffer
    log files.
    */
    public boolean getDirectLogIO() {
        return directLogIO;
    }

    /**
    Configure the database environment to flush database writes to the backing
    disk before returning from the write system call, rather than flushing
    database writes explicitly in a separate system call, as necessary.
    <p>
    This is only available on some systems (for example, systems supporting the
    m4_posix1_name standard O_DSYNC flag, or systems supporting the Win32
    FILE_FLAG_WRITE_THROUGH flag).  This flag may result in inaccurate file
    modification times and other file-level information for Berkeley DB database
    files.  This flag will almost certainly result in a performance decrease on
    most systems.  This flag is only applicable to certain filesysystem (for
    example, the Veritas VxFS filesystem), where the filesystem's support for
    trickling writes back to stable storage behaves badly (or more likely, has
    been misconfigured).
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param dsyncDatabases
    If true, configure the database environment to flush database writes to the
    backing disk before returning from the write system call, rather than
    flushing log writes explicitly in a separate system call.
    */
    public void setDsyncDatabases(final boolean dsyncDatabases) {
        this.dsyncDatabases = dsyncDatabases;
    }

    /**
Return true if the database environment has been configured to flush database
    writes to the backing disk before returning from the write system call.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment has been configured to flush database
    writes to the backing disk before returning from the write system call.
    */
    public boolean getDsyncDatabases() {
        return dsyncDatabases;
    }

    /**
    Configure the database environment to flush log writes to the
    backing disk before returning from the write system call, rather
    than flushing log writes explicitly in a separate system call.
    <p>
    This configuration is only available on some systems (for example,
    systems supporting the POSIX standard O_DSYNC flag, or systems
    supporting the Win32 FILE_FLAG_WRITE_THROUGH flag).  This
    configuration may result in inaccurate file modification times and
    other file-level information for Berkeley DB log files.  This
    configuration may offer a performance increase on some systems and
    a performance decrease on others.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param dsyncLog
    If true, configure the database environment to flush log writes to
    the backing disk before returning from the write system call, rather
    than flushing log writes explicitly in a separate system call.
    */
    public void setDsyncLog(final boolean dsyncLog) {
        this.dsyncLog = dsyncLog;
    }

    /**
Return true if the database environment has been configured to flush log
    writes to the backing disk before returning from the write system
    call.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment has been configured to flush log
    writes to the backing disk before returning from the write system
    call.
    */
    public boolean getDsyncLog() {
        return dsyncLog;
    }

    /**
    Set the password used to perform encryption and decryption.
    <p>
    Berkeley DB uses the Rijndael/AES (also known as the Advanced
    Encryption Standard and Federal Information Processing
    Standard (FIPS) 197) algorithm for encryption or decryption.
    */
    public void setEncrypted(final String password) {
        this.password = password;
    }

    /**
Return the database environment has been configured to perform
    encryption.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The database environment has been configured to perform
    encryption.
    */
    public boolean getEncrypted() {
        return (password != null);
    }

    /**
    Set the function to be called if an error occurs.
<p>
When an error occurs in the Berkeley DB library, an exception is thrown.
In some cases, however, the error information returned to the
application may be insufficient to completely describe the cause of the
error, especially during initial application debugging.
<p>
The {@link com.sleepycat.db.EnvironmentConfig#setErrorHandler EnvironmentConfig.setErrorHandler} and {@link com.sleepycat.db.DatabaseConfig#setErrorHandler DatabaseConfig.setErrorHandler} methods are used to enhance the mechanism for reporting
error messages to the application.  In some cases, when an error occurs,
Berkeley DB will invoke the ErrorHandler's object error method.  It is
up to this method to display the error message in an appropriate manner.
<p>
Alternatively, applications can use {@link com.sleepycat.db.EnvironmentConfig#setErrorStream EnvironmentConfig.setErrorStream} and {@link com.sleepycat.db.DatabaseConfig#setErrorStream DatabaseConfig.setErrorStream} to
display the additional information via an output stream.  Applications
should not mix these approaches.
<p>
This error-logging enhancement does not slow performance or significantly
increase application size, and may be run during normal operation as well
as during application debugging.
<p>
This method may be called at any time during the life of the application.
<p>
@param errorHandler
The function to be called if an error occurs.
    */
    public void setErrorHandler(final ErrorHandler errorHandler) {
        this.errorHandler = errorHandler;
    }

    /**
Return the function to be called if an error occurs.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The function to be called if an error occurs.
    */
    public ErrorHandler getErrorHandler() {
        return errorHandler;
    }

    /**
    Set the prefix string that appears before error messages.
<p>
This method may be called at any time during the life of the application.
<p>
@param errorPrefix
The prefix string that appears before error messages.
    */
    public void setErrorPrefix(final String errorPrefix) {
        this.errorPrefix = errorPrefix;
    }

    /**
Return the prefix string that appears before error messages.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The prefix string that appears before error messages.
    */
    public String getErrorPrefix() {
        return errorPrefix;
    }

    /**
    Set an OutputStream for displaying error messages.
<p>
When an error occurs in the Berkeley DB library, an exception is thrown.
In some cases, however, the error information returned to the
application may be insufficient to completely describe the cause of the
error, especially during initial application debugging.
<p>
The {@link com.sleepycat.db.EnvironmentConfig#setErrorStream EnvironmentConfig.setErrorStream} and
{@link com.sleepycat.db.DatabaseConfig#setErrorStream DatabaseConfig.setErrorStream} methods are used to enhance
the mechanism for reporting error messages to the application by setting
a OutputStream to be used for displaying additional Berkeley DB error
messages.  In some cases, when an error occurs, Berkeley DB will output
an additional error message to the specified stream.
<p>
The error message will consist of the prefix string and a colon
("<b>:</b>") (if a prefix string was previously specified using
{@link com.sleepycat.db.EnvironmentConfig#setErrorPrefix EnvironmentConfig.setErrorPrefix} or {@link com.sleepycat.db.DatabaseConfig#setErrorPrefix DatabaseConfig.setErrorPrefix}), an error string, and a trailing newline character.
<p>
Setting errorStream to null unconfigures the interface.
<p>
Alternatively, applications can use {@link com.sleepycat.db.EnvironmentConfig#setErrorHandler EnvironmentConfig.setErrorHandler} and {@link com.sleepycat.db.DatabaseConfig#setErrorHandler DatabaseConfig.setErrorHandler} to capture
the additional error information in a way that does not use output
streams.  Applications should not mix these approaches.
<p>
This error-logging enhancement does not slow performance or significantly
increase application size, and may be run during normal operation as well
as during application debugging.
<p>
This method may be called at any time during the life of the application.
<p>
@param errorStream
The application-specified OutputStream for error messages.
    */
    public void setErrorStream(final java.io.OutputStream errorStream) {
        this.errorStream = errorStream;
    }

    /**
Return the OutputStream for displaying error messages.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The OutputStream for displaying error messages.
    */
    public java.io.OutputStream getErrorStream() {
        return errorStream;
    }

    /**
Return the number of pages to read before pausing during the hot backup.
<p>
@return
The number of pages to read before pausing.
    */
    public int getBackupReadCount() {
        return this.backup_read_count;
    }

    /**
Configures the number of pages to read during a hot backup before pausing. 
<p>
Increasing this value increases the amount of I/O the backup process
performs for any given time interval. If your application is already
heavily I/O bound, setting this value to a lower number may help to improve
your overall data throughput by reducing the I/O demands placed on your
system. By default, all pages are read without a pause.
<p>
This method configures the behavior of the
{@link com.sleepycat.db.Environment#backup Environment.backup}
and 
{@link com.sleepycat.db.Environment#backupDatabase Environment.backupDatabase}
methods. It may be called at any time during the life of the application.
<p>
@param count
The number of pages to read before pausing.
    */
    public void setBackupReadCount(int count) {
        this.backup_read_count = count;
    }

    /**
Return the number of microseconds to sleep between batches of reads during
a hot backup.
<p>
@return
The number of microseconds to sleep.
    */
    public int getBackupReadSleep() {
        return this.backup_read_sleep;
    }

    /**
Configures the number of microseconds to sleep between batches of reads during
a hot backup.
<p>
Increasing this value decreases the amount of I/O the backup process
performs for any given time interval. If your application is already
heavily I/O bound, setting this value to a higher number may help to
improve your overall data throughput by reducing the I/O demands placed on
your system. 
<p>
This method configures the behavior of the
{@link com.sleepycat.db.Environment#backup Environment.backup}
and 
{@link com.sleepycat.db.Environment#backupDatabase Environment.backupDatabase}
methods. It may be called at any time during the life of the application.
<p>
@param sleep
The number of microseconds to sleep.
    */
    public void setBackupReadSleep(int sleep) {
        this.backup_read_sleep = sleep;
    }

    /**
Return the size of the buffer, in megabytes, to read from the database
during a hot backup.
<p>
@return
The size of the buffer in megabytes.
    */
    public int getBackupSize() {
        return this.backup_size;
    }

    /**
Configures the size of the buffer, in bytes, to read from the database
during a hot backup.
<p>
This method configures the behavior of the
{@link com.sleepycat.db.Environment#backup Environment.backup}
and 
{@link com.sleepycat.db.Environment#backupDatabase Environment.backupDatabase}
methods. It may be called at any time during the life of the application.
<p>
@param size
The size of the buffer in bytes. Default is 1 megabyte.
    */
    public void setBackupSize(int size) {
        this.backup_size = size;
    }

    /**
Return return true if direct I/O is used when writing pages to the disk
during a hot backup.
<p>
@return
The environment's current direct I/O for hot backup setting.
    */
    public boolean getBackupWriteDirect() {
        return this.backup_write_direct;
    }

    /**
Configures whether direct I/O is used when writing pages to the disk during a
hot backup.
<p>
For some environments, direct I/O can provide faster write throughput, but
usually it is slower because the OS buffer pool offers asynchronous
activity. 
<p>
This method configures the behavior of the
{@link com.sleepycat.db.Environment#backup Environment.backup}
and 
{@link com.sleepycat.db.Environment#backupDatabase Environment.backupDatabase}
methods. It may be called at any time during the life of the application.
<p>
@param writeDirect
If true, use direct I/O when writing database pages; if false, do not use 
direct I/O. 
    */
    public void setBackupWriteDirect(boolean writeDirect) {
        this.backup_write_direct = writeDirect;
    }

    /**
Sets the {@link com.sleepycat.db.BackupHandler BackupHandler}
interface to be used when performing hot backups.
<p>
The {@link com.sleepycat.db.BackupHandler BackupHandler} interface is used
to override the default behavior used by the 
{@link com.sleepycat.db.Environment#backup Environment.backup}
and 
{@link com.sleepycat.db.Environment#backupDatabase Environment.backupDatabase}
methods. 
<p>
This method may be called at any time during the life of the application.
<p>
This method configures operations performed using the Environment handle,
not all operations performed on the underlying database environment. 
<p>
@param backupHandler
The {@link com.sleepycat.db.BackupHandler BackupHandler}
interface to use when performing hot backups.
    */
    public void setBackupHandler(final BackupHandler backupHandler) {
        this.backupHandler = backupHandler;
    }

    /**
Return the {@link com.sleepycat.db.BackupHandler BackupHandler}
used to override default hot backup behavior.
<p>
@return
The {@link com.sleepycat.db.BackupHandler BackupHandler}
interface to use when performing hot backups.
    */
    public BackupHandler getBackupHandler() {
        return backupHandler;
    }

    /**
    Set an object whose methods are to be called when a triggered event occurs.
    <p>
    @param eventHandler
    An object whose methods are called when event callbacks are initiated from
    within Berkeley DB.
    */
    public void setEventHandler(final EventHandler eventHandler) {
        this.eventHandler = eventHandler;
    }

    /**
Return the object's methods to be called when a triggered event occurs.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The object's methods to be called when a triggered event occurs.
    */
    public EventHandler getEventHandler() {
        return eventHandler;
    }

    /**
    Set an object whose methods are called to provide feedback.
<p>
Some operations performed by the Berkeley DB library can take
non-trivial amounts of time.  This method can be used by applications
to monitor progress within these operations.  When an operation is
likely to take a long time, Berkeley DB will call the object's methods
with progress information.
<p>
It is up to the object's methods to display this information in an
appropriate manner.
<p>
This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle
<p>
This method may be called at any time during the life of the application.
<p>
@param feedbackHandler
An object whose methods are called to provide feedback.
    */
    public void setFeedbackHandler(final FeedbackHandler feedbackHandler) {
        this.feedbackHandler = feedbackHandler;
    }

    /**
Return the object's methods to be called to provide feedback.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The object's methods to be called to provide feedback.
    */
    public FeedbackHandler getFeedbackHandler() {
        return feedbackHandler;
    }

    /**
When a "hot backup" copy of a database environment is taken, this
attribute should be configured in the environment prior to copying.
If any transactions with the bulk insert optimization enabled (i.e.,
started with the Bulk configuration attribute) are in progress,
setting the HotBackupInProgress attribute will force a checkpoint in
the environment.  After this attribute is set, the bulk insert
optimization is disabled, until the attribute is reset.  Using this
protocol allows a hot backup procedure to make a consistent copy of
the database even when bulk transactions are ongoing.  Please see the
discussion of hot backups in the <i>Getting Started With Transactions
Guide</i>, and the description of the Bulk attribute in 
{@link com.sleepycat.db.TransactionConfig TransactionConfig}
for more information.
<p>
@param hotbackup
If true, set the HotBackupInProgress attribute; if false, reset it.
    */
    public void setHotbackupInProgress(boolean hotbackup) {
	this.hotbackupInProgress = hotbackup;
    }
    /**
Return true if a "hot backup" is in progress.
<p>
@return
The environment's current HotBackupInProgress setting.
    */
    public boolean getHotbackupInProgress() {
        return this.hotbackupInProgress;
    }

    /**
    Configure a shared memory buffer pool in the database environment.
    <p>
    This subsystem should be used whenever an application is using any
    Berkeley DB access method.
    <p>
    @param initializeCache
    If true, configure a shared memory buffer pool in the database
    environment.
    */
    public void setInitializeCache(final boolean initializeCache) {
        this.initializeCache = initializeCache;
    }

    /**
Return true if the database environment is configured with a shared
    memory buffer pool.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured with a shared
    memory buffer pool.
    */
    public boolean getInitializeCache() {
        return initializeCache;
    }

    /**
    Configure the database environment for the Concurrent Data Store
    product.
    <p>
    In this mode, Berkeley DB provides multiple reader/single writer access.
    The only other subsystem that should be specified for this handle is a
    cache.
    <p>
    @param initializeCDB
    If true, configure the database environment for the Concurrent Data
    Store product.
    */
    public void setInitializeCDB(final boolean initializeCDB) {
        this.initializeCDB = initializeCDB;
    }

    /**
Return true if the database environment is configured for the Concurrent
    Data Store product.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured for the Concurrent
    Data Store product.
    */
    public boolean getInitializeCDB() {
        return initializeCDB;
    }

    /**
    Configure the database environment for locking.
    <p>
    Locking should be used when multiple processes or threads are going
    to be reading and writing a database, so they do not interfere with
    each other.  If all threads are accessing the database(s) read-only,
    locking is unnecessary.  When locking is configured, it is usually
    necessary to run a deadlock detector, as well.
    <p>
    @param initializeLocking
    If true, configure the database environment for locking.
    */
    public void setInitializeLocking(final boolean initializeLocking) {
        this.initializeLocking = initializeLocking;
    }

    /**
Return true if the database environment is configured for locking.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured for locking.
    */
    public boolean getInitializeLocking() {
        return initializeLocking;
    }

    /**
    Configure the database environment for logging.
    <p>
    Logging should be used when recovery from application or system
    failure is necessary.  If the log region is being created and log
    files are already present, the log files are reviewed; subsequent
    log writes are appended to the end of the log, rather than overwriting
    current log entries.
    <p>
    @param initializeLogging
    If true, configure the database environment for logging.
    */
    public void setInitializeLogging(final boolean initializeLogging) {
        this.initializeLogging = initializeLogging;
    }

    /**
Return true if the database environment is configured for logging.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured for logging.
    */
    public boolean getInitializeLogging() {
        return initializeLogging;
    }

    /**
    Configure the database environment to page-fault shared regions into
    memory when initially creating or joining a database environment.
    <p>
    In some applications, the expense of page-faulting the underlying
    shared memory regions can affect performance.  For example, if the
    page-fault occurs while holding a lock, other lock requests can
    convoy, and overall throughput may decrease.  This method
    configures Berkeley DB to page-fault shared regions into memory when
    initially creating or joining a database environment.  In addition,
    Berkeley DB will write the shared regions when creating an
    environment, forcing the underlying virtual memory and filesystems
    to instantiate both the necessary memory and the necessary disk
    space.  This can also avoid out-of-disk space failures later on.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param initializeRegions
    If true, configure the database environment to page-fault shared
    regions into memory when initially creating or joining a database
    environment.
    */
    public void setInitializeRegions(final boolean initializeRegions) {
        this.initializeRegions = initializeRegions;
    }

    /**
Return true if the database environment has been configured to page-fault
    shared regions into memory when initially creating or joining a
    database environment.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment has been configured to page-fault
    shared regions into memory when initially creating or joining a
    database environment.
    */
    public boolean getInitializeRegions() {
        return initializeRegions;
    }

    /**
    Configure the database environment for replication.
    <p>
    Replication requires both locking and transactions.
    <p>
    @param initializeReplication
    If true, configure the database environment for replication.
    */
    public void setInitializeReplication(final boolean initializeReplication) {
        this.initializeReplication = initializeReplication;
    }

    /**
Return true if the database environment is configured for replication.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured for replication.
    */
    public boolean getInitializeReplication() {
        return initializeReplication;
    }

    /**
    Configure the handle to join an existing environment.
    <p>
    This option allows applications to join an existing environment
    without knowing which subsystems the environment supports.
    <p>
    @param joinEnvironment
    If true, configure the handle to join an existing environment.
    */
    public void setJoinEnvironment(final boolean joinEnvironment) {
        this.joinEnvironment = joinEnvironment;
    }

    /**
Return the handle is configured to join an existing environment.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The handle is configured to join an existing environment.
    */
    public boolean getJoinEnvironment() {
        return joinEnvironment;
    }

    /**
    Configure the locking conflicts matrix.
    <p>
    If the locking conflicts matrix is never configured, a standard
    conflicts array is used.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param lockConflicts
    The locking conflicts matrix.  A non-0 value for an array element
    indicates the requested_mode and held_mode conflict:
    <blockquote><pre>
        lockConflicts[requested_mode][held_mode]
    </pre></blockquote>
    <p>
    The <em>not-granted</em> mode must be represented by 0.
    */
    public void setLockConflicts(final byte[][] lockConflicts) {
        this.lockConflicts = lockConflicts;
    }

    /**
Return the locking conflicts matrix.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The locking conflicts matrix.
    */
    public byte[][] getLockConflicts() {
        return lockConflicts;
    }

    /**
    Configure if the deadlock detector is to be run whenever a lock
    conflict occurs.
    <p>
    The database environment's deadlock detector configuration may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_lk_detect", one or more whitespace characters, and the method <code>detect</code> parameter as a string; for example,
    "set_lk_detect DB_LOCK_OLDEST".
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    Although the method may be called at any time during the life of the
application, it should normally be called before opening the database
environment.
    <p>
    @param lockDetectMode
    The lock request(s) to be rejected.  As transactions acquire locks
    on behalf of a single locker ID, rejecting a lock request associated
    with a transaction normally requires the transaction be aborted.
    */
    public void setLockDetectMode(final LockDetectMode lockDetectMode) {
        this.lockDetectMode = lockDetectMode;
    }

    /**
Return true if the deadlock detector is configured to run whenever a lock
    conflict occurs.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the deadlock detector is configured to run whenever a lock
    conflict occurs.
    */
    public LockDetectMode getLockDetectMode() {
        return lockDetectMode;
    }

    /**
    Configure the database environment to lock shared environment files
    and memory-mapped databases into memory.
    <p>
    @param lockDown
    If true, configure the database environment to lock shared
    environment files and memory-mapped databases into memory.
    */
    public void setLockDown(final boolean lockDown) {
        this.lockDown = lockDown;
    }

    /**
Return true if the database environment is configured to lock shared
    environment files and memory-mapped databases into memory.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured to lock shared
    environment files and memory-mapped databases into memory.
    */
    public boolean getLockDown() {
        return lockDown;
    }

    /**
    Set the timeout value for the database environment
locks.
<p>
Lock timeouts are checked whenever a thread of control blocks on a lock
or when deadlock detection is performed.  The lock may have been
requested explicitly through the Lock subsystem interfaces, or it may
be a lock requested by the database access methods underlying the
application.
As timeouts are only checked when the lock request first blocks or when
deadlock detection is performed, the accuracy of the timeout depends on
how often deadlock detection is performed.
<p>
Timeout values specified for the database environment may be overridden
on a
per-lock basis by {@link com.sleepycat.db.Environment#lockVector Environment.lockVector}.
<p>
This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
<p>
This method may be called at any time during the life of the application.
<p>
@param lockTimeout
The timeout value, specified as an unsigned 32-bit number of
microseconds, limiting the maximum timeout to roughly 71 minutes.
<p>
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setLockTimeout(final long lockTimeout) {
        this.lockTimeout = lockTimeout;
    }

    /**
Return the database environment lock timeout value, in microseconds;
    a timeout of 0 means no timeout is set.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The database environment lock timeout value, in microseconds;
    a timeout of 0 means no timeout is set.
    */
    public long getLockTimeout() {
        return lockTimeout;
    }

    /**
    Configure the system to automatically remove log files that are no
    longer needed.
    <p>
    Automatic log file removal is likely to make catastrophic recovery
    impossible.
    <p>
    Replication Manager applications operate in a group-aware manner for log file
    removal, and automatic log file removal simplifies the application.
    <p>
    Replication Base API applications will rarely want to configure automatic log
    file removal as it increases the likelihood a master will be unable to
    satisfy a client's request for a recent log record. 
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param logAutoRemove
    If true, configure the system to automatically remove log files that
    are no longer needed.
    */
    public void setLogAutoRemove(final boolean logAutoRemove) {
        this.logAutoRemove = logAutoRemove;
    }

    /**
Return true if the system has been configured to to automatically remove log
    files that are no longer needed.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the system has been configured to to automatically remove log
    files that are no longer needed.
    */
    public boolean getLogAutoRemove() {
        return logAutoRemove;
    }

    /**
    Enable full logging of blob data.  Required for HA and the hotbackup
    utility.
    <p>
    @param logBlobContent
    If true, enable full logging of blob data.
    */
    public void setLogBlobContent(final boolean logBlobContent) {
        this.logBlobContent = logBlobContent;
    }

    /**
    Return true if full logging of blob data is enabled.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    True if full logging of blob data is enabled.
    */
    public boolean getLogBlobContent() {
        return logBlobContent;
    }

    /**
    If set, maintain transaction logs in memory rather than on disk. This means
    that transactions exhibit the ACI (atomicity, consistency, and isolation)
    properties, but not D (durability); that is, database integrity will be
    maintained, but if the application or system fails, integrity will not
    persist. All database files must be verified and/or restored from a
    replication group master or archival backup after application or system
    failure.
    <p>
    When in-memory logs are configured and no more log buffer space is
    available, Berkeley DB methods will throw a {@link com.sleepycat.db.DatabaseException DatabaseException}.
    When choosing log buffer and file sizes for in-memory logs, applications
    should ensure the in-memory log buffer size is large enough that no
    transaction will ever span the entire buffer, and avoid a state where the
    in-memory buffer is full and no space can be freed because a transaction
    that started in the first log "file" is still active.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param logInMemory
    If true, maintain transaction logs in memory rather than on disk.
    */
    public void setLogInMemory(final boolean logInMemory) {
        this.logInMemory = logInMemory;
    }

    /**
Return true if the database environment is configured to maintain transaction logs
    in memory rather than on disk.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured to maintain transaction logs
    in memory rather than on disk.
    */
    public boolean getLogInMemory() {
        return logInMemory;
    }

    /**
    Set a function to process application-specific log records.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, the
information specified to this method must be consistent with the
existing environment or corruption can occur.
    <p>
    @param logRecordHandler
    The handler for application-specific log records.
    */
    public void setLogRecordHandler(final LogRecordHandler logRecordHandler) {
        this.logRecordHandler = logRecordHandler;
    }

    /**
Return the handler for application-specific log records.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The handler for application-specific log records.
    */
    public LogRecordHandler getLogRecordHandler() {
        return logRecordHandler;
    }

    /** 
    If set, zero all pages of a log file when that log file is created.  This
    has been shown to provide greater transaction throughput in some
    environments.  The log file will be zeroed by the thread which needs to
    re-create the new log file.  Other threads may not write to the log file
    while this is happening.  
    <p>
    This method configures the database environment, including all threads of
    control accessing the database environment.
    <p>
    This method may not be called after the environment has been opened.
    <p>
    @param logZero
    If true, zero all pages of new log files upon their creation.
    */
    public void setLogZero(final boolean logZero) {
        this.logZero = logZero;
    }

    /** 
    Return true if the database environment is configured to zero all pages of
    new log files upon their creation.  
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    True if the database environment is configured to pre-zero log pages.
    */
    public boolean getLogZero() {
        return logZero;
    }

    /**
    Return the inital memory configuration for a region resource.
    <p>
    @param resource
    The resource type to get initial sizing for.
    <p>
    @return
    The initial amount of memory allocated for the requested resource.
    */
    public int getRegionMemoryInitialSize(RegionResourceType resource) {
        switch(resource.getFlag()) {
            case DbConstants.DB_MEM_LOCK:
                return initResourceLocks;
            case DbConstants.DB_MEM_LOCKOBJECT:
                return initResourceLockObjects;
            case DbConstants.DB_MEM_LOCKER:
                return initResourceLockers;
            case DbConstants.DB_MEM_LOGID:
                return initResourceLogIds;
            case DbConstants.DB_MEM_TRANSACTION:
                return initResourceTransactions;
            case DbConstants.DB_MEM_THREAD:
                return initResourceThreads;
        }
        return (0);
    }

    /**
    Assign an initial count for a particular resource within the shared region
    of Berkeley DB. The shared region is used to hold resources that are
    shared amongst processes in Berkeley DB.
    The default values are generally satisfactory.
    <p>
    @param resource
    The resource type to configure.
    @param count
    The initial number of objects of a particular type to allocate.
    */
    public void setRegionMemoryInitialSize(
        RegionResourceType resource, int count) {
        switch(resource.getFlag()) {
            case DbConstants.DB_MEM_LOCK:
                initResourceLocks = count;
                break;
            case DbConstants.DB_MEM_LOCKOBJECT:
                initResourceLockObjects = count;
                break;
            case DbConstants.DB_MEM_LOCKER:
                initResourceLockers = count;
                break;
            case DbConstants.DB_MEM_LOGID:
                initResourceLogIds = count;
                break;
            case DbConstants.DB_MEM_TRANSACTION:
                initResourceTransactions = count;
                break;
            case DbConstants.DB_MEM_THREAD:
                initResourceThreads = count;
                break;
        }
    }

    /**
    Return the maximum amount of memory that can be used by shared structures
    in the main environment region.
    <p>
    @return
    The maximum amount of memory that could be used by the main region.
    */
    public long getRegionMemoryMax() {
        return regionMemoryMax;
    }

    /**
    This method sets the maximum amount of memory to be used by shared
    structures in the main environment region. These are the structures
    other than mutexes and the page cache (memory pool). If the region files
    are in memory mapped files, or if DB_PRIVATE is specified, the memory
    specified by this method is not allocated completely at startup. As
    memory is needed, the shared region will be extended or, in the case of
    DB_PRIVATE, more memory will be allocated using the system malloc call.
    For memory mapped files, a mapped region will be allocated to this size
    but the underlying file will only be allocated sufficient memory to hold
    the initial allocation of shared memory structures as set by
    {@link com.sleepycat.db.EnvironmentConfig#setRegionMemoryInitialSize EnvironmentConfig.setRegionMemoryInitialSize}.
    <p>
    If no memory maximum is specified then it is calculated from defaults,
    initial settings or (deprecated) maximum settings of the various shared
    structures. In the case of environments created with DB_PRIVATE, no
    maximum need be set and the shared structure allocation will grow as
    needed until the process memory limit is exhausted. 
    <p>
    @param regionMemoryMax
    The total amount of memory that can be used by the main region.
    */
    public void setRegionMemoryMax(final long regionMemoryMax) {
        this.regionMemoryMax = regionMemoryMax;
    }

   /** 
    If set, store internal replication information in memory only. By default,
    replication creates files in the environment home directory to preserve
    some internal information. If this configuration flag is turned on, 
    replication only stores this internal information in-memory and cannot 
    keep persistent state across a site crash or reboot.   
    <p>
    This configuration flag can only be turned on before the environment is
    opened. Its value cannot be changed while the environment is open.
    <p>
    @param replicationInMemory
    If true, store internal replication information in memory only. 
    */
    public void setReplicationInMemory(final boolean replicationInMemory) {
        this.replicationInMemory = replicationInMemory;
    }

    /** 
    Return true if internal replication information is stored in memory only.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    True if internal replication information is stored in memory only.
    */
    public boolean getReplicationInMemory() {
        return replicationInMemory;
    }

    /**
    Set the network Ack policy used by the replication manager.
    <p>
    @param repmgrAckPolicy
    The network Ack policy used by the replication manager.
    */
    public void setReplicationManagerAckPolicy(
        final ReplicationManagerAckPolicy repmgrAckPolicy)
    {
        this.repmgrAckPolicy = repmgrAckPolicy;
    }

    /**
    Get the network Ack policy used by the replication manager.
    <p>
    @return
    The network Ack policy used by the replication manager.
    */
    public ReplicationManagerAckPolicy getReplicationManagerAckPolicy()
    {
        return repmgrAckPolicy;
    }

    /** 
    Configure a site in a replication group. This could be called more than once,
    to set local site and remote sites.
    <p>
    @param repmgrSiteConfig
    The configuration of a site site
    */
    public void addReplicationManagerSite(
        final ReplicationManagerSiteConfig repmgrSiteConfig)
    {
        this.repmgrSitesConfig.add(repmgrSiteConfig);
    }

    /**
    Set the number of lock table partitions in the Berkeley DB environment.
    */
    public void setLockPartitions(final int partitions) {
        this.partitionLocks = partitions;
    }

    /**
    Returns the number of lock table partitions in the Berkeley DB environment.
    */
    public int getLockPartitions() {
        return this.partitionLocks;
    }

    /**
    Set the maximum number of locks supported by the database
    environment.
    <p>
    This value is used during environment creation to estimate how much
    space to allocate for various lock-table data structures.  The
    default value is 1000 locks.
    <p>
    The database environment's maximum number of locks may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_lk_max_locks", one or more whitespace characters, and the number of locks.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param maxLocks
    The maximum number of locks supported by the database environment.
    */
    public void setMaxLocks(final int maxLocks) {
        this.maxLocks = maxLocks;
    }

    /**
    Return the maximum number of locks.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    The maximum number of locks.
    */
    public int getMaxLocks() {
        return maxLocks;
    }

    /**
    Set the size for the hash table that contains references to locks. It's
    important to make this large enough so that there is not increased
    contention in the system as it grows.
    <p>
    This method may not be called after the environment has been opened.
    If joining an existing database environment, any information specified
    to this method will be ignored.
    <p>
    @param lockTableSize
    The size of the hash table to allocate.
    */
    public void setLockTableSize(final int lockTableSize) {
        this.lockTableSize = lockTableSize;
    }

    /**
    Return the lock hash table size.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    The lock hash table size.
    */
    public int getLockTableSize() {
        return lockTableSize;
    }

    /**
    Set the maximum number of locking entities supported by the database
    environment.
    <p>
    This value is used during environment creation to estimate how much
    space to allocate for various lock-table data structures.  The default
    value is 1000 lockers.
    <p>
    The database environment's maximum number of lockers may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_lk_max_lockers", one or more whitespace characters, and the number of lockers.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param maxLockers
    The maximum number simultaneous locking entities supported by the
    database environment.
    */
    public void setMaxLockers(final int maxLockers) {
        this.maxLockers = maxLockers;
    }

    /**
Return the maximum number of lockers.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The maximum number of lockers.
    */
    public int getMaxLockers() {
        return maxLockers;
    }

    /**
    Set the maximum number of locked objects supported by the database
    environment.
    <p>
    This value is used during environment creation to estimate how much
    space to allocate for various lock-table data structures.  The default
    value is 1000 objects.
    <p>
    The database environment's maximum number of objects may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_lk_max_objects", one or more whitespace characters, and the number of objects.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param maxLockObjects
    The maximum number of locked objects supported by the database
    environment.
    */
    public void setMaxLockObjects(final int maxLockObjects) {
        this.maxLockObjects = maxLockObjects;
    }

    /**
Return the maximum number of locked objects.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The maximum number of locked objects.
    */
    public int getMaxLockObjects() {
        return maxLockObjects;
    }

    /**
    Set the maximum size of a single file in the log, in bytes.
    <p>
    By default, or if the maxLogFileSize parameter is set to 0, a size
    of 10MB is used.  If no size is specified by the application, the
    size last specified for the database region will be used, or if no
    database region previously existed, the default will be used.
    Because {@link com.sleepycat.db.LogSequenceNumber LogSequenceNumber} file offsets are unsigned four-byte
    values, the set value may not be larger than the maximum unsigned
    four-byte value.
    <p>
    The database environment's log file size may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_lg_max", one or more whitespace characters, and the size in bytes.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param maxLogFileSize
    The maximum size of a single file in the log, in bytes.
    */
    public void setMaxLogFileSize(final int maxLogFileSize) {
        this.maxLogFileSize = maxLogFileSize;
    }

    /**
Return the maximum size of a single file in the log, in bytes.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The maximum size of a single file in the log, in bytes.
    */
    public int getMaxLogFileSize() {
        return maxLogFileSize;
    }

    /**
    Set the size of the in-memory log buffer, in bytes.
    <p>
    Log information is stored in-memory until the storage space fills up
    or transaction commit forces the information to be flushed to stable
    storage.  In the presence of long-running transactions or transactions
    producing large amounts of data, larger buffer sizes can increase
    throughput.
    <p>
    By default, or if the value is set to 0, a size of 32K is used.
    <p>
    The database environment's log buffer size may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_lg_bsize", one or more whitespace characters, and the size in bytes.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param logBufferSize
    The size of the in-memory log buffer, in bytes.
    */
    public void setLogBufferSize(final int logBufferSize) {
        this.logBufferSize = logBufferSize;
    }

    /**
Return the size of the in-memory log buffer, in bytes.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The size of the in-memory log buffer, in bytes.
    */
    public int getLogBufferSize() {
        return logBufferSize;
    }

    /**
    Set the path of a directory to be used as the location of logging files.
    <p>
    Log files created by the Log Manager subsystem will be created in this
    directory.  If no logging directory is specified, log files are
    created in the environment home directory.
    <p>
    For the greatest degree of recoverability from system or application
    failure, database files and log files should be located on separate
    physical devices.
    <p>
    The database environment's logging directory may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_lg_dir", one or more whitespace characters, and the directory name.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, the
information specified to this method must be consistent with the
existing environment or corruption can occur.
    <p>
    @param logDirectory
    The directory used to store the logging files.
    On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
    */
    public void setLogDirectory(final java.io.File logDirectory) {
        this.logDirectory = logDirectory;
    }

    /**
Return the path of a directory to be used as the location of logging files.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The path of a directory to be used as the location of logging files.
    */
    public java.io.File getLogDirectory() {
        return logDirectory;
    }

    /**
    Set the absolute file mode for created log files.  This method is
    <b>only</b> useful for the rare Berkeley DB application that does not
    control its umask value.
    <p>
    Normally, if Berkeley DB applications set their umask appropriately, all
    processes in the application suite will have read permission on the log
    files created by any process in the application suite.  However, if the
    Berkeley DB application is a library, a process using the library might set
    its umask to a value preventing other processes in the application suite
    from reading the log files it creates.  In this rare case, this method
    can be used to set the mode of created log files to an absolute value.
    <p>
    @param logFileMode
    The absolute mode of the created log file.
    */
    public void setLogFileMode(final int logFileMode) {
        this.logFileMode = logFileMode;
    }

    /**
    Return the absolute file mode for created log files.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    The absolute file mode for created log files.
    */
    public int getLogFileMode() {
        return logFileMode;
    }

    /**
    Set the size of the underlying logging area of the database
    environment, in bytes.
    <p>
    By default, or if the value is set to 0, the default size is 60KB.
    The log region is used to store filenames, and so may need to be
    increased in size if a large number of files will be opened and
    registered with the specified database environment's log manager.
    <p>
    The database environment's log region size may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_lg_regionmax", one or more whitespace characters, and the size in bytes.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param logRegionSize
    The size of the logging area in the database environment, in bytes.
    */
    public void setLogRegionSize(final int logRegionSize) {
        this.logRegionSize = logRegionSize;
    }

    /**
Return the size of the underlying logging subsystem region.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The size of the underlying logging subsystem region.
    */
    public int getLogRegionSize() {
        return logRegionSize;
    }

    /**
    Limit the number of file descriptors the library will open concurrently
    when flushing dirty pages from the cache.
    <p>
    @param maxOpenFiles
    The maximum number of file descriptors that may be concurrently opened
    by the library when flushing dirty pages from the cache.
    **/
    public void setMaxOpenFiles(final int maxOpenFiles) {
        this.maxOpenFiles = maxOpenFiles;
    }

    /**
Return the maximum number of file descriptors that will be opened concurrently..
<p>
This method may be called at any time during the life of the application.
<p>
@return
The maximum number of file descriptors that will be opened concurrently..
    */
    public int getMaxOpenFiles() {
        return maxOpenFiles;
    }

    /**
    Limit the number of sequential write operations scheduled by the
    library when flushing dirty pages from the cache.
    <p>
    @param maxWrite
    The maximum number of sequential write operations scheduled by the
    library when flushing dirty pages from the cache.
    @param maxWriteSleep
    The number of microseconds the thread of control should pause before
    scheduling further write operations.
    **/
    public void setMaxWrite(final int maxWrite, final long maxWriteSleep) {
        this.maxWrite = maxWrite;
        this.maxWriteSleep = maxWriteSleep;
    }

    /**
Return the maximum number of sequential write operations.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The maximum number of sequential write operations.
    */
    public int getMaxWrite() {
        return maxWrite;
    }

    /**
Return the microseconds to pause before scheduling further write operations.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The microseconds to pause before scheduling further write operations.
    */
    public long getMaxWriteSleep() {
        return maxWriteSleep;
    }

    /**
    Set a function to be called with an informational message.
<p>
There are interfaces in the Berkeley DB library which either directly
output informational messages or statistical information, or configure
the library to output such messages when performing other operations,
{@link com.sleepycat.db.EnvironmentConfig#setVerboseDeadlock EnvironmentConfig.setVerboseDeadlock} for example.
<p>
The {@link com.sleepycat.db.EnvironmentConfig#setMessageHandler EnvironmentConfig.setMessageHandler} and
{@link com.sleepycat.db.DatabaseConfig#setMessageHandler DatabaseConfig.setMessageHandler} methods are used to display
these messages for the application.
<p>
Setting messageHandler to null unconfigures the interface.
<p>
Alternatively, you can use {@link com.sleepycat.db.EnvironmentConfig#setMessageStream EnvironmentConfig.setMessageStream}
and {@link com.sleepycat.db.DatabaseConfig#setMessageStream DatabaseConfig.setMessageStream} to send the additional
information directly to an output streams.  You should not mix these
approaches.
<p>
This method may be called at any time during the life of the application.
<p>
@param messageHandler
The application-specified function for informational messages.
    */
    public void setMessageHandler(final MessageHandler messageHandler) {
        this.messageHandler = messageHandler;
    }

    /**
Return the function to be called with an informational message.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The function to be called with an informational message.
    */
    public MessageHandler getMessageHandler() {
        return messageHandler;
    }

    /**
    Set an OutputStream for displaying informational messages.
<p>
There are interfaces in the Berkeley DB library which either directly
output informational messages or statistical information, or configure
the library to output such messages when performing other operations,
{@link com.sleepycat.db.EnvironmentConfig#setVerboseDeadlock EnvironmentConfig.setVerboseDeadlock} for example.
<p>
The {@link com.sleepycat.db.EnvironmentConfig#setMessageStream EnvironmentConfig.setMessageStream} and
{@link com.sleepycat.db.DatabaseConfig#setMessageStream DatabaseConfig.setMessageStream} methods are used to display
these messages for the application.  In this case, the message will
include a trailing newline character.
<p>
Setting messageStream to null unconfigures the interface.
<p>
Alternatively, you can use {@link com.sleepycat.db.EnvironmentConfig#setMessageHandler EnvironmentConfig.setMessageHandler}
and {@link com.sleepycat.db.DatabaseConfig#setMessageHandler DatabaseConfig.setMessageHandler} to capture the additional
information in a way that does not use output streams.  You should not
mix these approaches.
<p>
This method may be called at any time during the life of the application.
<p>
@param messageStream
The application-specified OutputStream for informational messages.
    */
    public void setMessageStream(final java.io.OutputStream messageStream) {
        this.messageStream = messageStream;
    }

    /**
Return the OutputStream for displaying informational messages.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The an OutputStream for displaying informational messages.
    */
    public java.io.OutputStream getMessageStream() {
        return messageStream;
    }

    /**
    Set the path of a directory to be used as the location to store the
    persistent metadata files.
    @param dir
    The path of metadata directory.
    */
    public void setMetadataDir(java.io.File dir) {
        metadataDir = dir;
    }

    /**
    Return the path of a directory to be used as the location to store the
    persistent metadata.
    @return
    The the path of metadata directory.
    */
    public java.io.File getMetadataDir() {
        return metadataDir;
    }

    /**
    Set the maximum file size, in bytes, for a file to be mapped into
    the process address space.
    <p>
    If no value is specified, it defaults to 10MB.
    <p>
    Files that are opened read-only in the pool (and that satisfy a few
    other criteria) are, by default, mapped into the process address space
    instead of being copied into the local cache.  This can result in
    better-than-usual performance because available virtual memory is
    normally much larger than the local cache, and page faults are faster
    than page copying on many systems.  However, it can cause resource
    starvation in the presence of limited virtual memory, and it can result
    in immense process sizes in the presence of large databases.
    <p>
    @param mmapSize
    The maximum file size, in bytes, for a file to be mapped into the
    process address space.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may be called at any time during the life of the application.
    */
    public void setMMapSize(final long mmapSize) {
        this.mmapSize = mmapSize;
    }

    /**
    Return the maximum file size, in bytes, for a file to be mapped into
    the process address space.
    <p>
    @return
    The maximum file size, in bytes, for a file to be mapped into the
    process address space.
    */
    public long getMMapSize() {
        return mmapSize;
    }

/**
Sets the page size used to allocate the hash table and the number of mutexes
expected to be needed by the buffer pool.
<p>
This method may be called only before the environment is opened.
<p>
@param mpPageSize
The expected page size to use. Generally, it is set to the expected average
page size for all the data pages that are in the buffer pool. 
 */
    public void setCachePageSize(final int mpPageSize) {
        this.mpPageSize = mpPageSize;
    }
/**
Returns the environment's current page size setting.
<p>
@return
The current setting for the environment's expected page size use.
 */
    public int getCachePageSize() {
        return mpPageSize;
    }

/**
Overrides the calculated hash table size.
<p>
The table size is then adjusted to a nearby prime number to enhance the 
hashing algorithm.
<p>
This method may be called only before the environment is opened.
<p>
@param mpTableSize
Specifies the size of the buffer pool hash table, which is then internally
adjusted to a nearby prime number.
 */
    public void setCacheTableSize(final int mpTableSize) {
        this.mpTableSize = mpTableSize;
    }
/**
Returns the environment's current table size setting.
<p>
@return
The current setting for the environment's table size.
 */
    public int getCacheTableSize() {
        return mpTableSize;
    }

    /**
    Configure the database environment to use a specific mode when
    creating underlying files and shared memory segments.
    <p>
    On UNIX systems or in POSIX environments, files created in the
    database environment are created with the specified mode (as
    modified by the process' umask value at the time of creation).
    <p>
    On UNIX systems or in POSIX environments, system shared memory
    segments created by the library are created with the specified
    mode, unmodified by the process' umask value.
    <p>
    If is 0, the library will use a default mode of readable and
    writable by both owner and group.
    <p>
    Created files are owned by the process owner; the group ownership
    of created files is based on the system and directory defaults,
    and is not further specified by the library.
    <p>
    @param mode
    The mode to use when creating underlying files and shared memory
    segments.
    */
    public void setMode(final int mode) {
        this.mode = mode;
    }

    /**
Return the mode to use when creating underlying files and shared
    memory segments.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The mode to use when creating underlying files and shared
    memory segments.
    */
    public long getMode() {
        return mode;
    }

    /**
    Configure the database environment to open all databases that are not
    using the queue access method for multiversion concurrency control.
    See {@link DatabaseConfig#setMultiversion} for more information.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param multiversion
    If true, all databases that are not using the queue access method will be
    opened for multiversion concurrency control.
    */
    public void setMultiversion(final boolean multiversion) {
        this.multiversion = multiversion;
    }

    /**
Return true if the handle is configured to open all databases for multiversion
    concurrency control.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the handle is configured to open all databases for multiversion
    concurrency control.
    */
    public boolean getMultiversion() {
        return multiversion;
    }

    /**
    Configure the system to grant all requested mutual exclusion mutexes
    and database locks without regard for their actual availability.
    <p>
    This functionality should never be used for purposes other than
    debugging.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param noLocking
    If true, configure the system to grant all requested mutual exclusion
    mutexes and database locks without regard for their actual availability.
    */
    public void setNoLocking(final boolean noLocking) {
        this.noLocking = noLocking;
    }

    /**
Return true if the system has been configured to grant all requested mutual
    exclusion mutexes and database locks without regard for their actual
    availability.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the system has been configured to grant all requested mutual
    exclusion mutexes and database locks without regard for their actual
    availability.
    */
    public boolean getNoLocking() {
        return noLocking;
    }

    /**
    Configure the system to copy read-only database files into the local
    cache instead of potentially mapping them into process memory.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param noMMap
    If true, configure the system to copy read-only database files into
    the local cache instead of potentially mapping them into process memory.
    */
    public void setNoMMap(final boolean noMMap) {
        this.noMMap = noMMap;
    }

    /**
Return true if the system has been configured to copy read-only database files
    into the local cache instead of potentially mapping them into process
    memory.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the system has been configured to copy read-only database files
    into the local cache instead of potentially mapping them into process
    memory.
    */
    public boolean getNoMMap() {
        return noMMap;
    }

    /**
    Configure the system to ignore any panic state in the database
    environment.
    <p>
    Database environments in a panic state normally refuse all attempts to
    call Berkeley DB functions, throwing {@link com.sleepycat.db.RunRecoveryException RunRecoveryException}.
    This functionality should never be used for purposes other than
    debugging.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param noPanic
    If true, configure the system to ignore any panic state in the
    database environment.
    */
    public void setNoPanic(final boolean noPanic) {
        this.noPanic = noPanic;
    }

    /**
Return true if the system has been configured to ignore any panic state in
    the database environment.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the system has been configured to ignore any panic state in
    the database environment.
    */
    public boolean getNoPanic() {
        return noPanic;
    }

    /**
    Configure the system to overwrite files stored in encrypted formats
    before deleting them.
    <p>
    Berkeley DB overwrites files using alternating 0xff, 0x00 and 0xff
    byte patterns.  For file overwriting to be effective, the underlying
    file must be stored on a fixed-block filesystem.  Systems with
    journaling or logging filesystems will require operating system
    support and probably modification of the Berkeley DB sources.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param overwrite
    If true, configure the system to overwrite files stored in encrypted
    formats before deleting them.
    */
    public void setOverwrite(final boolean overwrite) {
        this.overwrite = overwrite;
    }

    /**
Return true if the system has been configured to overwrite files stored in
    encrypted formats before deleting them.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the system has been configured to overwrite files stored in
    encrypted formats before deleting them.
    */
    public boolean getOverwrite() {
        return overwrite;
    }

    /**
    Set the function to be called if the database environment panics.
<p>
Errors can occur in the Berkeley DB library where the only solution is
to shut down the application and run recovery (for example, if Berkeley
DB is unable to allocate heap memory).  In such cases, the Berkeley DB
methods will throw a {@link com.sleepycat.db.RunRecoveryException RunRecoveryException}.  It is often easier
to simply exit the application when such errors occur rather than
gracefully return up the stack.  This method specifies a function to be
called when {@link com.sleepycat.db.RunRecoveryException RunRecoveryException} is about to be thrown from a
Berkeley DB method.
<p>
This method may be called at any time during the life of the application.
<p>
@param panicHandler
The function to be called if the database environment panics.
    */
    public void setPanicHandler(final PanicHandler panicHandler) {
        this.panicHandler = panicHandler;
    }

    /**
Return the function to be called if the database environment panics.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The function to be called if the database environment panics.
    */
    public PanicHandler getPanicHandler() {
        return panicHandler;
    }

    /**
    Set the function to be used by replication views to determine whether a
    database file is replicated to the local site.
    <p>
    @param repViewHandler
    The function name to determine whether a database file is replicated. If
    null, the replication view is a full view and all database files are
    replicated to the local site. Otherwise it is a partial view and only some
    database files are replicated to the local site.
    */
    public void setReplicationView(
        final ReplicationViewHandler repViewHandler) {
        this.repViewIsSet = true;
        this.replicationViewHandler = repViewHandler;
    }

    /**
    Return the function name used by replication views to determine whether
    a database file is replicated to the local site.
    <p>
    @return
    The function name used by replication views to determine whether a database
    file is replicated to the local site. If null, the replication view is a
    full view and all database files are replicated to the local site.
    Otherwise it is a partial view and only some database files are replicated
    to the local site.
    */
    public ReplicationViewHandler getReplicationViewHandler() {
        return this.replicationViewHandler;
    }

    /**
    Configure the database environment to only be accessed by a single
    process (although that process may be multithreaded).
    <p>
    This has two effects on the database environment.  First, all
    underlying data structures are allocated from per-process memory
    instead of from shared memory that is potentially accessible to more
    than a single process.  Second, mutexes are only configured to work
    between threads.
    <p>
    Use of this flag means that multiple handles cannot be simultaneously
    opened for the environment, even within the same process.  For example,
    if both a server application and a Berkeley DB utility are expected to
    access the environment at the same time, then do not configure the
    database environment as private.
    <p>
    @param isPrivate
    If true, configure the database environment to only be accessed by
    a single process.
    */
    public void setPrivate(final boolean isPrivate) {
        this.isPrivate = isPrivate;
    }

    /**
Return true if the database environment is configured to only be accessed
    by a single process.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured to only be accessed
    by a single process.
    */
    public boolean getPrivate() {
        return isPrivate;
    }

    /**
    Sets the clock skew ratio among replication group members based on the
    fastest and slowest measurements among the group for use with master leases.
    Calling this method is optional, the default values for clock skew assume no
    skew.  The user must also configure leases via the 
    {@link Environment#setReplicationConfig} method.  Additionally, the user must
    also set the master lease timeout via the
    {@link Environment#setReplicationTimeout} method and, if using the base
    replication API, the number of sites in the replication group via the
    (@link #setReplicationNumSites} method.  These methods may be called in
    any order.  For a description of the clock skew values,
    see <a href="{@docRoot}/../programmer_reference/rep_clock_skew.html">Clock skew</a>.
    For a description of master leases, see
    <a href="{@docRoot}/../programmer_reference/rep_lease.html">Master leases</a>.
    <p>
    These arguments can be used to express either raw measurements of a clock
    timing experiment or a percentage across machines.  For instance a group of
    sites have a 2% variance, then <code>replicationClockskewFast</code> should be given as
    102, and <code>replicationClockskewSlow</code> should be set at 100.  Or, for a 0.03%
    difference, you can use 10003 and 10000 respectively.
    <p>
    The database environment's replication subsystem may also be configured using
    the environment's
    <a href="{@docRoot}/../programmer_reference/env_db_config.html#DB_CONFIG">DB_CONFIG</a> file.
    The syntax of the entry in that file is a single line with the string
    "rep_set_clockskew", one or more whitespace characters, and the clockskew
    specified in two parts: the replicationClockskewFast and the replicationClockskewSlow.  For example,
    "rep_set_clockskew 102 100".  Because the
    <a href="{@docRoot}/../programmer_reference/env_db_config.html#DB_CONFIG">DB_CONFIG</a> file is
    read when the database environment is opened, it will silently overrule
    configuration done before that time.
    <p>
    This method configures a database environment, not only operations performed
    using the specified {@link Environment} handle.
    <p>
    This method may not be called after the {@link Environment#replicationManagerStart} or {@link Environment#startReplication} methods are called.
    <p>
    @param replicationClockskewFast
    The value, relative to the <code>replicationClockskewSlow</code>, of the fastest clock in the group of sites.
    @param replicationClockskewSlow
    The value of the slowest clock in the group of sites.

    */
    public void setReplicationClockskew(final int replicationClockskewFast,
        final int replicationClockskewSlow) {

        this.replicationClockskewFast = replicationClockskewFast;
        this.replicationClockskewSlow = replicationClockskewSlow;
    }

    /**  
    Return the current clock skew value for the fastest clock in the group of sites.
    <p>
    This method may be called at any time during the life of the application.
    @return
    The current clock skew value for the fastest clock in the group of sites.
    */
    public int getReplicationClockskewFast() {
        return replicationClockskewFast;
    }

    /**  
    Return the current clock skew value for the slowest clock in the group of sites.
    <p>
    This method may be called at any time during the life of the application.
    @return
    The current clock skew value for the slowest clock in the group of sites.
    */
    public int getReplicationClockskewSlow() {
        return replicationClockskewSlow;
    }

    /**
    Impose a byte-count limit on the amount of data that will be
    transmitted from a site in a single call to {@link com.sleepycat.db.Environment#processReplicationMessage Environment.processReplicationMessage}.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param replicationLimit
    The maximum number of bytes that will be sent in a single call to
    {@link com.sleepycat.db.Environment#processReplicationMessage Environment.processReplicationMessage}.
    */
    public void setReplicationLimit(final long replicationLimit) {
        this.replicationLimit = replicationLimit;
    }

    /**
    Return the transmit limit in bytes for a single call to
    {@link com.sleepycat.db.Environment#processReplicationMessage Environment.processReplicationMessage}.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    The transmit limit in bytes for a single call to {@link com.sleepycat.db.Environment#processReplicationMessage Environment.processReplicationMessage}.
    */
    public long getReplicationLimit() {
        return replicationLimit;
    }

    /**
    Set a threshold for the minimum time that a client waits before requesting
    retransmission of a missing message.  Specifically, if the client detects a
    gap in the sequence of incoming log records or database pages, Berkeley DB
    will wait for at least <code>replicationRequestMin</code> microseconds before requesting
    retransmission of the missing record.  Berkeley DB will double that amount
    before requesting the same missing record again, and so on, up to a maximum
    threshold, set by {@link #setReplicationRequestMax}.  
    <p>
    These values are thresholds only.  Replication Manager applications use these
    values to determine when to automatically request retransmission of missing
    messages. For Base API applications, Berkeley DB has no thread available in
    the library as a timer, the threshold is only checked when a thread enters
    the Berkeley DB library to process an incoming replication message.  Any
    amount of time may have passed since the last message arrived and Berkeley DB
    only checks whether the amount of time since a request was made is beyond the
    threshold value or not.
    <p>
    By default the minimum is 40000 and the maximum is 1280000 (1.28 seconds).  
    These defaults are fairly arbitrary and the application likely needs to
    adjust these.  The values should be based on expected load and performance
    characteristics of the master and client host platforms and transport
    infrastructure as well as round-trip message time.
    <p>
    @param replicationRequestMin
    The minimum amount of time the client waits before requesting retransmission
    of a missing message.
    */
    public void setReplicationRequestMin(final int replicationRequestMin) {
        this.replicationRequestMin = replicationRequestMin;
    }

    /**
    Get the threshold for the minimum amount of time that a client waits before
    requesting retransmission of a missed message.
    <p>
    @return
    The threshold for the minimum amount of time that a client waits before
    requesting retransmission of a missed message.
    */
    public int getReplicationRequestMin() {
        return replicationRequestMin;
    }

    /**
    Set a threshold for the maximum time that a client waits before requesting
    retransmission of a missing message.  Specifically, if the client detects a
    gap in the sequence of incoming log records or database pages, Berkeley DB
    will wait for at least the minimum threshold, set by
    {@link #setReplicationRequestMin}, before requesting retransmission of the
    missing record.  Berkeley DB will double that amount before requesting the
    same missing record again, and so on, up to <code>replicationRequestMax</code>.
    <p>
    These values are thresholds only.  Since Berkeley DB has no thread available
    in the library as a timer, the threshold is only checked when a thread enters
    the Berkeley DB library to process an incoming replication message.  Any
    amount of time may have passed since the last message arrived and Berkeley DB
    only checks whether the amount of time since a request was made is beyond the
    threshold value or not.
    <p>
    By default the minimum is 40000 and the maximum is 1280000 (1.28 seconds).
    These defaults are fairly arbitrary and the application likely needs to
    adjust these.  The values should be based on expected load and performance
    characteristics of the master and client host platforms and transport
    infrastructure as well as round-trip message time.
    <p>
    @param replicationRequestMax
    The maximum amount of time the client waits before requesting retransmission
    of a missing message.
    */
    public void setReplicationRequestMax(final int replicationRequestMax) {
        this.replicationRequestMax = replicationRequestMax;
    }

    /**
    Get the threshold for the maximum amount of time that a client waits before
    requesting retransmission of a missed message.
    <p>
    @return
    The threshold for the maximum amount of time that a client waits before
    requesting retransmission of a missed message.
    */
    public int getReplicationRequestMax() {
        return replicationRequestMax;
    }

    /**
    Initialize the communication infrastructure for a database environment
    participating in a replicated application.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param envid
    The local environment's ID.  It must be a positive integer and
    uniquely identify this Berkeley DB database environment.
    <p>
    @param replicationTransport
    The callback function is used to transmit data using the replication
    application's communication infrastructure.
    */
    public void setReplicationTransport(final int envid,
        final ReplicationTransport replicationTransport) {

        this.envid = envid;
        this.replicationTransport = replicationTransport;
    }

    /**
    Return the replication callback function used to transmit data using
    the replication application's communication infrastructure.
    <p>
    @return
    The replication callback function used to transmit data using the
    replication application's communication infrastructure.
    */
    public ReplicationTransport getReplicationTransport() {
        return replicationTransport;
    }

    /**
    Check if a process has failed while using the database environment, that
    is, if a process has exited with an open {@link Environment} handle.  (For
    this check to be accurate, all processes using the environment must
    specify this flag when opening the environment.)  If recovery
    needs to be run for any reason and either {@link #setRunRecovery} or
    {@link #setRunFatalRecovery} are also specified, recovery will be performed
    and the open will proceed normally.  If recovery needs to be run and no
    recovery flag is specified, a {@link RunRecoveryException} will be thrown.
    If recovery does not need to be run, the recovery flags will be ignored.
    See
    <a href="{@docRoot}/../programmer_reference/transapp_app.html" target="_top">Architecting
    Transactional Data Store applications</a>) for more information.
    <p>
    @param register
    If true, check for process failure when the environment is opened.
    **/
    public void setRegister(final boolean register) {
        this.register = register;
    }

    /**
Return true if the check for process failure when the environment is opened.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the check for process failure when the environment is opened.
    */
    public boolean getRegister() {
        return register;
    }

    /**
    Configure to run catastrophic recovery on this environment before opening it for
normal use.
<p>
A standard part of the recovery process is to remove the existing
database environment and create a new one.  Applications running
recovery must be prepared to re-create the environment because
underlying shared regions will be removed and re-created.
<p>
If the thread of control performing recovery does not specify the
correct database environment initialization information (for example,
the correct memory pool cache size), the result can be an application
running in an environment with incorrect cache and other subsystem
sizes.  For this reason, the thread of control performing recovery
should specify correct configuration information before recovering the
environment; or it should remove the environment after recovery is
completed, leaving creation of a correctly sized environment to a
subsequent call.
<p>
All recovery processing must be single-threaded; that is, only a single
thread of control may perform recovery or access a database environment
while recovery is being performed.  Because it is not an error to run
recovery for an environment for which no recovery is required, it is
reasonable programming practice for the thread of control responsible
for performing recovery and creating the environment to always specify
recovery during startup.
<p>
This method returns successfully if recovery is run no log files exist,
so it is necessary to ensure that all necessary log files are present
before running recovery.
<p>
@param runFatalRecovery
If true, configure to run catastrophic recovery on this environment
before opening it for normal use.
    */
    public void setRunFatalRecovery(final boolean runFatalRecovery) {
        this.runFatalRecovery = runFatalRecovery;
    }

    /**
Return the handle is configured to run catastrophic recovery on
    the database environment before opening it for use.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The handle is configured to run catastrophic recovery on
    the database environment before opening it for use.
    */
    public boolean getRunFatalRecovery() {
        return runFatalRecovery;
    }

    /**
    Configure to run normal recovery on this environment before opening it for
normal use.
<p>
A standard part of the recovery process is to remove the existing
database environment and create a new one.  Applications running
recovery must be prepared to re-create the environment because
underlying shared regions will be removed and re-created.
<p>
If the thread of control performing recovery does not specify the
correct database environment initialization information (for example,
the correct memory pool cache size), the result can be an application
running in an environment with incorrect cache and other subsystem
sizes.  For this reason, the thread of control performing recovery
should specify correct configuration information before recovering the
environment; or it should remove the environment after recovery is
completed, leaving creation of a correctly sized environment to a
subsequent call.
<p>
All recovery processing must be single-threaded; that is, only a single
thread of control may perform recovery or access a database environment
while recovery is being performed.  Because it is not an error to run
recovery for an environment for which no recovery is required, it is
reasonable programming practice for the thread of control responsible
for performing recovery and creating the environment to always specify
recovery during startup.
<p>
This method returns successfully if recovery is run no log files exist,
so it is necessary to ensure that all necessary log files are present
before running recovery.
<p>
@param runRecovery
If true, configure to run normal recovery on this environment
before opening it for normal use.
    */
    public void setRunRecovery(final boolean runRecovery) {
        this.runRecovery = runRecovery;
    }

    /**
Return the handle is configured to run normal recovery on the
    database environment before opening it for use.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The handle is configured to run normal recovery on the
    database environment before opening it for use.
    */
    public boolean getRunRecovery() {
        return runRecovery;
    }

    /**
    Configure the database environment to allocate memory from system
    shared memory instead of from memory backed by the filesystem.
    <p>
    @param systemMemory
    If true, configure the database environment to allocate memory from
    system shared memory instead of from memory backed by the filesystem.
    */
    public void setSystemMemory(final boolean systemMemory) {
        this.systemMemory = systemMemory;
    }

    /**
Return true if the database environment is configured to allocate memory
    from system shared memory instead of from memory backed by the
    filesystem.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured to allocate memory
    from system shared memory instead of from memory backed by the
    filesystem.
    */
    public boolean getSystemMemory() {
        return systemMemory;
    }

    /**
    Specify a base segment ID for database environment shared memory
    regions created in system memory on VxWorks or systems supporting
    X/Open-style shared memory interfaces; for example, UNIX systems
    supporting <code>shmget</code> and related System V IPC interfaces.
    <p>
    This base segment ID will be used when database environment shared
    memory regions are first created.  It will be incremented a small
    integer value each time a new shared memory region is created; that
    is, if the base ID is 35, the first shared memory region created
    will have a segment ID of 35, and the next one will have a segment
    ID between 36 and 40 or so.  A database environment always creates
    a master shared memory region; an additional shared memory region
    for each of the subsystems supported by the environment (Locking,
    Logging, Memory Pool and Transaction); plus an additional shared
    memory region for each additional memory pool cache that is
    supported.  Already existing regions with the same segment IDs will
    be removed.
    <p>
    The intent behind this method is two-fold: without it, applications
    have no way to ensure that two Berkeley DB applications don't
    attempt to use the same segment IDs when creating different database
    environments.  In addition, by using the same segment IDs each time
    the environment is created, previously created segments will be
    removed, and the set of segments on the system will not grow without
    bound.
    The database environment's base segment ID may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_shm_key", one or more whitespace characters, and the ID.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, the
information specified to this method must be consistent with the
existing environment or corruption can occur.
    <p>
    @param segmentId
    The base segment ID for the database environment.
    */
    public void setSegmentId(final long segmentId) {
        this.segmentId = segmentId;
    }

    /**
Return the base segment ID.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The base segment ID.
    */
    public long getSegmentId() {
        return segmentId;
    }

    /**
    Set the path of a directory to be used as the location of temporary
    files.
    <p>
    The files created to back in-memory access method databases will be
    created relative to this path.  These temporary files can be quite
    large, depending on the size of the database.
    <p>
    If no directory is specified, the following alternatives are checked
    in the specified order.  The first existing directory path is used
    for all temporary files.
    <blockquote><ol>
    <li>The value of the environment variable TMPDIR.
    <li>The value of the environment variable TEMP.
    <li>The value of the environment variable TMP.
    <li>The value of the environment variable TempFolder.
    <li>The value returned by the GetTempPath interface.
    <li>The directory /var/tmp.
    <li>The directory /usr/tmp.
    <li>The directory /temp.
    <li>The directory /tmp.
    <li>The directory C:/temp.
    <li>The directory C:/tmp.
    </ol</blockquote>
    <p>
    Note: the environment variables are only checked if the database
    environment has been configured with one of
    {@link com.sleepycat.db.EnvironmentConfig#setUseEnvironment EnvironmentConfig.setUseEnvironment} or
    {@link com.sleepycat.db.EnvironmentConfig#setUseEnvironmentRoot EnvironmentConfig.setUseEnvironmentRoot}.
    <p>
    Note: the GetTempPath interface is only checked on Win/32 platforms.
    <p>
    The database environment's temporary file directory may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_tmp_dir", one or more whitespace characters, and the directory name.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, the
information specified to this method must be consistent with the
existing environment or corruption can occur.
    <p>
    @param temporaryDirectory
    The directory to be used to store temporary files.
    On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
    */
    public void setTemporaryDirectory(final java.io.File temporaryDirectory) {
        this.temporaryDirectory = temporaryDirectory;
    }

    /** @deprecated replaced by {@link #setTemporaryDirectory(java.io.File)} */
    public void setTemporaryDirectory(final String temporaryDirectory) {
        this.setTemporaryDirectory(new java.io.File(temporaryDirectory));
    }

    /**
Return the path of a directory to be used as the location of
    temporary files.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The path of a directory to be used as the location of
    temporary files.
    */
    public java.io.File getTemporaryDirectory() {
        return temporaryDirectory;
    }

    /**
    Set the mutex alignment, in bytes.
    <p>
    It is sometimes advantageous to align mutexes on specific byte
    boundaries in order to minimize cache line collisions.   This method
    specifies an alignment for mutexes allocated by Berkeley DB.
    <p>
    The database environment's mutex alignment may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "mutex_set_align", one or more whitespace characters, and the mutex alignment in bytes.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    @param mutexAlignment
    mutex alignment, in bytes.  The mutex alignment must be a power-of-two.
    **/
    public void setMutexAlignment(final int mutexAlignment) {
        this.mutexAlignment = mutexAlignment;
    }

    /**
Return the mutex alignment, in bytes.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The mutex alignment, in bytes.
    **/
    public int getMutexAlignment() {
        return mutexAlignment;
    }

    /**
    Increase the number of mutexes to allocate.
    <p>
    Berkeley DB allocates a default number of mutexes based on the initial
    configuration of the database environment.  That default calculation may
    be too small if the application has an unusual need for mutexes (for
    example, if the application opens an unexpectedly large number of
    databases) or too large (if the application is trying to minimize its
    memory footprint).  This method configure the number of additional
    mutexes to allocate.
    <p>
    Calling this method discards any value previously
    set using the {@link #setMaxMutexes} method.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param mutexIncrement
    The number of additional mutexes to allocate.
    **/
    public void setMutexIncrement(final int mutexIncrement) {
        this.mutexIncrement = mutexIncrement;
    }

    /**
Return the number of additional mutexes to allocate.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The number of additional mutexes to allocate.
    **/
    public int getMutexIncrement() {
        return mutexIncrement;
    }

    /**
    Set the number of mutexes to allocate when an environment is created.
    <p>
    Berkeley DB allocates a default number of mutexes based on the initial
    configuration of the database environment.  That default calculation may
    be too small if the application has an unusual need for mutexes (for
    example, if the application opens an unexpectedly large number of
    databases) or too large (if the application is trying to minimize its
    memory footprint).  This method configure the number of initial
    mutexes to allocate.
    <p>
    Calling this method discards any value previously
    set using the {@link #setInitialMutexes} method.
    <p>
    This method configures a database environment, including all threads
    of control accessing the database environment, not only the operations
    performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the environment has been opened.
    If joining an existing database environment, any information specified 
    to this method will be ignored.
    <p>
    @param initMutexes
    The number of mutexes allocated when an environment is created.
    **/
    public void setInitialMutexes(final int initMutexes) {
        this.initMutexes = initMutexes;
    }

    /**
    Return the number of mutexes allocated when an environment is created.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    The number of mutexes allocated when an environment is created.
    **/
    public int getInitialMutexes() {
        return initMutexes;
    }

    /**
    Set the total number of mutexes to allocate.
    <p>
    Berkeley DB allocates a default number of mutexes based on the initial
    configuration of the database environment.  That default calculation may
    be too small if the application has an unusual need for mutexes (for
    example, if the application opens an unexpectedly large number of
    databases) or too large (if the application is trying to minimize its
    memory footprint).  This method is used to specify an
    absolute number of mutexes to allocate.
    <p>
    Calling this method discards any value previously
    set using the {@link #setMutexIncrement} method.
    <p>
    The database environment's total number of mutexes may also be set using
    the environment's <b>DB_CONFIG</b> file.  The syntax of the entry in that
    file is a single line with the string "mutex_set_max", one or more
    whitespace characters, and the total number of mutexes. Because the
    <b>DB_CONFIG</b> file is read when the database environment is opened, it
    will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param maxMutexes
    The absolute number of mutexes to allocate.
    **/
    public void setMaxMutexes(final int maxMutexes) {
        this.maxMutexes = maxMutexes;
    }

    /**
Return the total number of mutexes allocated.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The total number of mutexes allocated.
    **/
    public int getMaxMutexes() {
        return maxMutexes;
    }

    /**
    Specify the number of times that test-and-set mutexes should spin
    without blocking.  The value defaults to 1 on uniprocessor systems and
    to 50 times the number of processors on multiprocessor systems.
    <p>
    The database environment's test-and-set spin count may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_tas_spins", one or more whitespace characters, and the number of spins.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param mutexTestAndSetSpins
    The number of spins test-and-set mutexes should execute before blocking.
     **/
    public void setMutexTestAndSetSpins(final int mutexTestAndSetSpins) {
        this.mutexTestAndSetSpins = mutexTestAndSetSpins;
    }

    /**
Return the test-and-set spin count.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The test-and-set spin count.
    **/
    public int getMutexTestAndSetSpins() {
        return mutexTestAndSetSpins;
    }

    /**
    Set the total number of sites the replication group is configured for.
    <p>
    This method cannot be called by replication manager applications.
    <p>
    Note that this value is not automatically populated via a call to
    {@link com.sleepycat.db.Environment#getConfig}, explicit initialization is required for all objects.
    @param replicationNumSites
    The total number of sites in the replication group.
    */
    public void setReplicationNumSites(final int replicationNumSites) {
        if (replicationNumSites == 0)
		throw new java.lang.IllegalArgumentException(
		    "Can't set the number of sites to 0");
        this.replicationNumSites = replicationNumSites;
    }

    /**
    Get the total number of sites configured in this EnvironmentConfig object.
    <p>
    This method cannot be called by replication manager applications.
    <p>
    Note that most applications will be looking for the {@link com.sleepycat.db.Environment#getReplicationNumSites} method
    which returns the number of sites in the replication group as defined by
    the underlying environment object.
    @return
    The total number of sites configured in the EnvironmentConfig object.
    */
    public int getReplicationNumSites() {
        return replicationNumSites;
    }

    /**
    Set the current environment's priority. Priority is used to determine
    which replicated site will be selected as master when an election occurs.
    <p>
    @param replicationPriority
    The database environment priority.
    */
    public void setReplicationPriority(final int replicationPriority) {
        this.replicationPriority = replicationPriority;
    }

    /**
    Get the current environment's priority. Priority is used to determine
    which replicated site will be selected as master when an election occurs.
    <p>
    @return
    The database environment priority.
    */
    public int getReplicationPriority() {
        return replicationPriority;
    }

    /**
    Set the number of times test-and-set mutexes should spin before
    blocking.
    <p>
    The value defaults to 1 on uniprocessor systems and to 50 times the
    number of processors on multiprocessor systems.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param mutexTestAndSetSpins
    The number of times test-and-set mutexes should spin before blocking.
    <p>
    @deprecated replaced by {@link #setMutexTestAndSetSpins}
    */
    public void setTestAndSetSpins(final int mutexTestAndSetSpins) {
        setMutexTestAndSetSpins(mutexTestAndSetSpins);
    }

    /**
Return the number of times test-and-set mutexes should spin before
    blocking.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The number of times test-and-set mutexes should spin before
    blocking.
    <p>
    @deprecated replaced by {@link #getMutexTestAndSetSpins}
    */
    public int getTestAndSetSpins() {
        return getMutexTestAndSetSpins();
    }

    /**
    Configure the handle to be <em>free-threaded</em>; that is, usable
    by multiple threads within a single address space.
    <p>
    This is the default; threading is always assumed in Java, so no special
    configuration is required.
    <p>
    @param threaded
    If true, configure the handle to be <em>free-threaded</em>.
    */
    public void setThreaded(final boolean threaded) {
        this.threaded = threaded;
    }

    /**
Return true if the handle is configured to be <em>free-threaded</em>.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the handle is configured to be <em>free-threaded</em>.
    */
    public boolean getThreaded() {
        return threaded;
    }

    /**
    Configure the database environment for transactions.
    <p>
    This configuration option should be used when transactional guarantees
    such as atomicity of multiple operations and durability are important.
    <p>
    @param transactional
    If true, configure the database environment for transactions.
    */
    public void setTransactional(final boolean transactional) {
        this.transactional = transactional;
    }

    /**
Return true if the database environment is configured for transactions.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured for transactions.
    */
    public boolean getTransactional() {
        return transactional;
    }

    /**
    Configure the system to not write or synchronously flush the log
    on transaction commit.
    <p>
    This means that transactions exhibit the ACI (atomicity, consistency,
    and isolation) properties, but not D (durability); that is, database
    integrity will be maintained, but if the application or system fails,
    it is possible some number of the most recently committed transactions
    may be undone during recovery.  The number of transactions at risk is
    governed by how many log updates can fit into the log buffer, how often
    the operating system flushes dirty buffers to disk, and how often the
    log is checkpointed.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param txnNoSync
    If true, configure the system to not write or synchronously flush
    the log on transaction commit.
    */
    public void setTxnNoSync(final boolean txnNoSync) {
        this.txnNoSync = txnNoSync;
    }

    /**
Return true if the system has been configured to not write or synchronously
    flush the log on transaction commit.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the system has been configured to not write or synchronously
    flush the log on transaction commit.
    */
    public boolean getTxnNoSync() {
        return txnNoSync;
    }

    /**
    If a lock is unavailable for any Berkeley DB operation performed in the
    context of a transaction, cause the operation to throw {@link
    LockNotGrantedException} without waiting for the lock.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param txnNoWait
    If true, configure transactions to not wait for locks by default.
    */
    public void setTxnNoWait(final boolean txnNoWait) {
        this.txnNoWait = txnNoWait;
    }

    /**
Return true if the transactions have been configured to not wait for locks by default.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the transactions have been configured to not wait for locks by default.
    */
    public boolean getTxnNoWait() {
        return txnNoWait;
    }

    /**
    Configure the system to not write log records.
    <p>
    This means that transactions exhibit the ACI (atomicity, consistency,
    and isolation) properties, but not D (durability); that is, database
    integrity will be maintained, but if the application or system
    fails, integrity will not persist.  All database files must be
    verified and/or restored from backup after a failure.  In order to
    ensure integrity after application shut down, all database handles
    must be closed without specifying noSync, or all database changes
    must be flushed from the database environment cache using the
    {@link com.sleepycat.db.Environment#checkpoint Environment.checkpoint}.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param txnNotDurable
    If true, configure the system to not write log records.
    */
    public void setTxnNotDurable(final boolean txnNotDurable) {
        this.txnNotDurable = txnNotDurable;
    }

    /**
Return true if the system has been configured to not write log records.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the system has been configured to not write log records.
    */
    public boolean getTxnNotDurable() {
        return txnNotDurable;
    }

    /**
    Configure the database environment to run transactions at snapshot
    isolation by default.  See {@link TransactionConfig#setSnapshot} for more
    information.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param txnSnapshot
    If true, configure the system to default to snapshot isolation.
    */
    public void setTxnSnapshot(final boolean txnSnapshot) {
        this.txnSnapshot = txnSnapshot;
    }

    /**
Return true if the handle is configured to run all transactions at snapshot
    isolation.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the handle is configured to run all transactions at snapshot
    isolation.
    */
    public boolean getTxnSnapshot() {
        return txnSnapshot;
    }

    /**
    Configure the database environment to support at least txnMaxActive
    active transactions.
    <p>
    This value bounds the size of the memory allocated for transactions.
    Child transactions are counted as active until they either commit
    or abort.
    <p>
    When all of the memory available in the database environment for
    transactions is in use, calls to {@link com.sleepycat.db.Environment#beginTransaction Environment.beginTransaction}
    will fail (until some active transactions complete).  If this
    interface is never called, the database environment is configured
    to support at least 20 active transactions.
    <p>
    The database environment's number of active transactions may also be set using the environment's
DB_CONFIG file.  The syntax of the entry in that file is a single line
with the string "set_tx_max", one or more whitespace characters, and the number of transactions.
Because the DB_CONFIG file is read when the database environment is
opened, it will silently overrule configuration done before that time.
    <p>
    This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
    <p>
    This method may not be called after the
environment has been opened.
If joining an existing database environment, any
information specified to this method will be ignored.
    <p>
    @param txnMaxActive
    The minimum number of simultaneously active transactions supported
    by the database environment.
    */
    public void setTxnMaxActive(final int txnMaxActive) {
        this.txnMaxActive = txnMaxActive;
    }

    /**
Return the minimum number of simultaneously active transactions supported
    by the database environment.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The minimum number of simultaneously active transactions supported
    by the database environment.
    */
    public int getTxnMaxActive() {
        return txnMaxActive;
    }

    /**
    Set the timeout value for the database environment
transactions.
<p>
Transaction timeouts are checked whenever a thread of control blocks on
a lock or when deadlock detection is performed.  The lock is one
requested on behalf of a transaction, normally by the database access
methods underlying the application.
As timeouts are only checked when the lock request first blocks or when
deadlock detection is performed, the accuracy of the timeout depends on
how often deadlock detection is performed.
<p>
Timeout values specified for the database environment may be overridden
on a
per-transaction basis by {@link com.sleepycat.db.Transaction#setTxnTimeout Transaction.setTxnTimeout}.
<p>
This method configures a database environment, including all threads
of control accessing the database environment, not only the operations
performed using a specified {@link com.sleepycat.db.Environment Environment} handle.
<p>
This method may be called at any time during the life of the application.
<p>
@param txnTimeout
The timeout value, specified as an unsigned 32-bit number of
microseconds, limiting the maximum timeout to roughly 71 minutes.
<p>
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setTxnTimeout(final long txnTimeout) {
        this.txnTimeout = txnTimeout;
    }

    /**
Return the database environment transaction timeout value, in
    microseconds; a timeout of 0 means no timeout is set.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The database environment transaction timeout value, in
    microseconds; a timeout of 0 means no timeout is set.
    */
    public long getTxnTimeout() {
        return txnTimeout;
    }

    /**
    Recover to the specified time rather than to the most current
    possible date.
    <p>
    Once a database environment has been upgraded to a new version of
    Berkeley DB involving a log format change, it is no longer possible
    to recover to a specific time before that upgrade.
    <p>
    This method configures only operations performed using a single a
{@link com.sleepycat.db.Environment Environment} handle, not an entire database environment.
    <p>
    This method may not be called after the
environment has been opened.
    <p>
    @param txnTimestamp
    The recovery timestamp.
    Only the seconds (not the milliseconds) of the timestamp are used.
    */
    public void setTxnTimestamp(final java.util.Date txnTimestamp) {
        this.txnTimestamp = txnTimestamp;
    }

    /**
    Return the time to which recovery will be done, or 0 if recovery will
    be done to the most current possible date.
    <p>
    @return
    The time to which recovery will be done, or 0 if recovery will be
    done to the most current possible date.
    */
    public java.util.Date getTxnTimestamp() {
        return txnTimestamp;
    }

    /**
    Configure the system to write, but not synchronously flush, the log on
    transaction commit.
    <p>
    This means that transactions exhibit the ACI (atomicity, consistency,
    and isolation) properties, but not D (durability); that is, database
    integrity will be maintained, but if the system fails, it is possible
    some number of the most recently committed transactions may be undone
    during recovery.  The number of transactions at risk is governed by how
    often the system flushes dirty buffers to disk and how often the log is
    checkpointed.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param txnWriteNoSync
    If true, configure the system to write, but not synchronously flush,
    the log on transaction commit.
    */
    public void setTxnWriteNoSync(final boolean txnWriteNoSync) {
        this.txnWriteNoSync = txnWriteNoSync;
    }

    /**
Return true if the system has been configured to write, but not synchronously
    flush, the log on transaction commit.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the system has been configured to write, but not synchronously
    flush, the log on transaction commit.
    */
    public boolean getTxnWriteNoSync() {
        return txnWriteNoSync;
    }

    /**
    Configure the database environment to accept information from the
    process environment when naming files, regardless of the status of
    the process.
    <p>
    Because permitting users to specify which files are used can create
    security problems, environment information will be used in file
    naming for all users only if configured to do so.
    <p>
    @param useEnvironment
    If true, configure the database environment to accept information
    from the process environment when naming files.
    */
    public void setUseEnvironment(final boolean useEnvironment) {
        this.useEnvironment = useEnvironment;
    }

    /**
Return true if the database environment is configured to accept information
    from the process environment when naming files.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured to accept information
    from the process environment when naming files.
    */
    public boolean getUseEnvironment() {
        return useEnvironment;
    }

    /**
    Configure the database environment to accept information from the
    process environment when naming files, if the process has
    appropriate permissions (for example, users with a user-ID of 0 on
    UNIX systems).
    <p>
    Because permitting users to specify which files are used can create
    security problems, environment information will be used in file
    naming for all users only if configured to do so.
    <p>
    @param useEnvironmentRoot
    If true, configure the database environment to accept information
    from the process environment when naming files if the process has
    appropriate permissions.
    */
    public void setUseEnvironmentRoot(final boolean useEnvironmentRoot) {
        this.useEnvironmentRoot = useEnvironmentRoot;
    }

    /**
Return true if the database environment is configured to accept information
    from the process environment when naming files if the process has
    appropriate permissions.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the database environment is configured to accept information
    from the process environment when naming files if the process has
    appropriate permissions.
    */
    public boolean getUseEnvironmentRoot() {
        return useEnvironmentRoot;
    }

    /**
    Display verbose information.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param flag
    The type of verbose information being configured.
    <p>
    @param enable
    If true, display additional information.
    */
    public void setVerbose(final VerboseConfig flag, boolean enable) {
        int iflag = flag.getInternalFlag();
        switch (iflag) {
        case DbConstants.DB_VERB_BACKUP:
            verboseBackup = enable;
            break;
        case DbConstants.DB_VERB_DEADLOCK:
            verboseDeadlock = enable;
            break;
        case DbConstants.DB_VERB_FILEOPS:
            verboseFileops = enable;
            break;
        case DbConstants.DB_VERB_FILEOPS_ALL:
            verboseFileopsAll = enable;
            break;
        case DbConstants.DB_VERB_RECOVERY:
            verboseRecovery = enable;
            break;
        case DbConstants.DB_VERB_REGISTER:
            verboseRegister = enable;
            break;
        case DbConstants.DB_VERB_REPLICATION:
            verboseReplication = enable;
            break;
        case DbConstants.DB_VERB_REPMGR_CONNFAIL:
            verboseRepmgrConnfail = enable;
            break;
        case DbConstants.DB_VERB_REPMGR_MISC:
            verboseRepmgrMisc = enable;
            break;
        case DbConstants.DB_VERB_REP_ELECT:
            verboseReplicationElection = enable;
            break;
        case DbConstants.DB_VERB_REP_LEASE:
            verboseReplicationLease = enable;
            break;
        case DbConstants.DB_VERB_REP_MISC:
            verboseReplicationMisc = enable;
            break;
        case DbConstants.DB_VERB_REP_MSGS:
            verboseReplicationMsgs = enable;
            break;
        case DbConstants.DB_VERB_REP_SYNC:
            verboseReplicationSync = enable;
            break;
        case DbConstants.DB_VERB_REP_SYSTEM:
            verboseReplicationSystem = enable;
            break;
        case DbConstants.DB_VERB_REP_TEST:
            verboseReplicationTest = enable;
            break;
        case DbConstants.DB_VERB_WAITSFOR:
            verboseWaitsFor = enable;
            break;
        default:
            throw new IllegalArgumentException(
                "Unknown verbose flag: " + DbEnv.strerror(iflag));
        }
    }

    /**
    Return if the database environment is configured to display
    a given type of verbose information.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param flag
    The type of verbose information being queried.
    <p>
    @return
    If the database environment is configured to display additional
    information of the specified type.
    */
    public boolean getVerbose(final VerboseConfig flag) {
        int iflag = flag.getInternalFlag();
        switch (iflag) {
        case DbConstants.DB_VERB_BACKUP:
            return verboseBackup;
        case DbConstants.DB_VERB_DEADLOCK:
            return verboseDeadlock;
        case DbConstants.DB_VERB_FILEOPS:
            return verboseFileops;
        case DbConstants.DB_VERB_FILEOPS_ALL:
            return verboseFileopsAll;
        case DbConstants.DB_VERB_RECOVERY:
            return verboseRecovery;
        case DbConstants.DB_VERB_REGISTER:
            return verboseRegister;
        case DbConstants.DB_VERB_REPLICATION:
            return verboseReplication;
        case DbConstants.DB_VERB_REPMGR_CONNFAIL:
            return verboseRepmgrConnfail;
        case DbConstants.DB_VERB_REPMGR_MISC:
            return verboseRepmgrMisc;
        case DbConstants.DB_VERB_REP_ELECT:
            return verboseReplicationElection;
        case DbConstants.DB_VERB_REP_LEASE:
            return verboseReplicationLease;
        case DbConstants.DB_VERB_REP_MISC:
            return verboseReplicationMisc;
        case DbConstants.DB_VERB_REP_MSGS:
            return verboseReplicationMsgs;
        case DbConstants.DB_VERB_REP_SYNC:
            return verboseReplicationSync;
        case DbConstants.DB_VERB_REP_SYSTEM:
            return verboseReplicationSystem;
        case DbConstants.DB_VERB_REP_TEST:
            return verboseReplicationTest;
        case DbConstants.DB_VERB_WAITSFOR:
            return verboseWaitsFor;
        default:
            throw new IllegalArgumentException(
                "Unknown verbose flag: " + DbEnv.strerror(iflag));
       }
    }

    /**
    Display additional information when doing deadlock detection.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param verboseDeadlock
    If true, display additional information when doing deadlock
    detection.
    <p>
    @deprecated replaced by {@link #setVerbose}
    */
    public void setVerboseDeadlock(final boolean verboseDeadlock) {
        this.verboseDeadlock = verboseDeadlock;
    }

    /**
    Return if the database environment is configured to display
    additional information when doing deadlock detection.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    If the database environment is configured to display additional
    information when doing deadlock detection.
    <p>
    @deprecated replaced by {@link #getVerbose}
    */
    public boolean getVerboseDeadlock() {
        return verboseDeadlock;
    }

    /**
    Display additional information when performing recovery.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param verboseRecovery
    If true, display additional information when performing recovery.
    <p>
    @deprecated replaced by {@link #setVerbose}
    */
    public void setVerboseRecovery(final boolean verboseRecovery) {
        this.verboseRecovery = verboseRecovery;
    }

    /**
    Return if the database environment is configured to display
    additional information when performing recovery.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    If the database environment is configured to display additional
    information when performing recovery.
    <p>
    @deprecated replaced by {@link #getVerbose}
    */
    public boolean getVerboseRecovery() {
        return verboseRecovery;
    }

    /**
    Display additional information concerning support for the
     {@link #setRegister} method.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param verboseRegister
    If true, display additional information concerning support for the
     {@link #setRegister} method
    <p>
    @deprecated replaced by {@link #setVerbose}
    */
    public void setVerboseRegister(final boolean verboseRegister) {
        this.verboseRegister = verboseRegister;
    }

    /**
    Return if the database environment is configured to display
    additional information concerning support for the
     {@link #setRegister} method.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    If the database environment is configured to display additional
    information concerning support for the
     {@link #setRegister} method.
    <p>
    @deprecated replaced by {@link #getVerbose}
    */
    public boolean getVerboseRegister() {
        return verboseRegister;
    }

    /**
    Display additional information when processing replication messages.
    <p>
    Note, to get complete replication logging when debugging replication
    applications, you must also configure and build the Berkeley DB
    library with the --enable-diagnostic configuration option as well
    as call this method.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param verboseReplication
    If true, display additional information when processing replication
    messages.
    <p>
    @deprecated replaced by {@link #setVerbose}
    */
    public void setVerboseReplication(final boolean verboseReplication) {
        this.verboseReplication = verboseReplication;
    }

    /**
    Return if the database environment is configured to display
    additional information when processing replication messages.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    If the database environment is configured to display additional
    information when processing replication messages.
    <p>
    @deprecated replaced by {@link #getVerbose}
    */
    public boolean getVerboseReplication() {
        return verboseReplication;
    }

    /**
    Display the waits-for table when doing deadlock detection.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param verboseWaitsFor
    If true, display the waits-for table when doing deadlock detection.
    <p>
    @deprecated replaced by {@link #setVerbose}
    */
    public void setVerboseWaitsFor(final boolean verboseWaitsFor) {
        this.verboseWaitsFor = verboseWaitsFor;
    }

    /**
    Return if the database environment is configured to display the
    waits-for table when doing deadlock detection.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @return
    If the database environment is configured to display the waits-for
    table when doing deadlock detection.
    <p>
    @deprecated replaced by {@link #getVerbose}
    */
    public boolean getVerboseWaitsFor() {
        return verboseWaitsFor;
    }

    /**
    Configure the system to yield the processor immediately after each
    page or mutex acquisition.
    <p>
    This functionality should never be used for purposes other than
    stress testing.
    <p>
    This method only affects the specified {@link com.sleepycat.db.Environment Environment} handle (and
any other library handles opened within the scope of that handle).
For consistent behavior across the environment, all {@link com.sleepycat.db.Environment Environment}
handles opened in the database environment must either call this method
or the configuration should be specified in the database environment's
DB_CONFIG configuration file.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    @param yieldCPU
    If true, configure the system to yield the processor immediately
    after each page or mutex acquisition.
    */
    public void setYieldCPU(final boolean yieldCPU) {
        this.yieldCPU = yieldCPU;
    }

    /**
Return true if the system has been configured to yield the processor
    immediately after each page or mutex acquisition.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the system has been configured to yield the processor
    immediately after each page or mutex acquisition.
    */
    public boolean getYieldCPU() {
        return yieldCPU;
    }

    private boolean lockConflictsEqual(byte[][] lc1, byte[][]lc2) {
        if (lc1 == lc2)
            return true;
        if (lc1 == null || lc2 == null || lc1.length != lc2.length)
            return false;
        for (int i = 0; i < lc1.length; i++) {
            if (lc1[i].length != lc2[i].length)
                return false;
            for (int j = 0; j < lc1[i].length; j++)
                if (lc1[i][j] != lc2[i][j])
                    return false;
        }
        return true;
    }

    /* package */
    DbEnv openEnvironment(final java.io.File home)
        throws DatabaseException, java.io.FileNotFoundException {

        final DbEnv dbenv = createEnvironment();
        int openFlags = 0;

        openFlags |= allowCreate ? DbConstants.DB_CREATE : 0;
        openFlags |= initializeCache ? DbConstants.DB_INIT_MPOOL : 0;
        openFlags |= initializeCDB ? DbConstants.DB_INIT_CDB : 0;
        openFlags |= initializeLocking ? DbConstants.DB_INIT_LOCK : 0;
        openFlags |= initializeLogging ? DbConstants.DB_INIT_LOG : 0;
        openFlags |= initializeReplication ? DbConstants.DB_INIT_REP : 0;
        openFlags |= joinEnvironment ? DbConstants.DB_JOINENV : 0;
        openFlags |= lockDown ? DbConstants.DB_LOCKDOWN : 0;
        openFlags |= isPrivate ? DbConstants.DB_PRIVATE : 0;
        openFlags |= register ? DbConstants.DB_REGISTER : 0;
        openFlags |= runRecovery ? DbConstants.DB_RECOVER : 0;
        openFlags |= runFatalRecovery ? DbConstants.DB_RECOVER_FATAL : 0;
        openFlags |= systemMemory ? DbConstants.DB_SYSTEM_MEM : 0;
        openFlags |= threaded ? DbConstants.DB_THREAD : 0;
        openFlags |= transactional ? DbConstants.DB_INIT_TXN : 0;
        openFlags |= useEnvironment ? DbConstants.DB_USE_ENVIRON : 0;
        openFlags |= useEnvironmentRoot ? DbConstants.DB_USE_ENVIRON_ROOT : 0;

        boolean succeeded = false;
        try {
            dbenv.open((home == null) ? null : home.toString(),
                openFlags, mode);
            succeeded = true;
            return dbenv;
        } finally {
            if (!succeeded)
                try {
                    dbenv.close(0);
                } catch (Throwable t) {
                    // Ignore it -- an exception is already in flight.
                }
        }
    }


    /* package */
    DbEnv createEnvironment()
        throws DatabaseException {

        int createFlags = 0;

        final DbEnv dbenv = new DbEnv(createFlags);
        configureEnvironment(dbenv, DEFAULT);
        return dbenv;
    }

    /* package */
    void configureEnvironment(final DbEnv dbenv,
                              final EnvironmentConfig oldConfig)
        throws DatabaseException {

        if (errorHandler != oldConfig.errorHandler)
            dbenv.set_errcall(errorHandler);
        if (errorPrefix != oldConfig.errorPrefix &&
            errorPrefix != null && !errorPrefix.equals(oldConfig.errorPrefix))
            dbenv.set_errpfx(errorPrefix);
        if (errorStream != oldConfig.errorStream)
            dbenv.set_error_stream(errorStream);

        // We always set DB_TIME_NOTGRANTED in the Java API, because
        // LockNotGrantedException extends DeadlockException, so there's no
        // reason why an application would prefer one to the other.
        int onFlags = DbConstants.DB_TIME_NOTGRANTED;
        int offFlags = 0;

        if (cdbLockAllDatabases && !oldConfig.cdbLockAllDatabases)
            onFlags |= DbConstants.DB_CDB_ALLDB;
        if (!cdbLockAllDatabases && oldConfig.cdbLockAllDatabases)
            offFlags |= DbConstants.DB_CDB_ALLDB;

        if (directDatabaseIO && !oldConfig.directDatabaseIO)
            onFlags |= DbConstants.DB_DIRECT_DB;
        if (!directDatabaseIO && oldConfig.directDatabaseIO)
            offFlags |= DbConstants.DB_DIRECT_DB;

        if (dsyncDatabases && !oldConfig.dsyncDatabases)
            onFlags |= DbConstants.DB_DSYNC_DB;
        if (!dsyncDatabases && oldConfig.dsyncDatabases)
            offFlags |= DbConstants.DB_DSYNC_DB;

        if (hotbackupInProgress && !oldConfig.hotbackupInProgress)
            onFlags |= DbConstants.DB_HOTBACKUP_IN_PROGRESS;
        if (!hotbackupInProgress && oldConfig.hotbackupInProgress)
            offFlags |= DbConstants.DB_HOTBACKUP_IN_PROGRESS;

        if (initializeRegions && !oldConfig.initializeRegions)
            onFlags |= DbConstants.DB_REGION_INIT;
        if (!initializeRegions && oldConfig.initializeRegions)
            offFlags |= DbConstants.DB_REGION_INIT;

        if (multiversion && !oldConfig.multiversion)
            onFlags |= DbConstants.DB_MULTIVERSION;
        if (!multiversion && oldConfig.multiversion)
            offFlags |= DbConstants.DB_MULTIVERSION;

        if (noLocking && !oldConfig.noLocking)
            onFlags |= DbConstants.DB_NOLOCKING;
        if (!noLocking && oldConfig.noLocking)
            offFlags |= DbConstants.DB_NOLOCKING;

        if (noMMap && !oldConfig.noMMap)
            onFlags |= DbConstants.DB_NOMMAP;
        if (!noMMap && oldConfig.noMMap)
            offFlags |= DbConstants.DB_NOMMAP;

        if (noPanic && !oldConfig.noPanic)
            onFlags |= DbConstants.DB_NOPANIC;
        if (!noPanic && oldConfig.noPanic)
            offFlags |= DbConstants.DB_NOPANIC;

        if (overwrite && !oldConfig.overwrite)
            onFlags |= DbConstants.DB_OVERWRITE;
        if (!overwrite && oldConfig.overwrite)
            offFlags |= DbConstants.DB_OVERWRITE;

        if (txnNoSync && !oldConfig.txnNoSync)
            onFlags |= DbConstants.DB_TXN_NOSYNC;
        if (!txnNoSync && oldConfig.txnNoSync)
            offFlags |= DbConstants.DB_TXN_NOSYNC;

        if (txnNoWait && !oldConfig.txnNoWait)
            onFlags |= DbConstants.DB_TXN_NOWAIT;
        if (!txnNoWait && oldConfig.txnNoWait)
            offFlags |= DbConstants.DB_TXN_NOWAIT;

        if (txnNotDurable && !oldConfig.txnNotDurable)
            onFlags |= DbConstants.DB_TXN_NOT_DURABLE;
        if (!txnNotDurable && oldConfig.txnNotDurable)
            offFlags |= DbConstants.DB_TXN_NOT_DURABLE;

        if (txnSnapshot && !oldConfig.txnSnapshot)
            onFlags |= DbConstants.DB_TXN_SNAPSHOT;
        if (!txnSnapshot && oldConfig.txnSnapshot)
            offFlags |= DbConstants.DB_TXN_SNAPSHOT;

        if (txnWriteNoSync && !oldConfig.txnWriteNoSync)
            onFlags |= DbConstants.DB_TXN_WRITE_NOSYNC;
        if (!txnWriteNoSync && oldConfig.txnWriteNoSync)
            offFlags |= DbConstants.DB_TXN_WRITE_NOSYNC;

        if (yieldCPU && !oldConfig.yieldCPU)
            onFlags |= DbConstants.DB_YIELDCPU;
        if (!yieldCPU && oldConfig.yieldCPU)
            offFlags |= DbConstants.DB_YIELDCPU;

        if (onFlags != 0)
            dbenv.set_flags(onFlags, true);
        if (offFlags != 0)
            dbenv.set_flags(offFlags, false);

        /* Log flags */
        if (directLogIO != oldConfig.directLogIO)
            dbenv.log_set_config(DbConstants.DB_LOG_DIRECT, directLogIO);

        if (dsyncLog != oldConfig.dsyncLog)
            dbenv.log_set_config(DbConstants.DB_LOG_DSYNC, dsyncLog);

        if (logAutoRemove != oldConfig.logAutoRemove)
            dbenv.log_set_config(DbConstants.DB_LOG_AUTO_REMOVE, logAutoRemove);

        if (logBlobContent != oldConfig.logBlobContent)
            dbenv.log_set_config(DbConstants.DB_LOG_BLOB, logBlobContent);

        if (logInMemory != oldConfig.logInMemory)
            dbenv.log_set_config(DbConstants.DB_LOG_IN_MEMORY, logInMemory);

        if (logZero != oldConfig.logZero)
            dbenv.log_set_config(DbConstants.DB_LOG_ZERO, logZero);

        /* Verbose flags */
        if (verboseBackup != oldConfig.verboseBackup)
            dbenv.set_verbose(DbConstants.DB_VERB_BACKUP, verboseBackup);
        if (verboseDeadlock != oldConfig.verboseDeadlock)
            dbenv.set_verbose(DbConstants.DB_VERB_DEADLOCK, verboseDeadlock);
        if (verboseFileops != oldConfig.verboseFileops)
            dbenv.set_verbose(DbConstants.DB_VERB_FILEOPS, verboseFileops);
        if (verboseFileopsAll != oldConfig.verboseFileopsAll)
            dbenv.set_verbose(DbConstants.DB_VERB_FILEOPS_ALL,
                verboseFileopsAll);
        if (verboseRecovery != !oldConfig.verboseRecovery)
            dbenv.set_verbose(DbConstants.DB_VERB_RECOVERY, verboseRecovery);
        if (verboseRegister != oldConfig.verboseRegister)
            dbenv.set_verbose(DbConstants.DB_VERB_REGISTER, verboseRegister);
        if (verboseReplication != oldConfig.verboseReplication)
            dbenv.set_verbose(DbConstants.DB_VERB_REPLICATION,
                verboseReplication);
        if (verboseReplicationElection != oldConfig.verboseReplicationElection)
            dbenv.set_verbose(DbConstants.DB_VERB_REP_ELECT,
                verboseReplicationElection);
        if (verboseReplicationLease != oldConfig.verboseReplicationLease)
            dbenv.set_verbose(DbConstants.DB_VERB_REP_LEASE,
                verboseReplicationLease);
        if (verboseReplicationMisc != oldConfig.verboseReplicationMisc)
            dbenv.set_verbose(DbConstants.DB_VERB_REP_MISC,
                verboseReplicationMisc);
        if (verboseReplicationMsgs != oldConfig.verboseReplicationMsgs)
            dbenv.set_verbose(DbConstants.DB_VERB_REP_MSGS,
                verboseReplicationMsgs);
        if (verboseReplicationSync != oldConfig.verboseReplicationSync)
            dbenv.set_verbose(DbConstants.DB_VERB_REP_SYNC,
                verboseReplicationSync);
        if (verboseReplicationSystem != oldConfig.verboseReplicationSystem)
            dbenv.set_verbose(DbConstants.DB_VERB_REP_SYSTEM,
                verboseReplicationSystem);
        if (verboseReplicationTest != oldConfig.verboseReplicationTest)
            dbenv.set_verbose(DbConstants.DB_VERB_REP_TEST,
                verboseReplicationTest);
        if (verboseRepmgrConnfail != oldConfig.verboseRepmgrConnfail)
            dbenv.set_verbose(DbConstants.DB_VERB_REPMGR_CONNFAIL,
                verboseRepmgrConnfail);
        if (verboseRepmgrMisc != oldConfig.verboseRepmgrMisc)
            dbenv.set_verbose(DbConstants.DB_VERB_REPMGR_MISC,
                verboseRepmgrMisc);
        if (verboseWaitsFor != oldConfig.verboseWaitsFor)
            dbenv.set_verbose(DbConstants.DB_VERB_WAITSFOR, verboseWaitsFor);

        /* Callbacks */
        if (backupHandler != oldConfig.backupHandler)
            dbenv.set_backup_callbacks(backupHandler, backupHandler, backupHandler);
        if (feedbackHandler != oldConfig.feedbackHandler)
            dbenv.set_feedback(feedbackHandler);
        if (logRecordHandler != oldConfig.logRecordHandler)
            dbenv.set_app_dispatch(logRecordHandler);
        if (eventHandler != oldConfig.eventHandler)
            dbenv.set_event_notify(eventHandler);
        if (messageHandler != oldConfig.messageHandler)
            dbenv.set_msgcall(messageHandler);
        if (panicHandler != oldConfig.panicHandler)
            dbenv.set_paniccall(panicHandler);
        /*
         * Configure replication views for a new environment or an existing
         * environment with the callback provided by the application.
         * If the callback is set as null, the replication view is a full view
         * and all database files are replicated to the local site. Otherwise
         * it is a partial view and only some database files are replicated to
         * the local site.
         */
        if (repViewIsSet)
            dbenv.rep_set_view(replicationViewHandler);
        if (replicationTransport != oldConfig.replicationTransport)
            dbenv.rep_set_transport(envid, replicationTransport);

        /* Other settings */

	if (backup_read_count != 0)
	    dbenv.set_backup_config(DbConstants.DB_BACKUP_READ_COUNT, 
		backup_read_count);

	if (backup_read_sleep != 0)
	    dbenv.set_backup_config(DbConstants.DB_BACKUP_READ_SLEEP, 
		backup_read_sleep);

	if (backup_size != 0) {
	    dbenv.set_backup_config(DbConstants.DB_BACKUP_SIZE,
		backup_size);
        }

	if (backup_write_direct == true) 
	    dbenv.set_backup_config(DbConstants.DB_BACKUP_WRITE_DIRECT, 1);
	else
	    dbenv.set_backup_config(DbConstants.DB_BACKUP_WRITE_DIRECT, 0);

        if (blobDir != oldConfig.blobDir)
            dbenv.set_blob_dir(blobDir.toString());
        if (blobThreshold != oldConfig.blobThreshold)
            dbenv.set_blob_threshold(blobThreshold, 0);

        if (cacheSize != oldConfig.cacheSize ||
            cacheCount != oldConfig.cacheCount)
            dbenv.set_cachesize(cacheSize, cacheCount);
        if (cacheMax != oldConfig.cacheMax)
            dbenv.set_cache_max(cacheMax);

        for (final java.util.Enumeration e = dataDirs.elements();
            e.hasMoreElements();) {
            final java.io.File dir = (java.io.File)e.nextElement();
            if (!oldConfig.dataDirs.contains(dir))
                dbenv.add_data_dir(dir.toString());
        }
        if (createDir != oldConfig.createDir)
            dbenv.set_create_dir(createDir.toString());

        if (!lockConflictsEqual(lockConflicts, oldConfig.lockConflicts))
            dbenv.set_lk_conflicts(lockConflicts);
        if (lockDetectMode != oldConfig.lockDetectMode)
            dbenv.set_lk_detect(lockDetectMode.getFlag());
        if (lockTableSize != oldConfig.lockTableSize)
            dbenv.set_lk_tablesize(lockTableSize);
        if (maxLocks != oldConfig.maxLocks)
            dbenv.set_lk_max_locks(maxLocks);
        if (maxLockers != oldConfig.maxLockers)
            dbenv.set_lk_max_lockers(maxLockers);
        if (maxLockObjects != oldConfig.maxLockObjects)
            dbenv.set_lk_max_objects(maxLockObjects);
        if (partitionLocks != oldConfig.partitionLocks)
            dbenv.set_lk_partitions(partitionLocks);
        if (maxLogFileSize != oldConfig.maxLogFileSize)
            dbenv.set_lg_max(maxLogFileSize);
        if (logBufferSize != oldConfig.logBufferSize)
            dbenv.set_lg_bsize(logBufferSize);
        if (logDirectory != oldConfig.logDirectory && logDirectory != null &&
            !logDirectory.equals(oldConfig.logDirectory))
            dbenv.set_lg_dir(logDirectory.toString());
        if (logFileMode != oldConfig.logFileMode)
            dbenv.set_lg_filemode(logFileMode);
        if (logRegionSize != oldConfig.logRegionSize)
            dbenv.set_lg_regionmax(logRegionSize);
        if (maxOpenFiles != oldConfig.maxOpenFiles)
            dbenv.set_mp_max_openfd(maxOpenFiles);
        if (maxWrite != oldConfig.maxWrite ||
            maxWriteSleep != oldConfig.maxWriteSleep)
            dbenv.set_mp_max_write(maxWrite, maxWriteSleep);
        if (messageStream != oldConfig.messageStream)
            dbenv.set_message_stream(messageStream);
        if (metadataDir != oldConfig.metadataDir)
            dbenv.set_metadata_dir(metadataDir.toString());
        if (mmapSize != oldConfig.mmapSize)
            dbenv.set_mp_mmapsize(mmapSize);
        if (mpPageSize != oldConfig.mpPageSize)
            dbenv.set_mp_pagesize(mpPageSize);
        if (mpTableSize != oldConfig.mpTableSize)
            dbenv.set_mp_tablesize(mpTableSize);
        if (password != null)
            dbenv.set_encrypt(password, DbConstants.DB_ENCRYPT_AES);
        if (replicationClockskewFast != oldConfig.replicationClockskewFast ||
            replicationClockskewSlow != oldConfig.replicationClockskewSlow)
            dbenv.rep_set_clockskew(replicationClockskewFast, replicationClockskewSlow);
        if (replicationLimit != oldConfig.replicationLimit)
            dbenv.rep_set_limit(replicationLimit);
        if (replicationRequestMin != oldConfig.replicationRequestMin ||
            replicationRequestMax != oldConfig.replicationRequestMax)
            dbenv.rep_set_request(replicationRequestMin, replicationRequestMax);
        if (segmentId != oldConfig.segmentId)
            dbenv.set_shm_key(segmentId);
        if (mutexAlignment != oldConfig.mutexAlignment)
            dbenv.mutex_set_align(mutexAlignment);
        if (mutexIncrement != oldConfig.mutexIncrement)
            dbenv.mutex_set_increment(mutexIncrement);
        if (initMutexes != oldConfig.initMutexes)
            dbenv.mutex_set_max(initMutexes);
        if (maxMutexes != oldConfig.maxMutexes)
            dbenv.mutex_set_max(maxMutexes);
        if (mutexTestAndSetSpins != oldConfig.mutexTestAndSetSpins)
            dbenv.mutex_set_tas_spins(mutexTestAndSetSpins);
        if (replicationNumSites != oldConfig.replicationNumSites)
            dbenv.rep_set_nsites(replicationNumSites);
        if (replicationPriority != oldConfig.replicationPriority)
            dbenv.rep_set_priority(replicationPriority);
        if (lockTimeout != oldConfig.lockTimeout)
            dbenv.set_timeout(lockTimeout, DbConstants.DB_SET_LOCK_TIMEOUT);
        if (txnMaxActive != oldConfig.txnMaxActive)
            dbenv.set_tx_max(txnMaxActive);
        if (txnTimeout != oldConfig.txnTimeout)
            dbenv.set_timeout(txnTimeout, DbConstants.DB_SET_TXN_TIMEOUT);
        if (txnTimestamp != oldConfig.txnTimestamp && txnTimestamp != null &&
            !txnTimestamp.equals(oldConfig.txnTimestamp))
            dbenv.set_tx_timestamp(txnTimestamp);
        if (temporaryDirectory != oldConfig.temporaryDirectory &&
            temporaryDirectory != null &&
            !temporaryDirectory.equals(oldConfig.temporaryDirectory))
            dbenv.set_tmp_dir(temporaryDirectory.toString());
        if (initResourceLocks != oldConfig.initResourceLocks)
            dbenv.set_memory_init(
                DbConstants.DB_MEM_LOCK, initResourceLocks);
        if (initResourceLockObjects != oldConfig.initResourceLockObjects)
            dbenv.set_memory_init(
                DbConstants.DB_MEM_LOCKOBJECT, initResourceLockObjects);
        if (initResourceLockers != oldConfig.initResourceLockers)
            dbenv.set_memory_init(
                DbConstants.DB_MEM_LOCKER, initResourceLockers);
        if (initResourceLogIds != oldConfig.initResourceLogIds)
            dbenv.set_memory_init(
                DbConstants.DB_MEM_LOGID, initResourceLogIds);
        if (initResourceTransactions != oldConfig.initResourceTransactions)
            dbenv.set_memory_init(
                DbConstants.DB_MEM_TRANSACTION, initResourceTransactions);
        if (initResourceThreads != oldConfig.initResourceThreads)
            dbenv.set_memory_init(
                DbConstants.DB_MEM_THREAD, initResourceThreads);
        if (regionMemoryMax != oldConfig.regionMemoryMax)
            dbenv.set_memory_max(regionMemoryMax);
        if (replicationInMemory != oldConfig.replicationInMemory)
            dbenv.rep_set_config(
	        DbConstants.DB_REP_CONF_INMEM, replicationInMemory);
        if (repmgrAckPolicy != oldConfig.repmgrAckPolicy)
            dbenv.repmgr_set_ack_policy(repmgrAckPolicy.getId());
        java.util.Iterator elems = repmgrSitesConfig.listIterator();
        java.util.Iterator oldElems = oldConfig.repmgrSitesConfig.listIterator();
        while (elems.hasNext()){
            ReplicationManagerSiteConfig ent =
                (ReplicationManagerSiteConfig)elems.next();
            ReplicationManagerSiteConfig oldEnt = null;
            while (oldElems.hasNext()){
                ReplicationManagerSiteConfig tmpEnt =
                    (ReplicationManagerSiteConfig)oldElems.next();
                if (ent.getAddress().equals(tmpEnt.getAddress())) {
                    oldEnt = tmpEnt;
                    break;
                }
            }
            if (oldEnt == null)
                oldEnt = new ReplicationManagerSiteConfig(
                    ent.getHost(), ent.getPort());
            if (!oldEnt.equals(ent)) {
                DbSite site = dbenv.repmgr_site(ent.getHost(), ent.getPort());
                if (ent.getBootstrapHelper() != oldEnt.getBootstrapHelper()) 
                    site.set_config(DbConstants.DB_BOOTSTRAP_HELPER,
                        ent.getBootstrapHelper());
                if (ent.getGroupCreator() != oldEnt.getGroupCreator())
                    site.set_config(DbConstants.DB_GROUP_CREATOR,
                        ent.getGroupCreator());
                if (ent.getLegacy() != oldEnt.getLegacy())
                    site.set_config(DbConstants.DB_LEGACY, ent.getLegacy());
                if (ent.getLocalSite() != oldEnt.getLocalSite())
                    site.set_config(DbConstants.DB_LOCAL_SITE,
                        ent.getLocalSite());
                if (ent.getPeer() != oldEnt.getPeer())
                    site.set_config(DbConstants.DB_REPMGR_PEER, ent.getPeer());
                site.close();
            }
        }
    }

    /* package */
    EnvironmentConfig(final DbEnv dbenv)
        throws DatabaseException {

        final int openFlags = dbenv.get_open_flags();

        allowCreate = ((openFlags & DbConstants.DB_CREATE) != 0);
        initializeCache = ((openFlags & DbConstants.DB_INIT_MPOOL) != 0);
        initializeCDB = ((openFlags & DbConstants.DB_INIT_CDB) != 0);
        initializeLocking = ((openFlags & DbConstants.DB_INIT_LOCK) != 0);
        initializeLogging = ((openFlags & DbConstants.DB_INIT_LOG) != 0);
        initializeReplication = ((openFlags & DbConstants.DB_INIT_REP) != 0);
        joinEnvironment = ((openFlags & DbConstants.DB_JOINENV) != 0);
        lockDown = ((openFlags & DbConstants.DB_LOCKDOWN) != 0);
        isPrivate = ((openFlags & DbConstants.DB_PRIVATE) != 0);
        register = ((openFlags & DbConstants.DB_REGISTER) != 0);
        runRecovery = ((openFlags & DbConstants.DB_RECOVER) != 0);
        runFatalRecovery = ((openFlags & DbConstants.DB_RECOVER_FATAL) != 0);
        systemMemory = ((openFlags & DbConstants.DB_SYSTEM_MEM) != 0);
        threaded = ((openFlags & DbConstants.DB_THREAD) != 0);
        transactional = ((openFlags & DbConstants.DB_INIT_TXN) != 0);
        useEnvironment = ((openFlags & DbConstants.DB_USE_ENVIRON) != 0);
        useEnvironmentRoot =
            ((openFlags & DbConstants.DB_USE_ENVIRON_ROOT) != 0);

        final int envFlags = dbenv.get_flags();

        cdbLockAllDatabases = ((envFlags & DbConstants.DB_CDB_ALLDB) != 0);
        directDatabaseIO = ((envFlags & DbConstants.DB_DIRECT_DB) != 0);
        dsyncDatabases = ((envFlags & DbConstants.DB_DSYNC_DB) != 0);
        hotbackupInProgress = ((envFlags & DbConstants.DB_HOTBACKUP_IN_PROGRESS) != 0);
        initializeRegions = ((envFlags & DbConstants.DB_REGION_INIT) != 0);
        multiversion = ((envFlags & DbConstants.DB_MULTIVERSION) != 0);
        noLocking = ((envFlags & DbConstants.DB_NOLOCKING) != 0);
        noMMap = ((envFlags & DbConstants.DB_NOMMAP) != 0);
        noPanic = ((envFlags & DbConstants.DB_NOPANIC) != 0);
        overwrite = ((envFlags & DbConstants.DB_OVERWRITE) != 0);
        txnNoSync = ((envFlags & DbConstants.DB_TXN_NOSYNC) != 0);
        txnNoWait = ((envFlags & DbConstants.DB_TXN_NOWAIT) != 0);
        txnNotDurable = ((envFlags & DbConstants.DB_TXN_NOT_DURABLE) != 0);
        txnSnapshot = ((envFlags & DbConstants.DB_TXN_SNAPSHOT) != 0);
        txnWriteNoSync = ((envFlags & DbConstants.DB_TXN_WRITE_NOSYNC) != 0);
        yieldCPU = ((envFlags & DbConstants.DB_YIELDCPU) != 0);

        /* Log flags */
        if (initializeLogging) {
            directLogIO = dbenv.log_get_config(DbConstants.DB_LOG_DIRECT);
            dsyncLog = dbenv.log_get_config(DbConstants.DB_LOG_DSYNC);
            logAutoRemove = dbenv.log_get_config(DbConstants.DB_LOG_AUTO_REMOVE);
            logBlobContent = dbenv.log_get_config(DbConstants.DB_LOG_BLOB);
            logInMemory = dbenv.log_get_config(DbConstants.DB_LOG_IN_MEMORY);
            logZero = dbenv.log_get_config(DbConstants.DB_LOG_ZERO);
        }

        /* Verbose flags */
        verboseBackup = dbenv.get_verbose(DbConstants.DB_VERB_BACKUP);
        verboseDeadlock = dbenv.get_verbose(DbConstants.DB_VERB_DEADLOCK);
        verboseFileops = dbenv.get_verbose(DbConstants.DB_VERB_FILEOPS);
        verboseFileopsAll = dbenv.get_verbose(DbConstants.DB_VERB_FILEOPS_ALL);
        verboseRecovery = dbenv.get_verbose(DbConstants.DB_VERB_RECOVERY);
        verboseRegister = dbenv.get_verbose(DbConstants.DB_VERB_REGISTER);
        verboseReplication = dbenv.get_verbose(DbConstants.DB_VERB_REPLICATION);
        verboseReplicationElection = dbenv.get_verbose(DbConstants.DB_VERB_REP_ELECT);
        verboseReplicationLease = dbenv.get_verbose(DbConstants.DB_VERB_REP_LEASE);
        verboseReplicationMisc = dbenv.get_verbose(DbConstants.DB_VERB_REP_MISC);
        verboseReplicationMsgs = dbenv.get_verbose(DbConstants.DB_VERB_REP_MSGS);
        verboseReplicationSync = dbenv.get_verbose(DbConstants.DB_VERB_REP_SYNC);
        verboseReplicationSystem = dbenv.get_verbose(DbConstants.DB_VERB_REP_SYSTEM);
        verboseReplicationTest = dbenv.get_verbose(DbConstants.DB_VERB_REP_TEST);
        verboseRepmgrConnfail = dbenv.get_verbose(DbConstants.DB_VERB_REPMGR_CONNFAIL);
        verboseRepmgrMisc = dbenv.get_verbose(DbConstants.DB_VERB_REPMGR_MISC);
        verboseWaitsFor = dbenv.get_verbose(DbConstants.DB_VERB_WAITSFOR);

        /* Callbacks */
        errorHandler = dbenv.get_errcall();
        feedbackHandler = dbenv.get_feedback();
        logRecordHandler = dbenv.get_app_dispatch();
        eventHandler = dbenv.get_event_notify();
        messageHandler = dbenv.get_msgcall();
        panicHandler = dbenv.get_paniccall();
        backupHandler = dbenv.get_backup_handler();
        // XXX: replicationTransport and envid aren't available?

        /* Other settings */
	backup_read_count = 
	    dbenv.get_backup_config(DbConstants.DB_BACKUP_READ_COUNT);
	backup_read_sleep = 
	    dbenv.get_backup_config(DbConstants.DB_BACKUP_READ_SLEEP);
	backup_size = 
	    dbenv.get_backup_config(DbConstants.DB_BACKUP_SIZE);
	write_direct = 
	    dbenv.get_backup_config(DbConstants.DB_BACKUP_WRITE_DIRECT);
	backup_write_direct = (write_direct == 1) ? true : false;

        String blobDirStr = dbenv.get_blob_dir();
        if (blobDirStr != null)
            blobDir = new java.io.File(blobDirStr);
        blobThreshold = dbenv.get_blob_threshold();

        if (initializeCache) {
            cacheSize = dbenv.get_cachesize();
            cacheMax = dbenv.get_cache_max();
            cacheCount = dbenv.get_cachesize_ncache();
            mmapSize = dbenv.get_mp_mmapsize();
            maxOpenFiles = dbenv.get_mp_max_openfd();
            maxWrite = dbenv.get_mp_max_write();
            maxWriteSleep = dbenv.get_mp_max_write_sleep();
            mpPageSize = dbenv.get_mp_pagesize();
            mpTableSize = dbenv.get_mp_tablesize();
        }

        String createDirStr = dbenv.get_create_dir();
        if (createDirStr != null)
            createDir = new java.io.File(createDirStr);
        String[] dataDirArray = dbenv.get_data_dirs();
        if (dataDirArray == null)
            dataDirArray = new String[0];
        dataDirs = new java.util.Vector(dataDirArray.length);
        dataDirs.setSize(dataDirArray.length);
        for (int i = 0; i < dataDirArray.length; i++)
            dataDirs.set(i, new java.io.File(dataDirArray[i]));

        errorPrefix = dbenv.get_errpfx();
        errorStream = dbenv.get_error_stream();

        if (initializeLocking) {
            lockConflicts = dbenv.get_lk_conflicts();
            lockDetectMode = LockDetectMode.fromFlag(dbenv.get_lk_detect());
            lockTableSize = dbenv.get_lk_tablesize();
            lockTimeout = dbenv.get_timeout(DbConstants.DB_SET_LOCK_TIMEOUT);
            maxLocks = dbenv.get_lk_max_locks();
            maxLockers = dbenv.get_lk_max_lockers();
            maxLockObjects = dbenv.get_lk_max_objects();
            partitionLocks = dbenv.get_lk_partitions();
            txnTimeout = dbenv.get_timeout(DbConstants.DB_SET_TXN_TIMEOUT);
        } else {
            lockConflicts = null;
            lockDetectMode = LockDetectMode.NONE;
            lockTableSize = 0;
            lockTimeout = 0L;
            maxLocks = 0;
            maxLockers = 0;
            maxLockObjects = 0;
            txnTimeout = 0L;
        }
        if (initializeLogging) {
            maxLogFileSize = dbenv.get_lg_max();
            logBufferSize = dbenv.get_lg_bsize();
            logDirectory = (dbenv.get_lg_dir() == null) ? null :
                new java.io.File(dbenv.get_lg_dir());
            logFileMode = dbenv.get_lg_filemode();
            logRegionSize = dbenv.get_lg_regionmax();
        } else {
            maxLogFileSize = 0;
            logBufferSize = 0;
            logDirectory = null;
            logRegionSize = 0;
        }
        messageStream = dbenv.get_message_stream();

        // XXX: intentional information loss?
        password = (dbenv.get_encrypt_flags() == 0) ? null : "";

        // Get the initial region resource sizes.
        if (initializeLocking) {
            initResourceLocks =
                dbenv.get_memory_init(DbConstants.DB_MEM_LOCK);
            initResourceLockObjects =
                dbenv.get_memory_init(DbConstants.DB_MEM_LOCKOBJECT);
            initResourceLockers =
                dbenv.get_memory_init(DbConstants.DB_MEM_LOCKER);
        }
        if (initializeLogging)
            initResourceLogIds =
                dbenv.get_memory_init(DbConstants.DB_MEM_LOGID);
        if (transactional)
            initResourceTransactions =
                dbenv.get_memory_init(DbConstants.DB_MEM_TRANSACTION);
        initResourceThreads = dbenv.get_memory_init(DbConstants.DB_MEM_THREAD);

        regionMemoryMax = dbenv.get_memory_max();

        String metadataDirStr = dbenv.get_metadata_dir();
        if (metadataDirStr != null)
            metadataDir = new java.io.File(metadataDirStr);

        if (initializeReplication) {
            replicationClockskewFast = dbenv.rep_get_clockskew_fast();
            replicationClockskewSlow = dbenv.rep_get_clockskew_slow();
            replicationInMemory = dbenv.rep_get_config(
	        DbConstants.DB_REP_CONF_INMEM);
            replicationLimit = dbenv.rep_get_limit();
            /*
             * For the replicationNumSites field, it's sometimes illegal to
             * call rep_get_nsites, so we use 0 as a way to identify if the
             * user has changed the value.
             */
            replicationNumSites = 0;
            replicationPriority = dbenv.rep_get_priority();
            replicationRequestMin = dbenv.rep_get_request_min();
            replicationRequestMax = dbenv.rep_get_request_max();
            repmgrAckPolicy = ReplicationManagerAckPolicy.fromInt(
                dbenv.repmgr_get_ack_policy());
        } else {
            replicationLimit = 0L;
            replicationRequestMin = 0;
            replicationRequestMax = 0;
        }

        segmentId = dbenv.get_shm_key();
        mutexAlignment = dbenv.mutex_get_align();
        mutexIncrement = dbenv.mutex_get_increment();
        initMutexes = dbenv.mutex_get_init();
        maxMutexes = dbenv.mutex_get_max();
        mutexTestAndSetSpins = dbenv.mutex_get_tas_spins();
        if (transactional) {
            txnMaxActive = dbenv.get_tx_max();
            final long txnTimestampSeconds = dbenv.get_tx_timestamp();
            if (txnTimestampSeconds != 0L)
                txnTimestamp = new java.util.Date(txnTimestampSeconds * 1000);
            else
                txnTimestamp = null;
        } else {
            txnMaxActive = 0;
            txnTimestamp = null;
        }
        temporaryDirectory = new java.io.File(dbenv.get_tmp_dir());
    }
}
