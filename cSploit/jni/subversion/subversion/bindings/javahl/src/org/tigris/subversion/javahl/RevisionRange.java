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
 * Object that describes a revision range
 *
 * @since 1.5
 */
public class RevisionRange implements Comparable, java.io.Serializable
{
    // Update the serialVersionUID when there is a incompatible change
    // made to this class.  See any of the following, depending upon
    // the Java release.
    // http://java.sun.com/j2se/1.3/docs/guide/serialization/spec/version.doc7.html
    // http://java.sun.com/j2se/1.4/pdf/serial-spec.pdf
    // http://java.sun.com/j2se/1.5.0/docs/guide/serialization/spec/version.html#6678
    // http://java.sun.com/javase/6/docs/platform/serialization/spec/version.html#6678
    private static final long serialVersionUID = 1L;

    private Revision from;
    private Revision to;

    /**
     * Creates a new instance.  Called by native library.
     */
    private RevisionRange(long from, long to)
    {
        this.from = Revision.getInstance(from);
        this.to = Revision.getInstance(to);
    }

    public RevisionRange(Revision from, Revision to)
    {
        this.from = from;
        this.to = to;
    }

    public RevisionRange(org.apache.subversion.javahl.types.RevisionRange aRange)
    {
        this(Revision.createFromApache(aRange.getFromRevision()),
             Revision.createFromApache(aRange.getToRevision()));
    }

    /**
     * Accepts a string in one of these forms: n m-n Parses the results into a
     * from and to revision
     * @param revisionElement revision range or single revision
     */
    public RevisionRange(String revisionElement)
    {
        super();
        if (revisionElement == null)
        {
            return;
        }

        int hyphen = revisionElement.indexOf('-');
        if (hyphen > 0)
        {
            try
            {
                long fromRev = Long
                        .parseLong(revisionElement.substring(0, hyphen));
                long toRev = Long.parseLong(revisionElement
                        .substring(hyphen + 1));
                this.from = new Revision.Number(fromRev);
                this.to = new Revision.Number(toRev);
            }
            catch (NumberFormatException e)
            {
                return;
            }

        }
        else
        {
            try
            {
                long revNum = Long.parseLong(revisionElement.trim());
                this.from = new Revision.Number(revNum);
                this.to = this.from;
            }
            catch (NumberFormatException e)
            {
                return;
            }
        }
    }

    public org.apache.subversion.javahl.types.RevisionRange toApache()
    {
        return new org.apache.subversion.javahl.types.RevisionRange(
                from == null ? null : from.toApache(),
                to == null ? null : to.toApache());
    }

    public Revision getFromRevision()
    {
        return from;
    }

    public Revision getToRevision()
    {
        return to;
    }

    public String toString()
    {
        if (from != null && to != null)
        {
            if (from.equals(to))
                return from.toString();
            else
                return from.toString() + '-' + to.toString();
        }
        return super.toString();
    }

    public static Long getRevisionAsLong(Revision rev)
    {
        long val = 0;
        if (rev != null && rev instanceof Revision.Number)
        {
            val = ((Revision.Number) rev).getNumber();
        }
        return new Long(val);
    }

    public int hashCode()
    {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((from == null) ? 0 : from.hashCode());
        result = prime * result + ((to == null) ? 0 : to.hashCode());
        return result;
    }

    /**
     * @param range The RevisionRange to compare this object to.
     */
    public boolean equals(Object range)
    {
        if (this == range)
            return true;
        if (!super.equals(range))
            return false;
        if (getClass() != range.getClass())
            return false;

        final RevisionRange other = (RevisionRange) range;

        if (from == null)
        {
            if (other.from != null)
                return false;
        }
        else if (!from.equals(other.from))
        {
            return false;
        }

        if (to == null)
        {
            if (other.to != null)
                return false;
        }
        else if (!to.equals(other.to))
        {
            return false;
        }

        return true;
    }

    /**
     * @param range The RevisionRange to compare this object to.
     */
    public int compareTo(Object range)
    {
        if (this == range)
            return 0;

        Revision other = ((RevisionRange) range).getFromRevision();
        return RevisionRange.getRevisionAsLong(this.getFromRevision())
            .compareTo(RevisionRange.getRevisionAsLong(other));
    }
}
