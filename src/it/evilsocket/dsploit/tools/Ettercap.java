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

import java.io.IOException;

import it.evilsocket.dsploit.net.Target;

import android.content.Context;
import android.util.Log;

public class Ettercap extends Tool
{
	private static final String TAG = "ETTERCAP";
	
	public Ettercap( Context context ){
		super( "ettercap/ettercap", context );		
	}
	
	public static abstract class SnifferOutputReceiver implements Tool.OutputReceiver
	{		
		public void OnStart( String commandLine) {
			Log.d( TAG, "sniff OnStart( " + commandLine + " )" );
		}
		
		public void OnNewLine( String line ) {			
			Log.d( TAG, "sniff OnNewLine( " + line + " )" );			
		}
		
		public void OnEnd( int exitCode ) {
			// TODO: check exit code
			Log.d( TAG, "sniff OnEnd( " + exitCode +" )" );
		}		
	}
	
	public void sniff( Target target, SnifferOutputReceiver receiver )
	{
		try
		{
			String address = target.getType() == Target.Type.NETWORK ? "//" : "/" + target.getCommandLineRepresentation() + "/";
			
			super.run( "-T -M ARP " + address, receiver );
		}
		catch( InterruptedException ie )
		{
			Log.e( TAG, ie.toString() );
		}
		catch( IOException ioe )
		{
			Log.e( TAG, ioe.toString() );
		}		
	}
}
