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

import org.csploit.android.net.http.RequestParser;

public class CookieCleaner{
  private static CookieCleaner mInstance = null;

  private HashMap<String, ArrayList<String>> mMap = null;

  public static CookieCleaner getInstance(){
    if(mInstance == null)
      mInstance = new CookieCleaner();

    return mInstance;
  }

  public CookieCleaner(){
    mMap = new HashMap<String, ArrayList<String>>();
  }

  public boolean isClean(String client, String hostname, String request){
    if(request.startsWith("POST "))
      return true;

    else if(request.contains("Cookie:") == false)
      return true;

    else{
      String domain = RequestParser.getBaseDomain(hostname);
      ArrayList<String> domains = mMap.get(client);

      return (domains != null && domains.contains(domain));
    }
  }

  public String getExpiredResponse(String request, String hostname){
    String domain = RequestParser.getBaseDomain(hostname),
      response = "HTTP/1.1 302 Found\n";

    for(String line : request.split("\n")){
      if(line.indexOf(':') != -1){
        String[] split = line.split(":", 2);
        String header = split[0].trim(),
          value = split[1].trim();

        if(header.equals("Cookie")){
          String[] cookies = value.split(";");
          for(String cookie : cookies){
            split = cookie.split("=");

            if(split.length == 2){
              cookie = split[0].trim();

              response += "Set-Cookie: " + cookie + "=EXPIRED;Path=/;Domain=" + domain + ";Expires=Mon, 01-Jan-1990 00:00:00 GMT\n" +
                "Set-Cookie: " + cookie + "=EXPIRED;Path=/;Domain=" + hostname + ";Expires=Mon, 01-Jan-1990 00:00:00 GMT\n";
            }
          }
        }
      }
    }

    response += "Location: " + RequestParser.getUrlFromRequest(hostname, request) + "\n" +
      "Connection: close\n\n";

    return response;
  }

  public void addCleaned(String client, String hostname){
    String domain = RequestParser.getBaseDomain(hostname);

    if(mMap.containsKey(client) == false)
      mMap.put(client, new ArrayList<String>());

    mMap.get(client).add(domain);
  }
}
