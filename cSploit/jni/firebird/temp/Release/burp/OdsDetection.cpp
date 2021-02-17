/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
/*
 *	PROGRAM:	JRD Backup and Restore Program
 *	MODULE:		OdsDetection.cpp
 *	DESCRIPTION:	Detecting the backup (source) or restore (target) ODS
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
 *
 */

#include "firebird.h"
#include "../burp/burp.h"
#include "../burp/burp_proto.h"
#include "../burp/OdsDetection.h"


namespace
{
	// table used to determine capabilities, checking for specific
	// fields in system relations
	struct rel_field_t
	{
		const char* relation;
		const char* field;
		int ods_version;
	};

	const rel_field_t relations[] =
	{
		{"RDB$PROCEDURES",	0,	DB_VERSION_DDL8},	// IB4
		{"RDB$ROLES",		0,	DB_VERSION_DDL9},	// IB5
		{"RDB$PACKAGES",	0,	DB_VERSION_DDL12},	// FB3
		{0, 0, 0}
	};

	const rel_field_t rel_fields[] =
	{
		{"RDB$FIELDS",					"RDB$FIELD_PRECISION",	DB_VERSION_DDL10},		// FB1, FB1.5
		{"RDB$ROLES",					"RDB$DESCRIPTION",		DB_VERSION_DDL11},		// FB2
		{"RDB$RELATIONS",				"RDB$RELATION_TYPE",	DB_VERSION_DDL11_1},	// FB2.1
		{"RDB$PROCEDURE_PARAMETERS",	"RDB$FIELD_NAME",		DB_VERSION_DDL11_2},	// FB2.5
		{"RDB$PROCEDURES",				"RDB$ENGINE_NAME",		DB_VERSION_DDL12},		// FB3.0
		{0, 0, 0}
	};

	void general_on_error();
}


/*DATABASE DB = STATIC FILENAME "yachts.lnk";*/
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
   isc_0l = 168;
static const char
   isc_0 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 1,0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring2, 3,0, 32,0, 
	    blr_cstring2, 3,0, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 19, 'R','D','B','$','R','E','L','A','T','I','O','N','_','F','I','E','L','D','S', 0, 
		     blr_first, 
			blr_literal, blr_long, 0, 1,0,0,0,
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			      blr_parameter, 0, 1,0, 
			   blr_and, 
			      blr_eql, 
				 blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
				 blr_parameter, 0, 0,0, 
			      blr_eql, 
				 blr_field, 0, 15, 'R','D','B','$','S','Y','S','T','E','M','_','F','L','A','G', 
				 blr_literal, blr_long, 0, 1,0,0,0,
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_0 */

static const short
   isc_6l = 134;
static const char
   isc_6 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 1,0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring2, 3,0, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 13, 'R','D','B','$','R','E','L','A','T','I','O','N','S', 0, 
		     blr_first, 
			blr_literal, blr_long, 0, 1,0,0,0,
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			      blr_parameter, 0, 0,0, 
			   blr_eql, 
			      blr_field, 0, 15, 'R','D','B','$','S','Y','S','T','E','M','_','F','L','A','G', 
			      blr_literal, blr_long, 0, 1,0,0,0,
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_6 */

static const short
   isc_11l = 190;
static const char
   isc_11 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 1,0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 19, 'R','D','B','$','R','E','L','A','T','I','O','N','_','F','I','E','L','D','S', 0, 
		  blr_boolean, 
		     blr_and, 
			blr_or, 
			   blr_eql, 
			      blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			      blr_literal, blr_text, 13,0, 'R','D','B','$','R','E','L','A','T','I','O','N','S',
			   blr_eql, 
			      blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			      blr_literal, blr_text, 19,0, 'R','D','B','$','R','E','L','A','T','I','O','N','_','F','I','E','L','D','S',
			blr_eql, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_literal, blr_text, 15,0, 'R','D','B','$','S','Y','S','T','E','M','_','F','L','A','G',
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 0,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 0,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_11 */


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


#define DB			tdgbl->db_handle
#define gds_trans	tdgbl->tr_handle
#define isc_status	tdgbl->status_vector


void detectRuntimeODS()
{
   struct isc_4_struct {
          short isc_5;	/* isc_utility */
   } isc_4;
   struct isc_1_struct {
          char  isc_2 [32];	/* RDB$FIELD_NAME */
          char  isc_3 [32];	/* RDB$RELATION_NAME */
   } isc_1;
   struct isc_9_struct {
          short isc_10;	/* isc_utility */
   } isc_9;
   struct isc_7_struct {
          char  isc_8 [32];	/* RDB$RELATION_NAME */
   } isc_7;
   struct isc_12_struct {
          short isc_13;	/* isc_utility */
   } isc_12;
/**************************************
 *
 *	d e t e c t R u n t i m e O D S
 *
 **************************************
 *
 * Functional description
 *	Find the ODS version number of the database.
 *	Use system_flag to avoid any possibility of ambiguity (someone using the rdb$ prefix).
 *
 **************************************/

	BurpGlobals* tdgbl = BurpGlobals::getSpecific();
	tdgbl->runtimeODS = 0;

	// Detect very old server before IB4 just in case to exit gracefully.
	// select count(*) from rdb$relation_fields
	// where rdb$relation_name in ('RDB$RELATIONS', 'RDB$RELATION_FIELDS')
	// and rdb$field_name = 'RDB$SYSTEM_FLAG';

	int count = 0;
	isc_req_handle req_handle = 0;
	/*FOR (REQUEST_HANDLE req_handle)
		RFR IN RDB$RELATION_FIELDS
		WITH (RFR.RDB$RELATION_NAME = 'RDB$RELATIONS' OR RFR.RDB$RELATION_NAME = 'RDB$RELATION_FIELDS')
		AND RFR.RDB$FIELD_NAME = 'RDB$SYSTEM_FLAG'*/
	{
        if (!req_handle)
           isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &req_handle, (short) sizeof(isc_11), (char*) isc_11);
	if (req_handle)
           isc_start_request (isc_status, (FB_API_HANDLE*) &req_handle, (FB_API_HANDLE*) &gds_trans, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (FB_API_HANDLE*) &req_handle, (short) 0, (short) 2, &isc_12, (short) 0);
	   if (!isc_12.isc_13 || isc_status [1]) break;
		++count;
	/*END_FOR;*/
	   }
	   };
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		general_on_error();
	/*END_ERROR;*/
	   }
	}
	MISC_release_request_silent(req_handle);

	if (count != 2)
		return;

	isc_req_handle req_handle2 = 0;
	for (const rel_field_t* rel = relations; rel->relation; ++rel)
	{
		/*FOR (REQUEST_HANDLE req_handle2)
			FIRST 1 X IN RDB$RELATIONS
			WITH X.RDB$RELATION_NAME = rel->relation
			AND X.RDB$SYSTEM_FLAG = 1*/
		{
                if (!req_handle2)
                   isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &req_handle2, (short) sizeof(isc_6), (char*) isc_6);
		isc_vtov ((const char*) rel->relation, (char*) isc_7.isc_8, 32);
		if (req_handle2)
                   isc_start_and_send (isc_status, (FB_API_HANDLE*) &req_handle2, (FB_API_HANDLE*) &gds_trans, (short) 0, (short) 32, &isc_7, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, (FB_API_HANDLE*) &req_handle2, (short) 1, (short) 2, &isc_9, (short) 0);
		   if (!isc_9.isc_10 || isc_status [1]) break;
			if (tdgbl->runtimeODS < rel->ods_version)
				tdgbl->runtimeODS = rel->ods_version;
		/*END_FOR;*/
		   }
		   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			general_on_error();
		/*END_ERROR;*/
		   }
		}
	}
	MISC_release_request_silent(req_handle2);

	if (tdgbl->runtimeODS < DB_VERSION_DDL8)
		return;

	isc_req_handle req_handle3 = 0;
	for (const rel_field_t* rf = rel_fields; rf->relation; ++rf)
	{
		/*FOR (REQUEST_HANDLE req_handle3)
			FIRST 1 X2 IN RDB$RELATION_FIELDS
			WITH X2.RDB$RELATION_NAME = rf->relation
			AND X2.RDB$FIELD_NAME = rf->field
			AND X2.RDB$SYSTEM_FLAG = 1*/
		{
                if (!req_handle3)
                   isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &req_handle3, (short) sizeof(isc_0), (char*) isc_0);
		isc_vtov ((const char*) rf->field, (char*) isc_1.isc_2, 32);
		isc_vtov ((const char*) rf->relation, (char*) isc_1.isc_3, 32);
		if (req_handle3)
                   isc_start_and_send (isc_status, (FB_API_HANDLE*) &req_handle3, (FB_API_HANDLE*) &gds_trans, (short) 0, (short) 64, &isc_1, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, (FB_API_HANDLE*) &req_handle3, (short) 1, (short) 2, &isc_4, (short) 0);
		   if (!isc_4.isc_5 || isc_status [1]) break;
			if (tdgbl->runtimeODS < rf->ods_version)
				tdgbl->runtimeODS = rf->ods_version;
		/*END_FOR;*/
		   }
		   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			general_on_error();
		/*END_ERROR;*/
		   }
		}
	}
	MISC_release_request_silent(req_handle3);
}

namespace
{
	// copy/paste from backup.epp
	void general_on_error()
	{
	/**************************************
	 *
	 *	g e n e r a l _ o n _ e r r o r
	 *
	 **************************************
	 *
	 * Functional description
	 *	Handle any general ON_ERROR clause during ODS level detection.
	 *
	 **************************************/
		BurpGlobals* tdgbl = BurpGlobals::getSpecific();

		BURP_print_status(true, isc_status);
		BURP_abort();
	}
}
