/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

// excxx_wce_sqlDlg.cpp : implementation file
//

#include "stdafx.h"
#include "excxx_wce_sql.h"
#include "excxx_wce_sqlDlg.h"

#include "sqlite3.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Cexcxx_wce_sqlDlg dialog

Cexcxx_wce_sqlDlg::Cexcxx_wce_sqlDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cexcxx_wce_sqlDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cexcxx_wce_sqlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cexcxx_wce_sqlDlg, CDialog)
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	ON_WM_SIZE()
#endif
	//}}AFX_MSG_MAP
	ON_STN_CLICKED(IDC_STATIC_1, &Cexcxx_wce_sqlDlg::OnStnClickedStatic1)
	ON_BN_CLICKED(IDC_BUTTON1, &Cexcxx_wce_sqlDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &Cexcxx_wce_sqlDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// Cexcxx_wce_sqlDlg message handlers

BOOL Cexcxx_wce_sqlDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void Cexcxx_wce_sqlDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
	if (AfxIsDRAEnabled())
	{
		DRA::RelayoutDialog(
			AfxGetResourceHandle(), 
			this->m_hWnd, 
			DRA::GetDisplayMode() != DRA::Portrait ? 
			MAKEINTRESOURCE(IDD_SIMPLEPROJECT_DIALOG_WIDE) : 
			MAKEINTRESOURCE(IDD_SIMPLEPROJECT_DIALOG));
	}
}
#endif


void Cexcxx_wce_sqlDlg::OnStnClickedStatic1()
{
	// TODO: Add your control notification handler code here
}

void Cexcxx_wce_sqlDlg::OnBnClickedButton1()
{
	wchar_t msg[512];
	sqlite3 *db;
	int rc;

	rc = sqlite3_open("\\My Documents\\test.db", &db);
	if( rc ){
		StringCbPrintfW(msg, 512, 
		    _T("Can't open database: %hs\r\n"), sqlite3_errmsg(db));
		SetDlgItemText(IDC_TEXT_OUT, msg);
	} else
		SetDlgItemText(IDC_TEXT_OUT, _T("Opened the database.\r\n"));
	sqlite3_close(db);
}

int printer(void *dlg, int argc, char **argv, char**azColName) {
	int i, offset;
	wchar_t msg[512];
	memset(msg, 0, 512);
	Cexcxx_wce_sqlDlg *dialog = (Cexcxx_wce_sqlDlg *)dlg;
	for (i = 0; i < argc; i++) {
		dialog->GetDlgItemText(IDC_TEXT_OUT, msg, 512);
		offset = wcslen(msg);
		StringCbPrintfW(msg + offset, 512 - offset,
		    _T("%hs%hs = %hs%hs"), i == 0 ? "" : ", ", azColName[i], 
		    argv[i] ? argv[i] : "NULL", (i + 1 == argc) ? "\r\n" : "");
		dialog->SetDlgItemText(IDC_TEXT_OUT, msg);
	}
	return (0);
}

void Cexcxx_wce_sqlDlg::OnBnClickedButton2()
{
	wchar_t value[512];
	char sql[512], *errors;
	GetDlgItemText(IDC_EDIT2, value, 512);
	if (value[0] == 0) {
		SetDlgItemText(IDC_TEXT_OUT,
		    _T("You must set a value in the text field.\r\n"));
		return;
	}

	wchar_t msg[512];
	sqlite3 *db;
	int rc;

	rc = sqlite3_open("\\My Documents\\test.db", &db);
	if (rc) {
		StringCbPrintfW(msg, 512, 
		    _T("Can't open database: %s\n"), sqlite3_errmsg(db));
		SetDlgItemText(IDC_TEXT_OUT, msg);
		return;
	}

	rc = sqlite3_exec(db, 
	    "CREATE TABLE t1(x INTEGER PRIMARY KEY AUTOINCREMENT, y UNIQUE);",
	    NULL, NULL, &errors);
	if (rc && strcmp(errors, "table t1 already exists") != 0) {
		StringCbPrintfW(msg, 512,
		    _T("sqlite3_exec create failed: %hs\r\n"), errors);
		SetDlgItemText(IDC_TEXT_OUT, msg);
		sqlite3_free(errors);
		goto err;
	}
	_snprintf(sql, 512, "%s%S%s", 
	    "INSERT OR REPLACE INTO t1(y) VALUES (\"", value, "\");");
	rc = sqlite3_exec(db, sql, NULL, NULL, &errors);
	if (rc) {
		StringCbPrintfW(msg, 512,
		    _T("sqlite3_exec insert failed: %hs\r\n"), errors);
		SetDlgItemText(IDC_TEXT_OUT, msg);
		sqlite3_free(errors);
		goto err;
	}
	SetDlgItemText(IDC_TEXT_OUT, _T(""));
	rc = sqlite3_exec(db, "SELECT * FROM t1;", printer, this, &errors);
	if (rc) {
		StringCbPrintfW(msg, 512,
		    _T("sqlite3_exec select failed: %hs\r\n"), errors);
		SetDlgItemText(IDC_TEXT_OUT, msg);
		sqlite3_free(errors);
		goto err;
	} else {
		GetDlgItemText(IDC_TEXT_OUT, msg, 512);
		StringCbPrintfW(msg + wcslen(msg), 512 - wcslen(msg),
		    _T("Insert completed\r\n"));
		SetDlgItemText(IDC_TEXT_OUT, msg);
	}

err:	sqlite3_close(db);

}
