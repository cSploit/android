# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST  test146
# TEST  Test the BLOB APIs.
# TEST  1) Test that the db blob threshold value defaults to
# TEST     the env threshold value.
# TEST  2) Test that the db blob threshold value is retained when re-opening
# TEST     the db.
# TEST  3) Test that the db blob threshold value is retained when re-opening
# TEST     the db with a different threshold value.
proc test146 { method {tnum "146"} args } {
	global default_pagesize
	source ./include.tcl

	# Blobs are supported for btree, hash and heap only.
	if {[is_btree $method] != 1 && \
	    [is_hash $method] != 1 && [is_heap $method] != 1} {
		puts "Test$tnum skipping for method $method."
		return
	}

	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test$tnum skipping for env $env"
		return
	}

	if { [lsearch -exact $args "-chksum"] != -1 } {
		set indx [lsearch -exact $args "-chksum"]
		set args [lreplace $args $indx $indx]
		puts "Test$tnum ignoring -chksum for blob"
	} 

	# Look for incompatible configurations of blob.
	foreach conf { "-encryptaes" "-encrypt" "-compress" "-dup" "-dupsort" \
	    "-read_uncommitted" "-multiversion" } {
		if { [lsearch -exact $args $conf] != -1 } {
			puts "Test146 skipping $conf, incompatible with blobs."
			return
		}
	}

	set pgindex [lsearch -exact $args "-pagesize"]
	set end [expr $pgindex + 1]
	if { $pgindex != -1 } {
		set pgsize [lindex $args $end]
		set args [lreplace $args $pgindex $end]
	} else {
		set pgsize $default_pagesize
	}
	set threshold1 [expr $pgsize * 10]
	set threshold2 [expr $pgsize * 20]
	set threshold3 [expr $pgsize * 30]

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# Set the db open flags.
	set oflags " -create -pagesize $pgsize $args $omethod "
	set testfile blob001.db

	puts "Test$tnum: $method ($args -pagesize $pgsize) Test the BLOB APIs."
	env_cleanup $testdir

	puts "\tTest$tnum.a: Test db blob threshold value\
	    defaults to the env threshold value."

	puts "\tTest$tnum.a.0: open env with the blob threshold value and then\
	    open db."
	# Open the env with a blob threshold value.
	set env [eval {berkdb env} \
	    -create -home $testdir -blob_threshold $threshold1]
	error_check_good is_valid_env [is_valid_env $env] TRUE
	error_check_good env_get_blobthreshold \
	    [$env get_blob_threshold] $threshold1

	# Open the db with no blob threshold value.
	set db [eval {berkdb_open_noerr} -env $env $oflags $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	# Verify the db blob threshold value.
	error_check_good db_get_blobthreshold \
	    [$db get_blob_threshold] $threshold1

	puts "\tTest$tnum.a.1: change the env blob threshold value after\
	    opening env and then open db."
	# Change the env blob threshold value.
	error_check_good set_blob_threshold \
	    [$env set_blob_threshold $threshold2] 0
	error_check_good env_get_blobthreshold \
	    [$env get_blob_threshold] $threshold2

	# Open the db with no blob threshold value.
	set db1 [eval {berkdb_open_noerr} -env $env $oflags $testfile-1]
	error_check_good db_open [is_valid_db $db1] TRUE

	# Verify the db blob threshold value.
	error_check_good db_get_blobthreshold \
	    [$db1 get_blob_threshold] $threshold2

	puts "\tTest$tnum.a.2: join the env with a different blob threshold\
	    and then open db."
	# Join the env with a different blob threshold value.
	# We're going to get a warning message out this -- 
	# redirect to a file so it won't be tagged as unexpected
	# output. 
	set env1 [eval {berkdb env} -create -home $testdir\
	    -blob_threshold $threshold3 -msgfile $testdir/msgfile]
	error_check_good is_valid_env [is_valid_env $env1] TRUE
	error_check_good env_get_blobthreshold \
	    [$env1 get_blob_threshold] $threshold2

	# Open the db with no blob threshold value.
	set db2 [eval {berkdb_open_noerr} -env $env1 $oflags $testfile-2]
	error_check_good db_open [is_valid_db $db2] TRUE

	# Verify the db blob threshold value.
	error_check_good db_get_blobthreshold \
	    [$db2 get_blob_threshold] $threshold2

	# Check for the expected message.
	set msg "Ignoring blob_threshold size when joining environment"
	set messagefound [eval findstring {$msg} $testdir/msgfile]
	error_check_bad found_msg $messagefound ""

	error_check_good db_close [$db2 close] 0
	error_check_good db_close [$db1 close] 0
	error_check_good db_close [$db close] 0
	error_check_good env_close [$env1 close] 0
	error_check_good env_close [$env close] 0

	env_cleanup $testdir

	puts "\tTest$tnum.b: Test the db blob threshold value is retained\
	    when reopening the db."
	# Open the env with no blob threshold value.
	set env [eval {berkdb env} -create -home $testdir]
	error_check_good is_valid_env [is_valid_env $env] TRUE

	# Open the db with a blob threshold value and close it.
	set db [eval {berkdb_open_noerr} \
	    -env $env -blob_threshold $threshold1 $oflags $testfile]
	error_check_good db_open [is_valid_db $db] TRUE
	error_check_good db_get_blobthreshold \
	    [$db get_blob_threshold] $threshold1
	error_check_good db_close [$db close] 0

	# Reopen the db with no blob threshold value.
	set db [eval {berkdb_open_noerr} -env $env $oflags $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	# Verify the db blob threshold value is retained.
	error_check_good db_get_blobthreshold \
	    [$db get_blob_threshold] $threshold1

	error_check_good db_close [$db close] 0

	puts "\tTest$tnum.c: Test the db blob threshold value is retained\
	    when reopening the db with a different threshold value."
	set db [eval {berkdb_open_noerr} \
	    -env $env -blob_threshold $threshold2 $oflags $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	# Verify the db blob threshold value is retained.
	error_check_good db_get_blobthreshold \
	    [$db get_blob_threshold] $threshold1

	error_check_good db_close [$db close] 0
	error_check_good env_close [$env close] 0
}
