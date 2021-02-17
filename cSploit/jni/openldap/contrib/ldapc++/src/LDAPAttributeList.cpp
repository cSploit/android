// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "debug.h"

#include "LDAPAttributeList.h"

#include "LDAPException.h"
#include "LDAPAttribute.h"
#include "LDAPAsynConnection.h"
#include "LDAPMessage.h"

#include <cstdlib>

using namespace std;

// little helper function for doing case insensitve string comparison
bool nocase_compare(char c1, char c2);

LDAPAttributeList::LDAPAttributeList(){
    DEBUG(LDAP_DEBUG_CONSTRUCT,
            "LDAPAttributeList::LDAPAttributList( )" << endl);
}

LDAPAttributeList::LDAPAttributeList(const LDAPAttributeList& al){
    DEBUG(LDAP_DEBUG_CONSTRUCT,
            "LDAPAttributeList::LDAPAttributList(&)" << endl);
    m_attrs=al.m_attrs;
}

LDAPAttributeList::LDAPAttributeList(const LDAPAsynConnection *ld, 
        LDAPMessage *msg){
    DEBUG(LDAP_DEBUG_CONSTRUCT,
            "LDAPAttributeList::LDAPAttributList()" << endl);
    BerElement *ptr=0;
    char *name=ldap_first_attribute(ld->getSessionHandle(), msg, &ptr);
/*
   This code was making problems if no attribute were returned
   How am I supposed to find decoding errors? ldap_first/next_attribute
   return 0 in case of error or if there are no more attributes. In either
   case they set the LDAP* error code to 0x54 (Decoding error) ??? Strange..

   There will be some changes in the new version of the C-API so that this
   code should work in the future.
   if(name == 0){
        ber_free(ptr,0);
        ldap_memfree(name);
        throw LDAPException(ld);
    }else{
*/        BerValue **values;
        for (;name !=0;
                name=ldap_next_attribute(ld->getSessionHandle(),msg,ptr) ){
            values=ldap_get_values_len(ld->getSessionHandle(),
                    msg, name);
            this->addAttribute(LDAPAttribute(name, values));
            ldap_memfree(name);
            ldap_value_free_len(values);
        }
        ber_free(ptr,0);
//    }
}

LDAPAttributeList::~LDAPAttributeList(){
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPAttributeList::~LDAPAttributList()" << endl);
}

size_t LDAPAttributeList::size() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::size()" << endl);
    return m_attrs.size();
}

bool LDAPAttributeList::empty() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::empty()" << endl);
    return m_attrs.empty();
}

LDAPAttributeList::const_iterator LDAPAttributeList::begin() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::begin()" << endl);
    return m_attrs.begin();
}

LDAPAttributeList::const_iterator LDAPAttributeList::end() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::end()" << endl);
    return m_attrs.end();
}

const LDAPAttribute* LDAPAttributeList::getAttributeByName(
	const string& name) const {
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::getAttributeByName()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,
            "   name:" << name << endl);
    LDAPAttributeList::const_iterator i;
    for( i = m_attrs.begin(); i != m_attrs.end(); i++){
	const std::string& tmpType = i->getName();
	if(name.size() == tmpType.size()){
	    if(equal(name.begin(), name.end(), tmpType.begin(),
		    nocase_compare)){
		return &(*i);
		DEBUG(LDAP_DEBUG_TRACE,"    found:" << name << endl);
	    }
	}
    }
    return 0;
}

void LDAPAttributeList::addAttribute(const LDAPAttribute& attr){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::addAttribute()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,
            "   attr:" << attr << endl);
    const std::string attrType = attr.getName();
    const std::string::size_type attrLen = attrType.size();
    std::string::size_type tmpAttrLen = 0;
    bool done=false;
    LDAPAttributeList::iterator i;
    for( i=m_attrs.begin(); i != m_attrs.end(); i++ ){
	const std::string tmpAttrType = i->getName();
	tmpAttrLen = tmpAttrType.size();
	if(tmpAttrLen == attrLen){
	    if(equal(tmpAttrType.begin(), tmpAttrType.end(), attrType.begin(),
		    nocase_compare)){
		const StringList& values = attr.getValues();
		StringList::const_iterator j;
		for(j = values.begin(); j != values.end(); j++){
		    i->addValue(*j);
		}
		DEBUG(LDAP_DEBUG_TRACE,"Attribute" << i->getName() 
			<< "already present" << endl);
		done=true;
		break; // The AttributeType was already present,
		       // we are done here
	    }
	}
    }
    if(! done){
	m_attrs.push_back(attr);
    }
}

void LDAPAttributeList::delAttribute(const std::string& type)
{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::replaceAttribute()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER, "   type: " << type << endl);
    LDAPAttributeList::iterator i;
    for( i = m_attrs.begin(); i != m_attrs.end(); i++){
	if(type.size() == i->getName().size()){
	    if(equal(type.begin(), type.end(), i->getName().begin(),
		    nocase_compare)){
                m_attrs.erase(i);
                break;
            }
        }
    }
}

void LDAPAttributeList::replaceAttribute(const LDAPAttribute& attr)
{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::replaceAttribute()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,
            "   attr:" << attr << endl);
    
    LDAPAttributeList::iterator i;
    this->delAttribute( attr.getName() );
    m_attrs.push_back(attr);
}

LDAPMod** LDAPAttributeList::toLDAPModArray() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::toLDAPModArray()" << endl);
    LDAPMod **ret = (LDAPMod**) malloc((m_attrs.size()+1) * sizeof(LDAPMod*));
    LDAPAttributeList::const_iterator i;
    int j=0;
    for (i=m_attrs.begin(); i!= m_attrs.end(); i++, j++){
        ret[j]=i->toLDAPMod();
    }
    ret[m_attrs.size()]=0;
    return ret;
}

ostream& operator << (ostream& s, const LDAPAttributeList& al){
    LDAPAttributeList::const_iterator i;
    for(i=al.m_attrs.begin(); i!=al.m_attrs.end(); i++){
        s << *i << "; ";
    }
    return s;
}

bool nocase_compare( char c1, char c2){
    return toupper(c1) == toupper(c2);
}

