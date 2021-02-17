# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  rep112
# TEST
# TEST	Replication and partial view remove and rename.
# TEST	Start up master and view.  Create files and make sure
# TEST	the correct files appear on the view.  
#
proc rep112 { method { niter 100 } { tnum "112" } args } {
	source ./include.tcl
	global databases_in_memory
	global env_private
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
	set saved_args $args
	set logsets [create_logsets 2]

	set views { none odd full }
	foreach v $views {
		foreach l $logsets {
			set envargs ""
			set args $saved_args
			puts "Rep$tnum ($method $envargs $args view($v)):\
	Replication views and rename/remove $msg $msg2 $msg3."
			puts "Rep$tnum: Master logs are [lindex $l 0]"
			puts "Rep$tnum: View logs are [lindex $l 1]"
			rep112_sub $method $niter $tnum $envargs $l $v $args
		}
	}
}

proc rep112_sub { method niter tnum envargs logset view largs } {
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
	set viewdir $testdir/VIEWDIR

	file mkdir $masterdir
	file mkdir $viewdir

	#
	# Log size is small so we quickly create more than one.
	# The documentation says that the log file must be at least
	# four times the size of the in-memory log buffer.
	set maxpg 16384
	set log_max [expr $maxpg * 8]

	set m_logtype [lindex $logset 0]
	set v_logtype [lindex $logset 1]

	# In-memory logs require a large log buffer, and cannot
	# be used with -txn nosync.
	set m_logargs [adjust_logargs $m_logtype]
	set v_logargs [adjust_logargs $v_logtype]
	set m_txnargs [adjust_txnargs $m_logtype]
	set v_txnargs [adjust_txnargs $v_logtype]

	# Open a master.
	repladd 2
	set ma_envcmd "berkdb_env -create $m_txnargs $m_logargs \
	    $verbargs -errpfx MASTER -home $masterdir $envargs \
	    $repmemargs $privargs -log_max $log_max \
	    -rep_master -rep_transport \[list 2 replsend\]"
	set masterenv [eval $ma_envcmd]
	error_check_good master_env [is_valid_env $masterenv] TRUE

	#
	# Set the view callback.  
	#
	switch $view {
		"full" { set viewcb "" }
		"none" { set viewcb replview_none }
		"odd" { set viewcb replview_odd }
	}
	repladd 3
	set v_envcmd "berkdb_env_noerr -create $v_txnargs $v_logargs \
	    $verbargs -errpfx VIEW -home $viewdir -log_max $log_max \
	    $repmemargs $privargs -rep_client $envargs \
	    -rep_view \[list $viewcb \] -rep_transport \[list 3 replsend\]"

	set viewenv [eval $v_envcmd]
	error_check_good view_env [is_valid_env $viewenv] TRUE
	set envlist "{$masterenv 2} {$viewenv 3}"
	process_msgs $envlist

	# Clobber replication's 30-second anti-archive timer.
	$masterenv test force noarchive_timeout

	# 
	# Run rep_test several times, each time through, using
	# a different database name.
	#
	set nfiles 5
	puts "\tRep$tnum.a: Running rep_test $nfiles times in replicated env."
	set omethod [convert_method $method]
	#
	# Create the files and write some data to them.
	#
	for { set i 0 } { $i < $nfiles } { incr i } {
		set testfile "test.$i.db"
		set db [eval {berkdb_open_noerr} -env $masterenv -auto_commit \
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
		set mult [expr $i * 10]
		set nentries [expr $niter + $mult]
		eval rep_test $method $masterenv $db $nentries $mult $mult \
		    0 $largs
		error_check_good dbclose [$db close] 0
		process_msgs $envlist
	}

	puts "\tRep$tnum.b: Remove databases and recreate with same name."
	for { set i 0 } { $i < $nfiles } { incr i } {
		set testfile "test.$i.db"
		error_check_good remove.$testfile \
		    [$masterenv dbremove -auto_commit $testfile] 0
		set db [eval {berkdb_open_noerr} -env $masterenv -auto_commit \
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
		set mult [expr $i * 10]
		set nentries [expr $niter + $mult]
		eval rep_test $method $masterenv $db $nentries $mult $mult \
		    0 $largs
		error_check_good dbclose [$db close] 0
		process_msgs $envlist
	}
	puts "\tRep$tnum.c: Rename databases and recreate with same name."
	for { set i 0 } { $i < $nfiles } { incr i } {
		set testfile "test.$i.db"
		#
		# Rename to a file with a new number (so that odd files
		# originally now have an even digit, etc).
		#
		set j [expr $i + 1]
		set new "rename.$j.db"
		error_check_good rename.$testfile \
		    [$masterenv dbrename -auto_commit $testfile $new] 0
		set db [eval {berkdb_open_noerr} -env $masterenv -auto_commit \
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
		set mult [expr $i * 10]
		set nentries [expr $niter + $mult]
		eval rep_test $method $masterenv $db $nentries $mult $mult \
		    0 $largs
		error_check_good dbclose [$db close] 0
		process_msgs $envlist
	}

	#
	# Verify the right files are replicated.
	#
	# On the view everything should be there in the "full" case.
	# No test databases should exist in the "none" case.
	# Only databases with odd digits should be there for the "odd" case.
	# For rename, if the original was "odd" then the renamed file
	# with an even digit should exist.
	# No matter what the logs should be there and match.
	# We let rep_verify compare the logs, then compare the dbs
	# if they are expected to exist.  Send in NULL to just compare logs.
	#
	puts "\tRep$tnum.d:  Verify logs and databases."
	rep_verify $masterdir $masterenv $viewdir $viewenv \
	    1 1 1 NULL
	for { set i 0 } { $i < $nfiles } { incr i } {
		set dbname "test.$i.db"
		set j [expr $i + 1]
		set new "rename.$j.db"
		if { $view == "full" } {
			# Both original and new names should exist.
			rep_verify $masterdir $masterenv $viewdir $viewenv \
			    1 1 0 $dbname
			rep_verify $masterdir $masterenv $viewdir $viewenv \
			    1 1 0 $new
		} elseif { $view == "none" } {
			# Neither original nor new names should exist.
			puts "\t\tRep$tnum: $viewdir ($dbname) should not exist"
			error_check_good db$i [file exists $viewdir/$dbname] 0
			puts "\t\tRep$tnum: $viewdir ($new) should not exist"
			error_check_good new$j [file exists $viewdir/$new] 0
		} else {
			# odd digit case
			# Original and corrresponding new names should both
			# exist or not in tandem.  If an odd-numbered
			# dbname exists, the even-numbered new name
			# should exist too.
			set replicated [string match "*\[13579\]*" $dbname]
			if { $replicated } {
				rep_verify $masterdir $masterenv \
				    $viewdir $viewenv 1 1 0 $dbname
				rep_verify $masterdir $masterenv \
				    $viewdir $viewenv 1 1 0 $new
			} else {
				puts \
			"\t\tRep$tnum: $viewdir ($dbname) should not exist"
				error_check_good db$i \
				    [file exists $viewdir/$dbname] 0
				puts \
			"\t\tRep$tnum: $viewdir ($new) should not exist"
				error_check_good new$j \
				    [file exists $viewdir/$new] 0
			}
		}
	}

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good view_close [$viewenv close] 0

	replclose $testdir/MSGQUEUEDIR
}
