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


import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;
import java.util.ArrayList;

import android.util.Log;

public class ProxyThread extends Thread
{			  
	private final static String TAG				 = "PROXYTHREAD";
	private final static int    MAX_REQUEST_SIZE = 8192;
	private final static String HOST_HEADER 	 = "Host: ";
	private final static int    SERVER_PORT 	 = 80;
	
	private Socket 			 			 mSocket 	   = null;
	private BufferedOutputStream 		 mWriter 	   = null;
	private InputStream   	 	 		 mReader 	   = null;
	private String			 			 mServerName   = null;
	private Socket			 			 mServer 	   = null;
	private InputStream			 	 	 mServerReader = null;
	private OutputStream		 		 mServerWriter = null;
	private ArrayList<Proxy.ProxyFilter> mFilters 	   = null;
	
	public ProxyThread( Socket socket, ArrayList<Proxy.ProxyFilter> filters ) throws IOException {
		super( "ProxyThread" );
		
		mSocket  = socket;
		mWriter  = new BufferedOutputStream( mSocket.getOutputStream() );
		mReader  = mSocket.getInputStream();
		mFilters = filters;		
	}
	
	public void run() {
		
		try 
		{						
            // Apache's default header limit is 8KB.
            byte[] buffer = new byte[ MAX_REQUEST_SIZE ];
            int	   read   = 0;
            
            // Read the header and rebuild it
            if( ( read = mReader.read( buffer , 0,  MAX_REQUEST_SIZE ) ) > 0 )
            {            
	            ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream( buffer, 0, read );
	            BufferedReader 		 bReader 			  = new BufferedReader( new InputStreamReader( byteArrayInputStream ) );	            
	            StringBuilder 		 builder 			  = new StringBuilder();
	            String		  		 line    			  = null;
	            
	            while( ( line = bReader.readLine() ) != null )
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
					// Extract the real request target and connect to it.
					else if( line.contains( HOST_HEADER ) )
					{					
						mServerName   = line.substring( line.indexOf( HOST_HEADER ) + HOST_HEADER.length() ).trim();										
						mServer 	  = new Socket( mServerName, SERVER_PORT );
						mServerReader = mServer.getInputStream(); 
						mServerWriter = mServer.getOutputStream();		
						
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
	 				new StreamThread( mServerReader, mWriter, new Proxy.ProxyFilter() {									
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
            else
            {
            	mReader.close();
            	Log.w( TAG, "Empty HTTP request." );
            }
		} 
		catch( IOException e )
		{
			Log.e( TAG, e.toString() );
		}			
	}
}