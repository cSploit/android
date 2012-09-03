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

import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.lang.Thread.UncaughtExceptionHandler;
import java.util.Date;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;

public class CrashManager implements UncaughtExceptionHandler
{
	private static final String   TAG 		  = "CRASHMANAGER";
	private static final String   EMAIL_TITLE = "dSploit Error Report";
	private static final String[] EMAIL_RECVR = new String[]{ "evilsocket@gmail.com" };
	
	private Context 				 mContext 		  = null;
	private UncaughtExceptionHandler mDefaultHandler  = null;
	private Intent					 mIntent		  = null;
	
	public CrashManager( Context context ) {
		mContext 		= context;
		mDefaultHandler = Thread.getDefaultUncaughtExceptionHandler();
		mIntent			= new Intent( Intent.ACTION_SEND );
	}
	
	public static void register( Context context ) {
		 Thread.setDefaultUncaughtExceptionHandler( new CrashManager( context) );
	}
	
	@Override
	public void uncaughtException( Thread thread, Throwable exception ) {
		try
		{
			PackageManager manager = mContext.getPackageManager();
	        PackageInfo    info    = manager.getPackageInfo( mContext.getPackageName(), 0 );
	        Writer		   sWriter = new StringWriter();
	        PrintWriter    pWriter = new PrintWriter( sWriter );
	        Throwable 	   cause   = null;
			String 		   report  = "";
	
			report = "CRASH REPORT [Ê" + ( new Date() ).toString() + " ]\n\n" +
					 
					"==========================================\n" +
					 "Environment :\n\n" +
					 
					 "  Version   : " + info.versionName + "\n" +
					 "  Package   : " + info.packageName  + "\n" +
					 "  File Path : " + mContext.getFilesDir().getAbsolutePath()    + "\n\n" +
					 "  Package Data \n" +
					 "      Phone Model : " + android.os.Build.MODEL     + "\n" +
					 "      Android Ver : " + android.os.Build.VERSION.RELEASE + "\n" +
					 "      Board       : " + android.os.Build.BOARD          + "\n" +
					 "      Brand       : " + android.os.Build.BRAND          + "\n" +
					 "      Device      : " + android.os.Build.DEVICE         + "\n" +
					 "      Display     : " + android.os.Build.DISPLAY        + "\n" +
					 "      Finger Print: " + android.os.Build.FINGERPRINT    + "\n" +
					 "      Host        : " + android.os.Build.HOST           + "\n" +
					 "      ID          : " + android.os.Build.ID             + "\n" +
					 "      Model       : " + android.os.Build.MODEL          + "\n" +
					 "      Product     : " + android.os.Build.PRODUCT        + "\n" +
					 "      Tags        : " + android.os.Build.TAGS           + "\n" +
					 "      Time        : " + android.os.Build.TIME           + "\n" +
					 "      Type        : " + android.os.Build.TYPE           + "\n" +
					 "      User        : " + android.os.Build.USER           + "\n\n" +
					 
					 "==========================================\n" +
					 "Stack Trace :\n\n";
				 
			exception.printStackTrace( pWriter );
			
			report += sWriter.toString() + "\n\n" +
					 "==========================================\n";
			
			// If the exception was thrown in a background thread inside
	        // AsyncTask, then the actual exception can be found with getCause
	        while( ( cause = exception.getCause() ) != null ) 
	        {
	            cause.printStackTrace( pWriter );	            
	        	report += sWriter.toString() + "\n";
	        }
	        
	        pWriter.close();
	        
	        Log.w( TAG, report );
	        	    
	        mIntent.addFlags( Intent.FLAG_ACTIVITY_NEW_TASK );
	        mIntent.putExtra( Intent.EXTRA_EMAIL,   EMAIL_RECVR );
	        mIntent.putExtra( Intent.EXTRA_TEXT,    report );
	        mIntent.putExtra( Intent.EXTRA_SUBJECT, EMAIL_TITLE );
	        mIntent.putExtra( Intent.EXTRA_TITLE,   EMAIL_TITLE ); 
	        mIntent.setType("message/rfc822");
	        
	        mContext.startActivity( mIntent );
		}
		catch( Exception e )
		{
			Log.e( TAG, e.toString() );
		}

		// let the exception be handled by android framework
		mDefaultHandler.uncaughtException( thread, exception );
	}

}
