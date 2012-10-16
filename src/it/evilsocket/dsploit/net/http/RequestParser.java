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
package it.evilsocket.dsploit.net.http;

import java.sql.Date;
import java.util.ArrayList;

import org.apache.http.impl.cookie.BasicClientCookie;

public class RequestParser 
{
	public static String getBaseDomain( String host ) {
	    int startIndex = 0;
	    int nextIndex = host.indexOf('.');
	    int lastIndex = host.lastIndexOf('.');
	    while (nextIndex < lastIndex) {
	        startIndex = nextIndex + 1;
	        nextIndex = host.indexOf('.', startIndex);
	    }
	    if (startIndex > 0) {
	        return host.substring(startIndex);
	    } else {
	        return host;
	    }
	}
	
	private static ArrayList<BasicClientCookie> parseRawCookie( String rawCookie ) {
	    String[] rawCookieParams 	   		 = rawCookie.split(";");
	    ArrayList<BasicClientCookie> cookies = new ArrayList<BasicClientCookie>();
	    
	    for( String rawCookieParam : rawCookieParams )
	    {
		    String[] rawCookieNameAndValue = rawCookieParam.split("=");
		    
		    if( rawCookieNameAndValue.length != 2 ) 
		        continue;
		    
		    String 			  cookieName  = rawCookieNameAndValue[0].trim();
		    String			  cookieValue = rawCookieNameAndValue[1].trim();
		    BasicClientCookie cookie 	  = new BasicClientCookie( cookieName, cookieValue );
		    
		    for( int i = 1; i < rawCookieParams.length; i++ ) 
		    {
		        String rawCookieParamNameAndValue[] = rawCookieParams[i].trim().split("=");
		        String paramName 					= rawCookieParamNameAndValue[0].trim();
	
		        if( paramName.equalsIgnoreCase("secure") ) 
		            cookie.setSecure(true);
		        
		        else 
		        {
		        	// attribute not a flag or missing value.
		            if( rawCookieParamNameAndValue.length == 2 )		           
		            {
			            String paramValue = rawCookieParamNameAndValue[1].trim();
					            	               	             
			            if( paramName.equalsIgnoreCase("max-age") ) 
			            {
			                long maxAge = Long.parseLong(paramValue);
			                Date expiryDate = new Date( java.lang.System.currentTimeMillis() + maxAge );
			                cookie.setExpiryDate(expiryDate);
			            } 
			            else if( paramName.equalsIgnoreCase("domain") )	            
			                cookie.setDomain(paramValue);
			            
			            else if( paramName.equalsIgnoreCase("path") )
			                cookie.setPath(paramValue);
			            
			            else if( paramName.equalsIgnoreCase("comment") ) 
			                cookie.setComment( paramValue );     
		            }
		        }
		    }

		    cookies.add( cookie );
	    }

	    return cookies;
	}
	
	public static String getHeaderValue( String name, ArrayList<String> headers ) {
		for( String header : headers )
		{
			if( header.indexOf(':') != -1 )
			{
				String[] split  = header.split( ":", 2 );
        		String   hname  = split[0].trim(),
        				 hvalue = split[1].trim();
        		
        		if( hname.equals( name ) )
        			return hvalue;
			}
		}
				
		return null;
	}
	
	public static ArrayList<BasicClientCookie> getCookiesFromHeaders( ArrayList<String> headers ){
		String cookie = getHeaderValue( "Cookie", headers );
		
		if( cookie != null )
			return parseRawCookie( cookie );
		else
			return null;
	}
}
