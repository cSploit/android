# See the file LICENSE for redistribution information.
#
# Copyright (c) 2003, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep100
# TEST	Checkpoints and unresolved txns
# TEST
proc rep100 { method { niter 10 } { tnum "100" } args } {
	source ./include.tcl
	global repfiles_in_memory

	# Run for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 2]

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

			puts "Rep$tnum ($method $r):\
			    Checkpoints and unresolved txns $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep100_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep100_sub { method niter tnum logset recargs largs } {
	source ./include.tcl
	global perm_response_list
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

	file mkdir $masterdir
	file mkdir $clientdir
	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# Since we're sure to be using on-disk logs, txnargs will be -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_cmd "berkdb_env_noerr -create $verbargs \
	    -log_max 1000000 $m_txnargs $m_logargs $repmemargs \
	    -home $masterdir -rep_master -errpfx MASTER \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_cmd $recargs]

	# Open a client
	repladd 2
	set cl_cmd "berkdb_env_noerr -create -home $clientdir $verbargs \
	    $c_txnargs $c_logargs -rep_client -errpfx CLIENT $repmemargs \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_cmd $recargs]

	# Bring the client online.
	process_msgs "{$masterenv 1} {$clientenv 2}"

	# Open database in master, make lots of changes so checkpoint
	# will take a while, and propagate to client.
	puts "\tRep$tnum.a: Create and populate database."
	set dbname rep100.db
	set dbname1 rep100_1.db
	set db [eval "berkdb_open_noerr -create $omethod -auto_commit \
	    -env $masterenv $largs $dbname"]
	set db1 [eval "berkdb_open_noerr -create $omethod -auto_commit \
	    -env $masterenv $largs $dbname1"]
	for { set i 1 } { $i <= $niter } { incr i } {
		set t [$masterenv txn]
		error_check_good db_put \
		    [eval $db put -txn $t $i [chop_data $method data$i]] 0
		error_check_good db1_put \
		    [eval $db1 put -txn $t $i [chop_data $method data$i]] 0
		error_check_good txn_commit [$t commit] 0
	}
	process_msgs "{$masterenv 1} {$clientenv 2}" 1

	# Get the master's last LSN before the checkpoint
	set pre_ckp_offset \
		[stat_field $masterenv log_stat "Current log file offset"]

	puts "\tRep$tnum.b: Checkpoint on master."
	error_check_good checkpoint [$masterenv txn_checkpoint] 0
	process_msgs "{$masterenv 1} {$clientenv 2}"

	#
	# We want to generate a checkpoint that is mid-txn on the master,
	# but is mid-txn on a different txn on the client because the
	# client is behind the master.  We want to make sure we don't
	# get a Log sequence error on recovery.  The sequence of steps is:
	#
	# Open a txn T1 on the master.  Made a modification.
	# Open a 2nd txn T2 on the master and make a modification for that txn.
	# Replicate all of the above to the client.
	# Make another modification to T1.
	# Commit T1 but do not replicate to the client (i.e. lose those records).
	# Checkpoint on the master and replicate to client.
	# This should cause the client to sync pages but not the pages that
	# modify T1 because its commit hasn't appeared yet, even though it
	# has committed on the master.
	# Commit T2
	# Crash client and run recovery.
	#
	set start $niter
	set end [expr $niter * 2]

	# Open txn T1 and make a modification.
	puts "\tRep$tnum.c: Open txn t1 and do an update."
	set i $start
	set t1 [$masterenv txn]
	error_check_good db_put \
	    [eval $db1 put -txn $t1 $i [chop_data $method data$i]] 0

	# Open a 2nd txn T2 on the master and make a modification in that txn.
	# Replicate all to the client.
	puts "\tRep$tnum.d: Open txn T2."
	set t2 [$masterenv txn]

	error_check_good db_put \
	    [eval $db put -txn $t2 $end [chop_data $method data$end]] 0
	process_msgs "{$masterenv 1} {$clientenv 2}"

	# Make another modification to T1.
	# Commit T1 but do not replicate to the client (i.e. lose that record).
	puts "\tRep$tnum.e: Update and commit T1, clear msgs for client."
	error_check_good db_put \
	    [eval $db1 put -txn $t1 $i [chop_data $method data$end]] 0
	error_check_good txn_commit [$t1 commit] 0
	replclear 2

	# Checkpoint on the master and replicate to client.
	puts "\tRep$tnum.f: Checkpoint and replicate messages."
	error_check_good checkpoint [$masterenv txn_checkpoint] 0
	process_msgs "{$masterenv 1} {$clientenv 2}"

	#
	# Commit T2.  We have to sleep enough time so that the client
	# will rerequest and catch up when it receives these records.
	#
	puts "\tRep$tnum.g: Sleep, commit T2 and catch client up."
	tclsleep 2
	error_check_good txn_commit [$t2 commit] 0
	process_msgs "{$masterenv 1} {$clientenv 2}"

	#
	# At this point the client has all outstanding log records that
	# have been written by the master.  So everything should be okay.
	#
	puts "\tRep$tnum.h: Abandon clientenv and reopen with recovery."
	# "Crash" client (by abandoning its env) and run recovery.
	set clientenv_new [eval $cl_cmd -recover]

	# Clean up.
	puts "\tRep$tnum.i: Clean up."
	error_check_good db_close [$db close] 0
	error_check_good db_close [$db1 close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv_new close] 0
	# Clean up abandoned handle.
	catch {$clientenv close}

	replclose $testdir/MSGQUEUEDIR
}
