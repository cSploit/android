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

import java.util.Arrays;

public class ByteBuffer 
{
	private byte[] mBuffer = null;
	
	public ByteBuffer(){
		
	}

	public void append( byte[] buffer, int length ) {
		byte[] chunk   = Arrays.copyOfRange( buffer, 0, length ),
			   reallcd = null;
		int    i, j;
		
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
	
	public int indexOf( byte[] pattern ){
    	boolean match = false;
    	int     i, j;
    	
    	if( isEmpty() == false )
    	{
	    	for( i = 0; i < mBuffer.length; i++ )
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
			
	public String toString(){
		if( isEmpty() )
			return "";
		
		else
			return new String( mBuffer );
	}
	
	public byte[] getData(){
		return mBuffer;
	}
	
	public void setData( byte[] buffer ){
		mBuffer = buffer;
	}
	
	public int getLength(){
		return ( mBuffer == null ? 0 : mBuffer.length );
	}
	
	public boolean isEmpty(){
		return getLength() == 0;
	}
}
