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

import it.evilsocket.dsploit.core.CrashManager;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.ToolsInstaller;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.FatalDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.net.Endpoint;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.plugins.ExploitFinder;
import it.evilsocket.dsploit.plugins.Inspector;
import it.evilsocket.dsploit.plugins.LoginCracker;
import it.evilsocket.dsploit.plugins.PortScanner;
import it.evilsocket.dsploit.plugins.mitm.MITM;
import it.evilsocket.dsploit.tools.NMap;
import it.evilsocket.dsploit.tools.NMap.FindAliveEndpointsOutputReceiver;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity 
{
	private static final int LAYOUT = R.layout.target_layout;
	
	private ToolsInstaller mToolsInstaller = null;
	private TargetAdapter  mTargetAdapter  = null;
	private ListView	   mListView	   = null;
	private NMap		   mNmap		   = null;
	
	public class TargetAdapter extends ArrayAdapter<Target> 
	{
		private int mLayoutId = 0;
		
		class TargetHolder
	    {
			int			position;
			Target		target;
	        ImageView   itemImage;
	        TextView    itemTitle;
	        TextView	itemDescription;
	    }
		
		public TargetAdapter( int layoutId ) {		
	        super( MainActivity.this, layoutId );
	        
	        mLayoutId = layoutId;
	    }
											
		private class SelectTargetClickListener implements OnClickListener
		{
			public void onClick( View v ) 
			{		
				// select clicked target and go to action list activity
				int position = ( ( TargetHolder )v.getTag() ).position;
	
				System.setCurrentTarget( position );
				
				MainActivity.this.runOnUiThread( new Runnable() {
	                @Override
	                public void run() {
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
	            });
			}
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
	            
	            holder.position		   = position;
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
        	
        	row.setOnClickListener( new SelectTargetClickListener() );
	        
	        
	        holder.target = target;
	        
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
					
			        mToolsInstaller = new ToolsInstaller( MainActivity.this.getApplicationContext() );
			       
					if( Shell.isRootGranted() == false )  
					{
						dialog.dismiss();
						MainActivity.this.runOnUiThread( new Runnable() {
					       public void run() {
					    	   new FatalDialog( "Error", "This application can run only on rooted devices.", MainActivity.this ).show();
					       }
					    });
					}		        							                
					else if( mToolsInstaller.needed() && mToolsInstaller.install() == false )
			        {
			        	dialog.dismiss();
						MainActivity.this.runOnUiThread( new Runnable() {
					       public void run() {
					    	   new FatalDialog( "Error", "Error during files installation!", MainActivity.this ).show();
					       }
					    });
			        }
					
					dialog.dismiss();
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
	
		        // register the crash manager
		        CrashManager.register( getApplicationContext() );
		        
		        mNmap 		   = new NMap( this );
		        mListView 	   = ( ListView )findViewById( R.id.listView );
				mTargetAdapter = new TargetAdapter( R.layout.target_list_item );
		    	
				mListView.setAdapter( mTargetAdapter );						
	    	}
	    	catch( Exception e )
	    	{
	    		new FatalDialog( "Error", e.getMessage(), this ).show();
	    	}
        }                        
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		new MenuInflater(this).inflate( R.menu.main, menu );		
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch( item.getItemId() ) 
		{
			case R.id.add:
				
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
				                    	( ( TargetAdapter )mListView.getAdapter() ).notifyDataSetChanged();
				                    }
				                });
							}
						}
						else
							new ErrorDialog( "Error", "Invalid target.", MainActivity.this ).show();
					}} 
				).show();
				
				return true;
	
			case R.id.scan:
				
				try
				{
					final ProgressDialog dialog = ProgressDialog.show( MainActivity.this, "", "Searching alive endpoints ...", true, false );
									
					mNmap.findAliveEndpoints( System.getNetwork(), new FindAliveEndpointsOutputReceiver(){						
						@Override
						public void onEndpointFound( Endpoint endpoint ) {
							Target target = new Target( endpoint );

							if( System.addOrderedTarget( target ) == true )
							{													
								// refresh the target listview
		                    	MainActivity.this.runOnUiThread(new Runnable() {
				                    @Override
				                    public void run() {
				                    	( ( TargetAdapter )mListView.getAdapter() ).notifyDataSetChanged();
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
	
			case R.id.new_session:
								
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
				
			case R.id.save_session :
			case R.id.restore_session :
				
				Toast.makeText( this, "TODO :)", Toast.LENGTH_LONG ).show();
				
				return true;
												
			case R.id.about:
				
				// TODO: Make an About dialog
				Toast.makeText( this, "dSploit - Android Network Penetration Suite\n      by Simone Margaritelli aka evilsocket", Toast.LENGTH_LONG ).show();
				
				return true;
		}

		return super.onOptionsItemSelected(item);
	}

	@Override
	public void onDestroy() {		
		// make sure no zombie process is running before destroying the activity
		System.clean();
		
		super.onDestroy();
	}
}