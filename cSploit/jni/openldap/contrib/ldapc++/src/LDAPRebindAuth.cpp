// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include <iostream>

#include "LDAPRebindAuth.h"
#include "debug.h"

using namespace std;

LDAPRebindAuth::LDAPRebindAuth(const string& dn, const string& pwd){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPRebindAuth::LDAPRebindAuth()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,"   dn:" << dn << endl 
            << "   pwd:" << pwd << endl);
    m_dn=dn;
    m_password=pwd;
}

LDAPRebindAuth::LDAPRebindAuth(const LDAPRebindAuth& lra){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPRebindAuth::LDAPRebindAuth(&)" << endl);
    m_dn=lra.m_dn;
    m_password=lra.m_password;
}

LDAPRebindAuth::~LDAPRebindAuth(){
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPRebindAuth::~LDAPRebindAuth()" << endl);
}

const string& LDAPRebindAuth::getDN() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPRebindAuth::getDN()" << endl);
    return m_dn;
}

const string& LDAPRebindAuth::getPassword() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPRebindAuth::getPassword()" << endl);
    return m_password;
}
