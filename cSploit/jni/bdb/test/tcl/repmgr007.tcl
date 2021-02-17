# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr007
# TEST	Basic repmgr client shutdown/restart test.
# TEST
# TEST	Start an appointed master site and two clients. Shut down and
# TEST	restart each client, processing transactions after each restart.
# TEST	Verify all expected transactions are replicated.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr007 { { niter 100 } { tnum "007" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr client shutdown/restart test."
	repmgr007_sub $method $niter $tnum $args
}

proc repmgr007_sub { method niter tnum largs } {
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

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set clientdir2 $testdir/CLIENTDIR2

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $clientdir2

	# Open a master.
	puts "\tRepmgr$tnum.a: Start a master."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -errfile $testdir/rm7mas.err -home $masterdir \
	    -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	# Open first client
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

	# Open second client
	puts "\tRepmgr$tnum.c: Start second client."
	set cl2_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx CLIENT2 -home $clientdir2 -txn -rep -thread"
	set clientenv2 [eval $cl2_envcmd]
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 1]] \
	    -start client
	await_startup_done $clientenv2

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.d: Run first set of transactions at master."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.e: Shut down first client, wait and restart it."
	error_check_good client_close [$clientenv close] 0
	tclsleep 5
	# Open -recover to clear env region, including startup_done value.
	set clientenv [eval $cl_envcmd -recover]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 2]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.f: Run second set of transactions at master."
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs
	incr start $niter

	puts "\tRepmgr$tnum.g: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1

	puts "\tRepmgr$tnum.h: Shut down second client, wait and restart it."
	error_check_good client_close [$clientenv2 close] 0
	tclsleep 5
	# Open -recover to clear env region, including startup_done value.
	set clientenv2 [eval $cl2_envcmd -recover]
	$clientenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 2]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -remote [list 127.0.0.1 [lindex $ports 1]] \
	    -start client
	await_startup_done $clientenv2

	puts "\tRepmgr$tnum.i: Run third set of transactions at master."
	eval rep_test $method $masterenv NULL $niter $start 0 0 $largs

	puts "\tRepmgr$tnum.j: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1
	rep_verify $masterdir $masterenv $clientdir2 $clientenv2 1 1 1

	#
	# Test that repmgr won't crash on a small amount of unexpected
	# input over its port.  
	#
	puts "\tRepmgr$tnum.k: Test that repmgr ignores unexpected input."
	set msock [socket 127.0.0.1 [lindex $ports 0]]
	set garbage "abcdefghijklmnopqrstuvwxyz"
	puts $msock $garbage
	close $msock

	error_check_good client2_close [$clientenv2 close] 0
	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0

	#
	# We check the errfile after closing the env because the close
	# guarantees all messages are flushed to disk.
	#	
	set maserrfile [open $testdir/rm7mas.err r]
	set maserr [read $maserrfile]
	close $maserrfile
	error_check_good errchk [is_substr $maserr "unexpected msg type"] 1
}
