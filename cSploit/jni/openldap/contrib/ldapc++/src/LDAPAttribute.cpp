// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


//TODO!!!
//  * some kind of iterator to step through the attribute values
//  * remove values from Attribute
//  * handling of subtypes (;de; and so on)
//  * some documentation


#include <ldap.h> 
#include <cstdlib>

#include "debug.h"
#include "StringList.h"

#include "LDAPAttribute.h"

using namespace std;

LDAPAttribute::LDAPAttribute(){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPAttribute::LDAPAttribute( )" << endl);
    m_name=string();
}

LDAPAttribute::LDAPAttribute(const LDAPAttribute& attr){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPAttribute::LDAPAttribute(&)" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   attr:" << attr << endl);
	m_name=attr.m_name;
    m_values=StringList(attr.m_values);
}

LDAPAttribute::LDAPAttribute(const string& name, const string& value){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPAttribute::LDAPAttribute()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   name:" << name << endl << "   value:" << value << endl);
    this->setName(name);
    if(value != ""){
    	this->addValue(value);
    }
}


LDAPAttribute::LDAPAttribute(const string& name, const StringList& values){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPAttribute::LDAPAttribute()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   name:" << name << endl);
    m_name=name;
    m_values=values;
}

LDAPAttribute::LDAPAttribute(const char *name, char **values){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPAttribute::LDAPAttribute()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   name:" << name << endl);
	this->setName(name);
	this->setValues(values);
}

LDAPAttribute::LDAPAttribute(const char *name, BerValue **values){
    DEBUG(LDAP_DEBUG_CONSTRUCT, "LDAPAttribute::LDAPAttribute()" << endl);
    DEBUG(LDAP_DEBUG_CONSTRUCT | LDAP_DEBUG_PARAMETER,
            "   name:" << name << endl);
	this->setName(name);
	this->setValues(values);
}

LDAPAttribute::~LDAPAttribute(){
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPAttribute::~LDAPAttribute()" << endl);
}

void LDAPAttribute::addValue(const string& value){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::addValue()" << endl);
    m_values.add(value);
}

int LDAPAttribute::addValue(const BerValue *value){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::addValue()" << endl);
	if(value!=0){
		this->addValue(string(value->bv_val, value->bv_len));
		return 0;
	}
	return -1;
}

int LDAPAttribute::setValues(char **values){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::setValues()" << endl);
	if(values){
        m_values.clear();
        for( char **i=values; *i!=0; i++){
            this->addValue(*i);
        }
    }
    return 0;
}

int LDAPAttribute::setValues(BerValue **values){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::setValues()" << endl);
    if(values){
	    m_values.clear();
        for( BerValue **i=values; *i!=0; i++){
            if( this->addValue(*i) ){
                return -1;
            }
        }
    }
	return 0;
}

void LDAPAttribute::setValues(const StringList& values){
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::setValues()" << endl);
    m_values=values;
}

const StringList& LDAPAttribute::getValues() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::getValues()" << endl);
    return m_values;
}

BerValue** LDAPAttribute::getBerValues() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::getBerValues()" << endl);
    size_t size=m_values.size();
    if (size == 0){
        return 0;
    }else{
        BerValue **temp = (BerValue**) malloc(sizeof(BerValue*) * (size+1));
        StringList::const_iterator i;
        int p=0;

        for(i=m_values.begin(), p=0; i!=m_values.end(); i++,p++){
            temp[p]=(BerValue*) malloc(sizeof(BerValue));
            temp[p]->bv_len= i->size();
            temp[p]->bv_val= (char*) malloc(sizeof(char) * (i->size()+1));
            i->copy(temp[p]->bv_val,string::npos);
        }
        temp[size]=0;
        return temp;
    }
}

int LDAPAttribute::getNumValues() const{
    DEBUG(LDAP_DEBUG_TRACE,"LDAPAttribute::getNumValues()" << endl);
	return m_values.size();
}

const string& LDAPAttribute::getName() const {
    DEBUG(LDAP_DEBUG_TRACE, "LDAPAttribute::getName()" << endl);
	return m_name;
}

void LDAPAttribute::setName(const string& name){
    DEBUG(LDAP_DEBUG_TRACE, "LDAPAttribute::setName()" << endl);
    DEBUG(LDAP_DEBUG_TRACE | LDAP_DEBUG_PARAMETER,"   name:" << name << endl);
    m_name.erase();
    m_name=name;
}

// The bin-FLAG of the mod_op  is always set to LDAP_MOD_BVALUES (0x80) 
LDAPMod* LDAPAttribute::toLDAPMod() const {
    DEBUG(LDAP_DEBUG_TRACE, "LDAPAttribute::toLDAPMod()" << endl);
    LDAPMod* ret= (LDAPMod*) malloc(sizeof(LDAPMod));
    ret->mod_op=LDAP_MOD_BVALUES;	//always assume binary-Values
    ret->mod_type= (char*) malloc(sizeof(char) * (m_name.size()+1));
    m_name.copy(ret->mod_type,string::npos);
    ret->mod_type[m_name.size()]=0;
    ret->mod_bvalues=this->getBerValues();
    return ret;
}

bool LDAPAttribute::isNotPrintable() const {
    StringList::const_iterator i;
    for(i=m_values.begin(); i!=m_values.end(); i++){
	size_t len = i->size();
	for(size_t j=0; j<len; j++){
	    if (! isprint( (i->data())[j] ) ){
		return true;
	    }
	}
    }
    return false;
}

ostream& operator << (ostream& s, const LDAPAttribute& attr){
    s << attr.m_name << "=";
    StringList::const_iterator i;
    if (attr.isNotPrintable()){
	    s << "NOT_PRINTABLE" ;
    }else{
	for(i=attr.m_values.begin(); i!=attr.m_values.end(); i++){
	    s << *i << " ";
	}
    }
	return s;
}
