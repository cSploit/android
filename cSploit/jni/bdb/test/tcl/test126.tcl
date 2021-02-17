# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test126
# TEST	Test database bulk update for non-duplicate databases.
# TEST
# TEST	Put with -multiple, then with -multiple_key,
# TEST	and make sure the items in database are what we put.
# TEST  Later, delete some items with -multiple, then with -multiple_key,
# TEST	and make sure if the correct items are deleted.

proc test126 {method { nentries 10000 } { tnum "126" } {callback 1} 
    {subdb 0} {secondary 0} {sort_multiple 0} args } {
	source ./include.tcl

	# For rrecno, when keys are deleted, the ones after will move forward,
	# and the keys change, which is not good to verify after delete.
	# So, we skip rrecno temporarily.
        if {[is_rrecno $method] } {
		puts "Skipping test$tnum for $method test."
		return
	}

	# It is only makes sense for btree database to sort bulk buffer.
	if {$sort_multiple && ![is_btree $method]} {
		puts "Skipping test$tnum for $method with -sort_multiple."
		return
	}

	# For compressed databases, we need to sort the items in
	# the bulk buffer before doing the bulk put/delete operations.
	if {[is_compressed $args] && !$sort_multiple} {
		puts "Skipping test$tnum for compressed databases\
		    without -sort_multiple."
		return
	}

	set subname ""
	set sub_msg ""

	# Check if we use sub database.
	if { $subdb } {
		if {[is_queue $method] || [is_heap $method]} {
			puts "Skipping test$tnum with sub database for $method."
			    return
		}
		if {[is_partitioned $args]} {
			puts "Skipping test$tnum with sub database\
		       	    for partitioned $method test."
			    return		
		}
		set subname "subdb"
		set sub_msg "using sub databases"
	}

	set sec_msg ""
	# Check if we use secondary database.
	if { $secondary } {
		set sec_msg "with secondary databases"
	}

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set eindex [lsearch -exact $args "-env"]
	set txnenv 0
	set txn ""
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
		if {$subdb && $secondary } {
			puts "Skipping test$tnum $sub_msg $sec_msg for non-env test."
			return
		}
	} else {
		set testfile test$tnum.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env
	set sec_args $args

	set args [convert_args $method $args]
	set omethod [convert_method $method]	

	set extra_op ""
	if {$sort_multiple} {
		set extra_op "-sort_multiple"
	}

	puts "Test$tnum: $method ($args)\
	    Database bulk update $sub_msg $sec_msg."

	set db [eval {berkdb_open_noerr -create -mode 0644} \
	    $args $omethod $testfile $subname]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Open the secondary database and do association. 
	# This is the test for [#18878].
	if { $secondary } {
		if { $subdb } {
			set sec_subname "subdb-secondary"
			set sec_testfile $testfile
		} else {
			set sec_subname ""
			if { $eindex == -1 } {
				set sec_testfile $testdir/test$tnum-secondary.db
			} else {
				set sec_testfile test$tnum-secondary.db
			}
		}
		# Open a simple dupsort btree database.
		# In order to be consistent, we need to use all the passed-in 
		# am-unrelated flags.
		set sec_args [convert_args "btree" $sec_args]
		set sec_db [eval {berkdb_open_noerr -create -mode 0644} $sec_args \
		    -dup -dupsort -btree $sec_testfile $sec_subname]
		error_check_good secdb_open [is_valid_db $sec_db] TRUE
		set ret [$db associate -create [callback_n $callback] $sec_db]
		error_check_good db_associate $ret 0
	}
	
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}

	set did [open $dict]
	set count 0

	
	# Do bulk put.
	# First, we put half the entries using put -multiple.
	# Then, we put the rest half using put -multiple_key.

	puts "\tTest$tnum.a: Bulk put data using -multiple $extra_op."
	set key_list1 {}
	set data_list1 {}
	while { [gets $did str] != -1 && $count < $nentries / 2 } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
			set str [reverse $str]
		}
		set data [make_fixed_length $method $str]
		lappend key_list1 $key
		lappend data_list1 $data
		incr count
		if { [is_heap $method] } {
		    set ret [eval {$db put} $txn {$key $data}]
		}
	}

	if { [is_heap $method] == 0 } {
	 	set ret [eval {$db put} $txn -multiple $extra_op \
 		    {$key_list1 $data_list1}]
 		error_check_good "put(-multiple $extra_op)" $ret 0
	}

	# Put again, should succeed
 	set ret [eval {$db put} $txn -multiple $extra_op \
 	    {$key_list1 $data_list1}]
 	error_check_good "put_again(-multiple $extra_op)" $ret 0
  
 	puts "\tTest$tnum.b: Bulk put data using -multiple_key $extra_op."
	set pair_list1 {}
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1]
		} else {
			set key $str
			set str [reverse $str]
		}
		set data [make_fixed_length $method $str]
		lappend pair_list1 $key $data
		incr count	
		if { [is_heap $method] } {
			set ret [eval {$db put} $txn $key $data]
		}
	}

	if { [is_heap $method] == 0 } {
	 	set ret [eval {$db put} $txn -multiple_key $extra_op \
		    {$pair_list1}]
 		error_check_good "put(-multiple_key $extra_op)" $ret 0	
	}

	# Put again, should succeed
 	set ret [eval {$db put} $txn -multiple_key $extra_op \
 	    {$pair_list1}]
 	error_check_good "put_again(-multiple_key $extra_op)" $ret 0

	close $did	

	puts "\tTest$tnum.c: Verify the data after bulk put."
	set len [llength $pair_list1]
	for {set indx1 0; set indx2 1} {$indx2 < $len} \
	    {incr indx1 2; incr indx2 2} {
		lappend key_list1 [lindex $pair_list1 $indx1]
		lappend data_list1 [lindex $pair_list1 $indx2]
	}

	test126_check_prirecords $db $key_list1 $data_list1 $txn

	if { $secondary } {
		puts "\tTest$tnum.c.2: Verify the data in secondary database."
		set sec_key_list {}
		foreach key $key_list1 data $data_list1 {
			lappend sec_key_list \
			    [[callback_n $callback] $key $data]
		}
		test126_check_secrecords $sec_db $sec_key_list \
		    $key_list1 $data_list1 $txn
	}

	puts "\tTest$tnum.d: Bulk delete data using -multiple $extra_op."
	set key_list2 {}
	for { set i 0 } { $i < $nentries} { incr i 3 } {
		lappend key_list2 [lindex $key_list1 $i]
	}
	set ret [eval {$db del} $txn -multiple $extra_op {$key_list2}]
	error_check_good "del(-multiple $extra_op)" $ret 0

	# Delete again, should return DB_NOTFOUND/DB_KEYEMPTY.
	set ret [catch {eval {$db del} $txn -multiple $extra_op \
	    {$key_list2}} res]
	error_check_good {Check DB_NOTFOUND/DB_KEYEMPTY} \
	    [expr [is_substr $res DB_NOTFOUND] || \
	    [is_substr $res DB_KEYEMPTY]] 1

	puts "\tTest$tnum.e: Bulk delete data using -multiple_key $extra_op."
	set pair_list2 {}
	for { set i 1 } { $i < $nentries} { incr i 3} {
		lappend pair_list2 [lindex $key_list1 $i] \
		    [lindex $data_list1 $i]
	}

	set ret [eval {$db del} $txn -multiple_key $extra_op {$pair_list2}]
	error_check_good "del(-multiple_key $extra_op)" $ret 0

	# Delete again, should return DB_NOTFOUND/DB_KEYEMPTY.
	set ret [catch {eval {$db del} $txn -multiple_key $extra_op \
	    {$pair_list2}} res]
	error_check_good {Check DB_NOTFOUND/DB_KEYEMPTY} \
	    [expr [is_substr $res DB_NOTFOUND] || \
	    [is_substr $res DB_KEYEMPTY]] 1


	puts "\tTest$tnum.f: Verify the data after bulk delete."	

	# Check if the specified items are deleted
	set dbc [eval $db cursor $txn]
	error_check_good $dbc [is_valid_cursor $dbc $db] TRUE
	set len [llength $key_list2]
	for {set i 0} {$i < $len} {incr i} {
		set key [lindex $key_list2 $i]
		set pair [$dbc get -set $key]
		error_check_good pair [llength $pair] 0
	}

	set len [llength $pair_list2]
	for {set indx1 0; set indx2 1} {$indx2 < $len} \
       	    {incr indx1 2; incr indx2 2} {
		set key [lindex $pair_list2 $indx1]
		set data [lindex $pair_list2 $indx2]
		set pair [$dbc get -get_both $key $data]
		error_check_good pair [llength $pair] 0
	}

	error_check_good $dbc.close [$dbc close] 0	

	# Remove the deleted items from the original key-data lists.
	# Since the primary database is non-duplicate, it is enough 
	# for us to just compare using keys.
	set orig_key_list $key_list1
	set orig_data_list $data_list1
	set key_list1 {}
	set data_list1 {}
	set i 0
	set j 0
	set k 0
	while {$i < $nentries} {
		set key1 [lindex $orig_key_list $i]
		set key2 [lindex $key_list2 $j]
		set key3 [lindex $pair_list2 $k]
		if {$key1 == $key2} {
			incr i
			incr j
		} elseif {$key1  == $key3} {
			incr i
	    		incr k 2
		} else {
			lappend key_list1 $key1
			lappend data_list1 [lindex $orig_data_list $i]
			incr i
		}
	}

	test126_check_prirecords $db $key_list1 $data_list1 $txn

	if { $secondary } {
		puts "\tTest$tnum.f.2: Verify the data in secondary database."
		set sec_key_list {}
		foreach key $key_list1 data $data_list1 {
			lappend sec_key_list \
			    [[callback_n $callback] $key $data]
		}
		test126_check_secrecords $sec_db $sec_key_list \
		    $key_list1 $data_list1 $txn
	}
	
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_close [$db close] 0
	if { $secondary } {
		error_check_good secdb_close [$sec_db close] 0
	}
}

proc test126_check_prirecords {db key_list data_list txnarg} {

	set dbc [eval $db cursor $txnarg]
	error_check_good $dbc [is_valid_cursor $dbc $db] TRUE

	# Check if all the records are in key_list(key) and data_list(data).
	for {set pair [$dbc get -first]} {[llength $pair] > 0} \
	    {set pair [$dbc get -next]} {
		set key [lindex [lindex $pair 0] 0]
		set data [lindex [lindex $pair 0] 1]		
		set index [lsearch -exact $key_list $key]
		error_check_bad key_index $index -1
		error_check_good data $data [lindex $data_list $index]
	}

	# Check if all the items in the lists are in the database.
	set len [llength $key_list]
	for {set i 0} {$i < $len} {incr i} {
		set pair [$dbc get -get_both [lindex $key_list $i] \
		    [lindex $data_list $i]]
		error_check_bad pair [llength $pair] 0
	}

	error_check_good $dbc.close [$dbc close] 0
}

proc test126_check_secrecords {db sec_key_list pri_key_list data_list txnarg} {

	set dbc [eval $db cursor $txnarg]
	error_check_good $dbc [is_valid_cursor $dbc $db] TRUE

	# Check if all the records are in the lists
	for {set pair [$dbc pget -first]} {[llength $pair] > 0} \
	    {set pair [$dbc pget -next]} {
		set sec_key [lindex [lindex $pair 0] 0]
		set pri_key [lindex [lindex $pair 0] 1]
		set data [lindex [lindex $pair 0] 2]		
		set index [lsearch -exact $pri_key_list $pri_key]
		error_check_bad key_index $index -1
		error_check_good seckey $sec_key [lindex $sec_key_list $index]
		error_check_good data1 $data [lindex $data_list $index]
	}

	# Check if all the items in the lists are in the secondary database.
	set len [llength $sec_key_list]
	for {set i 0} {$i < $len} {incr i} {
		set pair [$dbc pget -get_both [lindex $sec_key_list $i] \
		    [lindex $pri_key_list $i]]
		error_check_bad pair [llength $pair] 0
		error_check_good data2 [lindex $data_list $i] \
		    [lindex [lindex $pair 0] 2]
	}

	error_check_good $dbc.close [$dbc close] 0
}
