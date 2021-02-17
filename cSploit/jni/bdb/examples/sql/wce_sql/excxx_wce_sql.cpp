/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */


// excxx_wce_sql.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "excxx_wce_sql.h"
#include "excxx_wce_sqlDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cexcxx_wce_sqlApp

BEGIN_MESSAGE_MAP(Cexcxx_wce_sqlApp, CWinApp)
END_MESSAGE_MAP()


// Cexcxx_wce_sqlApp construction
Cexcxx_wce_sqlApp::Cexcxx_wce_sqlApp()
	: CWinApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only Cexcxx_wce_sqlApp object
Cexcxx_wce_sqlApp theApp;

// Cexcxx_wce_sqlApp initialization

BOOL Cexcxx_wce_sqlApp::InitInstance()
{
    // SHInitExtraControls should be called once during your application's initialization to initialize any
    // of the Windows Mobile specific controls such as CAPEDIT and SIPPREF.
    SHInitExtraControls();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	Cexcxx_wce_sqlDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
