#ifndef __PRIVATE_LIBNCP_GENERIC_ATOMIC_H__
#define __PRIVATE_LIBNCP_GENERIC_ATOMIC_H__

#include "config.h"

#include "private/libncp-lock.h"

typedef struct {
	int counter;
	ncpt_mutex_t mutex;
	} ncpt_atomic_t;
	
#define NCPT_ATOMIC_INIT(i) { (i), NCPT_MUTEX_INITIALIZER }

static inline int ncpt_atomic_read(ncpt_atomic_t* v) {
	int tmp;
	
	ncpt_mutex_lock(&v->mutex);
	tmp = v->counter;
	ncpt_mutex_unlock(&v->mutex);
	return tmp;
}

static inline int ncpt_atomic_set(ncpt_atomic_t* v, int i) {
	v->counter = i;
	ncpt_mutex_init(&v->mutex);
	return i;
}

static inline void ncpt_atomic_add(int i, ncpt_atomic_t* v) {
	ncpt_mutex_lock(&v->mutex);
	v->counter += i;
	ncpt_mutex_unlock(&v->mutex);
}

static inline void ncpt_atomic_sub(int i, ncpt_atomic_t* v) {
	ncpt_mutex_lock(&v->mutex);
	v->counter -= i;
	ncpt_mutex_unlock(&v->mutex);
}

#define ncpt_atomic_inc(v) ncpt_atomic_add(1,v)
#define ncpt_atomic_dec(v) ncpt_atomic_sub(1,v)

static inline int ncpt_atomic_dec_and_test(ncpt_atomic_t* v) {
	int tmp;
	
	ncpt_mutex_lock(&v->mutex);
	tmp = !(--v->counter);
	ncpt_mutex_unlock(&v->mutex);
	return tmp;
}

#endif	/* __PRIVATE_LIBNCP_GENERIC_ATOMIC_H__ */
