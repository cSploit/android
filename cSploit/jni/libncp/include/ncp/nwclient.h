/*
    nwclient.h
    Copyright (C) 2001  Patrick Pollet

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


    Revision history:


	1.00  2000, February 27		Patrick Pollet
		Initial version.

	1.01  2000, February 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Move generic defines into nwcalls.h.

 */

#ifndef __NWCLIENT_H__
#define __NWCLIENT_H__

#include <ncp/nwnet.h>

#define ATTR_LOCATION	"L"

#define ATTR_LOGIN_SCRIPT "Login Script"
#define ATTR_GRP_MBS      "Group Membership"
#define ATTR_HOST_SERVER  "Host Server"
#define ATTR_HOST_RN     "Host Resource Name"
#define ATTR_SMTP_EMAIL  "Email Address"
#define ATTR_LDAP_EMAIL  "Internet Email Address"
#define ATTR_FULL_NAME   "Full Name"
#define ATTR_SURNAME     "Surname"
#define ATTR_GIVEN_NAME  "Given Name"
#define ATTR_POSTAL_ADDRESS     "Postal Address"
#define ATTR_HOME_NW     "Home Directory"
#define ATTR_MESSAGE_SERVER "Message Server"
#define ATTR_LOGIN_TIME  "Login Time"
#define ATTR_PROFILE      "Profile"

// the proper naming attribute may be customized here (must be a CI_STRING )
#define ATTR_GECOS        ATTR_FULL_NAME


// syntaxes of the used attributes
#define SYN_LOCATION     SYN_CI_STRING
#define SYN_LOGIN_SCRIPT SYN_STREAM
#define SYN_UID          SYN_INTEGER
#define SYN_PGNAME 	 SYN_DIST_NAME
#define SYN_PGID         SYN_INTEGER
#define SYN_GID          SYN_INTEGER
#define SYN_SHELL        SYN_CE_STRING
#define SYN_COM          SYN_CI_STRING
#define SYN_HOME	 SYN_CE_STRING
#define SYN_GRP_MBS      SYN_DIST_NAME
#define SYN_PROFILE      SYN_DIST_NAME

#ifdef __cplusplus
extern "C" {
#endif




/* caution to follow the NWIsDSServer convention all NWCXIs* fonctions
  return non zero if OK */
int NWCXIsDSServer ( NWCONN_HANDLE conn, NWDSChar * treeName);
/* same as API return 1 if OK but removes the trailings '_____'*/
int NWCXIsSameTree (NWCONN_HANDLE conn, const NWDSChar *treeName) ;
/*return 1 if conn points to the tree treeName */
int NWCXIsSameServer(NWCONN_HANDLE conn, const NWDSChar* server);
/*return 1 if conn points to the server server */
NWCCODE NWCXIsPermanentConnection (NWCONN_HANDLE conn);
/*return 1 if conn is permanent */
NWDSCCODE NWCXIsDSAuthenticated(NWDSContextHandle ctx);
/* return true if a context has authentication keys*/

NWDSCCODE NWCXGetPreferredDSTree    (NWDSChar * preferTree, size_t maxLen);
NWDSCCODE NWCXGetDefaultNameContext (const NWDSChar* treeName,NWDSChar * nameContext, size_t maxLen);
NWDSCCODE NWCXGetPreferredServer    (const NWDSChar * forTree,NWDSChar *preferedServer, size_t maxLen);
NWDSCCODE NWCXGetDefaultUserName (const NWDSChar * forTree,NWDSChar *defaultName, size_t maxLen);
/*may be we should return current user's Unix name if nothing found in env ? */
NWDSCCODE NWCXGetDefaultPassword (const NWDSChar * forTree,NWDSChar *defaultPwd, size_t maxLen);


NWCCODE NWCXGetPermConnInfo(NWCONN_HANDLE conn, nuint info, size_t len,void* buffer);
/* return infos specific top permanent connexions*/
/* fails if not permanent pass to NWCCGetConnInfo if needed             */
NWCCODE NWCXGetPermConnList (NWCONN_HANDLE* conns , int maxEntries, int* curEntries, uid_t uid);
/* returns the list of permanent connexions belonging to user uid*/
/* if uid=-1 = all Permanent connections (Root only can call it) */
NWCCODE NWCXGetPermConnListByTreeName (NWCONN_HANDLE* conns, int maxEntries,
                                      int* curEntries, uid_t uid,
                                      const NWDSChar *treeName);
/* returns the list of permanent connexions belonging to user uid*/
/* and related to tree treeName */
NWCCODE NWCXGetPermConnListByServerName (NWCONN_HANDLE* conns , int maxEntries,
                                      int* curEntries, uid_t uid,
                                      const NWDSChar *serverName);
/* returns the list of permanent connexions belonging to user uid*/
/* and related to server serverName */


NWDSCCODE NWDSCreateContextHandleMnt(NWDSContextHandle* ctx, const NWDSChar * treeName);
/* create a context ,add to it all permanent connections related to treeName */
/* belonging to me (by using getuid()) and  */


NWDSCCODE NWCXAttachToTreeByName( NWCONN_HANDLE* conn, const NWDSChar * treeName);
/* open an unauthenticated connection to the first visible server belonging to treeName*/


/* misc NDS properties reading functions to alleviate writing future utilities */

NWDSCCODE NWCXGetNDSVolumeServerAndResourceName(NWDSContextHandle ctx,const NWDSChar* ndsName,
                                               NWDSChar* serverName, size_t serverNameMaxLen,
                                               NWDSChar *resourceName, size_t resourceNameMaxlen);
/* return server name (DN or RN) and volume name of a NDS volume name */

NWDSCCODE NWCXGetObjectHomeDirectory(NWDSContextHandle ctx,
                                     const NWDSChar* ndsName,
                                     NWDSChar* serverName,
                                     size_t serverNameMaxLen,
                                     NWDSChar *resourceName,
                                     size_t resourceNameMaxLen,
                                     NWDSChar* NDSvolumeName,
                                     size_t NDSVolumeNameMaxLen,
                                     NWDSChar* pathName,
                                     size_t pathNameMaxLen);

NWDSCCODE NWCXGetObjectLastLoginTime(NWDSContextHandle ctx,
                                     const NWDSChar* ndsName,
                                     time_t * tm);

NWDSCCODE NWCXGetObjectMessageServer(NWDSContextHandle ctx,
                                     const NWDSChar* ndsName,
                                     NWDSChar* serverName,
                                     int serverNameMaxLen);


NWDSCCODE NWCXGetObjectLoginScript(NWDSContextHandle ctx,const NWDSChar* objectName,
                                        char* buffer, int * len, int maxlen);

NWDSCCODE NWCXGetContextLoginScript(NWDSContextHandle ctx,const NWDSChar* objectName,
                                        char* buffer, int * len,int maxlen);

NWDSCCODE NWCXGetProfileLoginScript(NWDSContextHandle ctx,const NWDSChar* objectName,
                                        char* buffer, int * len, int maxlen);

/* read a single valued string attribute */
NWDSCCODE NWCXGetStringAttributeValue (NWDSContextHandle ctx,
                                        const NWDSChar* objectName,
                                        const NWDSChar* attrName,
                                        char* buffer, int maxlen);

/* read a single valued numeric attribute */
NWDSCCODE NWCXGetIntAttributeValue     (NWDSContextHandle ctx,
                                        const NWDSChar* objectName,
                                        const NWDSChar* attrName,
                                        int * value);

/* read & convert to string a single valued attribute */
NWDSCCODE NWCXGetAttributeValueAsString (NWDSContextHandle ctx,
                                        const NWDSChar* objectName,
                                        const NWDSChar* attrName,
                                        char* buffer, size_t maxlen);

/* read a multi valued string attribute */
/* caller MUST free the buffer created here */
NWDSCCODE NWCXGetMultiStringAttributeValue (NWDSContextHandle ctx,
                                        const NWDSChar* objectName,
                                        const NWDSChar* attrName,
                                        char** buffer) ;


/* misc utilities */
NWDSCCODE NWCXSplitNameAndContext (NWDSContextHandle ctx,const NWDSChar * dn,
                                   char * name, char* context);

#ifdef __cplusplus
}
#endif

#endif	/* __NWCLIENT_H__ */
