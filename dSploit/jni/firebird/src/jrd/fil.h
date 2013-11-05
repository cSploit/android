/*
 *	PROGRAM:	JRD temp file space list
 *	MODULE:		fil.h
 *	DESCRIPTION:	Lists of directories for temporary files or UDFs
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
 * 26-Sept-2001 Paul Beach - External File Directory Config. Parameter
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef JRD_FIL_H
#define JRD_FIL_H

/* Defined the directory list structures. */

/* Temporary workfile directory list. */

struct dir_list
{
	SLONG dls_header;           // Never referenced directly.
	dir_list* dls_next;
	ULONG dls_size;				/* Maximum size in the directory */
	ULONG dls_inuse;			/* Occupied space in the directory */
	TEXT dls_directory[2];		/* Directory name */
};

struct mutexed_dir_list
{
	dir_list* mdls_dls;			/* Pointer to the directory list */
	Firebird::Mutex mdls_mutex;	/* Mutex for directory list. Must
								   be locked before list operations */
	mutexed_dir_list()
		: mdls_dls(NULL)
	{}
};

/* external function directory list */
/* OBSOLETE.
struct function_dir_list
{
	function_dir_list* fdls_next;
	TEXT fdls_directory[1];
};
*/

/* external file directory list */
/* OBSOLETE
struct extfile_dir_list
{
    extfile_dir_list* edls_next;
    TEXT edls_directory[1];
};
*/

#endif // JRD_FIL_H

