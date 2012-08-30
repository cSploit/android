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

import it.evilsocket.dsploit.system.Shell.OutputReceiver;

import java.io.IOException;

import android.content.Context;
import android.util.Log;

public class TcpDump extends Tool
{
	private static final String TAG = "TCPDUMP";
	
	public TcpDump( Context context ){
		super( "tcpdump/tcpdump", context );		
		// tcpdump is statically linked
		setCustomLibsUse(false);
	}
	
	public void sniff( String filter, OutputReceiver receiver ) {
	
		try
		{
			super.run( "-l -vv -s 0 " + ( filter == null ? "" : filter ), receiver );
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
	
	public void sniff( OutputReceiver receiver ) {
		sniff( null, receiver );
	}
}
