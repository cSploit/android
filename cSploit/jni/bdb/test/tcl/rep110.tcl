# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep110
# TEST	Test of internal initialization, nowait and child processes.
# TEST	This tests a particular code path for handle_cnt management.
# TEST
# TEST	One master, one client, with DB_REP_CONF_NOWAIT.
# TEST	Generate several log files.
# TEST	Remove old master log files.
# TEST	While in internal init, start a child process to open the env.
#
proc rep110 { method { niter 200 } { tnum "110" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	# Access method doesn't matter.  Just use btree.
	if { $checking_valid_methods } {
		return "btree"
	}

	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for non-btree method $method."
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
	# and with various options, such as in-memory databases.
	# Skip recovery with in-memory logging - it doesn't make sense.
	foreach r $test_recopts {
		foreach l $logsets {
			set logindex [lsearch -exact $l "in-memory"]
			if { $r == "-recover" && $logindex != -1 } {
				puts "Skipping rep$tnum for -recover\
				    with in-memory logs."
				continue
			}
			puts "Rep$tnum ($method $r $args): Test of internal\
			    init with NOWAIT and child proc $msg $msg2."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep110_sub $method $niter $tnum $l $r $args
		}
	}
}

proc rep110_sub { method niter tnum logset recargs largs } {
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

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	# Create a DB_CONFIG file in each directory to specify
	# DB_REP_CONF_NOWAIT.
	rep110_make_config $masterdir
	rep110_make_config $clientdir

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
	    $m_logargs -log_max $log_max -errpfx MASTER $verbargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Run rep_test in the master only.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	if { $databases_in_memory } {
		set testfile { "" "test.db" }
	} else {
		set testfile "test.db"
	}
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set mdb [eval {berkdb_open} -env $masterenv -auto_commit\
          	-create -mode 0644 $omethod $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master beyond the first log file.
		eval rep_test\
		    $method $masterenv $mdb $niter $start $start 0 $largs
		incr start $niter

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
	$clientenv rep_limit 0 0
	set envlist "{$masterenv 1} {$clientenv 2}"
	set loop 15
	set i 0
	set init 0
	puts "\tRep$tnum.c: While in internal init, fork a child process."
	while { $i < $loop } {
		set nproced 0
		incr nproced [proc_msgs_once $envlist NONE err]
		if { $nproced == 0 } {
			break
		}
		#
		# Wait until we are in SYNC_PAGE state and then create
		# a child process to open an env handle.
		#
		set clstat [exec $util_path/db_stat \
		    -N -r -R A -h $clientdir]
		if { $init == 0 && \
		    [expr [is_substr $clstat "SYNC_PAGE"] || \
		    [is_substr $clstat "SYNC_LOG"]] } {
			set pid [exec $tclsh_path $test_path/wrap.tcl \
			    rep110script.tcl $testdir/repscript.log \
			    $clientdir $rep_verbose &]
			tclsleep 1
			set init 1
		}
		incr i
	}
	watch_procs [list $pid] 1
	process_msgs $envlist

	puts "\tRep$tnum.d: Verify logs and databases."
	set cdb [eval {berkdb_open_noerr} -env $clientenv -auto_commit\
	    -create -mode 0644 $omethod $dbargs $testfile]
	error_check_good reptest_db [is_valid_db $cdb] TRUE

	if { $databases_in_memory } {
		rep_verify_inmem $masterenv $clientenv $mdb $cdb
	} else {
		rep_verify $masterdir $masterenv $clientdir $clientenv 1
	}

	# Add records to the master and update client.
	puts "\tRep$tnum.e: Add more records and check again."
	eval rep_test $method $masterenv $mdb $niter $start $start 0 $largs
	process_msgs $envlist 0 NONE err
	if { $databases_in_memory } {
		rep_verify_inmem $masterenv $clientenv $mdb $cdb
	} else {
		rep_verify $masterdir $masterenv $clientdir $clientenv 1
	}

	# Make sure log files are on-disk or not as expected.
	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good mdb_close [$mdb close] 0
	error_check_good cdb_close [$cdb close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

proc rep110_make_config { dir } {
	set cid [open $dir/DB_CONFIG w]
	puts $cid "rep_set_config db_rep_conf_nowait"
	close $cid
}
