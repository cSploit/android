# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  rep108
# TEST
# TEST	Replication and partial rep database creation.
# TEST	Have a master, a client and a view.
# TEST	Start up master and client.  Create files and make sure
# TEST	the correct files appear on the view.  Force creation
# TEST	via internal init, recovery or by applying live log records.
#
proc rep108 { method { niter 500 } { tnum "108" } args } {
	source ./include.tcl
	global databases_in_memory
	global env_private
	global mixed_mode_logging
	global repfiles_in_memory

	# Valid for all access methods.
	if { $checking_valid_methods } {
		return "ALL"
	}

	set msg "using on-disk databases"
	#
	# Partial replication does not support in-memory databases.
	#
	if { $databases_in_memory } {
		puts -nonewline "Skipping rep$tnum for method "
		puts "$method for named in-memory databases."
		return
	}

	set msg2 "and on-disk replication files"
	if { $repfiles_in_memory } {
		set msg2 "and in-memory replication files"
	}

	set msg3 ""
	if { $env_private } {
		set msg3 "with private env"
	}

	set args [convert_args $method $args]
	set logsets [create_logsets 3]

	set views { txnodd none odd full }
	#
	# Run the body of the test.  We have a specific recovery
	# case to test, so this test does not use test_recopts.
	# We cannot use/copy logs if they're in memory so skip the
	# 'recovery' piece if using in-memory logs.
	#
	if { $mixed_mode_logging == 0 } {
		# All logs on-disk
		set create { recovery live init }
	} else {
		set create { live init }
	}
	foreach c $create {
		foreach v $views {
			foreach l $logsets {
				puts \
		"Rep$tnum ($method $c view($v)):\
		Replication, views and database creation $msg $msg2 $msg3."
				puts \
		"Rep$tnum: Master logs are [lindex $l 0]"
				puts \
		"Rep$tnum: Client logs are [lindex $l 1]"
				puts \
		"Rep$tnum: View logs are [lindex $l 2]"
				rep108_sub $method $niter $tnum \
				    $l $v $c $args
			}
		}
	}
}

proc rep108_sub { method niter tnum logset view create largs } {
	source ./include.tcl
	global env_private
	global rep_verbose
	global repfiles_in_memory
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on} "
	}

	set repmemargs ""
	if { $repfiles_in_memory == 1 } {
		set repmemargs " -rep_inmem_files "
	}

	set privargs ""
	if { $env_private } {
		set privargs " -private "
	}

	env_cleanup $testdir

	set qdir $testdir/MSGQUEUEDIR
	replsetup $qdir

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR
	set viewdir $testdir/VIEWDIR

	file mkdir $masterdir
	file mkdir $clientdir
	file mkdir $viewdir

	#
	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set maxpg 16384
	set log_max [expr $maxpg * 8]

	set m_logtype [lindex $logset 0]
	set c_logtype [lindex $logset 1]
	set v_logtype [lindex $logset 2]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set c_logargs [adjust_logargs $c_logtype]
	set v_logargs [adjust_logargs $v_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set c_txnargs [adjust_txnargs $c_logtype]
	set v_txnargs [adjust_txnargs $v_logtype]

	# Open a master.
	repladd 2
	set ma_envcmd "berkdb_env -create $m_txnargs $m_logargs \
	    $verbargs -errpfx MASTER -home $masterdir \
	    $repmemargs $privargs -log_max $log_max \
	    -rep_master -rep_transport \[list 2 replsend\]"
	set masterenv [eval $ma_envcmd]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	# Open two clients, one of which is a view.
	repladd 3
	set cl_envcmd "berkdb_env_noerr -create $c_txnargs $c_logargs \
	    $verbargs -errpfx CLIENT -home $clientdir -log_max $log_max \
	    $repmemargs $privargs -rep_client \
	    -rep_transport \[list 3 replsend\]"
	set clientenv [eval $cl_envcmd]
	error_check_good client_env [is_valid_env $clientenv] TRUE

	#
	# Make this client the view site.
	# Set the view callback.  If the view type is txnodd, we want
	# to use the 'odd' callback but use one, single txn to create
	# all the databases rather than autocommit.
	#
	set autocommit 1
	if { $view == "txnodd" } {
		set autocommit 0
		set view "odd"
	}
	switch $view {
		"full" { set viewcb "" }
		"none" { set viewcb replview_none }
		"odd" { set viewcb replview_odd }
	}
	repladd 4
	#
	# Set up the view env.  Depending on our creation test, we may not
	# start it until later on.
	#
	set v_envcmd "berkdb_env_noerr -create $v_txnargs $v_logargs \
	    $verbargs -errpfx VIEW -home $viewdir -log_max $log_max \
	    $repmemargs $privargs -rep_client \
	    -rep_view \[list $viewcb \] -rep_transport \[list 4 replsend\]"

	# Bring the clients online by processing the startup messages.
	if { $create == "live" } {
		# Open the view env now for live record processing.
		set viewenv [eval $v_envcmd]
		error_check_good view_env [is_valid_env $viewenv] TRUE
		set envlist "{$masterenv 2} {$clientenv 3} {$viewenv 4}"
		set verify_letter "b"
	} else {
		set envlist "{$masterenv 2} {$clientenv 3}"
		set verify_letter "d"
	}
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer.
	$masterenv test force noarchive_timeout

	# 
	# Run rep_test several times, each time through, using
	# a different database name.  Also create an in-memory
	# database in the same env.
	#
	set nfiles 5
	puts "\tRep$tnum.a: Running rep_test $nfiles times in replicated env."
	set omethod [convert_method $method]
	if { $autocommit == 0 } {
		set t [$masterenv txn]
		set dbtxn "-txn $t"
	} else {
		set dbtxn "-auto_commit"
	}
	#
	# Create the files separately from writing the data to them so
	# that we can test both auto-commit and creating many databases
	# inside a single txn.
	#
	# Skip the in-memory database for queueext since extents are
	# on-disk.
	if { [is_queueext $method] == 0 } {
		set testfile { "" "inmem0.db" }
		puts "\t\tRep$tnum: Creating in-memory database with $dbtxn."
		set db [eval {berkdb_open_noerr} -env $masterenv $dbtxn \
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
		error_check_good dbclose [$db close] 0
	}
	for { set i 0 } { $i < $nfiles } { incr i } {
		set testfile "test.$i.db"
		puts "\t\tRep$tnum.$i: Creating $testfile with $dbtxn."
		set db [eval {berkdb_open_noerr} -env $masterenv $dbtxn \
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
		error_check_good dbclose [$db close] 0
	}
	if { $autocommit == 0 } {
		$t commit
	}
	process_msgs $envlist

	#
	# For this part of the test open the db with auto_commit because
	# it is not the creation of the file.  Then do our updates.
	#
	for { set i 0 } { $i < $nfiles } { incr i } {
		set mult [expr $i * 10]
		set nentries [expr $niter + $mult]
		set testfile "test.$i.db"
		puts "\t\tRep$tnum.a.$i: Running rep_test for $testfile."
		set db [eval {berkdb_open_noerr} -env $masterenv \
		    -auto_commit $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
		eval rep_test $method $masterenv $db $nentries $mult $mult \
		    0 $largs
		error_check_good dbclose [$db close] 0
		process_msgs $envlist
	}

	#
	# Internal init requires that we create a gap, and open the
	# view environment.   Otherwise, for live record processing
	# there is nothing else to do.
	#
	if { $create == "init" } {
		#
		# Force a gap on the client too.
		#
		set flags ""
		set cid 3
		#
		set testfile "test.db"
		set db [eval {berkdb_open_noerr -env $masterenv -auto_commit \
		    -create -mode 0644} $largs $omethod $testfile]
		error_check_good db [is_valid_db $db] TRUE
		set start 0
		eval rep_test $method $masterenv $db $niter $start $start 0 $largs
		incr start $niter
		process_msgs $envlist
	
		puts "\tRep$tnum.b:  Close client.  Force master ahead."
		set start [push_master_ahead $method $masterenv $masterdir $m_logtype \
		    $clientenv $cid $db $start $niter $flags $largs]
		$db close
		$masterenv log_archive -arch_remove
	
		#
		# Now reopen the client and open the view site.
		#
		replclear 3
		replclear 4
		puts "\tRep$tnum.c:  Reopen client.  Open view."
		set clientenv [eval $cl_envcmd]
		error_check_good client_env [is_valid_env $clientenv] TRUE
		set viewenv [eval $v_envcmd]
		error_check_good view_env [is_valid_env $viewenv] TRUE
		set envlist "{$masterenv 2} {$clientenv 3} {$viewenv 4}"
		process_msgs $envlist
	}

	#
	# For the recovery case, we want to take a copy of all the logs
	# that are on the other clientenv (which should be all logs)
	# to the view environment directory.  We then open the view
	# environment with catastrophic recovery to have it recover over
	# the entire log set and create, or not, all the databases.
	#
	# Since this is an entirely empty env we must use catastrophic
	# recovery to force it to start at the beginning of the logs.
	#
	if { $create == "recovery" } {
		$clientenv log_flush
		puts "\tRep$tnum.b:  Copy logs from client dir to view dir."
		set logs [glob $clientdir/log*]
		foreach log $logs {
			set l [file tail $log]
			file copy -force $clientdir/$l $viewdir/$l
		}
		# Now open the view with recovery.
		puts "\tRep$tnum.c:  Open view with recovery."
		set viewenv [eval $v_envcmd -recover_fatal]
		error_check_good view_env [is_valid_env $viewenv] TRUE
		set envlist "{$masterenv 2} {$clientenv 3} {$viewenv 4}"
	}
	
	puts "\tRep$tnum.$verify_letter.0: Create private dbs on view."
	#
	# Create two non-durable (private) databases in the view env.
	# Name them with both an odd and even number.  The callback
	# should never be called in this situation and all private
	# databases should exist on the view.
	#
	set vpriv1 "private1.db"
	set vpriv2 "private2.db"
	set v1db [eval berkdb_open -create -auto_commit -btree \
	    -env $viewenv -notdurable $vpriv1]
	set v2db [eval berkdb_open -create -auto_commit -btree \
	    -env $viewenv -notdurable $vpriv2]
	eval rep_test btree $viewenv $v1db $nentries 0 10 0 $largs
	eval rep_test btree $viewenv $v2db $nentries 0 10 0 $largs
	$v1db close
	$v2db close
	process_msgs $envlist

	puts "\tRep$tnum.$verify_letter.1: Run rep_test for $nfiles dbs again."
	# Use different items than last rep_test loop.
	set start [expr $nentries + $mult]
	for { set i 0 } { $i < $nfiles } { incr i } {
		set mult [expr $i * 10]
		set nentries [expr $niter + $mult]
		set testfile "test.$i.db"
		puts "\t\tRep$tnum.$verify_letter.$i: Running rep_test for $testfile."
		set db [eval {berkdb_open_noerr} -env $masterenv -auto_commit\
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE

		eval rep_test $method $masterenv $db $nentries $start $mult \
		    0 $largs
		incr start $nentries
		error_check_good dbclose [$db close] 0
		process_msgs $envlist
	}

	#
	# On the recovery test, archive the view's log files and
	# run catastrophic recovery again before verifying the view.
	#
	if { $create == "recovery" } {
		puts \
"\tRep$tnum.$verify_letter:  Archive view logs and run catastrophic recovery."
		$viewenv log_flush
		$viewenv log_archive -arch_remove
		$viewenv close
		# Now re-open the view with catastrophic recovery.
		set viewenv [eval $v_envcmd -recover_fatal]
		error_check_good view_env [is_valid_env $viewenv] TRUE
		set envlist "{$masterenv 2} {$clientenv 3} {$viewenv 4}"
		process_msgs $envlist
	}

	#
	# Verify the right files are replicated.
	#
	puts "\tRep$tnum.$verify_letter.2:  Verify logs and databases."

	# On the client everything should be there.  First compare just the
	# logs and no databases.
	#
	rep_verify $masterdir $masterenv $clientdir $clientenv\
	    1 1 1 NULL
	for { set i 0 } { $i < $nfiles } { incr i } {
		set dbname "test.$i.db"
		rep_verify $masterdir $masterenv $clientdir $clientenv \
		    1 1 0 $dbname
	}

	#
	# On the view everything should be there in the "full" case.
	# In all cases, the 2 non-durable databases should exist.
	# No test databases should exist in the "none" case.
	# Only databases with odd digits should be there for the "odd" case.
	# No matter what the logs should be there and match.
	#
	puts "\t\tRep$tnum: $viewdir ($vpriv1 and $vpriv2) should exist"
	error_check_good priv1 [file exists $viewdir/$vpriv1] 1
	error_check_good priv2 [file exists $viewdir/$vpriv2] 1

	if { [is_queueext $method] == 0 } {
		puts "\t\tRep$tnum: $viewdir in-memory database should exist."
		set testfile { "" "inmem0.db" }
		set db [eval {berkdb_open_noerr} -env $viewenv -auto_commit $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
		error_check_good dbclose [$db close] 0
	}

	#
	# We let rep_verify compare the logs, then compare the dbs
	# if appropriate.  Send in NULL to just compare logs.
	#
	rep_verify $masterdir $masterenv $viewdir $viewenv \
	    1 1 1 NULL
	#
	# Verify correct setting of all the databases.  Only the
	# init case has test.db from the gap creation part.
	#
	if { $create == "init" } {
		set dbname "test.db"
		if { $view == "full" } {
			rep_verify $masterdir $masterenv $viewdir $viewenv \
			    1 1 0 $dbname
		} else {
			# test.db should not be there for both "none" and "odd".
			puts "\t\tRep$tnum: $viewdir ($dbname) should not exist"
			error_check_good test.db [file exists $viewdir/$dbname] 0
		}
	}
	for { set i 0 } { $i < $nfiles } { incr i } {
		set dbname "test.$i.db"
		if { $view == "full" } {
			rep_verify $masterdir $masterenv $viewdir $viewenv \
			    1 1 0 $dbname
		} elseif { $view == "none" } {
			puts "\t\tRep$tnum: $viewdir ($dbname) should not exist"
			error_check_good db$i [file exists $viewdir/$dbname] 0
		} else {
			# odd digit case
			set replicated [string match "*\[13579\]*" $dbname]
			if { $replicated } {
				rep_verify $masterdir $masterenv \
				    $viewdir $viewenv 1 1 0 $dbname
			} else {
				puts \
			"\t\tRep$tnum: $viewdir ($dbname) should not exist"
				error_check_good db$i \
				    [file exists $viewdir/$dbname] 0
			}
		}
	}

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good cenv_close [$clientenv close] 0
	error_check_good view_close [$viewenv close] 0

	replclose $testdir/MSGQUEUEDIR
}
