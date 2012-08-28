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

import java.net.InetAddress;
import java.net.NoRouteToHostException;
import java.net.UnknownHostException;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.DhcpInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

public class Network 
{
	public enum Protocol {
		TCP,
		UDP,
		ICMP,
		IGMP,
		UNKNOWN;
		
		public static Protocol fromString( String proto ) {
			
			if( proto != null )
			{
				proto = proto.toLowerCase();
			
				if( proto.equals("tcp") )
					return TCP;
				
				else if( proto.equals("udp") )
					return UDP;
				
				else if( proto.equals("icmp") )
					return ICMP;
				
				else if( proto.equals("igmp") )
					return IGMP;			
			}
				
			return UNKNOWN;
		}
	}
	
	private ConnectivityManager mConnectivityManager = null;
	private WifiManager 		mWifiManager 		 = null;	
	private DhcpInfo   			mInfo        		 = null;

	public Network( Context context ) throws NoRouteToHostException {
		mWifiManager		 = ( WifiManager )context.getSystemService( Context.WIFI_SERVICE );
		mConnectivityManager = ( ConnectivityManager )context.getSystemService( Context.CONNECTIVITY_SERVICE );
		mInfo		 		 = mWifiManager.getDhcpInfo();
		
		if( isConnected() == false)
			throw new NoRouteToHostException("Not connected to any WiFi access point.");
	}
			
	private int countBits( int n ){
    	int bits = 0;
    	
    	while( n > 0 )
    	{
    		bits  += n & 1;
    		n    >>= 1; 
    	}
    	
    	return bits;
    }
	
	public boolean equals( Network network ){
		return mInfo.equals( network.getInfo() );
	}
	
	public boolean isInternal( String ip ){
		try
		{
			byte[] gateway = getGatewayAddress().getAddress();
			byte[] address = InetAddress.getByName( ip ).getAddress();
			byte[] mask    = getNetmaskAddress().getAddress();
			
			for( int i = 0; i < gateway.length; i++ )
		        if( ( gateway[i] & mask[i] ) != ( address[i] & mask[i] ) )
		            return false;

		    return true;
		}
		catch( UnknownHostException e )
		{
			Log.e( "Network", e.toString() );
		}
		
		return false;
	}
	
	public boolean isConnected(){
		return mConnectivityManager.getNetworkInfo( ConnectivityManager.TYPE_WIFI ).isConnected();
	}
	
	public String getNetworkRepresentation( )
	{				
		int bits = countBits( mInfo.netmask );
    	
    	return ( mInfo.gateway & 0xFF) + "." + (( mInfo.gateway >> 8 ) & 0xFF) + "." + (( mInfo.gateway >> 16 ) & 0xFF) + ".0/" + bits;
	}
	
	public DhcpInfo getInfo(){
		return mInfo;
	}
	
	public InetAddress getNetmaskAddress( ){
		try
		{
			return InetAddress.getByName( ( mInfo.netmask & 0xFF) + "." + (( mInfo.netmask >> 8 ) & 0xFF) + "." + (( mInfo.netmask >> 16 ) & 0xFF) + "." + (( mInfo.netmask >> 24 ) & 0xFF) );
		}
		catch( UnknownHostException e )
		{
			Log.e( "Network", e.toString() );
		}
		
		return null;
	}
	
	public InetAddress getGatewayAddress( ){
		try
		{
			return InetAddress.getByName( ( mInfo.gateway & 0xFF) + "." + (( mInfo.gateway >> 8 ) & 0xFF) + "." + (( mInfo.gateway >> 16 ) & 0xFF) + "." + (( mInfo.gateway >> 24 ) & 0xFF) );
		}
		catch( UnknownHostException e )
		{
			Log.e( "Network", e.toString() );
		}
		
		return null;
	}
	
	public InetAddress getLoacalAddress( ){
		try
		{
			return InetAddress.getByName( ( mInfo.ipAddress & 0xFF) + "." + (( mInfo.ipAddress >> 8 ) & 0xFF) + "." + (( mInfo.ipAddress >> 16 ) & 0xFF) + "." + (( mInfo.ipAddress >> 24 ) & 0xFF) );
		}
		catch( UnknownHostException e )
		{
			Log.e( "Network", e.toString() );
		}
		
		return null;
	}
}
