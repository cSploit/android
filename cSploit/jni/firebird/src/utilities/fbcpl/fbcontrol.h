// FBControl.h : main header file for the FBCONTROL DLL
//

#if !defined(AFX_FBCONTROL_H__A4777E98_E00D_11D6_9193_0050564001ED__INCLUDED_)
#define AFX_FBCONTROL_H__A4777E98_E00D_11D6_9193_0050564001ED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif


#include "FBPanel.h"


/////////////////////////////////////////////////////////////////////////////
// CFBControlApp
// See FBControl.cpp for the implementation of this class
//

class CFBControlApp : public CWinApp
{
public:
	CFBControlApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFBControlApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CFBControlApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    CFBPanel m_Control;

};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FBCONTROL_H__A4777E98_E00D_11D6_9193_0050564001ED__INCLUDED_)
