/**
 * @copyright
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
 * @endcopyright
 *
 * @file svn_sorts.h
 * @brief all sorts of sorts.
 */


#ifndef SVN_SORTS_H
#define SVN_SORTS_H

#include <apr.h>         /* for apr_ssize_t */
#include <apr_pools.h>   /* for apr_pool_t */
#include <apr_tables.h>  /* for apr_array_header_t */
#include <apr_hash.h>    /* for apr_hash_t */

/* Define a MAX macro if we don't already have one */
#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif

/* Define a MIN macro if we don't already have one */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/** This structure is used to hold a key/value from a hash table.
 * @note Private. For use by Subversion's own code only. See issue #1644.
 */
typedef struct svn_sort__item_t {
  /** pointer to the key */
  const void *key;

  /** size of the key */
  apr_ssize_t klen;

  /** pointer to the value */
  void *value;
} svn_sort__item_t;


/** Compare two @c svn_sort__item_t's, returning an integer greater than,
 * equal to, or less than 0, according to whether the key of @a a is
 * greater than, equal to, or less than the key of @a b as determined
 * by comparing them with svn_path_compare_paths().
 *
 * The key strings must be NULL-terminated, even though klen does not
 * include the terminator.
 *
 * This is useful for converting a hash into a sorted
 * @c apr_array_header_t.  For example, to convert hash @a hsh to a sorted
 * array, do this:
 *
 * @code
     apr_array_header_t *array;
     array = svn_sort__hash(hsh, svn_sort_compare_items_as_paths, pool);
   @endcode
 */
int
svn_sort_compare_items_as_paths(const svn_sort__item_t *a,
                                const svn_sort__item_t *b);


/** Compare two @c svn_sort__item_t's, returning an integer greater than,
 * equal to, or less than 0, according as @a a is greater than, equal to,
 * or less than @a b according to a lexical key comparison.  The keys are
 * not required to be zero-terminated.
 */
int
svn_sort_compare_items_lexically(const svn_sort__item_t *a,
                                 const svn_sort__item_t *b);

/** Compare two @c svn_revnum_t's, returning an integer greater than, equal
 * to, or less than 0, according as @a b is greater than, equal to, or less
 * than @a a. Note that this sorts newest revision to oldest (IOW, descending
 * order).
 *
 * This function is compatible for use with qsort().
 *
 * This is useful for converting an array of revisions into a sorted
 * @c apr_array_header_t. You are responsible for detecting, preventing or
 * removing duplicates.
 */
int
svn_sort_compare_revisions(const void *a,
                           const void *b);


/**
 * Compare two @c const char * paths, returning an integer greater
 * than, equal to, or less than 0, using the same comparison rules as
 * are used by svn_path_compare_paths().
 *
 * This function is compatible for use with qsort().
 *
 * @since New in 1.1.
 */
int
svn_sort_compare_paths(const void *a,
                       const void *b);

/**
 * Compare two @c svn_merge_range_t *'s, returning an integer greater
 * than, equal to, or less than 0 if the first range is greater than,
 * equal to, or less than, the second range.
 *
 * Both @c svn_merge_range_t *'s must describe forward merge ranges.
 *
 * If @a a and @a b intersect then the range with the lower start revision
 * is considered the lesser range.  If the ranges' start revisions are
 * equal then the range with the lower end revision is considered the
 * lesser range.
 *
 * @since New in 1.5
 */
int
svn_sort_compare_ranges(const void *a,
                        const void *b);

/** Sort @a ht according to its keys, return an @c apr_array_header_t
 * containing @c svn_sort__item_t structures holding those keys and values
 * (i.e. for each @c svn_sort__item_t @a item in the returned array,
 * @a item->key and @a item->size are the hash key, and @a item->data points to
 * the hash value).
 *
 * Storage is shared with the original hash, not copied.
 *
 * @a comparison_func should take two @c svn_sort__item_t's and return an
 * integer greater than, equal to, or less than 0, according as the first item
 * is greater than, equal to, or less than the second.
 *
 * @note Private. For use by Subversion's own code only. See issue #1644.
 *
 * @note This function and the @c svn_sort__item_t should go over to APR.
 */
apr_array_header_t *
svn_sort__hash(apr_hash_t *ht,
               int (*comparison_func)(const svn_sort__item_t *,
                                      const svn_sort__item_t *),
               apr_pool_t *pool);

/* Return the lowest index at which the element *KEY should be inserted into
   the array ARRAY, according to the ordering defined by COMPARE_FUNC.
   The array must already be sorted in the ordering defined by COMPARE_FUNC.
   COMPARE_FUNC is defined as for the C stdlib function bsearch(). */
int
svn_sort__bsearch_lower_bound(const void *key,
                              const apr_array_header_t *array,
                              int (*compare_func)(const void *, const void *));

/* Insert a shallow copy of *NEW_ELEMENT into the array ARRAY at the index
   INSERT_INDEX, growing the array and shuffling existing elements along to
   make room. */
void
svn_sort__array_insert(const void *new_element,
                       apr_array_header_t *array,
                       int insert_index);


/* Remove ELEMENTS_TO_DELETE elements starting  at DELETE_INDEX from the
   array ARR. If DELETE_INDEX is not a valid element of ARR,
   ELEMENTS_TO_DELETE is not greater than zero, or
   DELETE_INDEX + ELEMENTS_TO_DELETE is greater than ARR->NELTS, then do
   nothing. */
void
svn_sort__array_delete(apr_array_header_t *arr,
                       int delete_index,
                       int elements_to_delete);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_SORTS_H */
