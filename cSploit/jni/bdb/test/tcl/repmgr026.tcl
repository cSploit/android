# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	repmgr026
# TEST	Test of "full election" timeouts.
# TEST	1. Cold boot with all sites present.
# TEST	2. Cold boot with some sites missing.
# TEST	3. Partial-participation election with one client having seen a master,
# TEST	   but another just starting up fresh.
# TEST	4. Partial participation, with all participants already having seen a
# TEST	   master.
# TEST

proc repmgr026 { { tnum 026 } } {
	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	foreach use_leases {no yes} {
		foreach client_down {no yes} {
			puts "Repmgr$tnum: Full election test, \
			    client_down: $client_down; leases: $use_leases"
			repmgr026_sub $tnum $client_down $use_leases
		}
	}
}

proc repmgr026_sub { tnum client_down use_leases } {
	global testdir
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
	file mkdir [set dird $testdir/SITE_D]
	file mkdir [set dire $testdir/SITE_E]
	foreach { porta portb portc portd porte } [available_ports 5] {}

	# First, just create/establish the group.
	puts -nonewline "Repmgr$tnum: Create a group of 5 sites: "
	set common "-create -txn $verbargs $repmemargs \
	    -rep -thread -event"
	if { $use_leases } {
		append common " -rep_lease {[list 3000000]} "
	}
	set cmda "berkdb_env_noerr $common -errpfx SITE_A -home $dira"
	set cmdb "berkdb_env_noerr $common -errpfx SITE_B -home $dirb"
	set cmdc "berkdb_env_noerr $common -errpfx SITE_C -home $dirc"
	set cmdd "berkdb_env_noerr $common -errpfx SITE_D -home $dird"
	set cmde "berkdb_env_noerr $common -errpfx SITE_E -home $dire"
	set common_mgr " -start elect \
	    -timeout {connection_retry 5000000} \
	    -timeout {election_retry 2000000} \
	    -timeout {full_election 60000000} \
	    -timeout {election 5000000} -timeout {ack 3000000}"
	set enva [eval $cmda]
	eval $enva repmgr $common_mgr  \
	    -local {[list 127.0.0.1 $porta creator]}
	puts -nonewline "." ; 	flush stdout
	set envb [eval $cmdb]
	eval $envb repmgr $common_mgr \
	    -local {[list 127.0.0.1 $portb]} -remote {[list 127.0.0.1 $porta]}
	await_startup_done $envb
	puts -nonewline "." ; 	flush stdout
	set envc [eval $cmdc]
	eval $envc repmgr $common_mgr \
	    -local {[list 127.0.0.1 $portc]} -remote {[list 127.0.0.1 $porta]}
	await_startup_done $envc
	puts -nonewline "." ; 	flush stdout
	set envd [eval $cmdd]
	eval $envd repmgr $common_mgr \
	    -local {[list 127.0.0.1 $portd]} -remote {[list 127.0.0.1 $porta]}
	await_startup_done $envd
	puts -nonewline "." ; 	flush stdout
	set enve [eval $cmde]
	eval $enve repmgr $common_mgr \
	    -local {[list 127.0.0.1 $porte]} -remote {[list 127.0.0.1 $porta]}
	await_startup_done $enve
	puts "."
	$enve close
	$envd close
	$envc close
	$envb close
	$enva close

	# Cold boot the group (with or without site E), giving site A a
	# high priority.
	# 

	# The wait_limit's are intended to be an amount that is way more than
	# the expected timeout, used for nothing more than preventing the test
	# from hanging forever.  The leeway amount should be enough less than
	# the timeout to allow for any imprecision introduced by the test
	# mechanism.
	# 
	set elect_wait_limit 25
	set full_secs_leeway 59
	set full_wait_limit 85

	puts "\tRepmgr$tnum.a: Start first four sites."
	set enva [eval $cmda]
	eval $enva repmgr $common_mgr -pri 200 -local {[list 127.0.0.1 $porta]}

	set envb [eval $cmdb]
	eval $envb repmgr $common_mgr -pri 100 -local {[list 127.0.0.1 $portb]}

	set envc [eval $cmdc]
	eval $envc repmgr $common_mgr -pri 90 -local {[list 127.0.0.1 $portc]}

	set envd [eval $cmdd]
	eval $envd repmgr $common_mgr -pri 80 -local {[list 127.0.0.1 $portd]}

	if { $client_down } {
		set enve NONE
	} else {
		puts "\tRepmgr$tnum.b: Start fifth site."
		set enve [eval $cmde]
		eval $enve repmgr $common_mgr -pri 50 \
		    -local {[list 127.0.0.1 $porte]}
	}

	# wait for results, and make sure they're correct
	#
	set envlist [list $enva $envb $envc $envd]
	if { $enve != "NONE" } {
		lappend envlist $enve
	}
	set limit $full_wait_limit
	puts "\tRepmgr$tnum.c: wait (up to $limit seconds) for first election."
	set t [repmgr026_await_election_result $envlist $limit]
	if { $client_down } {
		error_check_good slow_election [expr $t > $full_secs_leeway] 1
	} else {
		# When all sites participate, the election should finish in way
		# less than 60 seconds.
		# 
		error_check_good timely_election [expr $t < $full_secs_leeway] 1
	}
	puts "\tRepmgr$tnum.d: first election completed in $t seconds"

	puts "\tRepmgr$tnum.e: wait for start-up done"
	$enva event_info -clear
	await_startup_done $envb
	$envb event_info -clear
	await_startup_done $envc
	$envc event_info -clear
	await_startup_done $envd
	$envd event_info -clear
	if { $enve != "NONE" } {
		await_startup_done $enve
		$enve event_info -clear
	}

	# Shut down site A, in order to test elections with less than the whole
	# group voting.  However, normally repmgr's reaction to losing master
	# connection is to try a "fast election" (the n-1 trick).  So we must do
	# something to mitigate that (see below).
	# 
	puts "\tRepmgr$tnum.f: shut down master site A"
	if { $client_down } {
		# The fifth site is already down, so now we'll have just B, C,
		# and D running.  Therefore, even with repmgr pulling its "fast
		# election" (n-1) trick, we don't have enough votes for a
		# full-participation short circuit; so this is a valid test of
		# the "normal" election timeout.
		#
		$enva close
	} else {
		# Here all sites are running, so if we just killed the master
		# repmgr would invoke its "fast election" trick, resulting in no
		# timeout.  Since the purpose of this test is to ensure the
		# correct use of timeouts, that's no good.  Instead, let's first
		# kill one more other site.
		$enve close
		$enva close
	}

	# wait for results, and check them
	# 
	set envlist [list $envb $envc $envd]
	set limit $elect_wait_limit
	puts "\tRepmgr$tnum.h: wait (up to $limit seconds) for second election."
	set t [repmgr026_await_election_result $envlist $limit]
	error_check_good normal_election [expr $t < $full_secs_leeway] 1
	puts "\tRepmgr$tnum.i: second election completed in $t seconds"

	$envd close
	$envc close
	$envb close
}

# Wait (a limited amount of time) for the election to finish.  The first env
# handle in the list is the expected winner, and the others are the remaining
# clients.  Returns the approximate amount of time (in seconds) that the
# election took.
# 
proc repmgr026_await_election_result { envlist limit } {
	set begin [clock seconds]
	set deadline [expr $begin + $limit]
	while { true } {
		set t [clock seconds]
		if { $t > $deadline } {
			error "FAIL: time limit exceeded"
		}

		if { [repmgr026_is_ready $envlist] } {
			return [expr $t - $begin]
		}

		tclsleep 1
	}
}

proc repmgr026_is_ready { envlist } {
	set winner [lindex $envlist 0]
	if {![is_elected $winner]} {
		return false
	}

	foreach client [lrange $envlist 1 end] {
		if {![is_event_present $client newmaster]} {
			return false
		}
	}
	return true
}
