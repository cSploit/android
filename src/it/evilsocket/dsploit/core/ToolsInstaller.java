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

import android.content.Context;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class ToolsInstaller 
{
	private final static String   TAG			   = "TOOLSINSTALLER";
	private final static String   TOOLS_FILENAME   = "tools.zip";
	private final static int      BUFFER_SIZE      = 4096;
	
	public  final static String[] TOOLS 		   = {
		"ettercap",
		"nmap",
		"arpspoof",
		"tcpdump",
		"hydra"
	};

	private final static String[] INSTALL_COMMANDS = {
		"chmod 777 {PATH}/*/",
		"chmod 777 {PATH}/ettercap/share",
		"chmod 777 {PATH}/ettercap/share/*",
		"chmod 777 {PATH}/ettercap/filters",
		"chmod 777 {PATH}/ettercap/filters/*",
		"chmod 755 {PATH}/ettercap/ettercap",
		"chmod 755 {PATH}/ettercap/etterfilter",
		"chmod 755 {PATH}/ettercap/etterlog",
		"chmod 755 {PATH}/nmap/nmap",
		"chmod 755 {PATH}/arpspoof/arpspoof",
		"chmod 755 {PATH}/tcpdump/tcpdump",
		"chmod 666 {PATH}/hydra/*",
		"chmod 755 {PATH}/hydra/hydra",
		"mount -o remount,rw /system /system && ( chmod 6755 /system/*/su; mount -o remount,ro /system /system )"
	};
	
	private Context mAppContext = null;
	private String  mDestPath   = null;
	
	public ToolsInstaller( Context c ){
		mAppContext = c;
		mDestPath   = mAppContext.getFilesDir().getAbsolutePath();
	}
	
	public boolean needed( )
	{
		return !( new File( mDestPath + "/tools/libs" ) ).exists();
	}
	
	public boolean install( )
	{
		Log.d( TAG, "Installing tools ..." );
		
		// Avoid some binary file being busy as a running process.
		System.clean( false );
		
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
	    	        	
        	Shell.exec( cmd );
	    	
	    	return true;
	    }
	    catch( Exception e )
	    {
	    	System.errorLogging( TAG, e );
	    	
	    	return false;
	    }
	}
}
