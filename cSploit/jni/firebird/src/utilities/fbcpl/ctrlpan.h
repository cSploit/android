// CtrlPan.h

#if !defined(UTILITIES_CTRLPAN_H)
#define UTILITIES_CTRLPAN_H

#include <cpl.h> // control panel definitions

class CControlPanel
{
public:
    CControlPanel();
    virtual ~CControlPanel();

    // Event handlers
    virtual LONG OnDblclk(HWND hwndCPl, UINT uAppNum, LONG lData);
    virtual LONG OnExit();
    virtual LONG OnGetCount();
    virtual LONG OnInit();
    virtual LONG OnInquire(UINT uAppNum, NEWCPLINFO* pInfo);
    virtual LONG OnSelect(UINT uAppNum, LONG lData);
    virtual LONG OnStop(UINT uAppNum, LONG lData);

    // static member functions (callbacks)
    static LONG APIENTRY CPlApplet(HWND hwndCPl, UINT  uMsg, LPARAM lParam1, LPARAM lParam2);

    // static data
    static CControlPanel* m_pThis; // nasty hack to get object ptr
};

#endif // UTILITIES_CTRLPAN_H

