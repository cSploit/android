/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
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
#include "dyn_consts.h"
#include "../dsql/DdlNodes.h"
#include "../dsql/BoolNodes.h"
#include "../dsql/ExprNodes.h"
#include "../dsql/StmtNodes.h"
#include "../jrd/blr.h"
#include "../jrd/btr.h"
#include "../jrd/dyn.h"
#include "../jrd/flags.h"
#include "../jrd/intl.h"
#include "../jrd/jrd.h"
#include "../jrd/msg_encode.h"
#include "../jrd/obj.h"
#include "../jrd/ods.h"
#include "../jrd/tra.h"
#include "../common/os/path_utils.h"
#include "../jrd/IntlManager.h"
#include "../jrd/PreparedStatement.h"
#include "../jrd/ResultSet.h"
#include "../jrd/UserManagement.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/dyn_ut_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/intl_proto.h"
#include "../common/isc_f_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/vio_proto.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/gen_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/metd_proto.h"
#include "../dsql/pass1_proto.h"
#include "../utilities/gsec/gsec.h"
#include "../common/dsc_proto.h"
#include "../common/StatusArg.h"
#include "../auth/SecureRemotePassword/Message.h"

namespace Jrd {

using namespace Firebird;

static void checkForeignKeyTempScope(thread_db* tdbb, jrd_tra* transaction,
	const MetaName&	childRelName, const MetaName& masterIndexName);
static void checkSpTrigDependency(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& fieldName);
static void checkViewDependency(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& fieldName);
static void clearPermanentField(dsql_rel* relation, bool permanent);
static void defineComputed(DsqlCompilerScratch* dsqlScratch, RelationSourceNode* relation,
	dsql_fld* field, ValueSourceClause* clause, string& source, BlrDebugWriter::BlrData& value);
static void deleteKeyConstraint(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& constraintName, const MetaName& indexName);
static void defineFile(thread_db* tdbb, jrd_tra* transaction, SLONG shadowNumber, bool manualShadow,
	bool conditionalShadow, SLONG& dbAlloc,
	const PathName& name, SLONG start, SLONG length);
static bool fieldExists(thread_db* tdbb, jrd_tra* transaction, const MetaName& relationName,
	const MetaName& fieldName);
static bool isItSqlRole(thread_db* tdbb, jrd_tra* transaction, const MetaName& inputName,
	MetaName& outputName);
static const char* getRelationScopeName(const rel_t type);
static void makeRelationScopeName(string& to, const MetaName& name, const rel_t type);
static void checkRelationType(const rel_t type, const MetaName& name);
static void checkFkPairTypes(const rel_t masterType, const MetaName& masterName,
	const rel_t childType, const MetaName& childName);
static void modifyLocalFieldPosition(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& fieldName, USHORT newPosition,
	USHORT existingPosition);
static rel_t relationType(SSHORT relationTypeNull, SSHORT relationType);
static void saveField(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, const MetaName& fieldName);
static void saveRelation(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	const MetaName& relationName, bool view, bool creating);
static void updateRdbFields(const TypeClause* type,
	SSHORT& fieldType,
	SSHORT& fieldLength,
	SSHORT& fieldSubTypeNull, SSHORT& fieldSubType,
	SSHORT& fieldScaleNull, SSHORT& fieldScale,
	SSHORT& characterSetIdNull, SSHORT& characterSetId,
	SSHORT& characterLengthNull, SSHORT& characterLength,
	SSHORT& fieldPrecisionNull, SSHORT& fieldPrecision,
	SSHORT& collationIdNull, SSHORT& collationId,
	SSHORT& segmentLengthNull, SSHORT& segmentLength);

static const char* const CHECK_CONSTRAINT_EXCEPTION = "check_constraint";

/*DATABASE DB = STATIC "ODS.RDB";*/
static const UCHAR	jrd_0 [53] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_long, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 10,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_5 [61] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 10,0, 0, 
            blr_end, 
         blr_send, 0, 
            blr_begin, 
               blr_assignment, 
                  blr_literal, blr_long, 0, 1,0,0,0,
                  blr_parameter, 0, 0,0, 
               blr_assignment, 
                  blr_fid, 0, 4,0, 
                  blr_parameter, 0, 1,0, 
               blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 0,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_9 [39] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 2,0, 
      blr_long, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 10,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 4,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_13 [160] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 5, 1,0, 
      blr_short, 0, 
   blr_message, 4, 1,0, 
      blr_short, 0, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 10,0, 0, 
            blr_end, 
         blr_begin, 
            blr_send, 0, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter2, 0, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 0, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 0, 3,0, 
                  blr_end, 
            blr_label, 0, 
               blr_loop, 
                  blr_select, 
                     blr_receive, 2, 
                        blr_leave, 0, 
                     blr_receive, 1, 
                        blr_modify, 0, 2, 
                           blr_begin, 
                              blr_assignment, 
                                 blr_parameter, 1, 0,0, 
                                 blr_fid, 2, 4,0, 
                              blr_end, 
                     blr_receive, 3, 
                        blr_erase, 0, 
                     blr_receive, 4, 
                        blr_modify, 0, 1, 
                           blr_begin, 
                              blr_assignment, 
                                 blr_parameter, 4, 0,0, 
                                 blr_fid, 1, 4,0, 
                              blr_end, 
                     blr_receive, 5, 
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
static const UCHAR	jrd_29 [144] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 1,0, 0, 
            blr_end, 
         blr_begin, 
            blr_send, 0, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter2, 0, 0,0, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter2, 0, 1,0, 3,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 0, 2,0, 
                  blr_end, 
            blr_label, 0, 
               blr_loop, 
                  blr_select, 
                     blr_receive, 2, 
                        blr_leave, 0, 
                     blr_receive, 1, 
                        blr_modify, 0, 1, 
                           blr_begin, 
                              blr_assignment, 
                                 blr_parameter2, 1, 1,0, 3,0, 
                                 blr_fid, 1, 4,0, 
                              blr_assignment, 
                                 blr_parameter2, 1, 0,0, 2,0, 
                                 blr_fid, 1, 3,0, 
                              blr_end, 
                     blr_end, 
            blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 2,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_43 [71] =
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
static const UCHAR	jrd_48 [156] =
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
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_missing, 
                           blr_fid, 0, 14,0, 
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
static const UCHAR	jrd_61 [124] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 9,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 0,0, 7,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 18,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 8,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter, 0, 6,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 7,0, 
               blr_fid, 0, 5,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_72 [135] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
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
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_upcase, 
                           blr_parameter, 0, 1,0, 
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
static const UCHAR	jrd_81 [144] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 5,0, 0, 
               blr_rid, 7,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_not, 
                           blr_missing, 
                              blr_fid, 0, 4,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 0,0, 
                              blr_fid, 0, 1,0, 
                           blr_eql, 
                              blr_fid, 1, 2,0, 
                              blr_fid, 0, 10,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
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
static const UCHAR	jrd_89 [159] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 0,0, 7,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_upcase, 
                           blr_parameter, 0, 1,0, 
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
                                 blr_parameter, 0, 4,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter2, 1, 0,0, 4,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter2, 1, 3,0, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_102 [87] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 8,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 13,0, 
                        blr_upcase, 
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
static const UCHAR	jrd_108 [86] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
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
static const UCHAR	jrd_114 [86] =
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
                     blr_fid, 0, 15,0, 
                     blr_parameter2, 1, 2,0, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_121 [166] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 0,0, 7,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 4,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 7,0, 
                              blr_parameter, 0, 3,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 0,0, 
                                 blr_parameter, 0, 0,0, 
                              blr_eql, 
                                 blr_fid, 0, 6,0, 
                                 blr_parameter, 0, 2,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_135 [181] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
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
                        blr_parameter, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_parameter, 0, 4,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 2,0, 
                              blr_parameter, 0, 5,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 0,0, 
                                 blr_parameter, 0, 1,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 0, 6,0, 
                                    blr_parameter, 0, 3,0, 
                                 blr_eql, 
                                    blr_fid, 0, 5,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_150 [210] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 3,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 7,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
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
                        blr_parameter, 0, 3,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_parameter, 0, 5,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 2,0, 
                              blr_parameter, 0, 6,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 0,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 0, 6,0, 
                                    blr_parameter, 0, 4,0, 
                                 blr_and, 
                                    blr_eql, 
                                       blr_fid, 0, 1,0, 
                                       blr_parameter, 0, 1,0, 
                                    blr_equiv, 
                                       blr_fid, 0, 5,0, 
                                       blr_value_if, 
                                          blr_eql, 
                                             blr_parameter, 0, 0,0, 
                                             blr_literal, blr_text2, 3,0, 0,0, 
                                          blr_null, 
                                          blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter2, 1, 2,0, 1,0, 
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
static const UCHAR	jrd_167 [124] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 6,0, 
                        blr_parameter, 0, 1,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_178 [137] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
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
                  blr_or, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_parameter, 0, 3,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 6,0, 
                           blr_parameter, 0, 2,0, 
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
static const UCHAR	jrd_190 [123] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
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
               blr_rid, 31,0, 0, 
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
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter, 1, 2,0, 
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
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_201 [142] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 12,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 3,0, 2,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 45,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 6,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 8,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 9,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 4,0, 10,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 11,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_215 [331] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 4, 11,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 3,0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 12,0, 
      blr_cstring2, 3,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 3,0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 45,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter2, 1, 0,0, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter2, 1, 2,0, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter2, 1, 3,0, 8,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter2, 1, 4,0, 11,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 9,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter, 1, 10,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_erase, 0, 
                        blr_receive, 4, 
                           blr_modify, 0, 1, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 4, 4,0, 10,0, 
                                    blr_fid, 1, 5,0, 
                                 blr_assignment, 
                                    blr_parameter, 4, 3,0, 
                                    blr_fid, 1, 4,0, 
                                 blr_assignment, 
                                    blr_parameter2, 4, 2,0, 9,0, 
                                    blr_fid, 1, 3,0, 
                                 blr_assignment, 
                                    blr_parameter2, 4, 1,0, 8,0, 
                                    blr_fid, 1, 2,0, 
                                 blr_assignment, 
                                    blr_parameter, 4, 7,0, 
                                    blr_fid, 1, 1,0, 
                                 blr_assignment, 
                                    blr_parameter, 4, 6,0, 
                                    blr_fid, 1, 6,0, 
                                 blr_assignment, 
                                    blr_parameter2, 4, 0,0, 5,0, 
                                    blr_fid, 1, 7,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 5,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_247 [71] =
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
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 13,0, 
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
static const UCHAR	jrd_252 [110] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
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
                  blr_or, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 0, 6,0, 
                           blr_parameter, 0, 3,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_parameter, 0, 2,0, 
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
static const UCHAR	jrd_260 [56] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 31,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 3,0, 
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
static const UCHAR	jrd_265 [95] =
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
static const UCHAR	jrd_274 [76] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 10,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 5,0, 
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
static const UCHAR	jrd_279 [98] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 16,0, 0, 
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
static const UCHAR	jrd_288 [92] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 6,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 16,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_296 [113] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 3,0, 
      blr_quad, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_307 [98] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 3,0, 0, 
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
static const UCHAR	jrd_316 [127] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_double, 
      blr_short, 0, 
   blr_message, 1, 3,0, 
      blr_double, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
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
                                    blr_fid, 1, 12,0, 
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
static const UCHAR	jrd_328 [129] =
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
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
                                    blr_fid, 1, 6,0, 
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
static const UCHAR	jrd_340 [125] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 4,0, 0, 
               blr_rid, 22,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 5,0, 
                        blr_fid, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
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
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
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
static const UCHAR	jrd_348 [86] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_quad, 0, 
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
static const UCHAR	jrd_355 [176] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 22,0, 0, 
               blr_rid, 4,0, 1, 
               blr_rid, 3,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 0,0, 
                           blr_fid, 0, 5,0, 
                        blr_eql, 
                           blr_fid, 2, 0,0, 
                           blr_fid, 1, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_not, 
                              blr_missing, 
                                 blr_fid, 1, 3,0, 
                           blr_or, 
                              blr_eql, 
                                 blr_fid, 0, 1,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_eql, 
                                 blr_fid, 0, 1,0, 
                                 blr_parameter, 0, 1,0, 
               blr_sort, 2, 
                  blr_ascending, 
                     blr_fid, 1, 0,0, 
                  blr_descending, 
                     blr_fid, 2, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 2, 1,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_364 [56] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 3,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_369 [193] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 12,0, 
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
                        blr_fid, 1, 0,0, 
                        blr_fid, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_parameter, 0, 0,0, 
                           blr_fid, 0, 1,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 4,0, 
                     blr_parameter2, 1, 0,0, 8,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 1, 25,0, 
                     blr_parameter2, 1, 3,0, 2,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 1, 26,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 18,0, 
                     blr_parameter2, 1, 7,0, 6,0, 
                  blr_assignment, 
                     blr_fid, 1, 22,0, 
                     blr_parameter2, 1, 10,0, 9,0, 
                  blr_assignment, 
                     blr_fid, 1, 10,0, 
                     blr_parameter, 1, 11,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_386 [86] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_quad, 0, 
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
static const UCHAR	jrd_393 [168] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 18,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
   blr_receive, 0, 
      blr_store, 
         blr_rid, 4,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter2, 0, 7,0, 6,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 8,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 9,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 10,0, 
               blr_fid, 0, 11,0, 
            blr_assignment, 
               blr_parameter2, 0, 4,0, 11,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter2, 0, 13,0, 12,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter2, 0, 15,0, 14,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 17,0, 16,0, 
               blr_fid, 0, 3,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_413 [432] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 18,0, 
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
   blr_message, 1, 20,0, 
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
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_missing, 
                              blr_fid, 0, 4,0, 
                           blr_eql, 
                              blr_fid, 1, 0,0, 
                              blr_fid, 0, 2,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 1, 4,0, 
                        blr_parameter2, 1, 0,0, 3,0, 
                     blr_assignment, 
                        blr_fid, 1, 0,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 1, 17,0, 
                        blr_parameter2, 1, 5,0, 4,0, 
                     blr_assignment, 
                        blr_fid, 1, 25,0, 
                        blr_parameter2, 1, 7,0, 6,0, 
                     blr_assignment, 
                        blr_fid, 1, 27,0, 
                        blr_parameter2, 1, 9,0, 8,0, 
                     blr_assignment, 
                        blr_fid, 1, 24,0, 
                        blr_parameter2, 1, 11,0, 10,0, 
                     blr_assignment, 
                        blr_fid, 1, 26,0, 
                        blr_parameter2, 1, 13,0, 12,0, 
                     blr_assignment, 
                        blr_fid, 1, 9,0, 
                        blr_parameter2, 1, 15,0, 14,0, 
                     blr_assignment, 
                        blr_fid, 1, 11,0, 
                        blr_parameter2, 1, 17,0, 16,0, 
                     blr_assignment, 
                        blr_fid, 1, 8,0, 
                        blr_parameter, 1, 18,0, 
                     blr_assignment, 
                        blr_fid, 1, 10,0, 
                        blr_parameter, 1, 19,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 1, 2, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 17,0, 
                                    blr_fid, 2, 4,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 16,0, 15,0, 
                                    blr_fid, 2, 17,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 14,0, 13,0, 
                                    blr_fid, 2, 25,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 12,0, 11,0, 
                                    blr_fid, 2, 27,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 10,0, 9,0, 
                                    blr_fid, 2, 24,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 8,0, 7,0, 
                                    blr_fid, 2, 26,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 6,0, 5,0, 
                                    blr_fid, 2, 9,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 4,0, 3,0, 
                                    blr_fid, 2, 11,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 2,0, 
                                    blr_fid, 2, 8,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 1,0, 
                                    blr_fid, 2, 10,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_459 [147] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
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
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_missing, 
                              blr_fid, 0, 4,0, 
                           blr_eql, 
                              blr_fid, 1, 0,0, 
                              blr_fid, 0, 2,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 1, 0,0, 
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
                           blr_erase, 1, 
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
static const UCHAR	jrd_470 [119] =
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
            blr_rse, 2, 
               blr_rid, 7,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_missing, 
                           blr_fid, 0, 5,0, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 13,0, 
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
static const UCHAR	jrd_477 [99] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 7,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 7,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 4,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 6,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_486 [86] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 6,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 6,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 16,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 15,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 8,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_494 [110] =
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
               blr_rid, 12,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 8,0, 
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
static const UCHAR	jrd_504 [98] =
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
static const UCHAR	jrd_513 [150] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_quad, 0, 
      blr_quad, 0, 
   blr_message, 1, 3,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 8,0, 
                        blr_parameter, 0, 0,0, 
                     blr_not, 
                        blr_missing, 
                           blr_fid, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
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
                                    blr_parameter, 2, 1,0, 
                                    blr_fid, 1, 0,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 1,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_525 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 6,0, 
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
static const UCHAR	jrd_535 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 7,0, 
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
static const UCHAR	jrd_545 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 6,0, 
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
static const UCHAR	jrd_555 [112] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 2,0, 
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
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_565 [116] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
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
                        blr_fid, 0, 9,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_576 [98] =
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
static const UCHAR	jrd_585 [148] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
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
            blr_rse, 1, 
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 1,0, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 19,0, 
                        blr_parameter2, 1, 2,0, 5,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 3,0, 
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
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_599 [128] =
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
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 22,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 0,0, 
                     blr_or, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 2,0, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_610 [112] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 17,0, 0, 
               blr_rid, 12,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_619 [112] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
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
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_629 [150] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 22,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 0,0, 
                     blr_or, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 3,0, 
                        blr_or, 
                           blr_eql, 
                              blr_fid, 0, 1,0, 
                              blr_parameter, 0, 2,0, 
                           blr_eql, 
                              blr_fid, 0, 1,0, 
                              blr_parameter, 0, 1,0, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_641 [71] =
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
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_646 [191] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 2,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_missing, 
                           blr_fid, 0, 3,0, 
                        blr_and, 
                           blr_missing, 
                              blr_fid, 0, 23,0, 
                           blr_and, 
                              blr_missing, 
                                 blr_fid, 0, 7,0, 
                              blr_and, 
                                 blr_starting, 
                                    blr_fid, 0, 0,0, 
                                    blr_parameter, 0, 0,0, 
                                 blr_and, 
                                    blr_not, 
                                       blr_any, 
                                          blr_rse, 1, 
                                             blr_rid, 5,0, 1, 
                                             blr_boolean, 
                                                blr_eql, 
                                                   blr_fid, 1, 2,0, 
                                                   blr_fid, 0, 0,0, 
                                             blr_end, 
                                    blr_and, 
                                       blr_not, 
                                          blr_any, 
                                             blr_rse, 1, 
                                                blr_rid, 27,0, 2, 
                                                blr_boolean, 
                                                   blr_eql, 
                                                      blr_fid, 2, 4,0, 
                                                      blr_fid, 0, 0,0, 
                                                blr_end, 
                                       blr_not, 
                                          blr_any, 
                                             blr_rse, 1, 
                                                blr_rid, 15,0, 3, 
                                                blr_boolean, 
                                                   blr_eql, 
                                                      blr_fid, 3, 12,0, 
                                                      blr_fid, 0, 0,0, 
                                                blr_end, 
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
static const UCHAR	jrd_656 [142] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 15,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 19,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 18,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
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
                                    blr_fid, 1, 12,0, 
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
static const UCHAR	jrd_667 [142] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 27,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 13,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 12,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
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
                                    blr_fid, 1, 4,0, 
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
static const UCHAR	jrd_678 [97] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_int64, 0, 
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
                     blr_fid, 0, 6,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_686 [820] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 8, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 7, 4,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 6, 1,0, 
      blr_short, 0, 
   blr_message, 5, 16,0, 
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
   blr_message, 4, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 4,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 43,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
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
            blr_rse, 3, 
               blr_rid, 5,0, 0, 
               blr_rid, 6,0, 1, 
               blr_rid, 2,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 2, 0,0, 
                           blr_fid, 0, 2,0, 
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
                        blr_fid, 2, 5,0, 
                        blr_parameter2, 1, 0,0, 11,0, 
                     blr_assignment, 
                        blr_fid, 2, 0,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 2, 6,0, 
                        blr_parameter2, 1, 2,0, 25,0, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter2, 1, 3,0, 26,0, 
                     blr_assignment, 
                        blr_fid, 0, 17,0, 
                        blr_parameter2, 1, 4,0, 27,0, 
                     blr_assignment, 
                        blr_fid, 0, 19,0, 
                        blr_parameter2, 1, 5,0, 28,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter2, 1, 6,0, 29,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter2, 1, 7,0, 16,0, 
                     blr_assignment, 
                        blr_fid, 2, 4,0, 
                        blr_parameter2, 1, 8,0, 41,0, 
                     blr_assignment, 
                        blr_fid, 1, 0,0, 
                        blr_parameter2, 1, 9,0, 42,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 10,0, 
                     blr_assignment, 
                        blr_fid, 0, 18,0, 
                        blr_parameter2, 1, 13,0, 12,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter2, 1, 15,0, 14,0, 
                     blr_assignment, 
                        blr_fid, 2, 17,0, 
                        blr_parameter2, 1, 18,0, 17,0, 
                     blr_assignment, 
                        blr_fid, 2, 22,0, 
                        blr_parameter, 1, 30,0, 
                     blr_assignment, 
                        blr_fid, 2, 23,0, 
                        blr_parameter2, 1, 32,0, 31,0, 
                     blr_assignment, 
                        blr_fid, 2, 24,0, 
                        blr_parameter2, 1, 33,0, 21,0, 
                     blr_assignment, 
                        blr_fid, 2, 27,0, 
                        blr_parameter2, 1, 34,0, 20,0, 
                     blr_assignment, 
                        blr_fid, 2, 25,0, 
                        blr_parameter2, 1, 35,0, 19,0, 
                     blr_assignment, 
                        blr_fid, 2, 26,0, 
                        blr_parameter2, 1, 36,0, 22,0, 
                     blr_assignment, 
                        blr_fid, 2, 11,0, 
                        blr_parameter2, 1, 37,0, 24,0, 
                     blr_assignment, 
                        blr_fid, 2, 8,0, 
                        blr_parameter, 1, 38,0, 
                     blr_assignment, 
                        blr_fid, 2, 9,0, 
                        blr_parameter2, 1, 39,0, 23,0, 
                     blr_assignment, 
                        blr_fid, 2, 10,0, 
                        blr_parameter, 1, 40,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 2, 7, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 3,0, 
                                    blr_fid, 7, 5,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 2,0, 
                                    blr_fid, 7, 4,0, 
                                 blr_end, 
                        blr_receive, 4, 
                           blr_modify, 0, 6, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 4, 5,0, 4,0, 
                                    blr_fid, 6, 18,0, 
                                 blr_assignment, 
                                    blr_parameter2, 4, 3,0, 2,0, 
                                    blr_fid, 6, 8,0, 
                                 blr_assignment, 
                                    blr_parameter2, 4, 0,0, 1,0, 
                                    blr_fid, 6, 2,0, 
                                 blr_end, 
                        blr_receive, 5, 
                           blr_modify, 2, 5, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 5, 15,0, 14,0, 
                                    blr_fid, 5, 17,0, 
                                 blr_assignment, 
                                    blr_parameter2, 5, 13,0, 12,0, 
                                    blr_fid, 5, 24,0, 
                                 blr_assignment, 
                                    blr_parameter2, 5, 11,0, 10,0, 
                                    blr_fid, 5, 27,0, 
                                 blr_assignment, 
                                    blr_parameter2, 5, 9,0, 8,0, 
                                    blr_fid, 5, 25,0, 
                                 blr_assignment, 
                                    blr_parameter2, 5, 7,0, 6,0, 
                                    blr_fid, 5, 26,0, 
                                 blr_assignment, 
                                    blr_parameter2, 5, 5,0, 4,0, 
                                    blr_fid, 5, 11,0, 
                                 blr_assignment, 
                                    blr_parameter, 5, 3,0, 
                                    blr_fid, 5, 8,0, 
                                 blr_assignment, 
                                    blr_parameter2, 5, 2,0, 1,0, 
                                    blr_fid, 5, 9,0, 
                                 blr_assignment, 
                                    blr_parameter, 5, 0,0, 
                                    blr_fid, 5, 10,0, 
                                 blr_end, 
                        blr_receive, 6, 
                           blr_erase, 2, 
                        blr_receive, 7, 
                           blr_modify, 0, 4, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 7, 1,0, 3,0, 
                                    blr_fid, 4, 12,0, 
                                 blr_assignment, 
                                    blr_parameter2, 7, 0,0, 2,0, 
                                    blr_fid, 4, 17,0, 
                                 blr_end, 
                        blr_receive, 8, 
                           blr_modify, 0, 3, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 8, 2,0, 5,0, 
                                    blr_fid, 3, 12,0, 
                                 blr_assignment, 
                                    blr_parameter2, 8, 1,0, 4,0, 
                                    blr_fid, 3, 17,0, 
                                 blr_assignment, 
                                    blr_parameter2, 8, 0,0, 3,0, 
                                    blr_fid, 3, 19,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 10,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_779 [113] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 22,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_789 [97] =
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
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
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
static const UCHAR	jrd_796 [172] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 19,0, 
                        blr_parameter2, 1, 0,0, 3,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 16,0, 
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
                                    blr_parameter, 2, 2,0, 
                                    blr_fid, 1, 16,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 1, 19,0, 
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
static const UCHAR	jrd_811 [142] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
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
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
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
                                    blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_822 [112] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 10,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 6,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 4,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 5,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 6,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 16,0, 
            blr_assignment, 
               blr_parameter, 0, 8,0, 
               blr_fid, 0, 15,0, 
            blr_assignment, 
               blr_parameter, 0, 9,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 8,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_834 [170] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 22,0, 0, 
               blr_rid, 3,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 0,0, 
                        blr_fid, 0, 5,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 2,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_neq, 
                              blr_fid, 0, 0,0, 
                              blr_parameter, 0, 0,0, 
                           blr_or, 
                              blr_eql, 
                                 blr_fid, 0, 1,0, 
                                 blr_parameter, 0, 3,0, 
                              blr_eql, 
                                 blr_fid, 0, 1,0, 
                                 blr_parameter, 0, 2,0, 
               blr_sort, 2, 
                  blr_ascending, 
                     blr_fid, 0, 5,0, 
                  blr_descending, 
                     blr_fid, 1, 1,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
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
static const UCHAR	jrd_844 [115] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 22,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 5,0, 
                        blr_parameter, 0, 0,0, 
                     blr_or, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 2,0, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_852 [81] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 23,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 4,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 5,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 1,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_860 [71] =
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
               blr_rid, 3,0, 0, 
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
static const UCHAR	jrd_865 [193] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 7,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
            blr_rse, 3, 
               blr_rid, 3,0, 0, 
               blr_rid, 5,0, 1, 
               blr_rid, 2,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 0,0, 
                              blr_fid, 0, 1,0, 
                           blr_eql, 
                              blr_fid, 2, 0,0, 
                              blr_fid, 1, 2,0, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 1,0, 
               blr_project, 3, 
                  blr_fid, 0, 1,0, 
                  blr_fid, 0, 0,0, 
                  blr_fid, 2, 23,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 1, 16,0, 
                     blr_parameter2, 1, 4,0, 3,0, 
                  blr_assignment, 
                     blr_fid, 2, 23,0, 
                     blr_parameter2, 1, 6,0, 5,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_877 [77] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 22,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 4,0, 
               blr_fid, 0, 5,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_884 [125] =
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
                           blr_fid, 0, 5,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_parameter, 0, 2,0, 
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
static const UCHAR	jrd_895 [163] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 1,0, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 19,0, 
                        blr_parameter2, 1, 2,0, 5,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 3,0, 
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
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_910 [144] =
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
               blr_rid, 4,0, 0, 
               blr_rid, 3,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_fid, 1, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 1,0, 
                              blr_parameter, 0, 0,0, 
                           blr_not, 
                              blr_any, 
                                 blr_rse, 1, 
                                    blr_rid, 22,0, 2, 
                                    blr_boolean, 
                                       blr_and, 
                                          blr_eql, 
                                             blr_fid, 2, 2,0, 
                                             blr_fid, 0, 1,0, 
                                          blr_eql, 
                                             blr_fid, 2, 5,0, 
                                             blr_fid, 0, 0,0, 
                                    blr_end, 
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
static const UCHAR	jrd_917 [183] =
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
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 4,0, 0, 
               blr_rid, 3,0, 1, 
               blr_rid, 22,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 2, 2,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 1,0, 
                              blr_parameter, 0, 0,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 0,0, 
                                 blr_fid, 1, 0,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 0, 0,0, 
                                    blr_fid, 2, 5,0, 
                                 blr_eql, 
                                    blr_fid, 2, 1,0, 
                                    blr_parameter, 0, 3,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 2, 0,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
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
static const UCHAR	jrd_928 [158] =
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
            blr_rse, 3, 
               blr_rid, 5,0, 0, 
               blr_rid, 5,0, 1, 
               blr_rid, 7,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 0,0, 
                              blr_fid, 1, 4,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 2,0, 
                                 blr_fid, 1, 2,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 1, 1,0, 
                                    blr_fid, 2, 0,0, 
                                 blr_and, 
                                    blr_eql, 
                                       blr_fid, 0, 1,0, 
                                       blr_fid, 2, 1,0, 
                                    blr_eql, 
                                       blr_fid, 1, 10,0, 
                                       blr_fid, 2, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
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
static const UCHAR	jrd_935 [211] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 22,0, 
      blr_cstring2, 3,0, 32,0, 
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
   blr_receive, 0, 
      blr_store, 
         blr_rid, 5,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 7,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter2, 0, 9,0, 8,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 11,0, 10,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 12,0, 
               blr_fid, 0, 12,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 13,0, 
               blr_fid, 0, 17,0, 
            blr_assignment, 
               blr_parameter2, 0, 15,0, 14,0, 
               blr_fid, 0, 16,0, 
            blr_assignment, 
               blr_parameter2, 0, 17,0, 16,0, 
               blr_fid, 0, 20,0, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 18,0, 
               blr_fid, 0, 19,0, 
            blr_assignment, 
               blr_parameter2, 0, 20,0, 19,0, 
               blr_fid, 0, 18,0, 
            blr_assignment, 
               blr_parameter, 0, 21,0, 
               blr_fid, 0, 13,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 6,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_959 [424] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 19,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
   blr_message, 1, 20,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_quad, 0, 
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
                        blr_fid, 0, 4,0, 
                        blr_parameter2, 1, 0,0, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter2, 1, 1,0, 11,0, 
                     blr_assignment, 
                        blr_fid, 0, 17,0, 
                        blr_parameter2, 1, 2,0, 12,0, 
                     blr_assignment, 
                        blr_fid, 0, 19,0, 
                        blr_parameter2, 1, 3,0, 17,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter2, 1, 8,0, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter2, 1, 10,0, 9,0, 
                     blr_assignment, 
                        blr_fid, 0, 16,0, 
                        blr_parameter2, 1, 14,0, 13,0, 
                     blr_assignment, 
                        blr_fid, 0, 20,0, 
                        blr_parameter2, 1, 16,0, 15,0, 
                     blr_assignment, 
                        blr_fid, 0, 18,0, 
                        blr_parameter2, 1, 19,0, 18,0, 
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
                                    blr_parameter2, 2, 4,0, 18,0, 
                                    blr_fid, 1, 4,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 17,0, 16,0, 
                                    blr_fid, 1, 10,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 15,0, 14,0, 
                                    blr_fid, 1, 6,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 3,0, 13,0, 
                                    blr_fid, 1, 12,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 2,0, 12,0, 
                                    blr_fid, 1, 17,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 11,0, 10,0, 
                                    blr_fid, 1, 16,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 9,0, 8,0, 
                                    blr_fid, 1, 20,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 7,0, 
                                    blr_fid, 1, 19,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 6,0, 5,0, 
                                    blr_fid, 1, 18,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 2,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 5,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1006 [98] =
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
static const UCHAR	jrd_1015 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 7,0, 
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
static const UCHAR	jrd_1025 [127] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
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
               blr_rid, 20,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter2, 1, 0,0, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 3,0, 
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
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1037 [97] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 8,0, 
      blr_int64, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 20,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 4,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 5,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 6,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 1,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1047 [158] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_long, 0, 
   blr_message, 1, 6,0, 
      blr_int64, 0, 
      blr_long, 0, 
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
               blr_rid, 20,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter2, 1, 0,0, 3,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 5,0, 
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
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1061 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 7,0, 
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
static const UCHAR	jrd_1071 [116] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
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
               blr_rid, 30,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_1082 [127] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_cstring2, 0,0, 0,4, 
   blr_message, 1, 2,0, 
      blr_cstring2, 0,0, 0,4, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
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
                                    blr_fid, 1, 2,0, 
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
static const UCHAR	jrd_1092 [85] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 6,0, 
      blr_cstring2, 0,0, 0,4, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 30,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 4,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 1,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1100 [131] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 15,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 12,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter2, 1, 0,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
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
static const UCHAR	jrd_1110 [131] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 27,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 14,0, 
                     blr_parameter2, 1, 0,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
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
static const UCHAR	jrd_1120 [113] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
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
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 0, 0,0, 
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
static const UCHAR	jrd_1128 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 7,0, 
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
static const UCHAR	jrd_1138 [116] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
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
               blr_rid, 2,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 28,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_1149 [98] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 21,0, 0, 
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
static const UCHAR	jrd_1158 [155] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_message, 1, 4,0, 
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
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 3,0, 
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
                                    blr_fid, 1, 2,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1170 [127] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 21,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
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
                                    blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_1180 [71] =
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
               blr_rid, 2,0, 0, 
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
static const UCHAR	jrd_1185 [99] =
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
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 0, 0,0, 
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
static const UCHAR	jrd_1192 [78] =
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
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 0,0, 
                     blr_not, 
                        blr_missing, 
                           blr_fid, 0, 19,0, 
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
static const UCHAR	jrd_1197 [572] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 29,0, 
      blr_quad, 0, 
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
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 31,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_quad, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter2, 1, 1,0, 26,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter2, 1, 2,0, 27,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter2, 1, 3,0, 28,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter2, 1, 4,0, 29,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 17,0, 
                        blr_parameter2, 1, 8,0, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 24,0, 
                        blr_parameter2, 1, 15,0, 11,0, 
                     blr_assignment, 
                        blr_fid, 0, 27,0, 
                        blr_parameter2, 1, 16,0, 10,0, 
                     blr_assignment, 
                        blr_fid, 0, 25,0, 
                        blr_parameter2, 1, 17,0, 9,0, 
                     blr_assignment, 
                        blr_fid, 0, 26,0, 
                        blr_parameter2, 1, 18,0, 12,0, 
                     blr_assignment, 
                        blr_fid, 0, 11,0, 
                        blr_parameter2, 1, 19,0, 14,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter, 1, 20,0, 
                     blr_assignment, 
                        blr_fid, 0, 9,0, 
                        blr_parameter2, 1, 21,0, 13,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter, 1, 22,0, 
                     blr_assignment, 
                        blr_fid, 0, 23,0, 
                        blr_parameter2, 1, 24,0, 23,0, 
                     blr_assignment, 
                        blr_fid, 0, 22,0, 
                        blr_parameter2, 1, 25,0, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 15,0, 
                        blr_parameter, 1, 30,0, 
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
                                    blr_parameter2, 2, 28,0, 27,0, 
                                    blr_fid, 1, 17,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 4,0, 
                                    blr_fid, 1, 0,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 26,0, 25,0, 
                                    blr_fid, 1, 24,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 24,0, 23,0, 
                                    blr_fid, 1, 27,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 22,0, 21,0, 
                                    blr_fid, 1, 25,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 20,0, 19,0, 
                                    blr_fid, 1, 26,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 18,0, 17,0, 
                                    blr_fid, 1, 11,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 16,0, 
                                    blr_fid, 1, 8,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 15,0, 14,0, 
                                    blr_fid, 1, 9,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 13,0, 
                                    blr_fid, 1, 10,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 12,0, 11,0, 
                                    blr_fid, 1, 23,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 10,0, 9,0, 
                                    blr_fid, 1, 22,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 3,0, 8,0, 
                                    blr_fid, 1, 7,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 2,0, 7,0, 
                                    blr_fid, 1, 6,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 6,0, 
                                    blr_fid, 1, 3,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 5,0, 
                                    blr_fid, 1, 2,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 5,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1264 [195] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 4, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 4,0, 0, 
               blr_rid, 3,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_fid, 1, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 1, 1,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 0, 3, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 3, 0,0, 
                                 blr_end, 
                        blr_receive, 4, 
                           blr_modify, 1, 2, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter, 4, 0,0, 
                                    blr_fid, 2, 1,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1278 [185] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 12,0, 
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
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 22,0, 
                     blr_parameter2, 1, 2,0, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 23,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 24,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 27,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 25,0, 
                     blr_parameter, 1, 6,0, 
                  blr_assignment, 
                     blr_fid, 0, 26,0, 
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter, 1, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 9,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 10,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter, 1, 11,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1294 [249] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 10,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 11,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
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
               blr_rid, 2,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter2, 1, 0,0, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter2, 1, 1,0, 8,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter2, 1, 2,0, 9,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter2, 1, 3,0, 10,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 23,0, 
                        blr_parameter2, 1, 6,0, 5,0, 
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
                                    blr_parameter2, 2, 9,0, 8,0, 
                                    blr_fid, 1, 23,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 3,0, 7,0, 
                                    blr_fid, 1, 2,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 2,0, 6,0, 
                                    blr_fid, 1, 3,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 5,0, 
                                    blr_fid, 1, 6,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 4,0, 
                                    blr_fid, 1, 7,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 4,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1322 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 7,0, 
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
static const UCHAR	jrd_1332 [94] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 2,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 26,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 25,0, 
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
static const UCHAR	jrd_1339 [140] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 15,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 12,0, 
                        blr_fid, 1, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 26,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 0, 15,0, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter2, 1, 0,0, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
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
static const UCHAR	jrd_1349 [140] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 27,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_fid, 1, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 26,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 0, 9,0, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 14,0, 
                     blr_parameter2, 1, 0,0, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
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
static const UCHAR	jrd_1359 [122] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
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
                           blr_fid, 1, 26,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 0, 18,0, 
                           blr_parameter, 0, 0,0, 
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
static const UCHAR	jrd_1367 [209] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 10,0, 
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
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 29,0, 0, 
               blr_rid, 28,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 1, 4,0, 
                        blr_fid, 0, 2,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 9,0, 
                        blr_parameter2, 1, 0,0, 5,0, 
                     blr_assignment, 
                        blr_fid, 1, 0,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 1, 3,0, 
                        blr_parameter2, 1, 3,0, 7,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 8,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 1, 9,0, 
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
               blr_parameter, 1, 4,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1385 [93] =
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
               blr_rid, 29,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 0,0, 
                     blr_not, 
                        blr_missing, 
                           blr_fid, 0, 1,0, 
               blr_sort, 1, 
                  blr_descending, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_1391 [130] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 12,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
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
   blr_receive, 0, 
      blr_store, 
         blr_rid, 29,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 5,0, 4,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 6,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 7,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 8,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 9,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter, 0, 10,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 11,0, 
               blr_fid, 0, 2,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1405 [121] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 5,0, 0, 
               blr_boolean, 
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
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
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
                                    blr_fid, 1, 8,0, 
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
static const UCHAR	jrd_1415 [107] =
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
            blr_rse, 3, 
               blr_rid, 7,0, 0, 
               blr_rid, 5,0, 1, 
               blr_rid, 12,0, 2, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_fid, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 1, 1,0, 
                           blr_fid, 2, 1,0, 
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
static const UCHAR	jrd_1420 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 6,0, 
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
static const UCHAR	jrd_1430 [98] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 17,0, 0, 
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
static const UCHAR	jrd_1439 [141] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 5,0, 
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
            blr_rse, 1, 
               blr_rid, 12,0, 0, 
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
                        blr_parameter2, 1, 0,0, 3,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter, 1, 4,0, 
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
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1452 [104] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_int64, 0, 
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
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter2, 1, 0,0, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
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
static const UCHAR	jrd_1461 [400] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 15,0, 
      blr_int64, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
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
   blr_message, 1, 20,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_int64, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 11,0, 
                        blr_parameter2, 1, 0,0, 13,0, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter2, 1, 1,0, 14,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter2, 1, 2,0, 15,0, 
                     blr_assignment, 
                        blr_fid, 0, 13,0, 
                        blr_parameter2, 1, 3,0, 16,0, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter2, 1, 4,0, 17,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter2, 1, 6,0, 19,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter, 1, 7,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 8,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter, 1, 9,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 10,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter2, 1, 12,0, 11,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter, 1, 18,0, 
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
                                    blr_parameter, 2, 14,0, 
                                    blr_fid, 1, 7,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 13,0, 
                                    blr_fid, 1, 2,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 12,0, 11,0, 
                                    blr_fid, 1, 10,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 5,0, 10,0, 
                                    blr_fid, 1, 11,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 4,0, 9,0, 
                                    blr_fid, 1, 5,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 3,0, 8,0, 
                                    blr_fid, 1, 4,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 2,0, 7,0, 
                                    blr_fid, 1, 13,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 6,0, 
                                    blr_fid, 1, 12,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 3,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 8,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1503 [104] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 8,0, 
      blr_int64, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 12,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 5,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 6,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 8,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1513 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 6,0, 
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
static const UCHAR	jrd_1523 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 7,0, 
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
static const UCHAR	jrd_1533 [169] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter2, 1, 0,0, 3,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter, 1, 4,0, 
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
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1547 [126] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 2,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_starting, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 15,0, 
                           blr_literal, blr_long, 0, 0,0,0,0,
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
static const UCHAR	jrd_1557 [180] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 7,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
            blr_rse, 1, 
               blr_rid, 27,0, 0, 
               blr_boolean, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter2, 1, 0,0, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 13,0, 
                        blr_parameter2, 1, 1,0, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter2, 1, 2,0, 6,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 3,0, 
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
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1573 [131] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 27,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_equiv, 
                           blr_fid, 0, 14,0, 
                           blr_value_if, 
                              blr_eql, 
                                 blr_parameter, 0, 0,0, 
                                 blr_literal, blr_text2, 3,0, 0,0, 
                              blr_null, 
                              blr_parameter, 0, 0,0, 
                        blr_not, 
                           blr_missing, 
                              blr_fid, 0, 5,0, 
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
static const UCHAR	jrd_1581 [260] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 30,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 27,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 9,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 10,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 11,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter2, 0, 13,0, 12,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 14,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter2, 0, 4,0, 15,0, 
               blr_fid, 0, 12,0, 
            blr_assignment, 
               blr_parameter2, 0, 5,0, 16,0, 
               blr_fid, 0, 13,0, 
            blr_assignment, 
               blr_parameter2, 0, 18,0, 17,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 20,0, 19,0, 
               blr_fid, 0, 11,0, 
            blr_assignment, 
               blr_parameter2, 0, 22,0, 21,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 24,0, 23,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter2, 0, 26,0, 25,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 6,0, 27,0, 
               blr_fid, 0, 14,0, 
            blr_assignment, 
               blr_parameter2, 0, 7,0, 28,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter2, 0, 8,0, 29,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1613 [239] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 27,0, 0, 
               blr_rid, 5,0, 1, 
               blr_rid, 7,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_equiv, 
                           blr_fid, 0, 14,0, 
                           blr_value_if, 
                              blr_eql, 
                                 blr_parameter, 0, 0,0, 
                                 blr_literal, blr_text2, 3,0, 0,0, 
                              blr_null, 
                              blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 2, 1,0, 
                              blr_fid, 0, 1,0, 
                           blr_and, 
                              blr_equiv, 
                                 blr_fid, 2, 5,0, 
                                 blr_fid, 0, 14,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 2, 4,0, 
                                    blr_parameter, 0, 2,0, 
                                 blr_and, 
                                    blr_eql, 
                                       blr_fid, 1, 1,0, 
                                       blr_fid, 2, 0,0, 
                                    blr_and, 
                                       blr_eql, 
                                          blr_fid, 1, 10,0, 
                                          blr_fid, 2, 2,0, 
                                       blr_eql, 
                                          blr_fid, 1, 4,0, 
                                          blr_fid, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 1, 2,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 1, 3, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 3, 2,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1626 [448] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 18,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 0,1, 
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
   blr_message, 1, 21,0, 
      blr_quad, 0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter2, 1, 0,0, 11,0, 
                     blr_assignment, 
                        blr_fid, 0, 15,0, 
                        blr_parameter2, 1, 1,0, 12,0, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 2,0, 13,0, 
                     blr_assignment, 
                        blr_fid, 0, 13,0, 
                        blr_parameter2, 1, 3,0, 18,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter2, 1, 4,0, 19,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 17,0, 
                        blr_parameter2, 1, 8,0, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter2, 1, 10,0, 9,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter, 1, 14,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 15,0, 
                     blr_assignment, 
                        blr_fid, 0, 11,0, 
                        blr_parameter2, 1, 17,0, 16,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter, 1, 20,0, 
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
                                    blr_parameter2, 2, 17,0, 16,0, 
                                    blr_fid, 1, 17,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 15,0, 14,0, 
                                    blr_fid, 1, 12,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 4,0, 13,0, 
                                    blr_fid, 1, 5,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 3,0, 12,0, 
                                    blr_fid, 1, 15,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 2,0, 11,0, 
                                    blr_fid, 1, 14,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 10,0, 
                                    blr_fid, 1, 3,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 9,0, 
                                    blr_fid, 1, 2,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 8,0, 7,0, 
                                    blr_fid, 1, 11,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 6,0, 
                                    blr_fid, 1, 13,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 5,0, 
                                    blr_fid, 1, 6,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 6,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1673 [100] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 8,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 26,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter2, 0, 4,0, 3,0, 
               blr_fid, 0, 17,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 5,0, 
               blr_fid, 0, 16,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 6,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 1,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1683 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 6,0, 
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
static const UCHAR	jrd_1693 [110] =
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
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 7,0, 
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
static const UCHAR	jrd_1703 [169] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
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
                        blr_parameter, 0, 1,0, 
                     blr_equiv, 
                        blr_fid, 0, 9,0, 
                        blr_value_if, 
                           blr_eql, 
                              blr_parameter, 0, 0,0, 
                              blr_literal, blr_text2, 3,0, 0,0, 
                           blr_null, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 16,0, 
                        blr_parameter2, 1, 0,0, 3,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter, 1, 4,0, 
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
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1717 [126] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 2,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_starting, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 15,0, 
                           blr_literal, blr_long, 0, 0,0,0,0,
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
static const UCHAR	jrd_1727 [180] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 7,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
            blr_rse, 1, 
               blr_rid, 15,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_equiv, 
                        blr_fid, 0, 10,0, 
                        blr_value_if, 
                           blr_eql, 
                              blr_parameter, 0, 0,0, 
                              blr_literal, blr_text2, 3,0, 0,0, 
                           blr_null, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 18,0, 
                        blr_parameter2, 1, 0,0, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 19,0, 
                        blr_parameter2, 1, 1,0, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter2, 1, 2,0, 6,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 3,0, 
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
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1743 [210] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 4,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 0,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 9,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter2, 1, 0,0, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter2, 1, 1,0, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 13,0, 
                        blr_parameter2, 1, 2,0, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter2, 1, 3,0, 8,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 4,0, 
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
                                    blr_parameter2, 2, 1,0, 3,0, 
                                    blr_fid, 1, 4,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 2,0, 
                                    blr_fid, 1, 5,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 4,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1763 [131] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 15,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_equiv, 
                           blr_fid, 0, 10,0, 
                           blr_value_if, 
                              blr_eql, 
                                 blr_parameter, 0, 0,0, 
                                 blr_literal, blr_text2, 3,0, 0,0, 
                              blr_null, 
                              blr_parameter, 0, 0,0, 
                        blr_not, 
                           blr_missing, 
                              blr_fid, 0, 21,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 21,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
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
static const UCHAR	jrd_1771 [365] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 44,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
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
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 15,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 9,0, 
               blr_fid, 0, 21,0, 
            blr_assignment, 
               blr_parameter2, 0, 11,0, 10,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter2, 0, 13,0, 12,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 15,0, 14,0, 
               blr_fid, 0, 17,0, 
            blr_assignment, 
               blr_parameter2, 0, 17,0, 16,0, 
               blr_fid, 0, 15,0, 
            blr_assignment, 
               blr_parameter2, 0, 19,0, 18,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter2, 0, 21,0, 20,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter2, 0, 23,0, 22,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter2, 0, 25,0, 24,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter2, 0, 27,0, 26,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 29,0, 28,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 30,0, 
               blr_fid, 0, 14,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 31,0, 
               blr_fid, 0, 13,0, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 32,0, 
               blr_fid, 0, 12,0, 
            blr_assignment, 
               blr_parameter2, 0, 4,0, 33,0, 
               blr_fid, 0, 18,0, 
            blr_assignment, 
               blr_parameter2, 0, 5,0, 34,0, 
               blr_fid, 0, 19,0, 
            blr_assignment, 
               blr_parameter2, 0, 36,0, 35,0, 
               blr_fid, 0, 16,0, 
            blr_assignment, 
               blr_parameter2, 0, 38,0, 37,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter2, 0, 40,0, 39,0, 
               blr_fid, 0, 20,0, 
            blr_assignment, 
               blr_parameter2, 0, 6,0, 41,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 7,0, 42,0, 
               blr_fid, 0, 11,0, 
            blr_assignment, 
               blr_parameter2, 0, 8,0, 43,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1817 [462] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 19,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 0,0, 0,1, 
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
   blr_message, 1, 22,0, 
      blr_quad, 0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
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
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
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
                        blr_parameter, 0, 1,0, 
                     blr_equiv, 
                        blr_fid, 0, 9,0, 
                        blr_value_if, 
                           blr_eql, 
                              blr_parameter, 0, 0,0, 
                              blr_literal, blr_text2, 3,0, 0,0, 
                           blr_null, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 11,0, 
                        blr_parameter2, 1, 0,0, 13,0, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter2, 1, 1,0, 16,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter2, 1, 2,0, 17,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter2, 1, 3,0, 18,0, 
                     blr_assignment, 
                        blr_fid, 0, 15,0, 
                        blr_parameter2, 1, 4,0, 19,0, 
                     blr_assignment, 
                        blr_fid, 0, 13,0, 
                        blr_parameter2, 1, 5,0, 20,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 6,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter2, 1, 9,0, 8,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter, 1, 10,0, 
                     blr_assignment, 
                        blr_fid, 0, 19,0, 
                        blr_parameter2, 1, 12,0, 11,0, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 15,0, 14,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter, 1, 21,0, 
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
                                    blr_parameter2, 2, 18,0, 17,0, 
                                    blr_fid, 1, 10,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 16,0, 
                                    blr_fid, 1, 6,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 15,0, 14,0, 
                                    blr_fid, 1, 19,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 5,0, 13,0, 
                                    blr_fid, 1, 11,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 12,0, 11,0, 
                                    blr_fid, 1, 14,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 4,0, 10,0, 
                                    blr_fid, 1, 5,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 3,0, 9,0, 
                                    blr_fid, 1, 4,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 2,0, 8,0, 
                                    blr_fid, 1, 8,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 7,0, 
                                    blr_fid, 1, 15,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 6,0, 
                                    blr_fid, 1, 13,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 7,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1866 [134] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 13,0, 
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
   blr_receive, 0, 
      blr_store, 
         blr_rid, 14,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 4,0, 3,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 6,0, 5,0, 
               blr_fid, 0, 18,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 7,0, 
               blr_fid, 0, 17,0, 
            blr_assignment, 
               blr_parameter2, 0, 9,0, 8,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 10,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 11,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter, 0, 12,0, 
               blr_fid, 0, 12,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1881 [83] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 29,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 1,0, 
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
static const UCHAR	jrd_1887 [146] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
               blr_rid, 28,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter2, 1, 0,0, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 1, 3,0, 
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
                                    blr_fid, 1, 3,0, 
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
static const UCHAR	jrd_1900 [64] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_long, 0, 
      blr_long, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 21,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1906 [232] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 26,0, 
      blr_quad, 0, 
      blr_quad, 0, 
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
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 2,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 5,0, 4,0, 
               blr_fid, 0, 17,0, 
            blr_assignment, 
               blr_parameter2, 0, 7,0, 6,0, 
               blr_fid, 0, 25,0, 
            blr_assignment, 
               blr_parameter2, 0, 9,0, 8,0, 
               blr_fid, 0, 27,0, 
            blr_assignment, 
               blr_parameter2, 0, 11,0, 10,0, 
               blr_fid, 0, 24,0, 
            blr_assignment, 
               blr_parameter2, 0, 13,0, 12,0, 
               blr_fid, 0, 26,0, 
            blr_assignment, 
               blr_parameter2, 0, 15,0, 14,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter2, 0, 17,0, 16,0, 
               blr_fid, 0, 11,0, 
            blr_assignment, 
               blr_parameter, 0, 18,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter, 0, 19,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 21,0, 20,0, 
               blr_fid, 0, 22,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 22,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 23,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 24,0, 
               blr_fid, 0, 29,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 25,0, 
               blr_fid, 0, 15,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1934 [92] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 0,0, 7,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 18,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 4,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1942 [98] =
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
static const UCHAR	jrd_1951 [181] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_geq, 
                           blr_fid, 0, 6,0, 
                           blr_parameter, 0, 2,0, 
                        blr_leq, 
                           blr_fid, 0, 6,0, 
                           blr_parameter, 0, 1,0, 
               blr_end, 
            blr_begin, 
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
                        blr_parameter2, 1, 3,0, 2,0, 
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
                                    blr_parameter2, 2, 2,0, 1,0, 
                                    blr_fid, 1, 6,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_1967 [128] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 6,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
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
                                    blr_fid, 1, 6,0, 
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
static const UCHAR	jrd_1977 [85] =
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
               blr_rid, 31,0, 0, 
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
static const UCHAR	jrd_1983 [86] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
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
static const UCHAR	jrd_1989 [75] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 5,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_long, 0, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 10,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_1996 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 0,0, 0,1, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 10,0, 0, 
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
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_2001 [143] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 22,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 3,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 2,0, 
                              blr_parameter, 0, 1,0, 
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
static const UCHAR	jrd_2013 [166] =
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
            blr_rse, 3, 
               blr_rid, 5,0, 0, 
               blr_rid, 5,0, 1, 
               blr_rid, 7,0, 2, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 0,0, 
                              blr_fid, 1, 4,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 2,0, 
                                 blr_fid, 1, 2,0, 
                              blr_and, 
                                 blr_eql, 
                                    blr_fid, 1, 1,0, 
                                    blr_fid, 2, 0,0, 
                                 blr_and, 
                                    blr_eql, 
                                       blr_fid, 0, 1,0, 
                                       blr_fid, 2, 1,0, 
                                    blr_eql, 
                                       blr_fid, 1, 10,0, 
                                       blr_fid, 2, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 2, 0,0, 
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
static const UCHAR	jrd_2020 [108] =
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
            blr_rse, 1, 
               blr_rid, 13,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_2027 [157] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 4, 
               blr_rid, 22,0, 0, 
               blr_rid, 4,0, 1, 
               blr_rid, 4,0, 2, 
               blr_rid, 6,0, 3, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 2,0, 
                           blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 0,0, 
                              blr_fid, 0, 5,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 2, 0,0, 
                                 blr_fid, 1, 8,0, 
                              blr_eql, 
                                 blr_fid, 2, 1,0, 
                                 blr_fid, 3, 8,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 3, 8,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 3, 16,0, 
                     blr_parameter2, 1, 3,0, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_2036 [178] =
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
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 22,0, 0, 
               blr_rid, 6,0, 1, 
               blr_rid, 6,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_or, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 3,0, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 5,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 8,0, 
                              blr_parameter, 0, 0,0, 
                           blr_eql, 
                              blr_fid, 2, 8,0, 
                              blr_fid, 0, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 2, 8,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 1, 16,0, 
                     blr_parameter2, 1, 3,0, 2,0, 
                  blr_assignment, 
                     blr_fid, 2, 16,0, 
                     blr_parameter2, 1, 5,0, 4,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 



//----------------------


// Check temporary table reference rules between given child relation and master
// relation (owner of given PK/UK index).
static void checkForeignKeyTempScope(thread_db* tdbb, jrd_tra* transaction,
	const MetaName&	childRelName, const MetaName& masterIndexName)
{
   struct {
          TEXT  jrd_2043 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_2044;	// gds__utility 
          SSHORT jrd_2045;	// gds__null_flag 
          SSHORT jrd_2046;	// RDB$RELATION_TYPE 
          SSHORT jrd_2047;	// gds__null_flag 
          SSHORT jrd_2048;	// RDB$RELATION_TYPE 
   } jrd_2042;
   struct {
          TEXT  jrd_2038 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_2039 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_2040 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_2041 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_2037;
	AutoCacheRequest request(tdbb, drq_l_rel_info, DYN_REQUESTS);
	MetaName masterRelName;
	rel_t masterType, childType;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		RLC_M IN RDB$RELATION_CONSTRAINTS CROSS
		REL_C IN RDB$RELATIONS CROSS
		REL_M IN RDB$RELATIONS
		WITH (RLC_M.RDB$CONSTRAINT_TYPE EQ UNIQUE_CNSTRT OR
			  RLC_M.RDB$CONSTRAINT_TYPE EQ PRIMARY_KEY) AND
			 RLC_M.RDB$INDEX_NAME EQ masterIndexName.c_str() AND
			 REL_C.RDB$RELATION_NAME EQ childRelName.c_str() AND
			 REL_M.RDB$RELATION_NAME EQ RLC_M.RDB$RELATION_NAME*/
	{
	request.compile(tdbb, (UCHAR*) jrd_2036, sizeof(jrd_2036));
	gds__vtov ((const char*) childRelName.c_str(), (char*) jrd_2037.jrd_2038, 32);
	gds__vtov ((const char*) masterIndexName.c_str(), (char*) jrd_2037.jrd_2039, 32);
	gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_2037.jrd_2040, 12);
	gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_2037.jrd_2041, 12);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 88, (UCHAR*) &jrd_2037);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 42, (UCHAR*) &jrd_2042);
	   if (!jrd_2042.jrd_2044) break;
	{
		fb_assert(masterRelName.isEmpty());

		masterRelName = /*REL_M.RDB$RELATION_NAME*/
				jrd_2042.jrd_2043;
		masterType = relationType(/*REL_M.RDB$RELATION_TYPE.NULL*/
					  jrd_2042.jrd_2047, /*REL_M.RDB$RELATION_TYPE*/
  jrd_2042.jrd_2048);
		childType = relationType(/*REL_C.RDB$RELATION_TYPE.NULL*/
					 jrd_2042.jrd_2045, /*REL_C.RDB$RELATION_TYPE*/
  jrd_2042.jrd_2046);

	}
	/*END_FOR*/
	   }
	}

	if (masterRelName.hasData())
	{
		checkRelationType(masterType, masterRelName);
		checkRelationType(childType, childRelName);
		checkFkPairTypes(masterType, masterRelName, childType, childRelName);
	}
}

// Check temporary table reference rules between just created child relation and all
// its master relations.
static void checkRelationTempScope(thread_db* tdbb, jrd_tra* transaction,
	const MetaName&	childRelName, const rel_t childType)
{
   struct {
          TEXT  jrd_2032 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_2033;	// gds__utility 
          SSHORT jrd_2034;	// gds__null_flag 
          SSHORT jrd_2035;	// RDB$RELATION_TYPE 
   } jrd_2031;
   struct {
          TEXT  jrd_2029 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_2030 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_2028;
	if (childType != rel_persistent &&
		childType != rel_global_temp_preserve &&
		childType != rel_global_temp_delete)
	{
		return;
	}

	AutoCacheRequest request(tdbb, drq_l_rel_info2, DYN_REQUESTS);
	MetaName masterRelName;
	rel_t masterType;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		RLC_C IN RDB$RELATION_CONSTRAINTS CROSS
		IND_C IN RDB$INDICES CROSS
		IND_M IN RDB$INDICES CROSS
		REL_M IN RDB$RELATIONS
		WITH RLC_C.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY AND
			 RLC_C.RDB$RELATION_NAME EQ childRelName.c_str() AND
			 IND_C.RDB$INDEX_NAME EQ RLC_C.RDB$INDEX_NAME AND
			 IND_M.RDB$INDEX_NAME EQ IND_C.RDB$FOREIGN_KEY AND
			 IND_M.RDB$RELATION_NAME EQ REL_M.RDB$RELATION_NAME*/
	{
	request.compile(tdbb, (UCHAR*) jrd_2027, sizeof(jrd_2027));
	gds__vtov ((const char*) childRelName.c_str(), (char*) jrd_2028.jrd_2029, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_2028.jrd_2030, 12);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 44, (UCHAR*) &jrd_2028);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 38, (UCHAR*) &jrd_2031);
	   if (!jrd_2031.jrd_2033) break;
	{
		fb_assert(masterRelName.isEmpty());

		masterType = relationType(/*REL_M.RDB$RELATION_TYPE.NULL*/
					  jrd_2031.jrd_2034, /*REL_M.RDB$RELATION_TYPE*/
  jrd_2031.jrd_2035);
		masterRelName = /*REL_M.RDB$RELATION_NAME*/
				jrd_2031.jrd_2032;
	}
	/*END_FOR*/
	   }
	}

	if (masterRelName.hasData())
	{
		checkRelationType(masterType, masterRelName);
		checkFkPairTypes(masterType, masterRelName, childType, childRelName);
	}
}

// Checks to see if the given field is referenced in a stored procedure or trigger.
// If the field is referenced, throw.
static void checkSpTrigDependency(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& fieldName)
{
   struct {
          TEXT  jrd_2025 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_2026;	// gds__utility 
   } jrd_2024;
   struct {
          TEXT  jrd_2022 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_2023 [32];	// RDB$DEPENDED_ON_NAME 
   } jrd_2021;
	AutoRequest request;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FIRST 1
		DEP IN RDB$DEPENDENCIES
		WITH DEP.RDB$DEPENDED_ON_NAME EQ relationName.c_str() AND
			 DEP.RDB$FIELD_NAME EQ fieldName.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_2020, sizeof(jrd_2020));
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_2021.jrd_2022, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_2021.jrd_2023, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_2021);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_2024);
	   if (!jrd_2024.jrd_2026) break;
	{
		MetaName depName(/*DEP.RDB$DEPENDENT_NAME*/
				 jrd_2024.jrd_2025);

		// msg 206: Column %s from table %s is referenced in %s.
		status_exception::raise(
			Arg::PrivateDyn(206) << fieldName << relationName << depName);
	}
	/*END_FOR*/
	   }
	}
}

// Checks to see if the given field is referenced in a view. If it is, throw.
static void checkViewDependency(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& fieldName)
{
   struct {
          TEXT  jrd_2018 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_2019;	// gds__utility 
   } jrd_2017;
   struct {
          TEXT  jrd_2015 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_2016 [32];	// RDB$RELATION_NAME 
   } jrd_2014;
	AutoRequest request;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FIRST 1
		X IN RDB$RELATION_FIELDS CROSS
		Y IN RDB$RELATION_FIELDS CROSS
		Z IN RDB$VIEW_RELATIONS
		WITH X.RDB$RELATION_NAME EQ relationName.c_str() AND
			 X.RDB$FIELD_NAME EQ fieldName.c_str() AND
			 X.RDB$FIELD_NAME EQ Y.RDB$BASE_FIELD AND
			 X.RDB$FIELD_SOURCE EQ Y.RDB$FIELD_SOURCE AND
			 Y.RDB$RELATION_NAME EQ Z.RDB$VIEW_NAME AND
			 X.RDB$RELATION_NAME EQ Z.RDB$RELATION_NAME AND
			 Y.RDB$VIEW_CONTEXT EQ Z.RDB$VIEW_CONTEXT*/
	{
	request.compile(tdbb, (UCHAR*) jrd_2013, sizeof(jrd_2013));
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_2014.jrd_2015, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_2014.jrd_2016, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_2014);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_2017);
	   if (!jrd_2017.jrd_2019) break;
	{
		MetaName viewName(/*Z.RDB$VIEW_NAME*/
				  jrd_2017.jrd_2018);

		// msg 206: Column %s from table %s is referenced in  %s.
		status_exception::raise(
			Arg::PrivateDyn(206) << fieldName << relationName << viewName);
	}
	/*END_FOR*/
	   }
	}
}

// Removes temporary pool pointers from field, stored in permanent cache.
static void clearPermanentField(dsql_rel* relation, bool permanent)
{
	if (relation && relation->rel_fields && permanent)
	{
		relation->rel_fields->fld_procedure = NULL;
		relation->rel_fields->ranges = NULL;
		relation->rel_fields->charSet = NULL;
		relation->rel_fields->subTypeName = NULL;
		relation->rel_fields->fld_relation = relation;
	}
}

// Define a COMPUTED BY clause, for a field or an index.
void defineComputed(DsqlCompilerScratch* dsqlScratch, RelationSourceNode* relation, dsql_fld* field,
	ValueSourceClause* clause, string& source, BlrDebugWriter::BlrData& value)
{
	// Get the table node and set up correct context.
	dsqlScratch->resetContextStack();

	// Save the size of the field if it is specified.
	dsc saveDesc;
	saveDesc.dsc_dtype = 0;

	if (field && field->dtype)
	{
		fb_assert(field->dtype <= MAX_UCHAR);
		saveDesc.dsc_dtype = (UCHAR) field->dtype;
		saveDesc.dsc_length = field->length;
		fb_assert(field->scale <= MAX_SCHAR);
		saveDesc.dsc_scale = (SCHAR) field->scale;
		saveDesc.dsc_sub_type = field->subType;

		field->dtype = 0;
		field->length = 0;
		field->scale = 0;
		field->subType = 0;
	}

	PASS1_make_context(dsqlScratch, relation);

	ValueExprNode* input = Node::doDsqlPass(dsqlScratch, clause->value);

	// Try to calculate size of the computed field. The calculated size
	// may be ignored, but it will catch self references.
	dsc desc;
	MAKE_desc(dsqlScratch, &desc, input);

	// Generate the blr expression.

	dsqlScratch->getBlrData().clear();
	dsqlScratch->getDebugData().clear();
	dsqlScratch->appendUChar(dsqlScratch->isVersion4() ? blr_version4 : blr_version5);

	GEN_expr(dsqlScratch, input);
	dsqlScratch->appendUChar(blr_eoc);

	if (saveDesc.dsc_dtype)
	{
		// Restore the field size/type overrides.
		field->dtype = saveDesc.dsc_dtype;
		field->length = saveDesc.dsc_length;
		field->scale = saveDesc.dsc_scale;

		if (field->dtype <= dtype_any_text)
		{
			field->charSetId = DSC_GET_CHARSET(&saveDesc);
			field->collationId = DSC_GET_COLLATE(&saveDesc);
		}
		else
			field->subType = saveDesc.dsc_sub_type;
	}
	else if (field)
	{
		// Use size calculated.
		field->dtype = desc.dsc_dtype;
		field->length = desc.dsc_length;
		field->scale = desc.dsc_scale;

		if (field->dtype <= dtype_any_text)
		{
			field->charSetId = DSC_GET_CHARSET(&desc);
			field->collationId = DSC_GET_COLLATE(&desc);
		}
		else
			field->subType = desc.dsc_sub_type;
	}

	dsqlScratch->resetContextStack();

	// Generate the source text.
	source = clause->source;

	value.assign(dsqlScratch->getBlrData());
}

// Delete a record from RDB$RELATION_CONSTRAINTS based on a constraint name.
//
//      On deleting from RDB$RELATION_CONSTRAINTS, 2 system triggers fire:
//
//      (A) pre delete trigger: pre_delete_constraint, will:
//
//        1. delete a record first from RDB$REF_CONSTRAINTS where
//           RDB$REF_CONSTRAINTS.RDB$CONSTRAINT_NAME =
//                               RDB$RELATION_CONSTRAINTS.RDB$CONSTRAINT_NAME
//
//      (B) post delete trigger: post_delete_constraint will:
//
//        1. also delete a record from RDB$INDICES where
//           RDB$INDICES.RDB$INDEX_NAME =
//                               RDB$RELATION_CONSTRAINTS.RDB$INDEX_NAME
//
//        2. also delete a record from RDB$INDEX_SEGMENTS where
//           RDB$INDEX_SEGMENTS.RDB$INDEX_NAME =
//                               RDB$RELATION_CONSTRAINTS.RDB$INDEX_NAME
static void deleteKeyConstraint(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& constraintName, const MetaName& indexName)
{
   struct {
          SSHORT jrd_2012;	// gds__utility 
   } jrd_2011;
   struct {
          SSHORT jrd_2010;	// gds__utility 
   } jrd_2009;
   struct {
          SSHORT jrd_2008;	// gds__utility 
   } jrd_2007;
   struct {
          TEXT  jrd_2003 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_2004 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_2005 [32];	// RDB$CONSTRAINT_NAME 
          TEXT  jrd_2006 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_2002;
	SET_TDBB(tdbb);

	AutoCacheRequest request(tdbb, drq_e_rel_const, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		RC IN RDB$RELATION_CONSTRAINTS
		WITH RC.RDB$CONSTRAINT_NAME EQ constraintName.c_str() AND
			 RC.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY AND
			 RC.RDB$RELATION_NAME EQ relationName.c_str() AND
			 RC.RDB$INDEX_NAME EQ indexName.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_2001, sizeof(jrd_2001));
	gds__vtov ((const char*) indexName.c_str(), (char*) jrd_2002.jrd_2003, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_2002.jrd_2004, 32);
	gds__vtov ((const char*) constraintName.c_str(), (char*) jrd_2002.jrd_2005, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_2002.jrd_2006, 12);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 108, (UCHAR*) &jrd_2002);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_2007);
	   if (!jrd_2007.jrd_2008) break;
	{
		found = true;
		/*ERASE RC;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_2009);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_2011);
	   }
	}

	if (!found)
	{
		// msg 130: "CONSTRAINT %s does not exist."
		status_exception::raise(
			Arg::PrivateDyn(130) << constraintName);
	}
}

// Define a database or shadow file.
static void defineFile(thread_db* tdbb, jrd_tra* transaction, SLONG shadowNumber, bool manualShadow,
	bool conditionalShadow, SLONG& dbAlloc, const PathName& name, SLONG start, SLONG length)
{
   struct {
          TEXT  jrd_1991 [256];	// RDB$FILE_NAME 
          SLONG  jrd_1992;	// RDB$FILE_LENGTH 
          SLONG  jrd_1993;	// RDB$FILE_START 
          SSHORT jrd_1994;	// RDB$FILE_FLAGS 
          SSHORT jrd_1995;	// RDB$SHADOW_NUMBER 
   } jrd_1990;
   struct {
          SSHORT jrd_2000;	// gds__utility 
   } jrd_1999;
   struct {
          TEXT  jrd_1998 [256];	// RDB$FILE_NAME 
   } jrd_1997;
	PathName expandedName = name;

	if (!ISC_expand_filename(expandedName, false))
		status_exception::raise(Arg::PrivateDyn(231));	// File name is invalid.

	if (tdbb->getDatabase()->dbb_filename == expandedName)
		status_exception::raise(Arg::PrivateDyn(166));

	AutoCacheRequest request(tdbb, drq_l_files, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FIRST 1 X IN RDB$FILES
		WITH X.RDB$FILE_NAME EQ expandedName.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1996, sizeof(jrd_1996));
	gds__vtov ((const char*) expandedName.c_str(), (char*) jrd_1997.jrd_1998, 256);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 256, (UCHAR*) &jrd_1997);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1999);
	   if (!jrd_1999.jrd_2000) break;
	{
		status_exception::raise(Arg::PrivateDyn(166));
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, drq_s_files, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$FILES*/
	{
	
	{
		expandedName.copyTo(/*X.RDB$FILE_NAME*/
				    jrd_1990.jrd_1991, sizeof(/*X.RDB$FILE_NAME*/
	 jrd_1990.jrd_1991));
		/*X.RDB$SHADOW_NUMBER*/
		jrd_1990.jrd_1995 = shadowNumber;
		/*X.RDB$FILE_FLAGS*/
		jrd_1990.jrd_1994 = (manualShadow ? FILE_manual : 0) |
			(conditionalShadow ? FILE_conditional : 0);

		dbAlloc = MAX(dbAlloc, start);
		/*X.RDB$FILE_START*/
		jrd_1990.jrd_1993 = dbAlloc;
		/*X.RDB$FILE_LENGTH*/
		jrd_1990.jrd_1992 = length;
		dbAlloc += length;
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_1989, sizeof(jrd_1989));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 268, (UCHAR*) &jrd_1990);
	}
}

// Checks to see if the given field already exists in a relation.
static bool fieldExists(thread_db* tdbb, jrd_tra* transaction, const MetaName& relationName,
	const MetaName& fieldName)
{
   struct {
          SSHORT jrd_1988;	// gds__utility 
   } jrd_1987;
   struct {
          TEXT  jrd_1985 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1986 [32];	// RDB$RELATION_NAME 
   } jrd_1984;
	AutoRequest request;
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FLD IN RDB$RELATION_FIELDS
		WITH FLD.RDB$RELATION_NAME EQ relationName.c_str() AND
			 FLD.RDB$FIELD_NAME EQ fieldName.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1983, sizeof(jrd_1983));
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_1984.jrd_1985, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_1984.jrd_1986, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_1984);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1987);
	   if (!jrd_1987.jrd_1988) break;
	{
		found = true;
	}
	/*END_FOR*/
	   }
	}

	return found;
}

// If inputName is found in RDB$ROLES, then returns true. Otherwise returns false.
static bool isItSqlRole(thread_db* tdbb, jrd_tra* transaction, const MetaName& inputName,
	MetaName& outputName)
{
   struct {
          TEXT  jrd_1981 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_1982;	// gds__utility 
   } jrd_1980;
   struct {
          TEXT  jrd_1979 [32];	// RDB$ROLE_NAME 
   } jrd_1978;
	AutoCacheRequest request(tdbb, drq_get_role_nm, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$ROLES
		WITH X.RDB$ROLE_NAME EQ inputName.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1977, sizeof(jrd_1977));
	gds__vtov ((const char*) inputName.c_str(), (char*) jrd_1978.jrd_1979, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1978);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_1980);
	   if (!jrd_1980.jrd_1982) break;
	{
		outputName = /*X.RDB$OWNER_NAME*/
			     jrd_1980.jrd_1981;
		return true;
	}
	/*END_FOR*/
	   }
	}

	return false;
}

// Make string with relation name and type of its temporary scope.
static void makeRelationScopeName(string& to, const MetaName& name, const rel_t type)
{
	const char* scope = getRelationScopeName(type);
	to.printf(scope, name.c_str());
}

// Get relation type name
static const char* getRelationScopeName(const rel_t type)
{
	switch(type)
	{
	case rel_global_temp_preserve:
		return REL_SCOPE_GTT_PRESERVE;
	case rel_global_temp_delete:
		return REL_SCOPE_GTT_DELETE;
	case rel_external:
		return REL_SCOPE_EXTERNAL;
	case rel_view:
		return REL_SCOPE_VIEW;
	case rel_virtual:
		return REL_SCOPE_VIRTUAL;
	}

	return REL_SCOPE_PERSISTENT;
}

// Check, can relation of given type be used in FK?
static void checkRelationType(const rel_t type, const MetaName& name)
{
	if (type == rel_persistent ||
		type == rel_global_temp_preserve ||
		type == rel_global_temp_delete)
	{
		return;
	}

	string scope;
	makeRelationScopeName(scope, name, type);
	(Arg::PrivateDyn(289) << scope).raise();
}

// Check, can a pair of relations be used in FK
static void checkFkPairTypes(const rel_t masterType, const MetaName& masterName,
	const rel_t childType, const MetaName& childName)
{
	if (masterType != childType &&
		!(masterType == rel_global_temp_preserve && childType == rel_global_temp_delete))
	{
		string master, child;
		makeRelationScopeName(master, masterName, masterType);
		makeRelationScopeName(child, childName, childType);
		// Msg 232 : "%s can't reference %s"
		status_exception::raise(Arg::PrivateDyn(232) << child << master);
	}
}


// Alters the position of a field with respect to the
// other fields in the relation.  This will only affect
// the order in which the fields will be returned when either
// viewing the relation or performing select * from the relation.
//
// The rules of engagement are as follows:
//      if new_position > original position
//         increase RDB$FIELD_POSITION for all fields with RDB$FIELD_POSITION
//         between the new_position and existing position of the field
//      then update the position of the field being altered.
//         just update the position
//
//      if new_position < original position
//         decrease RDB$FIELD_POSITION for all fields with RDB$FIELD_POSITION
//         between the new_position and existing position of the field
//      then update the position of the field being altered.
//
//      if new_position == original_position -- no_op
static void modifyLocalFieldPosition(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& fieldName, USHORT newPosition,
	USHORT existingPosition)
{
   struct {
          SSHORT jrd_1966;	// gds__utility 
   } jrd_1965;
   struct {
          TEXT  jrd_1962 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_1963;	// gds__null_flag 
          SSHORT jrd_1964;	// RDB$FIELD_POSITION 
   } jrd_1961;
   struct {
          TEXT  jrd_1957 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_1958;	// gds__utility 
          SSHORT jrd_1959;	// gds__null_flag 
          SSHORT jrd_1960;	// RDB$FIELD_POSITION 
   } jrd_1956;
   struct {
          TEXT  jrd_1953 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1954;	// RDB$FIELD_POSITION 
          SSHORT jrd_1955;	// RDB$FIELD_POSITION 
   } jrd_1952;
   struct {
          SSHORT jrd_1976;	// gds__utility 
   } jrd_1975;
   struct {
          SSHORT jrd_1974;	// RDB$FIELD_POSITION 
   } jrd_1973;
   struct {
          SSHORT jrd_1971;	// gds__utility 
          SSHORT jrd_1972;	// RDB$FIELD_POSITION 
   } jrd_1970;
   struct {
          TEXT  jrd_1969 [32];	// RDB$RELATION_NAME 
   } jrd_1968;
	// Make sure that there are no duplicate field positions and no gaps in the position sequence.
	// (gaps are introduced when fields are removed)

	AutoRequest request;
	USHORT newPos = 0;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FLD IN RDB$RELATION_FIELDS
		WITH FLD.RDB$RELATION_NAME EQ relationName.c_str()
		SORTED BY ASCENDING FLD.RDB$FIELD_POSITION*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1967, sizeof(jrd_1967));
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_1968.jrd_1969, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1968);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_1970);
	   if (!jrd_1970.jrd_1971) break;
	{
		if (/*FLD.RDB$FIELD_POSITION*/
		    jrd_1970.jrd_1972 != newPos)
		{
			/*MODIFY FLD USING*/
			{
			
				/*FLD.RDB$FIELD_POSITION*/
				jrd_1970.jrd_1972 = newPos;
			/*END_MODIFY*/
			jrd_1973.jrd_1974 = jrd_1970.jrd_1972;
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1973);
			}
		}

		++newPos;
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1975);
	   }
	}

	// Find the position of the last field in the relation.
	SLONG maxPosition = -1;
	DYN_UTIL_generate_field_position(tdbb, relationName, &maxPosition);

	// If the existing position of the field is less than the new position of
	// the field, subtract 1 to move the fields to their new positions otherwise,
	// increase the value in RDB$FIELD_POSITION by one.

	const bool moveDown = (existingPosition < newPosition);

	// Retrieve the records for the fields which have a position between the
	// existing field position and the new field position.

	request.reset();

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FLD IN RDB$RELATION_FIELDS
		WITH FLD.RDB$RELATION_NAME EQ relationName.c_str() AND
				FLD.RDB$FIELD_POSITION >= MIN(newPosition, existingPosition) AND
				FLD.RDB$FIELD_POSITION <= MAX(newPosition, existingPosition)*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1951, sizeof(jrd_1951));
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_1952.jrd_1953, 32);
	jrd_1952.jrd_1954 = MAX(newPosition,existingPosition);
	jrd_1952.jrd_1955 = MIN(newPosition,existingPosition);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 36, (UCHAR*) &jrd_1952);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 38, (UCHAR*) &jrd_1956);
	   if (!jrd_1956.jrd_1958) break;
	{
		/*MODIFY FLD USING*/
		{
		
			// If the field is the one we want, change the position, otherwise
			// increase the value of RDB$FIELD_POSITION.
			if (fieldName == /*FLD.RDB$FIELD_NAME*/
					 jrd_1956.jrd_1957)
			{
				if (newPosition > maxPosition)
				{
					// This prevents gaps in the position sequence of the fields.
					/*FLD.RDB$FIELD_POSITION*/
					jrd_1956.jrd_1960 = maxPosition;
				}
				else
					/*FLD.RDB$FIELD_POSITION*/
					jrd_1956.jrd_1960 = newPosition;
			}
			else
			{
				if (moveDown)
					/*FLD.RDB$FIELD_POSITION*/
					jrd_1956.jrd_1960 = /*FLD.RDB$FIELD_POSITION*/
   jrd_1956.jrd_1960 - 1;
				else
					/*FLD.RDB$FIELD_POSITION*/
					jrd_1956.jrd_1960 = /*FLD.RDB$FIELD_POSITION*/
   jrd_1956.jrd_1960 + 1;
			}

			/*FLD.RDB$FIELD_POSITION.NULL*/
			jrd_1956.jrd_1959 = FALSE;
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_1956.jrd_1957, (char*) jrd_1961.jrd_1962, 32);
		jrd_1961.jrd_1963 = jrd_1956.jrd_1959;
		jrd_1961.jrd_1964 = jrd_1956.jrd_1960;
		EXE_send (tdbb, request, 2, 36, (UCHAR*) &jrd_1961);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1965);
	   }
	}
}

// Convert RDB$RELATION_TYPE to rel_t type.
static rel_t relationType(SSHORT relationTypeNull, SSHORT relationType)
{
	return relationTypeNull ? rel_persistent : rel_t(relationType);
}

// Save the name of a field in the relation or view currently being defined. This is done to support
// definition of triggers which will depend on the metadata created in this statement.
static void saveField(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, const MetaName& fieldName)
{
	dsql_rel* relation = dsqlScratch->relation;
	if (!relation)
		return;

	MemoryPool& p = relation->rel_flags & REL_new_relation ?
		*tdbb->getDefaultPool() : dsqlScratch->getAttachment()->dbb_pool;
	dsql_fld* field = FB_NEW(p) dsql_fld(p);
	field->fld_name = fieldName.c_str();
	field->fld_next = relation->rel_fields;
	relation->rel_fields = field;
}

// Save the name of the relation or view currently being defined. This is done to support definition
// of triggers which will depend on the metadata created in this statement.
static void saveRelation(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	const MetaName& relationName, bool view, bool creating)
{
	if (dsqlScratch->flags & DsqlCompilerScratch::FLAG_METADATA_SAVED)
		return;

	dsqlScratch->flags |= DsqlCompilerScratch::FLAG_METADATA_SAVED;

	dsql_rel* relation;

	if (!view && !creating)
		relation = METD_get_relation(dsqlScratch->getTransaction(), dsqlScratch, relationName);
	else
	{
		MemoryPool& pool = *tdbb->getDefaultPool();
		relation = FB_NEW(pool) dsql_rel(pool);
		relation->rel_name = relationName;
		if (!view)
			relation->rel_flags = REL_creating;
	}

	dsqlScratch->relation = relation;
}

// Update RDB$FIELDS received by reference.
static void updateRdbFields(const TypeClause* type,
	SSHORT& fieldType,
	SSHORT& fieldLength,
	SSHORT& fieldSubTypeNull, SSHORT& fieldSubType,
	SSHORT& fieldScaleNull, SSHORT& fieldScale,
	SSHORT& characterSetIdNull, SSHORT& characterSetId,
	SSHORT& characterLengthNull, SSHORT& characterLength,
	SSHORT& fieldPrecisionNull, SSHORT& fieldPrecision,
	SSHORT& collationIdNull, SSHORT& collationId,
	SSHORT& segmentLengthNull, SSHORT& segmentLength)
{
	// Initialize all nullable fields.
	fieldSubTypeNull = fieldScaleNull = characterSetIdNull = characterLengthNull =
		fieldPrecisionNull = collationIdNull = segmentLengthNull = TRUE;

	if (type->dtype == dtype_blob)
	{
		fieldSubTypeNull = FALSE;
		fieldSubType = type->subType;

		fieldScaleNull = FALSE;
		fieldScale = 0;

		if (type->subType == isc_blob_text)
		{
			characterSetIdNull = FALSE;
			characterSetId = type->charSetId;

			collationIdNull = FALSE;
			collationId = type->collationId;
		}

		if (type->segLength != 0)
		{
			segmentLengthNull = FALSE;
			segmentLength = type->segLength;
		}
	}
	else if (type->dtype <= dtype_any_text)
	{
		fieldSubTypeNull = FALSE;
		fieldSubType = type->subType;

		fieldScaleNull = FALSE;
		fieldScale = 0;

		if (type->charLength != 0)
		{
			characterLengthNull = FALSE;
			characterLength = type->charLength;
		}

		characterSetIdNull = FALSE;
		characterSetId = type->charSetId;

		collationIdNull = FALSE;
		collationId = type->collationId;
	}
	else
	{
		fieldScaleNull = FALSE;
		fieldScale = type->scale;

		if (DTYPE_IS_EXACT(type->dtype))
		{
			fieldPrecisionNull = FALSE;
			fieldPrecision = type->precision;

			fieldSubTypeNull = FALSE;
			fieldSubType = type->subType;
		}
	}

	if (type->dtype == dtype_varying)
	{
		fb_assert(type->length <= MAX_SSHORT);
		fieldLength = (SSHORT) (type->length - sizeof(USHORT));
	}
	else
		fieldLength = type->length;

	fieldType = blr_dtypes[type->dtype];
}


//----------------------


// Delete a security class.
bool DdlNode::deleteSecurityClass(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& secClass)
{
   struct {
          SSHORT jrd_1950;	// gds__utility 
   } jrd_1949;
   struct {
          SSHORT jrd_1948;	// gds__utility 
   } jrd_1947;
   struct {
          SSHORT jrd_1946;	// gds__utility 
   } jrd_1945;
   struct {
          TEXT  jrd_1944 [32];	// RDB$SECURITY_CLASS 
   } jrd_1943;
	AutoCacheRequest request(tdbb, drq_e_class, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		SC IN RDB$SECURITY_CLASSES
		WITH SC.RDB$SECURITY_CLASS EQ secClass.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1942, sizeof(jrd_1942));
	gds__vtov ((const char*) secClass.c_str(), (char*) jrd_1943.jrd_1944, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1943);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1945);
	   if (!jrd_1945.jrd_1946) break;
	{
		found = true;
		/*ERASE SC;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1947);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1949);
	   }
	}

	return found;
}

void DdlNode::storePrivileges(thread_db* tdbb, jrd_tra* transaction,
							  const MetaName& name, int type,
							  const char* privileges)
{
   struct {
          TEXT  jrd_1936 [32];	// RDB$USER 
          TEXT  jrd_1937 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1938;	// RDB$GRANT_OPTION 
          SSHORT jrd_1939;	// RDB$OBJECT_TYPE 
          SSHORT jrd_1940;	// RDB$USER_TYPE 
          TEXT  jrd_1941 [7];	// RDB$PRIVILEGE 
   } jrd_1935;
	Attachment* const attachment = transaction->tra_attachment;
	const string& userName = attachment->att_user->usr_user_name;

	AutoCacheRequest request(tdbb, drq_s_usr_prvs, DYN_REQUESTS);

	for (const char* p = privileges; *p; ++p)
	{
		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			X IN RDB$USER_PRIVILEGES*/
		{
		
		{
			strcpy(/*X.RDB$RELATION_NAME*/
			       jrd_1935.jrd_1937, name.c_str());
			strcpy(/*X.RDB$USER*/
			       jrd_1935.jrd_1936, userName.c_str());
			/*X.RDB$USER_TYPE*/
			jrd_1935.jrd_1940 = obj_user;
			/*X.RDB$OBJECT_TYPE*/
			jrd_1935.jrd_1939 = type;
			/*X.RDB$PRIVILEGE*/
			jrd_1935.jrd_1941[0] = *p;
			/*X.RDB$PRIVILEGE*/
			jrd_1935.jrd_1941[1] = 0;
			/*X.RDB$GRANT_OPTION*/
			jrd_1935.jrd_1938 = 1;
		}
		/*END_STORE*/
		request.compile(tdbb, (UCHAR*) jrd_1934, sizeof(jrd_1934));
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 77, (UCHAR*) &jrd_1935);
		}
	}
}

void DdlNode::executeDdlTrigger(thread_db* tdbb, jrd_tra* transaction, DdlTriggerWhen when,
	int action, const MetaName& objectName, const string& sqlText)
{
	Attachment* const attachment = transaction->tra_attachment;

 	// do nothing if user doesn't want database triggers
	if (attachment->att_flags & ATT_no_db_triggers)
		return;

	fb_assert(action > 0);	// first element is NULL
	DdlTriggerContext context;
	context.eventType = DDL_TRIGGER_ACTION_NAMES[action][0];
	context.objectType = DDL_TRIGGER_ACTION_NAMES[action][1];
	context.objectName = objectName;
	context.sqlText = sqlText;

	Stack<DdlTriggerContext>::AutoPushPop autoContext(attachment->ddlTriggersContext, context);
	AutoSavePoint savePoint(tdbb, transaction);

	EXE_execute_ddl_triggers(tdbb, transaction, when == DTW_BEFORE, action);

	savePoint.release();	// everything is ok
}

void DdlNode::executeDdlTrigger(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction, DdlTriggerWhen when, int action, const MetaName& objectName)
{
	executeDdlTrigger(tdbb, transaction, when, action, objectName,
		*dsqlScratch->getStatement()->getSqlText());
}

void DdlNode::storeGlobalField(thread_db* tdbb, jrd_tra* transaction, MetaName& name,
	const TypeClause* field, const string& computedSource, const BlrDebugWriter::BlrData& computedValue)
{
   struct {
          TEXT  jrd_1902 [32];	// RDB$FIELD_NAME 
          SLONG  jrd_1903;	// RDB$LOWER_BOUND 
          SLONG  jrd_1904;	// RDB$UPPER_BOUND 
          SSHORT jrd_1905;	// RDB$DIMENSION 
   } jrd_1901;
   struct {
          bid  jrd_1908;	// RDB$COMPUTED_BLR 
          bid  jrd_1909;	// RDB$COMPUTED_SOURCE 
          TEXT  jrd_1910 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_1911 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_1912;	// gds__null_flag 
          SSHORT jrd_1913;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_1914;	// gds__null_flag 
          SSHORT jrd_1915;	// RDB$COLLATION_ID 
          SSHORT jrd_1916;	// gds__null_flag 
          SSHORT jrd_1917;	// RDB$FIELD_PRECISION 
          SSHORT jrd_1918;	// gds__null_flag 
          SSHORT jrd_1919;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_1920;	// gds__null_flag 
          SSHORT jrd_1921;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_1922;	// gds__null_flag 
          SSHORT jrd_1923;	// RDB$FIELD_SCALE 
          SSHORT jrd_1924;	// gds__null_flag 
          SSHORT jrd_1925;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_1926;	// RDB$FIELD_LENGTH 
          SSHORT jrd_1927;	// RDB$FIELD_TYPE 
          SSHORT jrd_1928;	// gds__null_flag 
          SSHORT jrd_1929;	// RDB$DIMENSIONS 
          SSHORT jrd_1930;	// gds__null_flag 
          SSHORT jrd_1931;	// gds__null_flag 
          SSHORT jrd_1932;	// gds__null_flag 
          SSHORT jrd_1933;	// RDB$SYSTEM_FLAG 
   } jrd_1907;
	Attachment* const attachment = transaction->tra_attachment;
	const string& userName = attachment->att_user->usr_user_name;

	const ValueListNode* elements = field->ranges;
	const USHORT dims = elements ? elements->items.getCount() / 2 : 0;

	if (dims > MAX_ARRAY_DIMENSIONS)
	{
		status_exception::raise(
			Arg::Gds(isc_sqlerr) << Arg::Num(-604) <<
			Arg::Gds(isc_dsql_max_arr_dim_exceeded));
	}

	if (name.isEmpty())
		DYN_UTIL_generate_field_name(tdbb, name);

	AutoCacheRequest requestHandle(tdbb, drq_s_fld_src, DYN_REQUESTS);

	/*STORE (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		FLD IN RDB$FIELDS*/
	{
	
	{
		/*FLD.RDB$SYSTEM_FLAG*/
		jrd_1907.jrd_1933 = 0;
		strcpy(/*FLD.RDB$FIELD_NAME*/
		       jrd_1907.jrd_1911, name.c_str());

		/*FLD.RDB$OWNER_NAME.NULL*/
		jrd_1907.jrd_1932 = FALSE;
		strcpy(/*FLD.RDB$OWNER_NAME*/
		       jrd_1907.jrd_1910, userName.c_str());

		/*FLD.RDB$COMPUTED_SOURCE.NULL*/
		jrd_1907.jrd_1931 = TRUE;
		/*FLD.RDB$COMPUTED_BLR.NULL*/
		jrd_1907.jrd_1930 = TRUE;
		/*FLD.RDB$DIMENSIONS.NULL*/
		jrd_1907.jrd_1928 = TRUE;

		updateRdbFields(field,
			/*FLD.RDB$FIELD_TYPE*/
			jrd_1907.jrd_1927,
			/*FLD.RDB$FIELD_LENGTH*/
			jrd_1907.jrd_1926,
			/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
			jrd_1907.jrd_1924, /*FLD.RDB$FIELD_SUB_TYPE*/
  jrd_1907.jrd_1925,
			/*FLD.RDB$FIELD_SCALE.NULL*/
			jrd_1907.jrd_1922, /*FLD.RDB$FIELD_SCALE*/
  jrd_1907.jrd_1923,
			/*FLD.RDB$CHARACTER_SET_ID.NULL*/
			jrd_1907.jrd_1920, /*FLD.RDB$CHARACTER_SET_ID*/
  jrd_1907.jrd_1921,
			/*FLD.RDB$CHARACTER_LENGTH.NULL*/
			jrd_1907.jrd_1918, /*FLD.RDB$CHARACTER_LENGTH*/
  jrd_1907.jrd_1919,
			/*FLD.RDB$FIELD_PRECISION.NULL*/
			jrd_1907.jrd_1916, /*FLD.RDB$FIELD_PRECISION*/
  jrd_1907.jrd_1917,
			/*FLD.RDB$COLLATION_ID.NULL*/
			jrd_1907.jrd_1914, /*FLD.RDB$COLLATION_ID*/
  jrd_1907.jrd_1915,
			/*FLD.RDB$SEGMENT_LENGTH.NULL*/
			jrd_1907.jrd_1912, /*FLD.RDB$SEGMENT_LENGTH*/
  jrd_1907.jrd_1913);

		if (dims != 0)
		{
			/*FLD.RDB$DIMENSIONS.NULL*/
			jrd_1907.jrd_1928 = FALSE;
			/*FLD.RDB$DIMENSIONS*/
			jrd_1907.jrd_1929 = dims;
		}

		if (computedSource.hasData())
		{
			/*FLD.RDB$COMPUTED_SOURCE.NULL*/
			jrd_1907.jrd_1931 = FALSE;
			attachment->storeMetaDataBlob(tdbb, transaction, &/*FLD.RDB$COMPUTED_SOURCE*/
									  jrd_1907.jrd_1909,
				computedSource);
		}

		if (computedValue.hasData())
		{
			/*FLD.RDB$COMPUTED_BLR.NULL*/
			jrd_1907.jrd_1930 = FALSE;
			attachment->storeBinaryBlob(tdbb, transaction, &/*FLD.RDB$COMPUTED_BLR*/
									jrd_1907.jrd_1908,
				computedValue);
		}
	}
	/*END_STORE*/
	requestHandle.compile(tdbb, (UCHAR*) jrd_1906, sizeof(jrd_1906));
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 124, (UCHAR*) &jrd_1907);
	}

	if (elements)	// Is the type an array?
	{
		requestHandle.reset(tdbb, drq_s_fld_dym, DYN_REQUESTS);

		SSHORT position = 0;
		const NestConst<ValueExprNode>* ptr = elements->items.begin();
		for (const NestConst<ValueExprNode>* const end = elements->items.end();
			 ptr != end;
			 ++ptr, ++position)
		{
			const ValueExprNode* element = *ptr++;
			const SLONG lrange = element->as<LiteralNode>()->getSlong();
			element = *ptr;
			const SLONG hrange = element->as<LiteralNode>()->getSlong();

			if (lrange >= hrange)
			{
				status_exception::raise(
					Arg::Gds(isc_sqlerr) << Arg::Num(-604) <<
					Arg::Gds(isc_dsql_arr_range_error));
			}

			/*STORE (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
				DIM IN RDB$FIELD_DIMENSIONS*/
			{
			
			{
				strcpy(/*DIM.RDB$FIELD_NAME*/
				       jrd_1901.jrd_1902, name.c_str());
				/*DIM.RDB$DIMENSION*/
				jrd_1901.jrd_1905 = position;
				/*DIM.RDB$UPPER_BOUND*/
				jrd_1901.jrd_1904 = hrange;
				/*DIM.RDB$LOWER_BOUND*/
				jrd_1901.jrd_1903 = lrange;
			}
			/*END_STORE*/
			requestHandle.compile(tdbb, (UCHAR*) jrd_1900, sizeof(jrd_1900));
			EXE_start (tdbb, requestHandle, transaction);
			EXE_send (tdbb, requestHandle, 0, 42, (UCHAR*) &jrd_1901);
			}
		}
	}

	storePrivileges(tdbb, transaction, name, obj_field, USAGE_PRIVILEGES);
}


//----------------------


ParameterClause::ParameterClause(MemoryPool& pool, dsql_fld* field, const MetaName& aCollate,
			ValueSourceClause* aDefaultClause, ValueExprNode* aParameterExpr)
	: name(pool, field ? field->fld_name : ""),
	  type(field),
	  defaultClause(aDefaultClause),
	  parameterExpr(aParameterExpr)
{
	type->collate = aCollate;
}

void ParameterClause::print(string& text) const
{
	string s;
	type->print(s);
	text.printf("name: '%s'  %s", name.c_str(), s.c_str());
}


//----------------------


void AlterCharSetNode::print(string& text) const
{
	text.printf(
		"AlterCharSetNode\n"
		"  charSet: %s\n"
		"  defaultCollation: %s\n",
		charSet.c_str(), defaultCollation.c_str());
}

void AlterCharSetNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1886;	// gds__utility 
   } jrd_1885;
   struct {
          TEXT  jrd_1883 [32];	// RDB$COLLATION_NAME 
          SSHORT jrd_1884;	// RDB$CHARACTER_SET_ID 
   } jrd_1882;
   struct {
          SSHORT jrd_1899;	// gds__utility 
   } jrd_1898;
   struct {
          TEXT  jrd_1896 [32];	// RDB$DEFAULT_COLLATE_NAME 
          SSHORT jrd_1897;	// gds__null_flag 
   } jrd_1895;
   struct {
          TEXT  jrd_1891 [32];	// RDB$DEFAULT_COLLATE_NAME 
          SSHORT jrd_1892;	// gds__utility 
          SSHORT jrd_1893;	// gds__null_flag 
          SSHORT jrd_1894;	// RDB$CHARACTER_SET_ID 
   } jrd_1890;
   struct {
          TEXT  jrd_1889 [32];	// RDB$CHARACTER_SET_NAME 
   } jrd_1888;
	METD_drop_charset(transaction, charSet);
	MET_dsql_cache_release(tdbb, SYM_intlsym_charset, charSet);

	bool charSetFound = false;
	bool collationFound = false;

	AutoCacheRequest requestHandle1(tdbb, drq_m_charset, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle1 TRANSACTION_HANDLE transaction)
		CS IN RDB$CHARACTER_SETS
		WITH CS.RDB$CHARACTER_SET_NAME EQ charSet.c_str()*/
	{
	requestHandle1.compile(tdbb, (UCHAR*) jrd_1887, sizeof(jrd_1887));
	gds__vtov ((const char*) charSet.c_str(), (char*) jrd_1888.jrd_1889, 32);
	EXE_start (tdbb, requestHandle1, transaction);
	EXE_send (tdbb, requestHandle1, 0, 32, (UCHAR*) &jrd_1888);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle1, 1, 38, (UCHAR*) &jrd_1890);
	   if (!jrd_1890.jrd_1892) break;
	{
		charSetFound = true;

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_ALTER_CHARACTER_SET, charSet);

		AutoCacheRequest requestHandle2(tdbb, drq_l_collation, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE requestHandle2 TRANSACTION_HANDLE transaction)
			COLL IN RDB$COLLATIONS
			WITH COLL.RDB$CHARACTER_SET_ID EQ CS.RDB$CHARACTER_SET_ID AND
				COLL.RDB$COLLATION_NAME EQ defaultCollation.c_str()*/
		{
		requestHandle2.compile(tdbb, (UCHAR*) jrd_1881, sizeof(jrd_1881));
		gds__vtov ((const char*) defaultCollation.c_str(), (char*) jrd_1882.jrd_1883, 32);
		jrd_1882.jrd_1884 = jrd_1890.jrd_1894;
		EXE_start (tdbb, requestHandle2, transaction);
		EXE_send (tdbb, requestHandle2, 0, 34, (UCHAR*) &jrd_1882);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle2, 1, 2, (UCHAR*) &jrd_1885);
		   if (!jrd_1885.jrd_1886) break;
		{
			collationFound = true;
		}
		/*END_FOR*/
		   }
		}

		if (collationFound)
		{
			/*MODIFY CS*/
			{
			
				/*CS.RDB$DEFAULT_COLLATE_NAME.NULL*/
				jrd_1890.jrd_1893 = FALSE;
				strcpy(/*CS.RDB$DEFAULT_COLLATE_NAME*/
				       jrd_1890.jrd_1891, defaultCollation.c_str());
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_1890.jrd_1891, (char*) jrd_1895.jrd_1896, 32);
			jrd_1895.jrd_1897 = jrd_1890.jrd_1893;
			EXE_send (tdbb, requestHandle1, 2, 34, (UCHAR*) &jrd_1895);
			}
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle1, 3, 2, (UCHAR*) &jrd_1898);
	   }
	}

	if (!charSetFound)
		status_exception::raise(Arg::Gds(isc_charset_not_found) << Arg::Str(charSet));

	if (!collationFound)
	{
		status_exception::raise(
			Arg::Gds(isc_collation_not_found) << Arg::Str(defaultCollation) << Arg::Str(charSet));
	}

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER,
		DDL_TRIGGER_ALTER_CHARACTER_SET, charSet);
}


//----------------------


void CommentOnNode::print(string& text) const
{
	text.printf(
		"CommentOnNode\n"
		"  objType: %s\n"
		"  objName: %s\n"
		"  text: %s\n",
		objType, objName.c_str(), this->text.c_str());
}

// select rdb$relation_name from rdb$relation_fields where rdb$field_name = 'RDB$DESCRIPTION';
// gives the list of objects that accept descriptions. At FB2 time, the only
// subobjects with descriptions are relation's fields and procedure's parameters.
// In FB3 we added function's arguments.
void CommentOnNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
	Attachment* const attachment = transaction->tra_attachment;

	const char* tableClause = NULL;
	const char* columnClause = NULL;
	const char* subColumnClause = NULL;
	const char* addWhereClause = NULL;
	Arg::StatusVector status;

	if (objType == obj_parameter)
	{
		fb_assert(subName.hasData());

		PreparedStatement::Builder sql;
		sql << "select 1 from rdb$procedures p join rdb$procedure_parameters pp using (rdb$procedure_name)" <<
			   "where p.rdb$procedure_name =" << objName <<
			   "and p.rdb$package_name is null and pp.rdb$package_name is null" <<
			   "and pp.rdb$parameter_name =" << subName <<
			   "union all" <<
			   "select 2 from rdb$functions f join rdb$function_arguments fa using (rdb$function_name)" <<
			   "where f.rdb$function_name =" << objName <<
			   "and f.rdb$package_name is null and fa.rdb$package_name is null" <<
			   "and fa.rdb$argument_name =" << subName;

		AutoPreparedStatement ps(attachment->prepareStatement(tdbb, transaction, sql));
		AutoResultSet rs(ps->executeQuery(tdbb, transaction));

		if (!rs->fetch(tdbb))
		{
			status_exception::raise(Arg::Gds(isc_dyn_routine_param_not_found) <<
				Arg::Str(subName) << Arg::Str(objName));
		}

		fb_assert(!rs->isNull(1));
		dsc& desc = rs->getDesc(1);
		fb_assert(desc.dsc_dtype == dtype_long);
		const SLONG result = *reinterpret_cast<SLONG*>(desc.dsc_address);

		switch (result)
		{
			case 1:
				objType = obj_procedure;
				break;

			case 2:
				objType = obj_udf;
				break;

			default:
				fb_assert(false);
		}

		if (rs->fetch(tdbb))
		{
			status_exception::raise(Arg::Gds(isc_dyn_routine_param_ambiguous) <<
				Arg::Str(subName) << Arg::Str(objName));
		}
	}

	switch (objType)
	{
		case obj_database:
			tableClause = "rdb$database";
			break;

		case obj_field:
			tableClause = "rdb$fields";
			columnClause = "rdb$field_name";
			status << Arg::Gds(isc_dyn_domain_not_found);
			break;

		case obj_relation:
			if (subName.hasData())
			{
				tableClause = "rdb$relation_fields";
				subColumnClause = "rdb$field_name";
				status << Arg::Gds(isc_dyn_column_does_not_exist) <<
					Arg::Str(subName) << Arg::Str(objName);
			}
			else
			{
				tableClause = "rdb$relations";
				addWhereClause = "rdb$view_blr is null";
				status << Arg::Gds(isc_dyn_table_not_found) << Arg::Str(objName);
			}
			columnClause = "rdb$relation_name";
			break;

		case obj_view:
			tableClause = "rdb$relations";
			columnClause = "rdb$relation_name";
			status << Arg::Gds(isc_dyn_view_not_found) << Arg::Str(objName);
			addWhereClause = "rdb$view_blr is not null";
			break;

		case obj_procedure:
			if (subName.hasData())
			{
				tableClause = "rdb$procedure_parameters";
				subColumnClause = "rdb$parameter_name";
				status << Arg::Gds(isc_dyn_proc_param_not_found) <<
					Arg::Str(subName) << Arg::Str(objName);
			}
			else
			{
				tableClause = "rdb$procedures";
				status << Arg::Gds(isc_dyn_proc_not_found) << Arg::Str(objName);
			}

			addWhereClause = "rdb$package_name is null";
			columnClause = "rdb$procedure_name";
			break;

		case obj_trigger:
			tableClause = "rdb$triggers";
			columnClause = "rdb$trigger_name";
			status << Arg::Gds(isc_dyn_trig_not_found) << Arg::Str(objName);
			break;

		case obj_udf:
			if (subName.hasData())
			{
				tableClause = "rdb$function_arguments";
				subColumnClause = "rdb$argument_name";
				status << Arg::Gds(isc_dyn_func_param_not_found) <<
					Arg::Str(subName) << Arg::Str(objName);
			}
			else
			{
				tableClause = "rdb$functions";
				status << Arg::Gds(isc_dyn_func_not_found) << Arg::Str(objName);
			}

			addWhereClause = "rdb$package_name is null";
			columnClause = "rdb$function_name";
			break;

		case obj_blob_filter:
			tableClause = "rdb$filters";
			columnClause = "rdb$function_name";
			status << Arg::Gds(isc_dyn_filter_not_found) << Arg::Str(objName);
			break;

		case obj_exception:
			tableClause = "rdb$exceptions";
			columnClause = "rdb$exception_name";
			status << Arg::Gds(isc_dyn_exception_not_found) << Arg::Str(objName);
			break;

		case obj_generator:
			tableClause = "rdb$generators";
			columnClause = "rdb$generator_name";
			status << Arg::Gds(isc_dyn_gen_not_found) << Arg::Str(objName);
			break;

		case obj_index:
			tableClause = "rdb$indices";
			columnClause = "rdb$index_name";
			status << Arg::Gds(isc_dyn_index_not_found) << Arg::Str(objName);
			break;

		case obj_sql_role:
			tableClause = "rdb$roles";
			columnClause = "rdb$role_name";
			status << Arg::Gds(isc_dyn_role_not_found) << Arg::Str(objName);
			break;

		case obj_charset:
			tableClause = "rdb$character_sets";
			columnClause = "rdb$character_set_name";
			status << Arg::Gds(isc_dyn_charset_not_found) << Arg::Str(objName);
			break;

		case obj_collation:
			tableClause = "rdb$collations";
			columnClause = "rdb$collation_name";
			status << Arg::Gds(isc_dyn_collation_not_found) << Arg::Str(objName);
			break;

		case obj_package_header:
			tableClause = "rdb$packages";
			columnClause = "rdb$package_name";
			status << Arg::Gds(isc_dyn_package_not_found) << Arg::Str(objName);
			break;

		case obj_schema:
			tableClause = "rdb$schemas";
			columnClause = "rdb$schema_name";
			status << Arg::Gds(isc_dyn_schema_not_found) << Arg::Str(objName);
			break;

		default:
			fb_assert(false);
			return;
	}

	Nullable<string> description;
	if (text.hasData())
		description = text;

	PreparedStatement::Builder sql;
	sql << "update" << tableClause << "set rdb$description =" << description;

	if (columnClause)
	{
		sql << "where" << columnClause << "=" << objName;

		if (subColumnClause)
			sql << "and" << subColumnClause << "=" << subName;
	}

	if (addWhereClause)
		sql << "and" << addWhereClause;

	AutoPreparedStatement ps(attachment->prepareStatement(tdbb, transaction, sql));

	if (ps->executeUpdate(tdbb, transaction) == 0)
		status_exception::raise(status);
}


//----------------------


void CreateAlterFunctionNode::print(string& text) const
{
	text.printf(
		"CreateAlterFunctionNode\n"
		"  name: '%s'  create: %d  alter: %d\n",
		name.c_str(), create, alter);

	if (external)
	{
		string s;
		s.printf("  external -> name: '%s'  engine: '%s'\n",
			external->name.c_str(), external->engine.c_str());
		text += s;
	}

	text += "  Parameters:\n";

	for (size_t i = 0; i < parameters.getCount(); ++i)
	{
		const ParameterClause* parameter = parameters[i];

		string s;
		parameter->print(s);
		text += "    " + s + "\n";
	}
}

DdlNode* CreateAlterFunctionNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	dsqlScratch->flags |= (DsqlCompilerScratch::FLAG_BLOCK | DsqlCompilerScratch::FLAG_FUNCTION);

	const CompoundStmtNode* variables = localDeclList;
	if (variables)
	{
		// insure that variable names do not duplicate parameter names

		SortedArray<MetaName> names;

		for (size_t i = 0; i < parameters.getCount(); ++i)
		{
			ParameterClause* parameter = parameters[i];
			names.add(parameter->name);
		}

		const NestConst<StmtNode>* ptr = variables->statements.begin();
		for (const NestConst<StmtNode>* const end = variables->statements.end(); ptr != end; ++ptr)
		{
			const DeclareVariableNode* varNode = (*ptr)->as<DeclareVariableNode>();

			if (varNode)
			{
				const dsql_fld* field = varNode->dsqlDef->type;
				DEV_BLKCHK(field, dsql_type_fld);

				if (names.exist(field->fld_name))
				{
					status_exception::raise(
						Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
						Arg::Gds(isc_dsql_var_conflict) <<
						Arg::Str(field->fld_name));
				}
			}
		}
	}

	source.ltrim("\n\r\t ");

	bool hasDefaultParams = false;

	// compile default expressions
	for (unsigned i = 0; i < parameters.getCount(); ++i)
	{
		ParameterClause* parameter = parameters[i];

		if (parameter->defaultClause)
		{
			hasDefaultParams = true;
			parameter->defaultClause->value = doDsqlPass(dsqlScratch, parameter->defaultClause->value);
		}
		else if (hasDefaultParams)
		{
			// parameter without default value after parameters with default
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					  Arg::Gds(isc_bad_default_value) <<
					  Arg::Gds(isc_invalid_clause) << Arg::Str("defaults must be last"));
		}
	}

	for (unsigned i = 0; i < parameters.getCount(); ++i)
		parameters[i]->type->resolve(dsqlScratch);

	if (returnType && returnType->type)
		returnType->type->resolve(dsqlScratch);

	return DdlNode::dsqlPass(dsqlScratch);
}

void CreateAlterFunctionNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
	fb_assert(create || alter);

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);
	bool altered = false;

	// first pass
	if (alter)
	{
		if (executeAlter(tdbb, dsqlScratch, transaction, false, true))
		{
			altered = true;
		}
		else
		{
			if (create)	// create or alter
				executeCreate(tdbb, dsqlScratch, transaction);
			else
				status_exception::raise(Arg::Gds(isc_dyn_func_not_found) << Arg::Str(name));
		}
	}
	else
		executeCreate(tdbb, dsqlScratch, transaction);

	compile(tdbb, dsqlScratch);

	executeAlter(tdbb, dsqlScratch, transaction, true, false);	// second pass

	if (package.isEmpty())
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER,
			(altered ? DDL_TRIGGER_ALTER_FUNCTION : DDL_TRIGGER_CREATE_FUNCTION), name);
	}

	savePoint.release();	// everything is ok

	if (alter)
	{
		// Update DSQL cache
		METD_drop_function(transaction, QualifiedName(name, package));
		MET_dsql_cache_release(tdbb, SYM_udf, name, package);
	}
}

void CreateAlterFunctionNode::executeCreate(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          TEXT  jrd_1868 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_1869 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1870 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_1871;	// gds__null_flag 
          SSHORT jrd_1872;	// RDB$RETURN_ARGUMENT 
          SSHORT jrd_1873;	// gds__null_flag 
          SSHORT jrd_1874;	// RDB$LEGACY_FLAG 
          SSHORT jrd_1875;	// gds__null_flag 
          SSHORT jrd_1876;	// gds__null_flag 
          SSHORT jrd_1877;	// RDB$PRIVATE_FLAG 
          SSHORT jrd_1878;	// gds__null_flag 
          SSHORT jrd_1879;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_1880;	// RDB$FUNCTION_ID 
   } jrd_1867;
	Attachment* const attachment = transaction->getAttachment();
	const string& userName = attachment->att_user->usr_user_name;

	if (package.isEmpty())
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_CREATE_FUNCTION, name);

		DYN_UTIL_check_unique_name(tdbb, transaction, name, obj_udf);
	}

	AutoCacheRequest requestHandle(tdbb, drq_s_funcs2, DYN_REQUESTS);

	int faults = 0;

	while (true)
	{
		try
		{
			SINT64 id = DYN_UTIL_gen_unique_id(tdbb, drq_g_nxt_fun_id, "RDB$FUNCTIONS");
			id %= (MAX_SSHORT + 1);

			if (!id)
				continue;

			/*STORE (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
				FUN IN RDB$FUNCTIONS*/
			{
			
			{
				/*FUN.RDB$FUNCTION_ID*/
				jrd_1867.jrd_1880 = id;
				/*FUN.RDB$SYSTEM_FLAG*/
				jrd_1867.jrd_1879 = 0;
				strcpy(/*FUN.RDB$FUNCTION_NAME*/
				       jrd_1867.jrd_1870, name.c_str());

				if (package.hasData())
				{
					/*FUN.RDB$PACKAGE_NAME.NULL*/
					jrd_1867.jrd_1878 = FALSE;
					strcpy(/*FUN.RDB$PACKAGE_NAME*/
					       jrd_1867.jrd_1869, package.c_str());

					/*FUN.RDB$PRIVATE_FLAG.NULL*/
					jrd_1867.jrd_1876 = FALSE;
					/*FUN.RDB$PRIVATE_FLAG*/
					jrd_1867.jrd_1877 = privateScope;

					/*FUN.RDB$OWNER_NAME.NULL*/
					jrd_1867.jrd_1875 = FALSE;
					strcpy(/*FUN.RDB$OWNER_NAME*/
					       jrd_1867.jrd_1868, packageOwner.c_str());
				}
				else
				{
					/*FUN.RDB$PACKAGE_NAME.NULL*/
					jrd_1867.jrd_1878 = TRUE;
					/*FUN.RDB$PRIVATE_FLAG.NULL*/
					jrd_1867.jrd_1876 = TRUE;

					/*FUN.RDB$OWNER_NAME.NULL*/
					jrd_1867.jrd_1875 = FALSE;
					strcpy(/*FUN.RDB$OWNER_NAME*/
					       jrd_1867.jrd_1868, userName.c_str());
				}

				/*FUN.RDB$LEGACY_FLAG.NULL*/
				jrd_1867.jrd_1873 = FALSE;
				/*FUN.RDB$LEGACY_FLAG*/
				jrd_1867.jrd_1874 = isUdf() ? TRUE : FALSE;

				/*FUN.RDB$RETURN_ARGUMENT.NULL*/
				jrd_1867.jrd_1871 = FALSE;
				/*FUN.RDB$RETURN_ARGUMENT*/
				jrd_1867.jrd_1872 = 0;
			}
			/*END_STORE*/
			requestHandle.compile(tdbb, (UCHAR*) jrd_1866, sizeof(jrd_1866));
			EXE_start (tdbb, requestHandle, transaction);
			EXE_send (tdbb, requestHandle, 0, 116, (UCHAR*) &jrd_1867);
			}

			break;
		}
		catch (const status_exception& ex)
		{
			if (ex.value()[1] != isc_unique_key_violation)
				throw;

			if (++faults > MAX_SSHORT)
				throw;

			fb_utils::init_status(tdbb->tdbb_status_vector);
		}
	}

	if (package.isEmpty())
		storePrivileges(tdbb, transaction, name, obj_udf, EXEC_PRIVILEGES);

	executeAlter(tdbb, dsqlScratch, transaction, false, false);
}

bool CreateAlterFunctionNode::executeAlter(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction, bool secondPass, bool runTriggers)
{
   struct {
          SSHORT jrd_1865;	// gds__utility 
   } jrd_1864;
   struct {
          bid  jrd_1845;	// RDB$FUNCTION_BLR 
          bid  jrd_1846;	// RDB$DEBUG_INFO 
          TEXT  jrd_1847 [32];	// RDB$ENGINE_NAME 
          TEXT  jrd_1848 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_1849 [256];	// RDB$ENTRYPOINT 
          bid  jrd_1850;	// RDB$FUNCTION_SOURCE 
          SSHORT jrd_1851;	// gds__null_flag 
          SSHORT jrd_1852;	// gds__null_flag 
          SSHORT jrd_1853;	// gds__null_flag 
          SSHORT jrd_1854;	// gds__null_flag 
          SSHORT jrd_1855;	// gds__null_flag 
          SSHORT jrd_1856;	// gds__null_flag 
          SSHORT jrd_1857;	// RDB$VALID_BLR 
          SSHORT jrd_1858;	// gds__null_flag 
          SSHORT jrd_1859;	// gds__null_flag 
          SSHORT jrd_1860;	// RDB$DETERMINISTIC_FLAG 
          SSHORT jrd_1861;	// RDB$RETURN_ARGUMENT 
          SSHORT jrd_1862;	// gds__null_flag 
          SSHORT jrd_1863;	// RDB$PRIVATE_FLAG 
   } jrd_1844;
   struct {
          bid  jrd_1822;	// RDB$FUNCTION_SOURCE 
          TEXT  jrd_1823 [256];	// RDB$ENTRYPOINT 
          TEXT  jrd_1824 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_1825 [32];	// RDB$ENGINE_NAME 
          bid  jrd_1826;	// RDB$DEBUG_INFO 
          bid  jrd_1827;	// RDB$FUNCTION_BLR 
          TEXT  jrd_1828 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_1829;	// gds__utility 
          SSHORT jrd_1830;	// gds__null_flag 
          SSHORT jrd_1831;	// RDB$PRIVATE_FLAG 
          SSHORT jrd_1832;	// RDB$RETURN_ARGUMENT 
          SSHORT jrd_1833;	// gds__null_flag 
          SSHORT jrd_1834;	// RDB$DETERMINISTIC_FLAG 
          SSHORT jrd_1835;	// gds__null_flag 
          SSHORT jrd_1836;	// gds__null_flag 
          SSHORT jrd_1837;	// RDB$VALID_BLR 
          SSHORT jrd_1838;	// gds__null_flag 
          SSHORT jrd_1839;	// gds__null_flag 
          SSHORT jrd_1840;	// gds__null_flag 
          SSHORT jrd_1841;	// gds__null_flag 
          SSHORT jrd_1842;	// gds__null_flag 
          SSHORT jrd_1843;	// RDB$SYSTEM_FLAG 
   } jrd_1821;
   struct {
          TEXT  jrd_1819 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1820 [32];	// RDB$FUNCTION_NAME 
   } jrd_1818;
	Attachment* const attachment = transaction->getAttachment();

	bool modified = false;
	unsigned returnPos = 0;

	AutoCacheRequest requestHandle(tdbb, drq_m_funcs2, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		FUN IN RDB$FUNCTIONS
		WITH FUN.RDB$FUNCTION_NAME EQ name.c_str() AND
			 FUN.RDB$PACKAGE_NAME EQUIV NULLIF(package.c_str(), '')*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1817, sizeof(jrd_1817));
	gds__vtov ((const char*) package.c_str(), (char*) jrd_1818.jrd_1819, 32);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1818.jrd_1820, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 64, (UCHAR*) &jrd_1818);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 630, (UCHAR*) &jrd_1821);
	   if (!jrd_1821.jrd_1829) break;
	{
		if (/*FUN.RDB$SYSTEM_FLAG*/
		    jrd_1821.jrd_1843)
		{
			status_exception::raise(
				Arg::Gds(isc_dyn_cannot_mod_sysfunc) << /*FUN.RDB$FUNCTION_NAME*/
									jrd_1821.jrd_1828);
		}

		if (!secondPass && runTriggers && package.isEmpty())
		{
			executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
				DDL_TRIGGER_ALTER_FUNCTION, name);
		}

		/*MODIFY FUN*/
		{
		
			if (secondPass)
			{
				/*FUN.RDB$FUNCTION_BLR.NULL*/
				jrd_1821.jrd_1842 = TRUE;
				/*FUN.RDB$DEBUG_INFO.NULL*/
				jrd_1821.jrd_1841 = TRUE;
			}
			else
			{
				/*FUN.RDB$ENGINE_NAME.NULL*/
				jrd_1821.jrd_1840 = TRUE;
				/*FUN.RDB$MODULE_NAME.NULL*/
				jrd_1821.jrd_1839 = TRUE;
				/*FUN.RDB$ENTRYPOINT.NULL*/
				jrd_1821.jrd_1838 = TRUE;
				/*FUN.RDB$VALID_BLR.NULL*/
				jrd_1821.jrd_1836 = TRUE;

				/*FUN.RDB$FUNCTION_SOURCE.NULL*/
				jrd_1821.jrd_1835 = !(source.hasData() && (external || package.isEmpty()));
				if (!/*FUN.RDB$FUNCTION_SOURCE.NULL*/
				     jrd_1821.jrd_1835)
					attachment->storeMetaDataBlob(tdbb, transaction, &/*FUN.RDB$FUNCTION_SOURCE*/
											  jrd_1821.jrd_1822, source);

				/*FUN.RDB$DETERMINISTIC_FLAG.NULL*/
				jrd_1821.jrd_1833 = FALSE;
				/*FUN.RDB$DETERMINISTIC_FLAG*/
				jrd_1821.jrd_1834 = deterministic ? TRUE : FALSE;

				if (isUdf())
				{
					dsql_fld* field = returnType ? returnType->type : NULL;

					if (field)
					{
						// CVC: This is case of "returns <type> [by value|reference]".
						// Some data types can not be returned as value.

						if (returnType->udfMechanism.value == FUN_value &&
							(field->dtype == dtype_text || field->dtype == dtype_varying ||
							 field->dtype == dtype_cstring || field->dtype == dtype_blob ||
							 field->dtype == dtype_timestamp))
						{
							// Return mode by value not allowed for this data type.
							status_exception::raise(
								Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
								Arg::Gds(isc_dsql_command_err) <<
								Arg::Gds(isc_return_mode_err));
						}

						// For functions returning a blob, coerce return argument position to
						// be the last parameter.

						if (field->dtype == dtype_blob)
						{
							returnPos = unsigned(parameters.getCount()) + 1;
							if (returnPos > MAX_UDF_ARGUMENTS)
							{
								// External functions can not have more than 10 parameters
								// Or 9 if the function returns a BLOB.
								status_exception::raise(
									Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
									Arg::Gds(isc_dsql_command_err) <<
									Arg::Gds(isc_extern_func_err));
							}

							/*FUN.RDB$RETURN_ARGUMENT*/
							jrd_1821.jrd_1832 = (SSHORT) returnPos;
						}
						else
							/*FUN.RDB$RETURN_ARGUMENT*/
							jrd_1821.jrd_1832 = 0;
					}
					else	// CVC: This is case of "returns parameter <N>"
					{
						// Function modifies an argument whose value is the function return value.

						if (udfReturnPos < 1 || ULONG(udfReturnPos) > parameters.getCount())
						{
							// CVC: We should devise new msg "position should be between 1 and #params";
							// here it is: dsql_udf_return_pos_err

							// External functions can not have more than 10 parameters
							// Not strictly correct -- return position error
							status_exception::raise(
								Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
								Arg::Gds(isc_dsql_command_err) <<	// gds__extern_func_err
								Arg::Gds(isc_dsql_udf_return_pos_err) << Arg::Num(parameters.getCount()));
						}

						// We'll verify that SCALAR_ARRAY can't be used as a return type.
						// The support for SCALAR_ARRAY is only for input parameters.

						if (parameters[udfReturnPos - 1]->udfMechanism.specified &&
							parameters[udfReturnPos - 1]->udfMechanism.value == FUN_scalar_array)
						{
							status_exception::raise(
								Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
								Arg::Gds(isc_dsql_command_err) <<
								Arg::Gds(isc_random) << "BY SCALAR_ARRAY can't be used as a return parameter");
						}

						/*FUN.RDB$RETURN_ARGUMENT*/
						jrd_1821.jrd_1832 = (SSHORT) udfReturnPos;
					}
				}
			}

			if (external)
			{
				if (!secondPass)
				{
					/*FUN.RDB$ENGINE_NAME.NULL*/
					jrd_1821.jrd_1840 = (SSHORT) external->engine.isEmpty();
					external->engine.copyTo(/*FUN.RDB$ENGINE_NAME*/
								jrd_1821.jrd_1825, sizeof(/*FUN.RDB$ENGINE_NAME*/
	 jrd_1821.jrd_1825));

					if (external->udfModule.length() >= sizeof(/*FUN.RDB$MODULE_NAME*/
										   jrd_1821.jrd_1824))
						status_exception::raise(Arg::Gds(isc_dyn_name_longer));

					/*FUN.RDB$MODULE_NAME.NULL*/
					jrd_1821.jrd_1839 = (SSHORT) external->udfModule.isEmpty();
					external->udfModule.copyTo(/*FUN.RDB$MODULE_NAME*/
								   jrd_1821.jrd_1824, sizeof(/*FUN.RDB$MODULE_NAME*/
	 jrd_1821.jrd_1824));

					if (external->name.length() >= sizeof(/*FUN.RDB$ENTRYPOINT*/
									      jrd_1821.jrd_1823))
						status_exception::raise(Arg::Gds(isc_dyn_name_longer));

					/*FUN.RDB$ENTRYPOINT.NULL*/
					jrd_1821.jrd_1838 = (SSHORT) external->name.isEmpty();
					external->name.copyTo(/*FUN.RDB$ENTRYPOINT*/
							      jrd_1821.jrd_1823, sizeof(/*FUN.RDB$ENTRYPOINT*/
	 jrd_1821.jrd_1823));
				}
			}
			else if (body)
			{
				if (secondPass)
				{
					/*FUN.RDB$VALID_BLR.NULL*/
					jrd_1821.jrd_1836 = FALSE;
					/*FUN.RDB$VALID_BLR*/
					jrd_1821.jrd_1837 = TRUE;

					/*FUN.RDB$FUNCTION_BLR.NULL*/
					jrd_1821.jrd_1842 = FALSE;
					attachment->storeBinaryBlob(tdbb, transaction, &/*FUN.RDB$FUNCTION_BLR*/
											jrd_1821.jrd_1827,
						dsqlScratch->getBlrData());

					/*FUN.RDB$DEBUG_INFO.NULL*/
					jrd_1821.jrd_1841 = FALSE;
					attachment->storeBinaryBlob(tdbb, transaction, &/*FUN.RDB$DEBUG_INFO*/
											jrd_1821.jrd_1826,
						dsqlScratch->getDebugData());
				}
			}

			if (package.hasData())
			{
				/*FUN.RDB$PRIVATE_FLAG.NULL*/
				jrd_1821.jrd_1830 = FALSE;
				/*FUN.RDB$PRIVATE_FLAG*/
				jrd_1821.jrd_1831 = privateScope;
			}
			else
				/*FUN.RDB$PRIVATE_FLAG.NULL*/
				jrd_1821.jrd_1830 = TRUE;

			modified = true;
		/*END_MODIFY*/
		jrd_1844.jrd_1845 = jrd_1821.jrd_1827;
		jrd_1844.jrd_1846 = jrd_1821.jrd_1826;
		gds__vtov((const char*) jrd_1821.jrd_1825, (char*) jrd_1844.jrd_1847, 32);
		gds__vtov((const char*) jrd_1821.jrd_1824, (char*) jrd_1844.jrd_1848, 256);
		gds__vtov((const char*) jrd_1821.jrd_1823, (char*) jrd_1844.jrd_1849, 256);
		jrd_1844.jrd_1850 = jrd_1821.jrd_1822;
		jrd_1844.jrd_1851 = jrd_1821.jrd_1842;
		jrd_1844.jrd_1852 = jrd_1821.jrd_1841;
		jrd_1844.jrd_1853 = jrd_1821.jrd_1840;
		jrd_1844.jrd_1854 = jrd_1821.jrd_1839;
		jrd_1844.jrd_1855 = jrd_1821.jrd_1838;
		jrd_1844.jrd_1856 = jrd_1821.jrd_1836;
		jrd_1844.jrd_1857 = jrd_1821.jrd_1837;
		jrd_1844.jrd_1858 = jrd_1821.jrd_1835;
		jrd_1844.jrd_1859 = jrd_1821.jrd_1833;
		jrd_1844.jrd_1860 = jrd_1821.jrd_1834;
		jrd_1844.jrd_1861 = jrd_1821.jrd_1832;
		jrd_1844.jrd_1862 = jrd_1821.jrd_1830;
		jrd_1844.jrd_1863 = jrd_1821.jrd_1831;
		EXE_send (tdbb, requestHandle, 2, 594, (UCHAR*) &jrd_1844);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1864);
	   }
	}

	if (!secondPass && modified)
	{
		// Get all comments from the old parameter list.
		MetaNameBidMap comments;
		collectParamComments(tdbb, transaction, comments);

		// delete all old arguments
		DropFunctionNode::dropArguments(tdbb, transaction, name, package);

		// and insert the new ones

		if (returnType && returnType->type)
			storeArgument(tdbb, dsqlScratch, transaction, returnPos, true, returnType, NULL);

		for (size_t i = 0; i < parameters.getCount(); ++i)
		{
			ParameterClause* parameter = parameters[i];
			bid comment;

			// Find the original comment to recreate in the new parameter.
			if (!comments.get(parameter->name, comment))
				comment.clear();

			storeArgument(tdbb, dsqlScratch, transaction, i + 1, false, parameter, &comment);
		}
	}

	return modified;
}

void CreateAlterFunctionNode::storeArgument(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction, unsigned pos, bool returnArg, ParameterClause* parameter,
	const bid* comment)
{
   struct {
          bid  jrd_1773;	// RDB$DESCRIPTION 
          bid  jrd_1774;	// RDB$DEFAULT_SOURCE 
          bid  jrd_1775;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_1776 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_1777 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1778 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_1779 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1780 [32];	// RDB$ARGUMENT_NAME 
          TEXT  jrd_1781 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_1782;	// gds__null_flag 
          SSHORT jrd_1783;	// gds__null_flag 
          SSHORT jrd_1784;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_1785;	// gds__null_flag 
          SSHORT jrd_1786;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_1787;	// gds__null_flag 
          SSHORT jrd_1788;	// RDB$ARGUMENT_MECHANISM 
          SSHORT jrd_1789;	// gds__null_flag 
          SSHORT jrd_1790;	// RDB$COLLATION_ID 
          SSHORT jrd_1791;	// gds__null_flag 
          SSHORT jrd_1792;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_1793;	// gds__null_flag 
          SSHORT jrd_1794;	// RDB$FIELD_SCALE 
          SSHORT jrd_1795;	// gds__null_flag 
          SSHORT jrd_1796;	// RDB$FIELD_PRECISION 
          SSHORT jrd_1797;	// gds__null_flag 
          SSHORT jrd_1798;	// RDB$FIELD_LENGTH 
          SSHORT jrd_1799;	// gds__null_flag 
          SSHORT jrd_1800;	// RDB$FIELD_TYPE 
          SSHORT jrd_1801;	// gds__null_flag 
          SSHORT jrd_1802;	// RDB$MECHANISM 
          SSHORT jrd_1803;	// gds__null_flag 
          SSHORT jrd_1804;	// gds__null_flag 
          SSHORT jrd_1805;	// gds__null_flag 
          SSHORT jrd_1806;	// gds__null_flag 
          SSHORT jrd_1807;	// gds__null_flag 
          SSHORT jrd_1808;	// gds__null_flag 
          SSHORT jrd_1809;	// RDB$NULL_FLAG 
          SSHORT jrd_1810;	// gds__null_flag 
          SSHORT jrd_1811;	// RDB$ARGUMENT_POSITION 
          SSHORT jrd_1812;	// gds__null_flag 
          SSHORT jrd_1813;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_1814;	// gds__null_flag 
          SSHORT jrd_1815;	// gds__null_flag 
          SSHORT jrd_1816;	// gds__null_flag 
   } jrd_1772;
	Attachment* const attachment = transaction->getAttachment();
	TypeClause* type = parameter->type;

	AutoCacheRequest requestHandle(tdbb, drq_s_func_args2, DYN_REQUESTS);

	/*STORE (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		ARG IN RDB$FUNCTION_ARGUMENTS*/
	{
	
	{
		/*ARG.RDB$FUNCTION_NAME.NULL*/
		jrd_1772.jrd_1816 = FALSE;
		strcpy(/*ARG.RDB$FUNCTION_NAME*/
		       jrd_1772.jrd_1781, name.c_str());

		/*ARG.RDB$ARGUMENT_NAME.NULL*/
		jrd_1772.jrd_1815 = (SSHORT) parameter->name.isEmpty();
		strcpy(/*ARG.RDB$ARGUMENT_NAME*/
		       jrd_1772.jrd_1780, parameter->name.c_str());

		/*ARG.RDB$PACKAGE_NAME.NULL*/
		jrd_1772.jrd_1814 = (SSHORT) package.isEmpty();
		strcpy(/*ARG.RDB$PACKAGE_NAME*/
		       jrd_1772.jrd_1779, package.c_str());

		/*ARG.RDB$SYSTEM_FLAG*/
		jrd_1772.jrd_1813 = 0;
		/*ARG.RDB$SYSTEM_FLAG.NULL*/
		jrd_1772.jrd_1812 = FALSE;

		/*ARG.RDB$ARGUMENT_POSITION.NULL*/
		jrd_1772.jrd_1810 = FALSE;
		/*ARG.RDB$ARGUMENT_POSITION*/
		jrd_1772.jrd_1811 = pos;

		/*ARG.RDB$NULL_FLAG.NULL*/
		jrd_1772.jrd_1808 = TRUE;
		/*ARG.RDB$RELATION_NAME.NULL*/
		jrd_1772.jrd_1807 = TRUE;
		/*ARG.RDB$FIELD_NAME.NULL*/
		jrd_1772.jrd_1806 = TRUE;
		/*ARG.RDB$FIELD_SOURCE.NULL*/
		jrd_1772.jrd_1805 = TRUE;
		/*ARG.RDB$DEFAULT_VALUE.NULL*/
		jrd_1772.jrd_1804 = TRUE;
		/*ARG.RDB$DEFAULT_SOURCE.NULL*/
		jrd_1772.jrd_1803 = TRUE;

		/*ARG.RDB$MECHANISM.NULL*/
		jrd_1772.jrd_1801 = TRUE;
		/*ARG.RDB$FIELD_TYPE.NULL*/
		jrd_1772.jrd_1799 = TRUE;
		/*ARG.RDB$FIELD_LENGTH.NULL*/
		jrd_1772.jrd_1797 = TRUE;
		/*ARG.RDB$FIELD_PRECISION.NULL*/
		jrd_1772.jrd_1795 = TRUE;
		/*ARG.RDB$FIELD_SCALE.NULL*/
		jrd_1772.jrd_1793 = TRUE;
		/*ARG.RDB$CHARACTER_SET_ID.NULL*/
		jrd_1772.jrd_1791 = TRUE;
		/*ARG.RDB$COLLATION_ID.NULL*/
		jrd_1772.jrd_1789 = TRUE;
		/*ARG.RDB$ARGUMENT_MECHANISM.NULL*/
		jrd_1772.jrd_1787 = TRUE;

		if (!isUdf())
		{
			/*ARG.RDB$ARGUMENT_MECHANISM.NULL*/
			jrd_1772.jrd_1787 = FALSE;
			/*ARG.RDB$ARGUMENT_MECHANISM*/
			jrd_1772.jrd_1788 = (USHORT) (type->fullDomain || type->typeOfName.isEmpty() ?
				prm_mech_normal : prm_mech_type_of);
		}

		if (type->notNull)
		{
			/*ARG.RDB$NULL_FLAG.NULL*/
			jrd_1772.jrd_1808 = FALSE;
			/*ARG.RDB$NULL_FLAG*/
			jrd_1772.jrd_1809 = TRUE;
		}

		if (type->typeOfTable.isEmpty())
		{
			if (type->typeOfName.hasData())
			{
				/*ARG.RDB$FIELD_SOURCE.NULL*/
				jrd_1772.jrd_1805 = FALSE;
				strcpy(/*ARG.RDB$FIELD_SOURCE*/
				       jrd_1772.jrd_1776, type->typeOfName.c_str());
			}
			else
			{
				if (isUdf())
				{
					SSHORT segmentLength, segmentLengthNull;

					/*ARG.RDB$FIELD_TYPE.NULL*/
					jrd_1772.jrd_1799 = FALSE;
					/*ARG.RDB$FIELD_LENGTH.NULL*/
					jrd_1772.jrd_1797 = FALSE;
					updateRdbFields(type,
						/*ARG.RDB$FIELD_TYPE*/
						jrd_1772.jrd_1800,
						/*ARG.RDB$FIELD_LENGTH*/
						jrd_1772.jrd_1798,
						/*ARG.RDB$FIELD_SUB_TYPE.NULL*/
						jrd_1772.jrd_1785, /*ARG.RDB$FIELD_SUB_TYPE*/
  jrd_1772.jrd_1786,
						/*ARG.RDB$FIELD_SCALE.NULL*/
						jrd_1772.jrd_1793, /*ARG.RDB$FIELD_SCALE*/
  jrd_1772.jrd_1794,
						/*ARG.RDB$CHARACTER_SET_ID.NULL*/
						jrd_1772.jrd_1791, /*ARG.RDB$CHARACTER_SET_ID*/
  jrd_1772.jrd_1792,
						/*ARG.RDB$CHARACTER_LENGTH.NULL*/
						jrd_1772.jrd_1783, /*ARG.RDB$CHARACTER_LENGTH*/
  jrd_1772.jrd_1784,
						/*ARG.RDB$FIELD_PRECISION.NULL*/
						jrd_1772.jrd_1795, /*ARG.RDB$FIELD_PRECISION*/
  jrd_1772.jrd_1796,
						/*ARG.RDB$COLLATION_ID.NULL*/
						jrd_1772.jrd_1789, /*ARG.RDB$COLLATION_ID*/
  jrd_1772.jrd_1790,
						segmentLengthNull, segmentLength);
				}
				else
				{
					MetaName fieldName;
					storeGlobalField(tdbb, transaction, fieldName, type);

					/*ARG.RDB$FIELD_SOURCE.NULL*/
					jrd_1772.jrd_1805 = FALSE;
					strcpy(/*ARG.RDB$FIELD_SOURCE*/
					       jrd_1772.jrd_1776, fieldName.c_str());
				}
			}
		}
		else
		{
			/*ARG.RDB$RELATION_NAME.NULL*/
			jrd_1772.jrd_1807 = FALSE;
			strcpy(/*ARG.RDB$RELATION_NAME*/
			       jrd_1772.jrd_1778, type->typeOfTable.c_str());

			/*ARG.RDB$FIELD_NAME.NULL*/
			jrd_1772.jrd_1806 = FALSE;
			strcpy(/*ARG.RDB$FIELD_NAME*/
			       jrd_1772.jrd_1777, type->typeOfName.c_str());

			/*ARG.RDB$FIELD_SOURCE.NULL*/
			jrd_1772.jrd_1805 = FALSE;
			strcpy(/*ARG.RDB$FIELD_SOURCE*/
			       jrd_1772.jrd_1776, type->fieldSource.c_str());
		}

		// ASF: If we used a collate with a domain or table.column type, write it
		// into RDB$FUNCTION_ARGUMENTS.

		if (type->collate.hasData() && type->typeOfName.hasData())
		{
			/*ARG.RDB$COLLATION_ID.NULL*/
			jrd_1772.jrd_1789 = FALSE;
			/*ARG.RDB$COLLATION_ID*/
			jrd_1772.jrd_1790 = type->collationId;
		}

		// ASF: I moved this block to write defaults on RDB$FUNCTION_ARGUMENTS.
		// It was writing in RDB$FIELDS, but that would require special support
		// for packaged functions signature verification.

		if (parameter->defaultClause)
		{
			/*ARG.RDB$DEFAULT_VALUE.NULL*/
			jrd_1772.jrd_1804 = FALSE;
			/*ARG.RDB$DEFAULT_SOURCE.NULL*/
			jrd_1772.jrd_1803 = FALSE;

			attachment->storeMetaDataBlob(tdbb, transaction, &/*ARG.RDB$DEFAULT_SOURCE*/
									  jrd_1772.jrd_1774,
				parameter->defaultClause->source);

			dsqlScratch->getBlrData().clear();

			if (dsqlScratch->isVersion4())
				dsqlScratch->appendUChar(blr_version4);
			else
				dsqlScratch->appendUChar(blr_version5);

			GEN_expr(dsqlScratch, parameter->defaultClause->value);

			dsqlScratch->appendUChar(blr_eoc);

			attachment->storeBinaryBlob(tdbb, transaction, &/*ARG.RDB$DEFAULT_VALUE*/
									jrd_1772.jrd_1775,
				dsqlScratch->getBlrData());
		}

		if (isUdf())
		{
			/*ARG.RDB$MECHANISM.NULL*/
			jrd_1772.jrd_1801 = FALSE;

			if (returnArg &&	// CVC: This is case of "returns <type> [by value|reference]"
				udfReturnPos == 0 && type->dtype == dtype_blob)
			{
				// CVC: I need to test returning blobs by descriptor before allowing the
				// change there. For now, I ignore the return type specification.
				const bool freeIt = parameter->udfMechanism.value < 0;
				/*ARG.RDB$MECHANISM*/
				jrd_1772.jrd_1802 = (SSHORT) ((freeIt ? -1 : 1) * FUN_blob_struct);
				// If we have the FREE_IT set then the blob has to be freed on return.
			}
			else if (parameter->udfMechanism.specified)
				/*ARG.RDB$MECHANISM*/
				jrd_1772.jrd_1802 = (SSHORT) parameter->udfMechanism.value;
			else if (type->dtype == dtype_blob)
				/*ARG.RDB$MECHANISM*/
				jrd_1772.jrd_1802 = (SSHORT) FUN_blob_struct;
			else
				/*ARG.RDB$MECHANISM*/
				jrd_1772.jrd_1802 = (SSHORT) FUN_reference;
		}

		/*ARG.RDB$DESCRIPTION.NULL*/
		jrd_1772.jrd_1782 = !comment || comment->isEmpty();
		if (!/*ARG.RDB$DESCRIPTION.NULL*/
		     jrd_1772.jrd_1782)
			/*ARG.RDB$DESCRIPTION*/
			jrd_1772.jrd_1773 = *comment;
	}
	/*END_STORE*/
	requestHandle.compile(tdbb, (UCHAR*) jrd_1771, sizeof(jrd_1771));
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 286, (UCHAR*) &jrd_1772);
	}
}

void CreateAlterFunctionNode::compile(thread_db* /*tdbb*/, DsqlCompilerScratch* dsqlScratch)
{
	if (invalid)
		status_exception::raise(Arg::Gds(isc_dyn_invalid_ddl_func) << name);

	if (compiled)
		return;

	compiled = true;
	invalid = true;

	if (body)
	{
		dsqlScratch->beginDebug();
		dsqlScratch->getBlrData().clear();

		if (dsqlScratch->isVersion4())
			dsqlScratch->appendUChar(blr_version4);
		else
			dsqlScratch->appendUChar(blr_version5);

		dsqlScratch->appendUChar(blr_begin);

		Array<NestConst<ParameterClause> > returns;
		returns.add(returnType);
		dsqlScratch->genParameters(parameters, returns);

		if (parameters.getCount() != 0)
		{
			dsqlScratch->appendUChar(blr_receive);
			dsqlScratch->appendUChar(0);
		}

		dsqlScratch->appendUChar(blr_begin);

		for (unsigned i = 0; i < parameters.getCount(); ++i)
		{
			ParameterClause* parameter = parameters[i];

			if (parameter->type->fullDomain || parameter->type->notNull)
			{
				// ASF: To validate input parameters we need only to read its value.
				// Assigning it to null is an easy way to do this.
				dsqlScratch->appendUChar(blr_assignment);
				dsqlScratch->appendUChar(blr_parameter2);
				dsqlScratch->appendUChar(0);	// input
				dsqlScratch->appendUShort(i * 2);
				dsqlScratch->appendUShort(i * 2 + 1);
				dsqlScratch->appendUChar(blr_null);
			}
		}

		dsql_var* const variable = dsqlScratch->outputVariables[0];
		dsqlScratch->putLocalVariable(variable, 0, NULL);

		// ASF: This is here to not change the old logic (proc_flag)
		// of previous calls to PASS1_node and PASS1_statement.
		dsqlScratch->setPsql(true);

		dsqlScratch->putLocalVariables(localDeclList, 1);

		dsqlScratch->loopLevel = 0;
		dsqlScratch->cursorNumber = 0;

		StmtNode* stmtNode = body->dsqlPass(dsqlScratch);
		GEN_hidden_variables(dsqlScratch);

		dsqlScratch->appendUChar(blr_stall);
		// put a label before body of procedure,
		// so that any EXIT statement can get out
		dsqlScratch->appendUChar(blr_label);
		dsqlScratch->appendUChar(0);

		stmtNode->genBlr(dsqlScratch);

		dsqlScratch->getStatement()->setType(DsqlCompiledStatement::TYPE_DDL);
		dsqlScratch->appendUChar(blr_end);
		dsqlScratch->genReturn(false);
		dsqlScratch->appendUChar(blr_end);
		dsqlScratch->appendUChar(blr_eoc);

		dsqlScratch->endDebug();
	}

	invalid = false;
}

void CreateAlterFunctionNode::collectParamComments(thread_db* tdbb, jrd_tra* transaction,
	MetaNameBidMap& items)
{
   struct {
          bid  jrd_1768;	// RDB$DESCRIPTION 
          TEXT  jrd_1769 [32];	// RDB$ARGUMENT_NAME 
          SSHORT jrd_1770;	// gds__utility 
   } jrd_1767;
   struct {
          TEXT  jrd_1765 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1766 [32];	// RDB$FUNCTION_NAME 
   } jrd_1764;
	AutoRequest requestHandle;

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		ARG IN RDB$FUNCTION_ARGUMENTS
		WITH ARG.RDB$FUNCTION_NAME EQ name.c_str() AND
			 ARG.RDB$PACKAGE_NAME EQUIV NULLIF(package.c_str(), '') AND
			 ARG.RDB$DESCRIPTION NOT MISSING*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1763, sizeof(jrd_1763));
	gds__vtov ((const char*) package.c_str(), (char*) jrd_1764.jrd_1765, 32);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1764.jrd_1766, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 64, (UCHAR*) &jrd_1764);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 42, (UCHAR*) &jrd_1767);
	   if (!jrd_1767.jrd_1770) break;
	{
		items.put(/*ARG.RDB$ARGUMENT_NAME*/
			  jrd_1767.jrd_1769, /*ARG.RDB$DESCRIPTION*/
  jrd_1767.jrd_1768);
	}
	/*END_FOR*/
	   }
	}
}


//----------------------


void AlterExternalFunctionNode::print(string& text) const
{
	text.printf(
		"AlterExternalFunctionNode\n"
		"  name: '%s'\n"
		"  entrypoint: '%s'\n"
		"  module: '%s'\n",
		name.c_str(), clauses.name.c_str(), clauses.udfModule.c_str());
}

// Allow changing the entry point and/or the module name of a UDF.
void AlterExternalFunctionNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1762;	// gds__utility 
   } jrd_1761;
   struct {
          TEXT  jrd_1757 [256];	// RDB$ENTRYPOINT 
          TEXT  jrd_1758 [256];	// RDB$MODULE_NAME 
          SSHORT jrd_1759;	// gds__null_flag 
          SSHORT jrd_1760;	// gds__null_flag 
   } jrd_1756;
   struct {
          TEXT  jrd_1747 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_1748 [256];	// RDB$ENTRYPOINT 
          bid  jrd_1749;	// RDB$FUNCTION_BLR 
          TEXT  jrd_1750 [32];	// RDB$ENGINE_NAME 
          SSHORT jrd_1751;	// gds__utility 
          SSHORT jrd_1752;	// gds__null_flag 
          SSHORT jrd_1753;	// gds__null_flag 
          SSHORT jrd_1754;	// gds__null_flag 
          SSHORT jrd_1755;	// gds__null_flag 
   } jrd_1746;
   struct {
          TEXT  jrd_1745 [32];	// RDB$FUNCTION_NAME 
   } jrd_1744;
	if (clauses.name.isEmpty() && clauses.udfModule.isEmpty())
	{
		status_exception::raise(
			Arg::Gds(isc_sqlerr) << Arg::Num(-104) /*** FIXME: <<
			// Unexpected end of command
			Arg::Gds(isc_command_end_err2) << Arg::Num(node->nod_line) <<
			Arg::Num(node->nod_column + obj_name->str_length)***/);	// + strlen("FUNCTION")
	}

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_m_fun, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FUN IN RDB$FUNCTIONS
		WITH FUN.RDB$FUNCTION_NAME EQ name.c_str() AND
			 FUN.RDB$PACKAGE_NAME MISSING*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1743, sizeof(jrd_1743));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1744.jrd_1745, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1744);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 562, (UCHAR*) &jrd_1746);
	   if (!jrd_1746.jrd_1751) break;
	{
		found = true;

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_ALTER_FUNCTION, name);

		if (!/*FUN.RDB$ENGINE_NAME.NULL*/
		     jrd_1746.jrd_1755 || !/*FUN.RDB$FUNCTION_BLR.NULL*/
     jrd_1746.jrd_1754)
			status_exception::raise(Arg::Gds(isc_dyn_newfc_oldsyntax) << name);

		/*MODIFY FUN*/
		{
		
			if (clauses.name.hasData())
			{
				if (clauses.name.length() >= sizeof(/*FUN.RDB$ENTRYPOINT*/
								    jrd_1746.jrd_1748))
					status_exception::raise(Arg::Gds(isc_dyn_name_longer));

				/*FUN.RDB$ENTRYPOINT.NULL*/
				jrd_1746.jrd_1753 = FALSE;
				strcpy(/*FUN.RDB$ENTRYPOINT*/
				       jrd_1746.jrd_1748, clauses.name.c_str());
			}

			if (clauses.udfModule.hasData())
			{
				if (clauses.udfModule.length() >= sizeof(/*FUN.RDB$MODULE_NAME*/
									 jrd_1746.jrd_1747))
					status_exception::raise(Arg::Gds(isc_dyn_name_longer));

				/*FUN.RDB$MODULE_NAME.NULL*/
				jrd_1746.jrd_1752 = FALSE;
				strcpy(/*FUN.RDB$MODULE_NAME*/
				       jrd_1746.jrd_1747, clauses.udfModule.c_str());
			}
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_1746.jrd_1748, (char*) jrd_1756.jrd_1757, 256);
		gds__vtov((const char*) jrd_1746.jrd_1747, (char*) jrd_1756.jrd_1758, 256);
		jrd_1756.jrd_1759 = jrd_1746.jrd_1753;
		jrd_1756.jrd_1760 = jrd_1746.jrd_1752;
		EXE_send (tdbb, request, 2, 516, (UCHAR*) &jrd_1756);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1761);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_ALTER_FUNCTION, name);
	else
	{
		// msg 41: "Function %s not found"
		status_exception::raise(Arg::PrivateDyn(41) << name);
	}

	savePoint.release();	// everything is ok

	// Update DSQL cache
	METD_drop_function(transaction, QualifiedName(name, ""));
	MET_dsql_cache_release(tdbb, SYM_udf, name, "");
}


//----------------------


void DropFunctionNode::dropArguments(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& functionName, const MetaName& packageName)
{
   struct {
          SSHORT jrd_1726;	// gds__utility 
   } jrd_1725;
   struct {
          SSHORT jrd_1724;	// gds__utility 
   } jrd_1723;
   struct {
          SSHORT jrd_1722;	// gds__utility 
   } jrd_1721;
   struct {
          TEXT  jrd_1719 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1720 [32];	// RDB$FIELD_SOURCE 
   } jrd_1718;
   struct {
          SSHORT jrd_1742;	// gds__utility 
   } jrd_1741;
   struct {
          SSHORT jrd_1740;	// gds__utility 
   } jrd_1739;
   struct {
          TEXT  jrd_1732 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1733 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_1734 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_1735;	// gds__utility 
          SSHORT jrd_1736;	// gds__null_flag 
          SSHORT jrd_1737;	// gds__null_flag 
          SSHORT jrd_1738;	// gds__null_flag 
   } jrd_1731;
   struct {
          TEXT  jrd_1729 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1730 [32];	// RDB$FUNCTION_NAME 
   } jrd_1728;
	AutoCacheRequest requestHandle(tdbb, drq_e_func_args, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		ARG IN RDB$FUNCTION_ARGUMENTS
		WITH ARG.RDB$FUNCTION_NAME EQ functionName.c_str() AND
			 ARG.RDB$PACKAGE_NAME EQUIV NULLIF(packageName.c_str(), '')*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1727, sizeof(jrd_1727));
	gds__vtov ((const char*) packageName.c_str(), (char*) jrd_1728.jrd_1729, 32);
	gds__vtov ((const char*) functionName.c_str(), (char*) jrd_1728.jrd_1730, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 64, (UCHAR*) &jrd_1728);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 104, (UCHAR*) &jrd_1731);
	   if (!jrd_1731.jrd_1735) break;
	{
		// get rid of arguments in rdb$fields
		if (!/*ARG.RDB$FIELD_SOURCE.NULL*/
		     jrd_1731.jrd_1738 && /*ARG.RDB$RELATION_NAME.NULL*/
    jrd_1731.jrd_1737 && /*ARG.RDB$FIELD_NAME.NULL*/
    jrd_1731.jrd_1736)
		{
			AutoCacheRequest requestHandle2(tdbb, drq_e_arg_gfld, DYN_REQUESTS);

			/*FOR (REQUEST_HANDLE requestHandle2 TRANSACTION_HANDLE transaction)
				FLD IN RDB$FIELDS
				WITH FLD.RDB$FIELD_NAME EQ ARG.RDB$FIELD_SOURCE AND
					 FLD.RDB$FIELD_NAME STARTING WITH IMPLICIT_DOMAIN_PREFIX AND
					 FLD.RDB$SYSTEM_FLAG EQ 0*/
			{
			requestHandle2.compile(tdbb, (UCHAR*) jrd_1717, sizeof(jrd_1717));
			gds__vtov ((const char*) IMPLICIT_DOMAIN_PREFIX, (char*) jrd_1718.jrd_1719, 32);
			gds__vtov ((const char*) jrd_1731.jrd_1734, (char*) jrd_1718.jrd_1720, 32);
			EXE_start (tdbb, requestHandle2, transaction);
			EXE_send (tdbb, requestHandle2, 0, 64, (UCHAR*) &jrd_1718);
			while (1)
			   {
			   EXE_receive (tdbb, requestHandle2, 1, 2, (UCHAR*) &jrd_1721);
			   if (!jrd_1721.jrd_1722) break;
			{
				/*ERASE FLD;*/
				EXE_send (tdbb, requestHandle2, 2, 2, (UCHAR*) &jrd_1723);
			}
			/*END_FOR*/
			   EXE_send (tdbb, requestHandle2, 3, 2, (UCHAR*) &jrd_1725);
			   }
			}
		}

		/*ERASE ARG;*/
		EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1739);
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1741);
	   }
	}
}

void DropFunctionNode::print(string& text) const
{
	text.printf(
		"DropFunctionNode\n"
		"  name: '%s'\n",
		name.c_str());
}

DdlNode* DropFunctionNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	dsqlScratch->flags |= (DsqlCompilerScratch::FLAG_BLOCK | DsqlCompilerScratch::FLAG_FUNCTION);
	return DdlNode::dsqlPass(dsqlScratch);
}

void DropFunctionNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1692;	// gds__utility 
   } jrd_1691;
   struct {
          SSHORT jrd_1690;	// gds__utility 
   } jrd_1689;
   struct {
          SSHORT jrd_1688;	// gds__utility 
   } jrd_1687;
   struct {
          TEXT  jrd_1685 [32];	// RDB$USER 
          SSHORT jrd_1686;	// RDB$USER_TYPE 
   } jrd_1684;
   struct {
          SSHORT jrd_1702;	// gds__utility 
   } jrd_1701;
   struct {
          SSHORT jrd_1700;	// gds__utility 
   } jrd_1699;
   struct {
          SSHORT jrd_1698;	// gds__utility 
   } jrd_1697;
   struct {
          TEXT  jrd_1695 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1696;	// RDB$OBJECT_TYPE 
   } jrd_1694;
   struct {
          SSHORT jrd_1716;	// gds__utility 
   } jrd_1715;
   struct {
          SSHORT jrd_1714;	// gds__utility 
   } jrd_1713;
   struct {
          TEXT  jrd_1708 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_1709 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_1710;	// gds__utility 
          SSHORT jrd_1711;	// gds__null_flag 
          SSHORT jrd_1712;	// RDB$SYSTEM_FLAG 
   } jrd_1707;
   struct {
          TEXT  jrd_1705 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1706 [32];	// RDB$FUNCTION_NAME 
   } jrd_1704;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);
	bool found = false;

	dropArguments(tdbb, transaction, name, package);

	AutoCacheRequest requestHandle(tdbb, drq_e_funcs, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		FUN IN RDB$FUNCTIONS
		WITH FUN.RDB$FUNCTION_NAME EQ name.c_str() AND
			 FUN.RDB$PACKAGE_NAME EQUIV NULLIF(package.c_str(), '')*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1703, sizeof(jrd_1703));
	gds__vtov ((const char*) package.c_str(), (char*) jrd_1704.jrd_1705, 32);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1704.jrd_1706, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 64, (UCHAR*) &jrd_1704);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 70, (UCHAR*) &jrd_1707);
	   if (!jrd_1707.jrd_1710) break;
	{
		if (/*FUN.RDB$SYSTEM_FLAG*/
		    jrd_1707.jrd_1712)
			status_exception::raise(Arg::Gds(isc_dyn_cannot_mod_sysfunc) << /*FUN.RDB$FUNCTION_NAME*/
											jrd_1707.jrd_1709);

		if (package.isEmpty())
			executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_DROP_FUNCTION, name);

		/*ERASE FUN;*/
		EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1713);

		if (!/*FUN.RDB$SECURITY_CLASS.NULL*/
		     jrd_1707.jrd_1711)
			deleteSecurityClass(tdbb, transaction, /*FUN.RDB$SECURITY_CLASS*/
							       jrd_1707.jrd_1708);

		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1715);
	   }
	}

	if (!found && !silent)
		status_exception::raise(Arg::Gds(isc_dyn_func_not_found) << Arg::Str(name));

	if (package.isEmpty())
	{
		requestHandle.reset(tdbb, drq_e_fun_prvs, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
			PRIV IN RDB$USER_PRIVILEGES WITH PRIV.RDB$RELATION_NAME EQ name.c_str()
				AND PRIV.RDB$OBJECT_TYPE = obj_udf*/
		{
		requestHandle.compile(tdbb, (UCHAR*) jrd_1693, sizeof(jrd_1693));
		gds__vtov ((const char*) name.c_str(), (char*) jrd_1694.jrd_1695, 32);
		jrd_1694.jrd_1696 = obj_udf;
		EXE_start (tdbb, requestHandle, transaction);
		EXE_send (tdbb, requestHandle, 0, 34, (UCHAR*) &jrd_1694);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle, 1, 2, (UCHAR*) &jrd_1697);
		   if (!jrd_1697.jrd_1698) break;
		{
			/*ERASE PRIV;*/
			EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1699);
		}
		/*END_FOR*/
		   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1701);
		   }
		}

		requestHandle.reset(tdbb, drq_e_fun_prv, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
			PRIV IN RDB$USER_PRIVILEGES WITH PRIV.RDB$USER EQ name.c_str()
				AND PRIV.RDB$USER_TYPE = obj_udf*/
		{
		requestHandle.compile(tdbb, (UCHAR*) jrd_1683, sizeof(jrd_1683));
		gds__vtov ((const char*) name.c_str(), (char*) jrd_1684.jrd_1685, 32);
		jrd_1684.jrd_1686 = obj_udf;
		EXE_start (tdbb, requestHandle, transaction);
		EXE_send (tdbb, requestHandle, 0, 34, (UCHAR*) &jrd_1684);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle, 1, 2, (UCHAR*) &jrd_1687);
		   if (!jrd_1687.jrd_1688) break;
		{
			/*ERASE PRIV;*/
			EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1689);
		}
		/*END_FOR*/
		   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1691);
		   }
		}
	}

	if (found && package.isEmpty())
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_FUNCTION, name);

	savePoint.release();	// everything is ok

	// Update DSQL cache
	METD_drop_function(transaction, QualifiedName(name, package));
	MET_dsql_cache_release(tdbb, SYM_udf, name, package);
}


//----------------------


void CreateAlterProcedureNode::print(string& text) const
{
	text.printf(
		"CreateAlterProcedureNode\n"
		"  name: '%s'  create: %d  alter: %d\n",
		name.c_str(), create, alter);

	if (external)
	{
		string s;
		s.printf("  external -> name: '%s'  engine: '%s'\n",
			external->name.c_str(), external->engine.c_str());
		text += s;
	}

	text += "  Parameters:\n";

	for (size_t i = 0; i < parameters.getCount(); ++i)
	{
		const ParameterClause* parameter = parameters[i];

		string s;
		parameter->print(s);
		text += "    " + s + "\n";
	}

	text += "  Returns:\n";

	for (size_t i = 0; i < returns.getCount(); ++i)
	{
		const ParameterClause* parameter = returns[i];

		string s;
		parameter->print(s);
		text += "    " + s + "\n";
	}
}

DdlNode* CreateAlterProcedureNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	dsqlScratch->flags |= (DsqlCompilerScratch::FLAG_BLOCK | DsqlCompilerScratch::FLAG_PROCEDURE);

	const CompoundStmtNode* variables = localDeclList;
	if (variables)
	{
		// insure that variable names do not duplicate parameter names

		SortedArray<MetaName> names;

		for (size_t i = 0; i < parameters.getCount(); ++i)
		{
			ParameterClause* parameter = parameters[i];
			names.add(parameter->name);
		}

		for (size_t i = 0; i < returns.getCount(); ++i)
		{
			ParameterClause* parameter = returns[i];
			names.add(parameter->name);
		}

		const NestConst<StmtNode>* ptr = variables->statements.begin();
		for (const NestConst<StmtNode>* const end = variables->statements.end(); ptr != end; ++ptr)
		{
			const DeclareVariableNode* varNode = (*ptr)->as<DeclareVariableNode>();

			if (varNode)
			{
				const dsql_fld* field = varNode->dsqlDef->type;
				DEV_BLKCHK(field, dsql_type_fld);

				if (names.exist(field->fld_name))
				{
					status_exception::raise(
						Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
						Arg::Gds(isc_dsql_var_conflict) <<
						Arg::Str(field->fld_name));
				}
			}
		}
	}

	source.ltrim("\n\r\t ");

	bool hasDefaultParams = false;

	// compile default expressions
	for (unsigned i = 0; i < parameters.getCount(); ++i)
	{
		ParameterClause* parameter = parameters[i];

		if (parameter->defaultClause)
		{
			hasDefaultParams = true;
			parameter->defaultClause->value = doDsqlPass(dsqlScratch, parameter->defaultClause->value);
		}
		else if (hasDefaultParams)
		{
			// parameter without default value after parameters with default
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					  Arg::Gds(isc_bad_default_value) <<
					  Arg::Gds(isc_invalid_clause) << Arg::Str("defaults must be last"));
		}
	}

	for (unsigned i = 0; i < parameters.getCount(); ++i)
		parameters[i]->type->resolve(dsqlScratch);

	for (unsigned i = 0; i < returns.getCount(); ++i)
		returns[i]->type->resolve(dsqlScratch);

	return DdlNode::dsqlPass(dsqlScratch);
}

void CreateAlterProcedureNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
	fb_assert(create || alter);

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);
	bool altered = false;

	// first pass
	if (alter)
	{
		if (executeAlter(tdbb, dsqlScratch, transaction, false, true))
			altered = true;
		else
		{
			if (create)	// create or alter
				executeCreate(tdbb, dsqlScratch, transaction);
			else
				status_exception::raise(Arg::Gds(isc_dyn_proc_not_found) << Arg::Str(name));
		}
	}
	else
		executeCreate(tdbb, dsqlScratch, transaction);

	compile(tdbb, dsqlScratch);

	executeAlter(tdbb, dsqlScratch, transaction, true, false);	// second pass

	if (package.isEmpty())
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER,
			(altered ? DDL_TRIGGER_ALTER_PROCEDURE : DDL_TRIGGER_CREATE_PROCEDURE), name);
	}

	savePoint.release();	// everything is ok

	if (alter)
	{
		// Update DSQL cache
		METD_drop_procedure(transaction, QualifiedName(name, package));
		MET_dsql_cache_release(tdbb, SYM_procedure, name, package);
	}
}

void CreateAlterProcedureNode::executeCreate(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          TEXT  jrd_1675 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_1676 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1677 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_1678;	// gds__null_flag 
          SSHORT jrd_1679;	// RDB$PRIVATE_FLAG 
          SSHORT jrd_1680;	// gds__null_flag 
          SSHORT jrd_1681;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_1682;	// RDB$PROCEDURE_ID 
   } jrd_1674;
	Attachment* const attachment = transaction->getAttachment();
	const string& userName = attachment->att_user->usr_user_name;

	if (package.isEmpty())
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_CREATE_PROCEDURE, name);

		DYN_UTIL_check_unique_name(tdbb, transaction, name, obj_procedure);
	}

	AutoCacheRequest requestHandle(tdbb, drq_s_prcs2, DYN_REQUESTS);

	int faults = 0;

	while (true)
	{
		try
		{
			SINT64 id = DYN_UTIL_gen_unique_id(tdbb, drq_g_nxt_prc_id, "RDB$PROCEDURES");
			id %= (MAX_SSHORT + 1);

			if (!id)
				continue;

			/*STORE (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
				P IN RDB$PROCEDURES*/
			{
			
			{
				/*P.RDB$PROCEDURE_ID*/
				jrd_1674.jrd_1682 = id;
				/*P.RDB$SYSTEM_FLAG*/
				jrd_1674.jrd_1681 = 0;
				strcpy(/*P.RDB$PROCEDURE_NAME*/
				       jrd_1674.jrd_1677, name.c_str());

				if (package.hasData())
				{
					/*P.RDB$PACKAGE_NAME.NULL*/
					jrd_1674.jrd_1680 = FALSE;
					strcpy(/*P.RDB$PACKAGE_NAME*/
					       jrd_1674.jrd_1676, package.c_str());

					/*P.RDB$PRIVATE_FLAG.NULL*/
					jrd_1674.jrd_1678 = FALSE;
					/*P.RDB$PRIVATE_FLAG*/
					jrd_1674.jrd_1679 = privateScope;

					strcpy(/*P.RDB$OWNER_NAME*/
					       jrd_1674.jrd_1675, packageOwner.c_str());
				}
				else
				{
					/*P.RDB$PACKAGE_NAME.NULL*/
					jrd_1674.jrd_1680 = TRUE;
					/*P.RDB$PRIVATE_FLAG.NULL*/
					jrd_1674.jrd_1678 = TRUE;

					strcpy(/*P.RDB$OWNER_NAME*/
					       jrd_1674.jrd_1675, userName.c_str());
				}
			}
			/*END_STORE*/
			requestHandle.compile(tdbb, (UCHAR*) jrd_1673, sizeof(jrd_1673));
			EXE_start (tdbb, requestHandle, transaction);
			EXE_send (tdbb, requestHandle, 0, 106, (UCHAR*) &jrd_1674);
			}

			break;
		}
		catch (const status_exception& ex)
		{
			if (ex.value()[1] != isc_unique_key_violation)
				throw;

			if (++faults > MAX_SSHORT)
				throw;

			fb_utils::init_status(tdbb->tdbb_status_vector);
		}
	}

	if (package.isEmpty())
		storePrivileges(tdbb, transaction, name, obj_procedure, EXEC_PRIVILEGES);

	executeAlter(tdbb, dsqlScratch, transaction, false, false);
}

bool CreateAlterProcedureNode::executeAlter(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction, bool secondPass, bool runTriggers)
{
   struct {
          SSHORT jrd_1625;	// gds__utility 
   } jrd_1624;
   struct {
          TEXT  jrd_1623 [32];	// RDB$FIELD_SOURCE 
   } jrd_1622;
   struct {
          TEXT  jrd_1619 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_1620 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_1621;	// gds__utility 
   } jrd_1618;
   struct {
          TEXT  jrd_1615 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1616 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_1617;	// RDB$CONTEXT_TYPE 
   } jrd_1614;
   struct {
          SSHORT jrd_1672;	// gds__utility 
   } jrd_1671;
   struct {
          bid  jrd_1653;	// RDB$PROCEDURE_BLR 
          bid  jrd_1654;	// RDB$DEBUG_INFO 
          TEXT  jrd_1655 [32];	// RDB$ENGINE_NAME 
          TEXT  jrd_1656 [256];	// RDB$ENTRYPOINT 
          bid  jrd_1657;	// RDB$PROCEDURE_SOURCE 
          SSHORT jrd_1658;	// gds__null_flag 
          SSHORT jrd_1659;	// gds__null_flag 
          SSHORT jrd_1660;	// gds__null_flag 
          SSHORT jrd_1661;	// RDB$PROCEDURE_TYPE 
          SSHORT jrd_1662;	// RDB$PROCEDURE_INPUTS 
          SSHORT jrd_1663;	// RDB$PROCEDURE_OUTPUTS 
          SSHORT jrd_1664;	// gds__null_flag 
          SSHORT jrd_1665;	// gds__null_flag 
          SSHORT jrd_1666;	// gds__null_flag 
          SSHORT jrd_1667;	// gds__null_flag 
          SSHORT jrd_1668;	// RDB$VALID_BLR 
          SSHORT jrd_1669;	// gds__null_flag 
          SSHORT jrd_1670;	// RDB$PRIVATE_FLAG 
   } jrd_1652;
   struct {
          bid  jrd_1631;	// RDB$PROCEDURE_SOURCE 
          TEXT  jrd_1632 [256];	// RDB$ENTRYPOINT 
          TEXT  jrd_1633 [32];	// RDB$ENGINE_NAME 
          bid  jrd_1634;	// RDB$DEBUG_INFO 
          bid  jrd_1635;	// RDB$PROCEDURE_BLR 
          TEXT  jrd_1636 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_1637;	// gds__utility 
          SSHORT jrd_1638;	// gds__null_flag 
          SSHORT jrd_1639;	// RDB$PRIVATE_FLAG 
          SSHORT jrd_1640;	// gds__null_flag 
          SSHORT jrd_1641;	// RDB$VALID_BLR 
          SSHORT jrd_1642;	// gds__null_flag 
          SSHORT jrd_1643;	// gds__null_flag 
          SSHORT jrd_1644;	// gds__null_flag 
          SSHORT jrd_1645;	// RDB$PROCEDURE_OUTPUTS 
          SSHORT jrd_1646;	// RDB$PROCEDURE_INPUTS 
          SSHORT jrd_1647;	// gds__null_flag 
          SSHORT jrd_1648;	// RDB$PROCEDURE_TYPE 
          SSHORT jrd_1649;	// gds__null_flag 
          SSHORT jrd_1650;	// gds__null_flag 
          SSHORT jrd_1651;	// RDB$SYSTEM_FLAG 
   } jrd_1630;
   struct {
          TEXT  jrd_1628 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1629 [32];	// RDB$PROCEDURE_NAME 
   } jrd_1627;
	Attachment* const attachment = transaction->getAttachment();
	AutoCacheRequest requestHandle(tdbb, drq_m_prcs2, DYN_REQUESTS);
	bool modified = false;

	DsqlCompiledStatement* statement = dsqlScratch->getStatement();

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		P IN RDB$PROCEDURES
		WITH P.RDB$PROCEDURE_NAME EQ name.c_str() AND
			 P.RDB$PACKAGE_NAME EQUIV NULLIF(package.c_str(), '')*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1626, sizeof(jrd_1626));
	gds__vtov ((const char*) package.c_str(), (char*) jrd_1627.jrd_1628, 32);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1627.jrd_1629, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 64, (UCHAR*) &jrd_1627);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 374, (UCHAR*) &jrd_1630);
	   if (!jrd_1630.jrd_1637) break;
	{
		if (/*P.RDB$SYSTEM_FLAG*/
		    jrd_1630.jrd_1651)
			status_exception::raise(Arg::Gds(isc_dyn_cannot_mod_sysproc) << /*P.RDB$PROCEDURE_NAME*/
											jrd_1630.jrd_1636);

		if (!secondPass && runTriggers && package.isEmpty())
		{
			executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
				DDL_TRIGGER_ALTER_PROCEDURE, name);
		}

		/*MODIFY P*/
		{
		
			if (secondPass)
			{
				/*P.RDB$PROCEDURE_BLR.NULL*/
				jrd_1630.jrd_1650 = TRUE;
				/*P.RDB$DEBUG_INFO.NULL*/
				jrd_1630.jrd_1649 = TRUE;
				/*P.RDB$PROCEDURE_TYPE.NULL*/
				jrd_1630.jrd_1647 = TRUE;

				/*P.RDB$PROCEDURE_INPUTS*/
				jrd_1630.jrd_1646 = (USHORT) parameters.getCount();
				/*P.RDB$PROCEDURE_OUTPUTS*/
				jrd_1630.jrd_1645 = (USHORT) returns.getCount();
			}
			else
			{
				/*P.RDB$ENGINE_NAME.NULL*/
				jrd_1630.jrd_1644 = TRUE;
				/*P.RDB$ENTRYPOINT.NULL*/
				jrd_1630.jrd_1643 = TRUE;
				/*P.RDB$PROCEDURE_SOURCE.NULL*/
				jrd_1630.jrd_1642 = TRUE;
				/*P.RDB$VALID_BLR.NULL*/
				jrd_1630.jrd_1640 = TRUE;

				/*P.RDB$PROCEDURE_SOURCE.NULL*/
				jrd_1630.jrd_1642 = !(source.hasData() && (external || package.isEmpty()));
				if (!/*P.RDB$PROCEDURE_SOURCE.NULL*/
				     jrd_1630.jrd_1642)
					attachment->storeMetaDataBlob(tdbb, transaction, &/*P.RDB$PROCEDURE_SOURCE*/
											  jrd_1630.jrd_1631, source);

				if (package.hasData())
				{
					/*P.RDB$PRIVATE_FLAG.NULL*/
					jrd_1630.jrd_1638 = FALSE;
					/*P.RDB$PRIVATE_FLAG*/
					jrd_1630.jrd_1639 = privateScope;
				}
				else
					/*P.RDB$PRIVATE_FLAG.NULL*/
					jrd_1630.jrd_1638 = TRUE;
			}

			if (external)
			{
				if (secondPass)
				{
					/*P.RDB$PROCEDURE_TYPE.NULL*/
					jrd_1630.jrd_1647 = FALSE;
					/*P.RDB$PROCEDURE_TYPE*/
					jrd_1630.jrd_1648 = (USHORT) prc_selectable;
				}
				else
				{
					/*P.RDB$ENGINE_NAME.NULL*/
					jrd_1630.jrd_1644 = FALSE;
					strcpy(/*P.RDB$ENGINE_NAME*/
					       jrd_1630.jrd_1633, external->engine.c_str());

					if (external->name.length() >= sizeof(/*P.RDB$ENTRYPOINT*/
									      jrd_1630.jrd_1632))
						status_exception::raise(Arg::Gds(isc_dyn_name_longer));

					/*P.RDB$ENTRYPOINT.NULL*/
					jrd_1630.jrd_1643 = (SSHORT) external->name.isEmpty();
					strcpy(/*P.RDB$ENTRYPOINT*/
					       jrd_1630.jrd_1632, external->name.c_str());
				}
			}
			else if (body)
			{
				if (secondPass)
				{
					/*P.RDB$VALID_BLR.NULL*/
					jrd_1630.jrd_1640 = FALSE;
					/*P.RDB$VALID_BLR*/
					jrd_1630.jrd_1641 = TRUE;

					/*P.RDB$PROCEDURE_BLR.NULL*/
					jrd_1630.jrd_1650 = FALSE;
					attachment->storeBinaryBlob(tdbb, transaction, &/*P.RDB$PROCEDURE_BLR*/
											jrd_1630.jrd_1635,
						dsqlScratch->getBlrData());

					/*P.RDB$DEBUG_INFO.NULL*/
					jrd_1630.jrd_1649 = FALSE;
					attachment->storeBinaryBlob(tdbb, transaction, &/*P.RDB$DEBUG_INFO*/
											jrd_1630.jrd_1634,
						dsqlScratch->getDebugData());

					/*P.RDB$PROCEDURE_TYPE.NULL*/
					jrd_1630.jrd_1647 = FALSE;
					/*P.RDB$PROCEDURE_TYPE*/
					jrd_1630.jrd_1648 = (USHORT)
						(statement->getFlags() & DsqlCompiledStatement::FLAG_SELECTABLE ?
							prc_selectable : prc_executable);
				}
			}

			modified = true;
		/*END_MODIFY*/
		jrd_1652.jrd_1653 = jrd_1630.jrd_1635;
		jrd_1652.jrd_1654 = jrd_1630.jrd_1634;
		gds__vtov((const char*) jrd_1630.jrd_1633, (char*) jrd_1652.jrd_1655, 32);
		gds__vtov((const char*) jrd_1630.jrd_1632, (char*) jrd_1652.jrd_1656, 256);
		jrd_1652.jrd_1657 = jrd_1630.jrd_1631;
		jrd_1652.jrd_1658 = jrd_1630.jrd_1650;
		jrd_1652.jrd_1659 = jrd_1630.jrd_1649;
		jrd_1652.jrd_1660 = jrd_1630.jrd_1647;
		jrd_1652.jrd_1661 = jrd_1630.jrd_1648;
		jrd_1652.jrd_1662 = jrd_1630.jrd_1646;
		jrd_1652.jrd_1663 = jrd_1630.jrd_1645;
		jrd_1652.jrd_1664 = jrd_1630.jrd_1644;
		jrd_1652.jrd_1665 = jrd_1630.jrd_1643;
		jrd_1652.jrd_1666 = jrd_1630.jrd_1642;
		jrd_1652.jrd_1667 = jrd_1630.jrd_1640;
		jrd_1652.jrd_1668 = jrd_1630.jrd_1641;
		jrd_1652.jrd_1669 = jrd_1630.jrd_1638;
		jrd_1652.jrd_1670 = jrd_1630.jrd_1639;
		EXE_send (tdbb, requestHandle, 2, 338, (UCHAR*) &jrd_1652);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1671);
	   }
	}

	if (!secondPass && modified)
	{
		// Get all comments from the old parameter list.
		MetaNameBidMap comments;
		collectParamComments(tdbb, transaction, comments);

		// Delete all old input and output parameters.
		DropProcedureNode::dropParameters(tdbb, transaction, name, package);

		// And insert the new ones.

		for (size_t i = 0; i < parameters.getCount(); ++i)
		{
			ParameterClause* parameter = parameters[i];
			bid comment;

			// Find the original comment to recreate in the new parameter.
			if (!comments.get(parameter->name, comment))
				comment.clear();

			storeParameter(tdbb, dsqlScratch, transaction, 0, i, parameter, &comment);
		}

		for (size_t i = 0; i < returns.getCount(); ++i)
		{
			ParameterClause* parameter = returns[i];
			bid comment;

			// Find the original comment to recreate in the new parameter.
			if (!comments.get(parameter->name, comment))
				comment.clear();

			storeParameter(tdbb, dsqlScratch, transaction, 1, i, parameter, &comment);
		}

		AutoCacheRequest requestHandle2(tdbb, drq_m_prm_view, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE requestHandle2 TRANSACTION_HANDLE transaction)
			PRM IN RDB$PROCEDURE_PARAMETERS CROSS
			RFR IN RDB$RELATION_FIELDS CROSS
			VRL IN RDB$VIEW_RELATIONS
			WITH PRM.RDB$PROCEDURE_NAME EQ name.c_str() AND
				 PRM.RDB$PACKAGE_NAME EQUIV NULLIF(package.c_str(), '') AND
				 VRL.RDB$RELATION_NAME EQ PRM.RDB$PROCEDURE_NAME AND
				 VRL.RDB$PACKAGE_NAME EQUIV PRM.RDB$PACKAGE_NAME AND
				 VRL.RDB$CONTEXT_TYPE EQ VCT_PROCEDURE AND
				 RFR.RDB$RELATION_NAME EQ VRL.RDB$VIEW_NAME AND
				 RFR.RDB$VIEW_CONTEXT EQ VRL.RDB$VIEW_CONTEXT AND
				 RFR.RDB$BASE_FIELD = PRM.RDB$PARAMETER_NAME*/
		{
		requestHandle2.compile(tdbb, (UCHAR*) jrd_1613, sizeof(jrd_1613));
		gds__vtov ((const char*) package.c_str(), (char*) jrd_1614.jrd_1615, 32);
		gds__vtov ((const char*) name.c_str(), (char*) jrd_1614.jrd_1616, 32);
		jrd_1614.jrd_1617 = VCT_PROCEDURE;
		EXE_start (tdbb, requestHandle2, transaction);
		EXE_send (tdbb, requestHandle2, 0, 66, (UCHAR*) &jrd_1614);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle2, 1, 66, (UCHAR*) &jrd_1618);
		   if (!jrd_1618.jrd_1621) break;
		{
			/*MODIFY RFR*/
			{
			
			{
				strcpy(/*RFR.RDB$FIELD_SOURCE*/
				       jrd_1618.jrd_1620, /*PRM.RDB$FIELD_SOURCE*/
  jrd_1618.jrd_1619);
			}
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_1618.jrd_1620, (char*) jrd_1622.jrd_1623, 32);
			EXE_send (tdbb, requestHandle2, 2, 32, (UCHAR*) &jrd_1622);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, requestHandle2, 3, 2, (UCHAR*) &jrd_1624);
		   }
		}
	}

	return modified;
}

void CreateAlterProcedureNode::storeParameter(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction, USHORT parameterType, unsigned pos, ParameterClause* parameter,
	const bid* comment)
{
   struct {
          bid  jrd_1583;	// RDB$DESCRIPTION 
          bid  jrd_1584;	// RDB$DEFAULT_SOURCE 
          bid  jrd_1585;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_1586 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_1587 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1588 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_1589 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1590 [32];	// RDB$PROCEDURE_NAME 
          TEXT  jrd_1591 [32];	// RDB$PARAMETER_NAME 
          SSHORT jrd_1592;	// gds__null_flag 
          SSHORT jrd_1593;	// gds__null_flag 
          SSHORT jrd_1594;	// gds__null_flag 
          SSHORT jrd_1595;	// gds__null_flag 
          SSHORT jrd_1596;	// RDB$COLLATION_ID 
          SSHORT jrd_1597;	// gds__null_flag 
          SSHORT jrd_1598;	// gds__null_flag 
          SSHORT jrd_1599;	// gds__null_flag 
          SSHORT jrd_1600;	// gds__null_flag 
          SSHORT jrd_1601;	// RDB$NULL_FLAG 
          SSHORT jrd_1602;	// gds__null_flag 
          SSHORT jrd_1603;	// RDB$PARAMETER_MECHANISM 
          SSHORT jrd_1604;	// gds__null_flag 
          SSHORT jrd_1605;	// RDB$PARAMETER_TYPE 
          SSHORT jrd_1606;	// gds__null_flag 
          SSHORT jrd_1607;	// RDB$PARAMETER_NUMBER 
          SSHORT jrd_1608;	// gds__null_flag 
          SSHORT jrd_1609;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_1610;	// gds__null_flag 
          SSHORT jrd_1611;	// gds__null_flag 
          SSHORT jrd_1612;	// gds__null_flag 
   } jrd_1582;
	Attachment* const attachment = transaction->getAttachment();
	TypeClause* type = parameter->type;

	AutoCacheRequest requestHandle(tdbb, drq_s_prms4, DYN_REQUESTS);

	/*STORE (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PRM IN RDB$PROCEDURE_PARAMETERS*/
	{
	
	{
		/*PRM.RDB$PARAMETER_NAME.NULL*/
		jrd_1582.jrd_1612 = FALSE;
		strcpy(/*PRM.RDB$PARAMETER_NAME*/
		       jrd_1582.jrd_1591, parameter->name.c_str());

		/*PRM.RDB$PROCEDURE_NAME.NULL*/
		jrd_1582.jrd_1611 = FALSE;
		strcpy(/*PRM.RDB$PROCEDURE_NAME*/
		       jrd_1582.jrd_1590, name.c_str());

		if (package.hasData())
		{
			/*PRM.RDB$PACKAGE_NAME.NULL*/
			jrd_1582.jrd_1610 = FALSE;
			strcpy(/*PRM.RDB$PACKAGE_NAME*/
			       jrd_1582.jrd_1589, package.c_str());
		}
		else
			/*PRM.RDB$PACKAGE_NAME.NULL*/
			jrd_1582.jrd_1610 = TRUE;

		/*PRM.RDB$SYSTEM_FLAG*/
		jrd_1582.jrd_1609 = 0;
		/*PRM.RDB$SYSTEM_FLAG.NULL*/
		jrd_1582.jrd_1608 = FALSE; // Probably superflous because the field is not nullable

		/*PRM.RDB$PARAMETER_NUMBER.NULL*/
		jrd_1582.jrd_1606 = FALSE;
		/*PRM.RDB$PARAMETER_NUMBER*/
		jrd_1582.jrd_1607 = pos;

		/*PRM.RDB$PARAMETER_TYPE.NULL*/
		jrd_1582.jrd_1604 = FALSE;
		/*PRM.RDB$PARAMETER_TYPE*/
		jrd_1582.jrd_1605 = parameterType;

		/*PRM.RDB$PARAMETER_MECHANISM.NULL*/
		jrd_1582.jrd_1602 = FALSE;
		/*PRM.RDB$PARAMETER_MECHANISM*/
		jrd_1582.jrd_1603 = (USHORT) (type->fullDomain || type->typeOfName.isEmpty() ?
			prm_mech_normal : prm_mech_type_of);

		/*PRM.RDB$NULL_FLAG.NULL*/
		jrd_1582.jrd_1600 = !type->notNull;
		/*PRM.RDB$NULL_FLAG*/
		jrd_1582.jrd_1601 = type->notNull;

		/*PRM.RDB$RELATION_NAME.NULL*/
		jrd_1582.jrd_1599 = type->typeOfTable.isEmpty();
		/*PRM.RDB$FIELD_NAME.NULL*/
		jrd_1582.jrd_1598 = /*PRM.RDB$RELATION_NAME.NULL*/
   jrd_1582.jrd_1599 || type->typeOfName.isEmpty();

		/*PRM.RDB$FIELD_SOURCE.NULL*/
		jrd_1582.jrd_1597 = FALSE;

		if (/*PRM.RDB$RELATION_NAME.NULL*/
		    jrd_1582.jrd_1599)
		{
			if (type->typeOfName.hasData())
				strcpy(/*PRM.RDB$FIELD_SOURCE*/
				       jrd_1582.jrd_1586, type->typeOfName.c_str());
			else
			{
				MetaName fieldName;
				storeGlobalField(tdbb, transaction, fieldName, type);
				strcpy(/*PRM.RDB$FIELD_SOURCE*/
				       jrd_1582.jrd_1586, fieldName.c_str());
			}
		}
		else
		{
			strcpy(/*PRM.RDB$RELATION_NAME*/
			       jrd_1582.jrd_1588, type->typeOfTable.c_str());
			strcpy(/*PRM.RDB$FIELD_NAME*/
			       jrd_1582.jrd_1587, type->typeOfName.c_str());
			strcpy(/*PRM.RDB$FIELD_SOURCE*/
			       jrd_1582.jrd_1586, type->fieldSource.c_str());
		}

		// ASF: If we used a collate with a domain or table.column type, write it
		// in RDB$PROCEDURE_PARAMETERS.

		/*PRM.RDB$COLLATION_ID.NULL*/
		jrd_1582.jrd_1595 = !(type->collate.hasData() && type->typeOfName.hasData());

		if (!/*PRM.RDB$COLLATION_ID.NULL*/
		     jrd_1582.jrd_1595)
			/*PRM.RDB$COLLATION_ID*/
			jrd_1582.jrd_1596 = type->collationId;

		// ASF: I moved this block to write defaults on RDB$PROCEDURE_PARAMETERS.
		// It was writing in RDB$FIELDS, but that would require special support
		// for packaged procedures signature verification.

		/*PRM.RDB$DEFAULT_VALUE.NULL*/
		jrd_1582.jrd_1594 = !parameter->defaultClause;
		/*PRM.RDB$DEFAULT_SOURCE.NULL*/
		jrd_1582.jrd_1593 = !parameter->defaultClause;

		if (parameter->defaultClause)
		{
			attachment->storeMetaDataBlob(tdbb, transaction, &/*PRM.RDB$DEFAULT_SOURCE*/
									  jrd_1582.jrd_1584,
				parameter->defaultClause->source);

			dsqlScratch->getBlrData().clear();

			if (dsqlScratch->isVersion4())
				dsqlScratch->appendUChar(blr_version4);
			else
				dsqlScratch->appendUChar(blr_version5);

			GEN_expr(dsqlScratch, parameter->defaultClause->value);

			dsqlScratch->appendUChar(blr_eoc);

			attachment->storeBinaryBlob(tdbb, transaction, &/*PRM.RDB$DEFAULT_VALUE*/
									jrd_1582.jrd_1585,
				dsqlScratch->getBlrData());
		}

		/*PRM.RDB$DESCRIPTION.NULL*/
		jrd_1582.jrd_1592 = !comment || comment->isEmpty();
		if (!/*PRM.RDB$DESCRIPTION.NULL*/
		     jrd_1582.jrd_1592)
			/*PRM.RDB$DESCRIPTION*/
			jrd_1582.jrd_1583 = *comment;
	}
	/*END_STORE*/
	requestHandle.compile(tdbb, (UCHAR*) jrd_1581, sizeof(jrd_1581));
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 258, (UCHAR*) &jrd_1582);
	}
}

void CreateAlterProcedureNode::compile(thread_db* /*tdbb*/, DsqlCompilerScratch* dsqlScratch)
{
	if (invalid)
		status_exception::raise(Arg::Gds(isc_dyn_invalid_ddl_proc) << name);

	if (compiled)
		return;

	compiled = true;

	if (!body)
		return;

	invalid = true;

	dsqlScratch->beginDebug();
	dsqlScratch->getBlrData().clear();

	if (dsqlScratch->isVersion4())
		dsqlScratch->appendUChar(blr_version4);
	else
		dsqlScratch->appendUChar(blr_version5);

	dsqlScratch->appendUChar(blr_begin);

	dsqlScratch->genParameters(parameters, returns);

	if (parameters.getCount() != 0)
	{
		dsqlScratch->appendUChar(blr_receive);
		dsqlScratch->appendUChar(0);
	}

	dsqlScratch->appendUChar(blr_begin);

	for (unsigned i = 0; i < parameters.getCount(); ++i)
	{
		ParameterClause* parameter = parameters[i];

		if (parameter->type->fullDomain || parameter->type->notNull)
		{
			// ASF: To validate an input parameter we need only to read its value.
			// Assigning it to null is an easy way to do this.
			dsqlScratch->appendUChar(blr_assignment);
			dsqlScratch->appendUChar(blr_parameter2);
			dsqlScratch->appendUChar(0);	// input
			dsqlScratch->appendUShort(i * 2);
			dsqlScratch->appendUShort(i * 2 + 1);
			dsqlScratch->appendUChar(blr_null);
		}
	}

	for (Array<dsql_var*>::const_iterator i = dsqlScratch->outputVariables.begin();
		 i != dsqlScratch->outputVariables.end();
		 ++i)
	{
		dsqlScratch->putLocalVariable(*i, 0, NULL);
	}

	// ASF: This is here to not change the old logic (proc_flag)
	// of previous calls to PASS1_node and PASS1_statement.
	dsqlScratch->setPsql(true);

	dsqlScratch->putLocalVariables(localDeclList, returns.getCount());

	dsqlScratch->loopLevel = 0;
	dsqlScratch->cursorNumber = 0;

	StmtNode* stmtNode = body->dsqlPass(dsqlScratch);
	GEN_hidden_variables(dsqlScratch);

	dsqlScratch->appendUChar(blr_stall);
	// put a label before body of procedure,
	// so that any EXIT statement can get out
	dsqlScratch->appendUChar(blr_label);
	dsqlScratch->appendUChar(0);

	stmtNode->genBlr(dsqlScratch);

	dsqlScratch->getStatement()->setType(DsqlCompiledStatement::TYPE_DDL);
	dsqlScratch->appendUChar(blr_end);
	dsqlScratch->genReturn(true);
	dsqlScratch->appendUChar(blr_end);
	dsqlScratch->appendUChar(blr_eoc);

	dsqlScratch->endDebug();

	invalid = false;
}

void CreateAlterProcedureNode::collectParamComments(thread_db* tdbb, jrd_tra* transaction,
	MetaNameBidMap& items)
{
   struct {
          bid  jrd_1578;	// RDB$DESCRIPTION 
          TEXT  jrd_1579 [32];	// RDB$PARAMETER_NAME 
          SSHORT jrd_1580;	// gds__utility 
   } jrd_1577;
   struct {
          TEXT  jrd_1575 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1576 [32];	// RDB$PROCEDURE_NAME 
   } jrd_1574;
	AutoRequest requestHandle;

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PRM IN RDB$PROCEDURE_PARAMETERS
		WITH PRM.RDB$PROCEDURE_NAME EQ name.c_str() AND
			 PRM.RDB$PACKAGE_NAME EQUIV NULLIF(package.c_str(), '') AND
			 PRM.RDB$DESCRIPTION NOT MISSING*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1573, sizeof(jrd_1573));
	gds__vtov ((const char*) package.c_str(), (char*) jrd_1574.jrd_1575, 32);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1574.jrd_1576, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 64, (UCHAR*) &jrd_1574);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 42, (UCHAR*) &jrd_1577);
	   if (!jrd_1577.jrd_1580) break;
	{
		items.put(/*PRM.RDB$PARAMETER_NAME*/
			  jrd_1577.jrd_1579, /*PRM.RDB$DESCRIPTION*/
  jrd_1577.jrd_1578);
	}
	/*END_FOR*/
	   }
	}
}


//----------------------


void DropProcedureNode::dropParameters(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& procedureName, const MetaName& packageName)
{
   struct {
          SSHORT jrd_1556;	// gds__utility 
   } jrd_1555;
   struct {
          SSHORT jrd_1554;	// gds__utility 
   } jrd_1553;
   struct {
          SSHORT jrd_1552;	// gds__utility 
   } jrd_1551;
   struct {
          TEXT  jrd_1549 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1550 [32];	// RDB$FIELD_SOURCE 
   } jrd_1548;
   struct {
          SSHORT jrd_1572;	// gds__utility 
   } jrd_1571;
   struct {
          SSHORT jrd_1570;	// gds__utility 
   } jrd_1569;
   struct {
          TEXT  jrd_1562 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1563 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_1564 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_1565;	// gds__utility 
          SSHORT jrd_1566;	// gds__null_flag 
          SSHORT jrd_1567;	// gds__null_flag 
          SSHORT jrd_1568;	// gds__null_flag 
   } jrd_1561;
   struct {
          TEXT  jrd_1559 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1560 [32];	// RDB$PROCEDURE_NAME 
   } jrd_1558;
	AutoCacheRequest requestHandle(tdbb, drq_e_prms2, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PRM IN RDB$PROCEDURE_PARAMETERS
		WITH PRM.RDB$PROCEDURE_NAME EQ procedureName.c_str() AND
			 PRM.RDB$PACKAGE_NAME EQUIV NULLIF(packageName.c_str(), '')*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1557, sizeof(jrd_1557));
	gds__vtov ((const char*) packageName.c_str(), (char*) jrd_1558.jrd_1559, 32);
	gds__vtov ((const char*) procedureName.c_str(), (char*) jrd_1558.jrd_1560, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 64, (UCHAR*) &jrd_1558);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 104, (UCHAR*) &jrd_1561);
	   if (!jrd_1561.jrd_1565) break;
	{
		// get rid of parameters in rdb$fields
		if (!/*PRM.RDB$FIELD_SOURCE.NULL*/
		     jrd_1561.jrd_1568 && /*PRM.RDB$RELATION_NAME.NULL*/
    jrd_1561.jrd_1567 && /*PRM.RDB$FIELD_NAME.NULL*/
    jrd_1561.jrd_1566)
		{
			AutoCacheRequest requestHandle2(tdbb, drq_e_prm_gfld, DYN_REQUESTS);

			/*FOR (REQUEST_HANDLE requestHandle2 TRANSACTION_HANDLE transaction)
				FLD IN RDB$FIELDS
				WITH FLD.RDB$FIELD_NAME EQ PRM.RDB$FIELD_SOURCE AND
					 FLD.RDB$FIELD_NAME STARTING WITH IMPLICIT_DOMAIN_PREFIX AND
					 FLD.RDB$SYSTEM_FLAG EQ 0*/
			{
			requestHandle2.compile(tdbb, (UCHAR*) jrd_1547, sizeof(jrd_1547));
			gds__vtov ((const char*) IMPLICIT_DOMAIN_PREFIX, (char*) jrd_1548.jrd_1549, 32);
			gds__vtov ((const char*) jrd_1561.jrd_1564, (char*) jrd_1548.jrd_1550, 32);
			EXE_start (tdbb, requestHandle2, transaction);
			EXE_send (tdbb, requestHandle2, 0, 64, (UCHAR*) &jrd_1548);
			while (1)
			   {
			   EXE_receive (tdbb, requestHandle2, 1, 2, (UCHAR*) &jrd_1551);
			   if (!jrd_1551.jrd_1552) break;
			{
				/*ERASE FLD;*/
				EXE_send (tdbb, requestHandle2, 2, 2, (UCHAR*) &jrd_1553);
			}
			/*END_FOR*/
			   EXE_send (tdbb, requestHandle2, 3, 2, (UCHAR*) &jrd_1555);
			   }
			}
		}

		/*ERASE PRM;*/
		EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1569);
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1571);
	   }
	}
}

void DropProcedureNode::print(string& text) const
{
	text.printf(
		"DropProcedureNode\n"
		"  name: '%s'\n",
		name.c_str());
}

DdlNode* DropProcedureNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	dsqlScratch->flags |= (DsqlCompilerScratch::FLAG_BLOCK | DsqlCompilerScratch::FLAG_PROCEDURE);
	return DdlNode::dsqlPass(dsqlScratch);
}

void DropProcedureNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1522;	// gds__utility 
   } jrd_1521;
   struct {
          SSHORT jrd_1520;	// gds__utility 
   } jrd_1519;
   struct {
          SSHORT jrd_1518;	// gds__utility 
   } jrd_1517;
   struct {
          TEXT  jrd_1515 [32];	// RDB$USER 
          SSHORT jrd_1516;	// RDB$USER_TYPE 
   } jrd_1514;
   struct {
          SSHORT jrd_1532;	// gds__utility 
   } jrd_1531;
   struct {
          SSHORT jrd_1530;	// gds__utility 
   } jrd_1529;
   struct {
          SSHORT jrd_1528;	// gds__utility 
   } jrd_1527;
   struct {
          TEXT  jrd_1525 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1526;	// RDB$OBJECT_TYPE 
   } jrd_1524;
   struct {
          SSHORT jrd_1546;	// gds__utility 
   } jrd_1545;
   struct {
          SSHORT jrd_1544;	// gds__utility 
   } jrd_1543;
   struct {
          TEXT  jrd_1538 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_1539 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_1540;	// gds__utility 
          SSHORT jrd_1541;	// gds__null_flag 
          SSHORT jrd_1542;	// RDB$SYSTEM_FLAG 
   } jrd_1537;
   struct {
          TEXT  jrd_1535 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1536 [32];	// RDB$PROCEDURE_NAME 
   } jrd_1534;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);
	bool found = false;

	dropParameters(tdbb, transaction, name, package);

	AutoCacheRequest requestHandle(tdbb, drq_e_prcs2, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PRC IN RDB$PROCEDURES
		WITH PRC.RDB$PROCEDURE_NAME EQ name.c_str() AND
			 PRC.RDB$PACKAGE_NAME EQUIV NULLIF(package.c_str(), '')*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1533, sizeof(jrd_1533));
	gds__vtov ((const char*) package.c_str(), (char*) jrd_1534.jrd_1535, 32);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1534.jrd_1536, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 64, (UCHAR*) &jrd_1534);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 70, (UCHAR*) &jrd_1537);
	   if (!jrd_1537.jrd_1540) break;
	{
		if (/*PRC.RDB$SYSTEM_FLAG*/
		    jrd_1537.jrd_1542)
			status_exception::raise(Arg::Gds(isc_dyn_cannot_mod_sysproc) << /*PRC.RDB$PROCEDURE_NAME*/
											jrd_1537.jrd_1539);

		if (package.isEmpty())
		{
			executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
				DDL_TRIGGER_DROP_PROCEDURE, name);
		}

		/*ERASE PRC;*/
		EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1543);

		if (!/*PRC.RDB$SECURITY_CLASS.NULL*/
		     jrd_1537.jrd_1541)
			deleteSecurityClass(tdbb, transaction, /*PRC.RDB$SECURITY_CLASS*/
							       jrd_1537.jrd_1538);

		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1545);
	   }
	}

	if (!found && !silent)
		status_exception::raise(Arg::Gds(isc_dyn_proc_not_found) << Arg::Str(name));

	if (package.isEmpty())
	{
		requestHandle.reset(tdbb, drq_e_prc_prvs, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
			PRIV IN RDB$USER_PRIVILEGES WITH PRIV.RDB$RELATION_NAME EQ name.c_str()
				AND PRIV.RDB$OBJECT_TYPE = obj_procedure*/
		{
		requestHandle.compile(tdbb, (UCHAR*) jrd_1523, sizeof(jrd_1523));
		gds__vtov ((const char*) name.c_str(), (char*) jrd_1524.jrd_1525, 32);
		jrd_1524.jrd_1526 = obj_procedure;
		EXE_start (tdbb, requestHandle, transaction);
		EXE_send (tdbb, requestHandle, 0, 34, (UCHAR*) &jrd_1524);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle, 1, 2, (UCHAR*) &jrd_1527);
		   if (!jrd_1527.jrd_1528) break;
		{
			/*ERASE PRIV;*/
			EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1529);
		}
		/*END_FOR*/
		   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1531);
		   }
		}

		requestHandle.reset(tdbb, drq_e_prc_prv, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
			PRIV IN RDB$USER_PRIVILEGES WITH PRIV.RDB$USER EQ name.c_str()
				AND PRIV.RDB$USER_TYPE = obj_procedure*/
		{
		requestHandle.compile(tdbb, (UCHAR*) jrd_1513, sizeof(jrd_1513));
		gds__vtov ((const char*) name.c_str(), (char*) jrd_1514.jrd_1515, 32);
		jrd_1514.jrd_1516 = obj_procedure;
		EXE_start (tdbb, requestHandle, transaction);
		EXE_send (tdbb, requestHandle, 0, 34, (UCHAR*) &jrd_1514);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle, 1, 2, (UCHAR*) &jrd_1517);
		   if (!jrd_1517.jrd_1518) break;
		{
			/*ERASE PRIV;*/
			EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1519);
		}
		/*END_FOR*/
		   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1521);
		   }
		}
	}

	if (found && package.isEmpty())
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_PROCEDURE, name);

	savePoint.release();	// everything is ok

	// Update DSQL cache
	METD_drop_procedure(transaction, QualifiedName(name, package));
	MET_dsql_cache_release(tdbb, SYM_procedure, name, package);
}


//----------------------


void TriggerDefinition::store(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
   struct {
          ISC_INT64  jrd_1505;	// RDB$TRIGGER_TYPE 
          TEXT  jrd_1506 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_1507 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_1508;	// RDB$TRIGGER_INACTIVE 
          SSHORT jrd_1509;	// RDB$TRIGGER_SEQUENCE 
          SSHORT jrd_1510;	// gds__null_flag 
          SSHORT jrd_1511;	// RDB$FLAGS 
          SSHORT jrd_1512;	// RDB$SYSTEM_FLAG 
   } jrd_1504;
	if (name.isEmpty())
		DYN_UTIL_generate_trigger_name(tdbb, transaction, name);

	AutoCacheRequest requestHandle(tdbb, drq_s_triggers, DYN_REQUESTS);

	/*STORE (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		TRG IN RDB$TRIGGERS*/
	{
	
	{
		/*TRG.RDB$SYSTEM_FLAG*/
		jrd_1504.jrd_1512 = SSHORT(systemFlag);
		/*TRG.RDB$FLAGS*/
		jrd_1504.jrd_1511 = TRG_sql | (fkTrigger ? TRG_ignore_perm : 0);
		strcpy(/*TRG.RDB$TRIGGER_NAME*/
		       jrd_1504.jrd_1507, name.c_str());

		/*TRG.RDB$RELATION_NAME.NULL*/
		jrd_1504.jrd_1510 = relationName.isEmpty();
		strcpy(/*TRG.RDB$RELATION_NAME*/
		       jrd_1504.jrd_1506, relationName.c_str());

		fb_assert(type.specified);
		/*TRG.RDB$TRIGGER_TYPE*/
		jrd_1504.jrd_1505 = type.value;

		/*TRG.RDB$TRIGGER_SEQUENCE*/
		jrd_1504.jrd_1509 = (!position.specified ? 0 : position.value);
		/*TRG.RDB$TRIGGER_INACTIVE*/
		jrd_1504.jrd_1508 = (!active.specified ? 0 : (USHORT) !active.value);
	}
	/*END_STORE*/
	requestHandle.compile(tdbb, (UCHAR*) jrd_1503, sizeof(jrd_1503));
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 82, (UCHAR*) &jrd_1504);
	}

	modify(tdbb, dsqlScratch, transaction);
}

bool TriggerDefinition::modify(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1502;	// gds__utility 
   } jrd_1501;
   struct {
          ISC_INT64  jrd_1486;	// RDB$TRIGGER_TYPE 
          TEXT  jrd_1487 [32];	// RDB$ENGINE_NAME 
          TEXT  jrd_1488 [256];	// RDB$ENTRYPOINT 
          bid  jrd_1489;	// RDB$TRIGGER_SOURCE 
          bid  jrd_1490;	// RDB$TRIGGER_BLR 
          bid  jrd_1491;	// RDB$DEBUG_INFO 
          SSHORT jrd_1492;	// gds__null_flag 
          SSHORT jrd_1493;	// gds__null_flag 
          SSHORT jrd_1494;	// gds__null_flag 
          SSHORT jrd_1495;	// gds__null_flag 
          SSHORT jrd_1496;	// gds__null_flag 
          SSHORT jrd_1497;	// gds__null_flag 
          SSHORT jrd_1498;	// RDB$VALID_BLR 
          SSHORT jrd_1499;	// RDB$TRIGGER_SEQUENCE 
          SSHORT jrd_1500;	// RDB$TRIGGER_INACTIVE 
   } jrd_1485;
   struct {
          bid  jrd_1465;	// RDB$DEBUG_INFO 
          bid  jrd_1466;	// RDB$TRIGGER_BLR 
          bid  jrd_1467;	// RDB$TRIGGER_SOURCE 
          TEXT  jrd_1468 [256];	// RDB$ENTRYPOINT 
          TEXT  jrd_1469 [32];	// RDB$ENGINE_NAME 
          TEXT  jrd_1470 [32];	// RDB$TRIGGER_NAME 
          TEXT  jrd_1471 [32];	// RDB$RELATION_NAME 
          ISC_INT64  jrd_1472;	// RDB$TRIGGER_TYPE 
          SSHORT jrd_1473;	// gds__utility 
          SSHORT jrd_1474;	// RDB$TRIGGER_INACTIVE 
          SSHORT jrd_1475;	// RDB$TRIGGER_SEQUENCE 
          SSHORT jrd_1476;	// gds__null_flag 
          SSHORT jrd_1477;	// RDB$VALID_BLR 
          SSHORT jrd_1478;	// gds__null_flag 
          SSHORT jrd_1479;	// gds__null_flag 
          SSHORT jrd_1480;	// gds__null_flag 
          SSHORT jrd_1481;	// gds__null_flag 
          SSHORT jrd_1482;	// gds__null_flag 
          SSHORT jrd_1483;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_1484;	// gds__null_flag 
   } jrd_1464;
   struct {
          TEXT  jrd_1463 [32];	// RDB$TRIGGER_NAME 
   } jrd_1462;
	Attachment* const attachment = transaction->getAttachment();
	bool modified = false;

	// ASF: Unregistered bug (2.0, 2.1, 2.5, 3.0): CREATE OR ALTER TRIGGER accepts different table
	// than one used in already created trigger.

	AutoCacheRequest requestHandle(tdbb, drq_m_trigger2, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$TRIGGER_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1461, sizeof(jrd_1461));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1462.jrd_1463, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_1462);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 408, (UCHAR*) &jrd_1464);
	   if (!jrd_1464.jrd_1473) break;
	{
		if (type.specified && type.value != (FB_UINT64) /*TRG.RDB$TRIGGER_TYPE*/
								jrd_1464.jrd_1472 &&
			/*TRG.RDB$RELATION_NAME.NULL*/
			jrd_1464.jrd_1484)
		{
			status_exception::raise(
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_dsql_db_trigger_type_cant_change));
		}

		if (systemFlag == fb_sysflag_user)
		{
			switch (/*TRG.RDB$SYSTEM_FLAG*/
				jrd_1464.jrd_1483)
			{
				case fb_sysflag_check_constraint:
				case fb_sysflag_referential_constraint:
				case fb_sysflag_view_check:
					status_exception::raise(Arg::Gds(isc_dyn_cant_modify_auto_trig));
					break;

				case fb_sysflag_system:
					status_exception::raise(
						Arg::Gds(isc_dyn_cannot_mod_systrig) << /*TRG.RDB$TRIGGER_NAME*/
											jrd_1464.jrd_1470);
					break;

				default:
					break;
			}
		}

		preModify(tdbb, dsqlScratch, transaction);

		/*MODIFY TRG*/
		{
		
			if (blrData.length > 0 || external)
			{
				fb_assert(!(blrData.length > 0 && external));

				/*TRG.RDB$ENGINE_NAME.NULL*/
				jrd_1464.jrd_1482 = TRUE;
				/*TRG.RDB$ENTRYPOINT.NULL*/
				jrd_1464.jrd_1481 = TRUE;
				/*TRG.RDB$TRIGGER_SOURCE.NULL*/
				jrd_1464.jrd_1480 = TRUE;
				/*TRG.RDB$TRIGGER_BLR.NULL*/
				jrd_1464.jrd_1479 = TRUE;
				/*TRG.RDB$DEBUG_INFO.NULL*/
				jrd_1464.jrd_1478 = TRUE;
				/*TRG.RDB$VALID_BLR.NULL*/
				jrd_1464.jrd_1476 = TRUE;
			}

			if (type.specified)
				/*TRG.RDB$TRIGGER_TYPE*/
				jrd_1464.jrd_1472 = type.value;

			if (position.specified)
				/*TRG.RDB$TRIGGER_SEQUENCE*/
				jrd_1464.jrd_1475 = position.value;
			if (active.specified)
				/*TRG.RDB$TRIGGER_INACTIVE*/
				jrd_1464.jrd_1474 = (USHORT) !active.value;

			if (external)
			{
				/*TRG.RDB$ENGINE_NAME.NULL*/
				jrd_1464.jrd_1482 = FALSE;
				strcpy(/*TRG.RDB$ENGINE_NAME*/
				       jrd_1464.jrd_1469, external->engine.c_str());

				if (external->name.length() >= sizeof(/*TRG.RDB$ENTRYPOINT*/
								      jrd_1464.jrd_1468))
					status_exception::raise(Arg::Gds(isc_dyn_name_longer));

				/*TRG.RDB$ENTRYPOINT.NULL*/
				jrd_1464.jrd_1481 = (SSHORT) external->name.isEmpty();
				strcpy(/*TRG.RDB$ENTRYPOINT*/
				       jrd_1464.jrd_1468, external->name.c_str());
			}
			else if (blrData.length > 0)
			{
				/*TRG.RDB$VALID_BLR.NULL*/
				jrd_1464.jrd_1476 = FALSE;
				/*TRG.RDB$VALID_BLR*/
				jrd_1464.jrd_1477 = TRUE;

				/*TRG.RDB$TRIGGER_BLR.NULL*/
				jrd_1464.jrd_1479 = FALSE;
				attachment->storeBinaryBlob(tdbb, transaction, &/*TRG.RDB$TRIGGER_BLR*/
										jrd_1464.jrd_1466, blrData);
			}

			if (debugData.length > 0)
			{
				/*TRG.RDB$DEBUG_INFO.NULL*/
				jrd_1464.jrd_1478 = FALSE;
				attachment->storeBinaryBlob(tdbb, transaction, &/*TRG.RDB$DEBUG_INFO*/
										jrd_1464.jrd_1465, debugData);
			}

			if (source.hasData())
			{
				/*TRG.RDB$TRIGGER_SOURCE.NULL*/
				jrd_1464.jrd_1480 = FALSE;
				attachment->storeMetaDataBlob(tdbb, transaction, &/*TRG.RDB$TRIGGER_SOURCE*/
										  jrd_1464.jrd_1467, source);
			}

			modified = true;
		/*END_MODIFY*/
		jrd_1485.jrd_1486 = jrd_1464.jrd_1472;
		gds__vtov((const char*) jrd_1464.jrd_1469, (char*) jrd_1485.jrd_1487, 32);
		gds__vtov((const char*) jrd_1464.jrd_1468, (char*) jrd_1485.jrd_1488, 256);
		jrd_1485.jrd_1489 = jrd_1464.jrd_1467;
		jrd_1485.jrd_1490 = jrd_1464.jrd_1466;
		jrd_1485.jrd_1491 = jrd_1464.jrd_1465;
		jrd_1485.jrd_1492 = jrd_1464.jrd_1482;
		jrd_1485.jrd_1493 = jrd_1464.jrd_1481;
		jrd_1485.jrd_1494 = jrd_1464.jrd_1480;
		jrd_1485.jrd_1495 = jrd_1464.jrd_1479;
		jrd_1485.jrd_1496 = jrd_1464.jrd_1478;
		jrd_1485.jrd_1497 = jrd_1464.jrd_1476;
		jrd_1485.jrd_1498 = jrd_1464.jrd_1477;
		jrd_1485.jrd_1499 = jrd_1464.jrd_1475;
		jrd_1485.jrd_1500 = jrd_1464.jrd_1474;
		EXE_send (tdbb, requestHandle, 2, 338, (UCHAR*) &jrd_1485);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1501);
	   }
	}

	if (modified)
		postModify(tdbb, dsqlScratch, transaction);

	return modified;
}


//----------------------


void CreateAlterTriggerNode::print(string& text) const
{
	text.printf(
		"CreateAlterTriggerNode\n"
		"  name: '%s'  create: %d  alter: %d  relationName: '%s'\n"
		"  type: %d, %d  active: %d, %d  position: %d, %d\n",
		name.c_str(), create, alter, relationName.c_str(), type.specified, type.value,
		active.specified, active.value, position.specified, position.value);

	if (external)
	{
		string s;
		s.printf("  external -> name: '%s'  engine: '%s'\n",
			external->name.c_str(), external->engine.c_str());
		text += s;
	}
}

DdlNode* CreateAlterTriggerNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	dsqlScratch->flags |= (DsqlCompilerScratch::FLAG_BLOCK | DsqlCompilerScratch::FLAG_TRIGGER);

	if (type.specified)
	{
		if (create &&	// ALTER TRIGGER doesn't accept table name
			((relationName.hasData() &&
				(type.value & (unsigned) TRIGGER_TYPE_MASK) != (unsigned) TRIGGER_TYPE_DML) ||
			 (relationName.isEmpty() &&
				(type.value & (unsigned) TRIGGER_TYPE_MASK) != (unsigned) TRIGGER_TYPE_DB &&
				(type.value & (unsigned) TRIGGER_TYPE_MASK) != (unsigned) TRIGGER_TYPE_DDL)))
		{
			status_exception::raise(
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_dsql_incompatible_trigger_type));
		}
	}

	return DdlNode::dsqlPass(dsqlScratch);
}

void CreateAlterTriggerNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          TEXT  jrd_1456 [32];	// RDB$RELATION_NAME 
          ISC_INT64  jrd_1457;	// RDB$TRIGGER_TYPE 
          SSHORT jrd_1458;	// gds__utility 
          SSHORT jrd_1459;	// gds__null_flag 
          SSHORT jrd_1460;	// gds__null_flag 
   } jrd_1455;
   struct {
          TEXT  jrd_1454 [32];	// RDB$TRIGGER_NAME 
   } jrd_1453;
	fb_assert(create || alter);

	Attachment* const attachment = transaction->getAttachment();

	if (relationName.isEmpty() && !attachment->locksmith())
		status_exception::raise(Arg::Gds(isc_adm_task_denied));

	source.ltrim("\n\r\t ");

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	if (!create)
	{
		AutoRequest requestHandle;

		/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
			TRG IN RDB$TRIGGERS
			WITH TRG.RDB$TRIGGER_NAME EQ name.c_str()*/
		{
		requestHandle.compile(tdbb, (UCHAR*) jrd_1452, sizeof(jrd_1452));
		gds__vtov ((const char*) name.c_str(), (char*) jrd_1453.jrd_1454, 32);
		EXE_start (tdbb, requestHandle, transaction);
		EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_1453);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle, 1, 46, (UCHAR*) &jrd_1455);
		   if (!jrd_1455.jrd_1458) break;
		{
			if (!type.specified && !/*TRG.RDB$TRIGGER_TYPE.NULL*/
						jrd_1455.jrd_1460)
				type = /*TRG.RDB$TRIGGER_TYPE*/
				       jrd_1455.jrd_1457;

			if (relationName.isEmpty() && !/*TRG.RDB$RELATION_NAME.NULL*/
						       jrd_1455.jrd_1459)
				relationName = /*TRG.RDB$RELATION_NAME*/
					       jrd_1455.jrd_1456;
		}
		/*END_FOR*/
		   }
		}

		if (!type.specified)
			status_exception::raise(Arg::Gds(isc_dyn_trig_not_found) << Arg::Str(name));
	}

	compile(tdbb, dsqlScratch);

	blrData = dsqlScratch->getBlrData();
	debugData = dsqlScratch->getDebugData();

	if (alter)
	{
		if (!modify(tdbb, dsqlScratch, transaction))
		{
			if (create)	// create or alter
				executeCreate(tdbb, dsqlScratch, transaction);
			else
				status_exception::raise(Arg::Gds(isc_dyn_trig_not_found) << Arg::Str(name));
		}
	}
	else
		executeCreate(tdbb, dsqlScratch, transaction);

	savePoint.release();	// everything is ok
}

void CreateAlterTriggerNode::executeCreate(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_CREATE_TRIGGER, name);
	store(tdbb, dsqlScratch, transaction);
	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_CREATE_TRIGGER, name);
}

void CreateAlterTriggerNode::compile(thread_db* /*tdbb*/, DsqlCompilerScratch* dsqlScratch)
{
	if (invalid)
		status_exception::raise(Arg::Gds(isc_dyn_invalid_ddl_trig) << name);

	if (compiled)
		return;

	compiled = true;
	invalid = true;

	if (body)
	{
		dsqlScratch->beginDebug();
		dsqlScratch->getBlrData().clear();

		// Create the "OLD" and "NEW" contexts for the trigger --
		// the new one could be a dummy place holder to avoid resolving
		// fields to that context but prevent relations referenced in
		// the trigger actions from referencing the predefined "1" context.
		if (dsqlScratch->contextNumber)
			dsqlScratch->resetTriggerContextStack();

		if (relationName.hasData())
		{
			RelationSourceNode* relationNode = FB_NEW(getPool()) RelationSourceNode(getPool(),
				relationName);

			const string temp = relationNode->alias;	// always empty?

			if (hasOldContext(type.value))
			{
				relationNode->alias = OLD_CONTEXT_NAME;
				dsql_ctx* oldContext = PASS1_make_context(dsqlScratch, relationNode);
				oldContext->ctx_flags |= CTX_system;
			}
			else
				dsqlScratch->contextNumber++;

			if (hasNewContext(type.value))
			{
				relationNode->alias = NEW_CONTEXT_NAME;
				dsql_ctx* newContext = PASS1_make_context(dsqlScratch, relationNode);
				newContext->ctx_flags |= CTX_system;
			}
			else
				dsqlScratch->contextNumber++;

			relationNode->alias = temp;
		}

		// generate the trigger blr

		if (dsqlScratch->isVersion4())
			dsqlScratch->appendUChar(blr_version4);
		else
			dsqlScratch->appendUChar(blr_version5);

		dsqlScratch->appendUChar(blr_begin);

		dsqlScratch->setPsql(true);
		dsqlScratch->putLocalVariables(localDeclList, 0);

		dsqlScratch->scopeLevel++;
		// dimitr: I see no reason to deny EXIT command in triggers,
		// hence I've added zero label at the beginning.
		// My first suspicion regarding an obvious conflict
		// with trigger messages (nod_abort) is wrong,
		// although the fact that they use the same BLR code
		// is still a potential danger and must be fixed.
		// Hopefully, system triggers are never recompiled.
		dsqlScratch->appendUChar(blr_label);
		dsqlScratch->appendUChar(0);
		dsqlScratch->loopLevel = 0;
		dsqlScratch->cursorNumber = 0;
		body->dsqlPass(dsqlScratch)->genBlr(dsqlScratch);
		dsqlScratch->scopeLevel--;
		dsqlScratch->appendUChar(blr_end);
		dsqlScratch->appendUChar(blr_eoc);

		dsqlScratch->endDebug();

		// The statement type may have been set incorrectly when parsing
		// the trigger actions, so reset it to reflect the fact that this
		// is a data definition statement; also reset the ddl node.
		dsqlScratch->getStatement()->setType(DsqlCompiledStatement::TYPE_DDL);
	}

	invalid = false;
}


//----------------------


void DropTriggerNode::print(string& text) const
{
	text.printf(
		"DropTriggerNode\n"
		"  name: '%s'\n",
		name.c_str());
}

DdlNode* DropTriggerNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	dsqlScratch->flags |= (DsqlCompilerScratch::FLAG_BLOCK | DsqlCompilerScratch::FLAG_TRIGGER);
	return DdlNode::dsqlPass(dsqlScratch);
}

void DropTriggerNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1414;	// gds__utility 
   } jrd_1413;
   struct {
          SSHORT jrd_1412;	// RDB$UPDATE_FLAG 
   } jrd_1411;
   struct {
          SSHORT jrd_1409;	// gds__utility 
          SSHORT jrd_1410;	// RDB$UPDATE_FLAG 
   } jrd_1408;
   struct {
          TEXT  jrd_1407 [32];	// RDB$RELATION_NAME 
   } jrd_1406;
   struct {
          SSHORT jrd_1419;	// gds__utility 
   } jrd_1418;
   struct {
          TEXT  jrd_1417 [32];	// RDB$VIEW_NAME 
   } jrd_1416;
   struct {
          SSHORT jrd_1429;	// gds__utility 
   } jrd_1428;
   struct {
          SSHORT jrd_1427;	// gds__utility 
   } jrd_1426;
   struct {
          SSHORT jrd_1425;	// gds__utility 
   } jrd_1424;
   struct {
          TEXT  jrd_1422 [32];	// RDB$USER 
          SSHORT jrd_1423;	// RDB$USER_TYPE 
   } jrd_1421;
   struct {
          SSHORT jrd_1438;	// gds__utility 
   } jrd_1437;
   struct {
          SSHORT jrd_1436;	// gds__utility 
   } jrd_1435;
   struct {
          SSHORT jrd_1434;	// gds__utility 
   } jrd_1433;
   struct {
          TEXT  jrd_1432 [32];	// RDB$TRIGGER_NAME 
   } jrd_1431;
   struct {
          SSHORT jrd_1451;	// gds__utility 
   } jrd_1450;
   struct {
          SSHORT jrd_1449;	// gds__utility 
   } jrd_1448;
   struct {
          TEXT  jrd_1443 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_1444 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_1445;	// gds__utility 
          SSHORT jrd_1446;	// gds__null_flag 
          SSHORT jrd_1447;	// RDB$SYSTEM_FLAG 
   } jrd_1442;
   struct {
          TEXT  jrd_1441 [32];	// RDB$TRIGGER_NAME 
   } jrd_1440;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	bool found = false;
	MetaName relationName;
	AutoCacheRequest requestHandle(tdbb, drq_e_trigger3, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		X IN RDB$TRIGGERS
		WITH X.RDB$TRIGGER_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1439, sizeof(jrd_1439));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1440.jrd_1441, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_1440);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 70, (UCHAR*) &jrd_1442);
	   if (!jrd_1442.jrd_1445) break;
	{
		switch (/*X.RDB$SYSTEM_FLAG*/
			jrd_1442.jrd_1447)
		{
			case fb_sysflag_check_constraint:
			case fb_sysflag_referential_constraint:
			case fb_sysflag_view_check:
				status_exception::raise(Arg::Gds(isc_dyn_cant_modify_auto_trig));
				break;

			case fb_sysflag_system:
				status_exception::raise(
					Arg::Gds(isc_dyn_cannot_mod_systrig) << /*X.RDB$TRIGGER_NAME*/
										jrd_1442.jrd_1444);
				break;

			default:
				break;
		}

		if (/*X.RDB$RELATION_NAME.NULL*/
		    jrd_1442.jrd_1446 && !transaction->getAttachment()->locksmith())
			status_exception::raise(Arg::Gds(isc_adm_task_denied));

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_DROP_TRIGGER, name);

		relationName = /*X.RDB$RELATION_NAME*/
			       jrd_1442.jrd_1443;
		/*ERASE X;*/
		EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1448);
		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1450);
	   }
	}

	if (!found && !silent)
		status_exception::raise(Arg::Gds(isc_dyn_trig_not_found) << Arg::Str(name));

	requestHandle.reset(tdbb, drq_e_trg_msgs3, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		TM IN RDB$TRIGGER_MESSAGES
		WITH TM.RDB$TRIGGER_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1430, sizeof(jrd_1430));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1431.jrd_1432, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_1431);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 2, (UCHAR*) &jrd_1433);
	   if (!jrd_1433.jrd_1434) break;
	{
		/*ERASE TM;*/
		EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1435);
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1437);
	   }
	}

	requestHandle.reset(tdbb, drq_e_trg_prv2, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH PRIV.RDB$USER EQ name.c_str() AND
			 PRIV.RDB$USER_TYPE = obj_trigger*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1420, sizeof(jrd_1420));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1421.jrd_1422, 32);
	jrd_1421.jrd_1423 = obj_trigger;
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 34, (UCHAR*) &jrd_1421);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 2, (UCHAR*) &jrd_1424);
	   if (!jrd_1424.jrd_1425) break;
	{
		/*ERASE PRIV;*/
		EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1426);
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1428);
	   }
	}

	// Clear the update flags on the fields if this is the last remaining
	// trigger that changes a view.

	bool viewFound = false;
	requestHandle.reset(tdbb, drq_e_trg_prv3, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		FIRST 1 V IN RDB$VIEW_RELATIONS
		CROSS F IN RDB$RELATION_FIELDS
		CROSS T IN RDB$TRIGGERS
		WITH V.RDB$VIEW_NAME EQ relationName.c_str() AND
			 F.RDB$RELATION_NAME EQ V.RDB$VIEW_NAME AND
			 F.RDB$RELATION_NAME EQ T.RDB$RELATION_NAME*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_1415, sizeof(jrd_1415));
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_1416.jrd_1417, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_1416);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 2, (UCHAR*) &jrd_1418);
	   if (!jrd_1418.jrd_1419) break;
	{
		viewFound = true;
	}
	/*END_FOR*/
	   }
	}

	if (!viewFound)
	{
		requestHandle.reset(tdbb, drq_m_rel_flds2, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
			F IN RDB$RELATION_FIELDS
			WITH F.RDB$RELATION_NAME EQ relationName.c_str()*/
		{
		requestHandle.compile(tdbb, (UCHAR*) jrd_1405, sizeof(jrd_1405));
		gds__vtov ((const char*) relationName.c_str(), (char*) jrd_1406.jrd_1407, 32);
		EXE_start (tdbb, requestHandle, transaction);
		EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_1406);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle, 1, 4, (UCHAR*) &jrd_1408);
		   if (!jrd_1408.jrd_1409) break;
		{
			/*MODIFY F USING*/
			{
			
				/*F.RDB$UPDATE_FLAG*/
				jrd_1408.jrd_1410 = FALSE;
			/*END_MODIFY*/
			jrd_1411.jrd_1412 = jrd_1408.jrd_1410;
			EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_1411);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_1413);
		   }
		}
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_TRIGGER, name);

	savePoint.release();	// everything is ok
}


//----------------------


void CreateCollationNode::print(string& text) const
{
	text.printf(
		"CreateCollationNode\n"
		"  name: '%s'\n"
		"  forCharSet: '%s'\n"
		"  fromName: '%s'\n"
		"  fromExternal: '%s'\n"
		"  attributesOn: %x\n"
		"  attributesOff: %x\n",
		name.c_str(), forCharSet.c_str(), fromName.c_str(), fromExternal.c_str(),
		attributesOn, attributesOff);
}

void CreateCollationNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1389;	// gds__utility 
          SSHORT jrd_1390;	// RDB$COLLATION_ID 
   } jrd_1388;
   struct {
          SSHORT jrd_1387;	// RDB$CHARACTER_SET_ID 
   } jrd_1386;
   struct {
          TEXT  jrd_1393 [32];	// RDB$BASE_COLLATION_NAME 
          bid  jrd_1394;	// RDB$SPECIFIC_ATTRIBUTES 
          TEXT  jrd_1395 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_1396 [32];	// RDB$COLLATION_NAME 
          SSHORT jrd_1397;	// gds__null_flag 
          SSHORT jrd_1398;	// RDB$COLLATION_ID 
          SSHORT jrd_1399;	// RDB$COLLATION_ATTRIBUTES 
          SSHORT jrd_1400;	// gds__null_flag 
          SSHORT jrd_1401;	// gds__null_flag 
          SSHORT jrd_1402;	// gds__null_flag 
          SSHORT jrd_1403;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_1404;	// RDB$CHARACTER_SET_ID 
   } jrd_1392;
	Attachment* const attachment = transaction->tra_attachment;
	const string& userName = attachment->att_user->usr_user_name;

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
		DDL_TRIGGER_CREATE_COLLATION, name);

	AutoCacheRequest request(tdbb, drq_s_colls, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$COLLATIONS*/
	{
	
	{
		/*X.RDB$CHARACTER_SET_ID*/
		jrd_1392.jrd_1404 = forCharSetId;
		strcpy(/*X.RDB$COLLATION_NAME*/
		       jrd_1392.jrd_1396, name.c_str());
		/*X.RDB$SYSTEM_FLAG*/
		jrd_1392.jrd_1403 = 0;

		/*X.RDB$OWNER_NAME.NULL*/
		jrd_1392.jrd_1402 = FALSE;
		strcpy(/*X.RDB$OWNER_NAME*/
		       jrd_1392.jrd_1395, userName.c_str());

		/*X.RDB$SPECIFIC_ATTRIBUTES.NULL*/
		jrd_1392.jrd_1401 = TRUE;
		/*X.RDB$BASE_COLLATION_NAME.NULL*/
		jrd_1392.jrd_1400 = TRUE;

		CharSet* cs = INTL_charset_lookup(tdbb, forCharSetId);
		SubtypeInfo info;

		if (fromName.hasData())
		{
			if (MET_get_char_coll_subtype_info(tdbb,
					INTL_CS_COLL_TO_TTYPE(forCharSetId, fromCollationId), &info) &&
				info.specificAttributes.hasData())
			{
				UCharBuffer temp;
				ULONG size = info.specificAttributes.getCount() * cs->maxBytesPerChar();

				size = INTL_convert_bytes(tdbb, forCharSetId, temp.getBuffer(size), size,
					CS_METADATA, info.specificAttributes.begin(),
					info.specificAttributes.getCount(), status_exception::raise);
				temp.shrink(size);
				info.specificAttributes = temp;
			}

			strcpy(/*X.RDB$BASE_COLLATION_NAME*/
			       jrd_1392.jrd_1393, info.baseCollationName.c_str());
			/*X.RDB$BASE_COLLATION_NAME.NULL*/
			jrd_1392.jrd_1400 = FALSE;
		}
		else if (fromExternal.hasData())
		{
			strcpy(/*X.RDB$BASE_COLLATION_NAME*/
			       jrd_1392.jrd_1393, fromExternal.c_str());
			/*X.RDB$BASE_COLLATION_NAME.NULL*/
			jrd_1392.jrd_1400 = FALSE;
		}

		if (specificAttributes.hasData() && forCharSetId != attachment->att_charset)
		{
			UCharBuffer temp;
			ULONG size = specificAttributes.getCount() * cs->maxBytesPerChar();

			size = INTL_convert_bytes(tdbb, forCharSetId, temp.getBuffer(size), size,
				attachment->att_charset, specificAttributes.begin(),
				specificAttributes.getCount(), status_exception::raise);
			temp.shrink(size);
			specificAttributes = temp;
		}

		info.charsetName = forCharSet.c_str();
		info.collationName = name;
		if (/*X.RDB$BASE_COLLATION_NAME.NULL*/
		    jrd_1392.jrd_1400)
			info.baseCollationName = info.collationName;
		else
			info.baseCollationName = /*X.RDB$BASE_COLLATION_NAME*/
						 jrd_1392.jrd_1393;
		info.ignoreAttributes = false;

		if (!IntlManager::collationInstalled(info.baseCollationName.c_str(),
				info.charsetName.c_str()))
		{
			// msg: 223: "Collation @1 not installed for character set @2"
			status_exception::raise(
				Arg::PrivateDyn(223) << info.baseCollationName << info.charsetName);
		}

		IntlUtil::SpecificAttributesMap map;

		if (!IntlUtil::parseSpecificAttributes(
				cs, info.specificAttributes.getCount(), info.specificAttributes.begin(), &map) ||
			!IntlUtil::parseSpecificAttributes(
				cs, specificAttributes.getCount(), specificAttributes.begin(), &map))
		{
			// msg: 222: "Invalid collation attributes"
			status_exception::raise(Arg::PrivateDyn(222));
		}

		const string s = IntlUtil::generateSpecificAttributes(cs, map);
		string newSpecificAttributes;

		if (!IntlManager::setupCollationAttributes(
				info.baseCollationName.c_str(), info.charsetName.c_str(), s,
				newSpecificAttributes))
		{
			// msg: 222: "Invalid collation attributes"
			status_exception::raise(Arg::PrivateDyn(222));
		}

		memcpy(info.specificAttributes.getBuffer(newSpecificAttributes.length()),
			newSpecificAttributes.begin(), newSpecificAttributes.length());

		if (info.specificAttributes.hasData())
		{
			/*X.RDB$SPECIFIC_ATTRIBUTES.NULL*/
			jrd_1392.jrd_1401 = FALSE;
			attachment->storeMetaDataBlob(tdbb, transaction, &/*X.RDB$SPECIFIC_ATTRIBUTES*/
									  jrd_1392.jrd_1394,
				string(info.specificAttributes.begin(), info.specificAttributes.getCount()),
				forCharSetId);
		}

		info.attributes = (info.attributes | attributesOn) & (~attributesOff);
		/*X.RDB$COLLATION_ATTRIBUTES*/
		jrd_1392.jrd_1399 = info.attributes;

		// Do not allow invalid attributes here.
		if (!INTL_texttype_validate(tdbb, &info))
		{
			// msg: 222: "Invalid collation attributes"
			status_exception::raise(Arg::PrivateDyn(222));
		}

		// ASF: User collations are created with the last number available,
		// to minimize the possibility of conflicts with future system collations.
		// The greater available number is 126 to avoid signed/unsigned problems.

		/*X.RDB$COLLATION_ID.NULL*/
		jrd_1392.jrd_1397 = TRUE;
		/*X.RDB$COLLATION_ID*/
		jrd_1392.jrd_1398 = 126;

		AutoCacheRequest request2(tdbb, drq_l_max_coll_id, DYN_REQUESTS);

		/*FOR(REQUEST_HANDLE request2)
			Y IN RDB$COLLATIONS
			WITH Y.RDB$CHARACTER_SET_ID = forCharSetId AND
					Y.RDB$COLLATION_ID NOT MISSING
			SORTED BY DESCENDING Y.RDB$COLLATION_ID*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_1385, sizeof(jrd_1385));
		jrd_1386.jrd_1387 = forCharSetId;
		EXE_start (tdbb, request2, attachment->getSysTransaction());
		EXE_send (tdbb, request2, 0, 2, (UCHAR*) &jrd_1386);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 4, (UCHAR*) &jrd_1388);
		   if (!jrd_1388.jrd_1389) break;
		{
			if (/*Y.RDB$COLLATION_ID*/
			    jrd_1388.jrd_1390 + 1 <= /*X.RDB$COLLATION_ID*/
	jrd_1392.jrd_1398)
			{
				/*X.RDB$COLLATION_ID.NULL*/
				jrd_1392.jrd_1397 = FALSE;
				break;
			}
			else
				/*X.RDB$COLLATION_ID*/
				jrd_1392.jrd_1398 = /*Y.RDB$COLLATION_ID*/
   jrd_1388.jrd_1390 - 1;
		}
		/*END_FOR*/
		   }
		}

		if (/*X.RDB$COLLATION_ID.NULL*/
		    jrd_1392.jrd_1397)
			status_exception::raise(Arg::Gds(isc_max_coll_per_charset));
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_1391, sizeof(jrd_1391));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 120, (UCHAR*) &jrd_1392);
	}

	storePrivileges(tdbb, transaction, name, obj_collation, USAGE_PRIVILEGES);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER,
		DDL_TRIGGER_CREATE_COLLATION, name);

	savePoint.release();	// everything is ok

	// Update DSQL cache
	METD_drop_collation(transaction, name);
	MET_dsql_cache_release(tdbb, SYM_intlsym_collation, name);
}

DdlNode* CreateCollationNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	const dsql_intlsym* resolvedCharSet = METD_get_charset(
		dsqlScratch->getTransaction(), forCharSet.length(), forCharSet.c_str());

	if (!resolvedCharSet)
	{
		// specified character set not found
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
				  Arg::Gds(isc_charset_not_found) << forCharSet);
	}

	forCharSetId = resolvedCharSet->intlsym_charset_id;

	if (fromName.hasData())
	{
		const dsql_intlsym* resolvedCollation = METD_get_collation(
			dsqlScratch->getTransaction(), fromName, forCharSetId);

		if (!resolvedCollation)
		{
			// Specified collation not found
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					  Arg::Gds(isc_collation_not_found) << fromName << forCharSet);
		}

		fromCollationId = resolvedCollation->intlsym_collate_id;
	}

	return DdlNode::dsqlPass(dsqlScratch);
}


//----------------------


void DropCollationNode::print(string& text) const
{
	text.printf(
		"DropCollationNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void DropCollationNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1331;	// gds__utility 
   } jrd_1330;
   struct {
          SSHORT jrd_1329;	// gds__utility 
   } jrd_1328;
   struct {
          SSHORT jrd_1327;	// gds__utility 
   } jrd_1326;
   struct {
          TEXT  jrd_1324 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1325;	// RDB$OBJECT_TYPE 
   } jrd_1323;
   struct {
          TEXT  jrd_1337 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_1338;	// gds__utility 
   } jrd_1336;
   struct {
          SSHORT jrd_1334;	// RDB$COLLATION_ID 
          SSHORT jrd_1335;	// RDB$CHARACTER_SET_ID 
   } jrd_1333;
   struct {
          TEXT  jrd_1344 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1345 [32];	// RDB$FUNCTION_NAME 
          TEXT  jrd_1346 [32];	// RDB$ARGUMENT_NAME 
          SSHORT jrd_1347;	// gds__utility 
          SSHORT jrd_1348;	// gds__null_flag 
   } jrd_1343;
   struct {
          SSHORT jrd_1341;	// RDB$COLLATION_ID 
          SSHORT jrd_1342;	// RDB$CHARACTER_SET_ID 
   } jrd_1340;
   struct {
          TEXT  jrd_1354 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1355 [32];	// RDB$PROCEDURE_NAME 
          TEXT  jrd_1356 [32];	// RDB$PARAMETER_NAME 
          SSHORT jrd_1357;	// gds__utility 
          SSHORT jrd_1358;	// gds__null_flag 
   } jrd_1353;
   struct {
          SSHORT jrd_1351;	// RDB$COLLATION_ID 
          SSHORT jrd_1352;	// RDB$CHARACTER_SET_ID 
   } jrd_1350;
   struct {
          TEXT  jrd_1364 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1365 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1366;	// gds__utility 
   } jrd_1363;
   struct {
          SSHORT jrd_1361;	// RDB$COLLATION_ID 
          SSHORT jrd_1362;	// RDB$CHARACTER_SET_ID 
   } jrd_1360;
   struct {
          SSHORT jrd_1384;	// gds__utility 
   } jrd_1383;
   struct {
          SSHORT jrd_1382;	// gds__utility 
   } jrd_1381;
   struct {
          TEXT  jrd_1371 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_1372 [32];	// RDB$CHARACTER_SET_NAME 
          TEXT  jrd_1373 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_1374 [32];	// RDB$DEFAULT_COLLATE_NAME 
          SSHORT jrd_1375;	// gds__utility 
          SSHORT jrd_1376;	// gds__null_flag 
          SSHORT jrd_1377;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_1378;	// gds__null_flag 
          SSHORT jrd_1379;	// RDB$COLLATION_ID 
          SSHORT jrd_1380;	// RDB$SYSTEM_FLAG 
   } jrd_1370;
   struct {
          TEXT  jrd_1369 [32];	// RDB$COLLATION_NAME 
   } jrd_1368;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	bool found = false;
	AutoCacheRequest request(tdbb, drq_e_colls, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		COLL IN RDB$COLLATIONS
		CROSS CS IN RDB$CHARACTER_SETS
		WITH COLL.RDB$COLLATION_NAME EQ name.c_str() AND
			 CS.RDB$CHARACTER_SET_ID EQ COLL.RDB$CHARACTER_SET_ID*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1367, sizeof(jrd_1367));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1368.jrd_1369, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1368);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 140, (UCHAR*) &jrd_1370);
	   if (!jrd_1370.jrd_1375) break;
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_DROP_COLLATION, name);

		if (/*COLL.RDB$SYSTEM_FLAG*/
		    jrd_1370.jrd_1380)
			status_exception::raise(Arg::Gds(isc_dyn_cannot_del_syscoll));

		if (/*COLL.RDB$COLLATION_ID*/
		    jrd_1370.jrd_1379 == 0 ||
			(!/*CS.RDB$DEFAULT_COLLATE_NAME.NULL*/
			  jrd_1370.jrd_1378 &&
				MetaName(/*COLL.RDB$COLLATION_NAME*/
					 jrd_1370.jrd_1373) == MetaName(/*CS.RDB$DEFAULT_COLLATE_NAME*/
	      jrd_1370.jrd_1374)))
		{
			fb_utils::exact_name_limit(/*CS.RDB$CHARACTER_SET_NAME*/
						   jrd_1370.jrd_1372,
				sizeof(/*CS.RDB$CHARACTER_SET_NAME*/
				       jrd_1370.jrd_1372));

			status_exception::raise(
				Arg::Gds(isc_dyn_cannot_del_def_coll) << /*CS.RDB$CHARACTER_SET_NAME*/
									 jrd_1370.jrd_1372);
		}

		found = true;
		fb_utils::exact_name_limit(/*COLL.RDB$COLLATION_NAME*/
					   jrd_1370.jrd_1373, sizeof(/*COLL.RDB$COLLATION_NAME*/
	 jrd_1370.jrd_1373));

		AutoCacheRequest request2(tdbb, drq_l_rfld_coll, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
			RF IN RDB$RELATION_FIELDS
			CROSS F IN RDB$FIELDS
			WITH RF.RDB$FIELD_SOURCE EQ F.RDB$FIELD_NAME AND
				 F.RDB$CHARACTER_SET_ID EQ COLL.RDB$CHARACTER_SET_ID AND
				 RF.RDB$COLLATION_ID EQ COLL.RDB$COLLATION_ID*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_1359, sizeof(jrd_1359));
		jrd_1360.jrd_1361 = jrd_1370.jrd_1379;
		jrd_1360.jrd_1362 = jrd_1370.jrd_1377;
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 4, (UCHAR*) &jrd_1360);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 66, (UCHAR*) &jrd_1363);
		   if (!jrd_1363.jrd_1366) break;
		{
			fb_utils::exact_name_limit(/*RF.RDB$RELATION_NAME*/
						   jrd_1363.jrd_1365, sizeof(/*RF.RDB$RELATION_NAME*/
	 jrd_1363.jrd_1365));
			fb_utils::exact_name_limit(/*RF.RDB$FIELD_NAME*/
						   jrd_1363.jrd_1364, sizeof(/*RF.RDB$FIELD_NAME*/
	 jrd_1363.jrd_1364));

			status_exception::raise(
				Arg::Gds(isc_dyn_coll_used_table) << /*COLL.RDB$COLLATION_NAME*/
								     jrd_1370.jrd_1373 <<
				/*RF.RDB$RELATION_NAME*/
				jrd_1363.jrd_1365 << /*RF.RDB$FIELD_NAME*/
    jrd_1363.jrd_1364);
		}
		/*END_FOR*/
		   }
		}

		request2.reset(tdbb, drq_l_prm_coll, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
			PRM IN RDB$PROCEDURE_PARAMETERS CROSS F IN RDB$FIELDS
			WITH PRM.RDB$FIELD_SOURCE EQ F.RDB$FIELD_NAME AND
				 F.RDB$CHARACTER_SET_ID EQ COLL.RDB$CHARACTER_SET_ID AND
				 PRM.RDB$COLLATION_ID EQ COLL.RDB$COLLATION_ID*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_1349, sizeof(jrd_1349));
		jrd_1350.jrd_1351 = jrd_1370.jrd_1379;
		jrd_1350.jrd_1352 = jrd_1370.jrd_1377;
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 4, (UCHAR*) &jrd_1350);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 100, (UCHAR*) &jrd_1353);
		   if (!jrd_1353.jrd_1357) break;
		{
			fb_utils::exact_name_limit(/*PRM.RDB$PARAMETER_NAME*/
						   jrd_1353.jrd_1356, sizeof(/*PRM.RDB$PARAMETER_NAME*/
	 jrd_1353.jrd_1356));

			status_exception::raise(
				Arg::Gds(isc_dyn_coll_used_procedure) <<
				/*COLL.RDB$COLLATION_NAME*/
				jrd_1370.jrd_1373 <<
				QualifiedName(/*PRM.RDB$PROCEDURE_NAME*/
					      jrd_1353.jrd_1355,
					(/*PRM.RDB$PACKAGE_NAME.NULL*/
					 jrd_1353.jrd_1358 ? NULL : /*PRM.RDB$PACKAGE_NAME*/
	  jrd_1353.jrd_1354)).toString().c_str() <<
				/*PRM.RDB$PARAMETER_NAME*/
				jrd_1353.jrd_1356);
		}
		/*END_FOR*/
		   }
		}

		request2.reset(tdbb, drq_l_arg_coll, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
			ARG IN RDB$FUNCTION_ARGUMENTS CROSS F IN RDB$FIELDS
			WITH ARG.RDB$FIELD_SOURCE EQ F.RDB$FIELD_NAME AND
				 F.RDB$CHARACTER_SET_ID EQ COLL.RDB$CHARACTER_SET_ID AND
				 ARG.RDB$COLLATION_ID EQ COLL.RDB$COLLATION_ID*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_1339, sizeof(jrd_1339));
		jrd_1340.jrd_1341 = jrd_1370.jrd_1379;
		jrd_1340.jrd_1342 = jrd_1370.jrd_1377;
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 4, (UCHAR*) &jrd_1340);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 100, (UCHAR*) &jrd_1343);
		   if (!jrd_1343.jrd_1347) break;
		{
			fb_utils::exact_name_limit(/*ARG.RDB$ARGUMENT_NAME*/
						   jrd_1343.jrd_1346, sizeof(/*ARG.RDB$ARGUMENT_NAME*/
	 jrd_1343.jrd_1346));

			status_exception::raise(
				Arg::Gds(isc_dyn_coll_used_function) <<
				/*COLL.RDB$COLLATION_NAME*/
				jrd_1370.jrd_1373 <<
				QualifiedName(/*ARG.RDB$FUNCTION_NAME*/
					      jrd_1343.jrd_1345,
					(/*ARG.RDB$PACKAGE_NAME.NULL*/
					 jrd_1343.jrd_1348 ? NULL : /*ARG.RDB$PACKAGE_NAME*/
	  jrd_1343.jrd_1344)).toString().c_str() <<
				/*ARG.RDB$ARGUMENT_NAME*/
				jrd_1343.jrd_1346);
		}
		/*END_FOR*/
		   }
		}

		request2.reset(tdbb, drq_l_fld_coll, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
			F IN RDB$FIELDS
			WITH F.RDB$CHARACTER_SET_ID EQ COLL.RDB$CHARACTER_SET_ID AND
				 F.RDB$COLLATION_ID EQ COLL.RDB$COLLATION_ID*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_1332, sizeof(jrd_1332));
		jrd_1333.jrd_1334 = jrd_1370.jrd_1379;
		jrd_1333.jrd_1335 = jrd_1370.jrd_1377;
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 4, (UCHAR*) &jrd_1333);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 34, (UCHAR*) &jrd_1336);
		   if (!jrd_1336.jrd_1338) break;
		{
			fb_utils::exact_name_limit(/*F.RDB$FIELD_NAME*/
						   jrd_1336.jrd_1337, sizeof(/*F.RDB$FIELD_NAME*/
	 jrd_1336.jrd_1337));

			status_exception::raise(
				Arg::Gds(isc_dyn_coll_used_domain) << /*COLL.RDB$COLLATION_NAME*/
								      jrd_1370.jrd_1373 << /*F.RDB$FIELD_NAME*/
    jrd_1336.jrd_1337);
		}
		/*END_FOR*/
		   }
		}

		/*ERASE COLL;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1381);

		if (!/*COLL.RDB$SECURITY_CLASS.NULL*/
		     jrd_1370.jrd_1376)
			deleteSecurityClass(tdbb, transaction, /*COLL.RDB$SECURITY_CLASS*/
							       jrd_1370.jrd_1371);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1383);
	   }
	}

	request.reset(tdbb, drq_e_coll_prvs, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH PRIV.RDB$RELATION_NAME EQ name.c_str() AND
			 PRIV.RDB$OBJECT_TYPE = obj_collation*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1322, sizeof(jrd_1322));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1323.jrd_1324, 32);
	jrd_1323.jrd_1325 = obj_collation;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_1323);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1326);
	   if (!jrd_1326.jrd_1327) break;
	{
		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1328);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1330);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_COLLATION, name);
	else
		status_exception::raise(Arg::Gds(isc_dyn_collation_not_found) << Arg::Str(name));

	savePoint.release();	// everything is ok

	// Update DSQL cache
	METD_drop_collation(transaction, name);
	MET_dsql_cache_release(tdbb, SYM_intlsym_collation, name);
}


//----------------------


void CreateDomainNode::print(string& text) const
{
	string nameTypeStr;
	nameType->print(nameTypeStr);

	text =
		"CreateDomainNode\n"
		"  " + nameTypeStr + "\n";
}

void CreateDomainNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1321;	// gds__utility 
   } jrd_1320;
   struct {
          bid  jrd_1310;	// RDB$DEFAULT_SOURCE 
          bid  jrd_1311;	// RDB$DEFAULT_VALUE 
          bid  jrd_1312;	// RDB$VALIDATION_SOURCE 
          bid  jrd_1313;	// RDB$VALIDATION_BLR 
          SSHORT jrd_1314;	// gds__null_flag 
          SSHORT jrd_1315;	// gds__null_flag 
          SSHORT jrd_1316;	// gds__null_flag 
          SSHORT jrd_1317;	// gds__null_flag 
          SSHORT jrd_1318;	// gds__null_flag 
          SSHORT jrd_1319;	// RDB$NULL_FLAG 
   } jrd_1309;
   struct {
          bid  jrd_1298;	// RDB$VALIDATION_BLR 
          bid  jrd_1299;	// RDB$VALIDATION_SOURCE 
          bid  jrd_1300;	// RDB$DEFAULT_VALUE 
          bid  jrd_1301;	// RDB$DEFAULT_SOURCE 
          SSHORT jrd_1302;	// gds__utility 
          SSHORT jrd_1303;	// gds__null_flag 
          SSHORT jrd_1304;	// RDB$NULL_FLAG 
          SSHORT jrd_1305;	// gds__null_flag 
          SSHORT jrd_1306;	// gds__null_flag 
          SSHORT jrd_1307;	// gds__null_flag 
          SSHORT jrd_1308;	// gds__null_flag 
   } jrd_1297;
   struct {
          TEXT  jrd_1296 [32];	// RDB$FIELD_NAME 
   } jrd_1295;
	Attachment* const attachment = transaction->tra_attachment;
	dsql_fld* type = nameType->type;

	// The commented line should be restored when implicit domains get their own sys flag.
	//if (fb_utils::implicit_domain(nameType->name.c_str()))
	if (strncmp(nameType->name.c_str(), IMPLICIT_DOMAIN_PREFIX, IMPLICIT_DOMAIN_PREFIX_LEN) == 0)
	{
		status_exception::raise(
			Arg::Gds(isc_sqlerr) << Arg::Num(-637) <<
			Arg::Gds(isc_dsql_implicit_domain_name) << nameType->name);
	}

	const ValueListNode* elements = type->ranges;
	const USHORT dims = elements ? elements->items.getCount() / 2 : 0;

	if (nameType->defaultClause && dims != 0)
	{
		// Default value is not allowed for array type in domain %s
		status_exception::raise(Arg::PrivateDyn(226) << nameType->name);
	}

	type->resolve(dsqlScratch);

	dsqlScratch->domainValue.dsc_dtype = type->dtype;
	dsqlScratch->domainValue.dsc_length = type->length;
	dsqlScratch->domainValue.dsc_scale = type->scale;

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
		DDL_TRIGGER_CREATE_DOMAIN, nameType->name);

	storeGlobalField(tdbb, transaction, nameType->name, type);

	if (nameType->defaultClause || check || notNull)
	{
		AutoCacheRequest request(tdbb, drq_m_fld, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			FLD IN RDB$FIELDS
			WITH FLD.RDB$FIELD_NAME EQ nameType->name.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_1294, sizeof(jrd_1294));
		gds__vtov ((const char*) nameType->name.c_str(), (char*) jrd_1295.jrd_1296, 32);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1295);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 46, (UCHAR*) &jrd_1297);
		   if (!jrd_1297.jrd_1302) break;
		{
			/*MODIFY FLD*/
			{
			
				if (nameType->defaultClause)
				{
					/*FLD.RDB$DEFAULT_SOURCE.NULL*/
					jrd_1297.jrd_1308 = FALSE;
					attachment->storeMetaDataBlob(tdbb, transaction, &/*FLD.RDB$DEFAULT_SOURCE*/
											  jrd_1297.jrd_1301,
						nameType->defaultClause->source);

					dsqlScratch->getBlrData().clear();
					dsqlScratch->appendUChar(dsqlScratch->isVersion4() ? blr_version4 : blr_version5);

					ValueExprNode* node = doDsqlPass(dsqlScratch, nameType->defaultClause->value);

					GEN_expr(dsqlScratch, node);

					dsqlScratch->appendUChar(blr_eoc);

					/*FLD.RDB$DEFAULT_VALUE.NULL*/
					jrd_1297.jrd_1307 = FALSE;
					attachment->storeBinaryBlob(tdbb, transaction, &/*FLD.RDB$DEFAULT_VALUE*/
											jrd_1297.jrd_1300,
						dsqlScratch->getBlrData());
				}

				if (check)
				{
					/*FLD.RDB$VALIDATION_SOURCE.NULL*/
					jrd_1297.jrd_1306 = FALSE;
					attachment->storeMetaDataBlob(tdbb, transaction, &/*FLD.RDB$VALIDATION_SOURCE*/
											  jrd_1297.jrd_1299,
						check->source);

					dsqlScratch->getBlrData().clear();
					dsqlScratch->appendUChar(dsqlScratch->isVersion4() ? blr_version4 : blr_version5);

					// Increment the context level for this statement, so that the context number for
					// any RSE generated for a SELECT within the CHECK clause will be greater than 0.
					// In the environment of a domain check constraint, context number 0 is reserved
					// for the "blr_fid, 0, 0, 0," which is emitted for a nod_dom_value, corresponding
					// to an occurance of the VALUE keyword in the body of the check constraint.
					// -- chrisj 1999-08-20
					++dsqlScratch->contextNumber;

					BoolExprNode* node = doDsqlPass(dsqlScratch, check->value);

					GEN_expr(dsqlScratch, node);

					dsqlScratch->appendUChar(blr_eoc);

					/*FLD.RDB$VALIDATION_BLR.NULL*/
					jrd_1297.jrd_1305 = FALSE;
					attachment->storeBinaryBlob(tdbb, transaction, &/*FLD.RDB$VALIDATION_BLR*/
											jrd_1297.jrd_1298,
						dsqlScratch->getBlrData());
				}

				if (notNull)
				{
					/*FLD.RDB$NULL_FLAG.NULL*/
					jrd_1297.jrd_1303 = FALSE;
					/*FLD.RDB$NULL_FLAG*/
					jrd_1297.jrd_1304 = 1;
				}
			/*END_MODIFY*/
			jrd_1309.jrd_1310 = jrd_1297.jrd_1301;
			jrd_1309.jrd_1311 = jrd_1297.jrd_1300;
			jrd_1309.jrd_1312 = jrd_1297.jrd_1299;
			jrd_1309.jrd_1313 = jrd_1297.jrd_1298;
			jrd_1309.jrd_1314 = jrd_1297.jrd_1308;
			jrd_1309.jrd_1315 = jrd_1297.jrd_1307;
			jrd_1309.jrd_1316 = jrd_1297.jrd_1306;
			jrd_1309.jrd_1317 = jrd_1297.jrd_1305;
			jrd_1309.jrd_1318 = jrd_1297.jrd_1303;
			jrd_1309.jrd_1319 = jrd_1297.jrd_1304;
			EXE_send (tdbb, request, 2, 44, (UCHAR*) &jrd_1309);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1320);
		   }
		}
	}

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER,
		DDL_TRIGGER_CREATE_DOMAIN, nameType->name);

	savePoint.release();	// everything is ok
}


//----------------------


// Compare the original field type with the new field type to determine if the original type can be
// changed to the new type.
//
// The following conversions are not allowed:
//   Blob to anything
//   Array to anything
//   Date to anything
//   Char to any numeric
//   Varchar to any numeric
//   Anything to Blob
//   Anything to Array
//
// This function throws an exception if the conversion can not be made.
//
// ASF: We should stop using dyn_fld here as soon DYN stops to be a caller of this function.
void AlterDomainNode::checkUpdate(const dyn_fld& origFld, const dyn_fld& newFld)
{
	ULONG errorCode = FB_SUCCESS;

	const USHORT origLen = DTYPE_IS_TEXT(origFld.dyn_dsc.dsc_dtype) ?
		origFld.dyn_charlen : DSC_string_length(&origFld.dyn_dsc);

	// Check to make sure that the old and new types are compatible
	switch (origFld.dyn_dtype)
	{
	// CHARACTER types
	case blr_text:
	case blr_varying:
	case blr_cstring:
		switch (newFld.dyn_dtype)
		{
		case blr_blob:
		case blr_blob_id:
			// Cannot change datatype for column %s.
			// The operation cannot be performed on BLOB, or ARRAY columns.
			errorCode = isc_dyn_dtype_invalid;
			break;

		case blr_sql_date:
		case blr_sql_time:
		case blr_timestamp:
		case blr_int64:
		case blr_long:
		case blr_short:
		case blr_d_float:
		case blr_double:
		case blr_float:
			// Cannot convert column %s from character to non-character data.
			errorCode = isc_dyn_dtype_conv_invalid;
			break;

		// If the original field is a character field and the new field is a character field,
		// is there enough space in the new field?
		case blr_text:
		case blr_varying:
		case blr_cstring:
			if (newFld.dyn_charlen < origLen)
			{
				// msg 208: New size specified for column %s must be at least %d characters.
				errorCode = isc_dyn_char_fld_too_small;
			}
			break;

		default:
			fb_assert(FALSE);
			errorCode = ENCODE_ISC_MSG(87, DYN_MSG_FAC);			// MODIFY RDB$FIELDS FAILED
			break;
		}
		break;

	// BLOB and ARRAY types
	case blr_blob:
	case blr_blob_id:
		// Cannot change datatype for column %s.
		// The operation cannot be performed on BLOB, or ARRAY columns.
		errorCode = isc_dyn_dtype_invalid;
		break;

	// DATE types
	case blr_sql_date:
	case blr_sql_time:
	case blr_timestamp:
		switch (newFld.dyn_dtype)
		{
		case blr_sql_date:
			if (origFld.dyn_dtype == blr_sql_time)
			{
				// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
				errorCode = isc_dyn_invalid_dtype_conversion;
			}
			break;

		case blr_sql_time:
			if (origFld.dyn_dtype == blr_sql_date)
			{
				// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
				errorCode = isc_dyn_invalid_dtype_conversion;
			}
			break;

		case blr_timestamp:
			if (origFld.dyn_dtype == blr_sql_time)
			{
				// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
				errorCode = isc_dyn_invalid_dtype_conversion;
			}
			break;

		// If the original field is a date field and the new field is a character field,
		// is there enough space in the new field?
		case blr_text:
		case blr_text2:
		case blr_varying:
		case blr_varying2:
		case blr_cstring:
		case blr_cstring2:
			if (newFld.dyn_charlen < origLen)
			{
				// msg 208: New size specified for column %s must be at least %d characters.
				errorCode = isc_dyn_char_fld_too_small;
			}
			break;

		default:
			// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
			errorCode = isc_dyn_invalid_dtype_conversion;
			break;
		}
		break;

	// NUMERIC types
	case blr_int64:
	case blr_long:
	case blr_short:
	case blr_d_float:
	case blr_double:
	case blr_float:
		switch (newFld.dyn_dtype)
		{
		case blr_blob:
		case blr_blob_id:
			// Cannot change datatype for column %s.
			// The operation cannot be performed on BLOB, or ARRAY columns.
			errorCode = isc_dyn_dtype_invalid;
			break;

		case blr_sql_date:
		case blr_sql_time:
		case blr_timestamp:
			// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
			errorCode = isc_dyn_invalid_dtype_conversion;
			break;

		// If the original field is a numeric field and the new field is a numeric field,
		// is there enough space in the new field (do not allow the base type to decrease)

		case blr_short:
			switch (origFld.dyn_dtype)
			{
			case blr_short:
				errorCode = checkUpdateNumericType(origFld, newFld);
				break;

			default:
				// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
				errorCode = isc_dyn_invalid_dtype_conversion;
				break;
			}
			break;

		case blr_long:
			switch (origFld.dyn_dtype)
			{
			case blr_long:
			case blr_short:
				errorCode = checkUpdateNumericType(origFld, newFld);
				break;

			default:
				// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
				errorCode = isc_dyn_invalid_dtype_conversion;
				break;
			}
			break;

		case blr_float:
			switch (origFld.dyn_dtype)
			{
			case blr_float:
			case blr_short:
				break;

			default:
				// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
				errorCode = isc_dyn_invalid_dtype_conversion;
				break;
			}
			break;

		case blr_int64:
			switch (origFld.dyn_dtype)
			{
			case blr_int64:
			case blr_long:
			case blr_short:
				errorCode = checkUpdateNumericType(origFld, newFld);
				break;

			default:
				// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
				errorCode = isc_dyn_invalid_dtype_conversion;
				break;
			}
			break;

		case blr_d_float:
		case blr_double:
			switch (origFld.dyn_dtype)
			{
			case blr_double:
			case blr_d_float:
			case blr_float:
			case blr_short:
			case blr_long:
				break;

			default:
				// Cannot change datatype for column %s.  Conversion from base type %s to base type %s is not supported.
				errorCode = isc_dyn_invalid_dtype_conversion;
				break;
			}
			break;

		// If the original field is a numeric field and the new field is a character field,
		// is there enough space in the new field?
		case blr_text:
		case blr_varying:
		case blr_cstring:
			if (newFld.dyn_charlen < origLen)
			{
				// msg 208: New size specified for column %s must be at least %d characters.
				errorCode = isc_dyn_char_fld_too_small;
			}
			break;

		default:
			fb_assert(FALSE);
			errorCode = ENCODE_ISC_MSG(87, DYN_MSG_FAC);			// MODIFY RDB$FIELDS FAILED
			break;
		}
		break;

	default:
		fb_assert(FALSE);
		errorCode = ENCODE_ISC_MSG(87, DYN_MSG_FAC);				// MODIFY RDB$FIELDS FAILED
		break;
	}

	if (errorCode == FB_SUCCESS)
		return;

	switch (errorCode)
	{
	case isc_dyn_dtype_invalid:
		// Cannot change datatype for column %s.The operation cannot be performed on DATE, BLOB, or ARRAY columns.
		status_exception::raise(Arg::Gds(errorCode) << origFld.dyn_fld_name.c_str());
		break;

	case isc_dyn_dtype_conv_invalid:
		// Cannot convert column %s from character to non-character data.
		status_exception::raise(Arg::Gds(errorCode) << origFld.dyn_fld_name.c_str());
		break;

	case isc_dyn_char_fld_too_small:
		// msg 208: New size specified for column %s must be at least %d characters.
		status_exception::raise(
			Arg::Gds(errorCode) << origFld.dyn_fld_name.c_str() << Arg::Num(origLen));
		break;

	case isc_dyn_scale_too_big:
		{
			int code = errorCode;
			int diff = newFld.dyn_precision -
				(origFld.dyn_precision + origFld.dyn_dsc.dsc_scale);
			if (diff < 0)
			{
				// If new scale becomes negative externally, the message is useless for the user.
				// (The scale is always zero or negative for us but externally is non-negative.)
				// Let's ask the user to widen the precision, then. Example: numeric(4, 0) -> numeric(1, 1).
				code = isc_dyn_precision_too_small;
				diff = newFld.dyn_precision - newFld.dyn_dsc.dsc_scale - diff;
			}

			// scale_too_big: New scale specified for column @1 must be at most @2.
			// precision_too_small: New precision specified for column @1 must be at least @2.
			status_exception::raise(
				Arg::Gds(code) << origFld.dyn_fld_name.c_str() << Arg::Num(diff));
		}
		break;

	case isc_dyn_invalid_dtype_conversion:
		{
			TEXT orig_type[25], new_type[25];

			DSC_get_dtype_name(&origFld.dyn_dsc, orig_type, sizeof(orig_type));
			DSC_get_dtype_name(&newFld.dyn_dsc, new_type, sizeof(new_type));

			// Cannot change datatype for @1.  Conversion from base type @2 to @3 is not supported.
			status_exception::raise(
				Arg::Gds(errorCode) << origFld.dyn_fld_name.c_str() << orig_type << new_type);
		}
		break;

	default:
		// msg 95: "MODIFY RDB$RELATION_FIELDS failed"
		status_exception::raise(Arg::PrivateDyn(95));
	}
}

// Compare the original field type with the new field type to determine if the original type can be
// changed to the new type.
// The types should be integral, since it tests only numeric/decimal subtypes to ensure the scale is
// not being widened at the expense of the precision, because the old stored values should fit in
// the new definition.
//
// This function returns an error code if the conversion can not be made. If the conversion can be
// made, FB_SUCCESS is returned.
ULONG AlterDomainNode::checkUpdateNumericType(const dyn_fld& origFld, const dyn_fld& newFld)
{
 	// Since dsc_scale is negative, the sum of precision and scale produces
	// the width of the integral part.
	if (origFld.dyn_sub_type && newFld.dyn_sub_type &&
		origFld.dyn_precision + origFld.dyn_dsc.dsc_scale >
			newFld.dyn_precision + newFld.dyn_dsc.dsc_scale)
	{
		return isc_dyn_scale_too_big;
	}

	return FB_SUCCESS;
}

// Retrieves the type information for a domain so that it can be compared to a local field before
// modifying the datatype of a field.
void AlterDomainNode::getDomainType(thread_db* tdbb, jrd_tra* transaction, dyn_fld& dynFld)
{
   struct {
          SSHORT jrd_1282;	// gds__utility 
          SSHORT jrd_1283;	// gds__null_flag 
          SSHORT jrd_1284;	// RDB$DIMENSIONS 
          SSHORT jrd_1285;	// RDB$NULL_FLAG 
          SSHORT jrd_1286;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_1287;	// RDB$FIELD_PRECISION 
          SSHORT jrd_1288;	// RDB$COLLATION_ID 
          SSHORT jrd_1289;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_1290;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_1291;	// RDB$FIELD_LENGTH 
          SSHORT jrd_1292;	// RDB$FIELD_SCALE 
          SSHORT jrd_1293;	// RDB$FIELD_TYPE 
   } jrd_1281;
   struct {
          TEXT  jrd_1280 [32];	// RDB$FIELD_NAME 
   } jrd_1279;
	AutoRequest request;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FLD IN RDB$FIELDS
		WITH FLD.RDB$FIELD_NAME EQ dynFld.dyn_fld_source.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1278, sizeof(jrd_1278));
	gds__vtov ((const char*) dynFld.dyn_fld_source.c_str(), (char*) jrd_1279.jrd_1280, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1279);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 24, (UCHAR*) &jrd_1281);
	   if (!jrd_1281.jrd_1282) break;;
	{
		DSC_make_descriptor(&dynFld.dyn_dsc, /*FLD.RDB$FIELD_TYPE*/
						     jrd_1281.jrd_1293, /*FLD.RDB$FIELD_SCALE*/
  jrd_1281.jrd_1292,
			/*FLD.RDB$FIELD_LENGTH*/
			jrd_1281.jrd_1291, /*FLD.RDB$FIELD_SUB_TYPE*/
  jrd_1281.jrd_1290, /*FLD.RDB$CHARACTER_SET_ID*/
  jrd_1281.jrd_1289,
			/*FLD.RDB$COLLATION_ID*/
			jrd_1281.jrd_1288);

		dynFld.dyn_charbytelen = /*FLD.RDB$FIELD_LENGTH*/
					 jrd_1281.jrd_1291;
		dynFld.dyn_dtype = /*FLD.RDB$FIELD_TYPE*/
				   jrd_1281.jrd_1293;
		dynFld.dyn_precision = /*FLD.RDB$FIELD_PRECISION*/
				       jrd_1281.jrd_1287;
		dynFld.dyn_sub_type = /*FLD.RDB$FIELD_SUB_TYPE*/
				      jrd_1281.jrd_1290;
		dynFld.dyn_charlen = /*FLD.RDB$CHARACTER_LENGTH*/
				     jrd_1281.jrd_1286;
		dynFld.dyn_collation = /*FLD.RDB$COLLATION_ID*/
				       jrd_1281.jrd_1288;
		dynFld.dyn_null_flag = /*FLD.RDB$NULL_FLAG*/
				       jrd_1281.jrd_1285 != 0;

		if (!/*FLD.RDB$DIMENSIONS.NULL*/
		     jrd_1281.jrd_1283 && /*FLD.RDB$DIMENSIONS*/
    jrd_1281.jrd_1284 > 0)
			dynFld.dyn_dtype = blr_blob;
	}
	/*END_FOR*/
	   }
	}
}

// Updates the field names in an index and forces the index to be rebuilt with the new field names.
void AlterDomainNode::modifyLocalFieldIndex(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& fieldName, const MetaName& newFieldName)
{
   struct {
          TEXT  jrd_1277 [32];	// RDB$FIELD_NAME 
   } jrd_1276;
   struct {
          SSHORT jrd_1275;	// gds__utility 
   } jrd_1274;
   struct {
          TEXT  jrd_1273 [32];	// RDB$INDEX_NAME 
   } jrd_1272;
   struct {
          TEXT  jrd_1269 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_1270 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_1271;	// gds__utility 
   } jrd_1268;
   struct {
          TEXT  jrd_1266 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1267 [32];	// RDB$RELATION_NAME 
   } jrd_1265;
	AutoRequest request;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES CROSS IDXS IN RDB$INDEX_SEGMENTS WITH
			IDX.RDB$INDEX_NAME EQ IDXS.RDB$INDEX_NAME AND
			IDX.RDB$RELATION_NAME EQ relationName.c_str() AND
			IDXS.RDB$FIELD_NAME EQ fieldName.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1264, sizeof(jrd_1264));
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_1265.jrd_1266, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_1265.jrd_1267, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_1265);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_1268);
	   if (!jrd_1268.jrd_1271) break;
	{
		// Change the name of the field in the index
		/*MODIFY IDXS USING*/
		{
		
			memcpy(/*IDXS.RDB$FIELD_NAME*/
			       jrd_1268.jrd_1270, newFieldName.c_str(), sizeof(/*IDXS.RDB$FIELD_NAME*/
			       jrd_1268.jrd_1270));
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_1268.jrd_1270, (char*) jrd_1276.jrd_1277, 32);
		EXE_send (tdbb, request, 4, 32, (UCHAR*) &jrd_1276);
		}

		// Set the index name to itself to tell the index to rebuild
		/*MODIFY IDX USING*/
		{
		
			// This is to fool both gpre and gcc.
			char* p = /*IDX.RDB$INDEX_NAME*/
				  jrd_1268.jrd_1269;
			p[MAX_SQL_IDENTIFIER_LEN] = 0;
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_1268.jrd_1269, (char*) jrd_1272.jrd_1273, 32);
		EXE_send (tdbb, request, 2, 32, (UCHAR*) &jrd_1272);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1274);
	   }
	}
}

void AlterDomainNode::print(string& text) const
{
	text.printf(
		"AlterDomainNode\n"
		"  %s\n", name.c_str());
}

void AlterDomainNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          TEXT  jrd_1189 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1190 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1191;	// gds__utility 
   } jrd_1188;
   struct {
          TEXT  jrd_1187 [32];	// RDB$FIELD_SOURCE 
   } jrd_1186;
   struct {
          SSHORT jrd_1196;	// gds__utility 
   } jrd_1195;
   struct {
          TEXT  jrd_1194 [32];	// RDB$FIELD_NAME 
   } jrd_1193;
   struct {
          SSHORT jrd_1263;	// gds__utility 
   } jrd_1262;
   struct {
          bid  jrd_1233;	// RDB$VALIDATION_BLR 
          bid  jrd_1234;	// RDB$VALIDATION_SOURCE 
          bid  jrd_1235;	// RDB$DEFAULT_VALUE 
          bid  jrd_1236;	// RDB$DEFAULT_SOURCE 
          TEXT  jrd_1237 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_1238;	// gds__null_flag 
          SSHORT jrd_1239;	// gds__null_flag 
          SSHORT jrd_1240;	// gds__null_flag 
          SSHORT jrd_1241;	// gds__null_flag 
          SSHORT jrd_1242;	// gds__null_flag 
          SSHORT jrd_1243;	// RDB$DIMENSIONS 
          SSHORT jrd_1244;	// gds__null_flag 
          SSHORT jrd_1245;	// RDB$NULL_FLAG 
          SSHORT jrd_1246;	// RDB$FIELD_TYPE 
          SSHORT jrd_1247;	// gds__null_flag 
          SSHORT jrd_1248;	// RDB$FIELD_SCALE 
          SSHORT jrd_1249;	// RDB$FIELD_LENGTH 
          SSHORT jrd_1250;	// gds__null_flag 
          SSHORT jrd_1251;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_1252;	// gds__null_flag 
          SSHORT jrd_1253;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_1254;	// gds__null_flag 
          SSHORT jrd_1255;	// RDB$COLLATION_ID 
          SSHORT jrd_1256;	// gds__null_flag 
          SSHORT jrd_1257;	// RDB$FIELD_PRECISION 
          SSHORT jrd_1258;	// gds__null_flag 
          SSHORT jrd_1259;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_1260;	// gds__null_flag 
          SSHORT jrd_1261;	// RDB$SEGMENT_LENGTH 
   } jrd_1232;
   struct {
          TEXT  jrd_1201 [32];	// RDB$FIELD_NAME 
          bid  jrd_1202;	// RDB$DEFAULT_SOURCE 
          bid  jrd_1203;	// RDB$DEFAULT_VALUE 
          bid  jrd_1204;	// RDB$VALIDATION_SOURCE 
          bid  jrd_1205;	// RDB$VALIDATION_BLR 
          SSHORT jrd_1206;	// gds__utility 
          SSHORT jrd_1207;	// gds__null_flag 
          SSHORT jrd_1208;	// gds__null_flag 
          SSHORT jrd_1209;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_1210;	// gds__null_flag 
          SSHORT jrd_1211;	// gds__null_flag 
          SSHORT jrd_1212;	// gds__null_flag 
          SSHORT jrd_1213;	// gds__null_flag 
          SSHORT jrd_1214;	// gds__null_flag 
          SSHORT jrd_1215;	// gds__null_flag 
          SSHORT jrd_1216;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_1217;	// RDB$FIELD_PRECISION 
          SSHORT jrd_1218;	// RDB$COLLATION_ID 
          SSHORT jrd_1219;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_1220;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_1221;	// RDB$FIELD_LENGTH 
          SSHORT jrd_1222;	// RDB$FIELD_SCALE 
          SSHORT jrd_1223;	// RDB$FIELD_TYPE 
          SSHORT jrd_1224;	// gds__null_flag 
          SSHORT jrd_1225;	// RDB$NULL_FLAG 
          SSHORT jrd_1226;	// RDB$DIMENSIONS 
          SSHORT jrd_1227;	// gds__null_flag 
          SSHORT jrd_1228;	// gds__null_flag 
          SSHORT jrd_1229;	// gds__null_flag 
          SSHORT jrd_1230;	// gds__null_flag 
          SSHORT jrd_1231;	// RDB$SYSTEM_FLAG 
   } jrd_1200;
   struct {
          TEXT  jrd_1199 [32];	// RDB$FIELD_NAME 
   } jrd_1198;
	Attachment* const attachment = transaction->tra_attachment;

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_m_fld2, DYN_REQUESTS);
	bool found = false;

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FLD IN RDB$FIELDS
		WITH FLD.RDB$FIELD_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1197, sizeof(jrd_1197));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1198.jrd_1199, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1198);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 116, (UCHAR*) &jrd_1200);
	   if (!jrd_1200.jrd_1206) break;
	{
		found = true;
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_ALTER_DOMAIN, name);

		if (/*FLD.RDB$SYSTEM_FLAG*/
		    jrd_1200.jrd_1231 == fb_sysflag_system)
		{
			status_exception::raise(Arg::Gds(isc_dyn_cant_modify_sysobj) <<
				"domain" << Arg::Str(name));
		}

		/*MODIFY FLD*/
		{
		
			if (dropConstraint)
			{
				/*FLD.RDB$VALIDATION_BLR.NULL*/
				jrd_1200.jrd_1230 = TRUE;
				/*FLD.RDB$VALIDATION_SOURCE.NULL*/
				jrd_1200.jrd_1229 = TRUE;
			}

			if (dropDefault)
			{
				/*FLD.RDB$DEFAULT_VALUE.NULL*/
				jrd_1200.jrd_1228 = TRUE;
				/*FLD.RDB$DEFAULT_SOURCE.NULL*/
				jrd_1200.jrd_1227 = TRUE;
			}

			if (setConstraint)
			{
				if (!/*FLD.RDB$VALIDATION_BLR.NULL*/
				     jrd_1200.jrd_1230)
				{
					// msg 160: "Only one constraint allowed for a domain"
					status_exception::raise(Arg::PrivateDyn(160));
				}

				dsql_fld localField(dsqlScratch->getStatement()->getPool());

				// Get the attributes of the domain, and set any occurrences of
				// keyword VALUE to the correct type, length, scale, etc.
				if (!METD_get_domain(dsqlScratch->getTransaction(), &localField, name))
				{
					// Specified domain or source field does not exist
					status_exception::raise(
						Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
						Arg::Gds(isc_dsql_command_err) <<
						Arg::Gds(isc_dsql_domain_not_found) << name);
				}

				dsqlScratch->domainValue.dsc_dtype = localField.dtype;
				dsqlScratch->domainValue.dsc_length = localField.length;
				dsqlScratch->domainValue.dsc_scale = localField.scale;

				/*FLD.RDB$VALIDATION_SOURCE.NULL*/
				jrd_1200.jrd_1229 = FALSE;
				attachment->storeMetaDataBlob(tdbb, transaction, &/*FLD.RDB$VALIDATION_SOURCE*/
										  jrd_1200.jrd_1204,
					setConstraint->source);

				dsqlScratch->getBlrData().clear();
				dsqlScratch->appendUChar(dsqlScratch->isVersion4() ? blr_version4 : blr_version5);

				// Increment the context level for this statement, so that the context number for
				// any RSE generated for a SELECT within the CHECK clause will be greater than 0.
				// In the environment of a domain check constraint, context number 0 is reserved
				// for the "blr_fid, 0, 0, 0," which is emitted for a nod_dom_value, corresponding
				// to an occurance of the VALUE keyword in the body of the check constraint.
				// -- chrisj 1999-08-20
				++dsqlScratch->contextNumber;

				BoolExprNode* node = doDsqlPass(dsqlScratch, setConstraint->value);

				GEN_expr(dsqlScratch, node);

				dsqlScratch->appendUChar(blr_eoc);

				/*FLD.RDB$VALIDATION_BLR.NULL*/
				jrd_1200.jrd_1230 = FALSE;
				attachment->storeBinaryBlob(tdbb, transaction, &/*FLD.RDB$VALIDATION_BLR*/
										jrd_1200.jrd_1205,
					dsqlScratch->getBlrData());
			}

			if (setDefault)
			{
				if (/*FLD.RDB$DIMENSIONS*/
				    jrd_1200.jrd_1226)
				{
					// msg 226: "Default value is not allowed for array type in domain %s"
					status_exception::raise(Arg::PrivateDyn(226) << name);
				}

				/*FLD.RDB$DEFAULT_SOURCE.NULL*/
				jrd_1200.jrd_1227 = FALSE;
				attachment->storeMetaDataBlob(tdbb, transaction, &/*FLD.RDB$DEFAULT_SOURCE*/
										  jrd_1200.jrd_1202,
					setDefault->source);

				dsqlScratch->getBlrData().clear();
				dsqlScratch->appendUChar(dsqlScratch->isVersion4() ? blr_version4 : blr_version5);

				ValueExprNode* node = doDsqlPass(dsqlScratch, setDefault->value);

				GEN_expr(dsqlScratch, node);

				dsqlScratch->appendUChar(blr_eoc);

				/*FLD.RDB$DEFAULT_VALUE.NULL*/
				jrd_1200.jrd_1228 = FALSE;
				attachment->storeBinaryBlob(tdbb, transaction, &/*FLD.RDB$DEFAULT_VALUE*/
										jrd_1200.jrd_1203,
					dsqlScratch->getBlrData());
			}

			if (notNullFlag.specified)
			{
				/*FLD.RDB$NULL_FLAG.NULL*/
				jrd_1200.jrd_1224 = FALSE;
				/*FLD.RDB$NULL_FLAG*/
				jrd_1200.jrd_1225 = notNullFlag.value;
			}

			if (type)
			{
				type->resolve(dsqlScratch);

				dyn_fld origDom, newDom;

				DSC_make_descriptor(&origDom.dyn_dsc, /*FLD.RDB$FIELD_TYPE*/
								      jrd_1200.jrd_1223, /*FLD.RDB$FIELD_SCALE*/
  jrd_1200.jrd_1222,
					/*FLD.RDB$FIELD_LENGTH*/
					jrd_1200.jrd_1221, /*FLD.RDB$FIELD_SUB_TYPE*/
  jrd_1200.jrd_1220, /*FLD.RDB$CHARACTER_SET_ID*/
  jrd_1200.jrd_1219,
					/*FLD.RDB$COLLATION_ID*/
					jrd_1200.jrd_1218);

				origDom.dyn_fld_name = name;
				origDom.dyn_charbytelen = /*FLD.RDB$FIELD_LENGTH*/
							  jrd_1200.jrd_1221;
				origDom.dyn_dtype = /*FLD.RDB$FIELD_TYPE*/
						    jrd_1200.jrd_1223;
				origDom.dyn_precision = /*FLD.RDB$FIELD_PRECISION*/
							jrd_1200.jrd_1217;
				origDom.dyn_sub_type = /*FLD.RDB$FIELD_SUB_TYPE*/
						       jrd_1200.jrd_1220;
				origDom.dyn_charlen = /*FLD.RDB$CHARACTER_LENGTH*/
						      jrd_1200.jrd_1216;
				origDom.dyn_collation = /*FLD.RDB$COLLATION_ID*/
							jrd_1200.jrd_1218;
				origDom.dyn_null_flag = !/*FLD.RDB$NULL_FLAG.NULL*/
							 jrd_1200.jrd_1224 && /*FLD.RDB$NULL_FLAG*/
    jrd_1200.jrd_1225 != 0;

				// If the original field type is an array, force its blr type to blr_blob
				if (/*FLD.RDB$DIMENSIONS*/
				    jrd_1200.jrd_1226 != 0)
					origDom.dyn_dtype = blr_blob;

				USHORT typeLength = type->length;
				switch (type->dtype)
				{
					case dtype_varying:
						typeLength -= sizeof(USHORT);
						break;

					// Not valid for domains, but may be important for a future refactor.
					case dtype_cstring:
						--typeLength;
						break;

					default:
						break;
				}

				DSC_make_descriptor(&newDom.dyn_dsc, blr_dtypes[type->dtype], type->scale,
					typeLength, type->subType, type->charSetId, type->collationId);

				newDom.dyn_fld_name = name;
				newDom.dyn_charbytelen = typeLength;
				newDom.dyn_dtype = blr_dtypes[type->dtype];
				newDom.dyn_precision = type->precision;
				newDom.dyn_sub_type = type->subType;
				newDom.dyn_charlen = type->charLength;
				newDom.dyn_collation = type->collationId;
				newDom.dyn_null_flag = type->notNull;

				// Now that we have all of the information needed, let's check to see if the field
				// type can be modifed.

				checkUpdate(origDom, newDom);

				if (!newDom.dyn_dsc.isExact() || newDom.dyn_dsc.dsc_scale != 0)
				{
					AutoCacheRequest request(tdbb, drq_l_ident_gens, DYN_REQUESTS);

					/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
						RFR IN RDB$RELATION_FIELDS
						WITH RFR.RDB$FIELD_SOURCE = FLD.RDB$FIELD_NAME AND
							 RFR.RDB$GENERATOR_NAME NOT MISSING*/
					{
					request.compile(tdbb, (UCHAR*) jrd_1192, sizeof(jrd_1192));
					gds__vtov ((const char*) jrd_1200.jrd_1201, (char*) jrd_1193.jrd_1194, 32);
					EXE_start (tdbb, request, transaction);
					EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1193);
					while (1)
					   {
					   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1195);
					   if (!jrd_1195.jrd_1196) break;
					{
						// Domain @1 must be of exact number type with zero scale because it's used
						// in an identity column.
						status_exception::raise(Arg::PrivateDyn(276) << name);
					}
					/*END_FOR*/
					   }
					}
				}

				// If the datatype was changed, update any indexes that involved the domain

				AutoRequest request2;

				/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
					DOM IN RDB$RELATION_FIELDS
					WITH DOM.RDB$FIELD_SOURCE EQ name.c_str()*/
				{
				request2.compile(tdbb, (UCHAR*) jrd_1185, sizeof(jrd_1185));
				gds__vtov ((const char*) name.c_str(), (char*) jrd_1186.jrd_1187, 32);
				EXE_start (tdbb, request2, transaction);
				EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_1186);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 66, (UCHAR*) &jrd_1188);
				   if (!jrd_1188.jrd_1191) break;
				{
					modifyLocalFieldIndex(tdbb, transaction, /*DOM.RDB$RELATION_NAME*/
										 jrd_1188.jrd_1190,
						/*DOM.RDB$FIELD_NAME*/
						jrd_1188.jrd_1189, /*DOM.RDB$FIELD_NAME*/
  jrd_1188.jrd_1189);
				}
				/*END_FOR*/
				   }
				}

				// Update RDB$FIELDS
				updateRdbFields(type,
					/*FLD.RDB$FIELD_TYPE*/
					jrd_1200.jrd_1223,
					/*FLD.RDB$FIELD_LENGTH*/
					jrd_1200.jrd_1221,
					/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
					jrd_1200.jrd_1215, /*FLD.RDB$FIELD_SUB_TYPE*/
  jrd_1200.jrd_1220,
					/*FLD.RDB$FIELD_SCALE.NULL*/
					jrd_1200.jrd_1214, /*FLD.RDB$FIELD_SCALE*/
  jrd_1200.jrd_1222,
					/*FLD.RDB$CHARACTER_SET_ID.NULL*/
					jrd_1200.jrd_1213, /*FLD.RDB$CHARACTER_SET_ID*/
  jrd_1200.jrd_1219,
					/*FLD.RDB$CHARACTER_LENGTH.NULL*/
					jrd_1200.jrd_1212, /*FLD.RDB$CHARACTER_LENGTH*/
  jrd_1200.jrd_1216,
					/*FLD.RDB$FIELD_PRECISION.NULL*/
					jrd_1200.jrd_1211, /*FLD.RDB$FIELD_PRECISION*/
  jrd_1200.jrd_1217,
					/*FLD.RDB$COLLATION_ID.NULL*/
					jrd_1200.jrd_1210, /*FLD.RDB$COLLATION_ID*/
  jrd_1200.jrd_1218,
					/*FLD.RDB$SEGMENT_LENGTH.NULL*/
					jrd_1200.jrd_1208, /*FLD.RDB$SEGMENT_LENGTH*/
  jrd_1200.jrd_1209);
			}

			if (renameTo.hasData())
			{
				rename(tdbb, transaction, (/*FLD.RDB$DIMENSIONS.NULL*/
							   jrd_1200.jrd_1207 ? 0 : /*FLD.RDB$DIMENSIONS*/
       jrd_1200.jrd_1226));
				strcpy(/*FLD.RDB$FIELD_NAME*/
				       jrd_1200.jrd_1201, renameTo.c_str());
			}
		/*END_MODIFY*/
		jrd_1232.jrd_1233 = jrd_1200.jrd_1205;
		jrd_1232.jrd_1234 = jrd_1200.jrd_1204;
		jrd_1232.jrd_1235 = jrd_1200.jrd_1203;
		jrd_1232.jrd_1236 = jrd_1200.jrd_1202;
		gds__vtov((const char*) jrd_1200.jrd_1201, (char*) jrd_1232.jrd_1237, 32);
		jrd_1232.jrd_1238 = jrd_1200.jrd_1230;
		jrd_1232.jrd_1239 = jrd_1200.jrd_1229;
		jrd_1232.jrd_1240 = jrd_1200.jrd_1228;
		jrd_1232.jrd_1241 = jrd_1200.jrd_1227;
		jrd_1232.jrd_1242 = jrd_1200.jrd_1207;
		jrd_1232.jrd_1243 = jrd_1200.jrd_1226;
		jrd_1232.jrd_1244 = jrd_1200.jrd_1224;
		jrd_1232.jrd_1245 = jrd_1200.jrd_1225;
		jrd_1232.jrd_1246 = jrd_1200.jrd_1223;
		jrd_1232.jrd_1247 = jrd_1200.jrd_1214;
		jrd_1232.jrd_1248 = jrd_1200.jrd_1222;
		jrd_1232.jrd_1249 = jrd_1200.jrd_1221;
		jrd_1232.jrd_1250 = jrd_1200.jrd_1215;
		jrd_1232.jrd_1251 = jrd_1200.jrd_1220;
		jrd_1232.jrd_1252 = jrd_1200.jrd_1213;
		jrd_1232.jrd_1253 = jrd_1200.jrd_1219;
		jrd_1232.jrd_1254 = jrd_1200.jrd_1210;
		jrd_1232.jrd_1255 = jrd_1200.jrd_1218;
		jrd_1232.jrd_1256 = jrd_1200.jrd_1211;
		jrd_1232.jrd_1257 = jrd_1200.jrd_1217;
		jrd_1232.jrd_1258 = jrd_1200.jrd_1212;
		jrd_1232.jrd_1259 = jrd_1200.jrd_1216;
		jrd_1232.jrd_1260 = jrd_1200.jrd_1208;
		jrd_1232.jrd_1261 = jrd_1200.jrd_1209;
		EXE_send (tdbb, request, 2, 112, (UCHAR*) &jrd_1232);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1262);
	   }
	}

	if (!found)
	{
		// msg 89: "Global field not found"
		status_exception::raise(Arg::PrivateDyn(89));
	}

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_ALTER_DOMAIN, name);

	savePoint.release();	// everything is ok
}

void AlterDomainNode::rename(thread_db* tdbb, jrd_tra* transaction, SSHORT dimensions)
{
   struct {
          SSHORT jrd_1169;	// gds__utility 
   } jrd_1168;
   struct {
          TEXT  jrd_1167 [32];	// RDB$FIELD_SOURCE 
   } jrd_1166;
   struct {
          TEXT  jrd_1162 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1163 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_1164 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_1165;	// gds__utility 
   } jrd_1161;
   struct {
          TEXT  jrd_1160 [32];	// RDB$FIELD_SOURCE 
   } jrd_1159;
   struct {
          SSHORT jrd_1179;	// gds__utility 
   } jrd_1178;
   struct {
          TEXT  jrd_1177 [32];	// RDB$FIELD_NAME 
   } jrd_1176;
   struct {
          TEXT  jrd_1174 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_1175;	// gds__utility 
   } jrd_1173;
   struct {
          TEXT  jrd_1172 [32];	// RDB$FIELD_NAME 
   } jrd_1171;
   struct {
          SSHORT jrd_1184;	// gds__utility 
   } jrd_1183;
   struct {
          TEXT  jrd_1182 [32];	// RDB$FIELD_NAME 
   } jrd_1181;
	// Checks to see if the given domain already exists.

	AutoRequest request;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FLD IN RDB$FIELDS WITH FLD.RDB$FIELD_NAME EQ renameTo.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1180, sizeof(jrd_1180));
	gds__vtov ((const char*) renameTo.c_str(), (char*) jrd_1181.jrd_1182, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1181);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1183);
	   if (!jrd_1183.jrd_1184) break;
	{
		// msg 204: Cannot rename domain %s to %s.  A domain with that name already exists.
		status_exception::raise(Arg::PrivateDyn(204) << name << renameTo);
	}
	/*END_FOR*/
	   }
	}

	// CVC: Let's update the dimensions, too.
	if (dimensions != 0)
	{
		request.reset();

		/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			FDIM IN RDB$FIELD_DIMENSIONS
			WITH FDIM.RDB$FIELD_NAME EQ name.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_1170, sizeof(jrd_1170));
		gds__vtov ((const char*) name.c_str(), (char*) jrd_1171.jrd_1172, 32);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1171);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_1173);
		   if (!jrd_1173.jrd_1175) break;
		{
			/*MODIFY FDIM USING*/
			{
			
				strcpy(/*FDIM.RDB$FIELD_NAME*/
				       jrd_1173.jrd_1174, renameTo.c_str());
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_1173.jrd_1174, (char*) jrd_1176.jrd_1177, 32);
			EXE_send (tdbb, request, 2, 32, (UCHAR*) &jrd_1176);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1178);
		   }
		}
	}

	request.reset();

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		RFLD IN RDB$RELATION_FIELDS
		WITH RFLD.RDB$FIELD_SOURCE EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1158, sizeof(jrd_1158));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1159.jrd_1160, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1159);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 98, (UCHAR*) &jrd_1161);
	   if (!jrd_1161.jrd_1165) break;
	{
		/*MODIFY RFLD USING*/
		{
		
			strcpy(/*RFLD.RDB$FIELD_SOURCE*/
			       jrd_1161.jrd_1164, renameTo.c_str());
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_1161.jrd_1164, (char*) jrd_1166.jrd_1167, 32);
		EXE_send (tdbb, request, 2, 32, (UCHAR*) &jrd_1166);
		}

		modifyLocalFieldIndex(tdbb, transaction, /*RFLD.RDB$RELATION_NAME*/
							 jrd_1161.jrd_1163,
			/*RFLD.RDB$FIELD_NAME*/
			jrd_1161.jrd_1162, /*RFLD.RDB$FIELD_NAME*/
  jrd_1161.jrd_1162);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1168);
	   }
	}
}


//----------------------


// Delete the records in RDB$FIELD_DIMENSIONS pertaining to a field.
bool DropDomainNode::deleteDimensionRecords(thread_db* tdbb, jrd_tra* transaction, const MetaName& name)
{
   struct {
          SSHORT jrd_1157;	// gds__utility 
   } jrd_1156;
   struct {
          SSHORT jrd_1155;	// gds__utility 
   } jrd_1154;
   struct {
          SSHORT jrd_1153;	// gds__utility 
   } jrd_1152;
   struct {
          TEXT  jrd_1151 [32];	// RDB$FIELD_NAME 
   } jrd_1150;
	AutoCacheRequest request(tdbb, drq_e_dims, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$FIELD_DIMENSIONS
		WITH X.RDB$FIELD_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1149, sizeof(jrd_1149));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1150.jrd_1151, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1150);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1152);
	   if (!jrd_1152.jrd_1153) break;
	{
		found = true;
		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1154);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1156);
	   }
	}

	return found;
}

void DropDomainNode::print(string& text) const
{
	text.printf(
		"DropDomainNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void DropDomainNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1137;	// gds__utility 
   } jrd_1136;
   struct {
          SSHORT jrd_1135;	// gds__utility 
   } jrd_1134;
   struct {
          SSHORT jrd_1133;	// gds__utility 
   } jrd_1132;
   struct {
          TEXT  jrd_1130 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1131;	// RDB$OBJECT_TYPE 
   } jrd_1129;
   struct {
          SSHORT jrd_1148;	// gds__utility 
   } jrd_1147;
   struct {
          SSHORT jrd_1146;	// gds__utility 
   } jrd_1145;
   struct {
          TEXT  jrd_1142 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_1143;	// gds__utility 
          SSHORT jrd_1144;	// gds__null_flag 
   } jrd_1141;
   struct {
          TEXT  jrd_1140 [32];	// RDB$FIELD_NAME 
   } jrd_1139;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_e_gfields, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$FIELDS
		WITH X.RDB$FIELD_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1138, sizeof(jrd_1138));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1139.jrd_1140, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1139);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_1141);
	   if (!jrd_1141.jrd_1143) break;
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_DROP_DOMAIN, name);

		check(tdbb, transaction);
		deleteDimensionRecords(tdbb, transaction, name);

		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1145);

		if (!/*X.RDB$SECURITY_CLASS.NULL*/
		     jrd_1141.jrd_1144)
			deleteSecurityClass(tdbb, transaction, /*X.RDB$SECURITY_CLASS*/
							       jrd_1141.jrd_1142);

		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1147);
	   }
	}

	request.reset(tdbb, drq_e_gfld_prvs, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH PRIV.RDB$RELATION_NAME EQ name.c_str() AND
			 PRIV.RDB$OBJECT_TYPE = obj_field*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1128, sizeof(jrd_1128));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1129.jrd_1130, 32);
	jrd_1129.jrd_1131 = obj_field;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_1129);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1132);
	   if (!jrd_1132.jrd_1133) break;
	{
		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1134);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1136);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_DOMAIN, name);
	else
	{
		// msg 89: "Domain not found"
		status_exception::raise(Arg::PrivateDyn(89));
	}

	savePoint.release();	// everything is ok
}

void DropDomainNode::check(thread_db* tdbb, jrd_tra* transaction)
{
   struct {
          TEXT  jrd_1104 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1105 [32];	// RDB$ARGUMENT_NAME 
          TEXT  jrd_1106 [32];	// RDB$FUNCTION_NAME 
          TEXT  jrd_1107 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_1108;	// gds__utility 
          SSHORT jrd_1109;	// gds__null_flag 
   } jrd_1103;
   struct {
          TEXT  jrd_1102 [32];	// RDB$FIELD_SOURCE 
   } jrd_1101;
   struct {
          TEXT  jrd_1114 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_1115 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_1116 [32];	// RDB$PROCEDURE_NAME 
          TEXT  jrd_1117 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_1118;	// gds__utility 
          SSHORT jrd_1119;	// gds__null_flag 
   } jrd_1113;
   struct {
          TEXT  jrd_1112 [32];	// RDB$FIELD_SOURCE 
   } jrd_1111;
   struct {
          TEXT  jrd_1124 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_1125 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_1126 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_1127;	// gds__utility 
   } jrd_1123;
   struct {
          TEXT  jrd_1122 [32];	// RDB$FIELD_SOURCE 
   } jrd_1121;
	AutoCacheRequest request(tdbb, drq_l_fld_src, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		Y IN RDB$RELATION_FIELDS
		WITH Y.RDB$FIELD_SOURCE EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1120, sizeof(jrd_1120));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1121.jrd_1122, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1121);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 98, (UCHAR*) &jrd_1123);
	   if (!jrd_1123.jrd_1127) break;
	{
		fb_utils::exact_name_limit(/*Y.RDB$FIELD_SOURCE*/
					   jrd_1123.jrd_1126, sizeof(/*Y.RDB$FIELD_SOURCE*/
	 jrd_1123.jrd_1126));
		fb_utils::exact_name_limit(/*Y.RDB$RELATION_NAME*/
					   jrd_1123.jrd_1125, sizeof(/*Y.RDB$RELATION_NAME*/
	 jrd_1123.jrd_1125));
		fb_utils::exact_name_limit(/*Y.RDB$FIELD_NAME*/
					   jrd_1123.jrd_1124, sizeof(/*Y.RDB$FIELD_NAME*/
	 jrd_1123.jrd_1124));

		// msg 43: "Domain %s is used in table %s (local name %s) and can not be dropped"
		status_exception::raise(
			Arg::PrivateDyn(43) << /*Y.RDB$FIELD_SOURCE*/
					       jrd_1123.jrd_1126 << /*Y.RDB$RELATION_NAME*/
    jrd_1123.jrd_1125 << /*Y.RDB$FIELD_NAME*/
    jrd_1123.jrd_1124);
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, drq_l_prp_src, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$PROCEDURE_PARAMETERS
		WITH X.RDB$FIELD_SOURCE EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1110, sizeof(jrd_1110));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1111.jrd_1112, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1111);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 132, (UCHAR*) &jrd_1113);
	   if (!jrd_1113.jrd_1118) break;
	{
		fb_utils::exact_name_limit(/*X.RDB$FIELD_SOURCE*/
					   jrd_1113.jrd_1117, sizeof(/*X.RDB$FIELD_SOURCE*/
	 jrd_1113.jrd_1117));
		fb_utils::exact_name_limit(/*X.RDB$PROCEDURE_NAME*/
					   jrd_1113.jrd_1116, sizeof(/*X.RDB$PROCEDURE_NAME*/
	 jrd_1113.jrd_1116));
		fb_utils::exact_name_limit(/*X.RDB$PARAMETER_NAME*/
					   jrd_1113.jrd_1115, sizeof(/*X.RDB$PARAMETER_NAME*/
	 jrd_1113.jrd_1115));

		// msg 239: "Domain %s is used in procedure %s (parameter name %s) and cannot be dropped"
		status_exception::raise(
			Arg::PrivateDyn(239) << /*X.RDB$FIELD_SOURCE*/
						jrd_1113.jrd_1117 <<
				QualifiedName(/*X.RDB$PROCEDURE_NAME*/
					      jrd_1113.jrd_1116,
					(/*X.RDB$PACKAGE_NAME.NULL*/
					 jrd_1113.jrd_1119 ? NULL : /*X.RDB$PACKAGE_NAME*/
	  jrd_1113.jrd_1114)).toString().c_str() <<
				/*X.RDB$PARAMETER_NAME*/
				jrd_1113.jrd_1115);
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, drq_l_arg_src, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$FUNCTION_ARGUMENTS
		WITH X.RDB$FIELD_SOURCE EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1100, sizeof(jrd_1100));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1101.jrd_1102, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1101);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 132, (UCHAR*) &jrd_1103);
	   if (!jrd_1103.jrd_1108) break;
	{
		fb_utils::exact_name_limit(/*X.RDB$FIELD_SOURCE*/
					   jrd_1103.jrd_1107, sizeof(/*X.RDB$FIELD_SOURCE*/
	 jrd_1103.jrd_1107));
		fb_utils::exact_name_limit(/*X.RDB$FUNCTION_NAME*/
					   jrd_1103.jrd_1106, sizeof(/*X.RDB$FUNCTION_NAME*/
	 jrd_1103.jrd_1106));
		fb_utils::exact_name_limit(/*X.RDB$ARGUMENT_NAME*/
					   jrd_1103.jrd_1105, sizeof(/*X.RDB$ARGUMENT_NAME*/
	 jrd_1103.jrd_1105));

		// msg 239: "Domain %s is used in function %s (parameter name %s) and cannot be dropped"
		status_exception::raise(
			Arg::Gds(isc_dyn_domain_used_function) << /*X.RDB$FIELD_SOURCE*/
								  jrd_1103.jrd_1107 <<
				QualifiedName(/*X.RDB$FUNCTION_NAME*/
					      jrd_1103.jrd_1106,
					(/*X.RDB$PACKAGE_NAME.NULL*/
					 jrd_1103.jrd_1109 ? NULL : /*X.RDB$PACKAGE_NAME*/
	  jrd_1103.jrd_1104)).toString().c_str() <<
				/*X.RDB$ARGUMENT_NAME*/
				jrd_1103.jrd_1105);
	}
	/*END_FOR*/
	   }
	}
}


//----------------------


void CreateAlterExceptionNode::print(string& text) const
{
	text.printf(
		"CreateAlterExceptionNode\n"
		"  name: '%s'  create: %d  alter: %d\n"
		"  message: '%s'\n",
		name.c_str(), create, alter, message.c_str());
}

void CreateAlterExceptionNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
	fb_assert(create || alter);

	if (message.length() > XCP_MESSAGE_LENGTH)
		status_exception::raise(Arg::Gds(isc_dyn_name_longer));

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	if (alter)
	{
		if (!executeAlter(tdbb, dsqlScratch, transaction))
		{
			if (create)	// create or alter
				executeCreate(tdbb, dsqlScratch, transaction);
			else
			{
				// msg 144: "Exception not found"
				status_exception::raise(Arg::PrivateDyn(144));
			}
		}
	}
	else
		executeCreate(tdbb, dsqlScratch, transaction);

	savePoint.release();	// everything is ok
}

void CreateAlterExceptionNode::executeCreate(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          TEXT  jrd_1094 [1024];	// RDB$MESSAGE 
          TEXT  jrd_1095 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_1096 [32];	// RDB$EXCEPTION_NAME 
          SLONG  jrd_1097;	// RDB$EXCEPTION_NUMBER 
          SSHORT jrd_1098;	// gds__null_flag 
          SSHORT jrd_1099;	// RDB$SYSTEM_FLAG 
   } jrd_1093;
	Attachment* const attachment = transaction->getAttachment();
	const string& userName = attachment->att_user->usr_user_name;

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
		DDL_TRIGGER_CREATE_EXCEPTION, name);

	DYN_UTIL_check_unique_name(tdbb, transaction, name, obj_exception);

	AutoCacheRequest request(tdbb, drq_s_xcp, DYN_REQUESTS);
	int faults = 0;

	while (true)
	{
		try
		{
			SINT64 id = DYN_UTIL_gen_unique_id(tdbb, drq_g_nxt_xcp_id, "RDB$EXCEPTIONS");
			id %= (MAX_SSHORT + 1);

			if (!id)
				continue;

			/*STORE (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
				X IN RDB$EXCEPTIONS*/
			{
			
			{
				/*X.RDB$EXCEPTION_NUMBER*/
				jrd_1093.jrd_1097 = id;
				/*X.RDB$SYSTEM_FLAG*/
				jrd_1093.jrd_1099 = 0;
				strcpy(/*X.RDB$EXCEPTION_NAME*/
				       jrd_1093.jrd_1096, name.c_str());

				/*X.RDB$OWNER_NAME.NULL*/
				jrd_1093.jrd_1098 = FALSE;
				strcpy(/*X.RDB$OWNER_NAME*/
				       jrd_1093.jrd_1095, userName.c_str());

				strcpy(/*X.RDB$MESSAGE*/
				       jrd_1093.jrd_1094, message.c_str());
			}
			/*END_STORE*/
			request.compile(tdbb, (UCHAR*) jrd_1092, sizeof(jrd_1092));
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 1096, (UCHAR*) &jrd_1093);
			}

			break;
		}
		catch (const status_exception& ex)
		{
			if (ex.value()[1] != isc_unique_key_violation)
				throw;

			if (++faults > MAX_SSHORT)
				throw;

			fb_utils::init_status(tdbb->tdbb_status_vector);
		}
	}

	storePrivileges(tdbb, transaction, name, obj_exception, USAGE_PRIVILEGES);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_CREATE_EXCEPTION, name);
}

bool CreateAlterExceptionNode::executeAlter(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1091;	// gds__utility 
   } jrd_1090;
   struct {
          TEXT  jrd_1089 [1024];	// RDB$MESSAGE 
   } jrd_1088;
   struct {
          TEXT  jrd_1086 [1024];	// RDB$MESSAGE 
          SSHORT jrd_1087;	// gds__utility 
   } jrd_1085;
   struct {
          TEXT  jrd_1084 [32];	// RDB$EXCEPTION_NAME 
   } jrd_1083;
	AutoCacheRequest request(tdbb, drq_m_xcp, DYN_REQUESTS);
	bool modified = false;

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$EXCEPTIONS
		WITH X.RDB$EXCEPTION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1082, sizeof(jrd_1082));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1083.jrd_1084, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1083);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 1026, (UCHAR*) &jrd_1085);
	   if (!jrd_1085.jrd_1087) break;
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_ALTER_EXCEPTION, name);

		/*MODIFY X*/
		{
		
			strcpy(/*X.RDB$MESSAGE*/
			       jrd_1085.jrd_1086, message.c_str());

			modified = true;
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_1085.jrd_1086, (char*) jrd_1088.jrd_1089, 1024);
		EXE_send (tdbb, request, 2, 1024, (UCHAR*) &jrd_1088);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1090);
	   }
	}

	if (modified)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_ALTER_EXCEPTION, name);

	return modified;
}


//----------------------


void DropExceptionNode::print(string& text) const
{
	text.printf(
		"DropExceptionNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void DropExceptionNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1070;	// gds__utility 
   } jrd_1069;
   struct {
          SSHORT jrd_1068;	// gds__utility 
   } jrd_1067;
   struct {
          SSHORT jrd_1066;	// gds__utility 
   } jrd_1065;
   struct {
          TEXT  jrd_1063 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1064;	// RDB$OBJECT_TYPE 
   } jrd_1062;
   struct {
          SSHORT jrd_1081;	// gds__utility 
   } jrd_1080;
   struct {
          SSHORT jrd_1079;	// gds__utility 
   } jrd_1078;
   struct {
          TEXT  jrd_1075 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_1076;	// gds__utility 
          SSHORT jrd_1077;	// gds__null_flag 
   } jrd_1074;
   struct {
          TEXT  jrd_1073 [32];	// RDB$EXCEPTION_NAME 
   } jrd_1072;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_e_xcp, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$EXCEPTIONS
	    WITH X.RDB$EXCEPTION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1071, sizeof(jrd_1071));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1072.jrd_1073, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1072);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_1074);
	   if (!jrd_1074.jrd_1076) break;
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_DROP_EXCEPTION, name);
		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1078);

		if (!/*X.RDB$SECURITY_CLASS.NULL*/
		     jrd_1074.jrd_1077)
			deleteSecurityClass(tdbb, transaction, /*X.RDB$SECURITY_CLASS*/
							       jrd_1074.jrd_1075);

		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1080);
	   }
	}

	request.reset(tdbb, drq_e_xcp_prvs, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH PRIV.RDB$RELATION_NAME EQ name.c_str() AND
			 PRIV.RDB$OBJECT_TYPE = obj_exception*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1061, sizeof(jrd_1061));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1062.jrd_1063, 32);
	jrd_1062.jrd_1064 = obj_exception;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_1062);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1065);
	   if (!jrd_1065.jrd_1066) break;
	{
		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1067);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1069);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_EXCEPTION, name);
	else if (!silent)
	{
		// msg 144: "Exception not found"
		status_exception::raise(Arg::PrivateDyn(144));
	}

	savePoint.release();	// everything is ok
}


//----------------------


void CreateAlterSequenceNode::print(string& text) const
{
	text.printf(
		"CreateAlterSequenceNode\n"
		"  name: %s\n",
		name.c_str());
}

void CreateAlterSequenceNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
	fb_assert(create || alter);

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	if (alter)
	{
		if (!executeAlter(tdbb, dsqlScratch, transaction))
		{
			if (create)	// create or alter
				executeCreate(tdbb, dsqlScratch, transaction);
			else
			{
				// msg 214: "Sequence not found"
				status_exception::raise(Arg::PrivateDyn(214) << name);
			}
		}
	}
	else
		executeCreate(tdbb, dsqlScratch, transaction);

	savePoint.release();	// everything is ok

}

void CreateAlterSequenceNode::putErrorPrefix(Firebird::Arg::StatusVector& statusVector)
{
	// Possibilities I see in parse.y
	//CREATE SEQ -> !legacy, create, !alter
	//REPL SEQ (create or alter) -> !legacy, create, alter
	//ALTER SEQ -> !legacy, !create, alter
	//SET GENERATOR -> legacy, !create, alter
	ISC_STATUS rc = 0;
	if (legacy)
	{
		if (alter)
			rc = isc_dsql_set_generator_failed;
		else
			rc = isc_dsql_create_sequence_failed; // no way to distinguish
	}
	else
	{
		if (alter)
			rc = isc_dsql_alter_sequence_failed;
		else
			rc = isc_dsql_create_sequence_failed;
	}
	statusVector << Firebird::Arg::Gds(rc) << name;
}

void CreateAlterSequenceNode::executeCreate(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_CREATE_SEQUENCE, name);

	const SINT64 val = value.specified ? value.value : 0;
	SLONG initialStep = 1;
	if (step.specified)
	{
		initialStep = step.value;
		if (initialStep == 0)
			status_exception::raise(Arg::Gds(isc_dyn_cant_use_zero_increment) << Arg::Str(name));
	}
	store(tdbb, transaction, name, fb_sysflag_user, val, initialStep);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_CREATE_SEQUENCE, name);
}

bool CreateAlterSequenceNode::executeAlter(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1060;	// gds__utility 
   } jrd_1059;
   struct {
          SLONG  jrd_1058;	// RDB$GENERATOR_INCREMENT 
   } jrd_1057;
   struct {
          ISC_INT64  jrd_1051;	// RDB$INITIAL_VALUE 
          SLONG  jrd_1052;	// RDB$GENERATOR_INCREMENT 
          SSHORT jrd_1053;	// gds__utility 
          SSHORT jrd_1054;	// gds__null_flag 
          SSHORT jrd_1055;	// RDB$GENERATOR_ID 
          SSHORT jrd_1056;	// RDB$SYSTEM_FLAG 
   } jrd_1050;
   struct {
          TEXT  jrd_1049 [32];	// RDB$GENERATOR_NAME 
   } jrd_1048;
	bool forbidden = false;

	if (legacy)
	{
		// The only need for this code is that for the sake of backward compatibility
		// SET GENERATOR is still described as isc_info_sql_stmt_set_generator and ISQL
		// treats it as DML thus executing it in a separate transaction. So we need to ensure
		// that the generator created in another transaction can be found here. This is done
		// using MET_lookup_generator() which works in the system transaction.

		SLONG oldStep = 0;
		const SLONG id = MET_lookup_generator(tdbb, name, &forbidden, &oldStep);
		if (id < 0)
			return false;

		if (forbidden && !tdbb->getAttachment()->isRWGbak())
			status_exception::raise(Arg::Gds(isc_dyn_cant_modify_sysobj) << "generator" << Arg::Str(name));

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_ALTER_SEQUENCE, name);

		fb_assert(restartSpecified && value.specified);
		const SINT64 val = value.specified ? value.value : 0;
		if (step.specified)
		{
			const SLONG newStep = step.value;
			if (newStep == 0)
				status_exception::raise(Arg::Gds(isc_dyn_cant_use_zero_increment) << Arg::Str(name));

			// Perhaps it's better to move this to DFW?
			if (newStep != oldStep)
				MET_update_generator_increment(tdbb, id, newStep);
		}

		transaction->getGenIdCache()->put(id, val);
		dsc desc;
		desc.makeText((USHORT) name.length(), ttype_metadata, (UCHAR*) name.c_str());
		DFW_post_work(transaction, dfw_set_generator, &desc, id);

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_ALTER_SEQUENCE, name);

		return true;
	}

	AutoCacheRequest request(tdbb, drq_l_gens, DYN_REQUESTS);

	bool found = false;

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$GENERATORS
		WITH X.RDB$GENERATOR_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1047, sizeof(jrd_1047));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1048.jrd_1049, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1048);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 20, (UCHAR*) &jrd_1050);
	   if (!jrd_1050.jrd_1053) break;
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_ALTER_SEQUENCE, name);

		if (/*X.RDB$SYSTEM_FLAG*/
		    jrd_1050.jrd_1056 == fb_sysflag_system)
		{
			forbidden = true;
			break;
		}

		const SLONG id = /*X.RDB$GENERATOR_ID*/
				 jrd_1050.jrd_1055;

		if (step.specified)
		{
			const SLONG newStep = step.value;
			if (newStep == 0)
				status_exception::raise(Arg::Gds(isc_dyn_cant_use_zero_increment) << Arg::Str(name));

			if (newStep != /*X.RDB$GENERATOR_INCREMENT*/
				       jrd_1050.jrd_1052)
			{
				/*MODIFY X*/
				{
				
					/*X.RDB$GENERATOR_INCREMENT*/
					jrd_1050.jrd_1052 = newStep;
				/*END_MODIFY*/
				jrd_1057.jrd_1058 = jrd_1050.jrd_1052;
				EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_1057);
				}
			}
		}

		if (restartSpecified)
		{
			const SINT64 val = value.specified ?
				value.value : (!/*X.RDB$INITIAL_VALUE.NULL*/
						jrd_1050.jrd_1054 ? /*X.RDB$INITIAL_VALUE*/
   jrd_1050.jrd_1051 : 0);
			transaction->getGenIdCache()->put(id, val);
		}

		dsc desc;
		desc.makeText((USHORT) name.length(), ttype_metadata, (UCHAR*) name.c_str());
		DFW_post_work(transaction, dfw_set_generator, &desc, id);

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_ALTER_SEQUENCE, name);

		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1059);
	   }
	}

	if (forbidden)
		status_exception::raise(Arg::Gds(isc_dyn_cant_modify_sysobj) << "generator" << Arg::Str(name));

	return found;
}

SSHORT CreateAlterSequenceNode::store(thread_db* tdbb, jrd_tra* transaction, const MetaName& name,
	fb_sysflag sysFlag, SINT64 val, SLONG step)
{
   struct {
          ISC_INT64  jrd_1039;	// RDB$INITIAL_VALUE 
          TEXT  jrd_1040 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_1041 [32];	// RDB$GENERATOR_NAME 
          SLONG  jrd_1042;	// RDB$GENERATOR_INCREMENT 
          SSHORT jrd_1043;	// gds__null_flag 
          SSHORT jrd_1044;	// gds__null_flag 
          SSHORT jrd_1045;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_1046;	// RDB$GENERATOR_ID 
   } jrd_1038;
	Attachment* const attachment = transaction->tra_attachment;
	const string& userName = attachment->att_user->usr_user_name;

	DYN_UTIL_check_unique_name(tdbb, transaction, name, obj_generator);

	AutoCacheRequest request(tdbb, drq_s_gens, DYN_REQUESTS);
	int faults = 0;
	SSHORT storedId = -1;

	while (true)
	{
		try
		{
			SINT64 id = DYN_UTIL_gen_unique_id(tdbb, drq_g_nxt_gen_id, MASTER_GENERATOR);

			id %= MAX_SSHORT + 1;
			if (id == 0)
				continue;

			/*STORE (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
				X IN RDB$GENERATORS*/
			{
			
			{
				/*X.RDB$GENERATOR_ID*/
				jrd_1038.jrd_1046 = id;
				/*X.RDB$SYSTEM_FLAG*/
				jrd_1038.jrd_1045 = (SSHORT) sysFlag;
				strcpy(/*X.RDB$GENERATOR_NAME*/
				       jrd_1038.jrd_1041, name.c_str());

				/*X.RDB$OWNER_NAME.NULL*/
				jrd_1038.jrd_1044 = FALSE;
				strcpy(/*X.RDB$OWNER_NAME*/
				       jrd_1038.jrd_1040, userName.c_str());

				/*X.RDB$INITIAL_VALUE.NULL*/
				jrd_1038.jrd_1043 = FALSE;
				/*X.RDB$INITIAL_VALUE*/
				jrd_1038.jrd_1039 = val;

				/*X.RDB$GENERATOR_INCREMENT*/
				jrd_1038.jrd_1042 = step;
			}
			/*END_STORE*/
			request.compile(tdbb, (UCHAR*) jrd_1037, sizeof(jrd_1037));
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 84, (UCHAR*) &jrd_1038);
			}

			storedId = id;
			break;
		}
		catch (const status_exception& ex)
		{
			if (ex.value()[1] != isc_unique_key_violation)
				throw;

			if (++faults > MAX_SSHORT)
				throw;

			fb_utils::init_status(tdbb->tdbb_status_vector);
		}
	}

	storePrivileges(tdbb, transaction, name, obj_generator, USAGE_PRIVILEGES);

	// The STORE above has caused the DFW item to be posted, so we just adjust the cached
	// generator value.
	transaction->getGenIdCache()->put(storedId, val);

	return storedId;
}


//----------------------


void DropSequenceNode::print(string& text) const
{
	text.printf(
		"DropSequenceNode\n"
		"  name: %s\n",
		name.c_str());
}

void DropSequenceNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1024;	// gds__utility 
   } jrd_1023;
   struct {
          SSHORT jrd_1022;	// gds__utility 
   } jrd_1021;
   struct {
          SSHORT jrd_1020;	// gds__utility 
   } jrd_1019;
   struct {
          TEXT  jrd_1017 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_1018;	// RDB$OBJECT_TYPE 
   } jrd_1016;
   struct {
          SSHORT jrd_1036;	// gds__utility 
   } jrd_1035;
   struct {
          SSHORT jrd_1034;	// gds__utility 
   } jrd_1033;
   struct {
          TEXT  jrd_1029 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_1030;	// gds__utility 
          SSHORT jrd_1031;	// gds__null_flag 
          SSHORT jrd_1032;	// RDB$SYSTEM_FLAG 
   } jrd_1028;
   struct {
          TEXT  jrd_1027 [32];	// RDB$GENERATOR_NAME 
   } jrd_1026;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_e_gens, DYN_REQUESTS);
	bool found = false;

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		GEN IN RDB$GENERATORS
		WITH GEN.RDB$GENERATOR_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1025, sizeof(jrd_1025));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1026.jrd_1027, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1026);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 38, (UCHAR*) &jrd_1028);
	   if (!jrd_1028.jrd_1030) break;
	{
		if (/*GEN.RDB$SYSTEM_FLAG*/
		    jrd_1028.jrd_1032 != 0)
		{
			// msg 272: "Cannot delete system generator @1"
			status_exception::raise(Arg::PrivateDyn(272) << name);
		}

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_DROP_SEQUENCE, name);

		/*ERASE GEN;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1033);

		if (!/*GEN.RDB$SECURITY_CLASS.NULL*/
		     jrd_1028.jrd_1031)
			deleteSecurityClass(tdbb, transaction, /*GEN.RDB$SECURITY_CLASS*/
							       jrd_1028.jrd_1029);

		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1035);
	   }
	}

	request.reset(tdbb, drq_e_gen_prvs, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH PRIV.RDB$RELATION_NAME EQ name.c_str() AND
			 PRIV.RDB$OBJECT_TYPE = obj_generator*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1015, sizeof(jrd_1015));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1016.jrd_1017, 32);
	jrd_1016.jrd_1018 = obj_generator;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_1016);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1019);
	   if (!jrd_1019.jrd_1020) break;
	{
		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1021);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1023);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_SEQUENCE, name);
	else if (!silent)
		status_exception::raise(Arg::Gds(isc_gennotdef) << Arg::Str(name));

	savePoint.release();	// everything is ok
}


// Delete a record from RDB$GENERATORS, without verifying RDB$SYSTEM_FLAG.
void DropSequenceNode::deleteIdentity(thread_db* tdbb, jrd_tra* transaction, const MetaName& name)
{
   struct {
          SSHORT jrd_1014;	// gds__utility 
   } jrd_1013;
   struct {
          SSHORT jrd_1012;	// gds__utility 
   } jrd_1011;
   struct {
          SSHORT jrd_1010;	// gds__utility 
   } jrd_1009;
   struct {
          TEXT  jrd_1008 [32];	// RDB$GENERATOR_NAME 
   } jrd_1007;
	AutoCacheRequest request(tdbb, drq_e_ident_gens, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		GEN IN RDB$GENERATORS
		WITH GEN.RDB$GENERATOR_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_1006, sizeof(jrd_1006));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1007.jrd_1008, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_1007);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_1009);
	   if (!jrd_1009.jrd_1010) break;
	{
		/*ERASE GEN;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_1011);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1013);
	   }
	}
}


//----------------------


RelationNode::RelationNode(MemoryPool& p, RelationSourceNode* aDsqlNode)
	: DdlNode(p),
	  dsqlNode(aDsqlNode),
	  name(p, dsqlNode->dsqlName),
	  clauses(p)
{
}

void RelationNode::FieldDefinition::modify(thread_db* tdbb, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_1005;	// gds__utility 
   } jrd_1004;
   struct {
          TEXT  jrd_985 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_986 [32];	// RDB$GENERATOR_NAME 
          bid  jrd_987;	// RDB$DEFAULT_SOURCE 
          bid  jrd_988;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_989 [32];	// RDB$BASE_FIELD 
          SSHORT jrd_990;	// gds__null_flag 
          SSHORT jrd_991;	// RDB$COLLATION_ID 
          SSHORT jrd_992;	// gds__null_flag 
          SSHORT jrd_993;	// gds__null_flag 
          SSHORT jrd_994;	// RDB$IDENTITY_TYPE 
          SSHORT jrd_995;	// gds__null_flag 
          SSHORT jrd_996;	// RDB$NULL_FLAG 
          SSHORT jrd_997;	// gds__null_flag 
          SSHORT jrd_998;	// gds__null_flag 
          SSHORT jrd_999;	// gds__null_flag 
          SSHORT jrd_1000;	// RDB$FIELD_POSITION 
          SSHORT jrd_1001;	// gds__null_flag 
          SSHORT jrd_1002;	// RDB$VIEW_CONTEXT 
          SSHORT jrd_1003;	// gds__null_flag 
   } jrd_984;
   struct {
          TEXT  jrd_964 [32];	// RDB$BASE_FIELD 
          bid  jrd_965;	// RDB$DEFAULT_VALUE 
          bid  jrd_966;	// RDB$DEFAULT_SOURCE 
          TEXT  jrd_967 [32];	// RDB$GENERATOR_NAME 
          TEXT  jrd_968 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_969;	// gds__utility 
          SSHORT jrd_970;	// gds__null_flag 
          SSHORT jrd_971;	// gds__null_flag 
          SSHORT jrd_972;	// RDB$VIEW_CONTEXT 
          SSHORT jrd_973;	// gds__null_flag 
          SSHORT jrd_974;	// RDB$FIELD_POSITION 
          SSHORT jrd_975;	// gds__null_flag 
          SSHORT jrd_976;	// gds__null_flag 
          SSHORT jrd_977;	// gds__null_flag 
          SSHORT jrd_978;	// RDB$NULL_FLAG 
          SSHORT jrd_979;	// gds__null_flag 
          SSHORT jrd_980;	// RDB$IDENTITY_TYPE 
          SSHORT jrd_981;	// gds__null_flag 
          SSHORT jrd_982;	// gds__null_flag 
          SSHORT jrd_983;	// RDB$COLLATION_ID 
   } jrd_963;
   struct {
          TEXT  jrd_961 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_962 [32];	// RDB$RELATION_NAME 
   } jrd_960;
	AutoRequest request;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		RFR IN RDB$RELATION_FIELDS
		WITH RFR.RDB$RELATION_NAME EQ relationName.c_str() AND
			 RFR.RDB$FIELD_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_959, sizeof(jrd_959));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_960.jrd_961, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_960.jrd_962, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_960);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 142, (UCHAR*) &jrd_963);
	   if (!jrd_963.jrd_969) break;
	{
		// ASF: This is prepared only to modify view fields!

		/*MODIFY RFR*/
		{
		
			strcpy(/*RFR.RDB$FIELD_SOURCE*/
			       jrd_963.jrd_968, fieldSource.c_str());

			/*RFR.RDB$COLLATION_ID.NULL*/
			jrd_963.jrd_982 = TRUE;
			/*RFR.RDB$GENERATOR_NAME.NULL*/
			jrd_963.jrd_981 = TRUE;
			/*RFR.RDB$IDENTITY_TYPE.NULL*/
			jrd_963.jrd_979 = TRUE;
			/*RFR.RDB$NULL_FLAG.NULL*/
			jrd_963.jrd_977 = TRUE;
			/*RFR.RDB$DEFAULT_SOURCE.NULL*/
			jrd_963.jrd_976 = TRUE;
			/*RFR.RDB$DEFAULT_VALUE.NULL*/
			jrd_963.jrd_975 = TRUE;
			/*RFR.RDB$FIELD_POSITION.NULL*/
			jrd_963.jrd_973 = TRUE;

			/*RFR.RDB$VIEW_CONTEXT.NULL*/
			jrd_963.jrd_971 = TRUE;
			/*RFR.RDB$BASE_FIELD.NULL*/
			jrd_963.jrd_970 = TRUE;
			///RFR.RDB$UPDATE_FLAG.NULL = TRUE;

			if (collationId.specified)
			{
				/*RFR.RDB$COLLATION_ID.NULL*/
				jrd_963.jrd_982 = FALSE;
				/*RFR.RDB$COLLATION_ID*/
				jrd_963.jrd_983 = collationId.value;
			}

			SLONG fieldPos = -1;

			if (position.specified)
				fieldPos = position.value;
			else
			{
				DYN_UTIL_generate_field_position(tdbb, relationName, &fieldPos);
				if (fieldPos >= 0)
					++fieldPos;
			}

			if (fieldPos >= 0)
			{
				/*RFR.RDB$FIELD_POSITION.NULL*/
				jrd_963.jrd_973 = FALSE;
				/*RFR.RDB$FIELD_POSITION*/
				jrd_963.jrd_974 = SSHORT(fieldPos);
			}

			if (baseField.hasData())
			{
				/*RFR.RDB$BASE_FIELD.NULL*/
				jrd_963.jrd_970 = FALSE;
				strcpy(/*RFR.RDB$BASE_FIELD*/
				       jrd_963.jrd_964, baseField.c_str());
			}

			if (viewContext.specified)
			{
				fb_assert(baseField.hasData());

				/*RFR.RDB$VIEW_CONTEXT.NULL*/
				jrd_963.jrd_971 = FALSE;
				/*RFR.RDB$VIEW_CONTEXT*/
				jrd_963.jrd_972 = viewContext.value;

				DYN_UTIL_find_field_source(tdbb, transaction, relationName, viewContext.value,
					baseField.c_str(), /*RFR.RDB$FIELD_SOURCE*/
							   jrd_963.jrd_968);
			}
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_963.jrd_968, (char*) jrd_984.jrd_985, 32);
		gds__vtov((const char*) jrd_963.jrd_967, (char*) jrd_984.jrd_986, 32);
		jrd_984.jrd_987 = jrd_963.jrd_966;
		jrd_984.jrd_988 = jrd_963.jrd_965;
		gds__vtov((const char*) jrd_963.jrd_964, (char*) jrd_984.jrd_989, 32);
		jrd_984.jrd_990 = jrd_963.jrd_982;
		jrd_984.jrd_991 = jrd_963.jrd_983;
		jrd_984.jrd_992 = jrd_963.jrd_981;
		jrd_984.jrd_993 = jrd_963.jrd_979;
		jrd_984.jrd_994 = jrd_963.jrd_980;
		jrd_984.jrd_995 = jrd_963.jrd_977;
		jrd_984.jrd_996 = jrd_963.jrd_978;
		jrd_984.jrd_997 = jrd_963.jrd_976;
		jrd_984.jrd_998 = jrd_963.jrd_975;
		jrd_984.jrd_999 = jrd_963.jrd_973;
		jrd_984.jrd_1000 = jrd_963.jrd_974;
		jrd_984.jrd_1001 = jrd_963.jrd_971;
		jrd_984.jrd_1002 = jrd_963.jrd_972;
		jrd_984.jrd_1003 = jrd_963.jrd_970;
		EXE_send (tdbb, request, 2, 140, (UCHAR*) &jrd_984);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_1004);
	   }
	}
}

void RelationNode::FieldDefinition::store(thread_db* tdbb, jrd_tra* transaction)
{
   struct {
          TEXT  jrd_937 [32];	// RDB$BASE_FIELD 
          bid  jrd_938;	// RDB$DEFAULT_VALUE 
          bid  jrd_939;	// RDB$DEFAULT_SOURCE 
          TEXT  jrd_940 [32];	// RDB$GENERATOR_NAME 
          TEXT  jrd_941 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_942 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_943 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_944;	// gds__null_flag 
          SSHORT jrd_945;	// gds__null_flag 
          SSHORT jrd_946;	// RDB$VIEW_CONTEXT 
          SSHORT jrd_947;	// gds__null_flag 
          SSHORT jrd_948;	// RDB$FIELD_POSITION 
          SSHORT jrd_949;	// gds__null_flag 
          SSHORT jrd_950;	// gds__null_flag 
          SSHORT jrd_951;	// gds__null_flag 
          SSHORT jrd_952;	// RDB$NULL_FLAG 
          SSHORT jrd_953;	// gds__null_flag 
          SSHORT jrd_954;	// RDB$IDENTITY_TYPE 
          SSHORT jrd_955;	// gds__null_flag 
          SSHORT jrd_956;	// gds__null_flag 
          SSHORT jrd_957;	// RDB$COLLATION_ID 
          SSHORT jrd_958;	// RDB$SYSTEM_FLAG 
   } jrd_936;
	Attachment* const attachment = transaction->tra_attachment;

	AutoCacheRequest request(tdbb, drq_s_lfields, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		RFR IN RDB$RELATION_FIELDS*/
	{
	
	{
		strcpy(/*RFR.RDB$FIELD_NAME*/
		       jrd_936.jrd_943, name.c_str());
		strcpy(/*RFR.RDB$RELATION_NAME*/
		       jrd_936.jrd_942, relationName.c_str());
		strcpy(/*RFR.RDB$FIELD_SOURCE*/
		       jrd_936.jrd_941, fieldSource.c_str());
		/*RFR.RDB$SYSTEM_FLAG*/
		jrd_936.jrd_958 = 0;

		/*RFR.RDB$COLLATION_ID.NULL*/
		jrd_936.jrd_956 = TRUE;
		/*RFR.RDB$GENERATOR_NAME.NULL*/
		jrd_936.jrd_955 = TRUE;
		/*RFR.RDB$IDENTITY_TYPE.NULL*/
		jrd_936.jrd_953 = TRUE;
		/*RFR.RDB$NULL_FLAG.NULL*/
		jrd_936.jrd_951 = TRUE;
		/*RFR.RDB$DEFAULT_SOURCE.NULL*/
		jrd_936.jrd_950 = TRUE;
		/*RFR.RDB$DEFAULT_VALUE.NULL*/
		jrd_936.jrd_949 = TRUE;
		/*RFR.RDB$FIELD_POSITION.NULL*/
		jrd_936.jrd_947 = TRUE;

		/*RFR.RDB$VIEW_CONTEXT.NULL*/
		jrd_936.jrd_945 = TRUE;
		/*RFR.RDB$BASE_FIELD.NULL*/
		jrd_936.jrd_944 = TRUE;
		///RFR.RDB$UPDATE_FLAG.NULL = TRUE;

		if (collationId.specified)
		{
			/*RFR.RDB$COLLATION_ID.NULL*/
			jrd_936.jrd_956 = FALSE;
			/*RFR.RDB$COLLATION_ID*/
			jrd_936.jrd_957 = collationId.value;
		}

		if (identitySequence.hasData())
		{
			/*RFR.RDB$GENERATOR_NAME.NULL*/
			jrd_936.jrd_955 = FALSE;
			strcpy(/*RFR.RDB$GENERATOR_NAME*/
			       jrd_936.jrd_940, identitySequence.c_str());

			/*RFR.RDB$IDENTITY_TYPE.NULL*/
			jrd_936.jrd_953 = FALSE;
			/*RFR.RDB$IDENTITY_TYPE*/
			jrd_936.jrd_954 = IDENT_TYPE_BY_DEFAULT;
		}

		if (notNullFlag.specified)
		{
			/*RFR.RDB$NULL_FLAG.NULL*/
			jrd_936.jrd_951 = FALSE;
			/*RFR.RDB$NULL_FLAG*/
			jrd_936.jrd_952 = notNullFlag.value;
		}

		if (defaultSource.hasData())
		{
			/*RFR.RDB$DEFAULT_SOURCE.NULL*/
			jrd_936.jrd_950 = FALSE;
			attachment->storeMetaDataBlob(tdbb, transaction, &/*RFR.RDB$DEFAULT_SOURCE*/
									  jrd_936.jrd_939,
				defaultSource);
		}

		if (defaultValue.length > 0)
		{
			/*RFR.RDB$DEFAULT_VALUE.NULL*/
			jrd_936.jrd_949 = FALSE;
			attachment->storeBinaryBlob(tdbb, transaction, &/*RFR.RDB$DEFAULT_VALUE*/
									jrd_936.jrd_938, defaultValue);
		}

		SLONG fieldPos = -1;

		if (position.specified)
			fieldPos = position.value;
		else
		{
			DYN_UTIL_generate_field_position(tdbb, relationName, &fieldPos);
			if (fieldPos >= 0)
				++fieldPos;
		}

		if (fieldPos >= 0)
		{
			/*RFR.RDB$FIELD_POSITION.NULL*/
			jrd_936.jrd_947 = FALSE;
			/*RFR.RDB$FIELD_POSITION*/
			jrd_936.jrd_948 = SSHORT(fieldPos);
		}

		if (baseField.hasData())
		{
			/*RFR.RDB$BASE_FIELD.NULL*/
			jrd_936.jrd_944 = FALSE;
			strcpy(/*RFR.RDB$BASE_FIELD*/
			       jrd_936.jrd_937, baseField.c_str());
		}

		if (viewContext.specified)
		{
			fb_assert(baseField.hasData());

			/*RFR.RDB$VIEW_CONTEXT.NULL*/
			jrd_936.jrd_945 = FALSE;
			/*RFR.RDB$VIEW_CONTEXT*/
			jrd_936.jrd_946 = viewContext.value;

			DYN_UTIL_find_field_source(tdbb, transaction, relationName, viewContext.value,
				baseField.c_str(), /*RFR.RDB$FIELD_SOURCE*/
						   jrd_936.jrd_941);
		}
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_935, sizeof(jrd_935));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 206, (UCHAR*) &jrd_936);
	}
}

// Delete local field.
//
//  The rules for dropping a regular column:
//
//   1. the column is not referenced in any views.
//   2. the column is not part of any user defined indexes.
//   3. the column is not used in any SQL statements inside of store
//        procedures or triggers
//   4. the column is not part of any check-constraints
//
// The rules for dropping a column that was created as primary key:
//
//   1. the column is not defined as any foreign keys
//   2. the column is not defined as part of compound primary keys
//
// The rules for dropping a column that was created as foreign key:
//
//   1. the column is not defined as a compound foreign key. A
//        compound foreign key is a foreign key consisted of more
//        than one columns.
//
// The RI enforcement for dropping primary key column is done by system
// triggers and the RI enforcement for dropping foreign key column is
// done by code and system triggers. See the functional description of
// deleteKeyConstraint function for detail.
void RelationNode::deleteLocalField(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relationName, const MetaName& fieldName)
{
   struct {
          SSHORT jrd_894;	// gds__utility 
   } jrd_893;
   struct {
          SSHORT jrd_892;	// gds__utility 
   } jrd_891;
   struct {
          SSHORT jrd_890;	// gds__utility 
   } jrd_889;
   struct {
          TEXT  jrd_886 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_887 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_888;	// RDB$OBJECT_TYPE 
   } jrd_885;
   struct {
          SSHORT jrd_909;	// gds__utility 
   } jrd_908;
   struct {
          SSHORT jrd_907;	// gds__utility 
   } jrd_906;
   struct {
          TEXT  jrd_900 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_901 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_902 [32];	// RDB$GENERATOR_NAME 
          SSHORT jrd_903;	// gds__utility 
          SSHORT jrd_904;	// gds__null_flag 
          SSHORT jrd_905;	// gds__null_flag 
   } jrd_899;
   struct {
          TEXT  jrd_897 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_898 [32];	// RDB$FIELD_NAME 
   } jrd_896;
   struct {
          TEXT  jrd_915 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_916;	// gds__utility 
   } jrd_914;
   struct {
          TEXT  jrd_912 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_913 [32];	// RDB$RELATION_NAME 
   } jrd_911;
   struct {
          TEXT  jrd_924 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_925 [32];	// RDB$CONSTRAINT_NAME 
          SSHORT jrd_926;	// gds__utility 
          SSHORT jrd_927;	// RDB$SEGMENT_COUNT 
   } jrd_923;
   struct {
          TEXT  jrd_919 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_920 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_921 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_922 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_918;
   struct {
          TEXT  jrd_933 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_934;	// gds__utility 
   } jrd_932;
   struct {
          TEXT  jrd_930 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_931 [32];	// RDB$RELATION_NAME 
   } jrd_929;
	AutoCacheRequest request(tdbb, drq_l_dep_flds, DYN_REQUESTS);
	bool found = false;

	// Make sure that column is not referenced in any views.

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$RELATION_FIELDS CROSS
		Y IN RDB$RELATION_FIELDS CROSS
		Z IN RDB$VIEW_RELATIONS
		WITH X.RDB$RELATION_NAME EQ relationName.c_str() AND
			 X.RDB$FIELD_NAME EQ fieldName.c_str() AND
			 X.RDB$FIELD_NAME EQ Y.RDB$BASE_FIELD AND
			 X.RDB$FIELD_SOURCE EQ Y.RDB$FIELD_SOURCE AND
			 Y.RDB$RELATION_NAME EQ Z.RDB$VIEW_NAME AND
			 X.RDB$RELATION_NAME EQ Z.RDB$RELATION_NAME AND
			 Y.RDB$VIEW_CONTEXT EQ Z.RDB$VIEW_CONTEXT*/
	{
	request.compile(tdbb, (UCHAR*) jrd_928, sizeof(jrd_928));
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_929.jrd_930, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_929.jrd_931, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_929);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_932);
	   if (!jrd_932.jrd_934) break;
	{
		// msg 52: "field %s from relation %s is referenced in view %s"
		status_exception::raise(
			Arg::PrivateDyn(52) << fieldName << relationName << /*Y.RDB$RELATION_NAME*/
									    jrd_932.jrd_933);
	}
	/*END_FOR*/
	   }
	}

	// If the column to be dropped is being used as a foreign key
	// and the column was not part of any compound foreign key,
	// then we can drop the column. But we have to drop the foreign key
	// constraint first.

	request.reset(tdbb, drq_g_rel_constr_nm, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES CROSS
		IDX_SEG IN RDB$INDEX_SEGMENTS CROSS
		REL_CONST IN RDB$RELATION_CONSTRAINTS
		WITH IDX.RDB$RELATION_NAME EQ relationName.c_str() AND
			 REL_CONST.RDB$RELATION_NAME EQ relationName.c_str() AND
			 IDX_SEG.RDB$FIELD_NAME EQ fieldName.c_str() AND
			 IDX.RDB$INDEX_NAME EQ IDX_SEG.RDB$INDEX_NAME AND
			 IDX.RDB$INDEX_NAME EQ REL_CONST.RDB$INDEX_NAME AND
			 REL_CONST.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY*/
	{
	request.compile(tdbb, (UCHAR*) jrd_917, sizeof(jrd_917));
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_918.jrd_919, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_918.jrd_920, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_918.jrd_921, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_918.jrd_922, 12);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 108, (UCHAR*) &jrd_918);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 68, (UCHAR*) &jrd_923);
	   if (!jrd_923.jrd_926) break;
	{
		if (/*IDX.RDB$SEGMENT_COUNT*/
		    jrd_923.jrd_927 == 1)
		{
			deleteKeyConstraint(tdbb, transaction, relationName,
				/*REL_CONST.RDB$CONSTRAINT_NAME*/
				jrd_923.jrd_925, /*IDX.RDB$INDEX_NAME*/
  jrd_923.jrd_924);
		}
		else
		{
			// msg 187: "field %s from relation %s is referenced in index %s"
			status_exception::raise(
				Arg::PrivateDyn(187) << fieldName << relationName << /*IDX.RDB$INDEX_NAME*/
										     jrd_923.jrd_924);
		}
	}
	/*END_FOR*/
	   }
	}

	// Make sure that column is not referenced in any user-defined indexes.

	// NOTE: You still could see the system generated indices even though
	// they were already been deleted when dropping column that was used
	// as foreign key before "commit".

	request.reset(tdbb, drq_e_l_idx, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES CROSS
		IDX_SEG IN RDB$INDEX_SEGMENTS
		WITH IDX.RDB$INDEX_NAME EQ IDX_SEG.RDB$INDEX_NAME AND
			 IDX.RDB$RELATION_NAME EQ relationName.c_str() AND
			 IDX_SEG.RDB$FIELD_NAME EQ fieldName.c_str() AND
			 NOT ANY REL_CONST IN RDB$RELATION_CONSTRAINTS
				WITH REL_CONST.RDB$RELATION_NAME EQ IDX.RDB$RELATION_NAME AND
					 REL_CONST.RDB$INDEX_NAME EQ IDX.RDB$INDEX_NAME*/
	{
	request.compile(tdbb, (UCHAR*) jrd_910, sizeof(jrd_910));
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_911.jrd_912, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_911.jrd_913, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_911);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_914);
	   if (!jrd_914.jrd_916) break;
	{
		// msg 187: "field %s from relation %s is referenced in index %s"
		status_exception::raise(
			Arg::PrivateDyn(187) <<
				fieldName << relationName <<
				fb_utils::exact_name_limit(/*IDX.RDB$INDEX_NAME*/
							   jrd_914.jrd_915, sizeof(/*IDX.RDB$INDEX_NAME*/
	 jrd_914.jrd_915)));
	}
	/*END_FOR*/
	   }
	}

	// Delete the automatically created generator for Identity columns.

	request.reset(tdbb, drq_e_lfield, DYN_REQUESTS);

	found = false;
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		RFR IN RDB$RELATION_FIELDS
		WITH RFR.RDB$FIELD_NAME EQ fieldName.c_str() AND
			 RFR.RDB$RELATION_NAME EQ relationName.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_895, sizeof(jrd_895));
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_896.jrd_897, 32);
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_896.jrd_898, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_896);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 102, (UCHAR*) &jrd_899);
	   if (!jrd_899.jrd_903) break;
	{
		if (!/*RFR.RDB$GENERATOR_NAME.NULL*/
		     jrd_899.jrd_905)
			DropSequenceNode::deleteIdentity(tdbb, transaction, /*RFR.RDB$GENERATOR_NAME*/
									    jrd_899.jrd_902);

		/*ERASE RFR;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_906);

		if (!/*RFR.RDB$SECURITY_CLASS.NULL*/
		     jrd_899.jrd_904 &&
			!strncmp(/*RFR.RDB$SECURITY_CLASS*/
				 jrd_899.jrd_901, SQL_SECCLASS_PREFIX, SQL_SECCLASS_PREFIX_LEN))
		{
			deleteSecurityClass(tdbb, transaction, /*RFR.RDB$SECURITY_CLASS*/
							       jrd_899.jrd_901);
		}

		found = true;
		DropRelationNode::deleteGlobalField(tdbb, transaction, /*RFR.RDB$FIELD_SOURCE*/
								       jrd_899.jrd_900);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_908);
	   }
	}

	request.reset(tdbb, drq_e_fld_prvs, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH PRIV.RDB$RELATION_NAME EQ relationName.c_str() AND
			 PRIV.RDB$FIELD_NAME EQ fieldName.c_str() AND
		     PRIV.RDB$OBJECT_TYPE = obj_relation*/
	{
	request.compile(tdbb, (UCHAR*) jrd_884, sizeof(jrd_884));
	gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_885.jrd_886, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_885.jrd_887, 32);
	jrd_885.jrd_888 = obj_relation;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_885);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_889);
	   if (!jrd_889.jrd_890) break;
	{
		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_891);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_893);
	   }
	}

	if (!found)
	{
		// msg 176: "column %s does not exist in table/view %s"
		status_exception::raise(Arg::PrivateDyn(176) << fieldName << relationName);
	}
}

void RelationNode::defineField(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction, AddColumnClause* clause, SSHORT position,
	const ObjectsArray<MetaName>* pkCols)
{
	dsql_fld* field = clause->field;

	// Add the field to the relation being defined for parsing purposes.

	bool permanent = false;
	dsql_rel* relation = dsqlScratch->relation;
	if (relation != NULL)
	{
		if (!(relation->rel_flags & REL_new_relation))
		{
			dsql_fld* permField = FB_NEW(dsqlScratch->getAttachment()->dbb_pool) dsql_fld(
				dsqlScratch->getAttachment()->dbb_pool);

			*permField = *field;
			field = permField;
			permanent = true;
		}

		field->fld_next = relation->rel_fields;
		relation->rel_fields = field;
	}

	try
	{
		// Check for constraints.
		ObjectsArray<Constraint> constraints;
		bool notNullFlag = false;

		if (clause->identity)
			notNullFlag = true;	// identity columns are implicitly not null

		for (ObjectsArray<AddConstraintClause>::iterator ptr = clause->constraints.begin();
			 ptr != clause->constraints.end(); ++ptr)
		{
			makeConstraint(tdbb, dsqlScratch, transaction, &*ptr, constraints, &notNullFlag);
		}

		if (!notNullFlag && pkCols)
		{
			// Let's see if the field appears in a "primary_key (a, b, c)" relation constraint.
			for (size_t i = 0; !notNullFlag && i < pkCols->getCount(); ++i)
			{
				if (field->fld_name == (*pkCols)[i].c_str())
					notNullFlag = true;
			}
		}

		FieldDefinition fieldDefinition(*tdbb->getDefaultPool());
		fieldDefinition.relationName = name;
		fieldDefinition.name = field->fld_name;
		fieldDefinition.notNullFlag = notNullFlag;

		if (position >= 0)
			fieldDefinition.position = position;

		if (field->typeOfName.hasData())
		{
			// Get the domain information.
			if (!METD_get_domain(transaction, field, field->typeOfName))
			{
				// Specified domain or source field does not exist
				status_exception::raise(
					Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					Arg::Gds(isc_dsql_command_err) <<
					Arg::Gds(isc_dsql_domain_not_found) << field->typeOfName);
			}

			fieldDefinition.fieldSource = field->typeOfName;
		}
		else
		{
			string computedSource;
			BlrDebugWriter::BlrData computedValue;

			if (clause->computed)
			{
				field->flags |= FLD_computed;

				defineComputed(dsqlScratch, dsqlNode, field, clause->computed,
					computedSource, computedValue);
			}

			field->collate = clause->collate;
			field->resolve(dsqlScratch);

			// Generate a domain.
			storeGlobalField(tdbb, transaction, fieldDefinition.fieldSource, field,
				computedSource, computedValue);
		}

		if ((relation->rel_flags & REL_external) &&
			(field->dtype == dtype_blob || field->dtype == dtype_array || field->dimensions))
		{
			const char* typeName = (field->dtype == dtype_blob ? "BLOB" : "ARRAY");

			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_dsql_type_not_supp_ext_tab) << typeName << name << field->fld_name);
		}

		if (clause->collate.hasData())
			DDL_resolve_intl_type(dsqlScratch, field, clause->collate);

		if (clause->identity)
		{
			dsc desc;
			MET_get_domain(tdbb, *tdbb->getDefaultPool(), fieldDefinition.fieldSource, &desc, NULL);

			if (!desc.isExact() || desc.dsc_scale != 0)
			{
				// Identity column @1 of table @2 must be exact numeric with zero scale.
				status_exception::raise(Arg::PrivateDyn(273) << field->fld_name << name);
			}

			DYN_UTIL_generate_generator_name(tdbb, fieldDefinition.identitySequence);

			CreateAlterSequenceNode::store(tdbb, transaction, fieldDefinition.identitySequence,
				fb_sysflag_identity_generator, clause->identityStart, 1);
		}

		BlrDebugWriter::BlrData defaultValue;

		if (clause->defaultValue)
		{
			if (defineDefault(tdbb, dsqlScratch, field, clause->defaultValue,
					fieldDefinition.defaultSource, defaultValue) &&
				notNullFlag)
			{
				status_exception::raise(
					Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					Arg::Gds(isc_bad_default_value) <<
					Arg::Gds(isc_invalid_clause) << "default null not null");
			}
		}

		fieldDefinition.defaultValue = defaultValue;

		if (clause->collate.hasData())
			fieldDefinition.collationId = field->collationId;

		fieldDefinition.store(tdbb, transaction);

		// Define the field constraints.
		for (ObjectsArray<Constraint>::iterator constraint(constraints.begin());
			 constraint != constraints.end();
			 ++constraint)
		{
			if (constraint->type != Constraint::TYPE_FK)
				constraint->columns.add(field->fld_name);
			defineConstraint(tdbb, dsqlScratch, transaction, *constraint);
		}
	}
	catch (const Exception&)
	{
		clearPermanentField(relation, permanent);
		throw;
	}

	clearPermanentField(relation, permanent);
}

// Define a DEFAULT clause. Return true for DEFAULT NULL.
bool RelationNode::defineDefault(thread_db* /*tdbb*/, DsqlCompilerScratch* dsqlScratch,
	dsql_fld* /*field*/, ValueSourceClause* clause, string& source, BlrDebugWriter::BlrData& value)
{
	ValueExprNode* input = doDsqlPass(dsqlScratch, clause->value);

	// Generate the blr expression.

	dsqlScratch->getBlrData().clear();
	dsqlScratch->getDebugData().clear();
	dsqlScratch->appendUChar(dsqlScratch->isVersion4() ? blr_version4 : blr_version5);

	GEN_expr(dsqlScratch, input);
	dsqlScratch->appendUChar(blr_eoc);

	// Generate the source text.
	source = clause->source;

	value.assign(dsqlScratch->getBlrData());

	return ExprNode::is<NullNode>(input);
}

// Make a constraint object from a legacy node.
void RelationNode::makeConstraint(thread_db* /*tdbb*/, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction, AddConstraintClause* clause, ObjectsArray<Constraint>& constraints,
	bool* notNull)
{
	switch (clause->constraintType)
	{
		case AddConstraintClause::CTYPE_NOT_NULL:
		case AddConstraintClause::CTYPE_PK:
			if (notNull && !*notNull)
			{
				*notNull = true;

				Constraint& constraint = constraints.add();
				constraint.type = Constraint::TYPE_NOT_NULL;
				if (clause->constraintType == AddConstraintClause::CTYPE_NOT_NULL)
					constraint.name = clause->name;
			}

			if (clause->constraintType == AddConstraintClause::CTYPE_NOT_NULL)
				break;
			// AddConstraintClause::CTYPE_PK falls into

		case AddConstraintClause::CTYPE_UNIQUE:
		{
			Constraint& constraint = constraints.add();
			constraint.type = clause->constraintType == AddConstraintClause::CTYPE_PK ?
				Constraint::TYPE_PK : Constraint::TYPE_UNIQUE;
			constraint.name = clause->name;
			constraint.index = clause->index;
			if (constraint.index && constraint.index->name.isEmpty())
				constraint.index->name = constraint.name;
			constraint.columns = clause->columns;
			break;
		}

		case AddConstraintClause::CTYPE_FK:
		{
			Constraint& constraint = constraints.add();
			constraint.type = Constraint::TYPE_FK;
			constraint.name = clause->name;
			constraint.columns = clause->columns;
			constraint.refRelation = clause->refRelation;
			constraint.refColumns = clause->refColumns;

			// If there is a referenced table name but no referenced field names, the
			// primary key of the referenced table designates the referenced fields.
			if (clause->refColumns.isEmpty())
			{
				Array<NestConst<FieldNode> > refColumns;
				METD_get_primary_key(transaction, clause->refRelation, refColumns);

				// If there is NEITHER an explicitly referenced field name, NOR does
				// the referenced table have a primary key to serve as the implicitly
				// referenced field, fail.
				if (refColumns.isEmpty())
				{
					// "REFERENCES table" without "(column)" requires PRIMARY
					// KEY on referenced table

					status_exception::raise(
						Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
						Arg::Gds(isc_dsql_command_err) <<
						Arg::Gds(isc_reftable_requires_pk));
				}
				else
				{
					const NestConst<FieldNode>* ptr = refColumns.begin();

					for (const NestConst<FieldNode>* const end = refColumns.end(); ptr != end; ++ptr)
						constraint.refColumns.add((*ptr)->dsqlName);
				}
			}

			if (constraint.refColumns.getCount() != constraint.columns.getCount())
			{
				// Foreign key field count does not match primary key.
				status_exception::raise(
					Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
					Arg::Gds(isc_dsql_command_err) <<
					Arg::Gds(isc_key_field_count_err));
			}

			// Define the foreign key index and the triggers that may be needed
			// for referential integrity action.
			constraint.index = clause->index;
			if (constraint.index && constraint.index->name.isEmpty())
				constraint.index->name = constraint.name;

			if (clause->refAction)
			{
				if (clause->refAction->updateAction != 0)
				{
					switch (clause->refAction->updateAction)
					{
						case RefActionClause::ACTION_CASCADE:
							constraint.refUpdateAction = RI_ACTION_CASCADE;
							defineUpdateCascadeTrigger(dsqlScratch, constraint);
							break;

						case RefActionClause::ACTION_SET_DEFAULT:
							constraint.refUpdateAction = RI_ACTION_DEFAULT;
							defineSetDefaultTrigger(dsqlScratch, constraint, true);
							break;

						case RefActionClause::ACTION_SET_NULL:
							constraint.refUpdateAction = RI_ACTION_NULL;
							defineSetNullTrigger(dsqlScratch, constraint, true);
							break;

						default:
							fb_assert(0);
							// fall into

						case RefActionClause::ACTION_NONE:
							constraint.refUpdateAction = RI_ACTION_NONE;
							break;
					}
				}

				if (clause->refAction->deleteAction != 0)
				{
					switch (clause->refAction->deleteAction)
					{
						case RefActionClause::ACTION_CASCADE:
							constraint.refDeleteAction = RI_ACTION_CASCADE;
							defineDeleteCascadeTrigger(dsqlScratch, constraint);
							break;

						case RefActionClause::ACTION_SET_DEFAULT:
							constraint.refDeleteAction = RI_ACTION_DEFAULT;
							defineSetDefaultTrigger(dsqlScratch, constraint, false);
							break;

						case RefActionClause::ACTION_SET_NULL:
							constraint.refDeleteAction = RI_ACTION_NULL;
							defineSetNullTrigger(dsqlScratch, constraint, false);
							break;

						default:
							fb_assert(0);
							// fall into

						case RefActionClause::ACTION_NONE:
							constraint.refDeleteAction = RI_ACTION_NONE;
							break;
					}
				}
			}

			break;
		}

		case AddConstraintClause::CTYPE_CHECK:
		{
			Constraint& constraint = constraints.add();
			constraint.type = Constraint::TYPE_CHECK;
			constraint.name = clause->name;
			defineCheckConstraint(dsqlScratch, constraint, clause->check);
			break;
		}
	}
}

// Define a constraint.
void RelationNode::defineConstraint(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction, Constraint& constraint)
{
   struct {
          TEXT  jrd_841 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_842 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_843;	// gds__utility 
   } jrd_840;
   struct {
          TEXT  jrd_836 [32];	// RDB$CONSTRAINT_NAME 
          TEXT  jrd_837 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_838 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_839 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_835;
   struct {
          TEXT  jrd_850 [32];	// RDB$CONSTRAINT_NAME 
          SSHORT jrd_851;	// gds__utility 
   } jrd_849;
   struct {
          TEXT  jrd_846 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_847 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_848 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_845;
   struct {
          TEXT  jrd_854 [32];	// RDB$CONSTRAINT_NAME 
          TEXT  jrd_855 [32];	// RDB$CONST_NAME_UQ 
          TEXT  jrd_856 [12];	// RDB$DELETE_RULE 
          TEXT  jrd_857 [12];	// RDB$UPDATE_RULE 
          SSHORT jrd_858;	// gds__null_flag 
          SSHORT jrd_859;	// gds__null_flag 
   } jrd_853;
   struct {
          SSHORT jrd_864;	// gds__utility 
   } jrd_863;
   struct {
          TEXT  jrd_862 [32];	// RDB$INDEX_NAME 
   } jrd_861;
   struct {
          TEXT  jrd_870 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_871 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_872;	// gds__utility 
          SSHORT jrd_873;	// gds__null_flag 
          SSHORT jrd_874;	// RDB$NULL_FLAG 
          SSHORT jrd_875;	// gds__null_flag 
          SSHORT jrd_876;	// RDB$NULL_FLAG 
   } jrd_869;
   struct {
          TEXT  jrd_867 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_868 [32];	// RDB$INDEX_NAME 
   } jrd_866;
   struct {
          TEXT  jrd_879 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_880 [32];	// RDB$CONSTRAINT_NAME 
          TEXT  jrd_881 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_882 [12];	// RDB$CONSTRAINT_TYPE 
          SSHORT jrd_883;	// gds__null_flag 
   } jrd_878;
	if (constraint.name.isEmpty())
		DYN_UTIL_generate_constraint_name(tdbb, constraint.name);

	AutoCacheRequest request(tdbb, drq_s_rel_con, DYN_REQUESTS);
	MetaName referredIndexName;

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		CRT IN RDB$RELATION_CONSTRAINTS*/
	{
	
	{
		/*CRT.RDB$INDEX_NAME.NULL*/
		jrd_878.jrd_883 = TRUE;

		strcpy(/*CRT.RDB$CONSTRAINT_NAME*/
		       jrd_878.jrd_880, constraint.name.c_str());
		strcpy(/*CRT.RDB$RELATION_NAME*/
		       jrd_878.jrd_879, name.c_str());

		switch (constraint.type)
		{
			case Constraint::TYPE_CHECK:
				strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
				       jrd_878.jrd_882, CHECK_CNSTRT);
				break;

			case Constraint::TYPE_NOT_NULL:
				strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
				       jrd_878.jrd_882, NOT_NULL_CNSTRT);
				break;

			case Constraint::TYPE_PK:
				strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
				       jrd_878.jrd_882, PRIMARY_KEY);
				break;

			case Constraint::TYPE_UNIQUE:
				strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
				       jrd_878.jrd_882, UNIQUE_CNSTRT);
				break;

			case Constraint::TYPE_FK:
				strcpy(/*CRT.RDB$CONSTRAINT_TYPE*/
				       jrd_878.jrd_882, FOREIGN_KEY);
				break;

			default:
				fb_assert(false);
		}

		// These constraints require creation of an index.
		switch (constraint.type)
		{
			case Constraint::TYPE_PK:
			case Constraint::TYPE_UNIQUE:
			case Constraint::TYPE_FK:
			{
				CreateIndexNode::Definition definition;
				definition.relation = name;
				definition.type = (constraint.type == Constraint::TYPE_PK ?
					isc_dyn_def_primary_key :
					(constraint.type == Constraint::TYPE_FK ? isc_dyn_def_foreign_key : 0));
				definition.unique = constraint.type != Constraint::TYPE_FK;
				if (constraint.index->descending)
					definition.descending = true;
				definition.columns = constraint.columns;
				definition.refRelation = constraint.refRelation;
				definition.refColumns = constraint.refColumns;

				CreateIndexNode::store(tdbb, transaction, constraint.index->name,
					definition, &referredIndexName);

				/*CRT.RDB$INDEX_NAME.NULL*/
				jrd_878.jrd_883 = FALSE;
				strcpy(/*CRT.RDB$INDEX_NAME*/
				       jrd_878.jrd_881, constraint.index->name.c_str());

				checkForeignKeyTempScope(tdbb, transaction, name, referredIndexName);

				// Check that we have references permissions on the table and
				// fields that the index:referredIndexName is on.
				SCL_check_index(tdbb, referredIndexName, 0, SCL_references);

				break;
			}
		}
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_877, sizeof(jrd_877));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 110, (UCHAR*) &jrd_878);
	}

	if (constraint.type == Constraint::TYPE_NOT_NULL)
	{
		fb_assert(constraint.columns.getCount() == 1);
		DYN_UTIL_store_check_constraints(tdbb, transaction, constraint.name,
			*constraint.columns.begin());
	}

	// Define the automatically generated triggers.

	for (ObjectsArray<TriggerDefinition>::iterator trigger(constraint.triggers.begin());
		 trigger != constraint.triggers.end();
		 ++trigger)
	{
		trigger->store(tdbb, dsqlScratch, transaction);
		DYN_UTIL_store_check_constraints(tdbb, transaction, constraint.name, trigger->name);
	}

	if (constraint.type == Constraint::TYPE_NOT_NULL || constraint.type == Constraint::TYPE_CHECK)
		return;

	// Make sure unique field names were specified for UNIQUE/PRIMARY/FOREIGN
	// All fields must have the NOT NULL attribute specified for UNIQUE/PRIMARY.

	request.reset(tdbb, drq_c_unq_nam2, DYN_REQUESTS);

	int allCount = 0;
	int uniqueCount = 0;
	ObjectsArray<MetaName> fieldList;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDS IN RDB$INDEX_SEGMENTS CROSS
		RFR IN RDB$RELATION_FIELDS CROSS
		FLX IN RDB$FIELDS
		WITH IDS.RDB$INDEX_NAME EQ constraint.index->name.c_str() AND
			 RFR.RDB$RELATION_NAME EQ name.c_str() AND
			 RFR.RDB$FIELD_NAME EQ IDS.RDB$FIELD_NAME AND
			 FLX.RDB$FIELD_NAME EQ RFR.RDB$FIELD_SOURCE
		REDUCED TO IDS.RDB$FIELD_NAME, IDS.RDB$INDEX_NAME, FLX.RDB$NULL_FLAG
		SORTED BY ASCENDING IDS.RDB$FIELD_NAME*/
	{
	request.compile(tdbb, (UCHAR*) jrd_865, sizeof(jrd_865));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_866.jrd_867, 32);
	gds__vtov ((const char*) constraint.index->name.c_str(), (char*) jrd_866.jrd_868, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_866);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 74, (UCHAR*) &jrd_869);
	   if (!jrd_869.jrd_872) break;
	{
		if ((/*FLX.RDB$NULL_FLAG.NULL*/
		     jrd_869.jrd_875 || !/*FLX.RDB$NULL_FLAG*/
     jrd_869.jrd_876) &&
			(/*RFR.RDB$NULL_FLAG.NULL*/
			 jrd_869.jrd_873 || !/*RFR.RDB$NULL_FLAG*/
     jrd_869.jrd_874) &&
			constraint.type == Constraint::TYPE_PK)
		{
			// msg 123: "Field: %s not defined as NOT NULL - can't be used in PRIMARY KEY
			// constraint definition"
			status_exception::raise(Arg::PrivateDyn(123) << MetaName(/*RFR.RDB$FIELD_NAME*/
										 jrd_869.jrd_871));
		}

		++uniqueCount;
		fieldList.add() = /*IDS.RDB$FIELD_NAME*/
				  jrd_869.jrd_870;
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, drq_n_idx_seg, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDS IN RDB$INDEX_SEGMENTS
		WITH IDS.RDB$INDEX_NAME EQ constraint.index->name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_860, sizeof(jrd_860));
	gds__vtov ((const char*) constraint.index->name.c_str(), (char*) jrd_861.jrd_862, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_861);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_863);
	   if (!jrd_863.jrd_864) break;
	{
		++allCount;
	}
	/*END_FOR*/
	   }
	}

	if (uniqueCount != allCount)
	{
		// msg 124: "A column name is repeated in the definition of constraint: %s"
		status_exception::raise(Arg::PrivateDyn(124) << constraint.name);
	}

	if (constraint.type == Constraint::TYPE_FK)
	{
		request.reset(tdbb, drq_s_ref_con, DYN_REQUESTS);

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			REF IN RDB$REF_CONSTRAINTS*/
		{
		
		{
			AutoCacheRequest request2(tdbb, drq_l_intg_con, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
				CRT IN RDB$RELATION_CONSTRAINTS
				WITH CRT.RDB$INDEX_NAME EQ referredIndexName.c_str() AND
					 (CRT.RDB$CONSTRAINT_TYPE = PRIMARY_KEY OR
					  CRT.RDB$CONSTRAINT_TYPE = UNIQUE_CNSTRT)*/
			{
			request2.compile(tdbb, (UCHAR*) jrd_844, sizeof(jrd_844));
			gds__vtov ((const char*) referredIndexName.c_str(), (char*) jrd_845.jrd_846, 32);
			gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_845.jrd_847, 12);
			gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_845.jrd_848, 12);
			EXE_start (tdbb, request2, transaction);
			EXE_send (tdbb, request2, 0, 56, (UCHAR*) &jrd_845);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 34, (UCHAR*) &jrd_849);
			   if (!jrd_849.jrd_851) break;
			{
				fb_utils::exact_name_limit(/*CRT.RDB$CONSTRAINT_NAME*/
							   jrd_849.jrd_850, sizeof(/*CRT.RDB$CONSTRAINT_NAME*/
	 jrd_849.jrd_850));
				strcpy(/*REF.RDB$CONST_NAME_UQ*/
				       jrd_853.jrd_855, /*CRT.RDB$CONSTRAINT_NAME*/
  jrd_849.jrd_850);
				strcpy(/*REF.RDB$CONSTRAINT_NAME*/
				       jrd_853.jrd_854, constraint.name.c_str());

				/*REF.RDB$UPDATE_RULE.NULL*/
				jrd_853.jrd_859 = FALSE;
				strcpy(/*REF.RDB$UPDATE_RULE*/
				       jrd_853.jrd_857, constraint.refUpdateAction);

				/*REF.RDB$DELETE_RULE.NULL*/
				jrd_853.jrd_858 = FALSE;
				strcpy(/*REF.RDB$DELETE_RULE*/
				       jrd_853.jrd_856, constraint.refDeleteAction);
			}
			/*END_FOR*/
			   }
			}
		}
		/*END_STORE*/
		request.compile(tdbb, (UCHAR*) jrd_852, sizeof(jrd_852));
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 92, (UCHAR*) &jrd_853);
		}
	}
	else
	{
		// For PRIMARY KEY/UNIQUE constraints, make sure same set of columns
		// is not used in another constraint of either type.

		request.reset(tdbb, drq_c_dup_con, DYN_REQUESTS);

		MetaName indexName;
		int listIndex = -1;
		bool found = false;

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			CRT IN RDB$RELATION_CONSTRAINTS CROSS
			IDS IN RDB$INDEX_SEGMENTS OVER RDB$INDEX_NAME
			WITH CRT.RDB$RELATION_NAME EQ name.c_str() AND
				 CRT.RDB$CONSTRAINT_NAME NE constraint.name.c_str() AND
				 (CRT.RDB$CONSTRAINT_TYPE EQ PRIMARY_KEY OR
				  CRT.RDB$CONSTRAINT_TYPE EQ UNIQUE_CNSTRT)
			SORTED BY CRT.RDB$INDEX_NAME, DESCENDING IDS.RDB$FIELD_NAME*/
		{
		request.compile(tdbb, (UCHAR*) jrd_834, sizeof(jrd_834));
		gds__vtov ((const char*) constraint.name.c_str(), (char*) jrd_835.jrd_836, 32);
		gds__vtov ((const char*) name.c_str(), (char*) jrd_835.jrd_837, 32);
		gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_835.jrd_838, 12);
		gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_835.jrd_839, 12);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 88, (UCHAR*) &jrd_835);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_840);
		   if (!jrd_840.jrd_843) break;
		{
			if (indexName != /*CRT.RDB$INDEX_NAME*/
					 jrd_840.jrd_842)
			{
				if (listIndex >= 0)
					found = false;

				if (found)
					break;

				listIndex = fieldList.getCount() - 1;
				indexName = /*CRT.RDB$INDEX_NAME*/
					    jrd_840.jrd_842;
				found = true;
			}

			if (listIndex >= 0)
			{
				if (fieldList[listIndex--] != /*IDS.RDB$FIELD_NAME*/
							      jrd_840.jrd_841)
					found = false;
			}
			else
				found = false;
		}
		/*END_FOR*/
		   }
		}

		if (listIndex >= 0)
			found = false;

		if (found)
		{
			// msg 126: "Same set of columns cannot be used in more than one PRIMARY KEY
			// and/or UNIQUE constraint definition"
			status_exception::raise(Arg::PrivateDyn(126));
		}
	}
}

// Generate triggers to implement the CHECK clause, either at the field or table level.
void RelationNode::defineCheckConstraint(DsqlCompilerScratch* dsqlScratch, Constraint& constraint,
	BoolSourceClause* clause)
{
	// Create the INSERT trigger.
	defineCheckConstraintTrigger(dsqlScratch, constraint, clause, PRE_STORE_TRIGGER);

	// Create the UPDATE trigger.
	defineCheckConstraintTrigger(dsqlScratch, constraint, clause, PRE_MODIFY_TRIGGER);
}

// Define a check constraint trigger.
void RelationNode::defineCheckConstraintTrigger(DsqlCompilerScratch* dsqlScratch,
	Constraint& constraint, BoolSourceClause* clause, FB_UINT64 triggerType)
{
	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	AutoSetRestore<bool> autoCheckConstraintTrigger(&dsqlScratch->checkConstraintTrigger, true);

	Constraint::BlrWriter& blrWriter = constraint.blrWritersHolder.add();
	blrWriter.init(dsqlScratch);

	// Specify that the trigger should abort if the condition is not met.
	NestConst<CompoundStmtNode> actionNode = FB_NEW(pool) CompoundStmtNode(pool);

	ExceptionNode* exceptionNode = FB_NEW(pool) ExceptionNode(pool, CHECK_CONSTRAINT_EXCEPTION);
	exceptionNode->exception->type = ExceptionItem::GDS_CODE;

	actionNode->statements.add(exceptionNode);

	// Generate the trigger blr.

	dsqlScratch->getBlrData().clear();
	dsqlScratch->getDebugData().clear();

	dsqlScratch->appendUChar(blr_begin);

	// Create the "OLD" and "NEW" contexts for the trigger -- the new one could be a dummy
	// place holder to avoid resolving fields to that context but prevent relations referenced
	// in the trigger actions from referencing the predefined "1" context.

	dsqlScratch->resetContextStack();

	// CVC: I thought I could disable the OLD context here to avoid "ambiguous field name"
	// errors in pre_store and pre_modify triggers. Also, what sense can I make from NEW in
	// pre_delete? However, we clash at JRD with "no current record for fetch operation".

	dsqlNode->alias = OLD_CONTEXT_NAME;
	dsql_ctx* oldContext = PASS1_make_context(dsqlScratch, dsqlNode);
	oldContext->ctx_flags |= CTX_system;

	dsqlNode->alias = NEW_CONTEXT_NAME;
	dsql_ctx* newContext = PASS1_make_context(dsqlScratch, dsqlNode);
	newContext->ctx_flags |= CTX_system;

	// Generate the condition for firing the trigger.

	NotBoolNode* notNode = FB_NEW(pool) NotBoolNode(pool, clause->value);

	BoolExprNode* condition = notNode->dsqlPass(dsqlScratch);

	dsqlScratch->appendUChar(blr_if);
	GEN_expr(dsqlScratch, condition);

	// Generate the action statement for the trigger.
	Node::doDsqlPass(dsqlScratch, actionNode)->genBlr(dsqlScratch);

	dsqlScratch->appendUChar(blr_end);	// of if (as there's no ELSE branch)
	dsqlScratch->appendUChar(blr_end);	// of begin

	dsqlScratch->appendUChar(blr_eoc);	// end of the blr

	dsqlScratch->resetContextStack();

	// Move the blr to the constraint blrWriter.
	blrWriter.getBlrData().join(dsqlScratch->getBlrData());

	TriggerDefinition& trigger = constraint.triggers.add();
	trigger.systemFlag = fb_sysflag_check_constraint;
	trigger.relationName = name;
	trigger.type = triggerType;
	trigger.source = clause->source;
	trigger.blrData = blrWriter.getBlrData();
}

// Define "on delete|update set default" trigger (for referential integrity) along with its blr.
void RelationNode::defineSetDefaultTrigger(DsqlCompilerScratch* dsqlScratch,
	Constraint& constraint, bool onUpdate)
{
	fb_assert(constraint.columns.getCount() == constraint.refColumns.getCount());
	fb_assert(constraint.columns.hasData());

	Constraint::BlrWriter& blrWriter = constraint.blrWritersHolder.add();
	blrWriter.init(dsqlScratch);

	generateUnnamedTriggerBeginning(constraint, onUpdate, blrWriter);

	const int BLOB_BUFFER_SIZE = 4096;	// to read in blr blob for default values
	UCHAR defaultVal[BLOB_BUFFER_SIZE];

	for (ObjectsArray<MetaName>::const_iterator column(constraint.columns.begin());
		 column != constraint.columns.end();
		 ++column)
	{
		blrWriter.appendUChar(blr_assignment);

		// ASF: This is wrong way to do the thing. See CORE-3073.

		// Here stuff the default value as blr_literal .... or blr_null
		// if this column does not have an applicable default.
		// The default is determined in many cases:
		// (1) the info. for the column is in memory. (This is because
		// the column is being created in this ddl statement).
		// (1-a) the table has a column level default. We get this by
		// searching the dsql parse tree.
		// (1-b) the table does not have a column level default, but
		// has a domain default. We get the domain name from the dsql
		// parse tree and call METD_get_domain_default to read the
		// default from the system tables.
		// (2) The default-info for this column is not in memory (This is
		// because this is an alter table ddl statement). The table
		// already exists; therefore we get the column and/or domain
		// default value from the system tables by calling:
		// METD_get_col_default().

		bool foundDefault = false;
		bool searchForDefault = true;

		// Search the parse tree to find the column

		for (NestConst<Clause>* ptr = clauses.begin(); ptr != clauses.end(); ++ptr)
		{
			if ((*ptr)->type != Clause::TYPE_ADD_COLUMN)
				continue;

			AddColumnClause* clause = static_cast<AddColumnClause*>(ptr->getObject());

			if (*column != clause->field->fld_name)
				continue;

			// Now we have the right column in the parse tree. case (1) above

			if (clause->defaultValue)
			{
				// case (1-a) above: There is a column level default

				dsqlScratch->getBlrData().clear();
				dsqlScratch->getDebugData().clear();

				GEN_expr(dsqlScratch, clause->defaultValue->value);

				foundDefault = true;
				searchForDefault = false;

				// Move the blr to the constraint blrWriter.
				blrWriter.getBlrData().join(dsqlScratch->getBlrData());
			}
			else
			{
				if (!clause->field->typeOfName.hasData())
					break;

				// case: (1-b): Domain name is available. Column level default
				// is not declared. So get the domain default.
				const USHORT defaultLen = METD_get_domain_default(dsqlScratch->getTransaction(),
					clause->field->typeOfName, &foundDefault, defaultVal, sizeof(defaultVal));

				searchForDefault = false;

				if (foundDefault)
					stuffDefaultBlr(ByteChunk(defaultVal, defaultLen), blrWriter);
				else
				{
					// Neither column level nor domain level default exists.
					blrWriter.appendUChar(blr_null);
				}
			}

			break;
		}

		if (searchForDefault)
		{
			// case 2: See if the column/domain has already been created.

			const USHORT defaultLen = METD_get_col_default(dsqlScratch->getTransaction(),
				name.c_str(), column->c_str(), &foundDefault, defaultVal, sizeof(defaultVal));

			if (foundDefault)
				stuffDefaultBlr(ByteChunk(defaultVal, defaultLen), blrWriter);
			else
				blrWriter.appendUChar(blr_null);
		}

		// The context for the foreign key relation.
		blrWriter.appendUChar(blr_field);
		blrWriter.appendUChar(2);
		blrWriter.appendNullString(0, column->c_str());
	}

	blrWriter.appendUChar(blr_end);

	if (onUpdate)
		blrWriter.appendUCharRepeated(blr_end, 3);

	blrWriter.appendUChar(blr_eoc);	// end of the blr

	TriggerDefinition& trigger = constraint.triggers.add();
	trigger.systemFlag = fb_sysflag_referential_constraint;
	trigger.fkTrigger = true;
	trigger.relationName = constraint.refRelation;
	trigger.type = (onUpdate ? POST_MODIFY_TRIGGER : POST_ERASE_TRIGGER);
	trigger.blrData = blrWriter.getBlrData();
}

// Define "on delete/update set null" trigger (for referential integrity).
// The trigger blr is the same for both the delete and update cases. Only difference is its
// TRIGGER_TYPE (ON DELETE or ON UPDATE).
// When onUpdate parameter == true is an update trigger.
void RelationNode::defineSetNullTrigger(DsqlCompilerScratch* dsqlScratch, Constraint& constraint,
	bool onUpdate)
{
	fb_assert(constraint.columns.getCount() == constraint.refColumns.getCount());
	fb_assert(constraint.columns.hasData());

	Constraint::BlrWriter& blrWriter = constraint.blrWritersHolder.add();
	blrWriter.init(dsqlScratch);

	generateUnnamedTriggerBeginning(constraint, onUpdate, blrWriter);

	for (ObjectsArray<MetaName>::const_iterator column(constraint.columns.begin());
		 column != constraint.columns.end();
		 ++column)
	{
		blrWriter.appendUChar(blr_assignment);
		blrWriter.appendUChar(blr_null);
		blrWriter.appendUChar(blr_field);
		blrWriter.appendUChar(2);
		blrWriter.appendNullString(0, column->c_str());
	}

	blrWriter.appendUChar(blr_end);

	if (onUpdate)
		blrWriter.appendUCharRepeated(blr_end, 3);

	blrWriter.appendUChar(blr_eoc);	// end of the blr

	TriggerDefinition& trigger = constraint.triggers.add();
	trigger.systemFlag = fb_sysflag_referential_constraint;
	trigger.fkTrigger = true;
	trigger.relationName = constraint.refRelation;
	trigger.type = (onUpdate ? POST_MODIFY_TRIGGER : POST_ERASE_TRIGGER);
	trigger.blrData = blrWriter.getBlrData();
}

// Define "on delete cascade" trigger (for referential integrity) along with the trigger blr.
void RelationNode::defineDeleteCascadeTrigger(DsqlCompilerScratch* dsqlScratch,
	Constraint& constraint)
{
	fb_assert(constraint.columns.getCount() == constraint.refColumns.getCount());
	fb_assert(constraint.columns.hasData());

	Constraint::BlrWriter& blrWriter = constraint.blrWritersHolder.add();
	blrWriter.init(dsqlScratch);

	blrWriter.appendUChar(blr_for);
	blrWriter.appendUChar(blr_rse);

	// The context for the prim. key relation.
	blrWriter.appendUChar(1);
	blrWriter.appendUChar(blr_relation);
	blrWriter.appendNullString(0, name.c_str());
	// The context for the foreign key relation.
	blrWriter.appendUChar(2);

	// Generate the blr for: foreign_key == primary_key.
	stuffMatchingBlr(constraint, blrWriter);

	blrWriter.appendUChar(blr_erase);
	blrWriter.appendUChar(2);

	blrWriter.appendUChar(blr_eoc);	// end of the blr

	TriggerDefinition& trigger = constraint.triggers.add();
	trigger.systemFlag = fb_sysflag_referential_constraint;
	trigger.fkTrigger = true;
	trigger.relationName = constraint.refRelation;
	trigger.type = POST_ERASE_TRIGGER;
	trigger.blrData = blrWriter.getBlrData();
}

// Define "on update cascade" trigger (for referential integrity) along with the trigger blr.
void RelationNode::defineUpdateCascadeTrigger(DsqlCompilerScratch* dsqlScratch,
	Constraint& constraint)
{
	fb_assert(constraint.columns.getCount() == constraint.refColumns.getCount());
	fb_assert(constraint.columns.hasData());

	Constraint::BlrWriter& blrWriter = constraint.blrWritersHolder.add();
	blrWriter.init(dsqlScratch);

	generateUnnamedTriggerBeginning(constraint, true, blrWriter);

	for (ObjectsArray<MetaName>::const_iterator column(constraint.columns.begin()),
			refColumn(constraint.refColumns.begin());
		 column != constraint.columns.end();
		 ++column, ++refColumn)
	{
		blrWriter.appendUChar(blr_assignment);

		blrWriter.appendUChar(blr_field);
		blrWriter.appendUChar(1);
		blrWriter.appendNullString(0, refColumn->c_str());

		blrWriter.appendUChar(blr_field);
		blrWriter.appendUChar(2);
		blrWriter.appendNullString(0, column->c_str());
	}

	blrWriter.appendUCharRepeated(blr_end, 4);
	blrWriter.appendUChar(blr_eoc);	// end of the blr

	TriggerDefinition& trigger = constraint.triggers.add();
	trigger.systemFlag = fb_sysflag_referential_constraint;
	trigger.fkTrigger = true;
	trigger.relationName = constraint.refRelation;
	trigger.type = POST_MODIFY_TRIGGER;
	trigger.blrData = blrWriter.getBlrData();
}

// Common code factored out.
void RelationNode::generateUnnamedTriggerBeginning(Constraint& constraint, bool onUpdate,
	BlrDebugWriter& blrWriter)
{
	// For ON UPDATE TRIGGER only: generate the trigger firing condition:
	// If prim_key.old_value != prim_key.new value.
	// Note that the key could consist of multiple columns.

	if (onUpdate)
	{
		stuffTriggerFiringCondition(constraint, blrWriter);
		blrWriter.appendUCharRepeated(blr_begin, 2);
	}

	blrWriter.appendUChar(blr_for);
	blrWriter.appendUChar(blr_rse);

	// The context for the prim. key relation.
	blrWriter.appendUChar(1);
	blrWriter.appendUChar(blr_relation);
	blrWriter.appendNullString(0, name.c_str());
	// The context for the foreign key relation.
	blrWriter.appendUChar(2);

	// Generate the blr for: foreign_key == primary_key.
	stuffMatchingBlr(constraint, blrWriter);

	blrWriter.appendUChar(blr_modify);
	blrWriter.appendUChar(2);
	blrWriter.appendUChar(2);
	blrWriter.appendUChar(blr_begin);
}

// The defaultBlr passed is of the form:
//     blr_version4 blr_literal ..... blr_eoc.
// Strip the blr_version4 and blr_eoc verbs and stuff the remaining blr in the blr stream in the
// statement.
void RelationNode::stuffDefaultBlr(const ByteChunk& defaultBlr, BlrDebugWriter& blrWriter)
{
	fb_assert(defaultBlr.length > 2 && defaultBlr.data[defaultBlr.length - 1] == blr_eoc);
	fb_assert(*defaultBlr.data == blr_version4 || *defaultBlr.data == blr_version5);

	blrWriter.appendBytes(defaultBlr.data + 1, defaultBlr.length - 2);
}

// Generate blr to express: foreign_key == primary_key
// ie., for_key.column_1 = prim_key.column_1 and
//      for_key.column_2 = prim_key.column_2 and ....  so on.
void RelationNode::stuffMatchingBlr(Constraint& constraint, BlrDebugWriter& blrWriter)
{
	// count of foreign key columns
	fb_assert(constraint.refColumns.getCount() == constraint.columns.getCount());
	fb_assert(constraint.refColumns.getCount() != 0);

	blrWriter.appendUChar(blr_boolean);

	size_t numFields = 0;

	for (ObjectsArray<MetaName>::const_iterator column(constraint.columns.begin()),
			refColumn(constraint.refColumns.begin());
		 column != constraint.columns.end();
		 ++column, ++refColumn, ++numFields)
	{
		if (numFields + 1 < constraint.columns.getCount())
			blrWriter.appendUChar(blr_and);

		blrWriter.appendUChar(blr_eql);

		blrWriter.appendUChar(blr_field);
		blrWriter.appendUChar(2);
		blrWriter.appendNullString(0, column->c_str());
		blrWriter.appendUChar(blr_field);
		blrWriter.appendUChar(0);
		blrWriter.appendNullString(0, refColumn->c_str());
	};

	blrWriter.appendUChar(blr_end);
}

// Generate blr to express: if (old.primary_key != new.primary_key).
// Do a column by column comparison.
void RelationNode::stuffTriggerFiringCondition(const Constraint& constraint, BlrDebugWriter& blrWriter)
{
	blrWriter.appendUChar(blr_if);

	size_t numFields = 0;

	for (ObjectsArray<MetaName>::const_iterator column(constraint.refColumns.begin());
		 column != constraint.refColumns.end();
		 ++column, ++numFields)
	{
		if (numFields + 1 < constraint.refColumns.getCount())
			blrWriter.appendUChar(blr_or);

		blrWriter.appendUChar(blr_neq);

		blrWriter.appendUChar(blr_field);
		blrWriter.appendUChar(0);
		blrWriter.appendNullString(0, column->c_str());
		blrWriter.appendUChar(blr_field);
		blrWriter.appendUChar(1);
		blrWriter.appendNullString(0, column->c_str());
	}
}


//----------------------


void CreateRelationNode::print(string& text) const
{
	text.printf(
		"CreateRelationNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void CreateRelationNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          TEXT  jrd_824 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_825;	// RDB$VIEW_SOURCE 
          bid  jrd_826;	// RDB$VIEW_BLR 
          TEXT  jrd_827 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_828;	// gds__null_flag 
          SSHORT jrd_829;	// gds__null_flag 
          SSHORT jrd_830;	// gds__null_flag 
          SSHORT jrd_831;	// RDB$RELATION_TYPE 
          SSHORT jrd_832;	// RDB$FLAGS 
          SSHORT jrd_833;	// RDB$SYSTEM_FLAG 
   } jrd_823;
	saveRelation(tdbb, dsqlScratch, name, false, true);

	if (externalFile)
	{
		fb_assert(dsqlScratch->relation);
		dsqlScratch->relation->rel_flags |= REL_external;
	}

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_CREATE_TABLE, name);

	DYN_UTIL_check_unique_name(tdbb, transaction, name, obj_relation);

	checkRelationTempScope(tdbb, transaction, name, relationType);

	AutoCacheRequest request(tdbb, drq_s_rels2, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		REL IN RDB$RELATIONS*/
	{
	
	{
		strcpy(/*REL.RDB$RELATION_NAME*/
		       jrd_823.jrd_827, name.c_str());
		/*REL.RDB$SYSTEM_FLAG*/
		jrd_823.jrd_833 = 0;
		/*REL.RDB$FLAGS*/
		jrd_823.jrd_832 = REL_sql;
		/*REL.RDB$RELATION_TYPE*/
		jrd_823.jrd_831 = relationType;

		/*REL.RDB$VIEW_BLR.NULL*/
		jrd_823.jrd_830 = TRUE;
		/*REL.RDB$VIEW_SOURCE.NULL*/
		jrd_823.jrd_829 = TRUE;
		/*REL.RDB$EXTERNAL_FILE.NULL*/
		jrd_823.jrd_828 = TRUE;

		if (externalFile)
		{
			if (externalFile->length() >= sizeof(/*REL.RDB$EXTERNAL_FILE*/
							     jrd_823.jrd_824))
				status_exception::raise(Arg::Gds(isc_dyn_name_longer));

			if (ISC_check_if_remote(externalFile->c_str(), false))
				status_exception::raise(Arg::PrivateDyn(163));

			/*REL.RDB$EXTERNAL_FILE.NULL*/
			jrd_823.jrd_828 = FALSE;
			strcpy(/*REL.RDB$EXTERNAL_FILE*/
			       jrd_823.jrd_824, externalFile->c_str());
			/*REL.RDB$RELATION_TYPE*/
			jrd_823.jrd_831 = rel_external;
		}
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_822, sizeof(jrd_822));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 316, (UCHAR*) &jrd_823);
	}

	storePrivileges(tdbb, transaction, name, obj_relation, ALL_PRIVILEGES);

	ObjectsArray<Constraint> constraints;
	const ObjectsArray<MetaName>* pkCols = findPkColumns();
	SSHORT position = 0;

	for (NestConst<Clause>* i = clauses.begin(); i != clauses.end(); ++i)
	{
		switch ((*i)->type)
		{
			case Clause::TYPE_ADD_COLUMN:
				defineField(tdbb, dsqlScratch, transaction,
					static_cast<AddColumnClause*>(i->getObject()), position, pkCols);
				++position;
				break;

			case Clause::TYPE_ADD_CONSTRAINT:
				makeConstraint(tdbb, dsqlScratch, transaction,
					static_cast<AddConstraintClause*>(i->getObject()), constraints);
				break;

			default:
				fb_assert(false);
				break;
		}
	}

	for (ObjectsArray<Constraint>::iterator constraint(constraints.begin());
		 constraint != constraints.end();
		 ++constraint)
	{
		defineConstraint(tdbb, dsqlScratch, transaction, *constraint);
	}

	dsqlScratch->relation->rel_flags &= ~REL_creating;

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_CREATE_TABLE, name);

	savePoint.release();	// everything is ok

	// Update DSQL cache
	METD_drop_relation(transaction, name);
	MET_dsql_cache_release(tdbb, SYM_relation, name);
}

// Starting from the elements in a table definition, locate the PK columns if given in a
// separate table constraint declaration.
const ObjectsArray<MetaName>* CreateRelationNode::findPkColumns()
{
	for (const NestConst<Clause>* i = clauses.begin(); i != clauses.end(); ++i)
	{
		if ((*i)->type == Clause::TYPE_ADD_CONSTRAINT)
		{
			const AddConstraintClause* clause = static_cast<const AddConstraintClause*>(i->getObject());

			if (clause->constraintType == AddConstraintClause::CTYPE_PK)
				return &clause->columns;
		}
	}

	return NULL;
}


//----------------------


void AlterRelationNode::print(string& text) const
{
	text.printf(
		"AlterRelationNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void AlterRelationNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_788;	// gds__utility 
   } jrd_787;
   struct {
          SSHORT jrd_786;	// gds__utility 
   } jrd_785;
   struct {
          SSHORT jrd_784;	// gds__utility 
   } jrd_783;
   struct {
          TEXT  jrd_781 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_782 [32];	// RDB$CONSTRAINT_NAME 
   } jrd_780;
   struct {
          SSHORT jrd_794;	// gds__utility 
          SSHORT jrd_795;	// RDB$FIELD_POSITION 
   } jrd_793;
   struct {
          TEXT  jrd_791 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_792 [32];	// RDB$FIELD_NAME 
   } jrd_790;
   struct {
          SSHORT jrd_810;	// gds__utility 
   } jrd_809;
   struct {
          TEXT  jrd_806 [32];	// RDB$GENERATOR_NAME 
          SSHORT jrd_807;	// gds__null_flag 
          SSHORT jrd_808;	// RDB$NULL_FLAG 
   } jrd_805;
   struct {
          TEXT  jrd_801 [32];	// RDB$GENERATOR_NAME 
          SSHORT jrd_802;	// gds__utility 
          SSHORT jrd_803;	// RDB$NULL_FLAG 
          SSHORT jrd_804;	// gds__null_flag 
   } jrd_800;
   struct {
          TEXT  jrd_798 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_799 [32];	// RDB$FIELD_NAME 
   } jrd_797;
   struct {
          SSHORT jrd_821;	// gds__utility 
   } jrd_820;
   struct {
          TEXT  jrd_819 [32];	// RDB$FIELD_NAME 
   } jrd_818;
   struct {
          TEXT  jrd_816 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_817;	// gds__utility 
   } jrd_815;
   struct {
          TEXT  jrd_813 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_814 [32];	// RDB$FIELD_NAME 
   } jrd_812;
	saveRelation(tdbb, dsqlScratch, name, false, false);

	if (!dsqlScratch->relation)
	{
		//// TODO: <Missing arg #1 - possibly status vector overflow>
		/***
		char linecol[64];
		sprintf(linecol, "At line %d, column %d.", (int) dsqlNode->line, (int) dsqlNode->column);
		***/

		status_exception::raise(
			Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
			Arg::Gds(isc_dsql_relation_err) <<
			Arg::Gds(isc_random) << name /***<<
			Arg::Gds(isc_random) << linecol***/);
	}

	// If there is an error, get rid of the cached data.

	try
	{
		// run all statements under savepoint control
		AutoSavePoint savePoint(tdbb, transaction);

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_ALTER_TABLE, name);

		ObjectsArray<Constraint> constraints;

		for (NestConst<Clause>* i = clauses.begin(); i != clauses.end(); ++i)
		{
			switch ((*i)->type)
			{
				case Clause::TYPE_ADD_COLUMN:
					defineField(tdbb, dsqlScratch, transaction,
						static_cast<AddColumnClause*>(i->getObject()), -1, NULL);
					break;

				case Clause::TYPE_ALTER_COL_TYPE:
					modifyField(tdbb, dsqlScratch, transaction,
						static_cast<AlterColTypeClause*>(i->getObject()));
					break;

				case Clause::TYPE_ALTER_COL_NAME:
				{
					const AlterColNameClause* clause =
						static_cast<const AlterColNameClause*>(i->getObject());
					AutoRequest request;
					bool found = false;

					/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
						RFL IN RDB$RELATION_FIELDS
						WITH RFL.RDB$FIELD_NAME EQ clause->fromName.c_str() AND
							 RFL.RDB$RELATION_NAME EQ name.c_str()*/
					{
					request.compile(tdbb, (UCHAR*) jrd_811, sizeof(jrd_811));
					gds__vtov ((const char*) name.c_str(), (char*) jrd_812.jrd_813, 32);
					gds__vtov ((const char*) clause->fromName.c_str(), (char*) jrd_812.jrd_814, 32);
					EXE_start (tdbb, request, transaction);
					EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_812);
					while (1)
					   {
					   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_815);
					   if (!jrd_815.jrd_817) break;
					{
						found = true;

						/*MODIFY RFL*/
						{
						
							checkViewDependency(tdbb, transaction, name, clause->fromName);
							checkSpTrigDependency(tdbb, transaction, name, clause->fromName);

							if (!fieldExists(tdbb, transaction, name, clause->toName))
							{
								strcpy(/*RFL.RDB$FIELD_NAME*/
								       jrd_815.jrd_816, clause->toName.c_str());
								AlterDomainNode::modifyLocalFieldIndex(tdbb, transaction, name,
									clause->fromName, clause->toName);
							}
							else
							{
								// msg 205: Cannot rename field %s to %s.  A field with that name
								// already exists in table %s.
								status_exception::raise(
									Arg::PrivateDyn(205) << clause->fromName << clause->toName << name);
							}
						/*END_MODIFY*/
						gds__vtov((const char*) jrd_815.jrd_816, (char*) jrd_818.jrd_819, 32);
						EXE_send (tdbb, request, 2, 32, (UCHAR*) &jrd_818);
						}
					}
					/*END_FOR*/
					   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_820);
					   }
					}

					if (!found)
					{
						// msg 176: "column %s does not exist in table/view %s"
						status_exception::raise(Arg::PrivateDyn(176) << clause->fromName << name);
					}

					break;
				}

				case Clause::TYPE_ALTER_COL_NULL:
				{
					const AlterColNullClause* clause =
						static_cast<const AlterColNullClause*>(i->getObject());

					//// FIXME: This clause allows inconsistencies like setting a field with a
					//// not null CONSTRAINT as nullable, or setting a field based on a not null
					//// domain as nullable (while ISQL shows it as not null).

					AutoRequest request;
					bool found = false;

					/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
						RFL IN RDB$RELATION_FIELDS
						WITH RFL.RDB$FIELD_NAME EQ clause->name.c_str() AND
							 RFL.RDB$RELATION_NAME EQ name.c_str()*/
					{
					request.compile(tdbb, (UCHAR*) jrd_796, sizeof(jrd_796));
					gds__vtov ((const char*) name.c_str(), (char*) jrd_797.jrd_798, 32);
					gds__vtov ((const char*) clause->name.c_str(), (char*) jrd_797.jrd_799, 32);
					EXE_start (tdbb, request, transaction);
					EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_797);
					while (1)
					   {
					   EXE_receive (tdbb, request, 1, 38, (UCHAR*) &jrd_800);
					   if (!jrd_800.jrd_802) break;
					{
						found = true;

						/*MODIFY RFL*/
						{
						
							if (!clause->notNullFlag && !/*RFL.RDB$GENERATOR_NAME.NULL*/
										     jrd_800.jrd_804)
							{
								// msg 274: Identity column @1 of table @2 cannot be changed to NULLable
								status_exception::raise(Arg::PrivateDyn(274) << clause->name << name);
							}

							/*RFL.RDB$NULL_FLAG*/
							jrd_800.jrd_803 = SSHORT(clause->notNullFlag);
						/*END_MODIFY*/
						gds__vtov((const char*) jrd_800.jrd_801, (char*) jrd_805.jrd_806, 32);
						jrd_805.jrd_807 = jrd_800.jrd_804;
						jrd_805.jrd_808 = jrd_800.jrd_803;
						EXE_send (tdbb, request, 2, 36, (UCHAR*) &jrd_805);
						}
					}
					/*END_FOR*/
					   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_809);
					   }
					}

					if (!found)
					{
						// msg 176: "column %s does not exist in table/view %s"
						status_exception::raise(Arg::PrivateDyn(176) << clause->name << name);
					}

					break;
				}

				case Clause::TYPE_ALTER_COL_POS:
				{
					const AlterColPosClause* clause =
						static_cast<const AlterColPosClause*>(i->getObject());
					// CVC: Since now the parser accepts pos=1..N, let's subtract one here.
					const SSHORT pos = clause->newPos - 1;

					AutoRequest request;
					bool found = false;
					SSHORT oldPos;

					/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
						RFL IN RDB$RELATION_FIELDS
						WITH RFL.RDB$FIELD_NAME EQ clause->name.c_str() AND
							 RFL.RDB$RELATION_NAME EQ name.c_str()*/
					{
					request.compile(tdbb, (UCHAR*) jrd_789, sizeof(jrd_789));
					gds__vtov ((const char*) name.c_str(), (char*) jrd_790.jrd_791, 32);
					gds__vtov ((const char*) clause->name.c_str(), (char*) jrd_790.jrd_792, 32);
					EXE_start (tdbb, request, transaction);
					EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_790);
					while (1)
					   {
					   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_793);
					   if (!jrd_793.jrd_794) break;
					{
						found = true;
						oldPos = /*RFL.RDB$FIELD_POSITION*/
							 jrd_793.jrd_795;
					}
					/*END_FOR*/
					   }
					}

					if (!found)
					{
						// msg 176: "column %s does not exist in table/view %s"
						status_exception::raise(Arg::PrivateDyn(176) << clause->name << name);
					}

					if (pos != oldPos)
						modifyLocalFieldPosition(tdbb, transaction, name, clause->name, pos, oldPos);

					break;
				}

				case Clause::TYPE_DROP_COLUMN:
				{
					// Fix for bug 8054:
					// [CASCADE | RESTRICT] syntax is available in IB4.5, but not
					// required until v5.0.
					//
					// Option CASCADE causes an error: unsupported DSQL construct.
					// Option RESTRICT is default behaviour.

					const DropColumnClause* clause =
						static_cast<const DropColumnClause*>(i->getObject());

					if (clause->cascade)
					{
						// Unsupported DSQL construct
						status_exception::raise(
							Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
							Arg::Gds(isc_dsql_command_err) <<
							Arg::Gds(isc_dsql_construct_err));
					}

					deleteLocalField(tdbb, transaction, name, clause->name);
					break;
				}

				case Clause::TYPE_ADD_CONSTRAINT:
					makeConstraint(tdbb, dsqlScratch, transaction,
						static_cast<AddConstraintClause*>(i->getObject()), constraints);
					break;

				case Clause::TYPE_DROP_CONSTRAINT:
				{
					const MetaName& constraintName =
						static_cast<const DropConstraintClause*>(i->getObject())->name;
					AutoCacheRequest request(tdbb, drq_e_rel_con, DYN_REQUESTS);
					bool found = false;

					/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
						RC IN RDB$RELATION_CONSTRAINTS
						WITH RC.RDB$CONSTRAINT_NAME EQ constraintName.c_str() AND
								RC.RDB$RELATION_NAME EQ name.c_str()*/
					{
					request.compile(tdbb, (UCHAR*) jrd_779, sizeof(jrd_779));
					gds__vtov ((const char*) name.c_str(), (char*) jrd_780.jrd_781, 32);
					gds__vtov ((const char*) constraintName.c_str(), (char*) jrd_780.jrd_782, 32);
					EXE_start (tdbb, request, transaction);
					EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_780);
					while (1)
					   {
					   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_783);
					   if (!jrd_783.jrd_784) break;
					{
						found = true;
						/*ERASE RC;*/
						EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_785);
					}
					/*END_FOR*/
					   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_787);
					   }
					}

					if (!found)
					{
						// msg 130: "CONSTRAINT %s does not exist."
						status_exception::raise(Arg::PrivateDyn(130) << constraintName);
					}

					break;
				}

				default:
					fb_assert(false);
					break;
			}
		}

		for (ObjectsArray<Constraint>::iterator constraint(constraints.begin());
			 constraint != constraints.end();
			 ++constraint)
		{
			defineConstraint(tdbb, dsqlScratch, transaction, *constraint);
		}

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_ALTER_TABLE, name);

		savePoint.release();	// everything is ok

		// Update DSQL cache
		METD_drop_relation(transaction, name);
		MET_dsql_cache_release(tdbb, SYM_relation, name);
	}
	catch (const Exception&)
	{
		METD_drop_relation(transaction, name);
		dsqlScratch->relation = NULL;
		throw;
	}
}

// Modify a field, as part of an alter table statement.
//
// If there are dependencies on the field, abort the operation
// unless the dependency is an index.  In this case, rebuild the
// index once the operation has completed.
//
// If the original datatype of the field was a domain:
//    if the new type is a domain, just make the change to the new domain
//    if it exists
//
//    if the new type is a base type, just make the change
//
// If the original datatype of the field was a base type:
//    if the new type is a base type, just make the change
//
//    if the new type is a domain, make the change to the field
//    definition and remove the entry for RDB$FIELD_SOURCE from the original
//    field.  In other words ... clean up after ourselves
//
// The following conversions are not allowed:
//       Blob to anything
//       Array to anything
//       Date to anything
//       Char to any numeric
//       Varchar to any numeric
//       Anything to Blob
//       Anything to Array
void AlterRelationNode::modifyField(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction, AlterColTypeClause* clause)
{
   struct {
          SSHORT jrd_666;	// gds__utility 
   } jrd_665;
   struct {
          TEXT  jrd_664 [32];	// RDB$FIELD_SOURCE 
   } jrd_663;
   struct {
          TEXT  jrd_661 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_662;	// gds__utility 
   } jrd_660;
   struct {
          TEXT  jrd_658 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_659 [32];	// RDB$RELATION_NAME 
   } jrd_657;
   struct {
          SSHORT jrd_677;	// gds__utility 
   } jrd_676;
   struct {
          TEXT  jrd_675 [32];	// RDB$FIELD_SOURCE 
   } jrd_674;
   struct {
          TEXT  jrd_672 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_673;	// gds__utility 
   } jrd_671;
   struct {
          TEXT  jrd_669 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_670 [32];	// RDB$RELATION_NAME 
   } jrd_668;
   struct {
          ISC_INT64  jrd_682;	// RDB$INITIAL_VALUE 
          SSHORT jrd_683;	// gds__utility 
          SSHORT jrd_684;	// gds__null_flag 
          SSHORT jrd_685;	// RDB$GENERATOR_ID 
   } jrd_681;
   struct {
          TEXT  jrd_680 [32];	// RDB$GENERATOR_NAME 
   } jrd_679;
   struct {
          TEXT  jrd_773 [32];	// RDB$GENERATOR_NAME 
          bid  jrd_774;	// RDB$DEFAULT_SOURCE 
          bid  jrd_775;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_776;	// gds__null_flag 
          SSHORT jrd_777;	// gds__null_flag 
          SSHORT jrd_778;	// gds__null_flag 
   } jrd_772;
   struct {
          bid  jrd_768;	// RDB$DEFAULT_SOURCE 
          bid  jrd_769;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_770;	// gds__null_flag 
          SSHORT jrd_771;	// gds__null_flag 
   } jrd_767;
   struct {
          SSHORT jrd_766;	// gds__utility 
   } jrd_765;
   struct {
          SSHORT jrd_749;	// RDB$FIELD_TYPE 
          SSHORT jrd_750;	// gds__null_flag 
          SSHORT jrd_751;	// RDB$FIELD_SCALE 
          SSHORT jrd_752;	// RDB$FIELD_LENGTH 
          SSHORT jrd_753;	// gds__null_flag 
          SSHORT jrd_754;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_755;	// gds__null_flag 
          SSHORT jrd_756;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_757;	// gds__null_flag 
          SSHORT jrd_758;	// RDB$COLLATION_ID 
          SSHORT jrd_759;	// gds__null_flag 
          SSHORT jrd_760;	// RDB$FIELD_PRECISION 
          SSHORT jrd_761;	// gds__null_flag 
          SSHORT jrd_762;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_763;	// gds__null_flag 
          SSHORT jrd_764;	// RDB$SEGMENT_LENGTH 
   } jrd_748;
   struct {
          TEXT  jrd_742 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_743;	// gds__null_flag 
          SSHORT jrd_744;	// gds__null_flag 
          SSHORT jrd_745;	// RDB$UPDATE_FLAG 
          SSHORT jrd_746;	// gds__null_flag 
          SSHORT jrd_747;	// RDB$COLLATION_ID 
   } jrd_741;
   struct {
          SSHORT jrd_740;	// gds__utility 
   } jrd_739;
   struct {
          bid  jrd_735;	// RDB$COMPUTED_BLR 
          bid  jrd_736;	// RDB$COMPUTED_SOURCE 
          SSHORT jrd_737;	// gds__null_flag 
          SSHORT jrd_738;	// gds__null_flag 
   } jrd_734;
   struct {
          bid  jrd_691;	// RDB$COMPUTED_SOURCE 
          TEXT  jrd_692 [32];	// RDB$FIELD_NAME 
          bid  jrd_693;	// RDB$DEFAULT_VALUE 
          bid  jrd_694;	// RDB$DEFAULT_VALUE 
          bid  jrd_695;	// RDB$DEFAULT_SOURCE 
          TEXT  jrd_696 [32];	// RDB$GENERATOR_NAME 
          TEXT  jrd_697 [32];	// RDB$BASE_FIELD 
          TEXT  jrd_698 [32];	// RDB$FIELD_SOURCE 
          bid  jrd_699;	// RDB$COMPUTED_BLR 
          bid  jrd_700;	// RDB$VIEW_BLR 
          SSHORT jrd_701;	// gds__utility 
          SSHORT jrd_702;	// gds__null_flag 
          SSHORT jrd_703;	// gds__null_flag 
          SSHORT jrd_704;	// RDB$COLLATION_ID 
          SSHORT jrd_705;	// gds__null_flag 
          SSHORT jrd_706;	// RDB$UPDATE_FLAG 
          SSHORT jrd_707;	// gds__null_flag 
          SSHORT jrd_708;	// gds__null_flag 
          SSHORT jrd_709;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_710;	// gds__null_flag 
          SSHORT jrd_711;	// gds__null_flag 
          SSHORT jrd_712;	// gds__null_flag 
          SSHORT jrd_713;	// gds__null_flag 
          SSHORT jrd_714;	// gds__null_flag 
          SSHORT jrd_715;	// gds__null_flag 
          SSHORT jrd_716;	// gds__null_flag 
          SSHORT jrd_717;	// gds__null_flag 
          SSHORT jrd_718;	// gds__null_flag 
          SSHORT jrd_719;	// gds__null_flag 
          SSHORT jrd_720;	// gds__null_flag 
          SSHORT jrd_721;	// RDB$DIMENSIONS 
          SSHORT jrd_722;	// gds__null_flag 
          SSHORT jrd_723;	// RDB$NULL_FLAG 
          SSHORT jrd_724;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_725;	// RDB$FIELD_PRECISION 
          SSHORT jrd_726;	// RDB$COLLATION_ID 
          SSHORT jrd_727;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_728;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_729;	// RDB$FIELD_LENGTH 
          SSHORT jrd_730;	// RDB$FIELD_SCALE 
          SSHORT jrd_731;	// RDB$FIELD_TYPE 
          SSHORT jrd_732;	// gds__null_flag 
          SSHORT jrd_733;	// gds__null_flag 
   } jrd_690;
   struct {
          TEXT  jrd_688 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_689 [32];	// RDB$RELATION_NAME 
   } jrd_687;
	Attachment* const attachment = transaction->tra_attachment;

	dsql_fld* field = clause->field;

	// Add the field to the relation being defined for parsing purposes.
	bool permanent = false;
	dsql_rel* relation = dsqlScratch->relation;

	if (relation)
	{
		if (!(relation->rel_flags & REL_new_relation))
		{
			dsql_fld* permField = FB_NEW(dsqlScratch->getAttachment()->dbb_pool)
				dsql_fld(dsqlScratch->getAttachment()->dbb_pool);
			*permField = *field;

			field = permField;
			permanent = true;
		}

		field->fld_next = relation->rel_fields;
		relation->rel_fields = field;
	}

	try
	{
		bool found = false;
		AutoRequest request;

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			RFR IN RDB$RELATION_FIELDS CROSS
			REL IN RDB$RELATIONS CROSS
			FLD IN RDB$FIELDS
			WITH REL.RDB$RELATION_NAME = RFR.RDB$RELATION_NAME AND
				 FLD.RDB$FIELD_NAME = RFR.RDB$FIELD_SOURCE AND
				 RFR.RDB$RELATION_NAME = name.c_str() AND
				 RFR.RDB$FIELD_NAME = field->fld_name.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_686, sizeof(jrd_686));
		gds__vtov ((const char*) field->fld_name.c_str(), (char*) jrd_687.jrd_688, 32);
		gds__vtov ((const char*) name.c_str(), (char*) jrd_687.jrd_689, 32);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_687);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 242, (UCHAR*) &jrd_690);
		   if (!jrd_690.jrd_701) break;
		{
			found = true;

			bool isView = !/*REL.RDB$VIEW_BLR.NULL*/
				       jrd_690.jrd_733;

			if (!isView && (!/*FLD.RDB$COMPUTED_BLR.NULL*/
					 jrd_690.jrd_732 != (clause->computed != NULL)))
			{
				// Cannot add or remove COMPUTED from column @1
				status_exception::raise(Arg::PrivateDyn(249) << field->fld_name);
			}

			dyn_fld origDom;

			DSC_make_descriptor(&origDom.dyn_dsc, /*FLD.RDB$FIELD_TYPE*/
							      jrd_690.jrd_731, /*FLD.RDB$FIELD_SCALE*/
  jrd_690.jrd_730,
				/*FLD.RDB$FIELD_LENGTH*/
				jrd_690.jrd_729, /*FLD.RDB$FIELD_SUB_TYPE*/
  jrd_690.jrd_728, /*FLD.RDB$CHARACTER_SET_ID*/
  jrd_690.jrd_727,
				/*FLD.RDB$COLLATION_ID*/
				jrd_690.jrd_726);

			origDom.dyn_fld_name = field->fld_name;
			origDom.dyn_charbytelen = /*FLD.RDB$FIELD_LENGTH*/
						  jrd_690.jrd_729;
			origDom.dyn_dtype = /*FLD.RDB$FIELD_TYPE*/
					    jrd_690.jrd_731;
			origDom.dyn_precision = /*FLD.RDB$FIELD_PRECISION*/
						jrd_690.jrd_725;
			origDom.dyn_sub_type = /*FLD.RDB$FIELD_SUB_TYPE*/
					       jrd_690.jrd_728;
			origDom.dyn_charlen = /*FLD.RDB$CHARACTER_LENGTH*/
					      jrd_690.jrd_724;
			origDom.dyn_collation = /*FLD.RDB$COLLATION_ID*/
						jrd_690.jrd_726;
			origDom.dyn_null_flag = !/*FLD.RDB$NULL_FLAG.NULL*/
						 jrd_690.jrd_722 && /*FLD.RDB$NULL_FLAG*/
    jrd_690.jrd_723 != 0;
			origDom.dyn_fld_source = /*RFR.RDB$FIELD_SOURCE*/
						 jrd_690.jrd_698;

			// If the original field type is an array, force its blr type to blr_blob.
			const bool hasDimensions = /*FLD.RDB$DIMENSIONS*/
						   jrd_690.jrd_721 != 0;
			if (hasDimensions)
				origDom.dyn_dtype = blr_blob;

			const bool wasInternalDomain = fb_utils::implicit_domain(origDom.dyn_fld_source.c_str()) &&
				/*RFR.RDB$BASE_FIELD.NULL*/
				jrd_690.jrd_720;
			string computedSource;
			BlrDebugWriter::BlrData computedValue;

			if (clause->computed)
			{
				defineComputed(dsqlScratch, dsqlNode, field, clause->computed, computedSource,
					computedValue);
			}

			if (clause->defaultValue)
			{
				/*MODIFY RFR*/
				{
				
					if (!/*RFR.RDB$GENERATOR_NAME.NULL*/
					     jrd_690.jrd_719)
					{
						// msg 275: Identity column @1 of table @2 cannot have default value
						status_exception::raise(Arg::PrivateDyn(275) << field->fld_name << name);
					}

					if (hasDimensions)
					{
						// msg 225: "Default value is not allowed for array type in field %s"
						status_exception::raise(Arg::PrivateDyn(225) << field->fld_name);
					}

					if (clause->computed)
					{
						// msg 233: "Local column %s is computed, cannot set a default value"
						status_exception::raise(Arg::PrivateDyn(233) << field->fld_name);
					}

					string defaultSource;
					BlrDebugWriter::BlrData defaultValue;

					defineDefault(tdbb, dsqlScratch, field, clause->defaultValue,
						defaultSource, defaultValue);

					/*RFR.RDB$DEFAULT_SOURCE.NULL*/
					jrd_690.jrd_718 = FALSE;
					attachment->storeMetaDataBlob(tdbb, transaction,
						&/*RFR.RDB$DEFAULT_SOURCE*/
						 jrd_690.jrd_695, defaultSource);

					/*RFR.RDB$DEFAULT_VALUE.NULL*/
					jrd_690.jrd_717 = FALSE;
					attachment->storeBinaryBlob(tdbb, transaction,
						&/*RFR.RDB$DEFAULT_VALUE*/
						 jrd_690.jrd_694, defaultValue);
				/*END_MODIFY*/
				gds__vtov((const char*) jrd_690.jrd_696, (char*) jrd_772.jrd_773, 32);
				jrd_772.jrd_774 = jrd_690.jrd_695;
				jrd_772.jrd_775 = jrd_690.jrd_694;
				jrd_772.jrd_776 = jrd_690.jrd_719;
				jrd_772.jrd_777 = jrd_690.jrd_718;
				jrd_772.jrd_778 = jrd_690.jrd_717;
				EXE_send (tdbb, request, 8, 54, (UCHAR*) &jrd_772);
				}
			}
			else if (clause->dropDefault)
			{
				/*MODIFY RFR*/
				{
				
					if (/*RFR.RDB$DEFAULT_VALUE.NULL*/
					    jrd_690.jrd_717)
					{
						if (/*FLD.RDB$DEFAULT_VALUE.NULL*/
						    jrd_690.jrd_716)
						{
							// msg 229: "Local column %s doesn't have a default"
							status_exception::raise(Arg::PrivateDyn(229) << field->fld_name);
						}
						else
						{
							// msg 230: "Local column %s default belongs to domain %s"
							status_exception::raise(
								Arg::PrivateDyn(230) <<
									field->fld_name << MetaName(/*FLD.RDB$FIELD_NAME*/
												    jrd_690.jrd_692));
						}
					}
					else
					{
						/*RFR.RDB$DEFAULT_SOURCE.NULL*/
						jrd_690.jrd_718 = TRUE;
						/*RFR.RDB$DEFAULT_VALUE.NULL*/
						jrd_690.jrd_717 = TRUE;
					}
				/*END_MODIFY*/
				jrd_767.jrd_768 = jrd_690.jrd_695;
				jrd_767.jrd_769 = jrd_690.jrd_694;
				jrd_767.jrd_770 = jrd_690.jrd_718;
				jrd_767.jrd_771 = jrd_690.jrd_717;
				EXE_send (tdbb, request, 7, 20, (UCHAR*) &jrd_767);
				}
			}
			else if (clause->identityRestart)
			{
				bool found = false;
				AutoRequest request2;

				/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
					GEN IN RDB$GENERATORS
					WITH GEN.RDB$GENERATOR_NAME EQ RFR.RDB$GENERATOR_NAME*/
				{
				request2.compile(tdbb, (UCHAR*) jrd_678, sizeof(jrd_678));
				gds__vtov ((const char*) jrd_690.jrd_696, (char*) jrd_679.jrd_680, 32);
				EXE_start (tdbb, request2, transaction);
				EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_679);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 14, (UCHAR*) &jrd_681);
				   if (!jrd_681.jrd_683) break;
				{
					const SLONG id = /*GEN.RDB$GENERATOR_ID*/
							 jrd_681.jrd_685;
					const MetaName genName(/*RFR.RDB$GENERATOR_NAME*/
							       jrd_690.jrd_696);
					const SINT64 val = clause->identityRestartValue.specified ?
						clause->identityRestartValue.value :
						(!/*GEN.RDB$INITIAL_VALUE.NULL*/
						  jrd_681.jrd_684 ? /*GEN.RDB$INITIAL_VALUE*/
   jrd_681.jrd_682 : 0);

					transaction->getGenIdCache()->put(id, val);
					dsc desc;
					desc.makeText((USHORT) genName.length(), ttype_metadata,
						(UCHAR*) genName.c_str());
					DFW_post_work(transaction, dfw_set_generator, &desc, id);

					found = true;
				}
				/*END_FOR*/
				   }
				}

				if (!found)
				{
					// msg 285: "Column @1 is not an identity column"
					status_exception::raise(Arg::PrivateDyn(285) << field->fld_name);
				}
			}
			else
			{
				// We have the type. Default and type/domain are exclusive for now.

				MetaName newDomainName;
				dyn_fld newDom;

				if (field->typeOfName.hasData())
				{
					// Case a1: Internal domain -> domain.
					// Case a2: Domain -> domain.

					newDomainName = field->typeOfName;

					if (fb_utils::implicit_domain(newDomainName.c_str()))
					{
						// msg 224: "Cannot use the internal domain %s as new type for field %s".
						status_exception::raise(
							Arg::PrivateDyn(224) << newDomainName << field->fld_name);
					}

					// Get the domain information.
					if (!METD_get_domain(dsqlScratch->getTransaction(), field, newDomainName))
					{
						// Specified domain or source field does not exist.
						status_exception::raise(
							Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
							Arg::Gds(isc_dsql_command_err) <<
							Arg::Gds(isc_dsql_domain_not_found) << newDomainName);
					}

					DDL_resolve_intl_type(dsqlScratch, field, NULL);

					// If the original definition was a base field type, remove the
					// entries from RDB$FIELDS.
					if (wasInternalDomain)
					{
						// Case a1: Internal domain -> domain.
						/*ERASE FLD;*/
						EXE_send (tdbb, request, 6, 2, (UCHAR*) &jrd_765);
					}
				}
				else
				{
					// Case b1: Internal domain -> internal domain.
					// Case b2: Domain -> internal domain.

					// If COMPUTED was specified but the type wasn't, we use the type of
					// the computed expression.
					if (clause->computed && field->dtype == dtype_unknown)
					{
						dsc desc;
						MAKE_desc(dsqlScratch, &desc, clause->computed->value);

						field->dtype = desc.dsc_dtype;
						field->length = desc.dsc_length;
						field->scale = desc.dsc_scale;

						if (field->dtype <= dtype_any_text)
						{
							field->charSetId = DSC_GET_CHARSET(&desc);
							field->collationId = DSC_GET_COLLATE(&desc);
						}
						else
							field->subType = desc.dsc_sub_type;
					}

					field->resolve(dsqlScratch, true);

					if (wasInternalDomain)	// Case b1: Internal domain -> internal domain.
					{
						/*MODIFY FLD*/
						{
						
							updateRdbFields(field,
								/*FLD.RDB$FIELD_TYPE*/
								jrd_690.jrd_731,
								/*FLD.RDB$FIELD_LENGTH*/
								jrd_690.jrd_729,
								/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
								jrd_690.jrd_715, /*FLD.RDB$FIELD_SUB_TYPE*/
  jrd_690.jrd_728,
								/*FLD.RDB$FIELD_SCALE.NULL*/
								jrd_690.jrd_714, /*FLD.RDB$FIELD_SCALE*/
  jrd_690.jrd_730,
								/*FLD.RDB$CHARACTER_SET_ID.NULL*/
								jrd_690.jrd_713, /*FLD.RDB$CHARACTER_SET_ID*/
  jrd_690.jrd_727,
								/*FLD.RDB$CHARACTER_LENGTH.NULL*/
								jrd_690.jrd_712, /*FLD.RDB$CHARACTER_LENGTH*/
  jrd_690.jrd_724,
								/*FLD.RDB$FIELD_PRECISION.NULL*/
								jrd_690.jrd_711, /*FLD.RDB$FIELD_PRECISION*/
  jrd_690.jrd_725,
								/*FLD.RDB$COLLATION_ID.NULL*/
								jrd_690.jrd_710, /*FLD.RDB$COLLATION_ID*/
  jrd_690.jrd_726,
								/*FLD.RDB$SEGMENT_LENGTH.NULL*/
								jrd_690.jrd_708, /*FLD.RDB$SEGMENT_LENGTH*/
  jrd_690.jrd_709);
						/*END_MODIFY*/
						jrd_748.jrd_749 = jrd_690.jrd_731;
						jrd_748.jrd_750 = jrd_690.jrd_714;
						jrd_748.jrd_751 = jrd_690.jrd_730;
						jrd_748.jrd_752 = jrd_690.jrd_729;
						jrd_748.jrd_753 = jrd_690.jrd_715;
						jrd_748.jrd_754 = jrd_690.jrd_728;
						jrd_748.jrd_755 = jrd_690.jrd_713;
						jrd_748.jrd_756 = jrd_690.jrd_727;
						jrd_748.jrd_757 = jrd_690.jrd_710;
						jrd_748.jrd_758 = jrd_690.jrd_726;
						jrd_748.jrd_759 = jrd_690.jrd_711;
						jrd_748.jrd_760 = jrd_690.jrd_725;
						jrd_748.jrd_761 = jrd_690.jrd_712;
						jrd_748.jrd_762 = jrd_690.jrd_724;
						jrd_748.jrd_763 = jrd_690.jrd_708;
						jrd_748.jrd_764 = jrd_690.jrd_709;
						EXE_send (tdbb, request, 5, 32, (UCHAR*) &jrd_748);
						}

						newDom.dyn_fld_source = origDom.dyn_fld_source;
					}
					else	// Case b2: Domain -> internal domain.
						storeGlobalField(tdbb, transaction, newDomainName, field);
				}

				if (!clause->computed && !isView)
				{
					if (newDomainName.hasData())
						newDom.dyn_fld_source = newDomainName;

					AlterDomainNode::getDomainType(tdbb, transaction, newDom);
					AlterDomainNode::checkUpdate(origDom, newDom);

					if (!/*RFR.RDB$GENERATOR_NAME.NULL*/
					     jrd_690.jrd_719)
					{
						if (!newDom.dyn_dsc.isExact() || newDom.dyn_dsc.dsc_scale != 0)
						{
							// Identity column @1 of table @2 must be exact numeric with zero scale.
							status_exception::raise(Arg::PrivateDyn(273) << field->fld_name << name);
						}
					}
				}

				if (newDomainName.hasData())
				{
					/*MODIFY RFR USING*/
					{
					
						/*RFR.RDB$FIELD_SOURCE.NULL*/
						jrd_690.jrd_707 = FALSE;
						strcpy(/*RFR.RDB$FIELD_SOURCE*/
						       jrd_690.jrd_698, newDomainName.c_str());

						if (clause->computed)
						{
							/*RFR.RDB$UPDATE_FLAG.NULL*/
							jrd_690.jrd_705 = FALSE;
							/*RFR.RDB$UPDATE_FLAG*/
							jrd_690.jrd_706 = 1;
						}

						/*RFR.RDB$COLLATION_ID.NULL*/
						jrd_690.jrd_703 = TRUE;	// CORE-2426
					/*END_MODIFY*/
					gds__vtov((const char*) jrd_690.jrd_698, (char*) jrd_741.jrd_742, 32);
					jrd_741.jrd_743 = jrd_690.jrd_707;
					jrd_741.jrd_744 = jrd_690.jrd_705;
					jrd_741.jrd_745 = jrd_690.jrd_706;
					jrd_741.jrd_746 = jrd_690.jrd_703;
					jrd_741.jrd_747 = jrd_690.jrd_704;
					EXE_send (tdbb, request, 4, 42, (UCHAR*) &jrd_741);
					}
				}
			}

			if (clause->computed)
			{
				// We can alter FLD directly here because if we are setting a computed expression,
				// it means the field already was computed. And if it was, it should be the
				// "b1 case", where the field source does not change.
				// This assumption may change, especially when this function starts dealing
				// with views.

				/*MODIFY FLD*/
				{
				
					/*FLD.RDB$COMPUTED_SOURCE.NULL*/
					jrd_690.jrd_702 = FALSE;
					attachment->storeMetaDataBlob(tdbb, transaction, &/*FLD.RDB$COMPUTED_SOURCE*/
											  jrd_690.jrd_691,
						computedSource);

					/*FLD.RDB$COMPUTED_BLR.NULL*/
					jrd_690.jrd_732 = FALSE;
					attachment->storeBinaryBlob(tdbb, transaction, &/*FLD.RDB$COMPUTED_BLR*/
											jrd_690.jrd_699,
						computedValue);
				/*END_MODIFY*/
				jrd_734.jrd_735 = jrd_690.jrd_699;
				jrd_734.jrd_736 = jrd_690.jrd_691;
				jrd_734.jrd_737 = jrd_690.jrd_732;
				jrd_734.jrd_738 = jrd_690.jrd_702;
				EXE_send (tdbb, request, 2, 20, (UCHAR*) &jrd_734);
				}
			}

			AutoRequest request2;

			/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
				PRM IN RDB$PROCEDURE_PARAMETERS
				WITH PRM.RDB$RELATION_NAME = name.c_str() AND
					 PRM.RDB$FIELD_NAME = field->fld_name.c_str()*/
			{
			request2.compile(tdbb, (UCHAR*) jrd_667, sizeof(jrd_667));
			gds__vtov ((const char*) field->fld_name.c_str(), (char*) jrd_668.jrd_669, 32);
			gds__vtov ((const char*) name.c_str(), (char*) jrd_668.jrd_670, 32);
			EXE_start (tdbb, request2, transaction);
			EXE_send (tdbb, request2, 0, 64, (UCHAR*) &jrd_668);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 34, (UCHAR*) &jrd_671);
			   if (!jrd_671.jrd_673) break;
			{
				/*MODIFY PRM USING*/
				{
				
					strcpy(/*PRM.RDB$FIELD_SOURCE*/
					       jrd_671.jrd_672, /*RFR.RDB$FIELD_SOURCE*/
  jrd_690.jrd_698);
				/*END_MODIFY*/
				gds__vtov((const char*) jrd_671.jrd_672, (char*) jrd_674.jrd_675, 32);
				EXE_send (tdbb, request2, 2, 32, (UCHAR*) &jrd_674);
				}
			}
			/*END_FOR*/
			   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_676);
			   }
			}

			request2.reset();

			/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
				ARG IN RDB$FUNCTION_ARGUMENTS
				WITH ARG.RDB$RELATION_NAME = name.c_str() AND
					 ARG.RDB$FIELD_NAME = field->fld_name.c_str()*/
			{
			request2.compile(tdbb, (UCHAR*) jrd_656, sizeof(jrd_656));
			gds__vtov ((const char*) field->fld_name.c_str(), (char*) jrd_657.jrd_658, 32);
			gds__vtov ((const char*) name.c_str(), (char*) jrd_657.jrd_659, 32);
			EXE_start (tdbb, request2, transaction);
			EXE_send (tdbb, request2, 0, 64, (UCHAR*) &jrd_657);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 34, (UCHAR*) &jrd_660);
			   if (!jrd_660.jrd_662) break;
			{
				/*MODIFY ARG USING*/
				{
				
					strcpy(/*ARG.RDB$FIELD_SOURCE*/
					       jrd_660.jrd_661, /*RFR.RDB$FIELD_SOURCE*/
  jrd_690.jrd_698);
				/*END_MODIFY*/
				gds__vtov((const char*) jrd_660.jrd_661, (char*) jrd_663.jrd_664, 32);
				EXE_send (tdbb, request2, 2, 32, (UCHAR*) &jrd_663);
				}
			}
			/*END_FOR*/
			   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_665);
			   }
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_739);
		   }
		}

		if (!found)
		{
			// msg 176: "column %s does not exist in table/view %s"
			status_exception::raise(Arg::PrivateDyn(176) << field->fld_name << name);
		}

		// Update any indices that exist.
		AlterDomainNode::modifyLocalFieldIndex(tdbb, transaction, name,
			field->fld_name, field->fld_name);
	}
	catch (const Exception&)
	{
		clearPermanentField(relation, permanent);
		throw;
	}

	clearPermanentField(relation, permanent);
}


//----------------------


// Delete a global field if it's not used in others objects.
void DropRelationNode::deleteGlobalField(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& globalName)
{
   struct {
          SSHORT jrd_655;	// gds__utility 
   } jrd_654;
   struct {
          SSHORT jrd_653;	// gds__utility 
   } jrd_652;
   struct {
          SSHORT jrd_651;	// gds__utility 
   } jrd_650;
   struct {
          TEXT  jrd_648 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_649 [32];	// RDB$FIELD_NAME 
   } jrd_647;
	AutoCacheRequest request(tdbb, drq_e_l_gfld, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FLD IN RDB$FIELDS
		WITH FLD.RDB$FIELD_NAME EQ globalName.c_str() AND
			 FLD.RDB$VALIDATION_SOURCE MISSING AND
			 FLD.RDB$NULL_FLAG MISSING AND
			 FLD.RDB$DEFAULT_SOURCE MISSING AND
			 FLD.RDB$FIELD_NAME STARTING WITH IMPLICIT_DOMAIN_PREFIX AND
			 (NOT ANY RFR IN RDB$RELATION_FIELDS WITH
				RFR.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME) AND
			 (NOT ANY PRM IN RDB$PROCEDURE_PARAMETERS WITH
				PRM.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME) AND
			 (NOT ANY ARG IN RDB$FUNCTION_ARGUMENTS WITH
				ARG.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME)*/
	{
	request.compile(tdbb, (UCHAR*) jrd_646, sizeof(jrd_646));
	gds__vtov ((const char*) IMPLICIT_DOMAIN_PREFIX, (char*) jrd_647.jrd_648, 32);
	gds__vtov ((const char*) globalName.c_str(), (char*) jrd_647.jrd_649, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_647);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_650);
	   if (!jrd_650.jrd_651) break;
	{
		DropDomainNode::deleteDimensionRecords(tdbb, transaction, globalName);
		/*ERASE FLD;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_652);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_654);
	   }
	}
}

void DropRelationNode::print(string& text) const
{
	text.printf(
		"DropRelationNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void DropRelationNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_534;	// gds__utility 
   } jrd_533;
   struct {
          SSHORT jrd_532;	// gds__utility 
   } jrd_531;
   struct {
          SSHORT jrd_530;	// gds__utility 
   } jrd_529;
   struct {
          TEXT  jrd_527 [32];	// RDB$USER 
          SSHORT jrd_528;	// RDB$USER_TYPE 
   } jrd_526;
   struct {
          SSHORT jrd_544;	// gds__utility 
   } jrd_543;
   struct {
          SSHORT jrd_542;	// gds__utility 
   } jrd_541;
   struct {
          SSHORT jrd_540;	// gds__utility 
   } jrd_539;
   struct {
          TEXT  jrd_537 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_538;	// RDB$OBJECT_TYPE 
   } jrd_536;
   struct {
          SSHORT jrd_554;	// gds__utility 
   } jrd_553;
   struct {
          SSHORT jrd_552;	// gds__utility 
   } jrd_551;
   struct {
          SSHORT jrd_550;	// gds__utility 
   } jrd_549;
   struct {
          TEXT  jrd_547 [32];	// RDB$USER 
          SSHORT jrd_548;	// RDB$USER_TYPE 
   } jrd_546;
   struct {
          SSHORT jrd_564;	// gds__utility 
   } jrd_563;
   struct {
          SSHORT jrd_562;	// gds__utility 
   } jrd_561;
   struct {
          TEXT  jrd_559 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_560;	// gds__utility 
   } jrd_558;
   struct {
          TEXT  jrd_557 [32];	// RDB$RELATION_NAME 
   } jrd_556;
   struct {
          SSHORT jrd_575;	// gds__utility 
   } jrd_574;
   struct {
          SSHORT jrd_573;	// gds__utility 
   } jrd_572;
   struct {
          TEXT  jrd_569 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_570;	// gds__utility 
          SSHORT jrd_571;	// gds__null_flag 
   } jrd_568;
   struct {
          TEXT  jrd_567 [32];	// RDB$RELATION_NAME 
   } jrd_566;
   struct {
          SSHORT jrd_584;	// gds__utility 
   } jrd_583;
   struct {
          SSHORT jrd_582;	// gds__utility 
   } jrd_581;
   struct {
          SSHORT jrd_580;	// gds__utility 
   } jrd_579;
   struct {
          TEXT  jrd_578 [32];	// RDB$VIEW_NAME 
   } jrd_577;
   struct {
          SSHORT jrd_598;	// gds__utility 
   } jrd_597;
   struct {
          SSHORT jrd_596;	// gds__utility 
   } jrd_595;
   struct {
          TEXT  jrd_589 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_590 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_591 [32];	// RDB$GENERATOR_NAME 
          SSHORT jrd_592;	// gds__utility 
          SSHORT jrd_593;	// gds__null_flag 
          SSHORT jrd_594;	// gds__null_flag 
   } jrd_588;
   struct {
          TEXT  jrd_587 [32];	// RDB$RELATION_NAME 
   } jrd_586;
   struct {
          SSHORT jrd_609;	// gds__utility 
   } jrd_608;
   struct {
          SSHORT jrd_607;	// gds__utility 
   } jrd_606;
   struct {
          SSHORT jrd_605;	// gds__utility 
   } jrd_604;
   struct {
          TEXT  jrd_601 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_602 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_603 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_600;
   struct {
          SSHORT jrd_618;	// gds__utility 
   } jrd_617;
   struct {
          SSHORT jrd_616;	// gds__utility 
   } jrd_615;
   struct {
          SSHORT jrd_614;	// gds__utility 
   } jrd_613;
   struct {
          TEXT  jrd_612 [32];	// RDB$RELATION_NAME 
   } jrd_611;
   struct {
          SSHORT jrd_628;	// gds__utility 
   } jrd_627;
   struct {
          SSHORT jrd_626;	// gds__utility 
   } jrd_625;
   struct {
          TEXT  jrd_623 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_624;	// gds__utility 
   } jrd_622;
   struct {
          TEXT  jrd_621 [32];	// RDB$RELATION_NAME 
   } jrd_620;
   struct {
          SSHORT jrd_640;	// gds__utility 
   } jrd_639;
   struct {
          SSHORT jrd_638;	// gds__utility 
   } jrd_637;
   struct {
          SSHORT jrd_636;	// gds__utility 
   } jrd_635;
   struct {
          TEXT  jrd_631 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_632 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_633 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_634 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_630;
   struct {
          SSHORT jrd_645;	// gds__utility 
   } jrd_644;
   struct {
          TEXT  jrd_643 [32];	// RDB$RELATION_NAME 
   } jrd_642;
	jrd_rel* rel_drop = MET_lookup_relation(tdbb, name);
	if (rel_drop)
		MET_scan_relation(tdbb, rel_drop);

	const dsql_rel* relation = METD_get_relation(transaction, dsqlScratch, name);

	if (!relation && silent)
		return;

	// Check that DROP TABLE is dropping a table and that DROP VIEW is dropping a view.
	if (view)
	{
		if (!relation || (relation && !(relation->rel_flags & REL_view)))
		{
			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_dsql_view_not_found) << name);
		}
	}
	else
	{
		if (!relation || (relation && (relation->rel_flags & REL_view)))
		{
			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_dsql_table_not_found) << name);
		}
	}

	const int ddlTriggerAction = (view ? DDL_TRIGGER_DROP_VIEW : DDL_TRIGGER_DROP_TABLE);

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_l_relation, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		R IN RDB$RELATIONS
		WITH R.RDB$RELATION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_641, sizeof(jrd_641));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_642.jrd_643, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_642);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_644);
	   if (!jrd_644.jrd_645) break;
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, ddlTriggerAction, name);
		found = true;
	}
	/*END_FOR*/
	   }
	}

	request.reset(tdbb, drq_e_rel_con2, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		CRT IN RDB$RELATION_CONSTRAINTS
		WITH CRT.RDB$RELATION_NAME EQ name.c_str() AND
			 (CRT.RDB$CONSTRAINT_TYPE EQ PRIMARY_KEY OR
			  CRT.RDB$CONSTRAINT_TYPE EQ UNIQUE_CNSTRT OR
			  CRT.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY)
		SORTED BY ASCENDING CRT.RDB$CONSTRAINT_TYPE*/
	{
	request.compile(tdbb, (UCHAR*) jrd_629, sizeof(jrd_629));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_630.jrd_631, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_630.jrd_632, 12);
	gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_630.jrd_633, 12);
	gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_630.jrd_634, 12);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_630);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_635);
	   if (!jrd_635.jrd_636) break;
	{
		/*ERASE CRT;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_637);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_639);
	   }
	}

	request.reset(tdbb, drq_e_rel_idxs, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES
		WITH IDX.RDB$RELATION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_619, sizeof(jrd_619));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_620.jrd_621, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_620);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_622);
	   if (!jrd_622.jrd_624) break;
	{
		DropIndexNode::deleteSegmentRecords(tdbb, transaction, /*IDX.RDB$INDEX_NAME*/
								       jrd_622.jrd_623);
		/*ERASE IDX;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_625);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_627);
	   }
	}

	request.reset(tdbb, drq_e_trg_msgs2, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		TM IN RDB$TRIGGER_MESSAGES
		CROSS T IN RDB$TRIGGERS
		WITH T.RDB$RELATION_NAME EQ name.c_str() AND
			 TM.RDB$TRIGGER_NAME EQ T.RDB$TRIGGER_NAME*/
	{
	request.compile(tdbb, (UCHAR*) jrd_610, sizeof(jrd_610));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_611.jrd_612, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_611);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_613);
	   if (!jrd_613.jrd_614) break;
	{
		/*ERASE TM;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_615);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_617);
	   }
	}

	// CVC: Moved this block here to avoid SF Bug #1111570.
	request.reset(tdbb, drq_e_rel_con3, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		CRT IN RDB$RELATION_CONSTRAINTS
		WITH CRT.RDB$RELATION_NAME EQ name.c_str() AND
			 (CRT.RDB$CONSTRAINT_TYPE EQ CHECK_CNSTRT OR
			  CRT.RDB$CONSTRAINT_TYPE EQ NOT_NULL_CNSTRT)*/
	{
	request.compile(tdbb, (UCHAR*) jrd_599, sizeof(jrd_599));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_600.jrd_601, 32);
	gds__vtov ((const char*) NOT_NULL_CNSTRT, (char*) jrd_600.jrd_602, 12);
	gds__vtov ((const char*) CHECK_CNSTRT, (char*) jrd_600.jrd_603, 12);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 56, (UCHAR*) &jrd_600);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_604);
	   if (!jrd_604.jrd_605) break;
	{
		/*ERASE CRT;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_606);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_608);
	   }
	}

	request.reset(tdbb, drq_e_rel_flds, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		RFR IN RDB$RELATION_FIELDS
		WITH RFR.RDB$RELATION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_585, sizeof(jrd_585));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_586.jrd_587, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_586);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 102, (UCHAR*) &jrd_588);
	   if (!jrd_588.jrd_592) break;
	{
		if (!/*RFR.RDB$GENERATOR_NAME.NULL*/
		     jrd_588.jrd_594)
			DropSequenceNode::deleteIdentity(tdbb, transaction, /*RFR.RDB$GENERATOR_NAME*/
									    jrd_588.jrd_591);

		/*ERASE RFR;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_595);

		if (!/*RFR.RDB$SECURITY_CLASS.NULL*/
		     jrd_588.jrd_593 &&
			!strncmp(/*RFR.RDB$SECURITY_CLASS*/
				 jrd_588.jrd_590, SQL_SECCLASS_PREFIX, SQL_SECCLASS_PREFIX_LEN))
		{
			deleteSecurityClass(tdbb, transaction, /*RFR.RDB$SECURITY_CLASS*/
							       jrd_588.jrd_590);
		}

		deleteGlobalField(tdbb, transaction, /*RFR.RDB$FIELD_SOURCE*/
						     jrd_588.jrd_589);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_597);
	   }
	}

	request.reset(tdbb, drq_e_view_rels, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		VR IN RDB$VIEW_RELATIONS
		WITH VR.RDB$VIEW_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_576, sizeof(jrd_576));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_577.jrd_578, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_577);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_579);
	   if (!jrd_579.jrd_580) break;
	{
		/*ERASE VR;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_581);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_583);
	   }
	}

	request.reset(tdbb, drq_e_relation, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		R IN RDB$RELATIONS
		WITH R.RDB$RELATION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_565, sizeof(jrd_565));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_566.jrd_567, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_566);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_568);
	   if (!jrd_568.jrd_570) break;
	{
		/*ERASE R;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_572);

		if (!/*R.RDB$SECURITY_CLASS.NULL*/
		     jrd_568.jrd_571 &&
			!strncmp(/*R.RDB$SECURITY_CLASS*/
				 jrd_568.jrd_569, SQL_SECCLASS_PREFIX, SQL_SECCLASS_PREFIX_LEN))
		{
			deleteSecurityClass(tdbb, transaction, /*R.RDB$SECURITY_CLASS*/
							       jrd_568.jrd_569);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_574);
	   }
	}

	if (!found)
	{
		// msg 61: "Relation not found"
		status_exception::raise(Arg::PrivateDyn(61));
	}

	// Triggers must be deleted after check constraints

	MetaName triggerName;

	request.reset(tdbb, drq_e_trigger2, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$TRIGGERS
		WITH X.RDB$RELATION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_555, sizeof(jrd_555));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_556.jrd_557, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_556);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_558);
	   if (!jrd_558.jrd_560) break;
	{
		triggerName = /*X.RDB$TRIGGER_NAME*/
			      jrd_558.jrd_559;
		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_561);

		AutoCacheRequest request2(tdbb, drq_e_trg_prv, DYN_REQUESTS);

		/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
			PRIV IN RDB$USER_PRIVILEGES
			WITH PRIV.RDB$USER EQ triggerName.c_str() AND
					PRIV.RDB$USER_TYPE = obj_trigger*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_545, sizeof(jrd_545));
		gds__vtov ((const char*) triggerName.c_str(), (char*) jrd_546.jrd_547, 32);
		jrd_546.jrd_548 = obj_trigger;
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 34, (UCHAR*) &jrd_546);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 2, (UCHAR*) &jrd_549);
		   if (!jrd_549.jrd_550) break;
		{
			/*ERASE PRIV;*/
			EXE_send (tdbb, request2, 2, 2, (UCHAR*) &jrd_551);
		}
		/*END_FOR*/
		   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_553);
		   }
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_563);
	   }
	}

	request.reset(tdbb, drq_e_usr_prvs, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH PRIV.RDB$RELATION_NAME EQ name.c_str() AND
			 PRIV.RDB$OBJECT_TYPE = obj_relation*/
	{
	request.compile(tdbb, (UCHAR*) jrd_535, sizeof(jrd_535));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_536.jrd_537, 32);
	jrd_536.jrd_538 = obj_relation;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_536);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_539);
	   if (!jrd_539.jrd_540) break;
	{
		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_541);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_543);
	   }
	}

	request.reset(tdbb, drq_e_view_prv, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH PRIV.RDB$USER EQ name.c_str() AND
			 PRIV.RDB$USER_TYPE = obj_view*/
	{
	request.compile(tdbb, (UCHAR*) jrd_525, sizeof(jrd_525));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_526.jrd_527, 32);
	jrd_526.jrd_528 = obj_view;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_526);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_529);
	   if (!jrd_529.jrd_530) break;
	{
		/*ERASE PRIV;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_531);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_533);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, ddlTriggerAction, name);
	else
	{
		// msg 61: "Relation not found"
		status_exception::raise(Arg::PrivateDyn(61));
	}

	savePoint.release();	// everything is ok

	METD_drop_relation(transaction, name.c_str());
	MET_dsql_cache_release(tdbb, SYM_relation, name);
}


//----------------------


void CreateAlterViewNode::print(string& text) const
{
	text.printf(
		"CreateAlterViewNode\n"
		"  name: '%s'\n",
		name.c_str());
}

DdlNode* CreateAlterViewNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	source.ltrim("\n\r\t ");
	return DdlNode::dsqlPass(dsqlScratch);
}

void CreateAlterViewNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_458;	// gds__utility 
   } jrd_457;
   struct {
          bid  jrd_439;	// RDB$COMPUTED_BLR 
          SSHORT jrd_440;	// RDB$FIELD_TYPE 
          SSHORT jrd_441;	// RDB$FIELD_LENGTH 
          SSHORT jrd_442;	// gds__null_flag 
          SSHORT jrd_443;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_444;	// gds__null_flag 
          SSHORT jrd_445;	// RDB$FIELD_SCALE 
          SSHORT jrd_446;	// gds__null_flag 
          SSHORT jrd_447;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_448;	// gds__null_flag 
          SSHORT jrd_449;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_450;	// gds__null_flag 
          SSHORT jrd_451;	// RDB$FIELD_PRECISION 
          SSHORT jrd_452;	// gds__null_flag 
          SSHORT jrd_453;	// RDB$COLLATION_ID 
          SSHORT jrd_454;	// gds__null_flag 
          SSHORT jrd_455;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_456;	// gds__null_flag 
   } jrd_438;
   struct {
          bid  jrd_418;	// RDB$COMPUTED_BLR 
          TEXT  jrd_419 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_420;	// gds__utility 
          SSHORT jrd_421;	// gds__null_flag 
          SSHORT jrd_422;	// gds__null_flag 
          SSHORT jrd_423;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_424;	// gds__null_flag 
          SSHORT jrd_425;	// RDB$COLLATION_ID 
          SSHORT jrd_426;	// gds__null_flag 
          SSHORT jrd_427;	// RDB$FIELD_PRECISION 
          SSHORT jrd_428;	// gds__null_flag 
          SSHORT jrd_429;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_430;	// gds__null_flag 
          SSHORT jrd_431;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_432;	// gds__null_flag 
          SSHORT jrd_433;	// RDB$FIELD_SCALE 
          SSHORT jrd_434;	// gds__null_flag 
          SSHORT jrd_435;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_436;	// RDB$FIELD_LENGTH 
          SSHORT jrd_437;	// RDB$FIELD_TYPE 
   } jrd_417;
   struct {
          TEXT  jrd_415 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_416 [32];	// RDB$FIELD_NAME 
   } jrd_414;
   struct {
          SSHORT jrd_469;	// gds__utility 
   } jrd_468;
   struct {
          SSHORT jrd_467;	// gds__utility 
   } jrd_466;
   struct {
          TEXT  jrd_464 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_465;	// gds__utility 
   } jrd_463;
   struct {
          TEXT  jrd_461 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_462 [32];	// RDB$FIELD_NAME 
   } jrd_460;
   struct {
          TEXT  jrd_474 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_475 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_476;	// gds__utility 
   } jrd_473;
   struct {
          TEXT  jrd_472 [32];	// RDB$VIEW_NAME 
   } jrd_471;
   struct {
          TEXT  jrd_479 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_480 [256];	// RDB$CONTEXT_NAME 
          TEXT  jrd_481 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_482 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_483;	// gds__null_flag 
          SSHORT jrd_484;	// RDB$VIEW_CONTEXT 
          SSHORT jrd_485;	// RDB$CONTEXT_TYPE 
   } jrd_478;
   struct {
          bid  jrd_488;	// RDB$VIEW_BLR 
          bid  jrd_489;	// RDB$VIEW_SOURCE 
          TEXT  jrd_490 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_491;	// RDB$RELATION_TYPE 
          SSHORT jrd_492;	// RDB$FLAGS 
          SSHORT jrd_493;	// RDB$SYSTEM_FLAG 
   } jrd_487;
   struct {
          SSHORT jrd_503;	// gds__utility 
   } jrd_502;
   struct {
          SSHORT jrd_501;	// gds__utility 
   } jrd_500;
   struct {
          SSHORT jrd_499;	// gds__utility 
   } jrd_498;
   struct {
          TEXT  jrd_496 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_497;	// RDB$SYSTEM_FLAG 
   } jrd_495;
   struct {
          SSHORT jrd_512;	// gds__utility 
   } jrd_511;
   struct {
          SSHORT jrd_510;	// gds__utility 
   } jrd_509;
   struct {
          SSHORT jrd_508;	// gds__utility 
   } jrd_507;
   struct {
          TEXT  jrd_506 [32];	// RDB$VIEW_NAME 
   } jrd_505;
   struct {
          SSHORT jrd_524;	// gds__utility 
   } jrd_523;
   struct {
          bid  jrd_521;	// RDB$VIEW_SOURCE 
          bid  jrd_522;	// RDB$VIEW_BLR 
   } jrd_520;
   struct {
          bid  jrd_517;	// RDB$VIEW_BLR 
          bid  jrd_518;	// RDB$VIEW_SOURCE 
          SSHORT jrd_519;	// gds__utility 
   } jrd_516;
   struct {
          TEXT  jrd_515 [32];	// RDB$RELATION_NAME 
   } jrd_514;
	Attachment* const attachment = transaction->tra_attachment;
	const string& userName = attachment->att_user->usr_user_name;

	const dsql_rel* modifyingView = NULL;

	if (alter)
	{
		modifyingView = METD_get_relation(dsqlScratch->getTransaction(), dsqlScratch, name);

		if (!modifyingView && !create)
			status_exception::raise(Arg::Gds(isc_dyn_view_not_found) << name);
	}

	saveRelation(tdbb, dsqlScratch, name, true, modifyingView == NULL);

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	const int ddlTriggerAction = (modifyingView ? DDL_TRIGGER_ALTER_VIEW : DDL_TRIGGER_CREATE_VIEW);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, ddlTriggerAction, name);

	if (!modifyingView)
		DYN_UTIL_check_unique_name(tdbb, transaction, name, obj_relation);

	// Compile the SELECT statement into a record selection expression, making sure to bump the
	// context number since view contexts start at 1 (except for computed fields) -- note that
	// calling PASS1_rse directly rather than PASS1_statement saves the context stack.

	dsqlScratch->resetContextStack();
	++dsqlScratch->contextNumber;
	RseNode* rse = PASS1_rse(dsqlScratch, selectExpr, false);

	dsqlScratch->getBlrData().clear();
	dsqlScratch->appendUChar(dsqlScratch->isVersion4() ? blr_version4 : blr_version5);

	GEN_expr(dsqlScratch, rse);
	dsqlScratch->appendUChar(blr_eoc);

	// Store the blr and source string for the view definition.

	if (modifyingView)
	{
		AutoCacheRequest request(tdbb, drq_m_view, DYN_REQUESTS);
		bool found = false;

		/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			REL IN RDB$RELATIONS
			WITH REL.RDB$RELATION_NAME EQ name.c_str() AND
				 REL.RDB$VIEW_BLR NOT MISSING*/
		{
		request.compile(tdbb, (UCHAR*) jrd_513, sizeof(jrd_513));
		gds__vtov ((const char*) name.c_str(), (char*) jrd_514.jrd_515, 32);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_514);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 18, (UCHAR*) &jrd_516);
		   if (!jrd_516.jrd_519) break;
		{
			found = true;

			/*MODIFY REL*/
			{
			
				attachment->storeMetaDataBlob(tdbb, transaction, &/*REL.RDB$VIEW_SOURCE*/
										  jrd_516.jrd_518, source);
				attachment->storeBinaryBlob(tdbb, transaction, &/*REL.RDB$VIEW_BLR*/
										jrd_516.jrd_517,
					dsqlScratch->getBlrData());
			/*END_MODIFY*/
			jrd_520.jrd_521 = jrd_516.jrd_518;
			jrd_520.jrd_522 = jrd_516.jrd_517;
			EXE_send (tdbb, request, 2, 16, (UCHAR*) &jrd_520);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_523);
		   }
		}

		if (!found)
			status_exception::raise(Arg::Gds(isc_dyn_view_not_found) << name);

		AutoRequest request2;

		/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
			VR IN RDB$VIEW_RELATIONS
			WITH VR.RDB$VIEW_NAME EQ name.c_str()*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_504, sizeof(jrd_504));
		gds__vtov ((const char*) name.c_str(), (char*) jrd_505.jrd_506, 32);
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_505);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 2, (UCHAR*) &jrd_507);
		   if (!jrd_507.jrd_508) break;
		{
			/*ERASE VR;*/
			EXE_send (tdbb, request2, 2, 2, (UCHAR*) &jrd_509);
		}
		/*END_FOR*/
		   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_511);
		   }
		}

		request2.reset();

		/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
			TRG IN RDB$TRIGGERS
			WITH TRG.RDB$RELATION_NAME EQ name.c_str() AND
				 TRG.RDB$SYSTEM_FLAG EQ fb_sysflag_view_check*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_494, sizeof(jrd_494));
		gds__vtov ((const char*) name.c_str(), (char*) jrd_495.jrd_496, 32);
		jrd_495.jrd_497 = fb_sysflag_view_check;
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 34, (UCHAR*) &jrd_495);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 2, (UCHAR*) &jrd_498);
		   if (!jrd_498.jrd_499) break;
		{
			/*ERASE TRG;*/
			EXE_send (tdbb, request2, 2, 2, (UCHAR*) &jrd_500);
		}
		/*END_FOR*/
		   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_502);
		   }
		}
	}
	else
	{
		AutoCacheRequest request(tdbb, drq_s_rels, DYN_REQUESTS);

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			REL IN RDB$RELATIONS*/
		{
		
		{
			strcpy(/*REL.RDB$RELATION_NAME*/
			       jrd_487.jrd_490, name.c_str());
			/*REL.RDB$SYSTEM_FLAG*/
			jrd_487.jrd_493 = 0;
			/*REL.RDB$FLAGS*/
			jrd_487.jrd_492 = REL_sql;
			/*REL.RDB$RELATION_TYPE*/
			jrd_487.jrd_491 = SSHORT(rel_view);

			attachment->storeMetaDataBlob(tdbb, transaction, &/*REL.RDB$VIEW_SOURCE*/
									  jrd_487.jrd_489, source);
			attachment->storeBinaryBlob(tdbb, transaction, &/*REL.RDB$VIEW_BLR*/
									jrd_487.jrd_488, dsqlScratch->getBlrData());
		}
		/*END_STORE*/
		request.compile(tdbb, (UCHAR*) jrd_486, sizeof(jrd_486));
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 54, (UCHAR*) &jrd_487);
		}

		storePrivileges(tdbb, transaction, name, obj_relation, ALL_PRIVILEGES);
	}

	// Define the view source relations from the statement contexts and union contexts.

	while (dsqlScratch->derivedContext.hasData())
		dsqlScratch->context->push(dsqlScratch->derivedContext.pop());

	while (dsqlScratch->unionContext.hasData())
		dsqlScratch->context->push(dsqlScratch->unionContext.pop());

	AutoCacheRequest request(tdbb, drq_s_view_rels, DYN_REQUESTS);

	for (DsqlContextStack::iterator temp(*dsqlScratch->context); temp.hasData(); ++temp)
	{
		const dsql_ctx* context = temp.object();
		const dsql_rel* relation = context->ctx_relation;
		const dsql_prc* procedure = context->ctx_procedure;

		if (relation || procedure)
		{
			const MetaName& refName = relation ? relation->rel_name : procedure->prc_name.identifier;
			const char* contextName = context->ctx_alias.hasData() ?
				context->ctx_alias.c_str() : refName.c_str();

			ViewContextType ctxType;
			if (relation)
			{
				if (!(relation->rel_flags & REL_view))
					ctxType = VCT_TABLE;
				else
					ctxType = VCT_VIEW;
			}
			else //if (procedure)
				ctxType = VCT_PROCEDURE;

			/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
				VRL IN RDB$VIEW_RELATIONS*/
			{
			
			{
				strcpy(/*VRL.RDB$VIEW_NAME*/
				       jrd_478.jrd_482, name.c_str());
				strcpy(/*VRL.RDB$RELATION_NAME*/
				       jrd_478.jrd_481, refName.c_str());
				/*VRL.RDB$CONTEXT_TYPE*/
				jrd_478.jrd_485 = SSHORT(ctxType);
				/*VRL.RDB$VIEW_CONTEXT*/
				jrd_478.jrd_484 = context->ctx_context;
				strcpy(/*VRL.RDB$CONTEXT_NAME*/
				       jrd_478.jrd_480, contextName);

				if (procedure && procedure->prc_name.package.hasData())
				{
					/*VRL.RDB$PACKAGE_NAME.NULL*/
					jrd_478.jrd_483 = FALSE;
					strcpy(/*VRL.RDB$PACKAGE_NAME*/
					       jrd_478.jrd_479, procedure->prc_name.package.c_str());
				}
				else
					/*VRL.RDB$PACKAGE_NAME.NULL*/
					jrd_478.jrd_483 = TRUE;
			}
			/*END_STORE*/
			request.compile(tdbb, (UCHAR*) jrd_477, sizeof(jrd_477));
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 358, (UCHAR*) &jrd_478);
			}
		}
	}

	// Check privileges on base tables and views.

	request.reset(tdbb, drq_l_view_rels, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		VRL IN RDB$VIEW_RELATIONS CROSS
		PREL IN RDB$RELATIONS OVER RDB$RELATION_NAME
		WITH VRL.RDB$PACKAGE_NAME MISSING AND
			 VRL.RDB$VIEW_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_470, sizeof(jrd_470));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_471.jrd_472, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_471);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_473);
	   if (!jrd_473.jrd_476) break;
	{
		// CVC: This never matches so it causes unnecessary calls to verify,
		// so I included a call to strip trailing blanks.
		fb_utils::exact_name_limit(/*PREL.RDB$OWNER_NAME*/
					   jrd_473.jrd_475, sizeof(/*PREL.RDB$OWNER_NAME*/
	 jrd_473.jrd_475));

		if (userName != /*PREL.RDB$OWNER_NAME*/
				jrd_473.jrd_475)
		{
			SecurityClass::flags_t priv;

			// I think this should be the responsability of DFW or the user will find ways to
			// circumvent DYN.
			priv = SCL_get_mask(tdbb, /*PREL.RDB$RELATION_NAME*/
						  jrd_473.jrd_474, "");

			if (!(priv & SCL_select))
			{
				// msg 32: no permission for %s access to %s %s
				status_exception::raise(
					Arg::Gds(isc_no_priv) << Arg::Str("SELECT") <<	//	Non-Translatable
					// Remember, a view may be based on a view.
					"TABLE/VIEW" <<	//  Non-Translatable
					// We want to print the name of the base table or view.
					MetaName(/*PREL.RDB$RELATION_NAME*/
						 jrd_473.jrd_474));
			}
		}
	}
	/*END_FOR*/
	   }
	}

	// If there are field names defined for the view, match them in order with the items from the
	// SELECT. Otherwise use all the fields from the rse node that was created from the select
	// expression.

	const NestConst<ValueExprNode>* ptr = NULL;
	const NestConst<ValueExprNode>* end = NULL;

	if (viewFields)
	{
		ptr = viewFields->items.begin();
		end = viewFields->items.end();
	}

	// Go through the fields list, defining or modifying the local fields;
	// If an expression is specified rather than a field, define a global
	// field for the computed value as well.

	ValueListNode* items = rse->dsqlSelectList;
	NestConst<ValueExprNode>* itemsPtr = items->items.begin();
	SortedArray<dsql_fld*> modifiedFields;
	bool updatable = true;
	SSHORT position = 0;

	for (NestConst<ValueExprNode>* itemsEnd = items->items.end();
		 itemsPtr < itemsEnd; ++itemsPtr, ++position)
	{
		ValueExprNode* fieldNode = *itemsPtr;

		// Determine the proper field name, replacing the default if necessary.

		ValueExprNode* nameNode = fieldNode;
		const char* aliasName = NULL;

		while (nameNode->is<DsqlAliasNode>() || nameNode->is<DerivedFieldNode>() ||
			nameNode->is<DsqlMapNode>())
		{
			DsqlAliasNode* aliasNode;
			DsqlMapNode* mapNode;
			DerivedFieldNode* derivedField;

			if ((aliasNode = nameNode->as<DsqlAliasNode>()))
			{
				if (!aliasName)
					aliasName = aliasNode->name.c_str();
				nameNode = aliasNode->value;
			}
			else if ((mapNode = nameNode->as<DsqlMapNode>()))
				nameNode = mapNode->map->map_node;
			else if ((derivedField = nameNode->as<DerivedFieldNode>()))
			{
				if (!aliasName)
					aliasName = derivedField->name.c_str();
				nameNode = derivedField->value;
			}
		}

		const dsql_fld* nameField = NULL;
		const FieldNode* fieldNameNode = nameNode->as<FieldNode>();

		if (fieldNameNode)
			nameField = fieldNameNode->dsqlField;

		const TEXT* fieldStr = NULL;

		if (aliasName)
			fieldStr = aliasName;
		else if (nameField)
			fieldStr = nameField->fld_name.c_str();

		// Check if this is a field or an expression.

		DsqlAliasNode* aliasNode = fieldNode->as<DsqlAliasNode>();

		if (aliasNode)
			fieldNode = aliasNode->value;

		dsql_fld* field = NULL;
		const dsql_ctx* context = NULL;

		fieldNameNode = fieldNode->as<FieldNode>();

		if (fieldNameNode)
		{
			field = fieldNameNode->dsqlField;
			context = fieldNameNode->dsqlContext;
		}
		else
			updatable = false;

		// If this is an expression, check to make sure there is a name specified.

		if (!ptr && !fieldStr)
		{
			// must specify field name for view select expression
			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_specify_field_err));
		}

		// CVC: Small modification here to catch any mismatch between number of
		// explicit field names in a view and number of fields in the select expression,
		// see comment below. This closes Firebird Bug #223059.
		if (ptr)
		{
			if (ptr < end)
				fieldStr = (*ptr)->as<FieldNode>()->dsqlName.c_str();
			else
			{
				// Generate an error when going out of this loop.
				++ptr;
				break;
			}

			++ptr;
		}

		// If not an expression, point to the proper base relation field,
		// else make up an SQL field with generated global field for calculations.

		dsql_fld* relField = NULL;

		if (modifyingView)	// if we're modifying a view
		{
			for (relField = modifyingView->rel_fields; relField; relField = relField->fld_next)
			{
				if (relField->fld_name == fieldStr)
				{
					if (modifiedFields.exist(relField))
					{
						// column @1 appears more than once in ALTER VIEW
						status_exception::raise(
							Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
							Arg::Gds(isc_dsql_command_err) <<
							Arg::Gds(isc_dsql_col_more_than_once_view) << Arg::Str(fieldStr));
					}

					modifiedFields.add(relField);
					break;
				}
			}
		}

		FieldDefinition fieldDefinition(*tdbb->getDefaultPool());
		fieldDefinition.relationName = name;
		fieldDefinition.name = fieldStr;
		fieldDefinition.position = position;

		// CVC: Not sure if something should be done now that isc_dyn_view_context is used here,
		// but if alter view is going to work, maybe we need here the context type and package, too.
		if (field)
		{
			field->resolve(dsqlScratch);

			fieldDefinition.viewContext = context->ctx_context;
			fieldDefinition.baseField = field->fld_name;

			if (field->dtype <= dtype_any_text)
				fieldDefinition.collationId = field->collationId;

			if (relField)	// modifying a view
			{
				// We're now modifying a field and it will be based on another one. So if the old
				// field was an expression, delete it now.

				AutoRequest request2;

				/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
					RFL IN RDB$RELATION_FIELDS CROSS
					FLD IN RDB$FIELDS
					WITH RFL.RDB$FIELD_NAME EQ fieldStr AND
						 RFL.RDB$RELATION_NAME EQ name.c_str() AND
						 RFL.RDB$BASE_FIELD MISSING AND
						 FLD.RDB$FIELD_NAME EQ RFL.RDB$FIELD_SOURCE*/
				{
				request2.compile(tdbb, (UCHAR*) jrd_459, sizeof(jrd_459));
				gds__vtov ((const char*) name.c_str(), (char*) jrd_460.jrd_461, 32);
				gds__vtov ((const char*) fieldStr, (char*) jrd_460.jrd_462, 32);
				EXE_start (tdbb, request2, transaction);
				EXE_send (tdbb, request2, 0, 64, (UCHAR*) &jrd_460);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 34, (UCHAR*) &jrd_463);
				   if (!jrd_463.jrd_465) break;
				{
					bool wasInternalDomain = fb_utils::implicit_domain(/*FLD.RDB$FIELD_NAME*/
											   jrd_463.jrd_464);
					fb_assert(wasInternalDomain);

					if (wasInternalDomain)
						/*ERASE FLD;*/
						EXE_send (tdbb, request2, 2, 2, (UCHAR*) &jrd_466);
				}
				/*END_FOR*/
				   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_468);
				   }
				}

				fieldDefinition.modify(tdbb, transaction);
			}
			else
				fieldDefinition.store(tdbb, transaction);
		}
		else
		{
			dsqlScratch->getBlrData().clear();
			dsqlScratch->appendUChar(dsqlScratch->isVersion4() ? blr_version4 : blr_version5);
			GEN_expr(dsqlScratch, fieldNode);
			dsqlScratch->appendUChar(blr_eoc);

			// Get the type of the expression.
			dsc desc;
			MAKE_desc(dsqlScratch, &desc, fieldNode);

			dsql_fld newField(*tdbb->getDefaultPool());
			newField.dtype = desc.dsc_dtype;
			newField.length = desc.dsc_length;
			newField.scale = desc.dsc_scale;

			if (desc.isText() || (desc.isBlob() && desc.getBlobSubType() == isc_blob_text))
			{
				newField.charSetId = desc.getCharSet();
				newField.collationId = desc.getCollation();
			}

			if (desc.isText())
			{
				newField.charLength = newField.length;
				newField.length *= METD_get_charset_bpc(
					dsqlScratch->getTransaction(), newField.charSetId);
			}
			else
				newField.subType = desc.dsc_sub_type;

			if (relField)	// modifying a view
			{
				AutoRequest request2;

				/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
					RFL IN RDB$RELATION_FIELDS CROSS
					FLD IN RDB$FIELDS
					WITH RFL.RDB$FIELD_NAME EQ fieldStr AND
						 RFL.RDB$RELATION_NAME EQ name.c_str() AND
						 RFL.RDB$BASE_FIELD MISSING AND
						 FLD.RDB$FIELD_NAME EQ RFL.RDB$FIELD_SOURCE*/
				{
				request2.compile(tdbb, (UCHAR*) jrd_413, sizeof(jrd_413));
				gds__vtov ((const char*) name.c_str(), (char*) jrd_414.jrd_415, 32);
				gds__vtov ((const char*) fieldStr, (char*) jrd_414.jrd_416, 32);
				EXE_start (tdbb, request2, transaction);
				EXE_send (tdbb, request2, 0, 64, (UCHAR*) &jrd_414);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 76, (UCHAR*) &jrd_417);
				   if (!jrd_417.jrd_420) break;
				{
					bool wasInternalDomain = fb_utils::implicit_domain(/*FLD.RDB$FIELD_NAME*/
											   jrd_417.jrd_419);
					fb_assert(wasInternalDomain);

					if (wasInternalDomain)
					{
						fieldDefinition.fieldSource = /*FLD.RDB$FIELD_NAME*/
									      jrd_417.jrd_419;

						/*MODIFY FLD*/
						{
						
							updateRdbFields(&newField,
								/*FLD.RDB$FIELD_TYPE*/
								jrd_417.jrd_437,
								/*FLD.RDB$FIELD_LENGTH*/
								jrd_417.jrd_436,
								/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
								jrd_417.jrd_434, /*FLD.RDB$FIELD_SUB_TYPE*/
  jrd_417.jrd_435,
								/*FLD.RDB$FIELD_SCALE.NULL*/
								jrd_417.jrd_432, /*FLD.RDB$FIELD_SCALE*/
  jrd_417.jrd_433,
								/*FLD.RDB$CHARACTER_SET_ID.NULL*/
								jrd_417.jrd_430, /*FLD.RDB$CHARACTER_SET_ID*/
  jrd_417.jrd_431,
								/*FLD.RDB$CHARACTER_LENGTH.NULL*/
								jrd_417.jrd_428, /*FLD.RDB$CHARACTER_LENGTH*/
  jrd_417.jrd_429,
								/*FLD.RDB$FIELD_PRECISION.NULL*/
								jrd_417.jrd_426, /*FLD.RDB$FIELD_PRECISION*/
  jrd_417.jrd_427,
								/*FLD.RDB$COLLATION_ID.NULL*/
								jrd_417.jrd_424, /*FLD.RDB$COLLATION_ID*/
  jrd_417.jrd_425,
								/*FLD.RDB$SEGMENT_LENGTH.NULL*/
								jrd_417.jrd_422, /*FLD.RDB$SEGMENT_LENGTH*/
  jrd_417.jrd_423);

							/*FLD.RDB$COMPUTED_BLR.NULL*/
							jrd_417.jrd_421 = FALSE;
							attachment->storeBinaryBlob(tdbb, transaction, &/*FLD.RDB$COMPUTED_BLR*/
													jrd_417.jrd_418,
								dsqlScratch->getBlrData());
						/*END_MODIFY*/
						jrd_438.jrd_439 = jrd_417.jrd_418;
						jrd_438.jrd_440 = jrd_417.jrd_437;
						jrd_438.jrd_441 = jrd_417.jrd_436;
						jrd_438.jrd_442 = jrd_417.jrd_434;
						jrd_438.jrd_443 = jrd_417.jrd_435;
						jrd_438.jrd_444 = jrd_417.jrd_432;
						jrd_438.jrd_445 = jrd_417.jrd_433;
						jrd_438.jrd_446 = jrd_417.jrd_430;
						jrd_438.jrd_447 = jrd_417.jrd_431;
						jrd_438.jrd_448 = jrd_417.jrd_428;
						jrd_438.jrd_449 = jrd_417.jrd_429;
						jrd_438.jrd_450 = jrd_417.jrd_426;
						jrd_438.jrd_451 = jrd_417.jrd_427;
						jrd_438.jrd_452 = jrd_417.jrd_424;
						jrd_438.jrd_453 = jrd_417.jrd_425;
						jrd_438.jrd_454 = jrd_417.jrd_422;
						jrd_438.jrd_455 = jrd_417.jrd_423;
						jrd_438.jrd_456 = jrd_417.jrd_421;
						EXE_send (tdbb, request2, 2, 42, (UCHAR*) &jrd_438);
						}
					}
				}
				/*END_FOR*/
				   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_457);
				   }
				}

				if (fieldDefinition.fieldSource.isEmpty())
				{
					storeGlobalField(tdbb, transaction, fieldDefinition.fieldSource, &newField,
						"", dsqlScratch->getBlrData());
				}

				fieldDefinition.modify(tdbb, transaction);
			}
			else
			{
				storeGlobalField(tdbb, transaction, fieldDefinition.fieldSource, &newField,
					"", dsqlScratch->getBlrData());

				fieldDefinition.store(tdbb, transaction);
			}
		}

		if (fieldStr)
			saveField(tdbb, dsqlScratch, fieldStr);
	}

	// CVC: This message was not catching the case when
	// #fields < items in select list, see comment above.

	if (ptr != end)
	{
		// number of fields does not match select list
		status_exception::raise(
			Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
			Arg::Gds(isc_dsql_command_err) <<
			Arg::Gds(isc_num_field_err));
	}

	if (modifyingView)	// modifying a view
	{
		// Delete the old fields not present in the new definition.
		for (dsql_fld* relField = modifyingView->rel_fields; relField; relField = relField->fld_next)
		{
			if (!modifiedFields.exist(relField))
				deleteLocalField(tdbb, transaction, name, relField->fld_name);
		}
	}

	// Setup to define triggers for WITH CHECK OPTION.

	if (withCheckOption)
	{
		if (!updatable)
		{
			// Only simple column names permitted for VIEW WITH CHECK OPTION
			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_col_name_err));
		}

		RseNode* querySpec = selectExpr->querySpec->as<RseNode>();
		fb_assert(querySpec);

		if (querySpec->dsqlFrom->items.getCount() != 1 ||
			!querySpec->dsqlFrom->items[0]->is<ProcedureSourceNode>())
		{
			// Only one table allowed for VIEW WITH CHECK OPTION
			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_table_view_err));
		}

		if (!querySpec->dsqlWhere)
		{
			// No where clause for VIEW WITH CHECK OPTION
			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_where_err));
		}

		if (querySpec->dsqlDistinct || querySpec->dsqlGroup || querySpec->dsqlHaving)
		{
			// DISTINCT, GROUP or HAVING not permitted for VIEW WITH CHECK OPTION
			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_distinct_err));
		}

		createCheckTrigger(tdbb, dsqlScratch, items, PRE_MODIFY_TRIGGER);
		createCheckTrigger(tdbb, dsqlScratch, items, PRE_STORE_TRIGGER);
	}

	dsqlScratch->resetContextStack();

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, ddlTriggerAction, name);

	savePoint.release();	// everything is ok

	// Update DSQL cache
	METD_drop_relation(transaction, name);
	MET_dsql_cache_release(tdbb, SYM_relation, name);
}

// Generate a trigger to implement the WITH CHECK OPTION clause for a VIEW.
void CreateAlterViewNode::createCheckTrigger(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	ValueListNode* items, TriggerType triggerType)
{
	MemoryPool& pool = *tdbb->getDefaultPool();

	// Specify that the trigger should abort if the condition is not met.
	ExceptionNode* exceptionNode = FB_NEW(pool) ExceptionNode(pool, CHECK_CONSTRAINT_EXCEPTION);
	exceptionNode->exception->type = ExceptionItem::GDS_CODE;

	AutoSetRestore<bool> autoCheckConstraintTrigger(&dsqlScratch->checkConstraintTrigger, true);

	RelationSourceNode* relationNode = dsqlNode;

	// Generate the trigger blr.

	dsqlScratch->getBlrData().clear();
	dsqlScratch->getDebugData().clear();
	dsqlScratch->appendUChar(dsqlScratch->isVersion4() ? blr_version4 : blr_version5);

	dsqlScratch->appendUChar(blr_begin);

	dsqlScratch->resetContextStack();

	RseNode* querySpec = selectExpr->querySpec->as<RseNode>();
	fb_assert(querySpec);

	ProcedureSourceNode* sourceNode = querySpec->dsqlFrom->items[0]->as<ProcedureSourceNode>();

	if (triggerType == PRE_MODIFY_TRIGGER)
	{
		dsqlScratch->contextNumber = 2;

		RelationSourceNode* baseRelation = FB_NEW(pool) RelationSourceNode(pool,
			sourceNode->dsqlName.identifier);
		baseRelation->alias = sourceNode->alias;

		dsqlScratch->appendUChar(blr_for);

		RseNode* rse = FB_NEW(pool) RseNode(pool);
		rse->dsqlStreams = FB_NEW(pool) RecSourceListNode(pool, 1);

		rse->dsqlStreams->items[0] = baseRelation;
		rse->dsqlStreams->items[0] = doDsqlPass(dsqlScratch, rse->dsqlStreams->items[0]);
		rse->dsqlWhere = doDsqlPass(dsqlScratch, querySpec->dsqlWhere);

		dsqlScratch->contextNumber = OLD_CONTEXT_VALUE;

		dsql_ctx* oldContext;

		{	/// scope
			AutoSetRestore<string> autoAlias(&relationNode->alias, sourceNode->alias);
			relationNode->alias = OLD_CONTEXT_NAME;

			oldContext = PASS1_make_context(dsqlScratch, relationNode);
			oldContext->ctx_flags |= CTX_system;
		}

		// Get the list of values and fields to compare to -- if there is no list of fields, get all
		// fields in the base relation that are not computed.

		ValueListNode* valuesNode = viewFields;
		ValueListNode* fieldsNode = querySpec->dsqlSelectList;

		if (!fieldsNode)
		{
			const dsql_rel* relation = METD_get_relation(dsqlScratch->getTransaction(),
				dsqlScratch, name);
			fieldsNode = FB_NEW(pool) ValueListNode(pool, 0u);

			for (const dsql_fld* field = relation->rel_fields; field; field = field->fld_next)
			{
				if (!(field->flags & FLD_computed))
					fieldsNode->add(MAKE_field_name(field->fld_name.c_str()));
			}
		}

		if (!valuesNode)
			valuesNode = fieldsNode;

		// Generate the list of assignments to fields in the base relation.

		NestConst<ValueExprNode>* ptr = fieldsNode->items.begin();
		const NestConst<ValueExprNode>* const end = fieldsNode->items.end();
		NestConst<ValueExprNode>* ptr2 = valuesNode->items.begin();
		const NestConst<ValueExprNode>* const end2 = valuesNode->items.end();
		int andArg = 0;

		BinaryBoolNode* andNode = FB_NEW(pool) BinaryBoolNode(pool, blr_and);

		for (; ptr != end && ptr2 != end2; ++ptr, ++ptr2)
		{
			NestConst<ValueExprNode> fieldNod = *ptr;
			NestConst<ValueExprNode> valueNod = *ptr2;
			DsqlAliasNode* aliasNode;

			if ((aliasNode = fieldNod->as<DsqlAliasNode>()))
				fieldNod = aliasNode->value;

			if ((aliasNode = valueNod->as<DsqlAliasNode>()))
				valueNod = aliasNode->value;

			FieldNode* fieldNode = fieldNod->as<FieldNode>();
			FieldNode* valueNode = valueNod->as<FieldNode>();

			// Generate the actual comparisons.

			if (fieldNode && valueNode)
			{
				FieldNode* oldValueNode = FB_NEW(pool) FieldNode(pool);
				oldValueNode->dsqlName = (aliasNode ? aliasNode->name : valueNode->dsqlName);
				oldValueNode->dsqlQualifier = OLD_CONTEXT_NAME;

				valueNod = oldValueNode->dsqlPass(dsqlScratch);
				fieldNod = fieldNode->dsqlPass(dsqlScratch);

				BoolExprNode* equivNode = FB_NEW(pool) ComparativeBoolNode(pool, blr_equiv,
					valueNod, fieldNod);

				rse->dsqlWhere = PASS1_compose(rse->dsqlWhere, equivNode, blr_and);
			}
		}

		GEN_expr(dsqlScratch, rse);
	}

	// ASF: We'll now map the view's base table into the trigger's NEW context.

	++dsqlScratch->scopeLevel;
	dsqlScratch->contextNumber = NEW_CONTEXT_VALUE;

	dsql_ctx* newContext;

	{	/// scope
		AutoSetRestore<string> autoAlias(&relationNode->alias, sourceNode->alias);

		if (relationNode->alias.isEmpty())
			relationNode->alias = sourceNode->dsqlName.identifier.c_str();

		newContext = PASS1_make_context(dsqlScratch, relationNode);
		newContext->ctx_flags |= CTX_system;

		if (triggerType == PRE_STORE_TRIGGER)
			newContext->ctx_flags |= CTX_view_with_check_store;
		else
			newContext->ctx_flags |= CTX_view_with_check_modify;
	}

	// Replace the view's field names by the base table field names. Save the original names
	// to restore after the condition processing.

	dsql_fld* field = newContext->ctx_relation->rel_fields;
	ObjectsArray<MetaName> savedNames;

	// ASF: rel_fields entries are in reverse order.
	for (NestConst<ValueExprNode>* ptr = items->items.end();
		 ptr-- != items->items.begin();
		 field = field->fld_next)
	{
		ValueExprNode* valueNode = *ptr;
		DsqlAliasNode* aliasNode;

		if ((aliasNode = valueNode->as<DsqlAliasNode>()))
			valueNode = aliasNode->value;

		FieldNode* fieldNode = valueNode->as<FieldNode>();
		fb_assert(fieldNode);

		savedNames.add(field->fld_name);

		dsql_fld* queryField = fieldNode->dsqlField;

		field->fld_name = queryField->fld_name;
		field->dtype = queryField->dtype;
		field->scale = queryField->scale;
		field->subType = queryField->subType;
		field->length = queryField->length;
		field->flags = queryField->flags;
		field->charSetId = queryField->charSetId;
		field->collationId = queryField->collationId;
	}

	dsqlScratch->appendUChar(blr_if);

	// Process the condition for firing the trigger.
	NestConst<BoolExprNode> condition = doDsqlPass(dsqlScratch, querySpec->dsqlWhere);

	// Restore the field names. This must happen before the condition's BLR generation.

	field = newContext->ctx_relation->rel_fields;

	for (ObjectsArray<MetaName>::iterator i = savedNames.begin(); i != savedNames.end(); ++i)
	{
		field->fld_name = *i;
		field = field->fld_next;
	}

	GEN_expr(dsqlScratch, condition);
	dsqlScratch->appendUChar(blr_begin);
	dsqlScratch->appendUChar(blr_end);

	// Generate the action statement for the trigger.
	exceptionNode->dsqlPass(dsqlScratch)->genBlr(dsqlScratch);

	dsqlScratch->appendUChar(blr_end);	// of begin
	dsqlScratch->appendUChar(blr_eoc);

	dsqlScratch->resetContextStack();

	TriggerDefinition trigger(pool);
	trigger.systemFlag = fb_sysflag_view_check;
	trigger.relationName = name;
	trigger.type = triggerType;
	trigger.blrData = dsqlScratch->getBlrData();
	trigger.store(tdbb, dsqlScratch, dsqlScratch->getTransaction());
}


//----------------------


// Store an index.
void CreateIndexNode::store(thread_db* tdbb, jrd_tra* transaction, MetaName& name,
	const Definition& definition, MetaName* referredIndexName)
{
   struct {
          TEXT  jrd_345 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_346;	// gds__utility 
          SSHORT jrd_347;	// RDB$SEGMENT_COUNT 
   } jrd_344;
   struct {
          TEXT  jrd_342 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_343 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_341;
   struct {
          bid  jrd_352;	// RDB$VIEW_BLR 
          SSHORT jrd_353;	// gds__utility 
          SSHORT jrd_354;	// gds__null_flag 
   } jrd_351;
   struct {
          TEXT  jrd_350 [32];	// RDB$RELATION_NAME 
   } jrd_349;
   struct {
          TEXT  jrd_361 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_362 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_363;	// gds__utility 
   } jrd_360;
   struct {
          TEXT  jrd_357 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_358 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_359 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_356;
   struct {
          TEXT  jrd_366 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_367 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_368;	// RDB$FIELD_POSITION 
   } jrd_365;
   struct {
          bid  jrd_374;	// RDB$COMPUTED_BLR 
          SSHORT jrd_375;	// gds__utility 
          SSHORT jrd_376;	// gds__null_flag 
          SSHORT jrd_377;	// RDB$COLLATION_ID 
          SSHORT jrd_378;	// RDB$FIELD_LENGTH 
          SSHORT jrd_379;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_380;	// gds__null_flag 
          SSHORT jrd_381;	// RDB$COLLATION_ID 
          SSHORT jrd_382;	// gds__null_flag 
          SSHORT jrd_383;	// gds__null_flag 
          SSHORT jrd_384;	// RDB$DIMENSIONS 
          SSHORT jrd_385;	// RDB$FIELD_TYPE 
   } jrd_373;
   struct {
          TEXT  jrd_371 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_372 [32];	// RDB$FIELD_NAME 
   } jrd_370;
   struct {
          bid  jrd_390;	// RDB$VIEW_BLR 
          SSHORT jrd_391;	// gds__utility 
          SSHORT jrd_392;	// gds__null_flag 
   } jrd_389;
   struct {
          TEXT  jrd_388 [32];	// RDB$RELATION_NAME 
   } jrd_387;
   struct {
          TEXT  jrd_395 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_396 [32];	// RDB$INDEX_NAME 
          bid  jrd_397;	// RDB$EXPRESSION_BLR 
          bid  jrd_398;	// RDB$EXPRESSION_SOURCE 
          TEXT  jrd_399 [32];	// RDB$FOREIGN_KEY 
          SSHORT jrd_400;	// RDB$SEGMENT_COUNT 
          SSHORT jrd_401;	// gds__null_flag 
          SSHORT jrd_402;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_403;	// gds__null_flag 
          SSHORT jrd_404;	// gds__null_flag 
          SSHORT jrd_405;	// gds__null_flag 
          SSHORT jrd_406;	// gds__null_flag 
          SSHORT jrd_407;	// gds__null_flag 
          SSHORT jrd_408;	// RDB$INDEX_TYPE 
          SSHORT jrd_409;	// gds__null_flag 
          SSHORT jrd_410;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_411;	// gds__null_flag 
          SSHORT jrd_412;	// RDB$UNIQUE_FLAG 
   } jrd_394;
	if (name.isEmpty())
		DYN_UTIL_generate_index_name(tdbb, transaction, name, definition.type);

	DYN_UTIL_check_unique_name(tdbb, transaction, name, obj_index);

	AutoCacheRequest request(tdbb, drq_s_indices, DYN_REQUESTS);

	ULONG keyLength = 0;

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES*/
	{
	
	{
		/*IDX.RDB$UNIQUE_FLAG.NULL*/
		jrd_394.jrd_411 = TRUE;
		/*IDX.RDB$INDEX_INACTIVE.NULL*/
		jrd_394.jrd_409 = TRUE;
		/*IDX.RDB$INDEX_TYPE.NULL*/
		jrd_394.jrd_407 = TRUE;
		/*IDX.RDB$FOREIGN_KEY.NULL*/
		jrd_394.jrd_406 = TRUE;
		/*IDX.RDB$EXPRESSION_SOURCE.NULL*/
		jrd_394.jrd_405 = TRUE;
		/*IDX.RDB$EXPRESSION_BLR.NULL*/
		jrd_394.jrd_404 = TRUE;
		strcpy(/*IDX.RDB$INDEX_NAME*/
		       jrd_394.jrd_396, name.c_str());
		strcpy(/*IDX.RDB$RELATION_NAME*/
		       jrd_394.jrd_395, definition.relation.c_str());
		/*IDX.RDB$RELATION_NAME.NULL*/
		jrd_394.jrd_403 = FALSE;
		/*IDX.RDB$SYSTEM_FLAG*/
		jrd_394.jrd_402 = 0;
		/*IDX.RDB$SYSTEM_FLAG.NULL*/
		jrd_394.jrd_401 = FALSE; // Probably redundant.

		// Check if the table is actually a view.

		AutoCacheRequest request2(tdbb, drq_l_view_idx, DYN_REQUESTS);

		/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
			VREL IN RDB$RELATIONS
			WITH VREL.RDB$RELATION_NAME EQ IDX.RDB$RELATION_NAME*/
		{
		request2.compile(tdbb, (UCHAR*) jrd_386, sizeof(jrd_386));
		gds__vtov ((const char*) jrd_394.jrd_395, (char*) jrd_387.jrd_388, 32);
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_387);
		while (1)
		   {
		   EXE_receive (tdbb, request2, 1, 12, (UCHAR*) &jrd_389);
		   if (!jrd_389.jrd_391) break;
		{
			if (!/*VREL.RDB$VIEW_BLR.NULL*/
			     jrd_389.jrd_392)
			{
				// msg 181: "attempt to index a view"
				status_exception::raise(Arg::PrivateDyn(181));
			}
		}
		/*END_FOR*/
		   }
		}

		if (definition.unique.specified)
		{
			/*IDX.RDB$UNIQUE_FLAG.NULL*/
			jrd_394.jrd_411 = FALSE;
			/*IDX.RDB$UNIQUE_FLAG*/
			jrd_394.jrd_412 = SSHORT(definition.unique.value);
		}

		if (definition.inactive.specified)
		{
			/*IDX.RDB$INDEX_INACTIVE.NULL*/
			jrd_394.jrd_409 = FALSE;
			/*IDX.RDB$INDEX_INACTIVE*/
			jrd_394.jrd_410 = SSHORT(definition.inactive.value);
		}

		if (definition.descending.specified)
		{
			/*IDX.RDB$INDEX_TYPE.NULL*/
			jrd_394.jrd_407 = FALSE;
			/*IDX.RDB$INDEX_TYPE*/
			jrd_394.jrd_408 = SSHORT(definition.descending.value);
		}

		request2.reset(tdbb, drq_l_lfield, DYN_REQUESTS);

		for (size_t i = 0; i < definition.columns.getCount(); ++i)
		{
			for (size_t j = 0; j < i; ++j)
			{
				if (definition.columns[i] == definition.columns[j])
				{
					// msg 240 "Field %s cannot be used twice in index %s"
					status_exception::raise(
						Arg::PrivateDyn(240) << definition.columns[i] << /*IDX.RDB$INDEX_NAME*/
												 jrd_394.jrd_396);
				}
			}

			bool found = false;

			/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
				F IN RDB$RELATION_FIELDS CROSS
				GF IN RDB$FIELDS
				WITH GF.RDB$FIELD_NAME EQ F.RDB$FIELD_SOURCE AND
					 F.RDB$FIELD_NAME EQ definition.columns[i].c_str() AND
					 IDX.RDB$RELATION_NAME EQ F.RDB$RELATION_NAME*/
			{
			request2.compile(tdbb, (UCHAR*) jrd_369, sizeof(jrd_369));
			gds__vtov ((const char*) jrd_394.jrd_395, (char*) jrd_370.jrd_371, 32);
			gds__vtov ((const char*) definition.columns[i].c_str(), (char*) jrd_370.jrd_372, 32);
			EXE_start (tdbb, request2, transaction);
			EXE_send (tdbb, request2, 0, 64, (UCHAR*) &jrd_370);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 30, (UCHAR*) &jrd_373);
			   if (!jrd_373.jrd_375) break;
			{
				ULONG length = 0;

				if (/*GF.RDB$FIELD_TYPE*/
				    jrd_373.jrd_385 == blr_blob)
				{
					// msg 116 "attempt to index blob field in index %s"
					status_exception::raise(Arg::PrivateDyn(116) << /*IDX.RDB$INDEX_NAME*/
											jrd_394.jrd_396);
				}
				else if (!/*GF.RDB$DIMENSIONS.NULL*/
					  jrd_373.jrd_383)
				{
					// msg 117 "attempt to index array field in index %s"
					status_exception::raise(Arg::PrivateDyn(117) << /*IDX.RDB$INDEX_NAME*/
											jrd_394.jrd_396);
				}
				else if (!/*GF.RDB$COMPUTED_BLR.NULL*/
					  jrd_373.jrd_382)
				{
					// msg 179 "attempt to index COMPUTED BY field in index %s"
					status_exception::raise(Arg::PrivateDyn(179) << /*IDX.RDB$INDEX_NAME*/
											jrd_394.jrd_396);
				}
				else if (/*GF.RDB$FIELD_TYPE*/
					 jrd_373.jrd_385 == blr_varying || /*GF.RDB$FIELD_TYPE*/
		   jrd_373.jrd_385 == blr_text)
				{
					// Compute the length of the key segment allowing for international
					// information. Note that we we must convert a <character set, collation>
					// type to an index type in order to compute the length.
					if (!/*F.RDB$COLLATION_ID.NULL*/
					     jrd_373.jrd_380)
					{
						length = INTL_key_length(tdbb,
							INTL_TEXT_TO_INDEX(INTL_CS_COLL_TO_TTYPE(
								/*GF.RDB$CHARACTER_SET_ID*/
								jrd_373.jrd_379, /*F.RDB$COLLATION_ID*/
  jrd_373.jrd_381)),
							/*GF.RDB$FIELD_LENGTH*/
							jrd_373.jrd_378);
					}
					else if (!/*GF.RDB$COLLATION_ID.NULL*/
						  jrd_373.jrd_376)
					{
						length = INTL_key_length(tdbb,
							INTL_TEXT_TO_INDEX(INTL_CS_COLL_TO_TTYPE(
								/*GF.RDB$CHARACTER_SET_ID*/
								jrd_373.jrd_379, /*GF.RDB$COLLATION_ID*/
  jrd_373.jrd_377)),
							/*GF.RDB$FIELD_LENGTH*/
							jrd_373.jrd_378);
					}
					else
						length = /*GF.RDB$FIELD_LENGTH*/
							 jrd_373.jrd_378;
				}
				else
					length = sizeof(double);

				if (keyLength)
				{
					keyLength += ((length + Ods::STUFF_COUNT - 1) / (unsigned) Ods::STUFF_COUNT) *
						(Ods::STUFF_COUNT + 1);
				}
				else
					keyLength = length;

				found = true;
			}
			/*END_FOR*/
			   }
			}

			if (!found)
			{
				// msg 120 "Unknown columns in index %s"
				status_exception::raise(Arg::PrivateDyn(120) << /*IDX.RDB$INDEX_NAME*/
										jrd_394.jrd_396);
			}
		}

		if (!definition.expressionBlr.isEmpty())
		{
			/*IDX.RDB$EXPRESSION_BLR.NULL*/
			jrd_394.jrd_404 = FALSE;
			/*IDX.RDB$EXPRESSION_BLR*/
			jrd_394.jrd_397 = definition.expressionBlr;
		}

		if (!definition.expressionSource.isEmpty())
		{
			/*IDX.RDB$EXPRESSION_SOURCE.NULL*/
			jrd_394.jrd_405 = FALSE;
			/*IDX.RDB$EXPRESSION_SOURCE*/
			jrd_394.jrd_398 = definition.expressionSource;
		}

		keyLength = ROUNDUP(keyLength, sizeof(SLONG));
		if (keyLength >= MAX_KEY)
		{
			// msg 118 "key size too big for index %s"
			status_exception::raise(Arg::PrivateDyn(118) << /*IDX.RDB$INDEX_NAME*/
									jrd_394.jrd_396);
		}

		if (definition.columns.hasData())
		{
			request2.reset(tdbb, drq_s_idx_segs, DYN_REQUESTS);

			SSHORT position = 0;

			for (ObjectsArray<MetaName>::const_iterator segment(definition.columns.begin());
				 segment != definition.columns.end();
				 ++segment)
			{
				/*STORE(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
					X IN RDB$INDEX_SEGMENTS*/
				{
				
				{
					strcpy(/*X.RDB$INDEX_NAME*/
					       jrd_365.jrd_367, /*IDX.RDB$INDEX_NAME*/
  jrd_394.jrd_396);
					strcpy(/*X.RDB$FIELD_NAME*/
					       jrd_365.jrd_366, segment->c_str());
					/*X.RDB$FIELD_POSITION*/
					jrd_365.jrd_368 = position++;
				}
				/*END_STORE*/
				request2.compile(tdbb, (UCHAR*) jrd_364, sizeof(jrd_364));
				EXE_start (tdbb, request2, transaction);
				EXE_send (tdbb, request2, 0, 66, (UCHAR*) &jrd_365);
				}
			}
		}
		else if (/*IDX.RDB$EXPRESSION_BLR.NULL*/
			 jrd_394.jrd_404)
		{
			// msg 119 "no keys for index %s"
			status_exception::raise(Arg::PrivateDyn(119) << /*IDX.RDB$INDEX_NAME*/
									jrd_394.jrd_396);
		}

		if (definition.refColumns.hasData())
		{
			// If referring columns count <> referred columns return error.

			if (definition.columns.getCount() != definition.refColumns.getCount())
			{
				// msg 133: "Number of referencing columns do not equal number of
				// referenced columns
				status_exception::raise(Arg::PrivateDyn(133));
			}

			// Lookup a unique index in the referenced relation with the
			// referenced fields mentioned.

			request2.reset(tdbb, drq_l_unq_idx, DYN_REQUESTS);

			MetaName indexName;
			int listIndex = -1;
			bool found = false;

			/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
				RC IN RDB$RELATION_CONSTRAINTS CROSS
				IND IN RDB$INDICES OVER RDB$INDEX_NAME CROSS
				ISEG IN RDB$INDEX_SEGMENTS OVER RDB$INDEX_NAME
				WITH IND.RDB$RELATION_NAME EQ definition.refRelation.c_str() AND
					 IND.RDB$UNIQUE_FLAG NOT MISSING AND
					 (RC.RDB$CONSTRAINT_TYPE = PRIMARY_KEY OR
					  RC.RDB$CONSTRAINT_TYPE = UNIQUE_CNSTRT)
				SORTED BY IND.RDB$INDEX_NAME,
						  DESCENDING ISEG.RDB$FIELD_POSITION*/
			{
			request2.compile(tdbb, (UCHAR*) jrd_355, sizeof(jrd_355));
			gds__vtov ((const char*) definition.refRelation.c_str(), (char*) jrd_356.jrd_357, 32);
			gds__vtov ((const char*) UNIQUE_CNSTRT, (char*) jrd_356.jrd_358, 12);
			gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_356.jrd_359, 12);
			EXE_start (tdbb, request2, transaction);
			EXE_send (tdbb, request2, 0, 56, (UCHAR*) &jrd_356);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 66, (UCHAR*) &jrd_360);
			   if (!jrd_360.jrd_363) break;
			{
				if (indexName != /*IND.RDB$INDEX_NAME*/
						 jrd_360.jrd_362)
				{
					if (listIndex >= 0)
						found = false;
					if (found)
						break;
					listIndex = definition.refColumns.getCount() - 1;
					indexName = /*IND.RDB$INDEX_NAME*/
						    jrd_360.jrd_362;
					found = true;
				}

				// If there are no more fields or the field name doesn't
				// match, then this is not the correct index.

				if (listIndex >= 0)
				{
					fb_utils::exact_name_limit(/*ISEG.RDB$FIELD_NAME*/
								   jrd_360.jrd_361, sizeof(/*ISEG.RDB$FIELD_NAME*/
	 jrd_360.jrd_361));
					if (definition.refColumns[listIndex--] != /*ISEG.RDB$FIELD_NAME*/
										  jrd_360.jrd_361)
						found = false;
				}
				else
					found = false;
			}
			/*END_FOR*/
			   }
			}

			if (listIndex >= 0)
				found = false;

			if (found)
			{
				/*IDX.RDB$FOREIGN_KEY.NULL*/
				jrd_394.jrd_406 = FALSE;
				strcpy(/*IDX.RDB$FOREIGN_KEY*/
				       jrd_394.jrd_399, indexName.c_str());

				if (referredIndexName)
					*referredIndexName = indexName;
			}
			else
			{
				AutoRequest request3;
				bool isView = false;

				/*FOR(REQUEST_HANDLE request3 TRANSACTION_HANDLE transaction)
					X IN RDB$RELATIONS
					WITH X.RDB$RELATION_NAME EQ definition.refRelation.c_str()*/
				{
				request3.compile(tdbb, (UCHAR*) jrd_348, sizeof(jrd_348));
				gds__vtov ((const char*) definition.refRelation.c_str(), (char*) jrd_349.jrd_350, 32);
				EXE_start (tdbb, request3, transaction);
				EXE_send (tdbb, request3, 0, 32, (UCHAR*) &jrd_349);
				while (1)
				   {
				   EXE_receive (tdbb, request3, 1, 12, (UCHAR*) &jrd_351);
				   if (!jrd_351.jrd_353) break;
				{
					found = true;
					isView = !/*X.RDB$VIEW_BLR.NULL*/
						  jrd_351.jrd_354;
				}
				/*END_FOR*/
				   }
				}

				if (isView)
				{
					// msg 242: "attempt to reference a view (%s) in a foreign key"
					status_exception::raise(Arg::PrivateDyn(242) << definition.refRelation);
				}

				if (found)
				{
					// msg 18: "could not find UNIQUE or PRIMARY KEY constraint in table %s with
					// specified columns"
					status_exception::raise(Arg::PrivateDyn(18) << definition.refRelation);
				}
				else
				{
					// msg 241: "Table %s not found"
					status_exception::raise(Arg::PrivateDyn(241) << definition.refRelation);
				}
			}
		}
		else if (definition.refRelation.hasData())
		{
			request2.reset(tdbb, drq_l_primary, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
				IND IN RDB$INDICES CROSS
				RC IN RDB$RELATION_CONSTRAINTS OVER RDB$INDEX_NAME
				WITH IND.RDB$RELATION_NAME EQ definition.refRelation.c_str() AND
					 RC.RDB$CONSTRAINT_TYPE EQ PRIMARY_KEY*/
			{
			request2.compile(tdbb, (UCHAR*) jrd_340, sizeof(jrd_340));
			gds__vtov ((const char*) definition.refRelation.c_str(), (char*) jrd_341.jrd_342, 32);
			gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_341.jrd_343, 12);
			EXE_start (tdbb, request2, transaction);
			EXE_send (tdbb, request2, 0, 44, (UCHAR*) &jrd_341);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 36, (UCHAR*) &jrd_344);
			   if (!jrd_344.jrd_346) break;
			{
				// Number of columns in referred index should be same as number
				// of columns in referring index.

				fb_assert(/*IND.RDB$SEGMENT_COUNT*/
					  jrd_344.jrd_347 >= 0);
				if (definition.columns.getCount() != ULONG(/*IND.RDB$SEGMENT_COUNT*/
									   jrd_344.jrd_347))
				{
					// msg 133: "Number of referencing columns do not equal number of
					// referenced columns"
					status_exception::raise(Arg::PrivateDyn(133));
				}

				fb_utils::exact_name_limit(/*IND.RDB$INDEX_NAME*/
							   jrd_344.jrd_345, sizeof(/*IND.RDB$INDEX_NAME*/
	 jrd_344.jrd_345));

				/*IDX.RDB$FOREIGN_KEY.NULL*/
				jrd_394.jrd_406 = FALSE;
				strcpy(/*IDX.RDB$FOREIGN_KEY*/
				       jrd_394.jrd_399, /*IND.RDB$INDEX_NAME*/
  jrd_344.jrd_345);

				if (referredIndexName)
					*referredIndexName = /*IND.RDB$INDEX_NAME*/
							     jrd_344.jrd_345;
			}
			/*END_FOR*/
			   }
			}

			if (/*IDX.RDB$FOREIGN_KEY.NULL*/
			    jrd_394.jrd_406)
			{
				// msg 20: "could not find PRIMARY KEY index in specified table %s"
				status_exception::raise(Arg::PrivateDyn(20) << definition.refRelation);
			}
		}

		/*IDX.RDB$SEGMENT_COUNT*/
		jrd_394.jrd_400 = SSHORT(definition.columns.getCount());
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_393, sizeof(jrd_393));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 138, (UCHAR*) &jrd_394);
	}
}


void CreateIndexNode::print(string& text) const
{
	text.printf(
		"CreateIndexNode\n"
		"  name: '%s'\n",
		name.c_str());
}

// Define an index.
void CreateIndexNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
	Attachment* const attachment = transaction->tra_attachment;

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_CREATE_INDEX, name);

	CreateIndexNode::Definition definition;
	definition.type = isc_dyn_def_idx;
	definition.relation = relation->dsqlName;
	definition.unique = unique;
	definition.descending = descending;

	if (columns)
	{
	    const NestConst<ValueExprNode>* ptr = columns->items.begin();
	    const NestConst<ValueExprNode>* const end = columns->items.end();

		for (; ptr != end; ++ptr)
		{
			MetaName& column = definition.columns.add();
			column = (*ptr)->as<FieldNode>()->dsqlName;
		}
	}
	else if (computed)
	{
		string computedSource;
		BlrDebugWriter::BlrData computedValue;

		defineComputed(dsqlScratch, relation, NULL, computed, computedSource, computedValue);

		attachment->storeMetaDataBlob(tdbb, transaction, &definition.expressionSource,
			computedSource);
		attachment->storeBinaryBlob(tdbb, transaction, &definition.expressionBlr, computedValue);
	}

	store(tdbb, transaction, name, definition);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_CREATE_INDEX, name);

	savePoint.release();	// everything is ok
}


//----------------------


void AlterIndexNode::print(string& text) const
{
	text.printf(
		"AlterIndexNode\n"
		"  name: '%s'\n"
		"  active: '%d'\n",
		name.c_str(), active);
}

void AlterIndexNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_339;	// gds__utility 
   } jrd_338;
   struct {
          SSHORT jrd_336;	// gds__null_flag 
          SSHORT jrd_337;	// RDB$INDEX_INACTIVE 
   } jrd_335;
   struct {
          SSHORT jrd_332;	// gds__utility 
          SSHORT jrd_333;	// gds__null_flag 
          SSHORT jrd_334;	// RDB$INDEX_INACTIVE 
   } jrd_331;
   struct {
          TEXT  jrd_330 [32];	// RDB$INDEX_NAME 
   } jrd_329;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_m_index, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES
		WITH IDX.RDB$INDEX_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_328, sizeof(jrd_328));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_329.jrd_330, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_329);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_331);
	   if (!jrd_331.jrd_332) break;
	{
		found = true;

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_ALTER_INDEX, name);

		/*MODIFY IDX*/
		{
		
			/*IDX.RDB$INDEX_INACTIVE.NULL*/
			jrd_331.jrd_333 = FALSE;
			/*IDX.RDB$INDEX_INACTIVE*/
			jrd_331.jrd_334 = active ? FALSE : TRUE;
		/*END_MODIFY*/
		jrd_335.jrd_336 = jrd_331.jrd_333;
		jrd_335.jrd_337 = jrd_331.jrd_334;
		EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_335);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_338);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_ALTER_INDEX, name);
	else
	{
		// msg 48: "Index not found"
		status_exception::raise(Arg::PrivateDyn(48));
	}

	savePoint.release();	// everything is ok
}


//----------------------


void SetStatisticsNode::print(string& text) const
{
	text.printf(
		"SetStatisticsNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void SetStatisticsNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_327;	// gds__utility 
   } jrd_326;
   struct {
          double  jrd_324;	// RDB$STATISTICS 
          SSHORT jrd_325;	// gds__null_flag 
   } jrd_323;
   struct {
          double  jrd_320;	// RDB$STATISTICS 
          SSHORT jrd_321;	// gds__utility 
          SSHORT jrd_322;	// gds__null_flag 
   } jrd_319;
   struct {
          TEXT  jrd_318 [32];	// RDB$INDEX_NAME 
   } jrd_317;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_m_set_statistics, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES
		WITH IDX.RDB$INDEX_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_316, sizeof(jrd_316));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_317.jrd_318, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_317);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_319);
	   if (!jrd_319.jrd_321) break;
	{
		found = true;

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_ALTER_INDEX, name);

		/*MODIFY IDX*/
		{
		
			// For V4 index selectivity can be set only to -1.
			/*IDX.RDB$STATISTICS.NULL*/
			jrd_319.jrd_322 = FALSE;
			/*IDX.RDB$STATISTICS*/
			jrd_319.jrd_320 = -1.0;
		/*END_MODIFY*/
		jrd_323.jrd_324 = jrd_319.jrd_320;
		jrd_323.jrd_325 = jrd_319.jrd_322;
		EXE_send (tdbb, request, 2, 10, (UCHAR*) &jrd_323);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_326);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_ALTER_INDEX, name);
	else
	{
		// msg 48: "Index not found"
		status_exception::raise(Arg::PrivateDyn(48));
	}

	savePoint.release();	// everything is ok
}


//----------------------


// Delete the records in RDB$INDEX_SEGMENTS pertaining to an index.
bool DropIndexNode::deleteSegmentRecords(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& name)
{
   struct {
          SSHORT jrd_315;	// gds__utility 
   } jrd_314;
   struct {
          SSHORT jrd_313;	// gds__utility 
   } jrd_312;
   struct {
          SSHORT jrd_311;	// gds__utility 
   } jrd_310;
   struct {
          TEXT  jrd_309 [32];	// RDB$INDEX_NAME 
   } jrd_308;
	AutoCacheRequest request(tdbb, drq_e_idx_segs, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDXSEG IN RDB$INDEX_SEGMENTS WITH IDXSEG.RDB$INDEX_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_307, sizeof(jrd_307));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_308.jrd_309, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_308);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_310);
	   if (!jrd_310.jrd_311) break;
	{
		found = true;
		/*ERASE IDXSEG;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_312);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_314);
	   }
	}

	return found;
}

void DropIndexNode::print(string& text) const
{
	text.printf(
		"DropIndexNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void DropIndexNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_306;	// gds__utility 
   } jrd_305;
   struct {
          SSHORT jrd_304;	// gds__utility 
   } jrd_303;
   struct {
          bid  jrd_300;	// RDB$EXPRESSION_BLR 
          SSHORT jrd_301;	// gds__utility 
          SSHORT jrd_302;	// gds__null_flag 
   } jrd_299;
   struct {
          TEXT  jrd_298 [32];	// RDB$INDEX_NAME 
   } jrd_297;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_e_indices, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES
		WITH IDX.RDB$INDEX_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_296, sizeof(jrd_296));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_297.jrd_298, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_297);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_299);
	   if (!jrd_299.jrd_301) break;
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_DROP_INDEX, name);

		/*ERASE IDX;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_303);

		if (/*IDX.RDB$EXPRESSION_BLR.NULL*/
		    jrd_299.jrd_302 && !deleteSegmentRecords(tdbb, transaction, name))
		{
			// msg 50: "No segments found for index"
			status_exception::raise(Arg::PrivateDyn(50));
		}

		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_305);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_INDEX, name);
	else
	{
		// msg 48: "Index not found"
		status_exception::raise(Arg::PrivateDyn(48));
	}

	savePoint.release();	// everything is ok
}


//----------------------


void CreateFilterNode::print(string& text) const
{
	text.printf(
		"CreateFilterNode\n"
		"  name: '%s'\n",
		name.c_str());
}

// Define a blob filter.
void CreateFilterNode::execute(thread_db* tdbb, DsqlCompilerScratch* /*dsqlScratch*/, jrd_tra* transaction)
{
   struct {
          TEXT  jrd_290 [256];	// RDB$ENTRYPOINT 
          TEXT  jrd_291 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_292 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_293;	// RDB$OUTPUT_SUB_TYPE 
          SSHORT jrd_294;	// RDB$INPUT_SUB_TYPE 
          SSHORT jrd_295;	// RDB$SYSTEM_FLAG 
   } jrd_289;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	///executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_DECLARE_FILTER, name);

	AutoCacheRequest request(tdbb, drq_s_filters, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$FILTERS*/
	{
	
	{
		strcpy(/*X.RDB$FUNCTION_NAME*/
		       jrd_289.jrd_292, name.c_str());
		/*X.RDB$SYSTEM_FLAG*/
		jrd_289.jrd_295 = 0;
		moduleName.copyTo(/*X.RDB$MODULE_NAME*/
				  jrd_289.jrd_291, sizeof(/*X.RDB$MODULE_NAME*/
	 jrd_289.jrd_291));
		entryPoint.copyTo(/*X.RDB$ENTRYPOINT*/
				  jrd_289.jrd_290, sizeof(/*X.RDB$ENTRYPOINT*/
	 jrd_289.jrd_290));

		if (inputFilter->name.hasData())
		{
			if (!METD_get_type(transaction, inputFilter->name, "RDB$FIELD_SUB_TYPE",
					&/*X.RDB$INPUT_SUB_TYPE*/
					 jrd_289.jrd_294))
			{
				status_exception::raise(
					Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					Arg::Gds(isc_dsql_datatype_err) <<
					Arg::Gds(isc_dsql_blob_type_unknown) << inputFilter->name);
			}
		}
		else
			/*X.RDB$INPUT_SUB_TYPE*/
			jrd_289.jrd_294 = inputFilter->number;

		if (outputFilter->name.hasData())
		{
			if (!METD_get_type(transaction, outputFilter->name, "RDB$FIELD_SUB_TYPE",
					&/*X.RDB$OUTPUT_SUB_TYPE*/
					 jrd_289.jrd_293))
			{
				status_exception::raise(
					Arg::Gds(isc_sqlerr) << Arg::Num(-204) <<
					Arg::Gds(isc_dsql_datatype_err) <<
					Arg::Gds(isc_dsql_blob_type_unknown) << outputFilter->name);
			}
		}
		else
			/*X.RDB$OUTPUT_SUB_TYPE*/
			jrd_289.jrd_293 = outputFilter->number;
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_288, sizeof(jrd_288));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 550, (UCHAR*) &jrd_289);
	}

	///executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DECLARE_FILTER, name);

	savePoint.release();	// everything is ok
}


//----------------------


void DropFilterNode::print(string& text) const
{
	text.printf(
		"DropFilterNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void DropFilterNode::execute(thread_db* tdbb, DsqlCompilerScratch* /*dsqlScratch*/, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_287;	// gds__utility 
   } jrd_286;
   struct {
          SSHORT jrd_285;	// gds__utility 
   } jrd_284;
   struct {
          SSHORT jrd_283;	// gds__utility 
   } jrd_282;
   struct {
          TEXT  jrd_281 [32];	// RDB$FUNCTION_NAME 
   } jrd_280;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_e_filters, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$FILTERS
		WITH X.RDB$FUNCTION_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_279, sizeof(jrd_279));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_280.jrd_281, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_280);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_282);
	   if (!jrd_282.jrd_283) break;
	{
		/*ERASE X;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_284);
		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_286);
	   }
	}

	if (!found)
	{
		// msg 37: "Blob Filter %s not found"
		status_exception::raise(Arg::PrivateDyn(37) << name);
	}

	savePoint.release();	// everything is ok
}


//----------------------


void CreateShadowNode::print(string& text) const
{
	text.printf(
		"CreateShadowNode\n"
		"  number: '%d'\n",
		number);
}

void CreateShadowNode::execute(thread_db* tdbb, DsqlCompilerScratch* /*dsqlScratch*/, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_278;	// gds__utility 
   } jrd_277;
   struct {
          SSHORT jrd_276;	// RDB$SHADOW_NUMBER 
   } jrd_275;
	if (!tdbb->getAttachment()->locksmith())
		status_exception::raise(Arg::Gds(isc_adm_task_denied));

	// Should be caught by the parser.
	if (number == 0)
	{
		status_exception::raise(
			Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
			Arg::Gds(isc_dsql_command_err) <<
			Arg::Gds(isc_dsql_shadow_number_err));
	}

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	// If a shadow set identified by the shadow number already exists return error.

	AutoCacheRequest request(tdbb, drq_l_shadow, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FIRST 1 X IN RDB$FILES
		WITH X.RDB$SHADOW_NUMBER EQ number*/
	{
	request.compile(tdbb, (UCHAR*) jrd_274, sizeof(jrd_274));
	jrd_275.jrd_276 = number;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_275);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_277);
	   if (!jrd_277.jrd_278) break;
	{
		// msg 165: "Shadow %ld already exists"
		status_exception::raise(Arg::PrivateDyn(165) << Arg::Num(number));
	}
	/*END_FOR*/
	   }
	}

	SLONG start = 0;

	for (NestConst<DbFileClause>* i = files.begin(); i != files.end(); ++i)
	{
		bool first = i == files.begin();
		DbFileClause* file = *i;

		if (!first && i[-1]->length == 0 && file->start == 0)
		{
			// Preceding file did not specify length, so %s must include starting page number
			status_exception::raise(
				Arg::Gds(isc_sqlerr) << Arg::Num(-607) <<
				Arg::Gds(isc_dsql_command_err) <<
				Arg::Gds(isc_dsql_file_length_err) << file->name);
		}

		defineFile(tdbb, transaction, number, manual && first, conditional && first,
			start, file->name.c_str(), file->start, file->length);
	}

	savePoint.release();	// everything is ok
}


//----------------------


void DropShadowNode::print(string& text) const
{
	text.printf(
		"DropShadowNode\n"
		"  number: '%d'\n",
		number);
}

void DropShadowNode::execute(thread_db* tdbb, DsqlCompilerScratch* /*dsqlScratch*/, jrd_tra* transaction)
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
          SSHORT jrd_267;	// RDB$SHADOW_NUMBER 
   } jrd_266;
	if (!tdbb->getAttachment()->locksmith())
		status_exception::raise(Arg::Gds(isc_adm_task_denied));

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_e_shadow, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FIL IN RDB$FILES
		WITH FIL.RDB$SHADOW_NUMBER EQ number*/
	{
	request.compile(tdbb, (UCHAR*) jrd_265, sizeof(jrd_265));
	jrd_266.jrd_267 = number;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_266);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_268);
	   if (!jrd_268.jrd_269) break;
	{
		/*ERASE FIL;*/
		EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_270);
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_272);
	   }
	}

	// ASF: No error is raised if the shadow is not found.

	savePoint.release();	// everything is ok
}


//----------------------


void CreateRoleNode::print(string& text) const
{
	text.printf(
		"CreateRoleNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void CreateRoleNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
   struct {
          TEXT  jrd_262 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_263 [32];	// RDB$ROLE_NAME 
          SSHORT jrd_264;	// RDB$SYSTEM_FLAG 
   } jrd_261;
	MetaName ownerName(tdbb->getAttachment()->att_user->usr_user_name);
	ownerName.upper7();

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
		DDL_TRIGGER_CREATE_ROLE, name);

	if (name == ownerName)
	{
		// msg 193: "user name @1 could not be used for SQL role"
		status_exception::raise(Arg::PrivateDyn(193) << ownerName);
	}

	if (name == NULL_ROLE)
	{
		// msg 195: "keyword @1 could not be used as SQL role name"
		status_exception::raise(Arg::PrivateDyn(195) << name);
	}

	if (isItUserName(tdbb, transaction))
	{
		// msg 193: "user name @1 could not be used for SQL role"
		status_exception::raise(Arg::PrivateDyn(193) << name);
	}

	MetaName dummyName;
	if (isItSqlRole(tdbb, transaction, name, dummyName))
	{
		// msg 194: "SQL role @1 already exists"
		status_exception::raise(Arg::PrivateDyn(194) << name);
	}

	AutoCacheRequest request(tdbb, drq_role_gens, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$ROLES*/
	{
	
	{
		strcpy(/*X.RDB$ROLE_NAME*/
		       jrd_261.jrd_263, name.c_str());
		strcpy(/*X.RDB$OWNER_NAME*/
		       jrd_261.jrd_262, ownerName.c_str());
		/*X.RDB$SYSTEM_FLAG*/
		jrd_261.jrd_264 = 0;
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_260, sizeof(jrd_260));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_261);
	}

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER,
		DDL_TRIGGER_CREATE_ROLE, name);

	savePoint.release();	// everything is ok
}

// If role name is user name returns true. Otherwise returns false.
bool CreateRoleNode::isItUserName(thread_db* tdbb, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_251;	// gds__utility 
   } jrd_250;
   struct {
          TEXT  jrd_249 [32];	// RDB$OWNER_NAME 
   } jrd_248;
   struct {
          SSHORT jrd_259;	// gds__utility 
   } jrd_258;
   struct {
          TEXT  jrd_254 [32];	// RDB$GRANTOR 
          TEXT  jrd_255 [32];	// RDB$USER 
          SSHORT jrd_256;	// RDB$OBJECT_TYPE 
          SSHORT jrd_257;	// RDB$USER_TYPE 
   } jrd_253;
	bool found = false;

	// If there is a user with privilege or a grantor on a relation we
	// can infer there is a user with this name

	AutoCacheRequest request(tdbb, drq_get_user_priv, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH (PRIV.RDB$USER EQ name.c_str() AND PRIV.RDB$USER_TYPE = obj_user) OR
			 (PRIV.RDB$GRANTOR EQ name.c_str() AND PRIV.RDB$OBJECT_TYPE = obj_relation)*/
	{
	request.compile(tdbb, (UCHAR*) jrd_252, sizeof(jrd_252));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_253.jrd_254, 32);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_253.jrd_255, 32);
	jrd_253.jrd_256 = obj_relation;
	jrd_253.jrd_257 = obj_user;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_253);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_258);
	   if (!jrd_258.jrd_259) break;
	{
		found = true;
	}
	/*END_FOR*/
	   }
	}

	if (found)
		return found;

	// We can infer that 'role name' is a user name if it owns any relations
	// Note we can only get here if a user creates a table and revokes all
	// his privileges on the table

	request.reset(tdbb, drq_get_rel_owner, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		REL IN RDB$RELATIONS
		WITH REL.RDB$OWNER_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_247, sizeof(jrd_247));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_248.jrd_249, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_248);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_250);
	   if (!jrd_250.jrd_251) break;
	{
		found = true;
	}
	/*END_FOR*/
	   }
	}

	return found;
}


//----------------------


void MappingNode::print(string& text) const
{
	const char* null = "<Null>";

	text.printf(
		"MappingNode\n"
		"  op: '%s'\n"
		"  global: '%d'\n"
		"  mode: '%c'\n"
		"  plugin: '%s'\n"
		"  db: '%s'\n"
		"  fromType: '%s'\n"
		"  from: '%s'\n"
		"  role: '%d'\n"
		"  to: '%s'\n",
		op, global, mode,
		plugin ? plugin->c_str() : null, db ? db->c_str() : null,
		fromType ? fromType->c_str() : null, from ? from->getString().c_str() : null,
		role, to ? to->c_str() : null);
}

void MappingNode::validateAdmin()
{
	if (to && (*to != ADMIN_ROLE))
		Arg::Gds(isc_alter_role).raise();
}

// add some item to DDL in "double quotes"
void MappingNode::addItem(string& ddl, const char* text)
{
	ddl += '"';
	char c;
	while (c = *text++)
	{
		ddl += c;
		if (c == '"')
			ddl += c;
	}
	ddl += '"';
}

// It's purpose is to add/drop mapping from any security name to DB security object.
void MappingNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
   struct {
          TEXT  jrd_203 [32];	// RDB$MAP_TO 
          TEXT  jrd_204 [256];	// RDB$MAP_FROM 
          TEXT  jrd_205 [32];	// RDB$MAP_FROM_TYPE 
          TEXT  jrd_206 [32];	// RDB$MAP_DB 
          TEXT  jrd_207 [32];	// RDB$MAP_PLUGIN 
          TEXT  jrd_208 [32];	// RDB$MAP_NAME 
          SSHORT jrd_209;	// gds__null_flag 
          SSHORT jrd_210;	// RDB$MAP_TO_TYPE 
          SSHORT jrd_211;	// gds__null_flag 
          SSHORT jrd_212;	// gds__null_flag 
          SSHORT jrd_213;	// gds__null_flag 
          TEXT  jrd_214 [2];	// RDB$MAP_USING 
   } jrd_202;
   struct {
          TEXT  jrd_236 [32];	// RDB$MAP_TO 
          TEXT  jrd_237 [32];	// RDB$MAP_PLUGIN 
          TEXT  jrd_238 [32];	// RDB$MAP_DB 
          TEXT  jrd_239 [32];	// RDB$MAP_FROM_TYPE 
          TEXT  jrd_240 [256];	// RDB$MAP_FROM 
          SSHORT jrd_241;	// gds__null_flag 
          SSHORT jrd_242;	// RDB$MAP_TO_TYPE 
          TEXT  jrd_243 [2];	// RDB$MAP_USING 
          SSHORT jrd_244;	// gds__null_flag 
          SSHORT jrd_245;	// gds__null_flag 
          SSHORT jrd_246;	// gds__null_flag 
   } jrd_235;
   struct {
          SSHORT jrd_234;	// gds__utility 
   } jrd_233;
   struct {
          SSHORT jrd_232;	// gds__utility 
   } jrd_231;
   struct {
          TEXT  jrd_219 [256];	// RDB$MAP_FROM 
          TEXT  jrd_220 [32];	// RDB$MAP_FROM_TYPE 
          TEXT  jrd_221 [32];	// RDB$MAP_DB 
          TEXT  jrd_222 [32];	// RDB$MAP_PLUGIN 
          TEXT  jrd_223 [32];	// RDB$MAP_TO 
          SSHORT jrd_224;	// gds__utility 
          SSHORT jrd_225;	// gds__null_flag 
          SSHORT jrd_226;	// gds__null_flag 
          SSHORT jrd_227;	// gds__null_flag 
          TEXT  jrd_228 [2];	// RDB$MAP_USING 
          SSHORT jrd_229;	// RDB$MAP_TO_TYPE 
          SSHORT jrd_230;	// gds__null_flag 
   } jrd_218;
   struct {
          TEXT  jrd_217 [32];	// RDB$MAP_NAME 
   } jrd_216;
	if (!(tdbb->getAttachment() && tdbb->getAttachment()->locksmith()))
		status_exception::raise(Arg::Gds(isc_adm_task_denied));

	if (global)
	{
		LocalStatus st;
		LocalStatus s2;			// we will use it in DDL case and remember
		IStatus* s = &st;
		class Check
		{
		public:
			static void status(IStatus* s)
			{
				if (!s->isSuccess())
					status_exception::raise(s->get());
			}
		};

		SecDbContext* secDbContext = transaction->getSecDbContext();
		if (!secDbContext)
		{
			const char* secDb = tdbb->getDatabase()->dbb_config->getSecurityDatabase();
			ClumpletWriter dpb(ClumpletWriter::WideTagged, MAX_DPB_SIZE, isc_dpb_version2);
			if (tdbb->getAttachment()->att_user)
				tdbb->getAttachment()->att_user->populateDpb(dpb);
			IAttachment* att = DispatcherPtr()->attachDatabase(s, secDb,
				dpb.getBufferLength(), dpb.getBuffer());
			Check::status(s);

			ITransaction* tra = att->startTransaction(s, 0, NULL);
			Check::status(s);

			secDbContext = transaction->setSecDbContext(att, tra);
		}

		// run all statements under savepoint control
		string savePoint;
		savePoint.printf("GLOBALMAP%d", secDbContext->savePoint++);
		secDbContext->att->execute(s, secDbContext->tra, 0, ("SAVEPOINT " + savePoint).c_str(),
			SQL_DIALECT_V6, NULL, NULL, NULL, NULL);
		Check::status(s);

		try
		{
			// first of all try to use regenerated DDL statement
			// that's the best way if security database is FB3 or higher fb version
			string ddl;

			switch(op)
			{
			case MAP_ADD:
				ddl = "CREATE MAPPING ";
				break;
			case MAP_MOD:
				ddl = "ALTER MAPPING ";
				break;
			case MAP_DROP:
				ddl = "DROP MAPPING ";
				break;
			case MAP_RPL:
				ddl = "CREATE OR ALTER MAPPING ";
				break;
			}

			addItem(ddl, name.c_str());
			if (op != MAP_DROP)
			{
				ddl += " USING ";
				switch (mode)
				{
				case 'P':
					if (!plugin)
						ddl += "ANY PLUGIN ";
					else
					{
						ddl += "PLUGIN ";
						addItem(ddl, plugin->c_str());
						ddl += ' ';
					}
					break;
				case 'S':
					ddl += "ANY PLUGIN SERVERWIDE ";
					break;
				case '*':
					ddl += "* ";
					break;
				case 'M':
					ddl += "MAPPING ";
					break;
				}

				if (db)
				{
					ddl += "IN ";
					addItem(ddl, db->c_str());
					ddl += ' ';
				}

				if (fromType)
				{
					ddl += "FROM ";
					if (!from)
						ddl += "ANY ";
					addItem(ddl, fromType->c_str());
					ddl += ' ';
					if (from)
					{
						addItem(ddl, from->getString().c_str());
						ddl += ' ';
					}
				}

				ddl += "TO ";
				ddl += (role ? "ROLE" : "USER");
				if (to)
				{
					ddl += ' ';
					addItem(ddl, to->c_str());
				}
			}

			// Now try to run DDL
			secDbContext->att->execute(&s2, secDbContext->tra, 0, ddl.c_str(), SQL_DIALECT_V6,
				NULL, NULL, NULL, NULL);

			if (!s2.isSuccess())
			{
				// try direct access to rdb$map table in secure db

				// check presence of such record in the table
				Message check;
				Field<Varying> nm(check, 1);
				nm = name.c_str();

				Message result;
				Field<ISC_INT64> cnt(result);

				const char* checkSql = "select count(*) from RDB$MAP where RDB$MAP_NAME = ?";

				secDbContext->att->execute(s, secDbContext->tra, 0, checkSql, SQL_DIALECT_V6,
					check.getMetadata(), check.getBuffer(), result.getMetadata(), result.getBuffer());
				Check::status(s);

				if (cnt > 1 && op != MAP_DROP)
					ERRD_bugcheck("Database mapping misconfigured");

				bool hasLine = cnt > 0;
				switch(op)
				{
				case MAP_ADD:
					if (hasLine)
						(Arg::Gds(isc_map_already_exists) << name).raise();
					break;

				case MAP_MOD:
				case MAP_DROP:
					if (!hasLine)
						(Arg::Gds(isc_map_not_exists) << name).raise();
					break;

				case MAP_RPL:
					op = hasLine ? MAP_MOD : MAP_DROP;
					break;
				}

				// Get ready to modify table
				Message full;
				Field<ISC_SHORT> toType(full);
				Field<Varying> t(full, MAX_SQL_IDENTIFIER_LEN);
				Field<Varying> usng2(full, 1);
				Field<Varying> plug2(full, MAX_SQL_IDENTIFIER_LEN);
				Field<Varying> d2(full, MAX_SQL_IDENTIFIER_LEN);
				Field<Varying> type2(full, MAX_SQL_IDENTIFIER_LEN);
				Field<Varying> f2(full, 255);
				Field<Varying> nm2(full, MAX_SQL_IDENTIFIER_LEN);

				toType = role ? 1 : 0;
				if (to)
					t = to->c_str();
				usng2.set(1, &mode);
				if (plugin)
					plug2 = plugin->c_str();
				if (db)
					d2 = db->c_str();
				if (fromType)
					type2 = fromType->c_str();
				if (from)
					f2 = from->getString().c_str();
				nm2 = name.c_str();

				Message* msg = NULL;
				const char* sql = NULL;
				switch(op)
				{
				case MAP_ADD:
					sql = "insert into RDB$MAP(RDB$MAP_TO_TYPE, RDB$MAP_TO, RDB$MAP_USING, "
						"RDB$MAP_PLUGIN, RDB$MAP_DB, RDB$MAP_FROM_TYPE, RDB$MAP_FROM, RDB$MAP_NAME) "
						"values (?, ?, ?, ?, ?, ?, ?, ?)";
					msg = &full;
					break;
				case MAP_MOD:
					sql = "update RDB$MAP set RDB$MAP_TO_TYPE = ?, RDB$MAP_TO = ?, "
						"RDB$MAP_USING = ?, RDB$MAP_PLUGIN = ?, RDB$MAP_DB = ?, "
						"RDB$MAP_FROM_TYPE = ?, RDB$MAP_FROM = ? "
						"where RDB$MAP_NAME = ?";
					msg = &full;
					break;
				case MAP_DROP:
					sql = "delete from RDB$MAP where RDB$MAP_NAME = ?";
					msg = &check;
					break;
				}

				// Actual modification
				fb_assert(sql && msg);
				secDbContext->att->execute(s, secDbContext->tra, 0, sql, SQL_DIALECT_V6,
					msg->getMetadata(), msg->getBuffer(), NULL, NULL);
				Check::status(s);

				secDbContext->att->execute(s, secDbContext->tra, 0, ("RELEASE SAVEPOINT " + savePoint).c_str(),
					SQL_DIALECT_V6, NULL, NULL, NULL, NULL);
				savePoint.erase();
				Check::status(s);
			}
		}
		catch (const Exception&)
		{
			if (savePoint.hasData())
			{
				secDbContext->att->execute(s, secDbContext->tra, 0, ("ROLLBACK TO SAVEPOINT " + savePoint).c_str(),
					SQL_DIALECT_V6, NULL, NULL, NULL, NULL);
			}

			if (!s2.isSuccess())
			{
				const ISC_STATUS* stat2 = s2.get();
				if (stat2[1] != isc_dsql_token_unk_err)
					status_exception::raise(stat2);
			}

			throw;
		}

		return;
	}

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	fb_assert(op == MAP_DROP || fromType);

	short plugNull = plugin ? FALSE  : TRUE;
	short dbNull = db ? FALSE : TRUE;
	short fromNull = from ? FALSE : TRUE;

	char usingText[2];
	usingText[0] = mode;
	usingText[1] = '\0';

	AutoCacheRequest request1(tdbb, drq_map_mod, DYN_REQUESTS);
	bool found = false;
	int ddlTriggerAction = 0;

	/*FOR(REQUEST_HANDLE request1 TRANSACTION_HANDLE transaction)
		M IN RDB$MAP
		WITH M.RDB$MAP_NAME EQ name.c_str()*/
	{
	request1.compile(tdbb, (UCHAR*) jrd_215, sizeof(jrd_215));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_216.jrd_217, 32);
	EXE_start (tdbb, request1, transaction);
	EXE_send (tdbb, request1, 0, 32, (UCHAR*) &jrd_216);
	while (1)
	   {
	   EXE_receive (tdbb, request1, 1, 398, (UCHAR*) &jrd_218);
	   if (!jrd_218.jrd_224) break;
	{
		found = true;
		switch (op)
		{
		case MAP_ADD:
			break;

		case MAP_MOD:
		case MAP_RPL:
			ddlTriggerAction = DDL_TRIGGER_ALTER_MAPPING;
			executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, ddlTriggerAction, name);

			/*MODIFY M*/
			{
			
				if (to)
				{
					/*M.RDB$MAP_TO.NULL*/
					jrd_218.jrd_230 = FALSE;
					strcpy(/*M.RDB$MAP_TO*/
					       jrd_218.jrd_223, to->c_str());
				}
				else
					/*M.RDB$MAP_TO.NULL*/
					jrd_218.jrd_230 = TRUE;
				/*M.RDB$MAP_TO_TYPE*/
				jrd_218.jrd_229 = role ? 1 : 0;

				strcpy(/*M.RDB$MAP_USING*/
				       jrd_218.jrd_228, usingText);
				/*M.RDB$MAP_PLUGIN.NULL*/
				jrd_218.jrd_227 = plugNull;
				if (!plugNull)
					strcpy(/*M.RDB$MAP_PLUGIN*/
					       jrd_218.jrd_222, plugin->c_str());

				/*M.RDB$MAP_DB.NULL*/
				jrd_218.jrd_226 = dbNull;
				if (!dbNull)
					strcpy(/*M.RDB$MAP_DB*/
					       jrd_218.jrd_221, db->c_str());

				strcpy(/*M.RDB$MAP_FROM_TYPE*/
				       jrd_218.jrd_220, fromType->c_str());
				/*M.RDB$MAP_FROM.NULL*/
				jrd_218.jrd_225 = fromNull;
				if (!fromNull)
					strcpy(/*M.RDB$MAP_FROM*/
					       jrd_218.jrd_219, from->getString().c_str());
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_218.jrd_223, (char*) jrd_235.jrd_236, 32);
			gds__vtov((const char*) jrd_218.jrd_222, (char*) jrd_235.jrd_237, 32);
			gds__vtov((const char*) jrd_218.jrd_221, (char*) jrd_235.jrd_238, 32);
			gds__vtov((const char*) jrd_218.jrd_220, (char*) jrd_235.jrd_239, 32);
			gds__vtov((const char*) jrd_218.jrd_219, (char*) jrd_235.jrd_240, 256);
			jrd_235.jrd_241 = jrd_218.jrd_230;
			jrd_235.jrd_242 = jrd_218.jrd_229;
			gds__vtov((const char*) jrd_218.jrd_228, (char*) jrd_235.jrd_243, 2);
			jrd_235.jrd_244 = jrd_218.jrd_227;
			jrd_235.jrd_245 = jrd_218.jrd_226;
			jrd_235.jrd_246 = jrd_218.jrd_225;
			EXE_send (tdbb, request1, 4, 396, (UCHAR*) &jrd_235);
			}
			break;

		case MAP_DROP:
			ddlTriggerAction = DDL_TRIGGER_DROP_MAPPING;
			executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, ddlTriggerAction, name);

			/*ERASE M;*/
			EXE_send (tdbb, request1, 2, 2, (UCHAR*) &jrd_231);
			break;
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request1, 3, 2, (UCHAR*) &jrd_233);
	   }
	}

	AutoCacheRequest request2(tdbb, drq_map_sto, DYN_REQUESTS);
	switch (op)
	{
	case MAP_ADD:
		if (found)
		{
			(Arg::Gds(isc_map_already_exists) << name).raise();
		}
		// fall through ...

	case MAP_RPL:
		if (found)
			break;

		ddlTriggerAction = DDL_TRIGGER_CREATE_MAPPING;
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, ddlTriggerAction, name);

		/*STORE(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
		M IN RDB$MAP*/
		{
		
		{
			strcpy(/*M.RDB$MAP_NAME*/
			       jrd_202.jrd_208, name.c_str());
			strcpy(/*M.RDB$MAP_USING*/
			       jrd_202.jrd_214, usingText);
			/*M.RDB$MAP_PLUGIN.NULL*/
			jrd_202.jrd_213 = plugNull;
			if (!plugNull)
				strcpy(/*M.RDB$MAP_PLUGIN*/
				       jrd_202.jrd_207, plugin->c_str());

			/*M.RDB$MAP_DB.NULL*/
			jrd_202.jrd_212 = dbNull;
			if (!dbNull)
				strcpy(/*M.RDB$MAP_DB*/
				       jrd_202.jrd_206, db->c_str());

			strcpy(/*M.RDB$MAP_FROM_TYPE*/
			       jrd_202.jrd_205, fromType->c_str());
			/*M.RDB$MAP_FROM.NULL*/
			jrd_202.jrd_211 = fromNull;
			if (!fromNull)
				strcpy(/*M.RDB$MAP_FROM*/
				       jrd_202.jrd_204, from->getString().c_str());

			/*M.RDB$MAP_TO_TYPE*/
			jrd_202.jrd_210 = role ? 1 : 0;
			if (to)
			{
				/*M.RDB$MAP_TO.NULL*/
				jrd_202.jrd_209 = FALSE;
				strcpy(/*M.RDB$MAP_TO*/
				       jrd_202.jrd_203, to->c_str());
			}
			else
				/*M.RDB$MAP_TO.NULL*/
				jrd_202.jrd_209 = TRUE;
		}
		/*END_STORE*/
		request2.compile(tdbb, (UCHAR*) jrd_201, sizeof(jrd_201));
		EXE_start (tdbb, request2, transaction);
		EXE_send (tdbb, request2, 0, 428, (UCHAR*) &jrd_202);
		}
		break;

	case MAP_MOD:
	case MAP_DROP:
		if (!found)
			(Arg::Gds(isc_map_not_exists) << name).raise();
		break;
	}

	fb_assert(ddlTriggerAction > 0);
	if (ddlTriggerAction > 0)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, ddlTriggerAction, name);

	DFW_post_work(transaction, dfw_clear_mapping, NULL, 0);
	savePoint.release();	// everything is ok
}


//----------------------


void DropRoleNode::print(string& text) const
{
	text.printf(
		"DropRoleNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void DropRoleNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_189;	// gds__utility 
   } jrd_188;
   struct {
          SSHORT jrd_187;	// gds__utility 
   } jrd_186;
   struct {
          SSHORT jrd_185;	// gds__utility 
   } jrd_184;
   struct {
          TEXT  jrd_180 [32];	// RDB$USER 
          TEXT  jrd_181 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_182;	// RDB$USER_TYPE 
          SSHORT jrd_183;	// RDB$OBJECT_TYPE 
   } jrd_179;
   struct {
          SSHORT jrd_200;	// gds__utility 
   } jrd_199;
   struct {
          SSHORT jrd_198;	// gds__utility 
   } jrd_197;
   struct {
          TEXT  jrd_194 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_195;	// gds__utility 
          SSHORT jrd_196;	// RDB$SYSTEM_FLAG 
   } jrd_193;
   struct {
          TEXT  jrd_192 [32];	// RDB$ROLE_NAME 
   } jrd_191;
	MetaName user(tdbb->getAttachment()->att_user->usr_user_name);
	user.upper7();

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest request(tdbb, drq_drop_role, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		ROL IN RDB$ROLES
		WITH ROL.RDB$ROLE_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_190, sizeof(jrd_190));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_191.jrd_192, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_191);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_193);
	   if (!jrd_193.jrd_195) break;
	{
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_DROP_ROLE, name);

		const MetaName roleOwner(/*ROL.RDB$OWNER_NAME*/
					 jrd_193.jrd_194);

		if (/*ROL.RDB$SYSTEM_FLAG*/
		    jrd_193.jrd_196 != 0)
		{
			// msg 284: can not drop system SQL role @1
			status_exception::raise(Arg::PrivateDyn(284) << name);
		}

		if (tdbb->getAttachment()->locksmith() || roleOwner == user)
		{
			AutoCacheRequest request2(tdbb, drq_del_role_1, DYN_REQUESTS);

			// The first OR clause finds all members of the role.
			// The 2nd OR clause finds all privileges granted to the role
			/*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
				PRIV IN RDB$USER_PRIVILEGES
				WITH (PRIV.RDB$RELATION_NAME EQ name.c_str() AND PRIV.RDB$OBJECT_TYPE = obj_sql_role) OR
					 (PRIV.RDB$USER EQ name.c_str() AND PRIV.RDB$USER_TYPE = obj_sql_role)*/
			{
			request2.compile(tdbb, (UCHAR*) jrd_178, sizeof(jrd_178));
			gds__vtov ((const char*) name.c_str(), (char*) jrd_179.jrd_180, 32);
			gds__vtov ((const char*) name.c_str(), (char*) jrd_179.jrd_181, 32);
			jrd_179.jrd_182 = obj_sql_role;
			jrd_179.jrd_183 = obj_sql_role;
			EXE_start (tdbb, request2, transaction);
			EXE_send (tdbb, request2, 0, 68, (UCHAR*) &jrd_179);
			while (1)
			   {
			   EXE_receive (tdbb, request2, 1, 2, (UCHAR*) &jrd_184);
			   if (!jrd_184.jrd_185) break;
			{
				/*ERASE PRIV;*/
				EXE_send (tdbb, request2, 2, 2, (UCHAR*) &jrd_186);
			}
			/*END_FOR*/
			   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_188);
			   }
			}

			/*ERASE ROL;*/
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_197);
		}
		else
		{
			// msg 191: "only owner of SQL role or USR_locksmith could drop SQL role"
			status_exception::raise(Arg::PrivateDyn(191));
		}

		found = true;
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_199);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_ROLE, name);
	else
	{
		// msg 155: "Role %s not found"
		status_exception::raise(Arg::PrivateDyn(155) << name);
	}

	savePoint.release();	// everything is ok
}


//----------------------


void CreateAlterUserNode::print(string& text) const
{
	text.printf(
		"CreateAlterUserNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void CreateAlterUserNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
	if (mode != USER_ADD && !password && !firstName && !middleName && !lastName &&
		!adminRole.specified && !active.specified && !comment && !properties.hasData())
	{
		// 283: ALTER USER requires at least one clause to be specified
		status_exception::raise(Arg::PrivateDyn(283));
	}

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	Auth::DynamicUserData* userData = FB_NEW(*transaction->tra_pool) Auth::DynamicUserData;

	string text = name.c_str();
	if (text.isEmpty() && mode == USER_MOD)
	{
		// alter current user
		UserId* usr = tdbb->getAttachment()->att_user;
		fb_assert(usr);

		if (!usr)
			(Arg::Gds(isc_random) << "Missing user name for ALTER CURRENT USER").raise();

		text = usr->usr_user_name;
	}
	text.upper();

	userData->op = mode == USER_ADD ? Auth::ADD_OPER : mode == USER_MOD ?
		Auth::MOD_OPER : Auth::ADDMOD_OPER;
	userData->user.set(text.c_str());
	userData->user.setEntered(1);

	if (password)
	{
		if (password->isEmpty())
		{
			// 250: Password should not be empty string
			status_exception::raise(Arg::PrivateDyn(250));
		}

		userData->pass.set(password->c_str());
		userData->pass.setEntered(1);
	}

	if (firstName)
	{
		if (firstName->hasData())
		{
			userData->first.set(firstName->c_str());
			userData->first.setEntered(1);
		}
		else
		{
			userData->first.setEntered(0);
			userData->first.setSpecified(1);
		}
	}

	if (middleName)
	{
		if (middleName->hasData())
		{
			userData->middle.set(middleName->c_str());
			userData->middle.setEntered(1);
		}
		else
		{
			userData->middle.setEntered(0);
			userData->middle.setSpecified(1);
		}
	}

	if (lastName)
	{
		if (lastName->hasData())
		{
			userData->last.set(lastName->c_str());
			userData->last.setEntered(1);
		}
		else
		{
			userData->last.setEntered(0);
			userData->last.setSpecified(1);
		}
	}

	if (comment)
	{
		if (comment->hasData())
		{
			userData->com.set(comment->c_str());
			userData->com.setEntered(1);
		}
		else
		{
			userData->com.setEntered(0);
			userData->com.setSpecified(1);
		}
	}

	if (adminRole.specified)
	{
		userData->adm.set(adminRole.value);
		userData->adm.setEntered(1);
	}

	if (active.specified)
	{
		userData->act.set((int) active.value);
		userData->act.setEntered(1);
	}

	string attributesBuffer;

	for (unsigned cnt = 0; cnt < properties.getCount(); ++cnt)
	{
		if (mode != USER_ADD || properties[cnt].value.hasData())
		{
			string attribute;
			attribute.printf("%s=%s\n", properties[cnt].property.c_str(), properties[cnt].value.c_str());
			attributesBuffer += attribute;
		}
	}

	if (attributesBuffer.hasData())
	{
		userData->attr.set(attributesBuffer.c_str());
		userData->attr.setEntered(1);
	}

	const int ddlAction = mode == USER_ADD ? DDL_TRIGGER_CREATE_USER : DDL_TRIGGER_ALTER_USER;

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, ddlAction, userData->user.get());

	const USHORT id = transaction->getUserManagement()->put(userData);
	DFW_post_work(transaction, dfw_user_management, NULL, id);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, ddlAction, userData->user.get());

	savePoint.release();	// everything is ok
}


//----------------------


void DropUserNode::print(string& text) const
{
	text.printf(
		"DropUserNode\n"
		"  name: '%s'\n",
		name.c_str());
}

void DropUserNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	Auth::DynamicUserData* userData = FB_NEW(*transaction->tra_pool) Auth::DynamicUserData;

	string text = name.c_str();
	text.upper();

	userData->op = Auth::DEL_OPER;
	userData->user.set(text.c_str());
	userData->user.setEntered(1);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE, DDL_TRIGGER_DROP_USER,
		userData->user.get());

	const USHORT id = transaction->getUserManagement()->put(userData);
	DFW_post_work(transaction, dfw_user_management, NULL, id);

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_USER,
		userData->user.get());

	savePoint.release();	// everything is ok
}


//----------------------


void GrantRevokeNode::print(string& text) const
{
	text.printf(
		"GrantRevokeNode\n"
		"  isGrant: '%d'\n",
		isGrant);
}

void GrantRevokeNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
{
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	const GranteeClause* usersPtr;
	const GranteeClause* usersEnd;

	if (!isGrant && roles.isEmpty() && privileges.isEmpty() && !object)	// REVOKE ALL ON ALL
	{
		usersEnd = users.end();
		for (usersPtr = users.begin(); usersPtr != usersEnd; ++usersPtr)
			grantRevoke(tdbb, transaction, NULL, usersPtr, NULL, NULL, 0);
	}
	else
	{
		SSHORT option = 0; // no grant/admin option

		if (roles.isEmpty())
		{
			if (grantAdminOption)
				option = 1; // with grant option

			usersEnd = users.end();
			for (usersPtr = users.begin(); usersPtr != usersEnd; ++usersPtr)
				modifyPrivileges(tdbb, transaction, option, usersPtr);
		}
		else
		{
			if (grantAdminOption)
				option = 2; // with admin option

			const GranteeClause* rolesEnd = roles.end();
			for (const GranteeClause* rolesPtr = roles.begin(); rolesPtr != rolesEnd; ++rolesPtr)
			{
				usersEnd = users.end();
				for (usersPtr = users.begin(); usersPtr != usersEnd; ++usersPtr)
					grantRevoke(tdbb, transaction, rolesPtr, usersPtr, "M", NULL, option);
			}
		}
	}

	savePoint.release();	// everything is ok
}

void GrantRevokeNode::modifyPrivileges(thread_db* tdbb, jrd_tra* transaction, SSHORT option,
	const GranteeClause* user)
{
	string privs;

	for (PrivilegeClause* i = privileges.begin(); i != privileges.end(); ++i)
	{
		if (i->first == 'A')
			grantRevoke(tdbb, transaction, object, user, "A", NULL, option);
		else if (i->second)
		{
			char privs0[2] = {i->first, '\0'};

			ValueListNode* fields = i->second;

			for (NestConst<ValueExprNode>* ptr = fields->items.begin(); ptr != fields->items.end(); ++ptr)
			{
				grantRevoke(tdbb, transaction, object, user, privs0,
					(*ptr)->as<FieldNode>()->dsqlName, option);
			}
		}
		else
			privs += i->first;
	}

	if (privs.hasData())
		grantRevoke(tdbb, transaction, object, user, privs.c_str(), NULL, option);
}

// Execute SQL grant/revoke operation.
void GrantRevokeNode::grantRevoke(thread_db* tdbb, jrd_tra* transaction, const GranteeClause* object,
	const GranteeClause* userNod, const char* privs,
	const MetaName& field, int options)
{
   struct {
          SSHORT jrd_134;	// gds__utility 
   } jrd_133;
   struct {
          SSHORT jrd_132;	// gds__utility 
   } jrd_131;
   struct {
          TEXT  jrd_129 [32];	// RDB$GRANTOR 
          SSHORT jrd_130;	// gds__utility 
   } jrd_128;
   struct {
          TEXT  jrd_123 [32];	// RDB$USER 
          TEXT  jrd_124 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_125;	// RDB$USER_TYPE 
          SSHORT jrd_126;	// RDB$OBJECT_TYPE 
          TEXT  jrd_127 [7];	// RDB$PRIVILEGE 
   } jrd_122;
   struct {
          SSHORT jrd_149;	// gds__utility 
   } jrd_148;
   struct {
          SSHORT jrd_147;	// gds__utility 
   } jrd_146;
   struct {
          TEXT  jrd_144 [32];	// RDB$GRANTOR 
          SSHORT jrd_145;	// gds__utility 
   } jrd_143;
   struct {
          TEXT  jrd_137 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_138 [32];	// RDB$USER 
          TEXT  jrd_139 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_140;	// RDB$USER_TYPE 
          SSHORT jrd_141;	// RDB$OBJECT_TYPE 
          TEXT  jrd_142 [7];	// RDB$PRIVILEGE 
   } jrd_136;
   struct {
          SSHORT jrd_166;	// gds__utility 
   } jrd_165;
   struct {
          SSHORT jrd_164;	// gds__utility 
   } jrd_163;
   struct {
          SSHORT jrd_160;	// gds__utility 
          SSHORT jrd_161;	// gds__null_flag 
          SSHORT jrd_162;	// RDB$GRANT_OPTION 
   } jrd_159;
   struct {
          TEXT  jrd_152 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_153 [32];	// RDB$GRANTOR 
          TEXT  jrd_154 [32];	// RDB$USER 
          TEXT  jrd_155 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_156;	// RDB$USER_TYPE 
          SSHORT jrd_157;	// RDB$OBJECT_TYPE 
          TEXT  jrd_158 [7];	// RDB$PRIVILEGE 
   } jrd_151;
   struct {
          SSHORT jrd_177;	// gds__utility 
   } jrd_176;
   struct {
          SSHORT jrd_175;	// gds__utility 
   } jrd_174;
   struct {
          TEXT  jrd_172 [32];	// RDB$GRANTOR 
          SSHORT jrd_173;	// gds__utility 
   } jrd_171;
   struct {
          TEXT  jrd_169 [32];	// RDB$USER 
          SSHORT jrd_170;	// RDB$USER_TYPE 
   } jrd_168;
	SSHORT userType = userNod->first;
	MetaName user(userNod->second);
    MetaName dummyName;

	switch (userType)
	{
		case obj_user_or_role:
			// This test may become obsolete as we now allow explicit ROLE keyword.
			if (isItSqlRole(tdbb, transaction, user, dummyName))
			{
				userType = obj_sql_role;
				if (user == NULL_ROLE)
				{
					// msg 195: keyword NONE could not be used as SQL role name.
					status_exception::raise(Arg::PrivateDyn(195) << user.c_str());
				}
			}
			else
			{
				userType = obj_user;
				user.upper7();
			}
			break;

		case obj_user:
			user.upper7();
			break;

		case obj_sql_role:
			if (!isItSqlRole(tdbb, transaction, user, dummyName))
			{
				// msg 188: Role doesn't exist.
				status_exception::raise(Arg::PrivateDyn(188) << user.c_str());
			}
			if (user == NULL_ROLE)
			{
				// msg 195: keyword NONE could not be used as SQL role name.
				status_exception::raise(Arg::PrivateDyn(195) << user.c_str());
			}
			break;
	}

	MetaName grantorRevoker(grantor ?
		*grantor : tdbb->getAttachment()->att_user->usr_user_name);

	if (grantor && !tdbb->getAttachment()->locksmith())
		status_exception::raise(Arg::PrivateDyn(252) << SYSDBA_USER_NAME);

	grantorRevoker.upper7();

	if (!isGrant && !privs)	// REVOKE ALL ON ALL
	{
		AutoCacheRequest request(tdbb, drq_e_grant3, DYN_REQUESTS);
		bool grantErased = false;
		bool badGrantor = false;

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			PRIV IN RDB$USER_PRIVILEGES
			WITH PRIV.RDB$USER = user.c_str() AND
				 PRIV.RDB$USER_TYPE = userType*/
		{
		request.compile(tdbb, (UCHAR*) jrd_167, sizeof(jrd_167));
		gds__vtov ((const char*) user.c_str(), (char*) jrd_168.jrd_169, 32);
		jrd_168.jrd_170 = userType;
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_168);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_171);
		   if (!jrd_171.jrd_173) break;
		{
			if (tdbb->getAttachment()->att_user->locksmith() || grantorRevoker == /*PRIV.RDB$GRANTOR*/
											      jrd_171.jrd_172)
			{
				/*ERASE PRIV;*/
				EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_174);
				grantErased = true;
			}
			else
				badGrantor = true;
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_176);
		   }
		}

		const char* all = "ALL";

		if (badGrantor && !grantErased)
		{
			// msg 246: @1 is not grantor of @2 on @3 to @4.
			status_exception::raise(Arg::PrivateDyn(246) <<
				grantorRevoker.c_str() << all << all << user.c_str());
		}

		if (!grantErased)
		{
			// msg 247: Warning: @1 on @2 is not granted to @3.
			ERR_post_warning(
				Arg::Warning(isc_dyn_miss_priv_warning) <<
				all << all << Arg::Str(user));
		}

		return;
	}

	const SSHORT objType = object->first;
	const MetaName objName(object->second);

	char privileges[16];
	strcpy(privileges, privs);
	if (strcmp(privileges, "A") == 0)
		strcpy(privileges, ALL_PRIVILEGES);

	if (objType == obj_sql_role && objName == NULL_ROLE)
	{
		if (isGrant)
		{
			// msg 195: keyword NONE could not be used as SQL role name.
			status_exception::raise(Arg::PrivateDyn(195) << objName.c_str());
		}
		else
		{
			///CVC: Make this a warning in the future.
			///DYN_error_punt(false, 195, objName.c_str());
		}
	}

	char priv[2];
	priv[1] = '\0';

	if (isGrant)
	{
		AutoCacheRequest request(tdbb, drq_l_grant1, DYN_REQUESTS);

		for (const char* pr = privileges; *pr; ++pr)
		{
			bool duplicate = false;
			priv[0] = *pr;

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
				PRIV IN RDB$USER_PRIVILEGES
				WITH PRIV.RDB$RELATION_NAME EQ objName.c_str() AND
					 PRIV.RDB$OBJECT_TYPE = objType AND
					 PRIV.RDB$PRIVILEGE EQ priv AND
					 PRIV.RDB$USER = user.c_str() AND
					 PRIV.RDB$USER_TYPE = userType AND
					 PRIV.RDB$GRANTOR EQ grantorRevoker.c_str() AND
					 PRIV.RDB$FIELD_NAME EQUIV NULLIF(field.c_str(), '')*/
			{
			request.compile(tdbb, (UCHAR*) jrd_150, sizeof(jrd_150));
			gds__vtov ((const char*) field.c_str(), (char*) jrd_151.jrd_152, 32);
			gds__vtov ((const char*) grantorRevoker.c_str(), (char*) jrd_151.jrd_153, 32);
			gds__vtov ((const char*) user.c_str(), (char*) jrd_151.jrd_154, 32);
			gds__vtov ((const char*) objName.c_str(), (char*) jrd_151.jrd_155, 32);
			jrd_151.jrd_156 = userType;
			jrd_151.jrd_157 = objType;
			gds__vtov ((const char*) priv, (char*) jrd_151.jrd_158, 7);
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 139, (UCHAR*) &jrd_151);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_159);
			   if (!jrd_159.jrd_160) break;
			{
				if (/*PRIV.RDB$GRANT_OPTION.NULL*/
				    jrd_159.jrd_161 ||
					/*PRIV.RDB$GRANT_OPTION*/
					jrd_159.jrd_162 ||
					/*PRIV.RDB$GRANT_OPTION*/
					jrd_159.jrd_162 == options)
				{
					duplicate = true;
				}
				else
					/*ERASE PRIV;*/
					EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_163);	// has to be 0 and options == 1
			}
			/*END_FOR*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_165);
			   }
			}

			if (duplicate)
				continue;

			if (objType == obj_sql_role)
			{
				checkGrantorCanGrantRole(tdbb, transaction, grantorRevoker, objName);

				if (userType == obj_sql_role)
				{
					// Temporary restriction. This should be removed once GRANT role1 TO rolex is
					// supported and this message could be reused for blocking cycles of role grants.
					status_exception::raise(Arg::PrivateDyn(192) << user.c_str());
				}
			}
			else
			{
				// In the case where the object is a view, then the grantor must have
				// some kind of grant privileges on the base table(s)/view(s).  If the
				// grantor is the owner of the view, then we have to explicitely check
				// this because the owner of a view by default has grant privileges on
				// his own view.  If the grantor is not the owner of the view, then the
				// base table/view grant privilege checks were made when the grantor
				// got its grant privilege on the view and no further checks are
				// necessary.
				// As long as only locksmith can use GRANTED BY, no need specially checking
				// for privileges of current user. AP-2008

				if (objType == 0)
				{
					// Relation or view because we cannot distinguish at this point.
					checkGrantorCanGrant(tdbb, transaction,
						tdbb->getAttachment()->att_user->usr_user_name.c_str(), priv, objName,
						field, true);
				}
			}

			storePrivilege(tdbb, transaction, objName, user, field, pr, userType, objType,
				options, grantorRevoker);
		}
	}
	else	// REVOKE
	{
		AutoCacheRequest request(tdbb, (field.hasData() ? drq_e_grant1 : drq_e_grant2), DYN_REQUESTS);

		for (const char* pr = privileges; (priv[0] = *pr); ++pr)
		{
			bool grantErased = false;
			bool badGrantor = false;

			if (field.hasData())
			{
				/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
					PRIV IN RDB$USER_PRIVILEGES
					WITH PRIV.RDB$RELATION_NAME EQ objName.c_str() AND
						 PRIV.RDB$OBJECT_TYPE = objType AND
						 PRIV.RDB$PRIVILEGE EQ priv AND
						 PRIV.RDB$USER = user.c_str() AND
						 PRIV.RDB$USER_TYPE = userType AND
						 PRIV.RDB$FIELD_NAME EQ field.c_str()*/
				{
				request.compile(tdbb, (UCHAR*) jrd_135, sizeof(jrd_135));
				gds__vtov ((const char*) field.c_str(), (char*) jrd_136.jrd_137, 32);
				gds__vtov ((const char*) user.c_str(), (char*) jrd_136.jrd_138, 32);
				gds__vtov ((const char*) objName.c_str(), (char*) jrd_136.jrd_139, 32);
				jrd_136.jrd_140 = userType;
				jrd_136.jrd_141 = objType;
				gds__vtov ((const char*) priv, (char*) jrd_136.jrd_142, 7);
				EXE_start (tdbb, request, transaction);
				EXE_send (tdbb, request, 0, 107, (UCHAR*) &jrd_136);
				while (1)
				   {
				   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_143);
				   if (!jrd_143.jrd_145) break;
				{
					if (grantorRevoker == /*PRIV.RDB$GRANTOR*/
							      jrd_143.jrd_144)
					{
						/*ERASE PRIV;*/
						EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_146);
						grantErased = true;
					}
					else
						badGrantor = true;
				}
				/*END_FOR*/
				   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_148);
				   }
				}
			}
			else
			{
				/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
					PRIV IN RDB$USER_PRIVILEGES
					WITH PRIV.RDB$PRIVILEGE EQ priv AND
						 PRIV.RDB$RELATION_NAME EQ objName.c_str() AND
						 PRIV.RDB$OBJECT_TYPE = objType AND
						 PRIV.RDB$USER EQ user.c_str() AND
						 PRIV.RDB$USER_TYPE = userType*/
				{
				request.compile(tdbb, (UCHAR*) jrd_121, sizeof(jrd_121));
				gds__vtov ((const char*) user.c_str(), (char*) jrd_122.jrd_123, 32);
				gds__vtov ((const char*) objName.c_str(), (char*) jrd_122.jrd_124, 32);
				jrd_122.jrd_125 = userType;
				jrd_122.jrd_126 = objType;
				gds__vtov ((const char*) priv, (char*) jrd_122.jrd_127, 7);
				EXE_start (tdbb, request, transaction);
				EXE_send (tdbb, request, 0, 75, (UCHAR*) &jrd_122);
				while (1)
				   {
				   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_128);
				   if (!jrd_128.jrd_130) break;
				{
					// Revoking a permission at the table level implies revoking the perm. on all
					// columns. So for all fields in this table which have been granted the
					// privilege, we erase the entries from RDB$USER_PRIVILEGES.

					if (grantorRevoker == /*PRIV.RDB$GRANTOR*/
							      jrd_128.jrd_129)
					{
						/*ERASE PRIV;*/
						EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_131);
						grantErased = true;
					}
					else
						badGrantor = true;
				}
				/*END_FOR*/
				   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_133);
				   }
				}
			}

			if (options && grantErased)
			{
				// Add the privilege without the grant option. There is a modify trigger on the
				// rdb$user_privileges which disallows the table from being updated. It would have
				// to be changed such that only the grant_option field can be updated.

				storePrivilege(tdbb, transaction, objName, user, field, pr, userType, objType,
					0, grantorRevoker);
			}

			if (badGrantor && !grantErased)
			{
				// msg 246: @1 is not grantor of @2 on @3 to @4.
				status_exception::raise(Arg::PrivateDyn(246) <<
					grantorRevoker.c_str() << privilegeName(priv[0]) << objName.c_str() <<
					user.c_str());
			}

			if (!grantErased)
			{
				// msg 247: Warning: @1 on @2 is not granted to @3.
				ERR_post_warning(
					Arg::Warning(isc_dyn_miss_priv_warning) <<
					Arg::Str(privilegeName(priv[0])) << Arg::Str(objName) << Arg::Str(user));
			}
		}
	}
}

// Check if the grantor has grant privilege on the relation/field.
void GrantRevokeNode::checkGrantorCanGrant(thread_db* tdbb, jrd_tra* transaction,
	const char* grantor, const char* privilege, const MetaName& relationName,
	const MetaName& fieldName, bool topLevel)
{
   struct {
          TEXT  jrd_85 [32];	// RDB$BASE_FIELD 
          TEXT  jrd_86 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_87 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_88;	// gds__utility 
   } jrd_84;
   struct {
          TEXT  jrd_83 [32];	// RDB$RELATION_NAME 
   } jrd_82;
   struct {
          TEXT  jrd_97 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_98;	// gds__utility 
          SSHORT jrd_99;	// gds__null_flag 
          SSHORT jrd_100;	// RDB$GRANT_OPTION 
          SSHORT jrd_101;	// gds__null_flag 
   } jrd_96;
   struct {
          TEXT  jrd_91 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_92 [32];	// RDB$USER 
          SSHORT jrd_93;	// RDB$OBJECT_TYPE 
          SSHORT jrd_94;	// RDB$USER_TYPE 
          TEXT  jrd_95 [7];	// RDB$PRIVILEGE 
   } jrd_90;
   struct {
          SSHORT jrd_107;	// gds__utility 
   } jrd_106;
   struct {
          TEXT  jrd_104 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_105 [32];	// RDB$RELATION_NAME 
   } jrd_103;
   struct {
          SSHORT jrd_113;	// gds__utility 
   } jrd_112;
   struct {
          TEXT  jrd_110 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_111 [32];	// RDB$RELATION_NAME 
   } jrd_109;
   struct {
          SSHORT jrd_118;	// gds__utility 
          SSHORT jrd_119;	// gds__null_flag 
          SSHORT jrd_120;	// RDB$FLAGS 
   } jrd_117;
   struct {
          TEXT  jrd_116 [32];	// RDB$RELATION_NAME 
   } jrd_115;
	// Verify that the input relation exists.

	AutoCacheRequest request(tdbb, drq_gcg4, DYN_REQUESTS);

	bool sqlRelation = false;
	bool relationExists = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		REL IN RDB$RELATIONS WITH
			REL.RDB$RELATION_NAME = relationName.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_114, sizeof(jrd_114));
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_115.jrd_116, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_115);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_117);
	   if (!jrd_117.jrd_118) break;
	{
		relationExists = true;
		if (!/*REL.RDB$FLAGS.NULL*/
		     jrd_117.jrd_119 && (/*REL.RDB$FLAGS*/
     jrd_117.jrd_120 & REL_sql))
			sqlRelation = true;
	}
	/*END_FOR*/
	   }
	}

	if (!relationExists)
	{
		// table/view .. does not exist
		status_exception::raise(Arg::PrivateDyn(175) << relationName.c_str());
	}

	// Verify the the input field exists.

	if (fieldName.hasData())
	{
		bool fieldExists = false;

		request.reset(tdbb, drq_gcg5, DYN_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			G_FLD IN RDB$RELATION_FIELDS WITH
				G_FLD.RDB$RELATION_NAME = relationName.c_str() AND
				G_FLD.RDB$FIELD_NAME = fieldName.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_108, sizeof(jrd_108));
		gds__vtov ((const char*) fieldName.c_str(), (char*) jrd_109.jrd_110, 32);
		gds__vtov ((const char*) relationName.c_str(), (char*) jrd_109.jrd_111, 32);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_109);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_112);
		   if (!jrd_112.jrd_113) break;
		{
			fieldExists = true;
		}
		/*END_FOR*/
		   }
		}

		if (!fieldExists)
		{
			// column .. does not exist in table/view ..
			status_exception::raise(Arg::PrivateDyn(176) <<
				fieldName.c_str() << relationName.c_str());
		}
	}

	// If the current user is locksmith - allow all grants to occur

	if (tdbb->getAttachment()->locksmith())
		return;

	// If this is a non-sql table, then the owner will probably not have any
	// entries in the rdb$user_privileges table.  Give the owner of a GDML
	// table all privileges.
	bool grantorIsOwner = false;

	request.reset(tdbb, drq_gcg2, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		REL IN RDB$RELATIONS WITH
			REL.RDB$RELATION_NAME = relationName.c_str() AND
			REL.RDB$OWNER_NAME = UPPERCASE(grantor)*/
	{
	request.compile(tdbb, (UCHAR*) jrd_102, sizeof(jrd_102));
	gds__vtov ((const char*) grantor, (char*) jrd_103.jrd_104, 32);
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_103.jrd_105, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_103);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_106);
	   if (!jrd_106.jrd_107) break;
	{
		grantorIsOwner = true;
	}
	/*END_FOR*/
	   }
	}

	if (!sqlRelation && grantorIsOwner)
		return;

	// Remember the grant option for non field-specific user-privileges, and
	// the grant option for the user-privileges for the input field.
	// -1 = no privilege found (yet)
	// 0 = privilege without grant option found
	// 1 = privilege with grant option found
	SSHORT goRel = -1;
	SSHORT goFld = -1;

	// Verify that the grantor has the grant option for this relation/field
	// in the rdb$user_privileges.  If not, then we don't need to look further.

	request.reset(tdbb, drq_gcg1, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRV IN RDB$USER_PRIVILEGES WITH
			PRV.RDB$USER = UPPERCASE(grantor) AND
			PRV.RDB$USER_TYPE = obj_user AND
			PRV.RDB$RELATION_NAME = relationName.c_str() AND
			PRV.RDB$OBJECT_TYPE = obj_relation AND
			PRV.RDB$PRIVILEGE = privilege*/
	{
	request.compile(tdbb, (UCHAR*) jrd_89, sizeof(jrd_89));
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_90.jrd_91, 32);
	gds__vtov ((const char*) grantor, (char*) jrd_90.jrd_92, 32);
	jrd_90.jrd_93 = obj_relation;
	jrd_90.jrd_94 = obj_user;
	gds__vtov ((const char*) privilege, (char*) jrd_90.jrd_95, 7);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 75, (UCHAR*) &jrd_90);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 40, (UCHAR*) &jrd_96);
	   if (!jrd_96.jrd_98) break;
	{
		if (/*PRV.RDB$FIELD_NAME.NULL*/
		    jrd_96.jrd_101)
		{
			if (/*PRV.RDB$GRANT_OPTION.NULL*/
			    jrd_96.jrd_99 || !/*PRV.RDB$GRANT_OPTION*/
     jrd_96.jrd_100)
				goRel = 0;
			else if (goRel)
				goRel = 1;
		}
		else
		{
			if (/*PRV.RDB$GRANT_OPTION.NULL*/
			    jrd_96.jrd_99 || !/*PRV.RDB$GRANT_OPTION*/
     jrd_96.jrd_100)
			{
				if (fieldName.hasData() && fieldName == /*PRV.RDB$FIELD_NAME*/
									jrd_96.jrd_97)
					goFld = 0;
			}
			else
			{
				if (fieldName.hasData() && fieldName == /*PRV.RDB$FIELD_NAME*/
									jrd_96.jrd_97)
					goFld = 1;
			}
		}
	}
	/*END_FOR*/
	   }
	}

	if (fieldName.hasData())
	{
		if (goFld == 0)
		{
			// no grant option for privilege .. on column .. of [base] table/view ..
			status_exception::raise(Arg::PrivateDyn(topLevel ? 167 : 168) <<
				privilege << fieldName.c_str() << relationName.c_str());
		}

		if (goFld == -1)
		{
			if (goRel == 0)
			{
				// no grant option for privilege .. on [base] table/view .. (for column ..)
				status_exception::raise(Arg::PrivateDyn(topLevel ? 169 : 170) <<
					privilege << relationName.c_str() << fieldName.c_str());
			}

			if (goRel == -1)
			{
				// no .. privilege with grant option on [base] table/view .. (for column ..)
				status_exception::raise(Arg::PrivateDyn(topLevel ? 171 : 172) <<
					privilege << relationName.c_str() << fieldName.c_str());
			}
		}
	}
	else
	{
		if (goRel == 0)
		{
			// no grant option for privilege .. on table/view ..
			status_exception::raise(Arg::PrivateDyn(173) << privilege << relationName.c_str());
		}

		if (goRel == -1)
		{
			// no .. privilege with grant option on table/view ..
			status_exception::raise(Arg::PrivateDyn(174) << privilege << relationName.c_str());
		}
	}

	// If the grantor is not the owner of the relation, then we don't need to
	// check the base table(s)/view(s) because that check was performed when
	// the grantor was given its privileges.

	if (!grantorIsOwner)
		return;

	// Find all the base fields/relations and check for the correct grant privileges on them.

	request.reset(tdbb, drq_gcg3, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		G_FLD IN RDB$RELATION_FIELDS CROSS
			G_VIEW IN RDB$VIEW_RELATIONS WITH
			G_FLD.RDB$RELATION_NAME = relationName.c_str() AND
			G_FLD.RDB$BASE_FIELD NOT MISSING AND
			G_VIEW.RDB$VIEW_NAME EQ G_FLD.RDB$RELATION_NAME AND
			G_VIEW.RDB$VIEW_CONTEXT EQ G_FLD.RDB$VIEW_CONTEXT*/
	{
	request.compile(tdbb, (UCHAR*) jrd_81, sizeof(jrd_81));
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_82.jrd_83, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_82);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 98, (UCHAR*) &jrd_84);
	   if (!jrd_84.jrd_88) break;
	{
		if (fieldName.hasData())
		{
			if (fieldName == /*G_FLD.RDB$FIELD_NAME*/
					 jrd_84.jrd_87)
			{
				checkGrantorCanGrant(tdbb, transaction, grantor, privilege,
					/*G_VIEW.RDB$RELATION_NAME*/
					jrd_84.jrd_86, /*G_FLD.RDB$BASE_FIELD*/
  jrd_84.jrd_85, false);
			}
		}
		else
		{
			checkGrantorCanGrant(tdbb, transaction, grantor, privilege,
				/*G_VIEW.RDB$RELATION_NAME*/
				jrd_84.jrd_86, /*G_FLD.RDB$BASE_FIELD*/
  jrd_84.jrd_85, false);
		}
	}
	/*END_FOR*/
	   }
	}
}

// Check if the grantor has admin privilege on the role.
void GrantRevokeNode::checkGrantorCanGrantRole(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& grantor, const MetaName& roleName)
{
   struct {
          SSHORT jrd_79;	// gds__utility 
          SSHORT jrd_80;	// RDB$GRANT_OPTION 
   } jrd_78;
   struct {
          TEXT  jrd_74 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_75 [32];	// RDB$USER 
          SSHORT jrd_76;	// RDB$OBJECT_TYPE 
          SSHORT jrd_77;	// RDB$USER_TYPE 
   } jrd_73;
	// Fetch the name of the owner of the ROLE.
	MetaName owner;
	if (isItSqlRole(tdbb, transaction, roleName, owner))
	{
		// Both SYSDBA and the owner of this ROLE can grant membership
		if (tdbb->getAttachment()->locksmith() || owner == grantor)
			return;
	}
	else
	{
		// 188: role name not exist.
		status_exception::raise(Arg::PrivateDyn(188) << roleName.c_str());
	}

	AutoCacheRequest request(tdbb, drq_get_role_au, DYN_REQUESTS);
	bool grantable = false;
	bool noAdmin = false;

	// The 'grantor' is not the owner of the ROLE, see if they have admin privilege on the role.
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRV IN RDB$USER_PRIVILEGES WITH
			PRV.RDB$USER = UPPERCASE(grantor.c_str()) AND
			PRV.RDB$USER_TYPE = obj_user AND
			PRV.RDB$RELATION_NAME EQ roleName.c_str() AND
			PRV.RDB$OBJECT_TYPE = obj_sql_role AND
			PRV.RDB$PRIVILEGE EQ "M"*/
	{
	request.compile(tdbb, (UCHAR*) jrd_72, sizeof(jrd_72));
	gds__vtov ((const char*) roleName.c_str(), (char*) jrd_73.jrd_74, 32);
	gds__vtov ((const char*) grantor.c_str(), (char*) jrd_73.jrd_75, 32);
	jrd_73.jrd_76 = obj_sql_role;
	jrd_73.jrd_77 = obj_user;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_73);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_78);
	   if (!jrd_78.jrd_79) break;
	{
		if (/*PRV.RDB$GRANT_OPTION*/
		    jrd_78.jrd_80 == 2)
			grantable = true;
		else
			noAdmin = true;
	}
	/*END_FOR*/
	   }
	}

	if (!grantable)
	{
		// 189: user have no admin option.
		// 190: user is not a member of the role.
		status_exception::raise(Arg::PrivateDyn(noAdmin ? 189 : 190) <<
			grantor.c_str() << roleName.c_str());
	}
}

void GrantRevokeNode::storePrivilege(thread_db* tdbb, jrd_tra* transaction, const MetaName& object,
	const MetaName& user, const MetaName& field, const TEXT* privilege, SSHORT userType,
	SSHORT objType, int option, const MetaName& grantor)
{
   struct {
          TEXT  jrd_63 [32];	// RDB$GRANTOR 
          TEXT  jrd_64 [32];	// RDB$USER 
          TEXT  jrd_65 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_66 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_67;	// RDB$GRANT_OPTION 
          SSHORT jrd_68;	// RDB$OBJECT_TYPE 
          SSHORT jrd_69;	// RDB$USER_TYPE 
          SSHORT jrd_70;	// gds__null_flag 
          TEXT  jrd_71 [7];	// RDB$PRIVILEGE 
   } jrd_62;
	AutoCacheRequest request(tdbb, drq_s_grant, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES*/
	{
	
		/*PRIV.RDB$FIELD_NAME.NULL*/
		jrd_62.jrd_70 = TRUE;
		strcpy(/*PRIV.RDB$RELATION_NAME*/
		       jrd_62.jrd_65, object.c_str());
		strcpy(/*PRIV.RDB$USER*/
		       jrd_62.jrd_64, user.c_str());
		strcpy(/*PRIV.RDB$GRANTOR*/
		       jrd_62.jrd_63, grantor.c_str());
		/*PRIV.RDB$USER_TYPE*/
		jrd_62.jrd_69 = userType;
		/*PRIV.RDB$OBJECT_TYPE*/
		jrd_62.jrd_68 = objType;
	{
		if (field.hasData())
		{
			strcpy(/*PRIV.RDB$FIELD_NAME*/
			       jrd_62.jrd_66, field.c_str());
			/*PRIV.RDB$FIELD_NAME.NULL*/
			jrd_62.jrd_70 = FALSE;
			setFieldClassName(tdbb, transaction, object, field);
		}
		/*PRIV.RDB$PRIVILEGE*/
		jrd_62.jrd_71[0] = privilege[0];
		/*PRIV.RDB$PRIVILEGE*/
		jrd_62.jrd_71[1] = 0;
		/*PRIV.RDB$GRANT_OPTION*/
		jrd_62.jrd_67 = option;
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_61, sizeof(jrd_61));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 143, (UCHAR*) &jrd_62);
	}
}

// For field level grants, be sure the field has a unique class name.
void GrantRevokeNode::setFieldClassName(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& relation, const MetaName& field)
{
   struct {
          SSHORT jrd_47;	// gds__utility 
   } jrd_46;
   struct {
          TEXT  jrd_45 [32];	// RDB$SECURITY_CLASS 
   } jrd_44;
   struct {
          SSHORT jrd_60;	// gds__utility 
   } jrd_59;
   struct {
          TEXT  jrd_57 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_58;	// gds__null_flag 
   } jrd_56;
   struct {
          TEXT  jrd_53 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_54;	// gds__utility 
          SSHORT jrd_55;	// gds__null_flag 
   } jrd_52;
   struct {
          TEXT  jrd_50 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_51 [32];	// RDB$FIELD_NAME 
   } jrd_49;
	AutoCacheRequest request(tdbb, drq_s_f_class, DYN_REQUESTS);

	bool unique = false;

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		RFR IN RDB$RELATION_FIELDS
		WITH RFR.RDB$FIELD_NAME = field.c_str() AND
			 RFR.RDB$RELATION_NAME = relation.c_str() AND
			 RFR.RDB$SECURITY_CLASS MISSING*/
	{
	request.compile(tdbb, (UCHAR*) jrd_48, sizeof(jrd_48));
	gds__vtov ((const char*) relation.c_str(), (char*) jrd_49.jrd_50, 32);
	gds__vtov ((const char*) field.c_str(), (char*) jrd_49.jrd_51, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_49);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_52);
	   if (!jrd_52.jrd_54) break;
	{
		/*MODIFY RFR*/
		{
		
			while (!unique)
		    {
				sprintf(/*RFR.RDB$SECURITY_CLASS*/
					jrd_52.jrd_53, "%s%" SQUADFORMAT, SQL_FLD_SECCLASS_PREFIX,
				DPM_gen_id(tdbb, MET_lookup_generator(tdbb, SQL_SECCLASS_GENERATOR), false, 1));

				unique = true;

				AutoCacheRequest request2(tdbb, drq_s_u_class, DYN_REQUESTS);
				/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
					RFR1 IN RDB$RELATION_FIELDS
					WITH RFR1.RDB$SECURITY_CLASS = RFR.RDB$SECURITY_CLASS*/
				{
				request2.compile(tdbb, (UCHAR*) jrd_43, sizeof(jrd_43));
				gds__vtov ((const char*) jrd_52.jrd_53, (char*) jrd_44.jrd_45, 32);
				EXE_start (tdbb, request2, transaction);
				EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_44);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 2, (UCHAR*) &jrd_46);
				   if (!jrd_46.jrd_47) break;
				{
					unique = false;
				}
				/*END_FOR*/
				   }
				}
		    }

			/*RFR.RDB$SECURITY_CLASS.NULL*/
			jrd_52.jrd_55 = FALSE;
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_52.jrd_53, (char*) jrd_56.jrd_57, 32);
		jrd_56.jrd_58 = jrd_52.jrd_55;
		EXE_send (tdbb, request, 2, 34, (UCHAR*) &jrd_56);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_59);
	   }
	}
}


//----------------------


void AlterDatabaseNode::print(string& text) const
{
	text.printf(
		"AlterDatabaseNode\n");
}

void AlterDatabaseNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_42;	// gds__utility 
   } jrd_41;
   struct {
          TEXT  jrd_37 [32];	// RDB$CHARACTER_SET_NAME 
          SLONG  jrd_38;	// RDB$LINGER 
          SSHORT jrd_39;	// gds__null_flag 
          SSHORT jrd_40;	// gds__null_flag 
   } jrd_36;
   struct {
          TEXT  jrd_31 [32];	// RDB$CHARACTER_SET_NAME 
          SLONG  jrd_32;	// RDB$LINGER 
          SSHORT jrd_33;	// gds__utility 
          SSHORT jrd_34;	// gds__null_flag 
          SSHORT jrd_35;	// gds__null_flag 
   } jrd_30;
	if (!tdbb->getAttachment()->locksmith())
		status_exception::raise(Arg::Gds(isc_adm_task_denied));

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	if (cryptPlugin.hasData())
		DFW_post_work(transaction, dfw_db_crypt, cryptPlugin.c_str(), 0);

	if (clauses & CLAUSE_DECRYPT)
		DFW_post_work(transaction, dfw_db_crypt, "", 0);

	Attachment* const attachment = transaction->tra_attachment;
	SLONG dbAlloc = PageSpace::maxAlloc(tdbb->getDatabase());
	SLONG start = create ? createLength + 1 : 0;
	AutoCacheRequest request(tdbb, drq_m_database, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		DBB IN RDB$DATABASE*/
	{
	request.compile(tdbb, (UCHAR*) jrd_29, sizeof(jrd_29));
	EXE_start (tdbb, request, transaction);
	while (1)
	   {
	   EXE_receive (tdbb, request, 0, 42, (UCHAR*) &jrd_30);
	   if (!jrd_30.jrd_33) break;
	{
		/*MODIFY DBB USING*/
		{
		
			if (clauses & CLAUSE_DROP_DIFFERENCE)
				changeBackupMode(tdbb, transaction, CLAUSE_DROP_DIFFERENCE);

			for (NestConst<DbFileClause>* i = files.begin(); i != files.end(); ++i)
			{
				DbFileClause* file = *i;

				start = MAX(start, file->start);
				defineFile(tdbb, transaction, 0, false, false, dbAlloc,
					file->name.c_str(), start, file->length);
				start += file->length;
			}

			if (differenceFile.hasData())
				defineDifference(tdbb, transaction, differenceFile.c_str());

			if (setDefaultCharSet.hasData())
			{
				//// TODO: Validate!
				/*DBB.RDB$CHARACTER_SET_NAME.NULL*/
				jrd_30.jrd_35 = FALSE;
				strcpy(/*DBB.RDB$CHARACTER_SET_NAME*/
				       jrd_30.jrd_31, setDefaultCharSet.c_str());
			}

			if (!/*DBB.RDB$CHARACTER_SET_NAME.NULL*/
			     jrd_30.jrd_35 && setDefaultCollation.hasData())
			{
				AlterCharSetNode alterCharSetNode(getPool(), setDefaultCharSet, setDefaultCollation);
				alterCharSetNode.execute(tdbb, dsqlScratch, transaction);
			}

			if (linger >= 0)
			{
				/*DBB.RDB$LINGER.NULL*/
				jrd_30.jrd_34 = FALSE;
				/*DBB.RDB$LINGER*/
				jrd_30.jrd_32 = linger;
			}

			if (clauses & CLAUSE_BEGIN_BACKUP)
				changeBackupMode(tdbb, transaction, CLAUSE_BEGIN_BACKUP);

			if (clauses & CLAUSE_END_BACKUP)
				changeBackupMode(tdbb, transaction, CLAUSE_END_BACKUP);
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_30.jrd_31, (char*) jrd_36.jrd_37, 32);
		jrd_36.jrd_38 = jrd_30.jrd_32;
		jrd_36.jrd_39 = jrd_30.jrd_35;
		jrd_36.jrd_40 = jrd_30.jrd_34;
		EXE_send (tdbb, request, 1, 40, (UCHAR*) &jrd_36);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_41);
	   }
	}

	savePoint.release();	// everything is ok
}

// Drop backup difference file for the database, begin or end backup.
void AlterDatabaseNode::changeBackupMode(thread_db* tdbb, jrd_tra* transaction, unsigned clause)
{
   struct {
          SLONG  jrd_11;	// RDB$FILE_START 
          SSHORT jrd_12;	// RDB$FILE_FLAGS 
   } jrd_10;
   struct {
          SSHORT jrd_28;	// gds__utility 
   } jrd_27;
   struct {
          SSHORT jrd_26;	// RDB$FILE_FLAGS 
   } jrd_25;
   struct {
          SSHORT jrd_24;	// gds__utility 
   } jrd_23;
   struct {
          SSHORT jrd_22;	// gds__utility 
   } jrd_21;
   struct {
          SSHORT jrd_20;	// RDB$FILE_FLAGS 
   } jrd_19;
   struct {
          TEXT  jrd_15 [256];	// RDB$FILE_NAME 
          SSHORT jrd_16;	// gds__utility 
          SSHORT jrd_17;	// gds__null_flag 
          SSHORT jrd_18;	// RDB$FILE_FLAGS 
   } jrd_14;
	AutoCacheRequest request(tdbb, drq_d_difference, DYN_REQUESTS);
	bool invalidState = false;
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		X IN RDB$FILES*/
	{
	request.compile(tdbb, (UCHAR*) jrd_13, sizeof(jrd_13));
	EXE_start (tdbb, request, transaction);
	while (1)
	   {
	   EXE_receive (tdbb, request, 0, 262, (UCHAR*) &jrd_14);
	   if (!jrd_14.jrd_16) break;
	{
		if (/*X.RDB$FILE_FLAGS*/
		    jrd_14.jrd_18 & FILE_difference)
		{
			found = true;

			switch (clause)
			{
				case CLAUSE_DROP_DIFFERENCE:
					/*ERASE X;*/
					EXE_send (tdbb, request, 5, 2, (UCHAR*) &jrd_27);
					break;

				case CLAUSE_BEGIN_BACKUP:
					if (/*X.RDB$FILE_FLAGS*/
					    jrd_14.jrd_18 & FILE_backing_up)
						invalidState = true;
					else
					{
						/*MODIFY X USING*/
						{
						
							/*X.RDB$FILE_FLAGS*/
							jrd_14.jrd_18 |= FILE_backing_up;
						/*END_MODIFY*/
						jrd_25.jrd_26 = jrd_14.jrd_18;
						EXE_send (tdbb, request, 4, 2, (UCHAR*) &jrd_25);
						}
					}
					break;

				case CLAUSE_END_BACKUP:
					if (/*X.RDB$FILE_FLAGS*/
					    jrd_14.jrd_18 & FILE_backing_up)
					{
						if (/*X.RDB$FILE_NAME.NULL*/
						    jrd_14.jrd_17)
							/*ERASE X;*/
							EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_23);
						else
						{
							/*MODIFY X USING*/
							{
							
								/*X.RDB$FILE_FLAGS*/
								jrd_14.jrd_18 &= ~FILE_backing_up;
							/*END_MODIFY*/
							jrd_19.jrd_20 = jrd_14.jrd_18;
							EXE_send (tdbb, request, 1, 2, (UCHAR*) &jrd_19);
							}
						}
					}
					else
						invalidState = true;
					break;
			}
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_21);
	   }
	}

	if (!found && clause == CLAUSE_BEGIN_BACKUP)
	{
		request.reset(tdbb, drq_s2_difference, DYN_REQUESTS);

		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			X IN RDB$FILES*/
		{
		
		{
			/*X.RDB$FILE_FLAGS*/
			jrd_10.jrd_12 = FILE_difference | FILE_backing_up;
			/*X.RDB$FILE_START*/
			jrd_10.jrd_11 = 0;
		}
		/*END_STORE*/
		request.compile(tdbb, (UCHAR*) jrd_9, sizeof(jrd_9));
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 6, (UCHAR*) &jrd_10);
		}

		found = true;
	}

	if (invalidState)
	{
		// msg 217: "Database is already in the physical backup mode"
		// msg 218: "Database is not in the physical backup mode"
		status_exception::raise(Arg::PrivateDyn(clause == CLAUSE_BEGIN_BACKUP ? 217 : 218));
	}

	if (!found)
	{
		// msg 218: "Database is not in the physical backup mode"
		// msg 215: "Difference file is not defined"
		status_exception::raise(Arg::PrivateDyn(clause == CLAUSE_END_BACKUP ? 218 : 215));
	}
}

// Define backup difference file.
void AlterDatabaseNode::defineDifference(thread_db* tdbb, jrd_tra* transaction, const PathName& file)
{
   struct {
          TEXT  jrd_2 [256];	// RDB$FILE_NAME 
          SLONG  jrd_3;	// RDB$FILE_START 
          SSHORT jrd_4;	// RDB$FILE_FLAGS 
   } jrd_1;
   struct {
          SSHORT jrd_7;	// gds__utility 
          SSHORT jrd_8;	// RDB$FILE_FLAGS 
   } jrd_6;
	AutoCacheRequest request(tdbb, drq_l_difference, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FIL IN RDB$FILES*/
	{
	request.compile(tdbb, (UCHAR*) jrd_5, sizeof(jrd_5));
	EXE_start (tdbb, request, transaction);
	while (1)
	   {
	   EXE_receive (tdbb, request, 0, 4, (UCHAR*) &jrd_6);
	   if (!jrd_6.jrd_7) break;
	{
		if (/*FIL.RDB$FILE_FLAGS*/
		    jrd_6.jrd_8 & FILE_difference)
			found = true;
	}
	/*END_FOR*/
	   }
	}

	if (found)
	{
		// msg 216: "Difference file is already defined"
		status_exception::raise(Arg::PrivateDyn(216));
	}

	request.reset(tdbb, drq_s_difference, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		FIL IN RDB$FILES*/
	{
	
	{
		if (file.length() >= sizeof(/*FIL.RDB$FILE_NAME*/
					    jrd_1.jrd_2))
			status_exception::raise(Arg::Gds(isc_dyn_name_longer));

		strcpy(/*FIL.RDB$FILE_NAME*/
		       jrd_1.jrd_2, file.c_str());
		/*FIL.RDB$FILE_FLAGS*/
		jrd_1.jrd_4 = FILE_difference;
		/*FIL.RDB$FILE_START*/
		jrd_1.jrd_3 = 0;
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 262, (UCHAR*) &jrd_1);
	}
}


}	// namespace Jrd
