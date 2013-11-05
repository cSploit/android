#ifndef __KERNEL_TYPES_H__
#define __KERNEL_TYPES_H__

#include <sys/types.h>
#include <sys/select.h>

#define NCP_IN_SUPPORT
#define NCP_IPX_SUPPORT

/* FIXME: What about configure?! */
#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__ 1

/* AIX4.3.x has these types, but does not define
   BIT_TYPES_DEFINED... */
#ifndef _H_INTTYPES
typedef unsigned char      u_int8_t;
typedef unsigned short int u_int16_t;
typedef unsigned int       u_int32_t;
typedef unsigned long long u_int64_t;

typedef signed char        int8_t;
typedef signed short int   int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;
#endif

#endif	/* __BIT_TYPES_DEFINED__ */

#if 0

#define NCP_KERNEL_NCPFS_AVAILABLE

typedef u_int16_t __kerXX_uid_t;

#include <linux/posix_types.h>

typedef __kernel_pid_t __ker20_pid_t;
typedef __kernel_uid_t __ker20_uid_t;
typedef __kernel_gid_t __ker20_gid_t;
typedef __kernel_mode_t __ker20_mode_t;

typedef __kernel_pid_t __ker21_pid_t;
typedef __kernel_uid_t __ker21_uid_t;
typedef __kernel_gid_t __ker21_gid_t;
typedef __kernel_mode_t __ker21_mode_t;

#else

#undef NCP_KERNEL_NCPFS_AVAILABLE

#endif	/* compiling on linux kernel with ncp support */

#endif	/* __KERNEL_TYPES_H__ */

