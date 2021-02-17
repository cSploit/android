set bulk_insert INSERT INTO MESSAGES (SYMBOL, ROUTINE, MODULE, TRANS_NOTES, FAC_CODE, NUMBER, FLAGS, TEXT, "ACTION", EXPLANATION) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
-- JRD
(NULL, NULL, 'jrd.c', NULL, 0, 0, NULL, '', NULL, NULL);
('arith_except', NULL, NULL, NULL, 0, 1, NULL, 'arithmetic exception, numeric overflow, or string truncation', NULL, NULL);
('bad_dbkey', NULL, NULL, NULL, 0, 2, NULL, 'invalid database key', NULL, NULL);
('bad_db_format', NULL, NULL, NULL, 0, 3, NULL, 'file @1 is not a valid database', NULL, NULL);
('bad_db_handle', NULL, NULL, NULL, 0, 4, NULL, 'invalid database handle (no active connection)', NULL, NULL);
('bad_dpb_content', NULL, NULL, NULL, 0, 5, NULL, 'bad parameters on attach or create database', NULL, NULL);
('bad_dpb_form', NULL, NULL, NULL, 0, 6, NULL, 'unrecognized database parameter block', NULL, NULL);
('bad_req_handle', NULL, NULL, NULL, 0, 7, NULL, 'invalid request handle', NULL, NULL);
('bad_segstr_handle', NULL, NULL, NULL, 0, 8, NULL, 'invalid BLOB handle', NULL, NULL);
('bad_segstr_id', NULL, NULL, NULL, 0, 9, NULL, 'invalid BLOB ID', NULL, NULL);
('bad_tpb_content', NULL, NULL, NULL, 0, 10, NULL, 'invalid parameter in transaction parameter block', NULL, NULL);
('bad_tpb_form', NULL, NULL, NULL, 0, 11, NULL, 'invalid format for transaction parameter block', NULL, NULL);
('bad_trans_handle', NULL, NULL, NULL, 0, 12, NULL, 'invalid transaction handle (expecting explicit transaction start)', NULL, NULL);
('bug_check', NULL, NULL, NULL, 0, 13, NULL, 'internal Firebird consistency check (@1)', NULL, NULL);
('convert_error', NULL, NULL, 'The "@1" in this message does not refer to the preceding word "string".
It calls up the message "Do you want to rollback your updates? "
To test the placement of the string run the following commands in QLI:
    ready msg.gdb
    for messages with text = 23 list
Text is a char field so this brings up the message:
    conversion error from string "Do you want to rollback your updates? "', 0, 14, NULL, 'conversion error from string "@1"', NULL, NULL);
('db_corrupt', NULL, NULL, NULL, 0, 15, NULL, 'database file appears corrupt (@1)', NULL, NULL);
('deadlock', NULL, NULL, NULL, 0, 16, NULL, 'deadlock', NULL, NULL);
('excess_trans', NULL, NULL, NULL, 0, 17, NULL, 'attempt to start more than @1 transactions', NULL, NULL);
('from_no_match', NULL, NULL, NULL, 0, 18, NULL, 'no match for first value expression', NULL, NULL);
('infinap', NULL, NULL, NULL, 0, 19, NULL, 'information type inappropriate for object specified', 'Check and correct the information items.', 'A call to one of the GDS information routines referenced
an item that does not exist.  For example, you asked for
the total length of a blob in a call to gds_$database_info.');
('infona', NULL, NULL, NULL, 0, 20, NULL, 'no information of this type available for object specified', NULL, NULL);
('infunk', NULL, NULL, NULL, 0, 21, NULL, 'unknown information item', NULL, NULL);
('integ_fail', NULL, NULL, NULL, 0, 22, NULL, 'action cancelled by trigger (@1) to preserve data integrity', NULL, NULL);
('invalid_blr', NULL, NULL, NULL, 0, 23, NULL, 'invalid request BLR at offset @1', NULL, NULL);
('io_error', NULL, NULL, NULL, 0, 24, NULL, 'I/O error during "@1" operation for file "@2"', +++
'Check secondary messages for more information.  The
problem may be an obvious one, such as incorrect file name or
a file protection problem.  If that does not eliminate the
problem, check your program logic.  To avoid errors when
the user enters a database name interactively,
add an error handler to the statement that causes this
message to appear.', 'Your program encountered an input or output error.');
('lock_conflict', NULL, NULL, NULL, 0, 25, NULL, 'lock conflict on no wait transaction', NULL, NULL);
('metadata_corrupt', NULL, NULL, NULL, 0, 26, NULL, 'corrupt system table', NULL, NULL);
('not_valid', NULL, NULL, NULL, 0, 27, NULL, 'validation error for column @1, value "@2"', NULL, NULL);
('no_cur_rec', NULL, NULL, NULL, 0, 28, NULL, 'no current record for fetch operation', NULL, NULL);
('no_dup', NULL, NULL, NULL, 0, 29, NULL, 'attempt to store duplicate value (visible to active transactions) in unique index "@1"', NULL, NULL);
('no_finish', NULL, NULL, NULL, 0, 30, NULL, 'program attempted to exit without finishing database', NULL, NULL);
('no_meta_update', NULL, NULL, NULL, 0, 31, NULL, 'unsuccessful metadata update', NULL, NULL);
('no_priv', NULL, NULL, NULL, 0, 32, NULL, 'no permission for @1 access to @2 @3', NULL, NULL);
('no_recon', NULL, NULL, NULL, 0, 33, NULL, 'transaction is not in limbo', NULL, NULL);
('no_record', NULL, NULL, NULL, 0, 34, NULL, 'invalid database key', NULL, NULL);
('no_segstr_close', NULL, NULL, NULL, 0, 35, NULL, 'BLOB was not closed', NULL, NULL);
('obsolete_metadata', NULL, NULL, NULL, 0, 36, NULL, 'metadata is obsolete', 'Check object names or identifiers to make sure that they
still exist in the database.  Correct your program if they
don''t.', 'Your program probably referenced an object that does not
exist in the database.');
('open_trans', NULL, NULL, NULL, 0, 37, NULL, 'cannot disconnect database with open transactions (@1 active)', 'Commit or roll back those transactions before you detach
the database.', 'Your program attempted to detach a database without
committing or rolling back one or more transactions.');
('port_len', NULL, NULL, NULL, 0, 38, NULL, 'message length error (encountered @1, expected @2)', 'Make sure that the blr_string_length parameter on the call
to gds_$compile_request matches the BLR string.  If you
receive this error while using GDML or SQL, please submit
a bug report.', 'The actual length of a buffer does not correspond to what
the request language says it should be.');
('read_only_field', NULL, NULL, NULL, 0, 39, NULL, 'attempted update of read-only column', 'If the read-only field is in a system relation, change your
program.  If the field is a COMPUTED field, you have to
change the source fields to change its value.  If the field
takes part in a view, update it in its source relations.', 'Your program tried to change the value of a read-only
field in a system relation, a COMPUTED field, or a field
used in a view.');
('read_only_rel', NULL, NULL, NULL, 0, 40, NULL, 'attempted update of read-only table', 'If you want to write to the relation, reserve it for WRITE.', 'Your program tried to update a relation that it earlier
reserved for READ access.');
('read_only_trans', NULL, NULL, NULL, 0, 41, NULL, 'attempted update during read-only transaction', 'If you want to update the database, use a READ_WRITE
transaction.', 'Your program tried to update during a READ_ONLY translation.');
('read_only_view', NULL, NULL, NULL, 0, 42, NULL, 'cannot update read-only view @1', 'Views that include a record select, join, or project cannot
be updated.  If you want to perform updates, you must do so
through the source relations.  If you are updating join terms,
make sure that you change them in all relations.  In any case,
 update the source relations in a single transaction so that
you make the changes consistently.', 'Your program tried to update a view that contains a
record select, join, or project operation.');
('req_no_trans', NULL, NULL, NULL, 0, 43, NULL, 'no transaction for request', 'Check and correct your program logic.  Commit or roll back
the transaction only after you have completed all operations
that you want in the transaction.', 'Your program tried to continue a request after the enveloping
transaction had been committed or rolled back.');
('req_sync', NULL, NULL, NULL, 0, 44, NULL, 'request synchronization error', 'For call interface programs, locate and correct the program
error.  If you received this error while using GDML or SQL,
please submit a bug report. ', 'Your program issued a send or receive for a message type
that did not match the logic of the BLR request.');
('req_wrong_db', NULL, NULL, NULL, 0, 45, NULL, 'request referenced an unavailable database', 'Change your program so that the required database is
within the scope of the transaction.', 'Your program referenced a relation from a database that is
not available within the current transaction.');
('segment', NULL, NULL, NULL, 0, 46, NULL, 'segment buffer length shorter than expected', 'Check the segment_buffer_length parameter on the blob calls
and make sure that it is long enough for handling the
segments of the blob field you are accessing.  Alternately,
you could trap for this error and accept truncated values.', 'The length of the segment_buffer on a blob call was shorter
than the segment returned by the database software.
Therefore, the database software could return only part of
the segment.');
('segstr_eof', NULL, NULL, NULL, 0, 47, NULL, 'attempted retrieval of more segments than exist', 'Change your program so that it tests for this condition
and stops retrieving segments when there aren''t any more.', 'Your program tried to retrieve more segments from a blob
field than were stored.');
('segstr_no_op', NULL, NULL, NULL, 0, 48, NULL, 'attempted invalid operation on a BLOB', 'Check your program to make sure that it does not reference
a blob field in a Boolean expression or in a statement
not intended for use with blobs.  Both GDML and the call
interface have statements or routines that perform
blob storage, retrieval, and update.', 'Your program tried to do something that cannot be done
with blob fields.');
('segstr_no_read', NULL, NULL, NULL, 0, 49, NULL, 'attempted read of a new, open BLOB', 'Check and correct your program logic.  Close the blob field
before you try to read from it.', 'Your program tried to read from a blob field that it is
creating.');
('segstr_no_trans', NULL, NULL, NULL, 0, 50, NULL, 'attempted action on BLOB outside transaction', 'Change your program so that you perform whatever data
manipulation is required in a transaction before you end
that transaction.', 'Your program reference a blob field after it committed or
rolled back the transaction that had been processing the
field.');
('segstr_no_write', NULL, NULL, NULL, 0, 51, NULL, 'attempted write to read-only BLOB', 'If you are using the call interface, open the blob for
by calling gds_$create_blob.  If you are using GDML, open
the blob with the create_blob statement.', 'Your program tried to write to a blob field that
that had been opened for read access.');
('segstr_wrong_db', NULL, NULL, NULL, 0, 52, NULL, 'attempted reference to BLOB in unavailable database', 'Change your program so that the required database is
available to the current transaction.', 'Your program referenced a blob field from a relation
in a database that is not available to the current
transaction.');
('sys_request', NULL, NULL, NULL, 0, 53, NULL, 'operating system directive @1 failed', 'Check secondary messages for more information.  When you
isolate the problem, you may want to include an error handler
to trap for this condition.', 'The operating system returned an error.');
('stream_eof', NULL, NULL, NULL, 0, 54, NULL, 'attempt to fetch past the last record in a record stream', NULL, NULL);
('unavailable', NULL, NULL, NULL, 0, 55, NULL, 'unavailable database', NULL, NULL);
('unres_rel', NULL, NULL, NULL, 0, 56, NULL, 'table @1 was omitted from the transaction reserving list', NULL, NULL);
('uns_ext', NULL, NULL, NULL, 0, 57, NULL, 'request includes a DSRI extension not supported in this implementation', NULL, NULL);
('wish_list', NULL, NULL, NULL, 0, 58, NULL, 'feature is not supported', NULL, NULL);
('wrong_ods', NULL, NULL, NULL, 0, 59, NULL, 'unsupported on-disk structure for file @1; found @2.@3, support @4.@5', NULL, NULL);
('wronumarg', NULL, NULL, NULL, 0, 60, NULL, 'wrong number of arguments on call', NULL, NULL);
('imp_exc', NULL, NULL, NULL, 0, 61, NULL, 'Implementation limit exceeded', NULL, NULL);
('random', NULL, NULL, NULL, 0, 62, NULL, '@1', NULL, NULL);
('fatal_conflict', NULL, NULL, NULL, 0, 63, NULL, 'unrecoverable conflict with limbo transaction @1', NULL, NULL);
('badblk', NULL, NULL, NULL, 0, 64, NULL, 'internal error', NULL, NULL);
('invpoolcl', NULL, NULL, NULL, 0, 65, NULL, 'internal error', NULL, NULL);
('nopoolids', NULL, NULL, NULL, 0, 66, NULL, 'too many requests', NULL, NULL);
('relbadblk', NULL, NULL, NULL, 0, 67, NULL, 'internal error', NULL, NULL);
('blktoobig', NULL, NULL, NULL, 0, 68, NULL, 'block size exceeds implementation restriction', NULL, NULL);
('bufexh', NULL, NULL, NULL, 0, 69, NULL, 'buffer exhausted', NULL, NULL);
('syntaxerr', NULL, NULL, NULL, 0, 70, NULL, 'BLR syntax error: expected @1 at offset @2, encountered @3', NULL, NULL);
('bufinuse', NULL, NULL, NULL, 0, 71, NULL, 'buffer in use', NULL, NULL);
('bdbincon', NULL, NULL, NULL, 0, 72, NULL, 'internal error', NULL, NULL);
('reqinuse', NULL, NULL, NULL, 0, 73, NULL, 'request in use', NULL, NULL);
('badodsver', NULL, NULL, NULL, 0, 74, NULL, 'incompatible version of on-disk structure', 'Look further in the status vector for more detail
and fix the error.', 'Some dynamic SQL error occurred - more to follow');
('relnotdef', NULL, NULL, NULL, 0, 75, NULL, 'table @1 is not defined', NULL, NULL);
('fldnotdef', NULL, NULL, NULL, 0, 76, NULL, 'column @1 is not defined in table @2', NULL, 'An undefined field was referenced in blr.');
('dirtypage', NULL, NULL, NULL, 0, 77, NULL, 'internal error', NULL, 'An external function was defined but was incompatible with either the
number or types of arguments given.');
('waifortra', NULL, NULL, NULL, 0, 78, NULL, 'internal error', NULL, NULL);
('doubleloc', NULL, NULL, NULL, 0, 79, NULL, 'internal error', NULL, NULL);
('nodnotfnd', NULL, NULL, NULL, 0, 80, NULL, 'internal error', NULL, NULL);
('dupnodfnd', NULL, NULL, NULL, 0, 81, NULL, 'internal error', NULL, NULL);
('locnotmar', NULL, NULL, NULL, 0, 82, NULL, 'internal error', NULL, NULL);
('badpagtyp', NULL, NULL, NULL, 0, 83, NULL, 'page @1 is of wrong type (expected @2, found @3)', NULL, NULL);
('corrupt', NULL, NULL, NULL, 0, 84, NULL, 'database corrupted', NULL, NULL);
('badpage', NULL, NULL, NULL, 0, 85, NULL, 'checksum error on database page @1', NULL, NULL);
('badindex', NULL, NULL, NULL, 0, 86, NULL, 'index is broken', NULL, NULL);
('dbbnotzer', NULL, NULL, NULL, 0, 87, NULL, 'database handle not zero', NULL, NULL);
('tranotzer', NULL, NULL, NULL, 0, 88, NULL, 'transaction handle not zero', 'Read and reform thine actions.?', 'Error code returned from a foriegn database
system.');
('trareqmis', NULL, NULL, NULL, 0, 89, NULL, 'transaction--request mismatch (synchronization error)', 'Commit or rollback your transaction and
retry your update.', 'Your transaction has attempted to update or erase a
record previously modified or erased by a concurrent
transaction.  Since your update would alter data you
transaction can not read, it has been refused by the
database system.');
('badhndcnt', NULL, NULL, NULL, 0, 90, NULL, 'bad handle count', 'If you believe the product should be licensed
for the machine you are using, your local Interbase
support contact should check that your registration
is correct in your copy of the authorization file.

If you wish that the product were licensed on your
machine, please contact your purchasing department
and ask them to call the Interbase sales department.', 'The component you requested is not licensed
on the computer you are using.');
('wrotpbver', NULL, NULL, NULL, 0, 91, NULL, 'wrong version of transaction parameter block', NULL, NULL);
('wroblrver', NULL, NULL, NULL, 0, 92, NULL, 'unsupported BLR version (expected @1, encountered @2)', NULL, NULL);
('wrodpbver', NULL, NULL, NULL, 0, 93, NULL, 'wrong version of database parameter block', NULL, NULL);
('blobnotsup', NULL, NULL, NULL, 0, 94, NULL, 'BLOB and array data types are not supported for @1 operation', NULL, NULL);
('badrelation', NULL, NULL, NULL, 0, 95, NULL, 'database corrupted', NULL, NULL);
('nodetach', NULL, NULL, NULL, 0, 96, NULL, 'internal error', NULL, NULL);
('notremote', NULL, NULL, NULL, 0, 97, NULL, 'internal error', NULL, NULL);
('trainlim', NULL, NULL, NULL, 0, 98, NULL, 'transaction in limbo', NULL, NULL);
('notinlim', NULL, NULL, NULL, 0, 99, NULL, 'transaction not in limbo', NULL, NULL);
('traoutsta', NULL, NULL, NULL, 0, 100, NULL, 'transaction outstanding', NULL, NULL);
('connect_reject', NULL, NULL, NULL, 0, 101, NULL, 'connection rejected by remote interface', NULL, NULL);
('dbfile', NULL, NULL, NULL, 0, 102, NULL, 'internal error', NULL, NULL);
('orphan', NULL, NULL, NULL, 0, 103, NULL, 'internal error', NULL, NULL);
('no_lock_mgr', NULL, NULL, NULL, 0, 104, NULL, 'no lock manager available', NULL, NULL);
('ctxinuse', NULL, NULL, NULL, 0, 105, NULL, 'context already in use (BLR error)', NULL, NULL);
('ctxnotdef', NULL, NULL, NULL, 0, 106, NULL, 'context not defined (BLR error)', NULL, NULL);
('datnotsup', NULL, NULL, NULL, 0, 107, NULL, 'data operation not supported', NULL, NULL);
('badmsgnum', NULL, NULL, NULL, 0, 108, NULL, 'undefined message number', NULL, NULL);
('badparnum', NULL, NULL, NULL, 0, 109, NULL, 'undefined parameter number', NULL, NULL);
('virmemexh', NULL, NULL, NULL, 0, 110, NULL, 'unable to allocate memory from operating system', NULL, NULL);
('blocking_signal', NULL, NULL, NULL, 0, 111, NULL, 'blocking signal has been received', NULL, NULL);
('lockmanerr', NULL, NULL, NULL, 0, 112, NULL, 'lock manager error', NULL, NULL);
('journerr', NULL, NULL, NULL, 0, 113, NULL, 'communication error with journal "@1"', NULL, NULL);
('keytoobig', NULL, NULL, NULL, 0, 114, NULL, 'key size exceeds implementation restriction for index "@1"', NULL, NULL);
('nullsegkey', NULL, NULL, NULL, 0, 115, NULL, 'null segment of UNIQUE KEY', NULL, NULL);
('sqlerr', NULL, NULL, NULL, 0, 116, NULL, 'SQL error code = @1', NULL, NULL);
('wrodynver', NULL, NULL, NULL, 0, 117, NULL, 'wrong DYN version', NULL, NULL);
('funnotdef', NULL, NULL, NULL, 0, 118, NULL, 'function @1 is not defined', NULL, NULL);
('funmismat', NULL, NULL, NULL, 0, 119, NULL, 'function @1 could not be matched', NULL, NULL);
('bad_msg_vec', NULL, NULL, NULL, 0, 120, NULL, '', NULL, 'This (blank) message is not in use as of Jan 1994.');
('bad_detach', NULL, NULL, NULL, 0, 121, NULL, 'database detach completed with errors', NULL, NULL);
('noargacc_read', NULL, NULL, NULL, 0, 122, NULL, 'database system cannot read argument @1', NULL, NULL);
('noargacc_write', NULL, NULL, NULL, 0, 123, NULL, 'database system cannot write argument @1', NULL, NULL);
('read_only', NULL, NULL, NULL, 0, 124, NULL, 'operation not supported', NULL, NULL);
('ext_err', NULL, NULL, NULL, 0, 125, NULL, '@1 extension error', NULL, NULL);
('non_updatable', NULL, NULL, NULL, 0, 126, NULL, 'not updatable', NULL, NULL);
('no_rollback', NULL, NULL, NULL, 0, 127, NULL, 'no rollback performed', NULL, NULL);
('bad_sec_info', NULL, NULL, NULL, 0, 128, NULL, '', NULL, NULL);
('invalid_sec_info', NULL, NULL, NULL, 0, 129, NULL, '', NULL, NULL);
('misc_interpreted', NULL, NULL, NULL, 0, 130, NULL, '@1', NULL, NULL);
('update_conflict', NULL, NULL, NULL, 0, 131, NULL, 'update conflicts with concurrent update', NULL, NULL);
('unlicensed', NULL, NULL, NULL, 0, 132, NULL, 'product @1 is not licensed', NULL, NULL);
('obj_in_use', NULL, NULL, NULL, 0, 133, NULL, 'object @1 is in use', NULL, 'The named relation or index is currently in use and cannot be deleted.');
('nofilter', NULL, NULL, NULL, 0, 134, 0, 'filter not found to convert type @1 to type @2', NULL, NULL);
('shadow_accessed', NULL, NULL, NULL, 0, 135, NULL, 'cannot attach active shadow file', 'If the original database file is available,
erase the record(s) in RDB$FILES which
defines the shadow.  Otherwise, use the
GFIX activate switch to convert the shadow
to an active database.', 'You have attempted to attach a file currently
attached to another database as a shadow file.');
('invalid_sdl', NULL, 'sdl', NULL, 0, 136, NULL, 'invalid slice description language at offset @1', NULL, NULL);
('out_of_bounds', NULL, NULL, NULL, 0, 137, NULL, 'subscript out of bounds', NULL, NULL);
('invalid_dimension', NULL, 'sdl.cpp', NULL, 0, 138, NULL, 'column not array or invalid dimensions (expected @1, encountered @2)', NULL, NULL);
('rec_in_limbo', NULL, NULL, NULL, 0, 139, 0, 'record from transaction @1 is stuck in limbo', NULL, NULL);
('shadow_missing', 'SDW_start', 'sdw.c', NULL, 0, 140, NULL, 'a file in manual shadow @1 is unavailable', NULL, NULL);
('cant_validate', 'jrd5_$attach_database', 'JRD', NULL, 0, 141, NULL, 'secondary server attachments cannot validate databases', NULL, NULL);
('cant_start_journal', 'jrd5_$attach_database', 'JRD', NULL, 0, 142, NULL, 'secondary server attachments cannot start journaling', NULL, NULL);
('gennotdef', 'parse', 'par.c', NULL, 0, 143, NULL, 'generator @1 is not defined', NULL, NULL);
('cant_start_logging', 'gds_$attach_database', 'jrd.c', NULL, 0, 144, NULL, 'secondary server attachments cannot start logging', NULL, NULL);
('bad_segstr_type', NULL, NULL, NULL, 0, 145, NULL, 'invalid BLOB type for operation', 'Program attempted a seek on a non-stream (i.e. segmented) blob.', 'Program attempted to a blob seek operation on a non-stream (i.e.
segmented) blob.');
('foreign_key', 'check_partner_index', 'IDX.C', NULL, 0, 146, NULL, 'violation of FOREIGN KEY constraint "@1" on table "@2"', NULL, NULL);
('high_minor', 'PAG_header', 'PAG', NULL, 0, 147, NULL, 'minor version too high found @1 expected @2', NULL, NULL);
('tra_state', 'TRA_reconnect', 'TRA', NULL, 0, 148, NULL, 'transaction @1 is @2', NULL, NULL);
('trans_invalid', 'TRA_invalidate', 'tra.c', NULL, 0, 149, NULL, 'transaction marked invalid by I/O error', NULL, NULL);
('buf_invalid', 'write_page', 'cch.c', NULL, 0, 150, NULL, 'cache buffer for page @1 invalid', NULL, NULL);
('indexnotdefined', 'set_index', 'exe.c', NULL, 0, 151, NULL, 'there is no index in table @1 with id @2', NULL, NULL);
('login', 'ServerAuth::authenticate', 'server.cpp', NULL, 0, 152, NULL, 'Your user name and password are not defined. Ask your database administrator to set up a Firebird login.', NULL, NULL);
('invalid_bookmark', 'EVL_expr', 'evl.c', NULL, 0, 153, NULL, 'invalid bookmark handle', NULL, NULL);
('bad_lock_level', 'lock_relation', 'EXE.C', NULL, 0, 154, NULL, 'invalid lock level @1', NULL, NULL);
('relation_lock', 'TRA_lock_relation', 'TRA.C', NULL, 0, 155, NULL, 'lock on table @1 conflicts with existing lock', NULL, NULL);
('record_lock', 'TRA_lock_record', 'TRA.C', NULL, 0, 156, NULL, 'requested record lock conflicts with existing lock', NULL, NULL);
('max_idx', 'BTR_resolve_slot', 'btr.c', NULL, 0, 157, NULL, 'maximum indexes per table (@1) exceeded', NULL, NULL);
('jrn_enable', 'OLD_dump', 'old', NULL, 0, 158, NULL, 'enable journal for database before starting online dump', NULL, NULL);
('old_failure', 'OLD_dump', 'old.c', NULL, 0, 159, NULL, 'online dump failure. Retry dump', NULL, NULL);
('old_in_progress', 'OLD_dump', 'old.c', NULL, 0, 160, NULL, 'an online dump is already in progress', NULL, NULL);
('old_no_space', 'OLD_dump', 'old.c', NULL, 0, 161, NULL, 'no more disk/tape space.  Cannot continue online dump', NULL, NULL);
('no_wal_no_jrn', 'AIL_enable', 'ail.c', NULL, 0, 162, NULL, 'journaling allowed only if database has Write-ahead Log', NULL, NULL);
('num_old_files', 'jrd5__attach_database', 'jrd.c', NULL, 0, 163, NULL, 'maximum number of online dump files that can be specified is 16', NULL, NULL);
('wal_file_open', 'REC_recover', 'rec.c', NULL, 0, 164, NULL, 'error in opening Write-ahead Log file during recovery', NULL, NULL);
('bad_stmt_handle', NULL, 'why.c', NULL, 0, 165, NULL, 'invalid statement handle', NULL, NULL);
('wal_failure', 'AIL_init', 'ail.c', NULL, 0, 166, NULL, 'Write-ahead log subsystem failure', NULL, NULL);
('walw_err', 'write_wal_block', 'walw.c', NULL, 0, 167, NULL, 'WAL Writer error', NULL, NULL);
('logh_small', 'setup_log_header_info', 'walw.c', NULL, 0, 168, NULL, 'Log file header of @1 too small', NULL, NULL);
('logh_inv_version', 'setup_log_header_info', 'walw.c', NULL, 0, 169, NULL, 'Invalid version of log file @1', NULL, NULL);
('logh_open_flag', 'setup_log_header_info', 'walw.c', NULL, 0, 170, NULL, 'Log file @1 not latest in the chain but open flag still set', NULL, NULL);
('logh_open_flag2', 'setup_log_header_info', 'walw.c', NULL, 0, 171, NULL, 'Log file @1 not closed properly; database recovery may be required', NULL, NULL);
('logh_diff_dbname', 'WALF_open_log_file', 'walf.c', NULL, 0, 172, NULL, 'Database name in the log file @1 is different', NULL, NULL);
('logf_unexpected_eof', 'read_next_block', 'walr.c', NULL, 0, 173, NULL, 'Unexpected end of log file @1 at offset @2', NULL, NULL);
('logr_incomplete', 'WALR_get', 'walr.c', NULL, 0, 174, NULL, 'Incomplete log record at offset @1 in log file @2', NULL, NULL);
('logr_header_small', 'WALR_open', 'walr.c', NULL, 0, 175, NULL, 'Log record header too small at offset @1 in log file @2', NULL, NULL);
('logb_small', 'WALR_open', 'walr.c', NULL, 0, 176, NULL, 'Log block too small at offset @1 in log file @2', NULL, NULL);
('wal_illegal_attach', 'WALC_init', 'walc.c', NULL, 0, 177, NULL, 'Illegal attempt to attach to an uninitialized WAL segment for @1', NULL, NULL);
('wal_invalid_wpb', 'setup_wal_params', 'walc.c', NULL, 0, 178, NULL, 'Invalid WAL parameter block option @1', NULL, NULL);
('wal_err_rollover', 'rollover_log', 'walw.c', NULL, 0, 179, NULL, 'Cannot roll over to the next log file @1', NULL, NULL);
('no_wal', 'check_wal', 'jrd.c', NULL, 0, 180, NULL, 'database does not use Write-ahead Log', NULL, NULL);
('drop_wal', 'check_wal', 'jrd.c', NULL, 0, 181, NULL, 'cannot drop log file when journaling is enabled', NULL, NULL);
('stream_not_defined', 'pass2', 'cmp.c', NULL, 0, 182, NULL, 'reference to invalid stream number', NULL, NULL);
('wal_subsys_error', 'wal_put2', 'wal.c', NULL, 0, 183, NULL, 'WAL subsystem encountered error', NULL, NULL);
('wal_subsys_corrupt', 'wal_put2', 'wal.c', NULL, 0, 184, NULL, 'WAL subsystem corrupted', NULL, NULL);
('no_archive', 'AIL_enable', 'ail.c', NULL, 0, 185, NULL, 'must specify archive file when enabling long term journal for databases with round-robin log files', NULL, NULL);
('shutinprog', 'jrd_attach_database', 'jrd.c tra.c', NULL, 0, 186, NULL, 'database @1 shutdown in progress', NULL, NULL);
('range_in_use', 'create_range', 'EXE.C', NULL, 0, 187, NULL, 'refresh range number @1 already in use', NULL, NULL);
('range_not_found', 'create_range', 'EXE.C', NULL, 0, 188, NULL, 'refresh range number @1 not found', NULL, NULL);
('charset_not_found', 'MET_find_intl_charset', 'MET.E', NULL, 0, 189, NULL, 'CHARACTER SET @1 is not defined', 'Define the character set name to the database, or reattach
without specifying a character set.', NULL);
('lock_timeout', NULL, 'lck.c', NULL, 0, 190, NULL, 'lock time-out on wait transaction', NULL, NULL);
('prcnotdef', 'par_procedure', 'PAR', NULL, 0, 191, NULL, 'procedure @1 is not defined', NULL, NULL);
('prcmismat', 'par_procedure', 'par.c', NULL, 0, 192, NULL, 'Input parameter mismatch for procedure @1', NULL, NULL);
('wal_bugcheck', 'WALC_bug', 'walc.c', NULL, 0, 193, NULL, 'Database @1: WAL subsystem bug for pid @2
@3', NULL, NULL);
('wal_cant_expand', 'increase_buffers', 'walw.c', NULL, 0, 194, NULL, 'Could not expand the WAL segment for database @1', NULL, NULL);
('codnotdef', 'par_condition', 'par.c', NULL, 0, 195, NULL, 'status code @1 unknown', NULL, NULL);
('xcpnotdef', 'par_condition', 'par.c', NULL, 0, 196, NULL, 'exception @1 not defined', NULL, NULL);
('except', 'looper', 'exe.c', NULL, 0, 197, NULL, 'exception @1', NULL, NULL);
('cache_restart', 'CASH_manager', 'cash.c', NULL, 0, 198, NULL, 'restart shared cache manager', NULL, NULL);
('bad_lock_handle', 'RLCK_release_lock', 'rlck.c', NULL, 0, 199, NULL, 'invalid lock handle', NULL, NULL);
('jrn_present', 'AIL_enable', 'ail.c', NULL, 0, 200, NULL, 'long-term journaling already enabled', NULL, NULL);
('wal_err_rollover2', 'rollover_log', 'walw.c', NULL, 0, 201, NULL, 'Unable to roll over please see Firebird log.', NULL, NULL);
('wal_err_logwrite', 'WALW_writer', 'walw.c', NULL, 0, 202, NULL, 'WAL I/O error.  Please see Firebird log.', NULL, NULL);
('wal_err_jrn_comm', 'journal_enable', 'walw.c', NULL, 0, 203, NULL, 'WAL writer - Journal server communication error.  Please see Firebird log.', NULL, NULL);
('wal_err_expansion', 'increase_buffers', 'walw.c', NULL, 0, 204, NULL, 'WAL buffers cannot be increased.  Please see Firebird log.', NULL, NULL);
('wal_err_setup', 'WALW_writer', 'walw.c', NULL, 0, 205, NULL, 'WAL setup error.  Please see Firebird log.', NULL, NULL);
('wal_err_ww_sync', NULL, NULL, NULL, 0, 206, NULL, 'obsolete', NULL, NULL);
('wal_err_ww_start', 'fork_writer', 'wal.c', NULL, 0, 207, NULL, 'Cannot start WAL writer for the database @1', NULL, NULL);
('shutdown', 'gds_$attach_database', 'jrd.c', NULL, 0, 208, NULL, 'database @1 shutdown', NULL, NULL);
('existing_priv_mod', 'trigger_messages', 'ini.e', NULL, 0, 209, NULL, 'cannot modify an existing user privilege', NULL, NULL);
('primary_key_ref', 'trigger_messages', 'ini.e', NULL, 0, 210, NULL, 'Cannot delete PRIMARY KEY being used in FOREIGN KEY definition.', NULL, NULL);
('primary_key_notnull', 'trigger_messages', 'ini.e', NULL, 0, 211, NULL, 'Column used in a PRIMARY constraint must be NOT NULL.', NULL, NULL);
('ref_cnstrnt_notfound', 'trigger_messages', 'ini.e', NULL, 0, 212, NULL, 'Name of Referential Constraint not defined in constraints table.', NULL, NULL);
('foreign_key_notfound', 'trigger_messages', 'ini.e', NULL, 0, 213, NULL, 'Non-existent PRIMARY or UNIQUE KEY specified for FOREIGN KEY.', NULL, NULL);
('ref_cnstrnt_update', 'trigger_messages', 'ini.e', NULL, 0, 214, NULL, 'Cannot update constraints (RDB$REF_CONSTRAINTS).', NULL, NULL);
('check_cnstrnt_update', 'trigger_messages', 'ini.e', NULL, 0, 215, NULL, 'Cannot update constraints (RDB$CHECK_CONSTRAINTS).', NULL, NULL);
('check_cnstrnt_del', 'trigger_messages', 'ini.e', NULL, 0, 216, NULL, 'Cannot delete CHECK constraint entry (RDB$CHECK_CONSTRAINTS)', NULL, NULL);
('integ_index_seg_del', 'trigger_messages', 'ini.e', NULL, 0, 217, NULL, 'Cannot delete index segment used by an Integrity Constraint', NULL, NULL);
('integ_index_seg_mod', 'trigger_messages', 'ini.e', NULL, 0, 218, NULL, 'Cannot update index segment used by an Integrity Constraint', NULL, NULL);
('integ_index_del', 'trigger_messages', 'ini.e', NULL, 0, 219, NULL, 'Cannot delete index used by an Integrity Constraint', NULL, NULL);
('integ_index_mod', 'trigger_messages', 'ini.e', NULL, 0, 220, NULL, 'Cannot modify index used by an Integrity Constraint', NULL, NULL);
('check_trig_del', 'trigger_messages', 'ini.e', NULL, 0, 221, NULL, 'Cannot delete trigger used by a CHECK Constraint', NULL, NULL);
('check_trig_update', 'trigger_messages', 'ini.e', NULL, 0, 222, NULL, 'Cannot update trigger used by a CHECK Constraint', NULL, NULL);
('cnstrnt_fld_del', 'trigger_messages', 'ini.e', NULL, 0, 223, NULL, 'Cannot delete column being used in an Integrity Constraint.', NULL, NULL);
('cnstrnt_fld_rename', 'trigger_messages', 'ini.e', NULL, 0, 224, NULL, 'Cannot rename column being used in an Integrity Constraint.', NULL, NULL);
('rel_cnstrnt_update', 'trigger_messages', 'ini.e', NULL, 0, 225, NULL, 'Cannot update constraints (RDB$RELATION_CONSTRAINTS).', NULL, NULL);
('constaint_on_view', 'trigger_messages', 'ini.e', NULL, 0, 226, NULL, 'Cannot define constraints on views', NULL, NULL);
('invld_cnstrnt_type', 'trigger_messages', 'ini.e', NULL, 0, 227, NULL, 'internal Firebird consistency check (invalid RDB$CONSTRAINT_TYPE)', NULL, NULL);
('primary_key_exists', 'trigger_messages', 'ini.e', NULL, 0, 228, NULL, 'Attempt to define a second PRIMARY KEY for the same table', NULL, NULL);
('systrig_update', 'trigger_messages', 'ini.e', NULL, 0, 229, NULL, 'cannot modify or erase a system trigger', NULL, NULL);
('not_rel_owner', 'trigger_messages', 'ini.e', NULL, 0, 230, NULL, 'only the owner of a table may reassign ownership', NULL, NULL);
('grant_obj_notfound', 'trigger_messages', 'ini.e', NULL, 0, 231, NULL, 'could not find object for GRANT', NULL, NULL);
('grant_fld_notfound', 'trigger_messages', 'ini.e', NULL, 0, 232, NULL, 'could not find column for GRANT', NULL, NULL);
('grant_nopriv', 'trigger_messages', 'ini.e', NULL, 0, 233, NULL, 'user does not have GRANT privileges for operation', NULL, NULL);
('nonsql_security_rel', 'trigger_messages', 'ini.e', NULL, 0, 234, NULL, 'object has non-SQL security class defined', NULL, NULL);
('nonsql_security_fld', 'trigger_messages', 'ini.e', NULL, 0, 235, NULL, 'column has non-SQL security class defined', NULL, NULL);
('wal_cache_err', 'create_log', 'dfw.e', NULL, 0, 236, NULL, 'Write-ahead Log without shared cache configuration not allowed', NULL, NULL);
('shutfail', 'SHUT_database', 'shut.c', NULL, 0, 237, NULL, 'database shutdown unsuccessful', NULL, NULL);
('check_constraint', 'blr_abort', 'exe.c', NULL, 0, 238, NULL, 'Operation violates CHECK constraint @1 on view or table @2', NULL, NULL);
('bad_svc_handle', NULL, 'why.c', NULL, 0, 239, NULL, 'invalid service handle', NULL, NULL);
('shutwarn', 'return_success', 'jrd.c', NULL, 0, 240, NULL, 'database @1 shutdown in @2 seconds', NULL, NULL);
('wrospbver', 'get_options', 'svc.c', NULL, 0, 241, NULL, 'wrong version of service parameter block', NULL, NULL);
('bad_spb_form', 'get_options', 'svc.c', NULL, 0, 242, NULL, 'unrecognized service parameter block', NULL, NULL);
('svcnotdef', 'SVC_attach', 'svc.c', NULL, 0, 243, NULL, 'service @1 is not defined', NULL, NULL);
('no_jrn', 'AIL_disable', 'ail.c', NULL, 0, 244, NULL, 'long-term journaling not enabled', NULL, NULL);
('transliteration_failed', 'several - INTL_convert_bytes', 'intl.c', NULL, 0, 245, NULL, 'Cannot transliterate character between character sets', NULL, NULL);
('start_cm_for_wal', 'AIL_init', 'ail.c', NULL, 0, 246, NULL, 'WAL defined; Cache Manager must be started first', NULL, NULL);
('wal_ovflow_log_required', 'setup_wal_params', 'walc.c', NULL, 0, 247, NULL, 'Overflow log specification required for round-robin log', NULL, NULL);
('text_subtype', 'init_obj', 'intl.c', NULL, 0, 248, NULL, 'Implementation of text subtype @1 not located.', NULL, NULL);
('dsql_error', '(several)', 'dsql (several)', NULL, 0, 249, NULL, 'Dynamic SQL Error', NULL, NULL);
('dsql_command_err', 'several', 'dsql', NULL, 0, 250, NULL, 'Invalid command', NULL, NULL);
('dsql_constant_err', 'gen_constant', 'dsql gen.c', NULL, 0, 251, NULL, 'Data type for constant unknown', NULL, NULL);
('dsql_cursor_err', '(several)', 'dsql (several)', NULL, 0, 252, NULL, 'Invalid cursor reference', NULL, NULL);
('dsql_datatype_err', '(several)', 'dsql (several)', NULL, 0, 253, NULL, 'Data type unknown', NULL, NULL);
('dsql_decl_err', 'dsql_set_cursor_name', 'dsql dsql.c', NULL, 0, 254, NULL, 'Invalid cursor declaration', NULL, NULL);
('dsql_cursor_update_err', 'pass1_cursor', 'dsql pass1.c', NULL, 0, 255, NULL, 'Cursor @1 is not updatable', NULL, NULL);
('dsql_cursor_open_err', 'dsql_execute', 'dsql dsql.c', NULL, 0, 256, NULL, 'Attempt to reopen an open cursor', NULL, NULL);
('dsql_cursor_close_err', 'dsql_free_statement', 'dsql dsql.c', NULL, 0, 257, NULL, 'Attempt to reclose a closed cursor', NULL, NULL);
('dsql_field_err', 'field_error', 'dsql pass1.c', NULL, 0, 258, NULL, 'Column unknown', NULL, NULL);
('dsql_internal_err', '(several)', 'dsql (several)', NULL, 0, 259, NULL, 'Internal error', NULL, NULL);
('dsql_relation_err', 'PASS1_make_context', 'dsql pass1.c', NULL, 0, 260, NULL, 'Table unknown', NULL, NULL);
('dsql_procedure_err', 'PASS1_statement', 'dsql pass1.c', NULL, 0, 261, NULL, 'Procedure unknown', NULL, NULL);
('dsql_request_err', 'lookup_stmt', 'dsql user_dsql.c', NULL, 0, 262, NULL, 'Request unknown', NULL, NULL);
('dsql_sqlda_err', '(several)', 'dsql (several)', NULL, 0, 263, NULL, 'SQLDA missing or incorrect version, or incorrect number/type of variables', NULL, NULL);
('dsql_var_count_err', 'pass1_insert', 'dsql pass1.c', NULL, 0, 264, NULL, 'Count of read-write columns does not equal count of values', NULL, NULL);
('dsql_stmt_handle', 'bad_sql_handle', 'whyb.c', NULL, 0, 265, NULL, 'Invalid statement handle', NULL, NULL);
('dsql_function_err', 'pass1_udf', 'dsql pass1.c', NULL, 0, 266, NULL, 'Function unknown', NULL, NULL);
('dsql_blob_err', 'pass1_blob', 'dsql pass1.c', NULL, 0, 267, NULL, 'Column is not a BLOB', NULL, NULL);
('collation_not_found', 'DDL_resolve_intl_type', 'dsql/ddl.c', NULL, 0, 268, NULL, 'COLLATION @1 for CHARACTER SET @2 is not defined', NULL, NULL);
('collation_not_for_charset', 'DDL_resolve_intl_type', 'dsql/ddl.c', NULL, 0, 269, NULL, 'COLLATION @1 is not valid for specified CHARACTER SET', NULL, NULL);
('dsql_dup_option', 'GEN_start_transaction', 'dsql gen.c', NULL, 0, 270, NULL, 'Option specified more than once', NULL, NULL);
('dsql_tran_err', 'GEN_start_transaction', 'dsql gen.c', NULL, 0, 271, NULL, 'Unknown transaction option', NULL, NULL);
('dsql_invalid_array', 'PASS1_node', 'dsql pass1.c', NULL, 0, 272, NULL, 'Invalid array reference', NULL, NULL);
('dsql_max_arr_dim_exceeded', 'define_dimensions', 'dsql ddl.c', NULL, 0, 273, NULL, 'Array declared with too many dimensions', NULL, NULL);
('dsql_arr_range_error', 'define_dimensions', 'dsql ddl.c', NULL, 0, 274, NULL, 'Illegal array dimension range', NULL, NULL);
('dsql_trigger_err', 'define_trigger', 'dsql ddl.c', NULL, 0, 275, NULL, 'Trigger unknown', NULL, NULL);
('dsql_subselect_err', 'PASS1_node', 'dsql pass1.c', NULL, 0, 276, NULL, 'Subselect illegal in this context', NULL, NULL);
('dsql_crdb_prepare_err', 'GDS_DSQL_PREPARE', 'dsql dsql.c', NULL, 0, 277, NULL, 'Cannot prepare a CREATE DATABASE/SCHEMA statement', NULL, NULL);
('specify_field_err', 'define_view', 'dsql ddl.c', NULL, 0, 278, NULL, 'must specify column name for view select expression', NULL, NULL);
('num_field_err', 'define_view', 'dsql ddl.c', NULL, 0, 279, NULL, 'number of columns does not match select list', NULL, NULL);
('col_name_err', 'define_view', 'dsql ddl.c', NULL, 0, 280, NULL, 'Only simple column names permitted for VIEW WITH CHECK OPTION', NULL, NULL);
('where_err', 'define_view', 'dsql ddl.c', NULL, 0, 281, NULL, 'No WHERE clause for VIEW WITH CHECK OPTION', NULL, NULL);
('table_view_err', 'define_view', 'dsql ddl.c', NULL, 0, 282, NULL, 'Only one table allowed for VIEW WITH CHECK OPTION', NULL, NULL);
('distinct_err', 'define_view', 'dsql ddl.c', NULL, 0, 283, NULL, 'DISTINCT, GROUP or HAVING not permitted for VIEW WITH CHECK OPTION', NULL, NULL);
('key_field_count_err', 'foreign_key', 'dsql ddl.c', NULL, 0, 284, NULL, 'FOREIGN KEY column count does not match PRIMARY KEY', NULL, NULL);
('subquery_err', 'replace_field_names', 'ddl.c', NULL, 0, 285, NULL, 'No subqueries permitted for VIEW WITH CHECK OPTION', NULL, NULL);
('expression_eval_err', 'GEN_expr', 'dsql gen.c', NULL, 0, 286, NULL, 'expression evaluation not supported', NULL, NULL);
('node_err', 'GEN_statement', 'dsql gen.c', NULL, 0, 287, NULL, 'gen.c: node not supported', NULL, NULL);
('command_end_err', 'yyerror', 'dsql parse.c', NULL, 0, 288, NULL, 'Unexpected end of command', NULL, NULL);
('index_name', 'check_dependencies', 'dfw.e', NULL, 0, 289, NULL, 'INDEX @1', NULL, NULL);
('exception_name', 'check_dependencies', 'dfw.e', NULL, 0, 290, NULL, 'EXCEPTION @1', NULL, NULL);
('field_name', 'check_dependencies', 'dfw.e', NULL, 0, 291, NULL, 'COLUMN @1', NULL, NULL);
('token_err', 'PASS1_statement', 'dsql pass1.c', NULL, 0, 292, NULL, 'Token unknown', NULL, NULL);
('union_err', 'PASS1_statement', 'dsql pass1.c', NULL, 0, 293, NULL, 'union not supported', NULL, NULL);
('dsql_construct_err', 'PASS1_statement', 'dsql pass1.c', NULL, 0, 294, NULL, 'Unsupported DSQL construct', NULL, NULL);
('field_aggregate_err', 'aggregate_found', 'dsql pass1.c', NULL, 0, 295, NULL, 'column used with aggregate', NULL, NULL);
('field_ref_err', 'pass1_rse', 'dsql pass1.c', NULL, 0, 296, NULL, 'invalid column reference', NULL, NULL);
('order_by_err', 'pass1_sort', 'dsql pass1.c', NULL, 0, 297, NULL, 'invalid ORDER BY clause', NULL, NULL);
('return_mode_err', 'define_udf', 'dsql ddl.c', NULL, 0, 298, NULL, 'Return mode by value not allowed for this data type', NULL, NULL);
('extern_func_err', 'define_udf', 'dsql ddl.c', NULL, 0, 299, NULL, 'External functions cannot have more than 10 parameters', NULL, NULL);
('alias_conflict_err', 'PASS1_make_context', 'dsql pass1.c', NULL, 0, 300, NULL, 'alias @1 conflicts with an alias in the same statement', NULL, NULL);
('procedure_conflict_error', 'PASS1_make_context', 'dsql pass1.c', NULL, 0, 301, NULL, 'alias @1 conflicts with a procedure in the same statement', NULL, NULL);
('relation_conflict_err', 'PASS1_make_context', 'dsql pass1.c', NULL, 0, 302, NULL, 'alias @1 conflicts with a table in the same statement', NULL, NULL);
('dsql_domain_err', 'GEN_expr', 'dsql gen.c', NULL, 0, 303, NULL, 'Illegal use of keyword VALUE', NULL, NULL);
('idx_seg_err', 'create_index', 'dfw.e', NULL, 0, 304, NULL, 'segment count of 0 defined for index @1', NULL, NULL);
('node_name_err', 'check_filename', 'dfw.e', NULL, 0, 305, NULL, 'A node name is not permitted in a secondary, shadow, cache or log file name', NULL, NULL);
('table_name', 'check_dependencies', 'dfw.e', NULL, 0, 306, NULL, 'TABLE @1', NULL, NULL);
('proc_name', 'check_dependencies', 'dfw.e', NULL, 0, 307, NULL, 'PROCEDURE @1', NULL, NULL);
('idx_create_err', 'create_index', 'dfw.e', NULL, 0, 308, NULL, 'cannot create index @1', NULL, NULL);
('wal_shadow_err', 'add_shadow', 'dfw.e', NULL, 0, 309, NULL, 'Write-ahead Log with shadowing configuration not allowed', NULL, NULL);
('dependency', 'several', 'dfw.e', NULL, 0, 310, NULL, 'there are @1 dependencies', NULL, NULL);
('idx_key_err', 'create_index', 'dfw.e', NULL, 0, 311, NULL, 'too many keys defined for index @1', NULL, NULL);
('dsql_file_length_err', 'define_shadow', 'dsql ddl.c', NULL, 0, 312, NULL, 'Preceding file did not specify length, so @1 must include starting page number', NULL, NULL);
('dsql_shadow_number_err', 'define_shadow', 'dsql ddl.c', NULL, 0, 313, NULL, 'Shadow number must be a positive integer', NULL, NULL);
('dsql_token_unk_err', 'yyerror', 'dsql parse.y', NULL, 0, 314, NULL, 'Token unknown - line @1, column @2', NULL, NULL);
('dsql_no_relation_alias', 'pass1_relation_alias', 'dsql pass1.c', NULL, 0, 315, NULL, 'there is no alias or table named @1 at this scope level', NULL, NULL);
('indexname', 'par_plan', 'par.c', NULL, 0, 316, NULL, 'there is no index @1 for table @2', NULL, NULL);
('no_stream_plan', 'plan_check', 'cmp.c', NULL, 0, 317, NULL, 'table @1 is not referenced in plan', NULL, NULL);
('stream_twice', 'plan_set', 'cmp.c', NULL, 0, 318, NULL, 'table @1 is referenced more than once in plan; use aliases to distinguish', NULL, NULL);
('stream_not_found', 'plan_set', 'cmp.c', NULL, 0, 319, NULL, 'table @1 is referenced in the plan but not the from list', NULL, NULL);
('collation_requires_text', 'DDL_resolve_intl_type', 'DSQL/ddl.c', NULL, 0, 320, NULL, 'Invalid use of CHARACTER SET or COLLATE', NULL, NULL);
('dsql_domain_not_found', 'ddl.c', 'DSQL', NULL, 0, 321, NULL, 'Specified domain or source column @1 does not exist', NULL, NULL);
('index_unused', 'check_indices', 'opt.c', NULL, 0, 322, NULL, 'index @1 cannot be used in the specified plan', NULL, NULL);
('dsql_self_join', 'dsql pass1_relation_alias', 'pass1.c', NULL, 0, 323, NULL, 'the table @1 is referenced twice; use aliases to differentiate', NULL, NULL);
('stream_bof', 'seek', 'exe.c', NULL, 0, 324, NULL, 'attempt to fetch before the first record in a record stream', NULL, NULL);
('stream_crack', 'mark_crack', 'exe.c', NULL, 0, 325, NULL, 'the current position is on a crack', NULL, NULL);
('db_or_file_exists', 'PREPARSE_execute', 'dsql preparse.c', NULL, 0, 326, NULL, 'database or file exists', NULL, NULL);
('invalid_operator', 'pars', 'par.c', NULL, 0, 327, NULL, 'invalid comparison operator for find operation', NULL, NULL);
('conn_lost', 'check_response', 'head.c', NULL, 0, 328, NULL, 'Connection lost to pipe server', NULL, NULL);
('bad_checksum', 'CASH_fetch', 'cash.c', NULL, 0, 329, NULL, 'bad checksum', NULL, NULL);
('page_type_err', 'CCH_fetch_validate', 'cch.c', NULL, 0, 330, NULL, 'wrong page type', NULL, NULL);
('ext_readonly_err', 'EXT_store', 'ext.c', NULL, 0, 331, NULL, 'Cannot insert because the file is readonly or is on a read only medium.', NULL, NULL);
('sing_select_err', 'get_record', 'rse.c', NULL, 0, 332, NULL, 'multiple rows in singleton select', NULL, NULL);
('psw_attach', 'lookup_user', 'pwd.e', NULL, 0, 333, NULL, 'cannot attach to password database', NULL, NULL);
('psw_start_trans', 'lookup_user', 'pwd.e', NULL, 0, 334, NULL, 'cannot start transaction for password database', NULL, NULL);
('invalid_direction', 'find', 'exe.c', NULL, 0, 335, NULL, 'invalid direction for find operation', NULL, NULL);
('dsql_var_conflict', 'PASS1_statement', 'pass1.c', NULL, 0, 336, NULL, 'variable @1 conflicts with parameter in same procedure', NULL, NULL);
('dsql_no_blob_array', 'define_computed', 'ddl.c', NULL, 0, 337, NULL, 'Array/BLOB/DATE data types not allowed in arithmetic', NULL, NULL);
('dsql_base_table', 'dsql pass1_alias_list', 'pass1.c', NULL, 0, 338, NULL, '@1 is not a valid base table of the specified view', NULL, NULL);
('duplicate_base_table', 'plan_set', 'cmp.c', NULL, 0, 339, NULL, 'table @1 is referenced twice in view; use an alias to distinguish', NULL, NULL);
('view_alias', 'plan_set', 'cmp.c', NULL, 0, 340, NULL, 'view @1 has more than one base table; use aliases to distinguish', NULL, NULL);
('index_root_page_full', 'BTR_reserve_slot', 'BTR', NULL, 0, 341, NULL, 'cannot add index, index root page is full.', NULL, NULL);
('dsql_blob_type_unknown', 'DDL_resolve_intl_type', 'dsql/ddl.c', NULL, 0, 342, NULL, 'BLOB SUB_TYPE @1 is not defined', NULL, NULL);
('req_max_clones_exceeded', 'EXE_find_request', 'exe.c', NULL, 0, 343, NULL, 'Too many concurrent executions of the same request', NULL, NULL);
('dsql_duplicate_spec', 'create_domain', 'DSQL', NULL, 0, 344, NULL, 'duplicate specification of @1 - not supported', NULL, NULL);
('unique_key_violation', 'ERR_duplicate_error', 'err.c', NULL, 0, 345, NULL, 'violation of PRIMARY or UNIQUE KEY constraint "@1" on table "@2"', NULL, NULL);
('srvr_version_too_old', 'isc_dsql_exec_immed2_m', 'why.c', NULL, 0, 346, NULL, 'server version too old to support all CREATE DATABASE options', NULL, NULL);
('drdb_completed_with_errs', 'isc_drop_database', 'why.c', NULL, 0, 347, NULL, 'drop database completed with errors', NULL, NULL);
('dsql_procedure_use_err', 'PASS1_make_context', 'dsql/pass1.c', NULL, 0, 348, NULL, 'procedure @1 does not return any values', NULL, NULL);
('dsql_count_mismatch', 'gen_for_select', 'dsql/gen.c', NULL, 0, 349, NULL, 'count of column list and variable list do not match', NULL, NULL);
('blob_idx_err', 'create_index', 'dfw.e', NULL, 0, 350, NULL, 'attempt to index BLOB column in index @1', NULL, NULL);
('array_idx_err', 'create_index', 'dfw.e', NULL, 0, 351, NULL, 'attempt to index array column in index @1', NULL, NULL);
('key_field_err', 'create_index', 'dfw.e', NULL, 0, 352, NULL, 'too few key columns found for index @1 (incorrect column name?)', NULL, NULL);
('no_delete', 'several', 'dfw.e', NULL, 0, 353, NULL, 'cannot delete', NULL, NULL);
('del_last_field', 'delete_rfr', 'dfw.e', NULL, 0, 354, NULL, 'last column in a table cannot be deleted', NULL, NULL);
('sort_err', 'error', 'sort.c', NULL, 0, 355, NULL, 'sort error', NULL, NULL);
('sort_mem_err', 'SORT_init', 'sort.c', NULL, 0, 356, NULL, 'sort error: not enough memory', NULL, NULL);
('version_err', 'make_version', 'dfw.e', NULL, 0, 357, NULL, 'too many versions', NULL, NULL);
('inval_key_posn', 'create_index', 'dfw.e', NULL, 0, 358, NULL, 'invalid key position', NULL, NULL);
('no_segments_err', 'PCMET_expression_index', 'pcmet.e', NULL, 0, 359, NULL, 'segments not allowed in expression index @1', NULL, NULL);
('crrp_data_err', 'validate', 'sort.c', NULL, 0, 360, NULL, 'sort error: corruption in data structure', NULL, NULL);
('rec_size_err', 'make_format', 'dfw.e', NULL, 0, 361, NULL, 'new record size of @1 bytes is too big', NULL, NULL);
('dsql_field_ref', 'MAKE_desc', 'dsql make.c', NULL, 0, 362, NULL, 'Inappropriate self-reference of column', NULL, NULL);
('req_depth_exceeded', 'CMP_find_request', 'cmp.c', NULL, 0, 363, NULL, 'request depth exceeded. (Recursive definition?)', NULL, NULL);
('no_field_access', 'pass1', 'cmp.c', NULL, 0, 364, NULL, 'cannot access column @1 in view @2', NULL, NULL);
('no_dbkey', 'par_fetch', 'par.c', NULL, 0, 365, NULL, 'dbkey not available for multi-table views', NULL, NULL);
('jrn_format_err', 'JIO_open', 'jio.c', NULL, 0, 366, NULL, 'journal file wrong format', NULL, NULL);
('jrn_file_full', 'JIO_put', 'jio.c', NULL, 0, 367, NULL, 'intermediate journal file full', NULL, NULL);
('dsql_open_cursor_request', 'dsql_prepare', 'dsql.c', NULL, 0, 368, NULL, 'The prepare statement identifies a prepare statement with an open cursor', NULL, NULL);
('ib_error', NULL, NULL, NULL, 0, 369, NULL, 'Firebird error', NULL, NULL);
('cache_redef', NULL, 'dsql parse.y', NULL, 0, 370, NULL, 'Cache redefined', NULL, NULL);
('cache_too_small', NULL, 'dsql parse.y', NULL, 0, 371, NULL, 'Insufficient memory to allocate page buffer cache', NULL, NULL);
('log_redef', NULL, 'dsql parse.y', NULL, 0, 372, NULL, 'Log redefined', NULL, NULL);
('log_too_small', 'check_log_file_attrs', 'dsql parse.y', NULL, 0, 373, NULL, 'Log size too small', NULL, NULL);
('partition_too_small', 'check_log_file_attrs', 'dsql parse.y', NULL, 0, 374, NULL, 'Log partition size too small', NULL, NULL);
('partition_not_supp', NULL, 'dsql parse.y', NULL, 0, 375, NULL, 'Partitions not supported in series of log file specification', NULL, NULL);
('log_length_spec', 'check_log_file_attrs', 'dsql parse.y', NULL, 0, 376, NULL, 'Total length of a partitioned log must be specified', NULL, NULL);
('precision_err', NULL, 'dsql parse.y', NULL, 0, 377, NULL, 'Precision must be from 1 to 18', NULL, NULL);
('scale_nogt', NULL, 'dsql parse.y', NULL, 0, 378, NULL, 'Scale must be between zero and precision', NULL, NULL);
('expec_short', NULL, 'dsql parse.y', NULL, 0, 379, NULL, 'Short integer expected', NULL, NULL);
('expec_long', NULL, 'dsql parse.y', NULL, 0, 380, NULL, 'Long integer expected', NULL, NULL);
('expec_ushort', NULL, 'dsql parse.y', NULL, 0, 381, NULL, 'Unsigned short integer expected', NULL, NULL);
('escape_invalid', 'string_function()', 'evl.c', NULL, 0, 382, NULL, 'Invalid ESCAPE sequence', NULL, NULL);
('svcnoexe', 'service_close', 'svc.c', NULL, 0, 383, NULL, 'service @1 does not have an associated executable', NULL, NULL);
('net_lookup_err', 'INET_connect', 'inet.c', NULL, 0, 384, NULL, 'Failed to locate host machine.', NULL, NULL);
('service_unknown', 'INET_connect', 'inet.c', NULL, 0, 385, NULL, 'Undefined service @1/@2.', NULL, NULL);
('host_unknown', 'INET_Connect', 'inet.c', NULL, 0, 386, NULL, 'The specified name was not found in the hosts file or Domain Name Services.', NULL, NULL);
('grant_nopriv_on_base', 'trigger_messages', 'ini.e', NULL, 0, 387, NULL, 'user does not have GRANT privileges on base table/view for operation', NULL, NULL);
('dyn_fld_ambiguous', 'pass1_field', 'dsql pass1.c', NULL, 0, 388, NULL, 'Ambiguous column reference.', NULL, NULL);
('dsql_agg_ref_err', 'PASS1_node', 'dsql', NULL, 0, 389, NULL, 'Invalid aggregate reference', NULL, NULL);
('complex_view', 'base_stream', 'cmp.c', NULL, 0, 390, NULL, 'navigational stream @1 references a view with more than one base table', NULL, NULL);
('unprepared_stmt', 'isc_dsql_execute2', 'WHY.C', NULL, 0, 391, NULL, 'Attempt to execute an unprepared dynamic SQL statement.', NULL, NULL);
('expec_positive', 'short_pos_integer:', 'dsql/parse.y', NULL, 0, 392, NULL, 'Positive value expected', NULL, NULL);
('dsql_sqlda_value_err', 'several', 'dsql/utld.c', NULL, 0, 393, NULL, 'Incorrect values within SQLDA structure', NULL, NULL);
('invalid_array_id', 'blb::put_slice', 'blb.c', NULL, 0, 394, NULL, 'invalid blob id', NULL, NULL);
('extfile_uns_op', 'IDX_create_index', 'jrd/idx.c', NULL, 0, 395, NULL, 'Operation not supported for EXTERNAL FILE table @1', NULL, NULL);
('svc_in_use', 'SVC_attach_service', 'jrd/svc.c', NULL, 0, 396, NULL, 'Service is currently busy: @1', NULL, NULL);
('err_stack_limit', 'VIO_start_save_point', 'vio.c', NULL, 0, 397, NULL, 'stack size insufficent to execute current request', NULL, NULL);
('invalid_key', 'NAV_find_record', 'jrd/nav.c', NULL, 0, 398, NULL, 'Invalid key for find operation', NULL, NULL);
('net_init_error', 'initWSA', 'inet.c', NULL, 0, 399, NULL, 'Error initializing the network software.', NULL, NULL);
('loadlib_failure', 'initWSA', 'inet.c', NULL, 0, 400, NULL, 'Unable to load required library @1.', NULL, NULL);
('network_error', 'several', 'remote/inet.c and others', NULL, 0, 401, NULL, 'Unable to complete network request to host "@1".', NULL, NULL);
('net_connect_err', 'several', 'remote/inet.c and others', NULL, 0, 402, NULL, 'Failed to establish a connection.', NULL, NULL);
('net_connect_listen_err', 'several', 'remote/inet.c and others', NULL, 0, 403, NULL, 'Error while listening for an incoming connection.', NULL, NULL);
('net_event_connect_err', 'several', 'remote/inet.c and others', NULL, 0, 404, NULL, 'Failed to establish a secondary connection for event processing.', NULL, NULL);
('net_event_listen_err', 'several', 'remote/inet.c and others', NULL, 0, 405, NULL, 'Error while listening for an incoming event connection request.', NULL, NULL);
('net_read_err', 'several', 'remote/inet.c and others', NULL, 0, 406, NULL, 'Error reading data from the connection.', NULL, NULL);
('net_write_err', 'several', 'remote/inet.c and others', NULL, 0, 407, NULL, 'Error writing data to the connection.', NULL, NULL);
('integ_index_deactivate', 'execute_triggers', 'exe.c', NULL, 0, 408, NULL, 'Cannot deactivate index used by an integrity constraint', NULL, NULL);
('integ_deactivate_primary', 'exe.c', 'jrd', NULL, 0, 409, NULL, 'Cannot deactivate index used by a PRIMARY/UNIQUE constraint', NULL, NULL);
('cse_not_supported', 'parse', 'par.c', NULL, 0, 410, NULL, 'Client/Server Express not supported in this release', NULL, NULL);
('tra_must_sweep', 'TRA_start', 'tra.c', NULL, 0, 411, NULL, '', NULL, NULL);
('unsupported_network_drive', 'GDS_ATTACH_DATABASE', 'jrd.c', NULL, 0, 412, NULL, 'Access to databases on file servers is not supported.', NULL, NULL);
('io_create_err', NULL, NULL, NULL, 0, 413, NULL, 'Error while trying to create file', NULL, NULL);
('io_open_err', NULL, NULL, NULL, 0, 414, NULL, 'Error while trying to open file', NULL, NULL);
('io_close_err', NULL, NULL, NULL, 0, 415, NULL, 'Error while trying to close file', NULL, NULL);
('io_read_err', NULL, NULL, NULL, 0, 416, NULL, 'Error while trying to read from file', NULL, NULL);
('io_write_err', NULL, NULL, NULL, 0, 417, NULL, 'Error while trying to write to file', NULL, NULL);
('io_delete_err', NULL, NULL, NULL, 0, 418, NULL, 'Error while trying to delete file', NULL, NULL);
('io_access_err', NULL, NULL, NULL, 0, 419, NULL, 'Error while trying to access file', NULL, NULL);
('udf_exception', 'open_blob', 'BLF.E', NULL, 0, 420, NULL, 'A fatal exception occurred during the execution of a user defined function.', NULL, NULL);
('lost_db_connection', 'send_and_wait', 'ipclient.c', NULL, 0, 421, NULL, 'connection lost to database', NULL, NULL);
('no_write_user_priv', 'trigger_messages', 'ini.e', NULL, 0, 422, NULL, 'User cannot write to RDB$USER_PRIVILEGES', NULL, NULL);
('token_too_long', 'yylex', 'dsql/parse.y', NULL, 0, 423, NULL, 'token size exceeds limit', NULL, NULL);
('max_att_exceeded', 'GDS_ATTACH_DATABASE', 'jrd.c', NULL, 0, 424, NULL, 'Maximum user count exceeded.  Contact your database administrator.', NULL, NULL);
('login_same_as_role_name', 'SCL_init', 'scl.e', NULL, 0, 425, NULL, 'Your login @1 is same as one of the SQL role name. Ask your database administrator to set up a valid Firebird login.', NULL, NULL);
('reftable_requires_pk', 'foreign_key', 'ddl.c', NULL, 0, 426, NULL, '"REFERENCES table" without "(column)" requires PRIMARY KEY on referenced table', NULL, NULL);
('usrname_too_long', 'isc_add_user', 'alt.c', NULL, 0, 427, NULL, 'The username entered is too long.  Maximum length is 31 bytes.', NULL, NULL);
('password_too_long', 'isc_add_user', 'alt.c', NULL, 0, 428, NULL, 'The password specified is too long.  Maximum length is 8 bytes.', NULL, NULL);
('usrname_required', 'isc_add_user', 'alt.c', NULL, 0, 429, NULL, 'A username is required for this operation.', NULL, NULL);
('password_required', 'isc_add_user', 'alt.c', NULL, 0, 430, NULL, 'A password is required for this operation', NULL, NULL);
('bad_protocol', 'isc_add_user', 'alt.c', NULL, 0, 431, NULL, 'The network protocol specified is invalid', NULL, NULL);
('dup_usrname_found', 'isc_add_user', 'alt.c', NULL, 0, 432, NULL, 'A duplicate user name was found in the security database', NULL, NULL);
('usrname_not_found', 'isc_delete_user', 'alt.c', NULL, 0, 433, NULL, 'The user name specified was not found in the security database', NULL, NULL);
('error_adding_sec_record', 'isc_add_user', 'alt.c', NULL, 0, 434, NULL, 'An error occurred while attempting to add the user.', NULL, NULL);
('error_modifying_sec_record', 'isc_modify_user', 'alt.c', NULL, 0, 435, NULL, 'An error occurred while attempting to modify the user record.', NULL, NULL);
('error_deleting_sec_record', 'isc_delete_user', 'alt.c', NULL, 0, 436, NULL, 'An error occurred while attempting to delete the user record.', NULL, NULL);
('error_updating_sec_db', 'isc_add_user', 'alt.c', NULL, 0, 437, NULL, 'An error occurred while updating the security database.', NULL, NULL);
('sort_rec_size_err', 'gen_sort', 'opt.c', NULL, 0, 438, NULL, 'sort record size of @1 bytes is too big', NULL, NULL);
('bad_default_value', 'define_field', 'ddl.c', NULL, 0, 439, NULL, 'can not define a not null column with NULL as default value', NULL, NULL);
('invalid_clause', 'define_field', 'ddl.c', NULL, 0, 440, NULL, 'invalid clause --- ''@1''', NULL, NULL);
('too_many_handles', 'get_id()', 'remote/server.c', NULL, 0, 441, NULL, 'too many open handles to database', NULL, NULL);
('optimizer_blk_exc', 'OPT_compile', 'opt.c', NULL, 0, 442, NULL, 'size of optimizer block exceeded', NULL, NULL);
('invalid_string_constant', 'yylex', 'dsql/parse.y', NULL, 0, 443, NULL, 'a string constant is delimited by double quotes', NULL, NULL);
('transitional_date', 'yylex', 'dsql/parse.y', NULL, 0, 444, NULL, 'DATE must be changed to TIMESTAMP', NULL, NULL);
('read_only_database', 'RLCK_reserve_relation', 'rlck.c', NULL, 0, 445, NULL, 'attempted update on read-only database', NULL, NULL);
('must_be_dialect_2_and_up', 'createDatabase', 'jrd.c', NULL, 0, 446, NULL, 'SQL dialect @1 is not supported in this database', NULL, NULL);
('blob_filter_exception', 'BLF_put_segment', 'blf.e', NULL, 0, 447, NULL, 'A fatal exception occurred during the execution of a blob filter.', NULL, NULL);
('exception_access_violation', 'ISC_exception_post', 'isc_sync.c', NULL, 0, 448, NULL, 'Access violation.  The code attempted to access a virtual address without privilege to do so.', NULL, NULL);
('exception_datatype_missalignment', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 449, NULL, 'Datatype misalignment.  The attempted to read or write a value that was not stored on a memory boundary.', NULL, NULL);
('exception_array_bounds_exceeded', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 450, NULL, 'Array bounds exceeded.  The code attempted to access an array element that is out of bounds.', NULL, NULL);
('exception_float_denormal_operand', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 451, NULL, 'Float denormal operand.  One of the floating-point operands is too small to represent a standard float value.', NULL, NULL);
('exception_float_divide_by_zero', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 452, NULL, 'Floating-point divide by zero.  The code attempted to divide a floating-point value by zero.', NULL, NULL);
('exception_float_inexact_result', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 453, NULL, 'Floating-point inexact result.  The result of a floating-point operation cannot be represented as a deciaml fraction.', NULL, NULL);
('exception_float_invalid_operand', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 454, NULL, 'Floating-point invalid operand.  An indeterminant error occurred during a floating-point operation.', NULL, NULL);
('exception_float_overflow', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 455, NULL, 'Floating-point overflow.  The exponent of a floating-point operation is greater than the magnitude allowed.', NULL, NULL);
('exception_float_stack_check', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 456, NULL, 'Floating-point stack check.  The stack overflowed or underflowed as the result of a floating-point operation.', NULL, NULL);
('exception_float_underflow', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 457, NULL, 'Floating-point underflow.  The exponent of a floating-point operation is less than the magnitude allowed.', NULL, NULL);
('exception_integer_divide_by_zero', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 458, NULL, 'Integer divide by zero.  The code attempted to divide an integer value by an integer divisor of zero.', NULL, NULL);
('exception_integer_overflow', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', NULL, 0, 459, NULL, 'Integer overflow.  The result of an integer operation caused the most significant bit of the result to carry.', NULL, NULL);
('exception_unknown', 'POST_EXCEPTIONS and POST_EXTRENA', 'common.h', 'obsolete', 0, 460, NULL, 'An exception occurred that does not have a description.  Exception number @1.', NULL, NULL);
('exception_stack_overflow', 'POST_EXCEPTION', 'common.h', NULL, 0, 461, NULL, 'Stack overflow.  The resource requirements of the runtime stack have exceeded the memory available to it.', NULL, NULL);
('exception_sigsegv', 'ISC_exception_post', 'isc_sync.c', NULL, 0, 462, NULL, 'Segmentation Fault. The code attempted to access memory without priviledges.', NULL, NULL);
('exception_sigill', 'ISC_exception_post', 'isc_sync.c', NULL, 0, 463, NULL, 'Illegal Instruction. The Code attempted to perfrom an illegal operation.', NULL, NULL);
('exception_sigbus', 'ISC_exception_post', 'isc_sync.c', NULL, 0, 464, NULL, 'Bus Error. The Code caused a system bus error.', NULL, NULL);
('exception_sigfpe', 'ISC_exception_post', 'isc_sync.c', NULL, 0, 465, NULL, 'Floating Point Error. The Code caused an Arithmetic Exception or a floating point exception.', NULL, NULL);
('ext_file_delete', 'EXT_erase', 'ext.c', NULL, 0, 466, NULL, 'Cannot delete rows from external files.', NULL, NULL);
('ext_file_modify', 'EXT_modify', 'ext.c', NULL, 0, 467, NULL, 'Cannot update rows in external files.', NULL, NULL);
('adm_task_denied', 'GDS_ATTACH_DATABASE', 'jrd.c', NULL, 0, 468, NULL, 'Unable to perform operation.  You must be either SYSDBA or owner of the database', NULL, NULL);
('extract_input_mismatch', 'looper', 'evl.c', NULL, 0, 469, NULL, 'Specified EXTRACT part does not exist in input datatype', NULL, NULL);
('insufficient_svc_privileges', 'SVC_query', 'svc.c', NULL, 0, 470, NULL, 'Service @1 requires SYSDBA permissions.  Reattach to the Service Manager using the SYSDBA account.', NULL, NULL);
('file_in_use', 'SVC_query', 'svc.c', NULL, 0, 471, NULL, 'The file @1 is currently in use by another process.  Try again later.', NULL, NULL);
('service_att_err', 'gds_service_attach', 'remote/interface.c', NULL, 0, 472, NULL, 'Cannot attach to services manager', NULL, NULL);
('ddl_not_allowed_by_db_sql_dial', 'prepare', 'dsql.c', NULL, 0, 473, NULL, 'Metadata update statement is not allowed by the current database SQL dialect @1', NULL, NULL);
('cancelled', 'JRD_reschedule', 'jrd.c', NULL, 0, 474, NULL, 'operation was cancelled', NULL, NULL);
('unexp_spb_form', 'process_switches', 'svc.c', NULL, 0, 475, NULL, 'unexpected item in service parameter block, expected @1', NULL, NULL);
('sql_dialect_datatype_unsupport', NULL, 'dsql/pass1.c', NULL, 0, 476, NULL, 'Client SQL dialect @1 does not support reference to @2 datatype', NULL, NULL);
('svcnouser', 'SVC_attach', 'svc.c', NULL, 0, 477, NULL, 'user name and password are required while attaching to the services manager', NULL, NULL);
('depend_on_uncommitted_rel', 'PAR_make_field', 'par.c', NULL, 0, 478, NULL, 'You created an indirect dependency on uncommitted metadata. You must roll back the current transaction.', NULL, NULL);
('svc_name_missing', 'isc_service_attach', 'why.c', NULL, 0, 479, NULL, 'The service name was not specified.', NULL, NULL);
('too_many_contexts', NULL, 'cmp.c', NULL, 0, 480, NULL, 'Too many Contexts of Relation/Procedure/Views. Maximum allowed is 256', NULL, NULL);
('datype_notsup', 'CMP_get_desc', 'cmp.c', NULL, 0, 481, NULL, 'data type not supported for arithmetic', NULL, NULL);
('dialect_reset_warning', 'PAG_set_db_SQL_dialect', 'pag.c', NULL, 0, 482, NULL, 'Database dialect being changed from 3 to 1', NULL, NULL);
('dialect_not_changed', 'PAG_set_db_sql_dialect', 'pag.c', NULL, 0, 483, NULL, 'Database dialect not changed.', NULL, NULL);
('database_create_failed', 'createDatabase', 'jrd.c', NULL, 0, 484, NULL, 'Unable to create database @1', NULL, NULL);
('inv_dialect_specified', NULL, 'pag.c', NULL, 0, 485, NULL, 'Database dialect @1 is not a valid dialect.', NULL, NULL);
('valid_db_dialects', 'createDatabase', 'jrd.c', NULL, 0, 486, NULL, 'Valid database dialects are @1.', NULL, NULL);
('sqlwarn', NULL, NULL, NULL, 0, 487, NULL, 'SQL warning code = @1', NULL, NULL);
('dtype_renamed', 'yylex', 'dsql/parse.y', NULL, 0, 488, NULL, 'DATE data type is now called TIMESTAMP', NULL, NULL);
('extern_func_dir_error', 'ISC_lookup_entrypoint', 'flu.c', NULL, 0, 489, NULL, 'Function @1 is in @2, which is not in a permitted directory for external functions.', NULL, NULL);
('date_range_exceeded', 'add_sql_date', 'evl.c', NULL, 0, 490, NULL, 'value exceeds the range for valid dates', NULL, NULL);
('inv_client_dialect_specified', 'GDS_ATTACH_DATABASE', 'jrd.c', NULL, 0, 491, NULL, 'passed client dialect @1 is not a valid dialect.', NULL, NULL);
('valid_client_dialects', 'GDS_ATTACH_DATABASE', 'jrd.c', NULL, 0, 492, NULL, 'Valid client dialects are @1.', NULL, NULL);
('optimizer_between_err', 'decompose', 'OPT.C', NULL, 0, 493, NULL, 'Unsupported field type specified in BETWEEN predicate.', NULL, NULL);
('service_not_supported', 'SVC_attach', 'svc.c', NULL, 0, 494, NULL, 'Services functionality will be supported in a later version  of the product', NULL, NULL);
('generator_name', 'check_dependencies', 'dfw.e', NULL, 0, 495, NULL, 'GENERATOR @1', NULL, NULL);
('udf_name', 'check_dependencies', 'dfw.e', NULL, 0, 496, NULL, 'UDF @1', NULL, NULL);
('bad_limit_param', 'RSE_open', 'rse.c', NULL, 0, 497, NULL, 'Invalid parameter to FIRST.  Only integers >= 0 are allowed.', NULL, NULL);
('bad_skip_param', 'RSE_open', 'rse.c', NULL, 0, 498, NULL, 'Invalid parameter to SKIP.  Only integers >= 0 are allowed.', NULL, NULL);
('io_32bit_exceeded_err', 'seek_file', 'unix.c', NULL, 0, 499, NULL, 'File exceeded maximum size of 2GB.  Add another database file or use a 64 bit I/O version of Firebird.', NULL, NULL);
('invalid_savepoint', 'looper', 'exe.cpp', NULL, 0, 500, NULL, 'Unable to find savepoint with name @1 in transaction context', NULL, NULL);
('dsql_column_pos_err', '(several)', 'pass1.cpp', NULL, 0, 501, NULL, 'Invalid column position used in the @1 clause', NULL, NULL);
('dsql_agg_where_err', 'pass1_rse', 'pass1.cpp', NULL, 0, 502, NULL, 'Cannot use an aggregate or window function in a WHERE clause, use HAVING (for aggregate only) instead', NULL, NULL);
('dsql_agg_group_err', 'pass1_rse', 'pass1.cpp', NULL, 0, 503, NULL, 'Cannot use an aggregate or window function in a GROUP BY clause', NULL, NULL);
('dsql_agg_column_err', 'pass1_rse', 'pass1.cpp', NULL, 0, 504, NULL, 'Invalid expression in the @1 (not contained in either an aggregate function or the GROUP BY clause)', NULL, NULL);
('dsql_agg_having_err', 'pass1_rse', 'pass1.cpp', NULL, 0, 505, NULL, 'Invalid expression in the @1 (neither an aggregate function nor a part of the GROUP BY clause)', NULL, NULL);
('dsql_agg_nested_err', 'invalid_reference', 'pass1.cpp', NULL, 0, 506, NULL, 'Nested aggregate and window functions are not allowed', NULL, NULL);
('exec_sql_invalid_arg', '(several)', '(several)', NULL, 0, 507, NULL, 'Invalid argument in EXECUTE STATEMENT - cannot convert to string', NULL, NULL);
('exec_sql_invalid_req', '(several)', 'dsql.cpp', NULL, 0, 508, NULL, 'Wrong request type in EXECUTE STATEMENT ''@1''', NULL, NULL);
('exec_sql_invalid_var', '(several)', NULL, NULL, 0, 509, NULL, 'Variable type (position @1) in EXECUTE STATEMENT ''@2'' INTO does not match returned column type', NULL, NULL);
('exec_sql_max_call_exceeded', '(several)', NULL, NULL, 0, 510, NULL, 'Too many recursion levels of EXECUTE STATEMENT', NULL, NULL);
('conf_access_denied', '(several)', 'dir_list.cpp', NULL, 0, 511, NULL, 'Access to @1 "@2" is denied by server administrator', NULL, NULL);
('wrong_backup_state', NULL, 'dfw.epp', NULL, 0, 512, NULL, 'Cannot change difference file name while database is in backup mode', NULL, NULL);
('wal_backup_err', 'begin_backup', 'dfw.epp', NULL, 0, 513, NULL, 'Physical backup is not allowed while Write-Ahead Log is in use', NULL, NULL);
('cursor_not_open', NULL, 'exe.cpp', NULL, 0, 514, NULL, 'Cursor is not open', NULL, NULL);
('bad_shutdown_mode', 'SHUT_database', 'shut.cpp', NULL, 0, 515, NULL, 'Target shutdown mode is invalid for database "@1"', NULL, NULL);
('concat_overflow', NULL, 'evl.cpp', NULL, 0, 516, NULL, 'Concatenation overflow. Resulting string cannot exceed 32765 bytes in length.', NULL, NULL);
('bad_substring_offset', NULL, 'evl.cpp', NULL, 0, 517, NULL, 'Invalid offset parameter @1 to SUBSTRING. Only positive integers are allowed.', NULL, NULL);
('foreign_key_target_doesnt_exist', 'check_partner_index', 'IDX.C', NULL, 0, 518, NULL, 'Foreign key reference target does not exist', NULL, NULL);
('foreign_key_references_present', 'check_partner_index', 'IDX.C', NULL, 0, 519, NULL, 'Foreign key references are present for the record', NULL, NULL);
('no_update', 'several', 'dfw.epp', NULL, 0, 520, NULL, 'cannot update', NULL, NULL);
('cursor_already_open', NULL, 'exe.epp', NULL, 0, 521, NULL, 'Cursor is already open', NULL, NULL);
('stack_trace', 'looper', 'exe.epp', NULL, 0, 522, NULL, '@1', NULL, NULL);
('ctx_var_not_found', NULL, 'functions.cpp', NULL, 0, 523, NULL, 'Context variable @1 is not found in namespace @2', NULL, NULL);
('ctx_namespace_invalid', NULL, 'functions.cpp', NULL, 0, 524, NULL, 'Invalid namespace name @1 passed to @2', NULL, NULL);
('ctx_too_big', NULL, 'functions.cpp', NULL, 0, 525, NULL, 'Too many context variables', NULL, NULL);
('ctx_bad_argument', NULL, 'functions.cpp', NULL, 0, 526, NULL, 'Invalid argument passed to @1', NULL, NULL);
('identifier_too_long', NULL, 'par.cpp', NULL, 0, 527, NULL, 'BLR syntax error. Identifier @1... is too long', NULL, NULL);
('except2', 'looper', 'exe.cpp', NULL, 0, 528, NULL, 'exception @1', NULL, 'unused');
('malformed_string', NULL, NULL, NULL, 0, 529, NULL, 'Malformed string', NULL, NULL);
('prc_out_param_mismatch', 'par_procedure', 'par.c', NULL, 0, 530, NULL, 'Output parameter mismatch for procedure @1', NULL, NULL);
('command_end_err2', 'yyerror', 'dsql parse.cpp', NULL, 0, 531, NULL, 'Unexpected end of command - line @1, column @2', NULL, NULL);
('partner_idx_incompat_type', NULL, NULL, NULL, 0, 532, NULL, 'partner index segment no @1 has incompatible data type', NULL, NULL);
('bad_substring_length', NULL, 'evl.cpp', NULL, 0, 533, NULL, 'Invalid length parameter @1 to SUBSTRING. Negative integers are not allowed.', NULL, NULL);
('charset_not_installed', NULL, NULL, NULL, 0, 534, NULL, 'CHARACTER SET @1 is not installed', NULL, NULL);
('collation_not_installed', NULL, NULL, NULL, 0, 535, NULL, 'COLLATION @1 for CHARACTER SET @2 is not installed', NULL, NULL);
('att_shutdown', NULL, 'jrd.cpp', NULL, 0, 536, NULL, 'connection shutdown', NULL, NULL);
('blobtoobig', NULL, 'blb.cpp', NULL, 0, 537, NULL, 'Maximum BLOB size exceeded', NULL, NULL);
('must_have_phys_field', 'make_version', 'dfw.epp', NULL, 0, 538, NULL, 'Can''t have relation with only computed fields or constraints', NULL, NULL);
('invalid_time_precision', NULL, 'par.cpp', NULL, 0, 539, NULL, 'Time precision exceeds allowed range (0-@1)', NULL, NULL);
('blob_convert_error', 'blb::move', 'blb.cpp', NULL, 0, 540, NULL, 'Unsupported conversion to target type BLOB (subtype @1)', NULL, NULL);
('array_convert_error', 'blb::move', 'blb.cpp', NULL, 0, 541, NULL, 'Unsupported conversion to target type ARRAY', NULL, NULL);
('record_lock_not_supp', 'RSE_get_record', 'rse.cpp', NULL, 0, 542, NULL, 'Stream does not support record locking', NULL, NULL);
('partner_idx_not_found', NULL, NULL, NULL, 0, 543, NULL, 'Cannot create foreign key constraint @1. Partner index does not exist or is inactive.', NULL, NULL);
('tra_num_exc', 'bump_transaction_id', 'tra.cpp', NULL, 0, 544, NULL, 'Transactions count exceeded. Perform backup and restore to make database operable again', NULL, NULL);
('field_disappeared', 'EVL_assign_to', 'evl.cpp', NULL, 0, 545, NULL, 'Column has been unexpectedly deleted', NULL, NULL);
('met_wrong_gtt_scope', 'store_dependencies', 'met.e', NULL, 0, 546, NULL, '@1 cannot depend on @2', NULL, NULL);
('subtype_for_internal_use', 'DDL_resolve_intl_type', 'DSQL/ddl.cpp', NULL, 0, 547, NULL, 'Blob sub_types bigger than 1 (text) are for internal use only', NULL, NULL);
('illegal_prc_type', 'par_procedure', 'par.cpp', NULL, 0, 548, NULL, 'Procedure @1 is not selectable (it does not contain a SUSPEND statement)', NULL, NULL);
('invalid_sort_datatype', 'gen_sort', 'opt.cpp', NULL, 0, 549, NULL, 'Datatype @1 is not supported for sorting operation', NULL, NULL);
('collation_name', 'check_dependencies', 'dfw.e', NULL, 0, 550, NULL, 'COLLATION @1', NULL, NULL);
('domain_name', 'check_dependencies', 'dfw.epp', NULL, 0, 551, NULL, 'DOMAIN @1', NULL, NULL);
('domnotdef', NULL, NULL, NULL, 0, 552, NULL, 'domain @1 is not defined', NULL, NULL);
('array_max_dimensions', 'scalar', 'evl.cpp', NULL, 0, 553, NULL, 'Array data type can use up to @1 dimensions', NULL, NULL);
('max_db_per_trans_allowed', 'GDS_START_TRANSACTION', 'jrd.cpp', NULL, 0, 554, NULL, 'A multi database transaction cannot span more than @1 databases', NULL, NULL);
('bad_debug_format', 'DBG_parse_debug_info', 'DebugInterface.cpp', NULL, 0, 555, NULL, 'Bad debug info format', NULL, NULL);
('bad_proc_BLR', 'MET_procedure', 'met.cpp', NULL, 0, 556, NULL, 'Error while parsing procedure @1''s BLR', NULL, NULL);
('key_too_big', 'IDX_create_index', 'idx.cpp', NULL, 0, 557, NULL, 'index key too big', NULL, NULL);
('concurrent_transaction', NULL, 'vio.cpp', NULL, 0, 558, NULL, 'concurrent transaction number is @1', NULL, NULL);
COMMIT WORK;
-- Do not change the arguments of the previous JRD messages.
-- Write the new JRD messages here.
('not_valid_for_var', 'EVL_validate', 'evl.cpp', NULL, 0, 559, NULL, 'validation error for variable @1, value "@2"', NULL, NULL);
('not_valid_for', 'EVL_validate', 'evl.cpp', NULL, 0, 560, NULL, 'validation error for @1, value "@2"', NULL, NULL);
('need_difference', 'BackupManager::begin_backup', 'nbak.cpp', NULL, 0, 561, NULL, 'Difference file name should be set explicitly for database on raw device', NULL, NULL);
('long_login', 'getUserInfo', 'jrd.cpp', NULL, 0, 562, NULL, 'Login name too long (@1 characters, maximum allowed @2)', NULL, NULL);
('fldnotdef2', NULL, NULL, NULL, 0, 563, NULL, 'column @1 is not defined in procedure @2', NULL, 'An undefined field was referenced in blr.');
('invalid_similar_pattern', 'SimilarToMatcher::Evaluator', 'SimilarToMatcher.h', NULL, 0, 564, NULL, 'Invalid SIMILAR TO pattern', NULL, NULL);
('bad_teb_form', 'GDS_START_MULTIPLE', 'why.cpp', NULL, 0, 565, NULL, 'Invalid TEB format', NULL, NULL);
('tpb_multiple_txn_isolation', 'transaction_options', 'tra.cpp', NULL, 0, 566, NULL, 'Found more than one transaction isolation in TPB', NULL, NULL)
('tpb_reserv_before_table', 'transaction_options', 'tra.cpp', NULL, 0, 567, NULL, 'Table reservation lock type @1 requires table name before in TPB', NULL, NULL)
('tpb_multiple_spec', 'transaction_options', 'tra.cpp', NULL, 0, 568, NULL, 'Found more than one @1 specification in TPB', NULL, NULL)
('tpb_option_without_rc', 'transaction_options', 'tra.cpp', NULL, 0, 569, NULL, 'Option @1 requires READ COMMITTED isolation in TPB', NULL, NULL)
('tpb_conflicting_options', 'transaction_options', 'tra.cpp', NULL, 0, 570, NULL, 'Option @1 is not valid if @2 was used previously in TPB', NULL, NULL)
('tpb_reserv_missing_tlen', 'transaction_options', 'tra.cpp', NULL, 0, 571, NULL, 'Table name length missing after table reservation @1 in TPB', NULL, NULL)
('tpb_reserv_long_tlen', 'transaction_options', 'tra.cpp', NULL, 0, 572, NULL, 'Table name length @1 is too long after table reservation @2 in TPB', NULL, NULL)
('tpb_reserv_missing_tname', 'transaction_options', 'tra.cpp', NULL, 0, 573, NULL, 'Table name length @1 without table name after table reservation @2 in TPB', NULL, NULL)
('tpb_reserv_corrup_tlen', 'transaction_options', 'tra.cpp', NULL, 0, 574, NULL, 'Table name length @1 goes beyond the remaining TPB size after table reservation @2', NULL, NULL)
('tpb_reserv_null_tlen', 'transaction_options', 'tra.cpp', NULL, 0, 575, NULL, 'Table name length is zero after table reservation @1 in TPB', NULL, NULL)
('tpb_reserv_relnotfound', 'transaction_options', 'tra.cpp', NULL, 0, 576, NULL, 'Table or view @1 not defined in system tables after table reservation @2 in TPB', NULL, NULL)
('tpb_reserv_baserelnotfound', 'expand_view_lock', 'tra.cpp', NULL, 0, 577, NULL, 'Base table or view @1 for view @2 not defined in system tables after table reservation @3 in TPB', NULL, NULL)
('tpb_missing_len', 'transaction_options', 'tra.cpp', NULL, 0, 578, NULL, 'Option length missing after option @1 in TPB', NULL, NULL)
('tpb_missing_value', 'transaction_options', 'tra.cpp', NULL, 0, 579, NULL, 'Option length @1 without value after option @2 in TPB', NULL, NULL)
('tpb_corrupt_len', 'transaction_options', 'tra.cpp', NULL, 0, 580, NULL, 'Option length @1 goes beyond the remaining TPB size after option @2', NULL, NULL)
('tpb_null_len', 'transaction_options', 'tra.cpp', NULL, 0, 581, NULL, 'Option length is zero after table reservation @1 in TPB', NULL, NULL)
('tpb_overflow_len', 'transaction_options', 'tra.cpp', NULL, 0, 582, NULL, 'Option length @1 exceeds the range for option @2 in TPB', NULL, NULL)
('tpb_invalid_value', 'transaction_options', 'tra.cpp', NULL, 0, 583, NULL, 'Option value @1 is invalid for the option @2 in TPB', NULL, NULL)
('tpb_reserv_stronger_wng', 'expand_view_lock', 'tra.cpp', NULL, 0, 584, NULL, 'Preserving previous table reservation @1 for table @2, stronger than new @3 in TPB', NULL, NULL)
('tpb_reserv_stronger', 'expand_view_lock', 'tra.cpp', NULL, 0, 585, NULL, 'Table reservation @1 for table @2 already specified and is stronger than new @3 in TPB', NULL, NULL)
('tpb_reserv_max_recursion', 'expand_view_lock', 'tra.cpp', NULL, 0, 586, NULL, 'Table reservation reached maximum recursion of @1 when expanding views in TPB', NULL, NULL)
('tpb_reserv_virtualtbl', 'expand_view_lock', 'tra.cpp', NULL, 0, 587, NULL, 'Table reservation in TPB cannot be applied to @1 because it''s a virtual table', NULL, NULL)
('tpb_reserv_systbl', 'expand_view_lock', 'tra.cpp', NULL, 0, 588, NULL, 'Table reservation in TPB cannot be applied to @1 because it''s a system table', NULL, NULL)
('tpb_reserv_temptbl', 'expand_view_lock', 'tra.cpp', NULL, 0, 589, NULL, 'Table reservation @1 or @2 in TPB cannot be applied to @3 because it''s a temporary table', NULL, NULL)
('tpb_readtxn_after_writelock', 'transaction_options', 'tra.cpp', NULL, 0, 590, NULL, 'Cannot set the transaction in read only mode after a table reservation isc_tpb_lock_write in TPB', NULL, NULL)
('tpb_writelock_after_readtxn', 'transaction_options', 'tra.cpp', NULL, 0, 591, NULL, 'Cannot take a table reservation isc_tpb_lock_write in TPB because the transaction is in read only mode', NULL, NULL)
('time_range_exceeded', 'EXE_assignment', 'evl.cpp', NULL, 0, 592, NULL, 'value exceeds the range for a valid time', NULL, NULL)
('datetime_range_exceeded', 'add_sql_timestamp', 'evl.cpp', NULL, 0, 593, NULL, 'value exceeds the range for valid timestamps', NULL, NULL)
('string_truncation', 'CVT_move', 'cvt.cpp', NULL, 0, 594, NULL, 'string right truncation', NULL, NULL)
('blob_truncation', 'move_to_string', 'blb.cpp', NULL, 0, 595, NULL, 'blob truncation when converting to a string: length limit exceeded', NULL, NULL)
('numeric_out_of_range', NULL, NULL, NULL, 0, 596, NULL, 'numeric value is out of range', NULL, NULL)
('shutdown_timeout', NULL, NULL, NULL, 0, 597, NULL, 'Firebird shutdown is still in progress after the specified timeout', NULL, NULL)
('att_handle_busy', NULL, NULL, NULL, 0, 598, NULL, 'Attachment handle is busy', NULL, NULL)
('bad_udf_freeit', NULL, NULL, NULL, 0, 599, NULL, 'Bad written UDF detected: pointer returned in FREE_IT function was not allocated by ib_util_malloc', NULL, NULL)
('eds_provider_not_found', NULL, 'ExtDS.cpp', NULL, 0, 600, NULL, 'External Data Source provider ''@1'' not found', NULL, NULL)
('eds_connection', NULL, 'ExtDS.cpp', NULL, 0, 601, NULL, 'Execute statement error at @1 :
@2Data source : @3', NULL, NULL)
('eds_preprocess', NULL, 'ExtDS.cpp', NULL, 0, 602, NULL, 'Execute statement preprocess SQL error', NULL, NULL)
('eds_stmt_expected', NULL, 'ExtDS.cpp', NULL, 0, 603, NULL, 'Statement expected', NULL, NULL)
('eds_prm_name_expected', NULL, 'ExtDS.cpp', NULL, 0, 604, NULL, 'Parameter name expected', NULL, NULL)
('eds_unclosed_comment', NULL, 'ExtDS.cpp', NULL, 0, 605, NULL, 'Unclosed comment found near ''@1''', NULL, NULL)
('eds_statement', NULL, 'ExtDS.cpp', NULL, 0, 606, NULL, 'Execute statement error at @1 :
@2Statement : @3
Data source : @4', NULL, NULL)
('eds_input_prm_mismatch', NULL, 'ExtDS.cpp', NULL, 0, 607, NULL, 'Input parameters mismatch', NULL, NULL)
('eds_output_prm_mismatch', NULL, 'ExtDS.cpp', NULL, 0, 608, NULL, 'Output parameters mismatch', NULL, NULL)
('eds_input_prm_not_set', NULL, 'ExtDS.cpp', NULL, 0, 609, NULL, 'Input parameter ''@1'' have no value set', NULL, NULL)
('too_big_blr', 'end_blr', 'ddl.cpp', NULL, 0, 610, NULL, 'BLR stream length @1 exceeds implementation limit @2', NULL, NULL)
('montabexh', 'acquire', 'DatabaseSnapshot.cpp', NULL, 0, 611, NULL, 'Monitoring table space exhausted', NULL, NULL)
('modnotfound', 'par_function', 'par.cpp', NULL, 0, 612, NULL, 'module name or entrypoint could not be found', NULL, NULL)
('nothing_to_cancel', 'FB_CANCEL_OPERATION', 'why.cpp', NULL, 0, 613, NULL, 'nothing to cancel', NULL, NULL)
('ibutil_not_loaded', NULL, NULL, NULL, 0, 614, NULL, 'ib_util library has not been loaded to deallocate memory returned by FREE_IT function', NULL, NULL)
('circular_computed', NULL, NULL, NULL, 0, 615, NULL, 'Cannot have circular dependencies with computed fields', NULL, NULL)
('psw_db_error', NULL, NULL, NULL, 0, 616, NULL, 'Security database error', NULL, NULL)
-- These add more information to the dumb isc_expression_eval_err
('invalid_type_datetime_op', 'add_datetime', 'evl.cpp', NULL, 0, 617, NULL, 'Invalid data type in DATE/TIME/TIMESTAMP addition or subtraction in add_datettime()', NULL, NULL)
('onlycan_add_timetodate', 'add_timestamp', 'evl.cpp', NULL, 0, 618, NULL, 'Only a TIME value can be added to a DATE value', NULL, NULL)
('onlycan_add_datetotime', 'add_timestamp', 'evl.cpp', NULL, 0, 619, NULL, 'Only a DATE value can be added to a TIME value', NULL, NULL)
('onlycansub_tstampfromtstamp', 'add_timestamp', 'evl.cpp', NULL, 0, 620, NULL, 'TIMESTAMP values can be subtracted only from another TIMESTAMP value', NULL, NULL)
('onlyoneop_mustbe_tstamp', 'add_timestamp', 'evl.cpp', NULL, 0, 621, NULL, 'Only one operand can be of type TIMESTAMP', NULL, NULL)
('invalid_extractpart_time', 'extract', 'evl.cpp', NULL, 0, 622, NULL, 'Only HOUR, MINUTE, SECOND and MILLISECOND can be extracted from TIME values', NULL, NULL)
('invalid_extractpart_date', 'extract', 'evl.cpp', NULL, 0, 623, NULL, 'HOUR, MINUTE, SECOND and MILLISECOND cannot be extracted from DATE values', NULL, NULL)
('invalidarg_extract', 'extract', 'evl.cpp', NULL, 0, 624, NULL, 'Invalid argument for EXTRACT() not being of DATE/TIME/TIMESTAMP type', NULL, NULL)
('sysf_argmustbe_exact', 'makeBin/makeBinShift', 'evl.cpp', NULL, 0, 625, NULL, 'Arguments for @1 must be integral types or NUMERIC/DECIMAL without scale', NULL, NULL)
('sysf_argmustbe_exact_or_fp', 'makeRound', 'evl.cpp', NULL, 0, 626, NULL, 'First argument for @1 must be integral type or floating point type', NULL, NULL)
('sysf_argviolates_uuidtype', 'evlCharToUuid', 'SysFunction.cpp', NULL, 0, 627, NULL, 'Human readable UUID argument for @1 must be of string type', NULL, NULL)
('sysf_argviolates_uuidlen', 'evlCharToUuid', 'SysFunction.cpp', NULL, 0, 628, NULL, 'Human readable UUID argument for @2 must be of exact length @1', NULL, NULL)
('sysf_argviolates_uuidfmt', 'evlCharToUuid', 'SysFunction.cpp', NULL, 0, 629, NULL, 'Human readable UUID argument for @3 must have "-" at position @2 instead of "@1"', NULL, NULL)
('sysf_argviolates_guidigits', 'evlCharToUuid', 'SysFunction.cpp', NULL, 0, 630, NULL, 'Human readable UUID argument for @3 must have hex digit at position @2 instead of "@1"', NULL, NULL)
('sysf_invalid_addpart_time', 'evlDateAdd', 'SysFunction.cpp', NULL, 0, 631, NULL, 'Only HOUR, MINUTE, SECOND and MILLISECOND can be added to TIME values in @1', NULL, NULL)
('sysf_invalid_add_datetime', 'evlDateAdd', 'SysFunction.cpp', NULL, 0, 632, NULL, 'Invalid data type in addition of part to DATE/TIME/TIMESTAMP in @1', NULL, NULL)
('sysf_invalid_addpart_dtime', 'evlDateAdd', 'SysFunction.cpp', NULL, 0, 633, NULL, 'Invalid part @1 to be added to a DATE/TIME/TIMESTAMP value in @2', NULL, NULL)
('sysf_invalid_add_dtime_rc', 'evlDateAdd', 'SysFunction.cpp', NULL, 0, 634, NULL, 'Expected DATE/TIME/TIMESTAMP type in evlDateAdd() result', NULL, NULL)
('sysf_invalid_diff_dtime', 'evlDateDiff', 'SysFunction.cpp', NULL, 0, 635, NULL, 'Expected DATE/TIME/TIMESTAMP type as first and second argument to @1', NULL, NULL)
('sysf_invalid_timediff', 'evlDateDiff', 'SysFunction.cpp', NULL, 0, 636, NULL, 'The result of TIME-<value> in @1 cannot be expressed in YEAR, MONTH, DAY or WEEK', NULL, NULL)
('sysf_invalid_tstamptimediff', 'evlDateDiff', 'SysFunction.cpp', NULL, 0, 637, NULL, 'The result of TIME-TIMESTAMP or TIMESTAMP-TIME in @1 cannot be expressed in HOUR, MINUTE, SECOND or MILLISECOND', NULL, NULL)
('sysf_invalid_datetimediff', 'evlDateDiff', 'SysFunction.cpp', NULL, 0, 638, NULL, 'The result of DATE-TIME or TIME-DATE in @1 cannot be expressed in HOUR, MINUTE, SECOND and MILLISECOND', NULL, NULL)
('sysf_invalid_diffpart', 'evlDateDiff', 'SysFunction.cpp', NULL, 0, 639, NULL, 'Invalid part @1 to express the difference between two DATE/TIME/TIMESTAMP values in @2', NULL, NULL)
('sysf_argmustbe_positive', 'evlLnLog10/evlLog', 'SysFunction.cpp', NULL, 0, 640, NULL, 'Argument for @1 must be positive', NULL, NULL)
('sysf_basemustbe_positive', 'evlLog/evlPosition', 'SysFunction.cpp', NULL, 0, 641, NULL, 'Base for @1 must be positive', NULL, NULL)
('sysf_argnmustbe_nonneg', 'evlOverlay/evlPad', 'SysFunction.cpp', NULL, 0, 642, NULL, 'Argument #@1 for @2 must be zero or positive', NULL, NULL)
('sysf_argnmustbe_positive', 'evlOverlay', 'SysFunction.cpp', NULL, 0, 643, NULL, 'Argument #@1 for @2 must be positive', NULL, NULL)
('sysf_invalid_zeropowneg', 'evlPower', 'SysFunction.cpp', NULL, 0, 644, NULL, 'Base for @1 cannot be zero if exponent is negative', NULL, NULL)
('sysf_invalid_negpowfp', 'evlPower', 'SysFunction.cpp', NULL, 0, 645, NULL, 'Base for @1 cannot be negative if exponent is not an integral value', NULL, NULL)
-- Validate that arg is integer?
('sysf_invalid_scale', 'evlRound/evlTrunc', 'SysFunction.cpp', NULL, 0, 646, NULL, 'The numeric scale must be between -128 and 127 in @1', NULL, NULL)
('sysf_argmustbe_nonneg', 'evlSqrt', 'SysFunction.cpp', NULL, 0, 647, NULL, 'Argument for @1 must be zero or positive', NULL, NULL)
('sysf_binuuid_mustbe_str', 'evlUuidToChar', 'SysFunction.cpp', NULL, 0, 648, NULL, 'Binary UUID argument for @1 must be of string type', NULL, NULL)
('sysf_binuuid_wrongsize', 'evlUuidToChar', 'SysFunction.cpp', NULL, 0, 649, NULL, 'Binary UUID argument for @2 must use @1 bytes', NULL, NULL)
-- End of extras for isc_expression_eval_err
('missing_required_spb', 'process_switches', 'svc.cpp', NULL, 0, 650, NULL, 'Missing required item @1 in service parameter block', NULL, NULL)
('net_server_shutdown', NULL, NULL, NULL, 0, 651, NULL, '@1 server is shutdown', NULL, NULL)
('bad_conn_str', NULL, NULL, NULL, 0, 652, NULL, 'Invalid connection string', NULL, NULL);
('bad_epb_form', 'EventManager::queEvents', 'event.cpp', NULL, 0, 653, NULL, 'Unrecognized events block', NULL, NULL);
('no_threads', 'Worker::start', 'server.cpp', NULL, 0, 654, NULL, 'Could not start first worker thread - shutdown server', NULL, NULL);
('net_event_connect_timeout', 'aux_connect', 'inet.cpp', NULL, 0, 655, NULL, 'Timeout occurred while waiting for a secondary connection for event processing', NULL, NULL);
-- More extras for isc_expression_eval_err
('sysf_argmustbe_nonzero', 'evlStdMath', 'SysFunction.cpp', NULL, 0, 656, NULL, 'Argument for @1 must be different than zero', NULL, NULL);
('sysf_argmustbe_range_inc1_1', 'evlStdMath', 'SysFunction.cpp', NULL, 0, 657, NULL, 'Argument for @1 must be in the range [-1, 1]', NULL, NULL);
('sysf_argmustbe_gteq_one', 'evlStdMath', 'SysFunction.cpp', NULL, 0, 658, NULL, 'Argument for @1 must be greater or equal than one', NULL, NULL);
('sysf_argmustbe_range_exc1_1', 'evlStdMath', 'SysFunction.cpp', NULL, 0, 659, NULL, 'Argument for @1 must be in the range ]-1, 1[', NULL, NULL);
-- End of extras for isc_expression_eval_err
('internal_rejected_params', NULL, 'inf.cpp', NULL, 0, 660, NULL, 'Incorrect parameters provided to internal function @1', NULL, NULL);
('sysf_fp_overflow', 'evlStdMath', 'SysFunction.cpp', NULL, 0, 661, NULL, 'Floating point overflow in built-in function @1', NULL, NULL);
('udf_fp_overflow', 'FUN_evaluate', 'fun.epp', NULL, 0, 662, NULL, 'Floating point overflow in result from UDF @1', NULL, NULL);
('udf_fp_nan', 'FUN_evaluate', 'fun.epp', NULL, 0, 663, NULL, 'Invalid floating point value returned by UDF @1', NULL, NULL);
('instance_conflict', 'ISC_map_file', 'isc_sync.cpp', NULL, 0, 664, NULL, 'Database is probably already opened by another engine instance in another Windows session', NULL, NULL);
('out_of_temp_space', 'setupFile', 'TempSpace.cpp', NULL, 0, 665, NULL, 'No free space found in temporary directories', NULL, NULL);
('eds_expl_tran_ctrl', NULL, '', NULL, 0, 666, NULL, 'Explicit transaction control is not allowed', NULL, NULL)
('no_trusted_spb', NULL, 'svc.cpp', NULL, 0, 667, NULL, 'Use of TRUSTED switches in spb_command_line is prohibited', NULL, NULL)
('package_name', 'check_dependencies', 'dfw.epp', NULL, 0, 668, NULL, 'PACKAGE @1', NULL, NULL);
('cannot_make_not_null', 'check_not_null', 'dfw.epp', NULL, 0, 669, NULL, 'Cannot make field @1 of table @2 NOT NULL because there are NULLs present', NULL, NULL);
('feature_removed', NULL, '', NULL, 0, 670, NULL, 'Feature @1 is not supported anymore', NULL, NULL);
('view_name', 'check_dependencies', 'dfw.epp', NULL, 0, 671, NULL, 'VIEW @1', NULL, NULL);
('lock_dir_access', 'createLockDirectory', 'os_utils.cpp', NULL, 0, 672, NULL, 'Can not access lock files directory @1', NULL, NULL);
('invalid_fetch_option', NULL, 'Cursor.cpp', NULL, 0, 673, NULL, 'Fetch option @1 is invalid for a non-scrollable cursor', NULL, NULL);
('bad_fun_BLR', 'loadMetadata', 'Function.epp', NULL, 0, 674, NULL, 'Error while parsing function @1''s BLR', NULL, NULL);
('func_pack_not_implemented', 'execute', 'Function.epp', NULL, 0, 675, NULL, 'Cannot execute function @1 of the unimplemented package @2', NULL, NULL);
('proc_pack_not_implemented', 'EXE_start', 'exe.cpp', NULL, 0, 676, NULL, 'Cannot execute procedure @1 of the unimplemented package @2', NULL, NULL);
('eem_func_not_returned', 'makeFunction', 'ExtEngineManager.cpp', NULL, 0, 677, NULL, 'External function @1 not returned by the external engine plugin @2', NULL, NULL)
('eem_proc_not_returned', 'makeProcedure', 'ExtEngineManager.cpp', NULL, 0, 678, NULL, 'External procedure @1 not returned by the external engine plugin @2', NULL, NULL)
('eem_trig_not_returned', 'makeTrigger', 'ExtEngineManager.cpp', NULL, 0, 679, NULL, 'External trigger @1 not returned by the external engine plugin @2', NULL, NULL)
('eem_bad_plugin_ver', 'getEngine', 'ExtEngineManager.cpp', NULL, 0, 680, NULL, 'Incompatible plugin version @1 for external engine @2', NULL, NULL)
('eem_engine_notfound', 'getEngine', 'ExtEngineManager.cpp', NULL, 0, 681, NULL, 'External engine @1 not found', NULL, NULL)
('attachment_in_use', 'GDS_DETACH/GDS_DROP_DATABASE', 'jrd.cpp', NULL, 0, 682, NULL, 'Attachment is in use', NULL, NULL)
('transaction_in_use', 'commit/prepare/rollback', 'jrd.cpp', NULL, 0, 683, NULL, 'Transaction is in use', NULL, NULL)
('pman_plugin_notfound', 'PluginManager::getPlugin', 'PluginManager.cpp', NULL, 0, 684, NULL, 'Plugin @1 not found', NULL, NULL)
('pman_cannot_load_plugin', 'PluginManager::getPlugin', 'PluginManager.cpp', NULL, 0, 685, NULL, 'Module @1 exists, but can not be loaded', NULL, NULL)
('pman_entrypoint_notfound', 'PluginManager::getPlugin', 'PluginManager.cpp', NULL, 0, 686, NULL, 'Entrypoint of plugin @1 does not exist', NULL, NULL)
('pman_bad_conf_index', 'PluginImpl::getConfigInfo', 'PluginManager.cpp', NULL, 0, 687, NULL, 'Invalid value @1 for parameter index at PluginImpl::getConfigInfo: out of bounds', NULL, NULL)
('pman_unknown_instance', 'PluginImpl::getExternalEngineFactory', 'PluginManager.cpp', NULL, 0, 688, NULL, 'Plugin @1 does not create @2 instances', NULL, NULL)
('sysf_invalid_trig_namespace', 'evlGetContext', 'SysFunction.cpp', NULL, 0, 689, NULL, 'Invalid usage of context namespace DDL_TRIGGER', NULL, NULL)
('unexpected_null', 'ValueMover::getValue', 'ValueImpl.cpp', NULL, 0, 690, NULL, 'Value is NULL but isNull parameter was not informed', NULL, NULL)
('type_notcompat_blob', 'ValueImpl::getBlobId', 'ValueImpl.cpp', NULL, 0, 691, NULL, 'Type @1 is incompatible with BLOB', NULL, NULL)
('invalid_date_val', 'ValueImpl::setDate', 'ValueImpl.cpp', NULL, 0, 692, NULL, 'Invalid date', NULL, NULL)
('invalid_time_val', 'ValueImpl::setTime', 'ValueImpl.cpp', NULL, 0, 693, NULL, 'Invalid time', NULL, NULL)
('invalid_timestamp_val', 'ValueImpl::setTimeStamp', 'ValueImpl.cpp', NULL, 0, 694, NULL, 'Invalid timestamp', NULL, NULL)
('invalid_index_val', NULL, NULL, NULL, 0, 695, NULL, 'Invalid index @1 in function @2', NULL, NULL)
('formatted_exception', 'ExceptionNode::setError', 'StmtNodes.cpp', NULL, 0, 696, NULL, '@1', NULL, NULL)
('async_active', 'REM_cancel_operation', 'interface.cpp', NULL, 0, 697, NULL, 'Asynchronous call is already running for this attachment', NULL, NULL)
('private_function', 'METD_get_function', 'metd.epp', NULL, 0, 698, NULL, 'Function @1 is private to package @2', NULL, NULL)
('private_procedure', 'METD_get_procedure', 'metd.epp', NULL, 0, 699, NULL, 'Procedure @1 is private to package @2', NULL, NULL)
('request_outdated', 'EVL_expr', 'evl.cpp', NULL, 0, 700, NULL, 'Request can''t access new records in relation @1 and should be recompiled', NULL, NULL)
('bad_events_handle', NULL, NULL, NULL, 0, 701, NULL, 'invalid events id (handle)', NULL, NULL);
('cannot_copy_stmt', NULL, NULL, NULL, 0, 702, NULL, 'Cannot copy statement @1', NULL, NULL);
('invalid_boolean_usage', NULL, NULL, NULL, 0, 703, NULL, 'Invalid usage of boolean expression', NULL, NULL);
('sysf_argscant_both_be_zero', 'evlAtan2', 'SysFunction.cpp', NULL, 0, 704, NULL, 'Arguments for @1 cannot both be zero', NULL, NULL)
('spb_no_id', 'Service::start', 'svc.c', NULL, 0, 705, NULL, 'missing service ID in spb', NULL, NULL);
('ee_blr_mismatch_null', NULL, 'met.epp', NULL, 0, 706, NULL, 'External BLR message mismatch: invalid null descriptor at field @1', NULL, NULL)
('ee_blr_mismatch_length', NULL, 'met.epp', NULL, 0, 707, NULL, 'External BLR message mismatch: length = @1, expected @2', NULL, NULL)
('ss_out_of_bounds', NULL, 'sdl.cpp', NULL, 0, 708, NULL, 'Subscript @1 out of bounds [@2, @3]', NULL, NULL)
('missing_data_structures', NULL, 'server.cpp', NULL, 0, 709, NULL, 'Install incomplete, please read chapter "Initializing security database" in Quick Start Guide', NULL, NULL)
('protect_sys_tab', NULL, 'vio.cpp', NULL, 0, 710, NULL, '@1 operation is not allowed for system table @2', NULL, NULL)
('libtommath_generic', 'check', 'BigInteger.cpp', NULL, 0, 711, NULL, 'Libtommath error code @1 in function @2', NULL, NULL)
('wroblrver2', NULL, NULL, NULL, 0, 712, NULL, 'unsupported BLR version (expected between @1 and @2, encountered @3)', NULL, NULL);
('trunc_limits', NULL, NULL, NULL, 0, 713, NULL, 'expected length @1, actual @2', NULL, NULL);
('info_access', NULL, 'svc.cpp', NULL, 0, 714, NULL, 'Wrong info requested in isc_svc_query() for anonymous service', NULL, NULL);
('svc_no_stdin', NULL, 'svc.cpp', NULL, 0, 715, NULL, 'No isc_info_svc_stdin in user request, but service thread requested stdin data', NULL, NULL);
('svc_start_failed', NULL, 'svc.cpp', NULL, 0, 716, NULL, 'Start request for anonymous service is impossible', NULL, NULL);
('svc_no_switches', NULL, 'svc.cpp', NULL, 0, 717, NULL, 'All services except for getting server log require switches', NULL, NULL);
('svc_bad_size', NULL, 'svc.cpp', NULL, 0, 718, NULL, 'Size of stdin data is more than was requested from client', NULL, NULL);
('no_crypt_plugin', NULL, 'CryptoManager.cpp', NULL, 0, 719, NULL, 'Crypt plugin @1 failed to load', NULL, NULL);
('cp_name_too_long', NULL, 'CryptoManager.cpp', NULL, 0, 720, NULL, 'Length of crypt plugin name should not exceed @1 bytes', NULL, NULL);
('cp_process_active', NULL, 'CryptoManager.cpp', NULL, 0, 721, NULL, 'Crypt failed - already crypting database', NULL, NULL);
('cp_already_crypted', NULL, 'CryptoManager.cpp', NULL, 0, 722, NULL, 'Crypt failed - database is already in requested state', NULL, NULL);
('decrypt_error', NULL, 'CryptoManager.cpp', NULL, 0, 723, NULL, 'Missing crypt plugin, but page appears encrypted', NULL, NULL);
('no_providers', NULL, 'why.cpp', NULL, 0, 724, NULL, 'No providers loaded', NULL, NULL);
('null_spb', NULL, 'why.cpp', NULL, 0, 725, NULL, 'NULL data with non-zero SPB length', NULL, NULL);
('max_args_exceeded', NULL, 'ExprNodes.cpp', NULL, 0, 726, NULL, 'Maximum (@1) number of arguments exceeded for function @2', NULL, NULL)
('ee_blr_mismatch_names_count', NULL, 'ExtEngineManager.cpp', NULL, 0, 727, NULL, 'External BLR message mismatch: names count = @1, blr count = @2', NULL, NULL)
('ee_blr_mismatch_name_not_found', NULL, 'ExtEngineManager.cpp', NULL, 0, 728, NULL, 'External BLR message mismatch: name @1 not found', NULL, NULL)
('bad_result_set', NULL, 'why.cpp', NULL, 0, 729, NULL, 'Invalid resultset interface', NULL, NULL);
('wrong_message_length', NULL, 'why.cpp', NULL, 0, 730, NULL, 'Message length passed from user application does not match set of columns', NULL, NULL);
('no_output_format', NULL, 'why.cpp', NULL, 0, 731, NULL, 'Resultset is missing output format information', NULL, NULL);
('item_finish', NULL, 'MsgMetadata.cpp', NULL, 0, 732, NULL, 'Message metadata not ready - item @1 is not finished', NULL, NULL);
('miss_config', NULL, 'config_file.cpp', NULL, 0, 733, NULL, 'Missing configuration file: @1', NULL, NULL);
('conf_line', NULL, 'config_file.cpp', NULL, 0, 734, NULL, '@1: illegal line <@2>', NULL, NULL);
('conf_include', NULL, 'config_file.cpp', NULL, 0, 735, NULL, 'Invalid include operator in @1 for <@2>', NULL, NULL);
('include_depth', NULL, 'config_file.cpp', NULL, 0, 736, NULL, 'Include depth too big', NULL, NULL);
('include_miss', NULL, 'config_file.cpp', NULL, 0, 737, NULL, 'File to include not found', NULL, NULL);
('protect_ownership', 'check_owner', 'vio.cpp', NULL, 0, 738, NULL, 'Only the owner can change the ownership', NULL, NULL);
('badvarnum', NULL, NULL, NULL, 0, 739, NULL, 'undefined variable number', NULL, NULL);
('sec_context', 'getUserInfo', 'jrd.cpp', NULL, 0, 740, NULL, 'Missing security context for @1', NULL, NULL);
('multi_segment', 'getMultiPartConnectParameter', 'server.cpp', NULL, 0, 741, NULL, 'Missing segment @1 in multisegment connect block parameter', NULL, NULL);
('login_changed', 'ServerAuth::ServerAuth', 'server.cpp', NULL, 0, 742, NULL, 'Different logins in connect and attach packets - client library error', NULL, NULL);
('auth_handshake_limit', 'ServerAuth::authenticate', 'server.cpp', NULL, 0, 743, NULL, 'Exceeded exchange limit during authentication handshake', NULL, NULL);
('wirecrypt_incompatible', 'requiredEncryption', 'server.cpp', NULL, 0, 744, NULL, 'Incompatible wire encryption levels requested on client and server', NULL, NULL);
('miss_wirecrypt', NULL, 'server.cpp', NULL, 0, 745, NULL, 'Client attempted to attach unencrypted but wire encryption is required', NULL, NULL);
('wirecrypt_key', 'start_crypt', 'server.cpp', NULL, 0, 746, NULL, 'Client attempted to start wire encryption using unknown key @1', NULL, NULL);
('wirecrypt_plugin', 'start_crypt', 'server.cpp', NULL, 0, 747, NULL, 'Client attempted to start wire encryption using unsupported plugin @1', NULL, NULL);
('secdb_name', NULL, NULL, NULL, 0, 748, NULL, 'Error getting security database name from configuration file', NULL, NULL);
('auth_data', NULL, NULL, NULL, 0, 749, NULL, 'Client authentication plugin is missing required data from server', NULL, NULL);
('auth_datalength', NULL, NULL, NULL, 0, 750, NULL, 'Client authentication plugin expected @2 bytes of @3 from server, got @1', NULL, NULL);
('info_unprepared_stmt', NULL, NULL, NULL, 0, 751, NULL, 'Attempt to get information about an unprepared dynamic SQL statement.', NULL, NULL);
('idx_key_value', NULL, NULL, NULL, 0, 752, NULL, 'Problematic key value is @1', NULL, NULL);
('forupdate_virtualtbl', 'PAR_rse', 'par.cpp', NULL, 0, 753, NULL, 'Cannot select virtual table @1 for update WITH LOCK', NULL, NULL)
('forupdate_systbl', 'PAR_rse', 'par.cpp', NULL, 0, 754, NULL, 'Cannot select system table @1 for update WITH LOCK', NULL, NULL)
('forupdate_temptbl', 'PAR_rse', 'par.cpp', NULL, 0, 755, NULL, 'Cannot select temporary table @1 for update WITH LOCK', NULL, NULL)
('cant_modify_sysobj', NULL, 'ExprNodes.cpp', NULL, 0, 756, NULL, 'System @1 @2 cannot be modified', NULL, 'Ex: System generator rdb$... cannot be modified')
('server_misconfigured', 'expandDatabaseName', 'db_alias.cpp', NULL, 0, 757, NULL, 'Server misconfigured - contact administrator please', NULL, NULL);
('alter_role', 'MappingNode::validateAdmin', 'DdlNodes.epp', NULL, 0, 758, NULL, 'Deprecated backward compatibility ALTER ROLE ... SET/DROP AUTO ADMIN mapping may be used only for RDB$ADMIN role', NULL, NULL);
('map_already_exists', 'MappingNode::execute', 'DdlNodes.epp', NULL, 0, 759, NULL, 'Mapping @1 already exists', NULL, NULL);
('map_not_exists', 'MappingNode::execute', 'DdlNodes.epp', NULL, 0, 760, NULL, 'Mapping @1 does not exist', NULL, NULL);
('map_load', 'check', 'Mapping.cpp', NULL, 0, 761, NULL, '@1 failed when loading mapping cache', NULL, NULL);
('map_aster', 'Cache::map', 'Mapping.cpp', NULL, 0, 762, NULL, 'Invalid name <*> in authentication block', NULL, NULL);
('map_multi', 'Cache::search', 'Mapping.cpp', NULL, 0, 763, NULL, 'Multiple maps found for @1', NULL, NULL);
('map_undefined', 'Found::set', 'Mapping.cpp', NULL, 0, 764, NULL, 'Undefined mapping result - more than one different results found', NULL, NULL);
('baddpb_damaged_mode', 'attachDatabase', 'jrd.cpp', NULL, 0, 765, NULL, 'Incompatible mode of attachment to damaged database', NULL, NULL);
('baddpb_buffers_range', 'DatabaseOptions::get', 'jrd.cpp', NULL, 0, 766, NULL, 'Attempt to set in database number of buffers which is out of acceptable range [@1:@2]', NULL, NULL);
('baddpb_temp_buffers', 'DatabaseOptions::get', 'jrd.cpp', NULL, 0, 767, NULL, 'Attempt to temporarily set number of buffers less than @1', NULL, NULL);
('map_nodb', 'MappingList::getList', 'Mapping.cpp', NULL, 0, 768, NULL, 'Global mapping is not available when database @1 is not present', NULL, NULL);
('map_notable', 'MappingList::getList', 'Mapping.cpp', NULL, 0, 769, NULL, 'Global mapping is not available when table RDB$MAP is not present in database @1', NULL, NULL);
('miss_trusted_role', 'SetRoleNode::execute', 'StmtNodes.cpp', NULL, 0, 770, NULL, 'Your attachment has no trusted role', NULL, NULL);
('set_invalid_role', 'SetRoleNode::execute', 'StmtNodes.cpp', NULL, 0, 771, NULL, 'Role @1 is invalid or unavailable', NULL, NULL);
-- QLI
(NULL, NULL, NULL, NULL, 1, 0, NULL, 'expected type', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 1, NULL, 'bad block type', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 2, NULL, 'bad block size', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 3, NULL, 'corrupt pool', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 4, NULL, 'bad pool ID', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 5, NULL, 'memory exhausted', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 6, NULL, 'set option not implemented', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 7, NULL, 'show option not implemented', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 8, NULL, 'show_fields: dtype not done', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 9, NULL, 'INTERNAL: @1', NULL, NULL);
(NULL, 'ERRQ_database_error', 'err.c', NULL, 1, 10, NULL, '** QLI error from database "@1" **', NULL, NULL);
(NULL, 'ERRQ_database_error', 'err.c', NULL, 1, 11, NULL, '** QLI error from database **', NULL, NULL);
(NULL, 'ERRQ_error', 'err.c', NULL, 1, 12, NULL, '** QLI error: @1 **', NULL, NULL);
(NULL, 'ERRQ_syntax', 'err.c', NULL, 1, 13, NULL, 'expected @1, encountered "@2"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 14, NULL, 'integer overflow', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 15, NULL, 'integer division by zero', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 16, NULL, 'floating overflow trap', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 17, NULL, 'floating division by zero', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 18, NULL, 'floating underflow trap', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 19, NULL, 'floating overflow fault', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 20, NULL, 'floating underflow fault', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 21, NULL, 'arithmetic exception', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 22, NULL, 'illegal instruction or address, recovering...', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 23, NULL, 'Please retry, supplying an application script file name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 24, NULL, 'Welcome to QLI
Query Language Interpreter', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 25, NULL, 'qli version @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 26, NULL, '
Statistics for database "@1"
@2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 27, NULL, 'HSH_remove failed', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 28, NULL, 'EVAL_boolean: not finished', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 29, NULL, 'EVAL_value: not finished', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 30, NULL, 'data type not supported for arithmetic', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 31, NULL, 'user name is supported in RSEs temporarily', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 32, NULL, 'Input value is too long', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 33, NULL, 'EXEC_execute: not implemented', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 34, NULL, 'print_blob: expected field node', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 35, NULL, 'output pipe is not supported on VMS', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 36, NULL, 'could not create pipe', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 37, NULL, 'fdopen failed', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 38, NULL, 'execution terminated by signal', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 39, NULL, 'field validation error', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 40, NULL, 'Request terminated by statement: @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 41, NULL, 'Request terminated by statement', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 42, NULL, 'Cannot open output file "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 43, NULL, 'Could not run "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 44, NULL, 'comparison not done', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 45, NULL, 'conversion not implemented', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 46, NULL, 'conversion not implemented', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 47, NULL, 'MOVQ_move: conversion not done', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 48, NULL, 'BLOB conversion is not supported', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 49, NULL, 'conversion error', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 50, NULL, 'conversion error', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 51, NULL, 'conversion error', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 52, NULL, 'conversion error', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 53, NULL, 'conversion error', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 54, NULL, 'conversion error', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 55, NULL, 'BLOB conversion is not supported', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 56, NULL, 'Error converting string "@1" to date', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 57, NULL, 'overflow during conversion', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 58, NULL, 'gds_$put_segment failed', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 59, NULL, 'fseek failed', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 60, NULL, 'unterminated quoted string', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 61, NULL, 'could not open scratch file', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 62, NULL, 'fseek failed', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 63, NULL, 'unterminated quoted string', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 64, NULL, 'unexpected end of procedure in procedure @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 65, NULL, 'unexpected end of file in file @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 66, NULL, 'unexpected eof', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 67, NULL, 'cannot open command file "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 68, NULL, 'PIC_edit: class not yet implemented', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 69, NULL, 'conversion error', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 70, NULL, 'procedure "@1" is undefined', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 71, NULL, 'procedure "@1" is undefined in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 72, NULL, 'procedure "@1" is undefined', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 73, NULL, 'Could not create QLI$PROCEDURE_NAME field', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 74, NULL, 'Could not create QLI$PROCEDURE field', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 75, NULL, 'Could not create QLI$PROCEDURES table', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 76, NULL, 'procedure name "@1" in use in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 77, NULL, 'database handle required', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 78, NULL, 'QLI$PROCEDURES table must be created with RDO in Rdb/VMS databases', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 79, NULL, 'procedure name over 31 characters', NULL, NULL);
(NULL, 'print_topic', 'help.e', NULL, 1, 80, NULL, '	[@1 topics matched @2]', NULL, NULL);
(NULL, 'print_topic', 'help.e', NULL, 1, 81, NULL, '
@1Sub-topics available:', NULL, NULL);
(NULL, 'print_topic', 'help.e', NULL, 1, 82, NULL, '
No help is available for @1 @2', NULL, NULL);
(NULL, 'print_topic', 'help.e', NULL, 1, 83, NULL, '
Sub-topics available for @1 are:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 84, NULL, 'Procedures can not be renamed across databases. Try COPY', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 85, NULL, 'Procedure @1 not found in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 86, NULL, 'substitute prompt string too long', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 87, NULL, 'substitute prompt string too long', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 88, NULL, 'Procedure @1 not found in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 89, NULL, 'Procedure @1 not found in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 90, NULL, 'No security classes defined', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 91, NULL, '	Security class @1 is not defined', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 92, NULL, '	No views defined', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 93, NULL, '	No indexes defined', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 94, NULL, '	No indexes defined', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 95, NULL, 'No databases are currently ready', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 96, NULL, 'Procedure @1 in database "@2" (@3)', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 97, NULL, 'text, length @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 98, NULL, 'varying text, length @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 99, NULL, 'null terminated text, length @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 100, NULL, 'short binary', NULL, 'no longer in use (1/7/94)');
(NULL, NULL, NULL, NULL, 1, 101, NULL, 'long binary', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 102, NULL, 'quad', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 103, NULL, 'short floating', NULL, 'no longer in use (1/7/94)');
(NULL, NULL, NULL, NULL, 1, 104, NULL, 'long floating', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 105, NULL, 'BLOB', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 106, NULL, ', segment length @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 107, NULL, ', subtype @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 108, NULL, ', subtype BLR', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 109, NULL, ', subtype ACL', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 110, NULL, 'date', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 111, NULL, ', scale @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 112, NULL, ', subtype fixed', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 113, NULL, 'Database "@1" readied as @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 114, NULL, 'Database "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 115, NULL, 'No databases are currently ready', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 116, NULL, '    Page size is @1 bytes.  Current allocation is @2 pages.', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 117, NULL, 'Field @1 does not exist in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 118, NULL, 'Field @1 does not exist in any open database', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 119, NULL, ' (computed expression)', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 120, NULL, 'There are no forms defined for database @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 121, NULL, 'There are no forms defined in any open database', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 122, NULL, 'Global field @1 does not exist in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 123, NULL, 'Global field @1 does not exist in any open database', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 124, NULL, 'There are no fields defined for database @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 125, NULL, 'There are no fields defined in any open database', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 126, NULL, 'Procedure @1 not found in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 127, NULL, 'Procedure @1 not found', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 128, NULL, 'Procedures in database "@1" (@2):', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 129, NULL, 'No triggers are defined for table @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 130, NULL, 'No triggers are defined in database @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 131, NULL, 'No triggers are defined in database @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 132, NULL, 'Variables:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 133, NULL, '    QLI, version "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 134, NULL, '    Version(s) for database "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 135, NULL, 'expand_expression: not yet implemented', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 136, NULL, 'expand_statement: not yet implemented', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 137, NULL, 'variables may not be based on BLOB fields', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 138, NULL, 'cannot perform assignment to computed field @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 139, NULL, 'no context for ERASE', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 140, NULL, 'cannot erase from a join', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 141, NULL, '@1.* cannot be used when a single element is required', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 142, NULL, '"@1" is undefined or used out of context', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 143, NULL, 'no default form name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 144, NULL, 'No database for form @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 145, NULL, 'form @1 is not defined in database "@2"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 146, NULL, 'no context for form ACCEPT statement', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 147, NULL, 'field @1 is not defined in form @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 148, NULL, 'no context for modify', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 149, NULL, 'field list required for modify', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 150, NULL, 'No items in print list', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 151, NULL, 'No items in print list', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 152, NULL, 'invalid ORDER BY ordinal', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 153, NULL, 'asterisk expressions require exactly one qualifying context', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 154, NULL, 'unrecognized context', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 155, NULL, 'field referenced in BASED ON cannot be resolved against readied databases', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 156, NULL, 'expected statement, encountered "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 157, NULL, 'Expected PROCEDURE encountered "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 158, NULL, 'period in qualified name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 159, NULL, 'no databases are ready', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 160, NULL, 'BLOB variables are not supported', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 161, NULL, 'end of statement', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 162, NULL, 'end of command', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 163, NULL, 'quoted edit string', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 164, NULL, 'variable definition clause', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 165, NULL, '@1 is not a database', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 166, NULL, '@1 is not a table in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 167, NULL, 'variable data type', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 168, NULL, 'no data type may be specified for a variable based on a field', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 169, NULL, 'object type for DEFINE', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 170, NULL, 'table name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 171, NULL, 'comma between field definitions', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 172, NULL, 'FROM', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 173, NULL, 'table or view name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 174, NULL, '[', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 175, NULL, ']', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 176, NULL, 'No statements issued yet', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 177, NULL, 'ON or TO', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 178, NULL, 'quoted edit string', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 179, NULL, 'column definition clause', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 180, NULL, 'global fields may not be based on other fields', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 181, NULL, 'field name or asterisk expression', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 182, NULL, 'FROM RSE clause', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 183, NULL, 'comma', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 184, NULL, 'quoted header segment', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 185, NULL, 'left parenthesis', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 186, NULL, 'comma or terminating right parenthesis', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 187, NULL, 'left parenthesis', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 188, NULL, 'VALUES list or SELECT clause', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 189, NULL, 'the number of values do not match the number of fields', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 190, NULL, 'value expression', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 191, NULL, 'right parenthesis', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 192, NULL, 'quoted string', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 193, NULL, 'ENTREE or END', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 194, NULL, 'quoted string', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 195, NULL, 'index state option', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 196, NULL, 'table name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 197, NULL, 'ADD, MODIFY, or DROP', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 198, NULL, 'comma between field definitions', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 199, NULL, 'identifier', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 200, NULL, 'positive number', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 201, NULL, 'FORM', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 202, NULL, 'period in qualified table name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 203, NULL, '@1 is not a table in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 204, NULL, 'database file name required on READY', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 205, NULL, 'EXISTS (SELECT * <sql rse>)', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 206, NULL, 'relational operator', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 207, NULL, 'a database has not been readied', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 208, NULL, 'expected "table_name", encountered "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 209, NULL, 'table name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 210, NULL, 'PROCEDURE', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 211, NULL, 'TOP or BOTTOM', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 212, NULL, 'report writer SET option', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 213, NULL, 'report item', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 214, NULL, 'set option', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 215, NULL, 'RELATIONS or TRIGGERS', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 216, NULL, 'table name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 217, NULL, 'database name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 218, NULL, 'table name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 219, NULL, 'database name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 220, NULL, 'table name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 221, NULL, 'database name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 222, NULL, 'table name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 223, NULL, 'table name', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 224, NULL, 'FROM clause', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 225, NULL, 'AVG, MAX, MIN, SUM, or COUNT', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 226, NULL, 'COUNT (*)', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 227, NULL, 'left parenthesis', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 228, NULL, 'OF', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 229, NULL, 'database handle', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 230, NULL, 'SET', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 231, NULL, 'database block not found for removal', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 232, NULL, 'show_fields: dtype not done', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 233, NULL, 'global field @1 already exists', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 234, NULL, 'Cannot define an index in a view', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 235, NULL, 'Index @1 already exists', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 236, NULL, 'Column @1 does not occur in table @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 237, NULL, 'Table @1 already exists', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 238, NULL, 'Field @1 is in use in the following relations:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 239, NULL, 'Field @1 is in use in database "@2"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 240, NULL, 'Field @1 is not defined in database "@2"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 241, NULL, 'Index @1 is not defined in database "@2"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 242, NULL, 'metadata operation failed', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 243, NULL, 'no active database for operation', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 244, NULL, 'Interactive metadata updates are not available on Rdb', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 245, NULL, 'global field @1 is not defined', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 246, NULL, 'Index @1 does not exist in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 247, NULL, 'field @1 does not exist', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 248, NULL, 'no active database for operation', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 249, NULL, 'Interactive metadata updates are not available on Rdb', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 250, NULL, 'Unlicensed for database "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 251, NULL, 'Field @1 already exists in relation @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 252, NULL, 'data type cannot be changed locally', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 253, NULL, 'global field @1 does not exist', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 254, NULL, 'field @1 not found in relation @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 255, NULL, 'Data type conflict with existing global field @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 256, NULL, 'No data type specified for field @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 257, NULL, 'database info call failed', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 258, NULL, 'do not understand BLR operator @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 259, NULL, 'Operation unlicensed for database "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 260, NULL, '    Security class for database @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 261, NULL, '    Database description:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 262, NULL, '    Database description:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 263, NULL, '	File:	@1 starting at page @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 264, NULL, 'Field @1 in @2 @3 of database @4', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 265, NULL, '    Global field @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 266, NULL, '    Field description:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 267, NULL, '    Datatype information:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 268, NULL, '    Field is computed from:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 269, NULL, '    Field validation:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 270, NULL, '    Security class @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 271, NULL, '    Query name:	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 272, NULL, '    Query name:	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 273, NULL, '    Edit string:	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 274, NULL, '    Edit string:	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 275, NULL, '    Query header:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 276, NULL, 'Global field @1 in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 277, NULL, '    Field description:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 278, NULL, '    Datatype information:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 279, NULL, '    Field is computed from:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 280, NULL, '    Field validation:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 281, NULL, '    Query name:	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 282, NULL, '    Edit string:	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 283, NULL, '    Query header:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 284, NULL, '    @1 is not used in any relations in database @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 285, NULL, 'Forms in database @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 286, NULL, 'Global fields for database @1:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 287, NULL, '    Field description:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 288, NULL, '        Index @1@2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 289, NULL, '            index @1 is NOT active', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 290, NULL, '        Index @1@2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 291, NULL, '    Description:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 292, NULL, '    Security class @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 293, NULL, '    Stored in external file @1', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 294, NULL, 'OBSOLETE -        An erase trigger is defined for @1', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 295, NULL, 'OBSOLETE -        A modify trigger is defined for @1', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 296, NULL, 'OBSOLETE -        A store trigger is defined for @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 297, NULL, '    Security classes for database @1', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 298, NULL, 'OBSOLETE -	Triggers for relation @1:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 299, NULL, 'OBSOLETE -    Source for the erase trigger is not available.  Trigger BLR:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 300, NULL, 'OBSOLETE -    Erase trigger for relation @1:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 301, NULL, 'OBSOLETE -    Source for the modify trigger is not available.  Trigger BLR:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 302, NULL, 'OBSOLETE -    Modify trigger for relation @1:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 303, NULL, 'OBSOLETE -    Source for the store trigger is not available.  Trigger BLR:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 304, NULL, 'OBSOLETE -    Store trigger for relation @1:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 305, NULL, 'OBSOLETE -    Triggers for relation @1:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 306, NULL, 'OBSOLETE -    Source for the erase trigger is not available.  Trigger BLR:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 307, NULL, 'OBSOLETE -    Erase trigger for relation @1:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 308, NULL, 'OBSOLETE -    Source for the modify trigger is not available.  Trigger BLR:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 309, NULL, 'OBSOLETE -    Modify trigger for relation @1:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 310, NULL, 'OBSOLETE -    Source for the store trigger is not available.  Trigger BLR:', NULL, NULL);
(NULL, NULL, NULL, '
This message is obsolete for version 4.3 and above.  No
need to translate.

1996-Aug-06 David Schnepper', 1, 311, NULL, 'OBSOLETE -    Store trigger for relation @1:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 312, NULL, '
    View source for relation @1 is not available.  View BLR:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 313, NULL, '
    Relation @1 is a view defined as:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 314, NULL, 'Views in database @1:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 315, NULL, '    @1 comprised of :', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 316, NULL, 'Views in database @1:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 317, NULL, '    @1 comprised of:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 318, NULL, 'BLOB', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 319, NULL, ', segment length @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 320, NULL, ', subtype text', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 321, NULL, ', subtype BLR', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 322, NULL, ', subtype ACL', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 323, NULL, ', subtype @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 324, NULL, 'text, length @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 325, NULL, 'varying text, length @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 326, NULL, 'null terminated text, length @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 327, NULL, 'short binary', NULL, 'no longer in use (1/7/94)');
(NULL, NULL, NULL, NULL, 1, 328, NULL, 'long binary', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 329, NULL, 'quadword binary', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 330, NULL, 'short floating', NULL, 'no longer in use (1/7/94)');
(NULL, NULL, NULL, NULL, 1, 331, NULL, 'long floating', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 332, NULL, 'date', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 333, NULL, ', scale @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 334, NULL, ', subtype fixed', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 335, NULL, '    Global field @1 is used in database @2 as :', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 336, NULL, '	@1 in relation @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 337, NULL, 'Field @1 in @2 @3 of database @4', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 338, NULL, '    Global field @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 339, NULL, '    Field description:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 340, NULL, '    Datatype information', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 341, NULL, '    Field is computed from:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 342, NULL, '    Field validation:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 343, NULL, '    Query name:	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 344, NULL, '    Query name:	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 345, NULL, '    Query header:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 346, NULL, '    Edit string:	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 347, NULL, '    Edit string: 	 @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 348, NULL, '@1 Based on field @2 of @3@4', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 349, NULL, '@1Base field description for @2:', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 350, NULL, 'END PROCEDURE', NULL, NULL);
(NULL, 'process_statement', 'dtr.c', NULL, 1, 351, NULL, 'Do you want to roll back your updates?', NULL, NULL);
(NULL, 'gen_descriptor', 'gener.c', NULL, 1, 352, NULL, 'gen_descriptor: dtype not recognized', NULL, NULL);
(NULL, 'gen_expression', 'gener.c', NULL, 1, 353, NULL, 'gen_expression: not understood', NULL, NULL);
(NULL, 'gen_statement', 'gener.c', NULL, 1, 354, NULL, 'gen_statement: not yet implemented', NULL, NULL);
(NULL, 'gen_statistical', 'gener.c', NULL, 1, 355, NULL, 'gen_statistical: not understood', NULL, NULL);
(NULL, 'compile_edit', 'compile.c', NULL, 1, 356, NULL, 'EDIT argument must be a BLOB field', NULL, NULL);
(NULL, 'compile_rse', 'compile.c', NULL, 1, 357, NULL, 'relations from multiple databases in single RSE', NULL, NULL);
(NULL, 'compile_edit', 'compile.c', NULL, 1, 358, NULL, 'cannot find database for BLOB edit', NULL, NULL);
(NULL, 'compile_expression', 'compile.c', NULL, 1, 359, NULL, 'compile_expression: not yet implemented', NULL, NULL);
(NULL, 'compile_statement', 'compile.c', NULL, 1, 360, NULL, 'not yet implemented (compile_statement)', NULL, NULL);
(NULL, 'computable', 'compile.c', NULL, 1, 361, NULL, 'computable: not yet implemented', NULL, NULL);
(NULL, 'make_descriptor', 'compile.c', NULL, 1, 362, NULL, 'make_descriptor: not yet implemented', NULL, NULL);
(NULL, 'make_reference', 'compile.c', NULL, 1, 363, NULL, 'missing message', NULL, NULL);
(NULL, 'release_message', 'compile.c', NULL, 1, 364, NULL, 'lost message', NULL, NULL);
(NULL, 'MET_show_trigger', 'meta.e', NULL, 1, 365, NULL, 'Triggers for relation @1:', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 366, NULL, '    @1	@2, Sequence @3, @4', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 367, NULL, 'Pre-store', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 368, NULL, 'Post-store', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 369, NULL, 'Pre-modify', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 370, NULL, 'Post-modify', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 371, NULL, 'Pre-erase', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 372, NULL, 'Post-erase', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 373, NULL, 'Active', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 374, NULL, 'Inactive', NULL, NULL);
(NULL, 'show_trigger_header', 'meta.e', NULL, 1, 375, NULL, '	Description:', NULL, NULL);
(NULL, 'MET_show_trigger', 'meta.e', NULL, 1, 376, NULL, '	Source for the trigger:', NULL, NULL);
(NULL, 'MET_show_trigger', 'meta.e', NULL, 1, 377, NULL, '	Source for the trigger is not available.  Trigger BLR:', NULL, NULL);
(NULL, 'show_sys_trigs', 'command.c', NULL, 1, 378, NULL, 'No system triggers are defined', NULL, NULL);
(NULL, 'MET_show_system_triggers', 'meta.e', NULL, 1, 379, NULL, 'System Trigger for relation @1', NULL, NULL);
(NULL, 'MET_show_rel_detail', 'meta.e', NULL, 1, 380, NULL, '	Triggers defined for this relation:', NULL, NULL);
(NULL, 'MET_show_triggers', 'meta.e', NULL, 1, 381, NULL, 'Trigger for relation @1:', NULL, NULL);
(NULL, 'parse_report', 'parse.c', NULL, 1, 382, NULL, 'TOP or BOTTOM', NULL, NULL);
(NULL, 'parse_report', 'parse.c', NULL, 1, 383, NULL, 'sort field', NULL, NULL);
(NULL, 'parse_rse', 'parse.c', NULL, 1, 384, NULL, 'Too many WITHs', NULL, NULL);
(NULL, 'MET_show_dbb_detail', 'META.E', NULL, 1, 385, NULL, '	Shadow @1, File: @2 starting at page @3', NULL, NULL);
(NULL, 'parse_sql_create', 'parse.c', NULL, 1, 386, NULL, 'DATABASE, TABLE, or INDEX', NULL, NULL);
(NULL, 'parse_sql_database_create', 'parse.c', NULL, 1, 387, NULL, 'Database filename required in CREATE', NULL, NULL);
(NULL, 'parse_sql_dtype', 'parse.c', NULL, 1, 388, NULL, 'FLOAT', NULL, NULL);
(NULL, 'parse_sql_create', 'parse.c', NULL, 1, 389, NULL, 'INDEX', NULL, NULL);
(NULL, 'parse_sql_database_create', 'parse.c', NULL, 1, 390, NULL, 'Multiple page size specifications', NULL, NULL);
(NULL, 'parse_sql_rse', 'parse.c', NULL, 1, 391, NULL, 'GROUP BY not allowed in view definition', NULL, NULL);
(NULL, 'parse_sql_statistical', 'parse.c', NULL, 1, 392, NULL, 'Aggregates not allowed in view definition', NULL, NULL);
(NULL, 'parse_sql_field', 'parse.c', NULL, 1, 393, NULL, 'NULL', NULL, NULL);
(NULL, 'parse_sql_view_create', 'parse.c', NULL, 1, 394, NULL, 'AS', NULL, NULL);
(NULL, 'parse_sql_view_create', 'parse.c', NULL, 1, 395, NULL, 'SELECT', NULL, NULL);
(NULL, 'parse_sql_database_create', 'parse.c', NULL, 1, 396, NULL, '=', NULL, NULL);
(NULL, 'parse_sql_index_create', 'parse.c', NULL, 1, 397, NULL, 'ON', NULL, NULL);
(NULL, 'parse_sql_grant', 'parse.c', NULL, 1, 398, NULL, 'field name', NULL, NULL);
(NULL, 'parse_sql_grant', 'parse.c', NULL, 1, 399, NULL, 'table name', NULL, NULL);
(NULL, 'parse_sql_grant', 'parse.c', NULL, 1, 400, NULL, 'user name identifier', NULL, NULL);
(NULL, 'parse_sql_grant', 'parse.c', NULL, 1, 401, NULL, 'GRANT', NULL, NULL);
(NULL, 'parse_sql_grant', 'parse.c', NULL, 1, 402, NULL, 'OPTION', NULL, NULL);
(NULL, 'parse_sql_revoke', 'parse.c', NULL, 1, 403, NULL, 'FROM', NULL, NULL);
(NULL, 'parse_sql_grant', 'parse.c', NULL, 1, 404, NULL, 'TO', NULL, NULL);
(NULL, 'parse_sql_alter', 'parse.c', NULL, 1, 405, NULL, 'ADD or DROP', NULL, NULL);
(NULL, 'MET_define_sql_relation', 'meta.e', NULL, 1, 406, NULL, 'Dynamic DDL buffer exceeded', NULL, NULL);
(NULL, 'parse_sql_alter', 'parse.c', NULL, 1, 407, NULL, 'TABLE', NULL, NULL);
(NULL, 'parse_ready', 'parse.c', NULL, 1, 408, NULL, 'Database handle @1 conflicts with an established name', NULL, NULL);
(NULL, 'create_qli_procedrues', 'PROC.E', NULL, 1, 409, NULL, 'Could not create QLI$PROCEDURES index', NULL, NULL);
(NULL, NULL, 'MOV', NULL, 1, 410, NULL, 'Cannot convert from @1 to @2', NULL, NULL);
(NULL, 'MOVQ_decompose', 'MOV', NULL, 1, 411, NULL, 'Cannot convert "@1" to a numeric value', NULL, NULL);
(NULL, 'expand_function', 'EXPAND.C', NULL, 1, 412, NULL, 'function @1 not found in database @2', NULL, NULL);
(NULL, 'clone_global_fields', 'META.E', NULL, 1, 413, NULL, 'Incompatible global field @1 already exists in target database', NULL, NULL);
(NULL, 'MET_sql_alter_table', 'META.E', NULL, 1, 414, NULL, 'Relation @1 is missing or undefined', NULL, NULL);
(NULL, 'CMD_set', 'command.c', NULL, 1, 415, NULL, 'matching language string too long', NULL, NULL);
(NULL, 'show_m_funcs', 'SHOW.E', NULL, 1, 416, NULL, 'Functions in database "@1" (@2):', NULL, NULL);
(NULL, 'show_funcs', 'show.e', NULL, 1, 417, NULL, 'Functions are not supported in database @1.', NULL, NULL);
-- 418 doesn't exist
(NULL, 'show_funcs', 'show.e', NULL, 1, 419, NULL, 'There are no functions defined in any open database.', NULL, NULL);
(NULL, 'show_funcs', 'show.e', NULL, 1, 420, NULL, 'Functions are not supported in any open database.', NULL, NULL);
(NULL, 'show_m_func', 'show.e', NULL, 1, 421, NULL, '    Function description:', NULL, NULL);
(NULL, 'show_func', 'show.e', NULL, 1, 422, NULL, 'Function @1 is not defined in database @2.', NULL, NULL);
(NULL, 'show_func', 'show.e', NULL, 1, 423, NULL, 'Function @1 is not defined in any open database.', NULL, NULL);
(NULL, 'show_m_func', 'show.e', NULL, 1, 424, NULL, 'Function @1 (@2) in database "@3" (@4):', NULL, NULL);
(NULL, 'show_m_func', 'show.e', NULL, 1, 425, NULL, 'Function @1 in database "@2" (@3):', NULL, NULL);
(NULL, 'show_m_func', 'show.e', NULL, 1, 426, NULL, '    Function library is @1', NULL, NULL);
(NULL, 'show_m_func', 'show.e', NULL, 1, 427, NULL, '    Return argument is', NULL, NULL);
(NULL, 'show_m_func', 'show.e', NULL, 1, 428, NULL, '    Argument @1 is', NULL, NULL);
(NULL, 'parse_drop', 'PARSE.C', NULL, 1, 429, NULL, 'database file name required on DROP DATABASE', NULL, NULL);
(NULL, 'MET_delete_database', 'META.E', NULL, 1, 430, NULL, 'Unlicensed for database "@1"', NULL, NULL);
(NULL, 'MET_delete_database', 'META.E', NULL, 1, 431, NULL, 'Could not drop database file "@1"', NULL, NULL);
(NULL, 'MET_delete_database', 'META.E', NULL, 1, 432, NULL, 'Operation unlicensed for database "@1"', NULL, NULL);
(NULL, 'show_datatype', 'SHOW.E', NULL, 1, 433, NULL, ' array', NULL, NULL);
(NULL, 'ALL_alloc', 'ALL.c', NULL, 1, 434, NULL, 'memory pool free list is incorrect', NULL, NULL);
(NULL, 'ALL_release', 'ALL.C', NULL, 1, 435, NULL, 'block released twice', NULL, NULL);
(NULL, 'ALL_release', 'ALL.C', NULL, 1, 436, NULL, 'released block overlaps following free block', NULL, NULL);
(NULL, 'ALL_release', 'ALL.C', NULL, 1, 437, NULL, 'released block overlaps prior free block', NULL, NULL);
(NULL, 'check_for_array', 'EXPAND.C', NULL, 1, 438, NULL, 'References to array fields like @1 in relation @2 are not supported', NULL, NULL);
(NULL, 'show_filter', 'show.e', NULL, 1, 439, NULL, 'Filters are not supported in database @1.', NULL, NULL);
(NULL, 'show_filter', 'show.e', NULL, 1, 440, NULL, 'Filter @1 is not defined in database @2.', NULL, NULL);
(NULL, 'show_filter', 'show.e', NULL, 1, 441, NULL, 'Filter @1 is not defined in any open database.', NULL, NULL);
(NULL, 'show_filter', 'show.e', NULL, 1, 442, NULL, 'Filters are not supported in any open database.', NULL, NULL);
(NULL, 'show_filters', 'show.e', NULL, 1, 443, NULL, 'There are no filters defined in any open database.', NULL, NULL);
(NULL, 'show_m_filter', 'show.e', NULL, 1, 444, NULL, 'Filter @1 in database "@2" (@3):', NULL, NULL);
(NULL, 'show_m_filter', 'show.e', NULL, 1, 445, NULL, '    Filter library is @1', NULL, NULL);
(NULL, 'show_m_filter', 'show.e', NULL, 1, 446, NULL, '    Input sub-type is @1', NULL, NULL);
(NULL, 'show_m_filter', 'show.e', NULL, 1, 447, NULL, '    Output sub-type is @1', NULL, NULL);
(NULL, 'show_m_filter', 'show.e', NULL, 1, 448, NULL, '    Filter description:', NULL, NULL);
(NULL, 'show_m_filters', 'show.e', NULL, 1, 449, NULL, 'Filters in database @1 (@2):', NULL, NULL);
(NULL, 'show_m_indices', 'SHOW.E', NULL, 1, 450, NULL, '	Index @1@2@3@4', NULL, NULL);
(NULL, 'expand_rse', 'EXPAND.C', NULL, 1, 451, NULL, 'simple field reference not allowed in global aggregates', NULL, NULL);
(NULL, 'expand_values', 'EXPAND.C', NULL, 1, 452, NULL, 'prompting not allowed in select field list', NULL, NULL);
(NULL, 'EXEC_open_output', 'exe.c', NULL, 1, 453, NULL, 'output pipe is not supported on MPE/XL', NULL, NULL);
(NULL, 'expand_expression', 'expand.c', NULL, 1, 454, NULL, 'could not resolve context for aggregate expression', NULL, NULL);
(NULL, 'clone_fields', 'META.E', NULL, 1, 455, NULL, 'source relation @1 does not exist', NULL, NULL);
(NULL, 'show_trigger_messages', 'show.e', NULL, 1, 456, NULL, 'Messages associated with @1:', NULL, NULL);
(NULL, 'show_trigger_messages', 'show.e', NULL, 1, 457, NULL, '    message @1:  @2', NULL, NULL);
(NULL, 'ERRQ_database_error', 'err.c', NULL, 1, 458, NULL, 'Connection to database @1 lost.
	Please FINISH the database!', NULL, NULL);
(NULL, 'FORM_lookup_form', 'form', NULL, 1, 459, NULL, 'Unable to create form window', NULL, NULL);
COMMIT WORK;
(NULL, 'process_statement', 'DTR', NULL, 1, 460, NULL, 'Do you want to rollback updates for @1?', NULL, NULL);
(NULL, 'show_funcs', 'show.e', NULL, 1, 461, NULL, 'functions are not supported in database @1', NULL, NULL);
(NULL, 'show_funcs', 'show.e', NULL, 1, 462, NULL, 'no functions are defined in database @1', NULL, NULL);
(NULL, 'show_filts', 'show.e', NULL, 1, 463, NULL, 'filters are not supported in database @1', NULL, NULL);
(NULL, 'show_filts', 'show.e', NULL, 1, 464, NULL, 'no filters are defined for database @1', NULL, NULL);
(NULL, 'CMD_transaction', 'command.c', NULL, 1, 465, NULL, 'Error during two phase commit on database @1.
Roll back all databases or commit databases individually', NULL, NULL);
(NULL, 'expand_expression', 'expand.c', NULL, 1, 466, NULL, 'Only fields may be subscripted', NULL, NULL);
(NULL, 'expand_field', 'expand.c', NULL, 1, 467, NULL, '"@1" is not a field and so may not be subscripted', NULL, NULL);
(NULL, 'MET_modify_field', 'meta.e', NULL, 1, 468, NULL, 'Data type of field @1 may not be changed to or from BLOB', NULL, NULL);
(NULL, 'main', 'dtr.c', NULL, 1, 469, NULL, 'qli: ignoring unknown switch -@1', NULL, 'obsolete, see 529');
(NULL, 'LEX_token', 'LEX', NULL, 1, 470, NULL, 'literal string  <MAXSYMLEN> characters or longer', NULL, NULL);
(NULL, 'show_var', 'show.e', NULL, 1, 471, NULL, 'Variable @1', NULL, NULL);
(NULL, 'show_var', 'show.e', NULL, 1, 472, NULL, '    Query name:	 @1', NULL, NULL);
(NULL, 'show_var', 'show.e', NULL, 1, 473, NULL, '    Edit string:	 @1', NULL, NULL);
(NULL, 'show_var', 'show.e', NULL, 1, 474, NULL, 'Variable @1 has not been declared', NULL, NULL);
(NULL, 'show_var', 'show.e', NULL, 1, 475, NULL, '    Datatype information:', NULL, NULL);
(NULL, 'LEX_get_line', 'lex.c', NULL, 1, 476, NULL, 'input line too long', NULL, NULL);
(NULL, 'get_line', 'lex.c', NULL, 1, 477, NULL, 'input line too long', NULL, NULL);
(NULL, 'parse_print_list', 'parse.c', NULL, 1, 478, NULL, 'number > 0', NULL, NULL);
(NULL, 'array_dimensions', 'SHOW.E', NULL, 1, 479, NULL, ' (@1)', NULL, NULL);
(NULL, 'format_value', 'format.c', NULL, 1, 480, NULL, 'cannot format unsubscripted array @1', NULL, NULL);
(NULL, 'extend_pool', 'ALL.C', NULL, 1, 481, NULL, 'unsuccessful attempt to extend pool beyond 64KB', NULL, NULL);
(NULL, 'FMT_format', 'FORMAT.C', NULL, 1, 482, NULL, 'field width (@1) * header segments (@2) greater than 60,000 characters', NULL, NULL);
(NULL, 'define_relation', 'META.E', NULL, 1, 483, NULL, 'Relation @1 does not exist', NULL, NULL);
(NULL, NULL, NULL, NULL, 1, 484, NULL, 'FORMs not supported', NULL, NULL);
(NULL, 'show_indices_detail', 'show.e', NULL, 1, 485, NULL, '	Expression index BLR:', NULL, NULL);
(NULL, 'parse_statistical', 'parse.c', NULL, 1, 487, NULL, 'Invalid argument for UDF', NULL, NULL);
(NULL, 'parse_relational', 'parse.c', NULL, 1, 488, NULL, 'SINGULAR (SELECT * <sql rse>)', NULL, NULL);
(NULL, 'parse_join_type', 'parse.c', NULL, 1, 489, NULL, 'JOIN', NULL, NULL);
(NULL, 'show_field_detail', 'show.e', NULL, 1, 495, NULL, 'Field @1 in view @2 of database @3', NULL, NULL);
(NULL, 'show_field_detail', 'show.e', NULL, 1, 496, NULL, 'Field @1 in relation @2 of database @3', NULL, NULL);
(NULL, 'yes_no', 'dtr.c', NULL, 1, 497, NULL, 'YES', NULL, NULL);
(NULL, 'yes_no', 'dtr.c', NULL, 1, 498, NULL, 'NO', NULL, NULL);
(NULL, 'EVAL_value', 'eval.c', NULL, 1, 499, NULL, 'Re-enter', NULL, NULL);
(NULL, 'EVAL_value', 'eval.c', NULL, 1, 500, NULL, 'Enter', NULL, NULL);
(NULL, 'global', 'format.c', NULL, 1, 501, NULL, 'bad kanji found while formatting output', NULL, NULL);
(NULL, 'print_more', 'help.e', 'This message should be followed by 1 blank space: "Subtopic? "', 1, 502, NULL, 'Subtopic? ', NULL, NULL);
(NULL, 'print_topic', 'help.e', 'There should be 1 blank space after the colon: " ... stop: "', 1, 503, NULL, '\ntype <cr> for next topic or <EOF> to stop: ', NULL, NULL);
(NULL, 'mover_error', 'mov.c', NULL, 1, 504, NULL, 'unknown data type @1', NULL, NULL);
(NULL, 'process_statement', 'dtr.c', 'This messages should be preceded by 4 blank spaces: "    reads ..."', 1, 505, NULL, '    reads = !r writes = !w fetches = !f marks = !m', NULL, NULL);
(NULL, 'process_statement', 'dtr.c', 'This message should be preceded by 4 blank spaces: "    elapsed ... "', 1, 506, NULL, '    elapsed = !e cpu = !u system = !s mem = !x buffers = !b', NULL, NULL);
(NULL, 'view_info', 'show.e', NULL, 1, 507, NULL, '@1 Based on field @2 of relation @3', NULL, NULL);
(NULL, 'view_info', 'show.e', NULL, 1, 508, NULL, '@1 Based on field @2 of view @3', NULL, NULL);
(NULL, 'parse_sql_dtype', 'parse.cpp', NULL, 1, 509, NULL, 'PRECISION', NULL, NULL);
(NULL, 'parse_sql_dtype', 'parse.cpp', NULL, 1, 510, NULL, 'Field scale exceeds allowed range', NULL, NULL);
(NULL, 'parse_sql_dtype', 'parse.cpp', NULL, 1, 511, NULL, 'Field length exceeds allowed range', NULL, NULL);
(NULL, 'parse_sql_dtype', 'parse.cpp', NULL, 1, 512, NULL, 'Field length should be greater than zero', NULL, NULL);
-- Do not change the arguments of the previous QLI messages.
-- Write the new QLI messages here.
(NULL, 'usage', 'dtr.cpp', NULL, 1, 513, NULL, 'Usage: qli [options] ["<command>"]', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 514, NULL, '   <command> may be a single qli command or a series of qli commands\n   separated by semicolons', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 515, NULL, 'Valid options are:', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 516, NULL, '   -a(pp_script)       application script <name>', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 517, NULL, '   -b(uffers)          set page buffers <n>', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 518, NULL, '   -f(etch_password)   fetch password from file', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 519, NULL, '   -i(nit_script)      startup script <name>', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 520, NULL, '   -n(obanner)         do not show the welcome message', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 521, NULL, '   -p(assword)         user''s password', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 522, NULL, '   -tra(ce)            show internal parser''s tokens', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 523, NULL, '   -tru(sted_auth)     use trusted authentication', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 524, NULL, '   -u(ser)             user name', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 525, NULL, '   -v(erify)           echo input', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 526, NULL, '   -z                  show program version', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 527, NULL, '   Options can be abbreviated to the unparenthesized characters', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 528, NULL, 'Start qli without [command] to enter interactive mode', NULL, NULL)
(NULL, 'main', 'dtr.cpp', NULL, 1, 529, NULL, 'qli: ignoring unknown switch @1', NULL, NULL)
(NULL, 'install', 'meta.epp', NULL, 1, 530, NULL, 'Warning: cannot issue DDL statements against database "@1"', NULL, NULL)
(NULL, 'usage', 'dtr.cpp', NULL, 1, 531, NULL, '   -nod(btriggers)     do not run database triggers', NULL, NULL)
-- GFIX
('gfix_db_name', 'ALICE_gfix', 'alice.c', NULL, 3, 1, NULL, 'data base file name (@1) already given', NULL, NULL);
('gfix_invalid_sw', 'ALICE_gfix', 'alice.c', NULL, 3, 2, NULL, 'invalid switch @1', NULL, NULL);
('gfix_version', 'ALICE_gfix', 'alice.c', NULL, 3, 3, NULL, 'gfix version @1', NULL, NULL);
('gfix_incmp_sw', 'ALICE_gfix', 'alice.c', NULL, 3, 4, NULL, 'incompatible switch combination', NULL, NULL);
('gfix_replay_req', 'ALICE_gfix', 'alice.c', NULL, 3, 5, NULL, 'replay log pathname required', NULL, NULL);
('gfix_pgbuf_req', 'ALICE_gfix', 'alice.c', NULL, 3, 6, NULL, 'number of page buffers for cache required', NULL, NULL);
('gfix_val_req', 'ALICE_gfix', 'alice.c', NULL, 3, 7, NULL, 'numeric value required', NULL, NULL);
('gfix_pval_req', 'ALICE_gfix', 'alice.c', NULL, 3, 8, NULL, 'positive numeric value required', NULL, NULL);
('gfix_trn_req', 'ALICE_gfix', 'alice.c', NULL, 3, 9, NULL, 'number of transactions per sweep required', NULL, NULL);
('gfix_trn_all_req', 'ALICE_gfix', 'alice.c', NULL, 3, 10, NULL, 'transaction number or "all" required', NULL, NULL);
('gfix_sync_req', 'ALICE_gfix', 'alice.c', NULL, 3, 11, NULL, '"sync" or "async" required', NULL, NULL);
('gfix_full_req', 'ALICE_gfix', 'alice.c', NULL, 3, 12, NULL, '"full" or "reserve" required', NULL, NULL);
('gfix_usrname_req', 'ALICE_gfix', 'alice.c', NULL, 3, 13, NULL, 'user name required', NULL, NULL);
('gfix_pass_req', 'ALICE_gfix', 'alice.c', NULL, 3, 14, NULL, 'password required', NULL, NULL);
('gfix_subs_name', 'ALICE_gfix', 'alice.c', NULL, 3, 15, NULL, 'subsystem name', NULL, NULL);
('gfix_wal_req', 'ALICE_gfix', 'alice.c', NULL, 3, 16, NULL, '"wal" required', NULL, NULL);
('gfix_sec_req', 'ALICE_gfix', 'alice.c', NULL, 3, 17, NULL, 'number of seconds required', NULL, NULL);
('gfix_nval_req', 'ALICE_gfix', 'alice.c', NULL, 3, 18, NULL, 'numeric value between 0 and 32767 inclusive required', NULL, NULL);
('gfix_type_shut', 'ALICE_gfix', 'alice.c', NULL, 3, 19, NULL, 'must specify type of shutdown', NULL, NULL);
('gfix_retry', 'ALICE_gfix', 'alice.c', NULL, 3, 20, NULL, 'please retry, specifying an option', NULL, NULL);
('gfix_opt', 'ALICE_gfix', 'alice.c', NULL, 3, 21, NULL, 'plausible options are:', NULL, NULL);
('gfix_qualifiers', 'ALICE_gfix', 'alice.c', NULL, 3, 22, NULL, '\n    Options can be abbreviated to the unparenthesized characters', NULL, NULL);
('gfix_retry_db', 'ALICE_gfix', 'alice.c', NULL, 3, 23, NULL, 'please retry, giving a database name', NULL, NULL);
('gfix_summary', 'ALICE_gfix', 'alice.c', NULL, 3, 24, NULL, 'Summary of validation errors', NULL, NULL);
('gfix_opt_active', 'ALICE_gfix', 'alice.c', NULL, 3, 25, NULL, '   -ac(tivate_shadow)   activate shadow file for database usage', NULL, NULL);
('gfix_opt_attach', 'ALICE_gfix', 'alice.c', NULL, 3, 26, NULL, '   -at(tach)            shutdown new database attachments', NULL, NULL);
('gfix_opt_begin_log', 'ALICE_gfix', 'alice.c', NULL, 3, 27, NULL, '	-begin_log	begin logging for replay utility', NULL, NULL);
('gfix_opt_buffers', 'ALICE_gfix', 'alice.c', NULL, 3, 28, NULL, '   -b(uffers)           set page buffers <n>', NULL, NULL);
('gfix_opt_commit', 'ALICE_gfix', 'alice.c', NULL, 3, 29, NULL, '   -co(mmit)            commit transaction <tr / all>', NULL, NULL);
('gfix_opt_cache', 'ALICE_gfix', 'alice.c', NULL, 3, 30, NULL, '   -ca(che)             shutdown cache manager', NULL, NULL);
('gfix_opt_disable', 'ALICE_gfix', 'alice.c', NULL, 3, 31, NULL, '	-disable	disable WAL', NULL, NULL);
('gfix_opt_full', 'ALICE_gfix', 'alice.c', NULL, 3, 32, NULL, '   -fu(ll)              validate record fragments (-v)', NULL, NULL);
('gfix_opt_force', 'ALICE_gfix', 'alice.c', NULL, 3, 33, NULL, '   -fo(rce_shutdown)    force database shutdown', NULL, NULL);
('gfix_opt_housekeep', 'ALICE_gfix', 'alice.c', NULL, 3, 34, NULL, '   -h(ousekeeping)      set sweep interval <n>', NULL, NULL);
('gfix_opt_ignore', 'ALICE_gfix', 'alice.c', NULL, 3, 35, NULL, '   -i(gnore)            ignore checksum errors', NULL, NULL);
('gfix_opt_kill', 'ALICE_gfix', 'alice.c', NULL, 3, 36, NULL, '   -k(ill_shadow)       kill all unavailable shadow files', NULL, NULL);
('gfix_opt_list', 'ALICE_gfix', 'alice.c', NULL, 3, 37, NULL, '   -l(ist)              show limbo transactions', NULL, NULL);
('gfix_opt_mend', 'ALICE_gfix', 'alice.c', NULL, 3, 38, NULL, '   -me(nd)              prepare corrupt database for backup', NULL, NULL);
('gfix_opt_no_update', 'ALICE_gfix', 'alice.c', NULL, 3, 39, NULL, '   -n(o_update)         read-only validation (-v)', NULL, NULL);
('gfix_opt_online', 'ALICE_gfix', 'alice.c', NULL, 3, 40, NULL, '   -o(nline)            database online <single / multi / normal>', NULL, NULL);
('gfix_opt_prompt', 'ALICE_gfix', 'alice.c', NULL, 3, 41, NULL, '   -pr(ompt)            prompt for commit/rollback (-l)', NULL, NULL);
('gfix_opt_password', 'ALICE_gfix', 'alice.c', NULL, 3, 42, NULL, '   -pa(ssword)          default password', NULL, NULL);
('gfix_opt_quit_log', 'ALICE_gfix', 'alice.c', NULL, 3, 43, NULL, '	-quit_log	quit logging for replay utility', NULL, NULL);
('gfix_opt_rollback', 'ALICE_gfix', 'alice.c', NULL, 3, 44, NULL, '   -r(ollback)          rollback transaction <tr / all>', NULL, NULL);
('gfix_opt_sweep', 'ALICE_gfix', 'alice.c', NULL, 3, 45, NULL, '   -sw(eep)             force garbage collection', NULL, NULL);
('gfix_opt_shut', 'ALICE_gfix', 'alice.c', NULL, 3, 46, NULL, '   -sh(utdown)          shutdown <full / single / multi>', NULL, NULL);
('gfix_opt_two_phase', 'ALICE_gfix', 'alice.c', NULL, 3, 47, NULL, '   -tw(o_phase)         perform automated two-phase recovery', NULL, NULL);
('gfix_opt_tran', 'ALICE_gfix', 'alice.c', NULL, 3, 48, NULL, '   -tra(nsaction)       shutdown transaction startup', NULL, NULL);
('gfix_opt_use', 'ALICE_gfix', 'alice.c', NULL, 3, 49, NULL, '   -u(se)               use full or reserve space for versions', NULL, NULL);
('gfix_opt_user', 'ALICE_gfix', 'alice.c', NULL, 3, 50, NULL, '   -user                default user name', NULL, NULL);
('gfix_opt_validate', 'ALICE_gfix', 'alice.c', NULL, 3, 51, NULL, '   -v(alidate)          validate database structure', NULL, NULL);
('gfix_opt_write', 'ALICE_gfix', 'alice.c', NULL, 3, 52, NULL, '   -w(rite)             write synchronously or asynchronously', NULL, NULL);
('gfix_opt_x', 'ALICE_gfix', 'alice.c', NULL, 3, 53, NULL, '   -x                   set debug on', NULL, NULL);
('gfix_opt_z', 'ALICE_gfix', 'alice.c', NULL, 3, 54, NULL, '   -z                   print software version number', NULL, NULL);
('gfix_rec_err', 'ALICE_gfix', 'alice.c', NULL, 3, 55, NULL, '\n	Number of record level errors	: @1', NULL, NULL);
('gfix_blob_err', 'ALICE_gfix', 'alice.c', NULL, 3, 56, NULL, '	Number of Blob page errors	: @1', NULL, NULL);
('gfix_data_err', 'ALICE_gfix', 'alice.c', NULL, 3, 57, NULL, '	Number of data page errors	: @1', NULL, NULL);
('gfix_index_err', 'ALICE_gfix', 'alice.c', NULL, 3, 58, NULL, '	Number of index page errors	: @1', NULL, NULL);
('gfix_pointer_err', 'ALICE_gfix', 'alice.c', NULL, 3, 59, NULL, '	Number of pointer page errors	: @1', NULL, NULL);
('gfix_trn_err', 'ALICE_gfix', 'alice.c', NULL, 3, 60, NULL, '	Number of transaction page errors	: @1', NULL, NULL);
('gfix_db_err', 'ALICE_gfix', 'alice.c', NULL, 3, 61, NULL, '	Number of database page errors	: @1', NULL, NULL);
('gfix_bad_block', 'ALLA_alloc', 'all.c', NULL, 3, 62, NULL, 'bad block type', NULL, NULL);
('gfix_exceed_max', 'ALLA_alloc', 'all.c', NULL, 3, 63, NULL, 'internal block exceeds maximum size', NULL, NULL);
('gfix_corrupt_pool', 'ALLA_alloc', 'all.c', NULL, 3, 64, NULL, 'corrupt pool', NULL, NULL);
('gfix_mem_exhausted', 'ALLA_malloc', 'all.c', NULL, 3, 65, NULL, 'virtual memory exhausted', NULL, NULL);
('gfix_bad_pool', 'find_pool', 'all.c', NULL, 3, 66, NULL, 'bad pool id', NULL, NULL);
('gfix_trn_not_valid', 'TDR_analyze', 'tdr.c', NULL, 3, 67, NULL, 'Transaction state @1 not in valid range.', NULL, NULL);
('gfix_dbg_attach', 'TDR_attach_database', 'tdr.c', NULL, 3, 68, NULL, 'ATTACH_DATABASE: attempted attach of @1,', NULL, NULL);
('gfix_dbg_failed', 'TDR_attach_database', 'tdr.c', NULL, 3, 69, NULL, ' failed', NULL, NULL);
('gfix_dbg_success', 'TDR_attach_database', 'tdr.c', NULL, 3, 70, NULL, ' succeeded', NULL, NULL);
('gfix_trn_limbo', 'TDR_list_limbo', 'tdr.c', NULL, 3, 71, NULL, 'Transaction @1 is in limbo.', NULL, NULL);
('gfix_try_again', 'TDR_list_limbo', 'tdr.c', NULL, 3, 72, NULL, 'More limbo transactions than fit.  Try again', NULL, NULL);
('gfix_unrec_item', 'TDR_list_limbo', 'tdr.c', NULL, 3, 73, NULL, 'Unrecognized info item @1', NULL, NULL);
('gfix_commit_violate', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 74, NULL, 'A commit of transaction @1 will violate two-phase commit.', NULL, NULL);
('gfix_preserve', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 75, NULL, 'A rollback of transaction @1 is needed to preserve two-phase commit.', NULL, NULL);
('gfix_part_commit', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 76, NULL, 'Transaction @1 has already been partially committed.', NULL, NULL);
('gfix_rback_violate', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 77, NULL, 'A rollback of this transaction will violate two-phase commit.', NULL, NULL);
('gfix_part_commit2', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 78, NULL, 'Transaction @1 has been partially committed.', NULL, NULL);
('gfix_commit_pres', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 79, NULL, 'A commit is necessary to preserve the two-phase commit.', NULL, NULL);
('gfix_insuff_info', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 80, NULL, 'Insufficient information is available to determine', NULL, NULL);
('gfix_action', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 81, NULL, 'a proper action for transaction @1.', NULL, NULL);
('gfix_all_prep', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 82, NULL, 'Transaction @1: All subtransactions have been prepared.', NULL, NULL);
('gfix_comm_rback', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 83, NULL, 'Either commit or rollback is possible.', NULL, NULL);
('gfix_unexp_eoi', 'TDR_reconnect_multiple', 'tdr.c', NULL, 3, 84, NULL, 'unexpected end of input', NULL, NULL);
('gfix_ask', 'ask', 'tdr.c', NULL, 3, 85, NULL, 'Commit, rollback, or neither (c, r, or n)?', NULL, NULL);
('gfix_reattach_failed', 'reattach_database', 'tdr.c', NULL, 3, 86, NULL, 'Could not reattach to database for transaction @1.', NULL, NULL);
('gfix_org_path', 'reattach_database', 'tdr.c', NULL, 3, 87, NULL, 'Original path: @1', NULL, NULL);
('gfix_enter_path', 'reattach_database', 'tdr.c', NULL, 3, 88, NULL, 'Enter a valid path:', NULL, NULL);
('gfix_att_unsucc', 'reattach_database', 'tdr.c', NULL, 3, 89, NULL, 'Attach unsuccessful.', NULL, NULL);
('gfix_recon_fail', 'reconnect', 'tdr.c', NULL, 3, 90, NULL, 'failed to reconnect to a transaction in database @1', NULL, NULL);
COMMIT WORK;
('gfix_trn2', 'reconnect', 'tdr.c', NULL, 3, 91, NULL, 'Transaction @1:', NULL, NULL);
('gfix_mdb_trn', 'print_description', 'tdr.c', NULL, 3, 92, NULL, '  Multidatabase transaction:', NULL, NULL);
('gfix_host_site', 'print_description', 'tdr.c', NULL, 3, 93, NULL, '    Host Site: @1', NULL, NULL);
('gfix_trn', 'print_description', 'tdr.c', NULL, 3, 94, NULL, '    Transaction @1', NULL, NULL);
('gfix_prepared', 'print_description', 'tdr.c', NULL, 3, 95, NULL, 'has been prepared.', NULL, NULL);
('gfix_committed', 'print_description', 'tdr.c', NULL, 3, 96, NULL, 'has been committed.', NULL, NULL);
('gfix_rolled_back', 'print_description', 'tdr.c', NULL, 3, 97, NULL, 'has been rolled back.', NULL, NULL);
('gfix_not_available', 'print_description', 'tdr.c', NULL, 3, 98, NULL, 'is not available.', NULL, NULL);
('gfix_not_prepared', 'print_description', 'tdr.c', NULL, 3, 99, NULL, 'is not found, assumed not prepared.', NULL, NULL);
('gfix_be_committed', 'print_description', 'tdr.c', NULL, 3, 100, NULL, 'is not found, assumed to be committed.', NULL, NULL);
('gfix_rmt_site', 'print_description', 'tdr.c', NULL, 3, 101, NULL, '        Remote Site: @1', NULL, NULL);
('gfix_db_path', 'print_description', 'tdr.c', NULL, 3, 102, NULL, '        Database Path: @1', NULL, NULL);
('gfix_auto_comm', 'print_description', 'tdr.c', NULL, 3, 103, NULL, '  Automated recovery would commit this transaction.', NULL, NULL);
('gfix_auto_rback', 'print_description', 'tdr.c', NULL, 3, 104, NULL, '  Automated recovery would rollback this transaction.', NULL, NULL);
('gfix_warning', 'TDR_analyze', 'tdr.c', NULL, 3, 105, NULL, 'Warning: Multidatabase transaction is in inconsistent state for recovery.', NULL, NULL);
('gfix_trn_was_comm', 'TDR_analyze', 'tdr.c', NULL, 3, 106, NULL, 'Transaction @1 was committed, but prior ones were rolled back.', NULL, NULL);
('gfix_trn_was_rback', 'TDR_analyze', 'tdr.c', NULL, 3, 107, NULL, 'Transaction @1 was rolled back, but prior ones were committed.', NULL, NULL);
('gfix_trn_unknown', 'get_description', 'met.e', NULL, 3, 108, NULL, 'Transaction description item unknown', NULL, NULL);
('gfix_opt_mode', 'ALICE_gfix', 'alice.c', NULL, 3, 109, NULL, '   -mo(de)              read_only or read_write database', NULL, NULL);
('gfix_mode_req', 'ALICE_gfix', 'alice.c', NULL, 3, 110, NULL, '"read_only" or "read_write" required', NULL, NULL);
('gfix_opt_SQL_dialect', 'ALICE_gfix', 'alice.c, enter_messages', NULL, 3, 111, NULL, '   -sq(l_dialect)       set database dialect n', NULL, NULL);
('gfix_SQL_dialect', 'ALICE_gfix', 'alice.c', NULL, 3, 112, NULL, 'database SQL dialect must be one of ''@1''', NULL, NULL);
('gfix_dialect_req', 'ALICE_gfix', 'alice.c', NULL, 3, 113, NULL, 'dialect number required', NULL, NULL);
('gfix_pzval_req', 'ALICE_gfix', 'alice.cpp', NULL, 3, 114, NULL, 'positive or zero numeric value required', NULL, NULL);
-- Do not change the arguments of the previous GFIX messages.
-- Write the new GFIX messages here.
('gfix_opt_trusted', 'ALICE_gfix', 'alice.cpp', NULL, 3, 115, NULL, '   -tru(sted)           use trusted authentication', NULL, NULL);
(NULL, 'ALICE_gfix', 'alice.cpp', NULL, 3, 116, NULL, 'could not open password file @1, errno @2', NULL, NULL);
(NULL, 'ALICE_gfix', 'alice.cpp', NULL, 3, 117, NULL, 'could not read password file @1, errno @2', NULL, NULL);
(NULL, 'ALICE_gfix', 'alice.cpp', NULL, 3, 118, NULL, 'empty password file @1', NULL, NULL);
(NULL, 'ALICE_gfix', 'alice.cpp', NULL, 3, 119, NULL, '   -fe(tch_password)    fetch password from file', NULL, NULL);
(NULL, 'alice', 'alice.cpp', NULL, 3, 120, NULL, 'usage: gfix [options] <database>', NULL, NULL);
('gfix_opt_nolinger', 'ALICE_gfix', 'alice.c', NULL, 3, 121, NULL, '   -nol(inger)          close database ignoring linger setting for it', NULL, NULL);
-- DSQL
('dsql_dbkey_from_non_table', 'MAKE_desc', 'make.c', NULL, 7, 2, NULL, 'Cannot SELECT RDB$DB_KEY from a stored procedure.', NULL, NULL);
('dsql_transitional_numeric', 'dsql_yyparse', 'parse.y', NULL, 7, 3, NULL, 'Precision 10 to 18 changed from DOUBLE PRECISION in SQL dialect 1 to 64-bit scaled integer in SQL dialect 3', NULL, NULL);
('dsql_dialect_warning_expr', 'GEN_expr', 'gen.c', NULL, 7, 4, NULL, 'Use of @1 expression that returns different results in dialect 1 and dialect 3', NULL, NULL);
('sql_db_dialect_dtype_unsupport', 'dsql_yyparse', 'parse.y', NULL, 7, 5, NULL, 'Database SQL dialect @1 does not support reference to @2 datatype', NULL, NULL);
(NULL, NULL, NULL, NULL, 7, 6, NULL, '', NULL, NULL);
('isc_sql_dialect_conflict_num', 'dsql_yyparse', 'parse.y', NULL, 7, 7, NULL, 'DB dialect @1 and client dialect @2 conflict with respect to numeric precision @3.', NULL, NULL);
('dsql_warning_number_ambiguous', 'yylex', 'parse.y', NULL, 7, 8, NULL, 'WARNING: Numeric literal @1 is interpreted as a floating-point', NULL, NULL);
('dsql_warning_number_ambiguous1', 'yylex', 'parse.y', NULL, 7, 9, NULL, 'value in SQL dialect 1, but as an exact numeric value in SQL dialect 3.', NULL, NULL);
('dsql_warn_precision_ambiguous', 'dsql_yyparse', 'parse.y', NULL, 7, 10, NULL, 'WARNING: NUMERIC and DECIMAL fields with precision 10 or greater are stored', NULL, NULL);
('dsql_warn_precision_ambiguous1', 'dsql_yyparse', 'parse.y', NULL, 7, 11, NULL, 'as approximate floating-point values in SQL dialect 1, but as 64-bit', NULL, NULL);
('dsql_warn_precision_ambiguous2', 'dsql_yyparse', 'parse.y', NULL, 7, 12, NULL, 'integers in SQL dialect 3.', NULL, NULL);
('dsql_ambiguous_field_name', 'pass1_field', 'pass1.c', NULL, 7, 13, NULL, 'Ambiguous field name between @1 and @2', NULL, NULL);
('dsql_udf_return_pos_err', 'define_udf', 'ddl.c', NULL, 7, 14, NULL, 'External function should have return position between 1 and @1', NULL, NULL);
('dsql_invalid_label', 'PASS1_label', 'pass1.cpp', NULL, 7, 15, NULL, 'Label @1 @2 in the current scope', NULL, NULL);
('dsql_datatypes_not_comparable', 'MAKE_desc_from_list', 'make.cpp', NULL, 7, 16, NULL, 'Datatypes @1are not comparable in expression @2', NULL, NULL);
('dsql_cursor_invalid', NULL, 'pass1.cpp', NULL, 7, 17, NULL, 'Empty cursor name is not allowed', NULL, NULL);
('dsql_cursor_redefined', NULL, 'pass1.cpp', NULL, 7, 18, NULL, 'Statement already has a cursor @1 assigned', NULL, NULL);
('dsql_cursor_not_found', NULL, 'pass1.cpp', NULL, 7, 19, NULL, 'Cursor @1 is not found in the current context', NULL, NULL);
('dsql_cursor_exists', NULL, 'pass1.cpp', NULL, 7, 20, NULL, 'Cursor @1 already exists in the current context', NULL, NULL);
('dsql_cursor_rel_ambiguous', NULL, 'pass1.cpp', NULL, 7, 21, NULL, 'Relation @1 is ambiguous in cursor @2', NULL, NULL);
('dsql_cursor_rel_not_found', NULL, 'pass1.cpp', NULL, 7, 22, NULL, 'Relation @1 is not found in cursor @2', NULL, NULL);
('dsql_cursor_not_open', NULL, 'dsql.cpp', NULL, 7, 23, NULL, 'Cursor is not open', NULL, NULL);
('dsql_type_not_supp_ext_tab', 'define_field', 'ddl.cpp', NULL, 7, 24, NULL, 'Data type @1 is not supported for EXTERNAL TABLES. Relation ''@2'', field ''@3''', NULL, NULL);
('dsql_feature_not_supported_ods', NULL, 'ddl.cpp', NULL, 7, 25, NULL, 'Feature not supported on ODS version older than @1.@2', NULL, NULL);
('primary_key_required', 'pass1_replace', 'pass1.cpp', NULL, 7, 26, NULL, 'Primary key required on table @1', NULL, NULL);
('upd_ins_doesnt_match_pk', 'pass1_update_or_insert', 'pass1.cpp', NULL, 7, 27, NULL, 'UPDATE OR INSERT field list does not match primary key of table @1', NULL, NULL);
('upd_ins_doesnt_match_matching', 'pass1_update_or_insert', 'pass1.cpp', NULL, 7, 28, NULL, 'UPDATE OR INSERT field list does not match MATCHING clause', NULL, NULL);
('upd_ins_with_complex_view', 'pass1_update_or_insert', 'pass1.cpp', NULL, 7, 29, NULL, 'UPDATE OR INSERT without MATCHING could not be used with views based on more than one table', NULL, NULL);
('dsql_incompatible_trigger_type', 'define_trigger', 'ddl.cpp', NULL, 7, 30, NULL, 'Incompatible trigger type', NULL, NULL);
('dsql_db_trigger_type_cant_change', 'define_trigger', 'ddl.cpp', NULL, 7, 31, NULL, 'Database trigger type can''t be changed', NULL, NULL);
('dsql_record_version_table', 'MAKE_desc', 'ExprNodes.cpp', NULL, 7, 32, NULL, 'To be used with RDB$RECORD_VERSION, @1 must be a table or a view of single table', NULL, NULL);
-- Do not change the arguments of the previous DSQL messages.
-- Write the new DSQL messages here.
-- DYN
(NULL, NULL, 'dyn.c', NULL, 8, 1, NULL, 'ODS version not supported by DYN', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 2, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 3, NULL, 'STORE RDB$FIELD_DIMENSIONS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 4, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 5, NULL, '@1', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 6, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 7, NULL, 'DEFINE BLOB FILTER failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 8, NULL, 'DEFINE GENERATOR failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 9, NULL, 'DEFINE GENERATOR unexpected DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 10, NULL, 'DEFINE FUNCTION failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 11, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 12, NULL, 'DEFINE FUNCTION ARGUMENT failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 13, NULL, 'STORE RDB$FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 14, NULL, 'No table specified for index', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 15, NULL, 'STORE RDB$INDEX_SEGMENTS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 16, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 17, NULL, 'PRIMARY KEY column lookup failed', NULL, NULL);
(NULL, NULL, 'dyn_def.epp', NULL, 8, 18, NULL, 'could not find UNIQUE or PRIMARY KEY constraint in table @1 with specified columns', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 19, NULL, 'PRIMARY KEY lookup failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 20, NULL, 'could not find PRIMARY KEY index in specified table @1', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 21, NULL, 'STORE RDB$INDICES failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 22, NULL, 'STORE RDB$FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 23, NULL, 'STORE RDB$RELATION_FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 24, NULL, 'STORE RDB$RELATIONS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 25, NULL, 'STORE RDB$USER_PRIVILEGES failed defining a table', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 26, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 27, NULL, 'STORE RDB$RELATIONS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 28, NULL, 'STORE RDB$FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 29, NULL, 'STORE RDB$RELATION_FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 30, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 31, NULL, 'DEFINE TRIGGER failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 32, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 33, NULL, 'DEFINE TRIGGER MESSAGE failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 34, NULL, 'STORE RDB$VIEW_RELATIONS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 35, NULL, 'ERASE RDB$FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 36, NULL, 'ERASE BLOB FILTER failed', NULL, NULL);
('dyn_filter_not_found', NULL, 'dyn.c', NULL, 8, 37, NULL, 'BLOB Filter @1 not found', 'Define blob filter in RDB$BLOB_FILTERS.', 'Blob filter was not found.');
(NULL, NULL, 'dyn.c', NULL, 8, 38, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 39, NULL, 'ERASE RDB$FUNCTION_ARGUMENTS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 40, NULL, 'ERASE RDB$FUNCTIONS failed', NULL, NULL);
('dyn_func_not_found', NULL, 'dyn.c', NULL, 8, 41, NULL, 'Function @1 not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 42, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn_del.epp', NULL, 8, 43, NULL, 'Domain @1 is used in table @2 (local name @3) and cannot be dropped', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 44, NULL, 'ERASE RDB$FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 45, NULL, 'ERASE RDB$FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 46, NULL, 'Column not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 47, NULL, 'ERASE RDB$INDICES failed', NULL, NULL);
('dyn_index_not_found', NULL, 'dyn.c', NULL, 8, 48, NULL, 'Index not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 49, NULL, 'ERASE RDB$INDEX_SEGMENTS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 50, NULL, 'No segments found for index', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 51, NULL, 'No table specified in ERASE RFR', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 52, NULL, 'Column @1 from table @2 is referenced in view @3', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 53, NULL, 'ERASE RDB$RELATION_FIELDS failed', NULL, NULL);
('dyn_view_not_found', NULL, 'dyn.c', NULL, 8, 54, NULL, 'View @1 not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 55, NULL, 'Column not found for table', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 56, NULL, 'ERASE RDB$INDEX_SEGMENTS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 57, NULL, 'ERASE RDB$INDICES failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 58, NULL, 'ERASE RDB$RELATION_FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 59, NULL, 'ERASE RDB$VIEW_RELATIONS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 60, NULL, 'ERASE RDB$RELATIONS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 61, NULL, 'Table not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 62, NULL, 'ERASE RDB$USER_PRIVILEGES failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 63, NULL, 'ERASE RDB$FILES failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 64, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 65, NULL, 'ERASE RDB$TRIGGER_MESSAGES failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 66, NULL, 'ERASE RDB$TRIGGERS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 67, NULL, 'Trigger not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 68, NULL, 'MODIFY RDB$VIEW_RELATIONS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 69, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 70, NULL, 'TRIGGER NAME expected', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 71, NULL, 'ERASE TRIGGER MESSAGE failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 72, NULL, 'Trigger Message not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 73, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 74, NULL, 'ERASE RDB$SECURITY_CLASSES failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 75, NULL, 'Security class not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 76, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 77, NULL, 'SELECT RDB$USER_PRIVILEGES failed in grant', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 78, NULL, 'SELECT RDB$USER_PRIVILEGES failed in grant', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 79, NULL, 'STORE RDB$USER_PRIVILEGES failed in grant', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 80, NULL, 'Specified domain or source column does not exist', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 81, NULL, 'Generation of column name failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 82, NULL, 'Generation of index name failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 83, NULL, 'Generation of trigger name failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 84, NULL, 'MODIFY DATABASE failed', NULL, NULL);
(NULL, NULL, 'dyn_mod.epp', NULL, 8, 85, NULL, 'MODIFY RDB$CHARACTER_SETS failed', NULL, NULL);
(NULL, NULL, 'dyn_mod.epp', NULL, 8, 86, NULL, 'MODIFY RDB$COLLATIONS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 87, NULL, 'MODIFY RDB$FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn_mod.epp', NULL, 8, 88, NULL, 'MODIFY RDB$BLOB_FILTERS failed', NULL, NULL);
('dyn_domain_not_found', NULL, 'dyn.c', NULL, 8, 89, NULL, 'Domain not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 90, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn_mod.epp', NULL, 8, 91, NULL, 'MODIFY RDB$INDICES failed', NULL, NULL);
(NULL, NULL, 'dyn_mod.epp', NULL, 8, 92, NULL, 'MODIFY RDB$FUNCTIONS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 93, NULL, 'Index column not found', NULL, NULL);
(NULL, NULL, 'dyn_mod.epp', NULL, 8, 94, NULL, 'MODIFY RDB$GENERATORS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 95, NULL, 'MODIFY RDB$RELATION_FIELDS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 96, NULL, 'Local column @1 not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 97, NULL, 'add EXTERNAL FILE not allowed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 98, NULL, 'drop EXTERNAL FILE not allowed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 99, NULL, 'MODIFY RDB$RELATIONS failed', NULL, NULL);
(NULL, NULL, 'dyn_mod.epp', NULL, 8, 100, NULL, 'MODIFY RDB$PROCEDURE_PARAMETERS failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 101, NULL, 'Table column not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 102, NULL, 'MODIFY TRIGGER failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 103, NULL, 'TRIGGER NAME expected', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 104, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 105, NULL, 'MODIFY TRIGGER MESSAGE failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 106, NULL, 'Create metadata BLOB failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 107, NULL, 'Write metadata BLOB failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 108, NULL, 'Close metadata BLOB failed', NULL, NULL);
('dyn_cant_modify_auto_trig', NULL, 'dyn.c', NULL, 8, 109, NULL, 'Triggers created automatically cannot be modified', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 110, NULL, 'unsupported DYN verb', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 111, NULL, 'ERASE RDB$USER_PRIVILEGES failed in revoke(1)', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 112, NULL, 'Access to RDB$USER_PRIVILEGES failed in revoke(2)', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 113, NULL, 'ERASE RDB$USER_PRIVILEGES failed in revoke (3)', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 114, NULL, 'Access to RDB$USER_PRIVILEGES failed in revoke (4)', NULL, NULL);
(NULL, 'define_relation', 'dyn.e', NULL, 8, 115, NULL, 'CREATE VIEW failed', NULL, NULL);
(NULL, 'define_index', 'dyn.e', NULL, 8, 116, NULL, ' attempt to index BLOB column in INDEX @1', NULL, NULL);
(NULL, 'define_index', 'dyn.e', NULL, 8, 117, NULL, ' attempt to index array column in index @1', NULL, NULL);
(NULL, 'define_index', 'dyn.e', NULL, 8, 118, NULL, 'key size too big for index @1', NULL, NULL);
(NULL, 'define_index', 'dyn.e', NULL, 8, 119, NULL, 'no keys for index @1', NULL, NULL);
(NULL, 'define_index', 'dyn.e', NULL, 8, 120, NULL, 'Unknown columns in index @1', NULL, NULL);
(NULL, 'define_constraint', 'dyn.e', NULL, 8, 121, NULL, 'STORE RDB$RELATION_CONSTRAINTS failed', NULL, NULL);
(NULL, 'define_constraint', 'dyn.e', NULL, 8, 122, NULL, 'STORE RDB$CHECK_CONSTRAINTS failed', NULL, NULL);
(NULL, 'define_constraint', 'dyn.e', NULL, 8, 123, NULL, 'Column: @1 not defined as NOT NULL - cannot be used in PRIMARY KEY constraint definition', NULL, NULL);
(NULL, 'define_constraint', 'dyn.e', NULL, 8, 124, NULL, 'A column name is repeated in the definition of constraint: @1', NULL, NULL);
(NULL, 'define_constraint', 'dyn.e', NULL, 8, 125, NULL, 'Integrity Constraint lookup failed', NULL, NULL);
(NULL, 'define_constraint', 'dyn.e', NULL, 8, 126, NULL, 'Same set of columns cannot be used in more than one PRIMARY KEY and/or UNIQUE constraint definition', NULL, NULL);
(NULL, 'define_constraint', 'dyn.e', NULL, 8, 127, NULL, 'STORE RDB$REF_CONSTRAINTS failed', NULL, NULL);
(NULL, 'delete_constraint', 'dyn.e', NULL, 8, 128, NULL, 'No table specified in delete_constraint', NULL, NULL);
(NULL, 'delete_constraint', 'dyn.e', NULL, 8, 129, NULL, 'ERASE RDB$RELATION_CONSTRAINTS failed', NULL, NULL);
(NULL, 'delete_constraint', 'dyn.e', NULL, 8, 130, NULL, 'CONSTRAINT @1 does not exist.', NULL, NULL);
(NULL, 'generate_constraint_name', 'dyn.e', NULL, 8, 131, NULL, 'Generation of constraint name failed', NULL, NULL);
('dyn_dup_table', 'define_relation', 'dyn.c', NULL, 8, 132, NULL, 'Table @1 already exists', NULL, NULL);
(NULL, 'define_index', 'dyn.e', NULL, 8, 133, NULL, 'Number of referencing columns do not equal number of referenced columns', NULL, NULL);
(NULL, 'define_procedure', 'dyn.e', NULL, 8, 134, NULL, 'STORE RDB$PROCEDURES failed', NULL, NULL);
('dyn_dup_procedure', 'define_procedure', 'dyn.e', NULL, 8, 135, NULL, 'Procedure @1 already exists', NULL, NULL);
(NULL, 'define_parameter', 'dyn.e', NULL, 8, 136, NULL, 'STORE RDB$PROCEDURE_PARAMETERS failed', NULL, NULL);
(NULL, 'define_intl_info', 'dyn.e', NULL, 8, 137, NULL, 'Store into system table @1 failed', NULL, NULL);
(NULL, 'delete_procedure', 'dyn.e', NULL, 8, 138, NULL, 'ERASE RDB$PROCEDURE_PARAMETERS failed', NULL, NULL);
(NULL, 'delete_procedure', 'dyn.e', NULL, 8, 139, NULL, 'ERASE RDB$PROCEDURES failed', NULL, NULL);
('dyn_proc_not_found', 'delete_procedure', 'dyn.e', NULL, 8, 140, NULL, 'Procedure @1 not found', NULL, NULL);
(NULL, 'modify_procedure', 'dyn.e', NULL, 8, 141, NULL, 'MODIFY RDB$PROCEDURES failed', NULL, NULL);
(NULL, 'define_exception', 'dyn.e', NULL, 8, 142, NULL, 'DEFINE EXCEPTION failed', NULL, NULL);
(NULL, 'delete_exception', 'dyn.e', NULL, 8, 143, NULL, 'ERASE EXCEPTION failed', NULL, NULL);
('dyn_exception_not_found', 'delete_exception', 'dyn.e', NULL, 8, 144, NULL, 'Exception not found', NULL, NULL);
(NULL, 'modify_exception', 'dyn.e', NULL, 8, 145, NULL, 'MODIFY EXCEPTION failed', NULL, NULL);
('dyn_proc_param_not_found', 'delete_parameter', 'dyn.e', NULL, 8, 146, NULL, 'Parameter @1 in procedure @2 not found', NULL, NULL);
('dyn_trig_not_found', 'modify_trigger', 'dyn.e', NULL, 8, 147, NULL, 'Trigger @1 not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 148, NULL, 'Only one data type change to the domain @1 allowed at a time', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 149, NULL, 'Only one data type change to the field @1 allowed at a time', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 150, NULL, 'STORE RDB$FILES failed', NULL, NULL);
('dyn_charset_not_found', NULL, 'dyn_mod.epp', NULL, 8, 151, NULL, 'Character set @1 not found', NULL, NULL);
('dyn_collation_not_found', NULL, 'dyn_mod.epp', NULL, 8, 152, NULL, 'Collation @1 not found', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 153, NULL, 'ERASE RDB$LOG_FILES failed', NULL, NULL);
(NULL, NULL, 'dyn.c', NULL, 8, 154, NULL, 'STORE RDB$LOG_FILES failed', NULL, NULL);
('dyn_role_not_found', NULL, 'dyn_mod.epp', NULL, 8, 155, NULL, 'Role @1 not found', NULL, NULL);
(NULL, NULL, 'dyn.e', NULL, 8, 156, NULL, 'Difference file lookup failed', NULL, NULL);
(NULL, 'define_shadow', 'dyn.e', NULL, 8, 157, NULL, 'DEFINE SHADOW failed', NULL, NULL);
(NULL, 'modify_role', 'dyn_mod.epp', NULL, 8, 158, NULL, 'MODIFY RDB$ROLES failed', NULL, NULL);
('dyn_name_longer', 'get_string', 'dyn.e', NULL, 8, 159, NULL, 'Name longer than database column size', NULL, NULL);
(NULL, 'modify_global_field', 'dyn', NULL, 8, 160, NULL, '"Only one constraint allowed for a domain"', NULL, NULL);
(NULL, 'generate_field_position', 'dyn.e', NULL, 8, 162, NULL, 'Looking up column position failed', NULL, NULL);
(NULL, 'define_relation', 'dyn.e', NULL, 8, 163, NULL, 'A node name is not permitted in a table with external file definition', NULL, NULL);
(NULL, 'define_shadow', 'dyn.e', NULL, 8, 164, NULL, 'Shadow lookup failed', NULL, NULL);
(NULL, 'define_shadow', 'dyn.e', NULL, 8, 165, NULL, 'Shadow @1 already exists', NULL, NULL);
(NULL, 'define_file', 'dyn.c', NULL, 8, 166, NULL, 'Cannot add file with the same name as the database or added files', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.e', NULL, 8, 167, NULL, 'no grant option for privilege @1 on column @2 of table/view @3', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.e', NULL, 8, 168, NULL, 'no grant option for privilege @1 on column @2 of base table/view @3', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.e', NULL, 8, 169, NULL, 'no grant option for privilege @1 on table/view @2 (for column @3)', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.e', NULL, 8, 170, NULL, 'no grant option for privilege @1 on base table/view @2 (for column @3)', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.e', NULL, 8, 171, NULL, 'no @1 privilege with grant option on table/view @2 (for column @3)', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.e', NULL, 8, 172, NULL, 'no @1 privilege with grant option on base table/view @2 (for column @3)', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.e', NULL, 8, 173, NULL, 'no grant option for privilege @1 on table/view @2', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.e', NULL, 8, 174, NULL, 'no @1 privilege with grant option on table/view @2', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.e', NULL, 8, 175, NULL, 'table/view @1 does not exist', NULL, NULL);
('dyn_column_does_not_exist', 'grantor_can_grant', 'dyn.e', NULL, 8, 176, NULL, 'column @1 does not exist in table/view @2', NULL, NULL);
(NULL, 'modify_relation', 'dyn.e', NULL, 8, 177, NULL, 'Can not alter a view', NULL, NULL);
(NULL, 'check_relation_existance', 'jrd/dyn.e', NULL, 8, 178, NULL, 'EXTERNAL FILE table not supported in this context', NULL, NULL);
(NULL, 'create_index', 'dyn.e', NULL, 8, 179, NULL, 'attempt to index COMPUTED BY column in INDEX @1', NULL, NULL);
(NULL, 'define_index', 'dyn.e', NULL, 8, 180, NULL, 'Table Name lookup failed', NULL, NULL);
(NULL, 'define_index', 'dyn.e', NULL, 8, 181, NULL, 'attempt to index a view', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.c', NULL, 8, 182, NULL, 'SELECT RDB$RELATIONS failed in grant', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.c', NULL, 8, 183, NULL, 'SELECT RDB$RELATION_FIELDS failed in grant', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.c', NULL, 8, 184, NULL, 'SELECT RDB$RELATIONS/RDB$OWNER_NAME failed in grant', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn.c', NULL, 8, 185, NULL, 'SELECT RDB$USER_PRIVILEGES failed in grant', NULL, NULL);
(NULL, 'grantor_can_grant', 'dyn', NULL, 8, 186, NULL, 'SELECT RDB$VIEW_RELATIONS/RDB$RELATION_FIELDS/... failed in grant', NULL, NULL);
(NULL, 'DYN_delete_local_field', 'dyn_del.e', NULL, 8, 187, NULL, 'column @1 from table @2 is referenced in index @3', NULL, NULL);
('dyn_role_does_not_exist', 'grant, revoke', 'dyn.e', NULL, 8, 188, NULL, 'SQL role @1 does not exist', NULL, NULL);
('dyn_no_grant_admin_opt', 'grant, revoke', 'dyn.e', NULL, 8, 189, NULL, 'user @1 has no grant admin option on SQL role @2', NULL, NULL);
('dyn_user_not_role_member', 'grant, revoke', 'dyn.e', NULL, 8, 190, NULL, 'user @1 is not a member of SQL role @2', NULL, NULL);
('dyn_delete_role_failed', 'DYN_delete_role', 'dyn.e', NULL, 8, 191, NULL, '@1 is not the owner of SQL role @2', NULL, NULL);
('dyn_grant_role_to_user', 'grant', 'dyn.e', NULL, 8, 192, NULL, '@1 is a SQL role and not a user', NULL, NULL);
('dyn_inv_sql_role_name', 'DYN_define_role', 'dyn_def.e', NULL, 8, 193, NULL, 'user name @1 could not be used for SQL role', NULL, NULL);
('dyn_dup_sql_role', 'DYN_define_role', 'dyn_def.e', NULL, 8, 194, NULL, 'SQL role @1 already exists', NULL, NULL);
('dyn_kywd_spec_for_role', 'grant', 'dyn.e', NULL, 8, 195, NULL, 'keyword @1 can not be used as a SQL role name', NULL, NULL);
('dyn_roles_not_supported', 'DYN_define_role, ...', 'dyn.e, dyn_def.e', NULL, 8, 196, NULL, 'SQL roles are not supported in on older versions of the database.  A backup and restore of the database is required.', NULL, NULL);
('dyn_domain_name_exists', 'DYN_modify_global_field', 'dyn_mod.e', NULL, 8, 204, NULL, 'Cannot rename domain @1 to @2.  A domain with that name already exists.', NULL, NULL);
('dyn_field_name_exists', 'DYN_modify_local_field', 'dyn_mod.e', NULL, 8, 205, NULL, 'Cannot rename column @1 to @2.  A column with that name already exists in table @3.', NULL, NULL);
('dyn_dependency_exists', 'DYN_modify_global/local field', 'dyn_mod.e', NULL, 8, 206, NULL, 'Column @1 from table @2 is referenced in @3', NULL, NULL);
('dyn_dtype_invalid', 'check_update_field_type', 'dyn_mod.e', NULL, 8, 207, NULL, 'Cannot change datatype for column @1.  Changing datatype is not supported for BLOB or ARRAY columns.', NULL, NULL);
('dyn_char_fld_too_small', 'check_update_field_type', 'dyn_mod.e', NULL, 8, 208, NULL, 'New size specified for column @1 must be at least @2 characters.', NULL, NULL);
('dyn_invalid_dtype_conversion', 'check_update_fld_type', 'dyn_mod.e', NULL, 8, 209, NULL, 'Cannot change datatype for @1.  Conversion from base type @2 to @3 is not supported.', NULL, NULL);
('dyn_dtype_conv_invalid', 'check_update_field_type', 'dyn_mod.e', NULL, 8, 210, NULL, 'Cannot change datatype for column @1 from a character type to a non-character type.', NULL, NULL);
('dyn_virmemexh', 'DYN_modify_sql/global_field', 'dyn_mod.e', NULL, 8, 211, NULL, 'unable to allocate memory from the operating system', NULL, NULL);
('dyn_zero_len_id', 'DYN_create_exception', 'dyn_def.e', NULL, 8, 212, NULL, 'Zero length identifiers are not allowed', NULL, NULL);
('del_gen_fail', 'DYN_delete_generator', 'dyn_del.e', NULL, 8, 213, NULL, 'ERASE RDB$GENERATORS failed', NULL, NULL);
('dyn_gen_not_found', 'DYN_delete_generator', 'dyn_del.e', NULL, 8, 214, NULL, 'Sequence @1 not found', NULL, NULL);
(NULL, 'change_backup_mode', 'dyn_mod.epp', NULL, 8, 215, NULL, 'Difference file is not defined', NULL, NULL);
(NULL, 'change_backup_mode', 'dyn_mod.epp', NULL, 8, 216, NULL, 'Difference file is already defined', NULL, NULL);
(NULL, 'DYN_define_difference', 'dyn_def.epp', NULL, 8, 217, NULL, 'Database is already in the physical backup mode', NULL, NULL);
(NULL, 'change_backup_mode', 'dyn_mod.epp', NULL, 8, 218, NULL, 'Database is not in the physical backup mode', NULL, NULL);
(NULL, NULL, 'dyn_def.epp', NULL, 8, 219, NULL, 'DEFINE COLLATION failed', NULL, NULL);
(NULL, NULL, 'dyn_def.epp', NULL, 8, 220, NULL, 'CREATE COLLATION statement is not supported in older versions of the database.  A backup and restore is required.', NULL, NULL);
('max_coll_per_charset', NULL, 'dyn_def.epp', NULL, 8, 221, NULL, 'Maximum number of collations per character set exceeded', NULL, NULL);
('invalid_coll_attr', NULL, 'dyn_def.epp', NULL, 8, 222, NULL, 'Invalid collation attributes', NULL, NULL);
(NULL, 'DYN_define_collation', 'dyn_def.epp', NULL, 8, 223, NULL, 'Collation @1 not installed for character set @2', NULL, NULL);
(NULL, 'DYN_modify_sql_field', 'dyn_mod.epp', NULL, 8, 224, NULL, 'Cannot use the internal domain @1 as new type for field @2', NULL, NULL);
(NULL, 'DYN_', 'dyn_def.epp/dyn_mod.epp', NULL, 8, 225, NULL, 'Default value is not allowed for array type in field @1', NULL, NULL);
(NULL, 'DYN_', 'dyn_def.epp/dyn_mod.epp', NULL, 8, 226, NULL, 'Default value is not allowed for array type in domain @1', NULL, NULL);
(NULL, 'DYN_UTIL_is_array', 'dyn_util.epp', NULL, 8, 227, NULL, 'DYN_UTIL_is_array failed for domain @1', NULL, NULL);
(NULL, 'DYN_UTIL_copy_domain', 'dyn_util.epp', NULL, 8, 228, NULL, 'DYN_UTIL_copy_domain failed for domain @1', NULL, NULL);
(NULL, 'dyn_mod.epp', 'DYN_modify_sql_field', NULL, 8, 229, NULL, 'Local column @1 doesn''t have a default', NULL, NULL);
(NULL, 'dyn_mod.epp', 'DYN_modify_sql_field', NULL, 8, 230, NULL, 'Local column @1 default belongs to domain @2', NULL, NULL);
(NULL, 'dyn_def.epp', 'DYN_define_file', NULL, 8, 231, NULL, 'File name is invalid', NULL, NULL);
('dyn_wrong_gtt_scope', 'DYN_define_constraint', 'dyn_def.e', NULL, 8, 232, NULL, '@1 cannot reference @2', NULL, NULL);
(NULL, 'dyn_mod.epp', 'DYN_modify_sql_field', NULL, 8, 233, NULL, 'Local column @1 is computed, cannot set a default value', NULL, NULL);
('del_coll_fail', 'DYN_delete_collation', 'dyn_del.epp', NULL, 8, 234, NULL, 'ERASE RDB$COLLATIONS failed', NULL, NULL);
('dyn_coll_used_table', NULL, 'dyn_del.epp', NULL, 8, 235, NULL, 'Collation @1 is used in table @2 (field name @3) and cannot be dropped', NULL, NULL);
('dyn_coll_used_domain', NULL, 'dyn_del.epp', NULL, 8, 236, NULL, 'Collation @1 is used in domain @2 and cannot be dropped', NULL, NULL);
('dyn_cannot_del_syscoll', 'DYN_delete_collation', 'dyn_del.epp', NULL, 8, 237, NULL, 'Cannot delete system collation', NULL, NULL);
('dyn_cannot_del_def_coll', 'DYN_delete_collation', 'dyn_del.epp', NULL, 8, 238, NULL, 'Cannot delete default collation of CHARACTER SET @1', NULL, NULL);
(NULL, NULL, 'dyn_del.epp', NULL, 8, 239, NULL, 'Domain @1 is used in procedure @2 (parameter name @3) and cannot be dropped', NULL, NULL);
(NULL, 'DYN_define_index', 'dyn_def.epp', NULL, 8, 240, NULL, 'Field @1 cannot be used twice in index @2', NULL, NULL);
('dyn_table_not_found', 'DYN_define_index', 'dyn_def.epp', NULL, 8, 241, NULL, 'Table @1 not found', NULL, NULL);
(NULL, 'DYN_define_index', 'dyn_def.epp', NULL, 8, 242, NULL, 'attempt to reference a view (@1) in a foreign key', NULL, NULL);
('dyn_coll_used_procedure', 'DYN_delete_collation', 'dyn_del.epp', NULL, 8, 243, NULL, 'Collation @1 is used in procedure @2 (parameter name @3) and cannot be dropped', NULL, NULL);
-- Do not change the arguments of the previous DYN messages.
-- Write the new DYN messages here.
('dyn_scale_too_big', 'check_update_numeric_type', 'dyn_mod.epp', NULL, 8, 244, NULL, 'New scale specified for column @1 must be at most @2.', NULL, NULL);
('dyn_precision_too_small', 'check_update_numeric_type', 'dyn_mod.epp', NULL, 8, 245, NULL, 'New precision specified for column @1 must be at least @2.', NULL, NULL);
(NULL, 'revoke_permission', 'dyn.epp', NULL, 8, 246, NULL, '@1 is not grantor of @2 on @3 to @4.', NULL, NULL);
('dyn_miss_priv_warning', 'revoke_permission', 'dyn.epp', NULL, 8, 247, NULL, 'Warning: @1 on @2 is not granted to @3.', NULL, NULL);
('dyn_ods_not_supp_feature', 'DYN_define_relation', 'dyn_def.epp', NULL, 8, 248, NULL, 'Feature ''@1'' is not supported in ODS @2.@3', NULL, NULL);
('dyn_cannot_addrem_computed', 'DYN_modify_sql_field', 'dyn_mod.epp', NULL, 8, 249, NULL, 'Cannot add or remove COMPUTED from column @1', NULL, NULL);
('dyn_no_empty_pw', 'dyn_user', 'dyn.epp', NULL, 8, 250, NULL, 'Password should not be empty string', NULL, NULL);
('dyn_dup_index', 'DYN_define_index', 'dyn_def.epp', NULL, 8, 251, NULL, 'Index @1 already exists', NULL, NULL);
('dyn_locksmith_use_granted', 'grant/revoke', 'dyn.epp', NULL, 8, 252, NULL, 'Only @1 or database owner can use GRANTED BY clause', NULL, NULL);
('dyn_dup_exception', 'DYN_define_exception', 'dyn_def.epp', NULL, 8, 253, NULL, 'Exception @1 already exists', NULL, NULL);
('dyn_dup_generator', 'DYN_define_generator', 'dyn_def.epp', NULL, 8, 254, NULL, 'Sequence @1 already exists', NULL, NULL);
(NULL, 'revoke_all', 'dyn.epp', NULL, 8, 255, NULL, 'ERASE RDB$USER_PRIVILEGES failed in REVOKE ALL ON ALL', NULL, NULL);
('dyn_package_not_found', NULL, 'DdlNodes.epp/PackageNodes.epp', NULL, 8, 256, NULL, 'Package @1 not found', NULL, NULL);
('dyn_schema_not_found', 'CommentOnNode::execute', 'DdlNodes.epp', NULL, 8, 257, NULL, 'Schema @1 not found', NULL, NULL);
('dyn_cannot_mod_sysproc', NULL, 'DdlNodes.epp', NULL, 8, 258, NULL, 'Cannot ALTER or DROP system procedure @1', NULL, NULL);
('dyn_cannot_mod_systrig', NULL, 'DdlNodes.epp', NULL, 8, 259, NULL, 'Cannot ALTER or DROP system trigger @1', NULL, NULL);
('dyn_cannot_mod_sysfunc', NULL, 'DdlNodes.epp', NULL, 8, 260, NULL, 'Cannot ALTER or DROP system function @1', NULL, NULL);
('dyn_invalid_ddl_proc', 'CreateAlterProcedureNode::compile', 'DdlNodes.epp', NULL, 8, 261, NULL, 'Invalid DDL statement for procedure @1', NULL, NULL);
('dyn_invalid_ddl_trig', 'CreateAlterTriggerNode::compile', 'DdlNodes.epp', NULL, 8, 262, NULL, 'Invalid DDL statement for trigger @1', NULL, NULL);
('dyn_funcnotdef_package', 'CreatePackageBodyNode::execute', 'PackageNodes.epp', NULL, 8, 263, NULL, 'Function @1 has not been defined on the package body @2', NULL, NULL);
('dyn_procnotdef_package', 'CreatePackageBodyNode::execute', 'PackageNodes.epp', NULL, 8, 264, NULL, 'Procedure @1 has not been defined on the package body @2', NULL, NULL);
('dyn_funcsignat_package', 'CreatePackageBodyNode::execute', 'PackageNodes.epp', NULL, 8, 265, NULL, 'Function @1 has a signature mismatch on package body @2', NULL, NULL);
('dyn_procsignat_package', 'CreatePackageBodyNode::execute', 'PackageNodes.epp', NULL, 8, 266, NULL, 'Procedure @1 has a signature mismatch on package body @2', NULL, NULL);
('dyn_defvaldecl_package', 'CreatePackageBodyNode::execute', 'PackageNodes.epp', NULL, 8, 267, NULL, 'Default values for parameters are allowed only in declaration of packaged procedure @1.@2', NULL, NULL);
('dyn_dup_function', 'DYN_define_function', 'dyn_def.epp', NULL, 8, 268, NULL, 'Function @1 already exists', NULL, NULL);
('dyn_package_body_exists', NULL, 'DdlNodes.epp/PackageNodes.epp', NULL, 8, 269, NULL, 'Package body @1 already exists', NULL, NULL);
('dyn_invalid_ddl_func', 'CreateAlterFunctionNode::compile', 'DdlNodes.epp', NULL, 8, 270, NULL, 'Invalid DDL statement for function @1', NULL, NULL);
('dyn_newfc_oldsyntax', 'DYN_modify_function', 'dyn_mod.epp', NULL, 8, 271, NULL, 'Cannot alter new style function @1 with ALTER EXTERNAL FUNCTION. Use ALTER FUNCTION instead.', NULL, NULL);
(NULL, 'DYN_delete_generator', 'dyn_del.epp', NULL, 8, 272, NULL, 'Cannot delete system generator @1', NULL, NULL);
(NULL, 'DYN_define_sql_field', 'dyn_def.epp', NULL, 8, 273, NULL, 'Identity column @1 of table @2 must be of exact number type with zero scale', NULL, NULL);
(NULL, 'DYN_modify_local_field', 'dyn_mod.epp', NULL, 8, 274, NULL, 'Identity column @1 of table @2 cannot be changed to NULLable', NULL, NULL);
(NULL, 'DYN_modify_sql_field', 'dyn_mod.epp', NULL, 8, 275, NULL, 'Identity column @1 of table @2 cannot have default value', NULL, NULL);
(NULL, 'DYN_modify_global_field', 'dyn_mod.epp', NULL, 8, 276, NULL, 'Domain @1 must be of exact number type with zero scale because it''s used in an identity column', NULL, NULL);
(NULL, 'DYN_UTIL_generate_generator_name', 'dyn_util.epp', NULL, 8, 277, NULL, 'Generation of generator name failed', NULL, NULL);
('dyn_func_param_not_found', 'OnCommentNode::execute', 'DdlNodes.epp', NULL, 8, 278, NULL, 'Parameter @1 in function @2 not found', NULL, NULL);
('dyn_routine_param_not_found', 'OnCommentNode::execute', 'DdlNodes.epp', NULL, 8, 279, NULL, 'Parameter @1 of routine @2 not found', NULL, NULL);
('dyn_routine_param_ambiguous', 'OnCommentNode::execute', 'DdlNodes.epp', NULL, 8, 280, NULL, 'Parameter @1 of routine @2 is ambiguous (found in both procedures and functions). Use a specifier keyword.', NULL, NULL);
('dyn_coll_used_function', 'DropCollationNode::execute', 'DdlNodes.epp', NULL, 8, 281, NULL, 'Collation @1 is used in function @2 (parameter name @3) and cannot be dropped', NULL, NULL);
('dyn_domain_used_function', 'DropDomainNode', 'DdlNodes.epp', NULL, 8, 282, NULL, 'Domain @1 is used in function @2 (parameter name @3) and cannot be dropped', NULL, NULL);
('dyn_alter_user_no_clause', 'CreateAlterUserNode', 'DdlNodes.epp', NULL, 8, 283, NULL, 'ALTER USER requires at least one clause to be specified', NULL, NULL);
(NULL, 'DYN_delete_role', 'dyn_del.epp', NULL, 8, 284, NULL, 'Cannot delete system SQL role @1', NULL, NULL);
(NULL, 'DdlNodes.epp', 'AlterRelationNode::modifyField', NULL, 8, 285, NULL, 'Column @1 is not an identity column', NULL, NULL);
('dyn_duplicate_package_item', NULL, 'PackageNodes.epp', NULL, 8, 286, NULL, 'Duplicate @1 @2', NULL, NULL);
('dyn_cant_modify_sysobj', NULL, 'DdlNodes.epp', NULL, 8, 287, NULL, 'System @1 @2 cannot be modified', NULL, 'Ex: System generator rdb$... cannot be modified');
('dyn_cant_use_zero_increment', NULL, 'DdlNodes.epp', NULL, 8, 288, NULL, 'INCREMENT BY 0 is an illegal option for sequence @1', NULL, NULL);
('dyn_cant_use_in_foreignkey', NULL, 'DdlNodes.epp', NULL, 8, 289, NULL, 'Can''t use @1 in FOREIGN KEY constraint', NULL, NULL);
COMMIT WORK;
-- TEST
(NULL, 'main', 'test.c', NULL, 11, 0, NULL, 'This is a modified text message', NULL, NULL);
(NULL, NULL, NULL, NULL, 11, 3, NULL, 'This is a test message', NULL, NULL);
-- Do not change the arguments of the previous TEST messages.
-- Write the new TEST messages here.
-- GBAK
(NULL, NULL, 'burp.c', NULL, 12, 0, NULL, 'could not locate appropriate error message', NULL, NULL);
('gbak_unknown_switch', NULL, 'burp.c', NULL, 12, 1, NULL, 'found unknown switch', NULL, NULL);
('gbak_page_size_missing', NULL, 'burp.c', NULL, 12, 2, NULL, 'page size parameter missing', NULL, NULL);
('gbak_page_size_toobig', NULL, 'burp.c', NULL, 12, 3, NULL, 'Page size specified (@1) greater than limit (16384 bytes)', NULL, NULL);
('gbak_redir_ouput_missing', NULL, 'burp.c', NULL, 12, 4, NULL, 'redirect location for output is not specified', NULL, NULL);
('gbak_switches_conflict', NULL, 'burp.c', NULL, 12, 5, NULL, 'conflicting switches for backup/restore', NULL, NULL);
('gbak_unknown_device', NULL, 'burp.c', NULL, 12, 6, NULL, 'device type @1 not known', NULL, NULL);
('gbak_no_protection', NULL, 'burp.c', NULL, 12, 7, NULL, 'protection is not there yet', NULL, NULL);
('gbak_page_size_not_allowed', NULL, 'burp.c', NULL, 12, 8, NULL, 'page size is allowed only on restore or create', NULL, NULL);
('gbak_multi_source_dest', NULL, 'burp.c', NULL, 12, 9, NULL, 'multiple sources or destinations specified', NULL, NULL);
('gbak_filename_missing', NULL, 'burp.c', NULL, 12, 10, NULL, 'requires both input and output filenames', NULL, NULL);
('gbak_dup_inout_names', NULL, 'burp.c', NULL, 12, 11, NULL, 'input and output have the same name.  Disallowed.', NULL, NULL);
('gbak_inv_page_size', NULL, 'burp.c', NULL, 12, 12, NULL, 'expected page size, encountered "@1"', NULL, NULL);
('gbak_db_specified', NULL, 'burp.c', NULL, 12, 13, NULL, 'REPLACE specified, but the first file @1 is a database', NULL, NULL);
('gbak_db_exists', NULL, 'burp.c', NULL, 12, 14, NULL, 'database @1 already exists.  To replace it, use the -REP switch', NULL, NULL);
('gbak_unk_device', NULL, 'burp.c', NULL, 12, 15, NULL, 'device type not specified', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 16, NULL, 'cannot create APOLLO tape descriptor file @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 17, NULL, 'cannot set APOLLO tape descriptor attribute for @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 18, NULL, 'cannot create APOLLO cartridge descriptor file @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 19, NULL, 'cannot close APOLLO tape descriptor file @1', NULL, NULL);
('gbak_blob_info_failed', NULL, 'burp.c', NULL, 12, 20, NULL, 'gds_$blob_info failed', NULL, NULL);
('gbak_unk_blob_item', NULL, 'burp.c', NULL, 12, 21, NULL, 'do not understand BLOB INFO item @1', NULL, NULL);
('gbak_get_seg_failed', NULL, 'burp.c', NULL, 12, 22, NULL, 'gds_$get_segment failed', NULL, NULL);
('gbak_close_blob_failed', NULL, 'burp.c', NULL, 12, 23, NULL, 'gds_$close_blob failed', NULL, NULL);
('gbak_open_blob_failed', NULL, 'burp.c', NULL, 12, 24, NULL, 'gds_$open_blob failed', NULL, NULL);
('gbak_put_blr_gen_id_failed', NULL, 'burp.c', NULL, 12, 25, NULL, 'Failed in put_blr_gen_id', NULL, NULL);
('gbak_unk_type', NULL, 'burp.c', NULL, 12, 26, NULL, 'data type @1 not understood', NULL, NULL);
('gbak_comp_req_failed', NULL, 'burp.c', NULL, 12, 27, NULL, 'gds_$compile_request failed', NULL, NULL);
('gbak_start_req_failed', NULL, 'burp.c', NULL, 12, 28, NULL, 'gds_$start_request failed', NULL, NULL);
('gbak_rec_failed', NULL, 'burp.c', NULL, 12, 29, NULL, 'gds_$receive failed', NULL, NULL);
('gbak_rel_req_failed', NULL, 'burp.c', NULL, 12, 30, NULL, 'gds_$release_request failed', NULL, NULL);
('gbak_db_info_failed', NULL, 'burp.c', NULL, 12, 31, NULL, 'gds_$database_info failed', NULL, NULL);
('gbak_no_db_desc', NULL, 'burp.c', NULL, 12, 32, NULL, 'Expected database description record', NULL, NULL);
('gbak_db_create_failed', NULL, 'burp.c', NULL, 12, 33, NULL, 'failed to create database @1', NULL, NULL);
('gbak_decomp_len_error', NULL, 'burp.c', NULL, 12, 34, NULL, 'RESTORE: decompression length error', NULL, NULL);
('gbak_tbl_missing', NULL, 'burp.c', NULL, 12, 35, NULL, 'cannot find table @1', NULL, NULL);
('gbak_blob_col_missing', NULL, 'burp.c', NULL, 12, 36, NULL, 'Cannot find column for BLOB', NULL, NULL);
('gbak_create_blob_failed', NULL, 'burp.c', NULL, 12, 37, NULL, 'gds_$create_blob failed', NULL, NULL);
('gbak_put_seg_failed', NULL, 'burp.c', NULL, 12, 38, NULL, 'gds_$put_segment failed', NULL, NULL);
('gbak_rec_len_exp', NULL, 'burp.c', NULL, 12, 39, NULL, 'expected record length', NULL, NULL);
('gbak_inv_rec_len', NULL, 'burp.c', NULL, 12, 40, NULL, 'wrong length record, expected @1 encountered @2', NULL, NULL);
('gbak_exp_data_type', NULL, 'burp.c', NULL, 12, 41, NULL, 'expected data attribute', NULL, NULL);
('gbak_gen_id_failed', NULL, 'burp.c', NULL, 12, 42, NULL, 'Failed in store_blr_gen_id', NULL, NULL);
('gbak_unk_rec_type', NULL, 'burp.c', NULL, 12, 43, NULL, 'do not recognize record type @1', NULL, NULL);
('gbak_inv_bkup_ver', NULL, 'burp.c', 'obsolete', 12, 44, NULL, 'Expected backup version 1..10.  Found @1', NULL, NULL);
('gbak_missing_bkup_desc', NULL, 'burp.c', NULL, 12, 45, NULL, 'expected backup description record', NULL, NULL);
('gbak_string_trunc', NULL, 'burp.c', NULL, 12, 46, NULL, 'string truncated', NULL, NULL);
('gbak_cant_rest_record', NULL, 'burp.c', NULL, 12, 47, NULL, 'warning -- record could not be restored', NULL, NULL);
('gbak_send_failed', NULL, 'burp.c', NULL, 12, 48, NULL, 'gds_$send failed', NULL, NULL);
('gbak_no_tbl_name', NULL, 'burp.c', NULL, 12, 49, NULL, 'no table name for data', NULL, NULL);
('gbak_unexp_eof', NULL, 'burp.c', NULL, 12, 50, NULL, 'unexpected end of file on backup file', NULL, NULL);
('gbak_db_format_too_old', NULL, 'burp.c', NULL, 12, 51, NULL, 'database format @1 is too old to restore to', NULL, NULL);
('gbak_inv_array_dim', NULL, 'burp.c', NULL, 12, 52, NULL, 'array dimension for column @1 is invalid', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 53, NULL, 'expected array version number @1 but instead found @2', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 54, NULL, 'expected array dimension @1 but instead found @2', NULL, NULL);
('gbak_xdr_len_expected', NULL, 'burp.c', NULL, 12, 55, NULL, 'Expected XDR record length', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 56, NULL, 'Unexpected I/O error while @1 backup file', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 57, NULL, 'adding file @1, starting at page @2', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 58, NULL, 'array', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 59, NULL, 'backup', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 60, NULL, '    @1B(ACKUP_DATABASE)    backup database to file', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 61, NULL, '		backup file is compressed', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 62, NULL, '    @1D(EVICE)             backup file device type on APOLLO (CT or MT)', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 63, NULL, '    @1M(ETA_DATA)          backup or restore metadata only', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 64, NULL, 'blob', NULL, NULL);
('gbak_open_bkup_error', NULL, 'burp.c', NULL, 12, 65, NULL, 'cannot open backup file @1', NULL, NULL);
('gbak_open_error', NULL, 'burp.c', NULL, 12, 66, NULL, 'cannot open status and error output file @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 67, NULL, 'closing file, committing, and finishing', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 68, NULL, 'committing metadata', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 69, NULL, 'commit failed on table @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 70, NULL, 'committing secondary files', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 71, NULL, 'creating index @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 72, NULL, 'committing data for table @1', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 73, NULL, '    @1C(REATE_DATABASE)    create database from backup file (restore)', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 74, NULL, 'created database @1, page_size @2 bytes', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 75, NULL, 'creating file @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 76, NULL, 'creating indexes', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 77, NULL, 'database @1 has a page size of @2 bytes.', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 78, NULL, '    @1I(NACTIVE)           deactivate indexes during restore', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 79, NULL, 'do not understand BLOB INFO item @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 80, NULL, 'do not recognize @1 attribute @2 -- continuing', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 81, NULL, 'error accessing BLOB column @1 -- continuing', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 82, NULL, 'Exiting before completion due to errors', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 83, NULL, 'Exiting before completion due to errors', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 84, NULL, 'column', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 85, NULL, 'file', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 86, NULL, 'file length', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 87, NULL, 'filter', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 88, NULL, 'finishing, closing, and going home', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 89, NULL, 'function', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 90, NULL, 'function argument', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 91, NULL, 'gbak version @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 92, NULL, 'domain', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 93, NULL, 'index', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 94, NULL, 'trigger @1 is invalid', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 95, NULL, 'legal switches are:', NULL, NULL);
(NULL, 'add_files', 'restore.epp', NULL, 12, 96, NULL, 'length given for initial file (@1) is less than minimum (@2)', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 97, NULL, '    @1E(XPAND)             no data compression', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 98, NULL, '    @1L(IMBO)              ignore transactions in limbo', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 99, NULL, '    @1O(NE_AT_A_TIME)      restore one table at a time', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 100, NULL, 'opened file @1', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 101, NULL, '    @1P(AGE_SIZE)          override default page size', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 102, NULL, 'page size', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 103, NULL, 'page size specified (@1 bytes) rounded up to @2 bytes', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 104, NULL, '    @1Z                    print version number', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 105, NULL, 'privilege', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 106, NULL, '     @1 records ignored', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 107, NULL, '   @1 records restored', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 108, NULL, '@1 records written', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 109, NULL, '    @1Y  <path>            redirect/suppress status message output', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 110, NULL, 'Reducing the database page size from @1 bytes to @2 bytes', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 111, NULL, 'table', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 112, NULL, '    @1REP(LACE_DATABASE)   replace database from backup file (restore)', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 113, NULL, '    @1V(ERIFY)             report each action taken', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 114, NULL, 'restore failed for record in table @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 115, NULL, '    restoring column @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 116, NULL, '    restoring file @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 117, NULL, '    restoring filter @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 118, NULL, 'restoring function @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 119, NULL, '    restoring argument for function @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 120, NULL, '     restoring gen id value of: @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 121, NULL, 'restoring domain @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 122, NULL, '    restoring index @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 123, NULL, '    restoring privilege for user @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 124, NULL, 'restoring data for table @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 125, NULL, 'restoring security class @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 126, NULL, '    restoring trigger @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 127, NULL, '    restoring trigger message for @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 128, NULL, '    restoring type @1 for column @2', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 129, NULL, 'started transaction', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 130, NULL, 'starting transaction', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 131, NULL, 'security class', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 132, NULL, 'switches can be abbreviated to the unparenthesized characters', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 133, NULL, 'transportable backup -- data in XDR format', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 134, NULL, 'trigger', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 135, NULL, 'trigger message', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 136, NULL, 'trigger type', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 137, NULL, 'unknown switch "@1"', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 138, NULL, 'validation error on column in table @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 139, NULL, '    Version(s) for database "@1"', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 140, NULL, 'view', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 141, NULL, '    writing argument for function @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 142, NULL, '    writing data for table @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 143, NULL, '     writing gen id of: @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 144, NULL, '         writing column @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 145, NULL, '    writing filter @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 146, NULL, 'writing filters', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 147, NULL, '    writing function @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 148, NULL, 'writing functions', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 149, NULL, '    writing domain @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 150, NULL, 'writing domains', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 151, NULL, '    writing index @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 152, NULL, '    writing privilege for user @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 153, NULL, '    writing table @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 154, NULL, 'writing tables', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 155, NULL, '    writing security class @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 156, NULL, '    writing trigger @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 157, NULL, '    writing trigger message for @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 158, NULL, 'writing trigger messages', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 159, NULL, 'writing triggers', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 160, NULL, '    writing type @1 for column @2', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 161, NULL, 'writing types', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 162, NULL, 'writing shadow files', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 163, NULL, '    writing shadow file @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 164, NULL, 'writing id generators', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 165, NULL, '    writing generator @1 value @2', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 166, NULL, 'readied database @1 for backup', NULL, NULL);
(NULL, 'get_relation', 'restore.epp', NULL, 12, 167, NULL, 'restoring table @1', NULL, NULL);
(NULL, NULL, 'burp.c', NULL, 12, 168, NULL, 'type', NULL, NULL);
(NULL, 'BURP_print', 'burp.c', NULL, 12, 169, NULL, 'gbak:', NULL, NULL);
(NULL, NULL, 'restore.e', NULL, 12, 170, NULL, 'committing metadata for table @1', NULL, NULL);
(NULL, NULL, 'restore.e', NULL, 12, 171, NULL, 'error committing metadata for table @1', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 172, NULL, '    @1K(ILL)               restore without creating shadows', NULL, NULL);
(NULL, 'get_index', 'restore.e', NULL, 12, 173, NULL, 'cannot commit index @1', NULL, NULL);
(NULL, 'add_files', 'restore.e', NULL, 12, 174, NULL, 'cannot commit files', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 175, NULL, '    @1T(RANSPORTABLE)      transportable backup -- data in XDR format', NULL, NULL);
(NULL, 'BACKUP_backup', 'backup.e', NULL, 12, 176, NULL, 'closing file, committing, and finishing. @1 bytes written', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 177, NULL, '    @1G(ARBAGE_COLLECT)    inhibit garbage collection', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 178, NULL, '    @1IG(NORE)             ignore bad checksums', NULL, NULL);
(NULL, 'put_index', 'backup.e', NULL, 12, 179, NULL, '	column @1 used in index @2 seems to have vanished', NULL, NULL);
(NULL, 'put_index', 'backup.e', NULL, 12, 180, NULL, 'index @1 omitted because @2 of the expected @3 keys were found', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 181, NULL, '    @1FA(CTOR)             blocking factor', NULL, NULL);
('gbak_missing_block_fac', 'main', 'burp.c', NULL, 12, 182, NULL, 'blocking factor parameter missing', NULL, NULL);
('gbak_inv_block_fac', 'main', 'burp.c', NULL, 12, 183, NULL, 'expected blocking factor, encountered "@1"', NULL, NULL);
('gbak_block_fac_specified', 'main', 'burp.c', NULL, 12, 184, NULL, 'a blocking factor may not be used in conjunction with device CT', NULL, NULL);
(NULL, 'get_generator', 'RESTORE.E', NULL, 12, 185, NULL, 'restoring generator @1 value: @2', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 186, NULL, '    @1OL(D_DESCRIPTIONS)   save old style metadata descriptions', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 187, NULL, '    @1N(O_VALIDITY)        do not restore database validity conditions', NULL, NULL);
('gbak_missing_username', 'main()', 'burp.c', NULL, 12, 188, NULL, 'user name parameter missing', NULL, NULL);
('gbak_missing_password', 'main()', 'burp.c', NULL, 12, 189, NULL, 'password parameter missing', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 190, NULL, '    @1PAS(SWORD)           Firebird password', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 191, NULL, '    @1USER                 Firebird user name', NULL, NULL);
(NULL, 'BACKUP_backup', 'backup.e', NULL, 12, 192, NULL, 'writing stored procedures', NULL, NULL);
(NULL, 'write_procedures', 'backup.e', NULL, 12, 193, NULL, 'writing stored procedure @1', NULL, NULL);
(NULL, 'write_procedure_prms', 'backup.e', NULL, 12, 194, NULL, 'writing parameter @1 for stored procedure', NULL, NULL);
(NULL, 'get_procedure', 'restore.e', NULL, 12, 195, NULL, 'restoring stored procedure @1', NULL, NULL);
(NULL, 'get_procedure_prm', 'restore.e', NULL, 12, 196, NULL, '    restoring parameter @1 for stored procedure', NULL, NULL);
(NULL, 'BACKUP_backup', 'backup.e', NULL, 12, 197, NULL, 'writing exceptions', NULL, NULL);
(NULL, 'write_exceptions', 'backup.e', NULL, 12, 198, NULL, 'writing exception @1', NULL, NULL);
(NULL, 'get_exception', 'restore.e', NULL, 12, 199, NULL, 'restoring exception @1', NULL, NULL);
('gbak_missing_skipped_bytes', NULL, 'burp.c', NULL, 12, 200, NULL, ' missing parameter for the number of bytes to be skipped', NULL, NULL);
('gbak_inv_skipped_bytes', NULL, 'burp.c', NULL, 12, 201, NULL, 'expected number of bytes to be skipped, encountered "@1"', NULL, NULL);
(NULL, NULL, 'restore.e', NULL, 12, 202, NULL, 'adjusting an invalid decompression length from @1 to @2', NULL, NULL);
(NULL, NULL, 'restore.e', NULL, 12, 203, NULL, 'skipped @1 bytes after reading a bad attribute @2', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 204, NULL, '    @1S(KIP_BAD_DATA)      skip number of bytes after reading bad data', NULL, NULL);
(NULL, NULL, 'restore.e', NULL, 12, 205, NULL, 'skipped @1 bytes looking for next valid attribute, encountered attribute @2', NULL, NULL);
(NULL, 'write_rel_constraints', 'backup.e', NULL, 12, 206, NULL, 'writing table constraints', NULL, NULL);
(NULL, 'write_rel_constraints', 'backup.e', NULL, 12, 207, NULL, 'writing constraint @1', NULL, NULL);
(NULL, NULL, 'restore.e', NULL, 12, 208, NULL, 'table constraint', NULL, NULL);
(NULL, 'BACKUP_backup', 'backup.e', NULL, 12, 209, NULL, 'writing referential constraints', NULL, NULL);
(NULL, 'BACKUP_backup', 'GBAK', NULL, 12, 210, NULL, 'writing check constraints', NULL, NULL);
('msgVerbose_write_charsets', 'write_charsets', 'backup.e', NULL, 12, 211, NULL, 'writing character sets', NULL, NULL);
('msgVerbose_write_collations', 'write_collations', 'backup.e', NULL, 12, 212, NULL, 'writing collations', NULL, NULL);
('gbak_err_restore_charset', 'get_charset', 'restore.e', NULL, 12, 213, NULL, 'character set', NULL, NULL);
('msgVerbose_restore_charset', 'get_charset', 'restore.e', NULL, 12, 214, NULL, 'writing character set @1', NULL, NULL);
('gbak_err_restore_collation', 'get_collation', 'restore.e', NULL, 12, 215, NULL, 'collation', NULL, NULL);
('msgVerbose_restore_collation', 'get_collation', 'restore.e', NULL, 12, 216, NULL, 'writing collation @1', NULL, NULL);
('gbak_read_error', 'MVOL_read', 'burp.c', NULL, 12, 220, NULL, 'Unexpected I/O error while reading from backup file', NULL, NULL);
('gbak_write_error', 'MVOL_write', 'burp.c', NULL, 12, 221, NULL, 'Unexpected I/O error while writing to backup file', NULL, NULL);
(NULL, 'next_volume', 'mvol.c', NULL, 12, 222, NULL, '

Could not open file name "@1"', NULL, NULL);
(NULL, 'next_volume', 'mvol.c', NULL, 12, 223, NULL, '

Could not write to file "@1"', NULL, NULL);
(NULL, 'next_volume', 'mvol.c', NULL, 12, 224, NULL, '

Could not read from file "@1"', NULL, NULL);
(NULL, 'prompt_for_name', 'mvol.c', NULL, 12, 225, NULL, 'Done with volume #@1, "@2"', NULL, NULL);
(NULL, 'prompt_for_name', 'mvol.c', NULL, 12, 226, NULL, '	Press return to reopen that file, or type a new
	name followed by return to open a different file.', NULL, NULL);
(NULL, 'prompt_for_name', 'mvol.c', NULL, 12, 227, NULL, 'Type a file name to open and hit return', NULL, NULL);
(NULL, 'prompt_for_name', 'mvol.c', 'This prompt should be preceded by 2 blank spaces and followed by 1: "  Name: "', 12, 228, NULL, '  Name: ', NULL, NULL);
(NULL, 'prompt_for_name', 'mvol.c', NULL, 12, 229, NULL, '

ERROR: Backup incomplete', NULL, NULL);
(NULL, 'read_header', 'mvol.c', NULL, 12, 230, NULL, 'Expected backup start time @1, found @2', NULL, NULL);
(NULL, 'read_header', 'mvol.c', NULL, 12, 231, NULL, 'Expected backup database @1, found @2', NULL, NULL);
(NULL, 'read_header', 'mvol.c', NULL, 12, 232, NULL, 'Expected volume number @1, found volume @2', NULL, NULL);
('gbak_db_in_use', 'open_files', 'burp.c', NULL, 12, 233, NULL, 'could not drop database @1 (database might be in use)', NULL, NULL);
(NULL, 'get_security_class', 'restore.e', NULL, 12, 234, NULL, 'Skipped bad security class entry: @1', NULL, NULL);
(NULL, 'convert_sub_type', 'restore.e', NULL, 12, 235, NULL, 'Unknown V3 SUB_TYPE: @1 in FIELD: @2.', NULL, NULL);
(NULL, 'cvt_v3_to_v4_intl', 'restore.e', NULL, 12, 236, NULL, 'Converted V3 sub_type: @1 to character_set_id: @2 and collate_id: @3.', NULL, NULL);
(NULL, 'cvt_v3_to_v4_intl', 'restore.e', NULL, 12, 237, NULL, 'Converted V3 scale: @1 to character_set_id: @2 and callate_id: @3.', NULL, NULL);
('gbak_sysmemex', 'MISC_alloc_memory', 'misc.c', NULL, 12, 238, NULL, 'System memory exhausted', NULL, NULL);
(NULL, 'main()', 'burp.c', NULL, 12, 239, NULL, '    @1NT                   Non-Transportable backup file format', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.e', NULL, 12, 240, NULL, 'Index "@1" failed to activate because:', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.e', NULL, 12, 241, NULL, '  The unique index has duplicate values or NULLs.', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.e', NULL, 12, 242, NULL, '  Delete or Update duplicate values or NULLs, and activate index with', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.e', NULL, 12, 243, NULL, '  ALTER INDEX "@1" ACTIVE;', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.e', NULL, 12, 244, NULL, '  Not enough disk space to create the sort file for an index.', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.e', NULL, 12, 245, NULL, '  Set the TMP environment variable to a directory on a filesystem that does have enough space, and activate index with', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.e', NULL, 12, 246, NULL, 'Database is not online due to failure to activate one or more indices.', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.e', NULL, 12, 247, NULL, 'Run gfix -online to bring database online without active indices.', NULL, NULL);
('write_role_1', 'BACKUP_backup', 'backup.e', NULL, 12, 248, NULL, 'writing SQL roles', NULL, NULL);
('write_role_2', 'write_sql_roles', 'backup.e', NULL, 12, 249, NULL, '    writing SQL role: @1', NULL, NULL);
('gbak_restore_role_failed', 'get_sql_roles', 'restore.e', NULL, 12, 250, NULL, 'SQL role', NULL, NULL);
('restore_role', 'get_sql_roles', 'restore.e', NULL, 12, 251, NULL, '    restoring SQL role: @1', NULL, NULL);
('gbak_role_op', 'burp_usage', 'burp.c', NULL, 12, 252, NULL, '    @1RO(LE)               Firebird SQL role', NULL, NULL);
('gbak_role_op_missing', 'BURP_gbak', 'burp.c', NULL, 12, 253, NULL, 'SQL role parameter missing', NULL, NULL);
('gbak_convert_ext_tables', 'burp_usage', 'burp.c', NULL, 12, 254, NULL, '    @1CO(NVERT)            backup external files as tables', NULL, NULL);
('gbak_warning', 'BURP_print_warning', 'burp.c', NULL, 12, 255, NULL, 'gbak: WARNING:', NULL, NULL);
('gbak_error', 'BURP_print_status', 'burp.c', NULL, 12, 256, NULL, 'gbak: ERROR:', NULL, NULL);
('gbak_page_buffers', 'burp_usage', 'burp.c', NULL, 12, 257, NULL, '    @1BU(FFERS)            override page buffers default', NULL, NULL);
('gbak_page_buffers_missing', 'BURP_gbak', 'burp.c', NULL, 12, 258, NULL, 'page buffers parameter missing', NULL, NULL);
('gbak_page_buffers_wrong_param', 'BURP_gbak', 'burp.c', NULL, 12, 259, NULL, 'expected page buffers, encountered "@1"', NULL, NULL);
('gbak_page_buffers_restore', 'BURP_gbak', 'burp.cpp', NULL, 12, 260, NULL, 'page buffers is allowed only on restore or create', NULL, NULL);
(NULL, 'next_volume', 'mvol.c', NULL, 12, 261, NULL, 'Starting with volume #@1, "@2"', NULL, NULL);
('gbak_inv_size', 'open_files', 'burp.c', NULL, 12, 262, NULL, 'size specification either missing or incorrect for file @1', NULL, NULL);
('gbak_file_outof_sequence', 'open_files', 'burp.c', NULL, 12, 263, NULL, 'file @1 out of sequence', NULL, NULL);
('gbak_join_file_missing', 'open_files', 'burp.c', NULL, 12, 264, NULL, 'can''t join -- one of the files missing', NULL, NULL);
('gbak_stdin_not_supptd', 'open_files', 'burp.c', NULL, 12, 265, NULL, ' standard input is not supported when using join operation', NULL, NULL);
('gbak_stdout_not_supptd', 'open_files', 'burp.c', NULL, 12, 266, NULL, 'standard output is not supported when using split operation or in verbose mode', NULL, NULL);
('gbak_bkup_corrupt', 'open_files', 'burp.c', NULL, 12, 267, NULL, 'backup file @1 might be corrupt', NULL, NULL);
('gbak_unk_db_file_spec', 'open_files', 'burp.c', NULL, 12, 268, NULL, 'database file specification missing', NULL, NULL);
('gbak_hdr_write_failed', 'MVOL_init_write', 'mvol.c', NULL, 12, 269, NULL, 'can''t write a header record to file @1', NULL, NULL);
('gbak_disk_space_ex', 'MVOL_write', 'mvol.c', NULL, 12, 270, NULL, 'free disk space exhausted', NULL, NULL);
('gbak_size_lt_min', 'open_files', 'burp.c', NULL, 12, 271, NULL, 'file size given (@1) is less than minimum allowed (@2)', NULL, NULL);
(NULL, 'MVOL_write', 'mvol.c', NULL, 12, 272, NULL, 'Warning -- free disk space exhausted for file @1, the rest of the bytes (@2) will be written to file @3', NULL, NULL);
('gbak_svc_name_missing', 'BURP_gbak', 'burp.c', NULL, 12, 273, NULL, 'service name parameter missing', NULL, NULL);
('gbak_not_ownr', 'open_files()', 'burp.c', NULL, 12, 274, NULL, 'Cannot restore over current database, must be SYSDBA or owner of the existing database.', NULL, NULL);
(NULL, NULL, NULL, NULL, 12, 275, NULL, '', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 276, NULL, '    @1USE_(ALL_SPACE)      do not reserve space for record versions', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 277, NULL, '    @1SE(RVICE)            use services manager', NULL, NULL);
('gbak_opt_mode', 'burp_usage', 'burp.c', NULL, 12, 278, NULL, '    @1MO(DE) <access>      "read_only" or "read_write" access', NULL, NULL);
('gbak_mode_req', 'BURP_gbak', 'burp.c', NULL, 12, 279, NULL, '"read_only" or "read_write" required', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.e', NULL, 12, 280, NULL, 'setting database to read-only access', NULL, NULL);
('gbak_just_data', 'main', 'burp.c', NULL, 12, 281, NULL, 'just data ignore all constraints etc.', NULL, NULL);
('gbak_data_only', 'restore', 'restore.e', NULL, 12, 282, NULL, 'restoring data only ignoring foreign key, unique, not null & other constraints', NULL, NULL);
(NULL, 'BACKUP_backup', 'backup.epp', NULL, 12, 283, NULL, 'closing file, committing, and finishing. @1 bytes written', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 284, NULL, +++
'    @1R(ECREATE_DATABASE) [O(VERWRITE)] create (or replace if OVERWRITE used)\n				database from backup file (restore)', NULL, NULL);
('gbak_activating_idx', 'RESTORE_restore', 'restore.epp', NULL, 12, 285, NULL, '    activating and creating deferred index @1', NULL, NULL);
(NULL, 'get_chk_constraint', 'restore.epp', NULL, 12, 286, NULL, 'check constraint', NULL, NULL);
(NULL, 'get_exception', 'restore.epp', NULL, 12, 287, NULL, 'exception', NULL, NULL);
(NULL, 'get_field_dimensions', 'restore.epp', NULL, 12, 288, NULL, 'array dimensions', NULL, NULL);
(NULL, 'get_generator', 'restore.epp', NULL, 12, 289, NULL, 'generator', NULL, NULL);
(NULL, 'get_procedure', 'restore.epp', NULL, 12, 290, NULL, 'procedure', NULL, NULL);
(NULL, 'get_procedure_prm', 'restore.epp', NULL, 12, 291, NULL, 'procedure parameter', NULL, NULL);
(NULL, 'get_ref_constraint', 'restore.epp', NULL, 12, 292, NULL, 'referential constraint', NULL, NULL);
(NULL, 'get_type', 'restore.epp', NULL, 12, 293, NULL, 'type (in RDB$TYPES)', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 294, NULL, '    @1NOD(BTRIGGERS)       do not run database triggers', NULL, NULL);
-- Do not change the arguments of the previous GBAK messages.
-- Write the new GBAK messages here.
(NULL, NULL, 'burp.cpp', NULL, 12, 295, NULL, '    @1TRU(STED)            use trusted authentication', NULL, NULL);
('write_map_1', 'BACKUP_backup', 'backup.epp', NULL, 12, 296, NULL, 'writing names mapping', NULL, NULL);
('write_map_2', 'write_mapping', 'backup.epp', NULL, 12, 297, NULL, '    writing map for @1', NULL, NULL);
('get_map_1', 'get_mapping', 'restore.epp', NULL, 12, 298, NULL, '    restoring map for @1', NULL, NULL);
('get_map_2', 'get_mapping', 'restore.epp', NULL, 12, 299, NULL, 'name mapping', NULL, NULL);
('get_map_3', 'get_mapping', 'restore.epp', NULL, 12, 300, NULL, 'cannot restore arbitrary mapping', NULL, NULL);
('get_map_4', 'get_mapping', 'restore.epp', NULL, 12, 301, NULL, 'restoring names mapping', NULL, NULL);
(NULL, 'burp_usage' 'burp.cpp', NULL, 12, 302, NULL, '    @1FIX_FSS_D(ATA)       fix malformed UNICODE_FSS data', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 303, NULL, '    @1FIX_FSS_M(ETADATA)   fix malformed UNICODE_FSS metadata', NULL, NULL);
(NULL, 'BURP_gbak', 'burp.cpp', NULL, 12, 304, NULL, 'Character set parameter missing', NULL, NULL);
(NULL, 'restore', 'restore.epp', NULL, 12, 305, NULL, 'Character set @1 not found', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 306, NULL, '    @1FE(TCH_PASSWORD)     fetch password from file', NULL, NULL);
(NULL, 'BURP_gbak', 'burp.cpp', NULL, 12, 307, NULL, 'too many passwords provided', NULL, NULL);
(NULL, 'BURP_gbak', 'burp.cpp', NULL, 12, 308, NULL, 'could not open password file @1, errno @2', NULL, NULL);
(NULL, 'BURP_gbak', 'burp.cpp', NULL, 12, 309, NULL, 'could not read password file @1, errno @2', NULL, NULL);
(NULL, 'BURP_gbak', 'burp.cpp', NULL, 12, 310, NULL, 'empty password file @1', NULL, NULL);
(NULL, 'get_exception', 'restore.epp', NULL, 12, 311, NULL, 'Attribute @1 was already processed for exception @2', NULL, NULL);
(NULL, 'get_exception', 'restore.epp', NULL, 12, 312, NULL, 'Skipping attribute @1 because the message already exists for exception @2', NULL, NULL);
(NULL, 'get_exception', 'restore.epp', NULL, 12, 313, NULL, 'Trying to recover from unexpected attribute @1 due to wrong message length for exception @2', NULL, NULL);
(NULL, 'put_exception', 'backup.epp', NULL, 12, 314, NULL, 'Attribute not specified for storing text bigger than 255 bytes', NULL, NULL);
(NULL, 'put_exception', 'backup.epp', NULL, 12, 315, NULL, 'Unable to store text bigger than 65536 bytes', NULL, NULL);
(NULL, 'fix_security_class_name', 'restore.epp', NULL, 12, 316, NULL, 'Failed while fixing the security class name', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 317, NULL, 'Usage:', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 318, NULL, '     gbak -b <db set> <backup set> [backup options] [general options]', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 319, NULL, '     gbak -c <backup set> <db set> [restore options] [general options]', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 320, NULL, '     <db set> = <database> | <db1 size1>...<dbN> (size in db pages)', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 321, NULL, '     <backup set> = <backup> | <bk1 size1>...<bkN> (size in bytes = n[K|M|G])', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 322, NULL, '     -recreate overwrite and -replace can be used instead of -c', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 323, NULL, 'backup options are:', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 324, NULL, 'restore options are:', NULL, NULL);
(NULL, 'burp_usage', 'burp.cpp', NULL, 12, 325, NULL, 'general options are:', NULL, NULL);
('gbak_missing_interval', 'api_gbak/gbak', 'burp.cpp', NULL, 12, 326, NULL, 'verbose interval value parameter missing', NULL, NULL);
('gbak_wrong_interval', 'api_gbak/gbak', 'burp.cpp', NULL, 12, 327, NULL, 'verbose interval value cannot be smaller than @1', NULL, NULL);
(NULL, 'burp_usage' 'burp.cpp', NULL, 12, 328, NULL, '    @1VERBI(NT) <n>        verbose information with explicit interval', NULL, NULL);
('gbak_verify_verbint', 'api_gbak/gbak', 'burp.cpp', NULL, 12, 329, NULL, 'verify (verbose) and verbint options are mutually exclusive', NULL, NULL);
('gbak_option_only_restore', 'gbak', 'burp.cpp', NULL, 12, 330, NULL, 'option -@1 is allowed only on restore or create', NULL, NULL);
('gbak_option_only_backup', 'gbak', 'burp.cpp', NULL, 12, 331, NULL, 'option -@1 is allowed only on backup', NULL, NULL);
('gbak_option_conflict', 'gbak', 'burp.cpp', NULL, 12, 332, NULL, 'options -@1 and -@2 are mutually exclusive', NULL, NULL);
('gbak_param_conflict', 'gbak', 'burp.cpp', NULL, 12, 333, NULL, 'parameter for option -@1 was already specified with value "@2"', NULL, NULL);
('gbak_option_repeated', 'gbak', 'burp.cpp', NULL, 12, 334, NULL, 'option -@1 was already specified', NULL, NULL);
(NULL, 'write_packages', 'backup.epp', NULL, 12, 335, NULL, 'writing package @1', NULL, NULL);
(NULL, 'BACKUP_backup', 'backup.epp', NULL, 12, 336, NULL, 'writing packages', NULL, NULL);
(NULL, 'get_package', 'restore.epp', NULL, 12, 337, NULL, 'restoring package @1', NULL, NULL);
(NULL, 'get_package', 'restore.epp', NULL, 12, 338, NULL, 'package', NULL, NULL);
('gbak_max_dbkey_recursion', 'update_view_dbkey_lengths', 'restore.epp', NULL, 12, 339, NULL, 'dependency depth greater than @1 for view @2', NULL, NULL);
('gbak_max_dbkey_length', 'update_view_dbkey_lengths', 'restore.epp', NULL, 12, 340, NULL, 'value greater than @1 when calculating length of rdb$db_key for view @2', NULL, NULL);
('gbak_invalid_metadata', 'general_on_error', 'restore.epp', NULL, 12, 341, NULL, 'Invalid metadata detected. Use -FIX_FSS_METADATA option.', NULL, NULL);
('gbak_invalid_data', 'get_data', 'restore.epp', NULL, 12, 342, NULL, 'Invalid data detected. Use -FIX_FSS_DATA option.', NULL, NULL);
(NULL, 'put_asciz', 'backup.epp', NULL, 12, 343, NULL, 'text for attribute @1 is too large in @2, truncating to @3 bytes', NULL, NULL);
('gbak_inv_bkup_ver2', 'restore.epp', 'burp.cpp', 'do not change the param order', 12, 344, NULL, 'Expected backup version @2..@3.  Found @1', NULL, NULL);
(NULL, 'write_relations', 'backup.epp', NULL, 12, 345, NULL, '    writing view @1', NULL, NULL);
(NULL, 'get_relation', 'restore.epp', NULL, 12, 346, NULL, '    table @1 is a view', NULL, NULL);
(NULL, 'write_secclasses', 'backup.epp', NULL, 12, 347, NULL, 'writing security classes', NULL, NULL);
('gbak_db_format_too_old2', 'BACKUP_backup', 'backup.epp', NULL, 12, 348, NULL, 'database format @1 is too old to backup', NULL, NULL);
(NULL, 'restore', 'restore.epp', NULL, 12, 349, NULL, 'backup version is @1', NULL, NULL);
(NULL, 'fix_system_generators', 'restore.epp', NULL, 12, 350, NULL, 'fixing system generators', NULL, NULL);
(NULL, 'BURP_abort', 'burp.cpp', NULL, 12, 351, NULL, 'Error closing database, but backup file is OK', NULL, NULL);
(NULL, NULL, 'restore.epp', NULL, 12, 352, NULL, 'database', NULL, NULL);
(NULL, 'get_mapping', 'restore.epp', NULL, 12, 353, NULL, 'required mapping attributes are missing in backup file', NULL, NULL);
(NULL, NULL, 'burp.cpp', NULL, 12, 354, NULL, 'missing regular expression to skip tables', NULL, NULL);
(NULL, 'burp_usage', 'burp.c', NULL, 12, 355, NULL, '    @1SKIP_D(ATA)          skip data for table', NULL, NULL);
(NULL, NULL, 'burp.cpp', NULL, 12, 356, NULL, 'regular expression to skip tables was already set', NULL, NULL);
(NULL, 'update_view_dbkey_lengths', 'restore.epp', NULL, 12, 357, NULL, 'fixing views dbkey length', NULL, NULL);
(NULL, 'update_ownership', 'restore.epp', NULL, 12, 358, NULL, 'updating ownership of packages, procedures and tables', NULL, NULL);
(NULL, 'fix_missing_privileges', 'restore.epp', NULL, 12, 359, NULL, 'adding missing privileges', NULL, NULL);
(NULL, 'RESTORE_restore', 'restore.epp', NULL, 12, 360, NULL, 'adjusting the ONLINE and FORCED WRITES flags', NULL, NULL);
-- SQLERR
(NULL, NULL, NULL, NULL, 13, 1, NULL, 'Firebird error', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 74, NULL, 'Rollback not performed', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 76, NULL, 'Connection error', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 77, NULL, 'Connection not established', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 78, NULL, 'Connection authorization failure.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 87, NULL, 'deadlock', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 88, NULL, 'Unsuccessful execution caused by deadlock.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 89, NULL, 'record from transaction @1 is stuck in limbo', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 91, NULL, 'operation completed with errors', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 94, NULL, 'the SQL statement cannot be executed', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 96, NULL, 'Unsuccessful execution caused by an unavailable resource.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 98, NULL, 'Unsuccessful execution caused by a system error that precludes successful execution of subsequent statements', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 99, NULL, 'Unsuccessful execution caused by system error that does not preclude successful execution of subsequent statements', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 158, NULL, 'Wrong numeric type', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 159, NULL, 'too many versions', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 160, NULL, 'intermediate journal file full', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 161, NULL, 'journal file wrong format', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 162, NULL, 'database @1 shutdown in @2 seconds', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 163, NULL, 'restart shared cache manager', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 164, NULL, 'exception @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 165, NULL, 'bad checksum', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 166, NULL, 'refresh range number @1 not found', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 167, NULL, 'expression evaluation not supported', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 168, NULL, 'FOREIGN KEY column count does not match PRIMARY KEY', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 169, NULL, 'Attempt to define a second PRIMARY KEY for the same table', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 170, NULL, 'column used with aggregate', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 171, NULL, 'invalid column reference', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 172, NULL, 'invalid key position', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 173, NULL, 'invalid direction for find operation', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 174, NULL, 'Invalid statement handle', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 175, NULL, 'invalid lock handle', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 176, NULL, 'invalid lock level @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 177, NULL, 'invalid bookmark handle', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 180, NULL, 'wrong or obsolete version', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 183, NULL, 'The INSERT, UPDATE, DELETE, DDL or authorization statement cannot be executed because the transaction is inquiry only', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 184, NULL, 'external file could not be opened for output', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 189, NULL, 'multiple rows in singleton select', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 190, NULL, 'No subqueries permitted for VIEW WITH CHECK OPTION', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 191, NULL, 'DISTINCT, GROUP or HAVING not permitted for VIEW WITH CHECK OPTION', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 192, NULL, 'Only one table allowed for VIEW WITH CHECK OPTION', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 193, NULL, 'No WHERE clause for VIEW WITH CHECK OPTION', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 194, NULL, 'Only simple column names permitted for VIEW WITH CHECK OPTION', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 196, NULL, 'An error was found in the application program input parameters for the SQL statement.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 197, NULL, 'Invalid insert or update value(s): object columns are constrained - no 2 table rows can have duplicate column values', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 198, NULL, 'Arithmetic overflow or division by zero has occurred.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 306, NULL, 'cannot access column @1 in view @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 307, NULL, 'Too many concurrent executions of the same request', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 308, NULL, 'maximum indexes per table (@1) exceeded', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 309, NULL, 'new record size of @1 bytes is too big', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 310, NULL, 'segments not allowed in expression index @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 311, NULL, 'wrong page type', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 315, NULL, 'invalid ARRAY or BLOB operation', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 323, NULL, '@1 extension error', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 336, NULL, 'key size exceeds implementation restriction for index "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 337, NULL, 'definition error for index @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 340, NULL, 'cannot create index', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 363, NULL, 'duplicate specification of @1 - not supported', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 375, NULL, 'The insert failed because a column definition includes validation constraints.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 382, NULL, 'Cannot delete object referenced by another object', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 383, NULL, 'Cannot modify object referenced by another object', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 384, NULL, 'Object is referenced by another object', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 385, NULL, 'lock on conflicts with existing lock', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 393, NULL, 'This operation is not defined for system tables.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 395, NULL, 'Inappropriate self-reference of column', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 396, NULL, 'Illegal array dimension range', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 399, NULL, 'database or file exists', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 400, NULL, 'sort error: corruption in data structure', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 401, NULL, 'node not supported', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 402, NULL, 'Shadow number must be a positive integer', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 403, NULL, 'Preceding file did not specify length, so @1 must include starting page number', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 404, NULL, 'illegal operation when at beginning of stream', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 405, NULL, 'the current position is on a crack', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 447, NULL, 'cannot modify an existing user privilege', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 448, NULL, 'user does not have the privilege to perform operation', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 449, NULL, 'This user does not have privilege to perform this operation on this object.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 468, NULL, 'transaction marked invalid by I/O error', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 469, NULL, 'Cannot prepare a CREATE DATABASE/SCHEMA statement', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 470, NULL, 'violation of FOREIGN KEY constraint "@1"', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 481, NULL, 'The prepare statement identifies a prepare statement with an open cursor', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 482, NULL, 'Unknown statement or request', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 490, NULL, 'Attempt to update non-updatable cursor', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 492, NULL, 'The cursor identified in the UPDATE or DELETE statement is not positioned on a row.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 496, NULL, 'Unknown cursor', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 498, NULL, 'The cursor identified in an OPEN statement is already open.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 499, NULL, 'The cursor identified in a FETCH or CLOSE statement is not open.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 587, NULL, 'Overflow occurred during data type conversion.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 593, NULL, 'null segment of UNIQUE KEY', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 594, NULL, 'subscript out of bounds', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 598, NULL, 'data operation not supported', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 599, NULL, 'invalid comparison operator for find operation', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 686, NULL, 'Cannot transliterate character between character sets', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 687, NULL, 'count of column list and variable list do not match', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 697, NULL, 'Incompatible column/host variable data type', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 703, NULL, 'Operation violates CHECK constraint @1 on view or table', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 704, NULL, 'internal Firebird consistency check (invalid RDB$CONSTRAINT_TYPE)', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 705, NULL, 'Cannot update constraints (RDB$RELATION_CONSTRAINTS).', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 706, NULL, 'Cannot delete CHECK constraint entry (RDB$CHECK_CONSTRAINTS)', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 707, NULL, 'Cannot update constraints (RDB$CHECK_CONSTRAINTS).', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 708, NULL, 'Cannot update constraints (RDB$REF_CONSTRAINTS).', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 709, NULL, 'Column used in a PRIMARY constraint must be NOT NULL.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 716, NULL, 'index @1 cannot be used in the specified plan', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 717, NULL, 'table @1 is referenced in the plan but not the from list', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 718, NULL, 'the table @1 is referenced twice; use aliases to differentiate', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 719, NULL, 'table @1 is not referenced in plan', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 739, NULL, 'Log file specification partition error', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 740, NULL, 'Cache or Log redefined', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 741, NULL, 'Write-ahead Log with shadowing configuration not allowed', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 742, NULL, 'Overflow log specification required for round-robin log', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 743, NULL, 'WAL defined; Cache Manager must be started first', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 745, NULL, 'Write-ahead Log without shared cache configuration not allowed', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 746, NULL, 'Cannot start WAL writer for the database @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 747, NULL, 'WAL writer synchronization error for the database @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 748, NULL, 'WAL setup error.  Please see Firebird log.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 749, NULL, 'WAL buffers cannot be increased.  Please see Firebird log.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 750, NULL, 'WAL writer - Journal server communication error.  Please see Firebird log.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 751, NULL, 'WAL I/O error.  Please see Firebird log.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 752, NULL, 'Unable to roll over; please see Firebird log.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 753, NULL, 'obsolete', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 754, NULL, 'obsolete', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 755, NULL, 'obsolete', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 756, NULL, 'obsolete', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 757, NULL, 'database does not use Write-ahead Log', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 758, NULL, 'Cannot roll over to the next log file @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 759, NULL, 'obsolete', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 760, NULL, 'obsolete', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 761, NULL, 'Cache or Log size too small', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 762, NULL, 'Log record header too small at offset @1 in log file @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 763, NULL, 'Incomplete log record at offset @1 in log file @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 764, NULL, 'Unexpected end of log file @1 at offset @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 765, NULL, 'Database name in the log file @1 is different', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 766, NULL, 'Log file @1 not closed properly; database recovery may be required', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 767, NULL, 'Log file @1 not latest in the chain but open flag still set', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 768, NULL, 'Invalid version of log file @1', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 769, NULL, 'Log file header of @1 too small', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 770, NULL, 'obsolete', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 781, NULL, 'table @1 is not defined', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 792, NULL, 'invalid ORDER BY clause', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 794, NULL, 'Column does not belong to referenced table', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 795, NULL, 'column @1 is not defined in table @2', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 796, NULL, 'Undefined name', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 797, NULL, 'Ambiguous column reference.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 828, NULL, 'function @1 is not defined', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 829, NULL, 'Invalid data type, length, or value', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 830, NULL, 'Invalid number of arguments', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 838, NULL, 'dbkey not available for multi-table views', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 842, NULL, 'number of columns does not match select list', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 843, NULL, 'must specify column name for view select expression', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 845, NULL, '@1 is not a valid base table of the specified view', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 849, NULL, 'This column cannot be updated because it is derived from an SQL function or expression.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 850, NULL, 'The object of the INSERT, DELETE or UPDATE statement is a view for which the requested operation is not permitted.', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 895, NULL, 'Invalid String', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 896, NULL, 'Invalid token', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 897, NULL, 'Invalid numeric literal', NULL, NULL);
(NULL, 'LICENTOOL_display_msg', NULL, NULL, 13, 915, NULL, 'An error occurred while trying to update the security database', NULL, NULL);
(NULL, NULL, NULL, NULL, 13, 916, NULL, 'non-SQL security class defined', NULL, NULL);
('dsql_too_old_ods', 'init', 'dsql.cpp', NULL, 13, 917, NULL, 'ODS versions before ODS@1 are not supported', NULL, NULL);
('dsql_table_not_found', 'delete_relation_view', 'ddl.cpp', NULL, 13, 918, NULL, 'Table @1 does not exist', NULL, NULL);
('dsql_view_not_found', 'define_view/delete_relation_view', 'ddl.cpp', NULL, 13, 919, NULL, 'View @1 does not exist', NULL, NULL);
('dsql_line_col_error', 'field_unknown', 'pass1.cpp', NULL, 13, 920, NULL, 'At line @1, column @2', NULL, NULL);
('dsql_unknown_pos', 'field_unknown', 'pass1.cpp', NULL, 13, 921, NULL, 'At unknown line and column', NULL, NULL);
('dsql_no_dup_name', 'field_repeated', 'pass1.cpp', NULL, 13, 922, NULL, 'Column @1 cannot be repeated in @2 statement', NULL, NULL);
('dsql_too_many_values', 'PASS1_node', 'pass1.cpp', NULL, 13, 923, NULL, 'Too many values (more than @1) in member list to match against', NULL, NULL);
('dsql_no_array_computed', 'is_array_or_blob', 'ddl.cpp', NULL, 13, 924, NULL, 'Array and BLOB data types not allowed in computed field', NULL, NULL);
('dsql_implicit_domain_name', 'define_domain', 'ddl.cpp', NULL, 13, 925, NULL, 'Implicit domain name @1 not allowed in user created domain', NULL, NULL);
('dsql_only_can_subscript_array', 'MAKE_field', 'make.cpp', NULL, 13, 926, NULL, 'scalar operator used on field @1 which is not an array', NULL, NULL);
('dsql_max_sort_items', 'pass1_sort', 'pass1.cpp', NULL, 13, 927, NULL, 'cannot sort on more than 255 items', NULL, NULL);
('dsql_max_group_items', 'pass1_group_by_list', 'pass1.cpp', NULL, 13, 928, NULL, 'cannot group on more than 255 items', NULL, NULL);
('dsql_conflicting_sort_field', 'pass1_sort', 'pass1.cpp', NULL, 13, 929, NULL, 'Cannot include the same field (@1.@2) twice in the ORDER BY clause with conflicting sorting options', NULL, NULL);
('dsql_derived_table_more_columns', 'pass1_derived_table', 'pass1.cpp', NULL, 13, 930, NULL, 'column list from derived table @1 has more columns than the number of items in its SELECT statement', NULL, NULL);
('dsql_derived_table_less_columns', 'pass1_derived_table', 'pass1.cpp', NULL, 13, 931, NULL, 'column list from derived table @1 has less columns than the number of items in its SELECT statement', NULL, NULL);
('dsql_derived_field_unnamed', 'pass1_derived_table', 'pass1.cpp', NULL, 13, 932, NULL, 'no column name specified for column number @1 in derived table @2', NULL, NULL);
('dsql_derived_field_dup_name', 'pass1_derived_table', 'pass1.cpp', NULL, 13, 933, NULL, 'column @1 was specified multiple times for derived table @2', NULL, NULL);
('dsql_derived_alias_select', 'pass1_expand_select_node', 'pass1.cpp', NULL, 13, 934, NULL, 'Internal dsql error: alias type expected by pass1_expand_select_node', NULL, NULL);
('dsql_derived_alias_field', 'pass1_field', 'pass1.cpp', NULL, 13, 935, NULL, 'Internal dsql error: alias type expected by pass1_field', NULL, NULL);
('dsql_auto_field_bad_pos', 'pass1_union_auto_cast', 'pass1.cpp', NULL, 13, 936, NULL, 'Internal dsql error: column position out of range in pass1_union_auto_cast', NULL, NULL);
('dsql_cte_wrong_reference', 'PASS1_node', 'dsql.cpp', NULL, 13, 937, NULL, 'Recursive CTE member (@1) can refer itself only in FROM clause', NULL, NULL);
('dsql_cte_cycle', 'PASS1_node', 'dsql.cpp', NULL, 13, 938, NULL, 'CTE ''@1'' has cyclic dependencies', NULL, NULL);
('dsql_cte_outer_join', 'pass1_join_is_recursive', 'dsql.cpp', NULL, 13, 939, NULL, 'Recursive member of CTE can''t be member of an outer join', NULL, NULL);
('dsql_cte_mult_references', 'pass1_join_is_recursive', 'dsql.cpp', NULL, 13, 940, NULL, 'Recursive member of CTE can''t reference itself more than once', NULL, NULL);
('dsql_cte_not_a_union', 'pass1_recursive_cte', 'dsql.cpp', NULL, 13, 941, NULL, 'Recursive CTE (@1) must be an UNION', NULL, NULL);
('dsql_cte_nonrecurs_after_recurs', 'pass1_recursive_cte', 'dsql.cpp', NULL, 13, 942, NULL, 'CTE ''@1'' defined non-recursive member after recursive', NULL, NULL);
('dsql_cte_wrong_clause', 'pass1_recursive_cte', 'dsql.cpp', NULL, 13, 943, NULL, 'Recursive member of CTE ''@1'' has @2 clause', NULL, NULL);
('dsql_cte_union_all', 'pass1_recursive_cte', 'dsql.cpp', NULL, 13, 944, NULL, 'Recursive members of CTE (@1) must be linked with another members via UNION ALL', NULL, NULL);
('dsql_cte_miss_nonrecursive', 'pass1_recursive_cte', 'dsql.cpp', NULL, 13, 945, NULL, 'Non-recursive member is missing in CTE ''@1''', NULL, NULL);
('dsql_cte_nested_with', 'dsql_req::addCTEs', 'dsql.cpp', NULL, 13, 946, NULL, 'WITH clause can''t be nested', NULL, NULL);
-- Do not change the arguments of the previous SQLERR messages.
-- Write the new SQLERR messages here.
('dsql_col_more_than_once_using', 'pass1_join', 'dsql.cpp', NULL, 13, 947, NULL, 'column @1 appears more than once in USING clause', NULL, NULL);
('dsql_unsupp_feature_dialect', NULL, NULL, NULL, 13, 948, NULL, 'feature is not supported in dialect @1', NULL, NULL);
('dsql_cte_not_used', 'dsql_req::checkUnusedCTEs', 'pass1.cpp', NULL, 13, 949, NULL, 'CTE "@1" is not used in query', NULL, NULL);
('dsql_col_more_than_once_view', 'define_view', 'ddl.cpp', NULL, 13, 950, NULL, 'column @1 appears more than once in ALTER VIEW', NULL, NULL);
('dsql_unsupported_in_auto_trans', 'PASS1_statement', 'pass1.cpp', NULL, 13, 951, NULL, '@1 is not supported inside IN AUTONOMOUS TRANSACTION block', NULL, NULL);
-- These add more information to the dumb isc_expression_eval_err
('dsql_eval_unknode', 'GEN_expr', 'dsql/gen.cpp', NULL, 13, 952, NULL, 'Unknown node type @1 in dsql/GEN_expr', NULL, NULL)
('dsql_agg_wrongarg', 'MAKE_desc', 'make.cpp', NULL, 13, 953, NULL, 'Argument for @1 in dialect 1 must be string or numeric', NULL, NULL)
('dsql_agg2_wrongarg', 'MAKE_desc', 'make.cpp', NULL, 13, 954, NULL, 'Argument for @1 in dialect 3 must be numeric', NULL, NULL)
('dsql_nodateortime_pm_string', 'MAKE_desc', 'make.cpp', NULL, 13, 955, NULL, 'Strings cannot be added to or subtracted from DATE or TIME types', NULL, NULL)
('dsql_invalid_datetime_subtract', 'MAKE_desc', 'make.cpp', NULL, 13, 956, NULL, 'Invalid data type for subtraction involving DATE, TIME or TIMESTAMP types', NULL, NULL)
('dsql_invalid_dateortime_add', 'MAKE_desc', 'make.cpp', NULL, 13, 957, NULL, 'Adding two DATE values or two TIME values is not allowed', NULL, NULL)
('dsql_invalid_type_minus_date', 'MAKE_desc', 'make.cpp', NULL, 13, 958, NULL, 'DATE value cannot be subtracted from the provided data type', NULL, NULL)
('dsql_nostring_addsub_dial3', 'MAKE_desc', 'make.cpp', NULL, 13, 959, NULL, 'Strings cannot be added or subtracted in dialect 3', NULL, NULL)
('dsql_invalid_type_addsub_dial3', 'MAKE_desc', 'make.cpp', NULL, 13, 960, NULL, 'Invalid data type for addition or subtraction in dialect 3', NULL, NULL)
('dsql_invalid_type_multip_dial1', 'MAKE_desc', 'make.cpp', NULL, 13, 961, NULL, 'Invalid data type for multiplication in dialect 1', NULL, NULL)
('dsql_nostring_multip_dial3', 'MAKE_desc', 'make.cpp', NULL, 13, 962, NULL, 'Strings cannot be multiplied in dialect 3', NULL, NULL)
('dsql_invalid_type_multip_dial3', 'MAKE_desc', 'make.cpp', NULL, 13, 963, NULL, 'Invalid data type for multiplication in dialect 3', NULL, NULL)
('dsql_mustuse_numeric_div_dial1', 'MAKE_desc', 'make.cpp', NULL, 13, 964, NULL, 'Division in dialect 1 must be between numeric data types', NULL, NULL)
('dsql_nostring_div_dial3', 'MAKE_desc', 'make.cpp', NULL, 13, 965, NULL, 'Strings cannot be divided in dialect 3', NULL, NULL)
('dsql_invalid_type_div_dial3', 'MAKE_desc', 'make.cpp', NULL, 13, 966, NULL, 'Invalid data type for division in dialect 3', NULL, NULL)
('dsql_nostring_neg_dial3',  'MAKE_desc', 'make.cpp', NULL, 13, 967, NULL, 'Strings cannot be negated (applied the minus operator) in dialect 3', NULL, NULL)
('dsql_invalid_type_neg', 'MAKE_desc', 'make.cpp', NULL, 13, 968, NULL, 'Invalid data type for negation (minus operator)', NULL, NULL)
-- End of extras for isc_expression_eval_err
('dsql_max_distinct_items', 'pass1_rse_impl', 'pass1.cpp', NULL, 13, 969, NULL, 'Cannot have more than 255 items in DISTINCT list', NULL, NULL);
('dsql_alter_charset_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 970, NULL, 'ALTER CHARACTER SET @1 failed', NULL, NULL);
('dsql_comment_on_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 971, NULL, 'COMMENT ON @1 failed', NULL, NULL);
('dsql_create_func_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 972, NULL, 'CREATE FUNCTION @1 failed', NULL, NULL);
('dsql_alter_func_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 973, NULL, 'ALTER FUNCTION @1 failed', NULL, NULL);
('dsql_create_alter_func_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 974, NULL, 'CREATE OR ALTER FUNCTION @1 failed', NULL, NULL);
('dsql_drop_func_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 975, NULL, 'DROP FUNCTION @1 failed', NULL, NULL);
('dsql_recreate_func_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 976, NULL, 'RECREATE FUNCTION @1 failed', NULL, NULL);
('dsql_create_proc_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 977, NULL, 'CREATE PROCEDURE @1 failed', NULL, NULL);
('dsql_alter_proc_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 978, NULL, 'ALTER PROCEDURE @1 failed', NULL, NULL);
('dsql_create_alter_proc_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 979, NULL, 'CREATE OR ALTER PROCEDURE @1 failed', NULL, NULL);
('dsql_drop_proc_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 980, NULL, 'DROP PROCEDURE @1 failed', NULL, NULL);
('dsql_recreate_proc_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 981, NULL, 'RECREATE PROCEDURE @1 failed', NULL, NULL);
('dsql_create_trigger_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 982, NULL, 'CREATE TRIGGER @1 failed', NULL, NULL);
('dsql_alter_trigger_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 983, NULL, 'ALTER TRIGGER @1 failed', NULL, NULL);
('dsql_create_alter_trigger_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 984, NULL, 'CREATE OR ALTER TRIGGER @1 failed', NULL, NULL);
('dsql_drop_trigger_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 985, NULL, 'DROP TRIGGER @1 failed', NULL, NULL);
('dsql_recreate_trigger_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 986, NULL, 'RECREATE TRIGGER @1 failed', NULL, NULL);
('dsql_create_collation_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 987, NULL, 'CREATE COLLATION @1 failed', NULL, NULL);
('dsql_drop_collation_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 988, NULL, 'DROP COLLATION @1 failed', NULL, NULL);
('dsql_create_domain_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 989, NULL, 'CREATE DOMAIN @1 failed', NULL, NULL);
('dsql_alter_domain_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 990, NULL, 'ALTER DOMAIN @1 failed', NULL, NULL);
('dsql_drop_domain_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 991, NULL, 'DROP DOMAIN @1 failed', NULL, NULL);
('dsql_create_except_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 992, NULL, 'CREATE EXCEPTION @1 failed', NULL, NULL);
('dsql_alter_except_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 993, NULL, 'ALTER EXCEPTION @1 failed', NULL, NULL);
('dsql_create_alter_except_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 994, NULL, 'CREATE OR ALTER EXCEPTION @1 failed', NULL, NULL);
('dsql_recreate_except_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 995, NULL, 'RECREATE EXCEPTION @1 failed', NULL, NULL);
('dsql_drop_except_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 996, NULL, 'DROP EXCEPTION @1 failed', NULL, NULL);
('dsql_create_sequence_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 997, NULL, 'CREATE SEQUENCE @1 failed', NULL, NULL);
('dsql_create_table_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 998, NULL, 'CREATE TABLE @1 failed', NULL, NULL);
('dsql_alter_table_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 999, NULL, 'ALTER TABLE @1 failed', NULL, NULL);
('dsql_drop_table_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1000, NULL, 'DROP TABLE @1 failed', NULL, NULL);
('dsql_recreate_table_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1001, NULL, 'RECREATE TABLE @1 failed', NULL, NULL);
('dsql_create_pack_failed', 'getMainErrorCode', 'PackageNodes.h', NULL, 13, 1002, NULL, 'CREATE PACKAGE @1 failed', NULL, NULL);
('dsql_alter_pack_failed', 'getMainErrorCode', 'PackageNodes.h', NULL, 13, 1003, NULL, 'ALTER PACKAGE @1 failed', NULL, NULL);
('dsql_create_alter_pack_failed', 'getMainErrorCode', 'PackageNodes.h', NULL, 13, 1004, NULL, 'CREATE OR ALTER PACKAGE @1 failed', NULL, NULL);
('dsql_drop_pack_failed', 'getMainErrorCode', 'PackageNodes.h', NULL, 13, 1005, NULL, 'DROP PACKAGE @1 failed', NULL, NULL);
('dsql_recreate_pack_failed', 'getMainErrorCode', 'PackageNodes.h', NULL, 13, 1006, NULL, 'RECREATE PACKAGE @1 failed', NULL, NULL);
('dsql_create_pack_body_failed', 'getMainErrorCode', 'PackageNodes.h', NULL, 13, 1007, NULL, 'CREATE PACKAGE BODY @1 failed', NULL, NULL);
('dsql_drop_pack_body_failed', 'getMainErrorCode', 'PackageNodes.h', NULL, 13, 1008, NULL, 'DROP PACKAGE BODY @1 failed', NULL, NULL);
('dsql_recreate_pack_body_failed', 'getMainErrorCode', 'PackageNodes.h', NULL, 13, 1009, NULL, 'RECREATE PACKAGE BODY @1 failed', NULL, NULL);
('dsql_create_view_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1010, NULL, 'CREATE VIEW @1 failed', NULL, NULL);
('dsql_alter_view_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1011, NULL, 'ALTER VIEW @1 failed', NULL, NULL);
('dsql_create_alter_view_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1012, NULL, 'CREATE OR ALTER VIEW @1 failed', NULL, NULL);
('dsql_recreate_view_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1013, NULL, 'RECREATE VIEW @1 failed', NULL, NULL);
('dsql_drop_view_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1014, NULL, 'DROP VIEW @1 failed', NULL, NULL);
('dsql_drop_sequence_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1015, NULL, 'DROP SEQUENCE @1 failed', NULL, NULL);
('dsql_recreate_sequence_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1016, NULL, 'RECREATE SEQUENCE @1 failed', NULL, NULL);
('dsql_drop_index_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1017, NULL, 'DROP INDEX @1 failed', NULL, NULL);
('dsql_drop_filter_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1018, NULL, 'DROP FILTER @1 failed', NULL, NULL);
('dsql_drop_shadow_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1019, NULL, 'DROP SHADOW @1 failed', NULL, NULL);
('dsql_drop_role_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1020, NULL, 'DROP ROLE @1 failed', NULL, NULL);
('dsql_drop_user_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1021, NULL, 'DROP USER @1 failed', NULL, NULL);
('dsql_create_role_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1022, NULL, 'CREATE ROLE @1 failed', NULL, NULL);
('dsql_alter_role_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1023, NULL, 'ALTER ROLE @1 failed', NULL, NULL);
('dsql_alter_index_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1024, NULL, 'ALTER INDEX @1 failed', NULL, NULL);
('dsql_alter_database_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1025, NULL, 'ALTER DATABASE failed', NULL, NULL);
('dsql_create_shadow_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1026, NULL, 'CREATE SHADOW @1 failed', NULL, NULL);
('dsql_create_filter_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1027, NULL, 'DECLARE FILTER @1 failed', NULL, NULL);
('dsql_create_index_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1028, NULL, 'CREATE INDEX @1 failed', NULL, NULL);
('dsql_create_user_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1029, NULL, 'CREATE USER @1 failed', NULL, NULL);
('dsql_alter_user_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1030, NULL, 'ALTER USER @1 failed', NULL, NULL);
('dsql_grant_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1031, NULL, 'GRANT failed', NULL, NULL);
('dsql_revoke_failed', 'getMainErrorCode', 'DdlNodes.h', NULL, 13, 1032, NULL, 'REVOKE failed', NULL, NULL);
('dsql_cte_recursive_aggregate', 'pass1_rse_impl', 'dsql.cpp', NULL, 13, 1033, NULL, 'Recursive member of CTE cannot use aggregate or window function', NULL, NULL);
('dsql_mapping_failed', 'MappingNode::putErrorPrefix', 'DdlNodes.h', NULL, 13, 1034, NULL, '@2 MAPPING @1 failed', NULL, NULL);
('dsql_alter_sequence_failed', 'putErrorPrefix', 'DdlNodes.h', NULL, 13, 1035, NULL, 'ALTER SEQUENCE @1 failed', NULL, NULL);
('dsql_create_generator_failed', 'putErrorPrefix', 'DdlNodes.h', NULL, 13, 1036, NULL, 'CREATE GENERATOR @1 failed', NULL, NULL);
('dsql_set_generator_failed', 'putErrorPrefix', 'DdlNodes.h', NULL, 13, 1037, NULL, 'SET GENERATOR @1 failed', NULL, NULL);
('dsql_wlock_simple', 'pass1_rse_impl', 'pass1.cpp', NULL, 13, 1038, NULL, 'WITH LOCK can be used only with a single physical table', NULL, NULL);
('dsql_firstskip_rows', 'pass1_rse_impl', 'pass1.cpp', NULL, 13, 1039, NULL, 'FIRST/SKIP cannot be used with ROWS', NULL, NULL);
('dsql_wlock_aggregates', 'pass1_rse_impl', 'pass1.cpp', NULL, 13, 1040, NULL, 'WITH LOCK cannot be used with aggregates', NULL, NULL);
('dsql_wlock_conflict', NULL, 'pass1.cpp', NULL, 13, 1041, NULL, 'WITH LOCK cannot be used with @1', NULL, NULL);
-- SQLWARN
(NULL, NULL, NULL, NULL, 14, 100, NULL, 'Row not found for fetch, update or delete, or the result of a query is an empty table.', NULL, NULL);
(NULL, NULL, NULL, NULL, 14, 101, NULL, 'segment buffer length shorter than expected', NULL, NULL);
(NULL, NULL, NULL, NULL, 14, 301, NULL, 'Datatype needs modification', NULL, NULL);
(NULL, NULL, NULL, NULL, 14, 612, NULL, 'Duplicate column or domain name found.', NULL, NULL);
-- Do not change the arguments of the previous SQLWARN messages.
-- Write the new SQLWARN messages here.
-- JRD_BUGCHK
(NULL, 'BLKCHK', 'jrd.h', NULL, 15, 147, NULL, 'invalid block type encountered', NULL, NULL);
(NULL, 'most', 'apollo.c', NULL, 15, 148, NULL, 'wrong packet type', NULL, 'Error message not currently in use.');
(NULL, 'find_file', 'apollo.c', NULL, 15, 149, NULL, 'cannot map page', NULL, NULL);
(NULL, 'ALL_alloc', 'all.c', NULL, 15, 150, NULL, 'request to allocate invalid block type', NULL, NULL);
(NULL, 'ALL_alloc', 'all.c', NULL, 15, 151, NULL, 'request to allocate block type larger than maximum size', NULL, NULL);
(NULL, 'ALL_alloc', 'all.c', NULL, 15, 152, NULL, 'memory pool free list is invalid', NULL, NULL);
(NULL, 'find_pool', 'all.c', NULL, 15, 153, NULL, 'invalid pool id encountered', NULL, NULL);
(NULL, 'release', 'all.c', NULL, 15, 154, NULL, 'attempt to release free block', NULL, NULL);
(NULL, 'release', 'all.c', NULL, 15, 155, NULL, 'attempt to release block overlapping following free block', NULL, NULL);
(NULL, 'release', 'all.c', NULL, 15, 156, NULL, 'attempt to release block overlapping prior free block', NULL, NULL);
(NULL, 'gen_sort', 'opt.c', NULL, 15, 157, NULL, 'cannot sort on a field that does not exist', NULL, NULL);
(NULL, 'seek_file', 'os2.c', NULL, 15, 158, NULL, 'database file not available', NULL, NULL);
(NULL, 'LCK_assert', 'lck.c', NULL, 15, 159, NULL, 'cannot assert logical lock', NULL, NULL);
(NULL, 'walk_acl', 'scl.e', NULL, 15, 160, NULL, 'wrong ACL version', NULL, NULL);
(NULL, 'PAG_init2', 'pag.c', NULL, 15, 161, NULL, 'shadow block not found', NULL, NULL);
(NULL, 'SDW_notify', 'sdw.c', NULL, 15, 162, NULL, 'shadow lock not synchronized properly', NULL, NULL);
(NULL, 'SDW_start', 'sdw.c', NULL, 15, 163, NULL, 'root file name not listed for shadow', NULL, NULL);
(NULL, 'SYM_remove', 'sym.c', NULL, 15, 164, NULL, 'failed to remove symbol from hash table', NULL, NULL);
(NULL, 'inventory_page', 'tra.c', NULL, 15, 165, NULL, 'cannot find tip page', NULL, NULL);
(NULL, 'many', 'rse.c', NULL, 15, 166, NULL, 'invalid rsb type', NULL, NULL);
(NULL, 'EXE_send', 'exe.c', NULL, 15, 167, NULL, 'invalid SEND request', NULL, NULL);
(NULL, 'looper', 'exe.c', NULL, 15, 168, NULL, 'looper: action not yet implemented', NULL, NULL);
(NULL, 'FUN_evaluate', 'fun.e', NULL, 15, 169, NULL, 'return data type not supported', NULL, NULL);
COMMIT WORK;
(NULL, 'do_connect', 'jrn.c', NULL, 15, 170, NULL, 'unexpected reply from journal server', NULL, NULL);
(NULL, 'do_connect', 'jrn.c', NULL, 15, 171, NULL, 'journal server is incompatible version', NULL, NULL);
(NULL, 'do_connect', 'jrn.c', NULL, 15, 172, NULL, 'journal server refused connection', NULL, NULL);
(NULL, 'IDX_check_access', 'idx.c', NULL, 15, 173, NULL, 'referenced index description not found', NULL, NULL);
(NULL, 'IDX_create_index', 'idx.c', NULL, 15, 174, NULL, 'index key too big', NULL, NULL);
(NULL, 'check_partner_index', 'idx.c', NULL, 15, 175, NULL, 'partner index description not found', NULL, NULL);
(NULL, 'SQZ_apply_differences', 'sqz.c', NULL, 15, 176, NULL, 'bad difference record', NULL, NULL);
(NULL, 'SQZ_apply_differences', 'sqz.c', NULL, 15, 177, NULL, 'applied differences will not fit in record', NULL, NULL);
(NULL, 'SQZ_compress', 'sqz.c', NULL, 15, 178, NULL, 'record length inconsistent', NULL, NULL);
(NULL, 'SQZ_decompress', 'sqz.c', NULL, 15, 179, NULL, 'decompression overran buffer', NULL, NULL);
(NULL, 'EXT_erase', 'extvms.c', NULL, 15, 180, NULL, 'cannot reposition for update after sort for RMS', NULL, NULL);
(NULL, 'EXT_get', 'extvms.c', NULL, 15, 181, NULL, 'external access type not implemented', NULL, NULL);
(NULL, 'VIO_data', 'vio.c', NULL, 15, 182, NULL, 'differences record too long', NULL, NULL);
(NULL, 'VIO_data', 'vio.c', NULL, 15, 183, NULL, 'wrong record length', NULL, NULL);
(NULL, 'VIO_get_current', 'vio.c', NULL, 15, 184, NULL, 'limbo impossible', NULL, NULL);
(NULL, 'VIO_verb_cleanup', 'vio.c', NULL, 15, 185, NULL, 'wrong record version', NULL, NULL);
(NULL, 'prepare_update', 'vio.c', NULL, 15, 186, NULL, 'record disappeared', NULL, NULL);
(NULL, 'VIO_erase', 'vio.c', NULL, 15, 187, NULL, 'cannot delete system tables', NULL, NULL);
(NULL, 'prepare update', 'vio.c', NULL, 15, 188, NULL, 'cannot update erased record', NULL, NULL);
(NULL, 'MOV_compare', 'mov.c', NULL, 15, 189, NULL, 'comparison not supported for specified data types', NULL, NULL);
(NULL, NULL, 'MOV_get_double', NULL, 15, 190, NULL, 'conversion not supported for specified data types', NULL, NULL);
(NULL, 'many', 'mov.c', NULL, 15, 191, NULL, 'conversion error', NULL, NULL);
(NULL, 'integer_to_text', 'mov.c', NULL, 15, 192, NULL, 'overflow during conversion', NULL, NULL);
(NULL, 'blb::get_array', 'blb.c', NULL, 15, 193, NULL, 'null or invalid array', NULL, NULL);
(NULL, 'BLB_open2', 'blb.c', NULL, 15, 194, NULL, 'BLOB not found', NULL, NULL);
(NULL, 'BLB_put_segment', 'blb.c', NULL, 15, 195, NULL, 'cannot update old BLOB', NULL, NULL);
(NULL, 'blb::put_slice', 'blb.c', NULL, 15, 196, NULL, 'relation for array not known', NULL, NULL);
(NULL, 'blb::put_slice', 'blb.c', NULL, 15, 197, NULL, 'field for array not known', NULL, NULL);
(NULL, 'slice_callback', 'blb.c', NULL, 15, 198, NULL, 'array subscript computation error', NULL, NULL);
(NULL, 'blb::move', 'blb.cpp', NULL, 15, 199, NULL, 'expected field node', NULL, NULL);
(NULL, 'delete_blob_id', 'blb.c', NULL, 15, 200, NULL, 'invalid BLOB ID', NULL, NULL);
(NULL, 'get_next_page', 'blb.c', NULL, 15, 201, NULL, 'cannot find BLOB page', NULL, NULL);
(NULL, 'MET_descriptor', 'met.e', NULL, 15, 202, NULL, 'unknown data type', NULL, NULL);
(NULL, 'add_shadow', 'met.e', NULL, 15, 203, NULL, 'shadow block not found for extend file', NULL, NULL);
(NULL, 'BTR_insert', 'btr.c', NULL, 15, 204, NULL, 'index inconsistent', NULL, NULL);
(NULL, 'fast_load', 'btr.c', NULL, 15, 205, NULL, 'index bucket overfilled', NULL, NULL);
(NULL, 'find_index', 'btr.c', NULL, 15, 206, NULL, 'exceeded index level', NULL, NULL);
(NULL, 'CCH_fake', 'cch.c', NULL, 15, 207, NULL, 'page already in use', NULL, NULL);
(NULL, 'CCH_mark', 'cch.c', NULL, 15, 208, NULL, 'page not accessed for write', NULL, NULL);
(NULL, 'CCH_release', 'cch.c', NULL, 15, 209, NULL, 'attempt to release page not acquired', NULL, NULL);
(NULL, 'CCH_flush', 'cch.c', NULL, 15, 210, NULL, 'page in use during flush', NULL, NULL);
(NULL, 'btc_remove', 'cch.c', NULL, 15, 211, NULL, 'attempt to remove page from dirty page list when not there', NULL, NULL);
(NULL, 'check_precedence', 'cch.c', NULL, 15, 212, NULL, 'CCH_precedence: block marked', NULL, NULL);
(NULL, 'get_buffer', 'cch.c', NULL, 15, 213, NULL, 'insufficient cache size', NULL, NULL);
(NULL, 'get_buffer', 'cch.c', NULL, 15, 214, NULL, 'no cache buffers available for reuse', NULL, NULL);
(NULL, 'lock_buffer', 'cch.c', NULL, 15, 215, NULL, 'page @1, page type @2 lock conversion denied', NULL, NULL);
(NULL, 'lock_buffer', 'cch.c', NULL, 15, 216, NULL, 'page @1, page type @2 lock denied', NULL, NULL);
(NULL, 'write_buffer', 'cch.c', NULL, 15, 217, NULL, 'buffer marked for update', NULL, NULL);
(NULL, 'mutex_bugcheck', 'cch.c', NULL, 15, 218, NULL, 'CCH: @1, status = @2 (218)', NULL, NULL);
(NULL, 'CMP_make_request', 'cmp.c', NULL, 15, 219, NULL, 'request of unknown resource', NULL, NULL);
(NULL, 'CMP_release', 'cmp.c', NULL, 15, 220, NULL, 'release of unknown resource', NULL, NULL);
(NULL, 'copy', 'cmp.c', NULL, 15, 221, NULL, '(CMP) copy: cannot remap', NULL, NULL);
(NULL, 'CMP_format', 'cmp.c', NULL, 15, 222, NULL, 'bad BLR -- invalid stream', NULL, NULL);
(NULL, 'CMP_get_desc', 'cmp.c', NULL, 15, 223, NULL, 'argument of scalar operation must be an array', NULL, NULL);
(NULL, 'CMP_get_desc', 'cmp.c', NULL, 15, 224, NULL, 'quad word arithmetic not supported', NULL, NULL);
(NULL, 'CMP_get_desc', 'cmp.c', NULL, 15, 225, NULL, 'data type not supported for arithmetic', NULL, NULL);
(NULL, 'CMP_make_request', 'cmp.c', NULL, 15, 226, NULL, 'request size limit exceeded', NULL, NULL);
(NULL, 'pass1', 'cmp.c', NULL, 15, 227, NULL, 'cannot access field @1 in view @2', NULL, NULL);
(NULL, 'pass1', 'cmp.c', NULL, 15, 228, NULL, 'cannot access field in view @1', NULL, NULL);
(NULL, 'EVL_assign_to', 'evl.c', NULL, 15, 229, NULL, 'EVL_assign_to: invalid operation', NULL, NULL);
(NULL, 'EVL_bitmap', 'evl.c', NULL, 15, 230, NULL, 'EVL_bitmap: invalid operation', NULL, NULL);
(NULL, 'EVL_boolean', 'evl.c', NULL, 15, 231, NULL, 'EVL_boolean: invalid operation', NULL, NULL);
(NULL, 'EVL_expr', 'evl.c', NULL, 15, 232, NULL, 'EVL_expr: invalid operation', NULL, NULL);
(NULL, 'eval_statistical', 'evl.v', NULL, 15, 233, NULL, 'eval_statistical: invalid operation', NULL, NULL);
(NULL, 'MAP_status_to_gds', 'map.c', NULL, 15, 234, NULL, 'Unimplemented conversion, FAO directive O,Z,S', NULL, NULL);
(NULL, 'MAP_status_to_gds', 'map.c', NULL, 15, 235, NULL, 'Unimplemented conversion, FAO directive X,U', NULL, NULL);
(NULL, 'MAP_status_to_gds', 'map.c', NULL, 15, 236, NULL, 'Error parsing RDB FAO msg string', NULL, NULL);
(NULL, 'MAP_status_to_gds', 'map.c', NULL, 15, 237, NULL, 'Error parsing RDB FAO msg str', NULL, NULL);
(NULL, 'MAP_status_to_gds', 'map.c', NULL, 15, 238, NULL, 'unknown parameter in RdB status vector', NULL, NULL);
(NULL, 'translate_status', 'map.c', NULL, 15, 239, NULL, 'Firebird status vector inconsistent', NULL, NULL);
(NULL, 'translate_status', 'map.c', NULL, 15, 240, NULL, 'Firebird/RdB message parameter inconsistency', NULL, NULL);
(NULL, 'translate_status', 'map.c', NULL, 15, 241, NULL, 'error parsing RDB FAO message string', NULL, NULL);
(NULL, 'translate_status', 'map.c', NULL, 15, 242, NULL, 'unimplemented FAO directive', NULL, NULL);
(NULL, 'DPM_data_pages', 'dpm.e', NULL, 15, 243, NULL, 'missing pointer page in DPM_data_pages', NULL, NULL);
(NULL, 'DPM_delete', 'dmp.e', NULL, 15, 244, NULL, 'Fragment does not exist', NULL, NULL);
(NULL, 'DPM_delete', 'dpm.e', NULL, 15, 245, NULL, 'pointer page disappeared in DPM_delete', NULL, NULL);
(NULL, 'DPM_delete_relation', 'dpm.e', NULL, 15, 246, NULL, 'pointer page lost from DPM_delete_relation', NULL, NULL);
(NULL, 'DPM_dump', 'dpm.e', NULL, 15, 247, NULL, 'missing pointer page in DPM_dump', NULL, NULL);
(NULL, 'DPM_fetch', 'dmp.e', NULL, 15, 248, NULL, 'cannot find record fragment', NULL, NULL);
(NULL, 'DPM_next', 'dpm.e', NULL, 15, 249, NULL, 'pointer page vanished from DPM_next', NULL, NULL);
(NULL, 'compress', 'dpm.e', NULL, 15, 250, NULL, 'temporary page buffer too small', NULL, NULL);
(NULL, 'compress', 'dpm.e', NULL, 15, 251, NULL, 'damaged data page', NULL, NULL);
(NULL, 'fragment', 'dpm.e', NULL, 15, 252, NULL, 'header fragment length changed', NULL, NULL);
(NULL, 'extend_relation', 'dpm.e', NULL, 15, 253, NULL, 'pointer page vanished from extend_relation', NULL, NULL);
(NULL, 'locate_space', 'dpm.e', NULL, 15, 254, NULL, 'pointer page vanished from relation list in locate_space', NULL, NULL);
(NULL, 'locate_space', 'dpm.e', NULL, 15, 255, NULL, 'cannot find free space', NULL, NULL);
(NULL, 'mark_ful', 'dpm.e', NULL, 15, 256, NULL, 'pointer page vanished from mark_full', NULL, NULL);
(NULL, 'DPM_scan_pages', 'dpm.e', NULL, 15, 257, NULL, 'bad record in RDB$PAGES', NULL, NULL);
(NULL, 'extend_relation', 'dpm.e', NULL, 15, 258, NULL, 'page slot not empty', NULL, NULL);
(NULL, 'get_pointer_page', 'dpm.e', NULL, 15, 259, NULL, 'bad pointer page', NULL, NULL);
(NULL, 'BTR_find_page', 'btr.c', NULL, 15, 260, NULL, 'index unexpectedly deleted', NULL, NULL);
(NULL, 'scalar', 'evl.c', NULL, 15, 261, NULL, 'scalar operator used on field which is not an array', NULL, NULL);
(NULL, 'TRA_reconnect', 'TRA', NULL, 15, 262, NULL, 'active', NULL, NULL);
(NULL, 'TRA_reconnect', 'TRA', NULL, 15, 263, NULL, 'committed', NULL, NULL);
(NULL, 'TRA_reconnect', 'TRA', NULL, 15, 264, NULL, 'rolled back', NULL, NULL);
(NULL, 'TRA_reconnect', 'TRA', NULL, 15, 265, NULL, 'in an ill-defined state', NULL, NULL);
(NULL, 'PAG_header', 'PAG.C', NULL, 15, 266, NULL, 'next transaction older than oldest active transaction', NULL, NULL);
(NULL, 'PAG_header', 'PAG.C', NULL, 15, 267, NULL, 'next transaction older than oldest transaction', NULL, NULL);
(NULL, 'CCH_unwind', 'cch.c', NULL, 15, 268, NULL, 'buffer marked during cache unwind', NULL, NULL);
(NULL, 'REC_recover', 'rec.c', NULL, 15, 269, NULL, 'error in recovery! database corrupted', NULL, NULL);
(NULL, 'REC_recover', 'rec.c', NULL, 15, 270, NULL, 'error in recovery! wrong data page record', NULL, NULL);
(NULL, 'REC_recover', 'rec.c', NULL, 15, 271, NULL, 'error in recovery! no space on data page', NULL, NULL);
(NULL, 'REC_recover', 'rec.c', NULL, 15, 272, NULL, 'error in recovery! wrong header page record', NULL, NULL);
(NULL, 'REC_recover', 'rec.c', NULL, 15, 273, NULL, 'error in recovery! wrong generator page record', NULL, NULL);
(NULL, 'REC_recover', 'rec.c', NULL, 15, 274, NULL, 'error in recovery! wrong b-tree page record', NULL, NULL);
(NULL, 'REC_recover', 'rec.c', NULL, 15, 275, NULL, 'error in recovery! wrong page inventory page record', NULL, NULL);
(NULL, 'apply_pointer', 'rec.c', NULL, 15, 276, NULL, 'error in recovery! wrong pointer page record', NULL, NULL);
(NULL, 'apply_root', 'rec.c', NULL, 15, 277, NULL, 'error in recovery! wrong index root page record', NULL, NULL);
(NULL, 'apply_transaction', 'rec.c', NULL, 15, 278, NULL, 'error in recovery! wrong transaction page record', NULL, NULL);
(NULL, 'process_page', 'rec.c', NULL, 15, 279, NULL, 'error in recovery! out of sequence log record encountered', NULL, NULL);
(NULL, 'process_page', 'rec.c', NULL, 15, 280, NULL, 'error in recovery! unknown page type', NULL, NULL);
(NULL, 'rec_process_record', 'rec.c', NULL, 15, 281, NULL, 'error in recovery! unknown record type', NULL, NULL);
(NULL, 'do_connect', 'jrn.c', NULL, 15, 282, NULL, 'journal server cannot archive to specified archive directory', NULL, NULL);
(NULL, 'REC_restore', 'rec.c', NULL, 15, 283, NULL, 'checksum error in log record when reading from log file', NULL, NULL);
(NULL, 'pop_rpbs', 'rse.c', NULL, 15, 284, NULL, 'cannot restore singleton select data', NULL, NULL);
(NULL, 'internal_dequeue()', 'lck.c', NULL, 15, 285, NULL, 'lock not found in internal lock manager', NULL, NULL);
(NULL, 'OPT_compile', 'opt.c', NULL, 15, 286, NULL, 'size of opt block exceeded', NULL, NULL);
(NULL, 'TRA_rollback', 'TRA', NULL, 15, 287, NULL, 'Too many savepoints', NULL, NULL);
(NULL, 'replace_gc_record', 'vio.c', NULL, 15, 288, NULL, 'garbage collect record disappeared', NULL, NULL);
(NULL, 'filter_transliterate_text()', 'filters.c', NULL, 15, 289, NULL, 'Unknown BLOB FILTER ACTION_', NULL, NULL);
('savepoint_error', NULL, 'exe.c', NULL, 15, 290, NULL, 'error during savepoint backout', NULL, NULL);
(NULL, NULL, 'vio.c', NULL, 15, 291, NULL, 'cannot find record back version', NULL, NULL);
(NULL, 'grant_user', 'grant.e', NULL, 15, 292, NULL, 'Illegal user_type.', NULL, NULL);
(NULL, 'squeeze_acl', 'grant.e', NULL, 15, 293, NULL, 'bad ACL', NULL, NULL);
(NULL, 'release_bdb', 'cch.c', NULL, 15, 294, NULL, 'inconsistent LATCH_mark release', NULL, NULL);
(NULL, 'latch_bdb', 'cch.c', NULL, 15, 295, NULL, 'inconsistent LATCH_mark call', NULL, NULL);
(NULL, 'release_bdb', 'cch.c', NULL, 15, 296, NULL, 'inconsistent latch downgrade call', NULL, NULL);
(NULL, 'release_bdb', 'cch.c', NULL, 15, 297, NULL, 'bdb is unexpectedly marked', NULL, NULL);
(NULL, 'release_bdb', 'cch.c', NULL, 15, 298, NULL, 'missing exclusive latch', NULL, NULL);
(NULL, 'latch_bdb', 'cch.c', NULL, 15, 299, NULL, 'exceeded maximum number of shared latches on a bdb', NULL, NULL);
(NULL, 'release_bdb', 'cch.c', NULL, 15, 300, NULL, 'can''t find shared latch', NULL, NULL);
('cache_non_zero_use_count', 'get_buffer', 'cch.c', NULL, 15, 301, NULL, 'Non-zero use_count of a buffer in the empty que', NULL, NULL);
('unexpected_page_change', 'CCH_flush', 'cch.c', NULL, 15, 302, NULL, 'Unexpected page change from latching', NULL, NULL);
(NULL, 'EVL_expr', 'jrd/evl.c', NULL, 15, 303, NULL, 'Invalid expression for evaluation', NULL, NULL);
('rdb$triggers_rdb$flags_corrupt', 'MET_load_trigger', 'met.e', NULL, 15, 304, NULL, 'RDB$FLAGS for trigger @1 in RDB$TRIGGERS is corrupted', NULL, NULL);
(NULL, NULL, NULL, NULL, 15, 305, NULL, 'Blobs accounting is inconsistent', NULL, NULL);
(NULL, 'CMP_get_desc', 'cmp.cpp', NULL, 15, 306, NULL, 'Found array data type with more than 16 dimensions', NULL, NULL);
-- Do not change the arguments of the previous JRD_BUGCHK messages.
-- Write the new JRD_BUGCHK messages here.
-- ISQL
('GEN_ERR', 'errmsg', 'isql.e', NULL, 17, 0, NULL, 'Statement failed, SQLSTATE = @1', NULL, NULL);
('USAGE', 'ISQL_main', 'isql.epp', NULL, 17, 1, NULL, 'usage:    isql [options] [<database>]', NULL, NULL);
('SWITCH', 'parse_arg', 'isql.e', NULL, 17, 2, NULL, 'Unknown switch: @1', NULL, NULL);
('NO_DB', 'do_isql', 'isql.e', NULL, 17, 3, NULL, 'Use CONNECT or CREATE DATABASE to specify a database', NULL, NULL);
('FILE_OPEN_ERR', 'newoutput', 'isql.e', NULL, 17, 4, NULL, 'Unable to open @1', NULL, NULL);
('COMMIT_PROMPT', 'newtrans', 'isql.e', NULL, 17, 5, NULL, 'Commit current transaction (y/n)?', NULL, NULL);
('COMMIT_MSG', 'newtrans', 'isql.e', NULL, 17, 6, NULL, 'Committing.', NULL, NULL);
('ROLLBACK_MSG', 'newtrans', 'isql.e', NULL, 17, 7, NULL, 'Rolling back work.', NULL, NULL);
('CMD_ERR', 'frontend', 'isql.e', NULL, 17, 8, NULL, 'Command error: @1', NULL, NULL);
('ADD_PROMPT', 'add_row', 'isql.e', NULL, 17, 9, NULL, 'Enter data or NULL for each column.  RETURN to end.', NULL, NULL);
('VERSION', 'parse_arg', 'isql.e', NULL, 17, 10, NULL, 'ISQL Version: @1', NULL, NULL);
('USAGE_ALL', 'ISQL_main', 'isql.epp', NULL, 17, 11, NULL, '	-a(ll)                  extract metadata incl. legacy non-SQL tables', NULL, NULL);
('NUMBER_PAGES', 'SHOW_dbb_parameters', 'show.e', NULL, 17, 12, NULL, 'Number of DB pages allocated = @1', NULL, NULL);
('SWEEP_INTERV', 'SHOW_dbb_parameters', 'show.e', NULL, 17, 13, NULL, 'Sweep interval = @1', NULL, NULL);
('NUM_WAL_BUFF', 'SHOW_dbb_parameters', 'show.e', NULL, 17, 14, NULL, 'Number of wal buffers = @1', NULL, NULL);
('WAL_BUFF_SIZE', 'SHOW_dbb_parameters', 'show.e', NULL, 17, 15, NULL, 'Wal buffer size = @1', NULL, NULL);
('CKPT_LENGTH', 'SHOW_dbb_parameters', 'show.e', NULL, 17, 16, NULL, 'Check point length = @1', NULL, NULL);
('CKPT_INTERV', 'SHOW_dbb_parameters', 'show.e', NULL, 17, 17, NULL, 'Check point interval = @1', NULL, NULL);
('WAL_GRPC_WAIT', 'SHOW_dbb_parameters', 'show.e', NULL, 17, 18, NULL, 'Wal group commit wait = @1', NULL, NULL);
('BASE_LEVEL', 'SHOW_dbb_parameters', 'show.e', NULL, 17, 19, NULL, 'Base level = @1', NULL, NULL);
('LIMBO', 'SHOW_dbb_parameters', 'show.e', NULL, 17, 20, NULL, 'Transaction in limbo = @1', NULL, NULL);
('HLP_FRONTEND', 'help', 'isql.e', 'This message is followed by one newline (''\n'').', 17, 21, NULL, 'Frontend commands:', NULL, NULL);
('HLP_BLOBED', 'help', 'isql.e', 'Do not translate the word "BLOBEDIT"
This message is followed by one newline (''\n'').', 17, 22, NULL, 'BLOBVIEW <blobid>          -- view BLOB in text editor', NULL, NULL);
('HLP_BLOBDMP', 'help', 'isql.e', 'Do not translate the word "BLOBDUMP"
This message is followed by one newline (''\n'').', 17, 23, NULL, 'BLOBDUMP <blobid> <file>   -- dump BLOB to a file', NULL, NULL);
('HLP_EDIT', 'help', 'isql.e', 'Do not translate the word "EDIT".
The  first line is followed by one newline (''\n''), and
the second line begins with a TAB (''\t'').', 17, 24, NULL, 'EDIT     [<filename>]      -- edit SQL script file and execute', NULL, NULL);
('HLP_INPUT', 'help', 'isql.e', 'Do not translate the word "INput"
This message is followed by one newline (''\n'').', 17, 25, NULL, 'INput    <filename>        -- take input from the named SQL file', NULL, NULL);
('HLP_OUTPUT', 'help', 'isql.e', 'Do not translate the word "OUTput".
The  first line is followed by one newline (''\n''), and
the second line begins with a TAB (''\t'').', 17, 26, NULL, 'OUTput   [<filename>]      -- write output to named file', NULL, NULL);
('HLP_SHELL', 'help', 'isql.e', 'Do not translate the word "SHELL".', 17, 27, NULL, 'SHELL    <command>         -- execute Operating System command in sub-shell', NULL, NULL);
('HLP_HELP', 'help', 'isql.e', 'Do not translate the word "HELP".', 17, 28, NULL, 'HELP                       -- display this menu', NULL, NULL);
('HLP_SETCOM', 'help', 'isql.e', 'This message is followed by one newline (''\n'').', 17, 29, NULL, 'Set commands:', NULL, NULL);
('HLP_SET', 'help', 'isql.e', 'Do not translate the word "SET".
This message begins with a TAB (''\t'') and ends with a newline (''\n'').', 17, 30, NULL, '    SET                    -- display current SET options', NULL, NULL);
('HLP_SETAUTO', 'help', 'isql.e', 'Do not translate the words "SET AUTOcommit".
This message begins with a TAB (''\t'') and ends with a newline (''\n'').', 17, 31, NULL, '    SET AUTOddl            -- toggle autocommit of DDL statements', NULL, NULL);
('HLP_SETBLOB', 'help', 'isql.e', 'Do not translate the words "SET BLOBdisplay [ALL|N]" or "N".
The  first line is followed by one newline (''\n''), and
the second line begins with a TAB (''\t'').', 17, 32, NULL, '    SET BLOB [ALL|<n>]     -- display BLOBS of subtype <n> or ALL', NULL, NULL);
('HLP_SETCOUNT', 'help', 'isql.e', 'Do not translate the words "SET COUNT".
This message begins with a TAB (''\t'') and ends with a newline (''\n'').', 17, 33, NULL, '    SET COUNT              -- toggle count of selected rows on/off', NULL, NULL);
('HLP_SETECHO', 'help', 'isql.e', 'Do not translate the words "SET ECHO".
This message begins with a TAB (''\t'') and ends with a newline (''\n'').', 17, 34, NULL, '    SET ECHO               -- toggle command echo on/off', NULL, NULL);
('HLP_SETSTAT', 'help', 'isql.e', 'Do not translate the words "SET STATs".
This message begins with a TAB (''\t'') and ends with a newline (''\n'').', 17, 35, NULL, '    SET STATs              -- toggle display of performance statistics', NULL, NULL);
('HLP_SETTERM', 'help', 'isql.e', 'Do not translate the words "SET TERM".
This message begins with a TAB (''\t'') and ends with a newline (''\n'').', 17, 36, NULL, '    SET TERM <string>      -- change statement terminator string', NULL, NULL);
('HLP_SHOW', 'help', 'isql.e', 'Do not translate the word "SHOW".
This message is followed by one newline (''\n'').', 17, 37, NULL, 'SHOW     <object> [<name>] -- display system information', NULL, NULL);
('HLP_OBJTYPE', 'help', 'isql.e', 'This message goes together with No. 37.  Translate only "object type".
The message begins with 2 blank spaces.
There is a newline (''\n'') after "INDEX" and another at the end.', 17, 38, NULL, '    <object> = CHECK, COLLATION, DATABASE, DOMAIN, EXCEPTION, FILTER, FUNCTION,', NULL, NULL);
('HLP_EXIT', 'help', 'isql.e', 'Do not translate the word "EXIT".
This message is followed by one newline (''\n'').', 17, 39, NULL, 'EXIT                       -- exit and commit changes', NULL, NULL);
('HLP_QUIT', 'help', 'isql.e', 'Do not translate the word "QUIT".
This message is followed by 2 newlines (''\n'').', 17, 40, NULL, 'QUIT                       -- exit and roll back changes', NULL, NULL);
('HLP_ALL', 'help', 'isql.e', 'This message is followed by one newline (''\n'').', 17, 41, NULL, 'All commands may be abbreviated to letters in CAPitals', NULL, NULL);
('HLP_SETSCHEMA', 'help', 'isql.e', 'Do not translate the words "SET SCHema/DB"
This message begins with a TAB (''\t'') and ends with a newline (''\n'').', 17, 42, NULL, '	SET SCHema/DB <db name> -- changes current database', NULL, NULL);
('YES_ANS', 'end_trans', 'isql.e', NULL, 17, 43, NULL, 'Yes', NULL, NULL);
('REPORT1', 'process_statement', 'isql.e', 'Each of these 4 items is followed by a newline (''\n'').', 17, 44, NULL, 'Current memory = !c
Delta memory = !d
Max memory = !x
Elapsed time= !e sec
', NULL, NULL);
('REPORT2', 'process_statement', 'isql.e', 'Each of these 5 items is followed by a newline (''\n'').', 17, 45, NULL, 'Cpu = !u sec
Buffers = !b
Reads = !r
Writes = !w
Fetches = !f', NULL, NULL);
('BLOB_SUBTYPE', 'ISQL_print_blob', 'isql.e', 'This message is followed by  one newline (''\n'').', 17, 46, NULL, 'BLOB display set to subtype @1. This BLOB: subtype = @2', NULL, NULL);
('BLOB_PROMPT', 'add_row', 'isql.e', 'Do not translate the word "edit".', 17, 47, NULL, 'BLOB: @1, type ''edit'' or filename to load>', NULL, NULL);
('DATE_PROMPT', 'add_row', 'isql.e', NULL, 17, 48, NULL, 'Enter @1 as Y/M/D>', NULL, NULL);
('NAME_PROMPT', 'add_row', 'isql.e', NULL, 17, 49, NULL, 'Enter @1>', NULL, NULL);
('DATE_ERR', 'add_row', 'isql.e', 'This message is followed by  one newline (''\n'').', 17, 50, NULL, 'Bad date @1', NULL, NULL);
('CON_PROMPT', 'get_statement', 'isql.e', NULL, 17, 51, NULL, 'CON> ', NULL, NULL);
('HLP_SETLIST', 'help', 'isql.e', NULL, 17, 52, NULL, '    SET LIST               -- toggle column or table display format', NULL, NULL);
('NOT_FOUND', 'copy_table EXTRACT_ddl', 'isql.e', NULL, 17, 53, NULL, '@1 not found', NULL, NULL);
('COPY_ERR', 'copy_table', 'isql.e', NULL, 17, 54, NULL, 'Errors occurred (possibly duplicate domains) in creating @1 in @2', NULL, NULL);
('SERVER_TOO_OLD', 'drop_db', 'isql.e', NULL, 17, 55, NULL, 'Server version too old to support the isql command', NULL, NULL);
('REC_COUNT', 'process_statement', 'isql.e', NULL, 17, 56, NULL, 'Records affected: @1', NULL, NULL);
('UNLICENSED', 'do_isql', 'isql.e', NULL, 17, 57, NULL, 'Unlicensed for database "@1"', NULL, NULL);
('HLP_SETWIDTH', 'help', 'isql.e', NULL, 17, 58, NULL, '    SET WIDTH <col> [<n>]  -- set/unset print width to <n> for column <col>', NULL, NULL);
('HLP_SETPLAN', 'help', 'isql.e', NULL, 17, 59, NULL, '    SET PLAN               -- toggle display of query access plan', NULL, NULL);
('HLP_SETTIME', 'help()', 'isql.e', NULL, 17, 60, NULL, '    SET TIME               -- toggle display of timestamp with DATE values', NULL, NULL);
('HLP_EDIT2', 'help', 'isql.e', NULL, 17, 61, NULL, 'EDIT                       -- edit current command buffer and execute', NULL, NULL);
('HLP_OUTPUT2', 'help', 'isql.e', NULL, 17, 62, NULL, 'OUTput                     -- return output to stdout', NULL, NULL);
('HLP_SETNAMES', 'help', 'isql.e', NULL, 17, 63, NULL, '    SET NAMES <csname>     -- set name of runtime character set', NULL, NULL);
('HLP_OBJTYPE2', 'help', 'isql.e', NULL, 17, 64, NULL, '               GENERATOR, GRANT, INDEX, PACKAGE, PROCEDURE, ROLE, SQL DIALECT,', NULL, NULL);
('HLP_SETBLOB2', 'help', '65', NULL, 17, 65, NULL, '    SET BLOB               -- turn off BLOB display', NULL, NULL);
('HLP_SET_ROOT', 'help', 'isql.e', NULL, 17, 66, NULL, 'SET      <option>          -- (Use HELP SET for complete list)', NULL, NULL);
('NO_TABLES', 'SHOW_metadata', 'show.e', NULL, 17, 67, NULL, 'There are no tables in this database', NULL, NULL);
('NO_TABLE', 'SHOW_metadata', 'show.e', NULL, 17, 68, NULL, 'There is no table @1 in this database', NULL, NULL);
('NO_VIEWS', 'SHOW_metadata', 'show.e', NULL, 17, 69, NULL, 'There are no views in this database', NULL, NULL);
('NO_VIEW', 'SHOW_metadata', 'show.e', NULL, 17, 70, NULL, 'There is no view @1 in this database', NULL, NULL);
('NO_INDICES_ON_REL', 'SHOW_metadata', 'show.e', NULL, 17, 71, NULL, 'There are no indices on table @1 in this database', NULL, NULL);
('NO_REL_OR_INDEX', 'SHOW_metadata', 'show.e', NULL, 17, 72, NULL, 'There is no table or index @1 in this database', NULL, NULL);
('NO_INDICES', 'SHOW_metadata', 'show.e', NULL, 17, 73, NULL, 'There are no indices in this database', NULL, NULL);
('NO_DOMAIN', 'SHOW_metadata', 'show.e', NULL, 17, 74, NULL, 'There is no domain @1 in this database', NULL, NULL);
('NO_DOMAINS', 'SHOW_metadata', 'show.e', NULL, 17, 75, NULL, 'There are no domains in this database', NULL, NULL);
('NO_EXCEPTION', 'SHOW_metadata', 'show.e', NULL, 17, 76, NULL, 'There is no exception @1 in this database', NULL, NULL);
('NO_EXCEPTIONS', 'SHOW_metadata', 'show.e', NULL, 17, 77, NULL, 'There are no exceptions in this database', NULL, NULL);
('NO_FILTER', 'SHOW_metadata', 'show.e', NULL, 17, 78, NULL, 'There is no filter @1 in this database', NULL, NULL);
('NO_FILTERS', 'SHOW_metadata', 'show.e', NULL, 17, 79, NULL, 'There are no filters in this database', NULL, NULL);
('NO_FUNCTION', 'SHOW_metadata', 'show.e', NULL, 17, 80, NULL, 'There is no user-defined function @1 in this database', NULL, NULL);
('NO_FUNCTIONS', 'SHOW_metadata', 'show.e', NULL, 17, 81, NULL, 'There are no user-defined functions in this database', NULL, NULL);
('NO_GEN', 'SHOW_metadata', 'show.e', NULL, 17, 82, NULL, 'There is no generator @1 in this database', NULL, NULL);
('NO_GENS', 'SHOW_metadata', 'show.e', NULL, 17, 83, NULL, 'There are no generators in this database', NULL, NULL);
('NO_GRANT_ON_REL', 'SHOW_metadata', 'show.e', NULL, 17, 84, NULL, 'There is no privilege granted on table @1 in this database', NULL, NULL);
('NO_GRANT_ON_PROC', 'SHOW_metadata', 'show.e', NULL, 17, 85, NULL, 'There is no privilege granted on stored procedure @1 in this database', NULL, NULL);
('NO_REL_OR_PROC', 'SHOW_metadata', 'show.e', NULL, 17, 86, NULL, 'There is no table or stored procedure @1 in this database', NULL, NULL);
('NO_PROC', 'SHOW_metadata', 'show.e', NULL, 17, 87, NULL, 'There is no stored procedure @1 in this database', NULL, NULL);
('NO_PROCS', 'SHOW_metadata', 'show.e', NULL, 17, 88, NULL, 'There are no stored procedures in this database', NULL, NULL);
('NO_TRIGGERS_ON_REL', 'SHOW_metadata', 'show.e', NULL, 17, 89, NULL, 'There are no triggers on table @1 in this database', NULL, NULL);
('NO_REL_OR_TRIGGER', 'SHOW_metadata', 'show.e', NULL, 17, 90, NULL, 'There is no table or trigger @1 in this database', NULL, NULL);
('NO_TRIGGERS', 'SHOW_metadata', 'show.e', NULL, 17, 91, NULL, 'There are no triggers in this database', NULL, NULL);
('NO_CHECKS_ON_REL', 'SHOW_metadata', 'show.e', NULL, 17, 92, NULL, 'There are no check constraints on table @1 in this database', NULL, NULL);
('REPORT2_WINDOWS_ONLY', 'process_statement (WINDOWS_ONLY)', 'isql.e', NULL, 17, 93, NULL, 'Buffers = !b
Reads = !r
Writes !w
Fetches = !f', NULL, NULL);
('BUFFER_OVERFLOW', 'get_statement', 'isql.e', NULL, 17, 94, NULL, 'Single isql command exceeded maximum buffer size', NULL, NULL);
('NO_ROLES', 'SHOW_metadata', 'show.e', NULL, 17, 95, NULL, 'There are no roles in this database', NULL, NULL);
('NO_OBJECT', 'SHOW_metadata', 'show.e', NULL, 17, 96, NULL, 'There is no metadata object @1 in this database', NULL, NULL);
('NO_GRANT_ON_ROL', 'SHOW_metadata', 'show.e', NULL, 17, 97, NULL, 'There is no membership privilege granted on @1 in this database', NULL, NULL);
('UNEXPECTED_EOF', 'do_isql', 'isql.e', NULL, 17, 98, NULL, 'Expected end of statement, encountered EOF', NULL, NULL);
('TIME_ERR', 'add_row()', 'isql.e', NULL, 17, 101, NULL, 'Bad TIME: @1', NULL, NULL);
('HLP_OBJTYPE3', 'help', 'isql.epp', NULL, 17, 102, NULL, '               SYSTEM, TABLE, TRIGGER, VERSION, USERS, VIEW', NULL, NULL);
(NULL, 'SHOW_metadata', 'show.e', NULL, 17, 103, NULL, 'There is no role @1 in this database', NULL, NULL);
('USAGE_BAIL', 'ISQL_main', 'isql.epp', NULL, 17, 104, NULL, '	-b(ail)                 bail on errors (set bail on)', NULL, NULL);
(NULL, 'create_db', 'isql.e', NULL, 17, 105, NULL, 'Incomplete string in @1', NULL, NULL);
('HLP_SETSQLDIALECT', 'help', 'isql.e', NULL, 17, 106, NULL, '    SET SQL DIALECT <n>    -- set sql dialect to <n>', NULL, NULL);
('NO_GRANT_ON_ANY', 'SHOW_metadata', 'show.e', NULL, 17, 107, NULL, 'There is no privilege granted in this database', NULL, NULL);
('HLP_SETPLANONLY', 'help', 'isql.e', NULL, 17, 108, NULL, '    SET PLANONLY           -- toggle display of query plan without executing', NULL, NULL);
('HLP_SETHEADING', 'help', 'isql.epp', NULL, 17, 109, NULL, '    SET HEADING            -- toggle display of query column titles', NULL, NULL);
('HLP_SETBAIL', 'help', 'isql.epp', NULL, 17, 110, NULL, '    SET BAIL               -- toggle bailing out on errors in non-interactive mode', NULL, NULL);
('USAGE_CACHE', 'ISQL_main', 'isql.epp', NULL, 17, 111, NULL, '	-c(ache) <num>          number of cache buffers', NULL, NULL);
('TIME_PROMPT', 'add_row', 'isql.epp', NULL, 17, 112, NULL, 'Enter @1 as H:M:S>', NULL, NULL);
('TIMESTAMP_PROMPT', 'add_row', 'isql.epp', NULL, 17, 113, NULL, 'Enter @1 as Y/MON/D H:MIN:S[.MSEC]>', NULL, NULL);
('TIMESTAMP_ERR', 'add_row()', 'isql.epp', NULL, 17, 114, NULL, 'Bad TIMESTAMP: @1', NULL, NULL);
('NO_COMMENTS', 'SHOW_metadata', 'show.epp', NULL, 17, 115, NULL, 'There are no comments for objects in this database', NULL, NULL);
('ONLY_FIRST_BLOBS', 'print_line', 'isql.epp', NULL, 17, 116, NULL, 'Printing only the first @1 blobs.', NULL, NULL);
('MSG_TABLES', 'SHOW_metadata', 'show.epp', NULL, 17, 117, NULL, 'Tables:', NULL, NULL);
('MSG_FUNCTIONS', 'SHOW_metadata', 'show.epp', NULL, 17, 118, NULL, 'Functions:', NULL, NULL);
(NULL, 'ISQL_errmsg', 'isql.epp', NULL, 17, 119, NULL, 'At line @1 in file @2', NULL, NULL);
(NULL, 'ISQL_errmsg', 'isql.epp', NULL, 17, 120, NULL, 'After line @1 in file @2', NULL, NULL);
('NO_TRIGGER', 'SHOW_metadata', 'show.epp', NULL, 17, 121, NULL, 'There is no trigger @1 in this database', NULL, NULL);
('USAGE_CHARSET', 'ISQL_main', 'isql.epp', NULL, 17, 122, NULL, '	-ch(arset) <charset>    connection charset (set names)', NULL, NULL);
('USAGE_DATABASE', 'ISQL_main', 'isql.epp', NULL, 17, 123, NULL, '	-d(atabase) <database>  database name to put in script creation', NULL, NULL);
('USAGE_ECHO', 'ISQL_main', 'isql.epp', NULL, 17, 124, NULL, '	-e(cho)                 echo commands (set echo on)', NULL, NULL);
('USAGE_EXTRACT', 'ISQL_main', 'isql.epp', NULL, 17, 125, NULL, '	-ex(tract)              extract metadata', NULL, NULL);
('USAGE_INPUT', 'ISQL_main', 'isql.epp', NULL, 17, 126, NULL, '	-i(nput) <file>         input file (set input)', NULL, NULL);
('USAGE_MERGE', 'ISQL_main', 'isql.epp', NULL, 17, 127, NULL, '	-m(erge)                merge standard error', NULL, NULL);
('USAGE_MERGE2', 'ISQL_main', 'isql.epp', NULL, 17, 128, NULL, '	-m2                     merge diagnostic', NULL, NULL);
('USAGE_NOAUTOCOMMIT', 'ISQL_main', 'isql.epp', NULL, 17, 129, NULL, '	-n(oautocommit)         no autocommit DDL (set autoddl off)', NULL, NULL);
('USAGE_NOWARN', 'ISQL_main', 'isql.epp', NULL, 17, 130, NULL, '	-now(arnings)           do not show warnings', NULL, NULL);
('USAGE_OUTPUT', 'ISQL_main', 'isql.epp', NULL, 17, 131, NULL, '	-o(utput) <file>        output file (set output)', NULL, NULL);
('USAGE_PAGE', 'ISQL_main', 'isql.epp', NULL, 17, 132, NULL, '	-pag(elength) <size>    page length', NULL, NULL);
('USAGE_PASSWORD', 'ISQL_main', 'isql.epp', NULL, 17, 133, NULL, '	-p(assword) <password>  connection password', NULL, NULL);
('USAGE_QUIET', 'ISQL_main', 'isql.epp', NULL, 17, 134, NULL, '	-q(uiet)                do not show the message "Use CONNECT..."', NULL, NULL);
('USAGE_ROLE', 'ISQL_main', 'isql.epp', NULL, 17, 135, NULL, '	-r(ole) <role>          role name', NULL, NULL);
('USAGE_ROLE2', 'ISQL_main', 'isql.epp', NULL, 17, 136, NULL, '	-r2 <role>              role (uses quoted identifier)', NULL, NULL);
('USAGE_SQLDIALECT', 'ISQL_main', 'isql.epp', NULL, 17, 137, NULL, '	-s(qldialect) <dialect> SQL dialect (set sql dialect)', NULL, NULL);
('USAGE_TERM', 'ISQL_main', 'isql.epp', NULL, 17, 138, NULL, '	-t(erminator) <term>    command terminator (set term)', NULL, NULL);
('USAGE_USER', 'ISQL_main', 'isql.epp', NULL, 17, 139, NULL, '	-u(ser) <user>          user name', NULL, NULL);
('USAGE_XTRACT', 'ISQL_main', 'isql.epp', NULL, 17, 140, NULL, '	-x                      extract metadata', NULL, NULL);
('USAGE_VERSION', 'ISQL_main', 'isql.epp', NULL, 17, 141, NULL, '	-z                      show program and server version', NULL, NULL);
('USAGE_NOARG', 'ISQL_main', 'isql.epp', NULL, 17, 142, NULL, 'missing argument for switch "@1"', NULL, NULL);
('USAGE_NOTINT', 'ISQL_main', 'isql.epp', NULL, 17, 143, NULL, 'argument "@1" for switch "@2" is not an integer', NULL, NULL);
('USAGE_RANGE', 'ISQL_main', 'isql.epp', NULL, 17, 144, NULL, 'value "@1" for switch "@2" is out of range', NULL, NULL);
('USAGE_DUPSW', 'ISQL_main', 'isql.epp', NULL, 17, 145, NULL, 'switch "@1" or its equivalent used more than once', NULL, NULL);
('USAGE_DUPDB', 'ISQL_main', 'isql.epp', NULL, 17, 146, NULL, 'more than one database name: "@1", "@2"', NULL, NULL);
('NO_DEPENDENCIES', 'SHOW_metadata', 'show.epp', NULL, 17, 147, NULL, 'No dependencies for @1 were found', NULL, NULL);
('NO_COLLATION', 'SHOW_metadata', 'show.epp', NULL, 17, 148, NULL, 'There is no collation @1 in this database', NULL, NULL);
('NO_COLLATIONS', 'SHOW_metadata', 'show.epp', NULL, 17, 149, NULL, 'There are no user-defined collations in this database', NULL, NULL);
('MSG_COLLATIONS', 'SHOW_metadata', 'show.epp', NULL, 17, 150, NULL, 'Collations:', NULL, NULL);
('NO_SECCLASS', 'SHOW_metadata', 'show.epp', NULL, 17, 151, NULL, 'There are no security classes for @1', NULL, NULL);
('NO_DB_WIDE_SECCLASS', 'SHOW_metadata', 'show.epp', NULL, 17, 152, NULL, 'There is no database-wide security class', NULL, NULL);
('CANNOT_GET_SRV_VER', 'SHOW_metadata', 'show.epp', NULL, 17, 153, NULL, 'Cannot get server version without database connection', NULL, NULL);
('USAGE_NODBTRIGGERS', 'ISQL_main', 'isql.epp', NULL, 17, 154, NULL, '	-nod(btriggers)         do not run database triggers', NULL, NULL);
('USAGE_TRUSTED', 'ISQL_main', 'isql.epp', NULL, 17, 155, NULL, '	-tr(usted)              use trusted authentication', NULL, NULL);
('BULK_PROMPT', 'bulk_insert_hack', 'isql.epp', NULL, 17, 156, NULL, 'BULK> ', NULL, NULL);
-- Do not change the arguments of the previous ISQL messages.
-- Write the new ISQL messages here.
('NO_CONNECTED_USERS', 'SHOW_metadata', 'show.epp', NULL, 17, 157, NULL, 'There are no connected users', NULL, NULL);
('USERS_IN_DB', 'SHOW_metadata', 'show.epp', NULL, 17, 158, NULL, 'Users in the database', NULL, NULL);
('OUTPUT_TRUNCATED', 'SHOW_metadata', 'show.epp', NULL, 17, 159, NULL, 'Output was truncated', NULL, NULL);
('VALID_OPTIONS', 'SHOW_metadata', 'show.epp', NULL, 17, 160, NULL, 'Valid options are:', NULL, NULL);
('USAGE_FETCH', 'ISQL_main', 'isql.epp', NULL, 17, 161, NULL, '	-f(etch_password)       fetch password from file', NULL, NULL);
('PASS_FILE_OPEN', 'ISQL_main', 'isql.epp', NULL, 17, 162, NULL, 'could not open password file @1, errno @2', NULL, NULL);
('PASS_FILE_READ', 'ISQL_main', 'isql.epp', NULL, 17, 163, NULL, 'could not read password file @1, errno @2', NULL, NULL);
('EMPTY_PASS', 'ISQL_main', 'isql.epp', NULL, 17, 164, NULL, 'empty password file @1', NULL, NULL);
('HLP_SETMAXROWS', 'help', 'isql.epp', NULL, 17, 165, NULL, '    SET MAXROWS [<n>]      -- limit select stmt to <n> rows, zero is no limit', NULL, NULL);
('NO_PACKAGE', 'SHOW_metadata', 'show.epp', NULL, 17, 166, NULL, 'There is no package @1 in this database', NULL, NULL)
('NO_PACKAGES', 'SHOW_metadata', 'show.epp', NULL, 17, 167, NULL, 'There are no packages in this database', NULL, NULL)
('NO_SCHEMA', 'SHOW_metadata', 'show.epp', NULL, 17, 168, NULL, 'There is no schema @1 in this database', NULL, NULL)
('NO_SCHEMAS', 'SHOW_metadata', 'show.epp', NULL, 17, 169, NULL, 'There are no schemas in this database', NULL, NULL)
('MAXROWS_INVALID', 'newRowCount', 'isql.epp', NULL, 17, 170, NULL, 'Unable to convert @1 to a number for MAXROWS option', NULL, NULL)
('MAXROWS_OUTOF_RANGE', 'newRowCount', 'isql.epp', NULL, 17, 171, NULL, 'Value @1 for MAXROWS is out of range. Max value is @2', NULL, NULL)
('MAXROWS_NEGATIVE', 'newRowCount', 'isql.epp', NULL, 17, 172, NULL, 'The value (@1) for MAXROWS must be zero or greater', NULL, NULL)
('HLP_SETEXPLAIN', 'help', 'isql.epp', NULL, 17, 173, NULL, '    SET EXPLAIN            -- toggle display of query access plan in the explained form', NULL, NULL)
('NO_GRANT_ON_GEN', 'SHOW_metadata', 'show.e', NULL, 17, 174, NULL, 'There is no privilege granted on generator @1 in this database', NULL, NULL);
('NO_GRANT_ON_XCP', 'SHOW_metadata', 'show.e', NULL, 17, 175, NULL, 'There is no privilege granted on exception @1 in this database', NULL, NULL);
('NO_GRANT_ON_FLD', 'SHOW_metadata', 'show.e', NULL, 17, 176, NULL, 'There is no privilege granted on domain @1 in this database', NULL, NULL);
('NO_GRANT_ON_CS', 'SHOW_metadata', 'show.e', NULL, 17, 177, NULL, 'There is no privilege granted on character set @1 in this database', NULL, NULL);
('NO_GRANT_ON_COLL', 'SHOW_metadata', 'show.e', NULL, 17, 178, NULL, 'There is no privilege granted on collation @1 in this database', NULL, NULL);
('NO_GRANT_ON_PKG', 'SHOW_metadata', 'show.e', NULL, 17, 179, NULL, 'There is no privilege granted on package @1 in this database', NULL, NULL);
('NO_GRANT_ON_FUN', 'SHOW_metadata', 'show.e', NULL, 17, 180, NULL, 'There is no privilege granted on function @1 in this database', NULL, NULL);
('REPORT_NEW1', 'print_performance', 'isql.epp', 'Each of these 4 items is followed by a newline (''\n'').', 17, 181, NULL, 'Current memory = !
Delta memory = !
Max memory = !
Elapsed time= ~ sec
', NULL, NULL);
('REPORT_NEW2', 'print_performance', 'isql.epp', 'Each of these 5 items is followed by a newline (''\n'').', 17, 182, NULL, 'Cpu = ~ sec
', NULL, NULL);
('REPORT_NEW3', 'print_performance', 'isql.epp', 'Each of these 5 items is followed by a newline (''\n'').', 17, 183, NULL, 'Buffers = !
Reads = !
Writes = !
Fetches = !', NULL, NULL);
('NO_MAP', 'SHOW_metadata', 'show.epp', NULL, 17, 184, NULL, 'There is no mapping @1 in this database', NULL, NULL)
('NO_MAPS', 'SHOW_metadata', 'show.epp', NULL, 17, 185, NULL, 'There are no mappings in this database', NULL, NULL)
('INVALID_TERM_CHARS', 'frontend_set', 'isql.epp', NULL, 17, 186, NULL, 'Invalid characters for SET TERMINATOR are @1', NULL, NULL)
('REC_DISPLAYCOUNT', 'process_statement', 'isql.epp', NULL, 17, 187, NULL, 'Records displayed: @1', NULL, NULL)
('COLUMNS_HIDDEN', 'process_statement', 'isql.epp', NULL, 17, 188, NULL, 'Full NULL columns hidden due to RecordBuff: @1', NULL, NULL)
('HLP_SETRECORDBUF', 'help', 'isql.epp', NULL, 17, 189, NULL, '    SET RECORDBuf          -- toggle limited buffering and trimming of columns', NULL, NULL)
-- GSEC
('GsecMsg1', 'get_line', 'gsec.e', NULL, 18, 1, NULL, 'GSEC>', NULL, NULL);
('GsecMsg2', 'printhelp', 'gsec.e', 'This message is used in the Help display. It should be the same as number 1 (but in lower case).', 18, 2, NULL, 'gsec', NULL, NULL);
('GsecMsg3', 'global', 'gsec.e', NULL, 18, 3, NULL, 'ADD            add user', NULL, NULL);
('GsecMsg4', 'global', 'gsec.e', NULL, 18, 4, NULL, 'DELETE         delete user', NULL, NULL);
('GsecMsg5', 'global', 'gsec.e', NULL, 18, 5, NULL, 'DISPLAY        display user(s)', NULL, NULL);
('GsecMsg6', 'global', 'gsec.e', NULL, 18, 6, NULL, 'MODIFY         modify user', NULL, NULL);
('GsecMsg7', 'global', 'gsec.e', NULL, 18, 7, NULL, 'PW             user''s password', NULL, NULL);
('GsecMsg8', 'global', 'gsec.e', NULL, 18, 8, NULL, 'UID            user''s ID', NULL, NULL);
('GsecMsg9', 'global', 'gsec.e', NULL, 18, 9, NULL, 'GID            user''s group ID', NULL, NULL);
('GsecMsg10', 'global', 'gsec.e', NULL, 18, 10, NULL, 'PROJ           user''s project name', NULL, NULL);
('GsecMsg11', 'global', 'gsec.e', NULL, 18, 11, NULL, 'ORG            user''s organization name', NULL, NULL);
('GsecMsg12', 'global', 'gsec.e', NULL, 18, 12, NULL, 'FNAME          user''s first name', NULL, NULL);
('GsecMsg13', 'global', 'gsec.e', NULL, 18, 13, NULL, 'MNAME          user''s middle name/initial', NULL, NULL);
('GsecMsg14', 'global', 'gsec.e', NULL, 18, 14, NULL, 'LNAME          user''s last name', NULL, NULL);
('gsec_cant_open_db', 'common_main', 'gsec.e', NULL, 18, 15, NULL, 'unable to open database', NULL, NULL);
('gsec_switches_error', 'exec_line', 'gsec.e', NULL, 18, 16, NULL, 'error in switch specifications', NULL, NULL);
('gsec_no_op_spec', 'exec_line', 'gsec.e', NULL, 18, 17, NULL, 'no operation specified', NULL, NULL);
('gsec_no_usr_name', 'exec_line', 'gsec.e', NULL, 18, 18, NULL, 'no user name specified', NULL, NULL);
('gsec_err_add', 'exec_line', 'gsec.e', NULL, 18, 19, NULL, 'add record error', NULL, NULL);
('gsec_err_modify', 'exec_line', 'gsec.e', NULL, 18, 20, NULL, 'modify record error', NULL, NULL);
('gsec_err_find_mod', 'exec_line', 'gsec.e', NULL, 18, 21, NULL, 'find/modify record error', NULL, NULL);
('gsec_err_rec_not_found', 'exec_line', 'gsec.e', NULL, 18, 22, NULL, 'record not found for user: @1', NULL, NULL);
('gsec_err_delete', 'exec_line', 'gsec.e', NULL, 18, 23, NULL, 'delete record error', NULL, NULL);
('gsec_err_find_del', 'exec_line', 'gsec.e', NULL, 18, 24, NULL, 'find/delete record error', NULL, NULL);
('GsecMsg25', 'exec_line', 'gsec.e', NULL, 18, 25, NULL, 'users defined for node', NULL, NULL);
('GsecMsg26', 'exec.line', 'gsec', NULL, 18, 26, NULL, '     user name                    uid   gid admin     full name', NULL, NULL);
('GsecMsg27', 'exec_line', 'gsec.e', NULL, 18, 27, NULL, '------------------------------------------------------------------------------------------------', NULL, NULL);
('gsec_err_find_disp', 'exec_line', 'gsec.e', NULL, 18, 28, NULL, 'find/display record error', NULL, NULL);
('gsec_inv_param', 'get_switches', 'gsec.e', NULL, 18, 29, NULL, 'invalid parameter, no switch defined', NULL, NULL);
('gsec_op_specified', 'get_switches', 'gsec.e', NULL, 18, 30, NULL, 'operation already specified', NULL, NULL);
('gsec_pw_specified', 'get_switches', 'gsec.e', NULL, 18, 31, NULL, 'password already specified', NULL, NULL);
('gsec_uid_specified', 'get_switches', 'gsec.e', NULL, 18, 32, NULL, 'uid already specified', NULL, NULL);
('gsec_gid_specified', 'get_switches', 'gsec.c', NULL, 18, 33, NULL, 'gid already specified', NULL, NULL);
('gsec_proj_specified', 'get_switches', 'gsec.e', NULL, 18, 34, NULL, 'project already specified', NULL, NULL);
('gsec_org_specified', 'get_switches', 'gsec.e', NULL, 18, 35, NULL, 'organization already specified', NULL, NULL);
('gsec_fname_specified', 'get_switches', 'gsec.c', NULL, 18, 36, NULL, 'first name already specified', NULL, NULL);
('gsec_mname_specified', 'get_switches', 'gsec.c', NULL, 18, 37, NULL, 'middle name already specified', NULL, NULL);
('gsec_lname_specified', 'get_switches', 'gsec.c', NULL, 18, 38, NULL, 'last name already specified', NULL, NULL);
('GsecMsg39', 'get_switches', 'gsec.e', NULL, 18, 39, NULL, 'gsec version', NULL, NULL);
('gsec_inv_switch', 'get_switches', 'gsec.e', NULL, 18, 40, NULL, 'invalid switch specified', NULL, NULL);
('gsec_amb_switch', 'get_switches', 'gsec.e', NULL, 18, 41, NULL, 'ambiguous switch specified', NULL, NULL);
('gsec_no_op_specified', 'get_switches', 'gsec.e', NULL, 18, 42, NULL, 'no operation specified for parameters', NULL, NULL);
('gsec_params_not_allowed', 'get_switches', 'gsec.e', NULL, 18, 43, NULL, 'no parameters allowed for this operation', NULL, NULL);
('gsec_incompat_switch', 'get_switches', 'gsec.e', NULL, 18, 44, NULL, 'incompatible switches specified', NULL, NULL);
('GsecMsg45', 'printhelp', 'gsec.e', NULL, 18, 45, NULL, 'gsec utility - maintains user password database', NULL, NULL);
('GsecMsg46', 'printhelp', 'gsec.e', NULL, 18, 46, NULL, 'command line usage:', NULL, NULL);
('GsecMsg47', 'printhelp', 'gsec.e', NULL, 18, 47, NULL, '<command> [ <parameter> ... ]', NULL, NULL);
('GsecMsg48', 'printhelp', 'gsec.e', NULL, 18, 48, NULL, 'interactive usage:', NULL, NULL);
('GsecMsg49', 'printhelp', 'gsec.e', NULL, 18, 49, NULL, 'available commands:', NULL, NULL);
('GsecMsgs50', 'printhelp', 'gsec.e', NULL, 18, 50, NULL, 'adding a new user:', NULL, NULL);
('GsecMsg51', 'printhelp', 'gsec.e', NULL, 18, 51, NULL, 'add <name> [ <parameter> ... ]', NULL, NULL);
('GsecMsg52', 'printhelp', 'gsec.e', NULL, 18, 52, NULL, 'deleting a current user:', NULL, NULL);
('GsecMsg53', 'printhelp', 'gsec.e', NULL, 18, 53, NULL, 'delete <name>', NULL, NULL);
('GsecMsg54', 'printhelp', 'gsec.e', NULL, 18, 54, NULL, 'displaying all users:', NULL, NULL);
('GsecMsg55', 'printhelp', 'gsec.e', NULL, 18, 55, NULL, 'display', NULL, NULL);
('GsecMsg56', 'printhelp', 'gsec.e', NULL, 18, 56, NULL, 'displaying one user:', NULL, NULL);
('GsecMsg57', 'printhelp', 'gsec.e', NULL, 18, 57, NULL, 'display <name>', NULL, NULL);
('GsecMsg58', 'printhelp', 'gsec.e', NULL, 18, 58, NULL, 'modifying a user''s parameters:', NULL, NULL);
('GsecMsg59', 'printhelp', 'gsec.e', NULL, 18, 59, NULL, 'modify <name> <parameter> [ <parameter> ... ]', NULL, NULL);
('GsecMsg60', 'printhelp', 'gsec.e', NULL, 18, 60, NULL, 'help:', NULL, NULL);
('GsecMsg61', 'printhelp', 'gsec.e', NULL, 18, 61, NULL, '? (interactive only)', NULL, NULL);
('GsecMsg62', 'printhelp', 'gsec.e', NULL, 18, 62, NULL, 'help', NULL, NULL);
('GsecMsg63', 'printhelp', 'gsec.e', NULL, 18, 63, NULL, 'quit interactive session:', NULL, NULL);
('GsecMsg64', 'printhelp', 'gsec.e', NULL, 18, 64, NULL, 'quit (interactive only)', NULL, NULL);
('GsecMsg65', 'printhelp', 'gsec.e', NULL, 18, 65, NULL, 'available parameters:', NULL, NULL);
('GsecMsg66', 'printhelp', 'gsec.e', NULL, 18, 66, NULL, '-pw <password>', NULL, NULL);
('GsecMsg67', 'printhelp', 'gsec.e', NULL, 18, 67, NULL, '-uid <uid>', NULL, NULL);
('GsecMsg68', 'printhelp', 'gsec.e', NULL, 18, 68, NULL, '-gid <uid>', NULL, NULL);
('GsecMsg69', 'printhelp', 'gsec.e', NULL, 18, 69, NULL, '-proj <projectname>', NULL, NULL);
('GsecMsg70', 'printhelp', 'gsec.e', NULL, 18, 70, NULL, '-org <organizationname>', NULL, NULL);
('GsecMsg71', 'printhelp', 'gsec.e', NULL, 18, 71, NULL, '-fname <firstname>', NULL, NULL);
('GsecMsg72', 'printhelp', 'gsec.e', NULL, 18, 72, NULL, '-mname <middlename>', NULL, NULL);
('GsecMsg73', 'printhelp', 'gsec.e', NULL, 18, 73, NULL, '-lname <lastname>', NULL, NULL);
(NULL, 'exec_line', 'gsec_exec.c', NULL, 18, 74, NULL, 'gsec - memory allocation error', NULL, NULL);
(NULL, 'exec_line', 'gsec_exec.c', NULL, 18, 75, NULL, 'gsec error', NULL, NULL);
('gsec_inv_username', 'get_switches', 'gsec.c', NULL, 18, 76, NULL, 'Invalid user name (maximum 31 bytes allowed)', NULL, NULL);
('gsec_inv_pw_length', 'get_switches', 'gsec.c', NULL, 18, 77, NULL, 'Warning - maximum 8 significant bytes of password used', NULL, NULL);
('gsec_db_specified', 'get_switches', 'gsec.c', NULL, 18, 78, NULL, 'database already specified', NULL, NULL);
('gsec_db_admin_specified', 'get_switches', 'gsec.c', NULL, 18, 79, NULL, 'database administrator name already specified', NULL, NULL);
('gsec_db_admin_pw_specified', 'get_switches', 'gsec.c', NULL, 18, 80, NULL, 'database administrator password already specified', NULL, NULL);
('gsec_sql_role_specified', 'get_switches', 'gsec.c', NULL, 18, 81, NULL, 'SQL role name already specified', NULL, NULL);
('GsecMsg82', 'printhelp', 'gsec.c', NULL, 18, 82, NULL, '[ <options> ... ]', NULL, NULL);
('GsecMsg83', 'printhelp', 'gsec.c', NULL, 18, 83, NULL, 'available options:', NULL, NULL);
('GsecMsg84', 'printhelp', 'gsec.c', NULL, 18, 84, NULL, '-user <database administrator name>', NULL, NULL);
('GsecMsg85', 'printhelp', 'gsec.c', NULL, 18, 85, NULL, '-password <database administrator password>', NULL, NULL);
('GsecMsg86', 'printhelp', 'gsec.c', NULL, 18, 86, NULL, '-role <database administrator SQL role name>', NULL, NULL);
('GsecMsg87', 'printhelp', 'gsec.c', NULL, 18, 87, NULL, '-database <database to manage>', NULL, NULL);
('GsecMsg88', 'printhelp', 'gsec.c', NULL, 18, 88, NULL, '-z', NULL, NULL);
('GsecMsg89', 'printhelp', 'gsec.c', NULL, 18, 89, NULL, 'displaying version number:', NULL, NULL);
('GsecMsg90', 'printhelp', 'gsec.c', NULL, 18, 90, NULL, 'z (interactive only)', NULL, NULL);
-- Do not change the arguments of the previous GSEC messages.
-- Write the new GSEC messages here.
('GsecMsg91', 'printhelp', 'gsec.cpp', NULL, 18, 91, NULL, '-trusted (use trusted authentication)', NULL, NULL);
('GsecMsg92', 'common_main', 'gsec.cpp', NULL, 18, 92, NULL, 'invalid switch specified in interactive mode', NULL, NULL);
('GsecMsg93', 'common_main', 'gsec.cpp', NULL, 18, 93, NULL, 'error closing security database', NULL, NULL);
('GsecMsg94', 'SECURITY_exec_line', 'security.epp', NULL, 18, 94, NULL, 'error releasing request in security database', NULL, NULL);
('GsecMsg95', 'printhelp', 'gsec.cpp', NULL, 18, 95, NULL, '-fetch_password <file to fetch password from>', NULL, NULL);
('GsecMsg96', 'get_switches', 'gsec.cpp', NULL, 18, 96, NULL, 'error fetching password from file', NULL, NULL);
('GsecMsg97', 'SECURITY_exec_line', 'security.epp', NULL, 18, 97, NULL, 'error changing AUTO ADMINS MAPPING in security database', NULL, NULL);
('GsecMsg98', 'printhelp', 'gsec.cpp', NULL, 18, 98, NULL, 'changing admins mapping to RDB$ADMIN role in security database:', NULL, NULL);
('GsecMsg99', 'get_switches', 'gsec.cpp', NULL, 18, 99, NULL, 'invalid parameter for -MAPPING, only SET or DROP is accepted', NULL, NULL);
('GsecMsg100', 'printhelp', 'gsec.cpp', NULL, 18, 100, NULL, 'mapping {set|drop}', NULL, NULL);
('GsecMsg101', 'gsec', 'gsec.cpp', NULL, 18, 101, NULL, 'use gsec -? to get help', NULL, NULL);
('GsecMsg102', 'gsec', 'gsec.cpp', NULL, 18, 102, NULL, '-admin {yes|no}', NULL, NULL);
('GsecMsg103', 'gsec', 'gsec.cpp', NULL, 18, 103, NULL, 'invalid parameter for -ADMIN, only YES or NO is accepted', NULL, NULL);
('GsecMsg104', 'SECURITY_exec_line', 'security.epp', NULL, 18, 104, NULL, 'not enough privileges to complete operation', NULL, NULL);
-- GSTAT
('gstat_unknown_switch', 'main', 'dba.e', NULL, 21, 1, NULL, 'found unknown switch', NULL, NULL);
('gstat_retry', 'main', 'dba.e', NULL, 21, 2, NULL, 'please retry, giving a database name', NULL, NULL);
('gstat_wrong_ods', 'main', 'dba.e', NULL, 21, 3, NULL, 'Wrong ODS version, expected @1, encountered @2', NULL, NULL);
('gstat_unexpected_eof', 'db_read', 'dba.e', NULL, 21, 4, NULL, 'Unexpected end of database file.', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 5, NULL, 'gstat version @1', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 6, NULL, '
Database "@1"', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 7, NULL, '

Database file sequence:', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 8, NULL, 'File @1 continues as file @2', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 9, NULL, 'File @1 is the @2 file', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 10, NULL, '
Analyzing database pages ...', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 11, NULL, '    Primary pointer page: @1, Index root page: @2', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 12, NULL, '    Data pages: @1, data page slots: @2, average fill: @3', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 13, NULL, '    Fill distribution:', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 14, NULL, '    Index @1 (@2)', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 15, NULL, '	Depth: @1, leaf buckets: @2, nodes: @3', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 16, NULL, '	Average data length: @1, total dup: @2, max dup: @3', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 17, NULL, '	Fill distribution:', NULL, NULL);
(NULL, 'analyze_data', 'dba.e', NULL, 21, 18, NULL, '    Expected data on page @1', NULL, NULL);
(NULL, 'analyze_index', 'dba.e', NULL, 21, 19, NULL, '    Expected b-tree bucket on page @1 from @2', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 20, NULL, 'unknown switch "@1"', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 21, NULL, 'Available switches:', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 22, NULL, '    -a      analyze data and index pages', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 23, NULL, '    -d      analyze data pages', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 24, NULL, '    -h      analyze header page ONLY', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 25, NULL, '    -i      analyze index leaf pages', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 26, NULL, '    -l      analyze log page', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 27, NULL, '    -s      analyze system relations in addition to user tables', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 28, NULL, '    -z      display version number', NULL, NULL);
('gstat_open_err', 'db_open', 'dba.e', NULL, 21, 29, NULL, 'Can''t open database file @1', NULL, NULL);
('gstat_read_err', 'db_read', 'dba.e', NULL, 21, 30, NULL, 'Can''t read a database page', NULL, NULL);
('gstat_sysmemex', 'alloc', 'dba.e', NULL, 21, 31, NULL, 'System memory exhausted', NULL, NULL);
('gstat_username', NULL, 'dba.e', NULL, 21, 32, NULL, '    -u      username', NULL, NULL);
('gstat_password', NULL, 'dba.e', NULL, 21, 33, NULL, '    -p      password', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 34, NULL, '    -r      analyze average record and version length', NULL, NULL);
(NULL, 'main', 'dba.e', NULL, 21, 35, NULL, '    -t      tablename <tablename2...> (case sensitive)', NULL, NULL);
-- Do not change the arguments of the previous GSTAT messages.
-- Write the new GSTAT messages here.
(NULL, 'main', 'dba.epp', NULL, 21, 36, NULL, '    -tr     use trusted authentication', NULL, NULL);
(NULL, 'main', 'dba.epp', NULL, 21, 37, NULL, '    -fetch  fetch password from file', NULL, NULL);
(NULL, 'main', 'dba.epp', NULL, 21, 38, NULL, 'option -h is incompatible with options -a, -d, -i, -r, -s and -t', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 39, NULL, 'usage:   gstat [options] <database> or gstat <database> [options]', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 40, NULL, 'database name was already specified', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 41, NULL, 'option -t needs a table name', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 42, NULL, 'option -t got a too long table name @1', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 43, NULL, 'option -t accepts several table names only if used after <database>', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 44, NULL, 'table "@1" not found', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 45, NULL, 'use gstat -? to get help', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 46, NULL, '    Primary pages: @1, full pages: @2, swept pages: @3', NULL, NULL);
(NULL, 'main', 'dba.epp', NULL, 21, 47, NULL, '    Big record pages: @1', NULL, NULL);
(NULL, 'main', 'dba.epp', NULL, 21, 48, NULL, '    Blobs: @1, total length: @2, blob pages: @3', NULL, NULL);
(NULL, 'main', 'dba.epp', NULL, 21, 49, NULL, '        Level 0: @1, Level 1: @2, Level 2: @3', NULL, NULL);
(NULL, 'main', 'dba.epp', NULL, 21, 50, NULL, 'option -e is incompatible with options -a, -d, -h, -i, -r, -s and -t', NULL, NULL)
(NULL, 'dba_in_sw_table', 'dbaswi.h', NULL, 21, 51, NULL, '    -e      analyze database encryption', NULL, NULL);
(NULL, 'main', 'dba.epp', NULL, 21, 52, NULL, 'Data pages: total @1, encrypted @2, non-crypted @3', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 53, NULL, 'Index pages: total @1, encrypted @2, non-crypted @3', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 54, NULL, 'Blob pages: total @1, encrypted @2, non-crypted @3', NULL, NULL)
(NULL, 'main', 'dba.epp', NULL, 21, 55, NULL, 'no encrypted database support, only -e and -h can be used', NULL, NULL)
-- FBSVCMGR
-- All messages use the new format.
('fbsvcmgr_bad_am', 'putAccessMode', 'fbsvcmgr.cpp', NULL, 22, 1, NULL, 'Wrong value for access mode', NULL, NULL);
('fbsvcmgr_bad_wm', 'putWriteMode', 'fbsvcmgr.cpp', NULL, 22, 2, NULL, 'Wrong value for write mode', NULL, NULL);
('fbsvcmgr_bad_rs', 'putReserveSpace', 'fbsvcmgr.cpp', NULL, 22, 3, NULL, 'Wrong value for reserve space', NULL, NULL);
('fbsvcmgr_info_err', 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 4, NULL, 'Unknown tag (@1) in info_svr_db_info block after isc_svc_query()', NULL, NULL);
('fbsvcmgr_query_err', 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 5, NULL, 'Unknown tag (@1) in isc_svc_query() results', NULL, NULL);
('fbsvcmgr_switch_unknown', 'main', 'fbsvcmgr.cpp', NULL, 22, 6, NULL, 'Unknown switch "@1"', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 7, NULL, 'Service Manager Version', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 8, NULL, 'Server version', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 9, NULL, 'Server implementation', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 10, NULL, 'Path to firebird.msg', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 11, NULL, 'Server root', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 12, NULL, 'Path to lock files', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 13, NULL, 'Security database', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 14, NULL, 'Databases', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 15, NULL, '   Database in use', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 16, NULL, '   Number of attachments', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 17, NULL, '   Number of databases', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 18, NULL, 'Information truncated', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 19, NULL, 'Usage: fbsvcmgr manager-name switches...', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 20, NULL, 'Manager-name should be service_mgr, may be prefixed with host name', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 21, NULL, 'according to common rules (host:service_mgr, \\host\service_mgr).', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 22, NULL, 'Switches exactly match SPB tags, used in abbreviated form.', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 23, NULL, 'Remove isc_, spb_ and svc_ parts of tag and you will get the switch.', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 24, NULL, 'For example: isc_action_svc_backup is specified as action_backup,', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 25, NULL, '             isc_spb_dbname => dbname,', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 26, NULL, '             isc_info_svc_implementation => info_implementation,', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 27, NULL, '             isc_spb_prp_db_online => prp_db_online and so on.', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 28, NULL, 'You may specify single action or multiple info items when calling fbsvcmgr once.', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 29, NULL, 'Full command line samples:', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 30, NULL, 'fbsvcmgr service_mgr user sysdba password masterkey action_db_stats dbname employee sts_hdr_pages', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 31, NULL, '  (will list header info in database employee on local machine)', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 32, NULL, 'fbsvcmgr yourserver:service_mgr user sysdba password masterkey info_server_version info_svr_db_info', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 33, NULL, '  (will show firebird version and databases usage on yourserver)', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 34, NULL, 'Transaction @1 is in limbo', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 35, NULL, 'Multidatabase transaction @1 is in limbo', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 36, NULL, 'Host Site: @1', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 37, NULL, 'Transaction @1', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 38, NULL, 'has been prepared', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 39, NULL, 'has been committed', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 40, NULL, 'has been rolled back', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 41, NULL, 'is not available', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 42, NULL, 'Remote Site: @1', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 43, NULL, 'Database Path: @1', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 44, NULL, 'Automated recovery would commit this transaction', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 45, NULL, 'Automated recovery would rollback this transaction', NULL, NULL);
(NULL, 'printInfo', 'fbsvcmgr.cpp', NULL, 22, 46, NULL, 'No idea should it be commited or rolled back', NULL, NULL);
('fbsvcmgr_bad_sm', 'putShutdownMode', 'fbsvcmgr.cpp', NULL, 22, 47, NULL, 'Wrong value for shutdown mode', NULL, NULL);
('fbsvcmgr_fp_open', 'putFileArgument', 'fbsvcmgr.cpp', NULL, 22, 48, NULL, 'could not open file @1', NULL, NULL);
('fbsvcmgr_fp_read', 'putFileArgument', 'fbsvcmgr.cpp', NULL, 22, 49, NULL, 'could not read file @1', NULL, NULL);
('fbsvcmgr_fp_empty', 'putFileArgument', 'fbsvcmgr.cpp', NULL, 22, 50, NULL, 'empty file @1', NULL, NULL);
(NULL, 'main', 'fbsvcmgr.cpp', NULL, 22, 51, NULL, 'Firebird Services Manager version @1', NULL, NULL)
('fbsvcmgr_bad_arg', 'populateSpbFromSwitches', 'fbsvcmgr.cpp', NULL, 22, 52, NULL, 'Invalid or missing parameter for switch @1', NULL, NULL)
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 53, NULL, 'To get full list of known services run with -? switch', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 54, NULL, 'Attaching to services manager:', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 55, NULL, 'Information requests:', NULL, NULL);
(NULL, 'usage', 'fbsvcmgr.cpp', NULL, 22, 56, NULL, 'Actions:', NULL, NULL);
(NULL, 'printCapabilities', 'fbsvcmgr.cpp', NULL, 22, 57, NULL, 'Server capabilities:', NULL, NULL);
-- UTL (messages common for many utilities)
-- All messages use the new format.
('utl_trusted_switch', 'checkService', 'UtilSvc.cpp', NULL, 23, 1, NULL, 'Switches trusted_user and trusted_role are not supported from command line', NULL, NULL);
-- NBACKUP
-- All messages use the new format.
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 1, NULL, 'ERROR: ', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 2, NULL, 'Physical Backup Manager    Copyright (C) 2004 Firebird development team', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 3, NULL, '  Original idea is of Sean Leyne <sean@@broadviewsoftware.com>', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 4, NULL, '  Designed and implemented by Nickolay Samofatov <skidder@@bssys.com>', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 5, NULL, '  This work was funded through a grant from BroadView Software, Inc.\n', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 6, NULL, 'Usage: nbackup <options>', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 7, NULL, 'exclusive options are:', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 8, NULL, '  -L(OCK) <database>                     Lock database for filesystem copy', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 9, NULL, '  -UN(LOCK) <database>                   Unlock previously locked database', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 10, NULL, '  -F(IXUP) <database>                    Fixup database after filesystem copy', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 11, NULL, '  -B(ACKUP) <level> <db> [<file>]        Create incremental backup', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 12, NULL, '  -R(ESTORE) <db> [<file0> [<file1>...]] Restore incremental backup', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 13, NULL, '  -U(SER) <user>                         User name', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 14, NULL, '  -P(ASSWORD) <password>                 Password', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 15, NULL, '  -FETCH_PASSWORD <file>                 Fetch password from file', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 16, NULL, '  -NOD(BTRIGGERS)                        Do not run database triggers', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 17, NULL, '  -S(IZE)                                Print database size in pages after lock', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 18, NULL, '  -Z                                     Print program version', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 19, NULL, 'Notes:', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 20, NULL, '  <database> may specify database alias.', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 21, NULL, '  Incremental backups of multi-file databases are not supported yet.', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 22, NULL, '  "stdout" may be used as a value of <filename> for -B option.', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 23, NULL, 'PROBLEM ON "@1".', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 24, NULL, 'general options are:', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 25, NULL, 'switches can be abbreviated to the unparenthesized characters', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 26, NULL, '  Option -S(IZE) only is valid together with -L(OCK).', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 27, NULL, '  For historical reasons, -N is equivalent to -UN(LOCK)', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 28, NULL, '  and -T is equivalent to -NOD(BTRIGGERS).', NULL, NULL)
('nbackup_missing_param', 'missingParameterForSwitch', 'nbackup.cpp', NULL, 24, 29, NULL, 'Missing parameter for switch @1', NULL, NULL)
('nbackup_allowed_switches', 'singleAction', 'nbackup.cpp', NULL, 24, 30, NULL, 'Only one of -LOCK, -UNLOCK, -FIXUP, -BACKUP or -RESTORE should be specified', NULL, NULL)
('nbackup_unknown_param', 'nbackup', 'nbackup.cpp', NULL, 24, 31, NULL, 'Unrecognized parameter @1', NULL, NULL)
('nbackup_unknown_switch', 'nbackup', 'nbackup.cpp', NULL, 24, 32, NULL, 'Unknown switch @1', NULL, NULL)
('nbackup_nofetchpw_svc', 'nbackup', 'nbackup.cpp', NULL, 24, 33, NULL, 'Fetch password can''t be used in service mode', NULL, NULL)
('nbackup_pwfile_error', 'nbackup', 'nbackup.cpp', NULL, 24, 34, NULL, 'Error working with password file "@1"', NULL, NULL)
('nbackup_size_with_lock', 'nbackup', 'nbackup.cpp', NULL, 24, 35, NULL, 'Switch -SIZE can be used only with -LOCK', NULL, NULL)
('nbackup_no_switch', 'nbackup', 'nbackup.cpp', NULL, 24, 36, NULL, 'None of -LOCK, -UNLOCK, -FIXUP, -BACKUP or -RESTORE specified', NULL, NULL)
(NULL, 'b_error::raise', 'nbackup.cpp', NULL, 24, 37, NULL, 'Failure: ', NULL, NULL)
(NULL, 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 38, NULL, 'Enter name of the backup file of level @1 ("." - do not restore further):', NULL, NULL)
('nbackup_err_read', 'NBackup::read_file', 'nbackup.cpp', NULL, 24, 39, NULL, 'IO error reading file: @1', NULL, NULL)
('nbackup_err_write', 'NBackup::write_file', 'nbackup.cpp', NULL, 24, 40, NULL, 'IO error writing file: @1', NULL, NULL)
('nbackup_err_seek', 'NBackup::seek_file', 'nbackup.cpp', NULL, 24, 41, NULL, 'IO error seeking file: @1', NULL, NULL)
('nbackup_err_opendb', 'NBackup::open_database_(write/scan)', 'nbackup.cpp', NULL, 24, 42, NULL, 'Error opening database file: @1', NULL, NULL)
('nbackup_err_fadvice', 'NBackup::open_database_scan', 'nbackup.cpp', NULL, 24, 43, NULL, 'Error in posix_fadvise(@1) for database @2', NULL, NULL)
('nbackup_err_createdb', 'NBackup::create_database', 'nbackup.cpp', NULL, 24, 44, NULL, 'Error creating database file: @1', NULL, NULL)
('nbackup_err_openbk', 'NBackup::open_backup_scan', 'nbackup.cpp', NULL, 24, 45, NULL, 'Error opening backup file: @1', NULL, NULL)
('nbackup_err_createbk', 'NBackup::create_backup', 'nbackup.cpp', NULL, 24, 46, NULL, 'Error creating backup file: @1', NULL, NULL)
('nbackup_err_eofdb', 'NBackup::fixup_database', 'nbackup.cpp', NULL, 24, 47, NULL, 'Unexpected end of database file @1', NULL, NULL)
('nbackup_fixup_wrongstate', 'NBackup::fixup_database', 'nbackup.cpp', NULL, 24, 48, NULL, 'Database @1 is not in state (@2) to be safely fixed up', NULL, NULL)
('nbackup_err_db', 'NBackup::pr_error', 'nbackup.cpp', NULL, 24, 49, NULL, 'Database error', NULL, NULL)
('nbackup_userpw_toolong', 'NBackup::attach_database', 'nbackup.cpp', NULL, 24, 50, NULL, 'Username or password is too long', NULL, NULL)
('nbackup_lostrec_db', 'NBackup::backup_database', 'nbackup.cpp', NULL, 24, 51, NULL, 'Cannot find record for database "@1" backup level @2 in the backup history', NULL, NULL)
('nbackup_lostguid_db', 'NBackup::backup_database', 'nbackup.cpp', NULL, 24, 52, NULL, 'Internal error. History query returned null SCN or GUID', NULL, NULL)
('nbackup_err_eofhdrdb', 'NBackup::backup_database', 'nbackup.cpp', NULL, 24, 53, NULL, 'Unexpected end of file when reading header of database file "@1" (stage @2)', NULL, NULL)
('nbackup_db_notlock', 'NBackup::backup_database', 'nbackup.cpp', NULL, 24, 54, NULL, 'Internal error. Database file is not locked. Flags are @1', NULL, NULL)
('nbackup_lostguid_bk', 'NBackup::backup_database', 'nbackup.cpp', NULL, 24, 55, NULL, 'Internal error. Cannot get backup guid clumplet', NULL, NULL)
('nbackup_page_changed', 'NBackup::backup_database', 'nbackup.cpp', NULL, 24, 56, NULL, 'Internal error. Database page @1 had been changed during backup (page SCN=@2, backup SCN=@3)', NULL, NULL)
('nbackup_dbsize_inconsistent', 'NBackup::backup_database', 'nbackup.cpp', NULL, 24, 57, NULL, 'Database file size is not a multiple of page size', NULL, NULL)
('nbackup_failed_lzbk', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 58, NULL, 'Level 0 backup is not restored', NULL, NULL)
('nbackup_err_eofhdrbk', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 59, NULL, 'Unexpected end of file when reading header of backup file: @1', NULL, NULL)
('nbackup_invalid_incbk', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 60, NULL, 'Invalid incremental backup file: @1', NULL, NULL)
('nbackup_unsupvers_incbk', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 61, NULL, 'Unsupported version @1 of incremental backup file: @2', NULL, NULL)
('nbackup_invlevel_incbk', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 62, NULL, 'Invalid level @1 of incremental backup file: @2, expected @3', NULL, NULL)
('nbackup_wrong_orderbk', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 63, NULL, 'Wrong order of backup files or invalid incremental backup file detected, file: @1', NULL, NULL)
('nbackup_err_eofbk', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 64, NULL, 'Unexpected end of backup file: @1', NULL, NULL)
('nbackup_err_copy', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 65, NULL, 'Error creating database file: @1 via copying from: @2', NULL, NULL)
('nbackup_err_eofhdr_restdb', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 66, NULL, 'Unexpected end of file when reading header of restored database file (stage @1)', NULL, NULL)
('nbackup_lostguid_l0bk', 'NBackup::restore_database', 'nbackup.cpp', NULL, 24, 67, NULL, 'Cannot get backup guid clumplet from L0 backup', NULL, NULL)
(NULL, 'nbackup', 'nbackup.cpp', NULL, 24, 68, NULL, 'Physical Backup Manager version @1', NULL, NULL)
(NULL, 'restore_database', 'nbackup.cpp', NULL, 24, 69, NULL, 'Enter name of the backup file of level @1 ("." - do not restore further):', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 70, NULL, '  -D(IRECT) [ON | OFF]                   Use or not direct I/O when backing up database', NULL, NULL)
('nbackup_switchd_parameter', 'main', 'nbackup.cpp', NULL, 24, 71, NULL, 'Wrong parameter @1 for switch -D, need ON or OFF', NULL, NULL)
(NULL, 'usage', 'nbackup.cpp', NULL, 24, 72, NULL, 'special options are:', NULL, NULL)
('nbackup_user_stop', 'checkCtrlC()', 'nbackup.cpp', NULL, 24, 73, NULL, 'Terminated due to user request', NULL, NULL)
-- FBTRACEMGR
-- All messages use the new format.
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 1, NULL, 'Firebird Trace Manager version @1', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 2, NULL, 'ERROR: ', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 3, NULL, 'Firebird Trace Manager.', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 4, NULL, 'Usage: fbtracemgr <action> [<parameters>]', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 5, NULL, 'Actions:', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 6, NULL, '  -STA[RT]                              Start trace session', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 7, NULL, '  -STO[P]                               Stop trace session', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 8, NULL, '  -SU[SPEND]                            Suspend trace session', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 9, NULL, '  -R[ESUME]                             Resume trace session', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 10, NULL, '  -L[IST]                               List existing trace sessions', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 11, NULL, '  -Z                                    Show program version', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 12, NULL, 'Action parameters:', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 13, NULL, '  -N[AME]    <string>                   Session name', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 14, NULL, '  -I[D]      <number>                   Session ID', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 15, NULL, '  -C[ONFIG]  <string>                   Trace configuration file name', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 16, NULL, 'Connection parameters:', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 17, NULL, '  -SE[RVICE]  <string>                  Service name', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 18, NULL, '  -U[SER]     <string>                  User name', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 19, NULL, '  -P[ASSWORD] <string>                  Password', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 20, NULL, '  -FE[TCH]    <string>                  Fetch password from file', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 21, NULL, '  -T[RUSTED]  <string>                  Force trusted authentication', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 22, NULL, 'Examples:', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 23, NULL, '  fbtracemgr -SE remote_host:service_mgr -USER SYSDBA -PASS masterkey -LIST', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 24, NULL, '  fbtracemgr -SE service_mgr -START -NAME my_trace -CONFIG my_cfg.txt', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 25, NULL, '  fbtracemgr -SE service_mgr -SUSPEND -ID 2', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 26, NULL, '  fbtracemgr -SE service_mgr -RESUME -ID 2', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 27, NULL, '  fbtracemgr -SE service_mgr -STOP -ID 4', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 28, NULL, 'Notes:', NULL, NULL)
(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, 29, NULL, '  Press CTRL+C to stop interactive trace session', NULL, NULL)
('trace_conflict_acts', 'usage', 'TraceCmdLine.cpp', NULL, 25, 30, NULL, 'conflicting actions "@1" and "@2" found', NULL, NULL)
('trace_act_notfound', 'usage', 'TraceCmdLine.cpp', NULL, 25, 31, NULL, 'action switch not found', NULL, NULL)
('trace_switch_once', 'usage', 'traceCmdLine.cpp', NULL, 25, 32, NULL, 'switch "@1" must be set only once', NULL, NULL)
('trace_param_val_miss', 'usage', 'TraceCmdLine.cpp', NULL, 25, 33, NULL, 'value for switch "@1" is missing', NULL, NULL)
('trace_param_invalid', 'usage', 'TraceCmdLine.cpp', NULL, 25, 34, NULL, 'invalid value ("@1") for switch "@2"', NULL, NULL)
('trace_switch_unknown', 'usage', 'TraceCmdLine.cpp', NULL, 25, 35, NULL, 'unknown switch "@1" encountered', NULL, NULL)
('trace_switch_svc_only', 'usage', 'TraceCmdLine.cpp', NULL, 25, 36, NULL, 'switch "@1" can be used by service only', NULL, NULL)
('trace_switch_user_only', 'usage', 'TraceCmdLine.cpp', NULL, 25, 37, NULL, 'switch "@1" can be used by interactive user only', NULL, NULL)
('trace_switch_param_miss', 'usage', 'TraceCmdLine.cpp', NULL, 25, 38, NULL, 'mandatory parameter "@1" for switch "@2" is missing', NULL, NULL)
('trace_param_act_notcompat', 'usage', 'TraceCmdLine.cpp', NULL, 25, 39, NULL, 'parameter "@1" is incompatible with action "@2"', NULL, NULL)
('trace_mandatory_switch_miss', 'usage', 'TraceCmdLine.cpp', NULL, 25, 40, NULL, 'mandatory switch "@1" is missing', NULL, NULL)
--(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, , NULL, '', NULL, NULL)
--(NULL, 'usage', 'TraceCmdLine.cpp', NULL, 25, , NULL, '', NULL, NULL)
stop

COMMIT WORK;
