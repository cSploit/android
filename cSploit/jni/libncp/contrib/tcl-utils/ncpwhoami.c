/*
    ncpwhoami - Returns details about all permanent connections
    belonging to current user
    Copyright (C) 2001 by Patrick Pollet

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

	0.00  2001	February 2001		Patrick Pollet
		Initial revision.
        1.00  2001	March 7th 2001		Patrick Pollet
                Corrected #140: use of NWCXGetPermConnInfo() for authentication state
                          #186: use of NWCXSplitNameAndContext()
                          #315: NWCCloseConn(conns[j]) and not NWCCloseConn(theConn)
                          #381: removed exit(1) in main after call to usage().
                Added the word "failed" to all error messages to
                simplify TCL/Tk parsing.
                Reorganized help options by alphabetic order.

	1.01  2001	March 16th		Patrick Pollet
                Added many NDS informations ( this program is the "major tester"
                for /lib/nwclient.c)
                Added switch -1 to show only the first connection to tree

 	1.02  2001	December 16		Petr Vandrovec
		const <-> non const pointers cleanup.
		NWDSAddConnection must be invoked before any other NWDS* function
			which can communicate with tree.

 	1.023 2002	January 24		Patrick Pollet
	        activated login scripts reading options 

	1.03  2002	March 15		Patrick Pollet
		new release of nwnet set name context to default name context if found
		so retrieving name context (-fc) or FQDN (-fD) got broken

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <ncp/nwclient.h>
#include <mntent.h>
#include <pwd.h>
#include "getopt.h"

#include "private/libintl.h"
#define _(X) X

static const char *progname;

/* three globals */
/*default separator is tab*/
static char gSeparator='\t';
/* by default we print the following data in that order
   Tree,Server,Netware Volume,Netware User, Unix User */
static const char * gDefaultFields ="TSVUX";
static const char * gUserFields=NULL;


static void
usage(void)
{
	fprintf(stderr, _("failed. usage: %s [-options] \n"), progname);
        exit(1);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [-options] \n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
               "-s              Specify fields separator (default \\t)\n"
               "-f              Specify fields (A..Z a..z) & order of display (default=-f TSVXU)\n"
               "                   A=Authentication state\n"
               "                   a=NDS postal addresses informations\n"
               "                   B=Broadcast state\n"
               "                   b NDS stored Bindery property\n"
               "                   C=connexion number\n"
               "                   c=NDS Name context\n"
               "                   D=NDS distinguished name\n"
               "                   d=NDS description\n"
               "                   E=NDS Foreign EMail address\n"
               "                   e=NDS Internet EMail address\n"
               "                   F=Netware File Server version\n"
               "                   f=NDS Full Name\n"
               "                   G=NDS group membership\n"
               "                   g=NDS given name\n"
               "                   H=Netware directory handle\n"
               "                   h=NDS home directory \n"
               "                   I=mount UID\n"
               "                   i=NDS initials\n"
          /*     "                   J= \n"*/
          /*     "                   j= \n"*/
               "                   K=NDS Public Key\n"
          /*     "                   k= \n"*/
               "                   L=NDS login time\n"
               "                   l=LINUX:* NDS properties\n"
               "                   M=mount point\n"
               "                   m=NDS message server\n"
               "                   N=Netware volume number\n"
               "                   n=NDS Network Address Restrictions\n"
               "                   O=NDS organization\n"
               "                   o=NDS location\n"
               "                   P=NDS profile\n"
               "                   p=NDS password informations\n"
               "                   Q=NDS language\n"
               "                   q=NDS generational qualifier\n"
               "                   R=NDS login restrictions\n"
               "                   r=NDS rights (ACL's)\n"
               "                   S=server bindery name\n"
               "                   s=NDS surname\n"
               "                   T=tree name\n"
               "                   t=NDS phone infos (telephon & fax)\n"
               "                   U=Netware user name\n"
               "                   u=unix:* NDS properties\n"
               "                   V=Netware volume name\n"
               "                   v=NCPFS version\n"
               "                   W=Netware UID\n"
               "                   w=NDS Network addresses (where logged in)\n"
               "                   X=Unix user name\n"
               "                   x=NDS title(s)\n"
               "                   Y=NDS Security equals to\n"
        /*       "                   y=\n"*/
        /*       "                   Z=\n"*/
               "                   z=NDS Other names (CN+ ozer names ;-)\n"
               "                   0=NDS login script\n"
               "                   1=NDS profil login script\n"
               "                   2=NDS context login script\n"

               "-T tree_name    Only connections to that tree\n"
               "-S server_name  Only connections to that server\n"
               "-g              All connections (default is only mine(s)) \n"
               "                  Only root can use this flag\n"
               "-a              All possible informations\n"
               "-1              Only the first connection to each tree\n"
	       "\n"));
        exit(1);
}





static NWCCODE processServer ( NWCONN_HANDLE conn, const char* treeName) {

        NWCCODE err;
        NWObjectID nwOid;
        NWCONN_NUM connNum;
        struct NWCCRootEntry rootEntry;
        NWCCVersion nwVersion;
        uid_t mountUID;
        int bcast;
        char buffer[8192];
        int len=sizeof(buffer);
        unsigned int i;

        char NDSname[MAX_DN_BYTES+1];
        NWDSContextHandle ctx=NULL;
        u_int32_t ctxflag;

        char separator=gSeparator;
        unsigned ll=strlen(gUserFields);


#define ERR_MARK  "***"
#define UNK_MARK  "???"

        if (!conn)
                return 0; /*already seen*/
        /* we are dealing with permanent connexions */
        err=NWCXGetPermConnInfo(conn,NWCC_INFO_MOUNT_UID, sizeof (uid_t), &mountUID);
        if (err)
              mountUID=-1;
       /* set up a context for NDS requests */
       /* if failed NDSname stays empty (tested below)*/
        NDSname[0]=0;
        if (NWIsDSServer(conn,NULL)) { /*do not talk NDS to bindery servers */
                  err = NWDSCreateContextHandle(&ctx);
                  if (!err) {
                        err=NWDSAddConnection (ctx,conn);
			if (!err) {
	                       err = NWDSGetContext(ctx, DCK_FLAGS, &ctxflag);
        	               ctxflag |= DCV_XLATE_STRINGS+DCV_TYPELESS_NAMES;
			}
                        if (!err)
                                err = NWDSSetContext(ctx, DCK_FLAGS, &ctxflag);
                        if (!err)
                                err=NWCXGetPermConnInfo(conn,NWCC_INFO_USER_ID,  sizeof(NWObjectID), &nwOid);
                        if (!err)
                                err=NWDSMapIDToName(ctx, conn, nwOid, NDSname);

			/*
        		err = NWDSGetContext(ctx, DCK_NAME_CONTEXT, buffer);
			if (err) {
				fprintf(stderr, "NWDSSetContext(DCK_NAME_CONTEXT) failed: %s\n",
					strnwerror(err));

			}else printf ("%s%c\n",buffer,separator);
			*/

                   }
        }

        for (i=0;i<ll;i++) {
          /* change separator to EOL at the last field*/
          if (i==ll-1)
                  separator='\n';
          buffer[0]=0;
          switch (gUserFields[i]) {
             case  'A':
                    err=NWCXGetPermConnInfo(conn,NWCC_INFO_AUTHENT_STATE, sizeof(bcast), &bcast);
                    if (err)
                       printf ("%s%c",ERR_MARK,separator); /* don't mess up display */
                    else
                       printf ("%d%c",bcast,separator);
                    break;
             case 'a':   { err=-1;
                       if (NDSname[0]) {
                                char aux[6*36]=""; /*biggest answer is 6 postal addresses with 30 chars max */
                                buffer[0]=0;
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"S",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"SA",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Physical Delivery Office Name",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Postal Office Box",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Postal Code",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);

                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Postal Address",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);

                                printf ("%s%c", buffer,separator);

                           }
                        }
                        break;

             case  'B':
                    err=NWCXGetPermConnInfo(conn,NWCC_INFO_BCAST_STATE, sizeof(bcast), &bcast);
                    if (err)
                       printf ("%s%c",ERR_MARK,separator);
                    else
                       printf ("%d%c",bcast,separator);
                    break;
             case  'b':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Bindery Property",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);
                        break;


             case  'C':
                     err=NWCXGetPermConnInfo(conn,NWCC_INFO_CONN_NUMBER,  sizeof(NWCONN_NUM), &connNum);
                     if (err)
                        printf("%s%c",ERR_MARK,separator);
                    else
                        printf ("%d%c",connNum,separator);
                    break;

             case  'c':
                        err=-1;
                        if (NDSname[0]) {
				if (!NWDSCanonicalizeName (ctx,NDSname,buffer))
                                	err = NWCXSplitNameAndContext(ctx,buffer,NULL,buffer);
			}
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c",buffer,separator);
                        break;
            case  'D':
			err=-1;
			if (NDSname[0])
				err = NWDSCanonicalizeName (ctx,NDSname,buffer);
			if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                        	printf ("%s%c",buffer,separator);

                        break;
            case  'd':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Description",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else {
                               /*description may have cr/lf in it */
                               unsigned int j;
                               for (j=0;j<strlen(buffer);j++)
                                 printf("%c",buffer[j]>=32? buffer[i]:' ');

                                printf ("%c", separator);
                        }
                        break;

            case  'E':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"EMail Address",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);
                        break;
            case  'e':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Internet EMail Address",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);

                        break;

            case  'F':
                        err=NWCXGetPermConnInfo(conn,NWCC_INFO_SERVER_VERSION,  sizeof(nwVersion), &nwVersion);
                        if (err)
                                 printf("%s%c",ERR_MARK,separator);
                        else
                                printf ("%d %d %d%c",nwVersion.major,nwVersion.minor,nwVersion.revision,separator);
                        break;
            case  'f':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Full Name",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);
                        break;
            case  'G': {
                        char * NDSresult=NULL;
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"Group Membership",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);
                        }
                        break;

            case  'g':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Given Name",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);
                        break;

            case  'H':
                       err=NWCXGetPermConnInfo(conn,NWCC_INFO_ROOT_ENTRY, sizeof(rootEntry), &rootEntry);
                       if (err)
                               printf ("%s%c",ERR_MARK,separator);
                       else
                               printf ("%x%c",rootEntry.dirEnt,separator);
                        break;
            case  'h':
                       err=-1;
                       if (NDSname[0]){
                           char path[MAX_DN_BYTES+1]="";
                           char NDSvolname[MAX_DN_BYTES+1]="";
                           char server[MAX_DN_BYTES+1]="";
                           char resource[MAX_DN_BYTES+1]="";
                           err=NWCXGetObjectHomeDirectory(ctx,NDSname,
                                                          server,sizeof(server),
                                                          resource,sizeof(resource),
                                                          NDSvolname,sizeof(NDSvolname),
                                                          path,sizeof(path));
                           if (!err)
                               sprintf(buffer,"%s,%s,%s,%s",NDSvolname,server,resource,path);

                      }
                      if (err)
                                printf ("%s%c",ERR_MARK,separator);
                      else
                                printf ("%s%c",buffer,separator);
                      break;
             case  'I':
                    if (mountUID ==(uid_t)-1)
                         printf("%s%c",ERR_MARK,separator);
                    else
                        printf ("%d%c",mountUID,separator);
                    break;
            case  'i':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Initials",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);
                        break;
            case  'K':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Public Key",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);
                        break;



            case  'L':
                        err=-1;
                        if (NDSname[0]){
                                /*
                                time_t time=0;
                                err=NWCXGetObjectLastLoginTime(ctx,NDSname,&time);
                                if (!err) {
                                        sprintf(buffer,ctime(&time));
                                        // remove LF added by ctime
                                        if (strlen(buffer)>0 )
                                                 buffer[strlen(buffer)-1]=0;
                              }
                              */
                              err=NWCXGetAttributeValueAsString(ctx,NDSname,"Login Time",buffer,sizeof(buffer));
                        }
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c",buffer,separator);
                        break;

             case 'l':   { err=-1;
                       if (NDSname[0]) {
                                char aux[128]="";
                                buffer[0]=0;
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"LINUX:UID",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"LINUX:GID",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"LINUX:Primary GroupID",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"LINUX:Primary GroupName",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"LINUX:Login Shell",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"LINUX:Home Directory",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"LINUX:Comments",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);

                                printf ("%s%c", buffer,separator);

                           }
                        }
                        break;

            case  'M':
                     err=NWCXGetPermConnInfo(conn,NWCC_INFO_MOUNT_POINT, len, buffer);
                     if (err)
                         printf("%s%c",ERR_MARK,separator);
                    else
                        printf ("%s%c",buffer,separator);
                    break;
            case  'm':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetObjectMessageServer(ctx,NDSname,buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c",buffer,separator);
                        break;
            case  'N':
                    err=NWCXGetPermConnInfo(conn,NWCC_INFO_ROOT_ENTRY, sizeof(rootEntry), &rootEntry);
                    if (err)
                       printf ("%s%c",ERR_MARK,separator);
                    else
                       printf ("%x%c",rootEntry.volume,separator);
                    break;
            case  'n': {
                        char * NDSresult=NULL;
                        err=-1;

                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"Network Address Restriction",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);
                        }
                        break;
            case  'O': {
                        char * NDSresult=NULL;
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"OU",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);
                        }
                        break;
            case  'o': {
                        char * NDSresult=NULL;
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"L",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);
                        }
                        break;
            case  'P':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Profile",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);
                        break;
            case 'p':   { err=-1;
                        if (NDSname[0]) {
                                char aux[128]="";
                                buffer[0]=0;
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Password Required",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Password Unique Required",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Password Allow Change",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Password Minimum Length",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Password Expiration Time",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Password Expiration Interval",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);

                                printf ("%s%c", buffer,separator);

                           }
                        }
                        break;
           case  'Q':   {
                        char * NDSresult=NULL;
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"Language",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);
                        }
                        break;

           case  'q':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Generational Qualifier",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);
                        break;

            case 'R':   { err=-1;
                       if (NDSname[0]) {
                                char aux[128]="";
                                buffer[0]=0;
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Login Disabled",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Login Expiration Time",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Login Grace Limit",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Login Grace Remaining",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Login Intruder Attempts",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Login Intruder Reset Time",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Login Maximum Simultaneous",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);

                                printf ("%s%c", buffer,separator);

                           }
                        }
                        break;
                case  'r': {
                                char * NDSresult=NULL;
                                err=-1;

                                if (NDSname[0])
                                        err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"ACL",&NDSresult);
                                if (err)
                                        printf ("%s(%s)%c",ERR_MARK,strnwerror(err),separator);
                                else
                                        printf ("%s%c", NDSresult,separator);
                                if (NDSresult)
                                        free(NDSresult);
                                }
                                break;

            case  'S':
                    err=NWCXGetPermConnInfo(conn,NWCC_INFO_SERVER_NAME, len, buffer);
                    if (err)
                        printf("%s%c",ERR_MARK,separator);
                    else
                        printf("%s%c",buffer,separator);
                    break;
            case  's':
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"Surname",buffer,sizeof(buffer));
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", buffer,separator);
                        break;

            case  'T':
                        printf("%s%c",treeName,separator);
                        break;
            case  't': {
                        char * NDSresult=NULL;
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"Telephone Number",&NDSresult);
                        if (err)
                                printf ("%s,",ERR_MARK);
                        else
                                printf ("%s,", NDSresult);
                        if (NDSresult)
                                free(NDSresult);
                        NDSresult=NULL;
                        err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"Facsimile Telephone Number",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);

                        }
                        break;
            case  'U':
                    err=NWCXGetPermConnInfo(conn,NWCC_INFO_USER_NAME, len, buffer);
                    if (err)
                        printf("%s%c",ERR_MARK,separator);
                    else
                        printf("%s%c",buffer,separator);
                    break;
           case 'u':   { err=-1;
                       if (NDSname[0]) {
                                char aux[128]="";
                                buffer[0]=0;
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"UNIX:UID",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"UNIX:GID",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"UNIX:Primary GroupID",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"UNIX:Primary GroupName",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"UNIX:Login Shell",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"UNIX:Home Directory",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);
                                strcat(buffer,",");
                                err=NWCXGetAttributeValueAsString(ctx,NDSname,"UNIX:Comments",aux,sizeof(aux));
                                if (!err)
                                        strcat(buffer,aux);
                                else
                                        strcat(buffer,ERR_MARK);

                                printf ("%s%c", buffer,separator);

                           }
                        }
                        break;
           case  'V':
                    err=NWCXGetPermConnInfo(conn,NWCC_INFO_ROOT_ENTRY, sizeof(rootEntry), &rootEntry);
                    if (!err)
                         err=NWGetVolumeName(conn,rootEntry.volume,buffer);
                    if (err)
                       printf ("%s%c",ERR_MARK,separator);
                    else
                       printf ("%s%c",buffer,separator);
                    break;
           case 'v':
                     printf("%s%c", NCPFS_VERSION,separator);
                     break;
           case  'W':
                    err=NWCXGetPermConnInfo(conn,NWCC_INFO_USER_ID,  sizeof(NWObjectID), &nwOid);
                    if (err)
                                printf("%s%c",ERR_MARK,separator);
                    else
                        printf ("%x%c",nwOid,separator);

                    break;
            case  'w':   {
                        char * NDSresult=NULL;
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"Network Address",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);
                        }
                        break;

            case  'X':{
                        struct passwd* pwd=NULL;
                        char unixUserName [128];
                        setpwent();
                        pwd=getpwuid(mountUID);
                        endpwent();
                        if (pwd)
                            strcpy(unixUserName,pwd->pw_name);
                        else
                            strcpy(unixUserName,ERR_MARK);
                     printf ("%s%c",unixUserName,separator);
                     }
                     break;
            case  'x': {
                        char * NDSresult=NULL;
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"Title",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);
                        }
                        break;
          case  'Y':   {
                        char * NDSresult=NULL;
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"Security Equals",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);
                        }
                        break;

        case  'z':   { /*ozer names as frenchmen say ;-)*/
                        char * NDSresult=NULL;
                        err=-1;
                        if (NDSname[0])
                                err=NWCXGetMultiStringAttributeValue(ctx,NDSname,"CN",&NDSresult);
                        if (err)
                                printf ("%s%c",ERR_MARK,separator);
                        else
                                printf ("%s%c", NDSresult,separator);
                        if (NDSresult)
                                free(NDSresult);
                        }
                        break;


             case  '0': /*login script reading */
             case '1':
             case '2':{
                      err=-1;
                       if (NDSname[0]) {
                                switch (gUserFields[i]) {
                                case '0':
                                        err=NWCXGetObjectLoginScript(ctx,NDSname,buffer,&len,sizeof(buffer));
                                        break;
                                case '1':
                                        err=NWCXGetProfileLoginScript(ctx,NDSname,buffer,&len,sizeof(buffer));
                                        break;
                                case '2':
                                        //strcpy(NDSname,"E14.OUT.R"); // to test context climbing default ctx is PC
                                        err=NWCXGetContextLoginScript(ctx,NDSname,buffer,&len,sizeof(buffer));
                                        break;
                                }
                      }
                      if (err)
                          printf ("%s%c",ERR_MARK,separator);
                        //printf ( "%s%c",strnwerror(err),separator);
                      else
                        printf ("%s%c",buffer,separator);

                    }
                    break;

            default:
                     printf ("%s%c",UNK_MARK,separator);;

          }
        }
      if (ctx) NWDSFreeContext(ctx);
      return err;
 }



static NWCCODE processBindServers ( NWCONN_HANDLE conns[],int curEntries){
  int i;
 for (i=0; i<curEntries; i++) {
        NWCONN_HANDLE theConn=conns[i];
        if (theConn){
           processServer(theConn,"");
           NWCCCloseConn (theConn);
           conns[i]=NULL; /* don't see it again*/
        }
  }
  return 0;
}




static NWCCODE processTrees ( NWCONN_HANDLE conns[],int curEntries,int allConns){
        char treeName[MAX_TREE_NAME_CHARS+1];
/* group connections by tree name */
        int i,j;
        NWCCODE err=0;
        for (i=0; i<curEntries; i++) {
                NWCONN_HANDLE theConn=conns[i];
                treeName[0]=0;
                if (theConn && NWCXIsDSServer(theConn,treeName)){
                        for (j=i;j<curEntries;j++) {
                                if (conns[j] && NWCXIsSameTree(conns[j],treeName)) {
                                        if (allConns || i==j)
                                                processServer(conns[j],treeName); /*error or not */
                                        NWCCCloseConn (conns[j]);
                                        conns[j]=NULL; /* don't see it again*/
                                }
                        }
                }

        }
        return err;

 }

int
main(int argc, char *argv[])
{

	long err;
        int opt;
        char *serverName=NULL;
        char *treeName=NULL;
        int allConns=1;

        int uid=getuid();

        progname = argv[0];
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
        NWCallsInit(NULL,NULL);

        gUserFields=gDefaultFields;

        while ((opt = getopt(argc, argv, "h?aAgG1f:F:s:S:T:")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
                case 'g': if (uid==0)
                                 uid=-1;
                          else{
                             printf("failed:Only super-user can uses the -a flag\n");
                             exit(2);
                          }
                  break;
                case 'f':
                case 'F':gUserFields=optarg;
                  break;
                case 'a':
                case 'A':gUserFields="AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz012";
                  break;
                case 's':
                   if (optarg && strlen(optarg)>0)
                                gSeparator =optarg[0];
                  break;
                case 'S':
                      serverName=optarg;
                  break;
                case 'T':
                      treeName=optarg;
                  break;
                case '1':allConns=0;
                  break;


		default:
			usage();
		}
	}

	if (optind != argc) {
                printf ("failed:%d %d",optind,argc);
		usage();
	}
        if (treeName && serverName) {
          fprintf(stderr,"%s failed:-S option cannot be used with -T option\n",progname);
        }


        { NWCONN_HANDLE conns[125];
          int maxEntries=125;
          int curEntries=0;

          if (treeName)
             err=NWCXGetPermConnListByTreeName (conns , maxEntries, &curEntries,uid,treeName);
          else if (serverName)
             err=NWCXGetPermConnListByServerName (conns , maxEntries, &curEntries,uid,serverName);
          else err=NWCXGetPermConnList (conns , maxEntries, &curEntries,uid);

        if (err) {
            fprintf(stderr, "NWCXGetPermConnList failed: %s\n",
                                strnwerror(err));
            return err;
        }


        /*printf ("got %d \n",curEntries);*/
        if (curEntries) {

          processTrees(conns,curEntries,allConns);

          processBindServers(conns,curEntries);
        }else
            if (treeName) printf("failed:no ncp connections to tree %s.\n",treeName);
            else if (serverName) printf("failed:no ncp connections to server %s.\n",serverName);
            else  printf("failed:no ncp connections.\n");

        }
	return 0;
}
