# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST repmgr107
# TEST Repmgr combined with replication-unaware process at master.

proc repmgr107 { } {
	source ./include.tcl

	set tnum "107"
	puts "Repmgr$tnum: Replication-unaware process at master."

	env_cleanup $testdir
	set ports [available_ports 2]
	foreach {mport cport} $ports {}

	file mkdir [set mdir $testdir/MASTER]
	file mkdir [set cdir $testdir/CLIENT]

	# Using heartbeats because this test relies on the rerequest
	# processing based on heartbeats.
	#
	puts "\tRepmgr$tnum.a: Set up simple master/client pair."
	set dbconfig {
		{rep_set_timeout DB_REP_ACK_TIMEOUT 2000000}
		{rep_set_timeout DB_REP_HEARTBEAT_SEND 500000}
		{rep_set_timeout DB_REP_HEARTBEAT_MONITOR 1100000}
	}
	make_dbconfig $mdir \
            [linsert $dbconfig 0 [list repmgr_site 127.0.0.1 $mport db_local_site on]]
	set cmds {
		"home $mdir"
		"output $testdir/moutput"
		"open_env"
		"start master"
		"open_db test.db"
		"put k1 v1"
		"put k2 v2"
	}
	set m [open_site_prog [subst $cmds]]

	make_dbconfig $cdir \
            [linsert $dbconfig 0 \
                 [list repmgr_site 127.0.0.1 $cport db_local_site on] \
                 [list repmgr_site 127.0.0.1 $mport db_bootstrap_helper on]]
	set cmds {
		"home $cdir"
		"output $testdir/coutput"
		"open_env"
		"start client"
	}
	set c [open_site_prog [subst $cmds]]

	puts "\tRepmgr$tnum.b: Wait for client to finish start-up."
	set cenv [berkdb_env -home $cdir]
	await_startup_done $cenv

	puts "\tRepmgr$tnum.c: Run checkpoint in a separate process."
	exec $util_path/db_checkpoint -h $mdir -1

	# Find out where the checkpoint record is.
	# 
	set menv [berkdb_env -home $mdir]
	set curs [$menv log_cursor]
	set ckp_lsn1 [lindex [$curs get -last] 0]

	puts "\tRepmgr$tnum.d: Write more log records at master."
	puts $m "put k3 v3"
	puts $m "put k4 v4"
	puts $m "echo done"
	gets $m

	puts "\tRepmgr$tnum.e: Do another checkpoint."
	exec $util_path/db_checkpoint -h $mdir -1
	set ckp_lsn2 [lindex [$curs get -last] 0]

	error_check_bad same_ckp_lsn $ckp_lsn2 $ckp_lsn1

	# db_checkpoint could have produced perm failures, because it doesn't
	# start repmgr explicitly.  Instead repmgr starts up automatically, on
	# the fly, by trapping the first transmitted log record that gets sent.
	# This causes a connection to be initiated, but that may take some time,
	# too much time for that first log record to be transmitted.  This means
	# the client will have to request retransmission of this log record
	# "gap".
	#
	# So, pause for a moment, to let replication's gap measurement algorithm
	# expire, and then send one more transaction from the master, so that
	# the client is forced to request the gap if necessary.
	# 
	set perm_failures "Acknowledgement failures"
	set pfs1 [stat_field $menv repmgr_stat $perm_failures]
	tclsleep 2
	
	puts $m "put k5 v5"
	puts $m "echo done"
	gets $m
	set pfs2 [stat_field $menv repmgr_stat $perm_failures]

	# The last "put" operation shouldn't have resulted in any additional
	# perm failures.
	# 
	error_check_good perm_fail $pfs2 $pfs1

	# Pause again to allow time for the request for retransmission to be
	# fulfilled.
	# 
	tclsleep 2

	# At this point that both checkpoint operations should have been
	# successfully replicated.  Examine the client-side log at the expected
	# LSNs.
	# 
	puts "\tRepmgr$tnum.f: Examine client log."
	foreach lsn [list $ckp_lsn1 $ckp_lsn2] {
		set lsnarg [join $lsn /]
		set listing [exec $util_path/db_printlog \
				 -h $cdir -b $lsnarg -e $lsnarg]
		
		set first_line [lindex [split $listing "\n"] 0]
		error_check_good found_ckp \
		    [string match "*__txn_ckp*" $first_line] 1
	}

	$curs close
	$cenv close
	$menv close
	close $c
	close $m
}
