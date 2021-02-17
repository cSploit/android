/* ldif.c - the ldif backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by Eric Stokes for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"
#include <stdio.h>
#include <ac/string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ac/dirent.h>
#include <fcntl.h>
#include <ac/errno.h>
#include <ac/unistd.h>
#include "slap.h"
#include "lutil.h"
#include "config.h"

struct ldif_tool {
	Entry	**entries;			/* collected by bi_tool_entry_first() */
	ID		elen;				/* length of entries[] array */
	ID		ecount;				/* number of entries */
	ID		ecurrent;			/* bi_tool_entry_next() position */
#	define	ENTRY_BUFF_INCREMENT 500 /* initial entries[] length */
	struct berval	*tl_base;
	int		tl_scope;
	Filter		*tl_filter;
};

/* Per-database data */
struct ldif_info {
	struct berval li_base_path;			/* database directory */
	struct ldif_tool li_tool;			/* for slap tools */
	/*
	 * Read-only LDAP requests readlock li_rdwr for filesystem input.
	 * Update requests first lock li_modop_mutex for filesystem I/O,
	 * and then writelock li_rdwr as well for filesystem output.
	 * This allows update requests to do callbacks that acquire
	 * read locks, e.g. access controls that inspect entries.
	 * (An alternative would be recursive read/write locks.)
	 */
	ldap_pvt_thread_mutex_t	li_modop_mutex; /* serialize update requests */
	ldap_pvt_thread_rdwr_t	li_rdwr;	/* no other I/O when writing */
};

static int write_data( int fd, const char *spew, int len, int *save_errno );

#ifdef _WIN32
#define mkdir(a,b)	mkdir(a)
#define move_file(from, to) (!MoveFileEx(from, to, MOVEFILE_REPLACE_EXISTING))
#else
#define move_file(from, to) rename(from, to)
#endif
#define move_dir(from, to) rename(from, to)


#define LDIF	".ldif"
#define LDIF_FILETYPE_SEP	'.'			/* LDIF[0] */

/*
 * Unsafe/translated characters in the filesystem.
 *
 * LDIF_UNSAFE_CHAR(c) returns true if the character c is not to be used
 * in relative filenames, except it should accept '\\', '{' and '}' even
 * if unsafe.  The value should be a constant expression.
 *
 * If '\\' is unsafe, #define LDIF_ESCAPE_CHAR as a safe character.
 * If '{' and '}' are unsafe, #define IX_FSL/IX_FSR as safe characters.
 * (Not digits, '-' or '+'.  IX_FSL == IX_FSR is allowed.)
 *
 * Characters are escaped as LDIF_ESCAPE_CHAR followed by two hex digits,
 * except '\\' is replaced with LDIF_ESCAPE_CHAR and {} with IX_FS[LR].
 * Also some LDIF special chars are hex-escaped.
 *
 * Thus an LDIF filename is a valid normalized RDN (or suffix DN)
 * followed by ".ldif", except with '\\' replaced with LDIF_ESCAPE_CHAR.
 */

#ifndef _WIN32

/*
 * Unix/MacOSX version.  ':' vs '/' can cause confusion on MacOSX so we
 * escape both.  We escape them on Unix so both OS variants get the same
 * filenames.
 */
#define LDIF_ESCAPE_CHAR	'\\'
#define LDIF_UNSAFE_CHAR(c)	((c) == '/' || (c) == ':')

#else /* _WIN32 */

/* Windows version - Microsoft's list of unsafe characters, except '\\' */
#define LDIF_ESCAPE_CHAR	'^'			/* Not '\\' (unsafe on Windows) */
#define LDIF_UNSAFE_CHAR(c)	\
	((c) == '/' || (c) == ':' || \
	 (c) == '<' || (c) == '>' || (c) == '"' || \
	 (c) == '|' || (c) == '?' || (c) == '*')

#endif /* !_WIN32 */

/*
 * Left and Right "{num}" prefix to ordered RDNs ("olcDatabase={1}bdb").
 * IX_DN* are for LDAP RDNs, IX_FS* for their .ldif filenames.
 */
#define IX_DNL	'{'
#define	IX_DNR	'}'
#ifndef IX_FSL
#define	IX_FSL	IX_DNL
#define IX_FSR	IX_DNR
#endif

/*
 * Test for unsafe chars, as well as chars handled specially by back-ldif:
 * - If the escape char is not '\\', it must itself be escaped.  Otherwise
 *   '\\' and the escape char would map to the same character.
 * - Escape the '.' in ".ldif", so the directory for an RDN that actually
 *   ends with ".ldif" can not conflict with a file of the same name.  And
 *   since some OSes/programs choke on multiple '.'s, escape all of them.
 * - If '{' and '}' are translated to some other characters, those
 *   characters must in turn be escaped when they occur in an RDN.
 */
#ifndef LDIF_NEED_ESCAPE
#define	LDIF_NEED_ESCAPE(c) \
	((LDIF_UNSAFE_CHAR(c)) || \
	 LDIF_MAYBE_UNSAFE(c, LDIF_ESCAPE_CHAR) || \
	 LDIF_MAYBE_UNSAFE(c, LDIF_FILETYPE_SEP) || \
	 LDIF_MAYBE_UNSAFE(c, IX_FSL) || \
	 (IX_FSR != IX_FSL && LDIF_MAYBE_UNSAFE(c, IX_FSR)))
#endif
/*
 * Helper macro for LDIF_NEED_ESCAPE(): Treat character x as unsafe if
 * back-ldif does not already treat is specially.
 */
#define LDIF_MAYBE_UNSAFE(c, x) \
	(!(LDIF_UNSAFE_CHAR(x) || (x) == '\\' || (x) == IX_DNL || (x) == IX_DNR) \
	 && (c) == (x))

/* Collect other "safe char" tests here, until someone needs a fix. */
enum {
	eq_unsafe = LDIF_UNSAFE_CHAR('='),
	safe_filenames = STRLENOF("" LDAP_DIRSEP "") == 1 && !(
		LDIF_UNSAFE_CHAR('-') || /* for "{-1}frontend" in bconfig.c */
		LDIF_UNSAFE_CHAR(LDIF_ESCAPE_CHAR) ||
		LDIF_UNSAFE_CHAR(IX_FSL) || LDIF_UNSAFE_CHAR(IX_FSR))
};
/* Sanity check: Try to force a compilation error if !safe_filenames */
typedef struct {
	int assert_safe_filenames : safe_filenames ? 2 : -2;
} assert_safe_filenames[safe_filenames ? 2 : -2];


static ConfigTable ldifcfg[] = {
	{ "directory", "dir", 2, 2, 0, ARG_BERVAL|ARG_OFFSET,
		(void *)offsetof(struct ldif_info, li_base_path),
		"( OLcfgDbAt:0.1 NAME 'olcDbDirectory' "
			"DESC 'Directory for database content' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs ldifocs[] = {
	{ "( OLcfgDbOc:2.1 "
		"NAME 'olcLdifConfig' "
		"DESC 'LDIF backend configuration' "
		"SUP olcDatabaseConfig "
		"MUST ( olcDbDirectory ) )", Cft_Database, ldifcfg },
	{ NULL, 0, NULL }
};


/*
 * Handle file/directory names.
 */

/* Set *res = LDIF filename path for the normalized DN */
static int
ndn2path( Operation *op, struct berval *dn, struct berval *res, int empty_ok )
{
	BackendDB *be = op->o_bd;
	struct ldif_info *li = (struct ldif_info *) be->be_private;
	struct berval *suffixdn = &be->be_nsuffix[0];
	const char *start, *end, *next, *p;
	char ch, *ptr;
	ber_len_t len;
	static const char hex[] = "0123456789ABCDEF";

	assert( dn != NULL );
	assert( !BER_BVISNULL( dn ) );
	assert( suffixdn != NULL );
	assert( !BER_BVISNULL( suffixdn ) );
	assert( dnIsSuffix( dn, suffixdn ) );

	if ( dn->bv_len == 0 && !empty_ok ) {
		return LDAP_UNWILLING_TO_PERFORM;
	}

	start = dn->bv_val;
	end = start + dn->bv_len;

	/* Room for dir, dirsep, dn, LDIF, "\hexpair"-escaping of unsafe chars */
	len = li->li_base_path.bv_len + dn->bv_len + (1 + STRLENOF( LDIF ));
	for ( p = start; p < end; ) {
		ch = *p++;
		if ( LDIF_NEED_ESCAPE( ch ) )
			len += 2;
	}
	res->bv_val = ch_malloc( len + 1 );

	ptr = lutil_strcopy( res->bv_val, li->li_base_path.bv_val );
	for ( next = end - suffixdn->bv_len; end > start; end = next ) {
		/* Set p = start of DN component, next = &',' or start of DN */
		while ( (p = next) > start ) {
			--next;
			if ( DN_SEPARATOR( *next ) )
				break;
		}
		/* Append <dirsep> <p..end-1: RDN or database-suffix> */
		for ( *ptr++ = LDAP_DIRSEP[0]; p < end; *ptr++ = ch ) {
			ch = *p++;
			if ( LDIF_ESCAPE_CHAR != '\\' && ch == '\\' ) {
				ch = LDIF_ESCAPE_CHAR;
			} else if ( IX_FSL != IX_DNL && ch == IX_DNL ) {
				ch = IX_FSL;
			} else if ( IX_FSR != IX_DNR && ch == IX_DNR ) {
				ch = IX_FSR;
			} else if ( LDIF_NEED_ESCAPE( ch ) ) {
				*ptr++ = LDIF_ESCAPE_CHAR;
				*ptr++ = hex[(ch & 0xFFU) >> 4];
				ch = hex[ch & 0x0FU];
			}
		}
	}
	ptr = lutil_strcopy( ptr, LDIF );
	res->bv_len = ptr - res->bv_val;

	assert( res->bv_len <= len );

	return LDAP_SUCCESS;
}

/*
 * *dest = dupbv(<dir + LDAP_DIRSEP>), plus room for <more>-sized filename.
 * Return pointer past the dirname.
 */
static char *
fullpath_alloc( struct berval *dest, const struct berval *dir, ber_len_t more )
{
	char *s = SLAP_MALLOC( dir->bv_len + more + 2 );

	dest->bv_val = s;
	if ( s == NULL ) {
		dest->bv_len = 0;
		Debug( LDAP_DEBUG_ANY, "back-ldif: out of memory\n", 0, 0, 0 );
	} else {
		s = lutil_strcopy( dest->bv_val, dir->bv_val );
		*s++ = LDAP_DIRSEP[0];
		*s = '\0';
		dest->bv_len = s - dest->bv_val;
	}
	return s;
}

/*
 * Append filename to fullpath_alloc() dirname or replace previous filename.
 * dir_end = fullpath_alloc() return value.
 */
#define FILL_PATH(fpath, dir_end, filename) \
	((fpath)->bv_len = lutil_strcopy(dir_end, filename) - (fpath)->bv_val)


/* .ldif entry filename length <-> subtree dirname length. */
#define ldif2dir_len(bv)  ((bv).bv_len -= STRLENOF(LDIF))
#define dir2ldif_len(bv)  ((bv).bv_len += STRLENOF(LDIF))
/* .ldif entry filename <-> subtree dirname, both with dirname length. */
#define ldif2dir_name(bv) ((bv).bv_val[(bv).bv_len] = '\0')
#define dir2ldif_name(bv) ((bv).bv_val[(bv).bv_len] = LDIF_FILETYPE_SEP)

/* Get the parent directory path, plus the LDIF suffix overwritten by a \0. */
static int
get_parent_path( struct berval *dnpath, struct berval *res )
{
	ber_len_t i = dnpath->bv_len;

	while ( i > 0 && dnpath->bv_val[ --i ] != LDAP_DIRSEP[0] ) ;
	if ( res == NULL ) {
		res = dnpath;
	} else {
		res->bv_val = SLAP_MALLOC( i + 1 + STRLENOF(LDIF) );
		if ( res->bv_val == NULL )
			return LDAP_OTHER;
		AC_MEMCPY( res->bv_val, dnpath->bv_val, i );
	}
	res->bv_len = i;
	strcpy( res->bv_val + i, LDIF );
	res->bv_val[i] = '\0';
	return LDAP_SUCCESS;
}

/* Make temporary filename pattern for mkstemp() based on dnpath. */
static char *
ldif_tempname( const struct berval *dnpath )
{
	static const char suffix[] = ".XXXXXX";
	ber_len_t len = dnpath->bv_len - STRLENOF( LDIF );
	char *name = SLAP_MALLOC( len + sizeof( suffix ) );

	if ( name != NULL ) {
		AC_MEMCPY( name, dnpath->bv_val, len );
		strcpy( name + len, suffix );
	}
	return name;
}

/* CRC-32 table for the polynomial:
 * x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.
 *
 * As used by zlib
 */

static const ber_uint_t crctab[256] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};

#define CRC1	crc = crctab[(crc ^ *buf++) & 0xff] ^ (crc >> 8)
#define CRC8	CRC1; CRC1; CRC1; CRC1; CRC1; CRC1; CRC1; CRC1
unsigned int
crc32(const void *vbuf, int len)
{
	const unsigned char	*buf = vbuf;
	ber_uint_t		crc = 0xffffffff;
	int				i;

	while (len > 7) {
		CRC8;
		len -= 8;
	}
	while (len) {
		CRC1;
		len--;
	}

	return crc ^ 0xffffffff;
}

/*
 * Read a file, or stat() it if datap == NULL.  Allocate and fill *datap.
 * Return LDAP_SUCCESS, LDAP_NO_SUCH_OBJECT (no such file), or another error.
 */
static int
ldif_read_file( const char *path, char **datap )
{
	int rc = LDAP_SUCCESS, fd, len;
	int res = -1;	/* 0:success, <0:error, >0:file too big/growing. */
	struct stat st;
	char *data = NULL, *ptr = NULL;
	const char *msg;

	if ( datap == NULL ) {
		res = stat( path, &st );
		goto done;
	}
	fd = open( path, O_RDONLY );
	if ( fd >= 0 ) {
		if ( fstat( fd, &st ) == 0 ) {
			if ( st.st_size > INT_MAX - 2 ) {
				res = 1;
			} else {
				len = st.st_size + 1; /* +1 detects file size > st.st_size */
				*datap = data = ptr = SLAP_MALLOC( len + 1 );
				if ( ptr != NULL ) {
					while ( len && (res = read( fd, ptr, len )) ) {
						if ( res > 0 ) {
							len -= res;
							ptr += res;
						} else if ( errno != EINTR ) {
							break;
						}
					}
					*ptr = '\0';
				}
			}
		}
		if ( close( fd ) < 0 )
			res = -1;
	}

 done:
	if ( res == 0 ) {
#ifdef LDAP_DEBUG
		msg = "entry file exists";
		if ( datap ) {
			msg = "read entry file";
			len = ptr - data;
			ptr = strstr( data, "\n# CRC32" );
			if (!ptr) {
				msg = "read entry file without checksum";
			} else {
				unsigned int crc1 = 0, crc2 = 1;
				if ( sscanf( ptr + 9, "%08x", &crc1) == 1) {
					ptr = strchr(ptr+1, '\n');
					if ( ptr ) {
						ptr++;
						len -= (ptr - data);
						crc2 = crc32( ptr, len );
					}
				}
				if ( crc1 != crc2 ) {
					Debug( LDAP_DEBUG_ANY, "ldif_read_file: checksum error on \"%s\"\n",
						path, 0, 0 );
					return rc;
				}
			}
		}
		Debug( LDAP_DEBUG_TRACE, "ldif_read_file: %s: \"%s\"\n", msg, path, 0 );
#endif /* LDAP_DEBUG */
	} else {
		if ( res < 0 && errno == ENOENT ) {
			Debug( LDAP_DEBUG_TRACE, "ldif_read_file: "
				"no entry file \"%s\"\n", path, 0, 0 );
			rc = LDAP_NO_SUCH_OBJECT;
		} else {
			msg = res < 0 ? STRERROR( errno ) : "bad stat() size";
			Debug( LDAP_DEBUG_ANY, "ldif_read_file: %s for \"%s\"\n",
				msg, path, 0 );
			rc = LDAP_OTHER;
		}
		if ( data != NULL )
			SLAP_FREE( data );
	}
	return rc;
}

/*
 * return nonnegative for success or -1 for error
 * do not return numbers less than -1
 */
static int
spew_file( int fd, const char *spew, int len, int *save_errno )
{
	int writeres;
#define HEADER	"# AUTO-GENERATED FILE - DO NOT EDIT!! Use ldapmodify.\n"
	char header[sizeof(HEADER "# CRC32 12345678\n")];

	sprintf(header, HEADER "# CRC32 %08x\n", crc32(spew, len));
	writeres = write_data(fd, header, sizeof(header)-1, save_errno);
	return writeres < 0 ? writeres : write_data(fd, spew, len, save_errno);
}

static int
write_data( int fd, const char *spew, int len, int *save_errno )
{
	int writeres = 0;
	while(len > 0) {
		writeres = write(fd, spew, len);
		if(writeres == -1) {
			*save_errno = errno;
			if (*save_errno != EINTR)
				break;
		}
		else {
			spew += writeres;
			len -= writeres;
		}
	}
	return writeres;
}

/* Write an entry LDIF file.  Create parentdir first if non-NULL. */
static int
ldif_write_entry(
	Operation *op,
	Entry *e,
	const struct berval *path,
	const char *parentdir,
	const char **text )
{
	int rc = LDAP_OTHER, res, save_errno = 0;
	int fd, entry_length;
	char *entry_as_string, *tmpfname;

	if ( op->o_abandon )
		return SLAPD_ABANDON;

	if ( parentdir != NULL && mkdir( parentdir, 0750 ) < 0 ) {
		save_errno = errno;
		Debug( LDAP_DEBUG_ANY, "ldif_write_entry: %s \"%s\": %s\n",
			"cannot create parent directory",
			parentdir, STRERROR( save_errno ) );
		*text = "internal error (cannot create parent directory)";
		return rc;
	}

	tmpfname = ldif_tempname( path );
	fd = tmpfname == NULL ? -1 : mkstemp( tmpfname );
	if ( fd < 0 ) {
		save_errno = errno;
		Debug( LDAP_DEBUG_ANY, "ldif_write_entry: %s for \"%s\": %s\n",
			"cannot create file", e->e_dn, STRERROR( save_errno ) );
		*text = "internal error (cannot create file)";

	} else {
		ber_len_t dn_len = e->e_name.bv_len;
		struct berval rdn;

		/* Only save the RDN onto disk */
		dnRdn( &e->e_name, &rdn );
		if ( rdn.bv_len != dn_len ) {
			e->e_name.bv_val[rdn.bv_len] = '\0';
			e->e_name.bv_len = rdn.bv_len;
		}

		res = -2;
		ldap_pvt_thread_mutex_lock( &entry2str_mutex );
		entry_as_string = entry2str( e, &entry_length );
		if ( entry_as_string != NULL )
			res = spew_file( fd, entry_as_string, entry_length, &save_errno );
		ldap_pvt_thread_mutex_unlock( &entry2str_mutex );

		/* Restore full DN */
		if ( rdn.bv_len != dn_len ) {
			e->e_name.bv_val[rdn.bv_len] = ',';
			e->e_name.bv_len = dn_len;
		}

		if ( close( fd ) < 0 && res >= 0 ) {
			res = -1;
			save_errno = errno;
		}

		if ( res >= 0 ) {
			if ( move_file( tmpfname, path->bv_val ) == 0 ) {
				Debug( LDAP_DEBUG_TRACE, "ldif_write_entry: "
					"wrote entry \"%s\"\n", e->e_name.bv_val, 0, 0 );
				rc = LDAP_SUCCESS;
			} else {
				save_errno = errno;
				Debug( LDAP_DEBUG_ANY, "ldif_write_entry: "
					"could not put entry file for \"%s\" in place: %s\n",
					e->e_name.bv_val, STRERROR( save_errno ), 0 );
				*text = "internal error (could not put entry file in place)";
			}
		} else if ( res == -1 ) {
			Debug( LDAP_DEBUG_ANY, "ldif_write_entry: %s \"%s\": %s\n",
				"write error to", tmpfname, STRERROR( save_errno ) );
			*text = "internal error (write error to entry file)";
		}

		if ( rc != LDAP_SUCCESS ) {
			unlink( tmpfname );
		}
	}

	if ( tmpfname )
		SLAP_FREE( tmpfname );
	return rc;
}

/*
 * Read the entry at path, or if entryp==NULL just see if it exists.
 * pdn and pndn are the parent's DN and normalized DN, or both NULL.
 * Return an LDAP result code.
 */
static int
ldif_read_entry(
	Operation *op,
	const char *path,
	struct berval *pdn,
	struct berval *pndn,
	Entry **entryp,
	const char **text )
{
	int rc;
	Entry *entry;
	char *entry_as_string;
	struct berval rdn;

	/* TODO: Does slapd prevent Abandon of Bind as per rfc4511?
	 * If so we need not check for LDAP_REQ_BIND here.
	 */
	if ( op->o_abandon && op->o_tag != LDAP_REQ_BIND )
		return SLAPD_ABANDON;

	rc = ldif_read_file( path, entryp ? &entry_as_string : NULL );

	switch ( rc ) {
	case LDAP_SUCCESS:
		if ( entryp == NULL )
			break;
		*entryp = entry = str2entry( entry_as_string );
		SLAP_FREE( entry_as_string );
		if ( entry == NULL ) {
			rc = LDAP_OTHER;
			if ( text != NULL )
				*text = "internal error (cannot parse some entry file)";
			break;
		}
		if ( pdn == NULL || BER_BVISEMPTY( pdn ) )
			break;
		/* Append parent DN to DN from LDIF file */
		rdn = entry->e_name;
		build_new_dn( &entry->e_name, pdn, &rdn, NULL );
		SLAP_FREE( rdn.bv_val );
		rdn = entry->e_nname;
		build_new_dn( &entry->e_nname, pndn, &rdn, NULL );
		SLAP_FREE( rdn.bv_val );
		break;

	case LDAP_OTHER:
		if ( text != NULL )
			*text = entryp
				? "internal error (cannot read some entry file)"
				: "internal error (cannot stat some entry file)";
		break;
	}

	return rc;
}

/*
 * Read the operation's entry, or if entryp==NULL just see if it exists.
 * Return an LDAP result code.  May set *text to a message on failure.
 * If pathp is non-NULL, set it to the entry filename on success.
 */
static int
get_entry(
	Operation *op,
	Entry **entryp,
	struct berval *pathp,
	const char **text )
{
	int rc;
	struct berval path, pdn, pndn;

	dnParent( &op->o_req_dn, &pdn );
	dnParent( &op->o_req_ndn, &pndn );
	rc = ndn2path( op, &op->o_req_ndn, &path, 0 );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	rc = ldif_read_entry( op, path.bv_val, &pdn, &pndn, entryp, text );

	if ( rc == LDAP_SUCCESS && pathp != NULL ) {
		*pathp = path;
	} else {
		SLAP_FREE( path.bv_val );
	}
 done:
	return rc;
}


/*
 * RDN-named directory entry, with special handling of "attr={num}val" RDNs.
 * For sorting, filename "attr=val.ldif" is truncated to "attr="val\0ldif",
 * and filename "attr={num}val.ldif" to "attr={\0um}val.ldif".
 * Does not sort escaped chars correctly, would need to un-escape them.
 */
typedef struct bvlist {
	struct bvlist *next;
	char *trunc;	/* filename was truncated here */
	int  inum;		/* num from "attr={num}" in filename, or INT_MIN */
	char savech;	/* original char at *trunc */
	/* BVL_NAME(&bvlist) is the filename, allocated after the struct: */
#	define BVL_NAME(bvl)     ((char *) ((bvl) + 1))
#	define BVL_SIZE(namelen) (sizeof(bvlist) + (namelen) + 1)
} bvlist;

static int
ldif_send_entry( Operation *op, SlapReply *rs, Entry *e, int scope )
{
	int rc = LDAP_SUCCESS;

	if ( scope == LDAP_SCOPE_BASE || scope == LDAP_SCOPE_SUBTREE ) {
		if ( rs == NULL ) {
			/* Save the entry for tool mode */
			struct ldif_tool *tl =
				&((struct ldif_info *) op->o_bd->be_private)->li_tool;

			if ( tl->ecount >= tl->elen ) {
				/* Allocate/grow entries */
				ID elen = tl->elen ? tl->elen * 2 : ENTRY_BUFF_INCREMENT;
				Entry **entries = (Entry **) SLAP_REALLOC( tl->entries,
					sizeof(Entry *) * elen );
				if ( entries == NULL ) {
					Debug( LDAP_DEBUG_ANY,
						"ldif_send_entry: out of memory\n", 0, 0, 0 );
					rc = LDAP_OTHER;
					goto done;
				}
				tl->elen = elen;
				tl->entries = entries;
			}
			tl->entries[tl->ecount++] = e;
			return rc;
		}

		else if ( !get_manageDSAit( op ) && is_entry_referral( e ) ) {
			/* Send a continuation reference.
			 * (ldif_back_referrals() handles baseobject referrals.)
			 * Don't check the filter since it's only a candidate.
			 */
			BerVarray refs = get_entry_referrals( op, e );
			rs->sr_ref = referral_rewrite( refs, &e->e_name, NULL, scope );
			rs->sr_entry = e;
			rc = send_search_reference( op, rs );
			ber_bvarray_free( rs->sr_ref );
			ber_bvarray_free( refs );
			rs->sr_ref = NULL;
			rs->sr_entry = NULL;
		}

		else if ( test_filter( op, e, op->ors_filter ) == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = e;
			rs->sr_attrs = op->ors_attrs;
			/* Could set REP_ENTRY_MUSTBEFREED too for efficiency,
			 * but refraining lets us test unFREEable MODIFIABLE
			 * entries.  Like entries built on the stack.
			 */
			rs->sr_flags = REP_ENTRY_MODIFIABLE;
			rc = send_search_entry( op, rs );
			rs->sr_entry = NULL;
			rs->sr_attrs = NULL;
		}
	}

 done:
	entry_free( e );
	return rc;
}

/* Read LDIF directory <path> into <listp>.  Set *fname_maxlenp. */
static int
ldif_readdir(
	Operation *op,
	SlapReply *rs,
	const struct berval *path,
	bvlist **listp,
	ber_len_t *fname_maxlenp )
{
	int rc = LDAP_SUCCESS;
	DIR *dir_of_path;

	*listp = NULL;
	*fname_maxlenp = 0;

	dir_of_path = opendir( path->bv_val );
	if ( dir_of_path == NULL ) {
		int save_errno = errno;
		struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
		int is_rootDSE = (path->bv_len == li->li_base_path.bv_len);

		/* Absent directory is OK (leaf entry), except the database dir */
		if ( is_rootDSE || save_errno != ENOENT ) {
			Debug( LDAP_DEBUG_ANY,
				"=> ldif_search_entry: failed to opendir \"%s\": %s\n",
				path->bv_val, STRERROR( save_errno ), 0 );
			rc = LDAP_OTHER;
			if ( rs != NULL )
				rs->sr_text =
					save_errno != ENOENT ? "internal error (bad directory)"
					: !is_rootDSE ? "internal error (missing directory)"
					: "internal error (database directory does not exist)";
		}

	} else {
		bvlist *ptr;
		struct dirent *dir;
		int save_errno = 0;

		while ( (dir = readdir( dir_of_path )) != NULL ) {
			size_t fname_len;
			bvlist *bvl, **prev;
			char *trunc, *idxp, *endp, *endp2;

			fname_len = strlen( dir->d_name );
			if ( fname_len < STRLENOF( "x=" LDIF )) /* min filename size */
				continue;
			if ( strcmp( dir->d_name + fname_len - STRLENOF(LDIF), LDIF ))
				continue;

			if ( *fname_maxlenp < fname_len )
				*fname_maxlenp = fname_len;

			bvl = SLAP_MALLOC( BVL_SIZE( fname_len ) );
			if ( bvl == NULL ) {
				rc = LDAP_OTHER;
				save_errno = errno;
				break;
			}
			strcpy( BVL_NAME( bvl ), dir->d_name );

			/* Make it sortable by ("attr=val" or <preceding {num}, num>) */
			trunc = BVL_NAME( bvl ) + fname_len - STRLENOF( LDIF );
			if ( (idxp = strchr( BVL_NAME( bvl ) + 2, IX_FSL )) != NULL &&
				 (endp = strchr( ++idxp, IX_FSR )) != NULL && endp > idxp &&
				 (eq_unsafe || idxp[-2] == '=' || endp + 1 == trunc) )
			{
				/* attr={n}val or bconfig.c's "pseudo-indexed" attr=val{n} */
				bvl->inum = strtol( idxp, &endp2, 10 );
				if ( endp2 == endp ) {
					trunc = idxp;
					goto truncate;
				}
			}
			bvl->inum = INT_MIN;
		truncate:
			bvl->trunc = trunc;
			bvl->savech = *trunc;
			*trunc = '\0';

			/* Insertion sort */
			for ( prev = listp; (ptr = *prev) != NULL; prev = &ptr->next ) {
				int cmp = strcmp( BVL_NAME( bvl ), BVL_NAME( ptr ));
				if ( cmp < 0 || (cmp == 0 && bvl->inum < ptr->inum) )
					break;
			}
			*prev = bvl;
			bvl->next = ptr;
		}

		if ( closedir( dir_of_path ) < 0 ) {
			save_errno = errno;
			rc = LDAP_OTHER;
			if ( rs != NULL )
				rs->sr_text = "internal error (bad directory)";
		}
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "ldif_search_entry: %s \"%s\": %s\n",
				"error reading directory", path->bv_val,
				STRERROR( save_errno ) );
		}
	}

	return rc;
}

/*
 * Send an entry, recursively search its children, and free or save it.
 * Return an LDAP result code.  Parameters:
 *  op, rs  operation and reply.  rs == NULL for slap tools.
 *  e       entry to search, or NULL for rootDSE.
 *  scope   scope for the part of the search from this entry.
 *  path    LDIF filename -- bv_len and non-directory part are overwritten.
 */
static int
ldif_search_entry(
	Operation *op,
	SlapReply *rs,
	Entry *e,
	int scope,
	struct berval *path )
{
	int rc = LDAP_SUCCESS;
	struct berval dn = BER_BVC( "" ), ndn = BER_BVC( "" );

	if ( scope != LDAP_SCOPE_BASE && e != NULL ) {
		/* Copy DN/NDN since we send the entry with REP_ENTRY_MODIFIABLE,
		 * which bconfig.c seems to need.  (TODO: see config_rename_one.)
		 */
		if ( ber_dupbv( &dn,  &e->e_name  ) == NULL ||
			 ber_dupbv( &ndn, &e->e_nname ) == NULL )
		{
			Debug( LDAP_DEBUG_ANY,
				"ldif_search_entry: out of memory\n", 0, 0, 0 );
			rc = LDAP_OTHER;
			goto done;
		}
	}

	/* Send the entry if appropriate, and free or save it */
	if ( e != NULL )
		rc = ldif_send_entry( op, rs, e, scope );

	/* Search the children */
	if ( scope != LDAP_SCOPE_BASE && rc == LDAP_SUCCESS ) {
		bvlist *list, *ptr;
		struct berval fpath;	/* becomes child pathname */
		char *dir_end;	/* will point past dirname in fpath */

		ldif2dir_len( *path );
		ldif2dir_name( *path );
		rc = ldif_readdir( op, rs, path, &list, &fpath.bv_len );

		if ( list != NULL ) {
			const char **text = rs == NULL ? NULL : &rs->sr_text;

			if ( scope == LDAP_SCOPE_ONELEVEL )
				scope = LDAP_SCOPE_BASE;
			else if ( scope == LDAP_SCOPE_SUBORDINATE )
				scope = LDAP_SCOPE_SUBTREE;

			/* Allocate fpath and fill in directory part */
			dir_end = fullpath_alloc( &fpath, path, fpath.bv_len );
			if ( dir_end == NULL )
				rc = LDAP_OTHER;

			do {
				ptr = list;

				if ( rc == LDAP_SUCCESS ) {
					*ptr->trunc = ptr->savech;
					FILL_PATH( &fpath, dir_end, BVL_NAME( ptr ));

					rc = ldif_read_entry( op, fpath.bv_val, &dn, &ndn,
						&e, text );
					switch ( rc ) {
					case LDAP_SUCCESS:
						rc = ldif_search_entry( op, rs, e, scope, &fpath );
						break;
					case LDAP_NO_SUCH_OBJECT:
						/* Only the search baseDN may produce noSuchObject. */
						rc = LDAP_OTHER;
						if ( rs != NULL )
							rs->sr_text = "internal error "
								"(did someone just remove an entry file?)";
						Debug( LDAP_DEBUG_ANY, "ldif_search_entry: "
							"file listed in parent directory does not exist: "
							"\"%s\"\n", fpath.bv_val, 0, 0 );
						break;
					}
				}

				list = ptr->next;
				SLAP_FREE( ptr );
			} while ( list != NULL );

			if ( !BER_BVISNULL( &fpath ) )
				SLAP_FREE( fpath.bv_val );
		}
	}

 done:
	if ( !BER_BVISEMPTY( &dn ) )
		ber_memfree( dn.bv_val );
	if ( !BER_BVISEMPTY( &ndn ) )
		ber_memfree( ndn.bv_val );
	return rc;
}

static int
search_tree( Operation *op, SlapReply *rs )
{
	int rc = LDAP_SUCCESS;
	Entry *e = NULL;
	struct berval path;
	struct berval pdn, pndn;

	(void) ndn2path( op, &op->o_req_ndn, &path, 1 );
	if ( !BER_BVISEMPTY( &op->o_req_ndn ) ) {
		/* Read baseObject */
		dnParent( &op->o_req_dn, &pdn );
		dnParent( &op->o_req_ndn, &pndn );
		rc = ldif_read_entry( op, path.bv_val, &pdn, &pndn, &e,
			rs == NULL ? NULL : &rs->sr_text );
	}
	if ( rc == LDAP_SUCCESS )
		rc = ldif_search_entry( op, rs, e, op->ors_scope, &path );

	ch_free( path.bv_val );
	return rc;
}


/*
 * Prepare to create or rename an entry:
 * Check that the entry does not already exist.
 * Check that the parent entry exists and can have subordinates,
 * unless need_dir is NULL or adding the suffix entry.
 *
 * Return an LDAP result code.  May set *text to a message on failure.
 * If success, set *dnpath to LDIF entry path and *need_dir to
 * (directory must be created ? dirname : NULL).
 */
static int
ldif_prepare_create(
	Operation *op,
	Entry *e,
	struct berval *dnpath,
	char **need_dir,
	const char **text )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	struct berval *ndn = &e->e_nname;
	struct berval ppath = BER_BVNULL;
	struct stat st;
	Entry *parent = NULL;
	int rc;

	if ( op->o_abandon )
		return SLAPD_ABANDON;

	rc = ndn2path( op, ndn, dnpath, 0 );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if ( stat( dnpath->bv_val, &st ) == 0 ) { /* entry .ldif file */
		rc = LDAP_ALREADY_EXISTS;

	} else if ( errno != ENOENT ) {
		Debug( LDAP_DEBUG_ANY,
			"ldif_prepare_create: cannot stat \"%s\": %s\n",
			dnpath->bv_val, STRERROR( errno ), 0 );
		rc = LDAP_OTHER;
		*text = "internal error (cannot check entry file)";

	} else if ( need_dir != NULL ) {
		*need_dir = NULL;
		rc = get_parent_path( dnpath, &ppath );
		/* If parent dir exists, so does parent .ldif:
		 * The directory gets created after and removed before the .ldif.
		 * Except with the database directory, which has no matching entry.
		 */
		if ( rc == LDAP_SUCCESS && stat( ppath.bv_val, &st ) < 0 ) {
			rc = errno == ENOENT && ppath.bv_len > li->li_base_path.bv_len
				? LDAP_NO_SUCH_OBJECT : LDAP_OTHER;
		}
		switch ( rc ) {
		case LDAP_NO_SUCH_OBJECT:
			/* No parent dir, check parent .ldif */
			dir2ldif_name( ppath );
			rc = ldif_read_entry( op, ppath.bv_val, NULL, NULL,
				(op->o_tag != LDAP_REQ_ADD || get_manageDSAit( op )
				 ? &parent : NULL),
				text );
			switch ( rc ) {
			case LDAP_SUCCESS:
				/* Check that parent is not a referral, unless
				 * ldif_back_referrals() already checked.
				 */
				if ( parent != NULL ) {
					int is_ref = is_entry_referral( parent );
					entry_free( parent );
					if ( is_ref ) {
						rc = LDAP_AFFECTS_MULTIPLE_DSAS;
						*text = op->o_tag == LDAP_REQ_MODDN
							? "newSuperior is a referral object"
							: "parent is a referral object";
						break;
					}
				}
				/* Must create parent directory. */
				ldif2dir_name( ppath );
				*need_dir = ppath.bv_val;
				break;
			case LDAP_NO_SUCH_OBJECT:
				*text = op->o_tag == LDAP_REQ_MODDN
					? "newSuperior object does not exist"
					: "parent does not exist";
				break;
			}
			break;
		case LDAP_OTHER:
			Debug( LDAP_DEBUG_ANY,
				"ldif_prepare_create: cannot stat \"%s\" parent dir: %s\n",
				ndn->bv_val, STRERROR( errno ), 0 );
			*text = "internal error (cannot stat parent dir)";
			break;
		}
		if ( *need_dir == NULL && ppath.bv_val != NULL )
			SLAP_FREE( ppath.bv_val );
	}

	if ( rc != LDAP_SUCCESS ) {
		SLAP_FREE( dnpath->bv_val );
		BER_BVZERO( dnpath );
	}
	return rc;
}

static int
apply_modify_to_entry(
	Entry *entry,
	Modifications *modlist,
	Operation *op,
	SlapReply *rs,
	char *textbuf )
{
	int rc = modlist ? LDAP_UNWILLING_TO_PERFORM : LDAP_SUCCESS;
	int is_oc = 0;
	Modification *mods;

	if (!acl_check_modlist(op, entry, modlist)) {
		return LDAP_INSUFFICIENT_ACCESS;
	}

	for (; modlist != NULL; modlist = modlist->sml_next) {
		mods = &modlist->sml_mod;

		if ( mods->sm_desc == slap_schema.si_ad_objectClass ) {
			is_oc = 1;
		}
		switch (mods->sm_op) {
		case LDAP_MOD_ADD:
			rc = modify_add_values(entry, mods,
				   get_permissiveModify(op),
				   &rs->sr_text, textbuf,
				   SLAP_TEXT_BUFLEN );
			break;

		case LDAP_MOD_DELETE:
			rc = modify_delete_values(entry, mods,
				get_permissiveModify(op),
				&rs->sr_text, textbuf,
				SLAP_TEXT_BUFLEN );
			break;

		case LDAP_MOD_REPLACE:
			rc = modify_replace_values(entry, mods,
				 get_permissiveModify(op),
				 &rs->sr_text, textbuf,
				 SLAP_TEXT_BUFLEN );
			break;

		case LDAP_MOD_INCREMENT:
			rc = modify_increment_values( entry,
				mods, get_permissiveModify(op),
				&rs->sr_text, textbuf,
				SLAP_TEXT_BUFLEN );
			break;

		case SLAP_MOD_SOFTADD:
			mods->sm_op = LDAP_MOD_ADD;
			rc = modify_add_values(entry, mods,
				   get_permissiveModify(op),
				   &rs->sr_text, textbuf,
				   SLAP_TEXT_BUFLEN );
			mods->sm_op = SLAP_MOD_SOFTADD;
			if (rc == LDAP_TYPE_OR_VALUE_EXISTS) {
				rc = LDAP_SUCCESS;
			}
			break;

		case SLAP_MOD_SOFTDEL:
			mods->sm_op = LDAP_MOD_DELETE;
			rc = modify_delete_values(entry, mods,
				   get_permissiveModify(op),
				   &rs->sr_text, textbuf,
				   SLAP_TEXT_BUFLEN );
			mods->sm_op = SLAP_MOD_SOFTDEL;
			if (rc == LDAP_NO_SUCH_ATTRIBUTE) {
				rc = LDAP_SUCCESS;
			}
			break;

		case SLAP_MOD_ADD_IF_NOT_PRESENT:
			if ( attr_find( entry->e_attrs, mods->sm_desc ) ) {
				rc = LDAP_SUCCESS;
				break;
			}
			mods->sm_op = LDAP_MOD_ADD;
			rc = modify_add_values(entry, mods,
				   get_permissiveModify(op),
				   &rs->sr_text, textbuf,
				   SLAP_TEXT_BUFLEN );
			mods->sm_op = SLAP_MOD_ADD_IF_NOT_PRESENT;
			break;
		}
		if(rc != LDAP_SUCCESS) break;
	}

	if ( rc == LDAP_SUCCESS ) {
		rs->sr_text = NULL; /* Needed at least with SLAP_MOD_SOFTADD */
		if ( is_oc ) {
			entry->e_ocflags = 0;
		}
		/* check that the entry still obeys the schema */
		rc = entry_schema_check( op, entry, NULL, 0, 0, NULL,
			  &rs->sr_text, textbuf, SLAP_TEXT_BUFLEN );
	}

	return rc;
}


static int
ldif_back_referrals( Operation *op, SlapReply *rs )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	struct berval path, dn = op->o_req_dn, ndn = op->o_req_ndn;
	ber_len_t min_dnlen;
	Entry *entry = NULL, **entryp;
	BerVarray ref;
	int rc;

	min_dnlen = op->o_bd->be_nsuffix[0].bv_len;
	if ( min_dnlen == 0 ) {
		/* Catch root DSE (empty DN), it is not a referral */
		min_dnlen = 1;
	}
	if ( ndn2path( op, &ndn, &path, 0 ) != LDAP_SUCCESS ) {
		return LDAP_SUCCESS;	/* Root DSE again */
	}

	entryp = get_manageDSAit( op ) ? NULL : &entry;
	ldap_pvt_thread_rdwr_rlock( &li->li_rdwr );

	for (;;) {
		dnParent( &dn, &dn );
		dnParent( &ndn, &ndn );
		rc = ldif_read_entry( op, path.bv_val, &dn, &ndn,
			entryp, &rs->sr_text );
		if ( rc != LDAP_NO_SUCH_OBJECT )
			break;

		rc = LDAP_SUCCESS;
		if ( ndn.bv_len < min_dnlen )
			break;
		(void) get_parent_path( &path, NULL );
		dir2ldif_name( path );
		entryp = &entry;
	}

	ldap_pvt_thread_rdwr_runlock( &li->li_rdwr );
	SLAP_FREE( path.bv_val );

	if ( entry != NULL ) {
		if ( is_entry_referral( entry ) ) {
			Debug( LDAP_DEBUG_TRACE,
				"ldif_back_referrals: tag=%lu target=\"%s\" matched=\"%s\"\n",
				(unsigned long) op->o_tag, op->o_req_dn.bv_val, entry->e_dn );

			ref = get_entry_referrals( op, entry );
			rs->sr_ref = referral_rewrite( ref, &entry->e_name, &op->o_req_dn,
				op->o_tag == LDAP_REQ_SEARCH ?
				op->ors_scope : LDAP_SCOPE_DEFAULT );
			ber_bvarray_free( ref );

			if ( rs->sr_ref != NULL ) {
				/* send referral */
				rc = rs->sr_err = LDAP_REFERRAL;
				rs->sr_matched = entry->e_dn;
				send_ldap_result( op, rs );
				ber_bvarray_free( rs->sr_ref );
				rs->sr_ref = NULL;
			} else {
				rc = LDAP_OTHER;
				rs->sr_text = "bad referral object";
			}
			rs->sr_matched = NULL;
		}

		entry_free( entry );
	}

	return rc;
}


/* LDAP operations */

static int
ldif_back_bind( Operation *op, SlapReply *rs )
{
	struct ldif_info *li;
	Attribute *a;
	AttributeDescription *password = slap_schema.si_ad_userPassword;
	int return_val;
	Entry *entry = NULL;

	switch ( be_rootdn_bind( op, rs ) ) {
	case SLAP_CB_CONTINUE:
		break;

	default:
		/* in case of success, front end will send result;
		 * otherwise, be_rootdn_bind() did */
		return rs->sr_err;
	}

	li = (struct ldif_info *) op->o_bd->be_private;
	ldap_pvt_thread_rdwr_rlock(&li->li_rdwr);
	return_val = get_entry(op, &entry, NULL, NULL);

	/* no object is found for them */
	if(return_val != LDAP_SUCCESS) {
		rs->sr_err = return_val = LDAP_INVALID_CREDENTIALS;
		goto return_result;
	}

	/* they don't have userpassword */
	if((a = attr_find(entry->e_attrs, password)) == NULL) {
		rs->sr_err = LDAP_INAPPROPRIATE_AUTH;
		return_val = 1;
		goto return_result;
	}

	/* authentication actually failed */
	if(slap_passwd_check(op, entry, a, &op->oq_bind.rb_cred,
			     &rs->sr_text) != 0) {
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		return_val = 1;
		goto return_result;
	}

	/* let the front-end send success */
	return_val = LDAP_SUCCESS;

 return_result:
	ldap_pvt_thread_rdwr_runlock(&li->li_rdwr);
	if(return_val != LDAP_SUCCESS)
		send_ldap_result( op, rs );
	if(entry != NULL)
		entry_free(entry);
	return return_val;
}

static int
ldif_back_search( Operation *op, SlapReply *rs )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;

	ldap_pvt_thread_rdwr_rlock(&li->li_rdwr);
	rs->sr_err = search_tree( op, rs );
	ldap_pvt_thread_rdwr_runlock(&li->li_rdwr);
	send_ldap_result(op, rs);

	return rs->sr_err;
}

static int
ldif_back_add( Operation *op, SlapReply *rs )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	Entry * e = op->ora_e;
	struct berval path;
	char *parentdir;
	char textbuf[SLAP_TEXT_BUFLEN];
	int rc;

	Debug( LDAP_DEBUG_TRACE, "ldif_back_add: \"%s\"\n", e->e_dn, 0, 0 );

	rc = entry_schema_check( op, e, NULL, 0, 1, NULL,
		&rs->sr_text, textbuf, sizeof( textbuf ) );
	if ( rc != LDAP_SUCCESS )
		goto send_res;

	rc = slap_add_opattrs( op, &rs->sr_text, textbuf, sizeof( textbuf ), 1 );
	if ( rc != LDAP_SUCCESS )
		goto send_res;

	ldap_pvt_thread_mutex_lock( &li->li_modop_mutex );

	rc = ldif_prepare_create( op, e, &path, &parentdir, &rs->sr_text );
	if ( rc == LDAP_SUCCESS ) {
		ldap_pvt_thread_rdwr_wlock( &li->li_rdwr );
		rc = ldif_write_entry( op, e, &path, parentdir, &rs->sr_text );
		ldap_pvt_thread_rdwr_wunlock( &li->li_rdwr );

		SLAP_FREE( path.bv_val );
		if ( parentdir != NULL )
			SLAP_FREE( parentdir );
	}

	ldap_pvt_thread_mutex_unlock( &li->li_modop_mutex );

 send_res:
	rs->sr_err = rc;
	Debug( LDAP_DEBUG_TRACE, "ldif_back_add: err: %d text: %s\n",
		rc, rs->sr_text ? rs->sr_text : "", 0 );
	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );
	rs->sr_text = NULL;	/* remove possible pointer to textbuf */
	return rs->sr_err;
}

static int
ldif_back_modify( Operation *op, SlapReply *rs )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	Modifications * modlst = op->orm_modlist;
	struct berval path;
	Entry *entry;
	char textbuf[SLAP_TEXT_BUFLEN];
	int rc;

	slap_mods_opattrs( op, &op->orm_modlist, 1 );

	ldap_pvt_thread_mutex_lock( &li->li_modop_mutex );

	rc = get_entry( op, &entry, &path, &rs->sr_text );
	if ( rc == LDAP_SUCCESS ) {
		rc = apply_modify_to_entry( entry, modlst, op, rs, textbuf );
		if ( rc == LDAP_SUCCESS ) {
			ldap_pvt_thread_rdwr_wlock( &li->li_rdwr );
			rc = ldif_write_entry( op, entry, &path, NULL, &rs->sr_text );
			ldap_pvt_thread_rdwr_wunlock( &li->li_rdwr );
		}

		entry_free( entry );
		SLAP_FREE( path.bv_val );
	}

	ldap_pvt_thread_mutex_unlock( &li->li_modop_mutex );

	rs->sr_err = rc;
	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );
	rs->sr_text = NULL;	/* remove possible pointer to textbuf */
	return rs->sr_err;
}

static int
ldif_back_delete( Operation *op, SlapReply *rs )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	struct berval path;
	int rc = LDAP_SUCCESS;

	if ( BER_BVISEMPTY( &op->o_csn )) {
		struct berval csn;
		char csnbuf[LDAP_PVT_CSNSTR_BUFSIZE];

		csn.bv_val = csnbuf;
		csn.bv_len = sizeof( csnbuf );
		slap_get_csn( op, &csn, 1 );
	}

	ldap_pvt_thread_mutex_lock( &li->li_modop_mutex );
	ldap_pvt_thread_rdwr_wlock( &li->li_rdwr );
	if ( op->o_abandon ) {
		rc = SLAPD_ABANDON;
		goto done;
	}

	rc = ndn2path( op, &op->o_req_ndn, &path, 0 );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	ldif2dir_len( path );
	ldif2dir_name( path );
	if ( rmdir( path.bv_val ) < 0 ) {
		switch ( errno ) {
		case ENOTEMPTY:
			rc = LDAP_NOT_ALLOWED_ON_NONLEAF;
			break;
		case ENOENT:
			/* is leaf, go on */
			break;
		default:
			rc = LDAP_OTHER;
			rs->sr_text = "internal error (cannot delete subtree directory)";
			break;
		}
	}

	if ( rc == LDAP_SUCCESS ) {
		dir2ldif_name( path );
		if ( unlink( path.bv_val ) < 0 ) {
			rc = LDAP_NO_SUCH_OBJECT;
			if ( errno != ENOENT ) {
				rc = LDAP_OTHER;
				rs->sr_text = "internal error (cannot delete entry file)";
			}
		}
	}

	if ( rc == LDAP_OTHER ) {
		Debug( LDAP_DEBUG_ANY, "ldif_back_delete: %s \"%s\": %s\n",
			"cannot delete", path.bv_val, STRERROR( errno ) );
	}

	SLAP_FREE( path.bv_val );
 done:
	ldap_pvt_thread_rdwr_wunlock( &li->li_rdwr );
	ldap_pvt_thread_mutex_unlock( &li->li_modop_mutex );
	rs->sr_err = rc;
	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );
	return rs->sr_err;
}


static int
ldif_move_entry(
	Operation *op,
	Entry *entry,
	int same_ndn,
	struct berval *oldpath,
	const char **text )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	struct berval newpath;
	char *parentdir = NULL, *trash;
	int rc, rename_res;

	if ( same_ndn ) {
		rc = LDAP_SUCCESS;
		newpath = *oldpath;
	} else {
		rc = ldif_prepare_create( op, entry, &newpath,
			op->orr_newSup ? &parentdir : NULL, text );
	}

	if ( rc == LDAP_SUCCESS ) {
		ldap_pvt_thread_rdwr_wlock( &li->li_rdwr );

		rc = ldif_write_entry( op, entry, &newpath, parentdir, text );
		if ( rc == LDAP_SUCCESS && !same_ndn ) {
			trash = oldpath->bv_val; /* will be .ldif file to delete */
			ldif2dir_len( newpath );
			ldif2dir_len( *oldpath );
			/* Move subdir before deleting old entry,
			 * so .ldif always exists if subdir does.
			 */
			ldif2dir_name( newpath );
			ldif2dir_name( *oldpath );
			rename_res = move_dir( oldpath->bv_val, newpath.bv_val );
			if ( rename_res != 0 && errno != ENOENT ) {
				rc = LDAP_OTHER;
				*text = "internal error (cannot move this subtree)";
				trash = newpath.bv_val;
			}

			/* Delete old entry, or if error undo change */
			for (;;) {
				dir2ldif_name( newpath );
				dir2ldif_name( *oldpath );
				if ( unlink( trash ) == 0 )
					break;
				if ( rc == LDAP_SUCCESS ) {
					/* Prepare to undo change and return failure */
					rc = LDAP_OTHER;
					*text = "internal error (cannot move this entry)";
					trash = newpath.bv_val;
					if ( rename_res != 0 )
						continue;
					/* First move subdirectory back */
					ldif2dir_name( newpath );
					ldif2dir_name( *oldpath );
					if ( move_dir( newpath.bv_val, oldpath->bv_val ) == 0 )
						continue;
				}
				*text = "added new but couldn't delete old entry!";
				break;
			}

			if ( rc != LDAP_SUCCESS ) {
				char s[128];
				snprintf( s, sizeof s, "%s (%s)", *text, STRERROR( errno ));
				Debug( LDAP_DEBUG_ANY,
					"ldif_move_entry: %s: \"%s\" -> \"%s\"\n",
					s, op->o_req_dn.bv_val, entry->e_dn );
			}
		}

		ldap_pvt_thread_rdwr_wunlock( &li->li_rdwr );
		if ( !same_ndn )
			SLAP_FREE( newpath.bv_val );
		if ( parentdir != NULL )
			SLAP_FREE( parentdir );
	}

	return rc;
}

static int
ldif_back_modrdn( Operation *op, SlapReply *rs )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	struct berval new_dn = BER_BVNULL, new_ndn = BER_BVNULL;
	struct berval p_dn, old_path;
	Entry *entry;
	char textbuf[SLAP_TEXT_BUFLEN];
	int rc, same_ndn;

	slap_mods_opattrs( op, &op->orr_modlist, 1 );

	ldap_pvt_thread_mutex_lock( &li->li_modop_mutex );

	rc = get_entry( op, &entry, &old_path, &rs->sr_text );
	if ( rc == LDAP_SUCCESS ) {
		/* build new dn, and new ndn for the entry */
		if ( op->oq_modrdn.rs_newSup != NULL ) {
			p_dn = *op->oq_modrdn.rs_newSup;
		} else {
			dnParent( &entry->e_name, &p_dn );
		}
		build_new_dn( &new_dn, &p_dn, &op->oq_modrdn.rs_newrdn, NULL );
		dnNormalize( 0, NULL, NULL, &new_dn, &new_ndn, NULL );
		same_ndn = !ber_bvcmp( &entry->e_nname, &new_ndn );
		ber_memfree_x( entry->e_name.bv_val, NULL );
		ber_memfree_x( entry->e_nname.bv_val, NULL );
		entry->e_name = new_dn;
		entry->e_nname = new_ndn;

		/* perform the modifications */
		rc = apply_modify_to_entry( entry, op->orr_modlist, op, rs, textbuf );
		if ( rc == LDAP_SUCCESS )
			rc = ldif_move_entry( op, entry, same_ndn, &old_path,
				&rs->sr_text );

		entry_free( entry );
		SLAP_FREE( old_path.bv_val );
	}

	ldap_pvt_thread_mutex_unlock( &li->li_modop_mutex );
	rs->sr_err = rc;
	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );
	rs->sr_text = NULL;	/* remove possible pointer to textbuf */
	return rs->sr_err;
}


/* Return LDAP_SUCCESS IFF we retrieve the specified entry. */
static int
ldif_back_entry_get(
	Operation *op,
	struct berval *ndn,
	ObjectClass *oc,
	AttributeDescription *at,
	int rw,
	Entry **e )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	struct berval op_dn = op->o_req_dn, op_ndn = op->o_req_ndn;
	int rc;

	assert( ndn != NULL );
	assert( !BER_BVISNULL( ndn ) );

	ldap_pvt_thread_rdwr_rlock( &li->li_rdwr );
	op->o_req_dn = *ndn;
	op->o_req_ndn = *ndn;
	rc = get_entry( op, e, NULL, NULL );
	op->o_req_dn = op_dn;
	op->o_req_ndn = op_ndn;
	ldap_pvt_thread_rdwr_runlock( &li->li_rdwr );

	if ( rc == LDAP_SUCCESS && oc && !is_entry_objectclass_or_sub( *e, oc ) ) {
		rc = LDAP_NO_SUCH_ATTRIBUTE;
		entry_free( *e );
		*e = NULL;
	}

	return rc;
}


/* Slap tools */

static int
ldif_tool_entry_open( BackendDB *be, int mode )
{
	struct ldif_tool *tl = &((struct ldif_info *) be->be_private)->li_tool;

	tl->ecurrent = 0;
	return 0;
}

static int
ldif_tool_entry_close( BackendDB *be )
{
	struct ldif_tool *tl = &((struct ldif_info *) be->be_private)->li_tool;
	Entry **entries = tl->entries;
	ID i;

	for ( i = tl->ecount; i--; )
		if ( entries[i] )
			entry_free( entries[i] );
	SLAP_FREE( entries );
	tl->entries = NULL;
	tl->ecount = tl->elen = 0;
	return 0;
}

static ID
ldif_tool_entry_next( BackendDB *be )
{
	struct ldif_tool *tl = &((struct ldif_info *) be->be_private)->li_tool;

	do {
		Entry *e = tl->entries[ tl->ecurrent ];

		if ( tl->ecurrent >= tl->ecount ) {
			return NOID;
		}

		++tl->ecurrent;

		if ( tl->tl_base && !dnIsSuffixScope( &e->e_nname, tl->tl_base, tl->tl_scope ) ) {
			continue;
		}

		if ( tl->tl_filter && test_filter( NULL, e, tl->tl_filter  ) != LDAP_COMPARE_TRUE ) {
			continue;
		}

		break;
	} while ( 1 );

	return tl->ecurrent;
}

static ID
ldif_tool_entry_first_x( BackendDB *be, struct berval *base, int scope, Filter *f )
{
	struct ldif_tool *tl = &((struct ldif_info *) be->be_private)->li_tool;

	tl->tl_base = base;
	tl->tl_scope = scope;
	tl->tl_filter = f;

	if ( tl->entries == NULL ) {
		Operation op = {0};

		op.o_bd = be;
		op.o_req_dn = *be->be_suffix;
		op.o_req_ndn = *be->be_nsuffix;
		op.ors_scope = LDAP_SCOPE_SUBTREE;
		if ( search_tree( &op, NULL ) != LDAP_SUCCESS ) {
			tl->ecurrent = tl->ecount; /* fail ldif_tool_entry_next() */
			return NOID; /* fail ldif_tool_entry_get() */
		}
	}
	return ldif_tool_entry_next( be );
}

static Entry *
ldif_tool_entry_get( BackendDB *be, ID id )
{
	struct ldif_tool *tl = &((struct ldif_info *) be->be_private)->li_tool;
	Entry *e = NULL;

	--id;
	if ( id < tl->ecount ) {
		e = tl->entries[id];
		tl->entries[id] = NULL;
	}
	return e;
}

static ID
ldif_tool_entry_put( BackendDB *be, Entry *e, struct berval *text )
{
	int rc;
	const char *errmsg = NULL;
	struct berval path;
	char *parentdir;
	Operation op = {0};

	op.o_bd = be;
	rc = ldif_prepare_create( &op, e, &path, &parentdir, &errmsg );
	if ( rc == LDAP_SUCCESS ) {
		rc = ldif_write_entry( &op, e, &path, parentdir, &errmsg );

		SLAP_FREE( path.bv_val );
		if ( parentdir != NULL )
			SLAP_FREE( parentdir );
		if ( rc == LDAP_SUCCESS )
			return 1;
	}

	if ( errmsg == NULL && rc != LDAP_OTHER )
		errmsg = ldap_err2string( rc );
	if ( errmsg != NULL )
		snprintf( text->bv_val, text->bv_len, "%s", errmsg );
	return NOID;
}


/* Setup */

static int
ldif_back_db_init( BackendDB *be, ConfigReply *cr )
{
	struct ldif_info *li;

	li = ch_calloc( 1, sizeof(struct ldif_info) );
	be->be_private = li;
	be->be_cf_ocs = ldifocs;
	ldap_pvt_thread_mutex_init( &li->li_modop_mutex );
	ldap_pvt_thread_rdwr_init( &li->li_rdwr );
	SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_ONE_SUFFIX;
	return 0;
}

static int
ldif_back_db_destroy( Backend *be, ConfigReply *cr )
{
	struct ldif_info *li = be->be_private;

	ch_free( li->li_base_path.bv_val );
	ldap_pvt_thread_rdwr_destroy( &li->li_rdwr );
	ldap_pvt_thread_mutex_destroy( &li->li_modop_mutex );
	free( be->be_private );
	return 0;
}

static int
ldif_back_db_open( Backend *be, ConfigReply *cr )
{
	struct ldif_info *li = (struct ldif_info *) be->be_private;
	if( BER_BVISEMPTY(&li->li_base_path)) {/* missing base path */
		Debug( LDAP_DEBUG_ANY, "missing base path for back-ldif\n", 0, 0, 0);
		return 1;
	}
	return 0;
}

int
ldif_back_initialize( BackendInfo *bi )
{
	static char *controls[] = {
		LDAP_CONTROL_MANAGEDSAIT,
		NULL
	};
	int rc;

	bi->bi_flags |=
		SLAP_BFLAG_INCREMENT |
		SLAP_BFLAG_REFERRALS;

	bi->bi_controls = controls;

	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = ldif_back_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = ldif_back_db_open;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = ldif_back_db_destroy;

	bi->bi_op_bind = ldif_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = ldif_back_search;
	bi->bi_op_compare = 0;
	bi->bi_op_modify = ldif_back_modify;
	bi->bi_op_modrdn = ldif_back_modrdn;
	bi->bi_op_add = ldif_back_add;
	bi->bi_op_delete = ldif_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = ldif_back_referrals;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	bi->bi_entry_get_rw = ldif_back_entry_get;

#if 0	/* NOTE: uncomment to completely disable access control */
	bi->bi_access_allowed = slap_access_always_allowed;
#endif

	bi->bi_tool_entry_open = ldif_tool_entry_open;
	bi->bi_tool_entry_close = ldif_tool_entry_close;
	bi->bi_tool_entry_first = backend_tool_entry_first;
	bi->bi_tool_entry_first_x = ldif_tool_entry_first_x;
	bi->bi_tool_entry_next = ldif_tool_entry_next;
	bi->bi_tool_entry_get = ldif_tool_entry_get;
	bi->bi_tool_entry_put = ldif_tool_entry_put;
	bi->bi_tool_entry_reindex = 0;
	bi->bi_tool_sync = 0;

	bi->bi_tool_dn2id_get = 0;
	bi->bi_tool_entry_modify = 0;

	bi->bi_cf_ocs = ldifocs;

	rc = config_register_schema( ldifcfg, ldifocs );
	if ( rc ) return rc;
	return 0;
}
