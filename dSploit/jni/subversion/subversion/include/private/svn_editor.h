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
 * @file svn_editor.h
 * @brief Tree editing functions and structures
 */

#ifndef SVN_EDITOR_H
#define SVN_EDITOR_H

#include <apr_pools.h>

#include "svn_types.h"
#include "svn_error.h"
#include "svn_io.h"    /* for svn_stream_t  */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Transforming trees ("editing").
 *
 * In Subversion, we have a number of occasions where we transform a tree
 * from one state into another. This process is called "editing" a tree.
 *
 * In processing a `commit' command:
 * - The client examines its working copy data to determine the set of
 *   changes necessary to transform its base tree into the desired target.
 * - The client networking library delivers that set of changes/operations
 *   across the wire as an equivalent series of network requests (for
 *   example, to svnserve as an ra_svn protocol stream, or to an
 *   Apache httpd server as WebDAV commands)
 * - The server receives those requests and applies the sequence of
 *   operations on a revision, producing a transaction representing the
 *   desired target.
 * - The Subversion server then commits the transaction to the filesystem.
 *
 * In processing an `update' command, the process is reversed:
 * - The Subversion server module talks to the filesystem and computes a
 *   set of changes necessary to bring the client's working copy up to date.
 * - The server serializes this description of changes, and delivers it to
 *   the client.
 * - The client networking library receives that reply, producing a set
 *   of changes/operations to alter the working copy into the revision
 *   requested by the update command.
 * - The working copy library applies those operations to the working copy
 *   to align it with the requested update target.
 *
 * The series of changes (or operations) necessary to transform a tree from
 * one state into another is passed between subsystems using this "editor"
 * interface. The "receiver" edits its tree according to the operations
 * described by the "driver".
 *
 * Note that the driver must have a perfect understanding of the tree which
 * the receiver will be applying edits upon. There is no room for error here,
 * and the interface embodies assumptions/requirements that the driver has
 * about the targeted tree. As a result, this interface is a standardized
 * mechanism of *describing* those change operations, but the intimate
 * knowledge between the driver and the receiver implies some level of
 * coupling between those subsystems.
 *
 * The set of changes, and the data necessary to describe it entirely, is
 * completely unbounded. An addition of one simple 20Gb file would be well
 * past the available memory of any machine processing these operations.
 * As a result, the API to describe the changes is designed to be applied
 * in a sequential (and relatively random-access) model. The operations
 * can be streamed from the driver to the receiver, resulting in the
 * receiver editing its tree to the target state defined by the driver.
 *
 *
 * HISTORY
 *
 * Classically, Subversion had a notion of a "tree delta" which could be
 * passed around as an independent entity. Theory implied this delta was an
 * entity in its own right, to be used when and where necessary.
 * Unfortunately, this theory did not work well in practice. The producer
 * and consumer of these tree deltas were (and are) tightly coupled. As noted
 * above, the tree delta producer needed to be *totally* aware of the tree
 * that it needed to edit. So rather than telling the delta consumer how to
 * edit its tree, the classic "@c svn_delta_editor_t" interface focused
 * entirely on the tree delta, an intermediate (logical) data structure
 * which was unusable outside of the particular, coupled pairing of producer
 * and consumer. This generation of the API forgoes the logical tree delta
 * entity and directly passes the necessary edits/changes/operations from
 * the producer to the consumer. In our new parlance, one subsystem "drives"
 * a set of operations describing the change, and a "receiver" accepts and
 * applies them to its tree.
 *
 * The classic interface was named "@c svn_delta_editor_t" and was described
 * idiomatically as the "editor interface". This generation of the interface
 * retains the "editor" name for that reason. All notions of a "tree delta"
 * structure are no longer part of this interface.
 *
 * The old interface was purely vtable-based and used a number of special
 * editors which could be interposed between the driver and receiver. Those
 * editors provided cancellation, debugging, and other various operations.
 * While the "interposition" pattern is still possible with this interface,
 * the most common functionality (cancellation and debugging) have been
 * integrated directly into this new editor system.
 *
 * @defgroup svn_editor The editor interface
 * @{
 */

/** An abstract object that edits a target tree.
 *
 * \n
 * <h3>Life-Cycle</h3>
 *
 * - @b Create: A receiver uses svn_editor_create() to create an
 *    "empty" svn_editor_t.  It cannot be used yet, since it still lacks
 *    actual callback functions.  svn_editor_create() sets the
 *    #svn_editor_t's callback baton and scratch pool that the callback
 *    functions receive, as well as a cancellation callback and baton
 *    (see "Cancellation" below).
 *
 * - @b Set callbacks: The receiver calls svn_editor_setcb_many() or a
 *    succession of the other svn_editor_setcb_*() functions to tell
 *    #svn_editor_t which functions to call when driven by the various
 *    operations.  Callback functions are implemented by the receiver and must
 *    adhere to the @c svn_editor_cb_*_t function types as expected by the
 *    svn_editor_setcb_*() functions. See: \n
 *      svn_editor_cb_many_t \n
 *      svn_editor_setcb_many() \n
 *      or \n
 *      svn_editor_setcb_add_directory() \n
 *      svn_editor_setcb_add_file() \n
 *      svn_editor_setcb_add_symlink() \n
 *      svn_editor_setcb_add_absent() \n
 *      svn_editor_setcb_set_props() \n
 *      svn_editor_setcb_set_text() \n
 *      svn_editor_setcb_set_target() \n
 *      svn_editor_setcb_delete() \n
 *      svn_editor_setcb_copy() \n
 *      svn_editor_setcb_move() \n
 *      svn_editor_setcb_complete() \n
 *      svn_editor_setcb_abort()
 *
 * - @b Drive: The driver is provided with the completed #svn_editor_t
 *    instance. (It is typically passed to a generic driving
 *    API, which could receive the driving editor calls over the network
 *    by providing a proxy #svn_editor_t on the remote side.)
 *    The driver invokes the #svn_editor_t instance's callback functions
 *    according to the restrictions defined below, in order to describe the
 *    entire set of operations necessary to transform the receiver's tree
 *    into the desired target. The callbacks can be invoked using the
 *    svn_editor_*() functions, i.e.: \n
 *      svn_editor_add_directory() \n
 *      svn_editor_add_file() \n
 *      svn_editor_add_symlink() \n
 *      svn_editor_add_absent() \n
 *      svn_editor_set_props() \n
 *      svn_editor_set_text() \n
 *      svn_editor_set_target() \n
 *      svn_editor_delete() \n
 *      svn_editor_copy() \n
 *      svn_editor_move()
 *    \n\n
 *    Just before each callback invocation is carried out, the @a cancel_func
 *    that was passed to svn_editor_create() is invoked to poll any
 *    external reasons to cancel the sequence of operations.  Unless it
 *    overrides the cancellation (denoted by SVN_ERR_CANCELLED), the driver
 *    aborts the transmission by invoking the svn_editor_abort() callback.
 *    Exceptions to this are calls to svn_editor_complete() and
 *    svn_editor_abort(), which cannot be canceled externally.
 *
 * - @b Receive: While the driver invokes operations upon the editor, the
 *    receiver finds its callback functions called with the information
 *    to operate on its tree. Each actual callback function receives those
 *    arguments that the driver passed to the "driving" functions, plus these:
 *    -  @a baton: This is the @a editor_baton pointer originally passed to
 *       svn_editor_create().  It may be freely used by the callback
 *       implementation to store information across all callbacks.
 *    -  @a scratch_pool: This temporary pool is cleared directly after
 *       each callback returns.  See "Pool Usage".
 *    \n\n
 *    If the receiver encounters an error within a callback, it returns an
 *    #svn_error_t*. The driver receives this and aborts transmission.
 *
 * - @b Complete/Abort: The driver will end transmission by calling \n
 *    svn_editor_complete() if successful, or \n
 *    svn_editor_abort() if an error or cancellation occurred.
 * \n\n
 *
 * <h3>Driving Order Restrictions</h3>
 * In order to reduce complexity of callback receivers, the editor callbacks
 * must be driven in adherence to these rules:
 *
 * - svn_editor_add_directory() -- Another svn_editor_add_*() call must
 *   follow for each child mentioned in the @a children argument of any
 *   svn_editor_add_directory() call.
 *
 * - svn_editor_add_file() -- An svn_editor_set_text() call must follow
 *   for the same path (at some point).
 *
 * - svn_editor_set_props()
 *   - The @a complete argument must be TRUE if no more calls will follow on
 *     the same path. @a complete must always be TRUE for directories.
 *   - If @a complete is FALSE, and:
 *     - if @a relpath is a file, this must (at some point) be followed by
 *       an svn_editor_set_text() call on the same path.
 *     - if @a relpath is a symlink, this must (at some point) be followed by
 *       an svn_editor_set_target() call on the same path.
 *
 * - svn_editor_set_text() and svn_editor_set_target() must always occur
 *   @b after an svn_editor_set_props() or svn_editor_add_file() call on
 *   the same path, if any.\n
 *   In other words, if there are two calls coming in on the same path, the
 *   first of them has to be either svn_editor_set_props() or
 *   svn_editor_add_file().
 *
 * - svn_editor_delete() must not be used to replace a path -- i.e.
 *   svn_editor_delete() must not be followed by an svn_editor_add_*() on
 *   the same path, nor by an svn_editor_copy() or svn_editor_move() with
 *   the same path as the copy/move target.
 *   Instead of a prior delete call, the add/copy/move callbacks should be
 *   called with the @a replaces_rev argument set to the revision number of
 *   the node at this path that is being replaced.  Note that the path and
 *   revision number are the key to finding any other information about the
 *   replaced node, like node kind, etc.
 *   @todo say which function(s) to use.
 *
 * - svn_editor_delete() must not be used to move a path -- i.e.
 *   svn_editor_delete() must not delete the source path of a previous
 *   svn_editor_copy() call. Instead, svn_editor_move() must be used.
 *
 * - One of svn_editor_complete() or svn_editor_abort() must be called
 *   exactly once, which must be the final call the driver invokes.
 *   Invoking svn_editor_complete() must imply that the set of changes has
 *   been transmitted completely and without errors, and invoking
 *   svn_editor_abort() must imply that the transformation was not completed
 *   successfully.
 *
 * - If any callback invocation returns with an error, the driver must
 *   invoke svn_editor_abort() and stop transmitting operations.
 * \n\n
 *
 * <h3>Receiving Restrictions</h3>
 * All callbacks must complete their handling of a path before they
 * return, except for the following pairs, where a change must be completed
 * when receiving the second callback in each pair:
 *  - svn_editor_add_file() and svn_editor_set_text()
 *  - svn_editor_set_props() (if @a complete is FALSE) and
 *    svn_editor_set_text() (if the node is a file)
 *  - svn_editor_set_props() (if @a complete is FALSE) and
 *    svn_editor_set_target() (if the node is a symbolic link)
 *
 * This restriction is not recursive -- a directory's children may remain
 * incomplete until later callback calls are received.
 *
 * For example, an svn_editor_add_directory() call during an 'update'
 * operation will create the directory itself, including its properties,
 * and will complete any client notification for the directory itself.
 * The immediate children of the added directory, given in @a children,
 * will be recorded in the WC as 'incomplete' and will be completed in the
 * course of the same operation sequence, when the corresponding callbacks
 * for these items are invoked.
 * \n\n
 *
 * <h3>Paths</h3>
 * Each driver/receiver implementation of this editor interface must
 * establish the expected root path for the paths sent and received via the
 * callbacks' @a relpath arguments.
 *
 * For example, during an "update", the driver has a working copy checked
 * out at a specific repository URL. The receiver sees the repository as a
 * whole. Here, the receiver could tell the driver which repository
 * URL the working copy refers to, and thus the driver could send
 * @a relpath arguments that are relative to the receiver's working copy.
 * \n\n
 *
 * <h3>Pool Usage</h3>
 * The @a result_pool passed to svn_editor_create() is used to allocate
 * the #svn_editor_t instance, and thus it must not be cleared before the
 * driver has finished driving the editor.
 *
 * The @a scratch_pool passed to each callback invocation is derived from
 * the @a result_pool that was passed to svn_editor_create(). It is
 * cleared directly after each single callback invocation.
 * To allocate memory with a longer lifetime from within a callback
 * function, you may use your own pool kept in the @a editor_baton.
 *
 * The @a scratch_pool passed to svn_editor_create() may be used to help
 * during construction of the #svn_editor_t instance, but it is assumed to
 * live only until svn_editor_create() returns.
 * \n\n
 *
 * <h3>Cancellation</h3>
 * To allow graceful interruption by external events (like a user abort),
 * svn_editor_create() can be passed an #svn_cancel_func_t that is
 * polled every time the driver invokes a callback, just before the
 * actual editor callback implementation is invoked.  If this function
 * decides to return with an error, the driver will receive this error
 * as if the callback function had returned it, i.e. as the result from
 * calling any of the driving functions (e.g. svn_editor_add_directory()).
 * As with any other error, the driver must then invoke svn_editor_abort()
 * and abort the transformation sequence. See #svn_cancel_func_t.
 *
 * The @a cancel_baton argument to svn_editor_create() is passed
 * unchanged to each poll of @a cancel_func.
 *
 * The cancellation function and baton are typically provided by the client
 * context.
 *
 *
 * ### TODO @todo anything missing? -- allow text and prop change to follow
 * a move or copy. -- set_text() vs. apply_text_delta()? -- If a
 * set_props/set_text/set_target/copy/move/delete in a merge source is
 * applied to a different branch, which side will REVISION arguments reflect
 * and is there still a problem?
 *
 * @since New in 1.7.
 */
typedef struct svn_editor_t svn_editor_t;


/** These function types define the callback functions a tree delta consumer
 * implements.
 *
 * Each of these "receiving" function types matches a "driving" function,
 * which has the same arguments with these differences:
 *
 * - These "receiving" functions have a @a baton argument, which is the
 *   @a editor_baton originally passed to svn_editor_create(), as well as
 *   a @a scratch_pool argument.
 *
 * - The "driving" functions have an #svn_editor_t* argument, in order to
 *   call the implementations of the function types defined here that are
 *   registered with the given #svn_editor_t instance.
 *
 * Note that any remaining arguments for these function types are explained
 * in the comment for the "driving" functions. Each function type links to
 * its corresponding "driver".
 *
 * @see svn_editor_t, svn_editor_cb_many_t.
 *
 * @defgroup svn_editor_callbacks Editor callback definitions
 * @{
 */

/** @see svn_editor_add_directory(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_add_directory_t)(
  void *baton,
  const char *relpath,
  const apr_array_header_t *children,
  apr_hash_t *props,
  svn_revnum_t replaces_rev,
  apr_pool_t *scratch_pool);

/** @see svn_editor_add_file(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_add_file_t)(
  void *baton,
  const char *relpath,
  apr_hash_t *props,
  svn_revnum_t replaces_rev,
  apr_pool_t *scratch_pool);

/** @see svn_editor_add_symlink(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_add_symlink_t)(
  void *baton,
  const char *relpath,
  const char *target,
  apr_hash_t *props,
  svn_revnum_t replaces_rev,
  apr_pool_t *scratch_pool);

/** @see svn_editor_add_absent(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_add_absent_t)(
  void *baton,
  const char *relpath,
  svn_node_kind_t kind,
  svn_revnum_t replaces_rev,
  apr_pool_t *scratch_pool);

/** @see svn_editor_set_props(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_set_props_t)(
  void *baton,
  const char *relpath,
  svn_revnum_t revision,
  apr_hash_t *props,
  svn_boolean_t complete,
  apr_pool_t *scratch_pool);

/** @see svn_editor_set_text(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_set_text_t)(
  void *baton,
  const char *relpath,
  svn_revnum_t revision,
  const svn_checksum_t *checksum,
  svn_stream_t *contents,
  apr_pool_t *scratch_pool);

/** @see svn_editor_set_target(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_set_target_t)(
  void *baton,
  const char *relpath,
  svn_revnum_t revision,
  const char *target,
  apr_pool_t *scratch_pool);

/** @see svn_editor_delete(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_delete_t)(
  void *baton,
  const char *relpath,
  svn_revnum_t revision,
  apr_pool_t *scratch_pool);

/** @see svn_editor_copy(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_copy_t)(
  void *baton,
  const char *src_relpath,
  svn_revnum_t src_revision,
  const char *dst_relpath,
  svn_revnum_t replaces_rev,
  apr_pool_t *scratch_pool);

/** @see svn_editor_move(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_move_t)(
  void *baton,
  const char *src_relpath,
  svn_revnum_t src_revision,
  const char *dst_relpath,
  svn_revnum_t replaces_rev,
  apr_pool_t *scratch_pool);

/** @see svn_editor_complete(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_complete_t)(
  void *baton,
  apr_pool_t *scratch_pool);

/** @see svn_editor_abort(), svn_editor_t.
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_editor_cb_abort_t)(
  void *baton,
  apr_pool_t *scratch_pool);

/** @} */


/** These functions create an editor instance so that it can be driven.
 *
 * @defgroup svn_editor_create Editor creation
 * @{
 */

/** Allocate an #svn_editor_t instance from @a result_pool, store
 * @a editor_baton, @a cancel_func and @a cancel_baton in the new instance
 * and return it in @a editor.
 * @a scratch_pool is used for temporary allocations (if any). Note that
 * this is NOT the same @a scratch_pool that is passed to callback functions.
 * @see svn_editor_t
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_create(svn_editor_t **editor,
                  void *editor_baton,
                  svn_cancel_func_t cancel_func,
                  void *cancel_baton,
                  apr_pool_t *result_pool,
                  apr_pool_t *scratch_pool);


/** Sets the #svn_editor_cb_add_directory_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_add_directory(svn_editor_t *editor,
                               svn_editor_cb_add_directory_t callback,
                               apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_add_file_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_add_file(svn_editor_t *editor,
                          svn_editor_cb_add_file_t callback,
                          apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_add_symlink_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_add_symlink(svn_editor_t *editor,
                             svn_editor_cb_add_symlink_t callback,
                             apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_add_absent_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_add_absent(svn_editor_t *editor,
                            svn_editor_cb_add_absent_t callback,
                            apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_set_props_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_set_props(svn_editor_t *editor,
                           svn_editor_cb_set_props_t callback,
                           apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_set_text_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_set_text(svn_editor_t *editor,
                          svn_editor_cb_set_text_t callback,
                          apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_set_target_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_set_target(svn_editor_t *editor,
                            svn_editor_cb_set_target_t callback,
                            apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_delete_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_delete(svn_editor_t *editor,
                        svn_editor_cb_delete_t callback,
                        apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_copy_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_copy(svn_editor_t *editor,
                      svn_editor_cb_copy_t callback,
                      apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_move_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_move(svn_editor_t *editor,
                      svn_editor_cb_move_t callback,
                      apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_complete_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_complete(svn_editor_t *editor,
                          svn_editor_cb_complete_t callback,
                          apr_pool_t *scratch_pool);

/** Sets the #svn_editor_cb_abort_t callback in @a editor
 * to @a callback.
 * @a scratch_pool is used for temporary allocations (if any).
 * @see also svn_editor_setcb_many().
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_abort(svn_editor_t *editor,
                       svn_editor_cb_abort_t callback,
                       apr_pool_t *scratch_pool);


/** Lists a complete set of editor callbacks.
 * This is a convenience structure.
 * @see svn_editor_setcb_many(), svn_editor_create(), svn_editor_t.
 * @since New in 1.7.
 */
typedef struct svn_editor_cb_many_t
{
  svn_editor_cb_add_directory_t cb_add_directory;
  svn_editor_cb_add_file_t cb_add_file;
  svn_editor_cb_add_symlink_t cb_add_symlink;
  svn_editor_cb_add_absent_t cb_add_absent;
  svn_editor_cb_set_props_t cb_set_props;
  svn_editor_cb_set_text_t cb_set_text;
  svn_editor_cb_set_target_t cb_set_target;
  svn_editor_cb_delete_t cb_delete;
  svn_editor_cb_copy_t cb_copy;
  svn_editor_cb_move_t cb_move;
  svn_editor_cb_complete_t cb_complete;
  svn_editor_cb_abort_t cb_abort;

} svn_editor_cb_many_t;

/** Sets all the callback functions in @a editor at once, according to the
 * callback functions stored in @a many.
 * @a scratch_pool is used for temporary allocations (if any).
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_setcb_many(svn_editor_t *editor,
                      const svn_editor_cb_many_t *many,
                      apr_pool_t *scratch_pool);

/** @} */


/** These functions are called by the tree delta driver to edit the target.
 *
 * @see svn_editor_t.
 *
 * @defgroup svn_editor_drive Driving the editor
 * @{
 */

/** Drive @a editor's #svn_editor_cb_add_directory_t callback.
 *
 * Create a new directory at @a relpath. The immediate parent of @a relpath
 * is expected to exist.
 *
 * Set the properties of the new directory to @a props, which is an
 * apr_hash_t holding key-value pairs. Each key is a const char* of a
 * property name, each value is a const svn_string_t*. If no properties are
 * being set on the new directory, @a props must be NULL.
 *
 * If this add is expected to replace a previously existing file or
 * directory at @a relpath, the revision number of the node to be replaced
 * must be given in @a replaces_rev. Otherwise, @a replaces_rev must be
 * SVN_INVALID_REVNUM.  Note: it is not allowed to call a "delete" followed
 * by an "add" on the same path. Instead, an "add" with @a replaces_rev set
 * accordingly MUST be used.
 *
 * A complete listing of the immediate children of @a relpath that will be
 * added subsequently is given in @a children. @a children is an array of
 * const char*s, each giving the basename of an immediate child.
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 */
svn_error_t *
svn_editor_add_directory(svn_editor_t *editor,
                         const char *relpath,
                         const apr_array_header_t *children,
                         apr_hash_t *props,
                         svn_revnum_t replaces_rev);

/** Drive @a editor's #svn_editor_cb_add_file_t callback.
 *
 * Create a new file at @a relpath. The immediate parent of @a relpath
 * is expected to exist.
 *
 * Set the properties of the new file to @a props, which is an
 * apr_hash_t holding key-value pairs. Each key is a const char* of a
 * property name, each value is a const svn_string_t*. If no properties are
 * being set on the new file, @a props must be NULL.
 *
 * If this add is expected to replace a previously existing file or
 * directory at @a relpath, the revision number of the node to be replaced
 * must be given in @a replaces_rev. Otherwise, @a replaces_rev must be
 * SVN_INVALID_REVNUM.  Note: it is not allowed to call a "delete" followed
 * by an "add" on the same path. Instead, an "add" with @a replaces_rev set
 * accordingly MUST be used.
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_add_file(svn_editor_t *editor,
                    const char *relpath,
                    apr_hash_t *props,
                    svn_revnum_t replaces_rev);

/** Drive @a editor's #svn_editor_cb_add_symlink_t callback.
 *
 * Create a new symbolic link at @a relpath, with a link target of @a
 * target. The immediate parent of @a relpath is expected to exist.
 *
 * For descriptions of @a props and @a replaces_rev, see
 * svn_editor_add_file().
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_add_symlink(svn_editor_t *editor,
                       const char *relpath,
                       const char *target,
                       apr_hash_t *props,
                       svn_revnum_t replaces_rev);

/** Drive @a editor's #svn_editor_cb_add_absent_t callback.
 *
 * Create an "absent" node of kind @a kind at @a relpath. The immediate
 * parent of @a relpath is expected to exist.
 * ### TODO @todo explain "absent".
 *
 * For a description of @a replaces_rev, see svn_editor_add_file().
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_add_absent(svn_editor_t *editor,
                      const char *relpath,
                      svn_node_kind_t kind,
                      svn_revnum_t replaces_rev);

/** Drive @a editor's #svn_editor_cb_set_props_t callback.
 *
 * Set or change properties on the existing node at @a relpath.
 * ### TODO @todo Does this send *all* properties, always?
 * ### TODO @todo What is REVISION for?
 * ### what about "entry props"? will these still be handled via
 * ### the general prop function?
 *
 * @a complete must be FALSE if and only if
 * - @a relpath is a file and an svn_editor_set_text() call will follow on
 *   the same path, or
 * - @a relpath is a symbolic link and an svn_editor_set_target() call will
 *   follow on the same path.
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_set_props(svn_editor_t *editor,
                     const char *relpath,
                     svn_revnum_t revision,
                     apr_hash_t *props,
                     svn_boolean_t complete);

/** Drive @a editor's #svn_editor_cb_set_text_t callback.
 *
 * Set/change the text content of a file at @a relpath to @a contents
 * with checksum @a checksum.
 * ### TODO @todo Does this send the *complete* content, always?
 * ### TODO @todo What is REVISION for?
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_set_text(svn_editor_t *editor,
                    const char *relpath,
                    svn_revnum_t revision,
                    const svn_checksum_t *checksum,
                    svn_stream_t *contents);

/** Drive @a editor's #svn_editor_cb_set_target_t callback.
 *
 * Set/change the link target that a symbolic link at @a relpath points at
 * to @a target.
 * ### TODO @todo What is REVISION for?
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_set_target(svn_editor_t *editor,
                      const char *relpath,
                      svn_revnum_t revision,
                      const char *target);

/** Drive @a editor's #svn_editor_cb_delete_t callback.
 *
 * Delete the existing node at @a relpath, expected to be identical to
 * revision @a revision of that path.
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_delete(svn_editor_t *editor,
                  const char *relpath,
                  svn_revnum_t revision);

/** Drive @a editor's #svn_editor_cb_copy_t callback.
 *
 * Copy the node at @a src_relpath, expected to be identical to revision @a
 * src_revision of that path, to @a dst_relpath.
 *
 * For a description of @a replaces_rev, see svn_editor_add_file().
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_copy(svn_editor_t *editor,
                const char *src_relpath,
                svn_revnum_t src_revision,
                const char *dst_relpath,
                svn_revnum_t replaces_rev);

/** Drive @a editor's #svn_editor_cb_move_t callback.
 * Move the node at @a src_relpath, expected to be identical to revision @a
 * src_revision of that path, to @a dst_relpath.
 *
 * For a description of @a replaces_rev, see svn_editor_add_file().
 *
 * ### stsp: How would I describe a merge of revision range rA-rB,
 * ###   within which a file foo.c was delete in rN, re-created in rM,
 * ###   and then renamed to bar.c in rX?
 * ###   Would the following be valid?
 * ###   svn_editor_add_file(ed, "foo.c", props, rN);
 * ###   svn_editor_move(ed, "foo.c", rM, "bar.c", rN);
 * ###
 * ### gstein: no, it would be:
 * ###   svn_editor_delete(e, "foo.c", rN);
 * ###   svn_editor_add_file(e, "foo.c", props, SVN_INVALID_REVNUM);
 * ###   svn_editor_move(e, "foo.c", rM, "bar.c", SVN_INVALID_REVNUM);
 * ###
 * ###   replaces_rev is to indicate a deletion of the destination node
 * ###   that occurs as part of the move. there are no replacements in
 * ###   your example.
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_move(svn_editor_t *editor,
                const char *src_relpath,
                svn_revnum_t src_revision,
                const char *dst_relpath,
                svn_revnum_t replaces_rev);

/** Drive @a editor's #svn_editor_cb_complete_t callback.
 *
 * Send word that the tree delta has been completed successfully.
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_complete(svn_editor_t *editor);

/** Drive @a editor's #svn_editor_cb_abort_t callback.
 *
 * Notify that the tree delta transmission was not successful.
 * ### TODO @todo Shouldn't we add a reason-for-aborting argument?
 *
 * For all restrictions on driving the editor, see #svn_editor_t.
 * @since New in 1.7.
 */
svn_error_t *
svn_editor_abort(svn_editor_t *editor);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_EDITOR_H */
