# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep101
# TEST	Test of exclusive access to databases and HA.
# TEST	Confirm basic functionality with master and client.
# TEST	Confirm excl access after checkpoint and archive.
# TEST	Confirm internal init with open excl databases.
# TEST
proc rep101 { method { niter 100 } { tnum "101" } args } {
	source ./include.tcl
	global repfiles_in_memory

	# Run for just btree.
	if { $checking_valid_methods } {
		return "btree"
	}

	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: skipping for non-btree method $method."
		return
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 4]

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $logindex != -1 } {
				puts "Rep$tnum: Skipping for in-memory logs."
				continue
			}

			puts "Rep$tnum ($method $r): Exclusive databases and HA."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			puts "Rep$tnum: Client2 logs are [lindex $l 2]"
			puts "Rep$tnum: Client3 logs are [lindex $l 3]"
			rep101_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep101_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory
	global rep_verbose
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
	set omethod [convert_method $method]

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2
	set clientdir3 $testdir/CLIENTDIR3

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2
	file mkdir $clientdir3

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set c2_logtype [lindex $logset 2]
	set c3_logtype [lindex $logset 3]

	# Since we're sure to be using on-disk logs, txnargs will be -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c2_logtype]
	set c3_logargs [adjust_logargs $c3_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]
	set c3_txnargs [adjust_txnargs $c3_logtype]

	# Open a master.
	repladd 1
	set ma_cmd "berkdb_env_noerr -create $verbargs \
	    -log_max $log_max $m_txnargs $m_logargs $repmemargs \
	    -home $masterdir -rep_master -errpfx MASTER \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_cmd $recargs]

	# Open some clients
	repladd 2
	set cl_cmd "berkdb_env_noerr -create -home $clientdir $verbargs \
	    $c_txnargs $c_logargs -rep_client -errpfx CLIENT $repmemargs \
	    -log_max $log_max -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_cmd $recargs]

	repladd 3
	set cl2_cmd "berkdb_env_noerr -create -home $clientdir2 $verbargs \
	    $c2_txnargs $c2_logargs -rep_client -errpfx CLIENT2 $repmemargs \
	    -log_max $log_max -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl2_cmd $recargs]

	#
	# Set up the command but don't do the repladd nor open the
	# environment until we need it.
	#
	set cl3_cmd "berkdb_env_noerr -create -home $clientdir3 $verbargs \
	    $c3_txnargs $c3_logargs -rep_client -errpfx CLIENT3 $repmemargs \
	    -log_max $log_max -rep_transport \[list 4 replsend\]"

	#
	# Need this so that clientenv2 does not later need an internal
	# init because we must match while having a checkpoint in the log.
	#
	$masterenv txn_checkpoint -force

	# Bring the clients online.
	process_msgs "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"

	# Close client2.  We want it later to synchronize with the
	# master, but not doing an internal init.  That is, we want
	# to test that running sync-up recovery does the right thing
	# regarding exclusive databases.
	$clientenv2 log_flush
	error_check_good env_close [$clientenv2 close] 0

	# Open exclusive database in master.  Process messages to client1.
	# Confirm client cannot open the database.
	puts "\tRep$tnum.a: Running rep_test to exclusive db."
	set start 0
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
	} else {
		set testfile "test.db"
	}
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set mdb [eval {berkdb_open} -env $masterenv -auto_commit -create $omethod \
          	-lk_exclusive 0 -mode 0644 $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE

	eval rep_test\
	    $method $masterenv $mdb $niter $start $start 0 $largs
	incr start $niter
	process_msgs "{$masterenv 1} {$clientenv 2}"

	puts "\tRep$tnum.a.1: Confirm client cannot access excl db."
	rep_client_access $clientenv $testfile FAIL

	# Open client2 with recovery.
	replclear 3
	puts "\tRep$tnum.a.2: Confirm client2 cannot access excl db after start."
	set clientenv2 [eval $cl2_cmd -recover]
	process_msgs "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"

	rep_client_access $clientenv2 $testfile FAIL

	puts "\tRep$tnum.a.3: Close excl database and confirm client access."
	error_check_good dbclose [$mdb close] 0
	process_msgs "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"

	# Only need to check on one client.
	rep_client_access $clientenv $testfile SUCCESS

	puts "\tRep$tnum.b: Confirm checkpoints and archiving."
	puts "\tRep$tnum.b.1: Reopen excl db and move log forward."
	set mdb [eval {berkdb_open} -env $masterenv -auto_commit $omethod \
          	-lk_exclusive 0 -mode 0644 $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE
	process_msgs "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"

	#
	# Test here that processing the open log records from the master
	# (as opposed to the create-then-open log records the master
	# originally wrote for exclusive access) also works to
	# provide exclusive access.  Testing one client is sufficient.
	#
	rep_client_access $clientenv2 $testfile FAIL

	#
	# Clobber replication's 30 second anti-archive timer.
	$masterenv test force noarchive_timeout

	#
	# We want to move all sites forward, beyond log file 1, and
	# then archive the logs to remove the exclusive creation and
	# open records from the log.  This entire test section tests
	# the ability to restore the exclusive property from just
	# running recovery with checkpoint records in the log.
	#
	set stop 0
	set orig_master_last [get_logfile $masterenv last]
	while { $stop == 0 } {
		# Run rep_test in the master beyond the first log file.
		eval rep_test\
		    $method $masterenv $mdb $niter $start $start 0 $largs
		incr start $niter

		puts "\tRep$tnum.b.2: Run db_archive on master."
		if { $m_logtype == "on-disk" } {
			$masterenv log_flush
			$masterenv log_archive -arch_remove
		}
		#
		# Make sure we have moved beyond the master's original logs.
		#
		set curr_master_first [get_logfile $masterenv first]
		if { $curr_master_first > $orig_master_last } {
			set stop 1
		}
	}
	puts "\tRep$tnum.b.3: Run db_archive on clients."
	process_msgs "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"
	eval exec $util_path/db_archive -d -h $clientdir
	eval exec $util_path/db_archive -d -h $clientdir2

	#
	# Client2 is now in a state where it no longer has the
	# the original exclusive open of the database in the
	# log.  Open with recovery to force it to use checkpoint
	# records and then run sync-up recovery.
	puts "\tRep$tnum.b.4: Close client, do more txns, reopen client."
	$clientenv2 log_flush
	error_check_good env_close [$clientenv2 close] 0
	eval rep_test $method $masterenv $mdb $niter $start $start 0 $largs
	incr start $niter
	replclear 3
	process_msgs "{$masterenv 1} {$clientenv 2}"
	set clientenv2 [eval $cl2_cmd -recover]
	process_msgs "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"

	puts "\tRep$tnum.b.5: Confirm client cannot access excl db."
	rep_client_access $clientenv2 $testfile FAIL
	
	# Close the client and then close the exclusive db.
	# Confirm that when the client runs recovery it is able
	# to access the database.
	puts "\tRep$tnum.b.6: Confirm client recovery and closed excl db."
	$clientenv2 log_flush
	error_check_good env_close [$clientenv2 close] 0
	eval rep_test $method $masterenv $mdb $niter $start $start 0 $largs
	incr start $niter
	error_check_good dbclose [$mdb close] 0
	replclear 3
	process_msgs "{$masterenv 1} {$clientenv 2}"
	set clientenv2 [eval $cl2_cmd -recover]
	process_msgs "{$masterenv 1} {$clientenv 2} {$clientenv2 3}"

	#
	# Check both clients to make sure they can both now access the db.
	#
	rep_client_access $clientenv $testfile SUCCESS
	rep_client_access $clientenv2 $testfile SUCCESS

	puts "\tRep$tnum.c: Add new client while excl db is open."
	set mdb [eval {berkdb_open} -env $masterenv -auto_commit $omethod \
          	-lk_exclusive 0 -mode 0644 $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE
	repladd 4
	set clientenv3 [eval $cl3_cmd $recargs]
	eval rep_test $method $masterenv $mdb $niter $start $start 0 $largs
	incr start $niter
	process_msgs "{$masterenv 1} {$clientenv 2} {$clientenv2 3} {$clientenv3 4}"
	puts "\tRep$tnum.c.1: Confirm new client cannot access db."
	rep_client_access $clientenv3 $testfile FAIL
	puts "\tRep$tnum.c.2: Close db and confirm access."
	error_check_good dbclose [$mdb close] 0
	process_msgs "{$masterenv 1} {$clientenv 2} {$clientenv2 3} {$clientenv3 4}"
	rep_client_access $clientenv3 $testfile SUCCESS

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	error_check_good clientenv_close [$clientenv2 close] 0
	error_check_good clientenv_close [$clientenv3 close] 0

	replclose $testdir/MSGQUEUEDIR
}
