/*
 * BK Id: SCCS/s.atomic.h 1.8 05/17/01 18:14:24 cort
 */
/*
 * PowerPC atomic operations
 */

#ifndef _ASM_PPC_ATOMIC_H_ 
#define _ASM_PPC_ATOMIC_H_

/*
 * Memory barrier.
 * The sync instruction guarantees that all memory accesses initiated
 * by this processor have been performed (with respect to all other
 * mechanisms that access memory).  The eieio instruction is a barrier
 * providing an ordering (separately) for (a) cacheable stores and (b)
 * loads and stores to non-cacheable memory (e.g. I/O devices).
 *
 * mb() prevents loads and stores being reordered across this point.
 * rmb() prevents loads being reordered across this point.
 * wmb() prevents stores being reordered across this point.
 *
 * We can use the eieio instruction for wmb, but since it doesn't
 * give any ordering guarantees about loads, we have to use the
 * stronger but slower sync instruction for mb and rmb.
 */
#define mb()  __asm__ __volatile__ ("sync" : : : "memory")
#define rmb()  __asm__ __volatile__ ("sync" : : : "memory")
#define wmb()  __asm__ __volatile__ ("eieio" : : : "memory")

#define smp_mb()	mb()
#define smp_rmb()	rmb()
#define smp_wmb()	wmb()

typedef struct { volatile int counter; } ncpt_atomic_t;

#define NCPT_ATOMIC_INIT(i)	{ (i) }

#define ncpt_atomic_read(v)		((v)->counter)
#define ncpt_atomic_set(v,i)		(((v)->counter) = (i))

extern void ncpt_atomic_clear_mask(unsigned long mask, unsigned long *addr);
extern void ncpt_atomic_set_mask(unsigned long mask, unsigned long *addr);

static __inline__ int ncpt_atomic_add_return(int a, ncpt_atomic_t *v)
{
	int t;

	__asm__ __volatile__("\n\
1:	lwarx	%0,0,%3\n\
	add	%0,%2,%0\n\
	stwcx.	%0,0,%3\n\
	bne-	1b"
	: "=&r" (t), "=m" (v->counter)
	: "r" (a), "r" (v), "m" (v->counter)
	: "cc");

	return t;
}

static __inline__ int ncpt_atomic_sub_return(int a, ncpt_atomic_t *v)
{
	int t;

	__asm__ __volatile__("\n\
1:	lwarx	%0,0,%3\n\
	subf	%0,%2,%0\n\
	stwcx.	%0,0,%3\n\
	bne-	1b"
	: "=&r" (t), "=m" (v->counter)
	: "r" (a), "r" (v), "m" (v->counter)
	: "cc");

	return t;
}

static __inline__ int ncpt_atomic_inc_return(ncpt_atomic_t *v)
{
	int t;

	__asm__ __volatile__("\n\
1:	lwarx	%0,0,%2\n\
	addic	%0,%0,1\n\
	stwcx.	%0,0,%2\n\
	bne-	1b"
	: "=&r" (t), "=m" (v->counter)
	: "r" (v), "m" (v->counter)
	: "cc");

	return t;
}

static __inline__ int ncpt_atomic_dec_return(ncpt_atomic_t *v)
{
	int t;

	__asm__ __volatile__("\n\
1:	lwarx	%0,0,%2\n\
	addic	%0,%0,-1\n\
	stwcx.	%0,0,%2\n\
	bne	1b"
	: "=&r" (t), "=m" (v->counter)
	: "r" (v), "m" (v->counter)
	: "cc");

	return t;
}

#define ncpt_atomic_add(a, v)		((void) ncpt_atomic_add_return((a), (v)))
#define ncpt_atomic_sub(a, v)		((void) ncpt_atomic_sub_return((a), (v)))
#define ncpt_atomic_sub_and_test(a, v)	(ncpt_atomic_sub_return((a), (v)) == 0)
#define ncpt_atomic_inc(v)			((void) ncpt_atomic_inc_return((v)))
#define ncpt_atomic_dec(v)			((void) ncpt_atomic_dec_return((v)))
#define ncpt_atomic_dec_and_test(v)		(ncpt_atomic_dec_return((v)) == 0)

/*
 * Atomically test *v and decrement if it is greater than 0.
 * The function returns the old value of *v minus 1.
 */
static __inline__ int ncpt_atomic_dec_if_positive(ncpt_atomic_t *v)
{
	int t;

	__asm__ __volatile__("\n"
"1:	lwarx	%0,0,%2\n"
"	addic.	%0,%0,-1\n"
"	blt	2f\n"
"	stwcx.	%0,0,%2\n"
"	bne	1b\n"
"2:"
	: "=&r" (t), "=m" (v->counter)
	: "r" (&v->counter), "m" (v->counter)
	: "cc");

	return t;
}

#define smp_mb__before_ncpt_atomic_dec()	smp_mb()
#define smp_mb__after_ncpt_atomic_dec()	smp_mb()
#define smp_mb__before_ncpt_atomic_inc()	smp_mb()
#define smp_mb__after_ncpt_atomic_inc()	smp_mb()

#endif /* _ASM_PPC_ATOMIC_H_ */
