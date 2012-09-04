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

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.util.Arrays;

import android.util.Log;

public class StreamThread implements Runnable
{
	private final static String  TAG            = "PROXYSTREAMTHREAD";
	private final static String  HEAD_SEPARATOR = "\r\n\r\n";
    private final static int     BUFFER_SIZE    = 1024;
    private final static int     TIMEOUT        = 20;
    
    private Socket			  mSocket = null;
    private InputStream       mReader = null;
    private OutputStream      mWriter = null;
    private Proxy.ProxyFilter mFilter = null;
    
    public StreamThread( Socket socket, InputStream reader, OutputStream writer, Proxy.ProxyFilter filter ){
    	mSocket = socket;
    	mReader = reader;
    	mWriter = writer;
    	mFilter = filter;
    	
    	try
    	{
    		mSocket.setSoTimeout( TIMEOUT );
    	}
    	catch( SocketException e )
    	{
    		Log.e( TAG, e.toString() );
    	}
    	
    	new Thread( this ).start();
    }
    
    // return first + second[ 0..length ]
    private static byte[] streamAppend( byte[] first, byte[] second, int length ){
		byte[] stream = null,
			   chunk  = Arrays.copyOfRange( second, 0, length );
		int    i, j;
		
		if( first == null )
			stream = chunk;
		
		else
		{
			stream = new byte[ first.length + length ];
						
			for( i = 0; i < first.length; i++ )
				stream[i] = first[i];
			
			for( j = 0; j < length; i++, j++ )
				stream[i] = chunk[j];
		}
		
		return stream;
	}
    
    // return the index of the first occurrence of pattern inside stream
    private static int streamIndexOf( byte[] stream, byte[] pattern ){
    	boolean match = false;
    	int     i, j;
    	
    	for( i = 0; i < stream.length; i++ )
    	{
    		match = true;
    		
    		for( j = 0; j < pattern.length && match && ( i + j ) < stream.length; j++ )
    		{
    			if( stream[ i + j ] != pattern[ j ] )
    				match = false;
    		}
    		
    		if( match ) return i;
    	}
    	
    	return -1;
    }
    
    public void run() {
    	
    	int    read   = -1;
    	byte[] stream = null,
    		   buffer = new byte[ BUFFER_SIZE ];
    	
    	try 
    	{
    		// just read BUFFER_SIZE bytes at time until there's nothing more to read.
    		while( true )
    		{
    			try
    			{
    				// TODO: Implement support for chunkend transfer encoding
    				if( ( read = mReader.read( buffer, 0, BUFFER_SIZE ) ) != -1 )
    				{
    					// since we don't know yet if we have a binary or text stream,
    					// use only a byte array buffer instead of a string builder to
    					// avoid encoding corruption
    					stream = streamAppend( stream, buffer, read );
    				}
    				else
    					break;
    			}
    			catch( SocketTimeoutException timeout )
    			{
    				
    			}
    		}
    		
    		if( stream != null && stream.length > 0 )
    		{
				// do we have html ?
				if( streamIndexOf( stream, "text/html".getBytes() ) != -1 )
				{
					// split headers and body, then apply the filter				
					String   data    = new String( stream );
					String[] split   = data.split( HEAD_SEPARATOR, 2 );
					String   headers = split[ 0 ],
							 body	 = ( split.length > 1 ? split[ 1 ] : "" ),
							 patched = "";
					
					body = mFilter.onHtmlReceived( body );
	
					// remove explicit content length, just in case the body changed after filtering				
					for( String header : headers.split("\n") )
					{
						if( header.toLowerCase().contains("content-length") == false )
							patched += header + "\n";
					}
					
					headers = patched;				
					stream  = ( headers + HEAD_SEPARATOR + body ).getBytes();				
				}
				
				mWriter.write( stream );    			    			
				mWriter.flush();
    		}
		} 
    	catch( IOException e ) 
    	{			
    		Log.e( TAG, e.toString() );
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
    			Log.e( TAG, e.toString() );
			}
    	}
    }
}
