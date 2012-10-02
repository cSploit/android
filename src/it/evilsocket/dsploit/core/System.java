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
package it.evilsocket.dsploit.core;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.NoRouteToHostException;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;
import android.util.Log;
import it.evilsocket.dsploit.net.Endpoint;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Network.Protocol;
import it.evilsocket.dsploit.net.Target.Port;
import it.evilsocket.dsploit.net.Target.Type;
import it.evilsocket.dsploit.net.Target.Vulnerability;
import it.evilsocket.dsploit.net.http.proxy.Proxy;
import it.evilsocket.dsploit.net.http.server.Server;
import it.evilsocket.dsploit.tools.Ettercap;
import it.evilsocket.dsploit.tools.Hydra;
import it.evilsocket.dsploit.tools.IPTables;
import it.evilsocket.dsploit.tools.NMap;
import it.evilsocket.dsploit.tools.TcpDump;

public class System 
{		
	private static final String  TAG 		  		    = "SYSTEM";
	private static final String  SESSION_MAGIC		    = "DSS"; 
	private static final Pattern SERVICE_PARSER		    = Pattern.compile( "^([^\\s]+)\\s+(\\d+).*$", Pattern.CASE_INSENSITIVE );
	public  static final int	 HTTP_PROXY_PORT		= 8080;
	public  static final int	 HTTP_SERVER_PORT		= 8081;
	public  static final String  IPV4_FORWARD_FILEPATH  = "/proc/sys/net/ipv4/ip_forward";	
 
	private static boolean			     mInitialized   = false;
	private static UpdateManager		 mUpdateManager = null;
	private static Context 			     mContext  	    = null;
	private static Network 			     mNetwork  	    = null;
	private static ArrayList<Target>     mTargets		= null;
	private static int  			     mCurrentTarget = 0;
	private static Map<String,String>    mServices 	    = null;
	private static Map<String,String>    mPorts         = null;
	private static Map< String, String > mVendors       = null;
	
	// registered plugins
	private static ArrayList<Plugin>     mPlugins  	    = null;
	private static Plugin			     mCurrentPlugin = null;	
	// tools singleton
	private static NMap					 mNmap			= null;
	private static Ettercap				 mEttercap		= null;
	private static IPTables				 mIptables		= null;
	private static Hydra				 mHydra			= null;
	private static TcpDump				 mTcpdump		= null;
	
	private static Proxy				 mProxy			= null;
	private static Server				 mServer		= null;
	
	private static String				 mStoragePath   = null;
	private static String				 mSessionName	= null;
		
	public static void init( Context context ) throws Exception {
		mContext = context;		
		try
		{				
			mUpdateManager = new UpdateManager( mContext );
			mPlugins 	   = new ArrayList<Plugin>();
			mTargets 	   = new ArrayList<Target>();
			
			// local network
			mTargets.add( new Target( System.getNetwork() ) );
			// network gateway
			mTargets.add( new Target( System.getNetwork().getGatewayAddress(), System.getNetwork().getGatewayHardware() ) );
			// device network address
			mTargets.add( new Target( System.getNetwork().getLoacalAddress(), System.getNetwork().getLocalHardware() ) );
			
			mNmap     = new NMap( mContext );
			mEttercap = new Ettercap( mContext );
			mIptables = new IPTables( );
			mHydra    = new Hydra( mContext );
			mTcpdump  = new TcpDump( mContext );
			
			mStoragePath = Environment.getExternalStorageDirectory().toString();
			mSessionName = "dsploit-session-" + java.lang.System.currentTimeMillis();
			
			mInitialized = true;
		}
		catch( Exception e )
		{
			Log.e( TAG, e.toString() );
			
			throw e;
		}
	}
	
	private static void preloadServices( ) {
		if( mServices == null || mPorts == null )
		{
			try
			{
				// preload network service map and mac vendors 
				mServices = new HashMap<String,String>();
				mPorts	  = new HashMap<String,String>();
				
				FileInputStream fstream = new FileInputStream( mContext.getFilesDir().getAbsolutePath() + "/tools/nmap/nmap-services" );
				DataInputStream in 	    = new DataInputStream(fstream);
				BufferedReader  reader  = new BufferedReader(new InputStreamReader(in));
				String 		    line;
				Matcher			matcher;
					
				while( ( line = reader.readLine() ) != null )   
				{
					line = line.trim();
					  
					if( ( matcher = SERVICE_PARSER.matcher(line) ) != null && matcher.find() )
					{
						String proto = matcher.group( 1 ),
							   port  = matcher.group( 2 );
						
						mServices.put( proto, port );
						mPorts.put( port, proto );
					}
				}
				  
				in.close();
			}
			catch( Exception e )
			{
				Log.e( TAG, e.toString() );
			}
		}
	}
	
	private static void preloadVendors( ) {
		if( mVendors == null )
		{
			try
			{
				mVendors				 = new HashMap<String,String>();			
				FileInputStream fstream  = new FileInputStream( mContext.getFilesDir().getAbsolutePath() + "/tools/nmap/nmap-mac-prefixes" );
				DataInputStream in 	     = new DataInputStream(fstream);
				BufferedReader  reader   = new BufferedReader(new InputStreamReader(in));
				String 		    line;

				while( ( line = reader.readLine() ) != null )   
				{
					line = line.trim();
					if( line.startsWith("#") == false && line.isEmpty() == false )  
					{
						String[] tokens = line.split( " ", 2 );
						
						if( tokens.length == 2 )
							mVendors.put( tokens[0], tokens[1] );						
					}
				}
				
				in.close();
			}
			catch( Exception e )
			{
				Log.e( TAG, e.toString() );
			}
		}
	}
	
	public static UpdateManager getUpdateManager() {
		return mUpdateManager;
	}

	public static String getSessionName() {
		return mSessionName;
	}
	
	public static String getStoragePath() {
		return mStoragePath;
	}
	
	public static String getAppVersionName() {
		try
		{
			PackageManager manager = mContext.getPackageManager();
			PackageInfo    info    = manager.getPackageInfo( mContext.getPackageName(), 0 );
		
			return info.versionName;
		}
		catch( NameNotFoundException e )
		{
			Log.e( TAG, e.toString() );
		}
		
		return "?";
	}
	
	public static ArrayList<String> getAvailableSessionFiles( )
	{
		ArrayList<String> files = new ArrayList<String>();		
		File storage            = new File( mStoragePath );

		String[] children = storage.list();
		
		for( String child : children )
		{
			if( child.endsWith(".dss") )
				files.add( child );
		}

		return files;
	}
	
	public static String saveSession( String sessionName ) throws IOException {
		StringBuilder builder  = new StringBuilder();
		String		  filename = mStoragePath + '/' + sessionName + ".dss",
					  session  = null;
		
		builder.append( SESSION_MAGIC + "\n" );
		
		// skip the network target
		builder.append( ( mTargets.size() - 1 ) + "\n" );
		for( Target target : mTargets )
		{
			if( target.getType() != Target.Type.NETWORK )
				target.serialize(builder);
		}
		builder.append( mCurrentTarget + "\n" );
		
		session = builder.toString();

		FileOutputStream ostream = new FileOutputStream( filename );
	    GZIPOutputStream gzip    = new GZIPOutputStream( ostream );

	    gzip.write( session.getBytes() );
	    
	    gzip.close();
	    
		mSessionName = sessionName;
		
		return filename;
	}
	
	public static void loadSession( String filename ) throws Exception
	{
		File file = new File( mStoragePath + '/' + filename );
		
		if( file.exists() && file.length() > 0 )
		{
			BufferedReader reader = new BufferedReader(new InputStreamReader( new GZIPInputStream( new FileInputStream( file ) ) ) );
			String		   line   = null;
			
			
			// begin decoding procedure
			try
			{
				line = reader.readLine();
				if( line == null || line.equals( SESSION_MAGIC ) == false )
					throw new Exception( "Not a dSploit session file." );
												
				reset();
				
				// read targets 
				int targets = Integer.parseInt( reader.readLine() );
				for( int i = 0; i < targets; i++ )
				{
					Target target = new Target( reader );
					
					if( hasTarget(target) == false )
					{
						System.addOrderedTarget(target);
					}
					else
					{
						for( int j = 0; j < mTargets.size(); j++ )
						{
							if( mTargets.get( j ) != null && mTargets.get( j ).equals( target ) )
							{
								mTargets.set( j, target );
								break;
							}
						}
					}
				}
				
				mCurrentTarget = Integer.parseInt( reader.readLine() );
				
				reader.close();
			}
			catch( Exception e )
			{
				reset();
				
				if( reader != null )
					reader.close();
				
				throw e;
			}
		}
		else
			throw new Exception( filename + " does not exists or is empty." );
	}
	
	public static NMap getNMap() {
		return mNmap;
	}
	
	public static Ettercap getEttercap() {
		return mEttercap;
	}
	
	public static IPTables getIPTables() {
		return mIptables;
	}
	
	public static Hydra getHydra() {
		return mHydra;
	}
	
	public static TcpDump getTcpDump() {
		return mTcpdump;
	}
	
	public static Proxy getProxy() {
		try
		{
			if( mProxy == null )
				mProxy = new Proxy( getNetwork().getLoacalAddress(), HTTP_PROXY_PORT );
		}
		catch( Exception e )
		{
			Log.e( TAG, e.toString() );
		}
		
		return mProxy;
	}
	
	public static Server getServer() {		
		try
		{
			if( mServer == null )			
				mServer = new Server( getNetwork().getLoacalAddress(), HTTP_SERVER_PORT );					
		}
		catch( Exception e )
		{
			Log.e( TAG, e.toString() );
		}
		
		return mServer;
	}
	
	public static String getImageMimeType( String fileName ) {
		String type = "image/jpeg",
			   name = fileName.toLowerCase();
		
		if( name.endsWith( ".jpeg") || name.endsWith( ".jpg" ) )
			type = "image/jpeg";
		
		else if( name.endsWith( ".png" ) )
			type = "image/png";
		
		else if( name.endsWith( ".gif" ) )
			type = "image/gif";
		
		else if( name.endsWith( ".tiff" ) )
			type = "image/tiff";

		return type;
	}

	public static void reset() throws NoRouteToHostException, SocketException {
		mTargets.clear();
		
		// local network
		mTargets.add( new Target( System.getNetwork() ) );
		// network gateway
		mTargets.add( new Target( System.getNetwork().getGatewayAddress(), System.getNetwork().getGatewayHardware() ) );
		// device network address
		mTargets.add( new Target( System.getNetwork().getLoacalAddress(), System.getNetwork().getLocalHardware() ) );
				
		mCurrentTarget = 0;
	}
	
	public static boolean isInitialized(){
		return mInitialized;
	}
	
	public static String getMacVendor( byte[] mac ){	
		preloadVendors();
		
		if( mac != null && mac.length >= 3 )
			return mVendors.get( String.format( "%02X%02X%02X", mac[0], mac[1], mac[2] ) );
		else
			return null;
	}
	
	public static String getProtocolByPort( String port ){
		preloadServices();
		
		return mPorts.containsKey(port) ? mPorts.get(port) : null;
	}
	
	public static int getPortByProtocol( String protocol ){
		preloadServices();
		
		return mServices.containsKey(protocol) ? Integer.parseInt( mServices.get(protocol) ) : 0;
	}
		
	public static Context getContext(){
		return mContext;
	}
	
	public static Network getNetwork() throws NoRouteToHostException, SocketException {
		if( mNetwork == null )
			mNetwork = new Network( mContext );
		
		return mNetwork;
	}
	
	public static ArrayList<Target> getTargets() {
		return mTargets;
	}
	
	public static ArrayList<Target> getTargetsByType( Target.Type type ) {
		ArrayList<Target> filtered = new ArrayList<Target>();
		
		for( Target target : mTargets )
		{
			if( target.getType() == type )
				filtered.add( target );
		}
		
		return filtered;
	}
	
	public static ArrayList<Endpoint> getNetworkEndpoints() {
		ArrayList<Endpoint> filtered = new ArrayList<Endpoint>();
		
		for( Target target : mTargets )
		{
			if( target.getType() == Type.ENDPOINT )
				filtered.add( target.getEndpoint() );
		}
		
		return filtered;
	}
	
	public static void addTarget( int index, Target target ){
		mTargets.add( index, target );
		// update current target index
		if( mCurrentTarget >= index )
			mCurrentTarget++;
	}
	
	public static void addTarget( Target target ){
		mTargets.add( target );
	}
	
	public static boolean addOrderedTarget( Target target ){
		if( target != null && hasTarget( target ) == false )
		{
			for( int i = 0; i < getTargets().size(); i++ )
			{
				if( getTarget( i ).comesAfter( target ) )
				{
					addTarget( i, target );
					return true;
				}
			}
			
			addTarget( target );
			
			return true;
		}
		
		return false;
	}
	
	public static Target getTarget( int index ){
		return mTargets.get( index );
	}
	
	public static boolean hasTarget( Target target ) {
		for( Target t : mTargets )
		{
			if( t != null && t.equals(target) )
				return true;
		}
		
		return false;
	}

	public static void setCurrentTarget( int index ){
		mCurrentTarget = index;
	}
	
	public static Target getCurrentTarget(){
		return getTarget( mCurrentTarget );
	}
	
	public static void registerPlugin( Plugin plugin ){
		mPlugins.add(plugin);
	}
	
	public static ArrayList<Plugin> getPlugins(){
		return mPlugins;
	}
	
	public static ArrayList<Plugin> getPluginsForTarget( Target target ){
		ArrayList<Plugin> filtered = new ArrayList<Plugin>();
		
		if( target != null )
		{
			for( Plugin plugin : mPlugins )
				if( plugin.isAllowedTarget(target) )
					filtered.add(plugin);
		}
				
		return filtered;
	}
	
	public static ArrayList<Plugin> getPluginsForTarget(){
		return getPluginsForTarget( getCurrentTarget() );
	} 
	
	public static void setCurrentPlugin( Plugin plugin ){
		Log.d( TAG, "Setting current plugin : " + plugin.getName() );
		
		mCurrentPlugin = plugin;
	}
	
	public static Plugin getCurrentPlugin( ){
		return mCurrentPlugin;
	}
	
	public static void addOpenPort( int port, Protocol protocol ) {
		addOpenPort( port, protocol, null );
	}
	
	public static void addOpenPort( int port, Protocol protocol, String service ) {
		Port p = new Port( port, protocol, service );
		
		getCurrentTarget().addOpenPort( p );
		
		for( Plugin plugin : getPluginsForTarget() ) {
			plugin.onTargetNewOpenPort( getCurrentTarget(), p );
		}
	}
	
	public static void addVulnerability( Port port, Vulnerability v ) {
		getCurrentTarget().addVulnerability( port, v );
		
		for( Plugin plugin : getPluginsForTarget() ) {
			plugin.onTargetNewVulnerability( getCurrentTarget(), port, v );
		}
	}

	public static String getGatewayAddress() {
		return mNetwork.getGatewayAddress().getHostAddress();
	}
		
	public static boolean isForwardingEnabled( ) {
		boolean 	   forwarding = false;
		BufferedReader reader;
		String		   line;
		
		try 
		{
			reader 	   = new BufferedReader( new FileReader( IPV4_FORWARD_FILEPATH ) );
			line   	   = reader.readLine().trim();			
			forwarding = line.equals("1");

			reader.close();
			
		} 
		catch( IOException e ) 
		{
			Log.w( TAG, e.toString() );
		}
		
		return forwarding;
	}
	
	public static void setForwarding( boolean enabled )
	{
		Log.d( TAG, "Setting ipv4 forwarding to " + enabled );
		
		String status = ( enabled ? "1" : "0" ),
			   cmd    = "echo " + status + " > " + IPV4_FORWARD_FILEPATH;
		    	
		try
		{
			Shell.exec(cmd);
		}
		catch( Exception e )
		{
			Log.e( TAG, e.toString() );
		}
	}
		
	public static void clean() {
		setForwarding( false );

		try
		{
			String tools = "";
			for( String tool : ToolsInstaller.TOOLS )
				tools += tool + " ";
			
			Log.d( TAG, "Killing any running instance of " + tools );
			Shell.exec( "killall -9 " + tools.trim() );							
		}
		catch( Exception e )
		{ 
			Log.e( TAG, e.toString() );
		}
	}
	
}
