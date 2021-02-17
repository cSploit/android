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

package org.apache.subversion.javahl.types;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * Class to specify a revision in a svn command.
 */
public class Revision implements java.io.Serializable
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
     * kind of revision specified
     */
    protected Kind revKind;

    /**
     * Internally create a new revision.  Public for backward compat reasons.
     * Callers should use getInstance() instead.
     * @param kind    kind of revision
     */
    public Revision(Kind kind)
    {
        if (kind.ordinal() < Kind.unspecified.ordinal()
               || kind.ordinal() > Kind.head.ordinal())
            throw new IllegalArgumentException(
                    kind+" is not a legal revision kind");
        revKind = kind;
    }

    /**
     * Returns the kind of the Revsion
     * @return kind
     */
    public Kind getKind()
    {
        return revKind;
    }

    /**
     * return the textual representation of the revision
     * @return english text
     */
    public String toString()
    {
        switch(revKind) {
            case base : return "BASE";
            case committed : return "COMMITTED";
            case head : return "HEAD";
            case previous : return "PREV";
            case working : return "WORKING";
        }
        return super.toString();
    }

    /* (non-Javadoc)
     * @see java.lang.Object#hashCode()
     */
    public int hashCode()
    {
        return revKind.ordinal() * -1;
    }

    /**
     * compare to revision objects
     * @param target
     * @return if both object have equal content
     */
    public boolean equals(Object target) {
        if (this == target)
            return true;
        if (!(target instanceof Revision))
            return false;

        return ((Revision)target).revKind == revKind;
    }

    /**
     * Creates a Revision.Number object
     * @param revisionNumber    the revision number of the new object
     * @return  the new object
     * @throws IllegalArgumentException If the specified revision
     * number is invalid.
     */
    public static Revision getInstance(long revisionNumber)
    {
        return new Revision.Number(revisionNumber);
    }

    /**
     * Factory which creates {@link #Number} objects for valid
     * revision numbers only (those greater than zero).  For internal
     * usage to avoid an IllegalArgumentException, where no external
     * consumer of the javahl API passed an invalid revision number.
     *
     * @param revNumber The revision number to create an object for.
     * @return An object representing <code>revNumber</code>, or
     * <code>null</code> if the revision number was invalid.
     */
    static Number createNumber(long revNumber)
    {
        return (revNumber < 0 ? null : new Number(revNumber));
    }

    /**
     * Creates a Revision.DateSpec objet
     * @param revisionDate  the date of the new object
     * @return  the new object
     */
    public static Revision getInstance(Date revisionDate)
    {
        return new Revision.DateSpec(revisionDate);
    }

    /**
     * last committed revision
     */
    public static final Revision HEAD = new Revision(Kind.head);

    /**
     * first existing revision
     */
    public static final Revision START = new Revision(Kind.unspecified);

    /**
     * last committed revision, needs working copy
     */
    public static final Revision COMMITTED = new Revision(Kind.committed);

    /**
     * previous committed revision, needs working copy
     */
    public static final Revision PREVIOUS = new Revision(Kind.previous);

    /**
     * base revision of working copy
     */
    public static final Revision BASE = new Revision(Kind.base);

    /**
     * working version in working copy
     */
    public static final Revision WORKING = new Revision(Kind.working);

    /**
     * Marker revision number for no real revision
     */
    public static final int SVN_INVALID_REVNUM = -1;

    /**
     * class to specify a Revision by number
     */
    public static class Number extends Revision
    {
        // Update the serialVersionUID when there is a incompatible
        // change made to this class.
        private static final long serialVersionUID = 1L;

        /**
         * the revision number
         */
        protected long revNumber;

        /**
         * create a revision by number object
         * @param number the number
         * @throws IllegalArgumentException If the specified revision
         * number is invalid.
         */
        public Number(long number)
        {
            super(Kind.number);
            if (number < 0)
                throw new IllegalArgumentException
                    ("Invalid (negative) revision number: " + number);
            revNumber = number;
        }

        /**
         * Returns the revision number
         * @return number
         */
        public long getNumber()
        {
            return revNumber;
        }

        /**
         * return the textual representation of the revision
         * @return english text
         */
        public String toString() {
            return Long.toString(revNumber);
        }

        /**
         * compare to revision objects
         * @param target
         * @return if both object have equal content
         */
        public boolean equals(Object target) {
            if (!super.equals(target))
                return false;

            return ((Revision.Number)target).revNumber == revNumber;
        }

        /* (non-Javadoc)
         * @see org.apache.subversion.javahl.Revision#hashCode()
         */
        public int hashCode()
        {
            return (int)(revNumber ^ (revNumber >>> 32));
        }
    }

    /**
     * class to specify a revision by a date
     */
    public static class DateSpec extends Revision
    {
        // Update the serialVersionUID when there is a incompatible change
        // made to this class.
        private static final long serialVersionUID = 1L;

        /**
         * the date
         */
        protected Date revDate;

        /**
         * create a revision by date object
         * @param date
         */
        public DateSpec(Date date)
        {
            super(Kind.date);
            if (date == null)
                throw new IllegalArgumentException("a date must be specified");
            revDate = date;
        }
        /**
         * Returns the date of the revision
         * @return the date
         */
        public Date getDate()
        {
            return revDate;
        }

        /**
         * return the textual representation of the revision
         * @return english text
         */
        public String toString() {

            SimpleDateFormat dateFormat =
                    new SimpleDateFormat("EEE, d MMM yyyy HH:mm:ss Z",
                                         Locale.US);
            return '{'+dateFormat.format(revDate)+'}';
        }

        /**
         * compare to revision objects
         * @param target
         * @return if both object have equal content
         */
        public boolean equals(Object target) {
            if (!super.equals(target))
                return false;

            return ((Revision.DateSpec)target).revDate.equals(revDate);
        }
        /* (non-Javadoc)
         * @see org.apache.subversion.javahl.Revision#hashCode()
         */
        public int hashCode()
        {
            return revDate.hashCode();
        }

    }

    /**
     * Various ways of specifying revisions.
     *
     * Note:
     * In contexts where local mods are relevant, the `working' kind
     * refers to the uncommitted "working" revision, which may be modified
     * with respect to its base revision.  In other contexts, `working'
     * should behave the same as `committed' or `current'.
     */
    public enum Kind
    {
       /** No revision information given. */
        unspecified,

        /** revision given as number */
        number,

        /** revision given as date */
        date,

        /** rev of most recent change */
        committed,

        /** (rev of most recent change) - 1 */
        previous,

        /** .svn/entries current revision */
        base,

        /** current, plus local mods */
        working,

        /** repository youngest */
        head;
    }
}
