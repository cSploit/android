// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <ldap.h>

#include "debug.h"

#include "LDAPDeleteRequest.h"
#include "LDAPException.h"
#include "LDAPMessageQueue.h"
#include "LDAPResult.h"

using namespace std;

LDAPDeleteRequest::LDAPDeleteRequest( const LDAPDeleteRequest& req) :
        LDAPRequest(req){
	DEBUG(LDAP_DEBUG_CONSTRUCT, 
		"LDAPDeleteRequest::LDAPDeleteRequest(&)" << endl);
    m_dn = req.m_dn;
}

LDAPDeleteRequest::LDAPDeleteRequest(const string& dn, 
        LDAPAsynConnection *connect, const LDAPConstraints *cons,
        bool isReferral, const LDAPRequest* parent) 
        : LDAPRequest(connect, cons, isReferral, parent) {
	DEBUG(LDAP_DEBUG_CONSTRUCT,
            "LDAPDeleteRequest::LDAPDeleteRequest()" << endl);
	DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER, "   dn:" << dn << endl);
    m_requestType=LDAPRequest::DELETE;
    m_dn=dn;
}

LDAPDeleteRequest::~LDAPDeleteRequest(){
    DEBUG(LDAP_DEBUG_DESTROY,
          "LDAPDeleteRequest::~LDAPDeleteRequest()" << endl);
}

LDAPMessageQueue* LDAPDeleteRequest::sendRequest(){
	DEBUG(LDAP_DEBUG_TRACE, "LDAPDeleteRequest::sendRequest()" << endl);
    int msgID=0;
    LDAPControl** tmpSrvCtrls=m_cons->getSrvCtrlsArray();
    LDAPControl** tmpClCtrls=m_cons->getClCtrlsArray();
    int err=ldap_delete_ext(m_connection->getSessionHandle(),m_dn.c_str(), 
            tmpSrvCtrls, tmpClCtrls ,&msgID);
    LDAPControlSet::freeLDAPControlArray(tmpSrvCtrls);
    LDAPControlSet::freeLDAPControlArray(tmpClCtrls);
    if(err != LDAP_SUCCESS){
        throw LDAPException(err);
    }else{
        m_msgID=msgID;
        return new LDAPMessageQueue(this);
    }
}

LDAPRequest* LDAPDeleteRequest::followReferral(LDAPMsg* refs){
	DEBUG(LDAP_DEBUG_TRACE, "LDAPDeleteRequest::followReferral()" << endl);
    LDAPUrlList::const_iterator usedUrl;
    LDAPUrlList urls= ((LDAPResult*)refs)->getReferralUrls();
    LDAPAsynConnection* con=0;
    try{
        con = getConnection()->referralConnect(urls,usedUrl,m_cons);
    }catch (LDAPException e){
        delete con;
        return 0;
    }
    if(con != 0){
        return new LDAPDeleteRequest(m_dn, con, m_cons, true, this);
    }
    return 0;
}


