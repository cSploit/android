# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep102
# TEST	Test of exclusive access to databases and HA.
# TEST	Master creates a database, writes some txns, and closes the db.
# TEST	Client opens the database (in child process).
# TEST	Master opens database exclusively.
# TEST	Client tries to read database and gets HANDLE_DEAD.  Closes db.
# TEST	Client tries to reopen database and is blocked.
# TEST
proc rep102 { method { niter 10 } { tnum "102" } args } {
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

			puts "Rep$tnum ($method $r): \
			    Exclusive databases and HANDLE_DEAD."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep102_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep102_sub { method niter tnum logset recargs largs } {
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

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
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

	# Open a master.
	repladd 1
	set ma_cmd "berkdb_env_noerr -create $verbargs \
	    -log_max $log_max $m_txnargs $m_logargs $repmemargs \
	    -home $masterdir -rep_master -errpfx MASTER \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_cmd $recargs]

	# Open a client.
	repladd 2
	set cl_cmd "berkdb_env_noerr -create -home $clientdir $verbargs \
	    $c_txnargs $c_logargs -rep_client -errpfx CLIENT $repmemargs \
	    -log_max $log_max -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_cmd $recargs]

	# Bring the client online.
	process_msgs "{$masterenv 1} {$clientenv 2}"

	# Create database on master and process messages to client.
	# Client (in script) will then open the database.
	puts "\tRep$tnum.a: Running rep_test to non-exclusive db."
	set start 0
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
	} else {
		set testfile "test.db"
	}
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set mdb [eval {berkdb_open} -env $masterenv -auto_commit -create $omethod \
          	-mode 0644 $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE

	eval rep_test\
	    $method $masterenv $mdb $niter $start $start 0 $largs
	error_check_good close [$mdb close] 0
	incr start $niter
	process_msgs "{$masterenv 1} {$clientenv 2}"

	# Communicate with child process by creating a marker file.
	set markerenv [berkdb_env_noerr -create -home $testdir -txn]
	error_check_good markerenv_open [is_valid_env $markerenv] TRUE
	set marker [eval "berkdb_open_noerr \
	    -create -btree -auto_commit -env $markerenv marker.db"]

	# Fork child process.  It should process whatever it finds in the
	# message queue -- the remaining messages for the internal
	# initialization.  It is run in a separate process to test multiple
	# processes using curinfo and originfo in the shared rep region.
	#
	puts "\tRep$tnum.b: Fork child process."
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep102script.tcl $testdir/repscript.log \
	    $masterdir $clientdir $testfile $rep_verbose $verbose_type &]

	# Give child time to open environments to avoid lockout conflicts.
	while { [llength [$marker get CHILDSETUP]] == 0 } {
		tclsleep 1
	}

	puts "\tRep$tnum.c: Reopen database exclusively on master."
	#
	# Processing messages will hang until the client closes its handle.
	# Signal the client the master is ready after we reopen.
	#
	set mdb [eval {berkdb_open} -env $masterenv -auto_commit $omethod \
	    -lk_exclusive 0 -mode 0644 $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE
	error_check_good putpartinit [$marker put PARENTPARTINIT 1] 0
	process_msgs "{$masterenv 1} {$clientenv 2}"

	# Now wait for the child to finish.
	puts "\tRep$tnum.d: Waiting for child to finish..."
	# Watch until the script is done.
	watch_procs $pid 10

	puts "\tRep$tnum.e: Confirm client cannot access."
	rep_client_access $clientenv $testfile FAIL

	error_check_good close [$mdb close] 0

	error_check_good marker_close [$marker close] 0
	error_check_good markerenv_close [$markerenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

