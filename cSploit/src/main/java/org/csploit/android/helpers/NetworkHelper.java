package org.csploit.android.helpers;

import org.apache.commons.compress.utils.IOUtils;
import org.csploit.android.core.Child;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.tools.Ip;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetAddress;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

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

  public static String getIfaceGateway(String iface) {
    Pattern pattern = Pattern.compile(String.format("^%s\\t+00000000\\t+([0-9A-F]{8})", iface), Pattern.CASE_INSENSITIVE);
    BufferedReader reader = null;
    String line;

    try {
      reader = new BufferedReader(new InputStreamReader(new FileInputStream("/proc/net/route")));

      while ((line = reader.readLine()) != null) {
        Matcher matcher = pattern.matcher(line);
        if (!matcher.find()) {
          continue;
        }
        String rawAddress = matcher.group(1);
        StringBuilder sb = new StringBuilder();
        for (int i = 6; ; i -= 2) {
          String part = rawAddress.substring(i, i + 2);
          sb.append(Integer.parseInt(part, 16));
          if (i > 0) {
            sb.append('.');
          } else {
            break;
          }
        }
        String res = sb.toString();
        Logger.debug("found system default gateway for interface " + iface + ": " + res);
        return res;
      }
    } catch (IOException e) {
      System.errorLogging(e);
    } finally {
      IOUtils.closeQuietly(reader);
    }

    Logger.warning("falling back to ip");

    return getIfaceGatewayUsingIp(iface);
  }




  private static String getIfaceGatewayUsingIp(String iface) {
    if(!System.isCoreInitialized())
      return null;

    final StringBuilder sb = new StringBuilder();

    try {
      Child process = System.getTools().ip.getGatewayForIface(iface, new Ip.GatewayReceiver() {
        @Override
        public void onGatewayFound(String gateway) {
          sb.append(gateway);
        }
      });
      process.join();
    } catch (ChildManager.ChildNotStartedException | InterruptedException e) {
      System.errorLogging(e);
    }

    return sb.length() > 0 ? sb.toString() : null;
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
