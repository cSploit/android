/*
    eaops.c - Add extended attribute to datastream
              Remove extended attribute from datastream
              Enumerate extended attributes
	      
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

	1.00  2000, August 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

 */

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <ncp/eas.h>

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] path\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
	         "Enumerate extended attributes:\n"
	         "usage: %s [options] path\n"), progname);
	printf(_("\n"
  	         "-h                Print this help text\n"
		 "-t level          Info level (0, 1, 6 or 7)\n"
		 "-n name           Extended attribute name (for info level 6)\n"
	         "\n"));
	printf(_("\n"
	         "Write extended attribute:\n"
	         "usage: %s [options] -w filename path\n"), progname);
	printf(_("\n"
	         "-h                Print this help text\n"
	         "-n name           Extended attribute name\n"
	         "-w filename       Write this extended attribute value\n"
		 "                  (-w /dev/null removes extended attribute)\n"
	         "\n"));
}

static void eaenum0(NWCONN_HANDLE conn, u_int32_t volume, u_int32_t dirent) {
	struct ncp_ea_enumerate_info winfo;
	NWCCODE err;

	winfo.enumSequence = 0;
	err = ncp_ea_enumerate(conn,
		NWEA_FL_INFO0 | NWEA_FL_CLOSE_IMM | NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_DIRENT,
		volume, dirent, -1, NULL, 0, &winfo, NULL, 0, NULL);
	if (err) {
		fprintf(stderr, "Enumeration failed: %s\n", 
			strnwerror(err));
		return;
	}
	if (winfo.errorCode) {
		/* should not happen as we used NWEA_FL_CLOSE_ERR */
		fprintf(stderr, "Enumeration extended fail: %s\n", 
			strnwerror((winfo.errorCode & 0xFF) | NWE_SERVER_ERROR));
	} else {
		printf("Total EAs:  %u\n", winfo.totalEAs);
		printf("DataSize:   %u\n", winfo.totalEAsDataSize);
		printf("KeySize:    %u\n", winfo.totalEAsKeySize);
		printf("New EA handle: %08X\n", winfo.newEAhandle);
	}
	if (winfo.newEAhandle) {
		/* should not happen as we used NWEA_FL_CLOSE_IMM */
		err = ncp_ea_close(conn, winfo.newEAhandle);
		if (err) {
			fprintf(stderr, "Close EA failed: %s\n",
				strnwerror(err));
		}
	}
}

static void eaenum1(NWCONN_HANDLE conn, u_int32_t volume, u_int32_t dirent) {
	unsigned char vv[2048];
	size_t pos;
	struct ncp_ea_enumerate_info winfo;
	NWCCODE err;
	int sawtitle = 0;
	size_t eaid = 1;

	winfo.enumSequence = 0;
	err = ncp_ea_enumerate(conn,
		NWEA_FL_INFO1 | NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_DIRENT,
		volume, dirent, -1, NULL, 0, &winfo, vv, sizeof(vv), &pos);
	do {
		size_t rinfo;
		const unsigned char* p;

		if (err) {
			fprintf(stderr, "Enumeration failed: %s\n", 
				strnwerror(err));
			return;
		}
		if (winfo.errorCode) {
			/* should not happen as we used NWEA_FL_CLOSE_ERR */
			fprintf(stderr, "Enumeration extended fail: %s\n", 
				strnwerror((winfo.errorCode & 0xFF) | NWE_SERVER_ERROR));
			break;
		}

		if (!sawtitle) {
			printf("Total EAs:  %u\n", winfo.totalEAs);
			printf("DataSize:   %u\n", winfo.totalEAsDataSize);
			printf("KeySize:    %u\n", winfo.totalEAsKeySize);
			sawtitle = 1;
		}
		printf("New EA handle: %08X\n", winfo.newEAhandle);
		printf("New search seq: %u\n", winfo.enumSequence);
		printf("Returned items: %u\n", winfo.returnedItems);
		printf("Size:       %u\n", pos);

		p = vv;
		for (rinfo = 0; rinfo < winfo.returnedItems; rinfo++) {
			struct ncp_ea_info_level1 ppp;

			err = ncp_ea_extract_info_level1(p,
				vv + pos, &ppp, sizeof(ppp), NULL, &p);
			if (err)
				printf("  Key %u: Cannot retrieve: %s\n",
					eaid, strnwerror(err));
			else {
				printf("  Key %u:\n", eaid);
				printf("    Name:  %s\n", ppp.key);
				printf("    Access Flag: %08X\n", ppp.accessFlag);
				printf("    Value Length: %u\n", ppp.valueLength);
			}
			eaid++;
		}
		if (!winfo.enumSequence)
			break;
		err = ncp_ea_enumerate(conn, 
			NWEA_FL_INFO1 | NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_EAHANDLE,
			winfo.newEAhandle, 0, -1, NULL, 0,
			&winfo, vv, sizeof(vv), &pos);
	} while (1);
	if (winfo.newEAhandle) {
		err = ncp_ea_close(conn, winfo.newEAhandle);
		if (err) {
			fprintf(stderr, "Close EA failed: %s\n",
				strnwerror(err));
		}
	}
}

static void eaenum6(NWCONN_HANDLE conn, u_int32_t volume, u_int32_t dirent,
		const char* attrname) {
	unsigned char vv[2048];
	size_t pos;
	struct ncp_ea_enumerate_info winfo;
	NWCCODE err;
	size_t eaid = 1;

	winfo.enumSequence = 0;
	err = ncp_ea_enumerate(conn,
		NWEA_FL_INFO6 | NWEA_FL_CLOSE_IMM | NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_DIRENT,
		volume, dirent, -1, attrname, strlen(attrname), &winfo, vv, sizeof(vv), &pos);
	if (err) {
		fprintf(stderr, "Enumeration failed: %s\n", 
			strnwerror(err));
		return;
	}
	if (winfo.errorCode) {
		/* should not happen as we used NWEA_FL_CLOSE_ERR */
		fprintf(stderr, "Enumeration extended fail: %s\n", 
			strnwerror((winfo.errorCode & 0xFF) | NWE_SERVER_ERROR));
	} else {
		size_t rinfo;
		const unsigned char* p;

		printf("Total EAs:  %u\n", winfo.totalEAs);
		printf("DataSize:   %u\n", winfo.totalEAsDataSize);
		printf("KeySize:    %u\n", winfo.totalEAsKeySize);
		printf("New EA handle: %08X\n", winfo.newEAhandle);
		printf("New search seq: %u\n", winfo.enumSequence);
		printf("Returned items: %u\n", winfo.returnedItems);
		printf("Size:       %u\n", pos);

		p = vv;
		for (rinfo = 0; rinfo < winfo.returnedItems; rinfo++) {
			struct ncp_ea_info_level6 ppp;

			err = ncp_ea_extract_info_level6(p,
				vv + pos, &ppp, sizeof(ppp), NULL, &p);
			if (err)
				printf("  Key %u: Cannot retrieve: %s\n",
					eaid, strnwerror(err));
			else {
				printf("  Key %u:\n", eaid);
				printf("    Name:  %s\n", ppp.key);
				printf("    Access Flag: %08X\n", ppp.accessFlag);
				printf("    Value Length: %u\n", ppp.valueLength);
				printf("    Key Extants: %u\n", ppp.keyExtants);
				printf("    Value Extants: %u\n", ppp.valueExtants);
			}
			eaid++;
		}
	}
	if (winfo.newEAhandle) {
		/* should not happen as we used NWEA_FL_CLOSE_IMM */
		err = ncp_ea_close(conn, winfo.newEAhandle);
		if (err) {
			fprintf(stderr, "Close EA failed: %s\n",
				strnwerror(err));
		}
	}
}

static void eaenum7(NWCONN_HANDLE conn, u_int32_t volume, u_int32_t dirent) {
	unsigned char vv[2048];
	size_t pos;
	struct ncp_ea_enumerate_info winfo;
	NWCCODE err;
	int sawtitle = 0;
	size_t eaid = 1;

	winfo.enumSequence = 0;
	err = ncp_ea_enumerate(conn,
		NWEA_FL_INFO7 | NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_DIRENT,
		volume, dirent, -1, NULL, 0, &winfo, vv, sizeof(vv), &pos);
	do {
		size_t rinfo;
		const unsigned char* p;

		if (err) {
			fprintf(stderr, "Enumeration failed: %s\n", 
				strnwerror(err));
			return;
		}
		if (winfo.errorCode) {
			/* should not happen as we used NWEA_FL_CLOSE_ERR */
			fprintf(stderr, "Enumeration extended fail: %s\n", 
				strnwerror((winfo.errorCode & 0xFF) | NWE_SERVER_ERROR));
			break;
		}

		if (!sawtitle) {
			printf("Total EAs:  %u\n", winfo.totalEAs);
			printf("DataSize:   %u\n", winfo.totalEAsDataSize);
			printf("KeySize:    %u\n", winfo.totalEAsKeySize);
			sawtitle = 1;
		}
		printf("New EA handle: %08X\n", winfo.newEAhandle);
		printf("New search seq: %u\n", winfo.enumSequence);
		printf("Returned items: %u\n", winfo.returnedItems);
		printf("Size:       %u\n", pos);

		p = vv;
		for (rinfo = 0; rinfo < winfo.returnedItems; rinfo++) {
			char ppp[300];

			err = ncp_ea_extract_info_level7(p,
				vv + pos, ppp, sizeof(ppp), NULL, &p);
			if (err)
				printf("  Key %u: Cannot retrieve: %s\n",
					eaid, strnwerror(err));
			else
				printf("  Key %u: %s\n", eaid, ppp);
			eaid++;
		}
		if (!winfo.enumSequence)
			break;
		err = ncp_ea_enumerate(conn, 
			NWEA_FL_INFO7 | NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_EAHANDLE,
			winfo.newEAhandle, 0, -1, NULL, 0,
			&winfo, vv, sizeof(vv), &pos);
	} while (1);
	if (winfo.newEAhandle) {
		err = ncp_ea_close(conn, winfo.newEAhandle);
		if (err) {
			fprintf(stderr, "Close EA failed: %s\n",
				strnwerror(err));
		}
	}
}

static void eaenumX(NWCONN_HANDLE conn, u_int32_t volume, u_int32_t dirent,
		const char* attrname, int ilev) {
	unsigned char vv[2048];
	size_t pos;
	struct ncp_ea_enumerate_info winfo;
	NWCCODE err;

	winfo.enumSequence = 0;
	err = ncp_ea_enumerate(conn,
		NWEA_FL_INFO(ilev) | NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_DIRENT,
		volume, dirent, -1, attrname, strlen(attrname), &winfo, vv, sizeof(vv), &pos);
	do {
		size_t rinfo;
		const unsigned char* p;

		if (err) {
			fprintf(stderr, "Enumeration failed: %s\n", 
				strnwerror(err));
			return;
		}
		if (winfo.errorCode) {
			/* should not happen as we used NWEA_FL_CLOSE_ERR */
			fprintf(stderr, "Enumeration extended fail: %s\n", 
				strnwerror((winfo.errorCode & 0xFF) | NWE_SERVER_ERROR));
			break;
		}

		printf("Total EAs:  %u\n", winfo.totalEAs);
		printf("DataSize:   %u\n", winfo.totalEAsDataSize);
		printf("KeySize:    %u\n", winfo.totalEAsKeySize);
		printf("New EA handle: %08X\n", winfo.newEAhandle);
		printf("New search seq: %u\n", winfo.enumSequence);
		printf("Returned items: %u\n", winfo.returnedItems);
		printf("Size:       %u\n", pos);

		printf("Returned data: ");
		p = vv;
		for (rinfo = 0; rinfo < pos; rinfo++) {
			printf("%02X ", vv[rinfo]);
		}
		printf("\n");
		if (!winfo.enumSequence)
			break;
		err = ncp_ea_enumerate(conn, 
			NWEA_FL_INFO(ilev) | NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_EAHANDLE,
			winfo.newEAhandle, 0, -1, NULL, 0,
			&winfo, vv, sizeof(vv), &pos);
	} while (1);
	if (winfo.newEAhandle) {
		err = ncp_ea_close(conn, winfo.newEAhandle);
		if (err) {
			fprintf(stderr, "Close EA failed: %s\n",
				strnwerror(err));
		}
	}
}

static void eawritebuf(NWCONN_HANDLE conn, u_int32_t volume, u_int32_t dirent,
		const char* attrname, const nuint8* buf, size_t buflen) {
	size_t cpos;
	size_t tbs;
	NWCCODE err;
	struct ncp_ea_write_info winfo;
	int flag;
	
	cpos = 0;
	flag = NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_DIRENT;
	do {
		tbs = 512;
		if (cpos + tbs >= buflen) {
			tbs = buflen - cpos;
			flag |= NWEA_FL_CLOSE_IMM;
		}
		err = ncp_ea_write(conn, flag, 
			volume, dirent, buflen, attrname, strlen(attrname),
			cpos, 0, &winfo, buf + cpos, tbs);
		if (err) {
			fprintf(stderr, "Write returned: %s\n", 
				strnwerror(err));
			return;
		}
		if (winfo.errorCode) {
			/* should not happen as we used NWEA_FL_CLOSE_ERR */
			fprintf(stderr, "Write extended error: %s\n",
				strnwerror((winfo.errorCode & 0xFF) | NWE_SERVER_ERROR));
			break;
		}
		if (winfo.written != tbs) {
			fprintf(stderr, "Partial write: %u instead of %u\n",
				winfo.written, tbs);
			break;
		}
		cpos += winfo.written;
		if (cpos >= buflen) {
			printf("Successfully written %u bytes\n", cpos);
			break;
		}
		printf("%u\r", cpos); fflush(stdout);

		attrname = "";
		dirent = 0;
		volume = winfo.newEAhandle;

		flag = NWEA_FL_CLOSE_ERR | NWEA_FL_SRC_EAHANDLE;
	} while (1);
	if (winfo.newEAhandle) {
		/* should happen only on error exit, we use 
		   NWEA_FL_CLOSE_IMM on last write... */
		err = ncp_ea_close(conn, winfo.newEAhandle);
		if (err) {
			fprintf(stderr, "Close EA failed: %s\n", 
				strnwerror(err));
		}
	}
}

static void eawrite(NWCONN_HANDLE conn, u_int32_t volume, u_int32_t dirent,
		const char* attrname, const char* filename) {
	int fd;
	unsigned char* buffer;
	size_t alloc;
	size_t used;
		
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open EA source file: %s\n",
			strerror(errno));
		return;
	}
	used = 0;
	buffer = (unsigned char*)malloc(2048);
	if (!buffer) {
		fprintf(stderr, "Cannot allocate memory: %s\n",
			strerror(errno));
		close(fd);
		return;
	}
	alloc = 2048;
	
	do {
		int rd;
		
		if (used >= alloc) {
			size_t nalloc;
			unsigned char* nb;
			
			nalloc = alloc * 2;
			nb = realloc(buffer, nalloc);
			if (!nb) {
				fprintf(stderr, "Cannot allocate memory: %s\n",
					strerror(errno));
				printf("Continuing with %u bytes read\n",
					used);
				break;
			}
			alloc = nalloc;
			buffer = nb;
		}
		rd = read(fd, buffer + used, alloc - used);
		if (rd <= 0)
			break;
		used += rd;
	} while (1);
	close(fd);

	eawritebuf(conn, volume, dirent, attrname, buffer, used);
	free(buffer);
}

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	int opt;
	struct NWCCRootEntry nwccre;
	const char* attrname = "OS2ExtendedAttribute";
	const char* writefile = NULL;
	int level = 7;
	
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);

	while ((opt = getopt(argc, argv, "h?n:w:t:")) != EOF)
	{
		switch (opt)
		{
		case 't':
			level = strtoul(optarg, NULL, 0);
			break;
		case 'n':
			attrname = optarg;
			break;
		case 'w':
			writefile = optarg;
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
		if (writefile)
			eawrite(conn, nwccre.volume, nwccre.dirEnt,
				attrname, writefile);
		else {
			switch (level) {
				case 0: eaenum0(conn, nwccre.volume, nwccre.dirEnt);
					break;
				case 1:	eaenum1(conn, nwccre.volume, nwccre.dirEnt);
					break;
				case 2:
				case 3:
				case 4:
				case 5: eaenumX(conn, nwccre.volume, nwccre.dirEnt, attrname,
						level);
					break;
				case 6: eaenum6(conn, nwccre.volume, nwccre.dirEnt, attrname);
					break;
				case 7: eaenum7(conn, nwccre.volume, nwccre.dirEnt);
					break;
				default:
					fprintf(stderr, "Unknown info level %u\n", level);
					break;
			}
		}
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
