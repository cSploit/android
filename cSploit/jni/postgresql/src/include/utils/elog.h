/*-------------------------------------------------------------------------
 *
 * elog.h
 *	  POSTGRES error reporting/logging definitions.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/elog.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef ELOG_H
#define ELOG_H

#include <setjmp.h>

/* Error level codes */
#define DEBUG5		10			/* Debugging messages, in categories of
								 * decreasing detail. */
#define DEBUG4		11
#define DEBUG3		12
#define DEBUG2		13
#define DEBUG1		14			/* used by GUC debug_* variables */
#define LOG			15			/* Server operational messages; sent only to
								 * server log by default. */
#define COMMERROR	16			/* Client communication problems; same as LOG
								 * for server reporting, but never sent to
								 * client. */
#define INFO		17			/* Messages specifically requested by user (eg
								 * VACUUM VERBOSE output); always sent to
								 * client regardless of client_min_messages,
								 * but by default not sent to server log. */
#define NOTICE		18			/* Helpful messages to users about query
								 * operation; sent to client and server log by
								 * default. */
#define WARNING		19			/* Warnings.  NOTICE is for expected messages
								 * like implicit sequence creation by SERIAL.
								 * WARNING is for unexpected messages. */
#define ERROR		20			/* user error - abort transaction; return to
								 * known state */
/* Save ERROR value in PGERROR so it can be restored when Win32 includes
 * modify it.  We have to use a constant rather than ERROR because macros
 * are expanded only when referenced outside macros.
 */
#ifdef WIN32
#define PGERROR		20
#endif
#define FATAL		21			/* fatal error - abort process */
#define PANIC		22			/* take down the other backends with me */

 /* #define DEBUG DEBUG1 */	/* Backward compatibility with pre-7.3 */


/* macros for representing SQLSTATE strings compactly */
#define PGSIXBIT(ch)	(((ch) - '0') & 0x3F)
#define PGUNSIXBIT(val) (((val) & 0x3F) + '0')

#define MAKE_SQLSTATE(ch1,ch2,ch3,ch4,ch5)	\
	(PGSIXBIT(ch1) + (PGSIXBIT(ch2) << 6) + (PGSIXBIT(ch3) << 12) + \
	 (PGSIXBIT(ch4) << 18) + (PGSIXBIT(ch5) << 24))

/* These macros depend on the fact that '0' becomes a zero in SIXBIT */
#define ERRCODE_TO_CATEGORY(ec)  ((ec) & ((1 << 12) - 1))
#define ERRCODE_IS_CATEGORY(ec)  (((ec) & ~((1 << 12) - 1)) == 0)

/* SQLSTATE codes for errors are defined in a separate file */
#include "utils/errcodes.h"


/* Which __func__ symbol do we have, if any? */
#ifdef HAVE_FUNCNAME__FUNC
#define PG_FUNCNAME_MACRO	__func__
#else
#ifdef HAVE_FUNCNAME__FUNCTION
#define PG_FUNCNAME_MACRO	__FUNCTION__
#else
#define PG_FUNCNAME_MACRO	NULL
#endif
#endif


/*----------
 * New-style error reporting API: to be used in this way:
 *		ereport(ERROR,
 *				(errcode(ERRCODE_UNDEFINED_CURSOR),
 *				 errmsg("portal \"%s\" not found", stmt->portalname),
 *				 ... other errxxx() fields as needed ...));
 *
 * The error level is required, and so is a primary error message (errmsg
 * or errmsg_internal).  All else is optional.	errcode() defaults to
 * ERRCODE_INTERNAL_ERROR if elevel is ERROR or more, ERRCODE_WARNING
 * if elevel is WARNING, or ERRCODE_SUCCESSFUL_COMPLETION if elevel is
 * NOTICE or below.
 *
 * ereport_domain() allows a message domain to be specified, for modules that
 * wish to use a different message catalog from the backend's.	To avoid having
 * one copy of the default text domain per .o file, we define it as NULL here
 * and have errstart insert the default text domain.  Modules can either use
 * ereport_domain() directly, or preferably they can override the TEXTDOMAIN
 * macro.
 *----------
 */
#define ereport_domain(elevel, domain, rest)	\
	(errstart(elevel, __FILE__, __LINE__, PG_FUNCNAME_MACRO, domain) ? \
	 (errfinish rest) : (void) 0)

#define ereport(elevel, rest)	\
	ereport_domain(elevel, TEXTDOMAIN, rest)

#define TEXTDOMAIN NULL

extern bool errstart(int elevel, const char *filename, int lineno,
		 const char *funcname, const char *domain);
extern void errfinish(int dummy,...);

extern int	errcode(int sqlerrcode);

extern int	errcode_for_file_access(void);
extern int	errcode_for_socket_access(void);

extern int
errmsg(const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int
errmsg_internal(const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int
errmsg_plural(const char *fmt_singular, const char *fmt_plural,
			  unsigned long n,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 4)))
__attribute__((format(PG_PRINTF_ATTRIBUTE, 2, 4)));

extern int
errdetail(const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int
errdetail_internal(const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int
errdetail_log(const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int
errdetail_plural(const char *fmt_singular, const char *fmt_plural,
				 unsigned long n,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 4)))
__attribute__((format(PG_PRINTF_ATTRIBUTE, 2, 4)));

extern int
errhint(const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int
errcontext(const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

extern int	errhidestmt(bool hide_stmt);

extern int	errfunction(const char *funcname);
extern int	errposition(int cursorpos);

extern int	internalerrposition(int cursorpos);
extern int	internalerrquery(const char *query);

extern int	geterrcode(void);
extern int	geterrposition(void);
extern int	getinternalerrposition(void);


/*----------
 * Old-style error reporting API: to be used in this way:
 *		elog(ERROR, "portal \"%s\" not found", stmt->portalname);
 *----------
 */
#define elog	elog_start(__FILE__, __LINE__, PG_FUNCNAME_MACRO), elog_finish

extern void elog_start(const char *filename, int lineno, const char *funcname);
extern void
elog_finish(int elevel, const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 2, 3)));


/* Support for constructing error strings separately from ereport() calls */

extern void pre_format_elog_string(int errnumber, const char *domain);
extern char *
format_elog_string(const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));


/* Support for attaching context information to error reports */

typedef struct ErrorContextCallback
{
	struct ErrorContextCallback *previous;
	void		(*callback) (void *arg);
	void	   *arg;
} ErrorContextCallback;

extern PGDLLIMPORT ErrorContextCallback *error_context_stack;


/*----------
 * API for catching ereport(ERROR) exits.  Use these macros like so:
 *
 *		PG_TRY();
 *		{
 *			... code that might throw ereport(ERROR) ...
 *		}
 *		PG_CATCH();
 *		{
 *			... error recovery code ...
 *		}
 *		PG_END_TRY();
 *
 * (The braces are not actually necessary, but are recommended so that
 * pg_indent will indent the construct nicely.)  The error recovery code
 * can optionally do PG_RE_THROW() to propagate the same error outwards.
 *
 * Note: while the system will correctly propagate any new ereport(ERROR)
 * occurring in the recovery section, there is a small limit on the number
 * of levels this will work for.  It's best to keep the error recovery
 * section simple enough that it can't generate any new errors, at least
 * not before popping the error stack.
 *
 * Note: an ereport(FATAL) will not be caught by this construct; control will
 * exit straight through proc_exit().  Therefore, do NOT put any cleanup
 * of non-process-local resources into the error recovery section, at least
 * not without taking thought for what will happen during ereport(FATAL).
 * The PG_ENSURE_ERROR_CLEANUP macros provided by storage/ipc.h may be
 * helpful in such cases.
 *----------
 */
#define PG_TRY()  \
	do { \
		sigjmp_buf *save_exception_stack = PG_exception_stack; \
		ErrorContextCallback *save_context_stack = error_context_stack; \
		sigjmp_buf local_sigjmp_buf; \
		if (sigsetjmp(local_sigjmp_buf, 0) == 0) \
		{ \
			PG_exception_stack = &local_sigjmp_buf

#define PG_CATCH()	\
		} \
		else \
		{ \
			PG_exception_stack = save_exception_stack; \
			error_context_stack = save_context_stack

#define PG_END_TRY()  \
		} \
		PG_exception_stack = save_exception_stack; \
		error_context_stack = save_context_stack; \
	} while (0)

/*
 * gcc understands __attribute__((noreturn)); for other compilers, insert
 * a useless exit() call so that the compiler gets the point.
 */
#ifdef __GNUC__
#define PG_RE_THROW()  \
	pg_re_throw()
#else
#define PG_RE_THROW()  \
	(pg_re_throw(), exit(1))
#endif

extern PGDLLIMPORT sigjmp_buf *PG_exception_stack;


/* Stuff that error handlers might want to use */

/*
 * ErrorData holds the data accumulated during any one ereport() cycle.
 * Any non-NULL pointers must point to palloc'd data.
 * (The const pointers are an exception; we assume they point at non-freeable
 * constant strings.)
 */
typedef struct ErrorData
{
	int			elevel;			/* error level */
	bool		output_to_server;		/* will report to server log? */
	bool		output_to_client;		/* will report to client? */
	bool		show_funcname;	/* true to force funcname inclusion */
	bool		hide_stmt;		/* true to prevent STATEMENT: inclusion */
	const char *filename;		/* __FILE__ of ereport() call */
	int			lineno;			/* __LINE__ of ereport() call */
	const char *funcname;		/* __func__ of ereport() call */
	const char *domain;			/* message domain */
	int			sqlerrcode;		/* encoded ERRSTATE */
	char	   *message;		/* primary error message */
	char	   *detail;			/* detail error message */
	char	   *detail_log;		/* detail error message for server log only */
	char	   *hint;			/* hint message */
	char	   *context;		/* context message */
	int			cursorpos;		/* cursor index into query string */
	int			internalpos;	/* cursor index into internalquery */
	char	   *internalquery;	/* text of internally-generated query */
	int			saved_errno;	/* errno at entry */
} ErrorData;

extern void EmitErrorReport(void);
extern ErrorData *CopyErrorData(void);
extern void FreeErrorData(ErrorData *edata);
extern void FlushErrorState(void);
extern void ReThrowError(ErrorData *edata);
extern void pg_re_throw(void) __attribute__((noreturn));


/* GUC-configurable parameters */

typedef enum
{
	PGERROR_TERSE,				/* single-line error messages */
	PGERROR_DEFAULT,			/* recommended style */
	PGERROR_VERBOSE				/* all the facts, ma'am */
}	PGErrorVerbosity;

extern int	Log_error_verbosity;
extern char *Log_line_prefix;
extern int	Log_destination;

/* Log destination bitmap */
#define LOG_DESTINATION_STDERR	 1
#define LOG_DESTINATION_SYSLOG	 2
#define LOG_DESTINATION_EVENTLOG 4
#define LOG_DESTINATION_CSVLOG	 8

/* Other exported functions */
extern void DebugFileOpen(void);
extern char *unpack_sql_state(int sql_state);
extern bool in_error_recursion_trouble(void);

#ifdef HAVE_SYSLOG
extern void set_syslog_parameters(const char *ident, int facility);
#endif

/*
 * Write errors to stderr (or by equal means when stderr is
 * not available). Used before ereport/elog can be used
 * safely (memory context, GUC load etc)
 */
extern void
write_stderr(const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(PG_PRINTF_ATTRIBUTE, 1, 2)));

#endif   /* ELOG_H */
