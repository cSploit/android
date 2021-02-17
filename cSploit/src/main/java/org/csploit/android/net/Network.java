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

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.DhcpInfo;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.util.Patterns;
import org.apache.commons.net.util.SubnetUtils;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.helpers.NetworkHelper;

import java.lang.reflect.Method;
import java.net.*;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;

public class Network implements Comparable<Network> {
  public enum Protocol {
    TCP,
    UDP,
    ICMP,
    IGMP,
    UNKNOWN;

    public static Protocol fromString(String proto) {

      if (proto != null) {
        proto = proto.toLowerCase();

        if (proto.equals("tcp"))
          return TCP;

        else if (proto.equals("udp"))
          return UDP;

        else if (proto.equals("icmp"))
          return ICMP;

        else if (proto.equals("igmp"))
          return IGMP;
      }

      return UNKNOWN;
    }

    public String toString() {
      switch (this) {
        case ICMP:
          return "icmp";
        case IGMP:
          return "igmp";
        case TCP:
          return "tcp";
        case UDP:
          return "udp";
        default:
          return "unknown";
      }
    }
  }

  private ConnectivityManager mConnectivityManager = null;
  private WifiManager mWifiManager = null;
  private DhcpInfo mInfo = null;
  private WifiInfo mWifiInfo = null;
  private NetworkInterface mInterface = null;
  private IP4Address mGateway = null;
  private IP4Address mNetmask = null;
  private IP4Address mLocal = null;
  private IP4Address mBase = null;
  private Method mTetheredIfacesMethod = null;

  /**
   * see http://en.wikipedia.org/wiki/Reserved_IP_addresses
   */
  private static final String[] PRIVATE_NETWORKS = {
          "10.0.0.0/8",
          "100.64.0.0/10",
          "172.16.0.0/12",
          "192.168.0.0/16"
  };

  public Network(Context context, String iface) throws SocketException, UnknownHostException {
    mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
    mConnectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    mInfo = mWifiManager.getDhcpInfo();
    mWifiInfo = mWifiManager.getConnectionInfo();
    mLocal = new IP4Address(mInfo.ipAddress);
    mGateway = new IP4Address(mInfo.gateway);
    mNetmask = getNetmask();
    mBase = new IP4Address(mInfo.netmask & mInfo.gateway);
    mTetheredIfacesMethod = getTetheredIfacesMethod(mConnectivityManager);

    if (iface != null) {
      if (initNetworkInterface(iface))
        return;
    } else {
      for (String ifname : getAvailableInterfaces()) {
        if (initNetworkInterface(ifname)) {
          return;
        }
      }
    }

    throw new NoRouteToHostException("Not connected to any network.");
  }

  private static Method getTetheredIfacesMethod(ConnectivityManager connectivityManager) {
    try {
      return connectivityManager.getClass().getDeclaredMethod("getTetheredIfaces");
    } catch (NoSuchMethodException e) {
      Logger.warning("unable to get 'ConnectivityManager#getTetheredIfaces()': " + e.getMessage());
      return null;
    }
  }

  public boolean initNetworkInterface(String iface) {

    InterfaceAddress ifaceAddress = null;
    try {
      if (iface == null)
        iface = getAvailableInterfaces().get(0);

      mInterface = NetworkInterface.getByName(iface);

      if (mInterface.getInterfaceAddresses().isEmpty()) {
        return false;
      }

      for (InterfaceAddress ia : mInterface.getInterfaceAddresses()) {
        if(Patterns.IP_ADDRESS.matcher(ia.getAddress().getHostAddress()).matches()) {
          ifaceAddress = ia;
          Logger.warning("interfaceAddress: " + ia.getAddress().getHostAddress() + "/" + Short.toString(ia.getNetworkPrefixLength()));
          break;
        }
        else
          Logger.error("not valid ip: " + ia.getAddress().getHostAddress() + "/" + Short.toString(ia.getNetworkPrefixLength()));
      }
      if (ifaceAddress == null){
        return false;
      }

      SubnetUtils su = new SubnetUtils(
              // get(1) == ipv4
              ifaceAddress.getAddress().getHostAddress() +
                      "/" +
                      Short.toString(ifaceAddress.getNetworkPrefixLength()));

      mLocal = new IP4Address(su.getInfo().getAddress());
      mNetmask = new IP4Address(su.getInfo().getNetmask());
      mBase = new IP4Address(su.getInfo().getNetworkAddress());

      updateGateway();

      return true;
    } catch (Exception e) {
      Logger.error("Error: " + e.getLocalizedMessage());
    }

    return false;
  }

  private void updateGateway() throws UnknownHostException {
    String gateway = NetworkHelper.getIfaceGateway(mInterface.getDisplayName());

    if(gateway != null) {
      mGateway = new IP4Address(gateway);
    } else {
      Logger.warning("gateway not found");
    }
  }

  public void onCoreAttached() {
    if(haveGateway())
      return;

    try {
      updateGateway();
    } catch (UnknownHostException e) {
      Logger.warning(e.getMessage());
    }

    if(!haveGateway())
      return;

    Target gateway = new Target(getGatewayAddress(), getGatewayHardware());
    gateway.setAlias(getSSID());

    System.addOrderedTarget(gateway);
  }

  private IP4Address getNetmask() throws UnknownHostException {
    IP4Address result = new IP4Address(mInfo.netmask);

    if (System.getSettings().getBoolean("WIDE_SCAN", false)) {
      SubnetUtils privateNetwork;

      for (String cidr_notation : PRIVATE_NETWORKS) {
        privateNetwork = new SubnetUtils(cidr_notation);

        if (privateNetwork.getInfo().isInRange(mLocal.toString())) {
          result = new IP4Address(privateNetwork.getInfo().getNetmask());
          break;
        }
      }
    }

    return result;
  }

  public boolean equals(Network network) {
    return mInfo.equals(network.getInfo());
  }

  public boolean isInternal(byte[] address) {
    byte[] local = mLocal.toByteArray();
    byte[] mask = mNetmask.toByteArray();

    for (int i = 0; i < local.length; i++)
      if ((local[i] & mask[i]) != (address[i] & mask[i]))
        return false;

    return true;
  }

  public boolean isInternal(String ip) {
    try {
      return isInternal(InetAddress.getByName(ip).getAddress());
    } catch (UnknownHostException e) {
      Logger.error(e.getMessage());
    }
    return false;
  }

  public boolean isInternal(InetAddress address) {
    return isInternal(address.getAddress());
  }

  public static boolean isWifiConnected(Context context) {
    ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkInfo info = manager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);

    return info != null && info.isConnected() && info.isAvailable();
  }

  public static boolean isConnectivityAvailable(Context context) {
    ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkInfo info = manager.getActiveNetworkInfo();

    return info != null && info.isConnected();
  }

  public boolean isConnected() {
    return isIfaceConnected(mInterface);
  }

  public String getSSID() {
    String res = mWifiInfo.getSSID();
    return res.equals("<unknown ssid>") ? "" : res;
  }

  public int getNumberOfAddresses() {
    return IP4Address.ntohl(~mNetmask.toInteger());
  }

  public IP4Address getStartAddress() {
    return mBase;
  }

  public String getNetworkMasked() {
    SubnetUtils sub = new SubnetUtils(mLocal.toString(), mNetmask.toString());
    return sub.getInfo().getNetworkAddress();
  }

  public String getNetworkRepresentation() {
    return getNetworkMasked() + "/" + mNetmask.getPrefixLength();
  }

  public DhcpInfo getInfo() {
    return mInfo;
  }

  public InetAddress getNetmaskAddress() {
    return mNetmask.toInetAddress();
  }

  public boolean haveGateway() {
    return mGateway != null;
  }

  public boolean isTetheringEnabled() {
    if(mTetheredIfacesMethod == null) {
      return false;
    }
    try {
      String[] ifaces = (String[]) mTetheredIfacesMethod.invoke(mConnectivityManager);
      return ifaces.length > 0;
    } catch (Exception e) {
      Logger.error("unable to retrieve tethered ifaces: " + e.getMessage());
      return false;
    }
  }

  public InetAddress getGatewayAddress() {
    return mGateway == null ? null : mGateway.toInetAddress();
  }

  public byte[] getGatewayHardware() {
    return Endpoint.parseMacAddress(mWifiInfo.getBSSID());
  }

  @Nullable
  public byte[] getLocalHardware() {
    try {
      return mInterface.getHardwareAddress(); //FIXME: #831
    } catch (SocketException e) {
      System.errorLogging(e);
    }

    return null;
  }

  public String getLocalAddressAsString() {
    return mLocal.toString();
  }

  public InetAddress getLocalAddress() {
    return mLocal.toInetAddress();
  }

  @Override
  public int compareTo(@NonNull Network another) {
    if(mBase.equals(another.mBase)) {
      return mNetmask.getPrefixLength() - another.mNetmask.getPrefixLength();
    }
    return mBase.compareTo(another.mBase);
  }

  private static boolean isIfaceConnected(NetworkInterface networkInterface) {
    try {
      return networkInterface.isUp() && !networkInterface.isLoopback() &&
              !networkInterface.getInterfaceAddresses().isEmpty();
    } catch (SocketException e) {
      return false;
    }
  }

  /**
   * Retrieves a list of ready to use network interfaces.
   *
   * @return list of ready to use network interfaces
   */
  public static List<String> getAvailableInterfaces() {
    List<String> result;
    Enumeration<NetworkInterface> interfaces = null;

    try {
      interfaces = NetworkInterface.getNetworkInterfaces();
    } catch (SocketException e) {
      System.errorLogging(e);
    }

    if (interfaces == null)
      return Collections.emptyList();

    result = new ArrayList<>();

    while (interfaces.hasMoreElements()) {
      NetworkInterface iface = interfaces.nextElement();
      if (isIfaceConnected(iface)) {
        result.add(iface.getName());
      }
    }

    return result;
  }

  public NetworkInterface getInterface() {
    return mInterface;
  }
}