#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#include <asm/types.h>

#ifndef __ASSEMBLY__

#include <linux/posix_types.h>


/*
 * Below are truly Linux-specific types that should never collide with
 * any application/library that wants linux/types.h.
 */


#define __bitwise__
#define __bitwise

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;

#endif /*  __ASSEMBLY__ */
#endif /* _LINUX_TYPES_H */
