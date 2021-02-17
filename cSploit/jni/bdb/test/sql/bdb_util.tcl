#
#    May you do good and not evil.
#    May you find forgiveness for yourself and forgive others.
#    May you share freely, never taking more than you give.
#
#***********************************************************************
# Utility functions for bdb tests.

source $testdir/tester.tcl
source $testdir/../../../../test/tcl_utils/multi_proc_utils.tcl
source $testdir/../../../../test/tcl_utils/common_test_utils.tcl

# 
# Functions for threads that return SQLITE_LOCK error when caught
set ::bdb_thread_procs { 
  proc execsql {sql} {
    set rc SQLITE_OK
    set err [catch {
      set ::STMT [sqlite3_prepare_v2 $::DB $sql -1 dummy_tail]
    } msg]

    if {$err == 0} {
      while {[set rc [sqlite3_step $::STMT]] eq "SQLITE_ROW"} {}
      set rc [sqlite3_finalize $::STMT]
    } else {
      if {[lindex $msg 0]=="(6)"} {
        set rc SQLITE_LOCKED
      } else {
        set rc SQLITE_ERROR
      }
    }

    if {[string first locked [sqlite3_errmsg $::DB]]>=0} {
      set rc SQLITE_LOCKED
    }
    if {$rc ne "SQLITE_OK" && $rc ne "SQLITE_LOCKED"} {
      set errtxt "$rc - [sqlite3_errmsg $::DB] (debug1)"
    }
    set rc
  }

  proc do_test {name script result} {
    set res [eval $script]
    if {$res ne $result} {
      puts "$name failed: expected \"$result\" got \"$res\""
      error "$name failed: expected \"$result\" got \"$res\""
    }
  }
}

#
# This procedure sets up three sites and databases suitable for replication
# testing.  The databases are created in separate subdirectories of the
# current working directory.
#
# This procedure populates global variables for each site's network
# address (host:port) and each site's directory for later use in tests.
# It uses the standard sqlite testing databases: db, db2 and db3.
#
proc setup_rep_sites {} {
	global site1addr site2addr site3addr site1dir site2dir site3dir

	# Get free ports in safe range for most platforms.
	set ports [available_ports 3]

	# Set up site1 directory and database.
	set site1dir ./repsite1
	catch {db close}
	file delete -force $site1dir/rep.db
	file delete -force $site1dir/rep.db-journal
	file delete -force $site1dir
	file mkdir $site1dir
	sqlite3 db $site1dir/rep.db
	set site1addr "127.0.0.1:[lindex $ports 0]"

	# Set up site2 directory and database.
	set site2dir ./repsite2
	catch {db2 close}
	file delete -force $site2dir/rep.db
	file delete -force $site2dir/rep.db-journal
	file delete -force $site2dir
	file mkdir $site2dir
	sqlite3 db2 $site2dir/rep.db
	set site2addr "127.0.0.1:[lindex $ports 1]"

	# Set up site3 directory and database.
	set site3dir ./repsite3
	catch {db3 close}
	file delete -force $site3dir/rep.db
	file delete -force $site3dir/rep.db-journal
	file delete -force $site3dir
	file mkdir $site3dir
	sqlite3 db3 $site3dir/rep.db
	set site3addr "127.0.0.1:[lindex $ports 2]"
}
