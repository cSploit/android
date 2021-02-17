# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	rep094
# TEST	Full election with less than majority initially connected.
# TEST
# TEST	Cold-boot a 4-site group.  The first two sites start quickly and
# TEST	initiate an election.  The other two sites don't join the election until
# TEST	the middle of the long full election timeout period.  It's important that
# TEST	the number of sites that start immediately be a sub-majority, because
# TEST	that's the case that used to have a bug in it [#18456].
#
proc rep094 { method { tnum "094" } args } {
	source ./include.tcl

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	rep094_sub $method $tnum
}

proc rep094_sub { method tnum } {
	global rep_verbose
	global testdir
	global verbose_type
	global repfiles_in_memory
	global elect_serial

	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
		set msg ""
	} else {
		set repmemargs ""
		set msg ", with in-memory replication files"
	}
	puts "Rep$tnum: Full election starting with minority$msg."

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	replsetup [set qdir $testdir/MSGQUEUEDIR]

	set dira $testdir/SITEA
	set dirb $testdir/SITEB
	set dirc $testdir/SITEC
	set dird $testdir/SITED

	file mkdir $dira
	file mkdir $dirb
	file mkdir $dirc
	file mkdir $dird

	puts "\tRep$tnum.a: Boot first two sites."
	repladd 1
	set envcmda "berkdb_env_noerr -create -txn -errpfx SITEA \
	    $repmemargs -event \
	    $verbargs -home $dira -rep_transport \[list 1 replsend\]"
	set enva [eval $envcmda -rep_client]

	repladd 2
	set envcmdb "berkdb_env_noerr -create -txn -errpfx SITEB \
	    $repmemargs -event \
	    $verbargs -home $dirb -rep_transport \[list 2 replsend\]"
	set envb [eval $envcmdb -rep_client]

	set envlist "{$enva 1} {$envb 2}"
	process_msgs $envlist


	# Start an election at site A, with a generous full-election timeout (3
	# minutes).  (Note that specifying the timeout as a two-element list is
	# the (only) way to set a full election timeout via start_election.)
	# 
	repladd 3
	repladd 4
	puts "\tRep$tnum.b: Start an election at site A."
	set pri 100
	set nsites 4
	set nvotes 3
	set envid 1
	set timeout [list [expr 10 * 1000000] [expr 180 * 1000000]]
	set elect_serial 1
	set pfx "A.1"
	start_election $pfx $qdir $dira $envid $nsites $nvotes $pri $timeout
	set elect_pipe($envid) $elect_serial

	# The standard run_election proc is not flexible enough for our needs
	# here, so we conduct our own customized version of the loop here.
	# What's different about our case here is that we want to start sites C
	# and D during the midst of the phase 1 timeout period.
	# 
	set wait_limit 20
	set done false
	for { set count 0 } { $count < $wait_limit && !$done} { incr count } {
		foreach pair $envlist {
			set env [lindex $pair 0]
			set envid [lindex $pair 1]
			if { [info exists elect_pipe($envid)] } {
				check_election $elect_pipe($envid) \
				    unavail child_elected
			} else {
				set child_elected false
			}
			set parent_elected [is_elected $env]
			if { $child_elected || $parent_elected } {
				error "FAIL: election succeeded unexpectedly"
			}
			replprocessqueue $env $envid 0 he
			if { $he } {
				incr elect_serial
				set envpfx [$env get_errpfx]
				set pfx "$envpfx.$elect_serial"
				puts "\tRep$tnum.c: Starting another\
				    election $pfx at $envpfx."
				set dir [$env get_home]
				start_election $pfx $qdir $dir $envid \
				    $nsites $nvotes $pri $timeout
				set elect_pipe($envid) $elect_serial
			}
		}
		if { $count >= 10 && $he } {
			set done true
		} else {
			tclsleep 1
		}
	}

	# Now that we've waited 10 seconds, start the "slow-booting" 
	# sites C and D.
	# 
	puts "\tRep$tnum.d: Boot third site, have it join election."
	set envcmdc "berkdb_env_noerr -create -txn -errpfx SITEC \
	    $repmemargs -event \
	    $verbargs -home $dirc -rep_transport \[list 3 replsend\]"
	set envc [eval $envcmdc -rep_client]
	set envlist "{$enva 1} {$envb 2} {$envc 3}"
	process_msgs $envlist

	incr elect_serial
	set env $envc
	set envid 3
	set envpfx [$env get_errpfx]
	set pfx "$envpfx.$elect_serial"
	puts "\tRep$tnum.e: Starting straggler election $pfx at $envpfx."
	set dir [$env get_home]
	start_election $pfx $qdir $dir $envid $nsites $nvotes $pri $timeout
	set elect_pipe($envid) $elect_serial

	for { set count 0 } { $count < 5 } { incr count } {
		process_msgs $envlist
		tclsleep 1
	}

	puts "\tRep$tnum.f: Boot fourth site, join election."
	set envcmdd "berkdb_env_noerr -create -txn -errpfx SITED \
	    $repmemargs -event \
	    $verbargs -home $dird -rep_transport \[list 4 replsend\]"
	set envd [eval $envcmdd -rep_client]
	set envlist "{$enva 1} {$envb 2} {$envc 3} {$envd 4}"
	process_msgs $envlist

	incr elect_serial
	set env $envd
	set envid 4
	set envpfx [$env get_errpfx]
	set pfx "$envpfx.$elect_serial"
	puts "\tRep$tnum.g: Starting straggler election $pfx at $envpfx."
	set dir [$env get_home]
	start_election $pfx $qdir $dir $envid $nsites $nvotes $pri $timeout
	set elect_pipe($envid) $elect_serial
	process_msgs $envlist

	# The election should now complete promptly.  We'll give it 60 seconds,
	# just in case this test is running on a slow overloaded system.  That's
	# still way less than the 3 minutes total full election timeout, so if
	# we finish before 60 seconds, it means we succeeded.
	#
	set wait_limit 60
	set master -1
	for { set count 0 } { $count < $wait_limit } { incr count } {
		set finishers 0
		set synced_clients 0
		foreach pair $envlist {
			set env [lindex $pair 0]
			set envid [lindex $pair 1]
			if { [info exists elect_pipe($envid)] } {
				if {[check_election $elect_pipe($envid) \
				    unavail child_elected]} {
					incr finishers
				}
			} else {
				set child_elected false
			}
			set parent_elected [is_elected $env]
			if { ( $child_elected || $parent_elected ) &&
			     $master == -1 } {
				set master $envid
				$env rep_start -master
			}
			replprocessqueue $env $envid 0 he
			if { $he } {
				error "FAIL: got HOLDELECTION unexpectedly"
			}
			
			# Once we've elected a master, start checking for
			# startupdone at the clients, just so's we can
			# gracefully shut everything down.
			# 
			if { $master > 0 && $envid != $master && 
			    [stat_field $env rep_stat "Startup complete"] } {
				incr synced_clients
			}
		}

		if { $finishers == $nsites && $synced_clients == $nsites - 1 } {
			break;
		}
		tclsleep 1
	}
	error_check_good got_newmaster [expr $master > 0] 1

	cleanup_elections

	$enva close
	$envb close
	$envc close
	$envd close
	replclose $qdir
}
