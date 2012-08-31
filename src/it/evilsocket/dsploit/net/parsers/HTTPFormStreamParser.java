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

public class HTTPFormStreamParser extends StreamParser 
{
	private static final String POST_TOKEN      = "POST ";
	private static final String HTTP_TOKEN      = "HTTP/1.";
	private static final String HOST_TOKEN	    = "Host: ";
	
	@SuppressWarnings("unused")
	private Stream  mStream   = null;
	private String  mPage	  = null;
	private String  mHostname = null;
	private boolean mNewLine  = false;
	private String  mData	  = null;
	private boolean mComplete = false;
	
	@Override 
	public void onStreamStart( Stream stream ){
		mStream = stream;
	}
	
	@Override
	public void onStreamAppend( String data ) {			
		if( mPage == null && data.contains( POST_TOKEN ) && data.contains( HTTP_TOKEN ) )
			mPage = data.substring( data.indexOf( POST_TOKEN ) + POST_TOKEN.length(), data.indexOf( HTTP_TOKEN ) );		
		else if( mPage != null && mHostname == null && data.contains( HOST_TOKEN ) )
			mHostname = data.substring( data.indexOf( HOST_TOKEN ) + HOST_TOKEN.length() );
		
		else if( mPage != null && mHostname != null && mNewLine == false && data.trim().isEmpty() )		
			mNewLine = true;		
		
		else if( mNewLine == true && mData == null )		
			mData = data;
		
		else if( mPage != null && mHostname != null && mData != null )
			mComplete = true;
	}
	
	@Override
	public boolean canParseStream( Stream stream ){
		return stream.type == Stream.Type.HTTP;
	}
	
	@Override
	public boolean isComplete(){
		return mComplete;
	}
	
	@Override
	public String getCredentials(){
		return "POST : " + mHostname + mPage + " > '" + mData + "'";
	}
	
	@Override
	public HTTPFormStreamParser clone(){
		return new HTTPFormStreamParser();
	}
}
