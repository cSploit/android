// $OpenLDAP$
/*
 * Copyright 2007-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "debug.h"
#include <lber.h>
#include "LDAPRequest.h"
#include "LDAPException.h"

#include "LDAPResult.h"
#include "LDAPSaslBindResult.h"

using namespace std;

LDAPSaslBindResult::LDAPSaslBindResult(const LDAPRequest* req, LDAPMessage* msg) :
        LDAPResult(req, msg){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPSaslBindResult::LDAPSaslBindResult()" 
            << std::endl);
    BerValue* data = 0;
    LDAP* lc = req->getConnection()->getSessionHandle();
    int err = ldap_parse_sasl_bind_result(lc, msg, &data, 0);
    if( err != LDAP_SUCCESS && err != LDAP_SASL_BIND_IN_PROGRESS ){
        ber_bvfree(data);
        throw LDAPException(err);
    }else{
        if(data){
            DEBUG(LDAP_DEBUG_TRACE, "   creds present" << std::endl);
            m_creds=string(data->bv_val, data->bv_len);
            ber_bvfree(data);
        } else {
            DEBUG(LDAP_DEBUG_TRACE, "   no creds present" << std::endl);
        }
    }
}

LDAPSaslBindResult::~LDAPSaslBindResult(){
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPSaslBindResult::~LDAPSaslBindResult()" << endl);
}

const string& LDAPSaslBindResult::getServerCreds() const{
    return m_creds;
}

