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
package it.evilsocket.dsploit.net;

public class Stream
{
	public enum Type 
	{
		HTTP,
		FTP,
		SMTP,
		IMAP,
		UNKNOWN;
		
		public static Type fromString( String port ) {
			port = port.trim().toLowerCase();
			
			if( port.equals("80") )
				return HTTP;
			
			else if( port.equals("21" ) )
				return FTP;

			else if( port.equals("25") )
				return SMTP;
			
			else if( port.equals("143") )
				return IMAP;
			
			return UNKNOWN;
		}
	}
	
	public String        address = "";
	public Type    	     type    = Type.UNKNOWN;
	public StringBuilder data    = null;
	
	public Stream( String address, Type type ){
		this.address = address;
		this.type	 = type;
		this.data	 = new StringBuilder();
	}

	public String toString(){
		return address + ":" + type;
	}
}