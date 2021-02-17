# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	repmgr028
# TEST	Repmgr allows applications to choose master explicitly, instead of
# TEST	relying on elections.

proc repmgr028 { { tnum 028 } } {
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

	puts "Repmgr$tnum: Repmgr applications may choose master explicitly"
	repmgr028_sub $tnum
}

proc repmgr028_sub { tnum } {
	global testdir 
	global tclsh_path
	global test_path
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
	foreach { porta portb } [available_ports 2] {}

	set common "-create -txn $verbargs $repmemargs \
	    -rep -thread -event"
	set common_mgr "-msgth 2 -timeout {connection_retry 3000000} \
	     -timeout {election_retry 3000000}"
	set cmda "berkdb_env_noerr $common -errpfx SITE_A -home $dira"
	set cmdb "berkdb_env_noerr $common -errpfx SITE_B -home $dirb"
	set enva [eval $cmda]
	eval $enva repmgr -local {[list 127.0.0.1 $porta]} -start master
	set envb [eval $cmdb]
	eval $envb repmgr -start client \
	    -local {[list 127.0.0.1 $portb]} -remote {[list 127.0.0.1 $porta]}
	await_startup_done $envb
	$envb close
	$enva close
	
	# Create a replication group of 2 sites, configured not to use
	# elections.  Even with this configuration, an "initial" election is
	# allowed, so try that to make sure it works.
	# 
	puts "\tRepmgr$tnum.a: Start two sites."
	set enva [eval $cmda -recover]
	$enva rep_config {mgrelections off}
	eval $enva repmgr $common_mgr \
	    -local {[list 127.0.0.1 $porta]} -start elect -pri 100

	set envb [eval $cmdb -recover]
	$envb rep_config {mgrelections off}
	eval $envb repmgr $common_mgr -start elect -pri 99 \
	    -local {[list 127.0.0.1 $portb]}
	await_startup_done $envb

	puts "\tRepmgr$tnum.b: Switch roles explicitly."
	$enva repmgr -start client -msgth 0
	# Allow time for envb to process enva's NEWCLIENT message.  On some
	# platforms, this message processing can lock out envb's attempt to
	# start as master.
	tclsleep 1
	$envb repmgr -start master -msgth 0
	await_startup_done $enva

	# Check that "-start elect" is forbidden when called as a dynamic
	# change, at either master or client.
	# 
	error_check_bad disallow_elect_restart \
	    [catch {$enva repmgr -start elect -msgth 0}] 0
	error_check_bad disallow_elect_restart \
	    [catch {$envb repmgr -start elect -msgth 0}] 0

	# Kill master, and observe that client does not react by starting an
	# election.  Before doing so, reset client's stats, so that the later
	# comparison against 0 makes sense.
	# 
	puts "\tRepmgr$tnum.c: Kill master"
	$enva rep_stat -clear
	$envb close

	# The choice of 5 seconds is arbitrary, not related to any configured
	# timeouts, and is simply intended to allow repmgr's threads time to
	# react.  We presume the system running this test isn't so horribly
	# overloaded that repmgr's threads can't get scheduled for that long.
	# 
	puts "\tRepmgr$tnum.d: Pause 5 seconds to observe (lack of) reaction."
	tclsleep 5

	error_check_good event \
	    [is_event_present $enva master_failure] 1

	error_check_good no_election_in_progress \
	    [stat_field $enva rep_stat "Election phase"] 0
	error_check_good no_elections_held \
	    [stat_field $enva rep_stat "Elections held"] 0

	# bring master back up, wait for client to get start up done (clear
	# event first).  make client a master, and observe dupmaster event.
	# check that both are then client role.  and again no election!
	# 
	puts "\tRepmgr$tnum.e: Restart master (wait for client to sync)."
	set orig_gen [stat_field $enva rep_stat "Generation number"]
	$enva event_info -clear
	set envb [eval $cmdb -recover]
	$envb rep_config {mgrelections off}
	eval $envb repmgr $common_mgr -start master \
	    -local {[list 127.0.0.1 $portb]}
	
	# Force a checkpoint so that the client hears something from the master,
	# which should cause the client to notice the gen number change.  Try a
	# few times, in case we're not quite completely connected at first.
	# 
	$envb event_info -clear
	$envb repmgr -ack all
	set tried 0
	set done false
	while {$tried < 10 && !$done} {
		tclsleep 1
		$envb txn_checkpoint -force
		if {![is_event_present $envb perm_failed]} {
			set done true
		}
		incr tried
		$envb event_info -clear
	}

	await_condition {[stat_field $enva rep_stat "Generation number"] \
			     > $orig_gen}
	await_startup_done $enva

	puts "\tRepmgr$tnum.f: Set master at other site, leading to dupmaster."
	$enva repmgr -start master -msgth 0
	tclsleep 5
	error_check_good dupmaster_event \
	    [is_event_present $envb dupmaster] 1
	error_check_good dupmaster_event2 \
	    [is_event_present $enva dupmaster] 1
	error_check_good role \
	    [stat_field $enva rep_stat "Role"] "client"
	error_check_good role2 \
	    [stat_field $envb rep_stat "Role"] "client"
	error_check_good no_election_in_progress2 \
	    [stat_field $enva rep_stat "Election phase"] 0
	error_check_good no_elections_held2 \
	    [stat_field $enva rep_stat "Elections held"] 0

	# Turn on elections mode at just one of the sites.  This should cause
	# the site to initiate an election, since it currently lacks a master.
	# If a (strict) election can succeed, this also tests the rule that the
	# other site accepts an invitation to an election even when it is not in
	# elections mode itself.
	#
	puts "\tRepmgr$tnum.g: Turn on elections mode dynamically."
	$envb event_info -clear
	$enva rep_config {mgr2sitestrict on}
	$enva rep_config {mgrelections on}
	await_condition {[is_elected $envb] || \
			     [is_event_present $envb newmaster]}
	error_check_good elections_held \
	    [expr [stat_field $enva rep_stat "Elections held"] > 0] 1
	error_check_good elections_held2 \
	    [expr [stat_field $envb rep_stat "Elections held"] > 0] 1

	# Make sure that changing the "elections" config is not allowed in a
	# subordinate replication process:
	# 
	set resultfile "$testdir/repmgr028script.log"
	exec $tclsh_path $test_path/wrap.tcl repmgr028script.tcl $resultfile
	set file [open $resultfile r]
	set result [read -nonewline $file]
	close $file
	error_check_good subprocess_script_result $result "OK"

	$enva close
	$envb close

	# Try a traditional set-up, and verify that dynamic role change is
	# forbidden.
	# 
	puts "\tRepmgr$tnum.h: Start up again, elections on by default."
	set enva [eval $cmda -recover]
	eval $enva repmgr $common_mgr \
	    -local {[list 127.0.0.1 $porta]} -start master
	set envb [eval $cmdb -recover]
	eval $envb repmgr $common_mgr -start client \
	    -local {[list 127.0.0.1 $portb]}
	await_startup_done $envb

	puts "\tRepmgr$tnum.i: Check that dynamic role change attempt fails."
	error_check_bad disallow_role_chg \
	    [catch {$enva repmgr -start client -msgth 0}] 0
	error_check_bad disallow_role_chg_b \
	    [catch {$envb repmgr -start master -msgth 0}] 0

	# Close master, observe that client tries an election, and gets the
	# event info.
	# 
	$envb rep_config {mgr2sitestrict on}
	error_check_bad event2 \
	    [is_event_present $envb master_failure] 1
	$enva close
	tclsleep 5
	error_check_good event3 \
	    [is_event_present $envb master_failure] 1
	
	error_check_good election \
	    [expr [stat_field $envb rep_stat "Election phase"] != 0 || \
		 [stat_field $envb rep_stat "Elections held"] > 0] 1

	await_condition {[is_event_present $envb election_failed]}
	$envb close

	# Check that "client" start policy suppresses elections, even if
	# elections mode has *NOT* been turned off for the general case.  (This
	# is old existing behavior which previously lacked a test.)
	# 
	set enva [eval $cmda -recover]
	eval $enva repmgr $common_mgr \
	    -local {[list 127.0.0.1 $porta]} -start client
	set envb [eval $cmdb -recover]
	eval $envb repmgr $common_mgr -start client \
	    -local {[list 127.0.0.1 $portb]}
	puts "\tRepmgr$tnum.j: Pause 10 seconds, check no election held."
	tclsleep 10
	error_check_good no_election \
	    [expr [stat_field $enva rep_stat "Election phase"] == 0 && \
		 [stat_field $enva rep_stat "Elections held"] == 0] 1
	error_check_good no_election2 \
	    [expr [stat_field $envb rep_stat "Election phase"] == 0 && \
		 [stat_field $envb rep_stat "Elections held"] == 0] 1
	$enva close
	$envb close

	# Check that "election" start policy starts an election when
	# a site that was previously a client starts up without recovery
	# and without finding a master.  This is another general test case 
	# where elections mode is *NOT* turned off. 
	#
	puts "\tRepmgr$tnum.k: Test election start policy on client startup."
	set enva [eval $cmda -recover]
	eval $enva repmgr $common_mgr \
	    -local {[list 127.0.0.1 $porta]} -start master
	$enva rep_config {mgr2sitestrict on}
	set envb [eval $cmdb -recover]
	eval $envb repmgr $common_mgr -start client \
	    -local {[list 127.0.0.1 $portb]} -remote {[list 127.0.0.1 $porta]}
	await_startup_done $envb
	$envb rep_config {mgr2sitestrict on}
	$envb close
	$enva close

	# Restart previous client with election start policy.  The
	# 2site_strict setting will cause the election to fail, but we
	# only care that the election was initiated.
	#
	set envb [eval $cmdb]
	eval $envb repmgr $common_mgr -start elect \
	    -local {[list 127.0.0.1 $portb]} -remote {[list 127.0.0.1 $porta]}
	puts "\tRepmgr$tnum.l: Pause 5 seconds, check election was attempted."
	tclsleep 5
	error_check_good startup_election \
	    [expr [stat_field $envb rep_stat "Election phase"] != 0 || \
		 [stat_field $envb rep_stat "Elections held"] > 0] 1
	await_condition {[is_event_present $envb election_failed]}
	$envb close
}
