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

import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Toast;
import android.widget.ToggleButton;
import android.widget.AdapterView.OnItemLongClickListener;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Target.Port;
import it.evilsocket.dsploit.tools.NMap.SynScanOutputReceiver;

public class PortScanner extends Plugin
{	
	private ToggleButton 		  mScanToggleButton = null;
	private ListView     		  mScanList		    = null;
	private ProgressBar			  mScanProgress     = null;
	private boolean      		  mRunning		    = false;
	private ArrayList<String>     mPortList    	    = null;
	private ArrayAdapter<String>  mListAdapter 	    = null;
	private Receiver              mScanReceiver     = null;
	private String				  mCustomPorts		= null;
	
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
			
			PortScanner.this.runOnUiThread( new Runnable() {
				@Override
                public void run() {
                	String proto = System.getProtocolByPort( openPort ),
                		   entry = openPort;
                	
                	if( proto != null )
                		entry = openPort + " ( " + proto + " )";
                	
                	else
                		entry = portProto + " : " + openPort;
                	
                	// add open port to the listview and notify the environment about the event
                	mPortList.add( entry );
                	mListAdapter.notifyDataSetChanged();
                	
                	System.addOpenPort( Integer.parseInt( openPort ), Network.Protocol.fromString(portProto) );                	
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
		mScanReceiver = new Receiver();
	}
	
	private void setStoppedState( ) {
		System.getNMap().kill();
		mScanProgress.setVisibility( View.INVISIBLE );
		mRunning = false;
		mScanToggleButton.setChecked( false );
		
		if( mPortList.size() == 0 )
        	Toast.makeText( this, "No open ports found, check if the target is reachable.", Toast.LENGTH_LONG ).show();	                	
	}
	
	private void setStartedState( ) {
		mPortList.clear();

		System.getNMap().synScan( System.getCurrentTarget(), mScanReceiver, mCustomPorts ).start();
		
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
		
		mScanList = ( ListView )findViewById( R.id.scanListView );
		
		for( Port port : System.getCurrentTarget().getOpenPorts() )
		{
			mPortList.add( port.number + " ( " + port.protocol.toString().toLowerCase() + " )" );
		}
		
		mListAdapter = new ArrayAdapter<String>( this, android.R.layout.simple_list_item_1, mPortList );
		mScanList.setAdapter( mListAdapter );
		mScanList.setOnItemLongClickListener( new OnItemLongClickListener(){
			@Override
			public boolean onItemLongClick( AdapterView<?> parent, View view, int position, long id ) {
				int portNumber = System.getCurrentTarget().getOpenPorts().get( position ).number;
				String url = "";
				
				if( portNumber == 80 )
					url = "http://" + System.getCurrentTarget().getCommandLineRepresentation() + "/";
				
				else if( portNumber == 443 )
					url = "https://" + System.getCurrentTarget().getCommandLineRepresentation() + "/";
				
				else if( portNumber == 21 )
					url = "ftp://" + System.getCurrentTarget().getCommandLineRepresentation();
				
				else if( portNumber == 22 )
					url = "ssh://" + System.getCurrentTarget().getCommandLineRepresentation();
				
				else 
					url = "telnet://" + System.getCurrentTarget().getCommandLineRepresentation() + ":" + portNumber; 

				final String furl = url;
						
				new ConfirmDialog
            	( 
            	  "Open", 
            	  "Open " + url + " ?",
            	  PortScanner.this,
            	  new ConfirmDialogListener(){
					@Override
					public void onConfirm() {
						try
						{
							Intent browser = new Intent( Intent.ACTION_VIEW, Uri.parse( furl ) );
						
							PortScanner.this.startActivity( browser );
						}
						catch( ActivityNotFoundException e )
						{
							System.errorLogging( "PORTSCANNER", e );
							
							new ErrorDialog( "Error", "No activities to handle this type of url.", PortScanner.this ).show();
						}
						
					}
					
					@Override
					public void onCancel() { }
            	  }
            	).show();
				
				return false;
			}
		});
	}
	
	@Override
	public boolean onCreateOptionsMenu( Menu menu ) {
		MenuInflater inflater = getSupportMenuInflater();		
		inflater.inflate( R.menu.port_scanner, menu );		
		return super.onCreateOptionsMenu(menu);
	}
		
	@Override
	public boolean onOptionsItemSelected( MenuItem item ) 
	{    
		switch( item.getItemId() ) 
		{        
			case R.id.select_ports:            
	         
				new InputDialog( "Select Ports", "Enter a comma separated list of ports:", this, new InputDialogListener() {					
					@Override
					public void onInputEntered( String input ) {
						input = input.trim();
						
						if( input.isEmpty() == false )
						{
							String[] ports = input.split("[^\\d]+");
							for( String port : ports )
							{
								try
								{
									if( port.isEmpty() )
										throw new Exception( "Invalid port '" + port + "'." );	
									
									else
									{
										int iport = Integer.parseInt( port );
		                                if( iport <= 0 || iport > 65535 )		                                
		                                	throw new Exception( "Port must be greater than 0 and smaller than 65535." );		                                
									}
								}
								catch( Exception e )
								{
									new ErrorDialog( "Error", e.toString(), PortScanner.this ).show();
									return;
								}
							}
							
							mCustomPorts = "";
							for( int i = 0, last = ports.length - 1; i < ports.length; i++ )
							{
								mCustomPorts += ports[i];
								if( i != last )
									mCustomPorts += ",";
							}
							
							if( mCustomPorts.isEmpty() )
							{
								mCustomPorts = null;
								new ErrorDialog( "Error", "Invalid ports specified.", PortScanner.this ).show();
							}
							
							android.util.Log.d( "PORT", "mCustomPorts = " + mCustomPorts );
						}
						else
							new ErrorDialog( "Error", "Empty port list.", PortScanner.this ).show();
					}
				}).show();
				
				return true;
	    	  
			default:            
				return super.onOptionsItemSelected(item);    
	   }
	}
	
	@Override
	public void onBackPressed() {
		setStoppedState();	
	    super.onBackPressed();
	}
}
