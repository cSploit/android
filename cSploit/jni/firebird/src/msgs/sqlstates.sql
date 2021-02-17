set bulk_insert INSERT INTO SQLSTATES (SQL_CLASS, SQL_SUBCLASS, SQL_STATE_TEXT) VALUES (?, ?, ?);
-- 00 Success
('00', '000', 'Success')
-- 01 Warning
('01' '000', 'General warning')
('01' '001', 'Cursor operation conflict')
('01' '002', 'Disconnect error')
('01' '003', 'NULL value eliminated in set function')
('01' '004', 'String data, right-truncated')
('01' '005', 'Insufficient item descriptor areas')
('01' '006', 'Privilege not revoked')
('01' '007', 'Privilege not granted')
('01' '008', 'Implicit zero-bit padding')
('01' '100', 'Statement reset to unprepared')
('01' '101', 'Ongoing transaction has been committed')
('01' '102', 'Ongoing transaction has been rolled back')
-- 02 No Data
('02', '000', 'No data found or no rows affected')
-- 07 Dynamic SQL error
('07', '000', 'Dynamic SQL error')
('07', '001', 'Wrong number of input parameters')
('07', '002', 'Wrong number of output parameters')
('07', '003', 'Cursor specification cannot be executed')
('07', '004', 'USING clause required for dynamic parameters')
('07', '005', 'Prepared statement not a cursor specification')
('07', '006', 'Restricted data type attribute violation')
('07', '007', 'USING clause required for result fields')
('07', '008', 'Invalid descriptor count')
('07', '009', 'Invalid descriptor index')
-- 08 Connection Exception
('08', '001', 'Client unable to establish connection')
('08', '002', 'Connection name in use')
('08', '003', 'Connection does not exist')
('08', '004', 'Server rejected the connection')
('08', '006', 'Connection failure')
('08', '007', 'Transaction resolution unknown')
-- 0A Feature Not Supported
('0A', '000', 'Feature not supported')
-- 0B Invalid Transaction Initiation
('0B', '000', 'Invalid transaction initiation')
-- 0L Invalid Grantor
('0L', '000', 'Invalid grantor')
-- 0P Invalid Role Specification
('0P', '000', 'Invalid role specification')
-- 0U Attempt to Assign to Non-Updatable Column
('0U', '000', 'Attempt to assign to non-updatable column')
-- OV Attempt to Assign to Ordering Column
('OV', '000', 'Attempt to assign to Ordering Column')
-- 20 Case Not Found For Case Statement
('20', '000', 'Case not found for case statement')
-- 21 Cardinality Violation
('21', '000', 'Cardinality violation')
('21', 'S01', 'Insert value list does not match column list')
('21', 'S02', 'Degree of derived table does not match column list')
-- 22 Data Exception
('22', '000', 'Data exception')
('22', '001', 'String data, right truncation')
('22', '002', 'Null value, no indicator parameter')
('22', '003', 'Numeric value out of range')
('22', '004', 'Null value not allowed')
('22', '005', 'Error in assignment')
('22', '006', 'Null value in field reference')
('22', '007', 'Invalid datetime format')
('22', '008', 'Datetime field overflow')
('22', '009', 'Invalid time zone displacement value')
('22', '00A', 'Null value in reference target')
('22', '00B', 'Escape character conflict')
('22', '00C', 'Invalid use of escape character')
('22', '00D', 'Invalid escape octet')
('22', '00E', 'Null value in array target')
('22', '00F', 'Zero-length character string')
('22', '00G', 'Most specific type mismatch')
('22', '010', 'Invalid indicator parameter value')
('22', '011', 'Substring error')
('22', '012', 'Division by zero')
('22', '014', 'Invalid update value')
('22', '015', 'Interval field overflow')
('22', '018', 'Invalid character value for cast')
('22', '019', 'Invalid escape character')
('22', '01B', 'Invalid regular expression')
('22', '01C', 'Null row not permitted in table')
('22', '020', 'Invalid limit value')
('22', '021', 'Character not in repertoire')
('22', '022', 'Indicator overflow')
('22', '023', 'Invalid parameter value')
('22', '024', 'Character string not properly terminated')
('22', '025', 'Invalid escape sequence')
('22', '026', 'String data, length mismatch')
('22', '027', 'Trim error')
('22', '028', 'Row already exists')
('22', '02D', 'Null instance used in mutator function')
('22', '02E', 'Array element error')
('22', '02F', 'Array data, right truncation')
-- 23 Integrity Constraint Violation
('23', '000', 'Integrity constraint violation')
-- 24 Invalid Cursor State
('24', '000', 'Invalid cursor state') 
('24', '504', 'The cursor identified in the UPDATE, DELETE, SET, or GET statement is not positioned on a row')
-- 25 Invalid Transaction State
('25', '000', 'Invalid transaction state')
('25', 'S01', 'Transaction state')
('25', 'S02', 'Transaction is still active') 
('25', 'S03', 'Transaction is rolled back')
-- 26 Invalid SQL Statement Name
('26', '000', 'Invalid SQL statement name')
-- 27 Triggered Data Change Violation
('27', '000', 'Triggered data change violation')
-- 28 Invalid Authorization Specification
('28', '000', 'Invalid authorization specification')
-- 2B Dependent Privilege Descriptors Still Exist
('2B', '000', 'Dependent privilege descriptors still exist')
-- 2C Invalid Character Set Name
('2C', '000', 'Invalid character set name')
-- 2D Invalid Transaction Termination
('2D', '000', 'Invalid transaction termination')
-- 2E Invalid Connection Name
('2E', '000', 'Invalid connection name')
-- 2F SQL Routine Exception
('2F', '000', 'SQL routine exception')
('2F', '002', 'Modifying SQL-data not permitted')
('2F', '003', 'Prohibited SQL-statement attempted')
('2F', '004', 'Reading SQL-data not permitted')
('2F', '005', 'Function executed no return statement')
-- 33 Invalid SQL Descriptor Name
('33', '000', 'Invalid SQL descriptor name')
-- 34 Invalid Cursor Name
('34', '000', 'Invalid cursor name')
-- 35 Invalid condition number
('35', '000', 'Invalid condition number')
-- 36 Cursor Sensitivity Exception
('36', '001', 'Request rejected')
('36', '002', 'Request failed')
-- External Routine Exception
('38', '000', 'External routine exception')
-- External Routine Invocation Exception
('39', '000', 'External routine invocation exception')
-- 3B Invalid Save Point
('3B', '000', 'Invalid save point')
-- 3C Ambiguous Cursor Name
('3C', '000', 'Ambiguous cursor name')
-- 3B Invalid Catalog Name
('3D', '000', 'Invalid catalog name')
-- 3D Catalog Name Not Found
('3D', '001', 'Catalog name not found')
-- 3F Invalid Schema Name
('3F', '000', 'Invalid schema name')
-- 40 Transaction Rollback
('40', '000', 'Ongoing transaction has been rolled back')
('40', '001', 'Serialization failure')
('40', '002', 'Transaction integrity constraint violation')
('40', '003', 'Statement completion unknown')
-- 42 Syntax Error or Access Violation
('42', '000', 'Syntax error or access violation')
('42', '702', 'Ambiguous column reference')
('42', '725', 'Ambiguous function reference')
('42', '818', 'The operands of an operator or function are not compatible')
('42', 'S01', 'Base table or view already exists')
('42', 'S02', 'Base table or view not found')
('42', 'S11', 'Index already exists')
('42', 'S12', 'Index not found')
('42', 'S21', 'Column already exists')
('42', 'S22', 'Column not found')
-- 44 With Check Option Violation
('44', '000', 'WITH CHECK OPTION violation')
-- 45 Unhandled user-defined exception
('45', '000', 'Unhandled user-defined exception')
-- 54 Program Limit Exceeded
('54', '000', 'Program limit exceeded')
('54', '001', 'Statement too complex')
('54', '011', 'Too many columns')
('54', '023', 'Too many arguments')
-- HY CLI-specific condition
('HY', '000', 'CLI-specific condition')
('HY', '001', 'Memory allocation error')
('HY', '003', 'Invalid data type in application descriptor')
('HY', '004', 'Invalid data type')
('HY', '007', 'Associated statement is not prepared')
('HY', '008', 'Operation canceled')
('HY', '009', 'Invalid use of null pointer')
('HY', '010', 'Function sequence error')
('HY', '011', 'Attribute cannot be set now')
('HY', '012', 'Invalid transaction operation code')
('HY', '013', 'Memory management error')
('HY', '014', 'Limit on the number of handles exceeded')
('HY', '015', 'No cursor name available')
('HY', '016', 'Cannot modify an implementation row descriptor')
('HY', '017', 'Invalid use of an automatically allocated descriptor handle')
('HY', '018', 'Server declined the cancellation request')
('HY', '019', 'Non-string data cannot be sent in pieces')
('HY', '020', 'Attempt to concatenate a null value')
('HY', '021', 'Inconsistent descriptor information')
('HY', '024', 'Invalid attribute value')
('HY', '055', 'Non-string data cannot be used with string routine')
('HY', '090', 'Invalid string length or buffer length')
('HY', '091', 'Invalid descriptor field identifier')
('HY', '092', 'Invalid attribute identifier')
('HY', '095', 'Invalid FunctionId specified')
('HY', '096', 'Invalid information type')
('HY', '097', 'Column type out of range')
('HY', '098', 'Scope out of range')
('HY', '099', 'Nullable type out of range')
('HY', '100', 'Uniqueness option type out of range')
('HY', '101', 'Accuracy option type out of range')
('HY', '103', 'Invalid retrieval code')
('HY', '104', 'Invalid LengthPrecision value')
('HY', '105', 'Invalid parameter type')
('HY', '106', 'Invalid fetch orientation')
('HY', '107', 'Row value out of range')
('HY', '109', 'Invalid cursor position')
('HY', '110', 'Invalid driver completion')
('HY', '111', 'Invalid bookmark value')
('HY', 'C00', 'Optional feature not implemented')
('HY', 'T00', 'Timeout expired')
('HY', 'T01', 'Connection timeout expired')
-- XX Internal Error
('XX', '000', 'Internal error')
('XX', '001', 'Data corrupted')
('XX', '002', 'Index corrupted')
-- Do not put spaces before this. The bulk insertion treats an empty line as end of input, unless it's inside a quoted string.
-- And empty line and stop cause the same effect, but stop is more evident.
stop

COMMIT WORK;
