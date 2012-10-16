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

import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.view.MenuItem;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.tools.Ettercap.OnReadyListener;
import android.content.Context;
import android.os.Bundle;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ToggleButton;

public class Sniffer extends SherlockActivity
{
	private static final String  TAG		 = "SNIFFER";
	private static final String  PCAP_FILTER = "not '(src host localhost or dst host localhost or arp)'";
	private static final Pattern PARSER 	 = Pattern.compile( "^.+length\\s+(\\d+)\\)\\s+([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})\\.[^\\s]+\\s+>\\s+([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})\\.[^\\:]+:.+", Pattern.CASE_INSENSITIVE );

	private ToggleButton 	   mSniffToggleButton = null;
	private ProgressBar	 	   mSniffProgress     = null;
	private ListView 		   mListView    	  = null;		
	private StatListAdapter	   mAdapter			  = null;
	private boolean	     	   mRunning			  = false;	
	private double			   mSampleTime		  = 1.0;
	
	public static class AddressStats
	{
		public String mAddress     = "";
		public int 	  mPackets	   = 0;
		public double mBandwidth   = 0;
		public long   mSampledTime = 0;
		public int    mSampledSize = 0;
		
		public AddressStats( String address )
		{
			mAddress     = address;
			mPackets     = 0;
			mBandwidth   = 0;
			mSampledTime = 0;
			mSampledSize = 0;
		}
	}

	public class StatListAdapter extends ArrayAdapter<AddressStats> 
	{
		private int 							mLayoutId = 0;
		private HashMap< String, AddressStats > mStats 	  = null;

		public class StatsHolder
	    {
			TextView  address;
			TextView  description;
	    }
		
		public StatListAdapter( int layoutId ) {
	        super( Sniffer.this, layoutId );
	        	       
	        mLayoutId = layoutId;
	        mStats    = new HashMap< String, AddressStats >();
	    }

		public AddressStats getStats( String address ) {
			return mStats.get( address );
		}
		
		public synchronized void addStats( AddressStats stats ) {			
			mStats.put( stats.mAddress, stats );
		}
						
		private synchronized AddressStats getByPosition( int position ) {
			return mStats.get( mStats.keySet().toArray()[ position ] );
		}
				
		@Override
		public int getCount(){
			return mStats.size();
		}
		
		private String formatSize( int size ) {			
			if( size < 1024 )
				return size + " B";
			
			else if( size < ( 1024 * 1024 ) )
				return ( size / 1024 ) + " KB";
			
			else if( size < ( 1024 * 1024 * 1024 ) )
				return ( size / ( 1024 * 1024 ) ) + " MB";
			
			else 
				return ( size / ( 1024 * 1024 * 1024 ) ) + " GB";
		}
		
		private String formatSpeed( int speed )
		{
			if( speed < 1024 )
				return speed + " B/s";
			
			else if( speed < ( 1024 * 1024 ) )
				return ( speed / 1024 ) + " KB/s";
			
			else if( speed < ( 1024 * 1024 * 1024 ) )
				return ( speed / ( 1024 * 1024 ) ) + " MB/s";
			
			else 
				return ( speed / ( 1024 * 1024 * 1024 ) ) + " GB/s";
		}
		
		@Override
	    public View getView( int position, View convertView, ViewGroup parent ) {				
	        View 		row    = convertView;
	        StatsHolder holder = null;
	        
	        if( row == null )
	        {
	            LayoutInflater inflater = ( LayoutInflater )Sniffer.this.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
	            row = inflater.inflate( mLayoutId, parent, false );
	            
	            holder = new StatsHolder();
	            
	            holder.address     = ( TextView )row.findViewById( R.id.statAddress );
	            holder.description = ( TextView )row.findViewById( R.id.statDescription );

	            row.setTag( holder );
	        }
	        else
	        {
	            holder = ( StatsHolder )row.getTag();
	        }
	        
	        AddressStats stats = getByPosition( position );
	        
	        holder.address.setText( stats.mAddress );
	        holder.description.setText
	        ( 
	          Html.fromHtml
	          (  
	            "<b>BANDWIDTH</b>: " + formatSpeed( (int)stats.mBandwidth ) + " | <b>TOTAL</b> " + formatSize( stats.mPackets ) 
	          )
	        );  

	        return row;
	    }
	}
	
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);        
        setTitle( System.getCurrentTarget() + " > MITM > Sniffer" );
        setContentView( R.layout.plugin_mitm_sniffer );
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        
        mSniffToggleButton = ( ToggleButton )findViewById( R.id.sniffToggleButton );
        mSniffProgress	   = ( ProgressBar )findViewById( R.id.sniffActivity );
        mListView 		   = ( ListView )findViewById( R.id.listView );
        mAdapter		   = new StatListAdapter( R.layout.plugin_mitm_sniffer_list_item );
        mSampleTime		   = Double.parseDouble( System.getSettings().getString( "PREF_SNIFFER_SAMPLE_TIME", "1.0" ) );
        
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

	private void setStoppedState( ) {		
		System.getEttercap().kill();
		System.getTcpDump().kill();

		System.setForwarding( false );
		
		mSniffProgress.setVisibility( View.INVISIBLE );
		
		mRunning = false;
		mSniffToggleButton.setChecked( false );                			
	}

	private void setStartedState( ) {		
		// never say never :)
		System.getEttercap().kill();
		System.getTcpDump().kill();
					
		System.getEttercap().spoof( System.getCurrentTarget(), new OnReadyListener(){
			@Override
			public void onReady() { 
				System.setForwarding( true );

				System.getTcpDump().sniff( PCAP_FILTER, new OutputReceiver(){
					@Override
					public void onStart(String command) { }

					@Override
					public void onNewLine( String line ) {
						try
						{
							Matcher matcher = PARSER.matcher( line.trim() );
							
							if( matcher != null && matcher.find() )
							{								
								String 		 length   = matcher.group( 1 ),
											 source   = matcher.group( 2 ),
									   		 dest     = matcher.group( 3 );							
								int    		 ilength  = Integer.parseInt( length );
								long		 now	  = java.lang.System.currentTimeMillis();
								double		 deltat	  = 0.0;
								AddressStats stats 	  = null;
								
								if( System.getNetwork().isInternal( source ) ) 
								{
									stats 	 = mAdapter.getStats( source );
								}
								else if( System.getNetwork().isInternal( dest ) ) 
								{
									source   = dest;
									stats 	 = mAdapter.getStats( dest );
								}
								
								if( stats == null )
								{
									stats 		   = new AddressStats( source );
									stats.mPackets = ilength;
									stats.mSampledTime = now;							
								}
								else
								{
									stats.mPackets += ilength;
									
									deltat = ( now - stats.mSampledTime ) / 1000.0;
									
									if( deltat >= mSampleTime )
									{										
										stats.mBandwidth   = ( stats.mPackets - stats.mSampledSize ) / deltat;
										stats.mSampledTime = java.lang.System.currentTimeMillis();
										stats.mSampledSize = stats.mPackets;
									}									
								}
								
								final AddressStats fstats = stats;
								Sniffer.this.runOnUiThread( new Runnable() {
									@Override
									public void run(){
										mAdapter.addStats( fstats );
										mAdapter.notifyDataSetChanged();
									}							
								});			
							}
						}
						catch( Exception e )
						{
							System.errorLogging( TAG, e );
						}
					}

					@Override
					public void onEnd(int exitCode) { }
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