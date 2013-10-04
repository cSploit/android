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
package it.evilsocket.dsploit.core;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class UpdateManager
{
  private static final String REMOTE_VERSION_URL = "http://update.dsploit.net/version";
  private static final String REMOTE_DOWNLOAD_URL = "http://update.dsploit.net/apk";
  private static final String VERSION_CHAR_MAP = "zyxwvutsrqponmlkjihgfedcba";

  private String mInstalledVersion = null;
  private String mRemoteVersion = null;

  public UpdateManager(){
    mInstalledVersion = System.getAppVersionName();
  }

  private static double getVersionCode(String version){
    double code = 0,
           multipliers[] = { 1000, 100, 1 };
    String parts[] = version.split( "[^0-9a-zA-Z]", 3 ),
           item, digit, letter;
    int i, j;
    char c;

    for( i = 0; i < 3; i++ )
    {
      item = parts[i];

      if( item.matches("\\d+[a-zA-Z]")){
        digit = "";
        letter = "";

        for(j = 0; j < item.length(); j++){
          c = item.charAt(j);
          if(c >= '0' && c <= '9')
            digit += c;
          else
            letter += c;
        }

        code += multipliers[i] * ( Integer.parseInt(digit) + 1 ) - ((VERSION_CHAR_MAP.indexOf(letter.toLowerCase()) + 1) / 100.0);
      }
      else if(item.matches("\\d+"))
        code += multipliers[i] * ( Integer.parseInt(item) + 1 );

      else
        code += multipliers[i];
    }

    return code;
  }

  public boolean isUpdateAvailable(){

    try{
      if(mInstalledVersion != null){
        // Read remote version
        if(mRemoteVersion == null){
          URL url = new URL(REMOTE_VERSION_URL);
          HttpURLConnection connection = (HttpURLConnection) url.openConnection();
          BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
          String line,
            buffer = "";

          while((line = reader.readLine()) != null){
            buffer += line + "\n";
          }

          reader.close();

          mRemoteVersion = buffer.trim();
        }

        // Compare versions
        double installedVersionCode = getVersionCode(mInstalledVersion),
          remoteVersionCode = getVersionCode(mRemoteVersion);

        Logger.debug( "mInstalledVersion = " + mInstalledVersion + " ( " + getVersionCode(mInstalledVersion)+ " ) " );
        Logger.debug( "mRemoteVersion    = " + mRemoteVersion + " ( " + getVersionCode(mRemoteVersion)+ " ) " );

        if(remoteVersionCode > installedVersionCode)
          return true;
      }
    } catch(Exception e){
      System.errorLogging(e);
    }

    return false;
  }

  public String getRemoteVersion(){
    return mRemoteVersion;
  }

  public String getRemoteVersionFileName(){
    return "dSploit-" + mRemoteVersion + ".apk";
  }

  public String getRemoteVersionUrl(){
    return REMOTE_DOWNLOAD_URL;
  }
}
