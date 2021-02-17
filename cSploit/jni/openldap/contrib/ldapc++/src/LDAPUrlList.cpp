// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "LDAPUrlList.h"
#include <assert.h>
#include "debug.h"

using namespace std;

LDAPUrlList::LDAPUrlList(){
    DEBUG(LDAP_DEBUG_CONSTRUCT," LDAPUrlList::LDAPUrlList()" << endl);
    m_urls=LDAPUrlList::ListType();
}

LDAPUrlList::LDAPUrlList(const LDAPUrlList& urls){
    DEBUG(LDAP_DEBUG_CONSTRUCT," LDAPUrlList::LDAPUrlList(&)" << endl);
    m_urls = urls.m_urls;
}


LDAPUrlList::LDAPUrlList(char** url){
    DEBUG(LDAP_DEBUG_CONSTRUCT," LDAPUrlList::LDAPUrlList()" << endl);
    char** i;
    assert(url);
    for(i = url; *i != 0; i++){
        add(LDAPUrl(*i));
    }
}

LDAPUrlList::~LDAPUrlList(){
    DEBUG(LDAP_DEBUG_DESTROY," LDAPUrlList::~LDAPUrlList()" << endl);
    m_urls.clear();
}

size_t LDAPUrlList::size() const{
    return m_urls.size();
}

bool LDAPUrlList::empty() const{
    return m_urls.empty();
}

LDAPUrlList::const_iterator LDAPUrlList::begin() const{
    return m_urls.begin();
}

LDAPUrlList::const_iterator LDAPUrlList::end() const{
    return m_urls.end();
}

void LDAPUrlList::add(const LDAPUrl& url){
    m_urls.push_back(url);
}

