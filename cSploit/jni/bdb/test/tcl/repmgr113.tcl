# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr113
# TEST	Multi-process repmgr automatic listener takeover.
# TEST
# TEST	One of the subordinate processes automatically becomes listener if the
# TEST	original listener leaves.  An election is delayed long enough for a
# TEST	takeover to occur if the takeover happens on the master.

proc repmgr113 { {tnum "113"} } {
	source ./include.tcl
	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	puts "Repmgr$tnum:\
	    Test automatic listener takeover among multiple processes."

	# Test running multiple listener takeovers on master and client.
	repmgr113_loop $tnum

	# Test listener takeovers in different scenarios.
	repmgr113_test $tnum

	# Test zero nthreads in taking over subordinate process.
	repmgr113_zero_nthreads $tnum
}

proc repmgr113_loop { {tnum "113"} } {
	global testdir

	puts "\tRepmgr$tnum.loop: Run short-lived processes to\
	    perform multiple takeovers."
	env_cleanup $testdir

	foreach {mport c1port c2port} [available_ports 3] {}
	file mkdir [set mdir $testdir/MASTER]
	file mkdir [set c1dir $testdir/CLIENT1]
	file mkdir [set c2dir $testdir/CLIENT2]
	make_dbconfig $mdir \
	    [list [list repmgr_site 127.0.0.1 $mport db_local_site on]]
	make_dbconfig $c1dir \
	    [list [list repmgr_site 127.0.0.1 $c1port db_local_site on] \
	    [list repmgr_site 127.0.0.1 $mport db_bootstrap_helper on]]
	make_dbconfig $c2dir \
	    [list [list repmgr_site 127.0.0.1 $c2port db_local_site on] \
	    [list repmgr_site 127.0.0.1 $mport db_bootstrap_helper on]]

	puts "\t\tRepmgr$tnum.loop.a: Start master and client1."
	set cmds {
		"home $mdir"
		"output $testdir/m_0_output"
		"open_env"
		"start master"
	}
	set m_1 [open_site_prog [subst $cmds]]
	set m_env [berkdb_env -home $mdir]
	set cmds {
		"home $c1dir"
		"output $testdir/c1_1_output"
		"open_env"
		"start client"
	}
	set c1_1 [open_site_prog [subst $cmds]]
	set c1_env [berkdb_env -home $c1dir]
	await_startup_done $c1_env

	# Test case 1: Test listener takeover on master.
	# 2 sites, master and client1
	# 2 master processes, m_1 (listener) and m_2
	# 1 client1 process, c1_1 (listener)
	#
	# Start all processes.  Stop master listener m_1.  Verify m_2 takes
	# over listener role and no election on client1.  Set m_2 to m_1 and
	# start another master process m_2, stop m_1 again and redo takeover
	# for multiple times.
	puts -nonewline "\t\tRepmgr$tnum.loop.b: Run short-lived processes\
	    to perform multiple takeovers on master"
	flush stdout
	for { set i 1 } { $i < 11 } { incr i} {
		# Close listener process and verify takeover happens.
		puts -nonewline "."
		flush stdout

		set cmds {
			"home $mdir"
			"output $testdir/m_$i\_output"
			"open_env"
			"start master"
		}
		set m_2 [open_site_prog [subst $cmds]]
		set count 0
		puts $m_2 "is_connected $c1port"
		while {! [gets $m_2]} {
			if {[incr count] > 30} {
				error "FAIL: couldn't connect to client1\
				    within 30 seconds"
			}
			tclsleep 1
			puts $m_2 "is_connected $c1port"
		}
		close $m_1
		set count 0
		set m_takeover_count [stat_field $m_env repmgr_stat \
		    "Automatic replication process takeovers"]
		while { $m_takeover_count != $i } {
			if {[incr count] > 30} {
				error "FAIL: couldn't takeover on master\
				    in 30 seconds"
			}
			tclsleep 1
			set m_takeover_count [stat_field $m_env repmgr_stat \
			    "Automatic replication process takeovers"]
		}
		set election_count [stat_field $c1_env rep_stat \
		    "Elections held"]
		error_check_good c1_no_elections_1 $election_count 0
		tclsleep 3
		puts $m_2 "is_connected $c1port"
		while {! [gets $m_2]} {
			if {[incr count] > 30} {
				error "FAIL: couldn't connect to client1
				    within 30 seconds"
			}
			tclsleep 1
			puts $m_2 "is_connected $c1port"
		}
		set m_1 $m_2
	}
	puts ""

	# Test case 2: Test listener takeover on master and client successively.
	# 3 sites, master, client1, client2
	# 2 master processes, m_1 (listener) and m_2
	# 1 client1 process, c1_1 (listener)
	# 2 client2 processes,  c2_1 (listener) and c2_2
	#
	# Start client2 process c2_1, c2_2 and master process m_2.  Stop
	# client2 listener c2_1.  Verify takeover happens on client2.  Stop
	# master listener m_1.  Verify m_2 takes over listener role and no
	# election on client1.  Set c2_2 to c2_1, m_2 to m_1.  Start another
	# client2 process c2_2 and master process m_2.  Stop c2_1 and m_2
	# again and redo takeovers for multiple times.
	puts "\t\tRepmgr$tnum.loop.c: Start client2."
	set cmds {
		"home $c2dir"
		"output $testdir/c2_1_output"
		"open_env"
		"start client"
	}
	set c2_1 [open_site_prog [subst $cmds]]
	set c2_env [berkdb_env -home $c2dir]
	await_startup_done $c2_env

	puts -nonewline "\t\tRepmgr$tnum.loop.d: Run short-lived processes to\
	    perform multiple takeovers on master and client2 successively"
	flush stdout
	for { set i 11 } { $i < 21 } { incr i} {
		puts -nonewline "."
		flush stdout
		set cmds {
			"home $mdir"
			"output $testdir/m_$i\_output"
			"open_env"
			"start master"
		}
		set m_2 [open_site_prog [subst $cmds]]
		set cmds {
			"home $c2dir"
			"output $testdir/c2_$i\_output"
			"open_env"
			"start client"
		}
		set c2_2 [open_site_prog [subst $cmds]]
		set count 0
		puts $m_2 "is_connected $c2port"
		while {! [gets $m_2]} {
			if {[incr count] > 30} {
				error "FAIL: couldn't connect to client2\
				    within 30 seconds"
			}
			tclsleep 1
			puts $m_2 "is_connected $c2port"
		}
		set count 0
		puts $c2_2 "is_connected $mport"
		while {! [gets $c2_2]} {
			if {[incr count] > 30} {
				error "FAIL: couldn't connect to master\
				    within 30 seconds"
			}
			tclsleep 1
			puts $c2_2 "is_connected $mport"
		}

		close $c2_1
		set count 0
		set c_takeover_count [stat_field $c2_env repmgr_stat \
		    "Automatic replication process takeovers"]
		while { $c_takeover_count != [expr $i - 10] } {
			if {[incr count] > 30} {
				error "FAIL: couldn't takeover on client2\
				    in 30 seconds"
			}
			tclsleep 1
			set c_takeover_count [stat_field $c2_env repmgr_stat \
			    "Automatic replication process takeovers"]
		}
		# Pause to let c2_2 connect to m_2.
		tclsleep 3

		close $m_1
		set count 0
		set m_takeover_count [stat_field $m_env repmgr_stat \
		    "Automatic replication process takeovers"]
		while { $m_takeover_count != $i } {
			if {[incr count] > 30} {
				error "FAIL: couldn't takeover on master\
				    in 30 seconds"
			}
			tclsleep 1
			set m_takeover_count [stat_field $m_env repmgr_stat \
			    "Automatic replication process takeovers"]
		}
		set election_count [stat_field $c1_env rep_stat \
		    "Elections held"]
		error_check_good c1_no_elections_2 $election_count 0

		set m_1 $m_2
		set c2_1 $c2_2
	}
	$m_env close
	$c1_env close
	$c2_env close
	close $c1_1
	close $c2_1
	close $m_1
	puts " "
}

proc repmgr113_test { {tnum "113"} } {
	global testdir

	puts "\tRepmgr$tnum.test: Takeover in any subordinate process and\
	    election delay due to the takeover on master"
	env_cleanup $testdir

	foreach {mport c1port c2port c3port} [available_ports 4] {}
	file mkdir [set mdir $testdir/MASTER]
	file mkdir [set c1dir $testdir/CLIENT1]
	file mkdir [set c2dir $testdir/CLIENT2]
	file mkdir [set c3dir $testdir/CLIENT3]
	make_dbconfig $mdir \
	    [list [list repmgr_site 127.0.0.1 $mport db_local_site on]]
	make_dbconfig $c1dir \
	    [list [list repmgr_site 127.0.0.1 $c1port db_local_site on] \
	    [list repmgr_site 127.0.0.1 $mport db_bootstrap_helper on]]
	make_dbconfig $c2dir \
	    [list [list repmgr_site 127.0.0.1 $c2port db_local_site on] \
	    [list repmgr_site 127.0.0.1 $mport db_bootstrap_helper on]]
	make_dbconfig $c3dir \
	    [list [list repmgr_site 127.0.0.1 $c3port db_local_site on] \
	    [list repmgr_site 127.0.0.1 $mport db_bootstrap_helper on]]

	# Test case 1: Test listener takeover on master.
	# 2 sites, master and client1
	# 2 master processes, m_1 (listener) and m_2
	# 1 client1 process, c1_1 (listener)
	#
	# Start all processes.  Stop master listener m_1.  Verify m_2 takes
	# over listener role and no election on client1.
	puts "\t\tRepmgr$tnum.test.a: Start two processes on master and one\
	    process on client1."
	set cmds {
		"home $mdir"
		"output $testdir/m_1_output"
		"open_env"
		"start master"
	}
	set m_1 [open_site_prog [subst $cmds]]
	set cmds {
		"home $mdir"
		"output $testdir/m_2_output"
		"open_env"
		"start master"
	}
	set m_2 [open_site_prog [subst $cmds]]
	set m_env [berkdb_env -home $mdir]
	set cmds {
		"home $c1dir"
		"output $testdir/c1_1_output"
		"open_env"
		"start client"
	}
	set c1_1 [open_site_prog [subst $cmds]]
	set c1_env [berkdb_env -home $c1dir]
	await_startup_done $c1_env
	await_condition {[expr [$m_env rep_get_nsites] == 2]}
	# Wait for some time so that m2 connects to c1
	tclsleep 3

	puts "\t\tRepmgr$tnum.test.b: Close master listener, verify takeover\
	    on master and no election on client1."
	close $m_1
	tclsleep 3
	set takeover_count [stat_field $m_env repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good m_takeover_count_1 $takeover_count 1
	set election_count [stat_field $c1_env rep_stat "Elections held"]
	error_check_good c1_no_elections_1 $election_count 0

	# Test case 2: Test listener takeover on client.
	# 2 sites, master and client1
	# 2 master processes, m_2 (listener) and m_3
	# 2 client1 processes, c1_1 (listener) and c1_2
	#
	# Start subordinate processes on master and client1, m_3 and c1_2.
	# Stop client1 listener c1_1.  Verify c1_2 takes over listener role.
	puts "\t\tRepmgr$tnum.test.c: Start a master subordinate process."
	set cmds {
		"home $mdir"
		"output $testdir/m_3_output"
		"open_env"
	}
	set m_3 [open_site_prog [subst $cmds]]
	puts $m_3 "start master"
	error_check_match m_sub_ret_1 [gets $m_3] "*DB_REP_IGNORE*"

	puts "\t\tRepmgr$tnum.test.d: Start a client1 subordinate process."
	set cmds {
		"home $c1dir"
		"output $testdir/c1_2_output"
		"open_env"
		"start client"
	}
	set c1_2 [open_site_prog [subst $cmds]]
	# Pause to let c1_2 connect to m_2 and m_3.
	tclsleep 2

	puts "\t\tRepmgr$tnum.test.e: Close client1 listener, verify\
	    takeover on client1."
	close $c1_1
	tclsleep 3
	set takeover_count [stat_field $c1_env repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good c1_takeover_count_1 $takeover_count 1

	# Test case 3: Test master takeover soon after client takeover in test
	# case 2.
	# 2 sites, master and client1
	# 2 master processes, m_2 (listener) and m_3
	# 1 client1 process, c1_2 (listener)
	#
	# Close master listener m_2.  Takeover happens on master.  Verify no
	# election on client1, which means the connections between subordinate
	# process m_3 and new listener c1_2 are established in time.
	puts "\t\tRepmgr$tnum.test.f: Close master listener, verify takeover\
	    on master and no election on client1."
	close $m_2
	tclsleep 3
	set takeover_count [stat_field $m_env repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good m_takeover_count_2 $takeover_count 2
	set election_count [stat_field $c1_env rep_stat "Elections held"]
	error_check_good c1_no_elections_2 $election_count 0

	# Test case 4: Test no takeover in subordinate rep-unaware process.
	# 2 sites, master and client1
	# 3 master processes, m_3 (listener), m_4 (rep-unaware) and
	# m_5 (rep-unaware)
	# 1 client1 process, c1_2 (listener)
	#
	# Start two master subordinate rep-unaware processes m_4 and m_5.
	# Close master listener m_3.  Verify m_4 and m_5 don't take over
	# listener role, client1 raises election.
	puts "\t\tRepmgr$tnum.test.g: Start two master rep-unaware processes."
	set cmds {
		"home $mdir"
		"output $testdir/m_4_output"
		"open_env"
	}
	set m_4 [open_site_prog [subst $cmds]]
	puts $m_4 "open_db test.db"
	set count 0
	puts $m_4 "is_connected $c1port"
	while {! [gets $m_4]} {
		if {[incr count] > 30} {
			error "FAIL:\
			    couldn't connect client1 within 30 seconds"
		}
		tclsleep 1
		puts $m_4 "is_connected $c1port"
	}

	set cmds {
		"home $mdir"
		"output $testdir/m_5_output"
		"open_env"
	}
	set m_5 [open_site_prog [subst $cmds]]
	puts $m_5 "open_db test.db"
	puts $m_5 "put k1 k1"
	puts $m_5 "echo done"
	error_check_good m_5_put_done_k1 [gets $m_5] "done"
	set count 0
	puts $m_5 "is_connected $c1port"
	while {! [gets $m_5]} {
		if {[incr count] > 30} {
			error "FAIL:\
			    couldn't connect client1 within 30 seconds"
		}
		tclsleep 1
		puts $m_5 "is_connected $c1port"
	}

	puts "\t\tRepmgr$tnum.test.h: Close master listener, verify no\
	    takeover on master, election happens on client1."
	close $m_3
	# Election should be held before election delay.
	tclsleep 2
	set election_count [stat_field $c1_env rep_stat "Elections held"]
	error_check_good c1_one_election_1 $election_count 1
	tclsleep 2
	set takeover_count [stat_field $m_env repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good m_takeover_count_3 $takeover_count 2
	close $m_4
	close $m_5

	# Test case 5: Test failed takeover.
	# 2 sites, master and client1
	# 2 master processes, m_6 (listener), m_7
	# 1 client1 process, c1_2 (listener)
	#
	# Start two master processes m_6 and m_7.  Close m_6, verify client1
	# delays the election.  Close m_7 before takeover succeeds, verify
	# takeover fails and election finally happens on client1.
	puts "\t\tRepmgr$tnum.test.i: A master process rejoins, should be\
	    the listener."
	set cmds {
		"home $mdir"
		"output $testdir/m_6_output"
		"open_env"
	}
	set m_6 [open_site_prog [subst $cmds]]
	puts $m_6 "start master"
	error_check_match m_sub_ret_2 [gets $m_6] "*Successful*"
	puts $m_6 "open_db test.db"
	puts $m_6 "put k2 k2"
	puts $m_6 "echo done"
	gets $m_6

	puts "\t\tRepmgr$tnum.test.j: Start a master subordinate process"
	set cmds {
		"home $mdir"
		"output $testdir/m_7_output"
		"open_env"
	}
	set m_7 [open_site_prog [subst $cmds]]
	puts $m_7 "start master"
	error_check_match m_sub_ret_1 [gets $m_7] "*DB_REP_IGNORE*"
	# Pause to let m_7 connect to c1_2
	tclsleep 3

	puts "\t\tRepmgr$tnum.test.k: Close master processes to prevent\
	    takeover, verify that election is delayed but finally happens"
	close $m_6
	set takeover_count [stat_field $m_env repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good m_takeover_count_4 $takeover_count 2
	set election_count [stat_field $c1_env rep_stat "Elections held"]
	error_check_good c1_no_elections_3 $election_count 1
	close $m_7
	tclsleep 3
	set election_count [stat_field $c1_env rep_stat "Elections held"]
	error_check_good c1_one_election_2 $election_count 2

	# Test case 6: Test one of subordinate processes succeeds in takeover.
	# 2 sites, master and client1
	# 1 master process, m_8 (listener)
	# 3 client1 processes, c1_2 (listener), c1_3 and c1_4.
	#
	# Start master listener m_8 and two client1 processes c1_3 and c1_4.
	# Close c1_2.  Verify takeover happens once.
	puts "\t\tRepmgr$tnum.test.l: A master process rejoins, should be\
	    master listener."
	set cmds {
		"home $mdir"
		"output $testdir/m_8_output"
		"open_env"
	}
	set m_8 [open_site_prog [subst $cmds]]
	puts $m_8 "start master"
	error_check_match m_sub_ret_4 [gets $m_8] "*Successful*"
	puts $m_8 "open_db test.db"
	puts $m_8 "put k3 k3"
	puts $m_8 "echo done"
	gets $m_8

	puts "\t\tRepmgr$tnum.test.m: Start two processes on client1, close\
	    client1 listener, verify takeover on client1."
	set cmds {
		"home $c1dir"
		"output $testdir/c1_3_output"
		"open_env"
		"start client"
	}
	set c1_3 [open_site_prog [subst $cmds]]
	set cmds {
		"home $c1dir"
		"output $testdir/c1_4_output"
		"open_env"
		"start client"
	}
	set c1_4 [open_site_prog [subst $cmds]]
	close $c1_2
	tclsleep 3
	set takeover_count [stat_field $c1_env repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good c1_takeover_count_2 $takeover_count 2

	# Test case 7: Test no takeover on removed site.
	# 2 sites, master and client1
	# 1 master process, m_8 (listener)
	# 2 client1 processes, c1_3 (listener), c1_4
	#
	# Remove client1.  Verify c1_4 doesn't take over listener role.
	puts "\t\tRepmgr$tnum.test.n: Remove client1 and verify no takeover on\
	    client1."
	puts $m_8 "remove $c1port"
	await_condition {[expr [$m_env rep_get_nsites] == 1]}
	tclsleep 3
	set takeover_count [stat_field $c1_env repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good c1_takeover_count_3 $takeover_count 2

	$c1_env close
	close $c1_3
	close $c1_4

	# Test case 8: Test takeover happens on a site with both subordinate
	# rep-aware process and rep-unaware process.
	# 3 sites, master, client2 and client3
	# 3 master processes, m_8 (listener), m_9 (rep-aware) and 
	# m_10 (rep-unaware)
	# 1 client2 process, c2_1 (listener)
	# 1 client3 process, c3_1 (listener)
	#
	# Start listener process on client2 and client3, one rep-aware master
	# process m_9 and another rep-unaware master process m_10.  Close
	# master listener m_8.  Verify takeover happens on master and no
	# election on client2 and client3.
	puts "\t\tRepmgr$tnum.test.o: Add client2 and client3."
	set cmds {
		"home $c2dir"
		"output $testdir/c2_1_output"
		"open_env"
		"start client"
	}
	set c2_1 [open_site_prog [subst $cmds]]
	set cmds {
		"home $c3dir"
		"output $testdir/c3_1_output"
		"open_env"
		"start client"
	}
	set c3_1 [open_site_prog [subst $cmds]]
	set c2_env [berkdb_env -home $c2dir]
	await_startup_done $c2_env
	set c3_env [berkdb_env -home $c3dir]
	await_startup_done $c3_env

	puts "\t\tRepmgr$tnum.test.p: Start a rep-aware and a rep-unaware\
	    processes on master, close master listener, verify no election."
	set cmds {
		"home $mdir"
		"output $testdir/m_9_output"
		"open_env"
		"start master"
	}
	set m_9 [open_site_prog [subst $cmds]]
	tclsleep 3
	puts $m_9 "is_connected $c2port"
	error_check_good m_10_connected_c2_1 [gets $m_9] 1
	puts $m_9 "is_connected $c3port"
	error_check_good m_10_connected_c3_1 [gets $m_9] 1

	set cmds {
		"home $mdir"
		"output $testdir/m_10_output"
		"open_env"
	}
	set m_10 [open_site_prog [subst $cmds]]
	puts $m_10 "open_db test.db"
	puts $m_10 "put k4 k4"
	puts $m_10 "echo done"
	error_check_good m_10_put_done_k1 [gets $m_10] "done"

	set count 0
	puts $m_10 "is_connected $c2port"
	while {! [gets $m_10]} {
		if {[incr count] > 30} {
			error "FAIL: couldn't connect c2_1 within 30 seconds"
		}
		tclsleep 1
		puts $m_10 "is_connected $c2port"
	}
	set count 0
	puts $m_10 "is_connected $c3port"
	while {! [gets $m_10]} {
		if {[incr count] > 30} {
			error "FAIL: couldn't connect c3_1 within 30 seconds"
		}
		tclsleep 1
		puts $m_10 "is_connected $c3port"
	}

	close $m_8
	tclsleep 3
	set election_count [stat_field $c2_env rep_stat "Elections held"]
	error_check_good c2_no_elections_1 $election_count 0
	set election_count [stat_field $c3_env rep_stat "Elections held"]
	error_check_good c3_no_elections_1 $election_count 0
	set takeover_count [stat_field $m_env repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good m_takeover_count_5 $takeover_count 3

	# Test case 9: Test election happens without listener candidate.
	# 3 sites, master, client2 and client3
	# 2 master processes, m_9 (listener), m_10 (rep-unaware)
	# 1 client2 process, c2_1 (listener)
	# 1 client3 process, c3_1 (listener)
	#
	# Close master listener m_9.  Verify no takeover on the master,
	# election happens and end with new master.
	puts "\t\tRepmgr$tnum.test.q: Close new master listener, verify that\
	    election happens."
	set old_master_id [stat_field $c2_env rep_stat "Master environment ID"]
	close $m_9
	tclsleep 2
	set election_count [stat_field $c2_env rep_stat "Elections held"]
	error_check_good c2_no_elections_2 $election_count 1
	set election_count [stat_field $c3_env rep_stat "Elections held"]
	error_check_good c3_no_elections_2 $election_count 1
	tclsleep 2
	set new_master_id [stat_field $c2_env rep_stat "Master environment ID"]
	error_check_bad new_master $new_master_id $old_master_id
	set takeover_count [stat_field $m_env repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good m_takeover_count_6 $takeover_count 3

	close $c2_1
	close $c3_1
	$m_env close
	$c2_env close
	$c3_env close
	close $m_10
}

proc repmgr113_zero_nthreads { {tnum "113"} } {
	global testdir

	puts "\tRepmgr$tnum.zero.nthreads: Test automatic takeover by a\
	    subordinate process configured with zero nthreads."
	env_cleanup $testdir

	foreach {mport} [available_ports 1] {}
	file mkdir [set mdir $testdir/MASTER]
	make_dbconfig $mdir \
	    [list [list repmgr_site 127.0.0.1 $mport db_local_site on]]

	puts "\t\tRepmgr$tnum.zero.nthreads.a: Start master listener."
	set cmds {
		"home $mdir"
		"output $testdir/m_1_output"
		"open_env"
		"start master"
	}
	set m_1 [open_site_prog [subst $cmds]]

	puts "\t\tRepmgr$tnum.zero.nthreads.b: Start master subordinate process\
	    configured with 0 message threads."
	set m_2 [berkdb_env -home $mdir -txn -rep -thread -event -errpfx \
	    "MASTER" -errfile $testdir/m_2_output]
	$m_2 repmgr -local [list 127.0.0.1 $mport] -start master -msgth 0

	puts "\t\tRepmgr$tnum.zero.nthreads.c: Close listener, verify takeover\
	    happens in the subordinate process."
	close $m_1
	tclsleep 3
	# Verify that the takeovers stat should show a takeover and there is
	# no autotakeover_failed event.
	set takeover_count [stat_field $m_2 repmgr_stat \
	    "Automatic replication process takeovers"]
	error_check_good m_takeover $takeover_count 1
	set ev [find_event [$m_2 event_info] autotakeover_failed]
	error_check_good m_no_autotakeover_failed [string length $ev] 0
	$m_2 close
}

