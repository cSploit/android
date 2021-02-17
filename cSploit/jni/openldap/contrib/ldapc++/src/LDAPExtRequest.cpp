/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <ldap.h>
#include <lber.h>

#include "debug.h"

#include "LDAPExtRequest.h"
#include "LDAPException.h"
#include "LDAPResult.h"

#include <cstdlib>

using namespace std;

LDAPExtRequest::LDAPExtRequest(const LDAPExtRequest& req) :
        LDAPRequest(req){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPExtRequest::LDAPExtRequest(&)" << endl);
    m_data=req.m_data;
    m_oid=req.m_oid;
}

LDAPExtRequest::LDAPExtRequest(const string& oid, const string& data, 
        LDAPAsynConnection *connect, const LDAPConstraints *cons,
        bool isReferral, const LDAPRequest* parent) 
        : LDAPRequest(connect, cons, isReferral, parent){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPExtRequest::LDAPExtRequest()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER, 
            "   oid:" << oid << endl);
    m_oid=oid;
    m_data=data;
}

LDAPExtRequest::~LDAPExtRequest(){
    DEBUG(LDAP_DEBUG_DESTROY, "LDAPExtRequest::~LDAPExtRequest()" << endl);
}

LDAPMessageQueue* LDAPExtRequest::sendRequest(){
    DEBUG(LDAP_DEBUG_TRACE, "LDAPExtRequest::sendRequest()" << endl);
    int msgID=0;
    BerValue* tmpdata=0;
    if(m_data != ""){
        tmpdata=(BerValue*) malloc(sizeof(BerValue));
        tmpdata->bv_len = m_data.size();
        tmpdata->bv_val = (char*) malloc(sizeof(char) * (m_data.size()) );
        m_data.copy(tmpdata->bv_val, string::npos);
    }
    LDAPControl** tmpSrvCtrls=m_cons->getSrvCtrlsArray();
    LDAPControl** tmpClCtrls=m_cons->getClCtrlsArray();
    int err=ldap_extended_operation(m_connection->getSessionHandle(),
            m_oid.c_str(), tmpdata, tmpSrvCtrls, tmpClCtrls, &msgID);
    LDAPControlSet::freeLDAPControlArray(tmpSrvCtrls);
    LDAPControlSet::freeLDAPControlArray(tmpClCtrls);
    ber_bvfree(tmpdata);
    if(err != LDAP_SUCCESS){
        delete this;
        throw LDAPException(err);
    }else{
        m_msgID=msgID;
        return new LDAPMessageQueue(this);
    }
}

LDAPRequest* LDAPExtRequest::followReferral(LDAPMsg* ref){
    DEBUG(LDAP_DEBUG_TRACE, "LDAPExtRequest::followReferral()" << endl);
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
        return new LDAPExtRequest(m_oid, m_data, con, m_cons,true,this);
    }
    return 0;
}


