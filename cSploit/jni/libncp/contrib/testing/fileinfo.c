/*
    fileinfo.c
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


    Dumps requested informations about file
    ncp_ns_obtain_entry_info API demo


    Revision history:

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
  
 */

#define MAKE_NCPLIB

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
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
	       "-r requestedInfo  Requested flags\n"
	       "-n namespace      Destination Namespace\n"
	       "-s search_attr    Search attributes\n"
	       "-p volume_path    Remote path (default is derived from path)\n"
	       "\n"));
}
			
static void printstruct2(u_int32_t rim, struct nw_info_struct2* info) {
	if (rim & RIM_SPACE_ALLOCATED) {
		printf("Space Allocated:       %08LX (%Ld)\n", info->SpaceAllocated,
							      info->SpaceAllocated);
	}
	if (rim & RIM_ATTRIBUTES) {
		printf("Attributes:            %08X\n", info->Attributes.Attributes);
		printf("Flags:                 %04X\n", info->Attributes.Flags);
	}
	if (rim & (RIM_DATA_SIZE | RIM_SIZE64)) {
		printf("Data Size:             %08LX (%Ld)\n", info->DataSize,
							     info->DataSize);
	}
	if (rim & RIM_TOTAL_SIZE) {
		printf("Total Size:            %08X (%d)\n", info->TotalSize.TotalAllocated,
							     info->TotalSize.TotalAllocated);
		printf("Datastreams:           %d\n", info->TotalSize.Datastreams);
	}
	if (rim & RIM_EXT_ATTR_INFO) {
		printf("ExtAttrInfo.DataSize:  %08X (%d)\n", info->ExtAttrInfo.DataSize,
							     info->ExtAttrInfo.DataSize);
		printf("ExtAttrInfo.Count:     %08X (%d)\n", info->ExtAttrInfo.Count,
							     info->ExtAttrInfo.Count);
		printf("ExtAttrInfo.KeySize:   %08X (%d)\n", info->ExtAttrInfo.KeySize,
							     info->ExtAttrInfo.KeySize);
	}
	if (rim & RIM_ARCHIVE) {
		printf("Archive.Date:          %04X\n", info->Archive.Date);
		printf("Archive.Time:          %04X\n", info->Archive.Time);
		printf("Archive.ID:            %08X\n", info->Archive.ID);
	}
	if (rim & RIM_CREATION) {
		printf("Creation.Date:         %04X\n", info->Creation.Date);
		printf("Creation.Time:         %04X\n", info->Creation.Time);
		printf("Creation.ID:           %08X\n", info->Creation.ID);
	}
	if (rim & RIM_MODIFY) {
		printf("Modify.Date:           %04X\n", info->Modify.Date);
		printf("Modify.Time:           %04X\n", info->Modify.Time);
		printf("Modify.ID:             %08X\n", info->Modify.ID);
		printf("LastAccess.Date:       %04X\n", info->LastAccess.Date);
	}
	if (rim & RIM_LAST_ACCESS_TIME) {
		printf("LastAccess.Time:       %04X\n", info->LastAccess.Time);
	}
	if (rim & RIM_OWNING_NAMESPACE) {
		printf("Owning Namespace:      %08X\n", info->OwningNamespace);
	}
	if (rim & RIM_DIRECTORY) {
		printf("Directory Base ID:     %08X\n", info->Directory.dirEntNum);
		printf("Directory DOS Base ID: %08X\n", info->Directory.DosDirNum);
		printf("Volume:                %08X\n", info->Directory.volNumber);
	}
	if (rim & RIM_RIGHTS) {
		printf("Inherited Rights Mask: %04X\n", info->Rights);
	}
	if (rim & RIM_REFERENCE_ID) {
		printf("Reference ID:          %04X\n", info->ReferenceID);
	}
	if (rim & RIM_NS_ATTRIBUTES) {
		printf("NS Attributes:         %08X\n", info->NSAttributes);
	}
	if (rim & RIM_UPDATE_TIME) {
		printf("Update Time:           %08X\n", info->UpdateTime);
	}
	if (rim & RIM_DOS_NAME) {
		printf("DOS Name:              ");
		fwrite(info->DOSName.Name, 1, info->DOSName.NameLength, stdout);
		printf("\n");
	}
	if (rim & RIM_FLUSH_TIME) {
		printf("Flush Time:            %08X\n", info->FlushTime);
	}
	if (rim & RIM_PARENT_BASE_ID) {
		printf("Parent Base ID:        %08X\n", info->ParentBaseID);
	}
	if (rim & RIM_MAC_FINDER_INFO) {
		int i;
		
		printf("MAC Finder Info:      ");
		for (i = 0; i < 32; i++)
			printf(" %02X", info->MacFinderInfo[i]);
		printf("\n");
	}
	if (rim & RIM_SIBLING_COUNT) {
		printf("Sibling Count:         %08X\n", info->SiblingCount);
	}
	if (rim & RIM_EFFECTIVE_RIGHTS) {
		printf("Effective Rights:      %08X\n", info->EffectiveRights);
	}
	if (rim & RIM_MAC_TIMES) {
		printf("Create Time:           %08X\n", info->MacTimes.CreateTime);
		printf("Backup Time:           %08X\n", info->MacTimes.BackupTime);
	}
	if (rim & RIM_UNKNOWN25) {
		printf("Unknown25:             %04X\n", info->Unknown25);
	}
	if (rim & RIM_NAME) {
		printf("Name:                  ");
		fwrite(info->Name.Name, 1, info->Name.NameLength, stdout);
		printf("\n");
	}
}								    

static int
ncp_add_handle_path2(u_int8_t* buf,
		     u_int8_t vol_num,
		     u_int32_t dir_base, int dir_style,
		     const unsigned char *encpath, int pathlen)
{
	buf[0] = vol_num;
	buf[1] = dir_base;
	buf[2] = dir_base >> 8;
	buf[3] = dir_base >> 16;
	buf[4] = dir_base >> 24;
	buf[5] = dir_style;	
	if (encpath) {
		memcpy(buf + 6, encpath, pathlen);
		return 6 + pathlen;
	} else {
		buf[6] = 0;
		return 7;
	}
}

static NWCCODE ncp_ns_obtain_entry_info_raw(struct ncp_conn *conn,
			 unsigned int source_ns,
			 unsigned int search_attribs,
			 int dir_style,
			 unsigned int vol, 
			 u_int32_t dirent, 
			 const unsigned char *encpath, size_t pathlen, 
			 unsigned int target_ns,
			 u_int32_t rim,
			 /* struct nw_info_struct2 */ void *target, size_t sizeoftarget)
{
	NWCCODE result;
	NW_FRAGMENT rp;
	u_int8_t rqb[1000];
	size_t rqlen;
	
	rp.fragAddress = target;
	rp.fragSize = sizeoftarget;
	
	rqb[0] = 6;
	rqb[1] = source_ns;
	rqb[2] = target_ns;
	rqb[3] = search_attribs;
	rqb[4] = search_attribs >> 8;
	rqb[5] = rim;
	rqb[6] = rim >> 8;
	rqb[7] = rim >> 16;
	rqb[8] = rim >> 24;	

	rqlen = ncp_add_handle_path2(rqb + 9, vol, dirent, dir_style, encpath, pathlen) + 9;
	
	result = NWRequestSimple(conn, 87, rqb, rqlen, &rp);
	return result;
}

						      
int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	char volume[1000];
	char volpath[1000];
	int opt;
	int len;
	unsigned char buffer[1000];
	u_int32_t rim = 0;
	u_int32_t destns = NW_NS_DOS;
	u_int32_t searchattr = 0x8006;
	char* s = NULL;
	int doraw = 0;
	
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
//	NWDSInitRequester();

	while ((opt = getopt(argc, argv, "h?r:n:s:p:R")) != EOF)
	{
		switch (opt)
		{
		case 'r':
			rim = strtoul(optarg, NULL, 0);
			break;
		case 'n':
			destns = strtoul(optarg, NULL, 0);
			break;
		case 's':
			searchattr = strtoul(optarg, NULL, 0);
			break;
		case 'p':
			s = optarg;
			break;
		case 'R':
			doraw = 1;
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

	dserr = NWParsePath(argv[optind++], NULL, &conn, volume, volpath);
	if (dserr) {
		fprintf(stderr, "NWParsePath failed: %s\n",
			strnwerror(dserr));
		return 123;
	}
	if (!conn) {
		fprintf(stderr, "Path is not remote\n");
		return 124;
	}
	strcat(volume, ":");
	strcat(volume, volpath);
	len = ncp_path_to_NW_format(s?s:volume, buffer, sizeof(buffer));
	if (len < 0) {
		fprintf(stderr, "Cannot convert path: %s\n",
			strerror(-len));
	} else {
		struct nw_info_struct2 iii;

		memset(&iii, 0xCC, sizeof(iii));
		
		if (doraw) {
			dserr = ncp_ns_obtain_entry_info_raw(conn,
				NW_NS_DOS, searchattr,
				0xFF, 0, 0, buffer, len, destns, rim, &iii, sizeof(iii));
			if (dserr) {
				fprintf(stderr, "Cannot obtain info: %s\n",
					strnwerror(dserr));
			} else {
				size_t i;
				unsigned char* p = (unsigned char*)&iii;
				
				for (i = 0; i < sizeof(iii); i++) {
					printf("%02X ", p[i]);
				}
				printf("\n");
			}
		} else {
			dserr = ncp_ns_obtain_entry_info(conn,
				NW_NS_DOS, searchattr,
				0xFF, 0, 0, buffer, len, destns, rim, &iii, sizeof(iii));
			if (dserr) {
				fprintf(stderr, "Cannot obtain info: %s\n",
					strnwerror(dserr));
			} else {
				printstruct2(rim, &iii);
			}
		}
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
