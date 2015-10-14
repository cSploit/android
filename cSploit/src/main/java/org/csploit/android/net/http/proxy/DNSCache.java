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

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.util.HashMap;

import javax.net.SocketFactory;

import org.csploit.android.core.Logger;

public class DNSCache
{
  private static DNSCache mInstance = null;

  private HashMap<String, InetAddress> mCache = null;

  public static DNSCache getInstance(){
    if(mInstance == null)
      mInstance = new DNSCache();

    return mInstance;
  }

  public DNSCache(){
    mCache = new HashMap<String, InetAddress>();
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
