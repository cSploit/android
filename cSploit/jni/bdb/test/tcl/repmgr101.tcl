# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST repmgr101
# TEST Repmgr support for multi-process master.
# TEST
# TEST Start two processes at the master.
# TEST Add a client site (not previously known to the master 
# TEST processes), and make sure
# TEST both master processes connect to it.

proc repmgr101 {  } {
	source ./include.tcl

	set tnum "101"
	puts "Repmgr$tnum: Two master processes both connect to a client."
	set site_prog [setup_site_prog]

	env_cleanup $testdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set ports [available_ports 2]
	set master_port [lindex $ports 0]
	set client_port [lindex $ports 1]

	puts "\tRepmgr$tnum.a: Set up the master (on TCP port $master_port)."
	set master [open "| $site_prog" "r+"]
	fconfigure $master -buffering line
	puts $master "home $masterdir"
	make_dbconfig $masterdir \
	    [list [list repmgr_site 127.0.0.1 $master_port db_local_site on] \
	    "rep_set_config db_repmgr_conf_2site_strict off"]
	puts $master "output $testdir/m1output"
	puts $master "open_env"
	puts $master "start master"
	set ignored [gets $master]
	puts $master "open_db test.db"
	puts $master "put myKey myValue"

	# sync.
	puts $master "echo setup"
	set sentinel [gets $master]
	error_check_good echo_setup $sentinel "setup"
	
	puts "\tRepmgr$tnum.b: Start a second process at master."
	set m2 [open "| $site_prog" "r+"]
	fconfigure $m2 -buffering line
	puts $m2 "home $masterdir"
	puts $m2 "output $testdir/m2output"
	puts $m2 "open_env"
	puts $m2 "open_db test.db"
	puts $m2 "put sub1 abc"
	puts $m2 "echo firstputted"
	set sentinel [gets $m2]
	error_check_good m2_firstputted $sentinel "firstputted"

	puts "\tRepmgr$tnum.c: Set up the client (on TCP port $client_port)."
	set client [open "| $site_prog" "r+"]
	fconfigure $client -buffering line
	puts $client "home $clientdir"
	make_dbconfig $clientdir \
	    [list [list repmgr_site 127.0.0.1 $client_port db_local_site on] \
		 [list repmgr_site 127.0.0.1 $master_port db_bootstrap_helper on] \
	    "rep_set_config db_repmgr_conf_2site_strict off"]
	puts $client "output $testdir/coutput"
	puts $client "open_env"
	puts $client "start client"
	error_check_match start_client [gets $client] "*Successful*"

	puts "\tRepmgr$tnum.d: Wait for STARTUPDONE."
	set clientenv [berkdb_env -home $clientdir]
	await_startup_done $clientenv
	  
	# Initially there should be no rerequests.
	set pfs1 [stat_field $clientenv rep_stat "Log records requested"]
	error_check_good rerequest_count $pfs1 0

	# At this point we know that the master (in its main process) knows
	# about the client, so the client address should be in the shared
	# region.  The second master process will discover the address as a
	# result of being asked to send out the log records for the following
	# transaction.  At that point, it will initiate a connection attempt,
	# though without blocking the commit() call of the transaction.  This
	# means that this first transaction may or may not (probably won't) get
	# transmitted directly (as a "live" log record) to the client; it will
	# have to be "re-requested".  However, we can then wait for the
	# connection to be established, and thereafter all transactions should
	# be transmitted live; and we know that they must have arrived at the
	# client by the time the commit() returns, because of the ack policy.
	# 
	puts $m2 "put sub2 xyz"
	set count 0
	puts $m2 "is_connected $client_port"
	while {! [gets $m2]} {
		if {[incr count] > 30} {
			error "FAIL: couldn't connect within 30 seconds"
		}
		tclsleep 1
		puts $m2 "is_connected $client_port"
	}
	
	puts $m2 "put sub3 ijk"
	puts $m2 "put sub4 pqr"
	puts $m2 "echo putted"
	set sentinel [gets $m2]
	error_check_good m2_putted $sentinel "putted"
	puts $master "put another record"
	puts $master "put and again"
	puts $master "echo m1putted"
	set sentinel [gets $master]
	error_check_good m1_putted $sentinel "m1putted"

	puts "\tRepmgr$tnum.e: Check that replicated data is visible at client."
	puts $client "open_db test.db"
	set expected {{myKey myValue} {sub1 abc} {sub2 xyz} {another record}}
	verify_client_data $clientenv test.db $expected

	# make sure there weren't too many rerequests
	puts "\tRepmgr$tnum.f: Check rerequest stats"
	set pfs [stat_field $clientenv rep_stat "Log records requested"]
	error_check_good rerequest_count [expr $pfs <= 1] 1

	puts "\tRepmgr$tnum.g: Clean up."
	$clientenv close
	close $client
	close $master
	close $m2
}
