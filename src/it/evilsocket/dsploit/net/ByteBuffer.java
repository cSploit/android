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
package it.evilsocket.dsploit.net;

import it.evilsocket.dsploit.core.System;

import java.io.UnsupportedEncodingException;
import java.util.Arrays;

import org.mozilla.universalchardet.UniversalDetector;

public class ByteBuffer 
{
	private byte[] 			  mBuffer 		   = null;
	private UniversalDetector mCharsetDetector = null;
	
	public ByteBuffer(){
		mCharsetDetector = new UniversalDetector( null );
	}
	
	public ByteBuffer( byte[] buffer ){
		mCharsetDetector = new UniversalDetector( null );
		setData( buffer );
	}
	
    public void replace( byte[] search, byte[] replace ) {
    	int idx  = indexOf( search ),
    		slen = search.length,
    		rlen = replace.length;
    	byte[] left, right;
    	
    	ByteBuffer buffer = null;
    	    	
    	while( idx != -1 )
    	{
	    	left   = Arrays.copyOfRange( mBuffer, 0, idx );
	    	right  = Arrays.copyOfRange( mBuffer, idx + slen, mBuffer.length );	    		  				
	    	buffer = new ByteBuffer( left );
	    	
	    	buffer.append( replace, rlen );
	    	buffer.append( right, right.length );
	    	
	    	mBuffer = buffer.mBuffer;
	    	
	    	idx = indexOf( search );
    	}    	
    }

	public void append( byte[] buffer, int length ) {
		byte[] chunk   = Arrays.copyOfRange( buffer, 0, length ),
			   reallcd = null;
		int    i, j;
				
		mCharsetDetector.handleData( buffer, 0, length );
		
		if( mBuffer == null )
			mBuffer = chunk;
		
		else
		{			
			reallcd = new byte[ mBuffer.length + length ];
						
			for( i = 0; i < mBuffer.length; i++ )
				reallcd[i] = mBuffer[i];
			
			for( j = 0; j < length; i++, j++ )
				reallcd[i] = chunk[j];
			
			mBuffer = reallcd;
		}
		
	}
	
	public int indexOf( byte[] pattern, int start ){
    	boolean match = false;
    	int     i, j;
    	
    	if( isEmpty() == false )
    	{
	    	for( i = start; i < mBuffer.length; i++ )
	    	{
	    		match = true;
	    		
	    		for( j = 0; j < pattern.length && match && ( i + j ) < mBuffer.length; j++ )
	    		{
	    			if( mBuffer[ i + j ] != pattern[ j ] )
	    				match = false;
	    		}
	    		
	    		if( match ) return i;
	    	}
    	}
    	
    	return -1;
    }
	
	public int indexOf( byte[] pattern ) {
		return indexOf( pattern, 0 );
	}
			
	public String toString(){
		mCharsetDetector.dataEnd();
				
		try
		{
			if( isEmpty() )
				return "";
			
			else
			{
				String charset = mCharsetDetector.getDetectedCharset();
				
				return new String( mBuffer, charset != null ? charset : "UTF-8" );
			}
		}
		catch( UnsupportedEncodingException e )
		{
			System.errorLogging( "BYTEBUFFER", e );
		}
		
		return new String( mBuffer );
	}
	
	public byte[] getData(){
		return mBuffer;
	}
	
	public void setData( byte[] buffer ){
		mBuffer = buffer;		
		mCharsetDetector.handleData( buffer, 0, buffer.length );
	}
	
	public int getLength(){
		return ( mBuffer == null ? 0 : mBuffer.length );
	}
	
	public boolean isEmpty(){
		return getLength() == 0;
	}
}
