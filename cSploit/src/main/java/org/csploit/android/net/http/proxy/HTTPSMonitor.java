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
package org.csploit.android.net.http.proxy;

import java.util.ArrayList;
import java.util.HashMap;

public class HTTPSMonitor{
  private static HTTPSMonitor mInstance = null;

  private HashMap<String, ArrayList<String>> mMap = null;

  public static HTTPSMonitor getInstance(){
    if(mInstance == null)
      mInstance = new HTTPSMonitor();

    return mInstance;
  }

  public HTTPSMonitor(){
    mMap = new HashMap<String, ArrayList<String>>();
  }

  public void addURL(String client, String url){
    if(mMap.containsKey(client) == false)
      mMap.put(client, new ArrayList<String>());

    mMap.get(client).add(url);
  }

  public boolean hasURL(String client, String url){
    ArrayList<String> urls = mMap.get(client);
    if(urls != null)
      return urls.contains(url);
    else
      return false;
  }
}
