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
import java.util.Date;
import java.util.List;

/**
 * Implementation of {@link BlameCallback} interface.
 *
 * @since 1.5
 */
public class BlameCallbackImpl implements BlameCallback, BlameCallback2
{

    /** list of blame records (lines) */
    private List<BlameLine> lines = new ArrayList<BlameLine>();

    /*
     * (non-Javadoc)
     * @see org.tigris.subversion.javahl.BlameCallback#singleLine(java.util.Date,
     * long, java.lang.String, java.lang.String)
     */
    public void singleLine(Date changed, long revision, String author,
                           String line)
    {
        addBlameLine(new BlameLine(revision, author, changed, line));
    }

    public void singleLine(Date date, long revision, String author,
                           Date merged_date, long merged_revision,
                           String merged_author, String merged_path,
                           String line)
    {
        addBlameLine(new BlameLine(getRevision(revision, merged_revision),
                                   getAuthor(author, merged_author),
                                   getDate(date, merged_date),
                                   line));
    }

    private Date getDate(Date date, Date merged_date) {
        return (merged_date == null ? date : merged_date);
    }

    private String getAuthor(String author, String merged_author) {
        return (merged_author == null ? author : merged_author);
    }

    private long getRevision(long revision, long merged_revision) {
        return (merged_revision == -1 ? revision : merged_revision);
    }

    /**
     * Retrieve the number of line of blame information
     * @return number of lines of blame information
     */
    public int numberOfLines()
    {
        return this.lines.size();
    }

    /**
     * Retrieve blame information for specified line number
     * @param i the line number to retrieve blame information about
     * @return  Returns object with blame information for line
     */
    public BlameLine getBlameLine(int i)
    {
        if (i >= this.lines.size())
        {
            return null;
        }
        return this.lines.get(i);
    }

    /**
     * Append the given blame info to the list
     * @param blameLine
     */
    protected void addBlameLine(BlameLine blameLine)
    {
        this.lines.add(blameLine);
    }

    /**
     * Class represeting one line of the lines, i.e. a blame record
     *
     */
    public static class BlameLine
    {

        private long revision;

        private String author;

        private Date changed;

        private String line;

        /**
         * Constructor
         *
         * @param revision
         * @param author
         * @param changed
         * @param line
         */
        public BlameLine(long revision, String author,
                         Date changed, String line)
        {
            super();
            this.revision = revision;
            this.author = author;
            this.changed = changed;
            this.line = line;
        }

        /**
         * @return Returns the author.
         */
        public String getAuthor()
        {
            return author;
        }

        /**
         * @return Returns the date changed.
         */
        public Date getChanged()
        {
            return changed;
        }

        /**
         * @return Returns the source line content.
         */
        public String getLine()
        {
            return line;
        }


        /**
         * @return Returns the revision.
         */
        public long getRevision()
        {
            return revision;
        }

        /*
         * (non-Javadoc)
         * @see java.lang.Object#toString()
         */
        public String toString()
        {
            StringBuffer sb = new StringBuffer();
            if (revision > 0)
            {
                pad(sb, Long.toString(revision), 6);
                sb.append(' ');
            }
            else
            {
                sb.append("     - ");
            }

            if (author != null)
            {
                pad(sb, author, 10);
                sb.append(" ");
            }
            else
            {
                sb.append("         - ");
            }

            sb.append(line);

            return sb.toString();
        }

        /**
         * Left pad the input string to a given length, to simulate printf()-
         * style output. This method appends the output to the class sb member.
         * @param sb StringBuffer to append to
         * @param val the input string
         * @param len the minimum length to pad to
         */
        private void pad(StringBuffer sb, String val, int len)
        {
            int padding = len - val.length();

            for (int i = 0; i < padding; i++)
            {
                sb.append(' ');
            }

            sb.append(val);
        }

    }
}
