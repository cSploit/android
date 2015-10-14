package org.csploit.android.helpers;

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
}
