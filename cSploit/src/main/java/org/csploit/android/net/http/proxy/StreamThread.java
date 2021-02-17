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
import org.csploit.android.core.Profiler;
import org.csploit.android.core.System;
import org.csploit.android.net.ByteBuffer;
import org.csploit.android.net.http.RequestParser;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;

public class StreamThread implements Runnable
{
  private final static String[] FILTERED_CONTENT_TYPES = new String[]
    {
      "/html",
      "/css",
      "/javascript",
      "/json",
      "/x-javascript",
      "/x-json"
    };

  private final static String HEAD_SEPARATOR = "\r\n\r\n";
  private final static int CHUNK_SIZE = 64 * 1024;

  private String mClient = null;
  private InputStream mReader = null;
  private OutputStream mWriter = null;
  private ByteBuffer mBuffer = null;
  private Proxy.ProxyFilter mFilter = null;

  public StreamThread(String client, InputStream reader, OutputStream writer, Proxy.ProxyFilter filter){
    mClient = client;
    mReader = reader;
    mWriter = writer;
    mBuffer = new ByteBuffer();
    mFilter = filter;

    new Thread(this).start();
  }

  public void run(){

    int read = -1,
      size = 0,
      max = 0;
    byte[] chunk = new byte[CHUNK_SIZE];

    try{
      max = Integer.parseInt(System.getSettings().getString("PREF_HTTP_MAX_BUFFER_SIZE", "10485760"));
    } catch(NumberFormatException e){
      max = 10485760;
    }

    try{
      String location = null,
        contentType = null;
      boolean wasContentTypeChecked = false,
        isHandledContentType = false;

      Profiler.instance().profile("chunk read");

      while((read = mReader.read(chunk, 0, CHUNK_SIZE)) > 0 && size < max){
        Profiler.instance().emit();

        mBuffer.append(chunk, read);
        size += read;

        location = location == null ? RequestParser.getHeaderValue(RequestParser.LOCATION_HEADER, mBuffer) : location;
        contentType = contentType == null ? RequestParser.getHeaderValue(RequestParser.CONTENT_TYPE_HEADER, mBuffer) : contentType;

        if(contentType != null && wasContentTypeChecked == false){
          wasContentTypeChecked = true;
          isHandledContentType = false;

          for(String handled : FILTERED_CONTENT_TYPES){
            if(contentType.contains(handled)){
              isHandledContentType = true;
              break;
            }
          }

          // not handled content type, start fast streaming
          if(isHandledContentType == false){
            Profiler.instance().profile("Fast streaming");

            Logger.debug("Content type " + contentType + " not handled, start fast streaming ...");

            mWriter.write(mBuffer.getData());
            mWriter.flush();

            while((read = mReader.read(chunk, 0, CHUNK_SIZE)) > 0){
              mWriter.write(chunk, 0, read);
              mWriter.flush();
            }

            mWriter.close();
            mReader.close();

            Profiler.instance().emit();

            return;
          }
        }
      }

      // if we are here, this means we have a document to be filtered
      // ( handled content type )
      if(mBuffer.isEmpty() == false){
        Profiler.instance().profile("content filtering");

        String data = mBuffer.toString();
        String[] split = data.split(HEAD_SEPARATOR, 2);
        String headers = split[0];

        // handle relocations for https support
        if(location != null && location.startsWith("https://") && System.getSettings().getBoolean("PREF_HTTPS_REDIRECT", true) == true){
          Logger.warning("Patching 302 HTTPS redirect : " + location);

          // update variables for further filtering
          mBuffer.replace("Location: https://".getBytes(), "Location: http://".getBytes());

          data = mBuffer.toString();
          split = data.split(HEAD_SEPARATOR, 2);
          headers = split[0];

          HTTPSMonitor.getInstance().addURL(mClient, location.replace("https://", "http://").replace("&amp;", "&"));
        }

        String body = (split.length > 1 ? split[1] : "");

        final StringBuilder patchedBuilder = new StringBuilder();

        body = mFilter.onDataReceived(headers, body);

        // remove explicit content length, just in case the body changed after filtering
        for(String header : headers.split("\n")){
          if(header.toLowerCase().contains("content-length") == false)
          patchedBuilder.append(header).append("\n");
        }

        headers = patchedBuilder.toString();

        // try to get the charset encoding from the HTTP headers.
        String charset = RequestParser.getCharsetFromHeaders(contentType);

        // if we haven't found the charset encoding on the HTTP headers, try it out on the body.
        if (charset == null) {
          charset = RequestParser.getCharsetFromBody(body);
        }

        if (charset != null) {
          try {
            mBuffer.setData((headers + HEAD_SEPARATOR + body).getBytes(charset));
          }
          catch (UnsupportedEncodingException e){
            Logger.error("UnsupportedEncoding: " + e.getLocalizedMessage());
            mBuffer.setData((headers + HEAD_SEPARATOR + body).getBytes());
          }
        }
        else {
          // if we haven't found the charset encoding, just handle it on ByteBuffer()
          mBuffer.setData((headers + HEAD_SEPARATOR + body).getBytes());
        }

        mWriter.write(mBuffer.getData());
        mWriter.flush();

        Profiler.instance().emit();
      }
    }
    catch(OutOfMemoryError ome){
      Logger.error(ome.toString());
    }
    catch(Exception e){
      System.errorLogging(e);
    }
    finally{
      try{
        mWriter.flush();
        mWriter.close();
        mReader.close();
      }
      catch(IOException e){ }
    }
  }
}
