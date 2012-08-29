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
import android.content.Context;
import com.stericson.RootTools.Command;
import com.stericson.RootTools.RootTools;

public class Tool 
{
	private static final String TAG = "Tool";
	
	private String  mName 		 = null;
	private String  mDirName	 = null;
	private String  mFileName    = null;
	private String  mLibPath     = null;
	private Context mAppContext  = null;
	
	public interface OutputReceiver
	{
		public void onStart( String commandLine );
		public void onNewLine( String line );
		public void onEnd( int exitCode );
	}
	
	public Tool( String name, Context context ) {
		// TODO: Add better file name & dir name handling with File class
		mAppContext  = context;
		mName        = name.substring( name.lastIndexOf('/') + 1 );
		mFileName    = mAppContext.getFilesDir().getAbsolutePath() + "/tools/" + name;
		mDirName	 = mFileName.substring( 0, mFileName.lastIndexOf('/') );
		mLibPath     = mAppContext.getFilesDir().getAbsolutePath() + "/tools/libs";
	}
	
	public Tool( String name ){
		mName = name;
	}
	
	public void run( String args, OutputReceiver receiver ) throws IOException, InterruptedException {
		final OutputReceiver outputReceiver = receiver;
		
		String  commandLine = null;
		
		if( mAppContext != null )
			commandLine = "LD_LIBRARY_PATH=" + mLibPath + ":$LD_LIBRARY_PATH && cd " + mDirName + " && ./" + mName + " " + args;		
		else
			commandLine =  mName + " " + args;
		
		Command command = new Command( 0, commandLine )
		{
	        @Override
	        public void output(int id, String line)
	        {
	        	outputReceiver.onNewLine(line);
	        }
		};
				
		outputReceiver.onStart( commandLine );
		
		int code = RootTools.getShell(true).add( command ).exitCode();
		
		outputReceiver.onEnd( code );
	}
	
	public boolean kill(){
		return RootTools.killProcess( mName );
	}
}
