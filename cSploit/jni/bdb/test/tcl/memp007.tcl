# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#

# TEST	memp007
# TEST	Tests the mpool methods in the mpool file handle.
# TEST	(1) -clear_len, -lsn_offset and -pgcookie.
# TEST	(2) set_maxsize, get_maxsize and get_last_pgno.
proc memp007 { } {
	source ./include.tcl
	global default_pagesize

	#
	# Since we need to test mpool file max size {0 bytes} where
	# "bytes" <= 3 * page size, set the page size as 1024 in the
	# max size testing if the default page size > 1/3 GB.
	#
	set gigabytes [expr 1024 * 1024 * 1024]
	if { $default_pagesize > [expr $gigabytes / 3] } {
		set mp_pgsz 1024
	} else {
		set mp_pgsz $default_pagesize
	}

	#
	# The mpool file handle config options.
	# Structure of the list is:
	#    0. Arg used in mpool command
	#    1. Value assigned in mpool command and/or retrieved from getter
	#    2. Arg used in getter ("" means the open should fail)
	#
	set mlist {
	{ "-clear_len" "258" "get_clear_len" }
	{ "-clear_len" "[expr $mp_pgsz + 1]" "" }
	{ "-lsn_offset" "10" "get_lsn_offset" }
	{ "-pgcookie" "abc" "get_pgcookie" }
	}

	#
	# The size list for mpool file set/get max size.
	# Structure of the list is:
	#    0. Value assigned in -maxsize or set_maxsize command
	#    1. Value retrieved from getter
	#
	set szlist {
	{ {0 [expr $mp_pgsz - 1]} {0 0} }
	{ {0 $mp_pgsz} {0 0} }
	{ {0 [expr $mp_pgsz * 2]} {0 [expr $mp_pgsz * 2]} }
	{ {0 [expr $mp_pgsz * 2 + 1]} {0 [expr $mp_pgsz * 3]} }
	{ {0 1073741823} {1 0} }
	{ {1 0} {1 0} }
	{ {1 $mp_pgsz} {1 $mp_pgsz} }
	}

	puts "Memp007: Test the methods in the mpool file handle."

	# Clean up TESTDIR and open a new env.
	env_cleanup $testdir
	set env [eval {berkdb env} -create -home $testdir]
	error_check_good is_valid_env [is_valid_env $env] TRUE

	# The list of step letters.
	set step { a b c d e f g h i j k }
	# Test the mpool methods in the mpoolfile handle.
	set cnt 0
	foreach item $mlist {
		set flag [lindex $item 0]
		set flagval [eval list [lindex $item 1]]
		set getter [lindex $item 2]
		set letter [lindex $step $cnt]

		if { $getter != "" } {
			puts "\tMemp007.$letter.0: open mpool file\
			    $flag $flagval, should succeed"
			set mp [$env mpool -create \
			    -pagesize $default_pagesize -mode 0644 \
			    $flag $flagval mpfile_$letter]
			error_check_good memp_fopen \
			    [is_valid_mpool $mp $env] TRUE

			puts "\tMemp007.$letter.1: mpool file $getter"
			error_check_good get_flagval \
			    [eval $mp $getter] $flagval

			error_check_good mpclose [$mp close] 0
		} else {
			puts "\tMemp007.$letter.0: open mpool file\
			    $flag $flagval, should fail"
			set ret [catch {eval {$env mpool -create \
			    -pagesize $default_pagesize -mode 0644 \
			    $flag $flagval mpfile_$letter}} mp]
			error_check_bad mpool_open $ret 0
			error_check_good is_substr \
			    [is_substr $mp \
			    "clear length larger than page size"] 1
		}
		incr cnt
	}

	# Test setting the mpool file max size.
	foreach item $szlist {
		set maxsz [eval list [lindex $item 0]]
		set expectsz [eval list [lindex $item 1]]
		set letter [lindex $step $cnt]

		#
		# The mpool file max size can be set before and after opening
		# the mpool file handle. And there are 2 ways to hit the mpool
		# file size limit: 1) extend the file from the first page until
		# the file size gets larger than the limit and 2) create any
		# page whose page number is larger than the maximum page number
		# allowed in the file.
		# So we test it in this way:
		#    1. For the file max size >= 1 GB, set the max size before
		#    opening the mpool file handle. Since the file size limit
		#    is >= 1 GB (kind of big), we will create 3 pages with
		#    -create: the first page, the maximum page allowed and a
		#    page whose page number is larger than the max page number.
		#    2. For the file max size < 1 GB, set the max size after
		#    opening the mpool file handle and extend the file with
		#    -new until the file size hits the limit.
		#
		if { [lindex $expectsz 0] == 0 } {
			set bopen 0
			set flags "-new"
		} else {
			set bopen 1
			set flags "-create"
		}
		memp007_createpg \
		    $env $mp_pgsz $bopen $maxsz $expectsz $flags $letter
		incr cnt
	}

	$env close
}

proc memp007_createpg { env pgsz bopen maxsz expectmsz flags letter } {
	global testdir

	# Calculate the expected max file size and max page number.
	set gbytes [lindex $expectmsz 0]
	set bytes [lindex $expectmsz 1]
	set gigabytes [expr 1024 * 1024 * 1024]
	set expectfsz [expr $gbytes * $gigabytes + $bytes]
	set maxpgno \
	    [expr ($gbytes * ($gigabytes / $pgsz) + $bytes / $pgsz) - 1]

	# Setting the mpool file max size <= page size will remove the limit.
	if { $expectfsz <= $pgsz } {
		error_check_good mp_maxpgno $maxpgno -1
	} else {
		error_check_bad mp_maxpgno $maxpgno -1
		error_check_bad mp_maxpgno $maxpgno 0
	}

	# Set the maxsize before or after opening the mpool file handle.
	if { $bopen != 0} {
		puts "\tMemp007.$letter.0: open mpool file -pagesize $pgsz\
		    and set_maxsize $maxsz after opening"
		set mp [$env mpool \
		    -create -pagesize $pgsz -mode 0644 mpfile_$letter]
		error_check_good mpool:set_maxsize \
		    [eval {$mp set_maxsize $maxsz}] 0
	} else {
		puts "\tMemp007.$letter.0: open mpool file -pagesize $pgsz\
		    -maxsize $maxsz"
		set mp [$env mpool -create -pagesize $pgsz -maxsize $maxsz \
		    -mode 0644 mpfile_$letter]
	}
	error_check_good memp_fopen [is_valid_mpool $mp $env] TRUE

	puts "\tMemp007.$letter.1: mpool file get_maxsize $expectmsz"
	error_check_good mpool:get_maxsize [eval {$mp get_maxsize}] $expectmsz

	#
	# Get the expected error message when creating pages whose page numbers
	# are larger than the maximum allow.
	#
	set num [expr $maxpgno + 1]
	set msg "file limited to $num pages"

	if { $maxpgno == 0 } {
		puts "\tMemp007.$letter.2: setting max size <= page size will\
		    remove the size limit, so just do 1 call of get -new"
	} else {
		puts "\tMemp007.$letter.2: create pages with $flags\
		    until hitting the max file size"
	}
	if { $flags == "-new" } {
		# The last page number after the first call of get -new is 1.
		set pgno 1

		#
		# For the max size <= page size, size limit is removed, so
		# just do one call of get -new which actually creates 2 pages.
		#
		if { $expectfsz <= $pgsz } {
			set total 1
		} else {
			set total $maxpgno
		}

		# For DB_MPOOL_NEW, extend the file until hitting the max size.
		while { $pgno <= $total} {
			set pg [$mp get -new]
			error_check_good mpool_put [eval {$pg put}] 0
			error_check_good mpool:get_last_pgno \
			    [eval {$mp get_last_pgno}] $pgno
			incr pgno
		}
		#
		# Create a page that is larger than the maximum allowed and
		# check for the appropriate error.
		#
		if { $expectfsz > $pgsz } {
			set ret [catch {eval {$mp get -new}} pg]
			error_check_bad mpool_get $ret 0
			error_check_good is_substr [is_substr $pg $msg] 1
		}
	} else {
		# For DB_MPOOL_CREATE, create the first page.
		set pgno 0
		set pg [$mp get -create -dirty $pgno]
		error_check_good mpool_put [eval {$pg put}] 0
		error_check_good mpool:get_last_pgno \
		    [eval {$mp get_last_pgno}] $pgno

		# Create the page whose page number is "maxpgno".
		set pgno $maxpgno
		set pg [$mp get -create $pgno]
		error_check_good mpool_put [eval {$pg put}] 0
		error_check_good mpool:get_last_pgno \
		    [eval {$mp get_last_pgno}] $pgno

		#
		# Create the page whose page number is "maxpgno" + 1 and
		# check for the appropriate error.
		#
		set pgno [expr $maxpgno + 1]
		set ret [catch {eval {$mp get -create $pgno}} pg]
		error_check_bad mpool_get $ret 0
		error_check_good is_substr [is_substr $pg $msg] 1
	}
	$mp fsync
	$mp close

	# Verify the file size.
	puts "\tMemp007.$letter.3: verify the file size"
	#
	# For max size <= page size, we do only one call of get -new
	# which actually creates 2 pages.
	#
	if { $expectfsz <= $pgsz } {
		set expectfsz [expr $pgsz * 2]
	}
	set filesz [file size $testdir/mpfile_$letter]
	error_check_good file_size $filesz $expectfsz
}
