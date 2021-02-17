/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
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
#include "../dsql/StmtNodes.h"
#include "../intl/charsets.h"
#include "../common/gdsassert.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../common/dsc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/flu_proto.h"
#include "../yvalve/gds_proto.h"
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

#include "../jrd/PreparedStatement.h"
#include "../jrd/RecordSourceNodes.h"
#include "../jrd/DebugInterface.h"
#include "../common/classes/MsgPrint.h"
#include "../jrd/Function.h"


#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

// Pick up relation ids
#include "../jrd/ini.h"

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [65] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_long, 0, 
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
                  blr_fid, 0, 4,0, 
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
static const UCHAR	jrd_5 [113] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 24,0, 0, 
               blr_rid, 23,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 1, 0,0, 
                        blr_fid, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 4,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 3,0, 
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
static const UCHAR	jrd_12 [103] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 8,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 13,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 5,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 6,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_22 [144] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 13,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 4,0, 
                              blr_parameter, 0, 4,0, 
                           blr_and, 
                              blr_missing, 
                                 blr_fid, 0, 2,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 0, 3,0, 
                                    blr_parameter, 0, 3,0, 
                                 blr_equiv, 
                                    blr_fid, 0, 5,0, 
                                    blr_value_if, 
                                       blr_eql, 
                                          blr_parameter, 0, 0,0, 
                                          blr_literal, blr_text2, 3,0, 0,0, 
                                       blr_null, 
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
static const UCHAR	jrd_31 [125] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 13,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 4,0, 
                              blr_parameter, 0, 4,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 2,0, 
                                 blr_parameter, 0, 0,0, 
                              blr_eql, 
                                 blr_fid, 0, 3,0, 
                                 blr_parameter, 0, 3,0, 
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
static const UCHAR	jrd_40 [156] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
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
               blr_rid, 4,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 3,0, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 1, 8,0, 
                           blr_fid, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 1, 2,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 1, 6,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 5,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_50 [185] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 4,0, 0, 
               blr_rid, 22,0, 1, 
               blr_rid, 4,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 5,0, 
                        blr_fid, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 1,0, 
                              blr_parameter, 0, 0,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 2, 0,0, 
                                 blr_fid, 0, 8,0, 
                              blr_eql, 
                                 blr_fid, 2, 3,0, 
                                 blr_literal, blr_long, 0, 1,0,0,0,
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 2, 1,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 2, 2,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 2, 6,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 5,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_61 [178] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 28,0, 0, 
               blr_rid, 29,0, 1, 
               blr_rid, 11,0, 2, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 2,0, 
                        blr_fid, 0, 4,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 2, 0,0, 
                           blr_literal, blr_text2, 3,0, 22,0, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E',
                        blr_and, 
                           blr_eql, 
                              blr_fid, 2, 2,0, 
                              blr_parameter, 0, 1,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 1, 0,0, 
                                 blr_parameter, 0, 0,0, 
                              blr_eql, 
                                 blr_fid, 2, 1,0, 
                                 blr_fid, 0, 4,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_69 [101] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 29,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
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
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_76 [90] =
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
               blr_rid, 28,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
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
                     blr_fid, 0, 4,0, 
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
static const UCHAR	jrd_82 [132] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 7,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter2, 1, 4,0, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 5,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_92 [105] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 11,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_99 [128] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 4, 1,0, 
      blr_short, 0, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_long, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 19,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
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
                        blr_receive, 4, 
                           blr_erase, 0, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_111 [118] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 10,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
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
                                    blr_fid, 1, 4,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_121 [97] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 0,0, 0,4, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 17,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_128 [205] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 14,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 14,0, 
                     blr_parameter2, 1, 0,0, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter2, 1, 6,0, 11,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter2, 1, 10,0, 9,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter, 1, 12,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 13,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 7,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_146 [128] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 7,0, 
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
                           blr_fid, 0, 2,0, 
                           blr_parameter, 0, 2,0, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_erase, 0, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_157 [109] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 7,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 18,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 2,0, 
                           blr_parameter, 0, 2,0, 
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
static const UCHAR	jrd_164 [133] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 3,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 26,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_not, 
                        blr_missing, 
                           blr_fid, 0, 6,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter2, 1, 2,0, 1,0, 
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
                                    blr_parameter2, 2, 1,0, 0,0, 
                                    blr_fid, 1, 12,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_176 [322] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 23,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 27,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 0,0, 
                        blr_fid, 0, 4,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 1,0, 
                        blr_equiv, 
                           blr_fid, 0, 14,0, 
                           blr_value_if, 
                              blr_eql, 
                                 blr_parameter, 0, 0,0, 
                                 blr_literal, blr_text2, 3,0, 0,0, 
                              blr_null, 
                              blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter2, 1, 1,0, 13,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 1, 6,0, 
                     blr_parameter2, 1, 3,0, 6,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter2, 1, 4,0, 20,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 1, 25,0, 
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 1, 26,0, 
                     blr_parameter, 1, 8,0, 
                  blr_assignment, 
                     blr_fid, 1, 11,0, 
                     blr_parameter, 1, 9,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter, 1, 10,0, 
                  blr_assignment, 
                     blr_fid, 1, 9,0, 
                     blr_parameter, 1, 11,0, 
                  blr_assignment, 
                     blr_fid, 1, 10,0, 
                     blr_parameter, 1, 12,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter2, 1, 15,0, 14,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter2, 1, 17,0, 16,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 18,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 19,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter2, 1, 22,0, 21,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 5,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_204 [89] =
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
static const UCHAR	jrd_211 [262] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 23,0, 
      blr_quad, 0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 26,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter2, 1, 0,0, 11,0, 
                  blr_assignment, 
                     blr_fid, 0, 15,0, 
                     blr_parameter2, 1, 1,0, 12,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter2, 1, 2,0, 13,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter2, 1, 3,0, 14,0, 
                  blr_assignment, 
                     blr_fid, 0, 14,0, 
                     blr_parameter2, 1, 4,0, 17,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter2, 1, 5,0, 20,0, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter2, 1, 6,0, 22,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter2, 1, 10,0, 9,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter2, 1, 16,0, 15,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 18,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 19,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 21,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 8,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_238 [50] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_quad, 0, 
      blr_long, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 19,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_243 [131] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_int64, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 12,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 8,0, 
                        blr_literal, blr_long, 0, 1,0,0,0,
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 4,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_252 [119] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter2, 1, 3,0, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 15,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 5,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_262 [108] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
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
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter2, 1, 2,0, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 15,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 4,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_271 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 26,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_277 [110] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
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
                        blr_parameter, 0, 1,0, 
                     blr_equiv, 
                        blr_fid, 0, 16,0, 
                        blr_value_if, 
                           blr_eql, 
                              blr_parameter, 0, 0,0, 
                              blr_literal, blr_text2, 3,0, 0,0, 
                           blr_null, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_284 [180] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 4,0, 0, 
               blr_rid, 4,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_or, 
                           blr_eql, 
                              blr_fid, 0, 2,0, 
                              blr_add, 
                                 blr_parameter, 0, 2,0, 
                                 blr_literal, blr_long, 0, 1,0,0,0,
                           blr_eql, 
                              blr_fid, 0, 0,0, 
                              blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 0,0, 
                              blr_fid, 0, 8,0, 
                           blr_eql, 
                              blr_fid, 1, 3,0, 
                              blr_literal, blr_long, 0, 1,0,0,0,
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 1, 2,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 1, 6,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 4,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_295 [107] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 4,0, 0, 
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
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
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
static const UCHAR	jrd_303 [97] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 4,0, 0, 
               blr_boolean, 
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
                     blr_fid, 0, 0,0, 
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
static const UCHAR	jrd_310 [129] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_long, 0, 
   blr_message, 1, 3,0, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 20,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 2,0, 
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
                                    blr_fid, 1, 7,0, 
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
static const UCHAR	jrd_321 [93] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 20,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
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
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_328 [104] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_long, 0, 
      blr_short, 0, 
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
                     blr_fid, 0, 7,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_336 [118] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_long, 0, 
      blr_short, 0, 
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
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 4,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_345 [122] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 0,0, 0,1, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 16,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 5,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_354 [104] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
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
                     blr_and, 
                        blr_not, 
                           blr_missing, 
                              blr_fid, 0, 9,0, 
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
                     blr_fid, 0, 9,0, 
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
static const UCHAR	jrd_361 [96] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_long, 0, 
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
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_368 [104] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_cstring2, 0,0, 0,4, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_long, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 30,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter2, 1, 0,0, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter2, 1, 1,0, 4,0, 
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
static const UCHAR	jrd_377 [85] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 24,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
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
static const UCHAR	jrd_383 [99] =
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
               blr_rid, 12,0, 0, 
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
                     blr_fid, 0, 0,0, 
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
static const UCHAR	jrd_390 [85] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 22,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
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
static const UCHAR	jrd_396 [228] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 15,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_int64, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 12,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_or, 
                        blr_missing, 
                           blr_fid, 0, 7,0, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_literal, blr_long, 0, 0,0,0,0,
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter2, 1, 3,0, 10,0, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter2, 1, 4,0, 11,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter2, 1, 6,0, 12,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter2, 1, 7,0, 13,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 9,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 14,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 8,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_415 [101] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_int64, 0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 12,0, 0, 
            blr_boolean, 
               blr_and, 
                  blr_missing, 
                     blr_fid, 0, 1,0, 
                  blr_eql, 
                     blr_fid, 0, 7,0, 
                     blr_literal, blr_long, 0, 0,0,0,0,
            blr_sort, 1, 
               blr_ascending, 
                  blr_fid, 0, 2,0, 
            blr_end, 
         blr_send, 0, 
            blr_begin, 
               blr_assignment, 
                  blr_fid, 0, 0,0, 
                  blr_parameter, 0, 0,0, 
               blr_assignment, 
                  blr_fid, 0, 3,0, 
                  blr_parameter, 0, 1,0, 
               blr_assignment, 
                  blr_literal, blr_long, 0, 1,0,0,0,
                  blr_parameter, 0, 2,0, 
               blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 2,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_420 [108] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_int64, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 12,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_missing, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 3,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_literal, blr_long, 0, 0,0,0,0,
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
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
static const UCHAR	jrd_426 [119] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 4,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 10,0, 0, 
            blr_boolean, 
               blr_and, 
                  blr_not, 
                     blr_missing, 
                        blr_fid, 0, 5,0, 
                  blr_and, 
                     blr_neq, 
                        blr_fid, 0, 5,0, 
                        blr_literal, blr_long, 0, 0,0,0,0,
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_literal, blr_long, 0, 0,0,0,0,
            blr_end, 
         blr_send, 0, 
            blr_begin, 
               blr_assignment, 
                  blr_fid, 0, 0,0, 
                  blr_parameter, 0, 0,0, 
               blr_assignment, 
                  blr_literal, blr_long, 0, 1,0,0,0,
                  blr_parameter, 0, 1,0, 
               blr_assignment, 
                  blr_fid, 0, 5,0, 
                  blr_parameter, 0, 2,0, 
               blr_assignment, 
                  blr_fid, 0, 4,0, 
                  blr_parameter, 0, 3,0, 
               blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 1,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_432 [114] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 5,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_fid, 1, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_439 [178] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 9,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 29,0, 0, 
               blr_rid, 28,0, 1, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 1, 4,0, 
                           blr_fid, 0, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter2, 1, 0,0, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter2, 1, 1,0, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter2, 1, 6,0, 5,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 4,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_453 [91] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_quad, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 8,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_460 [95] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 10,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_erase, 0, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_469 [110] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 13,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 3,0, 
                        blr_parameter, 0, 1,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_erase, 0, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_479 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
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
static const UCHAR	jrd_485 [160] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 13,0, 0, 
               blr_rid, 14,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 3,0, 
                        blr_and, 
                           blr_or, 
                              blr_eql, 
                                 blr_fid, 0, 3,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_eql, 
                                 blr_fid, 0, 3,0, 
                                 blr_parameter, 0, 1,0, 
                           blr_eql, 
                              blr_fid, 0, 0,0, 
                              blr_fid, 1, 9,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 9,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 1, 12,0, 
                     blr_parameter, 1, 3,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_496 [140] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 13,0, 0, 
               blr_rid, 14,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 2,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 3,0, 
                              blr_parameter, 0, 1,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 0,0, 
                                 blr_fid, 1, 0,0, 
                              blr_missing, 
                                 blr_fid, 1, 9,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 1, 12,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_505 [148] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_int64, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 13,0, 0, 
               blr_rid, 12,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 2,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 3,0, 
                              blr_parameter, 0, 1,0, 
                           blr_eql, 
                              blr_fid, 0, 0,0, 
                              blr_fid, 1, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 3,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_515 [160] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 13,0, 0, 
               blr_rid, 26,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 3,0, 
                        blr_and, 
                           blr_or, 
                              blr_eql, 
                                 blr_fid, 0, 3,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_eql, 
                                 blr_fid, 0, 3,0, 
                                 blr_parameter, 0, 1,0, 
                           blr_eql, 
                              blr_fid, 0, 0,0, 
                              blr_fid, 1, 16,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 16,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
                     blr_parameter, 1, 3,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_526 [140] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 13,0, 0, 
               blr_rid, 26,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 2,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 3,0, 
                              blr_parameter, 0, 1,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 0,0, 
                                 blr_fid, 1, 0,0, 
                              blr_missing, 
                                 blr_fid, 1, 16,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_535 [164] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 5,0, 0, 
               blr_rid, 13,0, 1, 
               blr_rid, 14,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_fid, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 2,0, 
                              blr_fid, 0, 0,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 1, 4,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 1, 3,0, 
                                    blr_parameter, 0, 1,0, 
                                 blr_and, 
                                    blr_eql, 
                                       blr_fid, 1, 0,0, 
                                       blr_fid, 2, 0,0, 
                                    blr_missing, 
                                       blr_fid, 2, 9,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 2, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 2, 12,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_544 [172] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_int64, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 5,0, 0, 
               blr_rid, 13,0, 1, 
               blr_rid, 12,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_fid, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 2,0, 
                              blr_fid, 0, 0,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 1, 4,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 1, 3,0, 
                                    blr_parameter, 0, 1,0, 
                                 blr_eql, 
                                    blr_fid, 1, 0,0, 
                                    blr_fid, 2, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 2, 3,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 2, 1,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 2, 0,0, 
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
static const UCHAR	jrd_554 [164] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 5,0, 0, 
               blr_rid, 13,0, 1, 
               blr_rid, 26,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_fid, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 2,0, 
                              blr_fid, 0, 0,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 1, 4,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 1, 3,0, 
                                    blr_parameter, 0, 1,0, 
                                 blr_and, 
                                    blr_eql, 
                                       blr_fid, 1, 0,0, 
                                       blr_fid, 2, 0,0, 
                                    blr_missing, 
                                       blr_fid, 2, 16,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 2, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 2, 1,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_563 [85] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_569 [118] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 10,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_parameter, 0, 0,0, 
                     blr_fid, 0, 5,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
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
                                    blr_fid, 1, 5,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_579 [122] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 10,0, 0, 
            blr_boolean, 
               blr_and, 
                  blr_not, 
                     blr_missing, 
                        blr_fid, 0, 5,0, 
                  blr_neq, 
                     blr_fid, 0, 5,0, 
                     blr_literal, blr_long, 0, 0,0,0,0,
            blr_end, 
         blr_begin, 
            blr_send, 0, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 0, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 0, 2,0, 
                  blr_end, 
            blr_label, 0, 
               blr_loop, 
                  blr_select, 
                     blr_receive, 2, 
                        blr_leave, 0, 
                     blr_receive, 1, 
                        blr_erase, 0, 
                     blr_end, 
            blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 1,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_588 [97] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 10,0, 0, 
            blr_boolean, 
               blr_and, 
                  blr_not, 
                     blr_missing, 
                        blr_fid, 0, 5,0, 
                  blr_eql, 
                     blr_fid, 0, 5,0, 
                     blr_literal, blr_long, 0, 0,0,0,0,
            blr_end, 
         blr_begin, 
            blr_send, 0, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 0, 0,0, 
                  blr_end, 
            blr_label, 0, 
               blr_loop, 
                  blr_select, 
                     blr_receive, 2, 
                        blr_leave, 0, 
                     blr_receive, 1, 
                        blr_erase, 0, 
                     blr_end, 
            blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 0,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_595 [270] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 20,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 5,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 1, 0,0, 
                           blr_fid, 0, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 2,0, 
                     blr_parameter2, 1, 0,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 1, 6,0, 
                     blr_parameter2, 1, 1,0, 6,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter2, 1, 2,0, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 1, 23,0, 
                     blr_parameter2, 1, 9,0, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter2, 1, 11,0, 10,0, 
                  blr_assignment, 
                     blr_fid, 1, 25,0, 
                     blr_parameter, 1, 12,0, 
                  blr_assignment, 
                     blr_fid, 0, 18,0, 
                     blr_parameter2, 1, 14,0, 13,0, 
                  blr_assignment, 
                     blr_fid, 1, 26,0, 
                     blr_parameter, 1, 15,0, 
                  blr_assignment, 
                     blr_fid, 1, 11,0, 
                     blr_parameter, 1, 16,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter, 1, 17,0, 
                  blr_assignment, 
                     blr_fid, 1, 9,0, 
                     blr_parameter, 1, 18,0, 
                  blr_assignment, 
                     blr_fid, 1, 10,0, 
                     blr_parameter, 1, 19,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 4,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_620 [182] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 13,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
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
                     blr_fid, 0, 2,0, 
                     blr_parameter2, 1, 0,0, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter2, 1, 1,0, 4,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 23,0, 
                     blr_parameter2, 1, 6,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 25,0, 
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 26,0, 
                     blr_parameter, 1, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter, 1, 9,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 10,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 11,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter, 1, 12,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 


using namespace Jrd;
using namespace Firebird;

static int blocking_ast_dsql_cache(void* ast_object);
static DSqlCacheItem* get_dsql_cache_item(thread_db* tdbb, int type, const QualifiedName& name);
static int blocking_ast_procedure(void*);
static int blocking_ast_relation(void*);
static int partners_ast_relation(void*);
static int rescan_ast_relation(void*);
static ULONG get_rel_flags_from_FLAGS(USHORT);
static void get_trigger(thread_db*, jrd_rel*, bid*, bid*, trig_vec**, const TEXT*, FB_UINT64, bool,
	USHORT, const MetaName&, const string&, const bid*);
static bool get_type(thread_db*, USHORT*, const UCHAR*, const TEXT*);
static void lookup_view_contexts(thread_db*, jrd_rel*);
static void make_relation_scope_name(const TEXT*, const USHORT, string& str);
static ValueExprNode* parse_field_default_blr(thread_db* tdbb, bid* blob_id);
static BoolExprNode* parse_field_validation_blr(thread_db* tdbb, bid* blob_id, const MetaName name);
static bool resolve_charset_and_collation(thread_db*, USHORT*, const UCHAR*, const UCHAR*);
static void save_trigger_data(thread_db*, trig_vec**, jrd_rel*, JrdStatement*, blb*, bid*,
	const TEXT*, FB_UINT64, bool, USHORT, const MetaName&, const string&,
	const bid*);
static void scan_partners(thread_db*, jrd_rel*);
static void store_dependencies(thread_db*, CompilerScratch*, const jrd_rel*,
	const MetaName&, int, jrd_tra*);
static bool verify_TRG_ignore_perm(thread_db*, const MetaName&);


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


static void inc_int_use_count(JrdStatement* statement)
{
	// Increment int_use_count for all procedures in resource list of request
	ResourceList& list = statement->resources;
	size_t i;

	for (list.find(Resource(Resource::rsc_procedure, 0, NULL, NULL, NULL), i);
		 i < list.getCount(); i++)
	{
		Resource& resource = list[i];
		if (resource.rsc_type != Resource::rsc_procedure)
			break;
		//// FIXME: CORE-4271: fb_assert(resource.rsc_routine->intUseCount >= 0);
		++resource.rsc_routine->intUseCount;
	}

	for (list.find(Resource(Resource::rsc_function, 0, NULL, NULL, NULL), i);
		 i < list.getCount(); i++)
	{
		Resource& resource = list[i];
		if (resource.rsc_type != Resource::rsc_function)
			break;
		//// FIXME: CORE-4271: fb_assert(resource.rsc_routine->intUseCount >= 0);
		++resource.rsc_routine->intUseCount;
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
		JrdStatement* stmt = (*vector)[i].statement;
		if (stmt && !stmt->isActive())
			inc_int_use_count(stmt);
	}
}


void MET_get_domain(thread_db* tdbb, MemoryPool& csbPool, const MetaName& name, dsc* desc,
	FieldInfo* fieldInfo)
{
   struct {
          bid  jrd_624;	// RDB$VALIDATION_BLR 
          bid  jrd_625;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_626;	// gds__utility 
          SSHORT jrd_627;	// gds__null_flag 
          SSHORT jrd_628;	// gds__null_flag 
          SSHORT jrd_629;	// gds__null_flag 
          SSHORT jrd_630;	// RDB$NULL_FLAG 
          SSHORT jrd_631;	// RDB$COLLATION_ID 
          SSHORT jrd_632;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_633;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_634;	// RDB$FIELD_LENGTH 
          SSHORT jrd_635;	// RDB$FIELD_SCALE 
          SSHORT jrd_636;	// RDB$FIELD_TYPE 
   } jrd_623;
   struct {
          TEXT  jrd_622 [32];	// RDB$FIELD_NAME 
   } jrd_621;
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
	Attachment* attachment = tdbb->getAttachment();
	bool found = false;

	AutoCacheRequest handle(tdbb, irq_l_domain, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle)
		FLD IN RDB$FIELDS WITH FLD.RDB$FIELD_NAME EQ name.c_str()*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_620, sizeof(jrd_620));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_621.jrd_622, 32);
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_621);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 38, (UCHAR*) &jrd_623);
	   if (!jrd_623.jrd_626) break;
	{
		if (DSC_make_descriptor(desc,
								/*FLD.RDB$FIELD_TYPE*/
								jrd_623.jrd_636,
								/*FLD.RDB$FIELD_SCALE*/
								jrd_623.jrd_635,
								/*FLD.RDB$FIELD_LENGTH*/
								jrd_623.jrd_634,
								/*FLD.RDB$FIELD_SUB_TYPE*/
								jrd_623.jrd_633,
								/*FLD.RDB$CHARACTER_SET_ID*/
								jrd_623.jrd_632,
								/*FLD.RDB$COLLATION_ID*/
								jrd_623.jrd_631))
		{
			found = true;

			if (fieldInfo)
			{
				fieldInfo->nullable = /*FLD.RDB$NULL_FLAG.NULL*/
						      jrd_623.jrd_629 || /*FLD.RDB$NULL_FLAG*/
    jrd_623.jrd_630 == 0;

				Jrd::ContextPoolHolder context(tdbb, &csbPool);

				if (/*FLD.RDB$DEFAULT_VALUE.NULL*/
				    jrd_623.jrd_628)
					fieldInfo->defaultValue = NULL;
				else
					fieldInfo->defaultValue = parse_field_default_blr(tdbb, &/*FLD.RDB$DEFAULT_VALUE*/
												 jrd_623.jrd_625);

				if (/*FLD.RDB$VALIDATION_BLR.NULL*/
				    jrd_623.jrd_627)
					fieldInfo->validationExpr = NULL;
				else
				{
					fieldInfo->validationExpr = parse_field_validation_blr(tdbb,
						&/*FLD.RDB$VALIDATION_BLR*/
						 jrd_623.jrd_624, name);
				}
			}
		}
	}
	/*END_FOR*/
	   }
	}

	if (!found)
	{
		ERR_post(Arg::Gds(isc_domnotdef) << Arg::Str(name));
	}
}


MetaName MET_get_relation_field(thread_db* tdbb, MemoryPool& csbPool, const MetaName& relationName,
	const MetaName& fieldName, dsc* desc, FieldInfo* fieldInfo)
{
   struct {
          bid  jrd_600;	// RDB$VALIDATION_BLR 
          bid  jrd_601;	// RDB$DEFAULT_VALUE 
          bid  jrd_602;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_603 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_604;	// gds__utility 
          SSHORT jrd_605;	// gds__null_flag 
          SSHORT jrd_606;	// gds__null_flag 
          SSHORT jrd_607;	// gds__null_flag 
          SSHORT jrd_608;	// gds__null_flag 
          SSHORT jrd_609;	// RDB$NULL_FLAG 
          SSHORT jrd_610;	// gds__null_flag 
          SSHORT jrd_611;	// RDB$NULL_FLAG 
          SSHORT jrd_612;	// RDB$COLLATION_ID 
          SSHORT jrd_613;	// gds__null_flag 
          SSHORT jrd_614;	// RDB$COLLATION_ID 
          SSHORT jrd_615;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_616;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_617;	// RDB$FIELD_LENGTH 
          SSHORT jrd_618;	// RDB$FIELD_SCALE 
          SSHORT jrd_619;	// RDB$FIELD_TYPE 
   } jrd_599;
   struct {
          TEXT  jrd_597 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_598 [32];	// RDB$RELATION_NAME 
   } jrd_596;
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
	Attachment* attachment = tdbb->getAttachment();
	bool found = false;
	MetaName sourceName;

	AutoCacheRequest handle(tdbb, irq_l_relfield, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle)
		RFL IN RDB$RELATION_FIELDS CROSS
		FLD IN RDB$FIELDS WITH
			RFL.RDB$RELATION_NAME EQ relationName.c_str() AND
			RFL.RDB$FIELD_NAME EQ fieldName.c_str() AND
			FLD.RDB$FIELD_NAME EQ RFL.RDB$FIELD_SOURCE*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_595, sizeof(jrd_595));
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_596.jrd_597, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_596.jrd_598, 32);
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_596);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 88, (UCHAR*) &jrd_599);
	   if (!jrd_599.jrd_604) break;
	{
		if (DSC_make_descriptor(desc,
								/*FLD.RDB$FIELD_TYPE*/
								jrd_599.jrd_619,
								/*FLD.RDB$FIELD_SCALE*/
								jrd_599.jrd_618,
								/*FLD.RDB$FIELD_LENGTH*/
								jrd_599.jrd_617,
								/*FLD.RDB$FIELD_SUB_TYPE*/
								jrd_599.jrd_616,
								/*FLD.RDB$CHARACTER_SET_ID*/
								jrd_599.jrd_615,
								(/*RFL.RDB$COLLATION_ID.NULL*/
								 jrd_599.jrd_613 ? /*FLD.RDB$COLLATION_ID*/
   jrd_599.jrd_612 : /*RFL.RDB$COLLATION_ID*/
   jrd_599.jrd_614)))
		{
			found = true;
			sourceName = /*RFL.RDB$FIELD_SOURCE*/
				     jrd_599.jrd_603;

			if (fieldInfo)
			{
				fieldInfo->nullable = /*RFL.RDB$NULL_FLAG.NULL*/
						      jrd_599.jrd_610 ?
					(/*FLD.RDB$NULL_FLAG.NULL*/
					 jrd_599.jrd_608 || /*FLD.RDB$NULL_FLAG*/
    jrd_599.jrd_609 == 0) : /*RFL.RDB$NULL_FLAG*/
	 jrd_599.jrd_611 == 0;

				Jrd::ContextPoolHolder context(tdbb, &csbPool);
				bid* defaultId = NULL;

				if (!/*RFL.RDB$DEFAULT_VALUE.NULL*/
				     jrd_599.jrd_607)
					defaultId = &/*RFL.RDB$DEFAULT_VALUE*/
						     jrd_599.jrd_602;
				else if (!/*FLD.RDB$DEFAULT_VALUE.NULL*/
					  jrd_599.jrd_606)
					defaultId = &/*FLD.RDB$DEFAULT_VALUE*/
						     jrd_599.jrd_601;

				if (defaultId)
					fieldInfo->defaultValue = parse_field_default_blr(tdbb, defaultId);
				else
					fieldInfo->defaultValue = NULL;

				if (/*FLD.RDB$VALIDATION_BLR.NULL*/
				    jrd_599.jrd_605)
					fieldInfo->validationExpr = NULL;
				else
				{
					fieldInfo->validationExpr = parse_field_validation_blr(tdbb,
						&/*FLD.RDB$VALIDATION_BLR*/
						 jrd_599.jrd_600, /*RFL.RDB$FIELD_SOURCE*/
  jrd_599.jrd_603);
				}
			}
		}
	}
	/*END_FOR*/
	   }
	}

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
	Attachment* attachment = tdbb->getAttachment();

	vec<jrd_rel*>* relations = attachment->att_relations;

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


static void adjust_dependencies(Routine* routine)
{
	if (routine->intUseCount == -1)
	{
		// Already processed
		return;
	}

	routine->intUseCount = -1; // Mark as undeletable

	if (routine->getStatement())
	{
		// Loop over procedures from resource list of request
		ResourceList& list = routine->getStatement()->resources;
		size_t i;

		for (list.find(Resource(Resource::rsc_procedure, 0, NULL, NULL, NULL), i);
			i < list.getCount(); i++)
		{
			Resource& resource = list[i];

			if (resource.rsc_type != Resource::rsc_procedure)
				break;

			routine = resource.rsc_routine;

			if (routine->intUseCount == routine->useCount)
			{
				// Mark it and all dependent procedures as undeletable
				adjust_dependencies(routine);
			}
		}

		for (list.find(Resource(Resource::rsc_function, 0, NULL, NULL, NULL), i);
			i < list.getCount(); i++)
		{
			Resource& resource = list[i];

			if (resource.rsc_type != Resource::rsc_function)
				break;

			routine = resource.rsc_routine;

			if (routine->intUseCount == routine->useCount)
			{
				// Mark it and all dependent functions as undeletable
				adjust_dependencies(routine);
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
 *      Check if all links between routines are properly counted
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* att = tdbb->getAttachment();
	if (!att)
		return;

	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine && routine->getStatement() /*&&
			!(routine->flags & Routine::FLAG_OBSOLETE)*/ )
		{
			fb_assert(routine->intUseCount == 0);
		}
	}

	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine && routine->getStatement() /*&&
			!(routine->flags & Routine::FLAG_OBSOLETE)*/ )
		{
			fb_assert(routine->intUseCount == 0);
		}
	}

	// Walk procedures and calculate internal dependencies
	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		jrd_prc* routine = *iter;

		if (routine && routine->getStatement() /*&&
			!(routine->flags & Routine::FLAG_OBSOLETE)*/ )
		{
			inc_int_use_count(routine->getStatement());
		}
	}

	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine && routine->getStatement() /*&&
			!(routine->flags & Routine::FLAG_OBSOLETE)*/ )
		{
			inc_int_use_count(routine->getStatement());
		}
	}

	// Walk procedures again and check dependencies
	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine && routine->getStatement() && /*
			 !(routine->flags & Routine::FLAG_OBSOLETE) && */
			 routine->useCount < routine->intUseCount)
		{
			char buffer[1024], *buf = buffer;
			buf += sprintf(buf, "Procedure %d:%s is not properly counted (use count=%d, prc use=%d). Used by: \n",
				routine->getId(), routine->getName().toString().c_str(),
				routine->useCount, routine->intUseCount);

			for (jrd_prc** iter2 = att->att_procedures.begin(); iter2 != att->att_procedures.end(); ++iter2)
			{
				Routine* routine2 = *iter2;

				if (routine2 && routine2->getStatement() /*&& !(routine2->flags & Routine::FLAG_OBSOLETE)*/ )
				{
					// Loop over procedures from resource list of request
					const ResourceList& list = routine2->getStatement()->resources;
					size_t i;

					for (list.find(Resource(Resource::rsc_procedure, 0, NULL, NULL, NULL), i);
					     i < list.getCount(); i++)
					{
						const Resource& resource = list[i];
						if (resource.rsc_type != Resource::rsc_procedure)
							break;

						if (resource.rsc_routine == routine)
						{
							// Do not enable this code in production builds unless
							// the possible B.O. is fixed here.
							buf += sprintf(buf, "%d:%s\n", routine2->getId(),
								routine2->getName().toString().c_str());
						}
					}
				}
			}

			for (Function** iter2 = att->att_functions.begin(); iter2 != att->att_functions.end(); ++iter2)
			{
				Routine* routine2 = *iter2;

				if (routine2 && routine2->getStatement() /*&& !(routine2->flags & Routine::FLAG_OBSOLETE)*/ )
				{
					// Loop over procedures from resource list of request
					const ResourceList& list = routine2->getStatement()->resources;
					size_t i;

					for (list.find(Resource(Resource::rsc_procedure, 0, NULL, NULL, NULL), i);
					     i < list.getCount(); i++)
					{
						const Resource& resource = list[i];
						if (resource.rsc_type != Resource::rsc_procedure)
							break;

						if (resource.rsc_routine == routine)
						{
							// Do not enable this code in production builds unless
							// the possible B.O. is fixed here.
							buf += sprintf(buf, "%d:%s\n", routine2->getId(),
								routine2->getName().toString().c_str());
						}
					}
				}
			}

			gds__log(buffer);
			fb_assert(false);
		}
	}

	// Walk functions again and check dependencies
	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine && routine->getStatement() && /*
			 !(routine->flags & Routine::FLAG_OBSOLETE) && */
			 routine->useCount < routine->intUseCount)
		{
			char buffer[1024], *buf = buffer;
			buf += sprintf(buf, "Function %d:%s is not properly counted (use count=%d, func use=%d). Used by: \n",
				routine->getId(), routine->getName().toString().c_str(),
				routine->useCount, routine->intUseCount);

			for (jrd_prc** iter2 = att->att_procedures.begin(); iter2 != att->att_procedures.end(); ++iter2)
			{
				Routine* routine2 = *iter2;

				if (routine2 && routine2->getStatement() /*&& !(routine2->flags & Routine::FLAG_OBSOLETE)*/ )
				{
					// Loop over procedures from resource list of request
					const ResourceList& list = routine2->getStatement()->resources;
					size_t i;

					for (list.find(Resource(Resource::rsc_function, 0, NULL, NULL, NULL), i);
					     i < list.getCount(); i++)
					{
						const Resource& resource = list[i];
						if (resource.rsc_type != Resource::rsc_function)
							break;

						if (resource.rsc_routine == routine)
						{
							// Do not enable this code in production builds unless
							// the possible B.O. is fixed here.
							buf += sprintf(buf, "%d:%s\n", routine2->getId(),
								routine2->getName().toString().c_str());
						}
					}
				}
			}

			for (Function** iter2 = att->att_functions.begin(); iter2 != att->att_functions.end(); ++iter2)
			{
				Routine* routine2 = *iter2;

				if (routine2 && routine2->getStatement() /*&& !(routine2->flags & Routine::FLAG_OBSOLETE)*/ )
				{
					// Loop over procedures from resource list of request
					const ResourceList& list = routine2->getStatement()->resources;
					size_t i;

					for (list.find(Resource(Resource::rsc_function, 0, NULL, NULL, NULL), i);
					     i < list.getCount(); i++)
					{
						const Resource& resource = list[i];
						if (resource.rsc_type != Resource::rsc_function)
							break;

						if (resource.rsc_routine == routine)
						{
							// Do not enable this code in production builds unless
							// the possible B.O. is fixed here.
							buf += sprintf(buf, "%d:%s\n", routine2->getId(),
								routine2->getName().toString().c_str());
						}
					}
				}
			}

			gds__log(buffer);
			fb_assert(false);
		}
	}

	// Fix back int_use_count
	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine)
			routine->intUseCount = 0;
	}

	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine)
			routine->intUseCount = 0;
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

	Attachment* att = tdbb->getAttachment();

	for (unsigned i = 0; i < DB_TRIGGER_MAX; i++)
		release_cached_triggers(tdbb, att->att_triggers[i]);

	vec<jrd_rel*>* relations = att->att_relations;
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

	// Walk routines and calculate internal dependencies.

	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine && routine->getStatement() &&
			!(routine->flags & Routine::FLAG_OBSOLETE) )
		{
			inc_int_use_count(routine->getStatement());
		}
	}

	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Function* routine = *iter;

		if (routine && routine->getStatement() &&
			!(routine->flags & Routine::FLAG_OBSOLETE) )
		{
			inc_int_use_count(routine->getStatement());
		}
	}

	// Walk routines again and adjust dependencies for routines which will not be removed.

	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine && routine->getStatement() &&
			!(routine->flags & Routine::FLAG_OBSOLETE) &&
			routine->useCount != routine->intUseCount )
		{
			adjust_dependencies(routine);
		}
	}

	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Function* routine = *iter;

		if (routine && routine->getStatement() &&
			!(routine->flags & Routine::FLAG_OBSOLETE) &&
			routine->useCount != routine->intUseCount )
		{
			adjust_dependencies(routine);
		}
	}

	// Deallocate all used requests.

	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		Routine* routine = *iter;

		if (routine)
		{
			if (routine->getStatement() && !(routine->flags & Routine::FLAG_OBSOLETE) &&
				routine->intUseCount >= 0 &&
				routine->useCount == routine->intUseCount)
			{
				routine->releaseStatement(tdbb);

				if (routine->existenceLock)	//// TODO: verify why this IF is necessary now
					LCK_release(tdbb, routine->existenceLock);
				routine->existenceLock = NULL;
				routine->flags |= Routine::FLAG_OBSOLETE;
			}

			// Leave it in state 0 to avoid extra pass next time to clear it
			// Note: we need to adjust intUseCount for all routines
			// in cache because any of them may have been affected from
			// dependencies earlier. Even routines that were not scanned yet !
			routine->intUseCount = 0;
		}
	}

	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Function* routine = *iter;

		if (routine)
		{
			if (routine->getStatement() && !(routine->flags & Routine::FLAG_OBSOLETE) &&
				routine->intUseCount >= 0 &&
				routine->useCount == routine->intUseCount)
			{
				routine->releaseStatement(tdbb);

				if (routine->existenceLock)	//// TODO: verify why this IF is necessary now
					LCK_release(tdbb, routine->existenceLock);
				routine->existenceLock = NULL;
				routine->flags |= Routine::FLAG_OBSOLETE;
			}

			// Leave it in state 0 to avoid extra pass next time to clear it
			// Note: we need to adjust intUseCount for all routines
			// in cache because any of them may have been affected from
			// dependencies earlier. Even routines that were not scanned yet !
			routine->intUseCount = 0;
		}
	}

#ifdef DEV_BUILD
	MET_verify_cache(tdbb);
#endif
}


bool MET_routine_in_use(thread_db* tdbb, Routine* routine)
{
/**************************************
 *
 *      M E T _ r o u t i n e _ i n _ u s e
 *
 **************************************
 *
 * Functional description
 *      Determine if routine is used by any user requests or transactions.
 *      Return false if routine is used only inside cache or not used at all.
 *
 **************************************/
	SET_TDBB(tdbb);

#ifdef DEV_BUILD
	MET_verify_cache(tdbb);
#endif

	Attachment* att = tdbb->getAttachment();

	vec<jrd_rel*>* relations = att->att_relations;
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

	// Walk routines and calculate internal dependencies

	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		jrd_prc* procedure = *iter;

		if (procedure && procedure->getStatement() &&
			!(procedure->flags & Routine::FLAG_OBSOLETE))
		{
			inc_int_use_count(procedure->getStatement());
		}
	}

	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Function* function = *iter;

		if (function && function->getStatement() &&
			!(function->flags & Routine::FLAG_OBSOLETE))
		{
			inc_int_use_count(function->getStatement());
		}
	}

	// Walk routines again and adjust dependencies for routines
	// which will not be removed.

	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		jrd_prc* procedure = *iter;

		if (procedure && procedure->getStatement() &&
			!(procedure->flags & Routine::FLAG_OBSOLETE) &&
			procedure->useCount != procedure->intUseCount && procedure != routine)
		{
			adjust_dependencies(procedure);
		}
	}

	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Function* function = *iter;

		if (function && function->getStatement() &&
			!(function->flags & Routine::FLAG_OBSOLETE) &&
			function->useCount != function->intUseCount && function != routine)
		{
			adjust_dependencies(function);
		}
	}

	const bool result = routine->useCount != routine->intUseCount;

	// Fix back intUseCount

	for (jrd_prc** iter = att->att_procedures.begin(); iter != att->att_procedures.end(); ++iter)
	{
		jrd_prc* procedure = *iter;

		if (procedure)
			procedure->intUseCount = 0;
	}

	for (Function** iter = att->att_functions.begin(); iter != att->att_functions.end(); ++iter)
	{
		Function* function = *iter;

		if (function)
			function->intUseCount = 0;
	}

#ifdef DEV_BUILD
	MET_verify_cache(tdbb);
#endif
	return result;
}


void MET_activate_shadow(thread_db* tdbb)
{
   struct {
          SSHORT jrd_578;	// gds__utility 
   } jrd_577;
   struct {
          SSHORT jrd_576;	// RDB$SHADOW_NUMBER 
   } jrd_575;
   struct {
          SSHORT jrd_573;	// gds__utility 
          SSHORT jrd_574;	// RDB$SHADOW_NUMBER 
   } jrd_572;
   struct {
          SSHORT jrd_571;	// RDB$SHADOW_NUMBER 
   } jrd_570;
   struct {
          SSHORT jrd_587;	// gds__utility 
   } jrd_586;
   struct {
          SSHORT jrd_585;	// gds__utility 
   } jrd_584;
   struct {
          TEXT  jrd_581 [256];	// RDB$FILE_NAME 
          SSHORT jrd_582;	// gds__utility 
          SSHORT jrd_583;	// RDB$SHADOW_NUMBER 
   } jrd_580;
   struct {
          SSHORT jrd_594;	// gds__utility 
   } jrd_593;
   struct {
          SSHORT jrd_592;	// gds__utility 
   } jrd_591;
   struct {
          SSHORT jrd_590;	// gds__utility 
   } jrd_589;
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
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	// Erase any secondary files of the primary database of the shadow being activated.

	AutoRequest handle;

	/*FOR(REQUEST_HANDLE handle) X IN RDB$FILES
		WITH X.RDB$SHADOW_NUMBER NOT MISSING
			AND X.RDB$SHADOW_NUMBER EQ 0*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_588, sizeof(jrd_588));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 2, (UCHAR*) &jrd_589);
	   if (!jrd_589.jrd_590) break;
		/*ERASE X;*/
		EXE_send (tdbb, handle, 1, 2, (UCHAR*) &jrd_591);
	/*END_FOR*/
	   EXE_send (tdbb, handle, 2, 2, (UCHAR*) &jrd_593);
	   }
	}

	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	const char* dbb_file_name = pageSpace->file->fil_string;

	// go through files looking for any that expand to the current database name
	SCHAR expanded_name[MAXPATHLEN];
	AutoRequest handle2;
	handle.reset();

	/*FOR(REQUEST_HANDLE handle) X IN RDB$FILES
		WITH X.RDB$SHADOW_NUMBER NOT MISSING
			AND X.RDB$SHADOW_NUMBER NE 0*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_579, sizeof(jrd_579));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 260, (UCHAR*) &jrd_580);
	   if (!jrd_580.jrd_582) break;

		PIO_expand(/*X.RDB$FILE_NAME*/
			   jrd_580.jrd_581, (USHORT)strlen(/*X.RDB$FILE_NAME*/
		 jrd_580.jrd_581),
					expanded_name, sizeof(expanded_name));

		if (!strcmp(expanded_name, dbb_file_name))
		{
			/*FOR(REQUEST_HANDLE handle2) Y IN RDB$FILES
				WITH X.RDB$SHADOW_NUMBER EQ Y.RDB$SHADOW_NUMBER*/
			{
			handle2.compile(tdbb, (UCHAR*) jrd_569, sizeof(jrd_569));
			jrd_570.jrd_571 = jrd_580.jrd_583;
			EXE_start (tdbb, handle2, attachment->getSysTransaction());
			EXE_send (tdbb, handle2, 0, 2, (UCHAR*) &jrd_570);
			while (1)
			   {
			   EXE_receive (tdbb, handle2, 1, 4, (UCHAR*) &jrd_572);
			   if (!jrd_572.jrd_573) break;
				/*MODIFY Y*/
				{
				
					/*Y.RDB$SHADOW_NUMBER*/
					jrd_572.jrd_574 = 0;
				/*END_MODIFY*/
				jrd_575.jrd_576 = jrd_572.jrd_574;
				EXE_send (tdbb, handle2, 2, 2, (UCHAR*) &jrd_575);
				}
			/*END_FOR*/
			   EXE_send (tdbb, handle2, 3, 2, (UCHAR*) &jrd_577);
			   }
			}

			/*ERASE X;*/
			EXE_send (tdbb, handle, 1, 2, (UCHAR*) &jrd_584);
		}
	/*END_FOR*/
	   EXE_send (tdbb, handle, 2, 2, (UCHAR*) &jrd_586);
	   }
	}
}


ULONG MET_align(const dsc* desc, ULONG value)
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

	alignment = MIN(alignment, FORMAT_ALIGNMENT);

	return FB_ALIGN(value, alignment);
}


DeferredWork* MET_change_fields(thread_db* tdbb, jrd_tra* transaction, const dsc* field_source)
{
   struct {
          TEXT  jrd_492 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_493 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_494;	// gds__utility 
          SSHORT jrd_495;	// RDB$FUNCTION_ID 
   } jrd_491;
   struct {
          TEXT  jrd_487 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_488;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_489;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_490;	// RDB$DEPENDED_ON_TYPE 
   } jrd_486;
   struct {
          TEXT  jrd_502 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_503;	// gds__utility 
          SSHORT jrd_504;	// RDB$FUNCTION_ID 
   } jrd_501;
   struct {
          TEXT  jrd_498 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_499;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_500;	// RDB$DEPENDED_ON_TYPE 
   } jrd_497;
   struct {
          ISC_INT64  jrd_511;	// RDB$TRIGGER_TYPE 
          TEXT  jrd_512 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_513 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_514;	// gds__utility 
   } jrd_510;
   struct {
          TEXT  jrd_507 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_508;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_509;	// RDB$DEPENDED_ON_TYPE 
   } jrd_506;
   struct {
          TEXT  jrd_522 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_523 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_524;	// gds__utility 
          SSHORT jrd_525;	// RDB$PROCEDURE_ID 
   } jrd_521;
   struct {
          TEXT  jrd_517 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_518;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_519;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_520;	// RDB$DEPENDED_ON_TYPE 
   } jrd_516;
   struct {
          TEXT  jrd_532 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_533;	// gds__utility 
          SSHORT jrd_534;	// RDB$PROCEDURE_ID 
   } jrd_531;
   struct {
          TEXT  jrd_528 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_529;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_530;	// RDB$DEPENDED_ON_TYPE 
   } jrd_527;
   struct {
          TEXT  jrd_541 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_542;	// gds__utility 
          SSHORT jrd_543;	// RDB$FUNCTION_ID 
   } jrd_540;
   struct {
          TEXT  jrd_537 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_538;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_539;	// RDB$DEPENDED_ON_TYPE 
   } jrd_536;
   struct {
          ISC_INT64  jrd_550;	// RDB$TRIGGER_TYPE 
          TEXT  jrd_551 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_552 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_553;	// gds__utility 
   } jrd_549;
   struct {
          TEXT  jrd_546 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_547;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_548;	// RDB$DEPENDED_ON_TYPE 
   } jrd_545;
   struct {
          TEXT  jrd_560 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_561;	// gds__utility 
          SSHORT jrd_562;	// RDB$PROCEDURE_ID 
   } jrd_559;
   struct {
          TEXT  jrd_556 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_557;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_558;	// RDB$DEPENDED_ON_TYPE 
   } jrd_555;
   struct {
          TEXT  jrd_567 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_568;	// gds__utility 
   } jrd_566;
   struct {
          TEXT  jrd_565 [32];	// RDB$FIELD_SOURCE 
   } jrd_564;
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
 *      Find all functions and schedule a BLR validate.
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();

	dsc relation_name;
	DeferredWork* dw = NULL;
	AutoCacheRequest request(tdbb, irq_m_fields, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$RELATION_FIELDS WITH
			X.RDB$FIELD_SOURCE EQ field_source->dsc_address*/
	{
	request.compile(tdbb, (UCHAR*) jrd_563, sizeof(jrd_563));
	gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_564.jrd_565, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_564);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_566);
	   if (!jrd_566.jrd_568) break;
	{
		relation_name.makeText(sizeof(/*X.RDB$RELATION_NAME*/
					      jrd_566.jrd_567), CS_METADATA,
			(UCHAR*) /*X.RDB$RELATION_NAME*/
				 jrd_566.jrd_567);
		SCL_check_relation(tdbb, &relation_name, SCL_control);
		dw = DFW_post_work(transaction, dfw_update_format, &relation_name, 0);

		AutoCacheRequest request2(tdbb, irq_m_fields4, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request2)
			RFL IN RDB$RELATION_FIELDS CROSS
			DEP IN RDB$DEPENDENCIES CROSS
			PRC IN RDB$PROCEDURES
				WITH RFL.RDB$FIELD_SOURCE EQ field_source->dsc_address AND
					 DEP.RDB$DEPENDED_ON_NAME EQ RFL.RDB$RELATION_NAME AND
					 DEP.RDB$FIELD_NAME EQ RFL.RDB$FIELD_NAME AND
					 DEP.RDB$DEPENDED_ON_TYPE EQ obj_relation AND
					 DEP.RDB$DEPENDENT_TYPE EQ obj_procedure AND
					 DEP.RDB$DEPENDENT_NAME EQ PRC.RDB$PROCEDURE_NAME AND
					 PRC.RDB$PACKAGE_NAME MISSING*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_554, sizeof(jrd_554));
		gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_555.jrd_556, 32);
		jrd_555.jrd_557 = obj_procedure;
		jrd_555.jrd_558 = obj_relation;
		EXE_start (tdbb, request2, attachment->getSysTransaction());
		EXE_send (tdbb, request2, 0, 36, (UCHAR*) &jrd_555);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 36, (UCHAR*) &jrd_559);
		   if (!jrd_559.jrd_561) break;
		{
			MetaName proc_name(/*PRC.RDB$PROCEDURE_NAME*/
					   jrd_559.jrd_560);

			dsc desc;
			desc.makeText(proc_name.length(), CS_METADATA, (UCHAR*) proc_name.c_str());

			DeferredWork* dw2 =
				DFW_post_work(transaction, dfw_modify_procedure, &desc, /*PRC.RDB$PROCEDURE_ID*/
											jrd_559.jrd_562);
			DFW_post_work_arg(transaction, dw2, NULL, 0, dfw_arg_check_blr);
		}
		/*END_FOR*/
		   }
		}

		request2.reset(tdbb, irq_m_fields5, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request2)
			RFL IN RDB$RELATION_FIELDS CROSS
			DEP IN RDB$DEPENDENCIES CROSS
			TRG IN RDB$TRIGGERS
				WITH RFL.RDB$FIELD_SOURCE EQ field_source->dsc_address AND
					 DEP.RDB$DEPENDED_ON_NAME EQ RFL.RDB$RELATION_NAME AND
					 DEP.RDB$FIELD_NAME EQ RFL.RDB$FIELD_NAME AND
					 DEP.RDB$DEPENDED_ON_TYPE EQ obj_relation AND
					 DEP.RDB$DEPENDENT_TYPE EQ obj_trigger AND
					 DEP.RDB$DEPENDENT_NAME EQ TRG.RDB$TRIGGER_NAME*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_544, sizeof(jrd_544));
		gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_545.jrd_546, 32);
		jrd_545.jrd_547 = obj_trigger;
		jrd_545.jrd_548 = obj_relation;
		EXE_start (tdbb, request2, attachment->getSysTransaction());
		EXE_send (tdbb, request2, 0, 36, (UCHAR*) &jrd_545);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 74, (UCHAR*) &jrd_549);
		   if (!jrd_549.jrd_553) break;
		{
			MetaName trigger_name(/*TRG.RDB$TRIGGER_NAME*/
					      jrd_549.jrd_552);
			MetaName trigger_relation_name(/*TRG.RDB$RELATION_NAME*/
						       jrd_549.jrd_551);

			dsc desc;
			desc.makeText(trigger_name.length(), CS_METADATA, (UCHAR*) trigger_name.c_str());

			DeferredWork* dw2 = DFW_post_work(transaction, dfw_modify_trigger, &desc, 0);
			DFW_post_work_arg(transaction, dw2, NULL, /*TRG.RDB$TRIGGER_TYPE*/
								  jrd_549.jrd_550, dfw_arg_trg_type);

			desc.dsc_length = trigger_relation_name.length();
			desc.dsc_address = (UCHAR*) trigger_relation_name.c_str();
			DFW_post_work_arg(transaction, dw2, &desc, 0, dfw_arg_check_blr);
		}
		/*END_FOR*/
		   }
		}

		request2.reset(tdbb, irq_m_fields8, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request2)
			RFL IN RDB$RELATION_FIELDS CROSS
			DEP IN RDB$DEPENDENCIES CROSS
			FUN IN RDB$FUNCTIONS
				WITH RFL.RDB$FIELD_SOURCE EQ field_source->dsc_address AND
					 DEP.RDB$DEPENDED_ON_NAME EQ RFL.RDB$RELATION_NAME AND
					 DEP.RDB$FIELD_NAME EQ RFL.RDB$FIELD_NAME AND
					 DEP.RDB$DEPENDED_ON_TYPE EQ obj_relation AND
					 DEP.RDB$DEPENDENT_TYPE EQ obj_udf AND
					 DEP.RDB$DEPENDENT_NAME EQ FUN.RDB$FUNCTION_NAME AND
					 FUN.RDB$PACKAGE_NAME MISSING*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_535, sizeof(jrd_535));
		gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_536.jrd_537, 32);
		jrd_536.jrd_538 = obj_udf;
		jrd_536.jrd_539 = obj_relation;
		EXE_start (tdbb, request2, attachment->getSysTransaction());
		EXE_send (tdbb, request2, 0, 36, (UCHAR*) &jrd_536);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 36, (UCHAR*) &jrd_540);
		   if (!jrd_540.jrd_542) break;
		{
			MetaName name(/*FUN.RDB$FUNCTION_NAME*/
				      jrd_540.jrd_541);

			dsc desc;
			desc.makeText(name.length(), CS_METADATA, (UCHAR*) name.c_str());

			DeferredWork* dw2 =
				DFW_post_work(transaction, dfw_modify_function, &desc, /*FUN.RDB$FUNCTION_ID*/
										       jrd_540.jrd_543);
			DFW_post_work_arg(transaction, dw2, NULL, 0, dfw_arg_check_blr);
		}
		/*END_FOR*/
		   }
		}
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, irq_m_fields2, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		DEP IN RDB$DEPENDENCIES CROSS
		PRC IN RDB$PROCEDURES
			WITH DEP.RDB$DEPENDED_ON_NAME EQ field_source->dsc_address AND
				 DEP.RDB$DEPENDED_ON_TYPE EQ obj_field AND
				 DEP.RDB$DEPENDENT_TYPE EQ obj_procedure AND
				 DEP.RDB$DEPENDENT_NAME EQ PRC.RDB$PROCEDURE_NAME AND
				 PRC.RDB$PACKAGE_NAME MISSING*/
	{
	request.compile(tdbb, (UCHAR*) jrd_526, sizeof(jrd_526));
	gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_527.jrd_528, 32);
	jrd_527.jrd_529 = obj_procedure;
	jrd_527.jrd_530 = obj_field;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 36, (UCHAR*) &jrd_527);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_531);
	   if (!jrd_531.jrd_533) break;
	{
		MetaName proc_name(/*PRC.RDB$PROCEDURE_NAME*/
				   jrd_531.jrd_532);

		dsc desc;
		desc.makeText(proc_name.length(), CS_METADATA, (UCHAR*) proc_name.c_str());

		DeferredWork* dw2 =
			DFW_post_work(transaction, dfw_modify_procedure, &desc, /*PRC.RDB$PROCEDURE_ID*/
										jrd_531.jrd_534);
		DFW_post_work_arg(transaction, dw2, NULL, 0, dfw_arg_check_blr);
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, irq_m_fields6, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		DEP IN RDB$DEPENDENCIES CROSS
		PRC IN RDB$PROCEDURES
			WITH DEP.RDB$DEPENDED_ON_NAME EQ field_source->dsc_address AND
				 DEP.RDB$DEPENDED_ON_TYPE EQ obj_field AND
				 (DEP.RDB$DEPENDENT_TYPE EQ obj_package_header OR
				 	DEP.RDB$DEPENDENT_TYPE EQ obj_package_body) AND
				 DEP.RDB$DEPENDENT_NAME EQ PRC.RDB$PACKAGE_NAME*/
	{
	request.compile(tdbb, (UCHAR*) jrd_515, sizeof(jrd_515));
	gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_516.jrd_517, 32);
	jrd_516.jrd_518 = obj_package_body;
	jrd_516.jrd_519 = obj_package_header;
	jrd_516.jrd_520 = obj_field;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 38, (UCHAR*) &jrd_516);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 68, (UCHAR*) &jrd_521);
	   if (!jrd_521.jrd_524) break;
	{
		MetaName proc_name(/*PRC.RDB$PROCEDURE_NAME*/
				   jrd_521.jrd_523);

		dsc desc;
		desc.makeText(proc_name.length(), CS_METADATA, (UCHAR*) proc_name.c_str());

		DeferredWork* dw2 = DFW_post_work(transaction, dfw_modify_procedure, &desc,
			/*PRC.RDB$PROCEDURE_ID*/
			jrd_521.jrd_525, /*PRC.RDB$PACKAGE_NAME*/
  jrd_521.jrd_522);
		DFW_post_work_arg(transaction, dw2, NULL, 0, dfw_arg_check_blr);
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, irq_m_fields3, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		DEP IN RDB$DEPENDENCIES CROSS
		TRG IN RDB$TRIGGERS
			WITH DEP.RDB$DEPENDED_ON_NAME EQ field_source->dsc_address AND
				 DEP.RDB$DEPENDED_ON_TYPE EQ obj_field AND
				 DEP.RDB$DEPENDENT_TYPE EQ obj_trigger AND
				 DEP.RDB$DEPENDENT_NAME EQ TRG.RDB$TRIGGER_NAME*/
	{
	request.compile(tdbb, (UCHAR*) jrd_505, sizeof(jrd_505));
	gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_506.jrd_507, 32);
	jrd_506.jrd_508 = obj_trigger;
	jrd_506.jrd_509 = obj_field;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 36, (UCHAR*) &jrd_506);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 74, (UCHAR*) &jrd_510);
	   if (!jrd_510.jrd_514) break;
	{
		MetaName trigger_name(/*TRG.RDB$TRIGGER_NAME*/
				      jrd_510.jrd_513);
		MetaName trigger_relation_name(/*TRG.RDB$RELATION_NAME*/
					       jrd_510.jrd_512);

		dsc desc;
		desc.makeText(trigger_name.length(), CS_METADATA, (UCHAR*) trigger_name.c_str());

		DeferredWork* dw2 = DFW_post_work(transaction, dfw_modify_trigger, &desc, 0);
		DFW_post_work_arg(transaction, dw2, NULL, /*TRG.RDB$TRIGGER_TYPE*/
							  jrd_510.jrd_511, dfw_arg_trg_type);

		desc.dsc_length = trigger_relation_name.length();
		desc.dsc_address = (UCHAR*) trigger_relation_name.c_str();
		DFW_post_work_arg(transaction, dw2, &desc, 0, dfw_arg_check_blr);
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, irq_m_fields7, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		DEP IN RDB$DEPENDENCIES CROSS
		FUN IN RDB$FUNCTIONS
			WITH DEP.RDB$DEPENDED_ON_NAME EQ field_source->dsc_address AND
				 DEP.RDB$DEPENDED_ON_TYPE EQ obj_field AND
				 DEP.RDB$DEPENDENT_TYPE EQ obj_udf AND
				 DEP.RDB$DEPENDENT_NAME EQ FUN.RDB$FUNCTION_NAME AND
				 FUN.RDB$PACKAGE_NAME MISSING*/
	{
	request.compile(tdbb, (UCHAR*) jrd_496, sizeof(jrd_496));
	gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_497.jrd_498, 32);
	jrd_497.jrd_499 = obj_udf;
	jrd_497.jrd_500 = obj_field;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 36, (UCHAR*) &jrd_497);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_501);
	   if (!jrd_501.jrd_503) break;
	{
		MetaName name(/*FUN.RDB$FUNCTION_NAME*/
			      jrd_501.jrd_502);

		dsc desc;
		desc.makeText(name.length(), CS_METADATA, (UCHAR*) name.c_str());

		DeferredWork* dw2 =
			DFW_post_work(transaction, dfw_modify_function, &desc, /*FUN.RDB$FUNCTION_ID*/
									       jrd_501.jrd_504);
		DFW_post_work_arg(transaction, dw2, NULL, 0, dfw_arg_check_blr);
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, irq_m_fields9, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		DEP IN RDB$DEPENDENCIES CROSS
		FUN IN RDB$FUNCTIONS
			WITH DEP.RDB$DEPENDED_ON_NAME EQ field_source->dsc_address AND
				 DEP.RDB$DEPENDED_ON_TYPE EQ obj_field AND
				 (DEP.RDB$DEPENDENT_TYPE EQ obj_package_header OR
				 	DEP.RDB$DEPENDENT_TYPE EQ obj_package_body) AND
				 DEP.RDB$DEPENDENT_NAME EQ FUN.RDB$PACKAGE_NAME*/
	{
	request.compile(tdbb, (UCHAR*) jrd_485, sizeof(jrd_485));
	gds__vtov ((const char*) field_source->dsc_address, (char*) jrd_486.jrd_487, 32);
	jrd_486.jrd_488 = obj_package_body;
	jrd_486.jrd_489 = obj_package_header;
	jrd_486.jrd_490 = obj_field;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 38, (UCHAR*) &jrd_486);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 68, (UCHAR*) &jrd_491);
	   if (!jrd_491.jrd_494) break;
	{
		MetaName name(/*FUN.RDB$FUNCTION_NAME*/
			      jrd_491.jrd_493);

		dsc desc;
		desc.makeText(name.length(), CS_METADATA, (UCHAR*) name.c_str());

		DeferredWork* dw2 = DFW_post_work(transaction, dfw_modify_function, &desc,
			/*FUN.RDB$FUNCTION_ID*/
			jrd_491.jrd_495, /*FUN.RDB$PACKAGE_NAME*/
  jrd_491.jrd_492);
		DFW_post_work_arg(transaction, dw2, NULL, 0, dfw_arg_check_blr);
	}
	/*END_FOR*/
	   }
	}

	return dw;
}


Format* MET_current(thread_db* tdbb, jrd_rel* relation)
{
   struct {
          SSHORT jrd_483;	// gds__utility 
          SSHORT jrd_484;	// RDB$FORMAT 
   } jrd_482;
   struct {
          SSHORT jrd_481;	// RDB$RELATION_ID 
   } jrd_480;
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
	Attachment* attachment = tdbb->getAttachment();

	if (!(relation->rel_flags & REL_scanned))
	{
		AutoCacheRequest request(tdbb, irq_l_curr_format, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			REL IN RDB$RELATIONS
			WITH REL.RDB$RELATION_ID EQ relation->rel_id*/
		{
		request.compile(tdbb, (UCHAR*) jrd_479, sizeof(jrd_479));
		jrd_480.jrd_481 = relation->rel_id;
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_480);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_482);
		   if (!jrd_482.jrd_483) break;
		{
			relation->rel_current_fmt = /*REL.RDB$FORMAT*/
						    jrd_482.jrd_484;
		}
		/*END_FOR*/
		   }
		}
	}

	// Usually, format numbers start with one and they are present in RDB$FORMATS.
	// However, system tables have zero as their initial format and they don't have
	// any related records in RDB$FORMATS, instead their rel_formats[0] is initialized
	// directly (see ini.epp). Every other case of zero format number found for an already
	// scanned table must be catched here and investigated.
	fb_assert(relation->rel_current_fmt || relation->isSystem());

	relation->rel_current_format = MET_format(tdbb, relation, relation->rel_current_fmt);

	return relation->rel_current_format;
}


void MET_delete_dependencies(thread_db* tdbb,
							 const MetaName& object_name,
							 int dependency_type,
							 jrd_tra* transaction)
{
   struct {
          SSHORT jrd_478;	// gds__utility 
   } jrd_477;
   struct {
          SSHORT jrd_476;	// gds__utility 
   } jrd_475;
   struct {
          SSHORT jrd_474;	// gds__utility 
   } jrd_473;
   struct {
          TEXT  jrd_471 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_472;	// RDB$DEPENDENT_TYPE 
   } jrd_470;
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

//	if (!object_name)
//		return;
//
	AutoCacheRequest request(tdbb, irq_d_deps, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		DEP IN RDB$DEPENDENCIES
			WITH DEP.RDB$DEPENDENT_NAME = object_name.c_str()
			AND DEP.RDB$DEPENDENT_TYPE = dependency_type*/
	{
	request.compile(tdbb, (UCHAR*) jrd_469, sizeof(jrd_469));
	gds__vtov ((const char*) object_name.c_str(), (char*) jrd_470.jrd_471, 32);
	jrd_470.jrd_472 = dependency_type;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_470);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_473);
	   if (!jrd_473.jrd_474) break;
	{
		/*ERASE DEP;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_475);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_477);
	   }
	}
}


void MET_delete_shadow(thread_db* tdbb, USHORT shadow_number)
{
   struct {
          SSHORT jrd_468;	// gds__utility 
   } jrd_467;
   struct {
          SSHORT jrd_466;	// gds__utility 
   } jrd_465;
   struct {
          SSHORT jrd_464;	// gds__utility 
   } jrd_463;
   struct {
          SSHORT jrd_462;	// RDB$SHADOW_NUMBER 
   } jrd_461;
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
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	AutoRequest handle;

	/*FOR(REQUEST_HANDLE handle)
		X IN RDB$FILES WITH X.RDB$SHADOW_NUMBER EQ shadow_number*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_460, sizeof(jrd_460));
	jrd_461.jrd_462 = shadow_number;
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_461);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_463);
	   if (!jrd_463.jrd_464) break;
		/*ERASE X;*/
		EXE_send (tdbb, handle, 2, 2, (UCHAR*) &jrd_465);
	/*END_FOR*/
	   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_467);
	   }
	}

	for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
	{
		if (shadow->sdw_number == shadow_number) {
			shadow->sdw_flags |= SDW_shutdown;
		}
	}

	// notify other processes to check for shadow deletion
	if (SDW_lck_update(tdbb, 0))
		SDW_notify(tdbb);
}


bool MET_dsql_cache_use(thread_db* tdbb, int type, const MetaName& name, const MetaName& package)
{
	DSqlCacheItem* item = get_dsql_cache_item(tdbb, type, QualifiedName(name, package));

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


void MET_dsql_cache_release(thread_db* tdbb, int type, const MetaName& name, const MetaName& package)
{
	DSqlCacheItem* item = get_dsql_cache_item(tdbb, type, QualifiedName(name, package));

	// release lock
	LCK_release(tdbb, item->lock);

	// notify others through AST to mark as obsolete
	const USHORT key_length = item->lock->lck_length;
	AutoPtr<Lock> temp_lock(FB_NEW_RPT(*tdbb->getDefaultPool(), key_length)
		Lock(tdbb, key_length, LCK_dsql_cache));
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
          bid  jrd_458;	// RDB$DESCRIPTOR 
          SSHORT jrd_459;	// gds__utility 
   } jrd_457;
   struct {
          SSHORT jrd_455;	// RDB$FORMAT 
          SSHORT jrd_456;	// RDB$RELATION_ID 
   } jrd_454;
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
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	Format* format;
	vec<Format*>* formats = relation->rel_formats;
	if (formats && (number < formats->count()) && (format = (*formats)[number]))
	{
		return format;
	}

	format = NULL;
	AutoCacheRequest request(tdbb, irq_r_format, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$FORMATS WITH X.RDB$RELATION_ID EQ relation->rel_id AND
			X.RDB$FORMAT EQ number*/
	{
	request.compile(tdbb, (UCHAR*) jrd_453, sizeof(jrd_453));
	jrd_454.jrd_455 = number;
	jrd_454.jrd_456 = relation->rel_id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_454);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 10, (UCHAR*) &jrd_457);
	   if (!jrd_457.jrd_459) break;
	{
		blb* blob = blb::open(tdbb, attachment->getSysTransaction(), &/*X.RDB$DESCRIPTOR*/
									      jrd_457.jrd_458);

		// Use generic representation of formats with 32-bit offsets

		HalfStaticArray<UCHAR, BUFFER_MEDIUM> buffer;
		blob->BLB_get_data(tdbb, buffer.getBuffer(blob->blb_length), blob->blb_length);
		unsigned bufferPos = 2;
		USHORT count = buffer[0] | (buffer[1] << 8);

		format = Format::newFormat(*relation->rel_pool, count);

		Array<Ods::Descriptor> odsDescs;
		Ods::Descriptor* odsDesc = odsDescs.getBuffer(count);
		memcpy(odsDesc, buffer.begin() + bufferPos, count * sizeof(Ods::Descriptor));

		for (Format::fmt_desc_iterator desc = format->fmt_desc.begin();
			 desc < format->fmt_desc.end(); ++desc, ++odsDesc)
		{
			*desc = *odsDesc;
			if (odsDesc->dsc_offset)
				format->fmt_length = odsDesc->dsc_offset + desc->dsc_length;
		}

		const UCHAR* p = buffer.begin() + bufferPos + count * sizeof(Ods::Descriptor);
		count = p[0] | (p[1] << 8);
		p += 2;

		while (count-- > 0)
		{
			USHORT offset = p[0] | (p[1] << 8);
			p += 2;

			const Ods::Descriptor* odsDflDesc = (Ods::Descriptor*) p;
			p = (UCHAR*) (odsDflDesc + 1);

			dsc desc = *odsDflDesc;
			desc.dsc_address = const_cast<UCHAR*>(p);
			EVL_make_value(tdbb, &desc, &format->fmt_defaults[offset], dbb->dbb_permanent);

			p += desc.dsc_length;
		}
	}
	/*END_FOR*/
	   }
	}

	if (!format)
		format = Format::newFormat(*relation->rel_pool);

	format->fmt_version = number;

	// Link the format block into the world

	formats = relation->rel_formats =
		vec<Format*>::newVector(*relation->rel_pool, relation->rel_formats, number + 1);
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

	// Force key to uppercase, following C locale rules for uppercasing
	// At the same time, search for the first period in the string (if any)
	UCHAR buffer[MAX_SQL_IDENTIFIER_SIZE];			// BASED ON RDB$COLLATION_NAME
	UCHAR* p = buffer;
	UCHAR* period = NULL;
	for (; name < end_name && p < buffer + sizeof(buffer) - 1; p++, name++)
	{
		*p = UPPER7(*name);
		if ((*p == '.') && !period) {
			period = p;
		}
	}
	*p = 0;

	// Is there a period, separating collation name from character set?
	if (period)
	{
		*period = 0;
		return resolve_charset_and_collation(tdbb, id, period + 1, buffer);
	}

	// Is it a character set name (implying charset default collation sequence)

	bool res = resolve_charset_and_collation(tdbb, id, buffer, NULL);
	if (!res)
	{
		// Is it a collation name (implying implementation-default character set)
		res = resolve_charset_and_collation(tdbb, id, NULL, buffer);
	}
	return res;
}


bool MET_get_char_coll_subtype_info(thread_db* tdbb, USHORT id, SubtypeInfo* info)
{
   struct {
          bid  jrd_444;	// RDB$SPECIFIC_ATTRIBUTES 
          TEXT  jrd_445 [32];	// RDB$BASE_COLLATION_NAME 
          TEXT  jrd_446 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_447 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_448;	// gds__utility 
          SSHORT jrd_449;	// gds__null_flag 
          SSHORT jrd_450;	// RDB$COLLATION_ATTRIBUTES 
          SSHORT jrd_451;	// gds__null_flag 
          SSHORT jrd_452;	// gds__null_flag 
   } jrd_443;
   struct {
          SSHORT jrd_441;	// RDB$COLLATION_ID 
          SSHORT jrd_442;	// RDB$CHARACTER_SET_ID 
   } jrd_440;
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
	Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, irq_l_subtype, IRQ_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request) FIRST 1
		CL IN RDB$COLLATIONS CROSS
		CS IN RDB$CHARACTER_SETS
		WITH CL.RDB$CHARACTER_SET_ID EQ charset_id AND
			CL.RDB$COLLATION_ID EQ collation_id AND
			CS.RDB$CHARACTER_SET_ID EQ CL.RDB$CHARACTER_SET_ID*/
	{
	request.compile(tdbb, (UCHAR*) jrd_439, sizeof(jrd_439));
	jrd_440.jrd_441 = collation_id;
	jrd_440.jrd_442 = charset_id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_440);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 114, (UCHAR*) &jrd_443);
	   if (!jrd_443.jrd_448) break;
	{
		found = true;

		info->charsetName = /*CS.RDB$CHARACTER_SET_NAME*/
				    jrd_443.jrd_447;
		info->collationName = /*CL.RDB$COLLATION_NAME*/
				      jrd_443.jrd_446;

		if (/*CL.RDB$BASE_COLLATION_NAME.NULL*/
		    jrd_443.jrd_452)
			info->baseCollationName = info->collationName;
		else
			info->baseCollationName = /*CL.RDB$BASE_COLLATION_NAME*/
						  jrd_443.jrd_445;

		if (/*CL.RDB$SPECIFIC_ATTRIBUTES.NULL*/
		    jrd_443.jrd_451)
			info->specificAttributes.clear();
		else
		{
			blb* blob = blb::open(tdbb, attachment->getSysTransaction(), &/*CL.RDB$SPECIFIC_ATTRIBUTES*/
										      jrd_443.jrd_444);
			const ULONG length = blob->blb_length;

			// ASF: Here info->specificAttributes is in UNICODE_FSS charset.
			// It will be converted to the collation charset in intl.cpp
			blob->BLB_get_data(tdbb, info->specificAttributes.getBuffer(length), length);
		}

		info->attributes = (USHORT)/*CL.RDB$COLLATION_ATTRIBUTES*/
					   jrd_443.jrd_450;
		info->ignoreAttributes = /*CL.RDB$COLLATION_ATTRIBUTES.NULL*/
					 jrd_443.jrd_449;
	}
	/*END_FOR*/
	   }
	}

	return found;
}


DmlNode* MET_get_dependencies(thread_db* tdbb,
							  jrd_rel* relation,
							  const UCHAR* blob,
							  const ULONG blob_length,
							  CompilerScratch* view_csb,
							  bid* blob_id,
							  JrdStatement** statementPtr,
							  CompilerScratch** csb_ptr,
							  const MetaName& object_name,
							  int type,
							  USHORT flags,
							  jrd_tra* transaction,
							  const MetaName& domain_validation)
{
   struct {
          TEXT  jrd_437 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_438;	// gds__utility 
   } jrd_436;
   struct {
          TEXT  jrd_434 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_435 [32];	// RDB$RELATION_NAME 
   } jrd_433;
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
	Attachment* attachment = tdbb->getAttachment();

	fb_assert(domain_validation.isEmpty() || object_name == domain_validation); // for now

	CompilerScratch* csb = CompilerScratch::newCsb(*tdbb->getDefaultPool(), 5, domain_validation);
	csb->csb_g_flags |= (csb_get_dependencies | flags);

	DmlNode* node;

	if (blob)
	{
		node = PAR_blr(tdbb, relation, blob, blob_length, view_csb, &csb, statementPtr,
			(type == obj_trigger && relation != NULL), 0);
	}
	else
	{
		node = MET_parse_blob(tdbb, relation, blob_id, &csb, statementPtr,
			(type == obj_trigger && relation != NULL), type == obj_validation);
	}

	if (type == obj_computed)
	{
		MetaName domainName;

		AutoRequest handle;

		/*FOR(REQUEST_HANDLE handle)
			RLF IN RDB$RELATION_FIELDS CROSS
				FLD IN RDB$FIELDS WITH
				RLF.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME AND
				RLF.RDB$RELATION_NAME EQ relation->rel_name.c_str() AND
				RLF.RDB$FIELD_NAME EQ object_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_432, sizeof(jrd_432));
		gds__vtov ((const char*) object_name.c_str(), (char*) jrd_433.jrd_434, 32);
		gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_433.jrd_435, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_433);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 34, (UCHAR*) &jrd_436);
		   if (!jrd_436.jrd_438) break;
		{
			domainName = /*FLD.RDB$FIELD_NAME*/
				     jrd_436.jrd_437;
		}
		/*END_FOR*/
		   }
		}

		MET_delete_dependencies(tdbb, domainName, type, transaction);
		store_dependencies(tdbb, csb, relation, domainName, type, transaction);
	}
	else
	{
		MET_delete_dependencies(tdbb, object_name, type, transaction);
		store_dependencies(tdbb, csb, relation, object_name, type, transaction);
	}

	if (csb_ptr)
		*csb_ptr = csb;
	else
		delete csb;

	return node;
}


jrd_fld* MET_get_field(const jrd_rel* relation, USHORT id)
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
		return NULL;

	return (*vector)[id];
}


void MET_get_shadow_files(thread_db* tdbb, bool delete_files)
{
   struct {
          TEXT  jrd_428 [256];	// RDB$FILE_NAME 
          SSHORT jrd_429;	// gds__utility 
          SSHORT jrd_430;	// RDB$SHADOW_NUMBER 
          SSHORT jrd_431;	// RDB$FILE_FLAGS 
   } jrd_427;
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
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	AutoRequest handle;

	/*FOR(REQUEST_HANDLE handle) X IN RDB$FILES
		WITH X.RDB$SHADOW_NUMBER NOT MISSING
			AND X.RDB$SHADOW_NUMBER NE 0
			AND X.RDB$FILE_SEQUENCE EQ 0*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_426, sizeof(jrd_426));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 262, (UCHAR*) &jrd_427);
	   if (!jrd_427.jrd_429) break;
	{
		if ((/*X.RDB$FILE_FLAGS*/
		     jrd_427.jrd_431 & FILE_shadow) && !(/*X.RDB$FILE_FLAGS*/
		     jrd_427.jrd_431 & FILE_inactive))
		{
			const USHORT file_flags = /*X.RDB$FILE_FLAGS*/
						  jrd_427.jrd_431;
			SDW_start(tdbb, /*X.RDB$FILE_NAME*/
					jrd_427.jrd_428, /*X.RDB$SHADOW_NUMBER*/
  jrd_427.jrd_430, file_flags, delete_files);

			// if the shadow exists, mark the appropriate shadow
			// block as found for the purposes of this routine;
			// if the shadow was conditional and is no longer, note it

			for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
			{
				if ((shadow->sdw_number == /*X.RDB$SHADOW_NUMBER*/
							   jrd_427.jrd_430) && !(shadow->sdw_flags & SDW_IGNORE))
				{
					shadow->sdw_flags |= SDW_found;
					if (!(file_flags & FILE_conditional)) {
						shadow->sdw_flags &= ~SDW_conditional;
					}
					break;
				}
			}
		}
	}
	/*END_FOR*/
	   }
	}

	// if any current shadows were not defined in database, mark
	// them to be shutdown since they don't exist anymore

	for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
	{
		if (!(shadow->sdw_flags & SDW_found))
			shadow->sdw_flags |= SDW_shutdown;
		else
			shadow->sdw_flags &= ~SDW_found;
	}

	SDW_check(tdbb);
}


void MET_load_db_triggers(thread_db* tdbb, int type)
{
   struct {
          TEXT  jrd_424 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_425;	// gds__utility 
   } jrd_423;
   struct {
          ISC_INT64  jrd_422;	// RDB$TRIGGER_TYPE 
   } jrd_421;
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
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	if ((attachment->att_flags & ATT_no_db_triggers) ||
		attachment->att_triggers[type] != NULL)
	{
		return;
	}

	attachment->att_triggers[type] = FB_NEW(*attachment->att_pool)
		trig_vec(*attachment->att_pool);

	AutoRequest trigger_request;
	int encoded_type = type | TRIGGER_TYPE_DB;

	/*FOR(REQUEST_HANDLE trigger_request)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME MISSING AND
			 TRG.RDB$TRIGGER_TYPE EQ encoded_type AND
			 TRG.RDB$TRIGGER_INACTIVE EQ 0
		SORTED BY TRG.RDB$TRIGGER_SEQUENCE*/
	{
	trigger_request.compile(tdbb, (UCHAR*) jrd_420, sizeof(jrd_420));
	jrd_421.jrd_422 = encoded_type;
	EXE_start (tdbb, trigger_request, attachment->getSysTransaction());
	EXE_send (tdbb, trigger_request, 0, 8, (UCHAR*) &jrd_421);
	while (1)
	   {
	   EXE_receive (tdbb, trigger_request, 1, 34, (UCHAR*) &jrd_423);
	   if (!jrd_423.jrd_425) break;
	{
		MET_load_trigger(tdbb, NULL, /*TRG.RDB$TRIGGER_NAME*/
					     jrd_423.jrd_424, &attachment->att_triggers[type]);
	}
	/*END_FOR*/
	   }
	}
}


// Load DDL triggers from RDB$TRIGGERS.
void MET_load_ddl_triggers(thread_db* tdbb)
{
   struct {
          TEXT  jrd_417 [32];	// RDB$TRIGGER_NAME 
          ISC_INT64  jrd_418;	// RDB$TRIGGER_TYPE 
          SSHORT jrd_419;	// gds__utility 
   } jrd_416;
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	if ((attachment->att_flags & ATT_no_db_triggers) ||
		attachment->att_ddl_triggers != NULL)
	{
		return;
	}

	attachment->att_ddl_triggers = FB_NEW(*tdbb->getDatabase()->dbb_permanent)
		trig_vec(*tdbb->getDatabase()->dbb_permanent);

	AutoRequest trigger_request;

	/*FOR(REQUEST_HANDLE trigger_request)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME MISSING AND
			 TRG.RDB$TRIGGER_INACTIVE EQ 0
		SORTED BY TRG.RDB$TRIGGER_SEQUENCE*/
	{
	trigger_request.compile(tdbb, (UCHAR*) jrd_415, sizeof(jrd_415));
	EXE_start (tdbb, trigger_request, attachment->getSysTransaction());
	while (1)
	   {
	   EXE_receive (tdbb, trigger_request, 0, 42, (UCHAR*) &jrd_416);
	   if (!jrd_416.jrd_419) break;
	{
		if ((/*TRG.RDB$TRIGGER_TYPE*/
		     jrd_416.jrd_418 & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DDL)
		{
			MET_load_trigger(tdbb, NULL, /*TRG.RDB$TRIGGER_NAME*/
						     jrd_416.jrd_417,
				&attachment->att_ddl_triggers);
		}
	}
	/*END_FOR*/
	   }
	}
}


void MET_load_trigger(thread_db* tdbb,
					  jrd_rel* relation,
					  const MetaName& trigger_name,
					  trig_vec** triggers)
{
   struct {
          TEXT  jrd_400 [32];	// RDB$TRIGGER_NAME 
          bid  jrd_401;	// RDB$TRIGGER_BLR 
          ISC_INT64  jrd_402;	// RDB$TRIGGER_TYPE 
          TEXT  jrd_403 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_404 [256];	// RDB$ENTRYPOINT 
          bid  jrd_405;	// RDB$TRIGGER_SOURCE 
          TEXT  jrd_406 [32];	// RDB$ENGINE_NAME 
          bid  jrd_407;	// RDB$DEBUG_INFO 
          SSHORT jrd_408;	// gds__utility 
          SSHORT jrd_409;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_410;	// gds__null_flag 
          SSHORT jrd_411;	// gds__null_flag 
          SSHORT jrd_412;	// gds__null_flag 
          SSHORT jrd_413;	// gds__null_flag 
          SSHORT jrd_414;	// RDB$FLAGS 
   } jrd_399;
   struct {
          TEXT  jrd_398 [32];	// RDB$TRIGGER_NAME 
   } jrd_397;
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
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	if (relation)
	{
		if (relation->rel_flags & REL_sys_trigs_being_loaded)
			return;

		// No need to load table triggers for ReadOnly databases,
		// since INSERT/DELETE/UPDATE statements are not going to be allowed
		// hvlad: GTT with ON COMMIT DELETE ROWS clause is writable

		if (dbb->readOnly() && !(relation->rel_flags & REL_temp_tran))
			return;
	}

	// Scan RDB$TRIGGERS next

	AutoCacheRequest request(tdbb, irq_s_triggers, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$TRIGGER_NAME EQ trigger_name.c_str() AND
			 (TRG.RDB$TRIGGER_INACTIVE MISSING OR TRG.RDB$TRIGGER_INACTIVE EQ 0)*/
	{
	request.compile(tdbb, (UCHAR*) jrd_396, sizeof(jrd_396));
	gds__vtov ((const char*) trigger_name.c_str(), (char*) jrd_397.jrd_398, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_397);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 398, (UCHAR*) &jrd_399);
	   if (!jrd_399.jrd_408) break;
	{
		// check if the trigger is to be fired without any permissions
		// checks. Verify such a claim
		USHORT trig_flags = (USHORT) /*TRG.RDB$FLAGS*/
					     jrd_399.jrd_414;

		// if there is an ignore permission flag, see if it is legit
		if ((/*TRG.RDB$FLAGS*/
		     jrd_399.jrd_414 & TRG_ignore_perm) && !verify_TRG_ignore_perm(tdbb, trigger_name))
		{
			fb_msg_format(NULL, JRD_BUGCHK, 304, sizeof(errmsg),
							errmsg, MsgFormat::SafeArg() << trigger_name.c_str());
			ERR_log(JRD_BUGCHK, 304, errmsg);

			trig_flags &= ~TRG_ignore_perm;
		}

		bid debug_blob_id;
		debug_blob_id.clear();

		bid extBodyId;
		extBodyId.clear();

		if (!/*TRG.RDB$DEBUG_INFO.NULL*/
		     jrd_399.jrd_413)	// ODS_11_1
			debug_blob_id = /*TRG.RDB$DEBUG_INFO*/
					jrd_399.jrd_407;

		MetaName engine;
		string entryPoint;

		if (!/*TRG.RDB$ENGINE_NAME.NULL*/
		     jrd_399.jrd_412)	// ODS_12_0
		{
			engine = /*TRG.RDB$ENGINE_NAME*/
				 jrd_399.jrd_406;
			extBodyId = /*TRG.RDB$TRIGGER_SOURCE*/
				    jrd_399.jrd_405;
		}

		if (!/*TRG.RDB$ENTRYPOINT.NULL*/
		     jrd_399.jrd_411)	// ODS_12_0
			entryPoint = /*TRG.RDB$ENTRYPOINT*/
				     jrd_399.jrd_404;

		if (/*TRG.RDB$RELATION_NAME.NULL*/
		    jrd_399.jrd_410)
		{
			if ((/*TRG.RDB$TRIGGER_TYPE*/
			     jrd_399.jrd_402 & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB ||
				(/*TRG.RDB$TRIGGER_TYPE*/
				 jrd_399.jrd_402 & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DDL)
			{
				// this is a database trigger
				get_trigger(tdbb,
							relation,
							&/*TRG.RDB$TRIGGER_BLR*/
							 jrd_399.jrd_401,
							&debug_blob_id,
							triggers,
							/*TRG.RDB$TRIGGER_NAME*/
							jrd_399.jrd_400,
							/*TRG.RDB$TRIGGER_TYPE*/
							jrd_399.jrd_402 & ~TRIGGER_TYPE_MASK,
							(bool) /*TRG.RDB$SYSTEM_FLAG*/
							       jrd_399.jrd_409,
							trig_flags,
							engine,
							entryPoint,
							&extBodyId);
			}
		}
		else
		{
			// dimitr: support for the universal triggers
			int trigger_action, slot_index = 0;
			while ((trigger_action = TRIGGER_ACTION_SLOT(/*TRG.RDB$TRIGGER_TYPE*/
								     jrd_399.jrd_402, ++slot_index)) > 0)
			{
				get_trigger(tdbb,
							relation,
							&/*TRG.RDB$TRIGGER_BLR*/
							 jrd_399.jrd_401,
							&debug_blob_id,
							triggers + trigger_action,
							/*TRG.RDB$TRIGGER_NAME*/
							jrd_399.jrd_400,
							(UCHAR) trigger_action,
							(bool) /*TRG.RDB$SYSTEM_FLAG*/
							       jrd_399.jrd_409,
							trig_flags,
							engine,
							entryPoint,
							&extBodyId);
			}
		}
	}
	/*END_FOR*/
	   }
	}
}



void MET_lookup_cnstrt_for_index(thread_db* tdbb,
								 MetaName& constraint_name,
								 const MetaName& index_name)
{
   struct {
          TEXT  jrd_394 [32];	// RDB$CONSTRAINT_NAME 
          SSHORT jrd_395;	// gds__utility 
   } jrd_393;
   struct {
          TEXT  jrd_392 [32];	// RDB$INDEX_NAME 
   } jrd_391;
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
	Attachment* attachment = tdbb->getAttachment();

	constraint_name = "";
	AutoCacheRequest request(tdbb, irq_l_cnstrt, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$RELATION_CONSTRAINTS WITH X.RDB$INDEX_NAME EQ index_name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_390, sizeof(jrd_390));
	gds__vtov ((const char*) index_name.c_str(), (char*) jrd_391.jrd_392, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_391);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_393);
	   if (!jrd_393.jrd_395) break;
	{
		constraint_name = /*X.RDB$CONSTRAINT_NAME*/
				  jrd_393.jrd_394;
	}
	/*END_FOR*/
	   }
	}
}


void MET_lookup_cnstrt_for_trigger(thread_db* tdbb,
								   MetaName& constraint_name,
								   MetaName& relation_name,
								   const MetaName& trigger_name)
{
   struct {
          TEXT  jrd_381 [32];	// RDB$CONSTRAINT_NAME 
          SSHORT jrd_382;	// gds__utility 
   } jrd_380;
   struct {
          TEXT  jrd_379 [32];	// RDB$TRIGGER_NAME 
   } jrd_378;
   struct {
          TEXT  jrd_387 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_388 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_389;	// gds__utility 
   } jrd_386;
   struct {
          TEXT  jrd_385 [32];	// RDB$TRIGGER_NAME 
   } jrd_384;
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
	Attachment* attachment = tdbb->getAttachment();

	constraint_name = "";
	relation_name = "";
	AutoCacheRequest request(tdbb, irq_l_check, IRQ_REQUESTS);
	AutoCacheRequest request2(tdbb, irq_l_check2, IRQ_REQUESTS);

	// utilize two requests rather than one so that we
	// guarantee we always return the name of the relation
	// that the trigger is defined on, even if we don't
	// have a check constraint defined for that trigger

	/*FOR(REQUEST_HANDLE request)
		Y IN RDB$TRIGGERS WITH
			Y.RDB$TRIGGER_NAME EQ trigger_name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_383, sizeof(jrd_383));
	gds__vtov ((const char*) trigger_name.c_str(), (char*) jrd_384.jrd_385, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_384);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_386);
	   if (!jrd_386.jrd_389) break;
	{
		/*FOR(REQUEST_HANDLE request2)
			X IN RDB$CHECK_CONSTRAINTS WITH
				X.RDB$TRIGGER_NAME EQ Y.RDB$TRIGGER_NAME*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_377, sizeof(jrd_377));
		gds__vtov ((const char*) jrd_386.jrd_388, (char*) jrd_378.jrd_379, 32);
		EXE_start (tdbb, request2, attachment->getSysTransaction());
		EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_378);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 34, (UCHAR*) &jrd_380);
		   if (!jrd_380.jrd_382) break;
		{
			constraint_name = /*X.RDB$CONSTRAINT_NAME*/
					  jrd_380.jrd_381;
		}
		/*END_FOR*/
		   }
		}

		relation_name = /*Y.RDB$RELATION_NAME*/
				jrd_386.jrd_387;
	}
	/*END_FOR*/
	   }
	}
}


void MET_lookup_exception(thread_db* tdbb,
						  SLONG number,
						  MetaName& name,
						  string* message)
{
   struct {
          TEXT  jrd_372 [1024];	// RDB$MESSAGE 
          TEXT  jrd_373 [32];	// RDB$EXCEPTION_NAME 
          SSHORT jrd_374;	// gds__utility 
          SSHORT jrd_375;	// gds__null_flag 
          SSHORT jrd_376;	// gds__null_flag 
   } jrd_371;
   struct {
          SLONG  jrd_370;	// RDB$EXCEPTION_NUMBER 
   } jrd_369;
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
	Attachment* attachment = tdbb->getAttachment();

	// We need to look up exception in RDB$EXCEPTIONS

	AutoCacheRequest request(tdbb, irq_l_exception, IRQ_REQUESTS);

	name = "";
	if (message)
		*message = "";

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$EXCEPTIONS WITH X.RDB$EXCEPTION_NUMBER = number*/
	{
	request.compile(tdbb, (UCHAR*) jrd_368, sizeof(jrd_368));
	jrd_369.jrd_370 = number;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_369);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 1062, (UCHAR*) &jrd_371);
	   if (!jrd_371.jrd_374) break;
	{
		if (!/*X.RDB$EXCEPTION_NAME.NULL*/
		     jrd_371.jrd_376)
			name = /*X.RDB$EXCEPTION_NAME*/
			       jrd_371.jrd_373;

		if (!/*X.RDB$MESSAGE.NULL*/
		     jrd_371.jrd_375 && message)
			*message = /*X.RDB$MESSAGE*/
				   jrd_371.jrd_372;
	}
	/*END_FOR*/
	   }
	}
}


bool MET_load_exception(thread_db* tdbb, ExceptionItem& item)
{
   struct {
          TEXT  jrd_365 [32];	// RDB$SECURITY_CLASS 
          SLONG  jrd_366;	// RDB$EXCEPTION_NUMBER 
          SSHORT jrd_367;	// gds__utility 
   } jrd_364;
   struct {
          TEXT  jrd_363 [32];	// RDB$EXCEPTION_NAME 
   } jrd_362;
/**************************************
 *
 *      M E T _ l o a d _ e x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *      Lookup exception by name and fill the passed instance.
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();

	// We need to look up exception in RDB$EXCEPTIONS

	AutoCacheRequest request(tdbb, irq_l_except_no, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$EXCEPTIONS WITH X.RDB$EXCEPTION_NAME = item.name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_361, sizeof(jrd_361));
	gds__vtov ((const char*) item.name.c_str(), (char*) jrd_362.jrd_363, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_362);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 38, (UCHAR*) &jrd_364);
	   if (!jrd_364.jrd_367) break;
	{
		item.type = ExceptionItem::XCP_CODE;
		item.code = /*X.RDB$EXCEPTION_NUMBER*/
			    jrd_364.jrd_366;
		item.secName = /*X.RDB$SECURITY_CLASS*/
			       jrd_364.jrd_365;

		return true;
	}
	/*END_FOR*/
	   }
	}

	return false;
}


int MET_lookup_field(thread_db* tdbb, jrd_rel* relation, const MetaName& name)
{
   struct {
          SSHORT jrd_359;	// gds__utility 
          SSHORT jrd_360;	// RDB$FIELD_ID 
   } jrd_358;
   struct {
          TEXT  jrd_356 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_357 [32];	// RDB$RELATION_NAME 
   } jrd_355;
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
	Attachment* attachment = tdbb->getAttachment();

	// Start by checking field names that we already know
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

	// Not found.  Next, try system relations directly

	int id = -1;

	if (relation->rel_flags & REL_deleted)
	{
		return id;
	}

	AutoCacheRequest request(tdbb, irq_l_field, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$RELATION_FIELDS WITH
			X.RDB$RELATION_NAME EQ relation->rel_name.c_str() AND
			X.RDB$FIELD_ID NOT MISSING AND
			X.RDB$FIELD_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_354, sizeof(jrd_354));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_355.jrd_356, 32);
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_355.jrd_357, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_355);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_358);
	   if (!jrd_358.jrd_359) break;
	{
		id = /*X.RDB$FIELD_ID*/
		     jrd_358.jrd_360;
	}
	/*END_FOR*/
	   }
	}

	return id;
}


BlobFilter* MET_lookup_filter(thread_db* tdbb, SSHORT from, SSHORT to)
{
   struct {
          TEXT  jrd_350 [32];	// RDB$FUNCTION_NAME 
          TEXT  jrd_351 [256];	// RDB$ENTRYPOINT 
          TEXT  jrd_352 [256];	// RDB$MODULE_NAME 
          SSHORT jrd_353;	// gds__utility 
   } jrd_349;
   struct {
          SSHORT jrd_347;	// RDB$OUTPUT_SUB_TYPE 
          SSHORT jrd_348;	// RDB$INPUT_SUB_TYPE 
   } jrd_346;
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
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	FPTR_BFILTER_CALLBACK filter = NULL;
	BlobFilter* blf = NULL;

	AutoCacheRequest request(tdbb, irq_r_filters, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$FILTERS WITH X.RDB$INPUT_SUB_TYPE EQ from AND
		X.RDB$OUTPUT_SUB_TYPE EQ to*/
	{
	request.compile(tdbb, (UCHAR*) jrd_345, sizeof(jrd_345));
	jrd_346.jrd_347 = to;
	jrd_346.jrd_348 = from;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_346);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 546, (UCHAR*) &jrd_349);
	   if (!jrd_349.jrd_353) break;
	{
		filter = (FPTR_BFILTER_CALLBACK)
			Module::lookup(/*X.RDB$MODULE_NAME*/
				       jrd_349.jrd_352, /*X.RDB$ENTRYPOINT*/
  jrd_349.jrd_351, dbb->dbb_modules);
		if (filter)
		{
			blf = FB_NEW(*dbb->dbb_permanent) BlobFilter(*dbb->dbb_permanent);
			blf->blf_next = NULL;
			blf->blf_from = from;
			blf->blf_to = to;
			blf->blf_filter = filter;
			blf->blf_exception_message.printf(EXCEPTION_MESSAGE,
					/*X.RDB$FUNCTION_NAME*/
					jrd_349.jrd_350, /*X.RDB$ENTRYPOINT*/
  jrd_349.jrd_351, /*X.RDB$MODULE_NAME*/
  jrd_349.jrd_352);
		}
	}
	/*END_FOR*/
	   }
	}

	return blf;
}


bool MET_load_generator(thread_db* tdbb, GeneratorItem& item, bool* sysGen, SLONG* step)
{
   struct {
          TEXT  jrd_340 [32];	// RDB$SECURITY_CLASS 
          SLONG  jrd_341;	// RDB$GENERATOR_INCREMENT 
          SSHORT jrd_342;	// gds__utility 
          SSHORT jrd_343;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_344;	// RDB$GENERATOR_ID 
   } jrd_339;
   struct {
          TEXT  jrd_338 [32];	// RDB$GENERATOR_NAME 
   } jrd_337;
/**************************************
 *
 *      M E T _ l o a d _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *      Lookup generator ID by its name and load its metadata into the passed object.
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();

	if (item.name == MASTER_GENERATOR)
	{
		item.id = 0;
		if (sysGen)
			*sysGen = true;
		if (step)
			*step = 1;
		return true;
	}

	AutoCacheRequest request(tdbb, irq_r_gen_id, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$GENERATORS WITH X.RDB$GENERATOR_NAME EQ item.name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_336, sizeof(jrd_336));
	gds__vtov ((const char*) item.name.c_str(), (char*) jrd_337.jrd_338, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_337);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 42, (UCHAR*) &jrd_339);
	   if (!jrd_339.jrd_342) break;
	{
		item.id = /*X.RDB$GENERATOR_ID*/
			  jrd_339.jrd_344;
		item.secName = /*X.RDB$SECURITY_CLASS*/
			       jrd_339.jrd_340;
		if (sysGen)
			*sysGen = (/*X.RDB$SYSTEM_FLAG*/
				   jrd_339.jrd_343 == fb_sysflag_system);
		if (step)
			*step = /*X.RDB$GENERATOR_INCREMENT*/
				jrd_339.jrd_341;

		return true;
	}
	/*END_FOR*/
	   }
	}

	return false;
}

SLONG MET_lookup_generator(thread_db* tdbb, const MetaName& name, bool* sysGen, SLONG* step)
{
   struct {
          SLONG  jrd_332;	// RDB$GENERATOR_INCREMENT 
          SSHORT jrd_333;	// gds__utility 
          SSHORT jrd_334;	// RDB$GENERATOR_ID 
          SSHORT jrd_335;	// RDB$SYSTEM_FLAG 
   } jrd_331;
   struct {
          TEXT  jrd_330 [32];	// RDB$GENERATOR_NAME 
   } jrd_329;
/**************************************
 *
 *      M E T _ l o o k u p _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *      Lookup generator ID by its name.
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();

	if (name == MASTER_GENERATOR)
	{
		if (sysGen)
			*sysGen = true;
		if (step)
			*step = 1;
		return 0;
	}

	AutoCacheRequest request(tdbb, irq_l_gen_id, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$GENERATORS WITH X.RDB$GENERATOR_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_328, sizeof(jrd_328));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_329.jrd_330, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_329);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 10, (UCHAR*) &jrd_331);
	   if (!jrd_331.jrd_333) break;
	{
		if (sysGen)
			*sysGen = (/*X.RDB$SYSTEM_FLAG*/
				   jrd_331.jrd_335 == fb_sysflag_system);
		if (step)
			*step = /*X.RDB$GENERATOR_INCREMENT*/
				jrd_331.jrd_332;

		return /*X.RDB$GENERATOR_ID*/
		       jrd_331.jrd_334;
	}
	/*END_FOR*/
	   }
	}

	return -1;
}

bool MET_lookup_generator_id(thread_db* tdbb, SLONG gen_id, MetaName& name, bool* sysGen)
{
   struct {
          TEXT  jrd_325 [32];	// RDB$GENERATOR_NAME 
          SSHORT jrd_326;	// gds__utility 
          SSHORT jrd_327;	// RDB$SYSTEM_FLAG 
   } jrd_324;
   struct {
          SSHORT jrd_323;	// RDB$GENERATOR_ID 
   } jrd_322;
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
	Attachment* attachment = tdbb->getAttachment();

	fb_assert(gen_id != 0);

	name = "";

	AutoCacheRequest request(tdbb, irq_r_gen_id_num, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		X IN RDB$GENERATORS WITH X.RDB$GENERATOR_ID EQ gen_id*/
	{
	request.compile(tdbb, (UCHAR*) jrd_321, sizeof(jrd_321));
	jrd_322.jrd_323 = gen_id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_322);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_324);
	   if (!jrd_324.jrd_326) break;
	{
		if (sysGen)
			*sysGen = (/*X.RDB$SYSTEM_FLAG*/
				   jrd_324.jrd_327 == fb_sysflag_system);

		name = /*X.RDB$GENERATOR_NAME*/
		       jrd_324.jrd_325;
	}
	/*END_FOR*/
	   }
	}

	return name.hasData();
}

void MET_update_generator_increment(thread_db* tdbb, SLONG gen_id, SLONG step)
{
   struct {
          SSHORT jrd_320;	// gds__utility 
   } jrd_319;
   struct {
          SLONG  jrd_318;	// RDB$GENERATOR_INCREMENT 
   } jrd_317;
   struct {
          SLONG  jrd_314;	// RDB$GENERATOR_INCREMENT 
          SSHORT jrd_315;	// gds__utility 
          SSHORT jrd_316;	// RDB$SYSTEM_FLAG 
   } jrd_313;
   struct {
          SSHORT jrd_312;	// RDB$GENERATOR_ID 
   } jrd_311;
/**************************************
 *
 *      M E T _ u p d a t e _ g e n e r a t o r _ i n c r e m e n t
 *
 **************************************
 *
 * Functional description
 *      Update the step in a generator searched by ID.
 *		This function is for legacy code "SET GENERATOR TO value" only!
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, irq_upd_gen_id_increm, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		X IN RDB$GENERATORS WITH X.RDB$GENERATOR_ID EQ gen_id*/
	{
	request.compile(tdbb, (UCHAR*) jrd_310, sizeof(jrd_310));
	jrd_311.jrd_312 = gen_id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_311);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 8, (UCHAR*) &jrd_313);
	   if (!jrd_313.jrd_315) break;

		// We never accept changing the step in sys gens.
		if (/*X.RDB$SYSTEM_FLAG*/
		    jrd_313.jrd_316 == fb_sysflag_system)
			return;

		/*MODIFY X*/
		{
		
			/*X.RDB$GENERATOR_INCREMENT*/
			jrd_313.jrd_314 = step;
		/*END_MODIFY*/
		jrd_317.jrd_318 = jrd_313.jrd_314;
		EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_317);
		}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_319);
	   }
	}
}


void MET_lookup_index(thread_db* tdbb,
						MetaName& index_name,
						const MetaName& relation_name,
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
	Attachment* attachment = tdbb->getAttachment();

	index_name = "";

	AutoCacheRequest request(tdbb, irq_l_index, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$INDICES WITH X.RDB$RELATION_NAME EQ relation_name.c_str()
			AND X.RDB$INDEX_ID EQ number*/
	{
	request.compile(tdbb, (UCHAR*) jrd_303, sizeof(jrd_303));
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_304.jrd_305, 32);
	jrd_304.jrd_306 = number;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_304);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_307);
	   if (!jrd_307.jrd_309) break;
	{
		index_name = /*X.RDB$INDEX_NAME*/
			     jrd_307.jrd_308;
	}
	/*END_FOR*/
	   }
	}
}


SLONG MET_lookup_index_name(thread_db* tdbb,
							const MetaName& index_name,
							SLONG* relation_id, IndexStatus* status)
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
	Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, irq_l_index_name, IRQ_REQUESTS);

	*status = MET_object_unknown;

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$INDICES WITH
			X.RDB$INDEX_NAME EQ index_name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_295, sizeof(jrd_295));
	gds__vtov ((const char*) index_name.c_str(), (char*) jrd_296.jrd_297, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_296);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 38, (UCHAR*) &jrd_298);
	   if (!jrd_298.jrd_300) break;
	{
		if (/*X.RDB$INDEX_INACTIVE*/
		    jrd_298.jrd_302 == 0)
			*status = MET_object_active;
		else
			*status = MET_object_inactive;

		id = /*X.RDB$INDEX_ID*/
		     jrd_298.jrd_301 - 1;
		const jrd_rel* relation = MET_lookup_relation(tdbb, /*X.RDB$RELATION_NAME*/
								    jrd_298.jrd_299);
		*relation_id = relation->rel_id;
	}
	/*END_FOR*/
	   }
	}

	return id;
}


bool MET_lookup_partner(thread_db* tdbb, jrd_rel* relation, index_desc* idx, const TEXT* index_name)
{
   struct {
          TEXT  jrd_290 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_291;	// gds__utility 
          SSHORT jrd_292;	// RDB$INDEX_ID 
          SSHORT jrd_293;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_294;	// RDB$INDEX_INACTIVE 
   } jrd_289;
   struct {
          TEXT  jrd_286 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_287 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_288;	// RDB$INDEX_ID 
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
	Attachment* attachment = tdbb->getAttachment();

	if (relation->rel_flags & REL_check_partners) {
		scan_partners(tdbb, relation);
	}

	if (idx->idx_flags & idx_foreign)
	{
		if (index_name)
		{
			// Since primary key index names aren't being cached, do a long
			// hard lookup. This is only called during index create for foreign keys.

			bool found = false;
			AutoRequest request;

			/*FOR(REQUEST_HANDLE request)
				IDX IN RDB$INDICES CROSS
					IND IN RDB$INDICES WITH
					IDX.RDB$RELATION_NAME EQ relation->rel_name.c_str() AND
					(IDX.RDB$INDEX_ID EQ idx->idx_id + 1 OR
					 IDX.RDB$INDEX_NAME EQ index_name) AND
					IND.RDB$INDEX_NAME EQ IDX.RDB$FOREIGN_KEY AND
					IND.RDB$UNIQUE_FLAG = 1*/
			{
			request.compile(tdbb, (UCHAR*) jrd_284, sizeof(jrd_284));
			gds__vtov ((const char*) index_name, (char*) jrd_285.jrd_286, 32);
			gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_285.jrd_287, 32);
			jrd_285.jrd_288 = idx->idx_id;
			EXE_start (tdbb, request, attachment->getSysTransaction());
			EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_285);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 40, (UCHAR*) &jrd_289);
			   if (!jrd_289.jrd_291) break;
			{
				//// ASF: Hack fix for CORE-4304, until nasty interactions between dfw and met are not resolved.
				const jrd_rel* partner_relation = relation->rel_name == /*IND.RDB$RELATION_NAME*/
											jrd_289.jrd_290 ?
					relation : MET_lookup_relation(tdbb, /*IND.RDB$RELATION_NAME*/
									     jrd_289.jrd_290);

				if (partner_relation && !/*IDX.RDB$INDEX_INACTIVE*/
							 jrd_289.jrd_294 && !/*IND.RDB$INDEX_INACTIVE*/
     jrd_289.jrd_293)
				{
					idx->idx_primary_relation = partner_relation->rel_id;
					idx->idx_primary_index = /*IND.RDB$INDEX_ID*/
								 jrd_289.jrd_292 - 1;
					fb_assert(idx->idx_primary_index != idx_invalid);
					found = true;
				}
			}
			/*END_FOR*/
			   }
			}

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


jrd_prc* MET_lookup_procedure(thread_db* tdbb, const QualifiedName& name, bool noscan)
{
   struct {
          SSHORT jrd_282;	// gds__utility 
          SSHORT jrd_283;	// RDB$PROCEDURE_ID 
   } jrd_281;
   struct {
          TEXT  jrd_279 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_280 [32];	// RDB$PROCEDURE_NAME 
   } jrd_278;
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
	Attachment* attachment = tdbb->getAttachment();
	jrd_prc* check_procedure = NULL;

	// See if we already know the procedure by name
	for (jrd_prc** iter = attachment->att_procedures.begin(); iter != attachment->att_procedures.end(); ++iter)
	{
		jrd_prc* procedure = *iter;

		if (procedure && !(procedure->flags & Routine::FLAG_OBSOLETE) &&
			((procedure->flags & Routine::FLAG_SCANNED) || noscan) &&
			!(procedure->flags & Routine::FLAG_BEING_SCANNED) &&
			!(procedure->flags & Routine::FLAG_BEING_ALTERED))
		{
			if (procedure->getName() == name)
			{
				if (procedure->flags & Routine::FLAG_CHECK_EXISTENCE)
				{
					check_procedure = procedure;
					LCK_lock(tdbb, check_procedure->existenceLock, LCK_SR, LCK_WAIT);
					break;
				}

				return procedure;
			}
		}
	}

	// We need to look up the procedure name in RDB$PROCEDURES

	jrd_prc* procedure = NULL;

	AutoCacheRequest request(tdbb, irq_l_procedure, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		P IN RDB$PROCEDURES
		WITH P.RDB$PROCEDURE_NAME EQ name.identifier.c_str() AND
			 P.RDB$PACKAGE_NAME EQUIV NULLIF(name.package.c_str(), '')*/
	{
	request.compile(tdbb, (UCHAR*) jrd_277, sizeof(jrd_277));
	gds__vtov ((const char*) name.package.c_str(), (char*) jrd_278.jrd_279, 32);
	gds__vtov ((const char*) name.identifier.c_str(), (char*) jrd_278.jrd_280, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_278);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_281);
	   if (!jrd_281.jrd_282) break;
	{
		procedure = MET_procedure(tdbb, /*P.RDB$PROCEDURE_ID*/
						jrd_281.jrd_283, noscan, 0);
	}
	/*END_FOR*/
	   }
	}

	if (check_procedure)
	{
		check_procedure->flags &= ~Routine::FLAG_CHECK_EXISTENCE;
		if (check_procedure != procedure)
		{
			LCK_release(tdbb, check_procedure->existenceLock);
			check_procedure->flags |= Routine::FLAG_OBSOLETE;
		}
	}

	return procedure;
}


jrd_prc* MET_lookup_procedure_id(thread_db* tdbb, USHORT id,
								 bool return_deleted, bool noscan, USHORT flags)
{
   struct {
          SSHORT jrd_275;	// gds__utility 
          SSHORT jrd_276;	// RDB$PROCEDURE_ID 
   } jrd_274;
   struct {
          SSHORT jrd_273;	// RDB$PROCEDURE_ID 
   } jrd_272;
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
	Attachment* attachment = tdbb->getAttachment();
	jrd_prc* check_procedure = NULL;

	jrd_prc* procedure;

	if (id < (USHORT) attachment->att_procedures.getCount() && (procedure = attachment->att_procedures[id]) &&
		procedure->getId() == id &&
		!(procedure->flags & Routine::FLAG_BEING_SCANNED) &&
		((procedure->flags & Routine::FLAG_SCANNED) || noscan) &&
		!(procedure->flags & Routine::FLAG_BEING_ALTERED) &&
		(!(procedure->flags & Routine::FLAG_OBSOLETE) || return_deleted))
	{
		if (procedure->flags & Routine::FLAG_CHECK_EXISTENCE)
		{
			check_procedure = procedure;
			LCK_lock(tdbb, check_procedure->existenceLock, LCK_SR, LCK_WAIT);
		}
		else {
			return procedure;
		}
	}

	// We need to look up the procedure name in RDB$PROCEDURES

	procedure = NULL;

	AutoCacheRequest request(tdbb, irq_l_proc_id, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		P IN RDB$PROCEDURES WITH P.RDB$PROCEDURE_ID EQ id*/
	{
	request.compile(tdbb, (UCHAR*) jrd_271, sizeof(jrd_271));
	jrd_272.jrd_273 = id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_272);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_274);
	   if (!jrd_274.jrd_275) break;
	{
		procedure = MET_procedure(tdbb, /*P.RDB$PROCEDURE_ID*/
						jrd_274.jrd_276, noscan, flags);
	}
	/*END_FOR*/
	   }
	}

	if (check_procedure)
	{
		check_procedure->flags &= ~Routine::FLAG_CHECK_EXISTENCE;
		if (check_procedure != procedure)
		{
			LCK_release(tdbb, check_procedure->existenceLock);
			check_procedure->flags |= Routine::FLAG_OBSOLETE;
		}
	}

	return procedure;
}


jrd_rel* MET_lookup_relation(thread_db* tdbb, const MetaName& name)
{
   struct {
          SSHORT jrd_266;	// gds__utility 
          SSHORT jrd_267;	// gds__null_flag 
          SSHORT jrd_268;	// RDB$RELATION_TYPE 
          SSHORT jrd_269;	// RDB$FLAGS 
          SSHORT jrd_270;	// RDB$RELATION_ID 
   } jrd_265;
   struct {
          TEXT  jrd_264 [32];	// RDB$RELATION_NAME 
   } jrd_263;
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
	Attachment* attachment = tdbb->getAttachment();

	// See if we already know the relation by name

	vec<jrd_rel*>* relations = attachment->att_relations;
	jrd_rel* check_relation = NULL;

	vec<jrd_rel*>::iterator ptr = relations->begin();
	for (const vec<jrd_rel*>::const_iterator end = relations->end(); ptr < end; ++ptr)
	{
		jrd_rel* const relation = *ptr;

		if (relation)
		{
			if (relation->rel_flags & REL_deleting)
				Attachment::CheckoutLockGuard guard(attachment, relation->rel_drop_mutex, FB_FUNCTION);

			if (!(relation->rel_flags & REL_deleted))
			{
				// dimitr: for non-system relations we should also check
				//		   REL_scanned and REL_being_scanned flags. Look
				//		   at MET_lookup_procedure for example.
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

	// We need to look up the relation name in RDB$RELATIONS

	jrd_rel* relation = NULL;

	AutoCacheRequest request(tdbb, irq_l_relation, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$RELATIONS WITH X.RDB$RELATION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_262, sizeof(jrd_262));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_263.jrd_264, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_263);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 10, (UCHAR*) &jrd_265);
	   if (!jrd_265.jrd_266) break;
	{
		relation = MET_relation(tdbb, /*X.RDB$RELATION_ID*/
					      jrd_265.jrd_270);
		if (relation->rel_name.length() == 0) {
			relation->rel_name = name;
		}

		relation->rel_flags |= get_rel_flags_from_FLAGS(/*X.RDB$FLAGS*/
								jrd_265.jrd_269);

		if (!/*X.RDB$RELATION_TYPE.NULL*/
		     jrd_265.jrd_267)
		{
			relation->rel_flags |= MET_get_rel_flags_from_TYPE(/*X.RDB$RELATION_TYPE*/
									   jrd_265.jrd_268);
		}
	}
	/*END_FOR*/
	   }
	}

	if (check_relation)
	{
		check_relation->rel_flags &= ~REL_check_existence;
		if (check_relation != relation)
		{
			LCK_release(tdbb, check_relation->rel_existence_lock);
			LCK_release(tdbb, check_relation->rel_partners_lock);
			LCK_release(tdbb, check_relation->rel_rescan_lock);
			check_relation->rel_flags &= ~REL_check_partners;
			check_relation->rel_flags |= REL_deleted;
		}
	}

	return relation;
}


jrd_rel* MET_lookup_relation_id(thread_db* tdbb, SLONG id, bool return_deleted)
{
   struct {
          TEXT  jrd_256 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_257;	// gds__utility 
          SSHORT jrd_258;	// gds__null_flag 
          SSHORT jrd_259;	// RDB$RELATION_TYPE 
          SSHORT jrd_260;	// RDB$FLAGS 
          SSHORT jrd_261;	// RDB$RELATION_ID 
   } jrd_255;
   struct {
          SSHORT jrd_254;	// RDB$RELATION_ID 
   } jrd_253;
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
	Attachment* attachment = tdbb->getAttachment();

	// System relations are above suspicion

	if (id < (int) rel_MAX)
	{
		fb_assert(id < MAX_USHORT);
		return MET_relation(tdbb, (USHORT) id);
	}

	jrd_rel* check_relation = NULL;
	jrd_rel* relation;
	vec<jrd_rel*>* vector = attachment->att_relations;
	if (vector && (id < (SLONG) vector->count()) && (relation = (*vector)[id]))
	{
		if (relation->rel_flags & REL_deleting)
			Attachment::CheckoutLockGuard guard(attachment, relation->rel_drop_mutex, FB_FUNCTION);

		if (relation->rel_flags & REL_deleted)
			return return_deleted ? relation : NULL;

		if (relation->rel_flags & REL_check_existence)
		{
			check_relation = relation;
			LCK_lock(tdbb, check_relation->rel_existence_lock, LCK_SR, LCK_WAIT);
		}
		else
			return relation;
	}

	// We need to look up the relation id in RDB$RELATIONS

	relation = NULL;

	AutoCacheRequest request(tdbb, irq_l_rel_id, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$RELATIONS WITH X.RDB$RELATION_ID EQ id*/
	{
	request.compile(tdbb, (UCHAR*) jrd_252, sizeof(jrd_252));
	jrd_253.jrd_254 = id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_253);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 42, (UCHAR*) &jrd_255);
	   if (!jrd_255.jrd_257) break;
	{
		relation = MET_relation(tdbb, /*X.RDB$RELATION_ID*/
					      jrd_255.jrd_261);
		if (relation->rel_name.length() == 0) {
			relation->rel_name = /*X.RDB$RELATION_NAME*/
					     jrd_255.jrd_256;
		}

		relation->rel_flags |= get_rel_flags_from_FLAGS(/*X.RDB$FLAGS*/
								jrd_255.jrd_260);

		if (!/*X.RDB$RELATION_TYPE.NULL*/
		     jrd_255.jrd_258)
		{
			relation->rel_flags |= MET_get_rel_flags_from_TYPE(/*X.RDB$RELATION_TYPE*/
									   jrd_255.jrd_259);
		}
	}
	/*END_FOR*/
	   }
	}

	if (check_relation)
	{
		check_relation->rel_flags &= ~REL_check_existence;
		if (check_relation != relation)
		{
			LCK_release(tdbb, check_relation->rel_existence_lock);
			LCK_release(tdbb, check_relation->rel_partners_lock);
			LCK_release(tdbb, check_relation->rel_rescan_lock);
			check_relation->rel_flags &= ~REL_check_partners;
			check_relation->rel_flags |= REL_deleted;
		}
	}

	return relation;
}


DmlNode* MET_parse_blob(thread_db*	tdbb,
				   jrd_rel*		relation,
				   bid*			blob_id,
				   CompilerScratch**		csb_ptr,
				   JrdStatement**	statementPtr,
				   const bool	trigger,
				   bool validationExpr)
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
	Attachment* attachment = tdbb->getAttachment();

	blb* blob = blb::open(tdbb, attachment->getSysTransaction(), blob_id);
	ULONG length = blob->blb_length + 10;
	HalfStaticArray<UCHAR, 512> tmp;
	UCHAR* temp = tmp.getBuffer(length);
	length = blob->BLB_get_data(tdbb, temp, length);

	DmlNode* node = NULL;

	if (validationExpr)
	{
		// The set of MET parse functions needs a rework.
		// For now, our caller chain is not interested in the returned node.
		PAR_validation_blr(tdbb, relation, temp, length, NULL, csb_ptr, 0);
	}
	else
		node = PAR_blr(tdbb, relation, temp, length, NULL, csb_ptr, statementPtr, trigger, 0);

	return node;
}


void MET_parse_sys_trigger(thread_db* tdbb, jrd_rel* relation)
{
   struct {
          bid  jrd_247;	// RDB$TRIGGER_BLR 
          TEXT  jrd_248 [32];	// RDB$TRIGGER_NAME 
          ISC_INT64  jrd_249;	// RDB$TRIGGER_TYPE 
          SSHORT jrd_250;	// gds__utility 
          SSHORT jrd_251;	// RDB$FLAGS 
   } jrd_246;
   struct {
          TEXT  jrd_245 [32];	// RDB$RELATION_NAME 
   } jrd_244;
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
	Attachment* attachment = tdbb->getAttachment();
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

	if (dbb->readOnly() && !(relation->rel_flags & REL_temp_tran))
		return;

	relation->rel_flags |= REL_sys_trigs_being_loaded;

	AutoCacheRequest request(tdbb, irq_s_triggers2, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME = relation->rel_name.c_str()
		AND TRG.RDB$SYSTEM_FLAG = 1*/
	{
	request.compile(tdbb, (UCHAR*) jrd_243, sizeof(jrd_243));
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_244.jrd_245, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_244);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 52, (UCHAR*) &jrd_246);
	   if (!jrd_246.jrd_250) break;
	{
		const FB_UINT64 type = /*TRG.RDB$TRIGGER_TYPE*/
				       jrd_246.jrd_249;
		const USHORT trig_flags = /*TRG.RDB$FLAGS*/
					  jrd_246.jrd_251;
		const TEXT* name = /*TRG.RDB$TRIGGER_NAME*/
				   jrd_246.jrd_248;

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
			blb* blob = blb::open(tdbb, attachment->getSysTransaction(), &/*TRG.RDB$TRIGGER_BLR*/
										      jrd_246.jrd_247);
			ULONG length = blob->blb_length + 10;
			HalfStaticArray<UCHAR, 128> blr;
			length = blob->BLB_get_data(tdbb, blr.getBuffer(length), length);

			USHORT par_flags = (USHORT) ((trig_flags & TRG_ignore_perm) ? csb_ignore_perm : 0);
			if (type & 1)
				par_flags |= csb_pre_trigger;
			else
				par_flags |= csb_post_trigger;

			JrdStatement* statement = NULL;

			{
				Jrd::ContextPoolHolder context(tdbb, attachment->createPool());
				PAR_blr(tdbb, relation, blr.begin(), length, NULL, NULL, &statement, true, par_flags);
			}

			statement->triggerName = name;

			statement->flags |= JrdStatement::FLAG_SYS_TRIGGER;
			if (trig_flags & TRG_ignore_perm)
				statement->flags |= JrdStatement::FLAG_IGNORE_PERM;

			save_trigger_data(tdbb, ptr, relation, statement, NULL, NULL, NULL, type, true, 0, "",
				"", NULL);
		}
	}
	/*END_FOR*/
	   }
	}

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
          bid  jrd_240;	// RDB$TRANSACTION_DESCRIPTION 
          SLONG  jrd_241;	// RDB$TRANSACTION_ID 
          SSHORT jrd_242;	// RDB$TRANSACTION_STATE 
   } jrd_239;
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
	Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, irq_s_trans, IRQ_REQUESTS);

	/*STORE(REQUEST_HANDLE request) X IN RDB$TRANSACTIONS*/
	{
	
	{
		/*X.RDB$TRANSACTION_ID*/
		jrd_239.jrd_241 = transaction->tra_number;
		/*X.RDB$TRANSACTION_STATE*/
		jrd_239.jrd_242 = /*RDB$TRANSACTIONS.RDB$TRANSACTION_STATE.LIMBO*/
   1;
		blb* blob = blb::create(tdbb, attachment->getSysTransaction(), &/*X.RDB$TRANSACTION_DESCRIPTION*/
										jrd_239.jrd_240);
		blob->BLB_put_segment(tdbb, msg, length);
		blob->BLB_close(tdbb);
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_238, sizeof(jrd_238));
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 14, (UCHAR*) &jrd_239);
	}
}


jrd_prc* MET_procedure(thread_db* tdbb, USHORT id, bool noscan, USHORT flags)
{
   struct {
          SSHORT jrd_175;	// gds__utility 
   } jrd_174;
   struct {
          SSHORT jrd_172;	// gds__null_flag 
          SSHORT jrd_173;	// RDB$VALID_BLR 
   } jrd_171;
   struct {
          SSHORT jrd_168;	// gds__utility 
          SSHORT jrd_169;	// gds__null_flag 
          SSHORT jrd_170;	// RDB$VALID_BLR 
   } jrd_167;
   struct {
          SSHORT jrd_166;	// RDB$PROCEDURE_ID 
   } jrd_165;
   struct {
          TEXT  jrd_181 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_182 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_183 [32];	// RDB$PARAMETER_NAME 
          bid  jrd_184;	// RDB$DEFAULT_VALUE 
          bid  jrd_185;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_186;	// gds__utility 
          SSHORT jrd_187;	// gds__null_flag 
          SSHORT jrd_188;	// RDB$COLLATION_ID 
          SSHORT jrd_189;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_190;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_191;	// RDB$FIELD_LENGTH 
          SSHORT jrd_192;	// RDB$FIELD_SCALE 
          SSHORT jrd_193;	// RDB$FIELD_TYPE 
          SSHORT jrd_194;	// gds__null_flag 
          SSHORT jrd_195;	// gds__null_flag 
          SSHORT jrd_196;	// RDB$PARAMETER_MECHANISM 
          SSHORT jrd_197;	// gds__null_flag 
          SSHORT jrd_198;	// RDB$NULL_FLAG 
          SSHORT jrd_199;	// RDB$PARAMETER_NUMBER 
          SSHORT jrd_200;	// RDB$PARAMETER_TYPE 
          SSHORT jrd_201;	// gds__null_flag 
          SSHORT jrd_202;	// gds__null_flag 
          SSHORT jrd_203;	// RDB$COLLATION_ID 
   } jrd_180;
   struct {
          TEXT  jrd_178 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_179 [32];	// RDB$PROCEDURE_NAME 
   } jrd_177;
   struct {
          TEXT  jrd_208 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_209;	// gds__utility 
          SSHORT jrd_210;	// gds__null_flag 
   } jrd_207;
   struct {
          TEXT  jrd_206 [32];	// RDB$PACKAGE_NAME 
   } jrd_205;
   struct {
          bid  jrd_215;	// RDB$DEBUG_INFO 
          TEXT  jrd_216 [256];	// RDB$ENTRYPOINT 
          bid  jrd_217;	// RDB$PROCEDURE_SOURCE 
          bid  jrd_218;	// RDB$PROCEDURE_BLR 
          TEXT  jrd_219 [32];	// RDB$ENGINE_NAME 
          TEXT  jrd_220 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_221 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_222 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_223;	// gds__utility 
          SSHORT jrd_224;	// gds__null_flag 
          SSHORT jrd_225;	// RDB$VALID_BLR 
          SSHORT jrd_226;	// gds__null_flag 
          SSHORT jrd_227;	// gds__null_flag 
          SSHORT jrd_228;	// gds__null_flag 
          SSHORT jrd_229;	// gds__null_flag 
          SSHORT jrd_230;	// gds__null_flag 
          SSHORT jrd_231;	// RDB$PROCEDURE_TYPE 
          SSHORT jrd_232;	// gds__null_flag 
          SSHORT jrd_233;	// RDB$PROCEDURE_OUTPUTS 
          SSHORT jrd_234;	// RDB$PROCEDURE_INPUTS 
          SSHORT jrd_235;	// gds__null_flag 
          SSHORT jrd_236;	// RDB$PROCEDURE_ID 
          SSHORT jrd_237;	// gds__null_flag 
   } jrd_214;
   struct {
          SSHORT jrd_213;	// RDB$PROCEDURE_ID 
   } jrd_212;
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
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	if (id >= attachment->att_procedures.getCount())
		attachment->att_procedures.resize(id + 10);

	jrd_prc* procedure = attachment->att_procedures[id];

	if (procedure && !(procedure->flags & Routine::FLAG_OBSOLETE))
	{
		// Make sure PRC_being_scanned and PRC_scanned are not set at the same time

		fb_assert(!(procedure->flags & Routine::FLAG_BEING_SCANNED) ||
			!(procedure->flags & Routine::FLAG_SCANNED));

		/* To avoid scanning recursive procedure's blr recursively let's
		   make use of Routine::FLAG_BEING_SCANNED bit. Because this bit is set
		   later in the code, it is not set when we are here first time.
		   If (in case of rec. procedure) we get here second time it is
		   already set and we return half baked procedure.
		   In case of superserver this code is under the rec. mutex
		   protection, thus the only guy (thread) who can get here and see
		   FLAG_BEING_SCANNED bit set is the guy which started procedure scan
		   and currently holds the mutex.
		   In case of classic, there is always only one guy and if it
		   sees FLAG_BEING_SCANNED bit set it is safe to assume it is here
		   second time.

		   If procedure has already been scanned - return. This condition
		   is for those threads that did not find procedure in cache and
		   came here to get it from disk. But only one was able to lock
		   the mutex and do the scanning, others were waiting. As soon as
		   the first thread releases the mutex another thread gets in and
		   it would be just unfair to make it do the work again.
		 */

		if ((procedure->flags & Routine::FLAG_BEING_SCANNED) ||
			(procedure->flags & Routine::FLAG_SCANNED))
		{
			return procedure;
		}
	}

	if (!procedure)
	{
		procedure = FB_NEW(*dbb->dbb_permanent) jrd_prc(*dbb->dbb_permanent);
	}

	try {

	procedure->flags |= (Routine::FLAG_BEING_SCANNED | flags);
	procedure->flags &= ~Routine::FLAG_OBSOLETE;

	procedure->setId(id);
	attachment->att_procedures[id] = procedure;

	if (!procedure->existenceLock)
	{
		Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, 0)
			Lock(tdbb, sizeof(SLONG), LCK_prc_exist, procedure, blocking_ast_procedure);
		procedure->existenceLock = lock;
		lock->lck_key.lck_long = procedure->getId();
	}

	LCK_lock(tdbb, procedure->existenceLock, LCK_SR, LCK_WAIT);

	if (!noscan)
	{
		bool valid_blr = true;

		AutoCacheRequest request(tdbb, irq_r_procedure, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			P IN RDB$PROCEDURES WITH P.RDB$PROCEDURE_ID EQ procedure->getId()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_211, sizeof(jrd_211));
		jrd_212.jrd_213 = procedure->getId();
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_212);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 438, (UCHAR*) &jrd_214);
		   if (!jrd_214.jrd_223) break;
		{
			if (procedure->getName().toString().length() == 0)
			{
				procedure->setName(QualifiedName(/*P.RDB$PROCEDURE_NAME*/
								 jrd_214.jrd_222,
					(/*P.RDB$PACKAGE_NAME.NULL*/
					 jrd_214.jrd_237 ? "" : /*P.RDB$PACKAGE_NAME*/
	jrd_214.jrd_221)));
			}

			procedure->setId(/*P.RDB$PROCEDURE_ID*/
					 jrd_214.jrd_236);

			if (!/*P.RDB$SECURITY_CLASS.NULL*/
			     jrd_214.jrd_235)
				procedure->setSecurityName(/*P.RDB$SECURITY_CLASS*/
							   jrd_214.jrd_220);
			else if (!/*P.RDB$PACKAGE_NAME.NULL*/
				  jrd_214.jrd_237)
			{
				AutoCacheRequest requestHandle(tdbb, irq_l_procedure_pkg_class, IRQ_REQUESTS);

				/*FOR (REQUEST_HANDLE requestHandle)
					PKG IN RDB$PACKAGES
					WITH PKG.RDB$PACKAGE_NAME EQ P.RDB$PACKAGE_NAME*/
				{
				requestHandle.compile(tdbb, (UCHAR*) jrd_204, sizeof(jrd_204));
				gds__vtov ((const char*) jrd_214.jrd_221, (char*) jrd_205.jrd_206, 32);
				EXE_start (tdbb, requestHandle, attachment->getSysTransaction());
				EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_205);
				while (1)
				   {
				   EXE_receive (tdbb, requestHandle, 1, 36, (UCHAR*) &jrd_207);
				   if (!jrd_207.jrd_209) break;
				{
					if (!/*PKG.RDB$SECURITY_CLASS.NULL*/
					     jrd_207.jrd_210)
						procedure->setSecurityName(/*PKG.RDB$SECURITY_CLASS*/
									   jrd_207.jrd_208);
				}
				/*END_FOR*/
				   }
				}
			}

			procedure->setImplemented(true);
			procedure->getInputFields().resize(/*P.RDB$PROCEDURE_INPUTS*/
							   jrd_214.jrd_234);
			procedure->getOutputFields().resize(/*P.RDB$PROCEDURE_OUTPUTS*/
							    jrd_214.jrd_233);
			procedure->setDefaultCount(0);

			AutoCacheRequest request2(tdbb, irq_r_params, IRQ_REQUESTS);

			const MetaName packageName(/*P.RDB$PACKAGE_NAME.NULL*/
						   jrd_214.jrd_237 ? NULL : /*P.RDB$PACKAGE_NAME*/
	  jrd_214.jrd_221);

			/*FOR (REQUEST_HANDLE request2)
				PA IN RDB$PROCEDURE_PARAMETERS
				CROSS F IN RDB$FIELDS
				WITH F.RDB$FIELD_NAME = PA.RDB$FIELD_SOURCE AND
					 PA.RDB$PROCEDURE_NAME = P.RDB$PROCEDURE_NAME AND
					 PA.RDB$PACKAGE_NAME EQUIV NULLIF(packageName.c_str(), '')*/
			{
			request2.compile(tdbb, (UCHAR*) jrd_176, sizeof(jrd_176));
			gds__vtov ((const char*) packageName.c_str(), (char*) jrd_177.jrd_178, 32);
			gds__vtov ((const char*) jrd_214.jrd_222, (char*) jrd_177.jrd_179, 32);
			EXE_start (tdbb, request2, attachment->getSysTransaction());
			EXE_send (tdbb, request2, 0, 64, (UCHAR*) &jrd_177);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 148, (UCHAR*) &jrd_180);
			   if (!jrd_180.jrd_186) break;
			{
				const SSHORT pa_collation_id_null = /*PA.RDB$COLLATION_ID.NULL*/
								    jrd_180.jrd_202;
				const SSHORT pa_collation_id = /*PA.RDB$COLLATION_ID*/
							       jrd_180.jrd_203;
				const SSHORT pa_default_value_null = /*PA.RDB$DEFAULT_VALUE.NULL*/
								     jrd_180.jrd_201;
				bid pa_default_value = pa_default_value_null ? /*F.RDB$DEFAULT_VALUE*/
									       jrd_180.jrd_184 : /*PA.RDB$DEFAULT_VALUE*/
   jrd_180.jrd_185;

				Array<NestConst<Parameter> >& paramArray = /*PA.RDB$PARAMETER_TYPE*/
									   jrd_180.jrd_200 ?
					procedure->getOutputFields() : procedure->getInputFields();

				// should be error if field already exists
				Parameter* parameter = FB_NEW(*dbb->dbb_permanent) Parameter(*dbb->dbb_permanent);
				parameter->prm_number = /*PA.RDB$PARAMETER_NUMBER*/
							jrd_180.jrd_199;
				paramArray[parameter->prm_number] = parameter;
				parameter->prm_name = /*PA.RDB$PARAMETER_NAME*/
						      jrd_180.jrd_183;
				parameter->prm_nullable = /*PA.RDB$NULL_FLAG.NULL*/
							  jrd_180.jrd_197 || /*PA.RDB$NULL_FLAG*/
    jrd_180.jrd_198 == 0;	// ODS_11_1
				parameter->prm_mechanism = /*PA.RDB$PARAMETER_MECHANISM.NULL*/
							   jrd_180.jrd_195 ?	// ODS_11_1
					prm_mech_normal : (prm_mech_t) /*PA.RDB$PARAMETER_MECHANISM*/
								       jrd_180.jrd_196;

				if (!/*PA.RDB$FIELD_SOURCE.NULL*/
				     jrd_180.jrd_194)
					parameter->prm_field_source = /*PA.RDB$FIELD_SOURCE*/
								      jrd_180.jrd_182;

				DSC_make_descriptor(&parameter->prm_desc, /*F.RDB$FIELD_TYPE*/
									  jrd_180.jrd_193,
									/*F.RDB$FIELD_SCALE*/
									jrd_180.jrd_192, /*F.RDB$FIELD_LENGTH*/
  jrd_180.jrd_191,
									/*F.RDB$FIELD_SUB_TYPE*/
									jrd_180.jrd_190, /*F.RDB$CHARACTER_SET_ID*/
  jrd_180.jrd_189,
									(pa_collation_id_null ? /*F.RDB$COLLATION_ID*/
												jrd_180.jrd_188 : pa_collation_id));

				if (/*PA.RDB$PARAMETER_TYPE*/
				    jrd_180.jrd_200 == 0 &&
					(!pa_default_value_null ||
					 (fb_utils::implicit_domain(/*F.RDB$FIELD_NAME*/
								    jrd_180.jrd_181) && !/*F.RDB$DEFAULT_VALUE.NULL*/
      jrd_180.jrd_187)))
				{
					procedure->setDefaultCount(procedure->getDefaultCount() + 1);
					MemoryPool* pool = attachment->createPool();
					Jrd::ContextPoolHolder context(tdbb, pool);

					try
					{
						parameter->prm_default_value = static_cast<ValueExprNode*>(
							MET_parse_blob(tdbb, NULL, &pa_default_value, NULL, NULL, false, false));
					}
					catch (const Exception&)
					{
						// Here we lose pools created for previous defaults.
						// Probably we should use common pool for defaults and procedure itself.

						attachment->deletePool(pool);
						throw;
					}
				}
			}
			/*END_FOR*/
			   }
			}

			const bool external = !/*P.RDB$ENGINE_NAME.NULL*/
					       jrd_214.jrd_232;	// ODS_12_0

			Array<NestConst<Parameter> >& paramArray = procedure->getOutputFields();

			if (paramArray.hasData() && paramArray[0])
			{
				Format* format = Format::newFormat(
					*dbb->dbb_permanent, procedure->getOutputFields().getCount());
				procedure->prc_record_format = format;
				ULONG length = FLAG_BYTES(format->fmt_count);
				Format::fmt_desc_iterator desc = format->fmt_desc.begin();
				Array<NestConst<Parameter> >::iterator ptr, end;
				for (ptr = paramArray.begin(), end = paramArray.end(); ptr < end; ++ptr, ++desc)
				{
					const Parameter* parameter = *ptr;
					// check for parameter to be null, this can only happen if the
					// parameter numbers get out of sync. This was added to fix bug
					// 10534. -Shaunak Mistry 12-May-99
					if (parameter)
					{
						*desc = parameter->prm_desc;
						length = MET_align(&(*desc), length);
						desc->dsc_address = (UCHAR *) (IPTR) length;
						length += desc->dsc_length;
					}
				}

				format->fmt_length = length;
			}

			procedure->prc_type = /*P.RDB$PROCEDURE_TYPE.NULL*/
					      jrd_214.jrd_230 ?
				prc_legacy : (prc_t) /*P.RDB$PROCEDURE_TYPE*/
						     jrd_214.jrd_231;

			if (external || !/*P.RDB$PROCEDURE_BLR.NULL*/
					 jrd_214.jrd_229)
			{
				MemoryPool* const csb_pool = attachment->createPool();
				Jrd::ContextPoolHolder context(tdbb, csb_pool);

				try
				{
					AutoPtr<CompilerScratch> csb(CompilerScratch::newCsb(*csb_pool, 5));

					if (external)
					{
						HalfStaticArray<char, 512> body;

						if (!/*P.RDB$PROCEDURE_SOURCE.NULL*/
						     jrd_214.jrd_228)
						{
							blb* blob = blb::open(tdbb, attachment->getSysTransaction(),
								&/*P.RDB$PROCEDURE_SOURCE*/
								 jrd_214.jrd_217);
							ULONG len = blob->BLB_get_data(tdbb,
								(UCHAR*) body.getBuffer(blob->blb_length + 1), blob->blb_length + 1);
							body.begin()[MIN(blob->blb_length, len)] = '\0';
						}
						else
							body.getBuffer(1)[0] = '\0';

						dbb->dbb_extManager.makeProcedure(tdbb, csb, procedure, /*P.RDB$ENGINE_NAME*/
													jrd_214.jrd_219,
							(/*P.RDB$ENTRYPOINT.NULL*/
							 jrd_214.jrd_227 ? "" : /*P.RDB$ENTRYPOINT*/
	jrd_214.jrd_216), body.begin());
					}
					else
					{
						if (!/*P.RDB$DEBUG_INFO.NULL*/
						     jrd_214.jrd_226)
							DBG_parse_debug_info(tdbb, &/*P.RDB$DEBUG_INFO*/
										    jrd_214.jrd_215, *csb->csb_dbg_info);

						try
						{
							procedure->parseBlr(tdbb, csb, &/*P.RDB$PROCEDURE_BLR*/
											jrd_214.jrd_218);
						}
						catch (const Exception& ex)
						{
							ISC_STATUS_ARRAY temp_status;
							ex.stuff_exception(temp_status);
							const string name = procedure->getName().toString();
							(Arg::Gds(isc_bad_proc_BLR) << Arg::Str(name)
								<< Arg::StatusVector(temp_status)).raise();
						}
					}
				}
				catch (const Exception&)
				{
					if (procedure->getStatement())
						procedure->releaseStatement(tdbb);
					else
						attachment->deletePool(csb_pool);

					throw;
				}

				procedure->getStatement()->procedure = procedure;
			}
			else
			{
				RefPtr<MsgMetadata> inputMetadata(REF_NO_INCR,
					Routine::createMetadata(procedure->getInputFields()));
				procedure->setInputFormat(
					Routine::createFormat(procedure->getPool(), inputMetadata, false));

				RefPtr<MsgMetadata> outputMetadata(REF_NO_INCR,
					Routine::createMetadata(procedure->getOutputFields()));
				procedure->setOutputFormat(
					Routine::createFormat(procedure->getPool(), outputMetadata, true));

				procedure->setImplemented(false);
			}

			if (/*P.RDB$VALID_BLR.NULL*/
			    jrd_214.jrd_224 || /*P.RDB$VALID_BLR*/
    jrd_214.jrd_225 == FALSE)
				valid_blr = false;
		}
		/*END_FOR*/
		   }
		}

		procedure->flags |= Routine::FLAG_SCANNED;

		if (!dbb->readOnly() && !valid_blr)
		{
			// if the BLR was marked as invalid but the procedure was compiled,
			// mark the BLR as valid

			AutoRequest request5;

			/*FOR(REQUEST_HANDLE request5)
				P IN RDB$PROCEDURES WITH
					P.RDB$PROCEDURE_ID EQ procedure->getId() AND P.RDB$PROCEDURE_BLR NOT MISSING*/
			{
			request5.compile(tdbb, (UCHAR*) jrd_164, sizeof(jrd_164));
			jrd_165.jrd_166 = procedure->getId();
			EXE_start (tdbb, request5, attachment->getSysTransaction());
			EXE_send (tdbb, request5, 0, 2, (UCHAR*) &jrd_165);
			while (1)
			   {
			   EXE_receive (tdbb, request5, 1, 6, (UCHAR*) &jrd_167);
			   if (!jrd_167.jrd_168) break;
			{
				/*MODIFY P USING*/
				{
				
					/*P.RDB$VALID_BLR*/
					jrd_167.jrd_170 = TRUE;
					/*P.RDB$VALID_BLR.NULL*/
					jrd_167.jrd_169 = FALSE;
				/*END_MODIFY*/
				jrd_171.jrd_172 = jrd_167.jrd_169;
				jrd_171.jrd_173 = jrd_167.jrd_170;
				EXE_send (tdbb, request5, 2, 4, (UCHAR*) &jrd_171);
				}
			}
			/*END_FOR*/
			   EXE_send (tdbb, request5, 3, 2, (UCHAR*) &jrd_174);
			   }
			}
		}
	} // if !noscan

	// Make sure that it is really being scanned
	fb_assert(procedure->flags & Routine::FLAG_BEING_SCANNED);

	procedure->flags &= ~Routine::FLAG_BEING_SCANNED;

	}	// try
	catch (const Exception&)
	{
		procedure->flags &= ~(Routine::FLAG_BEING_SCANNED | Routine::FLAG_SCANNED);

		if (procedure->getExternal())
		{
			delete procedure->getExternal();
			procedure->setExternal(NULL);
		}

		if (procedure->existenceLock)
		{
			LCK_release(tdbb, procedure->existenceLock);
			delete procedure->existenceLock;
			procedure->existenceLock = NULL;
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

	Attachment* attachment = tdbb->getAttachment();
	vec<jrd_rel*>* vector = attachment->att_relations;
	MemoryPool* pool = attachment->att_pool;

	if (!vector)
		vector = attachment->att_relations = vec<jrd_rel*>::newVector(*pool, id + 10);
	else if (id >= vector->count())
		vector->resize(id + 10);

	jrd_rel* relation = (*vector)[id];
	if (relation)
		return relation;

	// From ODS 9 onwards, the first 128 relation IDS have been
	// reserved for system relations
	const USHORT max_sys_rel = USER_DEF_REL_INIT_ID - 1;

	relation = FB_NEW(*pool) jrd_rel(*pool);
	(*vector)[id] = relation;
	relation->rel_id = id;

	{ // Scope block.
		Lock* lock = FB_NEW_RPT(*pool, 0)
			Lock(tdbb, sizeof(SLONG), LCK_rel_partners, relation, partners_ast_relation);
		relation->rel_partners_lock = lock;
		lock->lck_key.lck_long = relation->rel_id;
	}

	{ // Scope block.
		Lock* lock = FB_NEW_RPT(*pool, 0)
			Lock(tdbb, sizeof(SLONG), LCK_rel_rescan, relation, rescan_ast_relation);
		relation->rel_rescan_lock = lock;
		lock->lck_key.lck_long = relation->rel_id;
	}

	// This should check system flag instead.
	if (relation->rel_id <= max_sys_rel) {
		return relation;
	}

	{ // Scope block.
		Lock* lock = FB_NEW_RPT(*pool, 0)
			Lock(tdbb, sizeof(SLONG), LCK_rel_exist, relation, blocking_ast_relation);
		relation->rel_existence_lock = lock;
		lock->lck_key.lck_long = relation->rel_id;
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

	if (!relation->rel_use_count)
	{
		if (relation->rel_flags & REL_blocking) {
			LCK_re_post(tdbb, relation->rel_existence_lock);
		}

		if (relation->rel_file)
		{
			// close external file
			EXT_fini(relation, true);
		}
	}
}


void MET_revoke(thread_db* tdbb, jrd_tra* transaction, const MetaName& relation,
	const MetaName& revokee, const string& privilege)
{
   struct {
          SSHORT jrd_156;	// gds__utility 
   } jrd_155;
   struct {
          SSHORT jrd_154;	// gds__utility 
   } jrd_153;
   struct {
          SSHORT jrd_152;	// gds__utility 
   } jrd_151;
   struct {
          TEXT  jrd_148 [32];	// RDB$GRANTOR 
          TEXT  jrd_149 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_150 [7];	// RDB$PRIVILEGE 
   } jrd_147;
   struct {
          SSHORT jrd_163;	// gds__utility 
   } jrd_162;
   struct {
          TEXT  jrd_159 [32];	// RDB$USER 
          TEXT  jrd_160 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_161 [7];	// RDB$PRIVILEGE 
   } jrd_158;
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

	// See if the revokee still has the privilege.  If so, there's nothing to do

	USHORT count = 0;

	AutoCacheRequest request(tdbb, irq_revoke1, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FIRST 1 P IN RDB$USER_PRIVILEGES WITH
			P.RDB$RELATION_NAME EQ relation.c_str() AND
			P.RDB$PRIVILEGE EQ privilege.c_str() AND
			P.RDB$USER EQ revokee.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_157, sizeof(jrd_157));
	gds__vtov ((const char*) revokee.c_str(), (char*) jrd_158.jrd_159, 32);
	gds__vtov ((const char*) relation.c_str(), (char*) jrd_158.jrd_160, 32);
	gds__vtov ((const char*) privilege.c_str(), (char*) jrd_158.jrd_161, 7);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 71, (UCHAR*) &jrd_158);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_162);
	   if (!jrd_162.jrd_163) break;
	{
		++count;
	}
	/*END_FOR*/
	   }
	}

	if (count)
		return;

	request.reset(tdbb, irq_revoke2, IRQ_REQUESTS);

	// User lost privilege.  Take it away from anybody he/she gave it to.

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		P IN RDB$USER_PRIVILEGES WITH
			P.RDB$RELATION_NAME EQ relation.c_str() AND
			P.RDB$PRIVILEGE EQ privilege.c_str() AND
			P.RDB$GRANTOR EQ revokee.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_146, sizeof(jrd_146));
	gds__vtov ((const char*) revokee.c_str(), (char*) jrd_147.jrd_148, 32);
	gds__vtov ((const char*) relation.c_str(), (char*) jrd_147.jrd_149, 32);
	gds__vtov ((const char*) privilege.c_str(), (char*) jrd_147.jrd_150, 7);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 71, (UCHAR*) &jrd_147);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_151);
	   if (!jrd_151.jrd_152) break;
	{
		/*ERASE P;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_153);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_155);
	   }
	}
}


void MET_scan_partners(thread_db* tdbb, jrd_rel* relation)
{
/**************************************
 *
 *      M E T _ s c a n _ p a r t n e r s
 *
 **************************************
 *
 * Functional description
 *      Scan of foreign references on other relations' primary keys and
 *      scan of primary dependencies on relation's primary key.
 *
 **************************************/

	SET_TDBB(tdbb);

	if (relation->rel_flags & REL_check_partners)
	{
		scan_partners(tdbb, relation);
	}
}


void MET_scan_relation(thread_db* tdbb, jrd_rel* relation)
{
   struct {
          TEXT  jrd_132 [32];	// RDB$DEFAULT_CLASS 
          bid  jrd_133;	// RDB$RUNTIME 
          TEXT  jrd_134 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_135;	// RDB$VIEW_BLR 
          TEXT  jrd_136 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_137 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_138 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_139;	// gds__utility 
          SSHORT jrd_140;	// gds__null_flag 
          SSHORT jrd_141;	// gds__null_flag 
          SSHORT jrd_142;	// RDB$RELATION_TYPE 
          SSHORT jrd_143;	// gds__null_flag 
          SSHORT jrd_144;	// RDB$FIELD_ID 
          SSHORT jrd_145;	// RDB$FORMAT 
   } jrd_131;
   struct {
          SSHORT jrd_130;	// RDB$RELATION_ID 
   } jrd_129;
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
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();
	Jrd::ContextPoolHolder context(tdbb, dbb->dbb_permanent);
	bool dependencies = false;
	bool sys_triggers = false;

	blb* blob = NULL;

	jrd_tra* depTrans = tdbb->getTransaction() ?
		tdbb->getTransaction() : attachment->getSysTransaction();

	// If anything errors, catch it to reset the scan flag.  This will
	// make sure that the error will be caught if the operation is tried again.

	try {

	if (relation->rel_flags & (REL_scanned | REL_deleted))
		return;

	relation->rel_flags |= REL_being_scanned;
	dependencies = (relation->rel_flags & REL_get_dependencies) ? true : false;
	sys_triggers = (relation->rel_flags & REL_sys_triggers) ? true : false;
	relation->rel_flags &= ~(REL_get_dependencies | REL_sys_triggers);

	for (USHORT itr = 0; itr < TRIGGER_MAX; ++itr)
		triggers[itr] = NULL;

	// Since this can be called recursively, find an inactive clone of the request

	AutoCacheRequest request(tdbb, irq_r_fields, IRQ_REQUESTS);
	CompilerScratch* csb = NULL;

	/*FOR(REQUEST_HANDLE request)
		REL IN RDB$RELATIONS WITH REL.RDB$RELATION_ID EQ relation->rel_id*/
	{
	request.compile(tdbb, (UCHAR*) jrd_128, sizeof(jrd_128));
	jrd_129.jrd_130 = relation->rel_id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_129);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 414, (UCHAR*) &jrd_131);
	   if (!jrd_131.jrd_139) break;
	{
		// Pick up relation level stuff
		relation->rel_current_fmt = /*REL.RDB$FORMAT*/
					    jrd_131.jrd_145;
		vec<jrd_fld*>* vector = relation->rel_fields =
			vec<jrd_fld*>::newVector(*relation->rel_pool, relation->rel_fields, /*REL.RDB$FIELD_ID*/
											    jrd_131.jrd_144 + 1);
		if (!/*REL.RDB$SECURITY_CLASS.NULL*/
		     jrd_131.jrd_143) {
			relation->rel_security_name = /*REL.RDB$SECURITY_CLASS*/
						      jrd_131.jrd_138;
		}

		relation->rel_name = /*REL.RDB$RELATION_NAME*/
				     jrd_131.jrd_137;
		relation->rel_owner_name = /*REL.RDB$OWNER_NAME*/
					   jrd_131.jrd_136;

		if (!/*REL.RDB$VIEW_BLR*/
		     jrd_131.jrd_135.isEmpty())
		{
			// parse the view blr, getting dependencies on relations, etc. at the same time

			DmlNode* rseNode;

			if (dependencies)
			{
				const MetaName depName(/*REL.RDB$RELATION_NAME*/
						       jrd_131.jrd_137);
				rseNode = MET_get_dependencies(tdbb, relation, NULL, 0, NULL, &/*REL.RDB$VIEW_BLR*/
											       jrd_131.jrd_135,
					NULL, &csb, depName, obj_view, 0, depTrans);
			}
			else
				rseNode = MET_parse_blob(tdbb, relation, &/*REL.RDB$VIEW_BLR*/
									  jrd_131.jrd_135, &csb, NULL, false, false);

			if (rseNode)
			{
				fb_assert(rseNode->kind == DmlNode::KIND_REC_SOURCE);
				relation->rel_view_rse = static_cast<RseNode*>(rseNode);
				fb_assert(relation->rel_view_rse->type == RseNode::TYPE);
			}
			else
				relation->rel_view_rse = NULL;

			// retrieve the view context names

			lookup_view_contexts(tdbb, relation);
		}

		relation->rel_flags |= REL_scanned;
		if (/*REL.RDB$EXTERNAL_FILE*/
		    jrd_131.jrd_134[0])
		{
			EXT_file(relation, /*REL.RDB$EXTERNAL_FILE*/
					   jrd_131.jrd_134); //, &REL.RDB$EXTERNAL_DESCRIPTION);
		}

		if (!/*REL.RDB$RELATION_TYPE.NULL*/
		     jrd_131.jrd_141)
		{
			switch (/*REL.RDB$RELATION_TYPE*/
				jrd_131.jrd_142)
			{
				case rel_persistent:
					break;
				case rel_external:
					fb_assert(relation->rel_file);
					break;
				case rel_view:
					fb_assert(relation->rel_view_rse);
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
		}

		// Pick up field specific stuff

		blob = blb::open(tdbb, attachment->getSysTransaction(), &/*REL.RDB$RUNTIME*/
									 jrd_131.jrd_133);
		HalfStaticArray<UCHAR, 256> temp;
		UCHAR* const buffer = temp.getBuffer(blob->getMaxSegment() + 1U);

		jrd_fld* field = NULL;
		ArrayField* array = 0;
		USHORT view_context = 0;
		USHORT field_id = 0;
		for (;;)
		{
			USHORT length = blob->BLB_get_segment(tdbb, buffer, blob->getMaxSegment());
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
											jrd_131.jrd_140)
				{
					field->fld_security_name = /*REL.RDB$DEFAULT_CLASS*/
								   jrd_131.jrd_132;
				}
				field_id = n;
				field = (*vector)[field_id];

				if (field)
				{
					field->fld_computation = NULL;
					field->fld_missing_value = NULL;
					field->fld_default_value = NULL;
					field->fld_validation = NULL;
					field->fld_not_null = NULL;
				}

				array = NULL;
				break;

			case RSR_field_name:
				if (field)
				{
					// The field exists.  If its name hasn't changed, then
					// there's no need to copy anything.

					if (field->fld_name == reinterpret_cast<char*>(p))
						break;

					field->fld_name = reinterpret_cast<char*>(p);
				}
				else
				{
					field = FB_NEW(*dbb->dbb_permanent) jrd_fld(*dbb->dbb_permanent);
					(*vector)[field_id] = field;
					field->fld_name = reinterpret_cast<char*>(p);
				}

				// CVC: Be paranoid and allow the possible trigger(s) to have a
				//   not null security class to work on, even if we only take it
				//   from the relation itself.
				if (field->fld_security_name.length() == 0 && !/*REL.RDB$DEFAULT_CLASS.NULL*/
									       jrd_131.jrd_140)
				{
					field->fld_security_name = /*REL.RDB$DEFAULT_CLASS*/
								   jrd_131.jrd_132;
				}

				break;

			case RSR_view_context:
				view_context = n;
				break;

			case RSR_base_field:
				if (dependencies)
				{
					csb->csb_g_flags |= csb_get_dependencies;
					field->fld_source = PAR_make_field(tdbb, csb, view_context, (TEXT*) p);
					const MetaName depName(/*REL.RDB$RELATION_NAME*/
							       jrd_131.jrd_137);
					store_dependencies(tdbb, csb, 0, depName, obj_view, depTrans);
				}
				else
					field->fld_source = PAR_make_field(tdbb, csb, view_context, (TEXT*) p);
				break;

			case RSR_computed_blr:
				{
					DmlNode* nod = dependencies ?
						MET_get_dependencies(tdbb, relation, p, length, csb, NULL, NULL, NULL,
							field->fld_name, obj_computed, 0, depTrans) :
						PAR_blr(tdbb, relation, p, length, csb, NULL, NULL, false, 0);

					field->fld_computation = static_cast<ValueExprNode*>(nod);
				}
				break;

			case RSR_missing_value:
				field->fld_missing_value = static_cast<ValueExprNode*>(
					PAR_blr(tdbb, relation, p, length, csb, NULL, NULL, false, 0));
				break;

			case RSR_default_value:
				field->fld_default_value = static_cast<ValueExprNode*>(
					PAR_blr(tdbb, relation, p, length, csb, NULL, NULL, false, 0));
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
					field->fld_validation = PAR_validation_blr(tdbb, relation, p, length, csb,
						NULL, csb_validation);
				}
				break;

			case RSR_field_not_null:
				field->fld_not_null = PAR_validation_blr(tdbb, relation, p, length, csb,
					NULL, csb_validation);
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

			case RSR_field_generator_name:
				field->fld_generator_name = (const TEXT*) p;
				break;

			default:    // Shut up compiler warning
				break;
			}
		}
		blob->BLB_close(tdbb);
		blob = 0;

		if (field && field->fld_security_name.length() == 0 && !/*REL.RDB$DEFAULT_CLASS.NULL*/
									jrd_131.jrd_140)
		{
			field->fld_security_name = /*REL.RDB$DEFAULT_CLASS*/
						   jrd_131.jrd_132;
		}
	}
	/*END_FOR*/
	   }
	}

	delete csb;

	// release any triggers in case of a rescan, but not if the rescan
	// hapenned while system triggers were being loaded.

	if (!(relation->rel_flags & REL_sys_trigs_being_loaded))
	{
		// if we are scanning a system relation during loading the system
		// triggers, (during parsing its blr actually), we must not release the
		// existing system triggers; because we have already set the
		// relation->rel_flag to not have REL_sys_trig, so these
		// system triggers will not get loaded again. This fixes bug 8149.

		// We have just loaded the triggers onto the local vector triggers.
		// Its now time to place them at their rightful place ie the relation block.

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

	LCK_lock(tdbb, relation->rel_rescan_lock, LCK_SR, LCK_WAIT);
	relation->rel_flags &= ~REL_being_scanned;

	relation->rel_current_format = NULL;

	}	// try
	catch (const Exception&)
	{
		relation->rel_flags &= ~(REL_being_scanned | REL_scanned);
		if (dependencies) {
			relation->rel_flags |= REL_get_dependencies;
		}
		if (sys_triggers) {
			relation->rel_flags |= REL_sys_triggers;
		}
		if (blob)
			blob->BLB_close(tdbb);

		throw;
	}
}


void MET_trigger_msg(thread_db* tdbb, string& msg, const MetaName& name,
	USHORT number)
{
   struct {
          TEXT  jrd_126 [1024];	// RDB$MESSAGE 
          SSHORT jrd_127;	// gds__utility 
   } jrd_125;
   struct {
          TEXT  jrd_123 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_124;	// RDB$MESSAGE_NUMBER 
   } jrd_122;
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
	Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, irq_s_msgs, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		MSG IN RDB$TRIGGER_MESSAGES WITH
			MSG.RDB$TRIGGER_NAME EQ name.c_str() AND
			MSG.RDB$MESSAGE_NUMBER EQ number*/
	{
	request.compile(tdbb, (UCHAR*) jrd_121, sizeof(jrd_121));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_122.jrd_123, 32);
	jrd_122.jrd_124 = number;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_122);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 1026, (UCHAR*) &jrd_125);
	   if (!jrd_125.jrd_127) break;
	{
		msg = /*MSG.RDB$MESSAGE*/
		      jrd_125.jrd_126;
	}
	/*END_FOR*/
	   }
	}

	msg.rtrim();
}


void MET_update_shadow(thread_db* tdbb, Shadow* shadow, USHORT file_flags)
{
   struct {
          SSHORT jrd_120;	// gds__utility 
   } jrd_119;
   struct {
          SSHORT jrd_118;	// RDB$FILE_FLAGS 
   } jrd_117;
   struct {
          SSHORT jrd_115;	// gds__utility 
          SSHORT jrd_116;	// RDB$FILE_FLAGS 
   } jrd_114;
   struct {
          SSHORT jrd_113;	// RDB$SHADOW_NUMBER 
   } jrd_112;
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
	Attachment* attachment = tdbb->getAttachment();

	AutoRequest handle;

	/*FOR(REQUEST_HANDLE handle)
		FIL IN RDB$FILES WITH FIL.RDB$SHADOW_NUMBER EQ shadow->sdw_number*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_111, sizeof(jrd_111));
	jrd_112.jrd_113 = shadow->sdw_number;
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_112);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_114);
	   if (!jrd_114.jrd_115) break;
	{
		/*MODIFY FIL USING*/
		{
		
			/*FIL.RDB$FILE_FLAGS*/
			jrd_114.jrd_116 = file_flags;
		/*END_MODIFY*/
		jrd_117.jrd_118 = jrd_114.jrd_116;
		EXE_send (tdbb, handle, 2, 2, (UCHAR*) &jrd_117);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_119);
	   }
	}
}


void MET_update_transaction(thread_db* tdbb, jrd_tra* transaction, const bool do_commit)
{
   struct {
          SSHORT jrd_110;	// gds__utility 
   } jrd_109;
   struct {
          SSHORT jrd_108;	// gds__utility 
   } jrd_107;
   struct {
          SSHORT jrd_106;	// RDB$TRANSACTION_STATE 
   } jrd_105;
   struct {
          SSHORT jrd_103;	// gds__utility 
          SSHORT jrd_104;	// RDB$TRANSACTION_STATE 
   } jrd_102;
   struct {
          SLONG  jrd_101;	// RDB$TRANSACTION_ID 
   } jrd_100;
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
	Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, irq_m_trans, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$TRANSACTIONS
		WITH X.RDB$TRANSACTION_ID EQ transaction->tra_number*/
	{
	request.compile(tdbb, (UCHAR*) jrd_99, sizeof(jrd_99));
	jrd_100.jrd_101 = transaction->tra_number;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 4, (UCHAR*) &jrd_100);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_102);
	   if (!jrd_102.jrd_103) break;
	{
		if (do_commit && (transaction->tra_flags & TRA_prepare2))
			/*ERASE X*/
			EXE_send (tdbb, request, 4, 2, (UCHAR*) &jrd_109);
		else
		{
			/*MODIFY X*/
			{
			
				/*X.RDB$TRANSACTION_STATE*/
				jrd_102.jrd_104 = do_commit ?
					/*RDB$TRANSACTIONS.RDB$TRANSACTION_STATE.COMMITTED*/
					2 :
					/*RDB$TRANSACTIONS.RDB$TRANSACTION_STATE.ROLLED_BACK*/
					3;
			/*END_MODIFY*/
			jrd_105.jrd_106 = jrd_102.jrd_104;
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_105);
			}
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_107);
	   }
	}
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
	DSqlCacheItem* const item = static_cast<DSqlCacheItem*>(ast_object);

	try
	{
		Database* const dbb = item->lock->lck_dbb;

		AsyncContextHolder tdbb(dbb, FB_FUNCTION, item->lock);

		item->obsolete = true;
		item->locked = false;
		LCK_release(tdbb, item->lock);
	}
	catch (const Exception&)
	{} // no-op

	return 0;
}


static DSqlCacheItem* get_dsql_cache_item(thread_db* tdbb, int type, const QualifiedName& name)
{
	Database* dbb = tdbb->getDatabase();
	Attachment* attachment = tdbb->getAttachment();

	string key((char*) &type, sizeof(type));
	int len = name.identifier.length();
	key.append((char*) &len, sizeof(len));
	key.append(name.identifier.c_str(), len);

	len = name.package.length();
	key.append((char*) &len, sizeof(len));
	key.append(name.package.c_str(), len);

	DSqlCacheItem* item = attachment->att_dsql_cache.put(key);
	if (item)
	{
		item->obsolete = false;
		item->locked = false;
		item->lock = FB_NEW_RPT(*dbb->dbb_permanent, key.length())
			Lock(tdbb, key.length(), LCK_dsql_cache, item, blocking_ast_dsql_cache);
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
	jrd_prc* const procedure = static_cast<jrd_prc*>(ast_object);

	try
	{
		if (procedure->existenceLock)
		{
			Database* const dbb = procedure->existenceLock->lck_dbb;

			AsyncContextHolder tdbb(dbb, FB_FUNCTION, procedure->existenceLock);

			LCK_release(tdbb, procedure->existenceLock);
		}
		procedure->flags |= Routine::FLAG_OBSOLETE;
	}
	catch (const Exception&)
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
	jrd_rel* const relation = static_cast<jrd_rel*>(ast_object);

	try
	{
		if (relation->rel_existence_lock)
		{
			Database* const dbb = relation->rel_existence_lock->lck_dbb;

			AsyncContextHolder tdbb(dbb, FB_FUNCTION, relation->rel_existence_lock);

			if (relation->rel_use_count)
				relation->rel_flags |= REL_blocking;
			else
			{
				relation->rel_flags &= ~REL_blocking;
				relation->rel_flags |= REL_check_existence;
				LCK_release(tdbb, relation->rel_existence_lock);
			}
		}
	}
	catch (const Exception&)
	{} // no-op

	return 0;
}


static int partners_ast_relation(void* ast_object)
{
	jrd_rel* const relation = static_cast<jrd_rel*>(ast_object);

	try
	{
		Database* const dbb = relation->rel_partners_lock->lck_dbb;

		AsyncContextHolder tdbb(dbb, FB_FUNCTION, relation->rel_partners_lock);

		LCK_release(tdbb, relation->rel_partners_lock);
		relation->rel_flags |= REL_check_partners;
	}
	catch (const Exception&)
	{} // no-op

	return 0;
}


static int rescan_ast_relation(void* ast_object)
{
	jrd_rel* const relation = static_cast<jrd_rel*>(ast_object);

	try
	{
		Database* const dbb = relation->rel_rescan_lock->lck_dbb;

		AsyncContextHolder tdbb(dbb, FB_FUNCTION, relation->rel_rescan_lock);

		LCK_release(tdbb, relation->rel_rescan_lock);
		relation->rel_flags &= ~REL_scanned;
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
 *      M E T _g e t _ r e l _ f l a g s _ f r o m _ T Y P E
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
						const TEXT* name, FB_UINT64 type,
						bool sys_trigger, USHORT flags,
						const MetaName& engine, const string& entryPoint,
						const bid* body)
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
	Attachment* attachment = tdbb->getAttachment();

	if (blob_id->isEmpty() && (engine.isEmpty() || entryPoint.isEmpty()))
		return;

	blb* blrBlob = NULL;

	if (!blob_id->isEmpty())
		blrBlob = blb::open(tdbb, attachment->getSysTransaction(), blob_id);

	save_trigger_data(tdbb, ptr, relation, NULL, blrBlob, debug_blob_id,
					  name, type, sys_trigger, flags, engine, entryPoint, body);
}


static bool get_type(thread_db* tdbb, USHORT* id, const UCHAR* name, const TEXT* field)
{
   struct {
          SSHORT jrd_97;	// gds__utility 
          SSHORT jrd_98;	// RDB$TYPE 
   } jrd_96;
   struct {
          TEXT  jrd_94 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_95 [32];	// RDB$FIELD_NAME 
   } jrd_93;
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
	UCHAR buffer[MAX_SQL_IDENTIFIER_SIZE];			// BASED ON RDB$TYPE_NAME

	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();

	fb_assert(id != NULL);
	fb_assert(name != NULL);
	fb_assert(field != NULL);

	// Force key to uppercase, following C locale rules for uppercase
	UCHAR* p;
	for (p = buffer; *name && p < buffer + sizeof(buffer) - 1; p++, name++)
	{
		*p = UPPER7(*name);
	}
	*p = 0;

	// Try for exact name match

	bool found = false;

	AutoRequest handle;

	/*FOR(REQUEST_HANDLE handle)
		FIRST 1 T IN RDB$TYPES WITH
			T.RDB$FIELD_NAME EQ field AND
			T.RDB$TYPE_NAME EQ buffer*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_92, sizeof(jrd_92));
	gds__vtov ((const char*) buffer, (char*) jrd_93.jrd_94, 32);
	gds__vtov ((const char*) field, (char*) jrd_93.jrd_95, 32);
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_93);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_96);
	   if (!jrd_96.jrd_97) break;
	{
		found = true;
		*id = /*T.RDB$TYPE*/
		      jrd_96.jrd_98;
	}
	/*END_FOR*/
	   }
	}

	return found;
}


static void lookup_view_contexts( thread_db* tdbb, jrd_rel* view)
{
   struct {
          TEXT  jrd_86 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_87 [256];	// RDB$CONTEXT_NAME 
          SSHORT jrd_88;	// gds__utility 
          SSHORT jrd_89;	// gds__null_flag 
          SSHORT jrd_90;	// RDB$CONTEXT_TYPE 
          SSHORT jrd_91;	// RDB$VIEW_CONTEXT 
   } jrd_85;
   struct {
          TEXT  jrd_84 [32];	// RDB$VIEW_NAME 
   } jrd_83;
/**************************************
 *
 *      l o o k u p _ v i e w _ c o n t e x t s
 *
 **************************************
 *
 * Functional description
 *      Lookup view contexts and store in a sorted
 *      array on the relation block.
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();
	AutoCacheRequest request(tdbb, irq_view_context, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		V IN RDB$VIEW_RELATIONS WITH
			V.RDB$VIEW_NAME EQ view->rel_name.c_str()
			SORTED BY V.RDB$VIEW_CONTEXT*/
	{
	request.compile(tdbb, (UCHAR*) jrd_82, sizeof(jrd_82));
	gds__vtov ((const char*) view->rel_name.c_str(), (char*) jrd_83.jrd_84, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_83);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 296, (UCHAR*) &jrd_85);
	   if (!jrd_85.jrd_88) break;
	{
		// trim trailing spaces
		fb_utils::exact_name_limit(/*V.RDB$CONTEXT_NAME*/
					   jrd_85.jrd_87, sizeof(/*V.RDB$CONTEXT_NAME*/
	 jrd_85.jrd_87));

		ViewContext* view_context = FB_NEW(*dbb->dbb_permanent)
			ViewContext(*dbb->dbb_permanent,
				/*V.RDB$CONTEXT_NAME*/
				jrd_85.jrd_87, /*V.RDB$RELATION_NAME*/
  jrd_85.jrd_86, /*V.RDB$VIEW_CONTEXT*/
  jrd_85.jrd_91,
				(/*V.RDB$CONTEXT_TYPE.NULL*/
				 jrd_85.jrd_89 ? VCT_TABLE : ViewContextType(/*V.RDB$CONTEXT_TYPE*/
			       jrd_85.jrd_90)));

		view->rel_view_contexts.add(view_context);
	}
	/*END_FOR*/
	   }
	}
}


static void make_relation_scope_name(const TEXT* rel_name, const USHORT rel_flags,
	string& str)
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


// Parses default BLR for a field.
static ValueExprNode* parse_field_default_blr(thread_db* tdbb, bid* blob_id)
{
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();

	CompilerScratch* csb = CompilerScratch::newCsb(*tdbb->getDefaultPool(), 5);

	blb* blob = blb::open(tdbb, attachment->getSysTransaction(), blob_id);
	ULONG length = blob->blb_length + 10;
	HalfStaticArray<UCHAR, 512> temp;

	length = blob->BLB_get_data(tdbb, temp.getBuffer(length), length);

	DmlNode* node = PAR_blr(tdbb, NULL, temp.begin(), length, NULL, &csb, NULL, false, 0);

	csb->csb_blr_reader = BlrReader();
	delete csb;

	return static_cast<ValueExprNode*>(node);
}


// Parses validation BLR for a field.
static BoolExprNode* parse_field_validation_blr(thread_db* tdbb, bid* blob_id, const MetaName name)
{
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();

	CompilerScratch* csb = CompilerScratch::newCsb(*tdbb->getDefaultPool(), 5, name);

	blb* blob = blb::open(tdbb, attachment->getSysTransaction(), blob_id);
	ULONG length = blob->blb_length + 10;
	HalfStaticArray<UCHAR, 512> temp;

	length = blob->BLB_get_data(tdbb, temp.getBuffer(length), length);

	BoolExprNode* expr = PAR_validation_blr(tdbb, NULL,
		temp.begin(), length, NULL, &csb, 0);

	csb->csb_blr_reader = BlrReader();
	delete csb;

	return expr;
}


void MET_release_trigger(thread_db* tdbb, trig_vec** vector_ptr, const MetaName& name)
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
			JrdStatement* stmt = vector[i].statement;
			if (stmt)
			{
				if (stmt->isActive())
					break;
				stmt->release(tdbb);
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
		JrdStatement* stmt = (*vector)[i].statement;
		if (stmt && stmt->isActive())
			return;
	}

	for (size_t i = 0; i < vector->getCount(); i++)
	{
		JrdStatement* stmt = (*vector)[i].statement;
		if (stmt)
			stmt->release(tdbb);

		delete (*vector)[i].extTrigger;
	}

	delete vector;
}


static bool resolve_charset_and_collation(thread_db* tdbb,
										  USHORT* id,
										  const UCHAR* charset,
										  const UCHAR* collation)
{
   struct {
          SSHORT jrd_66;	// gds__utility 
          SSHORT jrd_67;	// RDB$COLLATION_ID 
          SSHORT jrd_68;	// RDB$CHARACTER_SET_ID 
   } jrd_65;
   struct {
          TEXT  jrd_63 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_64 [32];	// RDB$TYPE_NAME 
   } jrd_62;
   struct {
          SSHORT jrd_73;	// gds__utility 
          SSHORT jrd_74;	// RDB$COLLATION_ID 
          SSHORT jrd_75;	// RDB$CHARACTER_SET_ID 
   } jrd_72;
   struct {
          TEXT  jrd_71 [32];	// RDB$COLLATION_NAME 
   } jrd_70;
   struct {
          SSHORT jrd_80;	// gds__utility 
          SSHORT jrd_81;	// RDB$CHARACTER_SET_ID 
   } jrd_79;
   struct {
          TEXT  jrd_78 [32];	// RDB$CHARACTER_SET_NAME 
   } jrd_77;
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
	Attachment* attachment = tdbb->getAttachment();

	fb_assert(id != NULL);

	AutoRequest handle;

	if (!collation)
	{
		if (charset == NULL)
			charset = (const UCHAR*) DEFAULT_CHARACTER_SET_NAME;

		if (attachment->att_charset_ids.get((const TEXT*) charset, *id))
			return true;

		USHORT charset_id = 0;
		if (get_type(tdbb, &charset_id, charset, "RDB$CHARACTER_SET_NAME"))
		{
			attachment->att_charset_ids.put((const TEXT*) charset, charset_id);
			*id = charset_id;
			return true;
		}

		// Charset name not found in the alias table - before giving up
		// try the character_set table

		/*FOR(REQUEST_HANDLE handle)
			FIRST 1 CS IN RDB$CHARACTER_SETS
				WITH CS.RDB$CHARACTER_SET_NAME EQ charset*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_76, sizeof(jrd_76));
		gds__vtov ((const char*) charset, (char*) jrd_77.jrd_78, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_77);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_79);
		   if (!jrd_79.jrd_80) break;
		{
            found = true;
			attachment->att_charset_ids.put((const TEXT*) charset, /*CS.RDB$CHARACTER_SET_ID*/
									       jrd_79.jrd_81);
			*id = /*CS.RDB$CHARACTER_SET_ID*/
			      jrd_79.jrd_81;
		}
		/*END_FOR*/
		   }
		}

		return found;
	}

	if (!charset)
	{
		/*FOR(REQUEST_HANDLE handle)
			FIRST 1 COL IN RDB$COLLATIONS
				WITH COL.RDB$COLLATION_NAME EQ collation*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_69, sizeof(jrd_69));
		gds__vtov ((const char*) collation, (char*) jrd_70.jrd_71, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_70);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 6, (UCHAR*) &jrd_72);
		   if (!jrd_72.jrd_73) break;
		{
			found = true;
			*id = /*COL.RDB$CHARACTER_SET_ID*/
			      jrd_72.jrd_75 | (/*COL.RDB$COLLATION_ID*/
    jrd_72.jrd_74 << 8);
		}
		/*END_FOR*/
		   }
		}

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
	handle.compile(tdbb, (UCHAR*) jrd_61, sizeof(jrd_61));
	gds__vtov ((const char*) collation, (char*) jrd_62.jrd_63, 32);
	gds__vtov ((const char*) charset, (char*) jrd_62.jrd_64, 32);
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_62);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 6, (UCHAR*) &jrd_65);
	   if (!jrd_65.jrd_66) break;
	{
        found = true;
		attachment->att_charset_ids.put((const TEXT*) charset, /*CS.RDB$CHARACTER_SET_ID*/
								       jrd_65.jrd_68);
		*id = /*CS.RDB$CHARACTER_SET_ID*/
		      jrd_65.jrd_68 | (/*COL.RDB$COLLATION_ID*/
    jrd_65.jrd_67 << 8);
	}
	/*END_FOR*/
	   }
	}

	return found;
}


static void save_trigger_data(thread_db* tdbb, trig_vec** ptr, jrd_rel* relation,
							  JrdStatement* statement, blb* blrBlob, bid* dbgBlobID,
							  const TEXT* name, FB_UINT64 type,
							  bool sys_trigger, USHORT flags,
							  const MetaName& engine, const string& entryPoint,
							  const bid* body)
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
	Attachment* attachment = tdbb->getAttachment();
	trig_vec* vector = *ptr;

	if (!vector)
	{
		MemoryPool* pool = relation ? relation->rel_pool : attachment->att_pool;
		vector = FB_NEW(*pool) trig_vec(*pool);
		*ptr = vector;
	}

	Trigger& t = vector->add();
	if (blrBlob)
	{
		const ULONG length = blrBlob->blb_length + 10;
		UCHAR* ptr2 = t.blr.getBuffer(length);
		t.blr.resize(blrBlob->BLB_get_data(tdbb, ptr2, length));
	}

	if (dbgBlobID)
		t.dbg_blob_id = *dbgBlobID;

	if (name)
		t.name = name;

	if (body)
	{
		blb* bodyBlob = blb::open(tdbb, attachment->getSysTransaction(), body);

		HalfStaticArray<char, 512> temp;
		ULONG length = bodyBlob->BLB_get_data(tdbb, (UCHAR*) temp.getBuffer(bodyBlob->blb_length),
			bodyBlob->blb_length);

		t.extBody.assign(temp.begin(), length);
	}

	t.type = type;
	t.flags = flags;
	t.compile_in_progress = false;
	t.sys_trigger = sys_trigger;
	t.statement = statement;
	t.relation = relation;
	t.engine = engine;
	t.entryPoint = entryPoint;
}


const Trigger* findTrigger(trig_vec* triggers, const MetaName& trig_name)
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


void scan_partners(thread_db* tdbb, jrd_rel* relation)
{
   struct {
          TEXT  jrd_44 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_45;	// gds__utility 
          SSHORT jrd_46;	// RDB$INDEX_ID 
          SSHORT jrd_47;	// RDB$INDEX_ID 
          SSHORT jrd_48;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_49;	// RDB$INDEX_INACTIVE 
   } jrd_43;
   struct {
          TEXT  jrd_42 [32];	// RDB$RELATION_NAME 
   } jrd_41;
   struct {
          TEXT  jrd_55 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_56;	// gds__utility 
          SSHORT jrd_57;	// RDB$INDEX_ID 
          SSHORT jrd_58;	// RDB$INDEX_ID 
          SSHORT jrd_59;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_60;	// RDB$INDEX_INACTIVE 
   } jrd_54;
   struct {
          TEXT  jrd_52 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_53 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_51;
/**************************************
 *
 *      s c a n _ p a r t n e r s
 *
 **************************************
 *
 * Functional description
 *      Scan of foreign references on other relations' primary keys and
 *      scan of primary dependencies on relation's primary key.
 *
 **************************************/
	Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, irq_foreign1, IRQ_REQUESTS);
	frgn* references = &relation->rel_foreign_refs;
	int index_number = 0;

	if (references->frgn_reference_ids)
	{
		delete references->frgn_reference_ids;
		references->frgn_reference_ids = NULL;
	}
	if (references->frgn_relations)
	{
		delete references->frgn_relations;
		references->frgn_relations = NULL;
	}
	if (references->frgn_indexes)
	{
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
	request.compile(tdbb, (UCHAR*) jrd_50, sizeof(jrd_50));
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_51.jrd_52, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_51.jrd_53, 12);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 44, (UCHAR*) &jrd_51);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 42, (UCHAR*) &jrd_54);
	   if (!jrd_54.jrd_56) break;
	{
		//// ASF: Hack fix for CORE-4304, until nasty interactions between dfw and met are not resolved.
		const jrd_rel* partner_relation = relation->rel_name == /*IND.RDB$RELATION_NAME*/
									jrd_54.jrd_55 ?
			relation : MET_lookup_relation(tdbb, /*IND.RDB$RELATION_NAME*/
							     jrd_54.jrd_55);

		if (partner_relation && !/*IDX.RDB$INDEX_INACTIVE*/
					 jrd_54.jrd_60 && !/*IND.RDB$INDEX_INACTIVE*/
     jrd_54.jrd_59)
		{
			// This seems a good candidate for vcl.
			references->frgn_reference_ids =
				vec<int>::newVector(*relation->rel_pool, references->frgn_reference_ids,
							   index_number + 1);

			(*references->frgn_reference_ids)[index_number] = /*IDX.RDB$INDEX_ID*/
									  jrd_54.jrd_58 - 1;

			references->frgn_relations =
				vec<int>::newVector(*relation->rel_pool, references->frgn_relations,
							   index_number + 1);

			(*references->frgn_relations)[index_number] = partner_relation->rel_id;

			references->frgn_indexes =
				vec<int>::newVector(*relation->rel_pool, references->frgn_indexes,
							   index_number + 1);

			(*references->frgn_indexes)[index_number] = /*IND.RDB$INDEX_ID*/
								    jrd_54.jrd_57 - 1;

			index_number++;
		}
	}
	/*END_FOR*/
	   }
	}

	// Prepare for rescan of primary dependencies on relation's primary key and stale vectors.

	request.reset(tdbb, irq_foreign2, IRQ_REQUESTS);
	prim* dependencies = &relation->rel_primary_dpnds;
	index_number = 0;

	if (dependencies->prim_reference_ids)
	{
		delete dependencies->prim_reference_ids;
		dependencies->prim_reference_ids = NULL;
	}
	if (dependencies->prim_relations)
	{
		delete dependencies->prim_relations;
		dependencies->prim_relations = NULL;
	}
	if (dependencies->prim_indexes)
	{
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
	request.compile(tdbb, (UCHAR*) jrd_40, sizeof(jrd_40));
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_41.jrd_42, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_41);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 42, (UCHAR*) &jrd_43);
	   if (!jrd_43.jrd_45) break;
	{
		//// ASF: Hack fix for CORE-4304, until nasty interactions between dfw and met are not resolved.
		const jrd_rel* partner_relation = relation->rel_name == /*IND.RDB$RELATION_NAME*/
									jrd_43.jrd_44 ?
			relation : MET_lookup_relation(tdbb, /*IND.RDB$RELATION_NAME*/
							     jrd_43.jrd_44);

		if (partner_relation && !/*IDX.RDB$INDEX_INACTIVE*/
					 jrd_43.jrd_49 && !/*IND.RDB$INDEX_INACTIVE*/
     jrd_43.jrd_48)
		{
			dependencies->prim_reference_ids =
				vec<int>::newVector(*relation->rel_pool, dependencies->prim_reference_ids,
							   index_number + 1);

			(*dependencies->prim_reference_ids)[index_number] = /*IDX.RDB$INDEX_ID*/
									    jrd_43.jrd_47 - 1;

			dependencies->prim_relations =
				vec<int>::newVector(*relation->rel_pool, dependencies->prim_relations,
							   index_number + 1);

			(*dependencies->prim_relations)[index_number] = partner_relation->rel_id;

			dependencies->prim_indexes =
				vec<int>::newVector(*relation->rel_pool, dependencies->prim_indexes,
							   index_number + 1);

			(*dependencies->prim_indexes)[index_number] = /*IND.RDB$INDEX_ID*/
								      jrd_43.jrd_46 - 1;

			index_number++;
		}
	}
	/*END_FOR*/
	   }
	}

	LCK_lock(tdbb, relation->rel_partners_lock, LCK_SR, LCK_WAIT);
	relation->rel_flags &= ~REL_check_partners;
}


static void store_dependencies(thread_db* tdbb,
							   CompilerScratch* csb,
							   const jrd_rel* dep_rel,
							   const MetaName& object_name,
							   int dependency_type,
							   jrd_tra* transaction)
{
   struct {
          TEXT  jrd_14 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_15 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_16 [32];	// RDB$DEPENDED_ON_NAME 
          TEXT  jrd_17 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_18;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_19;	// gds__null_flag 
          SSHORT jrd_20;	// gds__null_flag 
          SSHORT jrd_21;	// RDB$DEPENDED_ON_TYPE 
   } jrd_13;
   struct {
          SSHORT jrd_30;	// gds__utility 
   } jrd_29;
   struct {
          TEXT  jrd_24 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_25 [32];	// RDB$DEPENDED_ON_NAME 
          TEXT  jrd_26 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_27;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_28;	// RDB$DEPENDED_ON_TYPE 
   } jrd_23;
   struct {
          SSHORT jrd_39;	// gds__utility 
   } jrd_38;
   struct {
          TEXT  jrd_33 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_34 [32];	// RDB$DEPENDED_ON_NAME 
          TEXT  jrd_35 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_36;	// RDB$DEPENDENT_TYPE 
          SSHORT jrd_37;	// RDB$DEPENDED_ON_TYPE 
   } jrd_32;
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
	MetaName name;

	SET_TDBB(tdbb);

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
		CompilerScratch::Dependency dependency = csb->csb_dependencies.pop();

		if (!dependency.relation && !dependency.function && !dependency.procedure &&
			!dependency.name && !dependency.number)
		{
			continue;
		}

		int dpdo_type = dependency.objType;
		jrd_rel* relation = NULL;
		const jrd_prc* procedure = NULL;
		const MetaName* dpdo_name = NULL;
		MetaName packageName;
		SubtypeInfo info;

		switch (dpdo_type)
		{
		case obj_relation:
			relation = dependency.relation;
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
					string sMaster, sChild;

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
			procedure = dependency.procedure;
			dpdo_name = &procedure->getName().identifier;
			packageName = procedure->getName().package;
			break;
		case obj_collation:
			{
				const USHORT number = dependency.number;
				MET_get_char_coll_subtype_info(tdbb, number, &info);
				dpdo_name = &info.collationName;
			}
			break;
		case obj_exception:
			{
				const SLONG number = dependency.number;
				MET_lookup_exception(tdbb, number, name, NULL);
				dpdo_name = &name;
			}
			break;
		case obj_field:
			dpdo_name = dependency.name;
			break;
		case obj_generator:
			{
				// CVC: Here I'm going to track those pesky things named generators and UDFs.
				// But don't track sys gens.
				bool sysGen = false;
				const SLONG number = dependency.number;
				if (number == 0 || !MET_lookup_generator_id(tdbb, number, name, &sysGen) || sysGen)
					continue;
				dpdo_name = &name;
			}
			break;
		case obj_udf:
			{
				const Function* const udf = dependency.function;
				dpdo_name = &udf->getName().identifier;
				packageName = udf->getName().package;
			}
			break;
		case obj_index:
			name = *dependency.name;
			dpdo_name = &name;
			break;
		}

		MetaName field_name;

		if (dependency.subNumber || dependency.subName)
		{
			if (dependency.subNumber)
			{
				const SSHORT fld_id = (SSHORT) dependency.subNumber;
				if (relation)
				{
					const jrd_fld* field = MET_get_field(relation, fld_id);
					if (field)
						field_name = field->fld_name;
				}
				else if (procedure)
				{
					const Parameter* param = procedure->getOutputFields()[fld_id];
					// CVC: Setting the field var here didn't make sense alone,
					// so I thought the missing code was to try to extract
					// the field name that's in this case an output var from a proc.
					if (param)
						field_name = param->prm_name;
				}
			}
			else
				field_name = *dependency.subName;
		}

		if (field_name.hasData())
		{
			AutoCacheRequest request(tdbb, irq_c_deps_f, IRQ_REQUESTS);
			bool found = false;
			fb_assert(dpdo_name);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction) X IN RDB$DEPENDENCIES WITH
				X.RDB$DEPENDENT_NAME = object_name.c_str() AND
					X.RDB$DEPENDED_ON_NAME = dpdo_name->c_str() AND
					X.RDB$DEPENDED_ON_TYPE = dpdo_type AND
					X.RDB$FIELD_NAME = field_name.c_str() AND
					X.RDB$DEPENDENT_TYPE = dependency_type*/
			{
			request.compile(tdbb, (UCHAR*) jrd_31, sizeof(jrd_31));
			gds__vtov ((const char*) field_name.c_str(), (char*) jrd_32.jrd_33, 32);
			gds__vtov ((const char*) dpdo_name->c_str(), (char*) jrd_32.jrd_34, 32);
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_32.jrd_35, 32);
			jrd_32.jrd_36 = dependency_type;
			jrd_32.jrd_37 = dpdo_type;
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 100, (UCHAR*) &jrd_32);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_38);
			   if (!jrd_38.jrd_39) break;
			{
				found = true;
			}
			/*END_FOR*/
			   }
			}

			if (found)
				continue;
		}
		else
		{
			AutoCacheRequest request(tdbb, irq_c_deps, IRQ_REQUESTS);
			bool found = false;

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction) X IN RDB$DEPENDENCIES WITH
				X.RDB$DEPENDENT_NAME = object_name.c_str() AND
					X.RDB$DEPENDED_ON_NAME = dpdo_name->c_str() AND
					X.RDB$DEPENDED_ON_TYPE = dpdo_type AND
					X.RDB$FIELD_NAME MISSING AND
					X.RDB$DEPENDENT_TYPE = dependency_type AND
					X.RDB$PACKAGE_NAME EQUIV NULLIF(packageName.c_str(), '')*/
			{
			request.compile(tdbb, (UCHAR*) jrd_22, sizeof(jrd_22));
			gds__vtov ((const char*) packageName.c_str(), (char*) jrd_23.jrd_24, 32);
			gds__vtov ((const char*) dpdo_name->c_str(), (char*) jrd_23.jrd_25, 32);
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_23.jrd_26, 32);
			jrd_23.jrd_27 = dependency_type;
			jrd_23.jrd_28 = dpdo_type;
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 100, (UCHAR*) &jrd_23);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_29);
			   if (!jrd_29.jrd_30) break;
			{
				found = true;
			}
			/*END_FOR*/
			   }
			}

			if (found)
				continue;
		}

		AutoCacheRequest request(tdbb, irq_s_deps, IRQ_REQUESTS);

		fb_assert(dpdo_name);

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction) DEP IN RDB$DEPENDENCIES*/
		{
		
		{
			strcpy(/*DEP.RDB$DEPENDENT_NAME*/
			       jrd_13.jrd_17, object_name.c_str());
			/*DEP.RDB$DEPENDED_ON_TYPE*/
			jrd_13.jrd_21 = dpdo_type;
			strcpy(/*DEP.RDB$DEPENDED_ON_NAME*/
			       jrd_13.jrd_16, dpdo_name->c_str());

			if (field_name.hasData())
			{
				/*DEP.RDB$FIELD_NAME.NULL*/
				jrd_13.jrd_20 = FALSE;
				strcpy(/*DEP.RDB$FIELD_NAME*/
				       jrd_13.jrd_15, field_name.c_str());
			}
			else
				/*DEP.RDB$FIELD_NAME.NULL*/
				jrd_13.jrd_20 = TRUE;

			if (packageName.hasData())
			{
				/*DEP.RDB$PACKAGE_NAME.NULL*/
				jrd_13.jrd_19 = FALSE;
				strcpy(/*DEP.RDB$PACKAGE_NAME*/
				       jrd_13.jrd_14, packageName.c_str());
			}
			else
				/*DEP.RDB$PACKAGE_NAME.NULL*/
				jrd_13.jrd_19 = TRUE;

			/*DEP.RDB$DEPENDENT_TYPE*/
			jrd_13.jrd_18 = dependency_type;
		}
		/*END_STORE*/
		request.compile(tdbb, (UCHAR*) jrd_12, sizeof(jrd_12));
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 136, (UCHAR*) &jrd_13);
		}
	}
}


static bool verify_TRG_ignore_perm(thread_db* tdbb, const MetaName& trig_name)
{
   struct {
          TEXT  jrd_9 [12];	// RDB$DELETE_RULE 
          TEXT  jrd_10 [12];	// RDB$UPDATE_RULE 
          SSHORT jrd_11;	// gds__utility 
   } jrd_8;
   struct {
          TEXT  jrd_7 [32];	// RDB$TRIGGER_NAME 
   } jrd_6;
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
	Attachment* attachment = tdbb->getAttachment();

	// See if this is a system trigger, with the flag set as TRG_ignore_perm

	if (INI_get_trig_flags(trig_name.c_str()) & TRG_ignore_perm)
	{
		return true;
	}

	// See if this is a RI trigger

	AutoCacheRequest request(tdbb, irq_c_trg_perm, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		CHK IN RDB$CHECK_CONSTRAINTS CROSS
			REF IN RDB$REF_CONSTRAINTS WITH
			CHK.RDB$TRIGGER_NAME EQ trig_name.c_str() AND
			REF.RDB$CONSTRAINT_NAME = CHK.RDB$CONSTRAINT_NAME*/
	{
	request.compile(tdbb, (UCHAR*) jrd_5, sizeof(jrd_5));
	gds__vtov ((const char*) trig_name.c_str(), (char*) jrd_6.jrd_7, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_6);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 26, (UCHAR*) &jrd_8);
	   if (!jrd_8.jrd_11) break;
	{
		fb_utils::exact_name_limit(/*REF.RDB$UPDATE_RULE*/
					   jrd_8.jrd_10, sizeof(/*REF.RDB$UPDATE_RULE*/
	 jrd_8.jrd_10));
		fb_utils::exact_name_limit(/*REF.RDB$DELETE_RULE*/
					   jrd_8.jrd_9, sizeof(/*REF.RDB$DELETE_RULE*/
	 jrd_8.jrd_9));

		if (!strcmp(/*REF.RDB$UPDATE_RULE*/
			    jrd_8.jrd_10, RI_ACTION_CASCADE) ||
			!strcmp(/*REF.RDB$UPDATE_RULE*/
				jrd_8.jrd_10, RI_ACTION_NULL) ||
			!strcmp(/*REF.RDB$UPDATE_RULE*/
				jrd_8.jrd_10, RI_ACTION_DEFAULT) ||
			!strcmp(/*REF.RDB$DELETE_RULE*/
				jrd_8.jrd_9, RI_ACTION_CASCADE) ||
			!strcmp(/*REF.RDB$DELETE_RULE*/
				jrd_8.jrd_9, RI_ACTION_NULL) ||
			!strcmp(/*REF.RDB$DELETE_RULE*/
				jrd_8.jrd_9, RI_ACTION_DEFAULT))
		{
			return true;
		}

		return false;
	}
	/*END_FOR*/
	   }
	}

	return false;
}

int MET_get_linger(thread_db* tdbb)
{
   struct {
          SLONG  jrd_2;	// RDB$LINGER 
          SSHORT jrd_3;	// gds__utility 
          SSHORT jrd_4;	// gds__null_flag 
   } jrd_1;
/**************************************
 *
 *	M E T _ g e t _ l i n g e r
 *
 **************************************
 *
 * Functional description
 *      Return linger value for current database
 *
 **************************************/
	SET_TDBB(tdbb);
	Attachment* attachment = tdbb->getAttachment();
	int rc = 0;

	AutoCacheRequest request(tdbb, irq_linger, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		DAT IN RDB$DATABASE*/
	{
	request.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
	EXE_start (tdbb, request, attachment->getSysTransaction());
	while (1)
	   {
	   EXE_receive (tdbb, request, 0, 8, (UCHAR*) &jrd_1);
	   if (!jrd_1.jrd_3) break;
	{
		if (!/*DAT.RDB$LINGER.NULL*/
		     jrd_1.jrd_4)
			rc = /*DAT.RDB$LINGER*/
			     jrd_1.jrd_2;
	}
	/*END_FOR*/
	   }
	}

	return rc;
}
