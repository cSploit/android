/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
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
 * Adriano dos Santos Fernandes - refactored from pass1.cpp, ddl.cpp, dyn*.epp
 */

#include "firebird.h"
#include "../jrd/common.h"
#include "../dsql/DdlNodes.h"
#include "../jrd/jrd.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/met_proto.h"
#include "../dsql/metd_proto.h"

using namespace Firebird;

namespace Jrd {


/*DATABASE DB = STATIC "ODS.RDB";*/
static const UCHAR	jrd_0 [83] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,7,0,12,0,2,7,'C',1,'K',29,
0,0,'G',58,47,24,0,2,0,25,0,1,0,47,24,0,0,0,25,0,0,0,255,14,1,
2,1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,
0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_6 [146] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,41,3,0,32,0,7,0,4,1,4,0,41,3,0,32,0,7,
0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',28,0,0,'G',47,
24,0,0,0,25,0,0,0,255,2,14,1,2,1,24,0,3,0,41,1,0,0,2,0,1,21,8,
0,1,0,0,0,25,1,1,0,1,24,0,4,0,25,1,3,0,255,17,0,9,13,12,3,18,
0,12,2,10,0,1,2,1,41,2,0,0,1,0,24,1,3,0,255,255,255,14,1,1,21,
8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 



void AlterCharSetNode::print(string& text, Array<dsql_nod*>& /*nodes*/) const
{
	text.printf(
		"alter character set\n"
		"	charSet: %s\n"
		"	defaultCollation: %s\n",
		charSet.c_str(), defaultCollation.c_str());
}


void AlterCharSetNode::execute(thread_db* tdbb, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_5;	// gds__utility 
   } jrd_4;
   struct {
          TEXT  jrd_2 [32];	// RDB$COLLATION_NAME 
          SSHORT jrd_3;	// RDB$CHARACTER_SET_ID 
   } jrd_1;
   struct {
          SSHORT jrd_18;	// gds__utility 
   } jrd_17;
   struct {
          TEXT  jrd_15 [32];	// RDB$DEFAULT_COLLATE_NAME 
          SSHORT jrd_16;	// gds__null_flag 
   } jrd_14;
   struct {
          TEXT  jrd_10 [32];	// RDB$DEFAULT_COLLATE_NAME 
          SSHORT jrd_11;	// gds__utility 
          SSHORT jrd_12;	// gds__null_flag 
          SSHORT jrd_13;	// RDB$CHARACTER_SET_ID 
   } jrd_9;
   struct {
          TEXT  jrd_8 [32];	// RDB$CHARACTER_SET_NAME 
   } jrd_7;
	if (compiledStatement && compiledStatement->req_dbb)	// do not run in CREATE DATABASE
	{
		METD_drop_charset(compiledStatement, charSet);
		MET_dsql_cache_release(tdbb, SYM_intlsym_charset, charSet);
	}

	Database* dbb = tdbb->getDatabase();
	bool charSetFound = false;
	bool collationFound = false;

	jrd_req* request1 = CMP_find_request(tdbb, drq_m_charset, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE request1 TRANSACTION_HANDLE transaction)
		CS IN RDB$CHARACTER_SETS
		WITH CS.RDB$CHARACTER_SET_NAME EQ charSet.c_str()*/
	{
	if (!request1)
	   request1 = CMP_compile2 (tdbb, (UCHAR*) jrd_6, sizeof(jrd_6), true);
	gds__vtov ((const char*) charSet.c_str(), (char*) jrd_7.jrd_8, 32);
	EXE_start (tdbb, request1, transaction);
	EXE_send (tdbb, request1, 0, 32, (UCHAR*) &jrd_7);
	while (1)
	   {
	   EXE_receive (tdbb, request1, 1, 38, (UCHAR*) &jrd_9);
	   if (!jrd_9.jrd_11) break;

		charSetFound = true;

		if (!DYN_REQUEST(drq_m_charset))
			DYN_REQUEST(drq_m_charset) = request1;

		jrd_req* request2 = CMP_find_request(tdbb, drq_l_collation, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
			COLL IN RDB$COLLATIONS
			WITH COLL.RDB$CHARACTER_SET_ID EQ CS.RDB$CHARACTER_SET_ID AND
				COLL.RDB$COLLATION_NAME EQ defaultCollation.c_str()*/
		{
		if (!request2)
		   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
		gds__vtov ((const char*) defaultCollation.c_str(), (char*) jrd_1.jrd_2, 32);
		jrd_1.jrd_3 = jrd_9.jrd_13;
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 34, (UCHAR*) &jrd_1);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 2, (UCHAR*) &jrd_4);
		   if (!jrd_4.jrd_5) break;

			if (!DYN_REQUEST(drq_l_collation))
				DYN_REQUEST(drq_l_collation) = request2;

			collationFound = true;
		/*END_FOR*/
		   }
		}

		if (!DYN_REQUEST(drq_l_collation))
			DYN_REQUEST(drq_l_collation) = request2;

		if (collationFound)
		{
			/*MODIFY CS*/
			{
			
				/*CS.RDB$DEFAULT_COLLATE_NAME.NULL*/
				jrd_9.jrd_12 = FALSE;
				strcpy(/*CS.RDB$DEFAULT_COLLATE_NAME*/
				       jrd_9.jrd_10, defaultCollation.c_str());
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_9.jrd_10, (char*) jrd_14.jrd_15, 32);
			jrd_14.jrd_16 = jrd_9.jrd_12;
			EXE_send (tdbb, request1, 2, 34, (UCHAR*) &jrd_14);
			}
		}
	/*END_FOR*/
	   EXE_send (tdbb, request1, 3, 2, (UCHAR*) &jrd_17);
	   }
	}

	if (!DYN_REQUEST(drq_m_charset))
		DYN_REQUEST(drq_m_charset) = request1;

	if (!charSetFound)
	{
		status_exception::raise(Arg::Gds(isc_charset_not_found) << Arg::Str(charSet));
	}

	if (!collationFound)
	{
		status_exception::raise(Arg::Gds(isc_collation_not_found) << Arg::Str(defaultCollation) <<
																	 Arg::Str(charSet));
	}
}


}	// namespace Jrd
