# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr004
# TEST	Test undocumented incoming queue maximum number of messages.
# TEST
# TEST	Test that limiting the number of messages that the repmgr incoming
# TEST	queue can hold has the expected side-effect of causing rerequests.
# TEST	Also test the interface defaults and the associated undocumented
# TEST	stat.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr004 { { niter 100 } { tnum "004" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	#
	# The timing on Windows is different enough to preclude reliably
	# getting the rerequests needed to verify that we filled the queue.
	#
	if { $is_windows_test } {
		puts "Skipping repmgr004 for Windows platform"
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr incoming queue limit test."
	repmgr004_sub $method $niter $tnum $args
}

proc repmgr004_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 3
	# Defaults for incoming queue values.
	set msgs 5000000
	set bmsgs 5000
	# Short set_request times to encourage rerequests.
	set req_min 4000
	set req_max 128000
	# Small number of transactions to clear out input queue.
	set small_iter 1
	if { $niter > 20 } {
		set small_iter [expr $niter / 20]
	}

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	puts "\tRepmgr$tnum.a: Start master and two clients."
	set ma_envcmd "berkdb_env -create $verbargs \
	    -errpfx MASTER -home $masterdir \
	    -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master
	# Client that receives one record at a time (singleton.)
	set cl_envcmd "berkdb_env -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv
	$clientenv rep_request $req_min $req_max
	# Client that receives multiple records at one time (bulk.)
	set cl2_envcmd "berkdb_env -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv2
	$clientenv2 rep_config {bulk on}
	$clientenv2 rep_request $req_min $req_max

	#
	# Use of -ack all guarantees replication completes before repmgr send
	# function returns and rep_test finishes.  But this guarantee doesn't
	# hold when we have deliberately filled up the incoming queue and
	# are getting PERM_FAILs.
	#
	puts "\tRepmgr$tnum.b: Run first set of transactions at master."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.c: Test incoming queue defaults."
	set inqmax [$clientenv repmgr_get_inqueue_max]
	error_check_good init_inqueue_defaults \
	    [expr [lindex $inqmax 0] == $msgs && \
	    [lindex $inqmax 1] == $bmsgs] 1
	# Try 0 for each individual incoming queue value.
	set val1 1234
	set val2 987
	$clientenv repmgr -inqueue [list $val1 0]
	set inqmax [$clientenv repmgr_get_inqueue_max]
	error_check_good inqueue_def_bmsgs \
	    [expr [lindex $inqmax 0] == $val1 && [lindex $inqmax 1] == $bmsgs] 1
	$clientenv repmgr -inqueue [list 0 $val2]
	set inqmax [$clientenv repmgr_get_inqueue_max]
	error_check_good inqueue_def_msgs \
	    [expr [lindex $inqmax 0] == $msgs && [lindex $inqmax 1] == $val2] 1

	puts "\tRepmgr$tnum.d: Test that full queue drops messages."
	set c_rereqs [stat_field $clientenv rep_stat "Log records requested"]
	set c2_rereqs [stat_field $clientenv2 rep_stat "Log records requested"]

	puts "\tRepmgr$tnum.d.1: Set tiny queue max values."
	# We need tiny values to make sure we get some rerequests.
	$clientenv repmgr -inqueue [list 2 1]
	$clientenv2 repmgr -inqueue [list 2 1]

	puts "\tRepmgr$tnum.d.2: Process enough messages so that some get\
	    dropped."
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.d.3: Set back to default queue max values."
	# Use 0 for both incoming queue values to get defaults.
	$clientenv repmgr -inqueue [list 0 0]
	set inqmax [$clientenv repmgr_get_inqueue_max]
	error_check_good inqueue_def_bmsgs \
	    [expr [lindex $inqmax 0] == $msgs && \
	    [lindex $inqmax 1] == $bmsgs] 1
	$clientenv2 repmgr -inqueue [list 0 0]

	#
	# We need a few more transactions to clear out the congested input
	# queues and predictably process all expected rerequests.  We can't
	# use repmgr heartbeats for automatic rerequests because they could
	# cause an election when the queues are full.  Base replication
	# rerequests are triggered only by master activity.
	#
	puts "\tRepmgr$tnum.d.4: Clear queues with a few more transactions."
	eval rep_test $method $masterenv NULL $small_iter $start 0 0 $largs
	incr start $small_iter

	puts "\tRepmgr$tnum.d.5: Verify that there were some rerequests."
	set c_rereqs2 [stat_field $clientenv rep_stat "Log records requested"]
	set c2_rereqs2 [stat_field $clientenv2 rep_stat "Log records requested"]
	error_check_good c_rereq [expr $c_rereqs2 > $c_rereqs] 1
	error_check_good c2_rereq [expr $c2_rereqs2 > $c2_rereqs] 1

	puts "\tRepmgr$tnum.d.6: Run more transactions at master."
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.d.7: Verify no more rerequests."
	set c_rereqs3 [stat_field $clientenv rep_stat "Log records requested"]
	set c2_rereqs3 [stat_field $clientenv2 rep_stat "Log records requested"]
	error_check_good c_norereq [expr $c_rereqs2 == $c_rereqs3] 1
	error_check_good c2_norereq [expr $c2_rereqs2 == $c2_rereqs3] 1

	puts "\tRepmgr$tnum.e: Access stat for incoming queue size."
	error_check_good inqueue_size_stat \
	    [stat_field $clientenv repmgr_stat "Incoming queue size"] 0

	puts "\tRepmgr$tnum.f: Verify client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}
