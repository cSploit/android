// CtrlPan.cpp
// Source for CControlPanel

#include "stdafx.h"
#include "ctrlpan.h"

// static data
CControlPanel* CControlPanel::m_pThis = NULL;

CControlPanel::CControlPanel()
{
    // Set up the static object pointer
    m_pThis = this;
}

CControlPanel::~CControlPanel()
{
}

//////////////////////////////////////////////////////////////////////////////
// Callback function (exported)

// static member functions (callbacks)
LONG APIENTRY CControlPanel::CPlApplet(HWND hwndCPl, UINT  uMsg, LPARAM lParam1, LPARAM lParam2)
{


	// Get a pointer to the C++ object
    CControlPanel* pCtrl = m_pThis;
    ASSERT(pCtrl);

    switch (uMsg)
    {
    case CPL_DBLCLK:
        return pCtrl->OnDblclk(hwndCPl, lParam1, lParam2);

    case CPL_EXIT:
        return pCtrl->OnExit();

    case CPL_GETCOUNT:
        return pCtrl->OnGetCount();

    case CPL_INIT:
        return pCtrl->OnInit();

    case CPL_NEWINQUIRE:
        return pCtrl->OnInquire(lParam1, (NEWCPLINFO*)lParam2);

    case CPL_INQUIRE:
        return 0; // not handled

    case CPL_SELECT:
        return pCtrl->OnSelect(lParam1, lParam2);

    case CPL_STOP:
        return pCtrl->OnStop(lParam1, lParam2);

    default:
        break;
    }

    return 1; // not processed
}

/////////////////////////////////////////////////////////////////////////////////////////
// Default command handlers

LONG CControlPanel::OnDblclk(HWND /*hwndCPl*/, UINT /*uAppNum*/, LONG /*lData*/)
{
    // Show the dialog
    return 0; // OK
}

LONG CControlPanel::OnExit()
{
    return 0; // OK
}

LONG CControlPanel::OnGetCount()
{
    return 1; // default is support for one dialog box
}

LONG CControlPanel::OnInit()
{
    return 1; // OK
}

LONG CControlPanel::OnInquire(UINT /*uAppNum*/, NEWCPLINFO* pInfo)
{
    // Fill in the data
    pInfo->dwSize = sizeof(NEWCPLINFO); // important
    pInfo->dwFlags = 0;
    pInfo->dwHelpContext = 0;
    pInfo->lData = 0;
    pInfo->hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(1));
    strcpy(pInfo->szName, "Applet");
    strcpy(pInfo->szInfo, "Raw Cpl Applet");
    strcpy(pInfo->szHelpFile, "");
    return 0; // OK (don't send CPL_INQUIRE msg)
}

LONG CControlPanel::OnSelect(UINT /*uAppNum*/, LONG /*lData*/)
{
    return 1; // not handled
}

LONG CControlPanel::OnStop(UINT /*uAppNum*/, LONG /*lData*/)
{
    return 1; // not handled
}
