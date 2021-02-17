/*
    php_auth_nds.c - User verification for ncpfs
    Copyright (C) 2000, 2001  Petr Vandrovec

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

	1.00  2000, September 30	Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision, used on dialog.cvut.cz.

	1.01  2001, January 10		Petr Vandrovec <vandrove@vc.cvut.cz>
		Further polishing, used on cdonline.cvut.cz.

	1.01  2001, February 19		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license, polished for release, added sample code.

	1.02  2001, March 25		Patrick Pollet
		Added some NWDSFreeContext in case of errors
		Added group parameter (NDS name or bindery)

	1.03  2001, March 28		Patrick Pollet
		Added NDS reading attribute functions
			string read_nds_string (tree, object, attribute)
			string read_nds_num (tree, object, attribute)

	1.04  2001, October 27		Patrick Pollet
		Added nds_tree_auth allowing a tree name and a list of context 
			to be searched
		Simplified code by adding internal CreateCtx and CreateCtxAndConn 
			(called by all NDS related functions)

	1.05  2002, May 17		Patrick Pollet
		Added contextless login when authenticating to NDS tree

	1.06  2003, Feb 20		Patrick Pollet
		Use NWCCOpenConnByName instead of ncp_open in nds_auth_fn 
			(auth against a NDS server)
		Corrected the code of nds_read_int_fn that **was** returning 
			the value converted to string (!) so boolean returned 
			true/false and time stamp were converted to localized 
			time... Now it return the numeric value (still in a PHP 
			string).

	1.07  2003, July 22		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added read_nds_string2 and read_nds_int2 to fetch NDS data against 
			a server in a pure IP environnment where NWCXAttachTotreeByName 
			usually fails with err 0x8847
 */

/*

Original example:

Expects uid as username, srv as server name and pwd as password. If server name
is FDNET or ST, it uses NDS username $uid.FADOP resp. $uid.STUDENT.CVUT-FSV. For
other servers it uses simple bindery login... If password verification passes,
$ss_username and $valid_user are set to $srv/$uid - you can use session_register
on one of them.

if ($HTTP_POST_VARS["uid"] == "")
  break;
if ($HTTP_POST_VARS["srv"] == "") {
  $auth_err = "Wrong URL";
} else if ($HTTP_POST_VARS["pwd"] == "") {
  $auth_err = "Invalid password";
} else {
  $uu = strtoupper($HTTP_POST_VARS["uid"]);
  if ($HTTP_POST_VARS["srv"] == "FDNET") {
    $auth_err = auth_nds($HTTP_POST_VARS["srv"], $uu . ".FADOP", strtoupper($HTTP_POST_VARS["pwd"]));
  } else if ($HTTP_POST_VARS["srv"] == "ST") {
    $auth_err = auth_nds($HTTP_POST_VARS["srv"], $uu . ".STUDENT.CVUT-FSV", strtoupper($HTTP_POST_VARS["pwd"]));
  } else {
    $auth_err = auth_bindery($HTTP_POST_VARS["srv"], $uu, $HTTP_POST_VARS["pwd"]);
  }
  if ($auth_err == "") {
    $valid_user = $HTTP_POST_VARS["srv"] . "/" . $uu;
    $ss_username = $valid_user;
  }
}

for more examples see /contrib/php/site
*/


//#include "config.h"
#include <php.h>
#include <ext/standard/info.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <ncp/nwclient.h>
#include <Zend/zend_modules.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>
#include <syslog.h>
#include <libintl.h>
#define _(X) gettext(X)

static int is_member_of_group_bind( char* errstr,NWCONN_HANDLE conn,const char* user,const char* group) {
	int err;

	err = NWIsObjectInSet(conn, user, NCP_BINDERY_USER, "GROUPS_I'M_IN", group, NCP_BINDERY_UGROUP);
	if (!err) {
		err= NWIsObjectInSet(conn, group, NCP_BINDERY_UGROUP, "GROUP_MEMBERS", user, NCP_BINDERY_USER);
		if (!err)
			return 0;
		else {
			sprintf(errstr, "inconsistent bindery database for user %s and group %s: %s",
				user, group, strnwerror(err));
		}
	} else {
		sprintf (errstr,"user %s is not member of %s: %s", user, group, strnwerror(err));
	}
	return -1;
}

static int is_member_of_group_nds( char* errstr,NWDSContextHandle ctx,NWCONN_HANDLE conn,NWObjectID oid,const char* group) {
	int eval = 0;
	Buf_T* buf=NULL;
	NWDSCCODE dserr;
	nbool8 match;

	dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &buf);
	if (dserr) {
		sprintf(errstr, "NWDSAllocBuf() failed with %s\n", strnwerror(dserr));
		eval = 120;
		goto bailout;
	}
	dserr = NWDSInitBuf(ctx, DSV_COMPARE, buf);
	if (dserr) {
	        sprintf(errstr, "NWDSInitBuf() failed with %s\n", strnwerror(dserr));
	        eval = 121;
	        goto bailout;
	}
	dserr = NWDSPutAttrName(ctx, buf, "Group Membership");
	if (dserr) {
	        sprintf(errstr, "NWDSPutAttrName() failed with %s\n", strnwerror(dserr));
	        eval = 122;
	        goto bailout;
        }
	dserr = NWDSPutAttrVal(ctx, buf, SYN_DIST_NAME, group);
	if (dserr) {
	        sprintf(errstr, "NWDSPutAttrVal() failed with %s\n", strnwerror(dserr));
	        eval = 123;
	        goto bailout;
	}
	dserr = __NWDSCompare(ctx, conn, oid, buf, &match);
	if (dserr) {
	        sprintf(errstr, "__NWDSCompare() failed with %s(oid=%x)\n", strnwerror(dserr), oid);
	        eval = 124;
	        goto bailout;
	}
	if (!match) {
	        sprintf(errstr, "Not member of NDS group %s\n",  group);
	        eval=125;
	}
bailout:
	if (buf)
	        NWDSFreeBuf(buf);
        return eval;
}

/* internal; called by all functions requiring a context */
static int _createCtx ( char * errstr, NWDSContextHandle *ctx) {

	NWDSCCODE dserr;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;
	int eval = 0;

	dserr = NWDSCreateContextHandle(ctx);
	if (dserr) {
		sprintf(errstr, "NWDSCreateContextHandle failed with %s\n", strnwerror(dserr));
		return 100;
	}

	ctxflag = DCV_XLATE_STRINGS | DCV_DEREF_ALIASES | DCV_TYPELESS_NAMES;

	dserr = NWDSSetContext(*ctx, DCK_FLAGS, &ctxflag);
	if (dserr) {
		sprintf(errstr, "NWDSSetContext(DCK_FLAGS) failed: %s\n", strnwerror(dserr));
		eval=101;
		goto bailout;
	}
	dserr = NWDSSetContext(*ctx, DCK_NAME_CONTEXT, "");
	if (dserr) {
		sprintf(errstr, "NWDSSetContext(DCK_NAME_CONTEXT) failed: %s\n",strnwerror(dserr));
		eval=102;
		goto bailout;
	}
	dserr = NWDSSetContext(*ctx, DCK_CONFIDENCE, &confidence);
	if (dserr) {
		sprintf(errstr, "NWDSSetContext(DCK_CONFIDENCE) failed: %s\n",strnwerror(dserr));
		eval=103;
		goto bailout;
	}
	     /* success */
	return 0;
	   /*failure */
bailout:
             dserr = NWDSFreeContext(*ctx);
        	    if (dserr) {
		sprintf(errstr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		eval=105;
	        }
	return eval;
}

/* internnal; called by all functions requiring a connection to a tree */
static int _createCtxAndConn (char * errstr, const char* treename, NWDSContextHandle *ctx, NWCONN_HANDLE *conn) {

             NWDSCCODE dserr;
	int eval = 0;

	eval=_createCtx (errstr,ctx);
	if (eval)
	        return eval;
	dserr= NWCXAttachToTreeByName(conn, treename);
	if (dserr) {
		sprintf(errstr, "NWCXAttachToTreeByName failed with %s\n", strnwerror(dserr));
		eval =99;
		goto bailout;
	}

	dserr = NWDSAddConnection(*ctx, *conn);
	if (dserr) {
		sprintf(errstr, "Cannot bind connection to context: %s\n",strnwerror(dserr));
		eval=104;
		goto bailout;
	}
 /* success */
	//sprintf (errstr,"tout va bien\n");
	return 0;
	   /*failure */
bailout:
	if (conn) NWCCCloseConn(*conn);
	dserr = NWDSFreeContext(*ctx);
	if (dserr) {
		sprintf(errstr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		eval=105;
	}
	return eval;
}



static int bindery_auth (char* errstr, const char* server, const char* user, const char* pwd,
                        const char * group)
{
	struct ncp_conn *conn;
	struct ncp_conn_spec spec;
	long err;
	char upperpw[256];
	int i;

	if (strlen(pwd) > 255) {
		sprintf(errstr, "Specified password is too long");
		return 1;
	}
	for (i = 0; pwd[i]; i++) {
		upperpw[i] = toupper(pwd[i]);
	}
	upperpw[i] = 0;
	spec.uid = ~0;	/* ensure that ncp_open will not use permanent connection */
	spec.server[0] = 0;
	spec.user[0] = 0;
	strcpy(spec.server, server);

	if ((conn = ncp_open(&spec, &err)) == NULL) {
		sprintf(errstr, "Cannot connect to server: %s", strnwerror(err));
		return 1;
	}
/*  useless code ???
	 {
	  NWCONN_NUM num;
	 err=NWGetConnectionNumber(conn, &num);
	 }
*/
	/* Bindery authorization */
	err = NWVerifyObjectPassword(conn, user, OT_USER, upperpw);

	if (err) {
		sprintf(errstr, "Wrong credentials: %s", strnwerror(err));

	}else {
	        if (group[0]) {
	                err=is_member_of_group_bind(errstr,conn,user,group);
	        }
	}
	NWCCCloseConn(conn);
	return err;
}


/* authenticate to a NDS server */
static int nds_auth_fn(char* errstr, const char* server, const char* objectname, const char* pwd,
                        const char * group)
{
	NWDSCCODE dserr;
	NWDSContextHandle ctx=NULL;
	NWCONN_HANDLE conn=NULL;
	int eval = 0;
	NWObjectID oid;

	eval=_createCtx (errstr,&ctx);
	if (eval)
	        return eval;

	if (server[0] == '/') {
		dserr = ncp_open_mount(server, &conn);
		if (dserr) {
			sprintf(errstr, "ncp_open_mount failed with %s\n", strnwerror(dserr));
			eval=104;
			goto bailout;
		}
	} else {
		dserr = NWCCOpenConnByName(NULL,server, NWCC_NAME_FORMAT_BIND, 0, 0, &conn);
		if (dserr) {
			sprintf(errstr, "ncp_open failed with %s\n",strnwerror(dserr));
			eval=105;
			goto bailout;
		}
	}
	dserr = NWDSAddConnection(ctx, conn);
	if (dserr) {
		sprintf(errstr, "Cannot bind connection to context: %s\n",strnwerror(dserr));
		eval=106;
		goto bailout;
	}

	dserr = NWDSVerifyObjectPassword(ctx, NDS_PASSWORD, objectname, pwd);
	if (dserr) {
		sprintf(errstr, "Verify password failed: %s\n",
			strnwerror(dserr));
		eval = 110;

	} else {
	        if (group[0])   {
		dserr=NWDSMapNameToID(ctx, conn,objectname,&oid);
		if (dserr) {
		        sprintf(errstr, "%s when retrieving object ID\n", strnwerror(dserr));
		         eval =119;
		         goto bailout;
	            }
	            eval=is_member_of_group_nds(errstr,ctx,conn,oid,group);
	        }
	     }
bailout:

	if (conn) NWCCCloseConn(conn);
	if (ctx) {
	        dserr = NWDSFreeContext(ctx);
        	        if (dserr) {
			sprintf(errstr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
			eval=111;
	        }
	}
	return eval;
}


// this code was contributed by Jean Francois Burdet <jean.francois.burdet@adm.unige.ch>
// in the pam authentication module
// to implement a contextless login if a list of context to serach is not provided in the command line
// ported to this php module by PP
// ctx and a conn have been set by caller and conn added to ctx
static NWDSCCODE nw_ctx_search(
	char * errstr,
	NWDSContextHandle context,
	const char * user_cn,
	char * contexts,
	size_t maxsize) {

	NWDSCCODE ccode;
	nuint32	iterationHandle = NO_MORE_ITERATIONS;
	nint32	countObjectsSearched;
	nuint32	objCntr;
	nuint32	objCount;
	char	objectName[MAX_DN_CHARS+1];
	size_t	ctxp=0;
	int cnt=0;


	// buffers
	pBuf_T	searchFilter=NULL;	// search filter
	pBuf_T	retBuf=NULL;		// result buffer for NWDSSearch
	Filter_Cursor_T* cur=NULL;	// search expression tree temporary buffer



	if (!contexts || maxsize < MAX_DN_CHARS+1)
		return EINVAL;

	/*******************************************************************
	In order to search, we need:
		A Filter Cursor (to build the search expression)
		A Filter Buffer (to store the expression; used by NWDSSearch)
		A Buffer to store which attributes we need information on
		A Result Buffer (to store the search results)
	*/

	/*******************************************************************
	** Allocate Filter buffer and Cursor and populate
	*/
	ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN,&searchFilter);
	if (ccode ) {
		sprintf(errstr,"nw_ctx_search:NWDSAllocBuf returned: %d\n", ccode);
		goto Exit4;
	}

	// Initialize the searchFilter buffer
	ccode = NWDSInitBuf(context,DSV_SEARCH_FILTER,searchFilter);
	if (ccode) {
		sprintf(errstr,"nw_ctx_search:NWDSInitBuf returned: %d\n", ccode);
		goto Exit4;
	}

	// Allocate a filter cursor to put the search expression
	ccode = NWDSAllocFilter(&cur);
	if (ccode) {
		sprintf(errstr,"nw_ctx_search:NWDSAllocFilter returned: %d\n", ccode);
		goto Exit4;
	}

	// Build the expression tree in cur, then place into searchFilter
	// Object Class = User AND CN =  user_cn
	ccode = NWDSAddFilterToken(cur,FTOK_ANAME,"Object Class",SYN_CLASS_NAME);
	ccode = NWDSAddFilterToken(cur,FTOK_EQ,NULL,0);
	ccode = NWDSAddFilterToken(cur,FTOK_AVAL,"User",SYN_CLASS_NAME);
	ccode = NWDSAddFilterToken(cur,FTOK_AND,NULL,0);
	ccode = NWDSAddFilterToken(cur,FTOK_ANAME,"CN",SYN_CI_STRING);
	ccode = NWDSAddFilterToken(cur,FTOK_EQ,NULL,0);
	ccode = NWDSAddFilterToken(cur,FTOK_AVAL,(char *)user_cn,SYN_CI_STRING);
	ccode = NWDSAddFilterToken(cur,FTOK_END,NULL,0);

	ccode = NWDSPutFilter(context,searchFilter,cur,NULL);
	if (ccode ) {
			sprintf(errstr,"nw_ctx_search:NWDSPutFilter returned: %d\n", ccode);
			goto Exit4;
	}
	else
		cur=NULL;


	ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN,&retBuf);
	if (ccode ) {
			sprintf(errstr,"nw_ctx_search:NWDSAllocBuf returned: %d\n", ccode);
			goto Exit4;
	}

	iterationHandle = NO_MORE_ITERATIONS;
	ctxp = 0;
	//syslog(LOG_NOTICE, "debut de recherche\n");
	// while NWDSSearch still can get some objects...
	do {

		ccode = NWDSSearch(context,
				"[Root]",
				DS_SEARCH_SUBTREE,
				0,		// don't dereference aliases
				searchFilter,
				0,		// we want attributes and values
				0,		// only want information in attrNames
				NULL,
				&iterationHandle,
				0,		// reserved
				&countObjectsSearched,
				retBuf);
		if (ccode) {
			sprintf(errstr,"nw_ctx_search:NWDSSearch returned: %s\n", strnwerror(ccode));
			goto Exit4;
		}
		syslog(LOG_NOTICE, "passe de recherche %d \n",cnt);
		// count the object returned in the buffer
		ccode = NWDSGetObjectCount(context,retBuf,&objCount);
		if (ccode) {
			sprintf(errstr,"nw_ctx_search:NWDSGetObjectCount returned: %d\n", ccode);
			goto Exit4;
		}

		// for the number of objects returned...
		syslog(LOG_NOTICE, "trouvé %d \n",objCount);

		for (objCntr=0;objCntr<objCount;objCntr++) {
			char* p;
			size_t ln;

			// get an object name
			ccode = NWDSGetObjectName(context,retBuf,objectName,NULL,NULL);
			if (ccode) {
				sprintf(errstr,"nw_ctx_search:NWDSGetObjectName returned: %d\n", ccode);
				goto Exit4;
			}
			// now, we get the user context wich starts at the first . occurence in the string
			p = strchr(objectName,'.');
			if (!p) {
				break;
			}
			ln = strlen(p + 1);
			if (ctxp + ln >= maxsize) {
				break;
			}
			if (ctxp) {
				*contexts++ = ',';
				maxsize--;
			}
			memcpy(contexts, p + 1, ln);
			contexts += ln;
			maxsize -= ln;
			ctxp = 1;
		} // objCntr
	} while (iterationHandle != NO_MORE_ITERATIONS);
	 if (iterationHandle != NO_MORE_ITERATIONS) {
            // let's keep the final dserr as the 'out of memory error' from ptr->getval()
                NWDSCloseIteration(context, DSV_SEARCH_FILTER, iterationHandle);
        }
	//lets remove trailing ','
	if (ctxp)
		*contexts = 0;
	else {
		ccode=109;  // not found...
		sprintf(errstr,"Failure of contextless login: unknown user");

	}

Exit4:
	if (retBuf)
		NWDSFreeBuf(retBuf);
	if (cur)
		NWDSFreeFilter(cur, NULL);
	if (searchFilter)
		NWDSFreeBuf(searchFilter);
	syslog(LOG_NOTICE, "fin de ctxless login err= %d :%s\n",ccode,contexts);
	return ccode;

}


/* authenticate to a NDS tree and return FQDN of found user */
static int tree_auth_fn(char* errstr, const char* treename, const char* objectname, const char* contexts,const char* pwd,
                        const char * group, char * fqdn)
{
	NWDSCCODE dserr;
	NWDSContextHandle ctx=NULL;
	NWCONN_HANDLE conn=NULL;
	int eval = 0;
	const char * ctxStart;
	const char* ctxEnd;
	NWObjectID oid;
	char ctxbuf[4096]; 	// buffer to hold possible matches in a contexless login

	eval=_createCtxAndConn (errstr,treename, &ctx, &conn);
	if (eval)
	        return eval;
	openlog("pph_ncp_auth", LOG_PID, LOG_AUTHPRIV);
	if (!contexts[0]) {
	//if (!contexts) {
		ctxbuf[0]=0;
		if (nw_ctx_search(errstr,ctx,objectname,ctxbuf,sizeof(ctxbuf))) {
			eval =109;
			goto bailout;
		}
		ctxStart=ctxbuf;
      	}
	else
		ctxStart=contexts;

	/* scan the search contexts list */

	syslog(LOG_NOTICE, "début de recherche  %s :%s\n",ctxStart,contexts);
	do {
                strcpy(fqdn,objectname);
                if (ctxStart) {
                        char ctxBuffer[MAX_DN_CHARS];
                        strcat(fqdn,".");
                        ctxEnd=strchr(ctxStart,',');
                        if (ctxEnd) {
                               memcpy(ctxBuffer,ctxStart,ctxEnd - ctxStart);
                               ctxBuffer[ctxEnd-ctxStart]=0;
                               ctxEnd++;
                               strcat(fqdn,ctxBuffer);
                        }  else
                                strcat(fqdn,ctxStart);
                        ctxStart=ctxEnd;
                }
	    	eval=109;    // don't forget to reset it !!!!

                dserr=NWDSMapNameToID(ctx,conn,fqdn,&oid);
		//dserr=0;
		syslog(LOG_NOTICE, "essai avec %s %x %d\n",fqdn,oid,dserr);
		if (!dserr) {   //found a matching user
	        	dserr = NWDSVerifyObjectPassword(ctx, NDS_PASSWORD, fqdn, pwd);
	        	if (dserr) {   //password makes the difference
				syslog(LOG_NOTICE,"Verify password failed: %s\n", strnwerror(dserr));
				sprintf(errstr, "Verify password failed: %s\n", strnwerror(dserr));
				eval = 110;
	       		} else  {
				eval=0;     // got him
				break;
	       		}
	 	}
	} while (ctxStart);

	 if (!dserr) {
	                 if (group[0])
        		      eval=is_member_of_group_nds(errstr,ctx,conn,oid,group);

	}
bailout:
	NWCCCloseConn(conn);
	dserr = NWDSFreeContext(ctx);
        if (dserr) {
		sprintf(errstr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		eval=111;
	}
	closelog();
	return eval;
}


static int nds_read_string_fn(char* errstr, const char* treename, const char* objectname, const char* attrname, char** buffer) {
/* return single or multi string attributes */
	NWDSCCODE dserr;
	NWDSContextHandle ctx=NULL;
	NWCONN_HANDLE conn=NULL;
	int eval = 0;

	if (!treename || !objectname || !attrname ) {
	   sprintf (errstr," invalid parameters.");
	   return 98;
	}
	eval=_createCtxAndConn (errstr,treename, &ctx, &conn);
	if (eval)
	        return eval;

	dserr= NWCXGetMultiStringAttributeValue (ctx,objectname,attrname,buffer);
	if (dserr) {
		sprintf(errstr, "NWCXGetAttribute failed : %s\n",strnwerror(dserr));
		eval=106;
	}
	NWCCCloseConn(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		sprintf(errstr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		eval=107;
	}
	return eval;
}


static int nds_read_string_fn2(char* errstr, const char* servername, const char* objectname, const char* attrname, char** buffer) {
/* return single or multi string attributes using a servername instead of a treename*/

	NWDSCCODE dserr;
	NWDSContextHandle ctx=NULL;
	NWCONN_HANDLE conn=NULL;
	int eval = 0;

	if (!servername || !objectname || !attrname ) {
	   sprintf (errstr," invalid parameters.");
	   return 98;
	}

	eval=_createCtx (errstr,&ctx);
	if (eval)
	        return eval;

	if (servername[0] == '/') {
		dserr = ncp_open_mount(servername, &conn);
		if (dserr) {
			sprintf(errstr, "ncp_open_mount failed with %s\n", strnwerror(dserr));
			eval=104;
			goto bailout;
		}
	} else {
		dserr = NWCCOpenConnByName(NULL,servername, NWCC_NAME_FORMAT_BIND, 0, 0, &conn);
		if (dserr) {
			sprintf(errstr, "ncp_open failed with %s\n",strnwerror(dserr));
			eval=105;
			goto bailout;
		}
	}
	dserr = NWDSAddConnection(ctx, conn);
	if (dserr) {
		sprintf(errstr, "Cannot bind connection to context: %s\n",strnwerror(dserr));
		eval=106;
		goto bailout;
	}


	dserr= NWCXGetMultiStringAttributeValue (ctx,objectname,attrname,buffer);
	if (dserr) {
		sprintf(errstr, "NWCXGetAttribute failed : %s\n",strnwerror(dserr));
		eval=106;
	}
bailout:
	if (conn) NWCCCloseConn(conn);
	if (ctx) {
		dserr = NWDSFreeContext(ctx);
		if (dserr) {
			sprintf(errstr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
			eval=107;
		}
	}
	return eval;
}


static int nds_read_int_fn(char* errstr, const char* treename, const char* objectname, const char* attrname, int* result) {
/* return single numeric NDS (include time and boolean attributes)*/

	NWDSCCODE dserr;
	NWDSContextHandle ctx=NULL;
	NWCONN_HANDLE conn=NULL;
	int eval = 0;

	if (!treename || !objectname || !attrname ) {
                        sprintf (errstr," invalid parameters.");
                        return 98;
	}

        eval=_createCtxAndConn (errstr,treename, &ctx, &conn);
	if (eval)
	        return eval;

	 dserr=NWCXGetIntAttributeValue (ctx,objectname,attrname,result);
	 if (dserr) {
                        sprintf(errstr, "NWCXGetAttribute failed : %s\n",strnwerror(dserr));
                        eval=106;
	}
	NWCCCloseConn(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
                        sprintf(errstr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
                        eval=107;
	}
	return eval;
}

static int nds_read_int_fn2(char* errstr, const char* servername, const char* objectname, const char* attrname,int* result)
{
/* return single numeric NDS (include time and boolean attributes)*/
	NWDSCCODE dserr;
	NWDSContextHandle ctx=NULL;
	NWCONN_HANDLE conn=NULL;
	int eval = 0;

	if (!servername || !objectname || !attrname ) {
	   sprintf (errstr," invalid parameters.");
	   return 98;
	}

	eval=_createCtx (errstr,&ctx);
	if (eval)
	        return eval;

	if (servername[0] == '/') {
		dserr = ncp_open_mount(servername, &conn);
		if (dserr) {
			sprintf(errstr, "ncp_open_mount failed with %s\n", strnwerror(dserr));
			eval=104;
			goto bailout;
		}
	} else {
		dserr = NWCCOpenConnByName(NULL,servername, NWCC_NAME_FORMAT_BIND, 0, 0, &conn);
		if (dserr) {
			sprintf(errstr, "ncp_open failed with %s\n",strnwerror(dserr));
			eval=105;
			goto bailout;
		}
	}
	dserr = NWDSAddConnection(ctx, conn);
	if (dserr) {
		sprintf(errstr, "Cannot bind connection to context: %s\n",strnwerror(dserr));
		eval=106;
		goto bailout;
	}

	 dserr=NWCXGetIntAttributeValue(ctx, objectname, attrname, result);
	 if (dserr) {
                        sprintf(errstr, "NWCXGetAttribute failed : %s\n",strnwerror(dserr));
                        eval=106;
	}

bailout:
	if (conn) NWCCCloseConn(conn);
	if (ctx) {
		dserr = NWDSFreeContext(ctx);
		if (dserr) {
			sprintf(errstr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
			eval=107;
		}
	}
	return eval;

}


PHP_FUNCTION (read_nds_string)
{
	int res;             /* fcn result code */
	const char *treen, *objectn, *attributen;
	char errstr[512];
	char local_buffer [8192];
	char * buffer; // returned by  NWCXGetMultiStringAttributeValue

	int argc = ZEND_NUM_ARGS();
	zval **tree, **object, **attribute;

	if (argc != 3 || zend_get_parameters_ex(argc, &tree, &object, &attribute)) {
		WRONG_PARAM_COUNT;
	}

	sprintf(errstr, "Wrong parameters values");
	if (!tree || !object || !attribute) {
	   RETURN_STRING(errstr,1);
	}

	convert_to_string_ex(tree);
	convert_to_string_ex(object);
	convert_to_string_ex(attribute);

	treen = (*tree)->value.str.val;
	objectn = (*object)->value.str.val;
	attributen = (*attribute)->value.str.val;

	if (!treen || !objectn || !attributen) {
	   RETURN_STRING(errstr,1);
	}

	sprintf(errstr, "failure");
	res=nds_read_string_fn(errstr,treen,objectn,attributen,&buffer);
	if (res) {
		RETURN_STRING(errstr,1);
	}
	if (strlen(buffer) >=sizeof(local_buffer)) {
		buffer[sizeof(local_buffer)-1]=0;

	}
	strcpy(local_buffer,buffer);
	free(buffer);
	RETURN_STRING(local_buffer,1);

}

PHP_FUNCTION (read_nds_string2)
//use a servername instaed of a tree name
{
	int res;             /* fcn result code */
	const char *servern, *objectn, *attributen;
	char errstr[512];
	char local_buffer [8192];
	char * buffer; // returned by  NWCXGetMultiStringAttributeValue

	int argc = ZEND_NUM_ARGS();
	zval **server, **object, **attribute;

	if (argc != 3 || zend_get_parameters_ex(argc, &server, &object, &attribute)) {
		WRONG_PARAM_COUNT;
	}

	sprintf(errstr, "Wrong parameters values");
	if (!server || !object || !attribute) {
	   RETURN_STRING(errstr,1);
	}

	convert_to_string_ex(server);
	convert_to_string_ex(object);
	convert_to_string_ex(attribute);

	servern = (*server)->value.str.val;
	objectn = (*object)->value.str.val;
	attributen = (*attribute)->value.str.val;

	if (!servern || !objectn || !attributen) {
	   RETURN_STRING(errstr,1);
	}

	sprintf(errstr, "failure");
	res=nds_read_string_fn2(errstr,servern,objectn,attributen,&buffer);
	if (res) {
		RETURN_STRING(errstr,1);
	}
	if (strlen(buffer) >=sizeof(local_buffer)) {
		buffer[sizeof(local_buffer)-1]=0;

	}
	strcpy(local_buffer,buffer);
	free(buffer);
	RETURN_STRING(local_buffer,1);

}

PHP_FUNCTION (read_nds_int)
{
        int res;             /* fcn result code */
        const char *treen, *objectn, *attributen;
        char errstr[512];

        int argc = ZEND_NUM_ARGS();
        zval **tree, **object, **attribute;

        if (argc != 3 || zend_get_parameters_ex(argc, &tree, &object, &attribute)) {
                WRONG_PARAM_COUNT;
        }

        sprintf(errstr, "Wrong parameters values");
        if (!tree || !object || !attribute) {
           RETURN_STRING(errstr,1);
        }

        convert_to_string_ex(tree);
        convert_to_string_ex(object);
        convert_to_string_ex(attribute);

        treen = (*tree)->value.str.val;
        objectn = (*object)->value.str.val;
        attributen = (*attribute)->value.str.val;

        if (!treen || !objectn || !attributen) {
           RETURN_STRING(errstr,1);
        }
        sprintf(errstr, "failure");
	{
		int result;
		res = nds_read_int_fn(errstr, treen, objectn, attributen, &result);
		if (!res) {
			sprintf(errstr, "%d", result);
		}
	}
        RETURN_STRING(errstr, 1);
}


PHP_FUNCTION (read_nds_int2)
// use a servername instead of a tree name as first argument
{
        int res;             /* fcn result code */
        const char *servern, *objectn, *attributen;
        char errstr[512];

        int argc = ZEND_NUM_ARGS();
        zval **server, **object, **attribute;

        if (argc != 3 || zend_get_parameters_ex(argc, &server, &object, &attribute)) {
                WRONG_PARAM_COUNT;
        }

        sprintf(errstr, "Wrong parameters values");
        if (!server || !object || !attribute) {
           RETURN_STRING(errstr,1);
        }

        convert_to_string_ex(server);
        convert_to_string_ex(object);
        convert_to_string_ex(attribute);

        servern = (*server)->value.str.val;
        objectn = (*object)->value.str.val;
        attributen = (*attribute)->value.str.val;

        if (!servern || !objectn || !attributen) {
           RETURN_STRING(errstr,1);
        }
        sprintf(errstr, "failure");
	{
		int result;

		res = nds_read_int_fn2(errstr, servern, objectn, attributen, &result);
		if (!res) {
			sprintf(errstr, "%d", result);
		}
	}
        RETURN_STRING(errstr, 1);
}


PHP_FUNCTION(auth_bindery)
{
	int res;             /* fcn result code */
	const char *servern, *usern, *sent_pw, *groupn;
	char errstr[512];

	int argc = ZEND_NUM_ARGS();
	zval **server, **user, **password, ** group;

	if (argc != 4 || zend_get_parameters_ex(argc, &server, &user, &password, &group)) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(server);
	convert_to_string_ex(user);
	convert_to_string_ex(password);
	convert_to_string_ex(group);

	servern = (*server)->value.str.val;
	usern = (*user)->value.str.val;
	sent_pw = (*password)->value.str.val;
	groupn = (*group)->value.str.val;

	/* do they know the magic word? */
	sprintf(errstr, "failure");
	res = bindery_auth(errstr, servern, usern, sent_pw, groupn );
	if (res) {
		RETURN_STRING(errstr,1);
	}
	RETURN_FALSE;
}

PHP_FUNCTION(auth_tree)
{
	int res;             /* fcn result code */
	const char *servern, *ctxs,*usern, *sent_pw, *groupn;
	char errstr[512];
	char fqdn[MAX_DN_CHARS+5]; // to get 'DN=' plus the FQDN used
	int argc = ZEND_NUM_ARGS();
	zval **server, **user, **contexts, **password, **group;

	if (argc != 5 || zend_get_parameters_ex(argc, &server, &user, &contexts,&password, &group)) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(server);
	convert_to_string_ex(user);
	convert_to_string_ex(contexts);
	convert_to_string_ex(password);
	convert_to_string_ex(group);

	servern = (*server)->value.str.val;
	usern = (*user)->value.str.val;
	ctxs = (*contexts)->value.str.val ;
	sent_pw = (*password)->value.str.val;
	groupn = (*group)->value.str.val;

	/* do they know the magic word? */
	sprintf(errstr, "failure");
	strcpy(fqdn, "DN=");
	res = tree_auth_fn(errstr, servern, usern, ctxs,sent_pw, groupn, fqdn + 3);
	if (res) {
		RETURN_STRING(errstr, 1);
	}
	RETURN_STRING(fqdn, 1);
}

PHP_FUNCTION(auth_nds)
{
	int res;             /* fcn result code */
	const char *servern, *usern, *sent_pw, *groupn;
	char errstr[512];

	int argc = ZEND_NUM_ARGS();
	zval **server, **user, **password, **group;

	if (argc != 4 || zend_get_parameters_ex(argc, &server, &user, &password, &group)) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(server);
	convert_to_string_ex(user);
	convert_to_string_ex(password);
	convert_to_string_ex(group);

	servern = (*server)->value.str.val;
	usern = (*user)->value.str.val;
	sent_pw = (*password)->value.str.val;
	groupn = (*group)->value.str.val;

	/* do they know the magic word? */
	sprintf(errstr, "failure");
	res = nds_auth_fn(errstr, servern, usern, sent_pw, groupn);
	if (res) {
		RETURN_STRING(errstr,1);
	}
	RETURN_FALSE;
}


PHP_MINIT_FUNCTION(auth_nds)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(auth_nds)
{
	php_info_print_table_start();
	php_info_print_table_row(3, "NDS/Bindery authentication support", "enabled", "(c) 2000-2001 by <a HREF=\"mailto:vandrove@vc.cvut.cz\">P.Vandrovec</a> and <A href=\"mailto:patrick.pollet@insa-lyon.fr\">P.Pollet</a>");
	php_info_print_table_row(3, "NDS reading properties support", "enabled", "(c) 2001 by <A HREF=\"mailto:patrick.pollet@insa-lyon.fr\">P.Pollet</a>");
	php_info_print_table_end();
}

function_entry auth_nds_functions[] = {
	PHP_FE(auth_bindery,			NULL)
	PHP_FE(auth_nds,			NULL)
	PHP_FE(auth_tree,			NULL)
	PHP_FE(read_nds_string,			NULL)
	PHP_FE(read_nds_int,			NULL)
	PHP_FE(read_nds_string2,		NULL)
	PHP_FE(read_nds_int2,			NULL)
	{NULL, NULL, NULL}
};

zend_module_entry auth_nds_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
        "auth_nds",
	auth_nds_functions,
	PHP_MINIT(auth_nds),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(auth_nds),
#if ZEND_MODULE_API_NO >= 20010901
	NCPFS_VERSION,         /* extension version number (string) */
#endif
	STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(auth_nds)

