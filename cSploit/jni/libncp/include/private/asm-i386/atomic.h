#ifndef __PRIVATE_ASM_I386_ATOMIC__
#define __PRIVATE_ASM_I386_ATOMIC__

/* Optimization barrier */
/* The "volatile" is due to gcc bugs */
#define barrier() __asm__ __volatile__("": : :"memory")

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

#define LOCK "lock ; "

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct { volatile int counter; } ncpt_atomic_t;

#define NCPT_ATOMIC_INIT(i)	{ (i) }

/**
 * ncpt_atomic_read - read atomic variable
 * @v: pointer of type ncpt_atomic_t
 * 
 * Atomically reads the value of @v.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */ 
#define ncpt_atomic_read(v)		((v)->counter)

/**
 * ncpt_atomic_set - set atomic variable
 * @v: pointer of type ncpt_atomic_t
 * @i: required value
 * 
 * Atomically sets the value of @v to @i.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */ 
#define ncpt_atomic_set(v,i)		(((v)->counter) = (i))

/**
 * ncpt_atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type ncpt_atomic_t
 * 
 * Atomically adds @i to @v.  Note that the guaranteed useful range
 * of an ncpt_atomic_t is only 24 bits.
 */
static __inline__ void ncpt_atomic_add(int i, ncpt_atomic_t *v)
{
	__asm__ __volatile__(
		LOCK "addl %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
}

/**
 * ncpt_atomic_sub - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type ncpt_atomic_t
 * 
 * Atomically subtracts @i from @v.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
static __inline__ void ncpt_atomic_sub(int i, ncpt_atomic_t *v)
{
	__asm__ __volatile__(
		LOCK "subl %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
}

/**
 * ncpt_atomic_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type ncpt_atomic_t
 * 
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */
static __inline__ int ncpt_atomic_sub_and_test(int i, ncpt_atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		LOCK "subl %2,%0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
}

/**
 * ncpt_atomic_inc - increment atomic variable
 * @v: pointer of type ncpt_atomic_t
 * 
 * Atomically increments @v by 1.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */ 
static __inline__ void ncpt_atomic_inc(ncpt_atomic_t *v)
{
	__asm__ __volatile__(
		LOCK "incl %0"
		:"=m" (v->counter)
		:"m" (v->counter));
}

/**
 * ncpt_atomic_dec - decrement atomic variable
 * @v: pointer of type ncpt_atomic_t
 * 
 * Atomically decrements @v by 1.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */ 
static __inline__ void ncpt_atomic_dec(ncpt_atomic_t *v)
{
	__asm__ __volatile__(
		LOCK "decl %0"
		:"=m" (v->counter)
		:"m" (v->counter));
}

/**
 * ncpt_atomic_dec_and_test - decrement and test
 * @v: pointer of type ncpt_atomic_t
 * 
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */ 
static __inline__ int ncpt_atomic_dec_and_test(ncpt_atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		LOCK "decl %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
}

/**
 * ncpt_atomic_inc_and_test - increment and test 
 * @v: pointer of type ncpt_atomic_t
 * 
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */ 
static __inline__ int ncpt_atomic_inc_and_test(ncpt_atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		LOCK "incl %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
}

/**
 * ncpt_atomic_add_negative - add and test if negative
 * @v: pointer of type ncpt_atomic_t
 * @i: integer value to add
 * 
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.  Note that the guaranteed
 * useful range of an ncpt_atomic_t is only 24 bits.
 */ 
static __inline__ int ncpt_atomic_add_negative(int i, ncpt_atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		LOCK "addl %2,%0; sets %1"
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
}

/* These are x86-specific, used by some header files */
#define ncpt_atomic_clear_mask(mask, addr) \
__asm__ __volatile__(LOCK "andl %0,%1" \
: : "r" (~(mask)),"m" (*addr) : "memory")

#define ncpt_atomic_set_mask(mask, addr) \
__asm__ __volatile__(LOCK "orl %0,%1" \
: : "r" (mask),"m" (*addr) : "memory")

/* Atomic operations are already serializing on x86 */
#define smp_mb__before_ncpt_atomic_dec()	barrier()
#define smp_mb__after_ncpt_atomic_dec()	barrier()
#define smp_mb__before_ncpt_atomic_inc()	barrier()
#define smp_mb__after_ncpt_atomic_inc()	barrier()

#endif
