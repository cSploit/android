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
 *  The Original Code was created by Mark O'Donohue
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 Mark O'Donohue <skywalker@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  25 May 2003 - Nickolay Samofatov: Fix several bugs that prevented
 *    compatibility with Kylix
 *
 */

#include "firebird.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "fb_types.h"
#include "../common/classes/fb_string.h"
#include "../common/config/os/config_root.h"
#include "../common/os/path_utils.h"
#include "binreloc.h"

typedef Firebird::PathName string;

/******************************************************************************
 *
 *	Platform-specific root locator
 */

void ConfigRoot::osConfigRoot()
{
	// Try to use value set at configure time
	if (FB_CONFDIR[0])
	{
		root_dir = FB_CONFDIR;
		if (root_dir[root_dir.length() - 1] != PathUtils::dir_sep)
		{
			root_dir += PathUtils::dir_sep;
		}
		return;
	}

    // As a last resort get it from the default install directory
	root_dir = install_dir + PathUtils::dir_sep;
}


void ConfigRoot::osConfigInstallDir()
{
#ifdef ENABLE_BINRELOC
	BrInitError brError;
	if (br_init_lib(&brError))
	{
		char* temp = br_find_exe_dir(NULL);
		if (temp)
		{
			string dummy;
			PathUtils::splitLastComponent(install_dir, dummy, temp);
			free(temp);
			return;
		}
	}
#endif

    // As a last resort get it from the default install directory
	install_dir = string(FB_PREFIX);
}
