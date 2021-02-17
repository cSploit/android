# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	repmgr032
# TEST	The (undocumented) AUTOROLLBACK config feature.

proc repmgr032 { { tnum 032 } args } {
	source ./include.tcl
	global databases_in_memory

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	# This test needs to have explicit control over whether/which databases
	# are on disk versus in memory.
	# 
	if { $databases_in_memory } {
		puts "Skipping repmgr$tnum for databases_in_memory"
		return
	}

	set method "btree"
	set args [convert_args $method $args]
	foreach test_case {normal do_close nimdbs} {
		puts "Repmgr$tnum: preventing auto-rollback, $test_case case"
		repmgr032_sub $method $tnum $test_case $args
	}
}

proc repmgr032_sub { method tnum test_case largs } {
	global testdir 
	global repfiles_in_memory
	global rep_verbose
	global verbose_type
	
	switch $test_case {
		normal { 
			set do_close false 
			set nimdbs false
		}
		do_close {
			set do_close true
			set nimdbs false
		}
		nimdbs {
			set do_close true
			set nimdbs true
		}
	}

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

	set common "-create -txn $verbargs $repmemargs \
	    -rep -thread -event "
	set common_mgr "-msgth 2"

	puts "\tRepmgr$tnum.a: Start sites."
	set cmda "berkdb_env_noerr $common -errpfx SITE_A -home $dira"
	set enva [eval $cmda]
	$enva rep_config {mgrelections off}
	eval $enva repmgr $common_mgr -ack allavailable \
	    -local {[list 127.0.0.1 $porta]} -start master

	if { $nimdbs } {
		set omethod [convert_method $method]
		set db [eval {berkdb_open_noerr} -env $enva -auto_commit\
		     -create -mode 0644 $omethod $largs {"" nim.db}]
		set niter 10
		eval rep_test $method $enva $db $niter 0 0 0 $largs
		$db close
	}

	set cmdb "berkdb_env_noerr $common -errpfx SITE_B -home $dirb"
	set envb [eval $cmdb]
	$envb rep_config {mgrelections off}
	eval $envb repmgr $common_mgr -start client \
	    -local {[list 127.0.0.1 $portb]} -remote {[list 127.0.0.1 $porta]}
	await_startup_done $envb

	set cmdc "berkdb_env_noerr $common -errpfx SITE_C -home $dirc"
	set envc [eval $cmdc]
	$envc rep_config {mgrelections off}
	$envc rep_config {autorollback off}
	eval $envc repmgr $common_mgr -start client \
	    -local {[list 127.0.0.1 $portc]} -remote {[list 127.0.0.1 $porta]}
	await_startup_done $envc

	puts "\tRepmgr$tnum.b: Run some transactions at master."
	set niter 20
	eval rep_test $method $enva NULL $niter 0 0 0 $largs

	# Remember where we are in the log.  Use a log cursor to read the last
	# record in the log, then do one more transaction, then read the log end
	# again.  This gives us a lower and upper bound on where the sync point
	# should be.
	# 
	set logc [$enva log_cursor]
	set lower [lindex [$logc get -last] 0]
	eval rep_test $method $enva NULL 1 0 0 0 $largs
	set upper [lindex [$logc get -last] 0]
	$logc close

	puts "\tRepmgr$tnum.c: Shut down client B, then do a few more txns"
	$envb close
	eval rep_test $method $enva NULL $niter 0 0 0 $largs

	puts "\tRepmgr$tnum.d: Kill master, and restart client B as new master"
	if { $do_close } {
		$envc close
	}
	$enva close
	# maybe wait for masterfailure event at env b?
	set envb [eval $cmdb]
	$envb rep_config {mgrelections off}
	eval $envb repmgr $common_mgr -start master \
	    -local {[list 127.0.0.1 $portb]} -remote {[list 127.0.0.1 $portc]}

	if { $do_close } {
		set envc [eval $cmdc -recover]
		$envc rep_config {autorollback off}
		eval $envc repmgr $common_mgr -start client \
		    -local {[list 127.0.0.1 $portc]} \
		    -remote {[list 127.0.0.1 $portb]}
	}
	puts "\tRepmgr$tnum.e: Wait for WOULD_ROLLBACK event."
	await_condition {[is_event_present $envc would_rollback]}

	set sync_point [lindex [find_event [$envc event_info] would_rollback] 1]
	puts "\tRepmgr$tnum.f: Reported sync point is $sync_point"

	error_check_good lower_bound [$envb log_compare $lower $sync_point] -1

	# The upper bound must be >= to the sync point, which means log_compare
	# must *NOT* return a -1.  The other possible values (0, 1) are OK.
	# 
	error_check_bad upper_bound [$envb log_compare $upper $sync_point] -1

	$envc close
	$envb close
	set be_quiet ""
}
