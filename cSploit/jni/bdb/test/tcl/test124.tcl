# See the file LICENSE for redistribution information.
#
# Copyright (c) 2008, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test124
# TEST	
# TEST	Test db->verify with noorderchk and orderchkonly flags.
# TEST
# TEST  Create a db with a non-standard sort order.  Check that 
# TEST	it fails a regular verify and succeeds with -noorderchk.
# TEST	Do a similar test with a db containing subdbs, one with
# TEST	the standard order and another with non-standard.

proc test124 { method { nentries 1000 } args } {
	source ./include.tcl
	global encrypt

	set tnum "124"
	if { [is_btree $method] == 0 } {
		puts "Skipping test$tnum for method $method"
		return
	}

	set args [convert_args $method $args]
	set encargs ""
	set args [split_encargs $args encargs]
	set omethod [convert_method $method]

	puts "Test$tnum ($method $args):\
	    db->verify with -noorderchk and -orderchkonly."

	# If we are given an env, use it.  Otherwise, open one.
	# We need it for the subdb portion of the test.
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex == -1 } {
		env_cleanup $testdir
		set env [eval\
		    {berkdb_env_noerr} $encargs -create -home $testdir]
		error_check_good env_open [is_valid_env $env] TRUE
	} else {
		incr eindex
		set env [lindex $args $eindex]
		set envflags [$env get_open_flags]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	puts "\tTest$tnum.a:\
	    Create and populate database with non-standard sort."
	set testfile "test124.db"

	# We already know the test is btree only, so btcompare is okay.
	set sortflags " -btcompare test093_cmp1 "
	set db [eval {berkdb_open_noerr -env $env -create \
	    -mode 0644} $sortflags $args {$omethod $testfile}]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Start a txn, populate, and close.
	set txn "" 
	if { $txnenv == 1 } {
		set txn [$env txn]
	}
	populate $db $method $txn $nentries 0 0 
	error_check_good db_close [$db close] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$txn commit] 0
	}

	puts "\tTest$tnum.b: Verify with -noorderchk succeeds."
	set ret [eval {berkdb dbverify} -env $env -noorderchk $testfile] 
	error_check_good verify_noorderchk $ret 0

	puts "\tTest$tnum.c: Check that a regular verify fails."
	catch { set ret [eval {berkdb dbverify} -env $env $testfile] } caught
	error_check_good verify_fails [is_substr $caught DB_VERIFY_BAD] 1

	# Skip the subdb portion of the test for partitioned databases --
	# you cannot have multiple databases in a file *and* partitioning.
	if { [is_partitioned $args] == 0 } {

		puts "\tTest$tnum.d:\
		    Create and populate 2 subdbs, one with non-standard sort."
		set testfile2 "test124.db2"
		set sub1 "SUB1"
		set sub2 "SUB2"
		set sdb1 [eval {berkdb_open_noerr -env $env -create \
		    -mode 0644} $args {$omethod $testfile2 $sub1}]
		set sdb2 [eval {berkdb_open_noerr -env $env -create \
		    -mode 0644} $sortflags $args {$omethod $testfile2 $sub2}]
		error_check_good sdb1open [is_valid_db $sdb1] TRUE
		error_check_good sdb2open [is_valid_db $sdb2] TRUE

		set nentries [expr $nentries * 2] 
		if { $txnenv == 1 } {
			set txn [$env txn]
		}
		populate $sdb1 $method $txn $nentries 0 0 
		populate $sdb2 $method $txn $nentries 0 0 
		if { $txnenv == 1 } {
			error_check_good txn_commit [$txn commit] 0
		}
	
		error_check_good sdb1_close [$sdb1 close] 0
		error_check_good sdb2_close [$sdb2 close] 0

		# Verify the whole file with -noorderchk.
		puts "\tTest$tnum.e: Verify with -noorderchk succeeds."
		set ret \
		    [eval {berkdb dbverify} -env $env -noorderchk $testfile2] 

		# Verify with the sorted subdb with -orderchkonly.
		puts "\tTest$tnum.f:\
		    Verify with -orderchkonly succeeds for sorted subdb."
		set ret [eval {berkdb dbverify} \
		    -env $env -orderchkonly $testfile2 $sub1]

		# The attempt to verify the non-standard-sort subdb
		# with -orderchkonly is expected to fail.
		puts "\tTest$tnum.g: Verify with\
		    -orderchkonly fails for non-standard-sort subdb."
		catch { set ret [eval {berkdb dbverify} \
		    -env $env -orderchkonly $testfile2 $sub2] } caught
		error_check_good \
		    verify_fails [is_substr $caught DB_VERIFY_BAD] 1

	} 

	# Clean up.  
	# 
	# Delete test files -- we cannot have non-standard-sort 
	# files hanging around because they will cause the 
	# automatic verify in a complete run to fail. 
	set testdir [get_home $env]
	if { [is_partitioned $args] == 0 } {
		fileremove -f $testdir/$testfile2
	}
	fileremove -f $testdir/$testfile
	cleanup $testdir $env

	# Close the env if this test created it.
	if { $eindex == -1 } {
		error_check_good env_close [$env close] 0
	}
}
