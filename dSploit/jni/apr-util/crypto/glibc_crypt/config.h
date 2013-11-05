/* config.h.  Generated automatically by configure.  */
/* Define if using GNU ld, with support for weak symbols in a.out,
   and for symbol set and warning messages extensions in a.out and ELF.
   This implies HAVE_WEAK_SYMBOLS; set by --with-gnu-ld.  */
#define	HAVE_GNU_LD 1

/* Define if using ELF, which supports weak symbols.
   This implies HAVE_ASM_WEAK_DIRECTIVE and NO_UNDERSCORES; set by
   --with-elf.  */
#define	HAVE_ELF 1

/* Define if using XCOFF. Set by --with-xcoff.  */
/* #undef	HAVE_XCOFF */

/* Define if C symbols are asm symbols.  Don't define if C symbols
   have a `_' prepended to make the asm symbol.  */
#define	NO_UNDERSCORES 1

/* Define if weak symbols are available via the `.weak' directive.  */
#define	HAVE_ASM_WEAK_DIRECTIVE 1

/* Define if weak symbols are available via the `.weakext' directive.  */
/* #undef	HAVE_ASM_WEAKEXT_DIRECTIVE */

/* Define to the assembler line separator character for multiple
   assembler instructions per line.  Default is `;'  */
/* #undef ASM_LINE_SEP */

/* Define if not using ELF, but `.init' and `.fini' sections are available.  */
/* #undef	HAVE_INITFINI */

/* Define if __attribute__((section("foo"))) puts quotes around foo.  */
/* #undef  HAVE_SECTION_QUOTES */

/* Define if using the GNU assembler, gas.  */
#define	HAVE_GNU_AS 1

/* Define if the assembler supports the `.set' directive.  */
/* #undef	HAVE_ASM_SET_DIRECTIVE */

/* Define to the name of the assembler's directive for
   declaring a symbol global (default `.globl').  */
#define	ASM_GLOBAL_DIRECTIVE .globl

/* Define a symbol_name as a global .symbol_name for ld.  */
/* #undef	HAVE_ASM_GLOBAL_DOT_NAME */

/* Define to use GNU libio instead of GNU stdio.
   This is defined by configure under --enable-libio.  */
#define	USE_IN_LIBIO 1

/* Define if using ELF and the assembler supports the `.previous'
   directive.  */
#define	HAVE_ASM_PREVIOUS_DIRECTIVE 1

/* Define if using ELF and the assembler supports the `.popsection'
   directive.  */
/* #undef	HAVE_ASM_POPSECTION_DIRECTIVE */

/* Define to the prefix Alpha/ELF GCC emits before ..ng symbols.  */
/* #undef  ASM_ALPHA_NG_SYMBOL_PREFIX */

/* Define if versioning of the library is wanted.  */
#define	DO_VERSIONING 1

/* Defined to the oldest ABI we support, like 2.1.  */
/* #undef GLIBC_OLDEST_ABI */

/* Define if static NSS modules are wanted.  */
/* #undef	DO_STATIC_NSS */

/* Define if gcc uses DWARF2 unwind information for exception support.  */
/* #undef	HAVE_DWARF2_UNWIND_INFO */

/* Define if gcc uses DWARF2 unwind information for exception support
   with static variable. */
/* #undef	HAVE_DWARF2_UNWIND_INFO_STATIC */

/* Define if the compiler supports __builtin_expect.  */
#define	HAVE_BUILTIN_EXPECT 1

/* Define if the regparm attribute shall be used for local functions
   (gcc on ix86 only).  */
/* #undef	USE_REGPARMS */

/* Defined on PowerPC if the GCC being used has a problem with clobbering
   certain registers (CR0, MQ, CTR, LR) in asm statements.  */
/* #undef	BROKEN_PPC_ASM_CR0 */


/* Defined to some form of __attribute__ ((...)) if the compiler supports
   a different, more efficient calling convention.  */
#if defined USE_REGPARMS && !defined PROF && !defined __BOUNDED_POINTERS__
# define internal_function __attribute__ ((regparm (3), stdcall))
#endif

/* Linux specific: minimum supported kernel version.  */
/* #undef	__LINUX_KERNEL_VERSION */

/* Override abi-tags ABI version if necessary.  */
/* #undef  __ABI_TAG_VERSION */

/* An extension in gcc 2.96 and up allows the subtraction of two
   local labels.  */
#define	HAVE_SUBTRACT_LOCAL_LABELS 1

/* bash 2.0 introduced the _XXX_GNU_nonoption_argv_flags_ variable to help
   getopt determine whether a parameter is a flag or not.  This features
   was disabled later since it caused trouble.  We are by default therefore
   disabling the support as well.  */
/* #undef USE_NONOPTION_FLAGS */

/*
 */

#ifndef	_LIBC

/* These symbols might be defined by some sysdeps configures.
   They are used only in miscellaneous generator programs, not
   in compiling libc itself.   */

/* sysdeps/generic/configure.in */
/* #undef	HAVE_PSIGNAL */

/* sysdeps/unix/configure.in */
/* #undef	HAVE_STRERROR */

/* sysdeps/unix/common/configure.in */
/* #undef	HAVE_SYS_SIGLIST */
/* #undef	HAVE__SYS_SIGLIST */
/* #undef	HAVE__CTYPE_ */
/* #undef	HAVE___CTYPE_ */
/* #undef	HAVE___CTYPE */
/* #undef	HAVE__CTYPE__ */
/* #undef	HAVE__CTYPE */
/* #undef	HAVE__LOCP */

#endif

/*
 */

#ifdef	_LIBC

/* The zic and zdump programs need these definitions.  */

#define	HAVE_STRERROR	1

/* The locale code needs these definitions.  */

#define HAVE_REGEX 1

#endif
