# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# For use with env019, a test of stat values returned by 
# different processes accessing the same env.

source ./include.tcl
source $test_path/test.tcl

set usage "env019script flag value"

# Verify usage
if { $argc != 2 } {
	puts stderr "FAIL:[timestamp] Usage: $usage"
	puts $argc
	exit
}

# Initialize arguments
set flag [ lindex $argv 0 ]
set value [ lindex $argv 1 ]

# Join the env, trying to set the flag values
set e [eval {berkdb_env -home $testdir} $flag $value]
error_check_good env_open [is_valid_env $e] TRUE

# Get the settings for the env.  This second process should
# not be able to override the settings created when the env
# was originally opened.
set gbytes [stat_field $e mpool_stat "Cache size (gbytes)"]
set bytes [stat_field $e mpool_stat "Cache size (bytes)"]
set ncache [stat_field $e mpool_stat "Number of caches"]

#puts "gbytes is $gbytes"
#puts "bytes is $bytes"
#puts "ncache is $ncache"

# Store the values so the first process can inspect them. 
set db [berkdb_open -create -env $e -btree values.db]
error_check_good put_gbytes [eval {$db put} "gbytes" $gbytes] 0
error_check_good put_bytes [eval {$db put} "bytes" $bytes] 0
error_check_good put_ncache [eval {$db put} "ncache" $ncache] 0
error_check_good db_close [$db close] 0

# Close environment system
error_check_good env_close [$e close] 0
exit
