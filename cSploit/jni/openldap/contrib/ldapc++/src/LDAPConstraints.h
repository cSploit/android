// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_CONSTRAINTS_H
#define LDAP_CONSTRAINTS_H 
#include <list>

#include <LDAPControl.h>
#include <LDAPControlSet.h>
#include <LDAPRebind.h>

//TODO!!
// * implement the Alias-Handling Option (OPT_DEREF)
// * the Restart-Option ???
// * default Server(s)

//* Class for representating the various protocol options
/** This class represents some options that can be set for a LDAPConnection
 *  operation. Namely these are time and size limits. Options for referral
 *  chasing and a default set of client of server controls to be used with
 *  every request
 */
class LDAPConstraints{
        
    public :
        static const int DEREF_NEVER = 0x00;   
        static const int DEREF_SEARCHING = 0x01;   
        static const int DEREF_FINDING = 0x02;   
        static const int DEREF_ALWAYS = 0x04;   
        
        //* Constructs a LDAPConstraints object with default values
        LDAPConstraints();

        //* Copy constructor
        LDAPConstraints(const LDAPConstraints& c);

        ~LDAPConstraints();
            
        void setAliasDeref(int deref);
        void setMaxTime(int t);
        void setSizeLimit(int s);
        void setReferralChase(bool rc);
        void setHopLimit(int hop);
        void setReferralRebind(const LDAPRebind* rebind);
        void setServerControls(const LDAPControlSet* ctrls);
        void setClientControls(const LDAPControlSet* ctrls);
        
        int getAliasDeref() const;
        int getMaxTime() const ;
        int getSizeLimit() const;
        const LDAPRebind* getReferralRebind() const;
        const LDAPControlSet* getServerControls() const;
        const LDAPControlSet* getClientControls() const;
        
        //*for internal use only
        LDAPControl** getSrvCtrlsArray() const;
        
        //*for internal use only
        LDAPControl** getClCtrlsArray() const;
        
        //*for internal use only
        timeval* getTimeoutStruct() const;
        bool getReferralChase() const ;
        int getHopLimit() const;

    private :
        int m_aliasDeref;

        //* max. time the server may spend for a search request
        int m_maxTime;

        //* max number of entries to return from a search request
        int m_maxSize;

        //* Flag for enabling automatic referral/reference chasing
        bool m_referralChase;

        //* HopLimit for referral chasing
        int m_HopLimit;

        //* Alias dereferencing option
        int m_deref;
	
        //* Object used to do bind for Referral chasing
        const LDAPRebind* m_refRebind;

        //* List of Client Controls that should be used for each request	
        LDAPControlSet* m_clientControls;

        //* List of Server Controls that should be used for each request	
        LDAPControlSet* m_serverControls;

};
#endif //LDAP_CONSTRAINTS_H
