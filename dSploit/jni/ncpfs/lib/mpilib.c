/*	C source code for multiprecision arithmetic library routines.
	Implemented Nov 86 by Philip Zimmermann
	Last revised 27 Nov 91 by PRZ

	Boulder Software Engineering
	3021 Eleventh Street
	Boulder, CO 80304
	(303) 541-0140

	(c) Copyright 1986-92 by Philip Zimmermann.  All rights reserved.
	The author assumes no liability for damages resulting from the use
	of this software, even if the damage results from defects in this
	software.  No warranty is expressed or implied.  The use of this
	cryptographic software for developing weapon systems is expressly
	forbidden.

	These routines implement all of the multiprecision arithmetic
	necessary for number-theoretic cryptographic algorithms such as
	ElGamal, Diffie-Hellman, Rabin, or factoring studies for large
	composite numbers, as well as Rivest-Shamir-Adleman (RSA) public
	key cryptography.

	Although originally developed in Microsoft C for the IBM PC, this code
	contains few machine dependencies.  It assumes 2's complement
	arithmetic.  It can be adapted to 8-bit, 16-bit, or 32-bit machines,
	lowbyte-highbyte order or highbyte-lowbyte order.  This version
	has been converted to ANSI C.


	The internal representation for these extended precision integer
	"registers" is an array of "units".  A unit is a machine word, which
	is either an 8-bit byte, a 16-bit unsigned integer, or a 32-bit
	unsigned integer, depending on the machine's word size.  For example,
	an IBM PC or AT uses a unit size of 16 bits.  To perform arithmetic
	on these huge precision integers, we pass pointers to these unit
	arrays to various subroutines.  A pointer to an array of units is of
	type unitptr.  This is a pointer to a huge integer "register".

	When calling a subroutine, we always pass a pointer to the BEGINNING
	of the array of units, regardless of the byte order of the machine.
	On a lowbyte-first machine, such as the Intel 80x86, this unitptr
	points to the LEAST significant unit, and the array of units increases
	significance to the right.  On a highbyte-first machine, such as the
	Motorola 680x0, this unitptr points to the MOST significant unit, and
	the array of units decreases significance to the right.

	Modified 8 Apr 92 - HAJK
	Implement new VAX/VMS primitive support.

	Modified 30 Sep 92 -Castor Fu <castor@drizzle.stanford.edu>
	Upgraded PORTABLE support to allow sizeof(unit) == sizeof(long)

	Modified 28 Nov 92 - Thad Smith
	Added Smith modmult, generalized non-portable support.
*/

/* #define COUNTMULTS */ /* count modmults for performance studies */

#ifdef DEBUG
#ifdef MSDOS
#ifdef __GO32__ /* DJGPP */
#include <pc.h>
#else
#include <conio.h>
#endif  /* __GO32__ */
#define poll_for_break() {while (kbhit()) getch();}
#endif	/* MSDOS */
#endif	/* DEBUG */

#ifndef poll_for_break
#define poll_for_break()  /* stub */
#endif

#include "mpilib.h"

#ifdef mp_smula
#ifdef mp_smul
	Error: Both mp_smula and mp_smul cannot be defined.
#else
#define mp_smul	mp_smula
#endif
#endif

/* set macros for MULTUNIT */
#if MULTUNITSIZE == 8
typedef unsigned char MULTUNIT;
#else
#if MULTUNITSIZE == 32
typedef word32 MULTUNIT;
#else
#if MULTUNITSIZE == 16
typedef word16 MULTUNIT;
else
#error "Unsupported MULTUNITSIZE"
#endif
#endif
#endif

#if UNITSIZE == MULTUNITSIZE
#  define MULTUNIT_SIZE_SAME
#else
#if UNITSIZE < MULTUNITSIZE
#  include "UNITSIZE cannot be smaller than MULTUNITSIZE"
#endif
#endif

#define MULTUNIT_msb    ((MULTUNIT)1 << (MULTUNITSIZE-1)) /* msb of MULTUNIT */
#define DMULTUNIT_msb   (1L << (2*MULTUNITSIZE-1))
#define MULTUNIT_mask   ((MULTUNIT)((1L << MULTUNITSIZE)-1))
#define MULTUNITs_perunit   (UNITSIZE/MULTUNITSIZE)


static inline void mp_smul (MULTUNIT *prod, const MULTUNIT *multiplicand, MULTUNIT multiplier);
static void mp_dmul (unitptr prod, const unit* multiplicand, const unit* multiplier);

short global_precision = 0; /* units of precision for all routines */
/*	global_precision is the unit precision last set by set_precision.
	Initially, set_precision() should be called to define global_precision
	before using any of these other multiprecision library routines.
	i.e.:   set_precision(MAX_UNIT_PRECISION);
*/

/*************** multiprecision library primitives ****************/
/*	The following portable C primitives should be recoded in assembly.
	The entry point name should be defined, in "mpilib.h" to the external
	entry point name.  If undefined, the C version will be used.
*/

typedef unsigned long int ulint;

#ifndef mp_addc
boolean mp_addc
	(register unitptr r1, const unit* r2,register boolean carry)
	/* multiprecision add with carry r2 to r1, result in r1 */
	/* carry is incoming carry flag-- value should be 0 or 1 */
{	register unit x;
	short precision;	/* number of units to add */
	precision = global_precision;
	make_lsbptr(r1,precision);
	make_lsbptr(r2,precision);
	while (precision--)
	{
		if (carry)
		{	x = *r1 + *r2 + 1;
			carry = (*r2  >= (unit)(~ *r1));
		} else
		{	x = *r1 + *r2;
			carry = (x < *r1) ;
		}
		post_higherunit(r2);
		*post_higherunit(r1) = x;
	}
	return(carry);		/* return the final carry flag bit */
}	/* mp_addc */
#endif  /* mp_addc */

#ifndef mp_subb
boolean mp_subb
	(register unitptr r1, const unit* r2,register boolean borrow)
	/* multiprecision subtract with borrow, r2 from r1, result in r1 */
	/* borrow is incoming borrow flag-- value should be 0 or 1 */
{	register unit x;
	short precision;	/* number of units to subtract */
	precision = global_precision;
	make_lsbptr(r1,precision);
	make_lsbptr(r2,precision);
	while (precision--)
	{	if (borrow)
		{	x = *r1 - *r2 - 1;
			borrow = (*r1 <= *r2);
		} else
		{	x = *r1 - *r2;
			borrow = (*r1 < *r2);
		}
		post_higherunit(r2);
		*post_higherunit(r1) = x;
	}
	return(borrow);	/* return the final carry/borrow flag bit */
}	/* mp_subb */
#endif  /* mp_subb */

#ifndef mp_rotate_left
boolean mp_rotate_left(register unitptr r1,register boolean carry)
	/* multiprecision rotate left 1 bit with carry, result in r1. */
	/* carry is incoming carry flag-- value should be 0 or 1 */
{	register int precision;	/* number of units to rotate */
	unsigned int mcarry = carry, nextcarry=0; /* int is supposed to be
						 * the efficient size for ops*/
	precision = global_precision;
	make_lsbptr(r1,precision);
	while (precision--)
	{
		nextcarry = (((signedunit) *r1) < 0);
		*r1 = (*r1 << 1) | mcarry;
		mcarry = nextcarry;
		pre_higherunit(r1);
	}
	return(nextcarry);	/* return the final carry flag bit */
}	/* mp_rotate_left */
#endif  /* mp_rotate_left */

/************** end of primitives that should be in assembly *************/


void mp_move_and_shift_left_bits(unit *dst, const unit *src, int bits) {
	int precision;
	int tmpbits;

	precision = global_precision;
	make_lsbptr(dst, precision);
	make_lsbptr(src, precision);
	for (tmpbits = bits / UNITSIZE; precision && tmpbits--; ) {
		*dst = 0;
		pre_higherunit(dst);
		precision--;
	}
	bits &= (UNITSIZE - 1);
	if (bits) {
		unit carry = 0;
		tmpbits = UNITSIZE - bits;
		while (precision--) {
			unit nextcarry;

			nextcarry = *src >> tmpbits;
			*dst = (*src << bits) | carry;
			carry = nextcarry;
			pre_higherunit(dst);
			pre_higherunit(src);
		}
	} else {
		while (precision--) {
			*dst = *src;
			pre_higherunit(dst);
			pre_higherunit(src);
		}
	}
}

/*	The mp_shift_right_bits function is not called in any time-critical
	situations in public-key cryptographic functions, so it doesn't
	need to be coded in assembly language.
*/
void mp_shift_right_bits(register unitptr r1,register short bits)
	/*	multiprecision shift right bits, result in r1.
		bits is how many bits to shift, must be <= UNITSIZE.
	*/
{	register short precision;	/* number of units to shift */
	register unit carry,nextcarry,bitmask;
	register short unbits;
	if (bits==0) return;	/* shift zero bits is a no-op */
	carry = 0;
	bitmask = power_of_2(bits)-1;
	unbits = UNITSIZE-bits;		/* shift bits must be <= UNITSIZE */
	precision = global_precision;
	make_msbptr(r1,precision);
	if (bits == UNITSIZE) {
		while (precision--)  {
			nextcarry = *r1;
			*r1 = carry;
			carry = nextcarry;
			pre_lowerunit(r1);
		}
	} else {
		while (precision--)
		{	nextcarry = *r1 & bitmask;
			*r1 >>= bits;
			*r1 |= carry << unbits;
			carry = nextcarry;
			pre_lowerunit(r1);
		}
	}
}	/* mp_shift_right_bits */


#ifndef mp_compare
short mp_compare(const unit* r1, const unit* r2)
/*	Compares multiprecision integers *r1, *r2, and returns:
	-1 iff *r1 < *r2
	 0 iff *r1 == *r2
	+1 iff *r1 > *r2
*/
{	register short precision;	/* number of units to compare */

	precision = global_precision;
	make_msbptr(r1,precision);
	make_msbptr(r2,precision);
	do
	{	if (*r1 < *r2)
			return(-1);
		if (*post_lowerunit(r1) > *post_lowerunit(r2))
			return(1);
	} while (--precision);
	return(0);	/*  *r1 == *r2  */
}	/* mp_compare */
#endif /* mp_compare */


boolean mp_inc(register unitptr r)
	/* Increment multiprecision integer r. */
{	register short precision;
	precision = global_precision;
	make_lsbptr(r,precision);
	do
	{	if ( ++(*r) ) return(0);	/* no carry */
		post_higherunit(r);
	} while (--precision);
	return(1);		/* return carry set */
}	/* mp_inc */


boolean mp_dec(register unitptr r)
	/* Decrement multiprecision integer r. */
{	register short precision;
	precision = global_precision;
	make_lsbptr(r,precision);
	do
	{	if ( (signedunit) (--(*r)) != (signedunit) -1 )
			return(0);	/* no borrow */
		post_higherunit(r);
	} while (--precision);
	return(1);		/* return borrow set */
}	/* mp_dec */


void mp_neg(register unitptr r)
	/* Compute 2's complement, the arithmetic negative, of r */
{	register short precision;	/* number of units to negate */
	precision = global_precision;
	mp_dec(r);	/* 2's complement is 1's complement plus 1 */
	do	/* now do 1's complement */
	{	*r = ~(*r);
		r++;
	} while (--precision);
}	/* mp_neg */

#ifndef mp_move
void mp_move(register unitptr dst,register unitptr src)
{	register short precision;	/* number of units to move */
	precision = global_precision;
	do { *dst++ = *src++; } while (--precision);
}	/* mp_move */
#endif /* mp_move */

void mp_init(register unitptr r, word16 value)
	/* Init multiprecision register r with short value. */
{	/* Note that mp_init doesn't extend sign bit for >32767 */

	unitfill0( r, global_precision);
	make_lsbptr(r,global_precision);
	*post_higherunit(r) = value;
#if UNITSIZE < 16
	*post_higherunit(r) = value >> UNITSIZE;
#endif
}	/* mp_init */


short significance(const unit* r)
	/* Returns number of significant units in r */
{	register short precision;
	precision = global_precision;
	make_msbptr(r,precision);
	do
	{	if (*post_lowerunit(r))
			return(precision);
	} while (--precision);
	return(precision);
}	/* significance */


#ifndef unitfill0
void unitfill0(unitptr r,word16 unitcount)
	/* Zero-fill the unit buffer r. */
{	while (unitcount--) *r++ = 0;
}	/* unitfill0 */
#endif  /* unitfill0 */


int mp_udiv(register unitptr remainder,register unitptr quotient,
	register unitptr dividend,register unitptr divisor)
	/* Unsigned divide, treats both operands as positive. */
{	int bits;
	short dprec;
	register unit bitmask;
	if (testeq(divisor,0))
		return(-1);	/* zero divisor means divide error */
	mp_init0(remainder);
	mp_init0(quotient);
	/* normalize and compute number of bits in dividend first */
	init_bitsniffer(dividend,bitmask,dprec,bits);
	/* rescale quotient to same precision (dprec) as dividend */
	rescale(quotient,global_precision,dprec);
	make_msbptr(quotient,dprec);

	while (bits--)
	{	mp_rotate_left(remainder,(boolean)(sniff_bit(dividend,bitmask)!=0));
		if (mp_compare(remainder,divisor) >= 0)
		{	mp_sub(remainder,divisor);
			stuff_bit(quotient,bitmask);
		}
		bump_2bitsniffers(dividend,quotient,bitmask);
	}
	return(0);
} /* mp_udiv */


#ifdef UPTON_OR_SMITH
#define RECIPMARGIN 0	/* extra margin bits used by mp_recip() */

int mp_recip(register unitptr quotient,register unitptr divisor)
	/* Compute reciprocal (quotient) as 1/divisor.  Used by faster modmult. */
{	int bits;
	short qprec;
	register unit bitmask;
	unit remainder[MAX_UNIT_PRECISION];
	if (testeq(divisor,0))
		return(-1);	/* zero divisor means divide error */
	mp_init0(remainder);
	mp_init0(quotient);

	/* normalize and compute number of bits in quotient first */
	bits = countbits(divisor) + RECIPMARGIN;
	bitmask = bitmsk(bits);		/* bitmask within a single unit */
	qprec = bits2units(bits+1);
	mp_setbit(remainder,(bits-RECIPMARGIN)-1);
	/* rescale quotient to precision of divisor + RECIPMARGIN bits */
	rescale(quotient,global_precision,qprec);
	make_msbptr(quotient,qprec);

	while (bits--)
	{	mp_shift_left(remainder);
		if (mp_compare(remainder,divisor) >= 0)
		{	mp_sub(remainder,divisor);
			stuff_bit(quotient,bitmask);
		}
		bump_bitsniffer(quotient,bitmask);
	}
	mp_init0(remainder);	/* burn sensitive data left on stack */
	return(0);
} /* mp_recip */
#endif	/* UPTON_OR_SMITH */


int mp_div(register unitptr remainder,register unitptr quotient,
	register unitptr dividend,register unitptr divisor)
	/* Signed divide, either or both operands may be negative. */
{	boolean dvdsign,dsign;
	int status;
	dvdsign = (boolean)(mp_tstminus(dividend)!=0);
	dsign = (boolean)(mp_tstminus(divisor)!=0);
	if (dvdsign) mp_neg(dividend);
	if (dsign) mp_neg(divisor);
	status = mp_udiv(remainder,quotient,dividend,divisor);
	if (dvdsign) mp_neg(dividend);		/* repair caller's dividend */
	if (dsign) mp_neg(divisor);		/* repair caller's divisor */
	if (status<0) return(status);		/* divide error? */
	if (dvdsign) mp_neg(remainder);
	if (dvdsign ^ dsign) mp_neg(quotient);
	return(status);
} /* mp_div */


word16 mp_shortdiv(register unitptr quotient,
	register unitptr dividend,register word16 divisor)
/*	This function does a fast divide and mod on a multiprecision dividend
	using a short integer divisor returning a short integer remainder.
	This is an unsigned divide.  It treats both operands as positive.
	It is used mainly for faster printing of large numbers in base 10.
*/
{	int bits;
	short dprec;
	register unit bitmask;
	register word16 remainder;
	if (!divisor)	/* if divisor == 0 */
		return(-1);	/* zero divisor means divide error */
	remainder=0;
	mp_init0(quotient);
	/* normalize and compute number of bits in dividend first */
	init_bitsniffer(dividend,bitmask,dprec,bits);
	/* rescale quotient to same precision (dprec) as dividend */
	rescale(quotient,global_precision,dprec);
	make_msbptr(quotient,dprec);

	while (bits--)
	{	remainder <<= 1;
		if (sniff_bit(dividend,bitmask))
			remainder++;
		if (remainder >= divisor)
		{	remainder -= divisor;
			stuff_bit(quotient,bitmask);
		}
		bump_2bitsniffers(dividend,quotient,bitmask);
	}
	return(remainder);
} /* mp_shortdiv */


int mp_mod(register unitptr remainder,
	const unit* dividend, const unit* divisor)
	/* Unsigned divide, treats both operands as positive. */
{	int bits;
	short dprec;
	register unit bitmask;
	if (testeq(divisor,0))
		return(-1);	/* zero divisor means divide error */
	mp_init0(remainder);
	/* normalize and compute number of bits in dividend first */
	init_bitsniffer(dividend,bitmask,dprec,bits);

	while (bits--)
	{	mp_rotate_left(remainder,(boolean)(sniff_bit(dividend,bitmask)!=0));
		msub(remainder,divisor);
		bump_bitsniffer(dividend,bitmask);
	}
	return(0);
} /* mp_mod */


word16 mp_shortmod(const unit* dividend,register word16 divisor)
/*	This function does a fast mod operation on a multiprecision dividend
	using a short integer modulus returning a short integer remainder.
	This is an unsigned divide.  It treats both operands as positive.
	It is used mainly for fast sieve searches for large primes.
*/
{	int bits;
	short dprec;
	register unit bitmask;
	register word16 remainder;
	if (!divisor)	/* if divisor == 0 */
		return(-1);	/* zero divisor means divide error */
	remainder=0;
	/* normalize and compute number of bits in dividend first */
	init_bitsniffer(dividend,bitmask,dprec,bits);

	while (bits--)
	{	remainder <<= 1;
		if (sniff_bit(dividend,bitmask))
			remainder++;
		if (remainder >= divisor) remainder -= divisor;
		bump_bitsniffer(dividend,bitmask);
	}
	return(remainder);
} /* mp_shortmod */



#ifdef COMB_MULT	/* use faster "comb" multiply algorithm */
	/* We are skipping this code because it has a bug... */

int mp_mult(register unitptr prod,
	const unit *multiplicand, const unit *multiplier)
	/*	Computes multiprecision prod = multiplicand * multiplier */
{	/*	Uses interleaved comb multiply algorithm.
		This improved multiply more than twice as fast as a Russian
		peasant multiply, because it does a lot fewer shifts.
		Must have global_precision set to the size of the multiplicand
		plus UNITSIZE-1 SLOP_BITS.  Produces a product that is the sum
		of the lengths of the multiplier and multiplicand.

		BUG ALERT:  Unfortunately, this code has a bug.  It fails for
		some numbers.  One such example:
		x=   59DE 60CE 2345 8091 A02B 2A1C DBC3 8BE5
		x*x= 59DE 60CE 2345 26B3 993B 67A5 2499 0B7D
		     52C8 CDC7 AFB3 61C8 243C 741B
		--which is obviously wrong.  The answer should be:
		x*x= 1F8C 607B 5EA6 C061 2714 04A9 A0C6 A17A
		     C9AB 6095 C62F 3756 3843 E4D0 3950 7AD9
		We'll have to fix this some day.  In the meantime, we'll
		just have the compiler skip over this code.

		BUG NOTE: Possibly fixed.  Needs testing.
	*/
	int bits;
	register unit bitmask;
	unitptr product, mplier, temp;
	short mprec,mprec2;
	unit mplicand[MAX_UNIT_PRECISION];

	/* better clear full width--double precision */
	mp_init(prod+tohigher(global_precision),0);

	if (testeq(multiplicand,0))
		return(0);	/* zero multiplicand means zero product */

	mp_move(mplicand,multiplicand);	/* save it from damage */

	normalize(multiplier,mprec);
	if (!mprec)
		return(0);

	make_lsbptr(multiplier,mprec);
	bitmask = 1;	/* start scan at LSB of multiplier */

	do	/* UNITSIZE times */
	{	/* do for bits 0-15 */
		product = prod;
		mplier = multiplier;
		mprec2 = mprec;
		while (mprec2--)	/* do for each word in multiplier */
		{
			if (sniff_bit(mplier,bitmask))
			{	if (mp_addc(product,multiplicand,0))	/* ripple carry */
				{	/* After 1st time thru, this is rarely encountered. */
					temp = msbptr(product,global_precision);
					pre_higherunit(temp);
					/* temp now points to LSU of carry region. */
					unmake_lsbptr(temp,global_precision);
					mp_inc(temp);
				}	/* ripple carry */
			}
			pre_higherunit(mplier);
			pre_higherunit(product);
		}
		if (!(bitmask <<= 1))
			break;
		mp_shift_left(multiplicand);

	} while (TRUE);

	mp_move(multiplicand,mplicand);	/* recover */

	return(0);	/* normal return */
}	/* mp_mult */

#endif	/* COMB_MULT */


/*	Because the "comb" multiply has a bug, we will use the slower
	Russian peasant multiply instead.  Fortunately, the mp_mult
	function is not called from any time-critical code.
*/

int mp_mult(register unitptr prod,
	const unit *multiplicand, const unit *multiplier)
	/* Computes multiprecision prod = multiplicand * multiplier */
{	/* Uses "Russian peasant" multiply algorithm. */
	int bits;
	register unit bitmask;
	short mprec;
	mp_init(prod,0);
	if (testeq(multiplicand,0))
		return(0);	/* zero multiplicand means zero product */
	/* normalize and compute number of bits in multiplier first */
	init_bitsniffer(multiplier,bitmask,mprec,bits);

	while (bits--)
	{	mp_shift_left(prod);
		if (sniff_bit(multiplier,bitmask))
			mp_add(prod,multiplicand);
		bump_bitsniffer(multiplier,bitmask);
	}
	return(0);
}	/* mp_mult */



/*	mp_modmult computes a multiprecision multiply combined with a
	modulo operation.  This is the most time-critical function in
	this multiprecision arithmetic library for performing modulo
	exponentiation.  We experimented with different versions of modmult,
	depending on the machine architecture and performance requirements.
	We will either use a Russian Peasant modmult (peasant_modmult), 
	Charlie Merritt's modmult (merritt_modmult), Jimmy Upton's
	modmult (upton_modmult), or Thad Smith's modmult (smith_modmult).
	On machines with a hardware atomic multiply instruction,
	Smith's modmult is fastest.  It can utilize assembly subroutines to
	speed up the hardware multiply logic and trial quotient calculation.
	If the machine lacks a fast hardware multiply, Merritt's modmult
	is preferred, which doesn't call any assembly multiply routine.
	We use the alias names mp_modmult, stage_modulus, and modmult_burn
	for the corresponding true names, which depend on what flavor of
	modmult we are using.

	Before making the first call to mp_modmult, you must set up the
	modulus-dependant precomputated tables by calling stage_modulus.
	After making all the calls to mp_modmult, you call modmult_burn to
	erase the tables created by stage_modulus that were left in memory.
*/

#ifdef COUNTMULTS
/* "number of modmults" counters, used for performance studies. */
static unsigned int tally_modmults = 0;
static unsigned int tally_modsquares = 0;
#endif	/* COUNTMULTS */


#ifdef PEASANT
/* Conventional Russian peasant multiply with modulo algorithm. */

static const unit* pmodulus = 0;	/* used only by mp_modmult */

int stage_peasant_modulus(const unit* n)
/*	Must pass modulus to stage_modulus before calling modmult.
	Assumes that global_precision has already been adjusted to the
	size of the modulus, plus SLOP_BITS.
*/
{	/* For this simple version of modmult, just copy unit pointer. */
	pmodulus = n;
	return(0);	/* normal return */
}	/* stage_peasant_modulus */


int peasant_modmult(register unitptr prod,
	unitptr multiplicand,register unitptr multiplier)
{	/*	"Russian peasant" multiply algorithm, combined with a modulo
		operation.  This is a simple naive replacement for Merritt's
		faster modmult algorithm.  References global unitptr "modulus".
		Computes:  prod = (multiplicand*multiplier) mod modulus
		WARNING: All the arguments must be less than the modulus!
	*/
	int bits;
	register unit bitmask;
	short mprec;
	mp_init(prod,0);
/*	if (testeq(multiplicand,0))
		return(0); */	/* zero multiplicand means zero product */
	/* normalize and compute number of bits in multiplier first */
	init_bitsniffer(multiplier,bitmask,mprec,bits);

	while (bits--)
	{	mp_shift_left(prod);
		msub(prod,pmodulus);	/* turns mult into modmult */
		if (sniff_bit(multiplier,bitmask))
		{	mp_add(prod,multiplicand);
			msub(prod,pmodulus);	/* turns mult into modmult */
		}
		bump_bitsniffer(multiplier,bitmask);
	}
	return(0);
}	/* peasant_modmult */


/*	If we are using a version of mp_modmult that uses internal tables
	in memory, we have to call modmult_burn() at the end of mp_modexp.
	This is so that no sensitive data is left in memory after the program
	exits.  The Russian peasant method doesn't use any such tables.
*/
void peasant_burn(void)
/*	Alias for modmult_burn, called only from mp_modexp().  Destroys
	internal modmult tables.  This version does nothing because no
	tables are used by the Russian peasant modmult. */
{ }	/* peasant_burn */

#endif	/* PEASANT */


#ifdef MERRITT
/*=========================================================================*/
/*
	This is Charlie Merritt's MODMULT algorithm, implemented in C by PRZ.
	Also refined by Zhahai Stewart to reduce the number of subtracts.
    Modified by Raymond Brand to reduce the number of SLOP_BITS by 1.
	It performs a multiply combined with a modulo operation, without
	going into "double precision".  It is faster than the Russian peasant
	method, and still works well on machines that lack a fast hardware
	multiply instruction.
*/

/*	The following support functions, macros, and data structures
	are used only by Merritt's modmult algorithm... */

static void mp_lshift_unit(register unitptr r1)
/*	Shift r1 1 whole unit to the left.  Used only by modmult function. */
{	register short precision;
	register unitptr r2;
	precision = global_precision;
	make_msbptr(r1,precision);
	r2 = r1;
	while (--precision)
		*post_lowerunit(r1) = *pre_lowerunit(r2);
	*r1 = 0;
}	/* mp_lshift_unit */


/* moduli_buf contains shifted images of the modulus, set by stage_modulus */
static unit moduli_buf[UNITSIZE-1][MAX_UNIT_PRECISION] = {0};
static unitptr moduli[UNITSIZE] = /* contains pointers into moduli_buf */
{	0
	,&moduli_buf[ 0][0], &moduli_buf[ 1][0], &moduli_buf[ 2][0], &moduli_buf[ 3][0],
	 &moduli_buf[ 4][0], &moduli_buf[ 5][0], &moduli_buf[ 6][0]
#if UNITSIZE > 8
	,&moduli_buf[ 7][0]
	,&moduli_buf[ 8][0], &moduli_buf[ 9][0], &moduli_buf[10][0], &moduli_buf[11][0], 
	 &moduli_buf[12][0], &moduli_buf[13][0], &moduli_buf[14][0]
#if UNITSIZE > 16
	,&moduli_buf[15][0]
	,&moduli_buf[16][0], &moduli_buf[17][0], &moduli_buf[18][0], &moduli_buf[19][0], 
	 &moduli_buf[20][0], &moduli_buf[21][0], &moduli_buf[22][0], &moduli_buf[23][0], 
	 &moduli_buf[24][0], &moduli_buf[25][0], &moduli_buf[26][0], &moduli_buf[27][0], 
	 &moduli_buf[28][0], &moduli_buf[29][0], &moduli_buf[30][0]
#endif	/* UNITSIZE > 16 */
#endif	/* UNITSIZE > 8 */
};

/*	To optimize msubs, need following 2 unit arrays, each filled
	with the most significant unit and next-to-most significant unit
	of the preshifted images of the modulus. */
static unit msu_moduli[UNITSIZE] = {0}; /* most signif. unit */
static unit nmsu_moduli[UNITSIZE] = {0}; /* next-most signif. unit */

/*	mpdbuf contains preshifted images of the multiplicand, mod n.
	It is used only by mp_modmult.  It could be staticly declared
	inside of mp_modmult, but we put it outside mp_modmult so that
	it can be wiped clean by modmult_burn(), which is called at the
	end of mp_modexp.  This is so that no sensitive data is left in
	memory after the program exits.
*/
static unit mpdbuf[UNITSIZE-1][MAX_UNIT_PRECISION] = {0};


static void stage_mp_images(unitptr images[UNITSIZE],unitptr r)
/*	Computes UNITSIZE images of r, each shifted left 1 more bit.
	Used only by modmult function.
*/
{	short int i;
	images[0] = r;	/* no need to move the first image, just copy ptr */
	for (i=1; i<UNITSIZE; i++)
	{	mp_move(images[i],images[i-1]);
		mp_shift_left(images[i]);
		msub(images[i],moduli[0]);
	}
}	/* stage_mp_images */


int stage_merritt_modulus(const unit* n)
/*	Computes UNITSIZE images of modulus, each shifted left 1 more bit.
	Before calling mp_modmult, you must first stage the moduli images by
	calling stage_modulus.  n is the pointer to the modulus.
	Assumes that global_precision has already been adjusted to the
	size of the modulus, plus SLOP_BITS.
*/
{	short int i;
	unitptr msu;	/* ptr to most significant unit, for faster msubs */
	moduli[0] = n;	/* no need to move the first image, just copy ptr */

	/* used by optimized msubs macro... */
	msu = msbptr(n,global_precision);	/* needed by msubs */
	msu_moduli[0] = *post_lowerunit(msu);
	nmsu_moduli[0] = *msu;

	for (i=1; i<UNITSIZE; i++)
	{	mp_move(moduli[i],moduli[i-1]);
		mp_shift_left(moduli[i]);

		/* used by optimized msubs macro... */
		msu = msbptr(moduli[i],global_precision);	/* needed by msubs */
		msu_moduli[i] = *post_lowerunit(msu);	/* for faster msubs */
		nmsu_moduli[i] = *msu;
	}
	return(0);	/* normal return */
}	/* stage_merritt_modulus */


/* The following macros, sniffadd and msubs, are used by modmult... */
#define sniffadd(i) if (*multiplier & power_of_2(i))  mp_add(prod,mpd[i])

/* Unoptimized msubs macro (msubs0) follows... */
/* #define msubs0(i) msub(prod,moduli[i])
*/

/*	To optimize msubs, compare the most significant units of the
	product and the shifted modulus before deciding to call the full
	mp_compare routine.  Better still, compare the upper two units
	before deciding to call mp_compare.
	Optimization of msubs relies heavily on standard C left-to-right
	parsimonious evaluation of logical expressions.
*/

/* Partially-optimized msubs macro (msubs1) follows... */
/* #define msubs1(i) if ( \
  ((p_m = (*msu_prod-msu_moduli[i])) >= 0) && \
  (p_m || (mp_compare(prod,moduli[i]) >= 0) ) \
  ) mp_sub(prod,moduli[i])
*/

/* Fully-optimized msubs macro (msubs2) follows... */
#define msubs(i) if (((p_m = *msu_prod-msu_moduli[i])>0) || ( \
 (p_m==0) && ( (*nmsu_prod>nmsu_moduli[i]) || ( \
 (*nmsu_prod==nmsu_moduli[i]) && ((mp_compare(prod,moduli[i]) >= 0)) ))) ) \
 mp_sub(prod,moduli[i])


int merritt_modmult(register unitptr prod,
	unitptr multiplicand,register unitptr multiplier)
	/*	Performs combined multiply/modulo operation.
		Computes:  prod = (multiplicand*multiplier) mod modulus
		WARNING: All the arguments must be less than the modulus!
		Assumes the modulus has been predefined by first calling
		stage_modulus.
	*/
{
	/* p_m, msu_prod, and nmsu_prod are used by the optimized msubs macro...*/
	register signedunit p_m;
	register unitptr msu_prod;	/* ptr to most significant unit of product */
	register unitptr nmsu_prod;	/* next-most signif. unit of product */
	short mprec;		/* precision of multiplier, in units */
	/*	Array mpd contains a list of pointers to preshifted images of
		the multiplicand: */
	static unitptr mpd[UNITSIZE] =
	{	 0,              &mpdbuf[ 0][0], &mpdbuf[ 1][0], &mpdbuf[ 2][0],
		 &mpdbuf[ 3][0], &mpdbuf[ 4][0], &mpdbuf[ 5][0], &mpdbuf[ 6][0]
#if UNITSIZE > 8
		,&mpdbuf[ 7][0], &mpdbuf[ 8][0], &mpdbuf[ 9][0], &mpdbuf[10][0],
		 &mpdbuf[11][0], &mpdbuf[12][0], &mpdbuf[13][0], &mpdbuf[14][0]
#if UNITSIZE > 16
		,&mpdbuf[15][0], &mpdbuf[16][0], &mpdbuf[17][0], &mpdbuf[18][0],
		 &mpdbuf[19][0], &mpdbuf[20][0], &mpdbuf[21][0], &mpdbuf[22][0],
		 &mpdbuf[23][0], &mpdbuf[24][0], &mpdbuf[25][0], &mpdbuf[26][0],
		 &mpdbuf[27][0], &mpdbuf[28][0], &mpdbuf[29][0], &mpdbuf[30][0]
#endif	/* UNITSIZE > 16 */
#endif	/* UNITSIZE > 8 */
	};

	/* Compute preshifted images of multiplicand, mod n: */
	stage_mp_images(mpd,multiplicand);

	/* To optimize msubs, set up msu_prod and nmsu_prod: */
	msu_prod = msbptr(prod,global_precision); /* Get ptr to MSU of prod */
	nmsu_prod = msu_prod;
	post_lowerunit(nmsu_prod); /* Get next-MSU of prod */

	/*	To understand this algorithm, it would be helpful to first
		study the conventional Russian peasant modmult algorithm.
		This one does about the same thing as Russian peasant, but
		is organized differently to save some steps.  It loops
		through the multiplier a word (unit) at a time, instead of
		a bit at a time.  It word-shifts the product instead of
		bit-shifting it, so it should be faster.  It also does about
		half as many subtracts as Russian peasant.
	*/

	mp_init(prod,0);	/* Initialize product to 0. */

	/*	The way mp_modmult is actually used in cryptographic
		applications, there will NEVER be a zero multiplier or
		multiplicand.  So there is no need to optimize for that
		condition.
	*/
/*	if (testeq(multiplicand,0))
		return(0); */	/* zero multiplicand means zero product */
	/* Normalize and compute number of units in multiplier first: */
	normalize(multiplier,mprec);
	if (mprec==0)	/* if precision of multiplier is 0 */
		return(0);	/* zero multiplier means zero product */
	make_msbptr(multiplier,mprec); /* start at MSU of multiplier */

	while (mprec--)	/* Loop for the number of units in the multiplier */
	{
		/*	Shift the product one whole unit to the left.
			This is part of the multiply phase of modmult.
		*/

		mp_lshift_unit(prod);

		/*      The product may have grown by as many as UNITSIZE
			bits.  That's why we have global_precision set to the
			size of the modulus plus UNITSIZE slop bits.
			Now reduce the product back down by conditionally
			subtracting the preshifted images of the modulus.
			This is part of the modulo reduction phase of modmult. 
			The following loop is unrolled for speed, using macros...

		for (i=UNITSIZE-1; i>=LOG_UNITSIZE; i--)
			if (mp_compare(prod,moduli[i]) >= 0)
				mp_sub(prod,moduli[i]);
		*/

#if UNITSIZE > 8
#if UNITSIZE > 16
		msubs(31);
		msubs(30);
		msubs(29);
		msubs(28);
		msubs(27);
		msubs(26);
		msubs(25);
		msubs(24);
		msubs(23);
		msubs(22);
		msubs(21);
		msubs(20);
		msubs(19);
		msubs(18);
		msubs(17);
		msubs(16);
#endif  /* UNITSIZE > 16 */
		msubs(15);
		msubs(14);
		msubs(13);
		msubs(12);
		msubs(11);
		msubs(10);
		msubs(9);
		msubs(8);
#endif  /* UNITSIZE > 8 */
		msubs(7);
		msubs(6);
		msubs(5);
#if UNITSIZE < 32
		msubs(4);
#if UNITSIZE < 16
		msubs(3);
#endif
#endif

		/*	Sniff each bit in the current unit of the multiplier, 
			and conditionally add the corresponding preshifted 
			image of the multiplicand to the product.
			This is also part of the multiply phase of modmult.

			The following loop is unrolled for speed, using macros...

		for (i=UNITSIZE-1; i>=0; i--)
		   if (*multiplier & power_of_2(i))
				mp_add(prod,mpd[i]);
		*/
#if UNITSIZE > 8
#if UNITSIZE > 16
		sniffadd(31);
		sniffadd(30);
		sniffadd(29);
		sniffadd(28);
		sniffadd(27);
		sniffadd(26);
		sniffadd(25);
		sniffadd(24);
		sniffadd(23);
		sniffadd(22);
		sniffadd(21);
		sniffadd(20);
		sniffadd(19);
		sniffadd(18);
		sniffadd(17);
		sniffadd(16);
#endif	/* UNITSIZE > 16 */
		sniffadd(15);
		sniffadd(14);
		sniffadd(13);
		sniffadd(12);
		sniffadd(11);
		sniffadd(10);
		sniffadd(9);
		sniffadd(8);
#endif	/* UNITSIZE > 8 */
		sniffadd(7);
		sniffadd(6);
		sniffadd(5);
		sniffadd(4);
		sniffadd(3);
		sniffadd(2);
		sniffadd(1);
		sniffadd(0);

		/*	The product may have grown by as many as LOG_UNITSIZE+1
			bits.
			Now reduce the product back down by conditionally 
			subtracting LOG_UNITSIZE+1 preshifted images of the 
			modulus.  This is the modulo reduction phase of modmult.

			The following loop is unrolled for speed, using macros...

		for (i=LOG_UNITSIZE; i>=0; i--)
			if (mp_compare(prod,moduli[i]) >= 0) 
				mp_sub(prod,moduli[i]); 
		*/

#if UNITSIZE > 8
#if UNITSIZE > 16
		msubs(5); 
#endif
		msubs(4);
#endif
		msubs(3); 
		msubs(2); 
		msubs(1); 
		msubs(0);

		/* Bump pointer to next lower unit of multiplier: */
		post_lowerunit(multiplier);

	}	/* Loop for the number of units in the multiplier */

	return(0);	/* normal return */

}	/* merritt_modmult */


#undef msubs
#undef sniffadd


/*	Merritt's mp_modmult function leaves some internal tables in memory,
	so we have to call modmult_burn() at the end of mp_modexp.
	This is so that no cryptographically sensitive data is left in memory
	after the program exits.
*/
void merritt_burn(void)
/*	Alias for modmult_burn, merritt_burn() is called only by mp_modexp. */
{	unitfill0(&(mpdbuf[0][0]),(UNITSIZE-1)*MAX_UNIT_PRECISION);
	unitfill0(&(moduli_buf[0][0]),(UNITSIZE-1)*MAX_UNIT_PRECISION);
	unitfill0(msu_moduli,UNITSIZE);
	unitfill0(nmsu_moduli,UNITSIZE);
} /* merritt_burn() */

/******* end of Merritt's MODMULT stuff. *******/
/*=========================================================================*/
#endif	/* MERRITT */


#ifdef UPTON_OR_SMITH	/* Used by Upton's and Smith's modmult algorithms */

/*	Jimmy Upton's multiprecision modmult algorithm in C.
	Performs a multiply combined with a modulo operation.

	The following support functions and data structures
	are used only by Upton's modmult algorithm...
*/

short munit_prec;	/* global_precision expressed in MULTUNITs */

/*	Note that since the SPARC CPU has no hardware integer multiply
	instruction, there is not that much advantage in having an
	assembly version of mp_smul on that machine.  It might be faster
	to use Merritt's modmult instead of Upton's modmult on the SPARC.
*/

/*
	Multiply the single-word multiplier times the multiprecision integer
	in multiplicand, accumulating result in prod.  The resulting
	multiprecision prod will be 1 word longer than the multiplicand.
	multiplicand is munit_prec words long.  We add into prod, so caller
	should zero it out first.  For best results, this time-critical
	function should be implemented in assembly.
	NOTE:  Unlike other functions in the multiprecision arithmetic
	library, both multiplicand and prod are pointing at the LSB,
	regardless of byte order of the machine.  On an 80x86, this makes
	no difference.  But if this assembly function is implemented
	on a 680x0, it becomes important.
*/
/*	Note that this has been modified from the previous version to allow
	better support for Smith's modmult:
		The final carry bit is added to the existing product
		array, rather than simply stored.
*/

#ifndef mp_smul
#if MULTUNITSIZE == 32 && defined(__i386__)
static inline void mp_smul (MULTUNIT *prod, const MULTUNIT *multiplicand, MULTUNIT multiplier)
{
	MULTUNIT carry;
	int i;
	
	carry = 0;
	for (i=munit_prec; i!=0; --i)
	{
		unsigned int dummy;
		
		asm volatile (
			"mull %6\n\t"
			"addl %3,%2\n\t"
			"adcl $0,%0\n\t"
			"addl %2,(%1)\n\t"
			"adcl $0,%0\n\t"
			:
			"=d"(carry), "=D"(prod), "=a"(dummy) :
			"c"(carry), "1"(prod), "2"(*multiplicand), "m"(multiplier)
		);
		post_higherunit(multiplicand);
		post_higherunit(prod);
	}
	/* Add carry to the next higher word of product / dividend */
	*prod += (MULTUNIT)carry;
}	/* mp_smul */
#else
static inline void mp_smul (MULTUNIT *prod, const MULTUNIT *multiplicand, MULTUNIT multiplier)
{
	unsigned long lmult = (unsigned long)multiplier;
	unsigned long carry;
	int i;
	
	carry = 0;
	for (i=munit_prec; i!=0; --i)
	{
		carry += lmult * *post_higherunit(multiplicand);
		carry += *prod;
		*post_higherunit(prod) = (MULTUNIT)carry;
		carry = carry >> MULTUNITSIZE;
	}
	/* Add carry to the next higher word of product / dividend */
	*prod += (MULTUNIT)carry;
}	/* mp_smul */
#endif
#endif

/*	mp_dmul is a double-precision multiply multiplicand times multiplier,
	result into prod.  prod must be pointing at a "double precision"
	buffer.  E.g. If global_precision is 10 words, prod must be
	pointing at a 20-word buffer.
*/
#ifndef mp_dmul
void mp_dmul (unitptr prod, const unit* multiplicand, const unit* multiplier)
{
	register	int i;
	const		MULTUNIT *p_multiplicand, *p_multiplier;
	register	MULTUNIT *prodp;


	unitfill0(prod,global_precision*2);	/* Pre-zero prod */
	/* Calculate precision in units of MULTUNIT */
	munit_prec = global_precision * UNITSIZE / MULTUNITSIZE;
	p_multiplicand = (const MULTUNIT *)multiplicand;
	p_multiplier = (const MULTUNIT *)multiplier;
	prodp = (MULTUNIT *)prod;
	make_lsbptr(p_multiplicand,munit_prec);
	make_lsbptr(p_multiplier,munit_prec);
	make_lsbptr(prodp,munit_prec*2);
	/* Multiply multiplicand by each word in multiplier, accumulating prod: */
	for (i=0; i<munit_prec; ++i)
		mp_smul (post_higherunit(prodp),
			p_multiplicand, *post_higherunit(p_multiplier));
}	/* mp_dmul */
#endif  /* mp_dmul */

static unit ALIGN modulus[MAX_UNIT_PRECISION];
static short nbits;			/* number of modulus significant bits */
#endif /* UPTON_OR_SMITH */

#ifdef UPTON

/*	These scratchpad arrays are used only by upton_modmult (mp_modmult).
	Some of them could be staticly declared inside of mp_modmult, but we
	put them outside mp_modmult so that they can be wiped clean by
	modmult_burn(), which is called at the end of mp_modexp.  This is
	so that no sensitive data is left in memory after the program exits.
*/

static unit ALIGN reciprocal[MAX_UNIT_PRECISION];
static unit ALIGN dhi[MAX_UNIT_PRECISION];
static unit ALIGN d_data[MAX_UNIT_PRECISION*2];     /* double-wide register */
static unit ALIGN e_data[MAX_UNIT_PRECISION*2];     /* double-wide register */
static unit ALIGN f_data[MAX_UNIT_PRECISION*2];     /* double-wide register */

static short nbitsDivUNITSIZE;
static short nbitsModUNITSIZE;

/*	stage_upton_modulus() is aliased to stage_modulus().
	Prepare for an Upton modmult.  Calculate the reciprocal of modulus,
	and save both.  Note that reciprocal will have one more bit than
	modulus.
	Assumes that global_precision has already been adjusted to the
	size of the modulus, plus SLOP_BITS.
*/
int stage_upton_modulus(const unit* n)
{
	mp_move(modulus, n);
	mp_recip(reciprocal, modulus);
	nbits = countbits(modulus);
	nbitsDivUNITSIZE = nbits / UNITSIZE;
	nbitsModUNITSIZE = nbits % UNITSIZE;
	return(0);	/* normal return */
}	/* stage_upton_modulus */


/*	Upton's algorithm performs a multiply combined with a modulo operation.
	Computes:  prod = (multiplicand*multiplier) mod modulus
	WARNING: All the arguments must be less than the modulus!
	References global unitptr modulus and reciprocal.
	The reciprocal of modulus is 1 bit longer than the modulus.
	upton_modmult() is aliased to mp_modmult().
*/
int upton_modmult (unitptr prod, unitptr multiplicand, unitptr multiplier)
{
	unitptr d = d_data;
	unitptr d1 = d_data;
	unitptr e = e_data;
	unitptr f = f_data;
	short orig_precision;

	orig_precision = global_precision;
	mp_dmul (d,multiplicand,multiplier);

	/* Throw off low nbits of d */
#ifndef HIGHFIRST
	d1 = d + nbitsDivUNITSIZE;
#else
	d1 = d + orig_precision - nbitsDivUNITSIZE;
#endif
	mp_move(dhi, d1);	/* Don't screw up d, we need it later */
	mp_shift_right_bits(dhi, nbitsModUNITSIZE);

	mp_dmul (e,dhi,reciprocal);	/* Note - reciprocal has nbits+1 bits */

#ifndef HIGHFIRST
	e += nbitsDivUNITSIZE;
#else
	e += orig_precision - nbitsDivUNITSIZE;
#endif
	mp_shift_right_bits(e, nbitsModUNITSIZE);

	mp_dmul (f,e,modulus);

	/* Now for the only double-precision call to mpilib: */
	set_precision (orig_precision * 2);
	mp_sub (d,f);

	/* d's precision should be <= orig_precision */
	rescale (d, orig_precision*2, orig_precision);
	set_precision (orig_precision);

	/* Should never have to do this final subtract more than twice: */
	while (mp_compare(d,modulus) > 0)
		mp_sub (d,modulus);

	mp_move(prod,d);
	return(0);  /* normal return */
}	/* upton_modmult */


/*	Upton's mp_modmult function leaves some internal arrays in memory,
	so we have to call modmult_burn() at the end of mp_modexp.
	This is so that no cryptographically sensitive data is left in memory
	after the program exits.
	upton_burn() is aliased to modmult_burn().
*/
static void upton_burn(void)
{
	unitfill0(modulus,MAX_UNIT_PRECISION);
	unitfill0(reciprocal,MAX_UNIT_PRECISION);
	unitfill0(dhi,MAX_UNIT_PRECISION);
	unitfill0(d_data,MAX_UNIT_PRECISION*2);
	unitfill0(e_data,MAX_UNIT_PRECISION*2);
	unitfill0(f_data,MAX_UNIT_PRECISION*2);
	nbits = nbitsDivUNITSIZE = nbitsModUNITSIZE = 0;
}	/* upton_burn */

/******* end of Upton's MODMULT stuff. *******/
/*=========================================================================*/
#endif  /* UPTON */

#ifdef SMITH	/* using Thad Smith's modmult algorithm */

/*	Thad Smith's implementation of multiprecision modmult algorithm in C.
	Performs a multiply combined with a modulo operation.
	The multiplication is done with mp_dmul, the same as for Upton's
	modmult.  The modulus reduction is done by long division, in
	which a trial quotient "digit" is determined, then the product of
	that digit and the divisor are subtracted from the dividend.

	In this case, the digit is MULTUNIT in size and the subtraction
	is done by adding the product to the one's complement of the
	dividend, which allows use of the existing mp_smul routine.

	The following support functions and data structures
	are used only by Smith's modmult algorithm...
*/

/*	These scratchpad arrays are used only by smith_modmult (mp_modmult).
	Some of them could be statically declared inside of mp_modmult, but we
	put them outside mp_modmult so that they can be wiped clean by
	modmult_burn(), which is called at the end of mp_modexp.  This is
	so that no sensitive data is left in memory after the program exits.
*/

static unit ALIGN ds_data[MAX_UNIT_PRECISION*2+2];

static unit mod_quotient  [4];
static unit mod_divisor   [4];	/* 2 most signif. MULTUNITs of modulus */

static MULTUNIT *modmpl;		/* ptr to modulus least significant
					** MULTUNIT			    */
static int  mshift;			/* number of bits for
					** recip scaling	  */
static MULTUNIT reciph; 		/* MSunit of scaled recip */
static MULTUNIT recipl; 		/* LSunit of scaled recip */

static short modlenMULTUNITS;		/* length of modulus in MULTUNITs */
static MULTUNIT mutemp; 		/* temporary */

/*	The routines mp_smul and mp_dmul are the same as for UPTON and
	should be coded in assembly.  Note, however, that the previous
	Upton's mp_smul version has been modified to compatible with
	Smith's modmult.  The new version also still works for Upton's
	modmult.
*/

#ifndef mp_set_recip
#define mp_set_recip(rh,rl,m)     /* null */
#else
/* setup routine for external mp_quo_digit */
void mp_set_recip(MULTUNIT rh, MULTUNIT rl, int m);
#endif
MULTUNIT mp_quo_digit (MULTUNIT *dividend);

#ifdef	MULTUNIT_SIZE_SAME
#define mp_musubb mp_subb	/* use existing routine */
#else  /* ! MULTUNIT_SIZE_SAME */

/*	This performs the same function as mp_subb, but with MULTUNITs.
	Note: Processors without alignment requirements may be able to use
	mp_subb, even though MULTUNITs are smaller than units.  In that case,
	use mp_subb, since it would be faster if coded in assembly.  Note that
	this implementation won't work for MULTUNITs which are long -- use
	mp_subb in that case.
*/
#ifndef mp_musubb
boolean mp_musubb
	(register MULTUNIT* r1,const MULTUNIT* r2,register boolean borrow)
	/* multiprecision subtract of MULTUNITs with borrow, r2 from r1,
	** result in r1 */
	/* borrow is incoming borrow flag-- value should be 0 or 1 */
{	register ulint x;	/* won't work if sizeof(MULTUNIT)==
						 sizeof(long)	    */
	short precision;	/* number of MULTUNITs to subtract */
	precision = global_precision * MULTUNITs_perunit;
	make_lsbptr(r1,precision);
	make_lsbptr(r2,precision);
	while (precision--)
	{	x = (ulint) *r1 - (ulint) *post_higherunit(r2) - (ulint) borrow;
		*post_higherunit(r1) = x;
		borrow = (((1L << MULTUNITSIZE) & x) != 0L);
	}
	return (borrow);
}	/* mp_musubb */
#endif	/* mp_musubb */
#endif	/* MULTUNIT_SIZE_SAME */

/*	The function mp_quo_digit is the heart of Smith's modulo reduction,
	which uses a form of long division.  It computes a trial quotient
	"digit" (MULTUNIT-sized digit) by multiplying the three most
	significant MULTUNITs of the dividend by the two most significant
	MULTUNITs of the reciprocal of the modulus.  Note that this function
	requires that MULTUNITSIZE * 2 <= sizeof(unsigned long).

	An important part of this technique is that the quotient never be
	too small, although it may occasionally be too large.  This was
	done to eliminate the need to check and correct for a remainder
	exceeding the divisor.	It is easier to check for a negative
	remainder.  The following technique rarely needs correction for
	MULTUNITs of at least 16 bits.

	The following routine has two implementations:

	ASM_PROTOTYPE defined: written to be an executable prototype for
	an efficient assembly language implementation.  Note that several
	of the following masks and shifts can be done by directly
	manipulating single precision registers on some architectures.

	ASM_PROTOTYPE undefined: a slightly more efficient implementation
	in C.  Although this version returns a result larger than the
	optimum (which is corrected in smith_modmult) more often than the
	prototype version, the faster execution in C more than makes up
	for the difference.
 
	Parameter: dividend - points to the most significant MULTUNIT
		of the dividend.  Note that dividend actually contains the 
		one's complement of the actual dividend value (see comments for 
		smith_modmult).
	
	 Return: the trial quotient digit resulting from dividing the first
		three MULTUNITs at dividend by the upper two MULTUNITs of the 
		modulus.
*/

/* #define ASM_PROTOTYPE */ /* undefined: use C-optimized version */

#ifndef mp_quo_digit
MULTUNIT mp_quo_digit (MULTUNIT *dividend) {
	unsigned long q, q0, q1, q2;
	unsigned short lsb_factor;

/*	Compute the least significant product group.
	The last terms of q1 and q2 perform upward rounding, which is
	needed to guarantee that the result not be too small.
*/
	q1 = (dividend[tohigher(-2)] ^ MULTUNIT_mask) * (unsigned long)reciph
	     + reciph;
	q2 = (dividend[tohigher(-1)] ^ MULTUNIT_mask) * (unsigned long)recipl
	     + (1L << MULTUNITSIZE) ;
#ifdef ASM_PROTOTYPE
	lsb_factor = 1 & (q1>>MULTUNITSIZE) & (q2>>MULTUNITSIZE);
	q = q1 + q2;

	/* The following statement is equivalent to shifting the sum right
	   one bit while shifting in the carry bit.
	*/
	q0 = (q1 > ~q2 ? DMULTUNIT_msb : 0) | (q >> 1);
#else 	/* optimized C version */
	q0 = (q1>>1) + (q2>>1) + 1;
#endif

/*	Compute the middle significant product group.	*/

	q1 = (dividend[tohigher(-1)] ^ MULTUNIT_mask) * (unsigned long)reciph;
	q2 = (dividend[ 0] ^ MULTUNIT_mask) * (unsigned long)recipl;
#ifdef ASM_PROTOTYPE
	q = q1 + q2;
	q = (q1 > ~q2 ? DMULTUNIT_msb : 0) | (q >> 1);

/*	Add in the most significant word of the first group.
	The last term takes care of the carry from adding the lsb's
	that were shifted out prior to addition.
*/
	q = (q0 >> MULTUNITSIZE)+ q + (lsb_factor & (q1 ^ q2));
#else 	/* optimized C version */
	q = (q0 >> MULTUNITSIZE)+ (q1>>1) + (q2>>1) + 1;
#endif

/*	Compute the most significant term and add in the others */

	q = (q >> (MULTUNITSIZE-2)) +
	    (((dividend[0] ^ MULTUNIT_mask) * (unsigned long)reciph) << 1);
	q >>= mshift;

/*	Prevent overflow and then wipe out the intermediate results. */

	mutemp = (MULTUNIT)min(q, (1L << MULTUNITSIZE) -1);
	q= q0 = q1 = q2 = 0;   lsb_factor = 0; (void)lsb_factor;
	return mutemp;
}
#endif	/* mp_quo_digit */

/*	stage_smith_modulus() - Prepare for a Smith modmult.

	Calculate the reciprocal of modulus with a precision of two MULTUNITs.
	Assumes that global_precision has already been adjusted to the
	size of the modulus, plus SLOP_BITS.

	Note: This routine was designed to work with large values and
	doesn't have the necessary testing or handling to work with a
	modulus having less than three significant units.  For such cases,
	the separate multiply and modulus routines can be used.

	stage_smith_modulus() is aliased to stage_modulus().
*/

int stage_smith_modulus(const unit* n_modulus)
{
	int   original_precision;
	int   sigmod;		/* significant units in modulus */
	unitptr  mp;		/* modulus most significant pointer */
	MULTUNIT *mpm;		/* reciprocal pointer */
	int   prec;		/* precision of reciprocal calc in units */

	mp_move(modulus, n_modulus);
	modmpl = (MULTUNIT*) modulus;
	modmpl = lsbptr (modmpl, global_precision * MULTUNITs_perunit);
	nbits = countbits(modulus);
	modlenMULTUNITS = (nbits+ MULTUNITSIZE-1) / MULTUNITSIZE;

	original_precision = global_precision;

	/* The following code copies the three most significant units of
	 * modulus to mod_divisor.
	*/
	mp = modulus;
	sigmod = significance (modulus);
	rescale (mp, original_precision, sigmod);
/* prec is the unit precision required for 3 MULTUNITs */
	prec = (3 +(MULTUNITs_perunit-1)) / MULTUNITs_perunit;
	set_precision (prec);
 
	/* set mp = ptr to most significant units of modulus, then move
	 * the most significant units to mp_divisor 
	*/
	mp = msbptr(mp,sigmod) -tohigher(prec-1);
	unmake_lsbptr (mp, prec);
	mp_move (mod_divisor, mp);
 
	/* Keep 2*MULTUNITSIZE bits in mod_divisor.
	 * This will (normally) result in a reciprocal of 2*MULTUNITSIZE+1 bits.
	*/
	mshift = countbits (mod_divisor) - 2*MULTUNITSIZE;
	mp_shift_right_bits (mod_divisor, mshift);
	mp_recip(mod_quotient,mod_divisor);
	mp_shift_right_bits (mod_quotient,1);

	/* Reduce to:   0 < mshift <= MULTUNITSIZE */
	mshift = ((mshift + (MULTUNITSIZE-1)) % MULTUNITSIZE) +1;
	/* round up, rescaling if necessary */
	mp_inc (mod_quotient);
	if (countbits (mod_quotient) > 2*MULTUNITSIZE) {
		mp_shift_right_bits (mod_quotient,1);
		mshift--;	/* now  0 <= mshift <= MULTUNITSIZE */
	}
	mpm = lsbptr ((MULTUNIT*)mod_quotient, prec*MULTUNITs_perunit);
	recipl = *post_higherunit (mpm);
	reciph = *mpm;
	mp_set_recip (reciph, recipl, mshift);
	set_precision (original_precision);
	return(0);	/* normal return */
}	/* stage_smith_modulus */

/*	Smith's algorithm performs a multiply combined with a modulo operation.
	Computes:  prod = (multiplicand*multiplier) mod modulus
	The modulus must first be set by stage_smith_modulus().
	smith_modmult() is aliased to mp_modmult().
*/

int
smith_modmult(unitptr prod, unitptr multiplicand, unitptr multiplier)
{
	unitptr    d;	/* ptr to product */ 
	MULTUNIT   *dmph, *dmpl, *dmp;	/* ptrs to dividend (high, low, first)
					 * aligned for subtraction	     */
/*	Note that dmph points one MULTUNIT higher than indicated by
	global precision.  This allows us to zero out a word one higher than
	the normal precision.
*/
	short	    orig_precision;
	short	    nqd;	/* number of quotient digits remaining to
				 * be generated 			     */
	short	    dmi;	/* number of significant MULTUNITs in product */

	d = ds_data + 1;	/* room for leading MSB if HIGHFIRST */
	orig_precision = global_precision;
	mp_dmul(d, multiplicand, multiplier);

	rescale(d, orig_precision * 2, orig_precision * 2 + 1);
	set_precision(orig_precision * 2 + 1);
	*msbptr(d, global_precision) = 0;	/* leading 0 unit */

/*	We now start working with MULTUNITs.
	Determine the most significant MULTUNIT of the product so we don't
	have to process leading zeros in our divide loop.
*/
	dmi = significance(d) * MULTUNITs_perunit;
	if (dmi >= modlenMULTUNITS)
	{	/* Make dividend negative.  This allows the use of mp_smul to
		 * "subtract" the product of the modulus and the trial divisor
		 * by actually adding to a negative dividend.
		 * The one's complement of the dividend is used, since it causes
		 * a zero value to be represented as all ones.  This facilitates
		 * testing the result for possible overflow, since a sign bit
		 * indicates that no adjustment is necessary, and we should not
		 * attempt to adjust if the result of the addition is zero.
		 */
		mp_inc(d);
		mp_neg(d);
		set_precision(orig_precision);
		munit_prec = global_precision * UNITSIZE / MULTUNITSIZE;

		/* Set msb, lsb, and normal ptrs of dividend */
		dmph = lsbptr((MULTUNIT *) d, (orig_precision * 2 + 1) *
			    MULTUNITs_perunit) + tohigher(dmi);
		nqd = dmi + 1 - modlenMULTUNITS;
		dmpl = dmph - tohigher(modlenMULTUNITS);

/*	Divide loop.
	Each iteration computes the next quotient MULTUNIT digit, then
	multiplies the divisor (modulus) by the quotient digit and adds
	it to the one's complement of the dividend (equivalent to
	subtracting).  If the product was greater than the remaining dividend,
	we get a non-negative result, in which case we subtract off the
	modulus to get the proper negative remainder.
*/
		for (; nqd; nqd--)
		{	MULTUNIT    q;	    /* quotient trial digit */

			q = mp_quo_digit(dmph);
			if (q > 0)
			{	mp_smul(dmpl, modmpl, q);

				/* Perform correction if q too large.
				*  This rarely occurs.
				*/
				if (!(*dmph & MULTUNIT_msb))
				{	dmp = dmpl;
					unmake_lsbptr(dmp, orig_precision * 
						   MULTUNITs_perunit);
					if (mp_musubb(dmp,
						   (const MULTUNIT *) modulus, 0))
						(*dmph)  --;
				}
			}
			pre_lowerunit(dmph);
			pre_lowerunit(dmpl);
		}
		/* d contains the one's complement of the remainder. */
		rescale(d, orig_precision * 2 + 1, orig_precision);
		set_precision(orig_precision);
		mp_neg(d);
		mp_dec(d);
	} else
	{	/* Product was less than modulus.  Return it. */
		rescale(d, orig_precision * 2 + 1, orig_precision);
		set_precision(orig_precision);
	}
	mp_move(prod, d);
	return (0);		/* normal return */
}				/* smith_modmult */


/*	Smith's mp_modmult function leaves some internal arrays in memory,
	so we have to call modmult_burn() at the end of mp_modexp.
	This is so that no cryptographically sensitive data is left in memory
	after the program exits.
	smith_burn() is aliased to modmult_burn().
*/
void smith_burn(void)
{
	empty_array (modulus);
	empty_array (ds_data);
	empty_array (mod_quotient);
	empty_array (mod_divisor);
	modmpl = 0;
	mshift =nbits = 0;
	reciph = recipl = 0;
	modlenMULTUNITS = mutemp = 0;
	mp_set_recip (0,0,0);
}	/* smith_burn */

/*	End of Thad Smith's implementation of modmult. */

#endif	/* SMITH */


unsigned int countbits(const unit* r)
	/* Returns number of significant bits in r */
{	unsigned int bits;
	short prec;
	register unit bitmask;
	init_bitsniffer(r,bitmask,prec,bits);
	return(bits);
} /* countbits */


const char *copyright_notice(void);
const char *copyright_notice(void)
/* force linker to include copyright notice in the executable object image. */
{ return ("(c)1986 Philip Zimmermann"); } /* copyright_notice */


int mp_modexp(register unitptr expout,register unitptr expin,
	register unitptr exponent,const unit* pmodulus)
{	/*	Russian peasant combined exponentiation/modulo algorithm.
		Calls modmult instead of mult.
		Computes:  expout = (expin**exponent) mod modulus
		WARNING: All the arguments must be less than the modulus!
	*/
	int bits;
	short oldprecision;
	register unit bitmask;
	unit product[MAX_UNIT_PRECISION];
	short eprec;

#ifdef COUNTMULTS
	tally_modmults = 0;	/* clear "number of modmults" counter */
	tally_modsquares = 0;	/* clear "number of modsquares" counter */
#endif	/* COUNTMULTS */
	mp_init(expout,1);
	if (testeq(exponent,0))
	{	if (testeq(expin,0))
			return(-1); /* 0 to the 0th power means return error */
		return(0);	/* otherwise, zero exponent means expout is 1 */
	}
	if (testeq(pmodulus,0))
		return(-2);		/* zero modulus means error */
#if SLOP_BITS > 0	/* if there's room for sign bits */
	if (mp_tstminus(pmodulus))
		return(-2);		/* negative modulus means error */
#endif	/* SLOP_BITS > 0 */
	if (mp_compare(expin,pmodulus) >= 0)
		return(-3); /* if expin >= modulus, return error */
	if (mp_compare(exponent,pmodulus) >= 0)
		return(-4); /* if exponent >= modulus, return error */

	oldprecision = global_precision;	/* save global_precision */
	/* set smallest optimum precision for this modulus */
	set_precision(bits2units(countbits(pmodulus)+SLOP_BITS));
	/* rescale all these registers to global_precision we just defined */
	rescale(pmodulus,oldprecision,global_precision);
	rescale(expin,oldprecision,global_precision);
	rescale(exponent,oldprecision,global_precision);
	rescale(expout,oldprecision,global_precision);

	if (stage_modulus(pmodulus))
	{	set_precision(oldprecision);	/* restore original precision */
		return(-5);		/* unstageable modulus (STEWART algorithm) */
	}

	/* normalize and compute number of bits in exponent first */
	init_bitsniffer(exponent,bitmask,eprec,bits);

	/* We can "optimize out" the first modsquare and modmult: */
	bits--;		/* We know for sure at this point that bits>0 */
	mp_move(expout,expin);		/*  expout = (1*1)*expin; */
	bump_bitsniffer(exponent,bitmask);

	while (bits--)
	{
		poll_for_break(); /* polls keyboard, allows ctrl-C to abort program */
#ifdef COUNTMULTS
		tally_modsquares++;	/* bump "number of modsquares" counter */
#endif	/* COUNTMULTS */
		mp_modsquare(product,expout);
		if (sniff_bit(exponent,bitmask))
		{	mp_modmult(expout,product,expin);
#ifdef COUNTMULTS
			tally_modmults++;	/* bump "number of modmults" counter */
#endif	/* COUNTMULTS */
		} else
		{
			mp_move(expout,product);
		}
		bump_bitsniffer(exponent,bitmask);
	}	/* while bits-- */
	mp_burn(product);	/* burn the evidence on the stack */
	modmult_burn(); /* ask mp_modmult to also burn its own evidence */

#ifdef COUNTMULTS	/* diagnostic analysis */
	{	long atomic_mults;
		unsigned int unitcount,totalmults;
		unitcount = bits2units(countbits(pmodulus));
		/* calculation assumes modsquare takes as long as a modmult: */
		atomic_mults = (long) tally_modmults * (unitcount * unitcount);
		atomic_mults += (long) tally_modsquares * (unitcount * unitcount);
		printf("%ld atomic mults for ",atomic_mults);
		printf("%d+%d = %d modsqr+modmlt, at %d bits, %d words.\n",
			tally_modsquares,tally_modmults,
			tally_modsquares+tally_modmults,
			countbits(pmodulus), unitcount);
	}
#endif	/* COUNTMULTS */

	set_precision(oldprecision);	/* restore original precision */

	/* Do an explicit reference to the copyright notice so that the linker
	   will be forced to include it in the executable object image... */
	copyright_notice();	/* has no real effect at run time */

	return(0);		/* normal return */
}	/* mp_modexp */

int mp_modexp_crt(unitptr expout, unitptr expin,
	unitptr p, unitptr q, unitptr ep, unitptr eq, unitptr u)
	/*	This is a faster modexp for moduli with a known
		factorisation into two relatively prime factors p and q,
		and an input relatively prime to the modulus,
		the Chinese Remainder Theorem to do the computation
		mod p and mod q, and then combine the results.  This
		relies on a number of precomputed values, but does not
		actually require the modulus n or the exponent e.

		expout = expin ^ e mod (p*q).
		We form this by evaluating
		p2 = (expin ^ e) mod p and
		q2 = (expin ^ e) mod q
		and then combining the two by the CRT.

		Two optimisations of this are possible.  First, we can
		reduce expin modulo p and q before starting.

		Second, since we know the factorisation of p and q
		(trivially derived from the factorisation of n = p*q),
		and expin is relatively prime to both p and q,
		we can use Euler's theorem, expin^phi(m) = 1 (mod m),
		to throw away multiples of phi(p) or phi(q) in e.
		Letting ep = e mod phi(p) and
		        eq = e mod phi(q)
		then combining these two speedups, we only need to evaluate
		p2 = ((expin mod p) ^ ep) mod p and
		q2 = ((expin mod q) ^ eq) mod q.

		Now we need to apply the CRT.  Starting with
		expout = p2 (mod p) and
		expout = q2 (mod q)
		we can say that expout = p2 + p * k, and if we assume that
		0 <= p2 < p, then 0 <= expout < p*q for some 0 <= k < q.
		Since we want expout = q2 (mod q), then
		p*k = q2-p2 (mod q).  Since p and q are relatively
		prime, p has a multiplicative inverse u mod q.  In other
		words,
		       u = 1/p (mod q).
		Multiplying by u on both sides gives k = u*(q2-p2) (mod q).
		Since we want 0 <= k < q, we can thus find k as
		k = (u * (q2-p2)) mod q.

		Once we have k, evaluating p2 + p * k is easy, and
		that gives us the result.

		In the detailed implementation, there is a temporary, temp,
		used to hold intermediate results, p2 is held in expout,
		and q2 is used as a temporary in the final step when it is
		no longer needed.  With that, you should be able to
		understand the code below.
	*/
{
	unit q2[MAX_UNIT_PRECISION];
	unit temp[MAX_UNIT_PRECISION];
	int status;

/*	First, compiute p2 (physically held in M) */

/*	p2 = [ (expin mod p) ^ ep ] mod p		*/
	mp_mod(temp,expin,p);		/* temp = expin mod p  */
	status = mp_modexp(expout,temp,ep,p);
	if (status < 0)	/* mp_modexp returned an error. */
	{	mp_init(expout,1);
		return(status);	/* error return */
	}

/*	And the same thing for q2 */

/*	q2 = [ (expin mod q) ^ eq ] mod q		*/
	mp_mod(temp,expin,q);		/* temp = expin mod q  */
	status = mp_modexp(q2,temp,eq,q);
	if (status < 0)	/* mp_modexp returned an error. */
	{	mp_init(expout,1);
		return(status);	/* error return */
	}

/*	Now use the multiplicative inverse u to glue together the
	two halves.
*/
#if 0
/* This optimisation is useful if you commonly get small results,
   but for random results and large q, the odds of (1/q) of it
   being useful do not warrant the test.
*/
	if (mp_compare(expout,q2) != 0)
	{
#endif
	/*	Find q2-p2 mod q */
	if (mp_sub(q2,expout))	/* if the result went negative */
		mp_add(q2,q);	/* add q to q2 */

	/*	expout = p2 + ( p * [(q2*u) mod q] )	*/
	mp_mult(temp,q2,u);		/* q2*u  */
	mp_mod(q2,temp,q);		/* (q2*u) mod q */
	mp_mult(temp,p,q2);		/* p * [(q2*u) mod q] */
	mp_add(expout,temp);		/* expout = p2 + p * [...] */
#if 0
	}
#endif

	mp_burn(q2);	/* burn the evidence on the stack...*/
	mp_burn(temp);
	return(0);	/* normal return */
}	/* mp_modexp_crt */


/****************** end of MPI library ****************************/
