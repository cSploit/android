// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_CONTROL_SET_H
#define LDAP_CONTROL_SET_H

#include <list>
#include <ldap.h>
#include <LDAPControl.h>

typedef std::list<LDAPCtrl> CtrlList;

/**
 * This container class is used to store multiple LDAPCtrl-objects.
 */
class LDAPControlSet {
    typedef CtrlList::const_iterator const_iterator;
    public :
        /**
         * Constructs an empty std::list
         */
        LDAPControlSet();   


        /**
         * Copy-constructor
         */
        LDAPControlSet(const LDAPControlSet& cs);
        
        /**
         * For internal use only
         *
         * This constructor creates a new LDAPControlSet for a
         * 0-terminiated array of LDAPControl-structures as used by the
         * C-API
         * @param controls: pointer to a 0-terminated array of pointers to 
         *                  LDAPControll-structures
         * @note: untested til now. Due to lack of server that return 
         *          Controls
         */
        LDAPControlSet(LDAPControl** controls);

        /**
         * Destructor
         */
        ~LDAPControlSet();

        /**
         * @return The number of LDAPCtrl-objects that are currently
         * stored in this list.
         */
        size_t size() const ;
        
        /**
         * @return true if there are zero LDAPCtrl-objects currently
         * stored in this list.
         */
        bool empty() const;
        
        /**
         * @return A iterator that points to the first element of the list.
         */
        const_iterator begin() const;

        /**
         * @return A iterator that points to the element after the last
         * element of the list.
         */
        const_iterator end() const;
       
        /**
         * Adds one element to the end of the list.
         * @param ctrl The Control to add to the list.
         */
        void add(const LDAPCtrl& ctrl); 
        
        /**
         * Translates the list to a 0-terminated array of pointers to
         * LDAPControl-structures as needed by the C-API
         */
        LDAPControl** toLDAPControlArray()const ;
	static void freeLDAPControlArray(LDAPControl **ctrl);
    private :
        CtrlList data;
} ;
#endif //LDAP_CONTROL_SET_H
