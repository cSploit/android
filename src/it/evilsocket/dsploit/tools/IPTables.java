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

import android.util.Log;

public class IPTables extends Tool
{			
	private static final String TAG = "IPTABLES";
	
	public IPTables( ){
		super( "iptables" );		
		setCustomLibsUse(false);		
	}
	
	public void portRedirect( int from, int to ){
		try
		{
			super.run("-P FORWARD ACCEPT");
			super.run("-t nat -A POSTROUTING -j MASQUERADE");
			super.run("-t nat -A PREROUTING -p tcp --dport " + from + " -j REDIRECT --to-port " + to );
		}
		catch( Exception e )
		{
			Log.e( TAG, e.toString() );
		}
	}
	
	public void undoPortRedirect( int from, int to ){
		try
		{
			super.run("-t nat -D PREROUTING -p tcp --dport " + from + " -j REDIRECT --to-port " + to );
			super.run("-t nat -D POSTROUTING -j MASQUERADE");
		}
		catch( Exception e )
		{
			Log.e( TAG, e.toString() );
		}
	}
	
}
