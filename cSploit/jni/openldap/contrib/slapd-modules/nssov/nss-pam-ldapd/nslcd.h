/*
   nslcd.h - file describing client/server protocol

   Copyright (C) 2006 West Consulting
   Copyright (C) 2006, 2007, 2009, 2010, 2011, 2012 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#ifndef _NSLCD_H
#define _NSLCD_H 1

/*
   The protocol used between the nslcd client and server is a simple binary
   protocol. It is request/response based where the client initiates a
   connection, does a single request and closes the connection again. Any
   mangled or not understood messages will be silently ignored by the server.

   A request looks like:
     INT32  NSLCD_VERSION
     INT32  NSLCD_ACTION_*
     [request parameters if any]
   A response looks like:
     INT32  NSLCD_VERSION
     INT32  NSLCD_ACTION_* (the original request type)
     [result(s)]
     INT32  NSLCD_RESULT_END
   A single result entry looks like:
     INT32  NSLCD_RESULT_BEGIN
     [result value(s)]
   If a response would return multiple values (e.g. for NSLCD_ACTION_*_ALL
   functions) each return value will be preceded by a NSLCD_RESULT_BEGIN
   value. After the last returned result the server sends
   NSLCD_RESULT_END. If some error occurs (e.g. LDAP server unavailable,
   error in the request, etc) the server terminates the connection to signal
   an error condition (breaking the protocol).

   These are the available basic data types:
     INT32  - 32-bit integer value
     TYPE   - a typed field that is transferred using sizeof()
     STRING - a string length (32bit) followed by the string value (not
              null-terminted) the string itself is assumed to be UTF-8
     STRINGLIST - a 32-bit number noting the number of strings followed by
                  the strings one at a time

   Furthermore the ADDRESS compound data type is defined as:
     INT32  type of address: e.g. AF_INET or AF_INET6
     INT32  lenght of address
     RAW    the address itself in network byte order
   With the ADDRESSLIST using the same construct as with STRINGLIST.

   The protocol uses host-byte order for all types (except in the raw
   address above).
*/

/* The current version of the protocol. Note that version 1
   is experimental and this version will be used until a
   1.0 release of nss-pam-ldapd is made. */
#define NSLCD_VERSION 1

/* Get a NSLCD configuration option. There is one request parameter:
    INT32   NSLCD_CONFIG_*
  the result value is:
    STRING  value, interpretation depending on request */
#define NSLCD_ACTION_CONFIG_GET        20006

/* return the message, if any, that is presented to the user when password
   modification through PAM is prohibited */
#define NSLCD_CONFIG_PAM_PASSWORD_PROHIBIT_MESSAGE  852

/* Email alias (/etc/aliases) NSS requests. The result values for a
   single entry are:
     STRING      alias name
     STRINGLIST  alias rcpts */
#define NSLCD_ACTION_ALIAS_BYNAME       4001
#define NSLCD_ACTION_ALIAS_ALL          4002

/* Ethernet address/name mapping NSS requests. The result values for a
   single entry are:
     STRING            ether name
     TYPE(uint8_t[6])  ether address */
#define NSLCD_ACTION_ETHER_BYNAME       3001
#define NSLCD_ACTION_ETHER_BYETHER      3002
#define NSLCD_ACTION_ETHER_ALL          3005

/* Group and group membership related NSS requests. The result values
   for a single entry are:
     STRING       group name
     STRING       group password
     TYPE(gid_t)  group id
     STRINGLIST   members (usernames) of the group
     (not that the BYMEMER call returns an emtpy members list) */
#define NSLCD_ACTION_GROUP_BYNAME       5001
#define NSLCD_ACTION_GROUP_BYGID        5002
#define NSLCD_ACTION_GROUP_BYMEMBER     5003
#define NSLCD_ACTION_GROUP_ALL          5004

/* Hostname (/etc/hosts) lookup NSS requests. The result values
   for an entry are:
     STRING       host name
     STRINGLIST   host aliases
     ADDRESSLIST  host addresses */
#define NSLCD_ACTION_HOST_BYNAME        6001
#define NSLCD_ACTION_HOST_BYADDR        6002
#define NSLCD_ACTION_HOST_ALL           6005

/* Netgroup NSS request return a number of results. Result values
   can be either a reference to another netgroup:
     INT32   NSLCD_NETGROUP_TYPE_NETGROUP
     STRING  other netgroup name
   or a netgroup triple:
     INT32   NSLCD_NETGROUP_TYPE_TRIPLE
     STRING  host
     STRING  user
     STRING  domain */
#define NSLCD_ACTION_NETGROUP_BYNAME   12001
#define NSLCD_NETGROUP_TYPE_NETGROUP 123
#define NSLCD_NETGROUP_TYPE_TRIPLE   456

/* Network name (/etc/networks) NSS requests. Result values for a single
   entry are:
     STRING       network name
     STRINGLIST   network aliases
     ADDRESSLIST  network addresses */
#define NSLCD_ACTION_NETWORK_BYNAME     8001
#define NSLCD_ACTION_NETWORK_BYADDR     8002
#define NSLCD_ACTION_NETWORK_ALL        8005

/* User account (/etc/passwd) NSS requests. Result values are:
     STRING       user name
     STRING       user password
     TYPE(uid_t)  user id
     TYPE(gid_t)  group id
     STRING       gecos information
     STRING       home directory
     STRING       login shell */
#define NSLCD_ACTION_PASSWD_BYNAME      1001
#define NSLCD_ACTION_PASSWD_BYUID       1002
#define NSLCD_ACTION_PASSWD_ALL         1004

/* Protocol information requests. Result values are:
     STRING      protocol name
     STRINGLIST  protocol aliases
     INT32       protocol number */
#define NSLCD_ACTION_PROTOCOL_BYNAME    9001
#define NSLCD_ACTION_PROTOCOL_BYNUMBER  9002
#define NSLCD_ACTION_PROTOCOL_ALL       9003

/* RPC information requests. Result values are:
     STRING      rpc name
     STRINGLIST  rpc aliases
     INT32       rpc number */
#define NSLCD_ACTION_RPC_BYNAME        10001
#define NSLCD_ACTION_RPC_BYNUMBER      10002
#define NSLCD_ACTION_RPC_ALL           10003

/* Service (/etc/services) information requests. Result values are:
     STRING      service name
     STRINGLIST  service aliases
     INT32       service (port) number
     STRING      service protocol */
#define NSLCD_ACTION_SERVICE_BYNAME    11001
#define NSLCD_ACTION_SERVICE_BYNUMBER  11002
#define NSLCD_ACTION_SERVICE_ALL       11005

/* Extended user account (/etc/shadow) information requests. Result
   values for a single entry are:
     STRING  user name
     STRING  user password
     INT32   last password change
     INT32   mindays
     INT32   maxdays
     INT32   warn
     INT32   inact
     INT32   expire
     INT32   flag */
#define NSLCD_ACTION_SHADOW_BYNAME      2001
#define NSLCD_ACTION_SHADOW_ALL         2005

/* PAM-related requests. The request parameters for all these requests
   begin with:
     STRING  user name
     STRING  DN (if value is known already, otherwise empty)
     STRING  service name
   all requests, except the SESSION requests start the result value with:
     STRING  user name (cannonical name)
     STRING  DN (can be used to speed up requests)
   Some functions may return an authorisation message. This message, if
   supplied will be used by the PAM module instead of a message that is
   generated by the PAM module itself. */

/* PAM authentication check request. The extra request values are:
     STRING  password
   and the result value ends with:
     INT32   authc NSLCD_PAM_* result code
     INT32   authz NSLCD_PAM_* result code
     STRING  authorisation error message
   If the username is empty in this request an attempt is made to
   authenticate as the administrator (set using rootpwmoddn). The returned DN
   is that of the administrator. */
#define NSLCD_ACTION_PAM_AUTHC         20001

/* PAM authorisation check request. The extra request values are:
     STRING ruser
     STRING rhost
     STRING tty
   and the result value ends with:
     INT32   authz NSLCD_PAM_* result code
     STRING  authorisation error message */
#define NSLCD_ACTION_PAM_AUTHZ         20002

/* PAM session open and close requests. These requests have the following
   extra request values:
     STRING tty
     STRING rhost
     STRING ruser
     INT32 session id (ignored for SESS_O)
   and these calls only return the session ID:
     INT32 session id
   The SESS_C must contain the ID that is retured by SESS_O to close the
   correct session. */
#define NSLCD_ACTION_PAM_SESS_O        20003
#define NSLCD_ACTION_PAM_SESS_C        20004

/* PAM password modification request. This requests has the following extra
   request values:
     STRING old password
     STRING new password
   and returns there extra result values:
     INT32   authz NSLCD_PAM_* result code
     STRING  authorisation error message
   In this request the DN may be set to the administrator's DN. In this
   case old password should be the administrator's password. This allows
   the administrator to change any user's password. */
#define NSLCD_ACTION_PAM_PWMOD         20005

/* Request result codes. */
#define NSLCD_RESULT_BEGIN                 0
#define NSLCD_RESULT_END                   3

/* Partial list of PAM result codes. */
#define NSLCD_PAM_SUCCESS             0 /* everything ok */
#define NSLCD_PAM_PERM_DENIED         6 /* Permission denied */
#define NSLCD_PAM_AUTH_ERR            7 /* Authc failure */
#define NSLCD_PAM_CRED_INSUFFICIENT   8 /* Cannot access authc data */
#define NSLCD_PAM_AUTHINFO_UNAVAIL    9 /* Cannot retrieve authc info */
#define NSLCD_PAM_USER_UNKNOWN       10 /* User not known */
#define NSLCD_PAM_MAXTRIES           11 /* Retry limit reached */
#define NSLCD_PAM_NEW_AUTHTOK_REQD   12 /* Password expired */
#define NSLCD_PAM_ACCT_EXPIRED       13 /* Account expired */
#define NSLCD_PAM_SESSION_ERR        14 /* Cannot make/remove session record */
#define NSLCD_PAM_AUTHTOK_ERR        20 /* Authentication token manipulation error */
#define NSLCD_PAM_AUTHTOK_DISABLE_AGING 23 /* Password aging disabled */
#define NSLCD_PAM_IGNORE             25 /* Ignore module */
#define NSLCD_PAM_ABORT              26 /* Fatal error */
#define NSLCD_PAM_AUTHTOK_EXPIRED    27 /* authentication token has expired */

#endif /* not _NSLCD_H */
