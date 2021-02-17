/*
 *      PROGRAM:        FB Server
 *      MODULE:         property.cpp
 *      DESCRIPTION:    Property sheet implementation for WIN32 server
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
 * 2002.10.29 Sean Leyne - Removed support for obsolete IPX/SPX Protocol
 *
 */

#include "firebird.h"
#include <windows.h>
#include <shellapi.h>
#include <prsht.h>
#include <dbt.h>

// Since it's a Win32-only file, we might as well assert it
#if !defined(WIN_NT)
#error This is a Win32 only file.
#endif

#include "../jrd/license.h"
#include "../common/file_params.h"
#include "../remote/remote.h"
#include "../remote/remote_def.h"
#include "../../remote/server/serve_proto.h"
#include "../../remote/server/os/win32/window.h"
#include "../../remote/server/os/win32/window.rh"
#include "../../remote/server/os/win32/property.rh"
#include "../../remote/server/os/win32/window_proto.h"
#include "../../remote/server/os/win32/chop_proto.h"

#include "../jrd/ibase.h"

#include "../common/StatusHolder.h"

#include "../common/thd.h"		// get jrd_proto.h to declare the function
#include "../jrd/jrd_proto.h"	// gds__log()
#include <stdio.h>				// sprintf()

using namespace Firebird;


static HINSTANCE hInstance = NULL;	// Handle to the current app. instance
static HWND hPSDlg = NULL;		// Handle to the parent prop. sheet window
HBRUSH hGrayBrush = NULL;		// Handle to a Gray Brush
static USHORT usServerFlags;	// Server Flag Mask

// Window procedures
LRESULT CALLBACK GeneralPage(HWND, UINT, WPARAM, LPARAM);

// Static functions to be called from this file only.
static char* MakeVersionString(char*, size_t, USHORT);
static void RefreshUserCount(HWND);

HWND DisplayProperties(HWND hParentWnd, HINSTANCE hInst, USHORT usServerFlagMask)
{
/******************************************************************************
 *
 *  D i s p l a y P r o p e r t i e s
 *
 ******************************************************************************
 *
 *  Input:  hParentWnd - Handle to the main window of this application
 *
 *  Return: Handle to the Property sheet dialog if successful
 *          NULL if error in displaying property sheet
 *
 *  Description: This function initializes the page(s) of the property sheet,
 *               and then calls the PropertySheet() function to display it.
 *****************************************************************************/
	hInstance = hInst;
	usServerFlags = usServerFlagMask;

	PROPSHEETPAGE PSPages[1];
	PSPages[0].dwSize = sizeof(PROPSHEETPAGE);
	PSPages[0].dwFlags = PSP_USETITLE;
	PSPages[0].hInstance = hInstance;
	PSPages[0].pszTemplate = MAKEINTRESOURCE(GENERAL_DLG);
	PSPages[0].pszTitle = "General";
	PSPages[0].pfnDlgProc = (DLGPROC) GeneralPage;
	PSPages[0].pfnCallback = NULL;

	PROPSHEETHEADER PSHdr;
	PSHdr.dwSize = sizeof(PROPSHEETHEADER);
	PSHdr.dwFlags = PSH_PROPTITLE | PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_MODELESS | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
	PSHdr.hwndParent = hParentWnd;
	PSHdr.hInstance = hInstance;
	PSHdr.pszIcon = MAKEINTRESOURCE(IDI_IBSVR);
	PSHdr.pszCaption = (LPSTR) APP_LABEL;
	PSHdr.nPages = FB_NELEM(PSPages);
	PSHdr.nStartPage = 0;
	PSHdr.ppsp = (LPCPROPSHEETPAGE) & PSPages;
	PSHdr.pfnCallback = NULL;

	// Initialize the gray brush to paint the background
	// for all prop. sheet pages and their controls
	hGrayBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

	hPSDlg = (HWND) PropertySheet(&PSHdr);

	if (hPSDlg == 0 || hPSDlg == (HWND) -1)
	{
		gds__log("Create property sheet window failed. Error code %d", GetLastError());
		hPSDlg = NULL;
	}

	return hPSDlg;
}

LRESULT CALLBACK GeneralPage(HWND hDlg, UINT unMsg, WPARAM wParam, LPARAM lParam)
{
/******************************************************************************
 *
 *  G e n e r a l P a g e
 *
 ******************************************************************************
 *
 *  Input:  hDlg - Handle to the page dialog
 *          unMsg - Message ID
 *          wParam - WPARAM message parameter
 *          lParam - LPARAM message parameter
 *
 *  Return: FALSE if message is not processed
 *          TRUE if message is processed here
 *
 *  Description: This is the window procedure for the "General" page dialog
 *               of the property sheet dialog box. All the Property Sheet
 *               related events are passed as WM_NOTIFY messages and they
 *               are identified within the LPARAM which will be pointer to
 *               the NMDR structure
 *****************************************************************************/

	switch (unMsg)
	{
	case WM_INITDIALOG:
		{
			char szText[BUFFER_MEDIUM];
			lstrcpy(szText, FB_VERSION);
			SetDlgItemText(hDlg, IDC_STAT1, szText);

			if (usServerFlags & (SRVR_inet | SRVR_wnet))
				LoadString(hInstance, IDS_SERVERPROD_NAME, szText, sizeof(szText));
			else
				LoadString(hInstance, IDS_LOCALPROD_NAME, szText, sizeof(szText));

			SetDlgItemText(hDlg, IDC_PRODNAME, szText);

			char szWindowText[BUFFER_MEDIUM];
			MakeVersionString(szWindowText, sizeof(szWindowText), usServerFlags);
			SetDlgItemText(hDlg, IDC_PROTOCOLS, szWindowText);

			GetModuleFileName(hInstance, szWindowText, sizeof(szWindowText));
			char* pszPtr = strrchr(szWindowText, '\\');
			pszPtr[1] = 0x00;

			ChopFileName(szWindowText, szWindowText, 38);
			SetDlgItemText(hDlg, IDC_PATH, szWindowText);

			RefreshUserCount(hDlg);
		}
		break;
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORBTN:
		{
			OSVERSIONINFO OsVersionInfo;
			ZeroMemory(&OsVersionInfo, sizeof(OsVersionInfo));

			OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			if (GetVersionEx(&OsVersionInfo) && OsVersionInfo.dwMajorVersion < 4)
			{
				SetBkMode((HDC) wParam, TRANSPARENT);
				return (LRESULT) hGrayBrush;
			}
		}
		break;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_REFRESH:
			RefreshUserCount(hDlg);
			break;
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code)
		{
		case PSN_KILLACTIVE:
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
			break;
		}
		break;
	}
	return FALSE;
}

static char* MakeVersionString(char* pchBuf, size_t nLen, USHORT usServerFlagMask)
{
/******************************************************************************
 *
 *  M a k e V e r s i o n S t r i n g
 *
 ******************************************************************************
 *
 *  Input:  pchBuf - Buffer to be filled into
 *          nLen - Length of the buffer
 *          usServerFlagMask - Bit flag mask encoding the server flags
 *
 *  Return: Buffer containing the version string
 *
 *  Description: This method is called to get the Version String. This string
 *               is based on the flag set in usServerFlagMask.
 *****************************************************************************/
	char* p = pchBuf;
	const char* const end = p + nLen;

	if (usServerFlagMask & SRVR_inet)
	{
		p += LoadString(hInstance, IDS_TCP, p, end - p);
		if (p < end)
			*p++ = '\r';
		if (p < end)
			*p++ = '\n';
	}
	if ((usServerFlagMask & SRVR_wnet) && end > p)
	{
		p += LoadString(hInstance, IDS_NP, p, end - p);
		if (p < end)
			*p++ = '\r';
		if (p < end)
			*p++ = '\n';
	}
	if ((usServerFlagMask & SRVR_xnet) && end > p)
	{
		p += LoadString(hInstance, IDS_IPC, p, end - p);
		if (p < end)
			*p++ = '\r';
		if (p < end)
			*p++ = '\n';
	}
	if ((usServerFlagMask & SRVR_multi_client) && end > p)
	{
		p += LoadString(hInstance, IDS_SUPER, p, end - p);
	}
	*p = '\0';
	return pchBuf;
}

static void RefreshUserCount(HWND hDlg)
{
/******************************************************************************
 *
 *  R e f r e s h U s e r C o u n t
 *
 ******************************************************************************
 *
 *  Input:  hDlg - Handle to the General page dialog
 *
 *  Return: void
 *
 *  Description: This method outputs the number of active
 *               databases, attachments and services.
 *****************************************************************************/
	const HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	ULONG num_att, num_dbs, num_svc;
	SRVR_enum_attachments(num_att, num_dbs, num_svc);

	char szText[BUFFER_MEDIUM];
	sprintf(szText, "%d", num_att);
	SetDlgItemText(hDlg, IDC_STAT2, szText);
	sprintf(szText, "%d", num_dbs);
	SetDlgItemText(hDlg, IDC_STAT3, szText);
	sprintf(szText, "%d", num_svc);
	SetDlgItemText(hDlg, IDC_STAT4, szText);

	SetCursor(hOldCursor);
}
