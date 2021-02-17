// $OpenLDAP$
/*
 * Copyright 2003-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "debug.h"
#include "LDAPAttrType.h"


LDAPAttrType::LDAPAttrType(){
    DEBUG(LDAP_DEBUG_CONSTRUCT,
            "LDAPAttrType::LDAPAttrType( )" << endl);

    oid = string ();
    desc = string ();
    names = StringList ();
    single = false;
    usage = 0;
}

LDAPAttrType::LDAPAttrType (string at_item, int flags ) { 

    DEBUG(LDAP_DEBUG_CONSTRUCT,
            "LDAPAttrType::LDAPAttrType( )" << endl);

    LDAPAttributeType *a;
    int ret;
    const char *errp;
    a = ldap_str2attributetype (at_item.c_str(), &ret, &errp, flags);

    if (a) {
	this->setNames( a->at_names );
	this->setDesc( a->at_desc );
	this->setOid( a->at_oid );
	this->setSingle( a->at_single_value );
	this->setUsage( a->at_usage );
        this->setSuperiorOid( a->at_sup_oid );
        this->setEqualityOid( a->at_equality_oid );
        this->setOrderingOid( a->at_ordering_oid );
        this->setSubstringOid( a->at_substr_oid );
        this->setSyntaxOid( a->at_syntax_oid );
    }
    // else? -> error
}

LDAPAttrType::~LDAPAttrType() {
    DEBUG(LDAP_DEBUG_DESTROY,"LDAPAttrType::~LDAPAttrType()" << endl);
}

void LDAPAttrType::setSingle (int at_single) {
    single = (at_single == 1);
}
    
void LDAPAttrType::setNames ( char **at_names ) {
    names = StringList(at_names);
}

void LDAPAttrType::setDesc (const char *at_desc) {
    desc = string ();
    if (at_desc)
	desc = at_desc;
}

void LDAPAttrType::setOid (const char *at_oid) {
    oid = string ();
    if (at_oid)
	oid = at_oid;
}

void LDAPAttrType::setUsage (int at_usage) {
    usage = at_usage;
}

void LDAPAttrType::setSuperiorOid( const char *oid ){
    if ( oid )
        superiorOid = oid;
}

void LDAPAttrType::setEqualityOid( const char *oid ){
    if ( oid )
        equalityOid = oid;
}

void LDAPAttrType::setOrderingOid( const char *oid ){
    if ( oid )
        orderingOid = oid;
}

void LDAPAttrType::setSubstringOid( const char *oid ){
    if ( oid )
        substringOid = oid;
}

void LDAPAttrType::setSyntaxOid( const char *oid ){
    if ( oid )
        syntaxOid = oid;
}

bool LDAPAttrType::isSingle() const {
    return single;
} 

string LDAPAttrType::getOid() const {
    return oid;
}

string LDAPAttrType::getDesc() const {
    return desc;
}

StringList LDAPAttrType::getNames() const {
    return names;
}

string LDAPAttrType::getName() const {

    if (names.empty())
	return "";
    else
	return *(names.begin());
}

int LDAPAttrType::getUsage() const {
    return usage;
}

std::string LDAPAttrType::getSuperiorOid() const {
    return superiorOid;
}

std::string LDAPAttrType::getEqualityOid() const {
    return equalityOid;
}

std::string LDAPAttrType::getOrderingOid() const {
    return orderingOid;
}

std::string LDAPAttrType::getSubstringOid() const {
    return substringOid;
}

std::string LDAPAttrType::getSyntaxOid() const {
    return syntaxOid;
}


