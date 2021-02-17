# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	env016
# TEST	Replication settings and DB_CONFIG
# TEST
# TEST	Create a DB_CONFIG for various replication settings.  Use
# TEST	rep_stat or getter functions to verify they're set correctly.
#
proc env016 { } {
	global errorCode
	source ./include.tcl

	puts "Env016: Replication DB_CONFIG settings."

	#
	# Test options that we query via rep_stat.
	# Structure of the list is:
	#	0. Arg used in DB_CONFIG.
	#	1. Value assigned in DB_CONFIG.
	#	2. Message output during test.
	#	3. String to search for in stat output.
	#
	set slist {
		{ "rep_set_priority" "1" "Env016.a0: Priority"
		    "Environment priority" }
	}
	puts "\tEnv016.a: Check settings via rep_stat."
	foreach l $slist {
		set carg [lindex $l 0]
		set val [lindex $l 1]
		set msg [lindex $l 2]
		set str [lindex $l 3]
		env_cleanup $testdir
		replsetup $testdir/MSGQUEUEDIR
		set masterdir $testdir/MASTERDIR
		file mkdir $masterdir
		repladd 1

		# Open a master.
		puts "\t\t$msg"
		#
		# Create DB_CONFIG.
		#
		env016_make_config $masterdir $carg $val
		#
		# Open env.
		#
		set ma_envcmd "berkdb_env_noerr -create -txn nosync \
		    -home $masterdir -errpfx MASTER -rep_master \
		    -rep_transport \[list 1 replsend\]"
		set masterenv [eval $ma_envcmd]
		#
		# Verify value
		#
		set gval [stat_field $masterenv rep_stat $str]
		error_check_good stat_get $gval $val

		error_check_good masterenv_close [$masterenv close] 0
		replclose $testdir/MSGQUEUEDIR
	}

	# Test options that we query via getter functions.
	# Structure of the list is:
	#	0. Arg used in DB_CONFIG.
	#	1. Value assigned in DB_CONFIG.
	#	2. Message output during test.
	#	3. Getter command.
	#	4. Getter results expected if different from #1 value.
	set glist {
		{ "rep_set_clockskew" "102 100" "Env016.b0: Rep clockskew"
		    "rep_get_clockskew" }
		{ "rep_set_config" "db_rep_conf_autoinit off" 
		    "Env016.b1: Rep config: autoinit"
		    "rep_get_config autoinit" "0" }
		{ "rep_set_config" "db_rep_conf_bulk" 
		    "Env016.b1: Rep config: bulk"
		    "rep_get_config bulk" "1" }
		{ "rep_set_config" "db_rep_conf_delayclient" 
		    "Env016.b1: Rep config: delayclient"
		    "rep_get_config delayclient" "1" }
		{ "rep_set_config" "db_rep_conf_inmem" 
		    "Env016.b1: Rep config: inmem"
		    "rep_get_config inmem" "1" }
		{ "rep_set_config" "db_rep_conf_lease" 
                    "Env016.b1: Rep config: lease"
		    "rep_get_config lease" "1" }
		{ "rep_set_config" "db_rep_conf_nowait" 
		    "Env016.b1: Rep config: nowait"
		    "rep_get_config nowait" "1" }
		{ "rep_set_config" "db_repmgr_conf_elections off" 
		    "Env016.b1: Repmgr config: elections"
		    "rep_get_config mgrelections" "0" }
		{ "rep_set_config" "db_repmgr_conf_2site_strict" 
		    "Env016.b1: Repmgr config: 2 site strict"
		    "rep_get_config mgr2sitestrict" "1" }
		{ "rep_set_limit" "0 1048576" "Env016.b2: Rep limit"
		    "rep_get_limit" }
		{ "rep_set_nsites" "6" "Env016.b3: Rep nsites"
		    "rep_get_nsites" }
		{ "rep_set_priority" "1" "Env016.b4: Rep priority"
		    "rep_get_priority" }
		{ "rep_set_request" "5000 10000" "Env016.b5: Rep request"
		    "rep_get_request" }
		{ "rep_set_timeout" "db_rep_ack_timeout 50000"
		    "Env016.b6: Rep ack timeout"
		    "rep_get_timeout ack" "50000" }
		{ "rep_set_timeout" "db_rep_checkpoint_delay 500000"
		    "Env016.b6: Rep ckp timeout"
		    "rep_get_timeout checkpoint_delay" "500000" }
		{ "rep_set_timeout" "db_rep_connection_retry 500000"
		    "Env016.b6: Rep connection retry timeout"
		    "rep_get_timeout connection_retry" "500000" }
		{ "rep_set_timeout" "db_rep_election_timeout 500000"
		    "Env016.b6: Rep elect timeout" "rep_get_timeout election" 
		    "500000" }
		{ "rep_set_timeout" "db_rep_election_retry 100000"
		    "Env016.b6: Rep election retry timeout" 
	 	    "rep_get_timeout election_retry" 
		    "100000" }
		{ "rep_set_timeout" "db_rep_full_election_timeout 500000"
		    "Env016.b6: Rep full election timeout"
		    "rep_get_timeout full_election" "500000" }
		{ "rep_set_timeout" "db_rep_heartbeat_monitor 50000"
		    "Env016.b6: Rep heartbeat monitor timeout"
		    "rep_get_timeout heartbeat_monitor" "50000" }
		{ "rep_set_timeout" "db_rep_heartbeat_send 50000"
		    "Env016.b6: Rep heartbeat send timeout"
		    "rep_get_timeout heartbeat_send" "50000" }
		{ "rep_set_timeout" "db_rep_lease_timeout 500"
		    "Env016.b6: Rep lease timeout"
		    "rep_get_timeout lease" "500" }
		{ "repmgr_set_ack_policy" "db_repmgr_acks_all"
		    "Env016.b8: Repmgr acks_all"
		    "repmgr_get_ack_policy" "all" }
		{ "repmgr_set_ack_policy" "db_repmgr_acks_all_available"
		    "Env016.b8: Repmgr acks_all_available"
		    "repmgr_get_ack_policy" "allavailable" }
		{ "repmgr_set_ack_policy" "db_repmgr_acks_all_peers"
		    "Env016.b8: Repmgr acks_all_peers"
		    "repmgr_get_ack_policy" "allpeers" }
		{ "repmgr_set_ack_policy" "db_repmgr_acks_none"
		    "Env016.b8: Repmgr acks_none"
		    "repmgr_get_ack_policy" "none" }
		{ "repmgr_set_ack_policy" "db_repmgr_acks_one"
		    "Env016.b8: Repmgr acks_one"
		    "repmgr_get_ack_policy" "one" }
		{ "repmgr_set_ack_policy" "db_repmgr_acks_one_peer"
		    "Env016.b8: Repmgr acks_one_peer"
		    "repmgr_get_ack_policy" "onepeer" }
		{ "repmgr_set_ack_policy" "db_repmgr_acks_quorum"
		    "Env016.b8: Repmgr acks_quorum"
		    "repmgr_get_ack_policy" "quorum" }
		{ "repmgr_set_incoming_queue_max" "1000000 1000"
		    "Env016.b9: Repmgr incoming queue max"
		    "repmgr_get_inqueue_max" }
		{ "repmgr_site" "example.com 49200 db_local_site on"
		    "Env016.b10: Repmgr set local site"
		    "repmgr_get_local_site" "example.com 49200" }
	}
	puts "\tEnv016.b: Check settings via getter functions."
	foreach l $glist {
		set carg [lindex $l 0]
		set val [lindex $l 1]
		set msg [lindex $l 2]
		set getter [lindex $l 3]
		if { [llength $l] > 4 } {
			set getval [lindex $l 4]
		} else {
			set getval $val
		}
		env_cleanup $testdir
		replsetup $testdir/MSGQUEUEDIR
		set masterdir $testdir/MASTERDIR
		file mkdir $masterdir
		repladd 1

		# Open a master.
		puts "\t\t$msg"
		#
		# Create DB_CONFIG.
		#
		env016_make_config $masterdir $carg $val
		#
		# Open env.
		#
		set ma_envcmd "berkdb_env_noerr -create -txn \
		    -home $masterdir -rep"
		set masterenv [eval $ma_envcmd]
		#
		# Verify value
		#
		set gval [eval $masterenv $getter]
		error_check_good stat_get $gval $getval

		error_check_good masterenv_close [$masterenv close] 0
		replclose $testdir/MSGQUEUEDIR
	}



	puts "\tEnv016.c: Test that bad rep config values are rejected."
	set bad_glist {
		{ "rep_set_clockskew" "103" }
		{ "rep_set_config" "db_rep_conf_bulk x" }
		{ "rep_set_config" "db_rep_conf_xxx" }
		{ "rep_set_config" "db_rep_conf_bulk x x1" }
		{ "rep_set_limit" "1" }
		{ "rep_set_nsites" "5 x" }
		{ "rep_set_priority" "100 200" }
		{ "rep_set_request" "500" }
		{ "rep_set_timeout" "db_rep_ack_timeout" }
		{ "rep_set_timeout" "db_rep_xxx_timeout 50" }
		{ "repmgr_set_ack_policy" "db_repmgr_acks_all on" }
		{ "repmgr_set_ack_policy" "db_repmgr_acks_xxx" }
		{ "repmgr_site" "localhost" }
		{ "repmgr_site" "localhost 10001 peer" }
		{ "repmgr_site" "localhost 10001 xxxx on" }
	}

	foreach l $bad_glist {
		set carg [lindex $l 0]
		set val [lindex $l 1]

		env_cleanup $testdir
		set masterdir $testdir/MASTERDIR
		file mkdir $masterdir

		env016_make_config $masterdir $carg $val

		set ma_envcmd "berkdb_env_noerr -create -txn \
		    -home $masterdir -rep"
		set masterenv [catch {eval $ma_envcmd} ret]
		error_check_good envopen $masterenv 1
		error_check_good error [is_substr $errorCode EINVAL] 1
	}
}

proc env016_make_config { dir carg cval } {
	set cid [open $dir/DB_CONFIG w]
	puts $cid "$carg $cval"
	close $cid
}
