/*
 *  PROGRAM:        Firebird 2.0 control panel applet
 *  MODULE:         fbpanel.h
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


#if !defined(UTILITIES_FBPANEL_H)
#define UTILITIES_FBPANEL_H

#include "ctrlpan.h"

class CFBPanel : public CControlPanel
{
public:
    virtual LONG OnInquire(UINT uAppNum, NEWCPLINFO* pInfo);
    virtual LONG OnDblclk(HWND hwndCPl, UINT uAppNum, LONG lData);

};

#endif // UTILITIES_FBPANEL_H

