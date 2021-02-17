# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# TEST	rep089
# TEST	Test of proper clean-up of mpool during interrupted internal init.
# TEST
# TEST  Have a client in the middle of internal init when a new master
# TEST  generation comes along, forcing the client to interrupt the internal
# TEST  init, including doing the clean-up.  The client is in the middle of
# TEST  retrieving database pages, so that we are forced to clean up mpool.
# TEST  (Regression test for bug 17121)

proc rep089 { method { niter 200 } { tnum "089" } args } {
	source ./include.tcl
	global databases_in_memory
	global repfiles_in_memory
	global env_private

	if { $checking_valid_methods } {
		set test_methods {}
		foreach method $valid_methods {
			if { [is_btree $method] == 1 || \
			    [is_queue $method] == 1 } {
				lappend test_methods $method
			}
		}
		return $test_methods
	}
	if { [is_btree $method] == 0 && [is_queue $method] == 0 } {
		puts "Rep$tnum: skipping for non-btree, non-queue method."
		return
	}

	# It's possible to run this test with in-memory databases.
	set msg "with named databases"
	if { $databases_in_memory } {
		set msg "with in-memory named databases"
		if { [is_queueext $method] == 1 } {
			puts "Skipping rep$tnum for method $method"
			return
		}
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set msg3 ""
	if { $env_private } {
		set msg3 "with private env"
	}

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	set args [convert_args $method $args]

	puts -nonewline "Rep$tnum: Mpool cleanup on aborted"
	puts " internal init $msg $msg2 $msg3."
	rep089_sub $method $niter $tnum $args
}

proc rep089_sub { method niter tnum largs } {
	global testdir
	global rep_verbose
	global verbose_type
	global databases_in_memory
	global repfiles_in_memory
	global env_private

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory } {
		set repmemargs "-rep_inmem_files "
	}

	set privargs ""
	if { $env_private == 1 } {
		set privargs " -private "
	}

	env_cleanup $testdir
	replsetup $testdir/MSGQUEUEDIR

	file mkdir [set masterdir $testdir/MASTERDIR]
	file mkdir [set clientdir $testdir/CLIENTDIR]

	# Use a small page size, so that it won't take too long to accumulate a
	# large number of pages.
	# 
	append largs " -pagesize 512"

	puts "\tRep$tnum.a: Create a master and three databases."

	# Note that we use a special version of the "send" call-back function
	# here and in the client, during this first "set up" part of the test.
	# 
	repladd 1
	set env_cmd(M) "berkdb_env -create -txn $repmemargs $privargs \
	    $verbargs -home $masterdir -errpfx MASTER -rep_master"
	set masterenv [eval $env_cmd(M) -rep_transport {{1 rep089_send}}]

	# Create three databases.  Later, we will interrupt an internal init
	# during the middle of transferring pages for the second database.  This
	# will force the client to exercise clean-up for all three cases: a
	# fully intact, already materialized database; one that's in the process
	# of being materialized; and one that we haven't even touched yet (i.e.,
	# totally nonexistent).
	# 
	foreach n {1 2 3} {
		rep089_make_data $masterenv test$n.db $method $niter $largs
	}

	# Turn off transmission limits, so that we can be sure that we're
	# controlling the pace of progress through internal init from the test
	# script. 
	#
	$masterenv rep_limit 0 0

	puts "\tRep$tnum.b: Create a client, and start internal init."

	repladd 2
	set env_cmd(C) "berkdb_env -create -txn $repmemargs $privargs \
	    $verbargs -home $clientdir -errpfx CLIENT -rep_client \
	    -rep_transport \[list 2 rep089_send\]"
	set clientenv [eval $env_cmd(C)]

	set envlist "{$masterenv 1} {$clientenv 2}"

	# Go through just a single message processing cycle at a time, until we
	# see that we've received some PAGE messages.  Since we're using a
	# special send() function that discards some PAGE messages, the client
	# will then be left in the middle of REP_F_RECOVER_PAGE state.
	# 
	# We want to interrupt the internal init in the middle of materializing
	# the 2nd database.  Note that queue databases require two PAGE_REQ
	# cycles.
	# 
	if {[is_queue $method]} {
		set max_cycles 4
	} else {
		set max_cycles 2
	}

	# Do as many cycles as it takes till we see the first PAGE_REQ, then do
	# enough additional cycles to bring the total page-related cycles up to
	# "max_cycles".  Then turn on message dropping for the following cycle.
	# 
	global rep089_page_count
	global rep089_pagereq_seen

	set rep089_page_count -1
	set rep089_pagereq_seen 0
	while {! $rep089_pagereq_seen} {
		proc_msgs_once $envlist
	}
	for { set i 1 } { $i < $max_cycles } { incr i } {
		proc_msgs_once $envlist
	}
	set rep089_page_count 0
	proc_msgs_once $envlist
	error_check_bad assert_page_count $rep089_page_count 0

	puts "\tRep$tnum.c: Restart master, to force internal init clean-up."

	# Trigger an interrupted internal init clean-up, by having the master
	# restart at a higher gen.
	# 
	replclear 1
	$masterenv close
	set masterenv [eval $env_cmd(M) -rep_transport {{1 replsend}} -recover]
	set envlist "{$masterenv 1} {$clientenv 2}"
	process_msgs $envlist

	# (If we get to this point without a crash, the bug has been fixed.)

	puts "\tRep$tnum.d: Sanity-check the sync-interrupted stat."
	
	set n [stat_field $clientenv mpool_stat "Number of syncs interrupted"]
	error_check_good nsyncs $n 1

	$clientenv close
	$masterenv close 
	replclose $testdir/MSGQUEUEDIR
}

proc rep089_send { control rec fromid toid flags lsn } {
	global rep089_page_count
	global rep089_pagereq_seen

	if {[berkdb msgtype $control] eq "page_req"} {
		set rep089_pagereq_seen 1
	} elseif {[berkdb msgtype $control] eq "page" && $rep089_page_count >= 0 } {
		incr rep089_page_count
		if { $rep089_page_count < 3 || $rep089_page_count > 10 } {
			return 0
		}
	}
	return [replsend $control $rec $fromid $toid $flags $lsn]
}

proc rep089_make_data { env dbname method niter largs } {
	global databases_in_memory

	if { $databases_in_memory } {
		set dbname " {} $dbname "
	}

	set omethod [convert_method $method]
	set db [eval berkdb_open_noerr -env $env -auto_commit\
	    -create -mode 0644 $omethod $largs $dbname]

	# Make sure the database has at least 15 pages.  That number is
	# arbitrary, but it should be more than the 10 that we skip in
	# rep089_send, above.
	# 
	set page_target 15
	if {[is_queue $method]} {
		set descriptor "Number of pages"
	} else {
		set descriptor "Leaf pages"
	}
	set start 0
	while true {
		eval rep_test $omethod $env $db $niter $start 0 0 $largs
		incr start $niter
	
		set npages [stat_field $db stat $descriptor]
		if {$npages >= $page_target} {
			break
		}
	}

	$db close
}
