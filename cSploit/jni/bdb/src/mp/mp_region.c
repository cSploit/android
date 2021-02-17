/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/mp.h"

static int	__memp_init_config __P((ENV *, MPOOL *, int));
static void	__memp_region_size __P((ENV *, roff_t *, u_int32_t *));

#define	MPOOL_DEFAULT_PAGESIZE	(4 * 1024)

/*
 * __memp_open --
 *	Internal version of memp_open: only called from ENV->open.
 *
 * PUBLIC: int __memp_open __P((ENV *, int));
 */
int
__memp_open(env, create_ok)
	ENV *env;
	int create_ok;
{
	DB_ENV *dbenv;
	DB_MPOOL *dbmp;
	MPOOL *mp, *mp_i;
	REGINFO reginfo;
	roff_t cache_size, max_size, reg_size;
	u_int i, max_nreg;
	u_int32_t htab_buckets, *regids;
	int create, ret;

	dbenv = env->dbenv;
	cache_size = 0;

	/* Calculate the region size and hash bucket count. */
	__memp_region_size(env, &max_size, &htab_buckets);

	/* Create and initialize the DB_MPOOL structure. */
	if ((ret = __os_calloc(env, 1, sizeof(*dbmp), &dbmp)) != 0)
		return (ret);
	LIST_INIT(&dbmp->dbregq);
	TAILQ_INIT(&dbmp->dbmfq);
	dbmp->env = env;

	/* Join/create the first mpool region. */
	memset(&reginfo, 0, sizeof(REGINFO));
	reginfo.env = env;
	reginfo.type = REGION_TYPE_MPOOL;
	reginfo.id = INVALID_REGION_ID;
	reginfo.flags = REGION_JOIN_OK;

	/* Calculate the minimum allocation. */
	reg_size = sizeof(MPOOL);
	reg_size += MPOOL_FILE_BUCKETS * sizeof(DB_MPOOL_HASH);
	reg_size += htab_buckets * sizeof(DB_MPOOL_HASH);
	reg_size += (dbenv->mp_pagesize == 0 ?
		MPOOL_DEFAULT_PAGESIZE : dbenv->mp_pagesize) * 10;
	if (reg_size > max_size)
		reg_size = max_size;

	if (create_ok)
		F_SET(&reginfo, REGION_CREATE_OK);
	if ((ret = __env_region_attach(env, &reginfo, reg_size, max_size)) != 0)
		goto err;
	cache_size = reginfo.rp->max;
	if (F_ISSET(env, ENV_PRIVATE))
		reginfo.max_alloc = reginfo.rp->max;

	/*
	 * If we created the region, initialize it.  Create or join any
	 * additional regions.
	 */
	create = F_ISSET(&reginfo, REGION_CREATE);
	if (create) {
		/*
		 * We define how many regions there are going to be, allocate
		 * the REGINFO structures and create them.  Make sure we don't
		 * clear the wrong entries on error.
		 */
		max_nreg = __memp_max_regions(env);
		if ((ret = __os_calloc(env,
		    max_nreg, sizeof(REGINFO), &dbmp->reginfo)) != 0)
			goto err;
		/* Make sure we don't clear the wrong entries on error. */
		dbmp->reginfo[0] = reginfo;
		for (i = 1; i < max_nreg; ++i)
			dbmp->reginfo[i].id = INVALID_REGION_ID;

		/* Initialize the first region. */
		if ((ret = __memp_init(env, dbmp,
		    0, htab_buckets, max_nreg)) != 0)
			goto err;

		/*
		 * Create/initialize remaining regions and copy their IDs into
		 * the first region.
		 */
		mp = R_ADDR(dbmp->reginfo, dbmp->reginfo[0].rp->primary);
		regids = R_ADDR(dbmp->reginfo, mp->regids);
		regids[0] = dbmp->reginfo[0].id;
		for (i = 1; i < dbenv->mp_ncache; ++i) {
			dbmp->reginfo[i].env = env;
			dbmp->reginfo[i].type = REGION_TYPE_MPOOL;
			dbmp->reginfo[i].id = INVALID_REGION_ID;
			dbmp->reginfo[i].flags = REGION_CREATE_OK;
			if ((ret = __env_region_attach(
			    env, &dbmp->reginfo[i], reg_size, max_size)) != 0)
				goto err;
			if (F_ISSET(env, ENV_PRIVATE))
				dbmp->reginfo[i].max_alloc = max_size;
			cache_size += dbmp->reginfo[i].rp->max;
			if ((ret = __memp_init(env, dbmp,
			    i, htab_buckets, max_nreg)) != 0)
				goto err;

			regids[i] = dbmp->reginfo[i].id;
		}
		mp->gbytes = (u_int32_t) (cache_size / GIGABYTE);
		mp->bytes = (u_int32_t) (cache_size % GIGABYTE);
	} else {
		/*
		 * Determine how many regions there are going to be, allocate
		 * the REGINFO structures and fill in local copies of that
		 * information.
		 */
		mp = R_ADDR(&reginfo, reginfo.rp->primary);
		dbenv->mp_ncache = mp->nreg;
		if ((ret = __os_calloc(env,
		    mp->max_nreg, sizeof(REGINFO), &dbmp->reginfo)) != 0)
			goto err;
		/* Make sure we don't clear the wrong entries on error. */
		for (i = 0; i < dbenv->mp_ncache; ++i)
			dbmp->reginfo[i].id = INVALID_REGION_ID;
		dbmp->reginfo[0] = reginfo;

		/* Join remaining regions. */
		regids = R_ADDR(dbmp->reginfo, mp->regids);
		for (i = 1; i < dbenv->mp_ncache; ++i) {
			dbmp->reginfo[i].env = env;
			dbmp->reginfo[i].type = REGION_TYPE_MPOOL;
			dbmp->reginfo[i].id = regids[i];
			dbmp->reginfo[i].flags = REGION_JOIN_OK;
			if ((ret = __env_region_attach(
			    env, &dbmp->reginfo[i], 0, 0)) != 0)
				goto err;
		}
	}

	/* Set the local addresses for the regions. */
	for (i = 0; i < dbenv->mp_ncache; ++i) {
		mp_i = dbmp->reginfo[i].primary =
		    R_ADDR(&dbmp->reginfo[i], dbmp->reginfo[i].rp->primary);
		dbmp->reginfo[i].mtx_alloc = mp_i->mtx_region;
	}

	/* If the region is threaded, allocate a mutex to lock the handles. */
	if ((ret = __mutex_alloc(env,
	    MTX_MPOOL_HANDLE, DB_MUTEX_PROCESS_ONLY, &dbmp->mutex)) != 0)
		goto err;

	env->mp_handle = dbmp;

	/* A process joining the region may reset the mpool configuration. */
	if ((ret = __memp_init_config(env, mp, create)) != 0)
		return (ret);

	return (0);

err:	(void)__mutex_free(env, &dbmp->mutex);
	(void)__memp_region_detach(env, dbmp);
	return (ret);
}

/* __memp_region_detach
 *	Detach from any attached mempool regions.
 *
 * PUBLIC: int __memp_region_detach __P((ENV *, DB_MPOOL *));
 */
int
__memp_region_detach(env, dbmp)
	ENV *env;
	DB_MPOOL *dbmp;
{
	u_int i;

	if (dbmp != NULL &&
	    dbmp->reginfo != NULL && dbmp->reginfo[0].addr != NULL) {
		for (i = 0; i < env->dbenv->mp_ncache; ++i)
			if (dbmp->reginfo[i].id != INVALID_REGION_ID)
				(void)__env_region_detach(
				    env, &dbmp->reginfo[i], 0);
		__os_free(env, dbmp->reginfo);
	}
	env->mp_handle = NULL;
	return (0);
}

/*
 * __memp_init --
 *	Initialize a MPOOL structure in shared memory.
 *
 * PUBLIC: int	__memp_init
 * PUBLIC:     __P((ENV *, DB_MPOOL *, u_int, u_int32_t, u_int));
 */
int
__memp_init(env, dbmp, reginfo_off, htab_buckets, max_nreg)
	ENV *env;
	DB_MPOOL *dbmp;
	u_int reginfo_off, max_nreg;
	u_int32_t htab_buckets;
{
	BH *frozen_bhp;
	BH_FROZEN_ALLOC *frozen;
	DB_ENV *dbenv;
	DB_MPOOL_HASH *htab, *hp;
	MPOOL *mp, *main_mp;
	REGINFO *infop;
	db_mutex_t mtx_base, mtx_discard, mtx_prev;
	u_int32_t i, mp_mtxcount;
	int ret;
	void *p;

	dbenv = env->dbenv;

	infop = &dbmp->reginfo[reginfo_off];
	if ((ret = __env_alloc(infop, sizeof(MPOOL), &infop->primary)) != 0)
		goto mem_err;
	infop->rp->primary = R_OFFSET(infop, infop->primary);
	mp = infop->primary;
	memset(mp, 0, sizeof(*mp));

	if ((ret =
	    __mutex_alloc(env, MTX_MPOOL_REGION, 0, &mp->mtx_region)) != 0)
		return (ret);

	/*
	 * Intializing the first mpool region allocates the mpool region id
	 * array, file table and, if not ENV_PRIVATE, all the cache regions'
	 * hash bucket mutexes in a single contiguous block of mutex ids, which
	 * remain allocated when the cache is resized. The block is 'known' to
	 * start with the first id (mtx_base), and to end #regions * mp_mtxcount
	 * later. In private environments, mutex ids are not smallish integers,
	 * but __env_alloc()'d pointers. Since a range of (base, count) doesn't
	 * work for these likely-scattered mutexes, we allocate private threaded
	 * mutexes as they are needed. Private non-threaded caches don't need
	 * any mutexes at all.
	 */
	if ((mp_mtxcount = dbenv->mp_mtxcount) == 0)
		mp_mtxcount = dbenv->mp_mtxcount = htab_buckets;
	if (!MUTEX_ON(env) ||
	    F_ISSET(env, ENV_PRIVATE | ENV_THREAD) == ENV_PRIVATE)
		mp_mtxcount = dbenv->mp_mtxcount = 0;
	if (reginfo_off == 0) {
		ZERO_LSN(mp->lsn);

		mp->nreg = dbenv->mp_ncache;
		mp->max_nreg = max_nreg;
		if ((ret = __env_alloc(&dbmp->reginfo[0],
		    max_nreg * sizeof(u_int32_t), &p)) != 0)
			goto mem_err;
		mp->regids = R_OFFSET(dbmp->reginfo, p);
		mp->nbuckets = dbenv->mp_ncache * htab_buckets;

		/* Allocate file table space and initialize it. */
		if ((ret = __env_alloc(infop,
		    MPOOL_FILE_BUCKETS * sizeof(DB_MPOOL_HASH), &htab)) != 0)
			goto mem_err;
		mp->ftab = R_OFFSET(infop, htab);
		for (i = 0; i < MPOOL_FILE_BUCKETS; i++) {
			if ((ret = __mutex_alloc(env,
			     MTX_MPOOL_FILE_BUCKET, 0, &htab[i].mtx_hash)) != 0)
				return (ret);
			SH_TAILQ_INIT(&htab[i].hash_bucket);
			atomic_init(&htab[i].hash_page_dirty, 0);
		}

		mtx_base = mtx_prev = MUTEX_INVALID;
		if (F_ISSET(env, ENV_PRIVATE))
			goto no_prealloc;
		for (i = 0; i < mp->max_nreg * mp_mtxcount; i++) {
			if ((ret = __mutex_alloc(env, MTX_MPOOL_HASH_BUCKET,
			    DB_MUTEX_SHARED, &mtx_discard)) != 0)
				return (ret);
			if (i == 0)
				mtx_base = mtx_discard;
			else
				DB_ASSERT(env, mtx_base == MUTEX_INVALID ||
				    mtx_discard == mtx_prev + 1);
			mtx_prev = mtx_discard;
		}
	} else {
		main_mp = dbmp->reginfo[0].primary;
		htab = R_ADDR(&dbmp->reginfo[0], main_mp->htab);
		mtx_base = htab[0].mtx_hash;
	}

	/*
	 * If we preallocated all the mutexes, then in regions after the first,
	 * we skip mutexes in use in earlier regions. Each region has the same
	 * number of buckets.
	 */
no_prealloc:
	mtx_base += reginfo_off * mp_mtxcount;

	/* Allocate hash table space and initialize it. */
	if ((ret = __env_alloc(infop,
	    htab_buckets * sizeof(DB_MPOOL_HASH), &htab)) != 0)
		goto mem_err;
	mp->htab = R_OFFSET(infop, htab);
	for (i = 0; i < htab_buckets; i++) {
		hp = &htab[i];
		/*
		 * Set mtx_hash to do no locking, or share a mutex with an
		 * earlier hash bucket in this region, or assign it from the
		 * block of mutexes allocated above, or (in a private
		 * environment) allocate a new mutex.
		 */
		if (mp_mtxcount == 0)
			hp->mtx_hash = MUTEX_INVALID;
		else if (i >= mp_mtxcount)
			hp->mtx_hash = htab[i % mp_mtxcount].mtx_hash;
		else if (!F_ISSET(env, ENV_PRIVATE))
			hp->mtx_hash = mtx_base + i;
		else if ((ret = __mutex_alloc(env, MTX_MPOOL_HASH_BUCKET,
		    DB_MUTEX_SHARED, &hp->mtx_hash)) != 0)
			return (ret);
		SH_TAILQ_INIT(&hp->hash_bucket);
		atomic_init(&hp->hash_page_dirty, 0);
#ifdef HAVE_STATISTICS
		hp->hash_io_wait = 0;
		hp->hash_frozen = hp->hash_thawed = hp->hash_frozen_freed = 0;
#endif
		hp->flags = 0;
		ZERO_LSN(hp->old_reader);
	}
	mp->htab_buckets = htab_buckets;
	mp->htab_mutexes = mp_mtxcount;
	mp->pagesize = dbenv->mp_pagesize == 0 ?
		MPOOL_DEFAULT_PAGESIZE : dbenv->mp_pagesize;

	SH_TAILQ_INIT(&mp->free_frozen);
	SH_TAILQ_INIT(&mp->alloc_frozen);

	/*
	 * Pre-allocate one frozen buffer header.  This avoids situations where
	 * the cache becomes full of pages and we don't even have the 28 bytes
	 * (or so) available to allocate a frozen buffer header.
	 */
	if ((ret = __env_alloc(infop,
	    sizeof(BH_FROZEN_ALLOC) + sizeof(BH_FROZEN_PAGE), &frozen)) != 0)
		goto mem_err;
	SH_TAILQ_INSERT_TAIL(&mp->alloc_frozen, frozen, links);
	frozen_bhp = (BH *)(frozen + 1);
	frozen_bhp->mtx_buf = MUTEX_INVALID;
	SH_TAILQ_INSERT_TAIL(&mp->free_frozen, frozen_bhp, hq);

	/*
	 * Only the environment creator knows the total cache size,
	 * fill in those fields now.
	 */
	mp->gbytes = dbenv->mp_gbytes;
	mp->bytes = dbenv->mp_bytes;
	infop->mtx_alloc = mp->mtx_region;
	return (0);

mem_err:__db_errx(env, DB_STR("3026",
	    "Unable to allocate memory for mpool region"));
	return (ret);
}

/*
 * PUBLIC: u_int32_t __memp_max_regions __P((ENV *));
 */
u_int32_t
__memp_max_regions(env)
	ENV *env;
{
	DB_ENV *dbenv;
	roff_t reg_size, max_size;
	size_t max_nreg;

	dbenv = env->dbenv;

	if (dbenv->mp_max_gbytes == 0 && dbenv->mp_max_bytes == 0)
		return (dbenv->mp_ncache);
	__memp_region_size(env, &reg_size, NULL);
	max_size =
	    (roff_t)dbenv->mp_max_gbytes * GIGABYTE + dbenv->mp_max_bytes;
	max_nreg = (max_size + reg_size / 2) / reg_size;

	/* Sanity check that the number of regions fits in 32 bits. */
	DB_ASSERT(env, max_nreg == (u_int32_t)max_nreg);

	if (max_nreg <= dbenv->mp_ncache)
		max_nreg = dbenv->mp_ncache;
	return ((u_int32_t)max_nreg);
}

/*
 * __memp_region_size --
 *	Size the region and figure out how many hash buckets we'll have.
 */
static void
__memp_region_size(env, reg_sizep, htab_bucketsp)
	ENV *env;
	roff_t *reg_sizep;
	u_int32_t *htab_bucketsp;
{
	DB_ENV *dbenv;
	roff_t reg_size, cache_size;
	u_int32_t pgsize;

	dbenv = env->dbenv;

	/*
	 * Figure out how big each cache region is.  Cast an operand to roff_t
	 * so we do 64-bit arithmetic as appropriate.
	 */
	cache_size = (roff_t)dbenv->mp_gbytes * GIGABYTE + dbenv->mp_bytes;
	reg_size = cache_size / dbenv->mp_ncache;
	if (reg_sizep != NULL)
		*reg_sizep = reg_size;

	/*
	 * Figure out how many hash buckets each region will have.  Assume we
	 * want to keep the hash chains with under 3 pages on each chain.  We
	 * don't know the pagesize in advance, and it may differ for different
	 * files.  Use a pagesize of 4K for the calculation -- we walk these
	 * chains a lot, they must be kept short.  We use 2.5 as this maintains
	 * compatibility with previous releases.
	 *
	 * XXX
	 * Cache sizes larger than 10TB would cause 32-bit wrapping in the
	 * calculation of the number of hash buckets.  This probably isn't
	 * something we need to worry about right now, but is checked when the
	 * cache size is set.
	 */
	if (htab_bucketsp != NULL) {
		if (dbenv->mp_tablesize != 0)
			*htab_bucketsp = __db_tablesize(dbenv->mp_tablesize);
		else {
			if ((pgsize = dbenv->mp_pagesize) == 0)
				pgsize = MPOOL_DEFAULT_PAGESIZE;
			*htab_bucketsp = __db_tablesize(
				(u_int32_t)(reg_size / (2.5 * pgsize)));
		}
	}

}

/*
 * __memp_region_mutex_count --
 *	Return the number of mutexes the mpool region will need.
 *
 * PUBLIC: u_int32_t __memp_region_mutex_count __P((ENV *));
 */
u_int32_t
__memp_region_mutex_count(env)
	ENV *env;
{
	DB_ENV *dbenv;
	u_int32_t htab_buckets;
	roff_t reg_size;
	u_int32_t max_region, num_per_cache, pgsize;

	dbenv = env->dbenv;

	__memp_region_size(env, &reg_size, &htab_buckets);
	if (dbenv->mp_mtxcount != 0)
		htab_buckets = dbenv->mp_mtxcount;
	max_region = __memp_max_regions(env);
	if ((pgsize = dbenv->mp_pagesize) == 0) {
		/*
		 * If MVCC is on during environment creation, provide enough
		 * mutexes so that half the cache can be frozen buffer headers.
		 */
		if (F_ISSET(env->dbenv, DB_ENV_MULTIVERSION))
			pgsize = (MPOOL_DEFAULT_PAGESIZE +
			    sizeof(BH_FROZEN_ALLOC) +
			    sizeof(BH_FROZEN_PAGE)) / 2;
		else
			pgsize = MPOOL_DEFAULT_PAGESIZE;
	}

	/*
	 * We need a couple of mutexes for the region itself, one for each
	 * file handle (MPOOLFILE) the application allocates, one for each
	 * of the MPOOL_FILE_BUCKETS, and each cache has one mutex per
	 * hash bucket. We then need one mutex per page in the cache,
	 * the worst case is really big if the pages are 512 bytes.
	 */
	num_per_cache = htab_buckets + (u_int32_t)(reg_size / pgsize);
	return ((max_region * num_per_cache) + 50 + MPOOL_FILE_BUCKETS);
}

/*
 * __memp_init_config --
 *	Initialize shared configuration information.
 */
static int
__memp_init_config(env, mp, create)
	ENV *env;
	MPOOL *mp;
	int create;
{
	DB_ENV *dbenv;

	dbenv = env->dbenv;

	MPOOL_SYSTEM_LOCK(env);
	if (create) {
		mp->mp_mmapsize = (db_size_t)dbenv->mp_mmapsize;
		mp->mp_maxopenfd = dbenv->mp_maxopenfd;
		mp->mp_maxwrite = dbenv->mp_maxwrite;
		mp->mp_maxwrite_sleep = dbenv->mp_maxwrite_sleep;
	} else {
		if (dbenv->mp_mmapsize != 0 &&
		    mp->mp_mmapsize != (db_size_t)dbenv->mp_mmapsize)
			__db_msg(env, DB_STR("3044",
"Warning: Ignoring maximum memory map size when joining environment"));

		if (dbenv->mp_maxopenfd != 0 &&
		    mp->mp_maxopenfd != dbenv->mp_maxopenfd)
			__db_msg(env, DB_STR("3045",
"Warning: Ignoring max open file descriptors value when joining environment"));

		if ((dbenv->mp_maxwrite != 0 &&
		    mp->mp_maxwrite != dbenv->mp_maxwrite) ||
		    (dbenv->mp_maxwrite_sleep != 0 &&
		    mp->mp_maxwrite_sleep != dbenv->mp_maxwrite_sleep))
			__db_msg(env, DB_STR("3046",
"Warning: Ignoring maximum sequential writes value when joining environment"));
	}
	MPOOL_SYSTEM_UNLOCK(env);

	return (0);
}

/*
 * __memp_env_refresh --
 *	Clean up after the mpool system on a close or failed open.
 *
 * PUBLIC: int __memp_env_refresh __P((ENV *));
 */
int
__memp_env_refresh(env)
	ENV *env;
{
	DB_MPOOL *dbmp;
	DB_MPOOLFILE *dbmfp;
	DB_MPREG *mpreg;
	MPOOL *mp, *c_mp;
	REGINFO *infop;
	u_int32_t i, nreg;
	int ret, t_ret;

	ret = 0;
	dbmp = env->mp_handle;
	mp = dbmp->reginfo[0].primary;
	nreg = mp->nreg;

	/*
	 * If a private region, return the memory to the heap.  Not needed for
	 * filesystem-backed or system shared memory regions, that memory isn't
	 * owned by any particular process.
	 */
	if (!F_ISSET(env, ENV_PRIVATE))
		goto not_priv;

	for (i = 0; i < nreg; ++i) {
		infop = &dbmp->reginfo[i];
		if ((t_ret = __memp_region_bhfree(infop)) != 0 && ret == 0)
			ret = t_ret;
	}

not_priv:
	/* Discard DB_MPOOLFILEs. */
	while ((dbmfp = TAILQ_FIRST(&dbmp->dbmfq)) != NULL)
		if ((t_ret = __memp_fclose(dbmfp, DB_FLUSH)) != 0 && ret == 0)
			ret = t_ret;

	/* Discard DB_MPREGs. */
	if (dbmp->pg_inout != NULL)
		__os_free(env, dbmp->pg_inout);
	while ((mpreg = LIST_FIRST(&dbmp->dbregq)) != NULL) {
		LIST_REMOVE(mpreg, q);
		__os_free(env, mpreg);
	}

	/* Discard the DB_MPOOL thread mutex. */
	if ((t_ret = __mutex_free(env, &dbmp->mutex)) != 0 && ret == 0)
		ret = t_ret;

	if (F_ISSET(env, ENV_PRIVATE)) {
		/* Discard REGION IDs. */
		infop = &dbmp->reginfo[0];
		infop->mtx_alloc = MUTEX_INVALID;
		__memp_free(infop, R_ADDR(infop, mp->regids));

		/* Discard all the MPOOLFILEs. */
		if ((t_ret = __memp_discard_all_mpfs(env, mp)) != 0 && ret == 0)
			ret = t_ret;
		/* Discard the File table. */
		__memp_free(infop, R_ADDR(infop, mp->ftab));

		/* Discard Hash tables. */
		for (i = 0; i < nreg; ++i) {
			infop = &dbmp->reginfo[i];
			c_mp = infop->primary;
			infop->mtx_alloc = MUTEX_INVALID;
			__memp_free(infop, R_ADDR(infop, c_mp->htab));
		}
	}

	/* Detach from the region. */
	for (i = 0; i < nreg; ++i) {
		infop = &dbmp->reginfo[i];
		if ((t_ret =
		    __env_region_detach(env, infop, 0)) != 0 && ret == 0)
			ret = t_ret;
	}

	/* Discard DB_MPOOL. */
	__os_free(env, dbmp->reginfo);
	__os_free(env, dbmp);

	env->mp_handle = NULL;
	return (ret);
}

/*
 * __memp_region_bhfree --
 *	Discard the buffers for a region.
 *
 * PUBLIC: int __memp_region_bhfree __P((REGINFO *));
 */
int
__memp_region_bhfree(infop)
	REGINFO *infop;
{
	BH *bhp;
	BH_FROZEN_ALLOC *frozen_alloc;
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	ENV *env;
	MPOOL *c_mp;
	u_int32_t bucket;
	int ret, t_ret;

	env = infop->env;
	dbmp = env->mp_handle;
	ret = 0;

	/* Discard buffers. */
	c_mp = infop->primary;
	for (hp = R_ADDR(infop, c_mp->htab), bucket = 0;
	    bucket < c_mp->htab_buckets; ++hp, ++bucket) {
		while ((bhp = SH_TAILQ_FIRST(&hp->hash_bucket, __bh)) != NULL)
			if (F_ISSET(bhp, BH_FROZEN))
				SH_TAILQ_REMOVE(&hp->hash_bucket,
				    bhp, hq, __bh);
			else {
				if (F_ISSET(bhp, BH_DIRTY)) {
					atomic_dec(env, &hp->hash_page_dirty);
					F_CLR(bhp, BH_DIRTY | BH_DIRTY_CREATE);
				}
				atomic_inc(env, &bhp->ref);
				if ((t_ret = __memp_bhfree(dbmp, infop,
				    R_ADDR(dbmp->reginfo, bhp->mf_offset),
				    hp, bhp, BH_FREE_FREEMEM |
				    BH_FREE_UNLOCKED)) != 0 && ret == 0)
					ret = t_ret;
			}
	}
	MPOOL_REGION_LOCK(env, infop);
	while ((frozen_alloc = SH_TAILQ_FIRST(
	    &c_mp->alloc_frozen, __bh_frozen_a)) != NULL) {
		SH_TAILQ_REMOVE(&c_mp->alloc_frozen,
		    frozen_alloc, links, __bh_frozen_a);
		__env_alloc_free(infop, frozen_alloc);
	}
	MPOOL_REGION_UNLOCK(env, infop);

	return (ret);
}
