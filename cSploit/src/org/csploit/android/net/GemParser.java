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

package org.csploit.android.net;

import org.joda.time.DateTime;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Date;

/**
 * This class parses JSON containing modification time of uploaded gems
 */
public class GemParser {

  /**
   * class that holds info about remote gem
   */
  public static class RemoteGemInfo {
    public final String name;
    public final String version;
    public final Date uploaded;

    private RemoteGemInfo(String name, String version, Date uploaded) {
      this.name = name;
      this.version = version;
      this.uploaded = uploaded;
    }
  }

  private final String mUrl;
  private RemoteGemInfo[] mGems = null;
  private RemoteGemInfo[] mOldGems = null;

  public GemParser (String url) {
    mUrl = url;
  }

  private void fetch() throws JSONException, IOException {
    HttpURLConnection connection = null;
    BufferedReader reader = null;

    try {
      HttpURLConnection.setFollowRedirects(true);
      connection = (HttpURLConnection) (new URL(mUrl)).openConnection();
      connection.connect();
      int ret = connection.getResponseCode();
      if (ret != 200)
        throw new IOException("cannot retrieve remote json: " + ret);
      reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
      StringBuilder sb = new StringBuilder();
      String line;

      while ((line = reader.readLine()) != null)
        sb.append(line);
      JSONArray jGems = new JSONArray(sb.toString());
      mGems = new RemoteGemInfo[jGems.length()];
      for (int i = 0; i < jGems.length(); i++) {
        JSONObject gem = jGems.getJSONObject(i);
        mGems[i] = new RemoteGemInfo(
                gem.getString("name"),
                gem.getString("version"),
                (new DateTime(gem.getString("uploaded"))).toDate()
        );
      }
    } finally {
      if(reader!=null)
        reader.close();
      if(connection!=null)
        connection.disconnect();
    }
  }

  public RemoteGemInfo[] parse() throws IOException, JSONException {
    if(mGems == null)
      fetch();
    return mGems;
  }

  public void setOldGems(RemoteGemInfo[] oldGems) {
    mOldGems = oldGems;
  }

  public RemoteGemInfo[] getGemsToUpdate() {
    return mOldGems;
  }

  public void reset() {
    mGems = null;
    mOldGems = null;
  }
}
