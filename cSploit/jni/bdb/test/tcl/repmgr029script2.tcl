# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$

source ./include.tcl
source $test_path/test.tcl
source $test_path/testutils.tcl
source $test_path/reputils.tcl

set dirB [lindex $argv 0]
set portB [lindex $argv 1]
set dirA [lindex $argv 2]
set portA [lindex $argv 3]

puts "Repmgr029script2: Open env of A."
set ma_cmd "berkdb_env -home $dirA -txn -thread"
set envA [eval $ma_cmd]
error_check_good script_env_open [is_valid_env $envA] TRUE

puts "Repmgr029script2: Wait until there is an active txn"
set maxcount 100
for { set count 0 } { $count < $maxcount } { incr count } {
	set active_txn_1 [stat_field $envA txn_stat "Number active txns"]
	if { $active_txn_1 > 0 } {
		break
	} else {
		tclsleep 1
	}
}
error_check_good a_txn_A_1 $active_txn_1 1

puts "Repmgr029script2: Start up B. It finishes when the txn has been aborted."
set envB [berkdb env -create -errpfx B -home $dirB -txn -rep -thread]
$envB repmgr -local [list 127.0.0.1 $portB] -remote [list 127.0.0.1 $portA] \
    -start client
await_startup_done $envB
error_check_good a_txn_A_0 [stat_field $envA txn_stat "Number active txns"] 0

puts "Repmgr029script2: Check gmdb at site B"
error_check_good nsites_B [$envB rep_get_nsites] 2

error_check_good client_close [$envB close] 0
