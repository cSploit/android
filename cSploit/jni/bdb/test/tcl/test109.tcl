# See the file LICENSE for redistribution information.
#
# Copyright (c) 2004, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test109
# TEST
# TEST	Test of full arguments combinations for sequences API.
proc test109 { method {tnum "109"} args } {
	source ./include.tcl
	global rand_init
	global fixed_len
	global errorCode

	set eindex [lsearch -exact $args "-env"]
	set txnenv 0

	if { [is_partitioned $args] == 1 } {
		puts "Test109 skipping for partitioned $method"
		return
	}

	set sargs " -thread "
	if { $eindex == -1 } {
		set env NULL
	} else {
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		set testdir [get_home $env]
	}

	# Fixed_len must be increased from the default to
	# accommodate fixed-record length methods.
	set orig_fixed_len $fixed_len
	set fixed_len 128
	set args [convert_args $method $args]
	#Make a copy of $args without -auto_commit flag for combined args test.
	set cargs $args
	set omethod [convert_method $method]
	error_check_good random_seed [berkdb srand $rand_init] 0

	if { $eindex != -1 && $txnenv == 1 } {
		append args " -auto_commit "
	}

	# Test with in-memory dbs, regular dbs, and subdbs.
	foreach filetype { subdb regular in-memory } {
		puts "Test$tnum: $method ($args) Test of sequences ($filetype)."

		# Skip impossible combinations.
		if { $filetype == "subdb" && [is_heap $method] } {
			puts "\tTest$tnum.a: Skipping $filetype test for method\
			    $method."
			continue
		}

		if { $filetype == "subdb" && [is_queue $method] } {
			puts "Test$tnum: Skipping $filetype test for method\
			    $method."
			continue
		}

		if { $filetype == "in-memory" && [is_queueext $method] } {
			puts "Test$tnum: Skipping $filetype test for method\
			    $method."
			continue
		}

		# Reinitialize file name for each file type, then adjust.
		if { $env == "NULL" } {
			set testfile $testdir/test$tnum.db
		} else {
			set testfile test$tnum.db
		}
		if { $filetype == "subdb" } {
			lappend testfile SUBDB
		}
		if { $filetype == "in-memory" } {
			set testfile ""
		}

		# Test sequences APIs with all possible arguments combinations.
		test_sequence_args_combine $tnum $method $env $txnenv $cargs\
	            $filetype $testfile

		# Skip impossible combinations.
	        if { [is_heap $method] } {
		        puts "Test$tnum: Skipping remain tests for method\
			    $method."
			continue
		}

		if { $env != "NULL" } {
			set testdir [get_home $env]
		}

		cleanup $testdir $env

		# Make the key numeric so we can test record-based methods.
		set key 1

		# Open a noerr db, since we expect errors.
		set db [eval {berkdb_open_noerr \
		    -create -mode 0644} $args $omethod $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		puts "\tTest$tnum.b: Max must be greater than min."
		set errorCode NONE
		catch {set seq [eval {berkdb sequence} -create $sargs \
		    -init 0 -min 100 -max 0 $db $key]} res
		error_check_good max>min [is_substr $errorCode EINVAL] 1

		puts "\tTest$tnum.c: Init can't be out of the min-max range."
		set errorCode NONE
		catch {set seq [eval {berkdb sequence} -create $sargs \
			-init 101 -min 0 -max 100 $db $key]} res
		error_check_good init [is_substr $errorCode EINVAL] 1

		# Test increment and decrement.
		set min 0
		set max 100
		foreach { init inc } { $min -inc $max -dec } {
			puts "\tTest$tnum.d: Test for overflow error with $inc."
			test_sequence $env $db $key $min $max $init $inc
		}

		# Test cachesize without wrap.  Make sure to test both
		# cachesizes that evenly divide the number of items in the
		# sequence, and that leave unused elements at the end.
		set min 0
		set max 99
		set init 1
		set cachesizes [list 2 7 11]
		foreach csize $cachesizes {
			foreach inc { -inc -dec } {
				puts "\tTest$tnum.e:\
				    -cachesize $csize, $inc, no wrap."
				test_sequence $env $db $key \
				    $min $max $init $inc $csize
			}
		}
		error_check_good db_close [$db close] 0

		# Open a regular db; we expect success on the rest of the tests.
		set db [eval {berkdb_open \
		     -create -mode 0644} $args $omethod $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE

		# Test increment and decrement with wrap.  Cross from negative
		# to positive integers.
		set min -50
		set max 99
		set wrap "-wrap"
		set csize 1
		foreach { init inc } { $min -inc $max -dec } {
			puts "\tTest$tnum.f: Test wrapping with $inc."
			test_sequence $env $db $key \
			    $min $max $init $inc $csize $wrap
		}

		# Test cachesize with wrap.
		set min 0
		set max 99
		set init 0
		set wrap "-wrap"
		foreach csize $cachesizes {
			puts "\tTest$tnum.g: Test -cachesize $csize with wrap."
			test_sequence $env $db $key \
			    $min $max $init $inc $csize $wrap
		}

		# Test multiple handles on the same sequence.
		foreach csize $cachesizes {
			puts "\tTest$tnum.h:\
			    Test multiple handles (-cachesize $csize) with\
			    wrap."
			test_sequence $env $db $key \
			    $min $max $init $inc $csize $wrap 1
		}
		error_check_good db_close [$db close] 0
	}
	set fixed_len $orig_fixed_len
	return
}

proc test_sequence { env db key min max init \
    {inc "-inc"} {csize 1} {wrap "" } {second_handle 0} } {
	global rand_init
	global errorCode

	set txn ""
	set txnenv 0
	if { $env != "NULL" } {
		set txnenv [is_txnenv $env]
	}

	set sargs " -thread "

	# The variable "skip" is the cachesize with a direction.
	set skip $csize
	if { $inc == "-dec" } {
		set skip [expr $csize * -1]
	}

	# The "limit" is the closest number to the end of the
	# sequence we can ever see.
	set limit [expr [expr $max + 1] - $csize]
	if { $inc == "-dec" } {
		set limit [expr [expr $min - 1] + $csize]
	}

	# The number of items in the sequence.
	set n [expr [expr $max - $min] + 1]

	# Calculate the number of values returned in the first
	# cycle, and in all other cycles.
	if { $inc == "-inc" } {
		set firstcyclehits \
		    [expr [expr [expr $max - $init] + 1] / $csize]
	} elseif { $inc == "-dec" } {
		set firstcyclehits \
		    [expr [expr [expr $init - $min] + 1] / $csize]
	} else {
		puts "FAIL: unknown inc flag $inc"
	}
	set hitspercycle [expr $n / $csize]

	# Create the sequence.
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	set seq [eval {berkdb sequence} -create $sargs -cachesize $csize \
	    $wrap -init $init -min $min -max $max $txn $inc $db $key]
	error_check_good is_valid_seq [is_valid_seq $seq] TRUE
	if { $second_handle == 1 } {
		set seq2 [eval {berkdb sequence} -create $sargs $txn $db $key]
		error_check_good is_valid_seq2 [is_valid_seq $seq2] TRUE
	}
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}

	# Exercise get options.
	set getdb [$seq get_db]
	error_check_good seq_get_db $getdb $db

	set flags [$seq get_flags]
	set exp_flags [list $inc $wrap]
	foreach item $exp_flags {
		if { [llength $item] == 0 } {
			set idx [lsearch -exact $exp_flags $item]
			set exp_flags [lreplace $exp_flags $idx $idx]
		}
	}
	error_check_good get_flags $flags $exp_flags

	set range [$seq get_range]
	error_check_good get_range_min [lindex $range 0] $min
	error_check_good get_range_max [lindex $range 1] $max

	set cache [$seq get_cachesize]
	error_check_good get_cachesize $cache $csize

	# Within the loop, for each successive seq get we calculate
	# the value we expect to receive, then do the seq get and
	# compare.
	#
	# Always test some multiple of the number of items in the
	# sequence; this tests overflow and wrap-around.
	#
	set mult 2
	for { set i 0 } { $i < [expr $n * $mult] } { incr i } {
		#
		# Calculate expected return value.
		#
		# On the first cycle, start from init.
		set expected [expr $init + [expr $i * $skip]]
		if { $i >= $firstcyclehits && $wrap != "-wrap" } {
			set expected "overflow"
		}

		# On second and later cycles, start from min or max.
		# We do a second cycle only if wrapping is specified.
		if { $wrap == "-wrap" } {
			if { $inc == "-inc" && $expected > $limit } {
				set j [expr $i - $firstcyclehits]
				while { $j >= $hitspercycle } {
					set j [expr $j - $hitspercycle]
				}
				set expected [expr $min + [expr $j * $skip]]
			}

			if { $inc == "-dec" && $expected < $limit } {
				set j [expr $i - $firstcyclehits]
				while { $j >= $hitspercycle } {
					set j [expr $j - $hitspercycle]
				}
				set expected [expr $max + [expr $j * $skip]]
			}
		}

		# Get return value.  If we've got a second handle, choose
		# randomly which handle does the seq get.
		if { $env != "NULL" && [is_txnenv $env] } {
			set syncarg " -nosync "
		} else {
			set syncarg ""
		}
		set errorCode NONE
		if { $second_handle == 0 } {
			catch {eval {$seq get} $syncarg $csize} res
		} elseif { [berkdb random_int 0 1] == 0 } {
			catch {eval {$seq get} $syncarg $csize} res
		} else {
			catch {eval {$seq2 get} $syncarg $csize} res
		}

		# Compare expected to actual value.
		if { $expected == "overflow" } {
			error_check_good overflow\
			    [is_substr $errorCode EINVAL] 1
		} else {
			error_check_good seq_get_wrap $res $expected
		}
	}

	# A single handle requires a 'seq remove', but a second handle
	# should be closed, and then we can remove the sequence.
	if { $second_handle == 1 } {
		error_check_good seq2_close [$seq2 close] 0
	}
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}
	error_check_good seq_remove [eval {$seq remove} $txn] 0
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
}

proc test_sequence_args_combine { tnum method env txnenv sargs filetype\
    testfile } {
	source ./include.tcl

	set omethod [convert_method $method]

	if { $env != "NULL" } {
		set testdir [get_home $env]
	}

	cleanup $testdir $env

	# To generate a transaction handle that with no relation with current
        # environment, we setup another environment.
	set err_env_home "$testdir/err_env"
	file mkdir $err_env_home
	set err_env [eval {berkdb_env_noerr -create -txn} -home $err_env_home]
	set err_txn_id [$err_env txn]
	error_check_good err_txn:txn_begin [is_valid_txn $err_txn_id $err_env]\
	    TRUE

	# Combine all possible arguments to emulate most cases.
	set txnflags	[list	""		""\
				" -auto_commit "	""\
				""		" -txn \$txn_id "]
	if { $txnenv != 0 } {
		set txnflags [list " -auto_commit " ""]
	}

	foreach dupflag { "" " -dup " } {
		if { [is_substr $sargs "-compress"] && $dupflag == " -dup " } {
			set dupflag " -dup -dupsort "
		}
		foreach rdflag { "" " -rdonly " } {
			# Skip testing in read-only and in-memory mode.	
			if { $rdflag != "" && $filetype == "in-memory" } {
				continue
			}

			# Skip dup flags for non-support DB types.
			if { ![is_btree $method] && ![is_hash $method] } {
				if { $dupflag != "" } {
					continue
				}
			}

			# Test in non-transaction mode.
			if { $env == "NULL" || $txnenv == 0 } {
				test_with_db_args $err_txn_id $sargs $env\
				    $omethod $testfile $rdflag $dupflag
				continue
			}

			# Test in transaction mode.
			foreach {acflag txnid_flag}\
			    $txnflags {
				test_with_db_args $err_txn_id $sargs $env\
				    $omethod $testfile $rdflag $dupflag $acflag\
				    $txnid_flag
			}
		}
	}

	# Close the individual environment.
	if { $err_txn_id != "" } {
		error_check_good err_txn_commit [$err_txn_id commit] 0
		error_check_good err_env_close [$err_env close] 0
	}
}

proc test_with_db_args { err_txn_id sargs env omethod testfile rdflag\
	dupflag {acflag ""} {usetxn ""} } {
	source ./include.tcl
	puts "\tTest109.a Test with dbargs: $sargs $omethod $rdflag\
	    $dupflag $acflag $usetxn"

	set db_open_args " $sargs $omethod $dupflag $acflag $usetxn "
	set db_put_args " $usetxn "
	set txn_id ""
	set err_db ""

	# Prepare txn_id.
	if { $acflag != "" || $usetxn !="" } {
		set txn_id [$env txn]
		error_check_good txn_check [is_valid_txn $txn_id $env] TRUE
	}

	# Create db filled with data.
	set db [eval {berkdb_open_noerr -create -mode 0644} $db_open_args\
	    $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE
	
	# Put some non-sequence data for later test.
	set err_key 1
	set err_key_data 99
	set ret [eval {$db put} $db_put_args $err_key $err_key_data]
	error_check_good put:$db $ret 0

	# Get into read-only mode if needed, skip this step for in-memory mode.
	if { $testfile != "" && $rdflag != "" } {
		# Close and reopen db in read-only mode if needed.
		error_check_good db_close [$db close] 0
		set db [eval {berkdb_open_noerr} $db_open_args $rdflag\
		    $testfile]
		error_check_good dbopen [is_valid_db $db] TRUE
	}

	# Combine every possible argument.
	set seq_params [list   1   10 100 " -inc "\
			       50  10 100 " -inc "\
			       100 10 100 " -inc "\
			       1   100 10 " -dec "\
			       50  100 10 " -dec "\
			       100 100 10 " -dec "\
			       1   10 10  " -inc "\
			       1   10 10  " -dec "\
			       1   100 10 " -inc "\
			       1   10 100 " -dec "]
	set seq_cache_size	[list 200 95 10 0 ""]
	set seq_txn_mode	[list ""]
	if { $usetxn != "" && $acflag == "" } {
		# Skip err_txn mode if DB is not opened with a txn.
		set seq_txn_mode	[list " -txn \$err_txn_id "\
	       				      " -txn \$txn_id "]
	} elseif { $usetxn == "" && $acflag != "" } {
		set seq_txn_mode	[list " -txn \$txn_id " ""]
	} else {
		set seq_txn_mode	[list " -txn \$err_txn_id " ""]
	}

	set seq_key 2

	foreach { seq_init seq_min seq_max incflag } $seq_params {
		foreach seq_cache $seq_cache_size {
			# Create sequence tests.
			set seq [test_create_seq $db $incflag $seq_min\
			    $seq_max $seq_init $seq_cache $txn_id $err_key\
			    $seq_key $omethod $rdflag $dupflag $acflag]
			# If test_create_seq did not return a sequence handle
			# because it was given invalid arguments, go on to the
			# next case. 
			if { $seq == "" } {
				continue
			}

			# Get and remove sequence test.
			foreach seq_corrupt_data { 0 1 } {
				foreach op_txnflag $seq_txn_mode {
					foreach nosyncflag { " -nosync " "" } {
						if { $seq_corrupt_data == 1 } {
							# Overwrite seq_key.
							eval {$db put}\
							    $db_put_args\
							    $seq_key\
							    $err_key_data
						}
						set ret [test_operate_seq $seq\
						    $incflag $seq_min $seq_max\
						    $seq_init $seq_cache\
						    $txn_id $err_txn_id\
						    $op_txnflag $nosyncflag\
						    $seq_corrupt_data $acflag]

						if { $acflag != "" &&\
						    ![is_substr\
						    $op_txnflag "err"] } {
							# Commit transaction
							# that not equal to
							# the auto-commit one.
							$txn_id commit
							set txn_id [$env txn]
							error_check_good\
							    txn_check\
							    [is_valid_txn\
							    $txn_id $env]\
							    TRUE
						}
						if { $seq_corrupt_data == 1 } {
							# Delete corrupted data.
							if { $acflag == "" } {
								eval {$db del}\
								$db_put_args\
								$seq_key
						    	} else {
								eval {$db del}\
								$seq_key
							}
						}
						# Re-create sequence.
						set seq [test_create_seq $db\
						    $incflag $seq_min\
						    $seq_max $seq_init\
						    $seq_cache $txn_id $err_key\
						    $seq_key $omethod $rdflag\
						    $dupflag $acflag]
					}
				}
			}
			# Close unused seq handle.
			$seq remove
		}
	}

	# Commit txn first in auto commit mode.
	if { $acflag != "" } {
		if { $txn_id != "" } {
			error_check_good txn_commit [$txn_id commit] 0
		}
	}
	
	# Close and remove db.
	error_check_good db_close [$db close] 0
	if { $env != "NULL" && $testfile != ""} {
		error_check_good remove [eval {$env dbremove} $acflag\
		    $usetxn $testfile] 0
	} elseif { $env == "NULL" } {
		set ret [catch { glob $testdir/*.db* } result]
		if { $ret == 0 } {
			foreach fileorig $result {
				file delete $fileorig
			}
		}
	}

	# Commit txn after close db with txn except for auto commit mode.
	if { $acflag == "" } {
		if { $txn_id != "" } {
			error_check_good txn_commit [$txn_id commit] 0
		}
	}
}

proc test_operate_seq {seq incflag seq_min seq_max seq_init seq_cache\
	txn_id err_txn_id txnflag nosyncflag seq_corrupt_data acflag} {
	
	# Prepare possible combinations.
	set seq_size [expr $seq_max - $seq_min]
	if { $incflag == " -dec " } {
		set seq_size [expr $seq_size * -1]
	}
	set seq_get_delta	[list 0 "" 2 [expr $seq_size - 1]\
	    [expr $seq_size + 1]]

	# Test get command.
	set expect_get_ret $seq_init
	foreach deltaflag $seq_get_delta {
		if { $deltaflag != "" && $incflag == " -dec " } {
			set expect_get_ret [expr $expect_get_ret - $deltaflag]
		} elseif { $deltaflag != "" && $incflag == " -inc " } {
			set expect_get_ret [expr $expect_get_ret + $deltaflag]
		}

		set expect_msg ""
		set err_handled 0
		set ret NULL
		catch {set ret [eval {$seq get} $txnflag $deltaflag]} res

		# Check whether error is handled.
		if { $acflag != "" && $txnflag != "" } {
			set expect_msg "sequence get:invalid"
			set err_handled [is_substr $res $expect_msg]
		}
		if { $seq_cache != "" && $seq_cache != 0 && $txnflag != "" &&\
		         $err_handled != 1} {
			set err_expect_msgs [list\
			    "non-zero cache may not specify transaction"\
			    "sequence get:invalid"]
			foreach expect_msg $err_expect_msgs {
				set err_handled [is_substr $res $expect_msg]
				if { $err_handled == 1 } {
					break
				}
			}
		} 
		if { $expect_get_ret > $seq_max && $err_handled != 1} {
			if { $incflag == " -inc " } {
				set err_expect_msgs [list "Sequence overflow"\
				    "sequence get:invalid"]
				foreach expect_msg $err_expect_msgs {
					set err_handled [is_substr $res\
					    $expect_msg]
					if { $err_handled == 1 } {
						break
					}
				}
			}
		}
		if { $expect_get_ret < $seq_max && $err_handled != 1} {
			if { $incflag == " -dec " } {
				set expect_msg "Sequence overflow"
				set err_handled [is_substr $res $expect_msg]
			}
		}
		if { $deltaflag == "" && $err_handled != 1} {
			set err_expect_msgs [list "wrong # args"\
						  "Wrong number of key/data"\
						  "sequence get:invalid"]
			foreach expect_msg $err_expect_msgs {
				set err_handled [is_substr $res $expect_msg]
				if { $err_handled == 1 } {
					break
				}
			}
		}
		if { $deltaflag <= 0 && $err_handled != 1} {
			set err_expect_msgs\
		            [list "delta must be greater than 0"\
		       	    "sequence get:invalid"]
			foreach expect_msg $err_expect_msgs {
				set err_handled [is_substr $res $expect_msg]
				if { $err_handled == 1 } {
					break
				}
			}
		} 
		if { [is_substr $txnflag "err"] && $err_handled != 1} {
			set err_expect_msgs [list\
			    "Transaction specified for a non-transactional\
 			    data base"\
			    "Transaction and database from different\
			    environments"\
			    "Transaction that opened the DB handle is still\
			    active"\
			    "DB environment not configured for transactions"]
			foreach expect_msg $err_expect_msgs {
				set err_handled [is_substr $res $expect_msg]
				if { $err_handled == 1 } {
					break
				}
			}
		}
		if { $seq_corrupt_data == 1 && $err_handled != 1 } {
			set err_expect_msgs [list "Bad sequence record format"\
			    "Sequence overflow" ":invalid argument"]
			foreach expect_msg $err_expect_msgs {
				set err_handled [is_substr $res $expect_msg]
				if { $err_handled == 1 } {
					break
				}
			}
		}

		# Make sure all errors were handled.
		if { $ret == "NULL" && $err_handled != 1} {
			puts "\t\t($seq) get ($txnflag) ($deltaflag),\
			    data corrupted:$seq_corrupt_data, ($incflag)\
			    , acflag:$acflag"
			puts "\t\tTest get:res:<$res>"
			puts "\t\tret:$ret"
			error_check_bad seq_get [is_substr $ret "NULL"] 1
		}
	}

	# Test remove command.
	set expect_msg ""
	set err_handled 0
	set ret NULL
	catch {set ret [eval {$seq remove} $txnflag $nosyncflag]} res

	# Check whether error is handled.
	if { $acflag != "" && $txnflag != "" } {
		set expect_msg "sequence remove:invalid"
		set err_handled [is_substr $res $expect_msg]
	}
	if { $nosyncflag != "" && $err_handled != 1} {
		set expect_msg "DB_SEQUENCE->remove illegal flag"
		set err_handled [is_substr $res $expect_msg]
	}
	if { [is_substr $txnflag "err"] && $err_handled != 1} {
		set err_expect_msgs [list\
		    "Transaction specified for a non-transactional data base"\
		    "Transaction and database from different environments"\
		    "Transaction that opened the DB handle is still active"\
		    "DB environment not configured for transactions"\
		    ":invalid argument"]
		foreach expect_msg $err_expect_msgs {
			set err_handled [is_substr $res $expect_msg]
			if { $err_handled == 1 } {
				break
			}
		}
	}

	# Make sure all errors were handled.
	if { $ret == "NULL" && $err_handled != 1} {
		puts "\t\t($seq) remove ($txnflag) ($nosyncflag),\
		    data corrupted:$seq_corrupt_data, ($incflag)"
		puts "\t\tTest remove:res:<$res>"
		error_check_bad seq_get [is_substr $ret "NULL"] 1
	}

	return $ret
}

proc test_create_seq {db incflag seq_min seq_max seq_init seq_cache\
	txn_id err_key seq_key omethod rdflag dupflag acflag} {

	set seq_args " -create $incflag -min $seq_min -max $seq_max\
	    -init $seq_init "
	set txnflag ""
	if { $txn_id != "" && $acflag == "" } {
		set txnflag " -txn \$txn_id "
	}
	if { $seq_cache != "" } {
		append seq_args " -cachesize $seq_cache "
	}

	# Test 1: create sequence at existed key.
	set expect_msg ""
	set seq ""
	set err_handled 0
	set seq_size [expr $seq_max - $seq_min]
	catch {set seq [eval {berkdb sequence} $seq_args $txnflag $db\
	    $err_key]} res

	# Check whether error is handled.
	if { [is_heap $omethod] } {
		set expect_msg "Heap databases may not be used with sequences"
		set err_handled [is_substr $res $expect_msg]
        }
	if { ([is_btree $omethod] || [is_hash $omethod] ||\
	    [is_recno $omethod] ) && $err_handled != 1} {
		set expect_msg "Bad sequence record format"
		set err_handled [is_substr $res $expect_msg]
        }
	if { $seq_cache > $seq_size && $err_handled != 1} {
		set expect_msg "Number of items to be cached is larger than\
		    the sequence range"
		set err_handled [is_substr $res $expect_msg]
	}
	if { $seq_cache < 0 && $err_handled != 1} {
		set expect_msg "Cache size must be >= 0"
		set err_handled [is_substr $res $expect_msg]
	}
	if { ( $seq_init < $seq_min || $seq_init > $seq_max ) &&\
	    $err_handled != 1} {
		set expect_msg "Sequence value out of range"
		set err_handled [is_substr $res $expect_msg]
	}
	if { $seq_min >= $seq_max && $err_handled != 1} {
		set expect_msg "Minimum sequence value must be less than\
		    maximum sequence value"
		set err_handled [is_substr $res $expect_msg]
	}
	if { $rdflag != "" && $err_handled != 1} {
		set err_expect_msgs\
	            [list "attempt to modify a read-only database"\
		    ":permission denied"]
		foreach expect_msg $err_expect_msgs {
			set err_handled [is_substr $res $expect_msg]
			if { $err_handled == 1 } {
				break
			}
		}
	}
	if { $dupflag != "" && $err_handled != 1} {
		set expect_msg "Sequences not supported in databases configured\
		    for duplicate data"
		set err_handled [is_substr $res $expect_msg]
	}
	if { $err_handled != 1} {
		set err_expect_msgs [list ":invalid argument"]
		foreach expect_msg $err_expect_msgs {
			set err_handled [is_substr $res $expect_msg]
			if { $err_handled == 1 } {
				break
			}
		}
	}

	# Make sure all errors were handled.
	if { [is_valid_seq $seq] != TRUE && $err_handled != 1} {
		puts "\t\tTest create seq 1:$seq_args $txnflag \$db \$err_key"
		puts "\t\tTest create part 1:res:<$res>"
		error_check_good is_valid_seq [is_valid_seq $seq] TRUE
	}

	#sequence might still could be create when db type is frecno or queue.
	if { $seq != "" } {
		$seq remove
	}

	# Test 2: create sequence at new key.
	set expect_msg ""
	set seq ""
	set err_handled 0
	catch {set seq [eval {berkdb sequence} $seq_args $txnflag $db\
	    $seq_key]} res

	# Check whether error is handled.
	if { [is_heap $omethod] } {
		set expect_msg "Heap databases may not be used with sequences."
		set err_handled [is_substr $res $expect_msg]
	}
	if { $rdflag != "" && $err_handled != 1} {
		set err_expect_msgs [list "attempt to modify a read-only\
		    database" ":permission denied"]
		foreach expect_msg $err_expect_msgs {
			set err_handled [is_substr $res $expect_msg]
			if { $err_handled == 1 } {
				break
			}
		}
	}
	if { $dupflag != "" && $err_handled != 1} {
		set expect_msg "Sequences not supported in databases configured\
		    for duplicate data"
		set err_handled [is_substr $res $expect_msg]
	}
	if { $seq_min >= $seq_max && $err_handled != 1} {
		set expect_msg "Minimum sequence value must be less than\
		    maximum sequence value"
		set err_handled [is_substr $res $expect_msg]
	}
	if { ( $seq_init < $seq_min || $seq_init > $seq_max ) &&\
	    $err_handled != 1} {
		set expect_msg "Sequence value out of range"
		set err_handled [is_substr $res $expect_msg]
	}
	if { $seq_cache > $seq_size && $err_handled != 1} {
		set expect_msg "Number of items to be cached is larger than\
		    the sequence range"
		set err_handled [is_substr $res $expect_msg]
	}
	if { $seq_cache < 0 && $err_handled != 1} {
		set expect_msg "Cache size must be >= 0"
		set err_handled [is_substr $res $expect_msg]
        }
	if { $err_handled != 1} {
		set err_expect_msgs [list ":invalid argument"]
		foreach expect_msg $err_expect_msgs {
			set err_handled [is_substr $res $expect_msg]
			if { $err_handled == 1 } {
				break
			}
		}
	}

	# Make sure all errors were handled.
	if { [is_valid_seq $seq] != TRUE && $err_handled != 1} {
		puts "\t\tTest create seq 2:$seq_args $txnflag \$db \$err_key"
		puts "\t\tTest create part 2:res:<$res>"
		error_check_good is_valid_seq [is_valid_seq $seq] TRUE
	}	

	return $seq
}
