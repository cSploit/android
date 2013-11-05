/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *  PROGRAM:    Dynamic SQL runtime support
 *  MODULE:     metd.epp
 *  DESCRIPTION:    Meta-data interface
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
 * 2001.11.28 Claudio Valderrama: load not only udfs but udf arguments;
 *   handle possible collisions with udf redefinitions (drop->declare).
 *   This closes SF Bug# 409769.
 * 2001.12.06 Claudio Valderrama: METD_get_charset_bpc() was added to
 *    get only the bytes per char of a field, given its charset id.
 *   This request is not cached.
 * 2001.02.23 Claudio Valderrama: Fix SF Bug #228135 with views spoiling
 *    NULLs in outer joins.
 * 2004.01.16 Vlad Horsun: make METD_get_col_default and
 *   METD_get_domain_default return actual length of default BLR
 * 2004.01.16 Vlad Horsun: added support for default parameters
 */

#include "firebird.h"
#include <string.h>
#include "../dsql/dsql.h"
#include "../jrd/ibase.h"
#include "../jrd/align.h"
#include "../jrd/intl.h"
#include "../jrd/irq.h"
#include "../jrd/tra.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/metd_proto.h"
#include "../dsql/hsh_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/errd_proto.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/why_proto.h"
#include "../common/utils_proto.h"
#include "../common/classes/init.h"

using namespace Jrd;
using namespace Firebird;

// NOTE: The static definition of DB and gds_trans by gpre will not
// be used by the meta data routines.  Each of those routines has
// its own local definition of these variables.

/*DATABASE DB = STATIC "yachts.lnk";*/
static const UCHAR	jrd_0 [93] =
   {	// blr string 

4,2,4,1,3,0,40,32,0,40,0,1,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',1,
'K',7,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,1,0,25,1,
0,0,1,24,0,3,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,255,14,1,1,
21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_7 [101] =
   {	// blr string 

4,2,4,1,5,0,40,32,0,40,32,0,7,0,7,0,7,0,4,0,1,0,40,32,0,12,0,
2,7,'C',1,'K',5,0,0,'G',47,24,0,1,0,25,0,0,0,255,14,1,2,1,24,
0,0,0,41,1,0,0,3,0,1,24,0,4,0,41,1,1,0,4,0,1,21,8,0,1,0,0,0,25,
1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_16 [116] =
   {	// blr string 

4,2,4,1,5,0,40,32,0,40,32,0,40,0,1,7,0,7,0,4,0,1,0,40,32,0,12,
0,2,7,'C',1,'K',7,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,
0,0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,1,24,0,3,0,25,1,2,0,1,21,8,
0,1,0,0,0,25,1,3,0,1,24,0,2,0,25,1,4,0,255,14,1,1,21,8,0,0,0,
0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_25 [93] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,2,0,40,32,0,40,32,0,12,0,2,7,'C',1,'K',
11,0,0,'G',58,47,24,0,0,0,25,0,1,0,47,24,0,2,0,25,0,0,0,255,14,
1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,255,14,1,1,
21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_32 [96] =
   {	// blr string 

4,2,4,1,4,0,40,32,0,7,0,7,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',
1,'K',12,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,1,0,41,
1,0,0,2,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,3,0,25,1,3,0,255,14,
1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_40 [303] =
   {	// blr string 

4,2,4,1,24,0,9,0,40,32,0,40,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,1,
0,40,32,0,12,0,2,7,'C',2,'K',2,0,0,'K',5,0,1,'G',58,47,24,0,0,
0,24,1,2,0,47,24,1,1,0,25,0,0,0,'F',1,'H',24,1,6,0,255,14,1,2,
1,24,0,4,0,41,1,0,0,18,0,1,24,1,2,0,25,1,1,0,1,24,1,0,0,25,1,
2,0,1,21,8,0,1,0,0,0,25,1,3,0,1,24,0,15,0,25,1,4,0,1,24,1,13,
0,25,1,5,0,1,24,0,23,0,25,1,6,0,1,24,1,16,0,25,1,7,0,1,24,0,25,
0,41,1,9,0,8,0,1,24,1,18,0,41,1,11,0,10,0,1,24,0,26,0,41,1,13,
0,12,0,1,24,0,22,0,41,1,15,0,14,0,1,24,0,17,0,25,1,16,0,1,24,
0,10,0,25,1,17,0,1,24,0,11,0,25,1,19,0,1,24,0,9,0,25,1,20,0,1,
24,0,8,0,25,1,21,0,1,24,1,9,0,41,1,23,0,22,0,255,14,1,1,21,8,
0,0,0,0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_68 [140] =
   {	// blr string 

4,2,4,1,9,0,41,0,0,0,1,9,0,40,32,0,7,0,7,0,7,0,7,0,7,0,7,0,4,
0,1,0,40,32,0,12,0,2,7,'C',1,'K',6,0,0,'G',47,24,0,8,0,25,0,0,
0,255,14,1,2,1,24,0,10,0,41,1,0,0,4,0,1,24,0,0,0,41,1,1,0,5,0,
1,24,0,13,0,25,1,2,0,1,21,8,0,1,0,0,0,25,1,3,0,1,24,0,5,0,25,
1,6,0,1,24,0,3,0,41,1,8,0,7,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_81 [95] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',2,'K',6,0,0,'K',
5,0,1,'G',58,47,24,1,1,0,24,0,8,0,58,47,24,0,8,0,25,0,0,0,57,
61,24,0,3,0,61,24,1,9,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_86 [100] =
   {	// blr string 

4,2,4,1,2,0,9,0,7,0,4,0,2,0,40,32,0,40,32,0,12,0,2,7,'C',1,'K',
27,0,0,'G',58,47,24,0,1,0,25,0,1,0,58,47,24,0,0,0,25,0,0,0,59,
61,24,0,5,0,255,14,1,2,1,24,0,5,0,25,1,0,0,1,21,8,0,1,0,0,0,25,
1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_93 [142] =
   {	// blr string 

4,2,4,1,9,0,9,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,2,0,40,32,
0,40,32,0,12,0,2,7,'C',1,'K',27,0,0,'G',58,47,24,0,1,0,25,0,1,
0,47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,7,0,41,1,0,0,6,0,1,21,
8,0,1,0,0,0,25,1,1,0,1,24,0,11,0,41,1,3,0,2,0,1,24,0,10,0,41,
1,5,0,4,0,1,24,0,9,0,41,1,8,0,7,0,255,14,1,1,21,8,0,0,0,0,0,25,
1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_107 [304] =
   {	// blr string 

4,2,4,1,22,0,9,0,40,32,0,40,32,0,40,32,0,40,32,0,40,32,0,40,32,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
4,0,2,0,40,32,0,7,0,12,0,2,7,'C',2,'K',27,0,0,'K',2,0,1,'G',58,
47,24,1,0,0,24,0,4,0,58,47,24,0,1,0,25,0,0,0,47,24,0,3,0,25,0,
1,0,'F',1,'I',24,0,2,0,255,14,1,2,1,24,1,6,0,41,1,0,0,8,0,1,24,
1,0,0,25,1,1,0,1,24,0,13,0,41,1,2,0,9,0,1,24,0,12,0,41,1,3,0,
10,0,1,24,0,4,0,25,1,4,0,1,24,0,0,0,25,1,5,0,1,24,0,1,0,25,1,
6,0,1,21,8,0,1,0,0,0,25,1,7,0,1,24,1,17,0,25,1,11,0,1,24,1,23,
0,25,1,12,0,1,24,1,10,0,25,1,13,0,1,24,1,25,0,41,1,15,0,14,0,
1,24,1,26,0,41,1,17,0,16,0,1,24,1,11,0,25,1,18,0,1,24,1,9,0,25,
1,19,0,1,24,1,8,0,25,1,20,0,1,24,0,2,0,25,1,21,0,255,14,1,1,21,
8,0,0,0,0,0,25,1,7,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_134 [92] =
   {	// blr string 

4,2,4,1,3,0,40,32,0,7,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',1,'K',
26,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,8,0,25,1,0,
0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,1,0,25,1,2,0,255,14,1,1,21,
8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_141 [137] =
   {	// blr string 

4,2,4,1,2,0,40,32,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',3,'K',4,
0,0,'K',3,0,1,'K',22,0,2,'G',58,58,47,24,1,0,0,24,0,0,0,47,24,
2,5,0,24,1,0,0,58,47,24,2,2,0,25,0,0,0,47,24,2,1,0,21,14,11,0,
'P','R','I','M','A','R','Y',32,'K','E','Y','F',1,'H',24,1,2,0,
255,14,1,2,1,24,1,1,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_147 [161] =
   {	// blr string 

4,2,4,1,10,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,
40,32,0,12,0,2,7,'C',1,'K',15,0,0,'G',47,24,0,0,0,25,0,0,0,'F',
1,'H',24,0,1,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,2,
0,25,1,1,0,1,24,0,7,0,41,1,3,0,2,0,1,24,0,5,0,25,1,4,0,1,24,0,
6,0,41,1,6,0,5,0,1,24,0,4,0,25,1,7,0,1,24,0,3,0,25,1,8,0,1,24,
0,1,0,25,1,9,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_161 [80] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',1,'K',14,0,0,
'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
1,24,0,6,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_167 [69] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',1,'K',30,0,0,'G',
47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_172 [84] =
   {	// blr string 

4,2,4,1,3,0,9,0,7,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',1,'K',2,
0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,6,0,41,1,0,0,2,
0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,
0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_179 [210] =
   {	// blr string 

4,2,4,1,17,0,9,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',1,'K',2,0,0,
'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,4,0,41,1,0,0,5,0,1,
21,8,0,1,0,0,0,25,1,1,0,1,24,0,17,0,25,1,2,0,1,24,0,10,0,25,1,
3,0,1,24,0,15,0,25,1,4,0,1,24,0,24,0,41,1,7,0,6,0,1,24,0,25,0,
41,1,9,0,8,0,1,24,0,26,0,41,1,11,0,10,0,1,24,0,22,0,41,1,13,0,
12,0,1,24,0,11,0,25,1,14,0,1,24,0,9,0,25,1,15,0,1,24,0,8,0,25,
1,16,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_200 [77] =
   {	// blr string 

4,2,4,0,2,0,40,32,0,7,0,2,7,'C',1,'K',1,0,0,'D',21,8,0,1,0,0,
0,'G',59,61,24,0,3,0,255,14,0,2,1,24,0,3,0,25,0,0,0,1,21,8,0,
1,0,0,0,25,0,1,0,255,14,0,1,21,8,0,0,0,0,0,25,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_204 [80] =
   {	// blr string 

4,2,4,1,2,0,40,32,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',28,0,0,
'G',47,24,0,4,0,25,0,0,0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,21,
8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_210 [176] =
   {	// blr string 

4,2,4,1,5,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,40,32,0,12,0,2,7,'C',
3,'K',29,0,0,'K',28,0,1,'K',11,0,2,'G',58,47,24,1,4,0,24,0,2,
0,58,47,24,2,1,0,24,1,4,0,58,47,24,2,2,0,25,0,0,0,58,47,24,2,
0,0,21,14,22,0,'R','D','B','$','C','H','A','R','A','C','T','E',
'R','_','S','E','T','_','N','A','M','E',47,24,1,3,0,24,0,0,0,
255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,1,8,0,41,1,2,0,1,0,
1,24,0,1,0,25,1,3,0,1,24,0,2,0,25,1,4,0,255,14,1,1,21,8,0,0,0,
0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_219 [126] =
   {	// blr string 

4,2,4,1,5,0,9,0,9,0,7,0,7,0,7,0,4,0,2,0,40,32,0,40,32,0,12,0,
2,7,'C',2,'K',5,0,0,'K',2,0,1,'G',58,47,24,0,1,0,25,0,1,0,58,
47,24,0,2,0,24,1,0,0,47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,1,6,
0,41,1,0,0,3,0,1,24,0,12,0,41,1,1,0,4,0,1,21,8,0,1,0,0,0,25,1,
2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_229 [132] =
   {	// blr string 

4,2,4,1,5,0,7,0,7,0,7,0,7,0,7,0,4,0,2,0,40,32,0,7,0,12,0,2,7,
'C',2,'K',29,0,0,'K',28,0,1,'G',58,47,24,1,4,0,24,0,2,0,58,47,
24,0,0,0,25,0,0,0,47,24,0,2,0,25,0,1,0,255,14,1,2,1,21,8,0,1,
0,0,0,25,1,0,0,1,24,1,8,0,41,1,2,0,1,0,1,24,0,1,0,25,1,3,0,1,
24,0,2,0,25,1,4,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 


static const UCHAR blr_bpb[] =
{
	isc_bpb_version1,
	isc_bpb_source_type, 1, isc_blob_blr,
	isc_bpb_target_type, 1, isc_blob_blr
};

static void convert_dtype(dsql_fld*, SSHORT);
static void free_procedure(dsql_prc*);
static void free_relation(dsql_rel*);
static void insert_symbol(dsql_sym*);
static dsql_sym* lookup_symbol(dsql_dbb*, USHORT, const char*, const SYM_TYPE, USHORT = 0);
static dsql_sym* lookup_symbol(dsql_dbb*, const dsql_str*, SYM_TYPE, USHORT = 0);

#define DSQL_REQUEST(id) dbb->dbb_database->dbb_internal[id]

namespace {
	class MutexHolder : public Database::CheckoutLockGuard
	{
	public:
		explicit MutexHolder(dsql_req* request)
			: CheckoutLockGuard(request->req_dbb->dbb_database, request->req_dbb->dbb_cache_mutex)
		{}

	private:
		// copying is prohibited
		MutexHolder(const MutexHolder&);
		MutexHolder& operator=(const MutexHolder&);
	};

	inline void validateTransaction(const dsql_req* request)
	{
		if (!request->req_transaction->checkHandle())
		{
			ERR_post(Arg::Gds(isc_bad_trans_handle));
		}
	}
}


void METD_drop_charset(dsql_req* request, const MetaName& name)
{
/**************************************
 *
 *  M E T D _ d r o p _ c h a r s e t
 *
 **************************************
 *
 * Functional description
 *  Drop a character set from our metadata, and the next caller who wants it will
 *  look up the new version.
 *  Dropping will be achieved by marking the character set
 *  as dropped.  Anyone with current access can continue
 *  accessing it.
 *
 **************************************/
	MutexHolder holder(request);

	// If the symbol wasn't defined, we've got nothing to do
	dsql_sym* symbol = lookup_symbol(request->req_dbb, name.length(), name.c_str(),
									 SYM_intlsym_charset, 0);

	if (symbol)
	{
		dsql_intlsym* intlsym = (dsql_intlsym*) symbol->sym_object;
		intlsym->intlsym_flags |= INTLSYM_dropped;
	}

	// mark other potential candidates as maybe dropped
	HSHD_set_flag(request->req_dbb, name.c_str(), name.length(), SYM_intlsym_charset, INTLSYM_dropped);
}


void METD_drop_collation(dsql_req* request, const dsql_str* name)
{
/**************************************
 *
 *  M E T D _ d r o p _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Drop a collation from our metadata, and
 *  the next caller who wants it will
 *  look up the new version.
 *
 *  Dropping will be achieved by marking the collation
 *  as dropped.  Anyone with current access can continue
 *  accessing it.
 *
 **************************************/
	MutexHolder holder(request);

	// If the symbol wasn't defined, we've got nothing to do
	dsql_sym* symbol = lookup_symbol(request->req_dbb, name, SYM_intlsym_collation, 0);

	if (symbol)
	{
		dsql_intlsym* intlsym = (dsql_intlsym*) symbol->sym_object;
		intlsym->intlsym_flags |= INTLSYM_dropped;
	}

	// mark other potential candidates as maybe dropped
	HSHD_set_flag(request->req_dbb, name->str_data, name->str_length,
				  SYM_intlsym_collation, INTLSYM_dropped);
}


void METD_drop_function(dsql_req* request, const dsql_str* name)
{
/**************************************
 *
 *  M E T D _ d r o p _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *  Drop a user defined function from our metadata, and
 *  the next caller who wants it will
 *  look up the new version.
 *
 *  Dropping will be achieved by marking the function
 *  as dropped.  Anyone with current access can continue
 *  accessing it.
 *
 **************************************/
	MutexHolder holder(request);

	// If the symbol wasn't defined, we've got nothing to do

	dsql_sym* symbol = lookup_symbol(request->req_dbb, name, SYM_udf);

	if (symbol) {
		dsql_udf* userFunc = (dsql_udf*) symbol->sym_object;
		userFunc->udf_flags |= UDF_dropped;
	}

	// mark other potential candidates as maybe dropped

	HSHD_set_flag(request->req_dbb, name->str_data, name->str_length, SYM_udf, UDF_dropped);

}


void METD_drop_procedure(dsql_req* request, const dsql_str* name)
{
/**************************************
 *
 *  M E T D _ d r o p _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *  Drop a procedure from our metadata, and
 *  the next caller who wants it will
 *  look up the new version.
 *
 *  Dropping will be achieved by marking the procedure
 *  as dropped.  Anyone with current access can continue
 *  accessing it.
 *
 **************************************/
	MutexHolder holder(request);

	// If the symbol wasn't defined, we've got nothing to do

	dsql_sym* symbol = lookup_symbol(request->req_dbb, name, SYM_procedure);

	if (symbol) {
		dsql_prc* procedure = (dsql_prc*) symbol->sym_object;
		procedure->prc_flags |= PRC_dropped;
	}

	// mark other potential candidates as maybe dropped

	HSHD_set_flag(request->req_dbb, name->str_data, name->str_length, SYM_procedure, PRC_dropped);

}


void METD_drop_relation(dsql_req* request, const dsql_str* name)
{
/**************************************
 *
 *  M E T D _ d r o p _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Drop a relation from our metadata, and
 *  rely on the next guy who wants it to
 *  look up the new version.
 *
 *      Dropping will be achieved by marking the relation
 *      as dropped.  Anyone with current access can continue
 *      accessing it.
 *
 **************************************/
	MutexHolder holder(request);

	// If the symbol wasn't defined, we've got nothing to do

	dsql_sym* symbol = lookup_symbol(request->req_dbb, name, SYM_relation);

	if (symbol) {
		dsql_rel* relation = (dsql_rel*) symbol->sym_object;
		relation->rel_flags |= REL_dropped;
	}

	// mark other potential candidates as maybe dropped

	HSHD_set_flag(request->req_dbb, name->str_data, name->str_length, SYM_relation, REL_dropped);

}


dsql_intlsym* METD_get_collation(dsql_req* request,
								 const dsql_str* name,
								 USHORT charset_id)
{
   struct {
          SSHORT jrd_234;	// isc_utility 
          SSHORT jrd_235;	// gds__null_flag 
          SSHORT jrd_236;	// RDB$BYTES_PER_CHARACTER 
          SSHORT jrd_237;	// RDB$COLLATION_ID 
          SSHORT jrd_238;	// RDB$CHARACTER_SET_ID 
   } jrd_233;
   struct {
          TEXT  jrd_231 [32];	// RDB$COLLATION_NAME 
          SSHORT jrd_232;	// RDB$CHARACTER_SET_ID 
   } jrd_230;
/**************************************
 *
 *  M E T D _ g e t _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Look up an international text type object.
 *  If it doesn't exist, return NULL.
 *
 **************************************/
	MutexHolder holder(request);

	thread_db* tdbb = JRD_get_thread_data();

	// Start by seeing if symbol is already defined

	dsql_sym* symbol = lookup_symbol(request->req_dbb, name, SYM_intlsym_collation, charset_id);
	if (symbol)
		return (dsql_intlsym*) symbol->sym_object;

	// Now see if it is in the database

	validateTransaction(request);

	dsql_dbb* dbb = request->req_dbb;
	dsql_intlsym* iname = NULL;

	jrd_req* handle = CMP_find_request(tdbb, irq_collation, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
	X IN RDB$COLLATIONS
		CROSS Y IN RDB$CHARACTER_SETS OVER RDB$CHARACTER_SET_ID
		WITH X.RDB$COLLATION_NAME EQ name->str_data AND
			 X.RDB$CHARACTER_SET_ID EQ charset_id*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_229, sizeof(jrd_229), true);
	gds__vtov ((const char*) name->str_data, (char*) jrd_230.jrd_231, 32);
	jrd_230.jrd_232 = charset_id;
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 34, (UCHAR*) &jrd_230);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_233);
	   if (!jrd_233.jrd_234) break;;

		if (!DSQL_REQUEST(irq_collation))
			DSQL_REQUEST(irq_collation) = handle;

		iname = FB_NEW_RPT(dbb->dbb_pool, name->str_length) dsql_intlsym;
		strcpy(iname->intlsym_name, name->str_data);
		iname->intlsym_flags = 0;
		iname->intlsym_charset_id = /*X.RDB$CHARACTER_SET_ID*/
					    jrd_233.jrd_238;
		iname->intlsym_collate_id = /*X.RDB$COLLATION_ID*/
					    jrd_233.jrd_237;
		iname->intlsym_ttype =
			INTL_CS_COLL_TO_TTYPE(iname->intlsym_charset_id, iname->intlsym_collate_id);
		iname->intlsym_bytes_per_char =
			(/*Y.RDB$BYTES_PER_CHARACTER.NULL*/
			 jrd_233.jrd_235) ? 1 : (/*Y.RDB$BYTES_PER_CHARACTER*/
	 jrd_233.jrd_236);

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_collation))
		DSQL_REQUEST(irq_collation) = handle;

	if (!iname)
		return NULL;

	// Store in the symbol table

	symbol = iname->intlsym_symbol = FB_NEW_RPT(dbb->dbb_pool, 0) dsql_sym;
	symbol->sym_object = iname;
	symbol->sym_string = iname->intlsym_name;
	symbol->sym_length = name->str_length;
	symbol->sym_type = SYM_intlsym_collation;
	symbol->sym_dbb = dbb;
	insert_symbol(symbol);

	return iname;
}


USHORT METD_get_col_default(dsql_req* request,
							const char* for_rel_name,
							const char* for_col_name,
							bool* has_default,
							UCHAR* buffer,
							USHORT buff_length)
{
   struct {
          bid  jrd_224;	// RDB$DEFAULT_VALUE 
          bid  jrd_225;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_226;	// isc_utility 
          SSHORT jrd_227;	// gds__null_flag 
          SSHORT jrd_228;	// gds__null_flag 
   } jrd_223;
   struct {
          TEXT  jrd_221 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_222 [32];	// RDB$RELATION_NAME 
   } jrd_220;
/*************************************************************
 *
 *  M E T D _ g e t _ c o l _ d e f a u l t
 *
 **************************************************************
 *
 * Function:
 *    Gets the default value for a column of an existing table.
 *    Will check the default for the column of the table, if that is
 *    not present, will check for the default of the relevant domain
 *
 *    The default blr is returned in buffer. The blr is of the form
 *    blr_version4 blr_literal ..... blr_eoc
 *
 *    Reads the system tables RDB$FIELDS and RDB$RELATION_FIELDS.
 *
 **************************************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(request);

	dsql_dbb* dbb = request->req_dbb;

	bid* blob_id;

	USHORT result = 0;
	blb* blob_handle = 0;

	*has_default = false;

	jrd_req* handle = CMP_find_request(tdbb, irq_col_default, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
		RFL IN RDB$RELATION_FIELDS CROSS
		FLD IN RDB$FIELDS WITH
		RFL.RDB$RELATION_NAME EQ for_rel_name AND
		RFL.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME AND
		RFL.RDB$FIELD_NAME EQ for_col_name*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_219, sizeof(jrd_219), true);
	gds__vtov ((const char*) for_col_name, (char*) jrd_220.jrd_221, 32);
	gds__vtov ((const char*) for_rel_name, (char*) jrd_220.jrd_222, 32);
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_220);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 22, (UCHAR*) &jrd_223);
	   if (!jrd_223.jrd_226) break;

		if (!DSQL_REQUEST(irq_col_default))
			DSQL_REQUEST(irq_col_default) = handle;

		if (!/*RFL.RDB$DEFAULT_VALUE.NULL*/
		     jrd_223.jrd_228) {
			blob_id = &/*RFL.RDB$DEFAULT_VALUE*/
				   jrd_223.jrd_225;
			*has_default = true;
		}
		else if (!/*FLD.RDB$DEFAULT_VALUE.NULL*/
			  jrd_223.jrd_227) {
			blob_id = &/*FLD.RDB$DEFAULT_VALUE*/
				   jrd_223.jrd_224;
			*has_default = true;
		}
		else
			*has_default = false;

		if (*has_default)
		{
			blob_handle = BLB_open2(tdbb, request->req_transaction, blob_id,
									sizeof(blr_bpb), blr_bpb, true);

			// fetch segments. Assuming here that the buffer is big enough.
			UCHAR* ptr_in_buffer = buffer;
			while (true)
			{
				const USHORT length = BLB_get_segment(tdbb, blob_handle, ptr_in_buffer, buff_length);

				ptr_in_buffer += length;
				buff_length -= length;
				result += length;

				if (blob_handle->blb_flags & BLB_eof)
				{
					// null terminate the buffer
					*ptr_in_buffer = 0;
					break;
				}
				if (blob_handle->blb_fragment_size)
					status_exception::raise(Arg::Gds(isc_segment));
				else
					continue;
			}

			try
			{
				ThreadStatusGuard status_vector(tdbb);

				BLB_close(tdbb, blob_handle);
				blob_handle = NULL;
			}
			catch (Exception&)
			{
			}

			// the default string must be of the form:
			// blr_version4 blr_literal ..... blr_eoc
			fb_assert((buffer[0] == blr_version4) || (buffer[0] == blr_version5));
			fb_assert(buffer[1] == blr_literal);
		}
		else {
			if (request->req_dbb->dbb_db_SQL_dialect > SQL_DIALECT_V5)
				buffer[0] = blr_version5;
			else
				buffer[0] = blr_version4;
			buffer[1] = blr_eoc;
			result = 2;
		}

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_col_default))
		DSQL_REQUEST(irq_col_default) = handle;

	return result;
}


dsql_intlsym* METD_get_charset(dsql_req* request, USHORT length, const char* name) // UTF-8
{
   struct {
          SSHORT jrd_214;	// isc_utility 
          SSHORT jrd_215;	// gds__null_flag 
          SSHORT jrd_216;	// RDB$BYTES_PER_CHARACTER 
          SSHORT jrd_217;	// RDB$COLLATION_ID 
          SSHORT jrd_218;	// RDB$CHARACTER_SET_ID 
   } jrd_213;
   struct {
          TEXT  jrd_212 [32];	// RDB$TYPE_NAME 
   } jrd_211;
/**************************************
 *
 *  M E T D _ g e t _ c h a r s e t
 *
 **************************************
 *
 * Functional description
 *  Look up an international text type object.
 *  If it doesn't exist, return NULL.
 *
 **************************************/
	MutexHolder holder(request);

	thread_db* tdbb = JRD_get_thread_data();

	// Start by seeing if symbol is already defined

	dsql_sym* symbol = lookup_symbol(request->req_dbb, length, name, SYM_intlsym_charset);
	if (symbol)
		return (dsql_intlsym*) symbol->sym_object;

	// Now see if it is in the database

	validateTransaction(request);

	dsql_dbb* dbb = request->req_dbb;
	dsql_intlsym* iname = NULL;

	jrd_req* handle = CMP_find_request(tdbb, irq_charset, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
	X IN RDB$COLLATIONS
		CROSS Y IN RDB$CHARACTER_SETS OVER RDB$CHARACTER_SET_ID
		CROSS Z IN RDB$TYPES
		WITH Z.RDB$TYPE EQ Y.RDB$CHARACTER_SET_ID
		AND Z.RDB$TYPE_NAME EQ name
		AND Z.RDB$FIELD_NAME EQ "RDB$CHARACTER_SET_NAME"
		AND Y.RDB$DEFAULT_COLLATE_NAME EQ X.RDB$COLLATION_NAME*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_210, sizeof(jrd_210), true);
	gds__vtov ((const char*) name, (char*) jrd_211.jrd_212, 32);
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_211);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_213);
	   if (!jrd_213.jrd_214) break;;

		if (!DSQL_REQUEST(irq_charset))
			DSQL_REQUEST(irq_charset) = handle;

		iname = FB_NEW_RPT(dbb->dbb_pool, length) dsql_intlsym;
		strcpy(iname->intlsym_name, name);
		iname->intlsym_flags = 0;
		iname->intlsym_charset_id = /*X.RDB$CHARACTER_SET_ID*/
					    jrd_213.jrd_218;
		iname->intlsym_collate_id = /*X.RDB$COLLATION_ID*/
					    jrd_213.jrd_217;
		iname->intlsym_ttype =
			INTL_CS_COLL_TO_TTYPE(iname->intlsym_charset_id, iname->intlsym_collate_id);
		iname->intlsym_bytes_per_char =
			(/*Y.RDB$BYTES_PER_CHARACTER.NULL*/
			 jrd_213.jrd_215) ? 1 : (/*Y.RDB$BYTES_PER_CHARACTER*/
	 jrd_213.jrd_216);

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_charset))
		DSQL_REQUEST(irq_charset) = handle;

	if (!iname)
		return NULL;

	// Store in the symbol table

	symbol = iname->intlsym_symbol = FB_NEW_RPT(dbb->dbb_pool, 0) dsql_sym;
	symbol->sym_object = iname;
	symbol->sym_string = iname->intlsym_name;
	symbol->sym_length = length;
	symbol->sym_type = SYM_intlsym_charset;
	symbol->sym_dbb = dbb;
	insert_symbol(symbol);

	dbb->dbb_charsets_by_id.add(iname);

	return iname;
}


USHORT METD_get_charset_bpc(dsql_req* request, SSHORT charset_id)
{
/**************************************
 *
 *  M E T D _ g e t _ c h a r s e t _ b p c
 *
 **************************************
 *
 * Functional description
 *  Look up an international text type object.
 *  If it doesn't exist, return NULL.
 *  Go directly to system tables & return only the
 *  number of bytes per character. Lookup by
 *  charset' id, not by name.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	dsql_dbb* dbb = request->req_dbb;

	if (charset_id == CS_dynamic)
		charset_id = tdbb->getAttachment()->att_charset;

	dsql_intlsym* cs_sym = 0;
	size_t pos = 0;
	if (dbb->dbb_charsets_by_id.find(charset_id, pos)) {
		cs_sym = dbb->dbb_charsets_by_id[pos];
	}
	else {
		const MetaName cs_name = METD_get_charset_name(request, charset_id);
		cs_sym = METD_get_charset(request, cs_name.length(), cs_name.c_str());
	}

	fb_assert(cs_sym);

	return cs_sym ? cs_sym->intlsym_bytes_per_char : 0;
}


MetaName METD_get_charset_name(dsql_req* request, SSHORT charset_id)
{
   struct {
          TEXT  jrd_208 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_209;	// isc_utility 
   } jrd_207;
   struct {
          SSHORT jrd_206;	// RDB$CHARACTER_SET_ID 
   } jrd_205;
/**************************************
 *
 *  M E T D _ g e t _ c h a r s e t _ n a m e
 *
 **************************************
 *
 * Functional description
 *  Look up an international text type object.
 *  If it doesn't exist, return empty string.
 *  Go directly to system tables & return only the
 *  name.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	dsql_dbb* dbb = request->req_dbb;

	if (charset_id == CS_dynamic)
		charset_id = tdbb->getAttachment()->att_charset;

	size_t pos = 0;
	if (dbb->dbb_charsets_by_id.find(charset_id, pos))
	{
		return dbb->dbb_charsets_by_id[pos]->intlsym_name;
	}

	validateTransaction(request);

	MetaName name;

	jrd_req* handle = CMP_find_request(tdbb, irq_cs_name, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
		Y IN RDB$CHARACTER_SETS
		WITH Y.RDB$CHARACTER_SET_ID EQ charset_id*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_204, sizeof(jrd_204), true);
	jrd_205.jrd_206 = charset_id;
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_205);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 34, (UCHAR*) &jrd_207);
	   if (!jrd_207.jrd_209) break;

		if (!DSQL_REQUEST(irq_cs_name))
			DSQL_REQUEST(irq_cs_name) = handle;

		name = /*Y.RDB$CHARACTER_SET_NAME*/
		       jrd_207.jrd_208;

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_cs_name))
		DSQL_REQUEST(irq_cs_name) = handle;

	// put new charset into hash table if needed
	METD_get_charset(request, name.length(), name.c_str());

	return name;
}


dsql_str* METD_get_default_charset(dsql_req* request)
{
   struct {
          TEXT  jrd_202 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_203;	// isc_utility 
   } jrd_201;
/**************************************
 *
 *  M E T D _ g e t _ d e f a u l t _ c h a r s e t
 *
 **************************************
 *
 * Functional description
 *  Find the default character set for a database
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	dsql_dbb* dbb = request->req_dbb;
	if (dbb->dbb_no_charset)
		return NULL;

	if (dbb->dbb_dfl_charset)
		return dbb->dbb_dfl_charset;

	// Now see if it is in the database

	validateTransaction(request);

	jrd_req* handle = CMP_find_request(tdbb, irq_default_cs, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
		FIRST 1 DBB IN RDB$DATABASE
		WITH DBB.RDB$CHARACTER_SET_NAME NOT MISSING*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_200, sizeof(jrd_200), true);
	EXE_start (tdbb, handle, request->req_transaction);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 34, (UCHAR*) &jrd_201);
	   if (!jrd_201.jrd_203) break;;

		if (!DSQL_REQUEST(irq_default_cs))
			DSQL_REQUEST(irq_default_cs) = handle;

		// Terminate ASCIIZ string on first trailing blank
		fb_utils::exact_name(/*DBB.RDB$CHARACTER_SET_NAME*/
				     jrd_201.jrd_202);
		const USHORT length = strlen(/*DBB.RDB$CHARACTER_SET_NAME*/
					     jrd_201.jrd_202);
		dbb->dbb_dfl_charset = FB_NEW_RPT(dbb->dbb_pool, length) dsql_str;
		dbb->dbb_dfl_charset->str_length = length;
		dbb->dbb_dfl_charset->str_charset = NULL;
		memcpy(dbb->dbb_dfl_charset->str_data, /*DBB.RDB$CHARACTER_SET_NAME*/
						       jrd_201.jrd_202, length);

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_default_cs))
		DSQL_REQUEST(irq_default_cs) = handle;

	if (!dbb->dbb_dfl_charset)
		dbb->dbb_no_charset = true;

	return dbb->dbb_dfl_charset;
}


bool METD_get_domain(dsql_req* request, dsql_fld* field, const char* name) // UTF-8
{
   struct {
          bid  jrd_183;	// RDB$COMPUTED_BLR 
          SSHORT jrd_184;	// isc_utility 
          SSHORT jrd_185;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_186;	// RDB$FIELD_TYPE 
          SSHORT jrd_187;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_188;	// gds__null_flag 
          SSHORT jrd_189;	// gds__null_flag 
          SSHORT jrd_190;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_191;	// gds__null_flag 
          SSHORT jrd_192;	// RDB$COLLATION_ID 
          SSHORT jrd_193;	// gds__null_flag 
          SSHORT jrd_194;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_195;	// gds__null_flag 
          SSHORT jrd_196;	// RDB$DIMENSIONS 
          SSHORT jrd_197;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_198;	// RDB$FIELD_SCALE 
          SSHORT jrd_199;	// RDB$FIELD_LENGTH 
   } jrd_182;
   struct {
          TEXT  jrd_181 [32];	// RDB$FIELD_NAME 
   } jrd_180;
/**************************************
 *
 *  M E T D _ g e t _ d o m a i n
 *
 **************************************
 *
 * Functional description
 *  Fetch domain information for field defined as 'name'
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(request);

	bool found = false;

	dsql_dbb* dbb = request->req_dbb;

	jrd_req* handle = CMP_find_request(tdbb, irq_domain, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
		FLX IN RDB$FIELDS WITH FLX.RDB$FIELD_NAME EQ name*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_179, sizeof(jrd_179), true);
	gds__vtov ((const char*) name, (char*) jrd_180.jrd_181, 32);
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_180);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 40, (UCHAR*) &jrd_182);
	   if (!jrd_182.jrd_184) break;

		if (!DSQL_REQUEST(irq_domain))
			DSQL_REQUEST(irq_domain) = handle;

		found = true;
		field->fld_length = /*FLX.RDB$FIELD_LENGTH*/
				    jrd_182.jrd_199;
		field->fld_scale = /*FLX.RDB$FIELD_SCALE*/
				   jrd_182.jrd_198;
		field->fld_sub_type = /*FLX.RDB$FIELD_SUB_TYPE*/
				      jrd_182.jrd_197;
		field->fld_dimensions = /*FLX.RDB$DIMENSIONS.NULL*/
					jrd_182.jrd_195 ? 0 : /*FLX.RDB$DIMENSIONS*/
       jrd_182.jrd_196;

		field->fld_character_set_id = 0;
		if (!/*FLX.RDB$CHARACTER_SET_ID.NULL*/
		     jrd_182.jrd_193)
			field->fld_character_set_id = /*FLX.RDB$CHARACTER_SET_ID*/
						      jrd_182.jrd_194;
		field->fld_collation_id = 0;
		if (!/*FLX.RDB$COLLATION_ID.NULL*/
		     jrd_182.jrd_191)
			field->fld_collation_id = /*FLX.RDB$COLLATION_ID*/
						  jrd_182.jrd_192;
		field->fld_character_length = 0;
		if (!/*FLX.RDB$CHARACTER_LENGTH.NULL*/
		     jrd_182.jrd_189)
			field->fld_character_length = /*FLX.RDB$CHARACTER_LENGTH*/
						      jrd_182.jrd_190;

		if (!/*FLX.RDB$COMPUTED_BLR.NULL*/
		     jrd_182.jrd_188)
			field->fld_flags |= FLD_computed;

		if (/*FLX.RDB$SYSTEM_FLAG*/
		    jrd_182.jrd_187 == 1)
			field->fld_flags |= FLD_system;

		convert_dtype(field, /*FLX.RDB$FIELD_TYPE*/
				     jrd_182.jrd_186);

		if (/*FLX.RDB$FIELD_TYPE*/
		    jrd_182.jrd_186 == blr_blob) {
			field->fld_seg_length = /*FLX.RDB$SEGMENT_LENGTH*/
						jrd_182.jrd_185;
		}

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_domain))
		DSQL_REQUEST(irq_domain) = handle;

	return found;
}


USHORT METD_get_domain_default(dsql_req* request,
							   const TEXT* domain_name,
							   bool* has_default,
							   UCHAR* buffer,
							   USHORT buff_length)
{
   struct {
          bid  jrd_176;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_177;	// isc_utility 
          SSHORT jrd_178;	// gds__null_flag 
   } jrd_175;
   struct {
          TEXT  jrd_174 [32];	// RDB$FIELD_NAME 
   } jrd_173;
/*************************************************************
 *
 *  M E T D _ g e t _ d o m a i n _ d e f a u l t
 *
 **************************************************************
 *
 * Function:
 *    Gets the default value for a domain of an existing table.
 *
 **************************************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(request);

	*has_default = false;

	dsql_dbb* dbb = request->req_dbb;
	USHORT result = 0;

	jrd_req* handle = CMP_find_request(tdbb, irq_domain_2, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
		FLD IN RDB$FIELDS WITH FLD.RDB$FIELD_NAME EQ domain_name*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_172, sizeof(jrd_172), true);
	gds__vtov ((const char*) domain_name, (char*) jrd_173.jrd_174, 32);
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_173);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 12, (UCHAR*) &jrd_175);
	   if (!jrd_175.jrd_177) break;

		if (!DSQL_REQUEST(irq_domain_2))
			DSQL_REQUEST(irq_domain_2) = handle;

		bid* blob_id;
		if (!/*FLD.RDB$DEFAULT_VALUE.NULL*/
		     jrd_175.jrd_178) {
			blob_id = &/*FLD.RDB$DEFAULT_VALUE*/
				   jrd_175.jrd_176;
			*has_default = true;
		}
		else
			*has_default = false;

		if (*has_default) {
			blb* blob_handle = BLB_open2(tdbb, request->req_transaction, blob_id,
										 sizeof(blr_bpb), blr_bpb, true);

			// fetch segments. Assume buffer is big enough.
			UCHAR* ptr_in_buffer = buffer;
			while (true)
			{
				const USHORT length = BLB_get_segment(tdbb, blob_handle, ptr_in_buffer, buff_length);

				ptr_in_buffer += length;
				buff_length -= length;
				result += length;

				if (blob_handle->blb_flags & BLB_eof)
				{
					// null terminate the buffer
					*ptr_in_buffer = 0;
					break;
				}
				if (blob_handle->blb_fragment_size)
					status_exception::raise(Arg::Gds(isc_segment));
				else
					continue;
			}

			try
			{
				ThreadStatusGuard status_vector(tdbb);

				BLB_close(tdbb, blob_handle);
				blob_handle = NULL;
			}
			catch (Exception&)
			{
			}

			// the default string must be of the form:
			// blr_version4 blr_literal ..... blr_eoc
			fb_assert((buffer[0] == blr_version4) || (buffer[0] == blr_version5));
			fb_assert(buffer[1] == blr_literal);
		}
		else {
			if (request->req_dbb->dbb_db_SQL_dialect > SQL_DIALECT_V5)
				buffer[0] = blr_version5;
			else
				buffer[0] = blr_version4;
			buffer[1] = blr_eoc;
			result = 2;
		}

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_domain_2))
		DSQL_REQUEST(irq_domain_2) = handle;

	return result;
}


bool METD_get_exception(dsql_req* request, const dsql_str* name)
{
   struct {
          SSHORT jrd_171;	// isc_utility 
   } jrd_170;
   struct {
          TEXT  jrd_169 [32];	// RDB$EXCEPTION_NAME 
   } jrd_168;
/**************************************
 *
 *  M E T D _ g e t _ e x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *  Look up an exception.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(request);

	dsql_dbb* dbb = request->req_dbb;
	bool found = false;

	jrd_req* handle = CMP_find_request(tdbb, irq_exception, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
		X IN RDB$EXCEPTIONS WITH
		X.RDB$EXCEPTION_NAME EQ name->str_data*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_167, sizeof(jrd_167), true);
	gds__vtov ((const char*) name->str_data, (char*) jrd_168.jrd_169, 32);
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_168);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_170);
	   if (!jrd_170.jrd_171) break;;

		if (!DSQL_REQUEST(irq_exception))
			DSQL_REQUEST(irq_exception) = handle;

		found = true;

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_exception))
		DSQL_REQUEST(irq_exception) = handle;

	return found;
}


dsql_udf* METD_get_function(dsql_req* request, const dsql_str* name)
{
   struct {
          SSHORT jrd_151;	// isc_utility 
          SSHORT jrd_152;	// RDB$MECHANISM 
          SSHORT jrd_153;	// gds__null_flag 
          SSHORT jrd_154;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_155;	// RDB$FIELD_LENGTH 
          SSHORT jrd_156;	// gds__null_flag 
          SSHORT jrd_157;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_158;	// RDB$FIELD_SCALE 
          SSHORT jrd_159;	// RDB$FIELD_TYPE 
          SSHORT jrd_160;	// RDB$ARGUMENT_POSITION 
   } jrd_150;
   struct {
          TEXT  jrd_149 [32];	// RDB$FUNCTION_NAME 
   } jrd_148;
   struct {
          SSHORT jrd_165;	// isc_utility 
          SSHORT jrd_166;	// RDB$RETURN_ARGUMENT 
   } jrd_164;
   struct {
          TEXT  jrd_163 [32];	// RDB$FUNCTION_NAME 
   } jrd_162;
/**************************************
 *
 *  M E T D _ g e t _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *  Look up a user defined function.  If it doesn't exist,
 *  return NULL.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	MutexHolder holder(request);

	// Start by seeing if symbol is already defined

	dsql_sym* symbol = lookup_symbol(request->req_dbb, name, SYM_udf);
	if (symbol)
		return (dsql_udf*) symbol->sym_object;

	// Now see if it is in the database

	validateTransaction(request);

	dsql_dbb* dbb = request->req_dbb;
	dsql_udf* userFunc = NULL;
	USHORT return_arg = 0;

	jrd_req* handle1 = CMP_find_request(tdbb, irq_function, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle1 TRANSACTION_HANDLE request->req_transaction)
		X IN RDB$FUNCTIONS WITH
		X.RDB$FUNCTION_NAME EQ name->str_data*/
	{
	if (!handle1)
	   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_161, sizeof(jrd_161), true);
	gds__vtov ((const char*) name->str_data, (char*) jrd_162.jrd_163, 32);
	EXE_start (tdbb, handle1, request->req_transaction);
	EXE_send (tdbb, handle1, 0, 32, (UCHAR*) &jrd_162);
	while (1)
	   {
	   EXE_receive (tdbb, handle1, 1, 4, (UCHAR*) &jrd_164);
	   if (!jrd_164.jrd_165) break;

		if (!DSQL_REQUEST(irq_function))
			DSQL_REQUEST(irq_function) = handle1;

		userFunc = FB_NEW(dbb->dbb_pool) dsql_udf(dbb->dbb_pool);
		// Moved below as still can't say for sure it will be stored.
		// Following the same logic for MET_get_procedure and MET_get_relation
		// userFunc->udf_next = dbb->dbb_functions;
		// dbb->dbb_functions = userFunc;

		userFunc->udf_name = name->str_data;
		return_arg = /*X.RDB$RETURN_ARGUMENT*/
			     jrd_164.jrd_166;

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_function))
		DSQL_REQUEST(irq_function) = handle1;

	if (!userFunc) {
		return NULL;
	}

	// Note: The following two requests differ in the fields which are
	// new since ODS7 (DBB_v3 flag).  One of the two requests
	// here will be cached in dbb_internal[ird_func_return],
	// the one that is appropriate for the database we are
	// working against.

	jrd_req* handle2 = CMP_find_request(tdbb, irq_func_return, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle2 TRANSACTION_HANDLE request->req_transaction)
		X IN RDB$FUNCTION_ARGUMENTS WITH
		X.RDB$FUNCTION_NAME EQ name->str_data
		SORTED BY X.RDB$ARGUMENT_POSITION*/
	{
	if (!handle2)
	   handle2 = CMP_compile2 (tdbb, (UCHAR*) jrd_147, sizeof(jrd_147), true);
	gds__vtov ((const char*) name->str_data, (char*) jrd_148.jrd_149, 32);
	EXE_start (tdbb, handle2, request->req_transaction);
	EXE_send (tdbb, handle2, 0, 32, (UCHAR*) &jrd_148);
	while (1)
	   {
	   EXE_receive (tdbb, handle2, 1, 20, (UCHAR*) &jrd_150);
	   if (!jrd_150.jrd_151) break;

		if (!DSQL_REQUEST(irq_func_return))
			DSQL_REQUEST(irq_func_return) = handle2;

		if (/*X.RDB$ARGUMENT_POSITION*/
		    jrd_150.jrd_160 == return_arg) {
			userFunc->udf_dtype = (/*X.RDB$FIELD_TYPE*/
					       jrd_150.jrd_159 != blr_blob) ?
				gds_cvt_blr_dtype[/*X.RDB$FIELD_TYPE*/
						  jrd_150.jrd_159] : dtype_blob;
			userFunc->udf_scale = /*X.RDB$FIELD_SCALE*/
					      jrd_150.jrd_158;

			if (!/*X.RDB$FIELD_SUB_TYPE.NULL*/
			     jrd_150.jrd_156) {
				userFunc->udf_sub_type = /*X.RDB$FIELD_SUB_TYPE*/
							 jrd_150.jrd_157;
			}
			else {
				userFunc->udf_sub_type = 0;
			}
			// CVC: We are overcoming a bug in ddl.c:put_field()
			// when any field is defined: the length is not given for blobs.
			if (/*X.RDB$FIELD_TYPE*/
			    jrd_150.jrd_159 == blr_blob)
				userFunc->udf_length = sizeof(ISC_QUAD);
			else
				userFunc->udf_length = /*X.RDB$FIELD_LENGTH*/
						       jrd_150.jrd_155;

			if (!/*X.RDB$CHARACTER_SET_ID.NULL*/
			     jrd_150.jrd_153) {
				userFunc->udf_character_set_id = /*X.RDB$CHARACTER_SET_ID*/
								 jrd_150.jrd_154;
			}
		}
		else {
			DSC d;
			d.dsc_dtype = (/*X.RDB$FIELD_TYPE*/
				       jrd_150.jrd_159 != blr_blob) ?
				gds_cvt_blr_dtype[/*X.RDB$FIELD_TYPE*/
						  jrd_150.jrd_159] : dtype_blob;
			// dimitr: adjust the UDF arguments for CSTRING
			if (d.dsc_dtype == dtype_cstring) {
				d.dsc_dtype = dtype_text;
			}
			d.dsc_scale = /*X.RDB$FIELD_SCALE*/
				      jrd_150.jrd_158;
			if (!/*X.RDB$FIELD_SUB_TYPE.NULL*/
			     jrd_150.jrd_156) {
				d.dsc_sub_type = /*X.RDB$FIELD_SUB_TYPE*/
						 jrd_150.jrd_157;
			}
			else {
				d.dsc_sub_type = 0;
			}
			d.dsc_length = /*X.RDB$FIELD_LENGTH*/
				       jrd_150.jrd_155;
			if (d.dsc_dtype == dtype_varying) {
				d.dsc_length += sizeof(USHORT);
			}
			d.dsc_address = NULL;

			if (!/*X.RDB$CHARACTER_SET_ID.NULL*/
			     jrd_150.jrd_153) {
				if (d.dsc_dtype != dtype_blob) {
					d.dsc_ttype() = /*X.RDB$CHARACTER_SET_ID*/
							jrd_150.jrd_154;
				}
				else {
					d.dsc_scale = /*X.RDB$CHARACTER_SET_ID*/
						      jrd_150.jrd_154;
				}
			}

			if (/*X.RDB$MECHANISM*/
			    jrd_150.jrd_152 != FUN_value && /*X.RDB$MECHANISM*/
		 jrd_150.jrd_152 != FUN_reference)
			{
				d.dsc_flags = DSC_nullable;
			}

			userFunc->udf_arguments.add(d);
		}

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_func_return))
		DSQL_REQUEST(irq_func_return) = handle2;

	// Adjust the return type & length of the UDF to account for
	// cstring & varying.  While a UDF can return CSTRING, we convert it
	// to VARCHAR for manipulation as CSTRING is not a SQL type.

	if (userFunc->udf_dtype == dtype_cstring) {
		userFunc->udf_dtype = dtype_varying;
		userFunc->udf_length += sizeof(USHORT);
		if (userFunc->udf_length > MAX_SSHORT)
			userFunc->udf_length = MAX_SSHORT;
	}
	else if (userFunc->udf_dtype == dtype_varying)
		userFunc->udf_length += sizeof(USHORT);

	if ((symbol = lookup_symbol(request->req_dbb, name, SYM_udf)))
	{
		// Get rid of all the stuff we just read in. Use existing one
		delete userFunc;
		return (dsql_udf*) symbol->sym_object;
	}

	// Add udf in the front of the list.
	userFunc->udf_next = dbb->dbb_functions;
	dbb->dbb_functions = userFunc;

	// Store in the symbol table
	// The UDF_new_udf flag is not used, so nothing extra to check.

	symbol = userFunc->udf_symbol = FB_NEW_RPT(dbb->dbb_pool, 0) dsql_sym;
	symbol->sym_object = userFunc;
	symbol->sym_string = userFunc->udf_name.c_str();
	symbol->sym_length = userFunc->udf_name.length();
	symbol->sym_type = SYM_udf;
	symbol->sym_dbb = dbb;
	insert_symbol(symbol);

	return userFunc;
}


dsql_nod* METD_get_primary_key(dsql_req* request, const dsql_str* relation_name)
{
   struct {
          TEXT  jrd_145 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_146;	// isc_utility 
   } jrd_144;
   struct {
          TEXT  jrd_143 [32];	// RDB$RELATION_NAME 
   } jrd_142;
/**************************************
 *
 *  M E T D _ g e t _ p r i m a r y _ k e y
 *
 **************************************
 *
 * Functional description
 *  Lookup the fields for the primary key
 *  index on a relation, returning a list
 *  node of the fields.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(request);

	dsql_dbb* dbb = request->req_dbb;

	DsqlNodStack stack;

	jrd_req* handle = CMP_find_request(tdbb, irq_primary_key, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
		X IN RDB$INDICES CROSS
		Y IN RDB$INDEX_SEGMENTS
		OVER RDB$INDEX_NAME CROSS
		Z IN RDB$RELATION_CONSTRAINTS
		OVER RDB$INDEX_NAME
		WITH Z.RDB$RELATION_NAME EQ relation_name->str_data
		AND Z.RDB$CONSTRAINT_TYPE EQ "PRIMARY KEY"
		SORTED BY Y.RDB$FIELD_POSITION*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_141, sizeof(jrd_141), true);
	gds__vtov ((const char*) relation_name->str_data, (char*) jrd_142.jrd_143, 32);
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_142);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 34, (UCHAR*) &jrd_144);
	   if (!jrd_144.jrd_146) break;

		if (!DSQL_REQUEST(irq_primary_key))
			DSQL_REQUEST(irq_primary_key) = handle;

		stack.push(MAKE_field_name(/*Y.RDB$FIELD_NAME*/
					   jrd_144.jrd_145));

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_primary_key))
		DSQL_REQUEST(irq_primary_key) = handle;

	return (stack.getCount() ? MAKE_list(stack) : NULL);
}


dsql_prc* METD_get_procedure(CompiledStatement* statement, const dsql_str* name)
{
   struct {
          bid  jrd_98;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_99;	// isc_utility 
          SSHORT jrd_100;	// gds__null_flag 
          SSHORT jrd_101;	// RDB$PARAMETER_MECHANISM 
          SSHORT jrd_102;	// gds__null_flag 
          SSHORT jrd_103;	// RDB$NULL_FLAG 
          SSHORT jrd_104;	// gds__null_flag 
          SSHORT jrd_105;	// gds__null_flag 
          SSHORT jrd_106;	// RDB$COLLATION_ID 
   } jrd_97;
   struct {
          TEXT  jrd_95 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_96 [32];	// RDB$PROCEDURE_NAME 
   } jrd_94;
   struct {
          bid  jrd_112;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_113 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_114 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_115 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_116 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_117 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_118 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_119;	// isc_utility 
          SSHORT jrd_120;	// gds__null_flag 
          SSHORT jrd_121;	// gds__null_flag 
          SSHORT jrd_122;	// gds__null_flag 
          SSHORT jrd_123;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_124;	// RDB$NULL_FLAG 
          SSHORT jrd_125;	// RDB$FIELD_TYPE 
          SSHORT jrd_126;	// gds__null_flag 
          SSHORT jrd_127;	// RDB$COLLATION_ID 
          SSHORT jrd_128;	// gds__null_flag 
          SSHORT jrd_129;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_130;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_131;	// RDB$FIELD_SCALE 
          SSHORT jrd_132;	// RDB$FIELD_LENGTH 
          SSHORT jrd_133;	// RDB$PARAMETER_NUMBER 
   } jrd_111;
   struct {
          TEXT  jrd_109 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_110;	// RDB$PARAMETER_TYPE 
   } jrd_108;
   struct {
          TEXT  jrd_138 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_139;	// isc_utility 
          SSHORT jrd_140;	// RDB$PROCEDURE_ID 
   } jrd_137;
   struct {
          TEXT  jrd_136 [32];	// RDB$PROCEDURE_NAME 
   } jrd_135;
/**************************************
 *
 *  M E T D _ g e t _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *  Look up a procedure.  If it doesn't exist, return NULL.
 *  If it does, fetch field information as well.
 *  If it is marked dropped, try to read from system tables
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	dsql_dbb* dbb = statement->req_dbb;

	// see if the procedure is the one currently being defined in this statement

	dsql_prc* temp = statement->req_procedure;
	if (temp != NULL && temp->prc_name == name->str_data)
	{
		return temp;
	}

	MutexHolder holder(statement);

	// Start by seeing if symbol is already defined

	dsql_sym* symbol = lookup_symbol(statement->req_dbb, name, SYM_procedure);
	if (symbol)
		return (dsql_prc*) symbol->sym_object;

	// now see if it is in the database

	validateTransaction(statement);

	dsql_prc* procedure = NULL;

	jrd_req* handle1 = CMP_find_request(tdbb, irq_procedure, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle1 TRANSACTION_HANDLE statement->req_transaction)
		X IN RDB$PROCEDURES WITH
		X.RDB$PROCEDURE_NAME EQ name->str_data*/
	{
	if (!handle1)
	   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_134, sizeof(jrd_134), true);
	gds__vtov ((const char*) name->str_data, (char*) jrd_135.jrd_136, 32);
	EXE_start (tdbb, handle1, statement->req_transaction);
	EXE_send (tdbb, handle1, 0, 32, (UCHAR*) &jrd_135);
	while (1)
	   {
	   EXE_receive (tdbb, handle1, 1, 36, (UCHAR*) &jrd_137);
	   if (!jrd_137.jrd_139) break;

		if (!DSQL_REQUEST(irq_procedure))
			DSQL_REQUEST(irq_procedure) = handle1;

		fb_utils::exact_name(/*X.RDB$OWNER_NAME*/
				     jrd_137.jrd_138);

		procedure = FB_NEW(dbb->dbb_pool) dsql_prc(dbb->dbb_pool);
		procedure->prc_id = /*X.RDB$PROCEDURE_ID*/
				    jrd_137.jrd_140;

		procedure->prc_name = name->str_data;
		procedure->prc_owner = /*X.RDB$OWNER_NAME*/
				       jrd_137.jrd_138;

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_procedure))
		DSQL_REQUEST(irq_procedure) = handle1;

	if (!procedure) {
		return NULL;
	}

	// Lookup parameter stuff

	for (int type = 0; type < 2; type++)
	{
		dsql_fld** ptr;
		if (type)
			ptr = &procedure->prc_outputs;
		else
			ptr = &procedure->prc_inputs;

		SSHORT count = 0, defaults = 0;

		jrd_req* handle2 = CMP_find_request(tdbb, irq_parameters, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE handle2 TRANSACTION_HANDLE statement->req_transaction)
			PR IN RDB$PROCEDURE_PARAMETERS CROSS
			FLD IN RDB$FIELDS
			WITH FLD.RDB$FIELD_NAME EQ PR.RDB$FIELD_SOURCE
			AND PR.RDB$PROCEDURE_NAME EQ name->str_data
			AND PR.RDB$PARAMETER_TYPE = type
			SORTED BY DESCENDING PR.RDB$PARAMETER_NUMBER*/
		{
		if (!handle2)
		   handle2 = CMP_compile2 (tdbb, (UCHAR*) jrd_107, sizeof(jrd_107), true);
		gds__vtov ((const char*) name->str_data, (char*) jrd_108.jrd_109, 32);
		jrd_108.jrd_110 = type;
		EXE_start (tdbb, handle2, statement->req_transaction);
		EXE_send (tdbb, handle2, 0, 34, (UCHAR*) &jrd_108);
		while (1)
		   {
		   EXE_receive (tdbb, handle2, 1, 230, (UCHAR*) &jrd_111);
		   if (!jrd_111.jrd_119) break;

			if (!DSQL_REQUEST(irq_parameters))
				DSQL_REQUEST(irq_parameters) = handle2;

			SSHORT pr_collation_id_null = TRUE;
			SSHORT pr_collation_id;
			SSHORT pr_default_value_null = TRUE;
			SSHORT pr_null_flag_null = TRUE;
			SSHORT pr_null_flag;
			bool pr_type_of = false;

			if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_version) >= ODS_11_1)
			{
				jrd_req* handle3 = CMP_find_request(tdbb, irq_parameters2, IRQ_REQUESTS);

				/*FOR(REQUEST_HANDLE handle3 TRANSACTION_HANDLE statement->req_transaction)
					PR2 IN RDB$PROCEDURE_PARAMETERS
					WITH PR2.RDB$PROCEDURE_NAME EQ PR.RDB$PROCEDURE_NAME AND
						 PR2.RDB$PARAMETER_NAME EQ PR.RDB$PARAMETER_NAME*/
				{
				if (!handle3)
				   handle3 = CMP_compile2 (tdbb, (UCHAR*) jrd_93, sizeof(jrd_93), true);
				gds__vtov ((const char*) jrd_111.jrd_117, (char*) jrd_94.jrd_95, 32);
				gds__vtov ((const char*) jrd_111.jrd_118, (char*) jrd_94.jrd_96, 32);
				EXE_start (tdbb, handle3, statement->req_transaction);
				EXE_send (tdbb, handle3, 0, 64, (UCHAR*) &jrd_94);
				while (1)
				   {
				   EXE_receive (tdbb, handle3, 1, 24, (UCHAR*) &jrd_97);
				   if (!jrd_97.jrd_99) break;

					if (!DSQL_REQUEST(irq_parameters2))
						DSQL_REQUEST(irq_parameters2) = handle3;

					pr_collation_id_null = /*PR2.RDB$COLLATION_ID.NULL*/
							       jrd_97.jrd_105;
					pr_collation_id = /*PR2.RDB$COLLATION_ID*/
							  jrd_97.jrd_106;

					pr_default_value_null = /*PR2.RDB$DEFAULT_VALUE.NULL*/
								jrd_97.jrd_104;

					pr_null_flag_null = /*PR2.RDB$NULL_FLAG.NULL*/
							    jrd_97.jrd_102;
					pr_null_flag = /*PR2.RDB$NULL_FLAG*/
						       jrd_97.jrd_103;

					if (!/*PR2.RDB$PARAMETER_MECHANISM.NULL*/
					     jrd_97.jrd_100 && /*PR2.RDB$PARAMETER_MECHANISM*/
    jrd_97.jrd_101 == prm_mech_type_of)
						pr_type_of = true;

				/*END_FOR*/
				   }
				}

				if (!DSQL_REQUEST(irq_parameters2))
					DSQL_REQUEST(irq_parameters2) = handle3;
			}

			count++;
			// allocate the field block

			fb_utils::exact_name(/*PR.RDB$PARAMETER_NAME*/
					     jrd_111.jrd_117);
			fb_utils::exact_name(/*PR.RDB$FIELD_SOURCE*/
					     jrd_111.jrd_116);

			dsql_fld* parameter = FB_NEW(dbb->dbb_pool) dsql_fld(dbb->dbb_pool);
			parameter->fld_next = *ptr;
			*ptr = parameter;

			// get parameter information

			parameter->fld_name = /*PR.RDB$PARAMETER_NAME*/
					      jrd_111.jrd_117;
			parameter->fld_source = /*PR.RDB$FIELD_SOURCE*/
						jrd_111.jrd_116;

			parameter->fld_id = /*PR.RDB$PARAMETER_NUMBER*/
					    jrd_111.jrd_133;
			parameter->fld_length = /*FLD.RDB$FIELD_LENGTH*/
						jrd_111.jrd_132;
			parameter->fld_scale = /*FLD.RDB$FIELD_SCALE*/
					       jrd_111.jrd_131;
			parameter->fld_sub_type = /*FLD.RDB$FIELD_SUB_TYPE*/
						  jrd_111.jrd_130;
			parameter->fld_procedure = procedure;

			if (!/*FLD.RDB$CHARACTER_SET_ID.NULL*/
			     jrd_111.jrd_128)
				parameter->fld_character_set_id = /*FLD.RDB$CHARACTER_SET_ID*/
								  jrd_111.jrd_129;

			if (!pr_collation_id_null)
				parameter->fld_collation_id = pr_collation_id;
			else if (!/*FLD.RDB$COLLATION_ID.NULL*/
				  jrd_111.jrd_126)
				parameter->fld_collation_id = /*FLD.RDB$COLLATION_ID*/
							      jrd_111.jrd_127;

			convert_dtype(parameter, /*FLD.RDB$FIELD_TYPE*/
						 jrd_111.jrd_125);

			if (!pr_null_flag_null)
			{
				if (!pr_null_flag)
					parameter->fld_flags |= FLD_nullable;
			}
			else if (!/*FLD.RDB$NULL_FLAG*/
				  jrd_111.jrd_124 || pr_type_of)
				parameter->fld_flags |= FLD_nullable;

			if (/*FLD.RDB$FIELD_TYPE*/
			    jrd_111.jrd_125 == blr_blob)
				parameter->fld_seg_length = /*FLD.RDB$SEGMENT_LENGTH*/
							    jrd_111.jrd_123;

			if (!/*PR.RDB$FIELD_NAME.NULL*/
			     jrd_111.jrd_122)
			{
				fb_utils::exact_name(/*PR.RDB$FIELD_NAME*/
						     jrd_111.jrd_115);
				parameter->fld_type_of_name = /*PR.RDB$FIELD_NAME*/
							      jrd_111.jrd_115;
			}

			if (!/*PR.RDB$RELATION_NAME.NULL*/
			     jrd_111.jrd_121)
			{
				fb_utils::exact_name(/*PR.RDB$RELATION_NAME*/
						     jrd_111.jrd_114);
				parameter->fld_type_of_table = /*PR.RDB$RELATION_NAME*/
							       jrd_111.jrd_114;
			}

			if (type == 0 &&
				(!pr_default_value_null ||
					(fb_utils::implicit_domain(/*FLD.RDB$FIELD_NAME*/
								   jrd_111.jrd_113) && !/*FLD.RDB$DEFAULT_VALUE.NULL*/
      jrd_111.jrd_120)))
			{
				defaults++;
			}

		/*END_FOR*/
		   }
		}

		if (!DSQL_REQUEST(irq_parameters))
			DSQL_REQUEST(irq_parameters) = handle2;

		if (type)
			procedure->prc_out_count = count;
		else {
			procedure->prc_in_count = count;
			procedure->prc_def_count = defaults;
		}
	}

	// Since we could give up control due to the THREAD_EXIT and THEAD_ENTER
	// calls, another thread may have added the same procedure in the mean time

	if ((symbol = lookup_symbol(statement->req_dbb, name, SYM_procedure)))
	{
		// Get rid of all the stuff we just read in. Use existing one
		free_procedure(procedure);
		return (dsql_prc*) symbol->sym_object;
	}

	// store in the symbol table unless the procedure is not yet committed
	// CVC: This is strange, because PRC_new_procedure is never set.

	if (!(procedure->prc_flags & PRC_new_procedure))
	{
		// add procedure in the front of the list

		procedure->prc_next = dbb->dbb_procedures;
		dbb->dbb_procedures = procedure;

		symbol = procedure->prc_symbol = FB_NEW_RPT(dbb->dbb_pool, 0) dsql_sym;
		symbol->sym_object = procedure;
		symbol->sym_string = procedure->prc_name.c_str();
		symbol->sym_length = procedure->prc_name.length();
		symbol->sym_type = SYM_procedure;
		symbol->sym_dbb = dbb;
		insert_symbol(symbol);
	}

	return procedure;
}


// Get the description of a procedure parameter. The description is returned in the client charset.
void METD_get_procedure_parameter(CompiledStatement* statement, const MetaName& procedure,
	const MetaName& parameter, UCharBuffer& buffer)
{
   struct {
          bid  jrd_91;	// RDB$DESCRIPTION 
          SSHORT jrd_92;	// isc_utility 
   } jrd_90;
   struct {
          TEXT  jrd_88 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_89 [32];	// RDB$PROCEDURE_NAME 
   } jrd_87;
	thread_db* tdbb = JRD_get_thread_data();

	dsql_dbb* dbb = statement->req_dbb;

	MutexHolder holder(statement);

	validateTransaction(statement);

	buffer.clear();

	jrd_req* handle = CMP_find_request(tdbb, irq_parameters_description, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE statement->req_transaction)
		X IN RDB$PROCEDURE_PARAMETERS
		WITH X.RDB$PROCEDURE_NAME EQ procedure.c_str() AND
			 X.RDB$PARAMETER_NAME EQ parameter.c_str() AND
			 X.RDB$DESCRIPTION NOT MISSING*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_86, sizeof(jrd_86), true);
	gds__vtov ((const char*) parameter.c_str(), (char*) jrd_87.jrd_88, 32);
	gds__vtov ((const char*) procedure.c_str(), (char*) jrd_87.jrd_89, 32);
	EXE_start (tdbb, handle, statement->req_transaction);
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_87);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_90);
	   if (!jrd_90.jrd_92) break;
	{
		if (!DSQL_REQUEST(irq_parameters_description))
			DSQL_REQUEST(irq_parameters_description) = handle;

		UCharBuffer bpb;
		BLB_gen_bpb(isc_blob_text, isc_blob_text, CS_METADATA, CS_dynamic, bpb);

		blb* blob = BLB_open2(tdbb, statement->req_transaction, &/*X.RDB$DESCRIPTION*/
									 jrd_90.jrd_91,
			bpb.getCount(), bpb.begin());
		buffer.resize(blob->blb_length);
		BLB_get_data(tdbb, blob, buffer.begin(), buffer.getCount());
	}
	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_parameters_description))
		DSQL_REQUEST(irq_parameters_description) = handle;
}


dsql_rel* METD_get_relation(CompiledStatement* statement, const char* name)
{
   struct {
          bid  jrd_44;	// RDB$COMPUTED_BLR 
          TEXT  jrd_45 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_46 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_47;	// isc_utility 
          SSHORT jrd_48;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_49;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_50;	// RDB$NULL_FLAG 
          SSHORT jrd_51;	// RDB$NULL_FLAG 
          SSHORT jrd_52;	// gds__null_flag 
          SSHORT jrd_53;	// RDB$COLLATION_ID 
          SSHORT jrd_54;	// gds__null_flag 
          SSHORT jrd_55;	// RDB$COLLATION_ID 
          SSHORT jrd_56;	// gds__null_flag 
          SSHORT jrd_57;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_58;	// gds__null_flag 
          SSHORT jrd_59;	// RDB$DIMENSIONS 
          SSHORT jrd_60;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_61;	// RDB$FIELD_TYPE 
          SSHORT jrd_62;	// gds__null_flag 
          SSHORT jrd_63;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_64;	// RDB$FIELD_SCALE 
          SSHORT jrd_65;	// RDB$FIELD_LENGTH 
          SSHORT jrd_66;	// gds__null_flag 
          SSHORT jrd_67;	// RDB$FIELD_ID 
   } jrd_43;
   struct {
          TEXT  jrd_42 [32];	// RDB$RELATION_NAME 
   } jrd_41;
   struct {
          TEXT  jrd_72 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_73;	// RDB$VIEW_BLR 
          TEXT  jrd_74 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_75;	// isc_utility 
          SSHORT jrd_76;	// gds__null_flag 
          SSHORT jrd_77;	// gds__null_flag 
          SSHORT jrd_78;	// RDB$DBKEY_LENGTH 
          SSHORT jrd_79;	// gds__null_flag 
          SSHORT jrd_80;	// RDB$RELATION_ID 
   } jrd_71;
   struct {
          TEXT  jrd_70 [32];	// RDB$RELATION_NAME 
   } jrd_69;
   struct {
          SSHORT jrd_85;	// isc_utility 
   } jrd_84;
   struct {
          TEXT  jrd_83 [32];	// RDB$RELATION_NAME 
   } jrd_82;
/**************************************
 *
 *  M E T D _ g e t _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Look up a relation.  If it doesn't exist, return NULL.
 *  If it does, fetch field information as well.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	dsql_dbb* dbb = statement->req_dbb;

	// See if the relation is the one currently being defined in this statement

	dsql_rel* temp = statement->req_relation;
	if (temp != NULL && temp->rel_name == name)
	{
		return temp;
	}

	MutexHolder holder(statement);

	// Start by seeing if symbol is already defined

	dsql_sym* symbol = lookup_symbol(statement->req_dbb, strlen(name), name, SYM_relation);
	if (symbol)
		return (dsql_rel*) symbol->sym_object;

	// If the relation id or any of the field ids have not yet been assigned,
	// and this is a type of statement which does not use ids, prepare a
	// temporary relation block to provide information without caching it

	validateTransaction(statement);

	bool permanent = true;

	jrd_req* handle1 = CMP_find_request(tdbb, irq_rel_ids, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle1 TRANSACTION_HANDLE statement->req_transaction)
		REL IN RDB$RELATIONS
		CROSS RFR IN RDB$RELATION_FIELDS OVER RDB$RELATION_NAME
		WITH REL.RDB$RELATION_NAME EQ name
		AND (REL.RDB$RELATION_ID MISSING OR RFR.RDB$FIELD_ID MISSING)*/
	{
	if (!handle1)
	   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_81, sizeof(jrd_81), true);
	gds__vtov ((const char*) name, (char*) jrd_82.jrd_83, 32);
	EXE_start (tdbb, handle1, statement->req_transaction);
	EXE_send (tdbb, handle1, 0, 32, (UCHAR*) &jrd_82);
	while (1)
	   {
	   EXE_receive (tdbb, handle1, 1, 2, (UCHAR*) &jrd_84);
	   if (!jrd_84.jrd_85) break;

		if (!DSQL_REQUEST(irq_rel_ids))
			DSQL_REQUEST(irq_rel_ids) = handle1;

		permanent = false;

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_rel_ids))
		DSQL_REQUEST(irq_rel_ids) = handle1;

	// Now see if it is in the database

	MemoryPool& pool = permanent ? dbb->dbb_pool : *tdbb->getDefaultPool();

	dsql_rel* relation = NULL;

	jrd_req* handle2 = CMP_find_request(tdbb, irq_relation, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle2 TRANSACTION_HANDLE statement->req_transaction)
		X IN RDB$RELATIONS WITH X.RDB$RELATION_NAME EQ name*/
	{
	if (!handle2)
	   handle2 = CMP_compile2 (tdbb, (UCHAR*) jrd_68, sizeof(jrd_68), true);
	gds__vtov ((const char*) name, (char*) jrd_69.jrd_70, 32);
	EXE_start (tdbb, handle2, statement->req_transaction);
	EXE_send (tdbb, handle2, 0, 32, (UCHAR*) &jrd_69);
	while (1)
	   {
	   EXE_receive (tdbb, handle2, 1, 308, (UCHAR*) &jrd_71);
	   if (!jrd_71.jrd_75) break;

		if (!DSQL_REQUEST(irq_relation))
			DSQL_REQUEST(irq_relation) = handle2;

		fb_utils::exact_name(/*X.RDB$OWNER_NAME*/
				     jrd_71.jrd_74);

		// Allocate from default or permanent pool as appropriate

		if (!/*X.RDB$RELATION_ID.NULL*/
		     jrd_71.jrd_79) {
			relation = FB_NEW(pool) dsql_rel(pool);
			relation->rel_id = /*X.RDB$RELATION_ID*/
					   jrd_71.jrd_80;
		}
		else if (!DDL_ids(statement)) {
			relation = FB_NEW(pool) dsql_rel(pool);
		}

		// fill out the relation information

		if (relation) {
			relation->rel_name = name;
			relation->rel_owner = /*X.RDB$OWNER_NAME*/
					      jrd_71.jrd_74;
			if (!(relation->rel_dbkey_length = /*X.RDB$DBKEY_LENGTH*/
							   jrd_71.jrd_78))
				relation->rel_dbkey_length = 8;
			// CVC: let's see if this is a table or a view.
			if (!/*X.RDB$VIEW_BLR.NULL*/
			     jrd_71.jrd_77) {
				relation->rel_flags |= REL_view;
			}
			if (!/*X.RDB$EXTERNAL_FILE.NULL*/
			     jrd_71.jrd_76) {
				relation->rel_flags |= REL_external;
			}
		}

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_relation))
		DSQL_REQUEST(irq_relation) = handle2;

	if (!relation) {
		return NULL;
	}

	// Lookup field stuff

	dsql_fld** ptr = &relation->rel_fields;

	jrd_req* handle3 = CMP_find_request(tdbb, irq_fields, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle3 TRANSACTION_HANDLE statement->req_transaction)
		FLX IN RDB$FIELDS CROSS
		RFR IN RDB$RELATION_FIELDS
		WITH FLX.RDB$FIELD_NAME EQ RFR.RDB$FIELD_SOURCE
		AND RFR.RDB$RELATION_NAME EQ name
		SORTED BY RFR.RDB$FIELD_POSITION*/
	{
	if (!handle3)
	   handle3 = CMP_compile2 (tdbb, (UCHAR*) jrd_40, sizeof(jrd_40), true);
	gds__vtov ((const char*) name, (char*) jrd_41.jrd_42, 32);
	EXE_start (tdbb, handle3, statement->req_transaction);
	EXE_send (tdbb, handle3, 0, 32, (UCHAR*) &jrd_41);
	while (1)
	   {
	   EXE_receive (tdbb, handle3, 1, 114, (UCHAR*) &jrd_43);
	   if (!jrd_43.jrd_47) break;

		if (!DSQL_REQUEST(irq_fields))
			DSQL_REQUEST(irq_fields) = handle3;

		// allocate the field block

		fb_utils::exact_name(/*RFR.RDB$FIELD_NAME*/
				     jrd_43.jrd_46);
		fb_utils::exact_name(/*RFR.RDB$FIELD_SOURCE*/
				     jrd_43.jrd_45);

		// Allocate from default or permanent pool as appropriate

		dsql_fld* field = NULL;

		if (!/*RFR.RDB$FIELD_ID.NULL*/
		     jrd_43.jrd_66) {
			field = FB_NEW(pool) dsql_fld(pool);
			field->fld_id = /*RFR.RDB$FIELD_ID*/
					jrd_43.jrd_67;
		}
		else if (!DDL_ids(statement))
			field = FB_NEW(pool) dsql_fld(pool);

		if (field) {
			*ptr = field;
			ptr = &field->fld_next;

			// get field information

			field->fld_name = /*RFR.RDB$FIELD_NAME*/
					  jrd_43.jrd_46;
			field->fld_source = /*RFR.RDB$FIELD_SOURCE*/
					    jrd_43.jrd_45;
			field->fld_length = /*FLX.RDB$FIELD_LENGTH*/
					    jrd_43.jrd_65;
			field->fld_scale = /*FLX.RDB$FIELD_SCALE*/
					   jrd_43.jrd_64;
			field->fld_sub_type = /*FLX.RDB$FIELD_SUB_TYPE*/
					      jrd_43.jrd_63;
			field->fld_relation = relation;

			if (!/*FLX.RDB$COMPUTED_BLR.NULL*/
			     jrd_43.jrd_62)
				field->fld_flags |= FLD_computed;

			convert_dtype(field, /*FLX.RDB$FIELD_TYPE*/
					     jrd_43.jrd_61);

			if (/*FLX.RDB$FIELD_TYPE*/
			    jrd_43.jrd_61 == blr_blob) {
				field->fld_seg_length = /*FLX.RDB$SEGMENT_LENGTH*/
							jrd_43.jrd_60;
			}

			if (!/*FLX.RDB$DIMENSIONS.NULL*/
			     jrd_43.jrd_58 && /*FLX.RDB$DIMENSIONS*/
    jrd_43.jrd_59)
			{
				field->fld_element_dtype = field->fld_dtype;
				field->fld_element_length = field->fld_length;
				field->fld_dtype = dtype_array;
				field->fld_length = sizeof(ISC_QUAD);
				field->fld_dimensions = /*FLX.RDB$DIMENSIONS*/
							jrd_43.jrd_59;
			}

			if (!/*FLX.RDB$CHARACTER_SET_ID.NULL*/
			     jrd_43.jrd_56)
				field->fld_character_set_id = /*FLX.RDB$CHARACTER_SET_ID*/
							      jrd_43.jrd_57;

			if (!/*RFR.RDB$COLLATION_ID.NULL*/
			     jrd_43.jrd_54)
				field->fld_collation_id = /*RFR.RDB$COLLATION_ID*/
							  jrd_43.jrd_55;
			else if (!/*FLX.RDB$COLLATION_ID.NULL*/
				  jrd_43.jrd_52)
				field->fld_collation_id = /*FLX.RDB$COLLATION_ID*/
							  jrd_43.jrd_53;

			if (!(/*RFR.RDB$NULL_FLAG*/
			      jrd_43.jrd_51 || /*FLX.RDB$NULL_FLAG*/
    jrd_43.jrd_50) || (relation->rel_flags & REL_view))
			{
				field->fld_flags |= FLD_nullable;
			}

			if (/*RFR.RDB$SYSTEM_FLAG*/
			    jrd_43.jrd_49 == 1 || /*FLX.RDB$SYSTEM_FLAG*/
	 jrd_43.jrd_48 == 1)
				field->fld_flags |= FLD_system;
		}

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_fields))
		DSQL_REQUEST(irq_fields) = handle3;

	if ((symbol = lookup_symbol(statement->req_dbb, strlen(name), name, SYM_relation)))
	{
		free_relation(relation);
		return (dsql_rel*) symbol->sym_object;
	}

	// Add relation to front of list

	if (permanent) {
		relation->rel_next = dbb->dbb_relations;
		dbb->dbb_relations = relation;

		// store in the symbol table unless the relation is not yet committed

		symbol = relation->rel_symbol = FB_NEW_RPT(dbb->dbb_pool, 0) dsql_sym;
		symbol->sym_object = relation;
		symbol->sym_string = relation->rel_name.c_str();
		symbol->sym_length = relation->rel_name.length();
		symbol->sym_type = SYM_relation;
		symbol->sym_dbb = dbb;
		insert_symbol(symbol);
	}
	else {
		relation->rel_flags |= REL_new_relation;
	}

	return relation;
}


bool METD_get_trigger(dsql_req* request, const dsql_str* name, dsql_str** relation, USHORT* trig_type)
{
   struct {
          TEXT  jrd_36 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_37;	// isc_utility 
          SSHORT jrd_38;	// gds__null_flag 
          SSHORT jrd_39;	// RDB$TRIGGER_TYPE 
   } jrd_35;
   struct {
          TEXT  jrd_34 [32];	// RDB$TRIGGER_NAME 
   } jrd_33;
/**************************************
 *
 *  M E T D _ g e t _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *  Look up a trigger's relation and it's current type
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(request);

	dsql_dbb* dbb = request->req_dbb;

	bool found = false;

	if (relation)
		*relation = NULL;

	jrd_req* handle = CMP_find_request(tdbb, irq_trigger, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
		X IN RDB$TRIGGERS WITH X.RDB$TRIGGER_NAME EQ name->str_data*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_32, sizeof(jrd_32), true);
	gds__vtov ((const char*) name->str_data, (char*) jrd_33.jrd_34, 32);
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_33);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 38, (UCHAR*) &jrd_35);
	   if (!jrd_35.jrd_37) break;

		if (!DSQL_REQUEST(irq_trigger))
			DSQL_REQUEST(irq_trigger) = handle;

		found = true;
		*trig_type = /*X.RDB$TRIGGER_TYPE*/
			     jrd_35.jrd_39;

		if (!/*X.RDB$RELATION_NAME.NULL*/
		     jrd_35.jrd_38)
		{
			if (relation)
			{
				fb_utils::exact_name(/*X.RDB$RELATION_NAME*/
						     jrd_35.jrd_36);
				*relation = MAKE_string(/*X.RDB$RELATION_NAME*/
							jrd_35.jrd_36, strlen(/*X.RDB$RELATION_NAME*/
	 jrd_35.jrd_36));
			}
		}

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_trigger))
		DSQL_REQUEST(irq_trigger) = handle;

	return found;
}


bool METD_get_type(dsql_req* request, const dsql_str* name, const char* field, SSHORT* value)
{
   struct {
          SSHORT jrd_30;	// isc_utility 
          SSHORT jrd_31;	// RDB$TYPE 
   } jrd_29;
   struct {
          TEXT  jrd_27 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_28 [32];	// RDB$FIELD_NAME 
   } jrd_26;
/**************************************
 *
 *  M E T D _ g e t _ t y p e
 *
 **************************************
 *
 * Functional description
 *  Look up a symbolic name in RDB$TYPES
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(request);

	dsql_dbb* dbb = request->req_dbb;
	bool found = false;

	jrd_req* handle = CMP_find_request(tdbb, irq_type, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE request->req_transaction)
		X IN RDB$TYPES WITH
		X.RDB$FIELD_NAME EQ field AND X.RDB$TYPE_NAME EQ name->str_data*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_25, sizeof(jrd_25), true);
	gds__vtov ((const char*) name->str_data, (char*) jrd_26.jrd_27, 32);
	gds__vtov ((const char*) field, (char*) jrd_26.jrd_28, 32);
	EXE_start (tdbb, handle, request->req_transaction);
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_26);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_29);
	   if (!jrd_29.jrd_30) break;;

		if (!DSQL_REQUEST(irq_type))
			DSQL_REQUEST(irq_type) = handle;

		found = true;
		*value = /*X.RDB$TYPE*/
			 jrd_29.jrd_31;

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_type))
		DSQL_REQUEST(irq_type) = handle;

	return found;
}


dsql_rel* METD_get_view_base(CompiledStatement* statement,
							 const char* view_name, // UTF-8
							 MetaNamePairMap& fields)
{
   struct {
          TEXT  jrd_11 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_12 [32];	// RDB$BASE_FIELD 
          SSHORT jrd_13;	// isc_utility 
          SSHORT jrd_14;	// gds__null_flag 
          SSHORT jrd_15;	// gds__null_flag 
   } jrd_10;
   struct {
          TEXT  jrd_9 [32];	// RDB$VIEW_NAME 
   } jrd_8;
   struct {
          TEXT  jrd_20 [32];	// RDB$VIEW_NAME 
          TEXT  jrd_21 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_22 [256];	// RDB$CONTEXT_NAME 
          SSHORT jrd_23;	// isc_utility 
          SSHORT jrd_24;	// RDB$VIEW_CONTEXT 
   } jrd_19;
   struct {
          TEXT  jrd_18 [32];	// RDB$VIEW_NAME 
   } jrd_17;
/**************************************
 *
 *  M E T D _ g e t _ v i e w _ b a s e
 *
 **************************************
 *
 * Functional description
 *  Return the base table of a view or NULL if there
 *  is more than one.
 *  If there is only one base, put in fields a map of
 *  top view field name / bottom base field name.
 *  Ignores the field in the case of a base field name
 *  appearing more than one time in a level.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(statement);

	dsql_rel* relation = NULL;

	dsql_dbb* dbb = statement->req_dbb;
	bool first = true;
	bool cont = true;
	MetaNamePairMap previousAux;

	fields.clear();

	while (cont)
	{
		jrd_req* handle1 = CMP_find_request(tdbb, irq_view_base, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE handle1 TRANSACTION_HANDLE statement->req_transaction)
			X IN RDB$VIEW_RELATIONS
			WITH X.RDB$VIEW_NAME EQ view_name*/
		{
		if (!handle1)
		   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_16, sizeof(jrd_16), true);
		gds__vtov ((const char*) view_name, (char*) jrd_17.jrd_18, 32);
		EXE_start (tdbb, handle1, statement->req_transaction);
		EXE_send (tdbb, handle1, 0, 32, (UCHAR*) &jrd_17);
		while (1)
		   {
		   EXE_receive (tdbb, handle1, 1, 324, (UCHAR*) &jrd_19);
		   if (!jrd_19.jrd_23) break;

			if (!DSQL_REQUEST(irq_view_base))
				DSQL_REQUEST(irq_view_base) = handle1;

			// return NULL if there is more than one context
			if (/*X.RDB$VIEW_CONTEXT*/
			    jrd_19.jrd_24 != 1)
			{
				relation = NULL;
				cont = false;
				EXE_unwind(tdbb, handle1);
				break;
			}

			fb_utils::exact_name(/*X.RDB$CONTEXT_NAME*/
					     jrd_19.jrd_22);
			fb_utils::exact_name(/*X.RDB$RELATION_NAME*/
					     jrd_19.jrd_21);

			relation = METD_get_relation(statement, /*X.RDB$RELATION_NAME*/
								jrd_19.jrd_21);

			Array<MetaName> ambiguities;
			MetaNamePairMap currentAux;

			if (!relation)
			{
				cont = false;
				EXE_unwind(tdbb, handle1);
				break;
			}

			jrd_req* handle2 = CMP_find_request(tdbb, irq_view_base_flds, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE handle2 TRANSACTION_HANDLE statement->req_transaction)
				RFL IN RDB$RELATION_FIELDS
				WITH RFL.RDB$RELATION_NAME EQ X.RDB$VIEW_NAME*/
			{
			if (!handle2)
			   handle2 = CMP_compile2 (tdbb, (UCHAR*) jrd_7, sizeof(jrd_7), true);
			gds__vtov ((const char*) jrd_19.jrd_20, (char*) jrd_8.jrd_9, 32);
			EXE_start (tdbb, handle2, statement->req_transaction);
			EXE_send (tdbb, handle2, 0, 32, (UCHAR*) &jrd_8);
			while (1)
			   {
			   EXE_receive (tdbb, handle2, 1, 70, (UCHAR*) &jrd_10);
			   if (!jrd_10.jrd_13) break;

				if (!DSQL_REQUEST(irq_view_base_flds))
					DSQL_REQUEST(irq_view_base_flds) = handle2;

				if (/*RFL.RDB$BASE_FIELD.NULL*/
				    jrd_10.jrd_15 || /*RFL.RDB$FIELD_NAME.NULL*/
    jrd_10.jrd_14)
					continue;

				const MetaName baseField(/*RFL.RDB$BASE_FIELD*/
							 jrd_10.jrd_12);
				if (currentAux.exist(baseField))
					ambiguities.add(baseField);
				else
				{
					const MetaName fieldName(/*RFL.RDB$FIELD_NAME*/
								 jrd_10.jrd_11);
					if (first)
					{
						fields.put(fieldName, baseField);
						currentAux.put(baseField, fieldName);
					}
					else
					{
						MetaName field;

						if (previousAux.get(fieldName, field))
						{
							fields.put(field, baseField);
							currentAux.put(baseField, field);
						}
					}
				}

			/*END_FOR*/
			   }
			}

			if (!DSQL_REQUEST(irq_view_base_flds))
				DSQL_REQUEST(irq_view_base_flds) = handle2;

			for (const MetaName* i = ambiguities.begin(); i != ambiguities.end(); ++i)
			{
				MetaName field;

				if (currentAux.get(*i, field))
				{
					currentAux.remove(*i);
					fields.remove(field);
				}
			}

			previousAux.takeOwnership(currentAux);

			if (relation->rel_flags & REL_view)
				view_name = /*X.RDB$RELATION_NAME*/
					    jrd_19.jrd_21;
			else
			{
				cont = false;
				EXE_unwind(tdbb, handle1);
				break;
			}

			first = false;

		/*END_FOR*/
		   }
		}

		if (!DSQL_REQUEST(irq_view_base))
			DSQL_REQUEST(irq_view_base) = handle1;
	}

	if (!relation)
		fields.clear();

	return relation;
}


dsql_rel* METD_get_view_relation(CompiledStatement* statement,
								const char* view_name,         // UTF-8
								const char* relation_or_alias) // UTF-8
{
   struct {
          TEXT  jrd_4 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_5 [256];	// RDB$CONTEXT_NAME 
          SSHORT jrd_6;	// isc_utility 
   } jrd_3;
   struct {
          TEXT  jrd_2 [32];	// RDB$VIEW_NAME 
   } jrd_1;
/**************************************
 *
 *  M E T D _ g e t _ v i e w _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Return TRUE if the passed view_name represents a
 *  view with the passed relation as a base table
 *  (the relation could be an alias).
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(statement);

	dsql_rel* relation = NULL;

	dsql_dbb* dbb = statement->req_dbb;

	jrd_req* handle = CMP_find_request(tdbb, irq_view, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE statement->req_transaction)
		X IN RDB$VIEW_RELATIONS WITH X.RDB$VIEW_NAME EQ view_name*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
	gds__vtov ((const char*) view_name, (char*) jrd_1.jrd_2, 32);
	EXE_start (tdbb, handle, statement->req_transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_1);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 290, (UCHAR*) &jrd_3);
	   if (!jrd_3.jrd_6) break;

		if (!DSQL_REQUEST(irq_view))
			DSQL_REQUEST(irq_view) = handle;

		fb_utils::exact_name(/*X.RDB$CONTEXT_NAME*/
				     jrd_3.jrd_5);
		fb_utils::exact_name(/*X.RDB$RELATION_NAME*/
				     jrd_3.jrd_4);

		if (!strcmp(/*X.RDB$RELATION_NAME*/
			    jrd_3.jrd_4, relation_or_alias) ||
			!strcmp(/*X.RDB$CONTEXT_NAME*/
				jrd_3.jrd_5, relation_or_alias))
		{
			relation = METD_get_relation(statement, /*X.RDB$RELATION_NAME*/
								jrd_3.jrd_4);
			EXE_unwind(tdbb, handle);
			return relation;
		}

		relation = METD_get_view_relation(statement, /*X.RDB$RELATION_NAME*/
							     jrd_3.jrd_4, relation_or_alias);
		if (relation) {
			EXE_unwind(tdbb, handle);
			return relation;
		}

	/*END_FOR*/
	   }
	}

	if (!DSQL_REQUEST(irq_view))
		DSQL_REQUEST(irq_view) = handle;

	return NULL;
}


static void convert_dtype(dsql_fld* field, SSHORT field_type)
{
/**************************************
 *
 *  c o n v e r t _ d t y p e
 *
 **************************************
 *
 * Functional description
 *  Convert from the blr_<type> stored in system metadata
 *  to the internal dtype_* descriptor.  Also set field
 *  length.
 *
 **************************************/

	// fill out the type descriptor
	switch (field_type)
	{
	case blr_text:
		field->fld_dtype = dtype_text;
		break;
	case blr_varying:
		field->fld_dtype = dtype_varying;
		field->fld_length += sizeof(USHORT);
		break;
	case blr_blob:
		field->fld_dtype = dtype_blob;
		field->fld_length = type_lengths[field->fld_dtype];
		break;
	default:
		field->fld_dtype = gds_cvt_blr_dtype[field_type];
		field->fld_length = type_lengths[field->fld_dtype];

		fb_assert(field->fld_dtype != dtype_unknown);
	}
}


static void free_procedure(dsql_prc* procedure)
{
/**************************************
 *
 *  f r e e _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *  Free memory allocated for a procedure block and params
 *
 **************************************/
	dsql_fld* param;

	// release the input & output parameter blocks

	for (param = procedure->prc_inputs; param;) {
		dsql_fld* temp = param;
		param = param->fld_next;
		delete temp;
	}

	for (param = procedure->prc_outputs; param;) {
		dsql_fld* temp = param;
		param = param->fld_next;
		delete temp;
	}

	// release the procedure & symbol blocks

	delete procedure;
}


static void free_relation(dsql_rel* relation)
{
/**************************************
 *
 *  f r e e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Free memory allocated for a relation block and fields
 *
 **************************************/

	// release the field blocks

	for (dsql_fld* field = relation->rel_fields; field;)
	{
		dsql_fld* temp = field;
		field = field->fld_next;
		delete temp;
	}

	// release the relation & symbol blocks

	delete relation;
}


static void insert_symbol(dsql_sym* symbol)
{
/**************************************
 *
 *  i n s e r t _ s y m b o l
 *
 **************************************
 *
 * Functional description
 *  Insert a symbol in the hash table and
 *  inform the engine that we're using it.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	HSHD_insert(symbol);

	MET_dsql_cache_use(tdbb, symbol->sym_type, symbol->sym_string);
}


static dsql_sym* lookup_symbol(dsql_dbb* dbb, USHORT length, const char* name,
	const SYM_TYPE type, USHORT charset_id)
{
/**************************************
 *
 *  l o o k u p _ s y m b o l
 *
 **************************************
 *
 * Functional description
 *  Lookup a symbol in the hash table and
 *  inform the engine that we're using it.
 *  The engine may inform that the symbol
 *  is obsolete, in this case mark it as
 *  dropped and return NULL.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	dsql_intlsym* intlSym = NULL;
	dsql_prc* procedure = NULL;
	dsql_rel* relation = NULL;
	dsql_udf* userFunc = NULL;

	dsql_sym* symbol = HSHD_lookup(dbb, name, length, type, 0);

	for (; symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == type)
		{
			if (type == SYM_intlsym_charset && (intlSym = (dsql_intlsym*) symbol->sym_object) &&
				!(intlSym->intlsym_flags & INTLSYM_dropped))
			{
				break;
			}
			else if (type == SYM_intlsym_collation &&
				(intlSym = (dsql_intlsym*) symbol->sym_object) &&
				!(intlSym->intlsym_flags & INTLSYM_dropped) &&
				(charset_id == 0 || intlSym->intlsym_charset_id == charset_id))
			{
				break;
			}
			else if (type == SYM_procedure && (procedure = (dsql_prc*) symbol->sym_object) &&
				!(procedure->prc_flags & PRC_dropped))
			{
				break;
			}
			else if (type == SYM_relation && (relation = (dsql_rel*) symbol->sym_object) &&
				!(relation->rel_flags & REL_dropped))
			{
				break;
			}
			else if (type == SYM_udf && (userFunc = (dsql_udf*) symbol->sym_object) &&
				!(userFunc->udf_flags & UDF_dropped))
			{
				break;
			}
		}
	}

	if (symbol)
	{
		bool obsolete = MET_dsql_cache_use(tdbb, type, name);

		if (obsolete)
		{
			switch (type)
			{
				case SYM_intlsym_charset:
				case SYM_intlsym_collation:
					intlSym->intlsym_flags |= INTLSYM_dropped;
					break;

				case SYM_procedure:
					procedure->prc_flags |= PRC_dropped;
					break;

				case SYM_relation:
					relation->rel_flags |= REL_dropped;
					break;

				case SYM_udf:
					userFunc->udf_flags |= UDF_dropped;
					break;

				default:
					return symbol;
			}

			symbol = NULL;
		}
	}

	return symbol;
}


static dsql_sym* lookup_symbol(dsql_dbb* dbb, const dsql_str* name, SYM_TYPE type, USHORT charset_id)
{
/**************************************
 *
 *  l o o k u p _ s y m b o l
 *
 **************************************
 *
 * Functional description
 *  Wrapper for the above function.
 *
 **************************************/
	return lookup_symbol(dbb, name->str_length, name->str_data, type, charset_id);
}
