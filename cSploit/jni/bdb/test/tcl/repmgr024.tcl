# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	repmgr024
# TEST	Test of group-wide log archiving awareness.
# TEST 	Verify that log archiving will use the ack from the clients in
# TEST	its decisions about what log files are allowed to be archived.
#
proc repmgr024 { { niter 50 } { tnum 024 } args } {
	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	# QNX does not support fork() in a multi-threaded environment.
	if { $is_qnx_test } {
		puts "Skipping repmgr$tnum on QNX."
		return
	}

	set method "btree"
	set args [convert_args $method $args]
	#
	# The view option verifies that a view can prevent log files from
	# being archived.
	#
	# The liverem option causes the master to remove one of the clients
	# and makes sure that the master will only hold the log files it needs
	# when the other sites are gone.  Internally, this means the master's
	# sites_avail counter is 0 at the end of the test.
	#
	set testopts { none view liverem }
	foreach t $testopts {
		puts "Repmgr$tnum ($method $t): group wide log archiving."
		repmgr024_sub $method $niter $tnum $t $args
	}
}

proc repmgr024_sub { method niter tnum testopt largs } {
	global testdir
	global util_path
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
	file mkdir [set dira $testdir/SITE_A]
	file mkdir [set dirb $testdir/SITE_B]
	file mkdir [set dirc $testdir/SITE_C]
	foreach { porta portb portc } [available_ports 3] {}

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	set cmda "berkdb_env_noerr -create -txn nosync \
	    $verbargs $repmemargs -rep -thread -event \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_A \
	    -home $dira"
	set enva [eval $cmda]
	# Use quorum ack policy (default, therefore not specified)
	# otherwise it will never wait when the client is closed and
	# we want to give it a chance to wait later in the test.
	$enva repmgr -timeout {connection_retry 5000000} \
	    -local [list 127.0.0.1 $porta] -start master

	# Define envb as a view if needed.
	if { $testopt == "view" } {
		set viewcb ""
		set viewstr "-rep_view \[list $viewcb \]"
	} else {
		set viewstr ""
	}
	set cmdb "berkdb_env_noerr -create -txn nosync \
	    $verbargs $repmemargs -rep -thread -event \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_B \
	    $viewstr -home $dirb"
	set envb [eval $cmdb]
	$envb repmgr -timeout {connection_retry 5000000} \
	    -local [list 127.0.0.1 $portb] -start client \
	    -remote [list 127.0.0.1 $porta]
	puts "\tRepmgr$tnum.a: wait for client B to sync with master."
	await_startup_done $envb

	set cmdc "berkdb_env_noerr -create -txn nosync \
	    $verbargs $repmemargs -rep -thread -event \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_C \
	    -home $dirc"
	set envc [eval $cmdc]
	$envc repmgr -timeout {connection_retry 5000000} \
	    -local [list 127.0.0.1 $portc] -start client \
	    -remote [list 127.0.0.1 $porta]
	puts "\tRepmgr$tnum.b: wait for client C to sync with master."
	await_startup_done $envc


	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$enva test force noarchive_timeout

	set stop 0
	set start 0
	while { $stop == 0 } {
		# Run rep_test in the master.
		puts "\tRepmgr$tnum.c: Running rep_test in replicated env."
		eval rep_test $method $enva NULL $niter $start 0 0 $largs
		incr start $niter

		set res [eval exec $util_path/db_archive -h $dira]
		if { [llength $res] != 0 } {
			set stop 1
		}
	}
	# Save list of files for later.
	set files_arch $res

	set outstr "Close"
	if { $testopt == "liverem" } {
		set outstr "Remove and close"
		$enva repmgr -remove  [list 127.0.0.1 $portc]
		await_event $envc local_site_removed
	}
	puts "\tRepmgr$tnum.d: $outstr client."
	$envc close

	# Now that the client closed its connection, verify that
	# we cannot archive files.
	#
	# When a connection is closed, repmgr updates the 30 second
	# noarchive timestamp in order to give the client process a
	# chance to restart and rejoin the group.  We verify that
	# when the connection is closed the master cannot archive.
	# due to the 30-second timer.
	#
	set res [eval exec $util_path/db_archive -h $dira]
	error_check_good files_archivable_closed [llength $res] 0

	#
	# Clobber the 30-second timer and verify we can again archive the
	# files.
	#
	$enva test force noarchive_timeout
	set res [eval exec $util_path/db_archive -h $dira]
	error_check_good files_arch2 $files_arch $res

	set res [eval exec $util_path/db_archive -l -h $dirc]
	set last_client_log [lindex [lsort $res] end]

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master.
		puts "\tRepmgr$tnum.e: Running rep_test in replicated env."
		eval rep_test $method $enva NULL $niter $start 0 0 $largs
		incr start $niter

		# We use log_archive when we want to remove log files so
		# that if we are running verbose, we get all of the output
		# we might need.
		#
		# However, we can use db_archive for all of the other uses
		# we need such as getting a list of what log files exist in
		# the environment.
		#
		puts "\tRepmgr$tnum.f: Run log_archive on master."
		set res [$enva log_archive -arch_remove]
		set res [eval exec $util_path/db_archive -l -h $dira]
		if { [lsearch -exact $res $last_client_log] == -1 } {
			set stop 1
		}
	}

	#
	# Get the new last log file for client 1.
	#
	set res [eval exec $util_path/db_archive -l -h $dirb]
	set last_client_log [lindex [lsort $res] end]

	#
	# Set test hook to prevent client 1 from sending any ACKs,
	# but remaining alive.
	#
	puts "\tRepmgr$tnum.g: Turn off acks via test hook on remaining client."
	$envb test abort repmgr_perm

	#
	# Advance logfiles again.
	puts "\tRepmgr$tnum.h: Advance master log files."
	set start [repmgr024_advlog $method $niter $start \
	    $enva $dira $last_client_log $largs]

	#
	# Make sure neither log_archive in same process nor db_archive
	# in a different process show any files to archive.
	#
	error_check_good no_files_log_archive [llength [$enva log_archive]] 0
	set dbarchres [eval exec $util_path/db_archive -h $dira]
	error_check_good no_files_db_archive [llength $dbarchres] 0

	puts "\tRepmgr$tnum.i: Try to archive. Verify it didn't."
	set res [$enva log_archive -arch_remove]
	set res [eval exec $util_path/db_archive -l -h $dira]
	error_check_bad cl1_archive1 [lsearch -exact $res $last_client_log] -1
	#
	# Turn off test hook preventing acks.  Then run a perm operation
	# so that the client can send its ack.
	#
	puts "\tRepmgr$tnum.j: Enable acks and archive again."
	$envb test abort none
	$enva txn_checkpoint -force

	#
	# Advance logfiles for the view to generate a file change ack.
	#
	if { $testopt == "view" } {
		set start [repmgr024_advlog $method $niter $start \
		    $enva $dira $last_client_log $largs]
	}

	#
	# Pause to allow time for the ack to arrive at the master.  If we
	# happen to be at a log file boundary, the ack must arrive before
	# doing the stable_lsn check for the next archive operation.
	#
	tclsleep 2

	#
	# Now archive again and make sure files were removed.
	#
	set res [$enva log_archive -arch_remove]
	set res [eval exec $util_path/db_archive -l -h $dira]
	error_check_good cl1_archive2 [lsearch -exact $res $last_client_log] -1

	#
	# Close last remaining client so that master gets no acks.
	# When a connection is closed, repmgr updates the 30 second
	# noarchive timestamp in order to give the client process a
	# chance to restart and rejoin the group.  We verify that
	# when the connection is closed the master cannot archive.
	# due to the 30-second timer.
	# 
	puts "\tRepmgr$tnum.k: Close client."
	set res [eval exec $util_path/db_archive -l -h $dirb]
	set last_client_log [lindex [lsort $res] end]
	$envb close

	#
	# Advance logfiles again.
	puts "\tRepmgr$tnum.l: Advance master log files."
	set start [repmgr024_advlog $method $niter $start \
	    $enva $dira $last_client_log $largs]

	#
	# Make sure neither log_archive in same process nor db_archive
	# in a different process show any files to archive.
	#
	puts "\tRepmgr$tnum.m: Try to archive. Verify it didn't."
	error_check_good no_files_log_archive2 [llength [$enva log_archive]] 0
	set dbarchres [eval exec $util_path/db_archive -h $dira]
	error_check_good no_files_db_archive2 [llength $dbarchres] 0

	set res [$enva log_archive -arch_remove]
	set res [eval exec $util_path/db_archive -l -h $dira]
	error_check_bad cl1_archive3 [lsearch -exact $res $last_client_log] -1

	#
	# Clobber the 30-second timer and verify we can again archive the
	# files.
	#
	$enva test force noarchive_timeout
	#
	# Now archive again and make sure files were removed.
	#
	puts "\tRepmgr$tnum.n: After clobbering timer verify we can archive."
	set res [$enva log_archive -arch_remove]
	set res [eval exec $util_path/db_archive -l -h $dira]
	error_check_good cl1_archive4 [lsearch -exact $res $last_client_log] -1

	$enva close
}

proc repmgr024_advlog { method niter initstart menv mdir lclog largs } {
	global util_path

	# Advance master logfiles until they are past last client log.
	set retstart $initstart
	set stop 0
	while { $stop == 0 } {
		# Run rep_test on master.
		eval rep_test $method $menv NULL $niter $retstart 0 0 $largs
		incr retstart $niter

		# Run db_archive on master.
		set res [eval exec $util_path/db_archive -l -h $mdir]
		set last_master_log [lindex [lsort $res] end]
		if { $last_master_log > $lclog } {
			set stop 1
		}
	}
	return $retstart
}
