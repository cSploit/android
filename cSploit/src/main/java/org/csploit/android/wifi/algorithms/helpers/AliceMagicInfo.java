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
package org.csploit.android.wifi.algorithms.helpers;

import android.os.Parcel;
import android.os.Parcelable;

import java.util.Arrays;

public class AliceMagicInfo implements Parcelable{
  final private String alice;
  final private int[] magic;
  final private String serial;
  final private String mac;

  public AliceMagicInfo(String alice, int[] magic, String serial, String mac){
    this.alice = alice;
    this.magic = Arrays.copyOf(magic, magic.length);
    this.serial = serial;
    this.mac = mac;
  }

  public String getAlice(){
    return alice;
  }

  public int[] getMagic(){
    return Arrays.copyOf(magic, magic.length);
  }

  public String getSerial(){
    return serial;
  }

  public String getMac(){
    return mac;
  }

  public int describeContents(){
    return 0;
  }

  public void writeToParcel(Parcel dest, int flags){
    dest.writeString(alice);
    dest.writeString(serial);
    dest.writeString(mac);
    dest.writeIntArray(magic);
  }

  private AliceMagicInfo(Parcel in){
    this.alice = in.readString();
    this.serial = in.readString();
    this.mac = in.readString();
    this.magic = in.createIntArray();
  }

  public static final Parcelable.Creator<AliceMagicInfo> CREATOR = new Parcelable.Creator<AliceMagicInfo>(){
    public AliceMagicInfo createFromParcel(Parcel in){
      return new AliceMagicInfo(in);
    }

    public AliceMagicInfo[] newArray(int size){
      return new AliceMagicInfo[size];
    }
  };
}
