// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "debug.h"
#include "LDAPEntry.h"

#include "LDAPAsynConnection.h"
#include "LDAPException.h"

using namespace std;

LDAPEntry::LDAPEntry(const LDAPEntry& entry){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPEntry::LDAPEntry(&)" << endl);
    m_dn=entry.m_dn;
    m_attrs=new LDAPAttributeList( *(entry.m_attrs));
}


LDAPEntry::LDAPEntry(const string& dn, const LDAPAttributeList *attrs){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPEntry::LDAPEntry()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   dn:" << dn << endl);
    if ( attrs )
        m_attrs=new LDAPAttributeList(*attrs);
    else
        m_attrs=new LDAPAttributeList();
    m_dn=dn;
}

LDAPEntry::LDAPEntry(const LDAPAsynConnection *ld, LDAPMessage *msg){
    DEBUG(LDAP_DEBUG_CONSTRUCT,"LDAPEntry::LDAPEntry()" << endl);
    char* tmp=ldap_get_dn(ld->getSessionHandle(),msg);
    m_dn=string(tmp);
    ldap_memfree(tmp);
    m_attrs = new LDAPAttributeList(ld, msg);
}

LDAPEntry::~LDAPEntry(){
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPEntry::~LDAPEntry()" << endl);
    delete m_attrs;
}

LDAPEntry& LDAPEntry::operator=(const LDAPEntry& from){
    m_dn = from.m_dn;
    delete m_attrs;
    m_attrs = new LDAPAttributeList( *(from.m_attrs));
    return *this;
}

void LDAPEntry::setDN(const string& dn){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPEntry::setDN()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,
            "   dn:" << dn << endl);
    m_dn=dn;
}

void LDAPEntry::setAttributes(LDAPAttributeList *attrs){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPEntry::setAttributes()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,
            "   attrs:" << *attrs << endl);
    if (m_attrs != 0){
        delete m_attrs;
    }
    m_attrs=attrs;
}

const string& LDAPEntry::getDN() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPEntry::getDN()" << endl);
    return m_dn;
}

const LDAPAttributeList* LDAPEntry::getAttributes() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPEntry::getAttributes()" << endl);
    return m_attrs;
}

const LDAPAttribute* LDAPEntry::getAttributeByName(const std::string& name) const 
{
    return m_attrs->getAttributeByName(name);
}

void LDAPEntry::addAttribute(const LDAPAttribute& attr)
{
    m_attrs->addAttribute(attr);
}

void LDAPEntry::delAttribute(const std::string& type)
{
    m_attrs->delAttribute(type);
}

void LDAPEntry::replaceAttribute(const LDAPAttribute& attr)
{
    m_attrs->replaceAttribute(attr); 
}

ostream& operator << (ostream& s, const LDAPEntry& le){
    s << "DN: " << le.m_dn << ": " << *(le.m_attrs); 
    return s;
}
