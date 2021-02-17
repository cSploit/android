# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr034
# TEST	Repmgr site removal and restart.
# TEST	
# TEST	Start two repmgr sites, master and client1.  The client1 removes
# TEST	itself and restarts, sometimes more than once.  Start a third 
# TEST	repmgr site client2 and make sure it can remove client1 from
# TEST	the group.
# TEST
# TEST	This test must use heartbeats to ensure that a client that has
# TEST	been removed and restarted without recovery can sync up.
# TEST
proc repmgr034 { {niter 3} {tnum "034"} } {
	source ./include.tcl

        if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	# The iter is the number of iteration on site removal and restart.
	# When iter is 0, start master, client1, client2, and let client2
	# remove client1.  When iter is non-0, client1 removes itself and
	# restart iterately before being removed by client2.
	foreach iter "0 $niter" {
		# The nentries is the number of records being inserted
		# to master when client1 is removed from the group.
		# When nentries is 0, client1 does not need any sync up
		# with master when restart its repmgr.  When it is 100,
		# client1 needs to sync up a bit.  When it is 1000,
		# it is simulated that client1 has been removed for a
		# while in the group.
		foreach nentries {0 100 1000} {
			repmgr034_sub $method $tnum $iter $nentries
		}
	}
}

proc repmgr034_sub { method tnum niter nentries} {
	source ./include.tcl
	global databases_in_memory
	global env_private
	global testdir
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	if { $databases_in_memory } {
		set dbmemmsg "using in-memory databases "
		if { [is_queueext $method] } {
			puts -nonewline "Skipping repmgr$tnum for method "
			puts "$method with named in-memory databases."
			return
		}
		set dbname { "" "test.db" }
	} else {
		set dbmemmsg "using on-disk databases "
		set dbname "test.db"
	}

	if { $env_private == 1 } {
		set privargs " -private "
		set primsg "in private env "
	} else {
		set privargs ""
		set primsg ""
	}

	if { $niter > 0 } {
		set nitermsg "and restart for $niter time(s) "
	} else {
		set nitermsg ""
	}

	if { $repfiles_in_memory } {
		set repmemargs " -rep_inmem_files "
		set repmemmsg "and in-memory replication files "
	} else {
		set repmemargs ""
		set repmemmsg "and on-disk replication files "
	}

	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	} else {
		set verbargs ""
	}

	set omethod [convert_method $method]

	puts "Repmgr$tnum: Repmgr site removal, clean-up\
	    $nitermsg$dbmemmsg$repmemmsg$primsg\
	    with $nentries record(s)"

	foreach {port0 port1 port2} [available_ports 3] {}
	env_cleanup $testdir
	file mkdir [set masterdir $testdir/MASTERDIR]
	file mkdir [set clientdir1 $testdir/CLIENTDIR1]
	file mkdir [set clientdir2 $testdir/CLIENTDIR2]

	puts "\tRepmgr$tnum.a: Start master"
	set env0 [eval "berkdb_env -create -errpfx MASTER -home $masterdir \
	    -txn -rep -thread -recover $privargs $repmemargs $verbargs"]
	$env0 repmgr -timeout {heartbeat_send 500000}
	$env0 repmgr -local [list 127.0.0.1 $port0] -start master
	error_check_good nsites_A0 [$env0 rep_get_nsites] 1
	set db [berkdb_open_noerr -create -auto_commit -env $env0 \
	    $omethod $dbname]
	error_check_good env0_opendb [is_valid_db $db] TRUE

	puts "\tRepmgr$tnum.b: Start client1"
	set env1 [eval "berkdb_env_noerr -create -errpfx CLIENT1 \
	    -home $clientdir1 -txn -rep -thread -recover -event \
	    $privargs $repmemargs $verbargs"]
	$env1 repmgr -timeout {heartbeat_monitor 1100000}
	$env1 repmgr -local [list 127.0.0.1 $port1] \
	    -remote [list 127.0.0.1 $port0] -start client
	await_startup_done $env1
	error_check_good nsites_B0 [$env0 rep_get_nsites] 2
	error_check_good nsites_A1 [$env1 rep_get_nsites] 2
	set db1 [berkdb_open_noerr -create -auto_commit -env $env1 \
	    $omethod $dbname]
	error_check_good env1_opendb [is_valid_db $db1] TRUE


	if {$niter == 0} {
    		puts "\tRepmgr$tnum.c: Skip site self-removal and restart"
	} else {
		puts "\tRepmgr$tnum.c: Site removal and restart for\
		    $niter time(s)"
	}
	for { set count 1 } { $count <= $niter } {incr count } {
		$env1 event_info -clear
		puts "\t\tRepmgr$tnum.c.$count.(a): Client1 removes itself from\
		    the group: iter $count"
		$env1 repmgr -remove [list 127.0.0.1 $port1]
		await_event $env1 local_site_removed
		error_check_good nsites_C0 [$env0 rep_get_nsites] 1

		puts "\t\tRepmgr$tnum.c.$count.(b): The removed client1 is\
		    still read only"
		catch { [$db1 put "key" "data"] } ret
		error_check_good readonly_failure \
		    [is_substr $ret "permission denied"] 1
		error_check_good db1_notfound [llength [$db1 get "none"]] 0

		if { $nentries > 0 } {
			puts "\t\tRepmgr$tnum.c.$count.(b1): Write $nentries\
			    record(s) on master, might/might not be synced\
			    up to client1"
			for { set i 0 } { $i < $nentries } { incr i } {
				error_check_good db_put \
				    [$db put "key_${count}_${i}]" "data"] 0
			}
			error_check_good nkeys \
			    [stat_field $db stat "Number of keys"] \
			    [expr $nentries * $count ]
		}

		puts "\t\tRepmgr$tnum.c.$count.(c): Restart client1"
		$env1 repmgr -timeout {heartbeat_monitor 1100000}
		# Allow a retry in case client1 didn't have time to fully
		# shut down.
		if {[catch {$env1 repmgr -local [list 127.0.0.1 $port1] \
		    -remote [list 127.0.0.1 $port0] -start client} result] && \
		    [string match "*REP_UNAVAIL*" $result]} {
			tclsleep 10
			$env1 repmgr -local [list 127.0.0.1 $port1] \
			-remote [list 127.0.0.1 $port0] -start client
		}
		await_event $env1 connection_established		
		error_check_good nsites_D0 [$env0 rep_get_nsites] 2
		error_check_good nsites_B1 [$env1 rep_get_nsites] 2

		if { $nentries > 0 } {
			puts "\t\tRepmgr$tnum.c.$count.(c1): Check client1 sync\
			    up with master"
			set max_retries [expr $nentries / 10 ]
			for { set i 0 } { $i < $max_retries } { incr i } {
				if { [stat_field $db1 stat "Number of keys"] \
				    == [expr $nentries * $count ] } {
					break
				} else {
					tclsleep 2
				}
			}
			if { $i == $max_retries } {
				error "sync up duration is longer than expected"
			}
		}
	}

	puts "\tRepmgr$tnum.d: Start client2"
	set env2 [eval "berkdb_env -create -errpfx CLIENT2  -home $clientdir2 \
	    -txn -rep -thread -recover -event $privargs $repmemargs $verbargs"]
	# It is possible, especially when nentries=0, that we need a delay
	# before the recently restarted client1 can ack a new site addition.
	$env2 repmgr -timeout {heartbeat_monitor 1200000}
	if {[catch {$env2 repmgr -local [list 127.0.0.1 $port2] \
	    -remote [list 127.0.0.1 $port0] -start client} result] && \
	    [string match "*REP_UNAVAIL*" $result]} {
		tclsleep 3
		$env2 repmgr -local [list 127.0.0.1 $port2] \
		    -remote [list 127.0.0.1 $port0] -start client
	}
	await_startup_done $env2 100

	puts "\tRepmgr$tnum.e: Client2 removes client1 from the group"
	$env2 repmgr -remove [list 127.0.0.1 $port1]
	await_event $env1 local_site_removed
	error_check_good nsites_F0 [$env0 rep_get_nsites] 2
	catch { [$db1 put "key" "data"] } ret
	error_check_good readonly_failure \
	    [is_substr $ret "permission denied"] 1

	puts "\tRepmgr$tnum.f: Close all"
	error_check_good db1_close [$db1 close] 0
	error_check_good db_close [$db close] 0
	error_check_good s_2_close [$env2 close] 0
	error_check_good s_1_close [$env1 close] 0
	error_check_good s_0_close [$env0 close] 0
}
