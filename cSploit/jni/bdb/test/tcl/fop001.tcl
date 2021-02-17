# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	fop001.tcl
# TEST	Test two file system operations combined in one transaction. 
proc fop001 { method { inmem 0 } { childtxn 0 } args } {
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set skipblob 0

	# The variable inmem determines whether the test is being
	# run with regular named databases or named in-memory databases.
	set txntype "transaction"
	if { $inmem == 0 } {
		if { $childtxn == 0 } {
			set tnum "001"
		} else {
			set tnum "009"
			set txntype "child transaction"
			puts "Fop001 with child txns is called fop009."
		}
		set string "regular named databases"
		set operator do_op
	} else {
		set skipblob 1
		set skipmsg "Skipping fop007 (fop001 + in-mem) for blobs."
		if {[is_queueext $method] } {
			puts "Skipping in-memory test for method $method."
			return
		}
		if { $childtxn == 0 } {
			set tnum "007"
			puts "Fop001 with in-memory dbs is called fop007."
		} else {
			set tnum "011"
			set txntype "child transaction"
			puts "Fop001 with in-memory dbs\
			    and child txns is called fop011."
		}
		set string "in-memory named databases"
		set operator do_inmem_op
	}

	# Look for incompatible configurations of blob.
	foreach conf { "-encryptaes" "-encrypt" "-compress" "-dup" "-dupsort" \
	    "-read_uncommitted" "-multiversion" } {
		if { [lsearch -exact $args $conf] != -1 } {
			set skipblob 1
			set skipmsg "Fop001 skipping $conf for blob"
			break
		}
	}
	if { [is_btree $omethod] != 1 && \
	    [is_hash $omethod] != 1 && [is_heap $omethod] != 1 } {
		set skipblob 1
		set skipmsg "Fop001 skipping $omethod for blob"
	}

	puts "\nFop$tnum: ($method)\
	    Two file system ops in one $txntype for $string."

	set exists {a b}
	set noexist {foo bar}
	set open {}
	set cases {}
	set ops {rename remove open open_create open_excl truncate}

	# Set up all sensible two-op cases (op1 succeeds).
	foreach retval { 0 "file exists" "no such file" } {
		foreach op1 {rename remove open open_excl \
		    open_create truncate} {
			foreach op2 $ops {
				append cases " " [create_tests $op1 $op2 \
				    $exists $noexist $open $retval]
			}
		}
	}

	# Set up evil two-op cases (op1 fails).  Omit open_create
	# and truncate from op1 list -- open_create always succeeds
	# and truncate requires a successful open.
	foreach retval { 0 "file exists" "no such file" } {
		foreach op1 { rename remove open open_excl } {
			foreach op2 $ops {
				append cases " " [create_badtests $op1 $op2 \
					$exists $noexist $open $retval]
			}
		}
	}

	# The structure of each case is:
	# {{op1 {names1} result end1} {op2 {names2} result} {remaining}}
	# A result of "0" indicates no error is expected.
	# Otherwise, the result is the expected error message.
	#
	# The "end1" variable indicates whether the first txn
	# ended with an abort or a commit, and is not used
	# in this test.
	#
	# The "remaining" variable lists the files that should 
	# exist at the end of the test case.
	#
	# Comment this loop out to remove the list of cases.
#	set i 1
#	foreach case $cases {
#		puts "\tFop$tnum:$i: $case"
#		incr i
#	}

	set testid 0

	foreach format { "normal" "blob" } {
		if { $format == "blob" } {
			append args " -blob_threshold 1 "
			if { $skipblob != 0 } {
				puts $skipmsg
				continue
			}
		}
		# Run all the cases
		foreach case $cases {
			env_cleanup $testdir
			incr testid

			# Extract elements of the case
			set op1 [lindex [lindex $case 0] 0]
			set names1 [lindex [lindex $case 0] 1]
			set res1 [lindex [lindex $case 0] 2]

			set op2 [lindex [lindex $case 1] 0]
			set names2 [lindex [lindex $case 1] 1]
			set res2 [lindex [lindex $case 1] 2]
			set remaining [lindex [lindex $case 1] 3]

			# Use the list of remaining files to derive
			# the list of files that should be gone.
			set allnames { a b foo bar }
			set gone {}
			foreach f $allnames {
				set idx [lsearch -exact $remaining $f]
				if { $idx == -1 } {
					lappend gone $f
				}
			}

			puts -nonewline "\tFop$tnum.$testid: $op1 ($names1), "
			puts -nonewline "then $op2 ($names2)"
			if { $format == "blob" } {
				puts ", with blobs enabled."
			} else {
				puts "."
			}

			# The variable 'when' describes when to resolve a txn
			# -- before or after closing any open databases. 
			foreach when { before after } {

				# Create transactional environment.
				set env [berkdb_env -create \
				    -home $testdir -txn nosync]
				error_check_good is_valid_env \
				    [is_valid_env $env] TRUE
	
				# Create two databases, dba and dbb.
				if { $inmem == 0 } {
					set dba [eval \
					    {berkdb_open -create} $omethod \
					    $args -env $env -auto_commit a]
				} else {
					set dba [eval {berkdb_open -create} \
					    $omethod $args -env $env \
					    -auto_commit { "" a }]
				}
				error_check_good \
				    dba_open [is_valid_db $dba] TRUE
				error_check_good dba_put [$dba put 1 a] 0
				set blobsubdira [$dba get_blob_sub_dir]
				error_check_good dba_close [$dba close] 0
	
				if { $inmem == 0 } {
					set dbb [eval \
					    {berkdb_open -create} $omethod \
					    $args -env $env -auto_commit b]
				} else {
					set dbb [eval {berkdb_open -create} \
					    $omethod $args -env $env \
					    -auto_commit { "" b }]
				}
				error_check_good \
				    dbb_open [is_valid_db $dbb] TRUE
				error_check_good dbb_put [$dbb put 1 b] 0
				set blobsubdirb [$dbb get_blob_sub_dir]
				error_check_good dbb_close [$dbb close] 0
	
				# The variable 'end' describes how to resolve
				# the txn.  We run the 'abort' first because
				# that leaves the env properly set up for the
				# 'commit' test.
				foreach end {abort commit} {
	
					# Start transaction
					set parent [$env txn]
					set parent_end "commit"
					set msg ""
					if { $childtxn } {
						set child \
						    [$env txn -parent $parent]
						set txn $child
						set msg "(committing parent)"
						if { [berkdb random_int 0 1] \
						    == 0 } {
							set parent_end "abort"
							set msg \
							    "(aborting parent)"
						}
					} else {
						set txn $parent
					}

					puts "\t\tFop$tnum.$testid:\
					    $end $when closing database. $msg"
	
					# Execute and check operation 1
					set result1 [$operator $omethod \
					    $op1 $names1 $txn $env $args]
					if { $res1 == 0 } {
						error_check_good \
						    op1_should_succeed \
						    $result1 $res1
					} else {
						set error \
						    [extract_error $result1]
						error_check_good \
						    op1_wrong_failure \
						    $error $res1
					}
	
					# Execute and check operation 2
					set result2 [$operator $omethod \
					    $op2 $names2 $txn $env $args]
					if { $res2 == 0 } {
						error_check_good \
						    op2_should_succeed \
						    $result2 $res2
					} else {
						set error \
						    [extract_error $result2]
						error_check_good \
						    op2_wrong_failure \
						    $error $res2
					}
	
					if { $when == "before" } {
						error_check_good \
						    txn_$end [$txn $end] 0
						if { $childtxn } {
							error_check_good \
							    parent_end \
							    [$parent \
							    $parent_end] 0
						}
		
						# If the txn was aborted, we
						# still have the original two
						# databases.  Otherwise check
						# for the expected remaining
						# files.
						if { $end == "abort" ||\
						    $parent_end == "abort" } {
							error_check_good \
							    db_exists \
							    [database_exists \
							    $inmem $testdir \
							    a] 1
							if { $format == \
							    "blob" } {
								error_check_good \
								    bloba_exists \
								    [blob_exists \
								    $testdir \
								    $blobsubdira] 1
							}
							error_check_good \
							    db_exists \
							    [database_exists \
							    $inmem $testdir \
							    b] 1
							if { $format == "blob" } {
								error_check_good \
								    blobb_exists \
								    [blob_exists \
								    $testdir \
								    $blobsubdirb] 1
							}
						} else {
							foreach db $remaining {
								error_check_good \
								    db_exists \
								    [database_exists \
								    $inmem \
								    $testdir \
								    $db] 1
							}
							foreach db $gone {
								error_check_good \
								    db_gone \
								    [database_exists \
								    $inmem \
								    $testdir \
								    $db] 0
							}
						}

						close_db_handles 
					} else {
						close_db_handles
						error_check_good \
						    txn_$end [$txn $end] 0
						if { $childtxn } {
							error_check_good \
							    resolve_parent \
							    [$parent \
							    $parent_end] 0
						}
	
						if { $end == "abort" || \
						    $parent_end == "abort" } {
							error_check_good \
							    db_exists \
							    [database_exists \
							    $inmem $testdir \
							    a] 1
							if { $format == \
							    "blob" } {
								error_check_good \
								    bloba_exists \
								    [blob_exists \
								    $testdir \
								    $blobsubdira] 1
							}
							error_check_good \
							    db_exists \
							    [database_exists \
							    $inmem $testdir \
							    b] 1
							if { $format == \
							    "blob" } {
								error_check_good \
								    blobb_exists \
								    [blob_exists \
								    $testdir \
								    $blobsubdirb] 1
							}
						} else {
							foreach db $remaining {
								error_check_good \
								    db_exists \
								    [database_exists \
								    $inmem \
								    $testdir \
								    $db] 1
							}
							foreach db $gone {
								error_check_good \
								    db_gone \
								    [database_exists \
								    $inmem \
								    $testdir \
								    $db] 0
							}

						}
					}		
				}
	
				# Clean up for next case
				error_check_good env_close [$env close] 0
				error_check_good envremove \
				    [berkdb envremove -home $testdir] 0
				env_cleanup $testdir
			}
		}
	}
}

proc database_exists { inmem testdir name } {
	if { $inmem == 1 } {
		return [inmem_exists $testdir $name]
	} else {
		return [file exists $testdir/$name]
	}	

}

proc blob_exists { testdir blobsubdir } {
	set blob_file $testdir/__db_bl/$blobsubdir/__db.bl001
	return [file exists $blob_file]
}

# This is a real hack.  We need to figure out if an in-memory named
# file exists.  In a perfect world we could use mpool stat.  Unfortunately,
# mpool_stat returns files that have deadfile set and we need to not consider
# those files to be meaningful.  So, we are parsing the output of db_stat -MA
# (I told you this was a hack)  If we ever change the output, this is going
# to break big time.  Here is what we assume:
# A file is represented by: File #N name
# The last field printed for a file is Flags
# If the file is dead, deadfile will show up in the flags
proc inmem_exists { dir filename } {
	source ./include.tcl
	set infile 0
	set islive 0
	set name ""
	set s [exec $util_path/db_stat -MA -h $dir]
	foreach i $s {
		if { $i == "File" } {
			set infile 1
			set islive 1
			set name ""
		} elseif { $i == "Flags" } {
			set infile 0
			if { $name != "" && $islive } {
				return 1
			}
		} elseif { $infile != 0 } {
			incr infile
		}

		if { $islive } {
			if { $i == "deadfile," || $i == "deadfile" } {
				set islive 0
			}
		}

		if { $infile == 3 } {
			if { $i == $filename } {
				set name $filename
			}
		}
	}
	return 0
}

