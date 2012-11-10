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
package it.evilsocket.dsploit.plugins.mitm;

import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.tools.Ettercap.OnAccountListener;

public class SpoofSession 
{
	private static final String TAG = "SPOOFSESSION";
	
	private boolean mWithProxy  	= false;
	private boolean mWithServer 	= false;
	private String  mServerFileName = null;
	private String  mServerMimeType = null;
	
	public static abstract interface OnSessionReadyListener 
	{
		public void onSessionReady();
		public void onError( String error );
	}
	
	public SpoofSession( boolean withProxy, boolean withServer, String serverFileName, String serverMimeType ) {
		mWithProxy  	= withProxy;
		mWithServer 	= withServer;
		mServerFileName = serverFileName;
		mServerMimeType = serverMimeType;
	}
	
	public SpoofSession() {
		// Standard spoof session with only transparent proxy
		this( true, false, null, null );
	}
			
	public void start( Target target, final OnSessionReadyListener listener ) {
		this.stop();
		
		if( mWithProxy )
		{
			if( System.getSettings().getBoolean( "PREF_HTTPS_REDIRECT", true ) )
				new Thread( System.getHttpsRedirector() ).start();
			
			new Thread( System.getProxy() ).start();
		}
		
		if( mWithServer )
		{
			try
			{
				System.getServer().setResource( mServerFileName, mServerMimeType );
				new Thread( System.getServer() ).start();
			}
			catch( Exception e )
			{
				System.errorLogging( TAG, e );
				mWithServer = false;
			}
		}
					
		System.getArpSpoof().spoof( target, new OutputReceiver() {			
			@Override
			public void onStart(String command) { 
				// android.util.Log.d( "ARPSPOOF", command );				
				System.setForwarding( true );
				
				if( mWithProxy )
				{
					if( System.getSettings().getBoolean( "PREF_HTTPS_REDIRECT", true ) )
						System.getIPTables().portRedirect( 443, System.HTTPS_REDIR_PORT );
					
					System.getIPTables().portRedirect( 80,  System.HTTP_PROXY_PORT );
				}
				
				listener.onSessionReady();
			}
			
			@Override
			public void onNewLine( String line ) {
				// android.util.Log.d( "ARPSPOOF", line );
				
				if( line.startsWith( "[ERROR]" ) )
					listener.onError( line.substring( "[ERROR]".length() + 1 ).trim() );
			}
			
			@Override
			public void onEnd(int exitCode) { 
				// android.util.Log.d( "ARPSPOOF", "onEnd( " + exitCode + " )" );
			}
		}).start();
	}
	
	public void start( final Target target, final OnAccountListener listener ) {
		this.stop();		
		System.getArpSpoof().spoof( target, new OutputReceiver() {			
			@Override
			public void onStart(String command) { 
				// Log.d( "ARPSPOOF", command );
				
				System.setForwarding( true );				
				System.getEttercap().dissect( target, listener ).start();	
			}
			
			@Override
			public void onNewLine(String line) {
				// Log.d( "ARPSPOOF", line );
			}
			
			@Override
			public void onEnd(int exitCode) { 
				// Log.d( "ARPSPOOF", "onEnd( " + exitCode + " )" );
			}
		}).start();					
	}
	
	public void start( final OnSessionReadyListener listener ) {
		this.start( System.getCurrentTarget(), listener );
	}
	
	public void start( OnAccountListener listener ) {
		this.start( System.getCurrentTarget(), listener );
	}
	
	public void stop() {
		System.getArpSpoof().kill();
		System.getEttercap().kill();
		System.setForwarding( false );
				
		if( mWithProxy )
		{
			System.getIPTables().undoPortRedirect( 80,  System.HTTP_PROXY_PORT );
			if( System.getSettings().getBoolean( "PREF_HTTPS_REDIRECT", true ) )
			{
				System.getIPTables().undoPortRedirect( 443, System.HTTPS_REDIR_PORT );
				
				if( System.getHttpsRedirector() != null )
					System.getHttpsRedirector().stop();
			}
			System.getProxy().stop();
			System.getProxy().setRedirection( null, 0 );
			System.getProxy().setFilter( null );
		}
		
		if( mWithServer )		
			System.getServer().stop();		
	}
}
