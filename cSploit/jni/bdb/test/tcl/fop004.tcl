# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	fop004
# TEST	Test of DB->rename(). (formerly test075)
# TEST	Test that files can be renamed from one directory to another.
# TEST	Test that files can be renamed using absolute or relative
# TEST	pathnames.
# TEST	Test that renaming a database does not change the location or
# TEST	name of its blob files.
proc fop004 { method { tnum "004" } args } {
	global encrypt
	global errorCode
	global errorInfo
	source ./include.tcl

	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set skipblob 0

	puts "Fop$tnum: ($method $args): Test of DB->rename()"

	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		# If we are using an env, then skip this test.
		# It needs its own.
		incr eindex
		set env [lindex $args $eindex]
		puts "Skipping fop$tnum for env $env"
		return
	}
	if { $encrypt != 0 } {
		puts "Skipping fop$tnum for security"
		return
	}
	cleanup $testdir NULL

	# Look for incompatible configurations of blob.
	foreach conf { "-encryptaes" "-encrypt" "-compress" "-dup" "-dupsort" \
	    "-read_uncommitted" "-multiversion" } {
		if { [lsearch -exact $args $conf] != -1 } {
			set skipblob 1
			set skipmsg "Fop004 skipping $conf for blob"
			break
		}
	}
	if { [is_btree $omethod] != 1 && \
	    [is_hash $omethod] != 1 && [is_heap $omethod] != 1 } {
		set skipblob 1
		set skipmsg "Fop004 skipping $omethod for blob"
	}

	# Define absolute pathnames
	set curdir [pwd]
	cd $testdir
	set fulldir [pwd]
	cd $curdir
	set reldir $testdir
	set blobdir $testdir/__db_bl

	# Name subdirectories for renaming from one directory to another.
	set subdira A
	set subdirb B

	# Set up absolute and relative pathnames for test
	set paths [list "absolute $fulldir" "relative $reldir"]
	set files [list "fop$tnum-old.db fop$tnum-new.db {name change}" \
	    "fop$tnum.db fop$tnum.db {directory change}"]

	foreach format { "normal" "blob" } {
		if { $format == "blob" } {
			if { $skipblob } {
				puts $skipmsg
				continue
			}
			append args " -blob_threshold 1 "
		}
		foreach pathinfo $paths {
			set pathtype [lindex $pathinfo 0]
			set path [lindex $pathinfo 1]
			foreach fileinfo $files {
				set desc [lindex $fileinfo 2]
				puts -nonewline "Fop$tnum: Test of $pathtype"
				puts -nonewline " path $path with $desc"
				if { $format == "blob" } {
					puts " with blobs enabled."
				} else {
					puts "."
				}
				set env NULL
				set envargs ""

				# Loop through test using the following rename
				# options
				# 1. no environment, not in transaction
				# 2. with environment, not in transaction
				# 3. rename with auto-commit
				# 4. rename in committed transaction
				# 5. rename in aborted transaction

				foreach op "noenv env auto commit abort" {

					puts "\tFop$tnum.a: Create/rename \
					    with $op"
					# If we are using an env, then testfile
					# should be the db name.  Otherwise it
					# is the path we are testing and the
					# name.
					#
					set old [lindex $fileinfo 0]
					set new [lindex $fileinfo 1]
					# Set up subdirectories if necessary.
					if { $desc == "directory change" } {
						file mkdir $testdir/$subdira
						file mkdir $testdir/$subdirb
						set oldname $subdira/$old
						set newname $subdirb/$new
						set oldextent \
						    $subdira/__dbq.$old.0
						set newextent \
						    $subdirb/__dbq.$new.0
					} else {
						set oldname $old
						set newname $new
						set oldextent __dbq.$old.0
						set newextent __dbq.$new.0
					}
					# If we don't have an env, we're
					# going to operate on the file using
					# its absolute or relative path.  Tack
					# it on the front.
					if { $op == "noenv" } {
						set oldfile $path/$oldname
						set newfile $path/$newname
						set oldextent $path/$oldextent
						set newextent $path/$newextent
						set dbargs \
						    "$args -blob_dir $blobdir"
					} else {
						set dbargs $args
						set oldfile $oldname
						set newfile $newname
						set txnarg ""
						if { $op == "auto" \
						    || $op == "commit" \
						    || $op == "abort" } {
							set txnarg " -txn"
						}
						set env [eval \
						    {berkdb_env -create} \
						    $txnarg -home $path]
						set envargs "-env $env"
						error_check_good env_open \
						    [is_valid_env $env] TRUE
					}

					# Files don't exist before starting
					# the test.
					#
					check_file_exist $oldfile $env $path 0
					check_file_exist $newfile $env $path 0

					puts "\t\tFop$tnum.a.1: Create file \
					    $oldfile"
					set db [eval \
					    {berkdb_open -create -mode 0644} \
					    $omethod $envargs $dbargs $oldfile]
					error_check_good \
					    dbopen [is_valid_db $db] TRUE
					set blobsubdir [$db get_blob_sub_dir]

					# Use numeric key so record-based
					# methods don't need special treatment.
					set key 1
					set data data

					error_check_good dbput [$db put $key \
					    [chop_data $method $data]] 0
					error_check_good dbclose [$db close] 0

					puts "\t\tFop$tnum.a.2:\
					    Rename file to $newfile"
					check_file_exist $oldfile $env $path 1
					check_file_exist $newfile $env $path 0
					if { $format == "blob" } {
						check_blob_exists \
						    $blobdir $blobsubdir 1
					}

					# Regular renames use berkdb dbrename
					# Txn-protected renames use $env
					# dbrename.
					if { $op == "noenv" || $op == "env" } {
						error_check_good rename [eval \
						    {berkdb dbrename} \
						    $envargs $oldfile \
						    $newfile] 0
					} elseif { $op == "auto" } {
						error_check_good rename [eval \
						    {$env dbrename} \
						    -auto_commit \
						    $oldfile $newfile] 0
					} else {
						# $op is "abort" or "commit"
						set txn [$env txn]
						error_check_good rename [eval \
						    {$env dbrename} -txn $txn \
						    $oldfile $newfile] 0
						error_check_good \
						    txn_$op [$txn $op] 0
					}

					if { $op != "abort" } {
						check_file_exist \
						    $oldfile $env $path 0
						check_file_exist \
						    $newfile $env $path 1
					} else {
						check_file_exist \
						    $oldfile $env $path 1
						check_file_exist \
						    $newfile $env $path 0
					}
					# Renaming has no effect on blobs.
					if { $format == "blob" } {
						check_blob_exists \
						    $blobdir $blobsubdir 1
					}

					# Check that extent files moved too,
					# unless we aborted the rename.
					if { [is_queueext $method ] == 1 } {
						if { $op != "abort" } {
							check_file_exist \
							    $oldextent $env \
							    $path 0
							check_file_exist \
							    $newextent $env \
							    $path 1
						} else {
							check_file_exist \
							    $oldextent $env \
							    $path 1
							check_file_exist \
							    $newextent $env \
							    $path 0
						}
					}

					puts "\t\tFop$tnum.a.3: \
					    Check file contents"
					# Open again with create to make sure
					# we're not caching.  In the normal
					# case (no env), we already know the
					# file doesn't exist.
					set odb [eval \
					    {berkdb_open -create -mode 0644} \
					    $envargs $omethod $dbargs $oldfile]
					set oldblobsubdir \
					    [$odb get_blob_sub_dir]
					set ndb [eval \
					    {berkdb_open -create -mode 0644} \
					    $envargs $omethod $dbargs $newfile]
					set newblobsubdir \
					    [$ndb get_blob_sub_dir]
					error_check_good \
					    odb_open [is_valid_db $odb] TRUE
					error_check_good \
					    ndb_open [is_valid_db $ndb] TRUE

					# The DBT from the "old" database
					# should be empty, not the "new" one,
					# except in the case of an abort.
					set odbt [$odb get $key]
					if { $op == "abort" } {
						error_check_good \
						    odbt_has_data \
						    [llength $odbt] 1
						if { $format == "blob" } {
							check_blob_exists \
							    $blobdir \
							    $oldblobsubdir 1
						}
					} else {
						set ndbt [$ndb get $key]
						error_check_good odbt_empty \
						    [llength $odbt] 0
						error_check_bad ndbt_empty \
						    [llength $ndbt] 0
						error_check_good ndbt [lindex \
						[lindex $ndbt 0] 1] \
						    [pad_data $method $data]
						# Confirm that the old database
						# has no blobs, but the new
						# database does.
						if { $format == "blob" } {
							check_blob_exists \
							    $blobdir \
							    $oldblobsubdir 0
							check_blob_exists \
							    $blobdir \
							    $newblobsubdir 1
						}
					}
					error_check_good \
					    odb_close [$odb close] 0
					error_check_good \
					    ndb_close [$ndb close] 0

					# Now there's both an old and a new.
					#  Rename the "new" to the "old" and
					# make sure that fails.
					#
					puts "\tFop$tnum.b: Make sure rename\
					     fails instead of overwriting"
					set envargs ""
					if { $env != "NULL" } {
						error_check_good \
						    env_close [$env close] 0
						set env [berkdb_env_noerr \
						    -home $path]
						set envargs " -env $env"
						error_check_good env_open2 \
						    [is_valid_env $env] TRUE
					}
					set ret [catch \
					    {eval {berkdb dbrename} \
					    $envargs $newfile $oldfile} res]
					error_check_bad rename_overwrite $ret 0
					error_check_good rename_overwrite_ret \
					    [is_substr $errorCode EEXIST] 1

					# Verify and then start over from a
					# clean slate.
					verify_dir $path \
					    "\tFop$tnum.c: " 0 0 0 0 1 $blobdir
					verify_dir $path/$subdira \
					    "\tFop$tnum.c: " 0 0 0 0 1 $blobdir
					verify_dir $path/$subdirb \
					    "\tFop$tnum.c: " 0 0 0 0 1 $blobdir
					if { $env != "NULL" } {
						error_check_good \
						    env_close2 [$env close] 0
					}
					env_cleanup $path
					check_file_exist $oldfile $env $path 0
					check_file_exist $newfile $env $path 0
					# Check that the blobs are deleted
					if { $format == "blob" } {
						check_blob_exists \
						    $blobdir $oldblobsubdir 0
						check_blob_exists \
						    $blobdir $newblobsubdir 0
					}
				}
			}
		}
	}
}

proc check_file_exist { filename env path expected } {
	if { $env != "NULL" } {
		error_check_good "$filename exists in env" \
		    [file exists $path/$filename] $expected
	} else {
		error_check_good \
		    "$filename exists" [file exists $filename] $expected
	}
}

proc check_blob_exists { blobdir blobsubdir expected } {
	set blob_file $blobdir/$blobsubdir/__db.bl001
	error_check_good "blob exists" [file exists $blob_file] $expected
}
