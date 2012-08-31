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

import java.util.ArrayList;

import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.FatalDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.net.Endpoint;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.plugins.LoginCracker;
import it.evilsocket.dsploit.plugins.PortScanner;
import it.evilsocket.dsploit.plugins.mitm.MITM;
import it.evilsocket.dsploit.system.Environment;
import it.evilsocket.dsploit.system.Shell;
import it.evilsocket.dsploit.system.ToolsInstaller;
import it.evilsocket.dsploit.tools.NMap;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
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
	
	public class TargetAdapter extends ArrayAdapter<Target> 
	{
		private int               mLayoutId 	 = 0;
		private ArrayList<Target> mTargets  	 = null;
		private boolean			  mSubnetScanned = false;
		
		class TargetHolder
	    {
			Target		target;
	        ImageView   itemImage;
	        TextView    itemTitle;
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
				final Target target = ( Target )v.getTag();
				
				if( target.getType() == Target.Type.NETWORK ) {
					final ProgressDialog dialog = ProgressDialog.show( MainActivity.this, "", "Scanning alive endpoints ...", true, false );
					final NMap 		     nmap   = new NMap( Environment.getContext() );
					
					new Thread(new Runnable() {
			            public void run() {		            	
			            	ArrayList<Endpoint> endpoints = nmap.findAliveEndpoints( target.getNetwork() );
			            	
			            	for( Endpoint e : endpoints )
			            	{
			            		Target target = new Target( e );
			            		
			            		if( hasTarget(target) == false )
			            			mTargets.add( 1, target );
			            	}
			            			         
			            	dialog.dismiss();
			            	
	                    	mSubnetScanned = true;
	                    	
			            	// refresh the target listview
	                    	MainActivity.this.runOnUiThread(new Runnable() {
			                    @Override
			                    public void run() {
			                    	notifyDataSetChanged();
			                    }
			                });
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
		            			mTargets.add( 1, target );
		            			
		            			// refresh the target listview
		            			MainActivity.this.runOnUiThread(new Runnable() {
				                    @Override
				                    public void run() {
				                    	notifyDataSetChanged();
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
				
				Environment.setTarget( target );
					
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
	            
	            holder.itemImage  = ( ImageView )row.findViewById( R.id.itemIcon );
	            holder.itemTitle  = ( TextView )row.findViewById( R.id.itemTitle );
	            holder.scanButton = ( Button )row.findViewById( R.id.scanButton );

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
	        	
	        	// hide the scan button in the network item if already scanned or in other items
	        	if( ( mSubnetScanned && target.getType() == Target.Type.NETWORK ) || target.getType() != Target.Type.NETWORK ) 
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
	        	holder.itemTitle.setTypeface( null, Typeface.BOLD );
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
       
        Log.d( "MAIN", "onCreate" );
        
        new Thread( new Runnable(){
			@Override
			public void run() 
			{						
		        mToolsInstaller = new ToolsInstaller( MainActivity.this.getApplicationContext() );
		       
				if( Shell.isRootGranted() == false )  
				{
					MainActivity.this.runOnUiThread( new Runnable() {
				       public void run() {
				    	   new FatalDialog( "Error", "This application can run only on rooted devices.", MainActivity.this ).show();
				       }
				    });
				}		        							                
		        else if( mToolsInstaller.needed() && mToolsInstaller.install() == false )
		        {
					MainActivity.this.runOnUiThread( new Runnable() {
				       public void run() {
				    	   new FatalDialog( "Error", "Error during files installation!", MainActivity.this ).show();
				       }
				    });
		        }
			}
		}).start();
        
        Environment.init( getApplicationContext() );
        
        // TODO: Implement automatic loading
        Environment.registerPlugin( new PortScanner( ) );
        Environment.registerPlugin( new MITM( ) );
        Environment.registerPlugin( new LoginCracker( ) );
        
        mListView = ( ListView )findViewById( R.id.listView );
	    
	    try
    	{
    		ArrayList<Target> targets = new ArrayList<Target>();
    		
    		targets.add( new Target( Environment.getNetwork() ) );
    		targets.add( new Target( Environment.getNetwork().getLoacalAddress(), null ) );
    		targets.add( null );
    		    		
    		mTargetAdapter = new TargetAdapter( R.layout.target_list_item, targets );
	    	
    		mListView.setAdapter( mTargetAdapter );
    	}
    	catch( Exception e )
    	{
    		new FatalDialog( "Error", e.getMessage(), this ).show();
    	}
	}
	
	
	@Override
	public void onDestroy() {		
		// make sure no zombie process is running before destroying the activity
		Environment.clean();
		
		super.onDestroy();
	}
}