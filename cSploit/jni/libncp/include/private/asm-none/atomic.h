#ifndef __PRIVATE_LIBNCP_NONE_ATOMIC_H__
#define __PRIVATE_LIBNCP_NONE_ATOMIC_H__

typedef struct { int counter; } ncpt_atomic_t;

#define NCPT_ATOMIC_INIT(i)	{ (i) }

#define ncpt_atomic_read(v)		((v)->counter)
#define ncpt_atomic_set(v,i)		(((v)->counter) = (i))
#define ncpt_atomic_add(i,v)		(((v)->counter) += (i))
#define ncpt_atomic_sub(i,v)		(((v)->counter) -= (i))
#define ncpt_atomic_inc(v)		((v)->counter++)
#define ncpt_atomic_dec(v)		((v)->counter--)
#define ncpt_atomic_dec_and_test(v)	(!(--(v)->counter))

#endif	/* __PRIVATE_LIBNCP_NONE_ATOMIC_H__ */
