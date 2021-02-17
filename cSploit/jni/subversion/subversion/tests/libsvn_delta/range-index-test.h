/*
 * range-index-test.c: An extension for random-test.
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

#ifndef SVN_RANGE_INDEX_TEST_H
#define SVN_RANGE_INDEX_TEST_H

#include "../../libsvn_delta/compose_delta.c"

static range_index_node_t *prev_node, *prev_prev_node;
static apr_off_t
walk_range_index(range_index_node_t *node, const char **msg)
{
  apr_off_t ret;

  if (node == NULL)
    return 0;

  ret = walk_range_index(node->left, msg);
  if (ret > 0)
    return ret;

  if (prev_node != NULL
      && node->target_offset > 0
      && (prev_node->offset >= node->offset
          || (prev_node->limit >= node->limit)))
    {
      ret = node->target_offset;
      node->target_offset = -node->target_offset;
      *msg = "Oops, the previous node ate me.";
      return ret;
    }
  if (prev_prev_node != NULL
      && prev_node->target_offset > 0
      && prev_prev_node->limit > node->offset)
    {
      ret = prev_node->target_offset;
      prev_node->target_offset = -prev_node->target_offset;
      *msg = "Arrgh, my neighbours are conspiring against me.";
      return ret;
    }
  prev_prev_node = prev_node;
  prev_node = node;

  return walk_range_index(node->right, msg);
}


static void
print_node_data(range_index_node_t *node, const char *msg, apr_off_t ndx)
{
  if (-node->target_offset == ndx)
    {
      printf("   * Node: [%3"APR_OFF_T_FMT
             ",%3"APR_OFF_T_FMT
             ") = %-5"APR_OFF_T_FMT"%s\n",
             node->offset, node->limit, -node->target_offset, msg);
    }
  else
    {
      printf("     Node: [%3"APR_OFF_T_FMT
             ",%3"APR_OFF_T_FMT
             ") = %"APR_OFF_T_FMT"\n",
             node->offset, node->limit,
             (node->target_offset < 0
              ? -node->target_offset : node->target_offset));
    }
}

static void
print_range_index_r(range_index_node_t *node, const char *msg, apr_off_t ndx)
{
  if (node == NULL)
    return;

  print_range_index_r(node->left, msg, ndx);
  print_node_data(node, msg, ndx);
  print_range_index_r(node->right, msg, ndx);
}

static void
print_range_index_i(range_index_node_t *node, const char *msg, apr_off_t ndx)
{
  if (node == NULL)
    return;

  while (node->prev)
    node = node->prev;

  do
    {
      print_node_data(node, msg, ndx);
      node = node->next;
    }
  while (node);
}

static void
print_range_index(range_index_node_t *node, const char *msg, apr_off_t ndx)
{
  printf("  (recursive)\n");
  print_range_index_r(node, msg, ndx);
  printf("  (iterative)\n");
  print_range_index_i(node, msg, ndx);
}


static void
check_copy_count(int src_cp, int tgt_cp)
{
  printf("Source copies: %d  Target copies: %d\n", src_cp, tgt_cp);
  if (src_cp > tgt_cp)
    printf("WARN: More source than target copies; inefficient combiner?\n");
}


static svn_error_t *
random_range_index_test(apr_pool_t *pool)
{
  unsigned long seed, bytes_range;
  int i, maxlen, iterations, dump_files, print_windows;
  const char *random_bytes;
  range_index_t *ndx;
  int tgt_cp = 0, src_cp = 0;

  /* Initialize parameters and print out the seed in case we dump core
     or something. */
  init_params(&seed, &maxlen, &iterations, &dump_files, &print_windows,
              &random_bytes, &bytes_range, pool);

  /* ### This test is expected to fail randomly at the moment, so don't
     enable it by default. --xbc */

  ndx = create_range_index(pool);
  for (i = 1; i <= iterations; ++i)
    {
      apr_off_t offset = myrand(&seed) % 47;
      apr_off_t limit = offset + myrand(&seed) % 16 + 1;
      range_list_node_t *list, *r;
      apr_off_t ret;
      const char *msg2;

      printf("%3d: Inserting [%3"APR_OFF_T_FMT",%3"APR_OFF_T_FMT") ...",
             i, offset, limit);
      splay_range_index(offset, ndx);
      list = build_range_list(offset, limit, ndx);
      insert_range(offset, limit, i, ndx);
      prev_prev_node = prev_node = NULL;
      ret = walk_range_index(ndx->tree, &msg2);
      if (ret == 0)
        {
          for (r = list; r; r = r->next)
            printf(" %s[%3"APR_OFF_T_FMT",%3"APR_OFF_T_FMT")",
                   (r->kind == range_from_source ?
                    (++src_cp, "S") : (++tgt_cp, "T")),
                   r->offset, r->limit);
          free_range_list(list, ndx);
          printf(" OK\n");
        }
      else
        {
          printf(" Ooops!\n");
          print_range_index(ndx->tree, msg2, ret);
          check_copy_count(src_cp, tgt_cp);
          return svn_error_create(SVN_ERR_TEST_FAILED, 0, NULL, pool,
                                  "insert_range");
        }
    }

  printf("Final tree state:\n");
  print_range_index(ndx->tree, "", iterations + 1);
  check_copy_count(src_cp, tgt_cp);
  return SVN_NO_ERROR;
}


#endif /* SVN_RANGE_INDEX_TEST_H */
