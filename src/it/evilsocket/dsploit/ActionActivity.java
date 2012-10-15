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

import com.actionbarsherlock.app.SherlockListActivity;
import com.actionbarsherlock.view.MenuItem;

import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.gui.dialogs.FinishDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class ActionActivity extends SherlockListActivity 
{
	private static final int LAYOUT = R.layout.actions_layout;
	
	private ActionsAdapter    mActionsAdapter = null; 
	private ArrayList<Plugin> mAvailable      = null;
	
	public class ActionsAdapter extends ArrayAdapter<Plugin> 
	{
		private int mLayoutId = 0;
		
		public class ActionHolder
	    {
			ImageView icon;
			TextView  name;
			TextView  description;
	    }
		
		public ActionsAdapter( int layoutId ) {
	        super( ActionActivity.this, layoutId, mAvailable );
	        	       
	        mLayoutId = layoutId;
	    }
		
		@Override
	    public View getView( int position, View convertView, ViewGroup parent ) {		
	        View 		 row    = convertView;
	        ActionHolder holder = null;
	        
	        if( row == null )
	        {
	            LayoutInflater inflater = ( LayoutInflater )ActionActivity.this.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
	            row = inflater.inflate( mLayoutId, parent, false );
	            
	            holder = new ActionHolder();
	            
	            holder.icon        = ( ImageView )row.findViewById( R.id.actionIcon );
	            holder.name        = ( TextView )row.findViewById( R.id.actionName );
	            holder.description = ( TextView )row.findViewById( R.id.actionDescription );

	            row.setTag(holder);
	        }
	        else
	        {
	            holder = ( ActionHolder )row.getTag();
	        }
	        
	        Plugin action = mAvailable.get( position );
	                
	        holder.icon.setImageResource( action.getIconResourceId() );
	        holder.name.setText( action.getName() );
	        holder.description.setText( action.getDescription() );
	        	        
	        return row;
	    }
	}
	
	@Override
    public void onCreate( Bundle savedInstanceState ) {
        super.onCreate(savedInstanceState);        
                        
        if( System.getTargets() != null && System.getTargets().size() > 0 && System.getCurrentTarget() != null )
        {
	        setTitle( "dSploit > " + System.getCurrentTarget() );
	        setContentView( LAYOUT );	        
	        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
	        	        	        
	        mAvailable 		= System.getPluginsForTarget();
	        mActionsAdapter = new ActionsAdapter( R.layout.actions_list_item );
	        
		    setListAdapter( mActionsAdapter );  
        }
        else
        	new FinishDialog( "Warning", "Something went wrong, please select again a valid target.", this ).show();
	}
	
	@Override
	public boolean onOptionsItemSelected( MenuItem item ) 
	{    
		switch( item.getItemId() ) 
		{        
			case android.R.id.home:            
	         
				onBackPressed();
				
				return true;
	    	  
			default:            
				return super.onOptionsItemSelected(item);    
	   }
	}
	
	@Override
	protected void onListItemClick( ListView l, View v, int position, long id ){
		super.onListItemClick( l, v, position, id);
				
		Plugin plugin = mAvailable.get( position );
				
		System.setCurrentPlugin( plugin );
		
		if( plugin.hasLayoutToShow() )
		{
			Toast.makeText( ActionActivity.this, "Selected " + plugin.getName(), Toast.LENGTH_SHORT ).show();
	    				
	    	startActivity
	        ( 
	          new Intent
	          ( 
	            ActionActivity.this, 
	            plugin.getClass()
	          ) 
	        );
	        overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
		}
		else			
			plugin.onActionClick( this );
	}

	@Override
	public void onBackPressed() {
	    super.onBackPressed();
	    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
	}
}
