# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id: rep104.tcl,v 1.31 2008/04/10 17:19:47 carol Exp $
#
# TEST	rep104
# TEST	Test of interrupted internal initialization changes.  The
# TEST	interruption is due to a changed master.
# TEST
# TEST	One master, two clients.
# TEST	Generate several log files. Remove old master log files.
# TEST	Restart client forcing an internal init.
# TEST	Interrupt the internal init.
# TEST	We create lots of databases and a small cache to reproduce an
# TEST	issue where interrupted init removed the files and then the later
# TEST	init tried to write dirty pages to the no-longer-existing file.
# TEST
# TEST	Run for btree and queue only because of the number of permutations.
# TEST
proc rep104 { method { ndbs 10 } { tnum "104" } args } {

	source ./include.tcl

	global repfiles_in_memory

	# This test needs to force database pages to disk specifically,
	# so it is not available for inmem mode.
	if { $repfiles_in_memory } { 
		puts "Rep$tnum: Skipping for in-memory replication files."
		return
	}

	# Run for btree and queue methods only.
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

	# Skip for mixed-mode logging -- this test has a very large
	# set of iterations already.
	global mixed_mode_logging
	if { $mixed_mode_logging > 0 } {
		puts "Rep$tnum: Skipping for mixed mode logging."
		return
	}

	# This test needs to set its own pagesize.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Rep$tnum: skipping for specific pagesizes"
		return
	}

	set args [convert_args $method $args]

	# Run the body of the test with and without recovery.
	set archopts { archive noarchive }
	foreach r $test_recopts {
		# Only one of the three sites in the replication group needs to
		# be tested with in-memory logs: the "client under test".
		#
		if { $r == "-recover" } {
			set cl_logopts { on-disk }
		} else {
			set cl_logopts { on-disk in-memory }
		}
		foreach a $archopts {
			foreach l $cl_logopts {
				puts "Rep$tnum ($method $r $a $l $args):\
            Test of interrupted init with full cache. $ndbs databases."
				rep104_sub $method $ndbs $tnum $r $a $l $args
			}
		}
	}
}

proc rep104_sub \
    { method ndbs tnum recargs archive cl_logopt largs } {
	global testdir
	global util_path
	global rep_verbose
	global verbose_type
	global env_private

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set privargs ""
	if { $env_private == 1 } {
		set privargs " -private "
	}

	set client_crash false
	set niter 1500

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	# This test has three replication sites: a master, a client whose
	# behavior is under test, and another client.  We'll call them
	# "A", "B" and "C".  At one point during the test, we 
	# switch roles between the master and the other client.
	#
	# The initial site/role assignments are as follows:
	#
	#     A = master
	#     B = client under test
	#     C = other client
	#
	# In the case where we do switch roles, the roles become:
	#
	#     A = down
	#     B = client under test (no change here)
	#     C = master
	#
	# Although the real names are A, B, and C, we'll use mnemonic names
	# whenever possible.  In particular, this means that we'll have to
	# re-jigger the mnemonic names after the role switch.

	file mkdir [set dirs(A) $testdir/SITE_A]
	file mkdir [set dirs(B) $testdir/SITE_B]
	file mkdir [set dirs(C) $testdir/SITE_C]

	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set pagesize 8192
	append largs " -pagesize $pagesize "
	set log_buf [expr $pagesize * 2]
	set log_max [expr $log_buf * 4]

	# Set up the three sites: A, B, and C will correspond to EID's
	# 1, 2, and 3 in the obvious way.  As we start out, site A is always the
	# master.
	#
	repladd 1
	set env_A_cmd "berkdb_env_noerr -create -txn nosync $verbargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_A \
	    -home $dirs(A) $privargs -rep_transport \[list 1 replsend\]"
	set envs(A) [eval $env_A_cmd $recargs -rep_master]

	# Set up the commands for site B, but we're not opening it yet.
	# We only use the c_*args for the client under test.
	set c_txnargs [adjust_txnargs $cl_logopt]
	set c_logargs [adjust_logargs $cl_logopt]
	if { $cl_logopt == "on-disk" } {
		# Override in this case, because we want to specify log_buffer.
		set c_logargs "-log_buffer $log_buf"
	}
	set env_B_cmd "berkdb_env_noerr -create $c_txnargs $verbargs \
	    $c_logargs -log_max $log_max -errpfx SITE_B \
	    -home $dirs(B) $privargs -rep_transport \[list 2 replsend\]"

	# Open 2nd client
	repladd 3
	set env_C_cmd "berkdb_env_noerr -create -txn nosync $verbargs \
	    -log_buffer $log_buf -log_max $log_max -errpfx SITE_C \
	    -home $dirs(C) $privargs -rep_transport \[list 3 replsend\]"
	set envs(C) [eval $env_C_cmd $recargs -rep_client]

	# Turn off throttling for this test.
	foreach site [array names envs] {
		$envs($site) rep_limit 0 0
	}

	# Bring one client online by processing the startup messages.
	set envlist "{$envs(A) 1} {$envs(C) 3}"
	process_msgs $envlist

	# Set up the (indirect) mnemonic role names for the first part of the
	# test.
	set master A
	set test_client B
	set other C

	# Clobber replication's 30-second anti-archive timer, which will have
	# been started by client sync-up internal init, so that we can do a
	# log_archive in a moment.
	#
	$envs($master) test force noarchive_timeout

	# Run rep_test in the master (and update client).
	puts "\tRep$tnum.a: Running rep_test in $ndbs databases."
	set omethod [convert_method $method]
	set start 0
	set save_name "test4.db"
	for { set i 1 } { $i <= $ndbs } { incr i } {
		set dbname "test$i.db"
		set db [eval {berkdb_open_noerr -env $envs($master) -create \
		    -auto_commit -mode 0644} $largs $omethod $dbname]
		#
		# Save the db handle for later, if the saved one.
		#
		if { [string compare $dbname $save_name] == 0 } {
			set save_db $db
		}
		eval rep_test $method $envs($master) $db $niter \
		    $start $start 0 $largs
		if { [string compare $dbname $save_name] != 0 } {
			$db close
		}
		incr start $niter
		#
		# Since we're putting so much data into so many databases
		# we need to reset on the wordlist.
		#
		if { $start > 8000 } {
			set start 0
		}
	}
	process_msgs $envlist

	puts "\tRep$tnum.b: Run rep_test to saved database, filling cache."
	set res [eval exec $util_path/db_archive -l -h $dirs($other)]
	set last_client_log [lindex [lsort $res] end]

	set stop 0
	set start 0
	#
	# Set a larger iteration so that we can fill more of the cache
	# with pages just for this one database (save_db from above).
	#
	set newiter [expr $niter * 3]
	while { $stop == 0 } {
		# Run rep_test in the master (don't update client).
		puts "\tRep$tnum.c: Running rep_test in replicated env."
		eval rep_test $method $envs($master) $save_db $newiter \
		    $start $start 0 $largs
		incr start $niter
		puts "\tRep$tnum.d: Run db_archive on master."
		set res [eval exec $util_path/db_archive -d -h $dirs($master)]
		set res [eval exec $util_path/db_archive -l -h $dirs($master)]
		if { [lsearch -exact $res $last_client_log] == -1 } {
			set stop 1
		}
	}

	set envlist "{$envs($master) 1} {$envs($other) 3}"
	process_msgs $envlist

	if { $archive == "archive" } {
		puts "\tRep$tnum.d: Run db_archive on other client."
		set res [eval exec $util_path/db_archive -l -h $dirs($other)]
		error_check_bad \
		    log.1.present [lsearch -exact $res log.0000000001] -1
		set res [eval exec $util_path/db_archive -d -h $dirs($other)]
		set res [eval exec $util_path/db_archive -l -h $dirs($other)]
		error_check_good \
		    log.1.gone [lsearch -exact $res log.0000000001] -1
	} else {
		puts "\tRep$tnum.d: Skipping db_archive on other client."
	}

	puts "\tRep$tnum.e: Open test_client."
	env_cleanup $dirs($test_client)

	# (The test client is always site B, EID 2.)
	#
	repladd 2
	set envs(B) [eval $env_B_cmd $recargs -rep_client]
	error_check_good client_env [is_valid_env $envs(B)] TRUE
	$envs(B) rep_limit 0 0

	set envlist "{$envs(A) 1} {$envs(B) 2} {$envs(C) 3}"
	proc_msgs_once $envlist

	#
	# We want to simulate a master getting new records while an
	# init is going on.
	#
	set entries 10
	eval rep_test $method $envs($master) $save_db $entries \
	    $niter 0 0 0 $largs
	#
	# We call proc_msgs_once to get partway through internal init:
	# 1.  Send master messages and client finds master.
	# 2.  Master replies and client does verify.
	# 3.  Master gives verify_fail and client does update_req.
	# 4.  Master send update info and client does page_req.
	# 5.  Master sends all pages for that page_req.
	# 6.  Repeat page_req/pages.
	#
	# We call proc_msgs_once until we have about half of the databases,
	# including the one that should fill the cache on test_client.
	#
	set nproced 0
	set half [expr $ndbs / 2]
	puts "\tRep$tnum.f: Get partially through initialization ($half dbs)."
	set stat [catch {glob $dirs(B)/test*.db} dbs]
	if { $stat == 1 } {
		set numdb 0
	} else {
		set numdb [llength $dbs]
	}
	set seendb 0
	while { $numdb < $half && !$seendb } {
		incr nproced [proc_msgs_once $envlist]
		set stat [catch {glob $dirs(B)/test*.db} dbs]
		if { $stat == 1 } {
			set numdb 0
		} else {
			set numdb [llength $dbs]
			foreach f $dbs {
				if { [string compare $save_name $f] == 0 } {
					set seendb 1
					break
				}
			}
		}
	}

	replclear 1
	replclear 3
	puts "\tRep$tnum.g: Abandon master.  Upgrade other client."
	set abandon 1
	set abandon_env A
	set master C
	set envlist "{$envs(B) 2} {$envs(C) 3}"

	#
	# Make sure site B can reset and successfully complete
	# the internal init.
	#
	error_check_good upgrade [$envs($master) rep_start -master] 0
	process_msgs $envlist

	puts "\tRep$tnum.h: Verify logs and databases"
	for { set i 1 } { $i <= $ndbs } { incr i } {
		set dbname "test$i.db"
		rep_verify $dirs(C) $envs(C) $dirs(B) $envs(B) 1 1 1 $dbname
	}

	# Process messages again in case we are running with debug_rop.
	process_msgs $envlist

	if { $abandon } {
		catch {$save_db close}
		error_check_good env_close [$envs($abandon_env) close] 0
	}
	error_check_good masterenv_close [$envs($master) close] 0
	error_check_good clientenv_close [$envs($test_client) close] 0
	replclose $testdir/MSGQUEUEDIR
}
