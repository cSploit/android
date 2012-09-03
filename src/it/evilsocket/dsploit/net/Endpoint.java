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

import java.math.BigInteger;
import java.net.InetAddress;
import java.net.UnknownHostException;

import android.util.Log;

public class Endpoint 
{
	private InetAddress mAddress  = null;
	private byte[]      mHardware = null;
	
	private static byte[] parseMacAddress( String macAddress) {
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
			Log.e( "ENDPOINT", e.toString() );
			mAddress = null;
		}
	}
	
	public boolean equals( Endpoint endpoint ){		
		return mAddress.equals( endpoint.getAddress() );
	}
	
	public InetAddress getAddress() {
		return mAddress;
	}
	
	public void setAddress( InetAddress address ) {
		this.mAddress = address;
	}
	
	public byte[] getHardware() {
		return mHardware;
	}
	
	public void setHardware( byte[] hardware ) {
		this.mHardware = hardware;
	}
	
	public String toString(){
		return mAddress.toString().substring(1);
	}
}
