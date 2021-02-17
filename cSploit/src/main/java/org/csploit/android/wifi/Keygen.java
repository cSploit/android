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
package org.csploit.android.wifi;

import android.net.wifi.ScanResult;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

public abstract class Keygen implements Comparable<Keygen>{
  // Constants used for different security types
  public static final String PSK = "PSK";
  public static final String WEP = "WEP";
  public static final String EAP = "EAP";
  public static final String OPEN = "Open";

  private ScanResult scanResult;
  final private String ssidName;
  final private String macAddress;
  final private int level;
  final private String encryption;
  private boolean stopRequested = false;
  private String errorMessage;
  private List<String> pwList;

  public Keygen(final String ssid, final String mac, int level, String enc){
    this.ssidName = ssid;
    this.macAddress = mac;
    this.level = level;
    this.encryption = enc;
    this.pwList = new ArrayList<String>();
  }

  public List<String> getResults(){
    return pwList;
  }

  public synchronized boolean isStopRequested(){
    return stopRequested;
  }

  public synchronized void setStopRequested(boolean stopRequested){
    this.stopRequested = stopRequested;
  }

  protected void addPassword(final String key){
    if(!pwList.contains(key))
      pwList.add(key);
  }

  public String getMacAddress(){
    return macAddress.replace(":", "");
  }

  public String getDisplayMacAddress(){
    return macAddress;
  }

  public String getSsidName(){
    return ssidName;
  }

  abstract public List<String> getKeys();

  public String getErrorMessage(){
    return errorMessage;
  }

  public void setErrorMessage(String error){
    this.errorMessage = error;
  }

  public boolean isSupported(){
    return true;
  }

  public int getLevel(){
    return level;
  }

  public String getEncryption(){
    return encryption;
  }

  public ScanResult getScanResult(){
    return scanResult;
  }

  public void setScanResult(ScanResult scanResult){
    this.scanResult = scanResult;
  }

  public int compareTo(Keygen another){
    if(isSupported() && another.isSupported()){
      if(another.level == this.level)
        return ssidName.compareTo(another.ssidName);
      else
        return another.level - level;
    } else if(isSupported())
      return -1;
    else
      return 1;
  }

  public int describeContents(){
    return 0;
  }

  public boolean isLocked(){
    return !OPEN.equals(getScanResultSecurity(this));
  }

  /**
   * @return The security of a given {@link ScanResult}.
   */
  public static String getScanResultSecurity(Keygen scanResult){
    final String cap = scanResult.encryption;
    final String[] securityModes = {WEP, PSK, EAP};
    for(int i = securityModes.length - 1; i >= 0; i--){
      if(cap.contains(securityModes[i])){
        return securityModes[i];
      }
    }

    return OPEN;
  }

  static public String dectoString(int mac){
    String ret = "";
    while(mac > 0){
      switch(mac % 10){
        case 0:
          ret = "Zero" + ret;
          break;
        case 1:
          ret = "One" + ret;
          break;
        case 2:
          ret = "Two" + ret;
          break;
        case 3:
          ret = "Three" + ret;
          break;
        case 4:
          ret = "Four" + ret;
          break;
        case 5:
          ret = "Five" + ret;
          break;
        case 6:
          ret = "Six" + ret;
          break;
        case 7:
          ret = "Seven" + ret;
          break;
        case 8:
          ret = "Eight" + ret;
          break;
        case 9:
          ret = "Nine" + ret;
          break;
      }
      mac /= 10;
    }
    return ret;
  }

  static final byte[] HEX_CHAR_TABLE = {(byte) '0', (byte) '1', (byte) '2',
    (byte) '3', (byte) '4', (byte) '5', (byte) '6', (byte) '7',
    (byte) '8', (byte) '9', (byte) 'a', (byte) 'b', (byte) 'c',
    (byte) 'd', (byte) 'e', (byte) 'f'};

  public static String getHexString(byte[] raw)
    throws UnsupportedEncodingException{
    byte[] hex = new byte[2 * raw.length];
    int index = 0;

    for(byte b : raw){
      int v = b & 0xFF;
      hex[index++] = HEX_CHAR_TABLE[v >>> 4];
      hex[index++] = HEX_CHAR_TABLE[v & 0xF];
    }
    return new String(hex, "ASCII");
  }

  public static String getHexString(short[] raw)
    throws UnsupportedEncodingException{
    byte[] hex = new byte[2 * raw.length];
    int index = 0;

    for(short b : raw){
      int v = b & 0xFF;
      hex[index++] = HEX_CHAR_TABLE[v >>> 4];
      hex[index++] = HEX_CHAR_TABLE[v & 0xF];
    }
    return new String(hex, "ASCII");
  }

  public static String getHexString(short raw){
    byte[] hex = new byte[2];
    int v = raw & 0xFF;
    hex[0] = HEX_CHAR_TABLE[v >>> 4];
    hex[1] = HEX_CHAR_TABLE[v & 0xF];
    try{
      return new String(hex, "ASCII");
    } catch(UnsupportedEncodingException ignored){
    }
    return "";
  }
}
