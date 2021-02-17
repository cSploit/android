/* zn_malloc.c - zone-based malloc routines */
/* $OpenLDAP$*/
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* Portions Copyright 2004 IBM Corporation
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 */
/* ACKNOWLEDGEMENTS
 * This work originally developed by Jong-Hyuk Choi for inclusion in
 * OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <sys/types.h>
#include <fcntl.h>

#include "slap.h"

#ifdef SLAP_ZONE_ALLOC

#include <sys/mman.h>

static int slap_zone_cmp(const void *v1, const void *v2);
void * slap_replenish_zopool(void *ctx);

static void
slap_zo_release(void *data)
{
	struct zone_object *zo = (struct zone_object *)data;
	ch_free( zo );
}

void
slap_zn_mem_destroy(
	void *ctx
)
{
	struct zone_heap *zh = ctx;
	int pad = 2*sizeof(int)-1, pad_shift;
	int order_start = -1, i, j;
	struct zone_object *zo;

	pad_shift = pad - 1;
	do {
		order_start++;
	} while (pad_shift >>= 1);

	ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
	for (i = 0; i < zh->zh_zoneorder - order_start + 1; i++) {
		zo = LDAP_LIST_FIRST(&zh->zh_free[i]);
		while (zo) {
			struct zone_object *zo_tmp = zo;
			zo = LDAP_LIST_NEXT(zo, zo_link);
			LDAP_LIST_REMOVE(zo_tmp, zo_link);
			LDAP_LIST_INSERT_HEAD(&zh->zh_zopool, zo_tmp, zo_link);
		}
	}
	ch_free(zh->zh_free);

	for (i = 0; i < zh->zh_numzones; i++) {
		for (j = 0; j < zh->zh_zoneorder - order_start + 1; j++) {
			ch_free(zh->zh_maps[i][j]);
		}
		ch_free(zh->zh_maps[i]);
		munmap(zh->zh_zones[i], zh->zh_zonesize);
		ldap_pvt_thread_rdwr_destroy(&zh->zh_znlock[i]);
	}
	ch_free(zh->zh_maps);
	ch_free(zh->zh_zones);
	ch_free(zh->zh_seqno);
	ch_free(zh->zh_znlock);

	avl_free(zh->zh_zonetree, slap_zo_release);

	zo = LDAP_LIST_FIRST(&zh->zh_zopool);
	while (zo) {
		struct zone_object *zo_tmp = zo;
		zo = LDAP_LIST_NEXT(zo, zo_link);
		if (!zo_tmp->zo_blockhead) {
			LDAP_LIST_REMOVE(zo_tmp, zo_link);
		}
	}
	zo = LDAP_LIST_FIRST(&zh->zh_zopool);
	while (zo) {
		struct zone_object *zo_tmp = zo;
		zo = LDAP_LIST_NEXT(zo, zo_link);
		ch_free(zo_tmp);
	}
	ldap_pvt_thread_mutex_unlock(&zh->zh_mutex);
	ldap_pvt_thread_rdwr_destroy(&zh->zh_lock);
	ldap_pvt_thread_mutex_destroy(&zh->zh_mutex);
	ch_free(zh);
}

void *
slap_zn_mem_create(
	ber_len_t initsize,
	ber_len_t maxsize,
	ber_len_t deltasize,
	ber_len_t zonesize
)
{
	struct zone_heap *zh = NULL;
	ber_len_t zpad;
	int pad = 2*sizeof(int)-1, pad_shift;
	int size_shift;
	int order = -1, order_start = -1, order_end = -1;
	int i, j;
	struct zone_object *zo;

	Debug(LDAP_DEBUG_NONE,
		"--> slap_zn_mem_create: initsize=%d, maxsize=%d\n",
		initsize, maxsize, 0);
	Debug(LDAP_DEBUG_NONE,
		"++> slap_zn_mem_create: deltasize=%d, zonesize=%d\n",
		deltasize, zonesize, 0);

	zh = (struct zone_heap *)ch_calloc(1, sizeof(struct zone_heap));

	zh->zh_fd = open("/dev/zero", O_RDWR);

	if ( zonesize ) {
		zh->zh_zonesize = zonesize;
	} else {
		zh->zh_zonesize = SLAP_ZONE_SIZE;
	}

	zpad = zh->zh_zonesize - 1;
	zh->zh_numzones = ((initsize + zpad) & ~zpad) / zh->zh_zonesize;

	if ( maxsize && maxsize >= initsize ) {
		zh->zh_maxzones = ((maxsize + zpad) & ~zpad) / zh->zh_zonesize;
	} else {
		zh->zh_maxzones = ((initsize + zpad) & ~zpad) / zh->zh_zonesize;
	}

	if ( deltasize ) {
		zh->zh_deltazones = ((deltasize + zpad) & ~zpad) / zh->zh_zonesize;
	} else {
		zh->zh_deltazones = ((SLAP_ZONE_DELTA+zpad) & ~zpad) / zh->zh_zonesize;
	}

	size_shift = zh->zh_zonesize - 1;
	do {
		order_end++;
	} while (size_shift >>= 1);

	pad_shift = pad - 1;
	do {
		order_start++;
	} while (pad_shift >>= 1);

	order = order_end - order_start + 1;

	zh->zh_zones = (void **)ch_malloc(zh->zh_maxzones * sizeof(void*));
	zh->zh_znlock = (ldap_pvt_thread_rdwr_t *)ch_malloc(
						zh->zh_maxzones * sizeof(ldap_pvt_thread_rdwr_t *));
	zh->zh_maps = (unsigned char ***)ch_malloc(
					zh->zh_maxzones * sizeof(unsigned char**));

	zh->zh_zoneorder = order_end;
	zh->zh_free = (struct zh_freelist *)
					ch_malloc(order * sizeof(struct zh_freelist));
	zh->zh_seqno = (unsigned long *)ch_calloc(zh->zh_maxzones,
											sizeof(unsigned long));
	for (i = 0; i < order; i++) {
		LDAP_LIST_INIT(&zh->zh_free[i]);
	}
	LDAP_LIST_INIT(&zh->zh_zopool);

	for (i = 0; i < zh->zh_numzones; i++) {
		zh->zh_zones[i] = mmap(0, zh->zh_zonesize, PROT_READ | PROT_WRITE,
							MAP_PRIVATE, zh->zh_fd, 0);
		zh->zh_maps[i] = (unsigned char **)
					ch_malloc(order * sizeof(unsigned char *));
		for (j = 0; j < order; j++) {
			int shiftamt = order_start + 1 + j;
			int nummaps = zh->zh_zonesize >> shiftamt;
			assert(nummaps);
			nummaps >>= 3;
			if (!nummaps) nummaps = 1;
			zh->zh_maps[i][j] = (unsigned char *)ch_malloc(nummaps);
			memset(zh->zh_maps[i][j], 0, nummaps);
		}

		if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
			slap_replenish_zopool(zh);
		}
		zo = LDAP_LIST_FIRST(&zh->zh_zopool);
		LDAP_LIST_REMOVE(zo, zo_link);
		zo->zo_ptr = zh->zh_zones[i];
		zo->zo_idx = i;
		LDAP_LIST_INSERT_HEAD(&zh->zh_free[order-1], zo, zo_link);

		if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
			slap_replenish_zopool(zh);
		}
		zo = LDAP_LIST_FIRST(&zh->zh_zopool);
		LDAP_LIST_REMOVE(zo, zo_link);
		zo->zo_ptr = zh->zh_zones[i];
		zo->zo_siz = zh->zh_zonesize;
		zo->zo_idx = i;
		avl_insert(&zh->zh_zonetree, zo, slap_zone_cmp, avl_dup_error);
		ldap_pvt_thread_rdwr_init(&zh->zh_znlock[i]);
	}

	LDAP_STAILQ_INIT(&zh->zh_latency_history_queue);
	ldap_pvt_thread_mutex_init(&zh->zh_mutex);
	ldap_pvt_thread_rdwr_init(&zh->zh_lock);

	return zh;
}

void *
slap_zn_malloc(
    ber_len_t	size,
	void *ctx
)
{
	struct zone_heap *zh = ctx;
	ber_len_t size_shift;
	int pad = 2*sizeof(int)-1, pad_shift;
	int order = -1, order_start = -1;
	struct zone_object *zo, *zo_new, *zo_left, *zo_right;
	ber_len_t *ptr, *new;
	int idx;
	unsigned long diff;
	int i, j, k;

	Debug(LDAP_DEBUG_NONE,
		"--> slap_zn_malloc: size=%d\n", size, 0, 0);

	if (!zh) return ber_memalloc_x(size, NULL);

	/* round up to doubleword boundary */
	size += 2*sizeof(ber_len_t) + pad;
	size &= ~pad;

	size_shift = size - 1;
	do {
		order++;
	} while (size_shift >>= 1);

	pad_shift = pad - 1;
	do {
		order_start++;
	} while (pad_shift >>= 1);

retry:

	ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
	for (i = order; i <= zh->zh_zoneorder &&
			LDAP_LIST_EMPTY(&zh->zh_free[i-order_start]); i++);

	if (i == order) {
		zo_new = LDAP_LIST_FIRST(&zh->zh_free[i-order_start]);
		LDAP_LIST_REMOVE(zo_new, zo_link);
		ptr = zo_new->zo_ptr;
		idx = zo_new->zo_idx;
		diff = (unsigned long)((char*)ptr -
				(char*)zh->zh_zones[idx]) >> (order + 1);
		zh->zh_maps[idx][order-order_start][diff>>3] |= (1 << (diff & 0x7));
		*ptr++ = zh->zh_seqno[idx];
		*ptr++ = size - 2*sizeof(ber_len_t);
		zo_new->zo_ptr = NULL;
		zo_new->zo_idx = -1;
		LDAP_LIST_INSERT_HEAD(&zh->zh_zopool, zo_new, zo_link);
		ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );
		Debug(LDAP_DEBUG_NONE, "slap_zn_malloc: returning 0x%x, 0x%x\n",
				ptr, (int)ptr>>(zh->zh_zoneorder+1), 0);
		return((void*)ptr);
	} else if (i <= zh->zh_zoneorder) {
		for (j = i; j > order; j--) {
			zo_left = LDAP_LIST_FIRST(&zh->zh_free[j-order_start]);
			LDAP_LIST_REMOVE(zo_left, zo_link);
			if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
				slap_replenish_zopool(zh);
			}
			zo_right = LDAP_LIST_FIRST(&zh->zh_zopool);
			LDAP_LIST_REMOVE(zo_right, zo_link);
			zo_right->zo_ptr = zo_left->zo_ptr + (1 << j);
			zo_right->zo_idx = zo_left->zo_idx;
			Debug(LDAP_DEBUG_NONE,
				"slap_zn_malloc: split (left=0x%x, right=0x%x)\n",
				zo_left->zo_ptr, zo_right->zo_ptr, 0);
			if (j == order + 1) {
				ptr = zo_left->zo_ptr;
				diff = (unsigned long)((char*)ptr -
						(char*)zh->zh_zones[zo_left->zo_idx]) >> (order+1);
				zh->zh_maps[zo_left->zo_idx][order-order_start][diff>>3] |=
						(1 << (diff & 0x7));
				*ptr++ = zh->zh_seqno[zo_left->zo_idx];
				*ptr++ = size - 2*sizeof(ber_len_t);
				LDAP_LIST_INSERT_HEAD(
						&zh->zh_free[j-1-order_start], zo_right, zo_link);
				LDAP_LIST_INSERT_HEAD(&zh->zh_zopool, zo_left, zo_link);
				ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );
				Debug(LDAP_DEBUG_NONE,
					"slap_zn_malloc: returning 0x%x, 0x%x\n",
					ptr, (int)ptr>>(zh->zh_zoneorder+1), 0);
				return((void*)ptr);
			} else {
				LDAP_LIST_INSERT_HEAD(
						&zh->zh_free[j-1-order_start], zo_right, zo_link);
				LDAP_LIST_INSERT_HEAD(
						&zh->zh_free[j-1-order_start], zo_left, zo_link);
			}
		}
		assert(0);
	} else {

		if ( zh->zh_maxzones < zh->zh_numzones + zh->zh_deltazones ) {
			ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );
			Debug( LDAP_DEBUG_TRACE,
				"zn_malloc %lu: ch_malloc\n",
				(long)size, 0, 0);
			Debug(LDAP_DEBUG_NONE,
				"slap_zn_malloc: returning 0x%x, 0x%x\n",
				ptr, (int)ptr>>(zh->zh_zoneorder+1), 0);
			return (void*)ch_malloc(size);
		}

		for (i = zh->zh_numzones; i < zh->zh_numzones+zh->zh_deltazones; i++) {
			zh->zh_zones[i] = mmap(0, zh->zh_zonesize, PROT_READ | PROT_WRITE,
								MAP_PRIVATE, zh->zh_fd, 0);
			zh->zh_maps[i] = (unsigned char **)
						ch_malloc((zh->zh_zoneorder - order_start + 1) *
						sizeof(unsigned char *));
			for (j = 0; j < zh->zh_zoneorder-order_start+1; j++) {
				int shiftamt = order_start + 1 + j;
				int nummaps = zh->zh_zonesize >> shiftamt;
				assert(nummaps);
				nummaps >>= 3;
				if (!nummaps) nummaps = 1;
				zh->zh_maps[i][j] = (unsigned char *)ch_malloc(nummaps);
				memset(zh->zh_maps[i][j], 0, nummaps);
			}
	
			if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
				slap_replenish_zopool(zh);
			}
			zo = LDAP_LIST_FIRST(&zh->zh_zopool);
			LDAP_LIST_REMOVE(zo, zo_link);
			zo->zo_ptr = zh->zh_zones[i];
			zo->zo_idx = i;
			LDAP_LIST_INSERT_HEAD(&zh->
						zh_free[zh->zh_zoneorder-order_start],zo,zo_link);
	
			if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
				slap_replenish_zopool(zh);
			}
			zo = LDAP_LIST_FIRST(&zh->zh_zopool);
			LDAP_LIST_REMOVE(zo, zo_link);
			zo->zo_ptr = zh->zh_zones[i];
			zo->zo_siz = zh->zh_zonesize;
			zo->zo_idx = i;
			avl_insert(&zh->zh_zonetree, zo, slap_zone_cmp, avl_dup_error);
			ldap_pvt_thread_rdwr_init(&zh->zh_znlock[i]);
		}
		zh->zh_numzones += zh->zh_deltazones;
		ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );
		goto retry;
	}
}

void *
slap_zn_calloc( ber_len_t n, ber_len_t size, void *ctx )
{
	void *new;

	new = slap_zn_malloc( n*size, ctx );
	if ( new ) {
		memset( new, 0, n*size );
	}
	return new;
}

void *
slap_zn_realloc(void *ptr, ber_len_t size, void *ctx)
{
	struct zone_heap *zh = ctx;
	int pad = 2*sizeof(int)-1, pad_shift;
	int order_start = -1, order = -1;
	struct zone_object zoi, *zoo;
	ber_len_t *p = (ber_len_t *)ptr, *new;
	unsigned long diff;
	int i;
	void *newptr = NULL;
	struct zone_heap *zone = NULL;

	Debug(LDAP_DEBUG_NONE,
		"--> slap_zn_realloc: ptr=0x%x, size=%d\n", ptr, size, 0);

	if (ptr == NULL)
		return slap_zn_malloc(size, zh);

	zoi.zo_ptr = p;
	zoi.zo_idx = -1;

	if (zh) {
		ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
		zoo = avl_find(zh->zh_zonetree, &zoi, slap_zone_cmp);
		ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );
	}

	/* Not our memory? */
	if (!zoo) {
		/* duplicate of realloc behavior, oh well */
		new = ber_memrealloc_x(ptr, size, NULL);
		if (new) {
			return new;
		}
		Debug(LDAP_DEBUG_ANY, "ch_realloc of %lu bytes failed\n",
				(long) size, 0, 0);
		assert(0);
		exit( EXIT_FAILURE );
	}

	assert(zoo->zo_idx != -1);	

	zone = zh->zh_zones[zoo->zo_idx];

	if (size == 0) {
		slap_zn_free(ptr, zh);
		return NULL;
	}

	newptr = slap_zn_malloc(size, zh);
	if (size < p[-1]) {
		AC_MEMCPY(newptr, ptr, size);
	} else {
		AC_MEMCPY(newptr, ptr, p[-1]);
	}
	slap_zn_free(ptr, zh);
	return newptr;
}

void
slap_zn_free(void *ptr, void *ctx)
{
	struct zone_heap *zh = ctx;
	int size, size_shift, order_size;
	int pad = 2*sizeof(int)-1, pad_shift;
	ber_len_t *p = (ber_len_t *)ptr, *tmpp;
	int order_start = -1, order = -1;
	struct zone_object zoi, *zoo, *zo;
	unsigned long diff;
	int i, k, inserted = 0, idx;
	struct zone_heap *zone = NULL;

	zoi.zo_ptr = p;
	zoi.zo_idx = -1;

	Debug(LDAP_DEBUG_NONE, "--> slap_zn_free: ptr=0x%x\n", ptr, 0, 0);

	if (zh) {
		ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
		zoo = avl_find(zh->zh_zonetree, &zoi, slap_zone_cmp);
		ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );
	}

	if (!zoo) {
		ber_memfree_x(ptr, NULL);
	} else {
		idx = zoo->zo_idx;
		assert(idx != -1);
		zone = zh->zh_zones[idx];

		size = *(--p);
		size_shift = size + 2*sizeof(ber_len_t) - 1;
		do {
			order++;
		} while (size_shift >>= 1);

		pad_shift = pad - 1;
		do {
			order_start++;
		} while (pad_shift >>= 1);

		ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
		for (i = order, tmpp = p; i <= zh->zh_zoneorder; i++) {
			order_size = 1 << (i+1);
			diff = (unsigned long)((char*)tmpp - (char*)zone) >> (i+1);
			zh->zh_maps[idx][i-order_start][diff>>3] &= (~(1 << (diff & 0x7)));
			if (diff == ((diff>>1)<<1)) {
				if (!(zh->zh_maps[idx][i-order_start][(diff+1)>>3] &
						(1<<((diff+1)&0x7)))) {
					zo = LDAP_LIST_FIRST(&zh->zh_free[i-order_start]);
					while (zo) {
						if ((char*)zo->zo_ptr == (char*)tmpp) {
							LDAP_LIST_REMOVE( zo, zo_link );
						} else if ((char*)zo->zo_ptr ==
								(char*)tmpp + order_size) {
							LDAP_LIST_REMOVE(zo, zo_link);
							break;
						}
						zo = LDAP_LIST_NEXT(zo, zo_link);
					}
					if (zo) {
						if (i < zh->zh_zoneorder) {
							inserted = 1;
							zo->zo_ptr = tmpp;
							Debug(LDAP_DEBUG_NONE,
								"slap_zn_free: merging 0x%x\n",
								zo->zo_ptr, 0, 0);
							LDAP_LIST_INSERT_HEAD(&zh->zh_free[i-order_start+1],
									zo, zo_link);
						}
						continue;
					} else {
						if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
							slap_replenish_zopool(zh);
						}
						zo = LDAP_LIST_FIRST(&zh->zh_zopool);
						LDAP_LIST_REMOVE(zo, zo_link);
						zo->zo_ptr = tmpp;
						zo->zo_idx = idx;
						Debug(LDAP_DEBUG_NONE,
							"slap_zn_free: merging 0x%x\n",
							zo->zo_ptr, 0, 0);
						LDAP_LIST_INSERT_HEAD(&zh->zh_free[i-order_start],
								zo, zo_link);
						break;

						Debug(LDAP_DEBUG_ANY, "slap_zn_free: "
							"free object not found while bit is clear.\n",
							0, 0, 0);
						assert(zo != NULL);

					}
				} else {
					if (!inserted) {
						if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
							slap_replenish_zopool(zh);
						}
						zo = LDAP_LIST_FIRST(&zh->zh_zopool);
						LDAP_LIST_REMOVE(zo, zo_link);
						zo->zo_ptr = tmpp;
						zo->zo_idx = idx;
						Debug(LDAP_DEBUG_NONE,
							"slap_zn_free: merging 0x%x\n",
							zo->zo_ptr, 0, 0);
						LDAP_LIST_INSERT_HEAD(&zh->zh_free[i-order_start],
								zo, zo_link);
					}
					break;
				}
			} else {
				if (!(zh->zh_maps[idx][i-order_start][(diff-1)>>3] &
						(1<<((diff-1)&0x7)))) {
					zo = LDAP_LIST_FIRST(&zh->zh_free[i-order_start]);
					while (zo) {
						if ((char*)zo->zo_ptr == (char*)tmpp) {
							LDAP_LIST_REMOVE(zo, zo_link);
						} else if ((char*)tmpp == zo->zo_ptr + order_size) {
							LDAP_LIST_REMOVE(zo, zo_link);
							tmpp = zo->zo_ptr;
							break;
						}
						zo = LDAP_LIST_NEXT(zo, zo_link);
					}
					if (zo) {
						if (i < zh->zh_zoneorder) {
							inserted = 1;
							Debug(LDAP_DEBUG_NONE,
								"slap_zn_free: merging 0x%x\n",
								zo->zo_ptr, 0, 0);
							LDAP_LIST_INSERT_HEAD(&zh->zh_free[i-order_start+1],
									zo, zo_link);
							continue;
						}
					} else {
						if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
							slap_replenish_zopool(zh);
						}
						zo = LDAP_LIST_FIRST(&zh->zh_zopool);
						LDAP_LIST_REMOVE(zo, zo_link);
						zo->zo_ptr = tmpp;
						zo->zo_idx = idx;
						Debug(LDAP_DEBUG_NONE,
							"slap_zn_free: merging 0x%x\n",
							zo->zo_ptr, 0, 0);
						LDAP_LIST_INSERT_HEAD(&zh->zh_free[i-order_start],
								zo, zo_link);
						break;

						Debug(LDAP_DEBUG_ANY, "slap_zn_free: "
							"free object not found while bit is clear.\n",
							0, 0, 0 );
						assert(zo != NULL);

					}
				} else {
					if ( !inserted ) {
						if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
							slap_replenish_zopool(zh);
						}
						zo = LDAP_LIST_FIRST(&zh->zh_zopool);
						LDAP_LIST_REMOVE(zo, zo_link);
						zo->zo_ptr = tmpp;
						zo->zo_idx = idx;
						Debug(LDAP_DEBUG_NONE,
							"slap_zn_free: merging 0x%x\n",
							zo->zo_ptr, 0, 0);
						LDAP_LIST_INSERT_HEAD(&zh->zh_free[i-order_start],
								zo, zo_link);
					}
					break;
				}
			}
		}
		ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );
	}
}

static int
slap_zone_cmp(const void *v1, const void *v2)
{
	const struct zone_object *zo1 = v1;
	const struct zone_object *zo2 = v2;
	char *ptr1;
	char *ptr2;
	ber_len_t zpad;

	zpad = zo2->zo_siz - 1;
	ptr1 = (char*)(((unsigned long)zo1->zo_ptr + zpad) & ~zpad);
	ptr2 = (char*)zo2->zo_ptr + ((char*)ptr1 - (char*)zo1->zo_ptr);
	ptr2 = (char*)(((unsigned long)ptr2 + zpad) & ~zpad);
	return (int)((char*)ptr1 - (char*)ptr2);
}

void *
slap_replenish_zopool(
	void *ctx
)
{
	struct zone_heap* zh = ctx;
	struct zone_object *zo_block;
	int i;

	zo_block = (struct zone_object *)ch_malloc(
					SLAP_ZONE_ZOBLOCK * sizeof(struct zone_object));

	if ( zo_block == NULL ) {
		return NULL;
	}

	zo_block[0].zo_blockhead = 1;
	LDAP_LIST_INSERT_HEAD(&zh->zh_zopool, &zo_block[0], zo_link);
	for (i = 1; i < SLAP_ZONE_ZOBLOCK; i++) {
		zo_block[i].zo_blockhead = 0;
		LDAP_LIST_INSERT_HEAD(&zh->zh_zopool, &zo_block[i], zo_link );
	}

	return zo_block;
}

int
slap_zn_invalidate(
	void *ctx,
	void *ptr
)
{
	struct zone_heap* zh = ctx;
	struct zone_object zoi, *zoo;
	struct zone_heap *zone = NULL;
	int seqno = *((ber_len_t*)ptr - 2);
	int idx = -1, rc = 0;
	int pad = 2*sizeof(int)-1, pad_shift;
	int order_start = -1, i;
	struct zone_object *zo;

	pad_shift = pad - 1;
	do {
		order_start++;
	} while (pad_shift >>= 1);

	zoi.zo_ptr = ptr;
	zoi.zo_idx = -1;

	ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
	zoo = avl_find(zh->zh_zonetree, &zoi, slap_zone_cmp);

	if (zoo) {
		idx = zoo->zo_idx;
		assert(idx != -1);
		madvise(zh->zh_zones[idx], zh->zh_zonesize, MADV_DONTNEED);
		for (i = 0; i < zh->zh_zoneorder - order_start + 1; i++) {
			int shiftamt = order_start + 1 + i;
			int nummaps = zh->zh_zonesize >> shiftamt;
			assert(nummaps);
			nummaps >>= 3;
			if (!nummaps) nummaps = 1;
			memset(zh->zh_maps[idx][i], 0, nummaps);
			zo = LDAP_LIST_FIRST(&zh->zh_free[i]);
			while (zo) {
				struct zone_object *zo_tmp = zo;
				zo = LDAP_LIST_NEXT(zo, zo_link);
				if (zo_tmp && zo_tmp->zo_idx == idx) {
					LDAP_LIST_REMOVE(zo_tmp, zo_link);
					LDAP_LIST_INSERT_HEAD(&zh->zh_zopool, zo_tmp, zo_link);
				}
			}
		}
		if (LDAP_LIST_EMPTY(&zh->zh_zopool)) {
			slap_replenish_zopool(zh);
		}
		zo = LDAP_LIST_FIRST(&zh->zh_zopool);
		LDAP_LIST_REMOVE(zo, zo_link);
		zo->zo_ptr = zh->zh_zones[idx];
		zo->zo_idx = idx;
		LDAP_LIST_INSERT_HEAD(&zh->zh_free[zh->zh_zoneorder-order_start],
								zo, zo_link);
		zh->zh_seqno[idx]++;
	} else {
		Debug(LDAP_DEBUG_NONE, "zone not found for (ctx=0x%x, ptr=0x%x) !\n",
				ctx, ptr, 0);
	}

	ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );
	Debug(LDAP_DEBUG_NONE, "zone %d invalidate\n", idx, 0, 0);
	return rc;
}

int
slap_zn_validate(
	void *ctx,
	void *ptr,
	int seqno
)
{
	struct zone_heap* zh = ctx;
	struct zone_object zoi, *zoo;
	struct zone_heap *zone = NULL;
	int idx, rc = 0;

	zoi.zo_ptr = ptr;
	zoi.zo_idx = -1;

	zoo = avl_find(zh->zh_zonetree, &zoi, slap_zone_cmp);

	if (zoo) {
		idx = zoo->zo_idx;
		assert(idx != -1);
		assert(seqno <= zh->zh_seqno[idx]);
		rc = (seqno == zh->zh_seqno[idx]);
	}

	return rc;
}

int slap_zh_rlock(
	void *ctx
)
{
	struct zone_heap* zh = ctx;
	ldap_pvt_thread_rdwr_rlock(&zh->zh_lock);
}

int slap_zh_runlock(
	void *ctx
)
{
	struct zone_heap* zh = ctx;
	ldap_pvt_thread_rdwr_runlock(&zh->zh_lock);
}

int slap_zh_wlock(
	void *ctx
)
{
	struct zone_heap* zh = ctx;
	ldap_pvt_thread_rdwr_wlock(&zh->zh_lock);
}

int slap_zh_wunlock(
	void *ctx
)
{
	struct zone_heap* zh = ctx;
	ldap_pvt_thread_rdwr_wunlock(&zh->zh_lock);
}

int slap_zn_rlock(
	void *ctx,
	void *ptr
)
{
	struct zone_heap* zh = ctx;
	struct zone_object zoi, *zoo;
	struct zone_heap *zone = NULL;
	int idx;

	zoi.zo_ptr = ptr;
	zoi.zo_idx = -1;

	ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
	zoo = avl_find(zh->zh_zonetree, &zoi, slap_zone_cmp);
	ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );

	if (zoo) {
		idx = zoo->zo_idx;
		assert(idx != -1);
		ldap_pvt_thread_rdwr_rlock(&zh->zh_znlock[idx]);
	}
}

int slap_zn_runlock(
	void *ctx,
	void *ptr
)
{
	struct zone_heap* zh = ctx;
	struct zone_object zoi, *zoo;
	struct zone_heap *zone = NULL;
	int idx;

	zoi.zo_ptr = ptr;
	zoi.zo_idx = -1;

	ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
	zoo = avl_find(zh->zh_zonetree, &zoi, slap_zone_cmp);
	ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );

	if (zoo) {
		idx = zoo->zo_idx;
		assert(idx != -1);
		ldap_pvt_thread_rdwr_runlock(&zh->zh_znlock[idx]);
	}
}

int slap_zn_wlock(
	void *ctx,
	void *ptr
)
{
	struct zone_heap* zh = ctx;
	struct zone_object zoi, *zoo;
	struct zone_heap *zone = NULL;
	int idx;

	zoi.zo_ptr = ptr;
	zoi.zo_idx = -1;

	ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
	zoo = avl_find(zh->zh_zonetree, &zoi, slap_zone_cmp);
	ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );

	if (zoo) {
		idx = zoo->zo_idx;
		assert(idx != -1);
		ldap_pvt_thread_rdwr_wlock(&zh->zh_znlock[idx]);
	}
}

int slap_zn_wunlock(
	void *ctx,
	void *ptr
)
{
	struct zone_heap* zh = ctx;
	struct zone_object zoi, *zoo;
	struct zone_heap *zone = NULL;
	int idx;

	zoi.zo_ptr = ptr;
	zoi.zo_idx = -1;

	ldap_pvt_thread_mutex_lock( &zh->zh_mutex );
	zoo = avl_find(zh->zh_zonetree, &zoi, slap_zone_cmp);
	ldap_pvt_thread_mutex_unlock( &zh->zh_mutex );

	if (zoo) {
		idx = zoo->zo_idx;
		assert(idx != -1);
		ldap_pvt_thread_rdwr_wunlock(&zh->zh_znlock[idx]);
	}
}

#define T_SEC_IN_USEC 1000000

static int
slap_timediff(struct timeval *tv_begin, struct timeval *tv_end)
{
	uint64_t t_begin, t_end, t_diff;

	t_begin = T_SEC_IN_USEC * tv_begin->tv_sec + tv_begin->tv_usec;
	t_end  = T_SEC_IN_USEC * tv_end->tv_sec  + tv_end->tv_usec;
	t_diff  = t_end - t_begin;
	
	if ( t_diff < 0 )
		t_diff = 0;

	return (int)t_diff;
}

void
slap_set_timing(struct timeval *tv_set)
{
	gettimeofday(tv_set, (struct timezone *)NULL);
}

int
slap_measure_timing(struct timeval *tv_set, struct timeval *tv_measure)
{
	gettimeofday(tv_measure, (struct timezone *)NULL);
	return(slap_timediff(tv_set, tv_measure));
}

#define EMA_WEIGHT 0.999000
#define SLAP_ZN_LATENCY_HISTORY_QLEN 500
int
slap_zn_latency_history(void* ctx, int ea_latency)
{
/* TODO: monitor /proc/stat (swap) as well */
	struct zone_heap* zh = ctx;
	double t_diff = 0.0;

	zh->zh_ema_latency = (double)ea_latency * (1.0 - EMA_WEIGHT)
					+ zh->zh_ema_latency * EMA_WEIGHT;
	if (!zh->zh_swapping && zh->zh_ema_samples++ % 100 == 99) {
		struct zone_latency_history *zlh_entry;
		zlh_entry = ch_calloc(1, sizeof(struct zone_latency_history));
		zlh_entry->zlh_latency = zh->zh_ema_latency;
		LDAP_STAILQ_INSERT_TAIL(
				&zh->zh_latency_history_queue, zlh_entry, zlh_next);
		zh->zh_latency_history_qlen++;
		while (zh->zh_latency_history_qlen > SLAP_ZN_LATENCY_HISTORY_QLEN) {
			struct zone_latency_history *zlh;
			zlh = LDAP_STAILQ_FIRST(&zh->zh_latency_history_queue);
			LDAP_STAILQ_REMOVE_HEAD(
					&zh->zh_latency_history_queue, zlh_next);
			zh->zh_latency_history_qlen--;
			ch_free(zlh);
		}
		if (zh->zh_latency_history_qlen == SLAP_ZN_LATENCY_HISTORY_QLEN) {
			struct zone_latency_history *zlh_first, *zlh_last;
			zlh_first = LDAP_STAILQ_FIRST(&zh->zh_latency_history_queue);
			zlh_last = LDAP_STAILQ_LAST(&zh->zh_latency_history_queue,
						zone_latency_history, zlh_next);
			t_diff = zlh_last->zlh_latency - zlh_first->zlh_latency;
		}
		if (t_diff >= 2000) {
			zh->zh_latency_jump++;
		} else {
			zh->zh_latency_jump = 0;
		}
		if (zh->zh_latency_jump > 3) {
			zh->zh_latency_jump = 0;
			zh->zh_swapping = 1;
		}
	}
	return zh->zh_swapping;
}
#endif /* SLAP_ZONE_ALLOC */
