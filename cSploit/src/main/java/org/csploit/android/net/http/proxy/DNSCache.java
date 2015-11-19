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

import org.csploit.android.core.Logger;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;

import javax.net.SocketFactory;

public class DNSCache
{
  private static DNSCache mInstance = new DNSCache();

  private HashMap<String, InetAddress> mCache = null;
  private ArrayList<String> mCachedRootDomain = null;

  public static DNSCache getInstance(){
    return mInstance;
  }

  private DNSCache() {
    mCache = new HashMap<>();
    mCachedRootDomain = new ArrayList<>();
  }

  /**
   * checks if a domain ends with an already intercepted root domain.
   *
   * @param hostname hostname to check
   * @return String the root domain or null
   */
  public String getCachedRootDomain (String hostname){
    for (String rootDomain : mCachedRootDomain){
      if (hostname.endsWith(rootDomain)){
        return rootDomain;
      }
    }

    return null;
  }

  /**
   * Adds a root domain extracted from the domain of a request,
   * to the list of intercepted root domains.
   *
   * @param rootdomain Root domain to add to the list
   */
  public void addCachedRootDomain (String rootdomain){
    mCachedRootDomain.add(rootdomain);
  }

  private InetAddress getAddress(String server) throws IOException{
    InetAddress address = mCache.get(server);

    if(address == null){
      address = InetAddress.getByName(server);
      mCache.put(server, address);

      Logger.debug(server + " resolved to " + address.getHostAddress());
    }
    else
      Logger.debug("Returning a cached DSN result for " + server + " : " + address.getHostAddress());

    return address;
  }

  public Socket connect(String server, int port) throws IOException{
    return new Socket(getAddress(server), port);
  }

  public Socket connect(SocketFactory factory, String server, int port) throws IOException{
    return factory.createSocket(getAddress(server), port);
  }
}
