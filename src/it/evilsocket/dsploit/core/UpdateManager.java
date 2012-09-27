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
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Arrays;

import javax.net.ssl.HttpsURLConnection;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.StrictMode;
import android.util.Log;

public class UpdateManager 
{
	private static final String TAG					= "UPDATEMANAGER";
	private static final String REMOTE_VERSION_FILE = "https://raw.github.com/evilsocket/dsploit/master/VERSION";
	private static final String VERSION_CHAR_MAP 	= "zyxwvutsrqponmlkjihgfedcba";
	
	private Context mContext		  = null;
	private String  mInstalledVersion = null;
	private String  mRemoteVersion	  = null;
	
	public UpdateManager( Context context ) {
		mContext = context;
		
		try
		{
			PackageManager manager = mContext.getPackageManager();
			PackageInfo    info    = manager.getPackageInfo( mContext.getPackageName(), 0 );
			
			mInstalledVersion = info.versionName;
		}
		catch( Exception e )
		{
			Log.e( TAG, e.getMessage() );
		}
	}
			
	private static double getVersionCode( String version )
	{		
		String[] padded = new String[ 3 ],
				 parts  = version.split( "[^0-9a-zA-Z]" );
		String 	 item   = "",
				 digit	= "";
		double   code	= 0,
				 coeff	= 0;
		int		 i, j;
		
		Arrays.fill( padded, 0, 3, "0" );
		
		for( i = 0; i < Math.min( 3, parts.length ); i++ )
		{
			padded[i] = parts[i];
		}
		
		for( i = padded.length - 1; i >= 0; i-- )
		{
			item  = padded[ i ];
			coeff = Math.pow( 10, padded.length - i );
		
			for( j = 0; j < item.length(); j++ )
			{
				digit = "" + item.charAt( j );
						
				if( digit.matches( "\\d" ) )				
					code += ( Integer.parseInt( digit ) + 1 ) * coeff;
				
				else if( digit.matches( "[a-zA-Z]" ) )				
					code -= ( ( VERSION_CHAR_MAP.indexOf( digit.toLowerCase() ) + 1 ) / 100.0 );
				
				else
					code += coeff;
			}			
		}
		
		return code;
	}
	
	public boolean isUpdateAvailable( ) {
		
		try
		{
			if( mInstalledVersion != null )
			{
				// Read remote version
				if( mRemoteVersion == null )
				{
					// This is needed to avoid NetworkOnMainThreadException
		        	StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
		        	StrictMode.setThreadPolicy(policy);
		        						
					URL 			   url 		  = new URL( REMOTE_VERSION_FILE );
				    HttpsURLConnection connection = (HttpsURLConnection)url.openConnection();
				    BufferedReader	   reader	  = new BufferedReader( new InputStreamReader( connection.getInputStream() ) );
				    String			   line		  = null,
				    				   buffer	  = "";
				     
				    while( ( line = reader.readLine() ) != null )
				    {
				    	buffer += line + "\n";
				    }
				    
				    reader.close();
				    
				    mRemoteVersion = buffer.trim();
				}
								
				// Compare versions
				double installedVersionCode = getVersionCode( mInstalledVersion ),
					   remoteVersionCode	= getVersionCode( mRemoteVersion );
				
				if( remoteVersionCode > installedVersionCode )
					return true;
			}
		}
		catch( Exception e )
		{
			Log.e( TAG, e.getMessage() );
		}
		
		return false;
	}
	
	public String getRemoteVersion() {
		return mRemoteVersion;
	}
	
	public String getRemoteVersionFileName() {
		return "dSploit-" + mRemoteVersion + ".apk";
	}
	
	public String getRemoteVersionUrl() {
		return "http://cloud.github.com/downloads/evilsocket/dsploit/" + getRemoteVersionFileName();
	}
	
	public boolean downloadUpdate(  )
	{
		try 
		{
			URL 			  url 		 = new URL( getRemoteVersionUrl() );
            HttpURLConnection connection = (HttpURLConnection)url.openConnection();
            File 			  file 	     = new File( System.getStoragePath() );
            String			  fileName   = getRemoteVersionFileName();
            byte[] 		      buffer 	 = new byte[1024];
            int		   	      read		 = 0;

            connection.connect();
            
            file.mkdirs();
            file = new File( file, fileName );
            if( file.exists() )
                file.delete();
            
            FileOutputStream writer = new FileOutputStream( file );
            InputStream 	 reader = connection.getInputStream();
            
            while( ( read = reader.read(buffer) ) != -1 ) 
            {
                writer.write( buffer, 0, read );
            }
            
            writer.close();
            reader.close();

            Intent intent = new Intent( Intent.ACTION_VIEW );
            intent.setDataAndType( Uri.fromFile( file ), "application/vnd.android.package-archive" );
            intent.setFlags( Intent.FLAG_ACTIVITY_NEW_TASK );
            
            mContext.startActivity(intent);
            
            return true;
		} 
		catch( Exception e ) 
		{
			e.printStackTrace();
	    	Log.e( TAG, e.getMessage() );
	    }
	    
	    return false;
	}
}
