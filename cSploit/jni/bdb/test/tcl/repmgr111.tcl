# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST repmgr111
# TEST Multi-process repmgr with env open before set local site.

proc repmgr111 { } {
	source ./include.tcl

	set tnum "111"
	puts "Repmgr$tnum: set local site after env open."
	set site_prog [setup_site_prog]

	env_cleanup $testdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set ports [available_ports 2]
	set master_port [lindex $ports 0]
	set client_port [lindex $ports 1]

	puts "\tRepmgr$tnum.a: Set up the master (port $master_port)."
	set master [open "| $site_prog" "r+"]
	fconfigure $master -buffering line
	puts $master "home $masterdir"
	make_dbconfig $masterdir {{rep_set_config db_repmgr_conf_2site_strict off}}
	puts $master "output $testdir/m1output"
	puts $master "open_env"
	puts $master "local $master_port"
	puts $master "start master"
	set ignored [gets $master]

	puts "\tRepmgr$tnum.b: Set up the client (on TCP port $client_port)."
	set client [open "| $site_prog" "r+"]
	fconfigure $client -buffering line
	puts $client "home $clientdir"
	puts $client "local $client_port"
	make_dbconfig $clientdir {{rep_set_config db_repmgr_conf_2site_strict off}}
	puts $client "output $testdir/coutput"
	puts $client "open_env"
	puts $client "remote 127.0.0.1 $master_port"
	puts $client "start client"
	error_check_match start_client [gets $client] "*Successful*"

	puts "\tRepmgr$tnum.c: Wait for STARTUPDONE."
	set clientenv [berkdb_env -home $clientdir]
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.d: Start second master process, rep-unaware."
	set m2 [open "| $site_prog" "r+"]
	fconfigure $m2 -buffering line
	puts $m2 "home $masterdir"
	puts $m2 "output $testdir/m2output"
	puts $m2 "open_env"
	puts $m2 "open_db test.db"
	puts $m2 "put sub1 abc"
	tclsleep 1
	puts $m2 "put sub2 def"
	puts $m2 "echo putted"
	set sentinel [gets $m2]
	error_check_good m2_firstputted $sentinel "putted"

	puts "\tRepmgr$tnum.e: Check that replicated data is visible at client."
	puts $client "open_db test.db"
	set expected {{sub1 abc}}
	verify_client_data $clientenv test.db $expected

	puts "\tRepmgr$tnum.f: Clean up."
	$clientenv close
	close $client
	close $master
	close $m2
}
