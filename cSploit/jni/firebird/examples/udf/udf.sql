/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

/*--------------------------------------------------------------
**	User Defined Function definitions for example databases
**--------------------------------------------------------------
*/


DECLARE EXTERNAL FUNCTION lower 
	VARCHAR (256)	
	RETURNS CSTRING(80) FREE_IT
	ENTRY_POINT 'fn_lower_c' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION substr 
	CSTRING(256), SMALLINT, SMALLINT
	RETURNS CSTRING(80) FREE_IT
	ENTRY_POINT 'fn_substr' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION trim 
	CSTRING(256)
	RETURNS CHAR (80) FREE_IT
	ENTRY_POINT 'fn_trim' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION trunc 
	CSTRING(256), SMALLINT
	RETURNS VARCHAR (80) FREE_IT
	ENTRY_POINT 'fn_trunc' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION strcat 
	VARCHAR(255), VARCHAR (255)
	RETURNS CSTRING(80) FREE_IT
	ENTRY_POINT 'fn_strcat' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION doy 
	RETURNS INTEGER BY VALUE
	ENTRY_POINT 'fn_doy' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION moy 
	RETURNS SMALLINT 
	ENTRY_POINT 'fn_moy' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION dow 
	RETURNS CSTRING(12) 
	ENTRY_POINT 'fn_dow' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION sysdate 
	RETURNS CSTRING(12) FREE_IT
	ENTRY_POINT 'fn_sysdate' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION fact 
	DOUBLE PRECISION	
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'fn_fact' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION add2 
	INTEGER,INTEGER	
	RETURNS INTEGER BY VALUE 
	ENTRY_POINT 'fn_add2' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION mul 
	DOUBLE PRECISION, DOUBLE PRECISION	
	RETURNS DOUBLE PRECISION BY VALUE 
	ENTRY_POINT 'fn_mul' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION abs
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'fn_abs' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION maxnum 
	DOUBLE PRECISION, DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'fn_max' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION sqrt 
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION
	ENTRY_POINT 'fn_sqrt' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION BLOB_BYTECOUNT
	BLOB
	RETURNS INTEGER BY VALUE
	ENTRY_POINT 'fn_blob_bytecount' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION BLOB_LINECOUNT
	BLOB
	RETURNS INTEGER BY VALUE
	ENTRY_POINT 'fn_blob_linecount' MODULE_NAME 'udflib';

DECLARE EXTERNAL FUNCTION BLOB_SUBSTR 
	BLOB, INTEGER, INTEGER 
	RETURNS CSTRING(256) FREE_IT
	ENTRY_POINT 'fn_blob_substr' MODULE_NAME 'udflib';

