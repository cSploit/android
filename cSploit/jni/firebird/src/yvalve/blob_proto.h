/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		blob_proto.h
 *	DESCRIPTION:	Prototype Header file for blob.epp
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

#ifndef DSQL_BLOB_PROTO_H
#define DSQL_BLOB_PROTO_H

ISC_STATUS API_ROUTINE isc_blob_gen_bpb(ISC_STATUS*, const ISC_BLOB_DESC*,
										const ISC_BLOB_DESC*, USHORT, UCHAR*, USHORT*);
ISC_STATUS API_ROUTINE isc_blob_lookup_desc(ISC_STATUS*, void**, void**,
											const UCHAR*, const UCHAR*, ISC_BLOB_DESC*, UCHAR*);
ISC_STATUS API_ROUTINE isc_blob_set_desc(ISC_STATUS*, const UCHAR*, const UCHAR*,
										 SSHORT, SSHORT, SSHORT, ISC_BLOB_DESC*);

// Only declared in ibase.h:
//void API_ROUTINE isc_blob_default_desc(ISC_BLOB_DESC* desc,
//									   const UCHAR*, const UCHAR*);


#endif // DSQL_BLOB_PROTO_H

