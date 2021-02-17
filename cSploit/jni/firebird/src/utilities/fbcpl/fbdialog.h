/*
 *	PROGRAM:	Firebird 2.0 control panel applet
 *	MODULE:		FBDialog.h
 *	DESCRIPTION:	Main file to provide GUI based server control functions
 *					for Firebird 2.0
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


/////////////////////////////////////////////////////////////////////////////
// CFBDialog dialog

#if !defined(UTILITIES_FBDIALOG_H)
#define UTILITIES_FBDIALOG_H

//#pragma once

#define FB_COMPILER_MESSAGE_STR(x) #x
#define FB_COMPILER_MESSAGE_STR2(x)   FB_COMPILER_MESSAGE_STR(x)
#define FB_COMPILER_MESSAGE(desc) message(__FILE__ "("	\
									FB_COMPILER_MESSAGE_STR2(__LINE__) "):" desc)

#include "resource.h"		// main symbols
#include "res/fbcontrol.rc2"

#include <winsvc.h>

#include "../install/install_nt.h"
#include "../install/servi_proto.h"
#include "../install/registry.h"
#include "../../remote/server/os/win32/window.h"
#include "../../iscguard/iscguard.h"

extern USHORT svc_error (SLONG, const TEXT*, SC_HANDLE);


class CFBDialog : public CDialog
{
// Construction
public:
	CFBDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFBDialog)
	enum { IDD = IDD_FBDIALOG };
	CButton	m_Server_Arch;
	CButton	m_Super_Server;
	CButton	m_Classic_Server;
	CButton	m_Apply;
	CButton	m_Use_Guardian;
	CButton	m_Manual_Start;
	CButton	m_Run_As_Application;
	CButton	m_Run_As_Service;
	CButton	m_Auto_Start;
	CButton	m_Run_Type;
	CButton	m_Button_Stop;
	CStatic	m_Icon;
	CString	m_FB_Version;
	CString	m_Firebird_Status;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFBDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL



// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFBDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonStop();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnService();
	afx_msg void OnManualStart();
	afx_msg void OnApplication();
	afx_msg void OnAutoStart();
	afx_msg void OnUseGuardian();
	afx_msg void OnApply();
	afx_msg void OnSuperServer();
	afx_msg void OnClassicServer();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

//========= End of MSVC specific stuff

//Our Stuff
public:
	CString m_SS_Server_Name;
	CString m_Guardian_Name;
	CString m_Root_Path;
	CString m_ServerClassName;
#ifdef MANAGE_CLASSIC
	CString m_ClassicClassName;
	CString m_CS_Server_Name;
#endif
	bool m_Reset_Display_To_Existing_Values;
	bool m_Restore_old_status;

	UINT m_uiTimer;
	SC_HANDLE hScManager;

    SERVICE_STATUS service_status;

	DWORD m_Error_Status;			//This is set by the calls to SERVICES_
									//and is also set by GetLastError()
									//It is tested in ShowError to prevent
									//the same error message being reported.

	struct STATUS					//This stores the current status
	{
#ifdef MANAGE_CLASSIC
		bool UseClassic;
#endif
		bool UseGuardian;
		bool ServicesAvailable;		// Set via UpdateServerStatus()
		int  ServerStatus;
		bool UseService;			// This is a convenience. It is set when
									// ServiceStatus is checked and saves trying
									// to do the more complex evaluation of
									// ServiceStatus
		bool AutoStart;
		bool WasRunning;			// Set via UpdateServerStatus(). Allows us
									// to check if server was running before we
									// updated our settings.
		bool SystemLogin;			// Are we using LocalSystem to control the
									// service
		bool SufficientUserRights;	// Does user have sufficient rights to change service

		CString ServerName;			// Initially set by call to ViewRegistryEntries
		CString ServiceExecutable;	// Initially set by call to ViewRegistryEntries

	} 	fb_status;


	STATUS new_settings;

	bool initialised;				// False on startup
//Check stuff
	bool CheckServiceInstalled( LPCTSTR service );
	int DatabasesConnected();

	//bool FirebirdInstalled();  Not implemented?
	bool FirebirdRunning();

//Get Stuff

	CString GetServiceName(const char* name) const;
	HWND GetSuperServerHandle() const;
#ifdef MANAGE_CLASSIC
	HWND GetClassicServerHandle() const;
#endif
	HWND GetFirebirdHandle() const;
	void GetFullAppPath( CFBDialog::STATUS status, char* app);
	HWND GetGuardianHandle() const;
//	bool GetGuardianUseSpecified();
	bool GetPreferredArchitecture();
	void GetServerName( CFBDialog::STATUS status, CString& AppName) const;
	int GetServerStatus();

	bool ServiceSupportAvailable() const;
	void ViewRegistryEntries();

//Set stuff
	bool ConfigureRegistryForApp(const CFBDialog::STATUS status );
	void SetAutoStart(const CFBDialog::STATUS status );
#ifdef FBCPL_UPDATE_CONF
	void SetGuardianUseInConf( bool UseGuardian );
#endif
#ifdef MANAGE_CLASSIC
	void SetPreferredArchitectureInConf( bool UseClassic );
#endif
#ifdef FBCPL_UPDATE_CONF
	bool UpdateFirebirdConf(CString option, CString value);
#endif


//Do stuff
	void ApplyChanges();
	bool AppInstall(const CFBDialog::STATUS status );
	bool AppRemove();
	void CloseServiceManager();
	void DisableApplyButton();
	void EnableApplyButton();
	void KillApp();
	bool OpenServiceManager( DWORD DesiredAccess );
	void ProcessMessages();
	void ResetCheckBoxes(const CFBDialog::STATUS status );
	bool ServerStop();
	bool ServerStart(const CFBDialog::STATUS status );
	bool ServiceInstall(CFBDialog::STATUS status );
	bool ServiceRemove();
	static void HandleSvcError(SLONG status, const TEXT* string);
	void HandleError(bool silent, const TEXT* string );
	void ShowError( LPTSTR lpMsgBuf, CString error_title );
	void UpdateServerStatus();
	bool UserHasSufficientRights();
	bool ValidateInstalledServices();
};


#endif // UTILITIES_FBDIALOG_H

