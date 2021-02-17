// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_ENTRY_LIST_H
#define LDAP_ENTRY_LIST_H

#include <cstdio>
#include <list>

class LDAPEntry;
   
/**
 * For internal use only.
 * 
 * This class is used by LDAPSearchResults to store a std::list of
 * LDAPEntry-Objects
 */
class LDAPEntryList{
    typedef std::list<LDAPEntry> ListType;

    public:
	typedef ListType::const_iterator const_iterator;

        /**
         * Copy-Constructor
         */
        LDAPEntryList(const LDAPEntryList& el);

        /**
         * Default-Constructor
         */
        LDAPEntryList();

        /**
         * Destructor
         */
        ~LDAPEntryList();

        /**
         * @return The number of entries currently stored in the list.
         */
        size_t size() const;

        /**
         * @return true if there are zero entries currently stored in the list.
         */
        bool empty() const;

        /**
         * @return An iterator pointing to the first element of the list.
         */
        const_iterator begin() const;

        /**
         * @return An iterator pointing to the end of the list
         */
        const_iterator end() const;

        /**
         * Adds an Entry to the end of the list.
         */
        void addEntry(const LDAPEntry& e);

    private:
        ListType m_entries;
};
#endif // LDAP_ENTRY_LIST_H
