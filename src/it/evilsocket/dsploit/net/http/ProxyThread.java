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
package it.evilsocket.dsploit.net.http;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.Arrays;

import android.util.Log;

public class ProxyThread extends Thread
{			  
	private final static String TAG			= "PROXYTHREAD";
	private final static int    BUFFER_SIZE = 2048;
	private final static int    TIMEOUT     = 200;
	private final static String HOST_TOKEN  = "Host: ";
	private final static int    SERVER_PORT = 80;
	
	private Socket 			 			 mSocket 	   = null;
	private BufferedOutputStream 		 mWriter 	   = null;
	private BufferedInputStream   	 	 mReader 	   = null;
	private String			 			 mServerName   = null;
	private Socket			 			 mServer 	   = null;
	private BufferedInputStream	 	 	 mServerReader = null;
	private BufferedOutputStream 		 mServerWriter = null;
	private ArrayList<Proxy.ProxyFilter> mFilters 	   = null;
	
	public ProxyThread( Socket socket, ArrayList<Proxy.ProxyFilter> filters ) throws IOException {
		super( "ProxyThread" );
		
		mSocket  = socket;
		mWriter  = new BufferedOutputStream( mSocket.getOutputStream() );
		mReader  = new BufferedInputStream( mSocket.getInputStream() );
		mFilters = filters;
		
		mSocket.setSoTimeout( TIMEOUT );					
	}
	
	private String readRequest( InputStream reader ) throws IOException {
		StringBuilder buffer = new StringBuilder();

		try
		{
			int    read = -1;
			byte[] toread = new byte[ BUFFER_SIZE ];
			
			while( ( read = reader.read( toread, 0, BUFFER_SIZE ) ) != -1 )
			{
				buffer.append( new String( Arrays.copyOfRange( toread, 0, read ) ) );
			}
		}
		catch( SocketTimeoutException timeout )
		{
			
		}
		
		return buffer.toString();
	}
	
	public void run() {
		
		try 
		{						
			String 		  request = readRequest( mReader );
			StringBuilder builder = new StringBuilder();
			
			for( String line : request.split("\n" ) )
			{
				// Set protocol version to 1.0 since we don't support chunked transfer encoding ( yet )
				if( line.contains("HTTP/1.1") )
					line = line.replace( "HTTP/1.1", "HTTP/1.0" );
				// Set encoding to identity since we are not handling gzipped streams
				else if( line.contains("Accept-Encoding") )
					line = "Accept-Encoding: identity";				
				// Can't easily handle keep alive connections with blocking sockets
				else if( line.contains("keep-alive") )
				 	line = "Connection: close";
				// Exctract the real request target and connect to it.
				else if( line.contains( HOST_TOKEN ) )
				{					
					mServerName   = line.substring( line.indexOf( HOST_TOKEN ) + HOST_TOKEN.length() ).trim();										
					mServer 	  = new Socket( mServerName, SERVER_PORT );
					mServerReader = new BufferedInputStream( mServer.getInputStream() ); 
					mServerWriter = new BufferedOutputStream( mServer.getOutputStream() );		
					
					Log.d( TAG, mSocket.getLocalAddress() + " > " + mServerName );
				}
				
				// build the patched request
				builder.append( line + "\n" );
			}
							
			// any real host found ?
			if( mServer != null )
			{				
				// send the patched request
				mServerWriter.write( builder.toString().getBytes() );
				mServerWriter.flush();
				// start the stream session with specified filters				
				new StreamThread( mServer, mServerReader, mWriter, new Proxy.ProxyFilter() {									
					@Override
					public String onHtmlReceived( String html ) 
					{	
						// apply each provided filter
						for( Proxy.ProxyFilter filter : mFilters )
						{
							html = filter.onHtmlReceived( html );
						}
						
						return html;
					}
				});
			}							
		} 
		catch( IOException e )
		{
			Log.e( TAG, e.toString() );
		}			
	}
}