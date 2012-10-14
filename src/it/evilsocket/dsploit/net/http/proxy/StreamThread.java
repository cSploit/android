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

import it.evilsocket.dsploit.net.ByteBuffer;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import it.evilsocket.dsploit.core.System;

import android.util.Log;

public class StreamThread implements Runnable
{
	private final static String   TAG              		 = "PROXYSTREAMTHREAD";
	private final static byte[][] FILTERED_CONTENT_TYPES = new byte[][]
    {
		"text/html".getBytes(),
		"text/css".getBytes()
    };
	
	private final static String  HEAD_SEPARATOR    = "\r\n\r\n";
    private final static int     CHUNK_SIZE        = 1024;
    
    private InputStream       mReader = null;
    private OutputStream      mWriter = null;
    private ByteBuffer		  mBuffer = null;
    private Proxy.ProxyFilter mFilter = null;
    
    public StreamThread( InputStream reader, OutputStream writer, Proxy.ProxyFilter filter ){
    	mReader = reader;
    	mWriter = writer;
    	mBuffer = new ByteBuffer();
    	mFilter = filter;
    	
    	new Thread( this ).start();
    }
    
    public void run() {
    	
    	int    read  = -1,
    		   size  = 0,
    		   max   = 0;
    	byte[] chunk = new byte[ CHUNK_SIZE ];
    	
    	try
    	{
    		max = Integer.parseInt( System.getSettings().getString( "PREF_HTTP_MAX_BUFFER_SIZE", "10485760" ) );
    	}
    	catch( NumberFormatException e )
    	{
    		max = 10485760;
    	}
    	
    	try 
    	{
    		while( ( read = mReader.read( chunk, 0, CHUNK_SIZE ) ) > 0 && size < max )
    		{
    			mBuffer.append( chunk, read );
    			
    			size += read;
    		}
    		
    		if( mBuffer.isEmpty() == false )
    		{    			
				// do we have an handled content type ?
    			boolean isHandledContentType = false;
    			for( byte[] handled : FILTERED_CONTENT_TYPES )
    			{
    				if( mBuffer.indexOf( handled ) != -1 )
    				{
    					isHandledContentType = true;
    					break;
    				}
    			}
    			
				if( isHandledContentType )
				{
					// split headers and body, then apply the filter				
					String   data    = mBuffer.toString();
					String[] split   = data.split( HEAD_SEPARATOR, 2 );
					String   headers = split[ 0 ],
							 body	 = ( split.length > 1 ? split[ 1 ] : "" ),
							 patched = "";
					
					body = mFilter.onDataReceived( headers, body );
	
					// remove explicit content length, just in case the body changed after filtering				
					for( String header : headers.split("\n") )
					{
						if( header.toLowerCase().contains("content-length") == false )
							patched += header + "\n";
					}
					
					headers = patched;				
					mBuffer.setData( ( headers + HEAD_SEPARATOR + body ).getBytes() );				
				}
				
				mWriter.write( mBuffer.getData() );    			    			
				mWriter.flush();
    		}
		} 
    	catch( OutOfMemoryError ome )
    	{
    		Log.e( TAG, ome.toString() );
    	}
    	catch( Exception e ) 
    	{			
    		System.errorLogging( TAG, e );
		}
    	finally
    	{
    		try 
    		{
    			mWriter.flush();
    			mWriter.close();
				mReader.close();						
			} 
    		catch( IOException e ) 
    		{			
    			System.errorLogging( TAG, e );
			}
    	}
    }
}
