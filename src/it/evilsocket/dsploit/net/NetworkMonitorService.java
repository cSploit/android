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

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.MainActivity;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.net.Endpoint;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.core.System;
import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class NetworkMonitorService extends IntentService
{
	public static final String TAG				 = "NETWORKMONITORSERVICE";
	
	public static final String NEW_ENDPOINT		 = "NetworkMonitorService.action.NEW_ENDPOINT";
	public static final String ENDPOINT_UPDATE	 = "NetworkMonitorService.action.ENDPOINT_UPDATE";
	public static final String ENDPOINT_ADDRESS  = "NetworkMonitorService.data.ENDPOINT_ADDRESS";
	public static final String ENDPOINT_HARDWARE = "NetworkMonitorService.data.ENDPOINT_HARDWARE";
	public static final String ENDPOINT_NAME     = "NetworkMonitorService.data.ENDPOINT_NAME";

	private static final String  ARP_TABLE_FILE   = "/proc/net/arp";
	private static final Pattern ARP_TABLE_PARSER = Pattern.compile( "^([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})\\s+([0-9-a-fx]+)\\s+([0-9-a-fx]+)\\s+([a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2})\\s+([^\\s]+)\\s+(.+)$", Pattern.CASE_INSENSITIVE );
	private static final short   NETBIOS_UDP_PORT = 137;
	// NBT UDP PACKET: QUERY; REQUEST; UNICAST
	private static final byte[]  NETBIOS_REQUEST  = 
	{ 
		(byte)0x82, (byte)0x28, (byte)0x0,  (byte)0x0,  (byte)0x0, 
		(byte)0x1,  (byte)0x0,  (byte)0x0,  (byte)0x0,  (byte)0x0, 
		(byte)0x0,  (byte)0x0,  (byte)0x20, (byte)0x43, (byte)0x4B, 
		(byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, 
		(byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, 
		(byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, 
		(byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, 
		(byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, 
		(byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, (byte)0x41, 
		(byte)0x0,  (byte)0x0,  (byte)0x21, (byte)0x0,  (byte)0x1
	};

	private UdpProber mProber		  = null;
	private ArpReader mArpReader 	  = null;
	private int 	  mNotificationId = 0;
	
	private class ArpReader extends Thread
	{				
		private boolean 			   mStopped    = true;
		private HashMap<String,String> mNetBiosMap = null;
		
		public ArpReader() {
			super("ArpReader");
			
			mNetBiosMap = new HashMap<String,String>(); 
		}
		
		public synchronized void addNetBiosName( String address, String name ) {
			synchronized( mNetBiosMap )
			{
				mNetBiosMap.put( address, name );
			}
		}
		
		@Override
		public void run() {
			Log.d( TAG, "ArpReader started ..." );
			
			mNetBiosMap.clear();
			mStopped = false;
			String iface = "";
			
			try
			{	
				iface = System.getNetwork().getInterface().getDisplayName();
			}
			catch( Exception e )
			{
				System.errorLogging( TAG, e );
			}
			
			while( mStopped == false )
			{
				try
				{
					BufferedReader reader   = new BufferedReader( new FileReader( ARP_TABLE_FILE ) );
					String		   line     = null,
								   name		= null;
					Matcher		   matcher  = null;
					Endpoint 	   endpoint = null;
					Target 		   target   = null;
					
					while( ( line = reader.readLine() ) != null )
					{
						if( ( matcher = ARP_TABLE_PARSER.matcher(line) ) != null && matcher.find() )
						{
							String address = matcher.group( 1 ),
								   // hwtype  = matcher.group( 2 ),
								   flags   = matcher.group( 3 ),
								   hwaddr  = matcher.group( 4 ),
								   // mask	   = matcher.group( 5 ),
								   device  = matcher.group( 6 );
							
							if( device.equals(iface) && hwaddr.equals("00:00:00:00:00:00") == false && flags.contains("2") )
							{
								endpoint = new Endpoint( address, hwaddr );
								target   = new Target( endpoint );
								
								synchronized( mNetBiosMap ){ name = mNetBiosMap.get(address); }
								
								if( name == null && target.isRouter() == false )
								{										
									// attempt DNS resolution
									name = endpoint.getAddress().getHostName();
									
									if( name.equals(address) == false )
									{
										Log.d( "NETBIOS", address + " was DNS resolved to " + name );
										
										synchronized( mNetBiosMap ){ mNetBiosMap.put( address, name ); }
									}
									else
										name = null;
								}
								
								if( System.hasTarget( target ) == false )				    				   
									sendNewEndpointNotification( endpoint, name );
			    				
								else if( name != null )
								{
									target = System.getTargetByAddress(address);
									if( target != null && target.hasAlias() == false )
									{
										target.setAlias( name );
										sendEndpointUpdateNotification( );
									}
								}								
							}
						}
					}
					
					reader.close();
					
					Thread.sleep(500);
				}
				catch( Exception e )
				{
					System.errorLogging( TAG, e );
				}
			}
		}		
		
		public synchronized void exit() {
			mStopped = true;
		}
	}
	
	private class NBResolver extends Thread
	{
		InetAddress    mAddress = null;
		DatagramSocket mSocket  = null;
		
		public NBResolver( InetAddress address, DatagramSocket socket ) {
			super( "NBResolver" );
			
			mAddress = address;
			mSocket  = socket;
		}
		
		@Override
		public void run() {			
			byte[] 		   buffer  = new byte[128];
			DatagramPacket packet  = new DatagramPacket( buffer, buffer.length, mAddress, NETBIOS_UDP_PORT ),
						   query   = new DatagramPacket( NETBIOS_REQUEST, NETBIOS_REQUEST.length, mAddress, NETBIOS_UDP_PORT );
			String		   name    = null,
						   address = mAddress.getHostAddress();
			Target 		   target  = null;

			for( int i = 0; i < 3; i++ )
			{				
				try
				{
					mSocket.receive( packet );
					
					byte[] data = packet.getData();
					
					if( data != null && data.length >= 74 )
					{
						String response = new String( data, "ASCII" );

						// i know this is horrible, but i really need only the netbios name
						name = response.substring( 57, 73 ).trim();		
						
						Log.d( "NETBIOS", address + " was resolved to " + name );
						
						// existing target
						target = System.getTargetByAddress( address );
						if( target != null )
						{
							target.setAlias( name );
							sendEndpointUpdateNotification( );
						}
						// not yet discovered/enqueued target
						else						
							mArpReader.addNetBiosName( address, name );
												
						break;
					}						
				}				
				catch( SocketTimeoutException ste ) 
				{ 		
					// swallow timeout error
				}
				catch( IOException e )
				{
					System.errorLogging( "NBResolver", e );
				}
				finally
				{
					try
					{
						// send again a query
						mSocket.send( query );
					}
					catch( Exception e )
					{
						System.errorLogging( "NBResolver", e );
					}
				}
			}
			
			mSocket.close();	
		}
	}
	
	private class UdpProber extends Thread
	{
		private boolean mStopped = true;
		private Network mNetwork = null;

		@Override
		public void run() {
			Log.d( TAG, "UdpProber started ..." );
			
			mStopped = false;
									
			int i, nhosts = 0;
			IP4Address current = null;
			
			try
			{
				mNetwork = System.getNetwork();	
				nhosts   = mNetwork.getNumberOfAddresses();
			}
			catch( Exception e )
			{
				System.errorLogging( TAG, e );
			}

			while( mStopped == false && mNetwork != null && nhosts > 0 )
			{										    			    			    
				try
    			{							
					for( i = 1, current = IP4Address.next( mNetwork.getStartAddress() ); current != null && i <= nhosts; current = IP4Address.next( current ), i++ ) 
					{						
						InetAddress    address = current.toInetAddress();
	    				DatagramSocket socket  = new DatagramSocket();
	    				DatagramPacket packet  = new DatagramPacket( NETBIOS_REQUEST, NETBIOS_REQUEST.length, address, NETBIOS_UDP_PORT );
	    				
	    				socket.setSoTimeout( 200 );
	    				socket.send( packet );    	  

	    				new NBResolver( address, socket ).start();
					}

					Thread.sleep( 1000 );
    			}
    			catch( Exception e )
    			{
    				System.errorLogging( TAG, e );
    			}				
			}
		}
		
		public synchronized void exit() {
			mStopped = true;
		}
	}
	
	public NetworkMonitorService() {
        super("NetworkMonitorService");
    }

    public NetworkMonitorService( String name ) {
        super(name);        
    }
	
	@Override
	public void onCreate() {
		super.onCreate();
		Log.d( TAG, "Starting ..." );	
		
		mArpReader = new ArpReader();
		mProber	   = new UdpProber();
	}
	
	private void sendNotification( String message ) {
		if( System.getSettings().getBoolean( "PREF_NETWORK_NOTIFICATIONS", true ) )
		{
			NotificationManager manager 	 = ( NotificationManager )getSystemService(Context.NOTIFICATION_SERVICE);
			Notification 		notification = new Notification( R.drawable.dsploit_icon_48 , message, java.lang.System.currentTimeMillis() );
			Context 			context 	 = getApplicationContext();
			PendingIntent	    pending		 = PendingIntent.getActivity
			( 
				context, 
				0, 
				new Intent( this, MainActivity.class ).setFlags( Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP ), 
				PendingIntent.FLAG_CANCEL_CURRENT 
			);

			notification.flags |= Notification.FLAG_ONGOING_EVENT;
			
			notification.setLatestEventInfo( context, "Network Monitor", message, pending );
			
			manager.cancel( mNotificationId );
			manager.notify( ++mNotificationId, notification );
		}
	}
	
	private void sendNewEndpointNotification( Endpoint endpoint, String name ) {
		if( name != null )
			sendNotification( "New endpoint found on network : " + name + " ( " + endpoint.toString() + " )" );
		else
			sendNotification( "New endpoint found on network : " + endpoint.toString() );
		
		Intent intent = new Intent( NEW_ENDPOINT );
		
		intent.putExtra( ENDPOINT_ADDRESS,  endpoint.toString() );
		intent.putExtra( ENDPOINT_HARDWARE, endpoint.getHardwareAsString() );
		intent.putExtra( ENDPOINT_NAME,     name == null ? "" : name );
		
        sendBroadcast(intent);    	
	}
	
	private void sendEndpointUpdateNotification( ) {
        sendBroadcast( new Intent( ENDPOINT_UPDATE ) );  
	}

	@Override
	protected void onHandleIntent( Intent intent ) {			
		sendNotification( "Network monitor started ..." );
		   		
		try
		{			
			mProber.start();					
			mArpReader.start();
						
			mProber.join();
			mArpReader.join();
		}
		catch( Exception e )
		{
			System.errorLogging( TAG, e );
		}		
	}

	@Override
	public void onDestroy() {		
		Log.d( TAG, "Stopping ..." );
		
		NotificationManager manager = ( NotificationManager )getSystemService( Context.NOTIFICATION_SERVICE );
		
		for( int i = mNotificationId; i >= 0; i-- )
			manager.cancel( i );
		
		try
		{
			mProber.exit();
			mArpReader.exit();
		}
		catch( Exception e )
		{
			System.errorLogging( TAG, e );
		}
	}

}
	