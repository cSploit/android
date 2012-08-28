package it.evilsocket.dsploit.system;

import android.content.Context;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import com.stericson.RootTools.CommandCapture;
import com.stericson.RootTools.RootTools;

public class ToolsInstaller {
	private final static String   TOOLS_FILENAME   = "tools.zip";
	private final static int      BUFFER_SIZE      = 4096;
	private final static String[] INSTALL_COMMANDS = {
		"chmod 755 {PATH}/utils/busybox",
		"chmod 777 {PATH}/*/",
		"chmod 777 {PATH}/ettercap/share",
		"chmod 777 {PATH}/ettercap/share/*",
		"chmod 777 {PATH}/ettercap/filters",
		"chmod 777 {PATH}/ettercap/filters/*",
		"chmod 755 {PATH}/nmap/nmap",
		"chmod 755 {PATH}/ettercap/ettercap",
		"chmod 755 {PATH}/ettercap/etterfilter",
		"chmod 755 {PATH}/ettercap/etterlog",
		"chmod 755 {PATH}/driftnet/*",
		"chmod 666 {PATH}/hydra/*",
		"chmod 755 {PATH}/hydra/hydra",
		"chmod 755 {PATH}/utils/*",
		"mount -o remount,rw /system /system && (chmod 6755 /system/*/su; mount -o remount,ro /system /system)"
	};
	
	private Context mAppContext = null;
	private String  mDestPath   = null;
	
	public ToolsInstaller( Context c ){
		mAppContext = c;
		mDestPath   = mAppContext.getFilesDir().getAbsolutePath();
	}
	
	public boolean needed( )
	{
		return !( new File( mDestPath + "/libs" ) ).exists();
	}
	
	public boolean install( )
	{
		ZipInputStream   zipInput;
		ZipEntry 		 zipEntry;
	    byte[] 		     buffer = new byte[ BUFFER_SIZE ];
	    int				 read;
	    FileOutputStream fileOutput;
	    File			 file;
	    String			 fileName;
	    
	    try
	    {
	    	zipInput = new ZipInputStream( new BufferedInputStream( mAppContext.getAssets().open( TOOLS_FILENAME ) ) );
	    	
	    	while( ( zipEntry = zipInput.getNextEntry() ) != null )
	    	{
	    		fileName = mDestPath + "/" + zipEntry.getName();
		    	file 	 = new File( mDestPath + "/" + zipEntry.getName() );

	    		if( zipEntry.isDirectory() )
	    			file.mkdirs();
	    		
	    		else
	    		{
	    			fileOutput = new FileOutputStream( fileName );
	    			
	    			while( (read = zipInput.read( buffer, 0, BUFFER_SIZE ) ) > -1 ) 
	    			{
	    				fileOutput.write( buffer, 0, read );
	                }

	    			fileOutput.close();
	    			zipInput.closeEntry();
	    		}
	    	}
	    	
	    	zipInput.close();
	    	
	    	String cmd = "";
	    	
	    	for( String install_cmd : INSTALL_COMMANDS )
	    	{
	    		cmd += install_cmd.replace( "{PATH}", mDestPath + "/tools" ) + "; ";
	    	}
	    	
	    	CommandCapture command = new CommandCapture( 0, cmd );
        	
        	RootTools.getShell(true).add(command).waitForFinish();
	    	
	    	return true;
	    }
	    catch( Exception e )
	    {
	    	Log.e( "ToolsInstaller", e.getMessage() );
	    	
	    	return false;
	    }
	}
}
