#!/usr/bin/perl

use ExtUtils::Constant qw(WriteConstants); 

use constant DEFINE => 'define' ;
use constant STRING => 'string' ;
use constant IGNORE => 'ignore' ;

%constants = (


	#########
	# 2.0.3
	#########

	DBM_INSERT                 => IGNORE,
	DBM_REPLACE                => IGNORE,
	DBM_SUFFIX                 => IGNORE,
	DB_AFTER                   => DEFINE,
	DB_AM_DUP                  => IGNORE,
	DB_AM_INMEM                => IGNORE,
	DB_AM_LOCKING              => IGNORE,
	DB_AM_LOGGING              => IGNORE,
	DB_AM_MLOCAL               => IGNORE,
	DB_AM_PGDEF                => IGNORE,
	DB_AM_RDONLY               => IGNORE,
	DB_AM_RECOVER              => IGNORE,
	DB_AM_SWAP                 => IGNORE,
	DB_AM_TXN                  => IGNORE,
	DB_APP_INIT                => DEFINE,
	DB_BEFORE                  => DEFINE,
	DB_BTREEMAGIC              => DEFINE,
	DB_BTREEVERSION            => DEFINE,
	DB_BT_DELIMITER            => IGNORE,
	DB_BT_EOF                  => IGNORE,
	DB_BT_FIXEDLEN             => IGNORE,
	DB_BT_PAD                  => IGNORE,
	DB_BT_SNAPSHOT             => IGNORE,
	DB_CHECKPOINT              => DEFINE,
	DB_CREATE                  => DEFINE,
	DB_CURRENT                 => DEFINE,
	DB_DBT_INTERNAL            => IGNORE,
	DB_DBT_MALLOC              => IGNORE,
	DB_DBT_PARTIAL             => IGNORE,
	DB_DBT_USERMEM             => IGNORE,
	DB_DELETED                 => DEFINE,
	DB_DELIMITER               => DEFINE,
	DB_DUP                     => DEFINE,
	DB_EXCL                    => DEFINE,
	DB_FIRST                   => DEFINE,
	DB_FIXEDLEN                => DEFINE,
	DB_FLUSH                   => DEFINE,
	DB_HASHMAGIC               => DEFINE,
	DB_HASHVERSION             => DEFINE,
	DB_HS_DIRTYMETA            => IGNORE,
	DB_INCOMPLETE              => DEFINE,
	DB_INIT_LOCK               => DEFINE,
	DB_INIT_LOG                => DEFINE,
	DB_INIT_MPOOL              => DEFINE,
	DB_INIT_TXN                => DEFINE,
	DB_KEYEXIST                => DEFINE,
	DB_KEYFIRST                => DEFINE,
	DB_KEYLAST                 => DEFINE,
	DB_LAST                    => DEFINE,
	DB_LOCKMAGIC               => DEFINE,
	DB_LOCKVERSION             => DEFINE,
	DB_LOCK_DEADLOCK           => DEFINE,
	DB_LOCK_NOTGRANTED         => DEFINE,
	DB_LOCK_NOTHELD            => DEFINE,
	DB_LOCK_NOWAIT             => DEFINE,
	DB_LOCK_RIW_N              => DEFINE,
	DB_LOCK_RW_N               => DEFINE,
	DB_LOGMAGIC                => DEFINE,
	DB_LOGVERSION              => DEFINE,
	DB_MAX_PAGES               => DEFINE,
	DB_MAX_RECORDS             => DEFINE,
	DB_MPOOL_CLEAN             => DEFINE,
	DB_MPOOL_CREATE            => DEFINE,
	DB_MPOOL_DIRTY             => DEFINE,
	DB_MPOOL_DISCARD           => DEFINE,
	DB_MPOOL_LAST              => DEFINE,
	DB_MPOOL_NEW               => DEFINE,
	DB_MPOOL_PRIVATE           => DEFINE,
	DB_MUTEXDEBUG              => DEFINE,
	DB_NEEDSPLIT               => DEFINE,
	DB_NEXT                    => DEFINE,
	DB_NOOVERWRITE             => DEFINE,
	DB_NORECURSE               => DEFINE,
	DB_NOSYNC                  => DEFINE,
	DB_NOTFOUND                => DEFINE,
	DB_PAD                     => DEFINE,
	DB_PREV                    => DEFINE,
	DB_RDONLY                  => DEFINE,
	DB_REGISTERED              => DEFINE,
	DB_RE_MODIFIED             => IGNORE,
	DB_SEQUENTIAL              => DEFINE,
	DB_SET                     => DEFINE,
	DB_SET_RANGE               => DEFINE,
	DB_SNAPSHOT                => DEFINE,
	DB_SWAPBYTES               => DEFINE,
	DB_TEMPORARY               => DEFINE,
	DB_TRUNCATE                => DEFINE,
	DB_TXNMAGIC                => DEFINE,
	DB_TXNVERSION              => DEFINE,
	DB_TXN_BACKWARD_ROLL       => DEFINE,
	DB_TXN_FORWARD_ROLL        => DEFINE,
	DB_TXN_LOCK_2PL            => DEFINE,
	DB_TXN_LOCK_MASK           => DEFINE,
	DB_TXN_LOCK_OPTIMISTIC     => DEFINE,
	DB_TXN_LOG_MASK            => DEFINE,
	DB_TXN_LOG_REDO            => DEFINE,
	DB_TXN_LOG_UNDO            => DEFINE,
	DB_TXN_LOG_UNDOREDO        => DEFINE,
	DB_TXN_OPENFILES           => DEFINE,
	DB_TXN_REDO                => DEFINE,
	DB_TXN_UNDO                => DEFINE,
	DB_USE_ENVIRON             => DEFINE,
	DB_USE_ENVIRON_ROOT        => DEFINE,
	DB_VERSION_MAJOR           => DEFINE,
	DB_VERSION_MINOR           => DEFINE,
	DB_VERSION_PATCH           => DEFINE,
	DB_VERSION_STRING          => STRING,
	_DB_H_                     => IGNORE,
	__BIT_TYPES_DEFINED__      => IGNORE,
	const                      => IGNORE,

	# enum DBTYPE
	DB_BTREE                   => '2.0.3',
	DB_HASH                    => '2.0.3',
	DB_RECNO                   => '2.0.3',
	DB_UNKNOWN                 => '2.0.3',

	# enum db_lockop_t
	DB_LOCK_DUMP               => '2.0.3',
	DB_LOCK_GET                => '2.0.3',
	DB_LOCK_PUT                => '2.0.3',
	DB_LOCK_PUT_ALL            => '2.0.3',
	DB_LOCK_PUT_OBJ            => '2.0.3',

	# enum db_lockmode_t
	DB_LOCK_NG                 => IGNORE, # 2.0.3
	DB_LOCK_READ               => IGNORE, # 2.0.3
	DB_LOCK_WRITE              => IGNORE, # 2.0.3
	DB_LOCK_IREAD              => IGNORE, # 2.0.3
	DB_LOCK_IWRITE             => IGNORE, # 2.0.3
	DB_LOCK_IWR                => IGNORE, # 2.0.3

	# enum ACTION
	FIND                       => IGNORE, # 2.0.3
	ENTER                      => IGNORE, # 2.0.3

	#########
	# 2.1.0
	#########

	DB_NOMMAP                  => DEFINE,

	#########
	# 2.2.6
	#########

	DB_AM_THREAD               => IGNORE,
	DB_ARCH_ABS                => DEFINE,
	DB_ARCH_DATA               => DEFINE,
	DB_ARCH_LOG                => DEFINE,
	DB_LOCK_CONFLICT           => DEFINE,
	DB_LOCK_DEFAULT            => DEFINE,
	DB_LOCK_NORUN              => DEFINE,
	DB_LOCK_OLDEST             => DEFINE,
	DB_LOCK_RANDOM             => DEFINE,
	DB_LOCK_YOUNGEST           => DEFINE,
	DB_RECOVER                 => DEFINE,
	DB_RECOVER_FATAL           => DEFINE,
	DB_THREAD                  => DEFINE,
	DB_TXN_NOSYNC              => DEFINE,

	#########
	# 2.3.0
	#########

	DB_BTREEOLDVER             => DEFINE,
	DB_BT_RECNUM               => IGNORE,
	DB_FILE_ID_LEN             => DEFINE,
	DB_GETREC                  => DEFINE,
	DB_HASHOLDVER              => DEFINE,
	DB_KEYEMPTY                => DEFINE,
	DB_LOGOLDVER               => DEFINE,
	DB_RECNUM                  => DEFINE,
	DB_RECORDCOUNT             => DEFINE,
	DB_RENUMBER                => DEFINE,
	DB_RE_DELIMITER            => IGNORE,
	DB_RE_FIXEDLEN             => IGNORE,
	DB_RE_PAD                  => IGNORE,
	DB_RE_RENUMBER             => IGNORE,
	DB_RE_SNAPSHOT             => IGNORE,

	#########
	# 2.3.10
	#########

	DB_APPEND                  => DEFINE,
	DB_GET_RECNO               => DEFINE,
	DB_SET_RECNO               => DEFINE,
	DB_TXN_CKP                 => DEFINE,

	#########
	# 2.3.11
	#########

	DB_ENV_APPINIT             => DEFINE,
	DB_ENV_STANDALONE          => DEFINE,
	DB_ENV_THREAD              => DEFINE,

	#########
	# 2.3.12
	#########

	DB_FUNC_CALLOC             => IGNORE,
	DB_FUNC_CLOSE              => IGNORE,
	DB_FUNC_DIRFREE            => IGNORE,
	DB_FUNC_DIRLIST            => IGNORE,
	DB_FUNC_EXISTS             => IGNORE,
	DB_FUNC_FREE               => IGNORE,
	DB_FUNC_FSYNC              => IGNORE,
	DB_FUNC_IOINFO             => IGNORE,
	DB_FUNC_MALLOC             => IGNORE,
	DB_FUNC_MAP                => IGNORE,
	DB_FUNC_OPEN               => IGNORE,
	DB_FUNC_READ               => IGNORE,
	DB_FUNC_REALLOC            => IGNORE,
	DB_FUNC_SEEK               => IGNORE,
	DB_FUNC_SLEEP              => IGNORE,
	DB_FUNC_STRDUP             => IGNORE,
	DB_FUNC_UNLINK             => IGNORE,
	DB_FUNC_UNMAP              => IGNORE,
	DB_FUNC_WRITE              => IGNORE,
	DB_FUNC_YIELD              => IGNORE,

	#########
	# 2.3.14
	#########

	DB_TSL_SPINS               => IGNORE,

	#########
	# 2.3.16
	#########

	DB_DBM_HSEARCH             => IGNORE,
	firstkey                   => IGNORE,
	hdestroy                   => IGNORE,

	#########
	# 2.4.10
	#########

	DB_CURLSN                  => DEFINE,
	DB_FUNC_RUNLINK            => IGNORE,
	DB_REGION_ANON             => DEFINE,
	DB_REGION_INIT             => DEFINE,
	DB_REGION_NAME             => DEFINE,
	DB_TXN_LOCK_OPTIMIST       => DEFINE,
	__CURRENTLY_UNUSED         => IGNORE,

	# enum db_status_t
	DB_LSTAT_ABORTED           => IGNORE, # 2.4.10
	DB_LSTAT_ERR               => IGNORE, # 2.4.10
	DB_LSTAT_FREE              => IGNORE, # 2.4.10
	DB_LSTAT_HELD              => IGNORE, # 2.4.10
	DB_LSTAT_NOGRANT           => IGNORE, # 2.4.10
	DB_LSTAT_PENDING           => IGNORE, # 2.4.10
	DB_LSTAT_WAITING           => IGNORE, # 2.4.10

	#########
	# 2.4.14
	#########

	DB_MUTEXLOCKS              => DEFINE,
	DB_PAGEYIELD               => DEFINE,
	__UNUSED_100               => IGNORE,
	__UNUSED_4000              => IGNORE,

	#########
	# 2.5.9
	#########

	DBC_CONTINUE               => IGNORE,
	DBC_KEYSET                 => IGNORE,
	DBC_RECOVER                => IGNORE,
	DBC_RMW                    => IGNORE,
	DB_DBM_ERROR               => IGNORE,
	DB_DUPSORT                 => DEFINE,
	DB_GET_BOTH                => DEFINE,
	DB_JOIN_ITEM               => DEFINE,
	DB_NEXT_DUP                => DEFINE,
	DB_OPFLAGS_MASK            => DEFINE,
	DB_RMW                     => DEFINE,
	DB_RUNRECOVERY             => DEFINE,
	dbmclose                   => IGNORE,

	#########
	# 2.6.4
	#########

	DBC_WRITER                 => IGNORE,
	DB_AM_CDB                  => IGNORE,
	DB_ENV_CDB                 => DEFINE,
	DB_INIT_CDB                => DEFINE,
	DB_LOCK_UPGRADE            => DEFINE,
	DB_WRITELOCK               => DEFINE,

	#########
	# 2.7.1
	#########


	# enum db_lockop_t
	DB_LOCK_INHERIT            => '2.7.1',

	#########
	# 2.7.7
	#########

	DB_FCNTL_LOCKING           => DEFINE,

	#########
	# 3.0.55
	#########

	DBC_WRITECURSOR            => IGNORE,
	DB_AM_DISCARD              => IGNORE,
	DB_AM_SUBDB                => IGNORE,
	DB_BT_REVSPLIT             => IGNORE,
	DB_CONSUME                 => DEFINE,
	DB_CXX_NO_EXCEPTIONS       => DEFINE,
	DB_DBT_REALLOC             => IGNORE,
	DB_DUPCURSOR               => DEFINE,
	DB_ENV_CREATE              => DEFINE,
	DB_ENV_DBLOCAL             => DEFINE,
	DB_ENV_LOCKDOWN            => DEFINE,
	DB_ENV_LOCKING             => DEFINE,
	DB_ENV_LOGGING             => DEFINE,
	DB_ENV_NOMMAP              => DEFINE,
	DB_ENV_OPEN_CALLED         => DEFINE,
	DB_ENV_PRIVATE             => DEFINE,
	DB_ENV_SYSTEM_MEM          => DEFINE,
	DB_ENV_TXN                 => DEFINE,
	DB_ENV_TXN_NOSYNC          => DEFINE,
	DB_ENV_USER_ALLOC          => DEFINE,
	DB_FORCE                   => DEFINE,
	DB_LOCKDOWN                => DEFINE,
	DB_LOCK_RECORD             => DEFINE,
	DB_LOGFILEID_INVALID       => DEFINE,
	DB_MPOOL_NEW_GROUP         => DEFINE,
	DB_NEXT_NODUP              => DEFINE,
	DB_OK_BTREE                => DEFINE,
	DB_OK_HASH                 => DEFINE,
	DB_OK_QUEUE                => DEFINE,
	DB_OK_RECNO                => DEFINE,
	DB_OLD_VERSION             => DEFINE,
	DB_OPEN_CALLED             => DEFINE,
	DB_PAGE_LOCK               => DEFINE,
	DB_POSITION                => DEFINE,
	DB_POSITIONI               => DEFINE,
	DB_PRIVATE                 => DEFINE,
	DB_QAMMAGIC                => DEFINE,
	DB_QAMOLDVER               => DEFINE,
	DB_QAMVERSION              => DEFINE,
	DB_RECORD_LOCK             => DEFINE,
	DB_REVSPLITOFF             => DEFINE,
	DB_SYSTEM_MEM              => DEFINE,
	DB_TEST_POSTLOG            => DEFINE,
	DB_TEST_POSTLOGMETA        => DEFINE,
	DB_TEST_POSTOPEN           => DEFINE,
	DB_TEST_POSTRENAME         => DEFINE,
	DB_TEST_POSTSYNC           => DEFINE,
	DB_TEST_PREOPEN            => DEFINE,
	DB_TEST_PRERENAME          => DEFINE,
	DB_TXN_NOWAIT              => DEFINE,
	DB_TXN_SYNC                => DEFINE,
	DB_UPGRADE                 => DEFINE,
	DB_VERB_CHKPOINT           => DEFINE,
	DB_VERB_DEADLOCK           => DEFINE,
	DB_VERB_RECOVERY           => DEFINE,
	DB_VERB_WAITSFOR           => DEFINE,
	DB_WRITECURSOR             => DEFINE,
	DB_XA_CREATE               => DEFINE,

	# enum DBTYPE
	DB_QUEUE                   => '3.0.55',

	#########
	# 3.1.14
	#########

	DBC_ACTIVE                 => IGNORE,
	DBC_OPD                    => IGNORE,
	DBC_TRANSIENT              => IGNORE,
	DBC_WRITEDUP               => IGNORE,
	DB_AGGRESSIVE              => DEFINE,
	DB_AM_DUPSORT              => IGNORE,
	DB_CACHED_COUNTS           => DEFINE,
	DB_CLIENT                  => DEFINE,
	DB_DBT_DUPOK               => IGNORE,
	DB_DBT_ISSET               => IGNORE,
	DB_ENV_RPCCLIENT           => DEFINE,
	DB_GET_BOTHC               => DEFINE,
	DB_JOIN_NOSORT             => DEFINE,
	DB_NODUPDATA               => DEFINE,
	DB_NOORDERCHK              => DEFINE,
	DB_NOSERVER                => DEFINE,
	DB_NOSERVER_HOME           => DEFINE,
	DB_NOSERVER_ID             => DEFINE,
	DB_ODDFILESIZE             => DEFINE,
	DB_ORDERCHKONLY            => DEFINE,
	DB_PREV_NODUP              => DEFINE,
	DB_PR_HEADERS              => DEFINE,
	DB_PR_PAGE                 => DEFINE,
	DB_PR_RECOVERYTEST         => DEFINE,
	DB_RDWRMASTER              => DEFINE,
	DB_SALVAGE                 => DEFINE,
	DB_VERIFY_BAD              => DEFINE,
	DB_VERIFY_FATAL            => DEFINE,
	DB_VRFY_FLAGMASK           => DEFINE,

	# enum db_recops
	DB_TXN_ABORT               => '3.1.14',
	DB_TXN_BACKWARD_ROLL       => '3.1.14',
	DB_TXN_FORWARD_ROLL        => '3.1.14',
	DB_TXN_OPENFILES           => '3.1.14',

	#########
	# 3.2.9
	#########

	DBC_COMPENSATE             => IGNORE,
	DB_ALREADY_ABORTED         => DEFINE,
	DB_AM_VERIFYING            => IGNORE,
	DB_CDB_ALLDB               => DEFINE,
	DB_CONSUME_WAIT            => DEFINE,
	DB_ENV_CDB_ALLDB           => DEFINE,
	DB_EXTENT                  => DEFINE,
	DB_JAVA_CALLBACK           => DEFINE,
	DB_JOINENV                 => DEFINE,
	DB_LOCK_SWITCH             => DEFINE,
	DB_MPOOL_EXTENT            => DEFINE,
	DB_REGION_MAGIC            => DEFINE,
	DB_VERIFY                  => DEFINE,

	# enum db_lockmode_t
	DB_LOCK_WAIT               => IGNORE, # 3.2.9

	#########
	# 4.0.14
	#########

	DBC_DIRTY_READ             => IGNORE,
	DBC_MULTIPLE               => IGNORE,
	DBC_MULTIPLE_KEY           => IGNORE,
	DB_AM_DIRTY                => IGNORE,
	DB_AM_SECONDARY            => IGNORE,
	DB_APPLY_LOGREG            => DEFINE,
	DB_CL_WRITER               => DEFINE,
	DB_COMMIT                  => DEFINE,
	DB_DBT_APPMALLOC           => IGNORE,
	DB_DIRTY_READ              => DEFINE,
	DB_DONOTINDEX              => DEFINE,
	DB_EID_BROADCAST           => DEFINE,
	DB_EID_INVALID             => DEFINE,
	DB_ENV_NOLOCKING           => DEFINE,
	DB_ENV_NOPANIC             => DEFINE,
	DB_ENV_REGION_INIT         => DEFINE,
	DB_ENV_REP_CLIENT          => DEFINE,
	DB_ENV_REP_LOGSONLY        => DEFINE,
	DB_ENV_REP_MASTER          => DEFINE,
	DB_ENV_RPCCLIENT_GIVEN     => DEFINE,
	DB_ENV_YIELDCPU            => DEFINE,
	DB_FAST_STAT               => DEFINE,
	DB_GET_BOTH_RANGE          => DEFINE,
	DB_LOCK_EXPIRE             => DEFINE,
	DB_LOCK_FREE_LOCKER        => DEFINE,
	DB_LOCK_MAXLOCKS           => DEFINE,
	DB_LOCK_MINLOCKS           => DEFINE,
	DB_LOCK_MINWRITE           => DEFINE,
	DB_LOCK_SET_TIMEOUT        => DEFINE,
	DB_LOGC_BUF_SIZE           => DEFINE,
	DB_LOG_DISK                => DEFINE,
	DB_LOG_LOCKED              => DEFINE,
	DB_LOG_SILENT_ERR          => DEFINE,
	DB_MULTIPLE                => DEFINE,
	DB_MULTIPLE_KEY            => DEFINE,
	DB_NOLOCKING               => DEFINE,
	DB_NOPANIC                 => DEFINE,
	DB_PAGE_NOTFOUND           => DEFINE,
	DB_PANIC_ENVIRONMENT       => DEFINE,
	DB_REP_CLIENT              => DEFINE,
	DB_REP_DUPMASTER           => DEFINE,
	DB_REP_HOLDELECTION        => DEFINE,
	DB_REP_LOGSONLY            => DEFINE,
	DB_REP_MASTER              => DEFINE,
	DB_REP_NEWMASTER           => DEFINE,
	DB_REP_NEWSITE             => DEFINE,
	DB_REP_OUTDATED            => DEFINE,
	DB_REP_PERMANENT           => DEFINE,
	DB_REP_UNAVAIL             => DEFINE,
	DB_RPC_SERVERPROG          => DEFINE,
	DB_RPC_SERVERVERS          => DEFINE,
	DB_SECONDARY_BAD           => DEFINE,
	DB_SET_LOCK_TIMEOUT        => DEFINE,
	DB_SET_TXN_NOW             => DEFINE,
	DB_SET_TXN_TIMEOUT         => DEFINE,
	DB_STAT_CLEAR              => DEFINE,
	DB_SURPRISE_KID            => DEFINE,
	DB_TEST_POSTDESTROY        => DEFINE,
	DB_TEST_PREDESTROY         => DEFINE,
	DB_TIMEOUT                 => DEFINE,
	DB_UPDATE_SECONDARY        => DEFINE,
	DB_VERB_REPLICATION        => DEFINE,
	DB_XIDDATASIZE             => DEFINE,
	DB_YIELDCPU                => DEFINE,
	MP_FLUSH                   => IGNORE,
	MP_OPEN_CALLED             => IGNORE,
	MP_READONLY                => IGNORE,
	MP_UPGRADE                 => IGNORE,
	MP_UPGRADE_FAIL            => IGNORE,
	TXN_CHILDCOMMIT            => IGNORE,
	TXN_COMPENSATE             => IGNORE,
	TXN_DIRTY_READ             => IGNORE,
	TXN_LOCKTIMEOUT            => IGNORE,
	TXN_MALLOC                 => IGNORE,
	TXN_NOSYNC                 => IGNORE,
	TXN_NOWAIT                 => IGNORE,
	TXN_SYNC                   => IGNORE,

	# enum db_recops
	DB_TXN_APPLY               => '4.0.14',
	DB_TXN_POPENFILES          => '4.0.14',

	# enum db_lockmode_t
	DB_LOCK_DIRTY              => IGNORE, # 4.0.14
	DB_LOCK_WWRITE             => IGNORE, # 4.0.14

	# enum db_lockop_t
	DB_LOCK_GET_TIMEOUT        => '4.0.14',
	DB_LOCK_PUT_READ           => '4.0.14',
	DB_LOCK_TIMEOUT            => '4.0.14',
	DB_LOCK_UPGRADE_WRITE      => '4.0.14',

	# enum db_status_t
	DB_LSTAT_EXPIRED           => IGNORE, # 4.0.14

	#########
	# 4.1.24
	#########

	DBC_OWN_LID                => IGNORE,
	DB_AM_CHKSUM               => IGNORE,
	DB_AM_CL_WRITER            => IGNORE,
	DB_AM_COMPENSATE           => IGNORE,
	DB_AM_CREATED              => IGNORE,
	DB_AM_CREATED_MSTR         => IGNORE,
	DB_AM_DBM_ERROR            => IGNORE,
	DB_AM_DELIMITER            => IGNORE,
	DB_AM_ENCRYPT              => IGNORE,
	DB_AM_FIXEDLEN             => IGNORE,
	DB_AM_IN_RENAME            => IGNORE,
	DB_AM_OPEN_CALLED          => IGNORE,
	DB_AM_PAD                  => IGNORE,
	DB_AM_RECNUM               => IGNORE,
	DB_AM_RENUMBER             => IGNORE,
	DB_AM_REVSPLITOFF          => IGNORE,
	DB_AM_SNAPSHOT             => IGNORE,
	DB_AUTO_COMMIT             => DEFINE,
	DB_CHKSUM_SHA1             => DEFINE,
	DB_DIRECT                  => DEFINE,
	DB_DIRECT_DB               => DEFINE,
	DB_DIRECT_LOG              => DEFINE,
	DB_ENCRYPT                 => DEFINE,
	DB_ENCRYPT_AES             => DEFINE,
	DB_ENV_AUTO_COMMIT         => DEFINE,
	DB_ENV_DIRECT_DB           => DEFINE,
	DB_ENV_DIRECT_LOG          => DEFINE,
	DB_ENV_FATAL               => DEFINE,
	DB_ENV_OVERWRITE           => DEFINE,
	DB_ENV_TXN_WRITE_NOSYNC    => DEFINE,
	DB_HANDLE_LOCK             => DEFINE,
	DB_LOCK_NOTEXIST           => DEFINE,
	DB_LOCK_REMOVE             => DEFINE,
	DB_NOCOPY                  => DEFINE,
	DB_OVERWRITE               => DEFINE,
	DB_PERMANENT               => DEFINE,
	DB_PRINTABLE               => DEFINE,
	DB_RENAMEMAGIC             => DEFINE,
	DB_TEST_ELECTINIT          => DEFINE,
	DB_TEST_ELECTSEND          => DEFINE,
	DB_TEST_ELECTVOTE1         => DEFINE,
	DB_TEST_ELECTVOTE2         => DEFINE,
	DB_TEST_ELECTWAIT1         => DEFINE,
	DB_TEST_ELECTWAIT2         => DEFINE,
	DB_TEST_SUBDB_LOCKS        => DEFINE,
	DB_TXN_LOCK                => DEFINE,
	DB_TXN_WRITE_NOSYNC        => DEFINE,
	DB_WRITEOPEN               => DEFINE,
	DB_WRNOSYNC                => DEFINE,
	_DB_EXT_PROT_IN_           => IGNORE,

	# enum db_lockop_t
	DB_LOCK_TRADE              => '4.1.24',

	# enum db_status_t
	DB_LSTAT_NOTEXIST          => IGNORE, # 4.1.24

	# enum DB_CACHE_PRIORITY
	DB_PRIORITY_VERY_LOW       => '4.1.24',
	DB_PRIORITY_LOW            => '4.1.24',
	DB_PRIORITY_DEFAULT        => '4.1.24',
	DB_PRIORITY_HIGH           => '4.1.24',
	DB_PRIORITY_VERY_HIGH      => '4.1.24',

	# enum db_recops
	DB_TXN_PRINT               => '4.1.24',

	#########
	# 4.2.50
	#########

	DB_AM_NOT_DURABLE          => IGNORE,
	DB_AM_REPLICATION          => IGNORE,
	DB_ARCH_REMOVE             => DEFINE,
	DB_CHKSUM                  => DEFINE,
	DB_ENV_LOG_AUTOREMOVE      => DEFINE,
	DB_ENV_TIME_NOTGRANTED     => DEFINE,
	DB_ENV_TXN_NOT_DURABLE     => DEFINE,
	DB_FILEOPEN                => DEFINE,
	DB_INIT_REP                => DEFINE,
	DB_LOG_AUTOREMOVE          => DEFINE,
	DB_LOG_CHKPNT              => DEFINE,
	DB_LOG_COMMIT              => DEFINE,
	DB_LOG_NOCOPY              => DEFINE,
	DB_LOG_NOT_DURABLE         => DEFINE,
	DB_LOG_PERM                => DEFINE,
	DB_LOG_WRNOSYNC            => DEFINE,
	DB_MPOOL_NOFILE            => DEFINE,
	DB_MPOOL_UNLINK            => DEFINE,
	DB_NO_AUTO_COMMIT          => DEFINE,
	DB_REP_CREATE              => DEFINE,
	DB_REP_HANDLE_DEAD         => DEFINE,
	DB_REP_ISPERM              => DEFINE,
	DB_REP_NOBUFFER            => DEFINE,
	DB_REP_NOTPERM             => DEFINE,
	DB_RPCCLIENT               => DEFINE,
	DB_TIME_NOTGRANTED         => DEFINE,
	DB_TXN_NOT_DURABLE         => DEFINE,
	DB_debug_FLAG              => DEFINE,
	DB_user_BEGIN              => DEFINE,
	MP_FILEID_SET              => IGNORE,
	TXN_RESTORED               => IGNORE,

	#########
	# 4.3.21
	#########

	DBC_DEGREE_2               => IGNORE,
	DB_AM_INORDER              => IGNORE,
	DB_BUFFER_SMALL            => DEFINE,
	DB_DEGREE_2                => DEFINE,
	DB_DSYNC_LOG               => DEFINE,
	DB_DURABLE_UNKNOWN         => DEFINE,
	DB_ENV_DSYNC_LOG           => DEFINE,
	DB_ENV_LOG_INMEMORY        => DEFINE,
	DB_INORDER                 => DEFINE,
	DB_LOCK_ABORT              => DEFINE,
	DB_LOCK_MAXWRITE           => DEFINE,
	DB_LOG_BUFFER_FULL         => DEFINE,
	DB_LOG_INMEMORY            => DEFINE,
	DB_LOG_RESEND              => DEFINE,
	DB_MPOOL_FREE              => DEFINE,
	DB_REP_EGENCHG             => DEFINE,
	DB_REP_LOGREADY            => DEFINE,
	DB_REP_PAGEDONE            => DEFINE,
	DB_REP_STARTUPDONE         => DEFINE,
	DB_SEQUENCE_VERSION        => DEFINE,
	DB_SEQ_DEC                 => DEFINE,
	DB_SEQ_INC                 => DEFINE,
	DB_SEQ_RANGE_SET           => DEFINE,
	DB_SEQ_WRAP                => DEFINE,
	DB_STAT_ALL                => DEFINE,
	DB_STAT_LOCK_CONF          => DEFINE,
	DB_STAT_LOCK_LOCKERS       => DEFINE,
	DB_STAT_LOCK_OBJECTS       => DEFINE,
	DB_STAT_LOCK_PARAMS        => DEFINE,
	DB_STAT_MEMP_HASH          => DEFINE,
	DB_STAT_SUBSYSTEM          => DEFINE,
	DB_UNREF                   => DEFINE,
	DB_VERSION_MISMATCH        => DEFINE,
	TXN_DEADLOCK               => IGNORE,
	TXN_DEGREE_2               => IGNORE,

	#########
	# 4.3.28
	#########

	DB_SEQUENCE_OLDVER         => DEFINE,

	#########
	# 4.4.16
	#########

	DBC_READ_COMMITTED         => IGNORE,
	DBC_READ_UNCOMMITTED       => IGNORE,
	DB_AM_READ_UNCOMMITTED     => IGNORE,
	DB_ASSOC_IMMUTABLE_KEY     => DEFINE,
	DB_COMPACT_FLAGS           => DEFINE,
	DB_DSYNC_DB                => DEFINE,
	DB_ENV_DSYNC_DB            => DEFINE,
	DB_FREELIST_ONLY           => DEFINE,
	DB_FREE_SPACE              => DEFINE,
	DB_IMMUTABLE_KEY           => DEFINE,
	DB_MUTEX_ALLOCATED         => DEFINE,
	DB_MUTEX_LOCKED            => DEFINE,
	DB_MUTEX_LOGICAL_LOCK      => DEFINE,
	DB_MUTEX_SELF_BLOCK        => DEFINE,
	DB_MUTEX_THREAD            => DEFINE,
	DB_READ_COMMITTED          => DEFINE,
	DB_READ_UNCOMMITTED        => DEFINE,
	DB_REGISTER                => DEFINE,
	DB_REP_ANYWHERE            => DEFINE,
	DB_REP_BULKOVF             => DEFINE,
	DB_REP_CONF_BULK           => DEFINE,
	DB_REP_CONF_DELAYCLIENT    => DEFINE,
	DB_REP_CONF_NOAUTOINIT     => DEFINE,
	DB_REP_CONF_NOWAIT         => DEFINE,
	DB_REP_IGNORE              => DEFINE,
	DB_REP_JOIN_FAILURE        => DEFINE,
	DB_REP_LOCKOUT             => DEFINE,
	DB_REP_REREQUEST           => DEFINE,
	DB_SEQ_WRAPPED             => DEFINE,
	DB_THREADID_STRLEN         => DEFINE,
	DB_VERB_REGISTER           => DEFINE,
	TXN_READ_COMMITTED         => IGNORE,
	TXN_READ_UNCOMMITTED       => IGNORE,
	TXN_SYNC_FLAGS             => IGNORE,
	TXN_WRITE_NOSYNC           => IGNORE,

	# enum db_lockmode_t
	DB_LOCK_READ_UNCOMMITTED   => IGNORE, # 4.4.16

	#########
	# 4.5.20
	#########

	DBC_DONTLOCK               => IGNORE,
	DB_DBT_USERCOPY            => IGNORE,
	DB_ENV_MULTIVERSION        => DEFINE,
	DB_ENV_TXN_SNAPSHOT        => DEFINE,
	DB_EVENT_NO_SUCH_EVENT     => DEFINE,
	DB_EVENT_PANIC             => DEFINE,
	DB_EVENT_REP_CLIENT        => DEFINE,
	DB_EVENT_REP_MASTER        => DEFINE,
	DB_EVENT_REP_NEWMASTER     => DEFINE,
	DB_EVENT_REP_STARTUPDONE   => DEFINE,
	DB_EVENT_WRITE_FAILED      => DEFINE,
	DB_MPOOL_EDIT              => DEFINE,
	DB_MULTIVERSION            => DEFINE,
	DB_MUTEX_PROCESS_ONLY      => DEFINE,
	DB_REPMGR_ACKS_ALL         => DEFINE,
	DB_REPMGR_ACKS_ALL_PEERS   => DEFINE,
	DB_REPMGR_ACKS_NONE        => DEFINE,
	DB_REPMGR_ACKS_ONE         => DEFINE,
	DB_REPMGR_ACKS_ONE_PEER    => DEFINE,
	DB_REPMGR_ACKS_QUORUM      => DEFINE,
	DB_REPMGR_CONNECTED        => DEFINE,
	DB_REPMGR_DISCONNECTED     => DEFINE,
	DB_REPMGR_PEER             => DEFINE,
	DB_REP_ACK_TIMEOUT         => DEFINE,
	DB_REP_CONNECTION_RETRY    => DEFINE,
	DB_REP_ELECTION            => DEFINE,
	DB_REP_ELECTION_RETRY      => DEFINE,
	DB_REP_ELECTION_TIMEOUT    => DEFINE,
	DB_REP_FULL_ELECTION       => DEFINE,
	DB_STAT_NOERROR            => DEFINE,
	DB_TEST_RECYCLE            => DEFINE,
	DB_TXN_SNAPSHOT            => DEFINE,
	DB_USERCOPY_GETDATA        => DEFINE,
	DB_USERCOPY_SETDATA        => DEFINE,
	MP_MULTIVERSION            => IGNORE,
	TXN_ABORTED                => IGNORE,
	TXN_CDSGROUP               => IGNORE,
	TXN_COMMITTED              => IGNORE,
	TXN_PREPARED               => IGNORE,
	TXN_PRIVATE                => IGNORE,
	TXN_RUNNING                => IGNORE,
	TXN_SNAPSHOT               => IGNORE,
	TXN_XA_ABORTED             => IGNORE,
	TXN_XA_DEADLOCKED          => IGNORE,
	TXN_XA_ENDED               => IGNORE,
	TXN_XA_PREPARED            => IGNORE,
	TXN_XA_STARTED             => IGNORE,
	TXN_XA_SUSPENDED           => IGNORE,

	#########
	# 4.6.18
	#########

	DB_CKP_INTERNAL            => DEFINE,
	DB_DBT_MULTIPLE            => IGNORE,
	DB_ENV_NO_OUTPUT_SET       => DEFINE,
	DB_ENV_RECOVER_FATAL       => DEFINE,
	DB_ENV_REF_COUNTED         => DEFINE,
	DB_ENV_TXN_NOWAIT          => DEFINE,
	DB_EVENT_NOT_HANDLED       => DEFINE,
	DB_EVENT_REP_ELECTED       => DEFINE,
	DB_EVENT_REP_PERM_FAILED   => DEFINE,
	DB_IGNORE_LEASE            => DEFINE,
	DB_PREV_DUP                => DEFINE,
	DB_REPFLAGS_MASK           => DEFINE,
	DB_REP_CHECKPOINT_DELAY    => DEFINE,
	DB_REP_DEFAULT_PRIORITY    => DEFINE,
	DB_REP_FULL_ELECTION_TIMEOUT => DEFINE,
	DB_REP_LEASE_EXPIRED       => DEFINE,
	DB_REP_LEASE_TIMEOUT       => DEFINE,
	DB_SPARE_FLAG              => DEFINE,
	DB_TXN_WAIT                => DEFINE,
	DB_VERB_FILEOPS            => DEFINE,
	DB_VERB_FILEOPS_ALL        => DEFINE,

	# enum DB_CACHE_PRIORITY
	DB_PRIORITY_UNCHANGED      => '4.6.18',

	#########
	# 4.7.25
	#########

	DBC_DUPLICATE              => IGNORE,
	DB_FOREIGN_ABORT           => DEFINE,
	DB_FOREIGN_CASCADE         => DEFINE,
	DB_FOREIGN_CONFLICT        => DEFINE,
	DB_FOREIGN_NULLIFY         => DEFINE,
	DB_LOG_AUTO_REMOVE         => DEFINE,
	DB_LOG_DIRECT              => DEFINE,
	DB_LOG_DSYNC               => DEFINE,
	DB_LOG_IN_MEMORY           => DEFINE,
	DB_LOG_ZERO                => DEFINE,
	DB_MPOOL_NOLOCK            => DEFINE,
	DB_REPMGR_CONF_2SITE_STRICT => DEFINE,
	DB_REP_CONF_LEASE          => DEFINE,
	DB_REP_HEARTBEAT_MONITOR   => DEFINE,
	DB_REP_HEARTBEAT_SEND      => DEFINE,
	DB_SA_SKIPFIRSTKEY         => DEFINE,
	DB_STAT_MEMP_NOERROR       => DEFINE,
	DB_ST_DUPOK                => DEFINE,
	DB_ST_DUPSET               => DEFINE,
	DB_ST_DUPSORT              => DEFINE,
	DB_ST_IS_RECNO             => DEFINE,
	DB_ST_OVFL_LEAF            => DEFINE,
	DB_ST_RECNUM               => DEFINE,
	DB_ST_RELEN                => DEFINE,
	DB_ST_TOPLEVEL             => DEFINE,
	DB_VERB_REPMGR_CONNFAIL    => DEFINE,
	DB_VERB_REPMGR_MISC        => DEFINE,
	DB_VERB_REP_ELECT          => DEFINE,
	DB_VERB_REP_LEASE          => DEFINE,
	DB_VERB_REP_MISC           => DEFINE,
	DB_VERB_REP_MSGS           => DEFINE,
	DB_VERB_REP_SYNC           => DEFINE,
	MP_DUMMY                   => IGNORE,

	#########
	# 4.8.24
	#########

	DBC_BULK                   => IGNORE,
	DBC_DOWNREV                => IGNORE,
	DBC_FROM_DB_GET            => IGNORE,
	DBC_PARTITIONED            => IGNORE,
	DBC_WAS_READ_COMMITTED     => IGNORE,
	DB_AM_COMPRESS             => IGNORE,
	DB_CURSOR_BULK             => DEFINE,
	DB_CURSOR_TRANSIENT        => DEFINE,
	DB_DBT_BULK                => IGNORE,
	DB_DBT_STREAMING           => IGNORE,
	DB_ENV_FAILCHK             => DEFINE,
	DB_EVENT_REG_ALIVE         => DEFINE,
	DB_EVENT_REG_PANIC         => DEFINE,
	DB_FAILCHK                 => DEFINE,
	DB_GET_BOTH_LTE            => DEFINE,
	DB_GID_SIZE                => DEFINE,
	DB_LOGCHKSUM               => DEFINE,
	DB_LOGVERSION_LATCHING     => DEFINE,
	DB_MPOOL_TRY               => DEFINE,
	DB_MUTEX_SHARED            => DEFINE,
	DB_OVERWRITE_DUP           => DEFINE,
	DB_REP_CONF_INMEM          => DEFINE,
	DB_REP_PAGELOCKED          => DEFINE,
	DB_SA_UNKNOWNKEY           => DEFINE,
	DB_SET_LTE                 => DEFINE,
	DB_SET_REG_TIMEOUT         => DEFINE,
	DB_SHALLOW_DUP             => DEFINE,
	DB_VERB_REP_TEST           => DEFINE,
	DB_VERIFY_PARTITION        => DEFINE,

	#########
	# 5.0.6
	#########

	DBC_FAMILY                 => IGNORE,
	DB_EVENT_REP_DUPMASTER     => DEFINE,
	DB_EVENT_REP_ELECTION_FAILED => DEFINE,
	DB_EVENT_REP_JOIN_FAILURE  => DEFINE,
	DB_EVENT_REP_MASTER_FAILURE => DEFINE,
	DB_FORCESYNC               => DEFINE,
	DB_LOG_VERIFY_BAD          => DEFINE,
	DB_LOG_VERIFY_CAF          => DEFINE,
	DB_LOG_VERIFY_DBFILE       => DEFINE,
	DB_LOG_VERIFY_ERR          => DEFINE,
	DB_LOG_VERIFY_FORWARD      => DEFINE,
	DB_LOG_VERIFY_INTERR       => DEFINE,
	DB_LOG_VERIFY_VERBOSE      => DEFINE,
	DB_LOG_VERIFY_WARNING      => DEFINE,
	DB_REPMGR_CONF_ELECTIONS   => DEFINE,
	DB_REPMGR_ISPEER           => DEFINE,
	DB_REP_CONF_AUTOINIT       => DEFINE,
	DB_TXN_FAMILY              => DEFINE,
	DB_TXN_TOKEN_SIZE          => DEFINE,
	DB_VERB_REP_SYSTEM         => DEFINE,
	DB_VERSION_FAMILY          => DEFINE,
	DB_VERSION_FULL_STRING     => STRING,
	DB_VERSION_RELEASE         => DEFINE,
	TXN_FAMILY                 => IGNORE,
	TXN_IGNORE_LEASE           => IGNORE,
	TXN_INFAMILY               => IGNORE,
	TXN_READONLY               => IGNORE,

	# enum log_rec_type_t
	LOGREC_Done                => '5.0.6',
	LOGREC_ARG                 => '5.0.6',
	LOGREC_HDR                 => '5.0.6',
	LOGREC_DATA                => '5.0.6',
	LOGREC_DB                  => '5.0.6',
	LOGREC_DBOP                => '5.0.6',
	LOGREC_DBT                 => '5.0.6',
	LOGREC_LOCKS               => '5.0.6',
	LOGREC_OP                  => '5.0.6',
	LOGREC_PGDBT               => '5.0.6',
	LOGREC_PGDDBT              => '5.0.6',
	LOGREC_PGLIST              => '5.0.6',
	LOGREC_POINTER             => '5.0.6',
	LOGREC_TIME                => '5.0.6',

	# enum db_recops
	DB_TXN_LOG_VERIFY          => '5.0.6',

	#########
	# 5.0.32
	#########

	DBC_ERROR                  => IGNORE,
	DB_LOG_VERIFY_PARTIAL      => DEFINE,
	DB_NOERROR                 => DEFINE,

	#########
	# 5.1.3
	#########

	DB_ASSOC_CREATE            => DEFINE,
	DB_DATABASE_LOCK           => DEFINE,
	DB_DATABASE_LOCKING        => DEFINE,
	DB_ENV_DATABASE_LOCKING    => DEFINE,
	DB_ENV_HOTBACKUP           => DEFINE,
	DB_HOTBACKUP_IN_PROGRESS   => DEFINE,
	DB_LOCK_CHECK              => DEFINE,
	DB_LOG_NO_DATA             => DEFINE,
	DB_REPMGR_ACKS_ALL_AVAILABLE => DEFINE,
	DB_TXN_BULK                => DEFINE,
	TXN_BULK                   => IGNORE,

	#########
	# 5.1.18
	#########

	DB_ENV_NOFLUSH             => DEFINE,
	DB_NOFLUSH                 => DEFINE,
	DB_NO_CHECKPOINT           => DEFINE,

	#########
	# 5.2.14
	#########

	DB_ALIGN8                  => IGNORE,
	DB_BOOTSTRAP_HELPER        => DEFINE,
	DB_DBT_READONLY            => IGNORE,
	DB_EID_MASTER              => DEFINE,
	DB_EVENT_REP_CONNECT_BROKEN => DEFINE,
	DB_EVENT_REP_CONNECT_ESTD  => DEFINE,
	DB_EVENT_REP_CONNECT_TRY_FAILED => DEFINE,
	DB_EVENT_REP_INIT_DONE     => DEFINE,
	DB_EVENT_REP_LOCAL_SITE_REMOVED => DEFINE,
	DB_EVENT_REP_SITE_ADDED    => DEFINE,
	DB_EVENT_REP_SITE_REMOVED  => DEFINE,
	DB_EVENT_REP_WOULD_ROLLBACK => DEFINE,
	DB_FAILCHK_ISALIVE         => DEFINE,
	DB_GROUP_CREATOR           => DEFINE,
	DB_HEAPMAGIC               => DEFINE,
	DB_HEAPOLDVER              => DEFINE,
	DB_HEAPVERSION             => DEFINE,
	DB_HEAP_FULL               => DEFINE,
	DB_HEAP_RID_SZ             => DEFINE,
	DB_INIT_MUTEX              => DEFINE,
	DB_INTERNAL_DB             => DEFINE,
	DB_LEGACY                  => DEFINE,
	DB_LOCAL_SITE              => DEFINE,
	DB_OK_HEAP                 => DEFINE,
	DB_REPMGR_NEED_RESPONSE    => DEFINE,
	DB_REP_CONF_AUTOROLLBACK   => DEFINE,
	DB_REP_WOULDROLLBACK       => DEFINE,
	DB_STAT_ALLOC              => DEFINE,
	DB_STAT_SUMMARY            => DEFINE,
	TXN_NEED_ABORT             => IGNORE,
	TXN_XA_ACTIVE              => IGNORE,
	TXN_XA_IDLE                => IGNORE,
	TXN_XA_ROLLEDBACK          => IGNORE,
	TXN_XA_THREAD_ASSOCIATED   => IGNORE,
	TXN_XA_THREAD_NOTA         => IGNORE,
	TXN_XA_THREAD_SUSPENDED    => IGNORE,
	TXN_XA_THREAD_UNASSOCIATED => IGNORE,

	# enum DBTYPE
	DB_HEAP                    => '5.2.14',

	# enum DB_MEM_CONFIG
	DB_MEM_LOCK                => '5.2.14',
	DB_MEM_LOCKOBJECT          => '5.2.14',
	DB_MEM_LOCKER              => '5.2.14',
	DB_MEM_LOGID               => '5.2.14',
	DB_MEM_TRANSACTION         => '5.2.14',
	DB_MEM_THREAD              => '5.2.14',

	#########
	# 5.3.15
	#########

	DB2_AM_EXCL                => DEFINE,
	DB2_AM_INTEXCL             => DEFINE,
	DB2_AM_NOWAIT              => DEFINE,
	DB_AM_PARTDB               => IGNORE,
	DB_BACKUP_CLEAN            => DEFINE,
	DB_BACKUP_FILES            => DEFINE,
	DB_BACKUP_NO_LOGS          => DEFINE,
	DB_BACKUP_SINGLE_DIR       => DEFINE,
	DB_BACKUP_UPDATE           => DEFINE,
	DB_CHKSUM_FAIL             => DEFINE,
	DB_INTERNAL_PERSISTENT_DB  => DEFINE,
	DB_INTERNAL_TEMPORARY_DB   => DEFINE,
	DB_LOCK_IGNORE_REC         => DEFINE,
	DB_VERB_BACKUP             => DEFINE,
	MP_FOR_FLUSH               => IGNORE,

	# enum DB_BACKUP_CONFIG
	DB_BACKUP_READ_COUNT       => '5.3.15',
	DB_BACKUP_READ_SLEEP       => '5.3.15',
	DB_BACKUP_SIZE             => '5.3.15',
	DB_BACKUP_WRITE_DIRECT     => '5.3.15',

	#########
	# 6.0.4
	#########

	DB_DBT_BLOB                => IGNORE,
	DB_DBT_BLOB_REC            => IGNORE,
	DB_EVENT_REP_AUTOTAKEOVER_FAILED => DEFINE,
	DB_INTERNAL_BLOB_DB        => DEFINE,
	DB_LOG_BLOB                => DEFINE,
	DB_REPMGR_ISVIEW           => DEFINE,
	DB_STREAM_READ             => DEFINE,
	DB_STREAM_SYNC_WRITE       => DEFINE,
	DB_STREAM_WRITE            => DEFINE,
	DB_VERB_MVCC               => DEFINE,
	) ;

sub enum_Macro
{
    my $str = shift ;
    my ($major, $minor, $patch) = split /\./, $str ;

    my $macro = 
    "#if (DB_VERSION_MAJOR > $major) || \\\n" .
    "    (DB_VERSION_MAJOR == $major && DB_VERSION_MINOR > $minor) || \\\n" .
    "    (DB_VERSION_MAJOR == $major && DB_VERSION_MINOR == $minor && \\\n" .
    "     DB_VERSION_PATCH >= $patch)\n" ;

    return $macro;

}

sub OutputXS
{

    my @names = () ;

    foreach my $key (sort keys %constants)
    {
        my $val = $constants{$key} ;
        next if $val eq IGNORE;

        if ($val eq STRING)
          { push @names, { name => $key, type => "PV" } }
        elsif ($val eq DEFINE)
          { push @names, $key }
        else
          { push @names, { name => $key, macro => [enum_Macro($val), "#endif\n"] } }
    }

    warn "Updating constants.xs & constants.h...\n";
    WriteConstants(
              NAME    => BerkeleyDB,
              NAMES   => \@names,
              C_FILE  => 'constants.h',
              XS_FILE => 'constants.xs',
          ) ;
}

sub OutputPM
{
    my $filename = 'BerkeleyDB.pm';
    warn "Updating $filename...\n";
    open IN, "<$filename" || die "Cannot open $filename: $!\n";
    open OUT, ">$filename.tmp" || die "Cannot open $filename.tmp: $!\n";

    my $START = '@EXPORT = qw(' ;
    my $START_re = quotemeta $START ;
    my $END = ');';
    my $END_re = quotemeta $END ;

    # skip to the @EXPORT declaration
    OUTER: while (<IN>)
    {
        if ( /^\s*$START_re/ )
        {
            # skip to the end marker.
            while (<IN>) 
                { last OUTER if /^\s*$END_re/ }
        }
        print OUT ;
    }
    
    print OUT "$START\n";
    foreach my $key (sort keys %constants)
    {
        next if $constants{$key} eq IGNORE;
	print OUT "\t$key\n";
    }
    print OUT "\t$END\n";
    
    while (<IN>)
    {
        print OUT ;
    }

    close IN;
    close OUT;

    rename $filename, "$filename.bak" || die "Cannot rename $filename: $!\n" ;
    rename "$filename.tmp", $filename || die "Cannot rename $filename.tmp: $!\n" ;
}

OutputXS() ;
OutputPM() ;
