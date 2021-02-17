# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  rep105
# TEST	Replication and rollback on sync over multiple log files.
# TEST
# TEST	Run rep_test in a replicated master env.
# TEST  Hold open various txns in various log files and make sure
# TEST	that when synchronization happens, we rollback the correct set
# TEST	of log files.
proc rep105 { method { niter 4 } { tnum "105" } args } {
	source ./include.tcl
	global repfiles_in_memory

	# Only Btree is needed.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}

	if { [is_btree $method] == 0 } {
		puts "Skipping for method $method."
		return
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Rep$tnum: Skipping for\
				    in-memory logs with -recover."
				continue
			}
			puts "Rep$tnum ($method $r): \
			    Replication and multi-logfile rollback $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client1 logs are [lindex $l 1]"
			puts "Rep$tnum: Client2 logs are [lindex $l 2]"
			rep105_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep105_sub { method niter tnum logset recargs largs } {
	global repfiles_in_memory
	global rep_verbose
	global testdir
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

	set orig_tdir $testdir
	set omethod [convert_method $method]

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR.2
	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]

	set m_logtype [lindex $logset 0]
	set m_logargs [adjust_logargs $m_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]

	set c_logtype [lindex $logset 1]
	set c_logargs [adjust_logargs $c_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	set c2_logtype [lindex $logset 2]
	set c2_logargs [adjust_logargs $c2_logtype]
	set c2_txnargs [adjust_txnargs $c2_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $m_logargs \
	    -home $masterdir $verbargs -errpfx MASTER -log_max $log_max \
	    -rep_transport \[list 1 replsend\] $repmemargs"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open two clients
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    -home $clientdir $verbargs -errpfx CLIENT1 -log_max $log_max \
	    -rep_transport \[list 2 replsend\] $repmemargs"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	repladd 3
	set cl2_envcmd "berkdb_env_noerr -create $c2_txnargs $c2_logargs \
	    -home $clientdir2 $verbargs -errpfx CLIENT2 -log_max $log_max \
	    -rep_transport \[list 3 replsend\] $repmemargs"
	set cl2env [eval $cl2_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2} {$cl2env 3}"
	process_msgs $envlist

	# Run rep_test in the master (and update clients).
	#
	# Set niter small so that no checkpoints are performed in
	# rep_test.  We want to control when checkpoints happen.
	#
	set niter 4
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	process_msgs $envlist

	#
	# We want to start several transactions and make sure
	# that the correct log files are left based on outstanding
	# txns after sync.
	#
	# The logfile sync LSN is in log file S.  Transactions
	# are noted with T and their commit with #.  
	# We want:
	#                    SYNC_LSN
	# S-2.... S-1.... S......|...... S+1.... S+2....
	# T1.................#   |
	#   T2...................|..#
	#          T3............|.........#
	#                    T4..|..#
	#                        | T5.#
	#                        |        T6........#
	#
	# 
	# Create a few extra databases so we can hold these txns
	# open and have operations on them outstanding.
	#
	# We close 'client' at the SYNC_LSN point.  Then run with
	# the master and client2 only.  Then in S+2, we close the
	# master and reopen 'client' as the master so that client2
	# needs to rollback all the way to the SYNC_LSN.
	#
	set t1db "txn1.db"
	set t2db "txn2.db"
	set t3db "txn3.db"
	set key1 "KEY1"
	set key2 "KEY2"
	set key3 "KEY3"
	set key4 "KEY4"
	set data1 "DATA1"
	set data2 "DATA2"
	set data3 "DATA3"
	set data4 "DATA4"
	set db1 [eval {berkdb_open_noerr} -env $masterenv -auto_commit\
	    -create -mode 0644 $omethod $largs $t1db]
	set db2 [eval {berkdb_open_noerr} -env $masterenv -auto_commit\
	    -create -mode 0644 $omethod $largs $t2db]
	set db3 [eval {berkdb_open_noerr} -env $masterenv -auto_commit\
	    -create -mode 0644 $omethod $largs $t3db]
	process_msgs $envlist

	puts "\tRep$tnum.b: Set up T1 and T2 long running txns."
	set t1 [$masterenv txn]
	set ret [$db1 put -txn $t1 $key1 $data1]
	error_check_good put $ret 0
	set t2 [$masterenv txn]
	set ret [$db2 put -txn $t2 $key1 $data1]
	error_check_good put $ret 0

	set logminus2 [get_logfile $masterenv last]
	set start $niter

	rep105_moveonelog $tnum $masterenv $method $niter $start $envlist \
	    $logminus2 $largs
	
	puts "\tRep$tnum.d: Set up T3 long running txn."
	set t3 [$masterenv txn]
	set ret [$db3 put -txn $t3 $key1 $data1]
	error_check_good put $ret 0

	set logminus1 [get_logfile $masterenv last]
	set start [expr $niter * 10]
	rep105_moveonelog $tnum $masterenv $method $niter $start $envlist \
	    $logminus1 $largs

	set logsync [get_logfile $masterenv last]
	#
	# We want to resolve T1 before the sync point.
	# Write another part of that txn and then commit.
	#
	puts "\tRep$tnum.e: Resolve T1 and start T4."
	set ret [$db1 put -txn $t1 $key2 $data2]
	error_check_good put $ret 0
	error_check_good commit [$t1 commit] 0
	set t4 [$masterenv txn]
	set ret [$db1 put -txn $t4 $key3 $data3]
	error_check_good put $ret 0

	# Run a couple more txns to get a sync point
	set start [expr $niter * 20]
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	process_msgs $envlist

	puts "\tRep$tnum.f: Close client 1 and make master changes."
	error_check_good client_close [$clientenv close] 0
	set envlist "{$masterenv 1} {$cl2env 3}"

	puts "\tRep$tnum.g: Resolve T2 and T4.  Start and resolve T5."
	set ret [$db2 put -txn $t2 $key2 $data2]
	error_check_good put $ret 0
	error_check_good commit [$t2 commit] 0
	set ret [$db1 put -txn $t4 $key4 $data4]
	error_check_good put $ret 0
	error_check_good commit [$t4 commit] 0
	set t5 [$masterenv txn]
	set ret [$db2 put -txn $t5 $key3 $data3]
	error_check_good put $ret 0
	error_check_good commit [$t5 commit] 0

	set start [expr $niter * 20]
	rep105_moveonelog $tnum $masterenv $method $niter $start $envlist \
	    $logsync $largs

	set logplus1 [get_logfile $masterenv last]
	puts "\tRep$tnum.h: Resolve T3.  Start T6."
	set ret [$db3 put -txn $t3 $key2 $data2]
	error_check_good put $ret 0
	error_check_good commit [$t3 commit] 0
	set t6 [$masterenv txn]
	set ret [$db3 put -txn $t6 $key3 $data3]
	error_check_good put $ret 0

	set start [expr $niter * 30]
	rep105_moveonelog $tnum $masterenv $method $niter $start $envlist \
	    $logplus1 $largs

	puts "\tRep$tnum.i: Resolve T6.  Close dbs"
	set ret [$db3 put -txn $t6 $key4 $data4]
	error_check_good put $ret 0
	error_check_good commit [$t6 commit] 0

	$db1 close
	$db2 close
	$db3 close
	process_msgs $envlist

	# Delete messages for closed client
	replclear 2

	puts "\tRep$tnum.j: Close master, reopen client as master."
	error_check_good master_close [$masterenv close] 0

	set newmasterenv [eval $cl_envcmd $recargs -rep_master]

	puts "\tRep$tnum.k: Process messages to cause rollback in client2."
	set lastlog [get_logfile $cl2env last]
	set envlist "{$newmasterenv 2} {$cl2env 3}"
	process_msgs $envlist
	replclear 1

	#
	# Verify we rolled back to the expected log file.
	# We know we're dealing with single digit log files nums so
	# do the easy thing using lfname.  If that ever changes,
	# this will need to be fixed.
	#
	# We expect to rollback to $logsync, and that $logplus1
	# through $lastlog are gone after processing messages.
	# All of the rollback verification is in clientdir2.
	#
	set cwd [pwd]
	cd $clientdir2
	set saved_lf [glob -nocomplain log.*]
	cd $cwd
	set lfname log.000000000

	# For in-memory logs we just check the log file
	# number of the first and last logs and assume that
	# the logs in the middle are available.  For on-disk
	# logs we check for physical existence of all logs.
	if { $c2_logtype == "in-memory" } {
		set last_lf [get_logfile $cl2env last]
		set first_lf [get_logfile $cl2env first]
		error_check_good first_inmem_log\
		    [expr $first_lf <= $logminus2] 1
		error_check_good last_inmem_log $last_lf $logsync
	} else {
		for { set i $logminus2 } { $i <= $logsync } { incr i } {
			set lf $lfname$i
			set present [lsearch -exact $saved_lf $lf]
			error_check_bad lf.present.$i $present -1
		}
		for { set i $logplus1 } { $i <= $lastlog } { incr i } {
			set lf $lfname$i
			set present [lsearch -exact $saved_lf $lf]
			error_check_good lf.notpresent.$i $present -1
		}
	}
	error_check_good newmasterenv_close [$newmasterenv close] 0
	error_check_good cl2_close [$cl2env close] 0
	replclose $testdir/MSGQUEUEDIR
	set testdir $orig_tdir
	return
}

proc rep105_moveonelog { tnum env method niter start envlist lognum largs } {
	set stop 0
	while { $stop == 0 } {
		puts "\t\tRep$tnum: Running rep_test until past log $lognum."
		eval rep_test $method $env NULL $niter $start $start \
		    0 $largs
		process_msgs $envlist
		incr start $niter
		set newlog [get_logfile $env last]
		if { $newlog > $lognum } {
			set stop 1
		}
	}
}
