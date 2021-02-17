# See the file LICENSE for redistribution information.
#
# Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# $Id$
#
# TEST	fop005
# TEST	Test of DB->remove()
# TEST	Formerly test080.
# TEST	Test use of dbremove with and without envs, with absolute
# TEST	and relative paths, and with subdirectories.

proc fop005 { method args } {
	source ./include.tcl

	set tnum "005"
	set args [convert_args $method $args]
	set omethod [convert_method $method]
	set skipblob 0

	puts "Fop$tnum: ($method $args): Test of DB->remove()"

	# Determine full path
	set curdir [pwd]
	cd $testdir
	set fulldir [pwd]
	cd $curdir
	set reldir $testdir

	# If we are using an env, then skip this test.
	# It needs its own.
	set eindex [lsearch -exact $args "-env"]
	if { $eindex != -1 } {
		incr eindex
		set env [lindex $args $eindex]
		puts "Skipping fop$tnum for env $env"
		return
	}

	# Look for incompatible configurations of blob.
	foreach conf { "-encryptaes" "-encrypt" "-compress" "-dup" "-dupsort" \
	    "-read_uncommitted" "-multiversion" } {
		if { [lsearch -exact $args $conf] != -1 } {
			set skipblob 1
			set skipmsg "Fop005 skipping $conf for blob"
			break
		}
	}
	if { [is_btree $omethod] != 1 && \
	    [is_hash $omethod] != 1 && [is_heap $omethod] != 1 } {
		set skipblob 1
		set skipmsg "Fop005 skipping $omethod for blob"
	}
	cleanup $testdir NULL

	# Set up absolute and relative pathnames, and a subdirectory.
	set subdira A
	set filename fop$tnum.db
	set extentname __dbq.$filename.0
	set paths [list $fulldir $reldir]
	set files [list "$filename $extentname"\
	    "$subdira/$filename $subdira/$extentname"]
	set blobdir $testdir/__db_bl

	foreach format { "normal" "blob" } {
		if { $format == "blob" } {
			append args " -blob_threshold 1 "
			if { $skipblob != 0 } {
				puts $skipmsg
				continue
			}
		}
		foreach path $paths {
			foreach fileset $files {
				set filename [lindex $fileset 0]
				set extentname [lindex $fileset 1]

				# Loop through test using the following
				# options:
				# 1. no environment, not in transaction
				# 2. with environment, not in transaction
				# 3. remove with auto-commit
				# 4. remove in committed transaction
				# 5. remove in aborted transaction

				foreach op "noenv env auto commit abort" {
					file mkdir $testdir/$subdira
					if { $op == "noenv" } {
						set file $path/$filename
						set extentfile \
						    $path/$extentname
						set env NULL
						set envargs ""
						set dbargs "$args \
						    -blob_dir $blobdir"
					} else {
						set file $filename
						set extentfile $extentname
						set largs " -txn"
						if { $op == "env" } {
							set largs ""
						}
						set env [eval \
						    {berkdb_env -create \
						    -home $path} $largs]
						set envargs " -env $env "
						error_check_good env_open \
						    [is_valid_env $env] TRUE
						set dbargs $args
					}

					puts -nonewline "\tFop$tnum: dbremove\
					     with $op in path $path"
					if { $format == "blob" } {
						puts " with blobs enabled."
					} else {
						puts "."
					}
					puts "\t\tFop$tnum.a.1:\
					     Create file $file"
					set db [eval \
					    {berkdb_open -create -mode 0644} \
					    $omethod $envargs $dbargs {$file}]
					error_check_good db_open \
					    [is_valid_db $db] TRUE
					set blobsubdir [$db get_blob_sub_dir]

					# Use a numeric key so record-based
					# methods don't need special treatment.
					set key 1
					set data [pad_data $method data]

					error_check_good dbput \
					    [$db put $key \
					    [chop_data $method $data]] 0
					error_check_good dbclose [$db close] 0
					check_file_exist $file $env $path 1
					if { $format == "blob" } {
						check_blob_exists \
						    $blobdir $blobsubdir 1
					}
					if { [is_queueext $method] == 1 } {
						check_file_exist \
						    $extentfile $env $path 1
					}

					# Use berkdb dbremove for non-txn tests
					# and $env dbremove for transactional
					# tests
					puts "\t\tFop$tnum.a.2: Remove file"
					if { $op == "noenv" || $op == "env" } {
						error_check_good remove_$op \
						    [eval {berkdb dbremove} \
						    $envargs -blob_dir \
						    $blobdir $file] 0
					} elseif { $op == "auto" } {
						error_check_good remove_$op \
						    [eval {$env dbremove} \
						    -auto_commit $file] 0
					} else {
						# $op is "abort" or "commit"
						set txn [$env txn]
						error_check_good remove_$op \
						    [eval {$env dbremove} \
						    -txn $txn $file] 0
						error_check_good txn_$op \
						    [$txn $op] 0
					}

					puts "\t\tFop$tnum.a.3: Check that\
					    file is gone"
					# File should now be gone, unless the
					# op is an abort.  Check extent files
					# if necessary.
					if { $op != "abort" } {
						check_file_exist \
						    $file $env $path 0
						if { [is_queueext $method] \
						    == 1 } {
							check_file_exist \
							    $extentfile \
							    $env $path 0
						}
						if { $format == "blob" } {
							check_blob_exists \
							    $blobdir \
							    $blobsubdir 0
						}
					} else {
						check_file_exist \
						    $file $env $path 1
						if { [is_queueext $method] \
						    == 1 } {
							check_file_exist \
							    $extentfile \
							    $env $path 1
						}
						if { $format == "blob" } {
							check_blob_exists \
							    $blobdir \
							    $blobsubdir 1
						}
					}
					# Check that the blob subdirectory is
					# removed when no txn is used.
					if { $op == "noenv" || $op == "env" } {
						if { $format == "blob" } {
							check_blob_sub_exists \
							    $blobdir \
							    $blobsubdir 0
						}
					}
					if { $env != "NULL" } {
						error_check_good envclose \
						    [$env close] 0
					}
					env_cleanup $path
					check_file_exist $file $env $path 0
					if { $format == "blob" } {
						check_blob_exists $blobdir \
						    $blobsubdir 0
					}
				}
			}
		}
	}
}
