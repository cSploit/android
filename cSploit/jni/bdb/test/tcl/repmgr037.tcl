# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr037
# TEST	Election test for repmgr views.
# TEST
# TEST	Run a set of elections in a replication group containing views,
# TEST	making sure views never become master.  Run test for replication
# TEST	groups containing different numbers of clients, unelectable clients
# TEST	and views.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr037 { { niter 100 } { tnum "037" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr view election test."

	#
	# repmgr037_sub runs this test with a flexible configuration of
	# sites.  In addition to the standard method, niter, tnum and 
	# largs arguments, it takes the following arguments to specify
	# the configuration:
	#     cid - unique string to identify configuration in output
	#     nelects - number of electable clients (minimum 2 required)
	#     nunelects - number of unelectable clients
	#     nviews - number of views (minimum 1 required)
	#
	# Minimal configuration of 2 electable clients and 1 view.
	repmgr037_sub $method $niter $tnum "A" 2 0 1 $args
	# Configuration that can include a much slower view.
	repmgr037_sub $method $niter $tnum "B" 3 0 1 $args
	# More views than electable sites.
	repmgr037_sub $method $niter $tnum "C" 2 0 3 $args
	# Include an unelectable site.
	repmgr037_sub $method $niter $tnum "D" 2 1 1 $args
	# Large number of all types of sites.
	repmgr037_sub $method $niter $tnum "E" 5 4 7 $args
}

#
# Run election and view test with a flexible configuration.  Caller can
# specify 'nelects' electable clients, 'nunelects' unelectable clients and
# 'nviews' views.  A minimal configuration of 2 electable clients and 1
# view is required.
#
# This test uses two potential masters (the first two electable clients) and
# switches master several times.  It verifies that no views take over as
# master or participate in any of the elections.
#
proc repmgr037_sub { method niter tnum cid nelects nunelects nviews largs } {
	global testdir
	global rep_verbose
	global verbose_type

	if { $nelects < 2 || $nviews < 1 } {
		puts "Invalid configuration nelects $nelects nviews $nviews"
		return
	}

	#
	# Set up arrays of dirs, envs and envcmds.  Each array contains the
	# values for electable clients, any unelectable clients and then views,
	# respectively.  Define variables to find the start of each type of
	# site in the array.
	#
	set nsites [expr $nelects + $nunelects + $nviews]
	set ui [expr $nelects + 1]
	set vi [expr $nelects + $nunelects + 1]
	set mas1 1
	set mas2 2

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]
	set omethod [convert_method $method]

	for { set i 1 } { $i <= $nsites } { incr i } {
		if { $i < $vi } {
			set dirs($i) $testdir/SITE$i
		} else {
			set dirs($i) $testdir/VIEW$i
		}
		file mkdir $dirs($i)
	}

	puts -nonewline "Repmgr$tnum Config.$cid sites: electable $nelects, "
	puts "unelectable $nunelects, view $nviews."
	puts "\tRepmgr$tnum.a: Start all sites."
	# This test depends on using the default callback so that each view
	# is a fully-replicated copy of the data.
	set viewcb ""
	for { set i 1 } { $i <= $nsites } { incr i } {
		if { $i < $vi } {
			set envargs "-errpfx SITE$i"
		} else {
			set envargs "-errpfx VIEW$i -rep_view \[list $viewcb \]"
		}
		set envcmds($i) "berkdb_env_noerr -create $verbargs \
		    $envargs -home $dirs($i) -txn -rep -thread"
		set envs($i) [eval $envcmds($i)]
		# Turn off 2SITE_STRICT to make sure we accurately test that
		# a view won't become master or participate when there is only
		# one other electable site.
		$envs($i) rep_config {mgr2sitestrict off}
		if { $i == $mas1 } {
			$envs($i) repmgr -ack all -pri 100 \
			    -local [list 127.0.0.1 [lindex $ports 0]] \
			    -start master
		} else {
			if { $i < $ui } {
				# Give electable clients descending priorities
				# less than 80.
				set priority [expr 80 - $i]
			} elseif { $nunelects > 0 && $i < $vi } {
				# Make site unelectable with priority 0.
				set priority 0
			} else {
				# Give view higher priority than clients to
				# make sure it still won't become master.
				set priority 90
			}
			$envs($i) repmgr -ack all -pri $priority \
			    -local [list 127.0.0.1 \
			    [lindex $ports [expr $i - 1]]] \
			    -remote [list 127.0.0.1 [lindex $ports 0]] \
			    -start client
			await_startup_done $envs($i)
		}
	}

	puts "\tRepmgr$tnum.b: Run/verify first set of transactions."
	set start 0
	eval rep_test $method $envs($mas1) NULL $niter $start 0 0 $largs
	incr start $niter
	rep_verify $dirs($mas1) $envs($mas1) $dirs($mas2) $envs($mas2) 1 1 1
	rep_verify $dirs($mas1) $envs($mas1) $dirs($vi) $envs($vi) 1 1 1

	puts "\tRepmgr$tnum.c: Close first master, second master takes over."
	error_check_good m1_close [$envs($mas1) close] 0
	await_expected_master $envs($mas2)

	puts "\tRepmgr$tnum.d: Run/verify second set of transactions."
	eval rep_test $method $envs($mas2) NULL $niter $start 0 0 $largs
	incr start $niter
	rep_verify $dirs($mas2) $envs($mas2) $dirs($vi) $envs($vi) 1 1 1

	puts "\tRepmgr$tnum.e: Restart first master as client."
	set envs($mas1) [eval $envcmds($mas1)]
	$envs($mas1) repmgr -start client
	await_startup_done $envs($mas1)

	puts "\tRepmgr$tnum.f: Run/verify third set of transactions."
	eval rep_test $method $envs($mas2) NULL $niter $start 0 0 $largs
	incr start $niter
	rep_verify $dirs($mas2) $envs($mas2) $dirs($mas1) $envs($mas1) 1 1 1
	rep_verify $dirs($mas2) $envs($mas2) $dirs($vi) $envs($vi) 1 1 1

	puts "\tRepmgr$tnum.g: Close second master, first master takes over."
	error_check_good m2_close [$envs($mas2) close] 0
	await_expected_master $envs($mas1)

	puts "\tRepmgr$tnum.h: Run/verify fourth set of transactions."
	eval rep_test $method $envs($mas1) NULL $niter $start 0 0 $largs
	incr start $niter
	rep_verify $dirs($mas1) $envs($mas1) $dirs($vi) $envs($vi) 1 1 1

	puts "\tRepmgr$tnum.i: Close first master."
	error_check_good m3_close [$envs($mas1) close] 0
	# Both potential masters are now closed.
	puts "\tRepmgr$tnum.j: Pause 20 seconds to verify no view takeover."
	tclsleep 20

	puts "\tRepmgr$tnum.k: Check stats on each view."
	for { set i $vi } { $i <= $nsites } { incr i } {
		error_check_bad v_master [stat_field $envs($i) \
		    rep_stat "Master"] 1
		error_check_good v_noelections \
		    [stat_field $envs($i) rep_stat "Elections held"] 0
	}

	puts "\tRepmgr$tnum.l: Verify/close all remaining sites."
	for { set i [expr $mas2 + 1] } { $i <= $nsites } { incr i } {
		# First view was verified above and is used to verify all
		# remaining sites.
		if { $i != $vi } {
			rep_verify $dirs($vi) $envs($vi) \
			    $dirs($i) $envs($i) 1 1 1
		}
	}
	for { set i [expr $mas2 + 1] } { $i <= $nsites } { incr i } {
		error_check_good s_close [$envs($i) close] 0
	}
}
