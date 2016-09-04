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

import android.text.TextUtils;

/*
 * The algorithm for the type of network
 * whose SSID must be in the form of [pP]1XXXXXX0000X
 * where X means a digit.
 * Algorithm:
 * Adding +1 to the last digit and use the resulting 
 * string as the passphrase for WEP key generation.
 * Use the first of the 64 bit keys and the 128 bit one
 * as possible keys.
 * Credit:
 *  pulido from http://foro.elhacker.net
 *  http://foro.elhacker.net/hacking_wireless/desencriptando_wep_por_defecto_de_las_redes_ono_wifi_instantaneamente-t160928.0.html
 * */
public class OnoKeygen extends Keygen{

  private MessageDigest md;

  public OnoKeygen(String ssid, String mac, int level, String enc){
    super(ssid, mac, level, enc);
  }

  @Override
  public List<String> getKeys(){
    if(getSsidName().length() != 13){
      setErrorMessage("Invalid ESSID! It must have 13 characters.");
      return null;
    }
    String val = getSsidName().substring(0, 11) +
      Integer.toString(Integer.parseInt(getSsidName().substring(11)) + 1);
    if(val.length() < 13)
      val = getSsidName().substring(0, 11) + "0" + getSsidName().substring(11);
    int[] pseed = new int[4];
    pseed[0] = 0;
    pseed[1] = 0;
    pseed[2] = 0;
    pseed[3] = 0;
    int randNumber = 0;
    String key = "";
    for(int i = 0; i < val.length(); i++){
      pseed[i % 4] ^= (int) val.charAt(i);
    }
    randNumber = pseed[0] | (pseed[1] << 8) | (pseed[2] << 16) | (pseed[3] << 24);
    short tmp = 0;
    for(int j = 0; j < 5; j++){
      randNumber = (randNumber * 0x343fd + 0x269ec3) & 0xffffffff;
      tmp = (short) ((randNumber >> 16) & 0xff);
      key += getHexString(tmp).toUpperCase();
    }
    addPassword(key);
    key = "";
    try{
      md = MessageDigest.getInstance("MD5");
    } catch(NoSuchAlgorithmException e1){
      setErrorMessage("This phone cannot process a MD5 hash");
      return null;
    }
    md.reset();
    md.update(padto64(val).getBytes());
    byte[] hash = md.digest();
    for(int i = 0; i < 13; ++i)
      key += getHexString((short) hash[i]);
    addPassword(key.toUpperCase());
    return getResults();
  }


  private String padto64(String val){
    if (TextUtils.isEmpty(val))
      return "";

    final StringBuilder retBuilder = new StringBuilder();
    for(int i = 0; i < (1 + (64 / (val.length()))); ++i)
      retBuilder.append(val);

    return retBuilder.toString().substring(0, 64);
  }
}
