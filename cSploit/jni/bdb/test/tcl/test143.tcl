# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test143
# TEST
# TEST	Test of mpool cache resizing.
# TEST	
# TEST	Open an env with specified cache size and cache max.
# TEST	Write some data, check cache size. 
# TEST	Resize cache.
# TEST	Configure cache-related mutex settings.

proc test143 { method {tnum "143"} args } {
	source ./include.tcl
	
	# Cache resizing is independent of method, so running 
	# for a single access method is enough.
	if { [is_btree $method] != 1 } {
		puts "Skipping test$tnum for method $method."
		return
	}

	# Set up multipliers for cache size.  We'll make
	# them all multiples of 1024*1024.
	set multipliers [list 1 8 16 32]
	set pgindex [lsearch -exact $args "-pagesize"]
	
	# Very small pagesizes can exhaust our mutex region. 
	# Use smaller (and different!) cache multipliers for
	# testing with explicit pagesizes.
        if { $pgindex != -1 } {
		set multipliers [list 1 4 10]
	}

	# When native pagesize is small, this test requires
	# a ver large number of mutexes. In this case, increase
	# the number of mutexes and also reduce the size of the
	# working data set.
	set mutexargs ""
	set nentries 10000
	set native_pagesize [get_native_pagesize]
	if {$native_pagesize < 2048} {
		set mutexargs "-mutex_set_max 100000"
		set nentries 2000
	}

	# Test for various environment types including
	# default, multiversion, private, and system_mem.
	test143_body $method $tnum "$mutexargs" $multipliers $nentries $args

	set multipliers [list 8]
	test143_body $method $tnum "-multiversion $mutexargs" \
	    $multipliers $nentries $args
	test143_body $method $tnum "-private $mutexargs" \
	    $multipliers $nentries $args
	if { $is_qnx_test } {
		puts "\tTest$tnum: Skipping system_mem\
		    testing for QNX."
	} else {
		set shm_key 20
		test143_body $method $tnum \
		    "-system_mem -shm_key $shm_key $mutexargs" \
		    $multipliers $nentries $args
    	}

	# Test that cache-related mutex configation options which exercise
	# certain code paths not executed by the cases above.
	foreach envopts { "-private" "-private -thread" "" } {
		foreach mtxopts { "-mutex_set_max 100000" \
		    "-mpool_mutex_count 10" \
		    "-mpool_mutex_count 10 -mutex_set_max 100000" } {
			test143_body $method $tnum \
			    "$envopts $mtxopts"  $multipliers 100 $args
		}
	}
}

proc test143_body { method tnum envargs multipliers \
    { nentries 10000 } largs } {

	source ./include.tcl
	global alphabet

	# This test needs its own env.
	set eindex [lsearch -exact $largs "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $largs $eindex]
		puts "Test$tnum skipping for env $env"
		return
	}

	set args [convert_args $method $largs]
	set omethod [convert_method $method]

	# To test with encryption, we'll need to add 
	# args to the env open.
	set encargs ""
	set args [split_encargs $args encargs]

	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}

	puts "Test$tnum: ($method $args) Cache resizing."

	set max_mult 128
	set maxsize [expr $max_mult * 1024 * 1024]

	set data [repeat $alphabet 100]

	# Create transactional env with various cache sizes.
	foreach m $multipliers {
		env_cleanup $testdir
		set csize [expr $m * 1024 * 1024]
		puts "\tTest$tnum.a:\
		    Create env ($envargs) with cachesize of $m megabyte(s)."
		set env [eval {berkdb_env_noerr} $encargs $envargs \
		    {-cachesize "0 $csize 1" -cache_max "0 $maxsize" \
		    -create -txn -home $testdir}]
		error_check_good env_open [is_valid_env $env] TRUE
		set htab_mutexes \
		    [stat_field $env mpool_stat "Mutexes for hash buckets"]
		# Private, non-threaded environments should not have any
		# mutexes for the hash table.
		if { [ is_substr $envargs "-private" ] && \
		   ! [ is_substr $envargs "-thread"] } {
			set mutexes_expected 0
		} elseif { [ is_substr $envargs "mpool_mutex_count" ] } {
			set mutexes_expected 10
		} else {
			set mutexes_expected \
			    [stat_field $env mpool_stat "Hash buckets" ]
		}
		error_check_good "Hash bucket $envargs mutexes " \
		    $mutexes_expected $htab_mutexes

		# Env is open, check and report cache size.
		set actual_cache_size [get_total_cache $env]
		set actual_cache_max [lindex [$env get_cache_max] 1]

		# Check actual cache size and cache max size 
		# against our expectations.  These smallish caches 
		# should have been sized up by about 25%. 
		check_within_range \
		    $actual_cache_size $csize 1.15 1.4 "cachesize" 
		check_within_range \
		    $actual_cache_max $maxsize 0.9 1.1 "cachemax" 
		
		# Open a db, write some data.
		puts "\tTest$tnum.b: Populate db."
		set db [eval {berkdb_open_noerr} $args \
		    {-env $env -create $omethod test143.db}]
		error_check_good db_open [is_valid_db $db] TRUE
		for { set i 1 } { $i <= $nentries } { incr i } {
			$db put $i [chop_data $method $i.$data]
		}
	
		# Check cache size again - it should not have changed. 
		check_within_range \
		    $actual_cache_size $csize 1.15 1.4 "cachesize" 
		check_within_range \
		    $actual_cache_max $maxsize 0.9 1.1 "cachemax" 
		
		# Resize cache.  
		set new_mult 3
		set newmb [expr $new_mult * $m]
		set newsize [expr $newmb * 1024 * 1024]
		puts "\tTest$tnum.c: Resize cache to $newmb megabytes."
		$env resize_cache "0 $newsize"
		set actual_cache_size [get_total_cache $env]
		set actual_cache_max [lindex [$env get_cache_max] 1]

		# Check cache size again; it should be the new size.
		check_within_range \
		    $actual_cache_size $newsize 1.15 1.4 "cachesize" 
		check_within_range \
		    $actual_cache_max $maxsize 0.9 1.1 "cachemax" 

		# Try to increase cache size beyond cache_max.  
		# The operation should fail, and cache size should 
		# remain the same.
		set big_mult 256
		puts "\tTest$tnum.d: Try to exceed cache_max.  Should fail."
		set bigsize [expr $big_mult * 1024 * 1024]
		catch {$env resize_cache "0 $bigsize"} res
		error_check_good \
		    cannot_resize [is_substr $res "cannot resize"] 1
		check_within_range \
		    $actual_cache_size $newsize 1.15 1.4 "cachesize" 

		error_check_good db_sync [$db sync] 0
		error_check_good verify\
		   [verify_dir $testdir "\tTest$tnum.e: " 0 0 $nodump] 0

		# Decrease cache size.
		set new_mult 2
		set newmb [expr $new_mult * $m]
		set newsize [expr $newmb * 1024 * 1024]
		puts "\tTest$tnum.f: Resize cache to $newmb megabytes."
		$env resize_cache "0 $newsize"
		set actual_cache_size [get_total_cache $env]
		set actual_cache_max [lindex [$env get_cache_max] 1]

		# Check cache size again; it should be the new size.
		check_within_range \
		    $actual_cache_size $newsize 1 1.4 "cachesize" 
		check_within_range \
		    $actual_cache_max $maxsize 0.9 1.1 "cachemax" 

		error_check_good db_sync [$db sync] 0
		error_check_good verify\
		   [verify_dir $testdir "\tTest$tnum.g: " 0 0 $nodump] 0

		# Clean up.
		error_check_good db_close [$db close] 0
		error_check_good env_close [$env close] 0
	}
}

# The "requested" value is what we told the system to use; 
# the "actual" value is what the system is actually using, 
# after applying its adjustments.  "Max" and "min" are factors,
# usually near 1, implying the allowed range of actual values.
#
proc check_within_range { actual requested min max name } {
	set largest [expr $requested * $max]
	set smallest [expr $requested * $min]

	error_check_good "$name too large" [expr $actual < $largest] 1
	error_check_good "$name too small" [expr $actual > $smallest] 1
}
 
# Figure out the total available cache.
# On 32bit system, we can only get the correct value when total cache size is
# less than 2GB, so we should make sure this proc is not called on env with
# cache size larger than 2GB.
proc get_total_cache { env } {
	set gbytes [lindex [$env get_cachesize] 0]
	set bytes [lindex [$env get_cachesize] 1]
	set total_cache [expr $gbytes * 1024 * 1024 * 1024 + $bytes]
	return $total_cache
}

