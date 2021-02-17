/*
 * xdelta.c:  xdelta generator.
 *
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 */


#include <assert.h>

#include <apr_general.h>        /* for APR_INLINE */
#include <apr_hash.h>

#include "svn_delta.h"
#include "delta.h"

#include "private/svn_adler32.h"

/* This is pseudo-adler32. It is adler32 without the prime modulus.
   The idea is borrowed from monotone, and is a translation of the C++
   code.  Graydon Hoare, the author of the original code, gave his
   explicit permission to use it under these terms at 8:02pm on
   Friday, February 11th, 2005.  */

/* Size of the blocks we compute checksums for. This was chosen out of
   thin air.  Monotone used 64, xdelta1 used 64, rsync uses 128.
   However, later optimizations assume it to be 256 or less.
 */
#define MATCH_BLOCKSIZE 64

/* Feed C_IN into the adler32 checksum and remove C_OUT at the same time.
 * This function may (and will) only be called for characters that are
 * MATCH_BLOCKSIZE positions apart.
 *
 * Please note that the lower 16 bits cannot overflow in neither direction.
 * Therefore, we don't need to split the value into separate values for
 * sum(char) and sum(sum(char)).
 */
static APR_INLINE apr_uint32_t
adler32_replace(apr_uint32_t adler32, const char c_out, const char c_in)
{
  adler32 -= (MATCH_BLOCKSIZE * 0x10000u * ((unsigned char) c_out));

  adler32 -= (unsigned char)c_out;
  adler32 += (unsigned char)c_in;

  return adler32 + adler32 * 0x10000;
}

/* Calculate an peudo-adler32 checksum for MATCH_BLOCKSIZE bytes starting
   at DATA.  Return the checksum value.  */

static APR_INLINE apr_uint32_t
init_adler32(const char *data)
{
  const unsigned char *input = (const unsigned char *)data;
  const unsigned char *last = input + MATCH_BLOCKSIZE;

  apr_uint32_t s1 = 0;
  apr_uint32_t s2 = 0;

  for (; input < last; input += 8)
    {
      s1 += input[0]; s2 += s1;
      s1 += input[1]; s2 += s1;
      s1 += input[2]; s2 += s1;
      s1 += input[3]; s2 += s1;
      s1 += input[4]; s2 += s1;
      s1 += input[5]; s2 += s1;
      s1 += input[6]; s2 += s1;
      s1 += input[7]; s2 += s1;
    }

  return s2 * 0x10000 + s1;
}

/* Information for a block of the delta source.  The length of the
   block is the smaller of MATCH_BLOCKSIZE and the difference between
   the size of the source data and the position of this block. */
struct block
{
  apr_uint32_t adlersum;
  apr_size_t pos;
};

/* A hash table, using open addressing, of the blocks of the source. */
struct blocks
{
  /* The largest valid index of slots. */
  apr_size_t max;
  /* Source buffer that the positions in SLOTS refer to. */
  const char* data;
  /* The vector of blocks.  A pos value of (apr_size_t)-1 represents an unused
     slot. */
  struct block *slots;
};


/* Return a hash value calculated from the adler32 SUM, suitable for use with
   our hash table. */
static apr_size_t hash_func(apr_uint32_t sum)
{
  /* Since the adl32 checksum have a bad distribution for the 11th to 16th
     bits when used for our small block size, we add some bits from the
     other half of the checksum. */
  return sum ^ (sum >> 12);
}

/* Insert a block with the checksum ADLERSUM at position POS in the source
   data into the table BLOCKS.  Ignore true duplicates, i.e. blocks with
   actually the same content. */
static void
add_block(struct blocks *blocks, apr_uint32_t adlersum, apr_size_t pos)
{
  apr_size_t h = hash_func(adlersum) & blocks->max;

  /* This will terminate, since we know that we will not fill the table. */
  for (; blocks->slots[h].pos != (apr_size_t)-1; h = (h + 1) & blocks->max)
    if (blocks->slots[h].adlersum == adlersum)
      if (memcmp(blocks->data + blocks->slots[h].pos, blocks->data + pos,
                 MATCH_BLOCKSIZE) == 0)
        return;

  blocks->slots[h].adlersum = adlersum;
  blocks->slots[h].pos = pos;
}

/* Find a block in BLOCKS with the checksum ADLERSUM and matching the content
   at DATA, returning its position in the source data.  If there is no such
   block, return (apr_size_t)-1. */
static apr_size_t
find_block(const struct blocks *blocks,
           apr_uint32_t adlersum,
           const char* data)
{
  apr_size_t h = hash_func(adlersum) & blocks->max;

  for (; blocks->slots[h].pos != (apr_size_t)-1; h = (h + 1) & blocks->max)
    if (blocks->slots[h].adlersum == adlersum)
      if (memcmp(blocks->data + blocks->slots[h].pos, data,
                 MATCH_BLOCKSIZE) == 0)
        return blocks->slots[h].pos;

  return (apr_size_t)-1;
}

/* Initialize the matches table from DATA of size DATALEN.  This goes
   through every block of MATCH_BLOCKSIZE bytes in the source and
   checksums it, inserting the result into the BLOCKS table.  */
static void
init_blocks_table(const char *data,
                  apr_size_t datalen,
                  struct blocks *blocks,
                  apr_pool_t *pool)
{
  apr_size_t i;
  apr_size_t nblocks;
  apr_size_t nslots = 1;

  /* Be pesimistic about the block count. */
  nblocks = datalen / MATCH_BLOCKSIZE + 1;
  /* Find nearest larger power of two. */
  while (nslots <= nblocks)
    nslots *= 2;
  /* Double the number of slots to avoid a too high load. */
  nslots *= 2;
  blocks->max = nslots - 1;
  blocks->data = data;
  blocks->slots = apr_palloc(pool, nslots * sizeof(*(blocks->slots)));
  for (i = 0; i < nslots; ++i)
    {
      /* Avoid using an indeterminate value in the lookup. */
      blocks->slots[i].adlersum = 0;
      blocks->slots[i].pos = (apr_size_t)-1;
    }

  /* If there is an odd block at the end of the buffer, we will
     not use that shorter block for deltification (only indirectly
     as an extension of some previous block). */
  for (i = 0; i + MATCH_BLOCKSIZE <= datalen; i += MATCH_BLOCKSIZE)
    add_block(blocks, init_adler32(data + i), i);
}

/* Return the lowest position at which A and B differ. If no difference
 * can be found in the first MAX_LEN characters, MAX_LEN will be returned.
 */
static apr_size_t
match_length(const char *a, const char *b, apr_size_t max_len)
{
  apr_size_t pos = 0;

#if SVN_UNALIGNED_ACCESS_IS_OK

  /* Chunky processing is so much faster ...
   *
   * We can't make this work on architectures that require aligned access
   * because A and B will probably have different alignment. So, skipping
   * the first few chars until alignment is reached is not an option.
   */
  for (; pos + sizeof(apr_size_t) <= max_len; pos += sizeof(apr_size_t))
    if (*(const apr_size_t*)(a + pos) != *(const apr_size_t*)(b + pos))
      break;

#endif

  for (; pos < max_len; ++pos)
    if (a[pos] != b[pos])
      break;

  return pos;
}

/* Try to find a match for the target data B in BLOCKS, and then
   extend the match as long as data in A and B at the match position
   continues to match.  We set the position in A we ended up in (in
   case we extended it backwards) in APOSP and update the correspnding
   position within B given in BPOSP. PENDING_INSERT_START sets the
   lower limit to BPOSP.
   Return number of matching bytes starting at ASOP.  Return 0 if
   no match has been found.
 */
static apr_size_t
find_match(const struct blocks *blocks,
           const apr_uint32_t rolling,
           const char *a,
           apr_size_t asize,
           const char *b,
           apr_size_t bsize,
           apr_size_t *bposp,
           apr_size_t *aposp,
           apr_size_t pending_insert_start)
{
  apr_size_t apos, bpos = *bposp;
  apr_size_t delta, max_delta;

  apos = find_block(blocks, rolling, b + bpos);

  /* See if we have a match.  */
  if (apos == (apr_size_t)-1)
    return 0;

  /* Extend the match forward as far as possible */
  max_delta = asize - apos - MATCH_BLOCKSIZE < bsize - bpos - MATCH_BLOCKSIZE
            ? asize - apos - MATCH_BLOCKSIZE
            : bsize - bpos - MATCH_BLOCKSIZE;
  delta = match_length(a + apos + MATCH_BLOCKSIZE,
                       b + bpos + MATCH_BLOCKSIZE,
                       max_delta);

  /* See if we can extend backwards (max MATCH_BLOCKSIZE-1 steps because A's
     content has been sampled only every MATCH_BLOCKSIZE positions).  */
  while (apos > 0 && bpos > pending_insert_start && a[apos-1] == b[bpos-1])
    {
      --apos;
      --bpos;
      ++delta;
    }

  *aposp = apos;
  *bposp = bpos;

  return MATCH_BLOCKSIZE + delta;
}


/* Compute a delta from A to B using xdelta.

   The basic xdelta algorithm is as follows:

   1. Go through the source data, checksumming every MATCH_BLOCKSIZE
      block of bytes using adler32, and inserting the checksum into a
      match table with the position of the match.
   2. Go through the target byte by byte, seeing if that byte starts a
      match that we have in the match table.
      2a. If so, try to extend the match as far as possible both
          forwards and backwards, and then insert a source copy
          operation into the delta ops builder for the match.
      2b. If not, insert the byte as new data using an insert delta op.

   Our implementation doesn't immediately insert "insert" operations,
   it waits until we have another copy, or we are done.  The reasoning
   is twofold:

   1. Otherwise, we would just be building a ton of 1 byte insert
      operations
   2. So that we can extend a source match backwards into a pending
     insert operation, and possibly remove the need for the insert
     entirely.  This can happen due to stream alignment.
*/
static void
compute_delta(svn_txdelta__ops_baton_t *build_baton,
              const char *a,
              apr_uint32_t asize,
              const char *b,
              apr_uint32_t bsize,
              apr_pool_t *pool)
{
  struct blocks blocks;
  apr_uint32_t rolling;
  apr_size_t lo = 0, pending_insert_start = 0;

  /* If the size of the target is smaller than the match blocksize, just
     insert the entire target.  */
  if (bsize < MATCH_BLOCKSIZE)
    {
      svn_txdelta__insert_op(build_baton, svn_txdelta_new,
                             0, bsize, b, pool);
      return;
    }

  /* Initialize the matches table.  */
  init_blocks_table(a, asize, &blocks, pool);

  /* Initialize our rolling checksum.  */
  rolling = init_adler32(b);
  while (lo < bsize)
    {
      apr_size_t matchlen = 0;
      apr_size_t apos;

      if (lo + MATCH_BLOCKSIZE <= bsize)
        matchlen = find_match(&blocks, rolling, a, asize, b, bsize,
                              &lo, &apos, pending_insert_start);

      /* If we didn't find a real match, insert the byte at the target
         position into the pending insert.  */
      if (matchlen == 0)
        {
          /* move block one position forward. Short blocks at the end of
             the buffer cannot be used as the beginning of a new match */
          if (lo + MATCH_BLOCKSIZE < bsize)
            rolling = adler32_replace(rolling, b[lo], b[lo+MATCH_BLOCKSIZE]);

          lo++;
        }
      else
        {
          /* store the sequence of B that is between the matches */
          if (lo - pending_insert_start > 0)
            svn_txdelta__insert_op(build_baton, svn_txdelta_new,
                                   0, lo - pending_insert_start,
                                   b + pending_insert_start, pool);

          /* Reset the pending insert start to immediately after the
             match. */
          lo += matchlen;
          pending_insert_start = lo;
          svn_txdelta__insert_op(build_baton, svn_txdelta_source,
                                 apos, matchlen, NULL, pool);

          /* Calculate the Adler32 sum for the first block behind the match.
           * Ignore short buffers at the end of B.
           */
          if (lo + MATCH_BLOCKSIZE <= bsize)
            rolling = init_adler32(b + lo);
        }
    }

  /* If we still have an insert pending at the end, throw it in.  */
  if (lo - pending_insert_start > 0)
    {
      svn_txdelta__insert_op(build_baton, svn_txdelta_new,
                             0, lo - pending_insert_start,
                             b + pending_insert_start, pool);
    }
}

void
svn_txdelta__xdelta(svn_txdelta__ops_baton_t *build_baton,
                    const char *data,
                    apr_size_t source_len,
                    apr_size_t target_len,
                    apr_pool_t *pool)
{
  /*  We should never be asked to compute something when the source_len is 0;
      we just use a single insert op there (and rely on zlib for
      compression). */
  assert(source_len != 0);
  compute_delta(build_baton, data, source_len,
                data + source_len, target_len,
                pool);
}
