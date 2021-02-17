# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	test137
# TEST  Test Automatic Resource Management. [#16188][#20281]
# TEST  Open an environment, and open a database in it.
# TEST  Do some operations in the database, including:
# TEST    insert, dump, delete
# TEST  Close the environment without closing the database.
# TEST  Re-open the database and verify the records are the ones we expected.

proc test137 { method {nentries 1000} {start 0} {skip 0} {use_encrypt 0}
    {tnum "137"} {envtype "ds"} args } {
	source ./include.tcl
	global encrypt
	global has_crypto

	# This test needs its own env.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test$tnum skipping for env $env"
		return
	}

	# Skip if use_encrypt and encryption is not supported.
	if { $use_encrypt && $has_crypto == 0 } {
		puts "Skipping test$tnum for non-crypto release."
		return
	}

	# Do not use db encryption flags from the $args -- this test
	# does its own encryption testing at the env level. 
	convert_encrypt $args
	if { $encrypt } {
		puts "Test$tnum skipping DB encryption"
		return
	}

	# We can not specify chksum option to opening db when using
	# an encrypted env.
	set chksum_indx [lsearch -exact $args "-chksum"]
	if { $chksum_indx != -1 && $use_encrypt } {
		puts "Test$tnum skipping DB chksum"
		return
	}

	set encmsg "non-encrypt"
	if { $use_encrypt } {
		set encmsg "encrypt"
	}

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	env_cleanup $testdir
	set env [test137_openenv $envtype $testdir $use_encrypt]
	set envargs  "-env $env"
	set txnenv [string equal $envtype "tds"]
	if { $txnenv == 1 } {
		set nentries 200 
		append args " -auto_commit"
	}

	set testfile test$tnum.db	
	set t1 $testdir/t1
	set t2 $testdir/t2

	puts "Test$tnum: $method \
	    ($envtype,$encmsg,$args) $nentries equal key/data pairs"

	set did [open $dict]
	# The "start" variable determines the record number to start
	# with, if we're using record numbers.  The "skip" variable
	# determines the dictionary entry to start with.
	# In normal use, skip will match start.
	puts "\tTest$tnum: Starting at $start with dictionary entry $skip"
	if { $skip != 0 } {
		for { set count 0 } { $count < $skip } { incr count } {
			gets $did str
		}
	}

	set db [eval {berkdb_open \
	     -create -mode 0644} $envargs $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	set txn ""
	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}

	puts "\tTest$tnum.a: put loop"
	# Here is the loop where we put and get each key/data pair
	set count 0
	set key_list {}
	set data_list {}
	while { [gets $did str] != -1 && $count < $nentries } {
		if { [is_record_based $method] == 1 } {
			set key [expr $count + 1 + $start]
		} else {
			set key $str
			set str [reverse $str]
		}
		set ret [eval \
		    {$db put} $txn {$key [chop_data $method $str]}]
		error_check_good db_put $ret 0
		lappend key_list $key
		lappend data_list $str
		incr count
	}
	close $did

	puts "\tTest$tnum.b: dump file and close env without closing database"
	dump_file $db $txn $t1 NONE
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good env_close [$env close] 0
	# Clean the tcl handle only.
        catch {$db close -handle_only} res

	puts "\tTest$tnum.c: open the env and database again"
	set env [test137_openenv $envtype $testdir $use_encrypt]
	set envargs  "-env $env"
	set db [eval {berkdb_open \
	     -create -mode 0644} $envargs $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}

	puts "\tTest$tnum.d: dump file and check"
	dump_file $db $txn $t2 NONE
	error_check_good Test$tnum:diff($t1,$t2) \
	    [filecmp $t1 $t2] 0

	puts "\tTest$tnum.e: delete loop"
	# We delete starting from the last record, since for rrecno, if we 
	# delete previous records, the ones after will change their keys.
	for {set i [expr [llength $key_list] - 1]} {$i >= 0} \
	    {incr i -[berkdb random_int 1 5]} {
		set key [lindex $key_list $i]
		set ret [eval $db del $txn {$key}]
		error_check_good db_del $ret 0
	} 

	puts "\tTest$tnum.f:\
	    dump file and close env without closing database again"
	dump_file $db $txn $t1 NONE
	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good env_close [$env close -forcesync] 0
        catch {$db close -handle_only} res

	# We remove the environment to make sure the regions files
	# are deleted(including mpool files), so we can verify if
	# the database has been synced during a force-synced env close.
	error_check_good env_remove [berkdb envremove -force -home $testdir] 0

	puts "\tTest$tnum.g: open, dump and check again"
	set env [test137_openenv $envtype $testdir $use_encrypt]
	set envargs  "-env $env"
	set db [eval {berkdb_open \
	     -create -mode 0644} $envargs $args $omethod $testfile]
	error_check_good dbopen [is_valid_db $db] TRUE

	if { $txnenv == 1 } {
		set t [$env txn]
		error_check_good txn [is_valid_txn $t $env] TRUE
		set txn "-txn $t"
	}

	dump_file $db $txn $t2 NONE
	error_check_good Test$tnum:diff($t1,$t2) \
	    [filecmp $t1 $t2] 0

	if { $txnenv == 1 } {
		error_check_good txn_commit [$t commit] 0
	}
	error_check_good db_close [$db close] 0
	error_check_good env_close [$env close] 0
}

proc test137_openenv {envtype testdir use_encrypt} {
	global passwd
	
	set encargs ""
	if {$use_encrypt} {
		set encargs " -encryptaes $passwd "
	}
	switch -exact "$envtype" {
		"ds" {
			set env [eval berkdb_env_noerr -create -mode 0644 \
			    $encargs -home $testdir]
		}
		"cds" {
			set env [eval berkdb_env_noerr -create -mode 0644 \
			    $encargs -cdb -home $testdir]
		}
		"tds" {
			set env [eval berkdb_env_noerr -create -mode 0644 \
			    $encargs -txn -lock -log -thread -home $testdir]
		}
	}
	error_check_good env_open [is_valid_env $env] TRUE
	return $env
}
