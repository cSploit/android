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

import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.net.Stream;
import it.evilsocket.dsploit.net.StreamAssembler;
import it.evilsocket.dsploit.net.parsers.FTPStreamParser;
import it.evilsocket.dsploit.net.parsers.HTTPCookieStreamParser;
import it.evilsocket.dsploit.net.parsers.HTTPFormStreamParser;

import android.content.Context;
import android.util.Log;

public class TcpDump extends Tool
{	
	public TcpDump( Context context ){
		super( "tcpdump/tcpdump", context );		
		// tcpdump is statically linked
		setCustomLibsUse(false);
	}
	
	public Thread sniff( String filter, OutputReceiver receiver ) {
		return super.async( "-l -vv -s 0 " + ( filter == null ? "" : filter ), receiver );
	}	
	
	public void sniff( OutputReceiver receiver ) {
		sniff( null, receiver );
	}

	public static abstract class PasswordReceiver implements OutputReceiver
	{		
		private StreamAssembler mAssembler = null;
		
		public void onStart( String commandLine ) { 
			Log.d( "PasswordReceiver", "Started '" + commandLine +"'" );
			
			mAssembler = new StreamAssembler( new StreamAssembler.NewCredentialHandler(){
				@Override
				public void onNewCredentials( Stream stream, String data ) {
					onAccountFound( stream, data );
				}} 
			);
			
			mAssembler.addStreamParser( new FTPStreamParser() );
			mAssembler.addStreamParser( new HTTPCookieStreamParser() );
			mAssembler.addStreamParser( new HTTPFormStreamParser() );
		}
		
		public void onNewLine( String line ) {			
			mAssembler.assemble(line);
		}
		
		public void onEnd( int exitCode ) { 
			Log.d( "PasswordReceiver", "Ended with exit code '" + exitCode +"'" );
		}
		
		public abstract void onAccountFound( Stream stream, String data );
	}
	
	public Thread sniffPasswords( String filter, PasswordReceiver receiver ) {
		return super.async( "-n -l -vv -s 0 -A " + ( filter == null ? "" : filter ), receiver );
	}
}
