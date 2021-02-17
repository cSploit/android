# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test139
# TEST
# TEST	Verify an open database.
# TEST	Create and populate a database, leave open, and run 
# TEST	db->verify. 
# TEST	Delete half the data, verify, compact, verify again. 
proc test139 { method {pagesize 512} {nentries 1000} {tnum "139"} args } {
	source ./include.tcl

	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then skip this test.  It needs its own.
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test$tnum skipping for env $env"
		return
	}

	set args [convert_args $method $args]
	set encargs ""
	set args [split_encargs $args encargs]
	set omethod [convert_method $method]

	env_cleanup $testdir
	set testfile test$tnum.db
    
	set env [eval {berkdb_env -create -mode 0644} $encargs -home $testdir]
	error_check_good dbenv [is_valid_env $env] TRUE
	set envargs "-env $env"

	puts "Test$tnum: $method ($args) Verify an open database."
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex == -1 } {
		append args " -pagesize $pagesize "
	}
	set did [open $dict]

	set db [eval {berkdb_open -env $env -create \
	    -mode 0644} $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	puts "\tTest$tnum.a: Populate the db."
	set count 0
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			global kvals

			set key [expr $count + 1]
			if { 0xffffffff > 0 && $key > 0xffffffff } {
				set key [expr $key - 0x100000000]
			}
			if { $key == 0 || $key - 0xffffffff == 1 } {
				incr key
				incr count
			}
			set kvals($key) [pad_data $method $str]
		} else {
			set key $str
			set str [reverse $str]
		}

		set ret [eval {$db put $key [chop_data $method $str]}]
		error_check_good put $ret 0
		
		# Sync the first item so we have something on disk.
		if { $count == 0 } {
			error_check_good db_sync [$db sync] 0
		}

		incr count
	}

	close $did

	# Now verify. 
	puts "\tTest$tnum.b: Verify the db while still open."
	set ret [eval {berkdb dbverify} $envargs $testfile]

	# Sync, verify again.
	error_check_good db_sync [$db sync] 0
	set ret [eval {berkdb dbverify} $envargs $testfile]

	# Cursor delete,leaving every third entry.  Since rrecno 
	# renumbers, delete starting at nentries and work backwards.
	puts "\tTest$tnum.c: Delete many entries from database."
	set did [open $dict]

	for { set i $nentries } { $i > 0 } { incr i -1 } {
		if { [is_record_based $method] == 1 } {
			set key $i
		} else {
			set key [gets $did]
		}

		# Leave every n'th item.
		set n 3
		if { [expr $i % $n] != 0 } {
			set ret [eval {$db del $key}]
			error_check_good del $ret 0
		}
	}
	close $did

	error_check_good db_sync [$db sync] 0

	puts "\tTest$tnum.d: Verify the db after deletes."
	set ret [eval {berkdb dbverify} $envargs $testfile]

	# Compact, if it's possible for the access method.
	if { ![is_queue $method] == 1 && ![is_heap $method] == 1 } {
		puts "\tTest$tnum.d: Compact database."
		if {[catch {eval {$db compact -freespace}} ret] } {
			error "FAIL: db compact: $ret"
		}
	}

	puts "\tTest$tnum.d: Verify the db after deletes."
	set ret [eval {berkdb dbverify} $envargs $testfile]

	error_check_good db_sync [$db sync] 0
	puts "\tTest$tnum.e: Verify the db after sync."
	set ret [eval {berkdb dbverify} $envargs $testfile]

	# Clean up. 
	puts "\tTest$tnum.: Clean up."
	error_check_good db_close [$db close] 0
	if { $env != "NULL" } {
		error_check_good env_close [$env close] 0
	}
}

