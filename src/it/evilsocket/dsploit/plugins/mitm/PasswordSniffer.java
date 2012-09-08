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

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.dialogs.FatalDialog;
import it.evilsocket.dsploit.net.Stream;
import it.evilsocket.dsploit.tools.Ettercap.OnReadyListener;
import it.evilsocket.dsploit.tools.TcpDump.PasswordReceiver;
import android.app.Activity;
import android.content.Context;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.CheckBox;
import android.widget.ExpandableListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

public class PasswordSniffer extends Activity 
{
	private static final String TAG			= "PASSWORDSNIFFER";
	private static final String PCAP_FILTER = "not '(src host localhost or dst host localhost or arp)'";
	private static final String FILENAME    = "dsploit-password-sniff.log";
	
	private ToggleButton       mSniffToggleButton = null;
	private CheckBox		   mSniffCookies	  = null;
	private ProgressBar	       mSniffProgress     = null;
	private ExpandableListView mListView		  = null;		
	private ListViewAdapter	   mAdapter			  = null;
	private boolean	           mRunning			  = false;	
	private String			   mExternalStorage   = null;
	private String			   mFileOutput		  = null;
	private FileWriter		   mFileWriter		  = null;
	private BufferedWriter	   mBufferedWriter	  = null;
	
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
        
        mExternalStorage   = Environment.getExternalStorageDirectory().toString();
        mFileOutput		   = ( new File( mExternalStorage, FILENAME ) ).getAbsolutePath();
        mSniffToggleButton = ( ToggleButton )findViewById( R.id.sniffToggleButton );
        mSniffCookies	   = ( CheckBox )findViewById( R.id.cookieCheckBox );
        mSniffProgress	   = ( ProgressBar )findViewById( R.id.sniffActivity );
        mListView		   = ( ExpandableListView )findViewById( R.id.listView );
        mAdapter		   = new ListViewAdapter( this );
        
        mListView.setAdapter( mAdapter );
                
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
		System.getEttercap().kill();
		System.getTcpDump().kill();

		try
		{
			if( mBufferedWriter != null )
				mBufferedWriter.close();
		}
		catch( IOException e )
		{
			Log.e( TAG, e.toString() );
		}

		System.setForwarding( false );
		
		mSniffProgress.setVisibility( View.INVISIBLE );
		
		mRunning = false;
		mSniffToggleButton.setChecked( false );                			
	}

	private void setStartedState( ) {		
		// never say never :)
		System.getEttercap().kill();
		System.getTcpDump().kill();
		
		try
		{
			// open file in appending mode
			mFileWriter     = new FileWriter( mFileOutput, true );
			mBufferedWriter = new BufferedWriter( mFileWriter );
		}
		catch( IOException e )
		{
			new FatalDialog( "Error", e.toString(), PasswordSniffer.this ).show();
		}
		
		Toast.makeText( PasswordSniffer.this, "Logging to " + mFileOutput, Toast.LENGTH_LONG ).show();
				
		System.getEttercap().spoof( System.getCurrentTarget(), new OnReadyListener(){
			@Override
			public void onReady() {
				System.setForwarding( true );

				System.getTcpDump().sniffPasswords( PCAP_FILTER, new PasswordReceiver(){
					@Override
					public void onAccountFound( final Stream stream, final String data ) {
						
						PasswordSniffer.this.runOnUiThread( new Runnable() {
							@Override
							public void run()
							{	
								boolean bSniffCookies = mSniffCookies.isChecked();
								
								if( bSniffCookies == false && data.startsWith("COOKIES") )
									return;
								
								try
								{
									mBufferedWriter.write( stream.endpoint.toString() + " :\n" + data + "\n\n" );
								}
								catch( IOException e )
								{
									Log.e( TAG, e.toString() );
								}
								
								mAdapter.addChild( stream.endpoint.toString(), data );
							}							
						});				
					}
					
				}).start();
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