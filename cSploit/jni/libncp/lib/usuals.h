/* usuals.h - The usual typedefs, etc.
*/
#ifndef USUALS /* Assures no redefinitions of usual types...*/
#define USUALS

#include <ncp/kernel/types.h>

typedef unsigned char boolean;	/* values are TRUE or FALSE */
typedef u_int8_t byte;	/* values are 0-255 */
typedef byte *byteptr;	/* pointer to byte */
typedef char *string;	/* pointer to ASCII character string */
typedef u_int16_t word16;	/* values are 0-65535 */
typedef u_int32_t word32;	/* values are 0-4294967295 */

#ifndef TRUE
#define FALSE 0
#define TRUE (!FALSE)
#endif	/* if TRUE not already defined */

#ifndef min	/* if min macro not already defined */
#define min(a,b) (((a)<(b)) ? (a) : (b) )
#define max(a,b) (((a)>(b)) ? (a) : (b) )
#endif	/* if min macro not already defined */

/* void for use in pointers */
#ifndef NO_VOID_STAR
#define	VOID	void
#else
#define	VOID	char
#endif

	/* Zero-fill the byte buffer. */
#define fill0(buffer,count)	memset( buffer, 0, count )

	/* This macro is for burning sensitive data.  Many of the
	   file I/O routines use it for zapping buffers */
#define burn(x) fill0((VOID *)&(x),sizeof(x))

#endif	/* if USUALS not already defined */
