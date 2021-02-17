# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  rep113
# TEST
# TEST	Replication and partial view special case testing.
# TEST	Start up master and view.  Create files and make sure
# TEST	the correct files appear on the view.  Run special cases
# TEST	such as partitioned databases, secondaries and many data_dirs.
#
proc rep113 { method { niter 100 } { tnum "113" } args } {
	source ./include.tcl
	global databases_in_memory
	global env_private
	global repfiles_in_memory

	# Not all methods support partitioning.  Only use btree.
	if { $checking_valid_methods } {
		return "btree"
	}
	if { [is_btree $method] == 0 } {
		puts "\tRep$tnum: Skipping for method $method"
		return
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
	set logsets [create_logsets 2]

	set views { none odd full }
	set testcases { partition secondary many_ddir }
	foreach t $testcases {
		foreach v $views {
			foreach l $logsets {
				puts "Rep$tnum ($method $t $args view($v)):\
		Replication views and special cases $msg $msg2 $msg3."
				puts "Rep$tnum: Master logs are [lindex $l 0]"
				puts "Rep$tnum: View logs are [lindex $l 1]"
				rep113_sub $method $niter $tnum $t $l $v $args
			}
		}
	}
}

proc rep113_sub { method niter tnum testcase logset view largs } {
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
	# This test always uses data dirs.  Just create one unless we
	# are specifically testing many data dirs.  When we have many
	# data dirs, use 5 so that we have adequate odd/even testing
	# for the "odd" view callback case.
	#
	set data_diropts {}
	set nfiles 5
	if { $testcase == "many_ddir" } {
		set create_dirs {}
		for { set i 0 } { $i < $nfiles } { incr i } {
			lappend create_dirs data$i
		}
	} else {
		set create_dirs {data0}
	}
	foreach d $create_dirs {
		file mkdir $masterdir/$d
		file mkdir $viewdir/$d
		lappend data_diropts -data_dir
		lappend data_diropts $d
	}

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
	    $verbargs -errpfx MASTER -home $masterdir \
	    $repmemargs $privargs -log_max $log_max $data_diropts \
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
	    $repmemargs $privargs -rep_client $data_diropts \
	    -rep_view \[list $viewcb \] -rep_transport \[list 3 replsend\]"
	set viewenv [eval $v_envcmd]
	error_check_good view_env [is_valid_env $viewenv] TRUE
	set envlist "{$masterenv 2} {$viewenv 3}"
	process_msgs $envlist

	# 
	# Run rep_test several times, each time through, using
	# a different database name.
	#
	puts "\tRep$tnum.a: Running rep_test $nfiles times in replicated env."
	set omethod [convert_method $method]
	#
	# Create the files and write some data to them.
	#
	for { set i 0 } { $i < $nfiles } { incr i } {
		switch $testcase {
			"many_ddir" {
				set ddir [lindex $create_dirs $i]
				set spec_arg ""
				}
			"partition" {
				set ddir [lindex $create_dirs 0]
				#
				# Set 2 partition files so it is easier
				# to check existence.
				#
				set spec_arg " -partition_callback 2 part "
			}
			"secondary" {
				set ddir [lindex $create_dirs 0]
				set spec_arg ""
			}
		}
		set sdb "NULL"
		set testfile "test.$ddir.$i.db"
		set db [eval {berkdb_open_noerr} -env $masterenv -auto_commit \
		    -create_dir $ddir $spec_arg \
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
		if { $testcase == "secondary" } {
			#
			# Open and associate a secondary.  Just use the
			# standard callbacks for secondary testing.  Use
			# case 0.  See siutils.tcl for info.
			#
			set sname "sec.$ddir.$i.db"
			set sdb [eval {berkdb_open_noerr} -env $masterenv \
			    -auto_commit -create_dir $ddir \
			    -create -mode 0644 $omethod $largs $sname]
			error_check_good secdb [is_valid_db $sdb] TRUE
			error_check_good associate [$db associate \
			    [callback_n 0] $sdb] 0
		}
		eval rep_test $method $masterenv $db $niter 0 0 0 $largs
		error_check_good dbclose [$db close] 0
		if { $sdb != "NULL" } {
			error_check_good dbclose [$sdb close] 0
		}
		process_msgs $envlist
	}

	#
	# Verify the right files are replicated.
	#
	# On the view everything should be there in the "full" case.
	# No test databases should exist in the "none" case.
	# Only databases with odd digits should be there for the "odd" case.
	# No matter what the logs should be there and match.
	# We let rep_verify compare the logs, then compare the dbs
	# if they are expected to exist.  Send in NULL to just compare logs.
	#
	puts "\tRep$tnum.d:  Verify logs and databases."
	rep_verify $masterdir $masterenv $viewdir $viewenv 1 1 1 NULL
	for { set i 0 } { $i < $nfiles } { incr i } {
		if { $testcase == "many_ddir" } {
			set ddir [lindex $create_dirs $i]
		} else {
			set ddir [lindex $create_dirs 0]
		}
		set dbname "test.$ddir.$i.db"
		set secname "sec.$ddir.$i.db"
		#
		# Just check the correct existence of the files for this
		# test.  The rep_verify proc does not take args and we
		# cannot otherwise open a partitioned database without the
		# callback.  We'll assume that if the right files are there,
		# then the right data is in them, as lots of other tests
		# confirm the content.
		#
		if { $view == "full" } {
			puts \
		"\t\tRep$tnum: $viewdir ($ddir/$dbname) should exist"
			error_check_good db$i \
			    [file exists $viewdir/$ddir/$dbname] 1
			if { $testcase == "secondary" } {
				puts \
		"\t\tRep$tnum: $viewdir ($ddir/$secname) should exist"
				error_check_good sdb$i \
				    [file exists $viewdir/$ddir/$secname] 1
			}
		} elseif { $view == "none" } {
			puts \
		"\t\tRep$tnum: $viewdir ($ddir/$dbname) should not exist"
			error_check_good db$i \
			    [file exists $viewdir/$ddir/$dbname] 0
			if { $testcase == "partition" } {
				set pname0 "__dbp.$dbname.000"
				set pname1 "__dbp.$dbname.001"
				error_check_good p0$i \
				    [file exists $viewdir/$pname0] 0
				error_check_good p1$i \
				    [file exists $viewdir/$pname1] 0
			}
			if { $testcase == "secondary" } {
				puts \
		"\t\tRep$tnum: $viewdir ($ddir/$dbname) should not exist"
				error_check_good sdb$i \
				    [file exists $viewdir/$ddir/$secname] 0
			}
		} else {
			# odd digit case
			set replicated [string match "*\[13579\]*" $dbname]
			if { $replicated } {
				puts \
		"\t\tRep$tnum: $viewdir ($ddir/$dbname) should exist"
				error_check_good db$i \
				    [file exists $viewdir/$ddir/$dbname] 1
				if { $testcase == "secondary" } {
					puts \
		"\t\tRep$tnum: $viewdir ($ddir/$secname) should exist"
					error_check_good sdb$i \
					    [file exists $viewdir/$ddir/$secname] 1
				}
			} else {
				puts \
		"\t\tRep$tnum: $viewdir ($ddir/$dbname) should not exist"
				error_check_good db$i \
				    [file exists $viewdir/$ddir/$dbname] 0
				if { $testcase == "partition" } {
					set pname0 "__dbp.$dbname.000"
					set pname1 "__dbp.$dbname.001"
					error_check_good p0$i \
					    [file exists $viewdir/$pname0] 0
					error_check_good p1$i \
					    [file exists $viewdir/$pname1] 0
				}
				if { $testcase == "secondary" } {
					puts \
		"\t\tRep$tnum: $viewdir ($ddir/$secname) should not exist"
					error_check_good sdb$i \
					    [file exists $viewdir/$ddir/$secname] 0
				}
			}
		}
	}

	error_check_good masterenv_close [$masterenv close] 0
	error_check_good view_close [$viewenv close] 0

	replclose $testdir/MSGQUEUEDIR
}
