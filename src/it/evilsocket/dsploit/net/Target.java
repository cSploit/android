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
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.net.Network.Protocol;

import java.io.BufferedReader;
import java.net.InetAddress;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.os.StrictMode;
import android.util.Log;

public class Target 
{
	private static final String TAG = "TARGET";
	
	public enum Type {
		NETWORK,
		ENDPOINT,
		REMOTE;
		
		public static Type fromString( String type ) throws Exception {
			if( type != null )
			{
				type = type.trim().toLowerCase();
				if( type.equals("network") )
					return Type.NETWORK;
				
				else if( type.equals("endpoint") )
					return Type.ENDPOINT;
				
				else if( type.equals("remote") )
					return Type.REMOTE;
			}
			
			throw new Exception( "Could not deserialize target type from string." );
		}
	}
	
	public static class Port 
	{
		public Protocol protocol;
		public int		number;
		public String   service;
						
		public Port( int port, Protocol proto, String service ) {
			this.number   = port;
			this.protocol = proto;
			this.service  = service != null ? ( service.equals("null") ? null : service ) : null;
		}
		
		public Port( int port, Protocol proto ) {
			this( port, proto, null );
		}
		
		public String getServiceQuery() {
			String query = "";
			
			if( service != null )
			{
				query = service;
				
				// remove version numbers
				query = query.replaceAll( "[\\d\\.]+", " " );
				// remove everything but letters, digits upper and under scores
				query = query.replaceAll( "[^a-zA-Z0-9\\-_]", " " );
				// remove multiple spaces
				query = query.replaceAll( "[\\s]{2,}", " " );
				// trim
				query = query.trim();
			}
			
			return query;
		}
		
		// needed for vulnerabilities hashmap
		public String toString(){
			return protocol.toString() + "|" + number + "|" + service;
		}
	}
	
	public static class Vulnerability
	{
		private String mIdentifier = null;
		private double mSeverity   = 0;
		private String mSummary	   = null;
		
		public Vulnerability(){
			
		}
		
		public Vulnerability( BufferedReader reader ) throws Exception {
			String serialized = reader.readLine();
			String[] parts	  = serialized.split( "\\|", 3 );
			
			mIdentifier = parts[0];
			mSeverity   = Double.parseDouble( parts[1] );
			mSummary	= parts[2];
		}
		
		public String getIdentifier() {
			return mIdentifier;
		}
		
		public double getSeverity() {
			return mSeverity;
		}
		
		public String getSummary() {
			return mSummary;
		}
		
		public void setIdentifier( String identifier ) {
			this.mIdentifier = identifier;
		}
		
		public void setSeverity( double severity ) {
			this.mSeverity = severity;
		}
		
		public void setSummary( String summary ) {
			this.mSummary = summary;
		}		
		
		public String toString(){
			return mIdentifier + "|" + mSeverity + "|" + mSummary;
		}
						
		public String getHtmlColor( )
		{
			if( mSeverity < 5.0 )
				return "#59FF00";
			
			else if( mSeverity < 7 )
				return "#FFD732";
			
			else
				return "#FF0000";
		}		
	}
	
	private Network 	mNetwork                              			 = null;
	private Endpoint    mEndpoint                             			 = null;
	private int 		mPort 	                              			 = 0;
	private String 		mHostname                             			 = null;
	private Type		mType                                 			 = null;
	private InetAddress mAddress                              			 = null;
	private List<Port>  mPorts	                              			 = new ArrayList<Port>();
	private String		mDeviceType										 = null;
	private String		mDeviceOS										 = null;
	private String		mAlias											 = null;
	private HashMap< String, ArrayList<Vulnerability> > mVulnerabilities = new HashMap< String, ArrayList<Vulnerability> >();
	
	public static Target getFromString( String string ){
		final Pattern PARSE_PATTERN = Pattern.compile( "^(([a-z]+)://)?([0-9a-z\\-\\.]+)(:([\\d]+))?[0-9a-z\\-\\./]*$", Pattern.CASE_INSENSITIVE );
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
						port = System.getPortByProtocol( protocol );
					
					// determine if the 'address' part is an ip address or a host name
					if( IP_PATTERN.matcher( address ).find() )
					{
						// internal ip address
						if( System.getNetwork().isInternal( address ) )
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
			System.errorLogging( TAG, e );
		}
		
		// determine if the target is reachable.
		if( target != null )
		{
			try
			{
				// This is needed to avoid NetworkOnMainThreadException
	        	StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
	        	StrictMode.setThreadPolicy(policy);
	        	
				InetAddress.getByName( target.getCommandLineRepresentation() );
			}
			catch( Exception e )
			{
				target = null;
			}
		}

		return target;
	}

	public Target( BufferedReader reader ) throws Exception {
		mType 		= Type.fromString( reader.readLine() );
		mDeviceType = reader.readLine();
		mDeviceType = mDeviceType.equals("null") ? null : mDeviceType;
		mDeviceOS   = reader.readLine();
		mDeviceOS   = mDeviceOS.equals("null") ? null : mDeviceOS;
		mAlias      = reader.readLine();
		mAlias      = mAlias.equals("null") ? null : mAlias;
		
		if( mType == Type.NETWORK )
		{
			return;
		}
		else if( mType == Type.ENDPOINT )
		{
			mEndpoint = new Endpoint( reader );
		}
		else if( mType == Type.REMOTE )
		{
			mHostname = reader.readLine();
			mHostname = mHostname.equals("null") ? null : mHostname;
			if( mHostname != null )		
			{
				// This is needed to avoid NetworkOnMainThreadException
	        	StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
	        	StrictMode.setThreadPolicy(policy);
	        	
				mAddress = InetAddress.getByName( mHostname );
			}
		}
		
		int ports = Integer.parseInt( reader.readLine() );
		for( int i = 0; i < ports; i++ )
		{
			String   key   = reader.readLine();
			String[] parts = key.split( "\\|", 3 );			
			Port	 port  = new Port
			(
			  Integer.parseInt( parts[1] ), 
			  Protocol.fromString( parts[0] ), 
			  parts[2]
			);
			
			mPorts.add( port );
			mVulnerabilities.put( key, new ArrayList< Vulnerability >() );
			
			int nvulns = Integer.parseInt( reader.readLine() );
			for( int j = 0; j < nvulns; j++ )
			{
				Vulnerability v = new Vulnerability( reader );
				
				mVulnerabilities.get(key).add(v);
			}
		}
	}
	
	public void serialize( StringBuilder builder ) {
		builder.append( mType + "\n" );
		builder.append( mDeviceType + "\n" );
		builder.append( mDeviceOS + "\n" );
		builder.append( mAlias + "\n" );
		
		// a network can't be saved in a session file
		if( mType == Type.NETWORK )
		{
			return;
		}
		else if( mType == Type.ENDPOINT )
		{
			mEndpoint.serialize(builder);
		}
		else if( mType == Type.REMOTE )
		{
			builder.append( mHostname + "\n" );
		}
		
		builder.append( mPorts.size() + "\n" );
		for( Port port : mPorts )
		{
			String key = port.toString();
			builder.append( key + "\n" );
			if( mVulnerabilities.containsKey( key ) )
			{
				builder.append( mVulnerabilities.get( key ).size() + "\n" );
				for( Vulnerability v : mVulnerabilities.get( key ) )
				{
					builder.append( v.toString() + "\n" );
				}
			}
			else
				builder.append( "0\n" );
		}				
	}
	
	public void setAlias( String alias ) {
		mAlias = alias.trim();
	}
	
	public String getAlias( ) {
		return mAlias;
	}
	
	public boolean hasAlias( ) {
		return mAlias != null && mAlias.isEmpty() == false;
	}
	
	public boolean comesAfter( Target target ){

		if( mType == Type.NETWORK )
			return false;
		
		else if( mType == Type.ENDPOINT )
			if( target.getType() == Type.ENDPOINT )					
				return mEndpoint.getAddressAsLong() > target.getEndpoint().getAddressAsLong();					
			else
				return false;
		
		else 
			return true; 
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
	
	public InetAddress getAddress() {
		if( mType == Type.ENDPOINT )
			return mEndpoint.getAddress();
		
		else if( mType == Type.REMOTE )
			return mAddress;
		
		else
			return null;
	}
	
	public boolean equals( Target target ) {
		if( mType == target.getType() )
		{
			if( mType == Type.NETWORK )
				return mNetwork.equals( target.getNetwork() );
			
			else if( mType == Type.ENDPOINT )
				return mEndpoint.equals( target.getEndpoint() ); 
			
			else if( mType == Type.REMOTE )
				return mHostname.equals( target.getHostname() ); 
		}
		
		
		return false;
	}
	
	public String getDisplayAddress() {
		if( mType == Type.NETWORK )
			return mNetwork.getNetworkRepresentation();
		
		else if( mType == Type.ENDPOINT )
			return mEndpoint.getAddress().getHostAddress() + ( mPort == 0 ? "" : ":" + mPort );
		
		else if( mType == Type.REMOTE )
			return mHostname + ( mPort == 0 ? "" : ":" + mPort );
		
		else
			return "???";
	}

	public String toString( ){
		if( hasAlias() == true )
			return mAlias;
		
		else 
			return getDisplayAddress();
	}
	
	public String getDescription(){
		if( mType == Type.NETWORK )
			return "This is your network subnet mask";
		
		else if( mType == Type.ENDPOINT )
		{
			String vendor = System.getMacVendor( mEndpoint.getHardware() ),
				   desc   = mEndpoint.getHardwareAsString();
			
			if( vendor != null )
				desc += " - " + vendor;
			
			try
			{
				if( mEndpoint.getAddress().equals( System.getNetwork().getGatewayAddress() ) )
					desc += " ( Your network gateway / router )";
			
				else if( mEndpoint.getAddress().equals( System.getNetwork().getLocalAddress() ) )
					desc += " ( This device )";
			}
			catch( SocketException e )
			{
				System.errorLogging( TAG, e );
			}
			
			return desc.trim();
		}		
		else if( mType == Type.REMOTE )
			return mAddress.getHostAddress();
		
		return "";
	}
	
	public String getCommandLineRepresentation()
	{
		if( mType == Type.NETWORK )
			return mNetwork.getNetworkRepresentation();
		
		else if( mType == Type.ENDPOINT )
			return mEndpoint.getAddress().getHostAddress();
		
		else if( mType == Type.REMOTE )
			return mHostname;
		
		else
			return "???";
	}
	
	public boolean isRouter() {
		try
		{
			return ( mType == Type.ENDPOINT && mEndpoint.getAddress().equals( System.getNetwork().getGatewayAddress() ) );
		}
		catch( Exception e )
		{
			System.errorLogging( TAG, e );
		}
		
		return false;
	}
	
	public int getDrawableResourceId( )
	{
		try
		{
			if( mType == Type.NETWORK )
				return R.drawable.target_network_48;
			
			else if( mType == Type.ENDPOINT )
				if( isRouter() )
					return R.drawable.target_router_48;
			
				else if( mEndpoint.getAddress().equals( System.getNetwork().getLocalAddress() ) )
					return R.drawable.target_self_48;
			
				else
					return R.drawable.target_endpoint_48;
			
			else if( mType == Type.REMOTE )
				return R.drawable.target_remote_48;
		}
		catch( Exception e )
		{
			System.errorLogging( TAG, e );
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
		
		try
		{
			// This is needed to avoid NetworkOnMainThreadException
        	StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
        	StrictMode.setThreadPolicy(policy);
        	
			mAddress = InetAddress.getByName( mHostname );
		}
		catch( Exception e )
		{
			Log.d( "Target.setHostname()", e.toString() );
		}
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
		for( int i = 0; i < mPorts.size(); i++ )
		{
			if( mPorts.get(i).number == port.number )
			{
				if( port.service != null )
					mPorts.get(i).service = port.service;
				
				return;
			}
		}

		mPorts.add( port );
	}
	
	public void addOpenPort( int port, Protocol protocol ) {
		addOpenPort( new Port( port, protocol ) );
	}
	
	public void addOpenPort( int port, Protocol protocol, String service ) {
		addOpenPort( new Port( port, protocol, service ) );
	}
		
	public List<Port> getOpenPorts() {
		return mPorts;
	}
	
	public boolean hasOpenPorts() {
		return !mPorts.isEmpty();
	}
	
	public boolean hasOpenPortsWithService() {
		if( mPorts.isEmpty() == false )
		{
			for( Port p : mPorts ) {
				if( p.service != null && p.service.isEmpty() == false )
					return true;
			}
		}
		
		return false;
	}
	
	public boolean hasOpenPort( int port ) {
		for( Port p : mPorts ) {
			if( p.number == port )
				return true;
		}
		
		return false;
	}
	
	public void setDeviceType( String type ) {
		mDeviceType = type;
	}
	
	public String getDeviceType() {
		return mDeviceType;
	}
	
	public void setDeviceOS( String os ) {
		mDeviceOS = os;
	}
	
	public String getDeviceOS() {
		return mDeviceOS;
	}
	
	public void addVulnerability( Port port, Vulnerability v ) {		
		if( mVulnerabilities.containsKey( port.toString() ) == false )	
		{
			mVulnerabilities.put( port.toString(), new ArrayList< Vulnerability >() );
		}
		else
		{
			for( Vulnerability vuln : mVulnerabilities.get( port.toString() ) )
				if( vuln.getIdentifier().equals( v.getIdentifier() ) )
					return;
		}
		
		mVulnerabilities.get( port.toString() ).add( v );
	}
	
	public HashMap< String, ArrayList< Vulnerability > > getVulnerabilities() {
		return mVulnerabilities;
	}
}
