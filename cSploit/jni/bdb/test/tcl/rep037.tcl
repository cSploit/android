# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep037
# TEST	Test of internal initialization and page throttling.
# TEST
# TEST	One master, one client, force page throttling.
# TEST	Generate several log files.
# TEST	Remove old master log files.
# TEST	Delete client files and restart client.
# TEST	Put one more record to the master.
# TEST  Verify page throttling occurred.
#
proc rep037 { method { niter 1500 } { tnum "037" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory
	global env_private

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set args [convert_args $method $args]
	set saved_args $args

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

	set msg3 ""
	if { $env_private } {
		set msg3 "with private env"
	}

	# Run the body of the test with and without recovery,
	# and with various configurations.
	set configopts { dup bulk clean noclean }
	foreach r $test_recopts {
		foreach c $configopts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
					    with in-memory logs."
					continue
				}
				if { $c == "dup" && $databases_in_memory } {
					puts "Skipping rep$tnum for dup\
					    with in-memory databases."
					continue
				}
				set args $saved_args
				puts "Rep$tnum ($method $c $r $args):\
				    Test of internal init with page\
				    throttling $msg $msg2 $msg3."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep037_sub $method $niter $tnum $l $r $c $args
			}
		}
	}
}

proc rep037_sub { method niter tnum logset recargs config largs } {
	global testdir
	global util_path
	global databases_in_memory
	global repfiles_in_memory
	global env_private
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

	set privargs ""
	if { $env_private == 1 } {
		set privargs " -private "
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

	#
	# If using bulk processing, just use clean.  We could add
	# another control loop to do bulk+clean and then bulk+noclean
	# but that seems like overkill.
	#
	set bulk 0
	set clean $config
	if { $config == "bulk" } {
		set bulk 1
		set clean "clean"
	}
	#
	# If using dups do not clean the env.  We want to keep the
	# database around to a dbp and cursor to open.
	#
	set dup 0
	if { $config == "dup" } {
		set dup 1
		set clean "noclean"
	}
	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $repmemargs \
	    $m_logargs -log_max $log_max -errpfx MASTER $verbargs \
	    $privargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]
	$masterenv rep_limit 0 [expr 32 * 1024]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $repmemargs \
	    $c_logargs -log_max $log_max -errpfx CLIENT $verbargs \
	    $privargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	if { $bulk } {
		error_check_good set_bulk [$masterenv rep_config {bulk on}] 0
	}

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	if { $dup } {
		#
		# Create a known db for dup cursor testing.
		#
		puts "\tRep$tnum.a0: Creating dup db."
		if { $databases_in_memory == 1 } {
			set dupfile { "" "dup.db" }
		} else {
			set dupfile "dup.db"
		}
		set dargs [convert_args $method $largs]
		set omethod [convert_method $method]
		set dupdb [eval {berkdb_open_noerr} -env $masterenv \
		    -auto_commit -create -mode 0644 $omethod $dargs $dupfile]
		$dupdb close
	}
	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	# Create a gap requiring internal initialization.
	set flags ""
	set cid 2
	set dbhandle NULL
	set start [push_master_ahead $method $masterenv $masterdir $m_logtype \
	    $clientenv $cid $dbhandle $start $niter $flags $largs]

	puts "\tRep$tnum.e: Reopen client ($clean)."
	if { $clean == "clean" } {
		env_cleanup $clientdir
	}
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	#
	# If testing duplicate cursors, open and close a dup cursor now.  All
	# we need to do is create a dup cursor and then close both
	# cursors before internal init begins.  That will make sure
	# that the lockout is working correctly.
	#
	if { $dup } {
		puts "\tRep$tnum.e.1: Open/close dup cursor."
		set dupdb [eval {berkdb_open_noerr} -env $clientenv $dupfile]
		set dbc [$dupdb cursor]
		set dbc2 [$dbc dup]
		$dbc2 close
		$dbc close
		$dupdb close
	}
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	if { $clean == "noclean" } {
		puts "\tRep$tnum.e.1: Trigger log request"
		#
		# When we don't clean, starting the client doesn't
		# trigger any events.  We need to generate some log
		# records so that the client requests the missing
		# logs and that will trigger it.
		#
		set entries 10
		eval rep_test \
		    $method $masterenv NULL $entries $start $start 0 $largs
		incr start $entries
		process_msgs $envlist 0 NONE err
	}

	puts "\tRep$tnum.f: Verify logs and databases"
	set verify_subset \
	    [expr { $m_logtype == "in-memory" || $c_logtype == "in-memory" }]
	rep_verify $masterdir $masterenv\
	     $clientdir $clientenv $verify_subset 1 1

	puts "\tRep$tnum.g: Verify throttling."
	if { $niter > 1000 } {
                set nthrottles \
		    [stat_field $masterenv rep_stat "Transmission limited"]
		error_check_bad nthrottles $nthrottles -1
		error_check_bad nthrottles $nthrottles 0
	}

	# Make sure log files are on-disk or not as expected.
	check_log_location $masterenv
	check_log_location $clientenv

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
