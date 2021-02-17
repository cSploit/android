# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id: test.tcl,v fa50617a1e4c 2012/08/17 09:59:58 carol $

source ./include.tcl

# Add the default Windows build sub-directory to the path, so that
# the binaries can be found without copies.
if {[string match Win* $tcl_platform(os)]} {
	global env
	global buildpath
	set env(PATH) "$env(PATH)\;$buildpath"
}

# Load DB's TCL API.
load $tcllib

# Check for existing files that might interfere with testing. 
set badfiles [glob -nocomplain DB_CONFIG __db.*]
if { [llength $badfiles] > 0 } {
	error "=====\nPlease move or delete these files from the current\
	    directory: \n$badfiles \nThey can cause test failures.\n====="
}

if { [file exists $testdir] != 1 } {
	file mkdir $testdir
}

global __debug_print
global __debug_on
global __debug_test

#
# Test if utilities work to figure out the path.  Most systems
# use ., but QNX has a problem with execvp of shell scripts which
# causes it to break.
#
set stat [catch {exec ./db_printlog -?} ret]
if { [string first "exec format error" $ret] != -1 } {
	set util_path ./.libs
} else {
	set util_path .
}
set __debug_print 0
set encrypt 0
set old_encrypt 0
set passwd test_passwd

# Error stream that (should!) always go to the console, even if we're
# redirecting to ALL.OUT.
set consoleerr stderr

set dict $test_path/wordlist
set alphabet "abcdefghijklmnopqrstuvwxyz"
set datastr "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"

# Random number seed.
global rand_init
set rand_init 11302005

# Default record length for fixed record length access method(s)
set fixed_len 20

set recd_debug	0
set log_log_record_types 0
set ohandles {}

# Normally, we're not running an all-tests-in-one-env run.  This matters
# for error stream/error prefix settings in berkdb_open.
global is_envmethod
set is_envmethod 0

#
# Set when we're running a child process in a rep test.
#
global is_repchild
set is_repchild 0

# Set when we want to use replication test messaging that cannot
# share an env -- for example, because the replication processes
# are not all from the same BDB version.
global noenv_messaging
set noenv_messaging 0

# For testing locker id wrap around.
global lock_curid
global lock_maxid
set lock_curid 0
set lock_maxid 2147483647
global txn_curid
global txn_maxid
set txn_curid 2147483648
set txn_maxid 4294967295

# The variable one_test allows us to run all the permutations
# of a test with run_all or run_std.
global one_test
if { [info exists one_test] != 1 } {
	set one_test "ALL"
}

# If you call a test with the proc find_valid_methods, it will
# return the list of methods for which it will run, instead of
# actually running.
# Btree and recno are always built, but hash, heap, and queue
# can be disabled, so verify that they are there before adding 
# them to the list.
source $test_path/testutils.tcl
global checking_valid_methods
set checking_valid_methods 0
global valid_methods
set valid_methods { btree rbtree recno frecno rrecno }

source $test_path/testutils.tcl
set conf [berkdb getconfig]
if { [is_substr $conf "queue"] } {
	lappend valid_methods "queue"
	lappend valid_methods "queueext"
}
if { [is_substr $conf "hash"] } {
	lappend valid_methods "hash"
}
if { [is_substr $conf "heap"] } {
	lappend valid_methods "heap"
}

# The variable test_recopts controls whether we open envs in
# replication tests with the -recover flag.   The default is
# to test with and without the flag, but to run a meaningful
# subset of rep tests more quickly, rep_subset will randomly
# pick one or the other.
global test_recopts
set test_recopts { "-recover" "" }

# Set up any OS-specific values.
global tcl_platform
set is_aix_test [string match AIX $tcl_platform(os)]
set is_freebsd_test [string match FreeBSD $tcl_platform(os)]
set is_hp_test [string match HP-UX $tcl_platform(os)]
set is_linux_test [string match Linux $tcl_platform(os)]
set is_osx_test [string match Darwin $tcl_platform(os)]
set is_qnx_test [string match QNX $tcl_platform(os)]
set is_sunos_test [string match SunOS $tcl_platform(os)]
set is_windows_test [string match Win* $tcl_platform(os)]
set is_windows9x_test [string match "Windows 95" $tcl_platform(osVersion)]
set is_je_test 0
set upgrade_be [big_endian]
global is_fat32
set is_fat32 [string match FAT32 [lindex [file system check] 1]]
global EXE BAT
if { $is_windows_test == 1 } {
	set EXE ".exe"
	set BAT ".bat"
} else {
	set EXE ""
	set BAT ""
}

if { $is_windows_test == 1 } {
	set util_path "./$buildpath"
}

# This is where the test numbering and parameters now live.
source $test_path/testparams.tcl
source $test_path/db_reptest.tcl

# Try to open an encrypted database.  If it fails, this release
# doesn't support encryption, and encryption tests should be skipped.
set has_crypto 1
set stat [catch {set db [eval {berkdb_open_noerr \
    -create -btree -encryptaes test_passwd} ] } result ]
if { $stat != 0 } {
	# Make sure it's the right error for a non-crypto release.
	error_check_good non_crypto_release \
	    [expr [is_substr $result "operation not supported"] || \
	    [is_substr $result "invalid argument"]] 1
	set has_crypto 0
} else {
	# It is a crypto release.  Get rid of the db, we don't need it.
	error_check_good close_encrypted_db [$db close] 0
}

# Get the default page size of this system
global default_pagesize
set db [berkdb_open_noerr -create -btree]
error_check_good "db open" [is_valid_db $db] TRUE
set stat [catch {set default_pagesize [$db get_pagesize]} result]
error_check_good "db get_pagesize" $stat 0
error_check_good "db close" [$db close] 0

# From here on out, test.tcl contains the procs that are used to
# run all or part of the test suite.

proc run_std { { testname ALL } args } {
	global test_names
	global one_test
	global has_crypto
	global valid_methods
	source ./include.tcl

	set one_test $testname
	if { $one_test != "ALL" } {
		# Source testparams again to adjust test_names.
		source $test_path/testparams.tcl
	}

	set exflgs [eval extractflags $args]
	set args [lindex $exflgs 0]
	set flags [lindex $exflgs 1]

	set display 1
	set run 1
	set am_only 0
	set no_am 0
	set std_only 1
	set rflags {--}
	foreach f $flags {
		switch $f {
			A {
				set std_only 0
			}
			M {
				set no_am 1
				puts "run_std: all but access method tests."
			}
			m {
				set am_only 1
				puts "run_std: access method tests only."
			}
			n {
				set display 1
				set run 0
				set rflags [linsert $rflags 0 "-n"]
			}
		}
	}

	if { $std_only == 1 } {
		fileremove -f ALL.OUT

		set o [open ALL.OUT a]
		if { $run == 1 } {
			puts -nonewline "Test suite run started at: "
			puts [clock format [clock seconds] -format "%H:%M %D"]
			puts [berkdb version -string]

			puts -nonewline $o "Test suite run started at: "
			puts $o [clock format [clock seconds] -format "%H:%M %D"]
			puts $o [berkdb version -string]
		}
		close $o
	}

	set test_list {
	{"environment"		"env"}
	{"archive"		"archive"}
	{"backup"		"backup"}
	{"file operations"	"fop"}
	{"locking"		"lock"}
	{"logging"		"log"}
	{"memory pool"		"memp"}
	{"mutex"		"mutex"}
	{"transaction"		"txn"}
	{"deadlock detection"	"dead"}
	{"subdatabase"		"sdb"}
	{"byte-order"		"byte"}
	{"recno backing file"	"rsrc"}
	{"DBM interface"	"dbm"}
	{"NDBM interface"	"ndbm"}
	{"Hsearch interface"	"hsearch"}
	{"secondary index"	"sindex"}
	{"partition"		"partition"}
	{"compression"		"compressed"}
	{"automated repmgr tests" 	"auto_repmgr"}
	{"other repmgr tests" 	"other_repmgr"}
	{"repmgr multi-process"	"multi_repmgr"}
	{"expected failures"	"fail"}
	}

	# If this is run_std only, run each rep test for a single
	# access method.  If run_all, run for all access methods.
	if { $std_only == 1 } {
		lappend test_list {"replication"	"rep_subset"}
	} else {
		lappend test_list {"replication"	"rep_complete"}
	}

	# If release supports encryption, run security tests.
	if { $has_crypto == 1 } {
	        lappend test_list {"security"   "sec"}
	}

	if { $am_only == 0 } {
		foreach pair $test_list {
			set msg [lindex $pair 0]
			set cmd [lindex $pair 1]
			puts "Running $msg tests"
			if [catch {exec $tclsh_path << \
			    "global one_test; set one_test $one_test; \
			    source $test_path/test.tcl; r $rflags $cmd" \
			    >>& ALL.OUT } res] {
				set o [open ALL.OUT a]
				puts $o "FAIL: $cmd test: $res"
				close $o
			}
		}

		# Run recovery tests.
		#
		# XXX These too are broken into separate tclsh instantiations
		# so we don't require so much memory, but I think it's cleaner
		# and more useful to do it down inside proc r than here,
		# since "r recd" gets done a lot and needs to work.
		#
		# Note that we still wrap the test in an exec so that
		# its output goes to ALL.OUT.  run_recd will wrap each test
		# so that both error streams go to stdout (which here goes
		# to ALL.OUT);  information that run_recd wishes to print
		# to the "real" stderr, but outside the wrapping for each test,
		# such as which tests are being skipped, it can still send to
		# stderr.
		puts "Running recovery tests"
		if [catch {
		    exec $tclsh_path << \
		    "global one_test; set one_test $one_test; \
		    source $test_path/test.tcl; r $rflags recd" \
			2>@ stderr >> ALL.OUT
		    } res] {
			set o [open ALL.OUT a]
			puts $o "FAIL: recd tests: $res"
			close $o
		}

		# Run join test
		#
		# XXX
		# Broken up into separate tclsh instantiations so we don't
		# require so much memory.
		if { $one_test == "ALL" } {
			puts "Running join test"
			foreach test "join1 join2 join3 join4 join5 join6" {
				if [catch {exec $tclsh_path << \
				    "source $test_path/test.tcl; r $rflags $test" \
				    >>& ALL.OUT } res] {
					set o [open ALL.OUT a]
					puts $o "FAIL: $test test: $res"
					close $o
				}
			}
		}
	}

	if { $no_am == 0 } {
		# Access method tests.
		#
		# XXX
		# Broken up into separate tclsh instantiations so we don't
		# require so much memory.
		foreach method $valid_methods {
			puts "Running $method tests"
			foreach test $test_names(test) {
				if { $run == 0 } {
					set o [open ALL.OUT a]
					run_method \
					    -$method $test $display $run $o
					close $o
				}
				if { $run } {
					if [catch {exec $tclsh_path << \
					    "global one_test; \
					    set one_test $one_test; \
					    source $test_path/test.tcl; \
					    run_method \
					    -$method $test $display $run"\
					    >>& ALL.OUT } res] {
						set o [open ALL.OUT a]
						puts $o "FAIL:$test $method: $res"
						close $o
					}
				}
			}
		}
	}

	# If not actually running, no need to check for failure.
	# If running in the context of the larger 'run_all' we don't
	# check for failure here either.
	if { $run == 0 || $std_only == 0 } {
		return
	}

	set failed [check_output ALL.OUT]

	set o [open ALL.OUT a]
	if { $failed == 0 } {
		puts "Regression Tests Succeeded"
		puts $o "Regression Tests Succeeded"
	} else {
		puts "Regression Tests Failed"
		puts "Check UNEXPECTED OUTPUT lines."
		puts "Review ALL.OUT.x for details."
		puts $o "Regression Tests Failed"
	}

	puts -nonewline "Test suite run completed at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	puts -nonewline $o "Test suite run completed at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	close $o
}

proc check_output { file } {
	# These are all the acceptable patterns.
	set pattern {(?x)
		^[:space:]*$|
		.*?wrap\.tcl.*|
		.*?dbscript\.tcl.*|
		.*?ddscript\.tcl.*|
		.*?db_replicate.*|
		.*?Freeing\slog\sinformation\s.*|
		.*?Freeing\smutex\s.*|
		.*?Freeing\sread\slocks\s.*|
		.*?lt-db_replicate.*|
		.*?mpoolscript\.tcl.*|
		^\d\d:\d\d:\d\d\s\(\d\d:\d\d:\d\d\)$|
		^\d\d:\d\d:\d\d\s\(\d\d:\d\d:\d\d\)\sCrashing$|
		^\d\d:\d\d:\d\d\s\(\d\d:\d\d:\d\d\)\s[p|P]rocesses\srunning:.*|
		^\d\d:\d\d:\d\d\s\(\d\d:\d\d:\d\d\)\s5\sprocesses\srunning.*|
		^\d:\sPut\s\d*\sstrings\srandom\soffsets.*|
		^100.*|
		^basic_repmgr_.*\swith:|
		^eval\s.*|
		^exec\s.*|
		^jointest.*$|
		^r\sarchive\s*|
		^r\sbackup\s*|
		^r\sdbm\s*|
		^r\shsearch\s*|
		^r\sndbm\s*|
		^run_recd:\s.*|
		^run_reptest\s.*|
		^run_secenv:\s.*|
		^All\sprocesses\shave\sexited.$|
		^Backuptest\s.*|
		^Beginning\scycle\s\d$|
		^Berkeley\sDB\s.*|
		^Byteorder:.*|
		^Child\sruns\scomplete\.\s\sParent\smodifies\sdata\.$|
		^Deadlock\sdetector:\s\d*\sCheckpoint\sdaemon\s\d*$|
		^Ending\srecord.*|
		^Environment\s.*?specified;\s\sskipping\.$|
		^Executing\srecord\s.*|
		^Join\stest:\.*|
		^Method:\s.*|
		^Putting\s.*databases.*|
		^Repl:\stest\d\d\d:.*|
		^Repl:\ssdb\d\d\d:.*|
		^Running\stest\ssdb.*|
		^Running\stest\stest.*|
		^Running\sall\scases\sof\s.*|
		^run_inmem_db\s.*rep.*|
		^run_inmem_log\s.*rep.*|
		^run_mixedmode_log\s.*rep.*|
		^Script\swatcher\sprocess\s.*|
		^Secondary\sindex\sjoin\s.*|
		^Test\ssuite\srun\s.*|
                ^Test\s.*rep.*|
		^To\sreproduce\sthis\scase:.*|
		^Unlinking\slog:\serror\smessage\sOK$|
		^Verifying\s.*|
		^\t*\.\.\.dbc->get.*$|
		^\t*\.\.\.dbc->put.*$|
		^\t*\.\.\.key\s\d.*$|
		^\t*\.\.\.Skipping\sdbc.*|
		^\t*and\s\d*\sduplicate\sduplicates\.$|
		^\t*About\sto\srun\srecovery\s.*complete$|
		^\t*Add\sa\sthird\sversion\s.*|
		^\t*Archive[:\.].*|
		^\t*Backuptest.*|
		^\t*Basic\srepmgr\s.*test.*:.*|
		^\t*Bigfile[0-9][0-9][0-9].*|
		^\t*Building\s.*|
		^\t*bulk\sprocessing.*|
		^\t*closing\ssecondaries\.$|
		^\t*Command\sexecuted\sand\s.*$|
		^\t*DBM.*|
		^\t*[d|D]ead[0-9][0-9][0-9].*|
		^\t*Dump\/load\sof.*|
		^\t*[e|E]nv[0-9][0-9][0-9].*|
		^\t*Executing\scommand$|
		^\t*Executing\stxn_.*|
		^\t*[F|f]ail[0-9][0-9][0-9].*|
		^\t*File\srecd005\.\d\.db\sexecuted\sand\saborted\.$|
		^\t*File\srecd005\.\d\.db\sexecuted\sand\scommitted\.$|
		^\t*[f|F]op[0-9][0-9][0-9].*|
		^\t*HSEARCH.*|
		^\t*in-memory\s.*|
		^\t*Initial\sCheckpoint$|
		^\t*Iteration\s\d*:\sCheckpointing\.$|
		^\t*Joining:\s.*|
		^\t*Kid[1|2]\sabort\.\.\.complete$|
		^\t*Kid[1|2]\scommit\.\.\.complete$|
		^\t*[l|L]ock[0-9][0-9][0-9].*|
		^\t*[l|L]og[0-9][0-9][0-9].*|
		^\t*[m|M]emp[0-9][0-9][0-9].*|
		^\t*[m|M]ut[0-9][0-9][0-9].*|
		^\t*NDBM.*|
		^\t*no\speering|
		^\t*on-disk\s.*|
		^\t*opening\ssecondaries\.$|
		^\t*op_recover_rec:\sRunning\srecovery.*|
		^\t*peering|
		^\t*[r|R]ecd[0-9][0-9][0-9].*|
		^\t*[r|R]ep[0-9][0-9][0-9].*|
		^\t*[r|R]epmgr[0-9][0-9][0-9].*|
		^\t*[r|R]ep_push.*|
		^\t*[r|R]ep_test.*|
		^\t*[r|R]pc[0-9][0-9][0-9].*|
		^\t*[r|R]src[0-9][0-9][0-9].*|
		^\t*Recover\sfrom\sfirst\sdatabase$|
		^\t*Recover\sfrom\ssecond\sdatabase$|
		^\t*regular\sprocessing.*|
		^\t*Remove\ssecond\sdb$|
		^\t*Rep_verify.*|
		^\t*Running\srecovery\son\s.*|
		^\t*[s|S]ec[0-9][0-9][0-9].*|
		^\t*[s|S]i[0-9][0-9][0-9].*|
		^\t*[s|S]ijoin.*|
		^\t*Salvage\stests\sof.*|
		^\t*sdb[0-9][0-9][0-9].*|
		^\t*Skipping\s.*|
		^\t*Subdb[0-9][0-9][0-9].*|
		^\t*Subdbtest[0-9][0-9][0-9].*|
		^\t*Syncing$|
		^\t*[t|T]est[0-9][0-9][0-9].*|
		^\t*[t|T]xn[0-9][0-9][0-9].*|
		^\t*Txnscript.*|
		^\t*Using\s.*option.*$|
		^\t*Using\s.*?\senvironment\.$|
		^\t*Verification\sof.*|
		^\t*with\stransactions$}

	set failed 0
	set f [open $file r]
	while { [gets $f line] >= 0 } {
		if { [regexp $pattern $line] == 0 } {
			puts -nonewline "UNEXPECTED OUTPUT: "
			puts $line
			set failed 1
		}
	}
	close $f
	return $failed
}

proc r { args } {
	global test_names
	global has_crypto
	global rand_init
	global one_test
	global test_recopts
	global checking_valid_methods

	source ./include.tcl

	set exflgs [eval extractflags $args]
	set args [lindex $exflgs 0]
	set flags [lindex $exflgs 1]

	set display 1
	set run 1
	set saveflags "--"
	foreach f $flags {
		switch $f {
			n {
				set display 1
				set run 0
				set saveflags "-n $saveflags"
			}
		}
	}

	if {[catch {
		set sub [ lindex $args 0 ]
		set starttest [lindex $args 1]
		switch $sub {
			auto_repmgr -
			bigfile -
			dead -
			env -
			fail -
			lock -
			log -
			memp -
			multi_repmgr -
			mutex -
			other_repmgr -
			rsrc -
			sdbtest -
			txn {
				if { $display } {
					run_subsystem $sub 1 0 $starttest
				}
				if { $run } {
					run_subsystem $sub 0 1 $starttest
				}
			}
			backup {
				if { $one_test == "ALL" } {
					run_test backup $display $run
				}
			}
			byte {
				if { $one_test == "ALL" } {
					run_test byteorder $display $run
				}
			}
			archive -
			dbm -
			hsearch -
			ndbm -
			shelltest {
				if { $one_test == "ALL" } {
					if { $display } { puts "eval $sub" }
					if { $run } {
						check_handles
						eval $sub
					}
				}
			}
			compact -
			elect -
			inmemdb -
			init -
			fop {
				set tindx [lsearch $test_names($sub) $starttest]
				if { $tindx == -1 } {
					set tindx 0
				}
				set rlist [lrange $test_names($sub) $tindx end]
				foreach test $rlist {
					eval run_test $test $display $run
				}
			}
			compressed {
				set tindex [lsearch $test_names(test) $starttest]
				if { $tindex == -1 } {
					set tindex 0
				}
				set clist [lrange $test_names(test) $tindex end]
				set clist [concat $clist $test_names(sdb)]
				foreach test $clist {
					# Each skipping test can be removed from 
					# below list if related bug is fixed.
					# (sdb006 - [#22058])(sdb013 - [#22055])
					# (sdb017 - [#22056])(sdb018 - [#22062])
					if { $test == "sdb006" ||
					     $test == "sdb013" ||
					     $test == "sdb017" ||
					     $test == "sdb018" } {
						continue
					}
					eval run_compressed\
					     btree $test $display $run
				}
			}
			join {
				eval r $saveflags join1
				eval r $saveflags join2
				eval r $saveflags join3
				eval r $saveflags join4
				eval r $saveflags join5
				eval r $saveflags join6
			}
			join1 {
				if { $display } { puts "eval jointest" }
				if { $run } {
					check_handles
					eval jointest
				}
			}
			joinbench {
				puts "[timestamp]"
				eval r $saveflags join1
				eval r $saveflags join2
				puts "[timestamp]"
			}
			join2 {
				if { $display } { puts "eval jointest 512" }
				if { $run } {
					check_handles
					eval jointest 512
				}
			}
			join3 {
				if { $display } {
					puts "eval jointest 8192 0 -join_item"
				}
				if { $run } {
					check_handles
					eval jointest 8192 0 -join_item
				}
			}
			join4 {
				if { $display } { puts "eval jointest 8192 2" }
				if { $run } {
					check_handles
					eval jointest 8192 2
				}
			}
			join5 {
				if { $display } { puts "eval jointest 8192 3" }
				if { $run } {
					check_handles
					eval jointest 8192 3
				}
			}
			join6 {
				if { $display } { puts "eval jointest 512 3" }
				if { $run } {
					check_handles
					eval jointest 512 3
				}
			}
			partition {
				foreach method { btree hash } {
					foreach test "$test_names(recd)\
					    $test_names(test)" {
						run_range_partition\
						    $test $method $display $run 
						run_partition_callback\
						    $test $method $display $run
					}
				}
			}
			recd {
				check_handles
				eval {run_recds all $run $display} [lrange $args 1 end]
			}
			rep {
				run_rep_subset rep $starttest $testdir \
				    $display $run $args
			}
			repmgr {
				r other_repmgr
				foreach test $test_names(basic_repmgr) {
					$test 100 1 1 1 1 1 
					$test 100 1 0 0 0 0 
					$test 100 0 1 0 0 0
					$test 100 0 0 1 0 0 
					$test 100 0 0 0 1 0 
					$test 100 0 0 0 0 1 
					$test 100 0 0 0 0 0 
				}
			}
			rep_commit {
				run_rep_subset rep_commit $starttest $testdir \
				    $display $run $args
				r repmgr
			}
			# To run a subset of the complete rep tests, use
			# rep_subset, which randomly picks an access type to
			# use, and randomly picks whether to open envs with
			# the -recover flag.
			rep_subset {
				run_rep_subset rep $starttest $testdir \
				    $display $run $args 
			}
			rep_complete {
				set tindex [lsearch $test_names(rep) $starttest]
				if { $tindex == -1 } {
					set tindex 0
				}
				set rlist [lrange $test_names(rep) $tindex end]
				foreach test $rlist {
					run_test $test $display $run
				}
				if { $one_test == "ALL" } {
					if { $display } {
						#puts "basic_db_reptest"
						#puts "basic_db_reptest 1"
					}
					if { $run } {
						#basic_db_reptest
						#basic_db_reptest 1
					}
				}
			}
			replicate {
				# We seed the random number generator here
				# instead of in run_replicate so that we
				# aren't always reusing the first few
				# responses from random_int.
				#
				berkdb srand $rand_init
				foreach sub { test sdb } {
					foreach test $test_names($sub) {
						eval run_test run_replicate \
						    $display $run $test
					}
				}
			}
			repmethod {
				# We seed the random number generator here
				# instead of in run_repmethod so that we
				# aren't always reusing the first few
				# responses from random_int.
				#
				berkdb srand $rand_init
				foreach sub { test sdb } {
					foreach test $test_names($sub) {
						eval run_test run_repmethod \
						    $display $run $test
					}
				}
			}
			sec {
				# Skip secure mode tests if release
				# does not support encryption.
				if { $has_crypto == 0 } {
					return
				}
				if { $display } {
					run_subsystem $sub 1 0
				}
				if { $run } {
					run_subsystem $sub 0 1
				}
			}
			secmethod {
				# Skip secure mode tests if release
				# does not support encryption.
				if { $has_crypto == 0 } {
					return
				}
				foreach test $test_names(test) {
					eval run_test run_secmethod \
					    $display $run $test
					eval run_test run_secenv \
					    $display $run $test
				}
			}
			sdb {
				if { $one_test == "ALL" } {
					if { $display } {
						run_subsystem sdbtest 1 0
					}
					if { $run } {
						run_subsystem sdbtest 0 1
					}
				}
				foreach test $test_names(sdb) {
					eval run_test $test $display $run
				}
			}
			sindex {
				if { $one_test == "ALL" } {
					if { $display } {
						sindex 1 0
						sijoin 1 0
					}
					if { $run } {
						sindex 0 1
						sijoin 0 1
					}
				}
			}
			btree -
			rbtree -
			hash -
			iqueue -
			iqueueext -
			queue -
			queueext -
			recno -
			frecno -
			heap -
			rrecno {
				foreach test $test_names(test) {
					eval run_method [lindex $args 0] $test \
					    $display $run stdout [lrange $args 1 end]
				}
			}

			default {
				error \
				    "FAIL:[timestamp] r: $args: unknown command"
			}
		}
		flush stdout
		flush stderr
	} res] != 0} {
		global errorInfo;
		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp] r: $args: $theError"
		} else {
			error $theError;
		}
	}
}

proc run_rep_subset { sub starttest testdir display run args } {
	global one_test
	global rand_init
	global test_names

	if  { [is_partition_callback $args] == 1 } {
		set nodump 1
	} else {
		set nodump 0
	}
	berkdb srand $rand_init
	set tindex [lsearch $test_names($sub) $starttest]
	if { $tindex == -1 } {
		set tindex 0
	}
	set rlist [lrange $test_names($sub) $tindex end]
	foreach test $rlist {
		set random_recopt [berkdb random_int 0 1]
		if { $random_recopt == 1 } {
			set test_recopts "-recover"
		} else {
			set test_recopts {""}
		}

		set method_list [find_valid_methods $test]
		set list_length [expr [llength $method_list] - 1]
		set method_index [berkdb random_int 0 $list_length]
		set rand_method [lindex $method_list $method_index]

		if { $display } {
			puts "eval $test $rand_method; verify_dir \
			    $testdir \"\" 1 0 $nodump; salvage_dir $testdir 1"
		}
		if { $run } {
	 		check_handles
			eval $test $rand_method
			verify_dir $testdir "" 1 0 $nodump
			salvage_dir $testdir 1
		}
	}
	if { $one_test == "ALL" } {
		if { $display } {
			#puts "basic_db_reptest"
			#puts "basic_db_reptest 1"
		}
		if { $run } {
			#basic_db_reptest
			#basic_db_reptest 1
		}
	}
	set test_recopts { "-recover" "" }
}

proc run_subsystem { sub {display 0} {run 1} {starttest "NULL"} } {
	global test_names
	global databases_in_memory

	if { [info exists test_names($sub)] != 1 } {
		puts stderr "Subsystem $sub has no tests specified in\
		    testparams.tcl; skipping."
		return
	}

	set index [lsearch $test_names($sub) $starttest]
	if { $index == -1 } {
		set index 0
	}
	set testlist [lrange $test_names($sub) $index end]

	foreach test $testlist {
		if { $display } {
			puts "eval $test"
		}
		if { $run } {
			check_handles
			if {[catch {eval $test} ret] != 0 } {
				set databases_in_memory 0
				error "FAIL: run_subsystem: $sub $test: \
				    $ret"
			}
		}
	}
}

proc run_test { test {display 0} {run 1} args } {
	source ./include.tcl
	global valid_methods

	foreach method $valid_methods {
		if { $display } {
			puts "eval $test -$method $args; \
			    verify_dir $testdir \"\" 1; \
			    salvage_dir $testdir 1"
		}
		if  { [is_partition_callback $args] == 1 } {
			set nodump 1
		} else {
			set nodump 0
		}
		if { $run } {
	 		check_handles
			eval {$test -$method} $args
			verify_dir $testdir "" 1 0 $nodump
			salvage_dir $testdir 1
		}
	}
}

proc run_method { method test {display 0} {run 1} \
    { outfile stdout } args } {
	global __debug_on
	global __debug_print
	global __debug_test
	global test_names
	global parms
	source ./include.tcl

	if  { [is_partition_callback $args] == 1 } {
		set nodump  1
	} else {
		set nodump  0
	}

	if {[catch {
		if { $display } {
			puts -nonewline $outfile "eval \{ $test \} $method"
			puts -nonewline $outfile " $parms($test) { $args }"
			puts -nonewline $outfile " ; verify_dir $testdir \"\" 1 0 $nodump"
			puts $outfile " ; salvage_dir $testdir 1"
		}
		if { $run } {
			check_handles $outfile
			puts $outfile "[timestamp]"
			eval {$test} $method $parms($test) $args
			if { $__debug_print != 0 } {
				puts $outfile ""
			}
			# Verify all databases the test leaves behind
			verify_dir $testdir "" 1 0 $nodump
			if { $__debug_on != 0 } {
				debug $__debug_test
			}
			salvage_dir $testdir 1
		}
		flush stdout
		flush stderr
	} res] != 0} {
		global errorInfo;

		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_method: $method $test: $theError"
		} else {
			error $theError;
		}
	}
}

# Run a testNNN or recdNNN test with range partitioning.
proc run_range_partition { test method {display 0} {run 1}\
    {outfile stdout} args } { 

	# The only allowed access method for range partitioning is btree.
	if { [is_btree $method] == 0 } {
		if { $display == 0 } {
			puts "Skipping range partition\
			    tests for method $method"
		}
		return
	}

	# If we've passed in explicit partitioning args, use them; 
	# otherwise set them.  This particular selection hits some 
	# interesting cases where we set the key to "key".
	set largs $args
	if { [is_partitioned $args] == 0 } {
		lappend largs  -partition {ab cd key key1 zzz}
	}

	if { [string first recd $test] == 0 } {
		eval {run_recd $method $test $run $display} $largs
	} elseif { [string first test $test] == 0 } {
		eval {run_method $method $test $display $run $outfile} $largs
	} else {
		puts "Skipping test $test with range partitioning."
	}
}

# Run a testNNN or recdNNN test with partition callbacks.
proc run_partition_callback { test method {display 0} {run 1}\
    {outfile stdout} args } {

	# The only allowed access methods are btree and hash.
	if { [is_btree $method] == 0 && [is_hash $method] == 0 } {
		if { $display == 0 } {
			puts "Skipping partition callback tests\
			    for method $method"
		}
		return
	}

	# If we've passed in explicit partitioning args, use them; 
	# otherwise set them. 
	set largs $args
	if { [is_partition_callback $args] == 0 } {
		lappend largs  -partition_callback 5 part 
	}

	if { [string first recd $test] == 0 } {
		eval {run_recd $method $test $run $display} $largs
	} elseif { [string first test $test] == 0 } {
		eval {run_method $method $test $display $run $outfile} $largs
	} else {
		puts "Skipping test $test with partition callbacks."
	}
}

#
# Run method tests for btree only using compression.
#
proc run_compressed { method test {display 0} {run 1} \
    { outfile stdout } args } {

	if { [is_btree $method] == 0 } {
		puts "Skipping compression test for method $method."
		return
	}

	set largs $args
	append largs " -compress "
	eval run_method $method $test $display $run $outfile $largs
}

#
# Run method tests in secure mode.
#
proc run_secmethod { method test {display 0} {run 1} \
    { outfile stdout } args } {
	global passwd
	global has_crypto

	# Skip secure mode tests if release does not support encryption.
	if { $has_crypto == 0 } {
		return
	}

	set largs $args
	append largs " -encryptaes $passwd "
	eval run_method $method $test $display $run $outfile $largs
}

#
# Run method tests each in its own, new secure environment.
#
proc run_secenv { method test {largs ""} } {
	global __debug_on
	global __debug_print
	global __debug_test
	global is_envmethod
	global has_crypto
	global test_names
	global parms
	global passwd
	source ./include.tcl

	# Skip secure mode tests if release does not support encryption.
	if { $has_crypto == 0 } {
		return
	}

	puts "run_secenv: $method $test $largs"

	set save_largs $largs
	env_cleanup $testdir
	set is_envmethod 1
	set stat [catch {
		check_handles
		set env [eval {berkdb_env -create -mode 0644 -home $testdir \
		    -encryptaes $passwd -pagesize 512 -cachesize {0 4194304 1}}]
		error_check_good env_open [is_valid_env $env] TRUE
		append largs " -env $env "

		puts "[timestamp]"
		if { [info exists parms($test)] != 1 } {
			puts stderr "$test disabled in\
			    testparams.tcl; skipping."
			continue
		}

		#
		# Run each test multiple times in the secure env.
		# Once with a secure env + clear database
		# Once with a secure env + secure database
		#
		eval $test $method $parms($test) $largs
		append largs " -encrypt "
		eval $test $method $parms($test) $largs

		if { $__debug_print != 0 } {
			puts ""
		}
		if { $__debug_on != 0 } {
			debug $__debug_test
		}
		flush stdout
		flush stderr
		set largs $save_largs
		error_check_good envclose [$env close] 0
		error_check_good envremove [berkdb envremove \
		    -home $testdir -encryptaes $passwd] 0
	} res]
	if { $stat != 0} {
		global errorInfo;

		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_secenv: $method $test: $theError"
		} else {
			error $theError;
		}
	set is_envmethod 0
	}

}

#
# Run replication method tests in master and client env.
# This proc runs a specific test/method using the db_replicate utility.
#
proc run_replicate_test { method test {nsites 2} {largs "" } } {
	source ./include.tcl

	global __debug_on
	global __debug_print
	global __debug_test
	global errorInfo
	global has_crypto
	global is_envmethod
	global masterdir
	global parms
	global passwd
	global rep_verbose
	global repenv
	global verbose_type

	puts "run_replicate_test $method $test $nsites $largs"

	# Test124 can't be run under reptest because we delete all 
	# the test files at the end of the test to avoid triggering 
	# verification failures (it uses a non-standard sort).    
	if { $test == "test124" } {
		puts "Skipping $test under run_replicate"
		return
	}
		
	set verbargs ""
	if { $rep_verbose == 1 } {
		set verbargs " -verbose {$verbose_type on}"
	}
	set do_sec 0
	env_cleanup $testdir
	set is_envmethod 1

	# Some tests that use a small db pagesize need a small 
	# mpool pagesize as well -- otherwise we'll run out of 
	# mutexes.   First determine the natural pagesize, so 
	# that can be used in the normal case, then adjust where
	# needed. 

	set tmpenv [berkdb_env -create -home $testdir]
	set pg [$tmpenv get_mp_pagesize]
	error_check_good env_close [$tmpenv close] 0
	berkdb envremove -home $testdir

	set small_pagesize_tests [list test035 test096 test112 test113 test114]
	if { [lsearch -exact $small_pagesize_tests $test] != -1  } {
		set pg 512
	}

	#
	# Set log smaller than default to force changing files,
	# but big enough so that the tests that use binary files
	# as keys/data can run.  Increase the size of the log region --
	# sdb004 needs this, now that subdatabase names are stored
	# in the env region.
	#
	# All the settings below will be the same for all sites in the group.
	#
	set logmax [expr 3 * 1024 * 1024]
	set lockmax 40000
	set logregion 2097152

	#
	# TODO:  Turn on crypto and test with that.  Off for now.
	#
	if { $do_sec && $has_crypto } {
		set envargs "-encryptaes $passwd"
		append largs " -encrypt "
	} else {
		set envargs ""
	}
	check_handles
	set last_site [expr $nsites - 1]
	set winner [berkdb random_int 0 $last_site]
	for { set i 0 } { $i < $nsites } { incr i } {
		set repdir($i) $testdir/ENV$i
		file mkdir $repdir($i)
		if { $i == $winner } {
			set pri 10
		} else {
			set pri [berkdb random_int 0 1]
		}
		replicate_make_config $repdir($i) $nsites $i $pri
		set envcmd($i) "berkdb_env_noerr -create -log_max $logmax \
		    $envargs -rep -home $repdir($i) -txn -thread -pagesize $pg \
		    -log_regionmax $logregion -lock_max_objects $lockmax \
		    -lock_max_locks $lockmax -errpfx $repdir($i) $verbargs"
		set env($i) [eval $envcmd($i)]
		error_check_good env_open($i) [is_valid_env $env($i)] TRUE
	}

	#
	# Now that we have all of the envs opened, we can start db_replicate
	# in each one too.  Afterward, we check for which site is master.
	#
	for { set i 0 } { $i < $nsites } { incr i } {
		set dpid($i) [eval {exec $util_path/db_replicate -t 3 \
		    -h $repdir($i)} -L $testdir/LOG$i &]
		puts "Started db_replicate $repdir($i): $dpid($i)"
	}

	#
	# Wait for enough sites to start and elect someone master.
	# For now assume that once the master is elected, all sites
	# have started up and we don't have any laggards.  If that
	# seems to be a problem we could loop checking whether every
	# single env knows this master and is at the right LSN.
	#
	puts "run_replicate_test: Wait for repmgr to elect a master."
	await_expected_master $env($winner) 30

	set masterdir $repdir($winner)
	#
	# Set up list of client env handles for later checking
	# and verification.  Skip the master env.
	#
	set j 0
	set repenv(master) $env($winner)
	for { set i 0 } { $i < $nsites } { incr i } {
		if { $winner != $i } {
			set repenv($j) $env($i)
			incr j
		}
	}
	puts "run_replicate_test: Found master at $repdir($winner)"
	#
	# Give a few seconds for the clients to sync with the master
	# before we begin blasting at them.  If we don't pause here,
	# we otherwise will race with the db_replicate process that is
	# in rep_start and our test will fail with DB_LOCK_DEADLOCK.
	# This pause gives the group a chance to quiesce.
	#
	tclsleep 5

	#
	# We went through all that so that we can append '-env masterenv'
	# to the largs for the test.  Clobber the 30-second anti-archive
	# timer in case the test we're about to run wants to do any log
	# archiving, database renaming and/or removal.
	#
	$env($winner) test force noarchive_timeout
	append largs " -env $env($winner) "

	#
	# Now run the actual test.
	#
	set stat [catch {
		puts "[timestamp]"
		if { [info exists parms($test)] != 1 } {
			puts stderr "$test disabled in\
			    testparams.tcl; skipping."
			continue
		}

		puts -nonewline "Replicate: $test: $nsites sites "
		if { $do_sec } {
			puts -nonewline " with security;"
		} else {
			puts -nonewline " no security;"
		}
		puts ""

		eval $test $method $parms($test) $largs

		if { $__debug_print != 0 } {
			puts ""
		}
		if { $__debug_on != 0 } {
			debug $__debug_test
		}
		flush stdout
		flush stderr
	} res]
	#
	# Test is over.  We must kill the db_replicate processes no matter
	# whether there was an error or not.
	# And we must close the envs.  We save the original errorInfo
	# because it could be overwritten by tclkill.
	#
	puts "Replicate: $test: Done ($stat).  Wait and kill db_replicate."
	set save_errInfo $errorInfo
	tclsleep 10
	#
	# We kill all the clients first then kill the master.  If we
	# just kill them in order, and kill the master first, the others
	# may complete an election and the processes get killed in the
	# middle of recovery, thus leaving the env locked out which is
	# a problem in the verify phase.
	#
	for { set i 0 } { $i < $nsites } { incr i } {
		if { $i != $winner } {
			tclkill $dpid($i)
		}
	}
	tclsleep 2
	tclkill $dpid($winner)
	if { $stat != 0} {
		for { set i 0 } { $i < $nsites } { incr i } {
			catch { $env($i) close } ignore
		}

		puts "Error result string: $res"
		set fnl [string first "\n" $save_errInfo]
		set theError [string range $save_errInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $save_errInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_reptest: $method $test: $theError"
		} else {
			error $theError;
		}
	} else {
		repl_envver0 $test $method [expr $nsites - 1]
		for { set i 0 } { $i < $nsites } { incr i } {
			catch { $env($i) close } ignore
		}
	}

	set is_envmethod 0
}

#
# Run replication method tests in master and client env.
# This proc runs a specific test/method with our own message handling.
#
proc run_reptest { method test {droppct 0} {nclients 1} {do_del 0} \
    {do_sec 0} {do_oob 0} {largs "" } } {
	source ./include.tcl

	global __debug_on
	global __debug_print
	global __debug_test
	global is_envmethod
	global parms
	global passwd
	global has_crypto

	puts "run_reptest \
	    $method $test $droppct $nclients $do_del $do_sec $do_oob $largs"

	# Test124 can't be run under reptest because we delete all 
	# the test files at the end of the test to avoid triggering 
	# verification failures (it uses a non-standard sort).
	if { $test == "test124"} {
		puts "Skipping $test under run_repmethod"
		return
	}
		
	env_cleanup $testdir
	set is_envmethod 1
	set stat [catch {
		if { $do_sec && $has_crypto } {
			set envargs "-encryptaes $passwd"
			append largs " -encrypt "
		} else {
			set envargs ""
		}
		check_handles
		#
		# This will set up the master and client envs
		# and will return us the args to pass to the
		# test.

		set largs [repl_envsetup \
		    $envargs $largs $test $nclients $droppct $do_oob]

		puts "[timestamp]"
		if { [info exists parms($test)] != 1 } {
			puts stderr "$test disabled in\
			    testparams.tcl; skipping."
			continue
		}

		puts -nonewline \
		    "Repl: $test: dropping $droppct%, $nclients clients "
		if { $do_del } {
			puts -nonewline " with delete verification;"
		} else {
			puts -nonewline " no delete verification;"
		}
		if { $do_sec } {
			puts -nonewline " with security;"
		} else {
			puts -nonewline " no security;"
		}
		if { $do_oob } {
			puts -nonewline " with out-of-order msgs;"
		} else {
			puts -nonewline " no out-of-order msgs;"
		}
		puts ""

		eval $test $method $parms($test) $largs

		if { $__debug_print != 0 } {
			puts ""
		}
		if { $__debug_on != 0 } {
			debug $__debug_test
		}
		flush stdout
		flush stderr
		repl_envprocq $test $nclients $do_oob
		repl_envver0 $test $method $nclients
		if { $do_del } {
			repl_verdel $test $method $nclients
		}
		repl_envclose $test $envargs
	} res]
	if { $stat != 0} {
		global errorInfo;

		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_reptest: $method $test: $theError"
		} else {
			error $theError;
		}
	}
	set is_envmethod 0
}

#
# Run replication method tests in master and client env.
# Wrapper to run db_replicate utility test.
#
proc run_replicate { method test {nums 0} {display 0} {run 1} \
    {outfile stdout} {largs ""} } {
	source ./include.tcl

	set save_largs $largs
	env_cleanup $testdir

	#
	# Run 2 sites 40%, 3 sites 40%, 4 sites 10%, 5 sites 10%
	set site_list { 2 2 2 2 3 3 3 3 4 5 }
	set s_len [expr [llength $site_list] - 1]

	if { $nums == 0 } {
		set sindex [berkdb random_int 0 $s_len]
		set nsites [lindex $site_list $sindex]
	} else {
		set nsites $nums
	}

	if { $display == 1 } {
		puts $outfile "eval run_replicate_test $method $test \
		    $nsites $largs"
	}
	if { $run == 1 } {
		run_replicate_test $method $test $nsites $largs
	}
}

#
# Run replication method tests in master and client env.
# Wrapper to run a test on a replicated group.
#
proc run_repmethod { method test {numcl 0} {display 0} {run 1} \
    {outfile stdout} {largs ""} } {
	source ./include.tcl

	global __debug_on
	global __debug_print
	global __debug_test
	global is_envmethod
	global test_names
	global parms
	global has_crypto
	global passwd

	set save_largs $largs
	env_cleanup $testdir

	# Use an array for number of clients because we really don't
	# want to evenly-weight all numbers of clients.  Favor smaller
	# numbers but test more clients occasionally.
	set drop_list { 0 0 0 0 0 1 1 5 5 10 20 }
	set drop_len [expr [llength $drop_list] - 1]
	set client_list { 1 1 2 1 1 1 2 2 3 1 }
	set cl_len [expr [llength $client_list] - 1]

	if { $numcl == 0 } {
		set clindex [berkdb random_int 0 $cl_len]
		set nclients [lindex $client_list $clindex]
	} else {
		set nclients $numcl
	}
	set drindex [berkdb random_int 0 $drop_len]
	set droppct [lindex $drop_list $drindex]

	# Do not drop messages on Windows.  Since we can't set 
	# re-request times with less than millisecond precision, 
	# dropping messages will cause test failures. 
	if { $is_windows_test == 1 } {
		set droppct 0
	}

 	set do_sec [berkdb random_int 0 1]
	set do_oob [berkdb random_int 0 1]
	
	# Test130 cannot run with delete verification. [#18944]
	if { $test == "test130" } {
		set do_del 0 
	} else {
		set do_del [berkdb random_int 0 1]
	}

	if { $display == 1 } {
		puts $outfile "eval run_reptest $method $test $droppct \
		    $nclients $do_del $do_sec $do_oob $largs"
	}
	if { $run == 1 } {
		run_reptest $method $test $droppct $nclients $do_del \
		    $do_sec $do_oob $largs
	}
}

#
# Run method tests, each in its own, new environment.  (As opposed to
# run_envmethod1 which runs all the tests in a single environment.)
#
proc run_envmethod { method test {display 0} {run 1} {outfile stdout} \
    { largs "" } } {
	global __debug_on
	global __debug_print
	global __debug_test
	global is_envmethod
	global test_names
	global parms
	source ./include.tcl

	set save_largs $largs
	set envargs ""

	# Enlarge the logging region by default - sdb004 needs this because
	# it uses very long subdb names, and the names are stored in the 
	# env region.
	set logargs " -log_regionmax 2057152 "

	# Enlarge the cache by default - some compaction tests need it.
	set cacheargs "-cachesize {0 4194304 1} -pagesize 512"
	env_cleanup $testdir

	if { $display == 1 } {
		if { $run == 0 } {
			puts $outfile "eval run_envmethod $method $test 0 1 \
			    stdout $largs; verify_log $testdir"
		} else {
			puts $outfile "eval run_envmethod $method \
			    $test 0 1 stdout $largs"
		}
	}

	# To run a normal test using system memory, call run_envmethod
	# with the flag -shm.
	set sindex [lsearch -exact $largs "-shm"]
	if { $sindex >= 0 } {
		set shm_key 20
		if { [mem_chk " -system_mem -shm_key $shm_key"] == 1 } {
			break
		} else {
			append envargs " -system_mem -shm_key $shm_key"
			set largs [lreplace $largs $sindex $sindex]
		}
	}

	set sindex [lsearch -exact $largs "-log_max"]
	if { $sindex >= 0 } {
		append envargs " -log_max 100000 "
		set largs [lreplace $largs $sindex $sindex]
	}

	# Test for -thread option and pass to berkdb_env open.  Leave in
	# $largs because -thread can also be passed to an individual
	# test as an arg.  Double the number of lockers because a threaded
	# env requires more than an ordinary env.
	if { [lsearch -exact $largs "-thread"] != -1 } {
		append envargs " -thread -lock_max_lockers 2000 "
	}

	# Test for -alloc option and pass to berkdb_env open only.
	# Remove from largs because -alloc is not an allowed test arg.
	set aindex [lsearch -exact $largs "-alloc"]
	if { $aindex >= 0 } {
		append envargs " -alloc "
		set largs [lreplace $largs $aindex $aindex]
	}

	# We raise the number of locks and objects - there are a few
	# compaction tests that require a large number.
	set lockargs " -lock_max_locks 40000 -lock_max_objects 20000 "

	if { $run == 1 } {
		set is_envmethod 1
		set stat [catch {
			check_handles
			set env [eval {berkdb_env -create -txn -mode 0644 \
			    -home $testdir} $logargs $cacheargs $lockargs $envargs]
			error_check_good env_open [is_valid_env $env] TRUE
			append largs " -env $env "

			puts "[timestamp]"
			if { [info exists parms($test)] != 1 } {
				puts stderr "$test disabled in\
				    testparams.tcl; skipping."
				continue
			}
			eval $test $method $parms($test) $largs

			if { $__debug_print != 0 } {
				puts ""
			}
			if { $__debug_on != 0 } {
				debug $__debug_test
			}
			flush stdout
			flush stderr
			set largs $save_largs
			error_check_good envclose [$env close] 0
#			error_check_good envremove [berkdb envremove \
#			    -home $testdir] 0
		} res]
		if { $stat != 0} {
			global errorInfo;

			set fnl [string first "\n" $errorInfo]
			set theError [string range $errorInfo 0 [expr $fnl - 1]]
			if {[string first FAIL $errorInfo] == -1} {
				error "FAIL:[timestamp]\
				    run_envmethod: $method $test: $theError"
			} else {
				error $theError;
			}
		}
		set is_envmethod 0
	}
}

proc run_compact { method } {
	source ./include.tcl
	for {set tnum 111} {$tnum <= 115} {incr tnum} {
		run_envmethod $method test$tnum 0 1 stdout -log_max

		puts "\tTest$tnum: Test Recovery"
		set env1 [eval  berkdb env -create -txn \
		    -recover_fatal -home $testdir]
		error_check_good env_close [$env1 close] 0
		error_check_good verify_dir \
		    [verify_dir $testdir "" 0 0 1 ] 0
		puts "\tTest$tnum: Remove db and test Recovery"
		exec sh -c "rm -f $testdir/*.db"
		set env1 [eval  berkdb env -create -txn \
		    -recover_fatal -home $testdir]
		error_check_good env_close [$env1 close] 0
		error_check_good verify_dir \
		    [verify_dir $testdir "" 0 0 1 ] 0
	}
}

proc run_recd { method test {run 1} {display 0} args } {
	global __debug_on
	global __debug_print
	global __debug_test
	global parms
	global test_names
	global log_log_record_types
	global gen_upgrade_log
	global upgrade_be
	global upgrade_dir
	global upgrade_method
	global upgrade_name
	source ./include.tcl

	if { $run == 1 } {
		puts "run_recd: $method $test $parms($test) $args"
	}
	if {[catch {
		if { $display } {
			puts "eval { $test } $method $parms($test) { $args }"
		}
		if { $run } {
			check_handles
			set upgrade_method $method
			set upgrade_name $test
			puts "[timestamp]"
			# By redirecting stdout to stdout, we make exec
			# print output rather than simply returning it.
			# By redirecting stderr to stdout too, we make
			# sure everything winds up in the ALL.OUT file.
			set ret [catch { exec $tclsh_path << \
			    "source $test_path/test.tcl; \
			    set log_log_record_types $log_log_record_types;\
			    set gen_upgrade_log $gen_upgrade_log;\
			    set upgrade_be $upgrade_be; \
			    set upgrade_dir $upgrade_dir; \
			    set upgrade_method $upgrade_method; \
			    set upgrade_name $upgrade_name; \
			    eval { $test } $method $parms($test) {$args}" \
			    >&@ stdout
			} res]

			# Don't die if the test failed;  we want
			# to just proceed.
			if { $ret != 0 } {
				puts "FAIL:[timestamp] $res"
			}

			if { $__debug_print != 0 } {
				puts ""
			}
			if { $__debug_on != 0 } {
				debug $__debug_test
			}
			flush stdout
			flush stderr
		}
	} res] != 0} {
		global errorInfo;

		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_recd: $method: $theError"
		} else {
			error $theError;
		}
	}
}

proc recds {method args} {
	eval {run_recds $method 1 0} $args
}

proc run_recds {{run_methods "all"} {run 1} {display 0} args } {
	source ./include.tcl
	global log_log_record_types
	global test_names
	global gen_upgrade_log
	global encrypt
	global valid_methods

	set log_log_record_types 1
	set run_zero 0
	if { $run_methods == "all" } {
		set run_methods  $valid_methods
		set run_zero 1
	}
	logtrack_init

	# Define a small set of tests to run with log file zeroing.
	set zero_log_tests \
	    {recd001 recd002 recd003 recd004 recd005 recd006 recd007}

	foreach method $run_methods {
		check_handles
#set test_names(recd) "recd005 recd017"
		foreach test $test_names(recd) {
			# Skip recd017 for non-crypto upgrade testing.
			# Run only recd017 for crypto upgrade testing.
			if { $gen_upgrade_log == 1 && $test == "recd017" && \
			    $encrypt == 0 } {
				puts "Skipping recd017 for non-crypto run."
				continue
			}
			if { $gen_upgrade_log == 1 && $test != "recd017" && \
			    $encrypt == 1 } {
				puts "Skipping $test for crypto run."
				continue
			}
			if { [catch {eval {run_recd $method $test $run \
			    $display} $args} ret ] != 0 } {
				puts $ret
			}
	
			# If it's one of the chosen tests, and btree, run with 
			# log file zeroing.
			set zlog_idx [lsearch -exact $zero_log_tests $test]
			if { $run_zero == 1 && \
			    $method == "btree" && $zlog_idx > -1 } {
				if { [catch {eval {run_recd $method $test \
				    $run $display -zero_log} $args} ret ] != 0 } { 
					puts $ret
				}	
			}

			if { $gen_upgrade_log == 1 } {
				save_upgrade_files $testdir
			}
		}
	}

	# We can skip logtrack_summary during the crypto upgrade run -
	# it doesn't introduce any new log types.
	if { $run } {
		if { $gen_upgrade_log == 0 || $encrypt == 0 } {
			logtrack_summary
		}
	}
	set log_log_record_types 0
}

# A small subset of tests to be used in conjunction with the 
# automated builds.  Ideally these tests will cover a lot of ground
# but run in only 15 minutes or so.  You can put any test in the 
# list of tests and it will be run all the ways that run_all 
# runs it. 
proc run_smoke { } { 
	source ./include.tcl
	global valid_methods
	
	fileremove -f SMOKE.OUT

	set smoke_tests { \ 
	    lock001 log001 test001 test004 sdb001 sec001 rep001 txn001 }

	# Run each test in all its permutations, and 
	# concatenate the results in the file SMOKE.OUT.
	foreach test $smoke_tests {
		run_all $test
		set in [open ALL.OUT r]
		set out [open SMOKE.OUT a]
		while { [gets $in str] != -1 } {
			puts $out $str
		}
		close $in
		close $out
	}
}

proc run_inmem_tests { { testname ALL } args } {
	global test_names
	global one_test
	global valid_methods
	source ./include.tcl

	fileremove -f ALL.OUT

	set one_test $testname
	# Source testparams again to adjust test_names.
	source $test_path/testparams.tcl

	set exflgs [eval extractflags $args]
	set flags [lindex $exflgs 1]
	set display 1
	set run 1
	foreach f $flags {
		switch $f {
			n {
				set display 1
				set run 0
			}
		}
	}

	set o [open ALL.OUT a]
	if { $run == 1 } {
		puts -nonewline "Test suite run started at: "
		puts [clock format [clock seconds] -format "%H:%M %D"]
		puts [berkdb version -string]

		puts -nonewline $o "Test suite run started at: "
		puts $o [clock format [clock seconds] -format "%H:%M %D"]
		puts $o [berkdb version -string]
	}
	close $o

	# Run in-memory testing for databases, logs, replication files, 
	# and region files (env -private).  It is not necessary to run 
	# both run_inmem_log and run_mixedmode_log because run_mixedmode_log
	# includes the pure in-memory case. 
	set inmem_procs [list run_inmem_db \
	    run_inmem_log run_mixedmode_log run_inmem_rep run_env_private]

	# The above 3 procs only support tests like repXXX, so we only run 
	# these tests here. 
	foreach inmem_proc $inmem_procs {
		foreach method $valid_methods {
			foreach test $test_names(rep) {
				# Skip the rep tests that don't support
				# particular kinds of in-memory testing
				# when appropriate. 
				if { $inmem_proc == "run_inmem_db" } {
					set indx [lsearch -exact \
					    $test_names(skip_for_inmem_db) $test]
					if { $indx >= 0 } {
						continue
					}
				}
				if { $inmem_proc == "run_inmem_rep" } {
					set indx [lsearch -exact \
					    $test_names(skip_for_inmem_rep) $test]
					if { $indx >= 0 } {
						continue
					}
				}
				if { $inmem_proc == "run_env_private" } {
					set indx [lsearch -exact \
					    $test_names(skip_for_env_private) $test]
					if { $indx >= 0 } {
						continue
					}
				}

				if { $display } {
					set o [open ALL.OUT a]
					puts $o "eval \
					    $inmem_proc $test -$method; \
					    verify_dir $testdir \"\" 1 0 0; \
					    salvage_dir $testdir"
					close $o
				}

				if { $run } {
					if [catch {exec $tclsh_path << \
					    "global one_test; \
					    set one_test $one_test; \
					    source $test_path/test.tcl; \
					    eval $inmem_proc $test -$method;\
					    verify_dir $testdir \"\" 1 0 0; \
					    salvage_dir $testdir" \
					    >>& ALL.OUT } res ] {
						set o [open ALL.OUT a]
						puts $o "FAIL:$inmem_proc \
						    $test -$method: $res"
						close $o
					}
				}
			}
		}
	}

	if { $run == 0 } {
		return 
	}

	set failed [check_output ALL.OUT]

	set o [open ALL.OUT a]
	if { $failed == 0 } {
		puts "Regression Tests Succeeded"
		puts $o "Regression Tests Succeeded"
	} else {
		puts "Regression Tests Failed"
		puts "Check UNEXPECTED OUTPUT lines."
		puts "Review ALL.OUT.x for details."
		puts $o "Regression Tests Failed"
	}

	puts -nonewline "Test suite run completed at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	puts -nonewline $o "Test suite run completed at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	close $o

}


proc run_all { { testname ALL } args } {
	global test_names
	global one_test
	global has_crypto
	global valid_methods
	source ./include.tcl

	fileremove -f ALL.OUT

	set one_test $testname
	if { $one_test != "ALL" } {
		# Source testparams again to adjust test_names.
		source $test_path/testparams.tcl
	}

	set exflgs [eval extractflags $args]
	set flags [lindex $exflgs 1]
	set display 1
	set run 1
	set am_only 0
	set parallel 0
	set nparalleltests 0
	set rflags {--}
	foreach f $flags {
		switch $f {
			m {
				set am_only 1
			}
			n {
				set display 1
				set run 0
				set rflags [linsert $rflags 0 "-n"]
			}
		}
	}

	set o [open ALL.OUT a]
	if { $run == 1 } {
		puts -nonewline "Test suite run started at: "
		puts [clock format [clock seconds] -format "%H:%M %D"]
		puts [berkdb version -string]

		puts -nonewline $o "Test suite run started at: "
		puts $o [clock format [clock seconds] -format "%H:%M %D"]
		puts $o [berkdb version -string]
	}
	close $o
	#
	# First run standard tests.  Send in a -A to let run_std know
	# that it is part of the "run_all" run, so that it doesn't
	# print out start/end times.
	#
	lappend args -A
	eval {run_std} $one_test $args

	set test_pagesizes [get_test_pagesizes]
	set args [lindex $exflgs 0]
	set save_args $args

	foreach pgsz $test_pagesizes {
		set args $save_args
		append args " -pagesize $pgsz -chksum"
		if { $am_only == 0 } {
			# Run recovery tests.
			#
			# XXX These don't actually work at multiple pagesizes;
			# disable them for now.
			#
			# XXX These too are broken into separate tclsh
			# instantiations so we don't require so much
			# memory, but I think it's cleaner
			# and more useful to do it down inside proc r than here,
			# since "r recd" gets done a lot and needs to work.
			#
			# XXX See comment in run_std for why this only directs
			# stdout and not stderr.  Don't worry--the right stuff
			# happens.
			#puts "Running recovery tests with pagesize $pgsz"
			#if [catch {exec $tclsh_path \
			#    << "source $test_path/test.tcl; \
			#    r $rflags recd $args" \
			#    2>@ stderr >> ALL.OUT } res] {
			#	set o [open ALL.OUT a]
			#	puts $o "FAIL: recd test:"
			#	puts $o $res
			#	close $o
			#}
		}

		# Access method tests.
		# Run subdb tests with varying pagesizes too.
		# XXX
		# Broken up into separate tclsh instantiations so
		# we don't require so much memory.
		foreach method $valid_methods {
			puts "Running $method tests with pagesize $pgsz"
			foreach sub {test sdb si} {
				foreach test $test_names($sub) {
					if { $run == 0 } {
						set o [open ALL.OUT a]
						eval {run_method -$method \
						    $test $display $run $o} \
						    $args
						close $o
					}
					if { $run } {
						if [catch {exec $tclsh_path << \
						    "global one_test; \
						    set one_test $one_test; \
						    source $test_path/test.tcl; \
						    eval {run_method -$method \
						    $test $display $run \
						    stdout} $args" \
						    >>& ALL.OUT } res] {
							set o [open ALL.OUT a]
							puts $o "FAIL: \
							    -$method $test: $res"
							close $o
						}
					}
				}
			}
		}
	}
	set args $save_args
	#
	# Run access method tests at default page size in one env.
	#
	foreach method $valid_methods {
		puts "Running $method tests in a txn env"
		foreach sub {test sdb si} {
			foreach test $test_names($sub) {
				if { $run == 0 } {
					set o [open ALL.OUT a]
					run_envmethod -$method $test $display \
					    $run $o $args
					close $o
				}
				if { $run } {
					if [catch {exec $tclsh_path << \
					    "global one_test; \
					    set one_test $one_test; \
					    source $test_path/test.tcl; \
					    run_envmethod -$method $test \
				  	    $display $run stdout $args" \
					    >>& ALL.OUT } res] {
						set o [open ALL.OUT a]
						puts $o "FAIL: run_envmethod \
						    $method $test: $res"
						close $o
					}
				}
			}
		}
	}
	#
	# Run access method tests at default page size in thread-enabled env.
	# We're not truly running threaded tests, just testing the interface.
	#
	foreach method $valid_methods {
		puts "Running $method tests in a threaded txn env"
		foreach sub {test sdb si} {
			foreach test $test_names($sub) {
				if { $run == 0 } {
					set o [open ALL.OUT a]
					eval {run_envmethod -$method $test \
					    $display $run $o -thread}
					close $o
				}
				if { $run } {
					if [catch {exec $tclsh_path << \
					    "global one_test; \
					    set one_test $one_test; \
					    source $test_path/test.tcl; \
					    eval {run_envmethod -$method $test \
				  	    $display $run stdout -thread}" \
					    >>& ALL.OUT } res] {
						set o [open ALL.OUT a]
						puts $o "FAIL: run_envmethod \
						    $method $test -thread: $res"
						close $o
					}
				}
			}
		}
	}
	#
	# Run access method tests at default page size with -alloc enabled.
	#
	foreach method $valid_methods {
		puts "Running $method tests in an env with -alloc"
		foreach sub {test sdb si} {
			foreach test $test_names($sub) {
				if { $run == 0 } {
					set o [open ALL.OUT a]
					eval {run_envmethod -$method $test \
					    $display $run $o -alloc}
					close $o
				}
				if { $run } {
					if [catch {exec $tclsh_path << \
					    "global one_test; \
					    set one_test $one_test; \
					    source $test_path/test.tcl; \
					    eval {run_envmethod -$method $test \
				  	    $display $run stdout -alloc}" \
					    >>& ALL.OUT } res] {
						set o [open ALL.OUT a]
						puts $o "FAIL: run_envmethod \
						    $method $test -alloc: $res"
						close $o
					}
				}
			}
		}
	}

	# Run standard access method tests under replication.
	#
	set test_list [list {"testNNN under replication"	"repmethod"}]

	# If we're on Windows, Linux, FreeBSD, or Solaris, run the
	# bigfile tests.  These create files larger than 4 GB.
	if { $is_freebsd_test == 1 || $is_linux_test == 1 || \
	    $is_sunos_test == 1 || $is_windows_test == 1 } {
		lappend test_list {"big files"	"bigfile"}
	}

	# If release supports encryption, run security tests.
	#
	if { $has_crypto == 1 } {
		lappend test_list {"testNNN with security"	"secmethod"}
	}

	foreach pair $test_list {
		set msg [lindex $pair 0]
		set cmd [lindex $pair 1]
		puts "Running $msg tests"
		if [catch {exec $tclsh_path << \
		    "global one_test; set one_test $one_test; \
		    source $test_path/test.tcl; \
		    r $rflags $cmd $args" >>& ALL.OUT } res] {
			set o [open ALL.OUT a]
			puts $o "FAIL: $cmd test: $res"
			close $o
		}
	}

	# If not actually running, no need to check for failure.
	if { $run == 0 } {
		return
	}

	set failed 0
	set o [open ALL.OUT r]
	while { [gets $o line] >= 0 } {
		if { [regexp {^FAIL} $line] != 0 } {
			set failed 1
		}
	}
	close $o
	set o [open ALL.OUT a]
	if { $failed == 0 } {
		puts "Regression Tests Succeeded"
		puts $o "Regression Tests Succeeded"
	} else {
		puts "Regression Tests Failed; see ALL.OUT for log"
		puts $o "Regression Tests Failed"
	}

	puts -nonewline "Test suite run completed at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	puts -nonewline $o "Test suite run completed at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	close $o
}

proc run_all_new { { testname ALL } args } {
	global test_names
	global one_test
	global has_crypto
	global valid_methods
	source ./include.tcl

	fileremove -f ALL.OUT

	set one_test $testname
	if { $one_test != "ALL" } {
		# Source testparams again to adjust test_names.
		source $test_path/testparams.tcl
	}

	set exflgs [eval extractflags $args]
	set flags [lindex $exflgs 1]
	set display 1
	set run 1
	set am_only 0
	set parallel 0
	set nparalleltests 0
	set rflags {--}
	foreach f $flags {
		switch $f {
			m {
				set am_only 1
			}
			n {
				set display 1
				set run 0
				set rflags [linsert $rflags 0 "-n"]
			}
		}
	}

	set o [open ALL.OUT a]
	if { $run == 1 } {
		puts -nonewline "Test suite run started at: "
		puts [clock format [clock seconds] -format "%H:%M %D"]
		puts [berkdb version -string]

		puts -nonewline $o "Test suite run started at: "
		puts $o [clock format [clock seconds] -format "%H:%M %D"]
		puts $o [berkdb version -string]
	}
	close $o
	#
	# First run standard tests.  Send in a -A to let run_std know
	# that it is part of the "run_all" run, so that it doesn't
	# print out start/end times.
	#
	lappend args -A
	eval {run_std} $one_test $args

	set test_pagesizes [get_test_pagesizes]
	set args [lindex $exflgs 0]
	set save_args $args

	#
	# Run access method tests at default page size in one env.
	#
	foreach method $valid_methods {
		puts "Running $method tests in a txn env"
		foreach sub {test sdb si} {
			foreach test $test_names($sub) {
				if { $run == 0 } {
					set o [open ALL.OUT a]
					run_envmethod -$method $test $display \
					    $run $o $args
					close $o
				}
				if { $run } {
					if [catch {exec $tclsh_path << \
					    "global one_test; \
					    set one_test $one_test; \
					    source $test_path/test.tcl; \
					    run_envmethod -$method $test \
				  	    $display $run stdout $args" \
					    >>& ALL.OUT } res] {
						set o [open ALL.OUT a]
						puts $o "FAIL: run_envmethod \
						    $method $test: $res"
						close $o
					}
				}
			}
		}
	}
	#
	# Run access method tests at default page size in thread-enabled env.
	# We're not truly running threaded tests, just testing the interface.
	#
	foreach method $valid_methods {
		puts "Running $method tests in a threaded txn env"
		set thread_tests "test001"	
		foreach test $thread_tests {
			if { $run == 0 } {
				set o [open ALL.OUT a]
				eval {run_envmethod -$method $test \
				    $display $run $o -thread}
				close $o
			}
			if { $run } {
				if [catch {exec $tclsh_path << \
				    "global one_test; \
				    set one_test $one_test; \
				    source $test_path/test.tcl; \
				    eval {run_envmethod -$method $test \
			  	    $display $run stdout -thread}" \
				    >>& ALL.OUT } res] {
					set o [open ALL.OUT a]
					puts $o "FAIL: run_envmethod \
					    $method $test -thread: $res"
					close $o
				}
			}
		}
	}
	#
	# Run access method tests at default page size with -alloc enabled.
	#
	foreach method $valid_methods {
		puts "Running $method tests in an env with -alloc"
		set alloc_tests "test001"
		foreach test $alloc_tests {
			if { $run == 0 } {
				set o [open ALL.OUT a]
				eval {run_envmethod -$method $test \
				    $display $run $o -alloc}
				close $o
			}
			if { $run } {
				if [catch {exec $tclsh_path << \
				    "global one_test; \
				    set one_test $one_test; \
				    source $test_path/test.tcl; \
				    eval {run_envmethod -$method $test \
			  	    $display $run stdout -alloc}" \
				    >>& ALL.OUT } res] {
					set o [open ALL.OUT a]
					puts $o "FAIL: run_envmethod \
					    $method $test -alloc: $res"
					close $o
				}
			}
		}
	}

	# Run standard access method tests under replication.
	#
	set test_list [list {"testNNN under replication"	"repmethod"}]

	# If we're on Windows, Linux, FreeBSD, or Solaris, run the
	# bigfile tests.  These create files larger than 4 GB.
	if { $is_freebsd_test == 1 || $is_linux_test == 1 || \
	    $is_sunos_test == 1 || $is_windows_test == 1 } {
		lappend test_list {"big files"	"bigfile"}
	}

	# If release supports encryption, run security tests.
	#
	if { $has_crypto == 1 } {
		lappend test_list {"testNNN with security"	"secmethod"}
	}

	foreach pair $test_list {
		set msg [lindex $pair 0]
		set cmd [lindex $pair 1]
		puts "Running $msg tests"
		if [catch {exec $tclsh_path << \
		    "global one_test; set one_test $one_test; \
		    source $test_path/test.tcl; \
		    r $rflags $cmd $args" >>& ALL.OUT } res] {
			set o [open ALL.OUT a]
			puts $o "FAIL: $cmd test: $res"
			close $o
		}
	}

	# If not actually running, no need to check for failure.
	if { $run == 0 } {
		return
	}

	set failed 0
	set o [open ALL.OUT r]
	while { [gets $o line] >= 0 } {
		if { [regexp {^FAIL} $line] != 0 } {
			set failed 1
		}
	}
	close $o
	set o [open ALL.OUT a]
	if { $failed == 0 } {
		puts "Regression Tests Succeeded"
		puts $o "Regression Tests Succeeded"
	} else {
		puts "Regression Tests Failed; see ALL.OUT for log"
		puts $o "Regression Tests Failed"
	}

	puts -nonewline "Test suite run completed at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	puts -nonewline $o "Test suite run completed at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	close $o
}

#
# Run method tests in one environment.  (As opposed to run_envmethod
# which runs each test in its own, new environment.)
#
proc run_envmethod1 { method {display 0} {run 1} { outfile stdout } args } {
	global __debug_on
	global __debug_print
	global __debug_test
	global is_envmethod
	global test_names
	global parms
	source ./include.tcl

	if { $run == 1 } {
		puts "run_envmethod1: $method $args"
	}

	set is_envmethod 1
	if { $run == 1 } {
		check_handles
		env_cleanup $testdir
		error_check_good envremove [berkdb envremove -home $testdir] 0
		set env [eval {berkdb_env -create -cachesize {0 10000000 0}} \
 		    {-pagesize 512 -mode 0644 -home $testdir} $args ]
		error_check_good env_open [is_valid_env $env] TRUE
		append largs " -env $env "
	}

	if { $display } {
		# The envmethod1 tests can't be split up, since they share
		# an env.
		puts $outfile "eval run_envmethod1 $method $args"
	}

	set stat [catch {
		foreach test $test_names(test) {
			if { [info exists parms($test)] != 1 } {
				puts stderr "$test disabled in\
				    testparams.tcl; skipping."
				continue
			}
			if { $run } {
				puts $outfile "[timestamp]"
				eval $test $method $parms($test) $largs
				if { $__debug_print != 0 } {
					puts $outfile ""
				}
				if { $__debug_on != 0 } {
					debug $__debug_test
				}
			}
			flush stdout
			flush stderr
		}
	} res]
	if { $stat != 0} {
		global errorInfo;

		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_envmethod: $method $test: $theError"
		} else {
			error $theError;
		}
	}
	set stat [catch {
		foreach test $test_names(test) {
			if { [info exists parms($test)] != 1 } {
				puts stderr "$test disabled in\
				    testparams.tcl; skipping."
				continue
			}
			if { $run } {
				puts $outfile "[timestamp]"
				eval $test $method $parms($test) $largs
				if { $__debug_print != 0 } {
					puts $outfile ""
				}
				if { $__debug_on != 0 } {
					debug $__debug_test
				}
			}
			flush stdout
			flush stderr
		}
	} res]
	if { $stat != 0} {
		global errorInfo;

		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_envmethod1: $method $test: $theError"
		} else {
			error $theError;
		}
	}
	if { $run == 1 } {
		error_check_good envclose [$env close] 0
		check_handles $outfile
	}
	set is_envmethod 0

}

# Run the secondary index tests.
proc sindex { {display 0} {run 1} {outfile stdout} {verbose 0} args } {
	global test_names
	global testdir
	global verbose_check_secondaries
	set verbose_check_secondaries $verbose
	# Standard number of secondary indices to create if a single-element
	# list of methods is passed into the secondary index tests.
	global nsecondaries
	set nsecondaries 2

	# Run basic tests with a single secondary index and a small number
	# of keys, then again with a larger number of keys.  (Note that
	# we can't go above 5000, since we use two items from our
	# 10K-word list for each key/data pair.)
	foreach n { 200 5000 } {
		foreach pm { btree hash recno frecno queue queueext } {
			foreach sm { dbtree dhash ddbtree ddhash btree hash } {
				foreach test $test_names(si) {
					if { $display } {
						puts -nonewline $outfile \
						    "eval $test {\[list\
						    $pm $sm $sm\]} $n ;"
						puts -nonewline $outfile \
						    " verify_dir \
						    $testdir \"\" 1; "
						puts $outfile " salvage_dir \
						    $testdir 1"
					}
					if { $run } {
			 			check_handles $outfile
						eval $test \
						    {[list $pm $sm $sm]} $n
						verify_dir $testdir "" 1
						salvage_dir $testdir 1
					}
				}
			}
		}
	}

	# Run tests with 20 secondaries.
	foreach pm { btree hash } {
		set methlist [list $pm]
		for { set j 1 } { $j <= 20 } {incr j} {
			# XXX this should incorporate hash after #3726
			if { $j % 2 == 0 } {
				lappend methlist "dbtree"
			} else {
				lappend methlist "ddbtree"
			}
		}
		foreach test $test_names(si) {
			if { $display } {
				puts "eval $test {\[list $methlist\]} 500"
			}
			if { $run } {
				eval $test {$methlist} 500
			}
		}
	}
}

# Run secondary index join test.  (There's no point in running
# this with both lengths, the primary is unhappy for now with fixed-
# length records (XXX), and we need unsorted dups in the secondaries.)
proc sijoin { {display 0} {run 1} {outfile stdout} } {
	foreach pm { btree hash recno } {
		if { $display } {
			foreach sm { btree hash } {
				puts $outfile "eval sijointest\
				    {\[list $pm $sm $sm\]} 1000"
			}
			puts $outfile "eval sijointest\
			    {\[list $pm btree hash\]} 1000"
			puts $outfile "eval sijointest\
			    {\[list $pm hash btree\]} 1000"
		}
		if { $run } {
			foreach sm { btree hash } {
				eval sijointest {[list $pm $sm $sm]} 1000
			}
			eval sijointest {[list $pm btree hash]} 1000
			eval sijointest {[list $pm hash btree]} 1000
		}
	}
}

proc run { proc_suffix method {start 1} {stop 999} } {
	global test_names

	switch -exact -- $proc_suffix {
		envmethod -
		method -
		recd -
		repmethod -
		reptest -
		secenv -
		secmethod {
			# Run_recd runs the recd tests, all others
			# run the "testxxx" tests.
			if { $proc_suffix == "recd" } {
				set testtype recd
			} else {
				set testtype test
			}

			for { set i $start } { $i <= $stop } { incr i } {
				set name [format "%s%03d" $testtype $i]
				# If a test number is missing, silently skip
				# to next test; sparse numbering is allowed.
				if { [lsearch -exact $test_names($testtype) \
				    $name] == -1 } {
					continue
				}
				run_$proc_suffix $method $name
			}
		}
		default {
			puts "$proc_suffix is not set up with to be used with run"
		}
	}
}


# We want to test all of 512b, 8Kb, and 64Kb pages, but chances are one
# of these is the default pagesize.  We don't want to run all the AM tests
# twice, so figure out what the default page size is, then return the
# other two.
proc get_test_pagesizes { } {
	# Create an in-memory database.
	set db [berkdb_open -create -btree]
	error_check_good gtp_create [is_valid_db $db] TRUE
	set statret [$db stat]
	set pgsz 0
	foreach pair $statret {
		set fld [lindex $pair 0]
		if { [string compare $fld {Page size}] == 0 } {
			set pgsz [lindex $pair 1]
		}
	}

	error_check_good gtp_close [$db close] 0

	error_check_bad gtp_pgsz $pgsz 0
	switch $pgsz {
		512 { return {8192 65536} }
		8192 { return {512 65536} }
		65536 { return {512 8192} }
		default { return {512 8192 65536} }
	}
	error_check_good NOTREACHED 0 1
}

proc run_timed_once { timedtest args } {
	set start [timestamp -r]
	set ret [catch {
		eval $timedtest $args
		flush stdout
		flush stderr
	} res]
	set stop [timestamp -r]
	if { $ret != 0 } {
		global errorInfo

		set fnl [string first "\n" $errorInfo]
		set theError [string range $errorInfo 0 [expr $fnl - 1]]
		if {[string first FAIL $errorInfo] == -1} {
			error "FAIL:[timestamp]\
			    run_timed: $timedtest: $theError"
		} else {
			error $theError;
		}
	}
	return [expr $stop - $start]
}

proc run_timed { niter timedtest args } {
	if { $niter < 1 } {
		error "run_timed: Invalid number of iterations $niter"
	}
	set sum 0
	set e {}
	for { set i 1 } { $i <= $niter } { incr i } {
		set elapsed [eval run_timed_once $timedtest $args]
		lappend e $elapsed
		set sum [expr $sum + $elapsed]
		puts "Test $timedtest run $i completed: $elapsed seconds"
	}
	if { $niter > 1 } {
		set avg [expr $sum / $niter]
		puts "Average $timedtest time: $avg"
		puts "Raw $timedtest data: $e"
	}
}
