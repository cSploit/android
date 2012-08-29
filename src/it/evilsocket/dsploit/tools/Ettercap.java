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
import it.evilsocket.dsploit.system.Environment;

import android.content.Context;
import android.util.Log;

public class Ettercap extends Tool
{
	private static final String TAG = "ETTERCAP";
	
	public Ettercap( Context context ){
		super( "ettercap/ettercap", context );		
	}
		
	public void sniff( Target target )
	{
		try
		{
			String targetCommand;
			
			// poison the entire network
			if( target.getType() == Target.Type.NETWORK )
				targetCommand = "// //";
			// router -> target poison
			else
				targetCommand = "/" + target.getCommandLineRepresentation() + "/ //"; 
			
			super.run( "-T -M arp:remote -i " + Environment.getNetwork().getInterface().getDisplayName() + " " + targetCommand, new Tool.OutputReceiver() {
				
				@Override
				public void onStart(String commandLine) {
					// TODO Auto-generated method stub
					Log.d( TAG, commandLine );
				}
				
				@Override
				public void onNewLine(String line) {
					// TODO Auto-generated method stub
					Log.d( TAG, line );
					
				}
				
				@Override
				public void onEnd(int exitCode) {
					// TODO Auto-generated method stub
					
				}
			} );
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
