# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	repmgr150
# TEST  Test repmgr with DB_REGISTER, DB_RECOVER, and FAILCHK
# TEST
# TEST  1. RepMgr can be started with -register and -recovery flags. 
# TEST
# TEST  2. A rep unaware process can join the master environment 
# TEST  with -register and -recovery without running recovery. 
# TEST
# TEST  3. RepMgr can be started with -register and -recovery flags, 
# TEST  even if the environment is corrupted.
# TEST
# TEST  4. RepMgr can be started with -failchk and -isalive. 
# TEST
# TEST  5. A rep unaware process can join the master environment 
# TEST  with -failchk and -isalive.

proc repmgr150 { } {
	source ./include.tcl
	source $tcl_utils/multi_proc_utils.tcl

	set tnum "150"
	puts "Repmgr$tnum: Repmgr, DB_REGISTER, DB_RECOVER, and FAILCHK"
	env_cleanup $testdir

	# Skip this test if threads are not enabled.
	if [catch {package require Thread}] {
		puts "Skipping Repmgr$tnum: requires Tcl Thread package."
		return 0
	}

	if { $is_freebsd_test == 1 } {
		puts "Skipping replication manager test on FreeBSD platform."
		return
	}

	set masterdir $testdir/MASTERDIR
	set clientdir $testdir/CLIENTDIR

	file mkdir $masterdir
	file mkdir $clientdir

	set ports [available_ports 2]
	set master_port [lindex $ports 0]
	set client_port [lindex $ports 1]
	set filename "test.db"
	set db_name "test"

	# The script is used to execute operations on the environment
	# and database in a separate process, in order to corrupt the
	# environment, and to run failchk and recover on the environment.
	# The script always opens the environment and database, and depending
	# on the command line arguments, will insert, read, run recovery, run 
	# failchk, and corrupt the environment.
	set script [repmgr150_script]

	puts "\tRepmgr$tnum.a: Start the HA sites with recovery and register."
	# Start the master with recovery and register
	set masterenv [berkdb_env_noerr -create -thread -txn -home $masterdir \
	    -errpfx MASTER -rep -recover -register]
	error_check_good master_open [is_valid_env $masterenv] TRUE
	$masterenv repmgr -ack all -local [list 127.0.0.1 $master_port] \
	    -start master

	# Start the client with recovery and register.
	set clientenv [berkdb_env_noerr -create -thread -txn -home $clientdir \
	    -errpfx CLIENT -rep -recover -register]
	error_check_good client_open [is_valid_env $clientenv] TRUE
	$clientenv repmgr -ack all -local [list 127.0.0.1 $client_port] \
	    -remote [list 127.0.0.1 $master_port] -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.b: Rep-unaware proc joins master env with recovery \
	    and register."
	# Run the script with commands to open the environment with register
	# and recovery, and to insert a value into the database then
	# exit cleanly.
	set value 1
	set args_list [list "-recover -register -create" 1 $value 0 $masterdir \
	    $filename $db_name]
	do_multi_proc_test Repmgr${tnum}.2 [list $script] [list $args_list]

	# Check that the changes done in the script have replicated to
	# the client.
	tclsleep 2
	set clientdb [eval berkdb_open -auto_commit -btree -env $clientenv \
	    $filename $db_name]
	error_check_good dbopen [is_valid_db $clientdb] TRUE
	set ret [$clientdb get $value]
	error_check_good clientdb_get $ret "{$value $value}"

	# Close the client and master sites.
	error_check_good clientdb_close [$clientdb close] 0
	error_check_good clientenv_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0

	puts "\tRepmgr$tnum.c: RepMgr can be started with -register and \
	    -recovery flags, even if the environment is corrupted"
	# Corrupt the environment executing the script with commands to write
	# to the master, then exit before committing the transaction.
	set value 2
	set args_list [list "-recover -register -create" 1 $value 1 $masterdir \
	    $filename $db_name]
	do_multi_proc_test Repmgr${tnum}.3 [list $script] [list $args_list]

	# Start the master with recovery and register, so it will see that
	# the last process died, and will run recovery.
	set masterenv [berkdb_env_noerr -create -thread -txn -home $masterdir \
	    -errpfx MASTER -rep -recover -register]
	error_check_good master_open [is_valid_env $masterenv] TRUE
	$masterenv repmgr -ack all -local [list 127.0.0.1 $master_port] -start master

	# Start the client.
	set clientenv [berkdb_env_noerr -create -thread -txn -home $clientdir \
	    -errpfx CLIENT -rep -recover -register]
	error_check_good client_open [is_valid_env $clientenv] TRUE
	$clientenv repmgr -ack all -local [list 127.0.0.1 $client_port] \
	    -remote [list 127.0.0.1 $master_port] -start client
	await_startup_done $clientenv

	# Check that the value inserted by the script before it died
	# mid-transaction has been rolled back.
	set masterdb [eval berkdb_open -auto_commit -btree -env $masterenv \
	    $filename $db_name]
	error_check_good dbopen [is_valid_db $masterdb] TRUE
	set ret [$masterdb get $value]
	error_check_good masterdb_get $ret ""

	# Close the master and client site, and delete them so the
	# environments can be re-created with failchk.
	error_check_good masterdb_close [$masterdb close] 0
	error_check_good clientenv_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
	env_cleanup $testdir

	file mkdir $masterdir
	file mkdir $clientdir

	puts "\tRepmgr$tnum.d: RepMgr can be started with -failchk and -isalive."
	# Start the master with failchk, isalive, recovery, and register.
	set env_args "-create -register -recover -failchk -isalive my_isalive"
	set masterenv [eval {berkdb_env -thread -rep -txn} -home $masterdir \
	    -errpfx MASTER  $env_args]
	error_check_good master_open [is_valid_env $masterenv] TRUE
	$masterenv repmgr -ack all -local [list 127.0.0.1 $master_port]\
	    -start master

	# Start the client with failchk, isalive, recovery, and register.
	set clientenv [eval {berkdb_env -thread -rep -txn -home} $clientdir \
	    -errpfx CLIENT $env_args]
	error_check_good client_open [is_valid_env $clientenv] TRUE
	$clientenv repmgr -ack all -local [list 127.0.0.1 $client_port] \
	    -remote [list 127.0.0.1 $master_port] -start client
	await_startup_done $clientenv

	puts "\tRepmgr$tnum.e: A rep unaware process can join the master \
	    environment with -failchk and -isalive."

	# Start the script with commands to open the environment with
	# failchk, isalive, recovery, and register, and to insert a
	# value into the database, and exit cleanly.
	set value 2
	set args_list [list $env_args 1 $value 0 $masterdir $filename $db_name]
	do_multi_proc_test Repmgr${tnum}.5 [list $script] [list $args_list]

	tclsleep 2
	# Check that the changes have replicated to the client.
	set clientdb [eval berkdb_open -auto_commit -btree -env $clientenv \
	    $filename $db_name]
	error_check_good dbopen [is_valid_db $clientdb] TRUE
	set ret [$clientdb get $value]
	error_check_good clientdb_get $ret "{$value $value}"
	error_check_good clientdb_close [$clientdb close] 0
	error_check_good clientenv_close [$clientenv close] 0
	error_check_good masterenv_close [$masterenv close] 0
}

# Script
#  1 Joins the master environment with some combination of recover, register,
#  isalive, and failchk.
#  2 Start a transaction.
#  3 Read or insert a given value.
#  4 Exit to corrupt the database, or commit and exit cleanly.
proc repmgr150_script {} {
    set script {
	source ./include.tcl
	source $test_path/test.tcl
	source $test_path/testutils.tcl
	source $tcl_utils/multi_proc_utils.tcl

	# Verify usage.
	set usage "script env_args write value corrupt homedir filename db_name."
	set cmd_args [lindex $argv 0]
	if { [llength $cmd_args] < 7 } {
		puts stderr "FAIL:[timestamp] Usage: $usage"
		exit
	}
	set env_args [lindex $cmd_args 0]
	set write [lindex $cmd_args 1]
	set val [lindex $cmd_args 2]
	set corrupt [lindex $cmd_args 3]
	set homedir [lindex $cmd_args 4]
	set filename [lindex $cmd_args 5]
	set db_name [lindex $cmd_args 6]

	# Open the environment and database.
	puts "Opening the environment in $homedir with arguments $env_args."
	set dbenv [eval {berkdb_env -txn -thread} $env_args \
	    -home $homedir]
	error_check_good envopen [is_valid_env $dbenv] TRUE

	puts "Opening the database $filename $db_name."
	set db [eval berkdb_open -create -mode 0644 -auto_commit \
	    -btree -env $dbenv $filename $db_name]
	error_check_good dbopen [is_valid_db $db] TRUE

	set txn [$dbenv txn]
	if { $write == 1 } {
		puts "Writing value $val."
		set ret [$db put -txn $txn $val $val]
		error_check_good db_put $ret 0
	} else {
		puts "Reading value $val."
		set ret [$db get -txn $txn $val]
		error_check_good db_get $ret "{$val $val}"
	}

	# Exit to corrupt the database and force recovery.
	if { $corrupt == 1 } {
		puts "Exiting to corrupt the environment."
		exit
	}
	
	set ret [$txn commit]
	error_check_good db_commit $ret 0

	catch { $db close }
	catch { $dbenv close }
    }
}
