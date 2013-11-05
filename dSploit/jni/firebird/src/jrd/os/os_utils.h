/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


// =====================================
// File functions

#ifndef INCLUDE_OS_FILE_UTILS_H
#define INCLUDE_OS_FILE_UTILS_H

#ifdef SFIO
#include <stdio.h>
#endif

#include "../common/classes/fb_string.h"

namespace os_utils
{

	SLONG get_user_group_id(const TEXT* user_group_name);
	SLONG get_user_id(const TEXT* user_name);
	bool get_user_home(int user_id, Firebird::PathName& homeDir);

	void createLockDirectory(const char* pathname);
	int openCreateSharedFile(const char* pathname, int flags);
	bool touchFile(const char* pathname);

} // namespace os_utils

#endif // INCLUDE_OS_FILE_UTILS_H

