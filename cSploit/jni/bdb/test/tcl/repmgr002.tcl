#
# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr002
# TEST	Basic repmgr test.
# TEST
# TEST	Run all combinations of the basic_repmgr_election_test.
# TEST
proc repmgr002 { {display 0} {run 1} args } {

	source ./include.tcl
	if { !$display && $is_freebsd_test == 1 } {
		puts "Skipping replication manager tests on FreeBSD platform."
		return
	}

	run_repmgr_tests election
}
#
# This is the basis for simple repmgr election test cases.  It opens three
# clients of different priorities and makes sure repmgr elects the
# expected master.  Then it shuts the master down and makes sure repmgr
# elects the expected remaining client master.  Then it makes sure the former
# master can join as a client.  The following parameters control 
# runtime options:
#     niter    - number of records to process
#     inmemdb  - put databases in-memory (0, 1)
#     inmemlog - put logs in-memory (0, 1)
#     inmemrep - put replication files in-memory (0, 1)
#     envprivate - put region files in-memory (0, 1)
#     bulk     - use bulk processing (0, 1)
#
proc basic_repmgr_election_test { niter inmemdb \
    inmemlog inmemrep envprivate bulk args } {

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

	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2
	set clientdir3 $testdir/CLIENTDIR3

	file mkdir $clientdir
	file mkdir $clientdir2
	file mkdir $clientdir3

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.  Adjust the args.
	set logtype "on-disk"
	if { $inmemlog } {
		set logtype "in-memory"
	}
	set logargs [adjust_logargs $logtype]
	set txnargs [adjust_txnargs $logtype]

	# Determine in-memory replication argument for environments.  Group
	# Membership needs the "legacy" workaround for in-memory replication
	# files.
	# 
	set repmemarg ""
	set creator_flag "creator"
	set legacy_flag ""
	if { $inmemrep } {
		set repmemarg " -rep_inmem_files "
		set creator_flag ""
		set legacy_flag "legacy"
	}

	# Determine argument for region files, on disk or in-mem.
	set private ""
	if { $envprivate } {
		set private " -private "
	}

	print_repmgr_headers basic_repmgr_election_test $niter $inmemdb\
	    $inmemlog $inmemrep $envprivate $bulk

	puts "\tBasic repmgr election test.a: Start three clients."

	# Open first client
	set cl_envcmd "berkdb_env_noerr -create \
	    $txnargs $verbargs $logargs $private \
	    -errpfx CLIENT -home $clientdir -rep -thread $repmemarg"
	set clientenv [eval $cl_envcmd]
	set cl1_repmgr_conf "-ack all -pri 100 \
	    -local { 127.0.0.1 [lindex $ports 0] \
		$creator_flag $legacy_flag } \
	    -remote { 127.0.0.1 [lindex $ports 1] $legacy_flag } \
	    -remote { 127.0.0.1 [lindex $ports 2] $legacy_flag } \
	    -start elect"
	eval $clientenv repmgr $cl1_repmgr_conf

	# Open second client
	set cl2_envcmd "berkdb_env_noerr -create \
	    $txnargs $verbargs $logargs $private \
	    -errpfx CLIENT2 -home $clientdir2 -rep -thread $repmemarg"
	set clientenv2 [eval $cl2_envcmd]
	set cl2_repmgr_conf "-ack all -pri 30 \
	    -local { 127.0.0.1 [lindex $ports 1] $legacy_flag } \
	    -remote { 127.0.0.1 [lindex $ports 0] $legacy_flag } \
	    -remote { 127.0.0.1 [lindex $ports 2] $legacy_flag } \
	    -start elect"
	eval $clientenv2 repmgr $cl2_repmgr_conf

	puts "\tBasic repmgr election test.b: Elect first client master."
	await_expected_master $clientenv
	set masterenv $clientenv
	set masterdir $clientdir
	await_startup_done $clientenv2

	# Open third client
	set cl3_envcmd "berkdb_env_noerr -create \
	    $txnargs $verbargs $logargs $private \
	    -errpfx CLIENT3 -home $clientdir3 -rep -thread $repmemarg"
	set clientenv3 [eval $cl3_envcmd]
	set cl3_repmgr_conf "-ack all -pri 20 \
	    -local { 127.0.0.1 [lindex $ports 2] $legacy_flag } \
	    -remote { 127.0.0.1 [lindex $ports 0] $legacy_flag } \
	    -remote { 127.0.0.1 [lindex $ports 1] $legacy_flag } \
	    -start elect"
	eval $clientenv3 repmgr $cl3_repmgr_conf
	await_startup_done $clientenv3

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tBasic repmgr election test.c: Run some transactions at master."
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

	puts "\tBasic repmgr election test.d: Verify client database contents."
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1
	rep_verify $masterdir $masterenv $clientdir3 $clientenv3 1 1 1

	puts "\tBasic repmgr election test.e:\
	    Shut down master, elect second client master."
	error_check_good client_close [$clientenv close] 0
	await_expected_master $clientenv2
	set masterenv $clientenv2
	await_startup_done $clientenv3

	# Open -recover to clear env region, including startup_done value.
	# Skip for in-memory logs, since that doesn't work with -recover.
	if { !$inmemlog } {
		puts "\tBasic repmgr election test.f: \
		    Restart former master as client."
		set clientenv [eval $cl_envcmd -recover]
		eval $clientenv repmgr $cl1_repmgr_conf
		await_startup_done $clientenv

		puts "\tBasic repmgr election test.g: \
		    Run some transactions at new master."
		eval rep_test $method $masterenv NULL $niter $niter 0 0 $largs

		puts "\tBasic repmgr election test.h: \
		    Verify client database contents."
		set masterdir $clientdir2
		rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
		rep_verify $masterdir $masterenv $clientdir3 $clientenv3 1 1 1
	}

	# For in-memory replication, verify replication files not there.
	if { $inmemrep } {
		puts "\tBasic repmgr election test.i: \
		    Verify no replication files on disk."
		no_rep_files_on_disk $clientdir
		no_rep_files_on_disk $clientdir2
		no_rep_files_on_disk $clientdir3
	}

	# For private environments, verify region files are not on disk.
	if { $envprivate } {
		puts "\tBasic repmgr election test.j: \
		    Verify no region files on disk."
		no_region_files_on_disk $clientdir
		no_region_files_on_disk $clientdir2
		no_region_files_on_disk $clientdir3
	}

	# Restore original databases_in_memory value. 
	if { $inmemdb } {
		set databases_in_memory $restore_dbinmem
	}

	if { !$inmemlog } {
		error_check_good client_close [$clientenv close] 0
	}
	error_check_good client3_close [$clientenv3 close] 0
	error_check_good client2_close [$clientenv2 close] 0
}

