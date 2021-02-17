#ifndef __PRIVATE_ARCH_M68K_ATOMIC__
#define __PRIVATE_ARCH_M68K_ATOMIC__

/* Optimization barrier */
/* The "volatile" is due to gcc bugs */
#define barrier() __asm__ __volatile__("": : :"memory")

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

/*
 * We do not have SMP m68k systems, so we don't have to deal with that.
 */

typedef struct { int counter; } ncpt_atomic_t;
#define NCPT_ATOMIC_INIT(i)	{ (i) }

#define ncpt_atomic_read(v)		((v)->counter)
#define ncpt_atomic_set(v, i)	(((v)->counter) = i)

static __inline__ void ncpt_atomic_add(int i, ncpt_atomic_t *v)
{
	__asm__ __volatile__("addl %1,%0" : "=m" (*v) : "id" (i), "0" (*v));
}

static __inline__ void ncpt_atomic_sub(int i, ncpt_atomic_t *v)
{
	__asm__ __volatile__("subl %1,%0" : "=m" (*v) : "id" (i), "0" (*v));
}

static __inline__ void ncpt_atomic_inc(volatile ncpt_atomic_t *v)
{
	__asm__ __volatile__("addql #1,%0" : "=m" (*v): "0" (*v));
}

static __inline__ void ncpt_atomic_dec(volatile ncpt_atomic_t *v)
{
	__asm__ __volatile__("subql #1,%0" : "=m" (*v): "0" (*v));
}

static __inline__ int ncpt_atomic_dec_and_test(volatile ncpt_atomic_t *v)
{
	char c;
	__asm__ __volatile__("subql #1,%1; seq %0" : "=d" (c), "=m" (*v): "1" (*v));
	return c != 0;
}

#define ncpt_atomic_clear_mask(mask, v) \
	__asm__ __volatile__("andl %1,%0" : "=m" (*v) : "id" (~(mask)),"0"(*v))

#define ncpt_atomic_set_mask(mask, v) \
	__asm__ __volatile__("orl %1,%0" : "=m" (*v) : "id" (mask),"0"(*v))

/* Atomic operations are already serializing */
#define smp_mb__before_ncpt_atomic_dec()	barrier()
#define smp_mb__after_ncpt_atomic_dec()	barrier()
#define smp_mb__before_ncpt_atomic_inc()	barrier()
#define smp_mb__after_ncpt_atomic_inc()	barrier()

#endif /* __ARCH_M68K_ATOMIC __ */
