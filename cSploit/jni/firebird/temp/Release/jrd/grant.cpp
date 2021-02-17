/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		grant.epp
 *	DESCRIPTION:	SQL Grant/Revoke Handler
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/scl.h"
#include "../jrd/acl.h"
#include "../jrd/irq.h"
#include "../jrd/blb.h"
#include "../jrd/btr.h"
#include "../jrd/req.h"
#include "../jrd/tra.h"
#include "../jrd/val.h"
#include "../jrd/met.h"
#include "../jrd/intl.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/grant_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/scl_proto.h"
#include "../common/utils_proto.h"
#include "../common/classes/array.h"
#include "../jrd/constants.h"

using namespace Jrd;

// privileges given to the owner of a relation

const SecurityClass::flags_t OWNER_PRIVS = SCL_control | SCL_drop | SCL_alter;

inline void CHECK_AND_MOVE(Acl& to, UCHAR from)
{
	to.add(from);
}

/*DATABASE DB = STATIC "yachts.lnk";*/
static const UCHAR	jrd_0 [42] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 2,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 9,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_4 [121] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_quad, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 0, 1, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 1,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_14 [71] =
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
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 14,0, 
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
static const UCHAR	jrd_19 [150] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 0,0, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 0, 1, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 1, 14,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_32 [252] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 9,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 0,0, 7,0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 5,0, 0, 
               blr_rid, 18,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 4,0, 
                           blr_fid, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 1, 5,0, 
                           blr_fid, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 7,0, 
                           blr_parameter, 0, 3,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 4,0, 
                              blr_parameter, 0, 1,0, 
                           blr_and, 
                              blr_not, 
                                 blr_missing, 
                                    blr_fid, 1, 5,0, 
                              blr_or, 
                                 blr_neq, 
                                    blr_fid, 1, 0,0, 
                                    blr_parameter, 0, 0,0, 
                                 blr_neq, 
                                    blr_fid, 1, 6,0, 
                                    blr_parameter, 0, 2,0, 
               blr_sort, 2, 
                  blr_ascending, 
                     blr_fid, 1, 5,0, 
                  blr_ascending, 
                     blr_fid, 1, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 14,0, 
                     blr_parameter2, 1, 2,0, 6,0, 
                  blr_assignment, 
                     blr_fid, 1, 5,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 1, 6,0, 
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 1, 2,0, 
                     blr_parameter, 1, 8,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 5,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_48 [197] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 0,0, 7,0, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_parameter, 0, 4,0, 
                        blr_and, 
                           blr_or, 
                              blr_neq, 
                                 blr_fid, 0, 0,0, 
                                 blr_literal, blr_text2, 3,0, 6,0, 'P','U','B','L','I','C',
                              blr_neq, 
                                 blr_fid, 0, 6,0, 
                                 blr_parameter, 0, 3,0, 
                           blr_and, 
                              blr_or, 
                                 blr_neq, 
                                    blr_fid, 0, 0,0, 
                                    blr_parameter, 0, 0,0, 
                                 blr_neq, 
                                    blr_fid, 0, 6,0, 
                                    blr_parameter, 0, 2,0, 
                              blr_missing, 
                                 blr_fid, 0, 5,0, 
               blr_sort, 2, 
                  blr_ascending, 
                     blr_fid, 0, 0,0, 
                  blr_ascending, 
                     blr_fid, 0, 6,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 3,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_60 [99] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 29,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 28,0, 
                     blr_parameter, 1, 1,0, 
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
static const UCHAR	jrd_67 [99] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 5,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 1,0, 
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
static const UCHAR	jrd_74 [99] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 1, 1,0, 
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
static const UCHAR	jrd_81 [99] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 10,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 1,0, 
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
static const UCHAR	jrd_88 [99] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 10,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 1,0, 
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
static const UCHAR	jrd_95 [105] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 17,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter, 1, 1,0, 
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
static const UCHAR	jrd_102 [105] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter, 1, 1,0, 
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
static const UCHAR	jrd_109 [99] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 1,0, 
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
static const UCHAR	jrd_116 [124] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 14,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
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
static const UCHAR	jrd_125 [133] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_cstring2, 0,0, 7,0, 
   blr_message, 0, 3,0, 
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
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_parameter, 0, 2,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 0,0, 
                              blr_literal, blr_text2, 3,0, 6,0, 'P','U','B','L','I','C',
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 6,0, 
                                 blr_parameter, 0, 1,0, 
                              blr_missing, 
                                 blr_fid, 0, 5,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_133 [135] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 0,0, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 0, 1, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 1, 14,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 


static void define_default_class(thread_db*, const TEXT*, Firebird::MetaName&, const Acl&,
							jrd_tra*);
static void finish_security_class(Acl&, SecurityClass::flags_t);
static void get_object_info(thread_db*, const TEXT*, SSHORT,
							Firebird::MetaName&, Firebird::MetaName&, Firebird::MetaName&, bool&);
static SecurityClass::flags_t get_public_privs(thread_db*, const TEXT*, SSHORT);
static void get_user_privs(thread_db*, Acl&, const TEXT*, SSHORT, const Firebird::MetaName&,
							SecurityClass::flags_t);
static void grant_user(Acl&, const Firebird::MetaName&, SSHORT, SecurityClass::flags_t);
static SecurityClass::flags_t save_field_privileges(thread_db*, Acl&, const TEXT*,
							const Firebird::MetaName&, SecurityClass::flags_t, jrd_tra*);
static void save_security_class(thread_db*, const Firebird::MetaName&, const Acl&, jrd_tra*);
static SecurityClass::flags_t trans_sql_priv(const TEXT*);
static SecurityClass::flags_t squeeze_acl(Acl&, const Firebird::MetaName&, SSHORT);
static bool check_string(const UCHAR*, const Firebird::MetaName&);


void GRANT_privileges(thread_db* tdbb, const Firebird::string& name, USHORT id, jrd_tra* transaction)
{
/**************************************
 *
 *	G R A N T _ p r i v i l e g e s
 *
 **************************************
 *
 * Functional description
 *	Compute access control list from SQL privileges.
 *	This calculation is tricky and involves interaction between
 *	the relation-level and field-level privileges.  Do not change
 *	the order of operations	lightly.
 *
 **************************************/
	SET_TDBB(tdbb);

	bool restrct = false;

	Firebird::MetaName s_class, owner, default_class;
	bool view; // unused after being retrieved.
	get_object_info(tdbb, name.c_str(), id, owner, s_class, default_class, view);

	if (s_class.length() == 0) {
		return;
	}

	// start the acl off by giving the owner all privileges
	Acl acl, default_acl;

	CHECK_AND_MOVE(acl, ACL_version);

	SecurityClass::flags_t priv = OWNER_PRIVS;

	switch (id)
	{
		case obj_relation:
			priv |= SCL_references;
		case obj_view:
			priv |= SCL_select | SCL_insert | SCL_update | SCL_delete;
			break;

		case obj_procedure:
		case obj_udf:
		case obj_package_header:
			priv |= SCL_execute;
			break;

		case obj_field:
		case obj_exception:
		case obj_generator:
		case obj_charset:
		case obj_collation:
			priv |= SCL_usage;
			break;

		default:
			break;
	}

	grant_user(acl, owner, obj_user, priv);

	// Pick up core privileges

	const SecurityClass::flags_t public_priv = get_public_privs(tdbb, name.c_str(), id);
	get_user_privs(tdbb, acl, name.c_str(), id, owner, public_priv);

	if (id == obj_relation)
	{
		// Now handle field-level privileges.  This might require adding
		// UPDATE privilege to the relation-level acl,  Therefore, save
		// off the relation acl because we need to add a default field
		// acl in that case.

		default_acl.assign(acl);

		const SecurityClass::flags_t aggregate_public =
			save_field_privileges(tdbb, acl, name.c_str(), owner, public_priv,
										  transaction);

		// finish off and store the security class for the relation

		finish_security_class(acl, aggregate_public);

		save_security_class(tdbb, s_class, acl, transaction);

		if (acl.getCount() != default_acl.getCount())	// relation privs were added?
			restrct = true;

		// if there have been privileges added at the relation level which
		// need to be restricted from other fields in the relation,
		// update the acl for them

		if (restrct)
		{
			finish_security_class(default_acl, public_priv);
			define_default_class(tdbb, name.c_str(), default_class, default_acl,
								 transaction);
		}
	}
	else
	{
		finish_security_class(acl, public_priv);
		save_security_class(tdbb, s_class, acl, transaction);
	}
}


static void define_default_class(thread_db* tdbb,
								 const TEXT* relation_name,
								 Firebird::MetaName& default_class,
								 const Acl& acl,
								 jrd_tra* transaction)
{
   struct {
          SSHORT jrd_144;	// gds__utility 
   } jrd_143;
   struct {
          TEXT  jrd_141 [32];	// RDB$DEFAULT_CLASS 
          SSHORT jrd_142;	// gds__null_flag 
   } jrd_140;
   struct {
          TEXT  jrd_137 [32];	// RDB$DEFAULT_CLASS 
          SSHORT jrd_138;	// gds__utility 
          SSHORT jrd_139;	// gds__null_flag 
   } jrd_136;
   struct {
          TEXT  jrd_135 [32];	// RDB$RELATION_NAME 
   } jrd_134;
/**************************************
 *
 *	d e f i n e _ d e f a u l t _ c l a s s
 *
 **************************************
 *
 * Functional description
 *	Update the default security class for fields
 *	which have not been specifically granted
 *	any privileges.  We must grant them all
 *	privileges which were specifically granted
 *	at the relation level, but none of the
 *	privileges we added at the relation level
 *	for the purpose of accessing other fields.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (default_class.length() == 0)
	{
		default_class.printf("%s%" SQUADFORMAT, DEFAULT_CLASS,
				DPM_gen_id(tdbb, MET_lookup_generator(tdbb, DEFAULT_CLASS), false, 1));

		AutoCacheRequest request(tdbb, irq_grant7, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			REL IN RDB$RELATIONS
				WITH REL.RDB$RELATION_NAME EQ relation_name*/
		{
		request.compile(tdbb, (UCHAR*) jrd_133, sizeof(jrd_133));
		gds__vtov ((const char*) relation_name, (char*) jrd_134.jrd_135, 32);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_134);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_136);
		   if (!jrd_136.jrd_138) break;
		{
            /*MODIFY REL USING*/
	    {
	    
				/*REL.RDB$DEFAULT_CLASS.NULL*/
				jrd_136.jrd_139 = FALSE;
				jrd_vtof(default_class.c_str(), /*REL.RDB$DEFAULT_CLASS*/
								jrd_136.jrd_137,
						 sizeof(/*REL.RDB$DEFAULT_CLASS*/
							jrd_136.jrd_137));
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_136.jrd_137, (char*) jrd_140.jrd_141, 32);
			jrd_140.jrd_142 = jrd_136.jrd_139;
			EXE_send (tdbb, request, 2, 34, (UCHAR*) &jrd_140);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_143);
		   }
		}
	}

	save_security_class(tdbb, default_class, acl, transaction);

	dsc desc;
	desc.dsc_dtype = dtype_text;
	desc.dsc_sub_type = 0;
	desc.dsc_scale = 0;
	desc.dsc_ttype() = ttype_metadata;
	desc.dsc_address = (UCHAR *) relation_name;
	desc.dsc_length = strlen(relation_name);
	DFW_post_work(transaction, dfw_scan_relation, &desc, 0);
}


static void finish_security_class(Acl& acl, SecurityClass::flags_t public_priv)
{
/**************************************
 *
 *	f i n i s h _ s e c u r i t y _ c l a s s
 *
 **************************************
 *
 * Functional description
 *	Finish off a security class, putting
 *	in a wildcard for any public privileges.
 *
 **************************************/
	if (public_priv)
	{
		CHECK_AND_MOVE(acl, ACL_id_list);
		SCL_move_priv(public_priv, acl);
	}

	CHECK_AND_MOVE(acl, ACL_end);
}


static SecurityClass::flags_t get_public_privs(thread_db* tdbb,
											   const TEXT* object_name,
											   SSHORT obj_type)
{
   struct {
          SSHORT jrd_131;	// gds__utility 
          TEXT  jrd_132 [7];	// RDB$PRIVILEGE 
   } jrd_130;
   struct {
          TEXT  jrd_127 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_128;	// RDB$USER_TYPE 
          SSHORT jrd_129;	// RDB$OBJECT_TYPE 
   } jrd_126;
/**************************************
 *
 *	g e t _ p u b l i c _ p r i v s
 *
 **************************************
 *
 * Functional description
 *	Get public privileges for a particular object.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	SecurityClass::flags_t public_priv = 0;

	AutoCacheRequest request(tdbb, irq_grant5, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		PRV IN RDB$USER_PRIVILEGES
			WITH PRV.RDB$RELATION_NAME EQ object_name AND
			PRV.RDB$OBJECT_TYPE EQ obj_type AND
			PRV.RDB$USER EQ "PUBLIC" AND
			PRV.RDB$USER_TYPE EQ obj_user AND
			PRV.RDB$FIELD_NAME MISSING*/
	{
	request.compile(tdbb, (UCHAR*) jrd_125, sizeof(jrd_125));
	gds__vtov ((const char*) object_name, (char*) jrd_126.jrd_127, 32);
	jrd_126.jrd_128 = obj_user;
	jrd_126.jrd_129 = obj_type;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 36, (UCHAR*) &jrd_126);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 9, (UCHAR*) &jrd_130);
	   if (!jrd_130.jrd_131) break;
	{
        public_priv |= trans_sql_priv(/*PRV.RDB$PRIVILEGE*/
				      jrd_130.jrd_132);
	}
	/*END_FOR*/
	   }
	}

	return public_priv;
}


static void get_object_info(thread_db* tdbb,
							const TEXT* object_name,
							SSHORT obj_type,
							Firebird::MetaName& owner,
							Firebird::MetaName& s_class,
							Firebird::MetaName& default_class,
							bool& view)
{
   struct {
          TEXT  jrd_64 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_65 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_66;	// gds__utility 
   } jrd_63;
   struct {
          TEXT  jrd_62 [32];	// RDB$FIELD_NAME 
   } jrd_61;
   struct {
          TEXT  jrd_71 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_72 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_73;	// gds__utility 
   } jrd_70;
   struct {
          TEXT  jrd_69 [32];	// RDB$GENERATOR_NAME 
   } jrd_68;
   struct {
          TEXT  jrd_78 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_79 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_80;	// gds__utility 
   } jrd_77;
   struct {
          TEXT  jrd_76 [32];	// RDB$EXCEPTION_NAME 
   } jrd_75;
   struct {
          TEXT  jrd_85 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_86 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_87;	// gds__utility 
   } jrd_84;
   struct {
          TEXT  jrd_83 [32];	// RDB$COLLATION_NAME 
   } jrd_82;
   struct {
          TEXT  jrd_92 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_93 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_94;	// gds__utility 
   } jrd_91;
   struct {
          TEXT  jrd_90 [32];	// RDB$CHARACTER_SET_NAME 
   } jrd_89;
   struct {
          TEXT  jrd_99 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_100 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_101;	// gds__utility 
   } jrd_98;
   struct {
          TEXT  jrd_97 [32];	// RDB$FUNCTION_NAME 
   } jrd_96;
   struct {
          TEXT  jrd_106 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_107 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_108;	// gds__utility 
   } jrd_105;
   struct {
          TEXT  jrd_104 [32];	// RDB$PROCEDURE_NAME 
   } jrd_103;
   struct {
          TEXT  jrd_113 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_114 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_115;	// gds__utility 
   } jrd_112;
   struct {
          TEXT  jrd_111 [32];	// RDB$PACKAGE_NAME 
   } jrd_110;
   struct {
          bid  jrd_120;	// RDB$VIEW_BLR 
          TEXT  jrd_121 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_122 [32];	// RDB$DEFAULT_CLASS 
          TEXT  jrd_123 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_124;	// gds__utility 
   } jrd_119;
   struct {
          TEXT  jrd_118 [32];	// RDB$RELATION_NAME 
   } jrd_117;
/**************************************
 *
 *	g e t _ o b j e c t _ i n f o
 *
 **************************************
 *
 * Functional description
 *	This could be done in MET_scan_relation () or MET_lookup_procedure,
 *	but presumably we wish to make sure the information we have is
 *	up-to-the-minute.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	owner = s_class = default_class = "";
	view = false;

	if (obj_type == obj_relation)
	{
		AutoCacheRequest request(tdbb, irq_grant1, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			REL IN RDB$RELATIONS WITH
				REL.RDB$RELATION_NAME EQ object_name*/
		{
		request.compile(tdbb, (UCHAR*) jrd_116, sizeof(jrd_116));
		gds__vtov ((const char*) object_name, (char*) jrd_117.jrd_118, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_117);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 106, (UCHAR*) &jrd_119);
		   if (!jrd_119.jrd_124) break;
		{
			s_class = /*REL.RDB$SECURITY_CLASS*/
				  jrd_119.jrd_123;
			default_class = /*REL.RDB$DEFAULT_CLASS*/
					jrd_119.jrd_122;
			owner = /*REL.RDB$OWNER_NAME*/
				jrd_119.jrd_121;
			view = !/*REL.RDB$VIEW_BLR*/
				jrd_119.jrd_120.isEmpty();
		}
		/*END_FOR*/
		   }
		}
	}
	else if (obj_type == obj_package_header)
	{
		AutoCacheRequest request(tdbb, irq_grant10, IRQ_REQUESTS);

		/*FOR (REQUEST_HANDLE request)
			PKG IN RDB$PACKAGES
			WITH PKG.RDB$PACKAGE_NAME EQ object_name*/
		{
		request.compile(tdbb, (UCHAR*) jrd_109, sizeof(jrd_109));
		gds__vtov ((const char*) object_name, (char*) jrd_110.jrd_111, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_110);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_112);
		   if (!jrd_112.jrd_115) break;
		{
			s_class = /*PKG.RDB$SECURITY_CLASS*/
				  jrd_112.jrd_114;
			default_class = "";
			owner = /*PKG.RDB$OWNER_NAME*/
				jrd_112.jrd_113;
			view = false;
		}
		/*END_FOR*/
		   }
		}
	}
	else if (obj_type == obj_procedure)
	{
		AutoCacheRequest request(tdbb, irq_grant9, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			PRC IN RDB$PROCEDURES WITH
				PRC.RDB$PROCEDURE_NAME EQ object_name AND
				PRC.RDB$PACKAGE_NAME MISSING*/
		{
		request.compile(tdbb, (UCHAR*) jrd_102, sizeof(jrd_102));
		gds__vtov ((const char*) object_name, (char*) jrd_103.jrd_104, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_103);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_105);
		   if (!jrd_105.jrd_108) break;
		{
			s_class = /*PRC.RDB$SECURITY_CLASS*/
				  jrd_105.jrd_107;
			default_class = "";
			owner = /*PRC.RDB$OWNER_NAME*/
				jrd_105.jrd_106;
			view = false;
		}
		/*END_FOR*/
		   }
		}
	}
	else if (obj_type == obj_udf)
	{
		AutoCacheRequest request(tdbb, irq_grant11, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			FUN IN RDB$FUNCTIONS WITH
				FUN.RDB$FUNCTION_NAME EQ object_name AND
				FUN.RDB$PACKAGE_NAME MISSING*/
		{
		request.compile(tdbb, (UCHAR*) jrd_95, sizeof(jrd_95));
		gds__vtov ((const char*) object_name, (char*) jrd_96.jrd_97, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_96);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_98);
		   if (!jrd_98.jrd_101) break;
		{
			s_class = /*FUN.RDB$SECURITY_CLASS*/
				  jrd_98.jrd_100;
			default_class = "";
			owner = /*FUN.RDB$OWNER_NAME*/
				jrd_98.jrd_99;
			view = false;
		}
		/*END_FOR*/
		   }
		}
	}
	else if (obj_type == obj_charset)
	{
		AutoCacheRequest request(tdbb, irq_grant12, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			CS IN RDB$CHARACTER_SETS WITH
				CS.RDB$CHARACTER_SET_NAME EQ object_name*/
		{
		request.compile(tdbb, (UCHAR*) jrd_88, sizeof(jrd_88));
		gds__vtov ((const char*) object_name, (char*) jrd_89.jrd_90, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_89);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_91);
		   if (!jrd_91.jrd_94) break;
		{
			s_class = /*CS.RDB$SECURITY_CLASS*/
				  jrd_91.jrd_93;
			default_class = "";
			owner = /*CS.RDB$OWNER_NAME*/
				jrd_91.jrd_92;
			view = false;
		}
		/*END_FOR*/
		   }
		}
	}
	else if (obj_type == obj_collation)
	{
		AutoCacheRequest request(tdbb, irq_grant13, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			COLL IN RDB$COLLATIONS WITH
				COLL.RDB$COLLATION_NAME EQ object_name*/
		{
		request.compile(tdbb, (UCHAR*) jrd_81, sizeof(jrd_81));
		gds__vtov ((const char*) object_name, (char*) jrd_82.jrd_83, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_82);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_84);
		   if (!jrd_84.jrd_87) break;
		{
			s_class = /*COLL.RDB$SECURITY_CLASS*/
				  jrd_84.jrd_86;
			default_class = "";
			owner = /*COLL.RDB$OWNER_NAME*/
				jrd_84.jrd_85;
			view = false;
		}
		/*END_FOR*/
		   }
		}
	}
	else if (obj_type == obj_exception)
	{
		AutoCacheRequest request(tdbb, irq_grant14, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			XCP IN RDB$EXCEPTIONS WITH
				XCP.RDB$EXCEPTION_NAME EQ object_name*/
		{
		request.compile(tdbb, (UCHAR*) jrd_74, sizeof(jrd_74));
		gds__vtov ((const char*) object_name, (char*) jrd_75.jrd_76, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_75);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_77);
		   if (!jrd_77.jrd_80) break;
		{
			s_class = /*XCP.RDB$SECURITY_CLASS*/
				  jrd_77.jrd_79;
			default_class = "";
			owner = /*XCP.RDB$OWNER_NAME*/
				jrd_77.jrd_78;
			view = false;
		}
		/*END_FOR*/
		   }
		}
	}
	else if (obj_type == obj_generator)
	{
		AutoCacheRequest request(tdbb, irq_grant15, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			GEN IN RDB$GENERATORS WITH
				GEN.RDB$GENERATOR_NAME EQ object_name*/
		{
		request.compile(tdbb, (UCHAR*) jrd_67, sizeof(jrd_67));
		gds__vtov ((const char*) object_name, (char*) jrd_68.jrd_69, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_68);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_70);
		   if (!jrd_70.jrd_73) break;
		{
			s_class = /*GEN.RDB$SECURITY_CLASS*/
				  jrd_70.jrd_72;
			default_class = "";
			owner = /*GEN.RDB$OWNER_NAME*/
				jrd_70.jrd_71;
			view = false;
		}
		/*END_FOR*/
		   }
		}
	}
	else if (obj_type == obj_field)
	{
		AutoCacheRequest request(tdbb, irq_grant16, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			FLD IN RDB$FIELDS WITH
				FLD.RDB$FIELD_NAME EQ object_name*/
		{
		request.compile(tdbb, (UCHAR*) jrd_60, sizeof(jrd_60));
		gds__vtov ((const char*) object_name, (char*) jrd_61.jrd_62, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_61);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_63);
		   if (!jrd_63.jrd_66) break;
		{
			s_class = /*FLD.RDB$SECURITY_CLASS*/
				  jrd_63.jrd_65;
			default_class = "";
			owner = /*FLD.RDB$OWNER_NAME*/
				jrd_63.jrd_64;
			view = false;
		}
		/*END_FOR*/
		   }
		}
	}
}


static void get_user_privs(thread_db*					tdbb,
						   Acl&							acl,
						   const TEXT*					object_name,
						   SSHORT						obj_type,
						   const Firebird::MetaName&	owner,
						   SecurityClass::flags_t		public_priv)
{
   struct {
          TEXT  jrd_56 [32];	// RDB$USER 
          SSHORT jrd_57;	// gds__utility 
          SSHORT jrd_58;	// RDB$USER_TYPE 
          TEXT  jrd_59 [7];	// RDB$PRIVILEGE 
   } jrd_55;
   struct {
          TEXT  jrd_50 [32];	// RDB$USER 
          TEXT  jrd_51 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_52;	// RDB$USER_TYPE 
          SSHORT jrd_53;	// RDB$USER_TYPE 
          SSHORT jrd_54;	// RDB$OBJECT_TYPE 
   } jrd_49;
/**************************************
 *
 *	g e t _ u s e r _ p r i v s
 *
 **************************************
 *
 * Functional description
 *	Get privileges for a particular object.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	Firebird::MetaName user;
	SSHORT user_type = -2;
	SecurityClass::flags_t priv = 0;

	AutoCacheRequest request(tdbb, irq_grant2, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		PRV IN RDB$USER_PRIVILEGES
			WITH PRV.RDB$RELATION_NAME EQ object_name AND
			PRV.RDB$OBJECT_TYPE EQ obj_type AND
			(PRV.RDB$USER NE "PUBLIC" OR PRV.RDB$USER_TYPE NE obj_user) AND
			(PRV.RDB$USER NE owner.c_str() OR PRV.RDB$USER_TYPE NE obj_user) AND
			PRV.RDB$FIELD_NAME MISSING
			SORTED BY PRV.RDB$USER, PRV.RDB$USER_TYPE*/
	{
	request.compile(tdbb, (UCHAR*) jrd_48, sizeof(jrd_48));
	gds__vtov ((const char*) owner.c_str(), (char*) jrd_49.jrd_50, 32);
	gds__vtov ((const char*) object_name, (char*) jrd_49.jrd_51, 32);
	jrd_49.jrd_52 = obj_user;
	jrd_49.jrd_53 = obj_user;
	jrd_49.jrd_54 = obj_type;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 70, (UCHAR*) &jrd_49);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 43, (UCHAR*) &jrd_55);
	   if (!jrd_55.jrd_57) break;
	{
		fb_utils::exact_name_limit(/*PRV.RDB$USER*/
					   jrd_55.jrd_56, sizeof(/*PRV.RDB$USER*/
	 jrd_55.jrd_56));
		if (user != /*PRV.RDB$USER*/
			    jrd_55.jrd_56 || user_type != /*PRV.RDB$USER_TYPE*/
		 jrd_55.jrd_58)
		{
			if (user.length())
			{
				grant_user(acl, user, user_type, priv);
			}
			user_type = /*PRV.RDB$USER_TYPE*/
				    jrd_55.jrd_58;
			if (user_type == obj_user)
			{
				priv = public_priv;
			}
			else
			{
				priv = 0;
			}
			user = /*PRV.RDB$USER*/
			       jrd_55.jrd_56;
		}
		priv |= trans_sql_priv(/*PRV.RDB$PRIVILEGE*/
				       jrd_55.jrd_59);
	}
	/*END_FOR*/
	   }
	}

	if (user.length())
		grant_user(acl, user, user_type, priv);
}


static void grant_user(Acl& acl,
					   const Firebird::MetaName& user,
					   SSHORT user_type,
					   SecurityClass::flags_t privs)
{
/**************************************
 *
 *	g r a n t _ u s e r
 *
 **************************************
 *
 * Functional description
 *	Grant privileges to a particular user.
 *
 **************************************/
	CHECK_AND_MOVE(acl, ACL_id_list);
	switch (user_type)
	{
	case obj_user_group:
		CHECK_AND_MOVE(acl, id_group);
		break;

	case obj_sql_role:
		CHECK_AND_MOVE(acl, id_sql_role);
		break;

	case obj_user:
		CHECK_AND_MOVE(acl, id_person);
		break;

	case obj_package_header:
		CHECK_AND_MOVE(acl, id_package);
		break;

	case obj_procedure:
		CHECK_AND_MOVE(acl, id_procedure);
		break;

	case obj_udf:
		CHECK_AND_MOVE(acl, id_function);
		break;

	case obj_trigger:
		CHECK_AND_MOVE(acl, id_trigger);
		break;

	case obj_view:
		CHECK_AND_MOVE(acl, id_view);
		break;

	default:
		BUGCHECK(292);			// Illegal user_type

	}

	const UCHAR length = user.length();
	CHECK_AND_MOVE(acl, length);
	if (length) {
		acl.add(reinterpret_cast<const UCHAR*>(user.c_str()), length);
	}

	SCL_move_priv(privs, acl);
}


static SecurityClass::flags_t save_field_privileges(thread_db* tdbb,
													Acl& relation_acl,
													const TEXT* relation_name,
													const Firebird::MetaName& owner,
													SecurityClass::flags_t public_priv,
													jrd_tra* transaction)
{
   struct {
          SSHORT jrd_18;	// gds__utility 
   } jrd_17;
   struct {
          TEXT  jrd_16 [32];	// RDB$SECURITY_CLASS 
   } jrd_15;
   struct {
          SSHORT jrd_31;	// gds__utility 
   } jrd_30;
   struct {
          TEXT  jrd_28 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_29;	// gds__null_flag 
   } jrd_27;
   struct {
          TEXT  jrd_24 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_25;	// gds__utility 
          SSHORT jrd_26;	// gds__null_flag 
   } jrd_23;
   struct {
          TEXT  jrd_21 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_22 [32];	// RDB$RELATION_NAME 
   } jrd_20;
   struct {
          TEXT  jrd_39 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_40 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_41 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_42 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_43 [32];	// RDB$USER 
          SSHORT jrd_44;	// gds__utility 
          SSHORT jrd_45;	// gds__null_flag 
          SSHORT jrd_46;	// RDB$USER_TYPE 
          TEXT  jrd_47 [7];	// RDB$PRIVILEGE 
   } jrd_38;
   struct {
          TEXT  jrd_34 [32];	// RDB$USER 
          TEXT  jrd_35 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_36;	// RDB$USER_TYPE 
          SSHORT jrd_37;	// RDB$OBJECT_TYPE 
   } jrd_33;
/**************************************
 *
 *	s a v e _ f i e l d _ p r i v i l e g e s
 *
 **************************************
 *
 * Functional description
 *	Compute the privileges for all fields within a relation.
 *	All fields must be given the initial relation-level privileges.
 *	Conversely, field-level privileges must be added to the relation
 *	security class to be effective.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	Acl field_acl(relation_acl);
	const Acl acl_start(relation_acl);

	Firebird::MetaName field_name, user, s_class;
	SecurityClass::flags_t aggregate_public = public_priv;
	SecurityClass::flags_t priv = 0;
	SecurityClass::flags_t field_public = 0;
	SSHORT user_type = -1;

	AutoCacheRequest request(tdbb, irq_grant6, IRQ_REQUESTS);
	AutoRequest request2, request3;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FLD IN RDB$RELATION_FIELDS CROSS
			PRV IN RDB$USER_PRIVILEGES
			OVER RDB$RELATION_NAME, RDB$FIELD_NAME
			WITH PRV.RDB$OBJECT_TYPE EQ obj_relation AND
			PRV.RDB$RELATION_NAME EQ relation_name AND
			PRV.RDB$FIELD_NAME NOT MISSING AND
			(PRV.RDB$USER NE owner.c_str() OR PRV.RDB$USER_TYPE NE obj_user)
		SORTED BY PRV.RDB$FIELD_NAME, PRV.RDB$USER*/
	{
	request.compile(tdbb, (UCHAR*) jrd_32, sizeof(jrd_32));
	gds__vtov ((const char*) owner.c_str(), (char*) jrd_33.jrd_34, 32);
	gds__vtov ((const char*) relation_name, (char*) jrd_33.jrd_35, 32);
	jrd_33.jrd_36 = obj_user;
	jrd_33.jrd_37 = obj_relation;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_33);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 173, (UCHAR*) &jrd_38);
	   if (!jrd_38.jrd_44) break;
	{
		fb_utils::exact_name_limit(/*PRV.RDB$USER*/
					   jrd_38.jrd_43, sizeof(/*PRV.RDB$USER*/
	 jrd_38.jrd_43));
		fb_utils::exact_name_limit(/*PRV.RDB$FIELD_NAME*/
					   jrd_38.jrd_42, sizeof(/*PRV.RDB$FIELD_NAME*/
	 jrd_38.jrd_42));

		// create a control break on field_name,user

		if (user != /*PRV.RDB$USER*/
			    jrd_38.jrd_43 || field_name != /*PRV.RDB$FIELD_NAME*/
		  jrd_38.jrd_42)
		{
			// flush out information for old user

			if (user.length())
			{
				if (user != "PUBLIC")
				{
					const SecurityClass::flags_t field_priv =
						public_priv | priv | squeeze_acl(field_acl, user, user_type);
					grant_user(field_acl, user, user_type, field_priv);

					const SecurityClass::flags_t relation_priv =
						public_priv | priv | squeeze_acl(relation_acl, user, user_type);
					grant_user(relation_acl, user, user_type, relation_priv);
				}
				else
				{
					field_public = field_public | public_priv | priv;
				}
			}

			// initialize for new user

			priv = 0;
			user = /*PRV.RDB$USER*/
			       jrd_38.jrd_43;
			user_type = /*PRV.RDB$USER_TYPE*/
				    jrd_38.jrd_46;
		}

		// create a control break on field_name

		if (field_name != /*PRV.RDB$FIELD_NAME*/
				  jrd_38.jrd_42)
		{
			// finish off the last field, adding a wildcard at end, giving PUBLIC
			// all privileges available at the table level as well as those
			// granted at the field level

			if (field_name.length())
			{
				aggregate_public |= field_public;
				finish_security_class(field_acl, (field_public | public_priv));
				save_security_class(tdbb, s_class, field_acl, transaction);
			}

			// initialize for new field

			field_name = /*PRV.RDB$FIELD_NAME*/
				     jrd_38.jrd_42;
			s_class = /*FLD.RDB$SECURITY_CLASS*/
				  jrd_38.jrd_41;

			if (/*FLD.RDB$SECURITY_CLASS.NULL*/
			    jrd_38.jrd_45 || s_class.length() == 0)
			{
				bool unique = false;

				/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
					RFR IN RDB$RELATION_FIELDS WITH
						RFR.RDB$RELATION_NAME EQ FLD.RDB$RELATION_NAME
						AND RFR.RDB$FIELD_NAME EQ FLD.RDB$FIELD_NAME*/
				{
				request2.compile(tdbb, (UCHAR*) jrd_19, sizeof(jrd_19));
				gds__vtov ((const char*) jrd_38.jrd_39, (char*) jrd_20.jrd_21, 32);
				gds__vtov ((const char*) jrd_38.jrd_40, (char*) jrd_20.jrd_22, 32);
				EXE_start (tdbb, request2, transaction);
				EXE_send (tdbb, request2, 0, 64, (UCHAR*) &jrd_20);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 36, (UCHAR*) &jrd_23);
				   if (!jrd_23.jrd_25) break;
				{
					/*MODIFY RFR*/
					{
					
						while (!unique)
						{
							sprintf(/*RFR.RDB$SECURITY_CLASS*/
								jrd_23.jrd_24, "%s%" SQUADFORMAT, SQL_FLD_SECCLASS_PREFIX,
								DPM_gen_id(tdbb, MET_lookup_generator(tdbb, SQL_SECCLASS_GENERATOR),
										   false, 1));

							unique = true;
							/*FOR (REQUEST_HANDLE request3)
								RFR2 IN RDB$RELATION_FIELDS
								WITH RFR2.RDB$SECURITY_CLASS = RFR.RDB$SECURITY_CLASS*/
							{
							request3.compile(tdbb, (UCHAR*) jrd_14, sizeof(jrd_14));
							gds__vtov ((const char*) jrd_23.jrd_24, (char*) jrd_15.jrd_16, 32);
							EXE_start (tdbb, request3, attachment->getSysTransaction());
							EXE_send (tdbb, request3, 0, 32, (UCHAR*) &jrd_15);
							while (1)
							   {
							   EXE_receive (tdbb, request3, 1, 2, (UCHAR*) &jrd_17);
							   if (!jrd_17.jrd_18) break;
							{
								unique = false;
							}
							/*END_FOR*/
							   }
							}
						}

						/*RFR.RDB$SECURITY_CLASS.NULL*/
						jrd_23.jrd_26 = FALSE;
						s_class = /*RFR.RDB$SECURITY_CLASS*/
							  jrd_23.jrd_24;
					/*END_MODIFY*/
					gds__vtov((const char*) jrd_23.jrd_24, (char*) jrd_27.jrd_28, 32);
					jrd_27.jrd_29 = jrd_23.jrd_26;
					EXE_send (tdbb, request2, 2, 34, (UCHAR*) &jrd_27);
					}
				}
				/*END_FOR*/
				   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_30);
				   }
				}
			}

			field_public = 0;

			// restart a security class at the end of the relation-level privs
			field_acl.assign(acl_start);
		}

		priv |= trans_sql_priv(/*PRV.RDB$PRIVILEGE*/
				       jrd_38.jrd_47);
	}
	/*END_FOR*/
	   }
	}

	// flush out the last user's info

	if (user.length())
	{
		if (user != "PUBLIC")
		{
			const SecurityClass::flags_t field_priv =
				public_priv | priv | squeeze_acl(field_acl, user, user_type);
			grant_user(field_acl, user, user_type, field_priv);

			const SecurityClass::flags_t relation_priv =
				public_priv | priv | squeeze_acl(relation_acl, user, user_type);
			grant_user(relation_acl, user, user_type, relation_priv);
		}
		else
		{
			field_public = field_public | public_priv | priv;
		}
	}

	// flush out the last field's info, and schedule a format update

	if (field_name.length())
	{
		aggregate_public |= field_public;
		finish_security_class(field_acl, (field_public | public_priv));
		save_security_class(tdbb, s_class, field_acl, transaction);

		dsc desc;
		desc.dsc_dtype = dtype_text;
		desc.dsc_sub_type = 0;
		desc.dsc_scale = 0;
		desc.dsc_ttype() = ttype_metadata;
		desc.dsc_address = (UCHAR *) relation_name;
		desc.dsc_length = strlen(relation_name);
		DFW_post_work(transaction, dfw_update_format, &desc, 0);
	}

	return aggregate_public;
}


static void save_security_class(thread_db* tdbb,
								const Firebird::MetaName& s_class,
								const Acl& acl,
								jrd_tra* transaction)
{
   struct {
          bid  jrd_2;	// RDB$ACL 
          TEXT  jrd_3 [32];	// RDB$SECURITY_CLASS 
   } jrd_1;
   struct {
          SSHORT jrd_13;	// gds__utility 
   } jrd_12;
   struct {
          bid  jrd_11;	// RDB$ACL 
   } jrd_10;
   struct {
          bid  jrd_8;	// RDB$ACL 
          SSHORT jrd_9;	// gds__utility 
   } jrd_7;
   struct {
          TEXT  jrd_6 [32];	// RDB$SECURITY_CLASS 
   } jrd_5;
/**************************************
 *
 *	s a v e _ s e c u r i t y _ c l a s s
 *
 **************************************
 *
 * Functional description
 *	Store or update the named security class.
 *
 **************************************/
	SET_TDBB(tdbb);

	bid blob_id;
	blb* blob = blb::create(tdbb, transaction, &blob_id);
	size_t length = acl.getCount();
	const UCHAR* buffer = acl.begin();
	while (length)
	{
		const size_t step = length > ACL_BLOB_BUFFER_SIZE ? ACL_BLOB_BUFFER_SIZE : length;
		blob->BLB_put_segment(tdbb, buffer, static_cast<USHORT>(step));
		length -= step;
		buffer += step;
	}
	blob->BLB_close(tdbb);

	AutoCacheRequest request(tdbb, irq_grant3, IRQ_REQUESTS);

	bool found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		CLS IN RDB$SECURITY_CLASSES
			WITH CLS.RDB$SECURITY_CLASS EQ s_class.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_4, sizeof(jrd_4));
	gds__vtov ((const char*) s_class.c_str(), (char*) jrd_5.jrd_6, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_5);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 10, (UCHAR*) &jrd_7);
	   if (!jrd_7.jrd_9) break;
	{
        found = true;
		/*MODIFY CLS*/
		{
		
			/*CLS.RDB$ACL*/
			jrd_7.jrd_8 = blob_id;
		/*END_MODIFY*/
		jrd_10.jrd_11 = jrd_7.jrd_8;
		EXE_send (tdbb, request, 2, 8, (UCHAR*) &jrd_10);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_12);
	   }
	}

	if (!found)
	{
		request.reset(tdbb, irq_grant4, IRQ_REQUESTS);

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			CLS IN RDB$SECURITY_CLASSES*/
		{
		
		{
            jrd_vtof(s_class.c_str(), /*CLS.RDB$SECURITY_CLASS*/
				      jrd_1.jrd_3, sizeof(/*CLS.RDB$SECURITY_CLASS*/
	 jrd_1.jrd_3));
			/*CLS.RDB$ACL*/
			jrd_1.jrd_2 = blob_id;
		}
		/*END_STORE*/
		request.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 40, (UCHAR*) &jrd_1);
		}
	}
}


static SecurityClass::flags_t trans_sql_priv(const TEXT* privileges)
{
/**************************************
 *
 *	t r a n s _ s q l _ p r i v
 *
 **************************************
 *
 * Functional description
 *	Map a SQL privilege letter into an internal privilege bit.
 *
 **************************************/
	SecurityClass::flags_t priv = 0;

	switch (UPPER7(privileges[0]))
	{
	case 'S':
		priv |= SCL_select;
		break;
	case 'I':
		priv |= SCL_insert;
		break;
	case 'U':
		priv |= SCL_update;
		break;
	case 'D':
		priv |= SCL_delete;
		break;
	case 'R':
		priv |= SCL_references;
		break;
	case 'X':
		priv |= SCL_execute;
		break;
	case 'G':
		priv |= SCL_usage;
		break;
	}

	return priv;
}


static SecurityClass::flags_t squeeze_acl(Acl& acl, const Firebird::MetaName& user, SSHORT user_type)
{
/**************************************
 *
 *	s q u e e z e _ a c l
 *
 **************************************
 *
 * Functional description
 *	Walk an access control list looking for a hit.  If a hit
 *	is found, return privileges and squeeze out that acl-element.
 *	The caller will use the returned privilege to insert a new
 *	privilege for the input user.
 *
 **************************************/
	UCHAR* dup_acl = NULL;
	SecurityClass::flags_t privilege = 0;
	UCHAR c;

	// Make sure that this half-finished acl looks good enough to process.
	acl.push(0);

	UCHAR* a = acl.begin();

	if (*a++ != ACL_version)
		BUGCHECK(160);			// msg 160 wrong ACL version

	bool hit = false;

	while ( (c = *a++) )
		switch (c)
		{
		case ACL_id_list:
			dup_acl = a - 1;
			hit = true;
			while ( (c = *a++) )
			{
				switch (c)
				{
				case id_person:
					if (user_type != obj_user)
						hit = false;
					if (check_string(a, user))
						hit = false;
					break;

				case id_sql_role:
					if (user_type != obj_sql_role)
						hit = false;
					if (check_string(a, user))
						hit = false;
					break;

				case id_view:
					if (user_type != obj_view)
						hit = false;
					if (check_string(a, user))
						hit = false;
					break;

				case id_procedure:
					if (user_type != obj_procedure)
						hit = false;
					if (check_string(a, user))
						hit = false;
					break;

				case id_function:
					if (user_type != obj_udf)
						hit = false;
					if (check_string(a, user))
						hit = false;
					break;

				case id_trigger:
					if (user_type != obj_trigger)
						hit = false;
					if (check_string(a, user))
						hit = false;
					break;

				case id_project:
				case id_organization:
					hit = false;
					// CVC: What's the idea of calling a function whose only
					// result is boolean without checking it?
					check_string(a, user);
					break;

				case id_views:
					hit = false;
					break;

				case id_node:
				case id_user:
					{
						hit = false;
						// Seems strange with the same increment just after the switch.
						a += *a + 1;
					}
					break;

				case id_group:
					if (user_type != obj_user_group)
						hit = false;
					if (check_string(a, user))
						hit = false;
					break;

				default:
					BUGCHECK(293);	// bad ACL
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
						privilege |= SCL_select;
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
						BUGCHECK(293);	// bad ACL
					}
				}

				// Squeeze out duplicate acl element.
				fb_assert(dup_acl);
				acl.remove(dup_acl, a);
				a = dup_acl;
			}
			else
				while (*a++);
			break;

		default:
			BUGCHECK(293);		// bad ACL
		}

	// remove added extra '\0' byte
    acl.pop();

	return privilege;
}


static bool check_string(const UCHAR* acl, const Firebird::MetaName& name)
{
/**************************************
 *
 *      c h e c k _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *      Check a string against an acl string.  If they don't match,
 *      return true.
 *
 **************************************/
	// JPN: Since Kanji User names are not allowed, No need to fix this UPPER loop.

	USHORT l = *acl++;
	const TEXT* string = name.c_str();
	if (l)
	{
		do
		{
			const UCHAR c1 = *acl++;
			const TEXT c2 = *string++;
			if (UPPER7(c1) != UPPER7(c2))
			{
				return true;
			}
		} while (--l);
	}

	return (*string && *string != ' ') ? true : false;
}
