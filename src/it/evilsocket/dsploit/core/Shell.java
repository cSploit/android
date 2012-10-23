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
import java.util.Map;

import android.content.Context;

public class Shell 
{
	private static final String TAG = "SHELL";	
	/*
	 * "gobblers" seem to be the recommended way to ensure the streams
	 * don't cause issues
	 */
	private static class StreamGobbler extends Thread 
	{
		private BufferedReader mReader   = null;
		private OutputReceiver mReceiver = null;
		
		public StreamGobbler( BufferedReader reader, OutputReceiver receiver ) {
			mReader   = reader;
			mReceiver = receiver;
			
			setDaemon( true );
		}

		public void run() {
			try 
			{
				while(true) 
				{
					String line = "";
					if( mReader.ready() ) 
					{
						if( ( line = mReader.readLine() ) == null )
							continue;
					} 
					else 
					{
						try 
						{
							Thread.sleep(200);
						} 
						catch( InterruptedException e ){ } 
						
						continue;
					}
					
					if( line != null && line.isEmpty() == false && mReceiver != null )
						mReceiver.onNewLine( line );
				}
			} 
			catch( IOException e ) 
			{
				System.errorLogging( TAG, e ); 				
			} 
			finally 
			{
				try 
				{
					mReader.close();
				} 
				catch( IOException e ) {
					//swallow error
				}
			}
		}
	}
	
	private static Process spawnShell( String command, boolean bUpdateLibraryPath, boolean bRedirectErrorStream ) throws IOException {
		ProcessBuilder 		builder 	= new ProcessBuilder().command( command );
		Map<String, String> environment = builder.environment();
		
		builder.redirectErrorStream( bRedirectErrorStream );
		
		if( bUpdateLibraryPath )
		{
			boolean found  = false;
			String libPath = System.getLibraryPath();

			for( Map.Entry<String, String> entry : environment.entrySet() )
			{
				if( entry.getKey().equals("LD_LIBRARY_PATH") )
				{
					environment.put( "LD_LIBRARY_PATH", entry.getValue() + ":" + libPath );
					found = true;
					break;
				}								
			}
			
			if( found == false )			
				environment.put( "LD_LIBRARY_PATH", libPath );			
		}
		
		return builder.start();
	}
	
	private static Process spawnShell( String command ) throws IOException {
		return spawnShell( command, false, true ); 
	}
	
	public static boolean isRootGranted( ) {
		Process			 process = null;
		DataOutputStream writer  = null;
		BufferedReader   reader  = null;
		String			 line    = null;
		boolean 		 granted = false;

		try 
		{
			process = spawnShell( "su" );
			writer  = new DataOutputStream( process.getOutputStream() );
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
		try
		{
			Process 	     process  = spawnShell( "sh" );
			DataOutputStream writer   = new DataOutputStream( process.getOutputStream() );;
			BufferedReader   reader   = new BufferedReader( new InputStreamReader( process.getInputStream() ) );  
			String		     line	  = null;
			
			writer.writeBytes( "which " + binary + "\n" );
			writer.flush();
			writer.writeBytes( "exit\n" );
			writer.flush();
			
			while( ( line = reader.readLine() ) != null ) 
			{  
				if( line.isEmpty() == false && line.startsWith("/") )					
					return true;
			}  				
		}
		catch( Exception e )
		{
			System.errorLogging( TAG, e );
		}
				
		return false;
	}
	
	public static boolean isLibraryPathOverridable( Context context ) {
		boolean linkerError = false;
		
		try
		{
			String libPath  = System.getLibraryPath(),
				   fileName = context.getFilesDir().getAbsolutePath() + "/tools/nmap/nmap";
			File   file	    = new File( fileName );
			String dirName  = file.getParent(),
				   command  = "cd " + dirName + " && ./nmap --version";
			Process			 process = spawnShell( "su", true, false );
			DataOutputStream writer  = null;
			BufferedReader   reader  = null,
							 stderr  = null;
			String			 line    = null,
							 error   = null;
			
			writer = new DataOutputStream( process.getOutputStream() );
			reader = new BufferedReader( new InputStreamReader( process.getInputStream() ) );
			stderr = new BufferedReader( new InputStreamReader( process.getErrorStream() ) );
			
			writer.writeBytes( "export LD_LIBRARY_PATH=" + libPath + ":$LD_LIBRARY_PATH\n" );
			writer.flush();
			
			writer.writeBytes( command + "\n" );
			writer.flush();
			writer.writeBytes( "exit\n" );
			writer.flush();

			while( ( ( line = reader.readLine() ) != null || ( error = stderr.readLine() ) != null ) && !linkerError )
			{
				line = line == null ? error : line;
				
				if( line.contains("CANNOT LINK EXECUTABLE") )
					linkerError = true;
			}
			// flush stderr		
			while( ( error = stderr.readLine() ) != null && !linkerError )
			{
				if( line.contains("CANNOT LINK EXECUTABLE") )
					linkerError = true;
			}

			process.waitFor();			
		}
		catch( Exception e )
		{
			System.errorLogging( TAG, e );
			
			return !linkerError;
		}
		
		return !linkerError;
	}
	
	public static int exec( String command, OutputReceiver receiver, boolean overrideLibraryPath ) throws IOException, InterruptedException {
		Process			 process = spawnShell( "su", overrideLibraryPath, false );
		DataOutputStream writer  = null;
		BufferedReader   reader  = null,
						 error   = null;
		String			 libPath = System.getLibraryPath();
		int 			 exit 	 = -1;

		if( receiver != null ) receiver.onStart( command );
				
		writer = new DataOutputStream( process.getOutputStream() );
		reader = new BufferedReader( new InputStreamReader( process.getInputStream() ) );
		error  = new BufferedReader( new InputStreamReader( process.getErrorStream() ) );

		// is this really needed ?
		if( overrideLibraryPath )
		{
			writer.writeBytes( "export LD_LIBRARY_PATH=" + libPath + ":$LD_LIBRARY_PATH\n" );
			writer.flush();
		}
		
		try
		{
			writer.writeBytes( command + "\n" );
			writer.flush();
			
			StreamGobbler outGobbler = new StreamGobbler( reader, receiver ),
						  errGobbler = new StreamGobbler( error, receiver );
			
			outGobbler.start();
			errGobbler.start();
							
			writer.writeBytes( "exit\n" );
			writer.flush();
											
			/* 
			 * The following catastrophe of code seems to be the best way to ensure 
			 * this thread can be interrupted.
			 */
			while( !Thread.currentThread().isInterrupted() ) 
			{
				try 
				{
					exit = process.exitValue();
					Thread.currentThread().interrupt();
				} 
				catch( IllegalThreadStateException e ) 
				{
					/*
					 * Just sleep, the process hasn't terminated yet but sleep should (but doesn't) cause 
					 * InterruptedException to be thrown if interrupt() has been called.
					 * 
					 * .25 seconds seems reasonable
					 */
					Thread.sleep(250);
				}
			}		
		}
		catch( IOException e ) 
		{
			System.errorLogging( TAG, e );
		} 
		catch( InterruptedException e ) 
		{
			try 
			{
				// key to killing executable and process
				writer.close();
				reader.close();
				error.close();
			} 
			catch( IOException ex ) 
			{
				// swallow error
			} 			
		} 

		if( receiver != null ) receiver.onEnd( exit );
		
		// android.util.Log.d( TAG, command + " -> EXITED WITH CODE " + exit );
		
		return exit;
	}
	
	public static int exec( String command, OutputReceiver reciever ) throws IOException, InterruptedException {
		return exec( command, reciever, true );
	}
	
	public static int exec( String command ) throws IOException, InterruptedException {
		return exec( command, null, true );
	}
	
	public static Thread async( final String command, final OutputReceiver receiver, final boolean overrideLibraryPath ) {
		Thread launcher = new Thread( new Runnable(){
			@Override
			public void run() {
				try
				{
					exec( command, receiver, overrideLibraryPath );
				}
				catch( Exception e )
				{
					System.errorLogging( TAG, e );
				}
			}} 
		);
		
		launcher.setDaemon( true );
		
		return launcher;
	}
	
	public static Thread async( String command ) {
		return async( command, null, true );
	}
	
	public static Thread async( final String command, final OutputReceiver receiver ) {
		return async( command, receiver, true );
	}
}
