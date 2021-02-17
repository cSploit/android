/*-------------------------------------------------------------------------
 *
 * spccache.h
 *	  Tablespace cache.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/spccache.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef SPCCACHE_H
#define SPCCACHE_H

void get_tablespace_page_costs(Oid spcid, float8 *spc_random_page_cost,
						  float8 *spc_seq_page_cost);

#endif   /* SPCCACHE_H */
