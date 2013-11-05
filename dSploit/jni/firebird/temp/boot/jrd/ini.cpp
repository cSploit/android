/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ini.epp
 *	DESCRIPTION:	Metadata initialization / population
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
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/common.h"
#include "../jrd/flags.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/ibase.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "gen/ids.h"
#include "../jrd/tra.h"
#include "../jrd/trig.h"
#include "../jrd/intl.h"
#include "../jrd/dflt.h"
#include "../jrd/ini.h"
#include "../jrd/idx.h"
#include "../jrd/gdsassert.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/ini_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/obj.h"
#include "../jrd/acl.h"
#include "../jrd/irq.h"
#include "../jrd/IntlManager.h"
#include "../jrd/constants.h"


using namespace Jrd;

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [104] =
   {	// blr string 

4,2,4,0,8,0,9,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,12,
0,15,'K',12,0,0,2,1,25,0,0,0,24,0,5,0,1,25,0,3,0,24,0,9,0,1,25,
0,4,0,24,0,3,0,1,41,0,6,0,5,0,24,0,8,0,1,25,0,7,0,24,0,2,0,1,
25,0,1,0,24,0,1,0,1,25,0,2,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_10 [107] =
   {	// blr string 

4,2,4,0,8,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,
0,7,0,12,0,15,'K',5,0,0,2,1,25,0,3,0,24,0,8,0,1,41,0,5,0,4,0,
24,0,13,0,1,25,0,6,0,24,0,9,0,1,25,0,7,0,24,0,6,0,1,25,0,0,0,
24,0,2,0,1,25,0,1,0,24,0,0,0,1,25,0,2,0,24,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_20 [56] =
   {	// blr string 

4,2,4,0,3,0,41,0,0,0,4,41,3,0,32,0,7,0,12,0,15,'K',17,0,0,2,1,
25,0,0,0,24,0,2,0,1,25,0,2,0,24,0,1,0,1,25,0,1,0,24,0,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_25 [97] =
   {	// blr string 

4,2,4,0,9,0,41,0,0,32,0,41,0,0,0,1,41,3,0,32,0,7,0,7,0,7,0,7,
0,7,0,7,0,12,0,15,'K',14,0,0,2,1,41,0,4,0,3,0,24,0,7,0,1,25,0,
5,0,24,0,6,0,1,41,0,0,0,6,0,24,0,5,0,1,41,0,1,0,7,0,24,0,4,0,
1,41,0,2,0,8,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_36 [134] =
   {	// blr string 

4,2,4,0,11,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,12,0,15,'K',15,0,0,2,1,25,0,1,0,24,0,9,0,1,25,0,2,0,24,0,
8,0,1,25,0,3,0,24,0,7,0,1,25,0,4,0,24,0,6,0,1,25,0,5,0,24,0,5,
0,1,25,0,6,0,24,0,4,0,1,25,0,7,0,24,0,3,0,1,25,0,8,0,24,0,2,0,
1,25,0,9,0,24,0,1,0,1,41,0,0,0,10,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_49 [112] =
   {	// blr string 

4,2,4,0,10,0,9,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,12,0,15,'K',29,0,0,2,1,41,0,0,0,3,0,24,0,8,0,1,25,0,4,
0,24,0,3,0,1,41,0,6,0,5,0,24,0,4,0,1,25,0,7,0,24,0,1,0,1,25,0,
8,0,24,0,2,0,1,41,0,1,0,9,0,24,0,7,0,1,25,0,2,0,24,0,0,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_61 [82] =
   {	// blr string 

4,2,4,0,6,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,12,0,15,'K',
28,0,0,2,1,41,0,3,0,2,0,24,0,5,0,1,25,0,4,0,24,0,8,0,1,25,0,5,
0,24,0,4,0,1,25,0,0,0,24,0,3,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_69 [154] =
   {	// blr string 

4,2,4,0,16,0,9,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,7,0,7,0,7,0,7,0,12,0,15,'K',2,0,0,2,1,25,0,2,0,24,0,10,
0,1,41,0,0,0,3,0,24,0,6,0,1,41,0,5,0,4,0,24,0,17,0,1,41,0,7,0,
6,0,24,0,25,0,1,41,0,9,0,8,0,24,0,26,0,1,41,0,11,0,10,0,24,0,
11,0,1,41,0,13,0,12,0,24,0,15,0,1,25,0,14,0,24,0,9,0,1,25,0,15,
0,24,0,8,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_87 [72] =
   {	// blr string 

4,2,4,0,6,0,9,0,41,3,0,32,0,7,0,7,0,7,0,7,0,12,0,15,'K',20,0,
0,2,1,41,0,0,0,2,0,24,0,3,0,1,41,0,4,0,3,0,24,0,2,0,1,25,0,5,
0,24,0,1,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_95 [164] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,41,3,0,32,0,7,0,4,1,3,0,41,3,0,32,0,7,
0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,'C',1,'K',5,0,
0,'G',58,47,24,0,1,0,25,0,1,0,47,24,0,0,0,25,0,0,0,255,2,14,1,
2,1,24,0,2,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,8,0,25,
1,2,0,255,17,0,9,13,12,3,18,0,12,2,10,0,1,2,1,25,2,1,0,24,1,8,
0,1,25,2,0,0,24,1,2,0,255,255,255,14,1,1,21,8,0,0,0,0,0,25,1,
1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_108 [124] =
   {	// blr string 

4,2,4,0,9,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,
0,7,0,7,0,7,0,41,0,0,7,0,12,0,15,'K',18,0,0,2,1,25,0,4,0,24,0,
7,0,1,25,0,5,0,24,0,6,0,1,41,0,0,0,6,0,24,0,5,0,1,25,0,1,0,24,
0,4,0,1,25,0,2,0,24,0,1,0,1,25,0,7,0,24,0,3,0,1,25,0,8,0,24,0,
2,0,1,25,0,3,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_119 [171] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,4,1,5,
0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,
0,2,7,'C',1,'K',6,0,0,'G',47,24,0,8,0,25,0,0,0,255,2,14,1,2,1,
24,0,14,0,41,1,0,0,3,0,1,24,0,9,0,41,1,1,0,4,0,1,21,8,0,1,0,0,
0,25,1,2,0,255,17,0,9,13,12,3,18,0,12,2,10,0,1,2,1,41,2,1,0,3,
0,24,1,14,0,1,41,2,0,0,2,0,24,1,9,0,255,255,255,14,1,1,21,8,0,
0,0,0,0,25,1,2,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_135 [42] =
   {	// blr string 

4,2,4,0,2,0,9,0,41,3,0,32,0,12,0,15,'K',9,0,0,2,1,25,0,0,0,24,
0,1,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_139 [42] =
   {	// blr string 

4,2,4,0,2,0,9,0,41,3,0,32,0,12,0,15,'K',9,0,0,2,1,25,0,0,0,24,
0,1,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_143 [56] =
   {	// blr string 

4,2,4,0,3,0,41,3,0,32,0,41,3,0,32,0,7,0,12,0,15,'K',3,0,0,2,1,
25,0,0,0,24,0,1,0,1,25,0,1,0,24,0,0,0,1,25,0,2,0,24,0,2,0,255,
255,76
   };	// end of blr string 
static const UCHAR	jrd_148 [119] =
   {	// blr string 

4,2,4,0,10,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,12,0,15,'K',4,0,0,2,1,25,0,2,0,24,0,2,0,1,25,0,3,0,24,
0,6,0,1,41,0,5,0,4,0,24,0,9,0,1,41,0,7,0,6,0,24,0,7,0,1,25,0,
8,0,24,0,5,0,1,25,0,9,0,24,0,3,0,1,25,0,0,0,24,0,0,0,1,25,0,1,
0,24,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_160 [71] =
   {	// blr string 

4,2,4,0,5,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,12,0,15,'K',11,
0,0,2,1,41,0,3,0,2,0,24,0,4,0,1,25,0,4,0,24,0,1,0,1,25,0,0,0,
24,0,2,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_167 [71] =
   {	// blr string 

4,2,4,0,5,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,12,0,15,'K',11,
0,0,2,1,41,0,3,0,2,0,24,0,4,0,1,25,0,4,0,24,0,1,0,1,25,0,0,0,
24,0,2,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_174 [71] =
   {	// blr string 

4,2,4,0,5,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,12,0,15,'K',11,
0,0,2,1,41,0,3,0,2,0,24,0,4,0,1,25,0,4,0,24,0,1,0,1,25,0,0,0,
24,0,2,0,1,25,0,1,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_181 [83] =
   {	// blr string 

4,2,4,0,8,0,9,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,12,
0,15,'K',31,0,0,2,1,41,0,4,0,3,0,24,0,3,0,1,41,0,0,0,5,0,24,0,
2,0,1,41,0,1,0,6,0,24,0,1,0,1,41,0,2,0,7,0,24,0,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_191 [46] =
   {	// blr string 

4,2,4,0,3,0,41,3,0,32,0,7,0,7,0,12,0,15,'K',1,0,0,2,1,41,0,0,
0,1,0,24,0,3,0,1,25,0,2,0,24,0,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_196 [119] =
   {	// blr string 

4,2,4,0,10,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,7,0,7,0,7,0,7,0,
7,0,7,0,12,0,15,'K',6,0,0,2,1,25,0,2,0,24,0,16,0,1,41,0,0,0,3,
0,24,0,13,0,1,25,0,4,0,24,0,5,0,1,41,0,6,0,5,0,24,0,4,0,1,25,
0,7,0,24,0,6,0,1,25,0,8,0,24,0,7,0,1,25,0,1,0,24,0,8,0,1,25,0,
9,0,24,0,3,0,255,255,76
   };	// end of blr string 


#define PAD(string, field) jrd_vtof ((char*)(string), field, sizeof (field))
const int FB_MAX_ACL_SIZE	= 4096;


static void add_index_set(Database*, bool, USHORT, USHORT);
static void add_relation_fields(thread_db*, USHORT);
static void add_security_to_sys_rel(thread_db*, const Firebird::MetaName&,
		const TEXT*, const UCHAR*, const SSHORT);
static int init2_helper(const int* fld, int n, jrd_rel* relation);
static void modify_relation_field(thread_db*, const int*, const int*, jrd_req**);
static void store_generator(thread_db*, const gen*, jrd_req**);
static void store_global_field(thread_db*, const gfld*, jrd_req**);
static void store_intlnames(thread_db*, Database*);
static void store_functions(thread_db*, Database*);
static void store_message(thread_db*, const trigger_msg*, jrd_req**);
static void store_relation_field(thread_db*, const int*, const int*, int, jrd_req**, bool);
static void store_trigger(thread_db*, const jrd_trg*, jrd_req**);


/* This is the table used in defining triggers; note that
   RDB$TRIGGER_0 was first changed to RDB$TRIGGER_7 to make it easier to
   upgrade a database to support field-level grant.  It has since been
   changed to RDB$TRIGGER_9 to handle SQL security on relations whose
   name is > 27 characters */

static const jrd_trg triggers[] =
{
	{ "RDB$TRIGGER_1", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger3), trigger3,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_8", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger2), trigger2,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_9", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger1), trigger1,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_2", (UCHAR) nam_trgs,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger4), trigger4,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_3", (UCHAR) nam_trgs,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger4), trigger4,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_4", (UCHAR) nam_relations,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger5), trigger5,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_5", (UCHAR) nam_relations,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger6), trigger6,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_6", (UCHAR) nam_gens,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger7), trigger7,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_26", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger26), trigger26,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_25", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger25),
		trigger25, 0, ODS_8_0 },
	{ "RDB$TRIGGER_10", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger10), trigger10,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_11", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger11),
		trigger11, 0, ODS_8_0 },
	{ "RDB$TRIGGER_12", (UCHAR) nam_ref_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger12), trigger12,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_13", (UCHAR) nam_ref_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger13),
		trigger13, 0, ODS_8_0 },
	{ "RDB$TRIGGER_14", (UCHAR) nam_chk_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger14),
		trigger14, 0, ODS_8_0 },
	{ "RDB$TRIGGER_15", (UCHAR) nam_chk_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger15), trigger15,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_16", (UCHAR) nam_chk_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger16),
		trigger16, 0, ODS_8_0 },
	{ "RDB$TRIGGER_17", (UCHAR) nam_i_segments,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger17), trigger17,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_18", (UCHAR) nam_i_segments,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger18),
		trigger18, 0, ODS_8_0 },
	{ "RDB$TRIGGER_19", (UCHAR) nam_indices,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger19), trigger19,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_20", (UCHAR) nam_indices,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger20),
		trigger20, 0, ODS_8_0 },
	{ "RDB$TRIGGER_21", (UCHAR) nam_trgs,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger21), trigger21,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_22", (UCHAR) nam_trgs,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger22),
		trigger22, 0, ODS_8_0 },
	{ "RDB$TRIGGER_23", (UCHAR) nam_r_fields,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger23), trigger23,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_24", (UCHAR) nam_r_fields,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger24),
		trigger24, 0, ODS_8_0 },
	{ "RDB$TRIGGER_27", (UCHAR) nam_r_fields,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger27),
		trigger27, 0, ODS_8_0 },
	{ "RDB$TRIGGER_28", (UCHAR) nam_procedures,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger28), trigger28,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_29", (UCHAR) nam_procedures,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger29),
		trigger29, 0, ODS_8_0 },
	{ "RDB$TRIGGER_30", (UCHAR) nam_exceptions,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger30), trigger30,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_31", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger31),
		trigger31, 0, ODS_8_1 },
	{ "RDB$TRIGGER_32", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger31), trigger31,
		0, ODS_8_1 },
	{ "RDB$TRIGGER_33", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger31), trigger31,
		0, ODS_8_1 },
	{ "RDB$TRIGGER_34", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger34),
		trigger34, TRG_ignore_perm, ODS_8_1 },
	{ "RDB$TRIGGER_35", (UCHAR) nam_chk_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger35),
		trigger35, TRG_ignore_perm, ODS_8_1 },
	{ "RDB$TRIGGER_36", (UCHAR) nam_fields,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger36),
		trigger36, 0, ODS_11_0 },
	{ 0, 0, 0, 0, 0, 0 }
};


/* this table is used in defining messages for system triggers */

static const trigger_msg trigger_messages[] =
{
	{ "RDB$TRIGGER_9", 0, "grant_obj_notfound", ODS_8_0 },
	{ "RDB$TRIGGER_9", 1, "grant_fld_notfound", ODS_8_0 },
	{ "RDB$TRIGGER_9", 2, "grant_nopriv", ODS_8_0 },
	{ "RDB$TRIGGER_9", 3, "nonsql_security_rel", ODS_8_0 },
	{ "RDB$TRIGGER_9", 4, "nonsql_security_fld", ODS_8_0 },
	{ "RDB$TRIGGER_9", 5, "grant_nopriv_on_base", ODS_8_0 },
	{ "RDB$TRIGGER_1", 0, "existing_priv_mod", ODS_8_0 },
	{ "RDB$TRIGGER_2", 0, "systrig_update", ODS_8_0 },
	{ "RDB$TRIGGER_3", 0, "systrig_update", ODS_8_0 },
	{ "RDB$TRIGGER_5", 0, "not_rel_owner", ODS_8_0 },
	{ "RDB$TRIGGER_24", 1, "cnstrnt_fld_rename", ODS_8_0 },
	{ "RDB$TRIGGER_23", 1, "cnstrnt_fld_del", ODS_8_0 },
	{ "RDB$TRIGGER_22", 1, "check_trig_update", ODS_8_0 },
	{ "RDB$TRIGGER_21", 1, "check_trig_del", ODS_8_0 },
	{ "RDB$TRIGGER_20", 1, "integ_index_mod", ODS_8_0 },
	{ "RDB$TRIGGER_20", 2, "integ_index_deactivate", ODS_8_0 },
	{ "RDB$TRIGGER_20", 3, "integ_deactivate_primary", ODS_8_0 },
	{ "RDB$TRIGGER_19", 1, "integ_index_del", ODS_8_0 },
	{ "RDB$TRIGGER_18", 1, "integ_index_seg_mod", ODS_8_0 },
	{ "RDB$TRIGGER_17", 1, "integ_index_seg_del", ODS_8_0 },
	{ "RDB$TRIGGER_15", 1, "check_cnstrnt_del", ODS_8_0 },
	{ "RDB$TRIGGER_14", 1, "check_cnstrnt_update", ODS_8_0 },
	{ "RDB$TRIGGER_13", 1, "ref_cnstrnt_update", ODS_8_0 },
	{ "RDB$TRIGGER_12", 1, "ref_cnstrnt_notfound", ODS_8_0 },
	{ "RDB$TRIGGER_12", 2, "foreign_key_notfound", ODS_8_0 },
	{ "RDB$TRIGGER_10", 1, "primary_key_ref", ODS_8_0 },
	{ "RDB$TRIGGER_10", 2, "primary_key_notnull", ODS_8_0 },
	{ "RDB$TRIGGER_25", 1, "rel_cnstrnt_update", ODS_8_0 },
	{ "RDB$TRIGGER_26", 1, "constaint_on_view", ODS_8_0 },
	{ "RDB$TRIGGER_26", 2, "invld_cnstrnt_type", ODS_8_0 },
	{ "RDB$TRIGGER_26", 3, "primary_key_exists", ODS_8_0 },
	{ "RDB$TRIGGER_31", 0, "no_write_user_priv", ODS_8_1 },
	{ "RDB$TRIGGER_32", 0, "no_write_user_priv", ODS_8_1 },
	{ "RDB$TRIGGER_33", 0, "no_write_user_priv", ODS_8_1 },
	{ "RDB$TRIGGER_24", 2, "integ_index_seg_mod", ODS_11_0 },
	{ "RDB$TRIGGER_36", 1, "integ_index_seg_mod", ODS_11_0 },
	{ 0, 0, 0, 0 }
};



void INI_format(const TEXT* owner, const TEXT* charset)
{
   struct {
          TEXT  jrd_162 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_163 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_164;	// gds__null_flag 
          SSHORT jrd_165;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_166;	// RDB$TYPE 
   } jrd_161;
   struct {
          TEXT  jrd_169 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_170 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_171;	// gds__null_flag 
          SSHORT jrd_172;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_173;	// RDB$TYPE 
   } jrd_168;
   struct {
          TEXT  jrd_176 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_177 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_178;	// gds__null_flag 
          SSHORT jrd_179;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_180;	// RDB$TYPE 
   } jrd_175;
   struct {
          bid  jrd_183;	// RDB$DESCRIPTION 
          TEXT  jrd_184 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_185 [32];	// RDB$ROLE_NAME 
          SSHORT jrd_186;	// gds__null_flag 
          SSHORT jrd_187;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_188;	// gds__null_flag 
          SSHORT jrd_189;	// gds__null_flag 
          SSHORT jrd_190;	// gds__null_flag 
   } jrd_182;
   struct {
          TEXT  jrd_193 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_194;	// gds__null_flag 
          SSHORT jrd_195;	// RDB$RELATION_ID 
   } jrd_192;
   struct {
          TEXT  jrd_198 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_199 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_200;	// RDB$RELATION_TYPE 
          SSHORT jrd_201;	// gds__null_flag 
          SSHORT jrd_202;	// RDB$DBKEY_LENGTH 
          SSHORT jrd_203;	// gds__null_flag 
          SSHORT jrd_204;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_205;	// RDB$FORMAT 
          SSHORT jrd_206;	// RDB$FIELD_ID 
          SSHORT jrd_207;	// RDB$RELATION_ID 
   } jrd_197;
/**************************************
 *
 *	I N I _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Initialize system relations in the database.
 *	The full complement of metadata should be
 *	stored here.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

/* Uppercase owner name */
	Firebird::MetaName string(owner ? owner : "");
	string.upper7();

/* Uppercase charset name */
	Firebird::MetaName string2(charset ? charset : "");
	string2.upper7();

	int n;
	const int* fld;

/* Make sure relations exist already */

	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		if (relfld[RFLD_R_TYPE] == rel_persistent)
		{
			DPM_create_relation(tdbb, MET_relation(tdbb, relfld[RFLD_R_ID]));
		}

		for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH)
			;
	}

/* Store RELATIONS and RELATION_FIELDS */

	jrd_req* handle1 = NULL;
	jrd_req* handle2 = NULL;
	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		for (n = 0, fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH)
		{
			if (!fld[RFLD_F_MINOR])
			{
				const int* pFld    = fld;
				const int* pRelFld = relfld;
				store_relation_field(tdbb, pFld, pRelFld, n, &handle2, true);
				n++;
			}
		}

		/*STORE(REQUEST_HANDLE handle1) X IN RDB$RELATIONS*/
		{
		
			/*X.RDB$RELATION_ID*/
			jrd_197.jrd_207 = relfld[RFLD_R_ID];
			PAD(names[relfld[RFLD_R_NAME]], /*X.RDB$RELATION_NAME*/
							jrd_197.jrd_199);
			/*X.RDB$FIELD_ID*/
			jrd_197.jrd_206 = n;
			/*X.RDB$FORMAT*/
			jrd_197.jrd_205 = 0;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_197.jrd_204 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_197.jrd_203 = FALSE;
			/*X.RDB$DBKEY_LENGTH*/
			jrd_197.jrd_202 = 8;
			/*X.RDB$OWNER_NAME.NULL*/
			jrd_197.jrd_201 = TRUE;
			/*X.RDB$RELATION_TYPE*/
			jrd_197.jrd_200 = relfld[RFLD_R_TYPE];

			if (string.length())
			{
				PAD(string.c_str(), /*X.RDB$OWNER_NAME*/
						    jrd_197.jrd_198);
				/*X.RDB$OWNER_NAME.NULL*/
				jrd_197.jrd_201 = FALSE;
			}

		/*END_STORE;*/
		if (!handle1)
		   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_196, sizeof(jrd_196), true);
		EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle1, 0, 80, (UCHAR*) &jrd_197);
		}
	}

	CMP_release(tdbb, handle1);
	CMP_release(tdbb, handle2);
	handle1 = handle2 = NULL;

/* Store global FIELDS */

	for (const gfld* gfield = gfields; gfield->gfld_name; gfield++)
	{
		store_global_field(tdbb, gfield, &handle1);
	}

	CMP_release(tdbb, handle1);
	handle1 = NULL;

	// Store DATABASE record

	/*STORE(REQUEST_HANDLE handle1) X IN RDB$DATABASE*/
	{
	
		/*X.RDB$RELATION_ID*/
		jrd_192.jrd_195 = (int) USER_DEF_REL_INIT_ID;
		/*X.RDB$CHARACTER_SET_NAME.NULL*/
		jrd_192.jrd_194 = TRUE;
		if (string2.length()) {
			PAD (string2.c_str(), /*X.RDB$CHARACTER_SET_NAME*/
					      jrd_192.jrd_193);
			/*X.RDB$CHARACTER_SET_NAME.NULL*/
			jrd_192.jrd_194 = FALSE;
		}
		else {
			PAD (DEFAULT_DB_CHARACTER_SET_NAME, /*X.RDB$CHARACTER_SET_NAME*/
							    jrd_192.jrd_193);
			/*X.RDB$CHARACTER_SET_NAME.NULL*/
			jrd_192.jrd_194 = FALSE;
		}
	/*END_STORE*/
	if (!handle1)
	   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_191, sizeof(jrd_191), true);
	EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle1, 0, 36, (UCHAR*) &jrd_192);
	}

	CMP_release(tdbb, handle1);
	handle1 = NULL;

	// Store ADMIN_ROLE record

	/*STORE(REQUEST_HANDLE handle1) X IN RDB$ROLES*/
	{
	
		PAD (ADMIN_ROLE, /*X.RDB$ROLE_NAME*/
				 jrd_182.jrd_185);
		/*X.RDB$ROLE_NAME.NULL*/
		jrd_182.jrd_190 = FALSE;
		if (string.length()) {
			PAD (string.c_str(), /*X.RDB$OWNER_NAME*/
					     jrd_182.jrd_184);
		}
		else {
			PAD (SYSDBA_USER_NAME, /*X.RDB$OWNER_NAME*/
					       jrd_182.jrd_184);
		}
		/*X.RDB$OWNER_NAME.NULL*/
		jrd_182.jrd_189 = FALSE;
		/*X.RDB$DESCRIPTION.NULL*/
		jrd_182.jrd_188 = TRUE;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_182.jrd_187 = ROLE_FLAG_DBO;	// Mark it as privileged role
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_182.jrd_186 = FALSE;
	/*END_STORE*/
	if (!handle1)
	   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_181, sizeof(jrd_181), true);
	EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle1, 0, 82, (UCHAR*) &jrd_182);
	}

	CMP_release(tdbb, handle1);
	handle1 = NULL;

	// Create indices for system relations
	add_index_set(dbb, false, 0, 0);

	// Create parameter types

	handle1 = NULL;

	for (const rtyp* type = types; type->rtyp_name; ++type)
	{
		// this STORE should be compatible with two below,
		// as they use the same compiled request
		/*STORE(REQUEST_HANDLE handle1) X IN RDB$TYPES*/
		{
		
			PAD(names[type->rtyp_field], /*X.RDB$FIELD_NAME*/
						     jrd_175.jrd_177);
			PAD(type->rtyp_name, /*X.RDB$TYPE_NAME*/
					     jrd_175.jrd_176);
			/*X.RDB$TYPE*/
			jrd_175.jrd_180 = type->rtyp_value;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_175.jrd_179 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_175.jrd_178 = FALSE;
		/*END_STORE;*/
		if (!handle1)
		   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_174, sizeof(jrd_174), true);
		EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle1, 0, 70, (UCHAR*) &jrd_175);
		}
	}

	for (const IntlManager::CharSetDefinition* charSet = IntlManager::defaultCharSets;
		 charSet->name; ++charSet)
	{
		// this STORE should be compatible with one above and below,
		// as they use the same compiled request
		/*STORE(REQUEST_HANDLE handle1) X IN RDB$TYPES*/
		{
		
			PAD(names[nam_charset_name], /*X.RDB$FIELD_NAME*/
						     jrd_168.jrd_170);
			PAD(charSet->name, /*X.RDB$TYPE_NAME*/
					   jrd_168.jrd_169);
			/*X.RDB$TYPE*/
			jrd_168.jrd_173 = charSet->id;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_168.jrd_172 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_168.jrd_171 = FALSE;
		/*END_STORE;*/
		if (!handle1)
		   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_167, sizeof(jrd_167), true);
		EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle1, 0, 70, (UCHAR*) &jrd_168);
		}
	}

	for (const IntlManager::CharSetAliasDefinition* alias = IntlManager::defaultCharSetAliases;
		alias->name; ++alias)
	{
		// this STORE should be compatible with two above,
		// as they use the same compiled request
		/*STORE(REQUEST_HANDLE handle1) X IN RDB$TYPES*/
		{
		
			PAD(names[nam_charset_name], /*X.RDB$FIELD_NAME*/
						     jrd_161.jrd_163);
			PAD(alias->name, /*X.RDB$TYPE_NAME*/
					 jrd_161.jrd_162);
			/*X.RDB$TYPE*/
			jrd_161.jrd_166 = alias->charSetId;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_161.jrd_165 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_161.jrd_164 = FALSE;
		/*END_STORE;*/
		if (!handle1)
		   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_160, sizeof(jrd_160), true);
		EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle1, 0, 70, (UCHAR*) &jrd_161);
		}
	}

	CMP_release(tdbb, handle1);

	// Store symbols for international character sets & collations
	store_intlnames(tdbb, dbb);

	// Create generators to be used by system triggers

	handle1 = NULL;
	for (const gen* generator = generators; generator->gen_name; generator++)
	{
		store_generator(tdbb, generator, &handle1);
	}
	CMP_release(tdbb, handle1);

	// Adjust the value of the hidden generator RDB$GENERATORS
	DPM_gen_id(tdbb, 0, true, FB_NELEM(generators) - 1);

/* store system-defined triggers */

	handle1 = NULL;
	for (const jrd_trg* trigger = triggers; trigger->trg_relation; ++trigger)
	{
		store_trigger(tdbb, trigger, &handle1);
	}
	CMP_release(tdbb, handle1);

/* store trigger messages to go with triggers */

	handle1 = NULL;
	for (const trigger_msg* message = trigger_messages; message->trigmsg_name;
		++message)
	{
		store_message(tdbb, message, &handle1);
	}
	CMP_release(tdbb, handle1);

// Store IUDFs declared automatically as system functions.
// CVC: Demonstration code to register IUDF automatically moved to functions.h

	store_functions(tdbb, dbb);

	DFW_perform_system_work(tdbb);

	add_relation_fields(tdbb, 0);

/*
====================================================================
==
== Add security on RDB$ROLES system table
==
======================================================================
*/

	UCHAR buffer[FB_MAX_ACL_SIZE];
	UCHAR* acl = buffer;
	*acl++ = ACL_version;
	*acl++ = ACL_id_list;
	*acl++ = id_person;

	USHORT length = string.length();
	if (length > MAX_UCHAR)
		length = MAX_UCHAR;

	*acl++ = (UCHAR)length;
	if (length)
	{
		const TEXT* p_1 = string.c_str();
		memcpy(acl, p_1, length);
		acl += length;
	}
	*acl++ = ACL_end;
	*acl++ = ACL_priv_list;
	*acl++ = priv_protect;
	*acl++ = priv_control;
	*acl++ = priv_delete;
	*acl++ = priv_write;
	*acl++ = priv_read;
	*acl++ = ACL_end;
	*acl++ = ACL_id_list;
	*acl++ = ACL_end;
	*acl++ = ACL_priv_list;
	*acl++ = priv_read;
	*acl++ = ACL_end;
	*acl++ = ACL_end;
	length = acl - buffer;

	add_security_to_sys_rel(tdbb, string, "RDB$ROLES", buffer, length);
	add_security_to_sys_rel(tdbb, string, "RDB$PAGES", buffer, length);
	// DFW writes here
	add_security_to_sys_rel(tdbb, string, "RDB$FORMATS", buffer, length);
}


USHORT INI_get_trig_flags(const TEXT* trig_name)
{
/*********************************************
 *
 *      I N I _ g e t _ t r i g _ f l a g s
 *
 *********************************************
 *
 * Functional description
 *      Return the trigger flags for a system trigger.
 *
 **************************************/
	for (const jrd_trg* trig = triggers; trig->trg_length > 0; trig++)
	{
		if (!strcmp(trig->trg_name, trig_name))
		{
			return (trig->trg_flags);
		}
	}

	return (0);
}


void INI_init(thread_db* tdbb)
{
/**************************************
 *
 *	I N I _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize in memory meta data.  Assume that all meta data
 *	fields exist in the database even if this is not the case.
 *	Do not fill in the format length or the address in each
 *	format descriptor.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	const int* fld;
	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		jrd_rel* relation = MET_relation(tdbb, relfld[RFLD_R_ID]);
		relation->rel_flags |= REL_system;
		relation->rel_flags |= MET_get_rel_flags_from_TYPE(relfld[RFLD_R_TYPE]);
		relation->rel_name = names[relfld[RFLD_R_NAME]];
		int n = 0;
		for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH)
		{
			n++;
		}

		/* Set a flag if their is a trigger on the relation.  Later we may
		   need to compile it. */

		for (const jrd_trg* trigger = triggers; trigger->trg_relation; trigger++)
		{
			if (relation->rel_name == names[trigger->trg_relation])
			{
				relation->rel_flags |= REL_sys_triggers;
				break;
			}
		}

		vec<jrd_fld*>* fields = vec<jrd_fld*>::newVector(*dbb->dbb_permanent, n);
		relation->rel_fields = fields;
		vec<jrd_fld*>::iterator itr = fields->begin();
		Format* format = Format::newFormat(*dbb->dbb_permanent, n);
		relation->rel_current_format = format;
		vec<Format*>* formats = vec<Format*>::newVector(*dbb->dbb_permanent, 1);
		relation->rel_formats = formats;
		(*formats)[0] = format;
		Format::fmt_desc_iterator desc = format->fmt_desc.begin();

		for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH, ++desc, ++itr)
		{
			const gfld* gfield = &gfields[fld[RFLD_F_ID]];
			desc->dsc_length = gfield->gfld_length;
			if (gfield->gfld_dtype == dtype_varying)
			{
				fb_assert(desc->dsc_length <= MAX_USHORT - sizeof(USHORT));
				desc->dsc_length += sizeof(USHORT);
			}
			desc->dsc_dtype = gfield->gfld_dtype;
			desc->dsc_sub_type = gfield->gfld_sub_type;
			if (desc->dsc_dtype == dtype_blob && desc->dsc_sub_type == isc_blob_text)
				desc->dsc_scale = CS_METADATA;	// blob charset

			jrd_fld* field = FB_NEW(*dbb->dbb_permanent) jrd_fld(*dbb->dbb_permanent);
			*itr = field;
			field->fld_name = names[fld[RFLD_F_NAME]];
		}
	}
}


void INI_init2(thread_db* tdbb)
{
/**************************************
 *
 *	I N I _ i n i t 2
 *
 **************************************
 *
 * Functional description
 *	Re-initialize in memory meta data.  Fill in
 *	format 0 based on the minor ODS version of
 *	the database when it was created.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	const USHORT major_version = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	if (ENCODE_ODS(major_version, minor_original) < ODS_9_0) {
		dbb->dbb_max_sys_rel = USER_REL_INIT_ID_ODS8 - 1;
	}
	else {
		dbb->dbb_max_sys_rel = USER_DEF_REL_INIT_ID - 1;
	}

	vec<jrd_rel*>* vector = dbb->dbb_relations;

	const int* fld;
	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		if (relfld[RFLD_R_ODS] > ENCODE_ODS(major_version, minor_original))
		{
		/*****************************************************
        **
        ** free the space allocated for RDB$ROLES
        **
        ******************************************************/
			const USHORT id = relfld[RFLD_R_ID];
			jrd_rel* relation = (*vector)[id];
			delete relation->rel_current_format;
			delete relation->rel_formats;
			delete relation->rel_fields;
			delete relation;
			(*vector)[id] = NULL;
			fld = relfld + RFLD_RPT;
			while (fld[RFLD_F_NAME])
			{
				fld += RFLD_F_LENGTH;
			}
		}
		else
		{
			jrd_rel* relation = MET_relation(tdbb, relfld[RFLD_R_ID]);
			Format* format = relation->rel_current_format;

			int n = 0;
			for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH)
			{
				/* If the ODS is less than 10, then remove all fields named
				 * RDB$FIELD_PRECISION and field RDB$CHARACTER_LENGTH from
				 * relation RDB$FUNCTION_ARGUMENTS , as they were not present
				 * in < 10 ODS
				 */
				if (fld[RFLD_F_NAME] == nam_f_precision ||
					(fld[RFLD_F_NAME] == nam_char_length && relfld[RFLD_R_NAME] == nam_args))
				{
					if (major_version >= ODS_VERSION10)
						n = init2_helper(fld, n, relation);
				}
				else if (fld[RFLD_F_NAME] == nam_statistics && relfld[RFLD_R_NAME] == nam_i_segments)
				{
					if (major_version >= ODS_VERSION11)
						n = init2_helper(fld, n, relation);
				}
				else if ((fld[RFLD_F_NAME] == nam_description || fld[RFLD_F_NAME] == nam_sys_flag) &&
					relfld[RFLD_R_NAME] == nam_roles)
				{
					if (major_version >= ODS_VERSION11)
						n = init2_helper(fld, n, relation);
				}
				else if (fld[RFLD_F_NAME] == nam_description && relfld[RFLD_R_NAME] == nam_gens)
				{
					if (major_version >= ODS_VERSION11)
						n = init2_helper(fld, n, relation);
				}
				else if ((fld[RFLD_F_NAME] == nam_base_collation_name ||
						fld[RFLD_F_NAME] == nam_specific_attr) &&
					relfld[RFLD_R_NAME] == nam_collations)
				{
					if (major_version >= ODS_VERSION11)
						n = init2_helper(fld, n, relation);
				}
				else if (fld[RFLD_F_NAME] == nam_r_type && relfld[RFLD_R_NAME] == nam_relations)
				{
					if (ENCODE_ODS(major_version, minor_original) >= ODS_11_1)
						n = init2_helper(fld, n, relation);
				}
				else if (fld[RFLD_F_NAME] == nam_valid_blr && relfld[RFLD_R_NAME] == nam_trgs)
				{
					if (ENCODE_ODS(major_version, minor_original) >= ODS_11_1)
						n = init2_helper(fld, n, relation);
				}
				else if ((fld[RFLD_F_NAME] == nam_prc_type || fld[RFLD_F_NAME] == nam_valid_blr) &&
					relfld[RFLD_R_NAME] == nam_procedures)
				{
					if (ENCODE_ODS(major_version, minor_original) >= ODS_11_1)
						n = init2_helper(fld, n, relation);
				}
				else if (fld[RFLD_F_NAME] == nam_debug_info &&
					(relfld[RFLD_R_NAME] == nam_trgs || relfld[RFLD_R_NAME] == nam_procedures))
				{
					if (ENCODE_ODS(major_version, minor_original) >= ODS_11_1)
						n = init2_helper(fld, n, relation);
				}
				else if ((fld[RFLD_F_NAME] == nam_default || fld[RFLD_F_NAME] == nam_d_source ||
						fld[RFLD_F_NAME] == nam_collate_id || fld[RFLD_F_NAME] == nam_null_flag ||
						fld[RFLD_F_NAME] == nam_prm_mechanism) &&
					relfld[RFLD_R_NAME] == nam_proc_parameters)
				{
					if (ENCODE_ODS(major_version, minor_original) >= ODS_11_1)
						n = init2_helper(fld, n, relation);
				}
				else if ((fld[RFLD_F_NAME] == nam_f_name || fld[RFLD_F_NAME] == nam_r_name) &&
					relfld[RFLD_R_NAME] == nam_proc_parameters)
				{
					if (ENCODE_ODS(major_version, minor_original) >= ODS_11_2)
						n = init2_helper(fld, n, relation);
				}
				else
					n = init2_helper(fld, n, relation);
			}

			relation->rel_fields->resize(n);
			format->fmt_count = n; // We are using less than the allocated members.
			format->fmt_length = (USHORT)FLAG_BYTES(n);
			Format::fmt_desc_iterator desc = format->fmt_desc.begin();
			for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH, ++desc)
			{
				if (n-- > 0)
				{
					format->fmt_length = (USHORT)MET_align(dbb, &(*desc), format->fmt_length);
					desc->dsc_address = (UCHAR*) (IPTR) format->fmt_length;

					// In ODS11 length of RDB$MESSAGE was enlarged from 80 to 1023 bytes
					if ((fld[RFLD_F_NAME] == nam_msg) && (major_version < ODS_VERSION11))
					{
						desc->dsc_length = 80 + sizeof(USHORT);
					}

					// In ODS prior to 11.2 all varchar columns were actually
					// two bytes shorter than defined in fields.h
					if (desc->dsc_dtype == dtype_varying &&
						ENCODE_ODS(major_version, minor_original) < ODS_11_2)
					{
						desc->dsc_length -= sizeof(USHORT);
					}

					// In ODS11.2 length of RDB$CONTEXT_NAME was enlarged from 31 to 255 bytes
					if ((fld[RFLD_F_NAME] == nam_context) &&
						ENCODE_ODS(major_version, minor_original) < ODS_11_2)
					{
						desc->dsc_length = 31;
					}

					format->fmt_length += desc->dsc_length;
				}
			}
		}
	}
}


static void add_index_set(Database*	dbb,
						  bool		update_ods,
						  USHORT	major_version,
						  USHORT	minor_version)
{
   struct {
          TEXT  jrd_145 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_146 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_147;	// RDB$FIELD_POSITION 
   } jrd_144;
   struct {
          TEXT  jrd_150 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_151 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_152;	// RDB$INDEX_ID 
          SSHORT jrd_153;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_154;	// gds__null_flag 
          SSHORT jrd_155;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_156;	// gds__null_flag 
          SSHORT jrd_157;	// RDB$INDEX_TYPE 
          SSHORT jrd_158;	// RDB$SEGMENT_COUNT 
          SSHORT jrd_159;	// RDB$UNIQUE_FLAG 
   } jrd_149;
/**************************************
 *
 *	a d d _ i n d e x _ s e t
 *
 **************************************
 *
 * Functional description
 *	Add system indices.  If update_ods is true we are performing
 *	an ODS update, and only add the indices marked as newer than
 *	ODS (major_version,minor_version).
 *
 **************************************/
	Firebird::MetaName string;
	index_desc idx;

	thread_db* tdbb = JRD_get_thread_data();
	jrd_req* handle1 = NULL;
	jrd_req* handle2 = NULL;

	for (int n = 0; n < SYSTEM_INDEX_COUNT; n++)
	{
		const ini_idx_t* index = &indices[n];

		/* For minor ODS updates, only add indices newer than on-disk ODS */
		if (update_ods &&
			((index->ini_idx_version_flag <= ENCODE_ODS(major_version, minor_version)) ||
				(index->ini_idx_version_flag > ODS_CURRENT_VERSION) ||
			 	(((USHORT) DECODE_ODS_MAJOR(index->ini_idx_version_flag)) != major_version)))
		{
			/* The DECODE_ODS_MAJOR() is used (in this case) to instruct the server
			   to perform updates for minor ODS versions only within a major ODS */
			continue;
		}

		/*STORE(REQUEST_HANDLE handle1) X IN RDB$INDICES*/
		{
		
			jrd_rel* relation = MET_relation(tdbb, index->ini_idx_relid);
			PAD(relation->rel_name.c_str(), /*X.RDB$RELATION_NAME*/
							jrd_149.jrd_151);
			string.printf("RDB$INDEX_%d", index->ini_idx_index_id);
			PAD(string.c_str(), /*X.RDB$INDEX_NAME*/
					    jrd_149.jrd_150);
			/*X.RDB$UNIQUE_FLAG*/
			jrd_149.jrd_159 = index->ini_idx_flags & idx_unique;
			/*X.RDB$SEGMENT_COUNT*/
			jrd_149.jrd_158 = index->ini_idx_segment_count;
			if (index->ini_idx_flags & idx_descending) {
				/*X.RDB$INDEX_TYPE.NULL*/
				jrd_149.jrd_156 = FALSE;
				/*X.RDB$INDEX_TYPE*/
				jrd_149.jrd_157 = 1;
			}
			else {
				/*X.RDB$INDEX_TYPE.NULL*/
				jrd_149.jrd_156 = TRUE;
			}
			/*X.RDB$SYSTEM_FLAG*/
			jrd_149.jrd_155 = 1;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_149.jrd_154 = FALSE;
			/*X.RDB$INDEX_INACTIVE*/
			jrd_149.jrd_153 = 0;

			/* Store each segment for the index */
			index_desc::idx_repeat* tail = idx.idx_rpt;
			for (USHORT position = 0; position < index->ini_idx_segment_count; position++, tail++)
			{
				const ini_idx_t::ini_idx_segment_t* segment = &index->ini_idx_segment[position];
				/*STORE(REQUEST_HANDLE handle2) Y IN RDB$INDEX_SEGMENTS*/
				{
				
					jrd_fld* field = (*relation->rel_fields)[segment->ini_idx_rfld_id];

					/*Y.RDB$FIELD_POSITION*/
					jrd_144.jrd_147 = position;
					PAD(/*X.RDB$INDEX_NAME*/
					    jrd_149.jrd_150, /*Y.RDB$INDEX_NAME*/
  jrd_144.jrd_146);
					PAD(field->fld_name.c_str(), /*Y.RDB$FIELD_NAME*/
								     jrd_144.jrd_145);
					tail->idx_field = segment->ini_idx_rfld_id;
					tail->idx_itype = segment->ini_idx_type;
				    tail->idx_selectivity = 0;
				/*END_STORE;*/
				if (!handle2)
				   handle2 = CMP_compile2 (tdbb, (UCHAR*) jrd_143, sizeof(jrd_143), true);
				EXE_start (tdbb, handle2, dbb->dbb_sys_trans);
				EXE_send (tdbb, handle2, 0, 66, (UCHAR*) &jrd_144);
				}
			}
			idx.idx_count = index->ini_idx_segment_count;
			idx.idx_flags = index->ini_idx_flags;
			SelectivityList selectivity(*tdbb->getDefaultPool());
			IDX_create_index(tdbb, relation, &idx, string.c_str(), NULL, dbb->dbb_sys_trans, selectivity);
			/*X.RDB$INDEX_ID*/
			jrd_149.jrd_152 = idx.idx_id + 1;
		/*END_STORE;*/
		if (!handle1)
		   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_148, sizeof(jrd_148), true);
		EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle1, 0, 80, (UCHAR*) &jrd_149);
		}

	}
	if (handle1) {
		CMP_release(tdbb, handle1);
	}
	if (handle2) {
		CMP_release(tdbb, handle2);
	}
}


static void add_relation_fields(thread_db* tdbb, USHORT minor_version)
{
/**************************************
 *
 *	a d d _ r e l a t i o n _ f i e l d s
 *
 **************************************
 *
 * Functional description
 *	Add any local fields which have a non-zero
 *	update number in the relfields table.  That
 *	is, those that have been added since the last
 *	ODS change.
 *
 **************************************/
	SET_TDBB(tdbb);
	//Database* dbb = tdbb->getDatabase();

/* add desired fields to system relations, forcing a new format version */

	jrd_req* s_handle = NULL;
	jrd_req* m_handle = NULL;
	const int* fld;
	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		int n = 0;
		for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; n++, fld += RFLD_F_LENGTH)
		{
			if (minor_version < fld[RFLD_F_MINOR] || minor_version < fld[RFLD_F_UPD_MINOR])
			{
				if (minor_version < fld[RFLD_F_MINOR])
				{
					store_relation_field(tdbb, fld, relfld, n, &s_handle, false);
				}
				else
				{
					modify_relation_field(tdbb, fld, relfld, &m_handle);
				}
				dsc desc;
				desc.dsc_dtype = dtype_text;
				INTL_ASSIGN_DSC(&desc, CS_METADATA, COLLATE_NONE);
				desc.dsc_address = (UCHAR*) names[relfld[RFLD_R_NAME]];
				desc.dsc_length = strlen((char*)desc.dsc_address);
				DFW_post_system_work(tdbb, dfw_update_format, &desc, 0);
			}
		}
	}

	if (s_handle) {
		CMP_release(tdbb, s_handle);
	}
	if (m_handle) {
		CMP_release(tdbb, m_handle);
	}

	DFW_perform_system_work(tdbb);
}


// The caller used an UCHAR* to store the acl, it was converted to TEXT* to
// be passed to this function, only to be converted to UCHAR* to be passed
// to BLB_put_segment. Therefore, "acl" was changed to UCHAR* as param.
static void add_security_to_sys_rel(thread_db*	tdbb,
									const Firebird::MetaName&	user_name,
									const TEXT*	rel_name,
									const UCHAR*	acl,
									const SSHORT	acl_length)
{
   struct {
          TEXT  jrd_110 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_111 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_112 [32];	// RDB$GRANTOR 
          TEXT  jrd_113 [32];	// RDB$USER 
          SSHORT jrd_114;	// RDB$OBJECT_TYPE 
          SSHORT jrd_115;	// RDB$USER_TYPE 
          SSHORT jrd_116;	// gds__null_flag 
          SSHORT jrd_117;	// RDB$GRANT_OPTION 
          TEXT  jrd_118 [7];	// RDB$PRIVILEGE 
   } jrd_109;
   struct {
          SSHORT jrd_134;	// gds__utility 
   } jrd_133;
   struct {
          TEXT  jrd_129 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_130 [32];	// RDB$DEFAULT_CLASS 
          SSHORT jrd_131;	// gds__null_flag 
          SSHORT jrd_132;	// gds__null_flag 
   } jrd_128;
   struct {
          TEXT  jrd_123 [32];	// RDB$DEFAULT_CLASS 
          TEXT  jrd_124 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_125;	// gds__utility 
          SSHORT jrd_126;	// gds__null_flag 
          SSHORT jrd_127;	// gds__null_flag 
   } jrd_122;
   struct {
          TEXT  jrd_121 [32];	// RDB$RELATION_NAME 
   } jrd_120;
   struct {
          bid  jrd_137;	// RDB$ACL 
          TEXT  jrd_138 [32];	// RDB$SECURITY_CLASS 
   } jrd_136;
   struct {
          bid  jrd_141;	// RDB$ACL 
          TEXT  jrd_142 [32];	// RDB$SECURITY_CLASS 
   } jrd_140;
/**************************************
 *
 *      a d d _ s e c u r i t y _ t o _ s y s _ r e l
 *
 **************************************
 *
 * Functional description
 *
 *      Add security to system relations. Only the owner of the
 *      database has SELECT/INSERT/UPDATE/DELETE privileges on
 *      any system relations. Any other users only has SELECT
 *      privilege.
 *
 **************************************/
	Firebird::MetaName security_class, default_class;

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

    bid blob_id_1;
	blb* blob = BLB_create(tdbb, dbb->dbb_sys_trans, &blob_id_1);
	BLB_put_segment(tdbb, blob, acl, acl_length);
	BLB_close(tdbb, blob);

    bid blob_id_2;
	blob = BLB_create(tdbb, dbb->dbb_sys_trans, &blob_id_2);
	BLB_put_segment(tdbb, blob, acl, acl_length);
	BLB_close(tdbb, blob);

	security_class.printf("%s%" SQUADFORMAT, SQL_SECCLASS_PREFIX,
						  DPM_gen_id(tdbb, MET_lookup_generator(tdbb, SQL_SECCLASS_GENERATOR), false, 1));

	default_class.printf("%s%" SQUADFORMAT, DEFAULT_CLASS,
						 DPM_gen_id(tdbb, MET_lookup_generator(tdbb, DEFAULT_CLASS), false, 1));

	jrd_req* handle1 = NULL;

	/*STORE(REQUEST_HANDLE handle1)
		CLS IN RDB$SECURITY_CLASSES*/
	{
	
		jrd_vtof(security_class.c_str(), /*CLS.RDB$SECURITY_CLASS*/
						 jrd_140.jrd_142, sizeof(/*CLS.RDB$SECURITY_CLASS*/
	 jrd_140.jrd_142));
		/*CLS.RDB$ACL*/
		jrd_140.jrd_141 = blob_id_1;
	/*END_STORE;*/
	if (!handle1)
	   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_139, sizeof(jrd_139), true);
	EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle1, 0, 40, (UCHAR*) &jrd_140);
	}

	CMP_release(tdbb, handle1);

	handle1 = NULL;

	/*STORE(REQUEST_HANDLE handle1)
		CLS IN RDB$SECURITY_CLASSES*/
	{
	
		jrd_vtof(default_class.c_str(), /*CLS.RDB$SECURITY_CLASS*/
						jrd_136.jrd_138, sizeof(/*CLS.RDB$SECURITY_CLASS*/
	 jrd_136.jrd_138));
		/*CLS.RDB$ACL*/
		jrd_136.jrd_137 = blob_id_2;
	/*END_STORE;*/
	if (!handle1)
	   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_135, sizeof(jrd_135), true);
	EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle1, 0, 40, (UCHAR*) &jrd_136);
	}

	CMP_release(tdbb, handle1);

	handle1 = NULL;

	/*FOR(REQUEST_HANDLE handle1) REL IN RDB$RELATIONS
		WITH REL.RDB$RELATION_NAME EQ rel_name*/
	{
	if (!handle1)
	   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_119, sizeof(jrd_119), true);
	gds__vtov ((const char*) rel_name, (char*) jrd_120.jrd_121, 32);
	EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle1, 0, 32, (UCHAR*) &jrd_120);
	while (1)
	   {
	   EXE_receive (tdbb, handle1, 1, 70, (UCHAR*) &jrd_122);
	   if (!jrd_122.jrd_125) break;
		/*MODIFY REL USING*/
		{
		
			/*REL.RDB$SECURITY_CLASS.NULL*/
			jrd_122.jrd_127 = FALSE;
			jrd_vtof(security_class.c_str(), /*REL.RDB$SECURITY_CLASS*/
							 jrd_122.jrd_124, sizeof(/*REL.RDB$SECURITY_CLASS*/
	 jrd_122.jrd_124));

			/*REL.RDB$DEFAULT_CLASS.NULL*/
			jrd_122.jrd_126 = FALSE;
			jrd_vtof(default_class.c_str(), /*REL.RDB$DEFAULT_CLASS*/
							jrd_122.jrd_123, sizeof(/*REL.RDB$DEFAULT_CLASS*/
	 jrd_122.jrd_123));
		/*END_MODIFY;*/
		gds__vtov((const char*) jrd_122.jrd_124, (char*) jrd_128.jrd_129, 32);
		gds__vtov((const char*) jrd_122.jrd_123, (char*) jrd_128.jrd_130, 32);
		jrd_128.jrd_131 = jrd_122.jrd_127;
		jrd_128.jrd_132 = jrd_122.jrd_126;
		EXE_send (tdbb, handle1, 2, 68, (UCHAR*) &jrd_128);
		}

	/*END_FOR;*/
	   EXE_send (tdbb, handle1, 3, 2, (UCHAR*) &jrd_133);
	   }
	}

	CMP_release(tdbb, handle1);

	handle1 = NULL;

	for (int cnt = 0; cnt < 6; cnt++)
	{
		/*STORE(REQUEST_HANDLE handle1) PRIV IN RDB$USER_PRIVILEGES*/
		{
		
			switch (cnt)
			{
			case 0:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_109.jrd_113, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_109.jrd_118[0] = 'S';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_109.jrd_117 = 1;
				break;
			case 1:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_109.jrd_113, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_109.jrd_118[0] = 'I';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_109.jrd_117 = 1;
				break;
			case 2:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_109.jrd_113, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_109.jrd_118[0] = 'U';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_109.jrd_117 = 1;
				break;
			case 3:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_109.jrd_113, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_109.jrd_118[0] = 'D';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_109.jrd_117 = 1;
				break;
			case 4:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_109.jrd_113, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_109.jrd_118[0] = 'R';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_109.jrd_117 = 1;
				break;
			default:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_109.jrd_113, "PUBLIC");
				/*PRIV.RDB$PRIVILEGE*/
				jrd_109.jrd_118[0] = 'S';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_109.jrd_117 = 0;
				break;
			}
			strcpy(/*PRIV.RDB$GRANTOR*/
			       jrd_109.jrd_112, user_name.c_str());
			/*PRIV.RDB$PRIVILEGE*/
			jrd_109.jrd_118[1] = 0;
			strcpy(/*PRIV.RDB$RELATION_NAME*/
			       jrd_109.jrd_111, rel_name);
			/*PRIV.RDB$FIELD_NAME.NULL*/
			jrd_109.jrd_116 = TRUE;
			/*PRIV.RDB$USER_TYPE*/
			jrd_109.jrd_115 = obj_user;
			/*PRIV.RDB$OBJECT_TYPE*/
			jrd_109.jrd_114 = obj_relation;
		/*END_STORE;*/
		if (!handle1)
		   handle1 = CMP_compile2 (tdbb, (UCHAR*) jrd_108, sizeof(jrd_108), true);
		EXE_start (tdbb, handle1, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle1, 0, 143, (UCHAR*) &jrd_109);
		}
	}

	CMP_release(tdbb, handle1);
}


static int init2_helper(const int* fld, int n, jrd_rel* relation)
{
	if (!fld[RFLD_F_MINOR])
	{
		++n;
		if (fld[RFLD_F_UPD_MINOR])
			relation->rel_flags |= REL_force_scan;
	}
	else
	{
		relation->rel_flags |= REL_force_scan;
	}
	return n;
}


static void modify_relation_field(thread_db*	tdbb,
								  const int*	fld,
								  const int*	relfld,
								  jrd_req**	handle)
{
   struct {
          SSHORT jrd_107;	// gds__utility 
   } jrd_106;
   struct {
          TEXT  jrd_104 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_105;	// RDB$UPDATE_FLAG 
   } jrd_103;
   struct {
          TEXT  jrd_100 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_101;	// gds__utility 
          SSHORT jrd_102;	// RDB$UPDATE_FLAG 
   } jrd_99;
   struct {
          TEXT  jrd_97 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_98 [32];	// RDB$RELATION_NAME 
   } jrd_96;
/**************************************
 *
 *	m o d i f y _ r e l a t i o n _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Modify a local field according to the
 *	passed information.  Note that the field id and
 *	field position do not change.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	/*FOR(REQUEST_HANDLE * handle) X IN RDB$RELATION_FIELDS WITH
		X.RDB$RELATION_NAME EQ names[relfld[RFLD_R_NAME]] AND
			X.RDB$FIELD_NAME EQ names[fld[RFLD_F_NAME]]*/
	{
	if (!*handle)
	   *handle = CMP_compile2 (tdbb, (UCHAR*) jrd_95, sizeof(jrd_95), true);
	gds__vtov ((const char*) names[fld[RFLD_F_NAME]], (char*) jrd_96.jrd_97, 32);
	gds__vtov ((const char*) names[relfld[RFLD_R_NAME]], (char*) jrd_96.jrd_98, 32);
	EXE_start (tdbb, *handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, *handle, 0, 64, (UCHAR*) &jrd_96);
	while (1)
	   {
	   EXE_receive (tdbb, *handle, 1, 36, (UCHAR*) &jrd_99);
	   if (!jrd_99.jrd_101) break;
		/*MODIFY X USING*/
		{
		
			const gfld* gfield = &gfields[fld[RFLD_F_UPD_ID]];
			PAD(names[gfield->gfld_name], /*X.RDB$FIELD_SOURCE*/
						      jrd_99.jrd_100);
			/*X.RDB$UPDATE_FLAG*/
			jrd_99.jrd_102 = fld[RFLD_F_UPDATE];
		/*END_MODIFY;*/
		gds__vtov((const char*) jrd_99.jrd_100, (char*) jrd_103.jrd_104, 32);
		jrd_103.jrd_105 = jrd_99.jrd_102;
		EXE_send (tdbb, *handle, 2, 34, (UCHAR*) &jrd_103);
		}
	/*END_FOR;*/
	   EXE_send (tdbb, *handle, 3, 2, (UCHAR*) &jrd_106);
	   }
	}
}


static void store_generator(thread_db* tdbb, const gen* generator, jrd_req** handle)
{
   struct {
          bid  jrd_89;	// RDB$DESCRIPTION 
          TEXT  jrd_90 [32];	// RDB$GENERATOR_NAME 
          SSHORT jrd_91;	// gds__null_flag 
          SSHORT jrd_92;	// gds__null_flag 
          SSHORT jrd_93;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_94;	// RDB$GENERATOR_ID 
   } jrd_88;
/**************************************
 *
 *	s t o r e _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *	Store the passed generator according to
 *	the information in the generator block.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	/*STORE(REQUEST_HANDLE * handle) X IN RDB$GENERATORS*/
	{
	
		PAD(generator->gen_name, /*X.RDB$GENERATOR_NAME*/
					 jrd_88.jrd_90);
		/*X.RDB$GENERATOR_ID*/
		jrd_88.jrd_94 = generator->gen_id;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_88.jrd_93 = RDB_system;
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_88.jrd_92 = FALSE;
		if (generator->gen_description)
		{
			blb* blob = BLB_create(tdbb, dbb->dbb_sys_trans, &/*X.RDB$DESCRIPTION*/
									  jrd_88.jrd_89);
			BLB_put_segment(tdbb,
							blob,
							reinterpret_cast<const UCHAR*>(generator->gen_description),
							strlen(generator->gen_description));
			BLB_close(tdbb, blob);
			/*X.RDB$DESCRIPTION.NULL*/
			jrd_88.jrd_91 = FALSE;
		}
		else
		{
			/*X.RDB$DESCRIPTION.NULL*/
			jrd_88.jrd_91 = TRUE;
		}

	/*END_STORE;*/
	if (!*handle)
	   *handle = CMP_compile2 (tdbb, (UCHAR*) jrd_87, sizeof(jrd_87), true);
	EXE_start (tdbb, *handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, *handle, 0, 48, (UCHAR*) &jrd_88);
	}
}


static void store_global_field(thread_db* tdbb, const gfld* gfield, jrd_req** handle)
{
   struct {
          bid  jrd_71;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_72 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_73;	// RDB$FIELD_TYPE 
          SSHORT jrd_74;	// gds__null_flag 
          SSHORT jrd_75;	// gds__null_flag 
          SSHORT jrd_76;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_77;	// gds__null_flag 
          SSHORT jrd_78;	// RDB$COLLATION_ID 
          SSHORT jrd_79;	// gds__null_flag 
          SSHORT jrd_80;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_81;	// gds__null_flag 
          SSHORT jrd_82;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_83;	// gds__null_flag 
          SSHORT jrd_84;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_85;	// RDB$FIELD_SCALE 
          SSHORT jrd_86;	// RDB$FIELD_LENGTH 
   } jrd_70;
/**************************************
 *
 *	s t o r e _ g l o b a l _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Store a global field according to the
 *	passed information.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	/*STORE(REQUEST_HANDLE * handle) X IN RDB$FIELDS*/
	{
	
		PAD(names[(USHORT)gfield->gfld_name], /*X.RDB$FIELD_NAME*/
						      jrd_70.jrd_72);
		/*X.RDB$FIELD_LENGTH*/
		jrd_70.jrd_86 = gfield->gfld_length;
		/*X.RDB$FIELD_SCALE*/
		jrd_70.jrd_85 = 0;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_70.jrd_84 = RDB_system;
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_70.jrd_83 = FALSE;
		/*X.RDB$FIELD_SUB_TYPE.NULL*/
		jrd_70.jrd_81 = TRUE;
		/*X.RDB$CHARACTER_SET_ID.NULL*/
		jrd_70.jrd_79 = TRUE;
		/*X.RDB$COLLATION_ID.NULL*/
		jrd_70.jrd_77 = TRUE;
		/*X.RDB$SEGMENT_LENGTH.NULL*/
		jrd_70.jrd_75 = TRUE;
		if (gfield->gfld_dflt_blr)
		{
			blb* blob = BLB_create(tdbb, dbb->dbb_sys_trans, &/*X.RDB$DEFAULT_VALUE*/
									  jrd_70.jrd_71);
			BLB_put_segment(tdbb,
							blob,
							gfield->gfld_dflt_blr,
							gfield->gfld_dflt_len);
			BLB_close(tdbb, blob);
			/*X.RDB$DEFAULT_VALUE.NULL*/
			jrd_70.jrd_74 = FALSE;
		}
		else
		{
			/*X.RDB$DEFAULT_VALUE.NULL*/
			jrd_70.jrd_74 = TRUE;
		}
		switch (gfield->gfld_dtype)
		{
		case dtype_timestamp:
			/*X.RDB$FIELD_TYPE*/
			jrd_70.jrd_73 = (int) blr_timestamp;
			break;

		case dtype_sql_time:
			/*X.RDB$FIELD_TYPE*/
			jrd_70.jrd_73 = (int) blr_sql_time;
			break;

		case dtype_sql_date:
			/*X.RDB$FIELD_TYPE*/
			jrd_70.jrd_73 = (int) blr_sql_date;
			break;

		case dtype_short:
		case dtype_long:
		case dtype_int64:
			if (gfield->gfld_dtype == dtype_short)
				/*X.RDB$FIELD_TYPE*/
				jrd_70.jrd_73 = (int) blr_short;
			else if (gfield->gfld_dtype == dtype_long)
				/*X.RDB$FIELD_TYPE*/
				jrd_70.jrd_73 = (int) blr_long;
			else
			{
				// Workaround to allow fld_counter domain
				// in dialect 1 databases
				if (dbb->dbb_flags & DBB_DB_SQL_dialect_3)
					/*X.RDB$FIELD_TYPE*/
					jrd_70.jrd_73 = (int) blr_int64;
				else
					/*X.RDB$FIELD_TYPE*/
					jrd_70.jrd_73 = (int) blr_double;
			}
			if ((gfield->gfld_sub_type == dsc_num_type_numeric) ||
				(gfield->gfld_sub_type == dsc_num_type_decimal))
			{
				/*X.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_70.jrd_81 = FALSE;
				/*X.RDB$FIELD_SUB_TYPE*/
				jrd_70.jrd_82 = gfield->gfld_sub_type;
			}
			break;

		case dtype_double:
			/*X.RDB$FIELD_TYPE*/
			jrd_70.jrd_73 = (int) blr_double;
			break;

		case dtype_text:
		case dtype_varying:
			if (gfield->gfld_dtype == dtype_text)
			{
				/*X.RDB$FIELD_TYPE*/
				jrd_70.jrd_73 = (int) blr_text;
			}
			else
			{
				/*X.RDB$FIELD_TYPE*/
				jrd_70.jrd_73 = (int) blr_varying;
			}
			switch (gfield->gfld_sub_type)
			{
			case dsc_text_type_metadata:
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_70.jrd_79 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_70.jrd_80 = CS_METADATA;
				/*X.RDB$COLLATION_ID.NULL*/
				jrd_70.jrd_77 = FALSE;
				/*X.RDB$COLLATION_ID*/
				jrd_70.jrd_78 = COLLATE_NONE;
				/*X.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_70.jrd_81 = FALSE;
				/*X.RDB$FIELD_SUB_TYPE*/
				jrd_70.jrd_82 = gfield->gfld_sub_type;
				break;
			case dsc_text_type_ascii:
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_70.jrd_79 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_70.jrd_80 = CS_ASCII;
				/*X.RDB$COLLATION_ID.NULL*/
				jrd_70.jrd_77 = FALSE;
				/*X.RDB$COLLATION_ID*/
				jrd_70.jrd_78 = COLLATE_NONE;
				/*X.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_70.jrd_81 = FALSE;
				/*X.RDB$FIELD_SUB_TYPE*/
				jrd_70.jrd_82 = gfield->gfld_sub_type;
				break;
			case dsc_text_type_fixed:
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_70.jrd_79 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_70.jrd_80 = CS_BINARY;
				/*X.RDB$COLLATION_ID.NULL*/
				jrd_70.jrd_77 = FALSE;
				/*X.RDB$COLLATION_ID*/
				jrd_70.jrd_78 = COLLATE_NONE;
				/*X.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_70.jrd_81 = FALSE;
				/*X.RDB$FIELD_SUB_TYPE*/
				jrd_70.jrd_82 = gfield->gfld_sub_type;
				break;
			default:
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_70.jrd_79 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_70.jrd_80 = CS_NONE;
				/*X.RDB$COLLATION_ID.NULL*/
				jrd_70.jrd_77 = FALSE;
				/*X.RDB$COLLATION_ID*/
				jrd_70.jrd_78 = COLLATE_NONE;
				break;
			}
			break;

		case dtype_blob:
			/*X.RDB$FIELD_TYPE*/
			jrd_70.jrd_73 = (int) blr_blob;
			/*X.RDB$FIELD_SUB_TYPE.NULL*/
			jrd_70.jrd_81 = FALSE;
			/*X.RDB$SEGMENT_LENGTH.NULL*/
			jrd_70.jrd_75 = FALSE;
			/*X.RDB$FIELD_SUB_TYPE*/
			jrd_70.jrd_82 = gfield->gfld_sub_type;
			/*X.RDB$SEGMENT_LENGTH*/
			jrd_70.jrd_76 = 80;
			if (gfield->gfld_sub_type == isc_blob_text)
			{
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_70.jrd_79 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_70.jrd_80 = CS_METADATA;
			}
			break;

		default:
			fb_assert(FALSE);
			break;
		}

		/* For the next ODS change, when we'll have not NULL sys fields
		X.RDB$NULL_FLAG.NULL = FALSE;
		X.RDB$NULL_FLAG = gfield->gfld_null_flag;
		*/

	/*END_STORE;*/
	if (!*handle)
	   *handle = CMP_compile2 (tdbb, (UCHAR*) jrd_69, sizeof(jrd_69), true);
	EXE_start (tdbb, *handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, *handle, 0, 68, (UCHAR*) &jrd_70);
	}
}


static void store_intlnames(thread_db* tdbb, Database* dbb)
{
   struct {
          bid  jrd_51;	// RDB$SPECIFIC_ATTRIBUTES 
          TEXT  jrd_52 [32];	// RDB$BASE_COLLATION_NAME 
          TEXT  jrd_53 [32];	// RDB$COLLATION_NAME 
          SSHORT jrd_54;	// gds__null_flag 
          SSHORT jrd_55;	// RDB$COLLATION_ATTRIBUTES 
          SSHORT jrd_56;	// gds__null_flag 
          SSHORT jrd_57;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_58;	// RDB$COLLATION_ID 
          SSHORT jrd_59;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_60;	// gds__null_flag 
   } jrd_50;
   struct {
          TEXT  jrd_63 [32];	// RDB$DEFAULT_COLLATE_NAME 
          TEXT  jrd_64 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_65;	// gds__null_flag 
          SSHORT jrd_66;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_67;	// RDB$BYTES_PER_CHARACTER 
          SSHORT jrd_68;	// RDB$CHARACTER_SET_ID 
   } jrd_62;
/**************************************
 *
 *	s t o r e _ i n t l n a m e s
 *
 **************************************
 *
 * Functional description
 *	Store symbolic names & information for international
 *	character sets & collations.
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_req* handle = NULL;

	for (const IntlManager::CharSetDefinition* charSet = IntlManager::defaultCharSets;
		 charSet->name; ++charSet)
	{
		/*STORE(REQUEST_HANDLE handle) X IN RDB$CHARACTER_SETS USING*/
		{
		
			PAD(charSet->name, /*X.RDB$CHARACTER_SET_NAME*/
					   jrd_62.jrd_64);
			PAD(charSet->name, /*X.RDB$DEFAULT_COLLATE_NAME*/
					   jrd_62.jrd_63);
			/*X.RDB$CHARACTER_SET_ID*/
			jrd_62.jrd_68 = charSet->id;
			/*X.RDB$BYTES_PER_CHARACTER*/
			jrd_62.jrd_67 = charSet->maxBytes;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_62.jrd_66 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_62.jrd_65 = FALSE;
		/*END_STORE;*/
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_61, sizeof(jrd_61), true);
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle, 0, 72, (UCHAR*) &jrd_62);
		}
	}

	CMP_release(tdbb, handle);
	handle = NULL;

	for (const IntlManager::CollationDefinition* collation = IntlManager::defaultCollations;
		collation->name; ++collation)
	{
		/*STORE(REQUEST_HANDLE handle) X IN RDB$COLLATIONS USING*/
		{
		
			PAD(collation->name, /*X.RDB$COLLATION_NAME*/
					     jrd_50.jrd_53);

			if (collation->baseName)
			{
				/*X.RDB$BASE_COLLATION_NAME.NULL*/
				jrd_50.jrd_60 = false;
				PAD(collation->baseName, /*X.RDB$BASE_COLLATION_NAME*/
							 jrd_50.jrd_52);
			}
			else
				/*X.RDB$BASE_COLLATION_NAME.NULL*/
				jrd_50.jrd_60 = true;

			/*X.RDB$CHARACTER_SET_ID*/
			jrd_50.jrd_59 = collation->charSetId;
			/*X.RDB$COLLATION_ID*/
			jrd_50.jrd_58 = collation->collationId;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_50.jrd_57 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_50.jrd_56 = FALSE;
			/*X.RDB$COLLATION_ATTRIBUTES*/
			jrd_50.jrd_55 = collation->attributes;

			if (collation->specificAttributes)
			{
				blb* blob = BLB_create(tdbb, dbb->dbb_sys_trans, &/*X.RDB$SPECIFIC_ATTRIBUTES*/
										  jrd_50.jrd_51);
				BLB_put_segment(tdbb,
								blob,
								reinterpret_cast<const UCHAR*>(collation->specificAttributes),
								strlen(collation->specificAttributes));
				BLB_close(tdbb, blob);
				/*X.RDB$SPECIFIC_ATTRIBUTES.NULL*/
				jrd_50.jrd_54 = FALSE;
			}
			else
				/*X.RDB$SPECIFIC_ATTRIBUTES.NULL*/
				jrd_50.jrd_54 = TRUE;

		/*END_STORE;*/
		if (!handle)
		   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_49, sizeof(jrd_49), true);
		EXE_start (tdbb, handle, dbb->dbb_sys_trans);
		EXE_send (tdbb, handle, 0, 86, (UCHAR*) &jrd_50);
		}
	}

	CMP_release(tdbb, handle);
	handle = NULL;
}


static void store_function_argument(thread_db* tdbb, Database* dbb, jrd_req*& handle,
	const char* function_name, SLONG argument_position, SLONG mechanism, SLONG type, SLONG scale,
	SLONG length, SLONG sub_type, SLONG charset, SLONG precision, SLONG char_length)
{
   struct {
          TEXT  jrd_38 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_39;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_40;	// RDB$FIELD_PRECISION 
          SSHORT jrd_41;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_42;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_43;	// RDB$FIELD_LENGTH 
          SSHORT jrd_44;	// RDB$FIELD_SCALE 
          SSHORT jrd_45;	// RDB$FIELD_TYPE 
          SSHORT jrd_46;	// RDB$MECHANISM 
          SSHORT jrd_47;	// RDB$ARGUMENT_POSITION 
          SSHORT jrd_48;	// gds__null_flag 
   } jrd_37;
	/*STORE(REQUEST_HANDLE handle) FA IN RDB$FUNCTION_ARGUMENTS*/
	{
	
		PAD(function_name, /*FA.RDB$FUNCTION_NAME*/
				   jrd_37.jrd_38);
		/*FA.RDB$FUNCTION_NAME.NULL*/
		jrd_37.jrd_48          = FALSE;
		/*FA.RDB$ARGUMENT_POSITION*/
		jrd_37.jrd_47           = argument_position;
		/*FA.RDB$MECHANISM*/
		jrd_37.jrd_46                   = mechanism;
		/*FA.RDB$FIELD_TYPE*/
		jrd_37.jrd_45                  = type;
		/*FA.RDB$FIELD_SCALE*/
		jrd_37.jrd_44                 = scale;
		/*FA.RDB$FIELD_LENGTH*/
		jrd_37.jrd_43                = length;
		/*FA.RDB$FIELD_SUB_TYPE*/
		jrd_37.jrd_42              = sub_type;
		/*FA.RDB$CHARACTER_SET_ID*/
		jrd_37.jrd_41            = charset;
		/*FA.RDB$FIELD_PRECISION*/
		jrd_37.jrd_40             = precision;
		/*FA.RDB$CHARACTER_LENGTH*/
		jrd_37.jrd_39            = char_length;
	/*END_STORE;*/
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_36, sizeof(jrd_36), true);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 52, (UCHAR*) &jrd_37);
	}
}


static void store_function(thread_db* tdbb, Database* dbb, jrd_req*& handle,
	const char* function_name, const char* module, const char* entrypoint, SLONG argument)
{
   struct {
          TEXT  jrd_27 [32];	// RDB$ENTRYPOINT 
          TEXT  jrd_28 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_29 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_30;	// gds__null_flag 
          SSHORT jrd_31;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_32;	// RDB$RETURN_ARGUMENT 
          SSHORT jrd_33;	// gds__null_flag 
          SSHORT jrd_34;	// gds__null_flag 
          SSHORT jrd_35;	// gds__null_flag 
   } jrd_26;
	/*STORE(REQUEST_HANDLE handle) F IN RDB$FUNCTIONS*/
	{
	
		PAD(function_name, /*F.RDB$FUNCTION_NAME*/
				   jrd_26.jrd_29);
		/*F.RDB$FUNCTION_NAME.NULL*/
		jrd_26.jrd_35          = FALSE;
		//F.RDB$FUNCTION_TYPE               <null>
		//F.RDB$QUERY_NAME                  <null>
		//F.RDB$DESCRIPTION                 <null>
		strcpy(/*F.RDB$MODULE_NAME*/
		       jrd_26.jrd_28, module);
		/*F.RDB$MODULE_NAME.NULL*/
		jrd_26.jrd_34            = FALSE;
		PAD(entrypoint, /*F.RDB$ENTRYPOINT*/
				jrd_26.jrd_27);
		/*F.RDB$ENTRYPOINT.NULL*/
		jrd_26.jrd_33             = FALSE;
		/*F.RDB$RETURN_ARGUMENT*/
		jrd_26.jrd_32             = argument;
		/*F.RDB$SYSTEM_FLAG*/
		jrd_26.jrd_31                 = RDB_system;
		/*F.RDB$SYSTEM_FLAG.NULL*/
		jrd_26.jrd_30            = FALSE;
	/*END_STORE;*/
	if (!handle)
	   handle = CMP_compile2 (tdbb, (UCHAR*) jrd_25, sizeof(jrd_25), true);
	EXE_start (tdbb, handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, handle, 0, 332, (UCHAR*) &jrd_26);
	}
}


static void store_functions(thread_db* tdbb, Database* dbb)
{
/**************************************
 *
 *	s t o r e _ f u n c t i o n s
 *
 **************************************
 *
 * Functional description
 *	Store built-in functions and their arguments
 *
 **************************************/
	SET_TDBB(tdbb);

	jrd_req *fun_handle = NULL, *arg_handle = NULL;
	const char *function_name;
	SLONG argument_position;

#define FUNCTION(ROUTINE, FUNCTION_NAME, MODULE_NAME, ENTRYPOINT, RET_ARG) \
	function_name = FUNCTION_NAME; \
	argument_position = RET_ARG ? 1 : 0; \
	store_function(tdbb, dbb, fun_handle, FUNCTION_NAME, MODULE_NAME, ENTRYPOINT, RET_ARG);
#define END_FUNCTION
#define FUNCTION_ARGUMENT(MECHANISM, TYPE, SCALE, LENGTH, SUB_TYPE, CHARSET, PRECISION, CHAR_LENGTH) \
	store_function_argument(tdbb, dbb, arg_handle, function_name, argument_position, \
		MECHANISM, TYPE, SCALE, LENGTH, SUB_TYPE, CHARSET, PRECISION, CHAR_LENGTH);	\
	argument_position++;

#include "../jrd/functions.h"

	CMP_release(tdbb, fun_handle);
	CMP_release(tdbb, arg_handle);
}


static void store_message(thread_db* tdbb, const trigger_msg* message, jrd_req** handle)
{
   struct {
          TEXT  jrd_22 [1024];	// RDB$MESSAGE 
          TEXT  jrd_23 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_24;	// RDB$MESSAGE_NUMBER 
   } jrd_21;
/**************************************
 *
 *	s t o r e _ m e s s a g e
 *
 **************************************
 *
 * Functional description
 *	Store system trigger messages.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* store the trigger */

	/*STORE(REQUEST_HANDLE * handle) X IN RDB$TRIGGER_MESSAGES*/
	{
	
		PAD(message->trigmsg_name, /*X.RDB$TRIGGER_NAME*/
					   jrd_21.jrd_23);
		/*X.RDB$MESSAGE_NUMBER*/
		jrd_21.jrd_24 = message->trigmsg_number;
		PAD(message->trigmsg_text, /*X.RDB$MESSAGE*/
					   jrd_21.jrd_22);
	/*END_STORE;*/
	if (!*handle)
	   *handle = CMP_compile2 (tdbb, (UCHAR*) jrd_20, sizeof(jrd_20), true);
	EXE_start (tdbb, *handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, *handle, 0, 1058, (UCHAR*) &jrd_21);
	}
}


static void store_relation_field(thread_db*		tdbb,
								 const int*		fld,
								 const int*		relfld,
								 int			field_id,
								 jrd_req**		handle,
								 bool			fmt0_flag)
{
   struct {
          TEXT  jrd_12 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_13 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_14 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_15;	// RDB$UPDATE_FLAG 
          SSHORT jrd_16;	// gds__null_flag 
          SSHORT jrd_17;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_18;	// RDB$FIELD_ID 
          SSHORT jrd_19;	// RDB$FIELD_POSITION 
   } jrd_11;
/**************************************
 *
 *	s t o r e _ r e l a t i o n _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Store a local field according to the
 *	passed information.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	/*STORE(REQUEST_HANDLE * handle) X IN RDB$RELATION_FIELDS*/
	{
	
		const gfld* gfield = (fld[RFLD_F_UPD_MINOR] && !fmt0_flag) ?
			&gfields[fld[RFLD_F_UPD_ID]] : &gfields[fld[RFLD_F_ID]];
		PAD(names[relfld[RFLD_R_NAME]], /*X.RDB$RELATION_NAME*/
						jrd_11.jrd_14);
		PAD(names[fld[RFLD_F_NAME]], /*X.RDB$FIELD_NAME*/
					     jrd_11.jrd_13);
		PAD(names[gfield->gfld_name], /*X.RDB$FIELD_SOURCE*/
					      jrd_11.jrd_12);
		/*X.RDB$FIELD_POSITION*/
		jrd_11.jrd_19 = field_id;
		/*X.RDB$FIELD_ID*/
		jrd_11.jrd_18 = field_id;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_11.jrd_17 = RDB_system;
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_11.jrd_16 = FALSE;
		/*X.RDB$UPDATE_FLAG*/
		jrd_11.jrd_15 = fld[RFLD_F_UPDATE];
	/*END_STORE;*/
	if (!*handle)
	   *handle = CMP_compile2 (tdbb, (UCHAR*) jrd_10, sizeof(jrd_10), true);
	EXE_start (tdbb, *handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, *handle, 0, 106, (UCHAR*) &jrd_11);
	}
}


static void store_trigger(thread_db* tdbb, const jrd_trg* trigger, jrd_req** handle)
{
   struct {
          bid  jrd_2;	// RDB$TRIGGER_BLR 
          TEXT  jrd_3 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_4 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_5;	// RDB$FLAGS 
          SSHORT jrd_6;	// RDB$TRIGGER_TYPE 
          SSHORT jrd_7;	// gds__null_flag 
          SSHORT jrd_8;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_9;	// RDB$TRIGGER_SEQUENCE 
   } jrd_1;
/**************************************
 *
 *	s t o r e _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Store the trigger according to the
 *	information in the trigger block.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

/* indicate that the relation format needs revising */
	dsc desc;
	desc.dsc_dtype = dtype_text;
	INTL_ASSIGN_DSC(&desc, CS_METADATA, COLLATE_NONE);
	desc.dsc_address = (UCHAR*) names[trigger->trg_relation];
	desc.dsc_length = strlen((char*)desc.dsc_address);
	DFW_post_system_work(tdbb, dfw_update_format, &desc, 0);

/* store the trigger */

	/*STORE(REQUEST_HANDLE * handle) X IN RDB$TRIGGERS*/
	{
	
		PAD(trigger->trg_name, /*X.RDB$TRIGGER_NAME*/
				       jrd_1.jrd_4);
		PAD(names[trigger->trg_relation], /*X.RDB$RELATION_NAME*/
						  jrd_1.jrd_3);
		/*X.RDB$TRIGGER_SEQUENCE*/
		jrd_1.jrd_9 = 0;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_1.jrd_8 = RDB_system;
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_1.jrd_7 = FALSE;
		/*X.RDB$TRIGGER_TYPE*/
		jrd_1.jrd_6 = trigger->trg_type;
		/*X.RDB$FLAGS*/
		jrd_1.jrd_5 = trigger->trg_flags;
		blb* blob = BLB_create(tdbb, dbb->dbb_sys_trans, &/*X.RDB$TRIGGER_BLR*/
								  jrd_1.jrd_2);
		BLB_put_segment(tdbb, blob, trigger->trg_blr, trigger->trg_length);
		BLB_close(tdbb, blob);
	/*END_STORE;*/
	if (!*handle)
	   *handle = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
	EXE_start (tdbb, *handle, dbb->dbb_sys_trans);
	EXE_send (tdbb, *handle, 0, 82, (UCHAR*) &jrd_1);
	}
}
