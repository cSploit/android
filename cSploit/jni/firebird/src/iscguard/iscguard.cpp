/*
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
#include "firebird.h"
#include <stdio.h>
#include "../yvalve/gds_proto.h"
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>
#include <prsht.h>
#include <commctrl.h>
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include "../iscguard/iscguard.rh"
#include "../iscguard/iscguard.h"
#include "../iscguard/cntlg_proto.h"
#include "../utilities/install/install_nt.h"
#include "../remote/server/os/win32/window.h"
#include "../remote/server/os/win32/chop_proto.h"
#include "../common/config/config.h"
#include "../common/classes/init.h"
#include "../common/os/path_utils.h"

#ifdef WIN_NT
#include <process.h>			// _beginthread
#endif

// Startup Configuration Entry point for regcfg.exe.
//#define SVC_CONFIG          4
//#define REGCFG_ENTRYPOINT   "LaunchInstReg"
//#define REGCFG_DLL	        "REGCFG.DLL"
typedef void (__cdecl * LPFNREGCFG) (char *, short);

// Define an array of dword pairs,
// where the first of each pair is the control ID,
// and the second is the context ID for a help topic,
// which is used in the help file.
static DWORD aMenuHelpIDs[] = {
	IDC_VERSION, ibs_guard_version,
	IDC_LOG, ibs_guard_log,
	IDC_LOCATION, ibs_server_directory,
	0, 0
};

// Function prototypes
static LRESULT CALLBACK WindowFunc(HWND, UINT, WPARAM, LPARAM);
static THREAD_ENTRY_DECLARE WINDOW_main(THREAD_ENTRY_PARAM);
#ifdef NOT_USED_OR_REPLACED
static void StartGuardian(HWND);
#endif
static bool parse_args(LPCSTR);

THREAD_ENTRY_DECLARE start_and_watch_server(THREAD_ENTRY_PARAM);
THREAD_ENTRY_DECLARE swap_icons(THREAD_ENTRY_PARAM);
static void addTaskBarIcons(HINSTANCE hInstance, HWND hWnd, BOOL& bInTaskBar);
static void write_log(int, const char*);

HWND DisplayPropSheet(HWND, HINSTANCE);
LRESULT CALLBACK GeneralPage(HWND, UINT, WPARAM, LPARAM);


HINSTANCE hInstance_gbl;
HWND hPSDlg, hWndGbl;
static int nRestarts = 0;		// the number of times the server was restarted
static bool service_flag = true;
static TEXT instance[MAXPATHLEN];
static Firebird::GlobalPtr<Firebird::string> service_name;
static Firebird::GlobalPtr<Firebird::string> remote_name;
static Firebird::GlobalPtr<Firebird::string> mutex_name;
// unsigned short shutdown_flag = FALSE;
static log_info* log_entry;
static Thread::Handle watcher_thd = 0;
static Thread::Handle swap_icons_thd = 0;

int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE /*hPrevInstance*/, LPSTR lpszCmdLine, int /*nCmdShow*/)
{
/**************************************
*
*	m a i n
*
**************************************
*
* Functional description
*     The main routine for Windows based server guardian.
*
**************************************/

	strcpy(instance, FB_DEFAULT_INSTANCE);

	service_flag = parse_args(lpszCmdLine);

	service_name->printf(ISCGUARD_SERVICE, instance);
	remote_name->printf(REMOTE_SERVICE, instance);
	mutex_name->printf(GUARDIAN_MUTEX, instance);

	// set the global HINSTANCE as we need it in WINDOW_main
	hInstance_gbl = hInstance;

	// allocate space for the event list
	log_entry = static_cast<log_info*>(malloc(sizeof(log_info)));
	log_entry->next = NULL;

	// since the flag is set we run as a service
	if (service_flag)
	{
		CNTL_init(WINDOW_main, instance);

		const SERVICE_TABLE_ENTRY service_table[] =
		{
			{const_cast<char*>(service_name->c_str()), CNTL_main_thread},
			{NULL, NULL}
		};

		// BRS There is a error in MinGW (3.1.0) headers
		// the parameter of StartServiceCtrlDispatcher is declared const in msvc headers
#if defined(MINGW)
		if (!StartServiceCtrlDispatcher(const_cast<SERVICE_TABLE_ENTRY*>(service_table)))
#else
		if (!StartServiceCtrlDispatcher(service_table))
#endif
		{
			if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
				CNTL_shutdown_service("StartServiceCtrlDispatcher failed");
		}

		if (watcher_thd)
		{
			WaitForSingleObject(watcher_thd, 5000);
			CloseHandle(watcher_thd);
		}
	}
	else {
		return WINDOW_main(0);
	}

	return TRUE;
}

static bool parse_args(LPCSTR lpszArgs)
{
/**************************************
*
*      p a r s e _ a r g s
*
**************************************
*
* Functional description
*      WinMain gives us a stupid command string, not
*      a cool argv.  Parse through the string and
*      set the options.
* Returns
*      A value of true or false depending on if -s is specified.
*      CVC: Service is the default for NT, use -a for application.
*
*
**************************************/
	bool is_service = true;
	bool delimited = false;

	for (const char* p = lpszArgs; *p; p++)
	{
		if (*p++ == '-')
		{
			char c;
			while (c = *p++)
			{
				switch (UPPER(c))
				{
				case 'A':
					is_service = false;
					break;

				case 'S':
					delimited = false;
					while (*p && *p == ' ')
						p++;
					if (*p && *p == '"')
					{
						p++;
						delimited = true;
					}

					if (delimited)
					{
						char* pi = instance;
						const char* pend = instance + sizeof(instance) - 1;
						while (*p && *p != '"' && pi < pend) {
							*pi++ = *p++;
						}
						*pi++ = '\0';
						if (*p && *p == '"')
							p++;
					}
					else
					{
						if (*p && *p != '-')
						{
							char* pi = instance;
							const char* pend = instance + sizeof(instance) - 1;
							while (*p && *p != ' ' && pi < pend) {
								*pi++ = *p++;
							}
							*pi++ = '\0';
						}
					}
					break;

				default:
					is_service = true;
					break;
				}
			}
		}
	}

	return is_service;
}

static THREAD_ENTRY_DECLARE WINDOW_main(THREAD_ENTRY_PARAM)
{
/**************************************
 *
 *	W I N D O W _ m a i n
 *
 **************************************
 *
 * Functional description
 *
 *      This function is where the actual service code starts.
 * Do all the window init stuff, then fork off a thread for starting
 * the server.
 *
 **************************************/

	// If we're a service, don't create a window
	if (service_flag)
	{
		try
		{
			Thread::start(start_and_watch_server, 0, THREAD_medium, &watcher_thd);
		}
		catch (const Firebird::Exception&)
		{
			// error starting server thread
			char szMsgString[256];
			LoadString(hInstance_gbl, IDS_CANT_START_THREAD, szMsgString, 256);
			gds__log(szMsgString);
		}

		return 0;
	}

	// Make sure that there is only 1 instance of the guardian running
	HWND hWnd = FindWindow(GUARDIAN_CLASS_NAME, GUARDIAN_APP_NAME);
	if (hWnd)
	{
		char szMsgString[256];
		LoadString(hInstance_gbl, IDS_ALREADYSTARTED, szMsgString, 256);
		MessageBox(NULL, szMsgString, GUARDIAN_APP_LABEL, MB_OK | MB_ICONSTOP);
		gds__log(szMsgString);
		return 0;
	}

	// initialize main window
	WNDCLASS wcl;
	wcl.hInstance = hInstance_gbl;
	wcl.lpszClassName = GUARDIAN_CLASS_NAME;
	wcl.lpfnWndProc = WindowFunc;
	wcl.style = 0;
	wcl.hIcon = LoadIcon(hInstance_gbl, MAKEINTRESOURCE(IDI_IBGUARD));
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcl.lpszMenuName = NULL;
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
	wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);

	if (!RegisterClass(&wcl))
	{
		char szMsgString[256];
		LoadString(hInstance_gbl, IDS_REGERROR, szMsgString, 256);
		MessageBox(NULL, szMsgString, GUARDIAN_APP_LABEL, MB_OK | MB_ICONSTOP);
		return 0;
	}

	hWnd = CreateWindowEx(0,
						  GUARDIAN_CLASS_NAME,
						  GUARDIAN_APP_NAME,
						  WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX,
						  CW_USEDEFAULT,
						  CW_USEDEFAULT,
						  CW_USEDEFAULT,
						  CW_USEDEFAULT,
						  HWND_DESKTOP, NULL, hInstance_gbl, NULL);

	// Save the window handle for the thread
	hWndGbl = hWnd;

	// begin a new thread for calling the start_and_watch_server
	try
	{
		Thread::start(start_and_watch_server, 0, THREAD_medium, NULL);
	}
	catch (const Firebird::Exception&)
	{
		// error starting server thread
		char szMsgString[256];
		LoadString(hInstance_gbl, IDS_CANT_START_THREAD, szMsgString, 256);
		MessageBox(NULL, szMsgString, GUARDIAN_APP_LABEL, MB_OK | MB_ICONSTOP);
		gds__log(szMsgString);
		DestroyWindow(hWnd);
		return 0;
	}

	SendMessage(hWnd, WM_COMMAND, IDM_CANCEL, 0);
	UpdateWindow(hWnd);

	MSG message;
	while (GetMessage(&message, NULL, 0, 0))
	{
		if (hPSDlg)
		{
			// If property sheet dialog is open
			// Check if the message is property sheet dialog specific
			BOOL bPSMsg = PropSheet_IsDialogMessage(hPSDlg, &message);

			// Check if the property sheet dialog is still valid, if not destroy it
			if (!PropSheet_GetCurrentPageHwnd(hPSDlg))
			{
				DestroyWindow(hPSDlg);
				hPSDlg = NULL;
				if (swap_icons_thd)
				{
					CloseHandle(swap_icons_thd);
					swap_icons_thd = 0;
				};
			}
			if (bPSMsg)
				continue;
		}
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	return message.wParam;
}


static LRESULT CALLBACK WindowFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
/**************************************
 *
 *	W i n d o w _ F u n c
 *
 **************************************
 *
 * Functional description
 *
 *      This function is where the windowing action takes place.
 * Handle the various messages which come here from GetMessage.
 *
 **************************************/

	static BOOL bInTaskBar = FALSE;
	static bool bStartup = false;
	static HINSTANCE hInstance = NULL;
	static UINT s_uTaskbarRestart;

	hInstance = (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	switch (message)
	{
	case WM_CLOSE:
		// Clean up memory for log_entry
		while (log_entry->next)
		{
			log_info* tmp = log_entry->next;
			free(log_entry);
			log_entry = tmp;
		}
		free(log_entry);
		DestroyWindow(hWnd);
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDM_CANCEL:
			ShowWindow(hWnd, bInTaskBar ? SW_HIDE : SW_MINIMIZE);
			return TRUE;

		case IDM_OPENPOPUP:
			{
				// The SetForegroundWindow() has to be called because our window
				// does not become the Foreground one (inspite of clicking on
				// the icon).  This is so because the icon is painted on the task
				// bar and is not the same as a minimized window.

				SetForegroundWindow(hWnd);

				HMENU hPopup = CreatePopupMenu();
				char szMsgString[256];
				LoadString(hInstance, IDS_SVRPROPERTIES, szMsgString, 256);
				AppendMenu(hPopup, MF_STRING, IDM_SVRPROPERTIES, szMsgString);
				LoadString(hInstance, IDS_SHUTDOWN, szMsgString, 256);
				AppendMenu(hPopup, MF_STRING, IDM_SHUTDOWN, szMsgString);
				LoadString(hInstance, IDS_PROPERTIES, szMsgString, 256);
				AppendMenu(hPopup, MF_STRING, IDM_PROPERTIES, szMsgString);
				SetMenuDefaultItem(hPopup, IDM_PROPERTIES, FALSE);

				POINT curPos;
				GetCursorPos(&curPos);
				TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
							   curPos.x, curPos.y, 0, hWnd, NULL);
				DestroyMenu(hPopup);
				return TRUE;
			}

		case IDM_SHUTDOWN:
			{
				HWND hTmpWnd = FindWindow(szClassName, szWindowName);
				PostMessage(hTmpWnd, WM_COMMAND, (WPARAM) IDM_SHUTDOWN, 0);
			}
			return TRUE;

		case IDM_PROPERTIES:
			if (!hPSDlg)
				hPSDlg = DisplayPropSheet(hWnd, hInstance);
			else
				SetForegroundWindow(hPSDlg);
			return TRUE;

		case IDM_INTRSVRPROPERTIES:
			return TRUE;

		case IDM_SVRPROPERTIES:
			{
				HWND hTmpWnd = FindWindow(szClassName, szWindowName);
				PostMessage(hTmpWnd, WM_COMMAND, (WPARAM) IDM_PROPERTIES, 0);
			}
			return TRUE;
		}
		break;

	case WM_SWITCHICONS:
		nRestarts++;
		{ // scope
			DWORD thr_exit = 0;
			if (swap_icons_thd == 0 ||
				!GetExitCodeThread(swap_icons_thd, &thr_exit) ||
				thr_exit != STILL_ACTIVE)
			{
				Thread::start(swap_icons, hWnd, THREAD_medium, &swap_icons_thd);
			}
		} // scope
		break;

	case ON_NOTIFYICON:
		if (bStartup)
		{
			SendMessage(hWnd, WM_COMMAND, 0, 0);
			return TRUE;
		}
		switch (lParam)
		{
		case WM_LBUTTONDOWN:
			break;

		case WM_LBUTTONDBLCLK:
			PostMessage(hWnd, WM_COMMAND, (WPARAM) IDM_PROPERTIES, 0);
			break;

		case WM_RBUTTONUP:
			PostMessage(hWnd, WM_COMMAND, (WPARAM) IDM_OPENPOPUP, 0);
			break;
		}
		break;

	case WM_CREATE:
		s_uTaskbarRestart = RegisterWindowMessage("TaskbarCreated");
		addTaskBarIcons(hInstance, hWnd, bInTaskBar);
		break;

	case WM_QUERYOPEN:
		if (!bInTaskBar)
			return FALSE;
		return DefWindowProc(hWnd, message, wParam, lParam);

	case WM_SYSCOMMAND:
		if (!bInTaskBar)
		{
			switch (wParam)
			{
			case SC_RESTORE:
				return TRUE;

			case IDM_SHUTDOWN:
				{
					HWND hTmpWnd = FindWindow(szClassName, szWindowName);
					PostMessage(hTmpWnd, WM_COMMAND, (WPARAM) IDM_SHUTDOWN, 0);
				}
				return TRUE;

			case IDM_PROPERTIES:
				if (!hPSDlg)
					hPSDlg = DisplayPropSheet(hWnd, hInstance);
				else
					SetFocus(hPSDlg);
				return TRUE;

			case IDM_SVRPROPERTIES:
				{
					HWND hTmpWnd = FindWindow(szClassName, szWindowName);
					PostMessage(hTmpWnd, WM_COMMAND, (WPARAM) IDM_PROPERTIES, 0);
				}
				return TRUE;
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);

	case WM_DESTROY:
		if (bInTaskBar)
		{
			NOTIFYICONDATA nid;

			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hWnd;
			nid.uID = IDI_IBGUARD;
			nid.uFlags = 0;
			Shell_NotifyIcon(NIM_DELETE, &nid);
		}
		PostQuitMessage(0);
		break;

	default:
		if (message == s_uTaskbarRestart)
			addTaskBarIcons(hInstance, hWnd, bInTaskBar);
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return FALSE;
}


THREAD_ENTRY_DECLARE start_and_watch_server(THREAD_ENTRY_PARAM)
{
/**************************************
 *
 *	s t a r t _ a n d _ w a t c h _ s e r v e r
 *
 **************************************
 *
 * Functional description
 *
 *      This function is where the server process is created and
 * the thread waits for this process to exit.
 *
 **************************************/
	Firebird::ContextPoolHolder threadContext(getDefaultMemoryPool());

	HANDLE procHandle = NULL;
	bool done = true;
	const UINT error_mode = SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
		SEM_NOOPENFILEERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT;
	SC_HANDLE hScManager = 0, hService = 0;

	// get the guardian startup information
	const short option = Config::getGuardianOption();

	char prefix_buffer[MAXPATHLEN];
	GetModuleFileName(NULL, prefix_buffer, sizeof(prefix_buffer));
	Firebird::PathName path = prefix_buffer;
	path = path.substr(0, path.rfind(PathUtils::dir_sep) + 1) + FBSERVER;
	path = "\"" + path + "\"";
	Firebird::PathName prog_name = path + " -a -n";

	// if the guardian is set to FOREVER then set the error mode
	UINT old_error_mode = 0;
	if (option == START_FOREVER)
		old_error_mode = SetErrorMode(error_mode);

	// Spawn the new process
	do {
		SERVICE_STATUS ServiceStatus;
		char out_buf[1024];
		BOOL success;
		int error = 0;

		if (service_flag)
		{
			if (hService)
			{
				while ((QueryServiceStatus(hService, &ServiceStatus) == TRUE) &&
					(ServiceStatus.dwCurrentState != SERVICE_STOPPED))
				{
					Sleep(500);
				}
			}

			procHandle = CreateMutex(NULL, FALSE, mutex_name->c_str());

			// start as a service.  If the service can not be found or
			// fails to start, close the handle to the mutex and set
			// success = FALSE
			if (!hScManager)
				hScManager = OpenSCManager(NULL, NULL, GENERIC_READ);
			if (!hService)
			{
				hService = OpenService(hScManager, remote_name->c_str(),
					GENERIC_READ | GENERIC_EXECUTE);
			}
			success = StartService(hService, 0, NULL);
			if (success != TRUE)
				error = GetLastError();
			// if the server is already running, then inform it that it should
			// open the guardian mutex so that it may be governed.
			if (!error || error == ERROR_SERVICE_ALREADY_RUNNING)
			{
				// Make sure that it is actually ready to receive commands.
				// If we were the one who started it, then it will need a few
				// seconds to get ready.
				while ((QueryServiceStatus(hService, &ServiceStatus) == TRUE) &&
					(ServiceStatus.dwCurrentState != SERVICE_RUNNING))
				{
					Sleep(500);
				}
				ControlService(hService, SERVICE_CREATE_GUARDIAN_MUTEX, &ServiceStatus);
				success = TRUE;
			}
		}
		else
		{
			HWND hTmpWnd = FindWindow(szClassName, szWindowName);
			if (hTmpWnd == NULL)
			{
				STARTUPINFO si;
				SECURITY_ATTRIBUTES sa;
				PROCESS_INFORMATION pi;
				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				sa.nLength = sizeof(sa);
				sa.lpSecurityDescriptor = NULL;
				sa.bInheritHandle = TRUE;
				success = CreateProcess(NULL, const_cast<char*>(prog_name.c_str()),
										&sa, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				if (success != TRUE)
					error = GetLastError();

				procHandle = pi.hProcess;
				// TMN: 04 Aug 2000 - closed the handle that previously leaked.
				CloseHandle(pi.hThread);
			}
			else
			{
				SendMessage(hTmpWnd, WM_COMMAND, (WPARAM) IDM_GUARDED, 0);
				DWORD server_pid;
				GetWindowThreadProcessId(hTmpWnd, &server_pid);
				procHandle = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, server_pid);
				if (procHandle == NULL)
				{
					error = GetLastError();
					success = FALSE;
				}
				else {
					success = TRUE;
				}
			}
		}

		if (success != TRUE)
		{
			// error creating new process
			char szMsgString[256];
			LoadString(hInstance_gbl, IDS_CANT_START_THREAD, szMsgString, 256);
			sprintf(out_buf, "%s : %s errno : %d", path.c_str(), szMsgString, error);
			write_log(IDS_CANT_START_THREAD, out_buf);

			if (service_flag)
			{
				SERVICE_STATUS status_info;
				// wait a second to get the mutex handle (just in case) and
				// then close it
				WaitForSingleObject(procHandle, 1000);
				CloseHandle(procHandle);
				hService = OpenService(hScManager, remote_name->c_str(),
					GENERIC_READ | GENERIC_EXECUTE);
				ControlService(hService, SERVICE_CONTROL_STOP, &status_info);
				CloseServiceHandle(hScManager);
				CloseServiceHandle(hService);
				CNTL_stop_service(); //service_name->c_str());
			}
			else
			{
				MessageBox(NULL, out_buf, NULL, MB_OK | MB_ICONSTOP);
				PostMessage(hWndGbl, WM_CLOSE, 0, 0);
			}
			return 0;
		}
		else
		{
			char szMsgString[256];
			LoadString(hInstance_gbl, IDS_STARTING_GUARD, szMsgString, 256);
			sprintf(out_buf, "%s: %s\n", szMsgString, path.c_str());
			write_log(IDS_LOG_START, out_buf);
		}

		// wait for process to terminate
		DWORD exit_status;
		if (service_flag)
		{
			while (WaitForSingleObject(procHandle, 500) == WAIT_OBJECT_0)
			{
				ReleaseMutex(procHandle);
				Sleep(100);
			}

			const int ret_val = WaitForSingleObject(procHandle, INFINITE);
			if (ret_val == WAIT_ABANDONED)
				exit_status = CRASHED;
			else if (ret_val == WAIT_OBJECT_0)
				exit_status = NORMAL_EXIT;

			CloseHandle(procHandle);
		}
		else
		{
			while (WaitForSingleObject(procHandle, INFINITE) == WAIT_FAILED)
				;
			GetExitCodeProcess(procHandle, &exit_status);
			CloseHandle(procHandle);
		}

		if (exit_status != NORMAL_EXIT)
		{
			// check for startup error
			if (exit_status == STARTUP_ERROR)
			{
				char szMsgString[256];
				LoadString(hInstance_gbl, IDS_STARTUP_ERROR, szMsgString, 256);
				sprintf(out_buf, "%s: %s (%lu)\n", path.c_str(), szMsgString, exit_status);
				write_log(IDS_STARTUP_ERROR, out_buf);
				done = true;

			}
			else
			{
				char szMsgString[256];
				LoadString(hInstance_gbl, IDS_ABNORMAL_TERM, szMsgString, 256);
				sprintf(out_buf, "%s: %s (%lu)\n", path.c_str(), szMsgString, exit_status);
				write_log(IDS_LOG_TERM, out_buf);

				// switch the icons if the server restarted
				if (!service_flag)
					PostMessage(hWndGbl, WM_SWITCHICONS, 0, 0);
				if (option == START_FOREVER)
					done = false;
			}
		}
		else
		{
			// Normal shutdown - ie: via ibmgr - don't restart the server
			char szMsgString[256];
			LoadString(hInstance_gbl, IDS_NORMAL_TERM, szMsgString, 256);
			sprintf(out_buf, "%s: %s\n", path.c_str(), szMsgString);
			write_log(IDS_LOG_STOP, out_buf);
			done = true;
		}

		if (option == START_ONCE)
			done = true;
	} while (!done);


	// If on WINNT
	if (service_flag)
	{
		CloseServiceHandle(hScManager);
		CloseServiceHandle(hService);
		CNTL_stop_service(); //(service_name->c_str());
	}
	else
		PostMessage(hWndGbl, WM_CLOSE, 0, 0);

	return 0;
}


HWND DisplayPropSheet(HWND hParentWnd, HINSTANCE hInst)
{
/******************************************************************************
 *
 *  D i s p l a y P r o p S h e e t
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
	PROPSHEETPAGE PSPages[1];
	HINSTANCE hInstance = hInst;

	PSPages[0].dwSize = sizeof(PROPSHEETPAGE);
	PSPages[0].dwFlags = PSP_USETITLE;
	PSPages[0].hInstance = hInstance;
	PSPages[0].pszTemplate = MAKEINTRESOURCE(IDD_PROPSHEET);
	PSPages[0].pszTitle = MAKEINTRESOURCE(IDS_PROP_TITLE);
	PSPages[0].pfnDlgProc = (DLGPROC) GeneralPage;
	PSPages[0].pfnCallback = NULL;

	PROPSHEETHEADER PSHdr;
	PSHdr.dwSize = sizeof(PROPSHEETHEADER);
	PSHdr.dwFlags = PSH_PROPTITLE | PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_MODELESS | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
	PSHdr.hwndParent = hParentWnd;
	PSHdr.hInstance = hInstance;
	PSHdr.pszIcon = MAKEINTRESOURCE(IDI_IBGUARD);
	PSHdr.pszCaption = (LPSTR) GUARDIAN_APP_LABEL;
	PSHdr.nPages = FB_NELEM(PSPages);
	PSHdr.nStartPage = 0;
	PSHdr.ppsp = (LPCPROPSHEETPAGE) & PSPages;
	PSHdr.pfnCallback = NULL;

	hPSDlg = (HWND) PropertySheet(&PSHdr);

	if (hPSDlg == 0 || hPSDlg == (HWND) -1)
	{
		gds__log("Create property sheet window failed. Error code %d", GetLastError());
		hPSDlg = NULL;
	}
	return hPSDlg;
}


LRESULT CALLBACK GeneralPage(HWND hDlg, UINT unMsg, WPARAM /*wParam*/, LPARAM lParam)
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
	HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hDlg, GWLP_HINSTANCE);

	switch (unMsg)
	{
	case WM_INITDIALOG:
		{
			char szText[256];
			char szWindowText[MAXPATHLEN];
			char szFullPath[MAXPATHLEN];
			int index = 0;
			const int NCOLS = 3;

			// Display the number of times the server has been started by
			//   this session of the guardian
			SetDlgItemInt(hDlg, IDC_RESTARTS, nRestarts, FALSE);

			// get the path to the exe.
			// Make sure that it is null terminated
			GetModuleFileName(hInstance, szWindowText, sizeof(szWindowText));
			char* pszPtr = strrchr(szWindowText, '\\');
			*(pszPtr + 1) = 0x00;

			ChopFileName(szWindowText, szWindowText, 38);
			SetDlgItemText(hDlg, IDC_LOCATION, szWindowText);

			// Get version information from the application
			GetModuleFileName(hInstance, szFullPath, sizeof(szFullPath));
			DWORD dwVerHnd;
			const DWORD dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
			if (dwVerInfoSize)
			{
				// If we were able to get the information, process it:
				UINT cchVer = 25;
				LPSTR lszVer = NULL;

				HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
				LPVOID lpvMem = GlobalLock(hMem);
				GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpvMem);
				if (VerQueryValue(lpvMem, "\\StringFileInfo\\040904E4\\FileVersion",
					 reinterpret_cast<void**>(&lszVer), &cchVer))
				{
					SetDlgItemText(hDlg, IDC_VERSION, lszVer);
				}
				else
					SetDlgItemText(hDlg, IDC_VERSION, "N/A");
				GlobalUnlock(hMem);
				GlobalFree(hMem);
			}

			// Create the columns Action, Date, Time for the listbox
			HWND hWndLog = GetDlgItem(hDlg, IDC_LOG);
			LV_COLUMN lvC;
			lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvC.fmt = LVCFMT_LEFT;	// left-align column
			lvC.pszText = szText;

			for (index = 0; index < NCOLS; index++)
			{
				// NOTE:  IDS_ACTION = 220
				// IDS_DATE   = 230
				// IDS_TIME   = 240
				lvC.iSubItem = index;
				lvC.cx = 85;
				LoadString(hInstance, IDS_ACTION + (index * 10), szText, sizeof(szText));
				ListView_InsertColumn(hWndLog, index, &lvC);
			}

			log_info* liTemp = log_entry->next;
			LV_ITEM lvI;
			lvI.cchTextMax = sizeof(liTemp->log_action);
			lvI.mask = LVIF_TEXT;
			for (index = 0; liTemp->log_action; index++, liTemp = liTemp->next)
			{
				lvI.iItem = index;
				lvI.iSubItem = 0;
				lvI.pszText = liTemp->log_action;
				ListView_InsertItem(hWndLog, &lvI);
				ListView_SetItemText(hWndLog, index, 0, lvI.pszText);

				lvI.iSubItem = 1;
				lvI.pszText = liTemp->log_date;
				ListView_InsertItem(hWndLog, &lvI);
				ListView_SetItemText(hWndLog, index, 1, lvI.pszText);

				lvI.iSubItem = 2;
				lvI.pszText = liTemp->log_time;
				ListView_InsertItem(hWndLog, &lvI);
				ListView_SetItemText(hWndLog, index, 2, lvI.pszText);
			}
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


THREAD_ENTRY_DECLARE swap_icons(THREAD_ENTRY_PARAM param)
{
/******************************************************************************
 *
 *  S w a p I c o n s
 *
 ******************************************************************************
 *
 *  Description:  Animates the icon if the server restarted
 *****************************************************************************/
	Firebird::ContextPoolHolder threadContext(getDefaultMemoryPool());

	HWND hWnd = static_cast<HWND>(param);
	HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	HICON hIconNormal = (HICON)
		LoadImage(hInstance, MAKEINTRESOURCE(IDI_IBGUARD), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);

	HICON hIconAlert = (HICON)
		LoadImage(hInstance, MAKEINTRESOURCE(IDI_IBGUARDALRT), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);

	NOTIFYICONDATA nidNormal;
	nidNormal.cbSize = sizeof(NOTIFYICONDATA);
	nidNormal.hWnd = hWnd;
	nidNormal.uID = IDI_IBGUARD;
	nidNormal.uFlags = NIF_ICON;
	nidNormal.hIcon = hIconNormal;

	NOTIFYICONDATA nidAlert;
	nidAlert.cbSize = sizeof(NOTIFYICONDATA);
	nidAlert.hWnd = hWnd;
	nidAlert.uID = IDI_IBGUARD;
	nidAlert.uFlags = NIF_ICON;
	nidAlert.hIcon = hIconAlert;

	// Animate until the property sheet is displayed.  Once the property sheet is
	// displayed, stop animating the icon
	while (!hPSDlg)
	{
		if (!Shell_NotifyIcon(NIM_MODIFY, &nidAlert))
			SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR) hIconAlert);
		Sleep(500);

		if (!Shell_NotifyIcon(NIM_MODIFY, &nidNormal))
			SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR) hIconNormal);
		Sleep(500);
	}
	// Make sure that the icon is normal
	if (!Shell_NotifyIcon(NIM_MODIFY, &nidNormal))
		SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR) hIconNormal);

	if (hIconNormal)
		DestroyIcon(hIconNormal);
	if (hIconAlert)
		DestroyIcon(hIconAlert);

	return 0;
}


static void addTaskBarIcons(HINSTANCE hInstance, HWND hWnd, BOOL& bInTaskBar)
{
	if (!service_flag)
	{
		HICON hIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_IBGUARD),
								  IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);

		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hWnd;
		nid.uID = IDI_IBGUARD;
		nid.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;
		nid.uCallbackMessage = ON_NOTIFYICON;
		nid.hIcon = hIcon;
		lstrcpy(nid.szTip, GUARDIAN_APP_LABEL);

		// This will be true if we are using the explorer interface
		bInTaskBar = Shell_NotifyIcon(NIM_ADD, &nid);

		if (hIcon)
			DestroyIcon(hIcon);

		// This will be true if we are using the program manager interface
		if (!bInTaskBar)
		{
			char szMsgString[256];
			HMENU hSysMenu = GetSystemMenu(hWnd, FALSE);
			DeleteMenu(hSysMenu, SC_RESTORE, MF_BYCOMMAND);
			AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL);
			LoadString(hInstance, IDS_SVRPROPERTIES, szMsgString, 256);
			AppendMenu(hSysMenu, MF_STRING, IDM_SVRPROPERTIES, szMsgString);
			LoadString(hInstance, IDS_SHUTDOWN, szMsgString, 256);
			AppendMenu(hSysMenu, MF_STRING, IDM_SHUTDOWN, szMsgString);
			LoadString(hInstance, IDS_PROPERTIES, szMsgString, 256);
			AppendMenu(hSysMenu, MF_STRING, IDM_PROPERTIES, szMsgString);
			DestroyMenu(hSysMenu);
		}
	}
}


static void write_log(int log_action, const char* buff)
{
/******************************************************************************
 *
 *  w r i t e _ l o g
 *
 ******************************************************************************
 *
 *  Description:  Writes the guardian information to either the Windows 95
 *                property sheet structure (log_entry) or to the Windows NT
 *                Event Log
 *****************************************************************************/
	const size_t BUFF_SIZE = 512;
	char tmp_buff[BUFF_SIZE];

	// Move to the end of the log_entry list
	log_info* log_temp = log_entry;
	while (log_temp->next)
		log_temp = log_temp->next;

	log_info* tmp = static_cast<log_info*>(malloc(sizeof(log_info)));
	memset(tmp, 0, sizeof(log_info));

#ifdef NOT_USED_OR_REPLACED
	time_t ltime;
	time(&ltime);
	const tm* today = localtime(&ltime);

	sprintf(tmp->log_time, "%02d:%02d", today->tm_hour, today->tm_min);
	sprintf(tmp->log_date, "%02d/%02d/%02d", today->tm_mon + 1, today->tm_mday, today->tm_year % 100);
#else
	// TMN: Fixed this after bug-report. Should it really force
	// 24hr format in e.g US, where they use AM/PM wharts?
	GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS | TIME_FORCE24HOURFORMAT, NULL, NULL,
				  tmp->log_time, sizeof(tmp->log_time));
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL,
				  tmp->log_date, sizeof(tmp->log_date));
#endif

	if (log_action >= IDS_LOG_START && log_action <= IDS_LOG_TERM)
	{
		// Only Windows 95 needs this information since it goes in the property sheet
		LoadString(hInstance_gbl, log_action, tmp_buff, sizeof(tmp->log_action));
		sprintf(tmp->log_action, "%s", tmp_buff);
		tmp->next = NULL;
		log_temp->next = tmp;
	}

	if (service_flag)
	{
		// on NT
		HANDLE hLog = RegisterEventSource(NULL, service_name->c_str());
		if (!hLog)
			gds__log("Error opening Windows NT Event Log");
		else
		{
			char buffer[BUFF_SIZE];
			char* act_buff[1];
			act_buff[0] = buffer;

			LoadString(hInstance_gbl, log_action + 1, tmp_buff, sizeof(tmp_buff));
			sprintf(act_buff[0], "%s", buff);

			LPTSTR lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
						  FORMAT_MESSAGE_ARGUMENT_ARRAY |
						  FORMAT_MESSAGE_FROM_STRING,
						  tmp_buff, 0, 0, (LPTSTR) &lpMsgBuf, 0,
						  reinterpret_cast<va_list*>(act_buff));

			const int len = MIN(BUFF_SIZE - 1, strlen(lpMsgBuf) - 1);
			strncpy(act_buff[0], lpMsgBuf, len);
			act_buff[0][len] = 0;
			LocalFree(lpMsgBuf);
			WORD wLogType;

			switch (log_action)
			{
			case IDS_LOG_START:
			case IDS_LOG_STOP:
				wLogType = EVENTLOG_INFORMATION_TYPE;
				break;
			default:
				wLogType = EVENTLOG_ERROR_TYPE;
			}

			if (!ReportEvent
				(hLog, wLogType, 0, log_action + 1, NULL, 1, 0,
				 const_cast<const char**>(act_buff), NULL))
			{
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
							  FORMAT_MESSAGE_FROM_SYSTEM |
							  FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
							  GetLastError(),
							  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
							  (LPTSTR) & lpMsgBuf, 0, NULL);
				gds__log("Unable to update NT Event Log.\n\tOS Message: %s", lpMsgBuf);
				LocalFree(lpMsgBuf);
			}
			DeregisterEventSource(hLog);
		}
	}

	// Write to the Firebird log
	if (*buff)
		gds__log(buff);
}
