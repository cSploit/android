# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep115
# TEST	Test correct behavior of TXN_WRNOSYNC, TXN_NOSYNC and synchronous
# TEST	transactions on client sites.
# TEST
proc rep115 { method { niter 20 } { tnum "115" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Run for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

        if { $databases_in_memory } {
		set msg "with in-memory named databases"
		if { [is_queueext $method] == 1 } {
			puts "Skipping rep$tnum for method $method"
			return
		}
	}

	set args [convert_args $method $args]
	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		puts "Rep$tnum ($method $r):\
		    WRNOSYNC, NOSYNC and sync txns $msg2."
		rep115_sub $method $niter $tnum $r $args
	}
}

proc rep115_sub { method niter tnum recargs largs } {
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
	#
	# Don't use adjust_txnargs here because we want to force
	# the clients to have different txn args.  We know we
	# are using on-disk logs here.
	#
	set m_logtype on-disk
	set c_logtype on-disk

	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set c2_logargs [adjust_logargs $c_logtype]
	set c3_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs " -txn "
	set c2_txnargs " -txn nosync "
	set c3_txnargs " -txn wrnosync "

	# Open a master.
	repladd 1
	set ma_cmd "berkdb_env_noerr -create $verbargs \
	    $m_txnargs $m_logargs $repmemargs \
	    -home $masterdir -rep_master -errpfx MASTER \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_cmd $recargs]

	# Open the clients.
	repladd 2
	set cl_cmd "berkdb_env_noerr -create -home $clientdir $verbargs \
	    $c_txnargs $c_logargs -rep_client -errpfx CLIENT $repmemargs \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_cmd $recargs]

	repladd 3
	set cl2_cmd "berkdb_env_noerr -create -home $clientdir2 $verbargs \
	    $c2_txnargs $c2_logargs -rep_client -errpfx CLIENT2 $repmemargs \
	    -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl2_cmd $recargs]

	repladd 4
	set cl3_cmd "berkdb_env_noerr -create -home $clientdir3 $verbargs \
	    $c3_txnargs $c3_logargs -rep_client -errpfx CLIENT3 $repmemargs \
	    -rep_transport \[list 4 replsend\]"
	set clientenv3 [eval $cl3_cmd $recargs]

	# Bring the clients online.
	set envlist \
	    "{$masterenv 1} {$clientenv 2} {$clientenv2 3} {$clientenv3 4}"
	process_msgs $envlist

	# Open database in master, write records using an explicit
	# txn and commit each record individually.
	puts "\tRep$tnum.a: Create and populate database."
	if { $databases_in_memory == 1 } {
		set dbname { "" "rep115.db" }
	} else {
		set dbname rep115.db
	}
	set db [eval "berkdb_open_noerr -create $omethod -auto_commit \
	    -env $masterenv $largs $dbname"]
	process_msgs $envlist

	#
	# Get the stats before we write data.
	#
	set cl_write0 [stat_field $clientenv log_stat "Times log written"]
	set cl_flush0 [stat_field $clientenv log_stat \
	    "Times log flushed to disk"]
	set cl2_write0 [stat_field $clientenv2 log_stat "Times log written"]
	set cl2_flush0 [stat_field $clientenv2 log_stat \
	    "Times log flushed to disk"]
	set cl3_write0 [stat_field $clientenv3 log_stat "Times log written"]
	set cl3_flush0 [stat_field $clientenv3 log_stat \
	    "Times log flushed to disk"]
	for { set i 1 } { $i <= $niter } { incr i } {
		set t [$masterenv txn]
		error_check_good db_put \
		    [eval $db put -txn $t $i [chop_data $method data$i]] 0
		error_check_good txn_commit [$t commit] 0
	}
	process_msgs $envlist
	puts "\tRep$tnum.b: Confirm client txn sync stats"
	set cl_write1 [stat_field $clientenv log_stat "Times log written"]
	set cl_flush1 [stat_field $clientenv log_stat \
	    "Times log flushed to disk"]
	set cl2_write1 [stat_field $clientenv2 log_stat "Times log written"]
	set cl2_flush1 [stat_field $clientenv2 log_stat \
	    "Times log flushed to disk"]
	set cl3_write1 [stat_field $clientenv3 log_stat "Times log written"]
	set cl3_flush1 [stat_field $clientenv3 log_stat \
	    "Times log flushed to disk"]

	#
	# First client is completely synchronous.  So, the log should have
	# been written and flushed to disk niter times, at least.
	#
	puts "\tRep$tnum.b.1: Check synchronous client stats."
	error_check_good cl_wr [expr $cl_write1 >= ($cl_write0 + $niter)] 1
	error_check_good cl_fl [expr $cl_flush1 >= ($cl_flush0 + $niter)] 1

	#
	# Second client is nosync.  So, the log should not have
	# been written or flushed at all.
	#
	puts "\tRep$tnum.b.2: Check nosync client stats."
	error_check_good cl2_wr $cl2_write1 $cl2_write0
	error_check_good cl2_fl $cl2_flush1 $cl2_flush0

	#
	# Third client is wrnosync.  So, the log should have
	# been written niter times, but never flushed.
	#
	puts "\tRep$tnum.b.3: Check wrnosync client stats."
	error_check_good cl3_wr [expr $cl3_write1 >= ($cl3_write0 + $niter)] 1
	error_check_good cl3_fl $cl3_flush1 $cl3_flush0

	error_check_good db_close [$db close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good cl_close [$clientenv close] 0
	error_check_good cl2_close [$clientenv2 close] 0
	error_check_good cl3_close [$clientenv3 close] 0

	replclose $testdir/MSGQUEUEDIR
}
