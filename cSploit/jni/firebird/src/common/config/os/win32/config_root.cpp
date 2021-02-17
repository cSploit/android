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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  Adriano dos Santos Fernandes
 */

#include "firebird.h"

#include <windows.h>

#include "fb_types.h"
#include "../../../../common/classes/fb_string.h"
#include "../../../../common/dllinst.h"
#include "../../../../common/config/os/config_root.h"

using Firebird::PathName;


/******************************************************************************
 *
 *	Platform-specific root locator
 */
namespace {

bool getPathFromHInstance(PathName& root)
{
	const HINSTANCE hDllInst = Firebird::hDllInst;
	if (!hDllInst)
	{
		return false;
	}

	char* filename = root.getBuffer(MAX_PATH);
	GetModuleFileName(hDllInst, filename, MAX_PATH);

	root.recalculate_length();

	return root.hasData();
}

} // namespace


void ConfigRoot::osConfigRoot()
{
	root_dir = install_dir;
}

void ConfigRoot::osConfigInstallDir()
{
	// get the pathname of the running dll / executable
	PathName module_path;
	if (!getPathFromHInstance(module_path))
	{
		module_path = fb_utils::get_process_name();
	}

	if (module_path.hasData())
	{
		// get rid of the filename
		PathName bin_dir, file_name;
		PathUtils::splitLastComponent(bin_dir, file_name, module_path);

		// search for the configuration file in the bin directory
		PathUtils::concatPath(file_name, bin_dir, CONFIG_FILE);
		DWORD attributes = GetFileAttributes(file_name.c_str());
		if (attributes == INVALID_FILE_ATTRIBUTES || attributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			// search for the configuration file in the parent directory
			PathName parent_dir;
			PathUtils::splitLastComponent(parent_dir, file_name, bin_dir);

			if (parent_dir.hasData())
			{
				PathUtils::concatPath(file_name, parent_dir, CONFIG_FILE);
				attributes = GetFileAttributes(file_name.c_str());
				if (attributes != INVALID_FILE_ATTRIBUTES && attributes != FILE_ATTRIBUTE_DIRECTORY)
				{
					install_dir = parent_dir;
				}
			}
		}

		if (install_dir.isEmpty())
		{
			install_dir = bin_dir;
		}
	}

	if (install_dir.isEmpty())
	{
		// As a last resort get it from the default install directory
		install_dir = FB_PREFIX;
	}

	PathUtils::ensureSeparator(install_dir);
}
