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

import java.io.*;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Date;

import junit.framework.Assert;
/**
 * This class describe the expected state of the working copy
 */
public class WC
{
    /**
     * the map of the items of the working copy. The relative path is the key
     * for the map
     */
    Map items = new HashMap();

    /**
     * Generate from the expected state of the working copy a new working copy
     * @param root      the working copy directory
     * @throws IOException
     */
    public void materialize(File root) throws IOException
    {
        // generate all directories first
        Iterator it = items.values().iterator();
        while (it.hasNext())
        {
            Item item = (Item) it.next();
            if (item.myContent == null) // is a directory
            {
                File dir = new File(root, item.myPath);
                if (!dir.exists())
                    dir.mkdirs();
            }
        }
        // generate all files with the content in the second run
        it = items.values().iterator();
        while (it.hasNext())
        {
            Item item = (Item) it.next();
            if (item.myContent != null) // is a file
            {
                File file = new File(root, item.myPath);
                PrintWriter pw = new PrintWriter(new FileOutputStream(file));
                pw.print(item.myContent);
                pw.close();
            }
        }
    }
    /**
     * Add a new item to the working copy
     * @param path      the path of the item
     * @param content   the content of the item. A null content signifies a
     *                  directory
     * @return          the new Item object
     */
    public Item addItem(String path, String content)
    {
        return new Item(path, content);
    }

    /**
     * Returns the item at a path
     * @param path  the path, where the item is searched
     * @return  the found item
     */
    public Item getItem(String path)
    {
        return (Item) items.get(path);
    }

    /**
     * Remove the item at a path
     * @param path  the path, where the item is removed
     */
    public void removeItem(String path)
    {
        items.remove(path);
    }

    /**
     * Set text (content) status of the item at a path
     * @param path      the path, where the status is set
     * @param status    the new text status
     */
    public void setItemTextStatus(String path, int status)
    {
        ((Item) items.get(path)).textStatus = status;
    }

    /**
     * Set property status of the item at a path
     * @param path      the path, where the status is set
     * @param status    the new property status
     */
    public void setItemPropStatus(String path, int status)
    {
        ((Item) items.get(path)).propStatus = status;
    }

    /**
     * Set the revision number of the item at a path
     * @param path      the path, where the revision number is set
     * @param revision  the new revision number
     */
    public void setItemWorkingCopyRevision(String path, long revision)
    {
        ((Item) items.get(path)).workingCopyRev = revision;
    }

    /**
     * Set the revision number of all paths in a working copy.
     * @param revision The revision number to associate with all
     * paths.
     */
    public void setRevision(long revision)
    {
        Iterator iter = this.items.values().iterator();

        while (iter.hasNext())
        {
            Item item = (Item) iter.next();
            item.workingCopyRev = revision;
        }
    }

    /**
     * Returns the file content of the item at a path
     * @param path  the path, where the content is retrieved
     * @return  the content of the file
     */
    public String getItemContent(String path)
    {
        return ((Item) items.get(path)).myContent;
    }

    /**
     * Set the file content of the item at a path
     * @param path      the path, where the content is set
     * @param content   the new content
     */
    public void setItemContent(String path, String content)
    {
        // since having no content signals a directory, changes of removing the
        // content or setting a former not set content is not allowed. That
        // would change the type of the item.
        Assert.assertNotNull("cannot unset content", content);
        Item i = (Item) items.get(path);
        Assert.assertNotNull("cannot set content on directory", i.myContent);
        i.myContent = content;
    }

    /**
     * set the flag to check the content of item at a path during next check.
     * @param path      the path, where the flag is set
     * @param check     the flag
     */
    public void setItemCheckContent(String path, boolean check)
    {
        Item i = (Item) items.get(path);
        i.checkContent = check;
    }

    /**
     * Set the expected node kind at a path
     * @param path      the path, where the node kind is set
     * @param nodeKind  the expected node kind
     */
    public void setItemNodeKind(String path, int nodeKind)
    {
        Item i = (Item) items.get(path);
        i.nodeKind = nodeKind;
    }

    /**
     * Set the expected lock state at a path
     * @param path      the path, where the lock state is set
     * @param isLocked  the flag
     */
    public void setItemIsLocked(String path, boolean isLocked)
    {
        Item i = (Item) items.get(path);
        i.isLocked = isLocked;
    }

    /**
     * Set the expected switched flag at a path
     * @param path          the path, where the switch flag is set
     * @param isSwitched    the flag
     */
    public void setItemIsSwitched(String path, boolean isSwitched)
    {
        Item i = (Item) items.get(path);
        i.isSwitched = isSwitched;
    }

    /**
     * Set the youngest committed revision of an out of date item at
     * <code>path</code>.
     * @param path The path to set the last revision for.
     * @param revision The last revision number for <code>path</code>
     * known to the repository.
     */
    public void setItemReposLastCmtRevision(String path, long revision)
    {
        ((Item) items.get(path)).reposLastCmtRevision = revision;
    }

    /**
     * Set the youngest committed revision of an out of date item at
     * <code>path</code>.
     * @param path The path to set the last author for.
     * @param revision The last author for <code>path</code> known to
     * the repository.
     */
    public void setItemReposLastCmtAuthor(String path, String author)
    {
        ((Item) items.get(path)).reposLastCmtAuthor = author;
    }

    /**
     * Set the youngest committed revision of an out of date item at
     * <code>path</cod>.
     * @param path The path to set the last date for.
     * @param revision The last date for <code>path</code> known to
     * the repository.
     */
    public void setItemReposLastCmtDate(String path, long date)
    {
        ((Item) items.get(path)).reposLastCmtDate = date;
    }

    /**
     * Set the youngest committed node kind of an out of date item at
     * <code>path</code>.
     * @param path The path to set the last node kind for.
     * @param revision The last node kind for <code>path</code> known
     * to the repository.
     */
    public void setItemReposKind(String path, int nodeKind)
    {
        ((Item) items.get(path)).reposKind = nodeKind;
    }

    /**
     * Set the youngest committed rev, author, date, and node kind of an
     * out of date item at <code>path</code>.
     *
     * @param path The path to set the repos info for.
     * @param revision The last revision number.
     * @param revision The last author.
     * @param date The last date.
     * @param nodeKind The last node kind.
     */
    public void setItemOODInfo(String path, long revision, String author,
                               long date, int nodeKind)
    {
        this.setItemReposLastCmtRevision(path, revision);
        this.setItemReposLastCmtAuthor(path, author);
        this.setItemReposLastCmtDate(path, date);
        this.setItemReposKind(path, nodeKind);
    }

    /**
     * Copy an expected working copy state
     * @return the copy of the exiting object
     */
    public WC copy()
    {
        WC c = new WC();
        Iterator it = items.values().iterator();
        while (it.hasNext())
        {
            ((Item) it.next()).copy(c);
        }
        return c;
    }

    /**
     * Check the result of a single file SVNClient.list call
     * @param tested            the result array
     * @param singleFilePath    the path to be checked
     * @throws Exception
     */
    void check(DirEntry[] tested, String singleFilePath)
    {
        Assert.assertEquals("not a single dir entry", 1, tested.length);
        Item item = (Item)items.get(singleFilePath);
        Assert.assertNotNull("not found in working copy", item);
        Assert.assertNotNull("not a file", item.myContent);
        Assert.assertEquals("state says file, working copy not",
                tested[0].getNodeKind(),
                item.nodeKind == -1 ? NodeKind.file : item.nodeKind);
    }

    /**
     * Check the result of a directory SVNClient.list call
     * @param tested        the result array
     * @param basePath      the path of the directory
     * @param recursive     the recursive flag of the call
     * @throws Exception
     */
    void check(DirEntry[] tested, String basePath, boolean recursive)
    {
        // clear the touched flag of all items
        Iterator it = items.values().iterator();
        while (it.hasNext())
        {
            Item item = (Item) it.next();
            item.touched = false;
        }

        // normalize directory path
        if (basePath != null && basePath.length() > 0)
        {
            basePath += '/';
        }
        else
        {
            basePath = "";
        }
        // check all returned DirEntry's
        for (int i = 0; i < tested.length; i++)
        {
            String name = basePath + tested[i].getPath();
            Item item = (Item) items.get(name);
            Assert.assertNotNull("null paths won't be found in working copy",
                                 item);
            if (item.myContent != null)
            {
                Assert.assertEquals("Expected '" + tested[i] + "' to be file",
                        tested[i].getNodeKind(),
                        item.nodeKind == -1 ? NodeKind.file : item.nodeKind);
            }
            else
            {
                Assert.assertEquals("Expected '" + tested[i] + "' to be dir",
                        tested[i].getNodeKind(),
                        item.nodeKind == -1 ? NodeKind.dir : item.nodeKind);
            }
            item.touched = true;
        }

        // all items should have been in items, should had their touched flag
        // set
        it = items.values().iterator();
        while (it.hasNext())
        {
            Item item = (Item) it.next();
            if (!item.touched)
            {
                if (item.myPath.startsWith(basePath) &&
                        !item.myPath.equals(basePath))
                {
                    // Non-recursive checks will fail here.
                    Assert.assertFalse("Expected path '" + item.myPath +
                                       "' not found in dir entries",
                                       recursive);

                    // Look deeper under the tree.
                    boolean found = false;
                    for (int i = 0; i < tested.length; i++)
                    {
                        if (tested[i].getNodeKind() == NodeKind.dir)
                        {
                            if (item.myPath.startsWith(basePath +
                                                       tested[i].getPath()))
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                    Assert.assertTrue("Expected path '" + item.myPath +
                                       "' not found in dir entries", found);
                }
            }
        }
    }

    /**
     * Check the result of a SVNClient.status() versus the expected
     * state.  Does not extract "out of date" information from the
     * repository.
     *
     * @param tested            the result to be tested
     * @param workingCopyPath   the path of the working copy
     * @throws IOException If there is a problem finding or reading
     * the WC.
     * @see #check(Status[], String, boolean)
     */
    void check(Status[] tested, String workingCopyPath)
        throws IOException
    {
        check(tested, workingCopyPath, false);
    }

    /**
     * Check the result of a SVNClient.status() versus the expected state.
     *
     * @param tested The result to be tested.
     * @param workingCopyPath The path of the working copy.
     * @param checkRepos Whether to compare the "out of date" statii.
     * @throws IOException If there is a problem finding or reading
     * the WC.
     */
    void check(Status[] tested, String workingCopyPath, boolean checkRepos)
        throws IOException
    {
        // clear the touched flag of all items
        Iterator it = items.values().iterator();
        while (it.hasNext())
        {
            Item item = (Item) it.next();
            item.touched = false;
        }

        String normalizeWCPath =
                workingCopyPath.replace(File.separatorChar, '/');

        // check all result Staus object
        for (int i = 0; i < tested.length; i++)
        {
            String path = tested[i].getPath();
            Assert.assertTrue("status path starts not with working copy path",
                    path.startsWith(normalizeWCPath));

            // we calculate the relative path to the working copy root
            if (path.length() > workingCopyPath.length() + 1)
            {
                Assert.assertEquals("missing '/' in status path",
                        path.charAt(workingCopyPath.length()), '/');
                path = path.substring(workingCopyPath.length() + 1);
            }
            else
                // this is the working copy root itself
                path = "";

            Item item = (Item) items.get(path);
            Assert.assertNotNull("status not found in working copy: " + path,
                    item);
            Assert.assertEquals("wrong text status in working copy: " + path,
                    item.textStatus, tested[i].getTextStatus());
            if (item.workingCopyRev != -1)
                Assert.assertEquals("wrong revision number in working copy: "
                            + path,
                        item.workingCopyRev, tested[i].getRevisionNumber());
            Assert.assertEquals("lock status wrong: " + path,
                    item.isLocked, tested[i].isLocked());
            Assert.assertEquals("switch status wrong: " + path,
                    item.isSwitched, tested[i].isSwitched());
            Assert.assertEquals("wrong prop status in working copy: " + path,
                    item.propStatus, tested[i].getPropStatus());
            if (item.myContent != null)
            {
                Assert.assertEquals("state says file, working copy not: " + path,
                        tested[i].getNodeKind(),
                        item.nodeKind == -1 ? NodeKind.file : item.nodeKind);
                if (tested[i].getTextStatus() == Status.Kind.normal ||
                        item.checkContent)
                {
                    File input = new File(workingCopyPath, item.myPath);
                    Reader rd =
                            new InputStreamReader(new FileInputStream(input));
                    StringBuffer buffer = new StringBuffer();
                    int ch;
                    while ((ch = rd.read()) != -1)
                    {
                        buffer.append((char) ch);
                    }
                    rd.close();
                    Assert.assertEquals("content mismatch: " + path,
                            buffer.toString(), item.myContent);
                }
            }
            else
            {
                Assert.assertEquals("state says dir, working copy not: " + path,
                        tested[i].getNodeKind(),
                        item.nodeKind == -1 ? NodeKind.dir : item.nodeKind);
            }

            if (checkRepos)
            {
                Assert.assertEquals("Last commit revisions for OOD path '"
                                    + item.myPath + "' don't match:",
                                    item.reposLastCmtRevision,
                                    tested[i].getReposLastCmtRevisionNumber());
                Assert.assertEquals("Last commit kinds for OOD path '"
                                    + item.myPath + "' don't match:",
                                    item.reposKind, tested[i].getReposKind());

                // Only the last committed rev and kind is available for
                // paths deleted in the repos.
                if (tested[i].getRepositoryTextStatus() != Status.Kind.deleted)
                {
                    long lastCmtTime =
                        (tested[i].getReposLastCmtDate() == null ?
                         0 : tested[i].getReposLastCmtDate().getTime());
                    Assert.assertEquals("Last commit dates for OOD path '" +
                                        item.myPath + "' don't match:",
                                        new Date(item.reposLastCmtDate),
                                        new Date(lastCmtTime));
                    Assert.assertEquals("Last commit authors for OOD path '"
                                        + item.myPath + "' don't match:",
                                        item.reposLastCmtAuthor,
                                        tested[i].getReposLastCmtAuthor());
                }
            }
            item.touched = true;
        }

        // all items which have the touched flag not set, are missing in the
        // result array
        it = items.values().iterator();
        while (it.hasNext())
        {
            Item item = (Item) it.next();
            Assert.assertTrue("item in working copy not found in status",
                    item.touched);
        }
    }

    /**
     * internal class to discribe a single working copy item
     */
    public class Item
    {
        /**
         * the content of a file. A directory has a null content
         */
        String myContent;

        /**
         * the relative path of the item
         */
        String myPath;

        /**
         * the text (content) status of the item
         */
        int textStatus = Status.Kind.normal;

        /**
         * the property status of the item.
         */
        int propStatus = Status.Kind.none;

        /**
         * the expected revision number. -1 means do not check.
         */
        long workingCopyRev = -1;

        /**
         * flag if item has been touched. To detect missing items.
         */
        boolean touched;

        /**
         * flag if the content will be checked
         */
        boolean checkContent;

        /**
         * expected node kind. -1 means do not check.
         */
        int nodeKind = -1;

        /**
         * expected locked status
         */
        boolean isLocked;

        /**
         * expected switched status
         */
        boolean isSwitched;

        /**
         * youngest committed revision on repos if out of date
         */
        long reposLastCmtRevision = Revision.SVN_INVALID_REVNUM;

        /**
         * most recent commit date on repos if out of date
         */
        long reposLastCmtDate = 0;

        /**
         * node kind of the youngest commit if out of date
         */
        int reposKind = NodeKind.none;

        /**
         * author of the youngest commit if out of date.
         */
        String reposLastCmtAuthor;

        /**
         * create a new item
         * @param path      the path of the item.
         * @param content   the content of the item. A null signals a directory.
         */
        private Item(String path, String content)
        {
            myPath = path;
            myContent = content;
            items.put(path, this);
        }

        /**
         * copy constructor
         * @param source    the copy source.
         * @param owner     the WC of the copy
         */
        private Item(Item source, WC owner)
        {
            myPath = source.myPath;
            myContent = source.myContent;
            textStatus = source.textStatus;
            propStatus = source.propStatus;
            owner.items.put(myPath, this);
        }

        /**
         * copy this item
         * @param owner the new WC
         * @return  the copied item
         */
        private Item copy(WC owner)
        {
            return new Item(this, owner);
        }
    }
}
