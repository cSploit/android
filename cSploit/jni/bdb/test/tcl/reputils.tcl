# See the file LICENSE for redistribution information.
#
# Copyright (c) 2001, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# Replication testing utilities

# Environment handle for the env containing the replication "communications
# structure" (really a CDB environment).

# The test environment consists of a queue and a # directory (environment)
# per replication site.  The queue is used to hold messages destined for a
# particular site and the directory will contain the environment for the
# site.  So the environment looks like:
#				$testdir
#			 ___________|______________________________
#			/           |              \               \
#		MSGQUEUEDIR	MASTERDIR	CLIENTDIR.0 ...	CLIENTDIR.N-1
#		| | ... |
#		1 2 .. N+1
#
# The master is site 1 in the MSGQUEUEDIR and clients 1-N map to message
# queues 2 - N+1.
#
# The globals repenv(1-N) contain the environment handles for the sites
# with a given id (i.e., repenv(1) is the master's environment.


# queuedbs is an array of DB handles, one per machine ID/machine ID pair,
# for the databases that contain messages from one machine to another.
# We omit the cases where the "from" and "to" machines are the same.
#
global queuedbs
global machids
global perm_response_list
set perm_response_list {}
global perm_sent_list
set perm_sent_list {}
global electable_pri
set electable_pri 5
set drop 0
global anywhere
set anywhere 0

global rep_verbose
set rep_verbose 0
global verbose_type
set verbose_type "rep"

# To run a replication test with verbose messages, type
# 'run_verbose' and then the usual test command string enclosed
# in double quotes or curly braces.  For example:
# 
# run_verbose "rep001 btree"
# 
# run_verbose {run_repmethod btree test001}
#
# To run a replication test with one of the subsets of verbose 
# messages, use the same syntax with 'run_verbose_elect', 
# 'run_verbose_lease', etc. 

proc run_verbose { commandstring } {
	global verbose_type
	set verbose_type "rep"
	run_verb $commandstring
}

proc run_verbose_elect { commandstring } {
	global verbose_type
	set verbose_type "rep_elect"
	run_verb $commandstring
}

proc run_verbose_lease { commandstring } {
	global verbose_type
	set verbose_type "rep_lease"
	run_verb $commandstring
}

proc run_verbose_misc { commandstring } {
	global verbose_type
	set verbose_type "rep_misc"
	run_verb $commandstring
}

proc run_verbose_msgs { commandstring } {
	global verbose_type
	set verbose_type "rep_msgs"
	run_verb $commandstring
}

proc run_verbose_sync { commandstring } {
	global verbose_type
	set verbose_type "rep_sync"
	run_verb $commandstring
}

proc run_verbose_test { commandstring } {
	global verbose_type
	set verbose_type "rep_test"
	run_verb $commandstring
}

proc run_verbose_repmgr_misc { commandstring } {
	global verbose_type
	set verbose_type "repmgr_misc"
	run_verb $commandstring
}

proc run_verb { commandstring } {
	global rep_verbose
	global verbose_type

	set rep_verbose 1
	if { [catch {
		eval $commandstring
		flush stdout
		flush stderr
	} res] != 0 } {
		global errorInfo

		set rep_verbose 0
		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_verbose: $commandstring: $theError"
		} else {
			error $theError;
		}
	}
	set rep_verbose 0
}

# Databases are on-disk by default for replication testing. 
# Some replication tests have been converted to run with databases
# in memory instead. 

global databases_in_memory
set databases_in_memory 0

proc run_inmem_db { test method } {
	run_inmem $test $method 1 0 0 0
}

# Replication files are on-disk by default for replication testing. 
# Some replication tests have been converted to run with rep files
# in memory instead. 

global repfiles_in_memory
set repfiles_in_memory 0

proc run_inmem_rep { test method } {
	run_inmem $test $method 0 0 1 0
}

# Region files are on-disk by default for replication testing. 
# Replication tests can force the region files in-memory by setting 
# the -private flag when opening an env.

global env_private
set env_private 0

proc run_env_private { test method } {
	global test_names

	if { [is_substr $test_names(skip_for_env_private) $test] == 1 } {
		puts "Test $test is not set up to use private envs."
		return 
	} else {
		run_inmem $test $method 0 0 0 1
	}
}

# Logs are on-disk by default for replication testing. 
# Mixed-mode log testing provides a mixture of on-disk and
# in-memory logging, or even all in-memory.  When testing on a
# 1-master/1-client test, we try all four options.  On a test
# with more clients, we still try four options, randomly
# selecting whether the later clients are on-disk or in-memory.
#

global mixed_mode_logging
set mixed_mode_logging 0

proc create_logsets { nsites } {
	global mixed_mode_logging
	global logsets
	global rand_init

	error_check_good set_random_seed [berkdb srand $rand_init] 0
	if { $mixed_mode_logging == 0 || $mixed_mode_logging == 2 } {
		if { $mixed_mode_logging == 0 } {
			set logmode "on-disk"
		} else {
			set logmode "in-memory"
		}
		set loglist {}
		for { set i 0 } { $i < $nsites } { incr i } {
			lappend loglist $logmode
		}
		set logsets [list $loglist]
	}
	if { $mixed_mode_logging == 1 } {
		set set1 {on-disk on-disk}
		set set2 {on-disk in-memory}
		set set3 {in-memory on-disk}
		set set4 {in-memory in-memory}

		# Start with nsites at 2 since we already set up
		# the master and first client.
		for { set i 2 } { $i < $nsites } { incr i } {
			foreach set { set1 set2 set3 set4 } {
				if { [berkdb random_int 0 1] == 0 } {
					lappend $set "on-disk"
				} else {
					lappend $set "in-memory"
				}
			}
		}
		set logsets [list $set1 $set2 $set3 $set4]
	}
	return $logsets
}

proc run_inmem_log { test method } {
	run_inmem $test $method 0 1 0 0
}

# Run_mixedmode_log is a little different from the other run_inmem procs:
# it provides a mixture of in-memory and on-disk logging on the different
# hosts in a replication group.
proc run_mixedmode_log { test method {display 0} {run 1} \
    {outfile stdout} {largs ""} } {
	global mixed_mode_logging
	set mixed_mode_logging 1

	set prefix [string range $test 0 2]
	if { $prefix != "rep" } {
		puts "Skipping mixed-mode log testing for non-rep test."
		set mixed_mode_logging 0
		return
	}

	eval run_method $method $test $display $run $outfile $largs

	# Reset to default values after run.
	set mixed_mode_logging 0
}

# The procs run_inmem_db, run_inmem_log, run_inmem_rep, and run_env_private 
# put databases, logs, rep files, or region files in-memory.  (Setting up 
# an env with the -private flag puts region files in memory.)
# The proc run_inmem allows you to put any or all of these in-memory 
# at the same time. 

proc run_inmem { test method\
     {dbinmem 1} {logsinmem 1} {repinmem 1} {envprivate 1} } {

	set prefix [string range $test 0 2]
	if { $prefix != "rep" } {
		puts "Skipping in-memory testing for non-rep test."
		return
	}
	global databases_in_memory 
	global mixed_mode_logging
	global repfiles_in_memory
	global env_private
	global test_names

	if { $dbinmem } {
		if { [is_substr $test_names(skip_for_inmem_db) $test] == 1 } {
                	puts "Test $test does not support in-memory databases."
			puts "Putting databases on-disk."
                	set databases_in_memory 0
		} else {
			set databases_in_memory 1
		}
	}
	if { $logsinmem } {
		set mixed_mode_logging 2
	}
	if { $repinmem } {
		set repfiles_in_memory 1
	}
	if { $envprivate } { 
		set env_private 1
	}

	if { [catch {eval run_method $method $test} res]  } {
		set databases_in_memory 0
		set mixed_mode_logging 0 
		set repfiles_in_memory 0 
		set env_private 0
		puts "FAIL: $res"
	}

	set databases_in_memory 0 
	set mixed_mode_logging 0 
	set repfiles_in_memory 0
	set env_private 0
}

# The proc run_diskless runs run_inmem with its default values.
# It's useful to have this name to remind us of its testing purpose, 
# which is to mimic a diskless host.

proc run_diskless { test method } {
	run_inmem $test $method 1 1 1 1
}

# Open the master and client environments; store these in the global repenv
# Return the master's environment: "-env masterenv"
proc repl_envsetup { envargs largs test {nclients 1} {droppct 0} { oob 0 } } {
	source ./include.tcl
	global clientdir
	global drop drop_msg
	global masterdir
	global repenv
	global rep_verbose
	global verbose_type

	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on}"
	}

	env_cleanup $testdir

	replsetup $testdir/MSGQUEUEDIR

	set masterdir $testdir/MASTERDIR
	file mkdir $masterdir
	if { $droppct != 0 } {
		set drop 1
		set drop_msg [expr 100 / $droppct]
	} else {
		set drop 0
	}

	for { set i 0 } { $i < $nclients } { incr i } {
		set clientdir($i) $testdir/CLIENTDIR.$i
		file mkdir $clientdir($i)
	}

	# Some tests that use a small db pagesize need a small 
	# mpool pagesize as well -- otherwise we'll run out of 
	# mutexes.   First determine the natural pagesize, so 
	# that can be used in the normal case, then adjust where
	# needed. 

	set env [berkdb_env -create -home $testdir]
	set pagesize [$env get_mp_pagesize]
	error_check_good env_close [$env close] 0
	berkdb envremove -home $testdir

	set small_pagesize_tests [list test035 test096 test112 test113 \
	    test114 test135 test136]
	if { [lsearch -exact $small_pagesize_tests $test] != -1  } {
		set pagesize 512
	}

	# Open a master.
	repladd 1
	#
	# Set log smaller than default to force changing files,
	# but big enough so that the tests that use binary files
	# as keys/data can run.  Increase the size of the log region --
	# sdb004 needs this, now that subdatabase names are stored 
	# in the env region.
	#
	set logmax [expr 3 * 1024 * 1024]
	set lockmax 40000
	set logregion 2097152

	set ma_cmd "berkdb_env_noerr -create -log_max $logmax $envargs \
	    -cachesize { 0 16777216 1 } -log_regionmax $logregion \
	    -lock_max_objects $lockmax -lock_max_locks $lockmax \
	    -errpfx $masterdir $verbargs -pagesize $pagesize \
	    -home $masterdir -txn nosync -rep_master -rep_transport \
	    \[list 1 replsend\]"
	set masterenv [eval $ma_cmd]
	error_check_good master_env [is_valid_env $masterenv] TRUE
	set repenv(master) $masterenv

	# Open clients
	for { set i 0 } { $i < $nclients } { incr i } {
		set envid [expr $i + 2]
		repladd $envid
                set cl_cmd "berkdb_env_noerr -create $envargs -txn nosync \
		    -cachesize { 0 16777216 0 } -log_regionmax $logregion \
		    -lock_max_objects $lockmax -lock_max_locks $lockmax \
		    -errpfx $clientdir($i) $verbargs -pagesize $pagesize \
		    -home $clientdir($i) -rep_client -rep_transport \
		    \[list $envid replsend\]"
                set clientenv [eval $cl_cmd]
		error_check_good client_env [is_valid_env $clientenv] TRUE
		set repenv($i) $clientenv
	}
	set repenv($i) NULL
	append largs " -env $masterenv "

	# Process startup messages
	repl_envprocq $test $nclients $oob

	# Clobber replication's 30-second anti-archive timer, which
	# will have been started by client sync-up internal init, in
	# case the test we're about to run wants to do any log
	# archiving, or database renaming and/or removal.
	$masterenv test force noarchive_timeout

	return $largs
}

# Process all incoming messages.  Iterate until there are no messages left
# in anyone's queue so that we capture all message exchanges. We verify that
# the requested number of clients matches the number of client environments
# we have.  The oob parameter indicates if we should process the queue
# with out-of-order delivery.  The replprocess procedure actually does
# the real work of processing the queue -- this routine simply iterates
# over the various queues and does the initial setup.
proc repl_envprocq { test { nclients 1 } { oob 0 }} {
	global repenv
	global drop

	set masterenv $repenv(master)
	for { set i 0 } { 1 } { incr i } {
		if { $repenv($i) == "NULL"} {
			break
		}
	}
	error_check_good i_nclients $nclients $i

	berkdb debug_check
	puts -nonewline "\t$test: Processing master/$i client queues"
	set rand_skip 0
	if { $oob } {
		puts " out-of-order"
	} else {
		puts " in order"
	}
	set droprestore $drop
	while { 1 } {
		set nproced 0

		if { $oob } {
			set rand_skip [berkdb random_int 2 10]
		}
		incr nproced [replprocessqueue $masterenv 1 $rand_skip]
		for { set i 0 } { $i < $nclients } { incr i } {
			set envid [expr $i + 2]
			if { $oob } {
				set rand_skip [berkdb random_int 2 10]
			}
			set n [replprocessqueue $repenv($i) \
			    $envid $rand_skip]
			incr nproced $n
		}

		if { $nproced == 0 } {
			# Now that we delay requesting records until
			# we've had a few records go by, we should always
			# see that the number of requests is lower than the
			# number of messages that were enqueued.
			for { set i 0 } { $i < $nclients } { incr i } {
				set clientenv $repenv($i)
				set queued [stat_field $clientenv rep_stat \
				   "Total log records queued"]
				error_check_bad queued_stats \
				    $queued -1
				set requested [stat_field $clientenv rep_stat \
				   "Log records requested"]
				error_check_bad requested_stats \
				    $requested -1

				#
				# Set to 100 usecs.  An average ping
				# to localhost should be a few 10s usecs.
				#
				$clientenv rep_request 100 400
			}

			# If we were dropping messages, we might need
			# to flush the log so that we get everything
			# and end up in the right state.
			if { $drop != 0 } {
				set drop 0
				$masterenv rep_flush
				berkdb debug_check
				puts "\t$test: Flushing Master"
			} else {
				break
			}
		}
	}

	# Reset the clients back to the default state in case we
	# have more processing to do.
	for { set i 0 } { $i < $nclients } { incr i } {
		set clientenv $repenv($i)
		$clientenv rep_request 40000 1280000
	}
	set drop $droprestore
}

# Verify that the directories in the master are exactly replicated in
# each of the client environments.
proc repl_envver0 { test method { nclients 1 } } {
	global masterdir
	global repenv

	# Verify the database in the client dir.
	# First dump the master.
	set t1 $masterdir/t1
	set t2 $masterdir/t2
	set t3 $masterdir/t3
	set omethod [convert_method $method]

	#
	# We are interested in the keys of whatever databases are present
	# in the master environment, so we just call a no-op check function
	# since we have no idea what the contents of this database really is.
	# We just need to walk the master and the clients and make sure they
	# have the same contents.
	#
	set cwd [pwd]
	cd $masterdir
	set stat [catch {glob test*.db} dbs]
	cd $cwd
	if { $stat == 1 } {
		return
	}
	foreach testfile $dbs {
		open_and_dump_file $testfile $repenv(master) $masterdir/t2 \
		    repl_noop dump_file_direction "-first" "-next"

		if { [string compare [convert_method $method] -recno] != 0 } {
			filesort $t2 $t3
			file rename -force $t3 $t2
		}
		for { set i 0 } { $i < $nclients } { incr i } {
	puts "\t$test: Verifying client $i database $testfile contents."
			open_and_dump_file $testfile $repenv($i) \
			    $t1 repl_noop dump_file_direction "-first" "-next"

			if { [string compare $omethod "-recno"] != 0 } {
				filesort $t1 $t3
			} else {
				catch {file copy -force $t1 $t3} ret
			}
			error_check_good diff_files($t2,$t3) [filecmp $t2 $t3] 0
		}
	}
}

# Remove all the elements from the master and verify that these
# deletions properly propagated to the clients.
proc repl_verdel { test method { nclients 1 } } {
	source ./include.tcl

	global clientdir
	global masterdir
	global repenv
	global encrypt
	global passwd
	global util_path

	# Delete all items in the master.
	set cwd [pwd]
	cd $masterdir
	set stat [catch {glob test*.db} dbs]
	cd $cwd
	if { $stat == 1 } {
		return
	}
	set utilflag ""
	if { $encrypt != 0 } {
		set utilflag "-P $passwd"
	}
	foreach testfile $dbs {

		# Dump the database to determine whether there are subdbs.
		#
		set ret [catch {eval {exec $util_path/db_dump} $utilflag\
		    -f $testdir/dumpfile $masterdir/$testfile} res]
		error_check_good dump($testfile:$res) $ret 0

		set subdbs ""
		set fd [open $testdir/dumpfile r]
		while { [gets $fd str] != -1 } {
			if { [string match database=* $str] } {
				set subdbname [string range $str 9 end]
				lappend subdbs $subdbname
			}
		}
		close $fd

		# Set up filenames depending on whether there are 
		# subdatabases or not.
		set files ""
		if { [llength $subdbs] > 0 } {
			foreach sub $subdbs {
				set filename "$testfile $sub" 
				lappend files $filename
			}
		} else {
			set files $testfile 
		}

		foreach f $files {
			puts "\t$test: Deleting all items from the master."
			set txn [$repenv(master) txn]
			error_check_good txn_begin [is_valid_txn $txn \
			    $repenv(master)] TRUE
			set db [eval {berkdb_open} -txn $txn -env $repenv(master) $f]
			error_check_good reopen_master [is_valid_db $db] TRUE
			set dbc [$db cursor -txn $txn]
			error_check_good reopen_master_cursor \
			    [is_valid_cursor $dbc $db] TRUE
			for { set dbt [$dbc get -first] } { [llength $dbt] > 0 } \
			    { set dbt [$dbc get -next] } {
			error_check_good del_item [$dbc del] 0
			}
			error_check_good dbc_close [$dbc close] 0
			error_check_good txn_commit [$txn commit] 0
			error_check_good db_close [$db close] 0
		}

		repl_envprocq $test $nclients

		# Check clients.
		for { set i 0 } { $i < $nclients } { incr i } {
			foreach f $files {
				puts "\t$test: Verifying client database $i is empty."

				set db [eval berkdb_open -env $repenv($i) $f]
				error_check_good reopen_client($i) \
				    [is_valid_db $db] TRUE
				set dbc [$db cursor]
				error_check_good reopen_client_cursor($i) \
				    [is_valid_cursor $dbc $db] TRUE

				error_check_good client($i)_empty \
				    [llength [$dbc get -first]] 0

				error_check_good dbc_close [$dbc close] 0
				error_check_good db_close [$db close] 0
			}
		}
	}
}

# Replication "check" function for the dump procs that expect to
# be able to verify the keys and data.
proc repl_noop { k d } {
	return
}

# Close all the master and client environments in a replication test directory.
proc repl_envclose { test envargs } {
	source ./include.tcl
	global clientdir
	global encrypt
	global masterdir
	global repenv
	global drop

	if { [lsearch $envargs "-encrypta*"] !=-1 } {
		set encrypt 1
	}

	# In order to make sure that we have fully-synced and ready-to-verify
	# databases on all the clients, do a checkpoint on the master and
	# process messages in order to flush all the clients.
	set drop 0
	berkdb debug_check
	puts "\t$test: Checkpointing master."
	error_check_good masterenv_ckp [$repenv(master) txn_checkpoint] 0

	# Count clients.
	for { set ncli 0 } { 1 } { incr ncli } {
		if { $repenv($ncli) == "NULL" } {
			break
		}
		$repenv($ncli) rep_request 100 100
	}
	repl_envprocq $test $ncli

	error_check_good masterenv_close [$repenv(master) close] 0
	verify_dir $masterdir "\t$test: " 0 0 1
	for { set i 0 } { $i < $ncli } { incr i } {
		error_check_good client($i)_close [$repenv($i) close] 0
		verify_dir $clientdir($i) "\t$test: " 0 0 1
	}
	replclose $testdir/MSGQUEUEDIR

}

# Replnoop is a dummy function to substitute for replsend
# when replication is off.
proc replnoop { control rec fromid toid flags lsn } {
	return 0
}

proc replclose { queuedir } {
	global queueenv queuedbs machids

	foreach m $machids {
		set db $queuedbs($m)
		error_check_good dbr_close [$db close] 0
	}
	error_check_good qenv_close [$queueenv close] 0
	set machids {}
}

# Create a replication group for testing.
proc replsetup { queuedir } {
	global queueenv queuedbs machids

	file mkdir $queuedir
	set max_locks 20000
	set queueenv [berkdb_env \
	     -create -txn nosync -lock_max_locks $max_locks -home $queuedir]
	error_check_good queueenv [is_valid_env $queueenv] TRUE

	if { [info exists queuedbs] } {
		unset queuedbs
	}
	set machids {}

	return $queueenv
}

# Send function for replication.
proc replsend { control rec fromid toid flags lsn } {
	global queuedbs queueenv machids
	global drop drop_msg
	global perm_sent_list
	global anywhere

	set permflags [lsearch $flags "perm"]
	if { [llength $perm_sent_list] != 0 && $permflags != -1 } {
#		puts "replsend sent perm message, LSN $lsn"
		lappend perm_sent_list $lsn
	}

	#
	# If we are testing with dropped messages, then we drop every
	# $drop_msg time.  If we do that just return 0 and don't do
	# anything.  However, avoid dropping PAGE_REQ and LOG_REQ, because
	# currently recovering from those cases can take a while, and some tests
	# rely on the assumption that a single log_flush from the master clears
	# up any missing messages.
	#
	if { $drop != 0 && 
	    !([berkdb msgtype $control] eq "page_req" ||
	    [berkdb msgtype $control] eq "log_req")} {
		incr drop
		if { $drop == $drop_msg } {
			set drop 1
			return 0
		}
	}
	# XXX
	# -1 is DB_BROADCAST_EID
	if { $toid == -1 } {
		set machlist $machids
	} else {
		if { [info exists queuedbs($toid)] != 1 } {
			error "replsend: machid $toid not found"
		}
		set m NULL
		if { $anywhere != 0 } {
			#
			# If we can send this anywhere, send it to the first
			# id we find that is neither toid or fromid.
			#
			set anyflags [lsearch $flags "any"]
			if { $anyflags != -1 } {
				foreach m $machids {
					if { $m == $fromid || $m == $toid } {
						continue
					}
					set machlist [list $m]
					break
				}
			}
		}
		#
		# If we didn't find a different site, then we must
		# fallback to the toid.
		#
		if { $m == "NULL" } {
			set machlist [list $toid]
		}
	}

	foreach m $machlist {
		# do not broadcast to self.
		if { $m == $fromid } {
			continue
		}

		set db $queuedbs($m)
		set txn [$queueenv txn]
		$db put -txn $txn -append [list $control $rec $fromid]
		error_check_good replsend_commit [$txn commit] 0
	}

	queue_logcheck
	return 0
}

#
# If the message queue log files are getting too numerous, checkpoint
# and archive them.  Some tests are so large (particularly from
# run_repmethod) that they can consume far too much disk space.
proc queue_logcheck { } {
	global queueenv


	set logs [$queueenv log_archive -arch_log]
	set numlogs [llength $logs]
	if { $numlogs > 10 } {
		$queueenv txn_checkpoint
		$queueenv log_archive -arch_remove
	}
}

# Discard all the pending messages for a particular site.
proc replclear { machid } {
	global queuedbs queueenv

	if { [info exists queuedbs($machid)] != 1 } {
		error "FAIL: replclear: machid $machid not found"
	}

	set db $queuedbs($machid)
	set txn [$queueenv txn]
	set dbc [$db cursor -txn $txn]
	for { set dbt [$dbc get -rmw -first] } { [llength $dbt] > 0 } \
	    { set dbt [$dbc get -rmw -next] } {
		error_check_good replclear($machid)_del [$dbc del] 0
	}
	error_check_good replclear($machid)_dbc_close [$dbc close] 0
	error_check_good replclear($machid)_txn_commit [$txn commit] 0
}

# Add a machine to a replication environment.
proc repladd { machid } {
	global queueenv queuedbs machids

	if { [info exists queuedbs($machid)] == 1 } {
		error "FAIL: repladd: machid $machid already exists"
	}

	set queuedbs($machid) [berkdb open -auto_commit \
	    -env $queueenv -create -recno -renumber repqueue$machid.db]
	error_check_good repqueue_create [is_valid_db $queuedbs($machid)] TRUE

	lappend machids $machid
}

# Acquire a handle to work with an existing machine's replication
# queue.  This is for situations where more than one process
# is working with a message queue.  In general, having more than one
# process handle the queue is wrong.  However, in order to test some
# things, we need two processes (since Tcl doesn't support threads).  We
# go to great pain in the test harness to make sure this works, but we
# don't let customers do it.
proc repljoin { machid } {
	global queueenv queuedbs machids

	set queuedbs($machid) [berkdb open -auto_commit \
	    -env $queueenv repqueue$machid.db]
	error_check_good repqueue_create [is_valid_db $queuedbs($machid)] TRUE

	lappend machids $machid
}

# Process a queue of messages, skipping every "skip_interval" entry.
# We traverse the entire queue, but since we skip some messages, we
# may end up leaving things in the queue, which should get picked up
# on a later run.
proc replprocessqueue { dbenv machid { skip_interval 0 } { hold_electp NONE } \
    { dupmasterp NONE } { errp NONE } } {
	global queuedbs queueenv errorCode
	global perm_response_list

	# hold_electp is a call-by-reference variable which lets our caller
	# know we need to hold an election.
	if { [string compare $hold_electp NONE] != 0 } {
		upvar $hold_electp hold_elect
	}
	set hold_elect 0

	# dupmasterp is a call-by-reference variable which lets our caller
	# know we have a duplicate master.
	if { [string compare $dupmasterp NONE] != 0 } {
		upvar $dupmasterp dupmaster
	}
	set dupmaster 0

	# errp is a call-by-reference variable which lets our caller
	# know we have gotten an error (that they expect).
	if { [string compare $errp NONE] != 0 } {
		upvar $errp errorp
	}
	set errorp 0

	set nproced 0

	set txn [$queueenv txn]

	# If we are running separate processes, the second process has
	# to join an existing message queue.
	if { [info exists queuedbs($machid)] == 0 } {
		repljoin $machid
	}

	set dbc [$queuedbs($machid) cursor -txn $txn]

	error_check_good process_dbc($machid) \
	    [is_valid_cursor $dbc $queuedbs($machid)] TRUE

	for { set dbt [$dbc get -first] } \
	    { [llength $dbt] != 0 } \
	    { } {
		set data [lindex [lindex $dbt 0] 1]
		set recno [lindex [lindex $dbt 0] 0]

		# If skip_interval is nonzero, we want to process messages
		# out of order.  We do this in a simple but slimy way--
		# continue walking with the cursor without processing the
		# message or deleting it from the queue, but do increment
		# "nproced".  The way this proc is normally used, the
		# precise value of nproced doesn't matter--we just don't
		# assume the queues are empty if it's nonzero.  Thus,
		# if we contrive to make sure it's nonzero, we'll always
		# come back to records we've skipped on a later call
		# to replprocessqueue.  (If there really are no records,
		# we'll never get here.)
		#
		# Skip every skip_interval'th record (and use a remainder other
		# than zero so that we're guaranteed to really process at least
		# one record on every call).
		if { $skip_interval != 0 } {
			if { $nproced % $skip_interval == 1 } {
				incr nproced
				set dbt [$dbc get -next]
				continue
			}
		}

		# We need to remove the current message from the queue,
		# because we're about to end the transaction and someone
		# else processing messages might come in and reprocess this
		# message which would be bad.
		error_check_good queue_remove [$dbc del] 0

		# We have to play an ugly cursor game here:  we currently
		# hold a lock on the page of messages, but rep_process_message
		# might need to lock the page with a different cursor in
		# order to send a response.  So save the next recno, close
		# the cursor, and then reopen and reset the cursor.
		# If someone else is processing this queue, our entry might
		# have gone away, and we need to be able to handle that.

		error_check_good dbc_process_close [$dbc close] 0
		error_check_good txn_commit [$txn commit] 0

		set ret [catch {$dbenv rep_process_message \
		    [lindex $data 2] [lindex $data 0] [lindex $data 1]} res]

		# Save all ISPERM and NOTPERM responses so we can compare their
		# LSNs to the LSN in the log.  The variable perm_response_list
		# holds the entire response so we can extract responses and
		# LSNs as needed.
		#
		if { [llength $perm_response_list] != 0 && \
		    ([is_substr $res ISPERM] || [is_substr $res NOTPERM]) } {
			lappend perm_response_list $res
		}

		if { $ret != 0 } {
			if { [string compare $errp NONE] != 0 } {
				set errorp "$dbenv $machid $res"
			} else {
				error "FAIL:[timestamp]\
				    rep_process_message returned $res"
			}
		}

		incr nproced

		# Now, re-establish the cursor position.  We fetch the
		# current record number.  If there is something there,
		# that is the record for the next iteration.  If there
		# is nothing there, then we've consumed the last item
		# in the queue.

		set txn [$queueenv txn]
		set dbc [$queuedbs($machid) cursor -txn $txn]
		set dbt [$dbc get -set_range $recno]

		if { $ret == 0 } {
			set rettype [lindex $res 0]
			set retval [lindex $res 1]
			#
			# Do nothing for 0 and NEWSITE
			#
			if { [is_substr $rettype HOLDELECTION] } {
				set hold_elect 1
			}
			if { [is_substr $rettype DUPMASTER] } {
				set dupmaster "1 $dbenv $machid"
			}
			if { [is_substr $rettype NOTPERM] || \
			    [is_substr $rettype ISPERM] } {
				set lsnfile [lindex $retval 0]
				set lsnoff [lindex $retval 1]
			}
		}

		if { $errorp != 0 } {
			# Break also on an error, caller wants to handle it.
			break
		}
		if { $hold_elect == 1 } {
			# Break also on a HOLDELECTION, for the same reason.
			break
		}
		if { $dupmaster == 1 } {
			# Break also on a DUPMASTER, for the same reason.
			break
		}

	}

	error_check_good dbc_close [$dbc close] 0
	error_check_good txn_commit [$txn commit] 0

	# Return the number of messages processed.
	return $nproced
}


set run_repl_flag "-run_repl"

proc extract_repl_args { args } {
	global run_repl_flag

	for { set arg [lindex $args [set i 0]] } \
	    { [string length $arg] > 0 } \
	    { set arg [lindex $args [incr i]] } {
		if { [string compare $arg $run_repl_flag] == 0 } {
			return [lindex $args [expr $i + 1]]
		}
	}
	return ""
}

proc delete_repl_args { args } {
	global run_repl_flag

	set ret {}

	for { set arg [lindex $args [set i 0]] } \
	    { [string length $arg] > 0 } \
	    { set arg [lindex $args [incr i]] } {
		if { [string compare $arg $run_repl_flag] != 0 } {
			lappend ret $arg
		} else {
			incr i
		}
	}
	return $ret
}

global elect_serial
global elections_in_progress
set elect_serial 0

# Start an election in a sub-process.
proc start_election { \
      pfx qdir home envid nsites nvotes pri timeout {err "none"} {crash 0}} {
	source ./include.tcl
	global elect_serial elections_in_progress machids
	global rep_verbose
	global verbose_type

	set filelist {}
	set ret [catch {glob $testdir/ELECTION*.$elect_serial} result]
	if { $ret == 0 } {
		set filelist [concat $filelist $result]
	}
	foreach f $filelist {
		fileremove -f $f
	}

	set oid [open $testdir/ELECTION_SOURCE.$elect_serial w]

	puts $oid "source $test_path/test.tcl"
	puts $oid "set is_repchild 1"
	puts $oid "replsetup $qdir"
	foreach i $machids { puts $oid "repladd $i" }
	set env_cmd "berkdb env -event -home $home -txn \
	    -rep_transport {$envid replsend} -errpfx $pfx"
	if { $rep_verbose == 1 } {
		append env_cmd " -errfile /dev/stdout -verbose {$verbose_type on}"
	} else {
		append env_cmd " -errfile $testdir/ELECTION_ERRFILE.$elect_serial"
	}
	puts $oid "set dbenv \[ $env_cmd \]"

	puts $oid "\$dbenv test abort $err"
	puts $oid "set res \[catch \{\$dbenv rep_elect $nsites \
	    $nvotes $pri $timeout\} ret\]"
	puts $oid "set r \[open \$testdir/ELECTION_RESULT.$elect_serial w\]"
	puts $oid "if \{\$res == 0 \} \{"
	puts $oid "puts \$r \"SUCCESS \$ret\""
	puts $oid "\} else \{"
	puts $oid "puts \$r \"ERROR \$ret\""
	puts $oid "\}"
	#
	# This loop calls rep_elect a second time with the error cleared.
	# We don't want to do that if we are simulating a crash.
	if { $err != "none" && $crash != 1 } {
		puts $oid "\$dbenv test abort none"
		puts $oid "set res \[catch \{\$dbenv rep_elect $nsites \
		    $nvotes $pri $timeout\} ret\]"
		puts $oid "if \{\$res == 0 \} \{"
		puts $oid "puts \$r \"SUCCESS \$ret\""
		puts $oid "\} else \{"
		puts $oid "puts \$r \"ERROR \$ret\""
		puts $oid "\}"
	}

	puts $oid "if \{ \[is_elected \$dbenv\] \} \{"
	puts $oid "puts \$r \"ELECTED \$dbenv\""
	puts $oid "\}"

	puts $oid "close \$r"
	close $oid

	set t [open "|$tclsh_path >& $testdir/ELECTION_OUTPUT.$elect_serial" w]
	if { $rep_verbose } {
		set t [open "|$tclsh_path" w]
	}
	puts $t "source ./include.tcl"
	puts $t "source $testdir/ELECTION_SOURCE.$elect_serial"
	flush $t

	set elections_in_progress($elect_serial) $t
	return $elect_serial
}

#
# If we are doing elections during upgrade testing, set
# upgrade to 1.  Doing that sets the priority to the
# test priority in rep_elect, which will simulate a
# 0-priority but electable site.
#
proc setpriority { priority nclients winner {start 0} {upgrade 0} } {
	global electable_pri
	upvar $priority pri

	for { set i $start } { $i < [expr $nclients + $start] } { incr i } {
		if { $i == $winner } {
			set pri($i) 100
		} else {
			if { $upgrade } {
				set pri($i) $electable_pri
			} else {
				set pri($i) 10
			}
		}
	}
}

# run_election has the following arguments:
# Arrays:
#	celist		List of env_handle, EID pairs.
#	errcmd		Array of where errors should be forced.
#	priority	Array of the priorities of each client env.
#	crash		If an error is forced, should we crash or recover?
# The upvar command takes care of making these arrays available to
# the procedure.
#
# Ordinary variables:
# 	qdir		Directory where the message queue is located.
#	msg		Message prefixed to the output.
#	elector		This client calls the first election.
#	nsites		Number of sites in the replication group.
#	nvotes		Number of votes required to win the election.
# 	nclients	Number of clients participating in the election.
#	win		The expected winner of the election.
#	reset_role	Should the new master (i.e. winner) be reset
#			to client role after the election?
#	dbname		Name of the underlying database.  The caller
#			should send in "NULL" if the database has not
# 			yet been created.
# 	ignore		Should the winner ignore its own election?
#			If ignore is 1, the winner is not made master.
#	timeout_ok	We expect that this election will not succeed
# 			in electing a new master (perhaps because there 
#			already is a master). 
#	elect_timeout	Timeout value to pass to rep_elect, which may be
#			a 2-element list in case "full election timeouts"
#			are in use.

proc run_election { celist errcmd priority crsh\
    qdir msg elector nsites nvotes nclients win reset_role\
    dbname {ignore 0} {timeout_ok 0} {elect_timeout 15000000} } {

	global elect_serial
	global is_hp_test
	global is_sunos_test
	global is_windows_test
	global rand_init
	upvar $celist cenvlist
	upvar $errcmd err_cmd
	upvar $priority pri
	upvar $crsh crash

	# Windows, HP-UX and SunOS require a longer timeout.
	if { [llength $elect_timeout] == 1 && ($is_windows_test == 1 ||
	    $is_hp_test == 1 || $is_sunos_test == 1) } {
		set elect_timeout [expr $elect_timeout * 2]
	}

	# Initialize tries based on timeout.  We use tries to loop looking for
	# messages because as sites are sleeping waiting for their timeout to
	# expire we need to keep checking for messages.
	#     The $elect_timeout might be either a scalar number, or a
	# two-element list in the case where we're interested in testing full
	# election timeouts.  Either is fine for passing to rep_elect (via
	# start_election); but of course for computing "$tries" we need just a
	# simple number.
	#
	if {[llength $elect_timeout] > 1} {
		set t [lindex $elect_timeout 1]
	} else {
		set t $elect_timeout
	}
	set tries [expr  ($t * 4) / 1000000]

	# Initialize each client participating in this election.  While we're at
	# it, save a copy of the envlist pair for the elector site, because
	# we'll need its EID and env handle in a moment (for the initial call to
	# start_election).  Note that $elector couldn't simple be used to index
	# into the list, because for example the envlist could look something
	# like this:
	#
	#     { { cenv4 4 } { cenv5 5 } { cenv6 6 } }
	#
	# and then "4" could be a valid $elector value (meaning EID 6).
	#
	set elector_pair NOTFOUND
	set win_pair NOTFOUND
	foreach pair $cenvlist {
		set id [lindex $pair 1]
		set i [expr $id - 2]
		if { $i == $elector } {
			set elector_pair $pair
		}
		set elect_pipe($i) INVALID
		set env [lindex $pair 0]
		$env event_info -clear
		replclear $id
		if { $i == $win } {
			set win_pair $pair
			set orig_pfx [$env get_errpfx]
		}
	}
	error_check_bad unknown_elector $elector_pair NOTFOUND
	error_check_good unknown_winner \
	    [expr { $win_pair != "NOTFOUND" || ! $reset_role }] 1

	#
	# XXX
	# We need to somehow check for the warning if nvotes is not
	# a majority.  Problem is that warning will go into the child
	# process' output.  Furthermore, we need a mechanism that can
	# handle both sending the output to a file and sending it to
	# /dev/stderr when debugging without failing the
	# error_check_good check.
	#
	puts "\t\t$msg.1: Election with nsites=$nsites,\
	    nvotes=$nvotes, nclients=$nclients"
	puts "\t\t$msg.2: First elector is $elector,\
	    expected winner is $win (eid [expr $win + 2])"
	incr elect_serial
	set pfx "CHILD$elector.$elect_serial"
	set env [lindex $elector_pair 0]
	set envid [lindex $elector_pair 1]
	set home [$env get_home]
	set elect_pipe($elector) [start_election \
	    $pfx $qdir $home $envid $nsites $nvotes $pri($elector) \
	    $elect_timeout $err_cmd($elector) $crash($elector)]
	tclsleep 2

	set got_newmaster 0

	# If we're simulating a crash, skip the while loop and
	# just give the initial election a chance to complete.
	set crashing 0
	for { set i 0 } { $i < $nclients } { incr i } {
		if { $crash($i) == 1 } {
			set crashing 1
		}
	}

	set child_elected 0

	if { $crashing == 1 } {
		tclsleep 10
	} else {
		set abandoned ""
		while { 1 } {
			set nproced 0
			set he 0

			foreach pair $cenvlist {
				set he 0
				set unavail 0
				set envid [lindex $pair 1]
				set i [expr $envid - 2]
				set clientenv($i) [lindex $pair 0]

				# If the "elected" event is received by the
				# child process, it writes to a file and we 
				# use check_election to get the message.  In
				# that case, the env set up in that child
				# is the elected env. 
				set child_done [check_election $elect_pipe($i)\
				    unavail child_elected]
		
				incr nproced [replprocessqueue \
				    $clientenv($i) $envid 0 he]

				# We use normal event processing to detect 
				# an "elected" event received by the parent 
				# process.
				set parent_elected [is_elected $clientenv($i)]

# puts "Tries $tries:\
# Processed queue for client $i, $nproced msgs he $he unavail $unavail"

				# Check for completed election.  If it's the
				# first time we've noticed it, deal with it.
				if { ( $child_elected || $parent_elected ) && \
				    $got_newmaster == 0 } {
					set got_newmaster 1

					# Make sure it's the expected winner.
					error_check_good right_winner \
					    $envid [expr $win + 2]

					# Reconfigure winning env as master.
					if { $ignore == 0 } {
						$clientenv($i) errpfx \
						    NEWMASTER
						error_check_good \
						    make_master($i) \
					    	    [$clientenv($i) \
						    rep_start -master] 0

						wait_all_startup $cenvlist $envid

						# Don't hold another election
						# yet if we are setting up a 
						# new master. This could 
						# cause the new master to
						# declare itself a client
						# during internal init.
						set he 0
					}

					# Occasionally force new log records
					# to be written, unless the database 
					# has not yet been created.
					set write [berkdb random_int 1 10]
					if { $write == 1 && $dbname != "NULL" } {
						set db [eval berkdb_open_noerr \
						    -env $clientenv($i) \
						    -auto_commit $dbname]
						error_check_good dbopen \
						    [is_valid_db $db] TRUE
						error_check_good dbclose \
						    [$db close] 0
					}
				}

				if { $he == 1 && $got_newmaster == 0 } {
					#
					# Only close down the election pipe if the
					# previously created one is done and
					# waiting for new commands, otherwise
					# if we try to close it while it's in
					# progress we hang this main tclsh.  If
					# it's not done, hold onto it in an
					# "abandoned" list, where we'll clean it
					# up later.
					#
					if { $elect_pipe($i) != "INVALID" && \
					    $child_done == 1 } {
						close_election $elect_pipe($i)
						set elect_pipe($i) "INVALID"
					} elseif { $elect_pipe($i) != "INVALID" } {
						lappend abandoned $elect_pipe($i)
						set elect_pipe($i) "INVALID"
					}
# puts "Starting election on client $i"
					incr elect_serial
					set pfx "CHILD$i.$elect_serial"
					set home [$clientenv($i) get_home]
					set elect_pipe($i) [start_election \
					    $pfx $qdir \
					    $home $envid $nsites \
					    $nvotes $pri($i) $elect_timeout]
					set got_hold_elect($i) 1
				}
			}

			# We need to wait around to make doubly sure that the
			# election has finished...
			if { $nproced == 0 } {
				incr tries -1
				#
				# If we have a newmaster already, set tries
				# down to just allow straggling messages to
				# be processed.  Tries could be a very large
				# number if we have long timeouts.
				#
				if { $got_newmaster != 0 && $tries > 10 } {
					set tries 10
				}
				if { $tries == 0 } {
					break
				} else {
					tclsleep 1
				}
			} else {
				set tries $tries
			}
			set abandoned [cleanup_abandoned $abandoned]
		}

		# If we did get a new master, its identity was checked 
		# at that time.  But we still have to make sure that we 
		# didn't just time out. 

		if { $got_newmaster == 0 && $timeout_ok == 0 } {
			error "FAIL: Did not elect new master."
		}
	}
	cleanup_elections

	#
	# Make sure we've really processed all the post-election
	# sync-up messages.  If we're simulating a crash, don't process
	# any more messages.
	#
	if { $crashing == 0 } {
		process_msgs $cenvlist
	}

	if { $reset_role == 1 } {
		puts "\t\t$msg.3: Changing new master to client role"
		error_check_good log_flush [$clientenv($win) log_flush] 0
		error_check_good role_chg [$clientenv($win) rep_start -client] 0
		$clientenv($win) errpfx $orig_pfx

		if { $crashing == 0 } {
			process_msgs $cenvlist
		}
	}
}

proc wait_all_startup { envlist master } {
	process_msgs $envlist

	for { set tries 0 } { $tries < 10 } { incr tries } {
		# Find a client that has not yet reached startupdone.
		# 
		set found 0
		foreach pair $envlist {
			foreach {env eid} $pair {}
			if { $eid == $master } {
				continue
			}
			if {![stat_field $env rep_stat "Startup complete"]} {
				set found 1
				break
			}
		}

		# We couldn't find a client who hadn't got startup done.  That
		# means we're all done and happy.
		# 
		if {!$found} {
			return
		}
		tclsleep 1
		process_msgs $envlist
	}
	error "FAIL: Clients could not get startupdone after master elected."
}

proc cleanup_abandoned { es } {
	set remaining ""
	foreach e $es {
		if { [check_election $e unused1 unused2] } {
			close_election $e
		} else {
			lappend remaining $e
		}
	}
	return $remaining
}

# Retrieve election results that may have been reported by a child process.  The
# child process communicates the results to us (the parent process) by writing
# them into a file.
# 
proc check_election { id unavailp elected_flagp } {
	source ./include.tcl

	if { $id == "INVALID" } {
		return 0
	}
	upvar $unavailp unavail
	upvar $elected_flagp elected_flag

	set unavail 0
	set elected_flag 0

	set res [catch {open $testdir/ELECTION_RESULT.$id} nmid]
	if { $res != 0 } {
		return 0
	}
	while { [gets $nmid val] != -1 } {
#		puts "result $id: $val"
		set str [lindex $val 0]
		if { [is_substr $val UNAVAIL] } {
			set unavail 1
		}
		if { [is_substr $val ELECTED] } {
			set elected_flag 1
		}
	}
	close $nmid
	return 1
}

proc is_elected { env } {
	return [is_event_present $env "elected"]
}

proc is_startup_done { env } {
	return [is_event_present $env "startupdone"]
}

proc is_event_present { env event_name } {
	set event_info [find_event [$env event_info] $event_name]
	return [expr [llength $event_info] > 0]
}

# Extracts info about a given event type from a list of events that have
# occurred in an environment.  The event_info might look something like this:
#
#     {startupdone {}} {newmaster 2}
#
# A key would be something like "startupdone" or "newmaster".  The return value
# might look like "newmaster 2".  In other words, we return the complete
# information about a single event -- the event named by the key.  If the event
# named by the key does not appear in the event_info, we return "".
# 
proc find_event { event_info key } {

	# Search for a glob pattern: a string beginning with the key name, and
	# containing anything after it.
	#
	return [lsearch -inline $event_info [append key *]]
}

proc close_election { i } {
	global elections_in_progress
	global noenv_messaging
	global qtestdir

	if { $noenv_messaging == 1 } {
		set testdir $qtestdir
	}

	set t $elections_in_progress($i)
	puts $t "replclose \$testdir/MSGQUEUEDIR"
	puts $t "\$dbenv close"
	close $t
	unset elections_in_progress($i)
}

proc cleanup_elections { } {
	global elect_serial elections_in_progress

	for { set i 0 } { $i <= $elect_serial } { incr i } {
		if { [info exists elections_in_progress($i)] != 0 } {
			close_election $i
		}
	}

	set elect_serial 0
}

#
# This is essentially a copy of test001, but it only does the put/get
# loop AND it takes an already-opened db handle.
#
proc rep_test { method env repdb {nentries 10000} \
    {start 0} {skip 0} {needpad 0} args } {

	source ./include.tcl
	global databases_in_memory

	#
	# Open the db if one isn't given.  Close before exit.
	#
	if { $repdb == "NULL" } {
		if { $databases_in_memory == 1 } {
			set testfile { "" "test.db" }
		} else {
			set testfile "test.db"
		}
		set largs [convert_args $method $args]
		set omethod [convert_method $method]
		set db [eval {berkdb_open_noerr} -env $env -auto_commit\
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
	} else {
		set db $repdb
	}

	puts "\t\tRep_test: $method $nentries key/data pairs starting at $start"
	set did [open $dict]

	# The "start" variable determines the record number to start
	# with, if we're using record numbers.  The "skip" variable
	# determines which dictionary entry to start with.  In normal
	# use, skip is equal to start.

	if { $skip != 0 } {
		for { set count 0 } { $count < $skip } { incr count } {
			gets $did str
		}
	}
	set pflags ""
	set gflags ""
	set txn ""

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}
	puts "\t\tRep_test.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	set count 0

	# Checkpoint 10 times during the run, but not more
	# frequently than every 5 entries.
	set checkfreq [expr $nentries / 10]

	# Abort occasionally during the run.
	set abortfreq [expr $nentries / 15]

	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1 + $start]
			if { 0xffffffff > 0 && $key > 0xffffffff } {
				set key [expr $key - 0x100000000]
			}
			if { $key == 0 || $key - 0xffffffff == 1 } {
				incr key
				incr count
			}
			set kvals($key) [pad_data $method $str]
		} else {
			set key $str
			set str [reverse $str]
		}
		#
		# We want to make sure we send in exactly the same
		# length data so that LSNs match up for some tests
		# in replication (rep021).
		#
		if { [is_fixed_length $method] == 1 && $needpad } {
			#
			# Make it something visible and obvious, 'A'.
			#
			set p 65
			set str [make_fixed_length $method $str $p]
			set kvals($key) $str
		}
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
		set ret [eval \
		    {$db put} $txn $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0
		error_check_good txn [$t commit] 0

		if { $checkfreq < 5 } {
			set checkfreq 5
		}
		if { $abortfreq < 3 } {
			set abortfreq 3
		}
		#
		# Do a few aborted transactions to test that
		# aborts don't get processed on clients and the
		# master handles them properly.  Just abort
		# trying to delete the key we just added.
		#
		if { $count % $abortfreq == 0 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set ret [$db del -txn $t $key]
			error_check_good txn [$t abort] 0
		}
		if { $count % $checkfreq == 0 } {
			error_check_good txn_checkpoint($count) \
			    [$env txn_checkpoint] 0
		}
		incr count
	}
	close $did
	set privateindx [lsearch [$env get_open_flags] "-private"]
	if { $databases_in_memory == 1 && $privateindx == -1 } {
	    set inmemfile [lindex [$db get_dbname] 1]
	    set inmemdir [$env get_home]
	    # Don't dump/load, we don't have a replicated environment handle
	    error_check_good dbverify_inmem \
		[dbverify_inmem $inmemfile $inmemdir "" 0 1 0 1] 0
	}
	if { $repdb == "NULL" } {
		error_check_good rep_close [$db close] 0
	}
}

#
# This is essentially a copy of rep_test, but it only does the put/get
# loop in a long running txn to an open db.  We use it for bulk testing
# because we want to fill the bulk buffer some before sending it out.
# Bulk buffer gets transmitted on every commit.
#
proc rep_test_bulk { method env repdb {nentries 10000} \
    {start 0} {skip 0} {useoverflow 0} args } {
	source ./include.tcl

	global overflowword1
	global overflowword2
	global databases_in_memory

	if { [is_fixed_length $method] && $useoverflow == 1 } {
		puts "Skipping overflow for fixed length method $method"
		return
	}
	#
	# Open the db if one isn't given.  Close before exit.
	#
	if { $repdb == "NULL" } {
		if { $databases_in_memory == 1 } {
			set testfile { "" "test.db" }
		} else {
			set testfile "test.db"
		}
		set largs [convert_args $method $args]
		set omethod [convert_method $method]
		set db [eval {berkdb_open_noerr -env $env -auto_commit -create \
		    -mode 0644} $largs $omethod $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
	} else {
		set db $repdb
	}

	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	# If we are not using an external env, then test setting
	# the database cache size and using multiple caches.
	puts \
"\t\tRep_test_bulk: $method $nentries key/data pairs starting at $start"
	set did [open $dict]

	# The "start" variable determines the record number to start
	# with, if we're using record numbers.  The "skip" variable
	# determines which dictionary entry to start with.  In normal
	# use, skip is equal to start.

	if { $skip != 0 } {
		for { set count 0 } { $count < $skip } { incr count } {
			gets $did str
		}
	}
	set pflags ""
	set gflags ""
	set txn ""

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}
	puts "\t\tRep_test_bulk.a: put/get loop in 1 txn"
	# Here is the loop where we put and get each key/data pair
	set count 0

	set t [$env txn]
	error_check_good txn [is_valid_txn $t $env] TRUE
	set txn "-txn $t"
	set pid [pid]
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1 + $start]
			if { 0xffffffff > 0 && $key > 0xffffffff } {
				set key [expr $key - 0x100000000]
			}
			if { $key == 0 || $key - 0xffffffff == 1 } {
				incr key
				incr count
			}
			set kvals($key) [pad_data $method $str]
			if { [is_fixed_length $method] == 0 } {
				set str [repeat $str 100]
			}
		} else {
			set key $str.$pid
			set str [repeat $str 100]
		}
		#
		# For use for overflow test.
		#
		if { $useoverflow == 0 } {
			if { [string length $overflowword1] < \
			    [string length $str] } {
				set overflowword2 $overflowword1
				set overflowword1 $str
			}
		} else {
			if { $count == 0 } {
				set len [string length $overflowword1]
				set word $overflowword1
			} else {
				set len [string length $overflowword2]
				set word $overflowword1
			}
			set rpt [expr 1024 * 1024 / $len]
			incr rpt
			set str [repeat $word $rpt]
		}
		set ret [eval \
		    {$db put} $txn $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0
		incr count
	}
	error_check_good txn [$t commit] 0
	error_check_good txn_checkpoint [$env txn_checkpoint] 0
	close $did
	set privateindx [lsearch [$env get_open_flags] "-private"]
	if { $databases_in_memory == 1 && $privateindx == -1 } {
	    set inmemfile [lindex [$db get_dbname] 1]
	    set inmemdir [$env get_home]
	    # Don't dump/load, we don't have a replicated environment handle
	    error_check_good dbverify_inmem \
		[dbverify_inmem $inmemfile $inmemdir "" 0 1 0 1] 0
	}
	if { $repdb == "NULL" } {
		error_check_good rep_close [$db close] 0
	}
}

proc rep_test_upg { method env repdb {nentries 10000} \
    {start 0} {skip 0} {needpad 0} {inmem 0} args } {

	source ./include.tcl

	#
	# Open the db if one isn't given.  Close before exit.
	#
	if { $repdb == "NULL" } {
		if { $inmem == 1 } {
			set testfile { "" "test.db" }
		} else {
			set testfile "test.db"
		}
		set largs [convert_args $method $args]
		set omethod [convert_method $method]
		set db [eval {berkdb_open_noerr} -env $env -auto_commit\
		    -create -mode 0644 $omethod $largs $testfile]
		error_check_good reptest_db [is_valid_db $db] TRUE
	} else {
		set db $repdb
	}

	set pid [pid]
	puts "\t\tRep_test_upg($pid): $method $nentries key/data pairs starting at $start"
	set did [open $dict]

	# The "start" variable determines the record number to start
	# with, if we're using record numbers.  The "skip" variable
	# determines which dictionary entry to start with.  In normal
	# use, skip is equal to start.

	if { $skip != 0 } {
		for { set count 0 } { $count < $skip } { incr count } {
			gets $did str
		}
	}
	set pflags ""
	set gflags ""
	set txn ""

	if { [is_record_based $method] == 1 } {
		append gflags " -recno"
	}
	puts "\t\tRep_test.a: put/get loop"
	# Here is the loop where we put and get each key/data pair
	set count 0

	# Checkpoint 10 times during the run, but not more
	# frequently than every 5 entries.
	set checkfreq [expr $nentries / 10]

	# Abort occasionally during the run.
	set abortfreq [expr $nentries / 15]

	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1 + $start]
			if { 0xffffffff > 0 && $key > 0xffffffff } {
				set key [expr $key - 0x100000000]
			}
			if { $key == 0 || $key - 0xffffffff == 1 } {
				incr key
				incr count
			}
			set kvals($key) [pad_data $method $str]
		} else {
			#
			# With upgrade test, we run the same test several
			# times with the same database.  We want to have
			# some overwritten records and some new records.
			# Therefore append our pid to half the keys.
			#
			if { $count % 2 } {
				set key $str.$pid
			} else {
				set key $str
			}
			set str [reverse $str]
		}
		#
		# We want to make sure we send in exactly the same
		# length data so that LSNs match up for some tests
		# in replication (rep021).
		#
		if { [is_fixed_length $method] == 1 && $needpad } {
			#
			# Make it something visible and obvious, 'A'.
			#
			set p 65
			set str [make_fixed_length $method $str $p]
			set kvals($key) $str
		}
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
# puts "rep_test_upg: put $count of $nentries: key $key, data $str"
		set ret [eval \
		    {$db put} $txn $pflags {$key [chop_data $method $str]}]
		error_check_good put $ret 0
		error_check_good txn [$t commit] 0

		if { $checkfreq < 5 } {
			set checkfreq 5
		}
		if { $abortfreq < 3 } {
			set abortfreq 3
		}
		#
		# Do a few aborted transactions to test that
		# aborts don't get processed on clients and the
		# master handles them properly.  Just abort
		# trying to delete the key we just added.
		#
		if { $count % $abortfreq == 0 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set ret [$db del -txn $t $key]
			error_check_good txn [$t abort] 0
		}
		if { $count % $checkfreq == 0 } {
			error_check_good txn_checkpoint($count) \
			    [$env txn_checkpoint] 0
		}
		incr count
	}
	close $did
	if { $repdb == "NULL" } {
		error_check_good rep_close [$db close] 0
	}
}

proc rep_test_upg.check { key data } {
	#
	# If the key has the pid attached, strip it off before checking.
	# If the key does not have the pid attached, then it is a recno
	# and we're done.
	#
	set i [string first . $key]
	if { $i != -1 } {
		set key [string replace $key $i end]
	}
	error_check_good "key/data mismatch" $data [reverse $key]
}

proc rep_test_upg.recno.check { key data } {
	#
	# If we're a recno database we better not have a pid in the key.
	# Otherwise we're done.
	#
	set i [string first . $key]
	error_check_good pid $i -1
}

# In a situation where logs are being archived off a master, it's
# possible for a client to get so far behind that there is a gap
# where the highest numbered client log file is lower than the 
# lowest numbered master log file, creating the need for internal
# initialization of the client. 
#
# This proc creates that situation for use in internal init tests.
# It closes the selected client and pushes the master forward 
# while archiving the master's log files.  

proc push_master_ahead { method masterenv masterdir m_logtype \
    clientenv clientid db start niter flags largs } {
	global util_path 

	# Identify last client log file and then close the client.
	puts "\t\tRep_push.a: Close client."
	set last_client_log [get_logfile $clientenv last]
	error_check_good client_close [$clientenv close] 0

	set stop 0
	while { $stop == 0 } {
		# Run rep_test in the master.  Discard messages
		# for the closed client.
		puts "\t\tRep_push.b: Pushing master ahead."
	 	eval rep_test \
		    $method $masterenv $db $niter $start $start 0 $largs
		incr start $niter
		replclear $clientid

		puts "\t\tRep_push.c: Run db_archive on master."
		if { $m_logtype == "on-disk"} {
			$masterenv log_flush
			eval exec $util_path/db_archive $flags -d -h $masterdir
		}

		# Check to see whether the gap has appeared yet.
		set first_master_log [get_logfile $masterenv first]
		if { $first_master_log > $last_client_log } {
			set stop 1
		}
	}
	return $start
}

proc run_repmgr_tests { which {display 0} {run 1} } {
	source ./include.tcl
	if { !$display && $is_freebsd_test == 1 } {
		puts "Skipping replication manager tests on FreeBSD platform."
		return
	}

	if { $which == "basic" } {
		set testname basic_repmgr_test
	} elseif { $which == "election" } {
		set testname basic_repmgr_election_test
	} elseif { $which == "init" } {
		set testname basic_repmgr_init_test
	} else {
		puts "No repmgr test of that name"
		return 
	}

	if { $run } { 
		puts "Running all cases of $testname."
	}

	set niter 100
	foreach inmemdb { 0 1 } {
		foreach inmemlog { 0 1 } {
			foreach inmemrep { 0 1 } {
				foreach envprivate { 0 1 } {
					foreach bulk { 0 1 } {
		if { $display } {
			puts "$testname $niter $inmemdb $inmemlog \
			    $inmemrep $envprivate $bulk"
		} 

		if { $run } {
			if { [catch {$testname $niter $inmemdb $inmemlog \
			    $inmemrep $envprivate $bulk} res ] } {
				set databases_in_memory 0
				error "FAIL: $res"
			}
		}
					}
				}
			}
		}
	}
}

proc print_repmgr_headers { test niter inmemdb inmemlog inmemrep \
    envprivate bulk } {

	set dbmsg "on-disk databases"
	if { $inmemdb } {
		set dbmsg "in-memory databases"
	} 
	
	set logmsg "on-disk logs"
	if { $inmemlog } {
		set logmsg "in-memory logs"
	}

	set repmsg "on-disk rep files"
	if { $inmemrep } {
		set repmsg "in-memory rep files"
	} 

	set regmsg "on-disk region files"
	if { $envprivate } {
		set regmsg "in-memory region files"
	}

	set bulkmsg "regular processing"
	if { $bulk } {
		set bulkmsg "bulk processing"
	}

	puts "\n$test with:"
	puts "\t$dbmsg"
	puts "\t$logmsg" 
	puts "\t$repmsg"
	puts "\t$regmsg"
	puts "\t$bulkmsg"
	puts -nonewline "To reproduce this case: $test "
	puts "$niter $inmemdb $inmemlog $inmemrep $envprivate $bulk" 
}

# Verify that no replication files are present in a given directory.
# This checks for the gen, egen, internal init, temp db and page db
# files.
#
proc no_rep_files_on_disk { dir } {
    error_check_good nogen [file exists "$dir/__db.rep.gen"] 0
    error_check_good noegen [file exists "$dir/__db.rep.egen"] 0
    error_check_good noinit [file exists "$dir/__db.rep.init"] 0
    error_check_good notmpdb [file exists "$dir/__db.rep.db"] 0
    error_check_good nopgdb [file exists "$dir/__db.reppg.db"] 0
    error_check_good nosysdb [file exists "$dir/__db.rep.system"] 0
}

proc process_msgs { elist {perm_response 0} {dupp NONE} {errp NONE} \
    {upg 0} } {
	if { $perm_response == 1 } {
		global perm_response_list
		set perm_response_list {{}}
	}

	if { [string compare $dupp NONE] != 0 } {
		upvar $dupp dupmaster
		set dupmaster 0
	} else {
		set dupmaster NONE
	}

	if { [string compare $errp NONE] != 0 } {
		upvar $errp errorp
		set errorp 0
		set var_name errorp
	} else {
		set errorp NONE
		set var_name NONE
	}

	set upgcount 0
	while { 1 } {
		set nproced 0
		incr nproced [proc_msgs_once $elist dupmaster $var_name]
		#
		# If we're running the upgrade test, we are running only
		# our own env, we need to loop a bit to allow the other
		# upgrade procs to run and reply to our messages.
		#
		if { $upg == 1 && $upgcount < 10 } {
			tclsleep 2
			incr upgcount
			continue
		}
		if { $nproced == 0 } {
			break
		} else {
			set upgcount 0
		}
	}
}


proc proc_msgs_once { elist {dupp NONE} {errp NONE} } {
	global noenv_messaging

	if { [string compare $dupp NONE] != 0 } {
		upvar $dupp dupmaster
		set dupmaster 0
	} else {
		set dupmaster NONE
	}

	if { [string compare $errp NONE] != 0 } {
		upvar $errp errorp
		set errorp 0
		set var_name errorp
	} else {
		set errorp NONE
		set var_name NONE
	}

	set nproced 0
	foreach pair $elist {
		set envname [lindex $pair 0]
		set envid [lindex $pair 1]
		#
		# If we need to send in all the other args
# puts "Call replpq with on $envid"
		if { $noenv_messaging } {
			incr nproced [replprocessqueue_noenv $envname $envid \
			    0 NONE dupmaster $var_name]
		} else {
			incr nproced [replprocessqueue $envname $envid \
			    0 NONE dupmaster $var_name]
		}
		#
		# If the user is expecting to handle an error and we get
		# one, return the error immediately.
		#
		if { $dupmaster != 0 && $dupmaster != "NONE" } {
			return 0
		}
		if { $errorp != 0 && $errorp != "NONE" } {
# puts "Returning due to error $errorp"
			return 0
		}
	}
	return $nproced
}

proc rep_verify { masterdir masterenv clientdir clientenv \
    {compare_shared_portion 0} {match 1} {logcompare 1} \
    {dbname "test.db"} {datadir ""} } {
	global util_path
	global encrypt
	global passwd
	global databases_in_memory
	global repfiles_in_memory
	global env_private

	# Whether a named database is in-memory or on-disk, only the 
	# the name itself is passed in.  Here we do the syntax adjustment
	# from "test.db" to { "" "test.db" } for in-memory databases.
	# 
	if { $databases_in_memory && $dbname != "NULL" } {
		set dbname " {} $dbname "
	} 

	# Check locations of dbs, repfiles, region files.
	if { $dbname != "NULL" } {
		#
		# check_db_location assumes the path through the env_home
		# to the datadir, when one is given.  Construct that here.
		#
		if { $datadir != "" } {
			set mhome [get_home $masterenv]
			set chome [get_home $clientenv]
			set mdatadir $mhome/$datadir
			set cdatadir $chome/$datadir
		} else {
			set mdatadir ""
			set cdatadir ""
		}
		check_db_location $masterenv $dbname $mdatadir
		check_db_location $clientenv $dbname $cdatadir
	}

	if { $repfiles_in_memory } {
		no_rep_files_on_disk $masterdir
		no_rep_files_on_disk $clientdir
	}
	if { $env_private } {
		no_region_files_on_disk $masterdir
		no_region_files_on_disk $clientdir
	}
	
	# The logcompare flag indicates whether to compare logs.
	# Sometimes we run a test where rep_verify is run twice with
	# no intervening processing of messages.  If that test is
	# on a build with debug_rop enabled, the master's log is
	# altered by the first rep_verify, and the second rep_verify
	# will fail.
	# To avoid this, skip the log comparison on the second rep_verify
	# by specifying logcompare == 0.
	#
	if { $logcompare } {
		set msg "Logs and databases"
	} else {
		set msg "Databases ($dbname)"
	}

	if { $match } {
		puts "\t\tRep_verify: $clientdir: $msg should match"
	} else {
		puts "\t\tRep_verify: $clientdir: $msg should not match"
	}
	# Check that master and client logs and dbs are identical.

	# Logs first, if specified ...
	#
	# If compare_shared_portion is set, run db_printlog on the log
	# subset that both client and master have.  Either the client or
	# the master may have more (earlier) log files, due to internal
	# initialization, in-memory log wraparound, or other causes.
	#
	if { $logcompare } {
		error_check_good logcmp \
		    [logcmp $masterenv $clientenv $compare_shared_portion] 0
	
		if { $dbname == "NULL" } {
			return
		}
	}

	# ... now the databases.
	#
	# We're defensive here and throw an error if a database does
	# not exist.  If opening the first database succeeded but the
	# second failed, we close the first before reporting the error.
	#
	if { [catch {eval {berkdb_open_noerr} -env $masterenv\
	    -rdonly $dbname} db1] } {
		error "FAIL:\
		    Unable to open first db $dbname in rep_verify: $db1"
	}
	if { [catch {eval {berkdb_open_noerr} -env $clientenv\
	    -rdonly $dbname} db2] } {
		error_check_good close_db1 [$db1 close] 0
		error "FAIL:\
		    Unable to open second db $dbname in rep_verify: $db2"
	}

	# db_compare uses the database handles to do the comparison, and 
	# we pass in the $mumbledir/$dbname string as a label to make it 
	# easier to identify the offending database in case of failure. 
	# Therefore this will work for both in-memory and on-disk databases.
	if { $match } {
		error_check_good [concat comparedbs. $dbname] [db_compare \
		    $db1 $db2 $masterdir/$dbname $clientdir/$dbname] 0
	} else {
		error_check_bad comparedbs [db_compare \
		    $db1 $db2 $masterdir/$dbname $clientdir/$dbname] 0
	}
	error_check_good db1_close [$db1 close] 0
	error_check_good db2_close [$db2 close] 0
}

proc rep_verify_inmem { masterenv clientenv mdb cdb } {
	#
	# Can't use rep_verify to compare the logs because each
	# commit record from db_printlog shows the database name
	# as text on the master and as the file uid on the client
	# because the client cannot find the "file".  
	#
	# !!! Check the LSN first.  Otherwise the DB->stat for the
	# number of records will write a log record on the master if
	# the build is configured for debug_rop.  Work around that issue.
	#
	set mlsn [next_expected_lsn $masterenv]
	set clsn [next_expected_lsn $clientenv]
	error_check_good lsn $mlsn $clsn

	set mrecs [stat_field $mdb stat "Number of records"]
	set crecs [stat_field $cdb stat "Number of records"]
	error_check_good recs $mrecs $crecs
}

# Return the corresponding site number for an individual port number
# previously returned by available_ports.  This procedure assumes that 
# the baseport number, n and rangeincr value are unchanged from the 
# original call to available_ports.  If a port value is supplied that
# is outside the expected baseport, n and rangeincr range, this procedure
# returns -1.
#
# As in available_ports, it uses a starting baseport number that falls
# in the non-ephemeral range on most platforms, which can be overridden
# by setting environment variable BDBBASEPORT.
#
proc site_from_port { port n { rangeincr 10 } } {
	global env

	if { [info exists env(BDBBASEPORT)] } {
		set baseport $env(BDBBASEPORT)
	} else {
		set baseport 30100
	}

	if { $port > $baseport && $port < $baseport + $rangeincr * 100 } {
		set site [expr ($port - $baseport) % $rangeincr]
		if { $site <= $n } {
			return $site
		}
	}
	return -1
}

# Wait (a limited amount of time) for an arbitrary condition to become true,
# polling once per second.  If time runs out we throw an error: a successful
# return implies the condition is indeed true.
# 
proc await_condition { cond { limit 20 } } {
	for {set i 0} {$i < $limit} {incr i} {
		if {[uplevel 1 [list expr $cond]]} {
			return
		}
		tclsleep 1
	}
	error "FAIL: condition \{$cond\} not achieved in $limit seconds."
}

proc await_startup_done { env { limit 20 } } {
	await_condition {[stat_field $env rep_stat "Startup complete"]} $limit
}

proc await_event { env event_name { limit 20 } } {
	await_condition {[is_event_present $env $event_name]} $limit
	return [find_event [$env event_info] $event_name]
}

# Wait (a limited amount of time) for an election to yield the expected
# environment as winner.
#
proc await_expected_master { env { limit 20 } } {
	await_condition {[stat_field $env rep_stat "Role"] == "master"} $limit
}

proc do_leaseop { env db method key envlist { domsgs 1 } } {
	global alphabet

	#
	# Put a txn to the database.  Process messages to envlist
	# if directed to do so.  Read data on the master, ignoring
	# leases (should always succeed).
	#
	set num [berkdb random_int 1 100]
	set data $alphabet.$num
	set t [$env txn]
	error_check_good txn [is_valid_txn $t $env] TRUE
	set txn "-txn $t"
	set ret [eval \
	    {$db put} $txn {$key [chop_data $method $data]}]
	error_check_good put $ret 0
	error_check_good txn [$t commit] 0

	if { $domsgs } {
		process_msgs $envlist
	}

	#
	# Now make sure we can successfully read on the master
	# if we ignore leases.  That should always work.  The
	# caller will do any lease related calls and checks
	# that are specific to the test.
	#
	set kd [$db get -nolease $key]
	set curs [$db cursor]
	set ckd [$curs get -nolease -set $key]
	$curs close
	error_check_good kd [llength $kd] 1
	error_check_good ckd [llength $ckd] 1
}

#
# Get the given key, expecting status depending on whether leases
# are currently expected to be valid or not.
#
proc check_leaseget { db key getarg status } {
	set stat [catch {eval {$db get} $getarg $key} kd]
	if { $status != 0 } {
		error_check_good get_result $stat 1
		error_check_good kd_check \
		    [is_substr $kd $status] 1
	} else {
		error_check_good get_result_good $stat $status
		error_check_good dbkey [lindex [lindex $kd 0] 0] $key
	}
	set curs [$db cursor]
	set stat [catch {eval {$curs get} $getarg -set $key} kd]
	if { $status != 0 } {
		error_check_good get_result2 $stat 1
		error_check_good kd_check \
		    [is_substr $kd $status] 1
	} else {
		error_check_good get_result2_good $stat $status
		error_check_good dbckey [lindex [lindex $kd 0] 0] $key
	}
	$curs close
}

# Simple utility to check a client database for expected values.  It does not
# handle dup keys.
# 
proc verify_client_data { env db items } {
	set dbp [berkdb open -env $env $db]
	foreach i $items {
		foreach {key expected_value} $i {
			set results [$dbp get $key]
			error_check_good result_length [llength $results] 1
			set value [lindex $results 0 1]
			error_check_good expected_value $value $expected_value
		}
	}
	$dbp close
}

proc make_dbconfig { dir cnfs } {
	global rep_verbose

	set f [open "$dir/DB_CONFIG" "w"]
	foreach line $cnfs {
		puts $f $line
	}
	if {$rep_verbose} {
		puts $f "set_verbose DB_VERB_REPLICATION"
	}
	close $f
}

proc open_site_prog { cmds } {

	set site_prog [setup_site_prog]

	set s [open "| $site_prog" "r+"]
	fconfigure $s -buffering line
	set synced yes
	foreach cmd $cmds {
		puts $s $cmd
		if {[lindex $cmd 0] == "start"} {
			gets $s
			set synced yes
		} else {
			set synced no
		}
	}
	if {! $synced} {
		puts $s "echo done"
		gets $s
	}
	return $s
}

proc setup_site_prog { } {
	source ./include.tcl
	
	# Generate the proper executable name for the system. 
	if { $is_windows_test } {
		set repsite_executable db_repsite.exe
	} else {
		set repsite_executable db_repsite
	}

	# Check whether the executable exists. 
	if { [file exists $util_path/$repsite_executable] == 0 } {
		error "Skipping: db_repsite executable\
		    not found.  Is it built?"
	} else {
		set site_prog $util_path/$repsite_executable
	}
	return $site_prog
}

proc next_expected_lsn { env } {
	return [stat_field $env rep_stat "Next LSN expected"]
}

proc lsn_file { lsn } {
	if { [llength $lsn] != 2 } {
		error "not a valid LSN: $lsn"
	}

	return [lindex $lsn 0]
}

proc assert_rep_flag { dir flag value } {
	global util_path

	set stat [exec $util_path/db_stat -N -RA -h $dir]
	set present [is_substr $stat $flag]
	error_check_good expected.flag.$flag $present $value
}

# Kind of like an abbreviated lsearch(3tcl), except that the list must be a list
# of lists, and we search each list for a key in the "head" (0-th) position.
#
#     lsearch_head ?option? list_of_lists key
#
# "option" can be -index or -inline (or may be omitted)
# 
proc lsearch_head { args } {
	if {[llength $args] > 2} {
		foreach { how lists key } $args {}
	} else {
		set how -index
		foreach { lists key } $args {}
	}

	set i 0
	foreach list $lists {
		if { $key eq [lindex $list 0] } {
			if {$how eq "-inline"} {
				return $list
			} else {
				return $i
			}
		}
		incr i
	}
	if { $how eq "-inline" } {
		return ""
	} else {
		return -1
	}
}

# 
# To build a profiled version of BDB and tclsh and run the rep
# tests individually with profiling you need the following steps:
#
# 0. NOTE: References to 'X' below for BDB versions obviously need
#	the current release version number used.
# 1. Need to build a static, profiled version of DB and install it.
#	../dist/configure with --disable-shared and --enable-static.
#
#	NOTE: Assumes you already have --enable-debug configured.
#
# (if you use the script 'dbconf' the 'args' line looks like:)
# args="--disable-shared --enable-static --with-tcl=/usr/local/lib --enable-test $args"
#
#	Edit build_unix/Makefile and add '-pg' to CFLAGS and LDFLAGS.
#	make
#	sudo make install
#
# 2. Need to make sure LD_LIBRARY_PATH in your .cshrc is pointing to the
#	right path for the profiled DB, such as
#	... :./.libs:/usr/local/BerkeleyDB.5.X/lib: ...
#
# source your new .cshrc if necessary.
#
# [NOTE: Your Tcl version may vary.  Use the paths and versions as a
# guide.  Mostly it should be the same.  These steps work for Tcl 5.8.]
# 3. Build a new, profiling tclsh:
#	Go to your Tcl source directory, e.g. <..srcpath>/tcl8.5.8/unix
#	make clean
#	./configure --disable-shared
#
#	Edit the generated Makefile:
#	Add '-L /usr/local/BerkeleyDB.5.X/lib' to tclsh target
#	   after ${TCLSH_OBJS}.
#	Add '-ldb_tcl-5.X' to tclsh target before -ltcl8.5....
# Should look something like this:
# ${CC} ${LDFLAGS} ${TCLSH_OBJS} -L/usr/local/BerkeleyDB.5.0/lib -L/users/sue/src/tcl8.5.8/unix -ldb_tcl-5.0 -ltcl8.5 ${LIBS} \
#                ${CC_SEARCH_FLAGS} -o tclsh
#
#	May want to switch CFLAGS to CFLAGS_DEBUG.
#	Add -pg to CFLAGS.
#	Add -pthread to CFLAGS if it isn't already there.
#	Need to add '-static -pg' to LDFLAGS.
#	Change LDFLAGS to use $(LDFLAGS_DEBUG) instead of OPTIMIZE if needed.
#	Change TCL_LIB_FILE to '.a' from '.so' if needed
#
# 4. Add Db_tcl_Init call to tclAppInit.c and an extern:
#====================
#*** tclAppInit.c.orig	Mon Mar 17 12:15:42 2008
#--- tclAppInit.c	Mon Mar 17 12:15:23 2008
#***************
#*** 30,35 ****
#--- 30,37 ----
#  
#  #endif /* TCL_TEST */
#  
#+ extern int		Db_tcl_Init _ANSI_ARGS_((Tcl_Interp *interp));
#+ 
#  #ifdef TCL_XT_TEST
#  extern void		XtToolkitInitialize _ANSI_ARGS_((void));
#  extern int		Tclxttest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#***************
#*** 145,150 ****
#--- 147,153 ----
#              Procbodytest_SafeInit);
#  #endif /* TCL_TEST */
#  
#+     Db_tcl_Init(interp);
#      /*
#       * Call the init procedures for included packages.  Each call should
#       * look like this:
#
#====================
# 5. Build tclsh with 'make' but I do NOT suggest 'make install'.
#
#	Test it has BDB built-in properly:
#	Run ./tclsh in Tcl src (unix) directory:
#	% berkdb version
#	[Should show current BDB version.]
#	% ^D
#
#	Current directory should now have a tclsh.gmon or gmon.out file.
#
#
# 6. Edit build_unix/include.tcl to point to profiled tclsh and
#    the static DB library:
#
# set tclsh_path <path to tclsrc>/tcl8.5.8/unix/tclsh
# set tcllib .libs/libdb_tcl-5.X.a
#
# 7. Comment out 'load $tcllib' in test/test.tcl
#
# 8. Run *your newly generated, profiled* tclsh as you normally would,
# including 'source ../test/test.tcl'
#	build_unix% <path to tclsrc>/unix/tclsh
#	% source ../test/test.tcl
#
# 9. Each test will be run in a separate tclsh and profiled individually.
# In the 'build_unix' directory you'll then find a <testname>.OUT file
# that contains the profile output.  Run:
# % run_rep_gprof [start reptest name]
# 	This form runs all rep tests, starting with the given
#	reptest name, or rep001 if no name is given.
# % run_gprof <testname> 
#	This form runs only the (required) specified test.
#	NOTE:  This form can be used on any individual test, not
#	just replication tests.  However, it uses 'run_test' so it
#	must be a test that can be run through all the methods.
#
proc run_rep_gprof { {starttest rep001} } {
	global test_names

	set tindex [lsearch $test_names(rep) $starttest]
	if { $tindex == -1 } {
		set tindex 0
	}
	set rlist [lrange $test_names(rep) $tindex end]
	run_gprof_int $rlist
}

proc run_gprof { testname } {
	global test_names

	set rlist [list $testname]
	run_gprof_int $rlist
}

proc run_gprof_int { rlist } {
	global one_test
	source ./include.tcl

	foreach test $rlist {
		puts "Test $test start: [timestamp]"
		fileremove -f $test.OUT
		if [catch {exec $tclsh_path << \
		    "global one_test; set one_test $one_test; \
		    source $test_path/test.tcl; run_test $test" \
		    >>& ALL.OUT } res] {
			set o [open ALL.OUT a]
			puts $o "FAIL: run_gprof_int $test: $res"
			close $o
		}
		puts "Test $test gprof: [timestamp]"
		set gmonfile NULL
		set known_gmons { tclsh.gmon gmon.out }
		foreach gmon $known_gmons {
			if { [file exists $gmon] } {
				set gmonfile $gmon
				break
			}
		}
		if { $gmonfile != "NULL" } {
			set stat [catch {exec gprof $tclsh_path $gmonfile \
			    >>& $test.OUT} ret]
		} else {
			puts "FAIL:  Could not find execution profile in \
			   either tclsh.gmon or gmon.out."
		}
		puts "Test $test complete: [timestamp]"
	}
}

#
# Make a DB_CONFIG file for a site about to run a db_replicate test.
# Args are 
# sitedir - the directory for this site
# i - my site index/number
# pri - my priority
#
proc replicate_make_config { sitedir i pri } {
	#
	# Generate global config values that should be the same
	# across all sites, such as number of sites and log size, etc.
	#
	set default_cfglist {
	{ "set_flags" "DB_TXN_NOSYNC" }
	{ "rep_set_request" "150000 2400000" }
	{ "rep_set_timeout" "db_rep_checkpoint_delay 0" }
	{ "rep_set_timeout" "db_rep_connection_retry 2000000" }
	{ "rep_set_timeout" "db_rep_heartbeat_monitor 5000000" }
	{ "rep_set_timeout" "db_rep_heartbeat_send 1000000" }
	{ "set_cachesize"  "0 4194304 1" }
	{ "set_lk_detect" "db_lock_default" }
	{ "rep_set_config" "db_repmgr_conf_2site_strict" }
	}

	#
	# Otherwise set up per-site config information
	#
	set cfglist $default_cfglist

	set litem [list rep_set_priority $pri]
	lappend cfglist $litem

	#
	# Now set up the local and remote ports.  Use 49210 so that
	# we don't collide with db_reptest which uses 49200.  For
	# now, we have site 0 know about no one, and all other sites
	# know about site 0.  Do not use peers for now.
	#
	set baseport 49210
	set rporttype NULL
	set lport [expr $baseport + $i]
	if { $i == 0 } {
		set creator_flag "db_group_creator on"
	} else {
		set creator_flag ""
	}
	set litem [list repmgr_site \
		       "localhost $lport $creator_flag db_local_site on"]
	lappend cfglist $litem
	set peers 0
	set p NULL
	if { $i != 0 } {
		set p $baseport
	}
	if { $peers } {
		set remote_arg "db_repmgr_peer on"
	} else {
		set remote_arg ""
	}
	if { $p != "NULL" } {
		set litem [list repmgr_site \
		    "localhost $p $remote_arg db_bootstrap_helper on"]
		lappend cfglist $litem
	}
	#
	# Now write out the DB_CONFIG file.
	#
	set cid [open $sitedir/DB_CONFIG a]
	foreach c $cfglist {
		set carg [subst [lindex $c 0]]
		set cval [subst [lindex $c 1]]
		puts $cid "$carg $cval"
	}
	close $cid
}

#
# Proc for checking exclusive access to databases on clients.
#
proc rep_client_access { env testfile result } {
	set t [$env txn -nowait]
	set ret [catch {eval {berkdb_open} -env $env -txn $t $testfile} res]
	$t abort
	if { $result == "FAIL" } {
		error_check_good clacc_fail $ret 1
		error_check_good clacc_fail_err \
		    [is_substr $res "DB_LOCK_NOTGRANTED"] 1
	} else {
		error_check_good clacc_good $ret 0
		error_check_good clacc_good1 [is_valid_db $res] TRUE
		error_check_good clacc_close [$res close] 0
	}
}

#
# View function for replication.
# This function always returns 0 and does not replicate any database files.
#
proc replview_none { name flags } {
	# Verify flags are always 0 - "none" in Tcl.
#	puts "Replview_none called with $name, $flags"
	set noflags [string compare $flags "none"]
	error_check_good chkflags $noflags 0

	# Verify we never get a BDB owned file.
	set bdbfile "__db"
	set prefix_len [string length $bdbfile]
	incr prefix_len -1
	set substr [string range $name 0 $prefix_len]
	set res [string compare $substr $bdbfile]
	error_check_bad notbdbfile $res 0

	#
	# Otherwise this proc always returns 0 to say we do not want the file.
	#
	return 0
}

#
# View function for replication.
# This function returns 1 if the name has an odd digit in it, and 0
# otherwise.
#
proc replview_odd { name flags } {
#	puts "Replview_odd called with $name, $flags"

	# Verify we never get a BDB owned file.
	set bdbfile "__db"
	set prefix_len [string length $bdbfile]
	incr prefix_len -1
	set substr [string range $name 0 $prefix_len]
	set res [string compare $substr $bdbfile]
	error_check_bad notbdbfile $res 0

	#
	# Otherwise look for an odd digit.
	#
	set odd [string match "*\[13579\]*" $name]
	return $odd
}

#
# Determine whether current version of Berkeley DB has group membership.
# This function returns 1 if group membership is supported, and 0
# otherwise.
#
proc have_group_membership { } {
	set bdbver [berkdb version]
	set vermaj [lindex $bdbver 0]
	set vermin [lindex $bdbver 1]
	if { $vermaj >= 6 } {
		return 1
	} elseif { $vermaj >= 5 && $vermin >= 2 } {
		return 1
	} else {
		return 0
	}
}

#
# Create an empty marker file.  The upgrade tests use marker files to
# synchronize between their different processes.
#
proc upgrade_create_markerfile { filename } {
	if [catch {open $filename { RDWR CREAT } 0777} markid] {
		puts "problem opening marker file $markid"
	} else {
		close $markid
	}
}

proc upgrade_setup_sites { nsites } {
	#
	# Set up a list that goes from 0 to $nsites running
	# upgraded.  A 0 represents running old version and 1
	# represents running upgraded.  So, for 3 sites it will look like:
	# { 0 0 0 } { 1 0 0 } { 1 1 0 } { 1 1 1 }
	#
	set sitelist {}
	for { set i 0 } { $i <= $nsites } { incr i } {
		set l ""
		for { set j 1 } { $j <= $nsites } { incr j } {
			if { $i < $j } {
				lappend l 0
			} else {
				lappend l 1
			}
		}
		lappend sitelist $l
	}
	return $sitelist
}

proc upgrade_one_site { histdir upgdir } {
	global util_path

	#
	# Upgrade a site to the current version.  This entails:
	# 1.  Removing any old files from the upgrade directory.
	# 2.  Copy all old version files to upgrade directory.
	# 3.  Remove any __db files from upgrade directory except __db.rep*gen.
	# 4.  Force checkpoint in new version.
	file delete -force $upgdir

	# Recovery was run before as part of upgradescript.
	# Archive dir by copying it to upgrade dir.
	file copy -force $histdir $upgdir
	set dbfiles [glob -nocomplain $upgdir/__db*]
	foreach d $dbfiles {
		if { $d == "$upgdir/__db.rep.gen" ||
		    $d == "$upgdir/__db.rep.egen" ||
		    $d == "$upgdir/__db.rep.system" } {
			continue
		}
		file delete -force $d
	}
	# Force current version checkpoint
	set stat [catch {eval exec $util_path/db_checkpoint -1 -h $upgdir} r]
	if { $stat != 0 } {
		puts "CHECKPOINT: $upgdir: $r"
	}
	error_check_good stat_ckp $stat 0
}

proc upgrade_get_master { nsites verslist } {
	error_check_good vlist_chk [llength $verslist] $nsites
	#
	# When we can, simply run an election to get a new master.
	# We then verify we got an old client.
	#
	# For now, randomly pick among the old sites, or if no old
	# sites just randomly pick anyone.
	#
	set old_count 0
	# Pick 1 out of N old sites or 1 out of nsites if all upgraded.
	foreach i $verslist {
		if { $i == 0 } {
			incr old_count
		}
	}
	if { $old_count == 0 } {
		set old_count $nsites
	}
	set master [berkdb random_int 0 [expr $old_count - 1]]
	#
	# Since the Nth old site may not be at the Nth place in the
	# list unless we used the entire list, we need to loop to find
	# the right index to return.
	if { $old_count == $nsites } {
		return $master
	}
	set ocount 0
	set index 0
	foreach i $verslist {
		if { $i == 1 } {
			incr index
			continue
		}
		if { $ocount == $master } {
			return $index
		}
		incr ocount
		incr index
	}
	#
	# If we get here there is a problem in the code.
	#
	error "FAIL: upgrade_get_master problem"
}

# Shared upgrade test script procedure to execute rep_test_upg on a master.
proc upgradescr_reptest { repenv oplist markerdir } {

	set method [lindex $oplist 1]
	set niter [lindex $oplist 2]
	set loop [lindex $oplist 3]
	set start 0
	puts "REPTEST: method $method, niter $niter, loop $loop"

	for {set n 0} {$n < $loop} {incr n} {
		puts "REPTEST: call rep_test_upg $n"
		eval rep_test_upg $method $repenv NULL $niter $start $start 0 0
		incr start $niter
		tclsleep 3
	}
	#
	# Sleep a bunch to help get the messages worked through.
	#
	tclsleep 10
	puts "create DONE marker file"
	upgrade_create_markerfile $markerdir/DONE
}

# Shared upgrade test script procedure to perform db_gets on a client.
proc upgradescr_repget { repenv oplist mydir markerdir } {
	set dbname "$mydir/DATADIR/test.db"
	set i 0
	while { [file exists $dbname] == 0 } {
		tclsleep 2
		incr i
		if { $i >= 15 && $i % 5 == 0 } {
			puts "After $i seconds, no database $dbname exists."
		}
		if { $i > 180 } {
			error "Database $dbname never created."
		}
	}
	set loop 1
	while { [file exists $markerdir/DONE] == 0 } {
		set db [berkdb_open -env $repenv $dbname]
		error_check_good dbopen [is_valid_db $db] TRUE
		set dbc [$db cursor]
		set i 0
		error_check_good curs [is_valid_cursor $dbc $db] TRUE
		for { set dbt [$dbc get -first ] } \
		    { [llength $dbt] > 0 } \
		    { set dbt [$dbc get -next] } {
			incr i
		}
		error_check_good dbc_close [$dbc close] 0
		error_check_good db_close [$db close] 0
		puts "REPTEST_GET: after $loop loops: key count $i"
		incr loop
		tclsleep 2
	}
}

# Shared upgrade test script procedure to verify dbs and logs.
proc upgradescr_verify { oplist mydir rep_env_cmd } {
	global util_path

	# Change directories to where this will run.
	# !!!
	# mydir is an absolute path of the form
	# <path>/build_unix/TESTDIR/MASTERDIR or
	# <path>/build_unix/TESTDIR/CLIENTDIR.0
	#
	# So we want to run relative to the build_unix directory
	cd $mydir/../..

	foreach op $oplist {
		set repenv [eval $rep_env_cmd]
		error_check_good env_open [is_valid_env $repenv] TRUE
		if { $op == "DB" } {
			set dbname "$mydir/DATADIR/test.db"
			puts "Open db: $dbname"
			set db [berkdb_open -env $repenv -rdonly $dbname]
			error_check_good dbopen [is_valid_db $db] TRUE
			set txn ""
			set method [$db get_type]
			set dumpfile "$mydir/VERIFY/dbdump"
			if { [is_record_based $method] == 1 } {
				dump_file $db $txn $dumpfile \
				    rep_test_upg.recno.check
			} else {
				dump_file $db $txn $dumpfile \
				    rep_test_upg.check
			}
			puts "Done dumping $dbname to $dumpfile"
			error_check_good dbclose [$db close] 0
		}
		if { $op == "LOG" } {
			set lgstat [$repenv log_stat]
			set lgfile [stat_field $repenv log_stat "Current log file number"]
			set lgoff [stat_field $repenv log_stat "Current log file offset"]
			puts "Current LSN: $lgfile $lgoff"
			set f [open $mydir/VERIFY/loglsn w]
			puts $f $lgfile
			puts $f $lgoff
			close $f

			set stat [catch {eval exec $util_path/db_printlog \
			    -h $mydir > $mydir/VERIFY/prlog} result]
			if { $stat != 0 } {
				puts "PRINTLOG: $result"
			}
			error_check_good stat_prlog $stat 0
		}
		error_check_good envclose [$repenv close] 0
	}
	#
	# Run recovery locally so that any later upgrades are ready
	# to be upgraded.
	#
	set stat [catch {eval exec $util_path/db_recover -h $mydir} result]
	if { $stat != 0 } {
		puts "RECOVERY: $result"
	}
	error_check_good stat_rec $stat 0

}
