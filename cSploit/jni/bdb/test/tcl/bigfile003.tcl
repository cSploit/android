# See the file LICENSE for redistribution information.
#
# Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	bigfile003
# TEST	1. Create a blob database.
# TEST	2. Create an empty blob.
# TEST	3. Append 5 GB data into the blob by database stream.
# TEST	4. Verify the blob size and data. For txn env, verify it with
# TEST	txn commit/abort.
# TEST	5. Verify getting the blob by database/cursor get method returns
# TEST	the error DB_BUFFER_SMALL.
# TEST	6. Run verify_dir.
# TEST	7. Run salvage_dir.
# TEST
# TEST	This test requires a platform that supports 5 GB files and
# TEST	64-bit integers.
proc bigfile003 { args } {
	source ./include.tcl
	global databases_in_memory
	global is_fat32
	global tcl_platform

	if { $is_fat32 } {
		puts "Skipping Bigfile003 for FAT32 file system."
		return
	}
	if { $databases_in_memory } {
		puts "Skipping Bigfile003 for in-memory database."
		return
	}
	if { $tcl_platform(pointerSize) != 8 } {
		puts "Skipping Bigfile003 for system\
		    that does not support 64-bit integers."
		return
	}

	# args passed to the test will be ignored.
	foreach method { btree rbtree hash heap } {
		foreach envtype { none regular txn } {
			bigfile003_sub $method $envtype
		}
	}
}

proc bigfile003_sub { method envtype } {
	source ./include.tcl
	global alphabet

	cleanup $testdir NULL

	#
	# Set up the args and env if needed.
	# It doesn't matter what blob threshold value we choose, since the
	# test will create an empty blob and append data into blob by
	# database stream.
	#
	set args ""
	set bflags "-blob_threshold 100"
	set txnenv 0
	if { $envtype == "none" } {
		set testfile $testdir/bigfile003.db
		set env NULL
		append bflags " -blob_dir $testdir/__db_bl"
		#
		# Use a 50MB cache. That should be
		# manageable and will help performance.
		#
		append args " -cachesize {0 50000000 0}"
	} else {
		set testfile bigfile003.db
		set txnargs ""
		if { $envtype == "txn" } {
			append args " -auto_commit "
			set txnargs "-txn"
			set txnenv 1
		}
		#
		# Use a 50MB cache. That should be
		# manageable and will help performance.
		#
		set env [eval {berkdb_env_noerr -cachesize {0 50000000 0}} \
		    -create -home $testdir $txnargs]
		error_check_good is_valid_env [is_valid_env $env] TRUE
		append args " -env $env"
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# No need to print the cachesize argument.
	set msg $args
	if { $env == "NULL" } {
		set indx [lsearch -exact $args "-cachesize"]
		set msg [lreplace $args $indx [expr $indx + 1]]
	}
	puts "Bigfile003: ($method $msg) Database stream test with 5 GB blob."

	puts "\tBigfile003.a: create the blob database."
	set db [eval {berkdb_open -create -mode 0644} \
	    $bflags $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tBigfile003.b: create an empty blob."
	set txn ""
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set key 1
	if { [is_heap $omethod] == 1 } {
		set ret [catch {eval {$db put} \
		    $txn -append -blob {""}} key]
	} else {
		set ret [eval {$db put} $txn -blob {$key ""}]
	}
	error_check_good db_put $ret 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	# Verify the blob is empty.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE
	set ret [catch {eval {$dbc get} -set {$key}} res]
	error_check_good cursor_get $ret 0
	error_check_good cmp_data [string length [lindex [lindex $res 0] 1]] 0

	# Open the database stream.
	set dbs [$dbc dbstream]
	error_check_good dbstream_open [is_valid_dbstream $dbs $dbc] TRUE
	error_check_good dbstream_size [$dbs size] 0

	puts "\tBigfile003.c: append 5 GB data\
	    into the blob by database stream..."
	flush stdout

	# Append 1 MB data into blob each time until it gets to 5GB.
	set basestr [repeat [repeat $alphabet 40] 1024]
	set offset 0
	set delta [string length $basestr]
	set size 0
	for { set gb 1} { $gb <= 5 } { incr gb } {
		for { set mb 1} { $mb <= 1024 } { incr mb } {
			error_check_good dbstream_write \
			    [$dbs write -offset $offset $basestr] 0
			incr size $delta
			error_check_good dbstream_size [$dbs size] $size
			error_check_good dbstream_read \
			    [string compare $basestr \
			    [$dbs read -offset $offset -size $delta]] 0
			incr offset $delta
		}
		if { $gb < 5} {
			set pct [expr 100 * $gb / 5]
			puts "\tBigfile003.c: $pct%..."
		}
	}
	puts "\tBigfile003.c: 100%."

	if { $txnenv == 1 } {
		# Close the database stream and cursor before aborting the txn.
		error_check_good dbstream_close [$dbs close] 0
		error_check_good cursor_close [$dbc close] 0

		puts "\tBigfile003.c1: abort the txn."
		error_check_good txn_abort [$t abort] 0

		# Open a new txn.
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"

		puts "\tBigfile003.c2: verify the blob is still empty."
		set dbc [eval {$db cursor} $txn]
		error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE
		set ret [catch {eval {$dbc get} -set {$key}} res]
		error_check_good cursor_get $ret 0
		error_check_good cmp_data \
		    [string length [lindex [lindex $res 0] 1]] 0
		set dbs [$dbc dbstream]
		error_check_good dbstream_open \
		    [is_valid_dbstream $dbs $dbc] TRUE
		error_check_good dbstream_size [$dbs size] 0

		puts "\tBigfile003.c3: append 5 GB\
		    data into the blob by database stream again..."
		set offset 0
		set size 0
		for { set gb 1} { $gb <= 5 } { incr gb } {
			for { set mb 1} { $mb <= 1024 } { incr mb } {
				error_check_good dbstream_write \
				    [$dbs write -offset $offset $basestr] 0
				incr size $delta
				error_check_good dbstream_size [$dbs size] $size
				error_check_good dbstream_read \
				    [string compare $basestr \
				    [$dbs read -offset $offset -size $delta]] 0
				incr offset $delta
			}
			if { $gb < 5 } {
				set pct [expr 100 * $gb / 5]
				puts "\tBigfile003.c3: $pct%..."
			}
		}
		puts "\tBigfile003.c3: 100%."
	}

	# Close the database stream and cursor.
	error_check_good dbstream_close [$dbs close] 0
	error_check_good cursor_close [$dbc close] 0

	if { $txnenv == 1 } {
		puts "\tBigfile003.c4: commit the txn"
		error_check_good txn_commit [$t commit] 0
	}

	puts "\tBigfile003.d: verify getting the blob by database/cursor\
	    get method returns the error DB_BUFFER_SMALL."
	# Try to get the blob by database get method.
	set ret [catch {eval {$db get $key}} res]
	error_check_bad db_get $ret 0
	error_check_good db_get [is_substr $res DB_BUFFER_SMALL] 1

	# Try to get the blob by cursor get method.
	set dbc [$db cursor]
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE
	set ret [catch {eval {$dbc get} -set {$key}} res]
	error_check_bad cursor_get $ret 0
	error_check_good cursor_get [is_substr $res DB_BUFFER_SMALL] 1

	# Close the cursor and database.
	error_check_good cursor_close [$dbc close] 0
	error_check_good db_close [$db close] 0

	# Close the env if opened.
	if { $env != "NULL" } {
		error_check_good env_close  [$env close] 0
	}

	puts "\tBigfile003.e: run verify_dir."
	error_check_good verify_dir \
	    [verify_dir $testdir "\tBigfile003" 0 0 0 50000000] 0

	# Calling salvage_dir creates very large dump files that
	# cause problems on some test platforms.  Test dumping
	# the database here instead.
	puts "\tBigfile003.f: dump the database with various options."
	set dumpfile $testdir/bigfile003.db-dump
	set salvagefile $testdir/bigfile003.db-salvage
	set aggsalvagefile $testdir/bigfile003.db-aggsalvage
	set utilflag "-b $testdir/__db_bl"

	# First do an ordinary db_dump.
	puts "\tBigfile003.f1: ordinary db_dump."
	set rval [catch {eval {exec $util_path/db_dump} $utilflag \
	    -f $dumpfile $testdir/bigfile003.db} res]
	error_check_good ordinary_dump $rval 0
	# Remove the dump file immediately to reuse the memory.
	fileremove -f $dumpfile

	# Now the regular salvage.
	puts "\tBigfile003.f2: regular salvage."
	set rval [catch {eval {exec $util_path/db_dump} $utilflag -r \
	    -f $salvagefile $testdir/bigfile003.db} res]
	error_check_good salvage_dump $rval 0
	fileremove -f $salvagefile

	# Finally the aggressive salvage.
	# We can't avoid occasional verify failures in aggressive
	# salvage.  Make sure it's the expected failure.
	puts "\tBigfile003.f3: aggressive salvage."
	set rval [catch {eval {exec $util_path/db_dump} $utilflag -R \
	    -f $aggsalvagefile $testdir/bigfile003.db} res]
	if { $rval == 1 } {
		error_check_good agg_failure \
		    [is_substr $res "DB_VERIFY_BAD"] 1
	} else {
		error_check_good aggressive_salvage $rval 0
	}
	fileremove -f $aggsalvagefile
}
