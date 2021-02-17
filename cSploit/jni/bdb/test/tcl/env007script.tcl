# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# env007script - for use with env007.
# Usage: configarg configval getter getval
# 

source ./include.tcl

set usage "env007script configarg configval getter getval"

# Verify usage
if { $argc != 4 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	exit
}

# Initialize arguments
set configarg [lindex $argv 0]
set configval [lindex $argv 1]
set getter [lindex $argv 2]
set getval [lindex $argv 3]
 
set e "berkdb_env_noerr -create -mode 0644 -home $testdir -txn"

env007_make_config $configarg $configval

# Verify using config file
set dbenv [eval $e]
error_check_good envvalid:1 [is_valid_env $dbenv] TRUE
set db [berkdb_open_noerr -create -env $dbenv -btree env007script.db]
error_check_good dbvalid:1 [is_valid_db $db] TRUE
error_check_good dbclose [$db close] 0
error_check_good getter:1 [eval $dbenv $getter] $getval
error_check_good envclose:1 [$dbenv close] 0

