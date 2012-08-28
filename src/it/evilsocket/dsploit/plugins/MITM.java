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

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.system.Plugin;

public class MITM extends Plugin 
{
	private ListView      	  mActionListView = null;
	private ActionAdapter 	  mActionAdapter  = null;
	private ArrayList<Action> mActions  	  = new ArrayList<Action>();
	
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
			TextView  name;
			TextView  description;
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
                
        mActions.add( new Action( "Simple Sniff", "Only redirect target's traffic through this device, useful when using a network sniffer like 'Sharp' for android.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));
        
        mActions.add( new Action( "Monitor Connections", "Show live connections of your current target.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));
        
        mActions.add( new Action( "Kill Connections", "Kill target connections preventing him to reach any website or server.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));
        
        mActions.add( new Action( "Password & Cookies", "Sniff cookies, http, ftp and web passwords from the target.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));

        mActions.add( new Action( "Traffic Redirect", "Redirect every http request to a specific website.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));
        
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
        
        mActionListView = ( ListView )findViewById( R.id.actionListView );
        mActionAdapter  = new ActionAdapter( R.layout.plugin_mitm_list_item, mActions );
        
        mActionListView.setAdapter( mActionAdapter );
	}
}
