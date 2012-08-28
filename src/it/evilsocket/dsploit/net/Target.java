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

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.net.Network.Protocol;
import it.evilsocket.dsploit.system.Environment;

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.util.Log;

public class Target {
	
	public enum Type {
		NETWORK,
		ENDPOINT,
		REMOTE
	}
	
	public static class Port 
	{
		public Protocol protocol;
		public int		port;
						
		public Port( int port, Protocol proto ) {
			this.port     = port;
			this.protocol = proto;
		}
	}
	
	private Network 	mNetwork  = null;
	private Endpoint    mEndpoint = null;
	private int 		mPort 	  = 0;
	private String 		mHostname = null;
	private Type		mType     = null;
	private List<Port>  mPorts	  = new ArrayList<Port>();
	
	public static Target getFromString( String string ){
		final Pattern PARSE_PATTERN = Pattern.compile( "^(([a-z]+)://)?([0-9a-z\\-\\.]+)(:([\\d]+))?.*$", Pattern.CASE_INSENSITIVE );
		final Pattern IP_PATTERN    = Pattern.compile( "^[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}$" );
		
		Matcher matcher = null;
		Target  target  = null;
		
		try
		{							
			string = string.trim();
			
			if( ( matcher = PARSE_PATTERN.matcher( string ) ) != null && matcher.find() )
			{
				String protocol = matcher.group( 2 ),
					   address  = matcher.group( 3 ),
					   sport    = matcher.group( 4 );
				
				protocol = protocol != null ? protocol.toLowerCase() : null;
				sport    = sport    != null ? sport.substring(1) : null;
						
				if( address != null )
				{
					// attempt to get the port from the protocol or the specified one
					int port = 0;
					
					if( sport != null )
						port = Integer.parseInt(sport);
					
					else if( protocol != null )
						port = Environment.getPortByProtocol( protocol );
					
					// determine if the 'address' part is an ip address or a host name
					if( IP_PATTERN.matcher( address ).find() )
					{
						// internal ip address
						if( Environment.getNetwork().isInternal( address ) )
						{
							target = new Target( new Endpoint( address, null ) );
							target.setPort( port );							
						}
						// external ip address, return as host name
						else
						{
							target = new Target( address, port );
						}
					}
					// found a host name
					else
						target = new Target( address, port );
				}
			}
		}
		catch( Exception e )
		{
			Log.e( "Target", e.toString() );
		}
		
		// determine if the target is reachable.
		/*
		if( target != null )
		{
			try
			{
				InetAddress.getByName( target.getCommandLineRepresentation() );
			}
			catch( Exception e )
			{
				target = null;
			}
		}
		*/
		return target;
	}
		
	public Target( Network net ){
		setNetwork( net );
	}
	
	public Target( InetAddress address, byte[] hardware ){
		setEndpoint( address, hardware );
	}
	
	public Target( Endpoint endpoint ){
		setEndpoint( endpoint );
	}
	
	public Target( String hostname, int port ){
		setHostname( hostname, port );
	}
	
	public boolean equals( Target target ) 
	{
		if( mType == target.getType() )
		{
			if( mType == Type.NETWORK )
				return mNetwork.equals( target.getNetwork() );
			
			else if( mType == Type.ENDPOINT )
				return mEndpoint.equals( target.getEndpoint() ) && mPort == target.getPort(); 
			
			else if( mType == Type.REMOTE )
				return mHostname.equals( target.getHostname() ) && mPort == target.getPort(); 
		}
		
		
		return false;
	}

	public String toString( ){
		if( mType == Type.NETWORK )
			return mNetwork.getNetworkRepresentation() + " ( Your Network )";
		
		else if( mType == Type.ENDPOINT )
			return mEndpoint.getAddress().toString().split("/")[1] + ( mPort == 0 ? "" : ":" + mPort );
		
		else if( mType == Type.REMOTE )
			return mHostname + ( mPort == 0 ? "" : ":" + mPort );
		
		else
			return "???";
	}
	
	public String getCommandLineRepresentation()
	{
		if( mType == Type.NETWORK )
			return mNetwork.getNetworkRepresentation();
		
		else if( mType == Type.ENDPOINT )
			return mEndpoint.getAddress().toString().split("/")[1];
		
		else if( mType == Type.REMOTE )
			return mHostname;
		
		else
			return "???";
	}
	
	public int getDrawableResourceId( )
	{
		try
		{
			if( mType == Type.NETWORK )
				return R.drawable.target_network_48;
			
			else if( mType == Type.ENDPOINT )
				if( mEndpoint.getAddress().equals(  Environment.getNetwork().getGatewayAddress() ) )
					return R.drawable.target_router_48;
			
				else if( mEndpoint.getAddress().equals( Environment.getNetwork().getLoacalAddress() ) )
					return R.drawable.target_self_48;
			
				else
					return R.drawable.target_endpoint_48;
			
			else if( mType == Type.REMOTE )
				return R.drawable.target_remote_48;
		}
		catch( Exception e )
		{
			Log.e( "Target", e.toString() );
		}
		
		return R.drawable.target_network_48;
	}
	
	public void setNetwork( Network net ){
		mNetwork = net;
		mType	 = Type.NETWORK;
	}
	
	public void setEndpoint( Endpoint endpoint ){
		mEndpoint = endpoint;
		mType     = Type.ENDPOINT;
	}
	
	public void setEndpoint( InetAddress address, byte[] hardware ){
		mEndpoint = new Endpoint( address, hardware );
		mType     = Type.ENDPOINT;
	}

	public void setHostname( String hostname, int port ){
		mHostname = hostname;
		mPort	  = port;
		mType	  = Type.REMOTE;
	}
	
	public Type getType(){
		return mType;
	}
	
	public Network getNetwork( ){
		return mNetwork;
	}
	
	public Endpoint getEndpoint( ){
		return mEndpoint;
	}
	
	public void setPort( int port ){
		mPort = port;
	}
	
	public int getPort( ){
		return mPort;
	}
	
	public String getHostname( ){
		return mHostname;
	}
	
	public void addOpenPort( Port port ) {
		mPorts.add( port );
	}
	
	public void addOpenPort( int port, Protocol protocol ) {
		mPorts.add( new Port( port, protocol ) );
	}
		
	public List<Port> getOpenPorts() {
		return mPorts;
	}
	
	public boolean hasOpenPorts() {
		return !mPorts.isEmpty();
	}
	
	public boolean hasOpenPort( int port ) {
		for( Port p : mPorts ) {
			if( p.port == port )
				return true;
		}
		
		return false;
	}
}
