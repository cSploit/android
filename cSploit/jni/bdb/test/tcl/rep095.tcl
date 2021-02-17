# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep095
# TEST	Test of internal initialization use of shared region memory.
# TEST
# TEST	One master, one client.  Create a gap that requires internal
# TEST	initialization.  Start the internal initialization in this
# TEST	parent process and complete it in a separate child process.
#
proc rep095 { method { niter 200 } { tnum "095" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	set args [convert_args $method $args]
	set saved_args $args

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

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	set logsets [create_logsets 2]

	# Run the body of the test with and without recovery,
	# and with and without cleaning.  Skip recovery with in-memory
	# logging - it doesn't make sense.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			set envargs ""
			set args $saved_args
			puts "Rep$tnum ($method $envargs $r $args):\
			    Test of internal initialization $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep095_sub $method $niter $tnum $envargs \
			    $l $r $args
		}
	}
}

proc rep095_sub { method niter tnum envargs logset recargs largs } {
	source ./include.tcl
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
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $repmemargs \
	    $m_logargs -log_max $log_max $envargs $verbargs \
	    -errpfx MASTER -home $masterdir \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $repmemargs \
	    $c_logargs -log_max $log_max $envargs $verbargs \
	    -errpfx CLIENT -home $clientdir \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the client online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist 0 NONE err
	error_check_good process_msgs $err 0

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	# Create a gap requiring internal initialization.
	set dbhandle NULL
	set cid 2
	set start [push_master_ahead $method $masterenv $masterdir $m_logtype \
	    $clientenv $cid $dbhandle $start $niter "" $largs]

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
	    rep095script.tcl $testdir/repscript.log \
	    $masterdir $clientdir $rep_verbose $verbose_type &]

	# Reopen client, which was closed by push_master_ahead above.
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 1} {$clientenv 2}"

	# Give child time to open environments to avoid lockout conflicts.
	while { [llength [$marker get CHILDSETUP]] == 0 } {
		tclsleep 1
	}

	set pmsgs 4
	puts "\tRep$tnum.c: Get partway through internal init ($pmsgs iters)."
	for { set i 1 } { $i < $pmsgs } { incr i } {
		proc_msgs_once $envlist
	}
	set pagerec [stat_field $clientenv rep_stat "Pages received"]
	# Let child know that partial internal init is done.
	error_check_good putpartinit [$marker put PARENTPARTINIT 1] 0

	# Now wait for the child to finish the internal init by simply
	# waiting for the child to finish.
	puts "\tRep$tnum.d: Waiting for child to finish internal init..."
	# Watch until the script is done.
	watch_procs $pid 10

	puts "\tRep$tnum.e: Verify more pages received after child ran."
	error_check_good more_pages_received [expr \
	    [stat_field $clientenv rep_stat "Pages received"] > $pagerec] 1

	puts "\tRep$tnum.f: Verify databases"
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	# Need to refresh our environment handles before processing more
	# messages because we abandoned message processing in the middle.
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	set masterenv [eval $ma_envcmd $recargs -rep_master]
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	set envlist "{$masterenv 1} {$clientenv 2}"

	# Add records to the master and update client.
	puts "\tRep$tnum.g: Add more records and check again."
	set entries 10
	eval rep_test $method $masterenv NULL $entries $start $start 0 $largs
	incr start $entries
	process_msgs $envlist 0 NONE err
	error_check_good process_msgs $err 0

	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	error_check_good marker_close [$marker close] 0
	error_check_good markerenv_close [$markerenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

