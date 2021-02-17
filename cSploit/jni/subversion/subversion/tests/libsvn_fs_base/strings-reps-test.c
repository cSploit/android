/* strings-reps-test.c --- test `strings' and `representations' interfaces
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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <apr.h>

#include "svn_error.h"
#include "private/svn_skel.h"

#include "../svn_test.h"
#include "../svn_test_fs.h"
#include "../../libsvn_fs/fs-loader.h"
#include "../../libsvn_fs_base/fs.h"
#include "../../libsvn_fs_base/util/fs_skels.h"
#include "../../libsvn_fs_base/bdb/strings-table.h"
#include "../../libsvn_fs_base/bdb/reps-table.h"



/*-----------------------------------------------------------------*/
/* Helper functions and batons for reps-table testing. */
struct rep_args
{
  const char *key;
  svn_fs_t *fs;
  svn_skel_t *skel;
};


static svn_error_t *
txn_body_write_new_rep(void *baton, trail_t *trail)
{
  struct rep_args *b = (struct rep_args *) baton;
  representation_t *rep;
  SVN_ERR(svn_fs_base__parse_representation_skel(&rep, b->skel,
                                                 trail->pool));
  return svn_fs_bdb__write_new_rep(&(b->key), b->fs, rep, trail, trail->pool);
}


static svn_error_t *
txn_body_write_rep(void *baton, trail_t *trail)
{
  struct rep_args *b = (struct rep_args *) baton;
  representation_t *rep;
  SVN_ERR(svn_fs_base__parse_representation_skel(&rep, b->skel,
                                                 trail->pool));
  return svn_fs_bdb__write_rep(b->fs, b->key, rep, trail, trail->pool);
}


static svn_error_t *
txn_body_read_rep(void *baton, trail_t *trail)
{
  struct rep_args *b = (struct rep_args *) baton;
  representation_t *rep;
  base_fs_data_t *bfd = b->fs->fsap_data;
  SVN_ERR(svn_fs_bdb__read_rep(&rep, b->fs, b->key, trail, trail->pool));
  return svn_fs_base__unparse_representation_skel(&(b->skel), rep,
                                                  bfd->format, trail->pool);
}


static svn_error_t *
txn_body_delete_rep(void *baton, trail_t *trail)
{
  struct rep_args *b = (struct rep_args *) baton;
  return svn_fs_bdb__delete_rep(b->fs, b->key, trail, trail->pool);
}



/* Representation Table Test functions. */

static svn_error_t *
write_new_rep(const svn_test_opts_t *opts,
              apr_pool_t *pool)
{
  struct rep_args args;
  const char *rep = "((fulltext 0 ) a83t2Z0q)";
  svn_fs_t *fs;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-write-new-rep", opts,
           pool));

  /* Set up transaction baton */
  args.fs = fs;
  args.skel = svn_skel__parse(rep, strlen(rep), pool);
  args.key = NULL;

  /* Write new rep to reps table. */
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_write_new_rep, &args,
                                 FALSE, pool));

  if (args.key == NULL)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "error writing new representation");

  return SVN_NO_ERROR;
}


static svn_error_t *
write_rep(const svn_test_opts_t *opts,
          apr_pool_t *pool)
{
  struct rep_args new_args;
  struct rep_args args;
  const char *new_rep = "((fulltext 0 ) a83t2Z0q)";
  const char *rep = "((fulltext 0 ) kfogel31337)";
  svn_fs_t *fs;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-write-rep", opts,
           pool));

  /* Set up transaction baton */
  new_args.fs = fs;
  new_args.skel = svn_skel__parse(new_rep, strlen(new_rep), pool);
  new_args.key = NULL;

  /* Write new rep to reps table. */
  SVN_ERR(svn_fs_base__retry_txn(new_args.fs, txn_body_write_new_rep,
                                 &new_args, FALSE, pool));

  /* Make sure we got a valid key. */
  if (new_args.key == NULL)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "error writing new representation");

  /* Set up transaction baton for re-writing reps. */
  args.fs = new_args.fs;
  args.skel = svn_skel__parse(rep, strlen(rep), pool);
  args.key = new_args.key;

  /* Overwrite first rep in reps table. */
  SVN_ERR(svn_fs_base__retry_txn(new_args.fs, txn_body_write_rep, &args,
                                 FALSE, pool));

  return SVN_NO_ERROR;
}


static svn_error_t *
read_rep(const svn_test_opts_t *opts,
         apr_pool_t *pool)
{
  struct rep_args new_args;
  struct rep_args args;
  struct rep_args read_args;
  svn_stringbuf_t *skel_data;
  svn_fs_t *fs;

  const char *rep = "((fulltext 0 ) kfogel31337)";
  const char *new_rep_before = "((fulltext 0 ) a83t2Z0)";

  /* This test also tests the introduction of checksums into skels that
     didn't have them. */

  /* Get writeable strings. */
  char *rep_after = apr_pstrdup
    (pool, "((fulltext 0  (md5 16 XXXXXXXXXXXXXXXX)) kfogel31337");
  char *new_rep_after = apr_pstrdup
    (pool, "((fulltext 0  (md5 16 XXXXXXXXXXXXXXXX)) a83t2Z0");
  size_t rep_after_len = strlen(rep_after);
  size_t new_rep_after_len = strlen(new_rep_after);

  /* Replace the fake fake checksums with the real fake checksums.
     And someday, when checksums are actually calculated, we can
     replace the real fake checksums with real real checksums. */
  {
    char *p;

    for (p = rep_after; *p; p++)
      if (*p == 'X')
        *p = '\0';

    for (p = new_rep_after; *p; p++)
      if (*p == 'X')
        *p = '\0';
  }

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-read-rep", opts,
           pool));

  /* Set up transaction baton */
  new_args.fs = fs;
  new_args.skel = svn_skel__parse(new_rep_before, strlen(new_rep_before),
                                  pool);
  new_args.key = NULL;

  /* Write new rep to reps table. */
  SVN_ERR(svn_fs_base__retry_txn(new_args.fs, txn_body_write_new_rep,
                                 &new_args, FALSE, pool));

  /* Make sure we got a valid key. */
  if (new_args.key == NULL)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "error writing new representation");

  /* Read the new rep back from the reps table. */
  read_args.fs = new_args.fs;
  read_args.skel = NULL;
  read_args.key = new_args.key;
  SVN_ERR(svn_fs_base__retry_txn(new_args.fs, txn_body_read_rep, &read_args,
                                 FALSE, pool));

  /* Make sure the skel matches. */
  if (! read_args.skel)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "error reading new representation");

  skel_data = svn_skel__unparse(read_args.skel, pool);
  if (memcmp(skel_data->data, new_rep_after, new_rep_after_len) != 0)
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "representation corrupted (first check)");

  /* Set up transaction baton for re-writing reps. */
  args.fs = new_args.fs;
  args.skel = svn_skel__parse(rep, strlen(rep), pool);
  args.key = new_args.key;

  /* Overwrite first rep in reps table. */
  SVN_ERR(svn_fs_base__retry_txn(new_args.fs, txn_body_write_rep, &args,
                                 FALSE, pool));

  /* Read the new rep back from the reps table (using the same FS and
     key as the first read...let's make sure this thing didn't get
     written to the wrong place). */
  read_args.skel = NULL;
  SVN_ERR(svn_fs_base__retry_txn(new_args.fs, txn_body_read_rep, &read_args,
                                 FALSE, pool));

  /* Make sure the skel matches. */
  if (! read_args.skel)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "error reading new representation");

  skel_data = svn_skel__unparse(read_args.skel, pool);
  if (memcmp(skel_data->data, rep_after, rep_after_len) != 0)
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "representation corrupted (second check)");

  return SVN_NO_ERROR;
}


static svn_error_t *
delete_rep(const svn_test_opts_t *opts,
           apr_pool_t *pool)
{
  struct rep_args new_args;
  struct rep_args delete_args;
  struct rep_args read_args;
  const char *new_rep = "((fulltext 0 ) a83t2Z0q)";
  svn_fs_t *fs;
  svn_error_t *err;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-delete-rep", opts,
           pool));

  /* Set up transaction baton */
  new_args.fs = fs;
  new_args.skel = svn_skel__parse(new_rep, strlen(new_rep), pool);
  new_args.key = NULL;

  /* Write new rep to reps table. */
  SVN_ERR(svn_fs_base__retry_txn(new_args.fs, txn_body_write_new_rep,
                                 &new_args, FALSE, pool));

  /* Make sure we got a valid key. */
  if (new_args.key == NULL)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "error writing new representation");

  /* Delete the rep we just wrote. */
  delete_args.fs = new_args.fs;
  delete_args.key = new_args.key;
  SVN_ERR(svn_fs_base__retry_txn(new_args.fs, txn_body_delete_rep,
                                 &delete_args, FALSE, pool));

  /* Try to read the new rep back from the reps table. */
  read_args.fs = new_args.fs;
  read_args.skel = NULL;
  read_args.key = new_args.key;
  err = svn_fs_base__retry_txn(new_args.fs, txn_body_read_rep, &read_args,
                               FALSE, pool);

  /* We better have an error... */
  if ((! err) && (read_args.skel))
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "error deleting representation");
  svn_error_clear(err);

  return SVN_NO_ERROR;
}


/* ------------------------------------------------------------------- */
/* Helper functions and batons for strings-table testing. */

static svn_error_t *
verify_expected_record(svn_fs_t *fs,
                       const char *key,
                       const char *expected_text,
                       apr_size_t expected_len,
                       trail_t *trail)
{
  apr_size_t size;
  char buf[100];
  svn_stringbuf_t *text;
  svn_filesize_t offset = 0;
  svn_filesize_t string_size;

  /* Check the string size. */
  SVN_ERR(svn_fs_bdb__string_size(&string_size, fs, key,
                                  trail, trail->pool));
  if (string_size > SVN_MAX_OBJECT_SIZE)
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "record size is too large "
                             "(got %" SVN_FILESIZE_T_FMT ", "
                             "limit is %" APR_SIZE_T_FMT ")",
                             string_size, SVN_MAX_OBJECT_SIZE);
  size = (apr_size_t) string_size;
  if (size != expected_len)
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "record has unexpected size "
                             "(got %" APR_SIZE_T_FMT ", "
                             "expected %" APR_SIZE_T_FMT ")",
                             size, expected_len);

  /* Read the string back in 100-byte chunks. */
  text = svn_stringbuf_create("", trail->pool);
  while (1)
    {
      size = sizeof(buf);
      SVN_ERR(svn_fs_bdb__string_read(fs, key, buf, offset, &size,
                                      trail, trail->pool));
      if (size == 0)
        break;
      svn_stringbuf_appendbytes(text, buf, size);
      offset += size;
    }

  /* Check the size and contents of the read data. */
  if (text->len != expected_len)
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "record read returned unexpected size "
                             "(got %" APR_SIZE_T_FMT ", "
                             "expected %" APR_SIZE_T_FMT ")",
                             size, expected_len);
  if (memcmp(expected_text, text->data, expected_len))
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "record read returned unexpected data");

  return SVN_NO_ERROR;
}


struct string_args
{
  svn_fs_t *fs;
  const char *key;
  const char *text;
  apr_size_t len;
};


static svn_error_t *
txn_body_verify_string(void *baton, trail_t *trail)
{
  struct string_args *b = (struct string_args *) baton;
  return verify_expected_record(b->fs, b->key, b->text, b->len, trail);
}


static svn_error_t *
txn_body_string_append(void *baton, trail_t *trail)
{
  struct string_args *b = (struct string_args *) baton;
  return svn_fs_bdb__string_append(b->fs, &(b->key), b->len,
                                   b->text, trail, trail->pool);
}


static svn_error_t *
txn_body_string_clear(void *baton, trail_t *trail)
{
  struct string_args *b = (struct string_args *) baton;
  return svn_fs_bdb__string_clear(b->fs, b->key, trail, trail->pool);
}


static svn_error_t *
txn_body_string_delete(void *baton, trail_t *trail)
{
  struct string_args *b = (struct string_args *) baton;
  return svn_fs_bdb__string_delete(b->fs, b->key, trail, trail->pool);
}


static svn_error_t *
txn_body_string_size(void *baton, trail_t *trail)
{
  struct string_args *b = (struct string_args *) baton;
  svn_filesize_t string_size;
  SVN_ERR(svn_fs_bdb__string_size(&string_size, b->fs, b->key,
                                  trail, trail->pool));
  if (string_size > SVN_MAX_OBJECT_SIZE)
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "txn_body_string_size: string size is too large "
       "(got %" SVN_FILESIZE_T_FMT ", limit is %" APR_SIZE_T_FMT ")",
       string_size, SVN_MAX_OBJECT_SIZE);
  b->len = (apr_size_t) string_size;
  return SVN_NO_ERROR;
}


static svn_error_t *
txn_body_string_append_fail(void *baton, trail_t *trail)
{
  struct string_args *b = (struct string_args *) baton;
  SVN_ERR(svn_fs_bdb__string_append(b->fs, &(b->key), b->len,
                                    b->text, trail, trail->pool));
  return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                          "la dee dah, la dee day...");
}

static svn_error_t *
txn_body_string_copy(void *baton, trail_t *trail)
{
  struct string_args *b = (struct string_args *) baton;
  return svn_fs_bdb__string_copy(b->fs, &(b->key), b->key,
                                 trail, trail->pool);
}


static const char *bigstring1 =
"    Alice opened the door and found that it led into a small\n"
"passage, not much larger than a rat-hole:  she knelt down and\n"
"looked along the passage into the loveliest garden you ever saw.\n"
"How she longed to get out of that dark hall, and wander about\n"
"among those beds of bright flowers and those cool fountains, but\n"
"she could not even get her head though the doorway; 'and even if\n"
"my head would go through,' thought poor Alice, 'it would be of\n"
"very little use without my shoulders.  Oh, how I wish\n"
"I could shut up like a telescope!  I think I could, if I only\n"
"know how to begin.'  For, you see, so many out-of-the-way things\n"
"had happened lately, that Alice had begun to think that very few\n"
"things indeed were really impossible.";

static const char *bigstring2 =
"    There seemed to be no use in waiting by the little door, so she\n"
"went back to the table, half hoping she might find another key on\n"
"it, or at any rate a book of rules for shutting people up like\n"
"telescopes:  this time she found a little bottle on it, ('which\n"
"certainly was not here before,' said Alice,) and round the neck\n"
"of the bottle was a paper label, with the words 'DRINK ME'\n"
"beautifully printed on it in large letters.";

static const char *bigstring3 =
"    It was all very well to say 'Drink me,' but the wise little\n"
"Alice was not going to do THAT in a hurry.  'No, I'll look\n"
"first,' she said, 'and see whether it's marked \"poison\" or not';\n"
"for she had read several nice little histories about children who\n"
"had got burnt, and eaten up by wild beasts and other unpleasant\n"
"things, all because they WOULD not remember the simple rules\n"
"their friends had taught them:  such as, that a red-hot poker\n"
"will burn you if you hold it too long; and that if you cut your\n"
"finger VERY deeply with a knife, it usually bleeds; and she had\n"
"never forgotten that, if you drink much from a bottle marked\n"
"'poison,' it is almost certain to disagree with you, sooner or\n"
"later.";


static svn_error_t *
test_strings(const svn_test_opts_t *opts,
             apr_pool_t *pool)
{
  struct string_args args;
  svn_fs_t *fs;
  svn_stringbuf_t *string;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-test-strings", opts,
           pool));

  /* The plan (after each step below, verify the size and contents of
     the string):

     1.  Write a new string (string1).
     2.  Append string2 to string.
     3.  Clear string.
     4.  Append string3 to string.
     5.  Delete string (verify by size requested failure).
     6.  Write a new string (string1), appending string2, string3, and
         string4.
  */

  /* 1. Write a new string (string1). */
  args.fs = fs;
  args.key = NULL;
  args.text = bigstring1;
  args.len = strlen(bigstring1);
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_string_append, &args,
                                 FALSE, pool));

  /* Make sure a key was returned. */
  if (! args.key)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "write of new string failed to return new key");

  /* Verify record's size and contents. */
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_verify_string, &args,
                                 FALSE, pool));

  /* Append a second string to our first one. */
  args.text = bigstring2;
  args.len = strlen(bigstring2);
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_string_append, &args,
                                 FALSE, pool));

  /* Verify record's size and contents. */
  string = svn_stringbuf_create(bigstring1, pool);
  svn_stringbuf_appendcstr(string, bigstring2);
  args.text = string->data;
  args.len = string->len;
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_verify_string, &args,
                                 FALSE, pool));

  /* Clear the record */
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_string_clear, &args,
                                 FALSE, pool));

  /* Verify record's size and contents. */
  args.text = "";
  args.len = 0;
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_verify_string, &args,
                                 FALSE, pool));

  /* Append a third string to our first one. */
  args.text = bigstring3;
  args.len = strlen(bigstring3);
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_string_append, &args,
                                 FALSE, pool));

  /* Verify record's size and contents. */
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_verify_string, &args,
                                 FALSE, pool));

  /* Delete our record...she's served us well. */
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_string_delete, &args,
                                 FALSE, pool));

  /* Now, we expect a size request on this record to fail with
     SVN_ERR_FS_NO_SUCH_STRING. */
  {
    svn_error_t *err = svn_fs_base__retry_txn(args.fs, txn_body_string_size,
                                              &args, FALSE, pool);

    if (! err)
      return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                              "query unexpectedly successful");
    if (err->apr_err != SVN_ERR_FS_NO_SUCH_STRING)
      return svn_error_create(SVN_ERR_FS_GENERAL, err,
                              "query failed with unexpected error");
    svn_error_clear(err);
  }

  return SVN_NO_ERROR;
}


static svn_error_t *
write_null_string(const svn_test_opts_t *opts,
                  apr_pool_t *pool)
{
  struct string_args args;
  svn_fs_t *fs;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-test-strings", opts,
           pool));

  args.fs = fs;
  args.key = NULL;
  args.text = NULL;
  args.len = 0;
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_string_append, &args,
                                 FALSE, pool));

  return SVN_NO_ERROR;
}


static svn_error_t *
abort_string(const svn_test_opts_t *opts,
             apr_pool_t *pool)
{
  struct string_args args, args2;
  svn_fs_t *fs;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-abort-string", opts,
           pool));

  /* The plan:

     1.  Write a new string (string1).
     2.  Overwrite string1 with string2, but then ABORT the transaction.
     3.  Read string to make sure it is still string1.
  */

  /* 1. Write a new string (string1). */
  args.fs = fs;
  args.key = NULL;
  args.text = bigstring1;
  args.len = strlen(bigstring1);
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_string_append, &args,
                                 FALSE, pool));

  /* Make sure a key was returned. */
  if (! args.key)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "write of new string failed to return new key");

  /* Verify record's size and contents. */
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_verify_string, &args,
                                 FALSE, pool));

  /* Append a second string to our first one. */
  args2.fs = fs;
  args2.key = args.key;
  args2.text = bigstring2;
  args2.len = strlen(bigstring2);
  {
    svn_error_t *err;

    /* This function is *supposed* to fail with SVN_ERR_TEST_FAILED */
    err = svn_fs_base__retry_txn(args.fs, txn_body_string_append_fail,
                                 &args2, FALSE, pool);
    if ((! err) || (err->apr_err != SVN_ERR_TEST_FAILED))
      return svn_error_create(SVN_ERR_TEST_FAILED, err,
                              "failed to intentionally abort a trail");
    svn_error_clear(err);
  }

  /* Verify that record's size and contents are still that of string1 */
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_verify_string, &args,
                                 FALSE, pool));

  return SVN_NO_ERROR;
}

static svn_error_t *
copy_string(const svn_test_opts_t *opts,
            apr_pool_t *pool)
{
  struct string_args args;
  svn_fs_t *fs;
  const char *old_key;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-copy-string", opts,
           pool));

  /*  Write a new string (string1). */
  args.fs = fs;
  args.key = NULL;
  args.text = bigstring1;
  args.len = strlen(bigstring1);
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_string_append, &args,
                                 FALSE, pool));

  /* Make sure a key was returned. */
  if (! (old_key = args.key))
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "write of new string failed to return new key");

  /* Now copy that string into a new location. */
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_string_copy, &args,
                                 FALSE, pool));

  /* Make sure a different key was returned. */
  if ((! args.key) || (! strcmp(old_key, args.key)))
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "copy of string failed to return new key");

  /* Verify record's size and contents. */
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_verify_string, &args,
                                 FALSE, pool));

  return SVN_NO_ERROR;
}



/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_OPTS_PASS(write_new_rep,
                       "write a new rep, get a new key back"),
    SVN_TEST_OPTS_PASS(write_rep,
                       "write a new rep, then overwrite it"),
    SVN_TEST_OPTS_PASS(read_rep,
                       "write and overwrite a new rep; confirm with reads"),
    SVN_TEST_OPTS_PASS(delete_rep,
                       "write, then delete, a new rep; confirm deletion"),
    SVN_TEST_OPTS_PASS(test_strings,
                       "test many strings table functions together"),
    SVN_TEST_OPTS_PASS(write_null_string,
                       "write a null string"),
    SVN_TEST_OPTS_PASS(abort_string,
                       "write a string, then abort during an overwrite"),
    SVN_TEST_OPTS_PASS(copy_string,
                       "create and copy a string"),
    SVN_TEST_NULL
  };
