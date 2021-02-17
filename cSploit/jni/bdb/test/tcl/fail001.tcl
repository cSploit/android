# See the file LICENSE for redistribution information.
#
# Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	fail001
# TEST	Test database compaction errors.
# TEST
# TEST	Populate a database.
# TEST	1) Compact the heap / queue database and it should fail.
# TEST	2) Reopen the database with -rdonly, compact the database and it
# TEST	should fail.

proc fail001 { } {

	source ./include.tcl

	set opts { "heap" "queue" "rdonly" }
	set nentries 10000
	set testfile fail001.db

	puts "Fail001: Database compaction errors."
	
	cleanup $testdir NULL
	set env [eval {berkdb_env_noerr -home $testdir} -create]
	error_check_good is_valid_env [is_valid_env $env] TRUE

	foreach opt $opts {
		cleanup $testdir $env

		set did [open $dict]

		if { $opt == "rdonly" } {
			set method "btree"
		} else {
			set method $opt
		}
		set args ""
		set args [convert_args $method $args]
		set omethod [convert_method $method]

		set flags "-env $env -create -mode 0644 "

		puts "\tFail001.a: Create and populate database ($omethod)."
		set db [eval {berkdb_open_noerr -env $env \
		    -mode 0644} -create $args $omethod $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		set count 0
		while { [gets $did str] != -1 && $count < $nentries } {
			if { [is_record_based $method] == 1 } {
				set key [expr $count + 1]
			} else {
				set key $str
				set str [reverse $str]
			}

			set ret [eval {$db put} \
			    {$key [chop_data $method $str]}]
			error_check_good put $ret 0
			incr count
		}
		close $did
		error_check_good db_sync [$db sync] 0

		if { $opt == "rdonly" } {
			puts "\tFail001.a1: Reopen the database with -rdonly"
			error_check_good db_close [$db close] 0
			set db [eval {berkdb_open_noerr -env $env} \
			    -rdonly $omethod $testfile]
			error_check_good dbopen [is_valid_db $db] TRUE
		}


		puts "\tFail001.b: Compact database and verify the error."
		set ret [catch {eval {$db compact}} res]

		if { $opt == "rdonly" } {
			set emsg "attempt to modify a read-only database"
		} else {
			set emsg "call implies an access method which is\
			    inconsistent with previous calls"
		}
		error_check_bad db_compact $ret 0
		error_check_good compact_err [is_substr $res $emsg] 1
		error_check_good db_close [$db close] 0
	}
}
