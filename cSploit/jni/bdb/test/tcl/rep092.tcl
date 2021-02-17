# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	rep092
# TEST	Read-your-writes consistency.
# TEST	Test events in one thread (process) waking up another sleeping thread,
# TEST	before a timeout expires.
# 

proc rep092 { method { niter 20 } { tnum "092" } args } {
	source ./include.tcl
	global repfiles_in_memory

	# Skip for all methods except btree.
	if { $checking_valid_methods } {
		set test_methods { btree }
		return $test_methods
	}
	if { [is_btree $method] == 0 } {
		puts "Rep005: Skipping for method $method."
		return
	}

	puts "Rep$tnum: read-your-writes consistency, multi-thread wake-up"
	foreach pad_flag { no yes } {
		foreach txn_flag { no yes } {
			rep092a_sub $method $niter \
			    $tnum $pad_flag $txn_flag $args
			rep092b_sub $method $niter \
			    $tnum $pad_flag $txn_flag $args
		}
	}
}

proc rep092a_sub { method niter tnum pad in_txn largs } {
	source ./include.tcl
	global rep_verbose
	global testdir
	global verbose_type
	global repfiles_in_memory

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	puts "\tRep$tnum.a: Create master and client."
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn -errpfx MASTER \
	    $repmemargs \
	    $verbargs -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd -rep_master]

	repladd 2
	set cl_envcmd "berkdb_env_noerr -create -txn -errpfx CLIENT \
	    $repmemargs -errfile /dev/stderr \
	    $verbargs -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd -rep_client]

	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	puts "\tRep$tnum.b: Create and replicate a few warm-up txns."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	puts "\tRep$tnum.c: Write txn and get its commit token."
	if { $pad } {
		eval rep_test $method $masterenv \
		    NULL $niter $start $start 0 $largs
		incr start $niter
	}
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set db [eval berkdb_open_noerr -env $masterenv -auto_commit \
		-create $omethod $dbargs test.db ]
	set txn [$masterenv txn -token]
	$db put -txn $txn "key1" "data1"
	set token [$txn commit]
	$db close
	if { $pad } {
		eval rep_test $method $masterenv \
		    NULL $niter $start $start 0 $largs
		incr start $niter
	}

	# Don't process msgs just yet.  We want to test the behavior when the
	# client checks/waits for the transaction more quickly than the client
	# receives it.  In order to do that in a test, we simulate the
	# replication being rather slow, by pausing for a moment after starting
	# up the txn_applied thread (in a separate child Tcl process).
	# 
	set pause 5

	# Define an emergency upper limit on the sleeping time, so that in case
	# the code is broken the test won't hang forever.  The child process
	# should complete promptly, as soon as we apply the transaction.
	# 
	set limit 60
	set tolerance 1

	# Spawn a process to call txn_applied
	# 
	puts "\tRep$tnum.d: Spawn child process, and pause to let it get started."
	set timeout [expr $limit * 1000000]
	error_check_good binary_scan [binary scan $token H40 token_chars] 1
	set listing $testdir/repscript.log
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep092script.tcl $listing $clientdir \
	    $token_chars $timeout $in_txn $rep_verbose $verbose_type &]
	tclsleep $pause

	puts "\tRep$tnum.e: Apply the transaction at the client."
	process_msgs $envlist

	watch_procs $pid 1
	set fd [open $listing]
	puts "\tRep$tnum.f: Examine the sub-process results."
	set report [split [read $fd] "\n"]
	close $fd
	set result [lindex [lsearch -inline $report RESULT*] 1]
	error_check_good wait_result $result 0
	set duration [lindex [lsearch -inline $report DURATION*] 1]
	error_check_good no_timeout \
	    [expr $duration < $limit - $tolerance] 1

	# Add a third client.
	# 
	set clientdir2 $testdir/CLIENTDIR2
	file mkdir $clientdir2
	
	puts "\tRep$tnum.g: Add another client, and make it master."
	repladd 3
	set cl_envcmd2 "berkdb_env_noerr -create -txn -errpfx CLIENT2 \
	    $repmemargs \
	    $verbargs -home $clientdir2 -rep_transport \[list 3 replsend\]"
	set clientenv2 [eval $cl_envcmd2 -rep_client]

	lappend envlist "$clientenv2 3"
	process_msgs $envlist

	# Swap roles between master and client2.  First client will eventually
	# see a gen change, while waiting.
	# 
	$masterenv rep_start -client
	$clientenv2 rep_start -master
	if { $pad } {
		eval rep_test $method $clientenv2 \
		    NULL $niter $start $start 0 $largs
		incr start $niter
	}
	set db [eval berkdb_open_noerr -env $clientenv2 -auto_commit \
		-create $omethod $dbargs test.db ]
	set txn [$clientenv2 txn -token]
	$db put -txn $txn "key2" "data2"
	set token [$txn commit]
	$db close
	if { $pad } {
		eval rep_test $method $clientenv2 \
		    NULL $niter $start $start 0 $largs
		incr start $niter
	}

	puts "\tRep$tnum.h: Spawn another child process."
	error_check_good binary_scan [binary scan $token H40 token_chars] 1
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep092script.tcl $listing $clientdir \
	    $token_chars $timeout $in_txn $rep_verbose $verbose_type &]
	tclsleep 5

	puts "\tRep$tnum.i: Apply the transaction at the client."
	process_msgs $envlist

	watch_procs $pid 1
	set fd [open $listing]
	puts "\tRep$tnum.j: Examine the sub-process results."
	set report [split [read $fd] "\n"]
	close $fd
	set result [lindex [lsearch -inline $report RESULT*] 1]
	error_check_good wait_result $result 0
	set duration [lindex [lsearch -inline $report DURATION*] 1]
	error_check_good no_timeout2 \
	    [expr $duration < $limit - $tolerance] 1

	$clientenv close
	$masterenv close
	$clientenv2 close

	replclose $testdir/MSGQUEUEDIR
}

proc rep092b_sub { method niter tnum pad in_txn largs } {
	source ./include.tcl
	global rep_verbose
	global testdir
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	# This part of the test only makes sense with INMEM
	# 
	set repmemargs "-rep_inmem_files "

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	puts "\tRep$tnum.a: Create master and client."
	repladd 1
	set ma_envcmd "berkdb_env_noerr -create -txn -errpfx MASTER \
	    $repmemargs \
	    $verbargs -home $masterdir -rep_transport \[list 1 replsend\]"
	set masterenv [eval $ma_envcmd -rep_master]

	repladd 2
	set cl_envcmd "berkdb_env_noerr -create -txn -errpfx CLIENT \
	    $repmemargs -errfile /dev/stderr \
	    $verbargs -home $clientdir -rep_transport \[list 2 replsend\]"
	set clientenv [eval $cl_envcmd -rep_client]

	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	puts "\tRep$tnum.b: Create and replicate a few warm-up txns."
	set start 0
	eval rep_test $method $masterenv NULL $niter $start $start 0 $largs
	incr start $niter
	process_msgs $envlist

	puts "\tRep$tnum.x: Shut down client."
	$clientenv close

	puts "\tRep$tnum.c: Write txn and get its commit token."
	if { $pad } {
		eval rep_test $method $masterenv \
		    NULL $niter $start $start 0 $largs
		incr start $niter
	}
	set omethod [convert_method $method]
	set dbargs [convert_args $method $largs]
	set db [eval berkdb_open_noerr -env $masterenv -auto_commit \
		-create $omethod $dbargs test.db ]
	set txn [$masterenv txn -token]
	$db put -txn $txn "key1" "data1"
	set token [$txn commit]
	$db close
	if { $pad } {
		eval rep_test $method $masterenv \
		    NULL $niter $start $start 0 $largs
		incr start $niter
	}

	puts "\tRep$tnum.x: Restart client, get partway through sync."
	set clientenv [eval $cl_envcmd -rep_client -recover]
	set envlist "{$masterenv 1} {$clientenv 2}"
	
	# This will put the client into a state where it doesn't have the LSN
	# history database, at a time when it needs to read it.  Therefore, it
	# will wait for it to be materialized in the "abbreviated internal init"
	# cycle that is needed when in-memory databases are involved.
	#
	proc_msgs_once $envlist
	proc_msgs_once $envlist

	set pause 5
	set limit 60
	set tolerance 1

	# Spawn a process to call txn_applied
	# 
	puts "\tRep$tnum.d: Spawn child process, and pause to let it get started."
	set timeout [expr $limit * 1000000]
	error_check_good binary_scan [binary scan $token H40 token_chars] 1
	set listing $testdir/repscript.log
	set pid [exec $tclsh_path $test_path/wrap.tcl \
	    rep092script.tcl $listing $clientdir \
	    $token_chars $timeout $in_txn $rep_verbose $verbose_type &]
	tclsleep $pause

	puts "\tRep$tnum.e: Apply the transaction at the client."
	process_msgs $envlist

	watch_procs $pid 1
	set fd [open $listing]
	puts "\tRep$tnum.f: Examine the sub-process results."
	set report [split [read $fd] "\n"]
	close $fd
	set result [lindex [lsearch -inline $report RESULT*] 1]
	error_check_good wait_result $result 0
	set duration [lindex [lsearch -inline $report DURATION*] 1]
	error_check_good no_timeout \
	    [expr $duration < $limit - $tolerance] 1

	$clientenv close
	$masterenv close

	replclose $testdir/MSGQUEUEDIR
}
