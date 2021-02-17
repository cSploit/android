// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_MSG_H
#define LDAP_MSG_H
#include <ldap.h>

#include <LDAPControlSet.h>

class LDAPRequest;
/**
 * This class represents any type of LDAP- Message returned 
 * from the server.
 *
 * This class is never not instantiated directly. Only
 * its subclasses are used. The main feature of this class is the
 * static method create() (see below)
 */
class LDAPMsg{
    public:
        //public Constants defining the response message types
        static const int BIND_RESPONSE=LDAP_RES_BIND;
        static const int SEARCH_ENTRY=LDAP_RES_SEARCH_ENTRY;
        static const int SEARCH_DONE=LDAP_RES_SEARCH_RESULT;
        static const int SEARCH_REFERENCE=LDAP_RES_SEARCH_REFERENCE;
        static const int MODIFY_RESPONSE=LDAP_RES_MODIFY;
        static const int ADD_RESPONSE=LDAP_RES_ADD;
        static const int DEL_RESPONSE=LDAP_RES_DELETE;
        static const int MODDN_RESPONSE=LDAP_RES_MODDN;
        static const int COMPARE_RESPONSE=LDAP_RES_COMPARE;
        static const int EXTENDED_RESPONSE=LDAP_RES_EXTENDED;
        //public Constants defining the request message types
        static const int BIND_REQUEST=LDAP_REQ_BIND;
        static const int UNBIND_REQUEST=LDAP_REQ_UNBIND;
        static const int SEARCH_REQUEST=LDAP_REQ_SEARCH;
        static const int MODIFY_REQUEST=LDAP_REQ_MODIFY;
        static const int ADD_REQUEST=LDAP_REQ_ADD;
        static const int DELETE_REQUEST=LDAP_REQ_DELETE;
        static const int MODRDN_REQUEST=LDAP_REQ_MODRDN;
        static const int COMPARE_REQUEST=LDAP_REQ_COMPARE;
        static const int ABANDON_REQUEST=LDAP_REQ_ABANDON;
        static const int EXTENDED_REQUEST=LDAP_REQ_EXTENDED;
       
        /**
         * The destructor has no implemenation, because this is an abstract
         * class.
         */
        virtual ~LDAPMsg() {}

        /**
         * This method is used by the library to parse the results returned
         * by the C-API.
         *
         * Based on msgtype-Value of the *msg-Parameter this method creates
         * an Object of one of the subtypes of LDAPMsg (e.g. LDAPSearchResult
         * or LDAPResult) that represents the same Message as the
         * *msg-Parameter. *msg is e.g. a Message returned by the C-API's
         * ldap_result call.
         * @param req   The LDAPRequest-object this result message is
         *          associated with.
         * @param msg   The LDAPMessage-structure from the C-API that
         *          contains the LDAP-message to parse.
         * @return An Object of one of the subtypes of this class. It
         *          contains the parsed LDAP-message.
         */
        static LDAPMsg* create(const LDAPRequest *req, LDAPMessage *msg);   
        
        /**
         * @returns The Type of message that this object contains. Possible
         *          values are: <BR>
         *          BIND_RESPONSE <BR>
         *          SEARCH_ENTRY  <BR>       
         *          SEARCH_DONE   <BR>      
         *          SEARCH_REFERENCE   <BR>      
         *          MODIFY_RESPONSE     <BR>    
         *          ADD_RESPONSE       <BR>  
         *          DEL_RESPONSE       <BR>  
         *          MODDN_RESPONSE     <BR>    
         *          COMPARE_RESPONSE   <BR>
         *          EXTENDED_REPONSE   <BR>      
         */
        int getMessageType();
        
        /**
         * @returns The message-ID that the C-API return for the
         *      Result-message. 
         */
        int getMsgID();

        /**
         * @returns If any Control was sent back by the server this method
         *       returns true. Otherwise false is returned.
         */
        bool hasControls() const;

        /**
         * @returns Server controls that were sent back by the server.
         * @note This feature is not test well yet.
         */
        const LDAPControlSet& getSrvControls() const;
    
    protected:
        /**
         * This constructor make a copy of a LDAPMsg-pointer. The object
         * itself (no the pointer) is copied.
         * Only for internal use.
         */
        LDAPMsg(LDAPMessage *msg);
        LDAPMsg(int msgType, int msgID);
       
        /**
         * This attribute stores Server-Control that were returned with the
         * message.
         */
        LDAPControlSet m_srvControls;
        
        bool m_hasControls;

    private:
        int msgType;
        int msgID;
};
#endif //ifndef LDAP_MSG_H
