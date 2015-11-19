package org.csploit.android.helpers;

import java.net.InetAddress;

/**
 * A class that provide some useful network-related static methods
 */
public final class NetworkHelper {
  /**
   * translate an OUI to it's integer representation
   * @param macAddress the 6-byte array that represent a mac address
   * @return the OUI integer
   */
  public static int getOUICode(byte[] macAddress) {
    return (macAddress[0] << 16) | (macAddress[1] << 8) | macAddress[2];
  }

  /**
   * translate an OUI to it's integer representation
   * @param hexOui a string that hold OUI in hexadecimal form ( e.g. "ACDE48" )
   * @return the OUI integer
   */
  public static int getOUICode(String hexOui) {
    return Integer.parseInt(hexOui, 16);
  }

  /**
   * compare two byte[] comparing their length and each of their values.
   * @return -1 if {@code a} is less than {@code b}, 0 if are equals, +1 if {@code a} is greater than {@code b}
   */
  public static int compareByteArray(byte[] a, byte[] b) {
    int result;

    result = a.length - b.length;

    if(result != 0) {
      return result;
    }

    for(int i = 0; i < a.length; i++) {
      result = ((short) a[i] & 0xFF) - ((short) b[i] & 0xFF);
      if(result != 0) {
        return result;
      }
    }

    return 0;
  }

  /**
   * compare two {@link InetAddress}
   * @return -1 if {@code a} is less than {@code b}, 0 if are equals, +1 if {@code a} is greater than {@code b}
   */
  public static int compareInetAddresses(InetAddress a, InetAddress b) {
    return compareByteArray(a.getAddress(), b.getAddress());
  }
}
