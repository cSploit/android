package it.evilsocket.dsploit.tools;

import android.util.Log;

public class IPTables extends Tool
{			
	private static final String TAG = "IPTABLES";
	
	public IPTables( ){
		super( "iptables" );		
		setCustomLibsUse(false);		
	}
	
	public void portRedirect( int from, int to ){
		try
		{
			super.run("-P FORWARD ACCEPT");
			super.run("-t nat -A POSTROUTING -j MASQUERADE");
			super.run("-t nat -A PREROUTING -p tcp --dport " + from + " -j REDIRECT --to-port " + to );
		}
		catch( Exception e )
		{
			Log.e( TAG, e.toString() );
		}
	}
	
	public void undoPortRedirect( int from, int to ){
		try
		{
			super.run("-t nat -D PREROUTING -p tcp --dport " + from + " -j REDIRECT --to-port " + to );
			super.run("-t nat -D POSTROUTING -j MASQUERADE");
		}
		catch( Exception e )
		{
			Log.e( TAG, e.toString() );
		}
	}
	
}
