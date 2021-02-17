# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST    env021
# TEST    Test the operations on a transaction in a CDS environment.
# TEST    These are the operations we test:
# TEST        $txn abort
# TEST        $txn commit
# TEST        $txn id
# TEST        $txn prepare
# TEST        $txn setname name
# TEST        $txn getname
# TEST        $txn discard
# TEST        $txn set_timeout
# TEST    In these operations, we only support the following:
# TEST        $txn id
# TEST        $txn commit

proc env021 { } {
	source ./include.tcl
	env_cleanup $testdir

	puts "Env021: Test operations on a transaction in a CDS environment."

	set indx 0
	puts "\tEnv021.$indx: Test DB_ENV->cdsgroup_begin in\
	    an environment without DB_INIT_CDB configured"
	file mkdir $testdir/envdir$indx
	set env1 [berkdb_env_noerr -create -home $testdir/envdir$indx \
	    -lock -log -txn -mode 0644]
	error_check_good is_valid_env [is_valid_env $env1] TRUE
	set txn1 [catch {eval $env1 cdsgroup} res]
	error_check_good env_without_cdb [expr \
	    [is_substr $res "requires"] && [is_substr $res "DB_INIT_CDB"]] 1
	error_check_good env_close [$env1 close] 0

	set env021parms {
		{"abort" "abort" 0}
		{"commit" "commit" 1}
		{"id" "id" 1}
		{"prepare" "prepare env021gid1" 0}
		{"setname" "setname env021txn1" 0}
		{"getname" "getname" 0}
		{"discard" "discard" 0}
		{"set_timeout" "set_timeout 1000" 0}
	}

	foreach param $env021parms {
		set testname [lindex $param 0]
		set cmd [lindex $param 1]
		set success [lindex $param 2]
		incr indx
		puts "\tEnv021.$indx: Test DB_TXN->$testname"
		file mkdir $testdir/envdir$indx
		set env1 [berkdb_env_noerr -create -home $testdir/envdir$indx \
		    -cdb -mode 0644]
		error_check_good is_valid_env [is_valid_env $env1] TRUE
		set txn1 [$env1 cdsgroup]
		error_check_good is_valid_txn [is_valid_txn $txn1 $env1] TRUE
		set ret [catch {eval $txn1 $cmd} res]
		if {$success} {
			error_check_bad "$txn1 $cmd" \
			    [is_substr $res "CDS groups do not support"] 1
		} else {
			error_check_good "$txn1 $cmd" \
			    [is_substr $res "CDS groups do not support"] 1
		}
		set ret [catch {$txn1 commit} res]
		if {$ret} {
			error_check_good \
			    txn_commit [is_substr $res "invalid command name"] 1
		}
		error_check_good env_close [$env1 close] 0
	}
}

