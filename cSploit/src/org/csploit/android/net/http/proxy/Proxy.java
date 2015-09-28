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

import android.util.Log;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;

import org.csploit.android.core.Profiler;
import org.csploit.android.core.System;

public class Proxy implements Runnable{
  private static final String TAG = "HTTP.PROXY";
  private static final int BACKLOG = 255;

  private InetAddress mAddress = null;
  private int mPort = System.HTTP_PROXY_PORT;
  private boolean mRunning = false;
  private ServerSocket mSocket = null;
  private OnRequestListener mRequestListener = null;
  private ArrayList<ProxyFilter> mFilters = null;
  private String mHostRedirect = null;
  private int mPortRedirect = 80;

  public interface ProxyFilter{
    String onDataReceived(String headers, String data);
  }

  public interface OnRequestListener{
    void onRequest(boolean https, String address, String hostname, ArrayList<String> headers);
  }

  public Proxy(InetAddress address, int port) throws IOException{
    mAddress = address;
    mPort = port;
    mSocket = new ServerSocket(mPort, BACKLOG, mAddress);
    mFilters = new ArrayList<ProxyFilter>();
  }

  public Proxy(String address, int port) throws IOException{
    this(InetAddress.getByName(address), port);
  }

  public void setOnRequestListener(OnRequestListener listener){
    mRequestListener = listener;
  }

  public void setRedirection(String host, int port){
    mHostRedirect = host;
    mPortRedirect = port;
  }

  public void setFilter(ProxyFilter filter){
    mFilters.clear();

    if(filter != null)
      mFilters.add(filter);
  }

  public void addFilter(ProxyFilter filter){
    mFilters.add(filter);
  }

  public void stop(){
    Log.d(TAG, "Stopping proxy ...");

    try{
      if(mSocket != null)
        mSocket.close();
    } catch(IOException e){

    }

    mRunning = false;
    mSocket = null;
  }

  public void run(){

    try{
      if(mSocket == null)
        mSocket = new ServerSocket(mPort, BACKLOG, mAddress);

      Log.d(TAG, "Proxy started on " + mAddress + ":" + mPort);

      mRunning = true;

      while(mRunning && mSocket != null){
        try{
          Profiler.instance().profile("client spawn");

          Socket client = mSocket.accept();

          new ProxyThread(client, mRequestListener, mFilters, mHostRedirect, mPortRedirect).start();

          Profiler.instance().profile("client spawn");
        } catch(Exception e){

        }
      }

      Log.d(TAG, "Proxy stopped.");
    } catch(IOException e){
      System.errorLogging(e);
    }
  }
}
