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

import java.io.BufferedReader;
import java.math.BigInteger;
import java.net.InetAddress;
import java.net.UnknownHostException;

public class Endpoint 
{	
	private static final String TAG = "ENDPOINT";
	
	private InetAddress mAddress  = null;
	private byte[]      mHardware = null;
		
	public static byte[] parseMacAddress( String macAddress ) {
		if( macAddress != null && macAddress.equals("null") == false && macAddress.isEmpty() == false )
		{

	        String[] bytes = macAddress.split(":");
	        byte[] parsed  = new byte[bytes.length];
	
	        for (int x = 0; x < bytes.length; x++)
	        {
	            BigInteger temp = new BigInteger(bytes[x], 16);
	            byte[] raw 		= temp.toByteArray();
	            parsed[x] 		= raw[raw.length - 1];
	        }
						        
	        return parsed;
		}
		
		return null;
		
    }
	
	public Endpoint( String address ){
		this( address, null );
	}
	
	public Endpoint( InetAddress address, byte[] hardware ) {
		mAddress  = address;
		mHardware = hardware;
	}
	
	public Endpoint( String address, String hardware ) {
		try
		{
			mAddress  = InetAddress.getByName( address );
			mHardware = hardware != null ? parseMacAddress( hardware ) : null;			
		}
		catch( UnknownHostException e )
		{
			System.errorLogging( TAG, e );
			mAddress = null;
		}
	}
	
	public Endpoint( BufferedReader reader ) throws Exception {
		mAddress  = InetAddress.getByName( reader.readLine() );
		mHardware = parseMacAddress( reader.readLine() ); 
	}
	
	public void serialize( StringBuilder builder ) {
		builder.append( mAddress.getHostAddress() + "\n" );
		builder.append( getHardwareAsString() + "\n" );
	}
	
	public boolean equals( Endpoint endpoint ){		
		return mAddress.equals( endpoint.getAddress() );
	}
	
	public InetAddress getAddress() {
		return mAddress;
	}
	
	public long getAddressAsLong() {
		byte[] baddr = mAddress.getAddress();
		
		return ( ( baddr[0] & 0xFFl) << 24 ) + ( ( baddr[1] & 0xFFl ) << 16 ) + ( ( baddr[2] & 0xFFl ) << 8 ) + ( baddr[3] &  0xFFl ); 
	}
	
	public void setAddress( InetAddress address ) {
		this.mAddress = address;
	}
	
	public byte[] getHardware() {
		return mHardware;
	}
	
	public String getHardwareAsString() {
		if( mHardware == null )
			return "";
		
		StringBuilder builder = new StringBuilder(18);
	    for( byte b : mHardware ) 
	    {
	        if( builder.length() > 0 )
	            builder.append(':');
	        
	        builder.append( String.format("%02X", b) );
	    }
	    
	    return builder.toString();
	}
	
	public void setHardware( byte[] hardware ) {
		this.mHardware = hardware;
	}
	
	public String toString(){
		return mAddress.getHostAddress();
	}
}
