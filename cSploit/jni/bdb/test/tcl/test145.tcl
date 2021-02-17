# See the file LICENSE for redistribution information.
#
# Copyright (c) 2012, 2014 Oracle and/or its affiliates. All rights reserved.
#
# $Id$
#
# TEST	test145
# TEST	Tests setting the database creation directory
# TEST	in the environment and database handles.
# TEST	1. Test setting the directory in the environment handle 
# TEST	(1) sets the db creation directory in the env handle with -data_dir;
# TEST	(2) opens the env handle with the env home directory;
# TEST	(3) opens the db handle with the db file name and db name.
# TEST	2. Test setting the directory in the database handle.
# TEST	(1) adds the db creation directory to the data directory list in the
# TEST	env handle with -add_dir;
# TEST	(2) opens the env handle with the env home directory;
# TEST	(3) sets the db creation directory in the db handle with -create_dir;
# TEST	(4) opens the db handle with the db file name and db name.
proc test145 { method {tnum "145"} args } {
	source ./include.tcl

	# This test is only for the behavior of setting the db creation
	# directory. So it is fine to test it only for btree.
	if { [is_btree $method] != 1 } {
		puts "Test$tnum skipping for method $method"
		return
	}

	# If we are using an env, then skip this test.  It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Test$tnum skipping for env $env"
		return
	}

	# Skip -encryptaes since we are using our own env.
	set encindx [lsearch -exact $args "-encryptaes"]
	if { $encindx != -1 } {
		puts "Test$tnum skipping for -encryptaes"
		return
	}

	# This test uses its own database creation directory.
	set dirindx [lsearch -exact $args "-create_dir"]
	if { $dirindx != -1 } {
		incr dirindx
		set cdir [lindex $arg $dirindx]
		puts "Test$tnum skipping for -create_dir $cdir"
		return
	}

	set omethod [convert_method $method]
	set args [convert_args $method $args]

	puts "Test$tnum: $method ($args) Set database creation directory."

	set curdir [pwd]
	set createdir \
	    [list "$curdir/$testdir/DATA" "DATA" "$testdir/DATA"]
	set homedir [list "$curdir/$testdir" "$testdir"]
	set subdbname subdb

	set cnt1 1
	foreach h $homedir {
		foreach d $createdir {
			set cnt2 1
			# Clean the TESTDIR.
			env_cleanup $testdir
			file mkdir $testdir/DATA

			# Set the database creation directory in the
			# environment handle and open it.
			puts "\tTest$tnum.$cnt1.$cnt2:\
			    open env -home $h -data_dir $d"
			incr cnt2
			set env [eval {berkdb env} \
			    -create -home $h -data_dir $d]
			error_check_good is_valid_env [is_valid_env $env] TRUE

			# Open the database handle using a file name with path
			# prefix.
			set oflags " -env $env -create $args $omethod "
			foreach f $createdir {
				# If the db file name is prefixed with absolute
				# path, db open succeeds as long as the path
				# directory exists. Otherwise the db open
				# succeeds only when the directory $h/$d/$f
				# exists.
				if { [is_substr $f $curdir] || \
				    [file exists $h/$d/$f]} {
					set res 0
					set msg "should succeed"
				} else {
					set res 1
					set msg "should fail"
				}
				puts "\tTest$tnum.$cnt1.$cnt2: open db\
				    with db file $f/test$tnum.$cnt2.db ($msg)"
				test145_dbopen $oflags $f/test$tnum.$cnt2.db \
				    NULL test$tnum.$cnt2.db $res
				incr cnt2
				# Partition is not supported for sub-databases.
				if { [is_partitioned $args] == 0 } {
					puts "\tTest$tnum.$cnt1.$cnt2: open db\
					    with db file $f/test$tnum.$cnt2.db\
					    and db name $subdbname ($msg)"
					test145_dbopen $oflags \
					    $f/test$tnum.$cnt2.db $subdbname \
					    test$tnum.$cnt2.db $res
				} else {
					puts "\tTest$tnum.$cnt1.$cnt2: skip\
					    creating sub-databases with\
					    partitioning"
				}
				incr cnt2
			}
			# Open the database handle with just the file name
			# and no path prefix.
			# The db open succeeds only when the $d is an
			# absolute path or the directory $h/$d exists.
			if { [is_substr $d $curdir] || [file exists $h/$d] } {
				set res 0
				set msg "should succeed"
			} else {
				set res 1
				set msg "should fail"
			}
			puts "\tTest$tnum.$cnt1.$cnt2: open db\
			    with db file test$tnum.$cnt2.db ($msg)"
			test145_dbopen $oflags test$tnum.$cnt2.db \
			    NULL test$tnum.$cnt2.db $res
			incr cnt2
			# Partition is not supported for sub-databases.
			if { [is_partitioned $args] == 0 } {
				puts "\tTest$tnum.$cnt1.$cnt2: open db\
				    with db file test$tnum.$cnt2.db\
				    and db name $subdbname ($msg)"
				test145_dbopen $oflags test$tnum.$cnt2.db \
				    $subdbname test$tnum.$cnt2.db $res
			} else {
				puts "\tTest$tnum.$cnt1.$cnt2: skip creating\
				    sub-databases with partitioning"
			}
			$env close
			incr cnt1

			# Clean the TESTDIR.
			env_cleanup $testdir 
			file mkdir $testdir/DATA

			# Add the database creation directory to the data
			# directory list in the environment handle and open it.
			set cnt2 1
			puts "\tTest$tnum.$cnt1.$cnt2:\
			    open env -home $h -add_dir $d"
			incr cnt2
			set env [eval {berkdb env} \
			    -create -home $h -add_dir $d]
			error_check_good is_valid_env [is_valid_env $env] TRUE

			# Set the database creation directory in the database
			# handle and open it with the database file name that
			# with path prefix.
			set oflags " -env $env -create $args \
			    -create_dir $d $omethod "
			foreach f $createdir {
				# If the db file name is prefixed with absolute
				# path, db open succeeds as long as the path
				# directory exists. Otherwise the db open
				# succeeds only when the directory $h/$d/$f
				# exists.
				if { [is_substr $f $curdir] || \
				    [file exists $h/$d/$f]} {
					set res 0
					set msg "should succeed"
				} else {
					set res 1
					set msg "should fail"
				}
				puts "\tTest$tnum.$cnt1.$cnt2: open db\
				    with db file $f/test$tnum.$cnt2.db ($msg)"
				test145_dbopen $oflags $f/test$tnum.$cnt2.db \
				    NULL test$tnum.$cnt2.db $res
				incr cnt2
				# Partition is not supported for sub-databases.
				if { [is_partitioned $args] == 0 } {
					puts "\tTest$tnum.$cnt1.$cnt2: open db\
					    with db file $f/test$tnum.$cnt2.db\
					    and with db name $subdbname ($msg)"
					test145_dbopen $oflags \
					    $f/test$tnum.$cnt2.db $subdbname \
					    test$tnum.$cnt2.db $res
				} else {
					puts "\tTest$tnum.$cnt1.$cnt2: skip\
					    creating sub-databases with\
					    partitioning"
				}
				incr cnt2
			}

			# Set the database creation directory in the database
			# handle and open it with just the file name and no
			# path prefix.
			# The db open succeeds only when the $d is an
			# absolute path or the directory $h/$d exists.
			if { [is_substr $d $curdir] || [file exists $h/$d] } {
				set res 0
				set msg "should succeed"
			} else {
				set res 1
				set msg "should fail"
			}
			puts "\tTest$tnum.$cnt1.$cnt2: open db\
			    with db file test$tnum.$cnt2.db ($msg)"
			test145_dbopen $oflags test$tnum.$cnt2.db \
			    NULL test$tnum.$cnt2.db $res
			incr cnt2
			# Partition is not supported for sub-databases.
			if { [is_partitioned $args] == 0 } {
				puts "\tTest$tnum.$cnt1.$cnt2: open db\
				    with db file test$tnum.$cnt2.db\
				    and with db name $subdbname ($msg)"
				test145_dbopen $oflags test$tnum.$cnt2.db \
				    $subdbname test$tnum.$cnt2.db $res
			} else {
				puts "\tTest$tnum.$cnt1.$cnt2: skip creating\
				    sub-databases with partitioning"
			}
			$env close
			incr cnt1
		}
	}
}

proc test145_dbopen \
    { oflags dbfile dbname resfile res } {
	global testdir
	# Open the database handle and verify if result is as expected.
	if { $res != 0 } {
		if { $dbname != "NULL" } {
			set ret [catch {eval {berkdb open} \
			    $oflags $dbfile $dbname} res1]
		} else {
			set ret [catch {eval {berkdb open} \
			    $oflags $dbfile} res1]
		}
		error_check_bad dbopen $ret 0
	} else {
		if { $dbname != "NULL" } {
			set db [eval {berkdb open} \
			    $oflags $dbfile $dbname]
		} else {
			set db [eval {berkdb open} \
			    $oflags $dbfile]
		}
		error_check_good dbopen [is_valid_db $db] TRUE
		$db close
		error_check_good dbfile_exist \
		    [file exists $testdir/DATA/$resfile] 1
	}
}
