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
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

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
	public static final String ENDPOINT_ADDRESS  = "NetworkMonitorService.data.ENDPOINT_ADDRESS";
	public static final String ENDPOINT_HARDWARE = "NetworkMonitorService.data.ENDPOINT_HARDWARE";
	
	private static final String  ARP_TABLE_FILE   = "/proc/net/arp";
	private static final Pattern ARP_TABLE_PARSER = Pattern.compile( "^([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})\\s+([0-9-a-fx]+)\\s+([0-9-a-fx]+)\\s+([a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2})\\s+([^\\s]+)\\s+(.+)$", Pattern.CASE_INSENSITIVE );
	private static final short   NETBIOS_UDP_PORT = 137;
	private static final byte[]  NETBIOS_REQUEST  = 
	{ 
		-126, 40, 1, 32, 67, 75,
		65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65,
		65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 
		33, 1 
	};

	private UdpProber mProber		  = null;
	private ArpReader mArpReader 	  = null;
	private int 	  mNotificationId = 0;
	
	private class ArpReader extends Thread
	{				
		private boolean mStopped = true;
		
		@Override
		public void run() {
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
					BufferedReader reader  = new BufferedReader( new FileReader( ARP_TABLE_FILE ) );
					String		   line    = null;
					Matcher		   matcher = null;
					
					while( ( line = reader.readLine() ) != null )
					{
						if( ( matcher = ARP_TABLE_PARSER.matcher(line) ) != null && matcher.find() )
						{
							String address = matcher.group( 1 ),
								   // hwtype  = matcher.group( 2 ),
								   // flags   = matcher.group( 3 ),
								   hwaddr  = matcher.group( 4 ),
								   // mask	   = matcher.group( 5 ),
								   device  = matcher.group( 6 );
							
							if( device.equals(iface) && hwaddr.equals("00:00:00:00:00:00") == false )
							{
								Endpoint endpoint = new Endpoint( address, hwaddr );
								Target   target   = new Target( endpoint );
								
								if( System.hasTarget( target ) == false )
			    				{    					
			    					sendNewEndpointNotification( endpoint );	            
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
	
	private class UdpProber extends Thread
	{
		private boolean mStopped = true;
		private Network mNetwork = null;

		@Override
		public void run() {
			mStopped = false;
			
			IP4Address mask	   = null,
					   base    = null,
					   current = null;
			
			int i, nhosts = 0;
			
			try
			{
				mNetwork = System.getNetwork();				
				mask     = new IP4Address( mNetwork.getInfo().netmask );
				base     = new IP4Address( mNetwork.getInfo().netmask & mNetwork.getInfo().gateway );				
				nhosts   = IP4Address.ntohl( ~mask.toInteger() );
			}
			catch( Exception e )
			{
				System.errorLogging( TAG, e );
			}

			while( mStopped == false && mNetwork != null && nhosts > 0 )
			{										    			    			    
				try
    			{
					current = base;
					
					for( i = 1; i <= nhosts && ( current = IP4Address.next( current ) ) != null; i++ ) 
					{
						InetAddress    address = current.toInetAddress();
	    				DatagramSocket socket  = new DatagramSocket();
	    				DatagramPacket packet  = new DatagramPacket( NETBIOS_REQUEST, NETBIOS_REQUEST.length, address, NETBIOS_UDP_PORT );
	    				
	    				socket.setSoTimeout( 200 );
	    				socket.send( packet );    	  
	    				socket.close();
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
			PendingIntent	    pending		 = PendingIntent.getActivity( context, 0, new Intent(), PendingIntent.FLAG_UPDATE_CURRENT );
	
			notification.flags |= Notification.FLAG_AUTO_CANCEL;
			
			notification.setLatestEventInfo( context, "Network Monitor", message, pending );
			
			manager.cancel( mNotificationId );
			manager.notify( ++mNotificationId, notification );
		}
	}
	
	private void sendNewEndpointNotification( Endpoint endpoint ) {
		sendNotification( "New endpoint found on network : " + endpoint.toString() );
		
		Intent intent = new Intent( NEW_ENDPOINT );
		
		intent.putExtra( ENDPOINT_ADDRESS,  endpoint.toString() );
		intent.putExtra( ENDPOINT_HARDWARE, endpoint.getHardwareAsString() );
        sendBroadcast(intent);    	
	}

	@Override
	protected void onHandleIntent( Intent intent ) {		
		sendNotification( "Network monitor started ..." );
		   
		if( mProber.isAlive() == false )
			mProber.start();		
		
		if( mArpReader.isAlive() == false )
			mArpReader.start();
		
		try
		{
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
			mProber.join();
			mArpReader.exit();
			mArpReader.join();
		}
		catch( InterruptedException ie )
		{
			// ignore interrupted exception
		}	
		catch( Exception e )
		{
			System.errorLogging( TAG, e );
		}
	}

}
	