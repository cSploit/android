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

import android.util.Base64;
import it.evilsocket.dsploit.net.Stream;

public class SMTPStreamParser extends StreamParser 
{
	private static final String AUTH_TOKEN = "AUTH PLAIN";
	private static final String SEPARATOR  = "" + (char)0x00;
	
	private Stream  mStream   = null;
	private String  mUsername = null;
	private String  mPassword = null;
	private boolean mComplete = false;
	
	@Override
	public void onStreamStart(Stream stream) {
		mStream = stream;
	}

	@Override
	public void onStreamAppend( String data ) {
		String upper = data.toUpperCase();
		
		if( mUsername == null && mPassword == null && upper.contains( AUTH_TOKEN ) )
		{
			String[] parts = data.split(" ");
			if( parts != null && parts.length >= 3 )
			{
				byte[] decoded = Base64.decode( parts[2], Base64.DEFAULT );
				
				if( decoded != null )
				{
					String[] auth = new String( decoded ).split( SEPARATOR );
					if( auth != null && auth.length >= 3 )
					{
						mUsername = auth[1];
						mPassword = auth[2];
					}
				}
			}
		}
		else if( mUsername != null && mPassword != null )
			mComplete = true;	
	}

	@Override
	public boolean canParseStream(Stream stream) {
		return stream.type == Stream.Type.SMTP;
	}

	@Override
	public boolean isComplete(){
		return mComplete;
	}
	
	@Override
	public Stream getStream(){
		return mStream;
	}
	
	@Override
	public String getData(){
		return mStream + " > USERNAME='" + mUsername + "' PASSWORD='" + mPassword + "'";
	}
	
	@Override
	public SMTPStreamParser clone(){
		return new SMTPStreamParser();
	}
}
