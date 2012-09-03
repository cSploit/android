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
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;

import android.util.Log;

public class Proxy implements Runnable
{
	private static final String TAG     = "PROXY";
	private static final int    BACKLOG = 10;
	
	private InetAddress  		   mAddress = null;
	private int     	 		   mPort    = 8080;
	private boolean 	 		   mRunning = false;
	private ServerSocket 		   mSocket  = null;
	private ArrayList<ProxyFilter> mFilters = null;
	
	public static interface ProxyFilter
	{
		public String onHtmlReceived( String html );
	}
	
	public Proxy( InetAddress address, int port ) throws UnknownHostException, IOException {
		mAddress = address;
		mPort	 = port;
		mSocket  = new ServerSocket( mPort, BACKLOG, mAddress );		
		mFilters = new ArrayList<ProxyFilter>();
	}
	
	public Proxy( String address, int port ) throws UnknownHostException, IOException {
		this( InetAddress.getByName( address ), port );
	}
	
	public void setFilter( ProxyFilter filter ){
		mFilters.clear();
		mFilters.add( filter );
	}
	
	public void addFilter( ProxyFilter filter ){
		mFilters.add( filter );
	}
		
	public void stop() {
		Log.d( TAG, "Stopping proxy ..." );
		
		try 
		{
			if( mSocket != null )
				mSocket.close();
		} 
		catch( IOException e )
		{

		}
		
		mRunning = false;
		mSocket  = null;
	}
	
	public void run() {
		
		try
		{
			if( mSocket == null )
				mSocket = new ServerSocket( mPort, BACKLOG, mAddress );		
			
			Log.d( TAG, "Proxy started on " + mAddress + ":" + mPort );
			
			mRunning = true;
			
			while( mRunning )
			{
				try
				{
					Socket client = mSocket.accept();
					
					new ProxyThread( client, mFilters ).start();
				}
				catch( IOException e )
				{
					Log.e( TAG, e.toString() );
				}
			}
			
			Log.d( TAG, "Proxy stopped." );
		}
		catch( IOException e )
		{
			Log.e( TAG, e.toString() );
		}
	}
}
