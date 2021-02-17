/*
 * main.c: Subversion server inspection tool.
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
#include <stdlib.h>

#include <apr_general.h>
#include <apr_pools.h>
#include <apr_time.h>
#include <apr_file_io.h>
#include <apr_signal.h>

#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>

#include "svn_cmdline.h"
#include "svn_types.h"
#include "svn_pools.h"
#include "svn_error.h"
#include "svn_error_codes.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_repos.h"
#include "svn_fs.h"
#include "svn_time.h"
#include "svn_utf.h"
#include "svn_subst.h"
#include "svn_opt.h"
#include "svn_props.h"
#include "svn_diff.h"
#include "svn_version.h"
#include "svn_xml.h"

#include "private/svn_cmdline_private.h"
#include "private/svn_fspath.h"

#include "svn_private_config.h"


/*** Some convenience macros and types. ***/


/* Option handling. */

static svn_opt_subcommand_t
  subcommand_author,
  subcommand_cat,
  subcommand_changed,
  subcommand_date,
  subcommand_diff,
  subcommand_dirschanged,
  subcommand_filesize,
  subcommand_help,
  subcommand_history,
  subcommand_info,
  subcommand_lock,
  subcommand_log,
  subcommand_pget,
  subcommand_plist,
  subcommand_tree,
  subcommand_uuid,
  subcommand_youngest;

/* Option codes and descriptions. */
enum
  {
    svnlook__version = SVN_OPT_FIRST_LONGOPT_ID,
    svnlook__show_ids,
    svnlook__no_diff_deleted,
    svnlook__no_diff_added,
    svnlook__diff_copy_from,
    svnlook__revprop_opt,
    svnlook__full_paths,
    svnlook__copy_info,
    svnlook__xml_opt
  };

/*
 * The entire list must be terminated with an entry of nulls.
 */
static const apr_getopt_option_t options_table[] =
{
  {NULL,                '?', 0,
   N_("show help on a subcommand")},

  {"copy-info",         svnlook__copy_info, 0,
   N_("show details for copies")},

  {"diff-copy-from",    svnlook__diff_copy_from, 0,
   N_("print differences against the copy source")},

  {"full-paths",        svnlook__full_paths, 0,
   N_("show full paths instead of indenting them")},

  {"help",              'h', 0,
   N_("show help on a subcommand")},

  {"limit",             'l', 1,
   N_("maximum number of history entries")},

  {"no-diff-added",     svnlook__no_diff_added, 0,
   N_("do not print differences for added files")},

  {"no-diff-deleted",   svnlook__no_diff_deleted, 0,
   N_("do not print differences for deleted files")},

  {"non-recursive",     'N', 0,
   N_("operate on single directory only")},

  {"revision",          'r', 1,
   N_("specify revision number ARG")},

  {"revprop",           svnlook__revprop_opt, 0,
   N_("operate on a revision property (use with -r or -t)")},

  {"show-ids",          svnlook__show_ids, 0,
   N_("show node revision ids for each path")},

  {"transaction",       't', 1,
   N_("specify transaction name ARG")},

  {"verbose",           'v', 0,
   N_("be verbose")},

  {"version",           svnlook__version, 0,
   N_("show program version information")},

  {"xml",               svnlook__xml_opt, 0,
   N_("output in XML")},

  {"extensions",        'x', 1,
   N_("Default: '-u'. When Subversion is invoking an\n"
      "                            "
      " external diff program, ARG is simply passed along\n"
      "                            "
      " to the program. But when Subversion is using its\n"
      "                            "
      " default internal diff implementation, or when\n"
      "                            "
      " Subversion is displaying blame annotations, ARG\n"
      "                            "
      " could be any of the following:\n"
      "                            "
      "    -u (--unified):\n"
      "                            "
      "       Output 3 lines of unified context.\n"
      "                            "
      "    -b (--ignore-space-change):\n"
      "                            "
      "       Ignore changes in the amount of white space.\n"
      "                            "
      "    -w (--ignore-all-space):\n"
      "                            "
      "       Ignore all white space.\n"
      "                            "
      "    --ignore-eol-style:\n"
      "                            "
      "       Ignore changes in EOL style\n"
      "                            "
      "    -p (--show-c-function):\n"
      "                            "
      "       Show C function name in diff output.")},

  {"quiet",             'q', 0,
   N_("no progress (only errors) to stderr")},

  {0,                   0, 0, 0}
};


/* Array of available subcommands.
 * The entire list must be terminated with an entry of nulls.
 */
static const svn_opt_subcommand_desc2_t cmd_table[] =
{
  {"author", subcommand_author, {0},
   N_("usage: svnlook author REPOS_PATH\n\n"
      "Print the author.\n"),
   {'r', 't'} },

  {"cat", subcommand_cat, {0},
   N_("usage: svnlook cat REPOS_PATH FILE_PATH\n\n"
      "Print the contents of a file.  Leading '/' on FILE_PATH is optional.\n"),
   {'r', 't'} },

  {"changed", subcommand_changed, {0},
   N_("usage: svnlook changed REPOS_PATH\n\n"
      "Print the paths that were changed.\n"),
   {'r', 't', svnlook__copy_info} },

  {"date", subcommand_date, {0},
   N_("usage: svnlook date REPOS_PATH\n\n"
      "Print the datestamp.\n"),
   {'r', 't'} },

  {"diff", subcommand_diff, {0},
   N_("usage: svnlook diff REPOS_PATH\n\n"
      "Print GNU-style diffs of changed files and properties.\n"),
   {'r', 't', svnlook__no_diff_deleted, svnlook__no_diff_added,
    svnlook__diff_copy_from, 'x'} },

  {"dirs-changed", subcommand_dirschanged, {0},
   N_("usage: svnlook dirs-changed REPOS_PATH\n\n"
      "Print the directories that were themselves changed (property edits)\n"
      "or whose file children were changed.\n"),
   {'r', 't'} },

  {"filesize", subcommand_filesize, {0},
   N_("usage: svnlook filesize REPOS_PATH PATH_IN_REPOS\n\n"
      "Print the size (in bytes) of the file located at PATH_IN_REPOS as\n"
      "it is represented in the repository.\n"),
   {'r', 't'} },

  {"help", subcommand_help, {"?", "h"},
   N_("usage: svnlook help [SUBCOMMAND...]\n\n"
      "Describe the usage of this program or its subcommands.\n"),
   {0} },

  {"history", subcommand_history, {0},
   N_("usage: svnlook history REPOS_PATH [PATH_IN_REPOS]\n\n"
      "Print information about the history of a path in the repository (or\n"
      "the root directory if no path is supplied).\n"),
   {'r', svnlook__show_ids, 'l'} },

  {"info", subcommand_info, {0},
   N_("usage: svnlook info REPOS_PATH\n\n"
      "Print the author, datestamp, log message size, and log message.\n"),
   {'r', 't'} },

  {"lock", subcommand_lock, {0},
   N_("usage: svnlook lock REPOS_PATH PATH_IN_REPOS\n\n"
      "If a lock exists on a path in the repository, describe it.\n"),
   {0} },

  {"log", subcommand_log, {0},
   N_("usage: svnlook log REPOS_PATH\n\n"
      "Print the log message.\n"),
   {'r', 't'} },

  {"propget", subcommand_pget, {"pget", "pg"},
   N_("usage: 1. svnlook propget REPOS_PATH PROPNAME PATH_IN_REPOS\n"
      "                    "
      /* The line above is actually needed, so do NOT delete it! */
      "       2. svnlook propget --revprop REPOS_PATH PROPNAME\n\n"
      "Print the raw value of a property on a path in the repository.\n"
      "With --revprop, print the raw value of a revision property.\n"),
   {'r', 't', svnlook__revprop_opt} },

  {"proplist", subcommand_plist, {"plist", "pl"},
   N_("usage: 1. svnlook proplist REPOS_PATH PATH_IN_REPOS\n"
      "                      "
      /* The line above is actually needed, so do NOT delete it! */
      "       2. svnlook proplist --revprop REPOS_PATH\n\n"
      "List the properties of a path in the repository, or\n"
      "with the --revprop option, revision properties.\n"
      "With -v, show the property values too.\n"),
   {'r', 't', 'v', svnlook__revprop_opt, svnlook__xml_opt} },

  {"tree", subcommand_tree, {0},
   N_("usage: svnlook tree REPOS_PATH [PATH_IN_REPOS]\n\n"
      "Print the tree, starting at PATH_IN_REPOS (if supplied, at the root\n"
      "of the tree otherwise), optionally showing node revision ids.\n"),
   {'r', 't', 'N', svnlook__show_ids, svnlook__full_paths} },

  {"uuid", subcommand_uuid, {0},
   N_("usage: svnlook uuid REPOS_PATH\n\n"
      "Print the repository's UUID.\n"),
   {0} },

  {"youngest", subcommand_youngest, {0},
   N_("usage: svnlook youngest REPOS_PATH\n\n"
      "Print the youngest revision number.\n"),
   {0} },

  { NULL, NULL, {0}, NULL, {0} }
};


/* Baton for passing option/argument state to a subcommand function. */
struct svnlook_opt_state
{
  const char *repos_path;  /* 'arg0' is always the path to the repository. */
  const char *arg1;        /* Usually an fs path, a propname, or NULL. */
  const char *arg2;        /* Usually an fs path or NULL. */
  svn_revnum_t rev;
  const char *txn;
  svn_boolean_t version;          /* --version */
  svn_boolean_t show_ids;         /* --show-ids */
  apr_size_t limit;               /* --limit */
  svn_boolean_t help;             /* --help */
  svn_boolean_t no_diff_deleted;  /* --no-diff-deleted */
  svn_boolean_t no_diff_added;    /* --no-diff-added */
  svn_boolean_t diff_copy_from;   /* --diff-copy-from */
  svn_boolean_t verbose;          /* --verbose */
  svn_boolean_t revprop;          /* --revprop */
  svn_boolean_t full_paths;       /* --full-paths */
  svn_boolean_t copy_info;        /* --copy-info */
  svn_boolean_t non_recursive;    /* --non-recursive */
  svn_boolean_t xml;              /* --xml */
  const char *extensions;         /* diff extension args (UTF-8!) */
  svn_boolean_t quiet;            /* --quiet */
};


typedef struct svnlook_ctxt_t
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_boolean_t is_revision;
  svn_boolean_t show_ids;
  apr_size_t limit;
  svn_boolean_t no_diff_deleted;
  svn_boolean_t no_diff_added;
  svn_boolean_t diff_copy_from;
  svn_boolean_t full_paths;
  svn_boolean_t copy_info;
  svn_revnum_t rev_id;
  svn_fs_txn_t *txn;
  const char *txn_name /* UTF-8! */;
  const apr_array_header_t *diff_options;

} svnlook_ctxt_t;

/* A flag to see if we've been cancelled by the client or not. */
static volatile sig_atomic_t cancelled = FALSE;


/*** Helper functions. ***/

/* A signal handler to support cancellation. */
static void
signal_handler(int signum)
{
  apr_signal(signum, SIG_IGN);
  cancelled = TRUE;
}

/* Our cancellation callback. */
static svn_error_t *
check_cancel(void *baton)
{
  if (cancelled)
    return svn_error_create(SVN_ERR_CANCELLED, NULL, _("Caught signal"));
  else
    return SVN_NO_ERROR;
}


/* Version compatibility check */
static svn_error_t *
check_lib_versions(void)
{
  static const svn_version_checklist_t checklist[] =
    {
      { "svn_subr",  svn_subr_version },
      { "svn_repos", svn_repos_version },
      { "svn_fs",    svn_fs_version },
      { "svn_delta", svn_delta_version },
      { "svn_diff",  svn_diff_version },
      { NULL, NULL }
    };

  SVN_VERSION_DEFINE(my_version);
  return svn_ver_check_list(&my_version, checklist);
}


/* Get revision or transaction property PROP_NAME for the revision or
   transaction specified in C, allocating in in POOL and placing it in
   *PROP_VALUE. */
static svn_error_t *
get_property(svn_string_t **prop_value,
             svnlook_ctxt_t *c,
             const char *prop_name,
             apr_pool_t *pool)
{
  svn_string_t *raw_value;

  /* Fetch transaction property... */
  if (! c->is_revision)
    SVN_ERR(svn_fs_txn_prop(&raw_value, c->txn, prop_name, pool));

  /* ...or revision property -- it's your call. */
  else
    SVN_ERR(svn_fs_revision_prop(&raw_value, c->fs, c->rev_id,
                                 prop_name, pool));

  *prop_value = raw_value;

  return SVN_NO_ERROR;
}


static svn_error_t *
get_root(svn_fs_root_t **root,
         svnlook_ctxt_t *c,
         apr_pool_t *pool)
{
  /* Open up the appropriate root (revision or transaction). */
  if (c->is_revision)
    {
      /* If we didn't get a valid revision number, we'll look at the
         youngest revision. */
      if (! SVN_IS_VALID_REVNUM(c->rev_id))
        SVN_ERR(svn_fs_youngest_rev(&(c->rev_id), c->fs, pool));

      SVN_ERR(svn_fs_revision_root(root, c->fs, c->rev_id, pool));
    }
  else
    {
      SVN_ERR(svn_fs_txn_root(root, c->txn, pool));
    }

  return SVN_NO_ERROR;
}



/*** Tree Routines ***/

/* Generate a generic delta tree. */
static svn_error_t *
generate_delta_tree(svn_repos_node_t **tree,
                    svn_repos_t *repos,
                    svn_fs_root_t *root,
                    svn_revnum_t base_rev,
                    svn_boolean_t use_copy_history,
                    apr_pool_t *pool)
{
  svn_fs_root_t *base_root;
  const svn_delta_editor_t *editor;
  void *edit_baton;
  apr_pool_t *edit_pool = svn_pool_create(pool);
  svn_fs_t *fs = svn_repos_fs(repos);

  /* Get the base root. */
  SVN_ERR(svn_fs_revision_root(&base_root, fs, base_rev, pool));

  /* Request our editor. */
  SVN_ERR(svn_repos_node_editor(&editor, &edit_baton, repos,
                                base_root, root, pool, edit_pool));

  /* Drive our editor. */
  SVN_ERR(svn_repos_replay2(root, "", SVN_INVALID_REVNUM, FALSE,
                            editor, edit_baton, NULL, NULL, edit_pool));

  /* Return the tree we just built. */
  *tree = svn_repos_node_from_baton(edit_baton);
  svn_pool_destroy(edit_pool);
  return SVN_NO_ERROR;
}



/*** Tree Printing Routines ***/

/* Recursively print only directory nodes that either a) have property
   mods, or b) contains files that have changed. */
static svn_error_t *
print_dirs_changed_tree(svn_repos_node_t *node,
                        const char *path /* UTF-8! */,
                        apr_pool_t *pool)
{
  svn_repos_node_t *tmp_node;
  int print_me = 0;
  const char *full_path;
  apr_pool_t *subpool;

  SVN_ERR(check_cancel(NULL));

  if (! node)
    return SVN_NO_ERROR;

  /* Not a directory?  We're not interested. */
  if (node->kind != svn_node_dir)
    return SVN_NO_ERROR;

  /* Got prop mods?  Excellent. */
  if (node->prop_mod)
    print_me = 1;

  if (! print_me)
    {
      /* Fly through the list of children, checking for modified files. */
      tmp_node = node->child;
      if (tmp_node)
        {
          if ((tmp_node->kind == svn_node_file)
              || (tmp_node->text_mod)
              || (tmp_node->action == 'A')
              || (tmp_node->action == 'D'))
            {
              print_me = 1;
            }
          while (tmp_node->sibling && (! print_me ))
            {
              tmp_node = tmp_node->sibling;
              if ((tmp_node->kind == svn_node_file)
                  || (tmp_node->text_mod)
                  || (tmp_node->action == 'A')
                  || (tmp_node->action == 'D'))
                {
                  print_me = 1;
                }
            }
        }
    }

  /* Print the node if it qualifies. */
  if (print_me)
    {
      SVN_ERR(svn_cmdline_printf(pool, "%s/\n", path));
    }

  /* Return here if the node has no children. */
  tmp_node = node->child;
  if (! tmp_node)
    return SVN_NO_ERROR;

  /* Recursively handle the node's children. */
  subpool = svn_pool_create(pool);
  full_path = svn_dirent_join(path, tmp_node->name, subpool);
  SVN_ERR(print_dirs_changed_tree(tmp_node, full_path, subpool));
  while (tmp_node->sibling)
    {
      svn_pool_clear(subpool);
      tmp_node = tmp_node->sibling;
      full_path = svn_dirent_join(path, tmp_node->name, subpool);
      SVN_ERR(print_dirs_changed_tree(tmp_node, full_path, subpool));
    }
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


/* Recursively print all nodes in the tree that have been modified
   (do not include directories affected only by "bubble-up"). */
static svn_error_t *
print_changed_tree(svn_repos_node_t *node,
                   const char *path /* UTF-8! */,
                   svn_boolean_t copy_info,
                   apr_pool_t *pool)
{
  const char *full_path;
  char status[4] = "_  ";
  int print_me = 1;
  apr_pool_t *subpool;

  SVN_ERR(check_cancel(NULL));

  if (! node)
    return SVN_NO_ERROR;

  /* Print the node. */
  if (node->action == 'A')
    {
      status[0] = 'A';
      if (copy_info && node->copyfrom_path)
        status[2] = '+';
    }
  else if (node->action == 'D')
    status[0] = 'D';
  else if (node->action == 'R')
    {
      if ((! node->text_mod) && (! node->prop_mod))
        print_me = 0;
      if (node->text_mod)
        status[0] = 'U';
      if (node->prop_mod)
        status[1] = 'U';
    }
  else
    print_me = 0;

  /* Print this node unless told to skip it. */
  if (print_me)
    {
      SVN_ERR(svn_cmdline_printf(pool, "%s %s%s\n",
                                 status,
                                 path,
                                 node->kind == svn_node_dir ? "/" : ""));
      if (copy_info && node->copyfrom_path)
        /* Remove the leading slash from the copyfrom path for consistency
           with the rest of the output. */
        SVN_ERR(svn_cmdline_printf(pool, "    (from %s%s:r%ld)\n",
                                   (node->copyfrom_path[0] == '/'
                                    ? node->copyfrom_path + 1
                                    : node->copyfrom_path),
                                   (node->kind == svn_node_dir ? "/" : ""),
                                   node->copyfrom_rev));
    }

  /* Return here if the node has no children. */
  node = node->child;
  if (! node)
    return SVN_NO_ERROR;

  /* Recursively handle the node's children. */
  subpool = svn_pool_create(pool);
  full_path = svn_dirent_join(path, node->name, subpool);
  SVN_ERR(print_changed_tree(node, full_path, copy_info, subpool));
  while (node->sibling)
    {
      svn_pool_clear(subpool);
      node = node->sibling;
      full_path = svn_dirent_join(path, node->name, subpool);
      SVN_ERR(print_changed_tree(node, full_path, copy_info, subpool));
    }
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


static svn_error_t *
dump_contents(svn_stream_t *stream,
              svn_fs_root_t *root,
              const char *path /* UTF-8! */,
              apr_pool_t *pool)
{
  if (root == NULL)
    SVN_ERR(svn_stream_close(stream));  /* leave an empty file */
  else
    {
      svn_stream_t *contents;

      /* Grab the contents and copy them into the given stream. */
      SVN_ERR(svn_fs_file_contents(&contents, root, path, pool));
      SVN_ERR(svn_stream_copy3(contents, stream, NULL, NULL, pool));
    }

  return SVN_NO_ERROR;
}


/* Prepare temporary files *TMPFILE1 and *TMPFILE2 for diffing
   PATH1@ROOT1 versus PATH2@ROOT2.  If either ROOT1 or ROOT2 is NULL,
   the temporary file for its path/root will be an empty one.
   Otherwise, its temporary file will contain the contents of that
   path/root in the repository.

   An exception to this is when either path/root has an svn:mime-type
   property set on it which indicates that the file contains
   non-textual data -- in this case, the *IS_BINARY flag is set and no
   temporary files are created.

   Use POOL for all that allocation goodness. */
static svn_error_t *
prepare_tmpfiles(const char **tmpfile1,
                 const char **tmpfile2,
                 svn_boolean_t *is_binary,
                 svn_fs_root_t *root1,
                 const char *path1,
                 svn_fs_root_t *root2,
                 const char *path2,
                 const char *tmpdir,
                 apr_pool_t *pool)
{
  svn_string_t *mimetype;
  svn_stream_t *stream;

  /* Init the return values. */
  *tmpfile1 = NULL;
  *tmpfile2 = NULL;
  *is_binary = FALSE;

  assert(path1 && path2);

  /* Check for binary mimetypes.  If either file has a binary
     mimetype, get outta here.  */
  if (root1)
    {
      SVN_ERR(svn_fs_node_prop(&mimetype, root1, path1,
                               SVN_PROP_MIME_TYPE, pool));
      if (mimetype && svn_mime_type_is_binary(mimetype->data))
        {
          *is_binary = TRUE;
          return SVN_NO_ERROR;
        }
    }
  if (root2)
    {
      SVN_ERR(svn_fs_node_prop(&mimetype, root2, path2,
                               SVN_PROP_MIME_TYPE, pool));
      if (mimetype && svn_mime_type_is_binary(mimetype->data))
        {
          *is_binary = TRUE;
          return SVN_NO_ERROR;
        }
    }

  /* Now, prepare the two temporary files, each of which will either
     be empty, or will have real contents.  */
  SVN_ERR(svn_stream_open_unique(&stream, tmpfile1,
                                 tmpdir,
                                 svn_io_file_del_none,
                                 pool, pool));
  SVN_ERR(dump_contents(stream, root1, path1, pool));

  SVN_ERR(svn_stream_open_unique(&stream, tmpfile2,
                                 tmpdir,
                                 svn_io_file_del_none,
                                 pool, pool));
  SVN_ERR(dump_contents(stream, root2, path2, pool));

  return SVN_NO_ERROR;
}


/* Generate a diff label for PATH in ROOT, allocating in POOL.
   ROOT may be NULL, in which case revision 0 is used. */
static svn_error_t *
generate_label(const char **label,
               svn_fs_root_t *root,
               const char *path,
               apr_pool_t *pool)
{
  svn_string_t *date;
  const char *datestr;
  const char *name = NULL;
  svn_revnum_t rev = SVN_INVALID_REVNUM;

  if (root)
    {
      svn_fs_t *fs = svn_fs_root_fs(root);
      if (svn_fs_is_revision_root(root))
        {
          rev = svn_fs_revision_root_revision(root);
          SVN_ERR(svn_fs_revision_prop(&date, fs, rev,
                                       SVN_PROP_REVISION_DATE, pool));
        }
      else
        {
          svn_fs_txn_t *txn;
          name = svn_fs_txn_root_name(root, pool);
          SVN_ERR(svn_fs_open_txn(&txn, fs, name, pool));
          SVN_ERR(svn_fs_txn_prop(&date, txn, SVN_PROP_REVISION_DATE, pool));
        }
    }
  else
    {
      rev = 0;
      date = NULL;
    }

  if (date)
    datestr = apr_psprintf(pool, "%.10s %.8s UTC", date->data, date->data + 11);
  else
    datestr = "                       ";

  if (name)
    *label = apr_psprintf(pool, "%s\t%s (txn %s)",
                          path, datestr, name);
  else
    *label = apr_psprintf(pool, "%s\t%s (rev %ld)",
                          path, datestr, rev);
  return SVN_NO_ERROR;
}

/* A helper function used by display_prop_diffs.
   TOKEN is a string holding a property value.
   If TOKEN is empty, or is already terminated by an EOL marker,
   return TOKEN unmodified. Else, return a new string consisting
   of the concatenation of TOKEN and the system's default EOL marker.
   The new string is allocated from POOL.
   If HAD_EOL is not NULL, indicate in *HAD_EOL if the token had a EOL. */
static const svn_string_t *
maybe_append_eol(const svn_string_t *token, svn_boolean_t *had_eol,
                 apr_pool_t *pool)
{
  const char *curp;

  if (had_eol)
    *had_eol = FALSE;

  if (token->len == 0)
    return token;

  curp = token->data + token->len - 1;
  if (*curp == '\r')
    {
      if (had_eol)
        *had_eol = TRUE;
      return token;
    }
  else if (*curp != '\n')
    {
      return svn_string_createf(pool, "%s%s", token->data, APR_EOL_STR);
    }
  else
    {
      if (had_eol)
        *had_eol = TRUE;
      return token;
    }
}

/*
 * Constant diff output separator strings
 */
static const char equal_string[] =
  "===================================================================";
static const char under_string[] =
  "___________________________________________________________________";


/* Helper function to display differences in properties of a file */
static svn_error_t *
display_prop_diffs(const apr_array_header_t *prop_diffs,
                   apr_hash_t *orig_props,
                   const char *path,
                   apr_pool_t *pool)
{
  int i;

  SVN_ERR(svn_cmdline_printf(pool, "\nProperty changes on: %s\n%s\n",
                             path, under_string));

  for (i = 0; i < prop_diffs->nelts; i++)
    {
      const char *header_label;
      const svn_string_t *orig_value;
      const svn_prop_t *pc = &APR_ARRAY_IDX(prop_diffs, i, svn_prop_t);

      SVN_ERR(check_cancel(NULL));

      if (orig_props)
        orig_value = apr_hash_get(orig_props, pc->name, APR_HASH_KEY_STRING);
      else
        orig_value = NULL;

      if (! orig_value)
        header_label = "Added";
      else if (! pc->value)
        header_label = "Deleted";
      else
        header_label = "Modified";
      SVN_ERR(svn_cmdline_printf(pool, "%s: %s\n", header_label, pc->name));

      /* Flush stdout before we open a stream to it below. */
      SVN_ERR(svn_cmdline_fflush(stdout));

      {
        svn_stream_t *out;
        svn_diff_t *diff;
        svn_diff_file_options_t options;
        const svn_string_t *tmp;
        const svn_string_t *orig;
        const svn_string_t *val;
        svn_boolean_t val_has_eol;

        SVN_ERR(svn_stream_for_stdout(&out, pool));

        /* The last character in a property is often not a newline.
           An eol character is appended to prevent the diff API to add a
           ' \ No newline at end of file' line. We add 
           ' \ No newline at end of property' manually if needed. */
        tmp = orig_value ? orig_value : svn_string_create("", pool);
        orig = maybe_append_eol(tmp, NULL, pool);

        tmp = pc->value ? pc->value :
                                  svn_string_create("", pool);
        val = maybe_append_eol(tmp, &val_has_eol, pool);

        SVN_ERR(svn_diff_mem_string_diff(&diff, orig, val, &options, pool));

        /* UNIX patch will try to apply a diff even if the diff header
         * is missing. It tries to be helpful by asking the user for a
         * target filename when it can't determine the target filename
         * from the diff header. But there usually are no files which
         * UNIX patch could apply the property diff to, so we use "##"
         * instead of "@@" as the default hunk delimiter for property diffs.
         * We also supress the diff header. */
        SVN_ERR(svn_diff_mem_string_output_unified2(out, diff, FALSE, "##",
                                           svn_dirent_local_style(path, pool),
                                           svn_dirent_local_style(path, pool),
                                           svn_cmdline_output_encoding(pool),
                                           orig, val, pool));
        if (!val_has_eol)
          {
            const char *s = "\\ No newline at end of property" APR_EOL_STR;
            apr_size_t len = strlen(s);
            SVN_ERR(svn_stream_write(out, s, &len));
          }
      }
    }
  return svn_cmdline_fflush(stdout);
}



/* Recursively print all nodes in the tree that have been modified
   (do not include directories affected only by "bubble-up"). */
static svn_error_t *
print_diff_tree(svn_fs_root_t *root,
                svn_fs_root_t *base_root,
                svn_repos_node_t *node,
                const char *path /* UTF-8! */,
                const char *base_path /* UTF-8! */,
                const svnlook_ctxt_t *c,
                const char *tmpdir,
                apr_pool_t *pool)
{
  const char *orig_path = NULL, *new_path = NULL;
  svn_boolean_t do_diff = FALSE;
  svn_boolean_t orig_empty = FALSE;
  svn_boolean_t is_copy = FALSE;
  svn_boolean_t binary = FALSE;
  svn_boolean_t diff_header_printed = FALSE;
  apr_pool_t *subpool;
  svn_stringbuf_t *header;

  SVN_ERR(check_cancel(NULL));

  if (! node)
    return SVN_NO_ERROR;

  header = svn_stringbuf_create("", pool);

  /* Print copyfrom history for the top node of a copied tree. */
  if ((SVN_IS_VALID_REVNUM(node->copyfrom_rev))
      && (node->copyfrom_path != NULL))
    {
      /* This is ... a copy. */
      is_copy = TRUE;

      /* Propagate the new base.  Copyfrom paths usually start with a
         slash; we remove it for consistency with the target path.
         ### Yes, it would be *much* better for something in the path
             library to be taking care of this! */
      if (node->copyfrom_path[0] == '/')
        base_path = apr_pstrdup(pool, node->copyfrom_path + 1);
      else
        base_path = apr_pstrdup(pool, node->copyfrom_path);

      svn_stringbuf_appendcstr
        (header,
         apr_psprintf(pool, _("Copied: %s (from rev %ld, %s)\n"),
                      path, node->copyfrom_rev, base_path));

      SVN_ERR(svn_fs_revision_root(&base_root,
                                   svn_fs_root_fs(base_root),
                                   node->copyfrom_rev, pool));
    }

  /*** First, we'll just print file content diffs. ***/
  if (node->kind == svn_node_file)
    {
      /* Here's the generalized way we do our diffs:

         - First, we'll check for svn:mime-type properties on the old
           and new files.  If either has such a property, and it
           represents a binary type, we won't actually be doing a real
           diff.

         - Second, dump the contents of the new version of the file
           into the temporary directory.

         - Then, dump the contents of the old version of the file into
           the temporary directory.

         - Next, we run 'diff', passing the repository paths as the
           labels.

         - Finally, we delete the temporary files.  */
      if (node->action == 'R' && node->text_mod)
        {
          do_diff = TRUE;
          SVN_ERR(prepare_tmpfiles(&orig_path, &new_path, &binary,
                                   base_root, base_path, root, path,
                                   tmpdir, pool));
        }
      else if (c->diff_copy_from && node->action == 'A' && is_copy)
        {
          if (node->text_mod)
            {
              do_diff = TRUE;
              SVN_ERR(prepare_tmpfiles(&orig_path, &new_path, &binary,
                                       base_root, base_path, root, path,
                                       tmpdir, pool));
            }
        }
      else if (! c->no_diff_added && node->action == 'A')
        {
          do_diff = TRUE;
          orig_empty = TRUE;
          SVN_ERR(prepare_tmpfiles(&orig_path, &new_path, &binary,
                                   NULL, base_path, root, path,
                                   tmpdir, pool));
        }
      else if (! c->no_diff_deleted && node->action == 'D')
        {
          do_diff = TRUE;
          SVN_ERR(prepare_tmpfiles(&orig_path, &new_path, &binary,
                                   base_root, base_path, NULL, path,
                                   tmpdir, pool));
        }

      /* The header for the copy case has already been created, and we don't
         want a header here for files with only property modifications. */
      if (header->len == 0
          && (node->action != 'R' || node->text_mod))
        {
          svn_stringbuf_appendcstr
            (header, apr_psprintf(pool, "%s: %s\n",
                                  ((node->action == 'A') ? _("Added") :
                                   ((node->action == 'D') ? _("Deleted") :
                                    ((node->action == 'R') ? _("Modified")
                                     : _("Index")))),
                                  path));
        }
    }

  if (do_diff)
    {
      svn_stringbuf_appendcstr(header, equal_string);
      svn_stringbuf_appendcstr(header, "\n");

      if (binary)
        {
          svn_stringbuf_appendcstr(header, _("(Binary files differ)\n\n"));
          SVN_ERR(svn_cmdline_printf(pool, "%s", header->data));
        }
      else
        {
          svn_diff_t *diff;
          svn_diff_file_options_t *opts = svn_diff_file_options_create(pool);

          if (c->diff_options)
            SVN_ERR(svn_diff_file_options_parse(opts, c->diff_options, pool));

          SVN_ERR(svn_diff_file_diff_2(&diff, orig_path,
                                       new_path, opts, pool));

          if (svn_diff_contains_diffs(diff))
            {
              svn_stream_t *ostream;
              const char *orig_label, *new_label;

              /* Print diff header. */
              SVN_ERR(svn_cmdline_printf(pool, "%s", header->data));

              /* This fflush() might seem odd, but it was added to deal
                 with this bug report:

                   http://subversion.tigris.org/servlets/ReadMsg?\
                   list=dev&msgNo=140782

                   From: "Steve Hay" <SteveHay{_AT_}planit.com>
                   To: <dev@subversion.tigris.org>
                   Subject: svnlook diff output in wrong order when redirected
                   Date: Fri, 4 Jul 2008 16:34:15 +0100
                   Message-ID: <1B32FF956ABF414C9BCE5E487A1497E702014F62@\
                                ukmail02.planit.group>

                 Adding the fflush() fixed the bug (not everyone could
                 reproduce it, but those who could confirmed the fix).
                 Later in the thread, Daniel Shahaf speculated as to
                 why the fix works:

                   "Because svn_cmdline_printf() uses the standard
                    'FILE *stdout' to write to stdout, while
                    svn_stream_for_stdout() uses (through
                    apr_file_open_stdout()) Windows API's to get a
                    handle for stdout?" */
              SVN_ERR(svn_cmdline_fflush(stdout));

              SVN_ERR(svn_stream_for_stdout(&ostream, pool));

              if (orig_empty)
                SVN_ERR(generate_label(&orig_label, NULL, path, pool));
              else
                SVN_ERR(generate_label(&orig_label, base_root,
                                       base_path, pool));
              SVN_ERR(generate_label(&new_label, root, path, pool));
              SVN_ERR(svn_diff_file_output_unified3
                      (ostream, diff, orig_path, new_path,
                       orig_label, new_label,
                       svn_cmdline_output_encoding(pool), NULL,
                       opts->show_c_function, pool));
              SVN_ERR(svn_stream_close(ostream));
              SVN_ERR(svn_cmdline_printf(pool, "\n"));
              diff_header_printed = TRUE;
            }
          else if (! node->prop_mod &&
                  ((! c->no_diff_added && node->action == 'A') ||
                   (! c->no_diff_deleted && node->action == 'D')))
            {
              /* There was an empty file added or deleted in this revision.
               * We can't print a diff, but we can at least print
               * a diff header since we know what happened to this file. */
              SVN_ERR(svn_cmdline_printf(pool, "%s", header->data));
            }
        }
      SVN_ERR(svn_cmdline_fflush(stdout));
    }

  /* Make sure we delete any temporary files. */
  if (orig_path)
    SVN_ERR(svn_io_remove_file2(orig_path, FALSE, pool));
  if (new_path)
    SVN_ERR(svn_io_remove_file2(new_path, FALSE, pool));

  /*** Now handle property diffs ***/
  if ((node->prop_mod) && (node->action != 'D'))
    {
      apr_hash_t *local_proptable;
      apr_hash_t *base_proptable;
      apr_array_header_t *propchanges, *props;

      SVN_ERR(svn_fs_node_proplist(&local_proptable, root, path, pool));
      if (c->diff_copy_from && node->action == 'A' && is_copy)
        SVN_ERR(svn_fs_node_proplist(&base_proptable, base_root,
                                     base_path, pool));
      else if (node->action == 'A')
        base_proptable = apr_hash_make(pool);
      else  /* node->action == 'R' */
        SVN_ERR(svn_fs_node_proplist(&base_proptable, base_root,
                                     base_path, pool));
      SVN_ERR(svn_prop_diffs(&propchanges, local_proptable,
                             base_proptable, pool));
      SVN_ERR(svn_categorize_props(propchanges, NULL, NULL, &props, pool));
      if (props->nelts > 0)
        {
          /* We print a diff header for the case when we only have property
           * mods. */
          if (! diff_header_printed)
            {
              const char *orig_label, *new_label;

              SVN_ERR(generate_label(&orig_label, base_root, base_path,
                                     pool));
              SVN_ERR(generate_label(&new_label, root, path, pool));

              SVN_ERR(svn_cmdline_printf(pool, "Index: %s\n", path));
              SVN_ERR(svn_cmdline_printf(pool, "%s\n", equal_string));
              SVN_ERR(svn_cmdline_printf(pool, "--- %s\n", orig_label));
              SVN_ERR(svn_cmdline_printf(pool, "+++ %s\n", new_label));
            }
          SVN_ERR(display_prop_diffs(props, base_proptable, path, pool));
        }
    }

  /* Return here if the node has no children. */
  node = node->child;
  if (! node)
    return SVN_NO_ERROR;

  /* Recursively handle the node's children. */
  subpool = svn_pool_create(pool);
  SVN_ERR(print_diff_tree(root, base_root, node,
                          svn_dirent_join(path, node->name, subpool),
                          svn_dirent_join(base_path, node->name, subpool),
                          c, tmpdir, subpool));
  while (node->sibling)
    {
      svn_pool_clear(subpool);
      node = node->sibling;
      SVN_ERR(print_diff_tree(root, base_root, node,
                              svn_dirent_join(path, node->name, subpool),
                              svn_dirent_join(base_path, node->name, subpool),
                              c, tmpdir, subpool));
    }
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


/* Print a repository directory, maybe recursively, possibly showing
   the node revision ids, and optionally using full paths.

   ROOT is the revision or transaction root used to build that tree.
   PATH and ID are the current path and node revision id being
   printed, and INDENTATION the number of spaces to prepent to that
   path's printed output.  ID may be NULL if SHOW_IDS is FALSE (in
   which case, ids won't be printed at all).  If RECURSE is TRUE,
   then print the tree recursively; otherwise, we'll stop after the
   first level (and use INDENTATION to keep track of how deep we are).

   Use POOL for all allocations.  */
static svn_error_t *
print_tree(svn_fs_root_t *root,
           const char *path /* UTF-8! */,
           const svn_fs_id_t *id,
           svn_boolean_t is_dir,
           int indentation,
           svn_boolean_t show_ids,
           svn_boolean_t full_paths,
           svn_boolean_t recurse,
           apr_pool_t *pool)
{
  apr_pool_t *subpool;
  apr_hash_t *entries;
  apr_hash_index_t *hi;
  const char* name;

  SVN_ERR(check_cancel(NULL));

  /* Print the indentation. */
  if (!full_paths)
    {
      int i;
      for (i = 0; i < indentation; i++)
        SVN_ERR(svn_cmdline_fputs(" ", stdout, pool));
    }

  /* ### The path format is inconsistent.. needs fix */
  if (full_paths)
    name = path;
  else if (*path == '/')
    name = svn_fspath__basename(path, pool);
  else
    name = svn_relpath_basename(path, NULL);

  if (svn_path_is_empty(name))
    name = "/"; /* basename of '/' is "" */

  /* Print the node. */
  SVN_ERR(svn_cmdline_printf(pool, "%s%s",
                             name,
                             is_dir && strcmp(name, "/") ? "/" : ""));

  if (show_ids)
    {
      svn_string_t *unparsed_id = NULL;
      if (id)
        unparsed_id = svn_fs_unparse_id(id, pool);
      SVN_ERR(svn_cmdline_printf(pool, " <%s>",
                                 unparsed_id
                                 ? unparsed_id->data
                                 : _("unknown")));
    }
  SVN_ERR(svn_cmdline_fputs("\n", stdout, pool));

  /* Return here if PATH is not a directory. */
  if (! is_dir)
    return SVN_NO_ERROR;

  /* Recursively handle the node's children. */
  if (recurse || (indentation == 0))
    {
      SVN_ERR(svn_fs_dir_entries(&entries, root, path, pool));
      subpool = svn_pool_create(pool);
      for (hi = apr_hash_first(pool, entries); hi; hi = apr_hash_next(hi))
        {
          svn_fs_dirent_t *entry = svn__apr_hash_index_val(hi);

          svn_pool_clear(subpool);
          SVN_ERR(print_tree(root,
                             (*path == '/')
                                 ? svn_fspath__join(path, entry->name, pool)
                                 : svn_relpath_join(path, entry->name, pool),
                             entry->id, (entry->kind == svn_node_dir),
                             indentation + 1, show_ids, full_paths,
                             recurse, subpool));
        }
      svn_pool_destroy(subpool);
    }

  return SVN_NO_ERROR;
}


/* Set *BASE_REV to the revision on which the target root specified in
   C is based, or to SVN_INVALID_REVNUM when C represents "revision
   0" (because that revision isn't based on another revision). */
static svn_error_t *
get_base_rev(svn_revnum_t *base_rev, svnlook_ctxt_t *c, apr_pool_t *pool)
{
  if (c->is_revision)
    {
      *base_rev = c->rev_id - 1;
    }
  else
    {
      *base_rev = svn_fs_txn_base_revision(c->txn);

      if (! SVN_IS_VALID_REVNUM(*base_rev))
        return svn_error_createf
          (SVN_ERR_FS_NO_SUCH_REVISION, NULL,
           _("Transaction '%s' is not based on a revision; how odd"),
           c->txn_name);
    }
  return SVN_NO_ERROR;
}



/*** Subcommand handlers. ***/

/* Print the revision's log message to stdout, followed by a newline. */
static svn_error_t *
do_log(svnlook_ctxt_t *c, svn_boolean_t print_size, apr_pool_t *pool)
{
  svn_string_t *prop_value;
  const char *prop_value_eol, *prop_value_native;
  svn_stream_t *stream;
  svn_error_t *err;
  apr_size_t len;

  SVN_ERR(get_property(&prop_value, c, SVN_PROP_REVISION_LOG, pool));
  if (! (prop_value && prop_value->data))
    {
      SVN_ERR(svn_cmdline_printf(pool, "%s\n", print_size ? "0" : ""));
      return SVN_NO_ERROR;
    }

  /* We immitate what svn_cmdline_printf does here, since we need the byte
     size of what we are going to print. */

  SVN_ERR(svn_subst_translate_cstring2(prop_value->data, &prop_value_eol,
                                       APR_EOL_STR, TRUE,
                                       NULL, FALSE, pool));

  err = svn_cmdline_cstring_from_utf8(&prop_value_native, prop_value_eol,
                                      pool);
  if (err)
    {
      svn_error_clear(err);
      prop_value_native = svn_cmdline_cstring_from_utf8_fuzzy(prop_value_eol,
                                                              pool);
    }

  len = strlen(prop_value_native);

  if (print_size)
    SVN_ERR(svn_cmdline_printf(pool, "%" APR_SIZE_T_FMT "\n", len));

  /* Use a stream to bypass all stdio translations. */
  SVN_ERR(svn_cmdline_fflush(stdout));
  SVN_ERR(svn_stream_for_stdout(&stream, pool));
  SVN_ERR(svn_stream_write(stream, prop_value_native, &len));
  SVN_ERR(svn_stream_close(stream));

  SVN_ERR(svn_cmdline_fputs("\n", stdout, pool));

  return SVN_NO_ERROR;
}


/* Print the timestamp of the commit (in the revision case) or the
   empty string (in the transaction case) to stdout, followed by a
   newline. */
static svn_error_t *
do_date(svnlook_ctxt_t *c, apr_pool_t *pool)
{
  svn_string_t *prop_value;

  SVN_ERR(get_property(&prop_value, c, SVN_PROP_REVISION_DATE, pool));
  if (prop_value && prop_value->data)
    {
      /* Convert the date for humans. */
      apr_time_t aprtime;
      const char *time_utf8;

      SVN_ERR(svn_time_from_cstring(&aprtime, prop_value->data, pool));

      time_utf8 = svn_time_to_human_cstring(aprtime, pool);

      SVN_ERR(svn_cmdline_printf(pool, "%s", time_utf8));
    }

  SVN_ERR(svn_cmdline_printf(pool, "\n"));
  return SVN_NO_ERROR;
}


/* Print the author of the commit to stdout, followed by a newline. */
static svn_error_t *
do_author(svnlook_ctxt_t *c, apr_pool_t *pool)
{
  svn_string_t *prop_value;

  SVN_ERR(get_property(&prop_value, c,
                       SVN_PROP_REVISION_AUTHOR, pool));
  if (prop_value && prop_value->data)
    SVN_ERR(svn_cmdline_printf(pool, "%s", prop_value->data));

  SVN_ERR(svn_cmdline_printf(pool, "\n"));
  return SVN_NO_ERROR;
}


/* Print a list of all directories in which files, or directory
   properties, have been modified. */
static svn_error_t *
do_dirs_changed(svnlook_ctxt_t *c, apr_pool_t *pool)
{
  svn_fs_root_t *root;
  svn_revnum_t base_rev_id;
  svn_repos_node_t *tree;

  SVN_ERR(get_root(&root, c, pool));
  SVN_ERR(get_base_rev(&base_rev_id, c, pool));
  if (base_rev_id == SVN_INVALID_REVNUM)
    return SVN_NO_ERROR;

  SVN_ERR(generate_delta_tree(&tree, c->repos, root, base_rev_id,
                              TRUE, pool));
  if (tree)
    SVN_ERR(print_dirs_changed_tree(tree, "", pool));

  return SVN_NO_ERROR;
}


/* Set *KIND to PATH's kind, if PATH exists.
 *
 * If PATH does not exist, then error; the text of the error depends
 * on whether PATH looks like a URL or not.
 */
static svn_error_t *
verify_path(svn_node_kind_t *kind,
            svn_fs_root_t *root,
            const char *path,
            apr_pool_t *pool)
{
  SVN_ERR(svn_fs_check_path(kind, root, path, pool));

  if (*kind == svn_node_none)
    {
      if (svn_path_is_url(path))  /* check for a common mistake. */
        return svn_error_createf
          (SVN_ERR_FS_NOT_FOUND, NULL,
           _("'%s' is a URL, probably should be a path"), path);
      else
        return svn_error_createf
          (SVN_ERR_FS_NOT_FOUND, NULL, _("Path '%s' does not exist"), path);
    }

  return SVN_NO_ERROR;
}


/* Print the size (in bytes) of a file. */
static svn_error_t *
do_filesize(svnlook_ctxt_t *c, const char *path, apr_pool_t *pool)
{
  svn_fs_root_t *root;
  svn_node_kind_t kind;
  svn_filesize_t length;

  SVN_ERR(get_root(&root, c, pool));
  SVN_ERR(verify_path(&kind, root, path, pool));

  if (kind != svn_node_file)
    return svn_error_createf
      (SVN_ERR_FS_NOT_FILE, NULL, _("Path '%s' is not a file"), path);

  /* Else. */

  SVN_ERR(svn_fs_file_length(&length, root, path, pool));
  return svn_cmdline_printf(pool, "%" SVN_FILESIZE_T_FMT "\n", length);
}

/* Print the contents of the file at PATH in the repository.
   Error with SVN_ERR_FS_NOT_FOUND if PATH does not exist, or with
   SVN_ERR_FS_NOT_FILE if PATH exists but is not a file. */
static svn_error_t *
do_cat(svnlook_ctxt_t *c, const char *path, apr_pool_t *pool)
{
  svn_fs_root_t *root;
  svn_node_kind_t kind;
  svn_stream_t *fstream, *stdout_stream;

  SVN_ERR(get_root(&root, c, pool));
  SVN_ERR(verify_path(&kind, root, path, pool));

  if (kind != svn_node_file)
    return svn_error_createf
      (SVN_ERR_FS_NOT_FILE, NULL, _("Path '%s' is not a file"), path);

  /* Else. */

  SVN_ERR(svn_fs_file_contents(&fstream, root, path, pool));
  SVN_ERR(svn_stream_for_stdout(&stdout_stream, pool));

  return svn_stream_copy3(fstream, svn_stream_disown(stdout_stream, pool),
                          check_cancel, NULL, pool);
}


/* Print a list of all paths modified in a format compatible with `svn
   update'. */
static svn_error_t *
do_changed(svnlook_ctxt_t *c, apr_pool_t *pool)
{
  svn_fs_root_t *root;
  svn_revnum_t base_rev_id;
  svn_repos_node_t *tree;

  SVN_ERR(get_root(&root, c, pool));
  SVN_ERR(get_base_rev(&base_rev_id, c, pool));
  if (base_rev_id == SVN_INVALID_REVNUM)
    return SVN_NO_ERROR;

  SVN_ERR(generate_delta_tree(&tree, c->repos, root, base_rev_id,
                              TRUE, pool));
  if (tree)
    SVN_ERR(print_changed_tree(tree, "", c->copy_info, pool));

  return SVN_NO_ERROR;
}


/* Print some diff-y stuff in a TBD way. :-) */
static svn_error_t *
do_diff(svnlook_ctxt_t *c, apr_pool_t *pool)
{
  svn_fs_root_t *root, *base_root;
  svn_revnum_t base_rev_id;
  svn_repos_node_t *tree;

  SVN_ERR(get_root(&root, c, pool));
  SVN_ERR(get_base_rev(&base_rev_id, c, pool));
  if (base_rev_id == SVN_INVALID_REVNUM)
    return SVN_NO_ERROR;

  SVN_ERR(generate_delta_tree(&tree, c->repos, root, base_rev_id,
                              TRUE, pool));
  if (tree)
    {
      const char *tmpdir;

      SVN_ERR(svn_fs_revision_root(&base_root, c->fs, base_rev_id, pool));
      SVN_ERR(svn_io_temp_dir(&tmpdir, pool));

      SVN_ERR(print_diff_tree(root, base_root, tree, "", "",
                              c, tmpdir, pool));
    }
  return SVN_NO_ERROR;
}



/* Callback baton for print_history() (and do_history()). */
struct print_history_baton
{
  svn_fs_t *fs;
  svn_boolean_t show_ids;    /* whether to show node IDs */
  apr_size_t limit;          /* max number of history items */
  apr_size_t count;          /* number of history items processed */
};

/* Implements svn_repos_history_func_t interface.  Print the history
   that's reported through this callback, possibly finding and
   displaying node-rev-ids. */
static svn_error_t *
print_history(void *baton,
              const char *path,
              svn_revnum_t revision,
              apr_pool_t *pool)
{
  struct print_history_baton *phb = baton;

  SVN_ERR(check_cancel(NULL));

  if (phb->show_ids)
    {
      const svn_fs_id_t *node_id;
      svn_fs_root_t *rev_root;
      svn_string_t *id_string;

      SVN_ERR(svn_fs_revision_root(&rev_root, phb->fs, revision, pool));
      SVN_ERR(svn_fs_node_id(&node_id, rev_root, path, pool));
      id_string = svn_fs_unparse_id(node_id, pool);
      SVN_ERR(svn_cmdline_printf(pool, "%8ld   %s <%s>\n",
                                 revision, path, id_string->data));
    }
  else
    {
      SVN_ERR(svn_cmdline_printf(pool, "%8ld   %s\n", revision, path));
    }

  if (phb->limit > 0)
    {
      phb->count++;
      if (phb->count >= phb->limit)
        /* Not L10N'd, since this error is supressed by the caller. */
        return svn_error_create(SVN_ERR_CEASE_INVOCATION, NULL,
                                _("History item limit reached"));
    }

  return SVN_NO_ERROR;
}


/* Print a tabular display of history location points for PATH in
   revision C->rev_id.  Optionally, SHOW_IDS.  Use POOL for
   allocations. */
static svn_error_t *
do_history(svnlook_ctxt_t *c,
           const char *path,
           apr_pool_t *pool)
{
  struct print_history_baton args;

  if (c->show_ids)
    {
      SVN_ERR(svn_cmdline_printf(pool, _("REVISION   PATH <ID>\n"
                                         "--------   ---------\n")));
    }
  else
    {
      SVN_ERR(svn_cmdline_printf(pool, _("REVISION   PATH\n"
                                         "--------   ----\n")));
    }

  /* Call our history crawler.  We want the whole lifetime of the path
     (prior to the user-supplied revision, of course), across all
     copies. */
  args.fs = c->fs;
  args.show_ids = c->show_ids;
  args.limit = c->limit;
  args.count = 0;
  SVN_ERR(svn_repos_history2(c->fs, path, print_history, &args,
                             NULL, NULL, 0, c->rev_id, TRUE, pool));
  return SVN_NO_ERROR;
}


/* Print the value of property PROPNAME on PATH in the repository.
   Error with SVN_ERR_FS_NOT_FOUND if PATH does not exist, or with
   SVN_ERR_PROPERTY_NOT_FOUND if no such property on PATH.
   If PATH is NULL, operate on a revision property. */
static svn_error_t *
do_pget(svnlook_ctxt_t *c,
        const char *propname,
        const char *path,
        apr_pool_t *pool)
{
  svn_fs_root_t *root;
  svn_string_t *prop;
  svn_node_kind_t kind;
  svn_stream_t *stdout_stream;
  apr_size_t len;

  SVN_ERR(get_root(&root, c, pool));
  if (path != NULL)
    {
      SVN_ERR(verify_path(&kind, root, path, pool));
      SVN_ERR(svn_fs_node_prop(&prop, root, path, propname, pool));
    }
  else
    SVN_ERR(get_property(&prop, c, propname, pool));

  if (prop == NULL)
    {
       const char *err_msg;
       if (path == NULL)
         {
           /* We're operating on a revprop (e.g. c->is_revision). */
           err_msg = apr_psprintf(pool,
                                  _("Property '%s' not found on revision %ld"),
                                  propname, c->rev_id);
         }
       else
         {
           if (SVN_IS_VALID_REVNUM(c->rev_id))
             err_msg = apr_psprintf(pool,
                                    _("Property '%s' not found on path '%s' "
                                      "in revision %ld"),
                                    propname, path, c->rev_id);
           else
             err_msg = apr_psprintf(pool,
                                    _("Property '%s' not found on path '%s' "
                                      "in transaction %s"),
                                    propname, path, c->txn_name);
         }
       return svn_error_create(SVN_ERR_PROPERTY_NOT_FOUND, NULL, err_msg);
    }

  /* Else. */

  SVN_ERR(svn_stream_for_stdout(&stdout_stream, pool));

  /* Unlike the command line client, we don't translate the property
     value or print a trailing newline here.  We just output the raw
     bytes of whatever's in the repository, as svnlook is more likely
     to be used for automated inspections. */
  len = prop->len;
  SVN_ERR(svn_stream_write(stdout_stream, prop->data, &len));

  return SVN_NO_ERROR;
}


/* Print the property names of all properties on PATH in the repository.
   If VERBOSE, print their values too.
   If XML, print as XML rather than as plain text.
   Error with SVN_ERR_FS_NOT_FOUND if PATH does not exist, or with
   SVN_ERR_PROPERTY_NOT_FOUND if no such property on PATH.
   If PATH is NULL, operate on a revision properties. */
static svn_error_t *
do_plist(svnlook_ctxt_t *c,
         const char *path,
         svn_boolean_t verbose,
         svn_boolean_t xml,
         apr_pool_t *pool)
{
  svn_stream_t *stdout_stream;
  svn_fs_root_t *root;
  apr_hash_t *props;
  apr_hash_index_t *hi;
  svn_node_kind_t kind;
  svn_stringbuf_t *sb = NULL;
  svn_boolean_t revprop = FALSE;

  SVN_ERR(svn_stream_for_stdout(&stdout_stream, pool));
  if (path != NULL)
    {
      SVN_ERR(get_root(&root, c, pool));
      SVN_ERR(verify_path(&kind, root, path, pool));
      SVN_ERR(svn_fs_node_proplist(&props, root, path, pool));
    }
  else if (c->is_revision)
    {
      SVN_ERR(svn_fs_revision_proplist(&props, c->fs, c->rev_id, pool));
      revprop = TRUE;
    }
  else
    {
      SVN_ERR(svn_fs_txn_proplist(&props, c->txn, pool));
      revprop = TRUE;
    }

  if (xml)
    {
      char *revstr = apr_psprintf(pool, "%ld", c->rev_id);
      /* <?xml version="1.0" encoding="UTF-8"?> */
      svn_xml_make_header2(&sb, "UTF-8", pool);

      /* "<properties>" */
      svn_xml_make_open_tag(&sb, pool, svn_xml_normal, "properties", NULL);

      if (revprop)
        {
          /* "<revprops ...>" */
          if (c->is_revision)
            {
              svn_xml_make_open_tag(&sb, pool, svn_xml_normal, "revprops",
                                    "rev", revstr, NULL);
            }
          else
            {
              svn_xml_make_open_tag(&sb, pool, svn_xml_normal, "revprops",
                                    "txn", c->txn_name, NULL);
            }
        }
      else
        {
          /* "<target ...>" */
          svn_xml_make_open_tag(&sb, pool, svn_xml_normal, "target",
                                "path", path, NULL);
        }
    }

  for (hi = apr_hash_first(pool, props); hi; hi = apr_hash_next(hi))
    {
      const char *pname = svn__apr_hash_index_key(hi);
      svn_string_t *propval = svn__apr_hash_index_val(hi);

      SVN_ERR(check_cancel(NULL));

      /* Since we're already adding a trailing newline (and possible a
         colon and some spaces) anyway, just mimic the output of the
         command line client proplist.   Compare to 'svnlook propget',
         which sends the raw bytes to stdout, untranslated. */
      /* We leave printf calls here, since we don't always know the encoding
         of the prop value. */
      if (svn_prop_needs_translation(pname))
        SVN_ERR(svn_subst_detranslate_string(&propval, propval, TRUE, pool));

      if (verbose)
        {
          if (xml)
            svn_cmdline__print_xml_prop(&sb, pname, propval, pool);
          else
            {
              const char *pname_stdout;

              SVN_ERR(svn_cmdline_cstring_from_utf8(&pname_stdout, pname,
                                                    pool));
              printf("  %s : %s\n", pname_stdout, propval->data);
            }
        }
      else if (xml)
        svn_xml_make_open_tag(&sb, pool, svn_xml_self_closing, "property",
                              "name", pname, NULL);
      else
        printf("  %s\n", pname);
    }
  if (xml)
    {
      errno = 0;
      if (revprop)
        {
          /* "</revprops>" */
          svn_xml_make_close_tag(&sb, pool, "revprops");
        }
      else
        {
          /* "</target>" */
          svn_xml_make_close_tag(&sb, pool, "target");
        }

      /* "</properties>" */
      svn_xml_make_close_tag(&sb, pool, "properties");

      if (fputs(sb->data, stdout) == EOF)
        {
          if (errno)
            return svn_error_wrap_apr(errno, _("Write error"));
          else
            return svn_error_create(SVN_ERR_IO_WRITE_ERROR, NULL, NULL);
        }
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
do_tree(svnlook_ctxt_t *c,
        const char *path,
        svn_boolean_t show_ids,
        svn_boolean_t full_paths,
        svn_boolean_t recurse,
        apr_pool_t *pool)
{
  svn_fs_root_t *root;
  const svn_fs_id_t *id;
  svn_boolean_t is_dir;

  SVN_ERR(get_root(&root, c, pool));
  SVN_ERR(svn_fs_node_id(&id, root, path, pool));
  SVN_ERR(svn_fs_is_dir(&is_dir, root, path, pool));
  SVN_ERR(print_tree(root, path, id, is_dir, 0, show_ids, full_paths,
                     recurse, pool));
  return SVN_NO_ERROR;
}


/* Custom filesystem warning function. */
static void
warning_func(void *baton,
             svn_error_t *err)
{
  if (! err)
    return;
  svn_handle_error2(err, stderr, FALSE, "svnlook: ");
}


/* Return an error if the number of arguments (excluding the repository
 * argument) is not NUM_ARGS.  NUM_ARGS must be 0 or 1.  The arguments
 * are assumed to be found in OPT_STATE->arg1 and OPT_STATE->arg2. */
static svn_error_t *
check_number_of_args(struct svnlook_opt_state *opt_state,
                     int num_args)
{
  if ((num_args == 0 && opt_state->arg1 != NULL)
      || (num_args == 1 && opt_state->arg2 != NULL))
    return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                            _("Too many arguments given"));
  if ((num_args == 1 && opt_state->arg1 == NULL))
    return svn_error_create(SVN_ERR_CL_INSUFFICIENT_ARGS, NULL,
                            _("Missing repository path argument"));
  return SVN_NO_ERROR;
}


/* Factory function for the context baton. */
static svn_error_t *
get_ctxt_baton(svnlook_ctxt_t **baton_p,
               struct svnlook_opt_state *opt_state,
               apr_pool_t *pool)
{
  svnlook_ctxt_t *baton = apr_pcalloc(pool, sizeof(*baton));

  SVN_ERR(svn_repos_open2(&(baton->repos), opt_state->repos_path, NULL,
                          pool));
  baton->fs = svn_repos_fs(baton->repos);
  svn_fs_set_warning_func(baton->fs, warning_func, NULL);
  baton->show_ids = opt_state->show_ids;
  baton->limit = opt_state->limit;
  baton->no_diff_deleted = opt_state->no_diff_deleted;
  baton->no_diff_added = opt_state->no_diff_added;
  baton->diff_copy_from = opt_state->diff_copy_from;
  baton->full_paths = opt_state->full_paths;
  baton->copy_info = opt_state->copy_info;
  baton->is_revision = opt_state->txn == NULL;
  baton->rev_id = opt_state->rev;
  baton->txn_name = apr_pstrdup(pool, opt_state->txn);
  baton->diff_options = svn_cstring_split(opt_state->extensions
                                          ? opt_state->extensions : "",
                                          " \t\n\r", TRUE, pool);

  if (baton->txn_name)
    SVN_ERR(svn_fs_open_txn(&(baton->txn), baton->fs,
                            baton->txn_name, pool));
  else if (baton->rev_id == SVN_INVALID_REVNUM)
    SVN_ERR(svn_fs_youngest_rev(&(baton->rev_id), baton->fs, pool));

  *baton_p = baton;
  return SVN_NO_ERROR;
}



/*** Subcommands. ***/

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_author(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 0));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_author(c, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_cat(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 1));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_cat(c, opt_state->arg1, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_changed(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 0));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_changed(c, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_date(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 0));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_date(c, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_diff(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 0));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_diff(c, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_dirschanged(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 0));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_dirs_changed(c, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_filesize(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 1));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_filesize(c, opt_state->arg1, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_help(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  const char *header =
    _("general usage: svnlook SUBCOMMAND REPOS_PATH [ARGS & OPTIONS ...]\n"
      "Note: any subcommand which takes the '--revision' and '--transaction'\n"
      "      options will, if invoked without one of those options, act on\n"
      "      the repository's youngest revision.\n"
      "Type 'svnlook help <subcommand>' for help on a specific subcommand.\n"
      "Type 'svnlook --version' to see the program version and FS modules.\n"
      "\n"
      "Available subcommands:\n");

  const char *fs_desc_start
    = _("The following repository back-end (FS) modules are available:\n\n");

  svn_stringbuf_t *version_footer;

  version_footer = svn_stringbuf_create(fs_desc_start, pool);
  SVN_ERR(svn_fs_print_modules(version_footer, pool));

  SVN_ERR(svn_opt_print_help3(os, "svnlook",
                              opt_state ? opt_state->version : FALSE,
                              opt_state ? opt_state->quiet : FALSE,
                              version_footer->data,
                              header, cmd_table, options_table, NULL,
                              NULL, pool));

  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_history(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;
  const char *path = (opt_state->arg1 ? opt_state->arg1 : "/");

  if (opt_state->arg2 != NULL)
    return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                            _("Too many arguments given"));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_history(c, path, pool));
  return SVN_NO_ERROR;
}


/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_lock(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;
  svn_lock_t *lock;

  SVN_ERR(check_number_of_args(opt_state, 1));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));

  SVN_ERR(svn_fs_get_lock(&lock, c->fs, opt_state->arg1, pool));

  if (lock)
    {
      const char *cr_date, *exp_date = "";
      int comment_lines = 0;

      cr_date = svn_time_to_human_cstring(lock->creation_date, pool);

      if (lock->expiration_date)
        exp_date = svn_time_to_human_cstring(lock->expiration_date, pool);

      if (lock->comment)
        comment_lines = svn_cstring_count_newlines(lock->comment) + 1;

      SVN_ERR(svn_cmdline_printf(pool, _("UUID Token: %s\n"), lock->token));
      SVN_ERR(svn_cmdline_printf(pool, _("Owner: %s\n"), lock->owner));
      SVN_ERR(svn_cmdline_printf(pool, _("Created: %s\n"), cr_date));
      SVN_ERR(svn_cmdline_printf(pool, _("Expires: %s\n"), exp_date));
      SVN_ERR(svn_cmdline_printf(pool,
                                 Q_("Comment (%i line):\n%s\n",
                                    "Comment (%i lines):\n%s\n",
                                    comment_lines),
                                 comment_lines,
                                 lock->comment ? lock->comment : ""));
    }

  return SVN_NO_ERROR;
}


/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_info(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 0));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_author(c, pool));
  SVN_ERR(do_date(c, pool));
  SVN_ERR(do_log(c, TRUE, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_log(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 0));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_log(c, FALSE, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_pget(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  if (opt_state->arg1 == NULL)
    {
      return svn_error_createf
        (SVN_ERR_CL_INSUFFICIENT_ARGS, NULL,
         opt_state->revprop ?  _("Missing propname argument") :
         _("Missing propname and repository path arguments"));
    }
  else if (!opt_state->revprop && opt_state->arg2 == NULL)
    {
      return svn_error_create
        (SVN_ERR_CL_INSUFFICIENT_ARGS, NULL,
         _("Missing propname or repository path argument"));
    }
  if ((opt_state->revprop && opt_state->arg2 != NULL)
      || os->ind < os->argc)
    return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                            _("Too many arguments given"));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_pget(c, opt_state->arg1,
                  opt_state->revprop ? NULL : opt_state->arg2, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_plist(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, opt_state->revprop ? 0 : 1));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_plist(c, opt_state->revprop ? NULL : opt_state->arg1,
                   opt_state->verbose, opt_state->xml, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_tree(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  if (opt_state->arg2 != NULL)
    return svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                            _("Too many arguments given"));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(do_tree(c, opt_state->arg1 ? opt_state->arg1 : "",
                  opt_state->show_ids, opt_state->full_paths,
                  ! opt_state->non_recursive, pool));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_youngest(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;

  SVN_ERR(check_number_of_args(opt_state, 0));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(svn_cmdline_printf(pool, "%ld\n", c->rev_id));
  return SVN_NO_ERROR;
}

/* This implements `svn_opt_subcommand_t'. */
static svn_error_t *
subcommand_uuid(apr_getopt_t *os, void *baton, apr_pool_t *pool)
{
  struct svnlook_opt_state *opt_state = baton;
  svnlook_ctxt_t *c;
  const char *uuid;

  SVN_ERR(check_number_of_args(opt_state, 0));

  SVN_ERR(get_ctxt_baton(&c, opt_state, pool));
  SVN_ERR(svn_fs_get_uuid(c->fs, &uuid, pool));
  SVN_ERR(svn_cmdline_printf(pool, "%s\n", uuid));
  return SVN_NO_ERROR;
}



/*** Main. ***/

int
main(int argc, const char *argv[])
{
  svn_error_t *err;
  apr_status_t apr_err;
  apr_allocator_t *allocator;
  apr_pool_t *pool;

  const svn_opt_subcommand_desc2_t *subcommand = NULL;
  struct svnlook_opt_state opt_state;
  apr_getopt_t *os;
  int opt_id;
  apr_array_header_t *received_opts;
  int i;

  /* Initialize the app. */
  if (svn_cmdline_init("svnlook", stderr) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  /* Create our top-level pool.  Use a separate mutexless allocator,
   * given this application is single threaded.
   */
  if (apr_allocator_create(&allocator))
    return EXIT_FAILURE;

  apr_allocator_max_free_set(allocator, SVN_ALLOCATOR_RECOMMENDED_MAX_FREE);

  pool = svn_pool_create_ex(NULL, allocator);
  apr_allocator_owner_set(allocator, pool);

  received_opts = apr_array_make(pool, SVN_OPT_MAX_OPTIONS, sizeof(int));

  /* Check library versions */
  err = check_lib_versions();
  if (err)
    return svn_cmdline_handle_exit_error(err, pool, "svnlook: ");

  /* Initialize the FS library. */
  err = svn_fs_initialize(pool);
  if (err)
    return svn_cmdline_handle_exit_error(err, pool, "svnlook: ");

  if (argc <= 1)
    {
      SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
      svn_pool_destroy(pool);
      return EXIT_FAILURE;
    }

  /* Initialize opt_state. */
  memset(&opt_state, 0, sizeof(opt_state));
  opt_state.rev = SVN_INVALID_REVNUM;

  /* Parse options. */
  err = svn_cmdline__getopt_init(&os, argc, argv, pool);
  if (err)
    return svn_cmdline_handle_exit_error(err, pool, "svnlook: ");

  os->interleave = 1;
  while (1)
    {
      const char *opt_arg;

      /* Parse the next option. */
      apr_err = apr_getopt_long(os, options_table, &opt_id, &opt_arg);
      if (APR_STATUS_IS_EOF(apr_err))
        break;
      else if (apr_err)
        {
          SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
          svn_pool_destroy(pool);
          return EXIT_FAILURE;
        }

      /* Stash the option code in an array before parsing it. */
      APR_ARRAY_PUSH(received_opts, int) = opt_id;

      switch (opt_id)
        {
        case 'r':
          {
            char *digits_end = NULL;
            opt_state.rev = strtol(opt_arg, &digits_end, 10);
            if ((! SVN_IS_VALID_REVNUM(opt_state.rev))
                || (! digits_end)
                || *digits_end)
              SVN_INT_ERR(svn_error_create
                          (SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                           _("Invalid revision number supplied")));
          }
          break;

        case 't':
          opt_state.txn = opt_arg;
          break;

        case 'N':
          opt_state.non_recursive = TRUE;
          break;

        case 'v':
          opt_state.verbose = TRUE;
          break;

        case 'h':
        case '?':
          opt_state.help = TRUE;
          break;

        case 'q':
          opt_state.quiet = TRUE;
          break;

        case svnlook__revprop_opt:
          opt_state.revprop = TRUE;
          break;

        case svnlook__xml_opt:
          opt_state.xml = TRUE;
          break;

        case svnlook__version:
          opt_state.version = TRUE;
          break;

        case svnlook__show_ids:
          opt_state.show_ids = TRUE;
          break;

        case 'l':
          {
            char *end;
            opt_state.limit = strtol(opt_arg, &end, 10);
            if (end == opt_arg || *end != '\0')
              {
                err = svn_error_create(SVN_ERR_CL_ARG_PARSING_ERROR, NULL,
                                       _("Non-numeric limit argument given"));
                return svn_cmdline_handle_exit_error(err, pool, "svnlook: ");
              }
            if (opt_state.limit <= 0)
              {
                err = svn_error_create(SVN_ERR_INCORRECT_PARAMS, NULL,
                                    _("Argument to --limit must be positive"));
                return svn_cmdline_handle_exit_error(err, pool, "svnlook: ");
              }
          }
          break;

        case svnlook__no_diff_deleted:
          opt_state.no_diff_deleted = TRUE;
          break;

        case svnlook__no_diff_added:
          opt_state.no_diff_added = TRUE;
          break;

        case svnlook__diff_copy_from:
          opt_state.diff_copy_from = TRUE;
          break;

        case svnlook__full_paths:
          opt_state.full_paths = TRUE;
          break;

        case svnlook__copy_info:
          opt_state.copy_info = TRUE;
          break;

        case 'x':
          opt_state.extensions = opt_arg;
          break;

        default:
          SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
          svn_pool_destroy(pool);
          return EXIT_FAILURE;

        }
    }

  /* The --transaction and --revision options may not co-exist. */
  if ((opt_state.rev != SVN_INVALID_REVNUM) && opt_state.txn)
    SVN_INT_ERR(svn_error_create
                (SVN_ERR_CL_MUTUALLY_EXCLUSIVE_ARGS, NULL,
                 _("The '--transaction' (-t) and '--revision' (-r) arguments "
                   "cannot co-exist")));

  /* If the user asked for help, then the rest of the arguments are
     the names of subcommands to get help on (if any), or else they're
     just typos/mistakes.  Whatever the case, the subcommand to
     actually run is subcommand_help(). */
  if (opt_state.help)
    subcommand = svn_opt_get_canonical_subcommand2(cmd_table, "help");

  /* If we're not running the `help' subcommand, then look for a
     subcommand in the first argument. */
  if (subcommand == NULL)
    {
      if (os->ind >= os->argc)
        {
          if (opt_state.version)
            {
              /* Use the "help" subcommand to handle the "--version" option. */
              static const svn_opt_subcommand_desc2_t pseudo_cmd =
                { "--version", subcommand_help, {0}, "",
                  {svnlook__version,  /* must accept its own option */
                   'q',
                  } };

              subcommand = &pseudo_cmd;
            }
          else
            {
              svn_error_clear
                (svn_cmdline_fprintf(stderr, pool,
                                     _("Subcommand argument required\n")));
              SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
              svn_pool_destroy(pool);
              return EXIT_FAILURE;
            }
        }
      else
        {
          const char *first_arg = os->argv[os->ind++];
          subcommand = svn_opt_get_canonical_subcommand2(cmd_table, first_arg);
          if (subcommand == NULL)
            {
              const char *first_arg_utf8;
              err = svn_utf_cstring_to_utf8(&first_arg_utf8, first_arg,
                                            pool);
              if (err)
                return svn_cmdline_handle_exit_error(err, pool, "svnlook: ");
              svn_error_clear
                (svn_cmdline_fprintf(stderr, pool,
                                     _("Unknown command: '%s'\n"),
                                     first_arg_utf8));
              SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
              svn_pool_destroy(pool);
              return EXIT_FAILURE;
            }
        }
    }

  /* If there's a second argument, it's the repository.  There may be
     more arguments following the repository; usually the next one is
     a path within the repository, or it's a propname and the one
     after that is the path.  Since we don't know, we just call them
     arg1 and arg2, meaning the first and second arguments following
     the repository. */
  if (subcommand->cmd_func != subcommand_help)
    {
      const char *repos_path = NULL;
      const char *arg1 = NULL, *arg2 = NULL;

      /* Get the repository. */
      if (os->ind < os->argc)
        {
          SVN_INT_ERR(svn_utf_cstring_to_utf8(&repos_path,
                                              os->argv[os->ind++],
                                              pool));
          repos_path = svn_dirent_internal_style(repos_path, pool);
        }

      if (repos_path == NULL)
        {
          svn_error_clear
            (svn_cmdline_fprintf(stderr, pool,
                                 _("Repository argument required\n")));
          SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
          svn_pool_destroy(pool);
          return EXIT_FAILURE;
        }
      else if (svn_path_is_url(repos_path))
        {
          svn_error_clear
            (svn_cmdline_fprintf(stderr, pool,
                                 _("'%s' is a URL when it should be a path\n"),
                                 repos_path));
          svn_pool_destroy(pool);
          return EXIT_FAILURE;
        }

      opt_state.repos_path = repos_path;

      /* Get next arg (arg1), if any. */
      if (os->ind < os->argc)
        {
          SVN_INT_ERR(svn_utf_cstring_to_utf8
                      (&arg1, os->argv[os->ind++], pool));
          arg1 = svn_dirent_internal_style(arg1, pool);
        }
      opt_state.arg1 = arg1;

      /* Get next arg (arg2), if any. */
      if (os->ind < os->argc)
        {
          SVN_INT_ERR(svn_utf_cstring_to_utf8
                      (&arg2, os->argv[os->ind++], pool));
          arg2 = svn_dirent_internal_style(arg2, pool);
        }
      opt_state.arg2 = arg2;
    }

  /* Check that the subcommand wasn't passed any inappropriate options. */
  for (i = 0; i < received_opts->nelts; i++)
    {
      opt_id = APR_ARRAY_IDX(received_opts, i, int);

      /* All commands implicitly accept --help, so just skip over this
         when we see it. Note that we don't want to include this option
         in their "accepted options" list because it would be awfully
         redundant to display it in every commands' help text. */
      if (opt_id == 'h' || opt_id == '?')
        continue;

      if (! svn_opt_subcommand_takes_option3(subcommand, opt_id, NULL))
        {
          const char *optstr;
          const apr_getopt_option_t *badopt =
            svn_opt_get_option_from_code2(opt_id, options_table, subcommand,
                                          pool);
          svn_opt_format_option(&optstr, badopt, FALSE, pool);
          if (subcommand->name[0] == '-')
            SVN_INT_ERR(subcommand_help(NULL, NULL, pool));
          else
            svn_error_clear
              (svn_cmdline_fprintf
               (stderr, pool,
                _("Subcommand '%s' doesn't accept option '%s'\n"
                  "Type 'svnlook help %s' for usage.\n"),
                subcommand->name, optstr, subcommand->name));
          svn_pool_destroy(pool);
          return EXIT_FAILURE;
        }
    }

  /* Set up our cancellation support. */
  apr_signal(SIGINT, signal_handler);
#ifdef SIGBREAK
  /* SIGBREAK is a Win32 specific signal generated by ctrl-break. */
  apr_signal(SIGBREAK, signal_handler);
#endif
#ifdef SIGHUP
  apr_signal(SIGHUP, signal_handler);
#endif
#ifdef SIGTERM
  apr_signal(SIGTERM, signal_handler);
#endif

#ifdef SIGPIPE
  /* Disable SIGPIPE generation for the platforms that have it. */
  apr_signal(SIGPIPE, SIG_IGN);
#endif

#ifdef SIGXFSZ
  /* Disable SIGXFSZ generation for the platforms that have it, otherwise
   * working with large files when compiled against an APR that doesn't have
   * large file support will crash the program, which is uncool. */
  apr_signal(SIGXFSZ, SIG_IGN);
#endif

  /* Run the subcommand. */
  err = (*subcommand->cmd_func)(os, &opt_state, pool);
  if (err)
    {
      /* For argument-related problems, suggest using the 'help'
         subcommand. */
      if (err->apr_err == SVN_ERR_CL_INSUFFICIENT_ARGS
          || err->apr_err == SVN_ERR_CL_ARG_PARSING_ERROR)
        {
          err = svn_error_quick_wrap(err,
                                     _("Try 'svnlook help' for more info"));
        }
      return svn_cmdline_handle_exit_error(err, pool, "svnlook: ");
    }
  else
    {
      svn_pool_destroy(pool);
      /* Ensure everything is printed on stdout, so the user sees any
         print errors. */
      SVN_INT_ERR(svn_cmdline_fflush(stdout));
      return EXIT_SUCCESS;
    }
}
