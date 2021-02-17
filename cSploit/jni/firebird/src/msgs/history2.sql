set bulk_insert INSERT INTO HISTORY (CHANGE_NUMBER, CHANGE_WHO, CHANGE_DATE, FAC_CODE, NUMBER, OLD_TEXT, OLD_ACTION, OLD_EXPLANATION, LOCALE) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);
--
(1, 'sriram', '1993-06-14 10:51:23', 16, 5, 'enter file size or <Ctrl-D> to end input', NULL, NULL, 'c_pg');
(2, 'sriram', '1993-06-25 15:13:23', 15, 283, 'checksum error when reading log record', NULL, NULL, 'c_pg');
(3, 'caro', '1993-07-14 13:32:19', 2, 339, 'minimum of %d shared cache pages required', NULL, NULL, 'c_pg');
(4, 'caro', '1993-07-14 14:03:25', 2, 339, 'minimum of %s cache pages required', NULL, NULL, 'c_pg');
(5, 'andreww', '1993-07-20 11:46:49', 12, 205, 'skipped %dth byte scanning for next valid attribute, encountered %d', NULL, NULL, 'c_pg');
(6, 'andreww', '1993-07-20 11:51:17', 12, 205, 'skipped %d bytes looking for next valid attribute, encountered attribute %d', NULL, NULL, 'c_pg');
(7, 'andreww', '1993-07-20 11:51:52', 12, 205, 'skipped %d bytes looking for next valid attribute, encountered attr. %d', NULL, NULL, 'c_pg');
(8, 'caro', '1993-07-30 12:32:24', 0, 186, 'database shutdown in progress', NULL, NULL, 'c_pg');
(9, 'caro', '1993-07-30 12:35:07', 0, 208, 'database %s is shutdown', NULL, NULL, 'c_pg');
(10, 'caro', '1993-07-30 12:36:07', 0, 237, 'Database shutdown unsuccessful', NULL, NULL, 'c_pg');
(11, 'eds', '1993-08-12 14:45:08', 17, 3, 'Use SET SCHEMA/DB or CREATE DATABASE to specify a database', NULL, NULL, 'c_pg');
(12, 'sriram', '1993-08-13 12:21:22', 16, 159, 'Disabling database (id = %d).', NULL, NULL, 'c_pg');
(13, 'sriram', '1993-08-13 12:21:46', 16, 160, 'Database (id = %d) not enabled with this journal server.', NULL, NULL, 'c_pg');
(14, 'sriram', '1993-08-13 14:18:52', 16, 159, 'Disabling database %s.', NULL, NULL, 'c_pg');
(15, 'sriram', '1993-08-13 14:20:37', 16, 143, 'Database (%s) not enabled with this journal server.', NULL, NULL, 'c_pg');
(16, 'sriram', '1993-08-13 14:24:29', 16, 177, 'Database (id = %d) not found.', NULL, NULL, 'c_pg');
(17, 'sriram', '1993-08-13 14:28:35', 16, 160, 'Database %s not enabled with this journal server.', NULL, NULL, 'c_pg');
(18, 'sriram', '1993-08-13 14:32:07', 16, 108, 'Release connection failed', NULL, NULL, 'c_pg');
(19, 'sriram', '1993-08-13 14:32:27', 16, 145, 'Removing file %s', NULL, NULL, 'c_pg');
(20, 'sriram', '1993-08-13 14:36:05', 16, 147, 'Error in unlink () for file %s', NULL, NULL, 'c_pg');
(21, 'sriram', '1993-08-13 14:36:39', 16, 144, 'Error while erasing records for database (%s).', NULL, NULL, 'c_pg');
(22, 'sriram', '1993-08-13 14:38:31', 16, 118, 'Error is updating delete flag', NULL, NULL, 'c_pg');
(23, 'jclear', '1993-08-25 16:35:38', 1, 506, '', NULL, NULL, 'c_pg');
(24, 'daves', '1993-08-27 10:19:57', 1, 495, 'view', NULL, NULL, 'c_pg');
(25, 'daves', '1993-08-27 10:20:04', 1, 496, 'relation', NULL, NULL, 'c_pg');
(26, 'sriram', '1993-09-01 17:04:03', 16, 159, 'Mark database "%s" as disbaled for long term journaling.', NULL, NULL, 'c_pg');
(27, 'eds', '1993-09-10 14:52:50', 17, 9, 'Enter data or NULL for each column.  Return or EOF to end.', NULL, NULL, 'c_pg');
(28, 'jclear', '1993-09-14 16:30:26', 12, 218, 'gbak: warning level parameter missing', NULL, NULL, 'c_pg');
(29, 'jclear', '1993-09-14 16:46:37', 12, 217, '%sWARNING_LEVEL', NULL, NULL, 'c_pg');
(30, 'jclear', '1993-09-14 16:48:50', 12, 217, '%sW(WARNING_LEVEL)', NULL, NULL, 'c_pg');
(31, 'chander', '1993-09-17 14:19:35', 8, 80, 'Find field source failed', NULL, NULL, 'c_pg');
(32, 'jclear', '1993-09-19 13:20:05', 0, 301, 'procedure %s conflicts with a procedure in the same statement', NULL, NULL, 'c_pg');
(33, 'jclear', '1993-09-19 13:20:41', 0, 302, 'relation %s conflicts with a relation in the same statement', NULL, NULL, 'c_pg');
(34, 'deej', '1993-09-23 11:54:42', 0, 319, 'table referenced by plan not found in query', NULL, NULL, 'c_pg');
(35, 'sjlee', '1993-09-24 12:27:35', 1, 24, 'È¯¿µ QLI
ÁúÀÇ¾î ¹ø¿ª±â', NULL, NULL, 'ko_KO');
(36, 'sjlee', '1993-09-24 12:39:21', 1, 24, 'ÁúÀÇ¾î ¹ø¿ª±â
È¯¿µ QLI', NULL, NULL, 'ko_KO');
(85, 'sriram', '1993-10-01 09:25:08', 17, 1, 'isql [<database>] [-e] [-t <terminator>] [-i <inputfile>] [-o <outputfile]
	 [-x|-a] [-d <target db>]', NULL, NULL, 'c_pg');
(86, 'sriram', '1993-10-01 09:31:14', 17, 1, 'isql [<database>] [-e] [-t <terminator>] [-i <inputfile>] [-o <outputfile]
	 [-x|-a] [-d <target db>] [-p <password>]', NULL, NULL, 'c_pg');
(87, 'sriram', '1993-10-01 09:31:43', 17, 1, 'isql [<database>] [-e] [-t <terminator>] [-i <inputfile>] [-o <out', NULL, NULL, 'c_pg');
(88, 'sriram', '1993-10-01 09:40:46', 17, 11, '	[-x|-a] [-d <target db>] [-p <password>] [-u <user>]', NULL, NULL, 'c_pg');
(89, 'sriram', '1993-10-04 14:29:45', 16, 232, '', NULL, NULL, 'c_pg');
(90, 'jclear', '1993-10-04 15:53:57', 17, 12, 'Number of DB pages allocated = %d', NULL, NULL, 'c_pg');
(93, 'jclear', '1993-10-06 14:53:02', 17, 29, 'Set commands:', NULL, NULL, 'c_pg');
(94, 'eds', '1993-10-10 21:54:17', 17, 31, '	SET AUTOcommit  -- toggle autocommit of DDL statments', NULL, NULL, 'c_pg');
(95, 'eds', '1993-10-10 21:57:28', 17, 11, '	 [-x|-a] [-d <target db>] [-p <password>] [-u <user>]', NULL, NULL, 'c_pg');
(96, 'jdavid', '1993-10-12 16:08:57', 0, 329, 'Fdopen failed in pipe server.  Errno = %d', NULL, NULL, 'c_pg');
(97, 'sriram', '1993-10-13 17:24:54', 16, 236, 'Archive process unable to open source log file.', NULL, NULL, 'c_pg');
(98, 'sriram', '1993-10-14 09:07:39', 16, 239, '	restart archive for database', NULL, NULL, 'c_pg');
(99, 'sriram', '1993-10-14 09:23:42', 16, 239, '%s:	restart archive for database', NULL, NULL, 'c_pg');
(100, 'sriram', '1993-10-14 09:25:21', 16, 239, '	%s:	restart archive for database', NULL, NULL, 'c_pg');
(101, 'sriram', '1993-10-14 09:28:51', 16, 239, '	%s:restart archive for database', NULL, NULL, 'c_pg');
(102, 'sriram', '1993-10-14 14:07:07', 0, 334, 'error in attaching to password database', NULL, NULL, 'c_pg');
(103, 'jclear', '1993-10-14 17:52:57', 0, 200, 'onglay ermtay ournalingjay alreadyay enableday', NULL, NULL, 'pg_PG');
(104, 'jclear', '1993-10-18 09:46:58', 0, 331, 'wrong type page', NULL, NULL, 'c_pg');
(105, 'jclear', '1993-10-21 10:11:48', 0, 305, 'Token unknown - line %d, char %d', NULL, NULL, 'c_pg');
(106, 'jclear', '1993-10-25 15:41:12', 18, 19, 'gsec - add record error :q', NULL, NULL, 'c_pg');
(107, 'jclear', '1993-10-25 15:56:42', 18, 33, 'GsecMsg33', NULL, NULL, 'c_pg');
(108, 'deej', '1993-10-25 18:20:28', 0, 325, 'position is on a crack in location of deleted record', NULL, NULL, 'c_pg');
(109, 'jclear', '1993-10-26 10:49:38', 18, 26, '    useray amenay       uiday   idgay     rojectpay   organizationay       ullfay amenay', NULL, NULL, 'pg_PG');
(110, 'jclear', '1993-10-26 11:07:53', 18, 26, 'useray amenay       uiday   idgay     rojectpay   organizationay', NULL, NULL, 'pg_PG');
(111, 'jclear', '1993-10-26 11:09:04', 18, 26, 'useray amenay       uiday   idgay     rojectpay   organizationay ullfay emenay', NULL, NULL, 'pg_PG');
(112, 'katz', '1993-10-27 15:47:40', 0, 336, 'variable %s conflicts with parameter is same procedure', NULL, NULL, 'c_pg');
(113, 'sriram', '1993-10-29 15:33:52', 16, 199, 'Error in opening database file.', NULL, NULL, 'c_pg');
(114, 'sriram', '1993-10-29 15:34:35', 16, 232, 'Error in adding secondary file.', NULL, NULL, 'c_pg');
(115, 'jclear', '1993-11-01 09:53:46', 16, 215, 'Journal server console program', NULL, NULL, 'c_pg');
(116, 'deej', '1993-11-11 12:02:06', 0, 319, 'a table specified in the plan is not in the query', NULL, NULL, 'c_pg');
(117, 'deej', '1993-11-14 17:45:21', 0, 318, 'table %s is referenced twice in plan', NULL, NULL, 'c_pg');
(118, 'rvander', '1993-11-22 14:10:15', 15, 287, 'Illegal savepoint number', NULL, NULL, 'c_pg');
(119, 'daves', '1993-11-23 10:15:49', 0, 320, 'Cannot specify doamin name and collation clause', NULL, NULL, 'c_pg');
(120, 'daves', '1993-11-23 14:20:28', 0, 320, 'COLLATION %s not valid for specified CHARACTER SET', NULL, NULL, 'c_pg');
(121, 'daves', '1993-11-23 14:39:09', 0, 269, 'Character set or collation (or combination) unknown', NULL, NULL, 'c_pg');
(122, 'daves', '1993-11-23 14:40:16', 0, 189, 'character set %s not found', 'Define the character set name to the database, or reattach
without specifying a character set.', NULL, 'c_pg');
(123, 'daves', '1993-11-23 14:41:20', 0, 268, 'Character set unknown', NULL, NULL, 'c_pg');
(124, 'daves', '1993-12-14 16:45:57', 0, 320, 'COLLATE clause can only be applied to text string datatypes', NULL, NULL, 'c_pg');
(125, 'chander', '1994-01-04 15:22:02', 0, 344, '"duplicate specification of %s - not supported"', NULL, NULL, 'c_pg');
(126, 'jclear', '1994-01-07 14:25:48', 0, 311, 'Short integer expected', NULL, NULL, 'c_pg');
(127, 'jclear', '1994-01-07 14:32:54', 0, 27, 'validation error for field %s, value "%s"', NULL, NULL, 'c_pg');
(128, 'jclear', '1994-01-07 14:33:35', 0, 39, 'attempted update of read-only field', 'If the read-only field is in a system relation, change your
program.  If the field is a COMPUTED field, you have to
change the source fields to change its value.  If the field
takes part in a view, update it in its source relations.', 'Your program tried to change the value of a read-only
field in a system relation, a COMPUTED field, or a field
used in a view.', 'c_pg');
(129, 'jclear', '1994-01-07 14:34:23', 0, 76, 'field %s is not defined in relation %s', NULL, 'An undefined external function was referenced in blr.', 'c_pg');
(130, 'jclear', '1994-01-07 14:36:06', 0, 138, 'field not array or invalid dimensions (expected %ld, encountered %ld)', NULL, NULL, 'c_pg');
(131, 'jclear', '1994-01-07 14:36:41', 0, 211, 'Field used in a PRIMARY/UNIQUE constraint must be NOT NULL.', NULL, NULL, 'c_pg');
(132, 'jclear', '1994-01-07 14:38:05', 0, 223, 'Cannot delete field being used in an integrity constraint.', NULL, NULL, 'c_pg');
(133, 'jclear', '1994-01-07 14:38:18', 0, 224, 'Cannot rename field being used in an integrity constraint.', NULL, NULL, 'c_pg');
(134, 'jclear', '1994-01-07 14:39:02', 0, 232, 'could not find field for GRANT', NULL, NULL, 'c_pg');
(135, 'jclear', '1994-01-07 14:39:55', 0, 235, 'field has non-SQL security class defined', NULL, NULL, 'c_pg');
(136, 'jclear', '1994-01-07 14:40:39', 0, 258, 'Field unknown', NULL, NULL, 'c_pg');
(137, 'jclear', '1994-01-07 14:41:07', 0, 267, 'Field is not a blob', NULL, NULL, 'c_pg');
(138, 'jclear', '1994-01-07 14:41:45', 0, 278, 'must specify field name for view select expression', NULL, NULL, 'c_pg');
(139, 'jclear', '1994-01-07 14:42:18', 0, 279, 'number of fields does not match select list', NULL, NULL, 'c_pg');
(140, 'jclear', '1994-01-07 14:45:12', 0, 284, 'foreign key field count does not match primary key', NULL, NULL, 'c_pg');
(141, 'jclear', '1994-01-07 14:45:44', 0, 295, 'field used with aggregate', NULL, NULL, 'c_pg');
(142, 'jclear', '1994-01-07 14:46:12', 0, 296, 'invalid field reference', NULL, NULL, 'c_pg');
(143, 'jclear', '1994-01-07 14:49:30', 0, 321, 'Specified domain or source field does not exist', NULL, NULL, 'c_pg');
(144, 'jclear', '1994-01-07 14:56:04', 0, 26, 'corrupt system relation', NULL, NULL, 'c_pg');
(145, 'jclear', '1994-01-07 14:56:28', 0, 40, 'attempted update of read-only relation', 'If you want to write to the relation, reserve it for WRITE.', 'Your program tried to update a relation that it earlier
reserved for READ access.', 'c_pg');
(146, 'jclear', '1994-01-07 14:57:16', 0, 56, 'relation %s was omitted from the transaction reserving list', NULL, NULL, 'c_pg');
(147, 'jclear', '1994-01-07 14:57:51', 0, 75, 'relation %s is not defined', NULL, NULL, 'c_pg');
(148, 'jclear', '1994-01-07 14:59:22', 0, 151, 'there is no index in relation %s with id %d', NULL, NULL, 'c_pg');
(149, 'jclear', '1994-01-07 14:59:53', 0, 155, 'lock on relation %s conflicts with existing lock', NULL, NULL, 'c_pg');
(150, 'jclear', '1994-01-07 15:00:31', 0, 157, 'max indices per relation (%d) exceeded', NULL, NULL, 'c_pg');
(151, 'jclear', '1994-01-07 15:01:08', 0, 212, 'Name of Referential Constraint not defined in constraints relation.', NULL, NULL, 'c_pg');
(152, 'jclear', '1994-01-07 15:07:51', 0, 228, 'Attempt to define a second primary key for the same relation', NULL, NULL, 'c_pg');
(153, 'jclear', '1994-01-07 15:08:30', 0, 230, 'only the owner of a relation may reassign ownership', NULL, NULL, 'c_pg');
(154, 'jclear', '1994-01-07 16:27:13', 0, 231, 'could not find relation/procedure for GRANT', NULL, NULL, 'c_pg');
(155, 'jclear', '1994-01-11 12:02:09', 0, 278, 'must specify column field name for view select expression', NULL, NULL, 'c_pg');
(156, 'chander', '1994-01-11 15:18:50', 0, 146, 'violation of foreign key constraint for index "%s"', NULL, NULL, 'c_pg');
(157, 'jclear', '1994-01-12 13:04:06', 2, 116, 'expected relation name, encountered "%s"', NULL, NULL, 'c_pg');
(158, 'jclear', '1994-01-12 13:22:54', 1, 237, 'Relation %s already exists', NULL, NULL, 'c_pg');
(159, 'jclear', '1994-01-12 13:23:28', 2, 300, 'relation %s already exists', NULL, NULL, 'c_pg');
(160, 'jclear', '1994-01-12 13:23:51', 2, 49, 'relation %s already exists', NULL, NULL, 'c_pg');
(161, 'jclear', '1994-01-12 13:24:16', 2, 55, 'relation %s already exists', NULL, NULL, 'c_pg');
(162, 'jclear', '1994-01-12 13:24:34', 2, 137, 'relation %s already exists', NULL, NULL, 'c_pg');
(163, 'jclear', '1994-01-12 13:30:49', 8, 132, 'Relation %s already exists', NULL, NULL, 'c_pg');
(164, 'jclear', '1994-01-12 13:41:51', 8, 55, 'Field not found for relation', NULL, NULL, 'c_pg');
(165, 'jclear', '1994-01-12 15:41:59', 0, 260, 'Relation unknown', NULL, NULL, 'c_pg');
(166, 'jclear', '1994-01-12 15:58:53', 0, 302, 'alias %s conflicts with a relation in the same statement', NULL, NULL, 'c_pg');
(167, 'jclear', '1994-01-12 15:59:35', 0, 315, 'there is no alias or relation named %s at this scope level', NULL, NULL, 'c_pg');
(168, 'jclear', '1994-01-12 15:59:55', 0, 316, 'there is no index %s in relation %s', NULL, NULL, 'c_pg');
(169, 'jclear', '1994-01-12 16:00:24', 0, 323, 'the relation %s is referenced twice; use aliases to differentiate', NULL, NULL, 'c_pg');
(170, 'jclear', '1994-01-12 16:00:58', 0, 340, 'view %s has more than one base relation; use aliases to distinguish', NULL, NULL, 'c_pg');
(171, 'jclear', '1994-01-12 16:14:03', 0, 234, 'relation/procedure has non-SQL security class defined', NULL, NULL, 'c_pg');
(172, 'jclear', '1994-01-12 16:25:46', 1, 170, 'relation name', NULL, NULL, 'c_pg');
(173, 'jclear', '1994-01-12 16:25:46', 1, 196, 'relation name', NULL, NULL, 'c_pg');
(174, 'jclear', '1994-01-12 16:25:46', 1, 209, 'relation name', NULL, NULL, 'c_pg');
(175, 'jclear', '1994-01-12 16:25:46', 1, 216, 'relation name', NULL, NULL, 'c_pg');
(176, 'jclear', '1994-01-12 16:25:47', 1, 218, 'relation name', NULL, NULL, 'c_pg');
(177, 'jclear', '1994-01-12 16:25:47', 1, 220, 'relation name', NULL, NULL, 'c_pg');
(178, 'jclear', '1994-01-12 16:25:47', 1, 222, 'relation name', NULL, NULL, 'c_pg');
(179, 'jclear', '1994-01-12 16:25:47', 1, 223, 'relation name', NULL, NULL, 'c_pg');
(180, 'jclear', '1994-01-12 16:25:47', 1, 399, 'relation name', NULL, NULL, 'c_pg');
(181, 'jclear', '1994-01-12 16:29:09', 1, 75, 'Could not create QLI$PROCEDURES relation', NULL, NULL, 'c_pg');
(182, 'jclear', '1994-01-12 16:29:40', 1, 78, 'QLI$PROCEDURES relation must be created with RDO in Rdb/VMS databases', NULL, NULL, 'c_pg');
(183, 'jclear', '1994-01-12 16:30:07', 1, 129, 'No triggers are defined for relation %s', NULL, NULL, 'c_pg');
(184, 'jclear', '1994-01-12 16:30:39', 1, 166, '%s is not a relation in database %s', NULL, NULL, 'c_pg');
(185, 'jclear', '1994-01-12 16:31:19', 1, 173, 'relation or view name', NULL, NULL, 'c_pg');
(186, 'jclear', '1994-01-12 16:33:07', 1, 202, 'period in qualified relation name', NULL, NULL, 'c_pg');
(187, 'jclear', '1994-01-12 16:33:34', 1, 203, '%s is not a relation in database %s', NULL, NULL, 'c_pg');
(188, 'jclear', '1994-01-12 16:36:46', 1, 236, 'Field %s does not occur in relation %s', NULL, NULL, 'c_pg');
(189, 'jclear', '1994-01-12 17:53:52', 2, 43, 'relation %s doesn''t exist', NULL, NULL, 'c_pg');
(190, 'jclear', '1994-01-12 17:54:13', 2, 54, 'relation %s doesn''t exist', NULL, NULL, 'c_pg');
(191, 'jclear', '1994-01-12 17:54:25', 2, 69, 'relation %s doesn''t exist', NULL, NULL, 'c_pg');
(192, 'jclear', '1994-01-12 17:54:39', 2, 87, 'relation %s doesn''t exist', NULL, NULL, 'c_pg');
(193, 'jclear', '1994-01-12 18:00:27', 15, 187, 'cannot delete system relations', NULL, NULL, 'c_pg');
(194, 'jclear', '1994-01-13 15:07:00', 8, 132, 'table %s already exists', NULL, NULL, 'c_pg');
(195, 'jclear', '1994-01-13 15:19:45', 1, 208, 'expected "relation_name", encountered "%s"', NULL, NULL, 'c_pg');
(196, 'jclear', '1994-01-13 15:25:26', 1, 179, 'field definition clause', NULL, NULL, 'c_pg');
(197, 'chander', '1994-01-14 12:05:21', 0, 238, 'Operation violates CHECK constraint on view or table', NULL, NULL, 'c_pg');
(198, 'manish', '1994-01-17 11:31:27', 0, 346, 'Server version too old to support all CREATE DATABASE options', NULL, NULL, 'c_pg');
(199, 'manish', '1994-01-17 11:33:15', 0, 347, 'Drop database completed with errors', NULL, NULL, 'c_pg');
(200, 'rvander', '1994-01-19 09:32:54', 0, 29, 'attempt to store duplicate value in unique index "%s"', NULL, NULL, 'c_pg');
(201, 'jclear', '1994-01-19 16:26:40', 0, 168, 'logfile header of %s too small', NULL, NULL, 'c_pg');
(202, 'jclear', '1994-01-19 16:27:28', 0, 169, 'invalid version of logfile %s', NULL, NULL, 'c_pg');
(203, 'jclear', '1994-01-19 16:28:00', 0, 170, 'logfile %s not latest in the chain but open flag still set', NULL, NULL, 'c_pg');
(204, 'jclear', '1994-01-19 16:29:14', 0, 171, 'logfile %s not closed properly; database recovery may be required', NULL, NULL, 'c_pg');
(205, 'jclear', '1994-01-19 16:30:00', 0, 172, 'database name in the logfile %s is different', NULL, NULL, 'c_pg');
(206, 'jclear', '1994-01-19 16:30:50', 0, 174, 'incomplete log record at offset %ld in logfile %s', NULL, NULL, 'c_pg');
(207, 'jclear', '1994-01-19 16:31:21', 0, 175, 'log record header too small at offset %ld in logfile %s', NULL, NULL, 'c_pg');
(208, 'jclear', '1994-01-19 16:31:52', 0, 176, 'log block too small at offset %ld in logfile %s', NULL, NULL, 'c_pg');
(209, 'jclear', '1994-01-19 16:33:51', 0, 179, 'can''t rollover to the next logfile %s', NULL, NULL, 'c_pg');
(210, 'jclear', '1994-01-20 16:33:57', 0, 309, 'Precision should be greater than 0', NULL, NULL, 'c_pg');
(211, 'jclear', '1994-01-20 16:37:08', 0, 310, 'Scale can not be greater than precision', NULL, NULL, 'c_pg');
(212, 'jclear', '1994-01-20 16:40:38', 0, 290, 'Log partition size too small', NULL, NULL, 'c_pg');
(213, 'jclear', '1994-01-20 16:42:21', 0, 291, 'Log size too small', NULL, NULL, 'c_pg');
(214, 'jclear', '1994-01-20 16:47:56', 0, 289, 'Total length of a partitioned log must be specified', NULL, NULL, 'c_pg');
(215, 'jclear', '1994-01-20 16:49:59', 0, 306, 'Log redefined', NULL, NULL, 'c_pg');
(216, 'jclear', '1994-01-20 16:50:38', 0, 307, 'Partitions not supported in series of log file specification', NULL, NULL, 'c_pg');
(217, 'jclear', '1994-01-20 16:52:55', 0, 308, 'Cache redefined', NULL, NULL, 'c_pg');
(218, 'jclear', '1994-01-20 16:54:32', 0, 304, 'Long integer expected', NULL, NULL, 'c_pg');
(219, 'jclear', '1994-01-20 16:55:22', 0, 311, 'SMALLINT expected', NULL, NULL, 'c_pg');
(220, 'jclear', '1994-01-21 11:37:54', 8, 17, 'Primary Key field lookup failed', NULL, NULL, 'c_pg');
(221, 'jclear', '1994-01-21 11:38:21', 8, 18, 'could not find unique index with specified fields', NULL, NULL, 'c_pg');
(222, 'jclear', '1994-01-21 11:41:46', 8, 43, 'field %s is used in relation %s (local name %s) and can not be dropped', NULL, NULL, 'c_pg');
(223, 'jclear', '1994-01-21 11:42:05', 8, 46, 'Field not found', NULL, NULL, 'c_pg');
(224, 'jclear', '1994-01-21 11:42:42', 8, 52, 'field %s from relation %s is referenced in view %s', NULL, NULL, 'c_pg');
(225, 'jclear', '1994-01-21 11:44:04', 8, 14, 'No relation specified for index', NULL, NULL, 'c_pg');
(226, 'jclear', '1994-01-21 11:44:23', 8, 20, 'could not find primary key index in specified relation', NULL, NULL, 'c_pg');
(227, 'jclear', '1994-01-21 11:44:55', 8, 51, 'No relation specified in ERASE RFR', NULL, NULL, 'c_pg');
(228, 'jclear', '1994-01-21 11:45:17', 8, 61, 'Relation not found', NULL, NULL, 'c_pg');
(229, 'jclear', '1994-01-21 11:45:55', 8, 80, 'Specified domain or source field does not exist', NULL, NULL, 'c_pg');
(230, 'jclear', '1994-01-21 11:47:12', 8, 89, 'Global field not found', NULL, NULL, 'c_pg');
(231, 'jclear', '1994-01-21 11:47:26', 8, 93, 'Index field not found', NULL, NULL, 'c_pg');
(232, 'jclear', '1994-01-21 11:52:29', 8, 96, 'Local field not found', NULL, NULL, 'c_pg');
(233, 'jclear', '1994-01-21 11:52:42', 8, 101, 'Relation field not found', NULL, NULL, 'c_pg');
(234, 'jclear', '1994-01-21 11:53:14', 8, 116, 'attempt to index blob field in index %s', NULL, NULL, 'c_pg');
(235, 'jclear', '1994-01-21 11:53:32', 8, 117, 'attempt to index array field in index %s', NULL, NULL, 'c_pg');
(236, 'jclear', '1994-01-21 11:53:54', 8, 120, 'Unknown fields in index %s', NULL, NULL, 'c_pg');
(237, 'jclear', '1994-01-21 11:54:22', 8, 123, 'Field: %s not defined as NOT NULL - can''t be used in PRIMARY KEY/UNIQUE constraint definition', NULL, NULL, 'c_pg');
(238, 'jclear', '1994-01-21 11:55:05', 8, 128, 'No relation specified in delete_constraint', NULL, NULL, 'c_pg');
(239, 'jclear', '1994-01-21 11:56:03', 8, 137, 'Store into system relation %s failed.', NULL, NULL, 'c_pg');
(240, 'jclear', '1994-01-21 11:56:29', 8, 159, 'Name longer than database field size', NULL, NULL, 'c_pg');
(241, 'jclear', '1994-01-21 11:57:00', 8, 162, 'Looking up field position failed', NULL, NULL, 'c_pg');
(242, 'christy', '1994-01-24 10:44:58', 2, 7, '
%d errors during input.', NULL, NULL, 'c_pg');
(243, 'christy', '1994-01-24 10:45:18', 2, 15, '%s:%d:', NULL, NULL, 'c_pg');
(244, 'christy', '1994-01-24 10:45:50', 2, 46, 'combined key length (%d) for index %s is > 254 bytes', NULL, NULL, 'c_pg');
(245, 'christy', '1994-01-24 10:46:15', 2, 74, 'Trigger message number %d for trigger %s doesn''t exist', NULL, NULL, 'c_pg');
(246, 'christy', '1994-01-24 10:46:55', 2, 142, 'message number %d exceeds 255', NULL, NULL, 'c_pg');
(247, 'christy', '1994-01-24 10:47:47', 2, 178, 'message number %d exceeds 255', NULL, NULL, 'c_pg');
(248, 'christy', '1994-01-24 10:48:18', 2, 210, 'PAGE_SIZE specified (%d) longer than limit of %d bytes', NULL, NULL, 'c_pg');
(249, 'christy', '1994-01-24 10:52:26', 12, 180, 'index %s omitted because %d of the expected %d keys were found', NULL, NULL, 'c_pg');
(250, 'christy', '1994-01-24 10:52:46', 12, 180, 'index %s omitted because %ld of the expected %ld keys were found', NULL, NULL, 'c_pg');
(251, 'christy', '1994-01-24 11:14:40', 12, 180, 'index %s omitted because %d of the expected %d keys were found', NULL, NULL, 'c_pg');
(252, 'christy', '1994-01-24 11:22:54', 0, 148, 'transaction %d is %s', NULL, NULL, 'c_pg');
(253, 'christy', '1994-01-24 11:32:47', 1, 482, 'field width (%d) * header segments (%d) greater than 60,000 characters', NULL, NULL, 'c_pg');
(254, 'christy', '1994-01-24 11:33:40', 2, 312, 'key length (%d) for compound index %s exceeds 202', NULL, NULL, 'c_pg');
(255, 'christy', '1994-01-24 11:35:34', 16, 211, 'Windows NT error %d', NULL, NULL, 'c_pg');
(256, 'christy', '1994-01-24 11:36:28', 16, 25, 'b-tree add node. before offset %d', NULL, NULL, 'c_pg');
(257, 'christy', '1994-01-24 11:37:11', 16, 26, 'b-tree add node. offset: %d  length: %d', NULL, NULL, 'c_pg');
(258, 'christy', '1994-01-24 11:37:47', 16, 27, 'valid b-tree btr_length: %d', NULL, NULL, 'c_pg');
(259, 'christy', '1994-01-24 11:38:07', 16, 28, 'b-tree add segment before length: %d', NULL, NULL, 'c_pg');
(260, 'christy', '1994-01-24 11:38:24', 16, 29, 'b-tree add segment. expected after length %d', NULL, NULL, 'c_pg');
(261, 'christy', '1994-01-24 11:38:44', 16, 30, 'b-tree drop node. offset %d', NULL, NULL, 'c_pg');
(262, 'christy', '1994-01-24 11:40:19', 16, 47, 'unknown log record type: %d', NULL, NULL, 'c_pg');
(263, 'christy', '1994-01-24 11:40:59', 16, 55, 'wrong clump type for data page (%d)', NULL, NULL, 'c_pg');
(264, 'christy', '1994-01-24 11:41:26', 16, 107, 'Archiving log file. id: %d	name: %s', NULL, NULL, 'c_pg');
(265, 'christy', '1994-01-24 11:41:53', 16, 117, 'Control Point. Seqno: %d	 Offset: %d', NULL, NULL, 'c_pg');
(266, 'christy', '1994-01-24 11:42:17', 16, 120, 'Online Dump file. dump id: %d.	name: (%s)', NULL, NULL, 'c_pg');
(267, 'christy', '1994-01-24 11:42:41', 16, 122, 'End Online Dump. dump id: %d', NULL, NULL, 'c_pg');
(268, 'christy', '1994-01-24 11:42:59', 16, 123, 'Don''t recognize journal message %d', NULL, NULL, 'c_pg');
(269, 'christy', '1994-01-24 11:43:14', 16, 125, 'Start Online Dump. dump_id: %d', NULL, NULL, 'c_pg');
(270, 'christy', '1994-01-24 11:43:38', 16, 132, '	%s (db id = %d), connections: %d', NULL, NULL, 'c_pg');
(271, 'christy', '1994-01-24 11:43:59', 16, 137, '	Resetting database "%s" (%d)', NULL, NULL, 'c_pg');
(272, 'christy', '1994-01-24 11:44:26', 16, 161, 'Database (id = %d) is currently in use.', NULL, NULL, 'c_pg');
(273, 'christy', '1994-01-24 11:44:49', 16, 162, 'Drop records for database (id = %d).', NULL, NULL, 'c_pg');
(274, 'christy', '1994-01-24 11:45:08', 16, 173, 'Dos error %d', NULL, NULL, 'c_pg');
(275, 'christy', '1994-01-24 11:45:25', 16, 175, 'Don''t understand mailbox message type %d', NULL, NULL, 'c_pg');
(276, 'christy', '1994-01-24 11:45:52', 16, 177, 'Database (id = %d) not enabled with this journal server.', NULL, NULL, 'c_pg');
(277, 'christy', '1994-01-24 11:46:16', 16, 178, 'Log archive complete.  File "%s" (id = %d).', NULL, NULL, 'c_pg');
(278, 'christy', '1994-01-24 11:46:37', 16, 179, 'Error in archiving file. (id = %d).', NULL, NULL, 'c_pg');
(279, 'christy', '1994-01-24 11:46:57', 16, 180, 'Secondary File: %s.  Sequence: %d.', NULL, NULL, 'c_pg');
(280, 'christy', '1994-01-24 11:49:07', 2, 336, 'Minimum log length should be %d Kbytes', NULL, NULL, 'c_pg');
(281, 'christy', '1994-01-24 11:49:35', 2, 339, 'minimum of %d cache pages required', NULL, NULL, 'c_pg');
(282, 'christy', '1994-01-24 11:50:16', 12, 202, 'adjusting an invalid decompression length from %d to %d', NULL, NULL, 'c_pg');
(283, 'christy', '1994-01-24 11:50:35', 12, 203, 'skipped %d bytes after reading a bad attribute %d', NULL, NULL, 'c_pg');
(284, 'christy', '1994-01-24 11:51:01', 12, 205, 'skipped %d bytes looking for next valid attribute, encountered attribute %d', NULL, NULL, 'c_pg');
(285, 'christy', '1994-01-24 11:54:10', 1, 504, 'unknown datatype %d', NULL, NULL, 'c_pg');
(286, 'christy', '1994-01-24 12:00:36', 17, 12, 'Number of DB pages allocated = %d', NULL, NULL, 'c_pg');
(287, 'christy', '1994-01-24 12:01:11', 17, 13, 'Sweep interval = %d', NULL, NULL, 'c_pg');
(288, 'christy', '1994-01-24 12:01:41', 17, 14, 'Number of wal buffers = %d', NULL, NULL, 'c_pg');
(289, 'christy', '1994-01-24 12:01:57', 17, 15, 'Wal buffer size = %d', NULL, NULL, 'c_pg');
(290, 'christy', '1994-01-24 12:02:15', 17, 16, 'Check point length = %d', NULL, NULL, 'c_pg');
(291, 'christy', '1994-01-24 12:02:29', 17, 17, 'Check point interval = %d', NULL, NULL, 'c_pg');
(292, 'christy', '1994-01-24 12:02:50', 17, 18, 'Wal group commit wait = %d', NULL, NULL, 'c_pg');
(293, 'christy', '1994-01-24 12:03:10', 17, 19, 'Base level = %d', NULL, NULL, 'c_pg');
(294, 'christy', '1994-01-24 12:03:48', 17, 20, 'Transaction in limbo = %d', NULL, NULL, 'c_pg');
(295, 'christy', '1994-01-24 12:06:39', 17, 46, 'Blob display set to subtype %d. This blob: subtype = %d', NULL, NULL, 'c_pg');
(296, 'christy', '1994-01-24 12:13:50', 0, 314, 'Token unknown - line %d, char %d', NULL, NULL, 'c_pg');
(297, 'jclear', '1994-01-27 12:05:22', 19, 9, '    between 9 am and 8 pm, Monday through Friday, Eastern time.', NULL, NULL, 'c_pg');
(298, 'jclear', '1994-01-28 10:41:05', 19, 2, '    Your node is not registered to run InterBase.    %s', NULL, NULL, 'c_pg');
(299, 'jclear', '1994-01-28 10:43:07', 19, 2, '    Your node is not registered to run InterBase.\n    %s\n', NULL, NULL, 'c_pg');
(300, 'jclear', '1994-01-28 10:49:39', 19, 2, '    Your node is not registered to run InterBase.\\n    %s\\n', NULL, NULL, 'c_pg');
(301, 'jclear', '1994-01-28 11:02:53', 19, 2, '    Your node is not registered to run InterBase.\n    %s\n', NULL, NULL, 'c_pg');
(302, 'jclear', '1994-01-31 15:53:26', 0, 142, 'secondary server attachments can not start journalling', NULL, NULL, 'c_pg');
(303, 'jclear', '1994-01-31 15:57:05', 0, 157, 'max indices per table (%d) exceeded', NULL, NULL, 'c_pg');
(304, 'jclear', '1994-01-31 15:58:34', 0, 177, 'Illegal attempt to attach to an unintialized WAL segment for %s', NULL, NULL, 'c_pg');
(305, 'jclear', '1994-01-31 15:59:53', 0, 190, 'lock timeout on wait transaction', NULL, NULL, 'c_pg');
(306, 'jclear', '1994-01-31 16:00:55', 0, 206, 'WAL writer synhronization error for the database %s', NULL, NULL, 'c_pg');
(307, 'jclear', '1994-01-31 16:02:05', 0, 213, 'Non-existent Primary or Unique key specifed for Foreign Key.', NULL, NULL, 'c_pg');
(308, 'jclear', '1994-01-31 16:03:24', 0, 320, 'CHARACTER SET & COLLATE apply only to text datatypes', NULL, NULL, 'c_pg');
(309, 'jclear', '1994-02-04 17:57:53', 0, 4, 'invalid database handle (missing READY?)', NULL, NULL, 'c_pg');
(310, 'jclear', '1994-02-07 09:56:25', 0, 4, 'invalid database handle (missing CONNECT?)', NULL, NULL, 'c_pg');
(311, 'jclear', '1994-02-07 13:36:27', 15, 239, 'Interbase status vector inconsistent', NULL, NULL, 'c_pg');
(312, 'jclear', '1994-02-07 13:37:01', 15, 240, 'Interbase/RdB message parameter inconsistency', NULL, NULL, 'c_pg');
(313, 'jclear', '1994-02-07 13:38:13', 0, 201, 'Unable to rollover.  Please see Interbase log.', NULL, NULL, 'c_pg');
(314, 'jclear', '1994-02-07 13:38:36', 0, 202, 'WAL I/O error.  Please see Interbase log.', NULL, NULL, 'c_pg');
(315, 'jclear', '1994-02-07 13:39:12', 0, 203, 'WAL writer - Journal server communication error.  Please see Interbase log.', NULL, NULL, 'c_pg');
(316, 'jclear', '1994-02-07 13:39:31', 0, 204, 'WAL buffers can''t be increased.  Please see Interbase log.', NULL, NULL, 'c_pg');
(317, 'jclear', '1994-02-07 13:39:56', 0, 205, 'WAL setup error.  Please see Interbase log.', NULL, NULL, 'c_pg');
(318, 'jclear', '1994-02-07 13:59:30', 1, 67, 'can''t open command file "%s"', NULL, NULL, 'c_pg');
(319, 'jclear', '1994-02-07 14:03:08', 1, 141, '%s.* can not be used when a single element is required', NULL, NULL, 'c_pg');
(320, 'jclear', '1994-02-07 14:03:29', 1, 141, '%s.* be used when a single element is required', NULL, NULL, 'c_pg');
(321, 'jclear', '1994-02-07 14:25:40', 1, 155, 'field referenced in BASED ON can not be resolved against readied databases', NULL, NULL, 'c_pg');
(322, 'jclear', '1994-02-07 14:26:09', 1, 234, 'Can''t define an index in a view', NULL, NULL, 'c_pg');
(323, 'jclear', '1994-02-07 14:26:34', 1, 252, 'datatype can not be changed locally', NULL, NULL, 'c_pg');
(324, 'jclear', '1994-02-07 14:30:36', 1, 252, 'datatype cannot be changed locally', NULL, NULL, 'c_pg');
(325, 'jclear', '1994-02-07 14:49:19', 1, 411, 'Can not convert "%s" to a numeric value', NULL, NULL, 'c_pg');
(326, 'jclear', '1994-02-07 14:50:35', 0, 122, 'database system can''t read argument %ld', NULL, NULL, 'c_pg');
(327, 'jclear', '1994-02-07 14:51:14', 0, 123, 'database system can''t write argument %ld', NULL, NULL, 'c_pg');
(328, 'jclear', '1994-02-07 14:51:39', 0, 141, 'secondary server attachments can not validate databases', NULL, NULL, 'c_pg');
(329, 'jclear', '1994-02-07 14:52:18', 0, 142, 'secondary server attachments can not start journaling', NULL, NULL, 'c_pg');
(330, 'jclear', '1994-02-07 14:53:44', 2, 20, 'Database "%s" exists but can''t be opened', NULL, NULL, 'c_pg');
(331, 'jclear', '1994-02-07 14:54:39', 2, 42, '%s is a view and can not be indexed', NULL, NULL, 'c_pg');
(332, 'jclear', '1994-02-07 14:55:13', 2, 45, 'index %s: field %s in %s is computed and can not be a key', NULL, NULL, 'c_pg');
(333, 'jclear', '1994-02-07 14:55:42', 2, 5, 'gdef: can''t open %s', NULL, NULL, 'c_pg');
(334, 'jclear', '1994-02-07 14:56:08', 2, 6, 'gdef: can''t open %s or %s', NULL, NULL, 'c_pg');
(335, 'jclear', '1994-02-07 14:56:45', 2, 64, 'field %s is used in relation %s (local name %s) and can not be dropped', NULL, NULL, 'c_pg');
(336, 'jclear', '1994-02-07 14:57:08', 2, 68, 'can''t drop system relation %s', NULL, NULL, 'c_pg');
(337, 'jclear', '1994-02-07 14:57:33', 2, 97, 'object can not be resolved', NULL, NULL, 'c_pg');
(338, 'jclear', '1994-02-07 14:58:02', 2, 101, 'field %s can''t be resolved', NULL, NULL, 'c_pg');
(339, 'jclear', '1994-02-07 14:58:32', 2, 102, 'field %s isn''t defined in relation %s', NULL, NULL, 'c_pg');
(340, 'jclear', '1994-02-07 15:03:37', 2, 109, 'Can''t resolve field "%s"', NULL, NULL, 'c_pg');
(341, 'jclear', '1994-02-07 15:04:12', 2, 122, ' PAGE_SIZE can not be modified', NULL, NULL, 'c_pg');
(342, 'jclear', '1994-02-07 15:04:47', 2, 169, 'A computed expression can not be changed or added', NULL, NULL, 'c_pg');
(343, 'jclear', '1994-02-07 15:13:12', 2, 173, 'A computed expression can not be changed or added', NULL, NULL, 'c_pg');
(344, 'jclear', '1994-02-07 15:13:39', 2, 181, 'A computed expression can not be changed or added', NULL, NULL, 'c_pg');
(345, 'jclear', '1994-02-07 15:14:28', 2, 248, 'ddl: can''t open %s', NULL, NULL, 'c_pg');
(346, 'jclear', '1994-02-07 15:14:54', 2, 252, '**** field %s can not be extracted, computed source missing ***', NULL, NULL, 'c_pg');
(347, 'jclear', '1994-02-07 15:15:33', 2, 281, 'gdef: can''t open DYN output file: %s', NULL, NULL, 'c_pg');
(348, 'jclear', '1994-02-07 15:16:33', 12, 16, 'can''t create APOLLO tape descriptor file %s', NULL, NULL, 'c_pg');
(349, 'jclear', '1994-02-07 15:16:53', 12, 17, 'can''t set APOLLO tape descriptor attribute for %s', NULL, NULL, 'c_pg');
(350, 'jclear', '1994-02-07 15:17:28', 12, 19, 'can''t close APOLLO tape descriptor file %s', NULL, NULL, 'c_pg');
(351, 'jclear', '1994-02-07 15:17:44', 12, 35, 'can''t find relation %s', NULL, NULL, 'c_pg');
(352, 'jclear', '1994-02-07 15:18:10', 12, 36, 'Can''t find field for blob', NULL, NULL, 'c_pg');
(353, 'jclear', '1994-02-07 15:18:28', 12, 65, 'can''t open backup file %s', NULL, NULL, 'c_pg');
(354, 'jclear', '1994-02-07 15:18:43', 12, 66, 'can''t open status and error output file %s', NULL, NULL, 'c_pg');
(355, 'jclear', '1994-02-07 15:20:00', 2, 295, 'Functions can''t return arrays.', NULL, NULL, 'c_pg');
(356, 'jclear', '1994-02-07 15:20:28', 2, 301, 'Array indexes and size can not be modified', NULL, NULL, 'c_pg');
(357, 'jclear', '1994-02-07 15:21:07', 2, 147, 'datatype cstring not supported for fields', NULL, NULL, 'c_pg');
(358, 'jclear', '1994-02-07 15:21:55', 2, 168, 'datatype cstring not supported for fields', NULL, NULL, 'c_pg');
(359, 'jclear', '1994-02-07 15:22:11', 2, 191, 'datatype cstring not supported for fields', NULL, NULL, 'c_pg');
(360, 'jclear', '1994-02-07 15:22:44', 2, 227, 'computed fields need datatypes', NULL, NULL, 'c_pg');
(361, 'jclear', '1994-02-07 15:24:48', 2, 302, 'Modify datatype of array %s requires complete field specification', NULL, NULL, 'c_pg');
(362, 'jclear', '1994-02-07 15:45:16', 2, 21, 'Couldn''t create database "%s"', NULL, NULL, 'c_pg');
(363, 'jclear', '1994-02-07 15:45:37', 2, 25, 'Couldn''t locate database', NULL, NULL, 'c_pg');
(364, 'jclear', '1994-02-07 15:46:00', 2, 27, 'Couldn''t release database', NULL, NULL, 'c_pg');
(365, 'jclear', '1994-02-07 15:46:31', 2, 29, 'Couldn''t attach database "%s"', NULL, NULL, 'c_pg');
(366, 'jclear', '1994-02-07 15:46:51', 2, 43, 'table %s doesn''t exist', NULL, NULL, 'c_pg');
(367, 'jclear', '1994-02-07 15:47:02', 2, 54, 'table %s doesn''t exist', NULL, NULL, 'c_pg');
(368, 'jclear', '1994-02-07 15:48:42', 2, 62, 'filter %s doesn''t exist', NULL, NULL, 'c_pg');
(369, 'jclear', '1994-02-07 15:52:09', 2, 57, 'field %s doesn''t exist in relation %s as referenced in view field %s', NULL, NULL, 'c_pg');
(370, 'jclear', '1994-02-07 15:53:16', 2, 61, 'field %s doesn''t exist in relation %s', NULL, NULL, 'c_pg');
(371, 'jclear', '1994-02-07 15:54:17', 2, 65, 'field %s doesn''t exist', NULL, NULL, 'c_pg');
(372, 'jclear', '1994-02-07 15:54:54', 2, 63, 'function %s doesn''t exist', NULL, NULL, 'c_pg');
(373, 'jclear', '1994-02-07 15:55:36', 2, 66, 'index %s doesn''t exist', NULL, NULL, 'c_pg');
(374, 'jclear', '1994-02-07 15:55:51', 2, 69, 'table %s doesn''t exist', NULL, NULL, 'c_pg');
(375, 'jclear', '1994-02-07 15:56:05', 2, 70, 'security class %s doesn''t exist', NULL, NULL, 'c_pg');
(376, 'jclear', '1994-02-07 15:56:24', 2, 71, 'shadow %ld doesn''t exist', NULL, NULL, 'c_pg');
(377, 'jclear', '1994-02-07 15:56:48', 2, 73, 'Trigger %s doesn''t exist', NULL, NULL, 'c_pg');
(378, 'jclear', '1994-02-07 15:58:43', 2, 75, 'Type %s for field %s doesn''t exist', NULL, NULL, 'c_pg');
(379, 'jclear', '1994-02-07 15:59:07', 2, 76, 'User privilege %s on field %s in relation %s
for user %s doesn''t exist', NULL, NULL, 'c_pg');
(380, 'jclear', '1994-02-07 15:59:34', 2, 77, 'User privilege %s on relation %s for user %s doesn''t exist', NULL, NULL, 'c_pg');
(381, 'jclear', '1994-02-07 16:00:04', 2, 81, '(EXE) make_desc: don''t understand node type', NULL, NULL, 'c_pg');
(382, 'jclear', '1994-02-07 16:00:30', 2, 82, 'field %s doesn''t exist', NULL, NULL, 'c_pg');
(383, 'jclear', '1994-02-07 16:02:02', 2, 84, 'field %s doesn''t exist', NULL, NULL, 'c_pg');
(384, 'jclear', '1994-02-07 16:02:24', 2, 87, 'table %s doesn''t exist', NULL, NULL, 'c_pg');
(385, 'jclear', '1994-02-07 16:02:43', 2, 89, 'Trigger %s doesn''t exist', NULL, NULL, 'c_pg');
(386, 'jclear', '1994-02-07 16:03:00', 2, 90, 'Type %s for field %s doesn''t exist', NULL, NULL, 'c_pg');
(387, 'jclear', '1994-02-07 17:00:42', 2, 99, 'field %s doesn''t exist in relation %s', NULL, NULL, 'c_pg');
(388, 'jclear', '1994-02-07 17:01:14', 2, 103, 'global field %s isn''t defined', NULL, NULL, 'c_pg');
(389, 'jclear', '1994-02-07 17:01:37', 2, 104, 'relation %s isn''t defined', NULL, NULL, 'c_pg');
(390, 'jclear', '1994-02-07 17:03:05', 2, 100, 'field %s doesn''t exist', NULL, NULL, 'c_pg');
(391, 'jclear', '1994-02-07 17:04:01', 2, 105, 'trigger %s isn''t defined', NULL, NULL, 'c_pg');
(392, 'jclear', '1994-02-07 17:05:36', 2, 230, 'global field %s isn''t defined', NULL, NULL, 'c_pg');
(393, 'jclear', '1994-02-07 17:05:51', 2, 276, 'couldn''t open scratch file', NULL, NULL, 'c_pg');
(394, 'jclear', '1994-02-07 17:09:11', 0, 144, 'secondary server attachments cant start logging', NULL, NULL, 'c_pg');
(395, 'jclear', '1994-02-07 17:09:29', 0, 353, 'can not delete', NULL, NULL, 'c_pg');
(396, 'jclear', '1994-02-07 17:09:55', 0, 179, 'Can''t rollover to the next logfile %s', NULL, NULL, 'c_pg');
(397, 'jclear', '1994-02-07 17:10:25', 0, 204, 'WAL buffers can''t be increased.  Please see InterBase log.', NULL, NULL, 'c_pg');
(398, 'jclear', '1994-02-07 17:10:45', 0, 214, 'Can''t update constraints (RDB$REF_CONSTRAINTS).', NULL, NULL, 'c_pg');
(399, 'jclear', '1994-02-07 17:11:03', 0, 215, 'Can''t update constraints (RDB$CHECK_CONSTRAINTS).', NULL, NULL, 'c_pg');
(400, 'jclear', '1994-02-07 17:11:22', 0, 216, 'Can''t delete CHECK constraint entry (RDB$CHECK_CONSTRAINTS)', NULL, NULL, 'c_pg');
(401, 'jclear', '1994-02-07 17:11:40', 0, 217, 'Can''t delete index segment used by an Integrity Constraint', NULL, NULL, 'c_pg');
(402, 'jclear', '1994-02-07 17:11:58', 0, 218, 'Can''t update index segment used by an Integrity Constraint', NULL, NULL, 'c_pg');
(403, 'jclear', '1994-02-07 17:12:17', 0, 219, 'Can''t delete index used by an Integrity Constraint', NULL, NULL, 'c_pg');
(404, 'jclear', '1994-02-07 17:12:34', 0, 220, 'Can''t modify index used by an Integrity Constraint', NULL, NULL, 'c_pg');
(405, 'jclear', '1994-02-07 17:12:49', 0, 221, 'Can''t delete trigger used by a CHECK Constraint', NULL, NULL, 'c_pg');
(406, 'jclear', '1994-02-07 17:13:11', 0, 222, 'Can''t update trigger used by a CHECK Constraint', NULL, NULL, 'c_pg');
(407, 'jclear', '1994-02-07 17:13:44', 0, 225, 'Can''t update constraints (RDB$RELATION_CONSTRAINTS).', NULL, NULL, 'c_pg');
(408, 'jclear', '1994-02-07 17:14:11', 0, 277, 'Can not prepare a CREATE DATABASE/SCHEMA statement', NULL, NULL, 'c_pg');
(409, 'jclear', '1994-02-07 17:15:01', 0, 299, 'External functions can not have more than 10 parameters', NULL, NULL, 'c_pg');
(410, 'jclear', '1994-02-07 17:15:21', 0, 308, 'can''t create index %s', NULL, NULL, 'c_pg');
(411, 'jclear', '1994-02-07 17:15:42', 0, 341, 'can''t add index, index root page is full.', NULL, NULL, 'c_pg');
(412, 'jclear', '1994-02-07 17:16:26', 16, 217, 'Can''t open mailbox %s', NULL, NULL, 'c_pg');
(413, 'jclear', '1994-02-08 12:03:05', 1, 168, 'no datatype may be specified for a variable based on a field', NULL, NULL, 'c_pg');
(414, 'jclear', '1994-02-08 12:03:25', 1, 255, 'Datatype conflict with existing global field %s', NULL, NULL, 'c_pg');
(415, 'jclear', '1994-02-08 12:03:42', 1, 256, 'No datatype specified for field %s', NULL, NULL, 'c_pg');
(416, 'jclear', '1994-02-08 12:04:55', 1, 468, 'Datatype of field %s may not be changed to or from blob', NULL, NULL, 'c_pg');
(417, 'jclear', '1994-02-08 12:05:46', 1, 504, 'unknown datatype %ld', NULL, NULL, 'c_pg');
(418, 'jclear', '1994-02-08 12:07:18', 12, 26, 'datatype %ld not understood', NULL, NULL, 'c_pg');
(419, 'jclear', '1994-02-08 12:07:48', 12, 7, 'protection isn''t there yet', NULL, NULL, 'c_pg');
(420, 'jclear', '1994-02-08 12:08:34', 12, 43, 'don''t recognize record type %ld', NULL, NULL, 'c_pg');
(421, 'jclear', '1994-02-08 12:08:54', 12, 79, 'don''t understand blob info item %ld', NULL, NULL, 'c_pg');
(422, 'jclear', '1994-02-08 12:10:09', 12, 80, 'don''t recognize %s attribute %ld -- continuing', NULL, NULL, 'c_pg');
(423, 'jclear', '1994-02-08 12:10:56', 1, 36, 'couldn''t create pipe', NULL, NULL, 'c_pg');
(424, 'jclear', '1994-02-08 12:11:24', 1, 43, 'Couldn''t run "%s"', NULL, NULL, 'c_pg');
(425, 'jclear', '1994-02-08 12:11:39', 1, 61, 'couldn''t open scratch file', NULL, NULL, 'c_pg');
(426, 'jclear', '1994-02-08 12:12:03', 1, 247, 'field %s doesn''t exist', NULL, NULL, 'c_pg');
(427, 'jclear', '1994-02-08 12:12:21', 1, 258, 'don''t understand BLR operator %ld', NULL, NULL, 'c_pg');
(428, 'jclear', '1994-02-08 12:13:57', 13, 99, 'Unsuccessful execution caused by system error that doesn''t
preclude successful execution of subsequent statements', NULL, NULL, 'c_pg');
(429, 'jclear', '1994-02-08 12:15:25', 8, 43, 'column %s is used in table %s (local name %s) and can not be dropped', NULL, NULL, 'c_pg');
(430, 'jclear', '1994-02-08 12:16:00', 8, 123, 'Column: %s not defined as NOT NULL - can''t be used in PRIMARY KEY/UNIQUE constraint definition', NULL, NULL, 'c_pg');
(431, 'jclear', '1994-02-08 12:16:45', 1, 480, 'can not format unsubscripted array %s', NULL, NULL, 'c_pg');
(432, 'jclear', '1994-02-08 12:17:26', 16, 116, 'Don''t understand command "%s"', NULL, NULL, 'c_pg');
(433, 'jclear', '1994-02-08 12:17:42', 16, 123, 'Don''t recognize journal message %ld', NULL, NULL, 'c_pg');
(434, 'jclear', '1994-02-08 12:18:03', 16, 175, 'Don''t understand mailbox message type %ld', NULL, NULL, 'c_pg');
(435, 'jclear', '1994-02-08 12:28:26', 16, 222, 'couldn''t open journal file "%s"-', NULL, NULL, 'c_pg');
(436, 'jclear', '1994-02-08 17:19:39', 0, 4, 'invalid database handle (missing READY?)', NULL, NULL, 'c_pg');
(437, 'jclear', '1994-02-08 17:20:09', 0, 4, 'y', NULL, NULL, 'c_pg');
(438, 'jclear', '1994-02-08 17:20:59', 0, 12, 'invalid transaction handle (missing START_TRANSACTION?)', NULL, NULL, 'c_pg');
(439, 'jclear', '1994-02-08 17:33:35', 0, 189, 'Not found: CHARACTER SET %s', 'Define the character set name to the database, or reattach
without specifying a character set.', NULL, 'c_pg');
(440, 'jclear', '1994-02-08 17:33:55', 0, 268, 'Not found: COLLATION %s', NULL, NULL, 'c_pg');
(441, 'jclear', '1994-02-08 17:34:29', 0, 342, 'Not found: BLOB SUB_TYPE %s', NULL, NULL, 'c_pg');
(442, 'jclear', '1994-02-08 17:36:11', 0, 37, 'cannot detach database with open transactions (%ld active)', 'Commit or roll back those transactions before you detach
the database.', 'Your program attempted to detach a database without
committing or rolling back one or more transactions.', 'c_pg');
(443, 'jclear', '1994-02-08 17:42:08', 0, 247, 'Overflow log specification required for round-robin logs configuration', NULL, NULL, 'c_pg');
(444, 'jclear', '1994-02-08 17:43:04', 0, 251, 'Constant data type unknown', NULL, NULL, 'c_pg');
(445, 'jclear', '1994-02-08 17:45:59', 0, 316, 'there is no index %s in table %s', NULL, NULL, 'c_pg');
(446, 'jclear', '1994-02-09 10:49:37', 0, 42, 'can''t update read only view %s', 'Views that include a record select, join, or project cannot
be updated.  If you want to perform updates, you must do so
through the source relations.  If you are updating join terms,
make sure that you change them in all relations.  In any case,
 update the source relations in a single transaction so that
you make the changes consistently.', 'Your program tried to update a view that contains a
record select, join, or project operation.', 'c_pg');
(447, 'jclear', '1994-02-09 10:57:12', 0, 144, 'secondary server attachments can not start logging', NULL, NULL, 'c_pg');
(448, 'deej', '1994-02-09 17:05:01', 0, 324, 'attempt to fetch backwards past the first record in a record stream', NULL, NULL, 'c_pg');
(449, 'deej', '1994-02-09 17:05:24', 0, 324, 'illegal operation when at beginning of file', NULL, NULL, 'c_pg');
(450, 'deej', '1994-02-09 17:05:37', 0, 324, 'illegal operation when at end of stream', NULL, NULL, 'c_pg');
(451, 'jclear', '1994-02-18 15:50:54', 0, 168, 'Logfile header of %s too small', NULL, NULL, 'c_pg');
(452, 'jclear', '1994-02-18 15:51:20', 0, 169, 'Invalid version of logfile %s', NULL, NULL, 'c_pg');
(453, 'jclear', '1994-02-18 15:51:38', 0, 170, 'Logfile %s not latest in the chain but open flag still set', NULL, NULL, 'c_pg');
(454, 'jclear', '1994-02-18 15:51:55', 0, 171, 'Logfile %s not closed properly; database recovery may be required', NULL, NULL, 'c_pg');
(455, 'jclear', '1994-02-18 15:52:21', 0, 172, 'Database name in the logfile %s is different', NULL, NULL, 'c_pg');
(456, 'jclear', '1994-02-18 15:52:43', 0, 173, 'Unexpected end of logfile %s at offset %ld', NULL, NULL, 'c_pg');
(457, 'jclear', '1994-02-18 15:53:09', 0, 174, 'Incomplete log record at offset %ld in logfile %s', NULL, NULL, 'c_pg');
(458, 'jclear', '1994-02-18 15:53:32', 0, 175, 'Log record header too small at offset %ld in logfile %s', NULL, NULL, 'c_pg');
(459, 'jclear', '1994-02-18 15:53:55', 0, 176, 'Log block too small at offset %ld in logfile %s', NULL, NULL, 'c_pg');
(460, 'jclear', '1994-02-18 15:54:26', 0, 179, 'Cannot rollover to the next logfile %s', NULL, NULL, 'c_pg');
(461, 'jclear', '1994-02-18 15:54:49', 0, 181, 'cannot drop logfile when journaling is enabled', NULL, NULL, 'c_pg');
(462, 'jclear', '1994-02-18 15:55:44', 2, 337, 'Cannot add and drop logfile in same statement.', NULL, NULL, 'c_pg');
(463, 'jclear', '1994-02-18 16:00:11', 0, 162, 'journaling allowed only if database has Write Ahead Log', NULL, NULL, 'c_pg');
(464, 'jclear', '1994-02-18 16:00:37', 0, 164, 'error in opening Write Ahead Log file during recovery', NULL, NULL, 'c_pg');
(465, 'jclear', '1994-02-18 16:00:59', 0, 166, 'write ahead log subsystem failure', NULL, NULL, 'c_pg');
(466, 'jclear', '1994-02-18 16:01:29', 0, 180, 'database does not use write ahead log', NULL, NULL, 'c_pg');
(467, 'jclear', '1994-02-18 16:02:02', 0, 236, 'Write-Ahead Log without Shared Cache configuration not allowed', NULL, NULL, 'c_pg');
(468, 'jclear', '1994-02-18 16:02:27', 0, 309, 'Write-Ahead Log with Shadowing configuration not allowed', NULL, NULL, 'c_pg');
(469, 'jclear', '1994-02-18 16:03:17', 2, 321, 'error commiting new write ahead log declarations', NULL, NULL, 'c_pg');
(470, 'jclear', '1994-02-18 16:03:57', 2, 327, 'error in getting write ahead log information', NULL, NULL, 'c_pg');
(471, 'jclear', '1994-02-18 16:05:30', 8, 151, 'Write ahead log already exists', NULL, NULL, 'c_pg');
(472, 'jclear', '1994-02-18 16:05:47', 8, 152, 'Write ahead log not found', NULL, NULL, 'c_pg');
(473, 'jclear', '1994-02-18 16:06:05', 8, 155, 'Write ahead log lookup failed', NULL, NULL, 'c_pg');
(474, 'jclear', '1994-02-18 16:08:11', 0, 23, 'invalid request blr at offset %ld', NULL, NULL, 'c_pg');
(475, 'jclear', '1994-02-18 16:08:37', 0, 70, 'blr syntax error: expected %s at offset %ld, encountered %ld', NULL, NULL, 'c_pg');
(476, 'jclear', '1994-02-18 16:09:03', 0, 92, 'unsupported blr version (expected %ld, encountered %ld)', 'Define blob filter in RDB$BLOB_FILTERS.', 'Blob filter was not found.', 'c_pg');
(477, 'jclear', '1994-02-18 16:14:39', 1, 108, ', subtype blr', NULL, NULL, 'c_pg');
(478, 'jclear', '1994-02-18 16:14:50', 1, 321, ', subtype blr', NULL, NULL, 'c_pg');
(479, 'jclear', '1994-02-18 16:16:17', 15, 222, 'bad blr -- invalid stream', NULL, NULL, 'c_pg');
(480, 'jclear', '1994-02-18 16:42:30', 0, 8, 'invalid blob handle', NULL, NULL, 'c_pg');
(481, 'jclear', '1994-02-18 16:42:58', 0, 9, 'invalid blob id', NULL, NULL, 'c_pg');
(482, 'jclear', '1994-02-18 16:43:38', 0, 35, 'blob was not closed', NULL, NULL, 'c_pg');
(483, 'jclear', '1994-02-18 16:44:01', 0, 48, 'attempted invalid operation on a blob', 'Check your program to make sure that it does not reference
a blob field in a Boolean expression or in a statement
not intended for use with blobs.  Both GDML and the call
interface have statements or routines that perform
blob storage, retrieval, and update.', 'Your program tried to do something that cannot be done
with blob fields.', 'c_pg');
(484, 'jclear', '1994-02-18 16:45:05', 0, 49, 'attempted read of a new, open blob', 'Check and correct your program logic.  Close the blob field
before you try to read from it.', 'Your program tried to read from a blob field that it is
creating.', 'c_pg');
(485, 'jclear', '1994-02-18 16:45:52', 0, 51, 'attempted write to read-only blob', 'If you are using the call interface, open the blob for
by calling gds_$create_blob.  If you are using GDML, open
the blob with the create_blob statement.', 'Your program tried to write to a blob field that
that had been opened for read access.', 'c_pg');
(486, 'jclear', '1994-02-18 16:49:16', 0, 52, 'attempted reference to blob in unavailable database', 'Change your program so that the required database is
available to the current transaction.', 'Your program referenced a blob field from a relation
in a database that is not available to the current
transaction.', 'c_pg');
(487, 'jclear', '1994-02-18 16:51:02', 0, 94, 'blob and array data types are not supported for %s operation', NULL, NULL, 'c_pg');
(488, 'jclear', '1994-02-18 16:58:02', 0, 145, 'invalid blob type for operation', 'Program attempted a seek on a non-stream (i.e. segmented) blob.', 'Program attempted to a blob seek operation on a non-stream (i.e.
segmented) blob.', 'c_pg');
(489, 'jclear', '1994-02-18 16:59:28', 0, 267, 'Column is not a blob', NULL, NULL, 'c_pg');
(490, 'jclear', '1994-02-18 17:16:37', 0, 337, 'arrays or blobs not allowed in value expression', NULL, NULL, 'c_pg');
(491, 'jclear', '1994-02-18 17:17:23', 0, 350, 'attempt to index blob column in index %s', NULL, NULL, 'c_pg');
(492, 'jclear', '1994-02-18 17:18:10', 1, 48, 'Blob conversion is not supported', NULL, NULL, 'c_pg');
(493, 'jclear', '1994-02-18 17:19:08', 1, 55, 'Blob conversion is not supported', NULL, NULL, 'c_pg');
(494, 'jclear', '1994-02-18 17:19:28', 1, 105, 'blob', NULL, NULL, 'c_pg');
(495, 'jclear', '1994-02-18 17:20:13', 1, 137, 'variables may not be based on blob fields', NULL, NULL, 'c_pg');
(496, 'jclear', '1994-02-18 17:20:35', 1, 160, 'blob variables are not supported', NULL, NULL, 'c_pg');
(497, 'jclear', '1994-02-18 17:20:50', 1, 318, 'blob', NULL, NULL, 'c_pg');
(498, 'jclear', '1994-02-18 17:21:11', 1, 356, 'EDIT argument must be a blob field', NULL, NULL, 'c_pg');
(499, 'jclear', '1994-02-18 17:22:23', 1, 358, 'can''t find database for blob edit', NULL, NULL, 'c_pg');
(500, 'jclear', '1994-02-18 17:22:58', 1, 468, 'Data type of field %s may not be changed to or from blob', NULL, NULL, 'c_pg');
(501, 'jclear', '1994-02-18 17:24:19', 2, 83, 'Unauthorized attempt to change field %s to or from blob', NULL, NULL, 'c_pg');
(502, 'jclear', '1994-02-18 17:28:13', 2, 94, '(EXE) string_length: No defined length for blobs', NULL, NULL, 'c_pg');
(503, 'jclear', '1994-02-18 17:28:35', 2, 228, 'subtypes are valid only for blobs and text', NULL, NULL, 'c_pg');
(504, 'jclear', '1994-02-18 17:29:35', 2, 229, 'segment length is valid only for blobs', NULL, NULL, 'c_pg');
(505, 'jclear', '1994-02-18 17:30:01', 2, 270, '*****  blob option not understood ****', NULL, NULL, 'c_pg');
(506, 'jclear', '1994-02-18 17:30:47', 8, 37, 'Blob Filter not found', NULL, NULL, 'c_pg');
(507, 'jclear', '1994-02-18 17:31:12', 8, 106, 'Create metadata blob failed', NULL, NULL, 'c_pg');
(508, 'jclear', '1994-02-18 17:31:27', 8, 107, 'Write metadata blob failed', NULL, NULL, 'c_pg');
(509, 'jclear', '1994-02-18 17:31:42', 8, 108, 'Close metadata blob failed', NULL, NULL, 'c_pg');
(510, 'jclear', '1994-02-18 17:32:04', 8, 109, 'Create metadata blob failed', NULL, NULL, 'c_pg');
(511, 'jclear', '1994-02-18 17:33:32', 8, 116, ' attempt to index blob column in index %s', NULL, NULL, 'c_pg');
(512, 'jclear', '1994-02-18 17:34:18', 12, 21, 'don''t understand blob info item %ld', NULL, NULL, 'c_pg');
(513, 'jclear', '1994-02-18 17:34:40', 12, 36, 'Cannot find field for blob', NULL, NULL, 'c_pg');
(514, 'jclear', '1994-02-18 17:35:14', 12, 79, 'do not understand blob info item %ld', NULL, NULL, 'c_pg');
(515, 'jclear', '1994-02-18 17:37:04', 12, 81, 'error accessing blob field %s -- continuing', NULL, NULL, 'c_pg');
(516, 'jclear', '1994-02-18 17:37:41', 15, 194, 'blob not found', NULL, NULL, 'c_pg');
(517, 'jclear', '1994-02-18 17:37:56', 15, 195, 'cannot update old blob', NULL, NULL, 'c_pg');
(518, 'jclear', '1994-02-18 17:38:19', 15, 200, 'invalid blob id', NULL, NULL, 'c_pg');
(519, 'jclear', '1994-02-18 17:38:39', 15, 201, 'cannot find blob page', NULL, NULL, 'c_pg');
(520, 'jclear', '1994-02-18 17:39:16', 15, 289, 'Unknown blob filter ACTION_', NULL, NULL, 'c_pg');
(521, 'jclear', '1994-02-18 17:40:25', 17, 23, 'BLOBDUMP <blobid as high:low> <file> -- dump blob to a file', NULL, NULL, 'c_pg');
(522, 'jclear', '1994-02-18 17:40:56', 17, 22, 'BLOBEDIT [<blobid as high:low>] -- edit a blob', NULL, NULL, 'c_pg');
(523, 'jclear', '1994-02-18 17:41:13', 17, 23, 'BLOBDUMP <blobid as high:low> <file> -- dump BLOB to a file', NULL, NULL, 'c_pg');
(524, 'jclear', '1994-02-18 17:42:18', 17, 32, '	SET BLOBdisplay [ALL|N]-- Display blobs of type N
	 SET BLOB turns off blob display', NULL, NULL, 'c_pg');
(525, 'jclear', '1994-02-18 17:43:05', 17, 46, 'Blob display set to subtype %ld. This blob: subtype = %ld', NULL, NULL, 'c_pg');
(526, 'jclear', '1994-02-18 17:43:29', 17, 47, 'Blob: %s, type ''edit'' or filename to load>', NULL, NULL, 'c_pg');
(527, 'jclear', '1994-02-18 17:46:39', 0, 213, 'Non-existent Primary or Unique key specified for Foreign Key.', NULL, NULL, 'c_pg');
(528, 'jclear', '1994-02-18 17:47:47', 0, 228, 'Attempt to define a second primary key for the same table', NULL, NULL, 'c_pg');
(529, 'jclear', '1994-02-18 17:48:36', 0, 284, 'foreign key column count does not match primary key', NULL, NULL, 'c_pg');
(530, 'jclear', '1994-02-18 17:49:15', 0, 345, 'violation of primary or unique key constraint: "%s"', NULL, NULL, 'c_pg');
(531, 'jclear', '1994-02-18 17:49:50', 8, 17, 'Primary Key column lookup failed', NULL, NULL, 'c_pg');
(532, 'jclear', '1994-02-18 17:51:11', 8, 19, 'Primary Key lookup failed', NULL, NULL, 'c_pg');
(533, 'jclear', '1994-02-18 17:51:52', 8, 20, 'could not find primary key index in specified table', NULL, NULL, 'c_pg');
(534, 'jclear', '1994-02-18 17:53:31', 0, 146, 'violation of foreign key constraint: "%s"', NULL, NULL, 'c_pg');
(535, 'jclear', '1994-02-18 17:55:35', 0, 115, 'null segment of unique key', NULL, NULL, 'c_pg');
(536, 'jclear', '1994-02-18 17:56:25', 8, 18, 'could not find unique index with specified columns', NULL, NULL, 'c_pg');
(537, 'jclear', '1994-02-18 17:58:23', 0, 223, 'Cannot delete column being used in an integrity constraint.', NULL, NULL, 'c_pg');
(538, 'jclear', '1994-02-18 17:58:46', 0, 224, 'Cannot rename column being used in an integrity constraint.', NULL, NULL, 'c_pg');
(539, 'jclear', '1994-02-18 17:59:20', 8, 125, 'Integrity constraint lookup failed', NULL, NULL, 'c_pg');
(540, 'jclear', '1994-02-18 18:04:21', 0, 117, 'wrong dyn version', NULL, NULL, 'c_pg');
(541, 'jclear', '1994-02-18 18:05:13', 8, 9, 'DEFINE GENERATOR unexpected dyn verb', NULL, NULL, 'c_pg');
(542, 'jclear', '1994-02-18 18:09:40', 0, 158, 'enable journal for database before starting Online dump', NULL, NULL, 'c_pg');
(543, 'jclear', '1994-02-18 18:10:01', 0, 160, 'an Online dump is already in progress', NULL, NULL, 'c_pg');
(544, 'jclear', '1994-02-18 18:10:19', 0, 161, 'no more disk/tape space.  Cannot continue Online dump', NULL, NULL, 'c_pg');
(545, 'jclear', '1994-02-18 18:10:52', 0, 163, 'maximum number of Online dump files that can be specified is 16', NULL, NULL, 'c_pg');
(546, 'jclear', '1994-02-18 18:12:09', 16, 191, 'Processing Online dump file %s.', NULL, NULL, 'c_pg');
(547, 'jclear', '1994-02-18 18:14:27', 0, 42, 'cannot update read only view %s', 'Views that include a record select, join, or project cannot
be updated.  If you want to perform updates, you must do so
through the source relations.  If you are updating join terms,
make sure that you change them in all relations.  In any case,
 update the source relations in a single transaction so that
you make the changes consistently.', 'Your program tried to update a view that contains a
record select, join, or project operation.', 'c_pg');
(548, 'jclear', '1994-02-18 18:15:28', 0, 180, 'database does not use Write-ahead log', NULL, NULL, 'c_pg');
(549, 'jclear', '1994-02-18 18:17:35', 0, 185, 'must specify archive file when enabling long term journal for databases with round robin log files', NULL, NULL, 'c_pg');
(550, 'jclear', '1994-02-18 18:18:11', 0, 179, 'Cannot rollover to the next log file %s', NULL, NULL, 'c_pg');
COMMIT WORK;
(551, 'jclear', '1994-02-18 18:19:19', 0, 201, 'Unable to rollover.  Please see InterBase log.', NULL, NULL, 'c_pg');
(552, 'jclear', '1994-02-18 18:19:49', 0, 204, 'WAL buffers can not be increased.  Please see InterBase log.', NULL, NULL, 'c_pg');
(553, 'jclear', '1994-02-18 18:20:23', 0, 226, 'Cannot define constraints on VIEWS', NULL, NULL, 'c_pg');
(554, 'jclear', '1994-02-18 18:21:01', 0, 236, 'Write-ahead Log without Shared Cache configuration not allowed', NULL, NULL, 'c_pg');
(555, 'jclear', '1994-02-18 18:22:02', 0, 244, 'long term journaling not enabled', NULL, NULL, 'c_pg');
(556, 'jclear', '1994-02-18 18:22:32', 0, 309, 'Write-ahead Log with Shadowing configuration not allowed', NULL, NULL, 'c_pg');
(557, 'jclear', '1994-02-24 16:46:40', 0, 264, 'Count of columns not equal count of values', NULL, NULL, 'c_pg');
(558, 'jclear', '1994-02-25 11:53:35', 0, 9, 'invalid BLOB id', NULL, NULL, 'c_pg');
(559, 'jclear', '1994-02-25 11:54:12', 0, 59, 'unsupported on disk structure for file %s; found %ld, support %ld', NULL, NULL, 'c_pg');
(560, 'jclear', '1994-02-25 11:55:06', 0, 157, 'maximum indices per table (%d) exceeded', NULL, NULL, 'c_pg');
(561, 'jclear', '1994-02-25 11:55:33', 0, 200, 'long term journaling already enabled', NULL, NULL, 'c_pg');
(562, 'jclear', '1994-02-25 12:01:46', 0, 227, 'internal gds software consistency check (Invalid RDB$CONSTRAINT_TYPE)', NULL, NULL, 'c_pg');
(563, 'jclear', '1994-02-25 12:03:51', 0, 281, 'No where clause for VIEW WITH CHECK OPTION', NULL, NULL, 'c_pg');
(564, 'jclear', '1994-02-25 12:04:37', 1, 4, 'bad pool id', NULL, NULL, 'c_pg');
(565, 'jclear', '1994-02-25 12:05:10', 1, 42, 'Can''t open output file "%s"', NULL, NULL, 'c_pg');
(566, 'jclear', '1994-02-25 12:11:13', 1, 93, '	No indices defined', NULL, NULL, 'c_pg');
(567, 'jclear', '1994-02-25 12:11:29', 1, 94, '	No indices defined', NULL, NULL, 'c_pg');
(568, 'jclear', '1994-02-25 12:13:40', 1, 126, 'Procedure %s not found  in database %s', NULL, NULL, 'c_pg');
(569, 'jclear', '1994-02-25 12:14:13', 1, 138, 'can''t perform assignment to computed field %s', NULL, NULL, 'c_pg');
(570, 'jclear', '1994-02-25 12:14:33', 1, 140, 'can''t erase from a join', NULL, NULL, 'c_pg');
(571, 'jclear', '1994-02-25 12:15:12', 1, 182, 'FROM rse clause', NULL, NULL, 'c_pg');
(572, 'jclear', '1994-02-25 12:17:54', 1, 233, 'global field%s already exists', NULL, NULL, 'c_pg');
(573, 'jclear', '1994-02-25 12:18:31', 1, 242, 'meta-data operation failed', NULL, NULL, 'c_pg');
(574, 'jclear', '1994-02-25 12:21:13', 1, 351, 'Do you want to rollback your updates?', NULL, NULL, 'c_pg');
(575, 'jclear', '1994-02-25 12:21:39', 1, 357, 'relations from multiple databases in single rse', NULL, NULL, 'c_pg');
(576, 'jclear', '1994-02-25 12:22:11', 1, 410, 'Can not convert from %s to %s', NULL, NULL, 'c_pg');
(577, 'jclear', '1994-02-25 12:23:29', 1, 465, 'Error during two phase commit on database %s
rollback all databases or commit databases individually', NULL, NULL, 'c_pg');
(578, 'jclear', '1994-02-25 12:27:18', 2, 26, 'error commiting metadata changes', NULL, NULL, 'c_pg');
(579, 'jclear', '1994-02-25 12:28:28', 2, 36, 'error commiting metadata changes', NULL, NULL, 'c_pg');
(580, 'jclear', '1994-02-25 12:29:00', 2, 40, 'error commiting new file declarations', NULL, NULL, 'c_pg');
(581, 'jclear', '1994-02-25 12:29:46', 2, 44, 'index %s: field %s doesn''t exist in relation %s', NULL, NULL, 'c_pg');
(582, 'jclear', '1994-02-25 12:30:06', 2, 72, 'error commiting deletion of shadow', NULL, NULL, 'c_pg');
(583, 'jclear', '1994-02-25 12:30:46', 2, 74, 'Trigger message number %ld for trigger %s doesn''t exist', NULL, NULL, 'c_pg');
(584, 'jclear', '1994-02-25 12:32:46', 2, 121, 'only SECURITY_CLASS, DESCRIPTION and CACHE can be dropped', NULL, NULL, 'c_pg');
(585, 'jclear', '1994-02-25 12:33:40', 2, 132, 'expected comma or semi-colon, encountered "%s"', NULL, NULL, 'c_pg');
(586, 'jclear', '1994-02-25 12:34:22', 2, 135, 'expected comma or semi-colon, encountered "%s"', NULL, NULL, 'c_pg');
(587, 'jclear', '1994-02-25 12:34:36', 2, 138, 'expected comma or semi-colon, encountered "%s"', NULL, NULL, 'c_pg');
(588, 'jclear', '1994-02-25 12:35:54', 2, 190, 'expected semi-colon, encountered "%s"', NULL, NULL, 'c_pg');
(589, 'jclear', '1994-02-25 12:36:41', 2, 194, 'computed by expression must be parenthesized', NULL, NULL, 'c_pg');
(590, 'jclear', '1994-02-25 12:38:54', 2, 207, 'expected comma between group and user ids, encountered "%s"', NULL, NULL, 'c_pg');
(591, 'jclear', '1994-02-25 12:40:21', 2, 239, 'expected FROM rse clause, encountered "%s"', NULL, NULL, 'c_pg');
(592, 'jclear', '1994-02-25 12:40:59', 2, 294, 'unexpected end of file, semi_colon missing?', NULL, NULL, 'c_pg');
(593, 'jclear', '1994-02-25 12:41:36', 2, 321, 'error commiting new Write-ahead Log declarations', NULL, NULL, 'c_pg');
(594, 'jclear', '1994-02-25 12:41:56', 2, 324, 'error commiting new shared cache file declaration', NULL, NULL, 'c_pg');
(595, 'jclear', '1994-02-25 12:42:24', 2, 326, 'error commiting deletion of shared cache file', NULL, NULL, 'c_pg');
(596, 'jclear', '1994-02-25 12:43:54', 12, 9, 'mutiple sources or destinations specified', NULL, NULL, 'c_pg');
(597, 'jclear', '1994-02-25 12:44:21', 12, 18, 'can''t create APOLLO cartridge descriptor file %s', NULL, NULL, 'c_pg');
(598, 'jclear', '1994-02-25 12:44:36', 12, 21, 'don''t understand BLOB INFO item %ld', NULL, NULL, 'c_pg');
(599, 'jclear', '1994-02-25 12:45:08', 12, 68, '    committing meta data', NULL, NULL, 'c_pg');
(600, 'jclear', '1994-02-25 12:46:05', 12, 132, 'switches can be abbreviated to the unparenthesised characters', NULL, NULL, 'c_pg');
(601, 'jclear', '1994-02-25 12:50:40', 13, 196, 'An error was found in the application program input parameters for the sql statement.', NULL, NULL, 'c_pg');
(602, 'jclear', '1994-02-25 12:51:24', 13, 849, 'This column cannot be updated because it is derived from a sql function or expression.', NULL, NULL, 'c_pg');
(603, 'jclear', '1994-02-25 12:52:42', 13, 849, 'This column cannot be updated because it is derived from a SQL function or expression.', NULL, NULL, 'c_pg');
(604, 'jclear', '1994-02-25 12:53:45', 15, 147, 'invalid block type encoutered', NULL, NULL, 'c_pg');
(605, 'jclear', '1994-02-25 12:59:42', 16, 84, 'do you want to change this?:(y/n):', NULL, NULL, 'c_pg');
(606, 'jclear', '1994-02-25 13:00:40', 16, 159, 'Mark database "%s" as disabled for long term journaling.', NULL, NULL, 'c_pg');
(607, 'jclear', '1994-02-25 13:01:59', 16, 170, '	%s:	shutdown journal server', NULL, NULL, 'c_pg');
(608, 'jclear', '1994-02-25 13:02:37', 17, 40, 'QUIT -- Exit program and rollback changes', NULL, NULL, 'c_pg');
(609, 'jclear', '1994-02-25 13:04:09', 17, 46, 'Blob display set to subtype %ld. This BLOB: subtype = %ld', NULL, NULL, 'c_pg');
(610, 'jclear', '1994-03-04 16:44:50', 0, 189, 'CHARACTER SET %s not defined', 'Define the character set name to the database, or reattach
without specifying a character set.', NULL, 'c_pg');
(611, 'jclear', '1994-03-04 16:45:14', 0, 268, 'COLLATION %s not defined', NULL, NULL, 'c_pg');
(612, 'jclear', '1994-03-04 16:45:40', 0, 342, 'BLOB SUB_TYPE %s not defined', NULL, NULL, 'c_pg');
(613, 'christy', '1994-03-10 16:18:58', 0, 384, 'Network lookup failure for host %s.', NULL, NULL, 'c_pg');
(614, 'harjit', '1994-03-23 14:43:35', 0, 263, 'SQLDA missing or wrong number of variables', NULL, NULL, 'c_pg');
(615, 'eds', '1994-03-23 19:45:40', 17, 56, 'Total returned: %ld', NULL, NULL, 'c_pg');
(616, 'hcl', '1994-03-25 11:42:41', 12, 233, 'database %s is in use and cannot be replaced until it no longer is', NULL, NULL, 'c_pg');
(617, 'klaus', '1994-03-25 13:36:44', 19, 1, '                 S T O P -- S T O P -- S T O P', NULL, NULL, 'c_pg');
(618, 'chander', '1994-03-29 15:46:57', 0, 337, 'Arrays or BLOBs not allowed in value expression', NULL, NULL, 'c_pg');
(619, 'chander', '1994-03-29 15:47:37', 0, 337, 'Arrays/BLOBs/Date not allowed in arithmetic expressions', NULL, NULL, 'c_pg');
(620, 'chander', '1994-03-29 15:48:27', 0, 337, 'Arrays/BLOBs/DATEs not allowed in arithmetic expressions', NULL, NULL, 'c_pg');
(621, 'deej', '1994-04-04 09:58:09', 0, 110, 'virtual memory exhausted', NULL, NULL, 'c_pg');
(622, 'katz', '1994-04-04 11:53:04', 19, 1, 'The InterBase license file does not contain the correct key for this product.', NULL, NULL, 'c_pg');
(623, 'eds', '1994-04-13 04:23:57', 17, 8, 'Unknown command: %s', NULL, NULL, 'c_pg');
(624, 'jclear', '1994-04-28 17:33:27', 0, 337, 'Array/BLOB/DATE datatypes not allowed in arithmetic expressions', NULL, NULL, 'c_pg');
(625, 'jclear', '1994-04-28 17:33:57', 12, 35, 'cannot find relation %s', NULL, NULL, 'c_pg');
(626, 'jclear', '1994-04-28 17:34:26', 12, 36, 'Cannot find field for BLOB', NULL, NULL, 'c_pg');
(627, 'jclear', '1994-04-28 17:34:43', 12, 49, 'no relation name for data', NULL, NULL, 'c_pg');
(628, 'jclear', '1994-04-28 17:35:23', 12, 52, 'array dimension for field %s is invalid', NULL, NULL, 'c_pg');
(629, 'jclear', '1994-04-28 17:38:01', 12, 69, 'commit failed on relation %s', NULL, NULL, 'c_pg');
(630, 'jclear', '1994-04-28 17:38:40', 12, 72, 'committing data for relation %s', NULL, NULL, 'c_pg');
(631, 'jclear', '1994-04-28 17:39:16', 12, 81, 'error accessing BLOB field %s -- continuing', NULL, NULL, 'c_pg');
(632, 'jclear', '1994-04-28 17:39:31', 12, 84, 'field', NULL, NULL, 'c_pg');
(633, 'jclear', '1994-04-28 17:39:55', 12, 92, 'global field', NULL, NULL, 'c_pg');
(634, 'jclear', '1994-04-28 17:40:22', 12, 99, '	%sO(NE_AT_A_TIME)      restore one relation at a time', NULL, NULL, 'c_pg');
(635, 'jclear', '1994-04-28 17:40:37', 12, 111, 'relation', NULL, NULL, 'c_pg');
(636, 'jclear', '1994-04-28 17:41:07', 12, 114, 'restore failed for record in relation %s', NULL, NULL, 'c_pg');
(637, 'jclear', '1994-04-28 17:42:05', 12, 121, 'restoring global field %s', NULL, NULL, 'c_pg');
(638, 'jclear', '1994-04-28 17:43:05', 12, 124, 'restoring data for relation %s', NULL, NULL, 'c_pg');
(639, 'jclear', '1994-04-28 17:44:01', 12, 128, '    restoring type %s for field %s', NULL, NULL, 'c_pg');
(640, 'jclear', '1994-04-28 17:44:31', 12, 138, 'validation error on field in relation %s', NULL, NULL, 'c_pg');
(641, 'jclear', '1994-04-28 17:45:10', 12, 142, '    writing data for relation %s', NULL, NULL, 'c_pg');
(642, 'jclear', '1994-04-28 17:45:53', 12, 144, '         writing field %s', NULL, NULL, 'c_pg');
(643, 'jclear', '1994-04-28 17:46:33', 12, 144, '        writing field %s', NULL, NULL, 'c_pg');
(644, 'jclear', '1994-04-28 17:47:02', 12, 149, '    writing global field %s', NULL, NULL, 'c_pg');
(645, 'jclear', '1994-04-28 17:47:18', 12, 150, 'writing global fields', NULL, NULL, 'c_pg');
(646, 'jclear', '1994-04-28 17:47:37', 12, 149, '    writing global column %s', NULL, NULL, 'c_pg');
(647, 'jclear', '1994-04-28 17:48:10', 12, 154, 'writing relations', NULL, NULL, 'c_pg');
(648, 'jclear', '1994-04-28 17:48:26', 12, 153, '    writing relation %s', NULL, NULL, 'c_pg');
(649, 'jclear', '1994-04-28 17:49:02', 12, 160, '    writing type %s for field %s', NULL, NULL, 'c_pg');
(650, 'jclear', '1994-04-28 17:49:19', 12, 167, 'restoring relation %s', NULL, NULL, 'c_pg');
(651, 'jclear', '1994-04-28 17:49:41', 12, 170, 'commiting metadata for relation %s', NULL, NULL, 'c_pg');
(652, 'jclear', '1994-04-28 17:50:15', 12, 179, '	field %s used in index %s seems to have vanished', NULL, NULL, 'c_pg');
(653, 'jclear', '1994-04-28 17:50:31', 12, 206, 'writing relation constraints', NULL, NULL, 'c_pg');
(654, 'jclear', '1994-04-28 17:51:09', 12, 208, 'Bad attribute for relation constraint', NULL, NULL, 'c_pg');
(655, 'eds', '1994-05-02 19:19:51', 13, 490, 'Attempt to update non-updateable table', NULL, NULL, 'c_pg');
(656, 'jclear', '1994-05-10 16:31:56', 0, 152, 'Your user name and password are not defined. Ask your database administrator to run GSEC to set up an InterBase login.', NULL, NULL, 'c_pg');
(657, 'jclear', '1994-05-10 16:42:06', 18, 26, '    user name       uid   gid     project   organization       full name', NULL, NULL, 'c_pg');
(658, 'jclear', '1994-06-03 15:35:54', 0, 320, 'CHARACTER SET & COLLATE apply only to text data types', NULL, NULL, 'c_pg');
(659, 'daves', '1994-07-08 10:33:40', 1, 106, ', segment length %ld', NULL, NULL, 'c_pg');
(660, 'daves', '1994-07-18 10:10:02', 12, 190, '	%sPA(SSWORD)           InterBase password', NULL, NULL, 'c_pg');
(661, 'daves', '1994-07-18 10:17:05', 12, 144, '         writing field %s', NULL, NULL, 'c_pg');
(662, 'daves', '1994-07-18 10:17:36', 12, 104, '	%sZ    print version number', NULL, NULL, 'c_pg');
(663, 'daves', '1994-07-18 10:18:38', 12, 109, '	%sY    redirect/suppress output (file path or OUTPUT_SUPPRESS)', NULL, NULL, 'c_pg');
(664, 'daves', '1994-07-18 10:34:16', 12, 190, '	%sPAS(SWORD)		InterBase password', NULL, NULL, 'c_pg');
(665, 'daves', '1994-07-18 10:34:54', 12, 104, '	%sZ		print version number', NULL, NULL, 'c_pg');
(666, 'daves', '1994-07-18 10:36:33', 12, 109, '	%sY		redirect/supress status output (path or OUTPUT_SUPRESS required)', NULL, NULL, 'c_pg');
(667, 'daves', '1994-07-18 10:39:14', 12, 104, '	%sZ		      print version number', NULL, NULL, 'c_pg');
(668, 'daves', '1994-07-18 10:39:30', 12, 109, '	%sY  <path>           redirect/supress status message output', NULL, NULL, 'c_pg');
(669, 'daves', '1994-07-18 11:00:17', 13, 382, 'Cannot delete object referencec by another object', NULL, NULL, 'c_pg');
(670, 'daves', '1994-07-18 11:02:30', 2, 177, 'Unsuccesful attempt to modify trigger relation', NULL, NULL, 'c_pg');
(671, 'daves', '1994-07-18 11:04:54', 12, 170, 'commiting metadata for table %s', NULL, NULL, 'c_pg');
(672, 'daves', '1994-07-18 11:05:46', 12, 171, 'error commiting metadata for relation %s', NULL, NULL, 'c_pg');
(673, 'daves', '1994-07-18 11:07:03', 16, 104, ' Number of archives running concurrently excceds limit.', NULL, NULL, 'c_pg');
(674, 'daves', '1994-07-18 11:08:46', 13, 198, 'Arithmetic overflow or division by zero has occurred.', NULL, NULL, 'c_pg');
(675, 'daves', '1994-07-18 11:10:26', 12, 109, '	%sY  <path>            redirect/supress status message output', NULL, NULL, 'c_pg');
(676, 'daves', '1994-07-18 11:12:10', 13, 492, 'The cursor identifed in the update or delete statement is not positioned on a row.', NULL, NULL, 'c_pg');
(677, 'daves', '1994-07-18 11:14:21', 0, 391, 'Attempt to execute an unprepared dynamic sql statement.', NULL, NULL, 'c_pg');
(678, 'daves', '1994-07-18 11:16:56', 8, 74, 'ERASE RDB$SECURITY_CLASSESS failed', NULL, NULL, 'c_pg');
(679, 'daves', '1994-07-18 11:24:13', 12, 104, '	%sZ		       print version number', NULL, NULL, 'c_pg');
(680, 'daves', '1994-07-18 11:26:13', 12, 104, '	%sZ             print version number', NULL, NULL, 'c_pg');
(681, 'daves', '1994-07-18 11:32:16', 8, 25, 'STORE RDB$USER_PRIVILEGES failed defining a relation', NULL, NULL, 'c_pg');
(682, 'daves', '1994-07-18 11:32:34', 8, 81, 'Generation of field name failed', NULL, NULL, 'c_pg');
(683, 'daves', '1994-07-18 11:33:13', 12, 115, '    restoring field %s', NULL, NULL, 'c_pg');
(684, 'daves', '1994-07-18 11:33:45', 12, 138, 'validation error on column in relation %s', NULL, NULL, 'c_pg');
(685, 'daves', '1994-07-18 11:34:24', 13, 697, 'Incompatible field/host variable data type', NULL, NULL, 'c_pg');
(716, 'daves', '1994-07-26 19:55:33', 17, 21, 'Frontend commands:', NULL, NULL, 'c_pg');
(717, 'daves', '1994-07-26 19:55:33', 17, 23, 'BLOBDUMP <blobid as high:low> <file> -- dump BLOB to a file', NULL, NULL, 'c_pg');
(718, 'daves', '1994-07-26 19:55:33', 17, 22, 'BLOBEDIT [<blobid as high:low>] -- edit a BLOB', NULL, NULL, 'c_pg');
(719, 'daves', '1994-07-26 19:55:33', 17, 24, 'EDIT [<filename>] -- edit and read a SQL file
	Without file name, edits current command buffer', NULL, NULL, 'c_pg');
(720, 'daves', '1994-07-26 19:55:33', 17, 61, '61', NULL, NULL, 'c_pg');
(721, 'daves', '1994-07-26 19:55:33', 17, 28, 'HELP -- Displays this menu', NULL, NULL, 'c_pg');
(722, 'daves', '1994-07-26 19:55:33', 17, 25, 'INput <filename> -- enter a named SQL file', NULL, NULL, 'c_pg');
(723, 'daves', '1994-07-26 19:55:33', 17, 26, 'OUTput [<filename>] -- write output to named file
	Without file name, returns output to stdout', NULL, NULL, 'c_pg');
(724, 'daves', '1994-07-26 19:55:33', 17, 62, '62', NULL, NULL, 'c_pg');
(725, 'daves', '1994-07-26 19:55:33', 17, 27, 'SHELL <shell command> -- execute command shell', NULL, NULL, 'c_pg');
(726, 'daves', '1994-07-26 19:55:33', 17, 37, 'SHOW <object type> [<object name>] -- displays information on metadata', NULL, NULL, 'c_pg');
(727, 'daves', '1994-07-26 19:55:33', 17, 38, '  <object type> = ''DB'', ''TABLE'', ''PROCedure'', ''INDEX'',
   ''GRANT'', ''DOMAIN'', ''VERSION''', NULL, NULL, 'c_pg');
(728, 'daves', '1994-07-26 19:55:33', 17, 64, '64', NULL, NULL, 'c_pg');
(729, 'daves', '1994-07-26 19:55:33', 17, 29, 'Set commands:', NULL, NULL, 'c_pg');
(730, 'daves', '1994-07-26 19:55:33', 17, 30, '	SET -- Display current options', NULL, NULL, 'c_pg');
(731, 'daves', '1994-07-26 19:55:33', 17, 31, '	SET AUTOddl -- toggle autocommit of DDL statements', NULL, NULL, 'c_pg');
(732, 'daves', '1994-07-26 19:55:33', 17, 32, '	SET BLOBdisplay [ALL|N]-- Display BLOBS of type N
	 SET BLOB turns off BLOB display', NULL, NULL, 'c_pg');
(733, 'daves', '1994-07-26 19:55:33', 17, 65, '65', NULL, NULL, 'c_pg');
(734, 'daves', '1994-07-26 19:55:33', 17, 33, '	SET COUNT  -- toggle count of selected rows on/off', NULL, NULL, 'c_pg');
(735, 'daves', '1994-07-26 19:55:33', 17, 34, '	SET ECHO  -- toggle command echo on/off', NULL, NULL, 'c_pg');
(736, 'daves', '1994-07-26 19:55:33', 17, 52, '	SET LIST -- toggles column or table display of data', NULL, NULL, 'c_pg');
(737, 'daves', '1994-07-26 19:55:33', 17, 63, '63', NULL, NULL, 'c_pg');
(738, 'daves', '1994-07-26 19:55:33', 17, 59, '	SET PLAN  -- Toggle display of query access plan', NULL, NULL, 'c_pg');
(739, 'daves', '1994-07-26 19:55:33', 17, 35, '	SET STATs -- toggles performance statistics display', NULL, NULL, 'c_pg');
(740, 'daves', '1994-07-26 19:55:33', 17, 60, '/tSET TIME -- Toggle display of timestamp with DATE values', NULL, NULL, 'c_pg');
(741, 'daves', '1994-07-26 19:55:33', 17, 36, '	SET TERM <string> -- changes termination charter', NULL, NULL, 'c_pg');
(742, 'daves', '1994-07-26 19:55:33', 17, 58, '	SET WIDTH <column name> [<width>] -- sets/unsets print width
	 for column alias name', NULL, NULL, 'c_pg');
(743, 'daves', '1994-07-26 19:55:33', 17, 39, 'EXIT -- Exit program and commit changes', NULL, NULL, 'c_pg');
(744, 'daves', '1994-07-26 19:55:33', 17, 40, 'QUIT -- Exit program and roll back changes', NULL, NULL, 'c_pg');
(745, 'daves', '1994-07-26 19:55:33', 17, 41, 'All commands may be abbreviated to letters in CAPs', NULL, NULL, 'c_pg');
(746, 'daves', '1994-07-26 20:37:20', 17, 30, '    SET                    -- display current SET options', NULL, NULL, 'c_pg');
(747, 'daves', '1994-07-26 20:37:21', 17, 30, 'SET      <option>          -- (Use HELP SET for complete list)', NULL, NULL, 'c_pg');
(748, 'daves', '1994-07-26 20:39:05', 17, 66, '  SET    <command>        -- (use HELP SET for complete list)', NULL, NULL, 'c_pg');
(749, 'dperiwal', '1994-08-08 18:43:33', 15, 302, 'Unexpected latch timeout', NULL, NULL, 'c_pg');
(750, 'klaus', '1994-08-15 09:48:16', 18, 26, '    user name       uid   gid     full name', NULL, NULL, 'c_pg');
(751, 'klaus', '1994-08-15 09:48:58', 18, 27, '---------------------------------------------------------------------------', NULL, NULL, 'c_pg');
(752, 'klaus', '1994-08-16 11:54:13', 18, 77, 'Invalid password (maximum 16 bytes allowed)', NULL, NULL, 'c_pg');
(753, 'caro', '1994-09-07 14:41:07', 15, 215, 'page lock conversion denied', NULL, NULL, 'c_pg');
(754, 'caro', '1994-09-07 14:41:43', 15, 216, 'page lock denied', NULL, NULL, 'c_pg');
(755, 'andreww', '1994-10-05 12:33:20', 17, 53, '%s not found', NULL, NULL, 'c_pg');
(756, 'andreww', '1994-10-05 14:43:33', 17, 53, 'There is no %s in this database', NULL, NULL, 'c_pg');
(757, 'andreww', '1994-10-06 12:06:01', 17, 53, 'No %s in this database', NULL, NULL, 'c_pg');
(758, 'andreww', '1994-10-06 12:49:47', 17, 89, 'There is no triggers on relation %s in this database', NULL, NULL, 'c_pg');
(759, 'andreww', '1994-10-06 13:09:43', 17, 71, 'There is no indices on relation %s in this database', NULL, NULL, 'c_pg');
(760, 'andreww', '1994-10-06 13:28:03', 17, 81, 'There is no user-defined functions in this database', NULL, NULL, 'c_pg');
(761, 'andreww', '1994-10-06 14:39:47', 17, 71, 'There are no indices on relation %s in this database', NULL, NULL, 'c_pg');
(762, 'andreww', '1994-10-06 14:40:14', 17, 72, 'There is no relation or index %s in this database', NULL, NULL, 'c_pg');
(763, 'andreww', '1994-10-06 14:40:45', 17, 84, 'There is no granted privilege on relation %s in this database', NULL, NULL, 'c_pg');
(764, 'andreww', '1994-10-06 14:41:06', 17, 86, 'There is no relation or stored procedure %s in this database', NULL, NULL, 'c_pg');
(765, 'andreww', '1994-10-06 14:41:35', 17, 89, 'There are no triggers on relation %s in this database', NULL, NULL, 'c_pg');
(766, 'andreww', '1994-10-06 14:42:08', 17, 90, 'There is no relation or trigger %s in this database', NULL, NULL, 'c_pg');
(767, 'andreww', '1994-10-06 14:42:41', 17, 92, 'There are no check constraints on relation %s in this database', NULL, NULL, 'c_pg');
(768, 'andreww', '1994-10-06 14:57:59', 17, 85, 'There is no granted privilege on stored procedure %s in this database', NULL, NULL, 'c_pg');
(769, 'SYSDBA', '1994-10-14 18:25:05', 0, 384, 'Network lookup failure for host "%s".', NULL, NULL, 'c_pg');
(770, 'deej', '1994-10-15 15:46:45', 0, 345, 'violation of PRIMARY or UNIQUE KEY constraint: "%s"', NULL, NULL, 'c_pg');
(771, 'deej', '1994-10-15 15:58:18', 13, 470, 'violation of FOREIGN KEY constraint: "%s"', NULL, NULL, 'c_pg');
(772, 'deej', '1994-10-15 16:02:21', 13, 470, 'violation of FOREIGN KEY constraint "%s" on table "%s"', NULL, NULL, 'c_pg');
(773, 'deej', '1994-10-15 16:02:40', 0, 146, 'violation of FOREIGN KEY constraint: "%s"', NULL, NULL, 'c_pg');
(774, 'deej', '1994-10-15 16:13:07', 0, 238, 'Operation violates CHECK constraint %s on view or table', NULL, NULL, 'c_pg');
(775, 'deej', '1994-10-15 16:13:22', 0, 238, 'Operation violates CHECK constraint %s on view or table %s', NULL, NULL, 'c_pg');
(776, 'SYSDBA', '1994-10-15 18:35:55', 20, 50, 'Not enough event control blocks available.', NULL, NULL, 'c_pg');
(777, 'andreww', '1994-10-17 16:10:39', 12, 234, 'Skipped one bad security class entry %s', NULL, NULL, 'c_pg');
(778, 'SYSDBA', '1994-10-19 16:03:03', 20, 15, 'Insufficient buffer space or sockets are available.', NULL, NULL, 'c_pg');
(779, 'SYSDBA', '1994-10-19 16:04:05', 20, 16, 'The socket is already connected.', NULL, NULL, 'c_pg');
(780, 'SYSDBA', '1994-10-19 16:04:42', 20, 17, 'The socket is not connected.', NULL, NULL, 'c_pg');
(781, 'SYSDBA', '1994-10-19 16:05:24', 20, 18, 'Cannot send or receive data on a socket which has been shutdown.', NULL, NULL, 'c_pg');
(782, 'SYSDBA', '1994-10-19 16:06:09', 20, 19, 'Request timed out without establishing a connection.', NULL, NULL, 'c_pg');
(783, 'SYSDBA', '1994-10-19 16:07:42', 20, 20, 'The connection requested was refused.  This can occur if the InterBase server is not started on the remote machine.', NULL, NULL, 'c_pg');
(784, 'deej', '1994-10-25 18:55:22', 0, 238, 'Operation violates CHECK constraint "%s" on view or table "%s"', NULL, NULL, 'c_pg');
(785, 'andreww', '1994-10-27 13:56:29', 12, 236, 'Converted V3 SUB_TYPE: %s to CHARACTER_SET_ID: %d and COLLATE_ID: %s.', NULL, NULL, 'c_pg');
(786, 'andreww', '1994-10-28 14:33:03', 12, 236, 'Converted V3 SUB_TYPE: %d to CHARACTER_SET_ID: %d and COLLATE_ID: %d.', NULL, NULL, 'c_pg');
(787, 'daves', '1994-10-30 22:39:46', 12, 239, '', NULL, NULL, 'c_pg');
(788, 'daves', '1994-10-30 22:45:30', 12, 239, '	%sNT            xxxxxx', NULL, NULL, 'c_pg');
(789, 'christy', '1994-10-31 14:12:12', 0, 386, 'Host unknown', NULL, NULL, 'c_pg');
(790, 'christy', '1994-10-31 16:25:51', 20, 31, 'The connection to the remote host was terminated.', NULL, NULL, 'c_pg');
(791, 'christy', '1994-10-31 16:26:23', 20, 32, 'The remote host is not responding or has closed the connection.', NULL, NULL, 'c_pg');
(792, 'christy', '1994-10-31 16:30:27', 20, 29, 'Specified name not found.', NULL, NULL, 'c_pg');
(793, 'andreww', '1994-11-09 17:01:14', 12, 236, 'Converted V3 sub_type: %d to character_set_id: %d and callate_id: %d.', NULL, NULL, 'c_pg');
(794, 'builder', '1994-11-16 23:22:53', 20, 45, 'The remote host is not responding.', NULL, NULL, 'c_pg');
(795, 'klaus', '1995-01-16 09:07:54', 20, 71, 'Beginning database sweep.   This can be controlled by varying the sweep interval in the server manager.', NULL, NULL, 'c_pg');
(796, 'klaus', '1995-01-16 10:07:27', 0, 411, 'A sweep must be performed on this database before it can be started.', NULL, NULL, 'c_pg');
(797, 'jmayer', '1995-01-25 13:03:47', 0, 412, 'Access to network mounted file systems is not supported.', NULL, NULL, 'c_pg');
(798, 'jmayer', '1995-01-26 15:34:44', 0, 412, 'Access to drives on file servers is not supported.', NULL, NULL, 'c_pg');
(799, 'klaus', '1995-01-27 15:13:54', 20, 71, 'Beginning database sweep.   This can be controlled by varying the sweep interval in the server manager.   Continue?', NULL, NULL, 'c_pg');
(800, 'klaus', '1995-01-27 15:14:54', 20, 72, 'A sweep must be performed on this database with server manager before it can be attached.', NULL, NULL, 'c_pg');
(801, 'klaus', '1995-01-27 15:15:35', 20, 71, 'Beginning database sweep.   This can take a long time, depending on the database size.   This can be controlled by var', NULL, NULL, 'c_pg');
(802, 'klaus', '1995-01-27 15:16:01', 20, 71, 'Beginning database sweep.   This can take a long time, depending on the', NULL, NULL, 'c_pg');
(803, 'klaus', '1995-01-27 15:16:34', 20, 71, 'Beginning database sweep, which can take a long time.   This can be controlled by varying the sweep interval in the se', NULL, NULL, 'c_pg');
(804, 'klaus', '1995-01-27 15:17:38', 20, 71, 'Beginning database sweep, which can take a long time.   This can be controlled by varying the sweep interval.   Sweep?', NULL, NULL, 'c_pg');
(805, 'klaus', '1995-01-27 15:18:15', 20, 71, '', NULL, NULL, 'c_pg');
(806, 'klaus', '1995-01-27 15:19:40', 20, 72, 'A sweep must be performed on this database before it can be started.   This can take a long time, depending on the dat', NULL, NULL, 'c_pg');
(807, 'thakur', '1995-03-07 14:34:49', 0, 94, 'BLOB and array data types are not supported for %s operation', NULL, NULL, 'c_pg');
(808, 'beck', '1995-04-05 16:16:09', 0, 24, 'I/O error during "%s" operation for file "%s"', 'Check secondary messages for more information.  The
problem may be an obvious one, such as incorrect file name or
a file protection problem.  If that does not eliminate the
problem, check your program logic.  To avoid errors when
the user enters a database name interactively,
add an error handler to the statement that causes this
message to appear.', 'Your program encountered an input or output error.', 'c_pg');
(809, 'beck', '1995-04-07 11:50:01', 0, 24, 'I/O error for filename[D[or for file %.0s"%s"', 'Check secondary messages for more information.  The
problem may be an obvious one, such as incorrect file name or
a file protection problem.  If that does not eliminate the
problem, check your program logic.  To avoid errors when
the user enters a database name interactively,
add an error handler to the statement that causes this
message to appear.', 'Your program encountered an input or output error.', 'c_pg');
(810, 'beck', '1995-04-07 13:51:13', 0, 413, 'Error while trying to create file.', NULL, NULL, 'c_pg');
(811, 'beck', '1995-04-07 13:51:49', 0, 415, 'Error while trying to close file.', NULL, NULL, 'c_pg');
(812, 'beck', '1995-04-07 13:52:02', 0, 416, 'Error while trying to read from file.', NULL, NULL, 'c_pg');
(813, 'beck', '1995-04-07 13:52:54', 0, 417, 'Error while trying to write to file.', NULL, NULL, 'c_pg');
(814, 'beck', '1995-04-07 13:53:28', 0, 418, 'Error while trying to delete file.', NULL, NULL, 'c_pg');
(815, 'beck', '1995-04-07 13:53:44', 0, 419, 'Error while trying to access file.', NULL, NULL, 'c_pg');
(816, 'beck', '1995-04-07 13:54:16', 0, 414, 'Error while trying to open file.', NULL, NULL, 'c_pg');
(817, 'beck', '1995-04-07 13:55:41', 20, 73, 'Not enough disk space remaining.', NULL, NULL, 'c_pg');
(818, 'daves', '1995-07-18 15:51:35', 0, 94, '', NULL, NULL, 'c_pg');
(819, 'klaus', '1995-11-03 16:23:39', 0, 420, 'Exception detected in blob filter or user defined function', NULL, NULL, 'c_pg');
(820, 'daves', '1996-08-06 14:18:03', 1, 294, '        An erase trigger is defined for %s', NULL, NULL, 'c_pg');
(821, 'daves', '1996-08-06 14:18:03', 1, 295, '        A modify trigger is defined for %s', NULL, NULL, 'c_pg');
(822, 'daves', '1996-08-06 14:18:03', 1, 296, '        A store trigger is defined for %s', NULL, NULL, 'c_pg');
(823, 'daves', '1996-08-06 14:18:03', 1, 298, '	Triggers for relation %s:', NULL, NULL, 'c_pg');
(824, 'daves', '1996-08-06 14:18:03', 1, 299, '    Source for the erase trigger is not available.  Trigger BLR:', NULL, NULL, 'c_pg');
(825, 'daves', '1996-08-06 14:18:03', 1, 300, '    Erase trigger for relation %s:', NULL, NULL, 'c_pg');
(826, 'daves', '1996-08-06 14:18:03', 1, 301, '    Source for the modify trigger is not available.  Trigger BLR:', NULL, NULL, 'c_pg');
(827, 'daves', '1996-08-06 14:18:03', 1, 302, '    Modify trigger for relation %s:', NULL, NULL, 'c_pg');
(828, 'daves', '1996-08-06 14:18:03', 1, 303, '    Source for the store trigger is not available.  Trigger BLR:', NULL, NULL, 'c_pg');
(829, 'daves', '1996-08-06 14:18:03', 1, 304, '    Store trigger for relation %s:', NULL, NULL, 'c_pg');
(830, 'daves', '1996-08-06 14:18:03', 1, 305, '    Triggers for relation %s:', NULL, NULL, 'c_pg');
(831, 'daves', '1996-08-06 14:18:03', 1, 306, '    Source for the erase trigger is not available.  Trigger BLR:', NULL, NULL, 'c_pg');
(832, 'daves', '1996-08-06 14:18:03', 1, 307, '    Erase trigger for relation %s:', NULL, NULL, 'c_pg');
(833, 'daves', '1996-08-06 14:18:03', 1, 308, '    Source for the modify trigger is not available.  Trigger BLR:', NULL, NULL, 'c_pg');
(834, 'daves', '1996-08-06 14:18:03', 1, 309, '    Modify trigger for relation %s:', NULL, NULL, 'c_pg');
(835, 'daves', '1996-08-06 14:18:03', 1, 310, '    Source for the store trigger is not available.  Trigger BLR:', NULL, NULL, 'c_pg');
(836, 'daves', '1996-08-06 14:18:03', 1, 311, '    Store trigger for relation %s:', NULL, NULL, 'c_pg');
(837, 'builder', '1996-09-24 12:09:16', 12, 241, 'The unique index has duplicate values or NULLs.', NULL, NULL, 'c_pg');
(838, 'builder', '1996-09-24 12:13:23', 12, 242, 'Delete or Update duplicate values or NULLs, and activate index with', NULL, NULL, 'c_pg');
(839, 'builder', '1996-09-24 12:13:48', 12, 243, 'ALTER INDEX "%s" ACTIVE;', NULL, NULL, 'c_pg');
(840, 'builder', '1996-09-24 12:15:29', 12, 244, 'Not enough disk space to create the sort file for an index.', NULL, NULL, 'c_pg');
(841, 'builder', '1996-09-24 12:16:13', 12, 245, 'Set the TMP environment variable to a directory on a filesystem that does have enough space, and activate index with', NULL, NULL, 'c_pg');
(842, 'smistry', '1996-10-01 15:37:23', 8, 182, 'SELECT RDB$RELATIONS failed in grantor_can_grant(1)', NULL, NULL, 'c_pg');
(843, 'smistry', '1996-10-01 15:38:09', 8, 183, 'SELECT RDB$RELATION_FIELDS failed in grantor_can_grant(2)', NULL, NULL, 'c_pg');
(844, 'smistry', '1996-10-01 15:38:41', 8, 184, 'SELECT RDB$RELATIONS/RDB$OWNER_NAME failed in grantor_can_grant(3)', NULL, NULL, 'c_pg');
(845, 'smistry', '1996-10-01 15:40:10', 8, 185, 'SELECT RDB$USER_PRIVILEGES failed in grantor_can_grant(4)', NULL, NULL, 'c_pg');
(846, 'smistry', '1996-10-01 15:40:41', 8, 186, 'SELECT RDB$VIEW_RELATIONS/RDB$RELATION_FIELDS/... failed in grantor_can_grant(5)', NULL, NULL, 'c_pg');
(847, 'daves', '1996-11-07 13:44:18', 8, 187, 'field %s from relation %s is referenced in index %s', NULL, NULL, 'c_pg');
(848, 'bietie', '1996-11-14 15:28:54', 8, 191, 'ERASE RDB$ROLES failed', NULL, NULL, 'c_pg');
(849, 'bietie', '1996-12-03 14:23:46', 12, 249, 'write_sql_roles', NULL, NULL, 'c_pg');
(850, 'bietie', '1996-12-03 14:24:44', 12, 249, 'write_sql_role %s', NULL, NULL, 'c_pg');
(851, 'bietie', '1996-12-03 14:25:41', 12, 249, 'write sql role: %s', NULL, NULL, 'c_pg');
(852, 'bietie', '1996-12-03 14:27:21', 12, 250, 'Bad attribute for restore SQL role', NULL, NULL, 'c_pg');
(853, 'bietie', '1996-12-04 09:07:13', 12, 252, '%sRO(LE)           InterBase SQL role', NULL, NULL, 'c_pg');
(854, 'bietie', '1996-12-04 09:09:17', 12, 252, '        %sRO(LE)           InterBase SQL role', NULL, NULL, 'c_pg');
(855, 'bietie', '1996-12-06 09:32:06', 8, 196, ':q', NULL, NULL, 'c_pg');
(856, 'CARO', '1997-02-11 11:43:48', 12, 257, '	%sBU(FFERS)	override page buffers default', NULL, NULL, 'c_pg');
(857, 'CARO', '1997-02-11 13:06:31', 12, 257, '	%sBU(FFERS)		override page buffers default', NULL, NULL, 'c_pg');
(858, 'CARO', '1997-02-11 13:17:11', 12, 260, 'page buffers is allowe only on restore or create', NULL, NULL, 'c_pg');
(859, 'CARO', '1997-02-12 12:39:53', 0, 371, 'Cache length too small', NULL, NULL, 'c_pg');
(860, 'DUQUETTE', '1997-06-11 10:37:28', 19, 17, 'IB_LICEN>', NULL, NULL, 'c_pg');
(861, 'SYSDBA', '1997-09-22 14:53:09', 12, 261, 'Starting witn volume #%d, "%s"', NULL, NULL, 'c_pg');
(862, 'ROMANINI', '1998-01-05 17:32:32', 19, 32, 'The certificate was not removed.  The key does not exist in the license file.', NULL, NULL, 'c_pg');
(863, 'SYSDBA', '1998-01-06 09:04:57', 19, 32, 'The certificate was not removed.  The key does not exist in the license file, or the key is for an evalutaion certific', NULL, NULL, 'c_pg');
(864, 'CARO', '1998-02-12 11:45:04', 0, 442, 'too many conjuncts used', NULL, NULL, 'c_pg');
(865, 'CARO', '1998-02-12 12:02:32', 0, 442, 'size of opt block exceeded', NULL, NULL, 'c_pg');
(866, 'bietie', '1998-04-07 10:33:55', 0, 443, 'a string constant is delimited by single quotes', NULL, NULL, 'c_pg');
(867, 'SYSDBA', '1998-06-08 15:45:49', 0, 420, 'Exception %d detected in blob filter or user defined function', NULL, NULL, 'c_pg');
(868, 'BUILDER', '1998-06-09 10:31:12', 0, 420, 'Exception %u detected in blob filter or user defined function', NULL, NULL, 'c_pg');
(869, 'SYSDBA', '1998-06-10 10:50:56', 0, 447, 'Exception %X detected in blob filter', NULL, NULL, 'c_pg');
(870, 'SYSDBA', '1998-06-10 10:51:43', 0, 420, 'Exception %X detected in user defined function', NULL, NULL, 'c_pg');
(871, 'SYSDBA', '1998-06-10 15:28:50', 0, 448, 'Access Violation', NULL, NULL, 'c_pg');
(872, 'SYSDBA', '1998-06-10 15:42:46', 0, 449, 'Datatype misalignment', NULL, NULL, 'c_pg');
(873, 'SYSDBA', '1998-06-10 15:43:47', 0, 450, 'Array bounds exceeded', NULL, NULL, 'c_pg');
(874, 'SYSDBA', '1998-06-10 15:45:32', 0, 451, 'Float denormal operand', NULL, NULL, 'c_pg');
(875, 'SYSDBA', '1998-06-10 15:47:32', 0, 452, 'Float divide by zero', NULL, NULL, 'c_pg');
(876, 'SYSDBA', '1998-06-10 15:48:40', 0, 453, 'Float inexcat result', NULL, NULL, 'c_pg');
(877, 'SYSDBA', '1998-06-10 15:52:19', 0, 454, 'Float invalid operand', NULL, NULL, 'c_pg');
(878, 'SYSDBA', '1998-06-10 15:53:51', 0, 455, 'Float overflow', NULL, NULL, 'c_pg');
(879, 'SYSDBA', '1998-06-10 15:54:14', 0, 454, 'Float invalid operand.  An indeterminant error occurred during a floating-point operation.', NULL, NULL, 'c_pg');
(880, 'SYSDBA', '1998-06-10 15:55:21', 0, 456, 'Float stack check', NULL, NULL, 'c_pg');
(881, 'SYSDBA', '1998-06-10 15:57:48', 0, 457, 'Float underflow', NULL, NULL, 'c_pg');
(882, 'SYSDBA', '1998-06-10 15:58:29', 0, 458, 'integer divide by zero', NULL, NULL, 'c_pg');
(883, 'SYSDBA', '1998-06-10 16:01:49', 0, 459, 'Integer overflow', NULL, NULL, 'c_pg');
(884, 'SYSDBA', '1998-06-10 16:05:56', 0, 460, 'Unknown exception %X', NULL, NULL, 'c_pg');
(885, 'SYSDBA', '1998-06-10 16:09:21', 0, 420, 'Fatal exception in uder defined function execution', NULL, NULL, 'c_pg');
(886, 'SYSDBA', '1998-06-10 16:10:08', 0, 447, 'Fatal exception in blob filter execution', NULL, NULL, 'c_pg');
(887, 'builder', '1998-07-17 13:39:34', 12, 273, 'host name parameter missing', NULL, NULL, 'c_pg');
(888, 'smistry', '1998-08-03 14:56:22', 0, 462, 'Segmentation Fault: code attempted to acces memory without priviledges', NULL, NULL, 'c_pg');
(889, 'smistry', '1998-08-03 14:57:07', 0, 462, 'Segmentation Fault: code attempted to access memory without priviledges', NULL, NULL, 'c_pg');
(890, 'smistry', '1998-08-04 18:16:12', 0, 448, 'Access violation.  The code attempted to access a virtual address without privelege to do so.', NULL, NULL, 'c_pg');
(891, 'SMISTRY', '1998-08-21 19:09:33', 0, 467, 'EXT_modify', NULL, NULL, 'c_pg');
(892, 'SMISTRY', '1998-08-21 19:10:19', 0, 466, 'Cannot delete rows from external files', NULL, NULL, 'c_pg');
(893, 'BUILDER', '1998-08-24 16:41:54', 0, 331, 'external file could not be opened for output', NULL, NULL, 'c_pg');
(894, 'DUQUETTE', '1998-09-06 11:03:51', 12, 1, 'found unknown switch', NULL, NULL, 'c_pg');
(895, 'DUQUETTE', '1998-09-06 11:08:42', 12, 5, 'conflicting switches for backup/restore', NULL, NULL, 'c_pg');
(896, 'DUQUETTE', '1998-09-06 11:09:11', 12, 5, 'conflictin switches for backup/restore', NULL, NULL, 'c_pg');
(897, 'duquette', '1998-09-08 15:46:08', 12, 1, 'found_unknown_switch', NULL, NULL, 'c_pg');
(898, 'duquette', '1998-09-08 15:53:16', 12, 25, 'Failed in put_blr_gen_id', NULL, NULL, 'c_pg');
(899, 'duquette', '1998-09-08 15:53:43', 12, 25, 'gbak_put_blr_gen_id_failed', NULL, NULL, 'c_pg');
(900, 'duquette', '1998-09-08 15:56:13', 12, 29, 'gds_$receive failed', NULL, NULL, 'c_pg');
(901, 'duquette', '1998-09-08 15:56:48', 12, 31, 'gds_$database_info failed', NULL, NULL, 'c_pg');
(902, 'duquette', '1998-09-08 16:07:00', 12, 47, 'warning -- record could not be restored', NULL, NULL, 'c_pg');
(903, 'duquette', '1998-09-08 16:12:16', 12, 200, 'missing parameter for the number of bytes to be skipped', NULL, NULL, 'c_pg');
(904, 'duquette', '1998-09-08 16:16:24', 12, 265, 'standard input is not supported when using join operation', NULL, NULL, 'c_pg');
(905, 'duquette', '1998-09-08 16:22:43', 12, 273, 'service name parameter missing', NULL, NULL, 'c_pg');
(906, 'duquette', '1998-09-08 16:23:02', 12, 273, ' service name parameter missing', NULL, NULL, 'c_pg');
(907, 'SYSDBA', '1998-09-29 12:46:38', 0, 451, 'Float denormal operand.  One of the floating-point operands is too small to represent as a standard floating-point val', NULL, NULL, 'c_pg');
(908, 'SYSDBA', '1998-09-29 12:47:14', 0, 452, 'Floating-point divide by zero.  The code attempted to divide a floating-point value by a floating-point divisor of zer', NULL, NULL, 'c_pg');
(909, 'SYSDBA', '1998-09-29 12:47:50', 0, 453, 'Floating-point inexact result.  The result of a floating-point operation cannot be represented exactly as a decimal fr', NULL, NULL, 'c_pg');
(910, 'SYSDBA', '1998-09-29 12:48:33', 0, 459, 'Interger overflow.  The result of an integer operation caused the most significant bit of the result to carry.', NULL, NULL, 'c_pg');
(911, 'SYSDBA', '1998-09-29 12:49:25', 0, 462, 'Segmentation Fault: code attempted to access memory without priviledges.', NULL, NULL, 'c_pg');
(912, 'SYSDBA', '1998-09-29 12:50:13', 0, 463, 'Illegal Instruction: code attempted to perfrom an illegal operation', NULL, NULL, 'c_pg');
(913, 'SYSDBA', '1998-09-29 12:50:32', 0, 464, 'Bus Error: code caused a system bus error', NULL, NULL, 'c_pg');
(914, 'SYSDBA', '1998-09-29 12:51:01', 0, 465, 'Floating Point Error: Code caused an Arithmetic Exception or a floating point exception', NULL, NULL, 'c_pg');
(915, 'duquette', '1998-10-15 17:17:38', 18, 15, 'gsec - unable to open database', NULL, NULL, 'c_pg');
(916, 'duquette', '1998-10-15 17:18:07', 18, 16, 'gsec - error in switch specifications', NULL, NULL, 'c_pg');
(917, 'duquette', '1998-10-15 17:18:30', 18, 17, 'gsec - no operation specified', NULL, NULL, 'c_pg');
(918, 'duquette', '1998-10-15 17:18:59', 18, 18, 'gsec - no user name specified', NULL, NULL, 'c_pg');
(919, 'duquette', '1998-10-15 17:19:22', 18, 19, 'gsec - add record error', NULL, NULL, 'c_pg');
(920, 'duquette', '1998-10-15 17:19:55', 18, 20, 'gsec - modify record error', NULL, NULL, 'c_pg');
(921, 'duquette', '1998-10-15 17:21:37', 18, 21, 'gsec - find/modify record error', NULL, NULL, 'c_pg');
(922, 'duquette', '1998-10-15 17:22:14', 18, 22, 'gsec - record not found for user:', NULL, NULL, 'c_pg');
(923, 'duquette', '1998-10-15 17:22:28', 18, 23, 'gsec - delete record error', NULL, NULL, 'c_pg');
(924, 'duquette', '1998-10-15 17:22:52', 18, 24, 'gsec - find/delete record error', NULL, NULL, 'c_pg');
(925, 'duquette', '1998-10-15 17:23:14', 18, 28, 'gsec - find/display record error', NULL, NULL, 'c_pg');
(926, 'duquette', '1998-10-15 17:23:41', 18, 29, 'gsec - invalid parameter, no switch defined', NULL, NULL, 'c_pg');
(927, 'duquette', '1998-10-15 17:24:00', 18, 30, 'gsec - operation already specified', NULL, NULL, 'c_pg');
(928, 'duquette', '1998-10-15 17:24:45', 18, 31, 'gsec - password already specified', NULL, NULL, 'c_pg');
(929, 'duquette', '1998-10-15 17:25:00', 18, 32, 'gsec - uid already specified', NULL, NULL, 'c_pg');
(930, 'duquette', '1998-10-15 17:25:44', 18, 34, 'gsec - project already specified', NULL, NULL, 'c_pg');
(931, 'duquette', '1998-10-15 17:25:58', 18, 35, 'gsec - organization already specified', NULL, NULL, 'c_pg');
(932, 'duquette', '1998-10-15 17:28:44', 18, 36, 'gsec - first_name already specified', NULL, NULL, 'c_pg');
(933, 'duquette', '1998-10-15 17:28:58', 18, 37, 'gsec - middle_name already specified', NULL, NULL, 'c_pg');
(934, 'duquette', '1998-10-15 17:29:16', 18, 38, 'gsec - last_name already specified', NULL, NULL, 'c_pg');
(935, 'duquette', '1998-10-15 17:30:19', 18, 40, 'gsec - invalid switch specified', NULL, NULL, 'c_pg');
(936, 'duquette', '1998-10-15 17:30:39', 18, 41, 'gsec - ambiguous switch specified', NULL, NULL, 'c_pg');
(937, 'duquette', '1998-10-15 17:30:57', 18, 42, 'gsec - no operation specified for parameters', NULL, NULL, 'c_pg');
(938, 'duquette', '1998-10-15 17:31:12', 18, 43, 'gsec - no parameters allowed for this operation', NULL, NULL, 'c_pg');
(939, 'duquette', '1998-10-15 17:31:33', 18, 44, 'gsec - incompatible switches specified', NULL, NULL, 'c_pg');
(940, 'duquette', '1998-10-15 17:32:45', 18, 78, 'gsec - database already specified', NULL, NULL, 'c_pg');
(941, 'duquette', '1998-10-15 17:33:08', 18, 79, 'gsec - database administrator name already specified', NULL, NULL, 'c_pg');
(942, 'duquette', '1998-10-15 17:33:25', 18, 80, 'gsec - database administrator password already specified', NULL, NULL, 'c_pg');
(943, 'duquette', '1998-10-15 17:33:40', 18, 81, 'gsec - SQL role name already specified', NULL, NULL, 'c_pg');
(944, 'duquette', '1998-10-15 17:42:15', 19, 32, 'The certificate was not removed.  The key does not exist or corresponds to a temperary evaluation license.', NULL, NULL, 'c_pg');
(945, 'duquette', '1998-10-15 17:42:56', 19, 33, 'Error occurred updating the license file.  Operation cancelled.', NULL, NULL, 'c_pg');
(946, 'duquette', '1998-10-15 17:43:47', 19, 33, '    Text: Error occurred updating the license f', NULL, NULL, 'c_pg');
(947, 'DUQUETTE', '1998-11-04 10:57:12', 12, 250, 'Bad attribute for restoring SQL role', NULL, NULL, 'c_pg');
(948, 'DUQUETTE', '1998-11-04 11:04:57', 18, 36, 'first_name already specified', NULL, NULL, 'c_pg');
(949, 'DUQUETTE', '1998-11-04 11:05:23', 18, 37, 'middle_name already specified', NULL, NULL, 'c_pg');
(950, 'DUQUETTE', '1998-11-04 11:05:45', 18, 38, 'last_name already specified', NULL, NULL, 'c_pg');
(951, 'DUQUETTE', '1998-11-04 11:06:15', 18, 33, 'gsec - gid already specified', NULL, NULL, 'c_pg');
(952, 'DUQUETTE', '1998-11-04 11:19:49', 3, 108, '        Transaction description item unknown.', NULL, NULL, 'c_pg');
(953, 'DUQUETTE', '1998-11-04 11:35:38', 8, 192, '%s is a SQL role not an user', NULL, NULL, 'c_pg');
(954, 'DUQUETTE', '1998-11-04 11:36:58', 8, 194, 'SQL role %s already exist', NULL, NULL, 'c_pg');
(955, 'DUQUETTE', '1998-11-04 11:39:11', 8, 195, 'keyword %s could not be used as SQL role name', NULL, NULL, 'c_pg');
(956, 'DUQUETTE', '1998-11-04 11:40:35', 8, 196, 'SQL role is not supported on older version of database. back up and restore database is required', NULL, NULL, 'c_pg');
(957, 'DUQUETTE', '1998-11-11 16:05:02', 12, 191, '	%sUSER                 InterBase user name', NULL, NULL, 'c_pg');
(958, 'DUQUETTE', '1998-11-11 16:05:41', 12, 276, '%sUSE_ALL_SPACE Do not reserve space on data pages for record versions', NULL, NULL, 'c_pg');
(959, 'DUQUETTE', '1998-11-11 16:08:04', 12, 191, '', NULL, NULL, 'c_pg');
(960, 'DUQUETTE', '1998-11-11 16:08:29', 12, 274, 'Cannot restore over current database, must be sysdba or owner of the existing database', NULL, NULL, 'c_pg');
(961, 'DUQUETTE', '1998-11-11 16:09:26', 12, 276, '	%sUSE_ALL_SPACE Do not reserve space on data pages for record versions', NULL, NULL, 'c_pg');
(962, 'DUQUETTE', '1998-11-11 16:09:41', 12, 191, '	%sUSER InterBase user name', NULL, NULL, 'c_pg');
(963, 'DUQUETTE', '1998-11-11 16:13:03', 12, 104, '	%sZ                    print version number', NULL, NULL, 'c_pg');
(964, 'DUQUETTE', '1998-11-11 16:13:42', 12, 191, '	%sUSER	InteBase user name', NULL, NULL, 'c_pg');
(965, 'DUQUETTE', '1998-11-11 16:14:27', 12, 276, '	%sUSE_(ALL_SPACE)	Do not reserve space on data pages for record versions', NULL, NULL, 'c_pg');
(966, 'DUQUETTE', '1998-11-11 16:16:56', 12, 191, '	%sUSER                InterBase user name', NULL, NULL, 'c_pg');
(967, 'DUQUETTE', '1998-11-11 16:17:48', 12, 104, '	%sZ                   print version number', NULL, NULL, 'c_pg');
(968, 'DUQUETTE', '1998-11-11 16:18:56', 12, 276, '	%sUSE_(ALL_SPACE) Do not reserve space for record versions', NULL, NULL, 'c_pg');
(969, 'DUQUETTE', '1998-11-11 16:21:31', 12, 276, '	%sUSE_(ALL_SPACE)       Do not reserve space for record versions on restore', NULL, NULL, 'c_pg');
(970, 'DUQUETTE', '1998-11-11 16:23:14', 12, 276, '	%sUSE_(ALL_SPACE)       do not reserve space for record versions', NULL, NULL, 'c_pg');
(971, 'chrisj', '1998-12-10 11:44:50', 0, 377, 'Precision should be greater than 0', NULL, NULL, 'c_pg');
(972, 'BSRIRAM', '1998-12-30 10:24:45', 3, 109, '-mode		read_only or read_write', NULL, NULL, 'c_pg');
(973, 'SYSDBA', '1999-01-06 15:13:12', 8, 197, 'Can not rename domain %s to %s.  A domain with that name already exists.', NULL, NULL, 'c_pg');
(974, 'SYSDBA', '1999-01-06 15:13:33', 8, 198, 'Can not rename field %s to %s.  A field with that name already exists.', NULL, NULL, 'c_pg');
(975, 'SYSDBA', '1999-01-08 10:49:15', 7, 2, 'Cannot select DB_KEY from this type of object', NULL, NULL, 'c_pg');
(976, 'SYSDBA', '1999-01-08 10:52:36', 7, 2, 'Cannot SELECT DB_KEY from stored procedure', NULL, NULL, 'c_pg');
(977, 'builder', '1999-01-18 14:24:52', 0, 472, 'Cannot attach to services manager: server is not running', NULL, NULL, 'c_pg');
(978, 'builder', '1999-01-19 13:10:58', 17, 11, '	 [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
	[-page <pagelength>] [-n]', NULL, NULL, 'c_pg');
(979, 'duquette', '1999-01-21 15:42:58', 8, 209, 'Cannot convert base datatype %s to base datatype %s.', NULL, NULL, 'c_pg');
(980, 'duquette', '1999-01-21 15:47:34', 8, 207, 'Cannot change datatype for column %s.  Changing datatype is not supported for DATE, BLOB, or ARRAY columns.', NULL, NULL, 'c_pg');
(981, 'bsriram', '1999-01-21 16:18:57', 12, 278, '	%sMO(DE) <access>	"read_only" or "read_write" access', NULL, NULL, 'c_pg');
(982, 'duquette', '1999-01-21 19:24:29', 8, 206, 'Column %s is referenced in %s.', NULL, NULL, 'c_pg');
(983, 'duquette', '1999-01-21 19:24:48', 8, 210, 'Cannont change datatype for column %s from a character type to a non-character type.', NULL, NULL, 'c_pg');
(984, 'duquette', '1999-01-21 19:25:03', 8, 210, 'Cannot  change datatype for column %s from a character type to a non-character type.', NULL, NULL, 'c_pg');
(985, 'duquette', '1999-01-21 21:15:04', 8, 206, 'Column %s from table %s is referenced in %s.', NULL, NULL, 'c_pg');
(986, 'bietie', '1999-01-25 12:45:20', 0, 473, 'Metadata update statement is not allowed by the current database SQL dialect %s', NULL, NULL, 'c_pg');
(987, 'sprabhu', '1999-01-26 16:04:36', 17, 64, '               GRANT, INDEX, PROCEDURE, SYSTEM, TABLE, TRIGGER, VERSION, VIEW', NULL, NULL, 'c_pg');
(988, 'sprabhu', '1999-01-26 16:07:15', 17, 64, '               GRANT, INDEX, PROCEDURE, SYSTEM, TABLE, TRIGGER, VERSION, VIEW, ROLE', NULL, NULL, 'c_pg');
(989, 'sprabhu', '1999-01-26 16:49:12', 17, 64, '               GRANT, INDEX, PROCEDURE, SYSTEM, TABLE, TRIGGER, VERSION, VIEW', NULL, NULL, 'c_pg');
(990, 'sprabhu', '1999-01-28 15:55:46', 17, 102, '              ROLE', NULL, NULL, 'c_pg');
(991, 'sprabhu', '1999-01-28 15:57:15', 17, 102, '                ROLE', NULL, NULL, 'c_pg');
(992, 'sprabhu', '1999-01-29 09:57:42', 17, 64, '               GRANT, INDEX, PROCEDURE, SYSTEM, TABLE, TRIGGER, VERSION, VIEW,', NULL, NULL, 'c_pg');
(993, 'sprabhu', '1999-01-29 09:57:59', 17, 102, '               ROLE', NULL, NULL, 'c_pg');
(994, 'DUQUETTE', '1999-02-19 16:58:46', 0, 468, 'Cannot backup/restore database must be sysdba or owner of the database', NULL, NULL, 'c_pg');
(995, 'DUQUETTE', '1999-02-19 17:01:30', 0, 468, 'Unable to perform operation.  In order to perform administrative tasks you must be either SYSDBA or owner of the datab', NULL, NULL, 'c_pg');
(996, 'DUQUETTE', '1999-02-19 17:40:04', 0, 468, '', NULL, NULL, 'c_pg');
(997, 'sprabhu', '1999-02-24 16:26:00', 17, 11, '     [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
     [-page <pagelength>] [-n] -nowarnings', NULL, NULL, 'c_pg');
(998, 'sprabhu', '1999-02-25 16:16:28', 17, 11, '     [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
     [-page <pagelength>] [-n] [-s <sql dialect>] -nowarning', NULL, NULL, 'c_pg');
(999, 'sprabhu', '1999-02-25 16:18:10', 17, 11, '      [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
     [-page <pagelength>] [-n] [-m] [-q] [-s <sql dialect>]', NULL, NULL, 'c_pg');
(1000, 'DUQUETTE', '1999-03-01 10:51:37', 21, 33, '-p      password', NULL, NULL, 'c_pg');
(1001, 'DUQUETTE', '1999-03-01 10:51:56', 21, 32, '-u        username (8)', NULL, NULL, 'c_pg');
(1002, 'DUQUETTE', '1999-03-01 10:54:23', 21, 33, '     -p      password', NULL, NULL, 'c_pg');
(1003, 'DUQUETTE', '1999-03-01 10:54:37', 21, 32, '     -u      username', NULL, NULL, 'c_pg');
(1004, 'builder', '1999-03-05 16:09:12', 0, 475, 'unexpected service parameter block: expected %d, encountered %d', NULL, NULL, 'c_pg');
(1005, 'chrisj', '1999-03-08 12:41:51', 0, 378, 'Scale cannot be greater than precision', NULL, NULL, 'c_pg');
(1006, 'DUQUETTE', '1999-03-10 15:12:31', 8, 208, 'New size specified for column %s must be greater than %d characters.', NULL, NULL, 'c_pg');
(1007, 'DUQUETTE', '1999-03-10 15:28:11', 8, 208, 'New size specified for column %s must be atleast %d characters.', NULL, NULL, 'c_pg');
(1008, 'DUQUETTE', '1999-03-22 09:15:17', 8, 209, 'Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.', NULL, NULL, 'c_pg');
(1009, 'stsikin', '1999-03-26 16:23:52', 17, 64, '               GRANT, INDEX, PROCEDURE, ROLE, SYSTEM, TABLE, TRIGGER, VERSION,', NULL, NULL, 'c_pg');
(1010, 'stsikin', '1999-03-26 16:25:03', 17, 102, '               VIEW', NULL, NULL, 'c_pg');
(1011, 'ROMANINI', '1999-04-01 09:39:02', 0, 478, 'Request failed because of a dependency on yet uncommited relation.', NULL, NULL, 'c_pg');
(1012, 'ROMANINI', '1999-04-13 15:40:54', 19, 58, 'Custom license.', NULL, NULL, 'c_pg');
(1013, 'bietie', '1999-05-21 10:48:57', 3, 111, '	-SQL_dialect	set database dialect n', NULL, NULL, 'c_pg');
(1014, 'bietie', '1999-05-21 11:08:48', 0, 444, 'DATE must be changed to SQLDATE', NULL, NULL, 'c_pg');
(1015, 'bietie', '1999-05-21 11:38:46', 3, 111, '-set_db_SQL_dialect        set database dialect n', NULL, NULL, 'c_pg');
(1016, 'bietie', '1999-05-21 11:47:34', 3, 111, '   -set_db_SQL_dialect     set database dialect n', NULL, NULL, 'c_pg');
(1017, 'bietie', '1999-05-21 11:57:29', 3, 111, '	-set_db_SQL_dialect	set database dialect n', NULL, NULL, 'c_pg');
(1018, 'bietie', '1999-05-21 11:59:15', 3, 111, '				-set_db_SQL_dialect	set database dialect n', NULL, NULL, 'c_pg');
(1019, 'bietie', '1999-05-21 12:01:00', 3, 111, '	-set_db_SQL_dialect	set database dialect n', NULL, NULL, 'c_pg');
(1020, 'bietie', '1999-05-21 12:03:14', 3, 111, '   -set_db_SQL_dialect     set database dialect n', NULL, NULL, 'c_pg');
(1021, 'bietie', '1999-05-21 12:08:22', 3, 111, '	-set_db_SQL_dialect	set database dialect n bie', NULL, NULL, 'c_pg');
(1022, 'bietie', '1999-05-24 09:30:59', 3, 111, '	-set_db_SQL_dialect	set database dialect n', NULL, NULL, 'c_pg');
(1023, 'bietie', '1999-05-24 09:40:01', 3, 111, '      -SQL_dialect    set database dialect n', NULL, NULL, 'c_pg');
(1024, 'bietie', '1999-06-21 11:09:16', 3, 111, '	-SQL_dialect	set database dialect n', NULL, NULL, 'c_pg');
(1028, 'bietie', '1999-06-21 11:57:45', 17, 11, '     [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
     [-page <pagelength>] [-n] [-m] [-q] [-s <sql dialect>]', NULL, NULL, 'c_pg');
(1029, 'bietie', '1999-06-21 12:08:46', 17, 11, '	[-x|-a] [-d <target db>] [-u <user>] [-p <password>]
	[-page <pagelength>] [-n] [-m] [-q] [-s <sql_dialect>]', NULL, NULL, 'c_pg');
(1030, 'bietie', '1999-06-21 12:12:31', 17, 11, '     [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
     [-page <pagelength>] [-n] [-m] [-q] [-s <sql dialect>]', NULL, NULL, 'c_pg');
(1031, 'bietie', '1999-06-21 13:41:07', 17, 11, '     [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
     [-page <pagelength>] [-n] [-m] [-q] [-s <sql_dialect>]', NULL, NULL, 'c_pg');
(1032, 'bietie', '1999-06-21 13:45:55', 17, 11, '', NULL, NULL, 'c_pg');
(1033, 'bietie', '1999-06-21 13:46:54', 17, 11, '[-x|-a] [-d <target db>] [-u <user>] [-p <password>]
  [-page <pagelength>] [-n] [-m] [-q] [-s <sql_dialect> n]', NULL, NULL, 'c_pg');
(1034, 'bietie', '1999-06-21 13:51:16', 17, 11, '     [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
   [-page <pagelength>] [-n] [-m] [-q] [-s <sql_dialect> n]', NULL, NULL, 'c_pg');
(1035, 'bietie', '1999-06-21 13:53:11', 17, 11, '    [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
     [-page <pagelength>] [-n] [-m] [-q] [-s <sql_dialect>]', NULL, NULL, 'c_pg');
(1036, 'DUQUETTE', '1999-06-25 14:38:14', 0, 483, 'Unable to set database dialect to %d.  Invalid dialect value.', NULL, NULL, 'c_pg');
(1037, 'BUILDER', '1999-06-28 07:15:01', 0, 485, 'Database dialect %d is not a valid dialect.  Valid database dialects are %s', NULL, NULL, 'c_pg');
(1038, 'duquette', '1999-07-27 06:37:59', 0, 444, 'DATE must be changed to SQL DATE', NULL, NULL, 'c_pg');
(1039, 'chrisj', '1999-07-28 17:40:42', 7, 8, 'Numeric literal %s is interpreted as a floating-point value in', NULL, NULL, 'c_pg');
(1040, 'chrisj', '1999-07-28 17:41:42', 7, 9, 'SQL dialect 1, but as an exact numeric value in SQL dialect 3.', NULL, NULL, 'c_pg');
(1041, 'chrisj', '1999-07-29 14:51:05', 7, 11, 'as approximate floating values in SQL dialect 1, but as 64-bit integers', NULL, NULL, 'c_pg');
(1042, 'chrisj', '1999-07-29 14:51:24', 7, 12, 'in SQL dialect 3.', NULL, NULL, 'c_pg');
(1043, 'duquette', '1999-07-30 06:35:25', 0, 482, 'Warning:  Database dialect being reset from 3 to 1', NULL, NULL, 'c_pg');
(1044, 'duquette', '1999-07-30 06:39:45', 7, 4, 'Use of %s expression that returns different results in  dialect 1 and dialect 3', NULL, NULL, 'c_pg');
(1045, 'duquette', '1999-07-30 07:37:50', 0, 488, '%s datatype in dialect %d is now called %s', NULL, NULL, 'c_pg');
(1046, 'duquette', '1999-07-30 08:00:18', 0, 482, 'Database dialect being reset from 3 to 1', NULL, NULL, 'c_pg');
(1047, 'duquette', '1999-08-05 09:30:12', 0, 488, '%s data type is now called %s', NULL, NULL, 'c_pg');
(1048, 'duquette', '1999-10-01 13:56:17', 0, 490, 'calculation exceeds range of valid dates', NULL, NULL, 'c_pg');
(1049, 'SYSDBA', '2001-06-25 23:09:35', 0, 321, 'Specified domain or source column does not exist', NULL, NULL, 'c_pg');
(1050, 'frank', '2001-06-30 11:00:56', 0, 264, 'Count of columns does not equal count of values', NULL, NULL, 'c_pg');
(1051, 'SYSDBA', '2001-08-22 13:25:24', 0, 497, 'Invalid parameter to FIRST/LIMIT.  Only positive values allowed.', NULL, NULL, 'c_pg');
(1052, 'SYSDBA', '2001-10-10 03:27:44', 17, 108, 'SET PLANONLY           -- toggle display of query plan without executing', NULL, NULL, 'c_pg');
(1053, 'SYSDBA', '2002-03-05 02:14:57', 0, 152, 'Your user name and password are not defined. Ask your database administrator to set up an InterBase login.', NULL, NULL, 'c_pg');
(1054, 'SYSDBA', '2002-03-05 02:15:34', 0, 201, 'Unable to roll over; please see InterBase log.', NULL, NULL, 'c_pg');
(1055, 'SYSDBA', '2002-03-05 02:16:03', 0, 202, 'WAL I/O error.  Please see InterBase log.', NULL, NULL, 'c_pg');
(1056, 'SYSDBA', '2002-03-05 02:16:49', 0, 203, 'WAL writer - Journal server communication error.  Please see InterBase log.', NULL, NULL, 'c_pg');
(1057, 'SYSDBA', '2002-03-05 02:17:15', 0, 204, 'WAL buffers cannot be increased.  Please see InterBase log.', NULL, NULL, 'c_pg');
(1058, 'SYSDBA', '2002-03-05 02:18:04', 0, 205, 'WAL setup error.  Please see InterBase log.', NULL, NULL, 'c_pg');
(1059, 'SYSDBA', '2002-03-05 02:19:15', 0, 369, 'InterBase error', NULL, NULL, 'c_pg');
(1060, 'SYSDBA', '2002-03-05 02:20:15', 0, 425, 'Your login %s is same as one of the SQL role name. Ask your database administrator to set up a valid InterBase login.', NULL, NULL, 'c_pg');
(1061, 'SYSDBA', '2002-03-05 02:28:18', 12, 252, '        %sRO(LE)               InterBase SQL role', NULL, NULL, 'c_pg');
(1062, 'SYSDBA', '2002-03-05 02:28:40', 15, 239, 'InterBase status vector inconsistent', NULL, NULL, 'c_pg');
(1063, 'SYSDBA', '2002-03-05 02:29:03', 15, 240, 'InterBase/RdB message parameter inconsistency', NULL, NULL, 'c_pg');
(1064, 'SYSDBA', '2002-03-05 02:29:40', 19, 45, 'InterBase Server: Client capability', NULL, NULL, 'c_pg');
(1065, 'SYSDBA', '2002-03-05 02:29:56', 19, 46, 'InterBase Server: Remote access capability.', NULL, NULL, 'c_pg');
(1066, 'SYSDBA', '2002-03-05 02:30:12', 19, 47, 'InterBase Server: Metadata capability.', NULL, NULL, 'c_pg');
(1067, 'SYSDBA', '2002-03-05 02:31:06', 20, 0, 'The connection request was refused.  This can occur if the InterBase server is not started on the host machine.', NULL, NULL, 'c_pg');
(1068, 'SYSDBA', '2002-03-05 02:31:33', 20, 31, 'The InterBase server is not responding or has closed the connection.', NULL, NULL, 'c_pg');
(1069, 'SYSDBA', '2002-03-05 02:31:54', 20, 32, 'The InterBase server is not responding or has closed the connection.', NULL, NULL, 'c_pg');
(1070, 'SYSDBA', '2002-03-05 02:36:43', 12, 190, '	%sPAS(SWORD)           InterBase password', NULL, NULL, 'c_pg');
(1071, 'SYSDBA', '2002-03-05 02:38:49', 12, 191, '	%sUSER                 InterBase user name', NULL, NULL, 'c_pg');
(1072, 'SYSDBA', '2002-03-05 02:41:40', 13, 1, 'Interbase error', NULL, NULL, 'c_pg');
(1073, 'SYSDBA', '2002-03-05 02:47:26', 13, 748, 'WAL setup error.  Please see InterBase log.', NULL, NULL, 'c_pg');
(1074, 'SYSDBA', '2002-03-05 02:51:01', 13, 749, 'WAL buffers cannot be increased.  Please see InterBase log.', NULL, NULL, 'c_pg');
(1075, 'SYSDBA', '2002-03-05 02:53:08', 13, 750, 'WAL writer - Journal server communication error.  Please see InterBase log.', NULL, NULL, 'c_pg');
(1076, 'SYSDBA', '2002-03-05 02:54:15', 13, 751, 'WAL I/O error.  Please see InterBase log.', NULL, NULL, 'c_pg');
(1077, 'SYSDBA', '2002-03-05 02:55:22', 13, 752, 'Unable to roll over; please see InterBase log.', NULL, NULL, 'c_pg');
(1078, 'SYSDBA', '2002-03-05 03:01:30', 0, 499, 'file exceeded maximum size of 2GB.  add another database file or use a 64 bit io version of Firebird.', NULL, NULL, 'c_pg');
(1079, 'SYSDBA', '2003-04-13 16:06:28', 0, 497, 'Invalid parameter to FIRST.  Only positive values are allowed.', NULL, NULL, 'c_pg');
(1080, 'SYSDBA', '2003-06-12 13:53:50', 12, 283, 'closing file, committing, and finishing. %s bytes written', NULL, NULL, 'c_pg');
(1081, 'SYSDBA', '2003-06-12 17:36:06', 0, 503, 'Cannot use an aggregate in a GROUP BY clause', NULL, NULL, 'c_pg');
(1082, 'SYSDBA', '2003-06-12 18:33:11', 0, 505, 'Invalid expression in the %s (neither an aggregate function nor contained in the GROUP BY clause)', NULL, NULL, 'c_pg');
(1083, 'SYSDBA', '2003-06-12 18:33:23', 0, 504, 'Invalid expression in the %s (not contained in either an aggregate function or the GROUP BY clause)', NULL, NULL, 'c_pg');
(1084, 'SYSDBA', '2003-06-13 11:43:24', 0, 504, 'Invalid expression in the %s (contained in neither an aggregate function nor the GROUP BY clause)', NULL, NULL, 'c_pg');
(1085, 'SYSDBA', '2003-08-12 23:20:03', 8, 216, 'Difference file is not defined', NULL, NULL, 'c_pg');
(1086, 'SYSDBA', '2003-08-12 23:20:22', 8, 217, 'Difference file is already defined', NULL, NULL, 'c_pg');
(1087, 'SYSDBA', '2003-10-26 11:45:02', 0, 252, 'Cursor unknown', NULL, NULL, 'c_pg');
(1088, 'SYSDBA', '2004-04-02 21:17:14', 3, 46, '	-shut		shutdown', NULL, NULL, 'c_pg');
(1089, 'SYSDBA', '2004-04-02 21:18:26', 3, 40, '	-online		database online', NULL, NULL, 'c_pg');
(1090, 'SYSDBA', '2004-10-22 08:29:23', 0, 520, 'cannot update', NULL, NULL, 'c_pg');
(1091, 'SYSDBA', '2004-11-01 23:42:43', 12, 44, 'Expected backup version 1, 2, or 3.  Found %ld', NULL, NULL, 'c_pg');
(1092, 'cvc', '2004-11-13 05:49:20', 18, 87, '-database <security database>', NULL, NULL, 'c_pg');
(1093, 'dimitr', '2004-11-17 17:08:12', 0, 252, 'Cursor %s %s', NULL, NULL, 'c_pg');
(1094, 'dimitr', '2004-11-17 17:08:45', 0, 254, 'Declared cursor already exists', NULL, NULL, 'c_pg');
(1095, 'dimitr', '2004-11-17 17:09:27', 0, 255, 'Cursor not updatable', NULL, NULL, 'c_pg');
(1096, 'dimitr', '2004-11-17 17:10:12', 0, 514, 'Invalid cursor state: %s', NULL, NULL, 'c_pg');
(1097, 'cvc', '2004-11-24 00:55:50', 17, 11, '     [-x|-a] [-d <target db>] [-u <user>] [-p <password>]
     [-page <pagelength>] [-n] [-m] [-q] [-s <sql_dialect>]', NULL, NULL, 'c_pg');
(1098, 'cvc', '2005-03-06 03:10:18', 17, 48, 'Enter %s as M/D/Y>', NULL, NULL, 'c_pg');
(1099, 'cvc', '2005-03-06 03:12:01', 17, 104, '     [-r <rolename>] [-c <num cache buffers>] [-z] -nowarnings -noautocommit', NULL, NULL, 'c_pg');
(1100, 'hvlad', '2005-03-03 15:20:00', 12, 112, '	%sR(EPLACE_DATABASE)   replace database from backup file', NULL, NULL, 'c_pg');
(1101, 'dimitr', '2005-04-27 14:20:00', 8, 123, 'Column: %s not defined as NOT NULL - cannot be used in PRIMARY KEY/UNIQUE constraint definition', NULL, NULL, 'c_pg');
(1102, 'dimitr', '2005-04-27 14:20:20', 0, 211, 'Column used in a PRIMARY/UNIQUE constraint must be NOT NULL.', NULL, NULL, 'c_pg');
(1103, 'dimitr', '2005-04-27 14:20:40', 13, 709, 'Column used in a PRIMARY/UNIQUE constraint must be NOT NULL.', NULL, NULL, 'c_pg');
(1104, 'cvc', '2005-04-10 00:47:30', 12, 14, 'database %s already exists.  To replace it, use the -R switch', NULL, NULL, 'c_pg');
(1105, 'cvc', '2005-05-10 15:00:30', 8, 37, 'BLOB Filter not found', 'Define blob filter in RDB$BLOB_FILTERS.', 'Blob filter was not found.', 'c_pg');
(1106, 'cvc', '2005-05-10 15:03:30', 8, 41, 'Function not found', NULL, NULL, 'c_pg');
(1107, 'cvc', '2005-05-10 15:07:10', 8, 54, 'ERASE RDB$RELATION_FIELDS failed', NULL, NULL, 'c_pg');
(1108, 'cvc', '2005-05-10 15:09:30', 8, 85, 'MODIFY DATABASE failed', NULL, NULL, 'c_pg');
(1109, 'cvc', '2005-05-10 15:11:45', 8, 86, 'MODIFY DATABASE failed', NULL, NULL, 'c_pg');
(1110, 'cvc', '2005-05-10 15:17:30', 8, 88, 'MODIFY RDB$FIELDS failed', NULL, NULL, 'c_pg');
(1111, 'cvc', '2005-05-10 15:22:30', 8, 91, 'MODIFY RDB$INDICESS failed', NULL, NULL, 'c_pg');
(1112, 'cvc', '2005-05-10 15:25:03', 8, 92, 'MODIFY RDB$INDICES failed', NULL, NULL, 'c_pg');
(1113, 'cvc', '2005-05-10 15:27:30', 8, 94, 'MODIFY RDB$FIELDS failed', NULL, NULL, 'c_pg');
(1114, 'cvc', '2005-05-10 15:33:00', 8, 100, 'MODIFY RDB$RELATIONS failed', NULL, NULL, 'c_pg');
(1115, 'cvc', '2005-05-10 15:37:20', 8, 151, 'Write-ahead Log already exists', NULL, NULL, 'c_pg');
(1116, 'cvc', '2005-05-10 15:39:55', 8, 152, 'Write-ahead Log not found', NULL, NULL, 'c_pg');
(1117, 'cvc', '2005-05-10 15:44:08', 8, 155, 'Write-ahead Log lookup failed', NULL, NULL, 'c_pg');
(1118, 'cvc', '2005-05-10 15:47:30', 8, 158, 'MODIFY DATABASE failed', NULL, NULL, 'c_pg');
(1119, 'cvc', '2005-05-10 15:50:00', 8, 214, 'Generator not found', NULL, NULL, 'c_pg');
(1120, 'cvc', '2005-05-19 04:41:22', 8, 156, 'Shared cache lookup failed', NULL, NULL, 'c_pg');
(1121, 'cvc', '2005-05-19 04:41:41', 8, 148, 'Shared cache file already exists', NULL, NULL, 'c_pg');
(1122, 'cvc', '2005-05-19 04:42:13', 8, 149, 'Shared cache file not found', NULL, NULL, 'c_pg');
(1123, 'cvc', '2005-05-19 05:31:33', 8, 109, 'Create metadata BLOB failed', NULL, NULL, 'c_pg');
(1124, 'cvc', '2005-06-02 21:01:00', 12, 211, 'writing Character Sets', NULL, NULL, 'c_pg');
(1125, 'cvc', '2005-06-02 21:03:05', 12, 212, 'writing Collations', NULL, NULL, 'c_pg');
(1126, 'cvc', '2005-06-02 21:05:27', 12, 214, 'Writing character set %s', NULL, NULL, 'c_pg');
(1127, 'cvc', '2005-06-02 21:09:09', 12, 216, 'Writing collation %s', NULL, NULL, 'c_pg');
(1128, 'cvc', '2005-06-02 21:12:03', 12, 249, 'writing sql role: %s', NULL, NULL, 'c_pg');
(1129, 'cvc', '2005-06-02 21:15:57', 8, 220, 'CREATE COLLATION statement are not supported in on older versions of the database.  A backup and restore is required.', NULL, NULL, 'c_pg');
(1130, 'dimitr', '2005-06-07 20:20:21', 0, 192, 'parameter mismatch for procedure %s', NULL, NULL, 'c_pg');
(1131, 'cvc', '2005-06-09 21:24:27', 0, 314, 'Token unknown - line %ld, char %ld', NULL, NULL, 'c_pg');
(1132, 'cvc', '2005-06-12 00:24:08', 0, 517, 'Invalid %s parameter to SUBSTRING. Only positive integers are allowed.', NULL, NULL, 'c_pg');
(1133, 'asf', '2005-06-13 22:54:00', 0, 268, 'COLLATION %s is not defined', NULL, NULL, 'c_pg');
(1134, 'cvc', '2005-07-31 02:18:54', 8, 223, 'Collation not installed', NULL, NULL, 'c_pg');
(1135, 'cvc', '2005-08-14 02:08:44', 0, 197, 'exception %d', NULL, NULL, 'c_pg');
(1136, 'cvc', '2005-09-10 00:27:09', 13, 770, 'WAL Writer error', NULL, NULL, 'c_pg');
(1137, 'cvc', '2005-09-10 00:28:08', 13, 760, 'Illegal attempt to attach to an uninitialized WAL segment for %s', NULL, NULL, 'c_pg');
(1138, 'cvc', '2005-09-10 00:21:09', 13, 759, 'Invalid WAL parameter block option %s', NULL, NULL, 'c_pg');
(1139, 'alexpeshkoff', '2005-09-30 11:49:49', 18, 87, '-server <server to manage>', NULL, NULL, 'c_pg');
(1140, 'cvc', '2005-10-05 03:07:00', 13, 756, 'WAL subsystem encountered error', NULL, NULL, 'c_pg');
(1141, 'cvc', '2005-10-05 03:08:08', 13, 755, 'WAL subsystem corrupted', NULL, NULL, 'c_pg');
(1142, 'cvc', '2005-10-05 03:09:24', 13, 754, 'Database %s: WAL subsystem bug for pid %d\\n%s', NULL, NULL, 'c_pg');
(1143, 'cvc', '2005-10-05 03:10:53', 13, 753, 'Could not expand the WAL segment for database %s', NULL, NULL, 'c_pg');
(1144, 'cvc', '2005-10-07 03:06:09', 0, 206, 'WAL writer synchronization error for the database %s', NULL, NULL, 'c_pg');
(1145, 'cvc', '2005-10-14 01:04:26', 17, 104, '     [-r <rolename>] [-c <num cache buffers>] [-z] [-nowarnings] [-noautocommit]', NULL, NULL, 'c_pg');
(1146, 'dimitr', '2005-12-12 12:20:30', 0, 408, 'Cannot deactivate index used by an Integrity Constraint', NULL, NULL, 'c_pg');
(1147, 'dimitr', '2005-12-12 12:22:25', 0, 409, 'Cannot deactivate primary index', NULL, NULL, 'c_pg');
(1148, 'dimitr', '2006-01-26 01:43:29', 12, 3, 'Page size specified (%ld) greater than limit (8192 bytes)', NULL, NULL, 'c_pg');
(1149, 'hvlad', '2006-02-02 19:15:11', 17, 44, 'Current memory = !c
Delta memory = !d
Max memory = !x
Elapsed time= !e sec', NULL, NULL, 'c_pg');
(1150, 'hvlad', '2006-02-03 19:17:10', 17, 45, 'Cpu = !u sec
Buffers = !b
Reads = !r
Writes = !w
Fetches = !f', NULL, NULL, 'c_pg');
(1151, 'hvlad', '2006-02-03 19:20:10', 17, 93, 'Buffers = !b
Reads = !r
Writes !w
Fetches = !f', NULL, NULL, 'c_pg');
(1152, 'dimitr', '2006-03-07 10:54:00', 0, 480, 'Too many Contexts of Relation/Procedure/Views. Maximum allowed is 127', NULL, NULL, 'c_pg');
(1153, 'mkubecek', '2006-03-14 09:40:20', 17, 11, '     [-x|-a] [-d <target db>] [-u <user>] [-p <password>]', NULL, NULL, 'c_pg');
(1154, 'mkubecek', '2006-03-14 09:45:22', 17, 1, 'isql [<database>] [-e] [-t <terminator>] [-i <inputfile>] [-o <outputfile>]', NULL, NULL, 'c_pg');
(1155, 'mkubecek', '2006-03-14 09:51:23', 17, 104, '     [-r|-r2 <rolename>] [-c <num cache buffers>] [-now] [-z]', NULL, NULL, 'c_pg');
(1156, 'mkubecek', '2006-03-14 09:55:55', 17, 111, '     [-page <pagelength>] [-n] [-m] [-m2] [-q] [-s <sql_dialect>] [-b]', NULL, NULL, 'c_pg');
(1157, 'mkubecek', '2006-03-24 01:00:05', 17, 11, '	-a                      extract metadata including legacy non-SQL tables', NULL, NULL, 'c_pg');
(1158, 'mkubecek', '2006-03-24 01:03:13', 17, 104, '	-b                      bail on errors (set bail on)', NULL, NULL, 'c_pg');
(1159, 'mkubecek', '2006-03-24 01:05:22', 17, 124, '	-e                      echo commands (set echo on)', NULL, NULL, 'c_pg');
(1160, 'mkubecek', '2006-03-24 01:09:09', 17, 126, '	-i <file>               input file (set input)', NULL, NULL, 'c_pg');
(1161, 'mkubecek', '2006-03-24 01:12:40', 17, 127, '	-i <file>               input file (set input)', NULL, NULL, 'c_pg');
(1162, 'mkubecek', '2006-03-24 01:14:52', 17, 131, '	-o <file>               output file (set output)', NULL, NULL, 'c_pg');
(1163, 'mkubecek', '2006-03-24 01:17:31', 17, 132, '	-page <size>            page length', NULL, NULL, 'c_pg');
(1164, 'mkubecek', '2006-03-24 01:20:04', 17, 134, '	-q                      do not show the welcome message "Use CONNECT..."', NULL, NULL, 'c_pg');
(1165, 'mkubecek', '2006-03-24 01:23:37', 17, 138, '	-t <terminator>         command terminator (set term)', NULL, NULL, 'c_pg');
(1166, 'mkubecek', '2006-03-24 01:25:21', 17, 139, '	-u <user>               user name', NULL, NULL, 'c_pg');
(1167, 'mkubecek', '2006-03-26 13:08:09', 17, 142, 'missing argument of switch "%s"', NULL, NULL, 'c_pg');
(1168, 'mkubecek', '2006-03-26 13:08:09', 17, 143, 'argument "%s" is not an integer', NULL, NULL, 'c_pg');
(1169, 'mkubecek', '2006-03-26 13:08:09', 17, 144, 'value "%s" is out of range', NULL, NULL, 'c_pg');
(1170, 'cvc', '2006-07-07 03:00:00', 17, 11, '	-a                      extract metadata incl. legacy non-SQL tables', NULL, NULL, 'c_pg');
(1171, 'cvc', '2006-07-07 03:02:10', 17, 123, '	-d <database>           database name to put in script creation', NULL, NULL, 'c_pg');
(1172, 'cvc', '2006-07-07 03:03:03', 17, 130, '	-now(arn)               do not show warnings', NULL, NULL, 'c_pg');
(1173, 'cvc', '2006-07-07 03:04:04', 17, 132, '	-pag(e) <size>          page length', NULL, NULL, 'c_pg');
(1174, 'cvc', '2006-07-07 03:05:25', 17, 137, '	-s <dialect>            SQL dialect (set sql dialect)', NULL, NULL, 'c_pg');
(1175, 'cvc', '2006-07-07 03:07:09', 17, 138, '	-t(erm) <terminator>    command terminator (set term)', NULL, NULL, 'c_pg');
(1176, 'cvc', '2006-07-14 00:34:17', 8, 18, 'could not find UNIQUE INDEX with specified columns', NULL, NULL, 'c_pg');
(1177, 'asf', '2006-07-18 15:07:17', 12, 44, 'Expected backup version 1, 2, 3, 4, 5, 6, or 7.  Found %ld', NULL, NULL, 'c_pg');
(1178, 'cvc', '2006-07-28 04:50:50', 12, 208, 'Bad attribute for table constraint', NULL, NULL, 'c_pg');
(1179, 'cvc', '2006-07-28 04:53:12', 12, 213, 'Bad attribute for RDB$CHARACTER_SETS', NULL, NULL, 'c_pg');
(1180, 'cvc', '2006-07-28 04:55:35', 12, 215, 'Bad attribute for RDB$COLLATIONS', NULL, NULL, 'c_pg');
(1181, 'cvc', '2006-07-28 04:58:25', 12, 250, 'Bad attributes for restoring SQL role', NULL, NULL, 'c_pg');
(1182, 'cvc', '2006-07-28 06:38:03', 12, 63, '	%sM(ETA_DATA)          backup metadata only', NULL, NULL, 'c_pg');
(1183, 'cvc', '2006-08-31 04:21:42', 8, 43, 'column %s is used in table %s (local name %s) and cannot be dropped', NULL, NULL, 'c_pg');
(1184, 'cvc', '2006-10-01 03:18:30', 8, 18, 'could not find UNIQUE or PRIMARY KEY constraint with specified columns', NULL, NULL, 'c_pg');
(1185, 'cvc', '2006-10-01 03:20:51', 8, 20, 'could not find PRIMARY KEY index in specified table', NULL, NULL, 'c_pg');
(1186, 'cvc', '2007-10-03 05:28:09', 17, 102, '               TRIGGER, VERSION, VIEW', NULL, NULL, 'c_pg');
(1187, 'cvc', '2007-11-20 02:42:50', 0, 76, 'column @1 is not defined in table @1', NULL, 'An undefined external function was referenced in blr.', 'c_pg')
(1188, 'alexpeshkoff', '2007-12-21 19:03:07', 12, 249, 'writing SQL role: @1', NULL, NULL, 'c_pg');
(1189, 'alexpeshkoff', '2007-12-21 19:03:07', 12, 251, 'restoring SQL role: @1', NULL, NULL, 'c_pg');
(1190, 'dimitr', '2008-03-12 08:02:00', 17, 0, 'Statement failed, SQLCODE = @1', NULL, NULL, 'c_pg')
(1191, 'dimitr', '2008-03-17 12:33:42', 8, 15, 'STORE RDB$INDICES failed', NULL, NULL, 'c_pg')
(1192, 'cvc', '2008-07-06 06:06:47', 0, 13, 'internal gds software consistency check (@1)', NULL, NULL, 'c_pg')
(1193, 'cvc', '2008-07-06 08:47:00', 0, 227, 'internal gds software consistency check (invalid RDB$CONSTRAINT_TYPE)', NULL, NULL, 'c_pg')
(1194, 'cvc', '2008-07-06 08:47:21', 13, 704, 'internal gds software consistency check (invalid RDB$CONSTRAINT_TYPE)', NULL, NULL, 'c_pg')
(1195, 'hvlad', '2008-11-07 10:55:00', 0, 601, 'Execute statement error at @1 :\n@2Data source : @3', NULL, NULL, 'c_pg')
(1196, 'hvlad', '2008-11-07 10:55:00', 0, 606, 'Execute statement error at @1 :\n@2Statement : @3\nData source : @4', NULL, NULL, 'c_pg')
(1197, 'asfernandes', '2009-04-21 11:18:00', 17, 38, '    <object> = CHECK, DATABASE, DOMAIN, EXCEPTION, FILTER, FUNCTION, GENERATOR,', NULL, NULL, 'c_pg')
(1198, 'asfernandes', '2009-04-21 11:19:00', 17, 64, '               GRANT, INDEX, PROCEDURE, ROLE, SQL DIALECT, SYSTEM, TABLE,', NULL, NULL, 'c_pg')
(1199, 'asfernandes', '2009-04-21 11:20:00', 17, 102, '               TRIGGER, VERSION, USERS, VIEW', NULL, NULL, 'c_pg')
(1200, 'cvc', '2009-06-22 05:53:30', 21, 27, '    -s      analyze system relations', NULL, NULL, 'c_pg')
(1201, 'cvc', '2009-06-22 05:54:50', 21, 24, '    -h      analyze header page', NULL, NULL, 'c_pg')
(1202, 'cvc', '2009-06-22 05:56:20', 21, 35, '    -t      tablename', NULL, NULL, 'c_pg')
(1203, 'cvc', '2009-07-05 09:04:08', 12, 73, '	@1C(REATE_DATABASE)    create database from backup file', NULL, NULL, 'c_pg')
(1204, 'cvc', '2009-07-05 09:05:10', 12, 112, '	@1REP(LACE_DATABASE)   replace database from backup file', NULL, NULL, 'c_pg')
(1205, 'cvc', '2009-07-05 09:06:09', 12, 284, '	@1R(ECREATE_DATABASE) [O(VERWRITE)] create (or replace if OVERWRITE used)
				database from backup file', NULL, NULL, 'c_pg')
(1206, 'cvc', '2009-07-13 22:30:01', 3, 22, '
    qualifiers show the major option in parenthesis', NULL, NULL, 'c_pg')
(1207, 'cvc', '2009-07-13 22:30:17', 3, 25, '	-activate	activate shadow file for database usage', NULL, NULL, 'c_pg')
(1208, 'cvc', '2009-07-13 22:30:50', 3, 26, '	-attach		shutdown new database attachments', NULL, NULL, 'c_pg')
(1209, 'cvc', '2009-07-13 22:31:42', 3, 28, '	-buffers	set page buffers <n>', NULL, NULL, 'c_pg')
(1210, 'cvc', '2009-07-13 22:32:12', 3, 29, '	-commit		commit transaction <tr / all>', NULL, NULL, 'c_pg')
(1211, 'cvc', '2009-07-13 22:35:05', 3, 30, '	-cache		shutdown cache manager', NULL, NULL, 'c_pg')
(1212, 'cvc', '2009-07-13 22:35:28', 3, 32, '	-full		validate record fragments (-v)', NULL, NULL, 'c_pg')
(1213, 'cvc', '2009-07-13 22:35:55', 3, 33, '	-force		force database shutdown', NULL, NULL, 'c_pg')
(1214, 'cvc', '2009-07-13 22:36:20', 3, 34, '	-housekeeping	set sweep interval <n>', NULL, NULL, 'c_pg')
(1215, 'cvc', '2009-07-13 22:36:53', 3, 35, '	-ignore		ignore checksum errors', NULL, NULL, 'c_pg')
(1216, 'cvc', '2009-07-13 22:37:10', 3, 36, '	-kill		kill all unavailable shadow files', NULL, NULL, 'c_pg')
(1217, 'cvc', '2009-07-13 22:37:33', 3, 37, '	-list		show limbo transactions', NULL, NULL, 'c_pg')
(1218, 'cvc', '2009-07-13 22:38:02', 3, 38, '	-mend		prepare corrupt database for backup', NULL, NULL, 'c_pg')
(1219, 'cvc', '2009-07-13 22:38:15', 3, 39, '	-no_update	read-only validation (-v)', NULL, NULL, 'c_pg')
(1220, 'cvc', '2009-07-13 22:38:28', 3, 40, '	-online		database online <single / multi / normal>', NULL, NULL, 'c_pg')
(1221, 'cvc', '2009-07-13 22:38:50', 3, 41, '	-prompt		prompt for commit/rollback (-l)', NULL, NULL, 'c_pg')
(1222, 'cvc', '2009-07-13 22:39:09', 3, 42, '	-password	default password', NULL, NULL, 'c_pg')
(1223, 'cvc', '2009-07-13 22:39:40', 3, 44, '	-rollback	rollback transaction <tr / all>', NULL, NULL, 'c_pg')
(1224, 'cvc', '2009-07-13 22:40:15', 3, 45, '	-sweep		force garbage collection', NULL, NULL, 'c_pg')
(1225, 'cvc', '2009-07-13 22:40:40', 3, 46, '	-shut		shutdown <full / single / multi>', NULL, NULL, 'c_pg')
(1226, 'cvc', '2009-07-13 22:40:59', 3, 47, '	-two_phase	perform automated two-phase recovery', NULL, NULL, 'c_pg')
(1227, 'cvc', '2009-07-13 22:41:19', 3, 48, '	-tran		shutdown transaction startup', NULL, NULL, 'c_pg')
(1228, 'cvc', '2009-07-13 22:41:38', 3, 49, '	-use		use full or reserve space for versions', NULL, NULL, 'c_pg')
(1229, 'cvc', '2009-07-13 22:41:58', 3, 50, '	-user		default user name', NULL, NULL, 'c_pg')
(1230, 'cvc', '2009-07-13 22:42:17', 3, 51, '	-validate	validate database structure', NULL, NULL, 'c_pg')
(1231, 'cvc', '2009-07-13 22:42:33', 3, 52, '	-write		write synchronously or asynchronously', NULL, NULL, 'c_pg')
(1232, 'cvc', '2009-07-13 22:42:54', 3, 53, '	-x		set debug on', NULL, NULL, 'c_pg')
(1233, 'cvc', '2009-07-13 22:43:13', 3, 54, '	-z		print software version number', NULL, NULL, 'c_pg')
(1234, 'cvc', '2009-07-13 22:43:29', 3, 55, '`
	Number of record level errors	: @1', NULL, NULL, 'c_pg')
(1235, 'cvc', '2009-07-13 22:43:55', 3, 109, '	-mode		read_only or read_write', NULL, NULL, 'c_pg')
(1236, 'cvc', '2009-07-13 22:44:10', 3, 111, '	-sql_dialect	set database dialect n', NULL, NULL, 'c_pg')
(1237, 'cvc', '2009-07-13 22:44:27', 3, 115, '	-trusted	use trusted authentication', NULL, NULL, 'c_pg')
(1238, 'cvc', '2009-07-13 22:45:00', 3, 119, '	-fetch_password fetch_password from file', NULL, NULL, 'c_pg')
(1239, 'cvc', '2009-07-16 05:26:11', 3, 22, '
    qualifiers show the major option in parenthesis', NULL, NULL, 'c_pg')
(1240, 'cvc', '2009-07-16 06:48:27', 17, 155, '	-tr(usted)              use Windows trusted authentication', NULL, NULL, 'c_pg')
(1241, 'asfernandes', '2009-10-20 12:20:20', 0, 231, 'could not find table/procedure for GRANT', NULL, NULL, 'c_pg')
(1242, 'asfernandes', '2009-10-20 12:20:40', 0, 502, 'Cannot use an aggregate function in a WHERE clause, use HAVING instead', NULL, NULL, 'c_pg')
(1243, 'asfernandes', '2009-10-20 12:21:00', 0, 503, 'Cannot use an aggregate function in a GROUP BY clause', NULL, NULL, 'c_pg')
(1244, 'asfernandes', '2009-10-20 12:21:20', 0, 506, 'Nested aggregate functions are not allowed', NULL, NULL, 'c_pg')
(1245, 'cvc', '2009-11-06 08:26:40', 12, 44, 'Expected backup version 1..9.  Found @1', NULL, NULL, 'c_pg')
(1246, 'cvc', '2009-11-13 05:37:37', 17, 64, '               GENERATOR, GRANT, INDEX, PROCEDURE, ROLE, SQL DIALECT, SYSTEM,', NULL, NULL, 'c_pg')
(1247, 'cvc', '2009-11-13 08:37:53', 17, 102, '               TABLE, TRIGGER, VERSION, USERS, VIEW', NULL, NULL, 'c_pg')
(1248, 'alexpeshkoff', '2009-11-13 17:49:10', 18, 26, '     user name                      uid   gid     full name', NULL, NULL, 'c_pg')
(1249, 'alexpeshkoff', '2009-11-13 17:49:54', 18, 27, '------------------------------------------------------------------------------------------', NULL, NULL, 'c_pg')
(1250, 'cvc', '2009-12-18 09:21:40', 12, 68, '    committing metadata', NULL, NULL, 'c_pg')
(1251, 'cvc', '2009-12-18 08:21:50', 12, 196, 'restoring parameter @1 for stored procedure', NULL, NULL, 'c_pg')
(1252, 'alexpeshkoff', '2012-06-23 05:40:21', 12, 266, 'standard output is not supported when using split operation', NULL, NULL, 'c_pg')
stop

COMMIT WORK;
