/* DO NOT EDIT: automatically built from dist/clib_port.in. */
/*
 * Minimum/maximum values for various types.
 */
#ifndef	UINT16_MAX			/* Maximum 16-bit unsigned. */
#define	UINT16_MAX	65535
#endif
#ifndef	UINT32_MAX			/* Maximum 32-bit unsigned. */
#define	UINT32_MAX	4294967295U
#endif

#ifndef	INT_MAX
#if SIZEOF_INT == 4
#define	INT_MAX		2147483647
#endif
#if SIZEOF_INT == 8
#define	INT_MAX		9223372036854775807
#endif
#endif

#ifndef	INT_MIN				/* minimum (signed) int value */
#define	INT_MIN		(-INT_MAX-1)
#endif

#ifndef	UINT_MAX			/* maximum (signed) int value */
#if SIZEOF_INT == 4
#define	UINT_MAX	4294967295U
#endif
#if SIZEOF_INT == 8
#define	UINT_MAX	18446744073709551615U
#endif
#endif

#ifndef	LONG_MAX			/* maximum (signed) long value */
#if SIZEOF_LONG == 4
#define	LONG_MAX	2147483647
#endif
#if SIZEOF_LONG == 8
#define	LONG_MAX	9223372036854775807L
#endif
#endif

#ifndef	LONG_MIN			/* minimum (signed) long value */
#define	LONG_MIN	(-LONG_MAX-1)
#endif

#ifndef	ULONG_MAX			/* maximum (unsigned) long value */
#if SIZEOF_LONG == 4
#define	ULONG_MAX	4294967295U
#endif
#if SIZEOF_LONG == 8
#define	ULONG_MAX	18446744073709551615UL
#endif
#endif

#if defined(HAVE_64BIT_TYPES)
/*
 * Override the system's 64-bit min/max constants.  AIX's 32-bit compiler can
 * handle 64-bit values, but the system's constants don't include the LL/ULL
 * suffix, and so can't be compiled using the 32-bit compiler.
 */
#undef	INT64_MAX
#undef	INT64_MIN
#undef	UINT64_MAX

#ifdef	DB_WIN32
#define	INT64_MAX	_I64_MAX
#define	INT64_MIN	_I64_MIN
#define	UINT64_MAX	_UI64_MAX
#else
#define	INT64_MAX	9223372036854775807LL
#define	INT64_MIN	(-INT64_MAX-1)
#define	UINT64_MAX	18446744073709551615ULL
#endif	/* DB_WIN32 */

#define	INT64_FMT	"%I64d"
#define	UINT64_FMT	"%I64u"
#endif	/* HAVE_64BIT_TYPES */

/*
 * Exit success/failure macros.
 */
#ifndef	HAVE_EXIT_SUCCESS
#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0
#endif

/*
 * File modes.
 */
#ifdef DB_WIN32
#ifndef S_IREAD				/* WinCE doesn't have S_IREAD. */
#define	S_IREAD		0
#endif
#ifndef S_IWRITE			/* WinCE doesn't have S_IWRITE. */
#define	S_IWRITE	0
#endif
#ifndef	S_IRUSR
#define	S_IRUSR		S_IREAD		/* R for owner */
#endif
#ifndef	S_IWUSR
#define	S_IWUSR		S_IWRITE	/* W for owner */
#endif
#ifndef	S_IXUSR
#define	S_IXUSR		0		/* X for owner */
#endif
#ifndef	S_IRGRP
#define	S_IRGRP		0		/* R for group */
#endif
#ifndef	S_IWGRP
#define	S_IWGRP		0		/* W for group */
#endif
#ifndef	S_IXGRP
#define	S_IXGRP		0		/* X for group */
#endif
#ifndef	S_IROTH
#define	S_IROTH		0		/* R for other */
#endif
#ifndef	S_IWOTH
#define	S_IWOTH		0		/* W for other */
#endif
#ifndef	S_IXOTH
#define	S_IXOTH		0		/* X for other */
#endif
#else /* !DB_WIN32 */
#ifndef	S_IRUSR
#define	S_IRUSR		0000400		/* R for owner */
#endif
#ifndef	S_IWUSR
#define	S_IWUSR		0000200		/* W for owner */
#endif
#ifndef	S_IXUSR
#define	S_IXUSR		0000100		/* X for owner */
#endif
#ifndef	S_IRGRP
#define	S_IRGRP		0000040		/* R for group */
#endif
#ifndef	S_IWGRP
#define	S_IWGRP		0000020		/* W for group */
#endif
#ifndef	S_IXGRP
#define	S_IXGRP		0000010		/* X for group */
#endif
#ifndef	S_IROTH
#define	S_IROTH		0000004		/* R for other */
#endif
#ifndef	S_IWOTH
#define	S_IWOTH		0000002		/* W for other */
#endif
#ifndef	S_IXOTH
#define	S_IXOTH		0000001		/* X for other */
#endif
#endif /* !DB_WIN32 */

/*
 * Don't step on the namespace.  Other libraries may have their own
 * implementations of these functions, we don't want to use their
 * implementations or force them to use ours based on the load order.
 */
#ifndef	HAVE_ATOI
#define	atoi		__db_Catoi
#endif
#ifndef	HAVE_ATOL
#define	atol		__db_Catol
#endif
#ifndef	HAVE_BSEARCH
#define	bsearch		__db_Cbsearch
#endif
#ifndef	HAVE_FCLOSE
#define	fclose		__db_Cfclose
#endif
#ifndef	HAVE_FGETC
#define	fgetc		__db_Cfgetc
#endif
#ifndef	HAVE_FGETS
#define	fgets		__db_Cfgets
#endif
#ifndef	HAVE_FOPEN
#define	fopen		__db_Cfopen
#endif
#ifndef	HAVE_FWRITE
#define	fwrite		__db_Cfwrite
#endif
#ifndef	HAVE_GETADDRINFO
#define	freeaddrinfo(a)		__db_Cfreeaddrinfo(a)
#define	getaddrinfo(a, b, c, d)	__db_Cgetaddrinfo(a, b, c, d)
#endif
#ifndef	HAVE_GETCWD
#define	getcwd		__db_Cgetcwd
#endif
#ifndef	HAVE_GETOPT
#define	getopt		__db_Cgetopt
#define	optarg		__db_Coptarg
#define	opterr		__db_Copterr
#define	optind		__db_Coptind
#define	optopt		__db_Coptopt
#define	optreset	__db_Coptreset
#endif
#ifndef	HAVE_ISALPHA
#define	isalpha		__db_Cisalpha
#endif
#ifndef	HAVE_ISDIGIT
#define	isdigit		__db_Cisdigit
#endif
#ifndef	HAVE_ISPRINT
#define	isprint		__db_Cisprint
#endif
#ifndef	HAVE_ISSPACE
#define	isspace		__db_Cisspace
#endif
#ifndef	HAVE_LOCALTIME
#define	localtime	__db_Clocaltime
#endif
#ifndef	HAVE_MEMCMP
#define	memcmp		__db_Cmemcmp
#endif
#ifndef	HAVE_MEMCPY
#define	memcpy		__db_Cmemcpy
#endif
#ifndef	HAVE_MEMMOVE
#define	memmove		__db_Cmemmove
#endif
#ifndef	HAVE_PRINTF
#define	printf		__db_Cprintf
#define	fprintf		__db_Cfprintf
#endif
#ifndef	HAVE_QSORT
#define	qsort		__db_Cqsort
#endif
#ifndef	HAVE_RAISE
#define	raise		__db_Craise
#endif
#ifndef	HAVE_RAND
#define	rand		__db_Crand
#define	srand		__db_Csrand
#endif
#ifndef	HAVE_SNPRINTF
#define	snprintf	__db_Csnprintf
#endif
#ifndef	HAVE_STRCASECMP
#define	strcasecmp	__db_Cstrcasecmp
#define	strncasecmp	__db_Cstrncasecmp
#endif
#ifndef	HAVE_STRCAT
#define	strcat		__db_Cstrcat
#endif
#ifndef	HAVE_STRCHR
#define	strchr		__db_Cstrchr
#endif
#ifndef	HAVE_STRDUP
#define	strdup		__db_Cstrdup
#endif
#ifndef	HAVE_STRERROR
#define	strerror	__db_Cstrerror
#endif
#ifndef	HAVE_STRNCAT
#define	strncat		__db_Cstrncat
#endif
#ifndef	HAVE_STRNCMP
#define	strncmp		__db_Cstrncmp
#endif
#ifndef	HAVE_STRRCHR
#define	strrchr		__db_Cstrrchr
#endif
#ifndef	HAVE_STRSEP
#define	strsep		__db_Cstrsep
#endif
#ifndef	HAVE_STRTOL
#define	strtol		__db_Cstrtol
#endif
#ifndef	HAVE_STRTOUL
#define	strtoul		__db_Cstrtoul
#endif
#ifndef	HAVE_TIME
#define	time		__db_Ctime
#endif
#ifndef	HAVE_VSNPRINTF
#define	vsnprintf	__db_Cvsnprintf
#endif
