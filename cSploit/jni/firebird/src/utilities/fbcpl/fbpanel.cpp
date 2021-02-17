/*
 *  PROGRAM:        Firebird 2.0 control panel applet
 *  MODULE:         fbpanel.cpp
 *  DESCRIPTION:    The FBPanel unit does the following things:
 *                    - It manages the display of the icon in the
 *                      Control Panel manager
 *                    - It tests whether Firebird is installed
 *                    - If Fb is installed it sets the status of items
 *                      that can be read from the Firebird registry entries
 *                      and launches the Applet.
 *
 *                  Everything else is handled by the fbdialog unit. This is
 *                  not a good thing as UI stuff is integrated with management
 *                  and diagnostic stuff. A better design for Fb 2.0 would be
 *                  to separate everything out and share the code with the
 *                  existing command line tools that manage services and
 *                  installation settings.
 *
 *  The contents of this file are subject to the Initial Developer's
 *  Public License Version 1.0 (the "License"); you may not use this
 *  file except in compliance with the License. You may obtain a copy
 *  of the License here:
 *
 *    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed on an "AS
 *  IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 *  implied. See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Initial Developer of the Original Code is Paul Reeves.
 *
 *  The Original Code is (C) 2003 Paul Reeves .
 *
 *  All Rights Reserved.
 *
 *  Contributor(s): ______________________________________.
 *
 */


#include "stdafx.h"
#include "../install/registry.h"
#include "../jrd/build_no.h"

#include "FBPanel.h"
#include "FBDialog.h"
#include <Shlwapi.h>


LONG CFBPanel::OnInquire(UINT /*uAppNum*/, NEWCPLINFO* pInfo)
{

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // Fill in the data
    pInfo->dwSize = sizeof(NEWCPLINFO); // important
    pInfo->dwFlags = 0;
    pInfo->dwHelpContext = 0;
    pInfo->lData = 0;
    pInfo->hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ICON1));
    // Shouldn't this read FB 2 without fixing the minor version?
    strcpy(pInfo->szName, "Firebird "FB_MAJOR_VER" Server Manager");
    strcpy(pInfo->szInfo, "Configure Firebird "FB_MAJOR_VER" Database Server");
    strcpy(pInfo->szHelpFile, "");
    return 0; // OK (don't send CPL_INQUIRE msg)
}


LONG CFBPanel::OnDblclk(HWND hwndCPl, UINT /*uAppNum*/, LONG /*lData*/)
{

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Create the dialog box using the parent window handle
    CFBDialog dlg(CWnd::FromHandle(hwndCPl));
	bool error = true;
	try
	{
		// Check if Firebird is installed by reading the registry
		HKEY hkey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_ROOT_INSTANCES, 0, KEY_QUERY_VALUE, &hkey)
			== ERROR_SUCCESS)
		{
			char rootpath[MAX_PATH - 2];
			DWORD buffer_size = sizeof(rootpath);
			if (RegQueryValueEx(hkey, FB_DEFAULT_INSTANCE, NULL, NULL, LPBYTE(rootpath), &buffer_size)
				== ERROR_SUCCESS)
			{
				PathAddBackslash(rootpath);
				dlg.m_Root_Path = rootpath;
			}

			RegCloseKey(hkey);

			dlg.m_FB_Version = "not known";
			CString afilename = dlg.m_Root_Path + "gbak.exe";
			buffer_size = GetFileVersionInfoSize( (LPCTSTR) afilename, 0);
			void* VersionInfo = new char [buffer_size];
			void* ProductVersion = 0;
			void* SpecialBuild = 0;
			void* PrivateBuild = 0;
			UINT ValueSize;
			if ( GetFileVersionInfo((LPCTSTR) afilename, 0, buffer_size, VersionInfo) )
			{
				VerQueryValue( VersionInfo, "\\StringFileInfo\\040904E4\\ProductVersion",
								&ProductVersion, &ValueSize);
				if (ValueSize)
				{
					dlg.m_FB_Version = "Version ";
					dlg.m_FB_Version += (char*) ProductVersion;
				}
				VerQueryValue( VersionInfo, "\\StringFileInfo\\040904E4\\SpecialBuild",
								&SpecialBuild, &ValueSize);
				if (ValueSize)
				{
					dlg.m_FB_Version += " ";
					dlg.m_FB_Version += (char*) SpecialBuild;
				}
				VerQueryValue( VersionInfo, "\\StringFileInfo\\040904E4\\PrivateBuild",
								&PrivateBuild, &ValueSize);
				if (ValueSize)
				{
					dlg.m_FB_Version += " ";
					dlg.m_FB_Version += (char*) PrivateBuild;
				}
/**/
			}
			delete[] (char*) VersionInfo;

			error = false;
			// Show the dialog box
			if (dlg.DoModal() != IDOK)
				return 0;
		}
	}
	catch ( ... )
	{
		//raise an error
		error = true;
	}

	if (error)
	{
		dlg.MessageBox("Firebird does not appear to be installed correctly.", "Installation Error", MB_OK);
	}
    return 0;
}


