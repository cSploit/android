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
package it.evilsocket.dsploit.plugins;

import it.evilsocket.dsploit.net.Target.Vulnerability;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class NVDatabase 
{	
	private final static Pattern ID_PATTERN       = Pattern.compile( ">(CVE-[^<]+)</a>", 					Pattern.MULTILINE | Pattern.DOTALL );
	private final static Pattern SUMMARY_PATTERN  = Pattern.compile( "Summary:</span>\\s*([^<]+)<", 		Pattern.MULTILINE | Pattern.DOTALL );
	private final static Pattern SEVERITY_PATTERN = Pattern.compile( "version=[\\d\\.]+[\"']>([\\d\\.]+)<", Pattern.MULTILINE | Pattern.DOTALL );
	
	public static ArrayList<Vulnerability> search( String query )
	{
		ArrayList<Vulnerability> results = new ArrayList<Vulnerability>();
		URLConnection  connection = null;
		BufferedReader reader	  = null;
		String		   line       = null,
					   body		  = "";
				
		try
		{
			query = "query=" + URLEncoder.encode( query, "UTF-8" ) + "&search_type=all&cves=on";
		}
		catch( UnsupportedEncodingException e )
		{
			query = "query=" + URLEncoder.encode( query ) + "&search_type=all&cves=on";
		}
		
		try
		{
			connection = new URL( "http://web.nvd.nist.gov/view/vuln/search-results?" + query ).openConnection();
			reader	   = new BufferedReader( new InputStreamReader( connection.getInputStream() ) );
			
			while( ( line = reader.readLine() ) != null )
			{
				body += line;
			}
			
			reader.close();

			Matcher 		  matcher	  = null;		
			ArrayList<String> identifiers = new ArrayList<String>(),
							  summaries	  = new ArrayList<String>(),
							  severities  = new ArrayList<String>();
			
			if( ( matcher = ID_PATTERN.matcher(body) ) != null )
			{
				while( matcher.find() )
				{
					identifiers.add( matcher.group(1) );
				}
				
				if( ( matcher = SUMMARY_PATTERN.matcher(body) ) != null )
				{
					while( matcher.find() )
					{
						summaries.add( matcher.group(1) );
					}
					
					if( ( matcher = SEVERITY_PATTERN.matcher(body) ) != null )
					{
						while( matcher.find() )
						{
							severities.add( matcher.group(1) );
						}										
					}
				}
			}		
			
			if( identifiers.size() == summaries.size() && summaries.size() == severities.size() )
			{
				for( int i = 0; i < identifiers.size(); i++ )
				{
					Vulnerability cve = new Vulnerability();
					
					cve.setIdentifier( identifiers.get(i) );
					cve.setSummary( summaries.get(i) );
					cve.setSeverity( Double.parseDouble( severities.get(i) ) );
					
					results.add( cve );
				}
				
				Collections.sort( results, new Comparator<Vulnerability>(){
				    public int compare( Vulnerability o1, Vulnerability o2 ) {
				        if( o1.getSeverity() > o2.getSeverity() )
				        	return -1;
				        
				        else if( o1.getSeverity() < o2.getSeverity() )
				        	return 1;
				        
				        else 
				        	return 0;
				    }
				});	
			}
			else
				results = null;
		}
		catch( MalformedURLException mue )
		{
			mue.printStackTrace();
		}
		catch( IOException ioe )
		{
			ioe.printStackTrace();
		}
		
		return results;
	}
}
