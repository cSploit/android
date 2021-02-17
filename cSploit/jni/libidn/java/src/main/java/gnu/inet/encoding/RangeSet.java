/* Copyright (C) 2013 Free Software Foundation, Inc.
   Author: Stefan Larsson

   This file is part of GNU Libidn.

   GNU Libidn is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version.

   or both in parallel, as here.

   GNU Libidn is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>. */

package gnu.inet.encoding;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;

/**
 * Set of integer ranges supporting efficient contains-checks.
 * @author Stefan Larsson
 */
public final class RangeSet
{
  private static final RangeContainsComparator CONTAINS_COMPARATOR =
      new RangeContainsComparator();

  private final Range[] ranges;

  private final Range mostSignificantGap;

  // TODO Store ranges with improved cache-locality, probably int[] with even/odd elements being first/last

  public static final class Range implements Comparable<Range>
  {
    private final int first;
    private final int last;

    public Range(int first, int last)
    {
      if (first > last) {
	throw new IllegalArgumentException("Reversed " + first + "-" + last);
      }
      this.first = first;
      this.last = last;
    }

    public Range(int single)
    {
      this.first = single;
      this.last = single;
    }

    public Range(Range firstRange, Range lastRange)
    {
      // firstRange shouldn't start later than lastRange starts
      if (firstRange.first > lastRange.first)
      {
	throw new IllegalArgumentException(firstRange + " starts later than "
					   + lastRange);
      }

      if (lastRange.first - firstRange.last > 1)
      {
	throw new IllegalArgumentException("Disjunct " + firstRange
					   + " - " + lastRange);
      }
      this.first = firstRange.first;
      this.last = lastRange.last;
    }

    public boolean contains(final int i) {
      return first <= i && i <= last;
    }

    /**
     * Checks if this range completely can contain the other range.
     * @param other other range to verify
     * @return {@code true} if other completely contained by this,
     *         otherwise {@code false}
     */
    public boolean contains(final Range other)
    {
      return (this.first <= other.first) && (other.last <= this.last);
    }

    @Override
    public String toString()
    {
      return "[" + Integer.toHexString(first).toUpperCase(Locale.ENGLISH) + ","
	  + Integer.toHexString(last).toUpperCase(Locale.ENGLISH) + ']';
    }

    //@Override
    public int compareTo(final Range other)
    {
      if (this.first < other.first)
      {
	return -1;
      }
      if (this.first > other.first)
      {
	return 1;
      }

      if (this.last < other.last)
      {
	return -1;
      }
      if (this.last > other.last)
      {
	return 1;
      }

      return 0;
    }

    @Override
    public boolean equals(Object o)
    {
      if (this == o) return true;
      if (o == null || getClass() != o.getClass()) return false;

      Range range = (Range) o;

      if (first != range.first) return false;
      if (last != range.last) return false;

      return true;
    }

    @Override
    public int hashCode()
    {
      return 31 * first + last;
    }
  }

  private static class RangeContainsComparator implements Comparator<Range> {

    public int compare(Range current, Range contained)
    {
      if (current.last < contained.first)
      {
	return -1;
      }
      if (contained.last < current.first)
      {
	return 1;
      }
      return 0;
    }
  }

  private RangeSet(final List<Range> ranges) {
    this.ranges = ranges.toArray(new Range[ranges.size()]);
    this.mostSignificantGap = findMostSignificantGap(this.ranges);
  }

  /**
   * Returns the most significant gap, or {@code null} if no important gap found.
   * @param ranges ranges to search
   * @return most significant gap, or {@code null} if no important gap found
   */
  private static Range findMostSignificantGap(final Range[] ranges)
  {
    if (ranges.length == 0) {
      return new Range(0, Integer.MAX_VALUE);
    }

    final int aIdx =
	    Arrays.binarySearch(ranges, new Range('a'), CONTAINS_COMPARATOR);
    if (aIdx >= 0)
    {
      // 'a' in ranges, don't even attempt to exclude smartly
      return null;
    }

    final int insertionPoint = -(aIdx + 1);
    if (insertionPoint == 0) {
      return new Range(0, ranges[0].first - 1);
    }
    if (insertionPoint == ranges.length) {
      return new Range(ranges[ranges.length - 1].last + 1, Integer.MAX_VALUE);
    }
    return new Range(ranges[insertionPoint - 1].last + 1,
		     ranges[insertionPoint].first - 1);
  }

  public static final class Builder {
    private final List<Range> ranges = new ArrayList<Range>();

    public Builder addRange(final Range range) {
      ranges.add(range);
      return this;
    }

    public Builder addRanges(final Collection<Range> ranges) {
      ranges.addAll(ranges);
      return this;
    }

    public Builder addRanges(final char[][] ranges) {
      for (final char[] range : ranges) {
	if (range.length == 1) {
	  this.ranges.add(new Range(range[0]));
	} else if (range.length == 2) {
	  this.ranges.add(new Range(range[0], range[1]));
	} else {
	  throw new IllegalArgumentException("Unexpected range len:"
					     + range.length);
	}
      }
      return this;
    }

    public Builder addRanges(final char[] items) {
      for (final char item : items) {
	this.ranges.add(new Range(item));
      }
      return this;
    }

    public RangeSet build() {
      Collections.sort(ranges);
      final List<Range> mergedRanges = mergeRanges(ranges);
      return new RangeSet(mergedRanges);
    }

    static List<Range> mergeRanges(final List<Range> ranges)
    {
      if (ranges.isEmpty())
      {
	return Collections.emptyList();
      }

      final List<Range> result = new ArrayList<Range>();
      final Iterator<Range> it = ranges.iterator();

      Range leftRange = it.next();
      List<Range> merged = Collections.singletonList(leftRange);
      while (it.hasNext())
      {
	// merge ranges as long as they're adjacent/overlapping
	while (merged.size() == 1 && it.hasNext())
	{
	  leftRange = merged.get(0);
	  Range rightRange = it.next();
	  merged = mergeRanges(leftRange, rightRange);
	}
	// when ranges weren't merge-able, add all but last, merge against last
	if (merged.size() > 1)
	{
	  result.addAll(merged.subList(0, merged.size() - 1));
	  merged = Collections.singletonList(merged.get(merged.size() - 1));
	}
      }
      result.addAll(merged);
      return result;
    }

    static List<Range> mergeRanges(Range leftRange, Range rightRange)
    {
      if (leftRange.last + 1 >= rightRange.first) {
	final int last = Math.max(rightRange.last, leftRange.last);
	return Collections.singletonList(new Range(leftRange.first, last));
      } else {
	final List<Range> result = new ArrayList<Range>(2);
	result.add(leftRange);
	result.add(rightRange);
	return result;
      }
    }
  }

  public static Builder builder() {
    return new Builder();
  }

  public boolean contains(final int i)
  {
    if (mostSignificantGap != null && mostSignificantGap.contains(i)) {
      return false;
    }

    final Range searchRange = new Range(i);
    int idx = Arrays.binarySearch(ranges, searchRange, CONTAINS_COMPARATOR);
    return idx >= 0;
  }

  public boolean containsAnyCodePoint(final CharSequence text) {
    final Range inputRange = createTextRange(text);
    return containsAnyCodePoint(text, inputRange);
  }

  public boolean containsAnyCodePoint(final CharSequence text,
				      final Range inputRange) {
    final int len = text.length();
    if (len == 0)
    {
      return false;
    }

    if (mostSignificantGap != null
	&& mostSignificantGap.contains(inputRange.first)
	&& mostSignificantGap.contains(inputRange.last)) {
      return false;
    }

    // if found, returns the index, otherwise "-insertionPoint - 1"
    final int idxEnd =
	    Arrays.binarySearch(ranges, new Range(inputRange.last), CONTAINS_COMPARATOR);
    // search for start in "head" range only (likely small)
    final int startFromIdx = 0;
    final int startEndIdx = idxEnd >= 0 ? idxEnd + 1 : -(idxEnd + 1);
    final int idxStart =
	    Arrays.binarySearch(ranges, startFromIdx, startEndIdx,
				new Range(inputRange.first), CONTAINS_COMPARATOR);

    // If whole range in text outside same non-contained range, won't be found
    // If whole range in text inside single contained range, must match
    if (idxStart == idxEnd)
    {
      return idxStart >= 0;
    }

    // if start or end inside contained range, match
    if (idxStart >= 0 || idxEnd >= 0)
    {
      return true;
    }

    // text spans across multiple ranges of set, need to search individual chars
    final int searchStart = -idxStart + 1;
    final int searchEnd = -idxEnd + 1;

    for (int i = 0; i < len; )
    {
      final int cp = Character.codePointAt(text, i);
      i += Character.charCount(cp);
      final int idx = Arrays.binarySearch(ranges, searchStart, searchEnd, new Range(cp), CONTAINS_COMPARATOR);
      if (idx > 0)
      {
	return true;
      }
    }
    return false;
  }

  /**
   * Returns the range of the input or {@code all-inclusive range} if input is empty
   * @param text input text
   * @return range of input, or {@code all-inclusive} if empty input
   */
  public static Range createTextRange(final CharSequence text)
  {
    final int len = text.length();
    if (len == 0) {
      return new Range(Integer.MIN_VALUE, Integer.MAX_VALUE);
    }

    int minCodePoint = Integer.MAX_VALUE;
    int maxCodePoint = Integer.MIN_VALUE;
    for (int i = 0; i < len; )
    {
      final int cp = Character.codePointAt(text, i);
      minCodePoint = Math.min(minCodePoint, cp);
      maxCodePoint = Math.max(maxCodePoint, cp);
      i += Character.charCount(cp);
    }
    return new Range(minCodePoint, maxCodePoint);
  }

  @Override
  public String toString()
  {
    return "RangeSet{" +
	    "ranges=" + Arrays.asList(ranges) +
	    ", mostSignificantGap=" + mostSignificantGap +
	    '}';
  }
}
