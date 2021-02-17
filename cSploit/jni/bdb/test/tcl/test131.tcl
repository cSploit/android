# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test131
# TEST  Test foreign database operations.
# TEST  Create a foreign db, and put some records into it.
# TEST  Then associate the foreign db with a secondary db, and
# TEST  put records into the primary db.
# TEST  Do operations in the foreign db and check results.
# TEST  Finally, verify the foreign relation between the foreign db 
# TEST  and secondary db.
# TEST  Here, we test three different foreign delete constraints:
# TEST      - DB_FOREIGN_ABORT
# TEST      - DB_FOREIGN_CASCADE
# TEST      - DB_FOREIGN_NULLIFY

proc test131 {method {nentries 1000} {tnum "131"} {ndups 5} {subdb 0} 
    {inmem 0} args } {
	source ./include.tcl

	# For rrecno, when keys are deleted, the ones after will move forward,
	# and the keys change. It is not good to use rrecno for the primary
	# database.
	if {[is_rrecno $method]} {
		puts "Skipping test$tnum for $method test."
		return
	}

	set sub_msg ""
	# Check if we use sub databases.
	if { $subdb } {
		if {[is_queue $method]} {
			puts "Skipping test$tnum with sub database\
			    for $method."
			return
		}
		if {[is_partitioned $args]} {
			puts "Skipping test$tnum with sub database\
		       	    for partitioned $method test."
			return		
		}
		if {[is_heap $method]} {
			puts "Skipping test$tnum with sub database\
			     for $method."
			return
		}
		# Check if the sub databases should be in-memory.
		if {$inmem} {
			set sub_msg "using in-memory sub databases"
		} else {
			set sub_msg "using sub databases"
		}
	}

	# If we are using an env, then basename should just be the name prefix.
	# Otherwise it is the test directory and the name prefix.
	set eindex [lsearch -exact $args "-env"]
	set txnenv 0
	set txn ""
	if { $eindex == -1 } {
		set basename $testdir/test$tnum
		set env NULL
		if {$subdb} {
			puts "Skipping test$tnum $sub_msg for non-env test."
			return
		}
	} else {
		set basename test$tnum
		set nentries 200
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
	}

	cleanup $testdir $env

	# To simplify the case, just set the type of foreign database as 
	# btree or hash. And for the secondary database, the type is dupsort
	# hash/btree. 
	set origargs $args
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	puts "Test$tnum: $method ($args) Foreign operations $sub_msg."

	# The type-pairs for the foreign and secondary databases.
	set typepairs {
		{"-btree" "-btree"}
		{"-btree" "-hash"}
		{"-hash" "-btree"}
		{"-hash" "-hash"}
	}
	
	# The sub procs which test specific foreign operations.
	set subtests {
		{"test131_sub1" "-abort" "0"}
		{"test131_sub2" "-cascade" "0"}
		{"test131_sub3" "-nullify test131_nullify_cb1" "1"}
		{"test131_sub4" "-nullify test131_nullify_cb2" "0"}
	}

	set i 0

	foreach subtest $subtests {
		foreach typepair $typepairs {
			# Initialize the names
			set pri_subname ""
			set sec_subname ""
			set foreign_subname ""
			set pri_file $basename.db
			set sec_file $basename.db
			set foreign_file $basename.db
			if {$inmem} {
				set pri_file ""
				set sec_file ""
				set foreign_file ""
			}

			set foreign_proc [lindex $subtest 0]
			set foreign_arg [lindex $subtest 1]
			set foreign_vrfy [lindex $subtest 2]
			set foreign_method [lindex $typepair 0]
			set sec_method [lindex $typepair 1]
			if {$subdb} {
				set pri_subname "primary_db$i"
				set sec_subname "secondary_db$i"
				set foreign_subname "foreign_db$i"
			} else {
				set pri_file "${basename}_primary$i.db"
				set sec_file "${basename}_secondary$i.db"
				set foreign_file "${basename}_foreign$i.db"
			}

			puts "\tTest$tnum.$i subtest={$subtest}, \
			    typepair={$typepair}"

			# Skip partition range test and compression test
			# when there are non-btree databases.
			if {![is_btree $foreign_method] || \
			    ![is_btree $sec_method]} {
				set skip 0
				if {[is_partitioned $origargs] && \
				    ![is_partition_callback $origargs]} {
					set skip 1
				}
				if {[is_compressed $origargs]} {
					set skip 1
				}
				if {$skip} {
					puts "\tSkipping for {$origargs}"
					continue
				}
			}

			set txn ""
			if {$txnenv == 1} {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			# Open the foreign database.
			puts "\tTest$tnum.$i.a Open and truncate the databases."
			set foreign_args [convert_args $foreign_method $origargs]
			set foreign_db [eval berkdb_open_noerr -create \
			    -mode 0644 $foreign_method $foreign_args \
			    {$foreign_file} $foreign_subname]
			error_check_good foreign_db_open \
			    [is_valid_db $foreign_db] TRUE

			# Open the primary database.
			set pri_db [eval berkdb_open_noerr -create -mode 0644 \
			    $omethod $args {$pri_file} $pri_subname]
			error_check_good pri_db_open \
			    [is_valid_db $pri_db] TRUE

			# Open the secondary database.
			set secargs [convert_args $sec_method $origargs]
			set sec_db [eval berkdb_open_noerr -create -mode 0644 \
			    $sec_method $secargs -dup -dupsort \
			    {$sec_file} $sec_subname]
			error_check_good sec_db_open \
			    [is_valid_db $sec_db] TRUE

			# Truncate the databases.
			# For some tests(e.g. run_secenv), we will run this 
			# unit twice. The "cleanup $testdir" could clean the
			# on-disk database files, but it can not clean the 
			# in-memory ones, so the records are still in the 
			# in-memory databases, which will affect our test.
			# So, we need to truncate these databases first.
			error_check_good foreign_db_trunc \
			    [expr [eval $foreign_db truncate $txn] >= 0] 1
			error_check_good pri_db_trunc \
			    [expr [eval $pri_db truncate $txn] >= 0] 1
			error_check_good sec_db_trunc \
			    [expr [eval $sec_db truncate $txn] >= 0] 1

			# Establish the relations between the databases.
			set ret [eval $pri_db \
			    associate -create $txn test131_sec_cb1 $sec_db]
			error_check_good db_associate $ret 0

			set ret [eval $foreign_db \
			    associate_foreign $foreign_arg $sec_db]
			error_check_good db_associate_foreign $ret 0

			puts "\tTest$tnum.$i.b Populate the foreign database."			
			test131_populate_foreigndb $txn $foreign_db $nentries	

			# Put records into the primary database.
			puts "\tTest$tnum.$i.c Populate primary database."
			test131_populate_pridb \
			    $txn $pri_db $method $nentries $ndups

			# Check records in the secondary database.
			puts "\tTest$tnum.$i.d\
			    Check records in secondary database."
			test131_check_secdb $txn $sec_db $nentries $ndups

			# Update the foreign database.
			puts "\tTest$tnum.$i.e Update the foreign database."
			test131_update_foreigndb $txn $foreign_db $nentries

			# Delete the keys only in the foreign database.
			puts "\tTest$tnum.$i.f Delete foreign-only keys."
			test131_delitems_foreigndb $txn $foreign_db $nentries

			# Test different foreign delete constraints.
			puts "\tTest$tnum.$i.g\
			    Test specific foreign delete constraints."
			$foreign_proc $txn $foreign_db $pri_db $sec_db \
			    $nentries $ndups "Test$tnum.$i.g"

			puts "\tTest$tnum.$i.h Verifying foreign key \
			    relationships ..."
			error_check_good verify_foreign [verify_foreign \
			    $txn $foreign_db $sec_db 0] $foreign_vrfy

			if {$txnenv == 1} {
				error_check_good txn_commit [$t commit] 0
			}

			error_check_good foreign_db_close [$foreign_db close] 0
			error_check_good pri_db_close [$pri_db close] 0
			error_check_good sec_db_close [$sec_db close] 0

			incr i
		}
	}
}

# Test for DB_FOREIGN_ABORT
# Delete the keys which exist in the secondary database.
# The delete should fail because DB_FOREIGN_ABORT has been set.
proc test131_sub1 {txn foreign_db pri_db sec_db nentries ndups msghdr} {
	puts "\t\t$msghdr.1 Test DB_FOREIGN_ABORT"
	set nentries2 [expr $nentries / 2]
	for {set i 0} {$i < $nentries2} {incr i} {
		set ret [catch {eval $foreign_db del $txn $i} res]
		error_check_bad foreign_del $ret 0
		error_check_good "check DB_FOREIGN_CONFLICT" \
		    [is_substr $res DB_FOREIGN_CONFLICT] 1		
		error_check_good check_ndups \
		    [llength [eval $sec_db get $txn $i]] $ndups
		error_check_good check_foreign \
		    [llength [eval $foreign_db get $txn $i]] 1
	}
}

# Test the DB_FOREIGN_CASCADE.
# Delete the keys which exist in the secondary database.
# The delete should succeed, and all the related records in
# the secondary database should be deleted as well.
proc test131_sub2 {txn foreign_db pri_db sec_db nentries ndups msghdr} {
	puts "\t\t$msghdr.1 Test DB_FOREIGN_CASCADE"
	set nentries2 [expr $nentries / 2]
	for {set i 0} {$i < $nentries2} {incr i} {
		set ret [catch {eval $foreign_db del $txn $i} res]
		error_check_good foreign_del $ret 0
		error_check_good check_ndups \
		    [llength [eval $sec_db get $txn $i]] 0
		error_check_good check_foreign \
		    [llength [eval $foreign_db get $txn $i]] 0
	}
}

# Test the DB_FOREIGN_NULLIFY with a bad nullify function(
# the nullify function does not change the data).
# Delete the keys which exist in the secondary database.
# The delete should fail, but the records in the foreign
# database should be removed anyway, while the related records
# in the primary/secondary are still there.
proc test131_sub3 {txn foreign_db pri_db sec_db nentries ndups msghdr} {
	puts "\t\t$msghdr.1 \
	    Test DB_FOREIGN_NULLIFY with a bad nullify function."
	set nentries2 [expr $nentries / 2]
	for {set i 0} {$i < $nentries2} {incr i} {
		set ret [catch {eval $foreign_db del $txn $i} res]
		error_check_good foreign_del $ret 0
		error_check_good check_ndups \
		    [llength [eval $sec_db get $txn $i]] $ndups
		error_check_good check_foreign \
		    [llength [eval $foreign_db get $txn $i]] 0
	}
}

# Test the DB_FOREIGN_NULLIFY with a good nullify function.
# Delete the keys which exist in the secondary database.
# The delete should succeed, and the records in the foreign
# database should be removed, while the related records
# in the primary/secondary are changed.
proc test131_sub4 {txn foreign_db pri_db sec_db nentries ndups msghdr} {
	puts "\t\t$msghdr.1 \
	    Test DB_FOREIGN_NULLIFY with a good nullify function."
	set nentries2 [expr $nentries / 2 - 1]
	for {set i 0} {$i < $nentries2} {incr i 2} {
		set ret [catch {eval $foreign_db del $txn $i} res]
		error_check_good foreign_del $ret 0
		error_check_good check_ndups \
		    [llength [eval $sec_db get $txn $i]] 0
		error_check_good check_foreign \
		    [llength [eval $foreign_db get $txn $i]] 0
		set newkey [expr $i + 1]
		error_check_good check_ndups \
		    [llength [eval $sec_db get $txn $newkey]] [expr $ndups * 2]
		error_check_good check_foreign \
		    [llength [eval $foreign_db get $txn $newkey]] 1
	}
}

# The callback function for secondary database.
proc test131_sec_cb1 {pkey pdata} {
	set indx [string first "-" $pdata]
	error_check_bad good_indx [expr $indx > 0] 0
	return [string range $pdata 0 [expr $indx - 1]]
}

# The 1st callback function for foreign.
proc test131_nullify_cb1 {pkey pdata fkey} {
	return $pdata
}

# The 2nd callback function for foreign.
proc test131_nullify_cb2 {pkey pdata fkey} {
	# We should make sure the size does not grow, since
	# that will cause queue test to fail.
	set indx [string first "-" $pdata]
	error_check_bad good_indx [expr $indx > 0] 0
	set num [string range $pdata 0 [expr $indx - 1]]
	set str [string range $pdata [expr $indx + 1] end]
	return "[expr $num + 1]-$str"
}

# Put records into the foreign database.
proc test131_populate_foreigndb {txn foreign_db nentries} {
	global alphabet
	for {set i 0} {$i < $nentries} {incr i} {
		set ret [eval $foreign_db put $txn $i $i$alphabet]	       
		error_check_good foreign_put $ret 0
	}
}

# Update the records in the foreign database.
proc test131_update_foreigndb {txn foreign_db nentries} {
	global alphabet
	for {set i 0} {$i < $nentries} {incr i} {
		set ret [eval $foreign_db get $txn $i]
		error_check_good check_pair [llength $ret] 1
		set key [lindex [lindex $ret 0] 0]
		set data [lindex [lindex $ret 0] 1]
		error_check_good check_key $key $i
		set ret [eval $foreign_db put $txn $key $data-NEW]
		error_check_good foreign_put $ret 0
	}
}

# Delete items which only exist in the foreign database.
proc test131_delitems_foreigndb {txn foreign_db nentries} {
	global alphabet
	set nentries2 [expr $nentries / 2]
	for {set i $nentries2} {$i < $nentries} {incr i 2} {
		set ret [eval $foreign_db del $txn $i]	       
		error_check_good foreign_del $ret 0
		set ret [eval $foreign_db get $txn $i]
		error_check_good check_empty [llength $ret] 0
	}
}

# Put records into the primary database.
# Here we make every key in the secondary database
# has $ndups duplicate records.
proc test131_populate_pridb {txn pri_db pri_method nentries ndups} {
	source ./include.tcl
	global alphabet
	
	set lastindx [expr [string length $alphabet] - 1]
	# We just cover the first half of keys in the foreign database.
	set nentries2 [expr $nentries / 2]
	set number 1
	for {set i 0} {$i < $nentries2} {incr i} {
		for {set j 0} {$j < $ndups} {incr j} {
			set beg [berkdb random_int 0 $lastindx]
			set end [berkdb random_int 0 $lastindx]
			if {$beg <= $end} {
				set str [string range $alphabet $beg $end]
			} else {
				set str [string range $alphabet $end $beg]
			}
			if {[is_record_based $pri_method] == 1} {
				set key $number
			} else {
				set key $number$str
			}
			set ret [eval $pri_db put $txn $key \
			    [make_fixed_length $pri_method "$i-$str"]]
			error_check_good pri_db_put $ret 0
			error_check_good pri_db_get \
			    [llength [eval $pri_db get $txn $key]] 1		
			incr number
		}
	}

	set nentries2 [expr $nentries + $nentries2]
	for {set i $nentries} {$i < $nentries2} {incr i} {
		for {set j 0} {$j < $ndups} {incr j} {
			set beg [berkdb random_int 0 $lastindx]
			set end [berkdb random_int 0 $lastindx]
			if {$beg <= $end} {
				set str [string range $alphabet $beg $end]
			} else {
				set str [string range $alphabet $end $beg]
			}
			if {[is_record_based $pri_method] == 1} {
				set key $number
			} else {
				set key $number$str
			}			
			set ret [catch {eval $pri_db put $txn $key \
			    [make_fixed_length $pri_method "$i-$str"]} res]
			error_check_bad pri_db_put $ret 0
			error_check_good "check DB_FOREIGN_CONFLICT" \
			    [is_substr $res DB_FOREIGN_CONFLICT] 1
			error_check_good pri_db_get \
			    [llength [eval $pri_db get $txn $key]] 0
			incr number
		}
	}
}

# Check the records in the secondary database.
proc test131_check_secdb {txn sec_db nentries ndups} {
	set nentries2 [expr $nentries / 2]
	for {set i 0} {$i < $nentries2} {incr i} {
		set ret [eval $sec_db get $txn $i]
		error_check_good check_rec_num [llength $ret] $ndups
	}
	for {set i $nentries2} {$i < $nentries} {incr i} {
		set ret [eval $sec_db get $txn $i]
		error_check_good check_rec_num [llength $ret] 0
	}

	set nentries2 [expr $nentries + $nentries2]
	for {set i $nentries} {$i < $nentries2} {incr i} {
		set ret [eval $sec_db get $txn $i]
		error_check_good check_rec_num [llength $ret] 0
	}
}
