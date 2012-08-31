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
import java.util.regex.Matcher;
import java.util.regex.Pattern;

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
	
	public Thread sniff( String filter, OutputReceiver receiver ) {
		return super.async( "-l -vv -s 0 " + ( filter == null ? "" : filter ), receiver );
	}	
	
	public void sniff( OutputReceiver receiver ) {
		sniff( null, receiver );
	}
		
	public static class Account
	{
		public enum Type 
		{
			FTP,
			HTTP,
			SMTP,
			IMAP,
			UNKNOWN;
			
			public static Type fromString( String port ) {
				port = port.trim().toLowerCase();
				
				if( port.equals("21" ) )
					return FTP;
				
				else if( port.equals("80") )
					return HTTP;
				
				else if( port.equals("25") )
					return SMTP;
				
				else if( port.equals("143") )
					return IMAP;
				
				return UNKNOWN;
			}
		}
		
		public Type	  type;
		public String username = "";
		public String password = "";
	}
	
	public static class Session
	{
		public String  address = "";
		public int     port    = 0;
		public Account account;
	}

	public static abstract class PasswordReceiver implements OutputReceiver
	{
		private static final Pattern TYPE_PATTERN = Pattern.compile( ".*\\s+([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})\\.([\\d]+)[\\s\\:].*", Pattern.CASE_INSENSITIVE );
		
		private String		 mLastServer = null;
		private Account.Type mLastType   = Account.Type.UNKNOWN;		
		private String		 mLastUser   = null;
		private String       mLastPass   = null;
		
		public void onStart( String commandLine ) { 
			Log.d( "PasswordReceiver", "Started '" + commandLine +"'" );
		}
		
		// TODO: Add better tcp stream management 
		public void onNewLine( String line ) {			
			Matcher match = null;
			
			line = line.trim();
			
			// STEP 1 : Determine the stream type
			if( mLastType == Account.Type.UNKNOWN && mLastServer == null )
			{
				if( ( match = TYPE_PATTERN.matcher( line ) ) != null && match.find() )
				{
					mLastServer = match.group( 1 );
					mLastType   = Account.Type.fromString( match.group( 2 ) );					
					
					Log.e( TAG, "Found a stream start - " + mLastServer + ":" + mLastType + " ( " + match.group( 2 ) + " )" );
					
					// reset, unhandled protocol
					if( mLastType == Account.Type.UNKNOWN )					
						mLastServer = null;
				}
			}
			else if( mLastType == Account.Type.FTP )
			{
				// handle stream
			}
			else if( mLastType == Account.Type.HTTP )
			{
				// handle stream
			}
			else if( mLastType == Account.Type.SMTP )
			{
				// handle stream
			}
			else if( mLastType == Account.Type.IMAP )
			{
				// handle stream
			}
		}
		
		public void onEnd( int exitCode ) { 
			Log.d( "PasswordReceiver", "Ended with exit code '" + exitCode +"'" );
		}
		
		public abstract void onAccountFound( String port, String protocol );
	}
	
	public Thread sniffPasswords( String filter, PasswordReceiver receiver ) {
		return super.async( "-n -l -vvv -s 0 -A " + ( filter == null ? "" : filter ), receiver );
	}
}
