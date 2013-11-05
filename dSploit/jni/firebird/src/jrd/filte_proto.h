/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		filte_proto.h
 *	DESCRIPTION:	Prototype Header file for filters.cpp
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

#ifndef JRD_FILTE_PROTO_H
#define JRD_FILTE_PROTO_H

ISC_STATUS filter_acl(USHORT, Jrd::BlobControl*);
ISC_STATUS filter_blr(USHORT, Jrd::BlobControl*);
ISC_STATUS filter_format(USHORT, Jrd::BlobControl*);
ISC_STATUS filter_runtime(USHORT, Jrd::BlobControl*);
ISC_STATUS filter_text(USHORT, Jrd::BlobControl*);
ISC_STATUS filter_transliterate_text(USHORT, Jrd::BlobControl*);
ISC_STATUS filter_trans(USHORT, Jrd::BlobControl*);

#endif // JRD_FILTE_PROTO_H

