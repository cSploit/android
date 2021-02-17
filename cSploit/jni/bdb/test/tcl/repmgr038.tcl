# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr038
# TEST	repmgr view demotion test.
# TEST
# TEST	Create a replication group of a master and two clients.  Demote
# TEST	the second client to a view, then check site statistics, transaction
# TEST	apply and election behavior for demoted view.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr038 { { niter 100 } { tnum "038" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr view demotion test."
	repmgr038_sub $method $niter $tnum $args
}

proc repmgr038_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 3

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]
	set omethod [convert_method $method]

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# Turn off 2SITE_STRICT at all sites to make sure we accurately test 
	# that a view won't become master when there is only one other
	# unelectable site later in the test.

	# Open a master.
	puts "\tRepmgr$tnum.a: Start a master."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv rep_config {mgr2sitestrict off}
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	puts "\tRepmgr$tnum.b: Start two clients."
	set cl_envcmd "berkdb_env_noerr -create $verbargs -errpfx CLIENT \
	    -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv rep_config {mgr2sitestrict off}
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv
	#
	# We need to open the client2 environment as a participant and as a
	# view in different parts of this test.  Start by opening it as a
	# regular client participant here.  We will later open it as a view
	# to test demotion and then again as a participant to test for the
	# expected error.  Define both env commands here.
	#
	set cl2_envcmd "berkdb_env_noerr -create $verbargs -errpfx CLIENT2 \
	    -home $clientdir2 -txn -rep -thread"
	set viewcb ""
	set view_envcmd "berkdb_env_noerr -create $verbargs -errpfx VIEW \
	    -rep_view \[list $viewcb \] -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 rep_config {mgr2sitestrict off}
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv2

	puts "\tRepmgr$tnum.b1: Check initial repmgr site-related stats."
	error_check_good m3tot \
	    [stat_field $masterenv repmgr_stat "Total sites"] 3
	error_check_good m3part \
	    [stat_field $masterenv repmgr_stat "Participant sites"] 3
	error_check_good m0view \
	    [stat_field $masterenv repmgr_stat "View sites"] 0
	error_check_good c_3tot \
	    [stat_field $clientenv repmgr_stat "Total sites"] 3
	error_check_good c_3part \
	    [stat_field $clientenv repmgr_stat "Participant sites"] 3
	error_check_good c_0view \
	    [stat_field $clientenv repmgr_stat "View sites"] 0
	error_check_good c2_3tot \
	    [stat_field $clientenv2 repmgr_stat "Total sites"] 3
	error_check_good c2_3part \
	    [stat_field $clientenv2 repmgr_stat "Participant sites"] 3
	error_check_good c2_0view \
	    [stat_field $clientenv2 repmgr_stat "View sites"] 0

	puts "\tRepmgr$tnum.c: Run first set of transactions at master."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	puts "\tRepmgr$tnum.d: Try to restart client2 as view with no master."
	# Close client2 first to prevent election of another master.
	error_check_good clientenv2_close [$clientenv2 close] 0
	error_check_good masterenv_close [$masterenv close] 0

	set clientenv2 [eval $view_envcmd -recover]
	$clientenv2 rep_config {mgr2sitestrict off}
	catch {$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client} res
	error_check_good unavail [is_substr $res "DB_REP_UNAVAIL"] 1
	error_check_good clientenv2_close [$clientenv2 close] 0

	puts "\tRepmgr$tnum.e: Restart master, restart client2 as view."
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	set clientenv2 [eval $view_envcmd -recover]
	$clientenv2 rep_config {mgr2sitestrict off}
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv2

	puts "\tRepmgr$tnum.e1: Check repmgr site-related stats after demotion."
	# Brief pause to give gmdb time to catch up with restarted sites.
	tclsleep 1
	error_check_good m3tot \
	    [stat_field $masterenv repmgr_stat "Total sites"] 3
	error_check_good m2part \
	    [stat_field $masterenv repmgr_stat "Participant sites"] 2
	error_check_good m1view \
	    [stat_field $masterenv repmgr_stat "View sites"] 1
	error_check_good c_3tot \
	    [stat_field $clientenv repmgr_stat "Total sites"] 3
	error_check_good c_2part \
	    [stat_field $clientenv repmgr_stat "Participant sites"] 2
	error_check_good c_1view \
	    [stat_field $clientenv repmgr_stat "View sites"] 1
	error_check_good c2_3tot \
	    [stat_field $clientenv2 repmgr_stat "Total sites"] 3
	error_check_good c2_2part \
	    [stat_field $clientenv2 repmgr_stat "Participant sites"] 2
	error_check_good c2_1view \
	    [stat_field $clientenv2 repmgr_stat "View sites"] 1

	puts "\tRepmgr$tnum.f: Run second set of transactions at master."
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	puts "\tRepmgr$tnum.g: Close view, try to reopen as participant."
	error_check_good clientenv2_close [$clientenv2 close] 0
	#
	# Although cl2_envcmd succeeds, the repmgr command will fail.  Capture
	# its internal error to a file to check it.
	#
	set clientenv2 [eval $cl2_envcmd -recover -errfile $testdir/rm38c2.err]
	error_check_bad disallow_reopen_part \
	    [catch {$clientenv2 repmgr -ack all -pri 70 \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client}] 0
	error_check_good clientenv2_close [$clientenv2 close] 0
	# Check file after env close to make sure output is flushed to disk.
	set c2errfile [open $testdir/rm38c2.err r]
	set c2err [read $c2errfile]
	close $c2errfile
	error_check_good errchk [is_substr $c2err \
	    "A view site must be started with a view callback"] 1

	puts "\tRepmgr$tnum.h: Reopen view, make other client unelectable."
	# Both view_envcmd and the repmgr command will succeed here.
	set clientenv2 [eval $view_envcmd -recover]
	$clientenv2 rep_config {mgr2sitestrict off}
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv2
	$clientenv repmgr -pri 0

	puts "\tRepmgr$tnum.i: Run third set of transactions at master."
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	puts "\tRepmgr$tnum.j: Shut down master."
	error_check_good masterenv_close [$masterenv close] 0
	puts "\tRepmgr$tnum.k: Pause 20 seconds to verify no view takeover."
	tclsleep 20
	error_check_bad c2_master [stat_field $clientenv2 rep_stat "Master"] 1

	error_check_good clientenv2_close [$clientenv2 close] 0
	error_check_good clientenv_close [$clientenv close] 0
}
