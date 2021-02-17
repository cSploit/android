# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test142
# TEST  Tests exclusive database handles.
# TEST 1. Test that exclusive database handles return an error in incompatible 
# TEST   environments.
# TEST 2. Test that an exclusive subdatabase handle does not block opening a 
# TEST   handle on another database in the same file.
# TEST 3. Run script tests on a subdatabase with both wait and no wait 
# TEST   configuration.
# TEST 4. Run script tests on a database with both wait and no wait 
# TEST   configuration.
proc test142 {method {tnum "142"} args } {
	source ./include.tcl
	source $tcl_utils/multi_proc_utils.tcl

	# Skip this test if threads are not enabled
	if [catch {package require Thread}] {
		puts "Skipping test$tnum: requires Tcl Thread package."
		return 0
	}

	set script1 [test142_script1]
	set script2 [test142_script2]
	set nowaits {0 1}
	set omethod [convert_method $method]
 	set args [convert_args $method $args] 
	set testfile test$tnum.db
 	set exclfile excl.db
	set cryargs {}
	set homedir $testdir

	puts "Test$tnum: Exclusive database handles ($method $args)" 
 
	# Check if we are using a transactional environment.
	set eindex [lsearch $args "-env"]
	set txnenv 1
	set threaded 0
	set opened 0    
	set env 0
	set args2 $args
	set noerr_env 0    
	if { $eindex != -1 } {
		set env [lindex $args $eindex+1]
		set args2 [lreplace $args $eindex $eindex+1]
		set txnenv [is_txnenv $env]
		set sys [$env get_open_flags]
		if { [lsearch $sys -thread] != -1 } {
			set threaded 1
		}
		set homedir [$env get_home]
	} else {
		set eindex [lsearch $args "-encryptaes"]
	    	if { $eindex != -1 } {
	    		set cryargs [lrange $args $eindex $eindex+1]
	    		set args [lreplace $args $eindex $eindex+1]
	    		set eindex [lsearch $args2 "-encryptaes"]
	    		set args2 [lreplace $args2 $eindex $eindex+1]
	    		lappend args -encrypt
	    		lappend args2 -encrypt
		}
		set opened 1
		set env [eval {berkdb_env_noerr -create -txn \
		    -cachesize { 0 1048576 1 }} $cryargs -home $homedir]
		error_check_good envopen [is_valid_env $env] TRUE
		lappend args -env
		lappend args $env
		set noerr_env 1
	}
	cleanup $homedir $env

	# Check that opening the database exclusively fails
	# on threaded or non-transactional environments
	if { $txnenv == 0 || $threaded == 1 } {
		puts "\tTest$tnum.a: Exclusive open fails with threaded or non-txn env." 
		foreach nowait $nowaits {
			set ret [catch {eval {berkdb_open_noerr -create \
			    -mode 0644 $omethod -lk_exclusive $nowait} $args \
			    {$testfile}} db]
			error_check_bad dbopen $ret 0
		}
		return
	}

    	if { $noerr_env } {
		puts "\tTest$tnum.b: Exclusive databases can have only 1 active txn."
 		set db [eval {berkdb_open_noerr -create -mode 0644 \
		    -auto_commit $omethod -lk_exclusive 0} $args \
		    ./multitxn.db ]
		error_check_good dbopen [is_valid_db $db] TRUE
		set txn1 [$env txn]
		set txn2 [$env txn]
		set did [open $dict]
 		set key ""
		gets $did str
		if { [is_record_based $omethod] == 1 } {
	    	    	set key 1
        	} else {
	    		set key $str
		}
 		set ret [eval {$db put} -txn $txn1 \
		    {$key [chop_data $omethod $str]}]
		error_check_good multi_txn $ret 0
        	catch { eval {$db put} -txn $txn2 \
		    {$key [chop_data $omethod $str]}} ret
		error_check_bad multi_txn $ret 0
 		$txn2 abort
		$txn1 commit
		$db close
    	}

	# Queue and heap databases do not support multiple databases
	# in a single file.
	if { ![is_queue $omethod] && ![is_heap $omethod] && 
	    ![is_partitioned $args]} {
		# Create a database with sub databases.
		puts "\tTest$tnum.c: Open a database with subdbs." 
		set ret [catch {eval {berkdb_open} -create -auto_commit \
		    -mode 0644 $omethod $args $exclfile one} db1]
		error_check_good db1open $ret 0
		set ret [catch {eval {berkdb_open} -create -auto_commit \
		    -mode 0644 $omethod $args $exclfile two} db2]
		error_check_good db2open $ret 0
		$db1 close
		$db2 close

		# Check that and exclusive subdatabase handle does not 
		# interfere with opening another subdatabase in the same file 
		foreach nowait $nowaits {
			puts "\tTest$tnum.d: Open one subdb exclusively, another not, for nowait value $nowait."
			set ret [catch {eval {berkdb_open $omethod \
			    -lk_exclusive $nowait} \
			    $args -auto_commit $exclfile one} db1]
 			error_check_good db1exclopen$nowait $ret 0
			set ret [catch {eval {berkdb_open $omethod} \
			    $args -auto_commit $exclfile two} db2]
			error_check_good db2exclopen$nowait $ret 0
			$db1 close
			$db2 close
		}

		if { [is_repenv $env] } {
 			puts "\tTest$tnum.e Skipping scripts in replication environment."
			return;		    
		}
	
		# Run the scripts with the sub databases
		foreach nowait $nowaits {
 			puts "\tTest$tnum.f: Test scripts with subdatabases, for nowait value $nowait." 
			set myports [available_ports 2]
			set myPort1 [lindex $myports 0]
			set myPort2 [lindex $myports 1]
			set arg_list1 [list $omethod $nowait $args2 \
			    $exclfile $cryargs $myPort1 $myPort2 \
			    $homedir one]
			set arg_list2 [list $omethod $nowait $args2 \
			    $exclfile $cryargs $myPort2 $myPort1 \
			    $homedir one]
			do_multi_proc_test test${tnum}excl1_$nowait \
			    [list $script1 $script2] \
			    [list $arg_list1 $arg_list2]
		}
	}

	if { [is_repenv $env] } {
		puts "\tTest$tnum.g Skipping scripts in replication environment."
		return;		    
	}

	# Run the scripts with the passed in database files
	foreach nowait $nowaits {
		puts "\tTest$tnum.h: Test scripts with passed-in db files, for nowait value $nowait." 
		set myports [available_ports 2]
		set myPort1 [lindex $myports 0]
		set myPort2 [lindex $myports 1]
		set arg_list1 [list $omethod $nowait $args2 $testfile \
		    $cryargs $myPort1 $myPort2 $homedir]
		set arg_list2 [list $omethod $nowait $args2 $testfile \
		    $cryargs $myPort2 $myPort1 $homedir]
		do_multi_proc_test test${tnum}excl2_$nowait \
		    [list $script1 $script2] [list $arg_list1 $arg_list2]
	}

	if { $opened } {
		$env close
	}
}

# Script 1
# 1. Opens an exclusive database handle
# 2. Populates the database.
# 3. Confirms that Script 2 is blocked trying to open a handle on
#   the same database.
# 4. Close the exclusive handle.
# 5. Try reopening the exclusive handle, which will fail if nowait is
#   true, and will succeed after blocking if it is false.
proc test142_script1 {} {
	set script1 {
		source ./include.tcl
		source $test_path/test.tcl
		source $test_path/testutils.tcl
		source $tcl_utils/multi_proc_utils.tcl
	
		set usage \
		"script omethod nowait args testfile cyrargs myPort clientPort homedir databaseName"

		# Verify usage
		set cmd_args [lindex $argv 0]
		if { [llength $cmd_args] < 8 } {
			puts stderr "FAIL:[timestamp] Usage: $usage"
			exit
		}
		set omethod [lindex $cmd_args 0]
		set nowait [lindex $cmd_args 1]
		set args [lindex $cmd_args 2]
		set testfile [lindex $cmd_args 3]
		set cryargs [lindex $cmd_args 4]
		set myPort [lindex $cmd_args 5]
		set clientPort [lindex $cmd_args 6]
		set homedir [lindex $cmd_args 7]
		set databasename ""	    
		if { [llength $cmd_args] > 8 } {
			set databasename [lindex $cmd_args 8]
		}
		set timeout 10

		# Join the environment
		puts "Joining the environment in $homedir."
		set dbenv [eval {berkdb_env -txn} $cryargs -home $homedir]
		error_check_good envopen [is_valid_env $dbenv] TRUE

		# open the exclusive database handle.
		puts "Opening the exclusive database handle."
		set db [eval berkdb_open -create -mode 0644 -auto_commit \
		    $omethod -lk_exclusive $nowait -env $dbenv $args \
		    $testfile $databasename]
		error_check_good dbopen [is_valid_db $db] TRUE

		set ret [do_sync $myPort $clientPort $timeout]
		if { $ret != 0 } {
			puts stderr "FAIL: Synchronization failed."
			$db close
			$dbenv close
			exit -1
		}

		# Check that inserting works.
		puts "Populating the database."
		set t [$dbenv txn]
		populate $db $omethod $t 10 0 0

		# close the database handle, this will not release
 		# the handle lock, because the transaction is holding it
		puts "Closing the exclusive database handle."
		$db close

		# This sync will fail because script2 is blocked trying
		# to open a handle on the exclusive database.
		puts "Confirming script 2 is blocked."
		set ret [do_sync $myPort $clientPort $timeout]	
		if { $ret == 0 } {
			puts stderr \
		"FAIL: Synchronization succeeded where it should have failed."
			$db close
			$dbenv close
			exit -1
		}
		# Now commit the transaction, releaseing the handle lock and
		# allowing the script 2 to proceed.
		error_check_good commit [eval {$t commit}] 0
		set ret [do_sync $myPort $clientPort $timeout]
		if { $ret != 0 } {
			puts stderr "FAIL: Synchronization failed."
			$dbenv close
			exit -1
		}

		puts "Opening another exclusive database handle."
		set ret [catch {eval berkdb_open_noerr -auto_commit \
		    -lk_exclusive $nowait $omethod -env $dbenv $args \
		    $testfile $databasename} db]
		if { $nowait == 1 } {
			error_check_bad dbopenwait $ret 0
			# wakeup script2
			set ret [do_sync $myPort $clientPort $timeout]
			if { $ret != 0 } {
				puts stderr "FAIL: Synchronization failed."
				$dbenv close
				exit -1
			}
		} else {
			error_check_good dbopennowait $ret 0
			$db close
			set ret [do_sync $myPort $clientPort $timeout]
			if { $ret == 0 } {
				puts stderr \
		"FAIL: Synchronization succeeded where it should have failed."
				$dbenv close
				exit -1
			}
		}

		$dbenv close
	}

	# Do not put anything here, this proc depends on the 
	# return of set script1
}

# Script 2
# 1. Lets Script 1 open an exclusive handle on the database.
# 2. Opens a handle on the database, this blocks until Script 1 closes
#   its exclusive handle.
# 3. Populates the database.
# 4. Tries to synchronize with Script 1, which will fail if nowait is 
#  false because Script 1 is blocked trying to open an exclusive handle
#  on the database, and will succeed if nowait is true the exclusive
#  handle in Script 1 would have returned an error immediately.
proc test142_script2 {} {
	set script2 {
		source ./include.tcl
		source $test_path/test.tcl
		source $test_path/testutils.tcl
		source $tcl_utils/multi_proc_utils.tcl

		set usage \
	"script omethod nowait args testfile myPort clientPort homedir databaseName"

		# Verify usage
		set cmd_args [lindex $argv 0]
		if { [llength $cmd_args] < 8 } {
			puts stderr "FAIL:[timestamp] Usage: $usage"
			exit
		}
		set omethod [lindex $cmd_args 0]
		set nowait [lindex $cmd_args 1]
		set args [lindex $cmd_args 2]
		set testfile [lindex $cmd_args 3]
		set cryargs [lindex $cmd_args 4]
		set myPort [lindex $cmd_args 5]
		set clientPort [lindex $cmd_args 6]
		set homedir [lindex $cmd_args 7]
		set databasename ""
		if { [llength $cmd_args] > 8 } {
			set databasename [lindex $cmd_args 8]
		}
		set timeout 10

		# Wait for script1 to open the exlusive database
		puts "Waiting for script 1 to open the exclusive database."
		set ret [do_sync $myPort $clientPort $timeout]
		if { $ret != 0 } {
			puts stderr "FAIL: Synchronization failed."
			exit
		}
	
		# Join the environment
		puts "Opening the environment in $homedir."
 		set dbenv [eval {berkdb_env -txn} $cryargs -home $homedir]
		error_check_good envopen [is_valid_env $dbenv] TRUE

		# This will block until script 1 closes the exclusive database
		puts "Opening the database."
		set db [eval berkdb_open -auto_commit $omethod -env $dbenv \
		    $args $testfile $databasename]
		error_check_good dbopen [is_valid_db $db] TRUE
		set db [eval berkdb_open -auto_commit \
		    $omethod -env $dbenv $args $testfile $databasename]
		error_check_good dbopen [is_valid_db $db] TRUE

		# Wakeup script1
		set ret [do_sync $myPort $clientPort $timeout]
		if { $ret != 0 } {
			puts stderr "FAIL: Synchronization failed."
			$db close
			$dbenv close
			exit -1
		}

		# Check that inserting works.
		puts "Populating the database."
		set t [$dbenv txn]
		populate $db $omethod $t 10 0 0
		error_check_good commit [eval {$t commit}] 0
	
		# This will succeed if nowait == 1, and fail otherwise
		set ret [do_sync $myPort $clientPort $timeout]
		if { $nowait == 1 } {
			if { $ret != 0 } {
				puts stderr "FAIL: Synchronization failed."
				$db close
				$dbenv close
				exit -1
			}
		} elseif { $ret == 0 } {
			puts stderr \
		"FAIL: Synchronization succeeded when it should have failed."
			$db close
			$dbenv close
			exit -1
		}

		$db close
		$dbenv close
	}

	# Do not put anything here, this proc depends on the 
	# return of set script2
}
