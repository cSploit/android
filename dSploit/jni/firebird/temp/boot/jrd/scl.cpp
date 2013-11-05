/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		scl.epp
 *	DESCRIPTION:	Security class handler
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
 * 2001.6.12 Claudio Valderrama: the role should be wiped out if invalid.
 * 2001.8.12 Claudio Valderrama: Squash security bug when processing
 *           identifiers with embedded blanks: check_procedure, check_relation
 *           and check_string, the latter being called from many places.
 *
 */

#include "firebird.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "../jrd/common.h"
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
#include "../jrd/scl.h"
#include "../jrd/jrd_pwd.h"
#include "../jrd/acl.h"
#include "../jrd/blb.h"
#include "../jrd/irq.h"
#include "../jrd/obj.h"
#include "../jrd/req.h"
#include "../jrd/tra.h"
#include "../jrd/gdsassert.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/enc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/grant_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/constants.h"
#include "../include/fb_exception.h"
#include "../common/utils_proto.h"
#include "../common/classes/array.h"
#include "../common/config/config.h"
#include "../jrd/os/os_utils.h"


const int UIC_BASE	= 10;

using namespace Jrd;
using namespace Firebird;

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [159] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,4,0,41,3,0,32,0,41,3,0,32,
0,7,0,7,0,12,0,2,7,'C',1,'K',18,0,0,'G',58,57,47,24,0,0,0,25,
0,1,0,47,24,0,0,0,21,15,3,0,6,0,'P','U','B','L','I','C',58,47,
24,0,6,0,25,0,3,0,58,47,24,0,4,0,25,0,0,0,58,47,24,0,7,0,25,0,
2,0,47,24,0,2,0,21,15,3,0,1,0,'M',255,14,1,2,1,24,0,0,0,41,1,
0,0,2,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,
1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_10 [82] =
   {	// blr string 

4,2,4,1,2,0,9,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',9,
0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,1,0,25,1,0,0,1,
21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_16 [86] =
   {	// blr string 

4,2,4,1,3,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
31,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,
1,0,0,1,24,0,3,0,41,1,2,0,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_23 [100] =
   {	// blr string 

4,2,4,0,3,0,41,3,0,32,0,7,0,7,0,2,7,'C',1,'K',6,0,0,'D',21,8,
0,1,0,0,0,'G',47,24,0,8,0,21,15,3,0,12,0,'R','D','B','$','D',
'A','T','A','B','A','S','E',255,14,0,2,1,24,0,13,0,41,0,0,0,2,
0,1,21,8,0,1,0,0,0,25,0,1,0,255,14,0,1,21,8,0,0,0,0,0,25,0,1,
0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_28 [68] =
   {	// blr string 

4,2,4,0,3,0,41,3,0,32,0,7,0,7,0,2,7,'C',1,'K',1,0,0,255,14,0,
2,1,24,0,2,0,41,0,0,0,2,0,1,21,8,0,1,0,0,0,25,0,1,0,255,14,0,
1,21,8,0,0,0,0,0,25,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_33 [97] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',31,
0,0,'D',21,8,0,1,0,0,0,'G',58,47,24,0,0,0,25,0,0,0,59,61,24,0,
3,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,3,0,25,1,1,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_39 [181] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,4,0,41,3,0,32,0,41,3,0,32,
0,7,0,7,0,12,0,2,7,'C',2,'K',31,0,0,'K',18,0,1,'D',21,8,0,1,0,
0,0,'G',58,47,24,0,0,0,25,0,1,0,58,47,24,0,0,0,24,1,4,0,58,47,
24,1,7,0,25,0,3,0,58,57,47,24,1,0,0,25,0,0,0,47,24,1,0,0,21,15,
3,0,6,0,'P','U','B','L','I','C',58,47,24,1,6,0,25,0,2,0,47,24,
1,2,0,21,15,3,0,1,0,'M',255,14,1,2,1,24,1,0,0,41,1,0,0,2,0,1,
21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_49 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',31,0,0,
'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_54 [89] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,
'C',1,'K',6,0,0,'G',47,24,0,8,0,25,0,0,0,255,14,1,2,1,24,0,9,
0,41,1,0,0,2,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,
0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_61 [89] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,
'C',1,'K',26,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,7,
0,41,1,0,0,2,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,
0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_68 [132] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,4,0,2,0,41,3,0,32,
0,41,3,0,32,0,12,0,2,7,'C',2,'K',3,0,0,'K',5,0,1,'G',58,47,24,
1,0,0,24,0,1,0,58,47,24,1,1,0,25,0,1,0,47,24,0,0,0,25,0,0,0,255,
14,1,2,1,24,1,0,0,25,1,0,0,1,24,1,14,0,41,1,1,0,3,0,1,21,8,0,
1,0,0,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_77 [161] =
   {	// blr string 

4,2,4,1,7,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,
0,7,0,7,0,4,0,2,0,41,3,0,32,0,7,0,12,0,2,7,'C',2,'K',4,0,0,'K',
6,0,1,'G',58,47,24,1,8,0,24,0,1,0,58,47,24,0,1,0,25,0,0,0,47,
24,0,2,0,25,0,1,0,255,14,1,2,1,24,1,14,0,41,1,0,0,5,0,1,24,1,
9,0,41,1,1,0,6,0,1,24,0,0,0,25,1,2,0,1,24,1,8,0,25,1,3,0,1,21,
8,0,1,0,0,0,25,1,4,0,255,14,1,1,21,8,0,0,0,0,0,25,1,4,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_89 [135] =
   {	// blr string 

4,2,4,1,6,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,4,
0,1,0,41,3,0,32,0,12,0,2,7,'C',2,'K',4,0,0,'K',6,0,1,'G',58,47,
24,1,8,0,24,0,1,0,47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,1,14,0,
41,1,0,0,4,0,1,24,1,9,0,41,1,1,0,5,0,1,24,1,8,0,25,1,2,0,1,21,
8,0,1,0,0,0,25,1,3,0,255,14,1,1,21,8,0,0,0,0,0,25,1,3,0,255,255,
76
   };	// end of blr string 


static bool check_hex(const UCHAR*, USHORT);
static bool check_number(const UCHAR*, USHORT);
static bool check_user_group(thread_db* tdbb, const UCHAR*, USHORT);
static bool check_string(const UCHAR*, const Firebird::MetaName&);
static SecurityClass::flags_t compute_access(thread_db* tdbb, const SecurityClass*,
	const jrd_rel*,	const Firebird::MetaName&, const Firebird::MetaName&);
static SecurityClass::flags_t walk_acl(thread_db* tdbb, const Acl&, const jrd_rel*,
	const Firebird::MetaName&, const Firebird::MetaName&);

static inline void check_and_move(UCHAR from, Acl& to)
{
	to.push(from);
}

struct P_NAMES {
	SecurityClass::flags_t p_names_priv;
	USHORT p_names_acl;
	const TEXT* p_names_string;
};

static const P_NAMES p_names[] =
{
	{ SCL_protect, priv_protect, "protect" },
	{ SCL_control, priv_control, "control" },
	{ SCL_delete, priv_delete, "delete" },
	{ SCL_sql_insert, priv_sql_insert, "insert/write" },
	{ SCL_sql_update, priv_sql_update, "update/write" },
	{ SCL_sql_delete, priv_sql_delete, "delete/write" },
	{ SCL_write, priv_write, "write" },
	{ SCL_read, priv_read, "read/select" },
	{ SCL_grant, priv_grant, "grant" },
	{ SCL_sql_references, priv_sql_references, "references" },
	{ SCL_execute, priv_execute, "execute" },
	{ 0, 0, "" }
};


void SCL_check_access(thread_db* tdbb,
					  const SecurityClass* s_class,
					  SLONG view_id,
					  const Firebird::MetaName& trg_name,
					  const Firebird::MetaName& prc_name,
					  SecurityClass::flags_t mask,
					  const TEXT* type,
					  const Firebird::MetaName& name,
					  const Firebird::MetaName& r_name)
{
/**************************************
 *
 *	S C L _ c h e c k _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *	Check security class for desired permission.  Check first that
 *	the desired access has been granted to the database then to the
 *	object in question.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (s_class && (s_class->scl_flags & SCL_corrupt))
	{
		ERR_post(Arg::Gds(isc_no_priv) << Arg::Str("(ACL unrecognized)") <<
										  Arg::Str("security_class") <<
										  Arg::Str(s_class->scl_name));
	}

	const Attachment& attachment = *tdbb->getAttachment();

	// Allow the database owner to back up a database even if he does not have
	// read access to all the tables in the database

	if ((attachment.att_flags & ATT_gbak_attachment) && (mask & SCL_read))
	{
		return;
	}

	// Allow the locksmith any access to database

	if (attachment.locksmith())
	{
		return;
	}

	bool denied_db = false;

	const SecurityClass* const att_class = attachment.att_security_class;
	if (att_class && !(att_class->scl_flags & mask))
	{
		denied_db = true;
	}
	else
	{
		if (!s_class ||	(mask & s_class->scl_flags)) {
			return;
		}
		const jrd_rel* view = NULL;
		if (view_id) {
			view = MET_lookup_relation_id(tdbb, view_id, false);
		}
		if ((view || trg_name.length() || prc_name.length()) &&
			 (compute_access(tdbb, s_class, view, trg_name, prc_name) & mask))
		{
			return;
		}
	}

	const P_NAMES* names;
	for (names = p_names; names->p_names_priv; names++)
	{
		if (names->p_names_priv & mask)
		{
			break;
		}
	}

	if (denied_db)
	{
		ERR_post(Arg::Gds(isc_no_priv) << Arg::Str(names->p_names_string) <<
										  Arg::Str(object_database) <<
										  Arg::Str(""));
	}
	else
	{
		const Firebird::string full_name = r_name.hasData() ?
			r_name.c_str() + Firebird::string(".") + name.c_str() : name.c_str();

		ERR_post(Arg::Gds(isc_no_priv) << Arg::Str(names->p_names_string) <<
										  Arg::Str(type) <<
										  Arg::Str(full_name));
	}
}


void SCL_check_index(thread_db* tdbb, const Firebird::MetaName& index_name, UCHAR index_id,
	SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_73 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_74 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_75;	// gds__utility 
          SSHORT jrd_76;	// gds__null_flag 
   } jrd_72;
   struct {
          TEXT  jrd_70 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_71 [32];	// RDB$RELATION_NAME 
   } jrd_69;
   struct {
          TEXT  jrd_82 [32];	// RDB$DEFAULT_CLASS 
          TEXT  jrd_83 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_84 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_85 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_86;	// gds__utility 
          SSHORT jrd_87;	// gds__null_flag 
          SSHORT jrd_88;	// gds__null_flag 
   } jrd_81;
   struct {
          TEXT  jrd_79 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_80;	// RDB$INDEX_ID 
   } jrd_78;
   struct {
          TEXT  jrd_93 [32];	// RDB$DEFAULT_CLASS 
          TEXT  jrd_94 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_95 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_96;	// gds__utility 
          SSHORT jrd_97;	// gds__null_flag 
          SSHORT jrd_98;	// gds__null_flag 
   } jrd_92;
   struct {
          TEXT  jrd_91 [32];	// RDB$INDEX_NAME 
   } jrd_90;
/******************************************************
 *
 *	S C L _ c h e c k _ i n d e x
 *
 ******************************************************
 *
 * Functional description
 *	Given a index name (as a TEXT), check for a
 *      set of privileges on the table that the index is on and
 *      on the fields involved in that index.
 *   CVC: Allow the same function to use the zero-based index id, too.
 *      The idx.idx_id value is zero based but system tables use
 *      index id's being one based, hence adjust the incoming value
 *      before calling this function. If you use index_id, index_name
 *      becomes relation_name since index ids are relative to tables.
 *
 *******************************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	const SecurityClass* s_class = NULL;
	const SecurityClass* default_s_class = NULL;

	// No security to check for if the index is not yet created

    if ((index_name.length() == 0) && (index_id < 1)) {
        return;
    }

    Firebird::MetaName reln_name, aux_idx_name;
    const Firebird::MetaName* idx_name_ptr = &index_name;
	const Firebird::MetaName* relation_name_ptr = &index_name;

	jrd_req* request = NULL;

	// No need to cache this request handle, it's only used when
	// new constraints are created

    if (index_id < 1) {
        /*FOR(REQUEST_HANDLE request) IND IN RDB$INDICES
            CROSS REL IN RDB$RELATIONS
			OVER RDB$RELATION_NAME
			WITH IND.RDB$INDEX_NAME EQ index_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_89, sizeof(jrd_89), true);
	gds__vtov ((const char*) index_name.c_str(), (char*) jrd_90.jrd_91, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_90);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 102, (UCHAR*) &jrd_92);
	   if (!jrd_92.jrd_96) break;

            reln_name = /*REL.RDB$RELATION_NAME*/
			jrd_92.jrd_95;
		    if (!/*REL.RDB$SECURITY_CLASS.NULL*/
			 jrd_92.jrd_98)
                s_class = SCL_get_class(tdbb, /*REL.RDB$SECURITY_CLASS*/
					      jrd_92.jrd_94);
            if (!/*REL.RDB$DEFAULT_CLASS.NULL*/
		 jrd_92.jrd_97)
                default_s_class = SCL_get_class(tdbb, /*REL.RDB$DEFAULT_CLASS*/
						      jrd_92.jrd_93);
        /*END_FOR;*/
	   }
	}

        CMP_release(tdbb, request);
    }
    else {
        idx_name_ptr = &aux_idx_name;
        /*FOR (REQUEST_HANDLE request) IND IN RDB$INDICES
            CROSS REL IN RDB$RELATIONS
            OVER RDB$RELATION_NAME
            WITH IND.RDB$RELATION_NAME EQ relation_name_ptr->c_str()
            AND IND.RDB$INDEX_ID EQ index_id*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_77, sizeof(jrd_77), true);
	gds__vtov ((const char*) relation_name_ptr->c_str(), (char*) jrd_78.jrd_79, 32);
	jrd_78.jrd_80 = index_id;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_78);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 134, (UCHAR*) &jrd_81);
	   if (!jrd_81.jrd_86) break;

            reln_name = /*REL.RDB$RELATION_NAME*/
			jrd_81.jrd_85;
            aux_idx_name = /*IND.RDB$INDEX_NAME*/
			   jrd_81.jrd_84;
            if (!/*REL.RDB$SECURITY_CLASS.NULL*/
		 jrd_81.jrd_88)
                s_class = SCL_get_class(tdbb, /*REL.RDB$SECURITY_CLASS*/
					      jrd_81.jrd_83);
            if (!/*REL.RDB$DEFAULT_CLASS.NULL*/
		 jrd_81.jrd_87)
                default_s_class = SCL_get_class(tdbb, /*REL.RDB$DEFAULT_CLASS*/
						      jrd_81.jrd_82);
        /*END_FOR;*/
	   }
	}

        CMP_release (tdbb, request);
    }

	// Check if the relation exists. It may not have been created yet.
	// Just return in that case.

	if (reln_name.length() == 0) {
		return;
	}

	SCL_check_access(tdbb, s_class, 0, NULL, NULL, mask, object_table, reln_name);

	request = NULL;

	// Set up the exception mechanism, so that we can release the request
	// in case of error in SCL_check_access

	try {

	// Check if the field used in the index has the appropriate
	// permission. If the field in question does not have a security class
	// defined, then the default security class for the table applies for that
	// field.

	// No need to cache this request handle, it's only used when
	// new constraints are created

	/*FOR(REQUEST_HANDLE request) ISEG IN RDB$INDEX_SEGMENTS
		CROSS RF IN RDB$RELATION_FIELDS
			OVER RDB$FIELD_NAME
			WITH RF.RDB$RELATION_NAME EQ reln_name.c_str()
			AND ISEG.RDB$INDEX_NAME EQ idx_name_ptr->c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_68, sizeof(jrd_68), true);
	gds__vtov ((const char*) idx_name_ptr->c_str(), (char*) jrd_69.jrd_70, 32);
	gds__vtov ((const char*) reln_name.c_str(), (char*) jrd_69.jrd_71, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_69);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 68, (UCHAR*) &jrd_72);
	   if (!jrd_72.jrd_75) break;

		s_class = (!/*RF.RDB$SECURITY_CLASS.NULL*/
			    jrd_72.jrd_76) ?
			SCL_get_class(tdbb, /*RF.RDB$SECURITY_CLASS*/
					    jrd_72.jrd_74) : default_s_class;
		SCL_check_access(tdbb, s_class, 0, NULL, NULL, mask,
						 object_column, /*RF.RDB$FIELD_NAME*/
								jrd_72.jrd_73, reln_name);

	/*END_FOR;*/
	   }
	}

	CMP_release(tdbb, request);
	}
	catch (const Firebird::Exception&) {
		if (request) {
			CMP_release(tdbb, request);
		}
		throw;
	}
}


void SCL_check_procedure(thread_db* tdbb, const dsc* dsc_name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_65 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_66;	// gds__utility 
          SSHORT jrd_67;	// gds__null_flag 
   } jrd_64;
   struct {
          TEXT  jrd_63 [32];	// RDB$PROCEDURE_NAME 
   } jrd_62;
/**************************************
 *
 *	S C L _ c h e c k _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Given a procedure name, check for a set of privileges.  The
 *	procedure in question may or may not have been created, let alone
 *	scanned.  This is used exclusively for meta-data operations.
 *
 **************************************/
	SET_TDBB(tdbb);

	// Get the name in CSTRING format, ending on NULL or SPACE
	fb_assert(dsc_name->dsc_dtype == dtype_text);
	const Firebird::MetaName name(reinterpret_cast<TEXT*>(dsc_name->dsc_address),
							dsc_name->dsc_length);

	Database* dbb = tdbb->getDatabase();
	const SecurityClass* s_class = NULL;

	jrd_req* request = CMP_find_request(tdbb, irq_p_security, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request) SPROC IN RDB$PROCEDURES
		WITH SPROC.RDB$PROCEDURE_NAME EQ name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_61, sizeof(jrd_61), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_62.jrd_63, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_62);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_64);
	   if (!jrd_64.jrd_66) break;

        if (!REQUEST(irq_p_security))
			REQUEST(irq_p_security) = request;

		if (!/*SPROC.RDB$SECURITY_CLASS.NULL*/
		     jrd_64.jrd_67)
			s_class = SCL_get_class(tdbb, /*SPROC.RDB$SECURITY_CLASS*/
						      jrd_64.jrd_65);
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_p_security))
		REQUEST(irq_p_security) = request;

	SCL_check_access(tdbb, s_class, 0, NULL, name, mask, object_procedure, name);
}


void SCL_check_relation(thread_db* tdbb, const dsc* dsc_name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_58 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_59;	// gds__utility 
          SSHORT jrd_60;	// gds__null_flag 
   } jrd_57;
   struct {
          TEXT  jrd_56 [32];	// RDB$RELATION_NAME 
   } jrd_55;
/**************************************
 *
 *	S C L _ c h e c k _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Given a relation name, check for a set of privileges.  The
 *	relation in question may or may not have been created, let alone
 *	scanned.  This is used exclusively for meta-data operations.
 *
 **************************************/
	SET_TDBB(tdbb);

	// Get the name in CSTRING format, ending on NULL or SPACE
	fb_assert(dsc_name->dsc_dtype == dtype_text);
	const Firebird::MetaName name(reinterpret_cast<TEXT*>(dsc_name->dsc_address),
							dsc_name->dsc_length);

	Database* dbb = tdbb->getDatabase();

	const SecurityClass* s_class = NULL;

	jrd_req* request = CMP_find_request(tdbb, irq_v_security, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request) REL IN RDB$RELATIONS
		WITH REL.RDB$RELATION_NAME EQ name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_54, sizeof(jrd_54), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_55.jrd_56, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_55);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_57);
	   if (!jrd_57.jrd_59) break;

        if (!REQUEST(irq_v_security))
			REQUEST(irq_v_security) = request;

        if (!/*REL.RDB$SECURITY_CLASS.NULL*/
	     jrd_57.jrd_60)
			s_class = SCL_get_class(tdbb, /*REL.RDB$SECURITY_CLASS*/
						      jrd_57.jrd_58);
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_v_security))
		REQUEST(irq_v_security) = request;

	SCL_check_access(tdbb, s_class, 0, NULL, NULL, mask, object_table, name);
}


SecurityClass* SCL_get_class(thread_db* tdbb, const TEXT* par_string)
{
/**************************************
 *
 *	S C L _ g e t _ c l a s s
 *
 **************************************
 *
 * Functional description
 *	Look up security class first in memory, then in database.  If
 *	we don't find it, just return NULL.  If we do, return a security
 *	class block.
 *
 **************************************/
	SET_TDBB(tdbb);
	//Database* dbb = tdbb->getDatabase();

	// Name may be absent or terminated with NULL or blank.  Clean up name.

	if (!par_string) {
		return NULL;
	}

	Firebird::MetaName string(par_string);

	//fb_utils::exact_name(string);

	if (string.isEmpty()) {
		return NULL;
	}

	Attachment* attachment = tdbb->getAttachment();

	// Look for the class already known

	SecurityClassList* list = attachment->att_security_classes;
	if (list && list->locate(string))
		return list->current();

	// Class isn't known. So make up a new security class block.

	MemoryPool& pool = *attachment->att_pool;

	SecurityClass* const s_class = FB_NEW(pool) SecurityClass(pool, string);
	s_class->scl_flags = compute_access(tdbb, s_class, NULL, NULL, NULL);

	if (s_class->scl_flags & SCL_exists)
	{
		if (!list) {
			attachment->att_security_classes = list = FB_NEW(pool) SecurityClassList(pool);
		}

		list->add(s_class);
		return s_class;
	}

	delete s_class;

	return NULL;
}


SecurityClass::flags_t SCL_get_mask(thread_db* tdbb, const TEXT* relation_name, const TEXT* field_name)
{
/**************************************
 *
 *	S C L _ g e t _ m a s k
 *
 **************************************
 *
 * Functional description
 *	Get a protection mask for a named object.  If field and
 *	relation names are present, get access to field.  If just
 *	relation name, get access to relation.  If neither, get
 *	access for database.
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();

	// Start with database security class

	const SecurityClass* s_class = attachment->att_security_class;
	SecurityClass::flags_t access = s_class ? s_class->scl_flags : -1;

	// If there's a relation, track it down
	jrd_rel* relation;
	if (relation_name && (relation = MET_lookup_relation(tdbb, relation_name)))
	{
		MET_scan_relation(tdbb, relation);
		if ( (s_class = SCL_get_class(tdbb, relation->rel_security_name.c_str())) )
		{
			access &= s_class->scl_flags;
		}

		const jrd_fld* field;
		SSHORT id;
		if (field_name &&
			(id = MET_lookup_field(tdbb, relation, field_name)) >= 0 &&
			(field = MET_get_field(relation, id)) &&
			(s_class = SCL_get_class(tdbb, field->fld_security_name.c_str())))
		{
			access &= s_class->scl_flags;
		}
	}

	return access & (SCL_read | SCL_write | SCL_delete | SCL_control |
					 SCL_grant | SCL_sql_insert | SCL_sql_update |
					 SCL_sql_delete | SCL_protect | SCL_sql_references |
					 SCL_execute);
}


void SCL_init(thread_db* tdbb, bool create, const UserId& tempId)
{
   struct {
          SSHORT jrd_20;	// gds__utility 
          SSHORT jrd_21;	// gds__null_flag 
          SSHORT jrd_22;	// RDB$SYSTEM_FLAG 
   } jrd_19;
   struct {
          TEXT  jrd_18 [32];	// RDB$ROLE_NAME 
   } jrd_17;
   struct {
          TEXT  jrd_25 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_26;	// gds__utility 
          SSHORT jrd_27;	// gds__null_flag 
   } jrd_24;
   struct {
          TEXT  jrd_30 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_31;	// gds__utility 
          SSHORT jrd_32;	// gds__null_flag 
   } jrd_29;
   struct {
          SSHORT jrd_37;	// gds__utility 
          SSHORT jrd_38;	// RDB$SYSTEM_FLAG 
   } jrd_36;
   struct {
          TEXT  jrd_35 [32];	// RDB$ROLE_NAME 
   } jrd_34;
   struct {
          TEXT  jrd_46 [32];	// RDB$USER 
          SSHORT jrd_47;	// gds__utility 
          SSHORT jrd_48;	// gds__null_flag 
   } jrd_45;
   struct {
          TEXT  jrd_41 [32];	// RDB$USER 
          TEXT  jrd_42 [32];	// RDB$ROLE_NAME 
          SSHORT jrd_43;	// RDB$USER_TYPE 
          SSHORT jrd_44;	// RDB$OBJECT_TYPE 
   } jrd_40;
   struct {
          SSHORT jrd_53;	// gds__utility 
   } jrd_52;
   struct {
          TEXT  jrd_51 [32];	// RDB$ROLE_NAME 
   } jrd_50;
/**************************************
 *
 *	S C L _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Check database access control list.
 *
 *	Checks the userinfo database to get the
 *	password and other stuff about the specified
 *	user.   Compares the password to that passed
 *	in, encrypting if necessary.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	const USHORT major_version = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	const TEXT* sql_role = tempId.usr_sql_role_name.nullStr();
	Firebird::string loginName(tempId.usr_user_name);
	loginName.upper();
	const TEXT* login_name = loginName.c_str();
	Firebird::MetaName role_name;
    // CVC: Let's clean any role in pre-ODS9 attachments
    bool preODS9 = true;

/***************************************************************
**
** skip reading system relation RDB$ROLES when attaching pre ODS_9_0 database
**
****************************************************************/

    // CVC: We'll verify the role and wipe it out when it doesn't exist

	if (ENCODE_ODS(major_version, minor_original) >= ODS_9_0) {

        preODS9 = false;

		if (strlen(login_name) != 0)
		{
			if (!create)
			{
				jrd_req* request = CMP_find_request(tdbb, irq_get_role_name, IRQ_REQUESTS);

				/*FOR(REQUEST_HANDLE request) X IN RDB$ROLES
					WITH X.RDB$ROLE_NAME EQ login_name*/
				{
				if (!request)
				   request = CMP_compile2 (tdbb, (UCHAR*) jrd_49, sizeof(jrd_49), true);
				gds__vtov ((const char*) login_name, (char*) jrd_50.jrd_51, 32);
				EXE_start (tdbb, request, dbb->dbb_sys_trans);
				EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_50);
				while (1)
				   {
				   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_52);
				   if (!jrd_52.jrd_53) break;

                    if (!REQUEST(irq_get_role_name))
                        REQUEST(irq_get_role_name) = request;

					EXE_unwind(tdbb, request);
					ERR_post(Arg::Gds(isc_login_same_as_role_name) << Arg::Str(login_name));

				/*END_FOR;*/
				   }
				}

				if (!REQUEST(irq_get_role_name))
					REQUEST(irq_get_role_name) = request;
			}
		}

        // CVC: If this is ODS>=ODS_9_0 and we aren't creating a db and sql_role was specified,
        // then verify it against rdb$roles and rdb$user_privileges

        if (!create && sql_role && *sql_role && strcmp(sql_role, NULL_ROLE)) {
            bool found = false;

			if (!(tempId.usr_flags & USR_trole))
			{
	            jrd_req* request = CMP_find_request (tdbb, irq_verify_role_name, IRQ_REQUESTS);

	            // CVC: The caller has hopefully uppercased the role or stripped quotes. Of course,
    	        // uppercase-UPPER7 should only happen if the role wasn't enclosed in quotes.
        	    // Shortsighted developers named the field rdb$relation_name instead of rdb$object_name.
    	       	// This request is not exactly the same than irq_get_role_mem, sorry, I can't reuse that.
	            // If you think that an unknown role cannot be granted, think again: someone made sure
        	    // in DYN that SYSDBA can do almost anything, including invalid grants.

            	/*FOR (REQUEST_HANDLE request) FIRST 1 RR IN RDB$ROLES
                	CROSS UU IN RDB$USER_PRIVILEGES
	                WITH RR.RDB$ROLE_NAME        EQ sql_role
    	            AND RR.RDB$ROLE_NAME         EQ UU.RDB$RELATION_NAME
        	        AND UU.RDB$OBJECT_TYPE       EQ obj_sql_role
            	    AND (UU.RDB$USER             EQ login_name
                	     OR UU.RDB$USER          EQ "PUBLIC")
	                AND UU.RDB$USER_TYPE         EQ obj_user
    	            AND UU.RDB$PRIVILEGE         EQ "M"*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_39, sizeof(jrd_39), true);
		gds__vtov ((const char*) login_name, (char*) jrd_40.jrd_41, 32);
		gds__vtov ((const char*) sql_role, (char*) jrd_40.jrd_42, 32);
		jrd_40.jrd_43 = obj_user;
		jrd_40.jrd_44 = obj_sql_role;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_40);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_45);
		   if (!jrd_45.jrd_47) break;

        	        if (!REQUEST (irq_verify_role_name))
            	        REQUEST (irq_verify_role_name) = request;

                	if (!/*UU.RDB$USER.NULL*/
			     jrd_45.jrd_48)
                    	found = true;

	            /*END_FOR;*/
		       }
		    }

    	        if (!REQUEST (irq_verify_role_name))
        	        REQUEST (irq_verify_role_name) = request;
			}

			if (!found && (tempId.usr_flags & USR_trole))
			{
	            jrd_req* request = CMP_find_request (tdbb, irq_verify_trusted_role, IRQ_REQUESTS);

	            /*FOR (REQUEST_HANDLE request) FIRST 1 RR IN RDB$ROLES
        	        WITH RR.RDB$ROLE_NAME  EQ sql_role
            	    AND RR.RDB$SYSTEM_FLAG NOT MISSING*/
		    {
		    if (!request)
		       request = CMP_compile2 (tdbb, (UCHAR*) jrd_33, sizeof(jrd_33), true);
		    gds__vtov ((const char*) sql_role, (char*) jrd_34.jrd_35, 32);
		    EXE_start (tdbb, request, dbb->dbb_sys_trans);
		    EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_34);
		    while (1)
		       {
		       EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_36);
		       if (!jrd_36.jrd_37) break;

            	    if (!REQUEST (irq_verify_trusted_role))
	                    REQUEST (irq_verify_trusted_role) = request;

	                if (/*RR.RDB$SYSTEM_FLAG*/
			    jrd_36.jrd_38 & ROLE_FLAG_MAY_TRUST)
	                    found = true;

	            /*END_FOR;*/
		       }
		    }

	            if (!REQUEST (irq_verify_trusted_role))
    	            REQUEST (irq_verify_trusted_role) = request;
			}

            if (!found)
			{
                role_name = NULL_ROLE;
			}
        }
    }

	if (sql_role) {
        if ((!preODS9) && (role_name != NULL_ROLE)) {
            role_name = sql_role;
        }
	}
	else {
		role_name = NULL_ROLE;
	}

	Attachment* const attachment = tdbb->getAttachment();
	MemoryPool& pool = *attachment->att_pool;
	UserId* const user = FB_NEW(pool) UserId(pool, tempId);
	user->usr_sql_role_name = role_name.c_str();
	attachment->att_user = user;

	if (!create) {
		jrd_req* handle = NULL;
		jrd_req* handle1 = NULL;
		jrd_req* handle2 = NULL;

		/*FOR(REQUEST_HANDLE handle) X IN RDB$DATABASE*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_28, sizeof(jrd_28), true);
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 0, 36, (UCHAR*) &jrd_29);
		   if (!jrd_29.jrd_31) break;

			if (!/*X.RDB$SECURITY_CLASS.NULL*/
			     jrd_29.jrd_32)
				attachment->att_security_class = SCL_get_class(tdbb, /*X.RDB$SECURITY_CLASS*/
										     jrd_29.jrd_30);
		/*END_FOR;*/
		   }
		}
		CMP_release(tdbb, handle);

		/*FOR(REQUEST_HANDLE handle1)
			FIRST 1 REL IN RDB$RELATIONS
			WITH REL.RDB$RELATION_NAME EQ "RDB$DATABASE"*/
		{
		if (!handle1)
		   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_23, sizeof(jrd_23), true);
		EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
		while (1)
		   {
		   EXE_receive (tdbb, handle1, 0, 36, (UCHAR*) &jrd_24);
		   if (!jrd_24.jrd_26) break;

            if (!/*REL.RDB$OWNER_NAME.NULL*/
		 jrd_24.jrd_27 && user->usr_user_name.hasData())
			{
				char name[129];
				*name = user->usr_user_name.length();
				user->usr_user_name.copyTo(name + 1, sizeof(name) - 1);
				if (!check_string(reinterpret_cast<const UCHAR*>(name), /*REL.RDB$OWNER_NAME*/
											jrd_24.jrd_25))
				{
					user->usr_flags |= USR_owner;
				}
			}
		/*END_FOR;*/
		   }
		}
		CMP_release(tdbb, handle1);

		if (!preODS9)
		{
			/*FOR(REQUEST_HANDLE handle2) R IN RDB$ROLES
				WITH R.RDB$ROLE_NAME EQ role_name.c_str()*/
			{
			if (!handle2)
			   handle2 = CMP_compile2 (tdbb, (UCHAR*) jrd_16, sizeof(jrd_16), true);
			gds__vtov ((const char*) role_name.c_str(), (char*) jrd_17.jrd_18, 32);
			EXE_start (tdbb, handle2, dbb->dbb_sys_trans);
			EXE_send (tdbb, handle2, 0, 32, (UCHAR*) &jrd_17);
			while (1)
			   {
			   EXE_receive (tdbb, handle2, 1, 6, (UCHAR*) &jrd_19);
			   if (!jrd_19.jrd_20) break;

				if (!/*R.RDB$SYSTEM_FLAG.NULL*/
				     jrd_19.jrd_21)
				{
					if (/*R.RDB$SYSTEM_FLAG*/
					    jrd_19.jrd_22 & ROLE_FLAG_DBO)
						user->usr_flags |= USR_dba;
				}
			/*END_FOR;*/
			   }
			}
			CMP_release(tdbb, handle2);
		}
	}
	else {
		user->usr_flags |= USR_owner;
	}

}


UserId::~UserId()
{
	if (usr_fini_sec_db)
	{
		SecurityDatabase::shutdown();
	}
}


void SCL_move_priv(SecurityClass::flags_t mask, Acl& acl)
{
/**************************************
 *
 *	S C L _ m o v e _ p r i v
 *
 **************************************
 *
 * Functional description
 *	Given a mask of privileges, move privileges types to acl.
 *
 **************************************/
	// Terminate identification criteria, and move privileges

	check_and_move(ACL_end, acl);
	check_and_move(ACL_priv_list, acl);

	for (const P_NAMES* priv = p_names; priv->p_names_priv; priv++)
	{
		if (mask & priv->p_names_priv)
		{
			fb_assert(priv->p_names_acl <= MAX_UCHAR);
			check_and_move(priv->p_names_acl, acl);
		}
	}

	check_and_move(0, acl);
}


SecurityClass* SCL_recompute_class(thread_db* tdbb, const TEXT* string)
{
/**************************************
 *
 *	S C L _ r e c o m p u t e _ c l a s s
 *
 **************************************
 *
 * Functional description
 *	Something changed with a security class, recompute it.  If we
 *	can't find it, return NULL.
 *
 **************************************/
	SET_TDBB(tdbb);

	SecurityClass* s_class = SCL_get_class(tdbb, string);
	if (!s_class) {
		return NULL;
	}

	s_class->scl_flags = compute_access(tdbb, s_class, NULL, NULL, NULL);

	if (s_class->scl_flags & SCL_exists) {
		return s_class;
	}

	// Class no long exists - get rid of it!

	SecurityClassList* list = tdbb->getAttachment()->att_security_classes;
	if (list && list->locate(string))
	{
		list->fastRemove();
		delete s_class;
	}

	return NULL;
}


void SCL_release_all(SecurityClassList*& list)
{
/**************************************
 *
 *	S C L _ r e l e a s e _ a l l
 *
 **************************************
 *
 * Functional description
 *	Release all security classes.
 *
 **************************************/
	if (!list)
		return;

	if (list->getFirst())
	{
		do {
			delete list->current();
		} while (list->getNext());
	}

	delete list;
	list = NULL;
}


static bool check_hex(const UCHAR* acl, USHORT number)
{
/**************************************
 *
 *	c h e c k _ h e x
 *
 **************************************
 *
 * Functional description
 *	Check a string against and acl numeric string.  If they don't match,
 *	return true.
 *
 **************************************/
	int n = 0;
	USHORT l = *acl++;
	if (l)
	{
		do {
			const TEXT c = *acl++;
			n *= 10;
			if (c >= '0' && c <= '9') {
				n += c - '0';
			}
			else if (c >= 'a' && c <= 'f') {
				n += c - 'a' + 10;
			}
			else if (c >= 'A' && c <= 'F') {
				n += c - 'A' + 10;
			}
		} while (--l);
	}

	return (n != number);
}


static bool check_number(const UCHAR* acl, USHORT number)
{
/**************************************
 *
 *	c h e c k _ n u m b e r
 *
 **************************************
 *
 * Functional description
 *	Check a string against and acl numeric string.  If they don't match,
 *	return true.
 *
 **************************************/
	int n = 0;
	USHORT l = *acl++;
	if (l)
	{
		do {
			n = n * UIC_BASE + *acl++ - '0';
		} while (--l);
	}

	return (n != number);
}


static bool check_user_group(thread_db* tdbb, const UCHAR* acl, USHORT number)
{
/**************************************
 *
 *	c h e c k _ u s e r _ g r o u p
 *
 **************************************
 *
 * Functional description
 *
 *	Check a string against an acl numeric string.
 *
 * logic:
 *
 *  If the string contains user group name,
 *    then
 *      converts user group name to numeric user group id.
 *    else
 *      converts character user group id to numeric user group id.
 *
 *	Check numeric user group id against an acl numeric string.
 *  If they don't match, return true.
 *
 **************************************/
	SET_TDBB(tdbb);
	//Database* dbb = tdbb->getDatabase();

	SLONG n = 0;

	USHORT l = *acl++;
	if (l)
	{
		if (isdigit(*acl))	// this is a group id
		{
			do {
				n = n * UIC_BASE + *acl++ - '0';
			} while (--l);
		}
		else				// processing group name
		{
			Firebird::string user_group_name;
			do {
				const TEXT one_char = *acl++;
				user_group_name += LOWWER(one_char);
			} while (--l);

			// convert unix group name to unix group id
			n = os_utils::get_user_group_id(user_group_name.c_str());
		}
	}

	return (n != number);
}


static bool check_string(const UCHAR* acl, const Firebird::MetaName& string)
{
/**************************************
 *
 *	c h e c k _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Check a string against and acl string.  If they don't match,
 *	return true.
 *
 **************************************/
	fb_assert(acl);

	const size_t length = *acl++;
	const TEXT* const ptr = (TEXT*) acl;

    return (string.compare(ptr, length) != 0);
}


static SecurityClass::flags_t compute_access(thread_db* tdbb,
											 const SecurityClass* s_class,
											 const jrd_rel* view,
											 const Firebird::MetaName& trg_name,
											 const Firebird::MetaName& prc_name)
{
   struct {
          bid  jrd_14;	// RDB$ACL 
          SSHORT jrd_15;	// gds__utility 
   } jrd_13;
   struct {
          TEXT  jrd_12 [32];	// RDB$SECURITY_CLASS 
   } jrd_11;
/**************************************
 *
 *	c o m p u t e _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *	Compute access for security class.  If a relation block is
 *	present, it is a view, and we should check for enhanced view
 *	access permissions.  Return a flag word of recognized privileges.
 *
 **************************************/
	Acl acl;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	SecurityClass::flags_t privileges = SCL_scanned;

	jrd_req* request = CMP_find_request(tdbb, irq_l_security, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request) X IN RDB$SECURITY_CLASSES
		WITH X.RDB$SECURITY_CLASS EQ s_class->scl_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_10, sizeof(jrd_10), true);
	gds__vtov ((const char*) s_class->scl_name.c_str(), (char*) jrd_11.jrd_12, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_11);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 10, (UCHAR*) &jrd_13);
	   if (!jrd_13.jrd_15) break;

        if (!REQUEST(irq_l_security))
			REQUEST(irq_l_security) = request;

		privileges |= SCL_exists;
		blb* blob = BLB_open(tdbb, dbb->dbb_sys_trans, &/*X.RDB$ACL*/
								jrd_13.jrd_14);
		UCHAR* buffer = acl.getBuffer(ACL_BLOB_BUFFER_SIZE);
		UCHAR* end = buffer;
		while (true)
		{
			end += BLB_get_segment(tdbb, blob, end, (USHORT) (acl.getCount() - (end - buffer)) );
			if (blob->blb_flags & BLB_eof)
				break;

			// There was not enough space, realloc point acl to the correct location

			if (blob->blb_fragment_size)
			{
				const ptrdiff_t old_offset = end - buffer;
				buffer = acl.getBuffer(acl.getCount() + ACL_BLOB_BUFFER_SIZE);
				end = buffer + old_offset;
			}
		}
		BLB_close(tdbb, blob);
		blob = NULL;
		acl.shrink(end - buffer);

		if (acl.getCount() > 0)
		{
			privileges |= walk_acl(tdbb, acl, view, trg_name, prc_name);
		}
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_security))
		REQUEST(irq_l_security) = request;

	return privileges;
}


static SecurityClass::flags_t walk_acl(thread_db* tdbb,
									   const Acl& acl,
									   const jrd_rel* view,
									   const Firebird::MetaName& trg_name,
									   const Firebird::MetaName& prc_name)
{
   struct {
          TEXT  jrd_7 [32];	// RDB$USER 
          SSHORT jrd_8;	// gds__utility 
          SSHORT jrd_9;	// gds__null_flag 
   } jrd_6;
   struct {
          TEXT  jrd_2 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_3 [32];	// RDB$USER 
          SSHORT jrd_4;	// RDB$OBJECT_TYPE 
          SSHORT jrd_5;	// RDB$USER_TYPE 
   } jrd_1;
/**************************************
 *
 *	w a l k _ a c l
 *
 **************************************
 *
 * Functional description
 *	Walk an access control list looking for a hit.  If a hit
 *	is found, return privileges.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// Munch ACL. If we find a hit, eat up privileges.

	UserId user = *tdbb->getAttachment()->att_user;
	const TEXT* role_name = user.usr_sql_role_name.nullStr();

	if (view && (view->rel_flags & REL_sql_relation))
	{
		// Use the owner of the view to perform the sql security
		// checks with: (1) The view user must have sufficient privileges
		// to the view, and (2a) the view owner must have sufficient
		// privileges to the base table or (2b) the view must have
		// sufficient privileges on the base table.

		user.usr_user_name = view->rel_owner_name.c_str();
	}

	SecurityClass::flags_t privilege = 0;
	const UCHAR* a = acl.begin();

	if (*a++ != ACL_version)
	{
		BUGCHECK(160);	// msg 160 wrong ACL version
	}

	if (user.locksmith())
	{
		return -1 & ~SCL_corrupt;
	}

	const TEXT* p;
	bool hit = false;
	UCHAR c;

	while ( (c = *a++) )
	{
		switch (c)
		{
		case ACL_id_list:
			hit = true;
			while ( (c = *a++) )
			{
				switch (c)
				{
				case id_person:
					if (!(p = user.usr_user_name.nullStr()) || check_string(a, p))
						hit = false;
					break;

				case id_project:
					if (!(p = user.usr_project_name.nullStr()) || check_string(a, p))
						hit = false;
					break;

				case id_organization:
					if (!(p = user.usr_org_name.nullStr()) || check_string(a, p))
						hit = false;
					break;

				case id_group:
					if (check_user_group(tdbb, a, user.usr_group_id))
					{
						hit = false;
					}
					break;

				case id_sql_role:
					if (!role_name || check_string(a, role_name))
						hit = false;
					else
					{
						TEXT login_name[129];
						TEXT* pln = login_name;
						const TEXT* q = user.usr_user_name.c_str();
						while (*pln++ = UPPER7(*q)) {
							++q;
						}
						hit = false;
						jrd_req* request = CMP_find_request(tdbb, irq_get_role_mem, IRQ_REQUESTS);

						/*FOR(REQUEST_HANDLE request) U IN RDB$USER_PRIVILEGES WITH
							(U.RDB$USER EQ login_name OR
							 U.RDB$USER EQ "PUBLIC") AND
								U.RDB$USER_TYPE EQ obj_user AND
								U.RDB$RELATION_NAME EQ user.usr_sql_role_name.c_str() AND
								U.RDB$OBJECT_TYPE EQ obj_sql_role AND
								U.RDB$PRIVILEGE EQ "M"*/
						{
						if (!request)
						   request = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
						gds__vtov ((const char*) user.usr_sql_role_name.c_str(), (char*) jrd_1.jrd_2, 32);
						gds__vtov ((const char*) login_name, (char*) jrd_1.jrd_3, 32);
						jrd_1.jrd_4 = obj_sql_role;
						jrd_1.jrd_5 = obj_user;
						EXE_start (tdbb, request, dbb->dbb_sys_trans);
						EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_1);
						while (1)
						   {
						   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_6);
						   if (!jrd_6.jrd_8) break;

                            if (!REQUEST(irq_get_role_mem))
                                REQUEST(irq_get_role_mem) = request;

							if (!/*U.RDB$USER.NULL*/
							     jrd_6.jrd_9)
								hit = true;
						/*END_FOR;*/
						   }
						}

						if (!REQUEST(irq_get_role_mem))
							REQUEST(irq_get_role_mem) = request;
					}
					break;

				case id_view:
					if (!view || check_string(a, view->rel_name))
						hit = false;
					break;

				case id_procedure:
					if (check_string(a, prc_name))
						hit = false;
					break;

				case id_trigger:
					if (check_string(a, trg_name))
					{
						hit = false;
					}
					break;

				case id_views:
					// Disable this catch-all that messes up the view security.
					// Note that this id_views is not generated anymore, this code
					// is only here for compatibility. id_views was only
					// generated for SQL.

					hit = false;
					if (!view) {
						hit = false;
					}
					break;

				case id_user:
					if (check_number(a, user.usr_user_id)) {
						hit = false;
					}
					break;

				case id_node:
					if (check_hex(a, user.usr_node_id)) {
						hit = false;
					}
					break;

				default:
					return SCL_corrupt;
				}
				a += *a + 1;
			}
			break;

		case ACL_priv_list:
			if (hit)
			{
				while ( (c = *a++) )
				{
					switch (c)
					{
					case priv_control:
						privilege |= SCL_control;
						break;

					case priv_read:
						// Note that READ access must imply REFERENCES
						// access for upward compatibility of existing
						// security classes
						privilege |= SCL_read | SCL_sql_references;
						break;

					case priv_write:
						privilege |=
							SCL_write | SCL_sql_insert | SCL_sql_update |
							SCL_sql_delete;
						break;

					case priv_sql_insert:
						privilege |= SCL_sql_insert;
						break;

					case priv_sql_delete:
						privilege |= SCL_sql_delete;
						break;

					case priv_sql_references:
						privilege |= SCL_sql_references;
						break;

					case priv_sql_update:
						privilege |= SCL_sql_update;
						break;

					case priv_delete:
						privilege |= SCL_delete;
						break;

					case priv_grant:
						privilege |= SCL_grant;
						break;

					case priv_protect:
						privilege |= SCL_protect;
						break;

					case priv_execute:
						privilege |= SCL_execute;
						break;

					default:
						return SCL_corrupt;
					}
				}

				// For a relation the first hit does not give the privilege.
				// Because, there could be some permissions for the table
				// (for user1) and some permissions for a column on that
				// table for public/user2, causing two hits.
				// Hence, we do not return at this point.
				// -- Madhukar Thakur (May 1, 1995)
			}
			else
				while (*a++);
			break;

		default:
			return SCL_corrupt;
		}
	}

	fb_assert(a == acl.end());

	return privilege;
}

