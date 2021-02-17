# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test127
# TEST	Test database bulk update.
# TEST
# TEST	This is essentially test126 with duplicates.
# TEST	To make it simple we use numerical keys all the time.
# TEST
# TEST	Put with -multiple, then with -multiple_key,
# TEST	and make sure the items in database are what we want.
# TEST  Later, delete some items with -multiple, then with -multiple_key,
# TEST	and make sure if the correct items are deleted.

proc test127 {method { nentries 10000 } { ndups 5} { tnum "127" } 
    {subdb 0} {sort_multiple 0} args } {
	source ./include.tcl
	global alphabet

	if {[is_btree $method] != 1 && [is_hash $method] != 1} {
		puts "Skipping test$tnum for $method."
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

	set args [convert_args $method $args]

	set subname ""
	set save_msg "with duplicates"
	if { $subdb } {
		if {[is_partitioned $args]} {
			puts "Skipping test$tnum with sub database\
		       	    for partitioned $method test."
			    return		
		}
		set subname "subdb"
		set save_msg "$save_msg using sub databases"
	}

	# If we are using an env, then basename should just be the db name.
	# Otherwise it is the test directory and the name.
	set eindex [lsearch -exact $args "-env"]
	set txnenv 0
	set txn ""
	if { $eindex == -1 } {
		set basename $testdir/test$tnum
		set env NULL
		set orig_testdir $testdir
	} else {
		set basename test$tnum
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set orig_testdir [get_home $env]
	}

	set extra_op ""
	if {$sort_multiple} {
		set extra_op "-sort_multiple"
	}

	if {[is_compressed $args]} {
		set options [list "-dup -dupsort"]
	}  else {
		set options [list "-dup -dupsort" "-dup"]
	}
	set i 0
	foreach ex_args $options {
		set dboargs [concat $args $ex_args]
		set fname $basename.db
		set dbname $subname
		puts "Test$tnum: $method ($dboargs)\
		    Database bulk update $save_msg."
		if {$subdb} {
			set dbname $subname.$i
		} else {
			set fname $basename.$i.db
		}
		test127_sub $fname $dbname $dboargs
		incr i
	}
}

proc test127_sub {fname dbname dboargs} {
	source ./include.tcl
	global alphabet
	upvar method method
	upvar nentries nentries
	upvar ndups ndups
	upvar tnum tnum
	upvar subdb subdb
	upvar txnenv txnenv
	upvar env env
	upvar extra_op extra_op
	upvar orig_testdir orig_testdir

	set testdir $orig_testdir
	cleanup $testdir $env

	set sorted [is_substr $dboargs "-dupsort"]
	set omethod [convert_method $method]
	
	set db [eval {berkdb_open_noerr -create -mode 0644} \
	    $dboargs $omethod $fname $dbname]
	error_check_good dbopen [is_valid_db $db] TRUE

	set txn ""
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}

	# Do bulk put.
	# First, we put half the entries using put -multiple.
	# Then, we put the rest half using put -multiple_key.

	puts "\tTest$tnum.a: Bulk put data using -multiple $extra_op."
	set key_list1 {}
	set data_list1 {}
	set key_list {}
	set datas_list {}
	for { set i 1 } { $i < $nentries / 2} { incr i } {
		lappend key_list $i
		set datas {}
		for { set j 1 } { $j <= $ndups } { incr j } {
			set str $i.$j.$alphabet
			lappend key_list1 $i
			lappend data_list1 [chop_data $method $str]
			lappend datas [chop_data $method $str]
		}
		lappend datas_list $datas
	}
	set ret [eval {$db put} $txn -multiple $extra_op \
	    {$key_list1 $data_list1}]
	error_check_good "put(-multiple $extra_op)" $ret 0

	# Put again without -overwritedup, should return DB_KEYEXIST.
	set ret [catch {eval {$db put} \
	    $txn -multiple $extra_op {$key_list1 $data_list1}} res]
	if {$sorted} {
		error_check_good \
		    {Check DB_KEYEXIST} [is_substr $res DB_KEYEXIST] 1
	} else {
		error_check_good "put_again(-multiple $extra_op)" $ret 0
	}
	# Put again with -overwritedup, should succeed
	set ret [eval {$db put} \
	    $txn -multiple $extra_op -overwritedup {$key_list1 $data_list1}]
	error_check_good "put_again(-multiple $extra_op -overwritedup)" $ret 0
	
	puts "\tTest$tnum.b: Bulk put data using -multiple_key $extra_op."
	set pair_list1 {}
	for { set i [expr $nentries / 2 ]} { $i <= $nentries} { incr i } {
		lappend key_list $i
		set datas {}
		for { set j 1 } { $j <= $ndups } { incr j } {
			set str $i.$j.$alphabet
			lappend pair_list1 $i [chop_data $method $str]
			lappend datas [chop_data $method $str]
		}
		lappend datas_list $datas
	}
	set ret [eval {$db put} $txn -multiple_key $extra_op {$pair_list1}]
	error_check_good "put(-multiple_key $extra_op)" $ret 0
	
	# Put again without -overwritedup, should return DB_KEYEXIST.
	set ret [catch {
	    eval {$db put} $txn -multiple_key $extra_op {$pair_list1}} res]
	if {$sorted} {
		error_check_good \
		    {Check DB_KEYEXIST} [is_substr $res DB_KEYEXIST] 1
	} else {
		error_check_good "put_again(-multiple_key $extra_op)" $ret 0
	}
	# Put again with -overwritedup, should succeed
	set ret [eval \
	    {$db put} $txn -multiple_key $extra_op -overwritedup {$pair_list1}]
	error_check_good \
	    "put_again(-multiple_key $extra_op -overwritedup)" $ret 0

	puts "\tTest$tnum.c: Verify the data after bulk put."
	test127_check_prirecords $db $key_list $datas_list $txn $sorted

	puts "\tTest$tnum.d: Bulk delete data using -multiple $extra_op."
	set key_list2 {}
	for { set i 1 } { $i <= $nentries} { incr i 2 } {
		lappend key_list2 $i
	}
	set ret [eval {$db del} $txn -multiple $extra_op {$key_list2}]
	error_check_good "del(-multiple $extra_op)" $ret 0

	# Delete again, should return DB_NOTFOUND.
	set ret [catch {eval {$db del} $txn -multiple $extra_op \
	    {$key_list2}} res]
	error_check_good {Check DB_NOTFOUND} [is_substr $res DB_NOTFOUND] 1

	puts "\tTest$tnum.e: Bulk delete using -multiple_key $extra_op."
	set pair_list2 {}
	for { set i 2 } { $i <= $nentries} { incr i  2} {
		for { set j 1 } { $j <= $ndups / 2 } { incr j } {
			set str $i.$j.$alphabet
			lappend pair_list2 $i [chop_data $method $str]
		}
	}

	set ret [eval {$db del} $txn -multiple_key $extra_op {$pair_list2}]
	error_check_good "del(-multiple_key $extra_op)" $ret 0

	# Delete again, should return DB_NOTFOUND.
	set ret [catch {eval {$db del} $txn -multiple_key $extra_op \
	    {$pair_list2}} res]
	if {$sorted} {
		error_check_good {Check DB_NOTFOUND} [is_substr $res DB_NOTFOUND] 1
	} else {
		error_check_good "del(-multiple_key $extra_op) 2 round" $ret 0
		set ret [catch {eval {$db del} $txn -multiple_key $extra_op \
		    {$pair_list2}} res]
		error_check_good "del(-multiple_key $extra_op) 3 round" $ret 0
		set ret [catch {eval {$db del} $txn -multiple_key $extra_op \
		    {$pair_list2}} res]
		error_check_good {Check DB_NOTFOUND} [is_substr $res DB_NOTFOUND] 1		
	}

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

	# Check all items to mare sure we do not delete other items.
	set orig_key_list $key_list
	set orig_datas_list $datas_list
	set key_list {}
	set datas_list {}
	set i 0
	set j 0
	set k 0
	while {$i < $nentries} {
		set datas [lindex $orig_datas_list $i]
		set key1 [lindex $orig_key_list $i]
		set key2 [lindex $key_list2 $j]
		set key3 [lindex $pair_list2 $k]
		if {$key1 == $key2} {
			incr i
			incr j
			continue
		} elseif {$key1 == $key3} {
			while {$key1 == $key3} {
				set data_index [expr $k + 1]
				set data [lindex $pair_list2 $data_index]
				set index [lsearch -exact $datas $data]
				error_check_bad data_index -1 $index
				set datas [lreplace $datas $index $index]
				incr k 2
				set key3 [lindex $pair_list2 $k]
			}
		}
		if {[llength $datas] > 0} {
			lappend key_list $key1
			lappend datas_list $datas
		}
		incr i
	}

	test127_check_prirecords $db $key_list $datas_list $txn $sorted

	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}

proc test127_check_prirecords {db key_list datas_list txnarg sorted} {
	
	set dbc [eval $db cursor $txnarg]
	error_check_good $dbc [is_valid_cursor $dbc $db] TRUE

	# Check if all the records are in key_list(key) and datas_list(data).
	for {set pair [$dbc get -first]} {[llength $pair] > 0} \
	    {set pair [$dbc get -next]} {
		set key [lindex [lindex $pair 0] 0]
		set data [lindex [lindex $pair 0] 1]		
		set index [lsearch -exact $key_list $key]
		error_check_bad key_index $index -1
		error_check_bad data_index -1 \
		    [lsearch -exact [lindex $datas_list $index] $data]
	}

	error_check_good $dbc.close [$dbc close] 0

	# Check if all the items in the lists are in the database.
	set len [llength $key_list]
	for {set i 0} {$i < $len} {incr i} {
		set key [lindex $key_list $i]
		set datas [lindex $datas_list $i]
		set pairs [eval $db get $txnarg $key]
		error_check_bad pairs [llength $pairs] 0
		if {$sorted} {
			error_check_good pairs [llength $pairs] [llength $datas]
		} else {
			error_check_good pairs [expr \
			    [llength $pairs] >= [llength $datas]] 1
		}
		foreach data $datas {
			set pair [list $key $data]
			error_check_bad pair_index -1 \
			    [lsearch -exact $pairs $pair]
		}
	}
}
