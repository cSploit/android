# See the file LICENSE for redistribution information.
#
# Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test149
# TEST	Database stream test.
# TEST	1. Append data to empty / non-empty blobs.
# TEST	2. Update the existing data in the blobs.
# TEST	3. Re-create blob of the same key by deleting the record and
# TEST	and writing new data to blob by database stream.
# TEST	4. Verify the error is returned when opening a database stream
# TEST	on a record that is not a blob.
# TEST	5. Verify database stream can not write in blobs when it is
# TEST	configured to read-only.
# TEST	6. Verify database stream can not write in read-only databases.
# TEST
# TEST	In each test case, verify database stream read/size/write/close
# TEST	operations work as expected with transaction commit/abort.
proc test149 { method {tnum "149"} args } {
	source ./include.tcl
	global alphabet
	global databases_in_memory
	global has_crypto

	if { $databases_in_memory } {
		puts "Test$tnum skipping for in-memory database."
		return
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# Blob is supported by btree, hash and heap.
	if { [is_btree $omethod] != 1 && \
	    [is_hash $omethod] != 1 && [is_heap $omethod] != 1 } {
		puts "Test$tnum skipping for method $method."
		return
	}

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	#
	if { $eindex == -1 } {
		set testfile $testdir/test$tnum.db
		set env NULL
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

	# Look for incompatible configurations of blob.
	foreach conf { "-encryptaes" "-encrypt" "-compress" "-dup" "-dupsort" \
	    "-read_uncommitted" "-multiversion" } {
		if { [lsearch -exact $args $conf] != -1 } {
			puts "Test$tnum skipping $conf."
			return
		}
	}
	if { $env != "NULL" } {
		if { [lsearch [$env get_flags] "-snapshot"] != -1 } {
			puts "Test$tnum skipping -snapshot."
			return
		}
		if { [is_repenv $env] == 1 } {
			puts "Test$tnum skipping replication env."
			return
		}
		if { $has_crypto == 1 } {
			if { [$env get_encrypt_flags] != "" } {
				puts "Test$tnum skipping encrypted env."
				return
			}
		}
	}
	if { [lsearch -exact $args "-chksum"] != -1 } {
		set indx [lsearch -exact $args "-chksum"]
		set args [lreplace $args $indx $indx]
		puts "Test$tnum ignoring -chksum for blob."
	}

	puts "Test$tnum: ($omethod $args) Database stream basic operations."

	puts "\tTest$tnum.a: create the blob database."
	if { $env != "NULL" } {
		set testdir [get_home $env]
	}
	cleanup $testdir $env

	#
	# It doesn't matter what blob threshold value we choose, since the
	# test will create blobs by -blob.
	#
	set bflags "-blob_threshold 100"
	set blrootdir $testdir/__db_bl
	if { $env == "NULL" } {
		append bflags " -blob_dir $blrootdir"
	}
	set db [eval {berkdb_open_noerr -create -mode 0644} \
	    $bflags $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	set step { b c d e f g }
	set cnt 0
	set startkey 10
	#
	# When blobdata is empty, it creates empty blobs.
	# Offset indicates the position where database stream write begins.
	# When the blob is empty, offset is 0.
	#
	foreach blobdata [list "" $alphabet ] {
		#
		# Test updating blobs in the beginning/middle/end of the
		# blob or after the end.
		#
		foreach offset { 0 13 29 } {
			# Set up the put message.
			set msg [lindex $step $cnt]
			set msg $tnum.$msg

			# Create blobs by database put method.
			set txn ""
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			set ridlist [create_blobs \
			    $db NULL $txn $startkey $blobdata ${msg}1]
			#
			# Get the rids for heap which will be
			# used in the verification.
			#
			if { [llength $ridlist] != 0 } {
				array set rids $ridlist
			}

			if { [is_heap $omethod] != 1 } {
				set ridlist NULL
			}
			if { $txnenv == 1 } {
				puts "\tTest${msg}1.1: abort the txn."
				error_check_good txn_abort [$t abort] 0

				# Verify no new blobs are created.
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
				verify_update_blobs $db NULL $txn $startkey \
				    $ridlist $blobdata FALSE -1 ${msg}1.2

				# Create blobs again.
				set ridlist [create_blobs $db NULL \
				    $txn $startkey $blobdata ${msg}1.3]
				#
				# Get the rids for heap which will be
				# used in the verification.
				#
				if { [llength $ridlist] != 0 } {
					array set rids $ridlist
				}

				if { [is_heap $omethod] != 1 } {
					set ridlist NULL
				}
				puts "\tTest${msg}1.4: commit the txn."
				error_check_good txn_commit [$t commit] 0

				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}

			#
			# Verify blobs are created and
			# update them by database stream.
			#
			verify_update_blobs $db NULL $txn \
			    $startkey $ridlist $blobdata TRUE $offset ${msg}2
			if { $txnenv == 1 } {
				error_check_good txn_commit [$t commit] 0
			}

			incr startkey 10

			#
			# Create blobs by cursor put method on btree/hash
			# database.
			# Skip this on heap database since it will insert
			# new blobs and we do not know the new rids in advance.
			#

			if { [is_heap $omethod] != 1 } {
				if { $txnenv == 1 } {
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				# Open the cursor
				set dbc [eval {$db cursor} $txn]
				error_check_good cursor_open \
				    [is_valid_cursor $dbc $db] TRUE

				create_blobs NULL $dbc $txn \
				    $startkey $blobdata ${msg}3

				if { $txnenv == 1 } {
					# Close cursor before aborting the txn.
					error_check_good cursor_close \
					    [$dbc close] 0

					puts "\tTest${msg}3.1: abort the txn."
					error_check_good txn_abort [$t abort] 0

					# Open the cursor.
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
					set dbc [eval {$db cursor} $txn]
					error_check_good cursor_open \
					    [is_valid_cursor $dbc $db] TRUE

					# Verify no new blobs are created.
					verify_update_blobs NULL $dbc $txn \
					    $startkey NULL $blobdata \
					    FALSE -1 ${msg}3.2

					# Create the blobs again.
					create_blobs NULL $dbc $txn \
					    $startkey $blobdata ${msg}3.3

					puts "\tTest${msg}3.4: commit the txn."
					# Close the cursor before commit the txn.
					error_check_good cursor_close \
					    [$dbc close] 0
					error_check_good txn_commit [$t commit] 0

					# Re-open the txn and cursor.
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
					set dbc [eval {$db cursor} $txn]
					error_check_good cursor_open \
					    [is_valid_cursor $dbc $db] TRUE
				}

				#
				# Verify blobs are created and
				# update them by database stream.
				#
				verify_update_blobs NULL $dbc $txn $startkey \
				    NULL $blobdata TRUE $offset ${msg}4
				if { $txnenv == 1 } {
					error_check_good txn_commit [$t commit] 0
				}

				incr startkey 10
				incr cnt
			}
		}
	}

	puts "\tTest$tnum.h: Re-create blob of the same key."

	puts "\tTest$tnum.h1: delete the blob."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set key [expr $startkey - 1]
	if { [is_heap $omethod] == 1 } {
		set key $rids(9)
	}
	set ret [eval {$db get} $txn {$key}]
	error_check_bad db_get [llength $ret] 0
	error_check_good db_delete [eval {$db del} $txn {$key}] 0

	if { $txnenv == 1 } {
		puts "\tTest$tnum.h1.1: abort the txn."
		error_check_good txn_abort [$t abort] 0

		puts "\tTest$tnum.h1.2: verify the blob is not deleted."
		set ret [eval {$db get $key}]
		error_check_bad db_get [llength $ret] 0

		# Delete the blob again.
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
		error_check_good db_delete [eval {$db del} $txn {$key}] 0

		puts "\tTest$tnum.h1.3: commit the txn."
		error_check_good txn_commit [$t commit] 0

		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}

	puts "\tTest$tnum.h2: verify the blob is deleted."
	set ret [eval {$db get} $txn {$key}]
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_get [llength $ret] 0

	puts "\tTest$tnum.h3: create an empty blob for the same key."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set data ""
	error_check_good db_put [eval {$db put} -blob $txn {$key $data}] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	set res [$db get $key]
	error_check_bad db_get [llength $res] 0
	error_check_good cmp_data \
	    [string length [lindex [lindex $res 0] 1]] 0

	puts "\tTest$tnum.h4: verify the empty blob."
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	# Open the cursor.
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE
	set ret [catch {eval {$dbc get} -set {$key}} res]
	error_check_good cursor_get $ret 0
	error_check_good cmp_data \
	    [string length [lindex [lindex $res 0] 1]] 0

	# Open the database stream.
	set dbs [$dbc dbstream]
	error_check_good dbstream_open [is_valid_dbstream $dbs $dbc] TRUE
	error_check_good dbstream_size [$dbs size] 0

	puts "\tTest$tnum.h5: add data into the blob by database stream."
	error_check_good dbstream_write [$dbs write -offset 0 $alphabet] 0

	puts "\tTest$tnum.h6: verify the updated blob."
	# Verify the update by database stream.
	error_check_good dbstream_size [$dbs size] 26
	error_check_good dbstream_read [string compare \
	    $alphabet [$dbs read -offset 0 -size 26]] 0
	error_check_good dbstream_close [$dbs close] 0

	# Verify the update by cursor.
	set ret [catch {eval {$dbc get} -set {$key}} res]
	error_check_good cursor_get $ret 0
	error_check_good cmp_data [string compare \
	    $alphabet [lindex [lindex $res 0] 1]] 0

	# Close the cursor.
	error_check_good cursor_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	puts "\tTest$tnum.i: verify error is returned when opening\
	    a database stream on a record that is not a blob."
	# Insert a non-blob record.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set key $startkey
	if { [is_heap $omethod] == 1 } {
		set ret [catch {eval {$db put} $txn -append {abc}} key]
	} else {
		set ret [eval {$db put} $txn {$key abc}]
	}
	error_check_good db_put $ret 0

	# Open the cursor.
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE
	set ret [eval {$dbc get} -set {$key}]
	error_check_bad cursor_get [llength $ret] 0

	# Open the database stream.
	set ret [catch {eval {$dbc dbstream}} res]
	error_check_bad dbstream_open $ret 0
	error_check_good dbstream_open \
	    [is_substr $res "cursor does not point to a blob"] 1

	puts "\tTest$tnum.j: verify database stream can not write\
	    in blobs when it is configured to read-only."
	# Set cursor on last blob record.
	set key [expr $startkey - 1]
	if { [is_heap $omethod] == 1 } {
		set key $rids(9)
	}
	set ret [catch {eval {$dbc get} -set {$key}} res]
	error_check_good cursor_get $ret 0
	error_check_bad cursor_get [llength $res] 0

	# Open the database stream as read only.
	set dbs [$dbc dbstream -rdonly]
	error_check_good dbstream_open [is_valid_dbstream $dbs $dbc] TRUE

	set ret [catch {eval {$dbs write -offset 0 abc}} res]
	error_check_bad dbstream_write $ret 0
	error_check_good dbstream_write [is_substr $res "blob is read only"] 1

	# Close the database stream and cursor.
	error_check_good dbstream_close [$dbs close] 0
	error_check_good cursor_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	puts "\tTest$tnum.k: verify database stream\
	    can not write in read-only databases."
	# Re-open the database as read only.
	error_check_good db_close [$db close] 0
	set db [eval {berkdb_open_noerr -rdonly} \
	    $bflags $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	# Open the cursor.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set dbc [eval {$db cursor} $txn]
	error_check_good cursor_open [is_valid_cursor $dbc $db] TRUE

	#
	# The last record put by $key is not a blob. So move the cursor
	# to the previous record.
	#
	set ret [eval {$dbc get} -set {$key}]
	error_check_bad cursor_get [llength $ret] 0
	set ret [eval {$dbc get} -prev]
	error_check_bad cursor_get [llength $ret] 0

	# Open the database stream.
	set dbs [$dbc dbstream]
	error_check_good dbstream_open [is_valid_dbstream $dbs $dbc] TRUE

	set ret [catch {eval {$dbs write -offset 0 abc}} res]
	error_check_bad dbstream_write $ret 0
	error_check_good dbstream_write [is_substr $res "blob is read only"] 1

	# Close the database stream and cursor.
	error_check_good dbstream_close [$dbs close] 0
	error_check_good cursor_close [$dbc close] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	error_check_good db_close [$db close] 0
}

proc create_blobs { db dbc txn startkey data msg } {
	source ./include.tcl
	global alphabet

	if { $data == "" } {
		puts -nonewline "\tTest$msg: create empty blobs "
	} else {
		puts -nonewline "\tTest$msg: create non-empty blobs "
	}
	if { $db != "NULL" } {
		puts "by database put."
	} else {
		puts "by cursor put."
	}

	#
	# Put by cursor when the database handle passed in is NULL,
	# and create empty blobs if "data" is empty.
	# 
	for { set i $startkey ; set cnt 0 } \
	    { $cnt < 10 } { incr i ; incr cnt} {
		set d ""
		if { $data != "" } {
			set d $i.$data
		}
		if { $db != "NULL" } {
			# Return the rid for heap database.
			if { [is_heap [$db get_type]] == 1 } {
				set ret [catch {eval {$db put} \
				    $txn -append -blob {$d}} rids($cnt)]
			} else {
				set ret [eval {$db put} $txn -blob {$i $d}]
			}
			error_check_good db_put $ret 0
		} else {
			error_check_good cursor_put \
			    [$dbc put -keyfirst -blob $i $d] 0
		}
	}

	# Return the rids.
	if { $db != "NULL" && [is_heap [$db get_type]] == 1 } {
		return [array get rids]
	}
}

proc verify_update_blobs { db dbc txn startkey ridlist data \
    exist offset msg } {
	source ./include.tcl
	global alphabet

	#
	# If exist is TRUE, it means the blobs to verify should be created
	# so db/dbc get should return the expected data. Otherwise it
	# should not return any data.
	# Verify blobs by db get when the db handle is NULL, otherwise
	# by cursor.
	# If offset >= 0, update and verify blobs by database stream.
	#
	set pmsg "created"
	if { $exist != TRUE } {
		set pmsg "not created"
	}
	puts -nonewline "\tTest$msg: verify blobs are $pmsg"
	if { $db != "NULL" } {
		puts -nonewline " by database get"
	} else {
		puts -nonewline " by cursor get"
	}
	if { $offset >= 0 } {
		if { $data == "" } {
			set pmsg "append data to\
			    empty blobs with offset $offset"
		} elseif { $offset == 29 } {
			set pmsg "append data to non-empty blobs"
		} elseif { $offset != 0 } {
			set pmsg "update in the middle of blobs"
		} else {
			set pmsg "update from the beginning of blobs"
		}
		puts ", $pmsg by database stream."
	} else {
		puts "."
	}

	#
	# For heap database, ridlist is the list of rids. For btree/hash,
	# it is NULL and each key used in get is $i.
	#
	if { $ridlist != "NULL" } {
		array set keys $ridlist
	} else {
		set keys ""
	}
	for { set i $startkey ; set cnt 0 } \
	    { $cnt < 10 } { incr i ; incr cnt} {
		#
		# For heap database, "key" is the list of rids. For btree/hash,
		# "key" is NULL and each key used in get is $i.
		#
		set k $i
		if { $ridlist != "NULL" } {
			set k $keys($cnt)
		}
		#
		# If "data" is not empty, each data returned should be
		# $i.$data. Otherwise it should be "".
		#
		set d ""
		if { $data != "" } {
			set d $i.$data
		}
		if { $db != "NULL" } {
			set ret [catch {eval {$db get} $txn {$k}} res]
		} else {
			set ret [catch {eval {$dbc get} -set {$k}} res]
		}
		if { $exist == TRUE } {
			error_check_good db/dbc_get $ret 0
			error_check_bad db/dbc_get [llength $res] 0

			# Verify the data.
			if { $d == "" } {
				error_check_good cmp_data [string length \
				    [lindex [lindex $res 0] 1]] 0
			} else {
				error_check_good cmp_data [string compare \
				    $d [lindex [lindex $res 0] 1]] 0
			}

			# Update and verify the update by database stream.
			if { $db == "NULL" && $offset >= 0 } {

				# Open database stream.
				set dbs [$dbc dbstream -sync]
				error_check_good dbstream_open \
				    [is_valid_dbstream $dbs $dbc] TRUE

				# Verify the blob size.
				set size [$dbs size]
				error_check_good dbstream_size \
				    $size [string length $d]

				# Verify the blob data.
				if { $d == "" } {
					error_check_good dbstream_read \
					    [string length [$dbs read \
					    -offset $offset -size $size]] 0
				} else {
					error_check_good dbstream_read \
					    [string compare $d [$dbs read \
					    -offset 0 -size $size]] 0
                                }

				# Calculate the expected data after update.
				set len [string length $alphabet]
				if { $offset >= $size } {
					if { $d != "" } {
						set dstr $d$alphabet
					} else {
						set dstr $alphabet
					}
				} else {
					if { $offset == 0 } {
						set dstr $alphabet
					} else {
						set substr [string range $d \
						    0 [expr $offset - 1]]
						set dstr $substr$alphabet
					}
					if { [expr $offset + $len] < $size } {
						set substr [string range $d \
						    [expr $offset + $len] \
						    $size]
						set dstr $dstr$substr
					}
				}

				# Update the blob data.
				error_check_good dbstream_write [$dbs write \
				    -offset $offset $alphabet] 0

				# Verify the blob size after update.
				if { [expr $offset + $len] > $size } {
					set size [expr $offset + $len]
				}
				error_check_good dbstream_size [$dbs size] $size
				set size [$dbs size]

				# Verify the blob data after update.
				set str [$dbs read -offset 0 -size $size]
				if { $d == "" &&  $offset != 0 } {
					set str \
					    [string range $str $offset $size]
					error_check_good dbstream_read \
					    [string compare $str $alphabet] 0
				} else {
					error_check_good dbstream_read \
					    [string compare $dstr $str] 0
				}

				# Close the database stream.
				error_check_good dbstream_close [$dbs close] 0
			}
		} else {
			error_check_good db/dbc_get $ret 0
			error_check_good db/dbc_get [llength $res] 0
		}
	}
}
