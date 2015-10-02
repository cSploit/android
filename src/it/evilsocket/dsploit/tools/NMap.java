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
package it.evilsocket.dsploit.tools;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.net.Endpoint;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.system.Shell.OutputReceiver;
import android.content.Context;
import android.util.Log;

public class NMap extends Tool {
	private static final String TAG = "NMAP";
	
	public NMap( Context context ){
		super( "nmap/nmap", context );		
	}
	
	public static abstract class FindAliveEndpointsOutputReceiver implements OutputReceiver
	{		
		private final Pattern IP_PATTERN  = Pattern.compile( "^nmap scan report for ([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}).*", 			  Pattern.CASE_INSENSITIVE );
		private final Pattern MAC_PATTERN = Pattern.compile( "^mac address: ([a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}).*", Pattern.CASE_INSENSITIVE );
		
		private String		  mLastAddress = null;
		private String		  mLastMac     = null;
		
		public void onStart( String commandLine) {

		}
		
		public void onNewLine( String line ) {						
			Matcher matcher;
			
			if( ( matcher = IP_PATTERN.matcher( line ) ) != null && matcher.find() )
			{
				mLastAddress = matcher.group( 1 );
			}
			else if( ( matcher = MAC_PATTERN.matcher( line ) ) != null && matcher.find() && mLastAddress != null )
			{
				mLastMac = matcher.group( 1 );

				onEndpointFound( new Endpoint( mLastAddress, mLastMac ) );
				
				mLastAddress = null;
				mLastMac	 = null;
			}
		}
		
		public void onEnd( int exitCode ) {
			if( exitCode != 0 )
				Log.e( TAG, "nmap exited with code " + exitCode );
		}

		public abstract void onEndpointFound( Endpoint endpoint );
	}
	
	public Thread findAliveEndpoints( Network network, FindAliveEndpointsOutputReceiver receiver ) {	
		return super.async( "-n -sP --system-dns " + network.getNetworkRepresentation() , receiver );
	}
	
	public static abstract class SynScanOutputReceiver implements OutputReceiver
	{
		private final Pattern PORT_PATTERN = Pattern.compile( "^discovered open port (\\d+)/([^\\s]+).+", Pattern.CASE_INSENSITIVE );
		
		public void onStart( String commandLine) {
			Log.d( TAG, "synScan OnStart( " + commandLine + " )" );
		}
		
		public void onNewLine( String line ) {			
			Matcher matcher;
			
			if( ( matcher = PORT_PATTERN.matcher( line ) ) != null && matcher.find() )
			{
				onPortFound( matcher.group( 1 ), matcher.group( 2 ) );
			}				
		}
		
		public void onEnd( int exitCode ) {
			if( exitCode != 0 )
				Log.e( TAG, "nmap exited with code " + exitCode );
		}
		
		public abstract void onPortFound( String port, String protocol );
	}
	
	public Thread synScan( Target target, SynScanOutputReceiver receiver ) {		
		return super.async( "-sS -P0 --system-dns -vvv " + target.getCommandLineRepresentation() , receiver );
	}
}
