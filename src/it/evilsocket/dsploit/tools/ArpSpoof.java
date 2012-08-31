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

import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Target.Type;
import it.evilsocket.dsploit.system.Environment;
import it.evilsocket.dsploit.system.Shell.OutputReceiver;

import java.net.SocketException;

import android.content.Context;
import android.util.Log;

public class ArpSpoof extends Tool 
{
	private static final String TAG = "ARPSPOOF";
	
	// TODO: Update arpspoof binary, this sometimes just goes to seg fault :(
	public ArpSpoof( Context context ){
		super( "arpspoof/arpspoof", context );			
		// arpspoof is statically linked
		setCustomLibsUse(false);
	}
	
	public Thread spoof( Target target, OutputReceiver receiver ) {		
		String commandLine = "";
		
		try
		{					
			if( target.getType() == Type.NETWORK )
				commandLine = "-i " + Environment.getNetwork().getInterface().getDisplayName() + " " + Environment.getGatewayAddress();
			
			else
				commandLine = "-i " + Environment.getNetwork().getInterface().getDisplayName() + " -t " + target.getCommandLineRepresentation() + " " + Environment.getGatewayAddress();
		}
		catch( SocketException e )
		{
			Log.e( TAG, e.toString() );
		}
		
		return super.async( commandLine, receiver );		
	}
}
