/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Data Definition Utility
 *	MODULE:		dyn_delete.epp
 *	DESCRIPTION:	Dynamic data definition - DYN_delete_<x>
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
 * 24-May-2001 Claudio Valderrama - Forbid zero length identifiers,
 *                                  they are not ANSI SQL compliant.
 * 23-May-2001 Claudio Valderrama - Move here DYN_delete_role.
 * 20-Jun-2001 Claudio Valderrama - Make available DYN_delete_generator.
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>

#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
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
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dyn_proto.h"
#include "../jrd/dyn_proto.h"
#include "../jrd/dyn_dl_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/vio_proto.h"
#include "../common/utils_proto.h"

using MsgFormat::SafeArg;

using namespace Jrd;
using namespace Firebird;


/*DATABASE DB = STATIC "ODS.RDB";*/
static const UCHAR	jrd_0 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',9,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,1,
21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_9 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',3,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,1,
21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_18 [185] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,41,3,0,32,0,7,0,4,0,2,0,41,
3,0,32,0,41,3,0,32,0,12,0,2,7,'C',1,'K',2,0,0,'G',58,47,24,0,
0,0,25,0,1,0,58,61,24,0,3,0,58,61,24,0,23,0,58,61,24,0,7,0,58,
55,24,0,0,0,25,0,0,0,58,59,60,'C',1,'K',5,0,1,'G',47,24,1,2,0,
24,0,0,0,255,59,60,'C',1,'K',27,0,2,'G',47,24,2,4,0,24,0,0,0,
255,255,2,14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,
0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,
0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_29 [143] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,4,0,41,3,0,32,0,41,
3,0,32,0,41,3,0,32,0,41,0,0,12,0,12,0,2,7,'C',1,'K',22,0,0,'G',
58,47,24,0,0,0,25,0,2,0,58,47,24,0,1,0,25,0,3,0,58,47,24,0,2,
0,25,0,1,0,47,24,0,5,0,25,0,0,0,255,2,14,1,2,1,21,8,0,1,0,0,0,
25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,
0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_41 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',21,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,
1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_50 [113] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',22,0,0,'G',58,47,24,0,0,0,25,0,1,
0,47,24,0,2,0,25,0,0,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,
0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_60 [110] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,7,
0,12,0,2,7,'C',1,'K',17,0,0,'G',58,47,24,0,0,0,25,0,0,0,47,24,
0,1,0,25,0,1,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_70 [121] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,7,0,7,0,4,0,1,0,41,3,0,32,
0,12,0,2,7,'C',1,'K',5,0,0,'G',47,24,0,1,0,25,0,0,0,255,2,14,
1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,8,0,25,1,1,0,255,17,0,9,
13,12,3,18,0,12,2,10,0,1,2,1,25,2,0,0,24,1,8,0,255,255,255,14,
1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_80 [107] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',3,'K',7,0,0,
'K',5,0,1,'K',12,0,2,'D',21,8,0,1,0,0,0,'G',58,47,24,0,0,0,25,
0,0,0,58,47,24,1,1,0,24,0,0,0,47,24,1,1,0,24,2,1,0,255,14,1,2,
1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,
255,255,76
   };	// end of blr string 
static const UCHAR	jrd_85 [110] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,7,
0,12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,0,0,25,0,0,0,47,24,
0,6,0,25,0,1,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_95 [116] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,1,
0,41,3,0,32,0,12,0,2,7,'C',1,'K',12,0,0,'G',47,24,0,0,0,25,0,
0,0,255,2,14,1,2,1,24,0,1,0,41,1,0,0,2,0,1,21,8,0,1,0,0,0,25,
1,1,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,
0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_106 [130] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,1,
0,41,3,0,32,0,12,0,2,7,'C',2,'K',17,0,0,'K',12,0,1,'G',58,47,
24,0,0,0,25,0,0,0,47,24,1,0,0,24,0,0,0,255,2,14,1,2,1,24,1,1,
0,41,1,0,0,2,0,1,21,8,0,1,0,0,0,25,1,1,0,255,17,0,9,13,12,3,18,
0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_117 [95] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,7,0,12,0,2,7,
'C',1,'K',10,0,0,'G',47,24,0,5,0,25,0,0,0,255,2,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,
1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_126 [137] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,4,0,41,3,0,32,0,41,
3,0,32,0,7,0,7,0,12,0,2,7,'C',1,'K',18,0,0,'G',57,58,47,24,0,
4,0,25,0,1,0,47,24,0,7,0,25,0,3,0,58,47,24,0,0,0,25,0,0,0,47,
24,0,6,0,25,0,2,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,
17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,
1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_138 [127] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,4,0,41,3,0,32,0,7,0,7,0,7,0,4,
0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',31,0,0,'G',47,24,0,0,0,25,
0,0,0,255,2,14,1,2,1,24,0,1,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,
1,0,1,24,0,3,0,41,1,3,0,2,0,255,17,0,9,13,12,3,18,0,12,2,5,0,
255,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_150 [110] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,7,
0,12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,0,0,25,0,0,0,47,24,
0,6,0,25,0,1,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_160 [110] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,7,
0,12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,4,0,25,0,0,0,47,24,
0,7,0,25,0,1,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_170 [110] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,7,
0,12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,0,0,25,0,0,0,47,24,
0,6,0,25,0,1,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_180 [112] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,41,3,0,32,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',12,0,0,'G',47,24,0,1,0,25,0,0,0,255,
2,14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_190 [116] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,1,
0,41,3,0,32,0,12,0,2,7,'C',1,'K',6,0,0,'G',47,24,0,8,0,25,0,0,
0,255,2,14,1,2,1,24,0,9,0,41,1,0,0,2,0,1,21,8,0,1,0,0,0,25,1,
1,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,
0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_201 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',7,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,1,
21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_210 [130] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,4,0,41,3,0,32,0,41,3,0,32,0,7,
0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',5,0,0,'G',47,24,
0,1,0,25,0,0,0,255,2,14,1,2,1,24,0,2,0,25,1,0,0,1,24,0,14,0,41,
1,1,0,3,0,1,21,8,0,1,0,0,0,25,1,2,0,255,17,0,9,13,12,3,18,0,12,
2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_222 [128] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,3,0,41,3,0,32,0,41,
0,0,12,0,41,0,0,12,0,12,0,2,7,'C',1,'K',22,0,0,'G',58,47,24,0,
2,0,25,0,0,0,57,47,24,0,1,0,25,0,2,0,47,24,0,1,0,25,0,1,0,255,
2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,
2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_233 [112] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',2,'K',17,0,0,'K',12,0,1,'G',58,47,24,1,1,0,25,0,0,0,
47,24,0,0,0,24,1,0,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,
17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,
1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_242 [112] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,41,3,0,32,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',4,0,0,'G',47,24,0,1,0,25,0,0,0,255,
2,14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_252 [150] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,4,0,41,3,0,32,0,41,
0,0,12,0,41,0,0,12,0,41,0,0,12,0,12,0,2,7,'C',1,'K',22,0,0,'G',
58,47,24,0,2,0,25,0,0,0,57,47,24,0,1,0,25,0,3,0,57,47,24,0,1,
0,25,0,2,0,47,24,0,1,0,25,0,1,0,'F',1,'H',24,0,1,0,255,2,14,1,
2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,
255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_264 [110] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,7,
0,12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,0,0,25,0,0,0,47,24,
0,6,0,25,0,1,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_274 [110] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,7,
0,12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,4,0,25,0,0,0,47,24,
0,7,0,25,0,1,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_284 [116] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,1,
0,41,3,0,32,0,12,0,2,7,'C',1,'K',26,0,0,'G',47,24,0,0,0,25,0,
0,0,255,2,14,1,2,1,24,0,7,0,41,1,0,0,2,0,1,21,8,0,1,0,0,0,25,
1,1,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,
0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_295 [122] =
   {	// blr string 

4,2,4,1,5,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,4,0,2,0,41,3,
0,32,0,41,3,0,32,0,12,0,2,7,'C',1,'K',27,0,0,'G',58,47,24,0,1,
0,25,0,1,0,47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,12,0,41,1,0,
0,3,0,1,24,0,13,0,41,1,1,0,4,0,1,21,8,0,1,0,0,0,25,1,2,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_305 [113] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',2,0,0,'G',58,47,24,0,0,0,25,0,1,0,
55,24,0,0,0,25,0,0,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,
17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,
1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_315 [144] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,5,0,41,3,0,32,0,41,3,0,32,0,41,
3,0,32,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',27,0,
0,'G',47,24,0,1,0,25,0,0,0,255,2,14,1,2,1,24,0,0,0,25,1,0,0,1,
24,0,1,0,25,1,1,0,1,24,0,4,0,41,1,2,0,4,0,1,21,8,0,1,0,0,0,25,
1,3,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,
0,0,0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_328 [122] =
   {	// blr string 

4,2,4,1,5,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,4,0,2,0,41,3,
0,32,0,41,3,0,32,0,12,0,2,7,'C',1,'K',27,0,0,'G',58,47,24,0,1,
0,25,0,1,0,47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,12,0,41,1,0,
0,3,0,1,24,0,13,0,41,1,1,0,4,0,1,21,8,0,1,0,0,0,25,1,2,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_338 [132] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',2,0,0,'G',58,47,24,0,0,0,25,0,1,0,
58,55,24,0,0,0,25,0,0,0,57,47,24,0,15,0,21,8,0,0,0,0,0,61,24,
0,15,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,
3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_348 [159] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,5,0,41,3,0,32,0,41,3,0,32,0,41,
3,0,32,0,7,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',27,0,0,'G',58,47,24,0,1,0,25,0,1,0,47,24,0,0,0,25,0,0,0,
255,2,14,1,2,1,24,0,0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,1,24,0,4,
0,41,1,2,0,4,0,1,21,8,0,1,0,0,0,25,1,3,0,255,17,0,9,13,12,3,18,
0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_362 [125] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,3,0,41,3,0,32,0,41,
3,0,32,0,7,0,12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,4,0,25,
0,1,0,58,47,24,0,5,0,25,0,0,0,47,24,0,7,0,25,0,2,0,255,2,14,1,
2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,
255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_373 [159] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,5,0,41,3,0,32,0,41,3,0,32,0,41,
3,0,32,0,7,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',5,0,0,'G',58,47,24,0,0,0,25,0,1,0,47,24,0,1,0,25,0,0,0,
255,2,14,1,2,1,24,0,1,0,25,1,0,0,1,24,0,2,0,25,1,1,0,1,24,0,14,
0,41,1,2,0,4,0,1,21,8,0,1,0,0,0,25,1,3,0,255,17,0,9,13,12,3,18,
0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_387 [144] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,
0,2,7,'C',2,'K',4,0,0,'K',3,0,1,'G',58,47,24,0,0,0,24,1,0,0,58,
47,24,0,1,0,25,0,1,0,58,47,24,1,1,0,25,0,0,0,59,60,'C',1,'K',
22,0,2,'G',58,47,24,2,2,0,24,0,1,0,47,24,2,5,0,24,0,0,0,255,255,
14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,
1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_394 [183] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,4,0,4,0,41,3,0,32,
0,41,3,0,32,0,41,3,0,32,0,41,0,0,12,0,12,0,2,7,'C',3,'K',4,0,
0,'K',3,0,1,'K',22,0,2,'G',58,47,24,0,1,0,25,0,2,0,58,47,24,2,
2,0,25,0,1,0,58,47,24,1,1,0,25,0,0,0,58,47,24,0,0,0,24,1,0,0,
58,47,24,0,0,0,24,2,5,0,47,24,2,1,0,25,0,3,0,255,14,1,2,1,24,
0,0,0,25,1,0,0,1,24,2,0,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,
1,24,0,5,0,25,1,3,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_405 [158] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,
0,2,7,'C',3,'K',5,0,0,'K',5,0,1,'K',7,0,2,'G',58,47,24,0,1,0,
25,0,1,0,58,47,24,0,0,0,25,0,0,0,58,47,24,0,0,0,24,1,4,0,58,47,
24,0,2,0,24,1,2,0,58,47,24,1,1,0,24,2,0,0,58,47,24,0,1,0,24,2,
1,0,47,24,1,10,0,24,2,2,0,255,14,1,2,1,24,1,1,0,25,1,0,0,1,21,
8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_412 [127] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,4,0,9,0,41,3,0,32,0,7,0,7,0,4,
0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',4,0,0,'G',47,24,0,0,0,25,
0,0,0,255,2,14,1,2,1,24,0,10,0,41,1,0,0,3,0,1,24,0,1,0,25,1,1,
0,1,21,8,0,1,0,0,0,25,1,2,0,255,17,0,9,13,12,3,18,0,12,2,5,0,
255,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_424 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',2,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,1,
21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_433 [113] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',27,0,0,'G',47,24,0,4,0,25,0,0,0,255,
14,1,2,1,24,0,0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,1,24,0,4,0,25,
1,2,0,1,21,8,0,1,0,0,0,25,1,3,0,255,14,1,1,21,8,0,0,0,0,0,25,
1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_441 [113] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',5,0,0,'G',47,24,0,2,0,25,0,0,0,255,
14,1,2,1,24,0,0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,1,24,0,2,0,25,
1,2,0,1,21,8,0,1,0,0,0,25,1,3,0,255,14,1,1,21,8,0,0,0,0,0,25,
1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_449 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',20,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,
1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_458 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',14,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,
1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_467 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',15,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,
1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_476 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',16,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,
1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_485 [98] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',30,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,
1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_494 [94] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,2,0,7,0,7,0,12,0,2,7,'C',1,'K',
2,0,0,'G',58,47,24,0,26,0,25,0,1,0,47,24,0,25,0,25,0,0,0,255,
14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,
1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_501 [122] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,2,0,7,0,7,0,12,0,
2,7,'C',2,'K',27,0,0,'K',2,0,1,'G',58,47,24,0,4,0,24,1,0,0,58,
47,24,1,26,0,25,0,1,0,47,24,0,9,0,25,0,0,0,255,14,1,2,1,24,0,
0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_509 [122] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,2,0,7,0,7,0,12,0,
2,7,'C',2,'K',5,0,0,'K',2,0,1,'G',58,47,24,0,2,0,24,1,0,0,58,
47,24,1,26,0,25,0,1,0,47,24,0,18,0,25,0,0,0,255,14,1,2,1,24,0,
0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_517 [195] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,9,0,41,3,0,32,0,41,3,0,32,0,41,
3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,
'C',2,'K',29,0,0,'K',28,0,1,'G',58,47,24,0,0,0,25,0,0,0,47,24,
1,4,0,24,0,2,0,255,2,14,1,2,1,24,1,0,0,25,1,0,0,1,24,0,0,0,25,
1,1,0,1,24,1,3,0,41,1,2,0,5,0,1,21,8,0,1,0,0,0,25,1,3,0,1,24,
0,2,0,25,1,4,0,1,24,0,1,0,25,1,6,0,1,24,0,4,0,41,1,8,0,7,0,255,
17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,
1,3,0,255,255,76
   };	// end of blr string 


static bool delete_constraint_records(Global*, const Firebird::MetaName&, const Firebird::MetaName&);
static bool delete_dimension_records(Global*, const Firebird::MetaName&);
static void delete_f_key_constraint(thread_db*, Global*,
									const Firebird::MetaName&, const Firebird::MetaName&,
									const Firebird::MetaName&, const Firebird::MetaName&);
static void delete_gfield_for_lfield(Global*, const Firebird::MetaName&);
static bool delete_index_segment_records(Global*, const Firebird::MetaName&);
static bool delete_security_class2(Global*, const Firebird::MetaName&);


void DYN_delete_collation(Global* gbl, const UCHAR** ptr)
{
   struct {
          TEXT  jrd_499 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_500;	// gds__utility 
   } jrd_498;
   struct {
          SSHORT jrd_496;	// RDB$COLLATION_ID 
          SSHORT jrd_497;	// RDB$CHARACTER_SET_ID 
   } jrd_495;
   struct {
          TEXT  jrd_506 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_507 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_508;	// gds__utility 
   } jrd_505;
   struct {
          SSHORT jrd_503;	// RDB$COLLATION_ID 
          SSHORT jrd_504;	// RDB$CHARACTER_SET_ID 
   } jrd_502;
   struct {
          TEXT  jrd_514 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_515 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_516;	// gds__utility 
   } jrd_513;
   struct {
          SSHORT jrd_511;	// RDB$COLLATION_ID 
          SSHORT jrd_512;	// RDB$CHARACTER_SET_ID 
   } jrd_510;
   struct {
          SSHORT jrd_533;	// gds__utility 
   } jrd_532;
   struct {
          SSHORT jrd_531;	// gds__utility 
   } jrd_530;
   struct {
          TEXT  jrd_521 [32];	// RDB$CHARACTER_SET_NAME 
          TEXT  jrd_522 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_523 [32];	// RDB$DEFAULT_COLLATE_NAME 
          SSHORT jrd_524;	// gds__utility 
          SSHORT jrd_525;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_526;	// gds__null_flag 
          SSHORT jrd_527;	// RDB$COLLATION_ID 
          SSHORT jrd_528;	// gds__null_flag 
          SSHORT jrd_529;	// RDB$SYSTEM_FLAG 
   } jrd_520;
   struct {
          TEXT  jrd_519 [32];	// RDB$COLLATION_NAME 
   } jrd_518;
/**************************************
 *
 *	D Y N _ d e l e t e _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a collation from rdb$collations.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = NULL;

	bool found = false;
	Firebird::MetaName collName;

	try
	{
        GET_STRING(ptr, collName);

        request = CMP_find_request(tdbb, drq_e_colls, DYN_REQUESTS);

        /*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
            COLL IN RDB$COLLATIONS
            CROSS CS IN RDB$CHARACTER_SETS
            WITH COLL.RDB$COLLATION_NAME EQ collName.c_str() AND
				 CS.RDB$CHARACTER_SET_ID EQ COLL.RDB$CHARACTER_SET_ID*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_517, sizeof(jrd_517), true);
	gds__vtov ((const char*) collName.c_str(), (char*) jrd_518.jrd_519, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_518);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 108, (UCHAR*) &jrd_520);
	   if (!jrd_520.jrd_524) break;

            if (!DYN_REQUEST(drq_e_colls))
                DYN_REQUEST(drq_e_colls) = request;

			if (!/*COLL.RDB$SYSTEM_FLAG.NULL*/
			     jrd_520.jrd_528 && /*COLL.RDB$SYSTEM_FLAG*/
    jrd_520.jrd_529 == 1)
			{
				DYN_rundown_request(request, -1);
				DYN_error_punt(false, 237);
				// msg 237: "Cannot delete system collation"
			}

			if (/*COLL.RDB$COLLATION_ID*/
			    jrd_520.jrd_527 == 0 ||
				(!/*CS.RDB$DEFAULT_COLLATE_NAME.NULL*/
				  jrd_520.jrd_526 &&
					Firebird::MetaName(/*COLL.RDB$COLLATION_NAME*/
							   jrd_520.jrd_522) == Firebird::MetaName(/*CS.RDB$DEFAULT_COLLATE_NAME*/
			jrd_520.jrd_523)))
			{
				fb_utils::exact_name_limit(/*CS.RDB$CHARACTER_SET_NAME*/
							   jrd_520.jrd_521,
					sizeof(/*CS.RDB$CHARACTER_SET_NAME*/
					       jrd_520.jrd_521));

				DYN_rundown_request(request, -1);
				DYN_error_punt(false, 238, /*CS.RDB$CHARACTER_SET_NAME*/
							   jrd_520.jrd_521);
				// msg 238: "Cannot delete default collation of CHARACTER SET %s"
			}

		    found = true;
			fb_utils::exact_name_limit(/*COLL.RDB$COLLATION_NAME*/
						   jrd_520.jrd_522,
				sizeof(/*COLL.RDB$COLLATION_NAME*/
				       jrd_520.jrd_522));

	        jrd_req* request2 = CMP_find_request(tdbb, drq_l_rfld_coll, DYN_REQUESTS);

			/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE gbl->gbl_transaction)
				RF IN RDB$RELATION_FIELDS CROSS F IN RDB$FIELDS
				WITH RF.RDB$FIELD_SOURCE EQ F.RDB$FIELD_NAME AND
					 F.RDB$CHARACTER_SET_ID EQ COLL.RDB$CHARACTER_SET_ID AND
					 RF.RDB$COLLATION_ID EQ COLL.RDB$COLLATION_ID*/
			{
			if (!request2)
			   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_509, sizeof(jrd_509), true);
			jrd_510.jrd_511 = jrd_520.jrd_527;
			jrd_510.jrd_512 = jrd_520.jrd_525;
			EXE_start (tdbb, request2, gbl->gbl_transaction);
			EXE_send (tdbb, request2, 0, 4, (UCHAR*) &jrd_510);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 66, (UCHAR*) &jrd_513);
			   if (!jrd_513.jrd_516) break;

				if (!DYN_REQUEST(drq_l_rfld_coll))
					DYN_REQUEST(drq_l_rfld_coll) = request2;

				fb_utils::exact_name_limit(/*RF.RDB$RELATION_NAME*/
							   jrd_513.jrd_515, sizeof(/*RF.RDB$RELATION_NAME*/
	 jrd_513.jrd_515));
				fb_utils::exact_name_limit(/*RF.RDB$FIELD_NAME*/
							   jrd_513.jrd_514, sizeof(/*RF.RDB$FIELD_NAME*/
	 jrd_513.jrd_514));

				DYN_rundown_request(request2, -1);
				DYN_error_punt(false, 235, SafeArg() << /*COLL.RDB$COLLATION_NAME*/
									jrd_520.jrd_522 <<
								/*RF.RDB$RELATION_NAME*/
								jrd_513.jrd_515 << /*RF.RDB$FIELD_NAME*/
    jrd_513.jrd_514);
				// msg 235: "Collation %s is used in table %s (field name %s) and cannot be dropped"
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_l_rfld_coll))
				DYN_REQUEST(drq_l_rfld_coll) = request2;

	        request2 = CMP_find_request(tdbb, drq_l_prm_coll, DYN_REQUESTS);

			/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE gbl->gbl_transaction)
				PRM IN RDB$PROCEDURE_PARAMETERS CROSS F IN RDB$FIELDS
				WITH PRM.RDB$FIELD_SOURCE EQ F.RDB$FIELD_NAME AND
					 F.RDB$CHARACTER_SET_ID EQ COLL.RDB$CHARACTER_SET_ID AND
					 PRM.RDB$COLLATION_ID EQ COLL.RDB$COLLATION_ID*/
			{
			if (!request2)
			   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_501, sizeof(jrd_501), true);
			jrd_502.jrd_503 = jrd_520.jrd_527;
			jrd_502.jrd_504 = jrd_520.jrd_525;
			EXE_start (tdbb, request2, gbl->gbl_transaction);
			EXE_send (tdbb, request2, 0, 4, (UCHAR*) &jrd_502);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 66, (UCHAR*) &jrd_505);
			   if (!jrd_505.jrd_508) break;

				if (!DYN_REQUEST(drq_l_prm_coll))
					DYN_REQUEST(drq_l_prm_coll) = request2;

				fb_utils::exact_name_limit(/*PRM.RDB$PROCEDURE_NAME*/
							   jrd_505.jrd_507, sizeof(/*PRM.RDB$PROCEDURE_NAME*/
	 jrd_505.jrd_507));
				fb_utils::exact_name_limit(/*PRM.RDB$PARAMETER_NAME*/
							   jrd_505.jrd_506, sizeof(/*PRM.RDB$PARAMETER_NAME*/
	 jrd_505.jrd_506));

				DYN_rundown_request(request2, -1);
				DYN_error_punt(false, 243, SafeArg() << /*COLL.RDB$COLLATION_NAME*/
									jrd_520.jrd_522 <<
								/*PRM.RDB$PROCEDURE_NAME*/
								jrd_505.jrd_507 << /*PRM.RDB$PARAMETER_NAME*/
    jrd_505.jrd_506);
				// msg 243: "Collation %s is used in procedure %s (parameter name %s) and cannot be dropped"
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_l_prm_coll))
				DYN_REQUEST(drq_l_prm_coll) = request2;

	        request2 = CMP_find_request(tdbb, drq_l_fld_coll, DYN_REQUESTS);

			/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE gbl->gbl_transaction)
				F IN RDB$FIELDS
				WITH F.RDB$CHARACTER_SET_ID EQ COLL.RDB$CHARACTER_SET_ID AND
					 F.RDB$COLLATION_ID EQ COLL.RDB$COLLATION_ID*/
			{
			if (!request2)
			   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_494, sizeof(jrd_494), true);
			jrd_495.jrd_496 = jrd_520.jrd_527;
			jrd_495.jrd_497 = jrd_520.jrd_525;
			EXE_start (tdbb, request2, gbl->gbl_transaction);
			EXE_send (tdbb, request2, 0, 4, (UCHAR*) &jrd_495);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 34, (UCHAR*) &jrd_498);
			   if (!jrd_498.jrd_500) break;

				if (!DYN_REQUEST(drq_l_fld_coll))
					DYN_REQUEST(drq_l_fld_coll) = request2;

				fb_utils::exact_name_limit(/*F.RDB$FIELD_NAME*/
							   jrd_498.jrd_499, sizeof(/*F.RDB$FIELD_NAME*/
	 jrd_498.jrd_499));

				DYN_rundown_request(request2, -1);
				DYN_error_punt(false, 236, SafeArg() << /*COLL.RDB$COLLATION_NAME*/
									jrd_520.jrd_522 << /*F.RDB$FIELD_NAME*/
    jrd_498.jrd_499);
				// msg 236: "Collation %s is used in domain %s and cannot be dropped"
			/*END_FOR;*/
			   }
			}

			if (!DYN_REQUEST(drq_l_fld_coll))
				DYN_REQUEST(drq_l_fld_coll) = request2;

			/*ERASE COLL;*/
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_530);
        /*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_532);
	   }
	}

        if (!DYN_REQUEST(drq_e_colls))
            DYN_REQUEST(drq_e_colls) = request;
    }
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
	    DYN_rundown_request(request, -1);
		DYN_error_punt(true, 233);
		// msg 234: "ERASE RDB$COLLATIONS failed"
    }

	if (!found)
	{
		DYN_error_punt(false, 152, collName.c_str());
	    // msg 152: "Collation %s not found"
	}
}


void DYN_delete_constraint (Global* gbl, const UCHAR** ptr, const Firebird::MetaName* relation)
{
/**************************************
 *
 *	D Y N _ d e l e t e _ c o n s t r a i n t
 *
 **************************************
 *
 * Functional description
 *	Execute ddl to DROP an Integrity Constraint
 *
 **************************************/
	Firebird::MetaName rel_name, constraint;

/* GET name of the constraint to be deleted  */

	GET_STRING(ptr, constraint);

	if (relation)
		rel_name = *relation;
	else if (*(*ptr)++ != isc_dyn_rel_name) {
		DYN_error_punt(false, 128);
		/* msg 128: "No relation specified in delete_constraint" */
	}
	else
		GET_STRING(ptr, rel_name);

	if (!delete_constraint_records(gbl, constraint, rel_name))
		DYN_error_punt(false, 130, constraint.c_str());
		/* msg 130: "CONSTRAINT %s does not exist." */
}


void DYN_delete_dimensions(Global* gbl,
						   const UCHAR** ptr)
						   //const Firebird::MetaName* relation_name,
						   //Firebird::MetaName* field_name) // Obtained from the stream
{
/**************************************
 *
 *	D Y N _ d e l e t e _ d i m e n s i o n s
 *
 **************************************
 *
 * Functional description
 *	Delete dimensions associated with
 *	a global field.  Used when modifying
 *	the datatype and from places where a
 *	field is deleted directly in the system
 *	relations.  The DYN version of delete
 *	global field deletes the dimensions for
 *	you.
 *
 **************************************/
	Firebird::MetaName f;

	GET_STRING(ptr, f);

	delete_dimension_records(gbl, f);

	while (*(*ptr)++ != isc_dyn_end) {
		--(*ptr);
		DYN_execute(gbl, ptr, NULL, &f, NULL, NULL, NULL);
	}
}


void DYN_delete_exception( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_493;	// gds__utility 
   } jrd_492;
   struct {
          SSHORT jrd_491;	// gds__utility 
   } jrd_490;
   struct {
          SSHORT jrd_489;	// gds__utility 
   } jrd_488;
   struct {
          TEXT  jrd_487 [32];	// RDB$EXCEPTION_NAME 
   } jrd_486;
/**************************************
 *
 *	D Y N _ d e l e t e _ e x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes an exception.
 *
 **************************************/
	Firebird::MetaName t;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	GET_STRING(ptr, t);

	jrd_req* request = CMP_find_request(tdbb, drq_e_xcp, DYN_REQUESTS);

	bool found = false;

	try {

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$EXCEPTIONS
        WITH X.RDB$EXCEPTION_NAME EQ t.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_485, sizeof(jrd_485), true);
	gds__vtov ((const char*) t.c_str(), (char*) jrd_486.jrd_487, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_486);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_488);
	   if (!jrd_488.jrd_489) break;
        if (!DYN_REQUEST(drq_e_xcp))
            DYN_REQUEST(drq_e_xcp) = request;

		found = true;
		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_490);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_492);
	   }
	}
	if (!DYN_REQUEST(drq_e_xcp))
		DYN_REQUEST(drq_e_xcp) = request;

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 143);
		/* msg 143: "ERASE EXCEPTION failed" */
	}

	if (!found) {
		DYN_error_punt(false, 144);
		/* msg 144: "Exception not found" */
	}

	if (*(*ptr)++ != isc_dyn_end) {
		DYN_unsupported_verb();
	}
}


void DYN_delete_filter( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_484;	// gds__utility 
   } jrd_483;
   struct {
          SSHORT jrd_482;	// gds__utility 
   } jrd_481;
   struct {
          SSHORT jrd_480;	// gds__utility 
   } jrd_479;
   struct {
          TEXT  jrd_478 [32];	// RDB$FUNCTION_NAME 
   } jrd_477;
/**************************************
 *
 *	D Y N _ d e l e t e _ f i l t e r
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a blob filter.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_filters, DYN_REQUESTS);

	bool found = false;

	Firebird::MetaName f;
	GET_STRING(ptr, f);

	try {

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$FILTERS WITH X.RDB$FUNCTION_NAME = f.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_476, sizeof(jrd_476), true);
	gds__vtov ((const char*) f.c_str(), (char*) jrd_477.jrd_478, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_477);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_479);
	   if (!jrd_479.jrd_480) break;
        if (!DYN_REQUEST(drq_e_filters))
            DYN_REQUEST(drq_e_filters) = request;

		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_481);
		found = true;
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_483);
	   }
	}
	if (!DYN_REQUEST(drq_e_filters))
		DYN_REQUEST(drq_e_filters) = request;
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 36);
		/* msg 36: "ERASE BLOB FILTER failed" */
	}

	if (!found) {
		DYN_error_punt(false, 37, f.c_str());
		/* msg 37: "Blob Filter %s not found" */
	}

	if (*(*ptr)++ != isc_dyn_end) {
		DYN_unsupported_verb();
	}
}


void DYN_delete_function( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_466;	// gds__utility 
   } jrd_465;
   struct {
          SSHORT jrd_464;	// gds__utility 
   } jrd_463;
   struct {
          SSHORT jrd_462;	// gds__utility 
   } jrd_461;
   struct {
          TEXT  jrd_460 [32];	// RDB$FUNCTION_NAME 
   } jrd_459;
   struct {
          SSHORT jrd_475;	// gds__utility 
   } jrd_474;
   struct {
          SSHORT jrd_473;	// gds__utility 
   } jrd_472;
   struct {
          SSHORT jrd_471;	// gds__utility 
   } jrd_470;
   struct {
          TEXT  jrd_469 [32];	// RDB$FUNCTION_NAME 
   } jrd_468;
/**************************************
 *
 *	D Y N _ d e l e t e _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a user defined function.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_func_args, DYN_REQUESTS);
	USHORT id = drq_e_func_args;

	bool found = false;
	Firebird::MetaName f;
	GET_STRING(ptr, f);

	try {
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		FA IN RDB$FUNCTION_ARGUMENTS WITH FA.RDB$FUNCTION_NAME EQ f.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_467, sizeof(jrd_467), true);
	gds__vtov ((const char*) f.c_str(), (char*) jrd_468.jrd_469, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_468);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_470);
	   if (!jrd_470.jrd_471) break;
        if (!DYN_REQUEST(drq_e_func_args))
            DYN_REQUEST(drq_e_func_args) = request;

		/*ERASE FA;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_472);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_474);
	   }
	}
	if (!DYN_REQUEST(drq_e_func_args))
		DYN_REQUEST(drq_e_func_args) = request;

	request = CMP_find_request(tdbb, drq_e_funcs, DYN_REQUESTS);
	id = drq_e_funcs;

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$FUNCTIONS WITH X.RDB$FUNCTION_NAME EQ f.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_458, sizeof(jrd_458), true);
	gds__vtov ((const char*) f.c_str(), (char*) jrd_459.jrd_460, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_459);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_461);
	   if (!jrd_461.jrd_462) break;
        if (!DYN_REQUEST(drq_e_funcs))
            DYN_REQUEST(drq_e_funcs) = request;

		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_463);
		found = true;
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_465);
	   }
	}
	if (!DYN_REQUEST(drq_e_funcs))
		DYN_REQUEST(drq_e_funcs) = request;
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		if (id == drq_e_func_args)
		{
			DYN_error_punt(true, 39);
			/* msg 39: "ERASE RDB$FUNCTION_ARGUMENTS failed" */
		}
		else
		{
			DYN_error_punt(true, 40);
			/* msg 40: "ERASE RDB$FUNCTIONS failed" */
		}
	}

	if (!found)
	{
		DYN_error_punt(false, 41, f.c_str());
		/* msg 41: "Function %s not found" */
	}

	if (*(*ptr)++ != isc_dyn_end) {
		DYN_unsupported_verb();
	}
}


void DYN_delete_generator(Global* gbl, const UCHAR**ptr)
{
   struct {
          SSHORT jrd_457;	// gds__utility 
   } jrd_456;
   struct {
          SSHORT jrd_455;	// gds__utility 
   } jrd_454;
   struct {
          SSHORT jrd_453;	// gds__utility 
   } jrd_452;
   struct {
          TEXT  jrd_451 [32];	// RDB$GENERATOR_NAME 
   } jrd_450;
/**************************************
 *
 *	D Y N _ d e l e t e _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a generator from rdb$generator but the
 *  space allocated in the page won't be released.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = NULL;

	bool found = false;
	Firebird::MetaName t;
    GET_STRING(ptr, t);

	try {
        request = CMP_find_request(tdbb, drq_e_gens, DYN_REQUESTS);

        found = false;
        /*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
            X IN RDB$GENERATORS
            WITH X.RDB$GENERATOR_NAME EQ t.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_449, sizeof(jrd_449), true);
	gds__vtov ((const char*) t.c_str(), (char*) jrd_450.jrd_451, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_450);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_452);
	   if (!jrd_452.jrd_453) break;

            if (!DYN_REQUEST(drq_e_gens))
                DYN_REQUEST(drq_e_gens) = request;

		    found = true;
            /*ERASE X;*/
	    EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_454);
        /*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_456);
	   }
	}

        if (!DYN_REQUEST(drq_e_gens))
            DYN_REQUEST(drq_e_gens) = request;

    }
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
	    DYN_rundown_request(request, -1);
		DYN_error_punt(true, 213);
		/* msg 213: "ERASE GENERATOR failed" */
    }

	if (!found) {
		DYN_error_punt(false, 214, t.c_str());
	    /* msg 214: "Generator %s not found" */
	}
}


void DYN_delete_global_field( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_432;	// gds__utility 
   } jrd_431;
   struct {
          SSHORT jrd_430;	// gds__utility 
   } jrd_429;
   struct {
          SSHORT jrd_428;	// gds__utility 
   } jrd_427;
   struct {
          TEXT  jrd_426 [32];	// RDB$FIELD_NAME 
   } jrd_425;
   struct {
          TEXT  jrd_437 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_438 [32];	// RDB$PROCEDURE_NAME 
          TEXT  jrd_439 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_440;	// gds__utility 
   } jrd_436;
   struct {
          TEXT  jrd_435 [32];	// RDB$FIELD_SOURCE 
   } jrd_434;
   struct {
          TEXT  jrd_445 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_446 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_447 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_448;	// gds__utility 
   } jrd_444;
   struct {
          TEXT  jrd_443 [32];	// RDB$FIELD_SOURCE 
   } jrd_442;
/**************************************
 *
 *	D Y N _ d e l e t e _ g l o b a l _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a global field.
 *
 **************************************/
	Firebird::MetaName f;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_l_fld_src, DYN_REQUESTS);

	bool found = false;

	try {
	GET_STRING(ptr, f);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		Y IN RDB$RELATION_FIELDS WITH Y.RDB$FIELD_SOURCE EQ f.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_441, sizeof(jrd_441), true);
	gds__vtov ((const char*) f.c_str(), (char*) jrd_442.jrd_443, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_442);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 98, (UCHAR*) &jrd_444);
	   if (!jrd_444.jrd_448) break;
        if (!DYN_REQUEST(drq_l_fld_src))
            DYN_REQUEST(drq_l_fld_src) = request;

		fb_utils::exact_name_limit(/*Y.RDB$FIELD_SOURCE*/
					   jrd_444.jrd_447, sizeof(/*Y.RDB$FIELD_SOURCE*/
	 jrd_444.jrd_447));
		fb_utils::exact_name_limit(/*Y.RDB$RELATION_NAME*/
					   jrd_444.jrd_446, sizeof(/*Y.RDB$RELATION_NAME*/
	 jrd_444.jrd_446));
		fb_utils::exact_name_limit(/*Y.RDB$FIELD_NAME*/
					   jrd_444.jrd_445, sizeof(/*Y.RDB$FIELD_NAME*/
	 jrd_444.jrd_445));
		DYN_rundown_request(request, -1);
		DYN_error_punt(false, 43, SafeArg() << /*Y.RDB$FIELD_SOURCE*/
						       jrd_444.jrd_447 << /*Y.RDB$RELATION_NAME*/
    jrd_444.jrd_446 <<
					   /*Y.RDB$FIELD_NAME*/
					   jrd_444.jrd_445);
		// msg 43: "Domain %s is used in table %s (local name %s) and can not be dropped"
	/*END_FOR;*/
	   }
	}
	if (!DYN_REQUEST(drq_l_fld_src))
		DYN_REQUEST(drq_l_fld_src) = request;

	request = CMP_find_request(tdbb, drq_l_prp_src, DYN_REQUESTS);

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$PROCEDURE_PARAMETERS
			WITH X.RDB$FIELD_SOURCE EQ f.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_433, sizeof(jrd_433), true);
	gds__vtov ((const char*) f.c_str(), (char*) jrd_434.jrd_435, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_434);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 98, (UCHAR*) &jrd_436);
	   if (!jrd_436.jrd_440) break;
        if (!DYN_REQUEST(drq_l_prp_src))
            DYN_REQUEST(drq_l_prp_src) = request;

		fb_utils::exact_name_limit(/*X.RDB$FIELD_SOURCE*/
					   jrd_436.jrd_439, sizeof(/*X.RDB$FIELD_SOURCE*/
	 jrd_436.jrd_439));
		fb_utils::exact_name_limit(/*X.RDB$PROCEDURE_NAME*/
					   jrd_436.jrd_438, sizeof(/*X.RDB$PROCEDURE_NAME*/
	 jrd_436.jrd_438));
		fb_utils::exact_name_limit(/*X.RDB$PARAMETER_NAME*/
					   jrd_436.jrd_437, sizeof(/*X.RDB$PARAMETER_NAME*/
	 jrd_436.jrd_437));

		DYN_rundown_request(request, -1);
		DYN_error_punt(false, 239, SafeArg() << /*X.RDB$FIELD_SOURCE*/
							jrd_436.jrd_439 << /*X.RDB$PROCEDURE_NAME*/
    jrd_436.jrd_438 <<
					   /*X.RDB$PARAMETER_NAME*/
					   jrd_436.jrd_437);
		// msg 239: "Domain %s is used in procedure %s (parameter name %s) and cannot be dropped"

	/*END_FOR*/
	   }
	}
	if (!DYN_REQUEST(drq_l_prp_src))
		DYN_REQUEST(drq_l_prp_src) = request;

	request = CMP_find_request(tdbb, drq_e_gfields, DYN_REQUESTS);

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$FIELDS WITH X.RDB$FIELD_NAME EQ f.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_424, sizeof(jrd_424), true);
	gds__vtov ((const char*) f.c_str(), (char*) jrd_425.jrd_426, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_425);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_427);
	   if (!jrd_427.jrd_428) break;
        if (!DYN_REQUEST(drq_e_gfields))
            DYN_REQUEST(drq_e_gfields) = request;

		delete_dimension_records(gbl, f);
		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_429);
		found = true;
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_431);
	   }
	}
	if (!DYN_REQUEST(drq_e_gfields))
		DYN_REQUEST(drq_e_gfields) = request;
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 44);
		/* msg 44: "ERASE RDB$FIELDS failed" */
	}

	if (!found) {
		DYN_error_punt(false, 89);
		// msg 89: "Domain not found"
	}

	while (*(*ptr)++ != isc_dyn_end) {
		--(*ptr);
		DYN_execute(gbl, ptr, NULL, &f, NULL, NULL, NULL);
	}
}


void DYN_delete_index( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_423;	// gds__utility 
   } jrd_422;
   struct {
          SSHORT jrd_421;	// gds__utility 
   } jrd_420;
   struct {
          bid  jrd_416;	// RDB$EXPRESSION_BLR 
          TEXT  jrd_417 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_418;	// gds__utility 
          SSHORT jrd_419;	// gds__null_flag 
   } jrd_415;
   struct {
          TEXT  jrd_414 [32];	// RDB$INDEX_NAME 
   } jrd_413;
/**************************************
 *
 *	D Y N _ d e l e t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes an index.
 *
 **************************************/
	Firebird::MetaName idx_name, rel_name;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_indices, DYN_REQUESTS);

	bool found = false;
	bool is_expression = false;

	try {
	GET_STRING(ptr, idx_name);

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		IDX IN RDB$INDICES WITH IDX.RDB$INDEX_NAME EQ idx_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_412, sizeof(jrd_412), true);
	gds__vtov ((const char*) idx_name.c_str(), (char*) jrd_413.jrd_414, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_413);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 44, (UCHAR*) &jrd_415);
	   if (!jrd_415.jrd_418) break;
        if (!DYN_REQUEST(drq_e_indices))
            DYN_REQUEST(drq_e_indices) = request;

		rel_name = /*IDX.RDB$RELATION_NAME*/
			   jrd_415.jrd_417;
		found = true;
		is_expression = !/*IDX.RDB$EXPRESSION_BLR.NULL*/
				 jrd_415.jrd_419;
		/*ERASE IDX;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_420);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_422);
	   }
	}
	if (!DYN_REQUEST(drq_e_indices))
		DYN_REQUEST(drq_e_indices) = request;
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 47);
		/* msg 47: "ERASE RDB$INDICES failed" */
	}

	if (!found)
	{
		DYN_error_punt(false, 48);
		/* msg 48: "Index not found" */
	}

	if (!is_expression)
		if (!delete_index_segment_records(gbl, idx_name))
		{
			DYN_error_punt(false, 50);
			/* msg 50: "No segments found for index" */
		}

	while (*(*ptr)++ != isc_dyn_end) {
		--(*ptr);
		DYN_execute(gbl, ptr, &rel_name, NULL, NULL, NULL, NULL);
	}
}


void DYN_delete_local_field(Global* gbl,
							const UCHAR** ptr,
							const Firebird::MetaName* relation_name)
							//Firebird::MetaName* field_name) // Obtained from the stream
{
   struct {
          SSHORT jrd_372;	// gds__utility 
   } jrd_371;
   struct {
          SSHORT jrd_370;	// gds__utility 
   } jrd_369;
   struct {
          SSHORT jrd_368;	// gds__utility 
   } jrd_367;
   struct {
          TEXT  jrd_364 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_365 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_366;	// RDB$OBJECT_TYPE 
   } jrd_363;
   struct {
          SSHORT jrd_386;	// gds__utility 
   } jrd_385;
   struct {
          SSHORT jrd_384;	// gds__utility 
   } jrd_383;
   struct {
          TEXT  jrd_378 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_379 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_380 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_381;	// gds__utility 
          SSHORT jrd_382;	// gds__null_flag 
   } jrd_377;
   struct {
          TEXT  jrd_375 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_376 [32];	// RDB$FIELD_NAME 
   } jrd_374;
   struct {
          TEXT  jrd_392 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_393;	// gds__utility 
   } jrd_391;
   struct {
          TEXT  jrd_389 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_390 [32];	// RDB$RELATION_NAME 
   } jrd_388;
   struct {
          TEXT  jrd_401 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_402 [32];	// RDB$CONSTRAINT_NAME 
          SSHORT jrd_403;	// gds__utility 
          SSHORT jrd_404;	// RDB$SEGMENT_COUNT 
   } jrd_400;
   struct {
          TEXT  jrd_396 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_397 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_398 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_399 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_395;
   struct {
          TEXT  jrd_410 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_411;	// gds__utility 
   } jrd_409;
   struct {
          TEXT  jrd_407 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_408 [32];	// RDB$RELATION_NAME 
   } jrd_406;
/**************************************
 *
 *	D Y N _ d e l e t e _ l o c a l _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl 'delete local field'
 *	statement.
 *
 *  The rules for dropping a regular column:
 *
 *    1. the column is not referenced in any views.
 *    2. the column is not part of any user defined indexes.
 *    3. the column is not used in any SQL statements inside of store
 *         procedures or triggers
 *    4. the column is not part of any check-constraints
 *
 *  The rules for dropping a column that was created as primary key:
 *
 *    1. the column is not defined as any foreign keys
 *    2. the column is not defined as part of compound primary keys
 *
 *  The rules for dropping a column that was created as foreign key:
 *
 *    1. the column is not defined as a compound foreign key. A
 *         compound foreign key is a foreign key consisted of more
 *         than one columns.
 *
 *  The RI enforcement for dropping primary key column is done by system
 *  triggers and the RI enforcement for dropping foreign key column is
 *  done by code and system triggers. See the functional description of
 *  delete_f_key_constraint function for detail.
 *
 **************************************/
	Firebird::MetaName tbl_nm, col_nm, constraint, index_name;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	GET_STRING(ptr, col_nm);

	if (relation_name)
		tbl_nm = *relation_name;
	else if (*(*ptr)++ != isc_dyn_rel_name) {
		DYN_error_punt(false, 51);
		/* msg 51: "No relation specified in ERASE RFR" */
	}
	else
		GET_STRING(ptr, tbl_nm);

	jrd_req* request = CMP_find_request(tdbb, drq_l_dep_flds, DYN_REQUESTS);
	USHORT id = drq_l_dep_flds;

	bool found;

	try {

/*
**  ================================================================
**  ==
**  ==  make sure that column is not referenced in any views
**  ==
**  ================================================================
*/
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$RELATION_FIELDS CROSS Y IN RDB$RELATION_FIELDS CROSS
			Z IN RDB$VIEW_RELATIONS WITH
			X.RDB$RELATION_NAME EQ tbl_nm.c_str() AND
			X.RDB$FIELD_NAME EQ col_nm.c_str() AND
			X.RDB$FIELD_NAME EQ Y.RDB$BASE_FIELD AND
			X.RDB$FIELD_SOURCE EQ Y.RDB$FIELD_SOURCE AND
			Y.RDB$RELATION_NAME EQ Z.RDB$VIEW_NAME AND
			X.RDB$RELATION_NAME EQ Z.RDB$RELATION_NAME AND
			Y.RDB$VIEW_CONTEXT EQ Z.RDB$VIEW_CONTEXT*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_405, sizeof(jrd_405), true);
	gds__vtov ((const char*) col_nm.c_str(), (char*) jrd_406.jrd_407, 32);
	gds__vtov ((const char*) tbl_nm.c_str(), (char*) jrd_406.jrd_408, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_406);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_409);
	   if (!jrd_409.jrd_411) break;
        if (!DYN_REQUEST(drq_l_dep_flds))
            DYN_REQUEST(drq_l_dep_flds) = request;

		DYN_rundown_request(request, -1);
		DYN_error_punt(false, 52, SafeArg() << col_nm.c_str() << tbl_nm.c_str() << /*Y.RDB$RELATION_NAME*/
											   jrd_409.jrd_410);
		/* msg 52: "field %s from relation %s is referenced in view %s" */
	/*END_FOR;*/
	   }
	}
	if (!DYN_REQUEST(drq_l_dep_flds))
		DYN_REQUEST(drq_l_dep_flds) = request;

/*
**  ===============================================================
**  ==
**  ==  If the column to be dropped is being used as a foreign key
**  ==  and the column was not part of any compound foreign key,
**  ==  then we can drop the column. But we have to drop the foreign key
**  ==  constraint first.
**  ==
**  ===============================================================
*/

	request = CMP_find_request(tdbb, drq_g_rel_constr_nm, DYN_REQUESTS);
	id = drq_g_rel_constr_nm;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		IDX IN RDB$INDICES CROSS
			IDX_SEG IN RDB$INDEX_SEGMENTS CROSS
			REL_CONST IN RDB$RELATION_CONSTRAINTS
			WITH IDX.RDB$RELATION_NAME EQ tbl_nm.c_str()
			AND REL_CONST.RDB$RELATION_NAME EQ tbl_nm.c_str()
			AND IDX_SEG.RDB$FIELD_NAME EQ col_nm.c_str()
			AND IDX.RDB$INDEX_NAME EQ IDX_SEG.RDB$INDEX_NAME
			AND IDX.RDB$INDEX_NAME EQ REL_CONST.RDB$INDEX_NAME
			AND REL_CONST.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_394, sizeof(jrd_394), true);
	gds__vtov ((const char*) col_nm.c_str(), (char*) jrd_395.jrd_396, 32);
	gds__vtov ((const char*) tbl_nm.c_str(), (char*) jrd_395.jrd_397, 32);
	gds__vtov ((const char*) tbl_nm.c_str(), (char*) jrd_395.jrd_398, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_395.jrd_399, 12);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 108, (UCHAR*) &jrd_395);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 68, (UCHAR*) &jrd_400);
	   if (!jrd_400.jrd_403) break;
        if (!DYN_REQUEST(drq_g_rel_constr_nm))
            DYN_REQUEST(drq_g_rel_constr_nm) = request;

		if (/*IDX.RDB$SEGMENT_COUNT*/
		    jrd_400.jrd_404 == 1) {
			constraint = /*REL_CONST.RDB$CONSTRAINT_NAME*/
				     jrd_400.jrd_402;
			index_name = /*IDX.RDB$INDEX_NAME*/
				     jrd_400.jrd_401;

			delete_f_key_constraint(tdbb, gbl, tbl_nm, col_nm, constraint, index_name);
		}
		else {
			DYN_rundown_request(request, -1);
			DYN_error_punt(false, 187, SafeArg() << col_nm.c_str() << tbl_nm.c_str() <<
						   /*IDX.RDB$INDEX_NAME*/
						   jrd_400.jrd_401);
			/* msg 187: "field %s from relation %s is referenced in index %s" */
		}

	/*END_FOR;*/
	   }
	}

	if (!DYN_REQUEST(drq_g_rel_constr_nm))
		DYN_REQUEST(drq_g_rel_constr_nm) = request;

/*
**  ================================================================
**  ==
**  ==  make sure that column is not referenced in any user-defined indexes
**  ==
**  ==  NOTE: You still could see the system generated indices even though
**  ==        they were already been deleted when dropping column that was
**  ==        used as foreign key before "commit".
**  ==
**  ================================================================
*/

	request = CMP_find_request(tdbb, drq_e_l_idx, DYN_REQUESTS);
	id = drq_e_l_idx;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		IDX IN RDB$INDICES CROSS
			IDX_SEG IN RDB$INDEX_SEGMENTS
			WITH IDX.RDB$INDEX_NAME EQ IDX_SEG.RDB$INDEX_NAME
			AND IDX.RDB$RELATION_NAME EQ tbl_nm.c_str()
			AND IDX_SEG.RDB$FIELD_NAME EQ col_nm.c_str()
			AND NOT ANY
				REL_CONST IN RDB$RELATION_CONSTRAINTS
				WITH REL_CONST.RDB$RELATION_NAME EQ IDX.RDB$RELATION_NAME
				AND REL_CONST.RDB$INDEX_NAME EQ IDX.RDB$INDEX_NAME*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_387, sizeof(jrd_387), true);
	gds__vtov ((const char*) col_nm.c_str(), (char*) jrd_388.jrd_389, 32);
	gds__vtov ((const char*) tbl_nm.c_str(), (char*) jrd_388.jrd_390, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_388);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_391);
	   if (!jrd_391.jrd_393) break;
        if (!DYN_REQUEST(drq_e_l_idx))
            DYN_REQUEST(drq_e_l_idx) = request;

		DYN_rundown_request(request, -1);
		DYN_error_punt(false, 187, SafeArg() << col_nm.c_str() << tbl_nm.c_str() <<
						fb_utils::exact_name_limit(/*IDX.RDB$INDEX_NAME*/
									   jrd_391.jrd_392, sizeof(/*IDX.RDB$INDEX_NAME*/
	 jrd_391.jrd_392)));
		// msg 187: "field %s from relation %s is referenced in index %s"
	/*END_FOR;*/
	   }
	}
	if (!DYN_REQUEST(drq_e_l_idx))
		DYN_REQUEST(drq_e_l_idx) = request;

	request = CMP_find_request(tdbb, drq_e_lfield, DYN_REQUESTS);
	id = drq_e_lfield;

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		RFR IN RDB$RELATION_FIELDS
			WITH RFR.RDB$FIELD_NAME EQ col_nm.c_str()
			AND RFR.RDB$RELATION_NAME EQ tbl_nm.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_373, sizeof(jrd_373), true);
	gds__vtov ((const char*) tbl_nm.c_str(), (char*) jrd_374.jrd_375, 32);
	gds__vtov ((const char*) col_nm.c_str(), (char*) jrd_374.jrd_376, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_374);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 100, (UCHAR*) &jrd_377);
	   if (!jrd_377.jrd_381) break;
        if (!DYN_REQUEST(drq_e_lfield))
            DYN_REQUEST(drq_e_lfield) = request;

		/*ERASE RFR;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_383);

		if (!/*RFR.RDB$SECURITY_CLASS.NULL*/
		     jrd_377.jrd_382 &&
			!strncmp(/*RFR.RDB$SECURITY_CLASS*/
				 jrd_377.jrd_380, SQL_SECCLASS_PREFIX, SQL_SECCLASS_PREFIX_LEN))
		{
			delete_security_class2(gbl, /*RFR.RDB$SECURITY_CLASS*/
						    jrd_377.jrd_380);
		}

		found = true;
		delete_gfield_for_lfield(gbl, /*RFR.RDB$FIELD_SOURCE*/
					      jrd_377.jrd_379);
		while (*(*ptr)++ != isc_dyn_end) {
			--(*ptr);
			Firebird::MetaName rel_name(/*RFR.RDB$RELATION_NAME*/
						    jrd_377.jrd_378);
			MetaTmp(/*RFR.RDB$FIELD_SOURCE*/
				jrd_377.jrd_379)
				DYN_execute(gbl, ptr, &rel_name, &tmp, NULL, NULL, NULL);
		}
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_385);
	   }
	}
	if (!DYN_REQUEST(drq_e_lfield)) {
		DYN_REQUEST(drq_e_lfield) = request;
	}

	request = CMP_find_request(tdbb, drq_e_fld_prvs, DYN_REQUESTS);
	id = drq_e_fld_prvs;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PRIV IN RDB$USER_PRIVILEGES WITH
			PRIV.RDB$RELATION_NAME EQ tbl_nm.c_str() AND
			PRIV.RDB$FIELD_NAME EQ col_nm.c_str() AND
            PRIV.RDB$OBJECT_TYPE = obj_relation*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_362, sizeof(jrd_362), true);
	gds__vtov ((const char*) col_nm.c_str(), (char*) jrd_363.jrd_364, 32);
	gds__vtov ((const char*) tbl_nm.c_str(), (char*) jrd_363.jrd_365, 32);
	jrd_363.jrd_366 = obj_relation;
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_363);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_367);
	   if (!jrd_367.jrd_368) break;

        if (!DYN_REQUEST(drq_e_fld_prvs))
			DYN_REQUEST(drq_e_fld_prvs) = request;

		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_369);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_371);
	   }
	}

	if (!DYN_REQUEST(drq_e_fld_prvs)) {
		DYN_REQUEST(drq_e_fld_prvs) = request;
	}

	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);

		switch (id)
		{
		case drq_l_dep_flds:
			DYN_error_punt(true, 53);
			// msg 53: "ERASE RDB$RELATION_FIELDS failed"
		case drq_e_fld_prvs:
			DYN_error_punt(true, 62);
			// msg 62: "ERASE RDB$USER_PRIVILEGES failed"
		default:
			DYN_error_punt(true, 53);
			// msg 53: "ERASE RDB$RELATION_FIELDS failed"
		}
	}

	if (!found) {
		DYN_error_punt(false, 176, SafeArg() << col_nm.c_str() << tbl_nm.c_str());
		// msg 176: "column %s does not exist in table/view %s"
	}
}


void DYN_delete_parameter(Global* gbl,
						  const UCHAR** ptr,
						  Firebird::MetaName* proc_name)
{
   struct {
          TEXT  jrd_333 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_334 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_335;	// gds__utility 
          SSHORT jrd_336;	// gds__null_flag 
          SSHORT jrd_337;	// gds__null_flag 
   } jrd_332;
   struct {
          TEXT  jrd_330 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_331 [32];	// RDB$PROCEDURE_NAME 
   } jrd_329;
   struct {
          SSHORT jrd_347;	// gds__utility 
   } jrd_346;
   struct {
          SSHORT jrd_345;	// gds__utility 
   } jrd_344;
   struct {
          SSHORT jrd_343;	// gds__utility 
   } jrd_342;
   struct {
          TEXT  jrd_340 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_341 [32];	// RDB$FIELD_SOURCE 
   } jrd_339;
   struct {
          SSHORT jrd_361;	// gds__utility 
   } jrd_360;
   struct {
          SSHORT jrd_359;	// gds__utility 
   } jrd_358;
   struct {
          TEXT  jrd_353 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_354 [32];	// RDB$PROCEDURE_NAME 
          TEXT  jrd_355 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_356;	// gds__utility 
          SSHORT jrd_357;	// gds__null_flag 
   } jrd_352;
   struct {
          TEXT  jrd_350 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_351 [32];	// RDB$PROCEDURE_NAME 
   } jrd_349;
/**************************************
 *
 *	D Y N _ d e l e t e _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a stored procedure parameter.
 *
 **************************************/
	Firebird::MetaName name;

	GET_STRING(ptr, name);
	if (**ptr == isc_dyn_prc_name) {
		GET_STRING(ptr, *proc_name);
	}

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_prm, DYN_REQUESTS);
	USHORT id = drq_e_prms;

	bool found = false;

	try {

	jrd_req* old_request = NULL;
	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PP IN RDB$PROCEDURE_PARAMETERS WITH PP.RDB$PROCEDURE_NAME EQ proc_name->c_str()
			AND PP.RDB$PARAMETER_NAME EQ name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_348, sizeof(jrd_348), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_349.jrd_350, 32);
	gds__vtov ((const char*) proc_name->c_str(), (char*) jrd_349.jrd_351, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_349);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 100, (UCHAR*) &jrd_352);
	   if (!jrd_352.jrd_356) break;
		if (!DYN_REQUEST(drq_e_prm))
			DYN_REQUEST(drq_e_prm) = request;
		found = true;

		/* get rid of parameters in rdb$fields */

		if (!/*PP.RDB$FIELD_SOURCE.NULL*/
		     jrd_352.jrd_357) {
			old_request = request;
			const USHORT old_id = id;
			request = CMP_find_request(tdbb, drq_d_gfields, DYN_REQUESTS);
			id = drq_d_gfields;

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				FLD IN RDB$FIELDS
					WITH FLD.RDB$FIELD_NAME EQ PP.RDB$FIELD_SOURCE AND
						 FLD.RDB$FIELD_NAME STARTING WITH IMPLICIT_DOMAIN_PREFIX AND
						 (FLD.RDB$SYSTEM_FLAG EQ 0 OR FLD.RDB$SYSTEM_FLAG MISSING)*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_338, sizeof(jrd_338), true);
			gds__vtov ((const char*) IMPLICIT_DOMAIN_PREFIX, (char*) jrd_339.jrd_340, 32);
			gds__vtov ((const char*) jrd_352.jrd_355, (char*) jrd_339.jrd_341, 32);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_339);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_342);
			   if (!jrd_342.jrd_343) break;

				if (!DYN_REQUEST(drq_d_gfields))
					DYN_REQUEST(drq_d_gfields) = request;

				bool erase = true;

				if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_2)
				{
					jrd_req* request2 = CMP_find_request(tdbb, drq_d_gfields2, DYN_REQUESTS);

					/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE gbl->gbl_transaction)
						PP2 IN RDB$PROCEDURE_PARAMETERS
						WITH PP2.RDB$PROCEDURE_NAME = PP.RDB$PROCEDURE_NAME AND
							 PP2.RDB$PARAMETER_NAME = PP.RDB$PARAMETER_NAME*/
					{
					if (!request2)
					   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_328, sizeof(jrd_328), true);
					gds__vtov ((const char*) jrd_352.jrd_353, (char*) jrd_329.jrd_330, 32);
					gds__vtov ((const char*) jrd_352.jrd_354, (char*) jrd_329.jrd_331, 32);
					EXE_start (tdbb, request2, gbl->gbl_transaction);
					EXE_send (tdbb, request2, 0, 64, (UCHAR*) &jrd_329);
					while (1)
					   {
					   EXE_receive (tdbb, request2, 1, 70, (UCHAR*) &jrd_332);
					   if (!jrd_332.jrd_335) break;

						if (!DYN_REQUEST(drq_d_gfields2))
							DYN_REQUEST(drq_d_gfields2) = request2;

						if (!/*PP2.RDB$RELATION_NAME.NULL*/
						     jrd_332.jrd_337 && !/*PP2.RDB$FIELD_NAME.NULL*/
     jrd_332.jrd_336)
							erase = false;
					/*END_FOR;*/
					   }
					}

					if (!DYN_REQUEST(drq_d_gfields2))
						DYN_REQUEST(drq_d_gfields2) = request2;
				}

				if (erase)
					/*ERASE FLD;*/
					EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_344);
			/*END_FOR;*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_346);
			   }
			}

			if (!DYN_REQUEST(drq_d_gfields))
				DYN_REQUEST(drq_d_gfields) = request;

			request = old_request;
			id = old_id;
		}
		/*ERASE PP;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_358);

	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_360);
	   }
	}
	if (!DYN_REQUEST(drq_e_prm)) {
		DYN_REQUEST(drq_e_prm) = request;
	}
	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		if (id == drq_e_prms) {
			DYN_error_punt(true, 138);
			/* msg 138: "ERASE RDB$PROCEDURE_PARAMETERS failed" */
		}
		else {
			DYN_error_punt(true, 35);
			/* msg 35: "ERASE RDB$FIELDS failed" */
		}
	}

	if (!found) {
		DYN_error_punt(false, 146, SafeArg() << name.c_str() << proc_name->c_str());
		/* msg 146: "Parameter %s in procedure %s not found" */
	}

	if (*(*ptr)++ != isc_dyn_end) {
		DYN_unsupported_verb();
	}
}


void DYN_delete_procedure( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_273;	// gds__utility 
   } jrd_272;
   struct {
          SSHORT jrd_271;	// gds__utility 
   } jrd_270;
   struct {
          SSHORT jrd_269;	// gds__utility 
   } jrd_268;
   struct {
          TEXT  jrd_266 [32];	// RDB$USER 
          SSHORT jrd_267;	// RDB$USER_TYPE 
   } jrd_265;
   struct {
          SSHORT jrd_283;	// gds__utility 
   } jrd_282;
   struct {
          SSHORT jrd_281;	// gds__utility 
   } jrd_280;
   struct {
          SSHORT jrd_279;	// gds__utility 
   } jrd_278;
   struct {
          TEXT  jrd_276 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_277;	// RDB$OBJECT_TYPE 
   } jrd_275;
   struct {
          SSHORT jrd_294;	// gds__utility 
   } jrd_293;
   struct {
          SSHORT jrd_292;	// gds__utility 
   } jrd_291;
   struct {
          TEXT  jrd_288 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_289;	// gds__utility 
          SSHORT jrd_290;	// gds__null_flag 
   } jrd_287;
   struct {
          TEXT  jrd_286 [32];	// RDB$PROCEDURE_NAME 
   } jrd_285;
   struct {
          TEXT  jrd_300 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_301 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_302;	// gds__utility 
          SSHORT jrd_303;	// gds__null_flag 
          SSHORT jrd_304;	// gds__null_flag 
   } jrd_299;
   struct {
          TEXT  jrd_297 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_298 [32];	// RDB$PROCEDURE_NAME 
   } jrd_296;
   struct {
          SSHORT jrd_314;	// gds__utility 
   } jrd_313;
   struct {
          SSHORT jrd_312;	// gds__utility 
   } jrd_311;
   struct {
          SSHORT jrd_310;	// gds__utility 
   } jrd_309;
   struct {
          TEXT  jrd_307 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_308 [32];	// RDB$FIELD_SOURCE 
   } jrd_306;
   struct {
          SSHORT jrd_327;	// gds__utility 
   } jrd_326;
   struct {
          SSHORT jrd_325;	// gds__utility 
   } jrd_324;
   struct {
          TEXT  jrd_319 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_320 [32];	// RDB$PROCEDURE_NAME 
          TEXT  jrd_321 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_322;	// gds__utility 
          SSHORT jrd_323;	// gds__null_flag 
   } jrd_318;
   struct {
          TEXT  jrd_317 [32];	// RDB$PROCEDURE_NAME 
   } jrd_316;
/**************************************
 *
 *	D Y N _ d e l e t e _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a stored procedure.
 *
 **************************************/
	Firebird::MetaName name;

	GET_STRING(ptr, name);
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	tdbb->tdbb_flags |= TDBB_prc_being_dropped;
	if (MET_lookup_procedure(tdbb, name, true) == 0)
	{
		tdbb->tdbb_flags &= ~TDBB_prc_being_dropped;
		DYN_error_punt(false, 140, name.c_str());
		/* msg 140: "Procedure %s not found" */
	}

	tdbb->tdbb_flags &= ~TDBB_prc_being_dropped;

	jrd_req* request = CMP_find_request(tdbb, drq_e_prms, DYN_REQUESTS);
	USHORT id = drq_e_prms;

	try {

    jrd_req* old_request = NULL;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PP IN RDB$PROCEDURE_PARAMETERS WITH PP.RDB$PROCEDURE_NAME EQ name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_315, sizeof(jrd_315), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_316.jrd_317, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_316);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 100, (UCHAR*) &jrd_318);
	   if (!jrd_318.jrd_322) break;

        if (!DYN_REQUEST(drq_e_prms))
            DYN_REQUEST(drq_e_prms) = request;

		/* get rid of parameters in rdb$fields */

		if (!/*PP.RDB$FIELD_SOURCE.NULL*/
		     jrd_318.jrd_323) {
			old_request = request;
			const USHORT old_id = id;
			request = CMP_find_request(tdbb, drq_d_gfields, DYN_REQUESTS);
			id = drq_d_gfields;

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				FLD IN RDB$FIELDS
					WITH FLD.RDB$FIELD_NAME EQ PP.RDB$FIELD_SOURCE AND
						 FLD.RDB$FIELD_NAME STARTING WITH IMPLICIT_DOMAIN_PREFIX*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_305, sizeof(jrd_305), true);
			gds__vtov ((const char*) IMPLICIT_DOMAIN_PREFIX, (char*) jrd_306.jrd_307, 32);
			gds__vtov ((const char*) jrd_318.jrd_321, (char*) jrd_306.jrd_308, 32);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_306);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_309);
			   if (!jrd_309.jrd_310) break;

                if (!DYN_REQUEST(drq_d_gfields))
                    DYN_REQUEST(drq_d_gfields) = request;

				bool erase = true;

				if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_2)
				{
					jrd_req* request2 = NULL;

					/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE gbl->gbl_transaction)
						PP2 IN RDB$PROCEDURE_PARAMETERS
						WITH PP2.RDB$PROCEDURE_NAME = PP.RDB$PROCEDURE_NAME AND
							 PP2.RDB$PARAMETER_NAME = PP.RDB$PARAMETER_NAME*/
					{
					if (!request2)
					   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_295, sizeof(jrd_295), true);
					gds__vtov ((const char*) jrd_318.jrd_319, (char*) jrd_296.jrd_297, 32);
					gds__vtov ((const char*) jrd_318.jrd_320, (char*) jrd_296.jrd_298, 32);
					EXE_start (tdbb, request2, gbl->gbl_transaction);
					EXE_send (tdbb, request2, 0, 64, (UCHAR*) &jrd_296);
					while (1)
					   {
					   EXE_receive (tdbb, request2, 1, 70, (UCHAR*) &jrd_299);
					   if (!jrd_299.jrd_302) break;

						if (!/*PP2.RDB$RELATION_NAME.NULL*/
						     jrd_299.jrd_304 && !/*PP2.RDB$FIELD_NAME.NULL*/
     jrd_299.jrd_303)
							erase = false;
					/*END_FOR;*/
					   }
					}

					CMP_release(tdbb, request2);
				}

				if (erase)
					/*ERASE FLD;*/
					EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_311);
			/*END_FOR;*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_313);
			   }
			}

			if (!DYN_REQUEST(drq_d_gfields))
				DYN_REQUEST(drq_d_gfields) = request;

			request = old_request;
			id = old_id;
		}

		/*ERASE PP;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_324);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_326);
	   }
	}
	if (!DYN_REQUEST(drq_e_prms)) {
		DYN_REQUEST(drq_e_prms) = request;
	}

	request = CMP_find_request(tdbb, drq_e_prcs, DYN_REQUESTS);
	id = drq_e_prcs;

	bool found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		P IN RDB$PROCEDURES WITH P.RDB$PROCEDURE_NAME EQ name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_284, sizeof(jrd_284), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_285.jrd_286, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_285);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_287);
	   if (!jrd_287.jrd_289) break;

        if (!DYN_REQUEST(drq_e_prcs)) {
            DYN_REQUEST(drq_e_prcs) = request;
		}

		/*ERASE P;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_291);

		if (!/*P.RDB$SECURITY_CLASS.NULL*/
		     jrd_287.jrd_290) {
			delete_security_class2(gbl, /*P.RDB$SECURITY_CLASS*/
						    jrd_287.jrd_288);
		}

		found = true;
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_293);
	   }
	}

	if (!DYN_REQUEST(drq_e_prcs)) {
		DYN_REQUEST(drq_e_prcs) = request;
	}

	if (!found) {
		goto dyn_punt_140;
	}

	request = CMP_find_request(tdbb, drq_e_prc_prvs, DYN_REQUESTS);
	id = drq_e_prc_prvs;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PRIV IN RDB$USER_PRIVILEGES WITH PRIV.RDB$RELATION_NAME EQ name.c_str()
			AND PRIV.RDB$OBJECT_TYPE = obj_procedure*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_274, sizeof(jrd_274), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_275.jrd_276, 32);
	jrd_275.jrd_277 = obj_procedure;
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_275);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_278);
	   if (!jrd_278.jrd_279) break;

        if (!DYN_REQUEST(drq_e_prc_prvs))
            DYN_REQUEST(drq_e_prc_prvs) = request;

		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_280);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_282);
	   }
	}

	if (!DYN_REQUEST(drq_e_prc_prvs))
		DYN_REQUEST(drq_e_prc_prvs) = request;

	request = CMP_find_request(tdbb, drq_e_prc_prv, DYN_REQUESTS);
	id = drq_e_prc_prv;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PRIV IN RDB$USER_PRIVILEGES WITH PRIV.RDB$USER EQ name.c_str()
			AND PRIV.RDB$USER_TYPE = obj_procedure*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_264, sizeof(jrd_264), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_265.jrd_266, 32);
	jrd_265.jrd_267 = obj_procedure;
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_265);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_268);
	   if (!jrd_268.jrd_269) break;

        if (!DYN_REQUEST(drq_e_prc_prv))
            DYN_REQUEST(drq_e_prc_prv) = request;

		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_270);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_272);
	   }
	}

	if (!DYN_REQUEST(drq_e_prc_prv))
		DYN_REQUEST(drq_e_prc_prv) = request;

	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);

		switch (id)
		{
		case drq_e_prms:
			DYN_error_punt(true, 138);
			/* msg 138: "ERASE RDB$PROCEDURE_PARAMETERS failed" */
			break;
		case drq_e_prcs:
			DYN_error_punt(true, 139);
			/* msg 139: "ERASE RDB$PROCEDURES failed" */
			break;
		case drq_d_gfields:
			DYN_error_punt(true, 35);
			/* msg 35: "ERASE RDB$FIELDS failed" */
			break;
		default:
			DYN_error_punt(true, 62);
			/* msg 62: "ERASE RDB$USER_PRIVILEGES failed" */
			break;
		}
	}

	if (*(*ptr)++ != isc_dyn_end) {
		DYN_unsupported_verb();
	}

	return;

dyn_punt_140:
	DYN_error_punt(false, 140, name.c_str());
	/* msg 140: "Procedure %s not found" */
}


void DYN_delete_relation( Global* gbl, const UCHAR** ptr, const Firebird::MetaName* relation)
{
   struct {
          SSHORT jrd_159;	// gds__utility 
   } jrd_158;
   struct {
          SSHORT jrd_157;	// gds__utility 
   } jrd_156;
   struct {
          SSHORT jrd_155;	// gds__utility 
   } jrd_154;
   struct {
          TEXT  jrd_152 [32];	// RDB$USER 
          SSHORT jrd_153;	// RDB$USER_TYPE 
   } jrd_151;
   struct {
          SSHORT jrd_169;	// gds__utility 
   } jrd_168;
   struct {
          SSHORT jrd_167;	// gds__utility 
   } jrd_166;
   struct {
          SSHORT jrd_165;	// gds__utility 
   } jrd_164;
   struct {
          TEXT  jrd_162 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_163;	// RDB$OBJECT_TYPE 
   } jrd_161;
   struct {
          SSHORT jrd_179;	// gds__utility 
   } jrd_178;
   struct {
          SSHORT jrd_177;	// gds__utility 
   } jrd_176;
   struct {
          SSHORT jrd_175;	// gds__utility 
   } jrd_174;
   struct {
          TEXT  jrd_172 [32];	// RDB$USER 
          SSHORT jrd_173;	// RDB$USER_TYPE 
   } jrd_171;
   struct {
          SSHORT jrd_189;	// gds__utility 
   } jrd_188;
   struct {
          SSHORT jrd_187;	// gds__utility 
   } jrd_186;
   struct {
          TEXT  jrd_184 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_185;	// gds__utility 
   } jrd_183;
   struct {
          TEXT  jrd_182 [32];	// RDB$RELATION_NAME 
   } jrd_181;
   struct {
          SSHORT jrd_200;	// gds__utility 
   } jrd_199;
   struct {
          SSHORT jrd_198;	// gds__utility 
   } jrd_197;
   struct {
          TEXT  jrd_194 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_195;	// gds__utility 
          SSHORT jrd_196;	// gds__null_flag 
   } jrd_193;
   struct {
          TEXT  jrd_192 [32];	// RDB$RELATION_NAME 
   } jrd_191;
   struct {
          SSHORT jrd_209;	// gds__utility 
   } jrd_208;
   struct {
          SSHORT jrd_207;	// gds__utility 
   } jrd_206;
   struct {
          SSHORT jrd_205;	// gds__utility 
   } jrd_204;
   struct {
          TEXT  jrd_203 [32];	// RDB$VIEW_NAME 
   } jrd_202;
   struct {
          SSHORT jrd_221;	// gds__utility 
   } jrd_220;
   struct {
          SSHORT jrd_219;	// gds__utility 
   } jrd_218;
   struct {
          TEXT  jrd_214 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_215 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_216;	// gds__utility 
          SSHORT jrd_217;	// gds__null_flag 
   } jrd_213;
   struct {
          TEXT  jrd_212 [32];	// RDB$RELATION_NAME 
   } jrd_211;
   struct {
          SSHORT jrd_232;	// gds__utility 
   } jrd_231;
   struct {
          SSHORT jrd_230;	// gds__utility 
   } jrd_229;
   struct {
          SSHORT jrd_228;	// gds__utility 
   } jrd_227;
   struct {
          TEXT  jrd_224 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_225 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_226 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_223;
   struct {
          SSHORT jrd_241;	// gds__utility 
   } jrd_240;
   struct {
          SSHORT jrd_239;	// gds__utility 
   } jrd_238;
   struct {
          SSHORT jrd_237;	// gds__utility 
   } jrd_236;
   struct {
          TEXT  jrd_235 [32];	// RDB$RELATION_NAME 
   } jrd_234;
   struct {
          SSHORT jrd_251;	// gds__utility 
   } jrd_250;
   struct {
          SSHORT jrd_249;	// gds__utility 
   } jrd_248;
   struct {
          TEXT  jrd_246 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_247;	// gds__utility 
   } jrd_245;
   struct {
          TEXT  jrd_244 [32];	// RDB$RELATION_NAME 
   } jrd_243;
   struct {
          SSHORT jrd_263;	// gds__utility 
   } jrd_262;
   struct {
          SSHORT jrd_261;	// gds__utility 
   } jrd_260;
   struct {
          SSHORT jrd_259;	// gds__utility 
   } jrd_258;
   struct {
          TEXT  jrd_254 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_255 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_256 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_257 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_253;
/**************************************
 *
 *	D Y N _ d e l e t e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a relation with all its indices, triggers
 *	and fields.
 *
 **************************************/
	Firebird::MetaName relation_name;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	if (relation)
		relation_name = *relation;
	else
		GET_STRING(ptr, relation_name);

	jrd_rel* rel_drop = MET_lookup_relation(tdbb, relation_name);
	if (rel_drop)
		MET_scan_relation(tdbb, rel_drop);

	jrd_req* req2 = 0;
	jrd_req* request = CMP_find_request(tdbb, drq_e_rel_con2, DYN_REQUESTS);
	USHORT id = drq_e_rel_con2;

	try {

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		CRT IN RDB$RELATION_CONSTRAINTS
			WITH CRT.RDB$RELATION_NAME EQ relation_name.c_str() AND
			(CRT.RDB$CONSTRAINT_TYPE EQ PRIMARY_KEY OR
			 CRT.RDB$CONSTRAINT_TYPE EQ UNIQUE_CNSTRT OR
			 CRT.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY)
		SORTED BY ASCENDING CRT.RDB$CONSTRAINT_TYPE*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_252, sizeof(jrd_252), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_253.jrd_254, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_253.jrd_255, 12);
	gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_253.jrd_256, 12);
	gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_253.jrd_257, 12);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_253);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_258);
	   if (!jrd_258.jrd_259) break;

        if (!DYN_REQUEST(drq_e_rel_con2))
            DYN_REQUEST(drq_e_rel_con2) = request;

		/*ERASE CRT;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_260);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_262);
	   }
	}

	if (!DYN_REQUEST(drq_e_rel_con2))
		DYN_REQUEST(drq_e_rel_con2) = request;

	request = CMP_find_request(tdbb, drq_e_rel_idxs, DYN_REQUESTS);
	id = drq_e_rel_idxs;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		IDX IN RDB$INDICES WITH IDX.RDB$RELATION_NAME EQ relation_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_242, sizeof(jrd_242), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_243.jrd_244, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_243);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_245);
	   if (!jrd_245.jrd_247) break;

        if (!DYN_REQUEST(drq_e_rel_idxs))
            DYN_REQUEST(drq_e_rel_idxs) = request;

		delete_index_segment_records(gbl, /*IDX.RDB$INDEX_NAME*/
						  jrd_245.jrd_246);
		/*ERASE IDX;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_248);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_250);
	   }
	}

	if (!DYN_REQUEST(drq_e_rel_idxs))
		DYN_REQUEST(drq_e_rel_idxs) = request;

	request = CMP_find_request(tdbb, drq_e_trg_msgs2, DYN_REQUESTS);
	id = drq_e_trg_msgs2;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		TM IN RDB$TRIGGER_MESSAGES CROSS
			T IN RDB$TRIGGERS WITH T.RDB$RELATION_NAME EQ relation_name.c_str() AND
			TM.RDB$TRIGGER_NAME EQ T.RDB$TRIGGER_NAME*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_233, sizeof(jrd_233), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_234.jrd_235, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_234);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_236);
	   if (!jrd_236.jrd_237) break;

        if (!DYN_REQUEST(drq_e_trg_msgs2))
            DYN_REQUEST(drq_e_trg_msgs2) = request;

		/*ERASE TM;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_238);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_240);
	   }
	}

	if (!DYN_REQUEST(drq_e_trg_msgs2))
		DYN_REQUEST(drq_e_trg_msgs2) = request;

	// CVC: Moved this block here to avoid SF Bug #1111570.
	request = CMP_find_request(tdbb, drq_e_rel_con3, DYN_REQUESTS);
	id = drq_e_rel_con3;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		CRT IN RDB$RELATION_CONSTRAINTS WITH
			CRT.RDB$RELATION_NAME EQ relation_name.c_str() AND
			(CRT.RDB$CONSTRAINT_TYPE EQ CHECK_CNSTRT OR
			 CRT.RDB$CONSTRAINT_TYPE EQ NOT_NULL_CNSTRT)*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_222, sizeof(jrd_222), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_223.jrd_224, 32);
	gds__vtov ((const char*) NOT_NULL_CNSTRT, (char*) jrd_223.jrd_225, 12);
	gds__vtov ((const char*) CHECK_CNSTRT, (char*) jrd_223.jrd_226, 12);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 56, (UCHAR*) &jrd_223);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_227);
	   if (!jrd_227.jrd_228) break;

		if (!DYN_REQUEST(drq_e_rel_con3))
            DYN_REQUEST(drq_e_rel_con3) = request;

		/*ERASE CRT;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_229);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_231);
	   }
	}

	if (!DYN_REQUEST(drq_e_rel_con3))
		DYN_REQUEST(drq_e_rel_con3) = request;

	request = CMP_find_request(tdbb, drq_e_rel_flds, DYN_REQUESTS);
	id = drq_e_rel_flds;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		RFR IN RDB$RELATION_FIELDS WITH RFR.RDB$RELATION_NAME EQ relation_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_210, sizeof(jrd_210), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_211.jrd_212, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_211);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 68, (UCHAR*) &jrd_213);
	   if (!jrd_213.jrd_216) break;

        if (!DYN_REQUEST(drq_e_rel_flds))
			  DYN_REQUEST(drq_e_rel_flds) = request;

		/*ERASE RFR;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_218);

		if (!/*RFR.RDB$SECURITY_CLASS.NULL*/
		     jrd_213.jrd_217 &&
			!strncmp(/*RFR.RDB$SECURITY_CLASS*/
				 jrd_213.jrd_215, SQL_SECCLASS_PREFIX, SQL_SECCLASS_PREFIX_LEN))
		{
			delete_security_class2(gbl, /*RFR.RDB$SECURITY_CLASS*/
						    jrd_213.jrd_215);
		}

		delete_gfield_for_lfield(gbl, /*RFR.RDB$FIELD_SOURCE*/
					      jrd_213.jrd_214);

	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_220);
	   }
	}

	if (!DYN_REQUEST(drq_e_rel_flds))
		DYN_REQUEST(drq_e_rel_flds) = request;

	request = CMP_find_request(tdbb, drq_e_view_rels, DYN_REQUESTS);
	id = drq_e_view_rels;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		VR IN RDB$VIEW_RELATIONS WITH VR.RDB$VIEW_NAME EQ relation_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_201, sizeof(jrd_201), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_202.jrd_203, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_202);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_204);
	   if (!jrd_204.jrd_205) break;
        if (!DYN_REQUEST(drq_e_view_rels))
            DYN_REQUEST(drq_e_view_rels) = request;

		/*ERASE VR;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_206);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_208);
	   }
	}

	if (!DYN_REQUEST(drq_e_view_rels))
		DYN_REQUEST(drq_e_view_rels) = request;

	request = CMP_find_request(tdbb, drq_e_relation, DYN_REQUESTS);
	id = drq_e_relation;

	bool found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		R IN RDB$RELATIONS WITH R.RDB$RELATION_NAME EQ relation_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_190, sizeof(jrd_190), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_191.jrd_192, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_191);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_193);
	   if (!jrd_193.jrd_195) break;
        if (!DYN_REQUEST(drq_e_relation))
            DYN_REQUEST(drq_e_relation) = request;

		/*ERASE R;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_197);

		if (!/*R.RDB$SECURITY_CLASS.NULL*/
		     jrd_193.jrd_196 &&
			!strncmp(/*R.RDB$SECURITY_CLASS*/
				 jrd_193.jrd_194, SQL_SECCLASS_PREFIX, SQL_SECCLASS_PREFIX_LEN))
		{
			delete_security_class2(gbl, /*R.RDB$SECURITY_CLASS*/
						    jrd_193.jrd_194);
		}

		found = true;
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_199);
	   }
	}

	if (!DYN_REQUEST(drq_e_relation))
		DYN_REQUEST(drq_e_relation) = request;

	if (!found) {
		goto dyn_punt_61;
	}

	// Triggers must be deleted after check constraints

	Firebird::MetaName trigger_name;

	request = CMP_find_request(tdbb, drq_e_trigger2, DYN_REQUESTS);
	id = drq_e_trigger2;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$TRIGGERS WITH X.RDB$RELATION_NAME EQ relation_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_180, sizeof(jrd_180), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_181.jrd_182, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_181);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_183);
	   if (!jrd_183.jrd_185) break;

        if (!DYN_REQUEST(drq_e_trigger2))
            DYN_REQUEST(drq_e_trigger2) = request;

		trigger_name = /*X.RDB$TRIGGER_NAME*/
			       jrd_183.jrd_184;
		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_186);

		req2 = CMP_find_request(tdbb, drq_e_trg_prv, DYN_REQUESTS);
		id = drq_e_trg_prv;

		/*FOR(REQUEST_HANDLE req2 TRANSACTION_HANDLE gbl->gbl_transaction)
			PRIV IN RDB$USER_PRIVILEGES WITH PRIV.RDB$USER EQ trigger_name.c_str()
				AND PRIV.RDB$USER_TYPE = obj_trigger*/
		{
		if (!req2)
		   req2 = CMP_compile2 (tdbb, (UCHAR*) jrd_170, sizeof(jrd_170), true);
		gds__vtov ((const char*) trigger_name.c_str(), (char*) jrd_171.jrd_172, 32);
		jrd_171.jrd_173 = obj_trigger;
		EXE_start (tdbb, req2, gbl->gbl_transaction);
		EXE_send (tdbb, req2, 0, 34, (UCHAR*) &jrd_171);
		while (1)
		   {
		   EXE_receive (tdbb, req2, 1, 2, (UCHAR*) &jrd_174);
		   if (!jrd_174.jrd_175) break;

			if (!DYN_REQUEST(drq_e_trg_prv))
				DYN_REQUEST(drq_e_trg_prv) = req2;

			/*ERASE PRIV;*/
			EXE_send (tdbb, req2, 2, 2, (UCHAR*) &jrd_176);
		/*END_FOR;*/
		   EXE_send (tdbb, req2, 3, 2, (UCHAR*) &jrd_178);
		   }
		}

		if (!DYN_REQUEST(drq_e_trg_prv))
			DYN_REQUEST(drq_e_trg_prv) = req2;

		id = drq_e_trigger2;

	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_188);
	   }
	}

	if (!DYN_REQUEST(drq_e_trigger2))
		DYN_REQUEST(drq_e_trigger2) = request;

	request = CMP_find_request(tdbb, drq_e_usr_prvs, DYN_REQUESTS);
	id = drq_e_usr_prvs;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PRIV IN RDB$USER_PRIVILEGES WITH
			PRIV.RDB$RELATION_NAME EQ relation_name.c_str() AND
            PRIV.RDB$OBJECT_TYPE = obj_relation*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_160, sizeof(jrd_160), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_161.jrd_162, 32);
	jrd_161.jrd_163 = obj_relation;
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_161);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_164);
	   if (!jrd_164.jrd_165) break;

        if (!DYN_REQUEST(drq_e_usr_prvs))
			DYN_REQUEST(drq_e_usr_prvs) = request;

		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_166);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_168);
	   }
	}

	if (!DYN_REQUEST(drq_e_usr_prvs)) {
		DYN_REQUEST(drq_e_usr_prvs) = request;
	}

	request = CMP_find_request(tdbb, drq_e_view_prv, DYN_REQUESTS);
	id = drq_e_view_prv;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PRIV IN RDB$USER_PRIVILEGES WITH
			PRIV.RDB$USER EQ relation_name.c_str() AND
            PRIV.RDB$USER_TYPE = obj_view*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_150, sizeof(jrd_150), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_151.jrd_152, 32);
	jrd_151.jrd_153 = obj_view;
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_151);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_154);
	   if (!jrd_154.jrd_155) break;

        if (!DYN_REQUEST(drq_e_view_prv))
			DYN_REQUEST(drq_e_view_prv) = request;

		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_156);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_158);
	   }
	}

	if (!DYN_REQUEST(drq_e_view_prv)) {
		DYN_REQUEST(drq_e_view_prv) = request;
	}
	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_rundown_request(req2, -1);

		// lookup error # from id
		// msg  57: "ERASE RDB$INDICES failed"
		// msg  58: "ERASE RDB$RELATION_FIELDS failed"
		// msg  59: "ERASE RDB$VIEW_RELATIONS failed"
		// msg  60: "ERASE RDB$RELATIONS failed"
		// msg  62: "ERASE RDB$USER_PRIVILEGES failed"
		// msg  65: "ERASE RDB$TRIGGER_MESSAGES failed"
		// msg  66: "ERASE RDB$TRIGGERS failed"
		// msg  74: "ERASE RDB$SECURITY_CLASSES failed"
		// msg 129: "ERASE RDB$RELATION_CONSTRAINTS failed"
		USHORT number;
		switch (id)
		{
			case drq_e_rel_con2:
				number = 129;
				break;
			case drq_e_rel_idxs:
				number = 57;
				break;
			case drq_e_trg_msgs2:
				number = 65;
				break;
			case drq_e_trigger2:
				number = 66;
				break;
			case drq_e_rel_flds:
				number = 58;
				break;
			case drq_e_view_rels:
				number = 59;
				break;
			case drq_e_relation:
				number = 60;
				break;
			//case drq_e_sec_class:
			//	number = 74;
			//	break;
			default:
				number = 62;
				break;
		}

		DYN_error_punt(true, number);
	}

	while (*(*ptr)++ != isc_dyn_end)
	{
		--(*ptr);
		DYN_execute(gbl, ptr, &relation_name, NULL, NULL, NULL, NULL);
	}

	return;

dyn_punt_61:
	DYN_error_punt(false, 61);
	/* msg 61: "Relation not found" */
}


void DYN_delete_role( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_137;	// gds__utility 
   } jrd_136;
   struct {
          SSHORT jrd_135;	// gds__utility 
   } jrd_134;
   struct {
          SSHORT jrd_133;	// gds__utility 
   } jrd_132;
   struct {
          TEXT  jrd_128 [32];	// RDB$USER 
          TEXT  jrd_129 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_130;	// RDB$USER_TYPE 
          SSHORT jrd_131;	// RDB$OBJECT_TYPE 
   } jrd_127;
   struct {
          SSHORT jrd_149;	// gds__utility 
   } jrd_148;
   struct {
          SSHORT jrd_147;	// gds__utility 
   } jrd_146;
   struct {
          TEXT  jrd_142 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_143;	// gds__utility 
          SSHORT jrd_144;	// gds__null_flag 
          SSHORT jrd_145;	// RDB$SYSTEM_FLAG 
   } jrd_141;
   struct {
          TEXT  jrd_140 [32];	// RDB$ROLE_NAME 
   } jrd_139;
/**************************************
 *
 *	D Y N _ d e l e t e _ r o l e
 *
 **************************************
 *
 * Functional description
 *
 *	Execute a dynamic ddl statement that deletes a role with all its
 *      members of the role.
 *
 **************************************/
	int id = -1;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	const USHORT major_version = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	if (ENCODE_ODS(major_version, minor_original) < ODS_9_0) {
		DYN_error(false, 196);
		ERR_punt();
		return;	// never reached
	}

	jrd_req* request = NULL;
	enum DelRoleResult {DEL_R_OK, DEL_R_NOT_FOUND, DEL_R_FAIL_OWNER, DEL_R_FAIL_SYSTEM};
	DelRoleResult del_role_result = DEL_R_NOT_FOUND;

	Firebird::MetaName user(tdbb->getAttachment()->att_user->usr_user_name);
	user.upper7();
	Firebird::MetaName role_name, role_owner;

	try {

		GET_STRING(ptr, role_name);

		request = CMP_find_request(tdbb, drq_drop_role, DYN_REQUESTS);
		id = drq_drop_role;

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			XX IN RDB$ROLES WITH
				XX.RDB$ROLE_NAME EQ role_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_138, sizeof(jrd_138), true);
		gds__vtov ((const char*) role_name.c_str(), (char*) jrd_139.jrd_140, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_139);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 38, (UCHAR*) &jrd_141);
		   if (!jrd_141.jrd_143) break;

			if (!DYN_REQUEST(drq_drop_role))
				DYN_REQUEST(drq_drop_role) = request;

			role_owner = /*XX.RDB$OWNER_NAME*/
				     jrd_141.jrd_142;

			// only owner of SQL role or USR_locksmith could drop SQL role

			if (tdbb->getAttachment()->locksmith() || role_owner == user)
			{
				if (/*XX.RDB$SYSTEM_FLAG.NULL*/
				    jrd_141.jrd_144 || /*XX.RDB$SYSTEM_FLAG*/
    jrd_141.jrd_145 == 0)
				{
					/*ERASE XX;*/
					EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_146);
					del_role_result = DEL_R_OK;
				}
				else
				{
					del_role_result = DEL_R_FAIL_SYSTEM;
				}
			}
			else
			{
				del_role_result = DEL_R_FAIL_OWNER;
			}

		/*END_FOR;*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_148);
		   }
		}

		if (!DYN_REQUEST(drq_drop_role)) {
			DYN_REQUEST(drq_drop_role) = request;
		}

		if (del_role_result == DEL_R_OK)
		{
			request = CMP_find_request(tdbb, drq_del_role_1, DYN_REQUESTS);
			id = drq_del_role_1;

			/* The first OR clause Finds all members of the role
			   The 2nd OR clause  Finds all privileges granted to the role */

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				PRIV IN RDB$USER_PRIVILEGES WITH
					(PRIV.RDB$RELATION_NAME EQ role_name.c_str() AND
					 PRIV.RDB$OBJECT_TYPE = obj_sql_role)
					OR (PRIV.RDB$USER EQ role_name.c_str() AND
					   PRIV.RDB$USER_TYPE = obj_sql_role)*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_126, sizeof(jrd_126), true);
			gds__vtov ((const char*) role_name.c_str(), (char*) jrd_127.jrd_128, 32);
			gds__vtov ((const char*) role_name.c_str(), (char*) jrd_127.jrd_129, 32);
			jrd_127.jrd_130 = obj_sql_role;
			jrd_127.jrd_131 = obj_sql_role;
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_127);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_132);
			   if (!jrd_132.jrd_133) break;

				if (!DYN_REQUEST(drq_del_role_1)) {
					DYN_REQUEST(drq_del_role_1) = request;
				}

				/*ERASE PRIV;*/
				EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_134);

			/*END_FOR;*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_136);
			   }
			}

			if (!DYN_REQUEST(drq_del_role_1)) {
				DYN_REQUEST(drq_del_role_1) = request;
			}
		}

	}	// try
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		if (id == drq_drop_role)
		{
			DYN_error_punt(true, 5, "ERASE RDB$ROLES failed");
			// hardcoded (non-translatable) msg: "ERASE RDB$ROLES failed"
		}
		else
		{
			DYN_error_punt(true, 62);
			// msg 62: "ERASE RDB$USER_PRIVILEGES failed"
		}
	}

	switch (del_role_result)
	{
	case DEL_R_NOT_FOUND:
		DYN_error_punt(false, 155, role_name.c_str());
		// msg 155: "Role %s not found"
	case DEL_R_FAIL_OWNER:
		DYN_error_punt(false, 191, SafeArg() << user.c_str() << role_name.c_str());
		// msg 191: @1 is not the owner of SQL role @2
		break;
	case DEL_R_FAIL_SYSTEM:
		DYN_error_punt(false, 284, SafeArg() << role_name.c_str());
		// msg 284: cannot drop system SQL role @1
		break;
	}
}


void DYN_delete_security_class( Global* gbl, const UCHAR** ptr)
{
/**************************************
 *
 *	D Y N _ d e l e t e _ s e c u r i t y _ c l a s s
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a security class.
 *
 **************************************/
	Firebird::MetaName security_class;

	GET_STRING(ptr, security_class);

	if (!delete_security_class2(gbl, security_class))
	{
		DYN_error_punt(false, 75);
		/* msg 75: "Security class not found" */
	}

	while (*(*ptr)++ != isc_dyn_end) {
		--(*ptr);
		DYN_execute(gbl, ptr, NULL, NULL, NULL, NULL, NULL);
	}
}


void DYN_delete_shadow( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_125;	// gds__utility 
   } jrd_124;
   struct {
          SSHORT jrd_123;	// gds__utility 
   } jrd_122;
   struct {
          SSHORT jrd_121;	// gds__utility 
   } jrd_120;
   struct {
          SSHORT jrd_119;	// RDB$SHADOW_NUMBER 
   } jrd_118;
/**************************************
 *
 *	D Y N _ d e l e t e _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	Delete a shadow.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	if (!tdbb->getAttachment()->locksmith())
	{
		ERR_post(Arg::Gds(isc_adm_task_denied));
	}

	jrd_req* request = CMP_find_request(tdbb, drq_e_shadow, DYN_REQUESTS);

	try {

	const int shadow_number = DYN_get_number(ptr);
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		FIL IN RDB$FILES WITH FIL.RDB$SHADOW_NUMBER EQ shadow_number*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_117, sizeof(jrd_117), true);
	jrd_118.jrd_119 = shadow_number;
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_118);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_120);
	   if (!jrd_120.jrd_121) break;

        if (!DYN_REQUEST(drq_e_shadow))
            DYN_REQUEST(drq_e_shadow) = request;

		/*ERASE FIL;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_122);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_124);
	   }
	}

	if (!DYN_REQUEST(drq_e_shadow))
		DYN_REQUEST(drq_e_shadow) = request;

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 63);
		/* msg 63: "ERASE RDB$FILES failed" */
	}

	if (*(*ptr)++ != isc_dyn_end) {
		DYN_unsupported_verb();
	}
}


void DYN_delete_trigger( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_79;	// gds__utility 
   } jrd_78;
   struct {
          SSHORT jrd_77;	// RDB$UPDATE_FLAG 
   } jrd_76;
   struct {
          SSHORT jrd_74;	// gds__utility 
          SSHORT jrd_75;	// RDB$UPDATE_FLAG 
   } jrd_73;
   struct {
          TEXT  jrd_72 [32];	// RDB$RELATION_NAME 
   } jrd_71;
   struct {
          SSHORT jrd_84;	// gds__utility 
   } jrd_83;
   struct {
          TEXT  jrd_82 [32];	// RDB$VIEW_NAME 
   } jrd_81;
   struct {
          SSHORT jrd_94;	// gds__utility 
   } jrd_93;
   struct {
          SSHORT jrd_92;	// gds__utility 
   } jrd_91;
   struct {
          SSHORT jrd_90;	// gds__utility 
   } jrd_89;
   struct {
          TEXT  jrd_87 [32];	// RDB$USER 
          SSHORT jrd_88;	// RDB$USER_TYPE 
   } jrd_86;
   struct {
          SSHORT jrd_105;	// gds__utility 
   } jrd_104;
   struct {
          SSHORT jrd_103;	// gds__utility 
   } jrd_102;
   struct {
          TEXT  jrd_99 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_100;	// gds__utility 
          SSHORT jrd_101;	// gds__null_flag 
   } jrd_98;
   struct {
          TEXT  jrd_97 [32];	// RDB$TRIGGER_NAME 
   } jrd_96;
   struct {
          SSHORT jrd_116;	// gds__utility 
   } jrd_115;
   struct {
          SSHORT jrd_114;	// gds__utility 
   } jrd_113;
   struct {
          TEXT  jrd_110 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_111;	// gds__utility 
          SSHORT jrd_112;	// gds__null_flag 
   } jrd_109;
   struct {
          TEXT  jrd_108 [32];	// RDB$TRIGGER_NAME 
   } jrd_107;
/**************************************
 *
 *	D Y N _ d e l e t e _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a trigger.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_trg_msgs, DYN_REQUESTS);
	USHORT id = drq_e_trg_msgs;

	Firebird::MetaName t;
	GET_STRING(ptr, t);

	try {
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		TM IN RDB$TRIGGER_MESSAGES
		CROSS TR IN RDB$TRIGGERS
		WITH TM.RDB$TRIGGER_NAME EQ t.c_str() AND
			 TR.RDB$TRIGGER_NAME EQ TM.RDB$TRIGGER_NAME*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_106, sizeof(jrd_106), true);
	gds__vtov ((const char*) t.c_str(), (char*) jrd_107.jrd_108, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_107);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_109);
	   if (!jrd_109.jrd_111) break;

        if (!DYN_REQUEST(drq_e_trg_msgs))
            DYN_REQUEST(drq_e_trg_msgs) = request;

		if (/*TR.RDB$RELATION_NAME.NULL*/
		    jrd_109.jrd_112 && !tdbb->getAttachment()->locksmith())
			ERR_post(Arg::Gds(isc_adm_task_denied));

		/*ERASE TM;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_113);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_115);
	   }
	}

	if (!DYN_REQUEST(drq_e_trg_msgs))
		DYN_REQUEST(drq_e_trg_msgs) = request;

	request = CMP_find_request(tdbb, drq_e_trigger, DYN_REQUESTS);
	id = drq_e_trigger;

	Firebird::MetaName r;

	bool found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$TRIGGERS WITH X.RDB$TRIGGER_NAME EQ t.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_95, sizeof(jrd_95), true);
	gds__vtov ((const char*) t.c_str(), (char*) jrd_96.jrd_97, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_96);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_98);
	   if (!jrd_98.jrd_100) break;

        if (!DYN_REQUEST(drq_e_trigger))
            DYN_REQUEST(drq_e_trigger) = request;

		if (/*X.RDB$RELATION_NAME.NULL*/
		    jrd_98.jrd_101 && !tdbb->getAttachment()->locksmith())
			ERR_post(Arg::Gds(isc_adm_task_denied));

		r = /*X.RDB$RELATION_NAME*/
		    jrd_98.jrd_99;
		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_102);
		found = true;
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_104);
	   }
	}

	if (!DYN_REQUEST(drq_e_trigger))
		DYN_REQUEST(drq_e_trigger) = request;

	if (!found) {
		goto dyn_punt_147;
	}

	request = CMP_find_request(tdbb, drq_e_trg_prv, DYN_REQUESTS);
	id = drq_e_trg_prv;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PRIV IN RDB$USER_PRIVILEGES WITH PRIV.RDB$USER EQ t.c_str()
			AND PRIV.RDB$USER_TYPE = obj_trigger*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_85, sizeof(jrd_85), true);
	gds__vtov ((const char*) t.c_str(), (char*) jrd_86.jrd_87, 32);
	jrd_86.jrd_88 = obj_trigger;
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_86);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_89);
	   if (!jrd_89.jrd_90) break;

        if (!DYN_REQUEST(drq_e_trg_prv))
            DYN_REQUEST(drq_e_trg_prv) = request;

		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_91);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_93);
	   }
	}

	if (!DYN_REQUEST(drq_e_trg_prv))
		DYN_REQUEST(drq_e_trg_prv) = request;

/* clear the update flags on the fields if this is the last remaining
   trigger that changes a view */

	request = CMP_find_request(tdbb, drq_l_view_rel2, DYN_REQUESTS);
	id = drq_l_view_rel2;

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		FIRST 1 V IN RDB$VIEW_RELATIONS
			CROSS F IN RDB$RELATION_FIELDS CROSS T IN RDB$TRIGGERS
			WITH V.RDB$VIEW_NAME EQ r.c_str() AND
			F.RDB$RELATION_NAME EQ V.RDB$VIEW_NAME AND
			F.RDB$RELATION_NAME EQ T.RDB$RELATION_NAME*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_80, sizeof(jrd_80), true);
	gds__vtov ((const char*) r.c_str(), (char*) jrd_81.jrd_82, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_81);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_83);
	   if (!jrd_83.jrd_84) break;

        if (!DYN_REQUEST(drq_l_view_rel2))
            DYN_REQUEST(drq_l_view_rel2) = request;

		found = true;
	/*END_FOR;*/
	   }
	}

	if (!DYN_REQUEST(drq_l_view_rel2))
		DYN_REQUEST(drq_l_view_rel2) = request;

	if (!found) {
		request = CMP_find_request(tdbb, drq_m_rel_flds, DYN_REQUESTS);
		id = drq_m_rel_flds;

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			F IN RDB$RELATION_FIELDS WITH F.RDB$RELATION_NAME EQ r.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_70, sizeof(jrd_70), true);
		gds__vtov ((const char*) r.c_str(), (char*) jrd_71.jrd_72, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_71);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_73);
		   if (!jrd_73.jrd_74) break;

            if (!DYN_REQUEST(drq_m_rel_flds))
                DYN_REQUEST(drq_m_rel_flds) = request;

			/*MODIFY F USING*/
			{
			
				/*F.RDB$UPDATE_FLAG*/
				jrd_73.jrd_75 = FALSE;
			/*END_MODIFY;*/
			jrd_76.jrd_77 = jrd_73.jrd_75;
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_76);
			}
		/*END_FOR;*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_78);
		   }
		}

		if (!DYN_REQUEST(drq_m_rel_flds))
			DYN_REQUEST(drq_m_rel_flds) = request;
	}

	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		switch (id)
		{
		case drq_e_trg_msgs:
			DYN_error_punt(true, 65);
			/* msg 65: "ERASE RDB$TRIGGER_MESSAGES failed" */
			break;
		case drq_e_trigger:
			DYN_error_punt(true, 66);
			/* msg 66: "ERASE RDB$TRIGGERS failed" */
			break;
		case drq_e_trg_prv:
			DYN_error_punt(true, 62);
			/* msg 62: "ERASE RDB$USER_PRIVILEGES failed" */
			break;
		default:
			DYN_error_punt(true, 68);
			/* msg 68: "MODIFY RDB$VIEW_RELATIONS failed" */
			break;
		}
	}

	if (*(*ptr)++ != isc_dyn_end) {
		DYN_unsupported_verb();
	}

	return;

dyn_punt_147:
	DYN_error_punt(false, 147, t.c_str());
	/* msg 147: "Trigger %s not found" */
}


void DYN_delete_trigger_msg( Global* gbl, const UCHAR** ptr, Firebird::MetaName* trigger_name)
{
   struct {
          SSHORT jrd_69;	// gds__utility 
   } jrd_68;
   struct {
          SSHORT jrd_67;	// gds__utility 
   } jrd_66;
   struct {
          SSHORT jrd_65;	// gds__utility 
   } jrd_64;
   struct {
          TEXT  jrd_62 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_63;	// RDB$MESSAGE_NUMBER 
   } jrd_61;
/**************************************
 *
 *	D Y N _ d e l e t e _ t r i g g e r _ m s g
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes an trigger message.
 *
 **************************************/
	Firebird::MetaName t;

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	const int number = DYN_get_number(ptr);
	if (trigger_name)
		t = *trigger_name;
	else if (*(*ptr)++ == isc_dyn_trg_name)
		GET_STRING(ptr, t);
	else
	{
		DYN_error_punt(false, 70);
		// msg 70: "TRIGGER NAME expected"
	}

	jrd_req* request = CMP_find_request(tdbb, drq_e_trg_msg, DYN_REQUESTS);

	bool found = false;

	try {

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$TRIGGER_MESSAGES
			WITH X.RDB$TRIGGER_NAME EQ t.c_str() AND X.RDB$MESSAGE_NUMBER EQ number*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_60, sizeof(jrd_60), true);
	gds__vtov ((const char*) t.c_str(), (char*) jrd_61.jrd_62, 32);
	jrd_61.jrd_63 = number;
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_61);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_64);
	   if (!jrd_64.jrd_65) break;

        if (!DYN_REQUEST(drq_e_trg_msg))
            DYN_REQUEST(drq_e_trg_msg) = request;

		found = true;
		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_66);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_68);
	   }
	}

	if (!DYN_REQUEST(drq_e_trg_msg))
		DYN_REQUEST(drq_e_trg_msg) = request;

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 71);
		/* msg 71: "ERASE TRIGGER MESSAGE failed" */
	}

	if (!found)
	{
		DYN_error_punt(false, 72);
		/* msg 72: "Trigger Message not found" */
	}

	if (*(*ptr)++ != isc_dyn_end) {
		DYN_unsupported_verb();
	}
}


static bool delete_constraint_records(Global*	gbl,
									  const Firebird::MetaName&	constraint_name,
									  const Firebird::MetaName&	relation_name)
{
   struct {
          SSHORT jrd_59;	// gds__utility 
   } jrd_58;
   struct {
          SSHORT jrd_57;	// gds__utility 
   } jrd_56;
   struct {
          SSHORT jrd_55;	// gds__utility 
   } jrd_54;
   struct {
          TEXT  jrd_52 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_53 [32];	// RDB$CONSTRAINT_NAME 
   } jrd_51;
/**************************************
 *
 *	d e l e t e _ c o n s t r a i n t _ r e c o r d s
 *
 **************************************
 *
 * Functional description
 *	Delete a record from RDB$RELATION_CONSTRAINTS
 *	based on a constraint name.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_rel_con, DYN_REQUESTS);

	bool found = false;

	try {

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		RC IN RDB$RELATION_CONSTRAINTS
			WITH RC.RDB$CONSTRAINT_NAME EQ constraint_name.c_str() AND
			RC.RDB$RELATION_NAME EQ relation_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_50, sizeof(jrd_50), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_51.jrd_52, 32);
	gds__vtov ((const char*) constraint_name.c_str(), (char*) jrd_51.jrd_53, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_51);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_54);
	   if (!jrd_54.jrd_55) break;

        if (!DYN_REQUEST(drq_e_rel_con))
            DYN_REQUEST(drq_e_rel_con) = request;

		found = true;
		/*ERASE RC;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_56);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_58);
	   }
	}

	if (!DYN_REQUEST(drq_e_rel_con))
		DYN_REQUEST(drq_e_rel_con) = request;

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 129);
		/* msg 129: "ERASE RDB$RELATION_CONSTRAINTS failed" */
	}

	return found;
}


static bool delete_dimension_records(Global* gbl,
									 const Firebird::MetaName& field_name)
{
   struct {
          SSHORT jrd_49;	// gds__utility 
   } jrd_48;
   struct {
          SSHORT jrd_47;	// gds__utility 
   } jrd_46;
   struct {
          SSHORT jrd_45;	// gds__utility 
   } jrd_44;
   struct {
          TEXT  jrd_43 [32];	// RDB$FIELD_NAME 
   } jrd_42;
/**************************************
 *
 *	d e l e t e _ d i m e n s i o n s _ r e c o r d s
 *
 **************************************
 *
 * Functional description
 *	Delete the records in RDB$FIELD_DIMENSIONS
 *	pertaining to a field.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_dims, DYN_REQUESTS);

	bool found = false;

	try {

		found = false;
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			X IN RDB$FIELD_DIMENSIONS WITH X.RDB$FIELD_NAME EQ field_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_41, sizeof(jrd_41), true);
		gds__vtov ((const char*) field_name.c_str(), (char*) jrd_42.jrd_43, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_42);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_44);
		   if (!jrd_44.jrd_45) break;

			if (!DYN_REQUEST(drq_e_dims))
				DYN_REQUEST(drq_e_dims) = request;

			found = true;
			/*ERASE X;*/
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_46);
		/*END_FOR;*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_48);
		   }
		}

		if (!DYN_REQUEST(drq_e_dims))
			DYN_REQUEST(drq_e_dims) = request;

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 35);
		/* msg 35: "ERASE RDB$FIELDS failed" */
	}

	return found;
}


static void delete_f_key_constraint(thread_db*	tdbb,
									Global*		gbl,
									const Firebird::MetaName&	tbl_nm,
									const Firebird::MetaName&	,
									const Firebird::MetaName&	constraint_nm,
									const Firebird::MetaName&	index_name)
{
   struct {
          SSHORT jrd_40;	// gds__utility 
   } jrd_39;
   struct {
          SSHORT jrd_38;	// gds__utility 
   } jrd_37;
   struct {
          SSHORT jrd_36;	// gds__utility 
   } jrd_35;
   struct {
          TEXT  jrd_31 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_32 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_33 [32];	// RDB$CONSTRAINT_NAME 
          TEXT  jrd_34 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_30;
/**************************************
 *
 *	d e l e t e _ f _ k e y _ c o n s t r a i n t
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a record from RDB$RELATION_CONSTRAINTS based on a constraint_nm
 *
 *      On deleting from RDB$RELATION_CONSTRAINTS, 2 system triggers fire:
 *
 *      (A) pre delete trigger: pre_delete_constraint, will:
 *
 *        1. delete a record first from RDB$REF_CONSTRAINTS where
 *           RDB$REF_CONSTRAINTS.RDB$CONSTRAINT_NAME =
 *                               RDB$RELATION_CONSTRAINTS.RDB$CONSTRAINT_NAME
 *
 *      (B) post delete trigger: post_delete_constraint will:
 *
 *        1. also delete a record from RDB$INDICES where
 *           RDB$INDICES.RDB$INDEX_NAME =
 *                               RDB$RELATION_CONSTRAINTS.RDB$INDEX_NAME
 *
 *        2. also delete a record from RDB$INDEX_SEGMENTS where
 *           RDB$INDEX_SEGMENTS.RDB$INDEX_NAME =
 *                               RDB$RELATION_CONSTRAINTS.RDB$INDEX_NAME
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_rel_const, DYN_REQUESTS);

	try {

		bool found = false;
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			RC IN RDB$RELATION_CONSTRAINTS
				WITH RC.RDB$CONSTRAINT_NAME EQ constraint_nm.c_str()
				AND RC.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY
				AND RC.RDB$RELATION_NAME EQ tbl_nm.c_str()
				AND RC.RDB$INDEX_NAME EQ index_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_29, sizeof(jrd_29), true);
		gds__vtov ((const char*) index_name.c_str(), (char*) jrd_30.jrd_31, 32);
		gds__vtov ((const char*) tbl_nm.c_str(), (char*) jrd_30.jrd_32, 32);
		gds__vtov ((const char*) constraint_nm.c_str(), (char*) jrd_30.jrd_33, 32);
		gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_30.jrd_34, 12);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 108, (UCHAR*) &jrd_30);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_35);
		   if (!jrd_35.jrd_36) break;

			if (!DYN_REQUEST(drq_e_rel_const))
				DYN_REQUEST(drq_e_rel_const) = request;

			found = true;
			/*ERASE RC;*/
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_37);
		/*END_FOR;*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_39);
		   }
		}

		if (!DYN_REQUEST(drq_e_rel_const))
			DYN_REQUEST(drq_e_rel_const) = request;

		if (!found)
		{
			DYN_error_punt(false, 130, constraint_nm.c_str());
			/* msg 130: "CONSTRAINT %s does not exist." */
		}
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 129);
		/* msg 49: "ERASE RDB$RELATION_CONSTRAINTS failed" */
	}
}


static void delete_gfield_for_lfield( Global* gbl, const Firebird::MetaName& lfield_name)
{
   struct {
          SSHORT jrd_28;	// gds__utility 
   } jrd_27;
   struct {
          SSHORT jrd_26;	// gds__utility 
   } jrd_25;
   struct {
          TEXT  jrd_23 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_24;	// gds__utility 
   } jrd_22;
   struct {
          TEXT  jrd_20 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_21 [32];	// RDB$FIELD_NAME 
   } jrd_19;
/**************************************
 *
 *	d e l e t e _ g f i e l d _ f o r _ l f i e l d
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement that
 *	deletes a global field for a given local field.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();
	jrd_req* request = CMP_find_request(tdbb, drq_e_l_gfld, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		FLD IN RDB$FIELDS
			WITH FLD.RDB$FIELD_NAME EQ lfield_name.c_str()
			AND FLD.RDB$VALIDATION_SOURCE MISSING AND
			FLD.RDB$NULL_FLAG MISSING AND
			FLD.RDB$DEFAULT_SOURCE MISSING AND
			FLD.RDB$FIELD_NAME STARTING WITH IMPLICIT_DOMAIN_PREFIX AND
			(NOT ANY RFR IN RDB$RELATION_FIELDS WITH
				RFR.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME) AND
			(NOT ANY PRC IN RDB$PROCEDURE_PARAMETERS WITH
				PRC.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME)*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_18, sizeof(jrd_18), true);
	gds__vtov ((const char*) IMPLICIT_DOMAIN_PREFIX, (char*) jrd_19.jrd_20, 32);
	gds__vtov ((const char*) lfield_name.c_str(), (char*) jrd_19.jrd_21, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_19);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_22);
	   if (!jrd_22.jrd_24) break;

        if (!DYN_REQUEST(drq_e_l_gfld))
            DYN_REQUEST(drq_e_l_gfld) = request;

		delete_dimension_records(gbl, /*FLD.RDB$FIELD_NAME*/
					      jrd_22.jrd_23);
		/*ERASE FLD;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_25);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_27);
	   }
	}


	if (!DYN_REQUEST(drq_e_l_gfld))
		DYN_REQUEST(drq_e_l_gfld) = request;

}


static bool delete_index_segment_records(Global* gbl,
										 const Firebird::MetaName& index_name)
{
   struct {
          SSHORT jrd_17;	// gds__utility 
   } jrd_16;
   struct {
          SSHORT jrd_15;	// gds__utility 
   } jrd_14;
   struct {
          SSHORT jrd_13;	// gds__utility 
   } jrd_12;
   struct {
          TEXT  jrd_11 [32];	// RDB$INDEX_NAME 
   } jrd_10;
/**************************************
 *
 *	d e l e t e _ i n d e x _ s e g m e n t _ r e c o r d s
 *
 **************************************
 *
 * Functional description
 *	Delete the records in RDB$INDEX_SEGMENTS
 *	pertaining to an index.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_idx_segs, DYN_REQUESTS);

	bool found = false;

	try {

		found = false;
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			I_S IN RDB$INDEX_SEGMENTS WITH I_S.RDB$INDEX_NAME EQ index_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_9, sizeof(jrd_9), true);
		gds__vtov ((const char*) index_name.c_str(), (char*) jrd_10.jrd_11, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_10);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_12);
		   if (!jrd_12.jrd_13) break;

			if (!DYN_REQUEST(drq_e_idx_segs))
				DYN_REQUEST(drq_e_idx_segs) = request;

			found = true;
			/*ERASE I_S;*/
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_14);
		/*END_FOR;*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_16);
		   }
		}

		if (!DYN_REQUEST(drq_e_idx_segs))
			DYN_REQUEST(drq_e_idx_segs) = request;
	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 49);
		/* msg 49: "ERASE RDB$INDEX_SEGMENTS failed" */
	}

	return found;
}


static bool delete_security_class2( Global* gbl, const Firebird::MetaName& security_class)
{
   struct {
          SSHORT jrd_8;	// gds__utility 
   } jrd_7;
   struct {
          SSHORT jrd_6;	// gds__utility 
   } jrd_5;
   struct {
          SSHORT jrd_4;	// gds__utility 
   } jrd_3;
   struct {
          TEXT  jrd_2 [32];	// RDB$SECURITY_CLASS 
   } jrd_1;
/**************************************
 *
 *	d e l e t e _ s e c u r i t y _ c l a s s 2
 *
 **************************************
 *
 * Functional description
 *	Utility routine for delete_security_class(),
 *	which takes a string as input.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_e_class, DYN_REQUESTS);

	bool found = false;

	try {

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		SC IN RDB$SECURITY_CLASSES
			WITH SC.RDB$SECURITY_CLASS EQ security_class.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
	gds__vtov ((const char*) security_class.c_str(), (char*) jrd_1.jrd_2, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_3);
	   if (!jrd_3.jrd_4) break;

        if (!DYN_REQUEST(drq_e_class))
            DYN_REQUEST(drq_e_class) = request;

		found = true;
		/*ERASE SC;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_5);

	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_7);
	   }
	}

	if (!DYN_REQUEST(drq_e_class)) {
		DYN_REQUEST(drq_e_class) = request;
	}

	}
	catch (const Firebird::Exception& ex) {
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 74);
		/* msg 74: "ERASE RDB$SECURITY_CLASSES failed" */
	}

	return found;
}

