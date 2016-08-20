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

import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.util.Arrays;

import org.csploit.android.core.System;
import org.csploit.android.core.Logger;

public class ServerThread extends Thread
{
  private final static int MAX_REQUEST_SIZE = 8192;

  private Socket mSocket = null;
  private BufferedOutputStream mWriter = null;
  private InputStream mReader = null;
  private byte[] mData = null;
  private String mContentType = null;

  public ServerThread(Socket socket, byte[] data, String contentType) throws IOException{
    super("ServerThread");

    mSocket = socket;
    mWriter = new BufferedOutputStream(mSocket.getOutputStream());
    mReader = mSocket.getInputStream();
    mData = Arrays.copyOf(data, data.length);
    mContentType = contentType;
  }

  public void run(){

    try{
      // Apache's default header limit is 8KB.
      byte[] request = new byte[MAX_REQUEST_SIZE];

      // Read the request
      if(mReader.read(request, 0, MAX_REQUEST_SIZE) > 0){
        String header = "HTTP/1.1 200 OK\r\n" +
          "Content-Type: " + mContentType + "\r\n" +
          "Content-Length: " + mData.length + "\r\n\r\n";

        mWriter.write(header.getBytes());
        mWriter.write(mData);
      }
      else
        Logger.warning("Empty HTTP request.");
    }
    catch(IOException e){
      System.errorLogging(e);
    }
    finally{
      try{
        mWriter.flush();
        mWriter.close();
        mReader.close();
      }
      catch(IOException e){
        System.errorLogging(e);
      }
    }
  }
}
