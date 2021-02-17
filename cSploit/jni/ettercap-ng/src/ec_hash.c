/***
 *
 * Fowler/Noll/Vo hash
 *
 * The basis of this hash algorithm was taken from an idea sent
 * as reviewer comments to the IEEE POSIX P1003.2 committee by:
 *
 *      Phong Vo (http://www.research.att.com/info/kpv/)
 *      Glenn Fowler (http://www.research.att.com/~gsf/)
 *
 * In a subsequent ballot round:
 *
 *      Landon Curt Noll (http://www.isthe.com/chongo/)
 *
 * improved on their algorithm.  Some people tried this hash
 * and found that it worked rather well.  In an EMail message
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low
 * collision rate. The FNV speed allows one to quickly hash lots
 * of data while maintaining a reasonable collision rate.  See:
 *
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * for more details as well as other forms of the FNV hash.
 ***
 *
 * NOTE: The FNV-0 historic hash is not recommended.  One should use
 *	 the FNV-1 hash instead.
 *
 * To use the 32 bit FNV-0 historic hash, pass FNV0_32_INIT as the
 * Fnv32_t hashval argument to fnv_32_buf() or fnv_32_str().
 *
 * To use the recommended 32 bit FNV-1 hash, pass FNV1_32_INIT as the
 * Fnv32_t hashval argument to fnv_32_buf() or fnv_32_str().
 *
 ***
 *
 * Please do not copyright this code.  This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * By:
 *	chongo <Landon Curt Noll> /\oo/\
 *      http://www.isthe.com/chongo/
 *
 * Share and Enjoy!	:-)
 */

#include <ec.h>
#include <ec_hash.h>

#define FNV_32_PRIME ((Fnv32_t)0x01000193)
#define FNV_64_PRIME ((Fnv64_t)0x100000001b3ULL)

Fnv32_t
fnv_32(void *buf, size_t len)
{
   unsigned char *bp = (unsigned char *)buf;	/* start of buffer */
   unsigned char *be = bp + len;		/* beyond end of buffer */
   Fnv32_t hval      = FNV1_32_INIT;

   /*
    * FNV-1 hash each octet in the buffer
    */
   while (bp < be) {

	   /* multiply by the 32 bit FNV magic prime mod 2^64 */
	   hval *= FNV_32_PRIME;

	   /* xor the bottom with the current octet */
	   hval ^= (Fnv32_t)*bp++;
   }

   /* return our new hash value */
   return hval;
}

Fnv64_t
fnv_64(void *buf, size_t len)
{
   unsigned char *bp = (unsigned char *)buf;	/* start of buffer */
   unsigned char *be = bp + len;		/* beyond end of buffer */
   Fnv64_t hval      = FNV1_64_INIT;
   /*
    * FNV-1 hash each octet of the buffer
    */
   while (bp < be) {

	   /* multiply by the 64 bit FNV magic prime mod 2^64 */
	   hval *= FNV_64_PRIME;

	   /* xor the bottom with the current octet */
	   hval ^= (Fnv64_t)*bp++;
   }
 
   /* return our new hash value */
   return hval;
}

/* EOF */

// vim:ts=3:expandtab

