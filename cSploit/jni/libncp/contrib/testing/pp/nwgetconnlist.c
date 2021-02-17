/*
    test_nwgetconnlist.c - Returns details about all permanent connections
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

	0.00  2001			Patrick Pollet
		Initial revision.
		
	1.00  2001, March 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed NWCC_INFO_SERVER_VERSION questions... You have really
		no interest in peeking ncp_conn struct. It is internal
		to ncpfs library. Use NWCCGetConnInfo instead...
		
	1.01  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Do not use _REENTRANT by default, get it from top level
		makefile.
		Do not use sockaddr_ipx if CONFIG_NATIVE_IPX not set.


really simple but can be called from tcl/tk front ends to fill
a listbox with available volumes on one server
 */

/* define this to match default libncp config */
/* #define _REENTRANT */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

// tempo here
#include <private/ncp_fs.h>
#include <private/list.h>
#include <private/libncp-lock.h>
#include <private/libncp-atomic.h>

#include "../lib/ncplib_i.h"

#ifdef NCP_KERNEL_NCPFS_AVAILABLE
#include <mntent.h>
#endif
#include <pwd.h>

#ifdef N_PLAT_MSW4
#include "getopt.h"
#endif

#include "private/libintl.h"
#define _(X) X

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [-a] \n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options] \n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
               "-a              all connections (default is only mine(s)) \n"
	       "\n"));
}


#ifdef CONFIG_NATIVE_IPX

void
ipx_fprint_node(FILE * file, IPXNode node)
{
	fprintf(file, "%02X%02X%02X%02X%02X%02X",
		(unsigned char) node[0],
		(unsigned char) node[1],
		(unsigned char) node[2],
		(unsigned char) node[3],
		(unsigned char) node[4],
		(unsigned char) node[5]
	    );
}

void
ipx_fprint_network(FILE * file, IPXNet net)
{
	fprintf(file, "%08X", (u_int32_t)ntohl(net));
}

void
ipx_fprint_port(FILE * file, IPXPort port)
{
	fprintf(file, "%04X", ntohs(port));
}

void
ipx_print_node(IPXNode node)
{
	ipx_fprint_node(stdout, node);
}

void
ipx_print_network(IPXNet net)
{
	ipx_fprint_network(stdout, net);
}

void
ipx_print_port(IPXPort port)
{
	ipx_fprint_port(stdout, port);
}

void
ipx_fprint_saddr(FILE * file, struct sockaddr_ipx *sipx)
{
	ipx_fprint_network(file, sipx->sipx_network);
	fprintf(file, ":");
	ipx_fprint_node(file, sipx->sipx_node);
	fprintf(file, ":");
	ipx_fprint_port(file, sipx->sipx_port);
}

void
ipx_print_saddr(struct sockaddr_ipx *sipx)
{
	ipx_fprint_saddr(stdout, sipx);
}

#endif

static void
dump_hex(const char *_msg, const unsigned char *_buf, size_t _len)
{
	static char sym[] = "0123456789ABCDEF";
	printf( "len = %d:msg->%s", _len, _msg);
	for (; _len > 0; _len--, _buf++) {
		putc(sym[(*_buf) >> 4],stdout);
		putc(sym[(*_buf) & 0x0F],stdout);
	}
	putc('\n', stdout);

}

/***
struct ncp_fs_info_v2 {
        int version;
        unsigned long mounted_uid;
        unsigned int connection;
        unsigned int buffer_size;

        unsigned int volume_number;
        u_int32_t directory_id;

        u_int32_t dummy1;
        u_int32_t dummy2;
        u_int32_t dummy3;
};

*****/


static NWCCODE NWCGetMountedConnList (NWCONN_HANDLE* conns , int maxEntries, int* curEntries, uid_t uid){

// returns a list of permanent connexions

#ifdef NCP_KERNEL_NCPFS_AVAILABLE
        FILE* mtab;
        struct ncp_conn* conn;
        struct mntent* mnt_ent;

        char aux[256];
        char aux2[256];
        char* user;


        *curEntries=0;

        if ((mtab = fopen(MOUNTED, "r")) == NULL) {
                return -1;
        }
        while ((*curEntries <maxEntries)&&(mnt_ent = getmntent(mtab))) {
                printf("%s %s %s\n ",mnt_ent->mnt_type,mnt_ent->mnt_fsname,mnt_ent->mnt_dir);
                if (strcmp(mnt_ent->mnt_type, "ncpfs") != 0) {
                        continue;
                }
                //printf("%s %s %s\n ",mnt_ent->mnt_type,mnt_ent->mnt_fsname,mnt_ent->mnt_dir);
                if (strlen(mnt_ent->mnt_fsname) >= sizeof(aux)) {
                        continue;
                }
                strcpy(aux, mnt_ent->mnt_fsname);
                user = strchr(aux, '/');
                if (!user) {
                        continue;
                }
                *user++ = '\0';

                strcpy(aux2, mnt_ent->mnt_dir);
                printf("-->%s %s %s\n ",aux,user,aux2);

                if (ncp_open_mount(aux2,&conn)) {
                   printf("failed%s\n","");
                       continue;
                }

             if ((uid == (uid_t)-1) || (conn->i.mounted_uid==uid)) {
                    //conn->serverInfo.serverName=strdup(aux);
                    //conn->user=strdup(user);

                    *conns++=conn;
                    (*curEntries)++;
                    printf("OK %d\n",*curEntries);
              }

        }


        fclose(mtab);
        return 0;
#else
        return -1;
#endif  /* NCP_KERNEL_NCPFS_AVAILABLE */
}



static void printConn (NWCONN_HANDLE conn, int stage) {
        printf("%s\n",conn->mount_point);
        printf ("------ info from kernel--------------------------------\n");
        printf ("infoV2.version %d\n",conn->i.version);
	printf ("infoV2.mounted_uid %ld\n",conn->i.mounted_uid);
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
	printf ("nds_ring %p,%p\n",conn->nds_ring.prev, conn->nds_ring.next);
        printf ("user %s\n",conn->user);
	printf ("user_id_is_valid %d\n",conn->user_id_valid);
	printf ("user_id %x\n",conn->user_id);

	printf ("mount_fid %d\n",conn->mount_fid);
        printf ("mount_point %s\n",conn->mount_point);
	printf ("ncpt_atomic_t store_count %d\n",ncpt_atomic_read(&conn->store_count));
        printf ("state %d\n",conn->state);


        dump_hex ("addr",(void *)&(conn-> addr),sizeof(conn-> addr));

        dump_hex ("serverAddresses",(void *)&(conn-> serverAddresses),256);

        printf ("connstate %x\n",conn->connState);
        printf ("sign_wanted %x\n",conn->sign_wanted);
	printf ("sign_active %x\n",conn->sign_active);

        printf ("serverInfo.valid %x\n",conn->serverInfo.valid);
        printf ("serverInfo.serverName %s\n",conn->serverInfo.serverName);
        printf ("serverInfo.version.major %x\n",conn->serverInfo.version.major);
        printf ("serverInfo.version.minor %x\n",conn->serverInfo.version.minor);
        printf ("serverInfo.version.revision %x\n",conn->serverInfo.version.revision);

        printf ("NET_ADDRESS_TYPE nt %x\n",conn->nt);;



 }

static void getAsMuchAsYouCan ( NWCONN_HANDLE conn) {
        char buffer[8192];
        int len=sizeof(buffer);

        NWCCTranAddr aux;
        aux.buffer=malloc (256);
        aux.len=256;
         printf ("------ getting back as much as I can :\n");
         printf("NWCC_INFO_TREE_NAME %s\n",  strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_TREE_NAME, len, buffer)));
         printf("NWCC_INFO_CONN_NUMBER %s\n", strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_CONN_NUMBER,  sizeof(NWCONN_NUM), buffer)));
         printf("NWCC_INFO_USER_ID %s\n", strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_USER_ID,  sizeof(NWObjectID), buffer)));
         printf("NWCC_INFO_SERVER_VERSION %s\n", strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_SERVER_VERSION,  sizeof(NWCCVersion), buffer)));

/// OK within NWCCGetConnInfo  but not here: stays NULL 0 0 0 I am getting CRAZYYYYYY
#if 1
  dump_hex("back in main VERSION buffer contains=",buffer,12);
  printf (" but within conn  it is %d %d %d %d %s\n",conn->serverInfo.valid ,conn->serverInfo.version.major,conn->serverInfo.version.minor,conn->serverInfo.version.revision,                   conn->serverInfo.serverName);
#endif

/// OK within NWCCGetConnInfo  but not back here ????
         printf("NWCC_INFO_SERVER_NAME %s\n", strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_SERVER_NAME, len, buffer)));
#if 1
  dump_hex("back in main S NAME buffer contains =",buffer,15);
  printf ("but within conn it is still %d %d %d %d %s\n",conn->serverInfo.valid ,conn->serverInfo.version.major,conn->serverInfo.version.minor,conn->serverInfo.version.revision,
                   conn->serverInfo.serverName);
#endif


         printf("NWCC_INFO_MAX_PACKET_SIZE %s\n", strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_MAX_PACKET_SIZE, sizeof(unsigned int), buffer)));
         printf("NWCC_INFO_TRAN_ADDR %s\n", strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_TRAN_ADDR, sizeof(aux), &aux)));
         printf("NWCC_INFO_USEr_NAME %s\n",strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_USER_NAME, len, buffer)));
         printf("NWCC_INFO_ROOT_ENTRY %s\n",strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_ROOT_ENTRY, len, buffer)));
         printf("NWCC_INFO_MOUNT_UID %s\n",strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_MOUNT_UID, sizeof (uid_t), buffer)));
         printf("NWCC_INFO_SECURITY %s\n",strnwerror(NWCCGetConnInfo(conn,NWCC_INFO_SECURITY, sizeof (int), buffer)));


         dump_hex("tranaddr:",(char *)&aux,sizeof(aux));
         dump_hex("tranaddr II: ",aux.buffer,12);

        free (aux.buffer);

 }

int
main(int argc, char *argv[])
{
	//struct ncp_conn *conn;
	long err;
        int opt;
        //int allConns=0;
        int uid=getuid();

        progname = argv[0];
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);


       	while ((opt = getopt(argc, argv, "h?a")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			exit (1);
                case 'a': uid=-1; break;

		default:
			usage();
			exit(1);
		}
	}

	if (optind != argc) {
                printf ("%d %d",optind,argc);
		usage();
		exit(1);
	}
        //static NWCCODE NWCGetMountedConnList (NWCONN_HANDLE* conns , int maxEntries, int* curEntries, int uid){

        { NWCONN_HANDLE conns[125];
          int maxEntries=125;
          int curEntries,i;

        if ((err=NWCGetMountedConnList (conns , maxEntries, &curEntries,uid))){
            fprintf(stderr, "NWCGetMountedConnList: %s\n",
                                strnwerror(err));
        }




        printf ("got %d \n",curEntries);
        for (i=0; i<curEntries;i++) {
            NWCONN_HANDLE conn;
            conn =conns[i];
            printConn (conn,1);
            getAsMuchAsYouCan(conn);
            printConn (conn,2);
        }


        }
	return 0;
}
