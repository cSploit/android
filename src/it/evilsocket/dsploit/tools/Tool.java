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

import com.stericson.RootTools.Command;
import com.stericson.RootTools.RootTools;

import android.content.Context;
import android.util.Log;

public class Tool {
	private static final String TAG = "Tool";
	
	private String  mName 		 = null;
	private String  mFileName    = null;
	private String  mLibPath     = null;
	private Context mAppContext  = null;
	
	public interface OutputReceiver
	{
		public void OnStart( String commandLine );
		public void OnNewLine( String line );
		public void OnEnd( int exitCode );
	}
	
	public Tool( String name, Context context )
	{
		mAppContext  = context;
		mName        = name.substring( name.lastIndexOf('/') + 1 );
		mFileName    = mAppContext.getFilesDir().getAbsolutePath() + "/tools/" + name;
		mLibPath     = mAppContext.getFilesDir().getAbsolutePath() + "/tools/libs";
	}
	
	public void run( String args, OutputReceiver receiver ) throws IOException, InterruptedException
	{
		final OutputReceiver outputReceiver = receiver;
		
		String  commandLine = "LD_LIBRARY_PATH=" + mLibPath + ":$LD_LIBRARY_PATH && " + mFileName + " " + args;		
		Command command     = new Command( 0, commandLine )
		{
	        @Override
	        public void output(int id, String line)
	        {
	        	outputReceiver.OnNewLine(line);
	        }
		};
		
		outputReceiver.OnStart( commandLine );
		
		int code = RootTools.getShell(true).add( command ).exitCode();
		
		outputReceiver.OnEnd( code );
	}
	
	public boolean kill() {
		Log.d( TAG, "RootTools.killProcess( " + mName + " )" );
		return RootTools.killProcess( mName );
	}
}
