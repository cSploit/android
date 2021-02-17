# See the file LICENSE for redistribution information.
#
# Copyright (c) 2007, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr009
# TEST	repmgr API error test.
# TEST
# TEST	Try a variety of repmgr calls that result in errors. Also
# TEST	try combinations of repmgr and base replication API calls
# TEST	that result in errors.
# TEST
# TEST	Run for btree only because access method shouldn't matter.
# TEST
proc repmgr009 { { niter 10 } { tnum "009" } args } {

	source ./include.tcl

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set method "btree"
	set args [convert_args $method $args]

	puts "Repmgr$tnum ($method): repmgr API error test."
	repmgr009_sub $method $niter $tnum $args
}

proc repmgr009_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	set nsites 2

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	env_cleanup $testdir
	set ports [available_ports [expr $nsites * 5]]

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set masterdir2 $testdir/MASTERDIR2
	set clientdir $testdir/CLIENTDIR
	set norepdir $testdir/NOREPDIR

	file mkdir $masterdir
	file mkdir $masterdir2
	file mkdir $clientdir
	file mkdir $norepdir

	puts "\tRepmgr$tnum.a: Set up environment without repmgr."
	set ma_envcmd "berkdb_env_noerr -create $verbargs \
	    -errpfx MASTER -home $masterdir -txn -rep -thread"
	set masterenv [eval $ma_envcmd]
	error_check_good masterenv_close [$masterenv close] 0

	puts "\tRepmgr$tnum.b: Call repmgr without open master (error)."
	catch {$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master} res
	error_check_good errchk [is_substr $res "invalid command"] 1

	puts "\tRepmgr$tnum.c: Call repmgr_stat without open master (error)."
	catch {[stat_field $masterenv repmgr_stat "Connections dropped"]} res
	error_check_good errchk [is_substr $res "invalid command"] 1

	puts "\tRepmgr$tnum.d: Call repmgr with 0 msgth (error)."
	set masterenv [eval $ma_envcmd]
	set ret [catch {$masterenv repmgr -start master -msgth 0 \
	    -local [list 127.0.0.1 [lindex $ports 0]]}]
	error_check_bad disallow_msgth_0 [is_substr $ret "invalid argument"] 1
	error_check_good allow_msgth_nonzero [$masterenv repmgr \
	    -start master -local [list 127.0.0.1 [lindex $ports 0]]] 0
	error_check_good masterenv_close [$masterenv close] 0

	puts "\tRepmgr$tnum.e: Start a master with repmgr."
	repladd 1
	set masterenv [eval $ma_envcmd]
	$masterenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master

	puts "\tRepmgr$tnum.f: Start repmgr with no local sites (error)."
	set cl_envcmd "berkdb_env_noerr -create $verbargs \
	    -home $clientdir -txn -rep -thread"
	set clientenv [eval $cl_envcmd]
	catch {$clientenv repmgr -ack all \
	    -remote [list 127.0.0.1 [lindex $ports 7]] \
	    -start client} res
	error_check_good errchk [is_substr $res \
	    "local site must be named before calling repmgr_start"] 1
	error_check_good client_close [$clientenv close] 0

	puts "\tRepmgr$tnum.g: Start repmgr with two local sites (error)."
	set clientenv [eval $cl_envcmd]
	catch {$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 8]] \
	    -local [list 127.0.0.1 [lindex $ports 9]] \
	    -start client} res
	error_check_good errchk [string match "*already*set*" $res] 1
	error_check_good client_close [$clientenv close] 0

	puts "\tRepmgr$tnum.h: Start a client."
	repladd 2
	set clientenv [eval $cl_envcmd -recover]
	$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.i: Start repmgr a second time (error)."
	catch {$clientenv repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 1]] \
	    -remote [list 127.0.0.1 [lindex $ports 0]] \
	    -start client} res
	error_check_good errchk [is_substr $res "repmgr is already started"] 1

	puts "\tRepmgr$tnum.j: Call rep_start after starting repmgr (error)."
	catch {$clientenv rep_start -client} res
	error_check_good errchk [is_substr $res \
	    "cannot call from Replication Manager application"] 1

	puts "\tRepmgr$tnum.k: Call rep_process_message (error)."
	set envlist "{$masterenv 1} {$clientenv 2}"
	catch {$clientenv rep_process_message 0 0 0} res
	error_check_good errchk [is_substr $res \
	    "cannot call from Replication Manager application"] 1

	#
	# Use of -ack all guarantees replication complete before repmgr send
	# function returns and rep_test finishes.
	#
	puts "\tRepmgr$tnum.l: Run some transactions at master."
	eval rep_test $method $masterenv NULL $niter $niter 0 0 $largs

	puts "\tRepmgr$tnum.m: Call rep_elect (error)."
	catch {$clientenv rep_elect 2 2 2 5000000} res
	error_check_good errchk [is_substr $res \
	    "cannot call from Replication Manager application"] 1

	puts "\tRepmgr$tnum.n: Verifying client database contents."
	rep_verify $masterdir $masterenv $clientdir $clientenv 1 1 1

	error_check_good client_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0

	puts "\tRepmgr$tnum.o: Start a master with base API rep_start."
	set ma_envcmd2 "berkdb_env_noerr -create $verbargs \
	    -home $masterdir2 -errpfx MASTER -txn -thread -rep_master \
	    -rep_transport \[list 1 replsend\]"
	set masterenv2 [eval $ma_envcmd2]

	puts "\tRepmgr$tnum.p: Call repmgr after rep_start (error)."
	catch {$masterenv2 repmgr -ack all \
	    -local [list 127.0.0.1 [lindex $ports 0]] \
	    -start master} res
	# Internal repmgr calls return EINVAL after hitting
	# base API application test.
	error_check_good errchk [is_substr $res "invalid argument"] 1

	error_check_good masterenv_close [$masterenv2 close] 0

	puts "\tRepmgr$tnum.q: Start an env without starting rep or repmgr."
	set norep_envcmd "berkdb_env_noerr -create $verbargs \
	    -home $norepdir -errpfx NOREP -txn -thread \
	    -rep_transport \[list 1 replsend\]"
	set norepenv [eval $norep_envcmd]

	puts "\tRepmgr$tnum.r: Call rep_elect before rep_start (error)."
	catch {$norepenv rep_elect 2 2 2 5000000} res
	# Internal rep_elect call returns EINVAL if rep_start has not 
	# been called first.
	error_check_good errchk [is_substr $res "invalid argument"] 1

	error_check_good norepenv_close [$norepenv close] 0
	replclose $testdir/MSGQUEUEDIR
}
