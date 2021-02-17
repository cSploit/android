// $OpenLDAP$
/*
 * Copyright 2003-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_ATTRTYPE_H
#define LDAP_ATTRTYPE_H

#include <ldap_schema.h>
#include <string>

#include "StringList.h"

using namespace std;

/**
 * Represents the Attribute Type (from LDAP schema)
 */
class LDAPAttrType{
    private :
	StringList names;
	std::string desc, oid, superiorOid, equalityOid;
        std::string orderingOid, substringOid, syntaxOid;
	bool single;
	int usage;

    public :

        /**
         * Constructor
         */   
        LDAPAttrType();

        /**
	 * Constructs new object and fills the data structure by parsing the
	 * argument.
	 * @param at_item description of attribute type is string returned
	 *  by the search command. It is in the form:
	 * "( SuSE.YaST.Attr:19 NAME ( 'skelDir' ) DESC ''
	 *    EQUALITY caseExactIA5Match SYNTAX 1.3.6.1.4.1.1466.115.121.1.26 )"
         */   
        LDAPAttrType (string at_item, int flags = LDAP_SCHEMA_ALLOW_NO_OID | 
                      LDAP_SCHEMA_ALLOW_QUOTED );

        /**
         * Destructor
         */
        virtual ~LDAPAttrType();
	
	
	/**
	 * Returns attribute description
	 */
	string getDesc() const;
	
	/**
	 * Returns attribute oid
	 */
	string getOid() const;

	/**
	 * Returns attribute name (first one if there are more of them)
	 */
	string getName() const;

	/**
	 * Returns all attribute names
	 */
	StringList getNames() const;
	
	/**
	 * Returns true if attribute type allows only single value
	 */
	bool isSingle() const;
	
	/**
 	 * Return the 'usage' value:
 	 * (0=userApplications, 1=directoryOperation, 2=distributedOperation, 
	 *  3=dSAOperation)
 	 */
 	int getUsage () const;
        std::string getSuperiorOid() const;
        std::string getEqualityOid() const;
        std::string getOrderingOid() const;
        std::string getSubstringOid() const;
        std::string getSyntaxOid() const;

	void setNames( char **at_names);
	void setDesc(const char *at_desc);
	void setOid(const char *at_oid);
	void setSingle(int at_single_value);
	void setUsage(int at_usage );
        void setSuperiorOid( const char *oid );
        void setEqualityOid( const char *oid );
        void setOrderingOid( const char *oid );
        void setSubstringOid( const char *oid );
        void setSyntaxOid( const char *oid );
};

#endif // LDAP_ATTRTYPE_H
