# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test135
# TEST  Test operations on similar overflow records. [#20329]
# TEST  Open 3 kinds of databases: one is not dup, and one is dup-unsorted
# TEST    while another one is dup-sorted.
# TEST  Then we put some similar but not identical overflow records into 
# TEST    these databases. 
# TEST  Also, we try to remove some records from these databases.
# TEST  We'll verify the records after put and after deletion.
# TEST  Here is how we define the 'similar overflow' in this test:
# TEST    * Both the key.size and data.size are not small than DB's pagesize:
# TEST      The key.size is around 2*pagesize and data.size equals to pagesize.
# TEST    * The keys are of same length, and they only differ in a small piece.
# TEST      The location of difference could be in the start/middle/end.
# TEST    * For dup databases, the dup datas have same rule as the key rule
# TEST      mentioned above.

proc test135 {method {keycnt 10} {datacnt 10} {subdb 0} {tnum "135"} args} {
	source ./include.tcl

	# We are only testing key-based methods.
	if {[is_record_based $method]} {
		puts "Skipping test$tnum for $method test."
		return
	}

	set sub_msg ""
	# Check if we use sub database.
	if { $subdb } {
		if {[is_partitioned $args]} {
			puts "Skipping test$tnum with sub database\
		       	    for partitioned $method test."
			    return
		}
		set sub_msg "using sub databases"
	}

	# Skip for specified pagesizes. This test uses a very small
	# pagesize to make it run fast.
	set pgindex [lsearch -exact $args "-pagesize"]
	if { $pgindex != -1 } {
		puts "Test$tnum: Skipping for specific pagesizes"
		return
	}

	set pgsize 512

	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	set eindex [lsearch -exact $args "-env"]
	set txnenv 0
	if { $eindex == -1 } {
		if {$subdb} {
			puts "Skipping test$tnum $sub_msg for non-env test."
			return
		}
		set basename $testdir/test$tnum
		set env NULL
	} else {
		set basename test$tnum
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env
	set args [convert_args $method $args]

	puts "Test$tnum: $method ($args)\
	    Similar Overflow Records Operation Test $sub_msg."

	set i 0
	set dupargs [list "" "-dup" "-dup -dupsort"]
	if {[is_rbtree $method]} {
		set dupargs [list ""]
	} elseif {[is_compressed $args]} {
		set dupargs [list "" "-dup -dupsort"]
	}
	foreach duparg $dupargs {
		# kloc specifies where the keys differ
		foreach kloc [list "start" "middle" "end"] {
			# dloc specifies where the dup datas differ
			foreach dloc [list "start" "middle" "end"] {
				test135_sub \
				    "\tTest$tnum.$i ($duparg $kloc $dloc)" \
				    $duparg $kloc $dloc $basename $subdb \
				    $method $i $args
				incr i
			}
		}
	}
}

proc test135_sub { prefix duparg kloc dloc basename use_subdb 
    method indx dboargs } {
	global alphabet
	upvar txnenv txnenv
	upvar env env
	upvar keycnt keycnt
	upvar datacnt datacnt
	upvar pgsize pgsize

	if { $use_subdb } {
		set testfile $basename.db
		set subname "db$indx"
	} else {
		set testfile $basename-$indx.db
		set subname ""
	}

	set isdup [is_substr $duparg "-dup"]

	puts "$prefix.a: Open the database."
	set omethod [convert_method $method]
	set db [eval {berkdb_open_noerr -create -mode 0644} \
	    -pagesize $pgsize $dboargs $duparg $omethod $testfile $subname]
	error_check_good dbopen [is_valid_db $db] TRUE

	set txn ""
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}

	set app_data(0) 0
	set alpha_len [string length $alphabet]
	set kbits [test135_countbits $alpha_len $keycnt]
	set dbits [test135_countbits $alpha_len $datacnt]

	# The key and data are composed of characters from 'a' to 'z' and they
	# have the format of: 
	#     $prefix$str$suffix
	# Both prefix and suffix are strings with all characters set to 'a'.
	# Here is an example to compose the str:
	# Assume we have 100 keys, and as 100 > 26('a'-'z', 26 chars),
	# we need at least two characters to represent 100 different strs.
	# These 100 strs should be: aa, ab, ..., az, ...., bz, ...., dv
	puts "$prefix.b: Putting records into the database."
	for {set klen [expr 2 * $pgsize - 50]} \
	    {$klen < [expr 2 * $pgsize + 50]} {incr klen 10} {
		set kpos [test135_getdiffpos $klen $kloc $kbits 10]
		set dpos [test135_getdiffpos $pgsize $dloc $dbits $alpha_len]
		set kprefix [repeat "a" $kpos]
		set ksuffix [repeat "a" \
		    [expr $klen - $kbits - $kpos]]
		set dprefix [repeat "a" $dpos]
		set dsuffix [repeat "a" \
		    [expr $pgsize - $dbits - $dpos]]
		for {set i 0} {$i < $keycnt} {incr i} {
			set k [test135_tostr $alphabet $alpha_len $kbits $i]
			if {$isdup} {
				for {set j 0} {$j < $datacnt} {incr j} {
					set d [test135_tostr $alphabet \
					    $alpha_len $dbits $j]
					set ret [eval $db put $txn \
					    $kprefix$k$ksuffix \
					    $dprefix$d$dsuffix]
					error_check_good db_put $ret 0
				}
			} else {
				set d [test135_tostr $alphabet \
				     $alpha_len $dbits 1]
				set ret [eval $db put $txn $kprefix$k$ksuffix \
				    $dprefix$d$dsuffix]
				error_check_good db_put $ret 0
			}
		}
		set app_data($klen) [list $kprefix $ksuffix \
		    $dprefix $dsuffix]
	}

	puts "$prefix.c: Verify records after putting."
	for {set klen [expr 2 * $pgsize - 50]} \
	    {$klen < [expr 2 * $pgsize + 50]} {incr klen 10} {
		set applist $app_data($klen)
		set kprefix [lindex $applist 0]
		set ksuffix [lindex $applist 1]
		set dprefix [lindex $applist 2]
		set dsuffix [lindex $applist 3]
		for {set i 0} {$i < $keycnt} {incr i} {
			set k [test135_tostr $alphabet $alpha_len $kbits $i]
			if {$isdup} {
				for {set j 0} {$j < $datacnt} {incr j} {
					set d [test135_tostr $alphabet \
					    $alpha_len $dbits $j]
					set ret [eval $db get -get_both $txn \
					    $kprefix$k$ksuffix \
					    $dprefix$d$dsuffix]
					error_check_good db_get [llength $ret] 1
				}
			} else {
				set d [test135_tostr $alphabet \
				     $alpha_len $dbits 1]
				set ret [eval $db get -get_both $txn \
				    $kprefix$k$ksuffix $dprefix$d$dsuffix]
				error_check_good db_get [llength $ret] 1
			}
		}
	}

	puts "$prefix.d: Delete some records from the database."
	for {set klen [expr 2 * $pgsize - 50]} \
	    {$klen < [expr 2 * $pgsize + 50]} {incr klen 10} {
		set applist $app_data($klen)
		set kprefix [lindex $applist 0]
		set ksuffix [lindex $applist 1]
		for {set i 0} {$i < $keycnt} {incr i 2} {
			set k [test135_tostr $alphabet $alpha_len $kbits $i]
			set ret [eval $db del $txn $kprefix$k$ksuffix]
			error_check_good db_del $ret 0
		}
	}

	puts "$prefix.e: Verify records after deleting."
	for {set klen [expr 2 * $pgsize - 50]} \
	    {$klen < [expr 2 * $pgsize + 50]} {incr klen 10} {
		set applist $app_data($klen)
		set kprefix [lindex $applist 0]
		set ksuffix [lindex $applist 1]
		set dprefix [lindex $applist 2]
		set dsuffix [lindex $applist 3]
		for {set i 0} {$i < $keycnt} {incr i 2} {
			set k [test135_tostr $alphabet $alpha_len $kbits $i]
			set ret [eval $db get $txn $kprefix$k$ksuffix]
			error_check_good db_get [llength $ret] 0
		}
		for {set i 1} {$i < $keycnt} {incr i 2} {
			set k [test135_tostr $alphabet $alpha_len $kbits $i]
			if {$isdup} {
				for {set j 0} {$j < $datacnt} {incr j} {
					set d [test135_tostr $alphabet \
					    $alpha_len $dbits $j]
					set ret [eval $db get -get_both $txn \
					    $kprefix$k$ksuffix \
					    $dprefix$d$dsuffix]
					error_check_good db_get [llength $ret] 1
				}
			} else {
				set d [test135_tostr $alphabet \
				     $alpha_len $dbits 1]
				set ret [eval $db get -get_both $txn \
				    $kprefix$k$ksuffix $dprefix$d$dsuffix]
				error_check_good db_get [llength $ret] 1
			}
		}
	}

	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_close [$db close] 0
}

# Determines how many bits we need to represent a scope of values from 0.
# For example, if we want to represent values from 0-99, we need
# at least two decimal bits.
proc test135_countbits {radix maxval} {
	set cnt 1
	while {$maxval > 0} {
		set maxval [expr $maxval / $radix]
		if {!$maxval} { 
			break
		} else {
			incr cnt
		}
	}
	return $cnt
}

# Return the string representation of a value.
# For example, if we want to represent 21 in 3 decimal bits,
# the returned value should be "021"
proc test135_tostr {src radix cnt val} {
	set str ""
	for {set i 0} {$i < $cnt} {incr i} {
		set indx [expr $val % $radix]
		set val [expr $val / $radix]
		set str [string index $src $indx]$str
	}
	return $str
}

# Return the location where the difference begins.
proc test135_getdiffpos {sz loc bits mbits} {
	set lower 0
	set upper [expr $sz - $bits]
	switch -exact $loc {
		"start" {
			set upper [expr $mbits - $bits]
		}
		"middle" {
			set upper [expr $sz / 2 - $mbits / 2 - $bits]
			set lower [expr $upper - $mbits + 1]
		}
		"end" {
			set lower [expr $sz - $mbits]
		}
	}
	return [berkdb random_int $lower $upper]
}
