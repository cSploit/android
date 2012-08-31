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
package it.evilsocket.dsploit.net.parsers;

import it.evilsocket.dsploit.net.Stream;

public class FTPStreamParser extends StreamParser 
{
	private static final String USER_TOKEN = "USER ";
	private static final String PASS_TOKEN = "PASS ";
	
	private Stream  mStream   = null;
	private String  mUsername = null;
	private String  mPassword = null;
	private boolean mComplete = false;
	
	@Override 
	public void onStreamStart( Stream stream ){
		mStream = stream;
	}
	
	@Override
	public void onStreamAppend( String data ) {
		if( mUsername == null && data.contains( USER_TOKEN ) )
			mUsername = data.substring( data.indexOf( USER_TOKEN ) + USER_TOKEN.length() );
		
		else if( mUsername != null && mPassword == null && data.contains( PASS_TOKEN ) )
			mPassword = data.substring( data.indexOf( PASS_TOKEN ) + PASS_TOKEN.length() );
		
		else if( mUsername != null && mPassword != null )		
			mComplete = true;		
	}
	
	@Override
	public boolean canParseStream( Stream stream ){
		return stream.type == Stream.Type.FTP;
	}
	
	@Override
	public boolean isComplete(){
		return mComplete;
	}
	
	@Override
	public String getCredentials(){
		return "FTP : " + mStream + " > USERNAME='" + mUsername + "' PASSWORD='" + mPassword + "'";
	}
	
	@Override
	public FTPStreamParser clone(){
		return new FTPStreamParser();
	}
}
