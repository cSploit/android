/*
    mkfile.c
    Copyright (C) 2000  Petr Vandrovec

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

	1.00  2000, March 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision, base copied from fileinfo.c.

 */

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
	       "-o offset         Offset for read/write\n"
	       "-l                Retrieve file length\n"
	       "-w                Write sample data to file\n"
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
	if (rim & RIM_DATA_SIZE) {
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
	if (rim & RIM_NAME) {
		printf("Name:                  ");
		fwrite(info->Name.Name, 1, info->Name.NameLength, stdout);
		printf("\n");
	}
}								    
						      
int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	char volume[1000];
	char volpath[1000];
	int opt;
	u_int32_t rim = 0;
	u_int32_t destns = NW_NS_DOS;
	u_int32_t searchattr = 0x8006;
	ncp_off64_t offs = 0x2D0000000000ULL;
	char* s = NULL;
	int dowrite = 0;
	int dolen = 0;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
//	NWDSInitRequester();

	while ((opt = getopt(argc, argv, "h?r:n:s:p:wlo:")) != EOF)
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
		case 'w':
			dowrite = 1;
			break;
		case 'l':
			dolen = 1;
			break;
		case 'o':
			offs = strtoull(optarg, NULL, 0);
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
	if (!s) {
		fprintf(stderr, "-p is mandatory\n");
		return 122;
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
	strcpy(volume + 2, s);
	volume[0] = 1;
	volume[1] = strlen(s);
	{
		struct nw_info_struct2 iii;
		u_int8_t fh[10];
		
		memset(&iii, 0xCC, sizeof(iii));

		dserr =	ncp_ns_open_create_entry(conn, destns, searchattr, NCP_DIRSTYLE_DIRBASE, 
			0, 0, volume, strlen(s) + 2, -1, dowrite ? OC_MODE_CREATE | OC_MODE_TRUNCATE | OC_MODE_OPEN | OC_MODE_OPEN_64BIT_ACCESS : OC_MODE_OPEN | OC_MODE_OPEN_64BIT_ACCESS, 
			0, AR_READ_ONLY|AR_WRITE_ONLY, RIM_ALL, 
			&iii, sizeof(iii), NULL, NULL, fh);
		if (dserr) {
			fprintf(stderr, "Cannot obtain info: %s\n",
				strnwerror(dserr));
		} else {
			nuint8  databuffer[8192];
			int length = 4000;
			unsigned int i;
			
			printstruct2(RIM_ALL, &iii);
			
			printf("File handle:");
			for (i = 0; i < 6; i++) {
				printf(" %02X", fh[i]);
			}
			printf("\n");
			
			if (dowrite) {
				size_t lnw;

				memset(databuffer, length, length);
			
				dserr = ncp_write64(conn, fh, offs, length, databuffer, &lnw);
				printf("64-bit write request done with %s\n", strnwerror(dserr));
				if (dserr == 0) {
					printf("%u bytes written\n", lnw);
				}
			} else if (dolen) {
				u_int64_t ln;

				dserr = ncp_get_file_size(conn, fh, &ln);
				printf("64-bit file length request done with %s\n", strnwerror(dserr));
				if (dserr == 0) {
					printf("%llu bytes (0x%016llX)\n", ln, ln);
				}
			} else {
				size_t lnr;

				dserr = ncp_read64(conn, fh, offs, length, databuffer, &lnr);
				printf("64-bit read request done with %s\n", strnwerror(dserr));
				if (dserr == 0) {
					printf("%u bytes read\n", lnr);
#if 0
					fwrite(databuffer, 1, lnr, stdout);
#else
					for (i = 0; i < lnr; i++) {
						printf("%02X ", databuffer[i]);
					}
					printf("\n");
#endif
				}
			}
			
		}
		ncp_close_file(conn, fh);
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
