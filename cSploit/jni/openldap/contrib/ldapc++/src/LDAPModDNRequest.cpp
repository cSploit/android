// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <ldap.h>

#include "debug.h"

#include "LDAPModDNRequest.h"
#include "LDAPException.h"
#include "LDAPResult.h"
#include "LDAPUrlList.h"

using namespace std;

LDAPModDNRequest::LDAPModDNRequest(const LDAPModDNRequest& req) :
        LDAPRequest(req){
    DEBUG(LDAP_DEBUG_CONSTRUCT, 
            "LDAPModDNRequest::LDAPModDNRequest(&)" << endl);
    m_dn = req.m_dn;
    m_newRDN = req.m_newRDN;
    m_newParentDN = req.m_newParentDN;
    m_deleteOld = req.m_deleteOld;
}

LDAPModDNRequest::LDAPModDNRequest(const string& dn, const string& newRDN, 
        bool deleteOld, const string& newParentDN, 
        LDAPAsynConnection *connect, 
        const LDAPConstraints *cons, bool isReferral, 
        const LDAPRequest* parent):
        LDAPRequest(connect, cons, isReferral, parent){
    DEBUG(LDAP_DEBUG_CONSTRUCT, 
            "LDAPModDNRequest::LDAPModDNRequest(&)" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER, 
            "   dn:" << dn << endl << "   newRDN:" << newRDN << endl
            << "   deleteOld:" << deleteOld << endl 
            << "   newParent:" << newParentDN << endl);
    m_dn = dn;
    m_newRDN = newRDN;
    m_newParentDN = newParentDN;
    m_deleteOld=deleteOld;
}

LDAPModDNRequest::~LDAPModDNRequest(){
    DEBUG(LDAP_DEBUG_DESTROY, "LDAPModDNRequest::~LDAPModDNRequest()" << endl);
}

LDAPMessageQueue* LDAPModDNRequest::sendRequest(){
    DEBUG(LDAP_DEBUG_TRACE, "LDAPModDNRequest::sendRequest()" << endl);
    int msg_id;
    const char* newRDN = (m_newRDN == "" ? 0 :m_newRDN.c_str());
    const char* newParentDN = (m_newParentDN == "" ? 
            0 :
            m_newParentDN.c_str());
    LDAPControl** tmpSrvCtrls=m_cons->getSrvCtrlsArray();
    LDAPControl** tmpClCtrls=m_cons->getClCtrlsArray();
    int err=ldap_rename(m_connection->getSessionHandle(),m_dn.c_str(),newRDN,
            newParentDN,m_deleteOld ? 1 : 0, tmpSrvCtrls, tmpClCtrls,&msg_id);
    LDAPControlSet::freeLDAPControlArray(tmpSrvCtrls);
    LDAPControlSet::freeLDAPControlArray(tmpClCtrls);
    if(err!=LDAP_SUCCESS){
        throw LDAPException(err);
    }else{
        m_msgID=msg_id;
        return new LDAPMessageQueue(this);
    }
}

LDAPRequest* LDAPModDNRequest::followReferral(LDAPMsg* ref){
    DEBUG(LDAP_DEBUG_TRACE, "LDAPModifyRequest::followReferral()" << endl);
    LDAPUrlList::const_iterator usedUrl;
    LDAPUrlList urls = ((LDAPResult*)ref)->getReferralUrls();
    LDAPAsynConnection* con = 0;
    try {
        con = getConnection()->referralConnect(urls,usedUrl,m_cons);
    } catch(LDAPException e){
        delete con;
        return 0;
    }
    if(con != 0){
        return new LDAPModDNRequest(m_dn, m_newRDN, m_deleteOld, m_newParentDN,
                con, m_cons,true,this);
    }
    return 0;
}

