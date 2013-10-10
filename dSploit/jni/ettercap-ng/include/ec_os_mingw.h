#ifndef EC_OS_MINGW_H
#define EC_OS_MINGW_H

/* This file is *not* MingW specific, but Ettercap requires gcc.
 * So that leaves other Win32 compilers out.
 */
#include <malloc.h>     /* for alloca() */
#include <winsock2.h>   /* u_char etc. */

#if !defined(HAVE_SLEEP)
   #define sleep(sec)    Sleep (1000*(sec))
#endif

#if !defined(HAVE_USLEEP)
   #define usleep(usec)  Sleep ((usec)/1000)
#endif

#if !defined(HAVE_GETUID)
   #define getuid()      (0)
#endif

#if !defined(HAVE_GETGID)
   #define getgid()   	 (0)
#endif

#if !defined(HAVE_GETEUID)
   #define geteuid()     (0)
#endif

#if !defined(HAVE_GETEUID)
   #define getegid()     (0)
#endif

#if !defined(HAVE_SETUID)
   #define setuid(x)     (0)
#endif

#if !defined(HAVE_SETGID)
   #define setgid(x)     (0)
#endif

#if !defined(HAVE_RANDOM)
   #define random()      rand()
#endif

#if !defined(HAVE_SRANDOM)
   #define srandom(s)    srand(s)
#endif

#if !defined(_TIMEVAL_DEFINED) && !defined(HAVE_STRUCT_TIMEVAL)
   #define _TIMEVAL_DEFINED
   struct timeval {
          long    tv_sec;
          long    tv_usec;
        };
#endif

#if !defined(HAVE_STRUCT_TIMEZONE)
   #define HAVE_STRUCT_TIMEZONE
   struct timezone {
          int tz_minuteswest;     /* minutes west of Greenwich */
          int tz_dsttime;         /* type of dst correction */
        };
#endif

#undef  _U_
#if defined(__GNUC__)
   #define _U_  __attribute__((unused))
#else
   #define _U_
#endif

#ifndef EINPROGRESS
#define EINPROGRESS  WSAEINPROGRESS
#endif

#ifndef EALREADY
#define EALREADY     WSAEALREADY
#endif

#ifndef EWOULDBLOCK
#define EWOULDBLOCK  WSAEWOULDBLOCK
#endif

#ifndef EISCONN
#define EISCONN WSAEISCONN
#endif

/* Only used in socket code */
#undef  EINTR
#define EINTR        WSAEINTR

#undef  EAGAIN
#define EAGAIN       WSAEWOULDBLOCK

#define gettimeofday(tv,tz)    ec_win_gettimeofday (tv, tz)
#define strsignal(signo)       ec_win_strsignal (signo)
#define poll(p,n,t)            ec_win_poll (p,n,t)
#define dn_expand(m,e,c,ex,l)  ec_win_dn_expand (m, e, c, ex, l)
#define dn_comp(e,c,l,d,ld)    ec_win_dn_comp(e,c,l,d,ld)

EC_API_EXTERN int         ec_win_dn_expand (const u_char *msg, const u_char *eom_orig,
                                     const u_char *comp_dn, char *exp_dn, int length);
EC_API_EXTERN int         ec_win_dn_comp   (const char *exp_dn, u_char *comp_dn, int length,
                                     u_char **dnptrs, u_char **lastdnptr);

EC_API_EXTERN int         ec_win_gettimeofday (struct timeval *tv, struct timezone *tz);
EC_API_EXTERN const char *ec_win_strsignal (int signo);

/* poll() emulation
 */
#define POLLIN   0x0001
#define POLLPRI  0x0002   /* not used */
#define POLLOUT  0x0004
#define POLLERR  0x0008
#define POLLHUP  0x0010   /* not used */
#define POLLNVAL 0x0020   /* not used */

struct pollfd {
       int fd;
       int events;     /* in param: what to poll for */
       int revents;    /* out param: what events occured */
     };

#undef  HAVE_POLL
#define HAVE_POLL 1

EC_API_EXTERN int ec_win_poll (struct pollfd *p, int num, int timeout);

/*  User/program dir
 */
EC_API_EXTERN const char *ec_win_get_user_dir (void);
EC_API_EXTERN const char *ec_win_get_ec_dir (void);

/* This is a stupid hack. How can we on compile time know the install location on a
 * on-Unix system?
 */
#ifndef INSTALL_PREFIX
   #define INSTALL_PREFIX  ec_win_get_ec_dir()
#endif

#ifndef INSTALL_EXECPREFIX
   #define INSTALL_EXECPREFIX ec_win_get_ec_dir()
#endif
   
#ifndef INSTALL_SYSCONFDIR
   #define INSTALL_SYSCONFDIR ec_win_get_ec_dir()
#endif

#ifndef INSTALL_BINDIR
   #define INSTALL_BINDIR     ec_win_get_ec_dir()
#endif

#ifndef INSTALL_LIBDIR
   #define INSTALL_LIBDIR    "/lib"    /* this cannot be a function (sigh) */
#endif

#ifndef INSTALL_DATADIR
   #define INSTALL_DATADIR   "/share"  /* this cannot be a function (sigh) */
#endif
   
/* dlopen() emulation (not exported)
 */
#if !defined(HAVE_DLOPEN)
   #define RTLD_NOW 0
   #define LTDL_SHLIB_EXT       "*.dll"

   #define dlopen(dll,flg)      ec_win_dlopen (dll, flg)
   #define lt_dlopen(dll)       ec_win_dlopen (dll, 0)
   #define lt_dlopenext(dll)    ec_win_dlopen (dll, 0)
   #define dlsym(hnd,func)      ec_win_dlsym (hnd, func)
   #define lt_dlsym(hnd,func)   ec_win_dlsym (hnd, func)
   #define dlclose(hnd)         ec_win_dlclose (hnd)
   #define lt_dlclose(hnd)      ec_win_dlclose (hnd)
   #define dlerror()            ec_win_dlerror()
   #define lt_dlerror()         ec_win_dlerror()
   #define lt_dlinit()          (0)
   #define lt_dlexit()          (0)

   EC_API_EXTERN void       *ec_win_dlopen  (const char *dll_name, int flags _U_);
   EC_API_EXTERN void       *ec_win_dlsym   (const void *dll_handle, const char *func_name);
   EC_API_EXTERN void        ec_win_dlclose (const void *dll_handle);
   EC_API_EXTERN const char *ec_win_dlerror (void);
#endif

/*
 * Unix process emulation
 */
#if !defined(HAVE_FORK)
  #define fork()  ec_win_fork()

  EC_API_EXTERN int ec_win_fork(void);
#endif

#if !defined(HAVE_WAIT)
  #define wait(st)  ec_win_wait(st)

  EC_API_EXTERN int ec_win_wait (int *status);
#endif
   
  
/* Missing stuff for ec_resolv.h / ec_win_dnexpand()
 */
#ifndef INT16SZ
#define INT16SZ 2
#endif

#ifndef INT32SZ
#define INT32SZ 4
#endif

#undef  GETSHORT
#define GETSHORT(s, cp) do { \
        register u_char *t_cp = (u_char *)(cp); \
        (s) = ((u_short)t_cp[0] << 8) \
            | ((u_short)t_cp[1]); \
        (cp) += INT16SZ; \
      } while (0)

#undef  GETLONG
#define GETLONG(l, cp) do { \
        register u_char *t_cp = (u_char *)(cp); \
        (l) = ((u_long)t_cp[0] << 24) \
            | ((u_long)t_cp[1] << 16) \
            | ((u_long)t_cp[2] << 8) \
            | ((u_long)t_cp[3]); \
        (cp) += INT32SZ; \
      } while (0)

#undef  PUTSHORT
#define PUTSHORT(s, cp) do { \
        register u_short t_s = (u_short)(s); \
        register u_char *t_cp = (u_char *)(cp); \
        *t_cp++ = t_s >> 8; \
        *t_cp   = t_s; \
        (cp) += INT16SZ; \
      } while (0)

#undef  PUTLONG
#define PUTLONG(l, cp) do { \
        register u_long t_l = (u_long)(l); \
        register u_char *t_cp = (u_char *)(cp); \
        *t_cp++ = t_l >> 24; \
        *t_cp++ = t_l >> 16; \
        *t_cp++ = t_l >> 8; \
        *t_cp   = t_l; \
        (cp) += INT32SZ; \
      } while (0)

/*
 * Misc. stuff
 */
#define strerror ec_win_strerror
EC_API_EXTERN char *ec_win_strerror(int err);
EC_API_EXTERN int  ec_win_pcap_stop(const void *pcap_handle);
  
#endif /* EC_WIN_MISC_H */
