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

import android.util.Xml;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.LinkedList;

/**
 * This class parses feed (XML/atom) containing modification time of uploaded gems
 */
public class GemParser {

  /**
   * class that holds info about remote gem
   */
  public static class RemoteGemInfo {
    public String name;
    public String version;
    public Date uploaded;
  }

  private final String mUrl;
  private RemoteGemInfo[] mGems = null;
  private RemoteGemInfo[] mOldGems = null;

  public GemParser (String url) {
    mUrl = url;
  }

  private void fetch() throws ParseException, XmlPullParserException, IOException {
    HttpURLConnection connection = null;
    InputStream stream = null;

    try {
      HttpURLConnection.setFollowRedirects(true);
      connection = (HttpURLConnection) (new URL(mUrl)).openConnection();
      connection.connect();
      int ret = connection.getResponseCode();
      if (ret != 200)
        throw new IOException("cannot retrieve remote json: " + ret);

      stream = connection.getInputStream();
      XmlPullParser parser = Xml.newPullParser();
      parser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, false);
      parser.setInput(stream, null);
      parser.nextTag();
      parser.require(XmlPullParser.START_TAG, null, "feed");

      RemoteGemInfo gemInfo = null;
      SimpleDateFormat parsingDateFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ssZ");

      LinkedList<RemoteGemInfo> gems = new LinkedList<RemoteGemInfo>();

      while(parser.next() != XmlPullParser.END_DOCUMENT) {

        if(parser.getEventType() == XmlPullParser.END_TAG && parser.getName().equals("feed"))
          break;

        if(parser.getEventType() == XmlPullParser.START_TAG && parser.getName().equals("entry"))
          gemInfo = new RemoteGemInfo();

        if(gemInfo != null && gemInfo.name != null && parser.getEventType() == XmlPullParser.END_TAG && parser.getName().equals("entry")) {
          gems.add(gemInfo);
          gemInfo = null;
        }

        if(gemInfo != null && parser.getEventType() == XmlPullParser.START_TAG && parser.getName().equals("title"))
        {
          String[] titleParts = parser.nextText().split(" *[(\\-)] *");
          gemInfo.name = titleParts[0];
          gemInfo.version = titleParts[titleParts.length-1];
        }

        if(gemInfo != null && parser.getEventType() == XmlPullParser.START_TAG && parser.getName().equals("updated")) {
          //convert UTC shortcut to offset. Android doesn't support Format X
          String datetime = parser.nextText().replaceFirst("Z$","+00");
          gemInfo.uploaded = parsingDateFormat.parse(datetime);
        }

      }

      mGems = gems.toArray(new RemoteGemInfo[gems.size()]);
    } finally {
      if(stream!=null)
        stream.close();
      if(connection!=null)
        connection.disconnect();
    }
  }

  public RemoteGemInfo[] parse() throws IOException, XmlPullParserException, ParseException {
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
