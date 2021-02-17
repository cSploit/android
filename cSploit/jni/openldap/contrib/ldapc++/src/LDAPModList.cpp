// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "LDAPModList.h"
#include "debug.h"

#include <cstdlib>

using namespace std;

LDAPModList::LDAPModList(){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPModList::LDAPModList()" << endl);
}

LDAPModList::LDAPModList(const LDAPModList& ml){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPModList::LDAPModList(&)" << endl);
    m_modList=ml.m_modList;
}

void LDAPModList::addModification(const LDAPModification &mod){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPModList::addModification()" << endl);
	m_modList.push_back(mod);
}

LDAPMod** LDAPModList::toLDAPModArray(){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPModList::toLDAPModArray()" << endl);
    LDAPMod **ret = (LDAPMod**) malloc(
		    (m_modList.size()+1) * sizeof(LDAPMod*));
    ret[m_modList.size()]=0;
    LDAPModList::ListType::const_iterator i;
    int j=0;
    for (i=m_modList.begin(); i != m_modList.end(); i++ , j++){
	    ret[j]=i->toLDAPMod();
    }
    return ret;
}

bool LDAPModList::empty() const {
    return m_modList.empty();
}

unsigned int LDAPModList::size() const {
    return m_modList.size();
}
