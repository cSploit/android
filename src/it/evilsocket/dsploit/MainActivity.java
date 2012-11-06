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

import it.evilsocket.dsploit.core.ManagedReceiver;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.ToolsInstaller;
import it.evilsocket.dsploit.core.UpdateChecker;
import it.evilsocket.dsploit.gui.dialogs.AboutDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import it.evilsocket.dsploit.gui.dialogs.ChangelogDialog;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.FatalDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.gui.dialogs.SpinnerDialog;
import it.evilsocket.dsploit.gui.dialogs.SpinnerDialog.SpinnerDialogListener;
import it.evilsocket.dsploit.net.Endpoint;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.NetworkDiscovery;
import it.evilsocket.dsploit.net.Target;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences.Editor;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.text.Html;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;

public class MainActivity extends SherlockListActivity
{	
	private static final int    WIFI_CONNECTION_REQUEST = 1012;
	private static final String NO_WIFI_UPDATE_MESSAGE  = "No WiFi connection available, the application will just check for updates.\n#STATUS#";
	 
	private boolean			 isWifiAvailable  		 = false;
	private boolean			 isConnectivityAvailable = false;
	private TargetAdapter  	 mTargetAdapter   	     = null;
	private NetworkDiscovery mNetworkDiscovery		 = null;
	private UpdateChecker	 mUpdateChecker			 = null;
	private EndpointReceiver mEndpointReceiver 		 = null;
	private UpdateReceiver	 mUpdateReceiver		 = null;	
	private Menu			 mMenu					 = null;
	private TextView		 mUpdateStatus			 = null;
	private Toast 			 mToast 			 	 = null;
	private long  			 mLastBackPressTime 	 = 0;
	
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
	        
	        if( target.hasAlias() == true )
	        {
	        	holder.itemTitle.setText
	        	(
	    			Html.fromHtml
		        	(
		        	  "<b>" + target.getAlias() + "</b> <small>( " + target.getDisplayAddress() +" )</small>"
		  			)	
	        	);
	        }
	        else
	        	holder.itemTitle.setText( target.toString() );
	        
        	holder.itemTitle.setTypeface( null, Typeface.NORMAL );
        	holder.itemImage.setImageResource( target.getDrawableResourceId() );
        	holder.itemDescription.setText( target.getDescription() );
        		       	       	        
	        return row;
	    }
	}

	private class EndpointReceiver extends ManagedReceiver
	{
		private IntentFilter mFilter = null;
		
		public EndpointReceiver() {
			mFilter = new IntentFilter();
			
			mFilter.addAction( NetworkDiscovery.NEW_ENDPOINT );
			mFilter.addAction( NetworkDiscovery.ENDPOINT_UPDATE );
		}
		
		public IntentFilter getFilter( ) {
			return mFilter;
		}
		
		@Override
		public void onReceive( Context context, Intent intent ) 
		{
			if( intent.getAction().equals( NetworkDiscovery.NEW_ENDPOINT ) )
			{
				String address  = ( String )intent.getExtras().get( NetworkDiscovery.ENDPOINT_ADDRESS ),
					   hardware = ( String )intent.getExtras().get( NetworkDiscovery.ENDPOINT_HARDWARE ),
					   name		= ( String )intent.getExtras().get( NetworkDiscovery.ENDPOINT_NAME );
				final  Target target = Target.getFromString( address );
				
				if( target != null && target.getEndpoint() != null )
				{
					if( name != null && name.isEmpty() == false )
						target.setAlias( name );
					
					target.getEndpoint().setHardware( Endpoint.parseMacAddress(hardware) );
																												
					// refresh the target listview
	            	MainActivity.this.runOnUiThread(new Runnable() {
	                    @Override
	                    public void run() {
	                    	if( System.addOrderedTarget( target ) == true )
							{
	                    		mTargetAdapter.notifyDataSetChanged();
							}
	                    }
	                });		
				}
			}	
			else if( intent.getAction().equals( NetworkDiscovery.ENDPOINT_UPDATE ) )
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
	}
	
	private class UpdateReceiver extends ManagedReceiver
	{
		private IntentFilter mFilter = null;
		
		public UpdateReceiver( ) {
			mFilter = new IntentFilter();
			
			mFilter.addAction( UpdateChecker.UPDATE_CHECKING );
			mFilter.addAction( UpdateChecker.UPDATE_AVAILABLE );
			mFilter.addAction( UpdateChecker.UPDATE_NOT_AVAILABLE );
		}
		
		public IntentFilter getFilter( ) {
			return mFilter;
		}
		
		@Override
		public void onReceive( Context context, Intent intent ) 
		{
			if( mUpdateStatus != null && intent.getAction().equals( UpdateChecker.UPDATE_CHECKING ) && mUpdateStatus != null )
			{
				mUpdateStatus.setText( NO_WIFI_UPDATE_MESSAGE.replace( "#STATUS#", "Checking ..." ) );
			}
			else if( mUpdateStatus != null && intent.getAction().equals( UpdateChecker.UPDATE_NOT_AVAILABLE ) && mUpdateStatus != null )
			{
				mUpdateStatus.setText( NO_WIFI_UPDATE_MESSAGE.replace( "#STATUS#", "No updates available." ) );
			}
			else if( intent.getAction().equals( UpdateChecker.UPDATE_AVAILABLE ) )
			{																						
				final String remoteVersion = ( String )intent.getExtras().get( UpdateChecker.AVAILABLE_VERSION );
				
				if( mUpdateStatus != null )
					mUpdateStatus.setText( NO_WIFI_UPDATE_MESSAGE.replace( "#STATUS#", "New version " + remoteVersion + " found!" ) );
				
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
								final ProgressDialog dialog = new ProgressDialog( MainActivity.this );
								dialog.setTitle( "Downloading update ..." );
							    dialog.setMessage( "Connecting ..." );
							    dialog.setProgressStyle( ProgressDialog.STYLE_HORIZONTAL );
							    dialog.setMax(100);
							    dialog.setCancelable( false );
							    dialog.setButton( DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
							        @Override
							        public void onClick(DialogInterface dialog, int which) {
							            dialog.dismiss();
							        }
							    });
							    dialog.show();
								
								new Thread( new Runnable(){
									@Override
									public void run() 
									{				
										if( System.getUpdateManager().downloadUpdate( MainActivity.this, dialog ) == false )
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
							}
							
							@Override
							public void onCancel() { }
						}
                    	).show();
                    }
                });
			}	
		}
	}
	
	private void createUpdateLayout( ) {	
		
		getListView().setVisibility( View.GONE );
		findViewById( R.id.textView ).setVisibility( View.GONE );
		
		RelativeLayout layout = ( RelativeLayout )findViewById( R.id.layout );
		LayoutParams   params = new LayoutParams( LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT );		
		
		mUpdateStatus = new TextView( this );
		
		mUpdateStatus.setGravity( Gravity.CENTER );
		mUpdateStatus.setLayoutParams( params );
		mUpdateStatus.setText( NO_WIFI_UPDATE_MESSAGE.replace( "#STATUS#", "..." ) );
		
		layout.addView( mUpdateStatus );		

		if( mUpdateReceiver == null )
			mUpdateReceiver = new UpdateReceiver();
		
	    mUpdateReceiver.unregister();
	    
        mUpdateReceiver.register( MainActivity.this );
		
        startUpdateChecker();
        stopNetworkDiscovery( true );
        
		invalidateOptionsMenu();
	}
	
	private void createOfflineLayout( ) {
		
		getListView().setVisibility( View.GONE );
		findViewById( R.id.textView ).setVisibility( View.GONE );
		
		RelativeLayout layout = ( RelativeLayout )findViewById( R.id.layout );
		LayoutParams   params = new LayoutParams( LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT );		
		
		mUpdateStatus = new TextView( this );
		
		mUpdateStatus.setGravity( Gravity.CENTER );
		mUpdateStatus.setLayoutParams( params );
		mUpdateStatus.setText( "No connectivity available." );
		
		layout.addView( mUpdateStatus );
		
		stopNetworkDiscovery( true );
		
		invalidateOptionsMenu();
	}
	
	public void createOnlineLayout( ) {
		mTargetAdapter = new TargetAdapter( R.layout.target_list_item );
		
		setListAdapter( mTargetAdapter );	
	
		getListView().setOnItemLongClickListener( new OnItemLongClickListener() 
		{
			@Override
			public boolean onItemLongClick( AdapterView<?> parent, View view, int position, long id ) {									
				final Target target = System.getTarget( position );
				
				new InputDialog
				( 
				  "Target Alias", 
				  "Set an alias for this target:", 
				  target.hasAlias() ? target.getAlias() : "",
				  true,
				  false,
				  MainActivity.this, 
				  new InputDialogListener(){
					@Override
					public void onInputEntered( String input ) {
						target.setAlias(input);			
						mTargetAdapter.notifyDataSetChanged();
					}
				  }
				).show();
														
				return false;
			}
		});

		if( mEndpointReceiver == null )		
			mEndpointReceiver = new EndpointReceiver();
		
		if( mUpdateReceiver == null )
			mUpdateReceiver = new UpdateReceiver();
		
	    mEndpointReceiver.unregister();
	    mUpdateReceiver.unregister();
	    
	    mEndpointReceiver.register( MainActivity.this );		
        mUpdateReceiver.register( MainActivity.this );
        
        startUpdateChecker();
        startNetworkDiscovery( false );
        
        // if called for the second time after wifi connection
        invalidateOptionsMenu();
	}
	
	@Override
	protected void onActivityResult( int requestCode, int resultCode, Intent intent ) {		
		if( requestCode == WIFI_CONNECTION_REQUEST && resultCode == RESULT_OK && intent.hasExtra( WifiScannerActivity.CONNECTED ) ) 
		{
			System.reloadNetworkMapping();						
            onCreate( null );
		}
	}
	
	@Override
    public void onCreate( Bundle savedInstanceState ) {		
        super.onCreate(savedInstanceState);           
        setContentView( R.layout.target_layout );
            	        	    	
        isWifiAvailable 		= Network.isWifiConnected( this );
    	isConnectivityAvailable = isWifiAvailable || Network.isConnectivityAvailable( this );
    	
    	// make sure system object was correctly initialized during application startup
    	if( System.isInitialized() == false )
    	{
    		// wifi available but system failed to initialize, this is a fatal :(
    		if( isWifiAvailable == true )
    		{
    			new FatalDialog( "Initialization Error", System.getLastError(), this ).show();        		
    			return;
    		}    		
    	}
    	    	
    	// initialization ok, but wifi is down
    	if( isWifiAvailable == false )
    	{
    		// just inform the user his wifi is down
    		if( isConnectivityAvailable )
    			createUpdateLayout( );
    		
    		// no connectivity at all
    		else
    			createOfflineLayout( ); 
    	}
    	// we are online, and the system was already initialized
    	else if( mTargetAdapter != null )
    		createOnlineLayout( );
    	
    	// initialize the ui for the first time
    	else if( mTargetAdapter == null )
        {	
        	final ProgressDialog dialog = ProgressDialog.show( this, "", "Initializing ...", true, false );
						
        	// this is necessary to not block the user interface while initializing
        	new Thread( new Runnable(){
				@Override
				public void run() 
				{				
					dialog.show();

					Context		   appContext = MainActivity.this.getApplicationContext(); 
					String 		   fatal      = null;										
					ToolsInstaller installer  = new ToolsInstaller( appContext );
			       
					if( Shell.isRootGranted() == false )  					
						fatal = "This application can run only on rooted devices.";
					
					else if( Shell.isBinaryAvailable("killall") == false )
		    			fatal = "Full BusyBox installation required, killall binary not found ( maybe you have an old busybox version ).";
		        							        						               
					else if( installer.needed() && installer.install() == false )			        
						fatal = "Error during files installation!";
					// check for LD_LIBRARY_PATH issue ( https://github.com/evilsocket/dsploit/issues/35 )
					else if( Shell.isLibraryPathOverridable( appContext ) == false )
			        	fatal = "<p>It seems like your ROM has the <b>LD_LIBRARY_PATH bug</b>, i'm sorry but it's not compatible with dSploit.</p>" +
			        			"<p>For more info read the <a href=\"http://dsploit.net/#faq\">FAQ</a>.</p>";
			        	
					dialog.dismiss();
					
					if( fatal != null )
					{						
						final String ffatal = fatal;
						MainActivity.this.runOnUiThread( new Runnable(){
							@Override
							public void run() {
								new FatalDialog( "Error", ffatal, ffatal.contains(">"), MainActivity.this ).show();
							}
						});													
					}
	
					MainActivity.this.runOnUiThread( new Runnable(){			    			
						@Override
						public void run() 
						{
							if( System.getAppVersionName().equals( System.getSettings().getString( "DSPLOIT_INSTALLED_VERSION", "0" ) ) == false )
							{
								new ChangelogDialog( MainActivity.this ).show();
								
								Editor editor = System.getSettings().edit();
								
								editor.putString( "DSPLOIT_INSTALLED_VERSION", System.getAppVersionName() );
								editor.commit();
							}
							
						    try
					    	{	    							        	
						    	createOnlineLayout( );						    							    	
					    	}
					    	catch( Exception e )
					    	{
					    		new FatalDialog( "Error", e.getMessage(), MainActivity.this ).show();	    		
					    	}									
						}
					});							
				}
			}).start();        		    		    
        }                       
	}
	
	@Override
	public boolean onCreateOptionsMenu( Menu menu ) {
		MenuInflater inflater = getSupportMenuInflater();		
		inflater.inflate( R.menu.main, menu );		
						
		if( isWifiAvailable == false )
		{			
			menu.findItem( R.id.add ).setVisible( false );
			menu.findItem( R.id.scan ).setVisible( false );			
			menu.findItem( R.id.new_session ).setEnabled( false );
			menu.findItem( R.id.save_session ).setEnabled( false );
			menu.findItem( R.id.restore_session ).setEnabled( false );
			menu.findItem( R.id.settings ).setEnabled( false );
			menu.findItem( R.id.ss_monitor ).setEnabled( false );
			menu.findItem( R.id.ss_monitor ).setEnabled( false );
		}
		
		mMenu = menu;
				
		return super.onCreateOptionsMenu(menu);
	}
	
	@Override
	public boolean onPrepareOptionsMenu( Menu menu ) {		
		MenuItem item = menu.findItem( R.id.ss_monitor );
				
		if( mNetworkDiscovery != null && mNetworkDiscovery.isRunning() )				
			item.setTitle( "Stop Network Monitor" );
		
		else				
			item.setTitle( "Start Network Monitor" );
		
		mMenu = menu;
						
		return super.onPrepareOptionsMenu(menu);
	}
	
	public void startUpdateChecker( ) {
		if( System.getSettings().getBoolean( "PREF_CHECK_UPDATES", true ) )
		{
			mUpdateChecker = new UpdateChecker( this );
			mUpdateChecker.start();
		}		
	}
	
	public void startNetworkDiscovery( boolean silent ) {
		stopNetworkDiscovery( silent );
		
		mNetworkDiscovery = new NetworkDiscovery( this );
		mNetworkDiscovery.start();
		
		if( silent == false )
			Toast.makeText( this, "Network discovery started.", Toast.LENGTH_SHORT ).show();
	}
	
	public void stopNetworkDiscovery( boolean silent ) {
		if( mNetworkDiscovery != null )
		{
			if( mNetworkDiscovery.isRunning() )
			{
				mNetworkDiscovery.exit();
				try
				{
					mNetworkDiscovery.join();
				}
				catch( Exception e )
				{
					// swallow
				}
				
				if( silent == false )
					Toast.makeText( this, "Network discovery stopped.", Toast.LENGTH_SHORT ).show();
			}
			
			mNetworkDiscovery = null;
		}
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
					final Target target = Target.getFromString(input);						
					if( target != null )
					{																							
						// refresh the target listview
                    	MainActivity.this.runOnUiThread( new Runnable() {	                    		
		                    @Override
		                    public void run() {
		        				if( System.addOrderedTarget( target ) == true && mTargetAdapter != null )
								{		
		        					mTargetAdapter.notifyDataSetChanged();
								}
		                    }
		                });
					}
					else
						new ErrorDialog( "Error", "Invalid target.", MainActivity.this ).show();
				}} 
			).show();
			
			return true;
		}
		else if( itemId == R.id.scan )
		{			
			if( mMenu != null )
				mMenu.findItem( R.id.scan ).setActionView( new ProgressBar(this) );
			
			new Thread( new Runnable(){
				@Override
				public void run() {
					startNetworkDiscovery( true );
					
					MainActivity.this.runOnUiThread( new Runnable(){
						@Override
						public void run() {
							if( mMenu != null )
								mMenu.findItem( R.id.scan ).setActionView( null);
						}
					});
				}
			}).start();
									
			item.setTitle( "Stop Network Monitor" );

			return true;
		}
		else if( itemId == R.id.wifi_scan )
		{
			stopNetworkDiscovery( true );
			
			if( mEndpointReceiver != null )
				mEndpointReceiver.unregister();
			
			if( mUpdateReceiver != null )
				mUpdateReceiver.unregister();
			
			startActivityForResult( new Intent( MainActivity.this, WifiScannerActivity.class ), WIFI_CONNECTION_REQUEST );
			
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
				}
				
				@Override
				public void onCancel() { }
				
			}).show();										
							
			return true;
		}
		else if( itemId == R.id.save_session )
		{
			new InputDialog( "Save Session", "Enter the name of the session file :", System.getSessionName(), true, false, MainActivity.this, new InputDialogListener(){
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
			if( mNetworkDiscovery != null && mNetworkDiscovery.isRunning() )
			{
				stopNetworkDiscovery( false );
				
				item.setTitle( "Start Network Monitor" );
			}
			else
			{
				startNetworkDiscovery( false );
				
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

		stopNetworkDiscovery( true );
		
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
	public void onBackPressed() {
		if( mLastBackPressTime < java.lang.System.currentTimeMillis() - 4000 ) 
		{
			mToast = Toast.makeText( this, "Press back again to close this app.", Toast.LENGTH_SHORT );
			mToast.show();
			mLastBackPressTime = java.lang.System.currentTimeMillis();
		} 
		else
		{
			if( mToast != null ) 
				mToast.cancel();
			
			new ConfirmDialog( "Exit", "This will close dSploit, are you sure you want to continue ?", this, new ConfirmDialogListener() {				
				@Override
				public void onConfirm() {
					MainActivity.this.finish();
				}
				
				@Override
				public void onCancel() { }
			}).show();
			
			mLastBackPressTime = 0;
		}
	}

	
	@Override
	public void onDestroy() {
		stopNetworkDiscovery( true );
					
		if( mEndpointReceiver != null )
			mEndpointReceiver.unregister();
		
		if( mUpdateReceiver != null )
			mUpdateReceiver.unregister();
				
		// make sure no zombie process is running before destroying the activity
		System.clean( true );		
						
		super.onDestroy();		
		
		// remove the application from the cache
		java.lang.System.exit( 0 );
	}
}