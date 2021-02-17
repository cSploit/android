# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test008
# TEST	Small keys/large data with overflows or BLOB.
# TEST		Put/get per key
# TEST		Loop through keys by steps (which change)
# TEST		    ... delete each key at step
# TEST		    ... add each key back
# TEST		    ... change step
# TEST		Confirm that overflow pages are getting reused or blobs
# TEST		are created.
# TEST
# TEST	Take the source files and dbtest executable and enter their names as
# TEST	the key with their contents as data.  After all are entered, begin
# TEST	looping through the entries; deleting some pairs and then readding them.
proc test008 { method {reopen "008"} {debug 0} args} {
	source ./include.tcl
	global alphabet
	global has_crypto
	global databases_in_memory

	set testname test$reopen
	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# Test overflow case for btree and hash since only btree and hash
	# have overflow pages.
	# Test blob case for btree, hash and heap.
	if { [is_btree $omethod] != 1 && \
	    [is_hash $omethod] != 1 && [is_heap $omethod] != 1 } {
		puts "Test$reopen skipping for method $method"
		return
	}

	# Create the database and open the dictionary
	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.  Just set up 
	# the "basename" here; we will save both the overflow db and the
	# blob db by adding $opt into the database name.
	if { $eindex == -1 } {
		set testfile $testdir/$testname
		set env NULL
		set blrootdir $testdir/__db_bl
		set vrflags "-blob_dir $blrootdir"
	} else {
		set testfile $testname
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testdir [get_home $env]
		set blrtdir [$env get_blob_dir]
		if { $blrtdir == "" } {
			set blrtdir __db_bl
		}
		set blrootdir $testdir/$blrtdir
		set vrflags "-env $env"
	}

	# Look for incompatible configurations of blob.
	set skipblob 0
	foreach conf { "-encryptaes" "-encrypt" "-compress" "-dup" "-dupsort" \
	    "-read_uncommitted" "-multiversion" } {
		if { [lsearch -exact $args $conf] != -1 } {
			set skipblob 1
			set skipmsg "Test$reopen skipping $conf for blob"
			break
		}
	}
	if { $skipblob == 0 && $env != "NULL" && \
	    [lsearch [$env get_flags] "-snapshot"] != -1 } {
		set skipblob 1
		set skipmsg "Test$reopen skipping -snapshot for blob"
	}
	if { $skipblob == 0 && $databases_in_memory } {
		set skipblob 1
		set skipmsg "Test$reopen skipping in-memory database for blob"
	}
	if { $has_crypto == 1 && $skipblob == 0 && $env != "NULL" } {
	    	if {[$env get_encrypt_flags] != "" } {
			set skipblob 1
			set skipmsg "Test$reopen skipping security environment"
		}
	}

	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3
	set t4 $testdir/t4

	# Here is the loop where we put and get each key/data pair
	set file_list [get_file_list]

	set msg "filename=key filecontents=data pairs"
	set pflags ""
	set gflags ""
	set fsflags ""
	if { [is_heap $omethod] == 1 } {
		set msg "filecontents=data"
		set pflags -append
		set fsflags -n
	}
	foreach opt { "overflow" "blob" } {
		puts -nonewline "$testname: $method $msg $args (with $opt)"
		if { $reopen == "009" } {
			puts " (with close)"
		} else {
			puts ""
		}

		#
		# Set a blob threshold so that some data items will be saved
		# as blobs and others as regular data items. 
		#
		set bflags ""
		if { $opt == "blob" } {
			if { $skipblob != 0 } {
				puts $skipmsg
				continue
			} elseif { $env != "NULL" && [is_repenv $env] == 1 } {
				puts "Test$reopen\
				    skipping blob for replication."
				continue
			} elseif { [lsearch -exact $args "-chksum"] != -1 } {
				set indx [lsearch -exact $args "-chksum"]
				set args [lreplace $args $indx $indx]
				puts "Test$reopen ignoring -chskum for blob"
			}
			set bflags "-blob_threshold 30"
			if { $env == "NULL" } {
				append bflags " -blob_dir $blrootdir"
			}
		} elseif { [is_heap $omethod] == 1 } {
			puts "Test$reopen\
			    skipping for method $method for overflow."
			continue
		}

		if { $env != "NULL" } {
			set testdir [get_home $env]
		}
		cleanup $testdir $env

		set db [eval {berkdb_open -create -mode 0644} \
		    $bflags $args $omethod $testfile-$opt.db]
		error_check_good dbopen [is_valid_db $db] TRUE
		if { $opt == "blob" } {
			error_check_good blob_threshold \
			    [$db get_blob_threshold] 30
			set blsubdir [$db get_blob_sub_dir]
			set blobdir $blrootdir/$blsubdir
		}

		set txn ""

		puts "\tTest$reopen.a: Initial put/get loop"
		# When blob is enabled, first put some regular data items and
		# verify there is no blob created. Then put some big data
		# items and verify they are saved as blobs.
		# When blob is disabled, just put some big data items so
		# overflow pages are created.
		set step 1
		if { $opt == "blob" } {
			puts "\tTest$reopen.a$step: Put some regular data\
			    items and verify there is no blob created."
			incr step
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			for { set i 0 } { $i < 100 } { incr i } {
				if { [is_heap $omethod] == 1 } {
					set ret [catch {eval {$db put} \
					    $txn -append {$i.$alphabet}} rids($i)]
				} else {
					set ret [eval {$db put} \
					    $txn $pflags {$i $i.$alphabet}]
				}
				error_check_good db_put $ret 0
			}
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
			error_check_bad no_blob_files [file exists $blobdir] 1

			# Skip putting the blobs by cursor in heap database.
			if { [is_heap $omethod] != 1 } {
				puts "\tTest$reopen.a$step: Put and get\
				    blobs by cursor, and verify blobs are\
				    created."
				incr step
				if { $txnenv == 1 } {
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				set dbc [eval {$db cursor} $txn]
				error_check_good cursor_open \
				    [is_valid_cursor $dbc $db] TRUE
				for { set i 100 } { $i < 110 } { incr i } {
					error_check_good cursor_put [$dbc put \
					    -keyfirst -blob $i $i.abc] 0
					set pair [$dbc get -set $i]
					error_check_bad cursor_get \
					    [llength $pair] 0
					error_check_good cmp_data \
					    [string compare $i.abc \
					    [lindex [lindex $pair 0] 1]] 0

				}
				error_check_good cursor_close [$dbc close] 0
				if { $txnenv == 1 } {
					error_check_good txn [$t commit] 0
				}

				set blfnum [llength \
				    [glob -nocomplain $blobdir/__db.bl*]]
				error_check_good blob_file_created $blfnum 10
				error_check_good blob_meta_db \
				    [file exists $blobdir/__db_blob_meta.db] 1
			}

			#
			# Delete the regular data items just put since they
			# do not work in dump_bin_file.
			#
			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			for { set i 0 } { $i < 110 } { incr i } {
				set key $i
				if { [is_heap $omethod] == 1 } {
					if { $i >= 100 } {
						break
					}
					set key $rids($i)
				}
				set r [eval {$db del} $txn {$key}]
				error_check_good db_del:$i $r 0
			}
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
			if { [is_heap $omethod] == 1 } {
				array unset rids
			}
			set blfnum [llength \
			    [glob -nocomplain $blobdir/__db.bl*]]
			error_check_good blob_file_deleted $blfnum 0
		}
		puts "\tTest$reopen.a$step: Put some big data items."
		incr step
		set count 0
		foreach f $file_list {
			set names($count) $f
			set key $f

			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			if { [is_heap $omethod] == 1 } {
				set rids($count) \
				    [test008_heap_put_file $db $txn $f]
				set key $rids($count)
			} else {
				put_file $db $txn $pflags $f
			}
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}

			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			get_file $db $txn $gflags $key $t4
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}

			error_check_good Test$reopen:diff($f,$t4) \
			    [filecmp $f $t4] 0

			incr count
		}
		error_check_good db_sync [$db sync] 0

		if { $opt == "blob" } {
			puts "\tTest$reopen.a$step: Verify\
			    blobs are created and run db_verify."
			set blfnum [llength \
			    [glob -nocomplain $blobdir/__db.bl*]]
			error_check_good blob_file_created [expr $blfnum > 0] 1
			error_check_good blob_meta_db \
			    [file exists $blobdir/__db_blob_meta.db] 1

			# Run verify to check the internal structure and order.
			if { [catch {eval {berkdb dbverify} \
			    $vrflags $testfile-$opt.db} res] } {
				error "FAIL: Verification failed with $res"
			}
		} else {
			puts "\tTest$reopen.a$step:\
			    Verify overflow pages are created."
			set ovf [stat_field $db stat "Overflow pages"]
			error_check_good overflow_pages [expr $ovf > 0] 1
		}

		if {$reopen == "009"} {
			error_check_good db_close [$db close] 0

			set bflags ""
			if { $opt == "blob" && $env == "NULL" } {
				set bflags "-blob_dir $blrootdir"
			}
			set db [eval {berkdb_open} $bflags $args $testfile-$opt.db]
			error_check_good dbopen [is_valid_db $db] TRUE
			if { $opt == "blob" } {
				error_check_good blob_threshold \
				    [$db get_blob_threshold] 30
			}
		}

		#
		# Now we will get step through keys again (by increments) and
		# delete all the entries, then re-insert them.
		#

		puts "\tTest$reopen.b: Delete re-add loop"
		foreach i "1 2 4 8 16" {
			for {set ndx 0} {$ndx < $count} { incr ndx $i} {
				if { $txnenv == 1 } {
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				set key $names($ndx)
				if { [is_heap $omethod] == 1 } {
					set key $rids($ndx)
				}
				set r [eval {$db del} $txn {$key}]
				error_check_good db_del:$key $r 0
				if { $txnenv == 1 } {
					error_check_good txn [$t commit] 0
				}
			}
			for {set ndx 0} {$ndx < $count} { incr ndx $i} {
				if { $txnenv == 1 } {
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				if { [is_heap $omethod] == 1 } {
					set rids($ndx) \
					    [test008_heap_put_file \
					    $db $txn $names($ndx)]
				} else {
					put_file $db $txn $pflags $names($ndx)
				}
				if { $txnenv == 1 } {
					error_check_good txn [$t commit] 0
				}
			}
		}
		error_check_good db_sync [$db sync] 0

		if {$reopen == "009"} {
			error_check_good db_close [$db close] 0
			set db [eval {berkdb_open} $bflags $args $testfile-$opt.db]
			error_check_good dbopen [is_valid_db $db] TRUE
			if { $opt == "blob" } {
				error_check_good blob_threshold \
				    [$db get_blob_threshold] 30
			}
		}

		#
		# Now, reopen the file and make sure the key/data pairs
		# look right.
		#
		puts "\tTest$reopen.c: Dump contents forward"
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		if { [is_heap $omethod] == 1 } {
			set nameslist [array get names]
			set ridslist [array get rids]
			test008_heap_dump_bin_file_direction \
			    $db $txn $t1 forward $nameslist $ridslist
		} else {
			dump_bin_file $db $txn $t1 test008.check
		}
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		set oid [open $t2.tmp w]
		if { [is_heap $omethod] == 1 } {
			incr count -1
			while { $count >= 0 } {
				puts $oid $rids($count)
				incr count -1
			}
		} else {
			foreach f $file_list {
				puts $oid $f
			}
		}
		close $oid
		filesort $t2.tmp $t2 $fsflags
		fileremove $t2.tmp
		filesort $t1 $t3 $fsflags

		error_check_good Test$reopen:diff($t3,$t2) \
		    [filecmp $t3 $t2] 0

		#
		# Now, reopen the file and run the last test again
		# in reverse direction.
		#
		puts "\tTest$reopen.d: Dump contents backward"
		if { $txnenv == 1 } {
			set t [$env txn]
			error_check_good txn [is_valid_txn $t $env] TRUE
			set txn "-txn $t"
		}
		if { [is_heap $omethod] == 1 } {
			test008_heap_dump_bin_file_direction \
			    $db $txn $t1 backward $nameslist $ridslist
		} else {
			dump_bin_file_direction \
			    $db $txn $t1 test008.check "-last" "-prev"
		}
		if { $txnenv == 1 } {
			error_check_good txn [$t commit] 0
		}

		filesort $t1 $t3 $fsflags

		error_check_good Test$reopen:diff($t3,$t2) \
		    [filecmp $t3 $t2] 0
		error_check_good close:$db [$db close] 0
	}
}

proc test008.check { binfile tmpfile } {
	source ./include.tcl

	error_check_good diff($binfile,$tmpfile) \
	    [filecmp $binfile $tmpfile] 0
}

proc test008_heap_put_file { db txn file } {
	source ./include.tcl

	set fid [open $file r]
	fconfigure $fid -translation binary
	set data [read $fid]
	close $fid

	set ret [catch {eval {$db put} $txn -append {$data}} res]
	error_check_good put_file $ret 0

	return $res
}

proc test008_heap_dump_bin_file_direction { db \
    txn outfile direction fnameslist ridslist } {
	source ./include.tcl

	set d1 $testdir/d1
	set outf [open $outfile w]
	array set fnames $fnameslist
	array set rids $ridslist
	set len [llength $ridslist]

	# Now we will get each key from the DB and dump to outfile
	set c [eval {$db cursor} $txn]
	if { $direction == "forward" } {
		set begin -first
		set cont -next
	} else {
		set begin -last
		set cont -prev
	}

	for {set d [$c get $begin] } \
	    { [llength $d] != 0 } {set d [$c get $cont] } {
		set k [lindex [lindex $d 0] 0]
		set data [lindex [lindex $d 0] 1]
		set ofid [open $d1 w]
		fconfigure $ofid -translation binary
		puts -nonewline $ofid $data
		close $ofid

		# Look for the corresponding file name based on the rid.
		for { set i 0 } { $i < $len} { incr i } {
			if { $k == $rids($i) } {
				break
			}
		}
		error_check_good file_rid [expr $i < $len] 1
		test008.check $fnames($i) $d1
		puts $outf $k
	}
	close $outf
	error_check_good curs_close [$c close] 0
	fileremove -f $d1
}
