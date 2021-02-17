# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST repmgr112
# TEST Multi-process repmgr ack policies.
# TEST
# TEST Subordinate processes sending live log records must observe the
# TEST ack policy set by the main process.  Also, a policy change made by a
# TEST subordinate process should be observed by all processes.

proc repmgr112 { } {
	source ./include.tcl

	set tnum "112"
	puts "Repmgr$tnum: consistent ack policy among processes."
	set site_prog [setup_site_prog]

	env_cleanup $testdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	set ports [available_ports 3]
	set master_port [lindex $ports 0]
	set client_port [lindex $ports 1]
	set client2_port [lindex $ports 2]

	puts "\tRepmgr$tnum.b: Set up the master (port $master_port)."
	set master [open "| $site_prog" "r+"]
	fconfigure $master -buffering line
	puts $master "home $masterdir"
	make_dbconfig $masterdir {}
	puts $master "output $testdir/m1output"
	puts $master "open_env"
	puts $master "local $master_port"
	puts $master "start master"
	set ignored [gets $master]

	# The client will have the default ack policy (QUORUM), so that it will
	# always send acks.  This isn't truly kosher in a real HA deployment,
	# because generally the ack policy should be the same at all sites,
	# but is useful for testing purposes here.
	# 
	puts "\tRepmgr$tnum.c: Set up the client (on TCP port $client_port)."
	set client [open "| $site_prog" "r+"]
	fconfigure $client -buffering line
	puts $client "home $clientdir"
	puts $client "local $client_port"
	make_dbconfig $clientdir {}
	puts $client "output $testdir/coutput"
	puts $client "open_env"
	puts $client "remote 127.0.0.1 $master_port"
	puts $client "start client"
	error_check_match start_client [gets $client] "*Successful*"

	puts "\tRepmgr$tnum.d: Wait for STARTUPDONE."
	set clientenv [berkdb_env -home $clientdir]
	await_startup_done $clientenv

	# Create a third site by starting another client, but then
	# shut it down, so that the test runs with an out-of-service
	# client.
	puts "\tRepmgr$tnum.c: Set up another client (on TCP port $client2_port)."
	set client2 [open "| $site_prog" "r+"]
	fconfigure $client2 -buffering line
	puts $client2 "home $clientdir2"
	puts $client2 "local $client2_port"
	make_dbconfig $clientdir2 {}
	puts $client2 "output $testdir/c2output"
	puts $client2 "open_env"
	puts $client2 "remote 127.0.0.1 $master_port"
	puts $client2 "start client"
	error_check_match start_client2 [gets $client2] "*Successful*"

	set clientenv2 [berkdb_env -home $clientdir2]
	await_startup_done $clientenv2
	$clientenv2 close
	close $client2

	# Here the Tcl script itself acts as the subordinate process, sharing
	# the environment with the db_repsite main process.  Change the ack
	# policy from here, and then check that the main process has observed
	# it.  With the ALL ack policy, since we don't have 3 sites running we
	# should get a perm failure.  (If the policy were the default QUORUM,
	# one client would be enough.)
	# 
	puts "\tRepmgr$tnum.e: Change ack policy from the Tcl process."
	set masterenv [berkdb_env -home $masterdir -txn -rep -thread]
	$masterenv repmgr -ack all

	puts $master "open_db test.db"
	puts $master "echo opendone"
	error_check_good opendone [gets $master] "opendone"
	
	set perm_failures "Acknowledgement failures"
	set pfs0 [stat_field $masterenv repmgr_stat $perm_failures]

	puts $master "put mykey mydata"
	puts $master "echo putdone"
	error_check_good putdone [gets $master] "putdone"

	set pfs1 [stat_field $masterenv repmgr_stat $perm_failures]
	error_check_good fail_count $pfs1 [expr $pfs0 + 1]

	puts "\tRepmgr$tnum.f: Change ack policy to 'none'."
	$masterenv repmgr -ack none

	# Shut down client, so that from now on we will not be able to get any
	# acks.
	# 
	close $client
	set count 0
	puts $master "is_connected 0"
	while {[gets $master]} {
		if {[incr count] > 30} {
			error "FAIL: couldn't disconnect within 30 seconds"
		}
		tclsleep 1
		puts $master "is_connected 0"
	}

	# Make sure that a subordinate process sending a live log record
	# observes the ack policy of the environment.  The default policy would
	# be QUORUM, but the policy currently in effect is NONE.  With no
	# clients running, we won't get any acks.  If the default policy were in
	# effect, this would cause a perm failure.  Thus, if we don't get a perm
	# failure we can conclude that the NONE policy must have been used.
	# 
	puts "\tRepmgr$tnum.g: Start second master process, rep-unaware."
	set m2 [open "| $site_prog" "r+"]
	fconfigure $m2 -buffering line
	puts $m2 "home $masterdir"
	puts $m2 "output $testdir/m2output"
	puts $m2 "open_env"
	puts $m2 "open_db test.db"
	puts $m2 "put sub1 abc"
	puts $m2 "echo putted"
	set sentinel [gets $m2]
	error_check_good m2_firstputted $sentinel "putted"

	puts "\tRepmgr$tnum.h: Make sure no more perm failures occurred."
	set pfs2 [stat_field $masterenv repmgr_stat $perm_failures]
	error_check_good fail_count $pfs2 $pfs1

	puts "\tRepmgr$tnum.i: Clean up."
	$clientenv close
	$masterenv close
	close $master
	close $m2
}
