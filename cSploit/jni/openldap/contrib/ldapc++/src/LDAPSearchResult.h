// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_SEARCH_RESULT_H
#define LDAP_SEARCH_RESULT_H

#include <LDAPMessage.h>
#include <LDAPEntry.h>

class LDAPRequest;

/**
 * This class is used to represent the result entries of a
 * SEARCH-operation.
 */
class LDAPSearchResult : public LDAPMsg{
    public:
        /**
         * Constructor that create an object from the C-API structures
         */
        LDAPSearchResult(const LDAPRequest *req, LDAPMessage *msg);

        /**
         * Copy-Constructor
         */
        LDAPSearchResult(const LDAPSearchResult& res);

        /**
         * The Destructor
         */
        virtual ~LDAPSearchResult();

        /**
         * @returns The entry that has been sent with this result message. 
         */
        const LDAPEntry* getEntry() const;
    
    private:
        LDAPEntry *entry;
};
#endif //LDAP_SEARCH_RESULT_H
