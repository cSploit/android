# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# Repmgr028 script - subordinate repmgr processes and dynamic role changes

source ./include.tcl
source $test_path/test.tcl

# Make sure a subordinate process is not allowed to set the ELECTIONS config.
# 
set dbenv [berkdb_env_noerr -thread -home $testdir/SITE_A -txn -rep]
$dbenv repmgr -start elect
set ret [catch {$dbenv rep_config {mgrelections off}} result]
error_check_bad role_chg_attempt $ret 0
$dbenv close

puts "OK"
