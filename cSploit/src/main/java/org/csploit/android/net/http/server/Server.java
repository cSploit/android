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
package org.csploit.android.net.http.server;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;

import org.csploit.android.core.System;
import org.csploit.android.core.Logger;

public class Server implements Runnable
{
  private static final int BACKLOG = 255;
  private static final int MAX_FILE_SIZE = 10 * 1024 * 1024;

  private InetAddress mAddress = null;
  private int mPort = System.HTTP_SERVER_PORT;
  private boolean mRunning = false;
  private ServerSocket mSocket = null;
  private String mResourcePath = null;
  private String mResourceContentType = null;
  private byte[] mResourceData = null;

  public Server(InetAddress address, int port, String resourcePath, String resourceContentType) throws IOException{
    mAddress = address;
    mPort = port;
    mSocket = new ServerSocket(mPort, BACKLOG, mAddress);

    if(resourcePath != null && resourceContentType != null)
      setResource(resourcePath, resourceContentType);
  }

  public Server(String address, int port, String resourcePath, String resourceContentType) throws IOException{
    this(InetAddress.getByName(address), port, resourcePath, resourceContentType);
  }

  public Server(String address, int port) throws IOException{
    this(address, port, null, null);
  }

  public Server(InetAddress address, int port) throws IOException{
    this(address, port, null, null);
  }

  public void setResource(String path, String contentType) throws IOException {
    mResourcePath = path;
    mResourceContentType = contentType;

    // preload resource data
    File file = new File(mResourcePath);
    FileInputStream is = new FileInputStream(file);
    long size = file.length();
    int offset = 0,
      read = 0;

    if(size > MAX_FILE_SIZE)
      throw new IOException("Max allowed file size is " + MAX_FILE_SIZE + " bytes.");

    mResourceData = new byte[(int) size];

    while(offset < size && (read = is.read(mResourceData, offset, (int) (size - offset))) >= 0){
      offset += read;
    }

    if(offset < size)
      throw new IOException("Could not completely read file " + file.getName() + " .");

    is.close();
  }

  public String getResourceURL(){
    return "http://" + mAddress.getHostAddress() + ":" + mPort + "/";
  }

  public void stop(){
    Logger.debug("Stopping server ...");

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

      Logger.debug("Server started on " + mAddress + ":" + mPort);

      mRunning = true;

      while(mRunning){
        try{
          Socket client = mSocket.accept();

          new ServerThread(client, mResourceData, mResourceContentType).start();
        }
        catch(IOException e){
          System.errorLogging(e);
        }
      }

      Logger.debug("Server stopped.");
    }
    catch(IOException e){
      System.errorLogging(e);
    }
  }
}
