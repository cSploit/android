// $OpenLDAP$
/*
 * Copyright 2003-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_OBJCLASS_H
#define LDAP_OBJCLASS_H

#include <ldap_schema.h>
#include <string>

#include "StringList.h"

using namespace std;

/**
 * Represents the Object Class (from LDAP schema)
 */
class LDAPObjClass{
    private :
	StringList names, must, may, sup;
	string desc, oid;
	int kind;
	
    public :

        /**
         * Constructs an empty object.
         */   
        LDAPObjClass();

        /**
         * Copy constructor
	 */   
	LDAPObjClass( const LDAPObjClass& oc );

        /**
	 * Constructs new object and fills the data structure by parsing the
	 * argument.
	 * @param oc_item description of object class is string returned
	 * by the search command. It is in the form:
	 * "( SuSE.YaST.OC:5 NAME 'userTemplate' SUP objectTemplate STRUCTURAL
	 *    DESC 'User object template' MUST ( cn ) MAY ( secondaryGroup ))"
         */   
        LDAPObjClass (string oc_item, int flags = LDAP_SCHEMA_ALLOW_NO_OID |
                      LDAP_SCHEMA_ALLOW_QUOTED);

        /**
         * Destructor
         */
        virtual ~LDAPObjClass();
	
	/**
	 * Returns object class description
	 */
	string getDesc() const;
	
	/**
	 * Returns object class oid
	 */
	string getOid() const;

	/**
	 * Returns object class name (first one if there are more of them)
	 */
	string getName() const;

	/**
	 * Returns object class kind: 0=ABSTRACT, 1=STRUCTURAL, 2=AUXILIARY
	 */
	int getKind() const;

	/**
	 * Returns all object class names
	 */
	StringList getNames() const;
	
	/**
	 * Returns list of required attributes
	 */
	StringList getMust() const;
	
	/**
	 * Returns list of allowed (and not required) attributes
	 */
	StringList getMay() const;
	
        /**
	 * Returns list of the OIDs of the superior ObjectClasses
	 */
	StringList getSup() const;

	void setNames (char **oc_names);
	void setMay (char **oc_may);
	void setMust (char **oc_must);
	void setDesc (char *oc_desc);
	void setOid (char *oc_oid);
	void setKind (int oc_kind);
	void setSup (char **oc_sup);
	
};

#endif // LDAP_OBJCLASS_H
