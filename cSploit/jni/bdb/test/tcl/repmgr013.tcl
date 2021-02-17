# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr013
# TEST	Site list test. 
# TEST
# TEST	Configure a master and two clients where one client is a peer of 
# TEST	the other and verify resulting site lists.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr013 { { niter 100 } { tnum "013" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr site list test."
	repmgr013_sub $method $niter $tnum $args
}

proc repmgr013_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 3

	set small_iter [expr $niter / 10]

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

	puts "\tRepmgr$tnum.a: Start a master."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	puts "\tRepmgr$tnum.b: Start first client."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.c: Start second client as peer of first."
	set cl2_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 1] peer] \
	    -start client
	await_startup_done $clientenv2

	puts "\tRepmgr$tnum.d: Verify repmgr site lists."
	verify_sitelist $masterenv $nsites {}
	verify_sitelist $clientenv $nsites {}
	verify_sitelist $clientenv2 $nsites [list [lindex $ports 1]]

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}

# For numsites, supply the nsites value defined for the test.
# For peervec, supply a list of ports whose sites should be considered peers.
proc verify_sitelist { env numsites peervec } {
	set sitelist [$env repmgr_site_list]

	# Make sure there are expected number of other sites.
	error_check_good lenchk [llength $sitelist] [expr {$numsites - 1}]

	# Make sure eid and port are integers; host, status and peer are
	# the expected string values.
	set pvind 0
	foreach tuple $sitelist {
		error_check_good eidchk [string is integer -strict \
					     [lindex $tuple 0]] 1
		error_check_good hostchk [lindex $tuple 1] "127.0.0.1"
		set port [lindex $tuple 2]
		error_check_good portchk [string is integer -strict $port] 1
		error_check_good statchk [lindex $tuple 3] connected
		if { [lsearch $peervec $port] >= 0 } {
			error_check_good peerchk [lindex $tuple 4] peer
		} else {
			error_check_good npeerchk [lindex $tuple 4] non-peer
		}
		incr pvind
	}
}
