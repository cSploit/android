/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	InterBase layered support library
 *	MODULE:		blob.epp
 *	DESCRIPTION:	Dynamic blob support
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
 * 2001.09.10 Claudio Valderrama: get_name() was preventing the API calls
 *   isc_blob_default_desc, isc_blob_lookup_desc & isc_blob_set_desc
 *   from working properly with dialect 3 names. Therefore, incorrect names
 *   could be returned or a lookup for a blob field could fail. In addition,
 *   a possible buffer overrun due to unchecked bounds was closed. The fc
 *   get_name() as been renamed copy_exact_name().
 *
 */

#include "firebird.h"
#include "../jrd/common.h"
#include "../jrd/ibase.h"
#include "../jrd/intl.h"
#include "../dsql/blob_proto.h"
#include "../dsql/utld_proto.h"
#include "../common/StatusArg.h"

using namespace Firebird;

/*DATABASE DB = STATIC "yachts.lnk";*/
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
   isc_0l = 301;
static const char
   isc_0 [] = {
   4,2,4,1,5,0,40,32,0,7,0,7,0,7,0,7,0,4,0,2,0,40,32,0,40,32,0,
   12,0,2,7,'C',2,'J',24,'R','D','B','$','P','R','O','C','E','D',
   'U','R','E','_','P','A','R','A','M','E','T','E','R','S',0,'J',
   10,'R','D','B','$','F','I','E','L','D','S',1,'G',58,47,23,0,
   16,'R','D','B','$','F','I','E','L','D','_','S','O','U','R','C',
   'E',23,1,14,'R','D','B','$','F','I','E','L','D','_','N','A',
   'M','E',58,47,23,0,18,'R','D','B','$','P','R','O','C','E','D',
   'U','R','E','_','N','A','M','E',25,0,1,0,47,23,0,18,'R','D',
   'B','$','P','A','R','A','M','E','T','E','R','_','N','A','M',
   'E',25,0,0,0,-1,14,1,2,1,23,1,14,'R','D','B','$','F','I','E',
   'L','D','_','N','A','M','E',25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,
   0,1,23,1,18,'R','D','B','$','S','E','G','M','E','N','T','_',
   'L','E','N','G','T','H',25,1,2,0,1,23,1,20,'R','D','B','$','C',
   'H','A','R','A','C','T','E','R','_','S','E','T','_','I','D',
   25,1,3,0,1,23,1,18,'R','D','B','$','F','I','E','L','D','_','S',
   'U','B','_','T','Y','P','E',25,1,4,0,-1,14,1,1,21,8,0,0,0,0,
   0,25,1,1,0,-1,-1,'L'
   };	/* end of blr string for request isc_0 */

static const short
   isc_10l = 291;
static const char
   isc_10 [] = {
   4,2,4,1,5,0,40,32,0,7,0,7,0,7,0,7,0,4,0,2,0,40,32,0,40,32,0,
   12,0,2,7,'C',2,'J',19,'R','D','B','$','R','E','L','A','T','I',
   'O','N','_','F','I','E','L','D','S',0,'J',10,'R','D','B','$',
   'F','I','E','L','D','S',1,'G',58,47,23,0,16,'R','D','B','$',
   'F','I','E','L','D','_','S','O','U','R','C','E',23,1,14,'R',
   'D','B','$','F','I','E','L','D','_','N','A','M','E',58,47,23,
   0,17,'R','D','B','$','R','E','L','A','T','I','O','N','_','N',
   'A','M','E',25,0,1,0,47,23,0,14,'R','D','B','$','F','I','E',
   'L','D','_','N','A','M','E',25,0,0,0,-1,14,1,2,1,23,1,14,'R',
   'D','B','$','F','I','E','L','D','_','N','A','M','E',25,1,0,0,
   1,21,8,0,1,0,0,0,25,1,1,0,1,23,1,18,'R','D','B','$','S','E',
   'G','M','E','N','T','_','L','E','N','G','T','H',25,1,2,0,1,23,
   1,20,'R','D','B','$','C','H','A','R','A','C','T','E','R','_',
   'S','E','T','_','I','D',25,1,3,0,1,23,1,18,'R','D','B','$','F',
   'I','E','L','D','_','S','U','B','_','T','Y','P','E',25,1,4,0,
   -1,14,1,1,21,8,0,0,0,0,0,25,1,1,0,-1,-1,'L'
   };	/* end of blr string for request isc_10 */


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


static void copy_exact_name (const UCHAR*, UCHAR*, SSHORT);
static ISC_STATUS error(ISC_STATUS* status, const Arg::StatusVector& v);


void API_ROUTINE isc_blob_default_desc(ISC_BLOB_DESC* desc,
									   const UCHAR* relation_name,
									   const UCHAR* field_name)
{
/**************************************
 *
 *	i s c _ b l o b _ d e f a u l t _ d e s c
 *
 **************************************
 *
 * Functional description
 *
 *	This function will set the default
 *	values in the blob_descriptor.
 *
 **************************************/

	desc->blob_desc_subtype = isc_blob_text;
	desc->blob_desc_charset = CS_dynamic;
	desc->blob_desc_segment_size = 80;

    copy_exact_name(field_name, desc->blob_desc_field_name, sizeof(desc->blob_desc_field_name));
    copy_exact_name(relation_name, desc->blob_desc_relation_name, sizeof(desc->blob_desc_relation_name));
}


ISC_STATUS API_ROUTINE isc_blob_gen_bpb(ISC_STATUS* status,
										const ISC_BLOB_DESC* to_desc,
										const ISC_BLOB_DESC* from_desc,
										USHORT bpb_buffer_length,
										UCHAR* bpb_buffer,
										USHORT* bpb_length)
{
/**************************************
 *
 *	i s c _ b l o b _ g e n _ b p b
 *
 **************************************
 *
 * Functional description
 *
 *  	This function will generate a bpb
 *	given a to_desc and a from_desc
 *	which contain the subtype and
 *	character set information.
 *
 **************************************/
	if (bpb_buffer_length < 17)
		return error(status, Arg::Gds(isc_random) << Arg::Str("BPB buffer too small"));

	UCHAR* p = bpb_buffer;
	*p++ = isc_bpb_version1;
	*p++ = isc_bpb_target_type;
	*p++ = 2;
	*p++ = (UCHAR)to_desc->blob_desc_subtype;
	*p++ = (UCHAR)(to_desc->blob_desc_subtype >> 8);
	*p++ = isc_bpb_source_type;
	*p++ = 2;
	*p++ = (UCHAR)from_desc->blob_desc_subtype;
	*p++ = (UCHAR)(from_desc->blob_desc_subtype >> 8);
	*p++ = isc_bpb_target_interp;
	*p++ = 2;
	*p++ = (UCHAR)to_desc->blob_desc_charset;
	*p++ = (UCHAR)(to_desc->blob_desc_charset >> 8);
	*p++ = isc_bpb_source_interp;
	*p++ = 2;
	*p++ = (UCHAR)from_desc->blob_desc_charset;
	*p++ = (UCHAR)(from_desc->blob_desc_charset >> 8);

	*bpb_length = p - bpb_buffer;

	return error(status, Arg::Gds(FB_SUCCESS));
}


ISC_STATUS API_ROUTINE isc_blob_lookup_desc(ISC_STATUS* status,
											FB_API_HANDLE* db_handle,
											FB_API_HANDLE* trans_handle,
											const UCHAR* relation_name,
											const UCHAR* field_name,
											ISC_BLOB_DESC* desc, UCHAR* global)
{
   struct isc_4_struct {
          char  isc_5 [32];	/* RDB$FIELD_NAME */
          short isc_6;	/* isc_utility */
          short isc_7;	/* RDB$SEGMENT_LENGTH */
          short isc_8;	/* RDB$CHARACTER_SET_ID */
          short isc_9;	/* RDB$FIELD_SUB_TYPE */
   } isc_4;
   struct isc_1_struct {
          char  isc_2 [32];	/* RDB$PARAMETER_NAME */
          char  isc_3 [32];	/* RDB$PROCEDURE_NAME */
   } isc_1;
   struct isc_14_struct {
          char  isc_15 [32];	/* RDB$FIELD_NAME */
          short isc_16;	/* isc_utility */
          short isc_17;	/* RDB$SEGMENT_LENGTH */
          short isc_18;	/* RDB$CHARACTER_SET_ID */
          short isc_19;	/* RDB$FIELD_SUB_TYPE */
   } isc_14;
   struct isc_11_struct {
          char  isc_12 [32];	/* RDB$FIELD_NAME */
          char  isc_13 [32];	/* RDB$RELATION_NAME */
   } isc_11;
/***********************************************
 *
 *	i s c _ b l o b _ l o o k u p _ d e s c
 *
 ***********************************************
 *
 * Functional description
 *
 *	This routine will lookup the subtype,
 *	character set and segment size information
 *	from the metadata, given a relation/procedure name
 *	and column/parameter name.  it will fill in the information
 *	in the BLOB_DESC.
 *
 ***********************************************/
	ISC_STATUS_ARRAY isc_status = {0};
	isc_db_handle DB = *db_handle;
	isc_req_handle handle = 0;

    copy_exact_name(field_name, desc->blob_desc_field_name, sizeof(desc->blob_desc_field_name));
    copy_exact_name(relation_name, desc->blob_desc_relation_name, sizeof(desc->blob_desc_relation_name));

	bool flag = false;

	/*FOR (REQUEST_HANDLE handle TRANSACTION_HANDLE *trans_handle)
		X IN RDB$RELATION_FIELDS CROSS Y IN RDB$FIELDS
		WITH X.RDB$FIELD_SOURCE EQ Y.RDB$FIELD_NAME AND
		X.RDB$RELATION_NAME EQ desc->blob_desc_relation_name AND
		X.RDB$FIELD_NAME EQ desc->blob_desc_field_name*/
	{
        if (!handle)
           isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &handle, (short) sizeof(isc_10), (char*) isc_10);
	isc_vtov ((const char*) desc->blob_desc_field_name, (char*) isc_11.isc_12, 32);
	isc_vtov ((const char*) desc->blob_desc_relation_name, (char*) isc_11.isc_13, 32);
	if (handle)
           isc_start_and_send (isc_status, (FB_API_HANDLE*) &handle, (FB_API_HANDLE*) &*trans_handle, (short) 0, (short) 64, &isc_11, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (FB_API_HANDLE*) &handle, (short) 1, (short) 40, &isc_14, (short) 0);
	   if (!isc_14.isc_16 || isc_status [1]) break;
		flag = true;

	    desc->blob_desc_subtype = /*Y.RDB$FIELD_SUB_TYPE*/
				      isc_14.isc_19;
        desc->blob_desc_charset = /*Y.RDB$CHARACTER_SET_ID*/
				  isc_14.isc_18;
        desc->blob_desc_segment_size = /*Y.RDB$SEGMENT_LENGTH*/
				       isc_14.isc_17;

        if (global) {
            copy_exact_name((UCHAR*) /*Y.RDB$FIELD_NAME*/
				     isc_14.isc_15, global, sizeof(/*Y.RDB$FIELD_NAME*/
		 isc_14.isc_15));
        }
	/*END_FOR*/
	   }
	   };
    /*ON_ERROR*/
    if (isc_status [1])
       {
		ISC_STATUS_ARRAY temp_status;
		isc_release_request(temp_status, &handle);
		return UTLD_copy_status(isc_status, status);
	/*END_ERROR;*/
	   }
	}

	isc_release_request(isc_status, &handle);

	if (!flag)
	{
		handle = 0;

		/*FOR (REQUEST_HANDLE handle TRANSACTION_HANDLE *trans_handle)
			X IN RDB$PROCEDURE_PARAMETERS CROSS Y IN RDB$FIELDS
			WITH X.RDB$FIELD_SOURCE EQ Y.RDB$FIELD_NAME AND
			X.RDB$PROCEDURE_NAME EQ desc->blob_desc_relation_name AND
			X.RDB$PARAMETER_NAME EQ desc->blob_desc_field_name*/
		{
                if (!handle)
                   isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &handle, (short) sizeof(isc_0), (char*) isc_0);
		isc_vtov ((const char*) desc->blob_desc_field_name, (char*) isc_1.isc_2, 32);
		isc_vtov ((const char*) desc->blob_desc_relation_name, (char*) isc_1.isc_3, 32);
		if (handle)
                   isc_start_and_send (isc_status, (FB_API_HANDLE*) &handle, (FB_API_HANDLE*) &*trans_handle, (short) 0, (short) 64, &isc_1, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, (FB_API_HANDLE*) &handle, (short) 1, (short) 40, &isc_4, (short) 0);
		   if (!isc_4.isc_6 || isc_status [1]) break;
			flag = true;

			desc->blob_desc_subtype = /*Y.RDB$FIELD_SUB_TYPE*/
						  isc_4.isc_9;
			desc->blob_desc_charset = /*Y.RDB$CHARACTER_SET_ID*/
						  isc_4.isc_8;
			desc->blob_desc_segment_size = /*Y.RDB$SEGMENT_LENGTH*/
						       isc_4.isc_7;

			if (global) {
				copy_exact_name((UCHAR*) /*Y.RDB$FIELD_NAME*/
							 isc_4.isc_5, global, sizeof(/*Y.RDB$FIELD_NAME*/
		 isc_4.isc_5));
			}
		/*END_FOR*/
		   }
		   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			ISC_STATUS_ARRAY temp_status;
			isc_release_request(temp_status, &handle);
			return UTLD_copy_status(isc_status, status);
		/*END_ERROR;*/
		   }
		}

		isc_release_request(isc_status, &handle);
	}

	if (!flag)
		return error(status, Arg::Gds(isc_fldnotdef) << Arg::Str((const char*)(desc->blob_desc_field_name)) <<
														Arg::Str((const char*)(desc->blob_desc_relation_name)));

	return error(status, Arg::Gds(FB_SUCCESS));
}


ISC_STATUS API_ROUTINE isc_blob_set_desc(ISC_STATUS* status,
										 const UCHAR* relation_name,
										 const UCHAR* field_name,
										 SSHORT subtype,
										 SSHORT charset,
										 SSHORT segment_size,
										 ISC_BLOB_DESC* desc)
{
/**************************************
 *
 *	i s c _ b l o b _ s e t _ d e s c
 *
 **************************************
 *
 * Functional description
 *
 *	This routine will set the subtype
 *	and character set information in the
 *	BLOB_DESC based on the information
 *	specifically passed in by the user.
 *
 **************************************/

    copy_exact_name(field_name, desc->blob_desc_field_name, sizeof(desc->blob_desc_field_name));
    copy_exact_name(relation_name, desc->blob_desc_relation_name, sizeof(desc->blob_desc_relation_name));

	desc->blob_desc_subtype = subtype;
	desc->blob_desc_charset = charset;
	desc->blob_desc_segment_size = segment_size;

	return error(status, Arg::Gds(FB_SUCCESS));
}




static void copy_exact_name (const UCHAR* from, UCHAR* to, SSHORT bsize)
{
/**************************************
 *
 *  c o p y _ e x a c t _ n a m e
 *
 **************************************
 *
 * Functional description
 *  Copy null terminated name ot stops at bsize - 1.
 *  CVC: This is just another fc like DYN_terminate.
 *
 **************************************/
	const UCHAR* const from_end = from + bsize - 1;
	UCHAR* to2 = to - 1;
	while (*from && from < from_end) {
		if (*from != ' ') {
			to2 = to;
		}
		*to++ = *from++;
	}
	*++to2 = 0;
}


static ISC_STATUS error(ISC_STATUS* status, const Arg::StatusVector& v)
{
/**************************************
 *
 *	e r r o r
 *
 **************************************
 *
 * Functional description
 *	Stuff a status vector.
 *
 **************************************/
	return v.copyTo(status);
}

