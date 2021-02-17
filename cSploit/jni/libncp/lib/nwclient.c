/*
    nwclient.c - Client oriented calls
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


	1.00  2001, Feb 18		Patrick Pollet
		Initial release.

        1.01  2001, March 9		Patrick Pollet
                Removed NWDSScanConnsForTrees.Is now in nwnet.c
		Corrected NWCXGetPermConnInfo(NWCC_INFO_AUTHENT_STATE).
                Corrected NWCXSplitNameAndContext (had no return values)

	1.02  2001, March 12		Patrick Pollet
                #131:Corrected NWCXIsDSAuthenticated ( return 0 if not dxh) and not -1
                #137:__NWCXGetPermConnList made static ( now internal only ?)
                #341: added tests for null pointers in NWCXAttachToTreeByName,NWCXGetDefault*
                #389: better error code NWE_BIND_NO_SUCH_PROP
                NWCXGetNDSVolumeServerAndResourceName now allows null pointers for serverName/resourceName

	1.03  2001, March 16		Patrick Pollet
		#284 (NWDSCreateContextHandleMnt): added missing NWDSFreeContext
		Rewrote the attribute reading part
		Added NWCXGetIntAttributeValues, NWCXGetStringAttributeValues,
                	NWCXGetMultiStringAttributeValues, NWCXGetAttributeValuesAsString,
                Added reading the ~/.nwinfos file to fetch default tree, ctx, user if none in environnment
                Added NWCXGetDefaultServer call to NWCXAttachToTreeByName()

	1.04  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add include strings.h, use ncpt_atomic_*, not atomic_*,
		use size_t for size...

	1.05  2001, July 15		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed NWCXSplitNameAndContext to honor ctx's settings.

	1.06  2001, October 21		Patrick.Pollet@insa-lyon.fr
		Fixed NWCXSplitNameAndContext to allow NULL name or context .
		Mofified NWCXAttachToTreeByName() to use the new NWCCOpenConnByname((,,NWCC_NAME_FORMAT_NDS_TREE,...)

	1.07  2001, December 7		Patrick.Pollet@insa-lyon.fr
		Reverted NWCXAttachToTreeByName() to use the old code since (see #define USE_OLD_CODE)
		new NWCCOpenConnByname((,,NWCC_NAME_FORMAT_NDS_TREE,...) is sometimes capricious

	1.08  2002, January 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Moved NWCC_INFO_MOUNT_POINT code from here to nwcalls.c.
		Compile portions need by ncpumount even when NDS_SUPPORT is disabled.

	1.09  2002, January 25		Patrick.Pollet@insa-lyon.fr
		Implemented LoginScript reading API calls
                          Removed NWDSAbbreviateName in  __docopy_string when retrieving a SYN_DIST_NAME 


*/

#include "config.h"


#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <ncp/nwclient.h>
#include "nwnet_i.h"

#ifdef NCP_KERNEL_NCPFS_AVAILABLE
#include <mntent.h>
#endif
#include <pwd.h>
#include <sys/stat.h>

/*should be in ndslib.h ?*/
#define NWE_BIND_NO_SUCH_PROP NWE_NCP_NOT_SUPPORTED

#undef DEBUG_PRINT

#if 1
/*read hard degugging*/
void printConn (NWCONN_HANDLE conn, int stage);
void printConn (NWCONN_HANDLE conn, int stage) {
        printf("%s\n",conn->mount_point);
        printf ("------ info from kernel--------------------------------\n");
        printf ("infoV2.version %d\n",conn->i.version);
	printf ("infoV2.mounted_uid %lu\n",conn->i.mounted_uid);
	printf ("infoV2.connection %d\n",conn->i.connection);
	printf ("infoV2.buffer_size %d\n",conn->i.buffer_size);
        printf ("infoV2.volume_number %d\n",conn->i.volume_number);
	printf ("infoV2.directory_id %d\n",conn->i.directory_id);

	printf ("infoV2.dummy1 %d\n",conn->i.dummy1);
	printf ("infoV2.dummy2 %d\n",conn->i.dummy2);
	printf ("infoV2.dummy3 %d\n",conn->i.dummy3);

        printf ("------ info from ncplib (recreated stage %d) ---------\n",stage);

        printf ("is_connected %d\n",conn->is_connected);
        printf ("nds_conn %p\n",conn->nds_conn);
	printf ("nds_ring %p/%p\n",conn->nds_ring.prev, conn->nds_ring.next);
        printf ("user %s\n",conn->user);
	printf ("user_id_is_valid %d\n",conn->user_id_valid);
	printf ("user_id %x\n",conn->user_id);

	printf ("mount_fid %d\n",conn->mount_fid);
        printf ("mount_point %s\n",conn->mount_point);
	printf ("ncpt_atomic_t store_count %d\n",ncpt_atomic_read(&conn->store_count));
        printf ("state %d\n",conn->state);

        printf ("connstate %x\n", conn->connState);
        printf ("sign_wanted %x\n", ncp_get_sign_wanted(conn));
	printf ("sign_active %x\n", ncp_get_sign_active(conn));

        printf ("serverInfo.valid %x\n",conn->serverInfo.valid);
        printf ("serverInfo.serverName %s\n",conn->serverInfo.serverName);
        printf ("serverInfo.version.major %x\n",conn->serverInfo.version.major);
        printf ("serverInfo.version.minor %x\n",conn->serverInfo.version.minor);
        printf ("serverInfo.version.revision %x\n",conn->serverInfo.version.revision);

        printf ("NET_ADDRESS_TYPE nt %x\n",conn->nt);
}
#endif

static int NWCIsPermanentConnection (NWCONN_HANDLE conn) {
	return (ncp_get_conn_type(conn) == NCP_CONN_PERMANENT);
}

NWCCODE NWCXGetPermConnInfo(NWCONN_HANDLE conn, nuint info, size_t len, void* buffer) {
        if (!buffer) /*just in case*/
		return ERR_NULL_POINTER;

	if (!NWCIsPermanentConnection(conn))
		return NWE_REQUESTER_FAILURE;

        switch (info) {
                case  NWCC_INFO_AUTHENT_STATE:
                        { /* must be authenticated since it is permanently mounted ? */
#ifdef NDS_SUPPORT			
                            if (NWIsDSServer(conn,NULL))
                                return ncp_put_req_size_unsigned(buffer, len, NWCC_AUTHENT_STATE_NDS);
                            else
#endif
                                return ncp_put_req_size_unsigned(buffer, len, NWCC_AUTHENT_STATE_BIND);
                        }

                default:
			return NWCCGetConnInfo(conn,info,len,buffer);
	}
}

#ifdef NDS_SUPPORT
int NWCXIsDSServer ( NWCONN_HANDLE conn, NWDSChar * treeName) {
/* same as API return 1 if OK but removes the trailings '_____'*/
   if (!treeName)
        return NWIsDSServer(conn,NULL);

   if (NWIsDSServer(conn,treeName)) {
      char *p;

       p = strchr(treeName, '\0') - 1;
       while ((p >= treeName) && (*p == '_'))
           p--;
       treeName[ p - treeName + 1] = 0;
      return 1;
   }
   else {
        treeName[0]=0;
        return 0;
  }
}

int NWCXIsSameTree (NWCONN_HANDLE conn, const NWDSChar *treeName) {
/* return 1 if OK  */
 char treeName2[MAX_TREE_NAME_CHARS+1]="";

 if (NWCXIsDSServer(conn,treeName2))  {
   if (!treeName)
      return 0;
   return  (!strcasecmp(treeName,treeName2));
 }
 else return 0;
}
#endif

int NWCXIsSameServer(NWCONN_HANDLE conn, const NWDSChar* server) {
	/* return 1 if OK  */
	char server2[NCP_BINDERY_NAME_LEN + 1];
	NWCCODE err;

	if (!server) {
		return 0;
	}
	err = NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME, sizeof(server2), server2);
	if (err) {
		return 0;
	}
	return !strcasecmp(server,server2);
}

#ifdef NDS_SUPPORT
NWDSCCODE NWCXIsDSAuthenticated(NWDSContextHandle ctx){
/* return non zero if a context has authentication keys*/
   NWDS_HANDLE dxh;

   dxh=ctx->ds_connection;
   if (! dxh)  /* should not be ???*/
      return 0;
   return
    (dxh->authinfo!=NULL);
}
#endif

static
NWCCODE __NWCXGetPermConnList (NWCONN_HANDLE* conns , int maxEntries, int* curEntries,
                               uid_t uid,const NWDSChar* treeName, const NWDSChar* serverName){
/* returns the list of permanent connexions belonging to user uid*/
/* if uid=-1 = all Permanent connections (Root only) */
/* if treeName !=NUll only them */
/*if serverName !=NULL only them*/

#ifdef NCP_KERNEL_NCPFS_AVAILABLE
        FILE* mtab;
        NWCONN_HANDLE conn;
        struct mntent* mnt_ent;

#if 0
        char aux[256];
/*        char aux2[256];*/
        char* user;
#endif
	
        uid_t callerUid=getuid();
	int alluid = 0;

        *curEntries=0;

        if ((uid == (uid_t)-1)) {
	        /*only root can scan all connections*/
		if (callerUid != 0) {
	               return EACCES;
		}
		alluid = 1;
	} else if (uid != callerUid) {
	        /*only root can scan other user connections*/
        	if (callerUid != 0) {
			return EACCES;
		}
	}

#ifdef NDS_SUPPORT
        if (treeName && serverName) {	/*should never happen on this internal function ? */
               return NWE_REQUESTER_FAILURE;
	}
#else
	if (treeName) {
		return ENOPKG;
	}
#endif

        if ((mtab = fopen(MOUNTED, "r")) == NULL) {
                return errno;
        }
        while ((*curEntries <maxEntries)&&(mnt_ent = getmntent(mtab))) {
/*                printf("%s %s %s\n ",mnt_ent->mnt_type,mnt_ent->mnt_fsname,mnt_ent->mnt_dir);*/
                if (strcmp(mnt_ent->mnt_type, "ncpfs") != 0) {
                        continue;
                }
#if 0		
                /*printf("%s %s %s\n ",mnt_ent->mnt_type,mnt_ent->mnt_fsname,mnt_ent->mnt_dir);*/
                if (strlen(mnt_ent->mnt_fsname) >= sizeof(aux)) {
                        continue;
                }
                strcpy(aux, mnt_ent->mnt_fsname);
                user = strchr(aux, '/');
                if (!user) {
                        continue;
                }
                *user++ = '\0';

/*                strcpy(aux2, mnt_ent->mnt_dir);
                printf("-->%s %s %s\n ",aux,user,aux2);*/
#endif
                if (ncp_open_mount(mnt_ent->mnt_dir, &conn)) {
/*                   printf("failed%s\n","");*/
                       continue;
                }

                if (alluid || (conn->i.mounted_uid == uid)) {
                        int addit;
#ifdef NDS_SUPPORT			
                        if (treeName)
                                addit = NWCXIsSameTree(conn, treeName);
                        else 
#endif			
			if (serverName)
                                addit = NWCXIsSameServer(conn, serverName);
			else
				addit = 1;
                        if (addit) {
                                *conns++ = conn;
                                (*curEntries)++;
/*                               printf("OK %d\n",*curEntries);*/
                        } else {
				NWCCCloseConn(conn); /*free memory only */
			}
                } else {
			NWCCCloseConn(conn); /*free memory only */
		}
        }
        fclose(mtab);
        return 0;
#else
        return ENOPKG;
#endif  /* NCP_KERNEL_NCPFS_AVAILABLE */
}


NWCCODE NWCXGetPermConnList (NWCONN_HANDLE* conns , int maxEntries, int* curEntries, uid_t uid){
/* returns the list of permanent connexions belonging to user uid*/
/* if uid=-1 = all Permanent connections (Root only) */
    return __NWCXGetPermConnList (conns,maxEntries,curEntries,uid,NULL,NULL);

}

NWCCODE NWCXGetPermConnListByTreeName (NWCONN_HANDLE* conns, int maxEntries,
                                      int* curEntries, uid_t uid,
                                      const NWDSChar *treeName){
   return __NWCXGetPermConnList (conns,maxEntries,curEntries,uid,treeName,NULL);
}

NWCCODE NWCXGetPermConnListByServerName (NWCONN_HANDLE* conns , int maxEntries,
                                      int* curEntries, uid_t uid,
                                      const NWDSChar *serverName){
   return __NWCXGetPermConnList (conns,maxEntries,curEntries,uid,NULL,serverName);
}

#ifdef NDS_SUPPORT
NWDSCCODE NWDSSetContextHandleTree(NWDSContextHandle ctx, const NWDSChar * treeName)
{
#define MAXCONNS 64
	NWDSCCODE err;
	NWCONN_HANDLE conns[MAXCONNS];
	int curEntries;
	int i;
	wchar_t treeNameW[MAX_DN_CHARS+1];
	char treeNameUTF[MAX_DN_CHARS*4 + 1];

	if (!treeName)
		return ERR_NULL_POINTER;

	err = NWDSXlateFromCtx(ctx, treeNameW, sizeof(treeNameW), treeName);
	if (err)
		return err;
	err = iconv_wchar_t_to_external(treeNameW, treeNameUTF, sizeof(treeNameUTF));
	if (err)
		return err;
	err = NWDSSetTreeNameW(ctx, treeNameW);
	if (err) {
		return err;
	}
	err = NWCXGetPermConnListByTreeName(conns, MAXCONNS, &curEntries, getuid(), treeNameUTF);
	if (err) {
		return err;
	}
	for (i = 0; i < curEntries; i++) {
		NWCONN_HANDLE conn = conns[i];
		err = NWDSAddConnection(ctx, conn);
		if (err) {
			NWCCCloseConn(conn);
			continue;
		}
	}
	return 0;
}

NWDSCCODE NWDSCreateContextHandleMnt(NWDSContextHandle* pctx, const NWDSChar * treeName)
{
	NWDSCCODE err;
	NWDSContextHandle ctx;

	if (!pctx) {
		return ERR_NULL_POINTER;
	}
	err = NWDSCreateContextHandle(&ctx);
	if (err)
		return err;
	err = NWDSSetContextHandleTree(ctx, treeName);
	if (err)
		NWDSFreeContext(ctx);
	else
		*pctx = ctx;
	return err;
}

//#define NOENV 1  <-- testing reading of .nwinfos file
#undef NOENV
#define NWINFOS ".nwinfos"

/**********************************************/
#define NDS_USER                "NDS_USER"
#define NDS_GECOS               "NDS_GECOS"
#define NDS_SHELL               "NDS_SHELL"
#define NDS_HOME                "NDS_HOME"
#define NDS_UID                 "NDS_UID"
#define NDS_GID                 "NDS_GID"
#define NDS_HOME_SERVER         "NDS_HOME_SERVER"
#define NDS_HOME_VOLUME         "NDS_HOME_VOLUME"
#define NDS_HOME_PATH           "NDS_HOME_PATH"
#define NDS_HOME_MNT_PNT        "NDS_HOME_MNT_PNT"
#define NDS_EMAIL               "NDS_EMAIL"

#define NDS_PREFERRED_SERVER    "NDS_PREFERRED_SERVER"
#define NDS_PREFERRED_TREE      "NDS_PREFERRED_TREE"
#define NDS_PREFERRED_NAME_CTX  "NDS_PREFERRED_NAME_CTX"

/***********************************************/

static char* readnwinfosfile (char * user, const char * info, const char * forTree, long * err) {
/*  strongly inspired from ncplib.c: ncp_fopen_nwc and ncp_get_nwc_ent
    user (=NULL) current user
    info : variable name we are looking for
    if forTree= NULL any tree will do
    else forTree must be equals to NWCLIENT_PREFERRED_TREE=xxxx
    stored in ~/.nwinfos file */

        FILE *f;
        static char value[1024]=""; /*static value of matching line*/
        char concernTree[1024]=""; /*tree read from ~/.nwinfos file */
        char line[1024]="";
        char path[1024]="";
        char * home=NULL;
        char *p;
        int line_len;
        struct stat st;

        if (!user) {
                home = getenv("HOME");
        } else {
                struct passwd *pwd;

                if ((pwd = getpwnam(user)) != NULL) {
                        home = pwd->pw_dir;
                }
        }

        if ((home == NULL)
            || (strlen(home) + sizeof(NWINFOS) + 2 > sizeof(path))) {
                *err = ENAMETOOLONG;
                return NULL;
        }
        strcpy(path, home);
        strcat(path, "/");
        strcat(path, NWINFOS);
#ifdef NOENV
   printf ("searching %s\n",path);
#endif

        if (stat(path, &st) != 0) {
                *err = errno;
                return NULL;
        }
        if (st.st_uid != getuid()) {
                *err = EACCES;
                return NULL;
        }
        if ((st.st_mode & (S_IRWXO | S_IRWXG)) != 0) {
                *err = NCPLIB_INVALID_MODE;
                return NULL;
        }
        f=fopen(path, "r");

        if (!f) {
                *err=errno;
                return NULL;
        }
        value[0]=0;      /*reset statics */
        concernTree[0]=0;
        while (fgets(line, sizeof(line), f) != NULL) {
                if ((line[0] == '\n')|| (line[0] == '#'))
                        continue;
                line_len = strlen(line);
                if (line[line_len - 1] == '\n') {
                        line[line_len - 1] = '\0';

                }

                p=strchr(line,'=');
                if (!p)
                        continue;
                *p=0;
                if (!strcasecmp(line,info)) {
                        strcpy(value,p+1);
#ifdef NOENV
  printf ("found required info %s=%s\n",info,value);
#endif
                }
                if (!strcasecmp(line,NDS_PREFERRED_TREE)) {
                        strcpy(concernTree,p+1);
#ifdef NOENV
  printf ("this file concerns tree %s\n",concernTree);
#endif

                }

                if (value[0] && concernTree[0])
                        break; /* got both */

        }
        fclose(f);
#ifdef NOENV
  printf ("found for tree %s :pref_tree = %s required info %s=%s\n",
           forTree,concernTree,info,value);
#endif

        if (!value[0]) return NULL; /* not found */
        if (!forTree) return value; /* Ok for any tree */
        return strcasecmp(forTree,concernTree)? NULL:value;
}


/* we are using the same env names as the Caldera client (hope they won't mind)
#define PREFER_TREE_ENV         "NWCLIENT_PREFERRED_TREE"
#define PREFER_CTX_ENV          "NWCLIENT_DEFAULT_NAME_CONTEXT"
#define PREFER_SRV_ENV          "NWCLIENT_PREFERRED_SERVER"
#define PREFER_USER_ENV         "NWCLIENT_DEFAULT_USER"

this one is an extra by PP for a possible autologon
#define PREFER_PWD_ENV          "NWCLIENT_DEFAULT_PASSWORD"
*/


NWDSCCODE NWCXGetPreferredDSTree    (NWDSChar * preferTree, size_t maxLen){

  char * res=NULL;
  long err;

  if (!preferTree)
      return ERR_NULL_POINTER;
#ifndef NOENV
  res =getenv (PREFER_TREE_ENV);
#endif
  if (!res)
        res=readnwinfosfile (NULL,NDS_PREFERRED_TREE,NULL, &err);
  if (!res)
    return -1;
  if (strlen (res)+1 >maxLen)
     return NWE_BUFFER_OVERFLOW;
  strcpy(preferTree,res);
  return 0;
}

NWDSCCODE NWCXGetDefaultNameContext (const NWDSChar* forTree,
                                     NWDSChar * nameContext, size_t maxLen){

   char * res=NULL;
   long err;

  if (!nameContext)
      return ERR_NULL_POINTER;
#ifndef NOENV
  res =getenv (PREFER_CTX_ENV);
#endif
   if (!res)
        res=readnwinfosfile (NULL,NDS_PREFERRED_NAME_CTX,forTree,&err);
  if (!res)
    return -1;
  if (strlen (res)+1 >maxLen)
     return NWE_BUFFER_OVERFLOW;
  strcpy(nameContext,res);
  return 0;

}

NWDSCCODE NWCXGetPreferredServer    (const NWDSChar * forTree,
                                     NWDSChar *preferedServer, size_t maxLen){
	char * res=NULL;

	if (!preferedServer)
		return ERR_NULL_POINTER;
#ifndef NOENV
	res = getenv(PREFER_SRV_ENV);
#endif
	if (!res) {
		long err;

		res=readnwinfosfile (NULL, NDS_PREFERRED_SERVER, forTree, &err);
		if (!res) {
			return -1;
		}
	}
	/* test that this server DO belongs to tree forTree*/
	/* NULL means any tree or even bindery (?)*/
	if (forTree) {
		NWCONN_HANDLE conn;
		NWCCODE err;

		err = NWCCOpenConnByName(NULL, res, NWCC_NAME_FORMAT_BIND, 0, NWCC_RESERVED, &conn);
		if (err) {
			return -1;	/* better code for not available ? */
		}
		err = !NWCXIsSameTree(conn, forTree);
		NWCCCloseConn(conn);
		if (err) {
			return -1;	/* not same tree ignore it */
		}
	}

	if (strlen(res) + 1 > maxLen) {
		return NWE_BUFFER_OVERFLOW;
	}
	strcpy(preferedServer, res);
	return 0;
}


NWDSCCODE NWCXGetDefaultUserName (const NWDSChar * forTree,
                                   NWDSChar *defaultName, size_t maxLen){
/*may be we should return current user's Unix name if nothing found in env ? */
        char * res=NULL;
        long err;
        if (!defaultName)
                return ERR_NULL_POINTER;
#ifndef NOENV
        res =getenv (PREFER_USER_ENV);
#endif
         if (!res)
                res=readnwinfosfile (NULL,NDS_USER,forTree, &err);
         if (!res)
                return -1;
        if (strlen (res)+1 >maxLen)
                return NWE_BUFFER_OVERFLOW;
        strcpy(defaultName,res);
        return 0;
}

NWDSCCODE NWCXGetDefaultPassword (const NWDSChar * forTree,
                                   NWDSChar *defaultPWD, size_t maxLen){
        char * res=NULL;
        if (!defaultPWD)
                return ERR_NULL_POINTER;
        (void) forTree; /* not yet used */
        res =getenv (PREFER_PWD_ENV);
        if (!res)
                return -1;
        if (strlen (res)+1 >maxLen)
                return NWE_BUFFER_OVERFLOW;
        strcpy(defaultPWD,res);
        return 0;
}


#define USE_OLD_CODE

NWDSCCODE NWCXAttachToTreeByName( NWCONN_HANDLE* conn, const NWDSChar * treeName){

#ifdef USE_OLD_CODE
        struct ncp_conn *connInit;
	struct ncp_bindery_object obj;
        long err;
        char prefered_server[MAX_DN_BYTES+1];

        if (!conn || !treeName)
                return ERR_NULL_POINTER;

        /* NULL  nearest server ????*/
	if ((connInit = ncp_open(NULL, &err)) == NULL){
                  return err;
        }
#if 0
  printf ("searching for default server in tree %s\n",treeName);
#endif
        /* see if there is a default server in env */
        err=NWCXGetPreferredServer(treeName,prefered_server,sizeof(prefered_server));
        if (!err) {
#if 0
  printf ("found default server %s\n",prefered_server);
#endif

           err=NWCCOpenConnByName(connInit,prefered_server,NWCC_NAME_FORMAT_BIND,
                               0, NWCC_RESERVED,conn);
           /*we already checked in NWCXGetPreferredServer that server belongs to the target tree */
           if (!err) {
#if 0
  printf ("OK on server %s\n",prefered_server);
  printConn (*conn,1);
#endif

                NWCCCloseConn(connInit);
                return 0;
           }
        }

/* try to speed up search of all servers but first checking that treeName exists */
        {
          nuint32 scanIndex=-1;
          char myTreeName [MAX_DN_CHARS+1];
          NWDSContextHandle ctx;
          int found=0;

          err= NWDSCreateContextHandle (&ctx);
          if (err)
            return err;

          while (!found && (err=NWDSScanForAvailableTrees(ctx,connInit,"*",&scanIndex,myTreeName)==0)){
                /*printf (" comparing %s and %s \n",treeName,myTreeName);*/
             found=!strcasecmp(treeName,myTreeName);
          }
          NWDSFreeContext(ctx);
          /*printf ("found=%d\n",err);*/
          if (!found) /* 0x89FC*/
            {
               NWCCCloseConn(connInit);
               return NWE_BIND_NO_SUCH_PROP;
            }
        }

	obj.object_id = 0xffffffff;

        err=NWE_BIND_NO_SUCH_PROP;
        while (ncp_scan_bindery_object(connInit, obj.object_id,
				       NCP_BINDERY_FSERVER, "*",
				       &obj) == 0){
                // do not ask AXIS CD ROM TOWERS they are bugged in opening conns !!!!
                 /*printf ("trying server %s\n",obj.object_name);*/
                 if (!strncmp ("AXIS",obj.object_name,4)) {
                        continue;
                 }
                 err=NWCCOpenConnByName(connInit,obj.object_name,NWCC_NAME_FORMAT_BIND,
                               0, NWCC_RESERVED,conn);
                 if (err)
                    continue;
                 if (NWCXIsSameTree (*conn,treeName))
                     break; /* gotcha*/
                  NWCCCloseConn(*conn);
                  err=NWE_BIND_NO_SUCH_PROP;
	}
	NWCCCloseConn(connInit);
        return err;

#else
 /* much shorter !!!  but does not always works , when nearest server does not belong to the target tree*/
 /* we get a Server unknown error 0x89FC */
 /* printf ("using new code\n");*/
  return  NWCCOpenConnByName(NULL, treeName, NWCC_NAME_FORMAT_NDS_TREE,
                        NWCC_OPEN_NEW_CONN,
                        NWCC_RESERVED, conn);
#endif

}


NWDSCCODE NWCXSplitNameAndContext (NWDSContextHandle ctx, const NWDSChar * dn,
                                   NWDSChar * name, NWDSChar* context) {
	NWDSCCODE err;
	wchar_t wdn[MAX_DN_CHARS + 1];
	wchar_t* cptr;
	wchar_t* wctx;

	err = NWDSXlateFromCtx(ctx, wdn, sizeof(wdn), dn);
	if (err) {
		return err;
	}
	cptr = wdn;
	while (*cptr && *cptr != L'.') {
		if (*cptr == L'\\') {
			if (!*++cptr) {
				return ERR_INVALID_OBJECT_NAME;
			}
		}
		cptr++;
	}
	if (*cptr) {
		wctx = cptr + 1;
		*cptr = 0;
	} else {
		wctx = cptr;
	}

	if (name) {
		err = NWDSXlateToCtx(ctx, name, MAX_DN_BYTES, wdn, NULL);
		if (err) {
			return err;
		}
	}
	if (context) {
		err = NWDSXlateToCtx(ctx, context, MAX_DN_BYTES, wctx, NULL);
		if (err) {
			return err;
		}
	}
	return 0;
}


/* internal only !!!!
 called with "real buffer" for single valued
    or a temp buffer for with multivalued attributes
 we process attributes with variable size (CI_LIST, CI_OCTET..
         and return them as strings with comma separators
attributes with fields are emitted on a single line with "," as separator*/

static NWDSCCODE __docopy_string (UNUSED(NWDSContextHandle ctx), const void* val,
                                   const enum SYNTAX synt, size_t currentSize,
                                   char* result, size_t maxSize){
  int l;
#ifdef DEBUG_PRINT
  printf ("__docopy_string got :%s synt = %d cursize=%d maxsize= %d\n",(char *)val,synt,currentSize,maxSize );
#endif

  if (currentSize > maxSize) return NWE_BUFFER_OVERFLOW;
  if (!result) return ERR_NULL_POINTER;
  switch (synt) {
	case SYN_DIST_NAME:
	case SYN_CI_STRING:
	case SYN_CE_STRING:
        case SYN_PR_STRING:
	case SYN_NU_STRING:
	case SYN_TEL_NUMBER:
	case SYN_CLASS_NAME:
		l = snprintf(result, maxSize, "%s", (const char *)val);
		break;
	case SYN_PATH:{
	        const Path_T* p = (const Path_T*)val;

		l = snprintf(result, maxSize, "%u,%s,%s", p->nameSpaceType, p->volumeName, p->path);
                }
		break;
	case SYN_TYPED_NAME:{
		const Typed_Name_T* tn = (const Typed_Name_T*)val;

		l = snprintf(result, maxSize, "%u,%u,%s", tn->interval, tn->level, tn->objectName);
		}
		break;
	case SYN_FAX_NUMBER:{
	        const Fax_Number_T* fn = (const Fax_Number_T*)val;
		
		l = snprintf(result, maxSize, "%s,%u", fn->telephoneNumber, fn->parameters.numOfBits);
		}
		break;
	case SYN_EMAIL_ADDRESS:{
		const EMail_Address_T* ea = (const EMail_Address_T*)val;
                /*change the SMTP:aaa@bbbb to SMTP,aaa@bbbb */
                char* p=strchr(ea->address,':');

                if (p) *p=',';
		l = snprintf(result, maxSize, "%u,%s", ea->type, ea->address);
		}
		break;
	case SYN_PO_ADDRESS:{
	        const NWDSChar* const* pa = (const NWDSChar* const*)val;

		l = snprintf(result, maxSize, "%s,%s,%s,%s,%s,%s", pa[0], pa[1], pa[2], pa[3], pa[4], pa[5]);
		}
		break;
	case SYN_HOLD:{
	        const Hold_T* h = (const Hold_T*)val;

		l = snprintf(result, maxSize, "%u,%s", h->amount, h->objectName);
		}
		break;
         case SYN_TIMESTAMP:{
                const TimeStamp_T* stamp = (const TimeStamp_T*)val;
                
		l = snprintf(result, maxSize, "%u,%u,%u", stamp->wholeSeconds, stamp->replicaNum, stamp->eventID);
                }
                break;
         case SYN_BACK_LINK:{
                const Back_Link_T* bl = (const Back_Link_T*)val;

		l = snprintf(result, maxSize, "%08X,%s", bl->remoteID, bl->objectName);
                }
                break;
        case SYN_CI_LIST:{
	        const CI_List_T* ol = (const CI_List_T*)val;
                const CI_List_T* p;
                size_t len=1;
                char *aux=result;
                for(p=ol;p;p=p->next) {
                  len=+strlen(p->s)+1;
                }
                if (len >=maxSize)
                        return NWE_BUFFER_OVERFLOW;
                if (len==1) {
                        result[0]=0;
                        return 0;
                }
                for (p=ol;p;p=p->next) {
                        len=strlen(p->s);
                        memcpy(aux,p->s,len);
                        aux+=len;
                        *(aux++)=',';
                }
                *(--aux)=0;
                }
		return 0;

         case SYN_OCTET_LIST:{
                const Octet_List_T* ol = (const Octet_List_T*)val;
                size_t i;
                char *aux;

                if (20 + (ol->length+1)*3+1 >=maxSize)
                        return NWE_BUFFER_OVERFLOW;
                sprintf(result, "%u", ol->length);
		aux = result + strlen(result);
                for (i = 0; i < ol->length; i++) {
                        sprintf(aux, ",%02X", ol->data[i]);
                        aux += 3;
                }
                }
                return 0;
        case SYN_OCTET_STRING:{
                const Octet_String_T* os = (const Octet_String_T*)val;
                size_t i;
		char *aux;
#ifdef DEBUG_PRINT
                printf ("len %d\n",os->length);
#endif
                if (20 + (os->length+1)*3+1 >=maxSize)
                        return NWE_BUFFER_OVERFLOW;
                sprintf(result, "%u", os->length);
		aux = result + strlen(result);
                for (i = 0; i < os->length; i++) {
                        sprintf(aux, ",%02X", os->data[i]);
                        aux += 3;
                }
                }
                return 0;
        case SYN_NET_ADDRESS:{
                const Net_Address_T* na = (const Net_Address_T*)val;
                size_t z;
                char *aux;

                z=na->addressLength;
                if (40 + 3*(z+2)+1  >=maxSize)
                        return NWE_BUFFER_OVERFLOW;
                sprintf(result, "%u,%u", na->addressType, na->addressLength);
		aux = result + strlen(result);
                for (z = 0; z < na->addressLength; z++) {
                        sprintf(aux, ",%02X", na->address[z]);
			aux += 3;
                }
                }
                return 0;
        case SYN_OBJECT_ACL:{
                const Object_ACL_T* oacl = (const Object_ACL_T*)val;

		l = snprintf(result, maxSize, "%s,%s,%08X", oacl->protectedAttrName, oacl->subjectName, oacl->privileges);
                }
                break;
	default:
		return EINVAL;
	}
	if (l < 0 || (size_t)l >= maxSize) {
		return NWE_BUFFER_OVERFLOW;
	}
        return 0;
}

static
NWDSCCODE docopy_string(NWDSContextHandle ctx, const void * val, const enum SYNTAX synt, size_t currentSize,
                                   void * result, size_t maxSize){

        return __docopy_string(ctx, val, synt, currentSize, (char*)result, maxSize);
}


static
NWDSCCODE docopy_int(UNUSED(NWDSContextHandle ctx), const void * val, const enum SYNTAX synt, UNUSED(size_t currentSize),
		     void * result, size_t maxSize) {
#ifdef DEBUG_PRINT
	printf ("docopy_int got :%s synt = %d cursize=%d maxsize= %d\n",(char *)val,synt,currentSize,maxSize );
#endif
	if (!result) {
		return ERR_NULL_POINTER;
	}
	switch (synt) {
		case SYN_BOOLEAN:
			return ncp_put_req_size_unsigned(result, maxSize, *(const Boolean_T*)val);
		case SYN_COUNTER:
			return ncp_put_req_size_unsigned(result, maxSize, *(const Counter_T*)val);
		case SYN_INTEGER:
			return ncp_put_req_size_unsigned(result, maxSize, *(const Integer_T*)val);
		case SYN_INTERVAL:
			return ncp_put_req_size_unsigned(result, maxSize, *(const Integer_T*)val);
		case SYN_TIME:
			return ncp_put_req_size_unsigned(result, maxSize, *(const Time_T*)val);
		default:
			return EINVAL;
	}
}

/***********************************************************************/

struct strlistnode {
   struct strlistnode* next;
   char * str;
};

struct strlist {
  struct strlistnode* first;
  struct strlistnode* last;
};

static struct strlist* initlist (void) {
  struct strlist* l;

  l=(struct strlist*) malloc(sizeof(*l));
  if (l) {
        l->first=l->last=NULL;
  }
  return l;
}

static void freelist ( struct strlist* l) {
  if (l) {
        struct strlistnode* p;
        struct strlistnode* q=NULL;
        for (p=l->first;p;p=q) {
                q=p->next;
                free(p->str);
                free(p);
        }
  }
}

static NWDSCCODE addstring (char * str, struct strlist * l) {

    char * v;
    struct strlistnode* newNode;

    if (!l)
        return ERR_NULL_POINTER;
    v= strdup(str);
    if (!v)
        return ENOMEM;
    newNode=malloc (sizeof (*newNode));
    if (!newNode)
        return ENOMEM;
    newNode->str=v;
    newNode->next=NULL;
    if (l->last)
        l->last->next=newNode;
    else
        l->first=newNode;
   l->last=newNode;
   return 0;

}

static NWDSCCODE strlist_to_string (struct strlist * l, char ** result, char sep) {
/*convert a linked list to a string with sep between strings */
        size_t ln;
        char* aux;
        struct strlistnode * p;

        *result = NULL;

        if (!l)
                return ERR_NULL_POINTER;

        /*compute needed memory */
        ln = 1;
        for (p = l->first; p; p = p->next)
                ln += strlen(p->str) + 1;
        if (ln == 1)
                return 0;

        aux = (char*)malloc(ln);
        if (!aux)
                return ENOMEM;
        *result = aux;
        /*transfert to result with commas between strings */
        for (p = l->first;p; p = p->next) {
                ln = strlen(p->str);
                memcpy(aux, p->str, ln);
                aux += ln;
                *aux++ = sep;
        }
        *--aux = 0; /* remove last comma */
        return 0;
}


/***********************************************************************/

/* append the new string to a linked list */
static NWDSCCODE docopy_multistring(NWDSContextHandle ctx, const void * val, const enum SYNTAX synt, size_t currentSize,
				    void * result, UNUSED(size_t maxSize)) {
	NWDSCCODE err;
	char tmpBuf[MAX_DN_BYTES+1];
	struct strlist* l=(struct strlist *) result;

#ifdef DEBUG_PRINT
	printf ("docopy_multistring got :%s synt = %d cursize=%d maxsize= %d\n",(char *)val,synt,currentSize,maxSize );
#endif
	/*copy string to tmpBuffer */
	err = __docopy_string(ctx, val, synt, currentSize, tmpBuf, sizeof(tmpBuf));
	if (!err) {
		/* and add to list if Ok */
		err= addstring(tmpBuf,l);
	}
	return err;
}



/*-------------------------------------------------------------------------*/
/* Function : ReadAttributesValues                                         */
/*                                                                         */
/* Description :                                                           */
/*   Function for reading an set of attributes values                      */
/*   nearly same as the one used in pam_ncp_auth.c/pamncp.c snapin         */
/*    except a "more complex" copying call back                            */
/*-------------------------------------------------------------------------*/

// element of the table of attributes to read

struct attrop {
	const NWDSChar* attrname;
	NWDSCCODE (*copyval)(NWDSContextHandle, const void* val, const enum SYNTAX synt,
                             size_t currentSize, void* result, size_t maxSize);
	enum SYNTAX synt;
        int maxsize;
};



static NWDSCCODE ReadAttributesValues(NWDSContextHandle ctx,
			 const char* objname,
			 void* arg,
			 struct attrop* atlist) {


	Buf_T* inBuf=NULL;
	Buf_T* outBuf=NULL;
	NWDSCCODE dserr;
	struct attrop* ptr;
	nuint32 iterHandle;


	iterHandle = NO_MORE_ITERATIONS; // test at bailout point !!!

#ifdef DEBUG_PRINT
{
  char aux [8192];
  NWDSCanonicalizeName(ctx,objname,aux);
  printf ("readattr called for %s %s fqdn=%s \n",objname,atlist->attrname,aux);
}
#endif

        dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &inBuf);
	if (dserr) {
		goto bailout;
	}
	dserr = NWDSInitBuf(ctx, DSV_READ, inBuf);
	if (dserr) {
		goto bailout;
	}
	for (ptr = atlist; ptr->attrname; ptr++) {
		dserr = NWDSPutAttrName(ctx, inBuf, ptr->attrname);
		if (dserr) {
			goto bailout;
		}
	}
	dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &outBuf);
	if (dserr) {
		goto bailout;
	}
	do {
		nuint32 attrs;
		dserr = NWDSRead(ctx, objname, DS_ATTRIBUTE_VALUES, 0, inBuf, &iterHandle, outBuf);
		if (dserr) {
                        /* caller must complains or not upon */
			/*ERR_NO_SUCH_ATTRIBUTE*/
			/*ERR_NO_SUCH_ENTRY*/

			goto bailout;
		}
		dserr = NWDSGetAttrCount(ctx, outBuf, &attrs);
		if (dserr) {
			goto bailout;
		}
#ifdef DEBUG_PRINT
   printf ("attr count %d\n",attrs);
#endif
		while (attrs--) {
			char attrname[MAX_SCHEMA_NAME_CHARS+1];
			nuint32 synt;
			nuint32 vals;

			dserr = NWDSGetAttrName(ctx, outBuf, attrname, &vals, &synt);
			if (dserr) {
				goto bailout;
			}
#ifdef DEBUG_PRINT
   printf ("val count %d\n",vals);
#endif

                        while (vals--) {
				size_t sz;
				void* val;

				dserr = NWDSComputeAttrValSize(ctx, outBuf, synt, &sz);
				if (dserr) {
					goto bailout;
				}
				val = malloc(sz);
				if (!val) {
					goto bailout;
				}
				dserr = NWDSGetAttrVal(ctx, outBuf, synt, val);
				if (dserr) {
					free(val);
					goto bailout;
				}
				for (ptr = atlist; ptr->attrname; ptr++) {
#if 0
  printf ("%s:%d:%p\n",ptr->attrname,ptr->synt,ptr->copyval);
#endif

					if (!strcmp(ptr->attrname, attrname))
#if 0
  printf ("ok\n");
#endif

						break;
				}
#if 0
  printf ("%s:%d:%p\n",ptr->attrname,ptr->synt,ptr->copyval);
#endif


#ifdef DEBUG_PRINT
  printf (ptr->copyval? "non null\n":"null\n");
#endif
				if (ptr->copyval) {
					if (ptr->synt != synt) {
                                             dserr=ERR_NO_SUCH_ATTRIBUTE;
					} else {
#ifdef DEBUG_PRINT
  printf ("zy go\n");
#endif

						dserr= ptr->copyval(ctx,val,synt,sz, arg,ptr->maxsize);
					}
				}else {
                                        dserr=ERR_NO_SUCH_ATTRIBUTE;
                                         /* mistyped: happens with upcae/lowcase errors */
                                }
				free(val);
				if (dserr) {
					goto bailout;
				}
			}
		}
	} while (iterHandle != NO_MORE_ITERATIONS);
bailout:;
	if (iterHandle != NO_MORE_ITERATIONS) {
	    // let's keep the final dserr as the 'out of memory error' from ptr->getval()
		NWDSCloseIteration(ctx, DSV_READ, iterHandle);
	}
	if (inBuf) NWDSFreeBuf(inBuf);
	if (outBuf) NWDSFreeBuf(outBuf);
#ifdef DEBUG_PRINT
  printf ("readattr ended with code :%s\n",strnwerror(dserr));
#endif

	return dserr;
}



/* specific buffer extracting functions that replace docopy_* */
struct nw_home_info {
	char*	hs;
	char*  hrn;
};


/* Note that this function is completely wrong: host server is DN, and you should use DN operations on it...
 * Cutting it at first dot is nonsense... It might work, but there is no reason why it should... */
static NWDSCCODE docopy_host_server(UNUSED(NWDSContextHandle ctx),
                                    const void* val, UNUSED(const enum SYNTAX synt),
                                    UNUSED(size_t currentSize), void* result, UNUSED(size_t maxSize)) {
	struct nw_home_info* hi = (struct nw_home_info*)result;
        char buf [MAX_DN_CHARS];
        char * dot;
        char *v;
#ifdef DEBUG_PRINT
  printf ("docopy_host_server got :%s synt = %d cursize=%d maxsize= %d\n",(char *)val,synt,currentSize,maxSize );
#endif

        strcpy(buf,(const char *) val);
        dot = strchr(buf, '.');  // trim out the context part ( it that good enough ???)
        if (dot) *dot=0;
        v = strdup(buf);
	if (!v) {
		return ENOMEM;
	}
	hi->hs = v;
	return 0;
}

static NWDSCCODE docopy_host_resource_name(UNUSED(NWDSContextHandle ctx),
                                    const void* val, UNUSED(const enum SYNTAX synt),
                                    UNUSED(size_t currentSize), void* result, UNUSED(size_t maxSize)) {
	struct nw_home_info* hi = (struct nw_home_info*)result;


        char* v = strdup((const char*)val);

#ifdef DEBUG_PRINT
  printf ("docopy_host_resource got :%s synt = %d cursize=%d maxsize= %d\n",(char *)val,synt,currentSize,maxSize );
#endif

        if (!v) {
		return ENOMEM;
	}
	hi->hrn =v;
	return 0;
}

static NWDSCCODE docopy_home_directory(UNUSED(NWDSContextHandle ctx),
                                        const void* val, UNUSED(const enum SYNTAX synt),
                                        UNUSED(size_t currentSize), void* result, UNUSED(size_t maxSize)) {


      const Path_T* pa = (const Path_T*) val;
      Path_T* result_int = (Path_T*)result;
      char * v;


      result_int->nameSpaceType=pa->nameSpaceType;
      v = strdup(pa->path);
      if (!v)
        return ENOMEM;
      result_int->path=v;

      v = strdup(pa->volumeName);
      if (!v)
        return ENOMEM;
      result_int->volumeName=v;

      return 0;
}

/******************************************************************************/
NWDSCCODE NWCXGetNDSVolumeServerAndResourceName(NWDSContextHandle ctx,
                                                const NWDSChar* ndsName,
                                                NWDSChar* serverName,
                                                size_t serverNameMaxLen,
                                                NWDSChar *resourceName,
                                                size_t resourceNameMaxLen){

	static  struct attrop atlist[] = {
		{ATTR_HOST_SERVER, 	docopy_host_server,	        SYN_DIST_NAME   ,9999},
                {ATTR_HOST_RN,          docopy_host_resource_name,      SYN_CI_STRING   ,9999},
		{ NULL,		        NULL,			        SYN_UNKNOWN     ,0 }};

        struct nw_home_info hi ={NULL,NULL};
        NWDSCCODE dserr;

        if (!ndsName )
                return ERR_NULL_POINTER;


	dserr=ReadAttributesValues(ctx,ndsName,&hi,atlist);
        if (!dserr) {
                if (serverName && hi.hs) {
                        if (strlen(hi.hs) <serverNameMaxLen)
                                 strcpy(serverName,hi.hs);
                        else
                                dserr=NWE_BUFFER_OVERFLOW;
                }

                if (resourceName &&hi.hrn) {
                        if (strlen(hi.hrn) <resourceNameMaxLen)
                                strcpy(resourceName,hi.hrn);
                        else
                                dserr= NWE_BUFFER_OVERFLOW;
               }

       }
#ifdef DEBUG_PRINT
  printf ("%s %s:\n",hi.hs,hi.hrn);
#endif

       if (hi.hs) free( hi.hs);
       if (hi.hrn) free( hi.hrn);
       return dserr;

}


NWDSCCODE NWCXGetObjectHomeDirectory(NWDSContextHandle ctx,
                                     const NWDSChar* ndsName,
                                     NWDSChar* serverName,
                                     size_t serverNameMaxLen,
                                     NWDSChar *resourceName,
                                     size_t resourceNameMaxLen,
                                     NWDSChar* NDSVolumeName,
                                     size_t NDSVolumeNameMaxLen,
                                     NWDSChar* pathName,
                                     size_t pathNameMaxLen) {

        Path_T reply={0,NULL,NULL};

        static  struct attrop atlist[] = {
	        { ATTR_HOME_NW , 	docopy_home_directory,   SYN_PATH   ,9999},
	        { NULL,		        NULL,			SYN_UNKNOWN     ,0 }};
         NWDSCCODE dserr;

        if (!ndsName )
                return ERR_NULL_POINTER;

	dserr=ReadAttributesValues(ctx,ndsName,&reply,atlist);

        if (!dserr) {
           if (NDSVolumeName && reply.volumeName) {
             if (strlen(reply.volumeName) <NDSVolumeNameMaxLen)
                                 strcpy(NDSVolumeName,reply.volumeName);
                        else
                                dserr=NWE_BUFFER_OVERFLOW;
           }

           if (pathName && reply.path) {
             if (strlen(reply.path) <pathNameMaxLen)
                                 strcpy(pathName,reply.path);
                        else
                                dserr=NWE_BUFFER_OVERFLOW;
           }

           if (serverName || resourceName){
                if (reply.volumeName)
                        dserr=NWCXGetNDSVolumeServerAndResourceName(ctx,reply.volumeName,
                                                serverName,serverNameMaxLen,
                                                resourceName,resourceNameMaxLen);
                       else dserr=ERR_NO_SUCH_ATTRIBUTE;

           }
       }
#ifdef DEBUG_PRINT
  printf ("%s %s\n",reply.volumeName,reply.path);
  printf ("%s %s %s %s\n",NDSVolumeName,pathName,serverName,resourceName);
#endif

        if (reply.path) free (reply.path);
        if (reply.volumeName) free (reply.volumeName);

#ifdef DEBUG_PRINT
  printf ("%s:\n",strnwerror(dserr));
#endif
        return dserr;

}


NWDSCCODE NWCXGetObjectLastLoginTime(NWDSContextHandle ctx,
                                     const NWDSChar* ndsName,
                                     time_t * tm){
	int int_tm;
	NWDSCCODE err;

        err = NWCXGetIntAttributeValue(ctx, ndsName, ATTR_LOGIN_TIME, &int_tm);
	if (!err) {
		*tm = int_tm;
	}
	return err;
}

NWDSCCODE NWCXGetObjectMessageServer(NWDSContextHandle ctx,
                                     const NWDSChar* ndsName,
                                     NWDSChar* serverName,
                                     int serverNameMaxLen){

        return NWCXGetStringAttributeValue(ctx,ndsName,ATTR_MESSAGE_SERVER,serverName,serverNameMaxLen);
}


/*** login script reading & writing ****************************************/

struct loginScriptInfo {
        const NWDSChar * user;
        char * buffer;
        int  bytesRead;
        int max;
};


static NWDSCCODE docopy_login_script(NWDSContextHandle ctx, const void * val, const enum SYNTAX synt, size_t currentSize,
                                   void * result, size_t maxSize){

// code from contrib/testing/dstream.c

        static char attrName[MAX_DN_CHARS + 1]=ATTR_LOGIN_SCRIPT;
        struct loginScriptInfo* lsi=result;
        nuint8 hndl[6];
        NWCONN_HANDLE fileConn;
        ncp_off64_t fileSize;

        NWDSCCODE err ;

        (void) val;(void) maxSize;(void) synt;(void) currentSize; // warnings out !

        if (!lsi->buffer) return ERR_NULL_POINTER;
#if 0
        printf("do_copy_login_script called for %s\n", lsi->user);
#endif
        //err=ERR_NO_SUCH_ATTRIBUTE;

         err= __NWDSOpenStream(ctx, lsi->user, attrName, 0, &fileConn, hndl, &fileSize);
         if(err==0){
                lsi->bytesRead=ncp_read ( fileConn,hndl,0, lsi->max-1,lsi->buffer);
                if (lsi->bytesRead <0 )   {// some error occured
		 err=-1;
		 lsi->bytesRead=0;
                }
                lsi->buffer[lsi->bytesRead]='\0';
                ncp_close_file(fileConn,hndl);
                NWCCCloseConn(fileConn);
#if  0
                 printf("%s:%d/%d bytes read from login script, err=%d\n",lsi->user, lsi->bytesRead,lsi->max,err);
#endif
          }
          return err;
}

NWDSCCODE NWCXGetObjectLoginScript(NWDSContextHandle ctx,const NWDSChar* objectName,
                                        char* buffer, int * len, int maxlen) {
       static  struct attrop atlist[] = {
	        { ATTR_LOGIN_SCRIPT , 	docopy_login_script,    SYN_STREAM      ,9999},
	        { NULL,		        NULL,			SYN_UNKNOWN     ,0 }};
       NWDSCCODE dserr;

  struct loginScriptInfo lsi;

  if (!objectName) return ERR_NULL_POINTER;

  lsi.user=objectName;
  lsi.buffer=buffer;
  lsi.max=maxlen;
  lsi.bytesRead=0;

  dserr= ReadAttributesValues(ctx,objectName,&lsi,atlist);
  if (!dserr)  {
     *len=lsi.bytesRead;
  }
        return dserr;
}

NWDSCCODE NWCXGetContextLoginScript(NWDSContextHandle ctx,const NWDSChar* objectName,
                                        char* buffer, int * len,int maxlen){
// PP: I am not too happy with this code, but it seems to works even if current user's have a
// Default Name context in environnment of a .nwclient file.

        char context[MAX_DN_BYTES+1];
        char fqdn[MAX_DN_BYTES+1];
        NWDSCCODE dserr;
        NWDSContextHandle ctx2;

        if (!objectName) return ERR_NULL_POINTER;
        // we must convert relative name to a FQDN and then climb up contexts up to root
         dserr= NWDSCanonicalizeName(ctx,objectName,fqdn);
        if (!dserr) {
                // we must duplicate context handle and have a [Root] name context there
                dserr= NWDSDuplicateContextHandle(ctx, &ctx2);
                if (dserr)
                        return dserr;
                dserr=NWDSSetContext(ctx2, DCK_NAME_CONTEXT, "[Root]");
                if (dserr)  {
                        NWDSFreeContext(ctx2);
                        return dserr;
                }
                // let's climb up until we found one
                dserr=NWCXSplitNameAndContext(ctx2,fqdn,NULL,context);
                if (!dserr) {
                         //printf ("trying to get context LS of %s from %s\n",fqdn,context);
                        dserr=ERR_NO_SUCH_ATTRIBUTE;
                        while (dserr==ERR_NO_SUCH_ATTRIBUTE && context[0]) {
	                 //printf ("trying to get context LS from %s\n",context);
                                dserr=NWCXGetObjectLoginScript(ctx2,context,buffer,len,maxlen);
                                if (dserr)
                                   NWCXSplitNameAndContext(ctx2,context,NULL,context);
                        }
                }
	       NWDSFreeContext(ctx2);
         }
        return dserr;
}

NWDSCCODE NWCXGetProfileLoginScript(NWDSContextHandle ctx,const NWDSChar* objectName,
				    char* buffer, int * len, int maxlen){
	char profile[MAX_DN_BYTES+1];
	NWDSCCODE dserr;

	if (!objectName) {
		return ERR_NULL_POINTER;
	}

	dserr = NWCXGetStringAttributeValue(ctx, objectName, ATTR_PROFILE, profile, sizeof(profile));
	if (!dserr) {
#if 0
		printf ("reading Login Script of profile  %s for object %s\n",profile,objectName);
#endif
		dserr = NWCXGetObjectLoginScript(ctx, profile, buffer, len, maxlen);
	}
	return dserr;
}



/* read a single valued string attribute */
NWDSCCODE NWCXGetStringAttributeValue (NWDSContextHandle ctx,
                                        const NWDSChar* objectName,
                                        const NWDSChar* attrName,
                                        char* buffer, int maxlen) {
       struct attrop atlist[] = {
	        { attrName, 	docopy_string,  SYN_UNKNOWN,	maxlen},
	        { NULL,		NULL,		SYN_UNKNOWN,	0 }};
       NWDSCCODE dserr;


       if (!objectName )
                return ERR_NULL_POINTER;

       dserr=NWDSGetSyntaxID(ctx, attrName, &atlist[0].synt);
#ifdef DEBUG_PRINT
  printf ("err NWDSGetSyntaxID %s for object %s\n",attrName,strnwerror(dserr));
#endif
        if (dserr)
               return dserr;
        switch (atlist[0].synt) {
        	case SYN_COUNTER:
	        case SYN_INTEGER:
        	case SYN_INTERVAL:
	        case SYN_BOOLEAN:
                case SYN_TIME:
			dserr=EINVAL;
			break;
                default:
			dserr=ReadAttributesValues(ctx,objectName,buffer,atlist);
                        break;

       }
        return dserr;
}


/* read a single valued numeric/boolean attribute */
NWDSCCODE NWCXGetIntAttributeValue     (NWDSContextHandle ctx,
                                        const NWDSChar* objectName,
                                        const NWDSChar* attrName,
                                        int * value){
       int aux=0;
       struct attrop atlist[] = {
	        { attrName, 	docopy_int,     SYN_UNKNOWN,	sizeof(aux) },
	        { NULL,		NULL,		SYN_UNKNOWN,	0 }};
       NWDSCCODE dserr;


       if (!objectName )
                return ERR_NULL_POINTER;

       dserr=NWDSGetSyntaxID(ctx, attrName, &atlist[0].synt);
#ifdef DEBUG_PRINT
  printf ("err NWDSGetSyntaxID %s for object %s\n",attrName,strnwerror(dserr));
#endif
        if (dserr)
               return dserr;
        switch (atlist[0].synt) {
        	case SYN_COUNTER:
	        case SYN_INTEGER:
        	case SYN_INTERVAL:
	        case SYN_BOOLEAN:
                case SYN_TIME:
                        dserr=ReadAttributesValues(ctx,objectName,&aux,atlist);
                        break;
                default: dserr=EINVAL;
       }
       if (!dserr)
                *value= aux;
        return dserr;
}

/* read a multi valued string attribute */
/*does check for syntax flag being DS_STRING and ! DS_SINGLE */
/* caller MUST free the buffer created here */
NWDSCCODE NWCXGetMultiStringAttributeValue (NWDSContextHandle ctx,
                                        const NWDSChar* objectName,
                                        const NWDSChar* attrName,
                                        char** buffer) {
	struct attrop atlist[] = {
		{ attrName, 	docopy_multistring,     SYN_UNKNOWN,	MAX_DN_BYTES + 1},
		{ NULL,		NULL,		        SYN_UNKNOWN     ,0 }};
	NWDSCCODE dserr;
	struct strlist *l=NULL;

	if (!objectName )
		return ERR_NULL_POINTER;

	dserr=NWDSGetSyntaxID(ctx, attrName, &atlist[0].synt);
#ifdef DEBUG_PRINT
  printf (" NWDSGetSyntaxID (%d) %s for object %s returned :%s\n",atlist[0].synt,attrName,objectName,strnwerror(dserr));
#endif
	if (dserr)
		return dserr;

	l = initlist();
	if (!l)
		return ENOMEM;

        switch (atlist[0].synt) {
        	case SYN_COUNTER:
	        case SYN_INTEGER:
        	case SYN_INTERVAL:
	        case SYN_BOOLEAN:
                case SYN_TIME:
			dserr=EINVAL;
			break;
                default:
                        dserr=ReadAttributesValues(ctx,objectName,l,atlist);
                        break;

	}
        if (!dserr) {
		dserr=strlist_to_string(l,buffer,',');
        }
        freelist(l);
        return dserr;
}



/* read & convert to string a single valued attribute */
NWDSCCODE NWCXGetAttributeValueAsString (NWDSContextHandle ctx,
                                        const NWDSChar* objectName,
                                        const NWDSChar* attrName,
                                        char* buffer, size_t maxlen) {

        enum SYNTAX synt;
        int auxint=0;
        NWDSCCODE dserr;
        char aux[128];

        dserr=NWDSGetSyntaxID(ctx, attrName, &synt);
#ifdef DEBUG_PRINT
  printf ("NWDSGetSyntaxID: %s %s %d\n",strnwerror(dserr),attrName,synt);
#endif

       if (dserr)
                return dserr;
       switch (synt) {
        	case SYN_COUNTER:
	        case SYN_INTEGER:
        	case SYN_INTERVAL:
	        case SYN_BOOLEAN:
                        dserr=NWCXGetIntAttributeValue(ctx,objectName,attrName,&auxint);
                        if (!dserr) {
                                if (synt == SYN_BOOLEAN) {
					sprintf(aux,"%s", (auxint == 0)?"false":"true");
				} else {
					sprintf(aux,"%u",auxint);
				}
                                if (strlen(aux)<maxlen)
                                        strcpy(buffer,aux);
                                else dserr=NWE_BUFFER_OVERFLOW;
                        }
                        break;
		case SYN_TIME:
                        dserr=NWCXGetIntAttributeValue(ctx,objectName,attrName,&auxint);
                        if (!dserr) {
				time_t t;
				
				t = auxint;
				sprintf(aux,"%s", ctime(&t));
                                /* remove LF added by ctime */
                                if (strlen(aux)>0 && aux[strlen(aux)-1]=='\n')
                                	aux[strlen(aux)-1]=0;
                                if (strlen(aux)<maxlen)
                                        strcpy(buffer,aux);
                                else dserr=NWE_BUFFER_OVERFLOW;
                        }
                        break;
                default:
                        dserr=NWCXGetStringAttributeValue(ctx,objectName,attrName,buffer,maxlen);
       }
       return dserr;
}
#endif	/* NDS_SUPPORT */
