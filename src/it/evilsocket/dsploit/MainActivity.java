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
package it.evilsocket.dsploit;

import java.io.IOException;
import java.util.ArrayList;

import com.actionbarsherlock.app.SherlockListActivity;

import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.ToolsInstaller;
import it.evilsocket.dsploit.core.UpdateService;
import it.evilsocket.dsploit.gui.dialogs.AboutDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.FatalDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.gui.dialogs.SpinnerDialog;
import it.evilsocket.dsploit.gui.dialogs.SpinnerDialog.SpinnerDialogListener;
import it.evilsocket.dsploit.net.Endpoint;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.NetworkMonitorService;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.plugins.ExploitFinder;
import it.evilsocket.dsploit.plugins.Inspector;
import it.evilsocket.dsploit.plugins.LoginCracker;
import it.evilsocket.dsploit.plugins.PacketForger;
import it.evilsocket.dsploit.plugins.PortScanner;
import it.evilsocket.dsploit.plugins.mitm.MITM;
import it.evilsocket.dsploit.tools.NMap.FindAliveEndpointsOutputReceiver;

import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;

public class MainActivity extends SherlockListActivity
{
	private static final int LAYOUT = R.layout.target_layout;
	
	private TargetAdapter  	  mTargetAdapter   = null;
	private IntentFilter	  mIntentFilter	   = null;
	private BroadcastReceiver mMessageReceiver = null;
	
	public class TargetAdapter extends ArrayAdapter<Target> 
	{
		private int mLayoutId = 0;
		
		class TargetHolder
	    {
	        ImageView  itemImage;
	        TextView   itemTitle;
	        TextView   itemDescription;
	    }
		
		public TargetAdapter( int layoutId ) {		
	        super( MainActivity.this, layoutId );
	        
	        mLayoutId = layoutId;
	    }

		@Override
		public int getCount(){
			return System.getTargets().size();
		}
		
		@Override
	    public View getView( int position, View convertView, ViewGroup parent ) {		
	        View 		 row    = convertView;
	        TargetHolder holder = null;
	        
	        if( row == null )
	        {
	            LayoutInflater inflater = ( LayoutInflater )MainActivity.this.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
	            row = inflater.inflate( mLayoutId, parent, false );
	            
	            holder = new TargetHolder();
	            
	            holder.itemImage  	   = ( ImageView )row.findViewById( R.id.itemIcon );
	            holder.itemTitle  	   = ( TextView )row.findViewById( R.id.itemTitle );
	            holder.itemDescription = ( TextView )row.findViewById( R.id.itemDescription );

	            row.setTag(holder);
	        }
	        else
	        {
	            holder = ( TargetHolder )row.getTag();
	        }
	        
	        Target target = System.getTarget( position );
	        	
        	holder.itemImage.setImageResource( target.getDrawableResourceId() );
        	holder.itemTitle.setText( target.toString() );
        	holder.itemTitle.setTypeface( null, Typeface.NORMAL );
        	holder.itemDescription.setText( target.getDescription() );
        		       	       	        
	        return row;
	    }
	}

	@Override
    public void onCreate( Bundle savedInstanceState ) {		
        super.onCreate(savedInstanceState);   
        setContentView( LAYOUT );
        
        if( System.isInitialized() == false )
        {	        	        
        	final ProgressDialog dialog = ProgressDialog.show( MainActivity.this, "", "Initializing ...", true, false );

	        new Thread( new Runnable(){
				@Override
				public void run() 
				{												
					dialog.show();

					String 		   fatal     = null;										
					ToolsInstaller installer = new ToolsInstaller( MainActivity.this.getApplicationContext() );
			       
					if( Shell.isRootGranted() == false )  					
						fatal = "This application can run only on rooted devices.";
					
					else if( Shell.isBinaryAvailable("killall") == false )
		    			fatal = "Full BusyBox installation required, killall binary not found.";
		        							        						               
					else if( installer.needed() && installer.install() == false )			        
						fatal = "Error during files installation!";
			        					
					dialog.dismiss();
					
					if( fatal != null )
					{
						final String fatalMessage = fatal;
						MainActivity.this.runOnUiThread( new Runnable() {
					       public void run() {
					    	   new FatalDialog( "Error", fatalMessage, MainActivity.this ).show();
					       }
					    });
					}
					else if( Network.isWifiConnected( MainActivity.this ) ) 
					{
						startService( new Intent( MainActivity.this, NetworkMonitorService.class ) );
					}
				}
			}).start();
	            					
		    try
	    	{	    	
		    	// initialize the system
		    	System.init( getApplicationContext() );

		        System.registerPlugin( new PortScanner( ) );
		        System.registerPlugin( new Inspector( ) );
		        System.registerPlugin( new ExploitFinder( ) );
		        System.registerPlugin( new LoginCracker( ) );
		        System.registerPlugin( new MITM( ) );
		        System.registerPlugin( new PacketForger( ) );

				mTargetAdapter = new TargetAdapter( R.layout.target_list_item );
		    	
				setListAdapter( mTargetAdapter );	
				
				mMessageReceiver = new BroadcastReceiver() {
					@Override
					public void onReceive(Context context, Intent intent) {
						if( intent.getAction().equals( NetworkMonitorService.NEW_ENDPOINT ) )
						{
							String address  = ( String )intent.getExtras().get( NetworkMonitorService.ENDPOINT_ADDRESS ),
								   hardware = ( String )intent.getExtras().get( NetworkMonitorService.ENDPOINT_HARDWARE );	            	
							Target target	= Target.getFromString( address );
							
							target.getEndpoint().setHardware( Endpoint.parseMacAddress(hardware) );
							
							if( System.addOrderedTarget( target ) == true )
							{													
								// refresh the target listview
				            	MainActivity.this.runOnUiThread(new Runnable() {
				                    @Override
				                    public void run() {
				                    	mTargetAdapter.notifyDataSetChanged();
				                    }
				                });
							}			
						}							
						else if( intent.getAction().equals( UpdateService.UPDATE_AVAILABLE ) )
						{
							final String remoteVersion = ( String )intent.getExtras().get( UpdateService.AVAILABLE_VERSION );
							
							MainActivity.this.runOnUiThread(new Runnable() {
			                    @Override
			                    public void run() {
			                    	new ConfirmDialog
			                    	( 
			                    	  "Update Available", 
			                    	  "A new update to version " + remoteVersion + " is available, do you want to download it ?",
			                    	  MainActivity.this,
			                    	  new ConfirmDialogListener(){
										@Override
										public void onConfirm() 
										{
											final ProgressDialog dialog = ProgressDialog.show( MainActivity.this, "", "Downloading update ...", true, false );
											
											new Thread( new Runnable(){
												@Override
												public void run() 
												{				
													if( System.getUpdateManager().downloadUpdate() == false )
													{
														MainActivity.this.runOnUiThread(new Runnable() {
										                    @Override
										                    public void run() {
										                    	new ErrorDialog( "Error", "An error occurred while downloading the update.", MainActivity.this ).show();
										                    }
										                });
													}
													
													dialog.dismiss();
												}
											}).start();											
										}}
			                    	).show();
			                    }
			                });
						}							
					}
			    };
			    
			    mIntentFilter = new IntentFilter( );
			    
			    mIntentFilter.addAction( NetworkMonitorService.NEW_ENDPOINT );
			    mIntentFilter.addAction( UpdateService.UPDATE_AVAILABLE );
			    
		        registerReceiver( mMessageReceiver, mIntentFilter );		
		        
		        if( System.getSettings().getBoolean( "PREF_CHECK_UPDATES", true ) )
		        	startService( new Intent( this, UpdateService.class ) );
	    	}
	    	catch( Exception e )
	    	{
	    		new FatalDialog( "Error", e.getMessage(), this ).show();
	    	}		    		    
        }                       
	}
	
	@Override
	public boolean onCreateOptionsMenu( Menu menu ) {
		MenuInflater inflater = getSupportMenuInflater();
		inflater.inflate( R.menu.main, menu );		
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected( MenuItem item ) {
					
		int itemId = item.getItemId();
		
		if( itemId == R.id.add )
		{
			new InputDialog( "Add custom target", "Enter an URL, host name or ip address below:", MainActivity.this, new InputDialogListener(){
				@Override
				public void onInputEntered( String input ) 
				{
					Target target = Target.getFromString(input);						
					if( target != null )
					{
						if( System.addOrderedTarget( target ) == true )
						{																								
							// refresh the target listview
	                    	MainActivity.this.runOnUiThread(new Runnable() {
			                    @Override
			                    public void run() {
			                    	if( mTargetAdapter != null )
			                    		mTargetAdapter.notifyDataSetChanged();
			                    }
			                });
						}
					}
					else
						new ErrorDialog( "Error", "Invalid target.", MainActivity.this ).show();
				}} 
			).show();
			
			return true;
		}
		else if( itemId == R.id.scan )
		{
			try
			{
				final ProgressDialog dialog = ProgressDialog.show( MainActivity.this, "", "Searching alive endpoints ...", true, false );
								
				System.getNMap().findAliveEndpoints( System.getNetwork(), new FindAliveEndpointsOutputReceiver(){						
					@Override
					public void onEndpointFound( Endpoint endpoint ) {
						Target target = new Target( endpoint );
						
						if( System.addOrderedTarget( target ) == true )
						{													
							// refresh the target listview
	                    	MainActivity.this.runOnUiThread(new Runnable() {
			                    @Override
			                    public void run() {
			                    	if( mTargetAdapter != null )
			                    		mTargetAdapter.notifyDataSetChanged();
			                    }
			                });
						}														
					}
				
					@Override
					public void onEnd( int code ) {
						dialog.dismiss();
					}
				}).start();		
			}
			catch( Exception e )
			{
				new FatalDialog( "Error", e.toString(), MainActivity.this ).show();
			}
			
			return true;
		}
		else if( itemId == R.id.new_session )
		{
			new ConfirmDialog( "Warning", "Starting a new session would delete the current one, continue ?", this, new ConfirmDialogListener(){
				@Override
				public void onConfirm() {
					try
					{
						System.reset();
						mTargetAdapter.notifyDataSetChanged();
						
						Toast.makeText( MainActivity.this, "New session started", Toast.LENGTH_SHORT ).show();
					}
					catch( Exception e )
					{
						new FatalDialog( "Error", e.toString(), MainActivity.this ).show();
					}
				}}
			).show();										
							
			return true;
		}
		else if( itemId == R.id.save_session )
		{
			new InputDialog( "Save Session", "Enter the name of the session file :", System.getSessionName(), true, MainActivity.this, new InputDialogListener(){
				@Override
				public void onInputEntered( String input ) 
				{
					String name = input.trim().replace( "/", "" ).replace( "..", "" );
					
					if( name.isEmpty() == false )
					{
						try
						{
							String filename = System.saveSession( name );
					
							Toast.makeText( MainActivity.this, "Session saved to " + filename + " .", Toast.LENGTH_SHORT ).show();
						}
						catch( IOException e )
						{
							new ErrorDialog( "Error", e.toString(), MainActivity.this ).show();
						}
					}
					else
						new ErrorDialog( "Error", "Invalid session name.", MainActivity.this ).show();
				}} 
			).show();	
			
			return true;
		}
		else if( itemId == R.id.restore_session )
		{
			final ArrayList<String> sessions = System.getAvailableSessionFiles();
			
			if( sessions != null && sessions.size() > 0 )
			{
				new SpinnerDialog( "Select Session", "Select a session file from the sd card :", sessions.toArray( new String[ sessions.size() ] ), MainActivity.this, new SpinnerDialogListener(){
					@Override
					public void onItemSelected(int index) 
					{						
						String session = sessions.get( index );
						
						try
						{
							System.loadSession( session );
							mTargetAdapter.notifyDataSetChanged();
						}
						catch( Exception e )
						{
							e.printStackTrace();
							new ErrorDialog( "Error", e.getMessage(), MainActivity.this ).show();
						}
					}
				}).show();
			}
			else
				new ErrorDialog( "Error", "No session file found on sd card.", MainActivity.this ).show();
			
			return true;
		}
		else if( itemId == R.id.settings )
		{
			startActivity( new Intent( MainActivity.this, SettingsActivity.class ) );
			
			return true;
		}
		else if( itemId == R.id.ss_monitor )
		{
			if( NetworkMonitorService.RUNNING )
			{
				stopService( new Intent( this, NetworkMonitorService.class ) );
				
				item.setTitle( "Start Network Monitor" );
			}
			else
			{
				startService( new Intent( this, NetworkMonitorService.class ) );
				
				item.setTitle( "Stop Network Monitor" );
			}
			
			return true;
		}
		else if( itemId == R.id.submit_issue )
		{
			String uri     = "https://github.com/evilsocket/dsploit/issues/new";
			Intent browser = new Intent( Intent.ACTION_VIEW, Uri.parse( uri ) );
			
			startActivity( browser );
			
			return true;
		}									
		else if( itemId == R.id.about )
		{
			new AboutDialog( this ).show();
			
			return true;
		}
		else
			return super.onOptionsItemSelected(item);
	}
	
	@Override
	protected void onListItemClick( ListView l, View v, int position, long id ){
		super.onListItemClick( l, v, position, id);
					
		System.setCurrentTarget( position );
		
		Toast.makeText( MainActivity.this, "Selected " + System.getCurrentTarget(), Toast.LENGTH_SHORT ).show();	                	
        startActivity
        ( 
          new Intent
          ( 
            MainActivity.this, 
            ActionActivity.class
          ) 
        );
        overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
	}

	@Override
	public void onDestroy() {				
		stopService( new Intent( MainActivity.this, NetworkMonitorService.class ) );
		
		if( mMessageReceiver != null )
			unregisterReceiver( mMessageReceiver );
		
		// make sure no zombie process is running before destroying the activity
		System.clean();			
		
		super.onDestroy();
		
		// this will remove the application from app cache, forcing it to re-initialize
		java.lang.System.exit(0);
	}
}