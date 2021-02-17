// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <ldap.h>

#include "debug.h"

#include "LDAPBindRequest.h"
#include "LDAPException.h"
#include "SaslInteractionHandler.h"
#include "SaslInteraction.h"

#include <cstdlib>
#include <sasl/sasl.h>

using namespace std;

LDAPBindRequest::LDAPBindRequest(const LDAPBindRequest& req) :
        LDAPRequest(req){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPBindRequest::LDAPBindRequest(&)" << endl);
    m_dn=req.m_dn;
    m_cred=req.m_cred;
    m_mech=req.m_mech;
}

LDAPBindRequest::LDAPBindRequest(const string& dn,const string& passwd, 
        LDAPAsynConnection *connect, const LDAPConstraints *cons,
        bool isReferral) : LDAPRequest(connect, cons, isReferral){
   DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPBindRequest::LDAPBindRequest()" << endl);
   DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER, "   dn:" << dn << endl
           << "   passwd:" << passwd << endl);
    m_dn = dn;
    m_cred = passwd;
    m_mech = "";
}

LDAPBindRequest::~LDAPBindRequest(){
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPBindRequest::~LDAPBindRequest()" << endl);
}

LDAPMessageQueue* LDAPBindRequest::sendRequest(){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPBindRequest::sendRequest()" << endl);
    int msgID=0;
    
    const char* mech = (m_mech == "" ? 0 : m_mech.c_str());
    BerValue* tmpcred=0;
    if(m_cred != ""){
        char* tmppwd = (char*) malloc( (m_cred.size()+1) * sizeof(char));
        m_cred.copy(tmppwd,string::npos);
        tmppwd[m_cred.size()]=0;
        tmpcred=ber_bvstr(tmppwd);
    }else{
        tmpcred=(BerValue*) malloc(sizeof(BerValue));
        tmpcred->bv_len=0;
        tmpcred->bv_val=0;
    }
    const char* dn = 0;
    if(m_dn != ""){
        dn = m_dn.c_str();
    }
    LDAPControl** tmpSrvCtrls=m_cons->getSrvCtrlsArray();
    LDAPControl** tmpClCtrls=m_cons->getClCtrlsArray();
    int err=ldap_sasl_bind(m_connection->getSessionHandle(),dn, 
            mech, tmpcred, tmpSrvCtrls, tmpClCtrls, &msgID);
    LDAPControlSet::freeLDAPControlArray(tmpSrvCtrls);
    LDAPControlSet::freeLDAPControlArray(tmpClCtrls);
    ber_bvfree(tmpcred);

    if(err != LDAP_SUCCESS){
        throw LDAPException(err);
    }else{
        m_msgID=msgID;
        return new LDAPMessageQueue(this);
    }
}

LDAPSaslBindRequest::LDAPSaslBindRequest(const std::string& mech,
        const std::string& cred, 
        LDAPAsynConnection *connect,
        const LDAPConstraints *cons, 
        bool isReferral) : LDAPRequest(connect, cons, isReferral),m_mech(mech), m_cred(cred) {}

LDAPMessageQueue* LDAPSaslBindRequest::sendRequest()
{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPSaslBindRequest::sendRequest()" << endl);
    int msgID=0;
    
    BerValue tmpcred;
    tmpcred.bv_val = (char*) malloc( m_cred.size() * sizeof(char));
    m_cred.copy(tmpcred.bv_val,string::npos);
    tmpcred.bv_len = m_cred.size();
    
    LDAPControl** tmpSrvCtrls=m_cons->getSrvCtrlsArray();
    LDAPControl** tmpClCtrls=m_cons->getClCtrlsArray();
    int err=ldap_sasl_bind(m_connection->getSessionHandle(), "", m_mech.c_str(), 
            &tmpcred, tmpSrvCtrls, tmpClCtrls, &msgID);
    LDAPControlSet::freeLDAPControlArray(tmpSrvCtrls);
    LDAPControlSet::freeLDAPControlArray(tmpClCtrls);
    free(tmpcred.bv_val);

    if(err != LDAP_SUCCESS){
        throw LDAPException(err);
    }else{
        m_msgID=msgID;
        return new LDAPMessageQueue(this);
    }
}

LDAPSaslBindRequest::~LDAPSaslBindRequest()
{
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPSaslBindRequest::~LDAPSaslBindRequest()" << endl);
}

LDAPSaslInteractiveBind::LDAPSaslInteractiveBind( const std::string& mech, 
        int flags, SaslInteractionHandler *sih, LDAPAsynConnection *connect,
        const LDAPConstraints *cons, bool isReferral) : 
            LDAPRequest(connect, cons, isReferral),
            m_mech(mech), m_flags(flags), m_sih(sih), m_res(0)
{
}

static int my_sasl_interact(LDAP *l, unsigned flags, void *cbh, void *interact)
{
    DEBUG(LDAP_DEBUG_TRACE, "LDAPSaslInteractiveBind::my_sasl_interact()" 
            << std::endl );
    std::list<SaslInteraction*> interactions;

    sasl_interact_t *iter = (sasl_interact_t*) interact;
    while ( iter->id != SASL_CB_LIST_END ) {
        SaslInteraction *si = new SaslInteraction(iter);
        interactions.push_back( si );
        iter++;
    }
    ((SaslInteractionHandler*)cbh)->handleInteractions(interactions);
    return LDAP_SUCCESS;
}

/* This kind of fakes an asynchronous operation, ldap_sasl_interactive_bind_s
 * is synchronous */
LDAPMessageQueue *LDAPSaslInteractiveBind::sendRequest()
{
    DEBUG(LDAP_DEBUG_TRACE, "LDAPSaslInteractiveBind::sendRequest()" <<
            m_mech << std::endl);

    LDAPControl** tmpSrvCtrls=m_cons->getSrvCtrlsArray();
    LDAPControl** tmpClCtrls=m_cons->getClCtrlsArray();
    int res = ldap_sasl_interactive_bind_s( m_connection->getSessionHandle(),
            "", m_mech.c_str(), tmpSrvCtrls, tmpClCtrls, m_flags, 
            my_sasl_interact, m_sih );

    DEBUG(LDAP_DEBUG_TRACE, "ldap_sasl_interactive_bind_s returned: " 
            << res << std::endl);
    if(res != LDAP_SUCCESS){
        throw LDAPException(res);
    } else {
        m_res = new LDAPResult(LDAPMsg::BIND_RESPONSE, res, ""); 
    }
    return new LDAPMessageQueue(this);
}

LDAPMsg* LDAPSaslInteractiveBind::getNextMessage() const 
{
    return m_res;
}

LDAPSaslInteractiveBind::~LDAPSaslInteractiveBind()
{
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPSaslInteractiveBind::~LDAPSaslInteractiveBind()" << endl);
}

