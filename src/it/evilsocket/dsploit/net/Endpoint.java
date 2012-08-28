package it.evilsocket.dsploit.net;

import java.math.BigInteger;
import java.net.InetAddress;
import java.net.UnknownHostException;

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
			mAddress = null;
		}
	}
	
	public boolean equals( Endpoint endpoint ){
		return mAddress.equals( endpoint.getAddress() ) && ( mHardware == null ? ( mHardware == endpoint.getHardware() ) : mHardware.equals( endpoint.getHardware() ) );
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
}
