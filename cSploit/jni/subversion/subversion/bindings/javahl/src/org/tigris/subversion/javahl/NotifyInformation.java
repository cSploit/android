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

import java.util.EventObject;

/**
 * The event passed to the {@link Notify2#onNotify(NotifyInformation)}
 * API to notify {@link SVNClientInterface} of relevant events.
 *
 * @since 1.2
 */
public class NotifyInformation extends EventObject
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
     * The {@link NotifyAction} which triggered this event.
     */
    private int action;

    /**
     * The {@link NodeKind} of the item.
     */
    private int kind;

    /**
     * The MIME type of the item.
     */
    private String mimeType;

    /**
     * Any lock for the item.
     */
    private Lock lock;

    /**
     * Any error message for the item.
     */
    private String errMsg;

    /**
     * The {@link NotifyStatus} of the content of the item.
     */
    private int contentState;

    /**
     * The {@link NotifyStatus} of the properties of the item.
     */
    private int propState;

    /**
     * The {@link LockStatus} of the lock of the item.
     */
    private int lockState;

    /**
     * The revision of the item.
     */
    private long revision;

    /**
     * The name of the changelist.
     * @since 1.5
     */
    private String changelistName;

    /**
     * The range of the merge just beginning to occur.
     * @since 1.5
     */
    private RevisionRange mergeRange;

    /**
     * A common absolute path prefix that can be subtracted from .path.
     * @since 1.6
     */
    private String pathPrefix;

    /**
     * This constructor is to be used by the native code.
     *
     * @param path The path of the item, which is the source of the event.
     * @param action The {@link NotifyAction} which triggered this event.
     * @param kind The {@link NodeKind} of the item.
     * @param mimeType The MIME type of the item.
     * @param lock Any lock for the item.
     * @param errMsg Any error message for the item.
     * @param contentState The {@link NotifyStatus} of the content of
     * the item.
     * @param propState The {@link NotifyStatus} of the properties of
     * the item.
     * @param lockState The {@link LockStatus} of the lock of the item.
     * @param revision The revision of the item.
     * @param changelistName The name of the changelist.
     * @param mergeRange The range of the merge just beginning to occur.
     * @param pathPrefix A common path prefix.
     */
    NotifyInformation(String path, int action, int kind, String mimeType,
                      Lock lock, String errMsg, int contentState,
                      int propState, int lockState, long revision,
                      String changelistName, RevisionRange mergeRange,
                      String pathPrefix)
    {
        super(path == null ? "" : path);
        this.action = action;
        this.kind = kind;
        this.mimeType = mimeType;
        this.lock = lock;
        this.errMsg = errMsg;
        this.contentState = contentState;
        this.propState = propState;
        this.lockState = lockState;
        this.revision = revision;
        this.changelistName = changelistName;
        this.mergeRange = mergeRange;
        this.pathPrefix = pathPrefix;
    }

    /**
     * A backward-compat callback.
     */
    public NotifyInformation(
                        org.apache.subversion.javahl.ClientNotifyInformation aInfo)
    {
        this(aInfo.getPath(),
             fromAAction(aInfo.getAction()),
             NodeKind.fromApache(aInfo.getKind()), aInfo.getMimeType(),
             aInfo.getLock() == null ? null : new Lock(aInfo.getLock()),
             aInfo.getErrMsg(), fromAStatus(aInfo.getContentState()),
             fromAStatus(aInfo.getPropState()),
             aInfo.getLockState().ordinal(), aInfo.getRevision(),
             aInfo.getChangelistName(),
             aInfo.getMergeRange() == null ? null
                : new RevisionRange(aInfo.getMergeRange()),
             aInfo.getPathPrefix());
    }

    /**
     * @return The path of the item, which is the source of the event.
     */
    public String getPath()
    {
        return (String) super.source;
    }

    /**
     * @return The {@link NotifyAction} which triggered this event.
     */
    public int getAction()
    {
        return action;
    }

    /**
     * @return The {@link NodeKind} of the item.
     */
    public int getKind()
    {
        return kind;
    }

    /**
     * @return The MIME type of the item.
     */
    public String getMimeType()
    {
        return mimeType;
    }

    /**
     * @return Any lock for the item.
     */
    public Lock getLock()
    {
        return lock;
    }

    /**
     * @return Any error message for the item.
     */
    public String getErrMsg()
    {
        return errMsg;
    }

    /**
     * @return The {@link NotifyStatus} of the content of the item.
     */
    public int getContentState()
    {
        return contentState;
    }

    /**
     * @return The {@link NotifyStatus} of the properties of the item.
     */
    public int getPropState()
    {
        return propState;
    }

    /**
     * @return The {@link LockStatus} of the lock of the item.
     */
    public int getLockState()
    {
        return lockState;
    }

    /**
     * @return The revision of the item.
     */
    public long getRevision()
    {
        return revision;
    }

    /**
     * @return The name of the changelist.
     * @since 1.5
     */
    public String getChangelistName()
    {
        return changelistName;
    }

    /**
     * @return The range of the merge just beginning to occur.
     * @since 1.5
     */
    public RevisionRange getMergeRange()
    {
        return mergeRange;
    }

    /**
     * @return The common absolute path prefix.
     * @since 1.6
     */
    public String getPathPrefix()
    {
        return pathPrefix;
    }

    private static int
    fromAStatus(org.apache.subversion.javahl.ClientNotifyInformation.Status aStatus)
    {
        switch(aStatus)
        {
        default:
        case inapplicable:
            return NotifyStatus.inapplicable;
        case unknown:
            return NotifyStatus.unknown;
        case unchanged:
            return NotifyStatus.unchanged;
        case missing:
            return NotifyStatus.missing;
        case obstructed:
            return NotifyStatus.obstructed;
        case changed:
            return NotifyStatus.changed;
        case merged:
            return NotifyStatus.merged;
        case conflicted:
            return NotifyStatus.conflicted;
        }
    }

    private static int
    fromAAction(org.apache.subversion.javahl.ClientNotifyInformation.Action aAction)
    {
        if (aAction == null)
            return -1;

        int order = aAction.ordinal();

        /* The new class adds an item after changelist_clear, so adjust
           accordingly. */
        if (order < NotifyAction.changelist_clear)
            return order;
        else
            return order - 1;
    }
}
