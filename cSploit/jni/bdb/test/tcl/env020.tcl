# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST    env020
# TEST    Check if the output information for stat_print is expected.
# TEST    These are the stat_print functions we test:
# TEST        env stat_print
# TEST        env lock_stat_print
# TEST        env log_stat_print
# TEST        env mpool_stat_print
# TEST        env mutex_stat_print
# TEST        env rep_stat_print
# TEST        env repmgr_stat_print
# TEST        env txn_stat_print
# TEST        db stat_print
# TEST        seq stat_print
#

proc env020 { } {
	puts "Env020: Check the output of the various stat_print"
	env020_init
	env020_env_stat_print
	env020_lock_stat_print
	env020_log_stat_print
	env020_mpool_stat_print
	env020_mutex_stat_print
	env020_rep_stat_print
	env020_repmgr_stat_print
	env020_txn_stat_print
	env020_bt_stat_print
	env020_ham_stat_print
	env020_heap_stat_print
	env020_ram_stat_print
	env020_qam_stat_print
	env020_seq_stat_print
}

# This is to create the include file for later use.
# As in this test, we define many global variables, and
# they are used in various procs. In order to avoid a 
# long list of "global XXXX", we create an include file and
# every proc could use these global variables after including
# the file.
proc env020_init_include { } {
	set f [open "./env020_include.tcl" w]
	puts $f "global section_separator"
	puts $f "global statprt_pattern"
	puts $f "global region_statprt_pattern	"
	puts $f "global lk_statprt_pattern_def"
	puts $f "global lk_statprt_pattern_params"
	puts $f "global lk_statprt_pattern_conf"
	puts $f "global lk_statprt_pattern_lockers"
	puts $f "global lk_statprt_pattern_objects"
	puts $f "global log_statprt_pattern_def"
	puts $f "global log_statprt_pattern_DBLOG"
	puts $f "global log_statprt_pattern_LOG"
	puts $f "global mp_statprt_pattern_def"
	puts $f "global mp_statprt_pattern_MPOOL"
	puts $f "global mp_statprt_pattern_DB_MPOOL"
	puts $f "global mp_statprt_pattern_DB_MPOOLFILE"
	puts $f "global mp_statprt_pattern_MPOOLFILE"
	puts $f "global mp_statprt_pattern_Cache"
	puts $f "global mut_statprt_pattern_def"
	puts $f "global mut_statprt_pattern_mutex"
	puts $f "global mut_statprt_pattern_DB_MUTEXREGION"
	puts $f "global rep_statprt_pattern_def"
	puts $f "global rep_statprt_pattern_DB_REP"
	puts $f "global rep_statprt_pattern_REP"
	puts $f "global rep_statprt_pattern_LOG"
	puts $f "global repmgr_statprt_pattern_def"
	puts $f "global repmgr_statprt_pattern_sites"	
	puts $f "global txn_statprt_pattern_def"
	puts $f "global txn_statprt_pattern_DB_TXNMGR"
	puts $f "global txn_statprt_pattern_DB_TXNREGION"
	puts $f "global env_statprt_pattern_Main"
	puts $f "global env_statprt_pattern_filehandle"
	puts $f "global env_statprt_pattern_ENV"
	puts $f "global env_statprt_pattern_DB_ENV"
	puts $f "global env_statprt_pattern_per_region"
	close $f
}

# For different regions, we show different description information.
proc env020_def_pattern {type} {
	set defpat ""
	switch $type {
		lock {
			set defpat "Default locking region information:"
		}
		log {
			set defpat "Default logging region information:"
		}
		mp {
			set defpat "Default cache region information:"
		}
		mut {
			set defpat "Default mutex region information:"
		}
		rep {
			set defpat "Default replication region information:"
		}
		txn {
			set defpat "Default transaction region information:"
		}
		env {
			set defpat "Default database environment information:"
		}
	}
	return [list $defpat]
}

# This proc initializes all the global variables mentioned before.
# These variables are regular expression patterns.
proc env020_init { } {
	env020_init_include
	source "./env020_include.tcl"

	set section_separator {
		"=-=-=-=-=-=-=-=-=-=-=-=-="
	}

	set region_statprt_pattern {
		"REGINFO information"
		"Region type"
		"Region ID"
		"Region name"
		"Region address"
		"Region allocation head"
		"Region primary address"
		"Region maximum allocation"
		"Region allocated"
		"Region allocations.*allocations.*failures.*frees.*longest"
		"Allocations by power-of-two sizes"
		{\s*\d*\s*KB}
		"Region flags"
	}

	set lk_statprt_pattern_def {
		# DIAGNOSTIC Information
		"Hash bucket"
		"Partition"
		"The number of partition mutex requests that required waiting"
		"Maximum hash bucket length"
		"Total number of locks requested"
		"Total number of locks released"
		"Total number of locks upgraded"
		"Total number of locks downgraded"
		"Lock requests not available due to conflicts, for which we waited"
		"Lock requests not available due to conflicts, for which we did not wait"
		"Number of locks that have timed out"
		"Number of transactions that have timed out"
		# Default Lock information
		"Last allocated locker ID"
		"Current maximum unused locker ID"
		"Number of lock modes"
		"Initial number of locks allocated"
		"Initial number of lockers allocated"
		"Initial number of lock objects allocated"
		"Maximum number of locks possible"
		"Maximum number of lockers possible"
		"Maximum number of lock objects possible"
		"Current number of locks allocated"
		"Current number of lockers allocated"
		"Current number of lock objects allocated"
		"Size of object hash table"
		"Number of lock object partitions"
		"Number of current locks"
		"Number of locks that have timed out"
		"Transaction timeout value"
		"The number of region locks that required waiting"
		"Maximum number of locks at any one time"
		"Maximum number of locks in any one bucket"
		"Maximum number of locks stolen by for an empty partition"
		"Maximum number of locks stolen for any one partition"
		"Number of current lockers"
		"Maximum number of lockers at any one time"
		"Number of hits in the thread locker cache"
		"Total number of lockers reused"
		"Number of current lock objects"
		"Maximum number of lock objects at any one time"
		"Maximum number of lock objects in any one bucket"
		"Maximum number of objects stolen by for an empty partition"
		"Maximum number of objects stolen for any one partition"
		"Total number of locks requested"
		"Total number of locks released"
		"Total number of locks upgraded"
		"Total number of locks downgraded"
		"Lock requests not available due to conflicts, for which we waited"
		"Lock requests not available due to conflicts, for which we did not wait"
		"Number of deadlocks"
		"Lock timeout value"
		"Number of transactions that have timed out"
		"Region size"
		"The number of partition locks that required waiting"
		"The maximum number of times any partition lock was waited for"
		"The number of object queue operations that required waiting"
		"The number of locker allocations that required waiting"
		"Maximum hash bucket length"
	}

	set lk_statprt_pattern_params {
		"Lock region parameters"
		"Lock region region mutex"
		"locker table size"
		"object table size"
		"obj_off"
		"locker_off"
		"need_dd"
		"next_timeout:"
	}

	# Need to check why it is an empty table.
	set lk_statprt_pattern_conf {
		"Lock conflict matrix"
	}

	set lk_statprt_pattern_lockers {
		"Locks grouped by lockers"
		{Locker.*Mode.*Count Status.*Object}
		{locks held.*locks.*pid/thread.*priority}
		"expires"
		"lk timeout"
		"lk expires"
		{READ|WRITE|IWR|IWRITE|NG|READ_UNCOMMITTED|WAS_WRITE|WAIT|UNKNOWN}
	}

	set lk_statprt_pattern_objects {
		"Locks grouped by object"
		{Locker.*Mode.*Count Status.*Object}
		{READ|WRITE|IWR|IWRITE|NG|READ_UNCOMMITTED|WAS_WRITE|WAIT|UNKNOWN}
		{^$}
	}

	set log_statprt_pattern_def {
		"Log magic number"
		"Log version number"
		"Log record cache size"
		"Log file mode"
		"Current log file size"
		"Initial fileid allocation"
		"Current fileids in use"
		"Maximum fileids used"
		"Records entered into the log"
		"Log bytes written"
		"Log bytes written since last checkpoint"
		"Total log file I/O writes"
		"Total log file I/O writes due to overflow"
		"Total log file flushes"
		"Total log file I/O reads"
		"Current log file number"
		"Current log file offset"
		"On-disk log file number"
		"On-disk log file offset"
		"Maximum commits in a log flush"
		"Minimum commits in a log flush"
		"Region size"
		"The number of region locks that required waiting"
	}

	set log_statprt_pattern_DBLOG {
		"DB_LOG handle information"
		"DB_LOG handle mutex"
		"Log file name"
		# File handle information
		"Log file handle"
		"file-handle.file name"
		"file-handle.mutex"
		"file-handle.reference count"
		"file-handle.file descriptor"
		"file-handle.page number"
		"file-handle.page size"
		"file-handle.page offset"
		"file-handle.seek count"
		"file-handle.read count"
		"file-handle.write count"
		"file-handle.flags"
		"Flags"
	}

	set log_statprt_pattern_LOG {
		"LOG handle information"
		"LOG region mutex"
		"File name list mutex"
		"persist.magic"
		"persist.version"
		"persist.log_size"
		"log file permissions mode"
		"current file offset LSN"
		"first buffer byte LSN"
		"current buffer offset"
		"current file write offset"
		"length of last record"
		"log flush in progress"
		"Log flush mutex"
		"last sync LSN"
		"cached checkpoint LSN"
		"log buffer size"
		"log file size"
		"next log file size"
		"transactions waiting to commit"
		"LSN of first commit"
	}

	set mp_statprt_pattern_def {
		"Total cache size"
		"Number of caches"
		"Maximum number of caches"
		"Pool individual cache size"
		"Maximum memory-mapped file size"
		"Maximum open file descriptors"
		"Maximum sequential buffer writes"
		"Sleep after writing maximum sequential buffers"
		"Pool File:"
		"Page size"
		"Requested pages mapped into the process' address space"
		"Requested pages found in the cache"
		"Requested pages not found in the cache"
		"Pages created in the cache"
		"Pages read into the cache"
		"Pages written from the cache to the backing file"
		"Clean pages forced from the cache"
		"Dirty pages forced from the cache"
		"Dirty pages written by trickle-sync thread"
		"Current total page count"
		"Current clean page count"
		"Current dirty page count"
		"Number of hash buckets used for page location"
		"Number of mutexes for the hash buckets"
		"Assumed page size used"
		"Total number of times hash chains searched for a page"
		"The longest hash chain searched for a page"
		"Total number of hash chain entries checked for page"
		"The number of hash bucket locks that required waiting"
		"The maximum number of times any hash bucket lock was waited for"
		"The number of region locks that required waiting"
		"The number of buffers frozen"
		"The number of buffers thawed"
		"The number of frozen buffers freed"
		"The number of outdated intermediate versions reused"
		"The number of page allocations"
		"The number of hash buckets examined during allocations"
		"The maximum number of hash buckets examined for an allocation"
		"The number of pages examined during allocations"
		"The max number of pages examined for an allocation"
		"Threads waited on page I/O"
		"The number of times a sync is interrupted"
		# Pool file information
		"Pool File"
		"Page size"
		"Requested pages mapped into the process' address space"
		"Requested pages found in the cache"
		"Requested pages not found in the cache"
		"Pages created in the cache"
		"Pages read into the cache"
		"Pages written from the cache to the backing file"		
	}

	set mp_statprt_pattern_MPOOL {
		"MPOOL structure"
		"MPOOL region mutex"
		"Maximum checkpoint LSN"
		"Hash table entries"
		"Hash table mutexes"
		"Hash table last-checked"
		"Hash table LRU priority"
 		"Hash table LRU generation"
		"Put counter"
	}

	set mp_statprt_pattern_DB_MPOOL {
		"DB_MPOOL handle information"
		"DB_MPOOL handle mutex"
		"Underlying cache regions"
	}

	set mp_statprt_pattern_DB_MPOOLFILE {
		"DB_MPOOLFILE structures"
		{File #\d*}
		"Reference count"
		"Pinned block reference count"
		"Clear length"
		"ID"
		"File type"
		"LSN offset"
		"Max gbytes"
		"Max bytes"
		"Cache priority"
		"mmap address"
		"mmap length"
		"Flags"
		"File handle"
		# File handle information
		"file-handle.file name"
		"file-handle.mutex"
		"file-handle.reference count"
		"file-handle.file descriptor"
		"file-handle.page number"
		"file-handle.page size"
		"file-handle.page offset"
		"file-handle.seek count"
		"file-handle.read count"
		"file-handle.write count"
		"file-handle.flags"
	}

	set mp_statprt_pattern_MPOOLFILE {
		"MPOOLFILE structures"
		{File #\d*}
		"Mutex"
		"Revision count"
		"Reference count"
		"Sync open count"
		"Sync/read only open count"
		"Block count"
		"Last page number"
		"Original last page number"
		"Maximum page number"
		"Type"
		"Priority"
		"Page's LSN offset"
		"Page's clear length"
		"ID"
		"Flags"
	}

	set mp_statprt_pattern_Cache {
		{Cache #\d*}
		"BH hash table"
		{^bucket \d*:.*}
		"pageno, file, ref, LSN, address, priority, flags"
		{\d*, #\d*,\s*\d*,\s*\d*/\d*, 0[xX][0-9a-fA-F]*, \d*}
		{free frozen \d* pgno \d* mtx_buf \d*}
	}

	set mut_statprt_pattern_def {
		# General mutex information
		"Mutex region size"
		"The number of region locks that required waiting"
		"Mutex alignment"
		"Mutex test-and-set spins"
		"Mutex total count"
		"Mutex free count"
		"Mutex in-use count"
		"Mutex maximum in-use count"
		"Mutex counts"
		"Unallocated"
		# Mutex type
		"application allocated"
		"atomic emulation"
		"db handle"
		"env dblist"
		"env handle"
		"env region"
		"lock region"
		"logical lock"
		"log filename"
		"log flush"
		"log handle"
		"log region"
		"mpoolfile handle"
		"mpool buffer"
		"mpool filehandle"
		"mpool file bucket"
		"mpool handle"
		"mpool hash bucket"
		"mpool region"
		"mutex region"
		"mutex test"
		"replication manager"		
		"replication checkpoint"
		"replication database"
		"replication diagnostics"
		"replication event"
		"replication region"
		"replication role config"
		"replication txn apply"
		"sequence"
		"twister"
		"Tcl events"
		"txn active list"
		"transaction checkpoint"
		"txn commit"
		"txn mvcc"
		"txn region"
		"unknown mutex type"
	}

	set mut_statprt_pattern_DB_MUTEXREGION {
		"DB_MUTEXREGION structure"
		"DB_MUTEXREGION region mutex"
		"Size of the aligned mutex"
		"Next free mutex"
	}

	set mut_statprt_pattern_mutex {
		"wait/nowait, pct wait, holder, flags"
	}

	set rep_statprt_pattern_def {
		"Environment configured as a replication master"
		"Environment configured as a replication client"
		"Environment not configured as view site"
		"Environment not configured for replication"
		"Next LSN to be used"
		"Next LSN expected"
		"Not waiting for any missed log records"
		"LSN of first log record we have after missed log records"
		"No maximum permanent LSN"
		"Maximum permanent LSN"
		"Next page number expected"
		"Not waiting for any missed pages"
		"Page number of first page we have after missed pages"
		"Number of duplicate master conditions originally detected at this site"
		"Current environment ID"
		"No current environment ID"
		"Current environment priority"
		"Current generation number"
		"Election generation number for the current or next election"
		"Number of duplicate log records received"
		"Number of log records currently queued"
		"Maximum number of log records ever queued at once"
		"Total number of log records queued"
		"Number of log records received and appended to the log"
		"Number of log records missed and requested"
		"Current master ID"
		"No current master ID"
		"Number of times the master has changed"
		"Number of messages received with a bad generation number"
		"Number of messages received and processed"
		"Number of messages ignored due to pending recovery"
		"Number of failed message sends"
		"Number of messages sent"
		"Number of new site messages received"
		"Number of environments used in the last election"
		"Transmission limited"
		"Number of outdated conditions detected"
		"Number of duplicate page records received"
		"Number of page records received and added to databases"
		"Number of page records missed and requested"
		"Startup incomplete"
		"Startup complete"
		"Number of transactions applied"
		"Number of startsync messages delayed"
		"Number of elections held"
		"Number of elections won"
		"No election in progress"
		"Duration of last election"
		"Current election phase"
		"Environment ID of the winner of the current or last election"
		"Master generation number of the winner of the current or last election"
		"Master data generation number of the winner of the current or last election"
		"Maximum LSN of the winner of the current or last election"
		"Number of sites responding to this site during the current election"
		"Number of votes required in the current or last election"
		"Priority of the winner of the current or last election"
		"Tiebreaker value of the winner of the current or last election"
		"Number of votes received during the current election"
		"Number of bulk buffer sends triggered by full buffer"
		"Number of single records exceeding bulk buffer size"
		"Number of records added to a bulk buffer"
		"Number of bulk buffers sent"
		"Number of re-request messages received"
		"Number of request messages this client failed to process"
		"Number of request messages received by this client"
		"Duration of maximum lease"
		"Number of lease validity checks"
		"Number of invalid lease validity checks"
		"Number of lease refresh attempts during lease validity checks"
		"Number of live messages sent while using leases"
	}

	# This needs to concat with db_print information.
	set rep_statprt_pattern_DB_REP {
		"DB_REP handle information"
		"Bookkeeping database"
		"Flags"
	}

	set rep_statprt_pattern_REP {
		"REP handle information"
		"Replication region mutex"
		"Bookkeeping database mutex"
		"Environment ID"
		"Master environment ID"
		"Election generation"
		"Last active egen"
		"Master generation"
		"Space allocated for sites"
		"Sites in group"
		"Votes needed for election"
		"Priority in election"
		"Limit on data sent in a single call"
		"Request gap seconds"
		"Request gap microseconds"
		"Maximum gap seconds"
		"Maximum gap microseconds"
		"Callers in rep_proc_msg"
		"Callers in rep_elect"
		"Library handle count"
		"Multi-step operation count"
		"Recovery timestamp"
		"Sites heard from"
		"Current winner"
		"Winner priority"
		"Winner generation"
		"Winner data generation"
		"Winner LSN"
		"Winner tiebreaker"
		"Votes for this site"
		"Synchronization State"
		"Config Flags"
		"Elect Flags"
		"Lockout Flags"
		"Flags"
	}

	set rep_statprt_pattern_LOG {
		"LOG replication information"
		"First log record after a gap"
		"Maximum permanent LSN processed"
		"LSN waiting to verify"
		"Maximum LSN requested"
		"Time to wait before requesting seconds"
		"Time to wait before requesting microseconds"
		"Next LSN expected"
		"Maximum lease timestamp seconds"
		"Maximum lease timestamp microseconds"
	}

	set repmgr_statprt_pattern_def {
		"Number of PERM messages not acknowledged"
		"Number of messages queued due to network delay"
		"Number of messages discarded due to queue length"
		"Number of existing connections dropped"
		"Number of failed new connection attempts"
		"Number of currently active election threads"
		"Election threads for which space is reserved"
		"Number of participant sites in replication group"
		"Total number of sites in replication group"
		"Number of view sites in replication group"
		"Number of automatic replication process takeovers"
		"Size of incoming message queue"
	}

	set repmgr_statprt_pattern_sites {
		"DB_REPMGR site information:"
		"eid:.*port:.*"
	}

	set txn_statprt_pattern_def {
		"No checkpoint LSN"
		"File/offset for last checkpoint LSN"
		"Checkpoint timestamp"
		"No checkpoint timestamp"
		"Last transaction ID allocated"
		"Maximum number of active transactions configured"
		"Active transactions"
		"Maximum active transactions"
		"Number of transactions begun"
		"Number of transactions aborted"
		"Number of transactions committed"
		"Snapshot transactions"
		"Maximum snapshot transactions"
		"Number of transactions restored"
		"Region size"
		"The number of region locks that required waiting"
		# Information for Active transactions
		"Active transactions"
		"running.*begin LSN"
	}

	set txn_statprt_pattern_DB_TXNMGR {
		"DB_TXNMGR handle information"
		"DB_TXNMGR mutex"
		"Number of transactions discarded"
	}

	set txn_statprt_pattern_DB_TXNREGION {
		"DB_TXNREGION handle information"
		"DB_TXNREGION region mutex"
		"Maximum number of active txns"
		"Last transaction ID allocated"
		"Current maximum unused ID"
		"checkpoint mutex"
		"Last checkpoint LSN"
		"Last checkpoint timestamp"
		"Flags"
	}

	set env_statprt_pattern_Main {
		"Local time"
		"Magic number"
		"Panic value"
		"Environment version"
		"Btree version"
		"Hash version"
		"Lock version"
		"Log version"
		"Queue version"
		"Sequence version"
		"Txn version"
		"Creation time"
		"Environment ID"
		"Primary region allocation and reference count mutex"
		"References"
		"Current region size"
		"Maximum region size"
	}

	set env_statprt_pattern_filehandle {
		"Environment file handle information"
		# File handle Information.
		"file-handle.file name"
		"file-handle.mutex"
		"file-handle.reference count"
		"file-handle.file descriptor"
		"file-handle.page number"
		"file-handle.page size"
		"file-handle.page offset"
		"file-handle.seek count"
		"file-handle.read count"
		"file-handle.write count"
		"file-handle.flags"
	}

	set env_statprt_pattern_ENV {
		"ENV"
		"DB_ENV handle mutex"
		"Errcall"
		"Errfile"
		"Errpfx"
		"Msgfile"
		"Msgcall"
		"AppDispatch"
		"Event"
		"Feedback"
		"Free"
		"Panic"
		"Malloc"
		"Realloc"
		"IsAlive"
		"ThreadId"
		"ThreadIdString"
		"Blob dir"
		"Log dir"
		"Metadata dir"
		"Tmp dir"
		"Data dir"
		"Intermediate directory mode"
		"Shared memory key"
		"Password"
		"Blob threshold"
		"App private"
		"Api1 internal"
		"Api2 internal"
		"Verbose flags"
		"Mutex align"
		"Mutex cnt"
		"Mutex inc"
		"Mutex tas spins"
		"Lock conflicts"
		"Lock modes"
		"Lock detect"
		"Lock init"
		"Lock init lockers"
		"Lock init objects"
		"Lock max"
		"Lock max lockers"
		"Lock max objects"
		"Lock partitions"
		"Lock object hash table size"
		"Lock timeout"
		"Log bsize"
		"Log file mode"
		"Log region max"
		"Log size"
		"Cache GB"
		"Cache B"
		"Cache max GB"
		"Cache max B"
		"Cache mmap size"
		"Cache max open fd"
		"Cache max write"
		"Cache number"
		"Cache max write sleep"
		"Txn init"
		"Txn max"
		"Txn timestamp"
		"Txn timeout"
		"Thread count"
		"Registry"
		"Registry offset"
		"Registry timeout"
		"Public environment flags"
	}

	set env_statprt_pattern_DB_ENV {
		"DB_ENV"
		"ENV handle mutex"
		"Home"
		"Open flags"
		"Mode"
		"Pid cache"
		"Lockfhp"
		"Locker"
		"Internal recovery table"
		"Number of recovery table slots"
		"External recovery table"
		"Number of recovery table slots"
		"Thread hash buckets"
		"Thread hash table"
		"Mutex initial count"
		"Mutex initial max"
		"ENV list of DB handles mutex"
		"DB reference count"
		"MT mutex"
		"Crypto handle"
		"Lock handle"
		"Log handle"
		"Cache handle"
		"Mutex handle"
		"Replication handle"
		"Txn handle"
		"User copy"
		"Test abort"
		"Test check"
		"Test copy"
		"Private environment flags"
	}

	set env_statprt_pattern_per_region {
		"Per region database environment information"
		{.*\sRegion:}
		"Region ID"
		"Segment ID"
		"Size"
		"Initialization flags"
		"Region slots"
		"Replication flags"
		"Operation timestamp"
		"Replication timestamp"
	}
}

proc env020_lock_stat_print { } {
	source "./env020_include.tcl"

	set opts {"" "-clear" "-lk_conf" "-lk_lockers" "-lk_objects"
	    "-lk_params" "-all"}
	set patterns [list $lk_statprt_pattern_def $lk_statprt_pattern_def \
	    [concat $section_separator $region_statprt_pattern \
	    $lk_statprt_pattern_conf] \
	    [concat $section_separator $region_statprt_pattern \
	    $lk_statprt_pattern_lockers] \
	    [concat $section_separator $region_statprt_pattern \
	    $lk_statprt_pattern_objects] \
	    [concat $section_separator $region_statprt_pattern \
	    $lk_statprt_pattern_params] \
	    [concat $section_separator [env020_def_pattern lock] \
	    $region_statprt_pattern $lk_statprt_pattern_def \
	    $lk_statprt_pattern_conf $lk_statprt_pattern_lockers \
	    $lk_statprt_pattern_objects $lk_statprt_pattern_params]]
	set check_type lock_stat_print
	set stp_method lock_stat_print

	env020_env_stp_chk $opts $patterns $check_type $stp_method
}

proc env020_log_stat_print { } {
	source "./env020_include.tcl"

	set opts {"" "-clear" "-all"}
	set patterns [list $log_statprt_pattern_def $log_statprt_pattern_def \
	    [concat $section_separator [env020_def_pattern log] \
	    $region_statprt_pattern $log_statprt_pattern_def \
	    $log_statprt_pattern_DBLOG $log_statprt_pattern_LOG]]
	set check_type log_stat_print
	set stp_method log_stat_print

	env020_env_stp_chk $opts $patterns $check_type $stp_method
}

proc env020_mpool_stat_print { } {
	source "./env020_include.tcl"

	set opts {"" "-clear" "-hash" "-all"}
	set patterns [list $mp_statprt_pattern_def $mp_statprt_pattern_def \
	    [concat $section_separator $region_statprt_pattern \
	    $mp_statprt_pattern_MPOOL $mp_statprt_pattern_DB_MPOOL \
	    $mp_statprt_pattern_DB_MPOOLFILE $mp_statprt_pattern_MPOOLFILE \
	    $mp_statprt_pattern_Cache] \
	    [concat $section_separator $region_statprt_pattern \
	    $mp_statprt_pattern_MPOOL $mp_statprt_pattern_DB_MPOOL \
	    $mp_statprt_pattern_DB_MPOOLFILE $mp_statprt_pattern_MPOOLFILE \
	    $mp_statprt_pattern_Cache $mp_statprt_pattern_def \
	    [env020_def_pattern mp]]]
	set check_type mpool_stat_print
	set stp_method mpool_stat_print

	env020_env_stp_chk $opts $patterns $check_type $stp_method
}

proc env020_mutex_stat_print { } {
	source "./env020_include.tcl"

	set opts {"" "-clear" "-all"}
	set patterns [list $mut_statprt_pattern_def $mut_statprt_pattern_def \
	    [concat $section_separator $region_statprt_pattern \
	    [env020_def_pattern mut] $mut_statprt_pattern_def \
	    $mut_statprt_pattern_mutex $mut_statprt_pattern_DB_MUTEXREGION]]
	set check_type mutex_stat_print
	set stp_method mutex_stat_print

	env020_env_stp_chk $opts $patterns $check_type $stp_method
}

proc env020_rep_stat_print { } {
	source "./env020_include.tcl"

	set opts {"" "-clear" "-all"}
	set patterns [list $rep_statprt_pattern_def $rep_statprt_pattern_def \
	    [concat $section_separator [env020_def_pattern rep] \
	    $region_statprt_pattern $rep_statprt_pattern_def \
	    $rep_statprt_pattern_DB_REP $rep_statprt_pattern_REP \
	    $rep_statprt_pattern_LOG]]
	set check_type rep_stat_print
	set stp_method rep_stat_print

	env020_env_stp_chk $opts $patterns $check_type $stp_method
}

proc env020_repmgr_stat_print { } {
	source "./env020_include.tcl"

	set opts {"" "-clear" "-all"}
	set patterns [list [concat $repmgr_statprt_pattern_def \
	    $repmgr_statprt_pattern_sites] $repmgr_statprt_pattern_def \
	    [concat $repmgr_statprt_pattern_def $repmgr_statprt_pattern_sites]]
	set check_type repmgr_stat_print
	set stp_method repmgr_stat_print

	env020_env_stp_chk $opts $patterns $check_type $stp_method
}

proc env020_txn_stat_print { } {
	source "./env020_include.tcl"

	set opts {"" "-clear" "-all"}
	set patterns [list $txn_statprt_pattern_def $txn_statprt_pattern_def \
	    [concat $section_separator [env020_def_pattern txn] \
	    $region_statprt_pattern $txn_statprt_pattern_def \
	    $txn_statprt_pattern_DB_TXNMGR $txn_statprt_pattern_DB_TXNREGION]]
	set check_type txn_stat_print
	set stp_method txn_stat_print

	env020_env_stp_chk $opts $patterns $check_type $stp_method
}

proc env020_env_stat_print { } {
	source "./env020_include.tcl"

	set opts {"" "-clear" "-all" "-subsystem"}
	set patterns [list \
	    [concat $env_statprt_pattern_Main $section_separator \
	    $env_statprt_pattern_filehandle] \
	    [concat $env_statprt_pattern_Main $section_separator \
	    $env_statprt_pattern_filehandle] \
	    [concat $section_separator [env020_def_pattern env] \
	    $region_statprt_pattern $env_statprt_pattern_Main \
	    $env_statprt_pattern_filehandle $env_statprt_pattern_ENV \
	    $env_statprt_pattern_DB_ENV $env_statprt_pattern_per_region] \
	    [concat $section_separator $env_statprt_pattern_Main \
	    $env_statprt_pattern_filehandle $log_statprt_pattern_def \
	    $lk_statprt_pattern_def $mp_statprt_pattern_def \
	    $rep_statprt_pattern_def $repmgr_statprt_pattern_def \
	    $txn_statprt_pattern_def $mut_statprt_pattern_def]]
	set check_type stat_print
	set stp_method stat_print

	env020_env_stp_chk $opts $patterns $check_type $stp_method
}


# Proc to check the patterns and lines in the message file.
# The basic flow is that: 
#   1 Create an environment, some transactions and databases.
#   2 Run specified stat_print with various options, the message
#     for different options will go to different message files.
#   3 Check each line in the message files to see if it matches
#     any of the specified patterns.
# Notice that, when we change the message file, the previous one
# will be flushed and closed.
proc env020_env_stp_chk {opts patterns check_type stp_method} {
	source ./include.tcl
	set envarg {-create -txn -lock -log -rep}
	set extopts {
		{""}
		{"-thread"}
		{"-private" {"mutex_stat_print" "requires.*mutex.*subsystem"}}
		{"-thread -private"}
	}

	foreach extopt $extopts {
		set extarg [lindex $extopt 0]
		set failmsg ""
		set fail 0
		if {[llength $extopt] > 1} {
			set len [llength $extopt]
			for {set i 1} {$i < $len} {incr i} {
				set item [lindex $extopt $i]
				set stp [lindex $item 0]
				if {$stp == $stp_method} {
					set failmsg [lindex $item 1]
					set fail 1
					break
				}
			}
		}

		puts "\tEnv020: Check DB_ENV->$stp_method ($envarg $extarg)"
		env_cleanup $testdir
		# Open the env
		set env [eval berkdb_env_noerr $envarg $extarg\
		    -home $testdir -msgfile $testdir/msgfile]
		error_check_good is_valid_env [is_valid_env $env] TRUE

		# Create two txns
		set txn1 [$env txn]
		error_check_good is_vaild_txn [is_valid_txn $txn1 $env] TRUE
		set txn2 [$env txn]
		error_check_good is_valid_txn [is_valid_txn $txn2 $env] TRUE

		# Open 4 dbs
		set db1 [berkdb_open_noerr -create -env $env \
		    -btree -auto_commit db1.db]
		error_check_good is_valid_db [is_valid_db $db1] TRUE
		set db2 [berkdb_open_noerr -create -env $env \
		    -btree -auto_commit db2.db "subdb1"]
		error_check_good is_valid_db [is_valid_db $db2] TRUE
		set db3 [berkdb_open_noerr -create -env $env \
		    -btree -auto_commit "" "subdb1"]
		error_check_good is_valid_db [is_valid_db $db3] TRUE
		set db4 [berkdb_open_noerr -create -env $env \
		    -btree -auto_commit "" ""]
		error_check_good is_valid_db [is_valid_db $db4] TRUE

		# Call txn_checkpoint
		error_check_good txn_chkpt [$env txn_checkpoint] 0

		set len [llength $opts]
		for {set i 0} {$i < $len} {incr i} {
			set opt [lindex $opts $i]
			if {$opt == ""} {
				puts "\t\tUsing the default option"
			} else {
				puts "\t\tUsing $opt option"
			}
			set pattern [lindex $patterns $i]
			$env msgfile $testdir/msgfile.$i
			if {$fail == 0} {
				error_check_good "${check_type}($opts)" \
				    [eval $env $stp_method $opt] 0
				$env msgfile /dev/stdout
				env020_check_output $pattern $testdir/msgfile.$i
			} else {
				set ret [catch {eval $env $stp_method $opt} res]
				$env msgfile /dev/stdout
				error_check_bad $stp_method $ret 0
				error_check_bad chk_err [regexp $failmsg $res] 0
			}
			file delete -force $testdir/msgfile.$i
			error_check_good "file_not_exists" \
			    [file exists $testdir/msgfile.$i] 0
		}

		error_check_good "$txn1 commit" [$txn1 commit] 0
		error_check_good "$txn2 commit" [$txn2 commit] 0
		error_check_good "$db4 close" [$db4 close] 0
		error_check_good "$db3 close" [$db3 close] 0
		error_check_good "$db2 close" [$db2 close] 0
		error_check_good "$db1 close" [$db1 close] 0
		error_check_good "$env close" [$env close] 0
	}
}

proc env020_check_output {pattern msgfile} {
	set f [open $msgfile r]
	set failed 0
	while {[gets $f line] >= 0} {
		set line_found 0
		foreach pat $pattern {
			if {[regexp $pat $line] != 0} {
				set line_found 1
				break
			}
		}
		if {$line_found == 0} {
			puts "BAD STAT STRING: $line"
			set failed 1
		}
	}
	close $f
	return $failed
}

proc env020_bt_stat_print {} {
	set pattern {
		"Local time"
		# Btree information
		"Btree magic number"
		"Btree version number"
		"Byte order"
		"Flags"
		"Minimum keys per-page"
		"Underlying database page size"
		"Overflow key/data size"
		"Number of levels in the tree"
		"Number of unique keys in the tree"
		"Number of data items in the tree"
		"Number of blobs in the tree"
		"Number of tree internal pages"
		"Number of bytes free in tree internal pages"
		"Number of tree leaf pages"
		"Number of bytes free in tree leaf pages"
		"Number of tree duplicate pages"
		"Number of bytes free in tree duplicate pages"
		"Number of tree overflow pages"
		"Number of bytes free in tree overflow pages"
		"Number of empty pages"
		"Number of pages on the free list"
	}

	set all_pattern {
		"Default Btree/Recno database information"
		# Btree cursor information
		"Overflow size"
		"Order"
		"Internal Flags"
	}

	puts "\tEnv020: Check DB->stat_print for btree"
	env020_db_stat_print btree $pattern $all_pattern
}

proc env020_ham_stat_print {} {
	set pattern {
		"Local time"
		# Hash information
		"Hash magic number"
		"Hash version number"
		"Byte order"
		"Flags"
		"Number of pages in the database"
		"Underlying database page size"
		"Specified fill factor"
		"Number of keys in the database"
		"Number of data items in the database"
		"Number of hash buckets"
		"Number of bytes free on bucket pages"
		"Number of blobs"
		"Number of overflow pages"
		"Number of bytes free in overflow pages"
		"Number of bucket overflow pages"
		"Number of bytes free in bucket overflow pages"
		"Number of duplicate pages"
		"Number of bytes free in duplicate pages"
		"Number of pages on the free list"
	}

	set all_pattern {
		"Default Hash database information"
		# HAM cursor information
		"Bucket traversing"
		"Bucket locked"
		"Duplicate set offset"
		"Current duplicate length"
		"Total duplicate set length"
		"Bytes needed for add"
		"Page on which we can insert"
		"Order"
		"Internal Flags"
	}

	puts "\tEnv020: Check DB->stat_print for hash"
	env020_db_stat_print hash $pattern $all_pattern
}

proc env020_heap_stat_print {} {
	set pattern {
		"Local time"
		# Heap information
		"Heap magic number"
		"Heap version number"
		"Number of records in the database"
		"Number of blobs in the database"
		"Number of database pages"
		"Underlying database page size"
		"Number of database regions"
		"Number of pages in a region"
	}

	set all_pattern {
		"Default Heap database information"
	}

	puts "\tEnv020: Check DB->stat_print for heap"
	env020_db_stat_print heap $pattern $all_pattern
}

proc env020_ram_stat_print {} {
	set pattern {
		"Local time"
		# Btree information
		"Btree magic number"
		"Btree version number"
		"Byte order"
		"Flags"
		"Fixed-length record size"
		"Fixed-length record pad"
		"Underlying database page size"
		"Number of levels in the tree"
		"Number of records in the tree"
		"Number of data items in the tree"
		"Number of tree internal pages"
		"Number of bytes free in tree internal pages"
		"Number of tree leaf pages"
		"Number of bytes free in tree leaf pages"
		"Number of tree duplicate pages"
		"Number of bytes free in tree duplicate pages"
		"Number of tree overflow pages"
		"Number of bytes free in tree overflow pages"
		"Number of empty pages"
		"Number of pages on the free list"
	}

	set all_pattern {
		"Default Btree/Recno database information"
		# Btree cursor information
		"Overflow size"
		{^[\d\s]*Recno}
		"Order"
		"Internal Flags"
	}

	puts "\tEnv020: Check DB->stat_print for recno"
	env020_db_stat_print recno $pattern $all_pattern
}

proc env020_qam_stat_print { } {
	set pattern {
		"Local time"
		# Queue information
		"Queue magic number"
		"Queue version number"
		"Fixed-length record size"
		"Fixed-length record pad"
		"Underlying database page size"
		"Underlying database extent size"
		"Number of records in the database"
		"Number of data items in the database"
		"Number of database pages"
		"Number of bytes free in database pages"
		"First undeleted record"
		"Next available record number"
	}

	set all_pattern {
		"Default Queue database information"
	}

	puts "\tEnv020: Check DB->stat_print for queue"
	env020_db_stat_print queue $pattern $all_pattern
}

proc env020_db_stat_print {method pattern all_pattern} {
	source ./include.tcl
	set dball_pattern {
		"Local time"
		"=-=-=-=-=-=-=-=-=-=-=-=-="
		# DB Handle information
		"DB handle information"
		"Page size"
		"Append recno"
		"Feedback"
		"Dup compare"
		"App private"
		"DbEnv"
		"Type"
		"Thread mutex"
		"File"
		"Database"
		"Open flags"
		"File ID"
		"Cursor adjust ID"
		"Meta pgno"
		"Locker ID"
		"Handle lock"
		"Associate lock"
		"Replication handle timestamp"
		"Secondary callback"
		"Primary handle"
		"api internal"
		"Btree/Recno internal"
		"Hash internal"
		"Queue internal"
		"Flags"
		"File naming information"
		# DB register information.
		"DB handle FNAME contents"
		"log ID"
		"Meta pgno"
		"create txn"
		"refcount"
		"Flags"
		# DB cursor information
		"DB handle cursors"
		"Active queue"
		"DBC"
		"Associated dbp"
		"Associated txn"
		"Internal"
		"Default locker ID"
		"Locker"
		"Type"
		"Off-page duplicate cursor"
		"Referenced page"
		"Root"
		"Page number"
		"Page index"
		"Lock mode"
		"Flags"
		"Join queue"
		"Free queue"
	}
	env_cleanup $testdir
	set env [eval berkdb_env_noerr -create -home $testdir]
	error_check_good is_valid_env [is_valid_env $env] TRUE

	# Test using the default option.
	puts "\t\tUsing the default option"
	set db [eval berkdb_open_noerr -create -env $env -$method \
	    -msgfile $testdir/msgfile1 db1.db]

	error_check_good db_stat_print [$db stat_print] 0
	error_check_good "$db close" [$db close] 0
	env020_check_output $pattern $testdir/msgfile1

	# Test using stat_print -fast
	puts "\t\tUsing -fast option"
	set db [eval berkdb_open_noerr -create -env $env -$method \
	    -msgfile $testdir/msgfile2 db2.db]
	error_check_good db_stat_print [$db stat_print -fast] 0
	error_check_good "$db close" [$db close] 0
	env020_check_output $pattern $testdir/msgfile2

	# Test using stat_print -all
	puts "\t\tUsing -all option"
	set db [eval berkdb_open_noerr -create -env $env -$method \
	    -msgfile $testdir/msgfile3 db3.db]
	error_check_good db_stat_print [$db stat_print -all] 0
	error_check_good "$db close" [$db close] 0
	env020_check_output [concat $dball_pattern $pattern $all_pattern] \
            $testdir/msgfile3

	error_check_good "$env close" [$env close] 0

	file delete -force $testdir/msgfile1
	error_check_good "file_not_exists" [file exists $testdir/msgfile1] 0
	file delete -force $testdir/msgfile2
	error_check_good "file_not_exists" [file exists $testdir/msgfile2] 0
	file delete -force $testdir/msgfile3
	error_check_good "file_not_exists" [file exists $testdir/msgfile3] 0
}

proc env020_seq_stat_print { } {
	source ./include.tcl
	set pattern {
		"The number of sequence locks that required waiting"
		"The current sequence value"
		"The cached sequence value"
		"The last cached sequence value"
		"The minimum sequence value"
		"The maximum sequence value"
		"The cache size"
		"Sequence flags"
	}

	puts "\tEnv020: Check DB_SEQUENCE->stat_print"
	env_cleanup $testdir
	set env [eval berkdb_env_noerr -create -home $testdir]
	error_check_good is_valid_env [is_valid_env $env] TRUE
	set db [eval berkdb_open_noerr -create -env $env -btree \
	    -msgfile $testdir/msgfile1 db1.db]
	set seq [eval berkdb sequence -create $db key1]
	error_check_good check_seq [is_valid_seq $seq] TRUE
	error_check_good seq_stat_print [$seq stat_print] 0

	$env msgfile $testdir/msgfile2
	error_check_good seq_stat_print [$seq stat_print -clear] 0

	error_check_good seq_close [$seq close] 0
	error_check_good "$db close" [$db close] 0
	error_check_good "$env close" [$env close] 0

	puts "\t\tUsing the default option"
	env020_check_output $pattern $testdir/msgfile1

	puts "\t\tUsing -clear option"
	env020_check_output $pattern $testdir/msgfile2

	file delete -force $testdir/msgfile1
	error_check_good "file_not_exists" [file exists $testdir/msgfile1] 0
	file delete -force $testdir/msgfile2
	error_check_good "file_not_exists" [file exists $testdir/msgfile2] 0
}
