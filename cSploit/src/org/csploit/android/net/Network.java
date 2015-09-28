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

import org.apache.commons.net.util.SubnetUtils;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.NoRouteToHostException;
import java.net.SocketException;
import java.net.UnknownHostException;

import org.csploit.android.core.Logger;
import org.csploit.android.core.System;

public class Network
{
  public enum Protocol{
    TCP,
    UDP,
    ICMP,
    IGMP,
    UNKNOWN;

    public static Protocol fromString(String proto){

      if(proto != null){
        proto = proto.toLowerCase();

        if(proto.equals("tcp"))
          return TCP;

        else if(proto.equals("udp"))
          return UDP;

        else if(proto.equals("icmp"))
          return ICMP;

        else if(proto.equals("igmp"))
          return IGMP;
      }

      return UNKNOWN;
    }

    public String toString()
    {
      switch(this)
      {
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

  /** see http://en.wikipedia.org/wiki/Reserved_IP_addresses
   */
  private static final String[] PRIVATE_NETWORKS = {
          "10.0.0.0/8",
          "100.64.0.0/10",
          "172.16.0.0/12",
          "192.168.0.0/16"
  };

  public Network(Context context) throws SocketException, UnknownHostException{
    mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
    mConnectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    mInfo = mWifiManager.getDhcpInfo();
    mWifiInfo = mWifiManager.getConnectionInfo();
    mLocal = new IP4Address(mInfo.ipAddress);
    mGateway = new IP4Address(mInfo.gateway);
    mNetmask = getNetmask();
    mBase = new IP4Address(mInfo.netmask & mInfo.gateway);

    if(isConnected() == false)
      throw new NoRouteToHostException("Not connected to any WiFi access point.");

    else{
      try{
        mInterface = NetworkInterface.getByInetAddress(getLocalAddress());
        if(mInterface == null)
          throw new IllegalStateException("Error retrieving network interface.");
      }
      catch(SocketException e){
        System.errorLogging(e);
        /*
         * Issue #26: Initialization error in ColdFusionX ROM
				 * 
				 * It seems it's a ROM issue which doesn't correctly populate device descriptors.
				 * This rom maps the default wifi interface to a generic usb device 
				 * ( maybe it's missing the specific interface driver ), which is obviously not, and
				 * it all goes shit, use an alternative method to obtain the interface object.
				 */
        mInterface = NetworkInterface.getByName(java.lang.System.getProperty("wifi.interface", "wlan0"));

        if(mInterface == null)
          throw e;
      }
    }
  }

  private IP4Address getNetmask() throws UnknownHostException {
    IP4Address result = new IP4Address(mInfo.netmask);

    if(System.getSettings().getBoolean("WIDE_SCAN", false)) {
      SubnetUtils privateNetwork;

      for(String cidr_notation : PRIVATE_NETWORKS) {
        privateNetwork = new SubnetUtils(cidr_notation);

        if(privateNetwork.getInfo().isInRange(mLocal.toString())) {
          result = new IP4Address(privateNetwork.getInfo().getNetmask());
          break;
        }
      }
    }

    return result;
  }

  public boolean equals(Network network){
    return mInfo.equals(network.getInfo());
  }

  public boolean isInternal(byte[] address) {
    byte[] gateway = mGateway.toByteArray();
    byte[] mask = mNetmask.toByteArray();

    for(int i = 0; i < gateway.length; i++)
      if((gateway[i] & mask[i]) != (address[i] & mask[i]))
        return false;

    return true;
  }

  public boolean isInternal(String ip){
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

  public static boolean isWifiConnected(Context context){
    ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkInfo info = manager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);

    return info != null && info.isConnected() && info.isAvailable();
  }

  public static boolean isConnectivityAvailable(Context context){
    ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    NetworkInfo info = manager.getActiveNetworkInfo();

    return info != null && info.isConnected();
  }

  public boolean isConnected(){
    return mConnectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI).isConnected();
  }

  public String getSSID(){
    return mWifiInfo.getSSID();
  }

  public int getNumberOfAddresses(){
    return IP4Address.ntohl(~mNetmask.toInteger());
  }

  public IP4Address getStartAddress(){
    return mBase;
  }

  public String getNetworkMasked(){
    int network = mBase.toInteger();

    return (network & 0xFF) + "." + ((network >> 8) & 0xFF) + "." + ((network >> 16) & 0xFF) + "." + ((network >> 24) & 0xFF);
  }

  public String getNetworkRepresentation(){
    return getNetworkMasked() + "/" + mNetmask.getPrefixLength();
  }

  public DhcpInfo getInfo(){
    return mInfo;
  }

  public InetAddress getNetmaskAddress(){
    return mNetmask.toInetAddress();
  }

  public InetAddress getGatewayAddress(){
    return mGateway.toInetAddress();
  }

  public byte[] getGatewayHardware(){
    return Endpoint.parseMacAddress(mWifiInfo.getBSSID());
  }

  public byte[] getLocalHardware(){
    try{
      return mInterface.getHardwareAddress();
    }
    catch(SocketException e){
      System.errorLogging(e);
    }

    return null;
  }

  public String getLocalAddressAsString(){
    return mLocal.toString();
  }

  public InetAddress getLocalAddress(){
    return mLocal.toInetAddress();
  }

  public NetworkInterface getInterface(){
    return mInterface;
  }
}
