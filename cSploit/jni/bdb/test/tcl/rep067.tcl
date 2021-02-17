# See the file LICENSE for redistribution information.
#
# Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	rep067
# TEST	Full election timeout test.
# TEST
# TEST	Verify that elections use a separate "full election timeout" (if such
# TEST	configuration is in use) instead of the normal timeout, when the
# TEST	replication group is "cold-booted" (all sites starting with recovery).
# TEST

proc rep067 { method args } {
	source ./include.tcl

	set tnum "067"

	# Run for btree only.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	puts "Rep$tnum: Full election timeout test."

	# This test consists of three cases, two of which can be handled by
	# script that is similar enough to be handled by a single proc
	# (rep067a_sub), with a parameter to determine whether a client is
	# down.  The other case is different enough to warrant its own proc
	# (rep067b_sub).
	# 
	rep067a_sub $tnum yes
	rep067a_sub $tnum no
	rep067b_sub $tnum
}

# Cold boot the group.  Sites A and B come up just fine, but site C might not
# come up (depending on the client_down flag).  Hold an election.  (The amount
# of time it takes depends on whether site C is running.)  Then, shut down site
# A, start site C if it isn't already running, and hold another election.
# 
proc rep067a_sub { tnum client_down } {
	source ./include.tcl
	global rand_init
	error_check_good set_random_seed [berkdb srand $rand_init] 0
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	# Set up some arbitrary timeout values for this test.  The only
	# constraint is that they should be large enough, and different enough,
	# so as to allow for some amount of measurement imprecision introduced
	# by the overhead of the test mechnism.  Timeout values themselves
	# expressed in microseconds, since they'll be passed to DB; leeway
	# values in seconds, so that we can measure the result here in Tcl.
	# 
	set elect_to 15000000
	set elect_secs_leeway 13
	set full_elect_to 30000000
	set full_secs_leeway 27

	puts -nonewline "Rep$tnum.a: Full election test, "
	if { $client_down } {
		puts "with one client missing"
		puts -nonewline "\tRep$tnum.b: First election" 
		puts " expected to take [expr $full_elect_to / 1000000] seconds"
	} else {
		puts "with all clients initially present"
		puts "\tRep$tnum.b: First election expected to complete quickly"
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

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	# Configure all three clients.  Use EID's starting at 2, because that's
	# what run_election expects.
	# 
	set nsites 3
	foreach i { 0 1 2 } eid { 2 3 4 } p { 20 50 100 } {
		set clientdir($i) $testdir/CLIENTDIR.$i
		file mkdir $clientdir($i)

		repladd $eid
		set env_cmd($i) "berkdb_env_noerr -create \
		    -event $repmemargs -home $clientdir($i) \
		    -txn -rep_client $verbargs \
		    -errpfx CLIENT.$i -rep_transport \[list $eid replsend\]"

		set errcmd($i) "none"
		set crash($i) 0
		set pri($i) $p
	}
	set elect_timeout [list $elect_to $full_elect_to]

	# Start the clients, but perhaps not all of them.
	# 
	set envlist {}
	if { $client_down } {
		set participants 2
	} else {
		set participants 3
	}
	for { set i 0 } { $i < $participants } { incr i } {
		set clientenv($i) [eval $env_cmd($i)]
		set eid [expr $i + 2]
		lappend envlist "$clientenv($i) $eid"
	}

	process_msgs $envlist

	# In this test, the expected winner is always the last one in the
	# array.  We made sure of that by arranging the priorities that way.
	# This is convenient so that we can remove the winner (master) in the
	# second phase, without leaving a hole in the arrays that the
	# run_election proc wouldn't cope with.
	# 
	set winner [expr $participants - 1]
	set initiator 0
	set nvotes 2
	set reopen_flag 0
	run_election envlist errcmd pri crash \
	    $qdir "Rep$tnum.c" $initiator $nsites $nvotes $participants \
	    $winner $reopen_flag NULL 0 0 $elect_timeout
	set duration [rep067_max_duration $envlist]
	puts "\tRep$tnum.d: the election took about $duration seconds"

	if { $client_down } {
		# Case #2.
		#
		# Without full participation on a cold boot, the election should
		# take the full long timeout.  In any case it should be way more
		# than the "normal" timeout.
		# 
		error_check_good duration1a \
		    [expr $duration > $full_secs_leeway] 1
	} else {
		# Case #1.
		#
		# With full participation, the election should complete "right
		# away".  At least it should be way less than the "normal"
		# election timeout.
		error_check_good duration1b \
		    [expr $duration < $elect_secs_leeway] 1
	}

	process_msgs $envlist

	if { !$client_down } {
		# Shut down the master and hold another election between the
		# remaining two sites.
		#
		puts "\tRep$tnum.e: Shut down elected master, and run another election"
		puts "\tRep$tnum.g: (expected to take [expr $elect_to / 1000000] seconds)"
		$clientenv($winner) close
		set envlist [lreplace $envlist $winner $winner]

		set winner 1
		set participants 2
		run_election envlist errcmd pri crash \
		    $qdir "Rep$tnum.b" $initiator $nsites $nvotes \
		    $participants $winner $reopen_flag NULL 0 0 $elect_timeout
		set duration [rep067_max_duration $envlist]

		# We don't have full participation, so the election can only be
		# won after a timeout.  But these clients have seen a master, so
		# we shouldn't have to wait for the full-election timeout.
		# 
		puts "\tRep$tnum.g: the election took about $duration seconds"
		error_check_good duration2 \
		    [expr $duration > $elect_secs_leeway && \
		    $duration < $full_secs_leeway] 1
	}
	$clientenv(0) close
	$clientenv(1) close

	replclose $testdir/MSGQUEUEDIR
}

# Run an election where one of the clients has seen a master, but the other has
# not.  Verify that the first client learns from the second that a master has
# been seen, and allows the election to complete after the normal timeout,
# rather than the full election timeout.
# 
proc rep067b_sub { tnum } {
	source ./include.tcl
	global rand_init
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	error_check_good set_random_seed [berkdb srand $rand_init] 0

	set elect_to 10000000
	set elect_secs_leeway 10
	set full_elect_to 180000000
	set full_secs_leeway 100

	puts "Rep$tnum.a: Mixed full election test"

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	# Start a master and one client.  This first step is just setup, for the
	# purpose of creating a client that has heard from a master.
	# 
	file mkdir $testdir/MASTERDIR
	set mcmd "berkdb_env_noerr -create \
		    -event $repmemargs -home $testdir/MASTERDIR \
		    -txn -rep_master $verbargs \
		    -errpfx MASTER -rep_transport \[list 1 replsend\]"
	file mkdir $testdir/CLIENTDIR
	set ccmd "berkdb_env_noerr -create \
		    -event $repmemargs -home $testdir/CLIENTDIR \
		    -txn -rep_client $verbargs \
		    -errpfx CLIENT.0 -rep_transport \[list 2 replsend\]"

	puts "\tRep$tnum.b: Start master and first client"
	repladd 1
	set menv [eval $mcmd]
	repladd 2
	set cenv [eval $ccmd]
	process_msgs [list [list $menv 1] [list $cenv 2]]

	puts "\tRep$tnum.c: Shut down master; start other client"
	$menv close

	# Now set up for the election test we're really interested in.  We'll
	# need $ccmd in array position 0 of env_cmd, for passing to
	# run_election.  Then, start the second client.  We now have a mixture
	# of clients: one who's seen a master, and the other who hasn't.
	#
	# The run_election proc assumes an offset of 2 between the array index
	# and the EID.  Thus EID 3 has to correspond to array index 1, etc.
	# 
	set env_cmd(0) $ccmd
	repladd 3
	file mkdir $testdir/CLIENTDIR2
	set env_cmd(1) "berkdb_env_noerr -create \
		    -event $repmemargs -home $testdir/CLIENTDIR2 \
		    -txn -rep_client $verbargs \
		    -errpfx CLIENT.1 -rep_transport \[list 3 replsend\]"
	set c2env [eval $env_cmd(1)]

	set envlist {}
	foreach i { 0 1 } eid { 2 3 } p { 100 50 } e [list $cenv $c2env] {
		set errcmd($i) "none"
		set crash($i) 0
		set pri($i) $p

		lappend envlist [list $e $eid]
	}
	set elect_timeout [list $elect_to $full_elect_to]

	set nsites 3
	set participants 2
	process_msgs $envlist

	puts "\tRep$tnum.d: Election expected to take [expr $elect_to / 1000000] seconds"
	set winner 0
	set initiator 0
	set nvotes 2
	set reopen_flag 0
	run_election envlist errcmd pri crash \
	    $qdir "Rep$tnum.e" $initiator $nsites $nvotes $participants \
	    $winner $reopen_flag NULL 0 0 $elect_timeout
	set duration [rep067_max_duration $envlist]
	puts "\tRep$tnum.f: the election took about $duration seconds"

	# We don't have full participation, so the election can only be won
	# after a timeout.  But even if only one client has seen a master, we
	# shouldn't have to wait for the full-election timeout.
	# 
	error_check_good duration3 \
	    [expr $duration > $elect_secs_leeway && \
	    $duration < $full_secs_leeway] 1

	$cenv close
	$c2env close

	replclose $testdir/MSGQUEUEDIR
}

proc rep067_max_duration { envlist } {
	set max 0.0
	foreach pair $envlist {
		set env [lindex $pair 0]
		set s [stat_field $env rep_stat "Election seconds"]
		set u [stat_field $env rep_stat "Election usecs"]
		set d [expr ( $u / 1000000.0 ) + $s ]
		if { $d > $max } {
			set max $d
		}
	}
	return $max
}
