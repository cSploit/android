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
 * This is the algorithm to generate the WPA passphrase 
 * for the Hitachi (TECOM) AH-4021 and Hitachi (TECOM) AH-4222.
 * The key is the 26 first characters from the SSID SHA1 hash.
 *  Link : http://rafale.org/~mattoufoutu/ebooks/Rafale-Mag/Rafale12/Rafale12.08.HTML
 */
public class TecomKeygen extends Keygen{

  private MessageDigest md;

  public TecomKeygen(String ssid, String mac, int level, String enc){
    super(ssid, mac, level, enc);
  }

  @Override
  public List<String> getKeys(){
    try{
      md = MessageDigest.getInstance("SHA1");
    } catch(NoSuchAlgorithmException e1){
      setErrorMessage("This phone cannot process a SHA1 hash.");
      return null;
    }
    md.reset();
    md.update(getSsidName().getBytes());
    byte[] hash = md.digest();
    try{
      addPassword(getHexString(hash).substring(0, 26));
    } catch(UnsupportedEncodingException e){
      e.printStackTrace();
    }
    return getResults();
  }
}
