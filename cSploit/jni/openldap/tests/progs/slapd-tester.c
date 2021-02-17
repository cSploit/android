/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Kurt Spanier for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include "ac/stdlib.h"

#include "ac/ctype.h"
#include "ac/dirent.h"
#include "ac/param.h"
#include "ac/socket.h"
#include "ac/string.h"
#include "ac/unistd.h"
#include "ac/wait.h"


#include "ldap_defaults.h"
#include "lutil.h"

#include "ldap.h"
#include "ldap_pvt.h"
#include "lber_pvt.h"
#include "slapd-common.h"

#define SEARCHCMD		"slapd-search"
#define READCMD			"slapd-read"
#define ADDCMD			"slapd-addel"
#define MODRDNCMD		"slapd-modrdn"
#define MODIFYCMD		"slapd-modify"
#define BINDCMD			"slapd-bind"
#define MAXARGS      		100
#define MAXREQS			5000
#define LOOPS			100
#define OUTERLOOPS		"1"
#define RETRIES			"0"

#define TSEARCHFILE		"do_search.0"
#define TREADFILE		"do_read.0"
#define TADDFILE		"do_add."
#define TMODRDNFILE		"do_modrdn.0"
#define TMODIFYFILE		"do_modify.0"
#define TBINDFILE		"do_bind.0"

static char *get_file_name( char *dirname, char *filename );
static int  get_search_filters( char *filename, char *filters[], char *attrs[], char *bases[], LDAPURLDesc *luds[] );
static int  get_read_entries( char *filename, char *entries[], char *filters[] );
static void fork_child( char *prog, char **args );
static void	wait4kids( int nkidval );

static int      maxkids = 20;
static int      nkids;

#ifdef HAVE_WINSOCK
static HANDLE	*children;
static char argbuf[BUFSIZ];
#define	ArgDup(x) strdup(strcat(strcat(strcpy(argbuf,"\""),x),"\""))
#else
#define	ArgDup(x) strdup(x)
#endif

static void
usage( char *name, char opt )
{
	if ( opt ) {
		fprintf( stderr, "%s: unable to handle option \'%c\'\n\n",
			name, opt );
	}

	fprintf( stderr,
		"usage: %s "
		"-H <uri> | ([-h <host>] -p <port>) "
		"-D <manager> "
		"-w <passwd> "
		"-d <datadir> "
		"[-i <ignore>] "
		"[-j <maxchild>] "
		"[-l {<loops>|<type>=<loops>[,...]}] "
		"[-L <outerloops>] "
		"-P <progdir> "
		"[-r <maxretries>] "
		"[-t <delay>] "
		"[-C] "
		"[-F] "
		"[-I] "
		"[-N]\n",
		name );
	exit( EXIT_FAILURE );
}

int
main( int argc, char **argv )
{
	int		i, j;
	char		*uri = NULL;
	char		*host = "localhost";
	char		*port = NULL;
	char		*manager = NULL;
	char		*passwd = NULL;
	char		*dirname = NULL;
	char		*progdir = NULL;
	int		loops = LOOPS;
	char		*outerloops = OUTERLOOPS;
	char		*retries = RETRIES;
	char		*delay = "0";
	DIR		*datadir;
	struct dirent	*file;
	int		friendly = 0;
	int		chaserefs = 0;
	int		noattrs = 0;
	int		nobind = 0;
	int		noinit = 1;
	char		*ignore = NULL;
	/* search */
	char		*sfile = NULL;
	char		*sreqs[MAXREQS];
	char		*sattrs[MAXREQS];
	char		*sbase[MAXREQS];
	LDAPURLDesc	*slud[MAXREQS];
	int		snum = 0;
	char		*sargs[MAXARGS];
	int		sanum;
	int		sextra_args = 0;
	char		scmd[MAXPATHLEN];
	int		swamp = 0;
	char		swampopt[sizeof("-SSS")];
	/* static so that its address can be used in initializer below. */
	static char	sloops[LDAP_PVT_INTTYPE_CHARS(unsigned long)];
	/* read */
	char		*rfile = NULL;
	char		*rreqs[MAXREQS];
	int		rnum = 0;
	char		*rargs[MAXARGS];
	char		*rflts[MAXREQS];
	int		ranum;
	int		rextra_args = 0;
	char		rcmd[MAXPATHLEN];
	static char	rloops[LDAP_PVT_INTTYPE_CHARS(unsigned long)];
	/* addel */
	char		*afiles[MAXREQS];
	int		anum = 0;
	char		*aargs[MAXARGS];
	int		aanum;
	char		acmd[MAXPATHLEN];
	static char	aloops[LDAP_PVT_INTTYPE_CHARS(unsigned long)];
	/* modrdn */
	char		*nfile = NULL;
	char		*nreqs[MAXREQS];
	int		nnum = 0;
	char		*nargs[MAXARGS];
	int		nanum;
	char		ncmd[MAXPATHLEN];
	static char	nloops[LDAP_PVT_INTTYPE_CHARS(unsigned long)];
	/* modify */
	char		*mfile = NULL;
	char		*mreqs[MAXREQS];
	char		*mdn[MAXREQS];
	int		mnum = 0;
	char		*margs[MAXARGS];
	int		manum;
	char		mcmd[MAXPATHLEN];
	static char	mloops[LDAP_PVT_INTTYPE_CHARS(unsigned long)];
	/* bind */
	char		*bfile = NULL;
	char		*breqs[MAXREQS];
	char		*bcreds[MAXREQS];
	char		*battrs[MAXREQS];
	int		bnum = 0;
	char		*bargs[MAXARGS];
	int		banum;
	char		bcmd[MAXPATHLEN];
	static char	bloops[LDAP_PVT_INTTYPE_CHARS(unsigned long)];
	char		**bargs_extra = NULL;

	char		*friendlyOpt = NULL;
	int		pw_ask = 0;
	char		*pw_file = NULL;

	/* extra action to do after bind... */
	typedef struct extra_t {
		char		*action;
		struct extra_t	*next;
	}		extra_t;

	extra_t		*extra = NULL;
	int		nextra = 0;

	tester_init( "slapd-tester", TESTER_TESTER );

	sloops[0] = '\0';
	rloops[0] = '\0';
	aloops[0] = '\0';
	nloops[0] = '\0';
	mloops[0] = '\0';
	bloops[0] = '\0';

	while ( ( i = getopt( argc, argv, "AB:CD:d:FH:h:Ii:j:L:l:NP:p:r:St:Ww:y:" ) ) != EOF )
	{
		switch ( i ) {
		case 'A':
			noattrs++;
			break;

		case 'B': {
			char	**p,
				**b = ldap_str2charray( optarg, "," );
			extra_t	**epp;

			for ( epp = &extra; *epp; epp = &(*epp)->next )
				;

			for ( p = b; p[0]; p++ ) {
				*epp = calloc( 1, sizeof( extra_t ) );
				(*epp)->action = p[0];
				epp = &(*epp)->next;
				nextra++;
			}

			ldap_memfree( b );
			} break;

		case 'C':
			chaserefs++;
			break;

		case 'D':		/* slapd manager */
			manager = ArgDup( optarg );
			break;

		case 'd':		/* data directory */
			dirname = strdup( optarg );
			break;

		case 'F':
			friendly++;
			break;

		case 'H':		/* slapd uri */
			uri = strdup( optarg );
			break;

		case 'h':		/* slapd host */
			host = strdup( optarg );
			break;

		case 'I':
			noinit = 0;
			break;

		case 'i':
			ignore = optarg;
			break;

		case 'j':		/* the number of parallel clients */
			if ( lutil_atoi( &maxkids, optarg ) != 0 ) {
				usage( argv[0], 'j' );
			}
			break;

		case 'l':		/* the number of loops per client */
			if ( !isdigit( (unsigned char) optarg[0] ) ) {
				char	**p,
					**l = ldap_str2charray( optarg, "," );

				for ( p = l; p[0]; p++) {
					struct {
						struct berval	type;
						char		*buf;
					} types[] = {
						{ BER_BVC( "add=" ),	aloops },
						{ BER_BVC( "bind=" ),	bloops },
						{ BER_BVC( "modify=" ),	mloops },
						{ BER_BVC( "modrdn=" ),	nloops },
						{ BER_BVC( "read=" ),	rloops },
						{ BER_BVC( "search=" ),	sloops },
						{ BER_BVNULL,		NULL }
					};
					int	c, n;

					for ( c = 0; types[c].type.bv_val; c++ ) {
						if ( strncasecmp( p[0], types[c].type.bv_val, types[c].type.bv_len ) == 0 ) {
							break;
						}
					}

					if ( types[c].type.bv_val == NULL ) {
						usage( argv[0], 'l' );
					}

					if ( lutil_atoi( &n, &p[0][types[c].type.bv_len] ) != 0 ) {
						usage( argv[0], 'l' );
					}

					snprintf( types[c].buf, sizeof( aloops ), "%d", n );
				}

				ldap_charray_free( l );

			} else if ( lutil_atoi( &loops, optarg ) != 0 ) {
				usage( argv[0], 'l' );
			}
			break;

		case 'L':		/* the number of outerloops per client */
			outerloops = strdup( optarg );
			break;

		case 'N':
			nobind++;
			break;

		case 'P':		/* prog directory */
			progdir = strdup( optarg );
			break;

		case 'p':		/* the servers port number */
			port = strdup( optarg );
			break;

		case 'r':		/* the number of retries in case of error */
			retries = strdup( optarg );
			break;

		case 'S':
			swamp++;
			break;

		case 't':		/* the delay in seconds between each retry */
			delay = strdup( optarg );
			break;

		case 'w':		/* the managers passwd */
			passwd = ArgDup( optarg );
			memset( optarg, '*', strlen( optarg ) );
			break;

		case 'W':
			pw_ask++;
			break;

		case 'y':
			pw_file = optarg;
			break;

		default:
			usage( argv[0], '\0' );
			break;
		}
	}

	if (( dirname == NULL ) || ( port == NULL && uri == NULL ) ||
			( manager == NULL ) || ( passwd == NULL ) || ( progdir == NULL ))
	{
		usage( argv[0], '\0' );
	}

#ifdef HAVE_WINSOCK
	children = malloc( maxkids * sizeof(HANDLE) );
#endif
	/* get the file list */
	if ( ( datadir = opendir( dirname )) == NULL ) {
		fprintf( stderr, "%s: couldn't open data directory \"%s\".\n",
					argv[0], dirname );
		exit( EXIT_FAILURE );
	}

	/*  look for search, read, modrdn, and add/delete files */
	for ( file = readdir( datadir ); file; file = readdir( datadir )) {

		if ( !strcasecmp( file->d_name, TSEARCHFILE )) {
			sfile = get_file_name( dirname, file->d_name );
			continue;
		} else if ( !strcasecmp( file->d_name, TREADFILE )) {
			rfile = get_file_name( dirname, file->d_name );
			continue;
		} else if ( !strcasecmp( file->d_name, TMODRDNFILE )) {
			nfile = get_file_name( dirname, file->d_name );
			continue;
		} else if ( !strcasecmp( file->d_name, TMODIFYFILE )) {
			mfile = get_file_name( dirname, file->d_name );
			continue;
		} else if ( !strncasecmp( file->d_name, TADDFILE, strlen( TADDFILE ))
			&& ( anum < MAXREQS )) {
			afiles[anum++] = get_file_name( dirname, file->d_name );
			continue;
		} else if ( !strcasecmp( file->d_name, TBINDFILE )) {
			bfile = get_file_name( dirname, file->d_name );
			continue;
		}
	}

	closedir( datadir );

	if ( pw_ask ) {
		passwd = getpassphrase( _("Enter LDAP Password: ") );

	} else if ( pw_file ) {
		struct berval	pw;

		if ( lutil_get_filed_password( pw_file, &pw ) ) {
			exit( EXIT_FAILURE );
		}

		passwd = pw.bv_val;
	}

	if ( !sfile && !rfile && !nfile && !mfile && !bfile && !anum ) {
		fprintf( stderr, "no data files found.\n" );
		exit( EXIT_FAILURE );
	}

	/* look for search requests */
	if ( sfile ) {
		snum = get_search_filters( sfile, sreqs, sattrs, sbase, slud );
		if ( snum < 0 ) {
			fprintf( stderr,
				"unable to parse file \"%s\" line %d\n",
				sfile, -2*(snum + 1));
			exit( EXIT_FAILURE );
		}
	}

	/* look for read requests */
	if ( rfile ) {
		rnum = get_read_entries( rfile, rreqs, rflts );
		if ( rnum < 0 ) {
			fprintf( stderr,
				"unable to parse file \"%s\" line %d\n",
				rfile, -2*(rnum + 1) );
			exit( EXIT_FAILURE );
		}
	}

	/* look for modrdn requests */
	if ( nfile ) {
		nnum = get_read_entries( nfile, nreqs, NULL );
		if ( nnum < 0 ) {
			fprintf( stderr,
				"unable to parse file \"%s\" line %d\n",
				nfile, -2*(nnum + 1) );
			exit( EXIT_FAILURE );
		}
	}

	/* look for modify requests */
	if ( mfile ) {
		mnum = get_search_filters( mfile, mreqs, NULL, mdn, NULL );
		if ( mnum < 0 ) {
			fprintf( stderr,
				"unable to parse file \"%s\" line %d\n",
				mfile, -2*(mnum + 1) );
			exit( EXIT_FAILURE );
		}
	}

	/* look for bind requests */
	if ( bfile ) {
		bnum = get_search_filters( bfile, bcreds, battrs, breqs, NULL );
		if ( bnum < 0 ) {
			fprintf( stderr,
				"unable to parse file \"%s\" line %d\n",
				bfile, -2*(bnum + 1) );
			exit( EXIT_FAILURE );
		}
	}

	/* setup friendly option */
	switch ( friendly ) {
	case 0:
		break;

	case 1:
		friendlyOpt = "-F";
		break;

	default:
		/* NOTE: right now we don't need it more than twice */
	case 2:
		friendlyOpt = "-FF";
		break;
	}

	/* setup swamp option */
	if ( swamp ) {
		swampopt[0] = '-';
		if ( swamp > 3 ) swamp = 3;
		swampopt[swamp + 1] = '\0';
		for ( ; swamp-- > 0; ) swampopt[swamp + 1] = 'S';
	}

	/* setup loop options */
	if ( sloops[0] == '\0' ) snprintf( sloops, sizeof( sloops ), "%d", 10 * loops );
	if ( rloops[0] == '\0' ) snprintf( rloops, sizeof( rloops ), "%d", 20 * loops );
	if ( aloops[0] == '\0' ) snprintf( aloops, sizeof( aloops ), "%d", loops );
	if ( nloops[0] == '\0' ) snprintf( nloops, sizeof( nloops ), "%d", loops );
	if ( mloops[0] == '\0' ) snprintf( mloops, sizeof( mloops ), "%d", loops );
	if ( bloops[0] == '\0' ) snprintf( bloops, sizeof( bloops ), "%d", 20 * loops );

	/*
	 * generate the search clients
	 */

	sanum = 0;
	snprintf( scmd, sizeof scmd, "%s" LDAP_DIRSEP SEARCHCMD,
		progdir );
	sargs[sanum++] = scmd;
	if ( uri ) {
		sargs[sanum++] = "-H";
		sargs[sanum++] = uri;
	} else {
		sargs[sanum++] = "-h";
		sargs[sanum++] = host;
		sargs[sanum++] = "-p";
		sargs[sanum++] = port;
	}
	sargs[sanum++] = "-D";
	sargs[sanum++] = manager;
	sargs[sanum++] = "-w";
	sargs[sanum++] = passwd;
	sargs[sanum++] = "-l";
	sargs[sanum++] = sloops;
	sargs[sanum++] = "-L";
	sargs[sanum++] = outerloops;
	sargs[sanum++] = "-r";
	sargs[sanum++] = retries;
	sargs[sanum++] = "-t";
	sargs[sanum++] = delay;
	if ( friendly ) {
		sargs[sanum++] = friendlyOpt;
	}
	if ( chaserefs ) {
		sargs[sanum++] = "-C";
	}
	if ( noattrs ) {
		sargs[sanum++] = "-A";
	}
	if ( nobind ) {
		sargs[sanum++] = "-N";
	}
	if ( ignore ) {
		sargs[sanum++] = "-i";
		sargs[sanum++] = ignore;
	}
	if ( swamp ) {
		sargs[sanum++] = swampopt;
	}
	sargs[sanum++] = "-b";
	sargs[sanum++] = NULL;		/* will hold the search base */
	sargs[sanum++] = "-s";
	sargs[sanum++] = NULL;		/* will hold the search scope */
	sargs[sanum++] = "-f";
	sargs[sanum++] = NULL;		/* will hold the search request */

	sargs[sanum++] = NULL;
	sargs[sanum++] = NULL;		/* might hold the "attr" request */
	sextra_args += 2;

	sargs[sanum] = NULL;

	/*
	 * generate the read clients
	 */

	ranum = 0;
	snprintf( rcmd, sizeof rcmd, "%s" LDAP_DIRSEP READCMD,
		progdir );
	rargs[ranum++] = rcmd;
	if ( uri ) {
		rargs[ranum++] = "-H";
		rargs[ranum++] = uri;
	} else {
		rargs[ranum++] = "-h";
		rargs[ranum++] = host;
		rargs[ranum++] = "-p";
		rargs[ranum++] = port;
	}
	rargs[ranum++] = "-D";
	rargs[ranum++] = manager;
	rargs[ranum++] = "-w";
	rargs[ranum++] = passwd;
	rargs[ranum++] = "-l";
	rargs[ranum++] = rloops;
	rargs[ranum++] = "-L";
	rargs[ranum++] = outerloops;
	rargs[ranum++] = "-r";
	rargs[ranum++] = retries;
	rargs[ranum++] = "-t";
	rargs[ranum++] = delay;
	if ( friendly ) {
		rargs[ranum++] = friendlyOpt;
	}
	if ( chaserefs ) {
		rargs[ranum++] = "-C";
	}
	if ( noattrs ) {
		rargs[ranum++] = "-A";
	}
	if ( ignore ) {
		rargs[ranum++] = "-i";
		rargs[ranum++] = ignore;
	}
	if ( swamp ) {
		rargs[ranum++] = swampopt;
	}
	rargs[ranum++] = "-e";
	rargs[ranum++] = NULL;		/* will hold the read entry */

	rargs[ranum++] = NULL;
	rargs[ranum++] = NULL;		/* might hold the filter arg */
	rextra_args += 2;

	rargs[ranum] = NULL;

	/*
	 * generate the modrdn clients
	 */

	nanum = 0;
	snprintf( ncmd, sizeof ncmd, "%s" LDAP_DIRSEP MODRDNCMD,
		progdir );
	nargs[nanum++] = ncmd;
	if ( uri ) {
		nargs[nanum++] = "-H";
		nargs[nanum++] = uri;
	} else {
		nargs[nanum++] = "-h";
		nargs[nanum++] = host;
		nargs[nanum++] = "-p";
		nargs[nanum++] = port;
	}
	nargs[nanum++] = "-D";
	nargs[nanum++] = manager;
	nargs[nanum++] = "-w";
	nargs[nanum++] = passwd;
	nargs[nanum++] = "-l";
	nargs[nanum++] = nloops;
	nargs[nanum++] = "-L";
	nargs[nanum++] = outerloops;
	nargs[nanum++] = "-r";
	nargs[nanum++] = retries;
	nargs[nanum++] = "-t";
	nargs[nanum++] = delay;
	if ( friendly ) {
		nargs[nanum++] = friendlyOpt;
	}
	if ( chaserefs ) {
		nargs[nanum++] = "-C";
	}
	if ( ignore ) {
		nargs[nanum++] = "-i";
		nargs[nanum++] = ignore;
	}
	nargs[nanum++] = "-e";
	nargs[nanum++] = NULL;		/* will hold the modrdn entry */
	nargs[nanum] = NULL;
	
	/*
	 * generate the modify clients
	 */

	manum = 0;
	snprintf( mcmd, sizeof mcmd, "%s" LDAP_DIRSEP MODIFYCMD,
		progdir );
	margs[manum++] = mcmd;
	if ( uri ) {
		margs[manum++] = "-H";
		margs[manum++] = uri;
	} else {
		margs[manum++] = "-h";
		margs[manum++] = host;
		margs[manum++] = "-p";
		margs[manum++] = port;
	}
	margs[manum++] = "-D";
	margs[manum++] = manager;
	margs[manum++] = "-w";
	margs[manum++] = passwd;
	margs[manum++] = "-l";
	margs[manum++] = mloops;
	margs[manum++] = "-L";
	margs[manum++] = outerloops;
	margs[manum++] = "-r";
	margs[manum++] = retries;
	margs[manum++] = "-t";
	margs[manum++] = delay;
	if ( friendly ) {
		margs[manum++] = friendlyOpt;
	}
	if ( chaserefs ) {
		margs[manum++] = "-C";
	}
	if ( ignore ) {
		margs[manum++] = "-i";
		margs[manum++] = ignore;
	}
	margs[manum++] = "-e";
	margs[manum++] = NULL;		/* will hold the modify entry */
	margs[manum++] = "-a";;
	margs[manum++] = NULL;		/* will hold the ava */
	margs[manum] = NULL;

	/*
	 * generate the add/delete clients
	 */

	aanum = 0;
	snprintf( acmd, sizeof acmd, "%s" LDAP_DIRSEP ADDCMD,
		progdir );
	aargs[aanum++] = acmd;
	if ( uri ) {
		aargs[aanum++] = "-H";
		aargs[aanum++] = uri;
	} else {
		aargs[aanum++] = "-h";
		aargs[aanum++] = host;
		aargs[aanum++] = "-p";
		aargs[aanum++] = port;
	}
	aargs[aanum++] = "-D";
	aargs[aanum++] = manager;
	aargs[aanum++] = "-w";
	aargs[aanum++] = passwd;
	aargs[aanum++] = "-l";
	aargs[aanum++] = aloops;
	aargs[aanum++] = "-L";
	aargs[aanum++] = outerloops;
	aargs[aanum++] = "-r";
	aargs[aanum++] = retries;
	aargs[aanum++] = "-t";
	aargs[aanum++] = delay;
	if ( friendly ) {
		aargs[aanum++] = friendlyOpt;
	}
	if ( chaserefs ) {
		aargs[aanum++] = "-C";
	}
	if ( ignore ) {
		aargs[aanum++] = "-i";
		aargs[aanum++] = ignore;
	}
	aargs[aanum++] = "-f";
	aargs[aanum++] = NULL;		/* will hold the add data file */
	aargs[aanum] = NULL;

	/*
	 * generate the bind clients
	 */

	banum = 0;
	snprintf( bcmd, sizeof bcmd, "%s" LDAP_DIRSEP BINDCMD,
		progdir );
	bargs[banum++] = bcmd;
	if ( !noinit ) {
		bargs[banum++] = "-I";	/* init on each bind */
	}
	if ( uri ) {
		bargs[banum++] = "-H";
		bargs[banum++] = uri;
	} else {
		bargs[banum++] = "-h";
		bargs[banum++] = host;
		bargs[banum++] = "-p";
		bargs[banum++] = port;
	}
	bargs[banum++] = "-l";
	bargs[banum++] = bloops;
	bargs[banum++] = "-L";
	bargs[banum++] = outerloops;
#if 0
	bargs[banum++] = "-r";
	bargs[banum++] = retries;
	bargs[banum++] = "-t";
	bargs[banum++] = delay;
#endif
	if ( friendly ) {
		bargs[banum++] = friendlyOpt;
	}
	if ( chaserefs ) {
		bargs[banum++] = "-C";
	}
	if ( ignore ) {
		bargs[banum++] = "-i";
		bargs[banum++] = ignore;
	}
	if ( nextra ) {
		bargs[banum++] = "-B";
		bargs_extra = &bargs[banum++];
	}
	bargs[banum++] = "-D";
	bargs[banum++] = NULL;
	bargs[banum++] = "-w";
	bargs[banum++] = NULL;
	bargs[banum] = NULL;

#define	DOREQ(n,j) ((n) && ((maxkids > (n)) ? ((j) < maxkids ) : ((j) < (n))))

	for ( j = 0; j < MAXREQS; j++ ) {
		/* search */
		if ( DOREQ( snum, j ) ) {
			int	jj = j % snum;
			int	x = sanum - sextra_args;

			/* base */
			if ( sbase[jj] != NULL ) {
				sargs[sanum - 7] = sbase[jj];

			} else {
				sargs[sanum - 7] = slud[jj]->lud_dn;
			}

			/* scope */
			if ( slud[jj] != NULL ) {
				sargs[sanum - 5] = (char *)ldap_pvt_scope2str( slud[jj]->lud_scope );

			} else {
				sargs[sanum - 5] = "sub";
			}

			/* filter */
			if ( sreqs[jj] != NULL ) {
				sargs[sanum - 3] = sreqs[jj];

			} else if ( slud[jj]->lud_filter != NULL ) {
				sargs[sanum - 3] = slud[jj]->lud_filter;

			} else {
				sargs[sanum - 3] = "(objectClass=*)";
			}

			/* extras */
			sargs[x] = NULL;

			/* attr */
			if ( sattrs[jj] != NULL ) {
				sargs[x++] = "-a";
				sargs[x++] = sattrs[jj];
			}

			/* attrs */
			if ( slud[jj] != NULL && slud[jj]->lud_attrs != NULL ) {
				int	i;

				for ( i = 0; slud[jj]->lud_attrs[ i ] != NULL && x + i < MAXARGS - 1; i++ ) {
					sargs[x + i] = slud[jj]->lud_attrs[ i ];
				}
				sargs[x + i] = NULL;
			}

			fork_child( scmd, sargs );
		}

		/* read */
		if ( DOREQ( rnum, j ) ) {
			int	jj = j % rnum;
			int	x = ranum - rextra_args;

			rargs[ranum - 3] = rreqs[jj];
			if ( rflts[jj] != NULL ) {
				rargs[x++] = "-f";
				rargs[x++] = rflts[jj];
			}
			rargs[x] = NULL;
			fork_child( rcmd, rargs );
		}

		/* rename */
		if ( j < nnum ) {
			nargs[nanum - 1] = nreqs[j];
			fork_child( ncmd, nargs );
		}

		/* modify */
		if ( j < mnum ) {
			margs[manum - 3] = mdn[j];
			margs[manum - 1] = mreqs[j];
			fork_child( mcmd, margs );
		}

		/* add/delete */
		if ( j < anum ) {
			aargs[aanum - 1] = afiles[j];
			fork_child( acmd, aargs );
		}

		/* bind */
		if ( DOREQ( bnum, j ) ) {
			int	jj = j % bnum;

			if ( nextra ) {
				int	n = ((double)nextra)*rand()/(RAND_MAX + 1.0);
				extra_t	*e;

				for ( e = extra; n-- > 0; e = e->next )
					;
				*bargs_extra = e->action;
			}

			if ( battrs[jj] != NULL ) {
				bargs[banum - 3] = manager ? manager : "";
				bargs[banum - 1] = passwd ? passwd : "";

				bargs[banum + 0] = "-b";
				bargs[banum + 1] = breqs[jj];
				bargs[banum + 2] = "-f";
				bargs[banum + 3] = bcreds[jj];
				bargs[banum + 4] = "-a";
				bargs[banum + 5] = battrs[jj];
				bargs[banum + 6] = NULL;

			} else {
				bargs[banum - 3] = breqs[jj];
				bargs[banum - 1] = bcreds[jj];
				bargs[banum] = NULL;
			}

			fork_child( bcmd, bargs );
			bargs[banum] = NULL;
		}
	}

	wait4kids( -1 );

	exit( EXIT_SUCCESS );
}

static char *
get_file_name( char *dirname, char *filename )
{
	char buf[MAXPATHLEN];

	snprintf( buf, sizeof buf, "%s" LDAP_DIRSEP "%s",
		dirname, filename );
	return( strdup( buf ));
}


static int
get_search_filters( char *filename, char *filters[], char *attrs[], char *bases[], LDAPURLDesc *luds[] )
{
	FILE    *fp;
	int     filter = 0;

	if ( (fp = fopen( filename, "r" )) != NULL ) {
		char  line[BUFSIZ];

		while (( filter < MAXREQS ) && ( fgets( line, BUFSIZ, fp ))) {
			char	*nl;
			int	got_URL = 0;

			if (( nl = strchr( line, '\r' )) || ( nl = strchr( line, '\n' )))
				*nl = '\0';

			if ( luds ) luds[filter] = NULL;

			if ( luds && strncmp( line, "ldap:///", STRLENOF( "ldap:///" ) ) == 0 ) {
				LDAPURLDesc	*lud;

				got_URL = 1;
				bases[filter] = NULL;
				if ( ldap_url_parse( line, &lud ) != LDAP_URL_SUCCESS ) {
					filter = -filter - 1;
					break;
				}

				if ( lud->lud_dn == NULL || lud->lud_exts != NULL ) {
					filter = -filter - 1;
					ldap_free_urldesc( lud );
					break;
				}

				luds[filter] = lud;

			} else {
				bases[filter] = ArgDup( line );
			}
			if ( fgets( line, BUFSIZ, fp ) == NULL )
				*line = '\0';
			if (( nl = strchr( line, '\r' )) || ( nl = strchr( line, '\n' )))
				*nl = '\0';

			filters[filter] = ArgDup( line );
			if ( attrs ) {
				if ( filters[filter][0] == '+') {
					char	*sep = strchr( filters[filter], ':' );

					attrs[ filter ] = &filters[ filter ][ 1 ];
					if ( sep != NULL ) {
						sep[ 0 ] = '\0';
						/* NOTE: don't free this! */
						filters[ filter ] = &sep[ 1 ];
					}

				} else {
					attrs[ filter ] = NULL;
				}
			}
			filter++;

		}
		fclose( fp );
	}

	return filter;
}


static int
get_read_entries( char *filename, char *entries[], char *filters[] )
{
	FILE    *fp;
	int     entry = 0;

	if ( (fp = fopen( filename, "r" )) != NULL ) {
		char  line[BUFSIZ];

		while (( entry < MAXREQS ) && ( fgets( line, BUFSIZ, fp ))) {
			char *nl;

			if (( nl = strchr( line, '\r' )) || ( nl = strchr( line, '\n' )))
				*nl = '\0';
			if ( filters != NULL && line[0] == '+' ) {
				LDAPURLDesc	*lud;

				if ( ldap_url_parse( &line[1], &lud ) != LDAP_URL_SUCCESS ) {
					entry = -entry - 1;
					break;
				}

				if ( lud->lud_dn == NULL || lud->lud_dn[ 0 ] == '\0' ) {
					ldap_free_urldesc( lud );
					entry = -entry - 1;
					break;
				}

				entries[entry] = ArgDup( lud->lud_dn );

				if ( lud->lud_filter ) {
					filters[entry] = ArgDup( lud->lud_filter );

				} else {
					filters[entry] = ArgDup( "(objectClass=*)" );
				}
				ldap_free_urldesc( lud );

			} else {
				if ( filters != NULL )
					filters[entry] = NULL;

				entries[entry] = ArgDup( line );
			}

			entry++;

		}
		fclose( fp );
	}

	return( entry );
}

#ifndef HAVE_WINSOCK
static void
fork_child( char *prog, char **args )
{
	/* note: obscures global pid var; intended */
	pid_t	pid;

	wait4kids( maxkids );

	switch ( pid = fork() ) {
	case 0:		/* child */
#ifdef HAVE_EBCDIC
		/* The __LIBASCII execvp only handles ASCII "prog",
		 * we still need to translate the arg vec ourselves.
		 */
		{ char *arg2[MAXREQS];
		int i;

		for (i=0; args[i]; i++) {
			arg2[i] = ArgDup(args[i]);
			__atoe(arg2[i]);
		}
		arg2[i] = NULL;
		args = arg2; }
#endif
		execvp( prog, args );
		tester_perror( "execvp", NULL );
		{ int i;
			for (i=0; args[i]; i++);
			fprintf(stderr,"%d args\n", i);
			for (i=0; args[i]; i++)
				fprintf(stderr,"%d %s\n", i, args[i]);
		}

		exit( EXIT_FAILURE );
		break;

	case -1:	/* trouble */
		tester_perror( "fork", NULL );
		break;

	default:	/* parent */
		nkids++;
		break;
	}
}

static void
wait4kids( int nkidval )
{
	int		status;

	while ( nkids >= nkidval ) {
		pid_t pid = wait( &status );

		if ( WIFSTOPPED(status) ) {
			fprintf( stderr,
			    "stopping: child PID=%ld stopped with signal %d\n",
			    (long) pid, (int) WSTOPSIG(status) );

		} else if ( WIFSIGNALED(status) ) {
			fprintf( stderr, 
			    "stopping: child PID=%ld terminated with signal %d%s\n",
			    (long) pid, (int) WTERMSIG(status),
#ifdef WCOREDUMP
				WCOREDUMP(status) ? ", core dumped" : ""
#else
				""
#endif
				);
			exit( WEXITSTATUS(status)  );

		} else if ( WEXITSTATUS(status) != 0 ) {
			fprintf( stderr, 
			    "stopping: child PID=%ld exited with status %d\n",
			    (long) pid, (int) WEXITSTATUS(status) );
			exit( WEXITSTATUS(status) );

		} else {
			nkids--;
		}
	}
}
#else

static void
wait4kids( int nkidval )
{
	int rc, i;

	while ( nkids >= nkidval ) {
		rc = WaitForMultipleObjects( nkids, children, FALSE, INFINITE );
		for ( i=rc - WAIT_OBJECT_0; i<nkids-1; i++)
			children[i] = children[i+1];
		nkids--;
	}
}

static void
fork_child( char *prog, char **args )
{
	int rc;

	wait4kids( maxkids );

	rc = _spawnvp( _P_NOWAIT, prog, args );

	if ( rc == -1 ) {
		tester_perror( "_spawnvp", NULL );
	} else {
		children[nkids++] = (HANDLE)rc;
	}
}
#endif
