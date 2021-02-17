/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		gpre_meta.h
 *	DESCRIPTION:	Prototype header file for gpre_meta.epp and gpre_meta_boot.cpp
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

#ifndef GPRE_GPRE_META_H
#define GPRE_GPRE_META_H

gpre_fld*	MET_context_field(gpre_ctx*, const char*);
bool		MET_database(gpre_dbb*, bool);
bool		MET_domain_lookup(gpre_req*, gpre_fld*, const char*);
gpre_fld*	MET_field(gpre_rel*, const char*);
gpre_nod*	MET_fields(gpre_ctx*);
void		MET_fini(gpre_dbb*);
const SCHAR*		MET_generator(const TEXT*, const gpre_dbb*);
bool		MET_get_column_default(const gpre_rel*, const TEXT*, TEXT*, USHORT);
bool		MET_get_domain_default(gpre_dbb*, const TEXT*, TEXT*, USHORT);
USHORT		MET_get_dtype(USHORT, USHORT, USHORT*);
gpre_lls*	MET_get_primary_key(gpre_dbb*, const TEXT*);
gpre_prc*	MET_get_procedure(gpre_dbb*, const TEXT*, const TEXT*);
gpre_rel*	MET_get_relation(gpre_dbb*, const TEXT*, const TEXT*);
intlsym*	MET_get_text_subtype(SSHORT);
udf*		MET_get_udf(gpre_dbb*, const TEXT*);
gpre_rel*	MET_get_view_relation(gpre_req*, const char*, const char*, USHORT);
gpre_index*	MET_index(gpre_dbb*, TEXT*);
void		MET_load_hash_table(gpre_dbb*);
gpre_fld*	MET_make_field(const SCHAR*, SSHORT, SSHORT, bool);
gpre_index*	MET_make_index(const SCHAR*);
gpre_rel*	MET_make_relation(const SCHAR*);
bool		MET_type(gpre_fld*, const TEXT*, SSHORT*);
bool		MET_trigger_exists(gpre_dbb*, const TEXT*);

#endif // GPRE_GPRE_META_H

