/*
 *	PROGRAM:	Firebird 2.0 control panel applet
 *	MODULE:		fbdialog.cpp
 *	DESCRIPTION:	Main file to provide GUI based server control functions
 *					for Firebird 2.0 Super Server
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
 *  The Original Code is (C) 2003 Paul Reeves.
 *
 *  All Rights Reserved.
 *
 *  Contributor(s): ______________________________________.
 *
 *	History:
 *		This current version is derived from the Fb 1.0 control panel applet
 *		It was adapted to support management of a dual Super Server and
 *		Classic Server install, allowing easy switching between the two server
 *		types. Unfortunately it became obvious too late in the development
 *		cycle that such support was not feasible without delaying release
 *		further. Consequently the applet has been converted back to manage
 *		Super Server only. The relevant code for Classic has been ifdef'ed out.
 *
 *
 */


#include "stdafx.h"
//#include "FBPanel.h"
#include "FBDialog.h"

//#include "../../common/config/config.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CFBDialog


CFBDialog::CFBDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CFBDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFBDialog)
	m_FB_Version = _T("");
	m_Firebird_Status = _T("");
	//}}AFX_DATA_INIT

	m_uiTimer = 0;
	hScManager = 0;
	initialised = false;

	m_Guardian_Name		= ISCGUARD_EXECUTABLE;
	m_SS_Server_Name	= REMOTE_EXECUTABLE;
#ifdef MANAGE_CLASSIC
	m_CS_Server_Name	= REMOTE_EXECUTABLE;
#endif

	fb_status.AutoStart = false;
	fb_status.ServicesAvailable = false;
	fb_status.ServerStatus = 0;
	fb_status.UseGuardian = false;
	fb_status.UseService = false;
	fb_status.WasRunning  = false;
#ifdef MANAGE_CLASSIC
	fb_status.UseClassic = false;
#endif
	fb_status.SystemLogin = true;
	fb_status.SufficientUserRights = true;
	fb_status.ServerName = "";

	new_settings = fb_status;
}


void CFBDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFBDialog)
	DDX_Control(pDX, IDC_SERVER_ARCH, m_Server_Arch);
	DDX_Control(pDX, IDAPPLY, m_Apply);
	DDX_Control(pDX, IDC_USE_GUARDIAN, m_Use_Guardian);
	DDX_Control(pDX, IDC_MANUAL_START, m_Manual_Start);
	DDX_Control(pDX, IDC_APPLICATION, m_Run_As_Application);
	DDX_Control(pDX, IDC_SERVICE, m_Run_As_Service);
	DDX_Control(pDX, IDC_AUTO_START, m_Auto_Start);
	DDX_Control(pDX, IDC_RUN_TYPE, m_Run_Type);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_Button_Stop);
	DDX_Control(pDX, IDC_STATUS_ICON, m_Icon);
	DDX_Text(pDX, IDC_FB_VERSION, m_FB_Version);
	DDV_MaxChars(pDX, m_FB_Version, 128);
	DDX_Text(pDX, IDC_FIREBIRD_STATUS, m_Firebird_Status);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFBDialog, CDialog)
	//{{AFX_MSG_MAP(CFBDialog)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnButtonStop)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SERVICE, OnService)
	ON_BN_CLICKED(IDC_MANUAL_START, OnManualStart)
	ON_BN_CLICKED(IDC_APPLICATION, OnApplication)
	ON_BN_CLICKED(IDC_AUTO_START, OnAutoStart)
	ON_BN_CLICKED(IDC_USE_GUARDIAN, OnUseGuardian)
	ON_BN_CLICKED(IDAPPLY, OnApply)
	ON_BN_CLICKED(IDC_CLASSIC_SERVER, OnClassicServer)
	ON_BN_CLICKED(IDC_SUPER_SERVER, OnSuperServer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFBDialog message handlers


BOOL CFBDialog::OnInitDialog()
// This method is meant to do the minimum, one-time setup stuff
// UpdateServerStatus does most of the work, and is called on
// a timer, so we don't need to repeat that work here.
{
	CDialog::OnInitDialog();

	m_Reset_Display_To_Existing_Values = true;

	fb_status.ServicesAvailable = ServiceSupportAvailable();
	if ( fb_status.ServicesAvailable )
	{
		m_Run_Type.EnableWindow(TRUE);
		m_Run_As_Service.EnableWindow(TRUE);
		m_Run_As_Application.EnableWindow(TRUE);
	}
	else
	{
		m_Run_As_Service.EnableWindow(FALSE);
	}

	if (fb_status.ServicesAvailable)
	{
		if (!ValidateInstalledServices())
		{
			if (ServerStop())
				ServiceRemove();
			ValidateInstalledServices();
		}
	}
	ViewRegistryEntries();

	fb_status.SufficientUserRights = UserHasSufficientRights();

	m_Auto_Start.EnableWindow( fb_status.SufficientUserRights );
	m_Manual_Start.EnableWindow( fb_status.SufficientUserRights );
	m_Run_As_Application.EnableWindow( fb_status.SufficientUserRights );
	m_Run_As_Service.EnableWindow( fb_status.SufficientUserRights );
	m_Use_Guardian.EnableWindow( fb_status.SufficientUserRights );
	m_Button_Stop.EnableWindow( fb_status.SufficientUserRights );

	m_uiTimer = SetTimer(1, 500, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
}


void CFBDialog::UpdateServerStatus()
// This is one of the key functions. It is called
// from the timer and sets the display.
// It is in three sections
//	a) Evaluate current status of the Firebird install
//	b) Update internal variables
//	c) Update display
{

	//These two methods more or less tell us everything
	//about the current state of the server.
	ViewRegistryEntries();
	fb_status.ServerStatus = GetServerStatus();


#ifdef MANAGE_CLASSIC
	if (fb_status.WasRunning && !fb_status.UseService)
	{
		fb_status.UseClassic = (bool) GetClassicServerHandle();
	}
	else
	{
		fb_status.UseClassic = (bool) GetPreferredArchitecture();
	}
#endif

//========Update other internal variables ==============

	m_Firebird_Status.Format(fb_status.ServerStatus);

#ifdef MANAGE_CLASSIC
	if (fb_status.UseClassic)
	{
		fb_status.ServiceExecutable = m_CS_Server_Name;
	}
	else
#endif
	{
		fb_status.ServiceExecutable = m_SS_Server_Name;
	}

//========Start of code that updates GUI================
	if ( fb_status.WasRunning )
	{
		m_Icon.SetIcon(AfxGetApp()->LoadIcon(IDI_ICON1));
		m_Button_Stop.SetWindowText("&Stop");
	}
	else
	{
		m_Icon.SetIcon(AfxGetApp()->LoadIcon(IDI_ICON4));
		m_Button_Stop.SetWindowText("&Start");
	}

	//Reset check boxes
	if (m_Reset_Display_To_Existing_Values)
	{
		// This is always called on startup. This method
		// is also called if an attempt to install
		// the service fails.
		ResetCheckBoxes( fb_status );
		if ( m_Apply.IsWindowEnabled() )
			ApplyChanges();
	}

	m_Reset_Display_To_Existing_Values = false;

	UpdateData(false);

	// This will be false the first time round.
	// It is needed because the Config class doesn't
	// refresh after the conf file has been updated.
	initialised = true;

	// The only time new_settings differs from fb_status is
	// during the ApplyChanges method.
	new_settings = fb_status;
}


bool CFBDialog::CheckServiceInstalled( LPCTSTR service )
{
	OpenServiceManager( GENERIC_READ );
	SC_HANDLE hService = OpenService (hScManager, service, GENERIC_READ );
	const bool result = hService != NULL;
	if (result)
		CloseServiceHandle( hService );
	CloseServiceManager();
	return result;
}


bool CFBDialog::ValidateInstalledServices()
// Check if services are installed.
// If Guardian installed but not Server service
// then return false;
{
	fb_status.UseService = CheckServiceInstalled(GetServiceName(REMOTE_SERVICE));
	fb_status.UseGuardian = CheckServiceInstalled(GetServiceName(ISCGUARD_SERVICE));

	return (!fb_status.UseService && fb_status.UseGuardian) ? false : true;
}

CString CFBDialog::GetServiceName(const char* name) const
{
	CString serviceName;
	serviceName.Format(name, FB_DEFAULT_INSTANCE);
	return serviceName;
}

int CFBDialog::GetServerStatus()
// This is called by UpdateServerStatus,
// which is called on the timer, so our status
// should always be 'up-to-date'.
// Returns:
//		fb_status.ServerStatus
// Also sets:
//		fb_status.UseService
//		fb_status.WasRunning

{
	int result = IDS_APPLICATION_STOPPED;

	const CString serviceName = GetServiceName(REMOTE_SERVICE);
	fb_status.UseService = CheckServiceInstalled(serviceName);

	if ( fb_status.UseService )
	{
		OpenServiceManager( GENERIC_READ );
		SC_HANDLE hService = OpenService (hScManager, serviceName, GENERIC_READ );
		QueryServiceStatus( hService, &service_status );
		CloseServiceHandle ( hService );
		CloseServiceManager();
		switch ( service_status.dwCurrentState )
		{
			case SERVICE_STOPPED :
				result = IDS_SERVICE_STOPPED;
				break;
			case SERVICE_START_PENDING :
				result = IDS_SERVICE_START_PENDING;
				break;
			case SERVICE_STOP_PENDING :
				result = IDS_SERVICE_STOP_PENDING;
				break;
			case SERVICE_RUNNING :
				result = IDS_SERVICE_RUNNING;
				break;
			case SERVICE_CONTINUE_PENDING :
				result = IDS_SERVICE_CONTINUE_PENDING;
				break;
			case SERVICE_PAUSE_PENDING :
				result = IDS_SERVICE_PAUSE_PENDING;
				break;
			case SERVICE_PAUSED :
				result = IDS_SERVICE_PAUSED;
				break;
		}
	}
	else
	{
		//Is Firebird running as an application...
		if ( FirebirdRunning() )
		{
			result = IDS_APPLICATION_RUNNING;
		}
		else
		{
			result = IDS_APPLICATION_STOPPED;
		}
	}

	fb_status.WasRunning = ((fb_status.ServerStatus == IDS_SERVICE_RUNNING) ||
							(fb_status.ServerStatus == IDS_APPLICATION_RUNNING) );

	// If running as an application and not set to run automatically on start up
	// we still don't  know if we the guardian is running.
	if (fb_status.WasRunning && !fb_status.UseService)
	{
		fb_status.UseGuardian = (bool) GetGuardianHandle();
	}

	return result;
}


void CFBDialog::ViewRegistryEntries()
// Find out what we have in the non-Firebird section of the
// registry ie, in Services or Application Run
//
// The following variables will be set on return:
//	fb_status.UseGuardian
//	fb_status.AutoStart
//	fb_status.SystemLogin
//	fb_status.UseClassic
//	fb_status.ServerName
{

	fb_status.AutoStart = false;

	fb_status.UseService = CheckServiceInstalled(GetServiceName(REMOTE_SERVICE));

	if ( fb_status.UseService )
	{
		OpenServiceManager( GENERIC_READ );

		CString service = GetServiceName(ISCGUARD_SERVICE);
		CString display_name = GetServiceName(ISCGUARD_DISPLAY_NAME);
		SC_HANDLE hService = OpenService (hScManager, service, SERVICE_QUERY_CONFIG);
		fb_status.UseGuardian = hService;
		if (hService != NULL) // then we are running as a Service
		{
			DWORD dwBytesNeeded;
			LPQUERY_SERVICE_CONFIG status_info = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LPTR, 4096);
			if (!QueryServiceConfig(hService, status_info, 4096, &dwBytesNeeded))
			{
				LocalFree( status_info );
				HandleError(false, "ViewRegistryEntries - Cannot query Guardian service.");
			}
			else
			{
				if (status_info->dwStartType == SERVICE_AUTO_START )
				{
					fb_status.AutoStart = true;
				}
			}
			CloseServiceHandle (hService);
			LocalFree( status_info );

		}

		//Now do the same again, but this time only look at the server itself
		service = GetServiceName(REMOTE_SERVICE);
		display_name = GetServiceName(REMOTE_DISPLAY_NAME);
		hService = OpenService (hScManager, service, SERVICE_QUERY_CONFIG);
		CloseServiceManager();
		if (hService != NULL) // then we are running as a Service
		{
			DWORD dwBytesNeeded;
			LPQUERY_SERVICE_CONFIG status_info = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LPTR, 4096);
			if (!QueryServiceConfig(hService, status_info, 4096, &dwBytesNeeded))
			{
				LocalFree( status_info );
				HandleError(false, "ViewRegistryEntries - Cannot query server service.");
			}
			else
			{
				fb_status.ServerName = status_info->lpBinaryPathName;
			}

			CString LoginAccount = status_info->lpServiceStartName;
			if (  LoginAccount == "LocalSystem" )
			{
				fb_status.SystemLogin = true;
			}
			else
			{
				fb_status.SystemLogin = false;
			}
			LocalFree( status_info );
			CloseServiceHandle (hService);
		}
	}
	else //Installed as Application, so look for an entry in registry
	{
		HKEY hkey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
				0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
		{
			DWORD dwType;
			DWORD dwSize = MAX_PATH;
			fb_status.AutoStart = (RegQueryValueEx(hkey, "Firebird", NULL, &dwType,
				(LPBYTE) fb_status.ServerName.GetBuffer(dwSize / sizeof(TCHAR)), &dwSize) ==
					ERROR_SUCCESS );
			fb_status.ServerName.ReleaseBuffer(-1);
			if (fb_status.AutoStart)
				fb_status.UseGuardian = ( fb_status.ServerName.Find("fbguard") == ERROR_SUCCESS );

			RegCloseKey (hkey);
		}
	}

#ifdef MANAGE_CLASSIC
	//Our look in the registry has probably given us
	//the filename of the installed server
	if ( fb_status.ServerName.Find("fb_inet_server") == ERROR_SUCCESS )
	{
		fb_status.UseClassic = true;
	}
	if ( fb_status.ServerName.Find("fbserver") == ERROR_SUCCESS )
	{
		fb_status.UseClassic = false;
	}
	if ( fb_status.ServerName == "" )
	{
		// Nothing is stored in the registry so we must look to see if
		// Firebird.conf has a preference
		fb_status.UseClassic = (bool) GetPreferredArchitecture();
	}
#endif
}


void CFBDialog::ApplyChanges()
/*
 *
 *	It all happens here.
 *
 *	With the addition of support for classic we now need to
 *  evaluate 16 possible states.
 *
 */
{
	// Stop the update timer before doing anything else
	if (m_uiTimer) KillTimer(m_uiTimer);

	//find out what has changed and implement the changes
	try
	{
	//Stage 1
		// Stop the Server
		// We don't try to restart unless we were running
		// and we successfully stopped the server.
		if ( fb_status.WasRunning )
		{
			if ( ServerStop() )
			{
#if defined(_DEBUG)
				// If we are in debug mode it is useful to
				// reset the display - but UpdateServerStatus
				// resyncs new_settings with fb_status, so
				// it must never be called in ApplyChanges()
				// after this point.
				UpdateServerStatus();
				ProcessMessages();
#endif
				new_settings.WasRunning = true;
			}
			else
			{
			//If we can't stop the server we should give up.
				HandleError(false, "Failed to stop server. New settings will not be applied.");
				throw;
			}
		}


	//Stage 2 - Gather details of changes to make

		//Manage change to startup - from/to manual or auto
		const bool ChangeStartType =
			( fb_status.AutoStart &&  m_Manual_Start.GetCheck() ) ||
			( !fb_status.AutoStart &&  m_Auto_Start.GetCheck() ) ;

		if ( ChangeStartType )
			new_settings.AutoStart = !fb_status.AutoStart;
		else
			new_settings.AutoStart = fb_status.AutoStart;

#ifdef MANAGE_CLASSIC
		//Do we use Super Server or Classic
		const bool ChangeServerArchitecture =
			( !fb_status.UseClassic && m_Classic_Server.GetCheck() ) ||
			( fb_status.UseClassic && m_Super_Server.GetCheck() );
		if ( ChangeServerArchitecture )
			new_settings.UseClassic = !fb_status.UseClassic;
		else
			new_settings.UseClassic = fb_status.UseClassic;

		if (new_settings.UseClassic)
		{
			new_settings.ServiceExecutable = m_CS_Server_Name;
		}
		else
		{
			new_settings.ServiceExecutable = m_SS_Server_Name;
		}
#endif

		//Do we change Guardian Usage?
		const bool ChangeGuardianUse =
			( !fb_status.UseGuardian && m_Use_Guardian.GetCheck() ) ||
			( fb_status.UseGuardian && !m_Use_Guardian.GetCheck() );
		if ( ChangeGuardianUse )
			new_settings.UseGuardian = !fb_status.UseGuardian;
		else
			new_settings.UseGuardian = fb_status.UseGuardian;

		//Finally, test for change between service and application usage.
		const bool ChangeRunStyle =
			( fb_status.UseService && m_Run_As_Application.GetCheck() )  ||
			( !fb_status.UseService && m_Run_As_Service.GetCheck() );
		if (ChangeRunStyle)
			new_settings.UseService = !fb_status.UseService;
		else
			new_settings.UseService = fb_status.UseService;


	//Stage 3 - implement changes

#if !defined(_DEBUG)
		BeginWaitCursor();
#endif

		//Three things to do
		// a) First pull down what is already there
		if ( ChangeRunStyle || ChangeGuardianUse /* || ChangeServerArchitecture */)
		{
			if ( fb_status.UseService )
			{
				ServiceRemove();
			}
			else
				AppRemove();
		}


#ifdef FBCPL_UPDATE_CONF
		// b) update firebird.conf
		if ( ChangeGuardianUse )
		{
			SetGuardianUseInConf( new_settings.UseGuardian );
		}
#endif
#ifdef MANAGE_CLASSIC
		if ( ChangeServerArchitecture )
		{
			SetPreferredArchitectureInConf( new_settings.UseClassic );
		}
#endif

		// c) install the new configuration
		if ( ChangeRunStyle || ChangeGuardianUse
#ifdef MANAGE_CLASSIC

			 || ChangeServerArchitecture
#endif
			)
		{
			if ( new_settings.UseService )
			{
				ServiceInstall( new_settings );
			}
			else
			{
				AppInstall( new_settings );
			}
		}
		else
		{
			// We are not changing the run style and not changing guardian usage
			// and we are not changing the server architecture so
			// we only have the autostart setting left to change
			SetAutoStart( new_settings );
		}

		if ( new_settings.WasRunning )
		{
			ProcessMessages();
			ServerStart( new_settings );
		}

		// If we haven't had a failure then we disable the apply button
		if (!m_Reset_Display_To_Existing_Values)
			DisableApplyButton();

		// Update fb_status if we are running as an application
		if (!new_settings.UseService && ChangeGuardianUse)
			fb_status.UseGuardian = !fb_status.UseGuardian;

		//And finally reset the m_error_status to zero;
		m_Error_Status = 0;

	}
	catch ( ... )
	{
		// Oops, something bad happened. Which
		// means the apply button is still enabled.
		// We should probably do a reset here.

		m_Reset_Display_To_Existing_Values = true;
	}

#if !defined(_DEBUG)
	EndWaitCursor();
#endif

	//Whatever the outcome of ApplyChanges we need to refresh the dialog
	m_uiTimer = SetTimer( 1, 500, NULL );
}


void CFBDialog::ResetCheckBoxes(const CFBDialog::STATUS status)
{
	switch (status.ServerStatus)
	{
	case IDS_APPLICATION_RUNNING:
	case IDS_APPLICATION_STOPPED:
		m_Run_As_Application.SetCheck(1);
		m_Run_As_Service.SetCheck(0);
		break;
	default:
		m_Run_As_Application.SetCheck(0);
		m_Run_As_Service.SetCheck(1);
	}

	//Now are we starting automatically or not?
	if (status.AutoStart)
	{
		m_Auto_Start.SetCheck(1);
		m_Manual_Start.SetCheck(0);
	}
	else
	{
		m_Auto_Start.SetCheck(0);
		m_Manual_Start.SetCheck(1);
	}

	m_Use_Guardian.SetCheck(status.UseGuardian);

#ifdef MANAGE_CLASSIC
	if (status.UseClassic)
	{
		m_Classic_Server.SetCheck(1);
		m_Super_Server.SetCheck(0);
	}
	else
	{
		m_Classic_Server.SetCheck(0);
		m_Super_Server.SetCheck(1);
	}
#endif

	// The server can now be controlled by a specific
	// username/password. If it is set then for now we
	// will disable all config options and add
	// support at a later date.
/*	m_Classic_Server.EnableWindow( status.SystemLogin );
	m_Super_Server.EnableWindow( status.SystemLogin );
*/
	m_Auto_Start.EnableWindow( status.SystemLogin && fb_status.SufficientUserRights );
	m_Manual_Start.EnableWindow( status.SystemLogin && fb_status.SufficientUserRights );
	m_Run_As_Application.EnableWindow( status.SystemLogin && fb_status.SufficientUserRights );
	m_Run_As_Service.EnableWindow( status.SystemLogin && fb_status.SufficientUserRights );
	m_Use_Guardian.EnableWindow( status.SystemLogin && fb_status.SufficientUserRights );
	m_Button_Stop.EnableWindow( status.SystemLogin && fb_status.SufficientUserRights );
}


int CFBDialog::DatabasesConnected()
// Check if any databases are open on the server.
//**
//** Note: We really need a way of getting number of attachments
//** without having to enter a username / password.
//** This could be done by reading the LOCK_Header
//**
{
	int nDatabases = 0;
	return nDatabases;
}


bool CFBDialog::ServerStart(const CFBDialog::STATUS status )
{
	bool result = false;

#if !defined(_DEBUG)
	BeginWaitCursor();
#endif

	if ( status.UseService )
	{
		CString service;
		CString display_name;
		if (status.UseGuardian)
		{
			service = GetServiceName(ISCGUARD_SERVICE);
			display_name = GetServiceName(ISCGUARD_DISPLAY_NAME);
		}
		else
		{
			service = GetServiceName(REMOTE_SERVICE);
			display_name = GetServiceName(REMOTE_DISPLAY_NAME);
		}

		OpenServiceManager(GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE);
		if (hScManager)
		{
			try
			{
				m_Error_Status = SERVICES_start (hScManager, service, /*display_name,*/ 0, svc_error);
				if (m_Error_Status == FB_SUCCESS)
					result = true;
			}
			catch ( ... )
			{
			}
		}
		CloseServiceManager();
	}
	else
	{
		try
		{
			STARTUPINFO si;
			SECURITY_ATTRIBUTES sa;
			PROCESS_INFORMATION pi;
			ZeroMemory (&si, sizeof(si));
			si.cb = sizeof (si);
			sa.nLength = sizeof (sa);
			sa.lpSecurityDescriptor = NULL;
			sa.bInheritHandle = TRUE;

			char full_name[MAX_PATH + 15] = "";
			GetFullAppPath( status, full_name );

			if (!CreateProcess (NULL, full_name, &sa, NULL, FALSE, 0, NULL, NULL, &si, &pi))
				HandleError(false, "Application Start");
			else
			{
				result = true;
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
		}
		catch ( ... )
		{
		}
	}

#if !defined(_DEBUG)
	EndWaitCursor();
#endif

	return result;
}


bool CFBDialog::ServerStop()
{
	bool result = false;

	if (!DatabasesConnected())
	{
		if ( fb_status.UseService )
		{
			try
			{
#if !defined(_DEBUG)
				BeginWaitCursor();
#endif
				OpenServiceManager( GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE );

				CString service;
				CString display_name;
				if (fb_status.UseGuardian)
				{
					service = GetServiceName(ISCGUARD_SERVICE);
					display_name = GetServiceName(ISCGUARD_DISPLAY_NAME);
				}
				else
				{
					service = GetServiceName(REMOTE_SERVICE);
					display_name = GetServiceName(REMOTE_DISPLAY_NAME);
				}
				m_Error_Status = SERVICES_stop(hScManager, service, /*display_name,*/ svc_error);

				result = !m_Error_Status;

			}
			catch (...)
			{
				MessageBeep(-1);
			}

			CloseServiceManager();
#if !defined(_DEBUG)
			EndWaitCursor();
#endif
		}
		else
		{
			try
			{
#if !defined(_DEBUG)
				BeginWaitCursor();
#endif
				KillApp();
				result = true;
			}
			catch (...)
			{
			}
#if !defined(_DEBUG)
			EndWaitCursor();
#endif
		}
	}

	return result;
}


void CFBDialog::KillApp()
{
	// Under Win2K3 and WinXP there seem to be timing issues that don't
	// exist in earlier platforms. Killing the server will kill the
	// Guardian, but it won't always do it quickly enough for the CPL
	// applet, so we try and do that before killing the server.
	LRESULT result;
	HWND hTmpWnd = GetGuardianHandle();
	if (hTmpWnd != NULL)
	{
		result = ::SendMessage(hTmpWnd, WM_CLOSE, 0, 0);
		ProcessMessages();
		hTmpWnd = NULL;
	}

	hTmpWnd = GetFirebirdHandle();
	if (hTmpWnd != NULL)
	{
		result = ::SendMessage(hTmpWnd, WM_CLOSE, 0, 0);
		ProcessMessages();
		hTmpWnd = NULL;
	}
}


bool CFBDialog::ServiceInstall(CFBDialog::STATUS status )
{
	OpenServiceManager( GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE );

	if (hScManager)
	{
		const char* ServerPath = m_Root_Path;

		const CString guard_service = GetServiceName(ISCGUARD_SERVICE);
		const CString guard_display_name = GetServiceName(ISCGUARD_DISPLAY_NAME);
		const CString remote_service = GetServiceName(REMOTE_SERVICE);
		const CString remote_display_name = GetServiceName(REMOTE_DISPLAY_NAME);

		if (new_settings.UseGuardian)
		{
			m_Error_Status = SERVICES_install (hScManager, guard_service, guard_display_name,
				ISCGUARD_DISPLAY_DESCR, ISCGUARD_EXECUTABLE, ServerPath, NULL, NULL,
				status.AutoStart, NULL, NULL, false, true, svc_error);
			if (m_Error_Status != FB_SUCCESS)
			{
				CloseServiceManager();
				return false;
			}

			// Set AutoStart to manual in preparation for installing the fb_server service
			status.AutoStart = false;

		}
		// do the install of server
		m_Error_Status = SERVICES_install (hScManager, remote_service, remote_display_name,
			REMOTE_DISPLAY_DESCR, (LPCTSTR) status.ServiceExecutable, ServerPath, NULL, NULL,
			status.AutoStart, NULL, NULL, false, !new_settings.UseGuardian, svc_error);
		if (m_Error_Status != FB_SUCCESS)
		{
			CloseServiceManager();
			try
			{
				ServiceRemove();
				m_Reset_Display_To_Existing_Values = true;
				EnableApplyButton();
			}
			catch ( ... )
			{
			}

			return false;
		}

		CloseServiceManager();
		return true;
	}

	return false;
}


bool CFBDialog::ServiceRemove()
{
	OpenServiceManager( GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE );
	if (hScManager)
	{
		const CString guard_service = GetServiceName(ISCGUARD_SERVICE);
		const CString guard_display_name = GetServiceName(ISCGUARD_DISPLAY_NAME);
		const CString remote_service = GetServiceName(REMOTE_SERVICE);
		const CString remote_display_name = GetServiceName(REMOTE_DISPLAY_NAME);

		m_Error_Status = SERVICES_remove (hScManager, guard_service, /*guard_display_name,*/ svc_error);
		if (m_Error_Status == IB_SERVICE_RUNNING)
		{
			CloseServiceManager();
			return false;
		}

		m_Error_Status = SERVICES_remove (hScManager, remote_service,/* remote_display_name,*/ svc_error);
		if (m_Error_Status == IB_SERVICE_RUNNING)
		{
			CloseServiceManager();
			return false;
		}
		CloseServiceManager();
		return true;
	}

	return false;
}


bool CFBDialog::AppInstall(const CFBDialog::STATUS status)
// This method is supplied as a corollary to ServiceInstall,
// but doesn't do very much as there isn't much to do.
{
	return ConfigureRegistryForApp( status );
}


bool CFBDialog::ConfigureRegistryForApp(const CFBDialog::STATUS status)
{
	// The calling procedure will have already removed the
	// service. All we need to do now
	// is configure the registry if AutoStart has been set.
	if ( status.AutoStart )
	{
		// Add line to registry
		HKEY hkey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
				0, KEY_WRITE, &hkey) == ERROR_SUCCESS)
		{

			char full_name[MAX_PATH + 15] = "";
			GetFullAppPath( status, full_name);
			if (!RegSetValueEx (hkey, "Firebird", 0,REG_SZ, (CONST BYTE*) full_name, sizeof(full_name) ) ==
					ERROR_SUCCESS)
			{
				HandleError(false, "AppInstall");
				return false;
			}
		}
		else
		{
			HandleError(false, "AppInstall");
			return false;
		}
	}
	else
	{
		// Remove registry entry if set to start automatically on boot.
		HKEY hkey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
				0, KEY_QUERY_VALUE | KEY_WRITE, &hkey) == ERROR_SUCCESS)

		{
			if (RegQueryValueEx(hkey, "Firebird", NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
			{
				if (RegDeleteValue(hkey, "Firebird") == ERROR_SUCCESS)
					return true;

				HandleError(false, "Removing registry entry to stop autorun failed.");
				return false;
			}
			// If an error is thrown it must be because there is no
			// entry in the registry so we shouldn't need to show an error.
			return true;
		}

		//Things are really bad - perhaps user has screwed up their registry?
		HandleError(false, "Could not find HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run in the registry.");
		return false;
	}

	return true;
}


bool CFBDialog::AppRemove()
{
	return ConfigureRegistryForApp( fb_status );
}


static USHORT svc_error (SLONG	error_status, const TEXT* string, SC_HANDLE service)
//This code is for use with the SERVICES_ functions
{
	bool RaiseError = true;

	// process the kinds of errors we may be need to deal with quietly
	switch ( error_status )
	{
		case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
		case ERROR_SERVICE_ALREADY_RUNNING:
		case ERROR_SERVICE_DOES_NOT_EXIST:
			RaiseError = false;
			break;
	}

	if (RaiseError)
		 CFBDialog::HandleSvcError(error_status, string);

	if (service != NULL)
		CloseServiceHandle (service);

	return error_status;
}


void CFBDialog::ProcessMessages()
{
	MSG Msg;

	while (::PeekMessage(&Msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!AfxGetApp()->PumpMessage())
		{
			::PostQuitMessage(0);
			return;
		}
	}

	LONG lIdle = 0;
	while (AfxGetApp()->OnIdle(lIdle++))
		;
	return;
}


void CFBDialog::HandleSvcError(SLONG error_status, const TEXT* string )
// This method supports the static svc_error() function
// and essentially duplicates HandleError. Oh to be rid of the
// legacy code.
{
	if (!error_status)
		error_status = GetLastError();

	LPTSTR lpMsgBuf = 0;
	CString error_title = "";

	const DWORD Size = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error_status,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,	0, NULL );

	if (Size)
		error_title.Format("Error '%s' raised in %s", lpMsgBuf, string);
	else
		error_title.Format("Error Code %d raised in %s", error_status, string );
	::MessageBox( NULL, lpMsgBuf, (LPCTSTR) error_title, MB_OK | MB_ICONINFORMATION );
	LocalFree( lpMsgBuf );
}


void CFBDialog::HandleError(bool silent, const TEXT *string )
{
	const DWORD error_code = GetLastError();
	if (error_code == m_Error_Status)
	{
		//Always be silent if error has not already been thrown.
		silent = true;
	}
	else
	{
		m_Error_Status = error_code;
	}

	if (silent)
	{
		//And do what
	}
	else
	{
		LPTSTR lpMsgBuf = 0;
		CString error_title = "";

		const DWORD Size = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error_code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,	0, NULL );

		if (Size)
			error_title.Format("Error '%s' raised in %s", lpMsgBuf, string);
		else
			error_title.Format("Error Code %d raised in %s", error_code, string );

		ShowError(lpMsgBuf, error_title);

		LocalFree( lpMsgBuf );
	}
}


void CFBDialog::ShowError( LPTSTR lpMsgBuf, CString error_title )
{
	CFBDialog::MessageBox( lpMsgBuf, (LPCTSTR) error_title, MB_OK | MB_ICONINFORMATION );
}


void CFBDialog::OnButtonStop()
{
	if ( fb_status.WasRunning )
		ServerStop();
	else
		ServerStart( fb_status );
}


void CFBDialog::OnOK()
{
	// Extra validation can be added here

	//if IDAPPLY is enabled then click IDAPPLY to apply changes before we close
	if (m_Apply.IsWindowEnabled())
	{
		OnApply();
	}

	CDialog::OnOK();
}


void CFBDialog::EnableApplyButton()
{
	m_Apply.EnableWindow(TRUE);
	m_Button_Stop.EnableWindow(FALSE);
}


void CFBDialog::DisableApplyButton()
{
	m_Apply.EnableWindow(FALSE);
	m_Button_Stop.EnableWindow(TRUE);
}


void CFBDialog::OnTimer(UINT_PTR /*nIDEvent*/)
{
	UpdateServerStatus();
}


void CFBDialog::OnDestroy()
{
	// Kill the update timer
	if (m_uiTimer) KillTimer(m_uiTimer);

	CDialog::OnDestroy();
}


void CFBDialog::OnService()
{
	EnableApplyButton();
}


void CFBDialog::OnManualStart()
{
	EnableApplyButton();
}


void CFBDialog::OnApplication()
{
	EnableApplyButton();
}


void CFBDialog::OnAutoStart()
{
	EnableApplyButton();
}


void CFBDialog::OnUseGuardian()
{
	EnableApplyButton();
}


void CFBDialog::OnSuperServer()
{
	EnableApplyButton();
}


void CFBDialog::OnClassicServer()
{
	EnableApplyButton();
}


void CFBDialog::OnApply()
{
	ApplyChanges();
}


#ifdef FBCPL_UPDATE_CONF
// Currently (Fb 1.5.1) there is no longer any
// need to change the .conf file
void CFBDialog::SetGuardianUseInConf( bool UseGuardian )
{
	CString newvalue = "";
	if (UseGuardian)
		newvalue = "1";
	else
		newvalue = "0";

	// One day the Config class will have set methods...
	//if (Config::setGuardianOption( UseGuardian ))

	if (UpdateFirebirdConf("GuardianOption", newvalue ) )
	{
		// Do we assign here? or wait for the
		// update status routine to pick this up?
		fb_status.UseGuardian = UseGuardian;
	}
	else
	{
		HandleError(false, "SetGuardianUseInConf");
		return;
	}
}

#ifdef MANAGE_CLASSIC
void CFBDialog::SetPreferredArchitectureInConf( bool UseClassic )
{
	CString newvalue = "";
	if ( UseClassic )
		newvalue = "1";
	else
		newvalue = "0";


	// One day the Config class will have set methods...
	// if (Config::setPreferClassicServer( UseClassic ))

	if ( UpdateFirebirdConf("PreferClassicServer", newvalue ) )
	{
		// Do we assign here? or wait for the
		// update status routine to pick this up?
		fb_status.UseClassic = UseClassic;
	}
	else
	{
		HandleError(false, "SetPreferredArchitectureInConf");
		return;
	}
}
#endif

bool CFBDialog::UpdateFirebirdConf(CString option, CString value)
{
	CStdioFile FbConfFile, FbConfFileNew;
	CString FirebirdConfFilename = m_Root_Path + "firebird.conf";
	CString FirebirdConfNewname = FirebirdConfFilename + ".new";
	CString FirebirdConfOldname = FirebirdConfFilename + ".old";
	CString FirebirdConfLine = "";

	if (!FbConfFile.Open(FirebirdConfFilename, CFile::modeReadWrite))
		return false;
	if (!FbConfFileNew.Open(FirebirdConfNewname, CFile::modeCreate |
							CFile::shareExclusive | CFile::modeWrite))
	{
		return false;
	}

	try
	{
		while (FbConfFile.ReadString(FirebirdConfLine) != NULL)
		{

			if (FirebirdConfLine.Find( option ) > -1 )
			{
				FirebirdConfLine = option + " = " + value;
			}

			FbConfFileNew.WriteString( FirebirdConfLine + '\n');

		}
	}
	 catch (CFileException *e)
	{
#ifdef _DEBUG
		afxDump << "Problem updating " << e->m_strFileName << ".\n \
					cause = " << e->m_cause << "\n";
#endif
	}


	bool result = false;
	FbConfFile.Close();
	FbConfFileNew.Close();

	try
	{
		CFile::Rename(FirebirdConfFilename, FirebirdConfOldname);
		CFile::Rename(FirebirdConfNewname, FirebirdConfFilename);

		//If we get this far then all is well and we can return good news
		result = true;
	}
	catch (CFileException*)
	{
		CFile::Rename(FirebirdConfOldname, FirebirdConfFilename);
	}

	//always try to delete the temporary old conf file.
	CFile::Remove(FirebirdConfOldname);

	return result;
}
#endif //#ifdef FBCPL_UPDATE_CONF


void CFBDialog::SetAutoStart(const CFBDialog::STATUS status )
{
	if (status.UseService)
	{
		OpenServiceManager( GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE );
		// Need to acquire database lock before reconfiguring.
		if (hScManager)
		{
			SC_LOCK sclLock = LockServiceDatabase(hScManager);

			// If the database cannot be locked, report the details.
			if (sclLock == NULL)
			{
				HandleError(false, "SetAutoStart - Could not lock service database");
				return;
			}

			// The database is locked, so it is safe to make changes.

			CString service;
			CString display_name;
			if (status.UseGuardian)
			{
				service = GetServiceName(ISCGUARD_SERVICE);
				display_name = GetServiceName(ISCGUARD_DISPLAY_NAME);
			}
			else
			{
				service = GetServiceName(REMOTE_SERVICE);
				display_name = GetServiceName(REMOTE_DISPLAY_NAME);
			}

			// Open a handle to the service.
			SC_HANDLE hService = OpenService(
				hScManager,				// SCManager database
				service,				// name of service
				SERVICE_CHANGE_CONFIG);	// need CHANGE access
			if (hService)
			{
				DWORD dwStartType = status.AutoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START;

				if (! ChangeServiceConfig(
					hService,			// handle of service
					SERVICE_NO_CHANGE,	// service type: no change
					dwStartType,		// change service start type
					SERVICE_NO_CHANGE,	// error control: no change
					NULL,				// binary path: no change
					NULL,				// load order group: no change
					NULL,				// tag ID: no change
					NULL,				// dependencies: no change
					NULL,				// account name: no change
					NULL,				// password: no change
					display_name ) )
				{
					HandleError(false, "ChangeServiceConfig in SetAutoStart");
				}
			}
			else
				HandleError(false, "OpenService in SetAutoStart");

			// Release the database lock.
			UnlockServiceDatabase(sclLock);

			// Close the handle to the service.
			CloseServiceHandle(hService);
		}

		CloseServiceManager();

	}
	else
	{
		ConfigureRegistryForApp( status );
	}
}


bool CFBDialog::FirebirdRunning()
/*
 * Check to see if Firebird is running as an application.
 */
{
	const bool result = (bool) GetFirebirdHandle();
	bool guardian_running = (bool) GetGuardianHandle();

	if (result && guardian_running)
		fb_status.UseGuardian = guardian_running;

	return result;
}


HWND CFBDialog::GetFirebirdHandle() const
{
	HWND result = GetSuperServerHandle();
#ifdef MANAGE_CLASSIC
	if ( !result )
	{
		result = GetClassicServerHandle();
	}
#endif
	return result;
}


HWND CFBDialog::GetSuperServerHandle() const
{
	return ::FindWindow(szClassName, szWindowName);
}


#ifdef MANAGE_CLASSIC
HWND CFBDialog::GetClassicServerHandle() const
{
	// oops - hard-coded string that is liable to change
	// Plus, the original definition is hidden locally
	// within a function in remote/windows (or whatever).
	return ::FindWindow( "FB_Disabled", szWindowName);
}
#endif

HWND CFBDialog::GetGuardianHandle() const
{
	return ::FindWindow(GUARDIAN_CLASS_NAME, GUARDIAN_APP_LABEL);
}


bool CFBDialog::ServiceSupportAvailable() const
{
	OSVERSIONINFO OsVersionInfo;
	ZeroMemory(&OsVersionInfo, sizeof(OsVersionInfo));

	// need to set the sizeof this structure for NT to work
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&OsVersionInfo);

	// true for NT family, false for 95 family
	return (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}


bool CFBDialog::OpenServiceManager( DWORD DesiredAccess )
{
	if (!fb_status.ServicesAvailable)
		return false;

	if (DesiredAccess == 0)
		DesiredAccess = GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE;

	// If the svc mgr is already opened, this function will return true since
	// it has no way to know if the new access is the same that opened the svc mgr
	// previously.
	if (hScManager == NULL)
		hScManager = OpenSCManager (NULL, SERVICES_ACTIVE_DATABASE, DesiredAccess );

	return hScManager ? true : false;
}


void CFBDialog::CloseServiceManager()
{
	if (hScManager == NULL)
		return;

	try
	{
		CloseServiceHandle (hScManager);
	}
	catch (...)
	{
		// Err... what do we do now?
	}

	hScManager = NULL;
}


void CFBDialog::GetFullAppPath( CFBDialog::STATUS status, char * app)
// This returns the fully qualified path and name of the application
// to start along with parameters -a and -c .
{
	CString AppName = m_Root_Path;

	if ( status.UseGuardian )
	{
		AppName += m_Guardian_Name;

#ifdef MANAGE_CLASSIC
		if (status.UseClassic)
		{
			AppName += ".exe -c -a";
		}
		else
#endif
		{
			AppName += ".exe -a";
		}
	}
	else
	{
		GetServerName( status, AppName );
	}

	::strcat(app, AppName);
}


void CFBDialog::GetServerName( CFBDialog::STATUS status, CString& AppName) const
{
#ifdef MANAGE_CLASSIC
	if ( status.UseClassic )
	{
		AppName += m_CS_Server_Name;
	}
	else
#endif
	{
		AppName += m_SS_Server_Name;
	}

	AppName += ".exe -a";
}


#ifdef MANAGE_CLASSIC
bool CFBDialog::GetPreferredArchitecture()
{
	int option;
	if (!initialised)
		option = Config::getPreferredArchitecture();
	else
		if (new_settings.UseClassic != fb_status.UseClassic)
			option = new_settings.UseClassic;
		else
			option = fb_status.UseClassic;

	return (bool) option;
}
#endif


bool CFBDialog::UserHasSufficientRights()
{
	bool HasRights = OpenServiceManager(0);
	CloseServiceManager();
	return HasRights;
}
