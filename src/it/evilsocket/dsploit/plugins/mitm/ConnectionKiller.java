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
import it.evilsocket.dsploit.tools.Ettercap.OnReadyListener;

public class ConnectionKiller 
{
	public static void start() {
		System.getEttercap().drop( System.getCurrentTarget(), new OnReadyListener(){
			@Override
			public void onReady() 
			{
				// just in case :)
				System.setForwarding( false );							
			}
			
		}).start();		
	}
	
	public static void stop() {
		System.setForwarding( false );
		System.getEttercap().kill();
	}
}
