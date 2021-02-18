/*
 * Copyright 2012 Rui Araújo, Luís Fonseca
 *
 * This file is part of Router Keygen.
 *
 * Router Keygen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Router Keygen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Router Keygen.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android.wifi.algorithms.helpers;


/**
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * <p>Fast, well distributed, cross-platform hash functions.
 * </p>
 * <p/>
 * <p>Development background: I was surprised to discovered that there isn't a good cross-platform hash function defined for strings. MD5, SHA, FVN, etc, all define hash functions over bytes, meaning that it's under-specified for strings.
 * </p>
 * <p/>
 * <p>So I set out to create a standard 32 bit string hash that would be well defined for implementation in all languages, have very high performance, and have very good hash properties such as distribution. After evaluating all the options, I settled on using Bob Jenkins' lookup3 as a base. It's a well studied and very fast hash function, and the hashword variant can work with 32 bits at a time (perfect for hashing unicode code points). It's also even faster on the latest JVMs which can translate pairs of shifts into native rotate instructions.
 * </p>
 * <p>The only problem with using lookup3 hashword is that it includes a length in the initial value. This would suck some performance out since directly hashing a UTF8 or UTF16 string (Java) would require a pre-scan to get the actual number of unicode code points. The solution was to simply remove the length factor, which is equivalent to biasing initVal by -(numCodePoints*4). This slightly modified lookup3 I define as lookup3ycs.
 * </p>
 * <p>So the definition of the cross-platform string hash lookup3ycs is as follows:
 * </p>
 * <p>The hash value of a character sequence (a string) is defined to be the hash of its unicode code points, according to lookup3 hashword, with the initval biased by -(length*4).
 * </p>
 * <p>So by definition
 * </p>
 * <pre>
 * lookup3ycs(k,offset,length,initval) == lookup3(k,offset,length,initval-(length*4))
 *
 * AND
 *
 * lookup3ycs(k,offset,length,initval+(length*4)) == lookup3(k,offset,length,initval)
 * </pre>
 * <p>An obvious advantage of this relationship is that you can use lookup3 if you don't have an implementation of lookup3ycs.
 * </p>
 *
 * @author yonik
 */
public class JenkinsHash{
  /**
   * A Java implementation of hashword from lookup3.c by Bob Jenkins
   * (<a href="http://burtleburtle.net/bob/c/lookup3.c">original source</a>).
   *
   * @param k   the key to hash
   * @param offset   offset of the start of the key
   * @param length   length of the key
   * @param initval  initial value to fold into the hash
   * @return the 32 bit hash code
   */


  // max value to limit it to 4 bytes
  private static final long MAX_VALUE = 0xFFFFFFFFL;

  // internal variables used in the various calculations
  long a;
  long b;
  long c;


  /**
   * Do addition and turn into 4 bytes.
   *
   * @param val
   * @param add
   * @return
   */
  private long add(long val, long add){
    return (val + add) & MAX_VALUE;
  }

  /**
   * Do subtraction and turn into 4 bytes.
   *
   * @param val
   * @param subtract
   * @return
   */
  private long subtract(long val, long subtract){
    return (val - subtract) & MAX_VALUE;
  }

  /**
   * Left shift val by shift bits and turn in 4 bytes.
   *
   * @param val
   * @param xor
   * @return
   */
  private long xor(long val, long xor){
    return (val ^ xor) & MAX_VALUE;
  }

  /**
   * Left shift val by shift bits.  Cut down to 4 bytes.
   *
   * @param val
   * @param shift
   * @return
   */
  private long leftShift(long val, int shift){
    return (val << shift) & MAX_VALUE;
  }


  private long rot(long val, int shift){
    return (leftShift(val, shift) | (val >>> (32 - shift))) & MAX_VALUE;
  }

  /**
   * Mix up the values in the hash function.
   */
  private void hashMix(){
    a = subtract(a, c);
    a = xor(a, rot(c, 4));
    c = add(c, b);
    b = subtract(b, a);
    b = xor(b, rot(a, 6));
    a = add(a, c);
    c = subtract(c, b);
    c = xor(c, rot(b, 8));
    b = add(b, a);
    a = subtract(a, c);
    a = xor(a, rot(c, 16));
    c = add(c, b);
    b = subtract(b, a);
    b = xor(b, rot(a, 19));
    a = add(a, c);
    c = subtract(c, b);
    c = xor(c, rot(b, 4));
    b = add(b, a);
  }

  @SuppressWarnings("fallthrough")
  public long hashword(long[] k, int length, long initval){

    a = b = c = 0xdeadbeef + (length << 2) + (initval & MAX_VALUE);

    int i = 0;
    while(length > 3){
      a = add(a, k[i + 0]);
      b = add(b, k[i + 1]);
      c = add(c, k[i + 2]);
      hashMix();

      length -= 3;
      i += 3;
    }

    switch(length){
      case 3:
        c = add(c, k[i + 2]);  // fall through
      case 2:
        b = add(b, k[i + 1]);  // fall through
      case 1:
        a = add(a, k[i + 0]);  // fall through
        finalHash();
      case 0:
        break;
    }
    return c;
  }

  void finalHash(){
    c = xor(c, b);
    c = subtract(c, rot(b, 14));
    a = xor(a, c);
    a = subtract(a, rot(c, 11));
    b = xor(b, a);
    b = subtract(b, rot(a, 25));
    c = xor(c, b);
    c = subtract(c, rot(b, 16));
    a = xor(a, c);
    a = subtract(a, rot(c, 4));
    b = xor(b, a);
    b = subtract(b, rot(a, 14));
    c = xor(c, b);
    c = subtract(c, rot(b, 24));
  }


}
