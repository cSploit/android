/* subversion/svn_private_config.h.tmp.  Generated from svn_private_config.h.in by configure.  */
/* subversion/svn_private_config.h.in.  Generated from configure.ac by autoheader.  */

/* The fs type to use by default */
#define DEFAULT_FS_TYPE "fsfs"

/* The http library to use by default */
#define DEFAULT_HTTP_LIBRARY "neon"

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #undef ENABLE_NLS */

/* Define to 1 if you have the `bind_textdomain_codeset' function. */
/* #undef HAVE_BIND_TEXTDOMAIN_CODESET */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `iconv' library (-liconv). */
/* #undef HAVE_LIBICONV */

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Define to 1 if you have the <magic.h> header file. */
/* #undef HAVE_MAGIC_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `rb_errinfo' function. */
/* #undef HAVE_RB_ERRINFO */

/* Define to 1 if you have the `readlink' function. */
#define HAVE_READLINK 1

/* Define to 1 if you have the <serf.h> header file. */
/* #undef HAVE_SERF_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `symlink' function. */
#define HAVE_SYMLINK 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the <zlib.h> header file. */
/* #undef HAVE_ZLIB_H */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://subversion.apache.org/"

/* Define to the full name of this package. */
#define PACKAGE_NAME "subversion"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "subversion 1.7.13"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "subversion"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.7.13"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to the Python/C API format character suitable for apr_int64_t */
#define SVN_APR_INT64_T_PYCFMT "l"

/* Define if circular linkage is not possible on this platform. */
/* #undef SVN_AVOID_CIRCULAR_LINKAGE_AT_ALL_COSTS_HACK */

/* Defined to be the path to the installed binaries */
#define SVN_BINDIR "/opt/android-ndk/platforms/android-18/arch-arm/usr/bin"

/* The path of a default editor for the client. */
/* #undef SVN_CLIENT_EDITOR */

/* Defined if the full version matching rules are disabled */
/* #undef SVN_DISABLE_FULL_VERSION_MATCH */

/* The desired major version for the Berkeley DB */
#define SVN_FS_WANT_DB_MAJOR 4

/* The desired minor version for the Berkeley DB */
#define SVN_FS_WANT_DB_MINOR 0

/* The desired patch version for the Berkeley DB */
#define SVN_FS_WANT_DB_PATCH 14

/* Is GNOME Keyring support enabled? */
/* #undef SVN_HAVE_GNOME_KEYRING */

/* Is Mac OS KeyChain support enabled? */
/* #undef SVN_HAVE_KEYCHAIN_SERVICES */

/* Defined if KWallet support is enabled */
/* #undef SVN_HAVE_KWALLET */

/* Defined if libmagic support is enabled */
/* #undef SVN_HAVE_LIBMAGIC */

/* Defined if apr_memcache (standalone or in apr-util) is present */
/* #undef SVN_HAVE_MEMCACHE */

/* Defined if support for Neon is enabled */
#define SVN_HAVE_NEON 1

/* Defined if Expat 1.0 or 1.1 was found */
/* #undef SVN_HAVE_OLD_EXPAT */

/* Defined if Cyrus SASL v2 is present on the system */
/* #undef SVN_HAVE_SASL */

/* Defined if support for Serf is enabled */
/* #undef SVN_HAVE_SERF */

/* Defined if libsvn_client should link against libsvn_ra_local */
#define SVN_LIBSVN_CLIENT_LINKS_RA_LOCAL 1

/* Defined if libsvn_client should link against libsvn_ra_neon */
#define SVN_LIBSVN_CLIENT_LINKS_RA_NEON 1

/* Defined if libsvn_client should link against libsvn_ra_serf */
/* #undef SVN_LIBSVN_CLIENT_LINKS_RA_SERF */

/* Defined if libsvn_client should link against libsvn_ra_svn */
#define SVN_LIBSVN_CLIENT_LINKS_RA_SVN 1

/* Defined if libsvn_fs should link against libsvn_fs_base */
/* #undef SVN_LIBSVN_FS_LINKS_FS_BASE */

/* Defined if libsvn_fs should link against libsvn_fs_fs */
#define SVN_LIBSVN_FS_LINKS_FS_FS 1

/* Defined to be the path to the installed locale dirs */
#define SVN_LOCALE_DIR "/opt/android-ndk/platforms/android-18/arch-arm/usr/share/locale"

/* Define to 1 if you have Neon 0.26 or later. */
#define SVN_NEON_0_26 1

/* Define to 1 if you have Neon 0.27 or later. */
#define SVN_NEON_0_27 1

/* Define to 1 if you have Neon 0.28 or later. */
#define SVN_NEON_0_28 1

/* Defined to be the null device for the system */
#define SVN_NULL_DEVICE_NAME "/dev/null"

/* Defined to be the path separator used on your local filesystem */
#define SVN_PATH_LOCAL_SEPARATOR '/'

/* Defined if support for GSSAPI is enabled */
/* #undef SVN_RA_SERF_HAVE_GSSAPI */

/* Defined if svn should use the amalgamated version of sqlite */
/* #undef SVN_SQLITE_INLINE */

/* Defined if svn should try to load DSOs */
/* #undef SVN_USE_DSO */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

#ifdef SVN_WANT_BDB
#define APU_WANT_DB

#endif


#define N_(x) x
#define U_(x) x
#ifdef ENABLE_NLS
#include <locale.h>
#include <libintl.h>
#define _(x) dgettext(PACKAGE_NAME, x)
#define Q_(x1, x2, n) dngettext(PACKAGE_NAME, x1, x2, n)
#else
#define _(x) (x)
#define Q_(x1, x2, n) (((n) == 1) ? x1 : x2)
#define gettext(x) (x)
#define dgettext(domain, x) (x)
#endif

