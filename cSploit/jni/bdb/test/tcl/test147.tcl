# See the file LICENSE for redistribution information.
#
# Copyright (c) 2013, 2014 Oracle and/or its affiliates. All rights reserved.
#
# $Id$
#
# TEST	test147
# TEST	Test db_stat and db_printlog with all allowed options.
proc test147 { method {tnum "147"} args } {
	source ./include.tcl
	global encrypt
	global passwd
	global EXE

	set args [convert_args $method $args]
	set omethod [convert_method $method]

	# db_stat and db_printlog do not support partition callback yet.
	set ptcbindex [lsearch -exact $args "-partition_callback"]
	if { $ptcbindex != -1 } {
		puts "Test$tnum: skip partition callback mode."
		return
	}

	# hpargs will contain arguments for homedir and password.
	set hpargs ""

	# Set up environment and home folder.
	set env NULL
	set secenv 0
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		set testdir [get_home $env]
		set secenv [is_secenv $env]
		set txnenv [is_txnenv $env]
		if { $txnenv == 1 } {
			append args " -auto_commit "
		}
		set testfile test$tnum.db
	} else {
		append args " -cachesize {0 1048576 3} "
		set testfile $testdir/test$tnum.db
	}
	set hpargs "-h $testdir"

	cleanup $testdir $env

	puts "Test$tnum: $method ($args) Test of db_stat and db_printlog."
	# Append password arg.
	if { $encrypt != 0 || $secenv != 0 } {
		append hpargs " -P $passwd"
	}

	# stat_file_args contains arguments used in command 'db_stat -d file'.
	set stat_file_args "-d test$tnum.db"

	# Create db and fill it with data.
	set db [eval {berkdb_open -create -mode 0644} $args\
	    $omethod $testfile]
	error_check_good db_open [is_valid_db $db] TRUE

	error_check_good db_fill [populate $db $method "" 1000 0 0] 0
	error_check_good db_close [$db close] 0

	puts "Test$tnum: testing db_stat with -d args."

	set binname db_stat
	set std_redirect "> /dev/null"
	if { $is_windows_test } {
		set std_redirect "> /nul"
		append binname $EXE
	}

	# Run the db_stat command for a specified file without extra options.
	test147_execmd "$binname $stat_file_args $hpargs $std_redirect"

	# Test for statistics.
	test147_execmd "$binname $stat_file_args -f $hpargs $std_redirect"

	# Do not acquire shared region mutexes while running.
	test147_execmd "$binname $stat_file_args -N $hpargs $std_redirect"

	puts "Test$tnum: testing db_stat without -d arg."

	# These flags can be used with -aNZ in the end.
	set flaglist [list "V" "L A"]
	set flaglist_env [list "E" "C A" "M A" "X A"]
	set end_flags [list "" "-a" "-N" "-Z"]

	foreach stflag $flaglist {
		if { $env != "NULL" && $stflag == "L A" && ![is_logenv $env] } {
			puts "\tTest$tnum: skip '-L A' in non-log env."
			continue
		}
		foreach endflag $end_flags {
			set combinearg $hpargs
			if { $endflag != "" } {
				set combinearg " $endflag $hpargs"
			}
			test147_execmd\
			    "$binname -$stflag $combinearg $std_redirect"
		}
	}

	# Skip these flags when db is not in environment.
	foreach stflag $flaglist_env {
		if { $env == "NULL" } {
			break
		}
		if { $stflag == "C A" && ![is_lockenv $env] } {
			puts "\tTest$tnum: skip '-C A' in non-lock env."
			continue
		}
		foreach endflag $end_flags {
			set combinearg $hpargs
			if { $endflag != "" } {
				set combinearg " $endflag $hpargs"
			}
			test147_execmd\
			    "$binname -$stflag $combinearg $std_redirect"
		}
	}

	# These flags can not be used with -aNZ in the end.
	set flaglist2_env [list "c" "e" "m" "r" "t" "x"\
	    "C c" "C l" "C o" "C p" "R A"]
	set flaglist2 [list "l"]

	foreach stflag $flaglist2 {
		if { $env != "NULL" && $stflag == "l" && ![is_logenv $env] } {
			puts "\tTest$tnum: skip '-l' in non-log env."
			continue
		}
		test147_execmd "$binname -$stflag $hpargs $std_redirect"
	}

	foreach stflag $flaglist2_env {
		if { $env == "NULL" } {
			break
		}
		if { $stflag == "r" && ![is_repenv $env] } {
			puts "\tTest$tnum: skip '-r' in non-rep env."
			continue
		}
		if { $stflag == "R A" && ![is_repenv $env] } {
			puts "\tTest$tnum: skip '-R A' in non-rep env."
			continue
		}
		if { $stflag == "c" && ![is_lockenv $env] } {
			puts "\tTest$tnum: skip '-c' in non-lock env."
			continue
		}
		if { [is_substr $stflag "C "] && ![is_lockenv $env] } {
			puts "\tTest$tnum: skip '-$stflag' in non-lock env."
			continue
		}
		if { $stflag == "t" && ![is_txnenv $env] } {
			puts "\tTest$tnum: skip '-t' in non-txn env."
			continue
		}
		test147_execmd "$binname -$stflag $hpargs $std_redirect"
	}

	# Check usage info is contained in error message.
	set execmd "$util_path/$binname $std_redirect"
	puts "\tTest$tnum: $execmd"
	catch {eval exec [split $execmd " "]} result
	error_check_good db_stat [is_substr $result "usage:"] 1

	if { $env != "NULL" && ![is_logenv $env] } {
		puts "Test$tnum: skip test db_printlog in non-log env."
		return
	}

	puts "Test$tnum: testing db_printlog."

	set binname db_printlog
	if { $is_windows_test } {
		append binname $EXE
	}

	set flaglist [list "-N" "-r" "-V" ""]
	foreach lgpflag $flaglist {
		set combinearg $hpargs
		if { $lgpflag != "" } {
			set combinearg " $lgpflag $hpargs"
		}
		test147_execmd "$binname $combinearg $std_redirect"
		# Test with given start and end LSN.
		test147_execmd "$binname -b 1/0 $combinearg $std_redirect"
		test147_execmd\
		    "$binname -e 1/1000 $combinearg $std_redirect"
		test147_execmd\
		    "$binname -b 1/0 -e 1/1000 $combinearg $std_redirect"
	}
}

proc test147_execmd { execmd } {
	source ./include.tcl
	puts "\tTest147: $util_path/$execmd"
	if { [catch {eval exec $util_path/$execmd} result] } {
		puts "FAIL: got $result while executing '$execmd'"
	}
}
