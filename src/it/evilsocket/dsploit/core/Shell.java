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
package it.evilsocket.dsploit.core;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;

public class Shell 
{
	private static final String TAG = "SHELL";
	
	private static ArrayList<String> mPath = null;

	public static boolean isRootGranted( ) {
		Process			 process = null;
		DataOutputStream writer  = null;
		BufferedReader   reader  = null;
		String			 line    = null;
		boolean 		 granted = false;

		try {
			process = Runtime.getRuntime().exec( System.getSuPath() );
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
			System.errorLogging( TAG, e );
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
		}
		
		return granted;		
	}
	
	public interface OutputReceiver
	{
		public void onStart( String command );
		public void onNewLine( String line );
		public void onEnd( int exitCode );
	}
	
	public static boolean isBinaryAvailable( String binary )
	{
		if( mPath == null )
		{
			try
			{
				Shell.exec( "cat /init.rc", new OutputReceiver(){
					@Override
					public void onStart(String command) { }
		
					@Override
					public void onNewLine(String line) {
						if( line.contains("export PATH") )
	                    {
							int tmp = line.indexOf("/");
	                        mPath = new ArrayList<String>( Arrays.asList( line.substring(tmp).split(":") ) );
	                    }
					}
		
					@Override
					public void onEnd(int exitCode) { }
				});
			}
			catch( Exception e )
			{
				System.errorLogging( TAG, e );
			}
		}
		
		if( mPath != null )
		{
			for( String path: mPath )
			{
				String fullPath = path + '/' + binary;
				if( new File( fullPath ).exists() )
					return true;
			}
		}
		
		return false;
	}
	
	public static int exec( String command, OutputReceiver receiver ) throws IOException, InterruptedException {
		Process			 process = null;
		DataOutputStream writer  = null;
		BufferedReader   reader  = null;
		String			 line    = null,
						 libPath = System.getLibraryPath();
		String[] 		 envp    = { "LD_LIBRARY_PATH=" + libPath + ":$LD_LIBRARY_PATH" };
				
		process = Runtime.getRuntime().exec( System.getSuPath(), envp );
				
		if( receiver != null ) receiver.onStart( command );
		
		writer = new DataOutputStream( process.getOutputStream() );
		reader = new BufferedReader( new InputStreamReader( process.getInputStream() ) );
		
		writer.writeBytes( "export LD_LIBRARY_PATH=" + libPath + ":$LD_LIBRARY_PATH\n" );
		writer.flush();
		writer.writeBytes( command + "\n" );
		writer.flush();
		writer.writeBytes( "exit\n" );
		writer.flush();
		
		while( ( line = reader.readLine() ) != null )
		{
			if( receiver != null ) receiver.onNewLine( line );
		}
		
		int exit = process.waitFor();
		
		if( receiver != null ) receiver.onEnd( exit );
		
		return exit;
	}
	
	public static int exec( String command ) throws IOException, InterruptedException {
		return exec( command, null );
	}
	
	public static Thread async( String command, OutputReceiver receiver ) {
		final String 		 fCommand  = command;
		final OutputReceiver fReceiver = receiver;
		
		return new Thread( new Runnable(){
			@Override
			public void run() {
				try
				{
					exec( fCommand, fReceiver );
				}
				catch( Exception e )
				{
					System.errorLogging( TAG, e );
				}
			}} 
		);
	}
	
	public static Thread async( String command ) {
		return async( command, null );
	}
}
