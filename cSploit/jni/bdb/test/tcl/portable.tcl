# See the file LICENSE for redistribution information.
#
# Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$

source ./include.tcl

global gen_portable
set gen_portable 0

global portable_dir
global portable_be
global portable_method
global portable_name

proc test_portable_logs { { archived_test_loc } } {
	source ./include.tcl
	global test_names
	global portable_dir
	global tcl_platform
	global saved_logvers

	if { [string match /* $archived_test_loc] != 1 } {
		puts "Specify an absolute path for the archived files."
		return
	}

	# Identify endianness of the machine we are testing on.
	if { [big_endian] } {
		set myendianness be
	} else {
		set myendianness le
	}

	if { [file exists $archived_test_loc/logversion] == 1 } {
		set fd [open $archived_test_loc/logversion r]
		set saved_logvers [read $fd]
		close $fd
	} else {
		puts "Old log version number must be available \
		    in $archived_test_loc/logversion"
		return
	}

	fileremove -f PORTABLE.OUT
	set o [open PORTABLE.OUT a]

	puts -nonewline $o "Log portability test started at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	puts $o [berkdb version -string]

	puts -nonewline "Log portability test started at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	puts [berkdb version -string]

	set portable_dir $archived_test_loc
	puts $o "Using archived databases in $portable_dir."
	puts "Using archived databases in $portable_dir."
	close $o

foreach version [glob $portable_dir/*] {
	regexp \[^\/\]*$ $version version
	if { [string equal $version "logversion"] == 1 } { continue }

	# Test only files where the endianness of the db does
	# not match the endianness of the test platform. 
	#
	set dbendianness [string range $version end-1 end]
	if { [string equal $myendianness $dbendianness] } {
		puts "Skipping test of $version \
		    on $myendianness platform."
	} else {
		set o [open PORTABLE.OUT a]
		puts $o "Testing $dbendianness files\
		    on $myendianness platform."
		close $o
		puts "Testing $dbendianness files\
		    on $myendianness platform."

		foreach method [glob -nocomplain $portable_dir/$version/*] {
			regexp \[^\/\]*$ $method method
			set o [open PORTABLE.OUT a]
			puts $o "\nTesting $method files"
			close $o
			puts "\tTesting $method files"

			foreach file [lsort -dictionary \
			    [glob -nocomplain \
			    $portable_dir/$version/$method/*]] {
				regexp (\[^\/\]*)\.tar\.gz$ \
				    $file dummy name

				cleanup $testdir NULL 1
				set curdir [pwd]
				cd $testdir
				set tarfd [open "|tar xf -" w]
				cd $curdir 

				catch {exec gunzip -c \
	    "$portable_dir/$version/$method/$name.tar.gz" >@$tarfd}
				close $tarfd

				set f [open $testdir/$name.tcldump \
				    {RDWR CREAT}]
				close $f

				# We exec a separate tclsh for each
				# separate subtest to keep the
				# testing process from consuming a
				# tremendous amount of memory.
				#
				# Then recover the db.
				if { [file exists \
				    $testdir/$name.db] } {
#puts "found file $testdir/$name.db"
					if { [catch {exec $tclsh_path \
					    << "source \
					    $test_path/test.tcl;\
					    _recover_test $testdir \
					    $version $method $name \
					    $dbendianness" >>& \
					    PORTABLE.OUT } message] } {
						set o [open \
						    PORTABLE.OUT a]
						puts $o "FAIL: $message"
						close $o
					}
				}
			}
		}
	}
}

	set o [open PORTABLE.OUT a]
	puts -nonewline $o "Completed at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	close $o

	puts -nonewline "Completed at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]

	# Don't provide a return value.
	return
}

proc _recover_test { dir version method name dbendianness } {
	source ./include.tcl
	global errorInfo

	puts "Recover db using opposite endian log: \
	    $version $method $name.db"

	set omethod [convert_method $method]

	# Move the saved database; we'll need to compare it to 
	# the recovered database.
	catch { file rename -force $dir/$name.db \
	    $dir/$name.db.init } res
	if { [is_heap $method] == 1 } { 
		file rename -force $dir/$name.db1 \
		    $dir/$name.db.init1
		file rename -force $dir/$name.db2 \
		    $dir/$name.db.init2
	}
	if { [file exists $dir/__db_bl] } {
		file copy -force $dir/__db_bl $dir/__db_init
	}

	# Recover.
        set ret [catch {eval {exec} $util_path/db_recover -h $dir} res]
        if { $ret != 0 } {
                puts "FAIL: db_recover outputted $res"
        }
        error_check_good db_recover $ret 0

	# Compare the original database to the recovered database.
	set dbinit\
 	    [berkdb_open -blob_dir $dir/__db_init $omethod $dir/$name.db.init]
	set db\
	    [berkdb_open -blob_dir $dir/__db_bl $omethod $dir/$name.db]
	db_compare $dbinit $db $dir/$name.db.init \
	    $dir/$name.db

	# Verify.
	error_check_good db_verify [verify_dir $dir "" 0 0 1] 0

}

proc generate_portable_logs { destination_dir } {
	global gen_portable
	global gen_dump
	global portable_dir
	global portable_be
	global portable_method
	global portable_name
	global valid_methods
	global test_names
	global parms
	source ./include.tcl

	if { [string match /* $destination_dir] != 1 } {
		puts "Specify an absolute path for the archived files."
		return
	}

	set portable_dir $destination_dir
	env_cleanup $testdir

	fileremove -f GENERATE.OUT
	set o [open GENERATE.OUT a]

	puts -nonewline $o "Generating files for portability test.  Started at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	puts $o [berkdb version -string]

	puts -nonewline "Generating files for portability test.  Started at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	puts [berkdb version -string]

	close $o

	# Create a file that contains the log version number.
	# If necessary, create the directory to contain the file.
	if { [file exists $destination_dir] == 0 } {
		file mkdir $destination_dir 
	} else {
		puts "$destination_dir already exists, exiting."
		return
	}

	set env [berkdb_env -create -log -home $testdir]
	error_check_good is_valid_env [is_valid_env $env] TRUE

	set lv [open $destination_dir/logversion w]
	puts $lv [get_log_vers $env]
	close $lv

	error_check_good env_close [$env close] 0

	# Generate test databases for each access method and endianness.
	set gen_portable 1
	foreach method $valid_methods {
		set o [open GENERATE.OUT a]
		puts $o "\nGenerating $method files"
		close $o
		puts "\tGenerating $method files"
		set portable_method $method

# Select a variety of tests.  
#set test_names(test) "test002 test011 test013 test017 \
#    test021 test024 test027 test028"
set test_names(test) "test008"
		foreach test $test_names(test) {
			if { [info exists parms($test)] != 1 } {
				continue
			}

			set o [open GENERATE.OUT a]
			puts $o "\t\tGenerating files for $test"
			close $o
			puts "\t\tGenerating files for $test"

			foreach portable_be { 0 1 } {
				set portable_name $test
				if [catch {exec $tclsh_path \
				    << "source $test_path/test.tcl;\
				    global gen_portable portable_be;\
				    global portable_method portable_name;\
				    global portable_dir;\
				    set gen_portable 1;\
				    set portable_be $portable_be;\
				    set portable_method $portable_method;\
				    set portable_name $portable_name;\
				    set portable_dir $portable_dir;\
				    run_envmethod -$method $test" \
				    >>& GENERATE.OUT} res] {
					puts "FAIL: run_envmethod \
					    $test $method"
				}
				cleanup $testdir NULL 1
			}
		}
	}

	set gen_portable 0
	set o [open GENERATE.OUT a]
	puts -nonewline $o "Completed at: "
	puts $o [clock format [clock seconds] -format "%H:%M %D"]
	puts -nonewline "Completed at: "
	puts [clock format [clock seconds] -format "%H:%M %D"]
	close $o
}

proc save_portable_files { dir } {
	global portable_dir
	global portable_be
	global portable_method
	global portable_name
	global gen_portable
	global gen_dump
	source ./include.tcl

	set vers [berkdb version]
	set maj [lindex $vers 0]
	set min [lindex $vers 1]

	if { [big_endian] } {
		set myendianness be
	} else {
		set myendianness le
	}

	if { $portable_be == 1 } {
		set version_dir "$myendianness-$maj.${min}be"
		set en be
	} else {
		set version_dir "$myendianness-$maj.${min}le"
		set en le
	}

	set dest $portable_dir/$version_dir/$portable_method
	if { [file exists $portable_dir/$version_dir/$portable_method] == 0 } {
		file mkdir $dest
	}

	if { $gen_portable == 1 } {
		# Some tests skip some access methods, so we 
		# only try to save files if there is a datafile
		# file.  
		set dbfiles [glob -nocomplain $dir/*.db]
		if { [llength $dbfiles] > 0 } {
			set logfiles [glob -nocomplain $dir/log.*]
			set dbfile [lindex $dbfiles 0]

			if { $portable_method == "heap" } { 
				append dbfile1 $dbfile "1"
				append dbfile2 $dbfile "2"
			}

			# We arbitrarily name the tar file where we save
			# everything after the first database file we 
			# find.  This works because the database files
			# are almost always named after the test.
			set basename [string range $dbfile \
				    [expr [string length $dir] + 1] end-3]
	
			set cwd [pwd]
			cd $dest
			set dest [pwd]
			cd $cwd
			cd $dir

			if { [catch {
				eval exec tar -cf $dest/$basename.tar \
				    [glob -nocomplain *.db *.db1 *.db2 \
				    __db_bl log.* __dbq.$basename-$en.db.*]
				exec gzip --fast -r $dest/$basename.tar
			} res ] } {
				puts "FAIL: tar/gzip of $basename failed\
				    with message $res"
			}
			cd $cwd
		}
	}
}


