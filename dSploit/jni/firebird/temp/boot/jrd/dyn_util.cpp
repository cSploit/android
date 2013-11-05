/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Data Definition Utility
 *	MODULE:		dyn_util.epp
 *	DESCRIPTION:	Dynamic data definition - utility functions
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
 * 2002-02-24 Sean Leyne - Code Cleanup of old Win 3.1 port (WINDOWS_ONLY)
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>

#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/tra.h"
#include "../jrd/scl.h"
#include "../jrd/drq.h"
#include "../jrd/flags.h"
#include "../jrd/ibase.h"
#include "../jrd/lls.h"
#include "../jrd/met.h"
#include "../jrd/btr.h"
#include "../jrd/intl.h"
#include "../jrd/dyn.h"
#include "../jrd/ods.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dyn_proto.h"
#include "../jrd/dyn_md_proto.h"
#include "../jrd/dyn_ut_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/vio_proto.h"
#include "../common/utils_proto.h"

using MsgFormat::SafeArg;

using namespace Jrd;

/*DATABASE DB = STATIC "ODS.RDB";*/
static const UCHAR	jrd_0 [416] =
   {	// blr string 

4,2,4,0,52,0,41,0,0,128,0,9,0,9,0,9,0,9,0,9,0,9,0,9,0,9,0,41,
3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
12,0,15,'K',2,0,0,2,1,41,0,12,0,11,0,24,0,27,0,1,41,0,14,0,13,
0,24,0,26,0,1,41,0,16,0,15,0,24,0,25,0,1,41,0,18,0,17,0,24,0,
24,0,1,41,0,20,0,19,0,24,0,23,0,1,41,0,22,0,21,0,24,0,22,0,1,
41,0,24,0,23,0,24,0,21,0,1,41,0,26,0,25,0,24,0,20,0,1,41,0,28,
0,27,0,24,0,19,0,1,41,0,0,0,29,0,24,0,18,0,1,41,0,31,0,30,0,24,
0,17,0,1,41,0,1,0,32,0,24,0,16,0,1,41,0,34,0,33,0,24,0,15,0,1,
41,0,2,0,35,0,24,0,14,0,1,41,0,3,0,'$',0,24,0,13,0,1,41,0,4,0,
37,0,24,0,12,0,1,41,0,39,0,38,0,24,0,11,0,1,41,0,41,0,40,0,24,
0,10,0,1,41,0,43,0,42,0,24,0,9,0,1,41,0,45,0,44,0,24,0,8,0,1,
41,0,5,0,46,0,24,0,5,0,1,41,0,6,0,47,0,24,0,4,0,1,41,0,7,0,48,
0,24,0,3,0,1,41,0,8,0,49,0,24,0,2,0,1,41,0,9,0,50,0,24,0,1,0,
1,41,0,10,0,51,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_54 [452] =
   {	// blr string 

4,2,4,1,51,0,41,0,0,128,0,9,0,9,0,9,0,9,0,9,0,9,0,9,0,9,0,41,
3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',2,0,0,'G',47,24,0,0,0,25,0,0,0,255,
14,1,2,1,24,0,18,0,41,1,0,0,27,0,1,24,0,16,0,41,1,1,0,30,0,1,
24,0,14,0,41,1,2,0,33,0,1,24,0,13,0,41,1,3,0,34,0,1,24,0,12,0,
41,1,4,0,35,0,1,24,0,5,0,41,1,5,0,44,0,1,24,0,4,0,41,1,6,0,45,
0,1,24,0,3,0,41,1,7,0,46,0,1,24,0,2,0,41,1,8,0,47,0,1,24,0,1,
0,41,1,9,0,48,0,1,21,8,0,1,0,0,0,25,1,10,0,1,24,0,27,0,41,1,12,
0,11,0,1,24,0,26,0,41,1,14,0,13,0,1,24,0,25,0,41,1,16,0,15,0,
1,24,0,24,0,41,1,18,0,17,0,1,24,0,23,0,41,1,20,0,19,0,1,24,0,
21,0,41,1,22,0,21,0,1,24,0,20,0,41,1,24,0,23,0,1,24,0,19,0,41,
1,26,0,25,0,1,24,0,17,0,41,1,29,0,28,0,1,24,0,15,0,41,1,32,0,
31,0,1,24,0,11,0,41,1,37,0,'$',0,1,24,0,10,0,41,1,39,0,38,0,1,
24,0,9,0,41,1,41,0,40,0,1,24,0,8,0,41,1,43,0,42,0,1,24,0,22,0,
41,1,50,0,49,0,255,14,1,1,21,8,0,0,0,0,0,25,1,10,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_109 [86] =
   {	// blr string 

4,2,4,1,3,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
2,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,
1,0,0,1,24,0,22,0,41,1,2,0,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_116 [45] =
   {	// blr string 

4,2,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,15,'K',24,0,0,2,1,25,
0,0,0,24,0,1,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_120 [139] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,3,0,41,3,0,32,0,41,3,0,32,0,7,
0,12,0,2,7,'C',2,'K',7,0,0,'K',27,0,1,'G',58,47,24,0,1,0,24,1,
1,0,58,47,24,0,0,0,25,0,1,0,58,47,24,0,2,0,25,0,2,0,58,47,24,
1,3,0,21,8,0,1,0,0,0,47,24,1,0,0,25,0,0,0,255,14,1,2,1,24,1,4,
0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,
0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_128 [126] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,3,0,41,3,0,32,0,41,3,0,32,0,7,
0,12,0,2,7,'C',2,'K',7,0,0,'K',5,0,1,'G',58,47,24,1,1,0,24,0,
1,0,58,47,24,0,0,0,25,0,1,0,58,47,24,0,2,0,25,0,2,0,47,24,1,0,
0,25,0,0,0,255,14,1,2,1,24,1,2,0,25,1,0,0,1,21,8,0,1,0,0,0,25,
1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_136 [79] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',12,0,0,
'D',21,8,0,1,0,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_141 [79] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',4,0,0,
'D',21,8,0,1,0,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_146 [86] =
   {	// blr string 

4,2,4,1,3,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
5,0,0,'G',47,24,0,1,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,
1,0,0,1,24,0,6,0,41,1,2,0,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_153 [79] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',2,0,0,
'D',21,8,0,1,0,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_158 [79] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',22,0,0,
'D',21,8,0,1,0,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 


static const UCHAR gen_id_blr1[] =
{
	blr_version5,
	blr_begin,
	blr_message, 0, 1, 0,
	blr_int64, 0,
	blr_begin,
	blr_send, 0,
	blr_begin,
	blr_assignment,
	blr_gen_id
};

static const UCHAR gen_id_blr2[] =
{
	blr_literal, blr_long, 0, 1, 0, 0, 0,
		blr_parameter, 0, 0, 0, blr_end, blr_end, blr_end, blr_eoc
};

static const UCHAR prot_blr[] =
{
	blr_version5,
	blr_begin,
	blr_message, 1, 1, 0,
	blr_short, 0,
	blr_message, 0, 2, 0,
	blr_cstring, 32, 0,
	blr_cstring, 32, 0,
	blr_receive, 0,
	blr_begin,
	blr_send, 1,
	blr_begin,
	blr_assignment,
	blr_prot_mask,
	blr_parameter, 0, 0, 0,
	blr_parameter, 0, 1, 0,
	blr_parameter, 1, 0, 0,
	blr_end,
	blr_end,
	blr_end,
	blr_eoc
};



SINT64 DYN_UTIL_gen_unique_id(thread_db* tdbb,
							  Global* /*gbl*/,
							  SSHORT id,
							  const char* generator_name)
{
/**************************************
 *
 *	D Y N _ U T I L _ g e n _ u n i q u e _ i d
 *
 **************************************
 *
 * Functional description
 *	Generate a unique id using a generator.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, id, DYN_REQUESTS);

	SINT64 value = 0;

	try
	{
		if (!request)
		{
			const size_t name_length = strlen(generator_name);
			fb_assert(name_length < MAX_SQL_IDENTIFIER_SIZE);
			const size_t blr_size = sizeof(gen_id_blr1) + sizeof(gen_id_blr2) + 1 + name_length;

			Firebird::UCharBuffer blr;
			UCHAR* p = blr.getBuffer(blr_size);

			memcpy(p, gen_id_blr1, sizeof(gen_id_blr1));
			p += sizeof(gen_id_blr1);
			*p++ = name_length;
			memcpy(p, generator_name, name_length);
			p += name_length;
			memcpy(p, gen_id_blr2, sizeof(gen_id_blr2));
			p += sizeof(gen_id_blr2);
			fb_assert(size_t(p - blr.begin()) == blr_size);

			request = CMP_compile2(tdbb, blr.begin(), (ULONG) blr.getCount(), true);
		}

		EXE_start(tdbb, request, dbb->dbb_sys_trans);
		EXE_receive(tdbb, request, 0, sizeof(value), (UCHAR*) &value);
		EXE_unwind(tdbb, request);
	}
	catch (const Firebird::Exception&)
	{
		DYN_rundown_request(request, id);
		throw;
	}

	if (!DYN_REQUEST(id)) {
		DYN_REQUEST(id) = request;
	}

	return value;
}


void DYN_UTIL_generate_constraint_name( thread_db* tdbb, Global* gbl, Firebird::MetaName& buffer)
{
   struct {
          SSHORT jrd_162;	// gds__utility 
   } jrd_161;
   struct {
          TEXT  jrd_160 [32];	// RDB$CONSTRAINT_NAME 
   } jrd_159;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ c o n s t r a i n t _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Generate a name unique to RDB$RELATION_CONSTRAINTS.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = NULL;
	SSHORT id = -1;

	try
	{
		bool found = false;

		do {
			buffer.printf("INTEG_%" SQUADFORMAT,
					DYN_UTIL_gen_unique_id(tdbb, gbl, drq_g_nxt_con, "RDB$CONSTRAINT_NAME"));

			request = CMP_find_request(tdbb, drq_f_nxt_con, DYN_REQUESTS);
			id = drq_f_nxt_con;

			found = false;
			/*FOR(REQUEST_HANDLE request)
				FIRST 1 X IN RDB$RELATION_CONSTRAINTS
					WITH X.RDB$CONSTRAINT_NAME EQ buffer.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_158, sizeof(jrd_158), true);
			gds__vtov ((const char*) buffer.c_str(), (char*) jrd_159.jrd_160, 32);
			EXE_start (tdbb, request, dbb->dbb_sys_trans);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_159);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_161);
			   if (!jrd_161.jrd_162) break;

	            if (!DYN_REQUEST(drq_f_nxt_con))
	                DYN_REQUEST(drq_f_nxt_con) = request;
				found = true;
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_f_nxt_con))
				DYN_REQUEST(drq_f_nxt_con) = request;
			request = NULL;
		} while (found);
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, id);
		DYN_error_punt(true, 131);
		// msg 131: "Generation of constraint name failed"
	}
}


void DYN_UTIL_generate_field_name( thread_db* tdbb, Global* gbl, TEXT* buffer)
{
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ f i e l d _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Stub to make it work with char* too.
 *
 **************************************/
	Firebird::MetaName temp;
	DYN_UTIL_generate_field_name(tdbb, gbl, temp);
	strcpy(buffer, temp.c_str());
}


void DYN_UTIL_generate_field_name( thread_db* tdbb, Global* gbl, Firebird::MetaName& buffer)
{
   struct {
          SSHORT jrd_157;	// gds__utility 
   } jrd_156;
   struct {
          TEXT  jrd_155 [32];	// RDB$FIELD_NAME 
   } jrd_154;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ f i e l d _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Generate a name unique to RDB$FIELDS.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = NULL;
	SSHORT id = -1;

	try
	{
		bool found = false;

		do {
			buffer.printf("RDB$%" SQUADFORMAT,
					DYN_UTIL_gen_unique_id(tdbb, gbl, drq_g_nxt_fld, "RDB$FIELD_NAME"));

			request = CMP_find_request(tdbb, drq_f_nxt_fld, DYN_REQUESTS);
			id = drq_f_nxt_fld;

			found = false;
			/*FOR(REQUEST_HANDLE request)
				FIRST 1 X IN RDB$FIELDS WITH X.RDB$FIELD_NAME EQ buffer.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_153, sizeof(jrd_153), true);
			gds__vtov ((const char*) buffer.c_str(), (char*) jrd_154.jrd_155, 32);
			EXE_start (tdbb, request, dbb->dbb_sys_trans);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_154);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_156);
			   if (!jrd_156.jrd_157) break;

	            if (!DYN_REQUEST(drq_f_nxt_fld))
					DYN_REQUEST(drq_f_nxt_fld) = request;
				found = true;
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_f_nxt_fld))
				DYN_REQUEST(drq_f_nxt_fld) = request;
			request = NULL;
		} while (found);
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, id);
		DYN_error_punt(true, 81);
		// msg 81: "Generation of field name failed"
	}
}


void DYN_UTIL_generate_field_position(thread_db* tdbb,
									  Global* /*gbl*/,
									  const Firebird::MetaName& relation_name,
									  SLONG* field_pos)
{
   struct {
          SSHORT jrd_150;	// gds__utility 
          SSHORT jrd_151;	// gds__null_flag 
          SSHORT jrd_152;	// RDB$FIELD_POSITION 
   } jrd_149;
   struct {
          TEXT  jrd_148 [32];	// RDB$RELATION_NAME 
   } jrd_147;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ f i e l d _ p o s i t i o n
 *
 **************************************
 *
 * Functional description
 *	Generate a field position if not specified
 *
 **************************************/
	SLONG field_position = -1;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = NULL;

	try
	{
		request = CMP_find_request(tdbb, drq_l_fld_pos, DYN_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			X IN RDB$RELATION_FIELDS
				WITH X.RDB$RELATION_NAME EQ relation_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_146, sizeof(jrd_146), true);
		gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_147.jrd_148, 32);
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_147);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_149);
		   if (!jrd_149.jrd_150) break;

	        if (!DYN_REQUEST(drq_l_fld_pos))
	            DYN_REQUEST(drq_l_fld_pos) = request;

			if (/*X.RDB$FIELD_POSITION.NULL*/
			    jrd_149.jrd_151)
				continue;

			field_position = MAX(/*X.RDB$FIELD_POSITION*/
					     jrd_149.jrd_152, field_position);
		/*END_FOR;*/
		   }
		}

		*field_pos = field_position;
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 162);
		// msg 162: "Looking up field position failed"
	}
}


void DYN_UTIL_generate_index_name(thread_db* tdbb, Global* gbl,
								  Firebird::MetaName& buffer, UCHAR verb)
{
   struct {
          SSHORT jrd_145;	// gds__utility 
   } jrd_144;
   struct {
          TEXT  jrd_143 [32];	// RDB$INDEX_NAME 
   } jrd_142;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ i n d e x _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Generate a name unique to RDB$INDICES.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = NULL;
	SSHORT id = -1;

	try
	{
		bool found = false;

		do {
			const SCHAR* format;
			switch (verb)
			{
			case isc_dyn_def_primary_key:
				format = "RDB$PRIMARY%" SQUADFORMAT;
				break;
			case isc_dyn_def_foreign_key:
				format = "RDB$FOREIGN%" SQUADFORMAT;
				break;
			default:
				format = "RDB$%" SQUADFORMAT;
			}

			buffer.printf(format,
				DYN_UTIL_gen_unique_id(tdbb, gbl, drq_g_nxt_idx, "RDB$INDEX_NAME"));

			request = CMP_find_request(tdbb, drq_f_nxt_idx, DYN_REQUESTS);
			id = drq_f_nxt_idx;

			found = false;
			/*FOR(REQUEST_HANDLE request)
				FIRST 1 X IN RDB$INDICES WITH X.RDB$INDEX_NAME EQ buffer.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_141, sizeof(jrd_141), true);
			gds__vtov ((const char*) buffer.c_str(), (char*) jrd_142.jrd_143, 32);
			EXE_start (tdbb, request, dbb->dbb_sys_trans);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_142);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_144);
			   if (!jrd_144.jrd_145) break;

	            if (!DYN_REQUEST(drq_f_nxt_idx))
					DYN_REQUEST(drq_f_nxt_idx) = request;
				found = true;
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_f_nxt_idx))
				DYN_REQUEST(drq_f_nxt_idx) = request;
			request = NULL;
		} while (found);
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, id);
		DYN_error_punt(true, 82);
		// msg 82: "Generation of index name failed"
	}
}


void DYN_UTIL_generate_trigger_name( thread_db* tdbb, Global* gbl, Firebird::MetaName& buffer)
{
   struct {
          SSHORT jrd_140;	// gds__utility 
   } jrd_139;
   struct {
          TEXT  jrd_138 [32];	// RDB$TRIGGER_NAME 
   } jrd_137;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ t r i g g e r _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Generate a name unique to RDB$TRIGGERS.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = NULL;
	SSHORT id = -1;

	try
	{
		bool found = false;

		do {
			buffer.printf("CHECK_%" SQUADFORMAT,
				DYN_UTIL_gen_unique_id(tdbb, gbl, drq_g_nxt_trg, "RDB$TRIGGER_NAME"));

			request = CMP_find_request(tdbb, drq_f_nxt_trg, DYN_REQUESTS);
			id = drq_f_nxt_trg;

			found = false;
			/*FOR(REQUEST_HANDLE request)
				FIRST 1 X IN RDB$TRIGGERS WITH X.RDB$TRIGGER_NAME EQ buffer.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_136, sizeof(jrd_136), true);
			gds__vtov ((const char*) buffer.c_str(), (char*) jrd_137.jrd_138, 32);
			EXE_start (tdbb, request, dbb->dbb_sys_trans);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_137);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_139);
			   if (!jrd_139.jrd_140) break;

	            if (!DYN_REQUEST(drq_f_nxt_trg))
					DYN_REQUEST(drq_f_nxt_trg) = request;
				found = true;
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_f_nxt_trg))
				DYN_REQUEST(drq_f_nxt_trg) = request;
			request = NULL;
		} while (found);
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, id);
		DYN_error_punt(true, 83);
		// msg 83: "Generation of trigger name failed"
	}
}


bool DYN_UTIL_find_field_source(thread_db* tdbb,
								Global* gbl,
								const Firebird::MetaName& view_name,
								USHORT context,
								const TEXT* local_name,
								TEXT* output_field_name)
{
   struct {
          TEXT  jrd_126 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_127;	// gds__utility 
   } jrd_125;
   struct {
          TEXT  jrd_122 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_123 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_124;	// RDB$VIEW_CONTEXT 
   } jrd_121;
   struct {
          TEXT  jrd_134 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_135;	// gds__utility 
   } jrd_133;
   struct {
          TEXT  jrd_130 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_131 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_132;	// RDB$VIEW_CONTEXT 
   } jrd_129;
/**************************************
 *
 *	D Y N _U T I L _ f i n d _ f i e l d _ s o u r c e
 *
 **************************************
 *
 * Functional description
 *	Find the original source for a view field.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = NULL;

	/* CVC: It seems the logic of this function was changed over time. It's unlikely
	it will cause a failure that leads to call DYN_error_punt(), unless the request finds
	problems due to database corruption or unexpected ODS changes. Under normal
	circumstances, it will return either true or false. When true, we found a field source
	for the view's name/context/field and are loading this value in the last parameter,
	that can be used against rdb$fields' rdb$field_name. */

	bool found = false;

	try {
		request = CMP_find_request(tdbb, drq_l_fld_src2, DYN_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			VRL IN RDB$VIEW_RELATIONS CROSS
				RFR IN RDB$RELATION_FIELDS OVER RDB$RELATION_NAME
				WITH VRL.RDB$VIEW_NAME EQ view_name.c_str() AND
				VRL.RDB$VIEW_CONTEXT EQ context AND
				RFR.RDB$FIELD_NAME EQ local_name*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_128, sizeof(jrd_128), true);
		gds__vtov ((const char*) local_name, (char*) jrd_129.jrd_130, 32);
		gds__vtov ((const char*) view_name.c_str(), (char*) jrd_129.jrd_131, 32);
		jrd_129.jrd_132 = context;
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_129);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_133);
		   if (!jrd_133.jrd_135) break;

			if (!DYN_REQUEST(drq_l_fld_src2)) {
				DYN_REQUEST(drq_l_fld_src2) = request;
			}

			found = true;
			fb_utils::exact_name_limit(/*RFR.RDB$FIELD_SOURCE*/
						   jrd_133.jrd_134, sizeof(/*RFR.RDB$FIELD_SOURCE*/
	 jrd_133.jrd_134));
			strcpy(output_field_name, /*RFR.RDB$FIELD_SOURCE*/
						  jrd_133.jrd_134);
		/*END_FOR;*/
		   }
		}
		if (!DYN_REQUEST(drq_l_fld_src2)) {
			DYN_REQUEST(drq_l_fld_src2) = request;
		}

		if (!found)
		{
			request = CMP_find_request(tdbb, drq_l_fld_src3, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				VRL IN RDB$VIEW_RELATIONS CROSS
					PPR IN RDB$PROCEDURE_PARAMETERS
					WITH VRL.RDB$RELATION_NAME EQ PPR.RDB$PROCEDURE_NAME AND
					VRL.RDB$VIEW_NAME EQ view_name.c_str() AND
					VRL.RDB$VIEW_CONTEXT EQ context AND
					PPR.RDB$PARAMETER_TYPE = 1 AND // output
					PPR.RDB$PARAMETER_NAME EQ local_name*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_120, sizeof(jrd_120), true);
			gds__vtov ((const char*) local_name, (char*) jrd_121.jrd_122, 32);
			gds__vtov ((const char*) view_name.c_str(), (char*) jrd_121.jrd_123, 32);
			jrd_121.jrd_124 = context;
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_121);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_125);
			   if (!jrd_125.jrd_127) break;

				if (!DYN_REQUEST(drq_l_fld_src3)) {
					DYN_REQUEST(drq_l_fld_src3) = request;
				}

				found = true;
				fb_utils::exact_name_limit(/*PPR.RDB$FIELD_SOURCE*/
							   jrd_125.jrd_126, sizeof(/*PPR.RDB$FIELD_SOURCE*/
	 jrd_125.jrd_126));
				strcpy(output_field_name, /*PPR.RDB$FIELD_SOURCE*/
							  jrd_125.jrd_126);
			/*END_FOR;*/
			   }
			}
			if (!DYN_REQUEST(drq_l_fld_src3)) {
				DYN_REQUEST(drq_l_fld_src3) = request;
			}
		}
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 80);
		// msg 80: "Specified domain or source field does not exist"
	}

	return found;
}


bool DYN_UTIL_get_prot(thread_db*		tdbb,
						  Global*		gbl,
						  const SCHAR*	rname,
						  const SCHAR*	fname,
						  SecurityClass::flags_t*	prot_mask)
{
/**************************************
 *
 *	D Y N _ U T I L _ g e t _ p r o t
 *
 **************************************
 *
 * Functional description
 *	Get protection mask for relation or relation_field
 *
 **************************************/
	struct
	{
		SqlIdentifier relation_name;
		SqlIdentifier field_name;
	} in_msg;

	SET_TDBB(tdbb);

	jrd_req* request = CMP_find_request(tdbb, drq_l_prot_mask, DYN_REQUESTS);

	try
	{
		if (!request)
		{
			request = CMP_compile2(tdbb, prot_blr, sizeof(prot_blr), true);
		}
		gds__vtov(rname, in_msg.relation_name, sizeof(in_msg.relation_name));
		gds__vtov(fname, in_msg.field_name, sizeof(in_msg.field_name));
		EXE_start(tdbb, request, gbl->gbl_transaction);
		EXE_send(tdbb, request, 0, sizeof(in_msg), (UCHAR*) &in_msg);
		EXE_receive(tdbb, request, 1, sizeof(SecurityClass::flags_t), (UCHAR*) prot_mask);

		DYN_rundown_request(request, drq_l_prot_mask);

	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, drq_l_prot_mask);
		return false;
	}
	return true;
}


void DYN_UTIL_store_check_constraints(thread_db*				tdbb,
									  Global*					gbl,
									  const Firebird::MetaName&	constraint_name,
									  const Firebird::MetaName&	trigger_name)
{
   struct {
          TEXT  jrd_118 [32];	// RDB$TRIGGER_NAME 
          TEXT  jrd_119 [32];	// RDB$CONSTRAINT_NAME 
   } jrd_117;
/**************************************
 *
 *	D Y N _ U T I L _s t o r e _ c h e c k _ c o n s t r a i n t s
 *
 **************************************
 *
 * Functional description
 *	Fill in rdb$check_constraints the association between a check name and the
 *	system defined trigger that implements that check.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_s_chk_con, DYN_REQUESTS);

	try
	{
		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			CHK IN RDB$CHECK_CONSTRAINTS*/
		{
		
	        strcpy(/*CHK.RDB$CONSTRAINT_NAME*/
		       jrd_117.jrd_119, constraint_name.c_str());
			strcpy(/*CHK.RDB$TRIGGER_NAME*/
			       jrd_117.jrd_118, trigger_name.c_str());

		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_116, sizeof(jrd_116), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_117);
		}

		if (!DYN_REQUEST(drq_s_chk_con)) {
			DYN_REQUEST(drq_s_chk_con) = request;
		}
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, drq_s_chk_con);
		DYN_error_punt(true, 122);
		// msg 122: "STORE RDB$CHECK_CONSTRAINTS failed"
	}
}


//***************************************
//
// 	D Y N _ U T I L _ i s _ a r r a y
//
//**************************************
//
// Functional description
//	Discover if a given domain (either automatic or explicit) has dimensions.
//
//***************************************
bool DYN_UTIL_is_array(thread_db* tdbb, Global* gbl, const Firebird::MetaName& domain_name)
{
   struct {
          SSHORT jrd_113;	// gds__utility 
          SSHORT jrd_114;	// gds__null_flag 
          SSHORT jrd_115;	// RDB$DIMENSIONS 
   } jrd_112;
   struct {
          TEXT  jrd_111 [32];	// RDB$FIELD_NAME 
   } jrd_110;
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_dom_is_array, DYN_REQUESTS);

	try
	{
		bool rc = false;
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			X IN RDB$FIELDS WITH X.RDB$FIELD_NAME EQ domain_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_109, sizeof(jrd_109), true);
		gds__vtov ((const char*) domain_name.c_str(), (char*) jrd_110.jrd_111, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_110);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_112);
		   if (!jrd_112.jrd_113) break;

			if (!DYN_REQUEST(drq_dom_is_array))
				DYN_REQUEST(drq_dom_is_array) = request;

			rc = !/*X.RDB$DIMENSIONS.NULL*/
			      jrd_112.jrd_114 && /*X.RDB$DIMENSIONS*/
    jrd_112.jrd_115 > 0;
		/*END_FOR*/
		   }
		}

		if (!DYN_REQUEST(drq_dom_is_array))
			DYN_REQUEST(drq_dom_is_array) = request;

		return rc;
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, drq_dom_is_array);
		DYN_error_punt(true, 227, domain_name.c_str());
		// msg 227: "DYN_UTIL_is_array failed for domain %s"
		return false; // Keep compiler happy.
	}
}


//***************************************
//
// 	D Y N _ U T I L _ c o p y _ d o m a i n
//
//**************************************
//
// Functional description
//	Copy a domain in another. More reliable than using dyn_fld as intermediate.
//  The source cannot be an array domain.
//  We don't copy the default, it may be useless work.
//
//***************************************
void DYN_UTIL_copy_domain(thread_db* tdbb,
						  Global* gbl,
						  const Firebird::MetaName& org_name,
						  const Firebird::MetaName& new_name)
{
   struct {
          TEXT  jrd_2 [128];	// RDB$EDIT_STRING 
          bid  jrd_3;	// RDB$QUERY_HEADER 
          bid  jrd_4;	// RDB$DESCRIPTION 
          bid  jrd_5;	// RDB$MISSING_SOURCE 
          bid  jrd_6;	// RDB$MISSING_VALUE 
          bid  jrd_7;	// RDB$COMPUTED_SOURCE 
          bid  jrd_8;	// RDB$COMPUTED_BLR 
          bid  jrd_9;	// RDB$VALIDATION_SOURCE 
          bid  jrd_10;	// RDB$VALIDATION_BLR 
          TEXT  jrd_11 [32];	// RDB$QUERY_NAME 
          TEXT  jrd_12 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_13;	// gds__null_flag 
          SSHORT jrd_14;	// RDB$FIELD_PRECISION 
          SSHORT jrd_15;	// gds__null_flag 
          SSHORT jrd_16;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_17;	// gds__null_flag 
          SSHORT jrd_18;	// RDB$COLLATION_ID 
          SSHORT jrd_19;	// gds__null_flag 
          SSHORT jrd_20;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_21;	// gds__null_flag 
          SSHORT jrd_22;	// RDB$NULL_FLAG 
          SSHORT jrd_23;	// gds__null_flag 
          SSHORT jrd_24;	// RDB$DIMENSIONS 
          SSHORT jrd_25;	// gds__null_flag 
          SSHORT jrd_26;	// RDB$EXTERNAL_TYPE 
          SSHORT jrd_27;	// gds__null_flag 
          SSHORT jrd_28;	// RDB$EXTERNAL_SCALE 
          SSHORT jrd_29;	// gds__null_flag 
          SSHORT jrd_30;	// RDB$EXTERNAL_LENGTH 
          SSHORT jrd_31;	// gds__null_flag 
          SSHORT jrd_32;	// gds__null_flag 
          SSHORT jrd_33;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_34;	// gds__null_flag 
          SSHORT jrd_35;	// gds__null_flag 
          SSHORT jrd_36;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_37;	// gds__null_flag 
          SSHORT jrd_38;	// gds__null_flag 
          SSHORT jrd_39;	// gds__null_flag 
          SSHORT jrd_40;	// gds__null_flag 
          SSHORT jrd_41;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_42;	// gds__null_flag 
          SSHORT jrd_43;	// RDB$FIELD_TYPE 
          SSHORT jrd_44;	// gds__null_flag 
          SSHORT jrd_45;	// RDB$FIELD_SCALE 
          SSHORT jrd_46;	// gds__null_flag 
          SSHORT jrd_47;	// RDB$FIELD_LENGTH 
          SSHORT jrd_48;	// gds__null_flag 
          SSHORT jrd_49;	// gds__null_flag 
          SSHORT jrd_50;	// gds__null_flag 
          SSHORT jrd_51;	// gds__null_flag 
          SSHORT jrd_52;	// gds__null_flag 
          SSHORT jrd_53;	// gds__null_flag 
   } jrd_1;
   struct {
          TEXT  jrd_58 [128];	// RDB$EDIT_STRING 
          bid  jrd_59;	// RDB$QUERY_HEADER 
          bid  jrd_60;	// RDB$DESCRIPTION 
          bid  jrd_61;	// RDB$MISSING_SOURCE 
          bid  jrd_62;	// RDB$MISSING_VALUE 
          bid  jrd_63;	// RDB$COMPUTED_SOURCE 
          bid  jrd_64;	// RDB$COMPUTED_BLR 
          bid  jrd_65;	// RDB$VALIDATION_SOURCE 
          bid  jrd_66;	// RDB$VALIDATION_BLR 
          TEXT  jrd_67 [32];	// RDB$QUERY_NAME 
          SSHORT jrd_68;	// gds__utility 
          SSHORT jrd_69;	// gds__null_flag 
          SSHORT jrd_70;	// RDB$FIELD_PRECISION 
          SSHORT jrd_71;	// gds__null_flag 
          SSHORT jrd_72;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_73;	// gds__null_flag 
          SSHORT jrd_74;	// RDB$COLLATION_ID 
          SSHORT jrd_75;	// gds__null_flag 
          SSHORT jrd_76;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_77;	// gds__null_flag 
          SSHORT jrd_78;	// RDB$NULL_FLAG 
          SSHORT jrd_79;	// gds__null_flag 
          SSHORT jrd_80;	// RDB$EXTERNAL_TYPE 
          SSHORT jrd_81;	// gds__null_flag 
          SSHORT jrd_82;	// RDB$EXTERNAL_SCALE 
          SSHORT jrd_83;	// gds__null_flag 
          SSHORT jrd_84;	// RDB$EXTERNAL_LENGTH 
          SSHORT jrd_85;	// gds__null_flag 
          SSHORT jrd_86;	// gds__null_flag 
          SSHORT jrd_87;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_88;	// gds__null_flag 
          SSHORT jrd_89;	// gds__null_flag 
          SSHORT jrd_90;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_91;	// gds__null_flag 
          SSHORT jrd_92;	// gds__null_flag 
          SSHORT jrd_93;	// gds__null_flag 
          SSHORT jrd_94;	// gds__null_flag 
          SSHORT jrd_95;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_96;	// gds__null_flag 
          SSHORT jrd_97;	// RDB$FIELD_TYPE 
          SSHORT jrd_98;	// gds__null_flag 
          SSHORT jrd_99;	// RDB$FIELD_SCALE 
          SSHORT jrd_100;	// gds__null_flag 
          SSHORT jrd_101;	// RDB$FIELD_LENGTH 
          SSHORT jrd_102;	// gds__null_flag 
          SSHORT jrd_103;	// gds__null_flag 
          SSHORT jrd_104;	// gds__null_flag 
          SSHORT jrd_105;	// gds__null_flag 
          SSHORT jrd_106;	// gds__null_flag 
          SSHORT jrd_107;	// gds__null_flag 
          SSHORT jrd_108;	// RDB$DIMENSIONS 
   } jrd_57;
   struct {
          TEXT  jrd_56 [32];	// RDB$FIELD_NAME 
   } jrd_55;
	SET_TDBB(tdbb);
	//Database* dbb = tdbb->getDatabase();

	jrd_req* request = NULL; //CMP_find_request(tdbb, drq_dom_copy, DYN_REQUESTS);
	jrd_req* req2 = NULL;

	try
	{
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			X IN RDB$FIELDS WITH X.RDB$FIELD_NAME EQ org_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_54, sizeof(jrd_54), true);
		gds__vtov ((const char*) org_name.c_str(), (char*) jrd_55.jrd_56, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_55);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 306, (UCHAR*) &jrd_57);
		   if (!jrd_57.jrd_68) break;

			//if (!DYN_REQUEST(drq_dom_copy))
			//	DYN_REQUEST(drq_dom_copy) = request;

			if (!/*X.RDB$DIMENSIONS.NULL*/
			     jrd_57.jrd_107 && /*X.RDB$DIMENSIONS*/
    jrd_57.jrd_108 > 0)
				ERR_punt();

			/*STORE(REQUEST_HANDLE req2 TRANSACTION_HANDLE gbl->gbl_transaction)
				X2 IN RDB$FIELDS*/
			{
			

				/*X2.RDB$FIELD_NAME.NULL*/
				jrd_1.jrd_53 = FALSE;
				strcpy(/*X2.RDB$FIELD_NAME*/
				       jrd_1.jrd_12, new_name.c_str());

				/*X2.RDB$QUERY_NAME.NULL*/
				jrd_1.jrd_52 = /*X.RDB$QUERY_NAME.NULL*/
   jrd_57.jrd_106;
				if (!/*X.RDB$QUERY_NAME.NULL*/
				     jrd_57.jrd_106)
					strcpy(/*X2.RDB$QUERY_NAME*/
					       jrd_1.jrd_11, /*X.RDB$QUERY_NAME*/
  jrd_57.jrd_67);

				// The following fields may require blob copying:
				// rdb$validation_blr, rdb$validation_source,
				// rdb$computed_blr, rdb$computed_source,
				// rdb$default_value, rdb$default_source,
				// rdb$missing_value,  rdb$missing_source,
				// rdb$description and rdb$query_header.

				/*X2.RDB$VALIDATION_BLR.NULL*/
				jrd_1.jrd_51    = /*X.RDB$VALIDATION_BLR.NULL*/
      jrd_57.jrd_105;
				/*X2.RDB$VALIDATION_BLR*/
				jrd_1.jrd_10         = /*X.RDB$VALIDATION_BLR*/
	   jrd_57.jrd_66;

				/*X2.RDB$VALIDATION_SOURCE.NULL*/
				jrd_1.jrd_50 = /*X.RDB$VALIDATION_SOURCE.NULL*/
   jrd_57.jrd_104;
				/*X2.RDB$VALIDATION_SOURCE*/
				jrd_1.jrd_9      = /*X.RDB$VALIDATION_SOURCE*/
	jrd_57.jrd_65;

				/*X2.RDB$COMPUTED_BLR.NULL*/
				jrd_1.jrd_49      = /*X.RDB$COMPUTED_BLR.NULL*/
	jrd_57.jrd_103;
				/*X2.RDB$COMPUTED_BLR*/
				jrd_1.jrd_8           = /*X.RDB$COMPUTED_BLR*/
	     jrd_57.jrd_64;

				/*X2.RDB$COMPUTED_SOURCE.NULL*/
				jrd_1.jrd_48   = /*X.RDB$COMPUTED_SOURCE.NULL*/
     jrd_57.jrd_102;
				/*X2.RDB$COMPUTED_SOURCE*/
				jrd_1.jrd_7        = /*X.RDB$COMPUTED_SOURCE*/
	  jrd_57.jrd_63;

				//X2.RDB$DEFAULT_VALUE.NULL     = X.RDB$DEFAULT_VALUE.NULL;
				//X2.RDB$DEFAULT_VALUE          = X.RDB$DEFAULT_VALUE;

				//X2.RDB$DEFAULT_SOURCE.NULL    = X.RDB$DEFAULT_SOURCE.NULL;
				//X2.RDB$DEFAULT_SOURCE         = X.RDB$DEFAULT_SOURCE;

				/*X2.RDB$FIELD_LENGTH.NULL*/
				jrd_1.jrd_46      = /*X.RDB$FIELD_LENGTH.NULL*/
	jrd_57.jrd_100;
				/*X2.RDB$FIELD_LENGTH*/
				jrd_1.jrd_47           = /*X.RDB$FIELD_LENGTH*/
	     jrd_57.jrd_101;

				/*X2.RDB$FIELD_SCALE.NULL*/
				jrd_1.jrd_44       = /*X.RDB$FIELD_SCALE.NULL*/
	 jrd_57.jrd_98;
				/*X2.RDB$FIELD_SCALE*/
				jrd_1.jrd_45            = /*X.RDB$FIELD_SCALE*/
	      jrd_57.jrd_99;

				/*X2.RDB$FIELD_TYPE.NULL*/
				jrd_1.jrd_42        = /*X.RDB$FIELD_TYPE.NULL*/
	  jrd_57.jrd_96;
				/*X2.RDB$FIELD_TYPE*/
				jrd_1.jrd_43             = /*X.RDB$FIELD_TYPE*/
	       jrd_57.jrd_97;

				/*X2.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_1.jrd_40    = /*X.RDB$FIELD_SUB_TYPE.NULL*/
      jrd_57.jrd_94;
				/*X2.RDB$FIELD_SUB_TYPE*/
				jrd_1.jrd_41         = /*X.RDB$FIELD_SUB_TYPE*/
	   jrd_57.jrd_95;

				/*X2.RDB$MISSING_VALUE.NULL*/
				jrd_1.jrd_39     = /*X.RDB$MISSING_VALUE.NULL*/
       jrd_57.jrd_93;
				/*X2.RDB$MISSING_VALUE*/
				jrd_1.jrd_6          = /*X.RDB$MISSING_VALUE*/
	    jrd_57.jrd_62;

				/*X2.RDB$MISSING_SOURCE.NULL*/
				jrd_1.jrd_38    = /*X.RDB$MISSING_SOURCE.NULL*/
      jrd_57.jrd_92;
				/*X2.RDB$MISSING_SOURCE*/
				jrd_1.jrd_5         = /*X.RDB$MISSING_SOURCE*/
	   jrd_57.jrd_61;

				/*X2.RDB$DESCRIPTION.NULL*/
				jrd_1.jrd_37       = /*X.RDB$DESCRIPTION.NULL*/
	 jrd_57.jrd_91;
				/*X2.RDB$DESCRIPTION*/
				jrd_1.jrd_4            = /*X.RDB$DESCRIPTION*/
	      jrd_57.jrd_60;

				/*X2.RDB$SYSTEM_FLAG.NULL*/
				jrd_1.jrd_35       = /*X.RDB$SYSTEM_FLAG.NULL*/
	 jrd_57.jrd_89;
				/*X2.RDB$SYSTEM_FLAG*/
				jrd_1.jrd_36            = /*X.RDB$SYSTEM_FLAG*/
	      jrd_57.jrd_90;

				/*X2.RDB$QUERY_HEADER.NULL*/
				jrd_1.jrd_34      = /*X.RDB$QUERY_HEADER.NULL*/
	jrd_57.jrd_88;
				/*X2.RDB$QUERY_HEADER*/
				jrd_1.jrd_3           = /*X.RDB$QUERY_HEADER*/
	     jrd_57.jrd_59;

				/*X2.RDB$SEGMENT_LENGTH.NULL*/
				jrd_1.jrd_32    = /*X.RDB$SEGMENT_LENGTH.NULL*/
      jrd_57.jrd_86;
				/*X2.RDB$SEGMENT_LENGTH*/
				jrd_1.jrd_33         = /*X.RDB$SEGMENT_LENGTH*/
	   jrd_57.jrd_87;

				/*X2.RDB$EDIT_STRING.NULL*/
				jrd_1.jrd_31       = /*X.RDB$EDIT_STRING.NULL*/
	 jrd_57.jrd_85;
				if (!/*X.RDB$EDIT_STRING.NULL*/
				     jrd_57.jrd_85)
					strcpy(/*X2.RDB$EDIT_STRING*/
					       jrd_1.jrd_2, /*X.RDB$EDIT_STRING*/
  jrd_57.jrd_58);

				/*X2.RDB$EXTERNAL_LENGTH.NULL*/
				jrd_1.jrd_29   = /*X.RDB$EXTERNAL_LENGTH.NULL*/
     jrd_57.jrd_83;
				/*X2.RDB$EXTERNAL_LENGTH*/
				jrd_1.jrd_30        = /*X.RDB$EXTERNAL_LENGTH*/
	  jrd_57.jrd_84;

				/*X2.RDB$EXTERNAL_SCALE.NULL*/
				jrd_1.jrd_27    = /*X.RDB$EXTERNAL_SCALE.NULL*/
      jrd_57.jrd_81;
				/*X2.RDB$EXTERNAL_SCALE*/
				jrd_1.jrd_28         = /*X.RDB$EXTERNAL_SCALE*/
	   jrd_57.jrd_82;

				/*X2.RDB$EXTERNAL_TYPE.NULL*/
				jrd_1.jrd_25     = /*X.RDB$EXTERNAL_TYPE.NULL*/
       jrd_57.jrd_79;
				/*X2.RDB$EXTERNAL_TYPE*/
				jrd_1.jrd_26          = /*X.RDB$EXTERNAL_TYPE*/
	    jrd_57.jrd_80;

				/*X2.RDB$DIMENSIONS.NULL*/
				jrd_1.jrd_23        = /*X.RDB$DIMENSIONS.NULL*/
	  jrd_57.jrd_107;
				/*X2.RDB$DIMENSIONS*/
				jrd_1.jrd_24             = /*X.RDB$DIMENSIONS*/
	       jrd_57.jrd_108;

				/*X2.RDB$NULL_FLAG.NULL*/
				jrd_1.jrd_21         = /*X.RDB$NULL_FLAG.NULL*/
	   jrd_57.jrd_77;
				/*X2.RDB$NULL_FLAG*/
				jrd_1.jrd_22              = /*X.RDB$NULL_FLAG*/
		jrd_57.jrd_78;

				/*X2.RDB$CHARACTER_LENGTH.NULL*/
				jrd_1.jrd_19  = /*X.RDB$CHARACTER_LENGTH.NULL*/
    jrd_57.jrd_75;
				/*X2.RDB$CHARACTER_LENGTH*/
				jrd_1.jrd_20       = /*X.RDB$CHARACTER_LENGTH*/
	 jrd_57.jrd_76;

				/*X2.RDB$COLLATION_ID.NULL*/
				jrd_1.jrd_17      = /*X.RDB$COLLATION_ID.NULL*/
	jrd_57.jrd_73;
				/*X2.RDB$COLLATION_ID*/
				jrd_1.jrd_18           = /*X.RDB$COLLATION_ID*/
	     jrd_57.jrd_74;

				/*X2.RDB$CHARACTER_SET_ID.NULL*/
				jrd_1.jrd_15  = /*X.RDB$CHARACTER_SET_ID.NULL*/
    jrd_57.jrd_71;
				/*X2.RDB$CHARACTER_SET_ID*/
				jrd_1.jrd_16       = /*X.RDB$CHARACTER_SET_ID*/
	 jrd_57.jrd_72;

				/*X2.RDB$FIELD_PRECISION.NULL*/
				jrd_1.jrd_13   = /*X.RDB$FIELD_PRECISION.NULL*/
     jrd_57.jrd_69;
				/*X2.RDB$FIELD_PRECISION*/
				jrd_1.jrd_14        = /*X.RDB$FIELD_PRECISION*/
	  jrd_57.jrd_70;
			/*END_STORE*/
			if (!req2)
			   req2 = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
			EXE_start (tdbb, req2, gbl->gbl_transaction);
			EXE_send (tdbb, req2, 0, 338, (UCHAR*) &jrd_1);
			}
			CMP_release(tdbb, req2);
			req2 = NULL;
		/*END_FOR*/
		   }
		}

		CMP_release(tdbb, request);
		request = NULL;
		// For now, CMP_release used instead of this:
		//if (!DYN_REQUEST(drq_dom_copy))
		//	DYN_REQUEST(drq_dom_copy) = request;

	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1); //drq_dom_copy);
		DYN_error_punt(true, 228, org_name.c_str());
		// msg 228: "DYN_UTIL_copy_domain failed for domain %s"
	}
}
