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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;

/**
 * Merge history for a path.
 *
 * @since 1.5
 */
public class Mergeinfo implements java.io.Serializable
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
     * A mapping of repository-relative paths to a list of revision
     * ranges.
     */
    private Map<String, List<RevisionRange>> mergeSources;

    public Mergeinfo()
    {
        mergeSources = new HashMap<String, List<RevisionRange>>();
    }

    /**
     * Create and populate an instance using the contents of the
     * <code>svn:mergeinfo</code> property.
     * @param mergeinfo <code>svn:mergeinfo</code> property value.
     */
    public Mergeinfo(String mergeinfo)
    {
        this();
        this.loadFromMergeinfoProperty(mergeinfo);
    }

    /**
     * A constructor for backward compat.
     */
    public Mergeinfo(org.apache.subversion.javahl.types.Mergeinfo aMergeinfo)
    {
        this();
        Set<String> srcPaths = aMergeinfo.getPaths();

        for (String srcPath : srcPaths)
        {
            List<org.apache.subversion.javahl.types.RevisionRange> aRanges =
                                    aMergeinfo.getRevisionRange(srcPath);
            List<RevisionRange> list = new ArrayList<RevisionRange>();

            for (org.apache.subversion.javahl.types.RevisionRange range : aRanges)
            {
                list.add(new RevisionRange(range));
            }
            mergeSources.put(srcPath, list);
        }
    }

    /**
     * Add one or more RevisionRange objects to merge info. If the
     * merge source is already stored, the list of revisions is
     * replaced.
     * @param mergeSrc The merge source URL.
     * @param ranges RevisionRange objects to add.
     * @throws SubversionException If range list contains objects of
     * type other than RevisionRange.
     */
    public void addRevisions(String mergeSrc, RevisionRange[] ranges)
    {
        for (int i = 0; i < ranges.length; i++)
            addRevisionRange(mergeSrc, ranges[i]);
    }

    /**
     * Add a revision range to the merged revisions for a path.  If
     * the merge source already has associated revision ranges, add
     * the revision range to the existing list.
     * @param mergeSrc The merge source URL.
     * @param range The revision range to add.
     */
    public void addRevisionRange(String mergeSrc, RevisionRange range)
    {
        List<RevisionRange> revisions = this.getRevisions(mergeSrc);
        if (revisions == null)
            revisions = new ArrayList<RevisionRange>();
        revisions.add(range);
        this.setRevisionList(mergeSrc, revisions);
    }

    /**
     * Get the merge source URLs.
     * @return The merge source URLs.
     */
    public String[] getPaths()
    {
        Set<String> pathSet = mergeSources.keySet();
        if (pathSet == null)
            return null;
        return pathSet.toArray(new String[pathSet.size()]);
    }

    /**
     * Get the revision ranges for the specified merge source URL.
     * @param mergeSrc The merge source URL, or <code>null</code>.
     * @return List of RevisionRange objects, or <code>null</code>.
     */
    public List<RevisionRange> getRevisions(String mergeSrc)
    {
        if (mergeSrc == null)
            return null;
        return mergeSources.get(mergeSrc);
    }

    /**
     * Get the RevisionRange objects for the specified merge source URL
     * @param mergeSrc The merge source URL, or <code>null</code>.
     * @return Array of RevisionRange objects, or <code>null</code>.
     */
    public RevisionRange[] getRevisionRange(String mergeSrc)
    {
        List<RevisionRange> revisions = this.getRevisions(mergeSrc);
        if (revisions == null)
            return null;
        return revisions.toArray(new RevisionRange[revisions.size()]);
    }

    /**
     * Parse the <code>svn:mergeinfo</code> property to populate the
     * merge source URLs and revision ranges of this instance.
     * @param mergeinfo <code>svn:mergeinfo</code> property value.
     */
    public void loadFromMergeinfoProperty(String mergeinfo)
    {
        if (mergeinfo == null)
            return;
        StringTokenizer st = new StringTokenizer(mergeinfo, "\n");
        while (st.hasMoreTokens())
        {
            parseMergeinfoLine(st.nextToken());
        }
    }

    /**
     * Parse a merge source line from a <code>svn:mergeinfo</code>
     * property value (e.g.
     * <code>"/trunk:1-100,104,108,110-115"</code>).
     *
     * @param line A line of merge info for a single merge source.
     */
    private void parseMergeinfoLine(String line)
    {
        int colon = line.indexOf(':');
        if (colon > 0)
        {
            String pathElement = line.substring(0, colon);
            String revisions = line.substring(colon + 1);
            parseRevisions(pathElement, revisions);
        }
    }

    /**
     * Parse the revisions in a merge info line into RevisionRange
     * objects and adds each of them to the internal Map
     * (e.g. <code>"1-100,104,108,110-115"</code>)
     *
     * @param path The merge source path.
     * @param revisions A textual representation of the revision ranges.
     */
    private void parseRevisions(String path, String revisions)
    {
        List<RevisionRange> rangeList = this.getRevisions(path);
        StringTokenizer st = new StringTokenizer(revisions, ",");
        while (st.hasMoreTokens())
        {
            String revisionElement = st.nextToken();
            RevisionRange range = new RevisionRange(revisionElement);
            if (rangeList == null)
                rangeList = new ArrayList<RevisionRange>();
            rangeList.add(range);
        }
        if (rangeList != null)
            setRevisionList(path, rangeList);
    }


    /**
     * Add the List object to the map.  This method is only
     * used internally where we know that List contains a
     * type-safe set of RevisionRange objects.
     * @param mergeSrc The merge source URL.
     * @param range List of RevisionRange objects to add.
     */
    private void setRevisionList(String mergeSrc, List<RevisionRange> range)
    {
        mergeSources.put(mergeSrc, range);
    }
}
