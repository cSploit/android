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

import it.evilsocket.dsploit.system.Shell;
import it.evilsocket.dsploit.system.Shell.OutputReceiver;

import java.io.File;
import java.io.IOException;
import android.content.Context;
import android.util.Log;

public class Tool 
{
	private static final String TAG = "Tool";
	
	private File    mFile		 = null;
	private String  mName 		 = null;
	private String  mDirName	 = null;
	private String  mFileName    = null;
	private String  mLibPath     = null;
	private Context mAppContext  = null;
	private boolean mCustomLibs  = true;

	public Tool( String name, Context context ) {
		mAppContext = context;
		mLibPath	= mAppContext.getFilesDir().getAbsolutePath() + "/tools/libs";
		mFileName   = mAppContext.getFilesDir().getAbsolutePath() + "/tools/" + name;
		mFile	    = new File( mFileName );
		mName		= mFile.getName();
		mDirName	= mFile.getParent();
	}
	
	public Tool( String name ){
		mName = name;
	}
	
	public void setCustomLibsUse( boolean use ){
		mCustomLibs = use;
	}
	
	public void run( String args, OutputReceiver receiver ) throws IOException, InterruptedException {		
		String cmd = null;
		
		if( mAppContext != null )
			if( mCustomLibs )
				cmd = "LD_LIBRARY_PATH=" + mLibPath + ":$LD_LIBRARY_PATH && cd " + mDirName + " && ./" + mName + " " + args;		
			else
				cmd = "cd " + mDirName + " && ./" + mName + " " + args;
		else
			cmd = mName + " " + args;

		Shell.exec( cmd, receiver );
	}
	
	public void run( String args ) throws IOException, InterruptedException {
		run( args, null );
	}
	
	public Thread async( String args, OutputReceiver receiver ) {
		String cmd = null;
		
		if( mAppContext != null )
			if( mCustomLibs )
				cmd = "LD_LIBRARY_PATH=" + mLibPath + ":$LD_LIBRARY_PATH && cd " + mDirName + " && ./" + mName + " " + args;		
			else
				cmd = "cd " + mDirName + " && ./" + mName + " " + args;
		else
			cmd = mName + " " + args;

		return Shell.async( cmd, receiver );
	}
	
	public boolean kill(){
		try
		{
			// TODO: Kill process by pid instead that by name
			
			// Some processes like ettercap need a sigint ( ctrl+c ) to restore previous state
			// such as the arp table.
			Shell.exec( "killall -SIGINT " + mName );
			Thread.sleep( 200 );
			Shell.exec( "killall -9 " + mName );
			
			return true;
		}
		catch( Exception e )
		{ 
			Log.e( TAG, e.toString() );
		}
		
		return false;
	}
}
