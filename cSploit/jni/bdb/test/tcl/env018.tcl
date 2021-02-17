# See the file LICENSE for redistribution information.
#
# Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	env018
# TEST	Test getters when joining an env.  When a second handle is 
# TEST	opened on an existing env, get_open_flags needs to return 
# TEST	the correct flags to the second handle so it knows what sort
# TEST 	of environment it's just joined.
# TEST
# TEST	For several different flags to env_open, open an env.  Open
# TEST	a second handle on the same env, get_open_flags and verify 
# TEST	the flag is returned.
# TEST
# TEST  Also check that the environment configurations lock and txn
# TEST  timeout, mpool max write openfd and mmap size, and log auto
# TEST  remove, when set before opening an environment, are applied
# TEST  when creating the environment, but not when joining.

proc env018 { } {
	source ./include.tcl
	set tnum "018"

	puts "Env$tnum: Test of join_env and getters."

	# Skip for HP-UX where a second handle on an env is not allowed.
	if { $is_hp_test == 1 } {
		puts "Skipping env$tnum for HP-UX."
		return
	}

	# Set up flags to use in opening envs. 
	set flags { -cdb -lock -log -txn } 

	foreach flag $flags {
		env_cleanup $testdir

		puts "\t\tEnv$tnum.a: Open env with $flag."
		set e1 [eval {berkdb_env} -create -home $testdir $flag]
		error_check_good e1_open [is_valid_env $e1] TRUE
	
		puts "\t\tEnv$tnum.b: Join the env."
		set e2 [eval {berkdb_env} -home $testdir]
		error_check_good e2_open [is_valid_env $e2] TRUE

		# Get open flags for both envs.
		set e1_flags_returned [$e1 get_open_flags]
		set e2_flags_returned [$e2 get_open_flags]

		# Test that the flag given to the original env is
		# returned by a call to the second env. 
		puts "\t\tEnv$tnum.c: Check that flag is returned."
		error_check_good flag_is_returned \
		    [is_substr $e2_flags_returned $flag] 1

		# Clean up. 
		error_check_good e1_close [$e1 close] 0
		error_check_good e2_close [$e2 close] 0
	}

	env_cleanup $testdir

	# Configurations being tested, values are:
	# env open command, set value, reset value, get command, 
	# printout string, get return value
	set rlist {
	{ " -blob_dir "  "." "./BLOBDIR" " get_blob_dir " " blob_dir " "." }
	{ " -blob_threshold "  "10485760" "20971520" " get_blob_threshold " 
	    " blob_threshold " "10485760" }
	{ " -log_remove " "" "" " log_get_config autoremove " 
	    " DB_LOG_AUTOREMOVE " "1" }
	{ " -txn_timeout " "3" "6" " get_timeout txn " "  txn_timeout " "3" }
	{ " -lock_timeout " "4" "7" " get_timeout lock " " lock_timeout " "4" }
	{ " -mpool_max_openfd " "1000" "2000" " get_mp_max_openfd " 
	    " mpool_max_openfd " "1000" }
	{ " -mpool_max_write " "{100 1000}" "{200 2000}" " get_mp_max_write " 
	    " mpool_max_write " "100 1000" }
	{ " -mpool_mmap_size " "1024" "2048" " get_mp_mmapsize " 
	    " mpool_mmap_size " "1024" }
	{ " -log_max " "100000" "900000" 
	    " get_lg_max " " get_log_max " "100000" }
	}    

	# Build one environment open command including all tested options.
	set envopen "berkdb_env -create -home $testdir -txn -lock -log \
	    -msgfile $testdir/msgfile "
	foreach item $rlist {
		append envopen [lindex $item 0] [lindex $item 1]	
	}
	puts "\t\tEnv$tnum.d: Create env with given configurations."
	set e3 [eval $envopen]
		error_check_good e3_open [is_valid_env $e3] TRUE


	puts "\t\tEnv$tnum.e: Check that env configurations have been set."
	foreach item $rlist {
		set printout [lindex $item 4]
		set value [lindex $item 5]
		puts "\t\t\tEnv$tnum.e.1 Check $printout."
		set command [lindex $item 3]
		error_check_good [lindex $item 4] \
		    [eval $e3 $command ] $value

	}

	# Build one environment re-open command including all tested options.
	set envopen "berkdb_env_noerr -home $testdir -txn -lock -log \
	    -msgfile $testdir/msgfile "
	foreach item $rlist {
		if { ![string equal [lindex $item 0] " -log_remove " ] } {
			append envopen [lindex $item 0] [lindex $item 2]
		}
	}
	puts "\t\tEnv$tnum.f: Join env with different configuration values."
	set e4 [eval $envopen]
	error_check_good e4_open [is_valid_env $e4] TRUE


	puts "\t\tEnv$tnum.g: Check that config values have not changed."
	foreach item $rlist {
		set printout [lindex $item 4]
		set value [lindex $item 5]
		puts "\t\t\tEnv$tnum.g.1 Check $printout."
		set command [lindex $item 3]
		error_check_good $printout \
		    [eval $e3 $command ] $value
	}

	# Clean up. 
	error_check_good e3_close [$e3 close] 0
	error_check_good e4_close [$e4 close] 0   
}

