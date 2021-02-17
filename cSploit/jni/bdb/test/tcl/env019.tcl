# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	env019
# TEST	Test that stats are correctly set and reported when 
# TEST  an env is accessed from a second process.

proc env019 { } {
	source ./include.tcl

	puts "Env019:  Test stats when env is joined from a second process."

	# Set up test cases.
	# The first two elements are the flag and value for the original
	# env.  The third and fourth elements are the flag and value for
	# the second process.
	#
	set testcases {
		{ "-cachesize" "{0 1048576 1}" "-cachesize" "{0 2097152 1}" }
		{ "-cachesize" "{0 2097152 1}" "-cachesize" "{0 1048576 1}" }
		{ "-cachesize" "{0 2097152 1}" "-cachesize" "{0 2097152 1}" }
		{ "-cachesize" "{0 1048576 1}" {} {} }
	}

	foreach case $testcases {

		env_cleanup $testdir

		# Extract the values 
		set flag1 [ lindex $case 0 ] 
		set value1 [ lindex $case 1 ]
		set flag2 [ lindex $case 2 ] 
		set value2 [ lindex $case 3 ]
		
		# Set up the original env.
		set e [eval {berkdb_env -create -home $testdir} $flag1 $value1 ]
		error_check_good dbenv [is_valid_env $e] TRUE

		# Start up a second process to join the env. 
		puts "$tclsh_path $test_path/wrap.tcl env019script.tcl\
		    $testdir/env019.log $flag2 $value2 &"
		set pid [exec $tclsh_path $test_path/wrap.tcl env019script.tcl\
		    $testdir/env019.log $flag2 $value2 &]

		watch_procs $pid 5

		# Read the value observed by the second process.
		set db [eval berkdb_open -env $e -btree values.db]
		set get_gbytes [lindex [lindex [$db get gbytes] 0] 1]
		set get_bytes [lindex [lindex [$db get bytes] 0] 1]
		set get_ncache [lindex [lindex [$db get ncache] 0] 1]

		# Now get the answer from memp_stat.
		set set_gbytes [stat_field $e mpool_stat "Cache size (gbytes)"]
		set set_bytes [stat_field $e mpool_stat "Cache size (bytes)"]
		set set_ncache [stat_field $e mpool_stat "Number of caches"]

		error_check_good gbytes [string equal $get_gbytes $set_gbytes] 1
		error_check_good bytes [string equal $get_bytes $set_bytes] 1
		error_check_good ncache [string equal $get_ncache $set_ncache] 1
		error_check_good db_close [$db close] 0
		error_check_good env_close [$e close] 0
	}

}
