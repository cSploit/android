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

import java.net.SocketException;
import java.util.ArrayList;

import it.evilsocket.dsploit.core.CrashManager;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.ToolsInstaller;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.FatalDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.net.Endpoint;
import it.evilsocket.dsploit.net.Network;
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
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
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
		private int               mLayoutId = 0;
		private ArrayList<Target> mTargets  = null;
		
		class TargetHolder
	    {
			Target		target;
	        ImageView   itemImage;
	        TextView    itemTitle;
	        TextView	itemDescription;
	        Button      scanButton;
	    }
		
		public TargetAdapter( int layoutId, ArrayList<Target> targets ) {		
	        super( MainActivity.this, layoutId, targets );
	        
	        mLayoutId = layoutId;
	        mTargets  = targets;                
	    }
		
		private boolean hasTarget( Target target ) {
			for( Target t : mTargets )
			{
				if( t != null && t.equals(target) )
					return true;
			}
			
			return false;
		}
		
		private class TargetScanClickListener implements OnClickListener 
		{					
			public void onClick( View v ) {
				Target 				 target = ( Target )v.getTag();
				final ProgressDialog dialog = ProgressDialog.show( MainActivity.this, "", "Searching alive endpoints ...", true, false );
				
				if( target.getType() == Target.Type.NETWORK ) {
					
					mNmap.findAliveEndpoints( target.getNetwork(), new FindAliveEndpointsOutputReceiver(){						
						@Override
						public void onEndpointFound( Endpoint endpoint ) {
							Target  target = new Target( endpoint );
							int     i;
							boolean added = false;
							
							if( hasTarget( target ) == false )
							{
								for( i = 0; i < mTargets.size() && !added; i++ )
								{
									if( mTargets.get( i ) != null && mTargets.get( i ).comesAfter( target ) )
									{
										mTargets.add( i, target );
										added = true;
									}
								}
								
								if( !added )								
									mTargets.add( mTargets.size() - 1, target );
																
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
			}
		}
		
		private class AddCustomTargetClickListener implements OnClickListener 
		{
			public void onClick( View v ) 
			{		
				new InputDialog( "Add custom target", "Enter an URL, host name or ip address below:", MainActivity.this, new InputDialogListener(){
					@Override
					public void onInputEntered( String input ) 
					{
						Target target = Target.getFromString(input);						
						if( target != null )
						{
							if( hasTarget(target) == false )
							{								
								boolean added = false;
								
								for( int i = 0; i < mTargets.size() && !added; i++ )
								{
									if( mTargets.get( i ) != null && mTargets.get( i ).comesAfter( target ) )
									{
										mTargets.add( i, target );
										added = true;
									}
								}
								
								if( !added )								
									mTargets.add( mTargets.size() - 1, target );
								
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
			}
		}
		
		private class SelectTargetClickListener implements OnClickListener
		{
			public void onClick( View v ) 
			{		
				// select clicked target and go to action list activity
				final Target target = ( ( TargetHolder )v.getTag() ).target;
	
				System.setTarget( target );
				
				MainActivity.this.runOnUiThread( new Runnable() {
	                @Override
	                public void run() {
	                	Toast.makeText( MainActivity.this, "Selected " + target, Toast.LENGTH_SHORT ).show();	                	
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
	            holder.scanButton 	   = ( Button )row.findViewById( R.id.scanButton );

	            row.setTag(holder);
	        }
	        else
	        {
	            holder = ( TargetHolder )row.getTag();
	        }
	        
	        Target target = mTargets.get( position );
	        
	        if( target != null )
	        {
	        	holder.itemImage.setImageResource( target.getDrawableResourceId() );
	        	holder.itemTitle.setText( target.toString() );
	        	holder.itemTitle.setTypeface( null, Typeface.NORMAL );
	        	holder.itemDescription.setText( target.getDescription() );
	        	
	        	// hide the scan button in the network item if already scanned or in other items
	        	if( target.getType() != Target.Type.NETWORK ) 
	        	{
	        		holder.scanButton.setVisibility( View.GONE );
	        	}
	        	else
	        	{
	        		holder.scanButton.setTag( target );
	        		holder.scanButton.setOnClickListener( new TargetScanClickListener() );
	        	}
	        	
	        	row.setOnClickListener( new SelectTargetClickListener() );
	        }
	        // last line to add a custom target
	        else
	        {
	        	holder.itemImage.setImageResource( R.drawable.target_add_48 );
	        	holder.itemTitle.setText( "Add custom target" );
	        	holder.itemDescription.setText( "Add any hostname or address to the list." );
	        	holder.scanButton.setVisibility( View.GONE );
	        	
	        	row.setOnClickListener( new AddCustomTargetClickListener() );
	        }
	        
	        holder.target = target;
	        
	        return row;
	    }
	}

	@Override
    public void onCreate( Bundle savedInstanceState ) {
        super.onCreate(savedInstanceState);        
        setContentView( LAYOUT );
        
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
        
        System.init( getApplicationContext() );
        
        System.registerPlugin( new PortScanner( ) );
        System.registerPlugin( new Inspector( ) );
        System.registerPlugin( new ExploitFinder( ) );
        System.registerPlugin( new MITM( ) );
        System.registerPlugin( new LoginCracker( ) );
        
        // register the crash manager
        CrashManager.register( getApplicationContext() );
        
        mNmap	  = new NMap( this );
        mListView = ( ListView )findViewById( R.id.listView );
	    
	    try
    	{	    	
	    	ArrayList<Target> targets = new ArrayList<Target>();
			
			targets.add( new Target( System.getNetwork() ) );
			targets.add( new Target( System.getNetwork().getGatewayAddress(), System.getNetwork().getGatewayHardware() ) );
			targets.add( new Target( System.getNetwork().getLoacalAddress(), System.getNetwork().getLocalHardware() ) );
			targets.add( null );

			mTargetAdapter = new TargetAdapter( R.layout.target_list_item, targets );
	    	
			mListView.setAdapter( mTargetAdapter );  	
    	}
    	catch( SocketException e )
    	{
    		new FatalDialog( "Error", e.getMessage(), this ).show();
    	}
	}

	@Override
	public void onDestroy() {		
		// make sure no zombie process is running before destroying the activity
		System.clean();
		
		super.onDestroy();
	}
}