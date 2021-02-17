/*
    ncpreadprops - print a list of NDS properties of current user
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

        1.00 2001   Nov 02             Patrick Pollet <patrick.pollet@insa-lyon.fr>
                the program is an extension of ncpreadprop to allow:
		   a list of NDS properties
                             a separator between them
         the first idea was to put in the attribute buffer the list of requested attributes:
            IT WORKS BUT NDS does not return attributes in the same order
example:
 ./ncpreadprops -o ppollet -A "Postal Address,Full Name,LINUX:uid,Home Directory,CN" -c PC
-->
Ppollet:0 CN=FCPC2_APPS home\ppollet:1 2 3 4 5 6:Patrick Pollet:9999

	1.01 2001   Nov 03             Patrick Pollet <patrick.pollet@insa-lyon.fr>
		we call ReadAttributeValue for every attribute of the list !!!

	1.02  2001 Dec 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		const <-> non const pointers cleanup

	1.03  2002 June 6		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		emission of separator between multiple values

	1.04  2002 Sept 16		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		emission of separator between multiple values when attributes are not read at once

 */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/errno.h>
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <ncp/nwclient.h>

#include <pwd.h>

#include "getopt.h"

#include "private/libintl.h"
#define _(X) X

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] \n"), progname);
        exit(1);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options] \n"), progname);
	printf(_("\n"
	       "-h                  Print this help text\n"
               "-o  object_name   Mandatory: Object name  \n"
               "-A  attribute_liste  Mandatory: One NDS attribute or a comma separated list\n"
               "-T  treename       \n"
               "-c  context        Default= [root]\n"
               "-v  value          Context DCK_FLAGS default=0 \n"
               "-f  character      separator between attributes (default= :)\n"
	  "-m  character       separator between values of multi-valued attributes (default = space) \n"
	       "\n"));
        exit(1);
}




static int doPrintOut (NWDSContextHandle ctx, const void * name, const enum SYNTAX synt, char sep ) {
/* do not emit CR here */
/* sep is the caracter used to separate multiples values such as postal address or home directory*/
  (void) ctx;
  switch (synt) {
	case SYN_DIST_NAME:
	case SYN_CI_STRING:
	case SYN_CE_STRING:
        case SYN_PR_STRING:
	case SYN_NU_STRING:
	case SYN_TEL_NUMBER:
	case SYN_CLASS_NAME:
		printf("%s", (const char*)name);
	        break;
	case SYN_OCTET_STRING:{
	        const Octet_String_T* os = (const Octet_String_T*)name;
		size_t i;
		printf("%u%c", os->length,sep);
		for (i = 0; i < os->length; i++)
			printf("%02X%c", os->data[i],sep);
		}
		break;
	case SYN_COUNTER:
	case SYN_INTEGER:
	case SYN_INTERVAL:
		printf("%u", *((const Integer_T*)name));
        	break;
	case SYN_BOOLEAN:{
	        const Boolean_T* b = (const Boolean_T*)name;
		printf("%s", (*b==1)?"true":(*b)?"unknown":"false");
		}
		break;
	case SYN_TIMESTAMP:{
		const TimeStamp_T* stamp = (const TimeStamp_T*)name;
                printf("%u%c%u%c%u",stamp->wholeSeconds,sep, stamp->replicaNum,sep,stamp->eventID);
		}
		break;
	case SYN_NET_ADDRESS:{
		const Net_Address_T* na = (const Net_Address_T*)name;
	        size_t z;
		printf("%u%c%u%c", na->addressType,sep,na->addressLength,sep);
		for (z = 0; z < na->addressLength; z++)
			printf("%02X%c", na->address[z],sep);
		}
		break;
	case SYN_OBJECT_ACL:{
	        const Object_ACL_T* oacl = (const Object_ACL_T*)name;
		printf("%s%c%s%c%08x", oacl->protectedAttrName,sep,oacl->subjectName,sep,oacl->privileges);
		}
		break;
	case SYN_PATH:{
	        const Path_T* p = (const Path_T*)name;
		printf("%u%c%s%c%s", p->nameSpaceType,sep,p->volumeName,sep, p->path);
		}
		break;
	case SYN_TIME:{
		printf("%s", ctime((const Time_T*)name));
                }
		break;
	case SYN_TYPED_NAME:{
		const Typed_Name_T* tn = (const Typed_Name_T*)name;
		printf("%u%c%u%c%s", tn->interval,sep, tn->level,sep, tn->objectName);
		}
		break;
	case SYN_CI_LIST:{
	        const CI_List_T* cl = (const CI_List_T*)name;
		for (; cl; cl = cl->next)
			printf("%s", cl->s);
		}
		break;
	case SYN_OCTET_LIST:{
	        const Octet_List_T* ol = (const Octet_List_T*)name;
		for (; ol; ol = ol->next) {
		        size_t i;
		        printf("%u%c", ol->length,sep);
			for (i = 0; i < ol->length; i++)
				printf("%02X%c", ol->data[i],sep);
		}
		}
		break;
	case SYN_BACK_LINK:{
	        const Back_Link_T* bl = (const Back_Link_T*)name;
		printf("%08X%c%s", bl->remoteID, sep,bl->objectName);
	        }
		break;
	case SYN_FAX_NUMBER:{
	        const Fax_Number_T* fn = (const Fax_Number_T*)name;
		printf("%s%c%u", fn->telephoneNumber, sep,fn->parameters.numOfBits);
		}
		break;
	case SYN_EMAIL_ADDRESS:{
		const EMail_Address_T* ea = (const EMail_Address_T*)name;
		printf("%u%c%s", ea->type, sep,ea->address);
		}
		break;
	case SYN_PO_ADDRESS:{
		const NWDSChar** pa = (const NWDSChar**)name;
		printf("%s%c%s%c%s%c%s%c%s%c%s",pa[0],sep,pa[1],sep,pa[2],sep,pa[3],sep,pa[4],sep,pa[5]);
		}
		break;
	case SYN_HOLD:{
	        const Hold_T* h = (const Hold_T*)name;
		printf("%u%c%s", h->amount,sep,h->objectName);
		}
		break;
	default:
	        printf(" *** ");  /* don't mess up display */
		return 1;
	}

        return 0;


}

static NWCCODE readAttributeValue( NWDSContextHandle ctx, char * objName,char *attrName, char sepvalue) {

        Buf_T* inbuf=NULL;
       	Buf_T* outbuf=NULL;
        NWDSCCODE dserr;
	nuint32 iterHandle;

	size_t size = DEFAULT_MESSAGE_LEN;

        iterHandle = NO_MORE_ITERATIONS; /*test at bailout point */

        dserr = NWDSAllocBuf(size, &inbuf);
	if (dserr) {
		fprintf(stderr, "NWDSAllocBuf() failed with %s\n", strnwerror(dserr));
		goto bailout;
	}
	dserr = NWDSInitBuf(ctx, DSV_READ, inbuf);
	if (dserr) {
		fprintf(stderr, "NWDSInitBuf() failed with %s\n", strnwerror(dserr));
		goto bailout;
	}

                dserr = NWDSPutAttrName(ctx, inbuf, attrName);
                if (dserr) {
		fprintf(stderr, "NWDSPutAttrName(%s) failed with %s\n", attrName, strnwerror(dserr));
		goto bailout;
	}



	dserr = NWDSAllocBuf(size, &outbuf);
	if (dserr) {
		fprintf(stderr, "NWDSAllocBuf() failed with %s\n", strnwerror(dserr));
		goto bailout;
	}
	do {
		NWObjectCount attrs;

		dserr = NWDSRead(ctx, objName, DS_ATTRIBUTE_VALUES, 0, inbuf, &iterHandle, outbuf);
		if (dserr) {
			if (dserr == ERR_NO_SUCH_ATTRIBUTE) {
                               	/*fprintf(stderr, "NWDSRead() failed with %s\n", strnwerror(dserr));*/
				dserr = 0;
                        }
			else
				fprintf(stderr, "NWDSRead() failed with %s\n", strnwerror(dserr));
			goto bailout;
		}
		dserr = NWDSGetAttrCount(ctx, outbuf, &attrs);
		if (dserr) {
			fprintf(stderr, "NWDSGetAttrCount() failed with %s\n", strnwerror(dserr));
			goto bailout;
		}
                /*fprintf(stderr, "%d attributes found\n", attrs);*/
		while (attrs--) {
			NWDSChar attrname[MAX_SCHEMA_NAME_CHARS+1];
			enum SYNTAX synt;
			NWObjectCount vals;

			dserr = NWDSGetAttrName(ctx, outbuf, attrname, &vals, &synt);
			if (dserr) {
				fprintf(stderr, "NWDSGetAttrName() failed with %s\n", strnwerror(dserr));
				goto bailout;
			}
			/*fprintf(stderr, "%d values found\n", vals);*/
			while (vals--) {
				size_t sz;
				void* val;

				dserr = NWDSComputeAttrValSize(ctx, outbuf, synt, &sz);
				if (dserr) {
					fprintf(stderr, "NWDSComputeAttrValSize() failed with %s\n", strnwerror(dserr));
					goto bailout;
				}
				val = malloc(sz);
				if (!val) {
					fprintf(stderr, "malloc() failed with %s\n", strnwerror(ENOMEM));
					goto bailout;
				}
				dserr = NWDSGetAttrVal(ctx,outbuf, synt, val);
				if (dserr) {
					free(val);
					fprintf(stderr, "NWDSGetAttrVal() failed with %s\n", strnwerror(dserr));
					goto bailout;
				}
				dserr=doPrintOut (ctx,val,synt,sepvalue);

				free(val);
				if (dserr) {
                                        fprintf(stderr, "doPrintOut failed with %s\n", strnwerror(dserr));
					goto bailout;
				}
				if ((vals) || (iterHandle != NO_MORE_ITERATIONS))
					printf("%c", sepvalue); /* emit separator between values */
			}

		}
	} while (iterHandle != NO_MORE_ITERATIONS);
bailout:;

	if (iterHandle != NO_MORE_ITERATIONS) {
        /*  let's keep the final dserr as the real one */
		NWDSCCODE dserr2 = NWDSCloseIteration(ctx, DSV_READ, iterHandle);
		if (dserr2) {
			fprintf(stderr, "NWDSCloseIteration() failed with %s\n", strnwerror(dserr2));
		}
	}
	if (inbuf) NWDSFreeBuf(inbuf);
	if (outbuf) NWDSFreeBuf(outbuf);
	return dserr;
}

static NWCCODE readAttributesValues( NWDSContextHandle ctx, char * objName,char *attrName, char sepvalue, char sepattr) {
/* scan the list of attributes*/
	   char  attrBuf[MAX_DN_CHARS+1];
	   const char * attrStart;
	   const char * attrEnd;

        attrStart=attrName;
        do {
                if (attrStart) {
                        attrEnd=strchr(attrStart,',');
                        if (attrEnd) {
                               memcpy(attrBuf,attrStart,attrEnd - attrStart);
                               attrBuf[attrEnd-attrStart]=0;
                               attrEnd++;
                        }  else
                                strcpy(attrBuf,attrStart);
                        attrStart=attrEnd;
                }
                readAttributeValue(ctx, objName,attrBuf, sepvalue);
                if (attrStart)   printf("%c",sepattr);  /* emit separator exepct after the last attribute*/
        } while (attrStart);
          printf("\n"); /* change line here */
        return 0;
}

int
main(int argc, char *argv[])
{
	long err;
	int opt;
	char * objectName=NULL;
	char * attributeName=NULL;
	const char * contextName="[Root]";
	char * server=NULL;
	char sepvalue=' ';  /*default separator between multiple values is space */
	char sepattr=':';     /* default separator between attributes is :*/



        NWDSContextHandle ctx=NULL;
	NWCONN_HANDLE conn=NULL;
        NWDSCCODE dserr,dserr2;
        u_int32_t ctxflag = 0;

        char     treeName    [MAX_TREE_NAME_CHARS +1]="";

        progname = argv[0];

        NWCallsInit(NULL, NULL);
#ifndef N_PLAT_MSW4
	NWDSInitRequester();
#else
        setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif


       	while ((opt = getopt(argc, argv, "h?o:A:T:c:S:v:f:m:")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
                case 'A':
                        attributeName=optarg;
                  break;
                case 'T':
                       if (strlen(optarg) <sizeof(treeName))
                                strcpy(treeName,optarg);
                       else {
                                fprintf(stderr,"failed:parameter treename is too long.\n");
                                exit(3);
                       }
                  break;
                case 'o':
                        objectName=optarg;
                  break;
                case 'c':
                        contextName=optarg;
                  break;
               	case 'S':
	        	server = optarg;
                  break;
  	case 'f':
	        	sepattr = optarg[0];
                  break;
	      case 'm':
	        	sepvalue = optarg[0];
                  break;
                case 'v':
                        ctxflag = strtoul(optarg, NULL, 0);
		break;

		default:
			usage();
		}
	}

	if (optind != argc) {
                printf ("failed %d %d",optind,argc);
		usage();
		exit(1);
	}

        if (!attributeName) {
            fprintf(stderr,"failed:you must specify an attribut name.\n");
            exit(2);
        }
        if (!objectName) {
            fprintf(stderr,"failed:you must specify a NDS object name.\n");
            exit(2);
        }
        if (server && treeName[0]) {
          	fprintf(stderr, "failed:cannot have both -T tree and -S server options\n");
		exit(101);
        }



        { /*tested code goes here */
          u_int32_t init_ctxflag ;

        dserr = NWDSCreateContextHandle(&ctx);
	if (dserr) {
		fprintf(stderr, "NWDSCreateContextHandle failed with %s\n", strnwerror(dserr));
		goto bailout;
	}





        dserr = NWDSGetContext(ctx, DCK_FLAGS, &init_ctxflag);
	if (dserr) {
		fprintf(stderr, "NWDSGetContext(DCK_FLAGS) failed: %s\n",
			strnwerror(dserr));
		goto bailout;
	}
       init_ctxflag |= DCV_XLATE_STRINGS+ctxflag;

	dserr = NWDSSetContext(ctx, DCK_FLAGS, &init_ctxflag);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_FLAGS) failed: %s\n",
			strnwerror(dserr));
		goto bailout;
	}
	dserr = NWDSSetContext(ctx, DCK_NAME_CONTEXT, contextName);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_NAME_CONTEXT) failed: %s\n",
			strnwerror(dserr));
        	goto bailout;
	}

#ifndef N_PLAT_MSW4
	{
		static const u_int32_t add[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		dserr = NWDSSetTransport(ctx, 16, add);
		if (dserr) {
			fprintf(stderr, "NWDSSetTransport failed: %s\n",
				strnwerror(dserr));
			goto bailout;
		}
	}
#endif
#ifdef N_PLAT_MSW4
	dserr = NWDSOpenConnToNDSServer(ctx, server, &conn);
#else
	if (server) {
          if  (server[0] == '/') {
		dserr = ncp_open_mount(server, &conn);
		if (dserr) {
			fprintf(stderr, "ncp_open_mount failed with %s\n",
				strnwerror(dserr));
			goto bailout;
		}
	  } else {
		struct ncp_conn_spec connsp;
		memset(&connsp, 0, sizeof(connsp));
		strcpy(connsp.server, server);
		conn = ncp_open(&connsp, &err);
            }
        } else {
                if (!treeName[0]) {
                         NWCXGetPreferredDSTree(treeName,sizeof(treeName));

                }
                if (!treeName[0]) {
                        fprintf(stderr,"failed: You must specify a server or a tree\n");
                        goto bailout;
                }


                  if ((dserr=NWCXAttachToTreeByName (&conn,treeName))) {
                         fprintf(stderr,"failed:Cannot attach to tree %s.err:%s\n",
                                      treeName,strnwerror(dserr));
                        goto bailout;

                }


        }
        if (!conn) {
		fprintf(stderr, "ncp_open failed with %s\n",strnwerror(err));
		goto bailout;
	}
	dserr = NWDSAddConnection(ctx, conn);
#endif
	if (dserr) {
		fprintf(stderr, "failed:Cannot bind connection to context: %s\n",
			strnwerror(dserr));
                goto bailout;
	}

/****WILL  fail if not using a permanent/shared connection
       so we prefer to check that parameter -o has been specified (see above)
       if (!objectName) {
            objectName=malloc (256);
            dserr= NWDSWhoAmI(ctx,objectName);
            if (dserr) {
	                fprintf(stderr, "NWDSWhoAmi failed with error: %s\n",
               			strnwerror(dserr));
               goto bailout;
           }
        }
*********************************************************************/
        /*fprintf(stderr, "fetching  %s %s \n", objectName,attributeName);*/
        dserr=readAttributesValues(ctx,objectName,attributeName,sepvalue,sepattr);

bailout:
       	if (conn)
            NWCCCloseConn(conn);
	if (ctx)
           if ((dserr2=NWDSFreeContext(ctx))){
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr2));
		return dserr2;
	   }

        }

	return dserr;

}
