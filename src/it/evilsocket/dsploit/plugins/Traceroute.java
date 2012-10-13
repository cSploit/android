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

import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.ToggleButton;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.tools.NMap.TraceOutputReceiver;

public class Traceroute extends Plugin
{
	private ToggleButton 		  mTraceToggleButton = null;
	private ListView     		  mTraceList		 = null;
	private ProgressBar			  mTraceProgress     = null;
	private boolean      		  mRunning		     = false;
	private ArrayAdapter<String>  mListAdapter 	     = null;
    private Receiver              mTraceReceiver     = null;
	
	private class Receiver extends TraceOutputReceiver
	{
		@Override 
		public void onStart( String commandLine ) {
			super.onStart( commandLine );
			
			Traceroute.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                	mRunning = true;
                	mTraceProgress.setVisibility( View.VISIBLE );
                }
            });							
		}
		
		@Override
		public void onEnd( int exitCode ) {
			super.onEnd(exitCode);
			
			Traceroute.this.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                	setStoppedState();                	                
                }
            });										
		}

		@Override
		public void onHop( final String hop, final String time, final String address ) {
			
			Traceroute.this.runOnUiThread( new Runnable() {
				@Override
                public void run() {
					if( time.equals("...") == false )
						mListAdapter.add( address + " ( " + time + " )" );
					
					else
						mListAdapter.add( address + " untraced hops" );
					
					mListAdapter.notifyDataSetChanged();
                }
            });					
		}
	}
	
	public Traceroute( ) {
		super
		( 
		    "Trace", 
		    "Perform a traceroute on target.", 
		    new Target.Type[]{ Target.Type.ENDPOINT, Target.Type.REMOTE }, 
		    R.layout.plugin_traceroute,
		    R.drawable.action_traceroute_48
		);
		
		mTraceReceiver = new Receiver();
	}
	
	private void setStoppedState( ) {
		System.getNMap().kill();
		mTraceProgress.setVisibility( View.INVISIBLE );
		mRunning = false;
		mTraceToggleButton.setChecked( false );       	
	}
	
	private void setStartedState( ) {
		mListAdapter.clear();
		
		System.getNMap().trace( System.getCurrentTarget(), mTraceReceiver ).start();
		
		mRunning = true;
	}
	
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);    
        
        mTraceToggleButton = ( ToggleButton )findViewById( R.id.traceToggleButton );
		mTraceProgress	  = ( ProgressBar )findViewById( R.id.traceActivity );
		    	
		mTraceToggleButton.setOnClickListener( new OnClickListener(){
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
		
		mTraceList = ( ListView )findViewById( R.id.traceListView );

		mListAdapter = new ArrayAdapter<String>( this, android.R.layout.simple_list_item_1 );
		mTraceList.setAdapter( mListAdapter );
	}
	
	@Override
	public void onBackPressed() {
		setStoppedState();	
	    super.onBackPressed();
	}
}
