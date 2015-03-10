package org.csploit.android.net;

import org.apache.commons.net.io.Util;
import org.csploit.android.core.*;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;

/**
 * Fetch content from remote sources.
 */
public class RemoteFetcher extends Thread {
  private final String mUrl;
  private byte[] content;

  public RemoteFetcher(String url) {
    mUrl = url;
    content = null;
  }

  private static URLConnection openConnection(String url) throws IOException {
    HttpURLConnection.setFollowRedirects(true);

    URLConnection connection = new URL(url).openConnection();

    connection.connect();

    if(connection instanceof HttpURLConnection) {
      int ret = ((HttpURLConnection) connection).getResponseCode();

      if(ret != 200) {
        ((HttpURLConnection) connection).disconnect();
        throw new IOException(
                String.format("[%s] HTTP response code: %d", url, ret));
      }
    }

    return connection;
  }

  public static byte[] fetch(String url) throws IOException {
    InputStream in = openConnection(url).getInputStream();
    ByteArrayOutputStream out = new ByteArrayOutputStream();

    try {
      Util.copyStream(in, out);

      return out.toByteArray();
    } finally {
      in.close();
      out.close();
    }
  }

  public static InputStream getInputStream(String url) throws IOException {
    return openConnection(url).getInputStream();
  }

  @Override
  public void run() {
    try {
      content = fetch(mUrl);
    } catch (IOException e) {
      Logger.error(e.getMessage());
    }
  }

  public byte[] getContent() {
    if(content == null) {
      try {
        content = fetch(mUrl);
      } catch (IOException e) {
        Logger.error(e.getMessage());
      }
    }
    return content;
  }
}
