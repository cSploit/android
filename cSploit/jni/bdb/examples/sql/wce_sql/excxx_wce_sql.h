/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

// excxx_wce_sql.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#ifdef POCKETPC2003_UI_MODEL
#include "resourceppc.h"
#endif 

// Cexcxx_wce_sqlApp:
// See excxx_wce_sql.cpp for the implementation of this class
//

class Cexcxx_wce_sqlApp : public CWinApp
{
public:
	Cexcxx_wce_sqlApp();
	
// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Cexcxx_wce_sqlApp theApp;
