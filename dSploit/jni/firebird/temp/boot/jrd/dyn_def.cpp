/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Data Definition Utility
 *	MODULE:		dyn_define.epp
 *	DESCRIPTION:	Dynamic data definition DYN_define_<x>
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
 * 23-May-2001 Claudio Valderrama - Forbid zero length identifiers,
 *                                   they are not ANSI SQL compliant.
 * 2001.10.08 Claudio Valderrama: Add case isc_dyn_system_flag to
 *	DYN_define_trigger() in order to receive values for special triggers
 *	as defined in constants.h.
 * 2001.10.08 Ann Harrison: Changed dyn_create_index so it doesn't consider
 *	simple unique indexes when finding a "referred index", but only
 * 	indexes that support unique constraints or primary keys.
 * 26-Sep-2001 Paul Beach - External File Directory Config. Parameter
 * 2002-02-24 Sean Leyne - Code Cleanup of old Win 3.1 port (WINDOWS_ONLY)
 * 2002.08.10 Dmitry Yemanov: ALTER VIEW
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2004.01.16 Vlad Horsun: added support for default parameters
 */

#include "firebird.h"
#include "../common/classes/fb_string.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
#include "../jrd/tra.h"
#include "../jrd/scl.h"
#include "../jrd/drq.h"
#include "../jrd/req.h"
#include "../jrd/flags.h"
#include "../jrd/ibase.h"
#include "../jrd/lls.h"
#include "../jrd/met.h"
#include "../jrd/btr.h"
#include "../jrd/ini.h"
#include "../jrd/intl.h"
#include "../jrd/dyn.h"
#include "../jrd/gdsassert.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/dyn_proto.h"
#include "../jrd/dyn_df_proto.h"
#include "../jrd/dyn_ut_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/gdsassert.h"
#include "../jrd/os/path_utils.h"
#include "../common/utils_proto.h"
#include "../jrd/IntlManager.h"
#include "../jrd/IntlUtil.h"

using MsgFormat::SafeArg;

using namespace Jrd;
using namespace Firebird;


typedef Firebird::ObjectsArray<Firebird::MetaName> MetaNameArray;

const int FOR_KEY_UPD_CASCADE	= 0x01;
const int FOR_KEY_UPD_NULL	= 0x02;
const int FOR_KEY_UPD_DEFAULT	= 0x04;
const int FOR_KEY_UPD_NONE	= 0x08;
const int FOR_KEY_DEL_CASCADE	= 0x10;
const int FOR_KEY_DEL_NULL	= 0x20;
const int FOR_KEY_DEL_DEFAULT	= 0x40;
const int FOR_KEY_DEL_NONE	= 0x80;

/*DATABASE DB = STATIC "ODS.RDB";*/
static const UCHAR	jrd_0 [142] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,2,0,41,3,0,32,0,41,0,0,12,0,12,
0,2,7,'C',4,'K',22,0,0,'K',4,0,1,'K',4,0,2,'K',6,0,3,'G',58,47,
24,0,1,0,25,0,1,0,58,47,24,0,2,0,25,0,0,0,58,47,24,1,0,0,24,0,
5,0,58,47,24,2,0,0,24,1,8,0,47,24,2,1,0,24,3,8,0,255,14,1,2,1,
24,3,8,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,
0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_7 [162] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,4,0,41,3,0,32,0,41,
3,0,32,0,41,0,0,12,0,41,0,0,12,0,12,0,2,7,'C',3,'K',22,0,0,'K',
6,0,1,'K',6,0,2,'G',58,57,47,24,0,1,0,25,0,3,0,47,24,0,1,0,25,
0,2,0,58,47,24,0,5,0,25,0,1,0,58,47,24,1,8,0,25,0,0,0,47,24,2,
8,0,24,0,2,0,255,14,1,2,1,24,1,8,0,25,1,0,0,1,24,2,8,0,25,1,1,
0,1,21,8,0,1,0,0,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,
0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_17 [89] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',6,
0,0,'G',58,47,24,0,8,0,25,0,0,0,59,61,24,0,16,0,255,14,1,2,1,
21,8,0,1,0,0,0,25,1,0,0,1,24,0,16,0,25,1,1,0,255,14,1,1,21,8,
0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_23 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',6,0,0,
'G',47,24,0,13,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,
0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_28 [110] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,12,0,
2,7,'C',1,'K',18,0,0,'G',57,58,47,24,0,0,0,25,0,1,0,47,24,0,6,
0,25,0,3,0,58,47,24,0,1,0,25,0,0,0,47,24,0,7,0,25,0,2,0,255,14,
1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_36 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',20,0,0,
'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_41 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',30,0,0,
'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_46 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',4,0,0,
'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_51 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',26,0,0,
'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_56 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',6,0,0,
'G',47,24,0,8,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_61 [78] =
   {	// blr string 

4,2,4,0,6,0,41,3,0,0,1,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,12,
0,15,'K',7,0,0,2,1,41,0,4,0,3,0,24,0,2,0,1,41,0,0,0,5,0,24,0,
3,0,1,25,0,1,0,24,0,1,0,1,25,0,2,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_69 [64] =
   {	// blr string 

4,2,4,0,5,0,41,3,0,32,0,41,0,0,0,4,7,0,7,0,7,0,12,0,15,'K',17,
0,0,2,1,41,0,0,0,2,0,24,0,0,0,1,41,0,1,0,3,0,24,0,2,0,1,25,0,
4,0,24,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_76 [159] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,4,0,9,0,7,0,7,0,7,0,4,1,5,0,9,0,7,0,7,0,7,
0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',12,0,0,'G',47,24,
0,0,0,25,0,0,0,255,2,14,1,2,1,24,0,11,0,41,1,0,0,2,0,1,21,8,0,
1,0,0,0,25,1,1,0,1,24,0,10,0,41,1,4,0,3,0,255,17,0,9,13,12,3,
18,0,12,2,10,0,1,2,1,41,2,0,0,3,0,24,1,11,0,1,41,2,2,0,1,0,24,
1,10,0,255,255,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_92 [169] =
   {	// blr string 

4,2,4,0,19,0,41,3,0,32,0,9,0,9,0,9,0,41,3,0,32,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',12,0,
0,2,1,25,0,0,0,24,0,0,0,1,41,0,1,0,5,0,24,0,6,0,1,41,0,2,0,6,
0,24,0,4,0,1,41,0,3,0,7,0,24,0,5,0,1,41,0,4,0,8,0,24,0,1,0,1,
41,0,10,0,9,0,24,0,9,0,1,41,0,12,0,11,0,24,0,8,0,1,41,0,14,0,
13,0,24,0,7,0,1,41,0,16,0,15,0,24,0,2,0,1,41,0,18,0,17,0,24,0,
3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_113 [76] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',10,0,0,'D',21,
8,0,1,0,0,0,'G',47,24,0,5,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,
0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_118 [311] =
   {	// blr string 

4,2,4,0,37,0,41,3,0,32,0,41,0,0,128,0,9,0,9,0,9,0,9,0,9,0,9,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,
'K',2,0,0,2,1,25,0,8,0,24,0,10,0,1,25,0,9,0,24,0,8,0,1,25,0,0,
0,24,0,0,0,1,41,0,11,0,10,0,24,0,25,0,1,41,0,13,0,12,0,24,0,26,
0,1,41,0,15,0,14,0,24,0,24,0,1,41,0,17,0,16,0,24,0,22,0,1,41,
0,1,0,18,0,24,0,18,0,1,41,0,20,0,19,0,24,0,23,0,1,41,0,2,0,21,
0,24,0,3,0,1,41,0,3,0,22,0,24,0,2,0,1,41,0,4,0,23,0,24,0,7,0,
1,41,0,5,0,24,0,24,0,6,0,1,41,0,6,0,25,0,24,0,5,0,1,41,0,7,0,
26,0,24,0,4,0,1,41,0,28,0,27,0,24,0,17,0,1,41,0,30,0,29,0,24,
0,11,0,1,41,0,32,0,31,0,24,0,27,0,1,41,0,34,0,33,0,24,0,9,0,1,
41,0,'$',0,35,0,24,0,15,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_157 [248] =
   {	// blr string 

4,2,4,0,27,0,41,3,0,32,0,9,0,9,0,41,3,0,32,0,41,0,0,128,0,9,0,
41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',5,0,
0,2,1,25,0,0,0,24,0,2,0,1,41,0,10,0,9,0,24,0,18,0,1,41,0,1,0,
11,0,24,0,12,0,1,41,0,2,0,12,0,24,0,17,0,1,41,0,14,0,13,0,24,
0,16,0,1,41,0,16,0,15,0,24,0,8,0,1,41,0,3,0,17,0,24,0,4,0,1,41,
0,19,0,18,0,24,0,10,0,1,41,0,21,0,20,0,24,0,6,0,1,41,0,4,0,22,
0,24,0,5,0,1,41,0,5,0,23,0,24,0,7,0,1,41,0,6,0,24,0,24,0,3,0,
1,41,0,26,0,25,0,24,0,13,0,1,25,0,7,0,24,0,1,0,1,25,0,8,0,24,
0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_186 [61] =
   {	// blr string 

4,2,4,0,5,0,9,0,9,0,41,3,0,32,0,7,0,7,0,12,0,15,'K',9,0,0,2,1,
41,0,0,0,3,0,24,0,2,0,1,41,0,1,0,4,0,24,0,1,0,1,25,0,2,0,24,0,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_193 [60] =
   {	// blr string 

4,2,4,0,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,12,0,15,'K',31,0,
0,2,1,41,0,3,0,2,0,24,0,3,0,1,25,0,0,0,24,0,1,0,1,25,0,1,0,24,
0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_199 [45] =
   {	// blr string 

4,2,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,15,'K',31,0,0,2,1,25,
0,0,0,24,0,1,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_203 [92] =
   {	// blr string 

4,2,4,0,6,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,41,0,0,7,0,12,
0,15,'K',18,0,0,2,1,25,0,2,0,24,0,3,0,1,25,0,5,0,24,0,2,0,1,25,
0,3,0,24,0,7,0,1,25,0,4,0,24,0,6,0,1,25,0,0,0,24,0,0,0,1,25,0,
1,0,24,0,4,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_211 [129] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,7,0,7,0,4,1,3,0,7,0,7,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',6,0,0,'G',47,24,0,8,0,25,0,0,0,255,
2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,16,0,41,1,2,0,1,0,255,
17,0,9,13,12,3,18,0,12,2,10,0,1,2,1,41,2,1,0,0,0,24,1,16,0,255,
255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_223 [113] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',2,'K',7,0,0,'K',6,0,1,'G',58,47,24,1,8,0,24,0,1,0,47,
24,0,0,0,25,0,0,0,255,14,1,2,1,24,1,8,0,25,1,0,0,1,24,1,13,0,
25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,
25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_230 [142] =
   {	// blr string 

4,2,4,0,15,0,41,0,0,0,1,9,0,41,3,0,32,0,9,0,9,0,41,3,0,32,0,7,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',6,0,0,2,1,41,0,
7,0,6,0,24,0,15,0,1,41,0,0,0,8,0,24,0,10,0,1,41,0,1,0,9,0,24,
0,2,0,1,41,0,2,0,10,0,24,0,9,0,1,41,0,3,0,11,0,24,0,1,0,1,41,
0,4,0,12,0,24,0,0,0,1,41,0,14,0,13,0,24,0,4,0,1,25,0,5,0,24,0,
8,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_247 [81] =
   {	// blr string 

4,2,4,0,5,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,41,0,0,7,0,12,0,15,
'K',18,0,0,2,1,25,0,4,0,24,0,2,0,1,25,0,2,0,24,0,7,0,1,25,0,3,
0,24,0,6,0,1,25,0,0,0,24,0,0,0,1,25,0,1,0,24,0,4,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_254 [189] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,6,0,9,0,7,0,7,0,7,0,7,0,7,0,4,1,7,0,9,0,7,
0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
26,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,1,24,0,13,0,41,1,
0,0,2,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,12,0,41,1,4,0,3,0,1,
24,0,11,0,41,1,6,0,5,0,255,17,0,9,13,12,3,18,0,12,2,10,0,1,2,
1,41,2,0,0,5,0,24,1,13,0,1,41,2,4,0,3,0,24,1,12,0,1,41,2,2,0,
1,0,24,1,11,0,255,255,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_274 [150] =
   {	// blr string 

4,2,4,0,16,0,9,0,41,3,0,32,0,9,0,9,0,41,3,0,32,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',26,0,0,2,1,41,0,6,
0,5,0,24,0,10,0,1,41,0,8,0,7,0,24,0,3,0,1,41,0,10,0,9,0,24,0,
2,0,1,41,0,0,0,11,0,24,0,4,0,1,41,0,1,0,12,0,24,0,7,0,1,41,0,
2,0,13,0,24,0,5,0,1,41,0,3,0,14,0,24,0,6,0,1,25,0,4,0,24,0,0,
0,1,25,0,15,0,24,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_292 [186] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,4,1,5,
0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,4,0,2,0,41,3,0,32,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',27,0,0,'G',58,47,24,0,1,0,25,0,1,
0,47,24,0,0,0,25,0,0,0,255,2,14,1,2,1,24,0,12,0,41,1,0,0,3,0,
1,24,0,13,0,41,1,1,0,4,0,1,21,8,0,1,0,0,0,25,1,2,0,255,17,0,9,
13,12,3,18,0,12,2,10,0,1,2,1,41,2,1,0,3,0,24,1,12,0,1,41,2,0,
0,2,0,24,1,13,0,255,255,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_309 [264] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,10,0,9,0,9,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,4,1,11,0,9,0,9,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,
2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,'C',1,'K',27,0,0,'G',58,
47,24,0,1,0,25,0,1,0,47,24,0,0,0,25,0,0,0,255,2,14,1,2,1,24,0,
8,0,41,1,0,0,7,0,1,24,0,7,0,41,1,1,0,8,0,1,21,8,0,1,0,0,0,25,
1,2,0,1,24,0,11,0,41,1,4,0,3,0,1,24,0,10,0,41,1,6,0,5,0,1,24,
0,9,0,41,1,10,0,9,0,255,17,0,9,13,12,3,18,0,12,2,10,0,1,2,1,41,
2,9,0,8,0,24,1,11,0,1,41,2,7,0,6,0,24,1,10,0,1,41,2,1,0,5,0,24,
1,8,0,1,41,2,0,0,4,0,24,1,7,0,1,41,2,3,0,2,0,24,1,9,0,255,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_338 [203] =
   {	// blr string 

4,2,4,0,23,0,9,0,9,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',
2,0,0,2,1,41,0,0,0,3,0,24,0,7,0,1,41,0,1,0,4,0,24,0,6,0,1,41,
0,6,0,5,0,24,0,25,0,1,41,0,8,0,7,0,24,0,26,0,1,41,0,10,0,9,0,
24,0,24,0,1,41,0,12,0,11,0,24,0,17,0,1,41,0,14,0,13,0,24,0,27,
0,1,41,0,16,0,15,0,24,0,9,0,1,41,0,18,0,17,0,24,0,11,0,1,25,0,
19,0,24,0,10,0,1,25,0,20,0,24,0,8,0,1,25,0,2,0,24,0,0,0,1,41,
0,22,0,21,0,24,0,15,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_363 [142] =
   {	// blr string 

4,2,4,0,15,0,9,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',27,0,0,2,1,41,
0,5,0,4,0,24,0,10,0,1,41,0,0,0,6,0,24,0,5,0,1,41,0,8,0,7,0,24,
0,6,0,1,41,0,1,0,9,0,24,0,4,0,1,41,0,11,0,10,0,24,0,3,0,1,41,
0,13,0,12,0,24,0,2,0,1,41,0,2,0,14,0,24,0,1,0,1,25,0,3,0,24,0,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_380 [188] =
   {	// blr string 

4,2,4,0,21,0,9,0,9,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',2,0,0,
2,1,41,0,4,0,3,0,24,0,26,0,1,41,0,6,0,5,0,24,0,27,0,1,41,0,8,
0,7,0,24,0,9,0,1,41,0,10,0,9,0,24,0,17,0,1,41,0,12,0,11,0,24,
0,24,0,1,41,0,14,0,13,0,24,0,11,0,1,41,0,16,0,15,0,24,0,8,0,1,
41,0,18,0,17,0,24,0,10,0,1,25,0,0,0,24,0,5,0,1,25,0,1,0,24,0,
4,0,1,25,0,2,0,24,0,0,0,1,41,0,20,0,19,0,24,0,15,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_403 [285] =
   {	// blr string 

4,2,4,0,32,0,41,0,0,128,0,9,0,9,0,9,0,41,3,0,32,0,9,0,41,3,0,
32,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,
0,7,0,7,0,7,0,12,0,15,'K',5,0,0,2,1,41,0,12,0,11,0,24,0,18,0,
1,41,0,0,0,13,0,24,0,5,0,1,41,0,1,0,14,0,24,0,17,0,1,41,0,2,0,
15,0,24,0,12,0,1,41,0,3,0,16,0,24,0,11,0,1,41,0,4,0,17,0,24,0,
14,0,1,41,0,5,0,18,0,24,0,7,0,1,41,0,6,0,19,0,24,0,3,0,1,41,0,
21,0,20,0,24,0,10,0,1,41,0,23,0,22,0,24,0,6,0,1,41,0,25,0,24,
0,24,0,8,0,1,41,0,7,0,26,0,24,0,4,0,1,41,0,28,0,27,0,24,0,16,
0,1,41,0,30,0,29,0,24,0,13,0,1,41,0,8,0,31,0,24,0,1,0,1,25,0,
9,0,24,0,2,0,1,25,0,10,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_437 [125] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,2,0,41,3,0,32,0,41,0,0,12,
0,12,0,2,7,'C',2,'K',4,0,0,'K',22,0,1,'G',58,47,24,1,5,0,24,0,
0,0,58,47,24,0,1,0,25,0,0,0,47,24,1,1,0,25,0,1,0,255,14,1,2,1,
24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,5,0,25,1,2,
0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_445 [86] =
   {	// blr string 

4,2,4,1,3,0,9,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
6,0,0,'G',47,24,0,8,0,25,0,0,0,255,14,1,2,1,24,0,0,0,41,1,0,0,
2,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_452 [176] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,3,0,41,3,0,32,0,41,
0,0,12,0,41,0,0,12,0,12,0,2,7,'C',3,'K',22,0,0,'K',4,0,1,'K',
3,0,2,'G',58,58,47,24,1,0,0,24,0,5,0,47,24,2,0,0,24,1,0,0,58,
47,24,1,1,0,25,0,0,0,58,59,61,24,1,3,0,57,47,24,0,1,0,25,0,2,
0,47,24,0,1,0,25,0,1,0,'F',2,'H',24,1,0,0,'I',24,2,2,0,255,14,
1,2,1,24,2,1,0,25,1,0,0,1,24,1,0,0,25,1,1,0,1,21,8,0,1,0,0,0,
25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_461 [56] =
   {	// blr string 

4,2,4,0,3,0,41,3,0,32,0,41,3,0,32,0,7,0,12,0,15,'K',3,0,0,2,1,
25,0,2,0,24,0,2,0,1,25,0,0,0,24,0,1,0,1,25,0,1,0,24,0,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_466 [193] =
   {	// blr string 

4,2,4,1,12,0,9,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,'C',2,'K',5,0,0,'K',
2,0,1,'G',58,47,24,1,0,0,24,0,2,0,58,47,24,0,0,0,25,0,1,0,47,
25,0,0,0,24,0,1,0,255,14,1,2,1,24,1,4,0,41,1,0,0,8,0,1,21,8,0,
1,0,0,0,25,1,1,0,1,24,1,25,0,41,1,3,0,2,0,1,24,1,8,0,25,1,4,0,
1,24,1,26,0,25,1,5,0,1,24,0,18,0,41,1,7,0,6,0,1,24,1,22,0,41,
1,10,0,9,0,1,24,1,10,0,25,1,11,0,255,14,1,1,21,8,0,0,0,0,0,25,
1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_483 [86] =
   {	// blr string 

4,2,4,1,3,0,9,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
6,0,0,'G',47,24,0,8,0,25,0,0,0,255,14,1,2,1,24,0,0,0,41,1,0,0,
2,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_490 [183] =
   {	// blr string 

4,2,4,0,20,0,41,3,0,32,0,41,3,0,32,0,9,0,9,0,41,3,0,32,0,9,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,
15,'K',4,0,0,2,1,25,0,6,0,24,0,5,0,1,41,0,8,0,7,0,24,0,9,0,1,
41,0,0,0,9,0,24,0,1,0,1,25,0,1,0,24,0,0,0,1,41,0,2,0,10,0,24,
0,10,0,1,41,0,3,0,11,0,24,0,11,0,1,41,0,4,0,12,0,24,0,8,0,1,41,
0,5,0,13,0,24,0,4,0,1,41,0,15,0,14,0,24,0,7,0,1,41,0,17,0,16,
0,24,0,6,0,1,41,0,19,0,18,0,24,0,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_512 [378] =
   {	// blr string 

4,2,4,0,46,0,9,0,9,0,9,0,9,0,9,0,9,0,9,0,9,0,41,0,0,128,0,9,0,
41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',2,0,0,2,1,25,
0,12,0,24,0,10,0,1,41,0,14,0,13,0,24,0,8,0,1,41,0,16,0,15,0,24,
0,27,0,1,41,0,18,0,17,0,24,0,25,0,1,41,0,20,0,19,0,24,0,26,0,
1,41,0,22,0,21,0,24,0,23,0,1,41,0,24,0,23,0,24,0,24,0,1,41,0,
26,0,25,0,24,0,22,0,1,41,0,0,0,27,0,24,0,14,0,1,41,0,1,0,28,0,
24,0,3,0,1,41,0,2,0,29,0,24,0,2,0,1,41,0,3,0,30,0,24,0,7,0,1,
41,0,4,0,31,0,24,0,6,0,1,41,0,5,0,32,0,24,0,5,0,1,41,0,6,0,33,
0,24,0,4,0,1,41,0,7,0,34,0,24,0,12,0,1,41,0,8,0,35,0,24,0,18,
0,1,41,0,9,0,'$',0,24,0,16,0,1,41,0,10,0,37,0,24,0,1,0,1,41,0,
39,0,38,0,24,0,17,0,1,41,0,41,0,40,0,24,0,11,0,1,41,0,43,0,42,
0,24,0,9,0,1,41,0,45,0,44,0,24,0,15,0,1,25,0,11,0,24,0,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_560 [57] =
   {	// blr string 

4,2,4,0,4,0,41,3,0,32,0,7,0,7,0,7,0,12,0,15,'K',20,0,0,2,1,41,
0,2,0,1,0,24,0,2,0,1,25,0,0,0,24,0,0,0,1,25,0,3,0,24,0,1,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_566 [166] =
   {	// blr string 

4,2,4,0,19,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',15,0,0,2,1,41,
0,2,0,1,0,24,0,9,0,1,41,0,4,0,3,0,24,0,8,0,1,41,0,6,0,5,0,24,
0,7,0,1,41,0,8,0,7,0,24,0,6,0,1,41,0,10,0,9,0,24,0,5,0,1,41,0,
12,0,11,0,24,0,4,0,1,41,0,14,0,13,0,24,0,3,0,1,41,0,16,0,15,0,
24,0,2,0,1,41,0,0,0,17,0,24,0,0,0,1,25,0,18,0,24,0,1,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_587 [130] =
   {	// blr string 

4,2,4,0,13,0,9,0,41,0,0,32,0,41,0,0,0,1,41,3,0,32,0,41,3,0,32,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',14,0,0,2,1,41,0,
6,0,5,0,24,0,7,0,1,41,0,0,0,7,0,24,0,3,0,1,41,0,1,0,8,0,24,0,
5,0,1,41,0,2,0,9,0,24,0,4,0,1,41,0,3,0,10,0,24,0,2,0,1,41,0,12,
0,11,0,24,0,6,0,1,25,0,4,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_602 [127] =
   {	// blr string 

4,2,4,0,13,0,9,0,41,0,0,32,0,41,0,0,0,1,41,3,0,32,0,7,0,7,0,7,
0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',16,0,0,2,1,41,0,5,0,4,0,
24,0,6,0,1,41,0,0,0,6,0,24,0,1,0,1,41,0,1,0,7,0,24,0,3,0,1,41,
0,2,0,8,0,24,0,2,0,1,41,0,10,0,9,0,24,0,4,0,1,41,0,12,0,11,0,
24,0,5,0,1,25,0,3,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_617 [91] =
   {	// blr string 

4,2,4,0,9,0,41,0,0,0,1,8,0,8,0,7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,
'K',10,0,0,2,1,41,0,4,0,3,0,24,0,5,0,1,41,0,1,0,5,0,24,0,3,0,
1,41,0,2,0,6,0,24,0,2,0,1,41,0,8,0,7,0,24,0,4,0,1,25,0,0,0,24,
0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_628 [61] =
   {	// blr string 

4,2,4,0,2,0,7,0,7,0,2,7,'C',1,'K',10,0,0,255,14,0,2,1,21,8,0,
1,0,0,0,25,0,0,0,1,24,0,4,0,25,0,1,0,255,14,0,1,21,8,0,0,0,0,
0,25,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_632 [87] =
   {	// blr string 

4,2,4,0,8,0,41,0,0,0,1,8,0,8,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',
10,0,0,2,1,41,0,1,0,3,0,24,0,3,0,1,41,0,2,0,4,0,24,0,2,0,1,41,
0,6,0,5,0,24,0,4,0,1,25,0,7,0,24,0,5,0,1,25,0,0,0,24,0,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_642 [79] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,0,0,0,1,12,0,2,7,'C',1,'K',10,0,0,
'D',21,8,0,1,0,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_647 [75] =
   {	// blr string 

4,2,4,0,6,0,41,0,0,0,4,41,3,0,32,0,8,0,7,0,7,0,7,0,12,0,15,'K',
30,0,0,2,1,41,0,0,0,3,0,24,0,2,0,1,41,0,5,0,4,0,24,0,4,0,1,25,
0,1,0,24,0,0,0,1,25,0,2,0,24,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_655 [72] =
   {	// blr string 

4,2,4,0,6,0,41,3,0,32,0,8,0,8,0,7,0,7,0,7,0,12,0,15,'K',21,0,
0,2,1,25,0,0,0,24,0,0,0,1,25,0,3,0,24,0,1,0,1,41,0,1,0,4,0,24,
0,2,0,1,41,0,2,0,5,0,24,0,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_663 [115] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,3,0,41,3,0,32,0,41,0,0,12,0,41,
0,0,12,0,12,0,2,7,'C',1,'K',22,0,0,'G',58,47,24,0,5,0,25,0,0,
0,57,47,24,0,1,0,25,0,2,0,47,24,0,1,0,25,0,1,0,255,14,1,2,1,24,
0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,
0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_671 [81] =
   {	// blr string 

4,2,4,0,6,0,41,3,0,32,0,41,3,0,32,0,41,0,0,12,0,41,0,0,12,0,7,
0,7,0,12,0,15,'K',23,0,0,2,1,41,0,2,0,4,0,24,0,4,0,1,41,0,3,0,
5,0,24,0,3,0,1,25,0,0,0,24,0,0,0,1,25,0,1,0,24,0,1,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_679 [170] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,4,0,41,3,0,32,0,41,
3,0,32,0,41,0,0,12,0,41,0,0,12,0,12,0,2,7,'C',2,'K',22,0,0,'K',
3,0,1,'G',58,47,24,1,0,0,24,0,5,0,58,47,24,0,2,0,25,0,1,0,58,
57,47,24,0,1,0,25,0,3,0,47,24,0,1,0,25,0,2,0,48,24,0,0,0,25,0,
0,0,'F',2,'H',24,0,5,0,'I',24,1,1,0,255,14,1,2,1,24,1,1,0,25,
1,0,0,1,24,0,5,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,255,14,1,
1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_689 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',3,0,0,
'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_694 [193] =
   {	// blr string 

4,2,4,1,7,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,4,0,2,
0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,'C',3,'K',3,0,0,'K',5,0,1,
'K',2,0,2,'G',58,47,24,0,0,0,25,0,1,0,58,47,24,1,1,0,25,0,0,0,
58,47,24,1,0,0,24,0,1,0,47,24,2,0,0,24,1,2,0,'F',1,'H',24,0,1,
0,'E',3,24,0,1,0,24,0,0,0,24,2,23,0,255,14,1,2,1,24,0,1,0,25,
1,0,0,1,24,1,0,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,1,24,1,16,
0,41,1,4,0,3,0,1,24,2,23,0,41,1,6,0,5,0,255,14,1,1,21,8,0,0,0,
0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_706 [77] =
   {	// blr string 

4,2,4,0,5,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,41,0,0,12,0,7,
0,12,0,15,'K',22,0,0,2,1,41,0,0,0,4,0,24,0,5,0,1,25,0,3,0,24,
0,1,0,1,25,0,1,0,24,0,2,0,1,25,0,2,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_713 [82] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',28,
0,0,'G',47,24,0,4,0,25,0,0,0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,
21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_719 [105] =
   {	// blr string 

4,2,4,0,10,0,41,3,0,32,0,41,3,0,32,0,9,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,12,0,15,'K',29,0,0,2,1,25,0,3,0,24,0,3,0,1,25,0,0,0,24,
0,0,0,1,41,0,5,0,4,0,24,0,2,0,1,41,0,1,0,6,0,24,0,7,0,1,41,0,
2,0,7,0,24,0,8,0,1,41,0,9,0,8,0,24,0,4,0,255,255,76
   };	// end of blr string 



static const UCHAR who_blr[] =
{
	blr_version5,
	blr_begin,
	blr_message, 0, 1, 0,
	blr_cstring2, 3, 0, 32, 0,
	blr_begin,
	blr_send, 0,
	blr_begin,
	blr_assignment,
	blr_user_name,
	blr_parameter, 0, 0, 0,
	blr_end,
	blr_end,
	blr_end,
	blr_eoc
};


static void check_unique_name(thread_db*, Global*, const Firebird::MetaName&, int);
static bool get_who(thread_db*, Global*, Firebird::MetaName&);
static bool is_it_user_name(Global*, const Firebird::MetaName&, thread_db*);

static rel_t get_relation_type(thread_db*, Global*, const Firebird::MetaName&);
static void check_foreign_key_temp_scope(thread_db*, Global*, const TEXT*, const TEXT*);
static void check_relation_temp_scope(thread_db*, Global*, const TEXT*, const rel_t);
static void make_relation_scope_name(const TEXT*, const rel_t, Firebird::string&);


void DYN_define_collation( Global* gbl, const UCHAR** ptr)
{
   struct {
          TEXT  jrd_717 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_718;	// gds__utility 
   } jrd_716;
   struct {
          SSHORT jrd_715;	// RDB$CHARACTER_SET_ID 
   } jrd_714;
   struct {
          TEXT  jrd_721 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_722 [32];	// RDB$BASE_COLLATION_NAME 
          bid  jrd_723;	// RDB$SPECIFIC_ATTRIBUTES 
          SSHORT jrd_724;	// RDB$COLLATION_ATTRIBUTES 
          SSHORT jrd_725;	// gds__null_flag 
          SSHORT jrd_726;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_727;	// gds__null_flag 
          SSHORT jrd_728;	// gds__null_flag 
          SSHORT jrd_729;	// gds__null_flag 
          SSHORT jrd_730;	// RDB$SYSTEM_FLAG 
   } jrd_720;
/**************************************
 *
 *	D Y N _ d e f i n e _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Define a collation.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	const USHORT major_version  = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	if (ENCODE_ODS(major_version, minor_original) < ODS_11_0)
	{
		DYN_error_punt(false, 220);
		// msg 220: "CREATE COLLATION statement is not supported in older versions of the database.
		//			 A backup and restore is required."
	}

	Firebird::MetaName collation_name;
	Firebird::MetaName charsetName;

	GET_STRING(ptr, collation_name);

	if (!collation_name.length())
		DYN_error_punt(false, 212);
	/* msg 212: "Zero length identifiers not allowed" */

	jrd_req* request = CMP_find_request(tdbb, drq_s_colls, DYN_REQUESTS);

	bool b_ending_store = false;

	try
	{
		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			X IN RDB$COLLATIONS*/
		{
		

			CharSet* cs = NULL;
			SubtypeInfo info;
			USHORT attributes_on = 0;
			USHORT attributes_off = 0;
			SSHORT specific_attributes_charset = CS_NONE;
			Firebird::UCharBuffer specific_attributes;

			/*X.RDB$SYSTEM_FLAG*/
			jrd_720.jrd_730 = 0;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_720.jrd_729 = FALSE;
			/*X.RDB$SPECIFIC_ATTRIBUTES.NULL*/
			jrd_720.jrd_728 = TRUE;
			/*X.RDB$BASE_COLLATION_NAME.NULL*/
			jrd_720.jrd_727 = TRUE;

			while (**ptr != isc_dyn_end)
			{
				switch (*(*ptr)++)
				{
					case isc_dyn_coll_for_charset:
					{
						/*X.RDB$CHARACTER_SET_ID.NULL*/
						jrd_720.jrd_725 = FALSE;
						/*X.RDB$CHARACTER_SET_ID*/
						jrd_720.jrd_726 = DYN_get_number(ptr);

						cs = INTL_charset_lookup(tdbb, /*X.RDB$CHARACTER_SET_ID*/
									       jrd_720.jrd_726);

						jrd_req* request2 = CMP_find_request(tdbb, drq_l_charset, DYN_REQUESTS);

						/*FOR(REQUEST_HANDLE request2
							TRANSACTION_HANDLE gbl->gbl_transaction)
							CS IN RDB$CHARACTER_SETS
							WITH CS.RDB$CHARACTER_SET_ID EQ X.RDB$CHARACTER_SET_ID*/
						{
						if (!request2)
						   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_713, sizeof(jrd_713), true);
						jrd_714.jrd_715 = jrd_720.jrd_726;
						EXE_start (tdbb, request2, gbl->gbl_transaction);
						EXE_send (tdbb, request2, 0, 2, (UCHAR*) &jrd_714);
						while (1)
						   {
						   EXE_receive (tdbb, request2, 1, 34, (UCHAR*) &jrd_716);
						   if (!jrd_716.jrd_718) break;

							if (!DYN_REQUEST(drq_l_charset))
								DYN_REQUEST(drq_l_charset) = request2;

							charsetName = /*CS.RDB$CHARACTER_SET_NAME*/
								      jrd_716.jrd_717;

						/*END_FOR*/
						   }
						}

						if (!DYN_REQUEST(drq_l_charset))
							DYN_REQUEST(drq_l_charset) = request2;

						break;
					}

					case isc_dyn_coll_from:
						if (!MET_get_char_coll_subtype_info(tdbb, DYN_get_number(ptr), &info))
							break;

						fb_assert(cs);
						if (cs)
						{
							if (info.specificAttributes.getCount() != 0)
							{
								Firebird::UCharBuffer temp;
								ULONG size = info.specificAttributes.getCount() * cs->maxBytesPerChar();

								size = INTL_convert_bytes(tdbb, /*X.RDB$CHARACTER_SET_ID*/
												jrd_720.jrd_726,
														temp.getBuffer(size), size,
														CS_METADATA, info.specificAttributes.begin(),
														info.specificAttributes.getCount(), ERR_post);
								temp.shrink(size);
								info.specificAttributes = temp;
							}
						}

						/*X.RDB$BASE_COLLATION_NAME.NULL*/
						jrd_720.jrd_727 = FALSE;
						strcpy(/*X.RDB$BASE_COLLATION_NAME*/
						       jrd_720.jrd_722, info.baseCollationName.c_str());

						break;

					case isc_dyn_coll_from_external:
						GET_STRING(ptr, /*X.RDB$BASE_COLLATION_NAME*/
								jrd_720.jrd_722);
						/*X.RDB$BASE_COLLATION_NAME.NULL*/
						jrd_720.jrd_727 = FALSE;
						break;

					case isc_dyn_coll_attribute:
					{
						const SSHORT attr = DYN_get_number(ptr);

						if (attr >= 0)
						{
							attributes_on |= attr;
							attributes_off &= ~attr;
						}
						else
						{
							attributes_on &= ~(-attr);
							attributes_off |= -attr;
						}

						break;
					}

					// ASF: Our DDL strings is very weak.
					// I've added isc_dyn_coll_specific_attributes_charset to pass the character set of a string.
					// It may be the connection charset or some charset specified with INTRODUCER.
					// Better approach is to pass DYN strings (including delimited identifiers) with the
					// charset and reading it converting to CS_METADATA.
					case isc_dyn_coll_specific_attributes_charset:
						specific_attributes_charset = DYN_get_number(ptr);
						break;

					case isc_dyn_coll_specific_attributes:
						GET_STRING(ptr, specific_attributes);

						fb_assert(cs);
						if (cs)
						{
							if (specific_attributes.getCount() != 0)
							{
								Firebird::UCharBuffer temp;
								ULONG size = specific_attributes.getCount() * cs->maxBytesPerChar();

								size = INTL_convert_bytes(tdbb, /*X.RDB$CHARACTER_SET_ID*/
												jrd_720.jrd_726,
														temp.getBuffer(size), size,
														specific_attributes_charset, specific_attributes.begin(),
														specific_attributes.getCount(), ERR_post);
								temp.shrink(size);
								specific_attributes = temp;
							}
						}

						break;

					default:
						DYN_unsupported_verb();
				}
			}

			strcpy(/*X.RDB$COLLATION_NAME*/
			       jrd_720.jrd_721, collation_name.c_str());

			info.charsetName = charsetName.c_str();
			info.collationName = /*X.RDB$COLLATION_NAME*/
					     jrd_720.jrd_721;
			if (/*X.RDB$BASE_COLLATION_NAME.NULL*/
			    jrd_720.jrd_727)
				info.baseCollationName = info.collationName;
			else
				info.baseCollationName = /*X.RDB$BASE_COLLATION_NAME*/
							 jrd_720.jrd_722;
			info.ignoreAttributes = false;

			if (!IntlManager::collationInstalled(info.baseCollationName.c_str(),
					info.charsetName.c_str()))
			{
				DYN_error_punt(false, 223,
					SafeArg() << info.baseCollationName.c_str() << info.charsetName.c_str());
				// msg: 223: "Collation @1 not installed for character set @2"
			}

			fb_assert(cs);
			if (cs)
			{
				Firebird::IntlUtil::SpecificAttributesMap map;

				if (!Firebird::IntlUtil::parseSpecificAttributes(
						cs, info.specificAttributes.getCount(), info.specificAttributes.begin(), &map) ||
					!Firebird::IntlUtil::parseSpecificAttributes(
						cs, specific_attributes.getCount(), specific_attributes.begin(), &map))
				{
					DYN_error_punt(false, 222);
					// msg: 222: "Invalid collation attributes"
				}

				const Firebird::string s = Firebird::IntlUtil::generateSpecificAttributes(cs, map);
				Firebird::string newSpecificAttributes;

				if (!IntlManager::setupCollationAttributes(
						info.baseCollationName.c_str(), info.charsetName.c_str(), s,
						newSpecificAttributes))
				{
					DYN_error_punt(false, 222);
					// msg: 222: "Invalid collation attributes"
				}

				memcpy(info.specificAttributes.getBuffer(newSpecificAttributes.length()),
					newSpecificAttributes.begin(), newSpecificAttributes.length());
			}

			info.attributes = (info.attributes | attributes_on) & (~attributes_off);
			/*X.RDB$COLLATION_ATTRIBUTES*/
			jrd_720.jrd_724 = info.attributes;

			if (info.specificAttributes.getCount() != 0)
			{
				/*X.RDB$SPECIFIC_ATTRIBUTES.NULL*/
				jrd_720.jrd_728 = FALSE;

				UCHAR bpb[] = {isc_bpb_version1,
							   isc_bpb_source_type, 1, isc_blob_text, isc_bpb_source_interp, 1, 0,
							   isc_bpb_target_type, 1, isc_blob_text, isc_bpb_target_interp, 1, 0};

				bpb[6] = /*X.RDB$CHARACTER_SET_ID*/
					 jrd_720.jrd_726;	// from charset
				bpb[12] = CS_METADATA;				// to charset

				blb* blob = BLB_create2(tdbb, gbl->gbl_transaction, &/*X.RDB$SPECIFIC_ATTRIBUTES*/
										     jrd_720.jrd_723,
										sizeof(bpb), bpb);
				BLB_put_segment(tdbb, blob, info.specificAttributes.begin(),
								info.specificAttributes.getCount());
				BLB_close(tdbb, blob);
			}

			// Do not allow invalid attributes here.
			if (!INTL_texttype_validate(tdbb, &info))
			{
				DYN_error_punt(false, 222);
				// msg: 222: "Invalid collation attributes"
			}

			b_ending_store = true;

		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_719, sizeof(jrd_719), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 86, (UCHAR*) &jrd_720);
		}

		if (!DYN_REQUEST(drq_s_colls))
			DYN_REQUEST(drq_s_colls) = request;
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_rundown_request(request, drq_s_colls);
			DYN_error_punt(true, 219);
			// msg 219: "DEFINE COLLATION failed"
		}
		throw;
	}
}


void DYN_define_constraint(Global*		gbl,
						   const UCHAR**	ptr,
						   const Firebird::MetaName*	relation_name,
						   Firebird::MetaName*	field_name)
{
   struct {
          TEXT  jrd_669 [32];	// RDB$CONSTRAINT_NAME 
          SSHORT jrd_670;	// gds__utility 
   } jrd_668;
   struct {
          TEXT  jrd_665 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_666 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_667 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_664;
   struct {
          TEXT  jrd_673 [32];	// RDB$CONSTRAINT_NAME 
          TEXT  jrd_674 [32];	// RDB$CONST_NAME_UQ 
          TEXT  jrd_675 [12];	// RDB$DELETE_RULE 
          TEXT  jrd_676 [12];	// RDB$UPDATE_RULE 
          SSHORT jrd_677;	// gds__null_flag 
          SSHORT jrd_678;	// gds__null_flag 
   } jrd_672;
   struct {
          TEXT  jrd_686 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_687 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_688;	// gds__utility 
   } jrd_685;
   struct {
          TEXT  jrd_681 [32];	// RDB$CONSTRAINT_NAME 
          TEXT  jrd_682 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_683 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_684 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_680;
   struct {
          SSHORT jrd_693;	// gds__utility 
   } jrd_692;
   struct {
          TEXT  jrd_691 [32];	// RDB$INDEX_NAME 
   } jrd_690;
   struct {
          TEXT  jrd_699 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_700 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_701;	// gds__utility 
          SSHORT jrd_702;	// gds__null_flag 
          SSHORT jrd_703;	// RDB$NULL_FLAG 
          SSHORT jrd_704;	// gds__null_flag 
          SSHORT jrd_705;	// RDB$NULL_FLAG 
   } jrd_698;
   struct {
          TEXT  jrd_696 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_697 [32];	// RDB$INDEX_NAME 
   } jrd_695;
   struct {
          TEXT  jrd_708 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_709 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_710 [32];	// RDB$CONSTRAINT_NAME 
          TEXT  jrd_711 [12];	// RDB$CONSTRAINT_TYPE 
          SSHORT jrd_712;	// gds__null_flag 
   } jrd_707;
/**************************************
 *
 *	D Y N _ d e f i n e _ c o n s t r a i n t
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	creates an integrity constraint and if not a CHECK
 *	constraint, also an index for the constraint.
 *
 **************************************/
	UCHAR verb;
	Firebird::MetaName constraint_name, index_name, referred_index_name;
	Firebird::MetaName null_field_name, trigger_name;
	MetaNameArray field_list;
	bool primary_flag = false, foreign_flag = false;
	UCHAR ri_action = 0;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	GET_STRING(ptr, constraint_name);
	if (constraint_name.length() == 0)
	{
		DYN_UTIL_generate_constraint_name(tdbb, gbl, constraint_name);
	}
	if (constraint_name.length() == 0)
	{
		DYN_error_punt(false, 212);
		// msg 212: "Zero length identifiers not allowed"
	}

	jrd_req* request = NULL;
	SSHORT id = -1;

	try {

	request = CMP_find_request(tdbb, drq_s_rel_con, DYN_REQUESTS);
	id = drq_s_rel_con;

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		CRT IN RDB$RELATION_CONSTRAINTS*/
	{
	
		strcpy(/*CRT.RDB$CONSTRAINT_NAME*/
		       jrd_707.jrd_710, constraint_name.c_str());
		strcpy(/*CRT.RDB$RELATION_NAME*/
		       jrd_707.jrd_709, relation_name->c_str());

		switch (verb = *(*ptr)++)
		{
		case isc_dyn_def_primary_key:
			primary_flag = true;
			strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
			       jrd_707.jrd_711, PRIMARY_KEY);
			break;

		case isc_dyn_def_foreign_key:
			foreign_flag = true;
			strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
			       jrd_707.jrd_711, FOREIGN_KEY);
			break;

		case isc_dyn_def_unique:
			strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
			       jrd_707.jrd_711, UNIQUE_CNSTRT);
			break;

		case isc_dyn_def_trigger:
			strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
			       jrd_707.jrd_711, CHECK_CNSTRT);
			/*CRT.RDB$INDEX_NAME.NULL*/
			jrd_707.jrd_712 = TRUE;
			break;

		case isc_dyn_fld_not_null:
			strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
			       jrd_707.jrd_711, NOT_NULL_CNSTRT);
			/*CRT.RDB$INDEX_NAME.NULL*/
			jrd_707.jrd_712 = TRUE;
			break;

		default:
			DYN_unsupported_verb();
		}

		if (verb != isc_dyn_def_trigger && verb != isc_dyn_fld_not_null) {
			referred_index_name = "";
			DYN_define_index(gbl, ptr, relation_name, verb, &index_name,
							 &referred_index_name, &constraint_name, &ri_action);
			strcpy(/*CRT.RDB$INDEX_NAME*/
			       jrd_707.jrd_708, index_name.c_str());
			/*CRT.RDB$INDEX_NAME.NULL*/
			jrd_707.jrd_712 = FALSE;

			SSHORT old_id = id;
			id = drq_l_rel_info;
			check_foreign_key_temp_scope(tdbb, gbl,
										 relation_name->c_str(), referred_index_name.c_str());
			id = old_id;

			/* check that we have references permissions on the table and
			   fields that the index:referred_index_name is on. */

			SCL_check_index(tdbb, referred_index_name, 0, SCL_sql_references);
		}
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_706, sizeof(jrd_706), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 110, (UCHAR*) &jrd_707);
	}

	if (!DYN_REQUEST(drq_s_rel_con))
	{
		DYN_REQUEST(drq_s_rel_con) = request;
	}

	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		SSHORT local_id = -1;
		USHORT number;
		// msg 121: "STORE RDB$RELATION_CONSTRAINTS failed"
		// msg 124: "A column name is repeated in the definition of constraint: %s"
		// msg 125: "Integrity constraint lookup failed"
		// msg 127: "STORE RDB$REF_CONSTRAINTS failed"
		// msg 232: "%s cannot reference %s"
		switch (id)
		{
			case drq_s_rel_con:
				number = 121;
				local_id = id;
				break;
			case drq_s_ref_con:
				number = 127;
				local_id = id;
				break;
			case drq_c_unq_nam:
				number = 121;
				break;
			case drq_n_idx_seg:
				number = 124;
				break;
			case drq_c_dup_con:
				number = 125;
				break;
			case drq_l_rel_info:
				number = 232;
				break;
			default:
				number = 125;
				break;
		}

		DYN_rundown_request(request, local_id);

		DYN_error_punt(true, number, number == 124 ? constraint_name.c_str() : NULL);
	}

	if (verb == isc_dyn_def_trigger)
	{
		do {
			DYN_define_trigger(gbl, ptr, relation_name, &trigger_name, false);
			DYN_UTIL_store_check_constraints(tdbb, gbl, constraint_name, trigger_name);
		} while ((verb = *(*ptr)++) == isc_dyn_def_trigger);

		if (verb != isc_dyn_end) {
			DYN_unsupported_verb();
		}
		return;
	}

	if (verb == isc_dyn_fld_not_null)
	{
		fb_assert(field_name);
		DYN_UTIL_store_check_constraints(tdbb, gbl, constraint_name, *field_name);

		if (*(*ptr)++ != isc_dyn_end) {
			DYN_unsupported_verb();
		}
		return;
	}

	try {

/* Make sure unique field names were specified for UNIQUE/PRIMARY/FOREIGN */
/* All fields must have the NOT NULL attribute specified for UNIQUE/PRIMARY. */

	request = CMP_find_request(tdbb, drq_c_unq_nam, DYN_REQUESTS);
	id = drq_c_unq_nam;

	bool not_null = true;
	int all_count = 0, unique_count = 0;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		IDS IN RDB$INDEX_SEGMENTS
			CROSS RFR IN RDB$RELATION_FIELDS
			CROSS FLX IN RDB$FIELDS WITH
			IDS.RDB$INDEX_NAME EQ index_name.c_str() AND
			RFR.RDB$RELATION_NAME EQ relation_name->c_str() AND
			RFR.RDB$FIELD_NAME EQ IDS.RDB$FIELD_NAME AND
			FLX.RDB$FIELD_NAME EQ RFR.RDB$FIELD_SOURCE
			REDUCED TO IDS.RDB$FIELD_NAME, IDS.RDB$INDEX_NAME,
			FLX.RDB$NULL_FLAG
			SORTED BY ASCENDING IDS.RDB$FIELD_NAME*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_694, sizeof(jrd_694), true);
	gds__vtov ((const char*) relation_name->c_str(), (char*) jrd_695.jrd_696, 32);
	gds__vtov ((const char*) index_name.c_str(), (char*) jrd_695.jrd_697, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_695);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 74, (UCHAR*) &jrd_698);
	   if (!jrd_698.jrd_701) break;
		if (!DYN_REQUEST(drq_c_unq_nam))
			DYN_REQUEST(drq_c_unq_nam) = request;

		if ((/*FLX.RDB$NULL_FLAG.NULL*/
		     jrd_698.jrd_704 || !/*FLX.RDB$NULL_FLAG*/
     jrd_698.jrd_705) &&
			(/*RFR.RDB$NULL_FLAG.NULL*/
			 jrd_698.jrd_702 || !/*RFR.RDB$NULL_FLAG*/
     jrd_698.jrd_703) &&
			primary_flag)
		{
			not_null = false;
			null_field_name = /*RFR.RDB$FIELD_NAME*/
					  jrd_698.jrd_700;
			EXE_unwind(tdbb, request);
			break;
		}

		unique_count++;
		field_list.add() = /*IDS.RDB$FIELD_NAME*/
				   jrd_698.jrd_699;
	/*END_FOR;*/
	   }
	}
	if (!DYN_REQUEST(drq_c_unq_nam)) {
		DYN_REQUEST(drq_c_unq_nam) = request;
	}

	if (!not_null) {
		DYN_error_punt(false, 123, null_field_name.c_str());
		/* msg 123: "Field: %s not defined as NOT NULL - can't be used in PRIMARY KEY constraint definition" */
	}

	request = CMP_find_request(tdbb, drq_n_idx_seg, DYN_REQUESTS);
	id = drq_n_idx_seg;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		IDS IN RDB$INDEX_SEGMENTS WITH IDS.RDB$INDEX_NAME EQ index_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_689, sizeof(jrd_689), true);
	gds__vtov ((const char*) index_name.c_str(), (char*) jrd_690.jrd_691, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_690);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_692);
	   if (!jrd_692.jrd_693) break;
		if (!DYN_REQUEST(drq_n_idx_seg)) {
			DYN_REQUEST(drq_n_idx_seg) = request;
		}

		all_count++;
	/*END_FOR;*/
	   }
	}
	if (!DYN_REQUEST(drq_n_idx_seg)) {
		DYN_REQUEST(drq_n_idx_seg) = request;
	}

	if (unique_count != all_count) {
		goto dyn_punt_false_124;
	}

/* For PRIMARY KEY/UNIQUE constraints, make sure same set of columns
   is not used in another constraint of either type */

	if (!foreign_flag)
	{
		request = CMP_find_request(tdbb, drq_c_dup_con, DYN_REQUESTS);
		id = drq_c_dup_con;

		index_name = "";
		int list_index = -1;
		bool found  = false;
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			CRT IN RDB$RELATION_CONSTRAINTS CROSS
				IDS IN RDB$INDEX_SEGMENTS OVER RDB$INDEX_NAME
				WITH CRT.RDB$RELATION_NAME EQ relation_name->c_str() AND
				(CRT.RDB$CONSTRAINT_TYPE EQ PRIMARY_KEY OR
				 CRT.RDB$CONSTRAINT_TYPE EQ UNIQUE_CNSTRT) AND
				CRT.RDB$CONSTRAINT_NAME NE constraint_name.c_str()
				SORTED BY CRT.RDB$INDEX_NAME, DESCENDING IDS.RDB$FIELD_NAME*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_679, sizeof(jrd_679), true);
		gds__vtov ((const char*) constraint_name.c_str(), (char*) jrd_680.jrd_681, 32);
		gds__vtov ((const char*) relation_name->c_str(), (char*) jrd_680.jrd_682, 32);
		gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_680.jrd_683, 12);
		gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_680.jrd_684, 12);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 88, (UCHAR*) &jrd_680);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_685);
		   if (!jrd_685.jrd_688) break;
			if (!DYN_REQUEST(drq_c_dup_con))
				DYN_REQUEST(drq_c_dup_con) = request;

			if (index_name != /*CRT.RDB$INDEX_NAME*/
					  jrd_685.jrd_687)
			{
				if (list_index >= 0) {
					found = false;
				}
				if (found) {
					EXE_unwind(tdbb, request);
					break;
				}
				list_index = field_list.getCount() - 1;
				index_name = /*CRT.RDB$INDEX_NAME*/
					     jrd_685.jrd_687;
				found = true;
			}

			if (list_index >= 0)
			{
				if (field_list[list_index--] != /*IDS.RDB$FIELD_NAME*/
								jrd_685.jrd_686)
				{
					found = false;
				}
			}
			else {
				found = false;
			}
		/*END_FOR;*/
		   }
		}
		if (!DYN_REQUEST(drq_c_dup_con)) {
			DYN_REQUEST(drq_c_dup_con) = request;
		}

		if (list_index >= 0) {
			found = false;
		}

		if (found) {
			goto dyn_punt_false_126;
		}
	}
	else
	{						/* Foreign key being defined   */

		request = CMP_find_request(tdbb, drq_s_ref_con, DYN_REQUESTS);
		id = drq_s_ref_con;

		jrd_req* old_request = NULL;

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			REF IN RDB$REF_CONSTRAINTS*/
		{
		
			old_request = request;
			const SSHORT old_id = id;
			request = CMP_find_request(tdbb, drq_l_intg_con, DYN_REQUESTS);
			id = drq_l_intg_con;

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				CRT IN RDB$RELATION_CONSTRAINTS WITH
					CRT.RDB$INDEX_NAME EQ referred_index_name.c_str() AND
					(CRT.RDB$CONSTRAINT_TYPE = PRIMARY_KEY OR
					 CRT.RDB$CONSTRAINT_TYPE = UNIQUE_CNSTRT)*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_663, sizeof(jrd_663), true);
			gds__vtov ((const char*) referred_index_name.c_str(), (char*) jrd_664.jrd_665, 32);
			gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_664.jrd_666, 12);
			gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_664.jrd_667, 12);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 56, (UCHAR*) &jrd_664);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_668);
			   if (!jrd_668.jrd_670) break;

				if (!DYN_REQUEST(drq_l_intg_con))
					DYN_REQUEST(drq_l_intg_con) = request;

				fb_utils::exact_name_limit(/*CRT.RDB$CONSTRAINT_NAME*/
							   jrd_668.jrd_669, sizeof(/*CRT.RDB$CONSTRAINT_NAME*/
	 jrd_668.jrd_669));
				strcpy(/*REF.RDB$CONST_NAME_UQ*/
				       jrd_672.jrd_674, /*CRT.RDB$CONSTRAINT_NAME*/
  jrd_668.jrd_669);
				strcpy(/*REF.RDB$CONSTRAINT_NAME*/
				       jrd_672.jrd_673, constraint_name.c_str());

				/*REF.RDB$UPDATE_RULE.NULL*/
				jrd_672.jrd_678 = FALSE;
				if (ri_action & FOR_KEY_UPD_CASCADE)
					strcpy(/*REF.RDB$UPDATE_RULE*/
					       jrd_672.jrd_676, RI_ACTION_CASCADE);
				else if (ri_action & FOR_KEY_UPD_NULL)
					strcpy(/*REF.RDB$UPDATE_RULE*/
					       jrd_672.jrd_676, RI_ACTION_NULL);
				else if (ri_action & FOR_KEY_UPD_DEFAULT)
					strcpy(/*REF.RDB$UPDATE_RULE*/
					       jrd_672.jrd_676, RI_ACTION_DEFAULT);
				else if (ri_action & FOR_KEY_UPD_NONE)
					strcpy(/*REF.RDB$UPDATE_RULE*/
					       jrd_672.jrd_676, RI_ACTION_NONE);
				else
					/* RESTRICT is the default value for this column */
					strcpy(/*REF.RDB$UPDATE_RULE*/
					       jrd_672.jrd_676, RI_RESTRICT);


				/*REF.RDB$DELETE_RULE.NULL*/
				jrd_672.jrd_677 = FALSE;
				if (ri_action & FOR_KEY_DEL_CASCADE)
					strcpy(/*REF.RDB$DELETE_RULE*/
					       jrd_672.jrd_675, RI_ACTION_CASCADE);
				else if (ri_action & FOR_KEY_DEL_NULL)
					strcpy(/*REF.RDB$DELETE_RULE*/
					       jrd_672.jrd_675, RI_ACTION_NULL);
				else if (ri_action & FOR_KEY_DEL_DEFAULT)
					strcpy(/*REF.RDB$DELETE_RULE*/
					       jrd_672.jrd_675, RI_ACTION_DEFAULT);
				else if (ri_action & FOR_KEY_DEL_NONE)
					strcpy(/*REF.RDB$DELETE_RULE*/
					       jrd_672.jrd_675, RI_ACTION_NONE);
				else
					/* RESTRICT is the default value for this column */
					strcpy(/*REF.RDB$DELETE_RULE*/
					       jrd_672.jrd_675, RI_RESTRICT);

			/*END_FOR;*/
			   }
			}
			if (!DYN_REQUEST(drq_l_intg_con))
				DYN_REQUEST(drq_l_intg_con) = request;
			request = old_request;
			id = old_id;
		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_671, sizeof(jrd_671), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 92, (UCHAR*) &jrd_672);
		}

		if (!DYN_REQUEST(drq_s_ref_con))
			DYN_REQUEST(drq_s_ref_con) = request;
	}

	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		SSHORT local_id = -1;
		USHORT number;
		// msg 121: "STORE RDB$RELATION_CONSTRAINTS failed"
		// msg 124: "A column name is repeated in the definition of constraint: %s"
		// msg 125: "Integrity constraint lookup failed"
		// msg 127: "STORE RDB$REF_CONSTRAINTS failed"
		switch (id)
		{
			case drq_s_rel_con:
				number = 121;
				local_id = id;
				break;
			case drq_s_ref_con:
				number = 127;
				local_id = id;
				break;
			case drq_c_unq_nam:
				number = 121;
				break;
			case drq_n_idx_seg:
				number = 124;
				break;
			case drq_c_dup_con:
				number = 125;
				break;
			default:
				number = 125;
				break;
		}

		DYN_rundown_request(request, local_id);

		DYN_error_punt(true, number, number == 124 ? constraint_name.c_str() : NULL);
	}

	return;

dyn_punt_false_124:
	DYN_error_punt(false, 124, constraint_name.c_str());
	/* msg 124: "A column name is repeated in the definition of constraint: %s" */
	return;

dyn_punt_false_126:
	DYN_error_punt(false, 126);
	/* msg 126: "Same set of columns cannot be used in more than one PRIMARY KEY and/or UNIQUE constraint definition" */
}


void DYN_define_dimension(Global*		gbl,
						  const UCHAR**	ptr,
						  const Firebird::MetaName*		relation_name,
						  Firebird::MetaName*		field_name)
{
   struct {
          TEXT  jrd_657 [32];	// RDB$FIELD_NAME 
          SLONG  jrd_658;	// RDB$LOWER_BOUND 
          SLONG  jrd_659;	// RDB$UPPER_BOUND 
          SSHORT jrd_660;	// RDB$DIMENSION 
          SSHORT jrd_661;	// gds__null_flag 
          SSHORT jrd_662;	// gds__null_flag 
   } jrd_656;
/**************************************
 *
 *	D Y N _ d e f i n e _ d i m e n s i o n
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	defines a single dimension for a field.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();
	jrd_req* request = CMP_find_request(tdbb, drq_s_dims, DYN_REQUESTS);

	bool b_ending_store = false;

	try {

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		DIM IN RDB$FIELD_DIMENSIONS*/
	{
	
		/*DIM.RDB$UPPER_BOUND.NULL*/
		jrd_656.jrd_662 = TRUE;
		/*DIM.RDB$LOWER_BOUND.NULL*/
		jrd_656.jrd_661 = TRUE;
		/*DIM.RDB$DIMENSION*/
		jrd_656.jrd_660 = (SSHORT)DYN_get_number(ptr);
		if (field_name) {
			strcpy(/*DIM.RDB$FIELD_NAME*/
			       jrd_656.jrd_657, field_name->c_str());
		}

		UCHAR verb;
		while ((verb = *(*ptr)++) != isc_dyn_end)
		{
			switch (verb)
			{
			case isc_dyn_fld_name:
				GET_STRING(ptr, /*DIM.RDB$FIELD_NAME*/
						jrd_656.jrd_657);
				break;

			case isc_dyn_dim_upper:
				/*DIM.RDB$UPPER_BOUND*/
				jrd_656.jrd_659 = DYN_get_number(ptr);
				/*DIM.RDB$UPPER_BOUND.NULL*/
				jrd_656.jrd_662 = FALSE;
				break;

			case isc_dyn_dim_lower:
				/*DIM.RDB$LOWER_BOUND*/
				jrd_656.jrd_658 = DYN_get_number(ptr);
				/*DIM.RDB$LOWER_BOUND.NULL*/
				jrd_656.jrd_661 = FALSE;
				break;

			default:
				--(*ptr);
				DYN_execute(gbl, ptr, relation_name, field_name, NULL, NULL, NULL);
			}
		}

		b_ending_store = true;

	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_655, sizeof(jrd_655), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 46, (UCHAR*) &jrd_656);
	}

	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_rundown_request(request, drq_s_dims);
			DYN_error_punt(true, 3);
			/* msg 3: "STORE RDB$FIELD_DIMENSIONS failed" */
		}
		throw;
	}

	if (!DYN_REQUEST(drq_s_dims)) {
		DYN_REQUEST(drq_s_dims) = request;
	}
}


void DYN_define_exception( Global* gbl, const UCHAR** ptr)
{
   struct {
          TEXT  jrd_649 [1024];	// RDB$MESSAGE 
          TEXT  jrd_650 [32];	// RDB$EXCEPTION_NAME 
          SLONG  jrd_651;	// RDB$EXCEPTION_NUMBER 
          SSHORT jrd_652;	// gds__null_flag 
          SSHORT jrd_653;	// gds__null_flag 
          SSHORT jrd_654;	// RDB$SYSTEM_FLAG 
   } jrd_648;
/**************************************
 *
 *	D Y N _ d e f i n e _ e x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *	Define an exception.
 *
 **************************************/
	Firebird::MetaName exception_name;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	GET_STRING(ptr, exception_name);

	if (exception_name.length() == 0) {
		DYN_error_punt(false, 212);
		// msg 212: "Zero length identifiers not allowed"
	}

	bool b_ending_store = false;

	try {

	check_unique_name(tdbb, gbl, exception_name, obj_exception);

	jrd_req* request = CMP_find_request(tdbb, drq_s_xcp, DYN_REQUESTS);

	const UCHAR* message_ptr = NULL;

	UCHAR verb;
	while ((verb = *(*ptr)++) != isc_dyn_end)
	{
		switch (verb)
		{
		case isc_dyn_xcp_msg:
			message_ptr = *ptr;
			DYN_skip_attribute(ptr);
			break;

		default:
			DYN_unsupported_verb();
		}
	}

	int faults = 0;

	while (true)
	{
		try
		{
			SINT64 xcp_id = DYN_UTIL_gen_unique_id(tdbb, gbl, drq_g_nxt_xcp_id, "RDB$EXCEPTIONS");

			xcp_id %= (MAX_SSHORT + 1);

			if (!xcp_id)
				continue;

			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				X IN RDB$EXCEPTIONS*/
			{
			

				/*X.RDB$EXCEPTION_NUMBER*/
				jrd_648.jrd_651 = xcp_id;
				strcpy(/*X.RDB$EXCEPTION_NAME*/
				       jrd_648.jrd_650, exception_name.c_str());

				/*X.RDB$SYSTEM_FLAG*/
				jrd_648.jrd_654 = 0;
				/*X.RDB$SYSTEM_FLAG.NULL*/
				jrd_648.jrd_653 = FALSE;
				/*X.RDB$MESSAGE.NULL*/
				jrd_648.jrd_652 = TRUE;

				if (message_ptr) {
					/*X.RDB$MESSAGE.NULL*/
					jrd_648.jrd_652 = FALSE;
					const UCHAR* temp_ptr = message_ptr;
					GET_BYTES(&temp_ptr, /*X.RDB$MESSAGE*/
							     jrd_648.jrd_649);
				}

				b_ending_store = true;

			/*END_STORE;*/
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_647, sizeof(jrd_647), true);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 1066, (UCHAR*) &jrd_648);
			}

			break;
		}
		catch (const Firebird::status_exception& ex)
		{
			if (b_ending_store)
				DYN_rundown_request(request, drq_s_xcp);

			if (ex.value()[1] != isc_no_dup)
				throw;

			if (++faults > MAX_SSHORT)
				throw;

			b_ending_store = false;
			fb_utils::init_status(tdbb->tdbb_status_vector);
		}
	}

	if (!DYN_REQUEST(drq_s_xcp)) {
		DYN_REQUEST(drq_s_xcp) = request;
	}

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_error_punt(true, 142);
			// msg 142: "DEFINE EXCEPTION failed"
		}
		throw;
	}
}


void DYN_define_file(Global*		gbl,
					 const UCHAR**	ptr,
					 SLONG		shadow_number,
					 SLONG*		start,
					 USHORT		msg)
{
   struct {
          TEXT  jrd_634 [256];	// RDB$FILE_NAME 
          SLONG  jrd_635;	// RDB$FILE_LENGTH 
          SLONG  jrd_636;	// RDB$FILE_START 
          SSHORT jrd_637;	// gds__null_flag 
          SSHORT jrd_638;	// gds__null_flag 
          SSHORT jrd_639;	// gds__null_flag 
          SSHORT jrd_640;	// RDB$FILE_FLAGS 
          SSHORT jrd_641;	// RDB$SHADOW_NUMBER 
   } jrd_633;
   struct {
          SSHORT jrd_646;	// gds__utility 
   } jrd_645;
   struct {
          TEXT  jrd_644 [256];	// RDB$FILE_NAME 
   } jrd_643;
/**************************************
 *
 *	D Y N _ d e f i n e _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Define a database or shadow file.
 *
 **************************************/
	UCHAR verb;
	SLONG temp;
	USHORT man_auto;
	SSHORT id;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	if (!tdbb->getAttachment()->locksmith())
	{
		ERR_post(Arg::Gds(isc_adm_task_denied));
	}

	jrd_req* request = NULL;

	try {

	id = -1;
	Firebird::PathName temp_f;
	GET_STRING(ptr, temp_f);
	if (!ISC_expand_filename(temp_f, false))
	{
		DYN_error_punt(false, 231);
		// File name is invalid.
	}

	request = CMP_find_request(tdbb, id = drq_l_files, DYN_REQUESTS);

	if (dbb->dbb_filename == temp_f) {
		DYN_error_punt(false, 166);
	}

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		FIRST 1 X IN RDB$FILES WITH X.RDB$FILE_NAME EQ temp_f.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_642, sizeof(jrd_642), true);
	gds__vtov ((const char*) temp_f.c_str(), (char*) jrd_643.jrd_644, 256);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 256, (UCHAR*) &jrd_643);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_645);
	   if (!jrd_645.jrd_646) break;
		if (!DYN_REQUEST(drq_l_files))
			DYN_REQUEST(drq_l_files) = request;

		DYN_error_punt(false, 166);
	/*END_FOR;*/
	   }
	}

	if (!DYN_REQUEST(drq_l_files))
		DYN_REQUEST(drq_l_files) = request;

	request = CMP_find_request(tdbb, id = drq_s_files, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$FILES*/
	{
	
		if (!DYN_REQUEST(drq_s_files))
			DYN_REQUEST(drq_s_files) = request;

		temp_f.copyTo(/*X.RDB$FILE_NAME*/
			      jrd_633.jrd_634, sizeof(/*X.RDB$FILE_NAME*/
	 jrd_633.jrd_634));
		/*X.RDB$SHADOW_NUMBER*/
		jrd_633.jrd_641 = (SSHORT)shadow_number;
		/*X.RDB$FILE_FLAGS*/
		jrd_633.jrd_640 = 0;
		/*X.RDB$FILE_FLAGS.NULL*/
		jrd_633.jrd_639 = FALSE;
		/*X.RDB$FILE_START.NULL*/
		jrd_633.jrd_638 = TRUE;
		/*X.RDB$FILE_LENGTH.NULL*/
		jrd_633.jrd_637 = TRUE;
		while ((verb = *(*ptr)++) != isc_dyn_end)
		{
			switch (verb)
			{
			case isc_dyn_file_start:
				temp = DYN_get_number(ptr);
				*start = MAX(*start, temp);
				/*X.RDB$FILE_START*/
				jrd_633.jrd_636 = *start;
				/*X.RDB$FILE_START.NULL*/
				jrd_633.jrd_638 = FALSE;
				break;

			case isc_dyn_file_length:
				/*X.RDB$FILE_LENGTH*/
				jrd_633.jrd_635 = DYN_get_number(ptr);
				/*X.RDB$FILE_LENGTH.NULL*/
				jrd_633.jrd_637 = FALSE;
				break;

			case isc_dyn_shadow_man_auto:
				man_auto = (USHORT)DYN_get_number(ptr);
				if (man_auto)
					/*X.RDB$FILE_FLAGS*/
					jrd_633.jrd_640 |= FILE_manual;
				break;

			case isc_dyn_shadow_conditional:
				if (DYN_get_number(ptr))
					/*X.RDB$FILE_FLAGS*/
					jrd_633.jrd_640 |= FILE_conditional;
				break;

			default:
				DYN_unsupported_verb();
			}
		}
		*start += /*X.RDB$FILE_LENGTH*/
			  jrd_633.jrd_635;
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_632, sizeof(jrd_632), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 274, (UCHAR*) &jrd_633);
	}

	if (!DYN_REQUEST(drq_s_files))
	{
		DYN_REQUEST(drq_s_files) = request;
	}

	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (id == drq_l_files)
		{
			DYN_rundown_request(request, drq_l_files);
			DYN_error_punt(false, 166);
		}
		else
		{
			if (id != -1)
				DYN_rundown_request(request, drq_s_files);

			DYN_error_punt(true, msg);
		}
	}

}


void DYN_define_difference(Global*		gbl,
						   const UCHAR**	ptr)
{
   struct {
          TEXT  jrd_619 [256];	// RDB$FILE_NAME 
          SLONG  jrd_620;	// RDB$FILE_LENGTH 
          SLONG  jrd_621;	// RDB$FILE_START 
          SSHORT jrd_622;	// gds__null_flag 
          SSHORT jrd_623;	// RDB$SHADOW_NUMBER 
          SSHORT jrd_624;	// gds__null_flag 
          SSHORT jrd_625;	// gds__null_flag 
          SSHORT jrd_626;	// gds__null_flag 
          SSHORT jrd_627;	// RDB$FILE_FLAGS 
   } jrd_618;
   struct {
          SSHORT jrd_630;	// gds__utility 
          SSHORT jrd_631;	// RDB$FILE_FLAGS 
   } jrd_629;
/**************************************
 *
 *	D Y N _ d e f i n e _ d i f f e r e n c e
 *
 **************************************
 *
 * Functional description
 *	Define backup difference file.
 *
 **************************************/
	SSHORT id = -1;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();
	jrd_req* request = NULL;

	if (!tdbb->getAttachment()->locksmith())
	{
		ERR_post(Arg::Gds(isc_adm_task_denied));
	}

	try {

	bool found = false;
	id = drq_l_difference;
	request = CMP_find_request(tdbb, drq_l_difference, DYN_REQUESTS);
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		FIL IN RDB$FILES*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_628, sizeof(jrd_628), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	while (1)
	   {
	   EXE_receive (tdbb, request, 0, 4, (UCHAR*) &jrd_629);
	   if (!jrd_629.jrd_630) break;
		if (/*FIL.RDB$FILE_FLAGS*/
		    jrd_629.jrd_631 & FILE_difference)
			found = true;
	/*END_FOR;*/
	   }
	}

	if (!DYN_REQUEST(drq_l_difference)) {
		DYN_REQUEST(drq_l_difference) = request;
	}

	if (found) {
		goto dyn_punt_216;
	}

	request = CMP_find_request(tdbb, drq_s_difference, DYN_REQUESTS);
	id = drq_s_difference;
	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$FILES*/
	{
	
		GET_STRING(ptr, /*X.RDB$FILE_NAME*/
				jrd_618.jrd_619);
		/*X.RDB$FILE_FLAGS*/
		jrd_618.jrd_627 = FILE_difference;
		/*X.RDB$FILE_FLAGS.NULL*/
		jrd_618.jrd_626 = FALSE;
		/*X.RDB$FILE_START*/
		jrd_618.jrd_621 = 0;
		/*X.RDB$FILE_START.NULL*/
		jrd_618.jrd_625 = FALSE;
		/*X.RDB$FILE_LENGTH.NULL*/
		jrd_618.jrd_624 = TRUE;
		/*X.RDB$SHADOW_NUMBER.NULL*/
		jrd_618.jrd_622 = TRUE;
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_617, sizeof(jrd_617), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 276, (UCHAR*) &jrd_618);
	}

	if (!DYN_REQUEST(drq_s_difference))
	{
		DYN_REQUEST(drq_s_difference) = request;
	}

	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (id == drq_s_difference)
		{
			DYN_rundown_request(request, drq_s_difference);
			DYN_error_punt(true, 150);
			/* msg 150: STORE RDB$FILES failed */
		}
		else
		{
			DYN_rundown_request(request, drq_l_difference);
			DYN_error_punt(true, 156);
			/* msg 156: Difference file lookup failed */
		}
	}

	return;

dyn_punt_216:
	DYN_error_punt(false, 216);
	/* msg 216: "Difference file is already defined" */
}


void DYN_define_filter( Global* gbl, const UCHAR** ptr)
{
   struct {
          bid  jrd_604;	// RDB$DESCRIPTION 
          TEXT  jrd_605 [32];	// RDB$ENTRYPOINT 
          TEXT  jrd_606 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_607 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_608;	// gds__null_flag 
          SSHORT jrd_609;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_610;	// gds__null_flag 
          SSHORT jrd_611;	// gds__null_flag 
          SSHORT jrd_612;	// gds__null_flag 
          SSHORT jrd_613;	// gds__null_flag 
          SSHORT jrd_614;	// RDB$INPUT_SUB_TYPE 
          SSHORT jrd_615;	// gds__null_flag 
          SSHORT jrd_616;	// RDB$OUTPUT_SUB_TYPE 
   } jrd_603;
/**************************************
 *
 *	D Y N _ d e f i n e _ f i l t e r
 *
 **************************************
 *
 * Functional description
 *	Define a blob filter.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	Firebird::MetaName filter_name;
	GET_STRING(ptr, filter_name);

	if (filter_name.length() == 0) {
		DYN_error_punt(false, 212);
	/* msg 212: "Zero length identifiers not allowed" */
	}

	jrd_req* request = CMP_find_request(tdbb, drq_s_filters, DYN_REQUESTS);

	bool b_ending_store = false;

	try {

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			X IN RDB$FILTERS USING*/
		{
		
			strcpy(/*X.RDB$FUNCTION_NAME*/
			       jrd_603.jrd_607, filter_name.c_str());
			/*X.RDB$OUTPUT_SUB_TYPE.NULL*/
			jrd_603.jrd_615 = TRUE;
			/*X.RDB$INPUT_SUB_TYPE.NULL*/
			jrd_603.jrd_613 = TRUE;
			/*X.RDB$MODULE_NAME.NULL*/
			jrd_603.jrd_612 = TRUE;
			/*X.RDB$ENTRYPOINT.NULL*/
			jrd_603.jrd_611 = TRUE;
			/*X.RDB$DESCRIPTION.NULL*/
			jrd_603.jrd_610 = TRUE;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_603.jrd_609 = 0;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_603.jrd_608 = FALSE;

			UCHAR verb;
			while ((verb = *(*ptr)++) != isc_dyn_end)
			{
				switch (verb)
				{
				case isc_dyn_filter_in_subtype:
					/*X.RDB$INPUT_SUB_TYPE*/
					jrd_603.jrd_614 = (SSHORT)DYN_get_number(ptr);
					/*X.RDB$INPUT_SUB_TYPE.NULL*/
					jrd_603.jrd_613 = FALSE;
					break;

				case isc_dyn_filter_out_subtype:
					/*X.RDB$OUTPUT_SUB_TYPE*/
					jrd_603.jrd_616 = (SSHORT)DYN_get_number(ptr);
					/*X.RDB$OUTPUT_SUB_TYPE.NULL*/
					jrd_603.jrd_615 = FALSE;
					break;

				case isc_dyn_func_module_name:
					GET_STRING(ptr, /*X.RDB$MODULE_NAME*/
							jrd_603.jrd_606);
					/*X.RDB$MODULE_NAME.NULL*/
					jrd_603.jrd_612 = FALSE;
					break;

				case isc_dyn_func_entry_point:
					GET_STRING(ptr, /*X.RDB$ENTRYPOINT*/
							jrd_603.jrd_605);
					/*X.RDB$ENTRYPOINT.NULL*/
					jrd_603.jrd_611 = FALSE;
					break;

				case isc_dyn_description:
					DYN_put_text_blob(gbl, ptr, &/*X.RDB$DESCRIPTION*/
								     jrd_603.jrd_604);
					/*X.RDB$DESCRIPTION.NULL*/
					jrd_603.jrd_610 = FALSE;
					break;

				default:
					DYN_unsupported_verb();
				}
			}

		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_602, sizeof(jrd_602), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 346, (UCHAR*) &jrd_603);
		}

		if (!DYN_REQUEST(drq_s_filters)) {
			DYN_REQUEST(drq_s_filters) = request;
		}
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_rundown_request(request, drq_s_filters);
			DYN_error_punt(true, 7);
			/* msg 7: "DEFINE BLOB FILTER failed" */
		}
		throw;
	}
}


void DYN_define_function( Global* gbl, const UCHAR** ptr)
{
   struct {
          bid  jrd_589;	// RDB$DESCRIPTION 
          TEXT  jrd_590 [32];	// RDB$ENTRYPOINT 
          TEXT  jrd_591 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_592 [32];	// RDB$QUERY_NAME 
          TEXT  jrd_593 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_594;	// gds__null_flag 
          SSHORT jrd_595;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_596;	// gds__null_flag 
          SSHORT jrd_597;	// gds__null_flag 
          SSHORT jrd_598;	// gds__null_flag 
          SSHORT jrd_599;	// gds__null_flag 
          SSHORT jrd_600;	// gds__null_flag 
          SSHORT jrd_601;	// RDB$RETURN_ARGUMENT 
   } jrd_588;
/**************************************
 *
 *	D Y N _ d e f i n e _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Define a user defined function.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	Firebird::MetaName function_name;
	GET_STRING(ptr, function_name);

	if (function_name.length() == 0)
	{
		DYN_error_punt(false, 212);
		// msg 212: "Zero length identifiers not allowed"
	}

	jrd_req* request = CMP_find_request(tdbb, drq_s_funcs, DYN_REQUESTS);

	bool b_ending_store = false;

	try {

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			X IN RDB$FUNCTIONS USING*/
		{
		
			strcpy(/*X.RDB$FUNCTION_NAME*/
			       jrd_588.jrd_593, function_name.c_str());
			/*X.RDB$RETURN_ARGUMENT.NULL*/
			jrd_588.jrd_600 = TRUE;
			/*X.RDB$QUERY_NAME.NULL*/
			jrd_588.jrd_599 = TRUE;
			/*X.RDB$MODULE_NAME.NULL*/
			jrd_588.jrd_598 = TRUE;
			/*X.RDB$ENTRYPOINT.NULL*/
			jrd_588.jrd_597 = TRUE;
			/*X.RDB$DESCRIPTION.NULL*/
			jrd_588.jrd_596 = TRUE;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_588.jrd_595 = 0;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_588.jrd_594 = FALSE;

			UCHAR verb;
			while ((verb = *(*ptr)++) != isc_dyn_end)
			{
				switch (verb)
				{
				case isc_dyn_func_return_argument:
					/*X.RDB$RETURN_ARGUMENT*/
					jrd_588.jrd_601 = (SSHORT)DYN_get_number(ptr);
					/*X.RDB$RETURN_ARGUMENT.NULL*/
					jrd_588.jrd_600 = FALSE;
					if (/*X.RDB$RETURN_ARGUMENT*/
					    jrd_588.jrd_601 > MAX_UDF_ARGUMENTS)
						DYN_error_punt(true, 10);
						/* msg 10: "DEFINE FUNCTION failed" */
					break;

				case isc_dyn_func_module_name:
					GET_STRING(ptr, /*X.RDB$MODULE_NAME*/
							jrd_588.jrd_591);
					/*X.RDB$MODULE_NAME.NULL*/
					jrd_588.jrd_598 = FALSE;
					break;

				case isc_dyn_fld_query_name:
					GET_STRING(ptr, /*X.RDB$QUERY_NAME*/
							jrd_588.jrd_592);
					/*X.RDB$QUERY_NAME.NULL*/
					jrd_588.jrd_599 = FALSE;
					break;

				case isc_dyn_func_entry_point:
					GET_STRING(ptr, /*X.RDB$ENTRYPOINT*/
							jrd_588.jrd_590);
					/*X.RDB$ENTRYPOINT.NULL*/
					jrd_588.jrd_597 = FALSE;
					break;

				case isc_dyn_description:
					DYN_put_text_blob(gbl, ptr, &/*X.RDB$DESCRIPTION*/
								     jrd_588.jrd_589);
					/*X.RDB$DESCRIPTION.NULL*/
					jrd_588.jrd_596 = FALSE;
					break;

				default:
					--(*ptr);
					MetaTmp(/*X.RDB$FUNCTION_NAME*/
						jrd_588.jrd_593)
						DYN_execute(gbl, ptr, NULL, NULL, NULL, &tmp, NULL);
				}
			}

			b_ending_store = true;
		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_587, sizeof(jrd_587), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 376, (UCHAR*) &jrd_588);
		}

		if (!DYN_REQUEST(drq_s_funcs)) {
			DYN_REQUEST(drq_s_funcs) = request;
		}
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_rundown_request(request, drq_s_funcs);
			DYN_error_punt(true, 10);
			/* msg 10: "DEFINE FUNCTION failed" */
		}
		throw;
	}
}


void DYN_define_function_arg(Global* gbl, const UCHAR** ptr, Firebird::MetaName* function_name)
{
   struct {
          TEXT  jrd_568 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_569;	// gds__null_flag 
          SSHORT jrd_570;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_571;	// gds__null_flag 
          SSHORT jrd_572;	// RDB$FIELD_PRECISION 
          SSHORT jrd_573;	// gds__null_flag 
          SSHORT jrd_574;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_575;	// gds__null_flag 
          SSHORT jrd_576;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_577;	// gds__null_flag 
          SSHORT jrd_578;	// RDB$FIELD_LENGTH 
          SSHORT jrd_579;	// gds__null_flag 
          SSHORT jrd_580;	// RDB$FIELD_SCALE 
          SSHORT jrd_581;	// gds__null_flag 
          SSHORT jrd_582;	// RDB$FIELD_TYPE 
          SSHORT jrd_583;	// gds__null_flag 
          SSHORT jrd_584;	// RDB$MECHANISM 
          SSHORT jrd_585;	// gds__null_flag 
          SSHORT jrd_586;	// RDB$ARGUMENT_POSITION 
   } jrd_567;
/**************************************
 *
 *	D Y N _ d e f i n e _ f u n c t i o n _ a r g
 *
 **************************************
 *
 * Functional description
 *	Define a user defined function argument.
 *
 **************************************/

	jrd_req* request = NULL;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	const USHORT major_version  = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	try {

	request = CMP_find_request(tdbb, drq_s_func_args, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$FUNCTION_ARGUMENTS*/
	{
	
		/*X.RDB$ARGUMENT_POSITION*/
		jrd_567.jrd_586 = (SSHORT)DYN_get_number(ptr);

		if (/*X.RDB$ARGUMENT_POSITION*/
		    jrd_567.jrd_586 > MAX_UDF_ARGUMENTS)
			DYN_error_punt(true, 12);
			/* msg 12: "DEFINE FUNCTION ARGUMENT failed" */

		if (function_name) {
			strcpy(/*X.RDB$FUNCTION_NAME*/
			       jrd_567.jrd_568, function_name->c_str());
			/*X.RDB$FUNCTION_NAME.NULL*/
			jrd_567.jrd_585 = FALSE;
		}
		else
			/*X.RDB$FUNCTION_NAME.NULL*/
			jrd_567.jrd_585 = TRUE;
		/*X.RDB$MECHANISM.NULL*/
		jrd_567.jrd_583 = TRUE;
		/*X.RDB$FIELD_TYPE.NULL*/
		jrd_567.jrd_581 = TRUE;
		/*X.RDB$FIELD_SCALE.NULL*/
		jrd_567.jrd_579 = TRUE;
		/*X.RDB$FIELD_LENGTH.NULL*/
		jrd_567.jrd_577 = TRUE;
		/*X.RDB$FIELD_SUB_TYPE.NULL*/
		jrd_567.jrd_575 = TRUE;
		/*X.RDB$CHARACTER_SET_ID.NULL*/
		jrd_567.jrd_573 = TRUE;
		/*X.RDB$FIELD_PRECISION.NULL*/
		jrd_567.jrd_571 = TRUE;
		/*X.RDB$CHARACTER_LENGTH.NULL*/
		jrd_567.jrd_569 = TRUE;

		UCHAR verb;
		while ((verb = *(*ptr)++) != isc_dyn_end)
			switch (verb)
			{
			case isc_dyn_function_name:
				GET_STRING(ptr, /*X.RDB$FUNCTION_NAME*/
						jrd_567.jrd_568);
				/*X.RDB$FUNCTION_NAME.NULL*/
				jrd_567.jrd_585 = FALSE;
				break;

			case isc_dyn_func_mechanism:
				/*X.RDB$MECHANISM*/
				jrd_567.jrd_584 = (SSHORT)DYN_get_number(ptr);
				/*X.RDB$MECHANISM.NULL*/
				jrd_567.jrd_583 = FALSE;
				break;

			case isc_dyn_fld_type:
				/*X.RDB$FIELD_TYPE*/
				jrd_567.jrd_582 = (SSHORT)DYN_get_number(ptr);
				/*X.RDB$FIELD_TYPE.NULL*/
				jrd_567.jrd_581 = FALSE;
				break;

			case isc_dyn_fld_sub_type:
				/*X.RDB$FIELD_SUB_TYPE*/
				jrd_567.jrd_576 = (SSHORT)DYN_get_number(ptr);
				/*X.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_567.jrd_575 = FALSE;
				break;

			case isc_dyn_fld_scale:
				/*X.RDB$FIELD_SCALE*/
				jrd_567.jrd_580 = (SSHORT)DYN_get_number(ptr);
				/*X.RDB$FIELD_SCALE.NULL*/
				jrd_567.jrd_579 = FALSE;
				break;

			case isc_dyn_fld_length:
				/*X.RDB$FIELD_LENGTH*/
				jrd_567.jrd_578 = (SSHORT)DYN_get_number(ptr);
				/*X.RDB$FIELD_LENGTH.NULL*/
				jrd_567.jrd_577 = FALSE;
				break;

			case isc_dyn_fld_character_set:
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_567.jrd_574 = (SSHORT)DYN_get_number(ptr);
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_567.jrd_573 = FALSE;
				break;

			case isc_dyn_fld_precision:
				/*X.RDB$FIELD_PRECISION*/
				jrd_567.jrd_572 = (SSHORT)DYN_get_number(ptr);
				/*X.RDB$FIELD_PRECISION.NULL*/
				jrd_567.jrd_571 = FALSE;
				break;

				/* Ignore the field character length as the system UDF parameter
				 * table has no place to store the information
				 * But IB6/FB has the place for this information. CVC 2001.
				 */
			case isc_dyn_fld_char_length:
				if (ENCODE_ODS(major_version, minor_original) < ODS_10_0)
				{
					DYN_get_number(ptr);
				}
				else
				{
					/*X.RDB$CHARACTER_LENGTH*/
					jrd_567.jrd_570      = (SSHORT)DYN_get_number (ptr);
					/*X.RDB$CHARACTER_LENGTH.NULL*/
					jrd_567.jrd_569 = FALSE;
				}
				break;

			default:
				DYN_unsupported_verb();
			}

	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_566, sizeof(jrd_566), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_567);
	}

	if (!DYN_REQUEST(drq_s_func_args)) {
		DYN_REQUEST(drq_s_func_args) = request;
	}

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (request) {
			DYN_rundown_request(request, drq_s_func_args);
		}
		DYN_error_punt(true, 12);
		/* msg 12: "DEFINE FUNCTION ARGUMENT failed" */
	}
}


void DYN_define_generator( Global* gbl, const UCHAR** ptr)
{
   struct {
          TEXT  jrd_562 [32];	// RDB$GENERATOR_NAME 
          SSHORT jrd_563;	// gds__null_flag 
          SSHORT jrd_564;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_565;	// RDB$GENERATOR_ID 
   } jrd_561;
/**************************************
 *
 *	D Y N _ d e f i n e _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *	Define a generator.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	Firebird::MetaName generator_name;
	GET_STRING(ptr, generator_name);

	if (generator_name.length() == 0) {
		DYN_error_punt(false, 212);
		/* msg 212: "Zero length identifiers not allowed" */
	}

	bool b_ending_store = false;

	try {

	check_unique_name(tdbb, gbl, generator_name, obj_generator);

	jrd_req* request = CMP_find_request(tdbb, drq_s_gens, DYN_REQUESTS);

	int faults = 0;

	while (true)
	{
		try
		{
			SINT64 gen_id = DYN_UTIL_gen_unique_id(tdbb, gbl, drq_g_nxt_gen_id, "RDB$GENERATORS");

			gen_id %= (MAX_SSHORT + 1);

			if (!gen_id)
				continue;

			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				X IN RDB$GENERATORS*/
			{
			

				/*X.RDB$GENERATOR_ID*/
				jrd_561.jrd_565 = gen_id;
				strcpy(/*X.RDB$GENERATOR_NAME*/
				       jrd_561.jrd_562, generator_name.c_str());

				/*X.RDB$SYSTEM_FLAG*/
				jrd_561.jrd_564 = 0;
				/*X.RDB$SYSTEM_FLAG.NULL*/
				jrd_561.jrd_563 = FALSE;

				b_ending_store = true;

			/*END_STORE;*/
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_560, sizeof(jrd_560), true);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 38, (UCHAR*) &jrd_561);
			}

			DPM_gen_id(tdbb, gen_id, true, 0);

			break;
		}
		catch (const Firebird::status_exception& ex)
		{
			if (b_ending_store)
				DYN_rundown_request(request, drq_s_gens);

			if (ex.value()[1] != isc_no_dup)
				throw;

			if (++faults > MAX_SSHORT)
				throw;

			b_ending_store = false;
			fb_utils::init_status(tdbb->tdbb_status_vector);
		}
	}

	if (!DYN_REQUEST(drq_s_gens)) {
		DYN_REQUEST(drq_s_gens) = request;
	}

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_error_punt(true, 8);
			/* msg 8: "DEFINE GENERATOR failed" */
		}
		throw;
	}

	if (*(*ptr)++ != isc_dyn_end)
	{
		DYN_error_punt(true, 9);
		/* msg 9: "DEFINE GENERATOR unexpected dyn verb" */
	}
}


void DYN_define_global_field(Global*		gbl,
							 const UCHAR**	ptr,
							 const Firebird::MetaName*		relation_name,
							 Firebird::MetaName*		field_name)
{
   struct {
          bid  jrd_514;	// RDB$DESCRIPTION 
          bid  jrd_515;	// RDB$VALIDATION_SOURCE 
          bid  jrd_516;	// RDB$VALIDATION_BLR 
          bid  jrd_517;	// RDB$DEFAULT_SOURCE 
          bid  jrd_518;	// RDB$DEFAULT_VALUE 
          bid  jrd_519;	// RDB$COMPUTED_SOURCE 
          bid  jrd_520;	// RDB$COMPUTED_BLR 
          bid  jrd_521;	// RDB$MISSING_VALUE 
          TEXT  jrd_522 [128];	// RDB$EDIT_STRING 
          bid  jrd_523;	// RDB$QUERY_HEADER 
          TEXT  jrd_524 [32];	// RDB$QUERY_NAME 
          TEXT  jrd_525 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_526;	// RDB$FIELD_TYPE 
          SSHORT jrd_527;	// gds__null_flag 
          SSHORT jrd_528;	// RDB$FIELD_LENGTH 
          SSHORT jrd_529;	// gds__null_flag 
          SSHORT jrd_530;	// RDB$FIELD_PRECISION 
          SSHORT jrd_531;	// gds__null_flag 
          SSHORT jrd_532;	// RDB$COLLATION_ID 
          SSHORT jrd_533;	// gds__null_flag 
          SSHORT jrd_534;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_535;	// gds__null_flag 
          SSHORT jrd_536;	// RDB$NULL_FLAG 
          SSHORT jrd_537;	// gds__null_flag 
          SSHORT jrd_538;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_539;	// gds__null_flag 
          SSHORT jrd_540;	// RDB$DIMENSIONS 
          SSHORT jrd_541;	// gds__null_flag 
          SSHORT jrd_542;	// gds__null_flag 
          SSHORT jrd_543;	// gds__null_flag 
          SSHORT jrd_544;	// gds__null_flag 
          SSHORT jrd_545;	// gds__null_flag 
          SSHORT jrd_546;	// gds__null_flag 
          SSHORT jrd_547;	// gds__null_flag 
          SSHORT jrd_548;	// gds__null_flag 
          SSHORT jrd_549;	// gds__null_flag 
          SSHORT jrd_550;	// gds__null_flag 
          SSHORT jrd_551;	// gds__null_flag 
          SSHORT jrd_552;	// gds__null_flag 
          SSHORT jrd_553;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_554;	// gds__null_flag 
          SSHORT jrd_555;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_556;	// gds__null_flag 
          SSHORT jrd_557;	// RDB$FIELD_SCALE 
          SSHORT jrd_558;	// gds__null_flag 
          SSHORT jrd_559;	// RDB$SYSTEM_FLAG 
   } jrd_513;
/**************************************
 *
 *	D Y N _ d e f i n e _ g l o b a l _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement. Create an explicit domain.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	UCHAR verb;
	Firebird::MetaName global_field_name;
	USHORT dtype;

	GET_STRING(ptr, global_field_name);

	if (global_field_name.length() == 0)
	{
		DYN_UTIL_generate_field_name(tdbb, gbl, global_field_name);
	}
	if (global_field_name.length() == 0)
	{
		DYN_error_punt(false, 212);
		/* msg 212: "Zero length identifiers not allowed" */
	}

	jrd_req* request = CMP_find_request(tdbb, drq_s_gfields, DYN_REQUESTS);

	bool b_ending_store = false;

	try {

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		FLD IN RDB$FIELDS USING*/
	{
	

		strcpy(/*FLD.RDB$FIELD_NAME*/
		       jrd_513.jrd_525, global_field_name.c_str());

		/*FLD.RDB$SYSTEM_FLAG*/
		jrd_513.jrd_559 = 0;
		/*FLD.RDB$SYSTEM_FLAG.NULL*/
		jrd_513.jrd_558 = FALSE;
		/*FLD.RDB$FIELD_SCALE.NULL*/
		jrd_513.jrd_556 = TRUE;
		/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
		jrd_513.jrd_554 = TRUE;
		/*FLD.RDB$SEGMENT_LENGTH.NULL*/
		jrd_513.jrd_552 = TRUE;
		/*FLD.RDB$QUERY_NAME.NULL*/
		jrd_513.jrd_551 = TRUE;
		/*FLD.RDB$QUERY_HEADER.NULL*/
		jrd_513.jrd_550 = TRUE;
		/*FLD.RDB$EDIT_STRING.NULL*/
		jrd_513.jrd_549 = TRUE;
		/*FLD.RDB$MISSING_VALUE.NULL*/
		jrd_513.jrd_548 = TRUE;
		/*FLD.RDB$COMPUTED_BLR.NULL*/
		jrd_513.jrd_547 = TRUE;
		/*FLD.RDB$COMPUTED_SOURCE.NULL*/
		jrd_513.jrd_546 = TRUE;
		/*FLD.RDB$DEFAULT_VALUE.NULL*/
		jrd_513.jrd_545 = TRUE;
		/*FLD.RDB$DEFAULT_SOURCE.NULL*/
		jrd_513.jrd_544 = TRUE;
		/*FLD.RDB$VALIDATION_BLR.NULL*/
		jrd_513.jrd_543 = TRUE;
		/*FLD.RDB$VALIDATION_SOURCE.NULL*/
		jrd_513.jrd_542 = TRUE;
		/*FLD.RDB$DESCRIPTION.NULL*/
		jrd_513.jrd_541 = TRUE;
		/*FLD.RDB$DIMENSIONS.NULL*/
		jrd_513.jrd_539 = TRUE;
		/*FLD.RDB$CHARACTER_LENGTH.NULL*/
		jrd_513.jrd_537 = TRUE;
		/*FLD.RDB$NULL_FLAG.NULL*/
		jrd_513.jrd_535 = TRUE;
		/*FLD.RDB$CHARACTER_SET_ID.NULL*/
		jrd_513.jrd_533 = TRUE;
		/*FLD.RDB$COLLATION_ID.NULL*/
		jrd_513.jrd_531 = TRUE;
		/*FLD.RDB$FIELD_PRECISION.NULL*/
		jrd_513.jrd_529 = TRUE;

		bool has_dimensions = false;
		bool has_default = false;

		while ((verb = *(*ptr)++) != isc_dyn_end)
		{
			switch (verb)
			{
			case isc_dyn_system_flag:
				/*FLD.RDB$SYSTEM_FLAG*/
				jrd_513.jrd_559 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$SYSTEM_FLAG.NULL*/
				jrd_513.jrd_558 = FALSE;
				break;

			case isc_dyn_fld_length:
				/*FLD.RDB$FIELD_LENGTH*/
				jrd_513.jrd_528 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$FIELD_LENGTH.NULL*/
				jrd_513.jrd_527 = FALSE;
				break;

			case isc_dyn_fld_type:
				dtype = (USHORT)DYN_get_number(ptr);
				/*FLD.RDB$FIELD_TYPE*/
				jrd_513.jrd_526 = (SSHORT)dtype;
				switch (dtype)
				{
				case blr_short:
					/*FLD.RDB$FIELD_LENGTH*/
					jrd_513.jrd_528 = 2;
					/*FLD.RDB$FIELD_LENGTH.NULL*/
					jrd_513.jrd_527 = FALSE;
					break;

				case blr_long:
				case blr_float:
				case blr_sql_time:
				case blr_sql_date:
					/*FLD.RDB$FIELD_LENGTH*/
					jrd_513.jrd_528 = 4;
					/*FLD.RDB$FIELD_LENGTH.NULL*/
					jrd_513.jrd_527 = FALSE;
					break;

				case blr_int64:
				case blr_quad:
				case blr_timestamp:
				case blr_double:
				case blr_d_float:
				case blr_blob:
					/*FLD.RDB$FIELD_LENGTH*/
					jrd_513.jrd_528 = 8;
					/*FLD.RDB$FIELD_LENGTH.NULL*/
					jrd_513.jrd_527 = FALSE;
					break;

				default:
					break;
				}
				break;

			case isc_dyn_fld_scale:
				/*FLD.RDB$FIELD_SCALE*/
				jrd_513.jrd_557 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$FIELD_SCALE.NULL*/
				jrd_513.jrd_556 = FALSE;
				break;

			case isc_dyn_fld_precision:
				/*FLD.RDB$FIELD_PRECISION*/
				jrd_513.jrd_530 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$FIELD_PRECISION.NULL*/
				jrd_513.jrd_529 = FALSE;
				break;

			case isc_dyn_fld_sub_type:
				/*FLD.RDB$FIELD_SUB_TYPE*/
				jrd_513.jrd_555 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_513.jrd_554 = FALSE;
				break;

			case isc_dyn_fld_char_length:
				/*FLD.RDB$CHARACTER_LENGTH*/
				jrd_513.jrd_538 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$CHARACTER_LENGTH.NULL*/
				jrd_513.jrd_537 = FALSE;
				break;

			case isc_dyn_fld_character_set:
				/*FLD.RDB$CHARACTER_SET_ID*/
				jrd_513.jrd_534 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$CHARACTER_SET_ID.NULL*/
				jrd_513.jrd_533 = FALSE;
				break;

			case isc_dyn_fld_collation:
				/*FLD.RDB$COLLATION_ID*/
				jrd_513.jrd_532 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$COLLATION_ID.NULL*/
				jrd_513.jrd_531 = FALSE;
				break;

			case isc_dyn_fld_segment_length:
				/*FLD.RDB$SEGMENT_LENGTH*/
				jrd_513.jrd_553 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$SEGMENT_LENGTH.NULL*/
				jrd_513.jrd_552 = FALSE;
				break;

			case isc_dyn_fld_query_name:
				GET_STRING(ptr, /*FLD.RDB$QUERY_NAME*/
						jrd_513.jrd_524);
				/*FLD.RDB$QUERY_NAME.NULL*/
				jrd_513.jrd_551 = FALSE;
				break;

			case isc_dyn_fld_query_header:
				DYN_put_blr_blob(gbl, ptr, &/*FLD.RDB$QUERY_HEADER*/
							    jrd_513.jrd_523);
				/*FLD.RDB$QUERY_HEADER.NULL*/
				jrd_513.jrd_550 = FALSE;
				break;

			case isc_dyn_fld_not_null:
				/*FLD.RDB$NULL_FLAG.NULL*/
				jrd_513.jrd_535 = FALSE;
				/*FLD.RDB$NULL_FLAG*/
				jrd_513.jrd_536 = TRUE;
				break;

			case isc_dyn_fld_missing_value:
				DYN_put_blr_blob(gbl, ptr, &/*FLD.RDB$MISSING_VALUE*/
							    jrd_513.jrd_521);
				/*FLD.RDB$MISSING_VALUE.NULL*/
				jrd_513.jrd_548 = FALSE;
				break;

			case isc_dyn_fld_computed_blr:
				DYN_put_blr_blob(gbl, ptr, &/*FLD.RDB$COMPUTED_BLR*/
							    jrd_513.jrd_520);
				/*FLD.RDB$COMPUTED_BLR.NULL*/
				jrd_513.jrd_547 = FALSE;
				break;

			case isc_dyn_fld_computed_source:
				DYN_put_text_blob(gbl, ptr, &/*FLD.RDB$COMPUTED_SOURCE*/
							     jrd_513.jrd_519);
				/*FLD.RDB$COMPUTED_SOURCE.NULL*/
				jrd_513.jrd_546 = FALSE;
				break;

			case isc_dyn_fld_default_value:
				if (has_dimensions)
				{
					DYN_error_punt(false, 226, global_field_name.c_str());
					// msg 226: "Default value is not allowed for array type in domain %s"
				}
				has_default = true;
				/*FLD.RDB$DEFAULT_VALUE.NULL*/
				jrd_513.jrd_545 = FALSE;
				DYN_put_blr_blob(gbl, ptr, &/*FLD.RDB$DEFAULT_VALUE*/
							    jrd_513.jrd_518);
				break;

			case isc_dyn_fld_default_source:
				if (has_dimensions)
				{
					DYN_error_punt(false, 226, global_field_name.c_str());
					// msg 226: "Default value is not allowed for array type in domain %s"
				}
				has_default = true;
				/*FLD.RDB$DEFAULT_SOURCE.NULL*/
				jrd_513.jrd_544 = FALSE;
				DYN_put_text_blob(gbl, ptr, &/*FLD.RDB$DEFAULT_SOURCE*/
							     jrd_513.jrd_517);
				break;

			case isc_dyn_fld_validation_blr:
				DYN_put_blr_blob(gbl, ptr, &/*FLD.RDB$VALIDATION_BLR*/
							    jrd_513.jrd_516);
				/*FLD.RDB$VALIDATION_BLR.NULL*/
				jrd_513.jrd_543 = FALSE;
				break;

			case isc_dyn_fld_validation_source:
				DYN_put_text_blob(gbl, ptr, &/*FLD.RDB$VALIDATION_SOURCE*/
							     jrd_513.jrd_515);
				/*FLD.RDB$VALIDATION_SOURCE.NULL*/
				jrd_513.jrd_542 = FALSE;
				break;

			case isc_dyn_fld_edit_string:
				GET_STRING(ptr, /*FLD.RDB$EDIT_STRING*/
						jrd_513.jrd_522);
				/*FLD.RDB$EDIT_STRING.NULL*/
				jrd_513.jrd_549 = FALSE;
				break;

			case isc_dyn_description:
				DYN_put_text_blob(gbl, ptr, &/*FLD.RDB$DESCRIPTION*/
							     jrd_513.jrd_514);
				/*FLD.RDB$DESCRIPTION.NULL*/
				jrd_513.jrd_541 = FALSE;
				break;

			case isc_dyn_fld_dimensions:
				if (has_default)
				{
					DYN_error_punt(false, 226, global_field_name.c_str());
					// msg 226: "Default value is not allowed for array type in domain %s"
				}
				has_dimensions = true;
				/*FLD.RDB$DIMENSIONS*/
				jrd_513.jrd_540 = (SSHORT)DYN_get_number(ptr);
				/*FLD.RDB$DIMENSIONS.NULL*/
				jrd_513.jrd_539 = FALSE;
				break;


			default:
				--(*ptr);
				MetaTmp(/*FLD.RDB$FIELD_NAME*/
					jrd_513.jrd_525)
					DYN_execute(gbl, ptr, relation_name, field_name ? field_name : &tmp,
								NULL, NULL, NULL);
			}
		}

		b_ending_store = true;

	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_512, sizeof(jrd_512), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 332, (UCHAR*) &jrd_513);
	}

	if (!DYN_REQUEST(drq_s_gfields)) {
		DYN_REQUEST(drq_s_gfields) = request;
	}

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_rundown_request(request, drq_s_gfields);
			DYN_error_punt(true, 13);
			/* msg 13: "STORE RDB$FIELDS failed" */
		}
		throw;
	}
}


void DYN_define_index(Global*		gbl,
					  const UCHAR**	ptr,
					  const Firebird::MetaName*		relation_name,
					  UCHAR		index_type,
					  Firebird::MetaName*		new_index_name,
					  Firebird::MetaName*		referred_index_name,
					  Firebird::MetaName*		cnst_name,
					  UCHAR*	ri_actionP)
{
   struct {
          TEXT  jrd_442 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_443;	// gds__utility 
          SSHORT jrd_444;	// RDB$SEGMENT_COUNT 
   } jrd_441;
   struct {
          TEXT  jrd_439 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_440 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_438;
   struct {
          bid  jrd_449;	// RDB$VIEW_BLR 
          SSHORT jrd_450;	// gds__utility 
          SSHORT jrd_451;	// gds__null_flag 
   } jrd_448;
   struct {
          TEXT  jrd_447 [32];	// RDB$RELATION_NAME 
   } jrd_446;
   struct {
          TEXT  jrd_458 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_459 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_460;	// gds__utility 
   } jrd_457;
   struct {
          TEXT  jrd_454 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_455 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_456 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_453;
   struct {
          TEXT  jrd_463 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_464 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_465;	// RDB$FIELD_POSITION 
   } jrd_462;
   struct {
          bid  jrd_471;	// RDB$COMPUTED_BLR 
          SSHORT jrd_472;	// gds__utility 
          SSHORT jrd_473;	// gds__null_flag 
          SSHORT jrd_474;	// RDB$COLLATION_ID 
          SSHORT jrd_475;	// RDB$FIELD_LENGTH 
          SSHORT jrd_476;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_477;	// gds__null_flag 
          SSHORT jrd_478;	// RDB$COLLATION_ID 
          SSHORT jrd_479;	// gds__null_flag 
          SSHORT jrd_480;	// gds__null_flag 
          SSHORT jrd_481;	// RDB$DIMENSIONS 
          SSHORT jrd_482;	// RDB$FIELD_TYPE 
   } jrd_470;
   struct {
          TEXT  jrd_468 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_469 [32];	// RDB$FIELD_NAME 
   } jrd_467;
   struct {
          bid  jrd_487;	// RDB$VIEW_BLR 
          SSHORT jrd_488;	// gds__utility 
          SSHORT jrd_489;	// gds__null_flag 
   } jrd_486;
   struct {
          TEXT  jrd_485 [32];	// RDB$RELATION_NAME 
   } jrd_484;
   struct {
          TEXT  jrd_492 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_493 [32];	// RDB$INDEX_NAME 
          bid  jrd_494;	// RDB$EXPRESSION_BLR 
          bid  jrd_495;	// RDB$EXPRESSION_SOURCE 
          TEXT  jrd_496 [32];	// RDB$FOREIGN_KEY 
          bid  jrd_497;	// RDB$DESCRIPTION 
          SSHORT jrd_498;	// RDB$SEGMENT_COUNT 
          SSHORT jrd_499;	// gds__null_flag 
          SSHORT jrd_500;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_501;	// gds__null_flag 
          SSHORT jrd_502;	// gds__null_flag 
          SSHORT jrd_503;	// gds__null_flag 
          SSHORT jrd_504;	// gds__null_flag 
          SSHORT jrd_505;	// gds__null_flag 
          SSHORT jrd_506;	// gds__null_flag 
          SSHORT jrd_507;	// RDB$INDEX_TYPE 
          SSHORT jrd_508;	// gds__null_flag 
          SSHORT jrd_509;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_510;	// gds__null_flag 
          SSHORT jrd_511;	// RDB$UNIQUE_FLAG 
   } jrd_491;
/**************************************
 *
 *	D Y N _ d e f i n e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	creates an index.
 *
 **************************************/
	Firebird::MetaName index_name;
	Firebird::MetaName referenced_relation;
	UCHAR verb;
	MetaNameArray field_list, seg_list;
	Firebird::MetaName trigger_name;

	if (ri_actionP != NULL) {
		(*ri_actionP) = 0;
	}

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	GET_STRING(ptr, index_name);

	if (index_name.length() == 0)
	{
		DYN_UTIL_generate_index_name(tdbb, gbl, index_name, index_type);
	}
	if (index_name.length() == 0) {
		DYN_error_punt(false, 212);
		/* msg 212: "Zero length identifiers not allowed" */
	}

	jrd_req* request = NULL;
	SSHORT id = -1;

	try {

	id = drq_l_idx_name;
	check_unique_name(tdbb, gbl, index_name, obj_index);

	request = CMP_find_request(tdbb, drq_s_indices, DYN_REQUESTS);
	id = drq_s_indices;

	ULONG key_length = 0;
	jrd_req* old_request = NULL;

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		IDX IN RDB$INDICES*/
	{
	
		/*IDX.RDB$UNIQUE_FLAG.NULL*/
		jrd_491.jrd_510 = TRUE;
		/*IDX.RDB$INDEX_INACTIVE.NULL*/
		jrd_491.jrd_508 = TRUE;
		/*IDX.RDB$INDEX_TYPE.NULL*/
		jrd_491.jrd_506 = TRUE;
		/*IDX.RDB$DESCRIPTION.NULL*/
		jrd_491.jrd_505 = TRUE;
		/*IDX.RDB$FOREIGN_KEY.NULL*/
		jrd_491.jrd_504 = TRUE;
		/*IDX.RDB$EXPRESSION_SOURCE.NULL*/
		jrd_491.jrd_503 = TRUE;
		/*IDX.RDB$EXPRESSION_BLR.NULL*/
		jrd_491.jrd_502 = TRUE;
		ULONG fld_count = 0, seg_count = 0;
		strcpy(/*IDX.RDB$INDEX_NAME*/
		       jrd_491.jrd_493, index_name.c_str());
		if (new_index_name)
			*new_index_name = /*IDX.RDB$INDEX_NAME*/
					  jrd_491.jrd_493;
		if (relation_name)
			strcpy(/*IDX.RDB$RELATION_NAME*/
			       jrd_491.jrd_492, relation_name->c_str());
		else if (*(*ptr)++ == isc_dyn_rel_name)
			GET_STRING(ptr, /*IDX.RDB$RELATION_NAME*/
					jrd_491.jrd_492);
		else
			DYN_error_punt(false, 14);
			/* msg 14: "No relation specified for index" */

		/*IDX.RDB$RELATION_NAME.NULL*/
		jrd_491.jrd_501 = FALSE;
		/*IDX.RDB$SYSTEM_FLAG*/
		jrd_491.jrd_500 = 0;
		/*IDX.RDB$SYSTEM_FLAG.NULL*/
		jrd_491.jrd_499 = FALSE;

		/* Check if the table is actually a view */

		old_request = request;
		SSHORT old_id = id;
		request = CMP_find_request(tdbb, drq_l_view_idx, DYN_REQUESTS);
		id = drq_l_view_idx;

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			VREL IN RDB$RELATIONS
			WITH VREL.RDB$RELATION_NAME EQ IDX.RDB$RELATION_NAME*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_483, sizeof(jrd_483), true);
		gds__vtov ((const char*) jrd_491.jrd_492, (char*) jrd_484.jrd_485, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_484);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_486);
		   if (!jrd_486.jrd_488) break;

			if (!DYN_REQUEST(drq_l_view_idx))
				DYN_REQUEST(drq_l_view_idx) = request;

			if (!/*VREL.RDB$VIEW_BLR.NULL*/
			     jrd_486.jrd_489)
				DYN_error_punt(false, 181);
				/* msg 181: "attempt to index a view" */

		/*END_FOR;*/
		   }
		}
		if (!DYN_REQUEST(drq_l_view_idx))
			DYN_REQUEST(drq_l_view_idx) = request;

		request = old_request;
		id = old_id;

		ULONG referred_cols = 0;

		while ((verb = *(*ptr)++) != isc_dyn_end)
			switch (verb)
			{
			case isc_dyn_idx_unique:
				/*IDX.RDB$UNIQUE_FLAG*/
				jrd_491.jrd_511 = (SSHORT)DYN_get_number(ptr);
				/*IDX.RDB$UNIQUE_FLAG.NULL*/
				jrd_491.jrd_510 = FALSE;
				break;

			case isc_dyn_idx_inactive:
				/*IDX.RDB$INDEX_INACTIVE*/
				jrd_491.jrd_509 = (SSHORT)DYN_get_number(ptr);
				/*IDX.RDB$INDEX_INACTIVE.NULL*/
				jrd_491.jrd_508 = FALSE;
				break;

			case isc_dyn_idx_type:
				/*IDX.RDB$INDEX_TYPE*/
				jrd_491.jrd_507 = (SSHORT)DYN_get_number(ptr);
				/*IDX.RDB$INDEX_TYPE.NULL*/
				jrd_491.jrd_506 = FALSE;
				break;

			case isc_dyn_fld_name:
				{
					Firebird::MetaName& str = seg_list.add();
					GET_STRING(ptr, str);
					// The famous brute force approach.
					for (ULONG iter = 0; iter < seg_count; ++iter)
					{
						if (seg_list[iter] == str)
						{
							DYN_error_punt(false, 240, SafeArg() << str.c_str() << /*IDX.RDB$INDEX_NAME*/
													       jrd_491.jrd_493);
							// msg 240 "Field %s cannot be used twice in index %s"
						}
					}

					seg_count++;

					old_request = request;
					old_id = id;
					request = CMP_find_request(tdbb, drq_l_lfield, DYN_REQUESTS);
					id = drq_l_lfield;

					/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
						F IN RDB$RELATION_FIELDS CROSS GF IN RDB$FIELDS
							WITH GF.RDB$FIELD_NAME EQ F.RDB$FIELD_SOURCE AND
							F.RDB$FIELD_NAME EQ str.c_str() AND
							IDX.RDB$RELATION_NAME EQ F.RDB$RELATION_NAME*/
					{
					if (!request)
					   request = CMP_compile2 (tdbb, (UCHAR*) jrd_466, sizeof(jrd_466), true);
					gds__vtov ((const char*) jrd_491.jrd_492, (char*) jrd_467.jrd_468, 32);
					gds__vtov ((const char*) str.c_str(), (char*) jrd_467.jrd_469, 32);
					EXE_start (tdbb, request, gbl->gbl_transaction);
					EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_467);
					while (1)
					   {
					   EXE_receive (tdbb, request, 1, 30, (UCHAR*) &jrd_470);
					   if (!jrd_470.jrd_472) break;

						ULONG length = 0;
						if (!DYN_REQUEST(drq_l_lfield))
						{
							DYN_REQUEST(drq_l_lfield) = request;
						}

						fld_count++;
						if (/*GF.RDB$FIELD_TYPE*/
						    jrd_470.jrd_482 == blr_blob)
						{
							DYN_error_punt(false, 116, /*IDX.RDB$INDEX_NAME*/
										   jrd_491.jrd_493);
							/* msg 116 "attempt to index blob field in index %s" */
						}
						else if (!/*GF.RDB$DIMENSIONS.NULL*/
							  jrd_470.jrd_480)
						{
							DYN_error_punt(false, 117, /*IDX.RDB$INDEX_NAME*/
										   jrd_491.jrd_493);
							/* msg 117 "attempt to index array field in index %s" */
						}
						else if (!/*GF.RDB$COMPUTED_BLR.NULL*/
							  jrd_470.jrd_479)
						{
							DYN_error_punt(false, 179, /*IDX.RDB$INDEX_NAME*/
										   jrd_491.jrd_493);
							/* msg 179 "attempt to index COMPUTED BY field in index %s" */
						}
						else if (/*GF.RDB$FIELD_TYPE*/
							 jrd_470.jrd_482 == blr_varying ||
								 /*GF.RDB$FIELD_TYPE*/
								 jrd_470.jrd_482 == blr_text)
						{
							/* Compute the length of the key segment allowing
							   for international information.  Note that we
							   must convert a <character set, collation> type
							   to an index type in order to compute the length */
							if (!/*F.RDB$COLLATION_ID.NULL*/
							     jrd_470.jrd_477)
							{
								length =
									INTL_key_length(tdbb,
										INTL_TEXT_TO_INDEX(
											INTL_CS_COLL_TO_TTYPE(/*GF.RDB$CHARACTER_SET_ID*/
													      jrd_470.jrd_476,
																  /*F.RDB$COLLATION_ID*/
																  jrd_470.jrd_478)),
										/*GF.RDB$FIELD_LENGTH*/
										jrd_470.jrd_475);
							}
							else if (!/*GF.RDB$COLLATION_ID.NULL*/
								  jrd_470.jrd_473)
							{
								length =
									INTL_key_length(tdbb,
										INTL_TEXT_TO_INDEX(
											INTL_CS_COLL_TO_TTYPE(/*GF.RDB$CHARACTER_SET_ID*/
													      jrd_470.jrd_476,
												/*GF.RDB$COLLATION_ID*/
												jrd_470.jrd_474)),
										/*GF.RDB$FIELD_LENGTH*/
										jrd_470.jrd_475);
							}
							else
							{
								length = /*GF.RDB$FIELD_LENGTH*/
									 jrd_470.jrd_475;
							}
						}
						else
						{
							length = sizeof(double);
						}
						if (key_length)
						{
							key_length += ((length + STUFF_COUNT - 1) / (unsigned) STUFF_COUNT) *
											(STUFF_COUNT + 1);
						}
						else
						{
							key_length = length;
						}
					/*END_FOR;*/
					   }
					}
					if (!DYN_REQUEST(drq_l_lfield))
						DYN_REQUEST(drq_l_lfield) = request;
					request = old_request;
					id = old_id;
					break;
				}

				/* for expression indices, store the BLR and the source string */

			case isc_dyn_fld_computed_blr:
				DYN_put_blr_blob(gbl, ptr, &/*IDX.RDB$EXPRESSION_BLR*/
							    jrd_491.jrd_494);
				/*IDX.RDB$EXPRESSION_BLR.NULL*/
				jrd_491.jrd_502 = FALSE;
				break;

			case isc_dyn_fld_computed_source:
				DYN_put_text_blob(gbl, ptr, &/*IDX.RDB$EXPRESSION_SOURCE*/
							     jrd_491.jrd_495);
				/*IDX.RDB$EXPRESSION_SOURCE.NULL*/
				jrd_491.jrd_503 = FALSE;
				break;

				/* for foreign keys, point to the corresponding relation */

			case isc_dyn_idx_foreign_key:
				GET_STRING(ptr, referenced_relation);
				if (referenced_relation.length() == 0) {
					DYN_error_punt(false, 212);
					/* msg 212: "Zero length identifiers not allowed" */
				}
				break;

			case isc_dyn_idx_ref_column:
				{
					Firebird::MetaName& str2 = field_list.add();
					GET_STRING(ptr, str2);
					referred_cols++;
				}
				break;

			case isc_dyn_description:
				DYN_put_text_blob(gbl, ptr, &/*IDX.RDB$DESCRIPTION*/
							     jrd_491.jrd_497);
				/*IDX.RDB$DESCRIPTION.NULL*/
				jrd_491.jrd_505 = FALSE;
				break;

			case isc_dyn_foreign_key_delete:
				fb_assert(ri_actionP != NULL);
				switch (verb = *(*ptr)++)
				{
				case isc_dyn_foreign_key_cascade:
					(*ri_actionP) |= FOR_KEY_DEL_CASCADE;
					if ((verb = *(*ptr)++) == isc_dyn_def_trigger)
					{
						DYN_define_trigger(gbl, ptr, relation_name, &trigger_name, true);
						fb_assert(cnst_name);
						DYN_UTIL_store_check_constraints(tdbb, gbl, *cnst_name, trigger_name);
					}
					else
						DYN_unsupported_verb();
					break;
				case isc_dyn_foreign_key_null:
					(*ri_actionP) |= FOR_KEY_DEL_NULL;
					if ((verb = *(*ptr)++) == isc_dyn_def_trigger)
					{
						DYN_define_trigger(gbl, ptr, relation_name, &trigger_name, true);
						fb_assert(cnst_name);
						DYN_UTIL_store_check_constraints(tdbb, gbl, *cnst_name, trigger_name);
					}
					else
						DYN_unsupported_verb();
					break;
				case isc_dyn_foreign_key_default:
					(*ri_actionP) |= FOR_KEY_DEL_DEFAULT;
					if ((verb = *(*ptr)++) == isc_dyn_def_trigger)
					{
						DYN_define_trigger(gbl, ptr, relation_name, &trigger_name, true);
						fb_assert(cnst_name);
						DYN_UTIL_store_check_constraints(tdbb, gbl, *cnst_name, trigger_name);
					}
					else
						DYN_unsupported_verb();
					break;
				case isc_dyn_foreign_key_none:
					(*ri_actionP) |= FOR_KEY_DEL_NONE;
					break;
				default:
					fb_assert(0);	/* should not come here */
					DYN_unsupported_verb();
				}
				break;

			case isc_dyn_foreign_key_update:
				fb_assert(ri_actionP != NULL);
				switch (verb = *(*ptr)++)
				{
				case isc_dyn_foreign_key_cascade:
					(*ri_actionP) |= FOR_KEY_UPD_CASCADE;
					if ((verb = *(*ptr)++) == isc_dyn_def_trigger)
					{
						DYN_define_trigger(gbl, ptr, relation_name, &trigger_name, true);
						fb_assert(cnst_name);
						DYN_UTIL_store_check_constraints(tdbb, gbl, *cnst_name, trigger_name);
					}
					else
						DYN_unsupported_verb();
					break;
				case isc_dyn_foreign_key_null:
					(*ri_actionP) |= FOR_KEY_UPD_NULL;
					if ((verb = *(*ptr)++) == isc_dyn_def_trigger)
					{
						DYN_define_trigger(gbl, ptr, relation_name, &trigger_name, true);
						fb_assert(cnst_name);
						DYN_UTIL_store_check_constraints(tdbb, gbl, *cnst_name, trigger_name);
					}
					else
						DYN_unsupported_verb();
					break;
				case isc_dyn_foreign_key_default:
					(*ri_actionP) |= FOR_KEY_UPD_DEFAULT;
					if ((verb = *(*ptr)++) == isc_dyn_def_trigger)
					{
						DYN_define_trigger(gbl, ptr, relation_name, &trigger_name, true);
						fb_assert(cnst_name);
						DYN_UTIL_store_check_constraints(tdbb, gbl, *cnst_name, trigger_name);
					}
					else
						DYN_unsupported_verb();
					break;
				case isc_dyn_foreign_key_none:
					(*ri_actionP) |= FOR_KEY_UPD_NONE;
					break;
				default:
					fb_assert(0);	/* should not come here */
					DYN_unsupported_verb();
				}
				break;
			default:
				DYN_unsupported_verb();
			}

		key_length = ROUNDUP(key_length, sizeof(SLONG));
		if (key_length >= MAX_KEY)
			DYN_error_punt(false, 118, /*IDX.RDB$INDEX_NAME*/
						   jrd_491.jrd_493);
			/* msg 118 "key size too big for index %s" */

		if (seg_count) {
			if (seg_count != fld_count)
				DYN_error_punt(false, 120, /*IDX.RDB$INDEX_NAME*/
							   jrd_491.jrd_493);
				/* msg 118 "Unknown fields in index %s" */

			old_request = request;
			old_id = id;
			request = CMP_find_request(tdbb, drq_s_idx_segs, DYN_REQUESTS);
			id = drq_s_idx_segs;
			while (seg_list.getCount()) {
				/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
					X IN RDB$INDEX_SEGMENTS*/
				{
				
					strcpy(/*X.RDB$INDEX_NAME*/
					       jrd_462.jrd_464, /*IDX.RDB$INDEX_NAME*/
  jrd_491.jrd_493);
					Firebird::MetaName str = seg_list.pop();
					strcpy(/*X.RDB$FIELD_NAME*/
					       jrd_462.jrd_463, str.c_str());
					/*X.RDB$FIELD_POSITION*/
					jrd_462.jrd_465 = --fld_count;
				/*END_STORE;*/
				if (!request)
				   request = CMP_compile2 (tdbb, (UCHAR*) jrd_461, sizeof(jrd_461), true);
				EXE_start (tdbb, request, gbl->gbl_transaction);
				EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_462);
				}
			}
			if (!DYN_REQUEST(drq_s_idx_segs))
				DYN_REQUEST(drq_s_idx_segs) = request;
			request = old_request;
			id = old_id;
		}
		else if (/*IDX.RDB$EXPRESSION_BLR.NULL*/
			 jrd_491.jrd_502)
		{
			DYN_error_punt(false, 119, /*IDX.RDB$INDEX_NAME*/
						   jrd_491.jrd_493);
			// msg 119 "no keys for index %s"
		}

		if (field_list.getCount()) {
			/* If referring columns count <> referred columns return error */

			if (seg_count != referred_cols)
				DYN_error_punt(true, 133);
				/* msg 133: "Number of referencing columns do not equal number of referenced columns */

			/* lookup a unique index in the referenced relation with the
			   referenced fields mentioned */

			old_request = request;
			old_id = id;
			request = CMP_find_request(tdbb, drq_l_unq_idx, DYN_REQUESTS);
			id = drq_l_unq_idx;

			index_name = "";
			int list_index = -1;
			bool found = false;
			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			    RC IN RDB$RELATION_CONSTRAINTS CROSS
				IND IN RDB$INDICES OVER RDB$INDEX_NAME CROSS
					ISEG IN RDB$INDEX_SEGMENTS OVER RDB$INDEX_NAME WITH
					IND.RDB$RELATION_NAME EQ referenced_relation.c_str() AND
				    IND.RDB$UNIQUE_FLAG NOT MISSING AND
					(RC.RDB$CONSTRAINT_TYPE = PRIMARY_KEY OR
					RC.RDB$CONSTRAINT_TYPE = UNIQUE_CNSTRT)
					SORTED BY IND.RDB$INDEX_NAME,
					DESCENDING ISEG.RDB$FIELD_POSITION*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_452, sizeof(jrd_452), true);
			gds__vtov ((const char*) referenced_relation.c_str(), (char*) jrd_453.jrd_454, 32);
			gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_453.jrd_455, 12);
			gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_453.jrd_456, 12);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 56, (UCHAR*) &jrd_453);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_457);
			   if (!jrd_457.jrd_460) break;

				if (!DYN_REQUEST(drq_l_unq_idx))
					DYN_REQUEST(drq_l_unq_idx) = request;

				if (index_name != /*IND.RDB$INDEX_NAME*/
						  jrd_457.jrd_459) {
					if (list_index >= 0)
						found = false;
					if (found) {
						EXE_unwind(tdbb, request);
						break;
					}
					list_index = field_list.getCount() - 1;
					index_name = /*IND.RDB$INDEX_NAME*/
						     jrd_457.jrd_459;
					found = true;
				}

				/* if there are no more fields or the field name doesn't
				   match, then this is not the correct index */

				if (list_index >= 0) {
					fb_utils::exact_name_limit(/*ISEG.RDB$FIELD_NAME*/
								   jrd_457.jrd_458, sizeof(/*ISEG.RDB$FIELD_NAME*/
	 jrd_457.jrd_458));
					if (field_list[list_index--] != /*ISEG.RDB$FIELD_NAME*/
									jrd_457.jrd_458)
					{
						found = false;
					}
				}
				else
					found = false;

			/*END_FOR;*/
			   }
			}
			if (!DYN_REQUEST(drq_l_unq_idx))
				DYN_REQUEST(drq_l_unq_idx) = request;
			request = old_request;
			id = old_id;

			if (list_index >= 0)
				found = false;

			if (found) {
				strcpy(/*IDX.RDB$FOREIGN_KEY*/
				       jrd_491.jrd_496, index_name.c_str());
				/*IDX.RDB$FOREIGN_KEY.NULL*/
				jrd_491.jrd_504 = FALSE;
				if (referred_index_name) {
					*referred_index_name = index_name;
				}
			}
			else {
				jrd_req* request2 = NULL;
				found = false;
				bool isView = false;

				/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE gbl->gbl_transaction)
					X IN RDB$RELATIONS
						WITH X.RDB$RELATION_NAME EQ referenced_relation.c_str()*/
				{
				if (!request2)
				   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_445, sizeof(jrd_445), true);
				gds__vtov ((const char*) referenced_relation.c_str(), (char*) jrd_446.jrd_447, 32);
				EXE_start (tdbb, request2, gbl->gbl_transaction);
				EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_446);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 12, (UCHAR*) &jrd_448);
				   if (!jrd_448.jrd_450) break;
					found = true;
					isView = !/*X.RDB$VIEW_BLR.NULL*/
						  jrd_448.jrd_451;
				/*END_FOR*/
				   }
				}

				CMP_release(tdbb, request2);

				if (isView)
				{
					DYN_error_punt(false, 242, referenced_relation.c_str());
					// msg 242: "attempt to reference a view (%s) in a foreign key"
				}

				if (found)
				{
					DYN_error_punt(false, 18, referenced_relation.c_str());
					// msg 18: "could not find UNIQUE or PRIMARY KEY constraint in table %s with specified columns"
				}
				else
				{
					DYN_error_punt(false, 241, referenced_relation.c_str());
					// msg 241: "Table %s not found"
				}
			}
		}
		else if (referenced_relation.length()) {
			old_request = request;
			old_id = id;
			request = CMP_find_request(tdbb, drq_l_primary, DYN_REQUESTS);
			id = drq_l_primary;

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				IND IN RDB$INDICES CROSS
					RC IN RDB$RELATION_CONSTRAINTS
					OVER RDB$INDEX_NAME WITH
					IND.RDB$RELATION_NAME EQ referenced_relation.c_str() AND
					RC.RDB$CONSTRAINT_TYPE EQ PRIMARY_KEY*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_437, sizeof(jrd_437), true);
			gds__vtov ((const char*) referenced_relation.c_str(), (char*) jrd_438.jrd_439, 32);
			gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_438.jrd_440, 12);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 44, (UCHAR*) &jrd_438);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_441);
			   if (!jrd_441.jrd_443) break;
				if (!DYN_REQUEST(drq_l_primary))
					DYN_REQUEST(drq_l_primary) = request;

				/* Number of columns in referred index should be same as number
				   of columns in referring index */

				fb_assert(/*IND.RDB$SEGMENT_COUNT*/
					  jrd_441.jrd_444 >= 0);
				if (seg_count != ULONG(/*IND.RDB$SEGMENT_COUNT*/
						       jrd_441.jrd_444))
					DYN_error_punt(true, 133);
				/* msg 133: "Number of referencing columns do not equal number of referenced columns" */

				fb_utils::exact_name_limit(/*IND.RDB$INDEX_NAME*/
							   jrd_441.jrd_442, sizeof(/*IND.RDB$INDEX_NAME*/
	 jrd_441.jrd_442));
				strcpy(/*IDX.RDB$FOREIGN_KEY*/
				       jrd_491.jrd_496, /*IND.RDB$INDEX_NAME*/
  jrd_441.jrd_442);
				/*IDX.RDB$FOREIGN_KEY.NULL*/
				jrd_491.jrd_504 = FALSE;
				if (referred_index_name)
					*referred_index_name = /*IND.RDB$INDEX_NAME*/
							       jrd_441.jrd_442;

			/*END_FOR;*/
			   }
			}
			if (!DYN_REQUEST(drq_l_primary))
				DYN_REQUEST(drq_l_primary) = request;
			request = old_request;
			id = old_id;

			if (/*IDX.RDB$FOREIGN_KEY.NULL*/
			    jrd_491.jrd_504)
			{
				DYN_error_punt(false, 20, referenced_relation.c_str());
				// msg 20: "could not find PRIMARY KEY index in specified table %s"
			}
		}

		/*IDX.RDB$SEGMENT_COUNT*/
		jrd_491.jrd_498 = seg_count;
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_490, sizeof(jrd_490), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 148, (UCHAR*) &jrd_491);
	}

	if (!DYN_REQUEST(drq_s_indices)) {
		DYN_REQUEST(drq_s_indices) = request;
	}

	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (id == drq_s_indices) {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 21);
			/* msg 21: "STORE RDB$INDICES failed" */
		}
		else if (id == drq_s_idx_segs) {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 15);
			/* msg 15: "STORE RDB$INDEX_SEGMENTS failed" */
		}

		DYN_rundown_request(request, -1);

		switch (id)
		{
		case drq_l_idx_name:
			DYN_error_punt(true, 21);
			/* msg 21: "STORE RDB$INDICES failed" */
			break;
		case drq_l_lfield:
			DYN_error_punt(true, 15);
			/* msg 15: "STORE RDB$INDEX_SEGMENTS failed" */
			break;
		case drq_l_unq_idx:
			DYN_error_punt(true, 17);
			/* msg 17: "Primary Key field lookup failed" */
			break;
		case drq_l_view_idx:
			DYN_error_punt(true, 180);
			/* msg 180: "Table Name lookup failed" */
			break;
		case drq_l_primary:
			DYN_error_punt(true, 19);
			/* msg 19: "Primary Key lookup failed" */
			break;
		default:
			fb_assert(false);
		}
	}
}


void DYN_define_local_field(Global*		gbl,
							const UCHAR**	ptr,
							const Firebird::MetaName*	relation_name,
							Firebird::MetaName*	field_name)
{
   struct {
          bid  jrd_382;	// RDB$COMPUTED_SOURCE 
          bid  jrd_383;	// RDB$COMPUTED_BLR 
          TEXT  jrd_384 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_385;	// gds__null_flag 
          SSHORT jrd_386;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_387;	// gds__null_flag 
          SSHORT jrd_388;	// RDB$FIELD_PRECISION 
          SSHORT jrd_389;	// gds__null_flag 
          SSHORT jrd_390;	// RDB$FIELD_SCALE 
          SSHORT jrd_391;	// gds__null_flag 
          SSHORT jrd_392;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_393;	// gds__null_flag 
          SSHORT jrd_394;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_395;	// gds__null_flag 
          SSHORT jrd_396;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_397;	// gds__null_flag 
          SSHORT jrd_398;	// RDB$FIELD_LENGTH 
          SSHORT jrd_399;	// gds__null_flag 
          SSHORT jrd_400;	// RDB$FIELD_TYPE 
          SSHORT jrd_401;	// gds__null_flag 
          SSHORT jrd_402;	// RDB$SYSTEM_FLAG 
   } jrd_381;
   struct {
          TEXT  jrd_405 [128];	// RDB$EDIT_STRING 
          bid  jrd_406;	// RDB$DEFAULT_SOURCE 
          bid  jrd_407;	// RDB$DEFAULT_VALUE 
          bid  jrd_408;	// RDB$DESCRIPTION 
          TEXT  jrd_409 [32];	// RDB$SECURITY_CLASS 
          bid  jrd_410;	// RDB$QUERY_HEADER 
          TEXT  jrd_411 [32];	// RDB$QUERY_NAME 
          TEXT  jrd_412 [32];	// RDB$BASE_FIELD 
          TEXT  jrd_413 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_414 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_415 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_416;	// gds__null_flag 
          SSHORT jrd_417;	// RDB$COLLATION_ID 
          SSHORT jrd_418;	// gds__null_flag 
          SSHORT jrd_419;	// gds__null_flag 
          SSHORT jrd_420;	// gds__null_flag 
          SSHORT jrd_421;	// gds__null_flag 
          SSHORT jrd_422;	// gds__null_flag 
          SSHORT jrd_423;	// gds__null_flag 
          SSHORT jrd_424;	// gds__null_flag 
          SSHORT jrd_425;	// gds__null_flag 
          SSHORT jrd_426;	// RDB$VIEW_CONTEXT 
          SSHORT jrd_427;	// gds__null_flag 
          SSHORT jrd_428;	// RDB$FIELD_POSITION 
          SSHORT jrd_429;	// gds__null_flag 
          SSHORT jrd_430;	// RDB$UPDATE_FLAG 
          SSHORT jrd_431;	// gds__null_flag 
          SSHORT jrd_432;	// gds__null_flag 
          SSHORT jrd_433;	// RDB$NULL_FLAG 
          SSHORT jrd_434;	// gds__null_flag 
          SSHORT jrd_435;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_436;	// gds__null_flag 
   } jrd_404;
/**************************************
 *
 *	D Y N _ d e f i n e _ l o c a l _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement.
 *
 **************************************/
	UCHAR verb;
	USHORT dtype, length, clength, precision;
	SSHORT stype, scale;
	SSHORT charset_id;
	SLONG fld_pos;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	Firebird::MetaName local_field_name;
	GET_STRING(ptr, local_field_name);

	if (local_field_name.length() == 0) {
		DYN_error_punt(false, 212);
		/* msg 212: "Zero length identifiers not allowed" */
	}

	jrd_req* request = NULL;
	SSHORT id = -1;

	try {

	request = CMP_find_request(tdbb, drq_s_lfields, DYN_REQUESTS);
	id = drq_s_lfields;

	bool lflag, sflag, slflag, scflag, clflag, prflag;
	scflag = lflag = sflag = slflag = clflag = prflag = false;
	bool charset_id_flag = false;
	const UCHAR* blr = NULL;
	const UCHAR* source = NULL;
	Firebird::MetaName relation_buffer;

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		RFR IN RDB$RELATION_FIELDS*/
	{
	
		strcpy(/*RFR.RDB$FIELD_NAME*/
		       jrd_404.jrd_415, local_field_name.c_str());
		strcpy(/*RFR.RDB$FIELD_SOURCE*/
		       jrd_404.jrd_414, /*RFR.RDB$FIELD_NAME*/
  jrd_404.jrd_415);
		if (field_name) {
			*field_name = /*RFR.RDB$FIELD_NAME*/
				      jrd_404.jrd_415;
		}
		/*RFR.RDB$RELATION_NAME.NULL*/
		jrd_404.jrd_436 = TRUE;
		if (relation_name) {
			strcpy(/*RFR.RDB$RELATION_NAME*/
			       jrd_404.jrd_413, relation_name->c_str());
			/*RFR.RDB$RELATION_NAME.NULL*/
			jrd_404.jrd_436 = FALSE;
		}
		/*RFR.RDB$SYSTEM_FLAG*/
		jrd_404.jrd_435 = 0;
		/*RFR.RDB$SYSTEM_FLAG.NULL*/
		jrd_404.jrd_434 = FALSE;
		/*RFR.RDB$NULL_FLAG.NULL*/
		jrd_404.jrd_432 = TRUE;
		/*RFR.RDB$BASE_FIELD.NULL*/
		jrd_404.jrd_431 = TRUE;
		/*RFR.RDB$UPDATE_FLAG.NULL*/
		jrd_404.jrd_429 = TRUE;
		/*RFR.RDB$FIELD_POSITION.NULL*/
		jrd_404.jrd_427 = TRUE;
		/*RFR.RDB$VIEW_CONTEXT.NULL*/
		jrd_404.jrd_425 = TRUE;
		/*RFR.RDB$QUERY_NAME.NULL*/
		jrd_404.jrd_424 = TRUE;
		/*RFR.RDB$QUERY_HEADER.NULL*/
		jrd_404.jrd_423 = TRUE;
		/*RFR.RDB$SECURITY_CLASS.NULL*/
		jrd_404.jrd_422 = TRUE;
		/*RFR.RDB$DESCRIPTION.NULL*/
		jrd_404.jrd_421 = TRUE;
		/*RFR.RDB$DEFAULT_VALUE.NULL*/
		jrd_404.jrd_420 = TRUE;
		/*RFR.RDB$DEFAULT_SOURCE.NULL*/
		jrd_404.jrd_419 = TRUE;
		/*RFR.RDB$EDIT_STRING.NULL*/
		jrd_404.jrd_418 = TRUE;
		/*RFR.RDB$COLLATION_ID.NULL*/
		jrd_404.jrd_416 = TRUE;

		bool has_default = false;

		while ((verb = *(*ptr)++) != isc_dyn_end)
			switch (verb)
			{
			case isc_dyn_rel_name:
				GET_STRING(ptr, relation_buffer);
				relation_name = &relation_buffer;
				strcpy(/*RFR.RDB$RELATION_NAME*/
				       jrd_404.jrd_413, relation_name->c_str());
				/*RFR.RDB$RELATION_NAME.NULL*/
				jrd_404.jrd_436 = FALSE;
				break;

			case isc_dyn_fld_source:
				GET_STRING(ptr, /*RFR.RDB$FIELD_SOURCE*/
						jrd_404.jrd_414);
				break;

			case isc_dyn_fld_base_fld:
				GET_STRING(ptr, /*RFR.RDB$BASE_FIELD*/
						jrd_404.jrd_412);
				/*RFR.RDB$BASE_FIELD.NULL*/
				jrd_404.jrd_431 = FALSE;
				break;

			case isc_dyn_fld_query_name:
				GET_STRING(ptr, /*RFR.RDB$QUERY_NAME*/
						jrd_404.jrd_411);
				/*RFR.RDB$QUERY_NAME.NULL*/
				jrd_404.jrd_424 = FALSE;
				break;

			case isc_dyn_fld_query_header:
				DYN_put_blr_blob(gbl, ptr, &/*RFR.RDB$QUERY_HEADER*/
							    jrd_404.jrd_410);
				/*RFR.RDB$QUERY_HEADER.NULL*/
				jrd_404.jrd_423 = FALSE;
				break;

			case isc_dyn_fld_edit_string:
				GET_STRING(ptr, /*RFR.RDB$EDIT_STRING*/
						jrd_404.jrd_405);
				/*RFR.RDB$EDIT_STRING.NULL*/
				jrd_404.jrd_418 = FALSE;
				break;

			case isc_dyn_fld_position:
				/*RFR.RDB$FIELD_POSITION*/
				jrd_404.jrd_428 = (SSHORT)DYN_get_number(ptr);
				/*RFR.RDB$FIELD_POSITION.NULL*/
				jrd_404.jrd_427 = FALSE;
				break;

			case isc_dyn_system_flag:
				/*RFR.RDB$SYSTEM_FLAG*/
				jrd_404.jrd_435 = (SSHORT)DYN_get_number(ptr);
				/*RFR.RDB$SYSTEM_FLAG.NULL*/
				jrd_404.jrd_434 = FALSE;
				break;

			case isc_dyn_fld_update_flag:
			case isc_dyn_update_flag:
				/*RFR.RDB$UPDATE_FLAG*/
				jrd_404.jrd_430 = (SSHORT)DYN_get_number(ptr);
				/*RFR.RDB$UPDATE_FLAG.NULL*/
				jrd_404.jrd_429 = FALSE;
				break;

			case isc_dyn_view_context:
				/*RFR.RDB$VIEW_CONTEXT*/
				jrd_404.jrd_426 = (SSHORT)DYN_get_number(ptr);
				/*RFR.RDB$VIEW_CONTEXT.NULL*/
				jrd_404.jrd_425 = FALSE;
				break;

			case isc_dyn_security_class:
				GET_STRING(ptr, /*RFR.RDB$SECURITY_CLASS*/
						jrd_404.jrd_409);
				/*RFR.RDB$SECURITY_CLASS.NULL*/
				jrd_404.jrd_422 = FALSE;
				break;

			case isc_dyn_description:
				DYN_put_text_blob(gbl, ptr, &/*RFR.RDB$DESCRIPTION*/
							     jrd_404.jrd_408);
				/*RFR.RDB$DESCRIPTION.NULL*/
				jrd_404.jrd_421 = FALSE;
				break;

			case isc_dyn_fld_computed_blr:
				DYN_UTIL_generate_field_name(tdbb, gbl, /*RFR.RDB$FIELD_SOURCE*/
									jrd_404.jrd_414);
				blr = *ptr;
				DYN_skip_attribute(ptr);
				break;

			case isc_dyn_fld_computed_source:
				source = *ptr;
				DYN_skip_attribute(ptr);
				break;

			case isc_dyn_fld_default_value:
				has_default = true;
				/*RFR.RDB$DEFAULT_VALUE.NULL*/
				jrd_404.jrd_420 = FALSE;
				DYN_put_blr_blob(gbl, ptr, &/*RFR.RDB$DEFAULT_VALUE*/
							    jrd_404.jrd_407);
				break;

			case isc_dyn_fld_default_source:
				has_default = true;
				/*RFR.RDB$DEFAULT_SOURCE.NULL*/
				jrd_404.jrd_419 = FALSE;
				DYN_put_text_blob(gbl, ptr, &/*RFR.RDB$DEFAULT_SOURCE*/
							     jrd_404.jrd_406);
				break;

			case isc_dyn_fld_not_null:
				/*RFR.RDB$NULL_FLAG.NULL*/
				jrd_404.jrd_432 = FALSE;
				/*RFR.RDB$NULL_FLAG*/
				jrd_404.jrd_433 = TRUE;
				break;

			case isc_dyn_fld_type:
				dtype = (USHORT)DYN_get_number(ptr);
				break;

			case isc_dyn_fld_length:
				length = (USHORT)DYN_get_number(ptr);
				lflag = true;
				break;

			case isc_dyn_fld_sub_type:
				stype = (SSHORT)DYN_get_number(ptr);
				sflag = true;
				break;

			case isc_dyn_fld_char_length:
				clength = (USHORT)DYN_get_number(ptr);
				clflag = true;
				break;

			case isc_dyn_fld_segment_length:
				stype = (SSHORT)DYN_get_number(ptr);
				slflag = true;
				break;

			case isc_dyn_fld_scale:
				scale = (SSHORT)DYN_get_number(ptr);
				scflag = true;
				break;

			case isc_dyn_fld_precision:
				precision = (USHORT)DYN_get_number(ptr);
				prflag = true;
				break;

			case isc_dyn_fld_character_set:
				charset_id = (SSHORT)DYN_get_number(ptr);
				charset_id_flag = true;
				break;

			case isc_dyn_fld_collation:
				/*RFR.RDB$COLLATION_ID.NULL*/
				jrd_404.jrd_416 = FALSE;
				/*RFR.RDB$COLLATION_ID*/
				jrd_404.jrd_417 = (SSHORT)DYN_get_number(ptr);
				break;

			default:
				MetaTmp(/*RFR.RDB$FIELD_SOURCE*/
					jrd_404.jrd_414)
					DYN_execute(gbl, ptr, relation_name, &tmp, NULL, NULL, NULL);
			}

		if (has_default && DYN_UTIL_is_array(tdbb, gbl, /*RFR.RDB$FIELD_SOURCE*/
								jrd_404.jrd_414))
		{
			DYN_error_punt(false, 226, /*RFR.RDB$FIELD_SOURCE*/
						   jrd_404.jrd_414);
			// msg 226: "Default value is not allowed for array type in domain %s"
		}

		if (/*RFR.RDB$FIELD_POSITION.NULL*/
		    jrd_404.jrd_427 == TRUE) {
			fld_pos = -1;
			fb_assert(relation_name);
			DYN_UTIL_generate_field_position(tdbb, gbl, *relation_name, &fld_pos);

			if (fld_pos >= 0) {
				/*RFR.RDB$FIELD_POSITION*/
				jrd_404.jrd_428 = (SSHORT)++fld_pos;
				/*RFR.RDB$FIELD_POSITION.NULL*/
				jrd_404.jrd_427 = FALSE;
			}
		}

		if (blr) {
			jrd_req* old_request = request;
			const SSHORT old_id = id;
			request = CMP_find_request(tdbb, drq_s_gfields2, DYN_REQUESTS);

			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				FLD IN RDB$FIELDS*/
			{
			

				/*FLD.RDB$SYSTEM_FLAG*/
				jrd_381.jrd_402 = 0;
				/*FLD.RDB$SYSTEM_FLAG.NULL*/
				jrd_381.jrd_401 = FALSE;
				strcpy(/*FLD.RDB$FIELD_NAME*/
				       jrd_381.jrd_384, /*RFR.RDB$FIELD_SOURCE*/
  jrd_404.jrd_414);
				DYN_put_blr_blob(gbl, &blr, &/*FLD.RDB$COMPUTED_BLR*/
							     jrd_381.jrd_383);
				if (source) {
					DYN_put_text_blob(gbl, &source, &/*FLD.RDB$COMPUTED_SOURCE*/
									 jrd_381.jrd_382);
				}
				/*FLD.RDB$FIELD_TYPE*/
				jrd_381.jrd_400 = dtype;
				/*FLD.RDB$FIELD_TYPE.NULL*/
				jrd_381.jrd_399 = FALSE;
				if (lflag) {
					/*FLD.RDB$FIELD_LENGTH*/
					jrd_381.jrd_398 = length;
					/*FLD.RDB$FIELD_LENGTH.NULL*/
					jrd_381.jrd_397 = FALSE;
				}
				else
					/*FLD.RDB$FIELD_LENGTH.NULL*/
					jrd_381.jrd_397 = TRUE;
				if (sflag) {
					/*FLD.RDB$FIELD_SUB_TYPE*/
					jrd_381.jrd_396 = stype;
					/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
					jrd_381.jrd_395 = FALSE;
				}
				else
					/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
					jrd_381.jrd_395 = TRUE;
				if (clflag) {
					/*FLD.RDB$CHARACTER_LENGTH*/
					jrd_381.jrd_394 = clength;
					/*FLD.RDB$CHARACTER_LENGTH.NULL*/
					jrd_381.jrd_393 = FALSE;
				}
				else
					/*FLD.RDB$CHARACTER_LENGTH.NULL*/
					jrd_381.jrd_393 = TRUE;
				if (slflag) {
					/*FLD.RDB$SEGMENT_LENGTH*/
					jrd_381.jrd_392 = stype;
					/*FLD.RDB$SEGMENT_LENGTH.NULL*/
					jrd_381.jrd_391 = FALSE;
				}
				else
					/*FLD.RDB$SEGMENT_LENGTH.NULL*/
					jrd_381.jrd_391 = TRUE;
				if (scflag) {
					/*FLD.RDB$FIELD_SCALE*/
					jrd_381.jrd_390 = scale;
					/*FLD.RDB$FIELD_SCALE.NULL*/
					jrd_381.jrd_389 = FALSE;
				}
				else
					/*FLD.RDB$FIELD_SCALE.NULL*/
					jrd_381.jrd_389 = TRUE;
				if (prflag) {
					/*FLD.RDB$FIELD_PRECISION*/
					jrd_381.jrd_388 = precision;
					/*FLD.RDB$FIELD_PRECISION.NULL*/
					jrd_381.jrd_387 = FALSE;
				}
				else
					/*FLD.RDB$FIELD_PRECISION.NULL*/
					jrd_381.jrd_387 = TRUE;
				if (charset_id_flag) {
					/*FLD.RDB$CHARACTER_SET_ID*/
					jrd_381.jrd_386 = charset_id;
					/*FLD.RDB$CHARACTER_SET_ID.NULL*/
					jrd_381.jrd_385 = FALSE;
				}
				else
					/*FLD.RDB$CHARACTER_SET_ID.NULL*/
					jrd_381.jrd_385 = TRUE;

			/*END_STORE;*/
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_380, sizeof(jrd_380), true);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 84, (UCHAR*) &jrd_381);
			}

			if (!DYN_REQUEST(drq_s_gfields2))
				DYN_REQUEST(drq_s_gfields2) = request;
			request = old_request;
			id = old_id;
		}

		if (!/*RFR.RDB$VIEW_CONTEXT.NULL*/
		     jrd_404.jrd_425)
		{
			fb_assert(relation_name);
			DYN_UTIL_find_field_source(tdbb, gbl, *relation_name, /*RFR.RDB$VIEW_CONTEXT*/
									      jrd_404.jrd_426,
				/*RFR.RDB$BASE_FIELD*/
				jrd_404.jrd_412, /*RFR.RDB$FIELD_SOURCE*/
  jrd_404.jrd_414);
		}
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_403, sizeof(jrd_403), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 394, (UCHAR*) &jrd_404);
	}

	if (!DYN_REQUEST(drq_s_lfields)) {
		DYN_REQUEST(drq_s_lfields) = request;
	}

	}	// try
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (id == drq_s_lfields) {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 23);
			/* msg 23: "STORE RDB$RELATION_FIELDS failed" */
		}
		else {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 22);
			/* msg 22: "STORE RDB$FIELDS failed" */
		}
	}
}


void DYN_define_parameter( Global* gbl, const UCHAR** ptr, Firebird::MetaName* procedure_name)
{
   struct {
          SSHORT jrd_308;	// gds__utility 
   } jrd_307;
   struct {
          TEXT  jrd_303 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_304 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_305;	// gds__null_flag 
          SSHORT jrd_306;	// gds__null_flag 
   } jrd_302;
   struct {
          TEXT  jrd_297 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_298 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_299;	// gds__utility 
          SSHORT jrd_300;	// gds__null_flag 
          SSHORT jrd_301;	// gds__null_flag 
   } jrd_296;
   struct {
          TEXT  jrd_294 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_295 [32];	// RDB$PROCEDURE_NAME 
   } jrd_293;
   struct {
          SSHORT jrd_337;	// gds__utility 
   } jrd_336;
   struct {
          bid  jrd_326;	// RDB$DEFAULT_VALUE 
          bid  jrd_327;	// RDB$DEFAULT_SOURCE 
          SSHORT jrd_328;	// gds__null_flag 
          SSHORT jrd_329;	// RDB$COLLATION_ID 
          SSHORT jrd_330;	// gds__null_flag 
          SSHORT jrd_331;	// gds__null_flag 
          SSHORT jrd_332;	// gds__null_flag 
          SSHORT jrd_333;	// RDB$NULL_FLAG 
          SSHORT jrd_334;	// gds__null_flag 
          SSHORT jrd_335;	// RDB$PARAMETER_MECHANISM 
   } jrd_325;
   struct {
          bid  jrd_314;	// RDB$DEFAULT_SOURCE 
          bid  jrd_315;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_316;	// gds__utility 
          SSHORT jrd_317;	// gds__null_flag 
          SSHORT jrd_318;	// RDB$PARAMETER_MECHANISM 
          SSHORT jrd_319;	// gds__null_flag 
          SSHORT jrd_320;	// RDB$NULL_FLAG 
          SSHORT jrd_321;	// gds__null_flag 
          SSHORT jrd_322;	// gds__null_flag 
          SSHORT jrd_323;	// gds__null_flag 
          SSHORT jrd_324;	// RDB$COLLATION_ID 
   } jrd_313;
   struct {
          TEXT  jrd_311 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_312 [32];	// RDB$PROCEDURE_NAME 
   } jrd_310;
   struct {
          bid  jrd_340;	// RDB$DEFAULT_SOURCE 
          bid  jrd_341;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_342 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_343;	// gds__null_flag 
          SSHORT jrd_344;	// gds__null_flag 
          SSHORT jrd_345;	// gds__null_flag 
          SSHORT jrd_346;	// RDB$COLLATION_ID 
          SSHORT jrd_347;	// gds__null_flag 
          SSHORT jrd_348;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_349;	// gds__null_flag 
          SSHORT jrd_350;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_351;	// gds__null_flag 
          SSHORT jrd_352;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_353;	// gds__null_flag 
          SSHORT jrd_354;	// RDB$FIELD_PRECISION 
          SSHORT jrd_355;	// gds__null_flag 
          SSHORT jrd_356;	// RDB$FIELD_SCALE 
          SSHORT jrd_357;	// gds__null_flag 
          SSHORT jrd_358;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_359;	// RDB$FIELD_TYPE 
          SSHORT jrd_360;	// RDB$FIELD_LENGTH 
          SSHORT jrd_361;	// gds__null_flag 
          SSHORT jrd_362;	// RDB$SYSTEM_FLAG 
   } jrd_339;
   struct {
          bid  jrd_365;	// RDB$DESCRIPTION 
          TEXT  jrd_366 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_367 [32];	// RDB$PROCEDURE_NAME 
          TEXT  jrd_368 [32];	// RDB$PARAMETER_NAME 
          SSHORT jrd_369;	// gds__null_flag 
          SSHORT jrd_370;	// RDB$NULL_FLAG 
          SSHORT jrd_371;	// gds__null_flag 
          SSHORT jrd_372;	// gds__null_flag 
          SSHORT jrd_373;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_374;	// gds__null_flag 
          SSHORT jrd_375;	// gds__null_flag 
          SSHORT jrd_376;	// RDB$PARAMETER_TYPE 
          SSHORT jrd_377;	// gds__null_flag 
          SSHORT jrd_378;	// RDB$PARAMETER_NUMBER 
          SSHORT jrd_379;	// gds__null_flag 
   } jrd_364;
/**************************************
 *
 *	D Y N _ d e f i n e _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *	Define the parameters for a stored procedure.
 *
 **************************************/
	// Leave these as USHORT. Don't convert the *_null ones to bool.
	USHORT f_length, f_type, f_charlength, f_seg_length,
		f_scale_null, f_subtype_null, f_seg_length_null,
		f_charlength_null, f_precision_null, f_charset_null, f_collation_null, f_notnull_null;
	SSHORT id;
	SSHORT f_subtype, f_scale, f_precision;
	SSHORT f_charset, f_collation;
	USHORT f_notnull;
	const UCHAR* default_value_ptr = NULL;
	const UCHAR* default_source_ptr = NULL;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	Firebird::MetaName parameter_name;
	GET_STRING(ptr, parameter_name);

	if (parameter_name.length() == 0) {
		DYN_error_punt(false, 212);
		/* msg 212: "Zero length identifiers not allowed" */
	}

	jrd_req* request = CMP_find_request(tdbb, drq_s_prms, DYN_REQUESTS);

	try {

	id = -1;

	f_length = f_type = f_subtype = f_charlength = f_scale = f_seg_length = 0;
	f_charset = f_collation = f_precision = 0;
	f_notnull = FALSE;
	f_scale_null = f_subtype_null = f_charlength_null = f_seg_length_null = TRUE;
	f_precision_null = f_charset_null = f_collation_null = f_notnull_null = TRUE;
	id = drq_s_prms;

	Firebird::MetaName prc_name, rel_name, fld_name;
	prm_mech_t mechanism = prm_mech_normal;
	bool explicit_domain = false;

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		P IN RDB$PROCEDURE_PARAMETERS USING*/
	{
	
		strcpy(/*P.RDB$PARAMETER_NAME*/
		       jrd_364.jrd_368, parameter_name.c_str());
		if (procedure_name) {
			/*P.RDB$PROCEDURE_NAME.NULL*/
			jrd_364.jrd_379 = FALSE;
			strcpy(/*P.RDB$PROCEDURE_NAME*/
			       jrd_364.jrd_367, procedure_name->c_str());
			prc_name = *procedure_name;
		}
		else
			/*P.RDB$PROCEDURE_NAME.NULL*/
			jrd_364.jrd_379 = TRUE;
		/*P.RDB$PARAMETER_NUMBER.NULL*/
		jrd_364.jrd_377 = TRUE;
		/*P.RDB$PARAMETER_TYPE.NULL*/
		jrd_364.jrd_375 = TRUE;
		/*P.RDB$FIELD_SOURCE.NULL*/
		jrd_364.jrd_374 = TRUE;
		/*P.RDB$SYSTEM_FLAG*/
		jrd_364.jrd_373 = 0;
		/*P.RDB$SYSTEM_FLAG.NULL*/
		jrd_364.jrd_372 = FALSE;
		/*P.RDB$DESCRIPTION.NULL*/
		jrd_364.jrd_371 = TRUE;

		UCHAR verb;
		while ((verb = *(*ptr)++) != isc_dyn_end)
		{
			switch (verb)
			{
			case isc_dyn_system_flag:
				/*P.RDB$SYSTEM_FLAG*/
				jrd_364.jrd_373 = (SSHORT)DYN_get_number(ptr);
				/*P.RDB$SYSTEM_FLAG.NULL*/
				jrd_364.jrd_372 = FALSE;
				break;

			case isc_dyn_prm_number:
				/*P.RDB$PARAMETER_NUMBER*/
				jrd_364.jrd_378 = (SSHORT)DYN_get_number(ptr);
				/*P.RDB$PARAMETER_NUMBER.NULL*/
				jrd_364.jrd_377 = FALSE;
				break;

			case isc_dyn_prm_type:
				/*P.RDB$PARAMETER_TYPE*/
				jrd_364.jrd_376 = (SSHORT)DYN_get_number(ptr);
				/*P.RDB$PARAMETER_TYPE.NULL*/
				jrd_364.jrd_375 = FALSE;
				break;

			case isc_dyn_prc_name:
				GET_STRING(ptr, /*P.RDB$PROCEDURE_NAME*/
						jrd_364.jrd_367);
				/*P.RDB$PROCEDURE_NAME.NULL*/
				jrd_364.jrd_379 = FALSE;
				prc_name = /*P.RDB$PROCEDURE_NAME*/
					   jrd_364.jrd_367;
				break;

			case isc_dyn_fld_not_null:
				if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) < ODS_11_1)
				{
					// Feature not supported on ODS version older than %d.%d
					ERR_post(Arg::Gds(isc_dsql_feature_not_supported_ods) << Arg::Num(11) <<
																			 Arg::Num(1));
				}

				f_notnull = TRUE;
				f_notnull_null = FALSE;
				break;

			case isc_dyn_fld_source:
				GET_STRING(ptr, /*P.RDB$FIELD_SOURCE*/
						jrd_364.jrd_366);
				/*P.RDB$FIELD_SOURCE.NULL*/
				jrd_364.jrd_374 = FALSE;
				explicit_domain = true;
				break;

			case isc_dyn_fld_length:
				f_length = (USHORT)DYN_get_number(ptr);
				break;

			case isc_dyn_fld_type:
				f_type = (USHORT)DYN_get_number(ptr);
				switch (f_type)
				{
				case blr_short:
					f_length = 2;
					break;

				case blr_long:
				case blr_float:
				case blr_sql_time:
				case blr_sql_date:
					f_length = 4;
					break;

				case blr_int64:
				case blr_quad:
				case blr_timestamp:
				case blr_double:
				case blr_d_float:
					f_length = 8;
					break;

				default:
					if (f_type == blr_blob)
						f_length = 8;
					break;
				}
				break;

			case isc_dyn_fld_scale:
				f_scale = (SSHORT)DYN_get_number(ptr);
				f_scale_null = FALSE;
				break;

			case isc_dyn_fld_precision:
				f_precision = (SSHORT)DYN_get_number(ptr);
				f_precision_null = FALSE;
				break;

			case isc_dyn_fld_sub_type:
				f_subtype = (SSHORT)DYN_get_number(ptr);
				f_subtype_null = FALSE;
				break;

			case isc_dyn_fld_char_length:
				f_charlength = (USHORT)DYN_get_number(ptr);
				f_charlength_null = FALSE;
				break;

			case isc_dyn_fld_character_set:
				f_charset = (SSHORT)DYN_get_number(ptr);
				f_charset_null = FALSE;
				break;

			case isc_dyn_fld_collation:
				f_collation = (SSHORT)DYN_get_number(ptr);
				f_collation_null = FALSE;
				break;

			case isc_dyn_fld_segment_length:
				f_seg_length = (USHORT)DYN_get_number(ptr);
				f_seg_length_null = FALSE;
				break;

			case isc_dyn_description:
				DYN_put_text_blob(gbl, ptr, &/*P.RDB$DESCRIPTION*/
							     jrd_364.jrd_365);
				/*P.RDB$DESCRIPTION.NULL*/
				jrd_364.jrd_371 = FALSE;
				break;

			case isc_dyn_fld_default_value:
				default_value_ptr = *ptr;
				DYN_skip_attribute(ptr);
				break;

			case isc_dyn_fld_default_source:
				default_source_ptr = *ptr;
				DYN_skip_attribute(ptr);
				break;

			case isc_dyn_prm_mechanism:
				mechanism = (prm_mech_t) DYN_get_number(ptr);
				break;

			case isc_dyn_rel_name:
				GET_STRING(ptr, rel_name);
				break;

			case isc_dyn_fld_name:
				GET_STRING(ptr, fld_name);
				break;

			default:
				--(*ptr);
				DYN_execute(gbl, ptr, NULL, NULL, NULL, NULL, procedure_name);
			}
		}

		if (/*P.RDB$FIELD_SOURCE.NULL*/
		    jrd_364.jrd_374) {
			/* Need to store dummy global field */
			jrd_req* old_request = request;
			id = drq_s_prm_src;
			request = CMP_find_request(tdbb, drq_s_prm_src, DYN_REQUESTS);

			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				PS IN RDB$FIELDS USING*/
			{
			

				// ODS_11_1
				/*P.RDB$NULL_FLAG.NULL*/
				jrd_364.jrd_369 = f_notnull_null;
				/*P.RDB$NULL_FLAG*/
				jrd_364.jrd_370 = f_notnull;

				/*PS.RDB$SYSTEM_FLAG*/
				jrd_339.jrd_362 = 0;
				/*PS.RDB$SYSTEM_FLAG.NULL*/
				jrd_339.jrd_361 = FALSE;
				DYN_UTIL_generate_field_name(tdbb, gbl, /*PS.RDB$FIELD_NAME*/
									jrd_339.jrd_342);
				strcpy(/*P.RDB$FIELD_SOURCE*/
				       jrd_364.jrd_366, /*PS.RDB$FIELD_NAME*/
  jrd_339.jrd_342);
				/*P.RDB$FIELD_SOURCE.NULL*/
				jrd_364.jrd_374 = FALSE;
				/*PS.RDB$FIELD_LENGTH*/
				jrd_339.jrd_360 = f_length;
				/*PS.RDB$FIELD_TYPE*/
				jrd_339.jrd_359 = f_type;
				/*PS.RDB$FIELD_SUB_TYPE*/
				jrd_339.jrd_358 = f_subtype;
				/*PS.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_339.jrd_357 = f_subtype_null;
				/*PS.RDB$FIELD_SCALE*/
				jrd_339.jrd_356 = f_scale;
				/*PS.RDB$FIELD_SCALE.NULL*/
				jrd_339.jrd_355 = f_scale_null;
				/*PS.RDB$FIELD_PRECISION*/
				jrd_339.jrd_354 = f_precision;
				/*PS.RDB$FIELD_PRECISION.NULL*/
				jrd_339.jrd_353 = f_precision_null;
				/*PS.RDB$SEGMENT_LENGTH*/
				jrd_339.jrd_352 = f_seg_length;
				/*PS.RDB$SEGMENT_LENGTH.NULL*/
				jrd_339.jrd_351 = f_seg_length_null;
				/*PS.RDB$CHARACTER_LENGTH*/
				jrd_339.jrd_350 = f_charlength;
				/*PS.RDB$CHARACTER_LENGTH.NULL*/
				jrd_339.jrd_349 = f_charlength_null;
				/*PS.RDB$CHARACTER_SET_ID*/
				jrd_339.jrd_348 = f_charset;
				/*PS.RDB$CHARACTER_SET_ID.NULL*/
				jrd_339.jrd_347 = f_charset_null;

				/*PS.RDB$COLLATION_ID*/
				jrd_339.jrd_346 = f_collation;
				/*PS.RDB$COLLATION_ID.NULL*/
				jrd_339.jrd_345 = f_collation_null;

				/*PS.RDB$DEFAULT_VALUE.NULL*/
				jrd_339.jrd_344 = (default_value_ptr == NULL) ? TRUE : FALSE;
				if (default_value_ptr) {
					DYN_put_blr_blob(gbl, &default_value_ptr, &/*PS.RDB$DEFAULT_VALUE*/
										   jrd_339.jrd_341);
				}

				/*PS.RDB$DEFAULT_SOURCE.NULL*/
				jrd_339.jrd_343 = (default_source_ptr == NULL) ? TRUE : FALSE;
				if (default_source_ptr) {
					DYN_put_text_blob(gbl, &default_source_ptr, &/*PS.RDB$DEFAULT_SOURCE*/
										     jrd_339.jrd_340);
				}
			/*END_STORE;*/
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_338, sizeof(jrd_338), true);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 88, (UCHAR*) &jrd_339);
			}
			if (!DYN_REQUEST(drq_s_prm_src))
				DYN_REQUEST(drq_s_prm_src) = request;
			id = drq_s_prms;
			request = old_request;
		}
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_363, sizeof(jrd_363), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 126, (UCHAR*) &jrd_364);
	}

	if (!DYN_REQUEST(drq_s_prms)) {
		DYN_REQUEST(drq_s_prms) = request;
	}

	request = NULL;
	id = -1;

	if (explicit_domain)
	{
		if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
		{
			request = CMP_find_request(tdbb, drq_s_prms2, DYN_REQUESTS);
			id = drq_s_prms2;

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				P IN RDB$PROCEDURE_PARAMETERS WITH
					P.RDB$PROCEDURE_NAME EQ prc_name.c_str() AND
					P.RDB$PARAMETER_NAME EQ parameter_name.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_309, sizeof(jrd_309), true);
			gds__vtov ((const char*) parameter_name.c_str(), (char*) jrd_310.jrd_311, 32);
			gds__vtov ((const char*) prc_name.c_str(), (char*) jrd_310.jrd_312, 32);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_310);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_313);
			   if (!jrd_313.jrd_316) break;

				if (!DYN_REQUEST(drq_s_prms2))
					DYN_REQUEST(drq_s_prms2) = request;

				/*MODIFY P USING*/
				{
				
					/*P.RDB$COLLATION_ID.NULL*/
					jrd_313.jrd_323 = f_collation_null;
					/*P.RDB$COLLATION_ID*/
					jrd_313.jrd_324 = f_collation;

					/*P.RDB$DEFAULT_VALUE.NULL*/
					jrd_313.jrd_322 = (default_value_ptr == NULL) ? TRUE : FALSE;
					if (default_value_ptr)
						DYN_put_blr_blob(gbl, &default_value_ptr, &/*P.RDB$DEFAULT_VALUE*/
											   jrd_313.jrd_315);

					/*P.RDB$DEFAULT_SOURCE.NULL*/
					jrd_313.jrd_321 = (default_source_ptr == NULL) ? TRUE : FALSE;
					if (default_source_ptr)
						DYN_put_text_blob(gbl, &default_source_ptr, &/*P.RDB$DEFAULT_SOURCE*/
											     jrd_313.jrd_314);

					/*P.RDB$NULL_FLAG.NULL*/
					jrd_313.jrd_319 = f_notnull_null;
					/*P.RDB$NULL_FLAG*/
					jrd_313.jrd_320 = f_notnull;

					/*P.RDB$PARAMETER_MECHANISM.NULL*/
					jrd_313.jrd_317 = FALSE;
					/*P.RDB$PARAMETER_MECHANISM*/
					jrd_313.jrd_318 = (USHORT) mechanism;
				/*END_MODIFY*/
				jrd_325.jrd_326 = jrd_313.jrd_315;
				jrd_325.jrd_327 = jrd_313.jrd_314;
				jrd_325.jrd_328 = jrd_313.jrd_323;
				jrd_325.jrd_329 = jrd_313.jrd_324;
				jrd_325.jrd_330 = jrd_313.jrd_322;
				jrd_325.jrd_331 = jrd_313.jrd_321;
				jrd_325.jrd_332 = jrd_313.jrd_319;
				jrd_325.jrd_333 = jrd_313.jrd_320;
				jrd_325.jrd_334 = jrd_313.jrd_317;
				jrd_325.jrd_335 = jrd_313.jrd_318;
				EXE_send (tdbb, request, 2, 32, (UCHAR*) &jrd_325);
				}
			/*END_FOR*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_336);
			   }
			}

			if (!DYN_REQUEST(drq_s_prms2))
				DYN_REQUEST(drq_s_prms2) = request;
		}
		else
		{
			// Feature not supported on ODS version older than %d.%d
			ERR_post(Arg::Gds(isc_dsql_feature_not_supported_ods) << Arg::Num(11) << Arg::Num(1));
		}
	}

	if (rel_name.hasData() && fld_name.hasData())
	{
		if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_2)
		{
			request = CMP_find_request(tdbb, drq_s_prms3, DYN_REQUESTS);
			id = drq_s_prms3;

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				P IN RDB$PROCEDURE_PARAMETERS WITH
					P.RDB$PROCEDURE_NAME EQ prc_name.c_str() AND
					P.RDB$PARAMETER_NAME EQ parameter_name.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_292, sizeof(jrd_292), true);
			gds__vtov ((const char*) parameter_name.c_str(), (char*) jrd_293.jrd_294, 32);
			gds__vtov ((const char*) prc_name.c_str(), (char*) jrd_293.jrd_295, 32);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_293);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 70, (UCHAR*) &jrd_296);
			   if (!jrd_296.jrd_299) break;

				if (!DYN_REQUEST(drq_s_prms3))
					DYN_REQUEST(drq_s_prms3) = request;

				/*MODIFY P USING*/
				{
				
					/*P.RDB$RELATION_NAME.NULL*/
					jrd_296.jrd_301 = FALSE;
					strcpy(/*P.RDB$RELATION_NAME*/
					       jrd_296.jrd_298, rel_name.c_str());

					/*P.RDB$FIELD_NAME.NULL*/
					jrd_296.jrd_300 = FALSE;
					strcpy(/*P.RDB$FIELD_NAME*/
					       jrd_296.jrd_297, fld_name.c_str());
				/*END_MODIFY*/
				gds__vtov((const char*) jrd_296.jrd_298, (char*) jrd_302.jrd_303, 32);
				gds__vtov((const char*) jrd_296.jrd_297, (char*) jrd_302.jrd_304, 32);
				jrd_302.jrd_305 = jrd_296.jrd_301;
				jrd_302.jrd_306 = jrd_296.jrd_300;
				EXE_send (tdbb, request, 2, 68, (UCHAR*) &jrd_302);
				}
			/*END_FOR*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_307);
			   }
			}

			if (!DYN_REQUEST(drq_s_prms3))
				DYN_REQUEST(drq_s_prms3) = request;
		}
		else
		{
			request = NULL;
			id = -1;

			ERR_post(Arg::Gds(isc_dsql_feature_not_supported_ods) << Arg::Num(11) << Arg::Num(2));
		}
	}

	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);

		switch (id)
		{
		case drq_s_prms:
		case drq_s_prm_src:
		case drq_s_prms2:
		case drq_s_prms3:
			DYN_rundown_request(request, id);
			// fall down
		case -1:
			DYN_error_punt(true, 136);
			// msg 136: "STORE RDB$PROCEDURE_PARAMETERS failed"
		}

		DYN_rundown_request(request, -1);

		/* Control should never reach this point,
		   because id should always have one of the values tested above.  */
		fb_assert(0);
		DYN_error_punt(true, 0);
	}
}


void DYN_define_procedure( Global* gbl, const UCHAR** ptr)
{
   struct {
          TEXT  jrd_249 [32];	// RDB$USER 
          TEXT  jrd_250 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_251;	// RDB$OBJECT_TYPE 
          SSHORT jrd_252;	// RDB$USER_TYPE 
          TEXT  jrd_253 [7];	// RDB$PRIVILEGE 
   } jrd_248;
   struct {
          SSHORT jrd_273;	// gds__utility 
   } jrd_272;
   struct {
          bid  jrd_266;	// RDB$DEBUG_INFO 
          SSHORT jrd_267;	// gds__null_flag 
          SSHORT jrd_268;	// RDB$PROCEDURE_TYPE 
          SSHORT jrd_269;	// gds__null_flag 
          SSHORT jrd_270;	// RDB$VALID_BLR 
          SSHORT jrd_271;	// gds__null_flag 
   } jrd_265;
   struct {
          bid  jrd_258;	// RDB$DEBUG_INFO 
          SSHORT jrd_259;	// gds__utility 
          SSHORT jrd_260;	// gds__null_flag 
          SSHORT jrd_261;	// gds__null_flag 
          SSHORT jrd_262;	// RDB$VALID_BLR 
          SSHORT jrd_263;	// gds__null_flag 
          SSHORT jrd_264;	// RDB$PROCEDURE_TYPE 
   } jrd_257;
   struct {
          TEXT  jrd_256 [32];	// RDB$PROCEDURE_NAME 
   } jrd_255;
   struct {
          bid  jrd_276;	// RDB$DESCRIPTION 
          TEXT  jrd_277 [32];	// RDB$SECURITY_CLASS 
          bid  jrd_278;	// RDB$PROCEDURE_SOURCE 
          bid  jrd_279;	// RDB$PROCEDURE_BLR 
          TEXT  jrd_280 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_281;	// gds__null_flag 
          SSHORT jrd_282;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_283;	// gds__null_flag 
          SSHORT jrd_284;	// RDB$PROCEDURE_OUTPUTS 
          SSHORT jrd_285;	// gds__null_flag 
          SSHORT jrd_286;	// RDB$PROCEDURE_INPUTS 
          SSHORT jrd_287;	// gds__null_flag 
          SSHORT jrd_288;	// gds__null_flag 
          SSHORT jrd_289;	// gds__null_flag 
          SSHORT jrd_290;	// gds__null_flag 
          SSHORT jrd_291;	// RDB$PROCEDURE_ID 
   } jrd_275;
/**************************************
 *
 *	D Y N _ d e f i n e _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement.
 *
 **************************************/
	Firebird::MetaName procedure_name;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	GET_STRING(ptr, procedure_name);

	if (procedure_name.length() == 0)
		DYN_error_punt(false, 212);
		/* msg 212: "Zero length identifiers not allowed" */

	jrd_req* request = NULL;
	SSHORT id = -1;

	try {

	id = drq_l_prc_name;
	check_unique_name(tdbb, gbl, procedure_name, obj_procedure);

	bool sql_prot = false;
	prc_t prc_type = prc_legacy;
	SSHORT sys_flag = 0, inputs = -1, outputs = -1;
	const UCHAR* blr_ptr = NULL;
	const UCHAR* description_ptr = NULL;
	const UCHAR* source_ptr = NULL;
	const UCHAR* debug_info_ptr = NULL;
	Firebird::MetaName security_class;

	UCHAR verb;
	while ((verb = *(*ptr)++) != isc_dyn_end)
	{
		switch (verb)
		{
		case isc_dyn_system_flag:
			sys_flag = (SSHORT) DYN_get_number(ptr);
			break;

		case isc_dyn_prc_blr:
			blr_ptr = *ptr;
			DYN_skip_attribute(ptr);
			break;

		case isc_dyn_description:
			description_ptr = *ptr;
			DYN_skip_attribute(ptr);
			break;

		case isc_dyn_prc_source:
			source_ptr = *ptr;
			DYN_skip_attribute(ptr);
			break;

		case isc_dyn_prc_inputs:
			inputs = (SSHORT) DYN_get_number(ptr);
			break;

		case isc_dyn_prc_outputs:
			outputs = (SSHORT) DYN_get_number(ptr);
			break;

		case isc_dyn_prc_type:
			prc_type = (prc_t) DYN_get_number(ptr);
			break;

		case isc_dyn_security_class:
			GET_STRING(ptr, security_class);
			break;

		case isc_dyn_rel_sql_protection:
			sql_prot = (bool) DYN_get_number(ptr);
			break;

		case isc_dyn_debug_info:
			debug_info_ptr = *ptr;
			DYN_skip_attribute(ptr);
			break;

		default:
			--(*ptr);
			DYN_execute(gbl, ptr, NULL, NULL, NULL, NULL, &procedure_name);
		}
	}

	request = CMP_find_request(tdbb, drq_s_prcs, DYN_REQUESTS);
	id = drq_s_prcs;

	int faults = 0;
	const UCHAR* temp_ptr;

	while (true)
	{
		try
		{
			SINT64 prc_id =
				DYN_UTIL_gen_unique_id(tdbb, gbl, drq_g_nxt_prc_id, "RDB$PROCEDURES");

			prc_id %= (MAX_SSHORT + 1);

			if (!prc_id)
				continue;

			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				P IN RDB$PROCEDURES*/
			{
			

				/*P.RDB$PROCEDURE_ID*/
				jrd_275.jrd_291 = prc_id;
				strcpy(/*P.RDB$PROCEDURE_NAME*/
				       jrd_275.jrd_280, procedure_name.c_str());

				/*P.RDB$PROCEDURE_BLR.NULL*/
				jrd_275.jrd_290 = TRUE;
				/*P.RDB$PROCEDURE_SOURCE.NULL*/
				jrd_275.jrd_289 = TRUE;
				/*P.RDB$SECURITY_CLASS.NULL*/
				jrd_275.jrd_288 = TRUE;
				/*P.RDB$DESCRIPTION.NULL*/
				jrd_275.jrd_287 = TRUE;
				/*P.RDB$PROCEDURE_INPUTS.NULL*/
				jrd_275.jrd_285 = TRUE;
				/*P.RDB$PROCEDURE_OUTPUTS.NULL*/
				jrd_275.jrd_283 = TRUE;

				/*P.RDB$SYSTEM_FLAG.NULL*/
				jrd_275.jrd_281 = FALSE;
				/*P.RDB$SYSTEM_FLAG*/
				jrd_275.jrd_282 = sys_flag;

				if (blr_ptr) {
					/*P.RDB$PROCEDURE_BLR.NULL*/
					jrd_275.jrd_290 = FALSE;
					temp_ptr = blr_ptr;
					DYN_put_blr_blob(gbl, &temp_ptr, &/*P.RDB$PROCEDURE_BLR*/
									  jrd_275.jrd_279);
				}

				if (source_ptr) {
					/*P.RDB$PROCEDURE_SOURCE.NULL*/
					jrd_275.jrd_289 = FALSE;
					temp_ptr = source_ptr;
					DYN_put_text_blob(gbl, &temp_ptr, &/*P.RDB$PROCEDURE_SOURCE*/
									   jrd_275.jrd_278);
				}

				if (description_ptr) {
					/*P.RDB$DESCRIPTION.NULL*/
					jrd_275.jrd_287 = FALSE;
					temp_ptr = description_ptr;
					DYN_put_text_blob(gbl, &temp_ptr, &/*P.RDB$DESCRIPTION*/
									   jrd_275.jrd_276);
				}

				if (inputs >= 0) {
					/*P.RDB$PROCEDURE_INPUTS.NULL*/
					jrd_275.jrd_285 = FALSE;
					/*P.RDB$PROCEDURE_INPUTS*/
					jrd_275.jrd_286 = inputs;
				}

				if (outputs >= 0) {
					/*P.RDB$PROCEDURE_OUTPUTS.NULL*/
					jrd_275.jrd_283 = FALSE;
					/*P.RDB$PROCEDURE_OUTPUTS*/
					jrd_275.jrd_284 = outputs;
				}

				if (security_class.length()) {
					/*P.RDB$SECURITY_CLASS.NULL*/
					jrd_275.jrd_288 = FALSE;
					GET_STRING(security_class.c_str(), /*P.RDB$SECURITY_CLASS*/
									   jrd_275.jrd_277);
				}

			/*END_STORE;*/
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_274, sizeof(jrd_274), true);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 110, (UCHAR*) &jrd_275);
			}

			break;
		}
		catch (const Firebird::status_exception& ex)
		{
			DYN_rundown_request(request, id);

			if (ex.value()[1] != isc_no_dup)
				throw;

			if (++faults > MAX_SSHORT)
				throw;

			fb_utils::init_status(tdbb->tdbb_status_vector);
		}
	}

	if (!DYN_REQUEST(drq_s_prcs))
		DYN_REQUEST(drq_s_prcs) = request;

	if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
	{
		jrd_req* sub_request = NULL;

		/*FOR(REQUEST_HANDLE sub_request TRANSACTION_HANDLE gbl->gbl_transaction)
			P IN RDB$PROCEDURES WITH P.RDB$PROCEDURE_NAME EQ procedure_name.c_str()*/
		{
		if (!sub_request)
		   sub_request = CMP_compile2 (tdbb, (UCHAR*) jrd_254, sizeof(jrd_254), true);
		gds__vtov ((const char*) procedure_name.c_str(), (char*) jrd_255.jrd_256, 32);
		EXE_start (tdbb, sub_request, gbl->gbl_transaction);
		EXE_send (tdbb, sub_request, 0, 32, (UCHAR*) &jrd_255);
		while (1)
		   {
		   EXE_receive (tdbb, sub_request, 1, 20, (UCHAR*) &jrd_257);
		   if (!jrd_257.jrd_259) break;

			/*MODIFY P USING*/
			{
			
				/*P.RDB$PROCEDURE_TYPE*/
				jrd_257.jrd_264 = prc_type;
				/*P.RDB$PROCEDURE_TYPE.NULL*/
				jrd_257.jrd_263 = FALSE;

				/*P.RDB$VALID_BLR*/
				jrd_257.jrd_262 = TRUE;
				/*P.RDB$VALID_BLR.NULL*/
				jrd_257.jrd_261 = FALSE;

				/*P.RDB$DEBUG_INFO.NULL*/
				jrd_257.jrd_260 = (debug_info_ptr == NULL) ? TRUE : FALSE;
				if (debug_info_ptr)
					DYN_put_blr_blob(gbl, &debug_info_ptr, &/*P.RDB$DEBUG_INFO*/
										jrd_257.jrd_258);
			/*END_MODIFY;*/
			jrd_265.jrd_266 = jrd_257.jrd_258;
			jrd_265.jrd_267 = jrd_257.jrd_263;
			jrd_265.jrd_268 = jrd_257.jrd_264;
			jrd_265.jrd_269 = jrd_257.jrd_261;
			jrd_265.jrd_270 = jrd_257.jrd_262;
			jrd_265.jrd_271 = jrd_257.jrd_260;
			EXE_send (tdbb, sub_request, 2, 18, (UCHAR*) &jrd_265);
			}
		/*END_FOR;*/
		   EXE_send (tdbb, sub_request, 3, 2, (UCHAR*) &jrd_272);
		   }
		}

		CMP_release(tdbb, sub_request);
	}

	if (sql_prot) {
		Firebird::MetaName owner_name;
		if (!get_who(tdbb, gbl, owner_name))
			DYN_error_punt(true, 134);
			/* msg 134: "STORE RDB$PROCEDURES failed" */

		for (const TEXT* p = ALL_PROC_PRIVILEGES; *p; p++) {
			request = CMP_find_request(tdbb, drq_s_prc_usr_prvs, DYN_REQUESTS);
			id = drq_s_prc_usr_prvs;

			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				X IN RDB$USER_PRIVILEGES*/
			{
			
				strcpy(/*X.RDB$RELATION_NAME*/
				       jrd_248.jrd_250, procedure_name.c_str());
				strcpy(/*X.RDB$USER*/
				       jrd_248.jrd_249, owner_name.c_str());
				/*X.RDB$USER_TYPE*/
				jrd_248.jrd_252 = obj_user;
				/*X.RDB$OBJECT_TYPE*/
				jrd_248.jrd_251 = obj_procedure;
				/*X.RDB$PRIVILEGE*/
				jrd_248.jrd_253[0] = *p;
				/*X.RDB$PRIVILEGE*/
				jrd_248.jrd_253[1] = 0;
			/*END_STORE;*/
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_247, sizeof(jrd_247), true);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 75, (UCHAR*) &jrd_248);
			}

			if (!DYN_REQUEST(drq_s_prc_usr_prvs))
				DYN_REQUEST(drq_s_prc_usr_prvs) = request;
		}
	}

	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (id == drq_s_prcs) {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 134);
			/* msg 134: "STORE RDB$PROCEDURES failed" */
		}
		else if (id == drq_s_prc_usr_prvs) {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 25);
			/* msg 25: "STORE RDB$USER_PRIVILEGES failed defining a relation" */
		}

		DYN_rundown_request(request, -1);

		if (id == drq_l_prc_name) {
			DYN_error_punt(true, 134);
			/* msg 134: "STORE RDB$PROCEDURES failed" */
		}

		/* Control should never reach this point, because id should have
		   one of the values tested-for above. */
		fb_assert(false);
		DYN_error_punt(true, 0);
	}
}


void DYN_define_relation( Global* gbl, const UCHAR** ptr)
{
   struct {
          TEXT  jrd_205 [32];	// RDB$USER 
          TEXT  jrd_206 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_207;	// RDB$GRANT_OPTION 
          SSHORT jrd_208;	// RDB$OBJECT_TYPE 
          SSHORT jrd_209;	// RDB$USER_TYPE 
          TEXT  jrd_210 [7];	// RDB$PRIVILEGE 
   } jrd_204;
   struct {
          SSHORT jrd_222;	// gds__utility 
   } jrd_221;
   struct {
          SSHORT jrd_219;	// gds__null_flag 
          SSHORT jrd_220;	// RDB$RELATION_TYPE 
   } jrd_218;
   struct {
          SSHORT jrd_215;	// gds__utility 
          SSHORT jrd_216;	// gds__null_flag 
          SSHORT jrd_217;	// RDB$RELATION_TYPE 
   } jrd_214;
   struct {
          TEXT  jrd_213 [32];	// RDB$RELATION_NAME 
   } jrd_212;
   struct {
          TEXT  jrd_227 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_228 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_229;	// gds__utility 
   } jrd_226;
   struct {
          TEXT  jrd_225 [32];	// RDB$VIEW_NAME 
   } jrd_224;
   struct {
          TEXT  jrd_232 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_233;	// RDB$DESCRIPTION 
          TEXT  jrd_234 [32];	// RDB$SECURITY_CLASS 
          bid  jrd_235;	// RDB$VIEW_SOURCE 
          bid  jrd_236;	// RDB$VIEW_BLR 
          TEXT  jrd_237 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_238;	// gds__null_flag 
          SSHORT jrd_239;	// RDB$FLAGS 
          SSHORT jrd_240;	// gds__null_flag 
          SSHORT jrd_241;	// gds__null_flag 
          SSHORT jrd_242;	// gds__null_flag 
          SSHORT jrd_243;	// gds__null_flag 
          SSHORT jrd_244;	// gds__null_flag 
          SSHORT jrd_245;	// gds__null_flag 
          SSHORT jrd_246;	// RDB$SYSTEM_FLAG 
   } jrd_231;
/**************************************
 *
 *	D Y N _ d e f i n e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement.
 *
 **************************************/
	Firebird::MetaName relation_name, owner_name;
	Firebird::MetaName field_name; // unused, only passed empty to DYN_execute again.

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	GET_STRING(ptr, relation_name);

	if (relation_name.length() == 0)
	{
		DYN_error_punt(false, 212);
		/* msg 212: "Zero length identifiers not allowed" */
	}

	jrd_req* request = NULL;
	SSHORT id = -1;

	Firebird::PathName Path, Name;

	try {

	id = drq_l_rel_name;
	check_unique_name(tdbb, gbl, relation_name, obj_relation);
	bool sql_prot = false;
	rel_t rel_type = rel_persistent;

	request = CMP_find_request(tdbb, drq_s_rels, DYN_REQUESTS);
	id = drq_s_rels;

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		REL IN RDB$RELATIONS*/
	{
	
		strcpy(/*REL.RDB$RELATION_NAME*/
		       jrd_231.jrd_237, relation_name.c_str());
		/*REL.RDB$SYSTEM_FLAG*/
		jrd_231.jrd_246 = 0;
		/*REL.RDB$SYSTEM_FLAG.NULL*/
		jrd_231.jrd_245 = FALSE;
		/*REL.RDB$VIEW_BLR.NULL*/
		jrd_231.jrd_244 = TRUE;
		/*REL.RDB$VIEW_SOURCE.NULL*/
		jrd_231.jrd_243 = TRUE;
		/*REL.RDB$SECURITY_CLASS.NULL*/
		jrd_231.jrd_242 = TRUE;
		/*REL.RDB$DESCRIPTION.NULL*/
		jrd_231.jrd_241 = TRUE;
		/*REL.RDB$EXTERNAL_FILE.NULL*/
		jrd_231.jrd_240 = TRUE;
		/*REL.RDB$FLAGS*/
		jrd_231.jrd_239 = 0;
		/*REL.RDB$FLAGS.NULL*/
		jrd_231.jrd_238 = FALSE;

		UCHAR verb;
		while ((verb = *(*ptr)++) != isc_dyn_end)
		{
			switch (verb)
			{
			case isc_dyn_system_flag:
				/*REL.RDB$SYSTEM_FLAG*/
				jrd_231.jrd_246 = DYN_get_number(ptr);
				/*REL.RDB$SYSTEM_FLAG.NULL*/
				jrd_231.jrd_245 = FALSE;
				break;

			case isc_dyn_sql_object:
				/*REL.RDB$FLAGS*/
				jrd_231.jrd_239 |= REL_sql;
				break;

			case isc_dyn_view_blr:
				/*REL.RDB$VIEW_BLR.NULL*/
				jrd_231.jrd_244 = FALSE;
				rel_type = rel_view;
				DYN_put_blr_blob(gbl, ptr, &/*REL.RDB$VIEW_BLR*/
							    jrd_231.jrd_236);
				break;

			case isc_dyn_description:
				DYN_put_text_blob(gbl, ptr, &/*REL.RDB$DESCRIPTION*/
							     jrd_231.jrd_233);
				/*REL.RDB$DESCRIPTION.NULL*/
				jrd_231.jrd_241 = FALSE;
				break;

			case isc_dyn_view_source:
				DYN_put_text_blob(gbl, ptr, &/*REL.RDB$VIEW_SOURCE*/
							     jrd_231.jrd_235);
				/*REL.RDB$VIEW_SOURCE.NULL*/
				jrd_231.jrd_243 = FALSE;
				break;

			case isc_dyn_security_class:
				GET_STRING(ptr, /*REL.RDB$SECURITY_CLASS*/
						jrd_231.jrd_234);
				/*REL.RDB$SECURITY_CLASS.NULL*/
				jrd_231.jrd_242 = FALSE;
				break;

			case isc_dyn_rel_ext_file:
				GET_STRING(ptr, /*REL.RDB$EXTERNAL_FILE*/
						jrd_231.jrd_232);
				if (ISC_check_if_remote(/*REL.RDB$EXTERNAL_FILE*/
							jrd_231.jrd_232, false))
					DYN_error_punt(true, 163);

				// Check for any path, present in filename.
				// If miss it, file will be searched in External Tables Dirs,
				// that's why no expand_filename required.
				PathUtils::splitLastComponent(Path, Name, /*REL.RDB$EXTERNAL_FILE*/
									  jrd_231.jrd_232);
				if (Path.length() > 0)	// path component present in filename
				{
					ISC_expand_filename(/*REL.RDB$EXTERNAL_FILE*/
							    jrd_231.jrd_232,
										strlen(/*REL.RDB$EXTERNAL_FILE*/
										       jrd_231.jrd_232),
										/*REL.RDB$EXTERNAL_FILE*/
										jrd_231.jrd_232,
										sizeof(/*REL.RDB$EXTERNAL_FILE*/
										       jrd_231.jrd_232),
										false);
				}
				/*REL.RDB$EXTERNAL_FILE.NULL*/
				jrd_231.jrd_240 = FALSE;
				rel_type = rel_external;
				break;

			case isc_dyn_rel_sql_protection:
				/*REL.RDB$FLAGS*/
				jrd_231.jrd_239 |= REL_sql;
				sql_prot = (bool) DYN_get_number(ptr);
				break;

			case isc_dyn_rel_temporary:
				if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) < ODS_11_1)
				{
					// msg 248: "Feature ''@1'' is not supported in ODS @2.@3"
					DYN_error_punt(false, 248, SafeArg() << "GLOBAL TEMPORARY TABLE" <<
						dbb->dbb_ods_version << dbb->dbb_minor_original);
				}
				switch (DYN_get_number(ptr))
				{
					case isc_dyn_rel_temp_global_preserve:
						rel_type = rel_global_temp_preserve;
						break;

					case isc_dyn_rel_temp_global_delete:
						rel_type = rel_global_temp_delete;
						break;

					default:
						fb_assert(false);
				}
				break;

			default:
				--(*ptr);
				MetaTmp(/*REL.RDB$RELATION_NAME*/
					jrd_231.jrd_237)
					DYN_execute(gbl, ptr, &tmp, &field_name, NULL, NULL, NULL);
			}
		}

		SSHORT old_id = id;
		id = drq_l_rel_info2;
		check_relation_temp_scope(tdbb, gbl, /*REL.RDB$RELATION_NAME*/
						     jrd_231.jrd_237, rel_type);
		id = old_id;

		if (sql_prot) {
			if (!get_who(tdbb, gbl, owner_name))
				DYN_error_punt(true, 115);
			/* msg 115: "CREATE VIEW failed" */

			if (rel_type == rel_view) {
				jrd_req* old_request = request;
				const SSHORT old_id2 = id;
				request = CMP_find_request(tdbb, drq_l_view_rels, DYN_REQUESTS);
				id = drq_l_view_rels;

				/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
					VRL IN RDB$VIEW_RELATIONS CROSS
						PREL IN RDB$RELATIONS OVER RDB$RELATION_NAME WITH
						VRL.RDB$VIEW_NAME EQ relation_name.c_str()*/
				{
				if (!request)
				   request = CMP_compile2 (tdbb, (UCHAR*) jrd_223, sizeof(jrd_223), true);
				gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_224.jrd_225, 32);
				EXE_start (tdbb, request, gbl->gbl_transaction);
				EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_224);
				while (1)
				   {
				   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_226);
				   if (!jrd_226.jrd_229) break;
					if (!DYN_REQUEST(drq_l_view_rels))
						DYN_REQUEST(drq_l_view_rels) = request;

					/* CVC: This never matches so it causes unnecessary calls to verify,
							so I included a call to strip trailing blanks. */
					fb_utils::exact_name_limit(/*PREL.RDB$OWNER_NAME*/
								   jrd_226.jrd_228, sizeof(/*PREL.RDB$OWNER_NAME*/
	 jrd_226.jrd_228));
					if (owner_name != /*PREL.RDB$OWNER_NAME*/
							  jrd_226.jrd_228) {
						SecurityClass::flags_t priv;
						if (!DYN_UTIL_get_prot(tdbb, gbl, /*PREL.RDB$RELATION_NAME*/
										  jrd_226.jrd_227, "", &priv))
						{
							// I think this should be the responsability of DFW
							// or the user will find ways to circumvent DYN.
							DYN_error_punt(true, 115);
							/* msg 115: "CREATE VIEW failed" */
						}

						if (!(priv & SCL_read)) {
							ERR_post_nothrow(Arg::Gds(isc_no_priv) << Arg::Str("SELECT") <<		//	Non-Translatable
															// Remember, a view may be based on a view.
																	  Arg::Str("TABLE/VIEW") <<	//  Non-Translatable
															// We want to print the name of the base table or view.
																	  Arg::Str(/*PREL.RDB$RELATION_NAME*/
																		   jrd_226.jrd_227));
							/* msg 32: no permission for %s access to %s %s */
							DYN_error_punt(true, 115);
							/* msg 115: "CREATE VIEW failed" */
						}
					}
				/*END_FOR;*/
				   }
				}
				if (!DYN_REQUEST(drq_l_view_rels))
					DYN_REQUEST(drq_l_view_rels) = request;
				request = old_request;
				id = old_id2;
			}
		}
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_230, sizeof(jrd_230), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 362, (UCHAR*) &jrd_231);
	}

	if (!DYN_REQUEST(drq_s_rels))
		DYN_REQUEST(drq_s_rels) = request;

	if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
	{
		jrd_req* sub_request = NULL;

		/*FOR(REQUEST_HANDLE sub_request TRANSACTION_HANDLE gbl->gbl_transaction)
			REL IN RDB$RELATIONS WITH REL.RDB$RELATION_NAME EQ relation_name.c_str()*/
		{
		if (!sub_request)
		   sub_request = CMP_compile2 (tdbb, (UCHAR*) jrd_211, sizeof(jrd_211), true);
		gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_212.jrd_213, 32);
		EXE_start (tdbb, sub_request, gbl->gbl_transaction);
		EXE_send (tdbb, sub_request, 0, 32, (UCHAR*) &jrd_212);
		while (1)
		   {
		   EXE_receive (tdbb, sub_request, 1, 6, (UCHAR*) &jrd_214);
		   if (!jrd_214.jrd_215) break;

			/*MODIFY REL USING*/
			{
			
				/*REL.RDB$RELATION_TYPE*/
				jrd_214.jrd_217 = rel_type;
				/*REL.RDB$RELATION_TYPE.NULL*/
				jrd_214.jrd_216 = FALSE;
			/*END_MODIFY;*/
			jrd_218.jrd_219 = jrd_214.jrd_216;
			jrd_218.jrd_220 = jrd_214.jrd_217;
			EXE_send (tdbb, sub_request, 2, 4, (UCHAR*) &jrd_218);
			}
		/*END_FOR;*/
		   EXE_send (tdbb, sub_request, 3, 2, (UCHAR*) &jrd_221);
		   }
		}

		CMP_release(tdbb, sub_request);
	}

	if (sql_prot)
		for (const TEXT* p = ALL_PRIVILEGES; *p; p++) {
			request = CMP_find_request(tdbb, drq_s_usr_prvs, DYN_REQUESTS);
			id = drq_s_usr_prvs;

			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				X IN RDB$USER_PRIVILEGES*/
			{
			
				strcpy(/*X.RDB$RELATION_NAME*/
				       jrd_204.jrd_206, relation_name.c_str());
				strcpy(/*X.RDB$USER*/
				       jrd_204.jrd_205, owner_name.c_str());
				/*X.RDB$USER_TYPE*/
				jrd_204.jrd_209 = obj_user;
				/*X.RDB$OBJECT_TYPE*/
				jrd_204.jrd_208 = obj_relation;
				/*X.RDB$PRIVILEGE*/
				jrd_204.jrd_210[0] = *p;
				/*X.RDB$PRIVILEGE*/
				jrd_204.jrd_210[1] = 0;
				/*X.RDB$GRANT_OPTION*/
				jrd_204.jrd_207 = 1;
			/*END_STORE;*/
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_203, sizeof(jrd_203), true);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 77, (UCHAR*) &jrd_204);
			}

			if (!DYN_REQUEST(drq_s_usr_prvs))
				DYN_REQUEST(drq_s_usr_prvs) = request;
		}

	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (id == drq_s_rels) {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 24);
			/* msg 24: "STORE RDB$RELATIONS failed" */
		}
		else if (id == drq_s_usr_prvs) {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 25);
			/* msg 25: "STORE RDB$USER_PRIVILEGES failed defining a relation" */
		}

		DYN_rundown_request(request, -1);

		switch (id)
		{
		case drq_l_rel_name:
			DYN_error_punt(true, 24); /* msg 24: "STORE RDB$RELATIONS failed" */
			break;
		case drq_l_view_rels:
			DYN_error_punt(true, 115); /* msg 115: "CREATE VIEW failed" */
			break;
		case drq_l_rel_info2:
			ERR_punt();
			break;
		}

		/* Control should never reach this point, because id should
		   always have one of the values test-for above. */
		fb_assert(false);
		DYN_error_punt(true, 0);
	}
}


void DYN_define_role( Global* gbl, const UCHAR** ptr)
{
   struct {
          TEXT  jrd_195 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_196 [32];	// RDB$ROLE_NAME 
          SSHORT jrd_197;	// gds__null_flag 
          SSHORT jrd_198;	// RDB$SYSTEM_FLAG 
   } jrd_194;
   struct {
          TEXT  jrd_201 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_202 [32];	// RDB$ROLE_NAME 
   } jrd_200;
/**************************************
 *
 *	D Y N _ d e f i n e _ r o l e
 *
 **************************************
 *
 * Functional description
 *
 *     Define a SQL role.
 *     ROLES cannot be named the same as any existing user name
 *
 **************************************/
	jrd_req* request = NULL;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	const USHORT major_version  = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	if (ENCODE_ODS(major_version, minor_original) < ODS_9_0)
	{
		DYN_error_punt(false, 196);
	}

	Firebird::MetaName owner_name(tdbb->getAttachment()->att_user->usr_user_name);
	owner_name.upper7();

	Firebird::MetaName role_name;
	GET_STRING(ptr, role_name);

	if (role_name == owner_name) {
		/************************************************
		**
		** user name could not be used for SQL role
		**
		*************************************************/
		DYN_error(false, 193, SafeArg() << owner_name.c_str());
		ERR_punt();
	}

	if (role_name == NULL_ROLE) {
		/************************************************
		**
		** keyword NONE could not be used as SQL role name
		**
		*************************************************/
		DYN_error(false, 195, SafeArg() << role_name.c_str());
		ERR_punt();
	}

	try {

		if (is_it_user_name(gbl, role_name, tdbb)) {
			/************************************************
			**
			** user name could not be used for SQL role
			**
			*************************************************/
			DYN_error(false, 193, SafeArg() << role_name.c_str());
			goto do_err_punt;
		}

		Firebird::MetaName dummy_name;
		if (DYN_is_it_sql_role(gbl, role_name, dummy_name, tdbb)) {
			/************************************************
			**
			** SQL role already exist
			**
			*************************************************/
			DYN_error(false, 194, SafeArg() << role_name.c_str());
			goto do_err_punt;
		}

		request = CMP_find_request(tdbb, drq_role_gens, DYN_REQUESTS);

		if (ENCODE_ODS(major_version, minor_original) < ODS_11_0)
		{
			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				X IN RDB$ROLES*/
			{
			
				strcpy(/*X.RDB$ROLE_NAME*/
				       jrd_200.jrd_202, role_name.c_str());
				strcpy(/*X.RDB$OWNER_NAME*/
				       jrd_200.jrd_201, owner_name.c_str());
			/*END_STORE;*/
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_199, sizeof(jrd_199), true);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_200);
			}
		}
		else
		{
			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				X IN RDB$ROLES*/
			{
			
				strcpy(/*X.RDB$ROLE_NAME*/
				       jrd_194.jrd_196, role_name.c_str());
				strcpy(/*X.RDB$OWNER_NAME*/
				       jrd_194.jrd_195, owner_name.c_str());
				/*X.RDB$SYSTEM_FLAG*/
				jrd_194.jrd_198 = 0;
				/*X.RDB$SYSTEM_FLAG.NULL*/
				jrd_194.jrd_197 = FALSE;
			/*END_STORE;*/
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_193, sizeof(jrd_193), true);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_194);
			}
		}

		if (!DYN_REQUEST(drq_role_gens)) {
			DYN_REQUEST(drq_role_gens) = request;
		}

		if (*(*ptr)++ != isc_dyn_end) {
			goto do_error_punt_9;
		}
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (request) {
			DYN_rundown_request(request, drq_role_gens);
		}
		DYN_error_punt(true, 8);
		/* msg 8: "DEFINE ROLE failed" */
	}

	return;

do_err_punt:
	ERR_punt();
	return;

do_error_punt_9:
	DYN_error_punt(true, 9);
	/* msg 9: "DEFINE ROLE unexpected dyn verb" */
}


void DYN_define_security_class( Global* gbl, const UCHAR** ptr)
{
   struct {
          bid  jrd_188;	// RDB$DESCRIPTION 
          bid  jrd_189;	// RDB$ACL 
          TEXT  jrd_190 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_191;	// gds__null_flag 
          SSHORT jrd_192;	// gds__null_flag 
   } jrd_187;
/**************************************
 *
 *	D Y N _ d e f i n e _ s e c u r i t y _ c l a s s
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();
	jrd_req* request = CMP_find_request(tdbb, drq_s_classes, DYN_REQUESTS);

	bool b_ending_store = false;

	try {

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			SC IN RDB$SECURITY_CLASSES*/
		{
		
			GET_STRING(ptr, /*SC.RDB$SECURITY_CLASS*/
					jrd_187.jrd_190);
			/*SC.RDB$ACL.NULL*/
			jrd_187.jrd_192 = TRUE;
			/*SC.RDB$DESCRIPTION.NULL*/
			jrd_187.jrd_191 = TRUE;

			UCHAR verb;
			while ((verb = *(*ptr)++) != isc_dyn_end)
			{
				switch (verb)
				{
				case isc_dyn_scl_acl:
					DYN_put_blr_blob(gbl, ptr, &/*SC.RDB$ACL*/
								    jrd_187.jrd_189);
					/*SC.RDB$ACL.NULL*/
					jrd_187.jrd_192 = FALSE;
					break;

				case isc_dyn_description:
					DYN_put_text_blob(gbl, ptr, &/*SC.RDB$DESCRIPTION*/
								     jrd_187.jrd_188);
					/*SC.RDB$DESCRIPTION.NULL*/
					jrd_187.jrd_191 = FALSE;
					break;

				default:
					DYN_unsupported_verb();
				}
			}

			b_ending_store = true;

		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_186, sizeof(jrd_186), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 52, (UCHAR*) &jrd_187);
		}

		if (!DYN_REQUEST(drq_s_classes)) {
			DYN_REQUEST(drq_s_classes) = request;
		}
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_rundown_request(request, drq_s_classes);
			DYN_error_punt(true, 27);
			/* msg 27: "STORE RDB$RELATIONS failed" */
		}
		throw;
	}
}


void DYN_define_sql_field(Global*		gbl,
						  const UCHAR**	ptr,
						  const Firebird::MetaName*		relation_name,
						  Firebird::MetaName*		field_name)
{
   struct {
          TEXT  jrd_120 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_121 [128];	// RDB$EDIT_STRING 
          bid  jrd_122;	// RDB$VALIDATION_SOURCE 
          bid  jrd_123;	// RDB$VALIDATION_BLR 
          bid  jrd_124;	// RDB$DEFAULT_SOURCE 
          bid  jrd_125;	// RDB$DEFAULT_VALUE 
          bid  jrd_126;	// RDB$COMPUTED_SOURCE 
          bid  jrd_127;	// RDB$COMPUTED_BLR 
          SSHORT jrd_128;	// RDB$FIELD_TYPE 
          SSHORT jrd_129;	// RDB$FIELD_LENGTH 
          SSHORT jrd_130;	// gds__null_flag 
          SSHORT jrd_131;	// RDB$COLLATION_ID 
          SSHORT jrd_132;	// gds__null_flag 
          SSHORT jrd_133;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_134;	// gds__null_flag 
          SSHORT jrd_135;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_136;	// gds__null_flag 
          SSHORT jrd_137;	// RDB$DIMENSIONS 
          SSHORT jrd_138;	// gds__null_flag 
          SSHORT jrd_139;	// gds__null_flag 
          SSHORT jrd_140;	// RDB$NULL_FLAG 
          SSHORT jrd_141;	// gds__null_flag 
          SSHORT jrd_142;	// gds__null_flag 
          SSHORT jrd_143;	// gds__null_flag 
          SSHORT jrd_144;	// gds__null_flag 
          SSHORT jrd_145;	// gds__null_flag 
          SSHORT jrd_146;	// gds__null_flag 
          SSHORT jrd_147;	// gds__null_flag 
          SSHORT jrd_148;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_149;	// gds__null_flag 
          SSHORT jrd_150;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_151;	// gds__null_flag 
          SSHORT jrd_152;	// RDB$FIELD_PRECISION 
          SSHORT jrd_153;	// gds__null_flag 
          SSHORT jrd_154;	// RDB$FIELD_SCALE 
          SSHORT jrd_155;	// gds__null_flag 
          SSHORT jrd_156;	// RDB$SYSTEM_FLAG 
   } jrd_119;
   struct {
          TEXT  jrd_159 [32];	// RDB$FIELD_SOURCE 
          bid  jrd_160;	// RDB$DEFAULT_VALUE 
          bid  jrd_161;	// RDB$DEFAULT_SOURCE 
          TEXT  jrd_162 [32];	// RDB$BASE_FIELD 
          TEXT  jrd_163 [128];	// RDB$EDIT_STRING 
          bid  jrd_164;	// RDB$QUERY_HEADER 
          TEXT  jrd_165 [32];	// RDB$QUERY_NAME 
          TEXT  jrd_166 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_167 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_168;	// gds__null_flag 
          SSHORT jrd_169;	// RDB$COLLATION_ID 
          SSHORT jrd_170;	// gds__null_flag 
          SSHORT jrd_171;	// gds__null_flag 
          SSHORT jrd_172;	// gds__null_flag 
          SSHORT jrd_173;	// RDB$NULL_FLAG 
          SSHORT jrd_174;	// gds__null_flag 
          SSHORT jrd_175;	// RDB$UPDATE_FLAG 
          SSHORT jrd_176;	// gds__null_flag 
          SSHORT jrd_177;	// gds__null_flag 
          SSHORT jrd_178;	// RDB$VIEW_CONTEXT 
          SSHORT jrd_179;	// gds__null_flag 
          SSHORT jrd_180;	// RDB$FIELD_POSITION 
          SSHORT jrd_181;	// gds__null_flag 
          SSHORT jrd_182;	// gds__null_flag 
          SSHORT jrd_183;	// gds__null_flag 
          SSHORT jrd_184;	// gds__null_flag 
          SSHORT jrd_185;	// RDB$SYSTEM_FLAG 
   } jrd_158;
/**************************************
 *
 *	D Y N _ d e f i n e _ s q l _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Define a local, SQL field.  This will require generation of
 *	an global field name.
 *
 **************************************/
	UCHAR verb;
	USHORT dtype;
	SLONG fld_pos;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	Firebird::MetaName sql_field_name;
	GET_STRING(ptr, sql_field_name);

	if (sql_field_name.length() == 0) {
		DYN_error_punt(false, 212);
		/* msg 212: "Zero length identifiers not allowed" */
	}

	jrd_req* request = NULL;
	SSHORT id = -1;

	try {

	request = CMP_find_request(tdbb, drq_s_sql_lfld, DYN_REQUESTS);
	id = drq_s_sql_lfld;
	jrd_req* old_request = NULL;

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		RFR IN RDB$RELATION_FIELDS*/
	{
	
		strcpy(/*RFR.RDB$FIELD_NAME*/
		       jrd_158.jrd_167, sql_field_name.c_str());
		if (field_name)
			*field_name = /*RFR.RDB$FIELD_NAME*/
				      jrd_158.jrd_167;
		if (relation_name)
			strcpy(/*RFR.RDB$RELATION_NAME*/
			       jrd_158.jrd_166, relation_name->c_str());
		/*RFR.RDB$SYSTEM_FLAG*/
		jrd_158.jrd_185 = 0;
		/*RFR.RDB$SYSTEM_FLAG.NULL*/
		jrd_158.jrd_184 = FALSE;
		/*RFR.RDB$QUERY_NAME.NULL*/
		jrd_158.jrd_183 = TRUE;
		/*RFR.RDB$QUERY_HEADER.NULL*/
		jrd_158.jrd_182 = TRUE;
		/*RFR.RDB$EDIT_STRING.NULL*/
		jrd_158.jrd_181 = TRUE;
		/*RFR.RDB$FIELD_POSITION.NULL*/
		jrd_158.jrd_179 = TRUE;
		/*RFR.RDB$VIEW_CONTEXT.NULL*/
		jrd_158.jrd_177 = TRUE;
		/*RFR.RDB$BASE_FIELD.NULL*/
		jrd_158.jrd_176 = TRUE;
		/*RFR.RDB$UPDATE_FLAG.NULL*/
		jrd_158.jrd_174 = TRUE;
		/*RFR.RDB$NULL_FLAG.NULL*/
		jrd_158.jrd_172 = TRUE;
		/*RFR.RDB$DEFAULT_SOURCE.NULL*/
		jrd_158.jrd_171 = TRUE;
		/*RFR.RDB$DEFAULT_VALUE.NULL*/
		jrd_158.jrd_170 = TRUE;
		/*RFR.RDB$COLLATION_ID.NULL*/
		jrd_158.jrd_168 = TRUE;

		old_request = request;
		const SSHORT old_id = id;
		request = CMP_find_request(tdbb, drq_s_sql_gfld, DYN_REQUESTS);
		id = drq_s_sql_gfld;

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			FLD IN RDB$FIELDS*/
		{
		
			/*FLD.RDB$SYSTEM_FLAG*/
			jrd_119.jrd_156 = 0;
			/*FLD.RDB$SYSTEM_FLAG.NULL*/
			jrd_119.jrd_155 = FALSE;
			/*FLD.RDB$FIELD_SCALE.NULL*/
			jrd_119.jrd_153 = TRUE;
			/*FLD.RDB$FIELD_PRECISION.NULL*/
			jrd_119.jrd_151 = TRUE;
			/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
			jrd_119.jrd_149 = TRUE;
			/*FLD.RDB$SEGMENT_LENGTH.NULL*/
			jrd_119.jrd_147 = TRUE;
			/*FLD.RDB$COMPUTED_BLR.NULL*/
			jrd_119.jrd_146 = TRUE;
			/*FLD.RDB$COMPUTED_SOURCE.NULL*/
			jrd_119.jrd_145 = TRUE;
			/*FLD.RDB$DEFAULT_VALUE.NULL*/
			jrd_119.jrd_144 = TRUE;
			/*FLD.RDB$DEFAULT_SOURCE.NULL*/
			jrd_119.jrd_143 = TRUE;
			/*FLD.RDB$VALIDATION_BLR.NULL*/
			jrd_119.jrd_142 = TRUE;
			/*FLD.RDB$VALIDATION_SOURCE.NULL*/
			jrd_119.jrd_141 = TRUE;
			/*FLD.RDB$NULL_FLAG.NULL*/
			jrd_119.jrd_139 = TRUE;
			/*FLD.RDB$EDIT_STRING.NULL*/
			jrd_119.jrd_138 = TRUE;
			/*FLD.RDB$DIMENSIONS.NULL*/
			jrd_119.jrd_136 = TRUE;
			/*FLD.RDB$CHARACTER_LENGTH.NULL*/
			jrd_119.jrd_134 = TRUE;
			/*FLD.RDB$CHARACTER_SET_ID.NULL*/
			jrd_119.jrd_132 = TRUE;
			/*FLD.RDB$COLLATION_ID.NULL*/
			jrd_119.jrd_130 = TRUE;

			bool has_dimensions = false;
			bool has_default = false;

			DYN_UTIL_generate_field_name(tdbb, gbl, /*RFR.RDB$FIELD_SOURCE*/
								jrd_158.jrd_159);
			strcpy(/*FLD.RDB$FIELD_NAME*/
			       jrd_119.jrd_120, /*RFR.RDB$FIELD_SOURCE*/
  jrd_158.jrd_159);
			while ((verb = *(*ptr)++) != isc_dyn_end)
				switch (verb)
				{
				case isc_dyn_rel_name:
					GET_STRING(ptr, /*RFR.RDB$RELATION_NAME*/
							jrd_158.jrd_166);
					break;

				case isc_dyn_fld_query_name:
					GET_STRING(ptr, /*RFR.RDB$QUERY_NAME*/
							jrd_158.jrd_165);
					/*RFR.RDB$QUERY_NAME.NULL*/
					jrd_158.jrd_183 = FALSE;
					break;

				case isc_dyn_fld_edit_string:
					GET_STRING(ptr, /*RFR.RDB$EDIT_STRING*/
							jrd_158.jrd_163);
					/*RFR.RDB$EDIT_STRING.NULL*/
					jrd_158.jrd_181 = FALSE;
					break;

				case isc_dyn_fld_position:
					/*RFR.RDB$FIELD_POSITION*/
					jrd_158.jrd_180 = DYN_get_number(ptr);
					/*RFR.RDB$FIELD_POSITION.NULL*/
					jrd_158.jrd_179 = FALSE;
					break;

				case isc_dyn_view_context:
					/*RFR.RDB$VIEW_CONTEXT*/
					jrd_158.jrd_178 = DYN_get_number(ptr);
					/*RFR.RDB$VIEW_CONTEXT.NULL*/
					jrd_158.jrd_177 = FALSE;
					break;

				case isc_dyn_system_flag:
					/*RFR.RDB$SYSTEM_FLAG*/
					jrd_158.jrd_185 = /*FLD.RDB$SYSTEM_FLAG*/
   jrd_119.jrd_156 = DYN_get_number(ptr);
					/*RFR.RDB$SYSTEM_FLAG.NULL*/
					jrd_158.jrd_184 = /*FLD.RDB$SYSTEM_FLAG.NULL*/
   jrd_119.jrd_155 = FALSE;
					break;

				case isc_dyn_update_flag:
					/*RFR.RDB$UPDATE_FLAG*/
					jrd_158.jrd_175 = DYN_get_number(ptr);
					/*RFR.RDB$UPDATE_FLAG.NULL*/
					jrd_158.jrd_174 = FALSE;
					break;

				case isc_dyn_fld_length:
					/*FLD.RDB$FIELD_LENGTH*/
					jrd_119.jrd_129 = DYN_get_number(ptr);
					break;

				case isc_dyn_fld_computed_blr:
					/*FLD.RDB$COMPUTED_BLR.NULL*/
					jrd_119.jrd_146 = FALSE;
					DYN_put_blr_blob(gbl, ptr, &/*FLD.RDB$COMPUTED_BLR*/
								    jrd_119.jrd_127);
					break;

				case isc_dyn_fld_computed_source:
					/*FLD.RDB$COMPUTED_SOURCE.NULL*/
					jrd_119.jrd_145 = FALSE;
					DYN_put_text_blob(gbl, ptr, &/*FLD.RDB$COMPUTED_SOURCE*/
								     jrd_119.jrd_126);
					break;

				case isc_dyn_fld_default_value:
					if (has_dimensions)
					{
						DYN_error_punt(false, 225, sql_field_name.c_str());
						// msg 225: "Default value is not allowed for array type in field %s"
					}
					has_default = true;
					/*RFR.RDB$DEFAULT_VALUE.NULL*/
					jrd_158.jrd_170 = FALSE;
					DYN_put_blr_blob(gbl, ptr, &/*RFR.RDB$DEFAULT_VALUE*/
								    jrd_158.jrd_160);
					break;

				case isc_dyn_fld_default_source:
					if (has_dimensions)
					{
						DYN_error_punt(false, 225, sql_field_name.c_str());
						// msg 225: "Default value is not allowed for array type in field %s"
					}
					has_default = true;
					/*RFR.RDB$DEFAULT_SOURCE.NULL*/
					jrd_158.jrd_171 = FALSE;
					DYN_put_text_blob(gbl, ptr, &/*RFR.RDB$DEFAULT_SOURCE*/
								     jrd_158.jrd_161);
					break;

				case isc_dyn_fld_validation_blr:
					/*FLD.RDB$VALIDATION_BLR.NULL*/
					jrd_119.jrd_142 = FALSE;
					DYN_put_blr_blob(gbl, ptr, &/*FLD.RDB$VALIDATION_BLR*/
								    jrd_119.jrd_123);
					break;

				case isc_dyn_fld_not_null:
					/*RFR.RDB$NULL_FLAG.NULL*/
					jrd_158.jrd_172 = FALSE;
					/*RFR.RDB$NULL_FLAG*/
					jrd_158.jrd_173 = TRUE;
					break;

				case isc_dyn_fld_query_header:
					DYN_put_blr_blob(gbl, ptr, &/*RFR.RDB$QUERY_HEADER*/
								    jrd_158.jrd_164);
					/*RFR.RDB$QUERY_HEADER.NULL*/
					jrd_158.jrd_182 = FALSE;
					break;

				case isc_dyn_fld_type:
					/*FLD.RDB$FIELD_TYPE*/
					jrd_119.jrd_128 = dtype = DYN_get_number(ptr);
					switch (dtype)
					{
					case blr_short:
						/*FLD.RDB$FIELD_LENGTH*/
						jrd_119.jrd_129 = 2;
						break;

					case blr_long:
					case blr_float:
					case blr_sql_date:
					case blr_sql_time:
						/*FLD.RDB$FIELD_LENGTH*/
						jrd_119.jrd_129 = 4;
						break;

					case blr_int64:
					case blr_quad:
					case blr_timestamp:
					case blr_double:
					case blr_d_float:
						/*FLD.RDB$FIELD_LENGTH*/
						jrd_119.jrd_129 = 8;
						break;

					default:
						if (dtype == blr_blob)
							/*FLD.RDB$FIELD_LENGTH*/
							jrd_119.jrd_129 = 8;
						break;
					}
					break;

				case isc_dyn_fld_scale:
					/*FLD.RDB$FIELD_SCALE*/
					jrd_119.jrd_154 = DYN_get_number(ptr);
					/*FLD.RDB$FIELD_SCALE.NULL*/
					jrd_119.jrd_153 = FALSE;
					break;

				case isc_dyn_fld_precision:
					/*FLD.RDB$FIELD_PRECISION*/
					jrd_119.jrd_152 = DYN_get_number(ptr);
					/*FLD.RDB$FIELD_PRECISION.NULL*/
					jrd_119.jrd_151 = FALSE;
					break;

				case isc_dyn_fld_sub_type:
					/*FLD.RDB$FIELD_SUB_TYPE*/
					jrd_119.jrd_150 = DYN_get_number(ptr);
					/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
					jrd_119.jrd_149 = FALSE;
					break;

				case isc_dyn_fld_char_length:
					/*FLD.RDB$CHARACTER_LENGTH*/
					jrd_119.jrd_135 = DYN_get_number(ptr);
					/*FLD.RDB$CHARACTER_LENGTH.NULL*/
					jrd_119.jrd_134 = FALSE;
					break;

				case isc_dyn_fld_character_set:
					/*FLD.RDB$CHARACTER_SET_ID*/
					jrd_119.jrd_133 = DYN_get_number(ptr);
					/*FLD.RDB$CHARACTER_SET_ID.NULL*/
					jrd_119.jrd_132 = FALSE;
					break;

				case isc_dyn_fld_collation:
					/* Note: the global field's collation is not set, just
					 *       the local field.  There is no full "domain"
					 *       created for the local field.
					 *       This is the same decision for items like NULL_FLAG
					 */
					/*RFR.RDB$COLLATION_ID*/
					jrd_158.jrd_169 = DYN_get_number(ptr);
					/*RFR.RDB$COLLATION_ID.NULL*/
					jrd_158.jrd_168 = FALSE;
					break;

				case isc_dyn_fld_dimensions:
					if (has_default)
					{
						DYN_error_punt(false, 225, sql_field_name.c_str());
						// msg 225: "Default value is not allowed for array type in field %s"
					}
					has_dimensions = true;
					/*FLD.RDB$DIMENSIONS*/
					jrd_119.jrd_137 = DYN_get_number(ptr);
					/*FLD.RDB$DIMENSIONS.NULL*/
					jrd_119.jrd_136 = FALSE;
					break;

				case isc_dyn_fld_segment_length:
					/*FLD.RDB$SEGMENT_LENGTH*/
					jrd_119.jrd_148 = DYN_get_number(ptr);
					/*FLD.RDB$SEGMENT_LENGTH.NULL*/
					jrd_119.jrd_147 = FALSE;
					break;

				default:
					--(*ptr);
					MetaTmp(/*RFR.RDB$FIELD_SOURCE*/
						jrd_158.jrd_159)
						DYN_execute(gbl, ptr, relation_name, &tmp, NULL, NULL, NULL);
				}

			if (/*RFR.RDB$FIELD_POSITION.NULL*/
			    jrd_158.jrd_179 == TRUE) {
				fld_pos = -1;
				fb_assert(relation_name);
				DYN_UTIL_generate_field_position(tdbb, gbl, *relation_name, &fld_pos);

				if (fld_pos >= 0) {
					/*RFR.RDB$FIELD_POSITION*/
					jrd_158.jrd_180 = ++fld_pos;
					/*RFR.RDB$FIELD_POSITION.NULL*/
					jrd_158.jrd_179 = FALSE;
				}
			}

		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_118, sizeof(jrd_118), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 266, (UCHAR*) &jrd_119);
		}

		if (!DYN_REQUEST(drq_s_sql_gfld))
			DYN_REQUEST(drq_s_sql_gfld) = request;
		request = old_request;
		id = old_id;
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_157, sizeof(jrd_157), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 348, (UCHAR*) &jrd_158);
	}

	if (!DYN_REQUEST(drq_s_sql_lfld))
		DYN_REQUEST(drq_s_sql_lfld) = request;

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (id == drq_s_sql_lfld) {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 29);
			/* msg 29: "STORE RDB$RELATION_FIELDS failed" */
		}
		else {
			DYN_rundown_request(request, id);
			DYN_error_punt(true, 28);
			/* msg 28: "STORE RDB$FIELDS failed" */
		}
	}
}


void DYN_define_shadow( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_117;	// gds__utility 
   } jrd_116;
   struct {
          SSHORT jrd_115;	// RDB$SHADOW_NUMBER 
   } jrd_114;
/**************************************
 *
 *	D Y N _ d e f i n e _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	Define a shadow.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	bool found = false;

	const SLONG shadow_number = DYN_get_number(ptr);

/* If a shadow set identified by the
   shadow number already exists return error.  */

	jrd_req* request = CMP_find_request(tdbb, drq_l_shadow, DYN_REQUESTS);

	try {

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			FIRST 1 X IN RDB$FILES WITH X.RDB$SHADOW_NUMBER EQ shadow_number*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_113, sizeof(jrd_113), true);
		jrd_114.jrd_115 = shadow_number;
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_114);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_116);
		   if (!jrd_116.jrd_117) break;
				found = true;
		/*END_FOR;*/
		   }
		}

		if (!DYN_REQUEST(drq_l_shadow)) {
			DYN_REQUEST(drq_l_shadow) = request;
		}
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, drq_l_shadow);
		DYN_error_punt(true, 164);
		/* msg 164: "Shadow lookup failed" */
	}

	if (found) {
		DYN_error_punt(false, 165, SafeArg() << shadow_number);
		/* msg 165: "Shadow %ld already exists" */
	}

	SLONG start = 0;
	UCHAR verb;
	while ((verb = *(*ptr)++) != isc_dyn_end)
	{
		switch (verb)
		{
		case isc_dyn_def_file:
			DYN_define_file(gbl, ptr, shadow_number, &start, 157);
			break;

		default:
			DYN_unsupported_verb();
		}
	}
}


void DYN_define_trigger(Global*						gbl,
						const UCHAR**				ptr,
						const Firebird::MetaName*	relation_name,
						Firebird::MetaName*			trigger_name,
						const bool					ignore_perm)
{
   struct {
          SSHORT jrd_91;	// gds__utility 
   } jrd_90;
   struct {
          bid  jrd_86;	// RDB$DEBUG_INFO 
          SSHORT jrd_87;	// gds__null_flag 
          SSHORT jrd_88;	// RDB$VALID_BLR 
          SSHORT jrd_89;	// gds__null_flag 
   } jrd_85;
   struct {
          bid  jrd_80;	// RDB$DEBUG_INFO 
          SSHORT jrd_81;	// gds__utility 
          SSHORT jrd_82;	// gds__null_flag 
          SSHORT jrd_83;	// gds__null_flag 
          SSHORT jrd_84;	// RDB$VALID_BLR 
   } jrd_79;
   struct {
          TEXT  jrd_78 [32];	// RDB$TRIGGER_NAME 
   } jrd_77;
   struct {
          TEXT  jrd_94 [32];	// RDB$TRIGGER_NAME 
          bid  jrd_95;	// RDB$DESCRIPTION 
          bid  jrd_96;	// RDB$TRIGGER_SOURCE 
          bid  jrd_97;	// RDB$TRIGGER_BLR 
          TEXT  jrd_98 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_99;	// gds__null_flag 
          SSHORT jrd_100;	// gds__null_flag 
          SSHORT jrd_101;	// gds__null_flag 
          SSHORT jrd_102;	// gds__null_flag 
          SSHORT jrd_103;	// gds__null_flag 
          SSHORT jrd_104;	// RDB$FLAGS 
          SSHORT jrd_105;	// gds__null_flag 
          SSHORT jrd_106;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_107;	// gds__null_flag 
          SSHORT jrd_108;	// RDB$TRIGGER_INACTIVE 
          SSHORT jrd_109;	// gds__null_flag 
          SSHORT jrd_110;	// RDB$TRIGGER_SEQUENCE 
          SSHORT jrd_111;	// gds__null_flag 
          SSHORT jrd_112;	// RDB$TRIGGER_TYPE 
   } jrd_93;
/**************************************
 *
 *	D Y N _ d e f i n e _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Define a trigger for a relation.
 *
 *
 * if the ignore_perm flag is true, then this trigger must be defined
 * now (and fired at run time) without making SQL permissions checks.
 * In particular, one should not need control permissions on the table
 * to define this trigger. Currently used to define triggers for
 * cascading referential interity.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	Firebird::MetaName t;

	GET_STRING(ptr, t);
	if (t.length() == 0)
	{
		DYN_UTIL_generate_trigger_name(tdbb, gbl, t);
	}

	if (t.length() == 0)
	{
		DYN_error_punt(false, 212);
		/* msg 212: "Zero length identifiers not allowed" */
	}

	if (trigger_name) {
		*trigger_name = t;
	}

	jrd_req* request = CMP_find_request(tdbb, drq_s_triggers, DYN_REQUESTS);

	bool b_ending_store = false;
	const UCHAR* debug_info_ptr = NULL;

	try {

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$TRIGGERS*/
	{
	
		/*X.RDB$TRIGGER_TYPE.NULL*/
		jrd_93.jrd_111 = TRUE;
		/*X.RDB$TRIGGER_SEQUENCE*/
		jrd_93.jrd_110 = 0;
		/*X.RDB$TRIGGER_SEQUENCE.NULL*/
		jrd_93.jrd_109 = FALSE;
		/*X.RDB$TRIGGER_INACTIVE*/
		jrd_93.jrd_108 = 0;
		/*X.RDB$TRIGGER_INACTIVE.NULL*/
		jrd_93.jrd_107 = FALSE;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_93.jrd_106 = 0;
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_93.jrd_105 = FALSE;

		/* currently, we make no difference between ignoring permissions in
		   order to define this trigger and ignoring permissions checks when the
		   trigger fires. The RDB$FLAGS is used to indicate permissions checks
		   when the trigger fires. Later, if we need to make a difference
		   between these, then the caller should pass the required value
		   of RDB$FLAGS as an extra argument to this func.  */

		/*X.RDB$FLAGS*/
		jrd_93.jrd_104 = ignore_perm ? TRG_ignore_perm : 0;
		/*X.RDB$FLAGS.NULL*/
		jrd_93.jrd_103 = FALSE;
		if (relation_name) {
			strcpy(/*X.RDB$RELATION_NAME*/
			       jrd_93.jrd_98, relation_name->c_str());
			/*X.RDB$RELATION_NAME.NULL*/
			jrd_93.jrd_102 = FALSE;
		}
		else
			/*X.RDB$RELATION_NAME.NULL*/
			jrd_93.jrd_102 = TRUE;
		/*X.RDB$TRIGGER_BLR.NULL*/
		jrd_93.jrd_101 = TRUE;
		/*X.RDB$TRIGGER_SOURCE.NULL*/
		jrd_93.jrd_100 = TRUE;
		/*X.RDB$DESCRIPTION.NULL*/
		jrd_93.jrd_99 = TRUE;
		strcpy(/*X.RDB$TRIGGER_NAME*/
		       jrd_93.jrd_94, t.c_str());

		UCHAR verb;
		while ((verb = *(*ptr)++) != isc_dyn_end)
		{
			switch (verb)
			{
			case isc_dyn_trg_type:
				/*X.RDB$TRIGGER_TYPE*/
				jrd_93.jrd_112 = DYN_get_number(ptr);
				/*X.RDB$TRIGGER_TYPE.NULL*/
				jrd_93.jrd_111 = FALSE;
				break;

			case isc_dyn_sql_object:
				/*X.RDB$FLAGS*/
				jrd_93.jrd_104 |= TRG_sql;
				/*X.RDB$FLAGS.NULL*/
				jrd_93.jrd_103 = FALSE;
				break;

			case isc_dyn_trg_sequence:
				/*X.RDB$TRIGGER_SEQUENCE*/
				jrd_93.jrd_110 = DYN_get_number(ptr);
				/*X.RDB$TRIGGER_SEQUENCE.NULL*/
				jrd_93.jrd_109 = FALSE;
				break;

			case isc_dyn_trg_inactive:
				/*X.RDB$TRIGGER_INACTIVE*/
				jrd_93.jrd_108 = DYN_get_number(ptr);
				/*X.RDB$TRIGGER_INACTIVE.NULL*/
				jrd_93.jrd_107 = FALSE;
				break;

			case isc_dyn_rel_name:
				GET_STRING(ptr, /*X.RDB$RELATION_NAME*/
						jrd_93.jrd_98);
				/*X.RDB$RELATION_NAME.NULL*/
				jrd_93.jrd_102 = FALSE;
				break;

			case isc_dyn_trg_blr:
				{
					const UCHAR* blr = *ptr;
					DYN_skip_attribute(ptr);
					DYN_put_blr_blob(gbl, &blr, &/*X.RDB$TRIGGER_BLR*/
								     jrd_93.jrd_97);
					/*X.RDB$TRIGGER_BLR.NULL*/
					jrd_93.jrd_101 = FALSE;
					break;
				}

			case isc_dyn_trg_source:
				{
					const UCHAR* source = *ptr;
					DYN_skip_attribute(ptr);
					DYN_put_text_blob(gbl, &source, &/*X.RDB$TRIGGER_SOURCE*/
									 jrd_93.jrd_96);
					/*X.RDB$TRIGGER_SOURCE.NULL*/
					jrd_93.jrd_100 = FALSE;
					break;
				}

			case isc_dyn_description:
				DYN_put_text_blob(gbl, ptr, &/*X.RDB$DESCRIPTION*/
							     jrd_93.jrd_95);
				/*X.RDB$DESCRIPTION.NULL*/
				jrd_93.jrd_99 = FALSE;
				break;

			case isc_dyn_system_flag:
				/*X.RDB$SYSTEM_FLAG*/
				jrd_93.jrd_106 = DYN_get_number(ptr);
				/*X.RDB$SYSTEM_FLAG.NULL*/
				jrd_93.jrd_105 = FALSE;
				/* fb_assert(!ignore_perm || ignore_perm &&
					X.RDB$SYSTEM_FLAG == fb_sysflag_referential_constraint); */
				break;

			case isc_dyn_debug_info:
				debug_info_ptr = *ptr;
				DYN_skip_attribute(ptr);
				break;

			default:
				--(*ptr);
				MetaTmp(/*X.RDB$RELATION_NAME*/
					jrd_93.jrd_98)
					DYN_execute(gbl, ptr, &tmp, NULL, &t, NULL, NULL);
			}
		}

		if (/*X.RDB$RELATION_NAME.NULL*/
		    jrd_93.jrd_102 && !tdbb->getAttachment()->locksmith())
			ERR_post(Arg::Gds(isc_adm_task_denied));

		b_ending_store = true;

/* the END_STORE_SPECIAL adds the foll. lines of code to the END_STORE
   if (ignore_perm)
       request->req_flags |= req_ignore_perm;
   after the request is compiled and before the request is sent.
   It makes the current request (to define the trigger) go through
   without checking any permissions lower in the engine */

	/*END_STORE_SPECIAL;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_92, sizeof(jrd_92), true);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	if (ignore_perm)
		request->req_flags |= req_ignore_perm;
	EXE_send (tdbb, request, 0, 116, (UCHAR*) &jrd_93);
	}

	if (ignore_perm)
		request->req_flags &= ~req_ignore_perm;

	if (!DYN_REQUEST(drq_s_triggers)) {
		DYN_REQUEST(drq_s_triggers) = request;
	}

	if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
	{
		jrd_req* sub_request = NULL;

		/*FOR(REQUEST_HANDLE sub_request TRANSACTION_HANDLE gbl->gbl_transaction)
			TRG IN RDB$TRIGGERS WITH TRG.RDB$TRIGGER_NAME EQ t.c_str()*/
		{
		if (!sub_request)
		   sub_request = CMP_compile2 (tdbb, (UCHAR*) jrd_76, sizeof(jrd_76), true);
		gds__vtov ((const char*) t.c_str(), (char*) jrd_77.jrd_78, 32);
		EXE_start (tdbb, sub_request, gbl->gbl_transaction);
		EXE_send (tdbb, sub_request, 0, 32, (UCHAR*) &jrd_77);
		while (1)
		   {
		   EXE_receive (tdbb, sub_request, 1, 16, (UCHAR*) &jrd_79);
		   if (!jrd_79.jrd_81) break;

			/*MODIFY TRG USING*/
			{
			
				/*TRG.RDB$VALID_BLR*/
				jrd_79.jrd_84 = TRUE;
				/*TRG.RDB$VALID_BLR.NULL*/
				jrd_79.jrd_83 = FALSE;

				/*TRG.RDB$DEBUG_INFO.NULL*/
				jrd_79.jrd_82 = (debug_info_ptr == NULL) ? TRUE : FALSE;
				if (debug_info_ptr)
					DYN_put_blr_blob(gbl, &debug_info_ptr, &/*TRG.RDB$DEBUG_INFO*/
										jrd_79.jrd_80);
			/*END_MODIFY;*/
			jrd_85.jrd_86 = jrd_79.jrd_80;
			jrd_85.jrd_87 = jrd_79.jrd_83;
			jrd_85.jrd_88 = jrd_79.jrd_84;
			jrd_85.jrd_89 = jrd_79.jrd_82;
			EXE_send (tdbb, sub_request, 2, 14, (UCHAR*) &jrd_85);
			}
		/*END_FOR;*/
		   EXE_send (tdbb, sub_request, 3, 2, (UCHAR*) &jrd_90);
		   }
		}

		CMP_release(tdbb, sub_request);
	}

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_rundown_request(request, drq_s_triggers);
			DYN_error_punt(true, 31);
			/* msg 31: "DEFINE TRIGGER failed" */
		}
		throw;
	}
}


void DYN_define_trigger_msg(Global* gbl, const UCHAR** ptr, const Firebird::MetaName* trigger_name)
{
   struct {
          TEXT  jrd_71 [32];	// RDB$TRIGGER_NAME 
          TEXT  jrd_72 [1024];	// RDB$MESSAGE 
          SSHORT jrd_73;	// gds__null_flag 
          SSHORT jrd_74;	// gds__null_flag 
          SSHORT jrd_75;	// RDB$MESSAGE_NUMBER 
   } jrd_70;
/**************************************
 *
 *	D Y N _ d e f i n e _ t r i g g e r _ m s g
 *
 **************************************
 *
 * Functional description
 *	Define a trigger message.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_s_trg_msgs, DYN_REQUESTS);

	bool b_ending_store = false;

	try {

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			X IN RDB$TRIGGER_MESSAGES*/
		{
		
			/*X.RDB$MESSAGE_NUMBER*/
			jrd_70.jrd_75 = DYN_get_number(ptr);
			/*X.RDB$MESSAGE.NULL*/
			jrd_70.jrd_74 = TRUE;
			if (trigger_name) {
				strcpy(/*X.RDB$TRIGGER_NAME*/
				       jrd_70.jrd_71, trigger_name->c_str());
				/*X.RDB$TRIGGER_NAME.NULL*/
				jrd_70.jrd_73 = FALSE;
			}
			else {
				/*X.RDB$TRIGGER_NAME.NULL*/
				jrd_70.jrd_73 = TRUE;
			}

			UCHAR verb;
			while ((verb = *(*ptr)++) != isc_dyn_end)
			{
				switch (verb)
				{
				case isc_dyn_trg_name:
					GET_STRING(ptr, /*X.RDB$TRIGGER_NAME*/
							jrd_70.jrd_71);
					/*X.RDB$TRIGGER_NAME.NULL*/
					jrd_70.jrd_73 = FALSE;
					break;

				case isc_dyn_trg_msg:
					GET_STRING(ptr, /*X.RDB$MESSAGE*/
							jrd_70.jrd_72);
					/*X.RDB$MESSAGE.NULL*/
					jrd_70.jrd_74 = FALSE;
					break;

				default:
					DYN_unsupported_verb();
				}
			}

			b_ending_store = true;

		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_69, sizeof(jrd_69), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 1062, (UCHAR*) &jrd_70);
		}

		if (!DYN_REQUEST(drq_s_trg_msgs)) {
			DYN_REQUEST(drq_s_trg_msgs) = request;
		}
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_rundown_request(request, drq_s_trg_msgs);
			DYN_error_punt(true, 33);
			/* msg 33: "DEFINE TRIGGER MESSAGE failed" */
		}
		throw;
	}
}


void DYN_define_view_relation( Global* gbl, const UCHAR** ptr, const Firebird::MetaName* view)
{
   struct {
          TEXT  jrd_63 [256];	// RDB$CONTEXT_NAME 
          TEXT  jrd_64 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_65 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_66;	// gds__null_flag 
          SSHORT jrd_67;	// RDB$VIEW_CONTEXT 
          SSHORT jrd_68;	// gds__null_flag 
   } jrd_62;
/**************************************
 *
 *	D Y N _ d e f i n e _ v i e w _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Store a RDB$VIEW_RELATION record.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	fb_assert(view);
	if (view->length() == 0) {
		DYN_error_punt(false, 212);
	/* msg 212: "Zero length identifiers not allowed" */
	}

	jrd_req* request = CMP_find_request(tdbb, drq_s_view_rels, DYN_REQUESTS);
	SSHORT id = drq_s_view_rels;

	bool b_ending_store = false;

	try {
/*
 * The below code has been added for ALTER VIEW support,
 * but implementation was definitely wrong,
 * so it's commented our till the better times
 *
		const SSHORT old_id = id;
		jrd_req* old_request = request;

		request = CMP_find_request(tdbb, drq_e_view_rels, DYN_REQUESTS);
		id = drq_e_view_rels;

		FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			VRL IN RDB$VIEW_RELATIONS WITH VRL.RDB$VIEW_NAME EQ view->c_str()

			if (!DYN_REQUEST(drq_e_view_rels))
				DYN_REQUEST(drq_e_view_rels) = request;

			ERASE VRL;
		END_FOR;

		if (!DYN_REQUEST(drq_e_view_rels))
			DYN_REQUEST(drq_e_view_rels) = request;

		request = old_request;
		id = old_id;
*/
		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			VRL IN RDB$VIEW_RELATIONS*/
		{
		
			strcpy(/*VRL.RDB$VIEW_NAME*/
			       jrd_62.jrd_65, view->c_str());
			GET_STRING(ptr, /*VRL.RDB$RELATION_NAME*/
					jrd_62.jrd_64);
			/*VRL.RDB$CONTEXT_NAME.NULL*/
			jrd_62.jrd_68 = TRUE;
			/*VRL.RDB$VIEW_CONTEXT.NULL*/
			jrd_62.jrd_66 = TRUE;

			UCHAR verb;
			while ((verb = *(*ptr)++) != isc_dyn_end)
			{
				switch (verb)
				{
				case isc_dyn_view_context:
					/*VRL.RDB$VIEW_CONTEXT*/
					jrd_62.jrd_67 = DYN_get_number(ptr);
					/*VRL.RDB$VIEW_CONTEXT.NULL*/
					jrd_62.jrd_66 = FALSE;
					break;

				case isc_dyn_view_context_name:
					GET_STRING(ptr, /*VRL.RDB$CONTEXT_NAME*/
							jrd_62.jrd_63);
					/*VRL.RDB$CONTEXT_NAME.NULL*/
					jrd_62.jrd_68 = FALSE;

					if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) < ODS_11_2) {
						/*VRL.RDB$CONTEXT_NAME*/
						jrd_62.jrd_63[31] = 0;
					}
					break;

				default:
					--(*ptr);
					MetaTmp(/*VRL.RDB$RELATION_NAME*/
						jrd_62.jrd_64)
						DYN_execute(gbl, ptr, &tmp, NULL, NULL, NULL, NULL);
				}
			}

			b_ending_store = true;
		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_61, sizeof(jrd_61), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 326, (UCHAR*) &jrd_62);
		}

		if (!DYN_REQUEST(drq_s_view_rels)) {
			DYN_REQUEST(drq_s_view_rels) = request;
		}
	}
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (b_ending_store) {
			DYN_rundown_request(request, id);
			if (id == drq_s_view_rels)
			{
				DYN_error_punt(true, 34);
				/* msg 34: "STORE RDB$VIEW_RELATIONS failed" */
			}
			else if (id == drq_e_view_rels)
			{
				DYN_error_punt(true, 59);
				/* msg 59: "ERASE RDB$VIEW_RELATIONS failed" */
			}
		}
		throw;
	}
}


static void check_unique_name(thread_db* tdbb,
							  Global* gbl,
							  const Firebird::MetaName& object_name,
							  int object_type)
{
   struct {
          SSHORT jrd_40;	// gds__utility 
   } jrd_39;
   struct {
          TEXT  jrd_38 [32];	// RDB$GENERATOR_NAME 
   } jrd_37;
   struct {
          SSHORT jrd_45;	// gds__utility 
   } jrd_44;
   struct {
          TEXT  jrd_43 [32];	// RDB$EXCEPTION_NAME 
   } jrd_42;
   struct {
          SSHORT jrd_50;	// gds__utility 
   } jrd_49;
   struct {
          TEXT  jrd_48 [32];	// RDB$INDEX_NAME 
   } jrd_47;
   struct {
          SSHORT jrd_55;	// gds__utility 
   } jrd_54;
   struct {
          TEXT  jrd_53 [32];	// RDB$PROCEDURE_NAME 
   } jrd_52;
   struct {
          SSHORT jrd_60;	// gds__utility 
   } jrd_59;
   struct {
          TEXT  jrd_58 [32];	// RDB$RELATION_NAME 
   } jrd_57;
/**************************************
 *
 *	c h e c k _ u n i q u e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Check if an object already exists.
 *	If yes then return error.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	USHORT error_code = 0;
	jrd_req* request = NULL;

	try
	{
		switch (object_type)
		{
		case obj_relation:
		case obj_procedure:
			request = CMP_find_request(tdbb, drq_l_rel_name, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				EREL IN RDB$RELATIONS WITH EREL.RDB$RELATION_NAME EQ object_name.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_56, sizeof(jrd_56), true);
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_57.jrd_58, 32);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_57);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_59);
			   if (!jrd_59.jrd_60) break;

				if (!DYN_REQUEST(drq_l_rel_name))
					DYN_REQUEST(drq_l_rel_name) = request;

				error_code = 132;
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_l_rel_name))
				DYN_REQUEST(drq_l_rel_name) = request;

			if (!error_code)
			{
				request = CMP_find_request(tdbb, drq_l_prc_name, DYN_REQUESTS);

				/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
					EPRC IN RDB$PROCEDURES WITH EPRC.RDB$PROCEDURE_NAME EQ object_name.c_str()*/
				{
				if (!request)
				   request = CMP_compile2 (tdbb, (UCHAR*) jrd_51, sizeof(jrd_51), true);
				gds__vtov ((const char*) object_name.c_str(), (char*) jrd_52.jrd_53, 32);
				EXE_start (tdbb, request, gbl->gbl_transaction);
				EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_52);
				while (1)
				   {
				   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_54);
				   if (!jrd_54.jrd_55) break;

					if (!DYN_REQUEST(drq_l_prc_name))
						DYN_REQUEST(drq_l_prc_name) = request;

					error_code = 135;
				/*END_FOR;*/
				   }
				}

				if (!DYN_REQUEST(drq_l_prc_name))
					DYN_REQUEST(drq_l_prc_name) = request;
			}
			break;

		case obj_index:
			request = CMP_find_request(tdbb, drq_l_idx_name, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				EIDX IN RDB$INDICES WITH EIDX.RDB$INDEX_NAME EQ object_name.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_46, sizeof(jrd_46), true);
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_47.jrd_48, 32);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_47);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_49);
			   if (!jrd_49.jrd_50) break;

				if (!DYN_REQUEST(drq_l_idx_name))
					DYN_REQUEST(drq_l_idx_name) = request;

				error_code = 251;
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_l_idx_name))
				DYN_REQUEST(drq_l_idx_name) = request;
			break;

		case obj_exception:
			request = CMP_find_request(tdbb, drq_l_xcp_name, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				EXCP IN RDB$EXCEPTIONS WITH EXCP.RDB$EXCEPTION_NAME EQ object_name.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_41, sizeof(jrd_41), true);
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_42.jrd_43, 32);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_42);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_44);
			   if (!jrd_44.jrd_45) break;

				if (!DYN_REQUEST(drq_l_xcp_name))
					DYN_REQUEST(drq_l_xcp_name) = request;

				error_code = 253;
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_l_xcp_name))
				DYN_REQUEST(drq_l_xcp_name) = request;
			break;

		case obj_generator:
			request = CMP_find_request(tdbb, drq_l_gen_name, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				EGEN IN RDB$GENERATORS WITH EGEN.RDB$GENERATOR_NAME EQ object_name.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_36, sizeof(jrd_36), true);
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_37.jrd_38, 32);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_37);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_39);
			   if (!jrd_39.jrd_40) break;

				if (!DYN_REQUEST(drq_l_gen_name))
					DYN_REQUEST(drq_l_gen_name) = request;

				error_code = 254;
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_l_gen_name))
				DYN_REQUEST(drq_l_gen_name) = request;
			break;

		default:
			fb_assert(false);
		}
	}
	catch (const Firebird::Exception&)
	{
		DYN_rundown_request(request, -1);
		throw;
	}

	if (error_code) {
		DYN_error_punt(false, error_code, object_name.c_str());
	}
}


static bool get_who( thread_db* tdbb, Global* gbl, Firebird::MetaName& output_name)
{
/**************************************
 *
 *	g e t _ w h o
 *
 **************************************
 *
 * Functional description
 *	Get user name
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_req* request = CMP_find_request(tdbb, drq_l_user_name, DYN_REQUESTS);

	try {
		if (!request)
		{
			request = CMP_compile2(tdbb, who_blr, sizeof(who_blr), true);
		}
		EXE_start(tdbb, request, gbl->gbl_transaction);
		SqlIdentifier x;
		EXE_receive(tdbb, request, 0, sizeof(x), (UCHAR*) x);
		output_name = x;

		DYN_rundown_request(request, drq_l_user_name);
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, drq_l_user_name);
		return false;
	}

	return true;
}


bool is_it_user_name(Global* gbl, const Firebird::MetaName& role_name, thread_db* tdbb)
{
   struct {
          SSHORT jrd_27;	// gds__utility 
   } jrd_26;
   struct {
          TEXT  jrd_25 [32];	// RDB$OWNER_NAME 
   } jrd_24;
   struct {
          SSHORT jrd_35;	// gds__utility 
   } jrd_34;
   struct {
          TEXT  jrd_30 [32];	// RDB$GRANTOR 
          TEXT  jrd_31 [32];	// RDB$USER 
          SSHORT jrd_32;	// RDB$OBJECT_TYPE 
          SSHORT jrd_33;	// RDB$USER_TYPE 
   } jrd_29;
/**************************************
 *
 *	i s _ i t _ u s e r _ n a m e
 *
 **************************************
 *
 * Functional description
 *
 *     if role_name is user name returns true. Otherwise returns false.
 *
 **************************************/

	jrd_req* request;
	USHORT request_id;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	bool found = false;

	try {

/* If there is a user with privilege or a grantor on a relation we
   can infer there is a user with this name */

		request_id = drq_get_user_priv;
		request = CMP_find_request(tdbb, request_id, DYN_REQUESTS);
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			PRIV IN RDB$USER_PRIVILEGES WITH
				(PRIV.RDB$USER EQ role_name.c_str() AND
				 PRIV.RDB$USER_TYPE = obj_user) OR
				(PRIV.RDB$GRANTOR EQ role_name.c_str() AND
				 PRIV.RDB$OBJECT_TYPE = obj_relation)*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_28, sizeof(jrd_28), true);
		gds__vtov ((const char*) role_name.c_str(), (char*) jrd_29.jrd_30, 32);
		gds__vtov ((const char*) role_name.c_str(), (char*) jrd_29.jrd_31, 32);
		jrd_29.jrd_32 = obj_relation;
		jrd_29.jrd_33 = obj_user;
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_29);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_34);
		   if (!jrd_34.jrd_35) break;

			found = true;

		/*END_FOR;*/
		   }
		}

		if (!DYN_REQUEST(drq_get_user_priv))
			DYN_REQUEST(drq_get_user_priv) = request;

		if (found) {
			return found;
		}

/* We can infer that 'role_name' is a user name if it owns any relations
   Note we can only get here if a user creates a table and revokes all
   his privileges on the table */

		request_id = drq_get_rel_owner;
		request = CMP_find_request(tdbb, request_id, DYN_REQUESTS);
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			REL IN RDB$RELATIONS WITH
				REL.RDB$OWNER_NAME EQ role_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_23, sizeof(jrd_23), true);
		gds__vtov ((const char*) role_name.c_str(), (char*) jrd_24.jrd_25, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_24);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_26);
		   if (!jrd_26.jrd_27) break;

			found = true;
		/*END_FOR;*/
		   }
		}

		if (!DYN_REQUEST(drq_get_rel_owner)) {
			DYN_REQUEST(drq_get_rel_owner) = request;
		}
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (request) {
			DYN_rundown_request(request, request_id);
		}
		ERR_punt();
	}

	return found;
}


static rel_t get_relation_type(thread_db* tdbb, Global* gbl,
							   const Firebird::MetaName& rel_name)
{
   struct {
          SSHORT jrd_21;	// gds__utility 
          SSHORT jrd_22;	// RDB$RELATION_TYPE 
   } jrd_20;
   struct {
          TEXT  jrd_19 [32];	// RDB$RELATION_NAME 
   } jrd_18;
	Database* dbb = tdbb->getDatabase();

	const USHORT major_version  = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	rel_t rel_type = rel_persistent;

	if (ENCODE_ODS(major_version, minor_original) >= ODS_11_1)
	{
		jrd_req* request = CMP_find_request(tdbb, drq_l_rel_type, DYN_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			REL IN RDB$RELATIONS
			WITH REL.RDB$RELATION_NAME EQ rel_name.c_str()
			  AND REL.RDB$RELATION_TYPE NOT MISSING*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_17, sizeof(jrd_17), true);
		gds__vtov ((const char*) rel_name.c_str(), (char*) jrd_18.jrd_19, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_18);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_20);
		   if (!jrd_20.jrd_21) break;

			if (!DYN_REQUEST(drq_l_rel_type))
				DYN_REQUEST(drq_l_rel_type) = request;

			rel_type = (rel_t) /*REL.RDB$RELATION_TYPE*/
					   jrd_20.jrd_22;

		/*END_FOR;*/
		   }
		}

		if (!DYN_REQUEST(drq_l_rel_type))
			DYN_REQUEST(drq_l_rel_type) = request;
	}

	return rel_type;
}


static void check_foreign_key_temp_scope(thread_db* tdbb, Global* gbl,
				const TEXT*	child_rel_name, const TEXT* master_index_name)
{
   struct {
          TEXT  jrd_14 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_15 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_16;	// gds__utility 
   } jrd_13;
   struct {
          TEXT  jrd_9 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_10 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_11 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_12 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_8;
/**********************************************************
 *
 *	c h e c k _ f o r e i g n _ k e y _ t e m p _ s c o p e
 *
 **********************************************************
 *
 * Functional description
 *	Check temporary table reference rules between given child
 *	relation and master relation (owner of given PK\UK index)
 *
 **********************************************************/
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_l_rel_info, DYN_REQUESTS);
	bool bErr = false;
	Firebird::string sMaster, sChild;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		RLC_M IN RDB$RELATION_CONSTRAINTS CROSS
		REL_C IN RDB$RELATIONS CROSS
		REL_M IN RDB$RELATIONS
		WITH (RLC_M.RDB$CONSTRAINT_TYPE EQ UNIQUE_CNSTRT OR
			  RLC_M.RDB$CONSTRAINT_TYPE EQ PRIMARY_KEY)
		 AND RLC_M.RDB$INDEX_NAME EQ master_index_name
		 AND REL_C.RDB$RELATION_NAME EQ child_rel_name
		 AND REL_M.RDB$RELATION_NAME EQ RLC_M.RDB$RELATION_NAME*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_7, sizeof(jrd_7), true);
	gds__vtov ((const char*) child_rel_name, (char*) jrd_8.jrd_9, 32);
	gds__vtov ((const char*) master_index_name, (char*) jrd_8.jrd_10, 32);
	gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_8.jrd_11, 12);
	gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_8.jrd_12, 12);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 88, (UCHAR*) &jrd_8);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_13);
	   if (!jrd_13.jrd_16) break;

		if (!DYN_REQUEST(drq_l_rel_info))
			DYN_REQUEST(drq_l_rel_info) = request;

		const rel_t master_type = get_relation_type(tdbb, gbl, /*REL_M.RDB$RELATION_NAME*/
								       jrd_13.jrd_15);
		fb_assert(master_type == rel_persistent ||
				  master_type == rel_global_temp_preserve ||
				  master_type == rel_global_temp_delete);

		const rel_t child_type = get_relation_type(tdbb, gbl, /*REL_C.RDB$RELATION_NAME*/
								      jrd_13.jrd_14);
		fb_assert(child_type == rel_persistent ||
				  child_type == rel_global_temp_preserve ||
				  child_type == rel_global_temp_delete);

		bErr = (master_type != child_type) &&
				!( (master_type == rel_global_temp_preserve) &&
				(child_type == rel_global_temp_delete) );

		if (bErr)
		{
			fb_utils::exact_name_limit(/*REL_M.RDB$RELATION_NAME*/
						   jrd_13.jrd_15, sizeof(/*REL_M.RDB$RELATION_NAME*/
	 jrd_13.jrd_15));
			fb_utils::exact_name_limit(/*REL_C.RDB$RELATION_NAME*/
						   jrd_13.jrd_14, sizeof(/*REL_C.RDB$RELATION_NAME*/
	 jrd_13.jrd_14));

			make_relation_scope_name(/*REL_M.RDB$RELATION_NAME*/
						 jrd_13.jrd_15, master_type, sMaster);
			make_relation_scope_name(/*REL_C.RDB$RELATION_NAME*/
						 jrd_13.jrd_14, child_type, sChild);
		}
	/*END_FOR;*/
	   }
	}

	if (!DYN_REQUEST(drq_l_rel_info))
		DYN_REQUEST(drq_l_rel_info) = request;

	if (bErr) {
		DYN_error_punt(false, 232, // Msg 232 : "%s can't reference %s"
			SafeArg() << sChild.c_str() << sMaster.c_str());
	}
}


static void check_relation_temp_scope(thread_db* tdbb, Global* gbl,
				const TEXT*	child_rel_name, const rel_t child_type)
{
   struct {
          TEXT  jrd_5 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_6;	// gds__utility 
   } jrd_4;
   struct {
          TEXT  jrd_2 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_3 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_1;
/****************************************************
 *
 *	c h e c k _ r e l a t i o n _ t e m p _ s c o p e
 *
 ****************************************************
 *
 * Functional description
 *	Check temporary table reference rules between just
 *	created child relation and all its master relations
 *
 ****************************************************/
	Database* dbb = tdbb->getDatabase();

	if (child_type != rel_persistent &&
		child_type != rel_global_temp_preserve &&
		child_type != rel_global_temp_delete)
	{
		return;
	}

	jrd_req* request = CMP_find_request(tdbb, drq_l_rel_info2, DYN_REQUESTS);
	bool bErr = false;
	Firebird::string sMaster, sChild;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		RLC_C IN RDB$RELATION_CONSTRAINTS CROSS
		IND_C IN RDB$INDICES CROSS
		IND_M IN RDB$INDICES CROSS
		REL_M IN RDB$RELATIONS
		WITH RLC_C.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY
		 AND RLC_C.RDB$RELATION_NAME EQ child_rel_name
		 AND IND_C.RDB$INDEX_NAME EQ RLC_C.RDB$INDEX_NAME
		 AND IND_M.RDB$INDEX_NAME EQ IND_C.RDB$FOREIGN_KEY
		 AND IND_M.RDB$RELATION_NAME EQ REL_M.RDB$RELATION_NAME*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
	gds__vtov ((const char*) child_rel_name, (char*) jrd_1.jrd_2, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_1.jrd_3, 12);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 44, (UCHAR*) &jrd_1);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_4);
	   if (!jrd_4.jrd_6) break;

		if (!DYN_REQUEST(drq_l_rel_info2))
			DYN_REQUEST(drq_l_rel_info2) = request;

		const rel_t master_type = get_relation_type(tdbb, gbl, /*REL_M.RDB$RELATION_NAME*/
								       jrd_4.jrd_5);
		fb_assert(master_type == rel_persistent ||
				  master_type == rel_global_temp_preserve ||
				  master_type == rel_global_temp_delete);

		bErr = (master_type != child_type) &&
				!( (master_type == rel_global_temp_preserve) &&
					(child_type == rel_global_temp_delete) );

		if (bErr)
		{
			fb_utils::exact_name_limit(/*REL_M.RDB$RELATION_NAME*/
						   jrd_4.jrd_5, sizeof(/*REL_M.RDB$RELATION_NAME*/
	 jrd_4.jrd_5));

			make_relation_scope_name(/*REL_M.RDB$RELATION_NAME*/
						 jrd_4.jrd_5, master_type, sMaster);
			make_relation_scope_name(child_rel_name, child_type, sChild);
		}
	/*END_FOR;*/
	   }
	}

	if (!DYN_REQUEST(drq_l_rel_info2))
		DYN_REQUEST(drq_l_rel_info2) = request;

	if (bErr) {
		DYN_error_punt(false, 232, // Msg 232 : "%s can't reference %s"
			SafeArg() << sChild.c_str() << sMaster.c_str());
	}
}


static void make_relation_scope_name(const TEXT* rel_name,
	const rel_t rel_type, Firebird::string& str)
{
/**************************************************
 *
 *	m a k e _ r e l a t i o n _ s c o p e _ n a m e
 *
 **************************************************
 *
 * Functional description
 *	Make string with relation name and type
 *	of its temporary scope
 *
 **************************************************/
	const char *scope = NULL;
	if (rel_type == rel_global_temp_preserve)
		scope = REL_SCOPE_GTT_PRESERVE;
	else if (rel_type == rel_global_temp_delete)
		scope = REL_SCOPE_GTT_DELETE;
	else
		scope = REL_SCOPE_PERSISTENT;

	str.printf(scope, rel_name);
}
