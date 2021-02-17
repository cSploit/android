# See the file LICENSE for redistribution information.
#
# Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	memp005
# TEST  Make sure that db pagesize does not interfere with mpool pagesize.
proc memp005 { } {
	source ./include.tcl

	puts "Memp005: Test interaction of database and mpool pagesize."
	env_cleanup $testdir

	# Set the mpool pagesize.
	puts "\tMemp005.a: Set mpool pagesize."
	set mp_pagesize 1024
	set e [eval {berkdb_env -create -pagesize $mp_pagesize -home $testdir} ]
	error_check_good dbenv [is_valid_env $e] TRUE

	# Check the pagesize through mpool_stat and through the getter.
	set mpool_stat_pagesize [stat_field $e mpool_stat "Default pagesize"]
	error_check_good check_mp_pagesize $mp_pagesize $mpool_stat_pagesize
	set get_mp_pagesize [$e get_mp_pagesize]
	error_check_good check_getter_pagesize $get_mp_pagesize $mp_pagesize

	# Set a different database pagesize.  
	puts "\tMemp005.b: Set different database pagesize."
	set db_pagesize 2048
	set db [eval {berkdb_open -create\
	     -pagesize $db_pagesize -env $e -btree foo.db} ]

	# Make sure the mpool pagesize and database pagesizes are correct. 
	# Check both the stats and the getters.
	puts "\tMemp005.c: Check values."
	set mpool_stat_pagesize [stat_field $e mpool_stat "Default pagesize"]
	error_check_good check_mp_pagesize $mp_pagesize $mpool_stat_pagesize
	set get_mp_pagesize [$e get_mp_pagesize]
	error_check_good check_mpgetter_pagesize $get_mp_pagesize $mp_pagesize
	set db_stat_pagesize [stat_field $db stat "Page size"]
	error_check_good check_db_pagesize $db_pagesize $db_stat_pagesize
	set db_get_pagesize [$db get_pagesize]
	error_check_good check_dbgetter_pagesize $db_pagesize $db_get_pagesize

	# Clean up.
	error_check_good db_close [$db close] 0
	error_check_good env_close [$e close] 0

}
