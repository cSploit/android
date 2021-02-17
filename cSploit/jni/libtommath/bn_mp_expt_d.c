#include <tommath.h>
#ifdef BN_MP_EXPT_D_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis
 *
 * LibTomMath is a library that provides multiple-precision
 * integer arithmetic as well as number theoretic functionality.
 *
 * The library was designed directly after the MPI library by
 * Michael Fromberger but has been written from scratch with
 * additional optimizations in place.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

/* calculate c = a**b  using a square-multiply algorithm */
int mp_expt_d (mp_int * a, mp_digit b, mp_int * c)
{
  int     res;
  mp_int  g;

  if ((res = mp_init_copy (&g, a)) != MP_OKAY) {
    return res;
  }

  /* set initial result */
  mp_set (c, 1);

  while (b > 0) {
    /* if the bit is set multiply */
    if (b & 1) {
      if ((res = mp_mul (c, &g, c)) != MP_OKAY) {
        mp_clear (&g);
        return res;
      }
    }

    /* square */
    if (b > 1 && (res = mp_sqr (&g, &g)) != MP_OKAY) {
      mp_clear (&g);
      return res;
    }

    /* shift to next bit */
    b >>= 1;
  }

  mp_clear (&g);
  return MP_OKAY;
}
#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
