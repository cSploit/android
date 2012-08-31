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

import it.evilsocket.dsploit.net.parsers.StreamParser;
import it.evilsocket.dsploit.system.Environment;

import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.util.Log;

public class StreamAssembler 
{
	private static final String  TAG = "StreamAssembler";
	
	private static final Pattern HEAD_PATTERN = Pattern.compile( "^.+\\(tos\\s+[0-9a-fx]+,.+$", Pattern.CASE_INSENSITIVE );
	private static final String  IP			  = "([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})\\.([\\d]+)";
	private static final Pattern TYPE_PATTERN = Pattern.compile( "^.+\\s+" + IP + "\\s+>\\s+" + IP + ":.+$" );
	
	
	public static abstract class NewCredentialHandler
	{
		public abstract void onNewCredentials( String data );
	}
		
	private ArrayList<StreamParser> 	   mAvailableParsers = null;
	private Map<String,List<StreamParser>> mParsers = null;
	private Map<String,Stream>      	   mStreams = null;
	private Stream			       	       mCurrent = null;
	private Matcher			        	   mMatch   = null;
	private NewCredentialHandler    	   mHandler = null;
	
	public StreamAssembler( NewCredentialHandler handler ){
		mStreams 		  = new HashMap<String,Stream>();		
		mAvailableParsers = new ArrayList<StreamParser>();
		mParsers		  = new HashMap<String,List<StreamParser>>();
		mHandler 		  = handler;
	}
	
	public void addStreamParser( StreamParser parser ){
		mAvailableParsers.add(parser);
	}

	public void assemble( String line ){
		line = line.trim();
		
		// packet header
		if( ( mMatch = HEAD_PATTERN.matcher( line ) ) != null && mMatch.find() )
		{
			// get or create the corresponding stream
			if( ( mMatch = TYPE_PATTERN.matcher( line ) ) != null && mMatch.find() )
			{				
				String source   = mMatch.group( 1 ),
					   sport    = mMatch.group( 2 ),
					   dest     = mMatch.group( 3 ),
					   dport    = mMatch.group( 4 ),
					   server   = null,
					   endpoint = null;
				
				Stream.Type type = Stream.Type.UNKNOWN;
				
				try
				{
					if( Environment.getNetwork().isInternal( source ) )
					{
						endpoint = source;
						server   = dest;
						type     = Stream.Type.fromString( dport );			
					}
					else
					{
						endpoint = dest;
						server   = source;
						type     = Stream.Type.fromString( sport );		
					}
				}
				catch( SocketException e )
				{
					Log.e( TAG, e.toString() );
					
					server = source;
					type   = Stream.Type.fromString( sport );		
				}
				
				if( type != Stream.Type.UNKNOWN )					
				{
					String key = server + ":" + type;
					
					// existing stream ?
					if( mStreams.containsKey( key ) )
					{							
						mCurrent = mStreams.get( key );	
					}
					// new stream
					else
					{
						List<StreamParser> streamParsers = new ArrayList<StreamParser>();
						
						mCurrent = new Stream( new Endpoint( endpoint ), server, type );
						mStreams.put( key, mCurrent );
						
						for( StreamParser parser : mAvailableParsers )
						{
							if( parser.canParseStream( mCurrent ) )	
							{
								streamParsers.add( parser.clone() );
							}
						}
						
						for( StreamParser parser : streamParsers )
						{
							parser.onStreamStart( mCurrent );
						}
						
						mParsers.put( key, streamParsers );
					}
				}												
				// unhandled protocol packet header, set current stream object to null to skip following data lines
				else						
					mCurrent = null;						
			}						
		}
		// handled stream data line ?
		else if( mCurrent != null )
		{
			mCurrent.data.append( line + "\n" );
			
			for( StreamParser parser : mParsers.get( mCurrent.toString() ) )
			{
				if( parser.isComplete() == false )		
				{
				    parser.onStreamAppend( line );
				    // last data line the parser needs ?
				    if( parser.isComplete() )
				    {
				    	mHandler.onNewCredentials( parser.getCredentials() );
				    }
				}
			}
		}	
	}
	
	public Collection<Stream> getStreams( ){
		return mStreams.values();
	}
}
