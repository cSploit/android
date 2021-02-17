#ifndef _PRIVATE_ALPHA_ATOMIC_H
#define _PRIVATE_ALPHA_ATOMIC_H

#define mb() \
__asm__ __volatile__("mb": : :"memory")

#define rmb() \
__asm__ __volatile__("mb": : :"memory")

#define wmb() \
__asm__ __volatile__("wmb": : :"memory")

#define smp_mb()	mb()
#define smp_rmb()	rmb()
#define smp_wmb()	wmb()

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc...
 *
 * But use these as seldom as possible since they are much slower
 * than regular operations.
 */


/*
 * Counter is volatile to make sure gcc doesn't try to be clever
 * and move things around on us. We need to use _exactly_ the address
 * the user gave us, not some alias that contains the same information.
 */
typedef struct { volatile int counter; } ncpt_atomic_t;

#define NCPT_ATOMIC_INIT(i)	( (ncpt_atomic_t) { (i) } )

#define ncpt_atomic_read(v)		((v)->counter)
#define ncpt_atomic_set(v,i)		((v)->counter = (i))

/*
 * To get proper branch prediction for the main line, we must branch
 * forward to code at the end of this object's .text section, then
 * branch back to restart the operation.
 */

static __inline__ void ncpt_atomic_add(int i, ncpt_atomic_t * v)
{
	unsigned long temp;
	__asm__ __volatile__(
	"1:	ldl_l %0,%1\n"
	"	addl %0,%2,%0\n"
	"	stl_c %0,%1\n"
	"	beq %0,2f\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter)
	:"Ir" (i), "m" (v->counter));
}

static __inline__ void ncpt_atomic_sub(int i, ncpt_atomic_t * v)
{
	unsigned long temp;
	__asm__ __volatile__(
	"1:	ldl_l %0,%1\n"
	"	subl %0,%2,%0\n"
	"	stl_c %0,%1\n"
	"	beq %0,2f\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter)
	:"Ir" (i), "m" (v->counter));
}

/*
 * Same as above, but return the result value
 */
static __inline__ long ncpt_atomic_add_return(int i, ncpt_atomic_t * v)
{
	long temp, result;
	__asm__ __volatile__(
	"1:	ldl_l %0,%1\n"
	"	addl %0,%3,%2\n"
	"	addl %0,%3,%0\n"
	"	stl_c %0,%1\n"
	"	beq %0,2f\n"
	"	mb\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter), "=&r" (result)
	:"Ir" (i), "m" (v->counter) : "memory");
	return result;
}

static __inline__ long ncpt_atomic_sub_return(int i, ncpt_atomic_t * v)
{
	long temp, result;
	__asm__ __volatile__(
	"1:	ldl_l %0,%1\n"
	"	subl %0,%3,%2\n"
	"	subl %0,%3,%0\n"
	"	stl_c %0,%1\n"
	"	beq %0,2f\n"
	"	mb\n"
	".subsection 2\n"
	"2:	br 1b\n"
	".previous"
	:"=&r" (temp), "=m" (v->counter), "=&r" (result)
	:"Ir" (i), "m" (v->counter) : "memory");
	return result;
}

#define ncpt_atomic_dec_return(v) ncpt_atomic_sub_return(1,(v))
#define ncpt_atomic_inc_return(v) ncpt_atomic_add_return(1,(v))

#define ncpt_atomic_sub_and_test(i,v) (ncpt_atomic_sub_return((i), (v)) == 0)
#define ncpt_atomic_dec_and_test(v) (ncpt_atomic_sub_return(1, (v)) == 0)

#define ncpt_atomic_inc(v) ncpt_atomic_add(1,(v))
#define ncpt_atomic_dec(v) ncpt_atomic_sub(1,(v))

#define smp_mb__before_ncpt_atomic_dec()	smp_mb()
#define smp_mb__after_ncpt_atomic_dec()	smp_mb()
#define smp_mb__before_ncpt_atomic_inc()	smp_mb()
#define smp_mb__after_ncpt_atomic_inc()	smp_mb()

#endif /* _ALPHA_ATOMIC_H */
