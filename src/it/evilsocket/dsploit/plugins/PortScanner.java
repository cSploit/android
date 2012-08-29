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

import java.util.ArrayList;

import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Toast;
import android.widget.ToggleButton;
import it.evilsocket.dsploit.MainActivity;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.system.Environment;
import it.evilsocket.dsploit.system.Plugin;
import it.evilsocket.dsploit.tools.NMap;
import it.evilsocket.dsploit.tools.NMap.SynScanOutputReceiver;

public class PortScanner extends Plugin
{	
	private ToggleButton 		  mScanToggleButton = null;
	private ListView     		  mScanList		    = null;
	private ProgressBar			  mScanProgress     = null;
	private boolean      		  mRunning		    = false;
	private ArrayList<String>     mPortList    	    = null;
	private ArrayAdapter<String>  mListAdapter 	    = null;
	private NMap           		  mNmap		  	    = null;
	private Receiver              mScanReceiver     = null;
	
	private class Receiver extends SynScanOutputReceiver
	{
		@Override 
		public void onStart( String commandLine ) {
			super.onStart( commandLine );
			
			PortScanner.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                	mRunning = true;
                	mScanProgress.setVisibility( View.VISIBLE );
                }
            });							
		}
		
		@Override
		public void onEnd( int exitCode ) {
			super.onEnd(exitCode);
			
			PortScanner.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                	setStoppedState();                	                
                }
            });										
		}
		
		@Override
		public void onPortFound( String port, String protocol ) {
			final String openPort  = port;
			final String portProto = protocol;
			
			PortScanner.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                	String proto = Environment.getProtocolByPort( openPort ),
                		   entry = openPort;
                	
                	if( proto != null )
                		entry = openPort + " ( " + proto + " )";
                	
                	else
                		entry = portProto + " : " + openPort;
                	
                	// add open port to the listview and notify the environment about the event
                	mPortList.add( entry );
                	mScanList.setAdapter( mListAdapter );
                	
                	Environment.addOpenPort( Integer.parseInt( openPort ), Network.Protocol.fromString(portProto) );                	
                }
            });					
		}
	}
	
	public PortScanner( ) {
		super
		( 
		    "Port Scanner", 
		    "Perform a SYN port scanning on target.", 
		    new Target.Type[]{ Target.Type.ENDPOINT, Target.Type.REMOTE }, 
		    R.layout.plugin_portscanner,
		    R.drawable.action_scanner_48 
		);

		mPortList     = new ArrayList<String>();
		mNmap 	      = new NMap( Environment.getContext() );
		mScanReceiver = new Receiver();
	}
	
	private void setStoppedState( ) {
		mNmap.kill();
		mScanProgress.setVisibility( View.INVISIBLE );
		mRunning = false;
		mScanToggleButton.setChecked( false );
		
		if( mPortList.size() == 0 )
        	Toast.makeText( this, "No open ports found, check if the target is reachable.", Toast.LENGTH_LONG ).show();	                	
	}
	
	private void setStartedState( ) {
		mPortList.clear();
		
		final NMap 					nmap		 = mNmap;
		final SynScanOutputReceiver scanReceiver = mScanReceiver;
		
		new Thread(new Runnable() {
            public void run() {	
            	nmap.synScan( Environment.getTarget(), scanReceiver );
            }
        }).start();
		
		mRunning = true;
	}

	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);    
        
        mScanToggleButton = ( ToggleButton )findViewById( R.id.scanToggleButton );
		mScanProgress	  = ( ProgressBar )findViewById( R.id.scanActivity );
		    	
		mScanToggleButton.setOnClickListener( new OnClickListener(){
			@Override
			public void onClick(View v) {
				if( mRunning )
				{
					setStoppedState();
				}
				else
				{
					setStartedState();
				}
			}} 
		);
		
		mScanList	 = ( ListView )findViewById( R.id.scanListView );
		mListAdapter = new ArrayAdapter<String>( this, android.R.layout.simple_list_item_1, mPortList );
		mScanList.setAdapter( mListAdapter );
	}
	
	@Override
	public void onBackPressed() {
		setStoppedState();	
	    super.onBackPressed();
	}
}
