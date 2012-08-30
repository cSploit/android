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
package it.evilsocket.dsploit.system;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;

import android.util.Log;

public class Shell 
{
	private static final String TAG = "SHELL";
	
	public static boolean isRootGranted( ) {
		Process			 process = null;
		DataOutputStream writer  = null;
		BufferedReader   reader  = null;
		String			 line    = null;
		boolean 		 granted = false;

		try {
			process = Runtime.getRuntime().exec("su");
			writer  = new DataOutputStream(process.getOutputStream());
			reader  = new BufferedReader( new InputStreamReader(process.getInputStream()) );

			writer.writeBytes( "id\n" );
			writer.flush();
			writer.writeBytes( "exit\n" );
			writer.flush();
			
			while( ( line = reader.readLine() ) != null && !granted )
			{
				if( line.toLowerCase().contains("uid=0") )
					granted = true;
			}

			process.waitFor();

		} 
		catch ( Exception e ) 
		{
			Log.e( TAG, e.toString() );
		} 
		finally 
		{
			try 
			{
				if( writer != null ) 
					writer.close();
				
				if( reader != null )
					reader.close();
			} 
			catch (IOException e) {
				// ignore errors
			} 
			finally 
			{
				if( process != null )
					process.destroy();
			}
		}
		
		return granted;		
	}
	
	public interface OutputReceiver
	{
		public void onStart( String command );
		public void onNewLine( String line );
		public void onEnd( int exitCode );
	}
	
	public static int exec( String command, OutputReceiver receiver ) throws IOException, InterruptedException {
		Process			 process = null;
		ProcessBuilder   builder = null;
		DataOutputStream writer  = null;
		BufferedReader   reader  = null;
		String			 line    = null;
		
		builder = new ProcessBuilder().command("su");
		builder.redirectErrorStream(true);
		
		process = builder.start();
		
		receiver.onStart( command );
		
		writer = new DataOutputStream(process.getOutputStream());
		reader = new BufferedReader( new InputStreamReader( process.getInputStream() ) );
		
		writer.writeBytes( command + "\n" );
		writer.flush();
		writer.writeBytes( "exit\n" );
		writer.flush();
		
		while( ( line = reader.readLine() ) != null )
		{
			receiver.onNewLine( line );
		}
		
		int exit = process.waitFor();
		
		receiver.onEnd( exit );
		
		return exit;
	}
	
	public static int exec( String command ) throws IOException, InterruptedException {
		return exec( command, new OutputReceiver(){
			@Override
			public void onStart(String command) {
				// TODO Auto-generated method stub
				
			}

			@Override
			public void onNewLine(String line) {
				// TODO Auto-generated method stub
				
			}

			@Override
			public void onEnd(int exitCode) {
				// TODO Auto-generated method stub
				
			}} 
		);
	}
}
