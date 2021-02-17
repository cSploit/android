/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
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
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
#include "../jrd/scl.h"
#include "../jrd/acl.h"
#include "../jrd/blb.h"
#include "../jrd/irq.h"
#include "../jrd/obj.h"
#include "../jrd/req.h"
#include "../jrd/tra.h"
#include "../common/gdsassert.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/grant_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/constants.h"
#include "fb_exception.h"
#include "../common/utils_proto.h"
#include "../common/classes/array.h"
#include "../common/config/config.h"
#include "../common/os/os_utils.h"
#include "../common/classes/ClumpletWriter.h"


const int UIC_BASE	= 10;

using namespace Jrd;
using namespace Firebird;

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [159] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_or, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_literal, blr_text2, 3,0, 6,0, 'P','U','B','L','I','C',
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 6,0, 
                           blr_parameter, 0, 3,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 4,0, 
                              blr_parameter, 0, 0,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 7,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_eql, 
                                 blr_fid, 0, 2,0, 
                                 blr_literal, blr_text2, 3,0, 1,0, 'M',
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_10 [82] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_quad, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 9,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_16 [100] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 6,0, 0, 
            blr_first, 
               blr_literal, blr_long, 0, 1,0,0,0,
            blr_boolean, 
               blr_eql, 
                  blr_fid, 0, 8,0, 
                  blr_literal, blr_text2, 3,0, 12,0, 'R','D','B','$','D','A','T','A','B','A','S','E',
            blr_end, 
         blr_send, 0, 
            blr_begin, 
               blr_assignment, 
                  blr_fid, 0, 13,0, 
                  blr_parameter2, 0, 0,0, 2,0, 
               blr_assignment, 
                  blr_literal, blr_long, 0, 1,0,0,0,
                  blr_parameter, 0, 1,0, 
               blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 1,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_21 [68] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 1,0, 0, 
            blr_end, 
         blr_send, 0, 
            blr_begin, 
               blr_assignment, 
                  blr_fid, 0, 2,0, 
                  blr_parameter2, 0, 0,0, 2,0, 
               blr_assignment, 
                  blr_literal, blr_long, 0, 1,0,0,0,
                  blr_parameter, 0, 1,0, 
               blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 1,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_26 [71] =
   {	// blr string 
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
               blr_rid, 31,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
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

   };	// end of blr string 
static const UCHAR	jrd_31 [82] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 31,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_37 [181] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 31,0, 0, 
               blr_rid, 18,0, 1, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_fid, 1, 4,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 7,0, 
                              blr_parameter, 0, 3,0, 
                           blr_and, 
                              blr_or, 
                                 blr_eql, 
                                    blr_fid, 1, 0,0, 
                                    blr_parameter, 0, 0,0, 
                                 blr_eql, 
                                    blr_fid, 1, 0,0, 
                                    blr_literal, blr_text2, 3,0, 6,0, 'P','U','B','L','I','C',
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 1, 6,0, 
                                    blr_parameter, 0, 2,0, 
                                 blr_eql, 
                                    blr_fid, 1, 2,0, 
                                    blr_literal, blr_text2, 3,0, 1,0, 'M',
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_47 [89] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_54 [95] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 14,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_missing, 
                        blr_fid, 0, 9,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_61 [95] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 26,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_missing, 
                        blr_fid, 0, 16,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_68 [89] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 42,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_75 [132] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 3,0, 0, 
               blr_rid, 5,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 0,0, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 14,0, 
                     blr_parameter2, 1, 1,0, 3,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_84 [161] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 7,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 4,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 2,0, 
                           blr_parameter, 0, 1,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 14,0, 
                     blr_parameter2, 1, 0,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 1, 9,0, 
                     blr_parameter2, 1, 1,0, 6,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 4,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 4,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_96 [135] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 4,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 14,0, 
                     blr_parameter2, 1, 0,0, 4,0, 
                  blr_assignment, 
                     blr_fid, 1, 9,0, 
                     blr_parameter2, 1, 1,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 3,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_106 [89] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 20,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_113 [89] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 30,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_120 [89] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 2,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 28,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_127 [89] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 29,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_134 [89] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 28,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 


static bool check_number(const UCHAR*, USHORT);
static bool check_user_group(thread_db* tdbb, const UCHAR*, USHORT);
static bool check_string(const UCHAR*, const Firebird::MetaName&);
static SecurityClass::flags_t compute_access(thread_db* tdbb, const SecurityClass*,
	const jrd_rel*,	SLONG, const Firebird::MetaName&);
static SecurityClass::flags_t walk_acl(thread_db* tdbb, const Acl&, const jrd_rel*,
	SLONG, const Firebird::MetaName&);


namespace
{
	struct P_NAMES
	{
		SecurityClass::flags_t p_names_priv;
		USHORT p_names_acl;
		const TEXT* p_names_string;
	};

	const P_NAMES p_names[] =
	{
		{ SCL_alter, priv_alter, "ALTER" },
		{ SCL_control, priv_control, "CONTROL" },
		{ SCL_drop, priv_drop, "DROP" },
		{ SCL_insert, priv_insert, "INSERT" },
		{ SCL_update, priv_update, "UPDATE" },
		{ SCL_delete, priv_delete, "DELETE" },
		{ SCL_select, priv_select, "SELECT" },
		{ SCL_references, priv_references, "REFERENCES" },
		{ SCL_execute, priv_execute, "EXECUTE" },
		{ SCL_usage, priv_usage, "USAGE" },
		{ 0, 0, "" }
	};


	// Database is never requested through CMP_post_access.
	const char* const object_str_database	= "DATABASE";
	//const char* const object_str_schema		= "SCHEMA";
	const char* const object_str_table		= "TABLE";
	const char* const object_str_package	= "PACKAGE";
	const char* const object_str_procedure	= "PROCEDURE";
	const char* const object_str_function	= "FUNCTION";
	const char* const object_str_column		= "COLUMN";
	const char* const object_str_charset	= "CHARACTER SET";
	const char* const object_str_collation	= "COLLATION";
	const char* const object_str_domain		= "DOMAIN";
	const char* const object_str_exception	= "EXCEPTION";
	const char* const object_str_generator	= "GENERATOR";


	struct SecObjectNamePriority
	{
		const char* name;
		SLONG num;
	};

	const SecObjectNamePriority priorities[] =
	{
		{object_str_database, SCL_object_database},
		//{object_str_schema, SCL_object_schema},
		{object_str_table, SCL_object_table},
		{object_str_package, SCL_object_package},
		{object_str_procedure, SCL_object_procedure},
		{object_str_function, SCL_object_function},
		{object_str_column, SCL_object_column},
		{object_str_charset, SCL_object_charset},
		{object_str_collation, SCL_object_collation},
		{object_str_domain, SCL_object_domain},
		{object_str_exception, SCL_object_exception},
		{object_str_generator, SCL_object_generator},
		{"", 0}
	};

	/* Unused, may be needed
	SLONG accTypeStrToNum(const char* name)
	{
		for (const SecObjectNamePriority* p = priorities; p->num != 0; ++p)
		{
			if (strcmp(p->name, name) == 0)
				return p->num;
		}
		fb_assert(false);
		return 0;
	}
	*/

	const char* accTypeNumToStr(const SLONG num)
	{
		for (const SecObjectNamePriority* p = priorities; p->num != 0; ++p)
		{
			if (p->num == num)
				return p->name;
		}
		fb_assert(false);
		return "<unknown object type>";
	}
} // anonymous namespace


void SCL_check_access(thread_db* tdbb,
					  const SecurityClass* s_class,
					  SLONG view_id,
					  SLONG obj_type,
					  const Firebird::MetaName& obj_name,
					  SecurityClass::flags_t mask,
					  SLONG type,
					  bool recursive,
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

	const Jrd::Attachment& attachment = *tdbb->getAttachment();

	// Allow the database owner to back up a database even if he does not have
	// read access to all the tables in the database

	if ((attachment.att_flags & ATT_gbak_attachment) && (mask & SCL_select))
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
		if ((view || obj_name.hasData()) &&
			 (compute_access(tdbb, s_class, view, obj_type, obj_name) & mask))
		{
			return;
		}
	}

	// Allow recursive procedure/function call

	if (recursive &&
		((type == SCL_object_procedure && obj_type == id_procedure) ||
		 (type == SCL_object_function && obj_type == id_function)) &&
		obj_name == name)
	{
		return;
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
		fb_assert(type == SCL_object_database);
		ERR_post(Arg::Gds(isc_no_priv) << Arg::Str(names->p_names_string) <<
										  Arg::Str(object_str_database) <<
										  Arg::Str(""));
	}
	else
	{
		fb_assert(type != SCL_object_database);
		const char* const typeAsStr = accTypeNumToStr(type);
		const Firebird::string fullName = r_name.hasData() ?
			r_name.c_str() + Firebird::string(".") + name.c_str() : name.c_str();
		ERR_post(Arg::Gds(isc_no_priv) << Arg::Str(names->p_names_string) <<
										  Arg::Str(typeAsStr) <<
										  Arg::Str(fullName));
	}
}


void SCL_check_charset(thread_db* tdbb, const MetaName& name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_138 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_139;	// gds__utility 
          SSHORT jrd_140;	// gds__null_flag 
   } jrd_137;
   struct {
          TEXT  jrd_136 [32];	// RDB$CHARACTER_SET_NAME 
   } jrd_135;
/**************************************
 *
 *	S C L _ c h e c k _ c h a r s e t
 *
 **************************************
 *
 * Functional description
 *	Given a character set name, check for a set of privileges.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	AutoCacheRequest request(tdbb, irq_cs_security, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		CS IN RDB$CHARACTER_SETS
		WITH CS.RDB$CHARACTER_SET_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_134, sizeof(jrd_134));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_135.jrd_136, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_135);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_137);
	   if (!jrd_137.jrd_139) break;
	{
		if (!/*CS.RDB$SECURITY_CLASS.NULL*/
		     jrd_137.jrd_140)
			s_class = SCL_get_class(tdbb, /*CS.RDB$SECURITY_CLASS*/
						      jrd_137.jrd_138);
	}
	/*END_FOR*/
	   }
	}

	SCL_check_access(tdbb, s_class, 0, 0, name, mask, SCL_object_charset, false, name);
}


void SCL_check_collation(thread_db* tdbb, const MetaName& name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_131 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_132;	// gds__utility 
          SSHORT jrd_133;	// gds__null_flag 
   } jrd_130;
   struct {
          TEXT  jrd_129 [32];	// RDB$COLLATION_NAME 
   } jrd_128;
/**************************************
 *
 *	S C L _ c h e c k _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Given a collation name, check for a set of privileges.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	AutoCacheRequest request(tdbb, irq_coll_security, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		COLL IN RDB$COLLATIONS
		WITH COLL.RDB$COLLATION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_127, sizeof(jrd_127));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_128.jrd_129, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_128);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_130);
	   if (!jrd_130.jrd_132) break;
	{
		if (!/*COLL.RDB$SECURITY_CLASS.NULL*/
		     jrd_130.jrd_133)
			s_class = SCL_get_class(tdbb, /*COLL.RDB$SECURITY_CLASS*/
						      jrd_130.jrd_131);
	}
	/*END_FOR*/
	   }
	}

	SCL_check_access(tdbb, s_class, 0, 0, name, mask, SCL_object_collation, false, name);
}


void SCL_check_domain(thread_db* tdbb, const MetaName& name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_124 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_125;	// gds__utility 
          SSHORT jrd_126;	// gds__null_flag 
   } jrd_123;
   struct {
          TEXT  jrd_122 [32];	// RDB$FIELD_NAME 
   } jrd_121;
/**************************************
 *
 *	S C L _ c h e c k _ d o m a i n
 *
 **************************************
 *
 * Functional description
 *	Given a domain name, check for a set of privileges.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	AutoCacheRequest request(tdbb, irq_gfld_security, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		FLD IN RDB$FIELDS
		WITH FLD.RDB$FIELD_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_120, sizeof(jrd_120));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_121.jrd_122, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_121);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_123);
	   if (!jrd_123.jrd_125) break;
	{
		if (!/*FLD.RDB$SECURITY_CLASS.NULL*/
		     jrd_123.jrd_126)
			s_class = SCL_get_class(tdbb, /*FLD.RDB$SECURITY_CLASS*/
						      jrd_123.jrd_124);
	}
	/*END_FOR*/
	   }
	}

	SCL_check_access(tdbb, s_class, 0, 0, name, mask, SCL_object_domain, false, name);
}


void SCL_check_exception(thread_db* tdbb, const MetaName& name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_117 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_118;	// gds__utility 
          SSHORT jrd_119;	// gds__null_flag 
   } jrd_116;
   struct {
          TEXT  jrd_115 [32];	// RDB$EXCEPTION_NAME 
   } jrd_114;
/**************************************
 *
 *	S C L _ c h e c k _ e x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *	Given an exception name, check for a set of privileges.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	AutoCacheRequest request(tdbb, irq_exc_security, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		XCP IN RDB$EXCEPTIONS
		WITH XCP.RDB$EXCEPTION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_113, sizeof(jrd_113));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_114.jrd_115, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_114);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_116);
	   if (!jrd_116.jrd_118) break;
	{
		if (!/*XCP.RDB$SECURITY_CLASS.NULL*/
		     jrd_116.jrd_119)
			s_class = SCL_get_class(tdbb, /*XCP.RDB$SECURITY_CLASS*/
						      jrd_116.jrd_117);
	}
	/*END_FOR*/
	   }
	}

	SCL_check_access(tdbb, s_class, 0, 0, name, mask, SCL_object_exception, false, name);
}


void SCL_check_generator(thread_db* tdbb, const MetaName& name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_110 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_111;	// gds__utility 
          SSHORT jrd_112;	// gds__null_flag 
   } jrd_109;
   struct {
          TEXT  jrd_108 [32];	// RDB$GENERATOR_NAME 
   } jrd_107;
/**************************************
 *
 *	S C L _ c h e c k _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *	Given a generator name, check for a set of privileges.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	AutoCacheRequest request(tdbb, irq_gen_security, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		GEN IN RDB$GENERATORS
		WITH GEN.RDB$GENERATOR_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_106, sizeof(jrd_106));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_107.jrd_108, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_107);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_109);
	   if (!jrd_109.jrd_111) break;
	{
		if (!/*GEN.RDB$SECURITY_CLASS.NULL*/
		     jrd_109.jrd_112)
			s_class = SCL_get_class(tdbb, /*GEN.RDB$SECURITY_CLASS*/
						      jrd_109.jrd_110);
	}
	/*END_FOR*/
	   }
	}

	SCL_check_access(tdbb, s_class, 0, 0, name, mask, SCL_object_generator, false, name);
}


void SCL_check_index(thread_db* tdbb, const Firebird::MetaName& index_name, UCHAR index_id,
	SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_80 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_81 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_82;	// gds__utility 
          SSHORT jrd_83;	// gds__null_flag 
   } jrd_79;
   struct {
          TEXT  jrd_77 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_78 [32];	// RDB$RELATION_NAME 
   } jrd_76;
   struct {
          TEXT  jrd_89 [32];	// RDB$DEFAULT_CLASS 
          TEXT  jrd_90 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_91 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_92 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_93;	// gds__utility 
          SSHORT jrd_94;	// gds__null_flag 
          SSHORT jrd_95;	// gds__null_flag 
   } jrd_88;
   struct {
          TEXT  jrd_86 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_87;	// RDB$INDEX_ID 
   } jrd_85;
   struct {
          TEXT  jrd_100 [32];	// RDB$DEFAULT_CLASS 
          TEXT  jrd_101 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_102 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_103;	// gds__utility 
          SSHORT jrd_104;	// gds__null_flag 
          SSHORT jrd_105;	// gds__null_flag 
   } jrd_99;
   struct {
          TEXT  jrd_98 [32];	// RDB$INDEX_NAME 
   } jrd_97;
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
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	const SecurityClass* default_s_class = NULL;

	// No security to check for if the index is not yet created

    if ((index_name.length() == 0) && (index_id < 1)) {
        return;
    }

    Firebird::MetaName reln_name, aux_idx_name;
    const Firebird::MetaName* idx_name_ptr = &index_name;
	const Firebird::MetaName* relation_name_ptr = &index_name;

	AutoRequest request;

	// No need to cache this request handle, it's only used when
	// new constraints are created

    if (index_id < 1)
    {
        /*FOR(REQUEST_HANDLE request) IND IN RDB$INDICES
            CROSS REL IN RDB$RELATIONS
			OVER RDB$RELATION_NAME
			WITH IND.RDB$INDEX_NAME EQ index_name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_96, sizeof(jrd_96));
	gds__vtov ((const char*) index_name.c_str(), (char*) jrd_97.jrd_98, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_97);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 102, (UCHAR*) &jrd_99);
	   if (!jrd_99.jrd_103) break;
		{
            reln_name = /*REL.RDB$RELATION_NAME*/
			jrd_99.jrd_102;
		    if (!/*REL.RDB$SECURITY_CLASS.NULL*/
			 jrd_99.jrd_105)
                s_class = SCL_get_class(tdbb, /*REL.RDB$SECURITY_CLASS*/
					      jrd_99.jrd_101);
            if (!/*REL.RDB$DEFAULT_CLASS.NULL*/
		 jrd_99.jrd_104)
                default_s_class = SCL_get_class(tdbb, /*REL.RDB$DEFAULT_CLASS*/
						      jrd_99.jrd_100);
		}
        /*END_FOR*/
	   }
	}
    }
    else
    {
        idx_name_ptr = &aux_idx_name;
        /*FOR (REQUEST_HANDLE request) IND IN RDB$INDICES
            CROSS REL IN RDB$RELATIONS
            OVER RDB$RELATION_NAME
            WITH IND.RDB$RELATION_NAME EQ relation_name_ptr->c_str()
            AND IND.RDB$INDEX_ID EQ index_id*/
	{
	request.compile(tdbb, (UCHAR*) jrd_84, sizeof(jrd_84));
	gds__vtov ((const char*) relation_name_ptr->c_str(), (char*) jrd_85.jrd_86, 32);
	jrd_85.jrd_87 = index_id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_85);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 134, (UCHAR*) &jrd_88);
	   if (!jrd_88.jrd_93) break;
		{
            reln_name = /*REL.RDB$RELATION_NAME*/
			jrd_88.jrd_92;
            aux_idx_name = /*IND.RDB$INDEX_NAME*/
			   jrd_88.jrd_91;
            if (!/*REL.RDB$SECURITY_CLASS.NULL*/
		 jrd_88.jrd_95)
                s_class = SCL_get_class(tdbb, /*REL.RDB$SECURITY_CLASS*/
					      jrd_88.jrd_90);
            if (!/*REL.RDB$DEFAULT_CLASS.NULL*/
		 jrd_88.jrd_94)
                default_s_class = SCL_get_class(tdbb, /*REL.RDB$DEFAULT_CLASS*/
						      jrd_88.jrd_89);
        }
        /*END_FOR*/
	   }
	}
    }

	// Check if the relation exists. It may not have been created yet.
	// Just return in that case.

	if (reln_name.isEmpty())
		return;

	SCL_check_access(tdbb, s_class, 0, 0, NULL, mask, SCL_object_table, false, reln_name);

	request.reset();

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
	request.compile(tdbb, (UCHAR*) jrd_75, sizeof(jrd_75));
	gds__vtov ((const char*) idx_name_ptr->c_str(), (char*) jrd_76.jrd_77, 32);
	gds__vtov ((const char*) reln_name.c_str(), (char*) jrd_76.jrd_78, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_76);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 68, (UCHAR*) &jrd_79);
	   if (!jrd_79.jrd_82) break;
	{
		s_class = (!/*RF.RDB$SECURITY_CLASS.NULL*/
			    jrd_79.jrd_83) ?
			SCL_get_class(tdbb, /*RF.RDB$SECURITY_CLASS*/
					    jrd_79.jrd_81) : default_s_class;
		SCL_check_access(tdbb, s_class, 0, 0, NULL, mask,
						 SCL_object_column, false, /*RF.RDB$FIELD_NAME*/
									   jrd_79.jrd_80, reln_name);
	}
	/*END_FOR*/
	   }
	}
}


void SCL_check_package(thread_db* tdbb, const dsc* dsc_name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_72 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_73;	// gds__utility 
          SSHORT jrd_74;	// gds__null_flag 
   } jrd_71;
   struct {
          TEXT  jrd_70 [32];	// RDB$PACKAGE_NAME 
   } jrd_69;
/**************************************
 *
 *	S C L _ c h e c k _ p a c k a g e
 *
 **************************************
 *
 * Functional description
 *	Given a package name, check for a set of privileges.  The
 *	package in question may or may not have been created, let alone
 *	scanned.  This is used exclusively for meta-data operations.
 *
 **************************************/
	SET_TDBB(tdbb);

	// Get the name in CSTRING format, ending on NULL or SPACE
	fb_assert(dsc_name->dsc_dtype == dtype_text);
	const Firebird::MetaName name(reinterpret_cast<TEXT*>(dsc_name->dsc_address),
		dsc_name->dsc_length);

	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	AutoCacheRequest request(tdbb, irq_pkg_security, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		PKG IN RDB$PACKAGES
		WITH PKG.RDB$PACKAGE_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_68, sizeof(jrd_68));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_69.jrd_70, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_69);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_71);
	   if (!jrd_71.jrd_73) break;
	{
		if (!/*PKG.RDB$SECURITY_CLASS.NULL*/
		     jrd_71.jrd_74)
			s_class = SCL_get_class(tdbb, /*PKG.RDB$SECURITY_CLASS*/
						      jrd_71.jrd_72);
	}
	/*END_FOR*/
	   }
	}

	SCL_check_access(tdbb, s_class, 0, id_package, name, mask, SCL_object_package, false, name);
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

	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	AutoCacheRequest request(tdbb, irq_p_security, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		SPROC IN RDB$PROCEDURES
		WITH SPROC.RDB$PROCEDURE_NAME EQ name.c_str() AND
			 SPROC.RDB$PACKAGE_NAME MISSING*/
	{
	request.compile(tdbb, (UCHAR*) jrd_61, sizeof(jrd_61));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_62.jrd_63, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_62);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_64);
	   if (!jrd_64.jrd_66) break;
	{
		if (!/*SPROC.RDB$SECURITY_CLASS.NULL*/
		     jrd_64.jrd_67)
			s_class = SCL_get_class(tdbb, /*SPROC.RDB$SECURITY_CLASS*/
						      jrd_64.jrd_65);
	}
	/*END_FOR*/
	   }
	}

	SCL_check_access(tdbb, s_class, 0, id_procedure, name, mask, SCL_object_procedure, false, name);
}


void SCL_check_function(thread_db* tdbb, const dsc* dsc_name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_58 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_59;	// gds__utility 
          SSHORT jrd_60;	// gds__null_flag 
   } jrd_57;
   struct {
          TEXT  jrd_56 [32];	// RDB$FUNCTION_NAME 
   } jrd_55;
/**************************************
 *
 *	S C L _ c h e c k _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Given a function name, check for a set of privileges.  The
 *	function in question may or may not have been created, let alone
 *	scanned.  This is used exclusively for meta-data operations.
 *
 **************************************/
	SET_TDBB(tdbb);

	// Get the name in CSTRING format, ending on NULL or SPACE
	fb_assert(dsc_name->dsc_dtype == dtype_text);
	const Firebird::MetaName name(reinterpret_cast<TEXT*>(dsc_name->dsc_address),
							dsc_name->dsc_length);

	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	AutoCacheRequest request(tdbb, irq_f_security, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		SFUN IN RDB$FUNCTIONS
		WITH SFUN.RDB$FUNCTION_NAME EQ name.c_str() AND
			 SFUN.RDB$PACKAGE_NAME MISSING*/
	{
	request.compile(tdbb, (UCHAR*) jrd_54, sizeof(jrd_54));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_55.jrd_56, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_55);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_57);
	   if (!jrd_57.jrd_59) break;
	{
		if (!/*SFUN.RDB$SECURITY_CLASS.NULL*/
		     jrd_57.jrd_60)
			s_class = SCL_get_class(tdbb, /*SFUN.RDB$SECURITY_CLASS*/
						      jrd_57.jrd_58);
	}
	/*END_FOR*/
	   }
	}

	SCL_check_access(tdbb, s_class, 0, id_function, name, mask, SCL_object_function, false, name);
}


void SCL_check_relation(thread_db* tdbb, const dsc* dsc_name, SecurityClass::flags_t mask)
{
   struct {
          TEXT  jrd_51 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_52;	// gds__utility 
          SSHORT jrd_53;	// gds__null_flag 
   } jrd_50;
   struct {
          TEXT  jrd_49 [32];	// RDB$RELATION_NAME 
   } jrd_48;
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

	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const SecurityClass* s_class = NULL;
	AutoCacheRequest request(tdbb, irq_v_security, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request) REL IN RDB$RELATIONS
		WITH REL.RDB$RELATION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_47, sizeof(jrd_47));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_48.jrd_49, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_48);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_50);
	   if (!jrd_50.jrd_52) break;
	{
        if (!/*REL.RDB$SECURITY_CLASS.NULL*/
	     jrd_50.jrd_53)
			s_class = SCL_get_class(tdbb, /*REL.RDB$SECURITY_CLASS*/
						      jrd_50.jrd_51);
	}
	/*END_FOR*/
	   }
	}

	SCL_check_access(tdbb, s_class, 0, 0, NULL, mask, SCL_object_table, false, name);
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

	// Name may be absent or terminated with NULL or blank.  Clean up name.

	if (!par_string) {
		return NULL;
	}

	Firebird::MetaName string(par_string);

	//fb_utils::exact_name(string);

	if (string.isEmpty()) {
		return NULL;
	}

	Jrd::Attachment* const attachment = tdbb->getAttachment();

	// Look for the class already known

	SecurityClassList* list = attachment->att_security_classes;
	if (list && list->locate(string))
		return list->current();

	// Class isn't known. So make up a new security class block.

	MemoryPool& pool = *attachment->att_pool;

	SecurityClass* const s_class = FB_NEW(pool) SecurityClass(pool, string);
	s_class->scl_flags = compute_access(tdbb, s_class, NULL, 0, NULL);

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
	Jrd::Attachment* const attachment = tdbb->getAttachment();

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

	return access & (SCL_select | SCL_drop | SCL_control |
					 SCL_insert | SCL_update |
					 SCL_delete | SCL_alter | SCL_references |
					 SCL_execute | SCL_usage);
}


bool SCL_role_granted(thread_db* tdbb, const UserId& usr, const TEXT* sql_role)
{
   struct {
          TEXT  jrd_44 [32];	// RDB$USER 
          SSHORT jrd_45;	// gds__utility 
          SSHORT jrd_46;	// gds__null_flag 
   } jrd_43;
   struct {
          TEXT  jrd_39 [32];	// RDB$USER 
          TEXT  jrd_40 [32];	// RDB$ROLE_NAME 
          SSHORT jrd_41;	// RDB$USER_TYPE 
          SSHORT jrd_42;	// RDB$OBJECT_TYPE 
   } jrd_38;
/**************************************
 *
 *	S C L _ r o l e _ g r a n t e d
 *
 **************************************
 *
 * Functional description
 *	Check is sql_role granted to the user.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	if (!strcmp(sql_role, NULL_ROLE))
	{
		return true;
	}

	Firebird::string loginName(usr.usr_user_name);
	loginName.upper();
	const TEXT* login_name = loginName.c_str();

	bool found = false;

	AutoCacheRequest request(tdbb, irq_verify_role_name, IRQ_REQUESTS);

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
	request.compile(tdbb, (UCHAR*) jrd_37, sizeof(jrd_37));
	gds__vtov ((const char*) login_name, (char*) jrd_38.jrd_39, 32);
	gds__vtov ((const char*) sql_role, (char*) jrd_38.jrd_40, 32);
	jrd_38.jrd_41 = obj_user;
	jrd_38.jrd_42 = obj_sql_role;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_38);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_43);
	   if (!jrd_43.jrd_45) break;
	{
		if (!/*UU.RDB$USER.NULL*/
		     jrd_43.jrd_46)
			found = true;
	}
	/*END_FOR*/
	   }
	}

	return found;
}


bool SCL_admin_role(thread_db* tdbb, const TEXT* sql_role)
{
   struct {
          SSHORT jrd_35;	// gds__utility 
          SSHORT jrd_36;	// RDB$SYSTEM_FLAG 
   } jrd_34;
   struct {
          TEXT  jrd_33 [32];	// RDB$ROLE_NAME 
   } jrd_32;
/**************************************
 *
 *	S C L _ a d m i n _ r o l e
 *
 **************************************
 *
 * Functional description
 *	Check is sql_role is an admin role.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	bool adminRole = false;

	AutoCacheRequest request(tdbb, irq_is_admin_role, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request) R IN RDB$ROLES
		WITH R.RDB$ROLE_NAME EQ sql_role*/
	{
	request.compile(tdbb, (UCHAR*) jrd_31, sizeof(jrd_31));
	gds__vtov ((const char*) sql_role, (char*) jrd_32.jrd_33, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_32);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_34);
	   if (!jrd_34.jrd_35) break;
	{
		if (/*R.RDB$SYSTEM_FLAG*/
		    jrd_34.jrd_36 & ROLE_FLAG_DBO)
			adminRole = true;
	}
	/*END_FOR*/
	   }
	}

	return adminRole;
}


void SCL_init(thread_db* tdbb, bool create, const UserId& tempId)
{
   struct {
          TEXT  jrd_18 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_19;	// gds__utility 
          SSHORT jrd_20;	// gds__null_flag 
   } jrd_17;
   struct {
          TEXT  jrd_23 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_24;	// gds__utility 
          SSHORT jrd_25;	// gds__null_flag 
   } jrd_22;
   struct {
          SSHORT jrd_30;	// gds__utility 
   } jrd_29;
   struct {
          TEXT  jrd_28 [32];	// RDB$ROLE_NAME 
   } jrd_27;
/**************************************
 *
 *	S C L _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Check database access control list.
 *
 *	Finally fills UserId information
 *	(role, flags, etc.).
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	const TEXT* sql_role = tempId.usr_sql_role_name.nullStr();

    // CVC: We'll verify the role and wipe it out when it doesn't exist

	if (tempId.usr_user_name.hasData())
	{
		if (!create)
		{
			Firebird::string loginName(tempId.usr_user_name);
			loginName.upper();
			const TEXT* login_name = loginName.c_str();

			AutoCacheRequest request(tdbb, irq_get_role_name, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE request) X IN RDB$ROLES
				WITH X.RDB$ROLE_NAME EQ login_name*/
			{
			request.compile(tdbb, (UCHAR*) jrd_26, sizeof(jrd_26));
			gds__vtov ((const char*) login_name, (char*) jrd_27.jrd_28, 32);
			EXE_start (tdbb, request, attachment->getSysTransaction());
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_27);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_29);
			   if (!jrd_29.jrd_30) break;
			{
				ERR_post(Arg::Gds(isc_login_same_as_role_name) << Arg::Str(login_name));
			}
			/*END_FOR*/
			   }
			}
		}
	}

    // CVC: If we aren't creating a db and sql_role was specified,
    // then verify it against rdb$roles and rdb$user_privileges

    if (!create && sql_role && *sql_role)
    {
        if (!SCL_role_granted(tdbb, tempId, sql_role))
            sql_role = NULL;
    }

	if (!sql_role)
		sql_role = tempId.usr_trusted_role.nullStr();

	MetaName role_name(sql_role ? sql_role : NULL_ROLE);

	MemoryPool& pool = *attachment->att_pool;
	UserId* const user = FB_NEW(pool) UserId(pool, tempId);
	user->usr_sql_role_name = role_name.c_str();
	attachment->att_user = user;

	if (!create)
	{
		AutoCacheRequest request(tdbb, irq_get_att_class, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request) X IN RDB$DATABASE*/
		{
		request.compile(tdbb, (UCHAR*) jrd_21, sizeof(jrd_21));
		EXE_start (tdbb, request, attachment->getSysTransaction());
		while (1)
		   {
		   EXE_receive (tdbb, request, 0, 36, (UCHAR*) &jrd_22);
		   if (!jrd_22.jrd_24) break;
		{
			if (!/*X.RDB$SECURITY_CLASS.NULL*/
			     jrd_22.jrd_25)
				attachment->att_security_class = SCL_get_class(tdbb, /*X.RDB$SECURITY_CLASS*/
										     jrd_22.jrd_23);
		}
		/*END_FOR*/
		   }
		}

		if (dbb->dbb_owner.isEmpty())
		{
			AutoRequest request2;

			/*FOR(REQUEST_HANDLE request2)
				FIRST 1 REL IN RDB$RELATIONS
				WITH REL.RDB$RELATION_NAME EQ "RDB$DATABASE"*/
			{
			request2.compile(tdbb, (UCHAR*) jrd_16, sizeof(jrd_16));
			EXE_start (tdbb, request2, attachment->getSysTransaction());
			while (1)
			   {
			   EXE_receive (tdbb, request2, 0, 36, (UCHAR*) &jrd_17);
			   if (!jrd_17.jrd_19) break;
			{
	            if (!/*REL.RDB$OWNER_NAME.NULL*/
			 jrd_17.jrd_20)
					dbb->dbb_owner = /*REL.RDB$OWNER_NAME*/
							 jrd_17.jrd_18;
			}
			/*END_FOR*/
			   }
			}
		}

		if (dbb->dbb_owner == user->usr_user_name)
			user->usr_flags |= USR_owner;

		if (sql_role && SCL_admin_role(tdbb, role_name.c_str()))
			user->usr_flags |= USR_dba;
	}
	else
	{
		dbb->dbb_owner = user->usr_user_name;
		user->usr_flags |= USR_owner;
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

	acl.push(ACL_end);
	acl.push(ACL_priv_list);

	for (const P_NAMES* priv = p_names; priv->p_names_priv; priv++)
	{
		if (mask & priv->p_names_priv)
		{
			fb_assert(priv->p_names_acl <= MAX_UCHAR);
			acl.push(priv->p_names_acl);
		}
	}

	acl.push(0);
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

	s_class->scl_flags = compute_access(tdbb, s_class, NULL, 0, NULL);

	if (s_class->scl_flags & SCL_exists) {
		return s_class;
	}

	// Class no long exists - get rid of it!

	const Firebird::MetaName m_string(string);
	SecurityClassList* list = tdbb->getAttachment()->att_security_classes;
	if (list && list->locate(m_string))
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
											 SLONG obj_type,
											 const Firebird::MetaName& obj_name)
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
	Jrd::Attachment* const attachment = tdbb->getAttachment();
	jrd_tra* sysTransaction = attachment->getSysTransaction();

	SecurityClass::flags_t privileges = 0;

	AutoCacheRequest request(tdbb, irq_l_security, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request) X IN RDB$SECURITY_CLASSES
		WITH X.RDB$SECURITY_CLASS EQ s_class->scl_name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_10, sizeof(jrd_10));
	gds__vtov ((const char*) s_class->scl_name.c_str(), (char*) jrd_11.jrd_12, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_11);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 10, (UCHAR*) &jrd_13);
	   if (!jrd_13.jrd_15) break;
	{
		privileges |= SCL_exists;
		blb* blob = blb::open(tdbb, sysTransaction, &/*X.RDB$ACL*/
							     jrd_13.jrd_14);
		UCHAR* buffer = acl.getBuffer(ACL_BLOB_BUFFER_SIZE);
		UCHAR* end = buffer;
		while (true)
		{
			end += blob->BLB_get_segment(tdbb, end, (USHORT) (acl.getCount() - (end - buffer)) );
			if (blob->blb_flags & BLB_eof)
				break;

			// There was not enough space, realloc point acl to the correct location

			if (blob->getFragmentSize())
			{
				const ptrdiff_t old_offset = end - buffer;
				buffer = acl.getBuffer(acl.getCount() + ACL_BLOB_BUFFER_SIZE);
				end = buffer + old_offset;
			}
		}
		blob->BLB_close(tdbb);
		blob = NULL;
		acl.shrink(end - buffer);

		if (acl.getCount() > 0)
			privileges |= walk_acl(tdbb, acl, view, obj_type, obj_name);
	}
	/*END_FOR*/
	   }
	}

	return privileges;
}


static SecurityClass::flags_t walk_acl(thread_db* tdbb,
									   const Acl& acl,
									   const jrd_rel* view,
									   SLONG obj_type,
									   const Firebird::MetaName& obj_name)
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
	Jrd::Attachment* const attachment = tdbb->getAttachment();

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
					else if (user.usr_sql_role_name == user.usr_trusted_role)
						hit = true;
					else
					{
						TEXT login_name[129];
						fb_assert(user.usr_user_name.length() < sizeof(login_name));
						TEXT* pln = login_name;
						const TEXT* q = user.usr_user_name.c_str();
						while (*pln++ = UPPER7(*q)) {
							++q;
						}
						hit = false;
						AutoCacheRequest request(tdbb, irq_get_role_mem, IRQ_REQUESTS);

						/*FOR(REQUEST_HANDLE request) U IN RDB$USER_PRIVILEGES WITH
							(U.RDB$USER EQ login_name OR
							 U.RDB$USER EQ "PUBLIC") AND
								U.RDB$USER_TYPE EQ obj_user AND
								U.RDB$RELATION_NAME EQ user.usr_sql_role_name.c_str() AND
								U.RDB$OBJECT_TYPE EQ obj_sql_role AND
								U.RDB$PRIVILEGE EQ "M"*/
						{
						request.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
						gds__vtov ((const char*) user.usr_sql_role_name.c_str(), (char*) jrd_1.jrd_2, 32);
						gds__vtov ((const char*) login_name, (char*) jrd_1.jrd_3, 32);
						jrd_1.jrd_4 = obj_sql_role;
						jrd_1.jrd_5 = obj_user;
						EXE_start (tdbb, request, attachment->getSysTransaction());
						EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_1);
						while (1)
						   {
						   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_6);
						   if (!jrd_6.jrd_8) break;
						{
							if (!/*U.RDB$USER.NULL*/
							     jrd_6.jrd_9)
								hit = true;
						}
						/*END_FOR*/
						   }
						}
					}
					break;

				case id_view:
					if (!view || check_string(a, view->rel_name))
						hit = false;
					break;

				case id_package:
				case id_procedure:
				case id_trigger:
				case id_function:
					if (c != obj_type || check_string(a, obj_name))
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

					case priv_select:
						// Note that SELECT access must imply REFERENCES
						// access for upward compatibility of existing
						// security classes
						privilege |= SCL_select | SCL_references;
						break;

					case priv_insert:
						privilege |= SCL_insert;
						break;

					case priv_delete:
						privilege |= SCL_delete;
						break;

					case priv_references:
						privilege |= SCL_references;
						break;

					case priv_update:
						privilege |= SCL_update;
						break;

					case priv_drop:
						privilege |= SCL_drop;
						break;

					case priv_alter:
						privilege |= SCL_alter;
						break;

					case priv_execute:
						privilege |= SCL_execute;
						break;

					case priv_usage:
						privilege |= SCL_usage;
						break;

					case priv_write:
						// unused, but supported for backward compatibility
						privilege |= SCL_insert | SCL_update | SCL_delete;
						break;

					case priv_grant:
						// unused
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

void Jrd::UserId::populateDpb(Firebird::ClumpletWriter& dpb)
{
	if (usr_auth_block.hasData())
		dpb.insertBytes(isc_dpb_auth_block, usr_auth_block.begin(), usr_auth_block.getCount());
	else
		dpb.insertString(isc_dpb_user_name, usr_user_name);
}
