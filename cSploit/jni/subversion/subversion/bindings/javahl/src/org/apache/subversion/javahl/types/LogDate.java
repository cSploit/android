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

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.TimeZone;

/**
 * Holds date for a log message.  This class maintains
 * the time to the microsecond and is not lossy.
 */
public class LogDate implements java.io.Serializable
{
    private static final long serialVersionUID = 1L;
    private static final DateFormat formatter = new SimpleDateFormat(
            "yyyy-MM-dd'T'HH:mm:ss.SSS z");
    private static final TimeZone UTC = TimeZone.getTimeZone("UTC");

    private final long timeMicros;
    private final String cachedString;
    private final Calendar cachedDate;

    public LogDate(String datestr) throws ParseException
    {
        if (datestr == null || datestr.length() != 27 || datestr.charAt(26) != 'Z')
        {
            throw new ParseException("String is not a valid Subversion date", 0);
        }
        Date date;
        synchronized(formatter)
        {
            date = formatter.parse(datestr.substring(0, 23) + " UTC");
        }
        this.cachedString = datestr;
        cachedDate = Calendar.getInstance(UTC);
        cachedDate.setTime(date);
        timeMicros = cachedDate.getTimeInMillis() * 1000
                        + Integer.parseInt(datestr.substring(23, 26));
    }

    /**
     * Returns the time of the commit in microseconds
     * @return the time of the commit measured in the number of
     *         microseconds since 00:00:00 January 1, 1970 UTC
     */
    public long getTimeMicros()
    {
        return timeMicros;
    }

    /**
     * Returns the time of the commit in milliseconds
     * @return the time of the commit measured in the number of
     *         milliseconds since 00:00:00 January 1, 1970 UTC
     */
    public long getTimeMillis()
    {
        return cachedDate.getTimeInMillis();
    }

    /**
     * Returns the time of the commit as Calendar
     * @return the time of the commit as java.util.Calendar
     */
    public Calendar getCalender()
    {
        return cachedDate;
    }

    /**
     * Returns the date of the commit
     * @return the time of the commit as java.util.Date
     */
    public Date getDate()
    {
        return cachedDate.getTime();
    }

    public String toString()
    {
         return cachedString;
    }

    public int hashCode()
    {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int) (timeMicros ^ (timeMicros >>> 32));
        return result;
    }

    public boolean equals(Object obj)
    {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        final LogDate other = (LogDate) obj;
        if (timeMicros != other.getTimeMicros())
            return false;
        return true;
    }

}
