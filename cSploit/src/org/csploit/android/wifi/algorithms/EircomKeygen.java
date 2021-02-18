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

import java.io.UnsupportedEncodingException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.List;

import org.csploit.android.wifi.Keygen;

/*
 * Eircom algorithm published here:
 * http://www.bacik.org/eircomwep/howto.html
 */
public class EircomKeygen extends Keygen{

  private MessageDigest md;

  public EircomKeygen(String ssid, String mac, int level, String enc){
    super(ssid, mac, level, enc);
  }

  @Override
  public List<String> getKeys(){
    String mac = getMacAddress().substring(6);
    try{
      md = MessageDigest.getInstance("SHA1");
    } catch(NoSuchAlgorithmException e1){
      setErrorMessage("This phone cannot process a SHA1 hash.");
      return null;
    }
    byte[] routerMAC = new byte[4];
    routerMAC[0] = 1;
    for(int i = 0; i < 6; i += 2)
      routerMAC[i / 2 + 1] = (byte) ((Character.digit(mac.charAt(i), 16) << 4)
        + Character.digit(mac.charAt(i + 1), 16));
    int macDec = ((0xFF & routerMAC[0]) << 24) | ((0xFF & routerMAC[1]) << 16) |
      ((0xFF & routerMAC[2]) << 8) | (0xFF & routerMAC[3]);
    mac = dectoString(macDec) + "Although your world wonders me, ";
    md.reset();
    md.update(mac.getBytes());
    byte[] hash = md.digest();
    try{
      addPassword(getHexString(hash).substring(0, 26));
    } catch(UnsupportedEncodingException e){
      e.printStackTrace();
    }
    return getResults();
  }
}
