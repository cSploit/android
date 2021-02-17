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

package org.tigris.subversion.javahl;

/**
 * The type of action triggering the notification
 */
public interface NotifyAction
{
    /** Adding a path to revision control. */
    public static final int add = 0;

    /** Copying a versioned path. */
    public static final int copy = 1;

    /** Deleting a versioned path. */
    public static final int delete =2;

    /** Restoring a missing path from the pristine text-base. */
    public static final int restore = 3;

    /** Reverting a modified path. */
    public static final int revert = 4;

    /** A revert operation has failed. */
    public static final int failed_revert = 5;

    /** Resolving a conflict. */
    public static final int resolved = 6;

    /** Skipping a path. */
    public static final int skip = 7;

    /* The update actions are also used for checkouts, switches, and merges. */

    /** Got a delete in an update. */
    public static final int update_delete = 8;

    /** Got an add in an update. */
    public static final int update_add = 9;

    /** Got any other action in an update. */
    public static final int update_update = 10;

    /** The last notification in an update */
    public static final int update_completed = 11;

    /** About to update an external module, use for checkouts and switches too,
     * end with @c svn_wc_update_completed.
     */
    public static final int update_external = 12;

    /** The last notification in a status (including status on externals). */
    public static final int status_completed = 13;

    /** Running status on an external module. */
    public static final int status_external = 14;


    /** Committing a modification. */
    public static final int commit_modified = 15;

    /** Committing an addition. */
    public static final int commit_added = 16;

    /** Committing a deletion. */
    public static final int commit_deleted = 17;

    /** Committing a replacement. */
    public static final int commit_replaced = 18;

    /** Transmitting post-fix text-delta data for a file. */
    public static final int commit_postfix_txdelta = 19;

    /** Processed a single revision's blame. */
    public static final int blame_revision = 20;

    /**
     * @since 1.2
     * Locking a path
     */
    public static final int locked = 21;

    /**
     * @since 1.2
     * Unlocking a path
     */
    public static final int unlocked = 22;

    /**
     * @since 1.2
     * Failed to lock a path
     */
    public static final int failed_lock = 23;

    /**
     * @since 1.2
     * Failed to unlock a path
     */
    public static final int failed_unlock = 24;

    /**
     * @since 1.5
     * Tried adding a path that already exists.
     */
    public static final int exists = 25;

    /**
     * @since 1.5
     * Set the changelist for a path.
     */
    public static final int changelist_set = 26;

    /**
     * @since 1.5
     * Clear the changelist for a path.
     */
    public static final int changelist_clear = 27;

    /**
     * @since 1.5
     * A merge operation has begun.
     */
    public static final int merge_begin = 28;

    /**
     * @since 1.5
     * A merge operation from a foreign repository has begun.
     */
   public static final int foreign_merge_begin = 29;

    /**
     * @since 1.5
     * Got a replaced in an update.
     */
    public static final int update_replaced = 30;

    /**
     * @since 1.6
     * Property added.
     */
    public static final int property_added = 31;

    /**
     * @since 1.6
     * Property modified.
     */
    public static final int property_modified = 32;

    /**
     * @since 1.6
     * Property deleted.
     */
    public static final int property_deleted = 33;

    /**
     * @since 1.6
     * Property delete nonexistent.
     */
    public static final int property_deleted_nonexistent = 34;

    /**
     * @since 1.6
     * Revision property set.
     */
    public static final int revprop_set = 35;

    /**
     * @since 1.6
     * Revision property deleted.
     */
    public static final int revprop_deleted = 36;

    /**
     * @since 1.6
     * The last notification in a merge
     */
    public static final int merge_completed = 37;

    /**
     * @since 1.6
     * The path is a tree-conflict victim of the intended action
     */
    public static final int tree_conflict = 38;

    /**
     * textual representation of the action types
     */
    public static final String[] actionNames =
    {
        "add",
        "copy",
        "delete",
        "restore",
        "revert",
        "failed revert",
        "resolved",
        "skip",
        "update delete",
        "update add",
        "update modified",
        "update completed",
        "update external",
        "status completed",
        "status external",
        "sending modified",
        "sending added   ",
        "sending deleted ",
        "sending replaced",
        "transfer",
        "blame revision processed",
        "locked",
        "unlocked",
        "locking failed",
        "unlocking failed",
        "path exists",
        "changelist set",
        "changelist cleared",
        "merge begin",
        "foreign merge begin",
        "replaced",
        "property added",
        "property modified",
        "property deleted",
        "nonexistent property deleted",
        "revprop set",
        "revprop deleted",
        "merge completed",
        "tree conflict"
    };
}
