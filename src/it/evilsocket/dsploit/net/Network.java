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

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.NoRouteToHostException;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Enumeration;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.DhcpInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;

public class Network 
{
	private static final String TAG = "NETWORK";
	
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
	private WifiInfo			mWifiInfo			 = null;
	private NetworkInterface    mInterface			 = null;
	
	public Network( Context context ) throws NoRouteToHostException, SocketException {
		mWifiManager		 = ( WifiManager )context.getSystemService( Context.WIFI_SERVICE );
		mConnectivityManager = ( ConnectivityManager )context.getSystemService( Context.CONNECTIVITY_SERVICE );
		mInfo		 		 = mWifiManager.getDhcpInfo();
		mWifiInfo			 = mWifiManager.getConnectionInfo();
		
		if( isConnected() == false)
			throw new NoRouteToHostException("Not connected to any WiFi access point.");
		
		else
		{
			try
			{
				mInterface = NetworkInterface.getByInetAddress( getLocalAddress() );
			}
			catch( SocketException e )
			{
				System.errorLogging( TAG, e );
				
				/*
				 * Issue #26: Initialization error in ColdFusionX ROM
				 * 
				 * It seems it's a ROM issue which doesn't correctly populate device descriptors.
				 * This rom maps the default wifi interface to a generic usb device 
				 * ( maybe it's missing the specific interface driver ), which is obviously not, and
				 * it all goes shit, use an alternative method to obtain the interface object.
				 */
				Enumeration<NetworkInterface> interfaces   = NetworkInterface.getNetworkInterfaces();
				InetAddress					  ifaceAddress = getLocalAddress();
				
				while( interfaces != null && mInterface == null && interfaces.hasMoreElements() ) 
				{
					NetworkInterface iface = interfaces.nextElement();
					//since each interface could have many InetAddresses…
					Enumeration<InetAddress> inetAddresses = iface.getInetAddresses();
					
					while( inetAddresses.hasMoreElements() ) 
					{
						InetAddress address = inetAddresses.nextElement();

						if( address.equals(ifaceAddress) )
						{
							mInterface = iface;
							break;
						}
					}
				}
			}
		}
	}
			
	private static int countBits( int n ){
    	int bits, i;
    	
    	for( i = 0, bits = ( n & 1 ); i < 32; i++, n >>= 1, bits += n & 1 );    		
    	
    	return bits;
    }
	
	public boolean equals( Network network ){
		return mInfo.equals( network.getInfo() );
	}
	
	public boolean isInternal( int ip ) {
		return isInternal( ( ip & 0xFF) + "." + (( ip >> 8 ) & 0xFF) + "." + (( ip >> 16 ) & 0xFF) + "." + (( ip >> 24 ) & 0xFF) );
	}
	
	public boolean isInternal( String ip ) {
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
			System.errorLogging( TAG, e );
		}
		
		return false;
	}
	
	public static boolean isWifiConnected( Context context ){
		return ( ( ConnectivityManager )context.getSystemService( Context.CONNECTIVITY_SERVICE ) ).getNetworkInfo( ConnectivityManager.TYPE_WIFI ).isConnected();
	}
	
	public static boolean isConnectivityAvailable( Context context ) {
	    return ( ( ConnectivityManager )context.getSystemService( Context.CONNECTIVITY_SERVICE ) ).getActiveNetworkInfo() != null;
	}
	
	public boolean isConnected(){
		return mConnectivityManager.getNetworkInfo( ConnectivityManager.TYPE_WIFI ).isConnected();
	}
	
	public String getSSID( ) {
		return mWifiInfo.getSSID();
	}
	
	public String getNetworkMasked( ) {
		int network = mInfo.gateway & mInfo.netmask;

    	return ( network & 0xFF) + "." + (( network >> 8 ) & 0xFF) + "." + (( network >> 16 ) & 0xFF) + "." + (( network >> 24 ) & 0xFF);
	}
	
	public int getNetworkMaskedAsInt( ) {
		return mInfo.gateway & mInfo.netmask;
	}
	
	public String getNetworkRepresentation( )
	{				
    	return getNetworkMasked() + "/" + countBits( mInfo.netmask );
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
			System.errorLogging( TAG, e );
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
			System.errorLogging( TAG, e );
		}
		
		return null;
	}
	
	public byte[] getGatewayHardware(){
		return Endpoint.parseMacAddress( mWifiInfo.getBSSID() );
	}
	
	public byte[] getLocalHardware( ){
		try 
		{
			return mInterface.getHardwareAddress();
		} 
		catch( SocketException e ) 
		{
			System.errorLogging( TAG, e );
		}
		
		return null;
	}
	
	public String getLocalAddressAsString( ){
		return ( mInfo.ipAddress & 0xFF) + "." + (( mInfo.ipAddress >> 8 ) & 0xFF) + "." + (( mInfo.ipAddress >> 16 ) & 0xFF) + "." + (( mInfo.ipAddress >> 24 ) & 0xFF);
	}
	
	public InetAddress getLocalAddress( ){
		try
		{
			return InetAddress.getByName( getLocalAddressAsString() );
		}
		catch( UnknownHostException e )
		{
			System.errorLogging( TAG, e );
		}
		
		return null;
	}
	
	public NetworkInterface getInterface(){
		return mInterface;
	}
}
