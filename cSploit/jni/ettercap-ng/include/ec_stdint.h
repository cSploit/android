

#ifndef EC_STDINT_H
#define EC_STDINT_H

#include <limits.h>

#if defined HAVE_STDINT_H && !defined OS_SOLARIS
	#include <stdint.h>
#elif !defined OS_SOLARIS
	#include <sys/types.h>
#elif defined OS_SOLARIS
	#include <sys/inttypes.h>
#endif

#ifndef TYPES_DEFINED
#define TYPES_DEFINED
	typedef int8_t    int8;
	typedef int16_t   int16;
	typedef int32_t   int32;
	typedef int64_t   int64;

	typedef u_int8_t   u_int8;
	typedef u_int16_t  u_int16;
	typedef u_int32_t  u_int32;
	typedef u_int64_t  u_int64;

   #if defined OS_BSD_OPEN && !defined HAVE_STDINT_H
      #define INT8_MAX     CHAR_MAX
      #define UINT8_MAX    UCHAR_MAX
      #define INT16_MAX    SHRT_MAX
      #define UINT16_MAX   USHRT_MAX
      #define INT32_MAX    INT_MAX
      #define UINT32_MAX   UINT_MAX
   #endif

   /* Maximum of signed integral types.  */
   #ifndef INT8_MAX
      #define INT8_MAX		(127)
   #endif
   #ifndef INT16_MAX
      #define INT16_MAX		(32767)
   #endif
   #ifndef INT32_MAX
      #define INT32_MAX		(2147483647)
   #endif

   /* Maximum of unsigned integral types.  */
   #ifndef UINT8_MAX
      #define UINT8_MAX		(255)
   #endif
   #ifndef UINT16_MAX
      #define UINT16_MAX	(65535)
   #endif
   #ifndef UINT32_MAX
      #define UINT32_MAX	(4294967295U)
   #endif
   
#endif
   
#endif

/* EOF */

// vim:ts=3:expandtab
