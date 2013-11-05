/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		met.epp
 *	DESCRIPTION:	Meta data handler
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
 * 2001.6.25 Claudio Valderrama: Finish MET_lookup_generator_id() by
 *	assigning it a number in the compiled requests table.
 *
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 * 2001.10.03 Claudio Valderrama: MET_relation_owns_trigger() determines if
 *   there's a row in rdb$triggers with the given relation's and trigger's names.
 * 2001.10.04 Claudio Valderrama: MET_relation_default_class() determines if the
 *   given security class name is the default security class for a relation.
 *   Modify MET_lookup_field() so it can verify the field's security class, too.
 * 2002-02-24 Sean Leyne - Code Cleanup of old Win 3.1 port (WINDOWS_ONLY)
 * 2002-09-16 Nickolay Samofatov - Deferred trigger compilation changes
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2004.01.16 Vlad Horsun: added support for default parameters
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/common.h"
#include <stdarg.h>

#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/irq.h"
#include "../jrd/tra.h"
#include "../jrd/lck.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/req.h"
#include "../jrd/exe.h"
#include "../jrd/scl.h"
#include "../jrd/blb.h"
#include "../jrd/met.h"
#include "../jrd/os/pio.h"
#include "../jrd/sdw.h"
#include "../jrd/flags.h"
#include "../jrd/lls.h"
#include "../jrd/intl.h"
#include "../jrd/align.h"
#include "../jrd/flu.h"
#include "../jrd/blob_filter.h"
#include "../intl/charsets.h"
#include "../jrd/gdsassert.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dsc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/flu_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/ini_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/pcmet_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/sdw_proto.h"
#include "../jrd/thread_proto.h"
#include "../common/utils_proto.h"

#include "../../src/jrd/DebugInterface.h"
#include "../common/classes/MsgPrint.h"


#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

/* Pick up relation ids */
#include "../jrd/ini.h"

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [113] =
   {	// blr string 

4,2,4,1,3,0,41,0,0,12,0,41,0,0,12,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',2,'K',24,0,0,'K',23,0,1,'G',58,47,24,0,1,0,25,0,0,0,
47,24,1,0,0,24,0,0,0,255,14,1,2,1,24,1,4,0,25,1,0,0,1,24,1,3,
0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,
0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_7 [85] =
   {	// blr string 

4,2,4,0,6,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,12,
0,15,'K',13,0,0,2,1,25,0,3,0,24,0,3,0,1,41,0,0,0,4,0,24,0,2,0,
1,25,0,1,0,24,0,1,0,1,25,0,5,0,24,0,4,0,1,25,0,2,0,24,0,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_15 [116] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,12,0,
2,7,'C',1,'K',13,0,0,'G',58,47,24,0,0,0,25,0,1,0,58,47,24,0,1,
0,25,0,0,0,58,47,24,0,4,0,25,0,3,0,58,61,24,0,2,0,47,24,0,3,0,
25,0,2,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,
0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_23 [125] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,5,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,
0,7,0,12,0,2,7,'C',1,'K',13,0,0,'G',58,47,24,0,0,0,25,0,2,0,58,
47,24,0,1,0,25,0,1,0,58,47,24,0,4,0,25,0,4,0,58,47,24,0,2,0,25,
0,0,0,47,24,0,3,0,25,0,3,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,
0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_32 [178] =
   {	// blr string 

4,2,4,1,3,0,7,0,7,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,
2,7,'C',3,'K',28,0,0,'K',29,0,1,'K',11,0,2,'D',21,8,0,1,0,0,0,
'G',58,47,24,1,2,0,24,0,4,0,58,47,24,2,0,0,21,15,3,0,22,0,'R',
'D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T',
'_','N','A','M','E',58,47,24,2,2,0,25,0,1,0,58,47,24,1,0,0,25,
0,0,0,47,24,2,1,0,24,0,4,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,
0,1,24,1,1,0,25,1,1,0,1,24,0,4,0,25,1,2,0,255,14,1,1,21,8,0,0,
0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_40 [101] =
   {	// blr string 

4,2,4,1,3,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
29,0,0,'D',21,8,0,1,0,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,
1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,1,24,0,2,0,25,1,
2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_47 [90] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',28,
0,0,'D',21,8,0,1,0,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,
21,8,0,1,0,0,0,25,1,0,0,1,24,0,4,0,25,1,1,0,255,14,1,1,21,8,0,
0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_53 [117] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,41,3,0,0,1,7,0,7,0,4,0,1,0,41,3,0,32,
0,12,0,2,7,'C',1,'K',7,0,0,'G',47,24,0,0,0,25,0,0,0,'F',1,'H',
24,0,2,0,255,14,1,2,1,24,0,1,0,25,1,0,0,1,24,0,3,0,25,1,1,0,1,
21,8,0,1,0,0,0,25,1,2,0,1,24,0,2,0,25,1,3,0,255,14,1,1,21,8,0,
0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_61 [105] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,
'C',1,'K',11,0,0,'D',21,8,0,1,0,0,0,'G',58,47,24,0,0,0,25,0,1,
0,47,24,0,2,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,
24,0,1,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_68 [128] =
   {	// blr string 

4,2,4,4,1,0,7,0,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,7,0,7,0,4,0,1,
0,8,0,12,0,2,7,'C',1,'K',19,0,0,'G',47,24,0,0,0,25,0,0,0,255,
2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,255,17,
0,9,13,12,3,18,0,12,2,10,0,1,2,1,25,2,0,0,24,1,1,0,255,12,4,5,
0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_80 [118] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,7,0,7,0,4,0,1,0,7,0,12,0,
2,7,'C',1,'K',10,0,0,'G',47,24,0,5,0,25,0,0,0,255,2,14,1,2,1,
21,8,0,1,0,0,0,25,1,0,0,1,24,0,4,0,25,1,1,0,255,17,0,9,13,12,
3,18,0,12,2,10,0,1,2,1,25,2,0,0,24,1,4,0,255,255,255,14,1,1,21,
8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_90 [97] =
   {	// blr string 

4,2,4,1,2,0,41,0,0,0,4,7,0,4,0,2,0,41,3,0,32,0,7,0,12,0,2,7,'C',
1,'K',17,0,0,'G',58,47,24,0,0,0,25,0,0,0,47,24,0,1,0,25,0,1,0,
255,14,1,2,1,24,0,2,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_97 [190] =
   {	// blr string 

4,2,4,1,12,0,41,3,0,32,0,9,0,41,0,0,0,1,9,0,41,3,0,32,0,41,3,
0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',
1,'K',6,0,0,'G',47,24,0,3,0,25,0,0,0,255,14,1,2,1,24,0,14,0,41,
1,0,0,8,0,1,24,0,11,0,25,1,1,0,1,24,0,10,0,25,1,2,0,1,24,0,0,
0,25,1,3,0,1,24,0,13,0,25,1,4,0,1,24,0,8,0,25,1,5,0,1,24,0,9,
0,41,1,6,0,9,0,1,21,8,0,1,0,0,0,25,1,7,0,1,24,0,7,0,25,1,10,0,
1,24,0,6,0,25,1,11,0,255,14,1,1,21,8,0,0,0,0,0,25,1,7,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_113 [83] =
   {	// blr string 

4,2,4,1,3,0,7,0,7,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',6,0,0,
'G',47,24,0,3,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
1,24,0,16,0,41,1,2,0,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_120 [128] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,3,0,41,3,0,32,0,41,
3,0,32,0,41,0,0,7,0,12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,
4,0,25,0,1,0,58,47,24,0,2,0,25,0,2,0,47,24,0,1,0,25,0,0,0,255,
2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,
2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_131 [109] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,3,0,41,3,0,32,0,41,3,0,32,0,41,0,0,7,0,12,
0,2,7,'C',1,'K',18,0,0,'D',21,8,0,1,0,0,0,'G',58,47,24,0,4,0,
25,0,1,0,58,47,24,0,2,0,25,0,2,0,47,24,0,0,0,25,0,0,0,255,14,
1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_138 [126] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,7,0,7,0,4,1,3,0,7,0,7,0,7,0,4,0,1,0,7,
0,12,0,2,7,'C',1,'K',26,0,0,'G',47,24,0,1,0,25,0,0,0,255,2,14,
1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,12,0,41,1,2,0,1,0,255,17,
0,9,13,12,3,18,0,12,2,10,0,1,2,1,41,2,1,0,0,0,24,1,12,0,255,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_150 [98] =
   {	// blr string 

4,2,4,1,5,0,9,0,7,0,7,0,7,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',
26,0,0,'G',47,24,0,1,0,25,0,0,0,255,14,1,2,1,24,0,13,0,41,1,0,
0,2,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,11,0,41,1,4,0,3,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_159 [116] =
   {	// blr string 

4,2,4,1,5,0,9,0,7,0,7,0,7,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,
0,12,0,2,7,'C',1,'K',27,0,0,'G',58,47,24,0,1,0,25,0,1,0,47,24,
0,0,0,25,0,0,0,255,14,1,2,1,24,0,7,0,41,1,0,0,2,0,1,21,8,0,1,
0,0,0,25,1,1,0,1,24,0,9,0,41,1,4,0,3,0,255,14,1,1,21,8,0,0,0,
0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_169 [230] =
   {	// blr string 

4,2,4,1,14,0,9,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,
'C',2,'K',27,0,0,'K',2,0,1,'G',58,47,24,1,0,0,24,0,4,0,47,24,
0,1,0,25,0,0,0,255,14,1,2,1,24,1,6,0,41,1,0,0,5,0,1,24,1,0,0,
25,1,1,0,1,24,0,0,0,25,1,2,0,1,24,0,1,0,25,1,3,0,1,21,8,0,1,0,
0,0,25,1,4,0,1,24,1,25,0,25,1,6,0,1,24,1,26,0,25,1,7,0,1,24,1,
11,0,25,1,8,0,1,24,1,8,0,25,1,9,0,1,24,1,9,0,25,1,10,0,1,24,1,
10,0,25,1,11,0,1,24,0,2,0,25,1,12,0,1,24,0,3,0,25,1,13,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,4,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_187 [159] =
   {	// blr string 

4,2,4,1,10,0,9,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',26,0,0,'G',47,24,0,1,0,
25,0,0,0,255,14,1,2,1,24,0,6,0,25,1,0,0,1,24,0,7,0,41,1,1,0,8,
0,1,24,0,0,0,25,1,2,0,1,21,8,0,1,0,0,0,25,1,3,0,1,24,0,3,0,25,
1,4,0,1,24,0,2,0,25,1,5,0,1,24,0,10,0,41,1,7,0,6,0,1,24,0,1,0,
25,1,9,0,255,14,1,1,21,8,0,0,0,0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_201 [50] =
   {	// blr string 

4,2,4,0,3,0,9,0,8,0,7,0,12,0,15,'K',19,0,0,2,1,25,0,0,0,24,0,
3,0,1,25,0,2,0,24,0,1,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_206 [131] =
   {	// blr string 

4,2,4,1,5,0,9,0,41,3,0,32,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',12,0,0,'G',58,47,24,0,1,0,25,0,0,0,47,24,0,8,
0,21,8,0,1,0,0,0,255,14,1,2,1,24,0,5,0,25,1,0,0,1,24,0,0,0,25,
1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,1,24,0,9,0,25,1,3,0,1,24,0,3,
0,25,1,4,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_215 [115] =
   {	// blr string 

4,2,4,1,5,0,9,0,41,3,0,32,0,7,0,7,0,7,0,4,0,1,0,7,0,12,0,2,7,
'C',1,'K',6,0,0,'G',47,24,0,3,0,25,0,0,0,255,14,1,2,1,24,0,0,
0,25,1,0,0,1,24,0,8,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,1,24,
0,15,0,25,1,3,0,1,24,0,3,0,25,1,4,0,255,14,1,1,21,8,0,0,0,0,0,
25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_224 [119] =
   {	// blr string 

4,2,4,1,6,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,7,0,12,0,
2,7,'C',1,'K',6,0,0,'G',47,24,0,3,0,25,0,0,0,255,14,1,2,1,24,
0,8,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,16,0,41,1,3,0,
2,0,1,24,0,15,0,25,1,4,0,1,24,0,3,0,25,1,5,0,255,14,1,1,21,8,
0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_234 [104] =
   {	// blr string 

4,2,4,1,4,0,9,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',6,0,0,'G',47,24,0,8,0,25,0,0,0,255,14,1,2,1,24,0,0,0,25,
1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,15,0,25,1,2,0,1,24,0,3,
0,25,1,3,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_242 [108] =
   {	// blr string 

4,2,4,1,5,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,
'C',1,'K',6,0,0,'G',47,24,0,8,0,25,0,0,0,255,14,1,2,1,21,8,0,
1,0,0,0,25,1,0,0,1,24,0,16,0,41,1,2,0,1,0,1,24,0,15,0,25,1,3,
0,1,24,0,3,0,25,1,4,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_251 [79] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',26,0,0,'G',
47,24,0,1,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,
0,1,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_257 [82] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',26,
0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,
0,0,1,24,0,1,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_263 [180] =
   {	// blr string 

4,2,4,1,5,0,41,3,0,32,0,7,0,7,0,7,0,7,0,4,0,3,0,41,3,0,32,0,41,
3,0,32,0,7,0,12,0,2,7,'C',2,'K',4,0,0,'K',4,0,1,'G',58,47,24,
0,1,0,25,0,1,0,58,57,47,24,0,2,0,34,25,0,2,0,21,8,0,1,0,0,0,47,
24,0,0,0,25,0,0,0,58,47,24,1,0,0,24,0,8,0,47,24,1,3,0,21,8,0,
1,0,0,0,255,14,1,2,1,24,1,1,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,
1,0,1,24,1,2,0,25,1,2,0,1,24,1,6,0,25,1,3,0,1,24,0,6,0,25,1,4,
0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_274 [156] =
   {	// blr string 

4,2,4,1,6,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,
0,12,0,2,7,'C',2,'K',4,0,0,'K',4,0,1,'G',58,47,24,0,3,0,21,8,
0,1,0,0,0,58,47,24,0,1,0,25,0,0,0,47,24,1,8,0,24,0,0,0,255,14,
1,2,1,24,1,1,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,1,2,0,
25,1,2,0,1,24,0,2,0,25,1,3,0,1,24,1,6,0,25,1,4,0,1,24,0,6,0,25,
1,5,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_284 [185] =
   {	// blr string 

4,2,4,1,6,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,4,0,2,0,41,3,0,32,
0,41,0,0,12,0,12,0,2,7,'C',3,'K',4,0,0,'K',22,0,1,'K',4,0,2,'G',
58,47,24,1,5,0,24,0,0,0,58,47,24,1,1,0,25,0,1,0,58,47,24,0,1,
0,25,0,0,0,58,47,24,2,0,0,24,0,8,0,47,24,2,3,0,21,8,0,1,0,0,0,
255,14,1,2,1,24,2,1,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,
2,2,0,25,1,2,0,1,24,0,2,0,25,1,3,0,1,24,2,6,0,25,1,4,0,1,24,0,
6,0,25,1,5,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_295 [107] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,
2,7,'C',1,'K',4,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,
0,1,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,2,0,25,1,2,0,
1,24,0,6,0,25,1,3,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_303 [97] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,2,0,41,3,0,32,0,7,0,12,0,2,7,
'C',1,'K',4,0,0,'G',58,47,24,0,1,0,25,0,0,0,47,24,0,2,0,25,0,
1,0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_310 [82] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',20,
0,0,'G',47,24,0,1,0,25,0,0,0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,
21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_316 [82] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',20,
0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,
0,0,1,24,0,1,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_322 [122] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,41,0,0,32,0,41,0,0,0,1,7,0,4,0,2,0,7,
0,7,0,12,0,2,7,'C',1,'K',16,0,0,'G',58,47,24,0,4,0,25,0,1,0,47,
24,0,5,0,25,0,0,0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,24,0,3,0,25,
1,1,0,1,24,0,2,0,25,1,2,0,1,21,8,0,1,0,0,0,25,1,3,0,255,14,1,
1,21,8,0,0,0,0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_331 [104] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,
'C',1,'K',5,0,0,'G',58,47,24,0,1,0,25,0,1,0,58,59,61,24,0,9,0,
47,24,0,0,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,
0,9,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_338 [82] =
   {	// blr string 

4,2,4,1,2,0,8,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',30,
0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,1,0,25,1,0,0,1,
21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_344 [104] =
   {	// blr string 

4,2,4,1,5,0,41,0,0,0,4,41,3,0,32,0,7,0,7,0,7,0,4,0,1,0,8,0,12,
0,2,7,'C',1,'K',30,0,0,'G',47,24,0,1,0,25,0,0,0,255,14,1,2,1,
24,0,2,0,41,1,0,0,3,0,1,24,0,0,0,41,1,1,0,4,0,1,21,8,0,1,0,0,
0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_353 [85] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',24,0,0,'G',47,24,0,1,0,25,0,0,0,255,14,1,2,1,24,0,0,0,25,
1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,
1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_359 [99] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',12,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,
24,0,1,0,25,1,0,0,1,24,0,0,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,
0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_366 [85] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',22,0,0,'G',47,24,0,5,0,25,0,0,0,255,14,1,2,1,24,0,0,0,25,
1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,
1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_372 [166] =
   {	// blr string 

4,2,4,1,8,0,41,3,0,32,0,9,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,4,
0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',12,0,0,'G',58,47,24,0,0,
0,25,0,0,0,57,61,24,0,7,0,47,24,0,7,0,21,8,0,0,0,0,0,255,14,1,
2,1,24,0,0,0,25,1,0,0,1,24,0,5,0,25,1,1,0,1,24,0,1,0,41,1,2,0,
6,0,1,21,8,0,1,0,0,0,25,1,3,0,1,24,0,8,0,25,1,4,0,1,24,0,3,0,
25,1,5,0,1,24,0,9,0,25,1,7,0,255,14,1,1,21,8,0,0,0,0,0,25,1,3,
0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_384 [105] =
   {	// blr string 

4,2,4,1,3,0,9,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
12,0,0,'G',58,47,24,0,0,0,25,0,0,0,57,61,24,0,7,0,47,24,0,7,0,
21,8,0,0,0,0,0,255,14,1,2,1,24,0,11,0,41,1,0,0,2,0,1,21,8,0,1,
0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_391 [108] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',12,
0,0,'G',58,61,24,0,1,0,58,47,24,0,3,0,25,0,0,0,47,24,0,7,0,21,
8,0,0,0,0,0,'F',1,'H',24,0,2,0,255,14,1,2,1,24,0,0,0,25,1,0,0,
1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,
255,255,76
   };	// end of blr string 
static const UCHAR	jrd_397 [119] =
   {	// blr string 

4,2,4,0,4,0,41,0,0,0,1,7,0,7,0,7,0,2,7,'C',1,'K',10,0,0,'G',58,
59,61,24,0,5,0,58,48,24,0,5,0,21,8,0,0,0,0,0,47,24,0,1,0,21,8,
0,0,0,0,0,255,14,0,2,1,24,0,0,0,25,0,0,0,1,21,8,0,1,0,0,0,25,
0,1,0,1,24,0,5,0,25,0,2,0,1,24,0,4,0,25,0,3,0,255,14,0,1,21,8,
0,0,0,0,0,25,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_403 [114] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,
0,2,7,'C',2,'K',5,0,0,'K',2,0,1,'G',58,47,24,0,2,0,24,1,0,0,58,
47,24,0,1,0,25,0,1,0,47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,1,0,
0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,
0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_410 [130] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,2,0,7,0,7,0,12,0,
2,7,'C',2,'K',29,0,0,'K',28,0,1,'D',21,8,0,1,0,0,0,'G',58,47,
24,0,2,0,25,0,1,0,58,47,24,0,1,0,25,0,0,0,47,24,1,4,0,24,0,2,
0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,24,1,0,0,25,1,1,0,1,21,8,0,
1,0,0,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_418 [178] =
   {	// blr string 

4,2,4,1,9,0,9,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,
0,7,0,7,0,4,0,2,0,7,0,7,0,12,0,2,7,'C',2,'K',29,0,0,'K',28,0,
1,'D',21,8,0,1,0,0,0,'G',58,47,24,0,2,0,25,0,1,0,58,47,24,0,1,
0,25,0,0,0,47,24,1,4,0,24,0,2,0,255,14,1,2,1,24,0,8,0,41,1,0,
0,7,0,1,24,0,7,0,41,1,1,0,8,0,1,24,0,0,0,25,1,2,0,1,24,1,0,0,
25,1,3,0,1,21,8,0,1,0,0,0,25,1,4,0,1,24,0,3,0,41,1,6,0,5,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,4,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_432 [91] =
   {	// blr string 

4,2,4,1,2,0,9,0,7,0,4,0,2,0,7,0,7,0,12,0,2,7,'C',1,'K',8,0,0,
'G',58,47,24,0,0,0,25,0,1,0,47,24,0,1,0,25,0,0,0,255,14,1,2,1,
24,0,2,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,
0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_439 [95] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,7,0,12,0,2,7,
'C',1,'K',10,0,0,'G',47,24,0,5,0,25,0,0,0,255,2,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,
1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_448 [110] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,7,
0,12,0,2,7,'C',1,'K',13,0,0,'G',58,47,24,0,0,0,25,0,0,0,47,24,
0,3,0,25,0,1,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,
0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_458 [148] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,4,0,3,0,41,3,0,32,
0,7,0,7,0,12,0,2,7,'C',2,'K',13,0,0,'K',12,0,1,'G',58,47,24,0,
1,0,25,0,0,0,58,47,24,0,4,0,25,0,2,0,58,47,24,0,3,0,25,0,1,0,
47,24,0,0,0,24,1,0,0,255,14,1,2,1,24,1,1,0,25,1,0,0,1,24,1,0,
0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,1,24,1,3,0,25,1,3,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_468 [134] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,3,0,41,3,0,32,0,7,0,7,0,12,
0,2,7,'C',2,'K',13,0,0,'K',26,0,1,'G',58,47,24,0,1,0,25,0,0,0,
58,47,24,0,4,0,25,0,2,0,58,47,24,0,3,0,25,0,1,0,47,24,0,0,0,24,
1,0,0,255,14,1,2,1,24,1,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,
0,1,24,1,1,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_477 [163] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,4,0,4,0,41,3,0,32,
0,41,3,0,32,0,7,0,7,0,12,0,2,7,'C',2,'K',13,0,0,'K',12,0,1,'G',
58,47,24,0,1,0,25,0,1,0,58,47,24,0,2,0,25,0,0,0,58,47,24,0,4,
0,25,0,3,0,58,47,24,0,3,0,25,0,2,0,47,24,0,0,0,24,1,0,0,255,14,
1,2,1,24,1,1,0,25,1,0,0,1,24,1,0,0,25,1,1,0,1,21,8,0,1,0,0,0,
25,1,2,0,1,24,1,3,0,25,1,3,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,
0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_488 [149] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,4,0,41,3,0,32,0,41,3,0,32,
0,7,0,7,0,12,0,2,7,'C',2,'K',13,0,0,'K',26,0,1,'G',58,47,24,0,
1,0,25,0,1,0,58,47,24,0,2,0,25,0,0,0,58,47,24,0,4,0,25,0,3,0,
58,47,24,0,3,0,25,0,2,0,47,24,0,0,0,24,1,0,0,255,14,1,2,1,24,
1,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,1,1,0,25,1,2,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_498 [99] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',5,0,0,'G',47,24,0,2,0,25,0,0,0,255,14,1,2,1,24,
0,0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_505 [118] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,7,0,7,0,4,0,1,0,7,0,12,0,
2,7,'C',1,'K',10,0,0,'G',47,25,0,0,0,24,0,5,0,255,2,14,1,2,1,
21,8,0,1,0,0,0,25,1,0,0,1,24,0,5,0,25,1,1,0,255,17,0,9,13,12,
3,18,0,12,2,10,0,1,2,1,25,2,0,0,24,1,5,0,255,255,255,14,1,1,21,
8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_515 [122] =
   {	// blr string 

4,2,4,2,1,0,7,0,4,1,1,0,7,0,4,0,3,0,41,0,0,0,1,7,0,7,0,2,7,'C',
1,'K',10,0,0,'G',58,59,61,24,0,5,0,48,24,0,5,0,21,8,0,0,0,0,0,
255,2,14,0,2,1,24,0,0,0,25,0,0,0,1,21,8,0,1,0,0,0,25,0,1,0,1,
24,0,5,0,25,0,2,0,255,17,0,9,13,12,2,18,0,12,1,5,0,255,255,14,
0,1,21,8,0,0,0,0,0,25,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_524 [97] =
   {	// blr string 

4,2,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,7,0,2,7,'C',1,'K',10,0,0,
'G',58,59,61,24,0,5,0,47,24,0,5,0,21,8,0,0,0,0,0,255,2,14,0,2,
1,21,8,0,1,0,0,0,25,0,0,0,255,17,0,9,13,12,2,18,0,12,1,5,0,255,
255,14,0,1,21,8,0,0,0,0,0,25,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_531 [255] =
   {	// blr string 

4,2,4,1,18,0,9,0,9,0,9,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,
0,12,0,2,7,'C',2,'K',5,0,0,'K',2,0,1,'G',58,47,24,0,1,0,25,0,
1,0,58,47,24,0,0,0,25,0,0,0,47,24,1,0,0,24,0,2,0,255,14,1,2,1,
24,1,2,0,41,1,0,0,5,0,1,24,1,6,0,41,1,1,0,6,0,1,24,0,12,0,41,
1,2,0,7,0,1,24,0,2,0,25,1,3,0,1,21,8,0,1,0,0,0,25,1,4,0,1,24,
1,23,0,41,1,9,0,8,0,1,24,0,16,0,41,1,11,0,10,0,1,24,1,25,0,25,
1,12,0,1,24,1,26,0,25,1,13,0,1,24,1,11,0,25,1,14,0,1,24,1,8,0,
25,1,15,0,1,24,1,9,0,25,1,16,0,1,24,1,10,0,25,1,17,0,255,14,1,
1,21,8,0,0,0,0,0,25,1,4,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_554 [182] =
   {	// blr string 

4,2,4,1,13,0,9,0,9,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',2,0,0,'G',47,24,0,
0,0,25,0,0,0,255,14,1,2,1,24,0,2,0,41,1,0,0,3,0,1,24,0,6,0,41,
1,1,0,4,0,1,21,8,0,1,0,0,0,25,1,2,0,1,24,0,23,0,41,1,6,0,5,0,
1,24,0,25,0,25,1,7,0,1,24,0,26,0,25,1,8,0,1,24,0,11,0,25,1,9,
0,1,24,0,8,0,25,1,10,0,1,24,0,9,0,25,1,11,0,1,24,0,10,0,25,1,
12,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 


using namespace Jrd;
using namespace Firebird;

static int blocking_ast_dsql_cache(void* ast_object);
static DSqlCacheItem* get_dsql_cache_item(thread_db* tdbb, int type, const Firebird::MetaName& name);
static int blocking_ast_procedure(void*);
static int blocking_ast_relation(void*);
static int partners_ast_relation(void*);
static ULONG get_rel_flags_from_FLAGS(USHORT);
static void get_trigger(thread_db*, jrd_rel*, bid*, bid*, trig_vec**, const TEXT*, UCHAR, bool, USHORT);
static bool get_type(thread_db*, USHORT*, const UCHAR*, const TEXT*);
static void lookup_view_contexts(thread_db*, jrd_rel*);
static void make_relation_scope_name(const TEXT*, const USHORT, Firebird::string& str);
static jrd_nod* parse_field_blr(thread_db* tdbb, bid* blob_id, const Firebird::MetaName name = Firebird::MetaName());
static jrd_nod* parse_procedure_blr(thread_db*, jrd_prc*, bid*, AutoPtr<CompilerScratch>&);
static void par_messages(thread_db*, const UCHAR* const, USHORT, jrd_prc*, AutoPtr<CompilerScratch>&);
static bool resolve_charset_and_collation(thread_db*, USHORT*, const UCHAR*, const UCHAR*);
static void save_trigger_data(thread_db*, trig_vec**, jrd_rel*, jrd_req*, blb*, bid*,
	const TEXT*, UCHAR, bool, USHORT);
static void store_dependencies(thread_db*, CompilerScratch*, const jrd_rel*,
	const Firebird::MetaName&, int, jrd_tra*);
static bool verify_TRG_ignore_perm(thread_db*, const Firebird::MetaName&);



// Decompile all triggers from vector
static void release_cached_triggers(thread_db* tdbb, trig_vec* vector)
{
	if (!vector)
	{
		return;
	}

	for (size_t i = 0; i < vector->getCount(); i++)
	{
		(*vector)[i].release(tdbb);
	}
}


static void inc_int_use_count(jrd_req* req)
{
	// Increment int_use_count for all procedures in resource list of request
	ResourceList& list = req->req_resources;
	size_t i;
	for (list.find(Resource(Resource::rsc_procedure, 0, NULL, NULL, NULL), i);
		i < list.getCount(); i++)
	{
		Resource& resource = list[i];
		if (resource.rsc_type != Resource::rsc_procedure)
			break;
		fb_assert(resource.rsc_prc->prc_int_use_count >= 0);
		++resource.rsc_prc->prc_int_use_count;
	}
}


// Increment int_use_count for all procedures used by triggers
static void post_used_procedures(trig_vec* vector)
{
	if (!vector)
	{
		return;
	}

	for (size_t i = 0; i < vector->getCount(); i++)
	{
		jrd_req* r = (*vector)[i].request;
		if (r && !CMP_clone_is_active(r))
		{
			inc_int_use_count(r);
		}
	}
}


void MET_get_domain(thread_db* tdbb, const Firebird::MetaName& name, dsc* desc, FieldInfo* fieldInfo)
{
   struct {
          bid  jrd_558;	// RDB$VALIDATION_BLR 
          bid  jrd_559;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_560;	// gds__utility 
          SSHORT jrd_561;	// gds__null_flag 
          SSHORT jrd_562;	// gds__null_flag 
          SSHORT jrd_563;	// gds__null_flag 
          SSHORT jrd_564;	// RDB$NULL_FLAG 
          SSHORT jrd_565;	// RDB$COLLATION_ID 
          SSHORT jrd_566;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_567;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_568;	// RDB$FIELD_LENGTH 
          SSHORT jrd_569;	// RDB$FIELD_SCALE 
          SSHORT jrd_570;	// RDB$FIELD_TYPE 
   } jrd_557;
   struct {
          TEXT  jrd_556 [32];	// RDB$FIELD_NAME 
   } jrd_555;
/**************************************
 *
 *	M E T _ g e t _ d o m a i n
 *
 **************************************
 *
 * Functional description
 *      Get domain descriptor and informations.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	bool found = false;

	jrd_req* handle = CMP_find_request(tdbb, irq_l_domain, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle)
		FLD IN RDB$FIELDS WITH FLD.RDB$FIELD_NAME EQ name.c_str()*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_554, sizeof(jrd_554), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_555.jrd_556, 32);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_555);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 38, (UCHAR*) &jrd_557);
	   if (!jrd_557.jrd_560) break;

		if (!REQUEST(irq_l_domain))
			REQUEST(irq_l_domain) = handle;

		if (DSC_make_descriptor(desc,
								/*FLD.RDB$FIELD_TYPE*/
								jrd_557.jrd_570,
								/*FLD.RDB$FIELD_SCALE*/
								jrd_557.jrd_569,
								/*FLD.RDB$FIELD_LENGTH*/
								jrd_557.jrd_568,
								/*FLD.RDB$FIELD_SUB_TYPE*/
								jrd_557.jrd_567,
								/*FLD.RDB$CHARACTER_SET_ID*/
								jrd_557.jrd_566,
								/*FLD.RDB$COLLATION_ID*/
								jrd_557.jrd_565))
		{
			found = true;

			if (fieldInfo)
			{
				fieldInfo->nullable = /*FLD.RDB$NULL_FLAG.NULL*/
						      jrd_557.jrd_563 || /*FLD.RDB$NULL_FLAG*/
    jrd_557.jrd_564 == 0;

				MemoryPool* csb_pool = dbb->createPool();
				Jrd::ContextPoolHolder context(tdbb, csb_pool);
				try
				{
					if (/*FLD.RDB$DEFAULT_VALUE.NULL*/
					    jrd_557.jrd_562)
						fieldInfo->defaultValue = NULL;
					else
						fieldInfo->defaultValue = parse_field_blr(tdbb, &/*FLD.RDB$DEFAULT_VALUE*/
												 jrd_557.jrd_559);

					if (/*FLD.RDB$VALIDATION_BLR.NULL*/
					    jrd_557.jrd_561)
						fieldInfo->validation = NULL;
					else
						fieldInfo->validation = parse_field_blr(tdbb, &/*FLD.RDB$VALIDATION_BLR*/
											       jrd_557.jrd_558, name);
				}
				catch (const Firebird::Exception&)
				{
					dbb->deletePool(csb_pool);
					throw;
				}
			}
		}

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_domain))
		REQUEST(irq_l_domain) = handle;

	if (!found)
	{
		ERR_post(Arg::Gds(isc_domnotdef) << Arg::Str(name));
	}
}


Firebird::MetaName MET_get_relation_field(thread_db* tdbb,
										  const Firebird::MetaName& relationName,
										  const Firebird::MetaName& fieldName,
										  dsc* desc,
										  FieldInfo* fieldInfo)
{
   struct {
          bid  jrd_536;	// RDB$VALIDATION_BLR 
          bid  jrd_537;	// RDB$DEFAULT_VALUE 
          bid  jrd_538;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_539 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_540;	// gds__utility 
          SSHORT jrd_541;	// gds__null_flag 
          SSHORT jrd_542;	// gds__null_flag 
          SSHORT jrd_543;	// gds__null_flag 
          SSHORT jrd_544;	// gds__null_flag 
          SSHORT jrd_545;	// RDB$NULL_FLAG 
          SSHORT jrd_546;	// gds__null_flag 
          SSHORT jrd_547;	// RDB$NULL_FLAG 
          SSHORT jrd_548;	// RDB$COLLATION_ID 
          SSHORT jrd_549;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_550;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_551;	// RDB$FIELD_LENGTH 
          SSHORT jrd_552;	// RDB$FIELD_SCALE 
          SSHORT jrd_553;	// RDB$FIELD_TYPE 
   } jrd_535;
   struct {
          TEXT  jrd_533 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_534 [32];	// RDB$RELATION_NAME 
   } jrd_532;
/**************************************
 *
 *	M E T _ g e t _ r e l a t i o n _ f i e l d
 *
 **************************************
 *
 * Functional description
 *  Get relation field descriptor and informations.
 *  Returns field source name.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	bool found = false;
	Firebird::MetaName sourceName;

	jrd_req* handle = CMP_find_request(tdbb, irq_l_relfield, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle)
		RFL IN RDB$RELATION_FIELDS CROSS
		FLD IN RDB$FIELDS WITH
			RFL.RDB$RELATION_NAME EQ relationName.c_str() AND
			RFL.RDB$FIELD_NAME EQ fieldName.c_str() AND
			FLD.RDB$FIELD_NAME EQ RFL.RDB$FIELD_SOURCE*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_531, sizeof(jrd_531), true);
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_532.jrd_533, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_532.jrd_534, 32);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_532);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 84, (UCHAR*) &jrd_535);
	   if (!jrd_535.jrd_540) break;

		if (!REQUEST(irq_l_relfield))
			REQUEST(irq_l_relfield) = handle;

		if (DSC_make_descriptor(desc,
								/*FLD.RDB$FIELD_TYPE*/
								jrd_535.jrd_553,
								/*FLD.RDB$FIELD_SCALE*/
								jrd_535.jrd_552,
								/*FLD.RDB$FIELD_LENGTH*/
								jrd_535.jrd_551,
								/*FLD.RDB$FIELD_SUB_TYPE*/
								jrd_535.jrd_550,
								/*FLD.RDB$CHARACTER_SET_ID*/
								jrd_535.jrd_549,
								/*FLD.RDB$COLLATION_ID*/
								jrd_535.jrd_548))
		{
			found = true;
			sourceName = /*RFL.RDB$FIELD_SOURCE*/
				     jrd_535.jrd_539;

			if (fieldInfo)
			{
				fieldInfo->nullable = /*RFL.RDB$NULL_FLAG.NULL*/
						      jrd_535.jrd_546 ?
					(/*FLD.RDB$NULL_FLAG.NULL*/
					 jrd_535.jrd_544 || /*FLD.RDB$NULL_FLAG*/
    jrd_535.jrd_545 == 0) : /*RFL.RDB$NULL_FLAG*/
	 jrd_535.jrd_547 == 0;

				MemoryPool* csb_pool = dbb->createPool();
				Jrd::ContextPoolHolder context(tdbb, csb_pool);
				try
				{
					bid* defaultId = NULL;

					if (!/*RFL.RDB$DEFAULT_VALUE.NULL*/
					     jrd_535.jrd_543)
						defaultId = &/*RFL.RDB$DEFAULT_VALUE*/
							     jrd_535.jrd_538;
					else if (!/*FLD.RDB$DEFAULT_VALUE.NULL*/
						  jrd_535.jrd_542)
						defaultId = &/*FLD.RDB$DEFAULT_VALUE*/
							     jrd_535.jrd_537;

					if (defaultId)
						fieldInfo->defaultValue = parse_field_blr(tdbb, defaultId);
					else
						fieldInfo->defaultValue = NULL;

					if (!/*FLD.RDB$VALIDATION_BLR.NULL*/
					     jrd_535.jrd_541)
						fieldInfo->validation = parse_field_blr(tdbb, &/*FLD.RDB$VALIDATION_BLR*/
											       jrd_535.jrd_536, /*RFL.RDB$FIELD_SOURCE*/
  jrd_535.jrd_539);
					else
						fieldInfo->validation = NULL;
				}
				catch (const Firebird::Exception&)
				{
					dbb->deletePool(csb_pool);
					throw;
				}
			}
		}

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_relfield))
		REQUEST(irq_l_relfield) = handle;

	if (!found)
	{
		ERR_post(Arg::Gds(isc_dyn_column_does_not_exist) << Arg::Str(fieldName) <<
															Arg::Str(relationName));
	}

	return sourceName;
}


void MET_update_partners(thread_db* tdbb)
{
/**************************************
 *
 *      M E T _ u p d a t e _ p a r t n e r s
 *
 **************************************
 *
 * Functional description
 *      Mark all relations to update their links to FK partners
 *      Called when any index is deleted because engine don't know
 *      was it used in any FK or not
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	vec<jrd_rel*>* relations = dbb->dbb_relations;

	vec<jrd_rel*>::iterator ptr = relations->begin();
	for (const vec<jrd_rel*>::const_iterator end = relations->end(); ptr < end; ++ptr)
	{
		jrd_rel* relation = *ptr;
		if (!relation) {
			continue;
		}

		// signal other processes
		LCK_lock(tdbb, relation->rel_partners_lock, LCK_EX, LCK_WAIT);
		LCK_release(tdbb, relation->rel_partners_lock);
		relation->rel_flags |= REL_check_partners;
	}
}


void adjust_dependencies(jrd_prc* procedure)
{
	if (procedure->prc_int_use_count == -1) {
	// Already processed
		return;
	}
	procedure->prc_int_use_count = -1; // Mark as undeletable
	if (procedure->prc_request) {
		// Loop over procedures from resource list of prc_request
		ResourceList& list = procedure->prc_request->req_resources;
		size_t i;
		for (list.find(Resource(Resource::rsc_procedure, 0, NULL, NULL, NULL), i);
			i < list.getCount(); i++)
		{
			Resource& resource = list[i];
			if (resource.rsc_type != Resource::rsc_procedure)
				break;
			procedure = resource.rsc_prc;
			if (procedure->prc_int_use_count == procedure->prc_use_count) {
				// Mark it and all dependent procedures as undeletable
				adjust_dependencies(procedure);
			}
		}
	}
}


#ifdef DEV_BUILD
void MET_verify_cache(thread_db* tdbb)
{
/**************************************
 *
 *      M E T _ v e r i f y _ c a c h e
 *
 **************************************
 *
 * Functional description
 *      Check if all links between procedures are properly counted
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	vec<jrd_prc*>* procedures = dbb->dbb_procedures;
	if (procedures) {
		jrd_prc* procedure;
		vec<jrd_prc*>::iterator	ptr, end;

		for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ++ptr)
		{
			if ( (procedure = *ptr) && procedure->prc_request /*&&
				 !(procedure->prc_flags & PRC_obsolete)*/ )
			{
				fb_assert(procedure->prc_int_use_count == 0);
			}
		}

		/* Walk procedures and calculate internal dependencies */
		for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ptr++)
		{
			if ( (procedure = *ptr) && procedure->prc_request /*&&
				 !(procedure->prc_flags & PRC_obsolete)*/ )
			{
				inc_int_use_count(procedure->prc_request);
			}
		}

		/* Walk procedures again and check dependencies */
		for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ptr++)
		{
			if ( (procedure = *ptr) && procedure->prc_request && /*
				 !(procedure->prc_flags & PRC_obsolete) && */
				 procedure->prc_use_count < procedure->prc_int_use_count)
			{
				char buffer[1024], *buf = buffer;
				buf += sprintf(buf, "Procedure %d:%s is not properly counted (use count=%d, prc use=%d). Used by: \n",
					procedure->prc_id, procedure->prc_name.c_str(), procedure->prc_use_count, procedure->prc_int_use_count);
				vec<jrd_prc*>::const_iterator ptr2 = procedures->begin();
				for (const vec<jrd_prc*>::const_iterator end2 = procedures->end();
					ptr2 < end2; ++ptr2)
				{
					const jrd_prc* prc = *ptr2;
					if (prc && prc->prc_request /*&& !(prc->prc_flags & PRC_obsolete)*/ )
					{
						// Loop over procedures from resource list of prc_request
						const ResourceList& list = prc->prc_request->req_resources;
						size_t i;
						for (list.find(Resource(Resource::rsc_procedure, 0, NULL, NULL, NULL), i);
						     i < list.getCount(); i++)
						{
							const Resource& resource = list[i];
							if (resource.rsc_type != Resource::rsc_procedure)
								break;
							if (resource.rsc_prc == procedure) {
								buf += sprintf(buf, "%d:%s\n", prc->prc_id, prc->prc_name.c_str());
							}
						}
					}
				}
				gds__log(buffer);
				fb_assert(false);
			}
		}

		/* Fix back int_use_count */
		for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ptr++)
		{
			if ( (procedure = *ptr) )
			{
				procedure->prc_int_use_count = 0;
			}
		}
	}
}
#endif


void MET_clear_cache(thread_db* tdbb)
{
/**************************************
 *
 *      M E T _ c l e a r _ c a c h e
 *
 **************************************
 *
 * Functional description
 *      Remove all unused objects from metadata cache to
 *      release resources they use
 *
 **************************************/
	SET_TDBB(tdbb);
#ifdef DEV_BUILD
	MET_verify_cache(tdbb);
#endif

	Database* dbb = tdbb->getDatabase();

	for (int i = 0; i < DB_TRIGGER_MAX; i++) {
		release_cached_triggers(tdbb, dbb->dbb_triggers[i]);
	}

	vec<jrd_rel*>* relations = dbb->dbb_relations;
	if (relations)
	{ // scope
		vec<jrd_rel*>::iterator ptr, end;
		for (ptr = relations->begin(), end = relations->end(); ptr < end; ++ptr)
		{
			jrd_rel* relation = *ptr;
			if (!relation)
				continue;
			release_cached_triggers(tdbb, relation->rel_pre_store);
			release_cached_triggers(tdbb, relation->rel_post_store);
			release_cached_triggers(tdbb, relation->rel_pre_erase);
			release_cached_triggers(tdbb, relation->rel_post_erase);
			release_cached_triggers(tdbb, relation->rel_pre_modify);
			release_cached_triggers(tdbb, relation->rel_post_modify);
		}
	} // scope

	vec<jrd_prc*>* procedures = dbb->dbb_procedures;
	if (procedures)
	{
		jrd_prc* procedure;
		vec<jrd_prc*>::iterator ptr, end;
		/* Walk procedures and calculate internal dependencies */
		for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ++ptr)
		{
			if ( (procedure = *ptr) && procedure->prc_request &&
				 !(procedure->prc_flags & PRC_obsolete) )
			{
				inc_int_use_count(procedure->prc_request);
			}
		}

		// Walk procedures again and adjust dependencies for procedures
		// which will not be removed.
		for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ptr++)
		{
			if ( (procedure = *ptr) && procedure->prc_request &&
				!(procedure->prc_flags & PRC_obsolete) &&
				procedure->prc_use_count != procedure->prc_int_use_count )
			{
				adjust_dependencies(procedure);
			}
		}

		/* Deallocate all used requests */
		for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ptr++)
		{
			if ( (procedure = *ptr)  )
			{

				if ( procedure->prc_request && !(procedure->prc_flags & PRC_obsolete) &&
					 procedure->prc_int_use_count >= 0 &&
					 procedure->prc_use_count == procedure->prc_int_use_count )
				{
					MET_release_procedure_request(tdbb, procedure);

					LCK_release(tdbb, procedure->prc_existence_lock);
					procedure->prc_existence_lock = NULL;
					procedure->prc_flags |= PRC_obsolete;
				}
				// Leave it in state 0 to avoid extra pass next time to clear it
				// Note: we need to adjust prc_int_use_count for all procedures
				// in cache because any of them may have been affected from
				// dependencies earlier. Even procedures that were not scanned yet !
				procedure->prc_int_use_count = 0;
			}
		}
	}
#ifdef DEV_BUILD
	MET_verify_cache(tdbb);
#endif
}


bool MET_procedure_in_use(thread_db* tdbb, jrd_prc* proc)
{
/**************************************
 *
 *      M E T _ p r o c e d u r e _ i n _ u s e
 *
 **************************************
 *
 * Functional description
 *      Determine if procedure is used by any user requests or transactions.
 *      Return false if procedure is used only inside cache or not used at all.
 *
 **************************************/
	SET_TDBB(tdbb);
#ifdef DEV_BUILD
	MET_verify_cache(tdbb);
#endif
	Database* dbb = tdbb->getDatabase();

	// This should not really happen
	vec<jrd_prc*>* procedures = dbb->dbb_procedures;
	if (!procedures) {
		return false;
	}

	vec<jrd_rel*>* relations = dbb->dbb_relations;
	{ // scope
	    vec<jrd_rel*>::iterator ptr, end;
		for (ptr = relations->begin(), end = relations->end(); ptr < end; ++ptr)
		{
			jrd_rel* relation = *ptr;
			if (!relation) {
				continue;
			}
			post_used_procedures(relation->rel_pre_store);
			post_used_procedures(relation->rel_post_store);
			post_used_procedures(relation->rel_pre_erase);
			post_used_procedures(relation->rel_post_erase);
			post_used_procedures(relation->rel_pre_modify);
			post_used_procedures(relation->rel_post_modify);
		}
	} // scope

	jrd_prc* procedure;
	vec<jrd_prc*>::iterator ptr, end;
	/* Walk procedures and calculate internal dependencies */
	for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ++ptr)
	{
		if ((procedure = *ptr) && procedure->prc_request && !(procedure->prc_flags & PRC_obsolete))
		{
			inc_int_use_count(procedure->prc_request);
		}
	}

	// Walk procedures again and adjust dependencies for procedures
	// which will not be removed.
	for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ptr++)
	{
		if ( (procedure = *ptr) && procedure->prc_request &&
			!(procedure->prc_flags & PRC_obsolete) &&
			procedure->prc_use_count != procedure->prc_int_use_count && procedure != proc )
		{
			adjust_dependencies(procedure);
		}
	}

	const bool result = proc->prc_use_count != proc->prc_int_use_count;

	/* Fix back int_use_count */
	for (ptr = procedures->begin(), end = procedures->end(); ptr < end; ptr++)
	{
		if ( (procedure = *ptr) )
		{
			procedure->prc_int_use_count = 0;
		}
	}
#ifdef DEV_BUILD
	MET_verify_cache(tdbb);
#endif
	return result;
}


void MET_release_procedure_request(thread_db* tdbb, jrd_prc* procedure)
{
	CMP_release(tdbb, procedure->prc_request);
	procedure->prc_request = NULL;
	procedure->prc_input_fmt = procedure->prc_output_fmt = NULL;
	procedure->prc_flags &= ~PRC_scanned;
}


void MET_activate_shadow(thread_db* tdbb)
{
   struct {
          SSHORT jrd_514;	// gds__utility 
   } jrd_513;
   struct {
          SSHORT jrd_512;	// RDB$SHADOW_NUMBER 
   } jrd_511;
   struct {
          SSHORT jrd_509;	// gds__utility 
          SSHORT jrd_510;	// RDB$SHADOW_NUMBER 
   } jrd_508;
   struct {
          SSHORT jrd_507;	// RDB$SHADOW_NUMBER 
   } jrd_506;
   struct {
          SSHORT jrd_523;	// gds__utility 
   } jrd_522;
   struct {
          SSHORT jrd_521;	// gds__utility 
   } jrd_520;
   struct {
          TEXT  jrd_517 [256];	// RDB$FILE_NAME 
          SSHORT jrd_518;	// gds__utility 
          SSHORT jrd_519;	// RDB$SHADOW_NUMBER 
   } jrd_516;
   struct {
          SSHORT jrd_530;	// gds__utility 
   } jrd_529;
   struct {
          SSHORT jrd_528;	// gds__utility 
   } jrd_527;
   struct {
          SSHORT jrd_526;	// gds__utility 
   } jrd_525;
/**************************************
 *
 *      M E T _ a c t i v a t e _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *      Activate the current database, which presumably
 *      was formerly a shadow, by deleting all records
 *      corresponding to the shadow that this database
 *      represents.
 *      Get rid of write ahead log for the activated shadow.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* Erase any secondary files of the primary database of the
   shadow being activated. */

	jrd_req* handle = NULL;
	/*FOR(REQUEST_HANDLE handle) X IN RDB$FILES
		WITH X.RDB$SHADOW_NUMBER NOT MISSING
			AND X.RDB$SHADOW_NUMBER EQ 0*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_524, sizeof(jrd_524), true);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 2, (UCHAR*) &jrd_525);
	   if (!jrd_525.jrd_526) break;
		/*ERASE X;*/
		EXE_send (tdbb, handle, 1, 2, (UCHAR*) &jrd_527);
	/*END_FOR;*/
	   EXE_send (tdbb, handle, 2, 2, (UCHAR*) &jrd_529);
	   }
	}

	CMP_release(tdbb, handle);

	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	const char* dbb_file_name = pageSpace->file->fil_string;

/* go through files looking for any that expand to the current database name */
	SCHAR expanded_name[MAXPATHLEN];
	jrd_req* handle2 = handle = NULL;
	/*FOR(REQUEST_HANDLE handle) X IN RDB$FILES
		WITH X.RDB$SHADOW_NUMBER NOT MISSING
			AND X.RDB$SHADOW_NUMBER NE 0*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_515, sizeof(jrd_515), true);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 260, (UCHAR*) &jrd_516);
	   if (!jrd_516.jrd_518) break;

		PIO_expand(/*X.RDB$FILE_NAME*/
			   jrd_516.jrd_517, (USHORT)strlen(/*X.RDB$FILE_NAME*/
		 jrd_516.jrd_517),
					expanded_name, sizeof(expanded_name));

		if (!strcmp(expanded_name, dbb_file_name)) {
			/*FOR(REQUEST_HANDLE handle2) Y IN RDB$FILES
				WITH X.RDB$SHADOW_NUMBER EQ Y.RDB$SHADOW_NUMBER*/
			{
			if (!handle2)
			   handle2 = CMP_compile2 (tdbb, (UCHAR*) jrd_505, sizeof(jrd_505), true);
			jrd_506.jrd_507 = jrd_516.jrd_519;
			EXE_start (tdbb, handle2, dbb->dbb_sys_trans);
			EXE_send (tdbb, handle2, 0, 2, (UCHAR*) &jrd_506);
			while (1)
			   {
			   EXE_receive (tdbb, handle2, 1, 4, (UCHAR*) &jrd_508);
			   if (!jrd_508.jrd_509) break;
				/*MODIFY Y*/
				{
				
					/*Y.RDB$SHADOW_NUMBER*/
					jrd_508.jrd_510 = 0;
				/*END_MODIFY;*/
				jrd_511.jrd_512 = jrd_508.jrd_510;
				EXE_send (tdbb, handle2, 2, 2, (UCHAR*) &jrd_511);
				}
			/*END_FOR;*/
			   EXE_send (tdbb, handle2, 3, 2, (UCHAR*) &jrd_513);
			   }
			}
			/*ERASE X;*/
			EXE_send (tdbb, handle, 1, 2, (UCHAR*) &jrd_520);
		}
	/*END_FOR;*/
	   EXE_send (tdbb, handle, 2, 2, (UCHAR*) &jrd_522);
	   }
	}

	if (handle2)
	{
		CMP_release(tdbb, handle2);
	}
	CMP_release(tdbb, handle);
}


ULONG MET_align(Database* dbb, const dsc* desc, ULONG value)
{
/**************************************
 *
 *      M E T _ a l i g n
 *
 **************************************
 *
 * Functional description
 *      Align value (presumed offset) on appropriate border
 *      and return.
 *
 **************************************/
	USHORT alignment = desc->dsc_length;
	switch (desc->dsc_dtype)
	{
	case dtype_text:
	case dtype_cstring:
		return value;

	case dtype_varying:
		alignment = sizeof(USHORT);
		break;
	}

	alignment = MIN(alignment, dbb->dbb_ods_version >= ODS_VERSION11 ? FORMAT_ALIGNMENT : FB_ALIGNMENT);

	return FB_ALIGN(value, alignment);
}


DeferredWork* MET_change_fields(thread_db* tdbb, jrd_tra* transaction, const dsc* field_source)
{
   struct {
          TEXT  jrd_464 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_465 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_466;	// gds__utility 
          SSHORT jrd_467;	// RDB$TRIGGER_TYPE 
   } jrd_463;
   struct {
          TEXT  jrd_460 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_461;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_462;	// RDB$DEPENDED_ON_TYPE 
   } jrd_459;
   struct {
          TEXT  jrd_474 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_475;	// gds__utility 
          SSHORT jrd_476;	// RDB$PROCEDURE_ID 
   } jrd_473;
   struct {
          TEXT  jrd_470 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_471;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_472;	// RDB$DEPENDED_ON_TYPE 
   } jrd_469;
   struct {
          TEXT  jrd_484 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_485 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_486;	// gds__utility 
          SSHORT jrd_487;	// RDB$TRIGGER_TYPE 
   } jrd_483;
   struct {
          TEXT  jrd_479 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_480 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_481;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_482;	// RDB$DEPENDED_ON_TYPE 
   } jrd_478;
   struct {
          TEXT  jrd_495 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_496;	// gds__utility 
          SSHORT jrd_497;	// RDB$PROCEDURE_ID 
   } jrd_494;
   struct {
          TEXT  jrd_490 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_491 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_492;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_493;	// RDB$DEPENDED_ON_TYPE 
   } jrd_489;
   struct {
          TEXT  jrd_502 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_503 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_504;	// gds__utility 
   } jrd_501;
   struct {
          TEXT  jrd_500 [32];	// RDB$FIELD_SOURCE 
   } jrd_499;
/**************************************
 *
 *      M E T _ c h a n g e _ f i e l d s
 *
 **************************************
 *
 * Functional description
 *      Somebody is modifying RDB$FIELDS.
 *      Find all relations affected and schedule a format update.
 *      Find all procedures and triggers and schedule a BLR validate.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	dsc relation_name;
	DeferredWork* dw = 0;
	jrd_req* request = CMP_find_request(tdbb, irq_m_fields, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$RELATION_FIELDS WITH
			X.RDB$FIELD_SOURCE EQ field_source->dsc_address*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_498, sizeof(jrd_498), true);
	gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_499.jrd_500, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_499);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_501);
	   if (!jrd_501.jrd_504) break;

		if (!REQUEST(irq_m_fields))
			REQUEST(irq_m_fields) = request;

		relation_name.dsc_dtype = dtype_text;
		INTL_ASSIGN_DSC(&relation_name, CS_METADATA, COLLATE_NONE);
		relation_name.dsc_length = sizeof(/*X.RDB$RELATION_NAME*/
						  jrd_501.jrd_503);
		relation_name.dsc_address = (UCHAR *) /*X.RDB$RELATION_NAME*/
						      jrd_501.jrd_503;
		SCL_check_relation(tdbb, &relation_name, SCL_control);
		dw = DFW_post_work(transaction, dfw_update_format, &relation_name, 0);

		if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
		{
			jrd_req* request2 = CMP_find_request(tdbb, irq_m_fields4, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE request2)
				DEP IN RDB$DEPENDENCIES CROSS
				PRC IN RDB$PROCEDURES
					WITH DEP.RDB$DEPENDED_ON_NAME EQ X.RDB$RELATION_NAME AND
						 DEP.RDB$FIELD_NAME EQ X.RDB$FIELD_NAME AND
						 DEP.RDB$DEPENDED_ON_TYPE EQ obj_relation AND
						 DEP.RDB$DEPENDENT_TYPE EQ obj_procedure AND
						 DEP.RDB$DEPENDENT_NAME EQ PRC.RDB$PROCEDURE_NAME*/
			{
			if (!request2)
			   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_488, sizeof(jrd_488), true);
			gds__vtov ((const char*) jrd_501.jrd_502, (char*) jrd_489.jrd_490, 32);
			gds__vtov ((const char*) jrd_501.jrd_503, (char*) jrd_489.jrd_491, 32);
			jrd_489.jrd_492 = obj_procedure;
			jrd_489.jrd_493 = obj_relation;
			EXE_start (tdbb, request2, dbb->dbb_sys_trans);
			EXE_send (tdbb, request2, 0, 68, (UCHAR*) &jrd_489);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 36, (UCHAR*) &jrd_494);
			   if (!jrd_494.jrd_496) break;

				if (!REQUEST(irq_m_fields4))
					REQUEST(irq_m_fields4) = request2;

				Firebird::MetaName proc_name(/*PRC.RDB$PROCEDURE_NAME*/
							     jrd_494.jrd_495);

				dsc desc;
				desc.dsc_dtype = dtype_text;
				INTL_ASSIGN_DSC(&desc, CS_METADATA, COLLATE_NONE);
				desc.dsc_length = proc_name.length();
				desc.dsc_address = (UCHAR*) proc_name.c_str();

				DeferredWork* dw2 =
					DFW_post_work(transaction, dfw_modify_procedure, &desc, /*PRC.RDB$PROCEDURE_ID*/
												jrd_494.jrd_497);
				DFW_post_work_arg(transaction, dw2, NULL, 0, dfw_arg_check_blr);
			/*END_FOR;*/
			   }
			}

			if (!REQUEST(irq_m_fields4))
				REQUEST(irq_m_fields4) = request2;

			request2 = CMP_find_request(tdbb, irq_m_fields5, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE request2)
				DEP IN RDB$DEPENDENCIES CROSS
				TRG IN RDB$TRIGGERS
					WITH DEP.RDB$DEPENDED_ON_NAME EQ X.RDB$RELATION_NAME AND
						 DEP.RDB$FIELD_NAME EQ X.RDB$FIELD_NAME AND
						 DEP.RDB$DEPENDED_ON_TYPE EQ obj_relation AND
						 DEP.RDB$DEPENDENT_TYPE EQ obj_trigger AND
						 DEP.RDB$DEPENDENT_NAME EQ TRG.RDB$TRIGGER_NAME*/
			{
			if (!request2)
			   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_477, sizeof(jrd_477), true);
			gds__vtov ((const char*) jrd_501.jrd_502, (char*) jrd_478.jrd_479, 32);
			gds__vtov ((const char*) jrd_501.jrd_503, (char*) jrd_478.jrd_480, 32);
			jrd_478.jrd_481 = obj_trigger;
			jrd_478.jrd_482 = obj_relation;
			EXE_start (tdbb, request2, dbb->dbb_sys_trans);
			EXE_send (tdbb, request2, 0, 68, (UCHAR*) &jrd_478);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 68, (UCHAR*) &jrd_483);
			   if (!jrd_483.jrd_486) break;

				if (!REQUEST(irq_m_fields5))
					REQUEST(irq_m_fields5) = request2;

				Firebird::MetaName trigger_name(/*TRG.RDB$TRIGGER_NAME*/
								jrd_483.jrd_485);
				Firebird::MetaName trigger_relation_name(/*TRG.RDB$RELATION_NAME*/
									 jrd_483.jrd_484);

				dsc desc;
				desc.dsc_dtype = dtype_text;
				INTL_ASSIGN_DSC(&desc, CS_METADATA, COLLATE_NONE);
				desc.dsc_length = trigger_name.length();
				desc.dsc_address = (UCHAR*) trigger_name.c_str();

				DeferredWork* dw2 = DFW_post_work(transaction, dfw_modify_trigger, &desc, 0);
				DFW_post_work_arg(transaction, dw2, NULL, /*TRG.RDB$TRIGGER_TYPE*/
									  jrd_483.jrd_487, dfw_arg_trg_type);

				desc.dsc_length = trigger_relation_name.length();
				desc.dsc_address = (UCHAR*) trigger_relation_name.c_str();
				DFW_post_work_arg(transaction, dw2, &desc, 0, dfw_arg_check_blr);
			/*END_FOR;*/
			   }
			}

			if (!REQUEST(irq_m_fields5))
				REQUEST(irq_m_fields5) = request2;
		}
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_m_fields))
		REQUEST(irq_m_fields) = request;

	if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
	{
		request = CMP_find_request(tdbb, irq_m_fields2, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			DEP IN RDB$DEPENDENCIES CROSS
			PRC IN RDB$PROCEDURES
				WITH DEP.RDB$DEPENDED_ON_NAME EQ field_source->dsc_address AND
					 DEP.RDB$DEPENDED_ON_TYPE EQ obj_field AND
					 DEP.RDB$DEPENDENT_TYPE EQ obj_procedure AND
					 DEP.RDB$DEPENDENT_NAME EQ PRC.RDB$PROCEDURE_NAME*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_468, sizeof(jrd_468), true);
		gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_469.jrd_470, 32);
		jrd_469.jrd_471 = obj_procedure;
		jrd_469.jrd_472 = obj_field;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 36, (UCHAR*) &jrd_469);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_473);
		   if (!jrd_473.jrd_475) break;

			if (!REQUEST(irq_m_fields2))
				REQUEST(irq_m_fields2) = request;

			Firebird::MetaName proc_name(/*PRC.RDB$PROCEDURE_NAME*/
						     jrd_473.jrd_474);

			dsc desc;
			desc.dsc_dtype = dtype_text;
			INTL_ASSIGN_DSC(&desc, CS_METADATA, COLLATE_NONE);
			desc.dsc_length = proc_name.length();
			desc.dsc_address = (UCHAR*) proc_name.c_str();

			DeferredWork* dw2 =
				DFW_post_work(transaction, dfw_modify_procedure, &desc, /*PRC.RDB$PROCEDURE_ID*/
											jrd_473.jrd_476);
			DFW_post_work_arg(transaction, dw2, NULL, 0, dfw_arg_check_blr);
		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_m_fields2))
			REQUEST(irq_m_fields2) = request;

		request = CMP_find_request(tdbb, irq_m_fields3, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			DEP IN RDB$DEPENDENCIES CROSS
			TRG IN RDB$TRIGGERS
				WITH DEP.RDB$DEPENDED_ON_NAME EQ field_source->dsc_address AND
					 DEP.RDB$DEPENDED_ON_TYPE EQ obj_field AND
					 DEP.RDB$DEPENDENT_TYPE EQ obj_trigger AND
					 DEP.RDB$DEPENDENT_NAME EQ TRG.RDB$TRIGGER_NAME*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_458, sizeof(jrd_458), true);
		gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_459.jrd_460, 32);
		jrd_459.jrd_461 = obj_trigger;
		jrd_459.jrd_462 = obj_field;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 36, (UCHAR*) &jrd_459);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 68, (UCHAR*) &jrd_463);
		   if (!jrd_463.jrd_466) break;

			if (!REQUEST(irq_m_fields3))
				REQUEST(irq_m_fields3) = request;

			Firebird::MetaName trigger_name(/*TRG.RDB$TRIGGER_NAME*/
							jrd_463.jrd_465);
			Firebird::MetaName trigger_relation_name(/*TRG.RDB$RELATION_NAME*/
								 jrd_463.jrd_464);

			dsc desc;
			desc.dsc_dtype = dtype_text;
			INTL_ASSIGN_DSC(&desc, CS_METADATA, COLLATE_NONE);
			desc.dsc_length = trigger_name.length();
			desc.dsc_address = (UCHAR*) trigger_name.c_str();

			DeferredWork* dw2 = DFW_post_work(transaction, dfw_modify_trigger, &desc, 0);
			DFW_post_work_arg(transaction, dw2, NULL, /*TRG.RDB$TRIGGER_TYPE*/
								  jrd_463.jrd_467, dfw_arg_trg_type);

			desc.dsc_length = trigger_relation_name.length();
			desc.dsc_address = (UCHAR*) trigger_relation_name.c_str();
			DFW_post_work_arg(transaction, dw2, &desc, 0, dfw_arg_check_blr);
		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_m_fields3))
			REQUEST(irq_m_fields3) = request;
	}

	return dw;
}


Format* MET_current(thread_db* tdbb, jrd_rel* relation)
{
/**************************************
 *
 *      M E T _ c u r r e n t
 *
 **************************************
 *
 * Functional description
 *      Get the current format for a relation.  The current format is the
 *      format in which new records are to be stored.
 *
 **************************************/

	if (relation->rel_current_format)
		return relation->rel_current_format;

	SET_TDBB(tdbb);

	return relation->rel_current_format = MET_format(tdbb, relation, relation->rel_current_fmt);
}


void MET_delete_dependencies(thread_db* tdbb,
							 const Firebird::MetaName& object_name,
							 int dependency_type,
							 jrd_tra* transaction)
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
          TEXT  jrd_450 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_451;	// RDB$DEPENDENT_TYPE 
   } jrd_449;
/**************************************
 *
 *      M E T _ d e l e t e _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *      Delete all dependencies for the specified
 *      object of given type.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

//	if (!object_name)
//		return;
//
	jrd_req* request = CMP_find_request(tdbb, irq_d_deps, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		DEP IN RDB$DEPENDENCIES
			WITH DEP.RDB$DEPENDENT_NAME = object_name.c_str()
			AND DEP.RDB$DEPENDENT_TYPE = dependency_type*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_448, sizeof(jrd_448), true);
	gds__vtov ((const char*) object_name.c_str(), (char*) jrd_449.jrd_450, 32);
	jrd_449.jrd_451 = dependency_type;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_449);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_452);
	   if (!jrd_452.jrd_453) break;

		if (!REQUEST(irq_d_deps))
			REQUEST(irq_d_deps) = request;

		/*ERASE DEP;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_454);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_456);
	   }
	}

	if (!REQUEST(irq_d_deps))
		REQUEST(irq_d_deps) = request;
}


void MET_delete_shadow(thread_db* tdbb, USHORT shadow_number)
{
   struct {
          SSHORT jrd_447;	// gds__utility 
   } jrd_446;
   struct {
          SSHORT jrd_445;	// gds__utility 
   } jrd_444;
   struct {
          SSHORT jrd_443;	// gds__utility 
   } jrd_442;
   struct {
          SSHORT jrd_441;	// RDB$SHADOW_NUMBER 
   } jrd_440;
/**************************************
 *
 *      M E T _ d e l e t e _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *      When any of the shadows in RDB$FILES for a particular
 *      shadow are deleted, stop shadowing to that file and
 *      remove all other files from the same shadow.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* handle = NULL;

	/*FOR(REQUEST_HANDLE handle)
		X IN RDB$FILES WITH X.RDB$SHADOW_NUMBER EQ shadow_number*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_439, sizeof(jrd_439), true);
	jrd_440.jrd_441 = shadow_number;
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_440);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_442);
	   if (!jrd_442.jrd_443) break;
		/*ERASE X;*/
		EXE_send (tdbb, handle, 2, 2, (UCHAR*) &jrd_444);
	/*END_FOR;*/
	   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_446);
	   }
	}

	CMP_release(tdbb, handle);

	for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next) {
		if (shadow->sdw_number == shadow_number) {
			shadow->sdw_flags |= SDW_shutdown;
		}
	}

/* notify other processes to check for shadow deletion */
	if (SDW_lck_update(tdbb, 0)) {
		SDW_notify(tdbb);
	}
}


bool MET_dsql_cache_use(thread_db* tdbb, int type, const Firebird::MetaName& name)
{
	DSqlCacheItem* item = get_dsql_cache_item(tdbb, type, name);

	const bool obsolete = item->obsolete;

	if (!item->locked)
	{
		// lock to be notified by others when we should mark as obsolete
		LCK_lock(tdbb, item->lock, LCK_SR, LCK_WAIT);
		item->locked = true;
	}

	item->obsolete = false;

	return obsolete;
}


void MET_dsql_cache_release(thread_db* tdbb, int type, const Firebird::MetaName& name)
{
	DSqlCacheItem* item = get_dsql_cache_item(tdbb, type, name);

	// release lock
	LCK_release(tdbb, item->lock);

	// notify others through AST to mark as obsolete
	Database* const dbb = tdbb->getDatabase();
	const size_t key_length = item->lock->lck_length;
	Firebird::AutoPtr<Lock> temp_lock(FB_NEW_RPT(*tdbb->getDefaultPool(), key_length) Lock());
	temp_lock->lck_dbb = dbb;
	temp_lock->lck_parent = dbb->dbb_lock;
	temp_lock->lck_type = LCK_dsql_cache;
	temp_lock->lck_owner_handle = LCK_get_owner_handle(tdbb, temp_lock->lck_type);
	temp_lock->lck_length = key_length;
	memcpy(temp_lock->lck_key.lck_string, item->lock->lck_key.lck_string, key_length);

	if (LCK_lock(tdbb, temp_lock, LCK_EX, LCK_WAIT))
		LCK_release(tdbb, temp_lock);

	item->locked = false;
	item->obsolete = false;
}


void MET_error(const TEXT* string, ...)
{
/**************************************
 *
 *      M E T _ e r r o r
 *
 **************************************
 *
 * Functional description
 *      Post an error in a metadata update
 *      Oh, wow.
 *
 **************************************/
	TEXT s[128];
	va_list ptr;

	va_start(ptr, string);
	VSNPRINTF(s, sizeof(s), string, ptr);
	va_end(ptr);

	ERR_post(Arg::Gds(isc_no_meta_update) <<
			 Arg::Gds(isc_random) << Arg::Str(s));
}

Format* MET_format(thread_db* tdbb, jrd_rel* relation, USHORT number)
{
   struct {
          bid  jrd_437;	// RDB$DESCRIPTOR 
          SSHORT jrd_438;	// gds__utility 
   } jrd_436;
   struct {
          SSHORT jrd_434;	// RDB$FORMAT 
          SSHORT jrd_435;	// RDB$RELATION_ID 
   } jrd_433;
/**************************************
 *
 *      M E T _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *      Lookup a format for given relation.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	Format* format;
	vec<Format*>* formats = relation->rel_formats;
	if (formats && (number < formats->count()) && (format = (*formats)[number]))
	{
		return format;
	}

	format = NULL;
	jrd_req* request = CMP_find_request(tdbb, irq_r_format, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$FORMATS WITH X.RDB$RELATION_ID EQ relation->rel_id AND
			X.RDB$FORMAT EQ number*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_432, sizeof(jrd_432), true);
	jrd_433.jrd_434 = number;
	jrd_433.jrd_435 = relation->rel_id;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_433);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 10, (UCHAR*) &jrd_436);
	   if (!jrd_436.jrd_438) break;

		if (!REQUEST(irq_r_format))
		{
			REQUEST(irq_r_format) = request;
		}
		blb* blob = BLB_open(tdbb, dbb->dbb_sys_trans, &/*X.RDB$DESCRIPTOR*/
								jrd_436.jrd_437);

		if (sizeof(Ods::Descriptor) == sizeof(struct dsc) || dbb->dbb_ods_version < ODS_VERSION11)
		{
			// For ODS10 and earlier read descriptors in their in-memory representation
			// This makes 64-bit and 32-bit ODS10 different for the same architecture.

			const USHORT count = blob->blb_length / sizeof(struct dsc);
			format = Format::newFormat(*dbb->dbb_permanent, count);
			BLB_get_data(tdbb, blob, (UCHAR*) &(format->fmt_desc[0]), blob->blb_length);

			for (Format::fmt_desc_const_iterator desc = format->fmt_desc.end() - 1;
				 desc >= format->fmt_desc.begin();
				 --desc)
			{
	 			if (desc->dsc_address)
				{
					format->fmt_length = (IPTR) desc->dsc_address + desc->dsc_length;
					break;
				}
			}
		}
		else {
			// Use generic representation of formats with 32-bit offsets

			const USHORT count = blob->blb_length / sizeof(Ods::Descriptor);
			format = Format::newFormat(*dbb->dbb_permanent, count);
			Firebird::Array<Ods::Descriptor> odsDescs;
			Ods::Descriptor *odsDesc = odsDescs.getBuffer(count);
			BLB_get_data(tdbb, blob, (UCHAR*) odsDesc, blob->blb_length);

			for (Format::fmt_desc_iterator desc = format->fmt_desc.begin();
				 desc < format->fmt_desc.end(); ++desc, ++odsDesc)
			{
				*desc = *odsDesc;
				if (odsDesc->dsc_offset)
					format->fmt_length = odsDesc->dsc_offset + desc->dsc_length;
			}
		}
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_r_format)) {
		REQUEST(irq_r_format) = request;
	}

	if (!format) {
		format = Format::newFormat(*dbb->dbb_permanent);
	}

	format->fmt_version = number;

/* Link the format block into the world */

	formats = relation->rel_formats =
		vec<Format*>::newVector(*dbb->dbb_permanent, relation->rel_formats, number + 1);
	(*formats)[number] = format;

	return format;
}


bool MET_get_char_coll_subtype(thread_db* tdbb, USHORT* id, const UCHAR* name, USHORT length)
{
/**************************************
 *
 *      M E T _ g e t _ c h a r _ c o l l _ s u b t y p e
 *
 **************************************
 *
 * Functional description
 *      Character types can be specified as either:
 *         a) A POSIX style locale name "<collation>.<characterset>"
 *         or
 *         b) A simple <characterset> name (using default collation)
 *         c) A simple <collation> name    (use charset for collation)
 *
 *      Given an ASCII7 string which could be any of the above, try to
 *      resolve the name in the order a, b, c
 *      a) is only tried iff the name contains a period.
 *      (in which case b) and c) are not tried).
 *
 * Return:
 *      1 if no errors (and *id is set).
 *      0 if the name could not be resolved.
 *
 **************************************/
	SET_TDBB(tdbb);

	fb_assert(id != NULL);
	fb_assert(name != NULL);

	const UCHAR* const end_name = name + length;

/* Force key to uppercase, following C locale rules for uppercasing */
/* At the same time, search for the first period in the string (if any) */
	UCHAR buffer[MAX_SQL_IDENTIFIER_SIZE];			/* BASED ON RDB$COLLATION_NAME */
	UCHAR* p = buffer;
	UCHAR* period = NULL;
	for (; name < end_name && p < buffer + sizeof(buffer) - 1; p++, name++) {
		*p = UPPER7(*name);
		if ((*p == '.') && !period) {
			period = p;
		}
	}
	*p = 0;

/* Is there a period, separating collation name from character set? */
	if (period) {
		*period = 0;
		return resolve_charset_and_collation(tdbb, id, period + 1, buffer);
	}

	/* Is it a character set name (implying charset default collation sequence) */

	bool res = resolve_charset_and_collation(tdbb, id, buffer, NULL);
	if (!res) {
		/* Is it a collation name (implying implementation-default character set) */
		res = resolve_charset_and_collation(tdbb, id, NULL, buffer);
	}
	return res;
}


bool MET_get_char_coll_subtype_info(thread_db* tdbb, USHORT id, SubtypeInfo* info)
{
   struct {
          TEXT  jrd_415 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_416 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_417;	// gds__utility 
   } jrd_414;
   struct {
          SSHORT jrd_412;	// RDB$COLLATION_ID 
          SSHORT jrd_413;	// RDB$CHARACTER_SET_ID 
   } jrd_411;
   struct {
          bid  jrd_423;	// RDB$SPECIFIC_ATTRIBUTES 
          TEXT  jrd_424 [32];	// RDB$BASE_COLLATION_NAME 
          TEXT  jrd_425 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_426 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_427;	// gds__utility 
          SSHORT jrd_428;	// gds__null_flag 
          SSHORT jrd_429;	// RDB$COLLATION_ATTRIBUTES 
          SSHORT jrd_430;	// gds__null_flag 
          SSHORT jrd_431;	// gds__null_flag 
   } jrd_422;
   struct {
          SSHORT jrd_420;	// RDB$COLLATION_ID 
          SSHORT jrd_421;	// RDB$CHARACTER_SET_ID 
   } jrd_419;
/**************************************
 *
 *      M E T _ g e t _ c h a r _ c o l l _ s u b t y p e _ i n f o
 *
 **************************************
 *
 * Functional description
 *      Get charset and collation informations
 *      for a subtype ID.
 *
 **************************************/
	fb_assert(info != NULL);

	const USHORT charset_id = id & 0x00FF;
	const USHORT collation_id = id >> 8;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, irq_l_subtype, IRQ_REQUESTS);
	bool found = false;

	if (dbb->dbb_ods_version >= ODS_VERSION11)
	{
		/*FOR(REQUEST_HANDLE request) FIRST 1
			CL IN RDB$COLLATIONS CROSS
			CS IN RDB$CHARACTER_SETS
			WITH CL.RDB$CHARACTER_SET_ID EQ charset_id AND
				CL.RDB$COLLATION_ID EQ collation_id AND
				CS.RDB$CHARACTER_SET_ID EQ CL.RDB$CHARACTER_SET_ID*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_418, sizeof(jrd_418), true);
		jrd_419.jrd_420 = collation_id;
		jrd_419.jrd_421 = charset_id;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_419);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 114, (UCHAR*) &jrd_422);
		   if (!jrd_422.jrd_427) break;

			found = true;

			info->charsetName = /*CS.RDB$CHARACTER_SET_NAME*/
					    jrd_422.jrd_426;
			info->collationName = /*CL.RDB$COLLATION_NAME*/
					      jrd_422.jrd_425;

			if (/*CL.RDB$BASE_COLLATION_NAME.NULL*/
			    jrd_422.jrd_431)
				info->baseCollationName = info->collationName;
			else
				info->baseCollationName = /*CL.RDB$BASE_COLLATION_NAME*/
							  jrd_422.jrd_424;

			if (/*CL.RDB$SPECIFIC_ATTRIBUTES.NULL*/
			    jrd_422.jrd_430)
				info->specificAttributes.clear();
			else
			{
				blb* blob = BLB_open(tdbb, dbb->dbb_sys_trans, &/*CL.RDB$SPECIFIC_ATTRIBUTES*/
										jrd_422.jrd_423);
				const SLONG length = blob->blb_length;

				// ASF: Here info->specificAttributes is in UNICODE_FSS charset.
				// It will be converted to the collation charset in intl.cpp
				BLB_get_data(tdbb, blob, info->specificAttributes.getBuffer(length), length);
			}

			info->attributes = (USHORT)/*CL.RDB$COLLATION_ATTRIBUTES*/
						   jrd_422.jrd_429;
			info->ignoreAttributes = /*CL.RDB$COLLATION_ATTRIBUTES.NULL*/
						 jrd_422.jrd_428;
		/*END_FOR;*/
		   }
		}
	}
	else
	{
		/*FOR(REQUEST_HANDLE request) FIRST 1
			CL IN RDB$COLLATIONS CROSS
			CS IN RDB$CHARACTER_SETS
			WITH CL.RDB$CHARACTER_SET_ID EQ charset_id AND
				CL.RDB$COLLATION_ID EQ collation_id AND
				CS.RDB$CHARACTER_SET_ID EQ CL.RDB$CHARACTER_SET_ID*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_410, sizeof(jrd_410), true);
		jrd_411.jrd_412 = collation_id;
		jrd_411.jrd_413 = charset_id;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_411);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_414);
		   if (!jrd_414.jrd_417) break;

			found = true;

			info->charsetName = /*CS.RDB$CHARACTER_SET_NAME*/
					    jrd_414.jrd_416;
			info->collationName = /*CL.RDB$COLLATION_NAME*/
					      jrd_414.jrd_415;
			info->baseCollationName = info->collationName;
			info->specificAttributes.clear();
			info->attributes = 0;
			info->ignoreAttributes = true;
		/*END_FOR;*/
		   }
		}
	}

	if (!REQUEST(irq_l_subtype))
		REQUEST(irq_l_subtype) = request;

	return found;
}


jrd_nod* MET_get_dependencies(thread_db* tdbb,
							  jrd_rel* relation,
							  const UCHAR* blob,
							  const ULONG blob_length,
							  CompilerScratch* view_csb,
							  bid* blob_id,
							  jrd_req** request,
							  AutoPtr<CompilerScratch>& csb,
							  const MetaName& object_name,
							  int type,
							  USHORT flags,
							  jrd_tra* transaction,
							  const MetaName& domain_validation)
{
   struct {
          TEXT  jrd_408 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_409;	// gds__utility 
   } jrd_407;
   struct {
          TEXT  jrd_405 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_406 [32];	// RDB$RELATION_NAME 
   } jrd_404;
/**************************************
 *
 *      M E T _ g e t _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *      Get dependencies for an object by parsing
 *      the blr used in its definition.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	csb.reset(CompilerScratch::newCsb(*dbb->dbb_permanent, 5, domain_validation));
	csb->csb_g_flags |= (csb_get_dependencies | flags);

	jrd_nod* node;
	if (blob)
	{
		node = PAR_blr(tdbb, relation, blob, blob_length, view_csb, csb, request,
					   (type == obj_trigger && relation != NULL), 0);
	}
	else
	{
		node = MET_parse_blob(tdbb, relation, blob_id, csb, request,
							  (type == obj_trigger && relation != NULL));
	}

	if (type == obj_computed)
	{
		MetaName domainName;

		jrd_req* handle = NULL;
		/*FOR(REQUEST_HANDLE handle)
			RLF IN RDB$RELATION_FIELDS CROSS
				FLD IN RDB$FIELDS WITH
				RLF.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME AND
				RLF.RDB$RELATION_NAME EQ relation->rel_name.c_str() AND
				RLF.RDB$FIELD_NAME EQ object_name.c_str()*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_403, sizeof(jrd_403), true);
		gds__vtov ((const char*) object_name.c_str(), (char*) jrd_404.jrd_405, 32);
		gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_404.jrd_406, 32);
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_404);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 34, (UCHAR*) &jrd_407);
		   if (!jrd_407.jrd_409) break;
			domainName = /*FLD.RDB$FIELD_NAME*/
				     jrd_407.jrd_408;
		/*END_FOR;*/
		   }
		}
		CMP_release(tdbb, handle);

		MET_delete_dependencies(tdbb, domainName, type, transaction);
		store_dependencies(tdbb, csb, relation, domainName, type, transaction);
	}
	else
	{
		MET_delete_dependencies(tdbb, object_name, type, transaction);
		store_dependencies(tdbb, csb, relation, object_name, type, transaction);
	}

	return node;
}

jrd_nod* MET_get_dependencies(thread_db* tdbb,
							  jrd_rel* relation,
							  const UCHAR* blob,
							  const ULONG blob_length,
							  CompilerScratch* view_csb,
							  bid* blob_id,
							  jrd_req** request,
							  const MetaName& object_name,
							  int type,
							  USHORT flags,
							  jrd_tra* transaction,
							  const MetaName& domain_validation)
{
/**************************************
 *
 *      M E T _ g e t _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *      Get dependencies for an object by parsing
 *      the blr used in its definition.
 *
 **************************************/
	AutoPtr<CompilerScratch> csb;
	return MET_get_dependencies(tdbb, relation, blob, blob_length, view_csb, blob_id, request,
								csb, object_name, type, flags, transaction, domain_validation);
}

jrd_fld* MET_get_field(jrd_rel* relation, USHORT id)
{
/**************************************
 *
 *      M E T _ g e t _ f i e l d
 *
 **************************************
 *
 * Functional description
 *      Get the field block for a field if possible.  If not,
 *      return NULL;
 *
 **************************************/
	vec<jrd_fld*>* vector;

	if (!relation || !(vector = relation->rel_fields) || id >= vector->count())
	{
		return NULL;
	}

	return (*vector)[id];
}


void MET_get_shadow_files(thread_db* tdbb, bool delete_files)
{
   struct {
          TEXT  jrd_399 [256];	// RDB$FILE_NAME 
          SSHORT jrd_400;	// gds__utility 
          SSHORT jrd_401;	// RDB$SHADOW_NUMBER 
          SSHORT jrd_402;	// RDB$FILE_FLAGS 
   } jrd_398;
/**************************************
 *
 *      M E T _ g e t _ s h a d o w _ f i l e s
 *
 **************************************
 *
 * Functional description
 *      Check the shadows found in the database against
 *      our in-memory list: if any new shadow files have
 *      been defined since the last time we looked, start
 *      shadowing to them; if any have been deleted, stop
 *      shadowing to them.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* handle = NULL;
	/*FOR(REQUEST_HANDLE handle) X IN RDB$FILES
		WITH X.RDB$SHADOW_NUMBER NOT MISSING
			AND X.RDB$SHADOW_NUMBER NE 0
			AND X.RDB$FILE_SEQUENCE EQ 0*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_397, sizeof(jrd_397), true);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 262, (UCHAR*) &jrd_398);
	   if (!jrd_398.jrd_400) break;
		if ((/*X.RDB$FILE_FLAGS*/
		     jrd_398.jrd_402 & FILE_shadow) && !(/*X.RDB$FILE_FLAGS*/
		     jrd_398.jrd_402 & FILE_inactive))
		{
			const USHORT file_flags = /*X.RDB$FILE_FLAGS*/
						  jrd_398.jrd_402;
			SDW_start(tdbb, /*X.RDB$FILE_NAME*/
					jrd_398.jrd_399, /*X.RDB$SHADOW_NUMBER*/
  jrd_398.jrd_401, file_flags, delete_files);

			/* if the shadow exists, mark the appropriate shadow
			   block as found for the purposes of this routine;
			   if the shadow was conditional and is no longer, note it */

			for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
			{
				if ((shadow->sdw_number == /*X.RDB$SHADOW_NUMBER*/
							   jrd_398.jrd_401) && !(shadow->sdw_flags & SDW_IGNORE))
				{
					shadow->sdw_flags |= SDW_found;
					if (!(file_flags & FILE_conditional)) {
						shadow->sdw_flags &= ~SDW_conditional;
					}
					break;
				}
			}
		}
	/*END_FOR;*/
	   }
	}

	CMP_release(tdbb, handle);

/* if any current shadows were not defined in database, mark
   them to be shutdown since they don't exist anymore */

	for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next) {
		if (!(shadow->sdw_flags & SDW_found)) {
			shadow->sdw_flags |= SDW_shutdown;
		}
		else {
			shadow->sdw_flags &= ~SDW_found;
		}
	}

	SDW_check(tdbb);
}


void MET_load_db_triggers(thread_db* tdbb, int type)
{
   struct {
          TEXT  jrd_395 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_396;	// gds__utility 
   } jrd_394;
   struct {
          SSHORT jrd_393;	// RDB$TRIGGER_TYPE 
   } jrd_392;
/**************************************
 *
 *      M E T _ l o a d _ d b _ t r i g g e r s
 *
 **************************************
 *
 * Functional description
 *      Load database triggers from RDB$TRIGGERS.
 *
 **************************************/

	SET_TDBB(tdbb);

	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	if ((tdbb->getAttachment()->att_flags & ATT_no_db_triggers) ||
		tdbb->getDatabase()->dbb_triggers[type] != NULL)
	{
		return;
	}

	tdbb->getDatabase()->dbb_triggers[type] = FB_NEW(*tdbb->getDatabase()->dbb_permanent)
		trig_vec(*tdbb->getDatabase()->dbb_permanent);

	jrd_req* trigger_request = NULL;
	int encoded_type = type | TRIGGER_TYPE_DB;

	/*FOR(REQUEST_HANDLE trigger_request)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME MISSING AND
			 TRG.RDB$TRIGGER_TYPE EQ encoded_type AND
			 TRG.RDB$TRIGGER_INACTIVE EQ 0
		SORTED BY TRG.RDB$TRIGGER_SEQUENCE*/
	{
	if (!trigger_request)
	   trigger_request = CMP_compile2 (tdbb, (UCHAR*) jrd_391, sizeof(jrd_391), true);
	jrd_392.jrd_393 = encoded_type;
	EXE_start (tdbb, trigger_request, dbb->dbb_sys_trans);
	EXE_send (tdbb, trigger_request, 0, 2, (UCHAR*) &jrd_392);
	while (1)
	   {
	   EXE_receive (tdbb, trigger_request, 1, 34, (UCHAR*) &jrd_394);
	   if (!jrd_394.jrd_396) break;

		MET_load_trigger(tdbb, NULL, /*TRG.RDB$TRIGGER_NAME*/
					     jrd_394.jrd_395, &tdbb->getDatabase()->dbb_triggers[type]);

	/*END_FOR;*/
	   }
	}

	CMP_release(tdbb, trigger_request);
}


void MET_load_trigger(thread_db* tdbb,
					  jrd_rel* relation,
					  const Firebird::MetaName& trigger_name,
					  trig_vec** triggers)
{
   struct {
          TEXT  jrd_376 [32];	// RDB$TRIGGER_NAME 
          bid  jrd_377;	// RDB$TRIGGER_BLR 
          TEXT  jrd_378 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_379;	// gds__utility 
          SSHORT jrd_380;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_381;	// RDB$TRIGGER_TYPE 
          SSHORT jrd_382;	// gds__null_flag 
          SSHORT jrd_383;	// RDB$FLAGS 
   } jrd_375;
   struct {
          TEXT  jrd_374 [32];	// RDB$TRIGGER_NAME 
   } jrd_373;
   struct {
          bid  jrd_388;	// RDB$DEBUG_INFO 
          SSHORT jrd_389;	// gds__utility 
          SSHORT jrd_390;	// gds__null_flag 
   } jrd_387;
   struct {
          TEXT  jrd_386 [32];	// RDB$TRIGGER_NAME 
   } jrd_385;
/**************************************
 *
 *      M E T _ l o a d _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *      Load triggers from RDB$TRIGGERS.  If a requested,
 *      also load triggers from RDB$RELATIONS.
 *
 **************************************/
	TEXT errmsg[MAX_ERRMSG_LEN + 1];

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	if (relation)
	{
		if (relation->rel_flags & REL_sys_trigs_being_loaded)
			return;

		// No need to load table triggers for ReadOnly databases,
		// since INSERT/DELETE/UPDATE statements are not going to be allowed
		// hvlad: GTT with ON COMMIT DELETE ROWS clause is writable

		if ((dbb->dbb_flags & DBB_read_only) && !(relation->rel_flags & REL_temp_tran))
			return;
	}

	bid debug_blob_id;
	debug_blob_id.clear();
	if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
	{
		jrd_req* debug_info_req = CMP_find_request(tdbb, irq_l_trg_dbg, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE debug_info_req)
			TRG IN RDB$TRIGGERS
			WITH TRG.RDB$TRIGGER_NAME EQ trigger_name.c_str() AND
				 (TRG.RDB$TRIGGER_INACTIVE MISSING OR TRG.RDB$TRIGGER_INACTIVE EQ 0)*/
		{
		if (!debug_info_req)
		   debug_info_req = CMP_compile2 (tdbb, (UCHAR*) jrd_384, sizeof(jrd_384), true);
		gds__vtov ((const char*) trigger_name.c_str(), (char*) jrd_385.jrd_386, 32);
		EXE_start (tdbb, debug_info_req, dbb->dbb_sys_trans);
		EXE_send (tdbb, debug_info_req, 0, 32, (UCHAR*) &jrd_385);
		while (1)
		   {
		   EXE_receive (tdbb, debug_info_req, 1, 12, (UCHAR*) &jrd_387);
		   if (!jrd_387.jrd_389) break;

			if (!REQUEST(irq_l_trg_dbg))
				REQUEST(irq_l_trg_dbg) = debug_info_req;

			if (!/*TRG.RDB$DEBUG_INFO.NULL*/
			     jrd_387.jrd_390)
				debug_blob_id = /*TRG.RDB$DEBUG_INFO*/
						jrd_387.jrd_388;
		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_l_trg_dbg))
			REQUEST(irq_l_trg_dbg) = debug_info_req;
	}

	// Scan RDB$TRIGGERS next

	jrd_req* trigger_request = CMP_find_request(tdbb, irq_s_triggers, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE trigger_request)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$TRIGGER_NAME EQ trigger_name.c_str() AND
			 (TRG.RDB$TRIGGER_INACTIVE MISSING OR TRG.RDB$TRIGGER_INACTIVE EQ 0)*/
	{
	if (!trigger_request)
	   trigger_request = CMP_compile2 (tdbb, (UCHAR*) jrd_372, sizeof(jrd_372), true);
	gds__vtov ((const char*) trigger_name.c_str(), (char*) jrd_373.jrd_374, 32);
	EXE_start (tdbb, trigger_request, dbb->dbb_sys_trans);
	EXE_send (tdbb, trigger_request, 0, 32, (UCHAR*) &jrd_373);
	while (1)
	   {
	   EXE_receive (tdbb, trigger_request, 1, 82, (UCHAR*) &jrd_375);
	   if (!jrd_375.jrd_379) break;
		if (!REQUEST(irq_s_triggers))
			REQUEST(irq_s_triggers) = trigger_request;

		/* check if the trigger is to be fired without any permissions
		   checks. Verify such a claim */
		USHORT trig_flags = (USHORT) /*TRG.RDB$FLAGS*/
					     jrd_375.jrd_383;

		/* if there is an ignore permission flag, see if it is legit */
		if ((/*TRG.RDB$FLAGS*/
		     jrd_375.jrd_383 & TRG_ignore_perm) && !verify_TRG_ignore_perm(tdbb, trigger_name))
		{
			fb_msg_format(NULL, JRD_BUGCHK, 304, sizeof(errmsg),
							errmsg, MsgFormat::SafeArg() << trigger_name.c_str());
			ERR_log(JRD_BUGCHK, 304, errmsg);

			trig_flags &= ~TRG_ignore_perm;
		}

		if (/*TRG.RDB$RELATION_NAME.NULL*/
		    jrd_375.jrd_382)
		{
			if ((/*TRG.RDB$TRIGGER_TYPE*/
			     jrd_375.jrd_381 & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB)
			{
				// this is a database trigger
				get_trigger(tdbb,
							relation,
							&/*TRG.RDB$TRIGGER_BLR*/
							 jrd_375.jrd_377,
							&debug_blob_id,
							triggers,
							/*TRG.RDB$TRIGGER_NAME*/
							jrd_375.jrd_376,
							(UCHAR) (/*TRG.RDB$TRIGGER_TYPE*/
								 jrd_375.jrd_381 & ~TRIGGER_TYPE_DB),
							(bool) /*TRG.RDB$SYSTEM_FLAG*/
							       jrd_375.jrd_380,
							trig_flags);
			}
		}
		else
		{
			// dimitr: support for the universal triggers
			int trigger_action, slot_index = 0;
			while ((trigger_action = TRIGGER_ACTION_SLOT(/*TRG.RDB$TRIGGER_TYPE*/
								     jrd_375.jrd_381, ++slot_index)) > 0)
			{
				get_trigger(tdbb,
							relation,
							&/*TRG.RDB$TRIGGER_BLR*/
							 jrd_375.jrd_377,
							&debug_blob_id,
							triggers + trigger_action,
							/*TRG.RDB$TRIGGER_NAME*/
							jrd_375.jrd_376,
							(UCHAR) trigger_action,
							(bool) /*TRG.RDB$SYSTEM_FLAG*/
							       jrd_375.jrd_380,
							trig_flags);
			}
		}
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_s_triggers))
		REQUEST(irq_s_triggers) = trigger_request;
}



void MET_lookup_cnstrt_for_index(thread_db* tdbb,
								 Firebird::MetaName& constraint_name,
								 const Firebird::MetaName& index_name)
{
   struct {
          TEXT  jrd_370 [32];	// RDB$CONSTRAINT_NAME 
          SSHORT jrd_371;	// gds__utility 
   } jrd_369;
   struct {
          TEXT  jrd_368 [32];	// RDB$INDEX_NAME 
   } jrd_367;
/**************************************
 *
 *      M E T _ l o o k u p _ c n s t r t _ f o r _ i n d e x
 *
 **************************************
 *
 * Functional description
 *      Lookup  constraint name from index name, if one exists.
 *      Calling routine must pass a buffer of at least 32 bytes.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	constraint_name = "";
	jrd_req* request = CMP_find_request(tdbb, irq_l_cnstrt, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$RELATION_CONSTRAINTS WITH X.RDB$INDEX_NAME EQ index_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_366, sizeof(jrd_366), true);
	gds__vtov ((const char*) index_name.c_str(), (char*) jrd_367.jrd_368, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_367);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_369);
	   if (!jrd_369.jrd_371) break;

		if (!REQUEST(irq_l_cnstrt))
			REQUEST(irq_l_cnstrt) = request;

		constraint_name = /*X.RDB$CONSTRAINT_NAME*/
				  jrd_369.jrd_370;

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_cnstrt))
		REQUEST(irq_l_cnstrt) = request;
}


void MET_lookup_cnstrt_for_trigger(thread_db* tdbb,
								   Firebird::MetaName& constraint_name,
								   Firebird::MetaName& relation_name,
								   const Firebird::MetaName& trigger_name)
{
   struct {
          TEXT  jrd_357 [32];	// RDB$CONSTRAINT_NAME 
          SSHORT jrd_358;	// gds__utility 
   } jrd_356;
   struct {
          TEXT  jrd_355 [32];	// RDB$TRIGGER_NAME 
   } jrd_354;
   struct {
          TEXT  jrd_363 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_364 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_365;	// gds__utility 
   } jrd_362;
   struct {
          TEXT  jrd_361 [32];	// RDB$TRIGGER_NAME 
   } jrd_360;
/**************************************
 *
 *      M E T _ l o o k u p _ c n s t r t _ f o r _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *      Lookup  constraint name from trigger name, if one exists.
 *      Calling routine must pass a buffer of at least 32 bytes.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	constraint_name = "";
	relation_name = "";
	jrd_req* request = CMP_find_request(tdbb, irq_l_check, IRQ_REQUESTS);
	jrd_req* request2 = CMP_find_request(tdbb, irq_l_check2, IRQ_REQUESTS);

/* utilize two requests rather than one so that we
   guarantee we always return the name of the relation
   that the trigger is defined on, even if we don't
   have a check constraint defined for that trigger */

	/*FOR(REQUEST_HANDLE request)
		Y IN RDB$TRIGGERS WITH
			Y.RDB$TRIGGER_NAME EQ trigger_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_359, sizeof(jrd_359), true);
	gds__vtov ((const char*) trigger_name.c_str(), (char*) jrd_360.jrd_361, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_360);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_362);
	   if (!jrd_362.jrd_365) break;

		if (!REQUEST(irq_l_check))
			REQUEST(irq_l_check) = request;

		/*FOR(REQUEST_HANDLE request2)
			X IN RDB$CHECK_CONSTRAINTS WITH
				X.RDB$TRIGGER_NAME EQ Y.RDB$TRIGGER_NAME*/
		{
		if (!request2)
		   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_353, sizeof(jrd_353), true);
		gds__vtov ((const char*) jrd_362.jrd_364, (char*) jrd_354.jrd_355, 32);
		EXE_start (tdbb, request2, dbb->dbb_sys_trans);
		EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_354);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 34, (UCHAR*) &jrd_356);
		   if (!jrd_356.jrd_358) break;

			if (!REQUEST(irq_l_check2))
				REQUEST(irq_l_check2) = request2;

			constraint_name = /*X.RDB$CONSTRAINT_NAME*/
					  jrd_356.jrd_357;

		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_l_check2))
			REQUEST(irq_l_check2) = request2;

		relation_name = /*Y.RDB$RELATION_NAME*/
				jrd_362.jrd_363;

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_check))
		REQUEST(irq_l_check) = request;
}


void MET_lookup_exception(thread_db* tdbb,
						  SLONG number,
						  Firebird::MetaName& name,
						  Firebird::string* message)
{
   struct {
          TEXT  jrd_348 [1024];	// RDB$MESSAGE 
          TEXT  jrd_349 [32];	// RDB$EXCEPTION_NAME 
          SSHORT jrd_350;	// gds__utility 
          SSHORT jrd_351;	// gds__null_flag 
          SSHORT jrd_352;	// gds__null_flag 
   } jrd_347;
   struct {
          SLONG  jrd_346;	// RDB$EXCEPTION_NUMBER 
   } jrd_345;
/**************************************
 *
 *      M E T _ l o o k u p _ e x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *      Lookup exception by number and return its name and message.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* We need to look up exception in RDB$EXCEPTIONS */

	jrd_req* request = CMP_find_request(tdbb, irq_l_exception, IRQ_REQUESTS);

	name = "";
	if (message)
		*message = "";

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$EXCEPTIONS WITH X.RDB$EXCEPTION_NUMBER = number*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_344, sizeof(jrd_344), true);
	jrd_345.jrd_346 = number;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_345);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 1062, (UCHAR*) &jrd_347);
	   if (!jrd_347.jrd_350) break;

		if (!REQUEST(irq_l_exception))
		{
			REQUEST(irq_l_exception) = request;
		}

		if (!/*X.RDB$EXCEPTION_NAME.NULL*/
		     jrd_347.jrd_352)
		{
			name = /*X.RDB$EXCEPTION_NAME*/
			       jrd_347.jrd_349;
		}
		if (!/*X.RDB$MESSAGE.NULL*/
		     jrd_347.jrd_351 && message)
		{
			*message = /*X.RDB$MESSAGE*/
				   jrd_347.jrd_348;
		}
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_exception))
	{
		REQUEST(irq_l_exception) = request;
	}
}


SLONG MET_lookup_exception_number(thread_db* tdbb, const Firebird::MetaName& name)
{
   struct {
          SLONG  jrd_342;	// RDB$EXCEPTION_NUMBER 
          SSHORT jrd_343;	// gds__utility 
   } jrd_341;
   struct {
          TEXT  jrd_340 [32];	// RDB$EXCEPTION_NAME 
   } jrd_339;
/**************************************
 *
 *      M E T _ l o o k u p _ e x c e p t i o n _ n u m b e r
 *
 **************************************
 *
 * Functional description
 *      Lookup exception by name and return its number.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* We need to look up exception in RDB$EXCEPTIONS */

	jrd_req* request = CMP_find_request(tdbb, irq_l_except_no, IRQ_REQUESTS);

	SLONG number = 0;

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$EXCEPTIONS WITH X.RDB$EXCEPTION_NAME = name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_338, sizeof(jrd_338), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_339.jrd_340, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_339);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_341);
	   if (!jrd_341.jrd_343) break;

		if (!REQUEST(irq_l_except_no))
			REQUEST(irq_l_except_no) = request;

		number = /*X.RDB$EXCEPTION_NUMBER*/
			 jrd_341.jrd_342;

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_except_no))
		REQUEST(irq_l_except_no) = request;

	return number;
}


int MET_lookup_field(thread_db* tdbb,
					 jrd_rel* relation,
					 const Firebird::MetaName& name)
{
   struct {
          SSHORT jrd_336;	// gds__utility 
          SSHORT jrd_337;	// RDB$FIELD_ID 
   } jrd_335;
   struct {
          TEXT  jrd_333 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_334 [32];	// RDB$RELATION_NAME 
   } jrd_332;
/**************************************
 *
 *      M E T _ l o o k u p _ f i e l d
 *
 **************************************
 *
 * Functional description
 *      Look up a field name.
 *
 *	if the field is not found return -1
 *
 *****************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* Start by checking field names that we already know */
	vec<jrd_fld*>* vector = relation->rel_fields;

	if (vector)
	{
		int id = 0;
		vec<jrd_fld*>::iterator fieldIter = vector->begin();

		for (const vec<jrd_fld*>::const_iterator end = vector->end();  fieldIter < end;
			++fieldIter, ++id)
		{
			if (*fieldIter)
			{
				jrd_fld* field = *fieldIter;
				if (field->fld_name == name)
				{
					return id;
				}
			}
		}
	}

/* Not found.  Next, try system relations directly */

	int id = -1;

	if (relation->rel_flags & REL_deleted)
	{
		return id;
	}

	jrd_req* request = CMP_find_request(tdbb, irq_l_field, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$RELATION_FIELDS WITH
			X.RDB$RELATION_NAME EQ relation->rel_name.c_str() AND
			X.RDB$FIELD_ID NOT MISSING AND
			X.RDB$FIELD_NAME EQ name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_331, sizeof(jrd_331), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_332.jrd_333, 32);
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_332.jrd_334, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_332);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_335);
	   if (!jrd_335.jrd_336) break;

		if (!REQUEST(irq_l_field)) {
			REQUEST(irq_l_field) = request;
		}

		id = /*X.RDB$FIELD_ID*/
		     jrd_335.jrd_337;

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_field)) {
		REQUEST(irq_l_field) = request;
	}

	return id;
}


BlobFilter* MET_lookup_filter(thread_db* tdbb, SSHORT from, SSHORT to)
{
   struct {
          TEXT  jrd_327 [32];	// RDB$FUNCTION_NAME 
          TEXT  jrd_328 [32];	// RDB$ENTRYPOINT 
          TEXT  jrd_329 [256];	// RDB$MODULE_NAME 
          SSHORT jrd_330;	// gds__utility 
   } jrd_326;
   struct {
          SSHORT jrd_324;	// RDB$OUTPUT_SUB_TYPE 
          SSHORT jrd_325;	// RDB$INPUT_SUB_TYPE 
   } jrd_323;
/**************************************
 *
 *      M E T _ l o o k u p _ f i l t e r
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	FPTR_BFILTER_CALLBACK filter = NULL;
	BlobFilter* blf = NULL;

	jrd_req* request = CMP_find_request(tdbb, irq_r_filters, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$FILTERS WITH X.RDB$INPUT_SUB_TYPE EQ from AND
		X.RDB$OUTPUT_SUB_TYPE EQ to*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_322, sizeof(jrd_322), true);
	jrd_323.jrd_324 = to;
	jrd_323.jrd_325 = from;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_323);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 322, (UCHAR*) &jrd_326);
	   if (!jrd_326.jrd_330) break;

		if (!REQUEST(irq_r_filters))
			REQUEST(irq_r_filters) = request;
		filter = (FPTR_BFILTER_CALLBACK)
			Module::lookup(/*X.RDB$MODULE_NAME*/
				       jrd_326.jrd_329, /*X.RDB$ENTRYPOINT*/
  jrd_326.jrd_328, dbb->dbb_modules);
		if (filter)
		{
			blf = FB_NEW(*dbb->dbb_permanent) BlobFilter(*dbb->dbb_permanent);
			blf->blf_next = NULL;
			blf->blf_from = from;
			blf->blf_to = to;
			blf->blf_filter = filter;
			blf->blf_exception_message.printf(EXCEPTION_MESSAGE,
					/*X.RDB$FUNCTION_NAME*/
					jrd_326.jrd_327, /*X.RDB$ENTRYPOINT*/
  jrd_326.jrd_328, /*X.RDB$MODULE_NAME*/
  jrd_326.jrd_329);
		}
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_r_filters))
		REQUEST(irq_r_filters) = request;

	return blf;
}


SLONG MET_lookup_generator(thread_db* tdbb, const Firebird::MetaName& name)
{
   struct {
          SSHORT jrd_320;	// gds__utility 
          SSHORT jrd_321;	// RDB$GENERATOR_ID 
   } jrd_319;
   struct {
          TEXT  jrd_318 [32];	// RDB$GENERATOR_NAME 
   } jrd_317;
/**************************************
 *
 *      M E T _ l o o k u p _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *      Lookup generator ID by its name.
 *		If the name is not found, return -1.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	if (name == "RDB$GENERATORS")
		return 0;

	SLONG gen_id = -1;

	jrd_req* request = CMP_find_request(tdbb, irq_r_gen_id, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$GENERATORS WITH X.RDB$GENERATOR_NAME EQ name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_316, sizeof(jrd_316), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_317.jrd_318, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_317);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_319);
	   if (!jrd_319.jrd_320) break;

		if (!REQUEST(irq_r_gen_id))
			REQUEST(irq_r_gen_id) = request;

		gen_id = /*X.RDB$GENERATOR_ID*/
			 jrd_319.jrd_321;
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_r_gen_id))
		REQUEST(irq_r_gen_id) = request;

	return gen_id;
}

void MET_lookup_generator_id (thread_db* tdbb, SLONG gen_id, Firebird::MetaName& name)
{
   struct {
          TEXT  jrd_314 [32];	// RDB$GENERATOR_NAME 
          SSHORT jrd_315;	// gds__utility 
   } jrd_313;
   struct {
          SSHORT jrd_312;	// RDB$GENERATOR_ID 
   } jrd_311;
/**************************************
 *
 *      M E T _ l o o k u p _ g e n e r a t o r _ i d
 *
 **************************************
 *
 * Functional description
 *      Lookup generator (aka gen_id) by ID. It will load
 *		the name in the third parameter.
 *
 **************************************/
	SET_TDBB (tdbb);
	Database* dbb  = tdbb->getDatabase();

	if (!gen_id) {
		name = "RDB$GENERATORS";
		return;
	}

	name = "";

	jrd_req* request = CMP_find_request (tdbb, irq_r_gen_id_num, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		X IN RDB$GENERATORS WITH X.RDB$GENERATOR_ID EQ gen_id*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_310, sizeof(jrd_310), true);
	jrd_311.jrd_312 = gen_id;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_311);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_313);
	   if (!jrd_313.jrd_315) break;

		if (!REQUEST (irq_r_gen_id_num))
			REQUEST (irq_r_gen_id_num) = request;
		name = /*X.RDB$GENERATOR_NAME*/
		       jrd_313.jrd_314;
	/*END_FOR;*/
	   }
	}

	if (!REQUEST (irq_r_gen_id_num))
		REQUEST (irq_r_gen_id_num) = request;
}

void MET_lookup_index(thread_db* tdbb,
						Firebird::MetaName& index_name,
						const Firebird::MetaName& relation_name,
						USHORT number)
{
   struct {
          TEXT  jrd_308 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_309;	// gds__utility 
   } jrd_307;
   struct {
          TEXT  jrd_305 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_306;	// RDB$INDEX_ID 
   } jrd_304;
/**************************************
 *
 *      M E T _ l o o k u p _ i n d e x
 *
 **************************************
 *
 * Functional description
 *      Lookup index name from relation and index number.
 *      Calling routine must pass a buffer of at least 32 bytes.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	index_name = "";

	jrd_req* request = CMP_find_request(tdbb, irq_l_index, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$INDICES WITH X.RDB$RELATION_NAME EQ relation_name.c_str()
			AND X.RDB$INDEX_ID EQ number*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_303, sizeof(jrd_303), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_304.jrd_305, 32);
	jrd_304.jrd_306 = number;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_304);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_307);
	   if (!jrd_307.jrd_309) break;

		if (!REQUEST(irq_l_index))
			REQUEST(irq_l_index) = request;

		index_name = /*X.RDB$INDEX_NAME*/
			     jrd_307.jrd_308;

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_index))
		REQUEST(irq_l_index) = request;
}


SLONG MET_lookup_index_name(thread_db* tdbb,
							const Firebird::MetaName& index_name,
							SLONG* relation_id, SSHORT* status)
{
   struct {
          TEXT  jrd_299 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_300;	// gds__utility 
          SSHORT jrd_301;	// RDB$INDEX_ID 
          SSHORT jrd_302;	// RDB$INDEX_INACTIVE 
   } jrd_298;
   struct {
          TEXT  jrd_297 [32];	// RDB$INDEX_NAME 
   } jrd_296;
/**************************************
 *
 *      M E T _ l o o k u p _ i n d e x _ n a m e
 *
 **************************************
 *
 * Functional description
 *      Lookup index id from index name.
 *
 **************************************/
	SLONG id = -1;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, irq_l_index_name, IRQ_REQUESTS);

	*status = MET_object_unknown;

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$INDICES WITH
			X.RDB$INDEX_NAME EQ index_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_295, sizeof(jrd_295), true);
	gds__vtov ((const char*) index_name.c_str(), (char*) jrd_296.jrd_297, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_296);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 38, (UCHAR*) &jrd_298);
	   if (!jrd_298.jrd_300) break;

		if (!REQUEST(irq_l_index_name))
			REQUEST(irq_l_index_name) = request;

		if (/*X.RDB$INDEX_INACTIVE*/
		    jrd_298.jrd_302 == 0) {
			*status = MET_object_active;
		}
		else {
			*status = MET_object_inactive;
		}

		id = /*X.RDB$INDEX_ID*/
		     jrd_298.jrd_301 - 1;
		const jrd_rel* relation = MET_lookup_relation(tdbb, /*X.RDB$RELATION_NAME*/
								    jrd_298.jrd_299);
		*relation_id = relation->rel_id;

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_index_name))
		REQUEST(irq_l_index_name) = request;

	return id;
}


bool MET_lookup_partner(thread_db*	tdbb,
						jrd_rel*	relation,
						index_desc*	idx,
						const TEXT* index_name)
{
   struct {
          TEXT  jrd_269 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_270;	// gds__utility 
          SSHORT jrd_271;	// RDB$INDEX_ID 
          SSHORT jrd_272;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_273;	// RDB$INDEX_INACTIVE 
   } jrd_268;
   struct {
          TEXT  jrd_265 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_266 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_267;	// RDB$INDEX_ID 
   } jrd_264;
   struct {
          TEXT  jrd_278 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_279;	// gds__utility 
          SSHORT jrd_280;	// RDB$INDEX_ID 
          SSHORT jrd_281;	// RDB$INDEX_ID 
          SSHORT jrd_282;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_283;	// RDB$INDEX_INACTIVE 
   } jrd_277;
   struct {
          TEXT  jrd_276 [32];	// RDB$RELATION_NAME 
   } jrd_275;
   struct {
          TEXT  jrd_289 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_290;	// gds__utility 
          SSHORT jrd_291;	// RDB$INDEX_ID 
          SSHORT jrd_292;	// RDB$INDEX_ID 
          SSHORT jrd_293;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_294;	// RDB$INDEX_INACTIVE 
   } jrd_288;
   struct {
          TEXT  jrd_286 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_287 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_285;
/**************************************
 *
 *      M E T _ l o o k u p _ p a r t n e r
 *
 **************************************
 *
 * Functional description
 *      Find partner index participating in a
 *      foreign key relationship.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	Database::CheckoutLockGuard guard(dbb, dbb->dbb_meta_mutex);

	if (relation->rel_flags & REL_check_partners)
	{
		/* Prepare for rescan of foreign references on other relations'
		   primary keys and release stale vectors. */

		jrd_req* request = CMP_find_request(tdbb, irq_foreign1, IRQ_REQUESTS);
		frgn* references = &relation->rel_foreign_refs;
		int index_number = 0;

		if (references->frgn_reference_ids) {
			delete references->frgn_reference_ids;
			references->frgn_reference_ids = NULL;
		}
		if (references->frgn_relations) {
			delete references->frgn_relations;
			references->frgn_relations = NULL;
		}
		if (references->frgn_indexes) {
			delete references->frgn_indexes;
			references->frgn_indexes = NULL;
		}

		/*FOR(REQUEST_HANDLE request)
			IDX IN RDB$INDICES CROSS
				RC IN RDB$RELATION_CONSTRAINTS
				OVER RDB$INDEX_NAME CROSS
				IND IN RDB$INDICES WITH
				RC.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY AND
				IDX.RDB$RELATION_NAME EQ relation->rel_name.c_str() AND
				IND.RDB$INDEX_NAME EQ IDX.RDB$FOREIGN_KEY AND
				IND.RDB$UNIQUE_FLAG = 1*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_284, sizeof(jrd_284), true);
		gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_285.jrd_286, 32);
		gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_285.jrd_287, 12);
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 44, (UCHAR*) &jrd_285);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 42, (UCHAR*) &jrd_288);
		   if (!jrd_288.jrd_290) break;

			if (!REQUEST(irq_foreign1))
			{
				REQUEST(irq_foreign1) = request;
			}

			const jrd_rel* partner_relation = MET_lookup_relation(tdbb, /*IND.RDB$RELATION_NAME*/
										    jrd_288.jrd_289);

			if (partner_relation && !/*IDX.RDB$INDEX_INACTIVE*/
						 jrd_288.jrd_294 && !/*IND.RDB$INDEX_INACTIVE*/
     jrd_288.jrd_293)
			{
				// This seems a good candidate for vcl.
				references->frgn_reference_ids =
					vec<int>::newVector(*dbb->dbb_permanent, references->frgn_reference_ids,
								   index_number + 1);

				(*references->frgn_reference_ids)[index_number] = /*IDX.RDB$INDEX_ID*/
										  jrd_288.jrd_292 - 1;

				references->frgn_relations =
					vec<int>::newVector(*dbb->dbb_permanent, references->frgn_relations,
								   index_number + 1);

				(*references->frgn_relations)[index_number] = partner_relation->rel_id;

				references->frgn_indexes =
					vec<int>::newVector(*dbb->dbb_permanent, references->frgn_indexes,
								   index_number + 1);

				(*references->frgn_indexes)[index_number] = /*IND.RDB$INDEX_ID*/
									    jrd_288.jrd_291 - 1;

				index_number++;
			}
		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_foreign1))
		{
			REQUEST(irq_foreign1) = request;
		}

		/* Prepare for rescan of primary dependencies on relation's primary
		   key and stale vectors. */

		request = CMP_find_request(tdbb, irq_foreign2, IRQ_REQUESTS);
		prim* dependencies = &relation->rel_primary_dpnds;
		index_number = 0;

		if (dependencies->prim_reference_ids) {
			delete dependencies->prim_reference_ids;
			dependencies->prim_reference_ids = NULL;
		}
		if (dependencies->prim_relations) {
			delete dependencies->prim_relations;
			dependencies->prim_relations = NULL;
		}
		if (dependencies->prim_indexes) {
			delete dependencies->prim_indexes;
			dependencies->prim_indexes = NULL;
		}

		/*FOR(REQUEST_HANDLE request)
			IDX IN RDB$INDICES CROSS
				IND IN RDB$INDICES WITH
				IDX.RDB$UNIQUE_FLAG = 1 AND
				IDX.RDB$RELATION_NAME EQ relation->rel_name.c_str() AND
				IND.RDB$FOREIGN_KEY EQ IDX.RDB$INDEX_NAME*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_274, sizeof(jrd_274), true);
		gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_275.jrd_276, 32);
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_275);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 42, (UCHAR*) &jrd_277);
		   if (!jrd_277.jrd_279) break;

			if (!REQUEST(irq_foreign2)) {
				REQUEST(irq_foreign2) = request;
			}

			const jrd_rel* partner_relation = MET_lookup_relation(tdbb, /*IND.RDB$RELATION_NAME*/
										    jrd_277.jrd_278);

			if (partner_relation && !/*IDX.RDB$INDEX_INACTIVE*/
						 jrd_277.jrd_283 && !/*IND.RDB$INDEX_INACTIVE*/
     jrd_277.jrd_282)
			{
				dependencies->prim_reference_ids =
					vec<int>::newVector(*dbb->dbb_permanent, dependencies->prim_reference_ids,
								   index_number + 1);

				(*dependencies->prim_reference_ids)[index_number] = /*IDX.RDB$INDEX_ID*/
										    jrd_277.jrd_281 - 1;

				dependencies->prim_relations =
					vec<int>::newVector(*dbb->dbb_permanent, dependencies->prim_relations,
								   index_number + 1);

				(*dependencies->prim_relations)[index_number] = partner_relation->rel_id;

				dependencies->prim_indexes =
					vec<int>::newVector(*dbb->dbb_permanent, dependencies->prim_indexes,
								   index_number + 1);

				(*dependencies->prim_indexes)[index_number] = /*IND.RDB$INDEX_ID*/
									      jrd_277.jrd_280 - 1;

				index_number++;
			}
		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_foreign2))
			REQUEST(irq_foreign2) = request;

		LCK_lock(tdbb, relation->rel_partners_lock, LCK_SR, LCK_WAIT);
		relation->rel_flags &= ~REL_check_partners;
	}

	if (idx->idx_flags & idx_foreign)
	{
		if (index_name)
		{
			/* Since primary key index names aren't being cached, do a long
			   hard lookup. This is only called during index create for foreign
			   keys. */

			bool found = false;
			jrd_req* request = NULL;

			/*FOR(REQUEST_HANDLE request)
				IDX IN RDB$INDICES CROSS
					IND IN RDB$INDICES WITH
					IDX.RDB$RELATION_NAME EQ relation->rel_name.c_str() AND
					(IDX.RDB$INDEX_ID EQ idx->idx_id + 1 OR
					 IDX.RDB$INDEX_NAME EQ index_name) AND
					IND.RDB$INDEX_NAME EQ IDX.RDB$FOREIGN_KEY AND
					IND.RDB$UNIQUE_FLAG = 1*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_263, sizeof(jrd_263), true);
			gds__vtov ((const char*) index_name, (char*) jrd_264.jrd_265, 32);
			gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_264.jrd_266, 32);
			jrd_264.jrd_267 = idx->idx_id;
			EXE_start (tdbb, request, dbb->dbb_sys_trans);
			EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_264);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 40, (UCHAR*) &jrd_268);
			   if (!jrd_268.jrd_270) break;

				const jrd_rel* partner_relation = MET_lookup_relation(tdbb, /*IND.RDB$RELATION_NAME*/
											    jrd_268.jrd_269);

				if (partner_relation && !/*IDX.RDB$INDEX_INACTIVE*/
							 jrd_268.jrd_273 && !/*IND.RDB$INDEX_INACTIVE*/
     jrd_268.jrd_272)
				{
					idx->idx_primary_relation = partner_relation->rel_id;
					idx->idx_primary_index = /*IND.RDB$INDEX_ID*/
								 jrd_268.jrd_271 - 1;
					fb_assert(idx->idx_primary_index != idx_invalid);
					found = true;
				}
			/*END_FOR;*/
			   }
			}

			CMP_release(tdbb, request);
			return found;
		}

		frgn* references = &relation->rel_foreign_refs;
		if (references->frgn_reference_ids)
		{
			for (unsigned int index_number = 0;
				index_number < references->frgn_reference_ids->count();
				index_number++)
			{
				if (idx->idx_id == (*references->frgn_reference_ids)[index_number])
				{
					idx->idx_primary_relation = (*references->frgn_relations)[index_number];
					idx->idx_primary_index = (*references->frgn_indexes)[index_number];
					return true;
				}
			}
		}
		return false;
	}
	else if (idx->idx_flags & (idx_primary | idx_unique))
	{
		const prim* dependencies = &relation->rel_primary_dpnds;
		if (dependencies->prim_reference_ids)
		{
			for (unsigned int index_number = 0;
				 index_number < dependencies->prim_reference_ids->count();
				 index_number++)
			{
				if (idx->idx_id == (*dependencies->prim_reference_ids)[index_number])
				{
					idx->idx_foreign_primaries = relation->rel_primary_dpnds.prim_reference_ids;
					idx->idx_foreign_relations = relation->rel_primary_dpnds.prim_relations;
					idx->idx_foreign_indexes = relation->rel_primary_dpnds.prim_indexes;
					return true;
				}
			}
		}
		return false;
	}

	return false;
}


jrd_prc* MET_lookup_procedure(thread_db* tdbb, const Firebird::MetaName& name, bool noscan)
{
   struct {
          SSHORT jrd_261;	// gds__utility 
          SSHORT jrd_262;	// RDB$PROCEDURE_ID 
   } jrd_260;
   struct {
          TEXT  jrd_259 [32];	// RDB$PROCEDURE_NAME 
   } jrd_258;
/**************************************
 *
 *      M E T _ l o o k u p _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *      Lookup procedure by name.  Name passed in is
 *      ASCIZ name.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	jrd_prc* check_procedure = NULL;

/* See if we already know the procedure by name */
	vec<jrd_prc*>* procedures = dbb->dbb_procedures;
	if (procedures) {
		vec<jrd_prc*>::iterator ptr = procedures->begin();
		for (const vec<jrd_prc*>::const_iterator end = procedures->end(); ptr < end; ++ptr)
		{
			jrd_prc* procedure = *ptr;
			if (procedure && !(procedure->prc_flags & PRC_obsolete) &&
				((procedure->prc_flags & PRC_scanned) || noscan) &&
				!(procedure->prc_flags & PRC_being_scanned) &&
				!(procedure->prc_flags & PRC_being_altered))
			{
				if (procedure->prc_name == name)
				{
					if (procedure->prc_flags & PRC_check_existence)
					{
						check_procedure = procedure;
						LCK_lock(tdbb, check_procedure->prc_existence_lock, LCK_SR, LCK_WAIT);
						break;
					}

					return procedure;
				}
			}
		}
	}

/* We need to look up the procedure name in RDB$PROCEDURES */

	jrd_prc* procedure = NULL;

	jrd_req* request = CMP_find_request(tdbb, irq_l_procedure, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		P IN RDB$PROCEDURES WITH P.RDB$PROCEDURE_NAME EQ name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_257, sizeof(jrd_257), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_258.jrd_259, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_258);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_260);
	   if (!jrd_260.jrd_261) break;

		if (!REQUEST(irq_l_procedure))
			REQUEST(irq_l_procedure) = request;

		procedure = MET_procedure(tdbb, /*P.RDB$PROCEDURE_ID*/
						jrd_260.jrd_262, noscan, 0);

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_procedure))
		REQUEST(irq_l_procedure) = request;

	if (check_procedure) {
		check_procedure->prc_flags &= ~PRC_check_existence;
		if (check_procedure != procedure) {
			LCK_release(tdbb, check_procedure->prc_existence_lock);
			check_procedure->prc_flags |= PRC_obsolete;
		}
	}

	return procedure;
}


jrd_prc* MET_lookup_procedure_id(thread_db* tdbb, SSHORT id,
							bool return_deleted, bool noscan, USHORT flags)
{
   struct {
          SSHORT jrd_255;	// gds__utility 
          SSHORT jrd_256;	// RDB$PROCEDURE_ID 
   } jrd_254;
   struct {
          SSHORT jrd_253;	// RDB$PROCEDURE_ID 
   } jrd_252;
/**************************************
 *
 *      M E T _ l o o k u p _ p r o c e d u r e _ i d
 *
 **************************************
 *
 * Functional description
 *      Lookup procedure by id.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	jrd_prc* check_procedure = NULL;

	jrd_prc* procedure;

	vec<jrd_prc*>* procedures = dbb->dbb_procedures;
	if (procedures && id < (SSHORT) procedures->count() && (procedure = (*procedures)[id]) &&
		procedure->prc_id == id &&
		!(procedure->prc_flags & PRC_being_scanned) &&
		((procedure->prc_flags & PRC_scanned) || noscan) &&
		!(procedure->prc_flags & PRC_being_altered) &&
		(!(procedure->prc_flags & PRC_obsolete) || return_deleted))
	{
		if (procedure->prc_flags & PRC_check_existence) {
			check_procedure = procedure;
			LCK_lock(tdbb, check_procedure->prc_existence_lock, LCK_SR, LCK_WAIT);
		}
		else {
			return procedure;
		}
	}

/* We need to look up the procedure name in RDB$PROCEDURES */

	procedure = NULL;

	jrd_req* request = CMP_find_request(tdbb, irq_l_proc_id, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		P IN RDB$PROCEDURES WITH P.RDB$PROCEDURE_ID EQ id*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_251, sizeof(jrd_251), true);
	jrd_252.jrd_253 = id;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_252);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_254);
	   if (!jrd_254.jrd_255) break;

		if (!REQUEST(irq_l_proc_id))
			REQUEST(irq_l_proc_id) = request;

		procedure = MET_procedure(tdbb, /*P.RDB$PROCEDURE_ID*/
						jrd_254.jrd_256, noscan, flags);

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_l_proc_id))
		REQUEST(irq_l_proc_id) = request;

	if (check_procedure) {
		check_procedure->prc_flags &= ~PRC_check_existence;
		if (check_procedure != procedure) {
			LCK_release(tdbb, check_procedure->prc_existence_lock);
			check_procedure->prc_flags |= PRC_obsolete;
		}
	}

	return procedure;
}


jrd_rel* MET_lookup_relation(thread_db* tdbb, const Firebird::MetaName& name)
{
   struct {
          bid  jrd_238;	// RDB$VIEW_BLR 
          SSHORT jrd_239;	// gds__utility 
          SSHORT jrd_240;	// RDB$FLAGS 
          SSHORT jrd_241;	// RDB$RELATION_ID 
   } jrd_237;
   struct {
          TEXT  jrd_236 [32];	// RDB$RELATION_NAME 
   } jrd_235;
   struct {
          SSHORT jrd_246;	// gds__utility 
          SSHORT jrd_247;	// gds__null_flag 
          SSHORT jrd_248;	// RDB$RELATION_TYPE 
          SSHORT jrd_249;	// RDB$FLAGS 
          SSHORT jrd_250;	// RDB$RELATION_ID 
   } jrd_245;
   struct {
          TEXT  jrd_244 [32];	// RDB$RELATION_NAME 
   } jrd_243;
/**************************************
 *
 *      M E T _ l o o k u p _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *      Lookup relation by name.  Name passed in is
 *      ASCIZ name.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* See if we already know the relation by name */

	vec<jrd_rel*>* relations = dbb->dbb_relations;
	jrd_rel* check_relation = NULL;

	vec<jrd_rel*>::iterator ptr = relations->begin();
	for (const vec<jrd_rel*>::const_iterator end = relations->end(); ptr < end; ++ptr)
	{
		jrd_rel* relation = *ptr;

		if (relation)
		{
			if (relation->rel_flags & REL_deleting)
			{
				Database::CheckoutLockGuard guard(dbb, relation->rel_drop_mutex);
			}

			if (!(relation->rel_flags & REL_deleted))
			{
				/* dimitr: for non-system relations we should also check
						   REL_scanned and REL_being_scanned flags. Look
						   at MET_lookup_procedure for example. */
				if (!(relation->rel_flags & REL_system) &&
					(!(relation->rel_flags & REL_scanned) || (relation->rel_flags & REL_being_scanned)))
				{
					continue;
				}

				if (relation->rel_name == name)
				{
					if (relation->rel_flags & REL_check_existence)
					{
						check_relation = relation;
						LCK_lock(tdbb, check_relation->rel_existence_lock, LCK_SR, LCK_WAIT);
						break;
					}

					return relation;
				}
			}
		}
	}

/* We need to look up the relation name in RDB$RELATIONS */

	jrd_rel* relation = NULL;

	jrd_req* request = CMP_find_request(tdbb, irq_l_relation, IRQ_REQUESTS);

	if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
	{
		/*FOR(REQUEST_HANDLE request)
			X IN RDB$RELATIONS WITH X.RDB$RELATION_NAME EQ name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_242, sizeof(jrd_242), true);
		gds__vtov ((const char*) name.c_str(), (char*) jrd_243.jrd_244, 32);
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_243);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 10, (UCHAR*) &jrd_245);
		   if (!jrd_245.jrd_246) break;

			if (!REQUEST(irq_l_relation))
			{
				REQUEST(irq_l_relation) = request;
			}

			relation = MET_relation(tdbb, /*X.RDB$RELATION_ID*/
						      jrd_245.jrd_250);
			if (relation->rel_name.length() == 0) {
				relation->rel_name = name;
			}

			relation->rel_flags |= get_rel_flags_from_FLAGS(/*X.RDB$FLAGS*/
									jrd_245.jrd_249);

			if (!/*X.RDB$RELATION_TYPE.NULL*/
			     jrd_245.jrd_247)
			{
				relation->rel_flags |= MET_get_rel_flags_from_TYPE(/*X.RDB$RELATION_TYPE*/
										   jrd_245.jrd_248);
			}
		/*END_FOR;*/
		   }
		}
	}
	else
	{
		/*FOR(REQUEST_HANDLE request)
			X IN RDB$RELATIONS WITH X.RDB$RELATION_NAME EQ name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_234, sizeof(jrd_234), true);
		gds__vtov ((const char*) name.c_str(), (char*) jrd_235.jrd_236, 32);
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_235);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 14, (UCHAR*) &jrd_237);
		   if (!jrd_237.jrd_239) break;

			if (!REQUEST(irq_l_relation))
			{
				REQUEST(irq_l_relation) = request;
			}

			relation = MET_relation(tdbb, /*X.RDB$RELATION_ID*/
						      jrd_237.jrd_241);
			if (relation->rel_name.length() == 0) {
				relation->rel_name = name;
			}

			relation->rel_flags |= get_rel_flags_from_FLAGS(/*X.RDB$FLAGS*/
									jrd_237.jrd_240);

			if (!/*X.RDB$VIEW_BLR*/
			     jrd_237.jrd_238.isEmpty())
			{
				relation->rel_flags |= REL_jrd_view;
			}
		/*END_FOR;*/
		   }
		}
	}

	if (!REQUEST(irq_l_relation))
	{
		REQUEST(irq_l_relation) = request;
	}

	if (check_relation) {
		check_relation->rel_flags &= ~REL_check_existence;
		if (check_relation != relation) {
			LCK_release(tdbb, check_relation->rel_existence_lock);
			LCK_release(tdbb, check_relation->rel_partners_lock);
			check_relation->rel_flags &= ~REL_check_partners;
			check_relation->rel_flags |= REL_deleted;
		}
	}

	return relation;
}


jrd_rel* MET_lookup_relation_id(thread_db* tdbb, SLONG id, bool return_deleted)
{
   struct {
          bid  jrd_219;	// RDB$VIEW_BLR 
          TEXT  jrd_220 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_221;	// gds__utility 
          SSHORT jrd_222;	// RDB$FLAGS 
          SSHORT jrd_223;	// RDB$RELATION_ID 
   } jrd_218;
   struct {
          SSHORT jrd_217;	// RDB$RELATION_ID 
   } jrd_216;
   struct {
          TEXT  jrd_228 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_229;	// gds__utility 
          SSHORT jrd_230;	// gds__null_flag 
          SSHORT jrd_231;	// RDB$RELATION_TYPE 
          SSHORT jrd_232;	// RDB$FLAGS 
          SSHORT jrd_233;	// RDB$RELATION_ID 
   } jrd_227;
   struct {
          SSHORT jrd_226;	// RDB$RELATION_ID 
   } jrd_225;
/**************************************
 *
 *      M E T _ l o o k u p _ r e l a t i o n _ i d
 *
 **************************************
 *
 * Functional description
 *      Lookup relation by id. Make sure it really exists.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* System relations are above suspicion */

	if (id <= (int) dbb->dbb_max_sys_rel)
	{
		fb_assert(id < MAX_USHORT);
		return MET_relation(tdbb, (USHORT) id);
	}

	jrd_rel* check_relation = NULL;
	jrd_rel* relation;
	vec<jrd_rel*>* vector = dbb->dbb_relations;
	if (vector && (id < (SLONG) vector->count()) && (relation = (*vector)[id]))
	{
		if (relation->rel_flags & REL_deleting)
		{
			Database::CheckoutLockGuard guard(dbb, relation->rel_drop_mutex);
		}

		if (relation->rel_flags & REL_deleted)
		{
			return return_deleted ? relation : NULL;
		}

		if (relation->rel_flags & REL_check_existence)
		{
			check_relation = relation;
			LCK_lock(tdbb, check_relation->rel_existence_lock, LCK_SR, LCK_WAIT);
		}
		else
		{
			return relation;
		}
	}

/* We need to look up the relation id in RDB$RELATIONS */

	relation = NULL;

	jrd_req* request = CMP_find_request(tdbb, irq_l_rel_id, IRQ_REQUESTS);

	if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
	{
		/*FOR(REQUEST_HANDLE request)
			X IN RDB$RELATIONS WITH X.RDB$RELATION_ID EQ id*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_224, sizeof(jrd_224), true);
		jrd_225.jrd_226 = id;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_225);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 42, (UCHAR*) &jrd_227);
		   if (!jrd_227.jrd_229) break;

			if (!REQUEST(irq_l_rel_id))
				REQUEST(irq_l_rel_id) = request;

			relation = MET_relation(tdbb, /*X.RDB$RELATION_ID*/
						      jrd_227.jrd_233);
			if (relation->rel_name.length() == 0) {
				relation->rel_name = /*X.RDB$RELATION_NAME*/
						     jrd_227.jrd_228;
			}

			relation->rel_flags |= get_rel_flags_from_FLAGS(/*X.RDB$FLAGS*/
									jrd_227.jrd_232);

			if (!/*X.RDB$RELATION_TYPE.NULL*/
			     jrd_227.jrd_230)
			{
				relation->rel_flags |= MET_get_rel_flags_from_TYPE(/*X.RDB$RELATION_TYPE*/
										   jrd_227.jrd_231);
			}
		/*END_FOR;*/
		   }
		}
	}
	else
	{
		/*FOR(REQUEST_HANDLE request)
			X IN RDB$RELATIONS WITH X.RDB$RELATION_ID EQ id*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_215, sizeof(jrd_215), true);
		jrd_216.jrd_217 = id;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_216);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 46, (UCHAR*) &jrd_218);
		   if (!jrd_218.jrd_221) break;

			if (!REQUEST(irq_l_rel_id))
				REQUEST(irq_l_rel_id) = request;

			relation = MET_relation(tdbb, /*X.RDB$RELATION_ID*/
						      jrd_218.jrd_223);
			if (relation->rel_name.length() == 0) {
				relation->rel_name = /*X.RDB$RELATION_NAME*/
						     jrd_218.jrd_220;
			}

			relation->rel_flags |= get_rel_flags_from_FLAGS(/*X.RDB$FLAGS*/
									jrd_218.jrd_222);

			if (!/*X.RDB$VIEW_BLR*/
			     jrd_218.jrd_219.isEmpty())
			{
				relation->rel_flags |= REL_jrd_view;
			}
		/*END_FOR;*/
		   }
		}
	}

	if (!REQUEST(irq_l_rel_id))
		REQUEST(irq_l_rel_id) = request;

	if (check_relation) {
		check_relation->rel_flags &= ~REL_check_existence;
		if (check_relation != relation) {
			LCK_release(tdbb, check_relation->rel_existence_lock);
			LCK_release(tdbb, check_relation->rel_partners_lock);
			check_relation->rel_flags &= ~REL_check_partners;
			check_relation->rel_flags |= REL_deleted;
		}
	}

	return relation;
}


jrd_nod* MET_parse_blob(thread_db*	tdbb,
				   jrd_rel*		relation,
				   bid*			blob_id,
				   AutoPtr<CompilerScratch>& csb,
				   jrd_req**	request_ptr,
				   const bool	trigger)
{
/**************************************
 *
 *      M E T _ p a r s e _ b l o b
 *
 **************************************
 *
 * Functional description
 *      Parse blr, returning a compiler scratch block with the results.
 *
 * if ignore_perm is true then, the request generated must be set to
 *   ignore all permissions checks. In this case, we call PAR_blr
 *   passing it the csb_ignore_perm flag to generate a request
 *   which must go through without checking any permissions.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	blb* blob = BLB_open(tdbb, dbb->dbb_sys_trans, blob_id);
	SLONG length = blob->blb_length + 10;
	Firebird::HalfStaticArray<UCHAR, 512> tmp;
	UCHAR* temp = tmp.getBuffer(length);
	length = BLB_get_data(tdbb, blob, temp, length);

	jrd_nod* node = PAR_blr(tdbb, relation, temp, length, NULL, csb, request_ptr, trigger, 0);

	return node;
}


void MET_parse_sys_trigger(thread_db* tdbb, jrd_rel* relation)
{
   struct {
          bid  jrd_210;	// RDB$TRIGGER_BLR 
          TEXT  jrd_211 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_212;	// gds__utility 
          SSHORT jrd_213;	// RDB$FLAGS 
          SSHORT jrd_214;	// RDB$TRIGGER_TYPE 
   } jrd_209;
   struct {
          TEXT  jrd_208 [32];	// RDB$RELATION_NAME 
   } jrd_207;
/**************************************
 *
 *      M E T _ p a r s e _ s y s _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *      Parse the blr for a system relation's triggers.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	relation->rel_flags &= ~REL_sys_triggers;

	// Release any triggers in case of a rescan

	if (relation->rel_pre_store)
		MET_release_triggers(tdbb, &relation->rel_pre_store);
	if (relation->rel_post_store)
		MET_release_triggers(tdbb, &relation->rel_post_store);
	if (relation->rel_pre_erase)
		MET_release_triggers(tdbb, &relation->rel_pre_erase);
	if (relation->rel_post_erase)
		MET_release_triggers(tdbb, &relation->rel_post_erase);
	if (relation->rel_pre_modify)
		MET_release_triggers(tdbb, &relation->rel_pre_modify);
	if (relation->rel_post_modify)
		MET_release_triggers(tdbb, &relation->rel_post_modify);

	// No need to load triggers for ReadOnly databases, since
	// INSERT/DELETE/UPDATE statements are not going to be allowed
	// hvlad: GTT with ON COMMIT DELETE ROWS clause is writable

	if ((dbb->dbb_flags & DBB_read_only) && !(relation->rel_flags & REL_temp_tran))
		return;

	relation->rel_flags |= REL_sys_trigs_being_loaded;

	jrd_req* trigger_request = CMP_find_request(tdbb, irq_s_triggers2, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE trigger_request)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME = relation->rel_name.c_str()
		AND TRG.RDB$SYSTEM_FLAG = 1*/
	{
	if (!trigger_request)
	   trigger_request = CMP_compile2 (tdbb, (UCHAR*) jrd_206, sizeof(jrd_206), true);
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_207.jrd_208, 32);
	EXE_start (tdbb, trigger_request, dbb->dbb_sys_trans);
	EXE_send (tdbb, trigger_request, 0, 32, (UCHAR*) &jrd_207);
	while (1)
	   {
	   EXE_receive (tdbb, trigger_request, 1, 46, (UCHAR*) &jrd_209);
	   if (!jrd_209.jrd_212) break;

		if (!REQUEST (irq_s_triggers2))
			REQUEST (irq_s_triggers2) = trigger_request;

		const UCHAR type = (UCHAR) /*TRG.RDB$TRIGGER_TYPE*/
					   jrd_209.jrd_214;
		const USHORT trig_flags = /*TRG.RDB$FLAGS*/
					  jrd_209.jrd_213;
		const TEXT* name = /*TRG.RDB$TRIGGER_NAME*/
				   jrd_209.jrd_211;

		trig_vec** ptr;

		switch (type)
		{
		case 1:
			ptr = &relation->rel_pre_store;
			break;
		case 2:
			ptr = &relation->rel_post_store;
			break;
		case 3:
			ptr = &relation->rel_pre_modify;
			break;
		case 4:
			ptr = &relation->rel_post_modify;
			break;
		case 5:
			ptr = &relation->rel_pre_erase;
			break;
		case 6:
			ptr = &relation->rel_post_erase;
			break;
		default:
			ptr = NULL;
			break;
		}

		if (ptr)
		{
			blb* blob = BLB_open(tdbb, dbb->dbb_sys_trans, &/*TRG.RDB$TRIGGER_BLR*/
									jrd_209.jrd_210);
			SLONG length = blob->blb_length + 10;
			Firebird::HalfStaticArray<UCHAR, 128> blr;
			length = BLB_get_data(tdbb, blob, blr.getBuffer(length), length);

			USHORT par_flags = (USHORT) ((trig_flags & TRG_ignore_perm) ? csb_ignore_perm : 0);
			if (type & 1)
				par_flags |= csb_pre_trigger;
			else
				par_flags |= csb_post_trigger;

			jrd_req* request = NULL;
			{
				Jrd::ContextPoolHolder context(tdbb, dbb->createPool());
				PAR_blr(tdbb, relation, blr.begin(), length, NULL,
					&request, true, par_flags);
			}

			request->req_trg_name = name;

			request->req_flags |= req_sys_trigger;
			if (trig_flags & TRG_ignore_perm)
			{
				request->req_flags |= req_ignore_perm;
			}

			save_trigger_data(tdbb, ptr, relation, request, NULL, NULL, NULL, type, true, 0);
		}

	/*END_FOR;*/
	   }
	}

	if (!REQUEST (irq_s_triggers2))
		REQUEST (irq_s_triggers2) = trigger_request;

	relation->rel_flags &= ~REL_sys_trigs_being_loaded;
}


void MET_post_existence(thread_db* tdbb, jrd_rel* relation)
{
/**************************************
 *
 *      M E T _ p o s t _ e x i s t e n c e
 *
 **************************************
 *
 * Functional description
 *      Post an interest in the existence of a relation.
 *
 **************************************/
	SET_TDBB(tdbb);

	relation->rel_use_count++;

	if (!MET_lookup_relation_id(tdbb, relation->rel_id, false))
	{
		relation->rel_use_count--;
		ERR_post(Arg::Gds(isc_relnotdef) << Arg::Str(relation->rel_name));
	}
}


void MET_prepare(thread_db* tdbb, jrd_tra* transaction, USHORT length, const UCHAR* msg)
{
   struct {
          bid  jrd_203;	// RDB$TRANSACTION_DESCRIPTION 
          SLONG  jrd_204;	// RDB$TRANSACTION_ID 
          SSHORT jrd_205;	// RDB$TRANSACTION_STATE 
   } jrd_202;
/**************************************
 *
 *      M E T _ p r e p a r e
 *
 **************************************
 *
 * Functional description
 *      Post a transaction description to RDB$TRANSACTIONS.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, irq_s_trans, IRQ_REQUESTS);

	/*STORE(REQUEST_HANDLE request) X IN RDB$TRANSACTIONS*/
	{
	
		/*X.RDB$TRANSACTION_ID*/
		jrd_202.jrd_204 = transaction->tra_number;
		/*X.RDB$TRANSACTION_STATE*/
		jrd_202.jrd_205 = /*RDB$TRANSACTIONS.RDB$TRANSACTION_STATE.LIMBO*/
   1;
		blb* blob = BLB_create(tdbb, dbb->dbb_sys_trans, &/*X.RDB$TRANSACTION_DESCRIPTION*/
								  jrd_202.jrd_203);
		BLB_put_segment(tdbb, blob, msg, length);
		BLB_close(tdbb, blob);
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_201, sizeof(jrd_201), true);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 14, (UCHAR*) &jrd_202);
	}

	if (!REQUEST(irq_s_trans))
		REQUEST(irq_s_trans) = request;
}


jrd_prc* MET_procedure(thread_db* tdbb, int id, bool noscan, USHORT flags)
{
   struct {
          SSHORT jrd_149;	// gds__utility 
   } jrd_148;
   struct {
          SSHORT jrd_146;	// gds__null_flag 
          SSHORT jrd_147;	// RDB$VALID_BLR 
   } jrd_145;
   struct {
          SSHORT jrd_142;	// gds__utility 
          SSHORT jrd_143;	// gds__null_flag 
          SSHORT jrd_144;	// RDB$VALID_BLR 
   } jrd_141;
   struct {
          SSHORT jrd_140;	// RDB$PROCEDURE_ID 
   } jrd_139;
   struct {
          bid  jrd_154;	// RDB$DEBUG_INFO 
          SSHORT jrd_155;	// gds__utility 
          SSHORT jrd_156;	// gds__null_flag 
          SSHORT jrd_157;	// gds__null_flag 
          SSHORT jrd_158;	// RDB$PROCEDURE_TYPE 
   } jrd_153;
   struct {
          SSHORT jrd_152;	// RDB$PROCEDURE_ID 
   } jrd_151;
   struct {
          bid  jrd_164;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_165;	// gds__utility 
          SSHORT jrd_166;	// gds__null_flag 
          SSHORT jrd_167;	// gds__null_flag 
          SSHORT jrd_168;	// RDB$COLLATION_ID 
   } jrd_163;
   struct {
          TEXT  jrd_161 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_162 [32];	// RDB$PROCEDURE_NAME 
   } jrd_160;
   struct {
          bid  jrd_173;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_174 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_175 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_176 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_177;	// gds__utility 
          SSHORT jrd_178;	// gds__null_flag 
          SSHORT jrd_179;	// RDB$COLLATION_ID 
          SSHORT jrd_180;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_181;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_182;	// RDB$FIELD_LENGTH 
          SSHORT jrd_183;	// RDB$FIELD_SCALE 
          SSHORT jrd_184;	// RDB$FIELD_TYPE 
          SSHORT jrd_185;	// RDB$PARAMETER_NUMBER 
          SSHORT jrd_186;	// RDB$PARAMETER_TYPE 
   } jrd_172;
   struct {
          TEXT  jrd_171 [32];	// RDB$PROCEDURE_NAME 
   } jrd_170;
   struct {
          bid  jrd_191;	// RDB$PROCEDURE_BLR 
          TEXT  jrd_192 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_193 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_194;	// gds__utility 
          SSHORT jrd_195;	// RDB$PROCEDURE_OUTPUTS 
          SSHORT jrd_196;	// RDB$PROCEDURE_INPUTS 
          SSHORT jrd_197;	// gds__null_flag 
          SSHORT jrd_198;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_199;	// gds__null_flag 
          SSHORT jrd_200;	// RDB$PROCEDURE_ID 
   } jrd_190;
   struct {
          SSHORT jrd_189;	// RDB$PROCEDURE_ID 
   } jrd_188;
/**************************************
 *
 *      M E T _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *      Find or create a procedure block for a given procedure id.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	vec<jrd_prc*>* vector = dbb->dbb_procedures;
	if (!vector)
	{
		vector = dbb->dbb_procedures = vec<jrd_prc*>::newVector(*dbb->dbb_permanent, id + 10);
	}
	else if (id >= (int) vector->count())
	{
		vector->resize(id + 10);
	}

	Database::CheckoutLockGuard guard(dbb, dbb->dbb_meta_mutex);

	jrd_prc* procedure = (*vector)[id];

	if (procedure && !(procedure->prc_flags & PRC_obsolete))
	{
		// Make sure PRC_being_scanned and PRC_scanned are not set at the same time

		fb_assert(!(procedure->prc_flags & PRC_being_scanned) || !(procedure->prc_flags & PRC_scanned));

		/* To avoid scanning recursive procedure's blr recursively let's
		   make use of PRC_being_scanned bit. Because this bit is set
		   later in the code, it is not set when we are here first time.
		   If (in case of rec. procedure) we get here second time it is
		   already set and we return half baked procedure.
		   In case of superserver this code is under the rec. mutex
		   protection, thus the only guy (thread) who can get here and see
		   PRC_being_scanned bit set is the guy which started procedure scan
		   and currently holds the mutex.
		   In case of classic, there is always only one guy and if it
		   sees PRC_being_scanned bit set it is safe to assume it is here
		   second time.

		   If procedure has already been scanned - return. This condition
		   is for those threads that did not find procedure in cache and
		   came here to get it from disk. But only one was able to lock
		   the mutex and do the scanning, others were waiting. As soon as
		   the first thread releases the mutex another thread gets in and
		   it would be just unfair to make it do the work again.
		 */

		if ((procedure->prc_flags & PRC_being_scanned) || (procedure->prc_flags & PRC_scanned))
		{
			return procedure;
		}
	}

	if (!procedure)
	{
		procedure = FB_NEW(*dbb->dbb_permanent) jrd_prc(*dbb->dbb_permanent);
	}

	try {

	procedure->prc_flags |= (PRC_being_scanned | flags);
	procedure->prc_flags &= ~PRC_obsolete;

	procedure->prc_id = id;
	(*vector)[id] = procedure;

	if (!procedure->prc_existence_lock) {
		Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, 0) Lock;
		procedure->prc_existence_lock = lock;
		lock->lck_parent = dbb->dbb_lock;
		lock->lck_dbb = dbb;
		lock->lck_key.lck_long = procedure->prc_id;
		lock->lck_length = sizeof(lock->lck_key.lck_long);
		lock->lck_type = LCK_prc_exist;
		lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
		lock->lck_object = procedure;
		lock->lck_ast = blocking_ast_procedure;
	}

	LCK_lock(tdbb, procedure->prc_existence_lock, LCK_SR, LCK_WAIT);

	if (!noscan)
	{
		// CVC: Where is this set to false? I made it const to draw attention here.
		const bool valid_blr = true;

		jrd_req* request = CMP_find_request(tdbb, irq_r_procedure, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			P IN RDB$PROCEDURES WITH P.RDB$PROCEDURE_ID EQ procedure->prc_id*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_187, sizeof(jrd_187), true);
		jrd_188.jrd_189 = procedure->prc_id;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_188);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 86, (UCHAR*) &jrd_190);
		   if (!jrd_190.jrd_194) break;

	        if (!REQUEST(irq_r_procedure))
			{
	            REQUEST(irq_r_procedure) = request;
			}

			if (procedure->prc_name.length() == 0)
			{
				procedure->prc_name = /*P.RDB$PROCEDURE_NAME*/
						      jrd_190.jrd_193;
			}
			procedure->prc_id = /*P.RDB$PROCEDURE_ID*/
					    jrd_190.jrd_200;
			if (!/*P.RDB$SECURITY_CLASS.NULL*/
			     jrd_190.jrd_199)
			{
				procedure->prc_security_name = /*P.RDB$SECURITY_CLASS*/
							       jrd_190.jrd_192;
			}
			if (/*P.RDB$SYSTEM_FLAG.NULL*/
			    jrd_190.jrd_197 || !/*P.RDB$SYSTEM_FLAG*/
     jrd_190.jrd_198)
			{
				procedure->prc_flags &= ~PRC_system;
			}
			else
			{
				procedure->prc_flags |= PRC_system;
			}
			if ( (procedure->prc_inputs = /*P.RDB$PROCEDURE_INPUTS*/
						      jrd_190.jrd_196) )
			{
				procedure->prc_input_fields =
					vec<Parameter*>::newVector(*dbb->dbb_permanent, procedure->prc_input_fields,
								   /*P.RDB$PROCEDURE_INPUTS*/
								   jrd_190.jrd_196);
			}
			if ( (procedure->prc_outputs = /*P.RDB$PROCEDURE_OUTPUTS*/
						       jrd_190.jrd_195) )
			{
				procedure->prc_output_fields =
					vec<Parameter*>::newVector(*dbb->dbb_permanent, procedure->prc_output_fields,
								   /*P.RDB$PROCEDURE_OUTPUTS*/
								   jrd_190.jrd_195);
			}

			vec<Parameter*>* paramVector = 0;
			procedure->prc_defaults = 0;

			jrd_req* request2 = CMP_find_request(tdbb, irq_r_params, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE request2)
				PA IN RDB$PROCEDURE_PARAMETERS CROSS
				F IN RDB$FIELDS WITH F.RDB$FIELD_NAME = PA.RDB$FIELD_SOURCE
					AND PA.RDB$PROCEDURE_NAME = P.RDB$PROCEDURE_NAME*/
			{
			if (!request2)
			   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_169, sizeof(jrd_169), true);
			gds__vtov ((const char*) jrd_190.jrd_193, (char*) jrd_170.jrd_171, 32);
			EXE_start (tdbb, request2, dbb->dbb_sys_trans);
			EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_170);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 124, (UCHAR*) &jrd_172);
			   if (!jrd_172.jrd_177) break;

	            if (!REQUEST(irq_r_params))
				{
					REQUEST(irq_r_params) = request2;
				}

				SSHORT pa_collation_id_null = true;
				SSHORT pa_collation_id = 0;
				SSHORT pa_default_value_null = true;
				bid pa_default_value;

				if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
				{
					jrd_req* request3 = CMP_find_request(tdbb, irq_r_params2, IRQ_REQUESTS);

					/*FOR(REQUEST_HANDLE request3)
						PA2 IN RDB$PROCEDURE_PARAMETERS WITH
							PA2.RDB$PROCEDURE_NAME EQ PA.RDB$PROCEDURE_NAME AND
							PA2.RDB$PARAMETER_NAME EQ PA.RDB$PARAMETER_NAME*/
					{
					if (!request3)
					   request3 = CMP_compile2 (tdbb, (UCHAR*) jrd_159, sizeof(jrd_159), true);
					gds__vtov ((const char*) jrd_172.jrd_175, (char*) jrd_160.jrd_161, 32);
					gds__vtov ((const char*) jrd_172.jrd_176, (char*) jrd_160.jrd_162, 32);
					EXE_start (tdbb, request3, dbb->dbb_sys_trans);
					EXE_send (tdbb, request3, 0, 64, (UCHAR*) &jrd_160);
					while (1)
					   {
					   EXE_receive (tdbb, request3, 1, 16, (UCHAR*) &jrd_163);
					   if (!jrd_163.jrd_165) break;

						if (!REQUEST(irq_r_params2))
							REQUEST(irq_r_params2) = request3;

						pa_collation_id_null = /*PA2.RDB$COLLATION_ID.NULL*/
								       jrd_163.jrd_167;
						pa_collation_id = /*PA2.RDB$COLLATION_ID*/
								  jrd_163.jrd_168;
						pa_default_value_null = /*PA2.RDB$DEFAULT_VALUE.NULL*/
									jrd_163.jrd_166;
						pa_default_value = /*PA2.RDB$DEFAULT_VALUE*/
								   jrd_163.jrd_164;
					/*END_FOR*/
					   }
					}

					if (!REQUEST(irq_r_params2))
						REQUEST(irq_r_params2) = request3;
				}

				if (/*PA.RDB$PARAMETER_TYPE*/
				    jrd_172.jrd_186) {
					paramVector = procedure->prc_output_fields;
				}
				else {
					paramVector = procedure->prc_input_fields;
				}

				// should be error if field already exists
				Parameter* parameter = FB_NEW(*dbb->dbb_permanent) Parameter(*dbb->dbb_permanent);
				parameter->prm_number = /*PA.RDB$PARAMETER_NUMBER*/
							jrd_172.jrd_185;
				(*paramVector)[parameter->prm_number] = parameter;
				parameter->prm_name = /*PA.RDB$PARAMETER_NAME*/
						      jrd_172.jrd_175;

				DSC_make_descriptor(&parameter->prm_desc, /*F.RDB$FIELD_TYPE*/
									  jrd_172.jrd_184,
									/*F.RDB$FIELD_SCALE*/
									jrd_172.jrd_183, /*F.RDB$FIELD_LENGTH*/
  jrd_172.jrd_182,
									/*F.RDB$FIELD_SUB_TYPE*/
									jrd_172.jrd_181, /*F.RDB$CHARACTER_SET_ID*/
  jrd_172.jrd_180,
									(pa_collation_id_null ? /*F.RDB$COLLATION_ID*/
												jrd_172.jrd_179 : pa_collation_id));

				if (/*PA.RDB$PARAMETER_TYPE*/
				    jrd_172.jrd_186 == 0 &&
					(!pa_default_value_null ||
					 (fb_utils::implicit_domain(/*F.RDB$FIELD_NAME*/
								    jrd_172.jrd_174) && !/*F.RDB$DEFAULT_VALUE.NULL*/
      jrd_172.jrd_178)))
				{
					procedure->prc_defaults++;
					MemoryPool* pool = dbb->createPool();
					Jrd::ContextPoolHolder context(tdbb, pool);

					if (pa_default_value_null)
						pa_default_value = /*F.RDB$DEFAULT_VALUE*/
								   jrd_172.jrd_173;

					try
					{
						AutoPtr<CompilerScratch> csb;
						parameter->prm_default_value =
							MET_parse_blob(tdbb, NULL, &pa_default_value, csb, NULL, false);
					}
					catch(const Exception&)
					{
						// Here we lose pools created for previous defaults.
						// Probably we should use common pool for defaults and procedure itself.

						dbb->deletePool(pool);
						throw;
					}
				}

			/*END_FOR;*/
			   }
			}

			if (!REQUEST(irq_r_params)) {
				REQUEST(irq_r_params) = request2;
			}

			if ((paramVector = procedure->prc_output_fields) && (*paramVector)[0])
			{
				Format* format = procedure->prc_format =
					Format::newFormat(*dbb->dbb_permanent, procedure->prc_outputs);
				ULONG length = FLAG_BYTES(format->fmt_count);
				Format::fmt_desc_iterator desc = format->fmt_desc.begin();
				vec<Parameter*>::iterator ptr, end;
				for (ptr = paramVector->begin(), end = paramVector->end(); ptr < end; ++ptr, ++desc)
				{
					Parameter* parameter = *ptr;
					// check for parameter to be null, this can only happen if the
					// parameter numbers get out of sync. This was added to fix bug
					// 10534. -Shaunak Mistry 12-May-99
					if (parameter)
					{
						*desc = parameter->prm_desc;
						length = MET_align(dbb, &(*desc), length);
						desc->dsc_address = (UCHAR *) (IPTR) length;
						length += desc->dsc_length;
					}
				}
				format->fmt_length = (USHORT) length;
			}

			prc_t prc_type = prc_legacy;

			MemoryPool* csb_pool = dbb->createPool();

			{
				Jrd::ContextPoolHolder context(tdbb, csb_pool);
				AutoPtr<CompilerScratch> csb(CompilerScratch::newCsb(*dbb->dbb_permanent, 5));

				if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
				{
					jrd_req* request4 = CMP_find_request(tdbb, irq_p_type, IRQ_REQUESTS);

					/*FOR(REQUEST_HANDLE request4)
						PT IN RDB$PROCEDURES
						WITH PT.RDB$PROCEDURE_ID EQ procedure->prc_id*/
					{
					if (!request4)
					   request4 = CMP_compile2 (tdbb, (UCHAR*) jrd_150, sizeof(jrd_150), true);
					jrd_151.jrd_152 = procedure->prc_id;
					EXE_start (tdbb, request4, dbb->dbb_sys_trans);
					EXE_send (tdbb, request4, 0, 2, (UCHAR*) &jrd_151);
					while (1)
					   {
					   EXE_receive (tdbb, request4, 1, 16, (UCHAR*) &jrd_153);
					   if (!jrd_153.jrd_155) break;

						if (!REQUEST(irq_p_type))
						{
							REQUEST(irq_p_type) = request4;
						}

						if (!/*PT.RDB$PROCEDURE_TYPE.NULL*/
						     jrd_153.jrd_157)
							prc_type = (prc_t) /*PT.RDB$PROCEDURE_TYPE*/
									   jrd_153.jrd_158;

						if (!/*PT.RDB$DEBUG_INFO.NULL*/
						     jrd_153.jrd_156)
							DBG_parse_debug_info(tdbb, &/*PT.RDB$DEBUG_INFO*/
										    jrd_153.jrd_154, csb->csb_dbg_info);

					/*END_FOR;*/
					   }
					}

					if (!REQUEST(irq_p_type)) {
						REQUEST(irq_p_type) = request4;
					}
				}

				procedure->prc_type = prc_type;

				try
				{
					parse_procedure_blr(tdbb, procedure, &/*P.RDB$PROCEDURE_BLR*/
									      jrd_190.jrd_191, csb);
				}
				catch (const Firebird::Exception&)
				{
					if (procedure->prc_request) {
						MET_release_procedure_request(tdbb, procedure);
					}
					else {
						dbb->deletePool(csb_pool);
					}

					ERR_post(Arg::Gds(isc_bad_proc_BLR) << Arg::Str(procedure->prc_name));
				}

				procedure->prc_request->req_procedure = procedure;
				for (size_t i = 0; i < csb->csb_rpt.getCount(); i++)
				{
					jrd_nod* node = csb->csb_rpt[i].csb_message;
					if (node)
					{
						/*
						if ((int) (IPTR) node->nod_arg[e_msg_number] == 0)
						{
							// Never used.
							procedure->prc_input_msg = node;
						}
						else
						*/
						if ((int) (IPTR) node->nod_arg[e_msg_number] == 1)
						{
							procedure->prc_output_msg = node;
						}
					}
				}
			}

		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_r_procedure)) {
			REQUEST(irq_r_procedure) = request;
		}

		procedure->prc_flags |= PRC_scanned;

		// CVC: This condition is always false because valid_blr is always true.
		if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1 &&
			!(dbb->dbb_flags & DBB_read_only) && !valid_blr)
		{
			// if the BLR was marked as invalid but the procedure was compiled,
			// mark the BLR as valid

			jrd_req* request5 = NULL;

			/*FOR(REQUEST_HANDLE request5)
				P IN RDB$PROCEDURES WITH P.RDB$PROCEDURE_ID EQ procedure->prc_id*/
			{
			if (!request5)
			   request5 = CMP_compile2 (tdbb, (UCHAR*) jrd_138, sizeof(jrd_138), true);
			jrd_139.jrd_140 = procedure->prc_id;
			EXE_start (tdbb, request5, dbb->dbb_sys_trans);
			EXE_send (tdbb, request5, 0, 2, (UCHAR*) &jrd_139);
			while (1)
			   {
			   EXE_receive (tdbb, request5, 1, 6, (UCHAR*) &jrd_141);
			   if (!jrd_141.jrd_142) break;

				/*MODIFY P USING*/
				{
				
					/*P.RDB$VALID_BLR*/
					jrd_141.jrd_144 = TRUE;
					/*P.RDB$VALID_BLR.NULL*/
					jrd_141.jrd_143 = FALSE;
				/*END_MODIFY;*/
				jrd_145.jrd_146 = jrd_141.jrd_143;
				jrd_145.jrd_147 = jrd_141.jrd_144;
				EXE_send (tdbb, request5, 2, 4, (UCHAR*) &jrd_145);
				}
			/*END_FOR;*/
			   EXE_send (tdbb, request5, 3, 2, (UCHAR*) &jrd_148);
			   }
			}

			CMP_release(tdbb, request5);
		}
	} // if !noscan

	// Make sure that it is really being scanned
	fb_assert(procedure->prc_flags & PRC_being_scanned);

	procedure->prc_flags &= ~PRC_being_scanned;

	}	// try
	catch (const Firebird::Exception&) {
		procedure->prc_flags &= ~(PRC_being_scanned | PRC_scanned);
		if (procedure->prc_existence_lock)
		{
			LCK_release(tdbb, procedure->prc_existence_lock);
			procedure->prc_existence_lock = NULL;
		}
		throw;
	}

	return procedure;
}


jrd_rel* MET_relation(thread_db* tdbb, USHORT id)
{
/**************************************
 *
 *      M E T _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *      Find or create a relation block for a given relation id.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	vec<jrd_rel*>* vector = dbb->dbb_relations;

	if (!vector)
	{
		vector = dbb->dbb_relations = vec<jrd_rel*>::newVector(*dbb->dbb_permanent, id + 10);
	}
	else if (id >= vector->count())
	{
		vector->resize(id + 10);
	}

	jrd_rel* relation = (*vector)[id];
	if (relation) {
		return relation;
	}

	relation = FB_NEW(*dbb->dbb_permanent) jrd_rel(*dbb->dbb_permanent);
	(*vector)[id] = relation;
	relation->rel_id = id;

	{ // Scope block.
		Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, 0) Lock;
		relation->rel_partners_lock = lock;
		lock->lck_parent = dbb->dbb_lock;
		lock->lck_dbb = dbb;
		lock->lck_key.lck_long = relation->rel_id;
		lock->lck_length = sizeof(lock->lck_key.lck_long);
		lock->lck_type = LCK_rel_partners;
		lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
		lock->lck_object = relation;
		lock->lck_ast = partners_ast_relation;
	}

	// This should check system flag instead.
	if (relation->rel_id <= dbb->dbb_max_sys_rel) {
		return relation;
	}

	{ // Scope block.
		Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, 0) Lock;
		relation->rel_existence_lock = lock;
		lock->lck_parent = dbb->dbb_lock;
		lock->lck_dbb = dbb;
		lock->lck_key.lck_long = relation->rel_id;
		lock->lck_length = sizeof(lock->lck_key.lck_long);
		lock->lck_type = LCK_rel_exist;
		lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
		lock->lck_object = relation;
		lock->lck_ast = blocking_ast_relation;
	}

	relation->rel_flags |= (REL_check_existence | REL_check_partners);
	return relation;
}


void MET_release_existence(thread_db* tdbb, jrd_rel* relation)
{
/**************************************
 *
 *      M E T _ r e l e a s e _ e x i s t e n c e
 *
 **************************************
 *
 * Functional description
 *      Release interest in relation. If no remaining interest
 *      and we're blocking the drop of the relation then release
 *      existence lock and mark deleted.
 *
 **************************************/
	if (relation->rel_use_count) {
		relation->rel_use_count--;
	}

	if (!relation->rel_use_count) {
		if (relation->rel_flags & REL_blocking) {
			LCK_re_post(tdbb, relation->rel_existence_lock);
		}

		if (relation->rel_file) {
			// close external file
			EXT_fini(relation, true);
		}
	}
}


void MET_remove_procedure(thread_db* tdbb, int id, jrd_prc* procedure)
{
/**************************************
 *
 *      M E T _ r e m o v e  _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *      Remove a procedure from cache
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	vec<jrd_prc*>* pvector = dbb->dbb_procedures;
	if (!pvector) {
		return;
	}

	if (!procedure) {
	/** If we are in here then dfw.epp/modify_procedure() called us **/
		if (!(procedure = (*pvector)[id]))
			return;
	}

	/* MET_procedure locks it. Lets unlock it now to avoid troubles later */
	if (procedure->prc_existence_lock)
	{
		LCK_release(tdbb, procedure->prc_existence_lock);
	}

/* Procedure that is being altered may have references
   to it by other procedures via pointer to current meta
   data structure, so don't loose the structure or the pointer. */
	if ((procedure == (*pvector)[id]) && !(procedure->prc_flags & PRC_being_altered))
	{
	    (*pvector)[id] = NULL;
	}

/* deallocate all structure which were allocated.  The procedure
 * blr is originally read into a new pool from which all request
 * are allocated.  That will not be freed up.
 */

	if (procedure->prc_existence_lock) {
		delete procedure->prc_existence_lock;
		procedure->prc_existence_lock = NULL;
	}

/* deallocate input param structures */
	vec<Parameter*>* vector;
	if ((procedure->prc_inputs) && (vector = procedure->prc_input_fields)) {
		for (int i = 0; i < procedure->prc_inputs; i++)
		{
			if ((*vector)[i])
			{
				delete (*vector)[i];
			}
		}
		delete vector;
		procedure->prc_inputs = 0;
		procedure->prc_input_fields = NULL;
	}

/* deallocate output param structures */

	if ((procedure->prc_outputs) && (vector = procedure->prc_output_fields)) {
		for (int i = 0; i < procedure->prc_outputs; i++)
		{
			if ((*vector)[i])
			{
				delete (*vector)[i];
			}
		}
		delete vector;
		procedure->prc_outputs = 0;
		procedure->prc_output_fields = NULL;
	}

	if (!procedure->prc_use_count)
	{
		if (procedure->prc_format)
		{
			delete procedure->prc_format;
			procedure->prc_format = NULL;
		}
	}

	if (!(procedure->prc_flags & PRC_being_altered) && !procedure->prc_use_count)
	{
		delete procedure;
	}
	else
	{
		// Fully clear procedure block. Some pieces of code check for empty
		// procedure name and ID, this is why we do it.
		procedure->prc_name = "";
		procedure->prc_security_name = "";
		procedure->prc_defaults = 0;
		procedure->prc_id = 0;
	}
}


void MET_revoke(thread_db* tdbb,
				jrd_tra* transaction,
				const TEXT* relation,
				const TEXT* revokee,
				const TEXT* privilege)
{
   struct {
          SSHORT jrd_130;	// gds__utility 
   } jrd_129;
   struct {
          SSHORT jrd_128;	// gds__utility 
   } jrd_127;
   struct {
          SSHORT jrd_126;	// gds__utility 
   } jrd_125;
   struct {
          TEXT  jrd_122 [32];	// RDB$GRANTOR 
          TEXT  jrd_123 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_124 [7];	// RDB$PRIVILEGE 
   } jrd_121;
   struct {
          SSHORT jrd_137;	// gds__utility 
   } jrd_136;
   struct {
          TEXT  jrd_133 [32];	// RDB$USER 
          TEXT  jrd_134 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_135 [7];	// RDB$PRIVILEGE 
   } jrd_132;
/**************************************
 *
 *      M E T _ r e v o k e
 *
 **************************************
 *
 * Functional description
 *      Execute a recursive revoke.  This is called only when
 *      a revoked privilege had the grant option.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* See if the revokee still has the privilege.  If so, there's
   nothing to do */

	USHORT count = 0;

	jrd_req* request = CMP_find_request(tdbb, irq_revoke1, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FIRST 1 P IN RDB$USER_PRIVILEGES WITH
			P.RDB$RELATION_NAME EQ relation AND
			P.RDB$PRIVILEGE EQ privilege AND
			P.RDB$USER EQ revokee*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_131, sizeof(jrd_131), true);
	gds__vtov ((const char*) revokee, (char*) jrd_132.jrd_133, 32);
	gds__vtov ((const char*) relation, (char*) jrd_132.jrd_134, 32);
	gds__vtov ((const char*) privilege, (char*) jrd_132.jrd_135, 7);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 71, (UCHAR*) &jrd_132);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_136);
	   if (!jrd_136.jrd_137) break;

		if (!REQUEST(irq_revoke1))
			REQUEST(irq_revoke1) = request;
		++count;
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_revoke1))
		REQUEST(irq_revoke1) = request;

	if (count)
		return;

	request = CMP_find_request(tdbb, irq_revoke2, IRQ_REQUESTS);

/* User lost privilege.  Take it away from anybody he/she gave
   it to. */

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		P IN RDB$USER_PRIVILEGES WITH
			P.RDB$RELATION_NAME EQ relation AND
			P.RDB$PRIVILEGE EQ privilege AND
			P.RDB$GRANTOR EQ revokee*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_120, sizeof(jrd_120), true);
	gds__vtov ((const char*) revokee, (char*) jrd_121.jrd_122, 32);
	gds__vtov ((const char*) relation, (char*) jrd_121.jrd_123, 32);
	gds__vtov ((const char*) privilege, (char*) jrd_121.jrd_124, 7);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 71, (UCHAR*) &jrd_121);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_125);
	   if (!jrd_125.jrd_126) break;

		if (!REQUEST(irq_revoke2))
			REQUEST(irq_revoke2) = request;
		/*ERASE P;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_127);
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_129);
	   }
	}

	if (!REQUEST(irq_revoke2))
		REQUEST(irq_revoke2) = request;
}


void MET_scan_relation(thread_db* tdbb, jrd_rel* relation)
{
   struct {
          TEXT  jrd_101 [32];	// RDB$DEFAULT_CLASS 
          bid  jrd_102;	// RDB$RUNTIME 
          TEXT  jrd_103 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_104;	// RDB$VIEW_BLR 
          TEXT  jrd_105 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_106 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_107 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_108;	// gds__utility 
          SSHORT jrd_109;	// gds__null_flag 
          SSHORT jrd_110;	// gds__null_flag 
          SSHORT jrd_111;	// RDB$FIELD_ID 
          SSHORT jrd_112;	// RDB$FORMAT 
   } jrd_100;
   struct {
          SSHORT jrd_99;	// RDB$RELATION_ID 
   } jrd_98;
   struct {
          SSHORT jrd_117;	// gds__utility 
          SSHORT jrd_118;	// gds__null_flag 
          SSHORT jrd_119;	// RDB$RELATION_TYPE 
   } jrd_116;
   struct {
          SSHORT jrd_115;	// RDB$RELATION_ID 
   } jrd_114;
/**************************************
 *
 *      M E T _ s c a n _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *      Scan a relation for view RecordSelExpr, computed by expressions, missing
 *      expressions, and validation expressions.
 *
 **************************************/
	SET_TDBB(tdbb);
	trig_vec* triggers[TRIGGER_MAX];
	Database* dbb = tdbb->getDatabase();
	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_permanent);
	bool dependencies = false;
	bool sys_triggers = false;

	blb* blob = NULL;
	jrd_req* request = NULL;

	jrd_tra* depTrans = (tdbb->getTransaction() ? tdbb->getTransaction() : dbb->dbb_sys_trans);

/* If anything errors, catch it to reset the scan flag.  This will
   make sure that the error will be caught if the operation is tried
   again. */

	try {

	Database::CheckoutLockGuard guard(dbb, dbb->dbb_meta_mutex);

	if (relation->rel_flags & REL_scanned || relation->rel_flags & REL_deleted)
	{
		return;
	}

	relation->rel_flags |= REL_being_scanned;
	dependencies = (relation->rel_flags & REL_get_dependencies) ? true : false;
	sys_triggers = (relation->rel_flags & REL_sys_triggers) ? true : false;
	relation->rel_flags &= ~(REL_get_dependencies | REL_sys_triggers);

	for (USHORT itr = 0; itr < TRIGGER_MAX; ++itr) {
		triggers[itr] = NULL;
	}

	rel_t relType = rel_persistent;

	if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
	{
		jrd_req* sub_request = CMP_find_request(tdbb, irq_r_type, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE sub_request)
			REL IN RDB$RELATIONS WITH REL.RDB$RELATION_ID EQ relation->rel_id*/
		{
		if (!sub_request)
		   sub_request = CMP_compile2 (tdbb, (UCHAR*) jrd_113, sizeof(jrd_113), true);
		jrd_114.jrd_115 = relation->rel_id;
		EXE_start (tdbb, sub_request, dbb->dbb_sys_trans);
		EXE_send (tdbb, sub_request, 0, 2, (UCHAR*) &jrd_114);
		while (1)
		   {
		   EXE_receive (tdbb, sub_request, 1, 6, (UCHAR*) &jrd_116);
		   if (!jrd_116.jrd_117) break;

			if (!REQUEST(irq_r_type))
				REQUEST(irq_r_type) = sub_request;

			if (!/*REL.RDB$RELATION_TYPE.NULL*/
			     jrd_116.jrd_118)
			{
				relType = (rel_t) /*REL.RDB$RELATION_TYPE*/
						  jrd_116.jrd_119;
			}

		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_r_type))
			REQUEST(irq_r_type) = sub_request;
	}

	switch (relType)
	{
		case rel_persistent:
		case rel_external:
			break;
		case rel_view:
			fb_assert(relation->rel_flags & REL_jrd_view);
			relation->rel_flags |= REL_jrd_view;
			break;
		case rel_virtual:
			fb_assert(relation->rel_flags & REL_virtual);
			relation->rel_flags |= REL_virtual;
			break;
		case rel_global_temp_preserve:
			fb_assert(relation->rel_flags & REL_temp_conn);
			relation->rel_flags |= REL_temp_conn;
			break;
		case rel_global_temp_delete:
			fb_assert(relation->rel_flags & REL_temp_tran);
			relation->rel_flags |= REL_temp_tran;
			break;
		default:
			fb_assert(false);
	}

/* Since this can be called recursively, find an inactive clone of the request */

	request = CMP_find_request(tdbb, irq_r_fields, IRQ_REQUESTS);
	AutoPtr<CompilerScratch> csb;

	/*FOR(REQUEST_HANDLE request)
		REL IN RDB$RELATIONS WITH REL.RDB$RELATION_ID EQ relation->rel_id*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_97, sizeof(jrd_97), true);
	jrd_98.jrd_99 = relation->rel_id;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_98);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 410, (UCHAR*) &jrd_100);
	   if (!jrd_100.jrd_108) break;
			/* Pick up relation level stuff */
		if (!REQUEST(irq_r_fields)) {
			REQUEST(irq_r_fields) = request;
		}
		relation->rel_current_fmt = /*REL.RDB$FORMAT*/
					    jrd_100.jrd_112;
		vec<jrd_fld*>* vector = relation->rel_fields =
			vec<jrd_fld*>::newVector(*dbb->dbb_permanent, relation->rel_fields, /*REL.RDB$FIELD_ID*/
											    jrd_100.jrd_111 + 1);
		if (!/*REL.RDB$SECURITY_CLASS.NULL*/
		     jrd_100.jrd_110) {
			relation->rel_security_name = /*REL.RDB$SECURITY_CLASS*/
						      jrd_100.jrd_107;
		}

		relation->rel_name = /*REL.RDB$RELATION_NAME*/
				     jrd_100.jrd_106;
		relation->rel_owner_name = /*REL.RDB$OWNER_NAME*/
					   jrd_100.jrd_105;

		if (!/*REL.RDB$VIEW_BLR*/
		     jrd_100.jrd_104.isEmpty())
		{
			/* parse the view blr, getting dependencies on relations, etc. at the same time */

			if (dependencies)
			{
				const Firebird::MetaName depName(/*REL.RDB$RELATION_NAME*/
								 jrd_100.jrd_106);
				relation->rel_view_rse =
					(RecordSelExpr*) MET_get_dependencies(tdbb, relation, NULL, 0, NULL,
											&/*REL.RDB$VIEW_BLR*/
											 jrd_100.jrd_104, NULL,
											csb, depName, obj_view, 0, depTrans);
			}
			else
			{
				relation->rel_view_rse =
					(RecordSelExpr*) MET_parse_blob(tdbb, relation,
											&/*REL.RDB$VIEW_BLR*/
											 jrd_100.jrd_104, csb, NULL, false);
			}

			/* retrieve the view context names */

			lookup_view_contexts(tdbb, relation);
		}

		relation->rel_flags |= REL_scanned;
		if (/*REL.RDB$EXTERNAL_FILE*/
		    jrd_100.jrd_103[0])
		{
			EXT_file(relation, /*REL.RDB$EXTERNAL_FILE*/
					   jrd_100.jrd_103); //, &REL.RDB$EXTERNAL_DESCRIPTION);
		}

		/* Pick up field specific stuff */

		blob = BLB_open(tdbb, dbb->dbb_sys_trans, &/*REL.RDB$RUNTIME*/
							   jrd_100.jrd_102);
		Firebird::HalfStaticArray<UCHAR, 256> temp;
		UCHAR* const buffer = temp.getBuffer(blob->blb_max_segment + 1);

		jrd_fld* field = NULL;
		ArrayField* array = 0;
		USHORT view_context = 0;
		USHORT field_id = 0;
		for (;;)
		{
			USHORT length = BLB_get_segment(tdbb, blob, buffer, blob->blb_max_segment);
			if (blob->blb_flags & BLB_eof)
			{
				break;
			}
			USHORT n;
			buffer[length] = 0;
			UCHAR* p = (UCHAR*) &n;
			const UCHAR* q = buffer + 1;
			while (q < buffer + 1 + sizeof(SSHORT))
			{
				*p++ = *q++;
			}
			p = buffer + 1;
			--length;
			switch ((rsr_t) buffer[0])
			{
			case RSR_field_id:
				if (field && field->fld_security_name.length() == 0 && !/*REL.RDB$DEFAULT_CLASS.NULL*/
											jrd_100.jrd_109)
				{
					field->fld_security_name = /*REL.RDB$DEFAULT_CLASS*/
								   jrd_100.jrd_101;
				}
				field_id = n;
				field = (*vector)[field_id];

				if (field) {
					field->fld_computation = NULL;
					field->fld_missing_value = NULL;
					field->fld_default_value = NULL;
					field->fld_validation = NULL;
					field->fld_not_null = NULL;
				}

				array = NULL;
				break;

			case RSR_field_name:
				if (field) {
					/* The field exists.  If its name hasn't changed, then
					   there's no need to copy anything. */

					if (field->fld_name == reinterpret_cast<char*>(p))
						break;

					field->fld_name = reinterpret_cast<char*>(p);
				}
				else {
					field = FB_NEW(*dbb->dbb_permanent) jrd_fld(*dbb->dbb_permanent);
					(*vector)[field_id] = field;
					field->fld_name = reinterpret_cast<char*>(p);
				}

				// CVC: Be paranoid and allow the possible trigger(s) to have a
				//   not null security class to work on, even if we only take it
				//   from the relation itself.
				if (field->fld_security_name.length() == 0 && !/*REL.RDB$DEFAULT_CLASS.NULL*/
									       jrd_100.jrd_109)
				{
					field->fld_security_name = /*REL.RDB$DEFAULT_CLASS*/
								   jrd_100.jrd_101;
				}

				break;

			case RSR_view_context:
				view_context = n;
				break;

			case RSR_base_field:
				if (dependencies) {
					csb->csb_g_flags |= csb_get_dependencies;
					field->fld_source = PAR_make_field(tdbb, csb, view_context, (TEXT*) p);
					const Firebird::MetaName depName(/*REL.RDB$RELATION_NAME*/
									 jrd_100.jrd_106);
					store_dependencies(tdbb, csb, 0, depName, obj_view, depTrans);
				}
				else {
					field->fld_source = PAR_make_field(tdbb, csb, view_context, (TEXT*) p);
				}
				break;

			case RSR_computed_blr:
				field->fld_computation = dependencies ?
					MET_get_dependencies(tdbb, relation, p, length, csb, NULL, NULL,
										 field->fld_name, obj_computed, 0, depTrans) :
					PAR_blr(tdbb, relation, p, length, csb, NULL, false, 0);
				break;

			case RSR_missing_value:
				field->fld_missing_value = PAR_blr(tdbb, relation, p, length, csb,
					NULL, false, 0);
				break;

			case RSR_default_value:
				field->fld_default_value = PAR_blr(tdbb, relation, p, length, csb,
					NULL, false, 0);
				break;

			case RSR_validation_blr:
				// AB: 2005-04-25   bug SF#1168898
				// Ignore validation for VIEWs, because fields (domains) which are
				// defined with CHECK constraints and have sub-selects should at least
				// be parsed without view-context information. With view-context
				// information the context-numbers are wrong.
				// Because a VIEW can't have a validation section i ignored the whole call.
				if (!csb)
				{
					field->fld_validation = PAR_blr(tdbb, relation, p, length, csb,
						NULL, false, csb_validation);
				}
				break;

			case RSR_field_not_null:
				field->fld_not_null = PAR_blr(tdbb, relation, p, length, csb,
					NULL, false, csb_validation);
				break;

			case RSR_security_class:
				field->fld_security_name = (const TEXT*) p;
				break;

			case RSR_trigger_name:
				MET_load_trigger(tdbb, relation, (const TEXT*) p, triggers);
				break;

			case RSR_dimensions:
				field->fld_array = array = FB_NEW_RPT(*dbb->dbb_permanent, n) ArrayField();
				array->arr_desc.iad_dimensions = n;
				break;

			case RSR_array_desc:
				if (array)
					memcpy(&array->arr_desc, p, length);
				break;

			default:    /* Shut up compiler warning */
				break;
			}
		}
		BLB_close(tdbb, blob);
		blob = 0;
		if (field && field->fld_security_name.length() == 0 && !/*REL.RDB$DEFAULT_CLASS.NULL*/
									jrd_100.jrd_109)
		{
			field->fld_security_name = /*REL.RDB$DEFAULT_CLASS*/
						   jrd_100.jrd_101;
		}
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_r_fields))
		REQUEST(irq_r_fields) = request;

	switch (relType)
	{
		case rel_persistent:
			break;
		case rel_external:
			fb_assert(relation->rel_file);
			break;
		case rel_view:
			fb_assert(relation->rel_view_rse);
			break;
		case rel_virtual:
		case rel_global_temp_preserve:
		case rel_global_temp_delete:
			break;
		default:
			fb_assert(false);
	}

/* release any triggers in case of a rescan, but not if the rescan
   hapenned while system triggers were being loaded. */

	if (!(relation->rel_flags & REL_sys_trigs_being_loaded)) {
		/* if we are scanning a system relation during loading the system
		   triggers, (during parsing its blr actually), we must not release the
		   existing system triggers; because we have already set the
		   relation->rel_flag to not have REL_sys_trig, so these
		   system triggers will not get loaded again. This fixes bug 8149. */

		/* We have just loaded the triggers onto the local vector triggers.
		   Its now time to place them at their rightful place ie the relation
		   block.
		 */
		trig_vec* tmp_vector;

		tmp_vector = relation->rel_pre_store;
		relation->rel_pre_store = triggers[TRIGGER_PRE_STORE];
		MET_release_triggers(tdbb, &tmp_vector);

		tmp_vector = relation->rel_post_store;
		relation->rel_post_store = triggers[TRIGGER_POST_STORE];
		MET_release_triggers(tdbb, &tmp_vector);

		tmp_vector = relation->rel_pre_erase;
		relation->rel_pre_erase = triggers[TRIGGER_PRE_ERASE];
		MET_release_triggers(tdbb, &tmp_vector);

		tmp_vector = relation->rel_post_erase;
		relation->rel_post_erase = triggers[TRIGGER_POST_ERASE];
		MET_release_triggers(tdbb, &tmp_vector);

		tmp_vector = relation->rel_pre_modify;
		relation->rel_pre_modify = triggers[TRIGGER_PRE_MODIFY];
		MET_release_triggers(tdbb, &tmp_vector);

		tmp_vector = relation->rel_post_modify;
		relation->rel_post_modify = triggers[TRIGGER_POST_MODIFY];
		MET_release_triggers(tdbb, &tmp_vector);
	}

	relation->rel_flags &= ~REL_being_scanned;

	relation->rel_current_format = NULL;

	}	// try
	catch (const Firebird::Exception&) {
		relation->rel_flags &= ~(REL_being_scanned | REL_scanned);
		if (dependencies) {
			relation->rel_flags |= REL_get_dependencies;
		}
		if (sys_triggers) {
			relation->rel_flags |= REL_sys_triggers;
		}
		if (blob)
			BLB_close(tdbb, blob);

		// Some functions inside FOR loop may throw, in which case request
		// remained active forever. AP: 13-may-05.
		if (request && request->req_flags & req_active)
		{
			EXE_unwind(tdbb, request);
		}
		throw;
	}
}


void MET_trigger_msg(thread_db* tdbb, Firebird::string& msg, const Firebird::MetaName& name, USHORT number)
{
   struct {
          TEXT  jrd_95 [1024];	// RDB$MESSAGE 
          SSHORT jrd_96;	// gds__utility 
   } jrd_94;
   struct {
          TEXT  jrd_92 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_93;	// RDB$MESSAGE_NUMBER 
   } jrd_91;
/**************************************
 *
 *      M E T _ t r i g g e r _ m s g
 *
 **************************************
 *
 * Functional description
 *      Look up trigger message using trigger and abort code.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, irq_s_msgs, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		MSG IN RDB$TRIGGER_MESSAGES WITH
			MSG.RDB$TRIGGER_NAME EQ name.c_str() AND
			MSG.RDB$MESSAGE_NUMBER EQ number*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_90, sizeof(jrd_90), true);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_91.jrd_92, 32);
	jrd_91.jrd_93 = number;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_91);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 1026, (UCHAR*) &jrd_94);
	   if (!jrd_94.jrd_96) break;

		if (!REQUEST(irq_s_msgs))
			REQUEST(irq_s_msgs) = request;
		msg = /*MSG.RDB$MESSAGE*/
		      jrd_94.jrd_95;
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_s_msgs))
		REQUEST(irq_s_msgs) = request;

	msg.rtrim();
}


void MET_update_shadow(thread_db* tdbb, Shadow* shadow, USHORT file_flags)
{
   struct {
          SSHORT jrd_89;	// gds__utility 
   } jrd_88;
   struct {
          SSHORT jrd_87;	// RDB$FILE_FLAGS 
   } jrd_86;
   struct {
          SSHORT jrd_84;	// gds__utility 
          SSHORT jrd_85;	// RDB$FILE_FLAGS 
   } jrd_83;
   struct {
          SSHORT jrd_82;	// RDB$SHADOW_NUMBER 
   } jrd_81;
/**************************************
 *
 *      M E T _ u p d a t e _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *      Update the stored file flags for the specified shadow.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* handle = NULL;
	/*FOR(REQUEST_HANDLE handle)
		FIL IN RDB$FILES WITH FIL.RDB$SHADOW_NUMBER EQ shadow->sdw_number*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_80, sizeof(jrd_80), true);
	jrd_81.jrd_82 = shadow->sdw_number;
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_81);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_83);
	   if (!jrd_83.jrd_84) break;
		/*MODIFY FIL USING*/
		{
		
			/*FIL.RDB$FILE_FLAGS*/
			jrd_83.jrd_85 = file_flags;
		/*END_MODIFY;*/
		jrd_86.jrd_87 = jrd_83.jrd_85;
		EXE_send (tdbb, handle, 2, 2, (UCHAR*) &jrd_86);
		}
	/*END_FOR;*/
	   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_88);
	   }
	}
	CMP_release(tdbb, handle);
}


void MET_update_transaction(thread_db* tdbb, jrd_tra* transaction, const bool do_commit)
{
   struct {
          SSHORT jrd_79;	// gds__utility 
   } jrd_78;
   struct {
          SSHORT jrd_77;	// gds__utility 
   } jrd_76;
   struct {
          SSHORT jrd_75;	// RDB$TRANSACTION_STATE 
   } jrd_74;
   struct {
          SSHORT jrd_72;	// gds__utility 
          SSHORT jrd_73;	// RDB$TRANSACTION_STATE 
   } jrd_71;
   struct {
          SLONG  jrd_70;	// RDB$TRANSACTION_ID 
   } jrd_69;
/**************************************
 *
 *      M E T _ u p d a t e _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *      Update a record in RDB$TRANSACTIONS.  If do_commit is true, this is a
 *      commit; otherwise it is a ROLLBACK.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, irq_m_trans, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$TRANSACTIONS
		WITH X.RDB$TRANSACTION_ID EQ transaction->tra_number*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_68, sizeof(jrd_68), true);
	jrd_69.jrd_70 = transaction->tra_number;
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_69);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_71);
	   if (!jrd_71.jrd_72) break;

		if (!REQUEST(irq_m_trans))
			REQUEST(irq_m_trans) = request;
		if (do_commit && (transaction->tra_flags & TRA_prepare2))
		{
			/*ERASE X*/
			EXE_send (tdbb, request, 4, 2, (UCHAR*) &jrd_78);
		}
		else
		{
			/*MODIFY X*/
			{
			
				/*X.RDB$TRANSACTION_STATE*/
				jrd_71.jrd_73 = do_commit ?
					/*RDB$TRANSACTIONS.RDB$TRANSACTION_STATE.COMMITTED*/
					2 :
					/*RDB$TRANSACTIONS.RDB$TRANSACTION_STATE.ROLLED_BACK*/
					3;
			/*END_MODIFY;*/
			jrd_74.jrd_75 = jrd_71.jrd_73;
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_74);
			}
		}
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_76);
	   }
	}

	if (!REQUEST(irq_m_trans))
		REQUEST(irq_m_trans) = request;
}


static int blocking_ast_dsql_cache(void* ast_object)
{
/**************************************
 *
 *	b l o c k i n g _ a s t _ d s q l _ c a c h e
 *
 **************************************
 *
 * Functional description
 *	Someone is trying to drop an item from the DSQL cache.
 *	Mark the symbol as obsolete and release the lock.
 *
 **************************************/
	DSqlCacheItem* item = static_cast<DSqlCacheItem*>(ast_object);

	try
	{
		Database* dbb = item->lock->lck_dbb;

		Database::SyncGuard dsGuard(dbb, true);

		ThreadContextHolder tdbb;
		tdbb->setDatabase(dbb);
		tdbb->setAttachment(item->lock->lck_attachment);

		Jrd::ContextPoolHolder context(tdbb, 0);

		item->obsolete = true;
		item->locked = false;
		LCK_release(tdbb, item->lock);
	}
	catch (const Firebird::Exception&)
	{} // no-op

	return 0;
}


static DSqlCacheItem* get_dsql_cache_item(thread_db* tdbb, int type, const Firebird::MetaName& name)
{
	Database* dbb = tdbb->getDatabase();
	Attachment* attachment = tdbb->getAttachment();

	Firebird::string key((char*) &type, sizeof(type));
	key.append(name.c_str());

	DSqlCacheItem* item = attachment->att_dsql_cache.put(key);
	if (item)
	{
		item->obsolete = false;
		item->locked = false;
		item->lock = FB_NEW_RPT(*dbb->dbb_permanent, key.length()) Lock();

		item->lock->lck_type = LCK_dsql_cache;
		item->lock->lck_owner_handle = LCK_get_owner_handle(tdbb, item->lock->lck_type);
		item->lock->lck_parent = dbb->dbb_lock;
		item->lock->lck_dbb = dbb;
		item->lock->lck_object = item;
		item->lock->lck_ast = blocking_ast_dsql_cache;
		item->lock->lck_length = key.length();
		memcpy(item->lock->lck_key.lck_string, key.c_str(), key.length());
	}
	else
	{
		item = attachment->att_dsql_cache.get(key);
	}

	return item;
}


static int blocking_ast_procedure(void* ast_object)
{
/**************************************
 *
 *      b l o c k i n g _ a s t _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *      Someone is trying to drop a proceedure. If there
 *      are outstanding interests in the existence of
 *      the relation then just mark as blocking and return.
 *      Otherwise, mark the procedure block as questionable
 *      and release the procedure existence lock.
 *
 **************************************/
	jrd_prc* procedure = static_cast<jrd_prc*>(ast_object);

	try
	{
		Database* dbb = procedure->prc_existence_lock->lck_dbb;

		Database::SyncGuard dsGuard(dbb, true);

		ThreadContextHolder tdbb;
		tdbb->setDatabase(dbb);
		tdbb->setAttachment(procedure->prc_existence_lock->lck_attachment);

		Jrd::ContextPoolHolder context(tdbb, 0);

		if (procedure->prc_existence_lock) {
			LCK_release(tdbb, procedure->prc_existence_lock);
		}
		procedure->prc_flags |= PRC_obsolete;
	}
	catch (const Firebird::Exception&)
	{} // no-op

	return 0;
}


static int blocking_ast_relation(void* ast_object)
{
/**************************************
 *
 *      b l o c k i n g _ a s t _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *      Someone is trying to drop a relation. If there
 *      are outstanding interests in the existence of
 *      the relation then just mark as blocking and return.
 *      Otherwise, mark the relation block as questionable
 *      and release the relation existence lock.
 *
 **************************************/
	jrd_rel* relation = static_cast<jrd_rel*>(ast_object);

	try
	{
		Database* dbb = relation->rel_existence_lock->lck_dbb;

		Database::SyncGuard dsGuard(dbb, true);

		ThreadContextHolder tdbb;
		tdbb->setDatabase(dbb);
		tdbb->setAttachment(relation->rel_existence_lock->lck_attachment);

		Jrd::ContextPoolHolder context(tdbb, 0);

		if (relation->rel_use_count) {
			relation->rel_flags |= REL_blocking;
		}
		else {
			relation->rel_flags &= ~REL_blocking;
			relation->rel_flags |= REL_check_existence;
			if (relation->rel_existence_lock)
				LCK_release(tdbb, relation->rel_existence_lock);
		}
	}
	catch (const Firebird::Exception&)
	{} // no-op

	return 0;
}


static int partners_ast_relation(void* ast_object)
{
	jrd_rel* relation = static_cast<jrd_rel*>(ast_object);

	try
	{
		Database* dbb = relation->rel_partners_lock->lck_dbb;

		Database::SyncGuard dsGuard(dbb, true);

		ThreadContextHolder tdbb;
		tdbb->setDatabase(dbb);
		tdbb->setAttachment(relation->rel_partners_lock->lck_attachment);

		Jrd::ContextPoolHolder context(tdbb, 0);

		LCK_release(tdbb, relation->rel_partners_lock);
		relation->rel_flags |= REL_check_partners;
	}
	catch (const Firebird::Exception&)
	{} // no-op

	return 0;
}


static ULONG get_rel_flags_from_FLAGS(USHORT flags)
{
/**************************************
 *
 *      g e t _ r e l _ f l a g s _ f r o m _ F L A G S
 *
 **************************************
 *
 * Functional description
 *      Get rel_flags from RDB$FLAGS
 *
 **************************************/
	ULONG ret = 0;

	if (flags & REL_sql) {
		ret |= REL_sql_relation;
	}

	return ret;
}


ULONG MET_get_rel_flags_from_TYPE(USHORT type)
{
/**************************************
 *
 *      M E T _ g e t _ r e l _ f l a g s _ f r o m _ T Y P E
 *
 **************************************
 *
 * Functional description
 *      Get rel_flags from RDB$RELATION_TYPE
 *
 **************************************/
	ULONG ret = 0;

	switch (type)
	{
		case rel_persistent:
			break;
		case rel_external:
			break;
		case rel_view:
			ret |= REL_jrd_view;
			break;
		case rel_virtual:
			ret |= REL_virtual;
			break;
		case rel_global_temp_preserve:
			ret |= REL_temp_conn;
			break;
		case rel_global_temp_delete:
			ret |= REL_temp_tran;
			break;
		default:
			fb_assert(false);
	}

	return ret;
}


static void get_trigger(thread_db* tdbb, jrd_rel* relation,
						bid* blob_id, bid* debug_blob_id, trig_vec** ptr,
						const TEXT* name, UCHAR type,
						bool sys_trigger, USHORT flags)
{
/**************************************
 *
 *      g e t _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *      Get trigger.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (blob_id->isEmpty())
		return;

	Database* dbb = tdbb->getDatabase();
	blb* blrBlob = BLB_open(tdbb, dbb->dbb_sys_trans, blob_id);
	save_trigger_data(tdbb, ptr, relation, NULL, blrBlob, debug_blob_id,
					  name, type, sys_trigger, flags);
}


static bool get_type(thread_db* tdbb, USHORT* id, const UCHAR* name, const TEXT* field)
{
   struct {
          SSHORT jrd_66;	// gds__utility 
          SSHORT jrd_67;	// RDB$TYPE 
   } jrd_65;
   struct {
          TEXT  jrd_63 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_64 [32];	// RDB$FIELD_NAME 
   } jrd_62;
/**************************************
 *
 *      g e t _ t y p e
 *
 **************************************
 *
 * Functional description
 *      Resoved a symbolic name in RDB$TYPES.  Returned the value
 *      defined for the name in (*id).  Don't touch (*id) if you
 *      don't find the name.
 *
 *      Return (1) if found, (0) otherwise.
 *
 **************************************/
	UCHAR buffer[MAX_SQL_IDENTIFIER_SIZE];			/* BASED ON RDB$TYPE_NAME */

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	fb_assert(id != NULL);
	fb_assert(name != NULL);
	fb_assert(field != NULL);

/* Force key to uppercase, following C locale rules for uppercase */
	UCHAR* p;
	for (p = buffer; *name && p < buffer + sizeof(buffer) - 1; p++, name++)
	{
		*p = UPPER7(*name);
	}
	*p = 0;

/* Try for exact name match */

	bool found = false;

	jrd_req* handle = NULL;
	/*FOR(REQUEST_HANDLE handle)
		FIRST 1 T IN RDB$TYPES WITH
			T.RDB$FIELD_NAME EQ field AND
			T.RDB$TYPE_NAME EQ buffer*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_61, sizeof(jrd_61), true);
	gds__vtov ((const char*) buffer, (char*) jrd_62.jrd_63, 32);
	gds__vtov ((const char*) field, (char*) jrd_62.jrd_64, 32);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_62);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_65);
	   if (!jrd_65.jrd_66) break;

		found = true;
		*id = /*T.RDB$TYPE*/
		      jrd_65.jrd_67;
	/*END_FOR;*/
	   }
	}
	CMP_release(tdbb, handle);

	return found;
}


static void lookup_view_contexts( thread_db* tdbb, jrd_rel* view)
{
   struct {
          TEXT  jrd_57 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_58 [256];	// RDB$CONTEXT_NAME 
          SSHORT jrd_59;	// gds__utility 
          SSHORT jrd_60;	// RDB$VIEW_CONTEXT 
   } jrd_56;
   struct {
          TEXT  jrd_55 [32];	// RDB$VIEW_NAME 
   } jrd_54;
/**************************************
 *
 *      l o o k u p _ v i e w _ c o n t e x t s
 *
 **************************************
 *
 * Functional description
 *      Lookup view contexts and store in a linked
 *      list on the relation block.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	jrd_req* request = CMP_find_request(tdbb, irq_view_context, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		V IN RDB$VIEW_RELATIONS WITH
			V.RDB$VIEW_NAME EQ view->rel_name.c_str()
			SORTED BY V.RDB$VIEW_CONTEXT*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_53, sizeof(jrd_53), true);
	gds__vtov ((const char*) view->rel_name.c_str(), (char*) jrd_54.jrd_55, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_54);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 292, (UCHAR*) &jrd_56);
	   if (!jrd_56.jrd_59) break;

		if (!REQUEST(irq_view_context))
			REQUEST(irq_view_context) = request;

		// trim trailing spaces
		fb_utils::exact_name_limit(/*V.RDB$CONTEXT_NAME*/
					   jrd_56.jrd_58, sizeof(/*V.RDB$CONTEXT_NAME*/
	 jrd_56.jrd_58));

		bool isProcedure = MET_lookup_procedure(tdbb, /*V.RDB$RELATION_NAME*/
							      jrd_56.jrd_57, true) != NULL;

		ViewContext* view_context = FB_NEW(*dbb->dbb_permanent)
			ViewContext(*dbb->dbb_permanent, /*V.RDB$CONTEXT_NAME*/
							 jrd_56.jrd_58, /*V.RDB$RELATION_NAME*/
  jrd_56.jrd_57,
				/*V.RDB$VIEW_CONTEXT*/
				jrd_56.jrd_60, !isProcedure);

		view->rel_view_contexts.add(view_context);

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_view_context))
		REQUEST(irq_view_context) = request;
}


static void make_relation_scope_name(const TEXT* rel_name,
	const USHORT rel_flags, Firebird::string& str)
{
/**************************************
 *
 *	m a k e _ r e l a t i o n _ s c o p e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Make string with relation name and type
 *	of its temporary scope
 *
 **************************************/
	const char *scope = NULL;
	if (rel_flags & REL_temp_conn)
		scope = REL_SCOPE_GTT_PRESERVE;
	else if (rel_flags & REL_temp_tran)
		scope = REL_SCOPE_GTT_DELETE;
	else
		scope = REL_SCOPE_PERSISTENT;

	str.printf(scope, rel_name);
}


// ***********************************************************
// *    p a r s e _ f i e l d _ b l r
// ***********************************************************
// Parses default BLR and validation BLR for a field.
static jrd_nod* parse_field_blr(thread_db* tdbb, bid* blob_id, const Firebird::MetaName name)
{
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	AutoPtr<CompilerScratch> csb(CompilerScratch::newCsb(*dbb->dbb_permanent, 5, name));

	blb* blob = BLB_open(tdbb, dbb->dbb_sys_trans, blob_id);
	SLONG length = blob->blb_length + 10;
	Firebird::HalfStaticArray<UCHAR, 512> temp;

	length = BLB_get_data(tdbb, blob, temp.getBuffer(length), length);

	jrd_nod* node = PAR_blr(tdbb, NULL, temp.begin(), length, NULL, csb, NULL, false, 0);

	csb->csb_blr_reader = BlrReader();

	return node;
}


static jrd_nod* parse_procedure_blr(thread_db* tdbb,
									jrd_prc* procedure,
									bid* blob_id,
									AutoPtr<CompilerScratch>& csb)
{
/**************************************
 *
 *      p a r s e _ p r o c e d u r e _ b l r
 *
 **************************************
 *
 * Functional description
 *      Parse blr, returning a compiler scratch block with the results.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	blb* blob = BLB_open(tdbb, dbb->dbb_sys_trans, blob_id);
	const SLONG length = blob->blb_length + 10;
	Firebird::HalfStaticArray<UCHAR, 512> tmp;
	UCHAR* temp = tmp.getBuffer(length);
	const ULONG real_len = BLB_get_data(tdbb, blob, temp, length);
	fb_assert(real_len >= length - 10 && real_len <= ULONG(MAX_USHORT));
	par_messages(tdbb, temp, (USHORT) real_len, procedure, csb);

	return PAR_blr(tdbb, NULL, temp, real_len, NULL, csb,
		&procedure->prc_request, false, 0);
}


static void par_messages(thread_db* tdbb,
						 const UCHAR* const blr,
						 USHORT blr_length,
						 jrd_prc* procedure,
						 AutoPtr<CompilerScratch>& csb)
{
/**************************************
 *
 *      p a r _ m e s s a g e s
 *
 **************************************
 *
 * Functional description
 *      Parse the messages of a blr request.  For specified message, allocate
 *      a format (Format) block.
 *
 **************************************/

 	if (blr_length < 2)
 		ERR_post(Arg::Gds(isc_metadata_corrupt));
 	blr_length -= 2;

	csb->csb_blr_reader = BlrReader(blr, blr_length);

	const SSHORT version = csb->csb_blr_reader.getByte();
	if (version != blr_version4 && version != blr_version5) {
		ERR_post(Arg::Gds(isc_metadata_corrupt) <<
				 Arg::Gds(isc_wroblrver) << Arg::Num(blr_version4) << Arg::Num(version));
	}

	if (csb->csb_blr_reader.getByte() != blr_begin) {
		ERR_post(Arg::Gds(isc_metadata_corrupt));
	}

	SET_TDBB(tdbb);

	while (csb->csb_blr_reader.getByte() == blr_message)
	{
		if (blr_length < 4) // Covers the call above and the next three
			ERR_post(Arg::Gds(isc_metadata_corrupt));
		blr_length -= 4;
		const USHORT msg_number = csb->csb_blr_reader.getByte();
		USHORT count = csb->csb_blr_reader.getByte();
		count += (csb->csb_blr_reader.getByte()) << 8;
		Format* format = Format::newFormat(*tdbb->getDefaultPool(), count);

		ULONG offset = 0;
		for (Format::fmt_desc_iterator desc = format->fmt_desc.begin(); count; --count, ++desc)
		{
			// CVC: I can't find an elegant way to make PAR_desc aware of the BLR length, so
			// for now I know that at least should consume one byte. I would have to change the
			// current 6 calls to it. Maybe a new member in CompilerScratch may be the solution?
			if (blr_length < 2)
				ERR_post(Arg::Gds(isc_metadata_corrupt));

			const UCHAR* const savePos = csb->csb_blr_reader.getPos();

			const USHORT align = PAR_desc(tdbb, csb, &*desc);
			if (align) {
				offset = FB_ALIGN(offset, align);
			}
			desc->dsc_address = (UCHAR *) (IPTR) offset;
			offset += desc->dsc_length;

			// This solution only checks that PAR_desc doesn't run out of bounds, but it
			// does after the disaster happened.
			const int diff = csb->csb_blr_reader.getPos() - savePos;
			if (diff > blr_length)
				ERR_post(Arg::Gds(isc_metadata_corrupt));
			blr_length -= diff;
		}

		if (offset > MAX_FORMAT_SIZE) {
			ERR_post(Arg::Gds(isc_imp_exc) <<
					 Arg::Gds(isc_blktoobig));
		}

		format->fmt_length = (USHORT) offset;

		switch (msg_number)
		{
		case 0:
			procedure->prc_input_fmt = format;
			break;
		case 1:
			procedure->prc_output_fmt = format;
			break;
		default:
			delete format;
		}
	}
}


void MET_release_trigger(thread_db* tdbb, trig_vec** vector_ptr,
	const Firebird::MetaName& name)
{
/***********************************************
 *
 *      M E T _ r e l e a s e _ t r i g g e r
 *
 ***********************************************
 *
 * Functional description
 *      Release a specified trigger.
 *      If trigger are still active let someone
 *      else do the work.
 *
 **************************************/
	if (!*vector_ptr)
		return;

	trig_vec& vector = **vector_ptr;

	SET_TDBB(tdbb);

	for (size_t i = 0; i < vector.getCount(); ++i)
	{
		if (vector[i].name == name)
		{
			jrd_req* r = vector[i].request;
			if (r)
			{
				if (CMP_clone_is_active(r))
					break;
				CMP_release(tdbb, r);
			}
			vector.remove(i);
			break;
		}
	}
}


void MET_release_triggers( thread_db* tdbb, trig_vec** vector_ptr)
{
/***********************************************
 *
 *      M E T _ r e l e a s e _ t r i g g e r s
 *
 ***********************************************
 *
 * Functional description
 *      Release a possibly null vector of triggers.
 *      If triggers are still active let someone
 *      else do the work.
 *
 **************************************/
	trig_vec* vector = *vector_ptr;
	if (!vector)
	{
		return;
	}

	SET_TDBB(tdbb);

	*vector_ptr = NULL;

	for (size_t i = 0; i < vector->getCount(); i++)
	{
		jrd_req* r = (*vector)[i].request;
		if (r && CMP_clone_is_active(r))
		{
			return;
		}
	}

	for (size_t i = 0; i < vector->getCount(); i++)
	{
		jrd_req* r = (*vector)[i].request;
		if (r)
		{
			CMP_release(tdbb, r);
		}
	}

	delete vector;
}


static bool resolve_charset_and_collation(thread_db* tdbb,
										  USHORT* id,
										  const UCHAR* charset,
										  const UCHAR* collation)
{
   struct {
          SSHORT jrd_37;	// gds__utility 
          SSHORT jrd_38;	// RDB$COLLATION_ID 
          SSHORT jrd_39;	// RDB$CHARACTER_SET_ID 
   } jrd_36;
   struct {
          TEXT  jrd_34 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_35 [32];	// RDB$TYPE_NAME 
   } jrd_33;
   struct {
          SSHORT jrd_44;	// gds__utility 
          SSHORT jrd_45;	// RDB$COLLATION_ID 
          SSHORT jrd_46;	// RDB$CHARACTER_SET_ID 
   } jrd_43;
   struct {
          TEXT  jrd_42 [32];	// RDB$COLLATION_NAME 
   } jrd_41;
   struct {
          SSHORT jrd_51;	// gds__utility 
          SSHORT jrd_52;	// RDB$CHARACTER_SET_ID 
   } jrd_50;
   struct {
          TEXT  jrd_49 [32];	// RDB$CHARACTER_SET_NAME 
   } jrd_48;
/**************************************
 *
 *      r e s o l v e _ c h a r s e t _ a n d _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *      Given ASCII7 name of charset & collation
 *      resolve the specification to a Character set id.
 *      This character set id is also the id of the text_object
 *      that implements the C locale for the Character set.
 *
 * Inputs:
 *      (charset)
 *              ASCII7z name of character set.
 *              NULL (implying unspecified) means use the character set
 *              for defined for (collation).
 *
 *      (collation)
 *              ASCII7z name of collation.
 *              NULL means use the default collation for (charset).
 *
 * Outputs:
 *      (*id)
 *              Set to character set specified by this name (low byte)
 *              Set to collation specified by this name (high byte).
 *
 * Return:
 *      true if no errors (and *id is set).
 *      false if either name not found.
 *        or if names found, but the collation isn't for the specified
 *        character set.
 *
 **************************************/
	bool found = false;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	fb_assert(id != NULL);

	jrd_req* handle = NULL;

	if (collation == NULL)
	{
		if (charset == NULL)
			charset = (const UCHAR*) DEFAULT_CHARACTER_SET_NAME;

		USHORT charset_id = 0;
		if (get_type(tdbb, &charset_id, charset, "RDB$CHARACTER_SET_NAME"))
		{
			*id = charset_id;
			return true;
		}

		/* Charset name not found in the alias table - before giving up
		   try the character_set table */

		/*FOR(REQUEST_HANDLE handle)
			FIRST 1 CS IN RDB$CHARACTER_SETS
				WITH CS.RDB$CHARACTER_SET_NAME EQ charset*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_47, sizeof(jrd_47), true);
		gds__vtov ((const char*) charset, (char*) jrd_48.jrd_49, 32);
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_48);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_50);
		   if (!jrd_50.jrd_51) break;

			found = true;
			*id = /*CS.RDB$CHARACTER_SET_ID*/
			      jrd_50.jrd_52;

		/*END_FOR;*/
		   }
		}
		CMP_release(tdbb, handle);

		return found;
	}

	if (charset == NULL)
	{
		/*FOR(REQUEST_HANDLE handle)
			FIRST 1 COL IN RDB$COLLATIONS
				WITH COL.RDB$COLLATION_NAME EQ collation*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_40, sizeof(jrd_40), true);
		gds__vtov ((const char*) collation, (char*) jrd_41.jrd_42, 32);
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_41);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 6, (UCHAR*) &jrd_43);
		   if (!jrd_43.jrd_44) break;

			found = true;
			*id = /*COL.RDB$CHARACTER_SET_ID*/
			      jrd_43.jrd_46 | (/*COL.RDB$COLLATION_ID*/
    jrd_43.jrd_45 << 8);

		/*END_FOR;*/
		   }
		}
		CMP_release(tdbb, handle);

		return found;
	}

	/*FOR(REQUEST_HANDLE handle)
		FIRST 1 CS IN RDB$CHARACTER_SETS CROSS
			COL IN RDB$COLLATIONS OVER RDB$CHARACTER_SET_ID CROSS
			AL1 IN RDB$TYPES
			WITH AL1.RDB$FIELD_NAME EQ "RDB$CHARACTER_SET_NAME"
			AND AL1.RDB$TYPE_NAME EQ charset
			AND COL.RDB$COLLATION_NAME EQ collation
			AND AL1.RDB$TYPE EQ CS.RDB$CHARACTER_SET_ID*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_32, sizeof(jrd_32), true);
	gds__vtov ((const char*) collation, (char*) jrd_33.jrd_34, 32);
	gds__vtov ((const char*) charset, (char*) jrd_33.jrd_35, 32);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_33);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 6, (UCHAR*) &jrd_36);
	   if (!jrd_36.jrd_37) break;

		found = true;
		*id = /*CS.RDB$CHARACTER_SET_ID*/
		      jrd_36.jrd_39 | (/*COL.RDB$COLLATION_ID*/
    jrd_36.jrd_38 << 8);

	/*END_FOR;*/
	   }
	}
	CMP_release(tdbb, handle);

	return found;
}


static void save_trigger_data(thread_db* tdbb, trig_vec** ptr, jrd_rel* relation,
							  jrd_req* request, blb* blrBlob, bid* dbgBlobID,
							  const TEXT* name, UCHAR type,
							  bool sys_trigger, USHORT flags)
{
/**************************************
 *
 *      s a v e _ t r i g g e r _ d a t a
 *
 **************************************
 *
 * Functional description
 *      Save trigger data to passed vector
 *
 **************************************/
	trig_vec* vector = *ptr;

	if (!vector) {
		vector = FB_NEW(*tdbb->getDatabase()->dbb_permanent)
			trig_vec(*tdbb->getDatabase()->dbb_permanent);
		*ptr = vector;
	}

	Trigger& t = vector->add();
	if (blrBlob)
	{
		const SLONG length = blrBlob->blb_length + 10;
		UCHAR* ptr2 = t.blr.getBuffer(length);
		t.blr.resize(BLB_get_data(tdbb, blrBlob, ptr2, length));
	}
	if (dbgBlobID)
		t.dbg_blob_id = *dbgBlobID;
	if (name) {
		t.name = name;
	}
	t.type = type;
	t.flags = flags;
	t.compile_in_progress = false;
	t.sys_trigger = sys_trigger;
	t.request = request;
	t.relation = relation;
}


const Trigger* findTrigger(trig_vec* triggers, const Firebird::MetaName& trig_name)
{
	if (triggers)
	{
		for (trig_vec::iterator t = triggers->begin(); t != triggers->end(); ++t)
		{
			if (t->name.compare(trig_name) == 0)
				return &(*t);
		}
	}

	return NULL;
}


static void store_dependencies(thread_db* tdbb,
							   CompilerScratch* csb,
							   const jrd_rel* dep_rel,
							   const Firebird::MetaName& object_name,
							   int dependency_type,
							   jrd_tra* transaction)
{
   struct {
          TEXT  jrd_9 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_10 [32];	// RDB$DEPENDED_ON_NAME 
          TEXT  jrd_11 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_12;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_13;	// gds__null_flag 
          SSHORT jrd_14;	// RDB$DEPENDED_ON_TYPE 
   } jrd_8;
   struct {
          SSHORT jrd_22;	// gds__utility 
   } jrd_21;
   struct {
          TEXT  jrd_17 [32];	// RDB$DEPENDED_ON_NAME 
          TEXT  jrd_18 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_19;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_20;	// RDB$DEPENDED_ON_TYPE 
   } jrd_16;
   struct {
          SSHORT jrd_31;	// gds__utility 
   } jrd_30;
   struct {
          TEXT  jrd_25 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_26 [32];	// RDB$DEPENDED_ON_NAME 
          TEXT  jrd_27 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_28;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_29;	// RDB$DEPENDED_ON_TYPE 
   } jrd_24;
/**************************************
 *
 *      s t o r e _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *      Store records in RDB$DEPENDENCIES
 *      corresponding to the objects found during
 *      compilation of blr for a trigger, view, etc.
 *
 **************************************/
	Firebird::MetaName name;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	const Trigger* t = 0;
	const bool checkTableScope =
		(dependency_type == obj_computed) ||
		(dependency_type == obj_trigger) && (dep_rel != 0) &&
		(
			(t = findTrigger(dep_rel->rel_pre_erase, object_name)) ||
			(t = findTrigger(dep_rel->rel_pre_modify, object_name)) ||
			(t = findTrigger(dep_rel->rel_pre_store, object_name)) ||
			(t = findTrigger(dep_rel->rel_post_erase, object_name)) ||
			(t = findTrigger(dep_rel->rel_post_modify, object_name)) ||
			(t = findTrigger(dep_rel->rel_post_store, object_name))
		) && t && (t->sys_trigger);

	while (csb->csb_dependencies.hasData())
	{
		jrd_nod* node = csb->csb_dependencies.pop();
		if (!node->nod_arg[e_dep_object])
			continue;

		int dpdo_type = (int) (IPTR) node->nod_arg[e_dep_object_type];
		jrd_rel* relation = NULL;
		jrd_prc* procedure = NULL;
		const Firebird::MetaName* dpdo_name = 0;
		SubtypeInfo info;

		switch (dpdo_type)
		{
		case obj_relation:
			relation = (jrd_rel*) node->nod_arg[e_dep_object];
			dpdo_name = &relation->rel_name;

			fb_assert(dep_rel || !checkTableScope);

			if (checkTableScope &&
				( (dep_rel->rel_flags & (REL_temp_tran | REL_temp_conn)) !=
				  (relation->rel_flags & (REL_temp_tran | REL_temp_conn)) ))
			{
				if ( !( // master is ON COMMIT PRESERVE, detail is ON COMMIT DELETE
						(dep_rel->rel_flags & REL_temp_tran) && (relation->rel_flags & REL_temp_conn) ||
						// computed field of a view
						(dependency_type == obj_computed) && (dep_rel->rel_view_rse != NULL)
					   ))
				{
					Firebird::string sMaster, sChild;

					make_relation_scope_name(relation->rel_name.c_str(),
						relation->rel_flags, sMaster);
					make_relation_scope_name(dep_rel->rel_name.c_str(),
						dep_rel->rel_flags, sChild);

					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_met_wrong_gtt_scope) << Arg::Str(sChild) <<
																  Arg::Str(sMaster));
				}
			}

			MET_scan_relation(tdbb, relation);
			if (relation->rel_view_rse) {
				dpdo_type = obj_view;
			}
			break;
		case obj_procedure:
			procedure = (jrd_prc*) node->nod_arg[e_dep_object];
			dpdo_name = &procedure->prc_name;
			break;
		case obj_collation:
			{
				const USHORT number = (IPTR) node->nod_arg[e_dep_object];
				MET_get_char_coll_subtype_info(tdbb, number, &info);
				dpdo_name = &info.collationName;
			}
			break;
		case obj_exception:
			{
				const SLONG number = (IPTR) node->nod_arg[e_dep_object];
				MET_lookup_exception(tdbb, number, name, NULL);
				dpdo_name = &name;
			}
			break;
		case obj_field:
			dpdo_name = (Firebird::MetaName*) node->nod_arg[e_dep_object];
			break;
			// CVC: Here I'm going to track those pesky things named generators and UDFs.
		case obj_generator:
			{
				const SLONG number = (IPTR) node->nod_arg[e_dep_object];
				MET_lookup_generator_id(tdbb, number, name);
				dpdo_name = &name;
			}
			break;
		case obj_udf:
			{
				UserFunction* udf = (UserFunction*) node->nod_arg[e_dep_object];
				dpdo_name = &udf->fun_name;
			}
			break;
		case obj_index:
			name = (TEXT*) node->nod_arg[e_dep_object];
			dpdo_name = &name;
			break;
		}

		jrd_nod* field_node = node->nod_arg[e_dep_field];

		Firebird::MetaName field_name;
		if (field_node)
		{
			if (field_node->nod_type == nod_field)
			{
				const SSHORT fld_id = (SSHORT) (IPTR) field_node->nod_arg[0];
				if (relation)
				{
					const jrd_fld* field = MET_get_field(relation, fld_id);
					if (field)
					{
						field_name = field->fld_name;
					}
				}
				else if (procedure)
				{
					const Parameter* param = (*procedure->prc_output_fields)[fld_id];
					// CVC: Setting the field var here didn't make sense alone,
					// so I thought the missing code was to try to extract
					// the field name that's in this case an output var from a proc.
					if (param)
					{
						field_name = param->prm_name;
					}
				}
			}
			else
			{
				field_name = (TEXT *) field_node->nod_arg[0];
			}
		}

		if (field_name.length() > 0)
		{
			jrd_req* request = CMP_find_request(tdbb, irq_c_deps_f, IRQ_REQUESTS);
			bool found = false;
			fb_assert(dpdo_name);
			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction) X IN RDB$DEPENDENCIES WITH
				X.RDB$DEPENDENT_NAME = object_name.c_str() AND
					X.RDB$DEPENDED_ON_NAME = dpdo_name->c_str() AND
					X.RDB$DEPENDED_ON_TYPE = dpdo_type AND
					X.RDB$FIELD_NAME = field_name.c_str() AND
					X.RDB$DEPENDENT_TYPE = dependency_type*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_23, sizeof(jrd_23), true);
			gds__vtov ((const char*) field_name.c_str(), (char*) jrd_24.jrd_25, 32);
			gds__vtov ((const char*) dpdo_name->c_str(), (char*) jrd_24.jrd_26, 32);
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_24.jrd_27, 32);
			jrd_24.jrd_28 = dependency_type;
			jrd_24.jrd_29 = dpdo_type;
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 100, (UCHAR*) &jrd_24);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_30);
			   if (!jrd_30.jrd_31) break;

				if (!REQUEST(irq_c_deps_f))
					REQUEST(irq_c_deps_f) = request;
				found = true;
			/*END_FOR;*/
			   }
			}

			if (!REQUEST(irq_c_deps_f))
				REQUEST(irq_c_deps_f) = request;

			if (found) {
				continue;
			}
		}
		else
		{
			jrd_req* request = CMP_find_request(tdbb, irq_c_deps, IRQ_REQUESTS);
			bool found = false;
			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction) X IN RDB$DEPENDENCIES WITH
				X.RDB$DEPENDENT_NAME = object_name.c_str() AND
					X.RDB$DEPENDED_ON_NAME = dpdo_name->c_str() AND
					X.RDB$DEPENDED_ON_TYPE = dpdo_type AND
					X.RDB$FIELD_NAME MISSING AND
					X.RDB$DEPENDENT_TYPE = dependency_type*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_15, sizeof(jrd_15), true);
			gds__vtov ((const char*) dpdo_name->c_str(), (char*) jrd_16.jrd_17, 32);
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_16.jrd_18, 32);
			jrd_16.jrd_19 = dependency_type;
			jrd_16.jrd_20 = dpdo_type;
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_16);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_21);
			   if (!jrd_21.jrd_22) break;

				if (!REQUEST(irq_c_deps))
					REQUEST(irq_c_deps) = request;
				found = true;

			/*END_FOR;*/
			   }
			}

			if (!REQUEST(irq_c_deps))
				REQUEST(irq_c_deps) = request;

			if (found) {
				continue;
			}
		}

		jrd_req* request = CMP_find_request(tdbb, irq_s_deps, IRQ_REQUESTS);

		fb_assert(dpdo_name);
		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction) DEP IN RDB$DEPENDENCIES*/
		{
		
			strcpy(/*DEP.RDB$DEPENDENT_NAME*/
			       jrd_8.jrd_11, object_name.c_str());
			/*DEP.RDB$DEPENDED_ON_TYPE*/
			jrd_8.jrd_14 = dpdo_type;
			strcpy(/*DEP.RDB$DEPENDED_ON_NAME*/
			       jrd_8.jrd_10, dpdo_name->c_str());
			if (field_name.length() > 0)
			{
				/*DEP.RDB$FIELD_NAME.NULL*/
				jrd_8.jrd_13 = FALSE;
				strcpy(/*DEP.RDB$FIELD_NAME*/
				       jrd_8.jrd_9, field_name.c_str());
			}
			else {
				/*DEP.RDB$FIELD_NAME.NULL*/
				jrd_8.jrd_13 = TRUE;
			}
			/*DEP.RDB$DEPENDENT_TYPE*/
			jrd_8.jrd_12 = dependency_type;
		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_7, sizeof(jrd_7), true);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 102, (UCHAR*) &jrd_8);
		}

		if (!REQUEST(irq_s_deps))
			REQUEST(irq_s_deps) = request;
	}
}


static bool verify_TRG_ignore_perm(thread_db* tdbb, const Firebird::MetaName& trig_name)
{
   struct {
          TEXT  jrd_4 [12];	// RDB$DELETE_RULE 
          TEXT  jrd_5 [12];	// RDB$UPDATE_RULE 
          SSHORT jrd_6;	// gds__utility 
   } jrd_3;
   struct {
          TEXT  jrd_2 [32];	// RDB$TRIGGER_NAME 
   } jrd_1;
/*****************************************************
 *
 *      v e r i f y _ T R G _ i g n o r e  _ p e r m
 *
 *****************************************************
 *
 * Functional description
 *      Return true if this trigger can go through without any permission
 *      checks. Currently, the only class of triggers that can go
 *      through without permission checks are
 *      (a) two system triggers (RDB$TRIGGERS_34 and RDB$TRIGGERS_35)
 *      (b) those defined for referential integrity actions such as,
 *      set null, set default, and cascade.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// See if this is a system trigger, with the flag set as TRG_ignore_perm

	if (INI_get_trig_flags(trig_name.c_str()) & TRG_ignore_perm)
	{
		return true;
	}

	// See if this is a RI trigger

	jrd_req* request = CMP_find_request(tdbb, irq_c_trg_perm, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		CHK IN RDB$CHECK_CONSTRAINTS CROSS
			REF IN RDB$REF_CONSTRAINTS WITH
			CHK.RDB$TRIGGER_NAME EQ trig_name.c_str() AND
			REF.RDB$CONSTRAINT_NAME = CHK.RDB$CONSTRAINT_NAME*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
	gds__vtov ((const char*) trig_name.c_str(), (char*) jrd_1.jrd_2, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 26, (UCHAR*) &jrd_3);
	   if (!jrd_3.jrd_6) break;

		if (!REQUEST(irq_c_trg_perm))
		{
			REQUEST(irq_c_trg_perm) = request;
		}

		EXE_unwind(tdbb, request);
		fb_utils::exact_name_limit(/*REF.RDB$UPDATE_RULE*/
					   jrd_3.jrd_5, sizeof(/*REF.RDB$UPDATE_RULE*/
	 jrd_3.jrd_5));
		fb_utils::exact_name_limit(/*REF.RDB$DELETE_RULE*/
					   jrd_3.jrd_4, sizeof(/*REF.RDB$DELETE_RULE*/
	 jrd_3.jrd_4));

		if (!strcmp(/*REF.RDB$UPDATE_RULE*/
			    jrd_3.jrd_5, RI_ACTION_CASCADE) ||
			!strcmp(/*REF.RDB$UPDATE_RULE*/
				jrd_3.jrd_5, RI_ACTION_NULL) ||
			!strcmp(/*REF.RDB$UPDATE_RULE*/
				jrd_3.jrd_5, RI_ACTION_DEFAULT) ||
			!strcmp(/*REF.RDB$DELETE_RULE*/
				jrd_3.jrd_4, RI_ACTION_CASCADE) ||
			!strcmp(/*REF.RDB$DELETE_RULE*/
				jrd_3.jrd_4, RI_ACTION_NULL) ||
			!strcmp(/*REF.RDB$DELETE_RULE*/
				jrd_3.jrd_4, RI_ACTION_DEFAULT))
		{
			return true;
		}

		return false;
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_c_trg_perm))
	{
		REQUEST(irq_c_trg_perm) = request;
	}

	return false;
}
