# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep109
# TEST	Test that snapshot isolation cannot be used on HA clients.
# TEST	Master creates a txn with DB_TXN_SNAPSHOT and succeeds.
# TEST	Client gets an error when creating txn with DB_TXN_SNAPSHOT.
# TEST	Master opens a cursor with DB_TXN_SNAPSHOT and succeeds.
# TEST	Client gets and error when opening a cursor with DB_TXN_SNAPSHOT.
# TEST
proc rep109 { method { niter 10 } { tnum "109" } args } {
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
	set logsets [create_logsets 2]

	# Run the body of the test with and without recovery.
	foreach r $test_recopts {
		foreach l $logsets {
			puts "Rep$tnum ($method $r): DB_TXN_SNAPSHOT and HA."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep109_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep109_sub { method niter tnum logset recargs largs } {
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

	file mkdir $masterdir
	file mkdir $clientdir

	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# Since we're sure to be using on-disk logs, txnargs will be -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master with MVCC.
	repladd 1
	set ma_cmd "berkdb_env_noerr -create $verbargs \
	    -log_max $log_max $m_txnargs $m_logargs $repmemargs \
	    -multiversion -home $masterdir -rep_master -errpfx MASTER \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_cmd $recargs]

	# Open a client with MVCC.
	repladd 2
	set cl_cmd "berkdb_env_noerr -create -home $clientdir $verbargs \
	    $c_txnargs$c_logargs -rep_client -errpfx CLIENT $repmemargs \
	    -multiversion -log_max $log_max -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_cmd $recargs]

	# Bring the client online.
	process_msgs "{$masterenv 1} {$clientenv 2}"

	puts "\tRep$tnum.a: Create a txn with DB_TXN_SNAPSHOT on the master."
	set txn [$masterenv txn -snapshot]
	error_check_good master_txn [is_valid_txn $txn $masterenv] TRUE
	error_check_good abort [$txn abort] 0

	puts "\tRep$tnum.b: Open a txn with DB_TXN_SNAPSHOT on the client."
	catch { [$clientenv txn -snapshot] } ret
	error_check_good client_txn_fail \
	    [is_substr $ret "invalid argument"] 1

	# Create database on master and open a DB_TXN_SNAPSHOT
	# cursor on it.
	puts "\tRep$tnum.c: Open a cursor with DB_TXN_SNAPSHOT on the master."
	set start 0
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
	} else {
		set testfile "test.db"
	}
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set mdb [eval {berkdb_open} -env $masterenv -auto_commit -create $omethod \
	    -multiversion -mode 0644 $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE

	# Since cursor does not have a -snapshot flag in Tcl, set it here.
	error_check_good set_snapshot_master \
	    [$masterenv set_flags -snapshot on] 0
	set cur [$mdb cursor]
	error_check_good master_cursor [is_valid_cursor $cur $mdb] TRUE
	error_check_good cur_close [$cur close] 0
	error_check_good close [$mdb close] 0

	# Replicate the new database to the client.
	process_msgs "{$masterenv 1} {$clientenv 2}"

	# Open the database on the client and try to open a cursor
	# with DB_TXN_SNAPSHOT.
	puts "\tRep$tnum.d: Open a cursor with DB_TXN_SNAPSHOT on the client."
	set cdb [eval {berkdb_open_noerr} -env $clientenv -auto_commit $omethod \
	    -multiversion -mode 0644 $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $cdb] TRUE

	# Since cursor does not have a -snapshot flag in Tcl, set it here.
	error_check_good set_snapshot_client \
	    [$clientenv set_flags -snapshot on] 0
	catch { [$cdb cursor] } ret
	error_check_good client_cursor_fail \
	    [is_substr $ret "invalid argument"] 1
	
	error_check_good close [$cdb close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

