// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "config.h"
#include "ac/time.h"
#include "debug.h"
#include "LDAPSearchRequest.h"
#include "LDAPException.h"
#include "LDAPSearchReference.h"
#include "LDAPResult.h"
#include "LDAPRequest.h"
#include "LDAPUrl.h"

using namespace std;

LDAPSearchRequest::LDAPSearchRequest(const LDAPSearchRequest& req ) :
        LDAPRequest (req){
    DEBUG(LDAP_DEBUG_CONSTRUCT, 
        "LDAPSearchRequest::LDAPSearchRequest(&)" << endl);
    m_base=req.m_base;
    m_scope=req.m_scope;
    m_filter=req.m_filter;
    m_attrs=req.m_attrs;
    m_attrsOnly=req.m_attrsOnly;
}
        

LDAPSearchRequest::LDAPSearchRequest(const string& base, int scope, 
        const string& filter, const StringList& attrs, bool attrsOnly,
        LDAPAsynConnection *connect,
        const LDAPConstraints* cons, bool isReferral, 
        const LDAPRequest* parent) 
            : LDAPRequest (connect,cons,isReferral,parent) {
    
    DEBUG(LDAP_DEBUG_CONSTRUCT,
            "LDAPSearchRequest:LDAPSearchRequest()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT & LDAP_DEBUG_PARAMETER,
            "   base:" << base << endl << "   scope:" << scope << endl
            << "   filter:" << filter << endl);
    m_requestType=LDAPRequest::SEARCH;
    //insert some validating and copying here  
    m_base=base;
    m_scope=scope;
    if(filter == ""){
        m_filter="objectClass=*";  
    }else{
        m_filter=filter;
    }
    m_attrs=attrs;
    m_attrsOnly=attrsOnly;
}

LDAPSearchRequest::~LDAPSearchRequest(){
    DEBUG(LDAP_DEBUG_DESTROY, "LDAPSearchRequest::~LDAPSearchRequest" << endl);
}

LDAPMessageQueue* LDAPSearchRequest::sendRequest(){
    int msgID; 
    DEBUG(LDAP_DEBUG_TRACE, "LDAPSearchRequest::sendRequest()" << endl);
    timeval* tmptime=m_cons->getTimeoutStruct();
    char** tmpattrs=m_attrs.toCharArray();
    LDAPControl** tmpSrvCtrl=m_cons->getSrvCtrlsArray();
    LDAPControl** tmpClCtrl=m_cons->getClCtrlsArray();
    int aliasDeref = m_cons->getAliasDeref();
    ldap_set_option(m_connection->getSessionHandle(), LDAP_OPT_DEREF, 
            &aliasDeref);
    int err=ldap_search_ext(m_connection->getSessionHandle(), m_base.c_str(),
            m_scope, m_filter.c_str(), tmpattrs, m_attrsOnly, tmpSrvCtrl,
            tmpClCtrl, tmptime, m_cons->getSizeLimit(), &msgID );
    delete tmptime;
    ber_memvfree((void**)tmpattrs);
    LDAPControlSet::freeLDAPControlArray(tmpSrvCtrl);
    LDAPControlSet::freeLDAPControlArray(tmpClCtrl);

    if (err != LDAP_SUCCESS){  
        throw LDAPException(err);
    } else if (isReferral()){
        m_msgID=msgID;
        return 0;
    }else{
        m_msgID=msgID;
        return  new LDAPMessageQueue(this);
    }
}

LDAPRequest* LDAPSearchRequest::followReferral(LDAPMsg* ref){
    DEBUG(LDAP_DEBUG_TRACE, "LDAPSearchRequest::followReferral()" << endl);
    LDAPUrlList urls;
    LDAPUrlList::const_iterator usedUrl;
    LDAPAsynConnection* con;
    string filter;
    int scope;
    if(ref->getMessageType() == LDAPMsg::SEARCH_REFERENCE){
        urls = ((LDAPSearchReference *)ref)->getUrls();
    }else{
        urls = ((LDAPResult *)ref)->getReferralUrls();
    }
    con = getConnection()->referralConnect(urls,usedUrl,m_cons);
    if(con != 0){
        if((usedUrl->getFilter() != "") && 
            (usedUrl->getFilter() != m_filter)){
                filter=usedUrl->getFilter();
        }else{
            filter=m_filter;
        }
        if( (ref->getMessageType() == LDAPMsg::SEARCH_REFERENCE) && 
            (m_scope == LDAPAsynConnection::SEARCH_ONE)
          ){
            scope = LDAPAsynConnection::SEARCH_BASE;
            DEBUG(LDAP_DEBUG_TRACE,"   adjusted scope to BASE" << endl);
        }else{
            scope = m_scope;
        }
    }else{
        return 0;
    }
    return new LDAPSearchRequest(usedUrl->getDN(), scope, filter,
            m_attrs, m_attrsOnly, con, m_cons,true,this);
}

bool LDAPSearchRequest::equals(const LDAPRequest* req)const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPSearchRequest::equals()" << endl);
    if( LDAPRequest::equals(req)){
        LDAPSearchRequest* sreq = (LDAPSearchRequest*)req;
        if ( (m_base == sreq->m_base) &&
             (m_scope == sreq->m_scope) 
           ){
            return true;
        }
    }
    return false;
}
