// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "LDAPReferenceList.h"
#include "LDAPSearchReference.h"

LDAPReferenceList::LDAPReferenceList(){
}

LDAPReferenceList::LDAPReferenceList(const LDAPReferenceList& e){
    m_refs = e.m_refs;
}

LDAPReferenceList::~LDAPReferenceList(){
}

size_t LDAPReferenceList::size() const{
    return m_refs.size();
}

bool LDAPReferenceList::empty() const{
    return m_refs.empty();
}

LDAPReferenceList::const_iterator LDAPReferenceList::begin() const{
    return m_refs.begin();
}

LDAPReferenceList::const_iterator LDAPReferenceList::end() const{
    return m_refs.end();
}

void LDAPReferenceList::addReference(const LDAPSearchReference& e){
    m_refs.push_back(e);
}

