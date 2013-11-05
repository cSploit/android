#ifndef _KERNEL_IPX_H
#define _KERNEL_IPX_H

#include <ncp/ext/socket.h>
#include <asm/types.h>
#include <linux/ipx.h>

#if 0 /* #define SIOCPROTOPRIVATE not needed */
#undef SIOCPROTOPRIVATE
#define SIOCPROTOPRIVATE 0x89E0
#endif
#if 0/* #define SIOCAIPX* not needed */
#undef SIOCAIPXITFCRT
#undef SIOCAIPXPRISLT
#undef SIOCIPXCFGDATA
#define SIOCAIPXITFCRT	(SIOCPROTOPRIVATE)
#define SIOCAIPXPRISLT	(SIOCPROTOPRIVATE+1)
#define SIOCIPXCFGDATA	(SIOCPROTOPRIVATE+2)
#endif

#ifndef IPXPROTO_IPX
#define	IPXPROTO_IPX	(PF_IPX)
#endif

#endif
