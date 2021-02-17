
/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * dSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android.net;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteOrder;
import java.util.Arrays;

public class IP4Address implements Comparable<IP4Address>
{
  private byte[] mByteArray = null;
  private String mString = "";
  private final int mInteger;
  private InetAddress mAddress = null;

  public static int ntohl(int n){
    return ((n >> 24) & 0xFF) + ((n >> 16) & 0xFF) + ((n >> 8) & 0xFF) + (n & 0xFF);
  }

  public static IP4Address next(IP4Address address) throws UnknownHostException{
    byte[] addr = address.toByteArray();

    int i = addr.length - 1;
    while(i >= 0 && addr[i] == (byte) 0xff){
      addr[i] = 0;
      i--;
    }

    if(i >= 0){
      addr[i]++;
      return new IP4Address(addr);
    } else
      return null;
  }


  public IP4Address(int address) throws UnknownHostException{
    mByteArray = new byte[4];
    mInteger = address;

    if(ByteOrder.nativeOrder() == ByteOrder.LITTLE_ENDIAN){
      mByteArray[0] = (byte) (address & 0xFF);
      mByteArray[1] = (byte) (0xFF & address >> 8);
      mByteArray[2] = (byte) (0xFF & address >> 16);
      mByteArray[3] = (byte) (0xFF & address >> 24);
    } else{
      mByteArray[0] = (byte) (0xFF & address >> 24);
      mByteArray[1] = (byte) (0xFF & address >> 16);
      mByteArray[2] = (byte) (0xFF & address >> 8);
      mByteArray[3] = (byte) (address & 0xFF);
    }

    mAddress = InetAddress.getByAddress(mByteArray);
    mString = mAddress.getHostAddress();
  }

  public IP4Address(String address) throws UnknownHostException{
    mString = address;
    mAddress = InetAddress.getByName(address);
    mByteArray = mAddress.getAddress();
    mInteger = ((mByteArray[0] & 0xFF) << 24) +
      ((mByteArray[1] & 0xFF) << 16) +
      ((mByteArray[2] & 0xFF) << 8) +
      (mByteArray[3] & 0xFF);
  }

  public IP4Address(byte[] address) throws UnknownHostException{
    mByteArray = Arrays.copyOf(address, address.length);
    mAddress = InetAddress.getByAddress(mByteArray);
    mString = mAddress.getHostAddress();
    mInteger = ((mByteArray[0] & 0xFF) << 24) +
      ((mByteArray[1] & 0xFF) << 16) +
      ((mByteArray[2] & 0xFF) << 8) +
      (mByteArray[3] & 0xFF);
  }

  public IP4Address(InetAddress address){
    mAddress = address;
    mByteArray = address.getAddress();
    mString = mAddress.getHostAddress();
    mInteger = ((mByteArray[0] & 0xFF) << 24) +
      ((mByteArray[1] & 0xFF) << 16) +
      ((mByteArray[2] & 0xFF) << 8) +
      (mByteArray[3] & 0xFF);
  }

  public byte[] toByteArray(){
    return Arrays.copyOf(mByteArray, mByteArray.length);
  }

  public String toString(){
    return mString;
  }

  public int toInteger(){
    return mInteger;
  }

  public InetAddress toInetAddress(){
    return mAddress;
  }

  public boolean equals(IP4Address address){
    return mInteger == address.toInteger();
  }

  public boolean equals(InetAddress address){
    return mAddress.equals(address);
  }

  @Override
  public int compareTo(IP4Address another) {
    return mInteger - another.mInteger;
  }

  public int getPrefixLength(){
    int bits, i, n = mInteger;

    // WTF is this?
    for(i = 0, bits = (n & 1); i < 32; i++, n >>>= 1, bits += n & 1) ;

    return bits;
  }
}
