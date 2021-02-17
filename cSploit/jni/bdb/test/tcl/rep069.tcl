# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	rep069
# TEST	Test of internal initialization and elections.
# TEST
# TEST	If a client is in a recovery mode of any kind, it
# TEST	participates in elections at priority 0 so it can
# TEST	never be elected master.
#
proc rep069 { method { niter 200 } { tnum "069" } args } {

	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory

	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep$tnum: Skipping for method $method."
		return
	}

	set args [convert_args $method $args]
	set saved_args $args

	set logsets [create_logsets 2]

	# Set up for on-disk or in-memory databases.
	set msg "using on-disk databases"
	if { $databases_in_memory } {
		set msg "using named in-memory databases"
		if { [is_queueext $method] } { 
			puts -nonewline "Skipping rep$tnum for method "
			puts "$method with named in-memory databases."
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	foreach l $logsets {
		set args $saved_args
		puts "Rep$tnum ($method $args): Test internal\
		    initialization and elections $msg $msg2."
		puts "Rep$tnum: Master logs are [lindex $l 0]"
		puts "Rep$tnum: Client logs are [lindex $l 1]"
		rep069_sub $method $niter $tnum $l $args
	}
}

proc rep069_sub { method niter tnum logset largs } {
	global testdir
	global util_path
	global databases_in_memory
	global repfiles_in_memory
	global rep_verbose
	global verbose_type

	env_cleanup $testdir

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	set masterdir $testdir/MASTERDIR
	file mkdir $masterdir

	set nclients 2
	for { set i 0 } { $i < $nclients } { incr i } {
		set clientdir($i) $testdir/CLIENTDIR.$i
		file mkdir $clientdir($i)
	}

	# Log size is small so we quickly create more than one, and
	# can easily force internal initialization.
	set pagesize 4096
	append largs " -pagesize $pagesize "
	set log_max [expr $pagesize * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]

	# In-memory logs cannot be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]

	# Open a master.
	set envlist {}
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create $m_txnargs \
	    $repmemargs \
	    $m_logargs -log_max $log_max -event $verbargs \
	    -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd -recover -rep_master]
	lappend envlist "$masterenv 1"

	# Open clients.
	for { set i 0 } { $i < $nclients } { incr i } {
		set envid [expr $i + 2]
		repladd $envid
		set envcmd($i) "berkdb_env_noerr -create \
		    $repmemargs \
		    $c_txnargs $c_logargs -log_max $log_max \
		    -home $clientdir($i) -event $verbargs \
		    -rep_transport \[list $envid replsend\]"
		set clientenv($i) [eval $envcmd($i) -recover -rep_client]
		lappend envlist "$clientenv($i) $envid"
	}

	# Bring the clients online by processing the startup messages.
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# db_archive in a moment.
	#
	$masterenv test force noarchive_timeout

	# Run rep_test in the master and update clients.
	puts "\tRep$tnum.a: Running rep_test in replicated env."
	set start 0
	eval rep_test \
	    $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist 0 NONE err
	error_check_good process_msgs $err 0

	# Create a gap requiring internal initialization.  The 
	# proc push_master_ahead will close & discard messages for
	# one of the clients.  Take care of the other one here.
	# 
	error_check_good client1_close [$clientenv(1) close] 0

	set cid 2
	set flags ""
	set dbhandle NULL
	set start [push_master_ahead $method $masterenv $masterdir $m_logtype \
	    $clientenv(0) $cid $dbhandle $start $niter $flags $largs]

	# Clear messages for second closed client.
	replclear 3

	# Adjust envlist for two closed clients.
	set envlist [lreplace $envlist 1 2]

	# Reopen clients.
	puts "\tRep$tnum.e: Reopen clients."
	for { set i 0 } { $i < $nclients } { incr i } {
		env_cleanup $clientdir($i)
		set clientenv($i) [eval $envcmd($i) -recover -rep_client]
		set envid [expr $i + 2]
		lappend envlist "$clientenv($i) $envid"
	}

	# Run proc_msgs_once until both clients are in internal
	# initialization.
	#
	# We figure out whether each client is in initialization
	# by searching for the state of SYNC_UPDATE.  As soon as
	# a client produces that state, it's marked as being
	# in initialization, and stays that way.  All clients
	# will get to that state on the same iteration.
	# 
	# We will always hit SYNC_UPDATE first, so we only need to
	# check for that one (i.e. not SYNC_PAGE or SYNC_LOG).
	#
	puts "\tRep$tnum.f:\
	    Run proc_msgs_once until all clients enter internal init."
	set in_init 0
	for { set i 0 } { $i < $nclients } { incr i } {
		set initializing($i) 0
	}

	while { $in_init != 1 } {
		set nproced [proc_msgs_once $envlist NONE err]
		for { set i 0 } { $i < $nclients } { incr i } {
			set stat($i) \
			    [exec $util_path/db_stat -N -r -R A -h $clientdir(1)]
			if {[is_substr $stat($i) "SYNC_UPDATE"] } {
				set initializing($i) 1
			}
		}
		set in_init 1
		for { set i 0 } { $i < $nclients } { incr i } {
			if { $initializing($i) == 0 } {
				set in_init 0
			}
		}
	}

	# Call an election.  It should fail, because both clients
	# are in internal initialization and therefore not electable.
	# Indicate failure with winner = -2.
	# First, close the master.
	error_check_good masterenv_close [$masterenv close] 0
	set envlist [lreplace $envlist 0 0]

        puts "\tRep$tnum.g: Run election; no one will get elected."
	set m "Rep$tnum.g"
        set nsites $nclients
        set nvotes $nclients
        set winner -2
        set elector 0
	for { set i 0 } { $i < $nclients } { incr i } {
		set err_cmd($i) "none"
		set crash($i) 0
		set pri($i) 10
	}

	# This election will time out instead of succeeding.
	set timeout_ok 1
	if { $databases_in_memory } {
		set dbname { "" "test.db" }
	} else { 
		set dbname "test.db"
	} 
        run_election envlist err_cmd pri crash \
            $qdir $m $elector $nsites $nvotes $nclients $winner \
	    0 $dbname 0 $timeout_ok

	# Verify that each client saw the message that no
	# electable site was found.
	puts "\tRep$tnum.h: Check for right error message."
	for { set i 0 } { $i < $nclients } { incr i } {
		set none_electable 0
		set id [expr $i + 1]
		set fd [open $testdir/ELECTION_RESULT.$id r]
		while { [gets $fd str] != -1 } {
			if { [is_substr $str "Too few remote sites"] == 1 } {
				set none_electable 1
				break
			}
		}
		close $fd
		error_check_good none_electable $none_electable 1
	}

	# Clean up for the next pass.
	for { set i 0 } { $i < $nclients } { incr i } {
		$clientenv($i) close
	}

	replclose $testdir/MSGQUEUEDIR
}

