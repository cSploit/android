# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr036
# TEST	Basic repmgr view test.
# TEST
# TEST	Start an appointed master site and one view.  Ensure replication
# TEST	is occurring to the view.  Shut down master, ensure view does not
# TEST	take over as master.  Restart master and make sure further master
# TEST	changes are replicated to view.  Test view-related stats and
# TEST	flag indicator in repmgr_site_list output.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr036 { { niter 100 } { tnum "036" } args } {

	#
	# Note about view callback restriction in repmgr tcl tests: repmgr
	# tests can only use the default "" view callback that replicates
	# everything.  Attempts to set a non-default callback fail because
	# the callback is set in a tcl thread but used in a repmgr message
	# thread and this violates tcl stack checking.  It is possible
	# to make this work if you rebuild tcl with TCL_NO_STACK_CHECKING
	# defined, but this isn't a generally safe way to run tcl.
	#

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr basic view test."
	repmgr036_sub $method $niter $tnum $args
}

proc repmgr036_sub { method niter tnum largs } {
	global rep_verbose
	global testdir
	global verbose_type

	set nsites 2
	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports $nsites]
	set omethod [convert_method $method]

	set masterdir $testdir/MASTERDIR
	set viewdir $testdir/VIEWDIR

	file mkdir $masterdir
	file mkdir $viewdir

	#
	# Create a 2-site replication group containing a master and a view.
	# Use ack_policy ALL to make sure master can operate in the absence
	# of expected transaction commit acks, which the view cannot send.
	# Set 2SITE_STRICT on both sites to make it possible for the one other
	# site in the group to elect itself master and then test that the
	# view does not do this.
	#

	# Open a master.
	puts "\tRepmgr$tnum.a: Start a master."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv rep_config {mgr2sitestrict off}
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	# Add some master data to make sure the view gets it during its
	# internal init.
	puts "\tRepmgr$tnum.b: Run first set of transactions at master."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter

	# Open a view.
	puts "\tRepmgr$tnum.c: Start view client."
	set viewcb ""
	set view_envcmd "berkdb_env_noerr -create $verbargs -errpfx VIEW \
	    -rep_view \[list $viewcb \] -home $viewdir -txn -rep -thread"
	set viewenv [eval $view_envcmd]
	$viewenv rep_config {mgr2sitestrict off}
	# Try incorrectly starting view with master and election.
	error_check_bad disallow_master_start \
	    [catch {$viewenv repmgr -local [list 127.0.0.1 [lindex $ports 1]] \
	    -start master}] 0
	error_check_bad disallow_elect_start \
	    [catch {$viewenv repmgr -local [list 127.0.0.1 [lindex $ports 1]] \
	    -start elect}] 0
	# Start view correctly as client and verify contents.
	$viewenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $viewenv
	rep_verify $masterdir $masterenv $viewdir $viewenv 1 1 1

	puts "\tRepmgr$tnum.d: Create new database on master."
	# Create and populate this database manually so that the later
	# test of read access to the view when the master is down can
	# easily validate the expected values.
	set dbname  "testview.db"
	set numtxns 10
	set mdb [eval "berkdb_open_noerr -create $omethod -auto_commit \
	    -env $masterenv $largs $dbname"]
	set t [$masterenv txn]
	for { set i 1 } { $i <= $numtxns } { incr i } {
		error_check_good db_put \
		    [eval $mdb put -txn $t $i [chop_data $method data$i]] 0
	}
	error_check_good txn_commit [$t commit] 0
	error_check_good mdb_close [$mdb close] 0

	puts "\tRepmgr$tnum.e: Verify new database transactions on view."
	# Brief pause to make sure view has time to catch up.
	tclsleep 1
	set vdb [eval "berkdb_open_noerr -create -mode 0644 $omethod \
	    -env $viewenv $largs $dbname"]
	error_check_good repmgr036_db [is_valid_db $vdb] TRUE
	for { set i 1 } { $i <= $numtxns } { incr i } {
		set ret [lindex [$vdb get $i] 0]
		error_check_good vdb_get $ret [list $i \
		    [pad_data $method data$i]]
	}
	error_check_good vdb_close [$vdb close] 0

	puts "\tRepmgr$tnum.f: Shut down master."
	error_check_good masterenv_close [$masterenv close] 0
	puts "\tRepmgr$tnum.g: Pause 20 seconds to verify no view takeover."
	tclsleep 20
	error_check_bad c2_master [stat_field $viewenv rep_stat "Master"] 1

	puts "\tRepmgr$tnum.h: Verify read access on view."
	set vdb [eval "berkdb_open_noerr -create -mode 0644 $omethod \
	    -env $viewenv $largs $dbname"]
	error_check_good repmgr036r_db [is_valid_db $vdb] TRUE
	for { set i 1 } { $i <= $numtxns } { incr i } {
		set ret [lindex [$vdb get $i] 0]
		error_check_good vdbr_get $ret [list $i \
		    [pad_data $method data$i]]
	}
	error_check_good vdbr_close [$vdb close] 0

	puts "\tRepmgr$tnum.i: Restart master"
	set masterenv [eval $ma_envcmd]
	$masterenv rep_config {mgr2sitestrict off}
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master
	await_expected_master $masterenv

	puts "\tRepmgr$tnum.j: Perform more master transactions, verify view."
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter
	rep_verify $masterdir $masterenv $viewdir $viewenv 1 1 1

	puts "\tRepmgr$tnum.k: Check repmgr_site_list site category output."
	set msitelist [$masterenv repmgr_site_list]
	error_check_good mlenchk [llength $msitelist] 1
	error_check_good mviewchk [lindex [lindex $msitelist 0] 5] view
	set vsitelist [$viewenv repmgr_site_list]
	error_check_good vlenchk [llength $vsitelist] 1
	error_check_good vpartchk [lindex [lindex $vsitelist 0] 5] participant

	puts "\tRepmgr$tnum.l: Check repmgr site-related stats."
	error_check_good m2tot \
	    [stat_field $masterenv repmgr_stat "Total sites"] 2
	error_check_good m1part \
	    [stat_field $masterenv repmgr_stat "Participant sites"] 1
	error_check_good m1view \
	    [stat_field $masterenv repmgr_stat "View sites"] 1
	error_check_good v2tot \
	    [stat_field $viewenv repmgr_stat "Total sites"] 2
	error_check_good v1part \
	    [stat_field $viewenv repmgr_stat "Participant sites"] 1
	error_check_good v1view \
	    [stat_field $viewenv repmgr_stat "View sites"] 1

	puts "\tRepmgr$tnum.m: Close view and try to start as participant."
	error_check_good view_close [$viewenv close] 0
	set view_envcmd "berkdb_env_noerr -create $verbargs -errpfx VIEW \
	    -home $viewdir -txn -rep -thread"
	set viewenv [eval $view_envcmd]
	# Try starting repmgr on a view site without its view callback.
	# It is an error to promote a view to a participant.
	error_check_bad disallow_view_to_part \
	    [catch {$viewenv repmgr -local [list 127.0.0.1 [lindex $ports 1]] \
	    -start client}] 0

	error_check_good view_close [$viewenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}
