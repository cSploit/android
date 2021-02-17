/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 *
 * But use these as seldom as possible since they are much more slower
 * than regular operations.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 2000 by Ralf Baechle
 */
#ifndef __PRIVATE_ASM_ATOMIC_H
#define __PRIVATE_ASM_ATOMIC_H

/* Optimization barrier */
/* The "volatile" is due to gcc bugs */
#define barrier() __asm__ __volatile__("": : :"memory")

typedef struct { volatile int counter; } ncpt_atomic_t;

#define NCPT_ATOMIC_INIT(i)    { (i) }

/*
 * ncpt_atomic_read - read atomic variable
 * @v: pointer of type ncpt_atomic_t
 *
 * Atomically reads the value of @v.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
#define ncpt_atomic_read(v)	((v)->counter)

/*
 * ncpt_atomic_set - set atomic variable
 * @v: pointer of type ncpt_atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
#define ncpt_atomic_set(v,i)	((v)->counter = (i))

/*
 * ... while for MIPS II and better we can use ll/sc instruction.  This
 * implementation is SMP safe ...
 */

/*
 * ncpt_atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type ncpt_atomic_t
 *
 * Atomically adds @i to @v.  Note that the guaranteed useful range
 * of an ncpt_atomic_t is only 24 bits.
 */
/* Hope that compiler will complain at ll/sc when CPU we are compiling
 * (MIPS I) for does not have them...
 */
extern __inline__ void ncpt_atomic_add(int i, ncpt_atomic_t * v)
{
	unsigned long temp;

	__asm__ __volatile__(
		"1:   ll      %0, %1      # ncpt_atomic_add\n"
		"     addu    %0, %2                  \n"
		"     sc      %0, %1                  \n"
		"     beqz    %0, 1b                  \n"
		: "=&r" (temp), "=m" (v->counter)
		: "Ir" (i), "m" (v->counter));
}

/*
 * ncpt_atomic_sub - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type ncpt_atomic_t
 *
 * Atomically subtracts @i from @v.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
extern __inline__ void ncpt_atomic_sub(int i, ncpt_atomic_t * v)
{
	unsigned long temp;

	__asm__ __volatile__(
		"1:   ll      %0, %1      # ncpt_atomic_sub\n"
		"     subu    %0, %2                  \n"
		"     sc      %0, %1                  \n"
		"     beqz    %0, 1b                  \n"
		: "=&r" (temp), "=m" (v->counter)
		: "Ir" (i), "m" (v->counter));
}

/*
 * Same as above, but return the result value
 */
extern __inline__ int ncpt_atomic_add_return(int i, ncpt_atomic_t * v)
{
	unsigned long temp, result;

	__asm__ __volatile__(
		".set push               # ncpt_atomic_add_return\n"
		".set noreorder                             \n"
		"1:   ll      %1, %2                        \n"
		"     addu    %0, %1, %3                    \n"
		"     sc      %0, %2                        \n"
		"     beqz    %0, 1b                        \n"
		"     addu    %0, %1, %3                    \n"
		".set pop                                   \n"
		: "=&r" (result), "=&r" (temp), "=m" (v->counter)
		: "Ir" (i), "m" (v->counter)
		: "memory");

	return result;
}

extern __inline__ int ncpt_atomic_sub_return(int i, ncpt_atomic_t * v)
{
	unsigned long temp, result;

	__asm__ __volatile__(
		".set push                                   \n"
		".set noreorder           # ncpt_atomic_sub_return\n"
		"1:   ll    %1, %2                           \n"
		"     subu  %0, %1, %3                       \n"
		"     sc    %0, %2                           \n"
		"     beqz  %0, 1b                           \n"
		"     subu  %0, %1, %3                       \n"
		".set pop                                    \n"
		: "=&r" (result), "=&r" (temp), "=m" (v->counter)
		: "Ir" (i), "m" (v->counter)
		: "memory");

	return result;
}
#endif

#define ncpt_atomic_dec_return(v) ncpt_atomic_sub_return(1,(v))
#define ncpt_atomic_inc_return(v) ncpt_atomic_add_return(1,(v))

/*
 * ncpt_atomic_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type ncpt_atomic_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
#define ncpt_atomic_sub_and_test(i,v) (ncpt_atomic_sub_return((i), (v)) == 0)

/*
 * ncpt_atomic_inc_and_test - increment and test
 * @v: pointer of type ncpt_atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
#define ncpt_atomic_inc_and_test(v) (ncpt_atomic_inc_return(1, (v)) == 0)

/*
 * ncpt_atomic_dec_and_test - decrement by 1 and test
 * @v: pointer of type ncpt_atomic_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
#define ncpt_atomic_dec_and_test(v) (ncpt_atomic_sub_return(1, (v)) == 0)

/*
 * ncpt_atomic_inc - increment atomic variable
 * @v: pointer of type ncpt_atomic_t
 *
 * Atomically increments @v by 1.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
#define ncpt_atomic_inc(v) ncpt_atomic_add(1,(v))

/*
 * ncpt_atomic_dec - decrement and test
 * @v: pointer of type ncpt_atomic_t
 *
 * Atomically decrements @v by 1.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
#define ncpt_atomic_dec(v) ncpt_atomic_sub(1,(v))

/*
 * ncpt_atomic_add_negative - add and test if negative
 * @v: pointer of type ncpt_atomic_t
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 *
 * Currently not implemented for MIPS.
 */

/* Atomic operations are already serializing */
#define smp_mb__before_ncpt_atomic_dec()	barrier()
#define smp_mb__after_ncpt_atomic_dec()	barrier()
#define smp_mb__before_ncpt_atomic_inc()	barrier()
#define smp_mb__after_ncpt_atomic_inc()	barrier()

#endif /* __ASM_ATOMIC_H */
