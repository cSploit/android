/*
 *	PROGRAM:	Interactive SQL utility
 *	MODULE:		show_proto.h
 *	DESCRIPTION:	Prototype header file for show.epp
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

#ifndef ISQL_SHOW_PROTO_H
#define ISQL_SHOW_PROTO_H

#include "../common/classes/fb_string.h"
#include <firebird/Provider.h>

void	SHOW_comments(bool force);
bool	SHOW_dbb_parameters (Firebird::IAttachment*, SCHAR*, const UCHAR*, unsigned, bool, const char*);
processing_state	SHOW_grants (const SCHAR*, const SCHAR*, USHORT);
processing_state	SHOW_grants2 (const SCHAR*, const SCHAR*, USHORT, const TEXT*, bool);
void	SHOW_grant_roles (const SCHAR*, bool*);
void	SHOW_grant_roles2 (const SCHAR*, bool*, const TEXT*, bool);
void	SHOW_print_metadata_text_blob(FILE*, ISC_QUAD*, bool escape_squote = false);
processing_state	SHOW_metadata(const SCHAR* const*, SCHAR**);
void	SHOW_read_owner();
const Firebird::string SHOW_trigger_action(SINT64);
processing_state	SHOW_maps(bool extract, const SCHAR* map_name);

#endif // ISQL_SHOW_PROTO_H
