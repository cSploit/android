/*
 *	PROGRAM:	C preprocessor
 *	MODULE:		sqlda_pub.h
 *	DESCRIPTION:	Public DSQL definitions (included in ibase.h)
 *
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

#ifndef DSQL_SQLDA_PUB_H
#define DSQL_SQLDA_PUB_H

/* Definitions for DSQL free_statement routine */

#define DSQL_close		1
#define DSQL_drop		2
#define DSQL_unprepare	4

/* Declare the extended SQLDA */

typedef struct
{
	ISC_SHORT	sqltype;			/* datatype of field */
	ISC_SHORT	sqlscale;			/* scale factor */
	ISC_SHORT	sqlsubtype;			/* datatype subtype - currently BLOBs only */
	ISC_SHORT	sqllen;				/* length of data area */
	ISC_SCHAR*	sqldata;			/* address of data */
	ISC_SHORT*	sqlind;				/* address of indicator variable */
	ISC_SHORT	sqlname_length;		/* length of sqlname field */
	ISC_SCHAR	sqlname[32];		/* name of field, name length + space for NULL */
	ISC_SHORT	relname_length;		/* length of relation name */
	ISC_SCHAR	relname[32];		/* field's relation name + space for NULL */
	ISC_SHORT	ownname_length;		/* length of owner name */
	ISC_SCHAR	ownname[32];		/* relation's owner name + space for NULL */
	ISC_SHORT	aliasname_length;	/* length of alias name */
	ISC_SCHAR	aliasname[32];		/* relation's alias name + space for NULL */
} XSQLVAR;

#define SQLDA_VERSION1		1

typedef struct
{
	ISC_SHORT	version;			/* version of this XSQLDA */
	ISC_SCHAR	sqldaid[8];			/* XSQLDA name field */
	ISC_LONG	sqldabc;			/* length in bytes of SQLDA */
	ISC_SHORT	sqln;				/* number of fields allocated */
	ISC_SHORT	sqld;				/* actual number of fields */
	XSQLVAR	sqlvar[1];			/* first field address */
} XSQLDA;

#define XSQLDA_LENGTH(n)	(sizeof (XSQLDA) + (n - 1) * sizeof (XSQLVAR))

#define SQL_TEXT                           452
#define SQL_VARYING                        448
#define SQL_SHORT                          500
#define SQL_LONG                           496
#define SQL_FLOAT                          482
#define SQL_DOUBLE                         480
#define SQL_D_FLOAT                        530
#define SQL_TIMESTAMP                      510
#define SQL_BLOB                           520
#define SQL_ARRAY                          540
#define SQL_QUAD                           550
#define SQL_TYPE_TIME                      560
#define SQL_TYPE_DATE                      570
#define SQL_INT64                          580
#define SQL_NULL                         32766

/* Historical alias for pre v6 code */
#define SQL_DATE                           SQL_TIMESTAMP

/***************************/
/* SQL Dialects            */
/***************************/

#define SQL_DIALECT_V5				1	/* meaning is same as DIALECT_xsqlda */
#define SQL_DIALECT_V6_TRANSITION	2	/* flagging anything that is delimited
										   by double quotes as an error and
										   flagging keyword DATE as an error */
#define SQL_DIALECT_V6				3	/* supports SQL delimited identifier,
										   SQLDATE/DATE, TIME, TIMESTAMP,
										   CURRENT_DATE, CURRENT_TIME,
										   CURRENT_TIMESTAMP, and 64-bit exact
										   numeric type */
#define SQL_DIALECT_CURRENT		SQL_DIALECT_V6	/* latest IB DIALECT */

#endif /* DSQL_SQLDA_PUB_H */

