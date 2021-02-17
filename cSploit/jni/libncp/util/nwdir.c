/*
    nwdir.c - list contents of directory
    Copyright (c) 1998       Milan Vandrovec
    Copyright (c) 1999-2001  Petr Vandrovec
  
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

	0.00  1998			Milan Vandrovec
		Initial revision.

	0.01  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Updated for new ncp_ns_* interface.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
  
	1.01  2000, August 24		Petr Vandrovec <vandrove@vc.cvut.cz>
		Updated to ncp_ns_search_init/ncp_ns_search_next/
			ncp_ns_search_end.
		Updated to ncp_ns_extract_info_field.

	1.02  2000, August 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added ncp_ea_enumerate code.
		
	1.03  2001, January 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Updated to use getopt interface.
		Added ncp_ns_scan_connections_using_file dump code.

	1.04  2001, February 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added ncp_ns_scan_physical_locks_by_file dump code.

	1.05  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Use unistd.h and not getopt.h for getopt().

	1.06  2002, February 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Do not dump core when file opened for nothing is found.

 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <ncp/eas.h>

#include "private/libintl.h"
#define _(X) gettext(X)
#define N_(X) (X)

#ifndef A_DONT_SUBALLOCATE
#define A_DONT_SUBALLOCATE  0x00000800L
#endif

#ifdef N_PLAT_DOS
#ifndef NTYPES_H
typedef unsigned int   nuint16;
#endif
typedef unsigned long  nuint32;
typedef unsigned int   nuint;
#define P08X	"(%08lX)"
#define P32U	"lu"
#else
#define stricmp(A,B) strcasecmp(A,B)
#define P08X	"(%08X)"
#define P32U	"u"
#endif

static int opt_t = 0;
static int opt_l = 0;
static int opt_v = 0;
static int opt_e = 0;
static int opt_f = 0;
static int opt_p = 0;

static void ncpb_add_byte(unsigned char** c, int v) {
  *(*c)++ = v;
}

static void ncpb_add_word_lh(unsigned char** c, nuint v) {
  WSET_LH(*c, 0, v);
  (*c)+=2;
}

static void ncpb_add_dword_lh(unsigned char** c, nuint32 v) {
  DSET_LH(*c, 0, v);
  (*c)+=4;
}

static nuint16 ncpb_get_word_lh(unsigned char** c) {
  nuint16 tmp;
  tmp = WVAL_LH(*c, 0);
  (*c)+=2;
  return tmp;
}

u_int32_t img;
struct nsInfo {
	struct NSI_Name	     name;
	struct NSI_Directory dir;
	int                  valid;
};

static struct nsInfo os2, mac, nfs;

static unsigned char buf[1024];
static unsigned char buf2[1024];

static NWCCODE NWNSEntryInfo2(NWCONN_HANDLE conn, nuint32 vol, nuint32 dosdir, nuint ns, struct NSI_Name* name, struct NSI_Directory* dir) {
	struct nw_info_struct3 info;
	NWCCODE err;

	info.len = 0;
	info.data = NULL;

	err = ncp_ns_obtain_entry_info(conn, NW_NS_DOS, SA_ALL,
		1, vol, dosdir, NULL, 0, ns, RIM_NAME | RIM_DIRECTORY,
		&info, sizeof(info));
	if (err)
		return err;
	err = ncp_ns_extract_info_field(&info, NSIF_NAME, name, sizeof(*name));
	if (!err) {
		err = ncp_ns_extract_info_field(&info, NSIF_DIRECTORY, dir, sizeof(*dir));
	}
	free(info.data);
	return err;
}

static NWCCODE NWNSScanForTrustees(NWCONN_HANDLE conn, 
		nuint32 vol, nuint32 dosdir, 
		nuint32* iter, nuint* no_tstees, TRUSTEE_INFO* tstees) {
	return ncp_ns_trustee_scan(conn, NW_NS_DOS, SA_ALL, 
		1, vol, dosdir, NULL, 0, iter, tstees, no_tstees);
}

static NWCCODE NWNSGetEffectiveRights(NWCONN_HANDLE conn, nuint32 vol, nuint32 dosdir, nuint16* eff) {
	unsigned char* c=buf2;
	NW_FRAGMENT rq;
	NW_FRAGMENT rp;
	NWCCODE err;

	ncpb_add_byte(&c, 0x1D);
	ncpb_add_byte(&c, NW_NS_DOS);
	ncpb_add_byte(&c, 0);		/* ?? */
	ncpb_add_word_lh(&c, SA_ALL);
	ncpb_add_dword_lh(&c, 0);	/* ReturnInfoMask=0, we are not interested in anything */
	ncpb_add_byte(&c, vol);
	ncpb_add_dword_lh(&c, dosdir);
	ncpb_add_byte(&c, 1);
	ncpb_add_byte(&c, 0);
	rq.fragAddress = buf2;
	rq.fragSize = c-buf2;
	rp.fragAddress = buf;
	rp.fragSize = sizeof(buf);
	err = NWRequest(conn, 0x57, 1, &rq, 1, &rp);
	if (err) return err;
	c = buf;
	*eff = ncpb_get_word_lh(&c);
	return 0;
}

static char* createMask311(nuint mask) {
	static char r[9];

	r[0] = (mask&TR_SUPERVISOR) ?'S':'-';
	r[1] = (mask&TR_READ)       ?'R':'-',
	r[2] = (mask&TR_WRITE)      ?'W':'-',
	r[3] = (mask&TR_CREATE)     ?'C':'-',
	r[4] = (mask&TR_ERASE)      ?'E':'-',
	r[5] = (mask&TR_MODIFY)     ?'M':'-',
	r[6] = (mask&TR_SEARCH)     ?'F':'-',
	r[7] = (mask&TR_ACCESS_CTRL)?'A':'-',
	r[8] = 0;
	return r;
}

static void doID(NWCONN_HANDLE conn, nuint32 id) {
	NWCCODE err;
	char user[MAX_DN_BYTES];
	nuint16 type;

	if (opt_t)
		printf(P08X " ", id);
	if (!id) {
		printf(_("Nobody"));
	} else {
#ifdef N_PLAT_DOS
		id = ntohl(id);
#endif
		err = NWGetObjectName(conn, id, user, &type);
		if (err) {
			NWDSContextHandle ctx;
			
			sprintf(user, _("Unknown:N/A"));
			err = NWDSCreateContextHandle(&ctx);
			if (!err) {
				NWDSAddConnection(ctx, conn);
				err = NWDSMapIDToName(ctx, conn, id, user);
				if (err)
					sprintf(user, _("Unknown:<%s>"), strnwerror(err));
				NWDSFreeContext(ctx);
			}
		} else {
			switch (type) {
				case OT_USER:       printf(_("User:"));break;
				case OT_USER_GROUP: printf(_("Group:"));break;
				case OT_FILE_SERVER:printf(_("FileServer:"));break;
				default:            printf(_("Unknown(%04X):"), type);break;
			}
		}
		printf("%s", user);
	}
}

static inline size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm) {
	return strftime(s, max, fmt, tm);
}

static void dodate(nuint date) {
	static const time_t zero_time_t = 0;
	struct tm* tm;
	char text[100];
	
	tm = gmtime(&zero_time_t);
	tm->tm_year = (date>>9)+80;
	tm->tm_mon = ((date>>5) & 0xF) - 1;
	tm->tm_mday = date & 0x1F;
	tm->tm_isdst = 0;
	my_strftime(text, sizeof(text), "%x", tm);
	printf("%s", text);
}

static void dotime(nuint dtime) {
	static const time_t zero_time_t = 0;
	struct tm* tm;
	char text[100];
	
	tm = gmtime(&zero_time_t);
	tm->tm_hour = dtime >> 11;
	tm->tm_min = (dtime >> 5) & 0x3F;
	tm->tm_sec = (dtime << 1) & 0x3F;
	tm->tm_isdst = 0;
	my_strftime(text, sizeof(text), "%X", tm);
	printf("%s", text);
}

static void dodatesTimesID(NWCONN_HANDLE conn, nuint dtime, nuint date, nuint32 id) {
	if (dtime || date) {
		dodate(date);
		printf(" ");
		dotime(dtime);
	} else {
		printf("%-17s", _("never"));
	}
	if (id) {
		printf(" ");
		doID(conn, id);
	}
	printf("\n");
}

static void eaenum(NWCONN_HANDLE conn, u_int32_t volume, u_int32_t dirent) {
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
			//fprintf(stderr, "Enumeration failed: %s\n", 
			//	strnwerror(err));
			return;
		}
		if (winfo.errorCode) {
			/* should not happen as we used NWEA_FL_CLOSE_ERR */
			//fprintf(stderr, "Enumeration extended fail: %s\n", 
			//	strnwerror((winfo.errorCode & 0xFF) | NWE_SERVER_ERROR));
			break;
		}
		/* no extended attributes */
		if (!winfo.totalEAs)
			break;
		if (!sawtitle) {
			printf(_("Extended attributes: %u attributes\n"
			         "                     %u bytes in keys, %u bytes in data\n"), 
				 	winfo.totalEAs, winfo.totalEAsKeySize, winfo.totalEAsDataSize);
			sawtitle = 1;
		}

		p = vv;
		for (rinfo = 0; rinfo < winfo.returnedItems; rinfo++) {
			struct ncp_ea_info_level1 ppp;

			err = ncp_ea_extract_info_level1(p,
				vv + pos, &ppp, sizeof(ppp), NULL, &p);
			if (err)
				printf("  Key %u: Cannot retrieve: %s\n",
					eaid, strnwerror(err));
			else {
				printf(_("  Key %u:\n"
				         "    Name:         %s\n"
					 "    Access Flag:  0x%08X\n"
					 "    Value Length: %u\n"), 
					 eaid, ppp.key, ppp.accessFlag, 
					 ppp.valueLength);
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
//		if (err) {
//			fprintf(stderr, "Close EA failed: %s\n",
//				strnwerror(err));
//		}
	}
	if (sawtitle)
		printf("\n");
}

static void dumpDataSizes(const struct nw_info_struct3* info) {
	NWCCODE err;
	struct NSI_DatastreamLogicals* logical;
	size_t tmplen;
	
	if (!(img & RIM_DATASTREAM_LOGICALS)) {
		ncp_off64_t off;

		err = ncp_ns_extract_info_field(info, NSIF_DATA_SIZE,
			&off, sizeof(off));
		if (err) {
			printf(_("  Cannot determine file size: %s\n"), strnwerror(err));
		} else {
			printf(_("  File size:       %10Lu"), off);

			err = ncp_ns_extract_info_field(info, NSIF_SPACE_ALLOCATED,
				&off, sizeof(off));
			if (!err) {
				printf(_(" (allocated %Lu)"), off * 8ULL);
			}
			printf("\n");
		}
		return;
	}
	err = ncp_ns_extract_info_field_size(info, NSIF_DATASTREAM_LOGICALS,
		&tmplen);
	if (err)
		return;
	logical = (struct NSI_DatastreamLogicals*)malloc(tmplen);
	if (!logical)
		return;
	err = ncp_ns_extract_info_field(info, NSIF_DATASTREAM_LOGICALS,
		logical, tmplen);
	if (err) {
		printf(_("  Cannot determine file size: %s\n"), strnwerror(err));
	} else if (!logical->NumberOfDatastreams) {
		printf(_("  No datastream exist\n"));
	} else {
		struct NSI_DatastreamSizes* size;
		size_t i;
		
		size = NULL;
		err = ncp_ns_extract_info_field_size(info, NSIF_DATASTREAM_SIZES,
			&tmplen);
		if (!err) {
			size = (struct NSI_DatastreamSizes*)malloc(tmplen);
			if (size) {
				err = ncp_ns_extract_info_field(info,
					NSIF_DATASTREAM_SIZES, size, tmplen);
				if (err) {
					free(size);
					size = NULL;
				}
			}
		}
		for (i = 0; i < logical->NumberOfDatastreams; i++) {
			size_t j;
			u_int32_t num = logical->ds[i].Number;
			
			if (num) {
				printf(_("  Stream %3u size: %10Lu"), num,
					logical->ds[i].Size);
			} else {
				printf(_("  File size:       %10Lu"),
					logical->ds[i].Size);
			}
			if (size) {
				for (j = 0; j < size->NumberOfDatastreams; j++) {
					if (size->ds[j].Number == num) {
						printf(_(" (allocated %Lu)"),
							((ncp_off64_t)size->ds[j].FATBlockSize) * 512ULL);
						break;					
					}
				}
			}
			printf("\n");
		}
		if (size)
			free(size);
	}
	free(logical);
}

static void dumpit(NWCONN_HANDLE conn, const struct nw_info_struct3* info) {
	NWCCODE err;
	struct NSI_Attributes attr;
	struct NSI_Name name;
	struct NSI_Directory dir;
	
	err = ncp_ns_extract_info_field(info, NSIF_ATTRIBUTES, &attr, sizeof(attr));
	if (err) {
		fprintf(stderr, _("Cannot retrieve file attributes: %s\n"),
			strnwerror(err));
		return;
	}
	err = ncp_ns_extract_info_field(info, NSIF_DIRECTORY, &dir, sizeof(dir));
	if (err) {
		fprintf(stderr, _("Cannot retrieve file number: %s\n"),
			strnwerror(err));
		return;
	}
	err = ncp_ns_extract_info_field(info, NSIF_NAME, &name, sizeof(name));
	if (err) {
		fprintf(stderr, _("Cannot retrieve file name: %s\n"),
			strnwerror(err));
		return;
	}
	if (attr.Attributes & A_DIRECTORY) {
		printf(_("Directory:\n"));
	} else {
		printf(_("File:\n"));
	}
	if (opt_v || opt_l) {
		err = NWNSEntryInfo2(conn, dir.volNumber, dir.DosDirNum, NW_NS_OS2, &os2.name, &os2.dir);
		os2.valid = err==0;
		err = NWNSEntryInfo2(conn, dir.volNumber, dir.DosDirNum, NW_NS_MAC, &mac.name, &mac.dir);
		mac.valid = err==0;
		err = NWNSEntryInfo2(conn, dir.volNumber, dir.DosDirNum, NW_NS_NFS, &nfs.name, &nfs.dir);
		nfs.valid = err==0;
	} else {
		os2.valid = 0;
		mac.valid = 0;
		nfs.valid = 0;
	}
	printf(_("  DOS:  "));
	if (opt_t) 
		printf(P08X " ", dir.dirEntNum);
	printf("%s\n", name.Name);
	if (os2.valid) {
		printf(_(" OS/2:  "));
		if (opt_t)
			printf(P08X " ", os2.dir.dirEntNum);
		printf("%s\n", os2.name.Name);
	}
	if (nfs.valid) {
		printf(_("  NFS:  "));
		if (opt_t)
			printf(P08X " ", nfs.dir.dirEntNum);
		printf("%s\n", nfs.name.Name);
	}
	if (mac.valid) {
		printf(_("  MAC:  "));
		if (opt_t)
			printf(P08X " ", mac.dir.dirEntNum);
		printf("%s\n", mac.name.Name);
	}
	printf("\n");
	if (opt_v) {
		u_int32_t inheritedRightsMask;
		
		printf(_("Rights:\n"));
		printf(_("  Inherited: "));
		err = ncp_ns_extract_info_field(info, NSIF_RIGHTS,
			&inheritedRightsMask, sizeof(inheritedRightsMask));
		if (err) {
			printf(_("Cannot determine: %s\n"), strnwerror(err));
		} else {
			nuint32 eff32;
			
			if (opt_t)
				printf("(%04X) ", inheritedRightsMask);
			printf("[%s]\n", createMask311(inheritedRightsMask));
			
			err = ncp_ns_extract_info_field(info, NSIF_EFFECTIVE_RIGHTS,
				&eff32, sizeof(eff32));
			if (err) {
				nuint16 eff;

				err = NWNSGetEffectiveRights(conn, 
					dir.volNumber, dir.DosDirNum, &eff);
				eff32 = eff;
			}
			printf(_("  Effective: "));
			if (err) {
				printf(_("Cannot determine: %s\n"), strnwerror(err));
			} else {
				if (opt_t)
					printf("(%04X) ", eff32);
				printf("[%s]\n", createMask311(eff32));
			}
		}
		printf("\n");
	}
	if (opt_l || opt_v) {
		u_int32_t ns;
		
		printf(_("Owning namespace: "));
		err = ncp_ns_extract_info_field(info, NSIF_OWNING_NAMESPACE,
			&ns, sizeof(ns));
		if (err) {
			printf(_("Cannot determine: %s\n"), strnwerror(err));
		} else {
			if (opt_t)
				printf("(%" P32U ") ", ns);
			printf("%s\n", ncp_namespace_to_str(NULL, ns));
		}
		printf("\n");
	}
	if (opt_v) {
		struct NSI_Modify modify;
		struct NSI_Change change;
		NWCCODE moderr;
		
		printf(_("Miscellaneous NetWare Information:\n"));
		printf(_("  Last update:    "));
		moderr = ncp_ns_extract_info_field(info, NSIF_MODIFY,
			&modify, sizeof(modify));
		if (moderr) {
			printf(_("Cannot determine: %s\n"), strnwerror(moderr));
		} else {
			dodatesTimesID(conn, modify.Modify.Time, modify.Modify.Date, modify.Modify.ID);
		}
		printf(_("  Last archived:  "));
		err = ncp_ns_extract_info_field(info, NSIF_ARCHIVE,
			&change, sizeof(change));
		if (err) {
			printf(_("Cannot determine: %s\n"), strnwerror(err));
		} else {
			dodatesTimesID(conn, change.Time, change.Date, change.ID);
		}
		if (1 || !(attr.Attributes&A_DIRECTORY)) {
			printf(_("  Last accessed:  "));
			if (moderr) {
				printf(_("Cannot determine: %s\n"), strnwerror(moderr));
			} else {
				dodatesTimesID(conn, modify.LastAccess.Time, modify.LastAccess.Date, 0);
			}
		}
		printf(_("  Created/Copied: "));
		err = ncp_ns_extract_info_field(info, NSIF_CREATION,
			&change, sizeof(change));
		if (err) {
			printf(_("Cannot determine: %s\n"), strnwerror(err));
		} else {
			dodatesTimesID(conn, change.Time, change.Date, change.ID);
		}
		printf(_("  Flags:          [%s%s%s%s]"),
			(attr.Attributes&A_READ_ONLY     )?"Ro":"Rw",
			(attr.Attributes&A_SYSTEM        )?"Sy":"--",
			(attr.Attributes&A_HIDDEN        )?"H" :"-",
			(attr.Attributes&A_NEEDS_ARCHIVED)?"A" :"-");
		printf(" [%s%s%s%s" "%s%s%s" "%s" "%s%s%s%s%s]",
			(attr.Attributes&A_EXECUTE_ONLY  )?"X" :"-",
			(attr.Attributes&A_TRANSACTIONAL )?"T" :"-",
			(attr.Attributes&A_IMMEDIATE_PURGE)?"P":"-",
			(attr.Attributes&A_SHAREABLE     )?"Sh":"--",

			(attr.Attributes&A_DELETE_INHIBIT)?"Di":"--",
			(attr.Attributes&A_COPY_INHIBIT  )?"Ci":"--",
			(attr.Attributes&A_RENAME_INHIBIT)?"Ri":"--",

			(attr.Attributes&A_DONT_COMPRESS )?"Dc":
				(attr.Attributes&A_IMMEDIATE_COMPRESS)?"Ic":"--",

			(attr.Attributes&A_DONT_MIGRATE  )?"Dm":"--",
			(attr.Attributes&A_DONT_SUBALLOCATE)?"Ds":"--",
			(attr.Attributes&A_INDEXED       )?"I":"--",
			(attr.Attributes&A_READ_AUDIT    )?"Ra":"--",
			(attr.Attributes&A_WRITE_AUDIT   )?"Wa":"--");
		printf(" [%s%s]",
			(attr.Attributes&A_FILE_COMPRESSED)?"Co":
				(attr.Attributes&A_CANT_COMPRESS)?"Cc":"--",
			(attr.Attributes&A_FILE_MIGRATED )?"M":"-");
		if (opt_t)
			printf(" " P08X, attr.Attributes);
		printf("\n");
		if (!(attr.Attributes&A_DIRECTORY)) {
			dumpDataSizes(info);
		}
		printf("\n");
		{
			nuint32 iter = 0;
			int first = 1;

			while (1) {
				nuint tcount;
				nuint i;
				TRUSTEE_INFO tstees[40];

				tcount = 40;
				err = NWNSScanForTrustees(conn, dir.volNumber, 
					dir.DosDirNum, &iter, &tcount, tstees);
			        if (err)
					break;
				for (i=0; i<tcount; i++) {
					if (first) {
						printf(_("Trustees:\n"));
						first = 0;
					}
					printf("  [%s] ", createMask311(tstees[i].objectRights));
					doID(conn, tstees[i].objectID);
					printf("\n");
				}
			}
			if (!first)
				printf("\n");
		} 
	}
	if (opt_e)
		eaenum(conn, dir.volNumber, dir.DosDirNum);
	if (opt_f) {
		u_int16_t iterHandle = 0;
		CONN_USING_FILE cf;
		CONNS_USING_FILE cfa;
		int first = 1;

		while (ncp_ns_scan_connections_using_file(conn, dir.volNumber, dir.DosDirNum, 0, &iterHandle, &cf, &cfa) == 0) {
			if (cfa.connCount != 0) {
				time_t t;
				struct ncp_bindery_object o;
				
				if (first) {
				        printf(_("File Usage:\n"));
					printf(_("  Use Count:              %5u  Open Count:              %5u\n"
					         "  Open For Reading Count: %5u  Open For Writting Count: %5u\n"
						 "  Deny Read Count:        %5u  Deny Write Count:        %5u\n"
						 "  Locked:       %-15s  Fork Count:              %5u\n"),
						 cfa.useCount, cfa.openCount, cfa.openForReadCount, cfa.openForWriteCount,
						 cfa.denyReadCount, cfa.denyWriteCount, cfa.locked ? "exclusive" : "no",
						 cfa.forkCount);
					printf(_("  Connection Count:  %10u\n"), cfa.connCount);
					first = 0;
				}
				
				printf(_("    Connection: %u/%u"), cf.connNumber, cf.taskNumber);
				err = ncp_get_stations_logged_info(conn, cf.connNumber, &o, &t);
				if (!err) {
					printf(_(", "));
					doID(conn, o.object_id);
				}
				printf("\n");
				{
					static const char* lock_bits[] = {N_("locked"), N_("open shareable"),
					                                 N_("logged"), N_("open normal"),
									 N_("rsvd"), N_("rsvd"),
									 N_("TTS locked"), N_("TTS")};
					static const char* acc_bits[] = {N_("read"), N_("write"),
									 N_("deny read"), N_("deny write"),
									 N_("detached"), N_("TTS holding detach"),
									 N_("TTS holding open"), N_("rsvd")};
					char lstr[200];
					char accstr[200];
					char* l2str;
					char* p;
					int i;
					
					p = NULL;
					for (i = 0; i < 8; i++) {
						if (cf.lockType & (1 << i)) {
							if (p) {
								strcpy(p, ", ");
								p = strchr(p, 0);
							} else {
								p = lstr;
							}
							strcpy(p, _(lock_bits[i]));
							p = strchr(p, 0);
						}
					}
					if (!p) {
						strcpy(lstr, _("unlocked"));
					}
					switch (cf.lockFlag) {
						case 0x00:	l2str = _("Not locked"); break;
						case 0xFE:	l2str = _("Locked by a file lock"); break;
						case 0xFF:	l2str = _("Locked by Begin Share File Set"); break;
						default:	l2str = _("Unknown lock state"); break;
					}
					p = NULL;
					for (i = 0; i < 8; i++) {
						if (cf.accessControl & (1 << i)) {
							if (p) {
								strcpy(p, ", ");
								p = strchr(p, 0);
							} else {
								p = accstr;
							}
							strcpy(p, _(acc_bits[i]));
							p = strchr(p, 0);
						}
					}
					if (!p) {
						strcpy(accstr, _("unlocked"));
					}
					if (opt_t) {
						printf(_("          Lock:   (%02X) %s\n"), cf.lockType, lstr);
						printf(_("                  (%02X) %s\n"), cf.lockFlag, l2str);
						printf(_("          Access: (%02X) %s\n"), cf.accessControl, accstr);
					} else {
						printf(_("          Lock:   %s\n"), lstr);
						printf(_("                  %s\n"), l2str);
						printf(_("          Access: %s\n"), accstr);
					}
				}
			}
		}
		if (!first) {
			printf("\n");
		}
	}
	if (opt_p) {
		u_int16_t iterHandle = 0;
		PHYSICAL_LOCK pl;
		PHYSICAL_LOCKS pls;
		int first = 1;

		while (ncp_ns_scan_physical_locks_by_file(conn, dir.volNumber, dir.DosDirNum, 0, &iterHandle, &pl, &pls) == 0) {
			if (pls.numRecords != 0) {
				time_t t;
				struct ncp_bindery_object o;
				
				if (first) {
				        printf(_("File Physical Locks:\n"));
					first = 0;
				}
				
				printf(_("    Connection: %u/%u"), pl.connNumber, pl.taskNumber);
				err = ncp_get_stations_logged_info(conn, pl.connNumber, &o, &t);
				if (!err) {
					printf(_(", "));
					doID(conn, o.object_id);
				}
				printf("\n");
				printf(_("    Range: 0x%08LX-0x%08LX\n"), pl.recordStart, pl.recordEnd);
			}
		}
		if (!first) {
			printf("\n");
		}
	}
}

static void listdir(NWCONN_HANDLE conn, nuint32 volume, nuint32 dirent, char* mask) {
	NWDIRLIST_HANDLE h;
	NWCCODE err;
	size_t maskl = strlen(mask);

	err = ncp_ns_search_init(conn, NW_NS_DOS, SA_ALL, 1,
		volume, dirent, NULL, 0,
		0, mask, maskl, img, &h);
	if (err) {
		fprintf(stderr, _("Unable to begin scandir: 0x%04X\n"), err);
		return;
	}
	while (1) {
		struct nw_info_struct3 info;
		
		info.len = 0;
		info.data = NULL;
		err = ncp_ns_search_next(h, &info, sizeof(info));
		if (err) {
			if (err != NWE_SERVER_FAILURE) {
				/* Non-standard error code */
				fprintf(stderr, _("Unexpected error in NextDir: %s\n"), strnwerror(err));
			}
			break;
		}
		dumpit(conn, &info);
		if (info.data)
			free(info.data);
	}
	ncp_ns_search_end(h);
}

static char volname[17];
static char dirpath[512];
static char fullpath[600];

static struct nw_info_struct2 dosNS2;

static void usage(void) {
  fprintf(stderr, _("nwdir [options] [path]\n"
		  "\n"
		  "      -d    List information about directory itself instead\n"
		  "            of directory content\n"
                  "      -l    List namespace informations\n"
		  "      -e    List extended attributes informations\n"
		  "      -v    Verbose listing\n"
		  "      -f    List connections using file\n"
		  "      -p    List physical locks on file\n"
		  "      -t    Technical - show values and their meaning\n"
		  "      -h    This help\n"
		  "      path  Path to list, may contain wildcards\n"
		  "\n"
		  "(c) 1998 Milan Vandrovec for ncpfs-2.0.12.8\n"));
}

static char dwild[200];

static void makewild(char* wildout, const char* wildin) {
  unsigned char c;

  while ((c=*wildin++)!=0) {
    switch (c) {
#ifdef N_PLAT_DOS
      case '.':
#endif
      case '*':
      case '?':*wildout++ = '\377';
#ifdef N_PLAT_DOS
	       *wildout++ = c | 0x80;
#else
	       *wildout++ = c;
#endif
	       break;
     case 0xFF:*wildout++ = 0xFF;
       default:*wildout++ =c;
	       break;
    }
  }
  *wildout=0;
}

int main(int argc, char* argv[]) {
	NWCCODE err;
	NWCONN_HANDLE conn;
	char* path;
	char* xpath;
	char* lst;
	const char* dpath;
	int wild;
	int opt_d = 0;
	int i;
	u_int16_t nwver;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	while ((i = getopt(argc, argv, "dltvefph")) != -1) {
		switch (i) {
			case 'd':	opt_d = 1; break;
			case 'l':	opt_l = 1; break;
			case 't':	opt_t = 1; break;
			case 'v':	opt_v = 1; break;
			case 'e':	opt_e = 1; break;
			case 'f':	opt_f = 1; break;
			case 'p':	opt_p = 1; break;
			case 'h':	usage(); return 123;
		}
	}
	i = optind;
	if (i < argc) {
		path = argv[i++];
	} else {
		path = (char*)".";
	}
	err = NWCallsInit(NULL, NULL);
	if (err) {
		fprintf(stderr, _("Unable to initialize: 0x%04X\n"), err);
		return 123;
	}
	xpath = path;
	lst = xpath;
	wild = 0;
	for (path=xpath; *path; path++) {
		switch (*path) {
#ifdef N_PLAT_DOS
			case '\\':
#endif
			case '/': lst=path; wild=0; break;
			case '?':
			case '*': wild=1; break;
			default:  break;
		}
	}
	if (wild) {
		if (lst == xpath) {
			makewild(dwild, lst);
			dpath = ".";
		} else {
			makewild(dwild, lst+1);
			dpath = xpath;
			lst[1] = 0;
		}
		opt_d = 0;
	} else {
		dpath = xpath;
		strcpy(dwild, "\377*");
	}
	err = NWParsePath(dpath, NULL, &conn, volname, dirpath);
	if (err) {
		fprintf(stderr, _("Invalid path: %s\n"), strnwerror(err));
		return 123;
	}
	if (!conn) {
		fprintf(stderr, _("Specified path is not remote\n"));
		return 123;
	}
	xpath = dirpath;
	for (path=xpath; *path; path++) {
		*path = toupper(*path);
	}
	if (*volname) {
		sprintf(fullpath, "%s:%s", volname, dirpath);
	} else {
		strcpy(fullpath, dirpath);
	}
	printf(_("Directory %s\n"), fullpath);

	err = ncp_ns_obtain_entry_info(conn, NW_NS_DOS, SA_ALL, NCP_DIRSTYLE_NOHANDLE, 0, 0, 
			fullpath, NCP_PATH_STD, NW_NS_DOS, RIM_ATTRIBUTES | RIM_DIRECTORY, 
			&dosNS2, sizeof(dosNS2));
	if (err) {
		fprintf(stderr, _("Path does not exist: %s\n"), strnwerror(err));
		return 123;
	}
	img = RIM_NAME | RIM_ATTRIBUTES | RIM_DIRECTORY;
	if (opt_l) {
		img |= RIM_OWNING_NAMESPACE;
	}
	if (opt_v) {
		img |= RIM_SPACE_ALLOCATED | RIM_DATA_SIZE |
		       RIM_ARCHIVE | RIM_MODIFY | RIM_CREATION |
		       RIM_OWNING_NAMESPACE | RIM_RIGHTS;
	}
	err = NWGetFileServerVersion(conn, &nwver);
	if (err) {
		nwver = 0;
	}
	if (nwver >= 0x400) {
		img |= RIM_COMPRESSED_INFO;
		if (opt_v) {
			img |= RIM_DATASTREAM_SIZES | RIM_DATASTREAM_LOGICALS;
		}
	}
	if (nwver >= 0x40B) {
		if (opt_v) {
			img |= RIM_MAC_FINDER_INFO | RIM_EFFECTIVE_RIGHTS |
			       RIM_MAC_TIMES;
		}
	}
	if (nwver >= 0x500) {
		if (opt_v) {
			img |= RIM_LAST_ACCESS_TIME;
		}
	}
	if ((dosNS2.Attributes.Attributes & A_DIRECTORY) && !opt_d) {
		listdir(conn, dosNS2.Directory.volNumber, dosNS2.Directory.DosDirNum, dwild);
	} else {
		struct nw_info_struct3 info;

		info.len = 0;
		info.data = NULL;

		err = ncp_ns_obtain_entry_info(conn, NW_NS_DOS, SA_ALL, 
			1, dosNS2.Directory.volNumber, dosNS2.Directory.DosDirNum,
			NULL, 0,
			NW_NS_DOS,
			IM_ALL,
			&info, sizeof(info));
		if (err) {
			fprintf(stderr, _("Cannot retrieve info: %s\n"), strnwerror(err));
		} else {
			dumpit(conn, &info);
			free(info.data);
		}
	}
	return 0;
}
