# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates. All rights reserved.
#
# $Id$
#
# TEST	test144
# TEST	Tests setting the heap size.
# TEST	1. Open the db with heap size smaller than 3 times the database page
# TEST	   size and it fails and it should fail.
# TEST	2. Open the db with heap size A and close it. Reopen the db with heap
# TEST	   size B (A != B) and it should fail.
# TEST	3. Open the db with heap size A, put some records to make the db file
# TEST	   size bigger than A and it returns DB_HEAP_FULL.
# TEST	4. Open another heap database after getitng DB_HEAP_FULL and it
# TEST	   should succeed.
proc test144 { method {tnum "144"} args } {
	global default_pagesize
	global errorCode
	source ./include.tcl

	# This is a heap-only test.
	if { [is_heap $method] != 1} {
		puts "Test$tnum skipping for method $method."
		return
	}

	# Pagesize is needed and use the default_pagesize if it is not passed.
	set pgindex [lsearch -exact $args "-pagesize"]
	set end [expr $pgindex + 1]
	if { $pgindex != -1 } {
		set pgsize [lindex $args $end]
		set args [lreplace $args $pgindex $end]
	} else {
		set pgsize $default_pagesize
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			set args "$args -auto_commit"
		}
		set testdir [get_home $env]
	}

	puts "Test$tnum: $method ($args -pagesize $pgsize) Set heap size."

	# Remove the db files created previously.
	cleanup $testdir $env

	# Open the db with heap size smaller than 3 times the db page size.
	puts "\tTest$tnum.a: open the database with very small heap size\
	    and it should fail."
	set heapsz [expr $pgsize * 2]
	set oflags " -create -pagesize $pgsize $omethod "
	set ret [catch {eval {berkdb_open_noerr} $args \
	    {-heapsize "0 $heapsz"} $oflags $testfile} res]
	error_check_bad dbopen $ret 0
	error_check_good dbopen [is_substr $errorCode EINVAL] 1

	# Open the db with heap size equal to 3 db page size, close it and
	# reopen it with a different heap size.
	puts "\tTest$tnum.b: close and reopen the database with a different\
	    heap size, and it should fail."
	set heapsz [expr $pgsize * 3]
	set db [eval {berkdb_open_noerr} $args \
	    {-heapsize "0 $heapsz"} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	error_check_good db_close [$db close] 0
	set heapsz [expr $pgsize * 4]
	set ret [catch {eval {berkdb_open_noerr} $args \
	    {-heapsize "0 $heapsz"} $oflags $testfile} res]
	error_check_bad dbopen $ret 0

	# Put some records into the db until it returns DB_HEAP_FULL
	puts "\tTest$tnum.c: put some records into the heap database\
	    until get DB_HEAP_FULL."
	set heapsz [expr $pgsize * 3]
	set db [eval {berkdb_open_noerr} $args \
	    {-heapsize "0 $heapsz"} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# The heap size is set 3 db page size: 1 heap meta page, 1 heap 
	# internal page and 1 heap data page. So we need to fill up 1 db page
	# in order to get DB_HEAP_FULL.
	# The size of heap data page header is 48 bytes if checksum is enabled,
	# 64 bytes if encryption is enabled, and 26 bytes if neither is
	# enabled.
	# Each data item put on the page is 4-byte aligned and preceded with a
	# header whose size is 16 bytes if it is a split-header and 4 bytes if
	# not. The minimum size of the data item on the page is equal to the
	# size of a split-header. We are putting 1 character (1 byte) as data
	# for each record into the database. 1 byte + 4 byte (data item header)
	# and get it 4-byte aligned. So the size of each data item is 8 bytes
	# and smaller than a split-header. Thus each data item will occupy 16
	# bytes on the page.
	# The size of each offset in the offset table is 2 bytes.
	# Calculate data page header size and the number of records to put.
	set encindx1 [lsearch $args "-encryptaes"]
	set encindx2 [lsearch $args "-encrypt"]
	if { $encindx1 != -1 || $encindx2 != -1 } {
		set hdrsz 64
	} else {
		set chkindex [lsearch -exact $args "-chksum"]
		if { $chkindex != -1 } {
			set hdrsz 48
		} else {
			set hdrsz 26
		}
	}
	set nentries [expr ($pgsize - $hdrsz) / (16 + 2)]
	# Do db_put with txn if it is a transactional env.
	set txn ""
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	for { set i 1 } { $i <= $nentries } { incr i } {
		set data [int_to_char [expr {($i - 1) % 26}]]
		set ret [eval {$db put} $txn $i $data]
		error_check_good db_put $ret 0
	}
	if { $txnenv == 1 } {
		error_check_good txn [$t commit] 0
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set data [int_to_char [expr {($i - 1) % 26}]]
	set ret [catch {eval {$db put} $txn $i $data} res]
	error_check_bad db_put $ret 0
	error_check_good db_put [is_substr $res "DB_HEAP_FULL"] 1
	if { $txnenv == 1 } {
		error_check_good txn [$t abort] 0
	}

	# Open another db after getting DB_HEAP_FULL
	puts "\tTest$tnum.d: open another database after\
	    getting DB_HEAP_FULL and it should succeed."
	set db1 [eval {berkdb_open_noerr} $args \
	    {-heapsize "0 $heapsz"} $oflags $testfile]
	error_check_good dbopen [is_valid_db $db1] TRUE

	error_check_good db1_close [$db1 close] 0
	error_check_good db_close [$db close] 0
}
