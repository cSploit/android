// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <ldap.h>

#include "debug.h"

#include "LDAPCompareRequest.h"
#include "LDAPException.h"
#include "LDAPMessageQueue.h"
#include "LDAPResult.h"

using namespace std;

LDAPCompareRequest::LDAPCompareRequest(const LDAPCompareRequest& req){
    DEBUG(LDAP_DEBUG_CONSTRUCT, 
            "LDAPCompareRequest::LDAPCompareRequest(&)" << endl);
    m_dn=req.m_dn;
    m_attr=req.m_attr;
}

LDAPCompareRequest::LDAPCompareRequest(const string& dn, 
        const LDAPAttribute& attr, LDAPAsynConnection *connect, 
        const LDAPConstraints *cons, bool isReferral, 
        const LDAPRequest* parent) :
        LDAPRequest(connect, cons, isReferral,parent){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPCompareRequest::LDAPCompareRequest()" 
            << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER, "   dn:" << dn << endl 
            << "   attr:" << attr << endl);
    m_requestType=LDAPRequest::COMPARE;
    m_dn=dn;
    m_attr=attr;
} 
    
LDAPCompareRequest::~LDAPCompareRequest(){
    DEBUG(LDAP_DEBUG_DESTROY, "LDAPCompareRequest::~LDAPCompareRequest()" 
            << endl);
}

LDAPMessageQueue* LDAPCompareRequest::sendRequest(){
    DEBUG(LDAP_DEBUG_TRACE, "LDAPCompareRequest::sendRequest()" << endl);
    int msgID=0;
    BerValue **val=m_attr.getBerValues();
    LDAPControl** tmpSrvCtrls=m_cons->getSrvCtrlsArray(); 
    LDAPControl** tmpClCtrls=m_cons->getClCtrlsArray(); 
    int err=ldap_compare_ext(m_connection->getSessionHandle(),m_dn.c_str(),
            m_attr.getName().c_str(), val[0], tmpSrvCtrls, 
            tmpClCtrls, &msgID);
    ber_bvecfree(val);
    LDAPControlSet::freeLDAPControlArray(tmpSrvCtrls);
    LDAPControlSet::freeLDAPControlArray(tmpClCtrls);
    if(err != LDAP_SUCCESS){
        throw LDAPException(err);
    }else{
        m_msgID=msgID;
        return new LDAPMessageQueue(this);
    }
}

LDAPRequest* LDAPCompareRequest::followReferral(LDAPMsg* ref){
	DEBUG(LDAP_DEBUG_TRACE, "LDAPCompareRequest::followReferral()" << endl);
    LDAPUrlList::const_iterator usedUrl;
    LDAPUrlList urls = ((LDAPResult*)ref)->getReferralUrls();
    LDAPAsynConnection* con = 0;
    try{
        con=getConnection()->referralConnect(urls,usedUrl,m_cons);
    }catch(LDAPException e){
        return 0;
    }
    if(con != 0){
        return new LDAPCompareRequest(m_dn, m_attr, con, m_cons, true, this);
    }
    return 0;
}

