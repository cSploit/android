/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *             Massimo Dragano aka tux_mind <massimo.dragano@gmail.com>
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
package it.evilsocket.dsploit.net.Databases;

import it.evilsocket.dsploit.net.Target.Exploit;
import it.evilsocket.dsploit.net.metasploit.MsfExploit;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;

public class Metasploit
{
  private static MsfExploit search( String encoded_query )
  {
    String query = null;
    URLConnection  connection = null;
    MsfExploit ex = null;
    String new_location = null;

    try
    {
      query = "http://www.rapid7.com/db/search?q=" + encoded_query + "&t=m";
      URL obj = new URL(query);
      connection = obj.openConnection();
      connection.getHeaderField("Location"); // this will resolve connection
      new_location = connection.getURL().toString();

      if (new_location.equals(query)) {
        return null;
      }
      int i = new_location.indexOf("/modules/exploit/");

      if(i<0)
        return null;

      ex = new MsfExploit(new_location.substring(i+9),new_location);
    }
    catch( MalformedURLException mue )
    {
      mue.printStackTrace();
    }
    catch( IOException ioe )
    {
      ioe.printStackTrace();
    }
    return ex;
  }

  //Search by cve
  public static MsfExploit search_by_cve( String query )
  {
    return search(query);
  }


  //Search by osvdb
  public static MsfExploit search_by_osvdb( int data )
  {
    return search("" + data);
  }
}