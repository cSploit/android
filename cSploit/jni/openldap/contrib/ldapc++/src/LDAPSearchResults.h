// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_SEARCH_RESULTS_H
#define LDAP_SEARCH_RESULTS_H

#include <LDAPEntry.h>
#include <LDAPEntryList.h>
#include <LDAPMessage.h>
#include <LDAPMessageQueue.h>
#include <LDAPReferenceList.h>
#include <LDAPSearchReference.h>

class LDAPResult;

/**
 * The class stores the results of a synchronous SEARCH-Operation 
 */
class LDAPSearchResults{
    public:
        /**
         * Default-Constructor
         */
        LDAPSearchResults();

        /**
         * For internal use only.
         *
         * This method reads Search result entries from a
         * LDAPMessageQueue-object.
         * @param msg The message queue to read
         */
        LDAPResult* readMessageQueue(LDAPMessageQueue* msg);

        /**
         * The method is used by the client-application to read the
         * result entries of the  SEARCH-Operation. Every call of this
         * method returns one entry. If all entries were read it return 0.
         * @throws LDAPReferralException  If a Search Reference was
         *          returned by the server
         * @returns A LDAPEntry-object as a result of a SEARCH-Operation or
         *          0 if no more entries are there to return.
         */
        LDAPEntry* getNext();
    private :
        LDAPEntryList entryList;
        LDAPReferenceList refList;
        LDAPEntryList::const_iterator entryPos;
        LDAPReferenceList::const_iterator refPos;
};
#endif //LDAP_SEARCH_RESULTS_H


