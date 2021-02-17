#
# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr001
# TEST	Basic repmgr test.
# TEST
# TEST	Run all mix-and-match combinations of the basic_repmgr_test.
# TEST
proc repmgr001 { {display 0} {run 1} args } {

	source ./include.tcl
	if { !$display && $is_freebsd_test == 1 } {
		puts "Skipping replication manager tests on FreeBSD platform."
		return
	}

	run_repmgr_tests basic
}

# This is the basis for a number of simple repmgr test cases. It creates
# an appointed master and two clients, calls rep_test to process some records 
# and verifies the resulting databases. The following parameters control 
# runtime options:
#     niter    - number of records to process
#     inmemdb  - put databases in-memory (0, 1)
#     inmemlog - put logs in-memory (0, 1)
#     inmemrep - put replication files in-memory (0, 1)
#     envprivate - put region files in-memory (0, 1)
#     bulk     - use bulk processing (0, 1)
#
proc basic_repmgr_test { niter inmemdb inmemlog \
    inmemrep envprivate bulk args } {

	source ./include.tcl
	global rep_verbose 
	global verbose_type
	global overflowword1
	global overflowword2
	global databases_in_memory
	set overflowword1 "0"
	set overflowword2 "0"
	set nsites 3

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set largs [convert_args $method $args]
	
	# Set databases_in_memory for this test, preserving original value.
	if { $inmemdb } {
		set restore_dbinmem $databases_in_memory
		set databases_in_memory 1
	}

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.  Adjust the args.
	set logtype "on-disk"
	if { $inmemlog } {
		set logtype "in-memory"
	}
	set logargs [adjust_logargs $logtype]
	set txnargs [adjust_txnargs $logtype]

	# Determine in-memory replication argument for environments.
	if { $inmemrep } {
		set repmemarg "-rep_inmem_files "
	} else {
		set repmemarg ""
	}

	# Determine argument for region files (env -private). 
	if { $envprivate } {
		set private " -private"
	} else {
		set private "" 
	}

	# Call the generalized header routine.
	print_repmgr_headers basic_repmgr_test $niter $inmemdb \
	    $inmemlog $inmemrep $envprivate $bulk 

	# Open a master.
	puts "\tBasic repmgr test.a: Start an appointed master."
	set ma_envcmd "berkdb_env_noerr -create $logargs $verbargs \
	    $private \
	    -errpfx MASTER -home $masterdir $txnargs -rep -thread \
	    -lock_max_locks 10000 -lock_max_objects 10000 $repmemarg"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	# Open first client
	puts "\tBasic repmgr test.b: Start first client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs $logargs \
	    $private \
	    -errpfx CLIENT -home $clientdir $txnargs -rep -thread \
	    -lock_max_locks 10000 -lock_max_objects 10000 $repmemarg"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	# Open second client
	puts "\tBasic repmgr test.c: Start second client."
	set cl2_envcmd "berkdb_env_noerr -create $verbargs $logargs \
	    $private \
	    -errpfx CLIENT2 -home $clientdir2 $txnargs -rep -thread \
	    -lock_max_locks 10000 -lock_max_objects 10000 $repmemarg"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all  \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 1]] \
	    -start client
	await_startup_done $clientenv2

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tBasic repmgr test.d: Run some transactions at master."
	if { $bulk } {
		# Turn on bulk processing on master.
		error_check_good set_bulk [$masterenv rep_config {bulk on}] 0

		eval rep_test_bulk $method $masterenv NULL $niter 0 0 0 $largs

		# Must turn off bulk because some configs (debug_rop/wop)
		# generate log records when verifying databases.
		error_check_good set_bulk [$masterenv rep_config {bulk off}] 0
	} else {
		eval rep_test $method $masterenv NULL $niter 0 0 0 $largs
	}

	puts "\tBasic repmgr test.e: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1

	# For in-memory replication, verify replication files not there.
	if { $inmemrep } {
		puts "\tBasic repmgr test.f: Verify no rep files on disk."
		no_rep_files_on_disk $masterdir
		no_rep_files_on_disk $clientdir
		no_rep_files_on_disk $clientdir2
	}

	# For private envs, verify no region files on disk.
	if { $envprivate } {
		puts "\tBasic repmgr test.g: Verify no region files on disk."
		no_region_files_on_disk $masterdir
		no_region_files_on_disk $clientdir
		no_region_files_on_disk $clientdir2
	}

	# Restore original databases_in_memory value.
	if { $inmemdb } {
		set databases_in_memory $restore_dbinmem
	}

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}

