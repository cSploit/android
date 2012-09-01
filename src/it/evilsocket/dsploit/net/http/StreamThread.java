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
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.util.Log;

public class StreamThread implements Runnable
{
	private final static String  TAG            = "PROXYSTREAMTHREAD";
	private final static Pattern LENGTH_PATTERN = Pattern.compile( "Content-Length:\\s+(\\d+)", Pattern.CASE_INSENSITIVE );
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
    
    public StreamThread( Socket socket, InputStream reader, OutputStream writer ){
    	this( socket, reader, writer, null );
    }
    
    public void run() {
    	
    	int    read     = -1;
    	StringBuilder builder = new StringBuilder();
    	byte[] buffer   = new byte[ BUFFER_SIZE ];
    	
    	try 
    	{
    		// just read BUFFER_SIZE bytes at time until there's nothing more to read.
    		while( true )
    		{
    			try
    			{
    				if( ( read = mReader.read( buffer, 0, BUFFER_SIZE ) ) != -1 )
    				{
    					builder.append( new String( Arrays.copyOfRange( buffer, 0, read ) ) );
    				}
    				else
    					break;
    			}
    			catch( SocketTimeoutException timeout )
    			{
    				
    			}
    		}
    		
			String data 	= builder.toString();
			byte[] response = data.getBytes();
						
			// do we have html ?
			if( data.toLowerCase().contains( "content-type: text/html" ) && mFilter != null )
			{
				String[] split   = data.split( "<", 2 );
				String   headers = split[ 0 ],
						 body	 = ( split.length > 1 ? split[ 1 ] : "" );
				int      length  = body.length(),
						 clength = 0;				
				Matcher	 match   = LENGTH_PATTERN.matcher( headers );
				
				body = mFilter.onHtmlReceived( body );
				
				if( match != null && match.find() )				
					clength = Integer.parseInt( match.group( 1 ) );
				else
					clength = length;
				
				// patch content-length
				if( body.length() != clength )
				{					
					headers = headers.replaceAll( "Content-Length: \\d+", "Content-Length: " + clength );					
				}
				
				response = ( headers + "<" + body ).getBytes();
			}

			mWriter.write( response );    			    			
			mWriter.flush();
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
