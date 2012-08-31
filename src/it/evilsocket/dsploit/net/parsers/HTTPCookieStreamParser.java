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

public class HTTPCookieStreamParser extends StreamParser 
{
	private static final String IP_HEADER_TOKEN = "E..";
	private static final String HOST_TOKEN	    = "Host: ";
	private static final String COOKIE_TOKEN    = "Cookie: ";
	
	@SuppressWarnings("unused")
	private Stream  mStream   = null;
	private String  mHostname = null;
	private String  mCookie   = "";
	private boolean mComplete = false;
	
	@Override 
	public void onStreamStart( Stream stream ){
		mStream = stream;
	}
	
	@Override
	public void onStreamAppend( String data ) {		
		if( mHostname == null && data.contains( HOST_TOKEN ) )
			mHostname = data.substring( data.indexOf( HOST_TOKEN ) + HOST_TOKEN.length() );
		
		else if( data.contains( COOKIE_TOKEN ) )
			mCookie += data.substring( data.indexOf( COOKIE_TOKEN ) + COOKIE_TOKEN.length() ) + ";";
		
		else if( mHostname != null && mCookie.isEmpty() == false && data.startsWith( IP_HEADER_TOKEN ) )		
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
		return "COOKIES : " + mHostname + " > '" + mCookie + "'";
	}
	
	@Override
	public HTTPCookieStreamParser clone(){
		return new HTTPCookieStreamParser();
	}
}
