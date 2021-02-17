/*
    dirlist.c - List the contents of a directory
    Copyright (C) 1999  Petr Vandrovec

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

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
  
 */

#define MAKE_NCPLIB
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include "ncplib_i.h"

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <errno.h>
#include <string.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
	         "usage: %s [options] path\n"), progname);
	printf(_("\n"
	       "-h                Print this help text\n"
	       "-m mode           0=find entry (NCP87/03), 1=find entries (NCP87/20), 2=find names only (NCP87/20)\n"
	       "-n namespace      0=DOS, 1=MAC, 2=NFS, 3=FTAM, 4=OS2\n"
	       "-s search_attr    Search attributes\n"
	       "\n"));
}
			
NWCCODE
ncp_ns_search_entry_set(NWCONN_HANDLE conn,
		unsigned int search_attributes,
		const char* pattern,
		unsigned int datastream,
		u_int32_t rim,
		int* more,
		size_t *itemcount,
		struct ncp_search_seq* seq,
		void* buffer, size_t* size) {
	size_t slen;
	NWCCODE result;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 0x14);
	ncp_add_byte(conn, seq->name_space);
	ncp_add_byte(conn, datastream);
	ncp_add_word_lh(conn, search_attributes);
	ncp_add_dword_lh(conn, rim);
	ncp_add_word_lh(conn, *itemcount);
	ncp_add_mem(conn, &seq->s, 9);
	slen = pattern ? strlen(pattern) : 0;
	ncp_add_byte(conn, slen);
	if (slen)
		ncp_add_mem(conn, pattern, slen);
	result = ncp_request(conn, 0x57);
	if (result) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 9 + 1 + 2) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	memcpy(&seq->s, ncp_reply_data(conn, 0), 9);
	*more = ncp_reply_byte(conn, 9);
	*itemcount = ncp_reply_word_lh(conn, 10);
	slen = conn->ncp_reply_size - 9 - 1 - 2;
	if (slen > *size) {
		slen = *size;
		result = NWE_BUFFER_OVERFLOW;
	} else
		*size = slen;
	memcpy(buffer, ncp_reply_data(conn, 12), slen);
	ncp_unlock_conn(conn);
	return result;
}
		

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	int opt;
	struct NWCCRootEntry nwccre;
	unsigned int searchattr = SA_ALL;
	u_int32_t destns = NW_NS_DOS;
	int searchmode = 0;
	const char* sstr = "\xFF*";
		
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
//	NWDSInitRequester();

	while ((opt = getopt(argc, argv, "h?n:s:m:")) != EOF)
	{
		switch (opt)
		{
		case 'n':
			destns = strtoul(optarg, NULL, 0);
			break;
		case 's':
			searchattr = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			searchmode = strtoul(optarg, NULL, 0);
			break;
		case 'h':
		case '?':
			help();
			goto finished;
		default:
			usage();
			goto finished;
		}
	}

	/* NFS and MAC do not handle wildcards :-( */
	if ((destns == NW_NS_NFS) || (destns == NW_NS_MAC))
		sstr = NULL;
		
	dserr = ncp_open_mount(argv[optind++], &conn);
	if (dserr) {
		fprintf(stderr, "ncp_open_mount failed: %s\n",
			strnwerror(dserr));
		return 123;
	}
	dserr = NWCCGetConnInfo(conn, NWCC_INFO_ROOT_ENTRY, sizeof(nwccre), &nwccre);
	if (dserr) {
		fprintf(stderr, "Cannot get entry info: %s\n",
			strerror(dserr));
	} else {
		struct ncp_search_seq seq;
		struct nw_info_struct nw;

		dserr = ncp_obtain_file_or_subdir_info(conn, NW_NS_DOS, destns, SA_ALL, RIM_DIRECTORY,
			nwccre.volume, nwccre.dirEnt, NULL, &nw);
		if (dserr)
			fprintf(stderr, "Cannot convert DOS entry to %u-NS entry: %s (%08X)\n", destns, strnwerror(dserr), dserr);
		else {
			dserr = ncp_initialize_search(conn, &nw, destns, &seq);
			if (dserr)
				fprintf(stderr, "Initialize search failed: %s (%08X)\n", strnwerror(dserr), dserr);
			else {
				switch (searchmode) {
				case 1:
					while (1) {
						int more;
						size_t itemcnt = 1000;
						unsigned char buffer[16384];
						size_t sbuffer = sizeof(buffer);
						void* bptr;
						
						dserr = ncp_ns_search_entry_set(conn,
							searchattr, sstr, 0, RIM_ALL, &more, &itemcnt, &seq,
							buffer, &sbuffer);
						if (dserr)
							break;
						printf("Search sequence: %02X:%08X:%08X, items: %u, more: %02X\n",
							seq.s.volNumber, seq.s.dirBase, seq.s.sequence,
							itemcnt, more);
						printf("Buffer size: %d\n", sbuffer);
						bptr = buffer;
						while (itemcnt-- > 0) {
							struct nw_info_struct* nwp = bptr;
							char tmpname[1000];
							
							memcpy(tmpname, nwp->entryName, nwp->nameLen);
							tmpname[nwp->nameLen] = 0;
							printf("  Entry %08X: %s\n", nwp->dirEntNum, tmpname);
							/* fixed size + variable size */
							((char*)bptr) += sizeof(*nwp) - 256 + nwp->nameLen;
						}
						
					}
					fprintf(stderr, "Search finished: %s (%08X)\n", strnwerror(dserr), dserr);
					break;
				case 2:
					while (1) {
						int more;
						size_t itemcnt = 1000;
						unsigned char buffer[16384];
						size_t sbuffer = sizeof(buffer);
						void* bptr;
						
						dserr = ncp_ns_search_entry_set(conn,
							searchattr, sstr, 0, 
							RIM_NAME | RIM_COMPRESSED_INFO, &more, &itemcnt, &seq,
							buffer, &sbuffer);
						if (dserr)
							break;
						printf("Search sequence: %02X:%08X:%08X, items: %u, more: %02X\n",
							seq.s.volNumber, seq.s.dirBase, seq.s.sequence,
							itemcnt, more);
						printf("Buffer size: %d\n", sbuffer);
						bptr = buffer;
						while (itemcnt-- > 0) {
							unsigned char* nwp = bptr;
							char tmpname[1000];
							
							memcpy(tmpname, nwp+1, nwp[0]);
							tmpname[nwp[0]] = 0;
							printf("  Entry: %s\n", tmpname);
							/* fixed size + variable size */
							((char*)bptr) += nwp[0] + 1;
						}
						
					}
					fprintf(stderr, "Search finished: %s (%08X)\n", strnwerror(dserr), dserr);
					break;
				default:
					while ((dserr = ncp_search_for_file_or_subdir2(conn,
						searchattr, RIM_ALL, &seq, &nw)) == 0) {
						nw.entryName[nw.nameLen] = 0;
						printf("Search sequence: %02X:%08X:%08X\n",
							seq.s.volNumber, seq.s.dirBase, seq.s.sequence);
						printf("  Entry %08X: %s\n", nw.dirEntNum, nw.entryName);
					}
					fprintf(stderr, "Search finished: %s (%08X)\n", strnwerror(dserr), dserr);
					break;
				}
			}
		}
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
