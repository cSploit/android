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
package it.evilsocket.dsploit.system;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.FileInputStream;
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

import android.content.Context;
import android.util.Log;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Network.Protocol;
import it.evilsocket.dsploit.net.Target.Port;

public class Environment 
{	
	private static final String TAG 		  		  = "Environment";
	public  static final String IPV4_FORWARD_FILEPATH = "/proc/sys/net/ipv4/ip_forward";

	private static Context 			  mContext  	 = null;
	private static Network 			  mNetwork  	 = null;
	private static Target  			  mTarget   	 = null;
	private static Map<String,String> mServices 	 = null;
	private static Map<String,String> mPorts     	 = null;
	
	private static ArrayList<Plugin>  mPlugins  	 = null;
	private static Plugin			  mCurrentPlugin = null;
		
	public static void init( Context context ){
		mContext = context;		
		mPlugins = new ArrayList<Plugin>();
	}

	public static String getProtocolByPort( String port ){
		if( mPorts == null )
		{
			mPorts = new HashMap<String,String>();
			
			// preload network service map
			try
			{
				final Pattern PARSER = Pattern.compile( "^([^\\s]+)\\s+(\\d+).*$", Pattern.CASE_INSENSITIVE );
						
				FileInputStream fstream = new FileInputStream( mContext.getFilesDir().getAbsolutePath() + "/tools/nmap/nmap-services" );
				DataInputStream in 	    = new DataInputStream(fstream);
				BufferedReader  reader  = new BufferedReader(new InputStreamReader(in));
				String 		    line;
				Matcher			matcher;
					
				while( ( line = reader.readLine() ) != null )   
				{
					line = line.trim();
					  
					if( ( matcher = PARSER.matcher(line) ) != null && matcher.find() )
					{
						mPorts.put( matcher.group( 2 ), matcher.group( 1 ) );
					}
				}
				  
				in.close();
			}
			catch( Exception e )
			{
				Log.e( TAG, e.toString() );
			}
		}
		
		return mPorts.containsKey(port) ? mPorts.get(port) : null;
	}
	
	public static int getPortByProtocol( String protocol ){
		if( mServices == null )
		{
			mServices = new HashMap<String,String>();
			
			// preload network service map
			try
			{
				final Pattern PARSER = Pattern.compile( "^([^\\s]+)\\s+(\\d+).*$", Pattern.CASE_INSENSITIVE );
						
				FileInputStream fstream = new FileInputStream( mContext.getFilesDir().getAbsolutePath() + "/tools/nmap/nmap-services" );
				DataInputStream in 	    = new DataInputStream(fstream);
				BufferedReader  reader  = new BufferedReader(new InputStreamReader(in));
				String 		    line;
				Matcher			matcher;
					
				while( ( line = reader.readLine() ) != null )   
				{
					line = line.trim();
					  
					if( ( matcher = PARSER.matcher(line) ) != null && matcher.find() )
					{
						String proto = matcher.group( 1 ),
							   port  = matcher.group( 2 );
						
						mServices.put( proto, port );
					}
				}
				  
				in.close();
			}
			catch( Exception e )
			{
				Log.e( TAG, e.toString() );
			}
		}
		
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
	
	public static void setTarget( Target target ){
		Log.d( TAG, "setTarget( " + target + " )" );
		mTarget = target;
	}
	
	public static Target getTarget(){
		return mTarget;
	}
	
	public static void registerPlugin( Plugin plugin ){
		Log.d( TAG, "Registering plugin '" + plugin.getName() + "'" );
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
		
		Log.d( TAG, "Returning " + filtered.size() + " plugins for current target ( " + mTarget + " )" );
		
		return filtered;
	}
	
	public static ArrayList<Plugin> getPluginsForTarget(){
		return getPluginsForTarget( mTarget );
	} 
	
	public static void setCurrentPlugin( Plugin plugin ){
		Log.d( TAG, "Setting current plugin : " + plugin.getName() );
		
		mCurrentPlugin = plugin;
	}
	
	public static Plugin getCurrentPlugin( ){
		return mCurrentPlugin;
	}
	
	public static void addOpenPort( int port, Protocol protocol ) {
		Port p = new Port( port, protocol );
		
		if( mTarget != null )
			mTarget.addOpenPort( p );
		
		for( Plugin plugin : getPluginsForTarget() ) {
			plugin.onTargetNewOpenPort( mTarget, p );
		}
	}

	public static String getGatewayAddress() {
		return mNetwork.getGatewayAddress().toString().substring( 1 );
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
	}
	
}
