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

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.List;

import org.csploit.android.wifi.Keygen;

/**
 * The link for this algorithm is:
 * http://sviehb.wordpress.com/2011/12/04/prg-eav4202n-default-wpa-key-algorithm/
 *
 * @author Rui Araújo
 */
public class PBSKeygen extends Keygen{

  transient private MessageDigest md;

  public PBSKeygen(String ssid, String mac, int level, String enc){
    super(ssid, mac, level, enc);
  }

  final static byte[] saltSHA256 =
    {0x54, 0x45, 0x4F, 0x74, 0x65, 0x6C, (byte) 0xB6,
      (byte) 0xD9, (byte) 0x86, (byte) 0x96, (byte) 0x8D,
      0x34, 0x45, (byte) 0xD2, 0x3B, 0x15,
      (byte) 0xCA, (byte) 0xAF, 0x12, (byte) 0x84, 0x02,
      (byte) 0xAC, 0x56, 0x00, 0x05, (byte) 0xCE, 0x20, 0x75,
      (byte) 0x94, 0x3F, (byte) 0xDC, (byte) 0xE8};

  final static String lookup = "0123456789ABCDEFGHIKJLMNOPQRSTUVWXYZabcdefghikjlmnopqrstuvwxyz";

  @Override
  public List<String> getKeys(){
    try{
      md = MessageDigest.getInstance("SHA-256");
    } catch(NoSuchAlgorithmException e1){
      setErrorMessage("This phone cannot process a SHA256 hash.");
      return null;
    }
    final String mac = getMacAddress();
    if(mac.length() != 12){
      setErrorMessage("The MAC address is invalid.");
      return null;
    }

    byte[] macHex = new byte[6];
    for(int i = 0; i < 12; i += 2)
      macHex[i / 2] = (byte) ((Character.digit(mac.charAt(i), 16) << 4)
        + Character.digit(mac.charAt(i + 1), 16));

    md.reset();
    md.update(saltSHA256);
    md.update(macHex);
    byte[] hash = md.digest();
    StringBuilder key = new StringBuilder();
    for(int i = 0; i < 13; ++i){
      key.append(lookup.charAt((hash[i] >= 0 ? hash[i] : 256 + hash[i]) % lookup.length()));
    }
    addPassword(key.toString());
    return getResults();
  }
}
