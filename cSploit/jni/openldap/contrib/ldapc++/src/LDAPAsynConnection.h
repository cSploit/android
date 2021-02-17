// $OpenLDAP$
/*
 * Copyright 2000-2014 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_ASYN_CONNECTION_H
#define LDAP_ASYN_CONNECTION_H

#include<iostream>
#include<string>

#include<ldap.h>

#include <LDAPEntry.h>
#include <LDAPException.h>
#include <LDAPMessageQueue.h>
#include <LDAPConstraints.h>
#include <LDAPModification.h>
#include <LDAPModList.h>
#include <LDAPUrl.h>
#include <LDAPUrlList.h>
#include <SaslInteractionHandler.h>
#include <TlsOptions.h>

//* Main class for an asynchronous LDAP connection 
/**
 * This class represents an asynchronous connection to an LDAP-Server. It 
 * provides the methods for authentication, and all other LDAP-Operations
 * (e.g. search, add, delete, etc.)
 * All of the LDAP-Operations return a pointer to a LDAPMessageQueue-Object,
 * which can be used to obtain the results of that operation.
 * A basic example of this class could be like this:  <BR>
 * 1. Create a new LDAPAsynConnection Object: <BR>
 * 2. Use the init-method to initialize the connection <BR>
 * 3. Call the bind-method to authenticate to the directory <BR>
 * 4. Obtain the bind results from the return LDAPMessageQueue-Object <BR>
 * 5. Perform on of the operations on the directory (add, delete, search, ..)
 *    <BR>
 * 6. Use the return LDAPMessageQueue to obtain the results of the operation 
 * <BR>
 * 7. Close the connection (feature not implemented yet :) ) <BR>
 */
class LDAPAsynConnection{
    public :
        /** 
         * Constant for the Search-Operation to indicate a Base-Level
         * Search
         */
        static const int SEARCH_BASE=0;
        
        /** 
         * Constant for the Search-Operation to indicate a One-Level
         * Search
         */
        static const int SEARCH_ONE=1;
        
        /** 
         * Constant for the Search-Operation to indicate a subtree 
         * Search
         */
        static const int SEARCH_SUB=2;

        /** Construtor that initializes a connection to a server
         * @param hostname Name (or IP-Adress) of the destination host
         * @param port Port the LDAP server is running on
         * @param cons Default constraints to use with operations over 
         *      this connection
         */
        LDAPAsynConnection(const std::string& url=std::string("localhost"),
                int port=0, LDAPConstraints *cons=new LDAPConstraints() );

        //* Destructor
        virtual ~LDAPAsynConnection();
        /** 
         * Initializes a connection to a server. 
         * 
         * There actually no
         * communication to the server. Just the object is initialized
         * (e.g. this method is called within the 
         * LDAPAsynConnection(char*,int,LDAPConstraints) constructor.)
         * @param hostname  The Name or IP-Address of the destination
         *             LDAP-Server
         * @param port      The Network Port the server is running on
         */
        void init(const std::string& hostname, int port);

        /**
         * Initializes a connection to a server. 
         * 
         * There actually no communication to the server. Just the
         * object is initialized 
         * @param uri  The LDAP-Uri for the destination
         */ 
        void initialize(const std::string& uri);

        /**
         * Start TLS on this connection.  This isn't in the constructor,
         * because it could fail (i.e. server doesn't have SSL cert, client
         * api wasn't compiled against OpenSSL, etc.). 
         * @throws LDAPException if the TLS Layer could not be setup 
         * correctly
         */
        void start_tls();

        /** Simple authentication to a LDAP-Server
         *
         * @throws LDAPException If the Request could not be sent to the
         *      destination server, a LDAPException-object contains the
         *      error that occured.
         * This method does a simple (username, password) bind to the server.
         * Other, saver, authentcation methods are provided later
         * @param dn the distiguished name to bind as
         * @param passwd cleartext password to use
         */
        LDAPMessageQueue* bind(const std::string& dn="", 
                const std::string& passwd="",
                const LDAPConstraints *cons=0);

        LDAPMessageQueue* saslBind(const std::string& mech, 
                const std::string& cred, 
                const LDAPConstraints *cons=0);

        LDAPMessageQueue* saslInteractiveBind(const std::string& mech,
                int flags=0,
                SaslInteractionHandler *sih=0,
                const LDAPConstraints *cons=0);

        /** Performing a search on a directory tree.
         *
         * Use the search method to perform a search on the LDAP-Directory
         * @throws LDAPException If the Request could not be sent to the
         *      destination server, a LDAPException-object contains the
         *      error that occured.
         * @param base The distinguished name of the starting point for the
         *      search operation
         * @param scope The scope of the search. Possible values: <BR> 
         *      LDAPAsynConnection::SEARCH_BASE, <BR> 
         *      LDAPAsynConnection::SEARCH_ONE, <BR>
         *      LDAPAsynConnection::SEARCH_SUB
         * @param filter The std::string representation of a search filter to
         *      use with this operation
         * @param attrsOnly true if only the attributes names (no values) 
         *      should be returned
         * @param cons A set of constraints that should be used with this
         *      request
         */
        LDAPMessageQueue* search(const std::string& base="", int scope=0, 
                                 const std::string& filter="objectClass=*", 
                                 const StringList& attrs=StringList(), 
                                 bool attrsOnly=false,
                                 const LDAPConstraints *cons=0);

        /** Delete an entry from the directory
         *
         * This method sends a delete request to the server
         * @throws LDAPException If the Request could not be sent to the
         *      destination server, a LDAPException-object contains the
         *      error that occured.
         * @param dn    Distinguished name of the entry that should be deleted
         * @param cons  A set of constraints that should be used with this
         *              request
         */
        LDAPMessageQueue* del(const std::string& dn, const LDAPConstraints *cons=0);

        /** 
         * Perform the COMPARE-operation on an attribute 
         *
         * @throws LDAPException If the Request could not be sent to the
         *      destination server, a LDAPException-object contains the
         *      error that occured.
         * @param dn    Distinguished name of the entry for which the compare
         *              should be performed
         * @param attr  An Attribute (one (!) value) to use for the
         *      compare operation
         * @param cons  A set of constraints that should be used with this
         *              request
         */
        LDAPMessageQueue* compare(const std::string& dn, 
                const LDAPAttribute& attr, 
                const LDAPConstraints *cons=0);

        /** Add an entry to the directory
         *
         * @throws LDAPException If the Request could not be sent to the
         *      destination server, a LDAPException-object contains the
         *      error that occured.
         * @param le The entry that will be added to the directory
         */
        LDAPMessageQueue* add( const LDAPEntry* le,
                const LDAPConstraints *const=0);

        /** Apply modifications to attributes of an entry
         *
         * @throws LDAPException If the Request could not be sent to the
         *      destination server, a LDAPException-object contains the
         *      error that occured.
         * @param dn Distiguished Name of the Entry to modify
         * @param modlist A set of modification that should be applied
         *      to the Entry
         * @param cons  A set of constraints that should be used with this
         *              request
         */
        LDAPMessageQueue* modify(const std::string& dn, 
                const LDAPModList *modlist,
                const LDAPConstraints *cons=0);

        /** modify the DN of an entry
         *
         * @throws LDAPException If the Request could not be sent to the
         *      destination server, a LDAPException-object contains the
         *      error that occured.
         * @param dn            DN to modify
         * @param newRDN        The new relative DN for the entry
         * @param delOldRDN     true=The old RDN will be removed from the 
         *                      attributes <BR>
         *                      false=The old RDN will still be present in the
         *                      attributes of the entry
         * @param newParentDN   The DN of the new parent entry of the entry
         *                      0 to keep the old one
         */
        LDAPMessageQueue* rename(const std::string& dn, 
                const std::string& newRDN,
                bool delOldRDN=false, const std::string& newParentDN="",
                const LDAPConstraints* cons=0);

        /** Perform a LDAP extended Operation
         *
         * @throws LDAPException If the Request could not be sent to the
         *      destination server, a LDAPException-object contains the
         *      error that occured.
         * @param oid The dotted decimal representation of the extended 
         *      Operation that should be performed
         * @param value The data asociated with this operation
         * @param cons  A set of constraints that should be used with this
         *              request
         */
        LDAPMessageQueue* extOperation(const std::string& oid, 
                const std::string& value="", const LDAPConstraints *cons=0);

        /** End an outstanding request
         *
         * @param q All outstanding request related to this LDAPMessageQueue 
         *      will be abandoned
         */
        void abandon(LDAPMessageQueue *q);

        /**
         * Performs the UNBIND-operation on the destination server
         * 
         * @throws LDAPException in any case of an error
         */
        void unbind();

        /**
         * @returns The C-APIs LDAP-structure that is associated with the
         *      current connection 
         */
        LDAP* getSessionHandle() const ;

        /**
         * @returns The Hostname of the destination server of the
         *      connection. 
         */
        const std::string& getHost() const;

        /**
         * @returns The Port to which this connection is connecting to on
         *      the remote server. 
         */
        int getPort() const;

        /** Change the default constraints of the connection
         *
         * @parameter cons cons New LDAPConstraints to use with the connection
         */
        void setConstraints(LDAPConstraints *cons);

        /** Get the default constraints of the connection
         *
         * @return Pointer to the LDAPConstraints-Object that is currently
         *      used with the Connection
         */
        const LDAPConstraints* getConstraints() const;
        TlsOptions getTlsOptions() const;
        /**
         * This method is used internally for automatic referral chasing.
         * It tries to bind to a destination server of the URLs of a
         * referral.
         *
         * @throws LDAPException in any case of an error
         * @param urls Contains a std::list of LDAP-Urls that indicate the
         *      destinations of a referral
         * @param usedUrl After this method has successfully bind to one of
         *      the Destination URLs this parameter contains the URLs 
         *      which was contacted.
         * @param cons An LDAPConstraints-Object that should be used for
         *      the new connection. If this object contains a 
         *      LDAPRebind-object it is used to bind to the new server
         */ 
        LDAPAsynConnection* referralConnect(const LDAPUrlList& urls,
                LDAPUrlList::const_iterator& usedUrl,
                const LDAPConstraints* cons) const;

    private :
        /**
         * Private copy constructor. So nobody can call it.
         */
        LDAPAsynConnection(const LDAPAsynConnection& lc){};

        /**
         * A pointer to the C-API LDAP-structure that is associated with
         * this connection
         */
        LDAP *cur_session;

        /**
         * A pointer to the default LDAPConstrains-object that is used when
         * no LDAPConstraints-parameter is provided with a call for a
         * LDAP-operation
         */
        LDAPConstraints *m_constr;

        /**
         * The URI of this connection
         */
        LDAPUrl m_uri;

    protected:
        /**
         * Is caching enabled?
         */
        bool m_cacheEnabled;
};
#endif //LDAP_ASYN_CONNECTION_H


