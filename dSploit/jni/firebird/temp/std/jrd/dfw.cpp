/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dfw.epp
 *	DESCRIPTION:	Deferred Work handler
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
 * 2001.6.25 Claudio Valderrama: Implement deferred check for udf usage
 * inside a procedure before dropping the udf and creating stub for future
 * processing of dependencies from dropped generators.
 *
 * 2001.8.12 Claudio Valderrama: find_depend_in_dfw() and other functions
 *   should respect identifiers with embedded blanks instead of chopping them
 *.
 * 2001.10.01 Claudio Valderrama: check constraints should fire AFTER the
 *   BEFORE <action> triggers; otherwise they allow invalid data to be stored.
 *   This is a quick fix for SF Bug #444463 until a more robust one is devised
 *   using trigger's rdb$flags or another mechanism.
 *
 * 2001.10.10 Ann Harrison:  Don't increment the format version unless the
 *   table is actually reformatted.  At the same time, break out some of
 *   the parts of make_version making some new subroutines with the goal
 *   of making make_version readable.
 *
 * 2001.10.18 Ann Harrison: some cleanup of trigger & constraint handling.
 *   it now appears to work correctly on new Firebird databases with lots
 *   of system types and on InterBase databases, without checking for
 *   missing source.
 *
 * 23-Feb-2002 Dmitry Yemanov - Events wildcarding
 *
 * 2002-02-24 Sean Leyne - Code Cleanup of old Win 3.1 port (WINDOWS_ONLY)
 *
 * Adriano dos Santos Fernandes
 *
 * 2008-03-16 Alex Peshkoff - avoid most of data modifications in system transaction.
 *	Problems took place when same data was modified in user transaction, and later -
 *	in system transaction. System transaction always performs updates in place,
 *	but when between RPB setup and actual modification garbage was collected (this
 *	was noticed with GC thread active, but may happen due to any read of the record),
 *	BUGCHECK(291) took place. To avoid that issue, it was decided not to modify data
 *	in system transaction. An exception is RDB$FORMATS relation, which is always modified
 *	by transaction zero. Also an aspect of 'dirty' access from system transaction was
 *	taken into an account in add_file(), make_version() and create_index().
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>

#include "../common/classes/fb_string.h"
#include "../common/classes/VaryStr.h"
#include "../jrd/common.h"
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/irq.h"
#include "../jrd/tra.h"
#include "../jrd/os/pio.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/req.h"
#include "../jrd/exe.h"
#include "../jrd/scl.h"
#include "../jrd/blb.h"
#include "../jrd/met.h"
#include "../jrd/lck.h"
#include "../jrd/sdw.h"
#include "../jrd/flags.h"
#include "../jrd/intl.h"
#include "../intl/charsets.h"
#include "../jrd/align.h"
#include "../jrd/gdsassert.h"
#include "../jrd/blb_proto.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/dsc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/grant_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/isc_f_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/pcmet_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/rlck_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/sdw_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/event_proto.h"
#include "../jrd/nbak.h"
#include "../jrd/trig.h"
#include "../jrd/IntlManager.h"
#include "../jrd/UserManagement.h"
#include "../common/utils_proto.h"

#include "../common/classes/Hash.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gen/iberror.h"

/* Pick up system relation ids */
#include "../jrd/ini.h"

/* Define range of user relation ids */

const int MAX_RELATION_ID	= 32767;

const int COMPUTED_FLAG	= 128;
const int WAIT_PERIOD	= -1;

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [203] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,5,0,41,3,0,32,0,41,0,0,12,
0,41,0,0,12,0,7,0,7,0,12,0,2,7,'C',1,'K',12,0,0,'G',58,47,24,
0,1,0,25,0,0,0,57,56,24,0,8,0,25,0,4,0,25,0,3,0,58,57,47,24,0,
8,0,21,8,0,0,0,0,0,61,24,0,8,0,60,'C',2,'K',24,0,1,'K',22,0,2,
'G',58,47,24,0,0,0,24,1,1,0,58,47,24,1,0,0,24,2,0,0,57,47,24,
2,1,0,25,0,2,0,47,24,2,1,0,25,0,1,0,255,'F',1,'H',24,0,2,0,255,
14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,7,
0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_11 [186] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,3,0,41,3,0,32,0,41,0,0,12,
0,41,0,0,12,0,12,0,2,7,'C',1,'K',12,0,0,'G',58,47,24,0,1,0,25,
0,0,0,58,57,47,24,0,8,0,21,8,0,0,0,0,0,61,24,0,8,0,59,60,'C',
2,'K',24,0,1,'K',22,0,2,'G',58,47,24,0,0,0,24,1,1,0,58,47,24,
1,0,0,24,2,0,0,57,47,24,2,1,0,25,0,2,0,47,24,2,1,0,25,0,1,0,255,
'F',1,'H',24,0,2,0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,
0,0,0,25,1,1,0,1,24,0,7,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,
25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_20 [116] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,
'C',1,'K',12,0,0,'G',58,47,24,0,1,0,25,0,0,0,47,24,0,8,0,21,8,
0,1,0,0,0,'F',1,'H',24,0,2,0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,
21,8,0,1,0,0,0,25,1,1,0,1,24,0,7,0,25,1,2,0,255,14,1,1,21,8,0,
0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_27 [71] =
   {	// blr string 

4,2,4,0,1,0,7,0,2,7,'C',1,'K',10,0,0,'D',21,8,0,1,0,0,0,'G',49,
24,0,5,0,21,8,0,0,0,0,0,255,14,0,2,1,21,8,0,1,0,0,0,25,0,0,0,
255,14,0,1,21,8,0,0,0,0,0,25,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_30 [129] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,7,0,7,0,4,1,3,0,7,0,7,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',12,0,0,'G',47,24,0,0,0,25,0,0,0,255,
2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,10,0,41,1,2,0,1,0,255,
17,0,9,13,12,3,18,0,12,2,10,0,1,2,1,41,2,1,0,0,0,24,1,10,0,255,
255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_42 [126] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,7,0,7,0,4,1,3,0,7,0,7,0,7,0,4,0,1,0,7,
0,12,0,2,7,'C',1,'K',26,0,0,'G',47,24,0,1,0,25,0,0,0,255,2,14,
1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,12,0,41,1,2,0,1,0,255,17,
0,9,13,12,3,18,0,12,2,10,0,1,2,1,41,2,1,0,0,0,24,1,12,0,255,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_54 [108] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,7,0,12,0,2,7,
'C',1,'K',8,0,0,'G',58,47,24,0,0,0,25,0,0,0,47,24,0,1,0,21,8,
0,0,0,0,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,17,0,9,13,
12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_63 [96] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',2,'K',7,
0,0,'K',6,0,1,'G',58,47,24,1,8,0,24,0,1,0,47,24,0,0,0,25,0,0,
0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,1,5,0,25,1,1,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_69 [84] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',12,0,0,
'G',58,47,24,0,1,0,25,0,0,0,47,24,0,3,0,21,8,0,1,0,0,0,255,14,
1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_74 [484] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,6,0,7,0,7,0,7,0,7,0,7,0,7,0,4,1,34,0,41,3,
0,32,0,41,3,0,32,0,9,0,9,0,9,0,9,0,41,3,0,32,0,41,3,0,32,0,9,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',2,'K',5,0,0,'K',2,0,1,'G',58,47,24,0,1,0,25,0,0,0,47,
24,0,2,0,24,1,0,0,255,2,14,1,2,1,24,1,0,0,25,1,0,0,1,24,0,14,
0,41,1,1,0,24,0,1,24,1,2,0,25,1,2,0,1,24,1,6,0,25,1,3,0,1,24,
0,12,0,25,1,4,0,1,24,1,12,0,25,1,5,0,1,24,0,4,0,25,1,6,0,1,24,
0,0,0,25,1,7,0,1,24,1,4,0,25,1,8,0,1,21,8,0,1,0,0,0,25,1,9,0,
1,24,1,19,0,25,1,10,0,1,24,1,20,0,25,1,11,0,1,24,1,21,0,25,1,
12,0,1,24,1,22,0,25,1,13,0,1,24,1,11,0,25,1,14,0,1,24,1,8,0,25,
1,15,0,1,24,1,9,0,25,1,16,0,1,24,1,10,0,25,1,17,0,1,24,0,18,0,
41,1,19,0,18,0,1,24,1,25,0,41,1,21,0,20,0,1,24,1,26,0,41,1,23,
0,22,0,1,24,0,16,0,25,1,25,0,1,24,1,23,0,25,1,26,0,1,24,0,10,
0,25,1,27,0,1,24,0,6,0,41,1,29,0,28,0,1,24,0,8,0,41,1,31,0,30,
0,1,24,0,9,0,41,1,33,0,32,0,255,17,0,9,13,12,3,18,0,12,2,10,0,
2,2,1,41,2,5,0,4,0,24,2,6,0,1,41,2,3,0,2,0,24,2,8,0,1,41,2,1,
0,0,0,24,2,9,0,255,255,255,14,1,1,21,8,0,0,0,0,0,25,1,9,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_121 [231] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,5,0,9,0,7,0,7,0,7,0,7,0,4,1,9,0,9,0,41,0,
0,0,1,9,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,
7,'C',1,'K',6,0,0,'G',47,24,0,8,0,25,0,0,0,255,2,14,1,2,1,24,
0,11,0,25,1,0,0,1,24,0,10,0,25,1,1,0,1,24,0,0,0,25,1,2,0,1,21,
8,0,1,0,0,0,25,1,3,0,1,24,0,5,0,25,1,5,0,1,24,0,7,0,25,1,6,0,
1,24,0,6,0,41,1,7,0,4,0,1,24,0,3,0,25,1,8,0,255,17,0,9,13,12,
3,18,0,12,2,10,0,1,2,1,25,2,4,0,24,1,5,0,1,25,2,3,0,24,1,7,0,
1,25,2,0,0,24,1,11,0,1,41,2,2,0,1,0,24,1,6,0,255,255,255,14,1,
1,21,8,0,0,0,0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_142 [50] =
   {	// blr string 

4,2,4,0,3,0,9,0,7,0,7,0,12,0,15,'K',8,0,0,2,1,25,0,0,0,24,0,2,
0,1,25,0,1,0,24,0,0,0,1,25,0,2,0,24,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_147 [107] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,9,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,
2,7,'C',1,'K',12,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,
0,1,0,25,1,0,0,1,24,0,5,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,
1,24,0,3,0,25,1,3,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_155 [82] =
   {	// blr string 

4,2,4,1,2,0,9,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',26,
0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,6,0,25,1,0,0,1,
21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_161 [104] =
   {	// blr string 

4,2,4,1,4,0,8,0,8,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',21,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,3,0,25,
1,0,0,1,24,0,2,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,1,24,0,1,
0,25,1,3,0,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_169 [86] =
   {	// blr string 

4,2,4,1,3,0,9,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
2,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,2,0,41,1,0,0,
2,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_176 [124] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,
'C',3,'K',2,0,0,'K',5,0,1,'K',6,0,2,'G',58,47,24,0,0,0,24,1,2,
0,58,47,24,0,0,0,25,0,0,0,47,24,2,8,0,24,1,1,0,255,14,1,2,1,24,
1,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,2,3,0,25,1,2,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_183 [82] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',2,'K',6,0,0,'K',5,0,
1,'G',58,47,24,1,1,0,24,0,8,0,47,24,0,3,0,25,0,0,0,255,14,1,2,
1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,
255,255,76
   };	// end of blr string 
static const UCHAR	jrd_188 [68] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',6,0,0,'G',47,24,
0,3,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,
1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_193 [149] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,2,0,41,3,0,32,0,7,
0,12,0,2,7,'C',3,'K',6,0,0,'K',7,0,1,'K',5,0,2,'G',58,47,24,1,
1,0,24,0,8,0,58,47,24,0,3,0,25,0,1,0,58,47,24,2,10,0,24,1,2,0,
58,47,24,2,1,0,24,1,0,0,47,24,2,4,0,25,0,0,0,255,14,1,2,1,24,
2,4,0,25,1,0,0,1,24,1,0,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_201 [95] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,1,0,7,0,4,0,1,0,7,0,12,0,2,7,
'C',1,'K',8,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,
1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_210 [85] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',7,0,0,'G',47,24,0,1,0,25,0,0,0,255,14,1,2,1,24,0,0,0,25,
1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,
1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_216 [78] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',2,0,0,
'G',58,47,24,0,0,0,25,0,0,0,59,61,24,0,4,0,255,14,1,2,1,21,8,
0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_221 [86] =
   {	// blr string 

4,2,4,1,3,0,9,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
2,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,2,0,41,1,0,0,
2,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_228 [110] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,
'C',2,'K',5,0,0,'K',6,0,1,'G',58,47,24,1,8,0,24,0,1,0,47,24,0,
2,0,25,0,0,0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,
25,1,1,0,1,24,1,3,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,
0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_235 [86] =
   {	// blr string 

4,2,4,1,3,0,9,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
2,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,2,0,41,1,0,0,
2,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_242 [86] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',29,0,0,'G',
47,24,0,2,0,25,0,0,0,'F',1,'I',24,0,1,0,255,14,1,2,1,21,8,0,1,
0,0,0,25,1,0,0,1,24,0,1,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,
25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_248 [253] =
   {	// blr string 

4,2,4,4,2,0,7,0,7,0,4,3,1,0,7,0,4,2,2,0,9,0,7,0,4,1,10,0,41,3,
0,32,0,41,3,0,32,0,41,3,0,32,0,9,0,7,0,7,0,7,0,7,0,7,0,7,0,4,
0,2,0,41,3,0,32,0,7,0,12,0,2,7,'C',2,'K',29,0,0,'K',28,0,1,'G',
58,47,24,1,4,0,24,0,2,0,58,47,24,0,0,0,25,0,0,0,47,24,0,2,0,25,
0,1,0,255,2,14,1,2,1,24,1,0,0,25,1,0,0,1,24,0,0,0,25,1,1,0,1,
24,0,7,0,41,1,2,0,5,0,1,24,0,8,0,41,1,3,0,6,0,1,21,8,0,1,0,0,
0,25,1,4,0,1,24,0,2,0,25,1,7,0,1,24,0,1,0,41,1,9,0,8,0,255,17,
0,9,13,12,3,18,0,12,2,10,0,3,2,1,41,2,0,0,1,0,24,3,8,0,255,12,
4,10,0,2,2,1,41,4,1,0,0,0,24,2,1,0,255,255,255,14,1,1,21,8,0,
0,0,0,0,25,1,4,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_271 [82] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',6,
0,0,'G',47,24,0,8,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,
0,0,1,24,0,3,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_277 [96] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',2,'K',7,
0,0,'K',6,0,1,'G',58,47,24,1,8,0,24,0,1,0,47,24,0,0,0,25,0,0,
0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,1,5,0,25,1,1,0,255,
14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_283 [205] =
   {	// blr string 

4,2,4,4,1,0,7,0,4,3,1,0,7,0,4,2,2,0,7,0,7,0,4,1,6,0,41,0,0,0,
1,9,0,7,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',2,'K',
1,0,0,'K',6,0,1,'G',47,24,1,8,0,25,0,0,0,255,2,14,1,2,1,24,1,
10,0,25,1,0,0,1,24,1,0,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,1,
24,1,5,0,25,1,3,0,1,24,1,3,0,25,1,4,0,1,24,0,1,0,25,1,5,0,255,
17,0,9,13,12,3,18,0,12,4,10,0,2,2,1,25,4,0,0,24,2,1,0,255,12,
2,10,1,3,2,1,25,2,1,0,24,3,5,0,1,25,2,0,0,24,3,3,0,255,255,255,
14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_300 [86] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,41,0,0,12,0,12,0,2,7,'C',
1,'K',22,0,0,'G',58,47,24,0,5,0,25,0,0,0,47,24,0,1,0,25,0,1,0,
255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,
0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_306 [129] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,7,0,7,0,4,1,3,0,7,0,7,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',4,0,0,'G',47,24,0,0,0,25,0,0,0,255,
2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,2,0,41,1,2,0,1,0,255,
17,0,9,13,12,3,18,0,12,2,10,0,1,2,1,41,2,1,0,0,0,24,1,2,0,255,
255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_318 [338] =
   {	// blr string 

4,2,4,1,22,0,41,3,0,32,0,41,3,0,32,0,27,7,0,7,0,7,0,7,0,7,0,7,
0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,4,0,1,0,
41,3,0,32,0,12,0,2,7,'C',5,'K',4,0,0,'K',3,0,1,'K',5,0,2,'K',
2,0,3,'K',6,0,4,'G',58,58,58,47,24,1,0,0,24,0,0,0,58,47,24,2,
0,0,24,1,1,0,47,24,2,1,0,24,0,1,0,47,24,4,8,0,24,2,1,0,58,47,
24,3,0,0,24,2,2,0,47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,2,0,0,
25,1,0,0,1,24,0,8,0,41,1,1,0,15,0,1,24,0,12,0,25,1,2,0,1,21,8,
0,1,0,0,0,25,1,3,0,1,24,3,25,0,41,1,5,0,4,0,1,24,2,18,0,41,1,
7,0,6,0,1,24,3,26,0,41,1,9,0,8,0,1,24,2,9,0,25,1,10,0,1,24,3,
22,0,41,1,12,0,11,0,1,24,3,10,0,25,1,13,0,1,24,1,2,0,25,1,14,
0,1,24,0,7,0,25,1,16,0,1,24,0,3,0,25,1,17,0,1,24,0,5,0,25,1,18,
0,1,24,0,6,0,25,1,19,0,1,24,0,2,0,25,1,20,0,1,24,4,3,0,25,1,21,
0,255,14,1,1,21,8,0,0,0,0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_344 [172] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,7,0,7,0,4,1,6,0,41,3,0,32,0,9,0,7,0,7,
0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',2,'K',4,0,0,'K',6,
0,1,'G',58,47,24,1,8,0,24,0,1,0,47,24,0,0,0,25,0,0,0,255,2,14,
1,2,1,24,0,1,0,25,1,0,0,1,24,1,0,0,41,1,1,0,5,0,1,21,8,0,1,0,
0,0,25,1,2,0,1,24,0,2,0,41,1,4,0,3,0,255,17,0,9,13,12,3,18,0,
12,2,10,0,2,2,1,41,2,1,0,0,0,24,2,2,0,255,255,255,14,1,1,21,8,
0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_359 [114] =
   {	// blr string 

4,2,4,1,3,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',2,'K',
4,0,0,'K',6,0,1,'G',58,47,24,1,8,0,24,0,1,0,58,47,24,0,0,0,25,
0,0,0,59,61,24,1,3,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,
1,3,0,25,1,1,0,1,24,1,16,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,
25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_366 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',1,0,0,
'G',47,24,0,2,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,
255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_371 [114] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,2,0,41,3,0,32,0,7,0,12,0,
2,7,'C',1,'K',13,0,0,'G',58,47,24,0,1,0,25,0,0,0,47,24,0,4,0,
25,0,1,0,'E',1,24,0,0,0,255,14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,
0,1,0,0,0,25,1,1,0,1,24,0,3,0,25,1,2,0,255,14,1,1,21,8,0,0,0,
0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_379 [129] =
   {	// blr string 

4,2,4,1,3,0,41,3,0,32,0,7,0,7,0,4,0,3,0,41,3,0,32,0,41,3,0,32,
0,7,0,12,0,2,7,'C',1,'K',13,0,0,'G',58,47,24,0,1,0,25,0,1,0,58,
47,24,0,4,0,25,0,2,0,47,24,0,2,0,25,0,0,0,'E',1,24,0,0,0,255,
14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,3,
0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_388 [133] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,3,0,41,3,0,32,0,7,0,7,0,12,0,
2,7,'C',2,'K',13,0,0,'K',5,0,1,'G',58,47,24,0,0,0,25,0,0,0,58,
47,24,0,3,0,25,0,2,0,58,47,24,0,4,0,25,0,1,0,58,47,24,1,1,0,24,
0,1,0,47,24,1,0,0,24,0,2,0,255,14,1,2,1,24,1,2,0,25,1,0,0,1,21,
8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,
76
   };	// end of blr string 
static const UCHAR	jrd_396 [211] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,3,0,8,0,7,0,7,0,4,1,6,0,41,0,0,0,1,8,0,7,
0,7,0,7,0,7,0,4,0,1,0,41,0,0,0,1,12,0,2,7,'C',2,'K',10,0,0,'K',
10,0,1,'G',58,47,24,1,5,0,24,0,5,0,47,24,0,0,0,25,0,0,0,'F',1,
'H',24,1,2,0,255,2,14,1,2,1,24,1,0,0,25,1,0,0,1,24,1,2,0,25,1,
1,0,1,21,8,0,1,0,0,0,25,1,2,0,1,24,1,1,0,25,1,3,0,1,24,1,4,0,
25,1,4,0,1,24,1,5,0,25,1,5,0,255,17,0,9,13,12,3,18,0,12,2,10,
1,2,2,1,25,2,2,0,24,2,1,0,1,25,2,0,0,24,2,2,0,1,25,2,1,0,24,2,
4,0,255,255,255,14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_412 [149] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,41,0,0,0,1,4,1,4,0,41,0,0,0,1,7,0,7,0,
7,0,4,0,1,0,41,0,0,0,1,12,0,2,7,'C',1,'K',10,0,0,'G',47,24,0,
0,0,25,0,0,0,255,2,14,1,2,1,24,0,0,0,25,1,0,0,1,21,8,0,1,0,0,
0,25,1,1,0,1,24,0,4,0,25,1,2,0,1,24,0,5,0,25,1,3,0,255,17,0,9,
13,12,3,18,0,12,2,10,0,1,2,1,25,2,0,0,24,1,0,0,255,255,255,14,
1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_424 [152] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,8,0,8,0,4,1,3,0,8,0,8,0,7,0,4,0,2,0,7,
0,7,0,12,0,2,7,'C',1,'K',10,0,0,'G',58,47,24,0,1,0,25,0,1,0,47,
24,0,5,0,25,0,0,0,255,2,14,1,2,1,24,0,2,0,25,1,0,0,1,24,0,3,0,
25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,255,17,0,9,13,12,3,18,0,12,
2,10,0,1,2,1,25,2,1,0,24,1,2,0,1,25,2,0,0,24,1,3,0,255,255,255,
14,1,1,21,8,0,0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_437 [112] =
   {	// blr string 

4,2,4,1,3,0,8,0,8,0,7,0,4,0,1,0,7,0,12,0,2,7,'C',1,'K',10,0,0,
'D',21,8,0,1,0,0,0,'G',58,47,24,0,5,0,25,0,0,0,59,61,24,0,1,0,
'F',1,'I',24,0,1,0,255,14,1,2,1,24,0,3,0,25,1,0,0,1,24,0,2,0,
25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,255,14,1,1,21,8,0,0,0,0,0,
25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_444 [193] =
   {	// blr string 

4,2,4,4,1,0,41,0,0,0,1,4,3,1,0,7,0,4,2,2,0,8,0,7,0,4,1,5,0,41,
0,0,0,1,8,0,7,0,7,0,7,0,4,0,1,0,41,0,0,0,1,12,0,2,7,'C',1,'K',
10,0,0,'G',47,24,0,0,0,25,0,0,0,255,2,14,1,2,1,24,0,0,0,25,1,
0,0,1,24,0,2,0,25,1,1,0,1,21,8,0,1,0,0,0,25,1,2,0,1,24,0,1,0,
25,1,3,0,1,24,0,5,0,25,1,4,0,255,17,0,9,13,12,3,18,0,12,2,10,
0,2,2,1,25,2,1,0,24,2,1,0,1,25,2,0,0,24,2,2,0,255,12,4,10,0,1,
2,1,25,4,0,0,24,1,0,0,255,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_460 [141] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,27,7,0,4,1,3,0,27,7,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',4,0,0,'G',47,24,0,0,0,25,0,0,0,255,
2,14,1,2,1,24,0,12,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,
0,2,0,25,1,2,0,255,17,0,9,13,12,3,18,0,12,2,10,0,1,2,1,25,2,0,
0,24,1,12,0,1,25,2,1,0,24,1,2,0,255,255,255,14,1,1,21,8,0,0,0,
0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_472 [148] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,27,7,0,4,1,3,0,27,7,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',1,'K',3,0,0,'G',47,24,0,0,0,25,0,0,0,'F',
1,'H',24,0,2,0,255,2,14,1,2,1,24,0,3,0,25,1,0,0,1,21,8,0,1,0,
0,0,25,1,1,0,1,24,0,2,0,25,1,2,0,255,17,0,9,13,12,3,18,0,12,2,
10,0,1,2,1,25,2,1,0,24,1,2,0,1,25,2,0,0,24,1,3,0,255,255,255,
14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 


using namespace Jrd;
using namespace Firebird;

namespace Jrd {

typedef Hash<
	DeferredWork,
	DefaultHash<DeferredWork>::DEFAULT_SIZE,
	DeferredWork,
	DefaultKeyValue<DeferredWork>,
	DeferredWork
> DfwHash;

class DeferredWork : public pool_alloc<type_dfw>,
					 public DfwHash::Entry
{
private:
	DeferredWork(const DeferredWork&);

public:
	enum dfw_t 		dfw_type;		// type of work deferred

private:
	DeferredWork***	dfw_end;
	DeferredWork**	dfw_prev;
	DeferredWork*	dfw_next;

public:
	Lock*			dfw_lock;		// relation creation lock
	Array<DeferredWork*>  dfw_args;	// arguments
	SLONG			dfw_sav_number;	// save point number
	USHORT			dfw_id;			// object id, if appropriate
	USHORT			dfw_count;		// count of block posts
	string			dfw_name;		// name of object

public:
	DeferredWork(MemoryPool& p, DeferredWork*** end,
				 enum dfw_t t, USHORT id, SLONG sn, const string& name)
	  : dfw_type(t), dfw_end(end), dfw_prev(dfw_end ? *dfw_end : NULL),
		dfw_next(dfw_prev ? *dfw_prev : NULL), dfw_lock(NULL), dfw_args(p),
		dfw_sav_number(sn), dfw_id(id), dfw_count(1), dfw_name(p, name)
	{
		// make previous element point to us
		if (dfw_prev)
		{
			*dfw_prev = this;
			// make next element (if present) to point to us
			if (dfw_next)
			{
				dfw_next->dfw_prev = &dfw_next;
			}
		}
	}

	~DeferredWork()
	{
		// if we are linked
		if (dfw_prev)
		{
			if (dfw_next)
			{
				// adjust previous pointer in next element ...
				dfw_next->dfw_prev = dfw_prev;
			}
			// adjust next pointer in previous element
			*dfw_prev = dfw_next;

			// Adjust end marker of the list
			if (*dfw_end == &dfw_next)
			{
				*dfw_end = dfw_prev;
			}
		}

		for (DeferredWork** itr = dfw_args.begin(); itr < dfw_args.end(); ++itr)
		{
			delete *itr;
		}

		if (dfw_lock)
		{
			LCK_release(JRD_get_thread_data(), dfw_lock);
			delete dfw_lock;
		}
	}

	DeferredWork* findArg(dfw_t type) const
	{
		for (DeferredWork* const* itr = dfw_args.begin(); itr < dfw_args.end(); ++itr)
		{
			DeferredWork* const arg = *itr;

			if (arg->dfw_type == type)
			{
				return arg;
			}
		}

		return NULL;
	}

	DeferredWork** getNextPtr()
	{
		return &dfw_next;
	}

	DeferredWork* getNext() const
	{
		return dfw_next;
	}

	// hash interface
	bool isEqual(const DeferredWork& work) const
	{
		if (dfw_type == work.dfw_type &&
			dfw_id == work.dfw_id &&
			dfw_name == work.dfw_name &&
			dfw_sav_number == work.dfw_sav_number)
		{
			return true;
		}
		return false;
	}

	DeferredWork* get() { return this; }

	static size_t hash(const DeferredWork& work, size_t hashSize)
	{
		const int nameLimit = 32;
		char key[sizeof work.dfw_type + sizeof work.dfw_id + nameLimit];
		memset(key, 0, sizeof key);
		char* place = key;

		memcpy(place, &work.dfw_type, sizeof work.dfw_type);
		place += sizeof work.dfw_type;

		memcpy(place, &work.dfw_id, sizeof work.dfw_id);
		place += sizeof work.dfw_id;

		work.dfw_name.copyTo(place, nameLimit);	// It's good enough to have first 32 bytes

		return DefaultHash<DeferredWork>::hash(key, sizeof key, hashSize);
	}
};

class DfwSavePoint;

typedef Hash<
	DfwSavePoint,
	DefaultHash<DfwSavePoint>::DEFAULT_SIZE,
	SLONG,
	DfwSavePoint
> DfwSavePointHash;

class DfwSavePoint : public DfwSavePointHash::Entry
{
	SLONG dfw_sav_number;

public:
	DfwHash hash;

	explicit DfwSavePoint(SLONG number) : dfw_sav_number(number) { }

	// hash interface
	bool isEqual(const SLONG& number) const
	{
		return dfw_sav_number == number;
	}

	DfwSavePoint* get() { return this; }

	static SLONG generate(const void* /*sender*/, const DfwSavePoint& item)
	{
		return item.dfw_sav_number;
	}
};

class DeferredJob
{
public:
	DfwSavePointHash hash;
	DeferredWork* work;
	DeferredWork** end;

	DeferredJob() : work(NULL), end(&work) { }
};

} // namespace Jrd

/*==================================================================
 *
 * NOTE:
 *
 *	The following functions required the same number of
 *	parameters to be passed.
 *
 *==================================================================
 */
static bool add_file(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool add_shadow(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_shadow(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool compute_security(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool modify_index(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_index(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_index(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_procedure(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_procedure(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool modify_procedure(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_relation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_relation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool scan_relation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_trigger(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_trigger(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool modify_trigger(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_collation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_collation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_exception(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_generator(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool modify_generator(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_udf(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_field(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_field(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool modify_field(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_global(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_parameter(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_rfr(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool make_version(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool add_difference(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_difference(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool begin_backup(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool end_backup(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool user_management(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool grant_privileges(thread_db*, SSHORT, DeferredWork*, jrd_tra*);

/* ---------------------------------------------------------------- */

static bool create_expression_index(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction);
static void check_computed_dependencies(thread_db* tdbb, jrd_tra* transaction,
	const Firebird::MetaName& fieldName);
static void check_dependencies(thread_db*, const TEXT*, const TEXT*, int, jrd_tra*);
static void check_filename(const Firebird::string&, bool);
static void check_system_generator(const TEXT*, const dfw_t);
static bool formatsAreEqual(const Format*, const Format*);
static bool	find_depend_in_dfw(thread_db*, TEXT*, USHORT, USHORT, jrd_tra*);
static void get_array_desc(thread_db*, const TEXT*, Ods::InternalArrayDesc*);
static void get_procedure_dependencies(DeferredWork*, bool, jrd_tra*);
static void get_trigger_dependencies(DeferredWork*, bool, jrd_tra*);
static void	load_trigs(thread_db*, jrd_rel*, trig_vec**);
static Format*	make_format(thread_db*, jrd_rel*, USHORT *, TemporaryField*);
static void put_summary_blob(thread_db* tdbb, blb*, enum rsr_t, bid*, jrd_tra*);
static void put_summary_record(thread_db* tdbb, blb*, enum rsr_t, const UCHAR*, USHORT);
static void	setup_array(thread_db*, blb*, const TEXT*, USHORT, TemporaryField*);
static blb*	setup_triggers(thread_db*, jrd_rel*, bool, trig_vec**, blb*);
static void	setup_trigger_details(thread_db*, jrd_rel*, blb*, trig_vec**, const TEXT*, bool);
static bool validate_text_type (thread_db*, const TemporaryField*);

static Lock* protect_relation(thread_db*, jrd_tra*, jrd_rel*, bool&);
static void release_protect_lock(thread_db*, jrd_tra*, Lock*);
static void check_partners(thread_db*, const USHORT);
static string get_string(const dsc* desc);

static const UCHAR nonnull_validation_blr[] =
{
	blr_version5,
	blr_not,
	blr_missing,
	blr_fid, 0, 0, 0,
	blr_eoc
};

typedef bool (*dfw_task_routine) (thread_db*, SSHORT, DeferredWork*, jrd_tra*);
struct deferred_task
{
	enum dfw_t task_type;
	dfw_task_routine task_routine;
};

static const deferred_task task_table[] =
{
	{ dfw_add_file, add_file },
	{ dfw_add_shadow, add_shadow },
	{ dfw_delete_index, modify_index },
	{ dfw_delete_expression_index, modify_index },
	{ dfw_delete_rfr, delete_rfr },
	{ dfw_delete_relation, delete_relation },
	{ dfw_delete_shadow, delete_shadow },
	{ dfw_create_field, create_field },
	{ dfw_delete_field, delete_field },
	{ dfw_modify_field, modify_field },
	{ dfw_delete_global, delete_global },
	{ dfw_create_relation, create_relation },
	{ dfw_update_format, make_version },
	{ dfw_scan_relation, scan_relation },
	{ dfw_compute_security, compute_security },
	{ dfw_create_index, modify_index },
	{ dfw_create_expression_index, modify_index },
	{ dfw_grant, grant_privileges },
	{ dfw_create_trigger, create_trigger },
	{ dfw_delete_trigger, delete_trigger },
	{ dfw_modify_trigger, modify_trigger },
	{ dfw_create_procedure, create_procedure },
	{ dfw_delete_procedure, delete_procedure },
	{ dfw_modify_procedure, modify_procedure },
	{ dfw_delete_prm, delete_parameter },
	{ dfw_create_collation, create_collation },
	{ dfw_delete_collation, delete_collation },
	{ dfw_delete_exception, delete_exception },
	{ dfw_delete_generator, delete_generator },
	{ dfw_modify_generator, modify_generator },
	{ dfw_delete_udf, delete_udf },
	{ dfw_add_difference, add_difference },
	{ dfw_delete_difference, delete_difference },
	{ dfw_begin_backup, begin_backup },
	{ dfw_end_backup, end_backup },
	{ dfw_user_management, user_management },
	{ dfw_null, NULL }
};


USHORT DFW_assign_index_type(thread_db* tdbb, const Firebird::string& name, SSHORT field_type, SSHORT ttype)
{
/**************************************
 *
 *	D F W _ a s s i g n _ i n d e x _ t y p e
 *
 **************************************
 *
 * Functional description
 *	Define the index segment type based
 * 	on the field's type and subtype.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (field_type == dtype_varying || field_type == dtype_cstring || field_type == dtype_text)
	{
	    switch (ttype)
	    {
		case ttype_none:
			return idx_string;
		case ttype_binary:
			return idx_byte_array;
		case ttype_metadata:
			return idx_metadata;
		case ttype_ascii:
			return idx_string;
		}

		/* Dynamic text cannot occur here as this is for an on-disk
		   index, which must be bound to a text type. */

		fb_assert(ttype != ttype_dynamic);

		if (INTL_defined_type(tdbb, ttype))
			return INTL_TEXT_TO_INDEX(ttype);

		ERR_post_nothrow(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_random) << Arg::Str(name));
		INTL_texttype_lookup(tdbb, ttype);	// should punt
		ERR_punt(); // if INTL_texttype_lookup hasn't punt
	}

	switch (field_type)
	{
	case dtype_timestamp:
		return idx_timestamp2;
	case dtype_sql_date:
		return idx_sql_date;
	case dtype_sql_time:
		return idx_sql_time;
	// idx_numeric2 used for 64-bit Integer support
	case dtype_int64:
		return idx_numeric2;
	default:
		return idx_numeric;
	}
}


void DFW_delete_deferred( jrd_tra* transaction, SLONG sav_number)
{
/**************************************
 *
 *	D F W _ d e l e t e _ d e f e r r e d
 *
 **************************************
 *
 * Functional description
 *	Get rid of work deferred that was to be done at
 *	COMMIT time as the statement has been rolled back.
 *
 *	if (sav_number == -1), then  remove all entries.
 *
 **************************************/

	// If there is no deferred work, just return

	if (!transaction->tra_deferred_job) {
		return;
	}

	// Remove deferred work and events which are to be rolled back

	if (sav_number == -1)
	{
		DeferredWork* work;
		while (work = transaction->tra_deferred_job->work)
		{
			delete work;
		}
		transaction->tra_flags &= ~TRA_deferred_meta;
		return;
	}

	DfwSavePoint* h = transaction->tra_deferred_job->hash.lookup(sav_number);
	if (!h)
	{
		return;
	}

	for (DfwHash::iterator i(h->hash); i.hasData();)
	{
		DeferredWork* work(i);
		++i;
		delete work;
	}
}


void DFW_merge_work(jrd_tra* transaction,
					SLONG old_sav_number,
					SLONG new_sav_number)
{
/**************************************
 *
 *	D F W _ m e r g e _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Merge the deferred work with the previous level.  This will
 *	be called only if there is a previous level.
 *
 **************************************/

	// If there is no deferred work, just return

	DeferredJob *job = transaction->tra_deferred_job;
	if (! job)
	{
		return;
	}

	// Check to see if work is already posted

	DfwSavePoint* oldSp = job->hash.lookup(old_sav_number);
	if (!oldSp)
	{
		return;
	}

	DfwSavePoint* newSp = job->hash.lookup(new_sav_number);

	// Decrement the save point number in the deferred block
	// i.e. merge with the previous level.

	for (DfwHash::iterator itr(oldSp->hash); itr.hasData();)
	{
		if (! newSp)
		{
			newSp = FB_NEW(*transaction->tra_pool) DfwSavePoint(new_sav_number);
			job->hash.add(newSp);
		}

		DeferredWork* work(itr);
		++itr;
		oldSp->hash.remove(*work); // After ++itr
		work->dfw_sav_number = new_sav_number;

		DeferredWork* foundWork = newSp->hash.lookup(*work);
		if (!foundWork)
		{
			newSp->hash.add(work);
		}
		else
		{
			foundWork->dfw_count += work->dfw_count;
			delete work;
		}
	}

	job->hash.remove(old_sav_number);
	delete oldSp;
}


void DFW_perform_system_work(thread_db* tdbb)
{
/**************************************
 *
 *	D F W _ p e r f o r m _ s y s t e m _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Flush out the work left to be done in the
 *	system transaction.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	Database::CheckoutLockGuard guard(dbb, dbb->dbb_sys_dfw_mutex);

	DFW_perform_work(tdbb, dbb->dbb_sys_trans);
}


void DFW_perform_work(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	D F W _ p e r f o r m _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Do work deferred to COMMIT time 'cause that time has
 *	come.
 *
 **************************************/

/* If no deferred work or it's all deferred event posting
   don't bother */

	if (!transaction->tra_deferred_job || !(transaction->tra_flags & TRA_deferred_meta))
	{
		return;
	}

	SET_TDBB(tdbb);
	Jrd::ContextPoolHolder context(tdbb, transaction->tra_pool);

/* Loop for as long as any of the deferred work routines says that it has
   more to do.  A deferred work routine should be able to deal with any
   value of phase, either to say that it wants to be called again in the
   next phase (by returning true) or that it has nothing more to do in this
   or later phases (by returning false). By convention, phase 0 has been
   designated as the cleanup phase. If any non-zero phase punts, then phase 0
   is executed for all deferred work blocks to cleanup work-in-progress. */
	bool dump_shadow = false;
	SSHORT phase = 1;
	bool more;
	ISC_STATUS_ARRAY err_status = {0};

	do
	{
		more = false;
		try {
			tdbb->tdbb_flags |= (TDBB_dont_post_dfw | TDBB_use_db_page_space);
			for (const deferred_task* task = task_table; task->task_type != dfw_null; ++task)
			{
				for (DeferredWork* work = transaction->tra_deferred_job->work; work; work = work->getNext())
				{
					if (work->dfw_type == task->task_type)
					{
						if (work->dfw_type == dfw_add_shadow)
						{
							dump_shadow = true;
						}
						if ((*task->task_routine)(tdbb, phase, work, transaction))
						{
							more = true;
						}
					}
				}
			}
			if (!phase) {
				Firebird::makePermanentVector(tdbb->tdbb_status_vector, err_status);
				ERR_punt();
			}
			++phase;
			tdbb->tdbb_flags &= ~(TDBB_dont_post_dfw | TDBB_use_db_page_space);
		}
		catch (const Firebird::Exception& ex) {
			tdbb->tdbb_flags &= ~(TDBB_dont_post_dfw | TDBB_use_db_page_space);

			/* Do any necessary cleanup */
			if (!phase) {
				Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
				ERR_punt();
			}
			else {
				Firebird::stuff_exception(err_status, ex);
			}
			phase = 0;
			more = true;
		}

	} while (more);

/* Remove deferred work blocks so that system transaction and
   commit retaining transactions don't re-execute them. Leave
   events to be posted after commit */

	for (DeferredWork* itr = transaction->tra_deferred_job->work; itr;)
	{
		DeferredWork* work = itr;
		itr = itr->getNext();

		switch (work->dfw_type)
		{
		case dfw_post_event:
		case dfw_delete_shadow:
			break;

		default:
			delete work;
			break;
		}
	}

	transaction->tra_flags &= ~TRA_deferred_meta;

	if (dump_shadow) {
		SDW_dump_pages(tdbb);
	}
}


void DFW_perform_post_commit_work(jrd_tra* transaction)
{
/**************************************
 *
 *	D F W _ p e r f o r m _ p o s t _ c o m m i t _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Perform any post commit work
 *	1. Post any pending events.
 *	2. Unlink shadow files for dropped shadows
 *
 *	Then, delete it from chain of pending work.
 *
 **************************************/

	if (!transaction->tra_deferred_job)
		return;

	bool pending_events = false;

	Database* dbb = GET_DBB();
	Lock* lock = dbb->dbb_lock;

	for (DeferredWork* itr = transaction->tra_deferred_job->work; itr;)
	{
		DeferredWork* work = itr;
		itr = itr->getNext();

		switch (work->dfw_type)
		{
		case dfw_post_event:
			EventManager::init(transaction->tra_attachment);

			dbb->dbb_event_mgr->postEvent(lock->lck_length, (const TEXT*) &lock->lck_key,
										  work->dfw_name.length(), work->dfw_name.c_str(),
										  work->dfw_count);

			delete work;
			pending_events = true;
			break;
		case dfw_delete_shadow:
			unlink(work->dfw_name.c_str());
			delete work;
			break;
		default:
			break;
		}
	}

	if (pending_events)
	{
		dbb->dbb_event_mgr->deliverEvents();
	}
}


DeferredWork* DFW_post_system_work(thread_db* tdbb, enum dfw_t type, const dsc* desc, USHORT id)
{
/**************************************
 *
 *	D F W _ p o s t _ s y s t e m _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Post work to be done in the context of system transaction.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	Database::CheckoutLockGuard guard(dbb, dbb->dbb_sys_dfw_mutex);

	return DFW_post_work(dbb->dbb_sys_trans, type, desc, id);
}


DeferredWork* DFW_post_work(jrd_tra* transaction, enum dfw_t type, const dsc* desc, USHORT id)
{
/**************************************
 *
 *	D F W _ p o s t _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Post work to be deferred to commit time.
 *
 **************************************/

	// get the current save point number

	const SLONG sav_number = transaction->tra_save_point ?
		transaction->tra_save_point->sav_number : 0;

	// initialize transaction if needed

	DeferredJob *job = transaction->tra_deferred_job;
	if (! job)
	{
		transaction->tra_deferred_job = job = FB_NEW(*transaction->tra_pool) DeferredJob;
	}

	// Check to see if work is already posted

	DfwSavePoint* sp = job->hash.lookup(sav_number);
	if (! sp)
	{
		sp = FB_NEW(*transaction->tra_pool) DfwSavePoint(sav_number);
		job->hash.add(sp);
	}

	const string name = get_string(desc);
	DeferredWork tmp(AutoStorage::getAutoMemoryPool(), 0, type, id, sav_number, name);
	DeferredWork* work = sp->hash.lookup(tmp);
	if (work)
	{
		work->dfw_count++;
		return work;
	}

	// Not already posted, so do so now.

	work = FB_NEW(*transaction->tra_pool)
		DeferredWork(*transaction->tra_pool, &(job->end), type, id, sav_number, name);
	job->end = work->getNextPtr();
	fb_assert(!(*job->end));
	sp->hash.add(work);

	switch (type)
	{
	case dfw_user_management:
		transaction->tra_flags |= TRA_deferred_meta;
		// fall down ...
	case dfw_post_event:
		if (transaction->tra_save_point)
			transaction->tra_save_point->sav_flags |= SAV_force_dfw;
		break;
	default:
		transaction->tra_flags |= TRA_deferred_meta;
		break;
	}

	return work;
}


DeferredWork* DFW_post_work_arg( jrd_tra* transaction, DeferredWork* work, const dsc* desc,
	USHORT id)
{
/**************************************
 *
 *	D F W _ p o s t _ w o r k _ a r g
 *
 **************************************
 *
 * Functional description
 *	Post an argument for work to be deferred to commit time.
 *
 **************************************/
	return DFW_post_work_arg(transaction, work, desc, id, work->dfw_type);
}


DeferredWork* DFW_post_work_arg( jrd_tra* transaction, DeferredWork* work, const dsc* desc,
	USHORT id, Jrd::dfw_t type)
{
/**************************************
 *
 *	D F W _ p o s t _ w o r k _ a r g
 *
 **************************************
 *
 * Functional description
 *	Post an argument for work to be deferred to commit time.
 *
 **************************************/
	const Firebird::string name = get_string(desc);

	DeferredWork* arg = work->findArg(type);

	if (! arg)
	{
		arg = FB_NEW(*transaction->tra_pool)
			DeferredWork(*transaction->tra_pool, 0, type, id, 0, name);
		work->dfw_args.add(arg);
	}

	return arg;
}


void DFW_update_index(const TEXT* name, USHORT id, const SelectivityList& selectivity, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_471;	// gds__utility 
   } jrd_470;
   struct {
          double  jrd_468;	// RDB$STATISTICS 
          SSHORT jrd_469;	// RDB$INDEX_ID 
   } jrd_467;
   struct {
          double  jrd_464;	// RDB$STATISTICS 
          SSHORT jrd_465;	// gds__utility 
          SSHORT jrd_466;	// RDB$INDEX_ID 
   } jrd_463;
   struct {
          TEXT  jrd_462 [32];	// RDB$INDEX_NAME 
   } jrd_461;
   struct {
          SSHORT jrd_483;	// gds__utility 
   } jrd_482;
   struct {
          double  jrd_480;	// RDB$STATISTICS 
          SSHORT jrd_481;	// RDB$FIELD_POSITION 
   } jrd_479;
   struct {
          double  jrd_476;	// RDB$STATISTICS 
          SSHORT jrd_477;	// gds__utility 
          SSHORT jrd_478;	// RDB$FIELD_POSITION 
   } jrd_475;
   struct {
          TEXT  jrd_474 [32];	// RDB$INDEX_NAME 
   } jrd_473;
/**************************************
 *
 *	D F W _ u p d a t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Update information in the index relation after creation
 *	of the index.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	if (dbb->dbb_ods_version >= ODS_VERSION11) {

		jrd_req* request = CMP_find_request(tdbb, irq_m_index_seg, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			SEG IN RDB$INDEX_SEGMENTS WITH SEG.RDB$INDEX_NAME EQ name
				SORTED BY SEG.RDB$FIELD_POSITION*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_472, sizeof(jrd_472), true);
		gds__vtov ((const char*) name, (char*) jrd_473.jrd_474, 32);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_473);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_475);
		   if (!jrd_475.jrd_477) break;
			if (!REQUEST(irq_m_index_seg))
				REQUEST(irq_m_index_seg) = request;
			/*MODIFY SEG USING*/
			{
			
				/*SEG.RDB$STATISTICS*/
				jrd_475.jrd_476 = selectivity[/*SEG.RDB$FIELD_POSITION*/
	       jrd_475.jrd_478];
			/*END_MODIFY;*/
			jrd_479.jrd_480 = jrd_475.jrd_476;
			jrd_479.jrd_481 = jrd_475.jrd_478;
			EXE_send (tdbb, request, 2, 10, (UCHAR*) &jrd_479);
			}
		/*END_FOR;*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_482);
		   }
		}

		if (!REQUEST(irq_m_index_seg))
			REQUEST(irq_m_index_seg) = request;
	}

	jrd_req* request = CMP_find_request(tdbb, irq_m_index, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES WITH IDX.RDB$INDEX_NAME EQ name*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_460, sizeof(jrd_460), true);
	gds__vtov ((const char*) name, (char*) jrd_461.jrd_462, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_461);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_463);
	   if (!jrd_463.jrd_465) break;
		if (!REQUEST(irq_m_index))
			REQUEST(irq_m_index) = request;
		/*MODIFY IDX USING*/
		{
		
			/*IDX.RDB$INDEX_ID*/
			jrd_463.jrd_466 = id + 1;
			/*IDX.RDB$STATISTICS*/
			jrd_463.jrd_464 = selectivity.back();
		/*END_MODIFY;*/
		jrd_467.jrd_468 = jrd_463.jrd_464;
		jrd_467.jrd_469 = jrd_463.jrd_466;
		EXE_send (tdbb, request, 2, 10, (UCHAR*) &jrd_467);
		}
	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_470);
	   }
	}

	if (!REQUEST(irq_m_index))
		REQUEST(irq_m_index) = request;
}


static bool add_file(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_436;	// gds__utility 
   } jrd_435;
   struct {
          SLONG  jrd_433;	// RDB$FILE_LENGTH 
          SLONG  jrd_434;	// RDB$FILE_START 
   } jrd_432;
   struct {
          SLONG  jrd_429;	// RDB$FILE_START 
          SLONG  jrd_430;	// RDB$FILE_LENGTH 
          SSHORT jrd_431;	// gds__utility 
   } jrd_428;
   struct {
          SSHORT jrd_426;	// RDB$SHADOW_NUMBER 
          SSHORT jrd_427;	// RDB$FILE_SEQUENCE 
   } jrd_425;
   struct {
          SLONG  jrd_441;	// RDB$FILE_LENGTH 
          SLONG  jrd_442;	// RDB$FILE_START 
          SSHORT jrd_443;	// gds__utility 
   } jrd_440;
   struct {
          SSHORT jrd_439;	// RDB$SHADOW_NUMBER 
   } jrd_438;
   struct {
          TEXT  jrd_459 [256];	// RDB$FILE_NAME 
   } jrd_458;
   struct {
          SSHORT jrd_457;	// gds__utility 
   } jrd_456;
   struct {
          SLONG  jrd_454;	// RDB$FILE_START 
          SSHORT jrd_455;	// RDB$FILE_SEQUENCE 
   } jrd_453;
   struct {
          TEXT  jrd_448 [256];	// RDB$FILE_NAME 
          SLONG  jrd_449;	// RDB$FILE_START 
          SSHORT jrd_450;	// gds__utility 
          SSHORT jrd_451;	// RDB$FILE_SEQUENCE 
          SSHORT jrd_452;	// RDB$SHADOW_NUMBER 
   } jrd_447;
   struct {
          TEXT  jrd_446 [256];	// RDB$FILE_NAME 
   } jrd_445;
/**************************************
 *
 *	a d d _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Add a file to a database.
 *	This file could be a regular database
 *	file or a shadow file.  Either way we
 *	require exclusive access to the database.
 *
 **************************************/
	USHORT section, shadow_number;
	SLONG start, max;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		CCH_release_exclusive(tdbb);
		return false;

	case 1:
	case 2:
		return true;

	case 3:
		if (CCH_exclusive(tdbb, LCK_EX, WAIT_PERIOD))
			return true;

		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_lock_timeout) <<
				 Arg::Gds(isc_obj_in_use) << Arg::Str(dbb->dbb_filename));
		return false;

	case 4:
		CCH_flush(tdbb, FLUSH_FINI, 0L);
		max = PageSpace::maxAlloc(dbb) + 1;
		jrd_req* handle = 0;
		jrd_req* handle2 = 0;

		// Check the file name for node name. This has already
		// been done for shadows in add_shadow()

		if (work->dfw_type != dfw_add_shadow) {
			check_filename(work->dfw_name, true);
		}

		// User transaction may be safely used instead of system, cause
		// we requested and got exclusive database access. AP-2008.

		// get any files to extend into

		/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction) X IN RDB$FILES
			WITH X.RDB$FILE_NAME EQ work->dfw_name.c_str()*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_444, sizeof(jrd_444), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_445.jrd_446, 256);
		EXE_start (tdbb, handle, transaction);
		EXE_send (tdbb, handle, 0, 256, (UCHAR*) &jrd_445);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 266, (UCHAR*) &jrd_447);
		   if (!jrd_447.jrd_450) break;
				/* First expand the file name This has already been done
				   ** for shadows in add_shadow ()) */
			if (work->dfw_type != dfw_add_shadow)
			{
				/*MODIFY X USING*/
				{
				
					ISC_expand_filename(/*X.RDB$FILE_NAME*/
							    jrd_447.jrd_448, 0,
						/*X.RDB$FILE_NAME*/
						jrd_447.jrd_448, sizeof(/*X.RDB$FILE_NAME*/
	 jrd_447.jrd_448), false);
				/*END_MODIFY;*/
				gds__vtov((const char*) jrd_447.jrd_448, (char*) jrd_458.jrd_459, 256);
				EXE_send (tdbb, handle, 4, 256, (UCHAR*) &jrd_458);
				}
			}

			/* If there is no starting position specified, or if it is
			   too low a value, make a stab at assigning one based on
			   the indicated preference for the previous file length. */

			if ((start = /*X.RDB$FILE_START*/
				     jrd_447.jrd_449) < max)
			{
				/*FOR(REQUEST_HANDLE handle2 TRANSACTION_HANDLE transaction)
					FIRST 1 Y IN RDB$FILES
						WITH Y.RDB$SHADOW_NUMBER EQ X.RDB$SHADOW_NUMBER
						AND Y.RDB$FILE_SEQUENCE NOT MISSING
						SORTED BY DESCENDING Y.RDB$FILE_SEQUENCE*/
				{
				if (!handle2)
				   handle2 = CMP_compile2 (tdbb, (UCHAR*) jrd_437, sizeof(jrd_437), true);
				jrd_438.jrd_439 = jrd_447.jrd_452;
				EXE_start (tdbb, handle2, transaction);
				EXE_send (tdbb, handle2, 0, 2, (UCHAR*) &jrd_438);
				while (1)
				   {
				   EXE_receive (tdbb, handle2, 1, 10, (UCHAR*) &jrd_440);
				   if (!jrd_440.jrd_443) break;
						start = /*Y.RDB$FILE_START*/
							jrd_440.jrd_442 + /*Y.RDB$FILE_LENGTH*/
   jrd_440.jrd_441;
				/*END_FOR;*/
				   }
				}
			}

			start = MAX(max, start);
			shadow_number = /*X.RDB$SHADOW_NUMBER*/
					jrd_447.jrd_452;
			if ((shadow_number &&
				(section = SDW_add_file(tdbb, /*X.RDB$FILE_NAME*/
							      jrd_447.jrd_448, start, shadow_number))) ||
				(section = PAG_add_file(tdbb, /*X.RDB$FILE_NAME*/
							      jrd_447.jrd_448, start)))
			{
				/*MODIFY X USING*/
				{
				
					/*X.RDB$FILE_SEQUENCE*/
					jrd_447.jrd_451 = section;
					/*X.RDB$FILE_START*/
					jrd_447.jrd_449 = start;
				/*END_MODIFY;*/
				jrd_453.jrd_454 = jrd_447.jrd_449;
				jrd_453.jrd_455 = jrd_447.jrd_451;
				EXE_send (tdbb, handle, 2, 6, (UCHAR*) &jrd_453);
				}
			}
		/*END_FOR;*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_456);
		   }
		}

		CMP_release(tdbb, handle);
		if (handle2)
		{
			CMP_release(tdbb, handle2);
		}

		if (section)
		{
			handle = NULL;
			section--;
			/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction) X IN RDB$FILES
				WITH X.RDB$FILE_SEQUENCE EQ section
					AND X.RDB$SHADOW_NUMBER EQ shadow_number*/
			{
			if (!handle)
			   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_424, sizeof(jrd_424), true);
			jrd_425.jrd_426 = shadow_number;
			jrd_425.jrd_427 = section;
			EXE_start (tdbb, handle, transaction);
			EXE_send (tdbb, handle, 0, 4, (UCHAR*) &jrd_425);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_428);
			   if (!jrd_428.jrd_431) break;
				/*MODIFY X USING*/
				{
				
					/*X.RDB$FILE_LENGTH*/
					jrd_428.jrd_430 = start - /*X.RDB$FILE_START*/
	   jrd_428.jrd_429;
				/*END_MODIFY;*/
				jrd_432.jrd_433 = jrd_428.jrd_430;
				jrd_432.jrd_434 = jrd_428.jrd_429;
				EXE_send (tdbb, handle, 2, 8, (UCHAR*) &jrd_432);
				}
			/*END_FOR;*/
			   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_435);
			   }
			}
			CMP_release(tdbb, handle);
		}

		CCH_release_exclusive(tdbb);
		break;
	}

	return false;
}



static bool add_shadow( thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_411;	// gds__utility 
   } jrd_410;
   struct {
          SLONG  jrd_407;	// RDB$FILE_START 
          SSHORT jrd_408;	// RDB$FILE_FLAGS 
          SSHORT jrd_409;	// RDB$FILE_SEQUENCE 
   } jrd_406;
   struct {
          TEXT  jrd_400 [256];	// RDB$FILE_NAME 
          SLONG  jrd_401;	// RDB$FILE_START 
          SSHORT jrd_402;	// gds__utility 
          SSHORT jrd_403;	// RDB$FILE_SEQUENCE 
          SSHORT jrd_404;	// RDB$FILE_FLAGS 
          SSHORT jrd_405;	// RDB$SHADOW_NUMBER 
   } jrd_399;
   struct {
          TEXT  jrd_398 [256];	// RDB$FILE_NAME 
   } jrd_397;
   struct {
          SSHORT jrd_423;	// gds__utility 
   } jrd_422;
   struct {
          TEXT  jrd_421 [256];	// RDB$FILE_NAME 
   } jrd_420;
   struct {
          TEXT  jrd_416 [256];	// RDB$FILE_NAME 
          SSHORT jrd_417;	// gds__utility 
          SSHORT jrd_418;	// RDB$FILE_FLAGS 
          SSHORT jrd_419;	// RDB$SHADOW_NUMBER 
   } jrd_415;
   struct {
          TEXT  jrd_414 [256];	// RDB$FILE_NAME 
   } jrd_413;
/**************************************
 *
 *	a d d _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	A file or files have been added for shadowing.
 *	Get all files for this particular shadow first
 *	in order of starting page, if specified, then
 *	in sequence order.
 *
 **************************************/

	jrd_req* handle;
	Shadow* shadow;
	USHORT sequence, add_sequence;
	bool finished;
	ULONG min_page;
	Firebird::PathName expanded_fname;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		CCH_release_exclusive(tdbb);
		return false;

	case 1:
	case 2:
	case 3:
		return true;

	case 4:
		check_filename(work->dfw_name, false);

		/* could have two cases:
		   1) this shadow has already been written to, so add this file using
		   the standard routine to extend a database
		   2) this file is part of a newly added shadow which has already been
		   fetched in totem and prepared for writing to, so just ignore it
		 */

		finished = false;
		handle = NULL;
		/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
			F IN RDB$FILES
				WITH F.RDB$FILE_NAME EQ work->dfw_name.c_str()*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_412, sizeof(jrd_412), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_413.jrd_414, 256);
		EXE_start (tdbb, handle, transaction);
		EXE_send (tdbb, handle, 0, 256, (UCHAR*) &jrd_413);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 262, (UCHAR*) &jrd_415);
		   if (!jrd_415.jrd_417) break;

			expanded_fname = /*F.RDB$FILE_NAME*/
					 jrd_415.jrd_416;
			ISC_expand_filename(expanded_fname, false);
			/*MODIFY F USING*/
			{
			
				expanded_fname.copyTo(/*F.RDB$FILE_NAME*/
						      jrd_415.jrd_416, sizeof(/*F.RDB$FILE_NAME*/
	 jrd_415.jrd_416));
			/*END_MODIFY;*/
			gds__vtov((const char*) jrd_415.jrd_416, (char*) jrd_420.jrd_421, 256);
			EXE_send (tdbb, handle, 2, 256, (UCHAR*) &jrd_420);
			}

			for (shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
			{
				if ((/*F.RDB$SHADOW_NUMBER*/
				     jrd_415.jrd_419 == shadow->sdw_number) && !(shadow->sdw_flags & SDW_IGNORE))
				{
					if (/*F.RDB$FILE_FLAGS*/
					    jrd_415.jrd_418 & FILE_shadow)
					{
						/* This is the case of a bogus duplicate posted
						 * work when we added a multi-file shadow
						 */
						finished = true;
					}
					else if (shadow->sdw_flags & (SDW_dumped))
					{
						/* Case of adding a file to a currently active
						 * shadow set.
						 * Note: as of 1995-January-31 there is
						 * no SQL syntax that supports this, but there
						 * may be GDML
						 */
						add_file(tdbb, 3, work, transaction);
						add_file(tdbb, 4, work, transaction);
						finished = true;
					}
					else
					{
						/* We cannot add a file to a shadow that is still
						 * in the process of being created.
						 */
						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_obj_in_use) << Arg::Str(dbb->dbb_filename));
					}
					break;
				}
			}

		/*END_FOR;*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_422);
		   }
		}
		CMP_release(tdbb, handle);

		if (finished) {
			return false;
		}

		/* this file is part of a new shadow, so get all files for the shadow
		   in order of the starting page for the file */

		/* Note that for a multi-file shadow, we have several pieces of
		 * work posted (one dfw_add_shadow for each file).  Rather than
		 * trying to cancel the other pieces of work we ignore them
		 * when they arrive in this routine.
		 */

		sequence = 0;
		min_page = 0;
		shadow = NULL;
		handle = NULL;
		/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
			X IN RDB$FILES CROSS
				Y IN RDB$FILES
				OVER RDB$SHADOW_NUMBER
				WITH X.RDB$FILE_NAME EQ expanded_fname.c_str()
				SORTED BY Y.RDB$FILE_START*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_396, sizeof(jrd_396), true);
		gds__vtov ((const char*) expanded_fname.c_str(), (char*) jrd_397.jrd_398, 256);
		EXE_start (tdbb, handle, transaction);
		EXE_send (tdbb, handle, 0, 256, (UCHAR*) &jrd_397);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 268, (UCHAR*) &jrd_399);
		   if (!jrd_399.jrd_402) break;
				/* for the first file, create a brand new shadow; for secondary
				   files that have a starting page specified, add a file */
			if (!sequence)
				SDW_add(tdbb, /*Y.RDB$FILE_NAME*/
					      jrd_399.jrd_400, /*Y.RDB$SHADOW_NUMBER*/
  jrd_399.jrd_405, /*Y.RDB$FILE_FLAGS*/
  jrd_399.jrd_404);
			else if (/*Y.RDB$FILE_START*/
				 jrd_399.jrd_401)
			{
				if (!shadow)
				{
					for (shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
					{
						if ((/*Y.RDB$SHADOW_NUMBER*/
						     jrd_399.jrd_405 == shadow->sdw_number) &&
							!(shadow->sdw_flags & SDW_IGNORE))
						{
								break;
						}
					}
				}

				if (!shadow)
					BUGCHECK(203);	/* msg 203 shadow block not found for extend file */

				min_page = MAX((min_page + 1), (ULONG) /*Y.RDB$FILE_START*/
								       jrd_399.jrd_401);
				add_sequence = SDW_add_file(tdbb, /*Y.RDB$FILE_NAME*/
								  jrd_399.jrd_400, min_page, /*Y.RDB$SHADOW_NUMBER*/
	    jrd_399.jrd_405);
			}

			/* update the sequence number and bless the file entry as being
			   good */

			if (!sequence || (/*Y.RDB$FILE_START*/
					  jrd_399.jrd_401 && add_sequence))
			{
				/*MODIFY Y*/
				{
				
					/*Y.RDB$FILE_FLAGS*/
					jrd_399.jrd_404 |= FILE_shadow;
					/*Y.RDB$FILE_SEQUENCE*/
					jrd_399.jrd_403 = sequence;
					/*Y.RDB$FILE_START*/
					jrd_399.jrd_401 = min_page;
				/*END_MODIFY;*/
				jrd_406.jrd_407 = jrd_399.jrd_401;
				jrd_406.jrd_408 = jrd_399.jrd_404;
				jrd_406.jrd_409 = jrd_399.jrd_403;
				EXE_send (tdbb, handle, 2, 8, (UCHAR*) &jrd_406);
				}
				sequence++;
			}

		/*END_FOR;*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_410);
		   }
		}
		CMP_release(tdbb, handle);
		break;
	}

	return false;
}

static bool add_difference( thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
/**************************************
 *
 *	a d d _ d i f f e r e n c e
 *
 **************************************
 *
 * Functional description
 *	Add backup difference file to the database
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	if (dbb->dbb_ods_version < ODS_VERSION11)
		ERR_post(Arg::Gds(isc_wish_list));

	switch (phase)
	{
	case 0:
		return false;

	case 1:
	case 2:
		return true;
	case 3:
		{
			BackupManager::StateReadGuard stateGuard(tdbb);
			if (dbb->dbb_backup_manager->getState() != nbak_state_normal)
			{
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_wrong_backup_state));
			}
			check_filename(work->dfw_name, true);
			dbb->dbb_backup_manager->setDifference(tdbb, work->dfw_name.c_str());
		}

		return false;
	}

	return false;
}


static bool delete_difference(	thread_db*	tdbb,
								SSHORT	phase,
								DeferredWork*,
								jrd_tra*)
{
/**************************************
 *
 *	d e l e t e _ d i f f e r e n c e
 *
 **************************************
 *
 * Delete backup difference file for database
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	if (dbb->dbb_ods_version < ODS_VERSION11)
		ERR_post(Arg::Gds(isc_wish_list));

	switch (phase)
	{
	case 0:
		return false;
	case 1:
	case 2:
		return true;
	case 3:
		{
			BackupManager::StateReadGuard stateGuard(tdbb);

			if (dbb->dbb_backup_manager->getState() != nbak_state_normal)
			{
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_wrong_backup_state));
			}
			dbb->dbb_backup_manager->setDifference(tdbb, NULL);
		}

		return false;
	}

	return false;
}

static bool begin_backup(	thread_db*	tdbb,
							SSHORT	phase,
							DeferredWork*,
							jrd_tra*)
{
/**************************************
 *
 *	b e g i n _ b a c k u p
 *
 **************************************
 *
 * Begin backup storing changed pages in difference file
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	if (dbb->dbb_ods_version < ODS_VERSION11)
		ERR_post(Arg::Gds(isc_wish_list));

	switch (phase)
	{
	case 0:
		return false;
	case 1:
	case 2:
		return true;
	case 3:
		dbb->dbb_backup_manager->beginBackup(tdbb);
		return false;
	}

	return false;
}

static bool end_backup(	thread_db*	tdbb,
						SSHORT	phase,
						DeferredWork*,
						jrd_tra*)
{
/**************************************
 *
 *	e n d _ b a c k u p
 *
 **************************************
 *
 * End backup and merge difference file if neseccary
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	if (dbb->dbb_ods_version < ODS_VERSION11)
		ERR_post(Arg::Gds(isc_wish_list));

	switch (phase)
	{
	case 0:
		return false;
	case 1:
	case 2:
		return true;
	case 3:
		// End backup normally
		dbb->dbb_backup_manager->endBackup(tdbb, false);
		return false;
	}

	return false;
}

static bool user_management(thread_db*		tdbb,
							SSHORT			phase,
							DeferredWork*	work,
							jrd_tra*		transaction)
{
/**************************************
 *
 *	u s e r _ m a n a g e m e n t
 *
 **************************************
 *
 * Commit in security database
 *
 **************************************/

	switch (phase)
	{
	case 0:
		return false;
	case 1:
	case 2:
		return true;
	case 3:
		transaction->getUserManagement()->execute(work->dfw_id);
		return true;
	case 4:
		transaction->getUserManagement()->commit();	// safe to be called multiple times
		return false;
	}

	return false;
}


static bool grant_privileges(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	g r a n t _ p r i v i l e g e s
 *
 **************************************
 *
 * Functional description
 *	Compute access control list from SQL privileges.
 *
 **************************************/
	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		GRANT_privileges(tdbb, work->dfw_name, work->dfw_id, transaction);
		break;

	default:
		break;
	}

	return false;
}


static bool create_expression_index(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
/**************************************
 *
 *	c r e a t e  _ e x p r e s s i o n _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Create a new expression index.
 *
 **************************************/
	switch (phase)
	{
	case 0:
		MET_delete_dependencies(tdbb, work->dfw_name, obj_expression_index, transaction);
		return false;

	case 1:
	case 2:
		return true;

	case 3:
		PCMET_expression_index(tdbb, work->dfw_name, work->dfw_id, transaction);
		break;

	default:
		break;
	}

	return false;
}


static void check_computed_dependencies(thread_db* tdbb, jrd_tra* transaction,
	const Firebird::MetaName& fieldName)
{
   struct {
          TEXT  jrd_394 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_395;	// gds__utility 
   } jrd_393;
   struct {
          TEXT  jrd_390 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_391;	// RDB$DEPENDED_ON_TYPE 
          SSHORT jrd_392;	// RDB$DEPENDENT_TYPE 
   } jrd_389;
/**************************************
 *
 *	c h e c k _ c o m p u t e d _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *	Checks if a computed field has circular dependencies.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	bool err = false;
	Firebird::SortedObjectsArray<Firebird::MetaName> sortedNames(*tdbb->getDefaultPool());
	Firebird::ObjectsArray<Firebird::MetaName> names;

	sortedNames.add(fieldName);
	names.add(fieldName);

	for (size_t pos = 0; !err && pos < names.getCount(); ++pos)
	{
		jrd_req* request = CMP_find_request(tdbb, irq_comp_circ_dpd, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			DEP IN RDB$DEPENDENCIES CROSS
			RFL IN RDB$RELATION_FIELDS WITH
				DEP.RDB$DEPENDENT_NAME EQ names[pos].c_str() AND
				DEP.RDB$DEPENDENT_TYPE EQ obj_computed AND
				DEP.RDB$DEPENDED_ON_TYPE = obj_relation AND
				RFL.RDB$RELATION_NAME = DEP.RDB$DEPENDED_ON_NAME AND
				RFL.RDB$FIELD_NAME = DEP.RDB$FIELD_NAME*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_388, sizeof(jrd_388), true);
		gds__vtov ((const char*) names[pos].c_str(), (char*) jrd_389.jrd_390, 32);
		jrd_389.jrd_391 = obj_relation;
		jrd_389.jrd_392 = obj_computed;
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 36, (UCHAR*) &jrd_389);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_393);
		   if (!jrd_393.jrd_395) break;
		{
			if (!REQUEST(irq_comp_circ_dpd))
				REQUEST(irq_comp_circ_dpd) = request;

			Firebird::MetaName fieldSource(/*RFL.RDB$FIELD_SOURCE*/
						       jrd_393.jrd_394);

			if (fieldName == fieldSource)
			{
				err = true;
				break;
			}

			if (!sortedNames.exist(fieldSource))
			{
				sortedNames.add(fieldSource);
				names.add(fieldSource);
			}
		}
		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_comp_circ_dpd))
			REQUEST(irq_comp_circ_dpd) = request;
	}

	if (err)
	{
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_circular_computed));
	}
}


static void check_dependencies(thread_db* tdbb,
							   const TEXT* dpdo_name,
							   const TEXT* field_name,
							   int dpdo_type,
							   jrd_tra* transaction)
{
   struct {
          TEXT  jrd_376 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_377;	// gds__utility 
          SSHORT jrd_378;	// RDB$DEPENDENT_TYPE 
   } jrd_375;
   struct {
          TEXT  jrd_373 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_374;	// RDB$DEPENDED_ON_TYPE 
   } jrd_372;
   struct {
          TEXT  jrd_385 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_386;	// gds__utility 
          SSHORT jrd_387;	// RDB$DEPENDENT_TYPE 
   } jrd_384;
   struct {
          TEXT  jrd_381 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_382 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_383;	// RDB$DEPENDED_ON_TYPE 
   } jrd_380;
/**************************************
 *
 *	c h e c k _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *	Check the dependency list for relation or relation.field
 *	before deleting such.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	SLONG dep_counts[obj_type_MAX];
	for (int i = 0; i < obj_type_MAX; i++)
		dep_counts[i] = 0;

	if (field_name)
	{
		jrd_req* request = CMP_find_request(tdbb, irq_ch_f_dpd, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			DEP IN RDB$DEPENDENCIES
				WITH DEP.RDB$DEPENDED_ON_NAME EQ dpdo_name
				AND DEP.RDB$DEPENDED_ON_TYPE = dpdo_type
				AND DEP.RDB$FIELD_NAME EQ field_name
				REDUCED TO DEP.RDB$DEPENDENT_NAME*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_379, sizeof(jrd_379), true);
		gds__vtov ((const char*) field_name, (char*) jrd_380.jrd_381, 32);
		gds__vtov ((const char*) dpdo_name, (char*) jrd_380.jrd_382, 32);
		jrd_380.jrd_383 = dpdo_type;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_380);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_384);
		   if (!jrd_384.jrd_386) break;
			if (!REQUEST(irq_ch_f_dpd))
				REQUEST(irq_ch_f_dpd) = request;

			/* If the found object is also being deleted, there's no dependency */

			if (!find_depend_in_dfw(tdbb, /*DEP.RDB$DEPENDENT_NAME*/
						      jrd_384.jrd_385, /*DEP.RDB$DEPENDENT_TYPE*/
  jrd_384.jrd_387,
									0, transaction))
			{
				++dep_counts[/*DEP.RDB$DEPENDENT_TYPE*/
					     jrd_384.jrd_387];
			}
		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_ch_f_dpd))
			REQUEST(irq_ch_f_dpd) = request;
	}
	else
	{
		jrd_req* request = CMP_find_request(tdbb, irq_ch_dpd, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			DEP IN RDB$DEPENDENCIES
				WITH DEP.RDB$DEPENDED_ON_NAME EQ dpdo_name
				AND DEP.RDB$DEPENDED_ON_TYPE = dpdo_type
				REDUCED TO DEP.RDB$DEPENDENT_NAME*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_371, sizeof(jrd_371), true);
		gds__vtov ((const char*) dpdo_name, (char*) jrd_372.jrd_373, 32);
		jrd_372.jrd_374 = dpdo_type;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_372);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_375);
		   if (!jrd_375.jrd_377) break;

			if (!REQUEST(irq_ch_dpd))
				REQUEST(irq_ch_dpd) = request;

			/* If the found object is also being deleted, there's no dependency */

			if (!find_depend_in_dfw(tdbb, /*DEP.RDB$DEPENDENT_NAME*/
						      jrd_375.jrd_376, /*DEP.RDB$DEPENDENT_TYPE*/
  jrd_375.jrd_378,
									0, transaction))
			{
				++dep_counts[/*DEP.RDB$DEPENDENT_TYPE*/
					     jrd_375.jrd_378];
			}
		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_ch_dpd))
			REQUEST(irq_ch_dpd) = request;
	}

	SLONG total = 0;
	for (int i = 0; i < obj_type_MAX; i++)
		total += dep_counts[i];

	if (!total)
		return;

	if (field_name)
	{
		string fld_name(dpdo_name);
		fld_name.append(".");
		fld_name.append(field_name);

		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_no_delete) <<						/* Msg353: can not delete */
				 Arg::Gds(isc_field_name) << Arg::Str(fld_name) <<
				 Arg::Gds(isc_dependency) << Arg::Num(total));	/* Msg310: there are %ld dependencies */
	}
	else
	{
		ISC_STATUS obj_type;
		switch (dpdo_type)
		{
		case obj_relation:
			obj_type = isc_table_name;
			break;
		case obj_view:
			obj_type = isc_table_name /*isc_view_name*/;
			break;
		case obj_procedure:
			obj_type = isc_proc_name;
			break;
		case obj_collation:
			obj_type = isc_collation_name;
			break;
		case obj_exception:
			obj_type = isc_exception_name;
			break;
		case obj_field:
			obj_type = isc_domain_name;
			break;
		case obj_generator:
			obj_type = isc_generator_name;
			break;
		case obj_udf:
			obj_type = isc_udf_name;
			break;
		case obj_index:
			obj_type = isc_index_name;
			break;
		default:
			fb_assert(FALSE);
			break;
		}

		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_no_delete) <<						/* can not delete */
				 Arg::Gds(obj_type) << Arg::Str(dpdo_name) <<
				 Arg::Gds(isc_dependency) << Arg::Num(total));	/* there are %ld dependencies */
	}
}


static void check_filename(const Firebird::string& name, bool shareExpand)
{
/**************************************
 *
 *	c h e c k _ f i l e n a m e
 *
 **************************************
 *
 * Functional description
 *	Make sure that a file path doesn't contain an
 *	inet node name.
 *
 **************************************/
	const Firebird::PathName file_name(name.ToPathName());
	const bool valid = file_name.find("::") == Firebird::PathName::npos;

	if (!valid || ISC_check_if_remote(file_name, shareExpand)) {
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_node_name_err));
		/* Msg305: A node name is not permitted in a secondary, shadow, or log file name */
	}

	if (!JRD_verify_database_access(file_name)) {
		ERR_post(Arg::Gds(isc_conf_access_denied) << Arg::Str("additional database file") <<
													 Arg::Str(name));
	}
}


static void check_system_generator(const TEXT* gen_name, const dfw_t action)
{
	// CVC: Replace this with a call to SCL when we have ACL's for gens.
	for (const gen* generator = generators; generator->gen_name; generator++)
	{
		if (!strcmp(generator->gen_name, gen_name)) // did we find a sys gen?
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(action == dfw_delete_generator ? isc_no_delete : isc_no_update) <<
												   // Msg353: can not delete   Msg520: can not update
					 Arg::Gds(isc_generator_name) << Arg::Str(gen_name) <<
					 Arg::Gds(isc_random) << Arg::Str("This is a system generator."));
		}
	}
}


static bool formatsAreEqual(const Format* old_format, const Format* new_format)
{
/**************************************
 *
 * Functional description
 *	Compare two format blocks
 *
 **************************************/

	if ((old_format->fmt_length != new_format->fmt_length) ||
		(old_format->fmt_count != new_format->fmt_count))
	{
		return false;
	}

	Format::fmt_desc_const_iterator old_desc = old_format->fmt_desc.begin();
	const Format::fmt_desc_const_iterator old_end = old_format->fmt_desc.end();

	Format::fmt_desc_const_iterator new_desc = new_format->fmt_desc.begin();

	while (old_desc != old_end)
	{
		if ((old_desc->dsc_dtype != new_desc->dsc_dtype) ||
			(old_desc->dsc_scale != new_desc->dsc_scale) ||
			(old_desc->dsc_length != new_desc->dsc_length) ||
			(old_desc->dsc_sub_type != new_desc->dsc_sub_type) ||
			(old_desc->dsc_flags != new_desc->dsc_flags) ||
			(old_desc->dsc_address != new_desc->dsc_address))
		{
			return false;
		}

		++new_desc;
		++old_desc;
	}

	return true;
}


static bool compute_security(thread_db*	tdbb,
							SSHORT	phase,
							DeferredWork*		work,
							jrd_tra*)
{
   struct {
          SSHORT jrd_370;	// gds__utility 
   } jrd_369;
   struct {
          TEXT  jrd_368 [32];	// RDB$SECURITY_CLASS 
   } jrd_367;
/**************************************
 *
 *	c o m p u t e _ s e c u r i t y
 *
 **************************************
 *
 * Functional description
 *	There was a change in a security class.  Recompute everything
 *	it touches.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			/* Get security class.  This may return NULL if it doesn't exist */

			SecurityClass* s_class = SCL_recompute_class(tdbb, work->dfw_name.c_str());

			jrd_req* handle = NULL;
			/*FOR(REQUEST_HANDLE handle) X IN RDB$DATABASE
				WITH X.RDB$SECURITY_CLASS EQ work->dfw_name.c_str()*/
			{
			if (!handle)
			   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_366, sizeof(jrd_366), true);
			gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_367.jrd_368, 32);
			EXE_start (tdbb, handle, dbb->dbb_sys_trans);
			EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_367);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_369);
			   if (!jrd_369.jrd_370) break;

				tdbb->getAttachment()->att_security_class = s_class;

			/*END_FOR;*/
			   }
			}
			CMP_release(tdbb, handle);
		}
		break;
	}

	return false;
}

static bool modify_index(thread_db*	tdbb,
						SSHORT	phase,
						DeferredWork*		work,
						jrd_tra*		transaction)
{
   struct {
          SSHORT jrd_363;	// gds__utility 
          SSHORT jrd_364;	// RDB$RELATION_ID 
          SSHORT jrd_365;	// RDB$RELATION_TYPE 
   } jrd_362;
   struct {
          TEXT  jrd_361 [32];	// RDB$INDEX_NAME 
   } jrd_360;
/**************************************
 *
 *	m o d i f y _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Create\drop an index or change the state of an index between active/inactive.
 *	If index owns by global temporary table with on commit preserve rows scope
 *	change index instance for this temporary table too. For "create index" work
 *	item create base index instance before temp index instance. For index
 *	deletion delete temp index instance first to release index usage counter
 *	before deletion of base index instance.
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	const bool have_gtt = (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_version) >= ODS_11_1);

	bool is_create = true;
	dfw_task_routine task_routine = NULL;

	switch (work->dfw_type)
	{
		case dfw_create_index :
			task_routine = create_index;
			break;

		case dfw_create_expression_index :
			task_routine = create_expression_index;
			break;

		case dfw_delete_index :
		case dfw_delete_expression_index :
			task_routine = delete_index;
			is_create = false;
			break;
	}
	fb_assert(task_routine);

	bool more = false, more2 = false;

	if (is_create) {
		more = (*task_routine)(tdbb, phase, work, transaction);
	}

	if (have_gtt)
	{
		bool gtt_preserve = false;
		jrd_rel* relation = NULL;

		if (is_create)
		{
			jrd_req* request = NULL;

			/*FOR(REQUEST_HANDLE request)
				IDX IN RDB$INDICES CROSS
				REL IN RDB$RELATIONS OVER RDB$RELATION_NAME WITH
					IDX.RDB$INDEX_NAME EQ work->dfw_name.c_str() AND
					REL.RDB$RELATION_ID NOT MISSING*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_359, sizeof(jrd_359), true);
			gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_360.jrd_361, 32);
			EXE_start (tdbb, request, dbb->dbb_sys_trans);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_360);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_362);
			   if (!jrd_362.jrd_363) break;
				gtt_preserve = (/*REL.RDB$RELATION_TYPE*/
						jrd_362.jrd_365 == rel_global_temp_preserve);
				relation = MET_lookup_relation_id(tdbb, /*REL.RDB$RELATION_ID*/
									jrd_362.jrd_364, false);
			/*END_FOR;*/
			   }
			}

			CMP_release(tdbb, request);
		}
		else if (work->dfw_id > 0)
		{
			relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
			gtt_preserve = (relation) && (relation->rel_flags & REL_temp_conn);
		}

		if (gtt_preserve && relation)
		{
			tdbb->tdbb_flags &= ~TDBB_use_db_page_space;
			try {
				if (relation->getPages(tdbb, -1, false)) {
					more2 = (*task_routine) (tdbb, phase, work, transaction);
				}
				tdbb->tdbb_flags |= TDBB_use_db_page_space;
			}
			catch (...)
			{
				tdbb->tdbb_flags |= TDBB_use_db_page_space;
				throw;
			}
		}
	}

	if (!is_create) {
		more = (*task_routine)(tdbb, phase, work, transaction);
	}

	return (more || more2);
}


static bool create_index(thread_db*	tdbb,
						SSHORT	phase,
						DeferredWork*		work,
						jrd_tra*		transaction)
{
   struct {
          SSHORT jrd_305;	// gds__utility 
   } jrd_304;
   struct {
          TEXT  jrd_302 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_303 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_301;
   struct {
          SSHORT jrd_317;	// gds__utility 
   } jrd_316;
   struct {
          SSHORT jrd_314;	// gds__null_flag 
          SSHORT jrd_315;	// RDB$INDEX_ID 
   } jrd_313;
   struct {
          SSHORT jrd_310;	// gds__utility 
          SSHORT jrd_311;	// gds__null_flag 
          SSHORT jrd_312;	// RDB$INDEX_ID 
   } jrd_309;
   struct {
          TEXT  jrd_308 [32];	// RDB$INDEX_NAME 
   } jrd_307;
   struct {
          TEXT  jrd_322 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_323 [32];	// RDB$FOREIGN_KEY 
          double  jrd_324;	// RDB$STATISTICS 
          SSHORT jrd_325;	// gds__utility 
          SSHORT jrd_326;	// gds__null_flag 
          SSHORT jrd_327;	// RDB$COLLATION_ID 
          SSHORT jrd_328;	// gds__null_flag 
          SSHORT jrd_329;	// RDB$COLLATION_ID 
          SSHORT jrd_330;	// gds__null_flag 
          SSHORT jrd_331;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_332;	// RDB$FIELD_ID 
          SSHORT jrd_333;	// gds__null_flag 
          SSHORT jrd_334;	// RDB$DIMENSIONS 
          SSHORT jrd_335;	// RDB$FIELD_TYPE 
          SSHORT jrd_336;	// RDB$FIELD_POSITION 
          SSHORT jrd_337;	// gds__null_flag 
          SSHORT jrd_338;	// RDB$INDEX_TYPE 
          SSHORT jrd_339;	// RDB$UNIQUE_FLAG 
          SSHORT jrd_340;	// RDB$SEGMENT_COUNT 
          SSHORT jrd_341;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_342;	// RDB$INDEX_ID 
          SSHORT jrd_343;	// RDB$RELATION_ID 
   } jrd_321;
   struct {
          TEXT  jrd_320 [32];	// RDB$INDEX_NAME 
   } jrd_319;
   struct {
          SSHORT jrd_358;	// gds__utility 
   } jrd_357;
   struct {
          SSHORT jrd_355;	// gds__null_flag 
          SSHORT jrd_356;	// RDB$INDEX_ID 
   } jrd_354;
   struct {
          TEXT  jrd_348 [32];	// RDB$RELATION_NAME 
          bid  jrd_349;	// RDB$VIEW_BLR 
          SSHORT jrd_350;	// gds__utility 
          SSHORT jrd_351;	// gds__null_flag 
          SSHORT jrd_352;	// RDB$INDEX_ID 
          SSHORT jrd_353;	// gds__null_flag 
   } jrd_347;
   struct {
          TEXT  jrd_346 [32];	// RDB$INDEX_NAME 
   } jrd_345;
/**************************************
 *
 *	c r e a t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Create a new index or change the state of an index between active/inactive.
 *
 **************************************/
	jrd_req* request;
	jrd_rel* relation;
	jrd_rel* partner_relation;
	index_desc idx;
	int key_count;
	jrd_req* handle;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		handle = NULL;

		/* Drop those indices at clean up time. */
		/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction) IDXN IN RDB$INDICES CROSS
			IREL IN RDB$RELATIONS OVER RDB$RELATION_NAME
				WITH IDXN.RDB$INDEX_NAME EQ work->dfw_name.c_str()*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_344, sizeof(jrd_344), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_345.jrd_346, 32);
		EXE_start (tdbb, handle, transaction);
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_345);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 48, (UCHAR*) &jrd_347);
		   if (!jrd_347.jrd_350) break;
				/* Views do not have indices */
			if (/*IREL.RDB$VIEW_BLR.NULL*/
			    jrd_347.jrd_353)
			{
				relation = MET_lookup_relation(tdbb, /*IDXN.RDB$RELATION_NAME*/
								     jrd_347.jrd_348);

				RelationPages* relPages = relation->getPages(tdbb, -1, false);
				if (!relPages) {
					return false;
				}

				// we need to special handle temp tables with ON PRESERVE ROWS only
				const bool isTempIndex = (relation->rel_flags & REL_temp_conn) &&
					(relPages->rel_instance_id != 0);

				/* Fetch the root index page and mark MUST_WRITE, and then
				   delete the index. It will also clean the index slot. Note
				   that the previous fixed definition of MAX_IDX (64) has been
				   dropped in favor of a computed value saved in the Database */
				if (relPages->rel_index_root)
				{
					if (work->dfw_id != dbb->dbb_max_idx)
					{
						WIN window(relPages->rel_pg_space_id, relPages->rel_index_root);
						CCH_FETCH(tdbb, &window, LCK_write, pag_root);
						CCH_MARK_MUST_WRITE(tdbb, &window);
						const bool tree_exists = BTR_delete_index(tdbb, &window, work->dfw_id);

						if (!isTempIndex) {
							work->dfw_id = dbb->dbb_max_idx;
						}
						else if (tree_exists)
						{
							IndexLock* idx_lock = CMP_get_index_lock(tdbb, relation, work->dfw_id);
							if (idx_lock)
							{
								if (!--idx_lock->idl_count) {
									LCK_release(tdbb, idx_lock->idl_lock);
								}
							}
						}
					}
					if (!/*IDXN.RDB$INDEX_ID.NULL*/
					     jrd_347.jrd_351)
					{
						/*MODIFY IDXN USING*/
						{
						
							/*IDXN.RDB$INDEX_ID.NULL*/
							jrd_347.jrd_351 = TRUE;
						/*END_MODIFY;*/
						jrd_354.jrd_355 = jrd_347.jrd_351;
						jrd_354.jrd_356 = jrd_347.jrd_352;
						EXE_send (tdbb, handle, 2, 4, (UCHAR*) &jrd_354);
						}
					}
				}
			}
		/*END_FOR;*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_357);
		   }
		}

		CMP_release(tdbb, handle);
		return false;

	case 1:
	case 2:
		return true;

	case 3:
		key_count = 0;
		relation = NULL;
		idx.idx_flags = 0;

		// Here we need dirty reads from database (first of all from
		// RDB$RELATION_FIELDS and RDB$FIELDS - tables not directly related
		// with index to be created and it's dfw_name. Missing it breaks gbak,
		// and appears can break other applications. Modification of
		// record in RDB$INDICES was moved into separate request. AP-2008.

		// Fetch the information necessary to create the index.  On the first
		// time thru, check to see if the index already exists.  If so, delete
		// it.  If the index inactive flag is set, don't create the index

		request = CMP_find_request(tdbb, irq_c_index, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			IDX IN RDB$INDICES CROSS
				SEG IN RDB$INDEX_SEGMENTS OVER RDB$INDEX_NAME CROSS
				RFR IN RDB$RELATION_FIELDS OVER RDB$FIELD_NAME,
				RDB$RELATION_NAME CROSS
				FLD IN RDB$FIELDS CROSS
				REL IN RDB$RELATIONS OVER RDB$RELATION_NAME WITH
					FLD.RDB$FIELD_NAME EQ RFR.RDB$FIELD_SOURCE AND
					IDX.RDB$INDEX_NAME EQ work->dfw_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_318, sizeof(jrd_318), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_319.jrd_320, 32);
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_319);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 110, (UCHAR*) &jrd_321);
		   if (!jrd_321.jrd_325) break;

			if (!REQUEST(irq_c_index))
				REQUEST(irq_c_index) = request;

			if (!relation)
			{
				relation = MET_lookup_relation_id(tdbb, /*REL.RDB$RELATION_ID*/
									jrd_321.jrd_343, false);
				if (!relation)
				{
					EXE_unwind(tdbb, request);
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_idx_create_err) << Arg::Str(work->dfw_name));
					/* Msg308: can't create index %s */
				}
				if (/*IDX.RDB$INDEX_ID*/
				    jrd_321.jrd_342 && /*IDX.RDB$STATISTICS*/
    jrd_321.jrd_324 < 0.0)
				{
					// we need to know if this relation is temporary or not
					MET_scan_relation(tdbb, relation);

					// no need to recalculate statistics for base instance of GTT
					RelationPages* relPages = relation->getPages(tdbb, -1, false);
					const bool isTempInstance = relation->isTemporary() &&
						relPages && (relPages->rel_instance_id != 0);

					if (isTempInstance || !relation->isTemporary())
					{
						SelectivityList selectivity(*tdbb->getDefaultPool());
						const USHORT id = /*IDX.RDB$INDEX_ID*/
								  jrd_321.jrd_342 - 1;
						IDX_statistics(tdbb, relation, id, selectivity);
						DFW_update_index(work->dfw_name.c_str(), id, selectivity, transaction);

						EXE_unwind(tdbb, request);
					}
					return false;
				}

				if (/*IDX.RDB$INDEX_ID*/
				    jrd_321.jrd_342)
				{
					IDX_delete_index(tdbb, relation, (USHORT)(/*IDX.RDB$INDEX_ID*/
										  jrd_321.jrd_342 - 1));

					jrd_req* request2 = CMP_find_request(tdbb, irq_c_index_m, IRQ_REQUESTS);

			        /*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
						IDXM IN RDB$INDICES WITH IDXM.RDB$INDEX_NAME EQ work->dfw_name.c_str()*/
				{
				if (!request2)
				   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_306, sizeof(jrd_306), true);
				gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_307.jrd_308, 32);
				EXE_start (tdbb, request2, transaction);
				EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_307);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 6, (UCHAR*) &jrd_309);
				   if (!jrd_309.jrd_310) break;

						if (!REQUEST(irq_c_index_m))
							REQUEST(irq_c_index_m) = request2;

						/*MODIFY IDXM*/
						{
						
							/*IDXM.RDB$INDEX_ID.NULL*/
							jrd_309.jrd_311 = TRUE;
						/*END_MODIFY;*/
						jrd_313.jrd_314 = jrd_309.jrd_311;
						jrd_313.jrd_315 = jrd_309.jrd_312;
						EXE_send (tdbb, request2, 2, 4, (UCHAR*) &jrd_313);
						}
					/*END_FOR;*/
					   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_316);
					   }
					}

					if (!REQUEST(irq_c_index_m))
						REQUEST(irq_c_index_m) = request2;
				}
				if (/*IDX.RDB$INDEX_INACTIVE*/
				    jrd_321.jrd_341)
				{
					EXE_unwind(tdbb, request);
					return false;
				}
				idx.idx_count = /*IDX.RDB$SEGMENT_COUNT*/
						jrd_321.jrd_340;
				if (!idx.idx_count || idx.idx_count > MAX_INDEX_SEGMENTS)
				{
					EXE_unwind(tdbb, request);
					if (!idx.idx_count)
					{
						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_idx_seg_err) << Arg::Str(work->dfw_name));
						/* Msg304: segment count of 0 defined for index %s */
					}
					else
					{
						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_idx_key_err) << Arg::Str(work->dfw_name));
						/* Msg311: too many keys defined for index %s */
					}
				}
				if (/*IDX.RDB$UNIQUE_FLAG*/
				    jrd_321.jrd_339)
					idx.idx_flags |= idx_unique;
				if (/*IDX.RDB$INDEX_TYPE*/
				    jrd_321.jrd_338 == 1)
					idx.idx_flags |= idx_descending;
				if (!/*IDX.RDB$FOREIGN_KEY.NULL*/
				     jrd_321.jrd_337)
					idx.idx_flags |= idx_foreign;

				jrd_req* rc_request = NULL;

				/*FOR(REQUEST_HANDLE rc_request TRANSACTION_HANDLE transaction)
					RC IN RDB$RELATION_CONSTRAINTS WITH
					RC.RDB$INDEX_NAME EQ work->dfw_name.c_str() AND
					RC.RDB$CONSTRAINT_TYPE = PRIMARY_KEY*/
				{
				if (!rc_request)
				   rc_request = CMP_compile2 (tdbb, (UCHAR*) jrd_300, sizeof(jrd_300), true);
				gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_301.jrd_302, 32);
				gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_301.jrd_303, 12);
				EXE_start (tdbb, rc_request, transaction);
				EXE_send (tdbb, rc_request, 0, 44, (UCHAR*) &jrd_301);
				while (1)
				   {
				   EXE_receive (tdbb, rc_request, 1, 2, (UCHAR*) &jrd_304);
				   if (!jrd_304.jrd_305) break;

					idx.idx_flags |= idx_primary;

				/*END_FOR;*/
				   }
				}

				CMP_release(tdbb, rc_request);
			}

			if (++key_count > idx.idx_count || /*SEG.RDB$FIELD_POSITION*/
							   jrd_321.jrd_336 > idx.idx_count ||
				/*FLD.RDB$FIELD_TYPE*/
				jrd_321.jrd_335 == blr_blob || !/*FLD.RDB$DIMENSIONS.NULL*/
		 jrd_321.jrd_333)
			{
				EXE_unwind(tdbb, request);

				if (key_count > idx.idx_count)
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_idx_key_err) << Arg::Str(work->dfw_name));
					/* Msg311: too many keys defined for index %s */
				}
				else if (/*SEG.RDB$FIELD_POSITION*/
					 jrd_321.jrd_336 > idx.idx_count)
				{
					fb_utils::exact_name(/*RFR.RDB$FIELD_NAME*/
							     jrd_321.jrd_322);
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_inval_key_posn) <<
							 /* Msg358: invalid key position */
							 Arg::Gds(isc_field_name) << Arg::Str(/*RFR.RDB$FIELD_NAME*/
											      jrd_321.jrd_322) <<
							 Arg::Gds(isc_index_name) << Arg::Str(work->dfw_name));
				}
				else if (/*FLD.RDB$FIELD_TYPE*/
					 jrd_321.jrd_335 == blr_blob)
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_blob_idx_err) << Arg::Str(work->dfw_name));
					/* Msg350: attempt to index blob column in index %s */
				}
				else
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_array_idx_err) << Arg::Str(work->dfw_name));
					/* Msg351: attempt to index array column in index %s */
				}
			}

			idx.idx_rpt[/*SEG.RDB$FIELD_POSITION*/
				    jrd_321.jrd_336].idx_field = /*RFR.RDB$FIELD_ID*/
	      jrd_321.jrd_332;

			if (/*FLD.RDB$CHARACTER_SET_ID.NULL*/
			    jrd_321.jrd_330)
				/*FLD.RDB$CHARACTER_SET_ID*/
				jrd_321.jrd_331 = CS_NONE;

			SSHORT collate;
			if (!/*RFR.RDB$COLLATION_ID.NULL*/
			     jrd_321.jrd_328)
				collate = /*RFR.RDB$COLLATION_ID*/
					  jrd_321.jrd_329;
			else if (!/*FLD.RDB$COLLATION_ID.NULL*/
				  jrd_321.jrd_326)
				collate = /*FLD.RDB$COLLATION_ID*/
					  jrd_321.jrd_327;
			else
				collate = COLLATE_NONE;

			const SSHORT text_type = INTL_CS_COLL_TO_TTYPE(/*FLD.RDB$CHARACTER_SET_ID*/
								       jrd_321.jrd_331, collate);
			idx.idx_rpt[/*SEG.RDB$FIELD_POSITION*/
				    jrd_321.jrd_336].idx_itype =
				DFW_assign_index_type(tdbb, work->dfw_name, gds_cvt_blr_dtype[/*FLD.RDB$FIELD_TYPE*/
											      jrd_321.jrd_335], text_type);

			// Initialize selectivity to zero. Otherwise random rubbish makes its way into database
			idx.idx_rpt[/*SEG.RDB$FIELD_POSITION*/
				    jrd_321.jrd_336].idx_selectivity = 0;
		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_c_index))
			REQUEST(irq_c_index) = request;

		if (key_count != idx.idx_count)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_key_field_err) << Arg::Str(work->dfw_name));
			/* Msg352: too few key columns found for index %s (incorrect column name?) */
		}
		if (!relation)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_idx_create_err) << Arg::Str(work->dfw_name));
			/* Msg308: can't create index %s */
		}

		/* Make sure the relation info is all current */

		MET_scan_relation(tdbb, relation);

		if (relation->rel_view_rse)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_idx_create_err) << Arg::Str(work->dfw_name));
			/* Msg308: can't create index %s */
		}

		/* Actually create the index */

		Lock *relationLock = NULL, *partnerLock = NULL;
		bool releaseRelationLock = false, releasePartnerLock = false;
		partner_relation = NULL;
		try
		{
			// Protect relation from modification to create consistent index
			relationLock = protect_relation(tdbb, transaction, relation, releaseRelationLock);

			if (idx.idx_flags & idx_foreign)
			{
				idx.idx_id = idx_invalid;

				if (MET_lookup_partner(tdbb, relation, &idx, work->dfw_name.c_str()))
				{
					partner_relation = MET_lookup_relation_id(tdbb, idx.idx_primary_relation, true);
				}

				if (!partner_relation)
				{
					Firebird::MetaName constraint_name;
					MET_lookup_cnstrt_for_index(tdbb, constraint_name, work->dfw_name);
					ERR_post(Arg::Gds(isc_partner_idx_not_found) << Arg::Str(constraint_name));
				}

				// Get an protected_read lock on the both relations if the index being
				// defined enforces a foreign key constraint. This will prevent
				// the constraint from being violated during index construction.

				partnerLock = protect_relation(tdbb, transaction, partner_relation, releasePartnerLock);

				int bad_segment;
				if (!IDX_check_master_types(tdbb, idx, partner_relation, bad_segment))
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_partner_idx_incompat_type) << Arg::Num(bad_segment + 1));
				}

/* hvlad: this code was never called but i preserve it for Claudio review and decision

				// CVC: Currently, the server doesn't enforce FK creation more than at DYN level.
				// If DYN is bypassed, then FK creation succeeds and operation will fail at run-time.
				// The aim is to check REFERENCES at DDL time instead of DML time and behave accordingly
				// to ANSI SQL rules for REFERENCES rights.
				// For testing purposes, I'm calling SCL_check_index, although most of the DFW ops are
				// carried using internal metadata structures that are refreshed from system tables.

				// Don't bother if the master's owner is the same than the detail's owner.
				// If both tables aren't defined in the same session, partner_relation->rel_owner_name
				// won't be loaded hence, we need to be careful about null pointers.

				if (relation->rel_owner_name.length() == 0 ||
					partner_relation->rel_owner_name.length() == 0 ||
					relation->rel_owner_name != partner_relation->rel_owner_name)
				{
					SCL_check_index(tdbb, partner_relation->rel_name,
									idx.idx_id + 1, SCL_sql_references);
				}
*/
			}

			fb_assert(work->dfw_id <= dbb->dbb_max_idx);
			idx.idx_id = work->dfw_id;
			SelectivityList selectivity(*tdbb->getDefaultPool());
			IDX_create_index(tdbb, relation, &idx, work->dfw_name.c_str(),
							&work->dfw_id, transaction, selectivity);
			fb_assert(work->dfw_id == idx.idx_id);
			DFW_update_index(work->dfw_name.c_str(), idx.idx_id, selectivity, transaction);

			if (partner_relation)
			{
				// signal to other processes about new constraint
				relation->rel_flags |= REL_check_partners;
				LCK_lock(tdbb, relation->rel_partners_lock, LCK_EX, LCK_WAIT);
				LCK_release(tdbb, relation->rel_partners_lock);

				if (relation != partner_relation) {
					partner_relation->rel_flags |= REL_check_partners;
					LCK_lock(tdbb, partner_relation->rel_partners_lock, LCK_EX, LCK_WAIT);
					LCK_release(tdbb, partner_relation->rel_partners_lock);
				}
			}
			if (relationLock && releaseRelationLock) {
				release_protect_lock(tdbb, transaction, relationLock);
			}
			if (partnerLock && releasePartnerLock) {
				release_protect_lock(tdbb, transaction, partnerLock);
			}
		}
		catch (const Firebird::Exception&)
		{
			if (relationLock && releaseRelationLock) {
				release_protect_lock(tdbb, transaction, relationLock);
			}
			if (partnerLock && releasePartnerLock) {
				release_protect_lock(tdbb, transaction, partnerLock);
			}
			throw;
		}

		break;
	}

	return false;
}

static bool create_procedure(thread_db*	tdbb,
							SSHORT	phase,
							DeferredWork*		work,
							jrd_tra*		transaction)
{
/**************************************
 *
 *	c r e a t e _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Create a new procedure.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			const bool compile = !work->findArg(dfw_arg_check_blr);
			get_procedure_dependencies(work, compile, transaction);

			jrd_prc* procedure = MET_lookup_procedure(tdbb, work->dfw_name, compile);
			if (!procedure) {
				return false;
			}
			// Never used.
			procedure->prc_flags |= PRC_create;
		}
		break;
	}

	return false;
}


static bool create_relation(thread_db*	tdbb,
							SSHORT	phase,
							DeferredWork*		work,
							jrd_tra*		transaction)
{
   struct {
          SSHORT jrd_275;	// gds__utility 
          SSHORT jrd_276;	// RDB$RELATION_ID 
   } jrd_274;
   struct {
          TEXT  jrd_273 [32];	// RDB$RELATION_NAME 
   } jrd_272;
   struct {
          SSHORT jrd_281;	// gds__utility 
          SSHORT jrd_282;	// RDB$DBKEY_LENGTH 
   } jrd_280;
   struct {
          TEXT  jrd_279 [32];	// RDB$VIEW_NAME 
   } jrd_278;
   struct {
          SSHORT jrd_299;	// RDB$RELATION_ID 
   } jrd_298;
   struct {
          SSHORT jrd_297;	// gds__utility 
   } jrd_296;
   struct {
          SSHORT jrd_294;	// RDB$RELATION_ID 
          SSHORT jrd_295;	// RDB$DBKEY_LENGTH 
   } jrd_293;
   struct {
          TEXT  jrd_287 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_288;	// RDB$VIEW_BLR 
          SSHORT jrd_289;	// gds__utility 
          SSHORT jrd_290;	// RDB$DBKEY_LENGTH 
          SSHORT jrd_291;	// RDB$RELATION_ID 
          SSHORT jrd_292;	// RDB$RELATION_ID 
   } jrd_286;
   struct {
          TEXT  jrd_285 [32];	// RDB$RELATION_NAME 
   } jrd_284;
/**************************************
 *
 *	c r e a t e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Create a new relation.
 *
 **************************************/
	jrd_req* request;
	jrd_rel* relation;
	USHORT rel_id, external_flag;
	bid blob_id;
	jrd_req* handle;
	Lock* lock;

	blob_id.clear();

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	USHORT local_min_relation_id = dbb->dbb_max_sys_rel + 1;
	switch (phase)
	{
	case 0:
		if (work->dfw_lock)
		{
			LCK_release(tdbb, work->dfw_lock);
			delete work->dfw_lock;
			work->dfw_lock = NULL;
		}
		break;

	case 1:
	case 2:
		return true;

	case 3:
		/* Take a relation lock on rel id -1 before actually
		   generating a relation id. */

		work->dfw_lock = lock = FB_NEW_RPT(*tdbb->getDefaultPool(), sizeof(SLONG)) Lock;
		lock->lck_dbb = dbb;
		lock->lck_length = sizeof(SLONG);
		lock->lck_key.lck_long = -1;
		lock->lck_type = LCK_relation;
		lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
		lock->lck_parent = dbb->dbb_lock;

		LCK_lock(tdbb, lock, LCK_EX, LCK_WAIT);

		/* Assign a relation ID and dbkey length to the new relation.
		   Probe the candidate relation ID returned from the system
		   relation RDB$DATABASE to make sure it isn't already assigned.
		   This can happen from nefarious manipulation of RDB$DATABASE
		   or wraparound of the next relation ID. Keep looking for a
		   usable relation ID until the search space is exhausted. */

		rel_id = 0;
		request = CMP_find_request(tdbb, irq_c_relation, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			X IN RDB$DATABASE CROSS Y IN RDB$RELATIONS WITH
				Y.RDB$RELATION_NAME EQ work->dfw_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_283, sizeof(jrd_283), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_284.jrd_285, 32);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_284);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 272, (UCHAR*) &jrd_286);
		   if (!jrd_286.jrd_289) break;
			if (!REQUEST(irq_c_relation))
				REQUEST(irq_c_relation) = request;

			blob_id = /*Y.RDB$VIEW_BLR*/
				  jrd_286.jrd_288;
			external_flag = /*Y.RDB$EXTERNAL_FILE*/
					jrd_286.jrd_287[0];

			/*MODIFY X USING*/
			{
			
				rel_id = /*X.RDB$RELATION_ID*/
					 jrd_286.jrd_292;

				if (rel_id < local_min_relation_id || rel_id > MAX_RELATION_ID)
				{
					rel_id = /*X.RDB$RELATION_ID*/
						 jrd_286.jrd_292 = local_min_relation_id;
				}

				while ( (relation = MET_lookup_relation_id(tdbb, rel_id++, false)) )
				{
					if (rel_id < local_min_relation_id ||
						rel_id > MAX_RELATION_ID)
					{
						rel_id = local_min_relation_id;
					}
					if (rel_id == /*X.RDB$RELATION_ID*/
						      jrd_286.jrd_292)
					{
						EXE_unwind(tdbb, request);
						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_table_name) << Arg::Str(work->dfw_name) <<
								 Arg::Gds(isc_imp_exc));
					}
				}
				/*X.RDB$RELATION_ID*/
				jrd_286.jrd_292 = (rel_id > MAX_RELATION_ID) ? local_min_relation_id : rel_id;
				/*MODIFY Y USING*/
				{
				
					/*Y.RDB$RELATION_ID*/
					jrd_286.jrd_291 = --rel_id;
					if (blob_id.isEmpty())
						/*Y.RDB$DBKEY_LENGTH*/
						jrd_286.jrd_290 = 8;
					else
					{
						/* update the dbkey length to include each of the base
						   relations */

						handle = NULL;
						/*Y.RDB$DBKEY_LENGTH*/
						jrd_286.jrd_290 = 0;
						/*FOR(REQUEST_HANDLE handle)
							Z IN RDB$VIEW_RELATIONS CROSS
								R IN RDB$RELATIONS OVER RDB$RELATION_NAME
								WITH Z.RDB$VIEW_NAME = work->dfw_name.c_str()*/
						{
						if (!handle)
						   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_277, sizeof(jrd_277), true);
						gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_278.jrd_279, 32);
						EXE_start (tdbb, handle, dbb->dbb_sys_trans);
						EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_278);
						while (1)
						   {
						   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_280);
						   if (!jrd_280.jrd_281) break;

							/*Y.RDB$DBKEY_LENGTH*/
							jrd_286.jrd_290 += /*R.RDB$DBKEY_LENGTH*/
    jrd_280.jrd_282;

						/*END_FOR;*/
						   }
						}
						CMP_release(tdbb, handle);
					}
				/*END_MODIFY*/
				jrd_293.jrd_294 = jrd_286.jrd_291;
				jrd_293.jrd_295 = jrd_286.jrd_290;
				EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_293);
				}
			/*END_MODIFY*/
			jrd_298.jrd_299 = jrd_286.jrd_292;
			EXE_send (tdbb, request, 4, 2, (UCHAR*) &jrd_298);
			}
		/*END_FOR;*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_296);
		   }
		}

		LCK_release(tdbb, lock);
		delete lock;
		work->dfw_lock = NULL;

		if (!REQUEST(irq_c_relation))
			REQUEST(irq_c_relation) = request;

		/* if this is not a view, create the relation */

		if (rel_id && blob_id.isEmpty() && !external_flag)
		{
			relation = MET_relation(tdbb, rel_id);
			DPM_create_relation(tdbb, relation);
		}

		return true;

	case 4:

		/* get the relation and flag it to check for dependencies
		   in the view blr (if it exists) and any computed fields */

		request = CMP_find_request(tdbb, irq_c_relation2, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			X IN RDB$RELATIONS WITH
				X.RDB$RELATION_NAME EQ work->dfw_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_271, sizeof(jrd_271), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_272.jrd_273, 32);
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_272);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_274);
		   if (!jrd_274.jrd_275) break;

			if (!REQUEST(irq_c_relation2))
				REQUEST(irq_c_relation2) = request;

			rel_id = /*X.RDB$RELATION_ID*/
				 jrd_274.jrd_276;
			relation = MET_relation(tdbb, rel_id);
			relation->rel_flags |= REL_get_dependencies;

			relation->rel_flags &= ~REL_scanned;
			DFW_post_work(transaction, dfw_scan_relation, NULL, rel_id);

		/*END_FOR;*/
		   }
		}

		if (!REQUEST(irq_c_relation2))
			REQUEST(irq_c_relation2) = request;

		break;
	}

	return false;
}


static bool create_trigger(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
/**************************************
 *
 *	c r e a t e _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Perform required actions on creation of trigger.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			const bool compile = !work->findArg(dfw_arg_check_blr);
			get_trigger_dependencies(work, compile, transaction);
			return true;
		}

	case 4:
		{
			if (!work->findArg(dfw_arg_rel_name))
			{
				const DeferredWork* const arg = work->findArg(dfw_arg_trg_type);
				fb_assert(arg);

				if (arg && (arg->dfw_id & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB)
				{
					MET_load_trigger(tdbb, NULL, work->dfw_name,
						&tdbb->getDatabase()->dbb_triggers[arg->dfw_id & ~TRIGGER_TYPE_DB]);
				}
			}
		}
		break;
	}

	return false;
}


static bool create_collation(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_246;	// gds__utility 
          SSHORT jrd_247;	// RDB$COLLATION_ID 
   } jrd_245;
   struct {
          SSHORT jrd_244;	// RDB$CHARACTER_SET_ID 
   } jrd_243;
   struct {
          SSHORT jrd_269;	// gds__null_flag 
          SSHORT jrd_270;	// RDB$COLLATION_ID 
   } jrd_268;
   struct {
          SSHORT jrd_267;	// gds__utility 
   } jrd_266;
   struct {
          bid  jrd_264;	// RDB$SPECIFIC_ATTRIBUTES 
          SSHORT jrd_265;	// gds__null_flag 
   } jrd_263;
   struct {
          TEXT  jrd_253 [32];	// RDB$CHARACTER_SET_NAME 
          TEXT  jrd_254 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_255 [32];	// RDB$BASE_COLLATION_NAME 
          bid  jrd_256;	// RDB$SPECIFIC_ATTRIBUTES 
          SSHORT jrd_257;	// gds__utility 
          SSHORT jrd_258;	// gds__null_flag 
          SSHORT jrd_259;	// gds__null_flag 
          SSHORT jrd_260;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_261;	// gds__null_flag 
          SSHORT jrd_262;	// RDB$COLLATION_ID 
   } jrd_252;
   struct {
          TEXT  jrd_250 [32];	// RDB$COLLATION_NAME 
          SSHORT jrd_251;	// RDB$CHARACTER_SET_ID 
   } jrd_249;
/**************************************
 *
 *	c r e a t e _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Get collation id or setup specific attributes.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		return false;

	case 1:
		{
			const USHORT charSetId = TTYPE_TO_CHARSET(work->dfw_id);
			jrd_req* handle = NULL;

			/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
				COLL IN RDB$COLLATIONS
				CROSS CS IN RDB$CHARACTER_SETS
					OVER RDB$CHARACTER_SET_ID
				WITH COLL.RDB$COLLATION_NAME EQ work->dfw_name.c_str() AND
					 COLL.RDB$CHARACTER_SET_ID EQ charSetId*/
			{
			if (!handle)
			   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_248, sizeof(jrd_248), true);
			gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_249.jrd_250, 32);
			jrd_249.jrd_251 = charSetId;
			EXE_start (tdbb, handle, transaction);
			EXE_send (tdbb, handle, 0, 34, (UCHAR*) &jrd_249);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 116, (UCHAR*) &jrd_252);
			   if (!jrd_252.jrd_257) break;

				if (/*COLL.RDB$COLLATION_ID.NULL*/
				    jrd_252.jrd_261)
				{
					// ASF: User collations are created with the last number available,
					// to minimize the possibility of conflicts with future system collations.
					// The greater available number is 126 to avoid signed/unsigned problems.

					SSHORT id = 126;

					jrd_req* request = CMP_find_request(tdbb, irq_l_colls, IRQ_REQUESTS);

					/*FOR(REQUEST_HANDLE request)
						Y IN RDB$COLLATIONS
						WITH Y.RDB$CHARACTER_SET_ID = COLL.RDB$CHARACTER_SET_ID
						SORTED BY DESCENDING Y.RDB$COLLATION_ID*/
					{
					if (!request)
					   request = CMP_compile2 (tdbb, (UCHAR*) jrd_242, sizeof(jrd_242), true);
					jrd_243.jrd_244 = jrd_252.jrd_260;
					EXE_start (tdbb, request, dbb->dbb_sys_trans);
					EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_243);
					while (1)
					   {
					   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_245);
					   if (!jrd_245.jrd_246) break;

						if (!REQUEST(irq_l_colls))
							REQUEST(irq_l_colls) = request;

						if (!/*COLL.RDB$COLLATION_ID.NULL*/
						     jrd_252.jrd_261)
						{
							EXE_unwind(tdbb, request);
							break;
						}

						if (/*Y.RDB$COLLATION_ID*/
						    jrd_245.jrd_247 + 1 <= id)
							/*COLL.RDB$COLLATION_ID.NULL*/
							jrd_252.jrd_261 = FALSE;
						else
							id = /*Y.RDB$COLLATION_ID*/
							     jrd_245.jrd_247 - 1;

					/*END_FOR*/
					   }
					}

					if (!REQUEST(irq_l_colls))
						REQUEST(irq_l_colls) = request;

					if (/*COLL.RDB$COLLATION_ID.NULL*/
					    jrd_252.jrd_261)
					{
						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_max_coll_per_charset));
					}
					else
					{
						/*MODIFY COLL USING*/
						{
						
							/*COLL.RDB$COLLATION_ID*/
							jrd_252.jrd_262 = id;
						/*END_MODIFY*/
						jrd_268.jrd_269 = jrd_252.jrd_261;
						jrd_268.jrd_270 = jrd_252.jrd_262;
						EXE_send (tdbb, handle, 4, 4, (UCHAR*) &jrd_268);
						}
					}
				}
				else
				{
					SLONG length = 0;
					Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;

					if (!/*COLL.RDB$SPECIFIC_ATTRIBUTES.NULL*/
					     jrd_252.jrd_259)
					{
						blb* blob = BLB_open(tdbb, transaction, &/*COLL.RDB$SPECIFIC_ATTRIBUTES*/
											 jrd_252.jrd_256);
						length = blob->blb_length + 10;
						length = BLB_get_data(tdbb, blob, buffer.getBuffer(length), length);
					}

					const Firebird::string specificAttributes((const char*) buffer.begin(), length);
					Firebird::string newSpecificAttributes;

					// ASF: If setupCollationAttributes fail we store the original
					// attributes. This should be what we want for new databases
					// and restores. CREATE COLLATION will fail in DYN.
					if (IntlManager::setupCollationAttributes(
							fb_utils::exact_name(/*COLL.RDB$BASE_COLLATION_NAME.NULL*/
									     jrd_252.jrd_258 ?
								/*COLL.RDB$COLLATION_NAME*/
								jrd_252.jrd_254 : /*COLL.RDB$BASE_COLLATION_NAME*/
   jrd_252.jrd_255),
							fb_utils::exact_name(/*CS.RDB$CHARACTER_SET_NAME*/
									     jrd_252.jrd_253),
							specificAttributes, newSpecificAttributes) &&
						newSpecificAttributes != specificAttributes) // if nothing changed, we do nothing
					{
						/*MODIFY COLL USING*/
						{
						
							if (newSpecificAttributes.isEmpty())
								/*COLL.RDB$SPECIFIC_ATTRIBUTES.NULL*/
								jrd_252.jrd_259 = TRUE;
							else
							{
								/*COLL.RDB$SPECIFIC_ATTRIBUTES.NULL*/
								jrd_252.jrd_259 = FALSE;

								blb* blob = BLB_create(tdbb, transaction, &/*COLL.RDB$SPECIFIC_ATTRIBUTES*/
													   jrd_252.jrd_256);
								BLB_put_segment(tdbb, blob,
									(const UCHAR*) newSpecificAttributes.begin(),
									newSpecificAttributes.length());
								BLB_close(tdbb, blob);
							}
						/*END_MODIFY;*/
						jrd_263.jrd_264 = jrd_252.jrd_256;
						jrd_263.jrd_265 = jrd_252.jrd_259;
						EXE_send (tdbb, handle, 2, 10, (UCHAR*) &jrd_263);
						}
					}
				}

			/*END_FOR;*/
			   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_266);
			   }
			}
			CMP_release(tdbb, handle);
		}

		return true;
	}

	return false;
}


static bool delete_collation(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
/**************************************
 *
 *	d e l e t e _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	a collation, and if so, clean up after it.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 0:
		return false;

	case 1:
		check_dependencies(tdbb, work->dfw_name.c_str(), NULL, obj_collation, transaction);
		return true;

	case 2:
		return true;

	case 3:
		INTL_texttype_unload(tdbb, work->dfw_id);
		return true;
	}

	return false;
}


static bool delete_exception(thread_db*	tdbb,
							SSHORT	phase,
							DeferredWork*		work,
							jrd_tra*		transaction)
{
/**************************************
 *
 *	d e l e t e _ e x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	an exception, and if so, clean up after it.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 0:
		return false;

	case 1:
		check_dependencies(tdbb, work->dfw_name.c_str(), NULL, obj_exception, transaction);
		return true;

	case 2:
		return true;

	case 3:
		return true;

	case 4:
		break;
	}

	return false;
}


static bool delete_generator(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
/**************************************
 *
 *	d e l e t e _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	a generator, and if so, clean up after it.
 * CVC: This function was modelled after delete_exception.
 *
 **************************************/

	SET_TDBB(tdbb);
	const char* gen_name = work->dfw_name.c_str();

	switch (phase)
	{
		case 0:
			return false;
		case 1:
			check_system_generator(gen_name, dfw_delete_generator);
			check_dependencies(tdbb, gen_name, 0, obj_generator, transaction);
			return true;
		case 2:
			return true;
		case 3:
			return true;
		case 4:
			break;
	}

	return false;
}


static bool modify_generator(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
/**************************************
 *
 *	m o d i f y _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to modify
 *	a generator's information in rdb$generators.
 * CVC: For now, the function always forbids this operation.
 * This has nothing to do with gen_id or set generator.
 *
 **************************************/

	SET_TDBB(tdbb);
	const char* gen_name = work->dfw_name.c_str();

	switch (phase)
	{
		case 0:
			return false;
		case 1:
			check_system_generator(gen_name, dfw_modify_generator);
			if (work->dfw_id) // != 0 means not only the desc was changed.
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_generator_name) << Arg::Str(gen_name) <<
						 Arg::Gds(isc_random) << Arg::Str("Only can modify description for user generators."));

			return true;
		case 2:
			return true;
		case 3:
			return true;
		case 4:
			break;
	}

	return false;

}


static bool delete_udf(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
/**************************************
 *
 *	d e l e t e _ u d f
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	an udf, and if so, clean up after it.
 * CVC: This function was modelled after delete_exception.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
		case 0:
			return false;
		case 1:
			check_dependencies(tdbb, work->dfw_name.c_str(), 0, obj_udf, transaction);
			return true;
		case 2:
			return true;
		case 3:
			return true;
		case 4:
			break;
	}

	return false;
}


static bool create_field(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          bid  jrd_239;	// RDB$VALIDATION_BLR 
          SSHORT jrd_240;	// gds__utility 
          SSHORT jrd_241;	// gds__null_flag 
   } jrd_238;
   struct {
          TEXT  jrd_237 [32];	// RDB$FIELD_NAME 
   } jrd_236;
/**************************************
 *
 *	c r e a t e _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Store dependencies of a field.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		return false;

	case 1:
		{
			const Firebird::MetaName depName(work->dfw_name);
			jrd_req* handle = NULL;
			bid validation;
			validation.clear();

			/*FOR(REQUEST_HANDLE handle)
				FLD IN RDB$FIELDS WITH
					FLD.RDB$FIELD_NAME EQ depName.c_str()*/
			{
			if (!handle)
			   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_235, sizeof(jrd_235), true);
			gds__vtov ((const char*) depName.c_str(), (char*) jrd_236.jrd_237, 32);
			EXE_start (tdbb, handle, dbb->dbb_sys_trans);
			EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_236);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 12, (UCHAR*) &jrd_238);
			   if (!jrd_238.jrd_240) break;

				if (!/*FLD.RDB$VALIDATION_BLR.NULL*/
				     jrd_238.jrd_241)
					validation = /*FLD.RDB$VALIDATION_BLR*/
						     jrd_238.jrd_239;
			/*END_FOR;*/
			   }
			}

			CMP_release(tdbb, handle);

			if (!validation.isEmpty())
			{
				MemoryPool* new_pool = dbb->createPool();
				Jrd::ContextPoolHolder context(tdbb, new_pool);

				MET_get_dependencies(tdbb, NULL, NULL, 0, NULL, &validation,
					NULL, depName, obj_validation, 0, transaction, depName);

				dbb->deletePool(new_pool);
			}
		}
		// fall through

	case 2:
	case 3:
		return true;

	case 4:	// after scan_relation (phase 3)
		check_computed_dependencies(tdbb, transaction, work->dfw_name);
		break;
	}

	return false;
}


static bool delete_field(thread_db*	tdbb,
						SSHORT	phase,
						DeferredWork*		work,
						jrd_tra*		transaction)
{
   struct {
          TEXT  jrd_232 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_233;	// gds__utility 
          SSHORT jrd_234;	// RDB$RELATION_ID 
   } jrd_231;
   struct {
          TEXT  jrd_230 [32];	// RDB$FIELD_SOURCE 
   } jrd_229;
/**************************************
 *
 *	d e l e t e _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	This whole routine exists just to
 *	return an error if someone attempts to
 *	delete a global field that is in use
 *
 **************************************/

	int field_count;
	jrd_req* handle;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
		/* Look up the field in RFR.  If we can't find the field,
		   go ahead with the delete. */

		handle = NULL;
		field_count = 0;

		/*FOR(REQUEST_HANDLE handle)
			RFR IN RDB$RELATION_FIELDS CROSS
				REL IN RDB$RELATIONS
				OVER RDB$RELATION_NAME
				WITH RFR.RDB$FIELD_SOURCE EQ work->dfw_name.c_str()*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_228, sizeof(jrd_228), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_229.jrd_230, 32);
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_229);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 36, (UCHAR*) &jrd_231);
		   if (!jrd_231.jrd_233) break;
				/* If the rfr field is also being deleted, there's no dependency */
			if (!find_depend_in_dfw(tdbb, /*RFR.RDB$FIELD_NAME*/
						      jrd_231.jrd_232, obj_computed, /*REL.RDB$RELATION_ID*/
		jrd_231.jrd_234,
									transaction))
			{
				field_count++;
			}
		/*END_FOR;*/
		   }
		}
		CMP_release(tdbb, handle);

		if (field_count)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_no_delete) <<								/* Msg353: can not delete */
					 Arg::Gds(isc_domain_name) << Arg::Str(work->dfw_name) <<
					 Arg::Gds(isc_dependency) << Arg::Num(field_count)); 	/* Msg310: there are %ld dependencies */
		}

		check_dependencies(tdbb, work->dfw_name.c_str(), NULL, obj_field, transaction);

	case 2:
		return true;

	case 3:
		MET_delete_dependencies(tdbb, work->dfw_name, obj_computed, transaction);
		MET_delete_dependencies(tdbb, work->dfw_name, obj_validation, transaction);
		break;
	}

	return false;
}


static bool modify_field(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          bid  jrd_225;	// RDB$VALIDATION_BLR 
          SSHORT jrd_226;	// gds__utility 
          SSHORT jrd_227;	// gds__null_flag 
   } jrd_224;
   struct {
          TEXT  jrd_223 [32];	// RDB$FIELD_NAME 
   } jrd_222;
/**************************************
 *
 *	m o d i f y _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Handle constraint dependencies of a field.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		return false;

	case 1:
		{
			const Firebird::MetaName depName(work->dfw_name);
			jrd_req* handle = NULL;
			bid validation;
			validation.clear();

			/*FOR(REQUEST_HANDLE handle)
				FLD IN RDB$FIELDS WITH
					FLD.RDB$FIELD_NAME EQ depName.c_str()*/
			{
			if (!handle)
			   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_221, sizeof(jrd_221), true);
			gds__vtov ((const char*) depName.c_str(), (char*) jrd_222.jrd_223, 32);
			EXE_start (tdbb, handle, dbb->dbb_sys_trans);
			EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_222);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 12, (UCHAR*) &jrd_224);
			   if (!jrd_224.jrd_226) break;

				if (!/*FLD.RDB$VALIDATION_BLR.NULL*/
				     jrd_224.jrd_227)
					validation = /*FLD.RDB$VALIDATION_BLR*/
						     jrd_224.jrd_225;
			/*END_FOR;*/
			   }
			}

			CMP_release(tdbb, handle);

			const DeferredWork* const arg = work->findArg(dfw_arg_new_name);

			// ASF: If there are procedures depending on the domain, it can't be renamed.
			if (arg && depName != arg->dfw_name.c_str())
				check_dependencies(tdbb, depName.c_str(), NULL, obj_field, transaction);

			MET_delete_dependencies(tdbb, depName, obj_validation, transaction);

			if (!validation.isEmpty())
			{
				MemoryPool* new_pool = dbb->createPool();
				Jrd::ContextPoolHolder context(tdbb, new_pool);

				MET_get_dependencies(tdbb, NULL, NULL, 0, NULL, &validation,
					NULL, depName, obj_validation, 0, transaction, depName);

				dbb->deletePool(new_pool);
			}
		}
		// fall through

	case 2:
	case 3:
		return true;

	case 4:	// after scan_relation (phase 3)
		check_computed_dependencies(tdbb, transaction, work->dfw_name);
		break;
	}

	return false;
}


static bool delete_global(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_220;	// gds__utility 
   } jrd_219;
   struct {
          TEXT  jrd_218 [32];	// RDB$FIELD_NAME 
   } jrd_217;
/**************************************
 *
 *	d e l e t e _ g l o b a l
 *
 **************************************
 *
 * Functional description
 *	If a local field has been deleted,
 *	check to see if its global field
 *	is computed. If so, delete all its
 *	dependencies under the assumption
 *	that a global computed field has only
 *	one local field.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
		jrd_req* handle = NULL;
		/*FOR(REQUEST_HANDLE handle)
			FLD IN RDB$FIELDS WITH
				FLD.RDB$FIELD_NAME EQ work->dfw_name.c_str() AND
				FLD.RDB$COMPUTED_BLR NOT MISSING*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_216, sizeof(jrd_216), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_217.jrd_218, 32);
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_217);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_219);
		   if (!jrd_219.jrd_220) break;

				MET_delete_dependencies(tdbb, work->dfw_name, obj_computed, transaction);

		/*END_FOR;*/
		   }
		}
		CMP_release(tdbb, handle);
		break;
		}
	}

	return false;
}


static void check_partners(thread_db* tdbb, const USHORT rel_id)
{
/**************************************
 *
 *	c h e c k _ p a r t n e r s
 *
 **************************************
 *
 * Functional description
 *	Signal other processes to check partners of relation rel_id
 * Used when FK index was dropped
 *
 **************************************/
	const Database* dbb = tdbb->getDatabase();
	vec<jrd_rel*>* relations = dbb->dbb_relations;

	fb_assert(relations);
	fb_assert(rel_id < relations->count());

	jrd_rel *relation = (*relations)[rel_id];
	fb_assert(relation);

	LCK_lock(tdbb, relation->rel_partners_lock, LCK_EX, LCK_WAIT);
	LCK_release(tdbb, relation->rel_partners_lock);
	relation->rel_flags |= REL_check_partners;
}


static bool delete_index(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
/**************************************
 *
 *	d e l e t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	IndexLock* index = NULL;

	SET_TDBB(tdbb);

	const DeferredWork* arg = work->findArg(dfw_arg_index_name);

	fb_assert(arg);
	fb_assert(arg->dfw_id > 0);
	const USHORT id = arg->dfw_id - 1;
	// Maybe a permanent check?
	//if (id == idx_invalid)
	//	ERR_post(...);

	// Look up the relation.  If we can't find the relation,
	// don't worry about the index.

	jrd_rel* relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
	if (!relation) {
		return false;
	}

	RelationPages* relPages = relation->getPages(tdbb, -1, false);
	if (!relPages) {
		return false;
	}
	// we need to special handle temp tables with ON PRESERVE ROWS only
	const bool isTempIndex = (relation->rel_flags & REL_temp_conn) &&
		(relPages->rel_instance_id != 0);

	switch (phase)
	{
	case 0:
		index = CMP_get_index_lock(tdbb, relation, id);
		if (index) {
			if (!index->idl_count) {
				LCK_release(tdbb, index->idl_lock);
			}
		}
		return false;

	case 1:
		check_dependencies(tdbb, arg->dfw_name.c_str(), NULL, obj_index, transaction);
		return true;

	case 2:
		return true;

	case 3:
		// Make sure nobody is currently using the index

		// If we about to delete temp index instance then usage counter
		// will remains 1 and will be decremented by IDX_delete_index at
		// phase 4

		index = CMP_get_index_lock(tdbb, relation, id);
		if (index)
		{
			// take into account lock probably used by temp index instance
			bool temp_lock_released = false;
			if (isTempIndex && (index->idl_count == 1))
			{
				index_desc idx;
				if (BTR_lookup(tdbb, relation, id, &idx, relPages) == FB_SUCCESS)
				{
					index->idl_count--;
					LCK_release(tdbb, index->idl_lock);
					temp_lock_released = true;
				}
			}

			// Try to clear trigger cache to release lock
			if (index->idl_count)
				MET_clear_cache(tdbb);

			if (!isTempIndex)
			{
				if (index->idl_count ||
					!LCK_lock(tdbb, index->idl_lock, LCK_EX, transaction->getLockWait()))
				{
					// restore lock used by temp index instance
					if (temp_lock_released)
					{
						LCK_lock(tdbb, index->idl_lock, LCK_SR, LCK_WAIT);
						index->idl_count++;
					}

					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_obj_in_use) << Arg::Str("INDEX"));
				}
				index->idl_count++;
			}
		}

		return true;

	case 4:
		index = CMP_get_index_lock(tdbb, relation, id);
		if (isTempIndex && index)
			index->idl_count++;
		IDX_delete_index(tdbb, relation, id);

		if (isTempIndex)
			return false;

		if (work->dfw_type == dfw_delete_expression_index)
		{
			MET_delete_dependencies(tdbb, arg->dfw_name, obj_expression_index, transaction);
		}

		// if index was bound to deleted FK constraint
		// then work->dfw_args was set in VIO_erase
		arg = work->findArg(dfw_arg_partner_rel_id);

		if (arg) {
			if (arg->dfw_id) {
				check_partners(tdbb, relation->rel_id);
				if (relation->rel_id != arg->dfw_id) {
					check_partners(tdbb, arg->dfw_id);
				}
			}
			else {
				// partner relation was not found in VIO_erase
				// we must check partners of all relations in database
				MET_update_partners(tdbb);
			}
		}

		if (index)
		{
			/* in order for us to have gotten the lock in phase 3
			* idl_count HAD to be 0, therefore after having incremented
			* it for the exclusive lock it would have to be 1.
			* IF now it is NOT 1 then someone else got a lock to
			* the index and something is seriously wrong */
			fb_assert(index->idl_count == 1);
			if (!--index->idl_count)
			{
				/* Release index existence lock and memory. */

				for (IndexLock** ptr = &relation->rel_index_locks; *ptr; ptr = &(*ptr)->idl_next)
				{
					if (*ptr == index)
					{
						*ptr = index->idl_next;
						break;
					}
				}
				if (index->idl_lock)
				{
					LCK_release(tdbb, index->idl_lock);
					delete index->idl_lock;
				}
				delete index;

				/* Release index refresh lock and memory. */

				for (IndexBlock** iptr = &relation->rel_index_blocks; *iptr; iptr = &(*iptr)->idb_next)
				{
					if ((*iptr)->idb_id == id)
					{
						IndexBlock* index_block = *iptr;
						*iptr = index_block->idb_next;

						/* Lock was released in IDX_delete_index(). */

						delete index_block->idb_lock;
						delete index_block;
						break;
					}
				}
			}
		}
		break;
	}

	return false;
}


static bool delete_parameter(thread_db*	tdbb,
							SSHORT	phase,
							DeferredWork*,
							jrd_tra*)
{
/**************************************
 *
 *	d e l e t e _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *	Return an error if someone attempts to
 *	delete a field from a procedure and it is
 *	used by a view or procedure.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
/* hvlad: temporary disable procedure parameters dependency check
		  until proper solution (something like dyn_mod_parameter)
		  will be implemented. This check never worked properly
		  so no harm is done

		if (MET_lookup_procedure_id(tdbb, work->dfw_id, false, true, 0))
		{
			const DeferredWork* arg = work->dfw_args;
			fb_assert(arg && (arg->dfw_type == dfw_arg_proc_name));

			check_dependencies(tdbb, arg->dfw_name.c_str(), work->dfw_name.c_str(),
							   obj_procedure, transaction);
		}
*/
	case 2:
		return true;

	case 3:
		break;
	}

	return false;
}


static bool delete_procedure(thread_db*	tdbb,
							SSHORT	phase,
							DeferredWork*		work,
							jrd_tra*		transaction)
{
/**************************************
 *
 *	d e l e t e _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	a procedure , and if so, clean up after it.
 *
 **************************************/
	jrd_prc* procedure;
	USHORT old_flags;

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 0:
		procedure = MET_lookup_procedure_id(tdbb, work->dfw_id, false, true, 0);
		if (!procedure) {
			return false;
		}

		if (procedure->prc_existence_lock)
		{
			LCK_convert(tdbb, procedure->prc_existence_lock, LCK_SR, transaction->getLockWait());
		}
		return false;

	case 1:
		check_dependencies(tdbb, work->dfw_name.c_str(), NULL, obj_procedure, transaction);
		return true;

	case 2:
		procedure = MET_lookup_procedure_id(tdbb, work->dfw_id, false, true, 0);
		if (!procedure) {
			return false;
		}

		if (procedure->prc_existence_lock)
		{
			if (!LCK_convert(tdbb, procedure->prc_existence_lock, LCK_EX, transaction->getLockWait()))
			{
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_obj_in_use) << Arg::Str(work->dfw_name));
			}
		}

		/* If we are in a multi-client server, someone else may have marked
		   procedure obsolete. Unmark and we will remark it later. */

		procedure->prc_flags &= ~PRC_obsolete;
		return true;

	case 3:
		return true;

	case 4:
		procedure = MET_lookup_procedure_id(tdbb, work->dfw_id, true, true, 0);
		if (!procedure) {
			return false;
		}

		// Do not allow to drop procedure used by user requests
		if (procedure->prc_use_count && MET_procedure_in_use(tdbb, procedure))
		{
			/*
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_obj_in_use) << Arg::Str(work->dfw_name));
			*/
			gds__log("Deleting procedure %s which is currently in use by active user requests",
					 work->dfw_name.c_str());
			MET_delete_dependencies(tdbb, work->dfw_name, obj_procedure, transaction);

			if (procedure->prc_existence_lock) {
				LCK_release(tdbb, procedure->prc_existence_lock);
			}
			(*tdbb->getDatabase()->dbb_procedures)[procedure->prc_id] = NULL;
			return false;
		}

		old_flags = procedure->prc_flags;
		procedure->prc_flags |= PRC_obsolete;
		if (procedure->prc_request)
		{
			if (CMP_clone_is_active(procedure->prc_request))
			{
				procedure->prc_flags = old_flags;
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_obj_in_use) << Arg::Str(work->dfw_name));
			}

			MET_release_procedure_request(tdbb, procedure);
		}

		/* delete dependency lists */

		MET_delete_dependencies(tdbb, work->dfw_name, obj_procedure, transaction);

		if (procedure->prc_existence_lock) {
			LCK_release(tdbb, procedure->prc_existence_lock);
		}
		break;
	}

	return false;
}


static bool delete_relation(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
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
          SSHORT jrd_203;	// RDB$RELATION_ID 
   } jrd_202;
   struct {
          TEXT  jrd_214 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_215;	// gds__utility 
   } jrd_213;
   struct {
          TEXT  jrd_212 [32];	// RDB$RELATION_NAME 
   } jrd_211;
/**************************************
 *
 *	d e l e t e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	a relation, and if so, clean up after it.
 *
 **************************************/
	jrd_req* request;
	jrd_rel* relation;
	Resource* rsc;
	USHORT view_count;
	bool adjusted;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
		if (!relation) {
			return false;
		}

		if (relation->rel_existence_lock)
		{
			LCK_convert(tdbb, relation->rel_existence_lock, LCK_SR, transaction->getLockWait());
		}

		if (relation->rel_flags & REL_deleting)
		{
			relation->rel_flags &= ~REL_deleting;
			relation->rel_drop_mutex.leave();
		}

		return false;

	case 1:
		/* check if any views use this as a base relation */

		request = NULL;
		view_count = 0;
		/*FOR(REQUEST_HANDLE request)
			X IN RDB$VIEW_RELATIONS WITH
			X.RDB$RELATION_NAME EQ work->dfw_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_210, sizeof(jrd_210), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_211.jrd_212, 32);
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_211);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_213);
		   if (!jrd_213.jrd_215) break;
				/* If the view is also being deleted, there's no dependency */
			if (!find_depend_in_dfw(tdbb, /*X.RDB$VIEW_NAME*/
						      jrd_213.jrd_214, obj_view, 0, transaction))
			{
				view_count++;
			}

		/*END_FOR;*/
		   }
		}
		CMP_release(tdbb, request);

		if (view_count)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_no_delete) << /* Msg353: can not delete */
					 Arg::Gds(isc_table_name) << Arg::Str(work->dfw_name) <<
					 Arg::Gds(isc_dependency) << Arg::Num(view_count));
					 /* Msg310: there are %ld dependencies */
		}

		relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
		if (!relation) {
			return false;
		}

		check_dependencies(tdbb, work->dfw_name.c_str(), NULL, 
			relation->isView() ? obj_view : obj_relation, transaction);

		return true;

	case 2:
		relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
		if (!relation) {
			return false;
		}

		/* Let relation be deleted if only this transaction is using it */

		adjusted = false;
		if (relation->rel_use_count == 1)
		{
			for (rsc = transaction->tra_resources.begin(); rsc < transaction->tra_resources.end();
				rsc++)
			{
				if (rsc->rsc_rel == relation)
				{
					--relation->rel_use_count;
					adjusted = true;
					break;
				}
			}
		}

		if (relation->rel_use_count)
			MET_clear_cache(tdbb);

		if (relation->rel_use_count || (relation->rel_existence_lock &&
			!LCK_convert(tdbb, relation->rel_existence_lock, LCK_EX, transaction->getLockWait())))
		{
			if (adjusted) {
				++relation->rel_use_count;
			}
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_obj_in_use) << Arg::Str(work->dfw_name));
		}

		fb_assert(!relation->rel_use_count);

		// Flag relation delete in progress so that active sweep or
		// garbage collector threads working on relation can skip over it

		relation->rel_flags |= REL_deleting;

		{ // scope
			Database::Checkout dcoHolder(dbb);
			relation->rel_drop_mutex.enter();
		}

		return true;

	case 3:
		return true;

	case 4:
		relation = MET_lookup_relation_id(tdbb, work->dfw_id, true);
		if (!relation) {
			return false;
		}

		/* The sweep and garbage collector threads have no more than
		   a single record latency in responding to the flagged relation
		   deletion. Nevertheless, as a defensive programming measure,
		   don't wait forever if something has gone awry and the sweep
		   count doesn't run down. */

		for (int wait = 0; wait < 60; wait++)
		{
			if (!relation->rel_sweep_count) {
				break;
			}

			Database::Checkout dcoHolder(dbb);
			THREAD_SLEEP(1 * 1000);
		}

		if (relation->rel_sweep_count)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_obj_in_use) << Arg::Str(work->dfw_name));
		}

#ifdef GARBAGE_THREAD
		// Free any memory associated with the relation's garbage collection bitmap

		delete relation->rel_gc_bitmap;
		relation->rel_gc_bitmap = NULL;

	    delete relation->rel_garbage;
	    relation->rel_garbage = NULL;
#endif
		if (relation->rel_file) {
		    EXT_fini (relation, false);
		}

		RelationPages* relPages = relation->getBasePages();
		if (relPages->rel_index_root) {
			IDX_delete_indices(tdbb, relation, relPages);
		}

		if (relPages->rel_pages) {
			DPM_delete_relation(tdbb, relation);
		}

		// if this is a view (or even if we don't know), delete dependency lists

		if (relation->rel_view_rse || !(relation->rel_flags & REL_scanned)) {
			MET_delete_dependencies(tdbb, work->dfw_name, obj_view, transaction);
		}

		// Now that the data, pointer, and index pages are gone,
		// get rid of the relation itself

		request = NULL;

		/*FOR(REQUEST_HANDLE request) X IN RDB$FORMATS WITH
			X.RDB$RELATION_ID EQ relation->rel_id*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_201, sizeof(jrd_201), true);
		jrd_202.jrd_203 = relation->rel_id;
		EXE_start (tdbb, request, dbb->dbb_sys_trans);
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_202);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_204);
		   if (!jrd_204.jrd_205) break;
			/*ERASE X;*/
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_206);
		/*END_FOR;*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_208);
		   }
		}

		CMP_release(tdbb, request);

		// Release relation locks
		if (relation->rel_existence_lock) {
			LCK_release(tdbb, relation->rel_existence_lock);
		}
		if (relation->rel_partners_lock) {
			LCK_release(tdbb, relation->rel_partners_lock);
		}

		// Mark relation in the cache as dropped
		relation->rel_flags |= REL_deleted;

		if (relation->rel_flags & REL_deleting)
		{
			relation->rel_flags &= ~REL_deleting;
			relation->rel_drop_mutex.leave();
		}

		// Release relation triggers
		MET_release_triggers(tdbb, &relation->rel_pre_store);
		MET_release_triggers(tdbb, &relation->rel_post_store);
		MET_release_triggers(tdbb, &relation->rel_pre_erase);
		MET_release_triggers(tdbb, &relation->rel_post_erase);
		MET_release_triggers(tdbb, &relation->rel_pre_modify);
		MET_release_triggers(tdbb, &relation->rel_post_modify);

		break;
	}

	return false;
}


static bool delete_rfr(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_187;	// gds__utility 
   } jrd_186;
   struct {
          SSHORT jrd_185;	// RDB$RELATION_ID 
   } jrd_184;
   struct {
          SSHORT jrd_192;	// gds__utility 
   } jrd_191;
   struct {
          SSHORT jrd_190;	// RDB$RELATION_ID 
   } jrd_189;
   struct {
          TEXT  jrd_198 [32];	// RDB$BASE_FIELD 
          TEXT  jrd_199 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_200;	// gds__utility 
   } jrd_197;
   struct {
          TEXT  jrd_195 [32];	// RDB$BASE_FIELD 
          SSHORT jrd_196;	// RDB$RELATION_ID 
   } jrd_194;
/**************************************
 *
 *	d e l e t e _ r f r
 *
 **************************************
 *
 * Functional description
 *	This whole routine exists just to
 *	return an error if someone attempts to
 *	1. delete a field from a relation if the relation
 *		is used in a view and the field is referenced in
 *		the view.
 *	2. drop the last column of a table
 *
 **************************************/
	int rel_exists, field_count;
	jrd_req* handle;
	Firebird::MetaName f;
	jrd_rel* relation;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
		/* first check if there are any fields used explicitly by the view */

		handle = NULL;
		field_count = 0;
		/*FOR(REQUEST_HANDLE handle)
			REL IN RDB$RELATIONS CROSS
				VR IN RDB$VIEW_RELATIONS OVER RDB$RELATION_NAME CROSS
				VFLD IN RDB$RELATION_FIELDS WITH
				REL.RDB$RELATION_ID EQ work->dfw_id AND
				VFLD.RDB$VIEW_CONTEXT EQ VR.RDB$VIEW_CONTEXT AND
				VFLD.RDB$RELATION_NAME EQ VR.RDB$VIEW_NAME AND
				VFLD.RDB$BASE_FIELD EQ work->dfw_name.c_str()*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_193, sizeof(jrd_193), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_194.jrd_195, 32);
		jrd_194.jrd_196 = work->dfw_id;
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle, 0, 34, (UCHAR*) &jrd_194);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 66, (UCHAR*) &jrd_197);
		   if (!jrd_197.jrd_200) break;
				/* If the view is also being deleted, there's no dependency */
			if (!find_depend_in_dfw(tdbb, /*VR.RDB$VIEW_NAME*/
						      jrd_197.jrd_199, obj_view, 0, transaction))
			{
				f = /*VFLD.RDB$BASE_FIELD*/
				    jrd_197.jrd_198;
				field_count++;
			}
		/*END_FOR;*/
		   }
		}
		CMP_release(tdbb, handle);

		if (field_count)
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_no_delete) <<	/* Msg353: can not delete */
					 Arg::Gds(isc_field_name) << Arg::Str(f) <<
					 Arg::Gds(isc_dependency) << Arg::Num(field_count));
					 /* Msg310: there are %ld dependencies */

		/* now check if there are any dependencies generated through the blr
		   that defines the relation */

		if ( (relation = MET_lookup_relation_id(tdbb, work->dfw_id, false)) )
		{
			check_dependencies(tdbb, relation->rel_name.c_str(), work->dfw_name.c_str(),
							   relation->isView() ? obj_view : obj_relation,
							   transaction);
		}

		/* see if the relation itself is being dropped */

		handle = NULL;
		rel_exists = 0;
		/*FOR(REQUEST_HANDLE handle)
			REL IN RDB$RELATIONS WITH REL.RDB$RELATION_ID EQ work->dfw_id*/
		{
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_188, sizeof(jrd_188), true);
		jrd_189.jrd_190 = work->dfw_id;
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_189);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_191);
		   if (!jrd_191.jrd_192) break;
			rel_exists++;
		/*END_FOR;*/
		   }
		}
		if (handle)
			CMP_release(tdbb, handle);

		/* if table exists, check if this is the last column in the table */

		if (rel_exists)
		{
			field_count = 0;
			handle = NULL;

			/*FOR(REQUEST_HANDLE handle)
				REL IN RDB$RELATIONS CROSS
					RFLD IN RDB$RELATION_FIELDS OVER RDB$RELATION_NAME
					WITH REL.RDB$RELATION_ID EQ work->dfw_id*/
			{
			if (!handle)
			   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_183, sizeof(jrd_183), true);
			jrd_184.jrd_185 = work->dfw_id;
			EXE_start (tdbb, handle, dbb->dbb_sys_trans);
			EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_184);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_186);
			   if (!jrd_186.jrd_187) break;
				field_count++;
			/*END_FOR;*/
			   }
			}
			if (handle)
			{
				CMP_release(tdbb, handle);
			}

			if (!field_count)
			{
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_del_last_field));
				/* Msg354: last column in a relation cannot be deleted */
			}
		}

	case 2:
		return true;

	case 3:
		/* Unlink field from data structures.  Don't try to actually release field and
		   friends -- somebody may be pointing to them */

		relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
		if (relation)
		{
			const int id = MET_lookup_field(tdbb, relation, work->dfw_name);
			if (id >= 0)
			{
				vec<jrd_fld*>* vector = relation->rel_fields;
				if (vector && (ULONG) id < vector->count() && (*vector)[id])
				{
					(*vector)[id] = NULL;
				}
			}
		}
		break;
	}

	return false;
}


static bool delete_shadow(thread_db*	tdbb,
						  SSHORT	phase,
						  DeferredWork*		work,
						  jrd_tra*)
{
/**************************************
 *
 *	d e l e t e _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	Provide deferred work interface to
 *	MET_delete_shadow.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		MET_delete_shadow(tdbb, work->dfw_id);
		break;
	}

	return false;
}


static bool delete_trigger(thread_db*	tdbb,
						   SSHORT	phase,
						   DeferredWork*		work,
						   jrd_tra*		transaction)
{
/**************************************
 *
 *	d e l e t e _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Cleanup after a deleted trigger.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		/* get rid of dependencies */
		MET_delete_dependencies(tdbb, work->dfw_name, obj_trigger, transaction);
		return true;

	case 4:
		{
			const DeferredWork* arg = work->findArg(dfw_arg_rel_name);
			if (!arg)
			{
				const DeferredWork* arg = work->findArg(dfw_arg_trg_type);
				fb_assert(arg);

				if (arg && (arg->dfw_id & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB)
				{
					MET_release_trigger(tdbb,
						&tdbb->getDatabase()->dbb_triggers[arg->dfw_id & ~TRIGGER_TYPE_DB],
						work->dfw_name);
				}
			}
		}
		break;
	}

	return false;
}


static bool find_depend_in_dfw(thread_db*	tdbb,
							   TEXT*	object_name,
							   USHORT	dep_type,
							   USHORT	rel_id,
							   jrd_tra*		transaction)
{
   struct {
          bid  jrd_173;	// RDB$VALIDATION_BLR 
          SSHORT jrd_174;	// gds__utility 
          SSHORT jrd_175;	// gds__null_flag 
   } jrd_172;
   struct {
          TEXT  jrd_171 [32];	// RDB$FIELD_NAME 
   } jrd_170;
   struct {
          TEXT  jrd_180 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_181;	// gds__utility 
          SSHORT jrd_182;	// RDB$RELATION_ID 
   } jrd_179;
   struct {
          TEXT  jrd_178 [32];	// RDB$FIELD_NAME 
   } jrd_177;
/**************************************
 *
 *	f i n d _ d e p e n d _ i n _ d f w
 *
 **************************************
 *
 * Functional description
 *	Check the object to see if it is being
 *	deleted as part of the deferred work.
 *	Return true if it is, false otherwise.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	fb_utils::exact_name(object_name);
	enum dfw_t dfw_type;
	switch (dep_type)
	{
	case obj_view:
		dfw_type = dfw_delete_relation;
		break;
	case obj_trigger:
		dfw_type = dfw_delete_trigger;
		break;
	case obj_computed:
		dfw_type = rel_id ? dfw_delete_rfr : dfw_delete_global;
		break;
	case obj_validation:
		dfw_type = dfw_delete_global;
		break;
	case obj_procedure:
		dfw_type = dfw_delete_procedure;
		break;
	case obj_expression_index:
		dfw_type = dfw_delete_expression_index;
		break;
	default:
		fb_assert(false);
		break;
	}

	// Look to see if an object of the desired type is being deleted or modified.
	// For an object being modified we verify dependencies separately when we parse its BLR.
	for (const DeferredWork* work = transaction->tra_deferred_job->work; work; work = work->getNext())
	{
		if ((work->dfw_type == dfw_type ||
			(work->dfw_type == dfw_modify_procedure && dfw_type == dfw_delete_procedure) ||
			(work->dfw_type == dfw_modify_field && dfw_type == dfw_delete_global) ||
			(work->dfw_type == dfw_modify_trigger && dfw_type == dfw_delete_trigger)) &&
			work->dfw_name == object_name && (!rel_id || rel_id == work->dfw_id))
		{
			if (work->dfw_type == dfw_modify_procedure)
			{
				// Don't consider that procedure is in DFW if we are only checking the BLR
				if (!work->findArg(dfw_arg_check_blr))
					return true;
			}
			else
				return true;
		}

		if (work->dfw_type == dfw_type && dfw_type == dfw_delete_expression_index)
		{
			for (size_t i = 0; i < work->dfw_args.getCount(); ++i)
			{
				const DeferredWork* arg = work->dfw_args[i];
				if (arg->dfw_type == dfw_arg_index_name &&
					arg->dfw_name == object_name)
				{
					return true;
				}
			}
		}
	}

	if (dfw_type == dfw_delete_global)
	{
		if (dep_type == obj_computed)
		{
			// Computed fields are more complicated.  If the global field isn't being
			// deleted, see if all of the fields it is the source for, are.

			jrd_req* request = CMP_find_request(tdbb, irq_ch_cmp_dpd, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE request)
				FLD IN RDB$FIELDS CROSS
					RFR IN RDB$RELATION_FIELDS CROSS
					REL IN RDB$RELATIONS
					WITH FLD.RDB$FIELD_NAME EQ RFR.RDB$FIELD_SOURCE
					AND FLD.RDB$FIELD_NAME EQ object_name
					AND REL.RDB$RELATION_NAME EQ RFR.RDB$RELATION_NAME*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_176, sizeof(jrd_176), true);
			gds__vtov ((const char*) object_name, (char*) jrd_177.jrd_178, 32);
			EXE_start (tdbb, request, dbb->dbb_sys_trans);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_177);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_179);
			   if (!jrd_179.jrd_181) break;
				if (!REQUEST(irq_ch_cmp_dpd))
					REQUEST(irq_ch_cmp_dpd) = request;

				if (!find_depend_in_dfw(tdbb, /*RFR.RDB$FIELD_NAME*/
							      jrd_179.jrd_180, obj_computed,
										/*REL.RDB$RELATION_ID*/
										jrd_179.jrd_182, transaction))
				{
					EXE_unwind(tdbb, request);
					return false;
				}
			/*END_FOR;*/
			   }
			}

			if (!REQUEST(irq_ch_cmp_dpd)) {
				REQUEST(irq_ch_cmp_dpd) = request;
			}

			return true;
		}

		if (dep_type == obj_validation)
		{
			// Maybe it's worth caching in the future?
			jrd_req* request = NULL;

			/*FOR(REQUEST_HANDLE request)
				FLD IN RDB$FIELDS WITH
					FLD.RDB$FIELD_NAME EQ object_name*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_169, sizeof(jrd_169), true);
			gds__vtov ((const char*) object_name, (char*) jrd_170.jrd_171, 32);
			EXE_start (tdbb, request, dbb->dbb_sys_trans);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_170);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_172);
			   if (!jrd_172.jrd_174) break;

				if (!/*FLD.RDB$VALIDATION_BLR.NULL*/
				     jrd_172.jrd_175)
				{
					EXE_unwind(tdbb, request);
					return false;
				}
			/*END_FOR;*/
			   }
			}

			return true;
		}
	}

	return false;
}


static void get_array_desc(thread_db* tdbb, const TEXT* field_name, Ods::InternalArrayDesc* desc)
{
   struct {
          SLONG  jrd_165;	// RDB$UPPER_BOUND 
          SLONG  jrd_166;	// RDB$LOWER_BOUND 
          SSHORT jrd_167;	// gds__utility 
          SSHORT jrd_168;	// RDB$DIMENSION 
   } jrd_164;
   struct {
          TEXT  jrd_163 [32];	// RDB$FIELD_NAME 
   } jrd_162;
/**************************************
 *
 *	g e t _ a r r a y _ d e s c
 *
 **************************************
 *
 * Functional description
 *	Get array descriptor for an array.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, irq_r_fld_dim, IRQ_REQUESTS);

	Ods::InternalArrayDesc::iad_repeat* ranges = 0;
	/*FOR (REQUEST_HANDLE request)
		D IN RDB$FIELD_DIMENSIONS WITH D.RDB$FIELD_NAME EQ field_name*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_161, sizeof(jrd_161), true);
	gds__vtov ((const char*) field_name, (char*) jrd_162.jrd_163, 32);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_162);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_164);
	   if (!jrd_164.jrd_167) break;
		if (!REQUEST(irq_r_fld_dim))
		{
			REQUEST(irq_r_fld_dim) = request;
		}
		if (/*D.RDB$DIMENSION*/
		    jrd_164.jrd_168 >= 0 && /*D.RDB$DIMENSION*/
	 jrd_164.jrd_168 < desc->iad_dimensions)
		{
			ranges = desc->iad_rpt + /*D.RDB$DIMENSION*/
						 jrd_164.jrd_168;
			ranges->iad_lower = /*D.RDB$LOWER_BOUND*/
					    jrd_164.jrd_166;
			ranges->iad_upper = /*D.RDB$UPPER_BOUND*/
					    jrd_164.jrd_165;
		}
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_r_fld_dim))
	{
		REQUEST(irq_r_fld_dim) = request;
	}

	desc->iad_count = 1;

	for (ranges = desc->iad_rpt + desc->iad_dimensions; --ranges >= desc->iad_rpt;)
	{
		ranges->iad_length = desc->iad_count;
		desc->iad_count *= ranges->iad_upper - ranges->iad_lower + 1;
	}

	desc->iad_version = Ods::IAD_VERSION_1;
	desc->iad_length = IAD_LEN(MAX(desc->iad_struct_count, desc->iad_dimensions));
	desc->iad_element_length = desc->iad_rpt[0].iad_desc.dsc_length;
	desc->iad_total_length = desc->iad_element_length * desc->iad_count;
}


static void get_procedure_dependencies(DeferredWork* work, bool compile, jrd_tra* transaction)
{
   struct {
          bid  jrd_159;	// RDB$PROCEDURE_BLR 
          SSHORT jrd_160;	// gds__utility 
   } jrd_158;
   struct {
          TEXT  jrd_157 [32];	// RDB$PROCEDURE_NAME 
   } jrd_156;
/**************************************
 *
 *	g e t _ p r o c e d u r e _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *	Get relations and fields on which this
 *	procedure depends, either when it's being
 *	created or when it's modified.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	if (compile)
		compile = !(tdbb->getAttachment()->att_flags & ATT_gbak_attachment);

	jrd_prc* procedure = NULL;
	bid blob_id;
	blob_id.clear();

	jrd_req* handle = CMP_find_request(tdbb, irq_c_prc_dpd, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle)
		X IN RDB$PROCEDURES WITH
			X.RDB$PROCEDURE_NAME EQ work->dfw_name.c_str()*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_155, sizeof(jrd_155), true);
	gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_156.jrd_157, 32);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_156);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_158);
	   if (!jrd_158.jrd_160) break;
		if (!REQUEST(irq_c_prc_dpd))
			REQUEST(irq_c_prc_dpd) = handle;
		blob_id = /*X.RDB$PROCEDURE_BLR*/
			  jrd_158.jrd_159;
		procedure = MET_lookup_procedure(tdbb, work->dfw_name, !compile);

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_c_prc_dpd))
		REQUEST(irq_c_prc_dpd) = handle;

#ifdef DEV_BUILD
	MET_verify_cache(tdbb);
#endif

/* get any dependencies now by parsing the blr */

	if (procedure && !blob_id.isEmpty())
	{
		jrd_req* request = NULL;
		/* Nickolay Samofatov: allocate statement memory pool... */
		MemoryPool* new_pool = dbb->createPool();
		// block is used to ensure MET_verify_cache
		// works in not deleted context
		{
			Jrd::ContextPoolHolder context(tdbb, new_pool);

			const Firebird::MetaName depName(work->dfw_name);
			MET_get_dependencies(tdbb, NULL, NULL, 0, NULL, &blob_id, compile ? &request : NULL,
								 depName, obj_procedure, 0, transaction);

			if (request)
				CMP_release(tdbb, request);
			else
				dbb->deletePool(new_pool);
		}

#ifdef DEV_BUILD
		MET_verify_cache(tdbb);
#endif
	}
}


static void get_trigger_dependencies(DeferredWork* work, bool compile, jrd_tra* transaction)
{
   struct {
          TEXT  jrd_151 [32];	// RDB$RELATION_NAME 
          bid  jrd_152;	// RDB$TRIGGER_BLR 
          SSHORT jrd_153;	// gds__utility 
          SSHORT jrd_154;	// RDB$TRIGGER_TYPE 
   } jrd_150;
   struct {
          TEXT  jrd_149 [32];	// RDB$TRIGGER_NAME 
   } jrd_148;
/**************************************
 *
 *	g e t _ t r i g g e r _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *	Get relations and fields on which this
 *	trigger depends, either when it's being
 *	created or when it's modified.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	if (compile)
		compile = !(tdbb->getAttachment()->att_flags & ATT_gbak_attachment);

	jrd_rel* relation = NULL;
	bid blob_id;
	blob_id.clear();

	USHORT type = 0;

	jrd_req* handle = CMP_find_request(tdbb, irq_c_trigger, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle)
		X IN RDB$TRIGGERS WITH
			X.RDB$TRIGGER_NAME EQ work->dfw_name.c_str()*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_147, sizeof(jrd_147), true);
	gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_148.jrd_149, 32);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_148);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 44, (UCHAR*) &jrd_150);
	   if (!jrd_150.jrd_153) break;
		if (!REQUEST(irq_c_trigger))
			REQUEST(irq_c_trigger) = handle;
		blob_id = /*X.RDB$TRIGGER_BLR*/
			  jrd_150.jrd_152;
		type = (USHORT) /*X.RDB$TRIGGER_TYPE*/
				jrd_150.jrd_154;
		relation = MET_lookup_relation(tdbb, /*X.RDB$RELATION_NAME*/
						     jrd_150.jrd_151);

	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_c_trigger))
		REQUEST(irq_c_trigger) = handle;

/* get any dependencies now by parsing the blr */

	if ((relation || (type & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB) && !blob_id.isEmpty())
	{
		jrd_req* request = NULL;
		/* Nickolay Samofatov: allocate statement memory pool... */
		MemoryPool* new_pool = dbb->createPool();
		const USHORT par_flags = (USHORT) (type & 1) ? csb_pre_trigger : csb_post_trigger;

		Jrd::ContextPoolHolder context(tdbb, new_pool);
		const Firebird::MetaName depName(work->dfw_name);
		MET_get_dependencies(tdbb, relation, NULL, 0, NULL, &blob_id, compile ? &request : NULL,
							 depName, obj_trigger, par_flags, transaction);

		if (request)
			CMP_release(tdbb, request);
		else
			dbb->deletePool(new_pool);
	}
}


static void load_trigs(thread_db* tdbb, jrd_rel* relation, trig_vec** triggers)
{
/**************************************
 *
 *	l o a d _ t r i g s
 *
 **************************************
 *
 * Functional description
 *	We have just loaded the triggers onto the local vector
 *      triggers. Its now time to place them at their rightful
 *      place ie the relation block.
 *
 **************************************/
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


static Format* make_format(thread_db* tdbb, jrd_rel* relation, USHORT* version,
	TemporaryField* stack)
{
   struct {
          bid  jrd_144;	// RDB$DESCRIPTOR 
          SSHORT jrd_145;	// RDB$RELATION_ID 
          SSHORT jrd_146;	// RDB$FORMAT 
   } jrd_143;
/**************************************
 *
 *	m a k e _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Make a format block for a relation.
 *
 **************************************/
	TemporaryField* tfb;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* Figure out the highest field id and allocate a format block */

	USHORT count = 0;
	for (tfb = stack; tfb; tfb = tfb->tfb_next)
		count = MAX(count, tfb->tfb_id);

	Format* format = Format::newFormat(*dbb->dbb_permanent, count + 1);
	format->fmt_version = version ? *version : 0;

/* Fill in the format block from the temporary field blocks */

	for (tfb = stack; tfb; tfb = tfb->tfb_next)
	{
		dsc* desc = &format->fmt_desc[tfb->tfb_id];
		if (tfb->tfb_flags & TFB_array)
		{
			desc->dsc_dtype = dtype_array;
			desc->dsc_length = sizeof(ISC_QUAD);
		}
		else
			*desc = tfb->tfb_desc;
		if (tfb->tfb_flags & TFB_computed)
			desc->dsc_dtype |= COMPUTED_FLAG;
	}

/* Compute the offsets of the various fields */

	ULONG offset = FLAG_BYTES(count);

	count = 0;
	for (Format::fmt_desc_iterator desc2 = format->fmt_desc.begin();
		count < format->fmt_count;
		++count, ++desc2)
	{
		if (desc2->dsc_dtype & COMPUTED_FLAG)
		{
			desc2->dsc_dtype &= ~COMPUTED_FLAG;
			continue;
		}
		if (desc2->dsc_dtype)
		{
			offset = MET_align(dbb, &(*desc2), offset);
			desc2->dsc_address = (UCHAR *) (IPTR) offset;
			offset += desc2->dsc_length;
		}
	}

	format->fmt_length = (USHORT)offset;

/* Release the temporary field blocks */

	while ( (tfb = stack) )
	{
		stack = tfb->tfb_next;
		delete tfb;
	}

	if (offset > MAX_FORMAT_SIZE)
	{
		delete format;
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_rec_size_err) << Arg::Num(offset) <<
				 Arg::Gds(isc_table_name) << Arg::Str(relation->rel_name));
		/* Msg361: new record size of %ld bytes is too big */
	}

	Format* old_format;
	if (format->fmt_version &&
		(old_format = MET_format(tdbb, relation, (format->fmt_version - 1))) &&
		(formatsAreEqual(old_format, format)))
	{
		delete format;
		*version = old_format->fmt_version;
		return old_format;
	}

/* Link the format block into the world */

	vec<Format*>* vector = relation->rel_formats =
		vec<Format*>::newVector(*dbb->dbb_permanent, relation->rel_formats, format->fmt_version + 1);
	(*vector)[format->fmt_version] = format;

/* Store format in system relation */

	jrd_req* request = CMP_find_request(tdbb, irq_format3, IRQ_REQUESTS);

	/*STORE(REQUEST_HANDLE request)
		FMTS IN RDB$FORMATS*/
	{
	
		if (!REQUEST(irq_format3))
			REQUEST(irq_format3) = request;
		/*FMTS.RDB$FORMAT*/
		jrd_143.jrd_146 = format->fmt_version;
		/*FMTS.RDB$RELATION_ID*/
		jrd_143.jrd_145 = relation->rel_id;
		blb* blob = BLB_create(tdbb, dbb->dbb_sys_trans, &/*FMTS.RDB$DESCRIPTOR*/
								  jrd_143.jrd_144);

		if (sizeof(Ods::Descriptor) == sizeof(struct dsc) || dbb->dbb_ods_version < ODS_VERSION11)
		{
			// For ODS10 and earlier put descriptors in their in-memory representation
			// This makes 64-bit and 32-bit ODS10 different for the same architecture.
			BLB_put_segment(tdbb, blob, reinterpret_cast<const UCHAR*>(&(format->fmt_desc[0])),
							(USHORT)(format->fmt_count * sizeof(dsc)));
		}
		else {
			// Use generic representation of formats with 32-bit offsets

			Firebird::Array<Ods::Descriptor> odsDescs;
			Ods::Descriptor *odsDesc = odsDescs.getBuffer(format->fmt_count);

			for (Format::fmt_desc_const_iterator desc = format->fmt_desc.begin();
				 desc < format->fmt_desc.end(); ++desc, ++odsDesc)
			{
				*odsDesc = *desc;
			}

			BLB_put_segment(tdbb, blob, (UCHAR*) odsDescs.begin(),
							odsDescs.getCount() * sizeof(Ods::Descriptor));
		}
		BLB_close(tdbb, blob);
	/*END_STORE;*/
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_142, sizeof(jrd_142), true);
	EXE_start (tdbb, request, dbb->dbb_sys_trans);
	EXE_send (tdbb, request, 0, 12, (UCHAR*) &jrd_143);
	}

	if (!REQUEST(irq_format3))
		REQUEST(irq_format3) = request;

	return format;
}


static bool make_version(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_62;	// gds__utility 
   } jrd_61;
   struct {
          SSHORT jrd_60;	// gds__utility 
   } jrd_59;
   struct {
          SSHORT jrd_58;	// gds__utility 
   } jrd_57;
   struct {
          SSHORT jrd_56;	// RDB$RELATION_ID 
   } jrd_55;
   struct {
          SSHORT jrd_67;	// gds__utility 
          SSHORT jrd_68;	// RDB$DBKEY_LENGTH 
   } jrd_66;
   struct {
          TEXT  jrd_65 [32];	// RDB$VIEW_NAME 
   } jrd_64;
   struct {
          SSHORT jrd_73;	// gds__utility 
   } jrd_72;
   struct {
          TEXT  jrd_71 [32];	// RDB$RELATION_NAME 
   } jrd_70;
   struct {
          SSHORT jrd_120;	// gds__utility 
   } jrd_119;
   struct {
          SSHORT jrd_113;	// gds__null_flag 
          SSHORT jrd_114;	// RDB$FIELD_ID 
          SSHORT jrd_115;	// gds__null_flag 
          SSHORT jrd_116;	// RDB$UPDATE_FLAG 
          SSHORT jrd_117;	// gds__null_flag 
          SSHORT jrd_118;	// RDB$FIELD_POSITION 
   } jrd_112;
   struct {
          TEXT  jrd_78 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_79 [32];	// RDB$SECURITY_CLASS 
          bid  jrd_80;	// RDB$VALIDATION_BLR 
          bid  jrd_81;	// RDB$DEFAULT_VALUE 
          bid  jrd_82;	// RDB$DEFAULT_VALUE 
          bid  jrd_83;	// RDB$MISSING_VALUE 
          TEXT  jrd_84 [32];	// RDB$BASE_FIELD 
          TEXT  jrd_85 [32];	// RDB$FIELD_NAME 
          bid  jrd_86;	// RDB$COMPUTED_BLR 
          SSHORT jrd_87;	// gds__utility 
          SSHORT jrd_88;	// RDB$EXTERNAL_LENGTH 
          SSHORT jrd_89;	// RDB$EXTERNAL_SCALE 
          SSHORT jrd_90;	// RDB$EXTERNAL_TYPE 
          SSHORT jrd_91;	// RDB$DIMENSIONS 
          SSHORT jrd_92;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_93;	// RDB$FIELD_LENGTH 
          SSHORT jrd_94;	// RDB$FIELD_SCALE 
          SSHORT jrd_95;	// RDB$FIELD_TYPE 
          SSHORT jrd_96;	// gds__null_flag 
          SSHORT jrd_97;	// RDB$COLLATION_ID 
          SSHORT jrd_98;	// gds__null_flag 
          SSHORT jrd_99;	// RDB$COLLATION_ID 
          SSHORT jrd_100;	// gds__null_flag 
          SSHORT jrd_101;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_102;	// gds__null_flag 
          SSHORT jrd_103;	// RDB$NULL_FLAG 
          SSHORT jrd_104;	// RDB$NULL_FLAG 
          SSHORT jrd_105;	// RDB$VIEW_CONTEXT 
          SSHORT jrd_106;	// gds__null_flag 
          SSHORT jrd_107;	// RDB$FIELD_POSITION 
          SSHORT jrd_108;	// gds__null_flag 
          SSHORT jrd_109;	// RDB$UPDATE_FLAG 
          SSHORT jrd_110;	// gds__null_flag 
          SSHORT jrd_111;	// RDB$FIELD_ID 
   } jrd_77;
   struct {
          TEXT  jrd_76 [32];	// RDB$RELATION_NAME 
   } jrd_75;
   struct {
          SSHORT jrd_141;	// gds__utility 
   } jrd_140;
   struct {
          bid  jrd_135;	// RDB$RUNTIME 
          SSHORT jrd_136;	// gds__null_flag 
          SSHORT jrd_137;	// RDB$FORMAT 
          SSHORT jrd_138;	// RDB$FIELD_ID 
          SSHORT jrd_139;	// RDB$DBKEY_LENGTH 
   } jrd_134;
   struct {
          bid  jrd_125;	// RDB$RUNTIME 
          TEXT  jrd_126 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_127;	// RDB$VIEW_BLR 
          SSHORT jrd_128;	// gds__utility 
          SSHORT jrd_129;	// gds__null_flag 
          SSHORT jrd_130;	// RDB$DBKEY_LENGTH 
          SSHORT jrd_131;	// RDB$FIELD_ID 
          SSHORT jrd_132;	// RDB$FORMAT 
          SSHORT jrd_133;	// RDB$RELATION_ID 
   } jrd_124;
   struct {
          TEXT  jrd_123 [32];	// RDB$RELATION_NAME 
   } jrd_122;
/**************************************
 *
 *	m a k e _ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Make a new format version for a relation.  While we're at it, make
 *	sure all fields have id's.  If the relation is a view, make a
 *	a format anyway -- used for view updates.
 *
 *	While we're in the vicinity, also check the updatability of fields.
 *
 **************************************/
	TemporaryField* stack;
	TemporaryField* external;
	jrd_rel* relation;
	//bid blob_id;
	//blob_id.clear();

	USHORT n;
	int physical_fields = 0;
	bool external_flag = false;
	bool computed_field;
	trig_vec* triggers[TRIGGER_MAX];

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	bool null_view;

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		relation = NULL;
		stack = external = NULL;
		computed_field = false;

		for (n = 0; n < TRIGGER_MAX; n++) {
			triggers[n] = NULL;
		}

		jrd_req* request_fmt1 = CMP_find_request(tdbb, irq_format1, IRQ_REQUESTS);

		// User transaction may be safely used instead of system cause
		// all required dirty reads are performed in metadata cache. AP-2008.

		/*FOR(REQUEST_HANDLE request_fmt1 TRANSACTION_HANDLE transaction)
			REL IN RDB$RELATIONS WITH REL.RDB$RELATION_NAME EQ work->dfw_name.c_str()*/
		{
		if (!request_fmt1)
		   request_fmt1 = CMP_compile2 (tdbb, (UCHAR*) jrd_121, sizeof(jrd_121), true);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_122.jrd_123, 32);
		EXE_start (tdbb, request_fmt1, transaction);
		EXE_send (tdbb, request_fmt1, 0, 32, (UCHAR*) &jrd_122);
		while (1)
		   {
		   EXE_receive (tdbb, request_fmt1, 1, 284, (UCHAR*) &jrd_124);
		   if (!jrd_124.jrd_128) break;
			if (!REQUEST(irq_format1)) {
				REQUEST(irq_format1) = request_fmt1;
			}
			relation = MET_lookup_relation_id(tdbb, /*REL.RDB$RELATION_ID*/
								jrd_124.jrd_133, false);
			if (!relation)
			{
				EXE_unwind(tdbb, request_fmt1);
				return false;
			}
			const bid blob_id = /*REL.RDB$VIEW_BLR*/
					    jrd_124.jrd_127;
			null_view = blob_id.isEmpty();
			external_flag = /*REL.RDB$EXTERNAL_FILE*/
					jrd_124.jrd_126[0];
			if (/*REL.RDB$FORMAT*/
			    jrd_124.jrd_132 == MAX_TABLE_VERSIONS)
			{
				EXE_unwind(tdbb, request_fmt1);
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_table_name) << Arg::Str(work->dfw_name) <<
						 Arg::Gds(isc_version_err));
				/* Msg357: too many versions */
			}
			/*MODIFY REL USING*/
			{
			
				blb* blob = BLB_create(tdbb, transaction, &/*REL.RDB$RUNTIME*/
									   jrd_124.jrd_125);
				jrd_req* request_fmtx = CMP_find_request(tdbb, irq_format2, IRQ_REQUESTS);

				/*FOR(REQUEST_HANDLE request_fmtx TRANSACTION_HANDLE transaction)
					RFR IN RDB$RELATION_FIELDS CROSS
						FLD IN RDB$FIELDS WITH
						RFR.RDB$RELATION_NAME EQ work->dfw_name.c_str() AND
						RFR.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME*/
				{
				if (!request_fmtx)
				   request_fmtx = CMP_compile2 (tdbb, (UCHAR*) jrd_74, sizeof(jrd_74), true);
				gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_75.jrd_76, 32);
				EXE_start (tdbb, request_fmtx, transaction);
				EXE_send (tdbb, request_fmtx, 0, 32, (UCHAR*) &jrd_75);
				while (1)
				   {
				   EXE_receive (tdbb, request_fmtx, 1, 218, (UCHAR*) &jrd_77);
				   if (!jrd_77.jrd_87) break;

					/* Update RFR to reflect new fields id */

					if (!/*RFR.RDB$FIELD_ID.NULL*/
					     jrd_77.jrd_110 && /*RFR.RDB$FIELD_ID*/
    jrd_77.jrd_111 >= /*REL.RDB$FIELD_ID*/
    jrd_124.jrd_131)
						/*REL.RDB$FIELD_ID*/
						jrd_124.jrd_131 = /*RFR.RDB$FIELD_ID*/
   jrd_77.jrd_111 + 1;

					// force recalculation of RDB$UPDATE_FLAG if field is calculated
					if (!/*FLD.RDB$COMPUTED_BLR*/
					     jrd_77.jrd_86.isEmpty())
					{
						/*RFR.RDB$UPDATE_FLAG.NULL*/
						jrd_77.jrd_108 = TRUE;
						computed_field = true;
					}

					if (/*RFR.RDB$FIELD_ID.NULL*/
					    jrd_77.jrd_110 || /*RFR.RDB$UPDATE_FLAG.NULL*/
    jrd_77.jrd_108)
					{
						/*MODIFY RFR USING*/
						{
						
							if (/*RFR.RDB$FIELD_ID.NULL*/
							    jrd_77.jrd_110)
							{
								if (external_flag)
								{
									/*RFR.RDB$FIELD_ID*/
									jrd_77.jrd_111 = /*RFR.RDB$FIELD_POSITION*/
   jrd_77.jrd_107;
									/* RFR.RDB$FIELD_POSITION.NULL is
									   needed to be referenced in the
									   code somewhere for GPRE to include
									   this field in the structures that
									   it generates at the top of this func. */
									/*RFR.RDB$FIELD_ID.NULL*/
									jrd_77.jrd_110 = /*RFR.RDB$FIELD_POSITION.NULL*/
   jrd_77.jrd_106;
								}
								else
								{
									/*RFR.RDB$FIELD_ID*/
									jrd_77.jrd_111 = /*REL.RDB$FIELD_ID*/
   jrd_124.jrd_131;
									/*RFR.RDB$FIELD_ID.NULL*/
									jrd_77.jrd_110 = FALSE;
								}
								/*REL.RDB$FIELD_ID*/
								jrd_124.jrd_131++;
							}
							if (/*RFR.RDB$UPDATE_FLAG.NULL*/
							    jrd_77.jrd_108)
							{
								/*RFR.RDB$UPDATE_FLAG.NULL*/
								jrd_77.jrd_108 = FALSE;
								/*RFR.RDB$UPDATE_FLAG*/
								jrd_77.jrd_109 = 1;
								if (!/*FLD.RDB$COMPUTED_BLR*/
								     jrd_77.jrd_86.isEmpty())
								{
									/*RFR.RDB$UPDATE_FLAG*/
									jrd_77.jrd_109 = 0;
								}
								if (!null_view && /*REL.RDB$DBKEY_LENGTH*/
										  jrd_124.jrd_130 > 8)
								{
									jrd_req* temp = NULL;
									/*RFR.RDB$UPDATE_FLAG*/
									jrd_77.jrd_109 = 0;
									/*FOR(REQUEST_HANDLE temp) X IN RDB$TRIGGERS WITH
										X.RDB$RELATION_NAME EQ work->dfw_name.c_str() AND
											X.RDB$TRIGGER_TYPE EQ 1*/
									{
									if (!temp)
									   temp = CMP_compile2 (tdbb, (UCHAR*) jrd_69, sizeof(jrd_69), true);
									gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_70.jrd_71, 32);
									EXE_start (tdbb, temp, dbb->dbb_sys_trans);
									EXE_send (tdbb, temp, 0, 32, (UCHAR*) &jrd_70);
									while (1)
									   {
									   EXE_receive (tdbb, temp, 1, 2, (UCHAR*) &jrd_72);
									   if (!jrd_72.jrd_73) break;
										/*RFR.RDB$UPDATE_FLAG*/
										jrd_77.jrd_109 = 1;
									/*END_FOR;*/
									   }
									}
									CMP_release(tdbb, temp);
								}
							}
						/*END_MODIFY;*/
						jrd_112.jrd_113 = jrd_77.jrd_110;
						jrd_112.jrd_114 = jrd_77.jrd_111;
						jrd_112.jrd_115 = jrd_77.jrd_108;
						jrd_112.jrd_116 = jrd_77.jrd_109;
						jrd_112.jrd_117 = jrd_77.jrd_106;
						jrd_112.jrd_118 = jrd_77.jrd_107;
						EXE_send (tdbb, request_fmtx, 2, 12, (UCHAR*) &jrd_112);
						}
					}

					/* Store stuff in field summary */

					n = /*RFR.RDB$FIELD_ID*/
					    jrd_77.jrd_111;
					put_summary_record(tdbb, blob, RSR_field_id, (UCHAR*)&n, sizeof(n));
					put_summary_record(tdbb, blob, RSR_field_name, (UCHAR*) /*RFR.RDB$FIELD_NAME*/
												jrd_77.jrd_85,
									   fb_utils::name_length(/*RFR.RDB$FIELD_NAME*/
												 jrd_77.jrd_85));
					if (!/*FLD.RDB$COMPUTED_BLR*/
					     jrd_77.jrd_86.isEmpty() && !/*RFR.RDB$VIEW_CONTEXT*/
	       jrd_77.jrd_105)
					{
						put_summary_blob(tdbb, blob, RSR_computed_blr, &/*FLD.RDB$COMPUTED_BLR*/
												jrd_77.jrd_86, transaction);
					}
					else if (!null_view)
					{
						n = /*RFR.RDB$VIEW_CONTEXT*/
						    jrd_77.jrd_105;
						put_summary_record(tdbb, blob, RSR_view_context, (UCHAR*)&n, sizeof(n));
						put_summary_record(tdbb, blob, RSR_base_field, (UCHAR*) /*RFR.RDB$BASE_FIELD*/
													jrd_77.jrd_84,
										   fb_utils::name_length(/*RFR.RDB$BASE_FIELD*/
													 jrd_77.jrd_84));
					}
					put_summary_blob(tdbb, blob, RSR_missing_value, &/*FLD.RDB$MISSING_VALUE*/
											 jrd_77.jrd_83, transaction);
					put_summary_blob(tdbb, blob, RSR_default_value,
								(/*RFR.RDB$DEFAULT_VALUE*/
								 jrd_77.jrd_82.isEmpty() ?
									&/*FLD.RDB$DEFAULT_VALUE*/
									 jrd_77.jrd_81 :
									&/*RFR.RDB$DEFAULT_VALUE*/
									 jrd_77.jrd_82), transaction);
					put_summary_blob(tdbb, blob, RSR_validation_blr, &/*FLD.RDB$VALIDATION_BLR*/
											  jrd_77.jrd_80, transaction);
					if (/*FLD.RDB$NULL_FLAG*/
					    jrd_77.jrd_104 || /*RFR.RDB$NULL_FLAG*/
    jrd_77.jrd_103)
					{
						put_summary_record(tdbb, blob, RSR_field_not_null,
											nonnull_validation_blr, sizeof(nonnull_validation_blr));
					}

					n = fb_utils::name_length(/*RFR.RDB$SECURITY_CLASS*/
								  jrd_77.jrd_79);
					if (!/*RFR.RDB$SECURITY_CLASS.NULL*/
					     jrd_77.jrd_102 && n)
						put_summary_record(tdbb, blob, RSR_security_class,
										   (UCHAR*) /*RFR.RDB$SECURITY_CLASS*/
											    jrd_77.jrd_79, n);

					/* Make a temporary field block */

					TemporaryField* tfb = FB_NEW(*tdbb->getDefaultPool()) TemporaryField;
					tfb->tfb_next = stack;
					stack = tfb;

					/* for text data types, grab the CHARACTER_SET and
					   COLLATION to give the type of international text */

					if (/*FLD.RDB$CHARACTER_SET_ID.NULL*/
					    jrd_77.jrd_100)
						/*FLD.RDB$CHARACTER_SET_ID*/
						jrd_77.jrd_101 = CS_NONE;

					SSHORT collation = COLLATE_NONE;	/* codepoint collation */
					if (!/*FLD.RDB$COLLATION_ID.NULL*/
					     jrd_77.jrd_98)
						collation = /*FLD.RDB$COLLATION_ID*/
							    jrd_77.jrd_99;
					if (!/*RFR.RDB$COLLATION_ID.NULL*/
					     jrd_77.jrd_96)
						collation = /*RFR.RDB$COLLATION_ID*/
							    jrd_77.jrd_97;

					if (!DSC_make_descriptor(&tfb->tfb_desc, /*FLD.RDB$FIELD_TYPE*/
										 jrd_77.jrd_95,
											 /*FLD.RDB$FIELD_SCALE*/
											 jrd_77.jrd_94,
											 /*FLD.RDB$FIELD_LENGTH*/
											 jrd_77.jrd_93,
											 /*FLD.RDB$FIELD_SUB_TYPE*/
											 jrd_77.jrd_92,
											 /*FLD.RDB$CHARACTER_SET_ID*/
											 jrd_77.jrd_101, collation))
					{
						EXE_unwind(tdbb, request_fmt1);
						EXE_unwind(tdbb, request_fmtx);
						if (/*REL.RDB$FORMAT.NULL*/
						    jrd_124.jrd_129)
						{
							DPM_delete_relation(tdbb, relation);
						}
						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_random) << Arg::Str(work->dfw_name));
					}

					/* Make sure the text type specified is implemented */
					if (!validate_text_type(tdbb, tfb))
					{
						EXE_unwind(tdbb, request_fmt1);
						EXE_unwind(tdbb, request_fmtx);
						if (/*REL.RDB$FORMAT.NULL*/
						    jrd_124.jrd_129)
						{
							DPM_delete_relation(tdbb, relation);
						}
						ERR_post_nothrow(Arg::Gds(isc_no_meta_update) <<
										 Arg::Gds(isc_random) << Arg::Str(work->dfw_name));
						INTL_texttype_lookup(tdbb,
							(DTYPE_IS_TEXT(tfb->tfb_desc.dsc_dtype) ?
								tfb->tfb_desc.dsc_ttype() : tfb->tfb_desc.dsc_blob_ttype()));	// should punt
						ERR_punt(); // if INTL_texttype_lookup hasn't punt
					}

					// dimitr: view fields shouldn't be marked as computed
					if (null_view && !/*FLD.RDB$COMPUTED_BLR*/
							  jrd_77.jrd_86.isEmpty())
						tfb->tfb_flags |= TFB_computed;
					else
						++physical_fields;

					tfb->tfb_id = /*RFR.RDB$FIELD_ID*/
						      jrd_77.jrd_111;

					if ((n = /*FLD.RDB$DIMENSIONS*/
						 jrd_77.jrd_91))
						setup_array(tdbb, blob, /*FLD.RDB$FIELD_NAME*/
									jrd_77.jrd_78, n, tfb);

					if (external_flag)
					{
						tfb = FB_NEW(*tdbb->getDefaultPool()) TemporaryField;
						tfb->tfb_next = external;
						external = tfb;
						fb_assert(/*FLD.RDB$EXTERNAL_TYPE*/
							  jrd_77.jrd_90 <= MAX_UCHAR);
						tfb->tfb_desc.dsc_dtype = (UCHAR)/*FLD.RDB$EXTERNAL_TYPE*/
										 jrd_77.jrd_90;
						fb_assert(/*FLD.RDB$EXTERNAL_SCALE*/
							  jrd_77.jrd_89 >= MIN_SCHAR &&
							/*FLD.RDB$EXTERNAL_SCALE*/
							jrd_77.jrd_89 <= MAX_SCHAR);
						tfb->tfb_desc.dsc_scale = (SCHAR)/*FLD.RDB$EXTERNAL_SCALE*/
										 jrd_77.jrd_89;
						tfb->tfb_desc.dsc_length = /*FLD.RDB$EXTERNAL_LENGTH*/
									   jrd_77.jrd_88;
						tfb->tfb_id = /*RFR.RDB$FIELD_ID*/
							      jrd_77.jrd_111;
					}
				/*END_FOR;*/
				   EXE_send (tdbb, request_fmtx, 3, 2, (UCHAR*) &jrd_119);
				   }
				}

				if (!REQUEST(irq_format2))
					REQUEST(irq_format2) = request_fmtx;

				if (null_view && !physical_fields)
				{
					EXE_unwind(tdbb, request_fmt1);
					if (/*REL.RDB$FORMAT.NULL*/
					    jrd_124.jrd_129)
					{
						DPM_delete_relation(tdbb, relation);
					}
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_table_name) << Arg::Str(work->dfw_name) <<
			 				 Arg::Gds(isc_must_have_phys_field));
				}

				blob = setup_triggers(tdbb, relation, null_view, triggers, blob);

				BLB_close(tdbb, blob);
				USHORT version = /*REL.RDB$FORMAT.NULL*/
						 jrd_124.jrd_129 ? 0 : /*REL.RDB$FORMAT*/
       jrd_124.jrd_132;
				version++;
				relation->rel_current_format = make_format(tdbb, relation, &version, stack);
				/*REL.RDB$FORMAT.NULL*/
				jrd_124.jrd_129 = FALSE;
				/*REL.RDB$FORMAT*/
				jrd_124.jrd_132 = version;

				if (!null_view)
				{
					// update the dbkey length to include each of the base relations

					/*REL.RDB$DBKEY_LENGTH*/
					jrd_124.jrd_130 = 0;

					jrd_req* handle = NULL;
					/*FOR(REQUEST_HANDLE handle)
						Z IN RDB$VIEW_RELATIONS
						CROSS R IN RDB$RELATIONS OVER RDB$RELATION_NAME
						WITH Z.RDB$VIEW_NAME = work->dfw_name.c_str()*/
					{
					if (!handle)
					   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_63, sizeof(jrd_63), true);
					gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_64.jrd_65, 32);
					EXE_start (tdbb, handle, dbb->dbb_sys_trans);
					EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_64);
					while (1)
					   {
					   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_66);
					   if (!jrd_66.jrd_67) break;
					{
						/*REL.RDB$DBKEY_LENGTH*/
						jrd_124.jrd_130 += /*R.RDB$DBKEY_LENGTH*/
    jrd_66.jrd_68;
					}
					/*END_FOR;*/
					   }
					}
					CMP_release(tdbb, handle);
				}
			/*END_MODIFY;*/
			jrd_134.jrd_135 = jrd_124.jrd_125;
			jrd_134.jrd_136 = jrd_124.jrd_129;
			jrd_134.jrd_137 = jrd_124.jrd_132;
			jrd_134.jrd_138 = jrd_124.jrd_131;
			jrd_134.jrd_139 = jrd_124.jrd_130;
			EXE_send (tdbb, request_fmt1, 2, 16, (UCHAR*) &jrd_134);
			}
		/*END_FOR;*/
		   EXE_send (tdbb, request_fmt1, 3, 2, (UCHAR*) &jrd_140);
		   }
		}

		if (!REQUEST(irq_format1))
			REQUEST(irq_format1) = request_fmt1;

		/* If we didn't find the relation, it is probably being dropped */

		if (!relation) {
			return false;
		}

		if (!(relation->rel_flags & REL_sys_trigs_being_loaded))
		    load_trigs(tdbb, relation, triggers);

		/* in case somebody changed the view definition or a computed
		   field, reset the dependencies by deleting the current ones
		   and setting a flag for MET_scan_relation to find the new ones */

		if (!null_view)
			MET_delete_dependencies(tdbb, work->dfw_name, obj_view, transaction);

		{ // begin scope
			const DeferredWork* arg = work->findArg(dfw_arg_force_computed);
			if (arg)
			{
				computed_field = true;
				MET_delete_dependencies(tdbb, arg->dfw_name, obj_computed, transaction);
			}
		} // end scope

		if (!null_view || computed_field)
			relation->rel_flags |= REL_get_dependencies;

		if (external_flag)
		{
			jrd_req* temp = NULL;
			/*FOR(REQUEST_HANDLE temp) FMTS IN RDB$FORMATS WITH
				FMTS.RDB$RELATION_ID EQ relation->rel_id AND
				FMTS.RDB$FORMAT EQ 0*/
			{
			if (!temp)
			   temp = CMP_compile2 (tdbb, (UCHAR*) jrd_54, sizeof(jrd_54), true);
			jrd_55.jrd_56 = relation->rel_id;
			EXE_start (tdbb, temp, dbb->dbb_sys_trans);
			EXE_send (tdbb, temp, 0, 2, (UCHAR*) &jrd_55);
			while (1)
			   {
			   EXE_receive (tdbb, temp, 1, 2, (UCHAR*) &jrd_57);
			   if (!jrd_57.jrd_58) break;
				/*ERASE FMTS;*/
				EXE_send (tdbb, temp, 2, 2, (UCHAR*) &jrd_59);
			/*END_FOR;*/
			   EXE_send (tdbb, temp, 3, 2, (UCHAR*) &jrd_61);
			   }
			}
			CMP_release(tdbb, temp);
			make_format(tdbb, relation, 0, external);
		}

		relation->rel_flags &= ~REL_scanned;
		DFW_post_work(transaction, dfw_scan_relation, NULL, relation->rel_id);

		break;
	}

	return false;
}


static bool modify_procedure(thread_db*	tdbb,
							SSHORT	phase,
							DeferredWork*		work,
							jrd_tra*		transaction)
{
   struct {
          SSHORT jrd_53;	// gds__utility 
   } jrd_52;
   struct {
          SSHORT jrd_50;	// gds__null_flag 
          SSHORT jrd_51;	// RDB$VALID_BLR 
   } jrd_49;
   struct {
          SSHORT jrd_46;	// gds__utility 
          SSHORT jrd_47;	// gds__null_flag 
          SSHORT jrd_48;	// RDB$VALID_BLR 
   } jrd_45;
   struct {
          SSHORT jrd_44;	// RDB$PROCEDURE_ID 
   } jrd_43;
/**************************************
 *
 *	m o d i f y _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Perform required actions when modifying procedure.
 *
 **************************************/
	jrd_prc* procedure;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
	{
		procedure = MET_lookup_procedure_id(tdbb, work->dfw_id, false, true, 0);
		if (!procedure) {
			return false;
		}

		if (procedure->prc_existence_lock)
		{
			LCK_convert(tdbb, procedure->prc_existence_lock, LCK_SR, transaction->getLockWait());
		}
		return false;
	}
	case 1:
		return true;

	case 2:
		return true;

	case 3:
		procedure = MET_lookup_procedure_id(tdbb, work->dfw_id, false, true, 0);
		if (!procedure) {
			return false;
		}

		if (procedure->prc_existence_lock)
		{
			/* Let procedure be deleted if only this transaction is using it */

			if (!LCK_convert(tdbb, procedure->prc_existence_lock, LCK_EX, transaction->getLockWait()))
			{
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_obj_in_use) << Arg::Str(work->dfw_name));
			}
		}

		/* If we are in a multi-client server, someone else may have marked
		   procedure obsolete. Unmark and we will remark it later. */

		procedure->prc_flags &= ~PRC_obsolete;
		return true;

	case 4:
		procedure = MET_lookup_procedure_id(tdbb, work->dfw_id, false, true, 0);
		if (!procedure) {
			return false;
		}

		{ // guard scope
			Database::CheckoutLockGuard guard(dbb, dbb->dbb_meta_mutex);

			// Do not allow to modify procedure used by user requests
			if (procedure->prc_use_count && MET_procedure_in_use(tdbb, procedure))
			{
				/*
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_obj_in_use) << Arg::Str(work->dfw_name));
				*/
				gds__log("Modifying procedure %s which is currently in use by active user requests",
						 work->dfw_name.c_str());

				USHORT prc_alter_count = procedure->prc_alter_count;
				if (prc_alter_count > MAX_PROC_ALTER)
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_proc_name) << Arg::Str(work->dfw_name) <<
							 Arg::Gds(isc_version_err));
					/* Msg357: too many versions */
				}

				if (procedure->prc_existence_lock) {
					LCK_release(tdbb, procedure->prc_existence_lock);
				}
				(*tdbb->getDatabase()->dbb_procedures)[procedure->prc_id] = NULL;

				if (!(procedure = MET_lookup_procedure_id(tdbb, work->dfw_id, false,
														  true, PRC_being_altered)))
				{
					return false;
				}
				procedure->prc_alter_count = ++prc_alter_count;
			}

			procedure->prc_flags |= PRC_being_altered;
			if (procedure->prc_request)
			{
				if (CMP_clone_is_active(procedure->prc_request))
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_obj_in_use) << Arg::Str(work->dfw_name));

				/* release the request */

				MET_release_procedure_request(tdbb, procedure);
			}

			/* delete dependency lists */

			MET_delete_dependencies(tdbb, work->dfw_name, obj_procedure, transaction);

			/* the procedure has just been scanned by MET_lookup_procedure_id
			   and its PRC_scanned flag is set. We are going to reread it
			   from file (create all new dependencies) and do not want this
			   flag to be set. That is why we do not add PRC_obsolete and
			   PRC_being_altered flags, we set only these two flags
			 */
			procedure->prc_flags = (PRC_obsolete | PRC_being_altered);

			if (procedure->prc_existence_lock) {
				LCK_release(tdbb, procedure->prc_existence_lock);
			}

			/* remove procedure from cache */

			MET_remove_procedure(tdbb, work->dfw_id, NULL);

			/* Now handle the new definition */
			bool compile = !work->findArg(dfw_arg_check_blr);
			get_procedure_dependencies(work, compile, transaction);

			procedure->prc_flags &= ~(PRC_obsolete | PRC_being_altered);
		}
		return true;

	case 5:
		if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
		{
			const DeferredWork* arg = work->findArg(dfw_arg_check_blr);
			if (arg)
			{
				SSHORT valid_blr = FALSE;

				MemoryPool* new_pool = dbb->createPool();
				try
				{
					Jrd::ContextPoolHolder context(tdbb, new_pool);

					// compile the procedure to know if the BLR is still valid
					if (MET_procedure(tdbb, work->dfw_id, false, 0))
						valid_blr = TRUE;
				}
				catch (const Firebird::Exception&)
				{
					fb_utils::init_status(tdbb->tdbb_status_vector);
				}

				dbb->deletePool(new_pool);

				jrd_req* request = CMP_find_request(tdbb, irq_prc_validate, IRQ_REQUESTS);

				/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
					PRC IN RDB$PROCEDURES WITH
						PRC.RDB$PROCEDURE_ID EQ work->dfw_id*/
				{
				if (!request)
				   request = CMP_compile2 (tdbb, (UCHAR*) jrd_42, sizeof(jrd_42), true);
				jrd_43.jrd_44 = work->dfw_id;
				EXE_start (tdbb, request, transaction);
				EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_43);
				while (1)
				   {
				   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_45);
				   if (!jrd_45.jrd_46) break;

					if (!REQUEST(irq_prc_validate))
						REQUEST(irq_prc_validate) = request;

					/*MODIFY PRC USING*/
					{
					
						/*PRC.RDB$VALID_BLR*/
						jrd_45.jrd_48 = valid_blr;
						/*PRC.RDB$VALID_BLR.NULL*/
						jrd_45.jrd_47 = FALSE;
					/*END_MODIFY;*/
					jrd_49.jrd_50 = jrd_45.jrd_47;
					jrd_49.jrd_51 = jrd_45.jrd_48;
					EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_49);
					}
				/*END_FOR;*/
				   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_52);
				   }
				}

				if (!REQUEST(irq_prc_validate))
					REQUEST(irq_prc_validate) = request;
			}
		}
		break;
	}

	return false;
}


static bool modify_trigger(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_41;	// gds__utility 
   } jrd_40;
   struct {
          SSHORT jrd_38;	// gds__null_flag 
          SSHORT jrd_39;	// RDB$VALID_BLR 
   } jrd_37;
   struct {
          SSHORT jrd_34;	// gds__utility 
          SSHORT jrd_35;	// gds__null_flag 
          SSHORT jrd_36;	// RDB$VALID_BLR 
   } jrd_33;
   struct {
          TEXT  jrd_32 [32];	// RDB$TRIGGER_NAME 
   } jrd_31;
/**************************************
 *
 *	m o d i f y _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Perform required actions when modifying trigger.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			bool compile = !work->findArg(dfw_arg_check_blr);

			/* get rid of old dependencies, bring in the new */
			MET_delete_dependencies(tdbb, work->dfw_name, obj_trigger, transaction);
			get_trigger_dependencies(work, compile, transaction);
		}
		return true;

	case 4:
		{
			const DeferredWork* arg = work->findArg(dfw_arg_rel_name);
			if (!arg)
			{
				arg = work->findArg(dfw_arg_trg_type);
				fb_assert(arg);

				if (arg && (arg->dfw_id & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB)
				{
					MET_release_trigger(tdbb,
						&tdbb->getDatabase()->dbb_triggers[arg->dfw_id & ~TRIGGER_TYPE_DB],
						work->dfw_name);

					MET_load_trigger(tdbb, NULL, work->dfw_name,
						&tdbb->getDatabase()->dbb_triggers[arg->dfw_id & ~TRIGGER_TYPE_DB]);
				}
			}
		}

		if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
		{
			const DeferredWork* arg = work->findArg(dfw_arg_check_blr);
			if (arg)
			{
				const Firebird::MetaName relation_name(arg->dfw_name);
				SSHORT valid_blr = FALSE;

				try
				{
					jrd_rel* relation = MET_lookup_relation(tdbb, relation_name);

					if (relation)
					{
						// remove cached triggers from relation
						relation->rel_flags &= ~REL_scanned;
						MET_scan_relation(tdbb, relation);

						trig_vec* triggers[TRIGGER_MAX];

						for (int i = 0; i < TRIGGER_MAX; ++i)
							triggers[i] = NULL;

						MemoryPool* new_pool = dbb->createPool();
						try
						{
							Jrd::ContextPoolHolder context(tdbb, new_pool);

							MET_load_trigger(tdbb, relation, work->dfw_name, triggers);

							for (int i = 0; i < TRIGGER_MAX; ++i)
							{
								if (triggers[i])
								{
									for (size_t j = 0; j < triggers[i]->getCount(); ++j)
										(*triggers[i])[j].compile(tdbb);

									MET_release_triggers(tdbb, &triggers[i]);
								}
							}

							valid_blr = TRUE;
						}
						catch (const Firebird::Exception&)
						{
							dbb->deletePool(new_pool);
							throw;
						}

						dbb->deletePool(new_pool);
					}
				}
				catch (const Firebird::Exception&)
				{
				}

				jrd_req* request = CMP_find_request(tdbb, irq_trg_validate, IRQ_REQUESTS);

				/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
					TRG IN RDB$TRIGGERS WITH
						TRG.RDB$TRIGGER_NAME EQ work->dfw_name.c_str()*/
				{
				if (!request)
				   request = CMP_compile2 (tdbb, (UCHAR*) jrd_30, sizeof(jrd_30), true);
				gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_31.jrd_32, 32);
				EXE_start (tdbb, request, transaction);
				EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_31);
				while (1)
				   {
				   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_33);
				   if (!jrd_33.jrd_34) break;

					if (!REQUEST(irq_trg_validate))
						REQUEST(irq_trg_validate) = request;

					/*MODIFY TRG USING*/
					{
					
						/*TRG.RDB$VALID_BLR*/
						jrd_33.jrd_36 = valid_blr;
						/*TRG.RDB$VALID_BLR.NULL*/
						jrd_33.jrd_35 = FALSE;
					/*END_MODIFY;*/
					jrd_37.jrd_38 = jrd_33.jrd_35;
					jrd_37.jrd_39 = jrd_33.jrd_36;
					EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_37);
					}
				/*END_FOR;*/
				   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_40);
				   }
				}

				if (!REQUEST(irq_trg_validate))
					REQUEST(irq_trg_validate) = request;
			}
		}
		break;
	}

	return false;
}


static void put_summary_blob(thread_db* tdbb, blb* blob, rsr_t type, bid* blob_id, jrd_tra* transaction)
{
/**************************************
 *
 *	p u t _ s u m m a r y _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Put an attribute record to the relation summary blob.
 *
 **************************************/

	UCHAR temp[128];

	SET_TDBB(tdbb);
	//Database* dbb  = tdbb->getDatabase();

	/* If blob is null, don't bother */

	if (blob_id->isEmpty())
		return;

	// Go ahead and open blob
	blb* blr = BLB_open(tdbb, transaction, blob_id);
	fb_assert(blr->blb_length <= MAX_USHORT);
	USHORT length = (USHORT)blr->blb_length;
	UCHAR* const buffer = (length > sizeof(temp)) ?
		FB_NEW(*getDefaultMemoryPool()) UCHAR[length] : temp;
	try {
		length = (USHORT)BLB_get_data(tdbb, blr, buffer, (SLONG) length);
		put_summary_record(tdbb, blob, type, buffer, length);
	}
	catch (const Firebird::Exception&) {
		if (buffer != temp)
			delete[] buffer;
		throw;
	}

	if (buffer != temp)
		delete[] buffer;
}


static Lock* protect_relation(thread_db* tdbb, jrd_tra* transaction, jrd_rel* relation, bool& releaseLock)
{
/**************************************
 *
 * p r o t e c t _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Lock relation with protected_read level or raise existing relation lock
 * to this level to ensure nobody can write to this relation.
 * Used when new index is built.
 * releaseLock set to true if there was no existing lock before
 *
 **************************************/
	Lock* relLock = RLCK_transaction_relation_lock(tdbb, transaction, relation);

	releaseLock = (relLock->lck_logical == LCK_none);

	bool inUse = false;

	if (!releaseLock) {
		if ( (relLock->lck_logical < LCK_PR) &&
			!LCK_convert(tdbb, relLock, LCK_PR, transaction->getLockWait()) )
		{
			inUse = true;
		}
	}
	else {
		if (!LCK_lock(tdbb, relLock, LCK_PR, transaction->getLockWait())) {
			inUse = true;
		}
	}

	if (inUse ) {
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_obj_in_use) << Arg::Str(relation->rel_name));
	}

	return relLock;
}


static void put_summary_record(thread_db* tdbb,
							   blb*		blob,
							   rsr_t	type,
							   const UCHAR*	data,
							   USHORT	length)
{
/**************************************
 *
 *	p u t _ s u m m a r y _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Put an attribute record to the relation summary blob.
 *
 **************************************/

	SET_TDBB(tdbb);

	fb_assert(length < MAX_USHORT); // otherwise length + 1 wraps. Or do we bugcheck???
	UCHAR temp[129];

	UCHAR* const buffer = ((size_t) (length + 1) > sizeof(temp)) ?
		FB_NEW(*getDefaultMemoryPool()) UCHAR[length + 1] : temp;

	UCHAR* p = buffer;
	*p++ = (UCHAR) type;
	memcpy(p, data, length);

	try {
		BLB_put_segment(tdbb, blob, buffer, length + 1);
	}
	catch (const Firebird::Exception&) {
		if (buffer != temp)
			delete[] buffer;
		throw;
	}

	if (buffer != temp)
		delete[] buffer;
}


static void release_protect_lock(thread_db* tdbb, jrd_tra*	transaction, Lock* relLock)
{
	vec<Lock*>* vector = transaction->tra_relation_locks;
	if (vector) {
		vec<Lock*>::iterator lock = vector->begin();
		for (ULONG i = 0; i < vector->count(); ++i, ++lock)
		{
			if (*lock == relLock)
			{
				LCK_release(tdbb, relLock);
				*lock = 0;
				break;
			}
		}
	}
}


static bool scan_relation(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
/**************************************
 *
 *	s c a n _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Call MET_scan_relation with the appropriate
 *	relation.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		// dimitr:	I suspect that nobody expects an updated format to
		//			appear at stage 3, so the logic would work reliably
		//			if this line is removed (and hence we rely on the
		//			4th stage only). But I leave it here for the time being.
		MET_scan_relation(tdbb, MET_relation(tdbb, work->dfw_id));
		return true;

	case 4:
		MET_scan_relation(tdbb, MET_relation(tdbb, work->dfw_id));
		break;
	}

	return false;
}

#ifdef NOT_USED_OR_REPLACED
static bool shadow_defined(thread_db* tdbb)
{
   struct {
          SSHORT jrd_29;	// gds__utility 
   } jrd_28;
/**************************************
 *
 *	s h a d o w _ d e f i n e d
 *
 **************************************
 *
 * Functional description
 *	Return true if any shadows have been has been defined
 *      for the database else return false.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	bool result = false;
	jrd_req*  handle = NULL;

	/*FOR(REQUEST_HANDLE handle) FIRST 1 X IN RDB$FILES
		WITH X.RDB$SHADOW_NUMBER > 0*/
	{
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_27, sizeof(jrd_27), true);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 2, (UCHAR*) &jrd_28);
	   if (!jrd_28.jrd_29) break;
		result = true;
	/*END_FOR*/
	   }
	}

	CMP_release(tdbb, handle);

	return result;
}
#endif


static void setup_array(thread_db* tdbb, blb* blob, const TEXT* field_name, USHORT n,
	TemporaryField* tfb)
{
/**************************************
 *
 *	s e t u p _ a r r a y
 *
 **************************************
 *
 * Functional description
 *
 *	setup an array descriptor in a tfb
 *
 **************************************/

	SLONG stuff[256];

	put_summary_record(tdbb, blob, RSR_dimensions, (UCHAR*) &n, sizeof(n));
	tfb->tfb_flags |= TFB_array;
	Ods::InternalArrayDesc* array = reinterpret_cast<Ods::InternalArrayDesc*>(stuff);
	MOVE_CLEAR(array, (SLONG) sizeof(Ods::InternalArrayDesc));
	array->iad_dimensions = n;
	array->iad_struct_count = 1;
	array->iad_rpt[0].iad_desc = tfb->tfb_desc;
	get_array_desc(tdbb, field_name, array);
	put_summary_record(tdbb, blob, RSR_array_desc, (UCHAR*) array, array->iad_length);
}


static blb* setup_triggers(thread_db* tdbb, jrd_rel* relation, bool null_view,
	trig_vec** triggers, blb* blob)
{
   struct {
          TEXT  jrd_8 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_9;	// gds__utility 
          SSHORT jrd_10;	// RDB$TRIGGER_INACTIVE 
   } jrd_7;
   struct {
          TEXT  jrd_2 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_3 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_4 [12];	// RDB$CONSTRAINT_TYPE 
          SSHORT jrd_5;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_6;	// RDB$SYSTEM_FLAG 
   } jrd_1;
   struct {
          TEXT  jrd_17 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_18;	// gds__utility 
          SSHORT jrd_19;	// RDB$TRIGGER_INACTIVE 
   } jrd_16;
   struct {
          TEXT  jrd_13 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_14 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_15 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_12;
   struct {
          TEXT  jrd_24 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_25;	// gds__utility 
          SSHORT jrd_26;	// RDB$TRIGGER_INACTIVE 
   } jrd_23;
   struct {
          TEXT  jrd_22 [32];	// RDB$RELATION_NAME 
   } jrd_21;
/**************************************
 *
 *	s e t u p _ t r i g g e r s
 *
 **************************************
 *
 * Functional description
 *
 *	Get the triggers in the right order, which appears
 *      to be system triggers first, then user triggers,
 *      then triggers that implement check constraints.
 *
 * BUG #8458: Check constraint triggers have to be loaded
 *	      (and hence executed) after the user-defined
 *	      triggers because user-defined triggers can modify
 *	      the values being inserted or updated so that
 *	      the end values stored in the database don't
 *	      fulfill the check constraint.
 *
 **************************************/
	if (!relation)
		return blob;

	Database* dbb = tdbb->getDatabase();


	/* system triggers */

	jrd_req* request_fmtx = CMP_find_request(tdbb, irq_format4, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request_fmtx)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME = relation->rel_name.c_str()
			AND TRG.RDB$SYSTEM_FLAG = 1
		SORTED BY TRG.RDB$TRIGGER_SEQUENCE*/
	{
	if (!request_fmtx)
	   request_fmtx = CMP_compile2 (tdbb, (UCHAR*) jrd_20, sizeof(jrd_20), true);
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_21.jrd_22, 32);
	EXE_start (tdbb, request_fmtx, dbb->dbb_sys_trans);
	EXE_send (tdbb, request_fmtx, 0, 32, (UCHAR*) &jrd_21);
	while (1)
	   {
	   EXE_receive (tdbb, request_fmtx, 1, 36, (UCHAR*) &jrd_23);
	   if (!jrd_23.jrd_25) break;

		if (!/*TRG.RDB$TRIGGER_INACTIVE*/
		     jrd_23.jrd_26)
			setup_trigger_details(tdbb, relation, blob, triggers, /*TRG.RDB$TRIGGER_NAME*/
									      jrd_23.jrd_24, null_view);
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_format4))
		REQUEST(irq_format4) = request_fmtx;


	/* user triggers */

	request_fmtx = CMP_find_request(tdbb, irq_format5, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request_fmtx)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME EQ relation->rel_name.c_str()
		AND (TRG.RDB$SYSTEM_FLAG = 0 OR TRG.RDB$SYSTEM_FLAG MISSING)
		AND (NOT ANY
			CHK IN RDB$CHECK_CONSTRAINTS CROSS
			RCN IN RDB$RELATION_CONSTRAINTS
			WITH TRG.RDB$TRIGGER_NAME EQ CHK.RDB$TRIGGER_NAME
			AND CHK.RDB$CONSTRAINT_NAME EQ RCN.RDB$CONSTRAINT_NAME
			AND (RCN.RDB$CONSTRAINT_TYPE EQ CHECK_CNSTRT
			    OR RCN.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY)
			)
		SORTED BY TRG.RDB$TRIGGER_SEQUENCE*/
	{
	if (!request_fmtx)
	   request_fmtx = CMP_compile2 (tdbb, (UCHAR*) jrd_11, sizeof(jrd_11), true);
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_12.jrd_13, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_12.jrd_14, 12);
	gds__vtov ((const char*) CHECK_CNSTRT, (char*) jrd_12.jrd_15, 12);
	EXE_start (tdbb, request_fmtx, dbb->dbb_sys_trans);
	EXE_send (tdbb, request_fmtx, 0, 56, (UCHAR*) &jrd_12);
	while (1)
	   {
	   EXE_receive (tdbb, request_fmtx, 1, 36, (UCHAR*) &jrd_16);
	   if (!jrd_16.jrd_18) break;

		if (!/*TRG.RDB$TRIGGER_INACTIVE*/
		     jrd_16.jrd_19)
			setup_trigger_details(tdbb, relation, blob, triggers, /*TRG.RDB$TRIGGER_NAME*/
									      jrd_16.jrd_17, null_view);
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_format5))
		REQUEST(irq_format5) = request_fmtx;


/* check constraint triggers.  We're looking for triggers that belong
   to the table and are system triggers (i.e. system flag in (3, 4, 5))
   or a user looking trigger that's involved in a check constraint */

	request_fmtx = CMP_find_request(tdbb, irq_format6, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request_fmtx)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME = relation->rel_name.c_str()
		AND (TRG.RDB$SYSTEM_FLAG BT fb_sysflag_check_constraint AND fb_sysflag_view_check
			OR ((TRG.RDB$SYSTEM_FLAG = 0 OR TRG.RDB$SYSTEM_FLAG MISSING)
				AND ANY
			CHK IN RDB$CHECK_CONSTRAINTS CROSS
			RCN IN RDB$RELATION_CONSTRAINTS
				WITH TRG.RDB$TRIGGER_NAME EQ CHK.RDB$TRIGGER_NAME
				AND CHK.RDB$CONSTRAINT_NAME EQ RCN.RDB$CONSTRAINT_NAME
				AND (RCN.RDB$CONSTRAINT_TYPE EQ CHECK_CNSTRT
				    OR RCN.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY)
		    	)
			)
		SORTED BY TRG.RDB$TRIGGER_SEQUENCE*/
	{
	if (!request_fmtx)
	   request_fmtx = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_1.jrd_2, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_1.jrd_3, 12);
	gds__vtov ((const char*) CHECK_CNSTRT, (char*) jrd_1.jrd_4, 12);
	jrd_1.jrd_5 = fb_sysflag_view_check;
	jrd_1.jrd_6 = fb_sysflag_check_constraint;
	EXE_start (tdbb, request_fmtx, dbb->dbb_sys_trans);
	EXE_send (tdbb, request_fmtx, 0, 60, (UCHAR*) &jrd_1);
	while (1)
	   {
	   EXE_receive (tdbb, request_fmtx, 1, 36, (UCHAR*) &jrd_7);
	   if (!jrd_7.jrd_9) break;

		if (!/*TRG.RDB$TRIGGER_INACTIVE*/
		     jrd_7.jrd_10)
			setup_trigger_details(tdbb, relation, blob, triggers, /*TRG.RDB$TRIGGER_NAME*/
									      jrd_7.jrd_8, null_view);
	/*END_FOR;*/
	   }
	}

	if (!REQUEST(irq_format6))
		REQUEST(irq_format6) = request_fmtx;

	return blob;
}


static void setup_trigger_details(thread_db* tdbb,
								  jrd_rel* relation,
								  blb* blob,
								  trig_vec** triggers,
								  const TEXT* trigger_name,
								  bool null_view)
{
/**************************************
 *
 *	s e t u p _ t r i g g e r _ d e t a i l s
 *
 **************************************
 *
 * Functional description
 *	Stuff trigger details in places.
 *
 * for a view, load the trigger temporarily --
 * this is inefficient since it will just be reloaded
 *  in MET_scan_relation () but it needs to be done
 *  in case the view would otherwise be non-updatable
 *
 **************************************/

	put_summary_record(tdbb, blob, RSR_trigger_name,
		(const UCHAR*) trigger_name, fb_utils::name_length(trigger_name));

	if (!null_view) {
		MET_load_trigger(tdbb, relation, trigger_name, triggers);
	}
}


static bool validate_text_type(thread_db* tdbb, const TemporaryField* tfb)
{
/**************************************
 *
 *	v a l i d a t e _ t e x t _ t y p e
 *
 **************************************
 *
 * Functional description
 *	Make sure the text type specified is implemented
 *
 **************************************/

	if ((DTYPE_IS_TEXT (tfb->tfb_desc.dsc_dtype) &&
		!INTL_defined_type(tdbb, tfb->tfb_desc.dsc_ttype())) ||
		(tfb->tfb_desc.dsc_dtype == dtype_blob && tfb->tfb_desc.dsc_sub_type == isc_blob_text &&
			!INTL_defined_type(tdbb, tfb->tfb_desc.dsc_blob_ttype())))
	{
		return false;
	}

	return true;
}


static string get_string(const dsc* desc)
{
/**************************************
 *
 *  g e t _ s t r i n g
 *
 **************************************
 *
 * Get string for a given descriptor.
 *
 **************************************/
	const char* str;
	VaryStr<MAXPATHLEN> temp;// Must hold largest metadata field or filename

	if (!desc)
	{
		return string();
	}

	// Find the actual length of the string, searching until the claimed
	// end of the string, or the terminating \0, whichever comes first.

	USHORT length = MOV_make_string(desc, ttype_metadata, &str, &temp, sizeof(temp));

	const char* p = str;
	const char* const q = str + length;
	while (p < q && *p)
	{
		++p;
	}

	// Trim trailing blanks (bug 3355)

	while (--p >= str && *p == ' ')
		;
	length = (p + 1) - str;

	return string(str, length);
}
