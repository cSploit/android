// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_REQUEST_H
#define LDAP_REQUEST_H

#include <LDAPConstraints.h>
#include <LDAPAsynConnection.h>
#include <LDAPMessageQueue.h>

class LDAPUrl;

/**
 * For internal use only
 * 
 * Each request that is sent to a LDAP-server by this library is
 * represented by a special object that contains the parameters and some
 * other info of the request. This virtual class is the common base classe
 * for these specialized request classes.
 */
class LDAPRequest{

    public :
        static const int BIND=0;
        static const int UNBIND=2;
        static const int SEARCH=3;
        static const int MODIFY=7;
        static const int ADD=8;
		static const int DELETE=10;
        static const int COMPARE=14;

        LDAPRequest(const LDAPRequest& req);
        LDAPRequest(LDAPAsynConnection* conn, 
                const LDAPConstraints* cons, bool isReferral=false,
                const LDAPRequest* parent=0);
        virtual ~LDAPRequest();
        
        const LDAPConstraints* getConstraints() const;
        const LDAPAsynConnection* getConnection() const;
        virtual LDAPMsg *getNextMessage() const;
        int getType()const;
        int getMsgID() const;
        int getHopCount() const;

        /**
         * @return The LDAPRequest that has created this object. Or 0 if
         * this object was not created by another request.
         */
        const LDAPRequest* getParent() const;

        /**
         * @return true if this object was created during the automatic
         * chasing of referrals. Otherwise false
         */
        bool isReferral() const;
        
        void unbind() const; 

        /**
         * This method encodes the request an calls the apprpriate
         * functions of the C-API to send the Request to a LDAP-Server
         */
        virtual LDAPMessageQueue* sendRequest()=0;
        virtual LDAPRequest* followReferral(LDAPMsg* ref);

        /**
         * Compare this request with another on. And returns true if they
         * have the same parameters.
         */
        virtual bool equals(const LDAPRequest* req) const;

        bool isCycle() const;
        
    protected :
        bool m_isReferral;
        int m_requestType;
        LDAPConstraints *m_cons;
        LDAPAsynConnection *m_connection;
        const LDAPRequest* m_parent;
        int m_hopCount;
        int m_msgID;  //the associated C-API Message ID
        LDAPRequest();
};
#endif //LDAP_REQUEST_H 

