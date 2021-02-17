// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_MOD_LIST_H
#define LDAP_MOD_LIST_H

#include <ldap.h>
#include <list>
#include <LDAPModification.h>

/**
 * This container class is used to store multiple LDAPModification-objects.
 */
class LDAPModList{
    typedef std::list<LDAPModification> ListType;

    public : 
        /**
         * Constructs an empty list.
         */   
        LDAPModList();
		
        /**
         * Copy-constructor
         */
        LDAPModList(const LDAPModList&);

        /**
         * Adds one element to the end of the list.
         * @param mod The LDAPModification to add to the std::list.
         */
        void addModification(const LDAPModification &mod);

        /**
         * Translates the list to a 0-terminated array of
         * LDAPMod-structures as needed by the C-API
         */
        LDAPMod** toLDAPModArray();

        /**
         * @returns true, if the ModList contains no Operations
         */
        bool empty() const;
        
        /**
         * @returns number of Modifications in the ModList
         */
        unsigned int size() const;

    private : 
        ListType m_modList;
};
#endif //LDAP_MOD_LIST_H


