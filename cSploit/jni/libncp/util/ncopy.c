/*
    ncopy.c - Copy file on a Netware server without Network Traffic
    Copyright (C) 1996 by Brian Reid and Tom Henderson.

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


    Send bug reports for ncopy to "breid@tim.com"
 
    Still to do: support recursive copy with two arguments
    Both must be directories. (similar to rcp -r)


    Revision history:

	0.01  1996			Brian Reid and Tom Henderson
		Initial revision.

	0.02  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Copy trustees and other Netware informations.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
		
	1.01  1999, November 21		Petr Vandrovec <vandrove@vc.cvut.cz>
		Copy MAC resource fork.

	1.02  2000, June 17		Petr Vandrovec <vandrove@vc.cvut.cz>
		Recursive copy.

	1.03  2001, August 9		Petr Vandrovec <vandrove@vc.cvut.cz>
		Copy trustees and other Netware informations on directories.
		Added -u option to copy different files only.

 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <ctype.h>
#include <mntent.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <ncp/nwcalls.h>

#include "private/libintl.h"
#define _(X) gettext(X)

#define dfprintf(X...) fprintf(X)

/****************************************************************************
 * Globals:
 *
 */
const char *VersionStr = "1.02";
static char *ProgramName;

#define RSRCSUBDIR	".rsrc/"

/* (initialized) command options  */

static int optVersion = 0;		/* -V TRUE if just want version */
static int optVerbose = 0;		/* -v TRUE if want verbose output */
static int optRecursive = 0;		/* -r TRUE if want recursive copy */
static int optNice = 0;			/* -n TRUE if we are cooperative (nice) */
static int optNiceFactorSel = 0;	/* -s TRUE if we selected a nice factor */
static int optNiceFactor = 10;		/* -s arg, number of 1M blocks to copy 
				   before sleeping for a second */
static u_int32_t CopyBlockSize = 1048576;	/* Size of the default block copy size */
static unsigned int NiceSleepTime = 1;	/* Number of seconds to sleep in Nice Mode */

static int optOwner = 0;		/* -pp */
static int optFlags = 0;		/* -p */
static int optTimes = 0;		/* -p too... Hack source... */
static int optTrustees = 0;		/* -t */
static int optMAC = 0;			/* -m */
static int optMACinRsrc = 0;		/* -M */
static int optOnlyChanged = 0;		/* -u */
static int BlocksCopied = 0;		/* Number of blocks copied */
static unsigned int MaxNcopyRetries = 25;	/* Maximum number of times to retry a failed
				   copy before giving up  */

/* Globals needed for signal handlers */
static NWCONN_HANDLE CurrentConn = NULL;	/* Connection of output file */
static u_int8_t *CurrentFile = NULL;	/* File info of output file */

/* Signal control structures */
static struct sigaction sHangupSig;
static struct sigaction sInterruptSig;
static struct sigaction sQuitSig;
static struct sigaction sTermSig;

/****************************************************************************
 *
 */
static void
usage(void) {
	fprintf(stderr, _("usage: %s [-V]\n"), ProgramName);
	fprintf(stderr, _("       %s [-vmMnppt] [-s amt] sourcefile destinationfile|directory\n"), ProgramName);
	fprintf(stderr, _("       %s [-vmMnppt] [-s amt] sourcefile [...] directory\n"), ProgramName);
	fprintf(stderr, _("       %s [-vmMnppt] [-s amt] -r sourcedir directory\n"), ProgramName);
}

/****************************************************************************
 * Return pointer to last component of the path.  
 * Returned string may have one or more "/" left on the end. 
 * ("/" returns pointer to "/", null returns pointer to null)
 * Return pointer to original string if no "/" in string. (except at end)
 */
static const char *
myBaseName(const char *path) {
	const char *p;

	for (p = &path[strlen(path)]; p != path; p--)
	{			/* skip ENDING "/" chars */
		if (*p && *p != '/')
			break;
	}
	if (p == path)
		return p;
	for (; p != path || *p == '/'; p--)
	{
		if (*p == '/')
			return ++p;
	}
	return p;
}

/****************************************************************************
 *
 */
static const char *
notDir(const char *path)
{
	struct stat buf;

	if (stat(path, &buf))
		return strerror(errno);		/* no permission? not exist? */
	if (!S_ISDIR(buf.st_mode))
		return _("not a directory");	/* not a directory */
	return NULL;	/* OK */
}

/****************************************************************************
 *
 */
static int
handleOptions(const int argc, char *const argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "vVns:ptmMru")) != EOF)
	{
		switch (opt)
		{

		case 'V':	/* Version */
			optVersion = 1;
			break;

		case 'v':	/* Verbose output */
			optVerbose = 1;
			break;

		case 'n':	/* Nice, cooperative copy */
			optNice = 1;
			break;

		case 's':	/* Nice Factor */
			optNiceFactorSel = 1;
			optNiceFactor = atoi(optarg);
			if (optNiceFactor < 1)
			{
				fprintf(stderr, _("%s: -s option requires positive numeric argument > 0\n"),
					ProgramName);
				return 1;
			}
			break;

		case 'p':	/* preserve flags */
			if (optTimes)
				optOwner = 1;	/* multiple 'p' */
			optFlags = optTimes = 1;
			break;
			
		case 't':
			optTrustees = 1;
			break;

		case 'm':	/* copy MAC resource fork */
			optMAC = 1;
			break;
			
		case 'M':	/* MAC resource fork in .rsrc subdirectory */
			optMACinRsrc = 1;
			break;
			
		case 'r':
			optRecursive = 1;
			break;

		case 'u':
			optOnlyChanged = 1;
			break;

		default:	/* invalid options or options without required arguments */
			return 1;
		}
		continue;
	}
	return 0;
}

/****************************************************************************
 * TODO: if recursive flag last MUST be a directory, even if only 2 args.
 */
static int
validateFileArgs(const int argc, char *const argv[])
{
	const char *p;
	if (argc == 0)
	{
		fprintf(stderr, _("%s: No arguments specified.\n"), ProgramName);
		return 1;
	}
	if (argc == 1)
	{
		fprintf(stderr, _("%s: No destination specified.\n"), ProgramName);
		return 1;
	}
	if ((optRecursive || argc > 2) && (p = notDir(argv[argc - 1])))
	{			/* last arg MUST be dir */
		fprintf(stderr, _("%s: %s: %s\n"), ProgramName, argv[argc - 1], p);
		return 1;
	}
	return 0;
}

static int xlatid(NWCONN_HANDLE scon, NWObjectID sID,
		NWCONN_HANDLE dcon, NWObjectID *dID) {
	NWCCODE err;
	char objectName[256];
	NWObjectType objectType;

	if (sID == 0) {
		/* set zero owner returns 0x8901 error from ncp_modify... */
		return 0;
	}	
	if (scon == dcon) {
		*dID = sID;
		return 1;
	}
	err = NWGetObjectName(scon, sID, objectName, &objectType);
	if (err) {
		fprintf(stderr, _("Cannot convert ID %08X to name: %s\n"),
			sID, strnwerror(err));
		return 0;
	}
	err = NWGetObjectID(dcon, objectName, objectType, dID);
	if (err) {
		fprintf(stderr, _("Cannot convert name %s(%04X) to ID: %s\n"),
			objectName, objectType, strnwerror(err));
		return 0;
	}
	return 1;
}

/****************************************************************************
 *  Interfaces with the ncplib to do the netware copy of the file.
 */
static int
copyNWNW(
		const char* header,
		int sfd,
		NWCONN_HANDLE scon,
		const u_int8_t* sfhandle,
		const char* source,
		int dfd,
		NWCONN_HANDLE dcon,
		const u_int8_t* dfhandle,
		const char* destination) {
	ncp_off64_t totalSize;
	ncp_off64_t sourceOff;
	NWCCODE err;
	unsigned int retryCount;
	int failed;
	int ncopy = scon && (scon == dcon);
	int dolf = optVerbose;

	if (scon) {
		err = ncp_get_file_size(scon, sfhandle, &totalSize);
		if (err)
			totalSize = 0;
	} else {
		struct stat stats;
		
		err = fstat(sfd, &stats);
		if (err)
			totalSize = 0;
		else
			totalSize = stats.st_size;
	}

	sourceOff = 0;
	retryCount = 0;

	if (optVerbose)	{
		if (totalSize)
			printf(_("%s: %s -> %s %5.1f%%"), header, source, destination, 0.0);
		else
			printf(_("%s: %s -> %s %lld"), header, source, destination, 0LL);
		fflush(stdout);
	}
	failed = -1;
	while (retryCount < MaxNcopyRetries) {
		if (ncopy) {
			u_int32_t amountCopied;

			/* If we are being nice and we've copied enough blocks, go to sleep */
			if (optNice)
			{
				if (BlocksCopied == optNiceFactor)
				{
					sleep(NiceSleepTime);
					BlocksCopied = 0;
				} else
					++BlocksCopied;
			}
			err = ncp_copy_file(dcon, sfhandle,
					    dfhandle, sourceOff, 
					    sourceOff,
					    CopyBlockSize, &amountCopied);
			if (err) {
				/* In my testing this only happens when you run out of space 
			   	on the server.
			   	Netware seems to wait a bit before reporting space recently
			   	free'd.  I will just wait a bit before bombin */
				sleep(1);	/* Sleep for a second and try again */
				retryCount++;
			} else {
				if (!amountCopied) {
					ncopy = 0;
				}
				sourceOff += amountCopied;
			}
		} else {
			u_int8_t buffer[49152];
			long bytes;
			long written;
			
			if (scon)
				bytes = ncp_read(scon, sfhandle, sourceOff, sizeof(buffer), buffer);
			else
				bytes = read(sfd, buffer, sizeof(buffer));
			if (bytes < 0) {
				dolf = 0;
				if (optVerbose)
					putchar('\n');
				fprintf(stderr, _("%s: Cannot read %s\n"),
					ProgramName,
					source);
				break;
			}
			if (bytes == 0) {
				/* whole file? */
				if (sourceOff >= totalSize) {
					failed = 0;
					break;
				}
				sleep(1);
				retryCount++;
			} else {
				if (dcon)
					written = ncp_write(dcon, dfhandle, sourceOff, bytes, buffer);
				else
					written = write(dfd, buffer, bytes);
				if (written != bytes) {
					dolf = 0;
					if (optVerbose)
						putchar('\n');
					fprintf(stderr, _("%s: Cannot write %s\n"),
						ProgramName,
						destination);
					break;
				}
				sourceOff += bytes;
			}
		}
		if (optVerbose)
		{
			putchar('\r');
			if (totalSize && (totalSize >= sourceOff))
				printf(_("%s: %s -> %s %5.1f%%"), header, source, destination,
			       		((float) sourceOff) / totalSize * 100.0);
			else
				printf(_("%s: %s -> %s %lld"), header, source, destination,
					sourceOff);
			if (retryCount)
				printf(_("  %d retries"), retryCount);
			fflush(stdout);
		}
	}
	if (dolf)
		putchar('\n');
	return failed;
}

static int
netwareCopyFile(int fdIn, int fdOut,
		const char *source,
		const char *destination,
		mode_t sourceMode, int dofile)
{
	NWCCODE err;
	struct nw_info_struct2 sfi, dfi;
	u_int8_t sfhandle[6];
	u_int8_t dfhandle[6];
	NWCONN_HANDLE scon, dcon;
	struct NWCCRootEntry sentry, dentry;
	int retValue = 2;
	int ncopy = 0;

	err = ncp_open_fd(fdIn, &scon);
	if (err) {
		dfprintf(stderr, _("%s: Unable to open source file: %s\n"),
			ProgramName, strnwerror(err));
		scon = NULL;
		goto srcDetermined;
	}
	err = NWCCGetConnInfo(scon, NWCC_INFO_ROOT_ENTRY, 
		sizeof(sentry), &sentry);
	if (err) {
		dfprintf(stderr, _("%s: Unable to stat source file: %s\n"),
			ProgramName, strnwerror(err));
		ncp_close(scon);
		scon = NULL;
		goto srcDetermined;
	}
	if (dofile) {
		err = ncp_ns_open_create_entry(scon, NW_NS_DOS, SA_ALL | 0x48,
			1, sentry.volume, sentry.dirEnt, NULL, 0, 
			-1, OC_MODE_OPEN, 0, AR_READ_ONLY, 
			RIM_ALL,
			&sfi, sizeof(sfi), NULL, NULL, sfhandle);
	} else {
		err = ncp_ns_obtain_entry_info(scon, NW_NS_DOS, SA_ALL | 0x48,
			1, sentry.volume, sentry.dirEnt, NULL, 0,
			NW_NS_DOS, RIM_ALL,
			&sfi, sizeof(sfi));
	}
	if (err) {
		dfprintf(stderr, _("%s: Unable to open source file: %s\n"),
			ProgramName, strnwerror(err));
		ncp_close(scon);
		scon = NULL;
		goto srcDetermined;
	}

srcDetermined:;
	{
		/* Grrr... All kernels since 2.1.43 up to 2.2.10 & 2.3.9 corrupts 
		           DosDirNum on file open :-( stat() fixes it, fstat does not */
		struct stat statbuf;
		stat(destination, &statbuf);
	}
	err = ncp_open_fd(fdOut, &dcon);
	if (err) {
		dfprintf(stderr, _("%s: Unable to open output file: %s\n"),
			ProgramName, strnwerror(err));
		dcon = NULL;
		goto dstDetermined;
	}
	err = NWCCGetConnInfo(dcon, NWCC_INFO_ROOT_ENTRY, 
		sizeof(dentry), &dentry);
	if (err) {
		dfprintf(stderr, _("%s: Unable to stat destination file: %s\n"),
			ProgramName, strnwerror(err));
		ncp_close(dcon);
		dcon = NULL;
		goto dstDetermined;
	}
	if (dofile) {
		err = ncp_ns_open_create_entry(dcon, NW_NS_DOS, SA_ALL,
			1, dentry.volume, dentry.dirEnt, NULL, 0,
			-1, OC_MODE_OPEN, 0, AR_WRITE_ONLY,
			0,
			&dfi, sizeof(dfi), NULL, NULL, dfhandle);
		if (err) {
			dfprintf(stderr, _("%s: Unable to open destination file: %s\n"),
				ProgramName, strnwerror(err));
			ncp_close(dcon);
			dcon = NULL;
			goto dstDetermined;
		}
	}
dstDetermined:;
	if (scon && dcon) {	
		NWCONN_NUM snum, dnum;
		
		if (!NWGetConnectionNumber(scon, &snum) 
		 && !NWGetConnectionNumber(dcon, &dnum)
		 && (snum == dnum)) {
			/* there is a chance that it is same connection */
			unsigned char saddr[100];
			unsigned char daddr[100];
			NWCCTranAddr srec = {0, sizeof(saddr), saddr};
			NWCCTranAddr drec = {0, sizeof(daddr), daddr};

			err = NWCCGetConnInfo(scon, NWCC_INFO_TRAN_ADDR, sizeof(srec), &srec);
			if (!err) err = NWCCGetConnInfo(dcon, NWCC_INFO_TRAN_ADDR, sizeof(drec), &drec);
			if (!err) {
				if ((srec.type == drec.type) 
				 && (srec.len == drec.len)
				 && !memcpy(saddr, daddr, srec.len)) {
				 	ncopy = 1;
				}
			} else {
				err = NWCCGetConnInfo(scon, NWCC_INFO_SERVER_NAME, sizeof(saddr), saddr);
				if (!err) err = NWCCGetConnInfo(dcon, NWCC_INFO_SERVER_NAME, sizeof(daddr), daddr);
				if (!err) {
					ncopy = !strcmp(saddr, daddr);
				}
			}
			if (ncopy) {
				ncp_close(scon);
				scon = dcon;
			}
		}
	}
	if (dofile) {
		/* Set globals in case a signal happens while copying */
		CurrentConn = dcon;
		CurrentFile = dfhandle;

		retValue = copyNWNW(_("NetWare copy"),
				    fdIn, scon, sfhandle, source,
				    fdOut, dcon, dfhandle, destination);
		if (dcon) {
			if (ncp_close_file(dcon, dfhandle) != 0)
			{
				fprintf(stderr, _("%s: Close failed for %s\n"), ProgramName, destination);
				retValue = 1;
			}
			CurrentFile = NULL;
		}
		if (scon) {
			if (ncp_close_file(scon, sfhandle) != 0)
			{
				fprintf(stderr, _("%s: Close failed for %s\n"), ProgramName, source);
				if (!retValue)
					retValue = 1;
			}
		}
		if ((optMAC || optMACinRsrc) && !retValue) {
			NWCONN_HANDLE sMAC = NULL;
			NWCONN_HANDLE dMAC = NULL;
			int sFD = -1;
			int dFD = -1;
			const char* sourceRsrc = source;
			const char* destinationRsrc = destination;
			char sourceB[MAXPATHLEN+1];
			char destinationB[MAXPATHLEN+1];

			if (scon && optMAC) {
				struct nw_info_struct2 srcMAC;

				err = ncp_ns_obtain_entry_info(scon, NW_NS_DOS, SA_ALL,
					1, sentry.volume, sentry.dirEnt, NULL, 0,
					NW_NS_MAC, RIM_DIRECTORY,
					&srcMAC, sizeof(srcMAC));
				if (err) {
					/* it is not fatal... it is user problem... */
					dfprintf(stderr, _("MAC namespace is not supported on source %s: %s\n"), source, strnwerror(err));
					/* look for .rsrc */
				} else {
					err = ncp_ns_open_create_entry(scon, NW_NS_MAC, SA_ALL,
						1, srcMAC.Directory.volNumber, srcMAC.Directory.dirEntNum, NULL, 0,
						1, OC_MODE_OPEN, 0, AR_READ_ONLY,
						0, 
						NULL, 0, NULL, NULL, sfhandle);
					if (err) {
						/* NWE_SERVER_FAILURE is returned on NSS volume if there does not exist resource fork yet... */
						/* ignore silently */
						if (err != NWE_SERVER_FAILURE) {
							fprintf(stderr, _("Unable to open MAC resource fork on source %s: %s\n"), source, strnwerror(err));
							retValue = 1;
						}
						/* no MAC... */
						goto nomac;
					}
					sMAC = scon;
				}
			}
			if (!sMAC) {
				const char* p;
				char* d;
			
				if (!optMACinRsrc)
					goto nomac;

				p = strrchr(source, '/');
				if (p)
					p = p + 1;
				else
					p = source;
				memcpy(sourceB, source, p - source);
				d = sourceB + (p - source);
				d = stpcpy(d, RSRCSUBDIR);
				strcpy(d, p);
				sFD = open(sourceB, O_RDONLY);
				if (sFD == -1)
					goto nomac;
				sourceRsrc = sourceB;
			}
			if (dcon && optMAC) {
				struct nw_info_struct2 dstMAC;

				err = ncp_ns_obtain_entry_info(dcon, NW_NS_DOS, SA_ALL,
					1, dentry.volume, dentry.dirEnt, NULL, 0,
					NW_NS_MAC, RIM_DIRECTORY,
					&dstMAC, sizeof(dstMAC));
				if (err) {
					/* possible information loss... maybe fallback to create .rsrc/name */
					fprintf(stderr, _("MAC namespace is not supported on destination %s: %s\n"), destination, strnwerror(err));
					retValue = 1;
				} else {
					err = ncp_ns_open_create_entry(dcon, NW_NS_MAC, SA_ALL,
						1, dstMAC.Directory.volNumber, dstMAC.Directory.dirEntNum, NULL, 0,
						1, OC_MODE_OPEN, 0, AR_WRITE_ONLY,
						0,
						NULL, 0, NULL, NULL, dfhandle);
					if (err) {
						/* it is fatal... */
						fprintf(stderr, _("Unable to create MAC resource fork on destination %s: %s\n"), destination, strnwerror(err));
						retValue = 1;
						goto nomac;
					}
					dMAC = dcon;
				}
			}
			if (!dMAC) {
				const char* p;
				char* d;
			
				if (!optMACinRsrc) {
					/* try to create .rsrc/destination */
					fprintf(stderr, _("Unable to copy MAC resource fork of %s because of %s does not support resource forks\n"),
						source,
						destination);
						retValue = 1;
					goto nomac;
				}
			
				p = strrchr(destination, '/');
				if (p)
					p = p + 1;
				else
					p = destination;
				memcpy(destinationB, destination, p - destination);
				d = destinationB + (p - destination);
				d = stpcpy(d, RSRCSUBDIR);
				strcpy(d, p);
				dFD = open(destinationB, O_CREAT | O_TRUNC | O_WRONLY, sourceMode);
				if (dFD == -1) {
					if (errno == ENOENT) {
						*(d-1) = 0;
						mkdir(destinationB, sourceMode | ((sourceMode & 0444) >> 2) | ((sourceMode & 0222) >> 1));
						*(d-1) = '/';
						dFD = open(destinationB, O_CREAT | O_TRUNC | O_WRONLY, sourceMode);
					}
					if (dFD == -1) {
						int err2 = errno;
					
						fprintf(stderr, _("Unable to copy MAC resource fork: %s: %s\n"),
							destinationB,
							strerror(err2));
						retValue = 1;
						goto nomac;
					}
				}
				destinationRsrc = destinationB;
			}
			retValue = copyNWNW(_("NetWare copy (resource fork)"), 
				    sFD, sMAC, sfhandle, sourceRsrc,
				    dFD, dMAC, dfhandle, destinationRsrc);
nomac:;
			if (dMAC)
				ncp_close_file(dMAC, dfhandle);
			if (dFD != -1)
				close(dFD);
			if (sMAC)
				ncp_close_file(sMAC, sfhandle);
			if (sFD != -1)
				close(sFD);
		}
	} else {
		retValue = 0;
	}
	if ((optTrustees) && (retValue == 0) && scon) {
		u_int32_t iter = 0;
		while (1) {
			TRUSTEE_INFO ts[128];
			unsigned int tstcount = 128;
			unsigned int sptr;
			unsigned int dptr;
				
			if (ncp_ns_trustee_scan(scon, NW_NS_DOS, SA_ALL,
					1, sentry.volume, sentry.dirEnt, 
					NULL, 0,
					&iter, ts, &tstcount))
				break;
			if (tstcount == 0)
				break;
			if (!dcon) {
				fprintf(stderr, _("Cannot set trustees on %s because of %s\n"),
					destination,
					_("not NetWare filesystem"));
				break;
			}
			dptr = 0;
			for (sptr = 0; sptr < tstcount; sptr++) {
				if (xlatid(scon, ts[sptr].objectID, dcon, &ts[dptr].objectID)) {
					ts[dptr].objectRights = ts[sptr].objectRights;
					dptr++;
				}
			}
			if (dptr) {
				err = ncp_ns_trustee_add(dcon, NW_NS_DOS, 0x0016,
					1, dentry.volume, dentry.dirEnt, 
					NULL, 0,
					ts, dptr, 0xFFFF);
				if (err) {
					fprintf(stderr, _("Cannot set trustees on %s because of %s\n"),
						destination,
						strnwerror(err));
				}
			}
		}
	}
	if ((optOwner || optFlags || optTimes) && (retValue == 0) && scon) {
		if (!dcon) {
			fprintf(stderr, _("Cannot set file attributes on %s because of %s\n"),
				destination,
				_("not NetWare filesystem"));
		} else {
			struct ncp_dos_info nwmd;
			u_int32_t mask = 0;
			memset(&nwmd, 0, sizeof(nwmd));
			if (optOwner) {
				if (xlatid(scon, sfi.Creation.ID, dcon, &nwmd.Creation.ID))
					mask |= DM_CREATOR_ID;
				if (xlatid(scon, sfi.Modify.ID, dcon, &nwmd.Modify.ID))
					mask |= DM_MODIFIER_ID;
				if (xlatid(scon, sfi.Archive.ID, dcon, &nwmd.Archive.ID))
					mask |= DM_ARCHIVER_ID;
			}
			if (optFlags) {
				nwmd.Attributes = sfi.Attributes.Attributes;
				nwmd.Rights.Grant = sfi.Rights;
				nwmd.Rights.Revoke = ~sfi.Rights;
				mask |= DM_ATTRIBUTES | DM_INHERITED_RIGHTS_MASK;
			}
			if (optTimes) {
				nwmd.Creation.Date = sfi.Creation.Date;
				nwmd.Creation.Time = sfi.Creation.Time;
				nwmd.Modify.Date = sfi.Modify.Date;
				nwmd.Modify.Time = sfi.Modify.Time;
				nwmd.Archive.Date = sfi.Archive.Date;
				nwmd.Archive.Time = sfi.Archive.Time;
				nwmd.LastAccess.Date = sfi.LastAccess.Date;
				mask |= DM_CREATE_DATE | DM_CREATE_TIME |
				        DM_MODIFY_DATE | DM_MODIFY_TIME |
					DM_ARCHIVE_DATE | DM_ARCHIVE_TIME |
					DM_LAST_ACCESS_DATE;
			}
			if (mask) {
				err = ncp_ns_modify_entry_dos_info(
					dcon, NW_NS_DOS, SA_ALL, 
					1, dentry.volume, dentry.dirEnt, NULL, 0,
					mask, &nwmd);
				if (err) {
					fprintf(stderr, _("Cannot set file attributes on %s because of %s\n"),
						destination,
						strnwerror(err));
				}
			}
		}
	}
	/* Clear signal handling globals */
	CurrentConn = NULL;
	if (dcon)
		ncp_close(dcon);
	if (scon && !ncopy)
		ncp_close(scon);
	return retValue;
}


/****************************************************************************
 *  Decides whether to use the traditional file copy or the netware remote
 *  file copy.
 */
static int
selectFn(const struct dirent* entry) {
	if (entry->d_name[0] == '.') {
		if (entry->d_name[1] == 0)
			return 0;
		if (entry->d_name[1] == '.' && entry->d_name[2] == 0)
			return 0;
	}
	return 1;
}

static char sourcePath[MAXPATHLEN * 2];
static char destinationPath[MAXPATHLEN * 2];

static int
copyFiles(void)
{
	int oldUMask;
	int retVal;
	int fdIn, fdOut;
	struct stat statBuf;
#ifdef NCOPY_DEBUG
	printf(_("Param Src   '%s'\n"
	         "Param Dest  '%s'\n"), sourcePath, destinationPath);
#endif

	fdIn = open(sourcePath, O_RDONLY);
	if (fdIn == -1)
	{
		fprintf(stderr, _("%s: Cannot open %s, %s\n"), ProgramName, sourcePath,
			strerror(errno));
		return 1;
	}
	if (fstat(fdIn, &statBuf))
	{
		fprintf(stderr, _("%s: Cannot stat %s, %s\n"), ProgramName, sourcePath,
			strerror(errno));
		close(fdIn);
		return 1;
	}
	if (S_ISDIR(statBuf.st_mode))
	{
		int ecode = 0;
		
		if (optRecursive) {
			struct stat dstBuf;
			struct dirent** entries;
			int rent;
			int i;
			char *dpx;
			char *spx;

			if (!stat(destinationPath, &dstBuf)) {
				if (!S_ISDIR(dstBuf.st_mode)) {
					fprintf(stderr, _("%s: %s: %s\n"), ProgramName, destinationPath, _("not a directory"));
					close(fdIn);
					return 0;
				}
				oldUMask = umask(0);
				if (dstBuf.st_mode != statBuf.st_mode) {
					if (chmod(destinationPath, statBuf.st_mode & 0777)) {
						fprintf(stderr, _("%s: Cannot chmod %s: %s\n"), ProgramName,
							destinationPath, strerror(errno));
					}
				}
			} else {
				oldUMask = umask(0);
				if (mkdir(destinationPath, statBuf.st_mode & 0777)) {
					fprintf(stderr, _("%s: Cannot create %s: %s\n"), ProgramName, destinationPath,
						strerror(errno));
					umask(oldUMask);
					close(fdIn);
					return 1;
				}
			}
			fdOut = open(destinationPath, O_RDONLY);
			if (fdOut == -1)
			{
				fprintf(stderr, _("%s: Cannot create %s, %s\n"), ProgramName, destinationPath,
					strerror(errno));
				umask(oldUMask);
				close(fdIn);
				return 1;
			}

			retVal = netwareCopyFile(fdIn, fdOut, sourcePath, destinationPath, statBuf.st_mode, 0);
			close(fdOut);
			umask(oldUMask);
			close(fdIn);
			rent = scandir(sourcePath, &entries, selectFn, alphasort);
			if (rent < 0) {
				fprintf(stderr, _("%s: Cannot read %s: %s\n"), ProgramName, sourcePath, strerror(errno));
				umask(oldUMask);
				close(fdIn);
				return 1;
			}
			dpx = destinationPath + strlen(destinationPath);
			*dpx = '/';
			spx = sourcePath + strlen(sourcePath);
			*spx = '/';

			for (i = 0; i < rent; i++) {
				int tmp;
				
				strcpy(dpx + 1, entries[i]->d_name);
				strcpy(spx + 1, entries[i]->d_name);
				tmp = copyFiles();
				if (tmp)
					ecode = tmp;
				if (tmp > 1)
					break;
				
			}
			*spx = 0;
			*dpx = 0;
			umask(oldUMask);
			close(fdIn);
		} else {
			fprintf(stderr, _("%s: %s: omitting directory\n"), ProgramName, sourcePath);
			close(fdIn);
		}
		return ecode;
	}

	oldUMask = umask(0);
	if (optOnlyChanged) {
		struct stat statBufOut;

		if (!stat(destinationPath, &statBufOut)) {
			if (statBuf.st_size == statBufOut.st_size &&
			    statBuf.st_mtime == statBufOut.st_mtime) {
			    	printf("%s and %s are same\n", sourcePath, destinationPath);
			    	fdOut = open(destinationPath, O_RDONLY);
			    	if (fdOut == -1) {
			    		goto canopen;
			    	}
			    	retVal = netwareCopyFile(fdIn, fdOut, sourcePath, destinationPath, statBuf.st_mode, 0);
			    	goto cpok;
			}
		}
	}
	fdOut = open(destinationPath, O_CREAT | O_TRUNC | O_WRONLY, statBuf.st_mode);
	if (fdOut == -1)
	{
canopen:;
		fprintf(stderr, _("%s: Cannot create %s, %s\n"), ProgramName, destinationPath,
			strerror(errno));
		umask(oldUMask);
		close(fdIn);
		return 1;
	}

	retVal = netwareCopyFile(fdIn, fdOut, sourcePath, destinationPath, statBuf.st_mode, 1);
cpok:;
	close(fdOut);
	umask(oldUMask);
	close(fdIn);
	return retVal;
}


/****************************************************************************
 * 
 * HERE
 *
 * Brian may NEED "fake" path if he prints error messages?
 * or I may need a way to get his error messages so I can
 * print them with the "fake" path.
 * My current error messages are on the REAL path, which would be confusing...
 d
 * (1-source problem, 2-destination problem, 3-other fatal)
 * We need to decide when to exit or continue the loop, 
 * and what to return when we do exit the loop.
 * Is it failure if 3 files are to be copied, and 1 fails?
 * If one copy fails, we stay in the loop, right?
 * Is it failure if destination fails?
 * Do we Stay in the loop?
 */
static int
copyRealPaths(void)
{
	return copyFiles();
}

/****************************************************************************
 * guaranteed argc is at least 2 and
 * if argc > 2 last parameter is a directory
 * by validateFileArgs()
 */
static int
handleFileArgs(int argc, char *const argv[])
{
	int loop;
	const char *destination;
	int copyStatus;
	int returnCode = 0;	/* default program exit code */
	const char *baseNamePtr;

	destination = argv[argc - 1];	/* get LAST argument */
	for (loop = 0; loop < (argc - 1); loop++)
	{			/* all file arguments, but last */
		strncpy(destinationPath, destination, MAXPATHLEN);
		destinationPath[MAXPATHLEN] = 0;
		if ((argc > 2) || (!notDir(argv[argc - 1])))
		{		/* destination is a dir */
			if (*destinationPath != '/' || *(destinationPath + 1))
				strcat(destinationPath, "/");
			baseNamePtr = myBaseName(argv[loop]);	/* get the file name */
			strcat(destinationPath, baseNamePtr);	/* add it on end of directory */
		}
		strncpy(sourcePath, argv[loop], MAXPATHLEN);
		sourcePath[MAXPATHLEN] = 0;
		copyStatus = copyRealPaths();	/* do the copy */
		if (copyStatus > 1)
			return copyStatus;	/* fatal failure? bye */
		if (copyStatus == 1)
			returnCode = 1;		/* a partial failure? we can continue */
	}
	return returnCode;	/* return what will be the program exit code */
}
/****************************************************************************
 * 
 */
static void __attribute__((noreturn))
handleSignals(int sigNumber)
{
	/* Ignore Signal Handling while cleaning up */

	/* SIGHUP */
	sHangupSig.sa_handler = SIG_IGN;
	if (sigaction(SIGHUP, &sHangupSig, NULL) == -1)
	{
		fprintf(stderr, _("%s: Reset to ignore SIGHUP signal failed: %s"),
			ProgramName, strerror(errno));
	}
	/* SIGINT */
	sInterruptSig.sa_handler = SIG_IGN;
	if (sigaction(SIGINT, &sInterruptSig, NULL) == -1)
	{
		fprintf(stderr, _("%s: Reset to ignore SIGINT signal failed: %s"),
			ProgramName, strerror(errno));
	}
	/* SIGQUIT */
	sQuitSig.sa_handler = SIG_IGN;
	if (sigaction(SIGQUIT, &sQuitSig, NULL) == -1)
	{
		fprintf(stderr, _("%s: Reset to ignore SIGQUIT signal failed: %s"),
			ProgramName, strerror(errno));
	}
	/* SIGTERM */
	sTermSig.sa_handler = SIG_IGN;
	if (sigaction(SIGTERM, &sTermSig, NULL) == -1)
	{
		fprintf(stderr, _("%s: Reset to ignore SIGTERM signal failed: %s"),
			ProgramName, strerror(errno));
	}
	/* If we don't close the ncp output file, we have to ncpumount and
	   ncpmount before we can get rid of it.  */
	if (CurrentFile)
	{
		/*  Issue a warning if we cannot close the file */
		/*  If an error occurs we probably have to umount/mount to 
		   remove the file */
		if (ncp_close_file(CurrentConn, CurrentFile) != 0)
		{
			fprintf(stderr, _("%s: unclean close of output file"), ProgramName);
		}
		CurrentFile = NULL;
	}
	if (CurrentConn) {
		ncp_close(CurrentConn);
		CurrentConn = NULL;
	}
	exit(128 + sigNumber);
}

/****************************************************************************
 * We'll trap Hangup, Interrupt, Quit or Terminate
 */
static int
trapSignals(void)
{
	if (sigaction(SIGHUP, NULL, &sHangupSig))
	{			/* init structure fields */
		fprintf(stderr, _("%s: Get HANGUP signal action failed: %s"),
			ProgramName, strerror(errno));
		return 1;
	}
	sHangupSig.sa_handler = handleSignals;
	if (sigaction(SIGHUP, &sHangupSig, NULL) == -1)
	{
		fprintf(stderr, _("%s: Reset HANGUP signal action failed: %s"),
			ProgramName, strerror(errno));
		return 1;
	}
	if (sigaction(SIGINT, NULL, &sInterruptSig))
	{			/* init structure fields */
		fprintf(stderr, _("%s: Get INTERRUPT signal action failed: %s"),
			ProgramName, strerror(errno));
		return 1;
	}
	sInterruptSig.sa_handler = handleSignals;
	if (sigaction(SIGINT, &sInterruptSig, NULL) == -1)
	{
		fprintf(stderr, _("%s: Reset INTERRUPT signal action failed: %s"),
			ProgramName, strerror(errno));
		return 1;
	}
	if (sigaction(SIGQUIT, NULL, &sQuitSig))
	{			/* init structure fields */
		fprintf(stderr, _("%s: Get QUIT signal action failed: %s"),
			ProgramName, strerror(errno));
		return 1;
	}
	sQuitSig.sa_handler = handleSignals;
	if (sigaction(SIGQUIT, &sQuitSig, NULL) == -1)
	{
		fprintf(stderr, _("%s: Reset QUIT signal action failed: %s"),
			ProgramName, strerror(errno));
		return 1;
	}
	if (sigaction(SIGTERM, NULL, &sTermSig))
	{			/* init structure fields */
		fprintf(stderr, _("%s: Get TERMINATE signal action failed: %s"),
			ProgramName, strerror(errno));
		return 1;
	}
	sTermSig.sa_handler = handleSignals;
	if (sigaction(SIGTERM, &sTermSig, NULL) == -1)
	{
		fprintf(stderr, _("%s: Reset TERMINATE signal action failed: %s"),
			ProgramName, strerror(errno));
		return 1;
	}
	return 0;
}

/****************************************************************************
 *
 */
int
main(int argc, char *const argv[])
{
	int returnCode;
	ProgramName = strrchr(argv[0], '/');
	if (ProgramName)
		ProgramName++;
	else
		ProgramName = argv[0];

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	if (handleOptions(argc, argv))
	{			/* bad option, missing option parameter */
		usage();
		return 1;
	}
	if (optVersion)
	{			/* only option not requiring any arguments */
		printf(_("%s version %s\n"), ProgramName, VersionStr);
		return 0;
	}
	if (validateFileArgs(argc - optind, argv + optind))
	{
		usage();
		return 1;
	}
	if (trapSignals())
		return 1;
	returnCode = handleFileArgs(argc - optind, argv + optind);
	return returnCode;
}
