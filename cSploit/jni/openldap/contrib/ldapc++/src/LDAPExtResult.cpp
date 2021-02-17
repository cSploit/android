// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "debug.h"
#include <lber.h>
#include "LDAPRequest.h"
#include "LDAPException.h"

#include "LDAPResult.h"
#include "LDAPExtResult.h"

using namespace std;

LDAPExtResult::LDAPExtResult(const LDAPRequest* req, LDAPMessage* msg) :
        LDAPResult(req, msg){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPExtResult::LDAPExtResult()" << endl);
    char* oid = 0;
    BerValue* data = 0;
    LDAP* lc = req->getConnection()->getSessionHandle();
    int err=ldap_parse_extended_result(lc, msg, &oid, &data, 0);
    if(err != LDAP_SUCCESS){
        ber_bvfree(data);
        ldap_memfree(oid);
        throw LDAPException(err);
    }else{
        m_oid=string(oid);
        ldap_memfree(oid);
        if(data){
            m_data=string(data->bv_val, data->bv_len);
            ber_bvfree(data);
        }
    }
}

LDAPExtResult::~LDAPExtResult(){
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPExtResult::~LDAPExtResult()" << endl);
}

const string& LDAPExtResult::getResponseOid() const{
    return m_oid;
}

const string& LDAPExtResult::getResponse() const{
    return m_data;
}

