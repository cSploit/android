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
package it.evilsocket.dsploit.plugins.mitm;

import java.util.ArrayList;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.system.Environment;
import it.evilsocket.dsploit.system.Plugin;
import it.evilsocket.dsploit.system.Shell.OutputReceiver;
import it.evilsocket.dsploit.tools.ArpSpoof;

public class MITM extends Plugin 
{
	private ListView      	  mActionListView = null;
	private ActionAdapter 	  mActionAdapter  = null;
	private ArrayList<Action> mActions  	  = new ArrayList<Action>();	
	private ArpSpoof		  mArpSpoof		  = null;
	
	class Action
	{
		public String 		   name;
		public String 		   description;
		public OnClickListener listener;
		
		public Action( String name, String description, OnClickListener listener )
		{
			this.name 		 = name;
			this.description = description;
			this.listener 	 = listener;
		}
	}
	
	class ActionAdapter extends ArrayAdapter<Action> 
	{
		private int 			  mLayoutId = 0;
		private ArrayList<Action> mActions;
		
		class ActionHolder
		{		
			TextView    name;
			TextView    description;
			ProgressBar activity;
		}
		
		public ActionAdapter( int layoutId, ArrayList<Action> actions ) {
	        super( MITM.this, layoutId, actions );
	        	       
	        mLayoutId = layoutId;
	        mActions  = actions;
	    }
		
		@Override
	    public View getView( int position, View convertView, ViewGroup parent ) {		
	        View 		 row    = convertView;
	        ActionHolder holder = null;
	        
	        if( row == null )
	        {
	            LayoutInflater inflater = ( LayoutInflater )MITM.this.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
	            row = inflater.inflate( mLayoutId, parent, false );
	            
	            holder = new ActionHolder();
	            
	            holder.name        = ( TextView )row.findViewById( R.id.itemName );
	            holder.description = ( TextView )row.findViewById( R.id.itemDescription );
	            holder.activity    = ( ProgressBar )row.findViewById( R.id.itemActivity );
	            
	            row.setTag(holder);
	        }
	        else
	        {
	            holder = ( ActionHolder )row.getTag();
	        }
	        
	        Action action = mActions.get( position );

	        holder.name.setText( action.name );
	        holder.description.setText( action.description );
	        
	        row.setOnClickListener( action.listener );
	        	        
	        return row;
	    }
	}
	
	public MITM( ) {
		super
		( 
		    "MITM", 
		    "Perform various man-in-the-middle attacks, such as network sniffing, traffic manipulation, etc.", 
		    new Target.Type[]{ Target.Type.ENDPOINT, Target.Type.NETWORK }, 
		    R.layout.plugin_mitm,
		    R.drawable.action_mitm_48 
	    );		
	}

	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);   
        
        mArpSpoof = new ArpSpoof( this );
        
        mActions.add( new Action( "Simple Sniff", "Only redirect target's traffic through this device, useful when using a network sniffer like 'Sharp' for android.", new OnClickListener(){
			@Override
			public void onClick( View v ) 
			{
				startActivity
                ( 
                  new Intent
                  ( 
                	MITM.this, 
                	Sniffer.class
                  ) 
                );
                overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
			}
		}));
        
        mActions.add( new Action( "Kill Connections", "Kill connections preventing everyone to reach any website or server.", new OnClickListener(){
			@Override
			public void onClick(View v) {
				ProgressBar activity = ( ProgressBar )v.findViewById( R.id.itemActivity );

				if( activity.getVisibility() == View.INVISIBLE )
				{
					activity.setVisibility( View.VISIBLE );
					
					Toast.makeText( MITM.this, "Tap again to stop.", Toast.LENGTH_LONG ).show();

					Environment.setForwarding( false );

					OutputReceiver receiver = new OutputReceiver(){
						@Override
						public void onStart(String command) { }

						@Override
						public void onNewLine( String line ) {
							// arpspoof sometimes goes segfault, restart it just in case :P
							if( line.trim().toLowerCase().contains("segmentation fault") )
							{
								Log.w( "ARPSPOOF", "Restarting arpspoof after SEGFAULT" );
								mArpSpoof.kill();
								mArpSpoof.spoof( Environment.getTarget(), this );
							}
						}

						@Override
						public void onEnd(int exitCode) { }
					};
					
					mArpSpoof.spoof( Environment.getTarget(), receiver ).start();											
				}
				else
				{
	
					Environment.setForwarding( true );
					mArpSpoof.kill();
								
					activity.setVisibility( View.INVISIBLE );
				}
			}
		}));
        
        mActions.add( new Action( "Password & Cookies", "Sniff cookies, http, ftp and web passwords from the target.", new OnClickListener(){
        	@Override
			public void onClick( View v ) 
			{
				startActivity
                ( 
                  new Intent
                  ( 
                	MITM.this, 
                	PasswordSniffer.class
                  ) 
                );
                overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
			}
		}));

        mActions.add( new Action( "Traffic Redirect", "Redirect every http request to a specific website.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));
/*        
 TODO:
 
        mActions.add( new Action( "Replace Images", "Replace all images on webpages with the specified one.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));
        
        mActions.add( new Action( "Custom Replace", "Replace custom text on webpages with the specified one.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));
*/        
        mActionListView = ( ListView )findViewById( R.id.actionListView );
        mActionAdapter  = new ActionAdapter( R.layout.plugin_mitm_list_item, mActions );
        
        mActionListView.setAdapter( mActionAdapter );
	}
}
