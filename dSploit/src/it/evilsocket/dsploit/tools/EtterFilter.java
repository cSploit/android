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

import it.evilsocket.dsploit.core.System;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;

import android.content.Context;

public class EtterFilter extends Tool
{
	private static final String TAG = "ETTERFILTER";
	
	private String mFileName   = null;
	private File   mTmpFile    = null;
	private String mFilterData = "";

	public EtterFilter( Context context, String scriptName ) throws IOException {
		super( "ettercap/etterfilter", context );		
		
		mFileName = mDirName + "/filters/" + scriptName;
		mTmpFile  = File.createTempFile( "DSPLOIT", scriptName, context.getCacheDir() );
		
		mTmpFile.deleteOnExit();
		
		FileInputStream fstream = new FileInputStream( mFileName );
		DataInputStream in 	 	= new DataInputStream(fstream);
		BufferedReader  reader	= new BufferedReader(new InputStreamReader(in));
		String 		 	line    = null;
		  
		while( ( line = reader.readLine() ) != null )
			mFilterData += line + "\n";
		 
		in.close();
	}
	
	public void setVariable( String name, String value ) {
		mFilterData = mFilterData.replace( name, value );
	}
	
	public void compile( String output ) {
		try
		{
			BufferedWriter writer = new BufferedWriter( new FileWriter( mTmpFile ) );
			writer.write( mFilterData );
			writer.close();
					    
			super.run( mTmpFile.getAbsolutePath() + " -o " + output );
		}
		catch( Exception e )
		{
			System.errorLogging( TAG, e );
		}
	}
	
}
