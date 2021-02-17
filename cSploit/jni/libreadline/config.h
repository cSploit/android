/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Maintained by hand. */

/* Define NO_MULTIBYTE_SUPPORT to not compile in support for multibyte
   characters, even if the OS supports them. */
/* #undef NO_MULTIBYTE_SUPPORT */

/* #undef _FILE_OFFSET_BITS */

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

#define VOID_SIGHANDLER 1

/* Characteristics of the compiler. */
/* #undef sig_atomic_t */

/* #undef size_t */

/* #undef ssize_t */

/* #undef const */

/* #undef volatile */

#define PROTOTYPES 1

/* #undef __CHAR_UNSIGNED__ */

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
/* #undef STAT_MACROS_BROKEN */

/* Define if you have the fcntl function. */
#define HAVE_FCNTL 1

/* Define if you have the getpwent function. */
/* #undef HAVE_GETPWENT */

/* Define if you have the getpwnam function. */
#define HAVE_GETPWNAM 1

/* Define if you have the getpwuid function. */
#define HAVE_GETPWUID 1

/* Define if you have the isascii function. */
#define HAVE_ISASCII 1

/* Define if you have the iswctype function.  */
#define HAVE_ISWCTYPE 1

/* Define if you have the iswlower function.  */
#define HAVE_ISWLOWER 1

/* Define if you have the iswupper function.  */
#define HAVE_ISWUPPER 1

/* Define if you have the isxdigit function. */
#define HAVE_ISXDIGIT 1

/* Define if you have the kill function. */
#define HAVE_KILL 1

/* Define if you have the lstat function. */
#define HAVE_LSTAT 1

/* Define if you have the mbrlen function. */
#define HAVE_MBRLEN 1

/* Define if you have the mbrtowc function. */
#define HAVE_MBRTOWC 1

/* Define if you have the mbsrtowcs function. */
#define HAVE_MBSRTOWCS 1

/* Define if you have the memmove function. */
#define HAVE_MEMMOVE 1

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the setenv function.  */
#define HAVE_SETENV 1

/* Define if you have the setlocale function. */
#define HAVE_SETLOCALE 1

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strcoll function.  */
/* #undef HAVE_STRCOLL */

/* #undef STRCOLL_BROKEN */

/* Define if you have the strpbrk function.  */
#define HAVE_STRPBRK 1

/* Define if you have the tcgetattr function.  */
/* #undef HAVE_TCGETATTR */

/* Define if you have the towlower function.  */
#define HAVE_TOWLOWER 1

/* Define if you have the towupper function.  */
#define HAVE_TOWUPPER 1

/* Define if you have the vsnprintf function.  */
#define HAVE_VSNPRINTF 1

/* Define if you have the wcrtomb function.  */
#define HAVE_WCRTOMB 1

/* Define if you have the wcscoll function.  */
#define HAVE_WCSCOLL 1

/* Define if you have the wctype function.  */
#define HAVE_WCTYPE 1

/* Define if you have the wcwidth function.  */
#define HAVE_WCWIDTH 1

/* and whether it works */
/* #undef WCWIDTH_BROKEN */

#define STDC_HEADERS 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <langinfo.h> header file.  */
/* #undef HAVE_LANGINFO_H */

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <locale.h> header file.  */
#define HAVE_LOCALE_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <pwd.h> header file.  */
#define HAVE_PWD_H 1

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <stdbool.h> header file.  */
#define HAVE_STDBOOL_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/file.h> header file.  */
#define HAVE_SYS_FILE_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/pte.h> header file.  */
/* #undef HAVE_SYS_PTE_H */

/* Define if you have the <sys/ptem.h> header file.  */
/* #undef HAVE_SYS_PTEM_H */

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/stream.h> header file.  */
/* #undef HAVE_SYS_STREAM_H */

/* Define if you have the <termcap.h> header file.  */
/* #undef HAVE_TERMCAP_H */

/* Define if you have the <termio.h> header file.  */
#define HAVE_TERMIO_H 1

/* Define if you have the <termios.h> header file.  */
#define HAVE_TERMIOS_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <varargs.h> header file.  */
/* #undef HAVE_VARARGS_H */

/* Define if you have the <wchar.h> header file.  */
#define HAVE_WCHAR_H 1

/* Define if you have the <wctype.h> header file.  */
#define HAVE_WCTYPE_H 1

#define HAVE_MBSTATE_T 1

/* Define if you have wchar_t in <wctype.h>. */
#define HAVE_WCHAR_T 1

/* Define if you have wctype_t in <wctype.h>. */
#define HAVE_WCTYPE_T 1

/* Define if you have wint_t in <wctype.h>. */  
#define HAVE_WINT_T 1

/* Define if you have <langinfo.h> and nl_langinfo(CODESET). */
/* #undef HAVE_LANGINFO_CODESET */

/* Define if you have <linux/audit.h> and it defines AUDIT_USER_TTY */
#define HAVE_DECL_AUDIT_USER_TTY 0

/* Definitions pulled in from aclocal.m4. */
#define VOID_SIGHANDLER 1

/* #undef GWINSZ_IN_SYS_IOCTL */

/* #undef STRUCT_WINSIZE_IN_SYS_IOCTL */

#define STRUCT_WINSIZE_IN_TERMIOS 1

/* #undef TIOCSTAT_IN_SYS_IOCTL */

#define FIONREAD_IN_SYS_IOCTL 1

/* #undef SPEED_T_IN_SYS_TYPES */

#define HAVE_GETPW_DECLS 1

/* #undef STRUCT_DIRENT_HAS_D_INO */

/* #undef STRUCT_DIRENT_HAS_D_FILENO */

/* #undef HAVE_BSD_SIGNALS */

#define HAVE_POSIX_SIGNALS 1

/* #undef HAVE_USG_SIGHOLD */

/* #undef MUST_REINSTALL_SIGHANDLERS */

/* #undef HAVE_POSIX_SIGSETJMP */

/* #undef CTYPE_NON_ASCII */

/* modify settings or make new ones based on what autoconf tells us. */

/* Ultrix botches type-ahead when switching from canonical to
   non-canonical mode, at least through version 4.3 */
#if !defined (HAVE_TERMIOS_H) || !defined (HAVE_TCGETATTR) || defined (ultrix)
#  define TERMIOS_MISSING
#endif

/* VARARGS defines moved to rlstdc.h */
