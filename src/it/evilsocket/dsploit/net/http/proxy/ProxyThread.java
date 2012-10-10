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
package it.evilsocket.dsploit.net.http.proxy;


import it.evilsocket.dsploit.core.System;

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
	private final static int    SERVER_PORT 	 = 80;
	
	private final static String HOST_HEADER 	 		 = "Host";
	private final static String ACCEPT_ENCODING_HEADER 	 = "Accept-Encoding";
	private final static String CONNECTION_HEADER     	 = "Connection";
	private final static String IF_MODIFIED_SINCE_HEADER = "If-Modified-Since";
	private final static String CACHE_CONTROL_HEADER	 = "Cache-Control";

	private Socket 			 			 mSocket 	   = null;
	private BufferedOutputStream 		 mWriter 	   = null;
	private InputStream   	 	 		 mReader 	   = null;
	private String			 			 mServerName   = null;
	private Socket			 			 mServer 	   = null;
	private InputStream			 	 	 mServerReader = null;
	private OutputStream		 		 mServerWriter = null;
	private ArrayList<Proxy.ProxyFilter> mFilters 	   = null;
	private String				  		 mHostRedirect = null;
	private int					   		 mPortRedirect = 80;
	
	public ProxyThread( Socket socket, ArrayList<Proxy.ProxyFilter> filters, String hostRedirection, int portRedirection ) throws IOException {
		super( "ProxyThread" );
		
		mSocket  	  = socket;
		mWriter  	  = new BufferedOutputStream( mSocket.getOutputStream() );
		mReader  	  = mSocket.getInputStream();
		mFilters 	  = filters;		
		mHostRedirect = hostRedirection;
		mPortRedirect = portRedirection;
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
	            boolean				 headersProcessed     = false;
	            
	            while( ( line = bReader.readLine() ) != null )
				{	    
	            	if( headersProcessed == false )
	            	{
	            		// \r\n\r\n received ?
	            		if( line.trim().isEmpty() )
	            			headersProcessed = true;
						// Set protocol version to 1.0 since we don't support chunked transfer encoding ( yet )
	            		else if( line.contains( "HTTP/1.1" ) )
							line = line.replace( "HTTP/1.1", "HTTP/1.0" );
		            	// Fix headers
		            	else if( line.indexOf(':') != -1 )
		            	{
		            		String[] split  = line.split( ":", 2 );
		            		String   header = split[0].trim(),
		            				 value  = split[1].trim();
		            		
							// Set encoding to identity since we are not handling gzipped streams
		            		if( header.equals( ACCEPT_ENCODING_HEADER ) )
		            			value = "identity";	            		
							// Can't easily handle keep alive connections with blocking sockets
		            		else if( header.equals( CONNECTION_HEADER ) )
		            			value = "close";
							// Keep requesting fresh files and ignore any cache instance
		            		else if( header.equals( IF_MODIFIED_SINCE_HEADER ) || header.equals( CACHE_CONTROL_HEADER ) )
		            			header = null;
							// Extract the real request target and connect to it.
		            		else if( header.equals( HOST_HEADER ) )
		            		{
		            			if( mHostRedirect == null )
		            			{
		            				mServerName = value;										
		            				mServer     = new Socket( mServerName, SERVER_PORT );
		            			}
		            			else
		            			{
		            				mServerName = mHostRedirect;
		            				mServer     = new Socket( mServerName, mPortRedirect );
		            			}
		            			
								mServerReader = mServer.getInputStream(); 
								mServerWriter = mServer.getOutputStream();		
								
								Log.d( TAG, mSocket.getLocalAddress().getHostAddress() + " > " + mServerName );
		            		}
		            			
		            		if( header != null )
		            			line = header + ": " + value;
		            	}
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
	 					public String onDataReceived( String headers, String data ) 
	 					{	
	 						// apply each provided filter
	 						for( Proxy.ProxyFilter filter : mFilters )
	 						{
	 							data = filter.onDataReceived( headers, data );
	 						}
	 						
	 						return data;
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
			System.errorLogging( TAG, e );
		}			
	}
}