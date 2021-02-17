# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep029
# TEST	Test of internal initialization.
# TEST
# TEST	One master, one client.
# TEST	Generate several log files.
# TEST	Remove old master log files.
# TEST	Delete client files and restart client.
# TEST	Put one more record to the master.
#
proc rep029 { method { niter 200 } { tnum "029" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory
	global env_private

	if { $checking_valid_methods } {
		return "ALL"
	}
	global passwd
	global has_crypto

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

	set msg3 ""
	if { $env_private } {
		set msg3 "with private env"
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
	set opts { bulk clean noclean }
	foreach r $test_recopts {
		foreach c $opts {
			foreach l $logsets {
				set logindex [lsearch -exact $l "in-memory"]
				if { $r == "-recover" && $logindex != -1 } {
					puts "Skipping rep$tnum for -recover\
					    with in-memory logs."
					continue
				}
				set envargs ""
				set args $saved_args
				puts "Rep$tnum ($method $envargs $r $c $args):\
				    Test of internal initialization\
				    $msg $msg2 $msg3."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep029_sub $method $niter $tnum $envargs \
				    $l $r $c $args

				# Skip encrypted tests if not supported.
				if { $has_crypto == 0 || $databases_in_memory } {
					continue
				}

				# Run same set of tests with security.
				#
				append envargs " -encryptaes $passwd "
				append args " -encrypt "
				puts "Rep$tnum ($method $envargs $r $c $args):\
				    Test of internal initialization\
				    $msg $msg2 $msg3."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: Client logs are [lindex $l 1]"
				rep029_sub $method $niter $tnum $envargs \
				    $l $r $c $args
			}
		}
	}
}

proc rep029_sub { method niter tnum envargs logset recargs opts largs } {
	global testdir
	global passwd
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
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs $repmemargs \
	    $m_logargs -log_max $log_max $envargs $verbargs $privargs \
	    -errpfx MASTER -home $masterdir \
	    -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd $recargs -rep_master]

	# Open a client
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $repmemargs \
	    $c_logargs -log_max $log_max $envargs $verbargs $privargs \
	    -errpfx CLIENT -home $clientdir \
	    -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd $recargs -rep_client]

	# Bring the clients online by processing the startup messages.
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist 0 NONE err
	error_check_good process_msgs $err 0

	# Create a gap requiring internal initialization.
	set dbhandle NULL
	set cid 2
	if { [lsearch $envargs "-encrypta*"] !=-1 } {
		set flags "-P $passwd"
	} else {
		set flags ""
	}
	set start [push_master_ahead $method $masterenv $masterdir $m_logtype \
	    $clientenv $cid $dbhandle $start $niter $flags $largs]

	puts "\tRep$tnum.b: Reopen client ($opts)."
	if { $opts == "clean" } {
		env_cleanup $clientdir
	}
	if { $opts == "bulk" } {
		error_check_good bulk [$masterenv rep_config {bulk on}] 0
	}
	set clientenv [eval $cl_envcmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $clientenv] TRUE
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist 0 NONE err
	error_check_good process_msgs $err 0
	if { $opts != "clean" } {
		puts "\tRep$tnum.b.1: Trigger log request"
		#
		# When we don't clean, starting the client doesn't
		# trigger any events.  We need to generate some log
		# records so that the client requests the missing
		# logs and that will trigger it.
		#
		set entries 10
		eval rep_test\
		    $method $masterenv NULL $entries $start $start 0 $largs
		incr start $entries
		process_msgs $envlist 0 NONE err
		error_check_good process_msgs $err 0
	}

	puts "\tRep$tnum.c: Verify databases"
	#
	# If doing bulk testing, turn it off now so that it forces us
	# to flush anything currently in the bulk buffer.  We need to
	# do this because rep_test might have aborted a transaction on
	# its last iteration and those log records would still be in
	# the bulk buffer causing the log comparison to fail.
	#
	if { $opts == "bulk" } {
		puts "\tRep$tnum.c.1: Turn off bulk transfers."
		error_check_good bulk [$masterenv rep_config {bulk off}] 0
		process_msgs $envlist 0 NONE err
		error_check_good process_msgs $err 0
	}

	#
	# !!! This test CANNOT use rep_verify for logs due to encryption.
	# Just compare databases.  We either have to copy in
	# all the code in rep_verify to adjust the beginning LSN
	# or skip the log check for just this one test.

	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 0

	# Add records to the master and update client.
	puts "\tRep$tnum.d: Add more records and check again."
	set entries 10
	eval rep_test $method $masterenv NULL $entries $start $start 0 $largs
	incr start $entries
	process_msgs $envlist 0 NONE err
	error_check_good process_msgs $err 0

	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 0

	set bulkxfer [stat_field $masterenv rep_stat "Bulk buffer transfers"]
	if { $opts == "bulk" } {
		error_check_bad bulkxferon $bulkxfer 0
	} else {
		error_check_good bulkxferoff $bulkxfer 0
	}

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}

