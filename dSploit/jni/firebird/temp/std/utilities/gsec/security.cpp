/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *
 *	PROGRAM:	Security data base manager
 *	MODULE:		security.epp
 *	DESCRIPTION:	Security routines
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

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../jrd/common.h"
#include "../jrd/ibase.h"
#include "../jrd/jrd_pwd.h"
#include "../jrd/enc_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../utilities/gsec/gsec.h"
#include "../utilities/gsec/secur_proto.h"
#include "../common/utils_proto.h"
#include "../common/classes/init.h"

/*DATABASE DB = STATIC FILENAME "security2.fdb";*/
/**** GDS Preprocessor Definitions ****/
#ifndef JRD_IBASE_H
#include <ibase.h>
#endif

static const ISC_QUAD
   isc_blob_null = {0, 0};	/* initializer for blobs */
static isc_db_handle
   DB = 0;		/* database handle */

static isc_tr_handle
   gds_trans = 0;		/* default transaction handle */
static ISC_STATUS
   isc_status [20],	/* status vector */
   isc_status2 [20];	/* status vector */
static ISC_LONG
   isc_array_length, 	/* array return size */
   SQLCODE;		/* SQL status code */
static const short
   isc_0l = 152;
static const char
   isc_0 [] = {
   5,2,4,1,1,0,7,0,4,0,1,0,40,-127,1,12,0,2,7,'C',1,'J',19,'R',
   'D','B','$','U','S','E','R','_','P','R','I','V','I','L','E',
   'G','E','S',0,'G',58,47,23,0,8,'R','D','B','$','U','S','E','R',
   25,0,0,0,58,47,23,0,17,'R','D','B','$','R','E','L','A','T','I',
   'O','N','_','N','A','M','E',21,14,9,0,'R','D','B','$','A','D',
   'M','I','N',47,23,0,13,'R','D','B','$','P','R','I','V','I','L',
   'E','G','E',21,14,1,0,'M',-1,14,1,2,1,21,8,0,1,0,0,0,25,1,0,
   0,-1,14,1,1,21,8,0,0,0,0,0,25,1,0,0,-1,-1,'L'
   };	/* end of blr string for request isc_0 */

static const short
   isc_5l = 211;
static const char
   isc_5 [] = {
   5,2,4,1,8,0,8,0,8,0,7,0,40,-127,1,40,-127,1,40,97,0,40,97,0,
   40,97,0,4,0,1,0,40,-127,1,12,0,2,7,'C',1,'J',5,'U','S','E','R',
   'S',0,'G',47,23,0,9,'U','S','E','R','_','N','A','M','E',25,0,
   0,0,-1,14,1,2,1,23,0,3,'G','I','D',25,1,0,0,1,23,0,3,'U','I',
   'D',25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,1,23,0,9,'U','S','E',
   'R','_','N','A','M','E',25,1,3,0,1,23,0,10,'G','R','O','U','P',
   '_','N','A','M','E',25,1,4,0,1,23,0,10,'F','I','R','S','T','_',
   'N','A','M','E',25,1,5,0,1,23,0,11,'M','I','D','D','L','E','_',
   'N','A','M','E',25,1,6,0,1,23,0,9,'L','A','S','T','_','N','A',
   'M','E',25,1,7,0,-1,14,1,1,21,8,0,0,0,0,0,25,1,2,0,-1,-1,'L'
   
   };	/* end of blr string for request isc_5 */

static const short
   isc_17l = 152;
static const char
   isc_17 [] = {
   5,2,4,1,1,0,7,0,4,0,1,0,40,-127,1,12,0,2,7,'C',1,'J',19,'R',
   'D','B','$','U','S','E','R','_','P','R','I','V','I','L','E',
   'G','E','S',0,'G',58,47,23,0,8,'R','D','B','$','U','S','E','R',
   25,0,0,0,58,47,23,0,17,'R','D','B','$','R','E','L','A','T','I',
   'O','N','_','N','A','M','E',21,14,9,0,'R','D','B','$','A','D',
   'M','I','N',47,23,0,13,'R','D','B','$','P','R','I','V','I','L',
   'E','G','E',21,14,1,0,'M',-1,14,1,2,1,21,8,0,1,0,0,0,25,1,0,
   0,-1,14,1,1,21,8,0,0,0,0,0,25,1,0,0,-1,-1,'L'
   };	/* end of blr string for request isc_17 */

static const short
   isc_22l = 184;
static const char
   isc_22 [] = {
   5,2,4,0,8,0,8,0,8,0,7,0,40,-127,1,40,-127,1,40,97,0,40,97,0,
   40,97,0,2,7,'C',1,'J',5,'U','S','E','R','S',0,-1,14,0,2,1,23,
   0,3,'G','I','D',25,0,0,0,1,23,0,3,'U','I','D',25,0,1,0,1,21,
   8,0,1,0,0,0,25,0,2,0,1,23,0,9,'U','S','E','R','_','N','A','M',
   'E',25,0,3,0,1,23,0,10,'G','R','O','U','P','_','N','A','M','E',
   25,0,4,0,1,23,0,10,'F','I','R','S','T','_','N','A','M','E',25,
   0,5,0,1,23,0,11,'M','I','D','D','L','E','_','N','A','M','E',
   25,0,6,0,1,23,0,9,'L','A','S','T','_','N','A','M','E',25,0,7,
   0,-1,14,0,1,21,8,0,0,0,0,0,25,0,2,0,-1,-1,'L'
   };	/* end of blr string for request isc_22 */

static const short
   isc_32l = 109;
static const char
   isc_32 [] = {
   5,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,40,-127,1,12,
   0,2,7,'C',1,'J',5,'U','S','E','R','S',0,'G',47,23,0,9,'U','S',
   'E','R','_','N','A','M','E',25,0,0,0,-1,2,14,1,2,1,21,8,0,1,
   0,0,0,25,1,0,0,-1,17,0,9,13,12,3,18,0,12,2,11,5,0,-1,-1,14,1,
   1,21,8,0,0,0,0,0,25,1,0,0,-1,-1,'L'
   };	/* end of blr string for request isc_32 */

static const short
   isc_41l = 420;
static const char
   isc_41 [] = {
   5,2,4,3,1,0,7,0,4,2,14,0,8,0,8,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
   40,97,0,40,97,0,40,97,0,40,'A',0,40,-127,1,4,1,15,0,8,0,8,0,
   7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,40,-127,1,40,'A',0,40,97,0,40,
   97,0,40,97,0,4,0,1,0,40,-127,1,12,0,2,7,'C',1,'J',5,'U','S',
   'E','R','S',0,'G',47,23,0,9,'U','S','E','R','_','N','A','M',
   'E',25,0,0,0,-1,2,14,1,2,1,23,0,3,'G','I','D',41,1,0,0,8,0,1,
   23,0,3,'U','I','D',41,1,1,0,9,0,1,21,8,0,1,0,0,0,25,1,2,0,1,
   23,0,10,'G','R','O','U','P','_','N','A','M','E',41,1,10,0,7,
   0,1,23,0,6,'P','A','S','S','W','D',41,1,11,0,6,0,1,23,0,10,'F',
   'I','R','S','T','_','N','A','M','E',41,1,12,0,5,0,1,23,0,11,
   'M','I','D','D','L','E','_','N','A','M','E',41,1,13,0,4,0,1,
   23,0,9,'L','A','S','T','_','N','A','M','E',41,1,14,0,3,0,-1,
   17,0,9,13,12,3,18,0,12,2,11,10,0,1,2,1,41,2,9,0,8,0,23,1,9,'L',
   'A','S','T','_','N','A','M','E',1,41,2,10,0,7,0,23,1,11,'M',
   'I','D','D','L','E','_','N','A','M','E',1,41,2,11,0,6,0,23,1,
   10,'F','I','R','S','T','_','N','A','M','E',1,41,2,12,0,5,0,23,
   1,6,'P','A','S','S','W','D',1,41,2,13,0,4,0,23,1,10,'G','R',
   'O','U','P','_','N','A','M','E',1,41,2,1,0,3,0,23,1,3,'G','I',
   'D',1,41,2,0,0,2,0,23,1,3,'U','I','D',-1,-1,-1,14,1,1,21,8,0,
   0,0,0,0,25,1,2,0,-1,-1,'L'
   };	/* end of blr string for request isc_41 */

static const short
   isc_77l = 196;
static const char
   isc_77 [] = {
   5,2,4,0,15,0,8,0,8,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,40,-127,1,40,
   -127,1,40,'A',0,40,97,0,40,97,0,40,97,0,12,0,15,'J',5,'U','S',
   'E','R','S',0,2,1,41,0,14,0,2,0,23,0,9,'L','A','S','T','_','N',
   'A','M','E',1,41,0,13,0,3,0,23,0,11,'M','I','D','D','L','E',
   '_','N','A','M','E',1,41,0,12,0,4,0,23,0,10,'F','I','R','S',
   'T','_','N','A','M','E',1,41,0,11,0,5,0,23,0,6,'P','A','S','S',
   'W','D',1,41,0,10,0,6,0,23,0,10,'G','R','O','U','P','_','N',
   'A','M','E',1,41,0,0,0,7,0,23,0,3,'G','I','D',1,41,0,1,0,8,0,
   23,0,3,'U','I','D',1,25,0,9,0,23,0,9,'U','S','E','R','_','N',
   'A','M','E',-1,-1,'L'
   };	/* end of blr string for request isc_77 */

static const short
   isc_94l = 174;
static const char
   isc_94 [] = {
   5,2,4,1,2,0,40,32,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',1,'J',19,
   'R','D','B','$','U','S','E','R','_','P','R','I','V','I','L',
   'E','G','E','S',0,'G',58,47,23,0,8,'R','D','B','$','U','S','E',
   'R',25,0,0,0,58,47,23,0,17,'R','D','B','$','R','E','L','A','T',
   'I','O','N','_','N','A','M','E',21,14,9,0,'R','D','B','$','A',
   'D','M','I','N',47,23,0,13,'R','D','B','$','P','R','I','V','I',
   'L','E','G','E',21,14,1,0,'M',-1,14,1,2,1,23,0,11,'R','D','B',
   '$','G','R','A','N','T','O','R',25,1,0,0,1,21,8,0,1,0,0,0,25,
   1,1,0,-1,14,1,1,21,8,0,0,0,0,0,25,1,1,0,-1,-1,'L'
   };	/* end of blr string for request isc_94 */


#define gds_blob_null	isc_blob_null	/* compatibility symbols */
#define gds_status	isc_status
#define gds_status2	isc_status2
#define gds_array_length	isc_array_length
#define gds_count	isc_count
#define gds_slack	isc_slack
#define gds_utility	isc_utility	/* end of compatibility symbols */

#ifndef isc_version4
    Generate a compile-time error.
    Picking up a V3 include file after preprocessing with V4 GPRE.
#endif

/**** end of GPRE definitions ****/


static Firebird::GlobalPtr<Firebird::Mutex> execLineMutex;	// protects various gpre generated structures

static bool grantRevokeAdmin(ISC_STATUS* isc_status, FB_API_HANDLE DB, FB_API_HANDLE trans,
	const internal_user_data* io_user_data)
{
   struct isc_97_struct {
          char  isc_98 [32];	/* RDB$GRANTOR */
          short isc_99;	/* isc_utility */
   } isc_97;
   struct isc_95_struct {
          char  isc_96 [32];	/* RDB$USER */
   } isc_95;
	if (!io_user_data->admin_entered)
	{
		return true;
	}

	Firebird::string sql;

	sql.printf(io_user_data->admin ? "GRANT %s TO \"%s\"" : "REVOKE %s FROM \"%s\"", 
			"RDB$ADMIN", io_user_data->user_name);
	isc_dsql_execute_immediate(isc_status, &DB, &trans, sql.length(), sql.c_str(), SQL_DIALECT_V6, NULL);

	if (isc_status[1] && io_user_data->admin == 0)
	{
		isc_req_handle request = 0;
		/*FOR (TRANSACTION_HANDLE trans REQUEST_HANDLE request) R IN RDB$USER_PRIVILEGES
				WITH R.RDB$USER EQ io_user_data->user_name 
				 AND R.RDB$RELATION_NAME EQ 'RDB$ADMIN'
				 AND R.RDB$PRIVILEGE EQ 'M'*/
		{
                if (!request)
                   isc_compile_request (NULL, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request, (short) sizeof(isc_94), (char*) isc_94);
		isc_vtov ((const char*) io_user_data->user_name, (char*) isc_95.isc_96, 32);
                isc_start_and_send (NULL, (FB_API_HANDLE*) &request, (FB_API_HANDLE*) &trans, (short) 0, (short) 32, &isc_95, (short) 0);
		while (1)
		   {
                   isc_receive (NULL, (FB_API_HANDLE*) &request, (short) 1, (short) 34, &isc_97, (short) 0);
		   if (!isc_97.isc_99) break;
			sql.printf("REVOKE RDB$ADMIN FROM \"%s\" GRANTED BY \"%s\"", 
				io_user_data->user_name, /*R.RDB$GRANTOR*/
							 isc_97.isc_98);
		/*END_FOR*/
		   }
		}
		if (request)
		{
			ISC_STATUS_ARRAY s;
			if (isc_release_request(s, &request) != FB_SUCCESS)
			{
				return false;
			}
		}
		isc_dsql_execute_immediate(isc_status, &DB, &trans, sql.length(), sql.c_str(), SQL_DIALECT_V6, NULL);
	}

	return isc_status[1] == 0;
}


SSHORT SECURITY_exec_line(ISC_STATUS* isc_status,
						  FB_API_HANDLE DB,
						  FB_API_HANDLE trans,
						  internal_user_data* io_user_data,
						  FPTR_SECURITY_CALLBACK display_func,
						  void* callback_arg)
{
   struct isc_3_struct {
          short isc_4;	/* isc_utility */
   } isc_3;
   struct isc_1_struct {
          char  isc_2 [385];	/* USER_NAME */
   } isc_1;
   struct isc_8_struct {
          ISC_LONG isc_9;	/* GID */
          ISC_LONG isc_10;	/* UID */
          short isc_11;	/* isc_utility */
          char  isc_12 [385];	/* USER_NAME */
          char  isc_13 [385];	/* GROUP_NAME */
          char  isc_14 [97];	/* FIRST_NAME */
          char  isc_15 [97];	/* MIDDLE_NAME */
          char  isc_16 [97];	/* LAST_NAME */
   } isc_8;
   struct isc_6_struct {
          char  isc_7 [385];	/* USER_NAME */
   } isc_6;
   struct isc_20_struct {
          short isc_21;	/* isc_utility */
   } isc_20;
   struct isc_18_struct {
          char  isc_19 [385];	/* USER_NAME */
   } isc_18;
   struct isc_23_struct {
          ISC_LONG isc_24;	/* GID */
          ISC_LONG isc_25;	/* UID */
          short isc_26;	/* isc_utility */
          char  isc_27 [385];	/* USER_NAME */
          char  isc_28 [385];	/* GROUP_NAME */
          char  isc_29 [97];	/* FIRST_NAME */
          char  isc_30 [97];	/* MIDDLE_NAME */
          char  isc_31 [97];	/* LAST_NAME */
   } isc_23;
   struct isc_39_struct {
          short isc_40;	/* isc_utility */
   } isc_39;
   struct isc_37_struct {
          short isc_38;	/* isc_utility */
   } isc_37;
   struct isc_35_struct {
          short isc_36;	/* isc_utility */
   } isc_35;
   struct isc_33_struct {
          char  isc_34 [385];	/* USER_NAME */
   } isc_33;
   struct isc_75_struct {
          short isc_76;	/* isc_utility */
   } isc_75;
   struct isc_60_struct {
          ISC_LONG isc_61;	/* UID */
          ISC_LONG isc_62;	/* GID */
          short isc_63;	/* gds__null_flag */
          short isc_64;	/* gds__null_flag */
          short isc_65;	/* gds__null_flag */
          short isc_66;	/* gds__null_flag */
          short isc_67;	/* gds__null_flag */
          short isc_68;	/* gds__null_flag */
          short isc_69;	/* gds__null_flag */
          char  isc_70 [97];	/* LAST_NAME */
          char  isc_71 [97];	/* MIDDLE_NAME */
          char  isc_72 [97];	/* FIRST_NAME */
          char  isc_73 [65];	/* PASSWD */
          char  isc_74 [385];	/* GROUP_NAME */
   } isc_60;
   struct isc_44_struct {
          ISC_LONG isc_45;	/* GID */
          ISC_LONG isc_46;	/* UID */
          short isc_47;	/* isc_utility */
          short isc_48;	/* gds__null_flag */
          short isc_49;	/* gds__null_flag */
          short isc_50;	/* gds__null_flag */
          short isc_51;	/* gds__null_flag */
          short isc_52;	/* gds__null_flag */
          short isc_53;	/* gds__null_flag */
          short isc_54;	/* gds__null_flag */
          char  isc_55 [385];	/* GROUP_NAME */
          char  isc_56 [65];	/* PASSWD */
          char  isc_57 [97];	/* FIRST_NAME */
          char  isc_58 [97];	/* MIDDLE_NAME */
          char  isc_59 [97];	/* LAST_NAME */
   } isc_44;
   struct isc_42_struct {
          char  isc_43 [385];	/* USER_NAME */
   } isc_42;
   struct isc_78_struct {
          ISC_LONG isc_79;	/* GID */
          ISC_LONG isc_80;	/* UID */
          short isc_81;	/* gds__null_flag */
          short isc_82;	/* gds__null_flag */
          short isc_83;	/* gds__null_flag */
          short isc_84;	/* gds__null_flag */
          short isc_85;	/* gds__null_flag */
          short isc_86;	/* gds__null_flag */
          short isc_87;	/* gds__null_flag */
          char  isc_88 [385];	/* USER_NAME */
          char  isc_89 [385];	/* GROUP_NAME */
          char  isc_90 [65];	/* PASSWD */
          char  isc_91 [97];	/* FIRST_NAME */
          char  isc_92 [97];	/* MIDDLE_NAME */
          char  isc_93 [97];	/* LAST_NAME */
   } isc_78;
/*************************************
 *
 *	S E C U R I T Y _ e x e c _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Process a command line for the security data base manager.
 *	This is used to add and delete users from the user information
 *	database (security2.fdb).   It also displays information
 *	about current users and allows modification of current
 *	users' parameters.
 *	Returns 0 on success, otherwise returns a Gsec message number
 *	and the status vector containing the error info.
 *	The syntax is:
 *
 *	Adding a new user:
 *
 *	    gsec -add <name> [ <parameter> ... ]    -- command line
 *	    add <name> [ <parameter> ... ]          -- interactive
 *
 *	Deleting a current user:
 *
 *	    gsec -delete <name>     -- command line
 *	    delete <name>           -- interactive
 *
 *	Displaying all current users:
 *
 *	    gsec -display           -- command line
 *	    display                 -- interactive
 *
 *	Displaying one user:
 *
 *	    gsec -display <name>    -- command line
 *	    display <name>          -- interactive
 *
 *	Modifying a user's parameters:
 *
 *	    gsec -modify <name> <parameter> [ <parameter> ... ] -- command line
 *	    modify <name> <parameter> [ <parameter> ... ]       -- interactive
 *
 *	Get help:
 *
 *	    gsec -help              -- command line
 *	    ?                       -- interactive
 *	    help                    -- interactive
 *
 *	Quit interactive session:
 *
 *	    quit                    -- interactive
 *
 *	where <parameter> can be one of:
 *
 *	    -uid <uid>
 *	    -gid <gid>
 *	    -fname <firstname>
 *	    -mname <middlename>
 *	    -lname <lastname>
 *
 **************************************/
	Firebird::MutexLockGuard guard(execLineMutex);

	SCHAR encrypted1[MAX_PASSWORD_LENGTH + 2];
	Firebird::string encrypted2;
	bool found;
	SSHORT ret = 0;

	// check for non-printable characters in user name
	for (const TEXT* p = io_user_data->user_name; *p; p++)
	{
		if (! isprint(*p)) {
			return GsecMsg75;  // Add special error message for this case ?
		}
	}

	isc_req_handle request = 0;
	isc_req_handle request2 = 0;

	switch (io_user_data->operation)
	{
	case MAP_DROP_OPER:
	case MAP_SET_OPER:
		{
			Firebird::string sql;
			sql.printf("ALTER ROLE RDB$ADMIN %s AUTO ADMIN MAPPING",
				io_user_data->operation == MAP_SET_OPER ? "SET" : "DROP");
			isc_dsql_execute_immediate(isc_status, &DB, &trans, sql.length(), sql.c_str(), 1, NULL);
			if (isc_status[1] != 0)
			{
				ret = GsecMsg97;
			}
		}
		break;
	case ADD_OPER:
		// this checks the "entered" flags for each parameter (except the name)
		// and makes all non-entered parameters null valued

		/*STORE (TRANSACTION_HANDLE trans REQUEST_HANDLE request) U IN USERS USING*/
		{
		
                if (!request)
                   isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request, (short) sizeof(isc_77), (char*) isc_77);
		if (request)
		   {
			strcpy(/*U.USER_NAME*/
			       isc_78.isc_88, io_user_data->user_name);
			if (io_user_data->uid_entered)
			{
				/*U.UID*/
				isc_78.isc_80 = io_user_data->uid;
				/*U.UID.NULL*/
				isc_78.isc_87 = ISC_FALSE;
			}
			else
				/*U.UID.NULL*/
				isc_78.isc_87 = ISC_TRUE;
			if (io_user_data->gid_entered)
			{
				/*U.GID*/
				isc_78.isc_79 = io_user_data->gid;
				/*U.GID.NULL*/
				isc_78.isc_86 = ISC_FALSE;
			}
			else
				/*U.GID.NULL*/
				isc_78.isc_86 = ISC_TRUE;
			if (io_user_data->group_name_entered)
			{
				strcpy(/*U.GROUP_NAME*/
				       isc_78.isc_89, io_user_data->group_name);
				/*U.GROUP_NAME.NULL*/
				isc_78.isc_85 = ISC_FALSE;
			}
			else
				/*U.GROUP_NAME.NULL*/
				isc_78.isc_85 = ISC_TRUE;
			if (io_user_data->password_entered)
			{
				ENC_crypt(encrypted1, sizeof encrypted1, io_user_data->password, PASSWORD_SALT);
				Jrd::SecurityDatabase::hash(encrypted2, io_user_data->user_name, &encrypted1[2]);
				strcpy(/*U.PASSWD*/
				       isc_78.isc_90, encrypted2.c_str());
				/*U.PASSWD.NULL*/
				isc_78.isc_84 = ISC_FALSE;
			}
			else
				/*U.PASSWD.NULL*/
				isc_78.isc_84 = ISC_TRUE;
			if (io_user_data->first_name_entered)
			{
				strcpy(/*U.FIRST_NAME*/
				       isc_78.isc_91, io_user_data->first_name);
				/*U.FIRST_NAME.NULL*/
				isc_78.isc_83 = ISC_FALSE;
			}
			else
				/*U.FIRST_NAME.NULL*/
				isc_78.isc_83 = ISC_TRUE;
			if (io_user_data->middle_name_entered)
			{
				strcpy(/*U.MIDDLE_NAME*/
				       isc_78.isc_92, io_user_data->middle_name);
				/*U.MIDDLE_NAME.NULL*/
				isc_78.isc_82 = ISC_FALSE;
			}
			else
				/*U.MIDDLE_NAME.NULL*/
				isc_78.isc_82 = ISC_TRUE;
			if (io_user_data->last_name_entered)
			{
				strcpy(/*U.LAST_NAME*/
				       isc_78.isc_93, io_user_data->last_name);
				/*U.LAST_NAME.NULL*/
				isc_78.isc_81 = ISC_FALSE;
			}
			else
				/*U.LAST_NAME.NULL*/
				isc_78.isc_81 = ISC_TRUE;
		/*END_STORE*/
		   
                   isc_start_and_send (isc_status, (FB_API_HANDLE*) &request, (FB_API_HANDLE*) &trans, (short) 0, (short) 1148, &isc_78, (short) 0);
		   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			ret = GsecMsg19;	// gsec - add record error
		/*END_ERROR;*/
		   }
		}
		if (ret == 0 && !grantRevokeAdmin(isc_status, DB, trans, io_user_data))
		{
			ret = GsecMsg19;	// gsec - add record error
		}
		break;

	case MOD_OPER:
		// this updates an existing record, replacing all fields that are
		// entered, and for those that were specified but not entered, it
		// changes the current value to the null value

		found = false;
		/*FOR (TRANSACTION_HANDLE trans REQUEST_HANDLE request) U IN USERS
				WITH U.USER_NAME EQ io_user_data->user_name*/
		{
                if (!request)
                   isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request, (short) sizeof(isc_41), (char*) isc_41);
		isc_vtov ((const char*) io_user_data->user_name, (char*) isc_42.isc_43, 385);
		if (request)
                   isc_start_and_send (isc_status, (FB_API_HANDLE*) &request, (FB_API_HANDLE*) &trans, (short) 0, (short) 385, &isc_42, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, (FB_API_HANDLE*) &request, (short) 1, (short) 765, &isc_44, (short) 0);
		   if (!isc_44.isc_47 || isc_status [1]) break;
			found = true;
			/*MODIFY U USING*/
			{
				if (io_user_data->uid_entered)
				{
					/*U.UID*/
					isc_44.isc_46 = io_user_data->uid;
					/*U.UID.NULL*/
					isc_44.isc_54 = ISC_FALSE;
				}
				else if (io_user_data->uid_specified)
					/*U.UID.NULL*/
					isc_44.isc_54 = ISC_TRUE;
				if (io_user_data->gid_entered)
				{
					/*U.GID*/
					isc_44.isc_45 = io_user_data->gid;
					/*U.GID.NULL*/
					isc_44.isc_53 = ISC_FALSE;
				}
				else if (io_user_data->gid_specified)
					/*U.GID.NULL*/
					isc_44.isc_53 = ISC_TRUE;
				if (io_user_data->group_name_entered)
				{
					strcpy(/*U.GROUP_NAME*/
					       isc_44.isc_55, io_user_data->group_name);
					/*U.GROUP_NAME.NULL*/
					isc_44.isc_52 = ISC_FALSE;
				}
				else if (io_user_data->group_name_specified)
					/*U.GROUP_NAME.NULL*/
					isc_44.isc_52 = ISC_TRUE;
				if (io_user_data->password_entered)
				{
					ENC_crypt(encrypted1, sizeof encrypted1, io_user_data->password, PASSWORD_SALT);
					Jrd::SecurityDatabase::hash(encrypted2, io_user_data->user_name, &encrypted1[2]);
					strcpy(/*U.PASSWD*/
					       isc_44.isc_56, encrypted2.c_str());
					/*U.PASSWD.NULL*/
					isc_44.isc_51 = ISC_FALSE;
				}
				else if (io_user_data->password_specified)
					/*U.PASSWD.NULL*/
					isc_44.isc_51 = ISC_TRUE;
				if (io_user_data->first_name_entered)
				{
					strcpy(/*U.FIRST_NAME*/
					       isc_44.isc_57, io_user_data->first_name);
					/*U.FIRST_NAME.NULL*/
					isc_44.isc_50 = ISC_FALSE;
				}
				else if (io_user_data->first_name_specified)
					/*U.FIRST_NAME.NULL*/
					isc_44.isc_50 = ISC_TRUE;
				if (io_user_data->middle_name_entered)
				{
					strcpy(/*U.MIDDLE_NAME*/
					       isc_44.isc_58, io_user_data->middle_name);
					/*U.MIDDLE_NAME.NULL*/
					isc_44.isc_49 = ISC_FALSE;
				}
				else if (io_user_data->middle_name_specified)
					/*U.MIDDLE_NAME.NULL*/
					isc_44.isc_49 = ISC_TRUE;
				if (io_user_data->last_name_entered)
				{
					strcpy(/*U.LAST_NAME*/
					       isc_44.isc_59, io_user_data->last_name);
					/*U.LAST_NAME.NULL*/
					isc_44.isc_48 = ISC_FALSE;
				}
				else if (io_user_data->last_name_specified)
					/*U.LAST_NAME.NULL*/
					isc_44.isc_48 = ISC_TRUE;
			/*END_MODIFY*/
			isc_60.isc_61 = isc_44.isc_46;
			isc_60.isc_62 = isc_44.isc_45;
			isc_60.isc_63 = isc_44.isc_54;
			isc_60.isc_64 = isc_44.isc_53;
			isc_60.isc_65 = isc_44.isc_52;
			isc_60.isc_66 = isc_44.isc_51;
			isc_60.isc_67 = isc_44.isc_50;
			isc_60.isc_68 = isc_44.isc_49;
			isc_60.isc_69 = isc_44.isc_48;
			isc_vtov ((const char*) isc_44.isc_59, (char*) isc_60.isc_70, 97);
			isc_vtov ((const char*) isc_44.isc_58, (char*) isc_60.isc_71, 97);
			isc_vtov ((const char*) isc_44.isc_57, (char*) isc_60.isc_72, 97);
			isc_vtov ((const char*) isc_44.isc_56, (char*) isc_60.isc_73, 65);
			isc_vtov ((const char*) isc_44.isc_55, (char*) isc_60.isc_74, 385);
                        isc_send (isc_status, (FB_API_HANDLE*) &request, (short) 2, (short) 763, &isc_60, (short) 0);;
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				ret = GsecMsg20;
			/*END_ERROR;*/
			   }
			}
		/*END_FOR*/
                   isc_send (isc_status, (FB_API_HANDLE*) &request, (short) 3, (short) 2, &isc_75, (short) 0);
		   }
		   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			ret = GsecMsg21;
		/*END_ERROR;*/
		   }
		}
		if (!ret && !found)
			ret = GsecMsg22;
		if (ret == 0 && !grantRevokeAdmin(isc_status, DB, trans, io_user_data))
		{
			ret = GsecMsg21;
		}
		break;

	case DEL_OPER:
		// looks up the specified user record and deletes it

		found = false;
		// Do not allow SYSDBA user to be deleted
		if (!fb_utils::stricmp(io_user_data->user_name, SYSDBA_USER_NAME))
			ret = GsecMsg23;
		else
		{
			/*FOR (TRANSACTION_HANDLE trans REQUEST_HANDLE request) U IN USERS
					WITH U.USER_NAME EQ io_user_data->user_name*/
			{
                        if (!request)
                           isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request, (short) sizeof(isc_32), (char*) isc_32);
			isc_vtov ((const char*) io_user_data->user_name, (char*) isc_33.isc_34, 385);
			if (request)
                           isc_start_and_send (isc_status, (FB_API_HANDLE*) &request, (FB_API_HANDLE*) &trans, (short) 0, (short) 385, &isc_33, (short) 0);
			if (!isc_status [1]) {
			while (1)
			   {
                           isc_receive (isc_status, (FB_API_HANDLE*) &request, (short) 1, (short) 2, &isc_35, (short) 0);
			   if (!isc_35.isc_36 || isc_status [1]) break;
				found = true;
				/*ERASE U*/
				{
                                isc_send (isc_status, (FB_API_HANDLE*) &request, (short) 2, (short) 2, &isc_37, (short) 0);
				/*ON_ERROR*/
				if (isc_status [1])
				   {
					ret = GsecMsg23;	// gsec - delete record error
				/*END_ERROR;*/
				   }
				}
			/*END_FOR*/
                           isc_send (isc_status, (FB_API_HANDLE*) &request, (short) 3, (short) 2, &isc_39, (short) 0);
			   }
			   };
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				ret = GsecMsg24;	// gsec - find/delete record error
			/*END_ERROR;*/
			   }
			}
		}

		if (!ret && !found)
			ret = GsecMsg22;	// gsec - record not found for user:

		io_user_data->admin = 0;
		io_user_data->admin_entered = true;
		if (ret == 0 && ! grantRevokeAdmin(isc_status, DB, trans, io_user_data))
		{
			ret = GsecMsg24;
		}
		break;

	case DIS_OPER:
	case OLD_DIS_OPER:
		// gets either the desired record, or all records, and displays them

		found = false;
		if (!io_user_data->user_name_entered)
		{
			/*FOR (TRANSACTION_HANDLE trans REQUEST_HANDLE request) U IN USERS*/
			{
                        if (!request)
                           isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request, (short) sizeof(isc_22), (char*) isc_22);
			if (request)
                           isc_start_request (isc_status, (FB_API_HANDLE*) &request, (FB_API_HANDLE*) &trans, (short) 0);
			if (!isc_status [1]) {
			while (1)
			   {
                           isc_receive (isc_status, (FB_API_HANDLE*) &request, (short) 0, (short) 1071, &isc_23, (short) 0);
			   if (!isc_23.isc_26 || isc_status [1]) break;
				io_user_data->uid = /*U.UID*/
						    isc_23.isc_25;
				io_user_data->gid = /*U.GID*/
						    isc_23.isc_24;
				*(io_user_data->sys_user_name) = '\0';
				strcpy(io_user_data->user_name, /*U.USER_NAME*/
								isc_23.isc_27);
				strcpy(io_user_data->group_name, /*U.GROUP_NAME*/
								 isc_23.isc_28);
				io_user_data->password[0] = 0;
				strcpy(io_user_data->first_name, /*U.FIRST_NAME*/
								 isc_23.isc_29);
				strcpy(io_user_data->middle_name, /*U.MIDDLE_NAME*/
								  isc_23.isc_30);
				strcpy(io_user_data->last_name, /*U.LAST_NAME*/
								isc_23.isc_31);

				io_user_data->admin = 0;
				/*FOR (TRANSACTION_HANDLE trans REQUEST_HANDLE request2) P IN RDB$USER_PRIVILEGES
						WITH P.RDB$USER EQ U.USER_NAME
						 AND P.RDB$RELATION_NAME EQ 'RDB$ADMIN'
						 AND P.RDB$PRIVILEGE EQ 'M'*/
				{
                                if (!request2)
                                   isc_compile_request (NULL, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request2, (short) sizeof(isc_17), (char*) isc_17);
				isc_vtov ((const char*) isc_23.isc_27, (char*) isc_18.isc_19, 385);
                                isc_start_and_send (NULL, (FB_API_HANDLE*) &request2, (FB_API_HANDLE*) &trans, (short) 0, (short) 385, &isc_18, (short) 0);
				while (1)
				   {
                                   isc_receive (NULL, (FB_API_HANDLE*) &request2, (short) 1, (short) 2, &isc_20, (short) 0);
				   if (!isc_20.isc_21) break;
					io_user_data->admin = 1;
				/*END_FOR*/
				   }
				}

				display_func(callback_arg, io_user_data, !found);

				found = true;
			/*END_FOR*/
			   }
			   };
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				ret = GsecMsg28;	// gsec - find/display record error
			/*END_ERROR;*/
			   }
			}
		}
		else
		{
			/*FOR (TRANSACTION_HANDLE trans REQUEST_HANDLE request) U IN USERS
					WITH U.USER_NAME EQ io_user_data->user_name*/
			{
                        if (!request)
                           isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request, (short) sizeof(isc_5), (char*) isc_5);
			isc_vtov ((const char*) io_user_data->user_name, (char*) isc_6.isc_7, 385);
			if (request)
                           isc_start_and_send (isc_status, (FB_API_HANDLE*) &request, (FB_API_HANDLE*) &trans, (short) 0, (short) 385, &isc_6, (short) 0);
			if (!isc_status [1]) {
			while (1)
			   {
                           isc_receive (isc_status, (FB_API_HANDLE*) &request, (short) 1, (short) 1071, &isc_8, (short) 0);
			   if (!isc_8.isc_11 || isc_status [1]) break;
				io_user_data->uid = /*U.UID*/
						    isc_8.isc_10;
				io_user_data->gid = /*U.GID*/
						    isc_8.isc_9;
				*(io_user_data->sys_user_name) = '\0';
				strcpy(io_user_data->user_name, /*U.USER_NAME*/
								isc_8.isc_12);
				strcpy(io_user_data->group_name, /*U.GROUP_NAME*/
								 isc_8.isc_13);
				io_user_data->password[0] = 0;
				strcpy(io_user_data->first_name, /*U.FIRST_NAME*/
								 isc_8.isc_14);
				strcpy(io_user_data->middle_name, /*U.MIDDLE_NAME*/
								  isc_8.isc_15);
				strcpy(io_user_data->last_name, /*U.LAST_NAME*/
								isc_8.isc_16);

				io_user_data->admin = 0;
				/*FOR (TRANSACTION_HANDLE trans REQUEST_HANDLE request2) P IN RDB$USER_PRIVILEGES
						WITH P.RDB$USER EQ U.USER_NAME
						 AND P.RDB$RELATION_NAME EQ 'RDB$ADMIN'
						 AND P.RDB$PRIVILEGE EQ 'M'*/
				{
                                if (!request2)
                                   isc_compile_request (NULL, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request2, (short) sizeof(isc_0), (char*) isc_0);
				isc_vtov ((const char*) isc_8.isc_12, (char*) isc_1.isc_2, 385);
                                isc_start_and_send (NULL, (FB_API_HANDLE*) &request2, (FB_API_HANDLE*) &trans, (short) 0, (short) 385, &isc_1, (short) 0);
				while (1)
				   {
                                   isc_receive (NULL, (FB_API_HANDLE*) &request2, (short) 1, (short) 2, &isc_3, (short) 0);
				   if (!isc_3.isc_4) break;
					io_user_data->admin = 1;
				/*END_FOR*/
				   }
				}

				display_func(callback_arg, io_user_data, !found);

				found = true;
			/*END_FOR*/
			   }
			   };
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				ret = GsecMsg28;	// gsec - find/display record error
			/*END_ERROR;*/
			   }
			}
		}
		break;

	default:
		ret = GsecMsg16;		// gsec - error in switch specifications
		break;
	}

	if (request)
	{
		ISC_STATUS_ARRAY s;
		if (isc_release_request(s, &request) != FB_SUCCESS)
		{
			if (! ret)
			{
				ret = GsecMsg94;	// error releasing request in security database
			}
		}
	}

	if (request2)
	{
		ISC_STATUS_ARRAY s;
		if (isc_release_request(s, &request2) != FB_SUCCESS)
		{
			if (! ret)
			{
				ret = GsecMsg94;	// error releasing request in security database
			}
		}
	}

	return ret;
}

SSHORT SECURITY_exec_line(ISC_STATUS* isc_status,
						  FB_API_HANDLE DB,
						  internal_user_data* io_user_data,
						  FPTR_SECURITY_CALLBACK display_func,
						  void* callback_arg)
{
	FB_API_HANDLE gds_trans = 0;

	/*START_TRANSACTION*/
	{
	{
	isc_start_transaction (isc_status, (FB_API_HANDLE*) &gds_trans, (short) 1, &DB, (short) 0, (char*) 0);
	};
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		return GsecMsg75;		// gsec error
	/*END_ERROR;*/
	   }
	}

	SSHORT ret = SECURITY_exec_line(isc_status, DB, gds_trans, io_user_data, display_func, callback_arg);

	// rollback if we have an error using tmp_status to avoid
	// overwriting the error status which the caller wants to see

	if (ret)
	{
		ISC_STATUS_ARRAY tmp_status;
		isc_rollback_transaction(tmp_status, &gds_trans);
	}
	else
	{
		/*COMMIT*/
		{
		isc_commit_transaction (isc_status, (FB_API_HANDLE*) &gds_trans);;
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			// CVC: Shouldn't we call isc_rollback_transaction() here?
			return GsecMsg75;	// gsec error
		/*END_ERROR;*/
		   }
		}
	}

	return ret;
}
