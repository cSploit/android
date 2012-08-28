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
