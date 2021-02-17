# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep090
# TEST	Test of AUTO_REMOVE on both master and client sites.
# TEST
# TEST	One master, one client.  Set AUTO_REMOVE on the client env.
# TEST	Generate several log files.
# TEST	Verify the client has properly removed the log files.
# TEST	Turn on AUTO_REMOVE on the master and generate more log files.
# TEST	Confirm both envs have the same log files.
#
proc rep090 { method { niter 50 } { tnum "090" } args } {
	source ./include.tcl
	global databases_in_memory
	global mixed_mode_logging
	global repfiles_in_memory

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "\tRep$tnum: Skipping for method $method."
		return
	}
	if { $databases_in_memory } {
		puts "\tRep$tnum: Skipping for in-memory databases."
		return
	}
	if { $repfiles_in_memory } {
		puts "\tRep$tnum: Skipping for in-memory replication files."
		return
	}
	if { $mixed_mode_logging != 0 } {
		puts "\tRep$tnum: Skipping for in-memory log files."
		return
	}

	set args [convert_args $method $args]

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
        if { $pgindex != -1 } {
                puts "Rep$tnum: skipping for specific pagesizes"
                return
        }

	#
	# Even though we skip for in-memory logs, keep the standard
	# log configuration format for Tcl consistent with all other tests.
	#
	set logsets [create_logsets 2]
	set msgopts { dropped normal }

	# Run with options to drop some messages or normal/all messages.
	foreach l $logsets {
		foreach m $msgopts {
			puts "Rep$tnum ($method $args): Test of\
			    client log autoremove with $m messages."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: Client logs are [lindex $l 1]"
			rep090_sub $method $niter $tnum $l $m $args
		}
	}
}

proc rep090_sub { method niter tnum logset msgopt largs } {
	global drop drop_msg
	global rep_verbose
	global testdir
	global util_path
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
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

	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	# Don't turn on autoremove yet on the master.
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $m_logargs -log_max $log_max -errpfx MASTER $verbargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd -rep_master]

	puts "\tRep$tnum.a: Open client and set autoremove."
	repladd 2
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs \
	    $c_logargs -log_max $log_max -errpfx CLIENT $verbargs \
	    -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd -rep_client]
	$clientenv log_config autoremove on
	set envlist "{$masterenv 1} {$clientenv 2}"

	set start 0
	set testfile "test.db"

	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set mdb [eval {berkdb_open_noerr} -env $masterenv -auto_commit\
		-create -mode 0644 $omethod $dbargs $testfile ]
	error_check_good reptest_db [is_valid_db $mdb] TRUE

	process_msgs $envlist
	puts "\tRep$tnum.b: Running rep_test in replicated env."
	set stop 0
	set logstop 5
	#
	# If we're dropping messages we want to set it up so that the
	# rerequest happens after our loop is finished.  Set the
	# rerequest values high, 10 and 20 seconds.
	#
	if { $msgopt == "dropped" } {
		set req_min 10000000
		set req_max 20000000
		$clientenv rep_request $req_min $req_max
		set drop 1
		# Drop 5% of messages, 100/5 or every 20th.
		set drop_msg 20
	}
	while { $stop == 0 } {
		# Run rep_test in the master to $logstop log files.
		eval rep_test $method \
		    $masterenv $mdb $niter $start $start 0 $largs
		incr start $niter
		process_msgs $envlist
		#
		# Run until we have at least $logstop log files on the master.
		#
		set last_master_log [get_logfile $masterenv last]
		if { $last_master_log >= $logstop} {
			set stop 1
		}
	}
	#
	# Since the client has autoremove turned on, it should have only
	# one log file whether we're dropping messages or not.
	#
	# Flush the logs first to ensure they are on disk.
	#
	$clientenv log_flush
	puts "\tRep$tnum.c: Verify client log file removal."
	set cl_logs [eval exec $util_path/db_archive -l -h $clientdir]
	error_check_good cllog [llength $cl_logs] 1

	if { $msgopt == "dropped" } {
		#
		# If we catch everything up now, the client
		# will still have only one log file, but it
		# will be a different one. 
		#
		# Turn off dropped messages.
		# Sleep beyond max rerequest time.
		# Force a message to generate rerequest.
		# Process messages
		# Verify logs are autoremoved.
		#
		set drop 0
		set slp [expr $req_max / 1000000]
		puts "\tRep$tnum.c.0: Sleep beyond rerequest time ($slp sec)."
		tclsleep $slp

		puts "\tRep$tnum.c.1: Generate message."
		$masterenv rep_start -master
		process_msgs $envlist

		puts "\tRep$tnum.c.2: Verify client log file removal now."
		$clientenv log_flush
		set new_cl_logs [eval exec $util_path/db_archive -l -h $clientdir]
		error_check_bad new_cllog $cl_logs $new_cl_logs
		error_check_good cllog [llength $cl_logs] 1
	}

	#
	# Turn on autoremove on the master and advance past the end of
	# the current log file to cause removal of all earlier logs.
	# Also clobber replication's 30-second anti-archive timer.
	#
	puts "\tRep$tnum.d: Turn on autoremove on master."
	$masterenv log_config autoremove on
	$masterenv test force noarchive_timeout
	set last_master_log [get_logfile $masterenv last]

	puts "\tRep$tnum.e: Running rep_test in replicated env."
	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master beyond former last log.
		eval rep_test $method \
		    $masterenv $mdb $niter $start $start 0 $largs
		incr start $niter
		process_msgs $envlist
		#
		# Run until we've advanced past the old last log.
		# Notice we're letting autoremove do the work, 
		# not db_archive.
		#
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > $last_master_log } {
			set stop 1
		}
	}
	puts "\tRep$tnum.f: Verify both sites file removal."
	$masterenv log_flush
	set ma_logs [eval exec $util_path/db_archive -l -h $masterdir]
	error_check_good malog_1 [llength $ma_logs] 1

	$clientenv log_flush
	set cl_logs [eval exec $util_path/db_archive -l -h $clientdir]
	error_check_good cllog2_1 [llength $cl_logs] 1

	# Make sure both sites have the exact same logs.
	error_check_good match_logs $ma_logs $cl_logs

	error_check_good mdb_close2 [$mdb close] 0
	error_check_good masterenv_close [$masterenv close] 0
	error_check_good clientenv_close [$clientenv close] 0
	replclose $testdir/MSGQUEUEDIR
}


