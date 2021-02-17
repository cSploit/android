# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	sdb003
# TEST	Tests many subdbs
# TEST		Creates many subdbs and puts a small amount of
# TEST		data in each (many defaults to 1000)
# TEST
# TEST	Use the first 1000 entries from the dictionary as subdbnames.
# TEST	Insert each with entry as name of subdatabase and a partial list
# TEST	as key/data.  After all are entered, retrieve all; compare output
# TEST	to original.  Close file, reopen, do retrieve and re-verify.
# TEST	Run the test with blob enabled and disabled.
proc sdb003 { method {nentries 1000} args } {
	source ./include.tcl
	global has_crypto

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	if { [is_queue $method] == 1 || [is_heap $method] == 1 } {
		puts "Subdb003: skipping for method $method"
		return
	}

	set txnenv 0
	set eindex [lsearch -exact $args "-env"]
	#
	# If we are using an env, then testfile should just be the db name.
	# Otherwise it is the test directory and the name.
	if { $eindex == -1 } {
		set testfile $testdir/subdb003.db
		set env NULL
	} else {
		set testfile subdb003.db
		incr eindex
		set env [lindex $args $eindex]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
			if { $nentries == 1000 } {
				set nentries 100
			}
		}
		set testdir [get_home $env]
	}

	set t1 $testdir/t1
	set t2 $testdir/t2
	set t3 $testdir/t3

	#
	# Set blob threshold as 5 since most words in the wordlist to put into
	# the database have length <= 10.
	#
	set threshold 5
	set orig_args $args
	foreach blob [list "" " -blob_threshold $threshold"] {
		set args $orig_args
		set msg ""
		if { $blob != "" } {
			set msg "with blob"
		}
		puts "Subdb003: $method ($args) many subdb tests ($msg)"

		if { $blob != "" } {
			# Blob is supported by btree, hash and heap.
			if { [is_btree $method] != 1 && \
			    [is_hash $method] != 1 } {
				puts "Subdb003 skipping\
				    for method $method for blob"
				return
			}
			# Look for incompatible configurations of blob.
			foreach conf { "-encryptaes" "-encrypt" "-compress" \
			    "-dup" "-dupsort" "-read_uncommitted" \
			    "-multiversion" } {
				if { [lsearch -exact $args $conf] != -1 } {
					puts "Subdb003 skipping $conf for blob"
					return
				}
			}
			if { $env != "NULL" } {
				if { [lsearch \
				    [$env get_flags] "-snapshot"] != -1 } {
					puts "Subdb003\
					    skipping -snapshot for blob"
					return
				}
				if { [is_repenv $env] == 1 } {
					puts "Subdb003 skipping\
					    replication env for blob"
					return
				}
				if { $has_crypto == 1 } {
					if { [$env get_encrypt_flags] != "" } {
						puts "Subdb003 skipping\
						    encrypted env for blob"
						return
					}
				}
			}
			if { [lsearch -exact $args "-chksum"] != -1 } {
				set indx [lsearch -exact $args "-chksum"]
				set args [lreplace $args $indx $indx]
				puts "Subdb003 ignoring -chksum for blob"
			}

			# Set up the blob arguments.
			append args $blob
			if { $env == "NULL" } {
				append args " -blob_dir $testdir/__db_bl"
			}
		}

		# Create the database and open the dictionary
		cleanup $testdir $env

		set pflags ""
		set gflags ""
		set txn ""
		set fcount 0

		if { [is_record_based $method] == 1 } {
			set checkfunc subdb003_recno.check
			append gflags " -recno"
		} else {
			set checkfunc subdb003.check
		}

		# Here is the loop where we put and get each key/data pair
		set ndataent 10
		set fdid [open $dict]
		while { [gets $fdid str] != -1 && $fcount < $nentries } {

			set subdb $str
			set db [eval {berkdb_open -create -mode 0644} \
			    $args {$omethod $testfile $subdb}]
			error_check_good dbopen [is_valid_db $db] TRUE

			set count 0
			set did [open $dict]
			while { [gets $did str] != -1 && $count < $ndataent } {
				if { [is_record_based $method] == 1 } {
					global kvals

					set key [expr $count + 1]
					set kvals($key) [pad_data $method $str]
				} else {
					set key $str
				}
				if { $txnenv == 1 } {
					set t [$env txn]
					error_check_good txn \
					    [is_valid_txn $t $env] TRUE
					set txn "-txn $t"
				}
				set ret [eval {$db put} $txn $pflags \
				    {$key [chop_data $method $str]}]
				error_check_good put $ret 0
				if { $txnenv == 1 } {
					error_check_good txn [$t commit] 0
				}

				set ret [eval {$db get} $gflags {$key}]
				error_check_good get $ret [list [list $key \
				    [pad_data $method $str]]]
				incr count
			}
			close $did
			incr fcount

			if { $txnenv == 1 } {
				set t [$env txn]
				error_check_good txn \
				    [is_valid_txn $t $env] TRUE
				set txn "-txn $t"
			}
			dump_file $db $txn $t1 $checkfunc
			if { $txnenv == 1 } {
				error_check_good txn [$t commit] 0
			}
			error_check_good db_close [$db close] 0

			# Now compare the keys to see if they match
			if { [is_record_based $method] == 1 } {
				set oid [open $t2 w]
				for {set i 1} \
				    {$i <= $ndataent} {set i [incr i]} {
					puts $oid $i
				}
				close $oid
				file rename -force $t1 $t3
			} else {
				set q q
				filehead $ndataent $dict $t3
				filesort $t3 $t2
				filesort $t1 $t3
			}

			error_check_good Subdb003:diff($t3,$t2) \
			    [filecmp $t3 $t2] 0

			# Now, reopen the file and run the last test again.
			open_and_dump_subfile $testfile $env $t1 $checkfunc \
			    dump_file_direction "-first" "-next" $subdb
			if { [is_record_based $method] != 1 } {
				filesort $t1 $t3
			}

			error_check_good Subdb003:diff($t2,$t3) \
			    [filecmp $t2 $t3] 0

			# Now, reopen the file and run the last test again in
			# the reverse direction.
			open_and_dump_subfile $testfile $env $t1 $checkfunc \
			    dump_file_direction "-last" "-prev" $subdb

			if { [is_record_based $method] != 1 } {
				filesort $t1 $t3
			}

			error_check_good Subdb003:diff($t3,$t2) \
			    [filecmp $t3 $t2] 0
			if { [expr $fcount % 100] == 0 } {
				puts -nonewline "$fcount "
				flush stdout
			}
		}
		close $fdid
		puts ""
	}
}

# Check function for Subdb003; keys and data are identical
proc subdb003.check { key data } {
	error_check_good "key/data mismatch" $data $key
}

proc subdb003_recno.check { key data } {
	global dict
	global kvals

	error_check_good key"$key"_exists [info exists kvals($key)] 1
	error_check_good "key/data mismatch, key $key" $data $kvals($key)
}
