# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep098
# TEST	Test of internal initialization and page deallocation.
# TEST
# TEST	Use one master, one client.
# TEST	Generate several log files.
# TEST	Remove old master log files.
# TEST	Start a client.
# TEST	After client gets UPDATE file information, delete entries to
# TEST	remove pages in the database.
#
proc rep098 { method { niter 200 } { tnum "098" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Run for btree and queue methods only.
	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || \
			    [is_queue $method] == 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_btree $method] == 0 && [is_queue $method] == 0 } {
		puts "Rep$tnum: skipping for non-btree, non-queue method."
		return
	}

	set args [convert_args $method $args]

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
        if { $pgindex != -1 } {
                puts "Rep$tnum: skipping for specific pagesizes"
                return
        }

	set logsets [create_logsets 2]

	# Set up for on-disk or in-memory databases.
	set msg "using on-disk databases"
	if { $databases_in_memory } {
		set msg "using named in-memory databases"
		if { [is_queueext $method] } { 
			puts -nonewline "Skipping rep$tnum for method "
			puts "$method with named in-memory databases."
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery,
	# Skip recovery with in-memory logging - it doesn't make sense.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			puts "Rep$tnum ($method $r $args): Test of\
			    internal init with page deallocation $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep098_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep098_sub { method niter tnum logset recargs largs } {
	global databases_in_memory
	global repfiles_in_memory
	global rep_verbose
	global testdir
	global util_path
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype 1048576]
	set c_logargs [adjust_logargs $c_logtype 1048576]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $repmemargs \
	    $m_logargs -log_max $log_max -errpfx MASTER $verbargs \
	    -lock_max_locks 20000 \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Run rep_test in the master only.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
		set bigfile { "" "big.db" }
	} else {
		set testfile "test.db"
		set bigfile "big.db"
	}
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set mdb [eval {berkdb_open} -env $masterenv -auto_commit\
          	-create -mode 0644 $omethod $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE
	#
	# Create a database with lots of items forcing lots of pages.
	# We want two databases so that we
	#
	set bigdb [eval {berkdb_open} -env $masterenv -auto_commit\
          	-create -mode 0644 $omethod $dbargs $bigfile ]
	error_check_good reptest_db [is_valid_db $bigdb] TRUE

	set stop 0
	set bigniter [expr $niter * 10]
	while { $stop == 0 } {
		eval rep_test \
		    $method $masterenv $mdb $niter $start $start 0 $largs
		incr start $niter
		eval rep_test \
		    $method $masterenv $bigdb $bigniter $start $start 0 $largs
		incr start $bigniter

		puts "\tRep$tnum.a.1: Run db_archive on master."
		if { $m_logtype == "on-disk" } {
			$masterenv log_flush
			eval exec $util_path/db_archive -d -h $masterdir
		}
		#
		# Make sure we have moved beyond the first log file.
		#
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > 1 } {
			set stop 1
		}
	}

	puts "\tRep$tnum.b: Open client."
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $repmemargs \
	    $c_logargs -log_max $log_max -errpfx CLIENT $verbargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	set envlist "{$masterenv 1} {$clientenv 2}"

	#
	# Process messages until the client gets into the SYNC_PAGE state.
	# We want to stop once we have the file information telling us
	# how many pages there are, but before # we ask for the pages.
	# Then we can remove some of the pages the master said were there.
	#
	set stop 0
	while { $stop == 0 } {
		set nproced 0
		incr nproced [proc_msgs_once $envlist NONE err]
		error_check_bad nproced $nproced 0
		set clstat [exec $util_path/db_stat \
		    -N -r -R A -h $clientdir]
		if { [is_substr $clstat "SYNC_PAGE"] } {
			set stop 1
		}
	}
	#
	# At this point the client has all the file info about both
	# databases.  Now let's remove pages from big.db.
	#
	if { [is_queue $method] == 0 } {
		set consume ""
		set statfld "Page count"
	} else {
		set consume "-consume"
		set statfld "Number of pages"
	}
	set pg1 [stat_field $bigdb stat $statfld]
	set txn [$masterenv txn]
	set dbc [$bigdb cursor -txn $txn]
	#
	# Note, we get the last item and then move to the prev so that
	# we remove all but the last item.  We use the last item instead
	# of the first so that the queue head moves.
	#
	set kd [$dbc get -last]
	for { set kd [$dbc get -prev] } { [llength $kd] > 0 } \
	    { set kd [$dbc get -prev] } {
		error_check_good del_item [eval {$dbc del} $consume] 0
	}
	error_check_good dbc_close [$dbc close] 0
	if { [is_queue $method] == 0 } {
		set stat [catch {eval $bigdb compact -txn $txn -freespace} ret]
		error_check_good compact $stat 0
	}
	error_check_good txn_commit [$txn commit] 0
	$bigdb sync
	set pg2 [stat_field $bigdb stat $statfld]
	error_check_good pgcnt [expr $pg2 < $pg1] 1
	puts "\tRep$tnum.c: Process msgs after page count reduction $pg1 to $pg2"
	#
	# Now that we've removed pages, let the init complete.
	#
	process_msgs $envlist

	puts "\tRep$tnum.d: Verify logs and databases"
	set cdb [eval {berkdb_open_noerr} -env $clientenv -auto_commit\
       	    -create -mode 0644 $omethod $dbargs $testfile]
	error_check_good reptest_db [is_valid_db $cdb] TRUE
	set cbdb [eval {berkdb_open_noerr} -env $clientenv -auto_commit\
       	    -create -mode 0644 $omethod $dbargs $bigfile]
	error_check_good reptest_db [is_valid_db $cdb] TRUE
	if { $databases_in_memory } {
		rep_verify_inmem $masterenv $clientenv $mdb $cdb
		rep_verify_inmem $masterenv $clientenv $bigdb $cbdb
	} else {
		rep_verify $masterdir $masterenv $clientdir $clientenv 1
	}

	# Make sure log files are on-disk or not as expected.
	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good mdb_close [$mdb close] 0
	error_check_good mdb_close [$bigdb close] 0
	error_check_good cdb_close [$cdb close] 0
	error_check_good cdb_close [$cbdb close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
