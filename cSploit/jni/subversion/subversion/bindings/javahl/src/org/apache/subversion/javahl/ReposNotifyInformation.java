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
 */

package org.apache.subversion.javahl;

import java.util.EventObject;
import org.apache.subversion.javahl.callback.ReposNotifyCallback;

/**
 * The event passed to the {@link ReposNotifyCallback#onNotify}
 * API to notify {@link ISVNClient} of relevant events.
 */
public class ReposNotifyInformation extends EventObject
{
    // Update the serialVersionUID when there is a incompatible change
    // made to this class.  See any of the following, depending upon
    // the Java release.
    // http://java.sun.com/j2se/1.3/docs/guide/serialization/spec/version.doc7.html
    // http://java.sun.com/j2se/1.4/pdf/serial-spec.pdf
    // http://java.sun.com/j2se/1.5.0/docs/guide/serialization/spec/version.html#6678
    // http://java.sun.com/javase/6/docs/platform/serialization/spec/version.html#6678
    private static final long serialVersionUID = 1L;

    /**
     * The {@link Action} which triggered this event.
     */
    private Action action;

    /**
     * The revision of the item.
     */
    private long revision;

    /**
     * The warning text.
     */
    private String warning;

    private long shard;

    private long newRevision;

    private long oldRevision;

    private NodeAction nodeAction;

    private String path;

    /**
     * This constructor is to be used by the native code.
     *
     * @param action The {@link Action} which triggered this event.
     * @param revision potentially the revision.
     */
    public ReposNotifyInformation(Action action, long revision, String warning,
                                  long shard, long newRevision,
                                  long oldRevision, NodeAction nodeAction,
                                  String path)
    {
        super(action);
        this.action = action;
        this.revision = revision;
        this.warning = warning;
        this.shard = shard;
        this.newRevision = newRevision;
        this.oldRevision = oldRevision;
        this.nodeAction = nodeAction;
        this.path = path;
    }

    /**
     * @return The {@link Action} which triggered this event.
     */
    public Action getAction()
    {
        return action;
    }

    /**
     * @return The revision for the item.
     */
    public long getRevision()
    {
        return revision;
    }

    /**
     * @return The warning text.
     */
    public String getWarning()
    {
        return warning;
    }

    public long getShard()
    {
       return shard;
    }

    public long getNewRevision()
    {
       return newRevision;
    }

    public long getOldRevision()
    {
       return oldRevision;
    }

    public NodeAction getNodeAction()
    {
       return nodeAction;
    }

    public String getPath()
    {
       return path;
    }

    /**
     * The type of action triggering the notification
     */
    public enum Action
    {
        /** A warning message is waiting. */
        warning,

        /** A revision has finished being dumped. */
        dump_rev_end,

        /** A revision has finished being verified. */
        verify_rev_end,

        /** packing of an FSFS shard has commenced */
        pack_shard_start,

        /** packing of an FSFS shard is completed */
        pack_shard_end,

        /** packing of the shard revprops has commenced */
        pack_shard_start_revprop,

        /** packing of the shard revprops has completed */
        pack_shard_end_revprop,

        /** A revision has begun loading */
        load_txn_start,

        /** A revision has finished loading */
        load_txn_committed,

        /** A node has begun loading */
        load_node_start,

        /** A node has finished loading */
        load_node_done,

        /** A copied node has been encountered */
        load_copied_node,

        /** Mergeinfo has been normalized */
        load_normalized_mergeinfo,

        /** The operation has acquired a mutex for the repo. */
        mutex_acquired,

        /** Recover has started. */
        recover_start,

        /** Upgrade has started. */
        upgrade_start;
    }

    public enum NodeAction
    {
         change,
         add,
         deleted,
         replace;
    }
}
