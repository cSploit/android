/* temp_serializer.c: serialization functions for caching of FSFS structures
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

#include <apr_pools.h>

#include "svn_pools.h"
#include "svn_hash.h"

#include "id.h"
#include "svn_fs.h"

#include "private/svn_fs_util.h"
#include "private/svn_temp_serializer.h"

#include "temp_serializer.h"

/* Utility to encode a signed NUMBER into a variable-length sequence of
 * 8-bit chars in KEY_BUFFER and return the last writen position.
 *
 * Numbers will be stored in 7 bits / byte and using byte values above
 * 32 (' ') to make them combinable with other string by simply separating
 * individual parts with spaces.
 */
static char*
encode_number(apr_int64_t number, char *key_buffer)
{
  /* encode the sign in the first byte */
  if (number < 0)
  {
    number = -number;
    *key_buffer = (number & 63) + ' ' + 65;
  }
  else
    *key_buffer = (number & 63) + ' ' + 1;
  number /= 64;

  /* write 7 bits / byte until no significant bits are left */
  while (number)
  {
    *++key_buffer = (number & 127) + ' ' + 1;
    number /= 128;
  }

  /* return the last written position */
  return key_buffer;
}

/* Prepend the NUMBER to the STRING in a space efficient way that no other
 * (number,string) combination can produce the same result.
 * Allocate temporaries as well as the result from POOL.
 */
const char*
svn_fs_fs__combine_number_and_string(apr_int64_t number,
                                     const char *string,
                                     apr_pool_t *pool)
{
  apr_size_t len = strlen(string);

  /* number part requires max. 10x7 bits + 1 space.
   * Add another 1 for the terminal 0 */
  char *key_buffer = apr_palloc(pool, len + 12);
  const char *key = key_buffer;

  /* Prepend the number to the string and separate them by space. No other
   * number can result in the same prefix, no other string in the same
   * postfix nor can the boundary between them be ambiguous. */
  key_buffer = encode_number(number, key_buffer);
  *++key_buffer = ' ';
  memcpy(++key_buffer, string, len+1);

  /* return the start of the key */
  return key;
}

/* Combine the numbers A and B a space efficient way that no other
 * combination of numbers can produce the same result.
 * Allocate temporaries as well as the result from POOL.
 */
const char*
svn_fs_fs__combine_two_numbers(apr_int64_t a,
                               apr_int64_t b,
                               apr_pool_t *pool)
{
  /* encode numbers as 2x 10x7 bits + 1 space + 1 terminating \0*/
  char *key_buffer = apr_palloc(pool, 22);
  const char *key = key_buffer;

  /* combine the numbers. Since the separator is disjoint from any part
   * of the encoded numbers, there is no other combination that can yield
   * the same result */
  key_buffer = encode_number(a, key_buffer);
  *++key_buffer = ' ';
  key_buffer = encode_number(b, ++key_buffer);
  *++key_buffer = '\0';

  /* return the start of the key */
  return key;
}

/* Utility function to serialize string S in the given serialization CONTEXT.
 */
static void
serialize_svn_string(svn_temp_serializer__context_t *context,
                     const svn_string_t * const *s)
{
  const svn_string_t *string = *s;

  /* Nothing to do for NULL string references. */
  if (string == NULL)
    return;

  svn_temp_serializer__push(context,
                            (const void * const *)s,
                            sizeof(*string));

  /* the "string" content may actually be arbitrary binary data.
   * Thus, we cannot use svn_temp_serializer__add_string. */
  svn_temp_serializer__push(context,
                            (const void * const *)&string->data,
                            string->len);

  /* back to the caller's nesting level */
  svn_temp_serializer__pop(context);
  svn_temp_serializer__pop(context);
}

/* Utility function to deserialize the STRING inside the BUFFER.
 */
static void
deserialize_svn_string(void *buffer, svn_string_t **string)
{
  svn_temp_deserializer__resolve(buffer, (void **)string);
  if (*string == NULL)
    return;

  svn_temp_deserializer__resolve(*string, (void **)&(*string)->data);
}

/* Utility function to serialize checkum CS within the given serialization
 * CONTEXT.
 */
static void
serialize_checksum(svn_temp_serializer__context_t *context,
                   svn_checksum_t * const *cs)
{
  const svn_checksum_t *checksum = *cs;
  if (checksum == NULL)
    return;

  svn_temp_serializer__push(context,
                            (const void * const *)cs,
                            sizeof(*checksum));

  /* The digest is arbitrary binary data.
   * Thus, we cannot use svn_temp_serializer__add_string. */
  svn_temp_serializer__push(context,
                            (const void * const *)&checksum->digest,
                            svn_checksum_size(checksum));

  /* return to the caller's nesting level */
  svn_temp_serializer__pop(context);
  svn_temp_serializer__pop(context);
}

/* Utility function to deserialize the checksum CS inside the BUFFER.
 */
static void
deserialize_checksum(void *buffer, svn_checksum_t * const *cs)
{
  svn_temp_deserializer__resolve(buffer, (void **)cs);
  if (*cs == NULL)
    return;

  svn_temp_deserializer__resolve(*cs, (void **)&(*cs)->digest);
}

/* Utility function to serialize the REPRESENTATION within the given
 * serialization CONTEXT.
 */
static void
serialize_representation(svn_temp_serializer__context_t *context,
                         representation_t * const *representation)
{
  const representation_t * rep = *representation;
  if (rep == NULL)
    return;

  /* serialize the representation struct itself */
  svn_temp_serializer__push(context,
                            (const void * const *)representation,
                            sizeof(*rep));

  /* serialize sub-structures */
  serialize_checksum(context, &rep->md5_checksum);
  serialize_checksum(context, &rep->sha1_checksum);

  svn_temp_serializer__add_string(context, &rep->txn_id);
  svn_temp_serializer__add_string(context, &rep->uniquifier);

  /* return to the caller's nesting level */
  svn_temp_serializer__pop(context);
}

/* Utility function to deserialize the REPRESENTATIONS inside the BUFFER.
 */
static void
deserialize_representation(void *buffer,
                           representation_t **representation)
{
  representation_t *rep;

  /* fixup the reference to the representation itself */
  svn_temp_deserializer__resolve(buffer, (void **)representation);
  rep = *representation;
  if (rep == NULL)
    return;

  /* fixup of sub-structures */
  deserialize_checksum(rep, &rep->md5_checksum);
  deserialize_checksum(rep, &rep->sha1_checksum);

  svn_temp_deserializer__resolve(rep, (void **)&rep->txn_id);
  svn_temp_deserializer__resolve(rep, (void **)&rep->uniquifier);
}

/* auxilliary structure representing the content of a directory hash */
typedef struct hash_data_t
{
  /* number of entries in the directory */
  apr_size_t count;

  /* number of unused dir entry buckets in the index */
  apr_size_t over_provision;

  /* internal modifying operations counter
   * (used to repack data once in a while) */
  apr_size_t operations;

  /* size of the serialization buffer actually used.
   * (we will allocate more than we actually need such that we may
   * append more data in situ later) */
  apr_size_t len;

  /* reference to the entries */
  svn_fs_dirent_t **entries;

  /* size of the serialized entries and don't be too wasteful
   * (needed since the entries are no longer in sequence) */
  apr_uint32_t *lengths;
} hash_data_t;

static int
compare_dirent_id_names(const void *lhs, const void *rhs)
{
  return strcmp((*(const svn_fs_dirent_t *const *)lhs)->name,
                (*(const svn_fs_dirent_t *const *)rhs)->name);
}

/* Utility function to serialize the *ENTRY_P into a the given
 * serialization CONTEXT. Return the serialized size of the
 * dir entry in *LENGTH.
 */
static void
serialize_dir_entry(svn_temp_serializer__context_t *context,
                    svn_fs_dirent_t **entry_p,
                    apr_uint32_t *length)
{
  svn_fs_dirent_t *entry = *entry_p;
  apr_size_t initial_length = svn_temp_serializer__get_length(context);

  svn_temp_serializer__push(context,
                            (const void * const *)entry_p,
                            sizeof(svn_fs_dirent_t));

  svn_fs_fs__id_serialize(context, &entry->id);
  svn_temp_serializer__add_string(context, &entry->name);

  *length = (apr_uint32_t)(  svn_temp_serializer__get_length(context)
                           - APR_ALIGN_DEFAULT(initial_length));

  svn_temp_serializer__pop(context);
}

/* Utility function to serialize the ENTRIES into a new serialization
 * context to be returned. Allocation will be made form POOL.
 */
static svn_temp_serializer__context_t *
serialize_dir(apr_hash_t *entries, apr_pool_t *pool)
{
  hash_data_t hash_data;
  apr_hash_index_t *hi;
  apr_size_t i = 0;
  svn_temp_serializer__context_t *context;

  /* calculate sizes */
  apr_size_t count = apr_hash_count(entries);
  apr_size_t over_provision = 2 + count / 4;
  apr_size_t entries_len = (count + over_provision) * sizeof(svn_fs_dirent_t*);
  apr_size_t lengths_len = (count + over_provision) * sizeof(apr_uint32_t);

  /* copy the hash entries to an auxilliary struct of known layout */
  hash_data.count = count;
  hash_data.over_provision = over_provision;
  hash_data.operations = 0;
  hash_data.entries = apr_palloc(pool, entries_len);
  hash_data.lengths = apr_palloc(pool, lengths_len);

  for (hi = apr_hash_first(pool, entries); hi; hi = apr_hash_next(hi), ++i)
    hash_data.entries[i] = svn__apr_hash_index_val(hi);

  /* sort entry index by ID name */
  qsort(hash_data.entries,
        count,
        sizeof(*hash_data.entries),
        compare_dirent_id_names);

  /* Serialize that aux. structure into a new one. Also, provide a good
   * estimate for the size of the buffer that we will need. */
  context = svn_temp_serializer__init(&hash_data,
                                      sizeof(hash_data),
                                      50 + count * 200 + entries_len,
                                      pool);

  /* serialize entries references */
  svn_temp_serializer__push(context,
                            (const void * const *)&hash_data.entries,
                            entries_len);

  /* serialize the individual entries and their sub-structures */
  for (i = 0; i < count; ++i)
    serialize_dir_entry(context,
                        &hash_data.entries[i],
                        &hash_data.lengths[i]);

  svn_temp_serializer__pop(context);

  /* serialize entries references */
  svn_temp_serializer__push(context,
                            (const void * const *)&hash_data.lengths,
                            lengths_len);

  return context;
}

/* Utility function to reconstruct a dir entries hash from serialized data
 * in BUFFER and HASH_DATA. Allocation will be made form POOL.
 */
static apr_hash_t *
deserialize_dir(void *buffer, hash_data_t *hash_data, apr_pool_t *pool)
{
  apr_hash_t *result = apr_hash_make(pool);
  apr_size_t i;
  apr_size_t count;
  svn_fs_dirent_t *entry;
  svn_fs_dirent_t **entries;

  /* resolve the reference to the entries array */
  svn_temp_deserializer__resolve(buffer, (void **)&hash_data->entries);
  entries = hash_data->entries;

  /* fixup the references within each entry and add it to the hash */
  for (i = 0, count = hash_data->count; i < count; ++i)
    {
      svn_temp_deserializer__resolve(entries, (void **)&entries[i]);
      entry = hash_data->entries[i];

      /* pointer fixup */
      svn_temp_deserializer__resolve(entry, (void **)&entry->name);
      svn_fs_fs__id_deserialize(entry, (svn_fs_id_t **)&entry->id);

      /* add the entry to the hash */
      apr_hash_set(result, entry->name, APR_HASH_KEY_STRING, entry);
    }

  /* return the now complete hash */
  return result;
}

void
svn_fs_fs__noderev_serialize(svn_temp_serializer__context_t *context,
                             node_revision_t * const *noderev_p)
{
  const node_revision_t *noderev = *noderev_p;
  if (noderev == NULL)
    return;

  /* serialize the representation struct itself */
  svn_temp_serializer__push(context,
                            (const void * const *)noderev_p,
                            sizeof(*noderev));

  /* serialize sub-structures */
  svn_fs_fs__id_serialize(context, &noderev->id);
  svn_fs_fs__id_serialize(context, &noderev->predecessor_id);
  serialize_representation(context, &noderev->prop_rep);
  serialize_representation(context, &noderev->data_rep);

  svn_temp_serializer__add_string(context, &noderev->copyfrom_path);
  svn_temp_serializer__add_string(context, &noderev->copyroot_path);
  svn_temp_serializer__add_string(context, &noderev->created_path);

  /* return to the caller's nesting level */
  svn_temp_serializer__pop(context);
}


void
svn_fs_fs__noderev_deserialize(void *buffer,
                               node_revision_t **noderev_p)
{
  node_revision_t *noderev;

  /* fixup the reference to the representation itself,
   * if this is part of a parent structure. */
  if (buffer != *noderev_p)
    svn_temp_deserializer__resolve(buffer, (void **)noderev_p);

  noderev = *noderev_p;
  if (noderev == NULL)
    return;

  /* fixup of sub-structures */
  svn_fs_fs__id_deserialize(noderev, (svn_fs_id_t **)&noderev->id);
  svn_fs_fs__id_deserialize(noderev, (svn_fs_id_t **)&noderev->predecessor_id);
  deserialize_representation(noderev, &noderev->prop_rep);
  deserialize_representation(noderev, &noderev->data_rep);

  svn_temp_deserializer__resolve(noderev, (void **)&noderev->copyfrom_path);
  svn_temp_deserializer__resolve(noderev, (void **)&noderev->copyroot_path);
  svn_temp_deserializer__resolve(noderev, (void **)&noderev->created_path);
}


/* Utility function to serialize COUNT svn_txdelta_op_t objects
 * at OPS in the given serialization CONTEXT.
 */
static void
serialize_txdelta_ops(svn_temp_serializer__context_t *context,
                      const svn_txdelta_op_t * const * ops,
                      apr_size_t count)
{
  if (*ops == NULL)
    return;

  /* the ops form a contiguous chunk of memory with no further references */
  svn_temp_serializer__push(context,
                            (const void * const *)ops,
                            count * sizeof(svn_txdelta_op_t));
  svn_temp_serializer__pop(context);
}

/* Utility function to serialize W in the given serialization CONTEXT.
 */
static void
serialize_txdeltawindow(svn_temp_serializer__context_t *context,
                        svn_txdelta_window_t * const * w)
{
  svn_txdelta_window_t *window = *w;

  /* serialize the window struct itself */
  svn_temp_serializer__push(context,
                            (const void * const *)w,
                            sizeof(svn_txdelta_window_t));

  /* serialize its sub-structures */
  serialize_txdelta_ops(context, &window->ops, window->num_ops);
  serialize_svn_string(context, &window->new_data);

  svn_temp_serializer__pop(context);
}

svn_error_t *
svn_fs_fs__serialize_txdelta_window(char **buffer,
                                    apr_size_t *buffer_size,
                                    void *item,
                                    apr_pool_t *pool)
{
  svn_fs_fs__txdelta_cached_window_t *window_info = item;
  svn_stringbuf_t *serialized;

  /* initialize the serialization process and allocate a buffer large
   * enough to do without the need of re-allocations in most cases. */
  apr_size_t text_len = window_info->window->new_data
                      ? window_info->window->new_data->len
                      : 0;
  svn_temp_serializer__context_t *context =
      svn_temp_serializer__init(window_info,
                                sizeof(*window_info),
                                500 + text_len,
                                pool);

  /* serialize the sub-structure(s) */
  serialize_txdeltawindow(context, &window_info->window);

  /* return the serialized result */
  serialized = svn_temp_serializer__get(context);

  *buffer = serialized->data;
  *buffer_size = serialized->len;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__deserialize_txdelta_window(void **item,
                                      char *buffer,
                                      apr_size_t buffer_size,
                                      apr_pool_t *pool)
{
  svn_txdelta_window_t *window;

  /* Copy the _full_ buffer as it also contains the sub-structures. */
  svn_fs_fs__txdelta_cached_window_t *window_info =
      (svn_fs_fs__txdelta_cached_window_t *)buffer;

  /* pointer reference fixup */
  svn_temp_deserializer__resolve(window_info,
                                 (void **)&window_info->window);
  window = window_info->window;

  svn_temp_deserializer__resolve(window, (void **)&window->ops);

  deserialize_svn_string(window, (svn_string_t**)&window->new_data);

  /* done */
  *item = window_info;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__serialize_manifest(char **data,
                              apr_size_t *data_len,
                              void *in,
                              apr_pool_t *pool)
{
  apr_array_header_t *manifest = in;

  *data_len = sizeof(apr_off_t) *manifest->nelts;
  *data = apr_palloc(pool, *data_len);
  memcpy(*data, manifest->elts, *data_len);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__deserialize_manifest(void **out,
                                char *data,
                                apr_size_t data_len,
                                apr_pool_t *pool)
{
  apr_array_header_t *manifest = apr_array_make(pool, 1, sizeof(apr_off_t));

  manifest->nelts = (int) (data_len / sizeof(apr_off_t));
  manifest->nalloc = (int) (data_len / sizeof(apr_off_t));
  manifest->elts = (char*)data;

  *out = manifest;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__serialize_id(char **data,
                        apr_size_t *data_len,
                        void *in,
                        apr_pool_t *pool)
{
  const svn_fs_id_t *id = in;
  svn_stringbuf_t *serialized;

  /* create an (empty) serialization context with plenty of buffer space */
  svn_temp_serializer__context_t *context =
      svn_temp_serializer__init(NULL, 0, 250, pool);

  /* serialize the id */
  svn_fs_fs__id_serialize(context, &id);

  /* return serialized data */
  serialized = svn_temp_serializer__get(context);
  *data = serialized->data;
  *data_len = serialized->len;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__deserialize_id(void **out,
                          char *data,
                          apr_size_t data_len,
                          apr_pool_t *pool)
{
  /* Copy the _full_ buffer as it also contains the sub-structures. */
  svn_fs_id_t *id = (svn_fs_id_t *)data;

  /* fixup of all pointers etc. */
  svn_fs_fs__id_deserialize(id, &id);

  /* done */
  *out = id;
  return SVN_NO_ERROR;
}

/** Caching node_revision_t objects. **/

svn_error_t *
svn_fs_fs__serialize_node_revision(char **buffer,
                                    apr_size_t *buffer_size,
                                    void *item,
                                    apr_pool_t *pool)
{
  svn_stringbuf_t *serialized;
  node_revision_t *noderev = item;

  /* create an (empty) serialization context with plenty of buffer space */
  svn_temp_serializer__context_t *context =
      svn_temp_serializer__init(NULL, 0, 503, pool);

  /* serialize the noderev */
  svn_fs_fs__noderev_serialize(context, &noderev);

  /* return serialized data */
  serialized = svn_temp_serializer__get(context);
  *buffer = serialized->data;
  *buffer_size = serialized->len;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__deserialize_node_revision(void **item,
                                     char *buffer,
                                     apr_size_t buffer_size,
                                     apr_pool_t *pool)
{
  /* Copy the _full_ buffer as it also contains the sub-structures. */
  node_revision_t *noderev = (node_revision_t *)buffer;

  /* fixup of all pointers etc. */
  svn_fs_fs__noderev_deserialize(noderev, &noderev);

  /* done */
  *item = noderev;
  return SVN_NO_ERROR;
}

/* Utility function that returns the directory serialized inside CONTEXT
 * to DATA and DATA_LEN. */
static svn_error_t *
return_serialized_dir_context(svn_temp_serializer__context_t *context,
                              char **data,
                              apr_size_t *data_len)
{
  svn_stringbuf_t *serialized = svn_temp_serializer__get(context);

  *data = serialized->data;
  *data_len = serialized->blocksize;
  ((hash_data_t *)serialized->data)->len = serialized->len;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__serialize_dir_entries(char **data,
                                 apr_size_t *data_len,
                                 void *in,
                                 apr_pool_t *pool)
{
  apr_hash_t *dir = in;

  /* serialize the dir content into a new serialization context
   * and return the serialized data */
  return return_serialized_dir_context(serialize_dir(dir, pool),
                                       data,
                                       data_len);
}

svn_error_t *
svn_fs_fs__deserialize_dir_entries(void **out,
                                   char *data,
                                   apr_size_t data_len,
                                   apr_pool_t *pool)
{
  /* Copy the _full_ buffer as it also contains the sub-structures. */
  hash_data_t *hash_data = (hash_data_t *)data;

  /* reconstruct the hash from the serialized data */
  *out = deserialize_dir(hash_data, hash_data, pool);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__get_sharded_offset(void **out,
                              const char *data,
                              apr_size_t data_len,
                              void *baton,
                              apr_pool_t *pool)
{
  apr_off_t *manifest = (apr_off_t *)data;
  apr_int64_t shard_pos = *(apr_int64_t *)baton;

  *(apr_off_t *)out = manifest[shard_pos];

  return SVN_NO_ERROR;
}

/* Utility function that returns the lowest index of the first entry in
 * *ENTRIES that points to a dir entry with a name equal or larger than NAME.
 * If an exact match has been found, *FOUND will be set to TRUE. COUNT is
 * the number of valid entries in ENTRIES.
 */
static apr_size_t
find_entry(svn_fs_dirent_t **entries,
           const char *name,
           apr_size_t count,
           svn_boolean_t *found)
{
  /* binary search for the desired entry by name */
  apr_size_t lower = 0;
  apr_size_t upper = count;
  apr_size_t middle;

  for (middle = upper / 2; lower < upper; middle = (upper + lower) / 2)
    {
      const svn_fs_dirent_t *entry =
          svn_temp_deserializer__ptr(entries, (const void **)&entries[middle]);
      const char* entry_name =
          svn_temp_deserializer__ptr(entry, (const void **)&entry->name);

      int diff = strcmp(entry_name, name);
      if (diff < 0)
        lower = middle + 1;
      else
        upper = middle;
    }

  /* check whether we actually found a match */
  *found = FALSE;
  if (lower < count)
    {
      const svn_fs_dirent_t *entry =
          svn_temp_deserializer__ptr(entries, (const void **)&entries[lower]);
      const char* entry_name =
          svn_temp_deserializer__ptr(entry, (const void **)&entry->name);

      if (strcmp(entry_name, name) == 0)
        *found = TRUE;
    }

  return lower;
}

svn_error_t *
svn_fs_fs__extract_dir_entry(void **out,
                             const char *data,
                             apr_size_t data_len,
                             void *baton,
                             apr_pool_t *pool)
{
  hash_data_t *hash_data = (hash_data_t *)data;
  const char* name = baton;
  svn_boolean_t found;

  /* resolve the reference to the entries array */
  const svn_fs_dirent_t * const *entries =
    svn_temp_deserializer__ptr(data, (const void **)&hash_data->entries);

  /* resolve the reference to the lengths array */
  const apr_uint32_t *lengths =
    svn_temp_deserializer__ptr(data, (const void **)&hash_data->lengths);

  /* binary search for the desired entry by name */
  apr_size_t pos = find_entry((svn_fs_dirent_t **)entries,
                              name,
                              hash_data->count,
                              &found);

  /* de-serialize that entry or return NULL, if no match has been found */
  *out = NULL;
  if (found)
    {
      const svn_fs_dirent_t *source =
          svn_temp_deserializer__ptr(entries, (const void **)&entries[pos]);

      /* Entries have been serialized one-by-one, each time including all
       * nestes structures and strings. Therefore, they occupy a single
       * block of memory whose end-offset is either the beginning of the
       * next entry or the end of the buffer
       */
      apr_size_t size = lengths[pos];

      /* copy & deserialize the entry */
      svn_fs_dirent_t *new_entry = apr_palloc(pool, size);
      memcpy(new_entry, source, size);

      svn_temp_deserializer__resolve(new_entry, (void **)&new_entry->name);
      svn_fs_fs__id_deserialize(new_entry, (svn_fs_id_t **)&new_entry->id);
      *(svn_fs_dirent_t **)out = new_entry;
    }

  return SVN_NO_ERROR;
}

/* Utility function for svn_fs_fs__replace_dir_entry that implements the
 * modification as a simply deserialize / modify / serialize sequence.
 */
static svn_error_t *
slowly_replace_dir_entry(char **data,
                         apr_size_t *data_len,
                         void *baton,
                         apr_pool_t *pool)
{
  replace_baton_t *replace_baton = (replace_baton_t *)baton;
  hash_data_t *hash_data = (hash_data_t *)*data;
  apr_hash_t *dir;

  SVN_ERR(svn_fs_fs__deserialize_dir_entries((void **)&dir,
                                             *data,
                                             hash_data->len,
                                             pool));
  apr_hash_set(dir,
               replace_baton->name,
               APR_HASH_KEY_STRING,
               replace_baton->new_entry);

  return svn_fs_fs__serialize_dir_entries(data, data_len, dir, pool);
}

svn_error_t *
svn_fs_fs__replace_dir_entry(char **data,
                             apr_size_t *data_len,
                             void *baton,
                             apr_pool_t *pool)
{
  replace_baton_t *replace_baton = (replace_baton_t *)baton;
  hash_data_t *hash_data = (hash_data_t *)*data;
  svn_boolean_t found;
  svn_fs_dirent_t **entries;
  apr_uint32_t *lengths;
  apr_uint32_t length;
  apr_size_t pos;

  svn_temp_serializer__context_t *context;

  /* after quite a number of operations, let's re-pack everything.
   * This is to limit the number of vasted space as we cannot overwrite
   * existing data but must always append. */
  if (hash_data->operations > 2 + hash_data->count / 4)
    return slowly_replace_dir_entry(data, data_len, baton, pool);

  /* resolve the reference to the entries array */
  entries = (svn_fs_dirent_t **)
    svn_temp_deserializer__ptr((const char *)hash_data,
                               (const void **)&hash_data->entries);

  /* resolve the reference to the lengths array */
  lengths = (apr_uint32_t *)
    svn_temp_deserializer__ptr((const char *)hash_data,
                               (const void **)&hash_data->lengths);

  /* binary search for the desired entry by name */
  pos = find_entry(entries, replace_baton->name, hash_data->count, &found);

  /* handle entry removal (if found at all) */
  if (replace_baton->new_entry == NULL)
    {
      if (found)
        {
          /* remove reference to the entry from the index */
          memmove(&entries[pos],
                  &entries[pos + 1],
                  sizeof(entries[pos]) * (hash_data->count - pos));
          memmove(&lengths[pos],
                  &lengths[pos + 1],
                  sizeof(lengths[pos]) * (hash_data->count - pos));

          hash_data->count--;
          hash_data->over_provision++;
          hash_data->operations++;
        }

      return SVN_NO_ERROR;
    }

  /* if not found, prepare to insert the new entry */
  if (!found)
    {
      /* fallback to slow operation if there is no place left to insert an
       * new entry to index. That will automatically give add some spare
       * entries ("overprovision"). */
      if (hash_data->over_provision == 0)
        return slowly_replace_dir_entry(data, data_len, baton, pool);

      /* make entries[index] available for pointing to the new entry */
      memmove(&entries[pos + 1],
              &entries[pos],
              sizeof(entries[pos]) * (hash_data->count - pos));
      memmove(&lengths[pos + 1],
              &lengths[pos],
              sizeof(lengths[pos]) * (hash_data->count - pos));

      hash_data->count++;
      hash_data->over_provision--;
      hash_data->operations++;
    }

  /* de-serialize the new entry */
  entries[pos] = replace_baton->new_entry;
  context = svn_temp_serializer__init_append(hash_data,
                                             entries,
                                             hash_data->len,
                                             *data_len,
                                             pool);
  serialize_dir_entry(context, &entries[pos], &length);

  /* return the updated serialized data */
  SVN_ERR (return_serialized_dir_context(context,
                                         data,
                                         data_len));

  /* since the previous call may have re-allocated the buffer, the lengths
   * pointer may no longer point to the entry in that buffer. Therefore,
   * re-map it again and store the length value after that. */

  hash_data = (hash_data_t *)*data;
  lengths = (apr_uint32_t *)
    svn_temp_deserializer__ptr((const char *)hash_data,
                               (const void **)&hash_data->lengths);
  lengths[pos] = length;

  return SVN_NO_ERROR;
}
