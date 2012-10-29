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

import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Typeface;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.actionbarsherlock.app.SherlockListActivity;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;
import com.google.android.apps.analytics.GoogleAnalyticsTracker;

public class WifiScannerActivity extends SherlockListActivity
{	
	public static final String CONNECTED = "WifiScannerActivity.CONNECTED";
		
	private WifiManager  mWifiManager  = null;
	private TextView     mStatusText   = null;
	private ProgressBar  mStatus	   = null;
	private ScanReceiver mReceiver	   = null;     
	private ScanAdapter	 mAdapter	   = null;
	private IntentFilter mIntentFilter = null;
	private boolean      mConnected	   = false;
	
	public class ScanAdapter extends ArrayAdapter<ScanResult> 
	{						
		class ResultHolder
	    {
	        ImageView  powerIcon;
	        TextView   ssid;
	        TextView   bssid;
	    }
		
		private ArrayList<ScanResult> mResults = null;
		
		public ScanAdapter() {		
	        super( WifiScannerActivity.this, R.layout.wifi_scanner_list_item );	   
	        
	        mResults = new ArrayList<ScanResult>();
	    }
						
		public void addResult( ScanResult result ) {
			for( ScanResult res : mResults )
			{
				if( res.BSSID.equals( result.BSSID ) )
					return;
			}
			
			mResults.add( result );
			
			Collections.sort( mResults, new Comparator<ScanResult>() {
			    @Override 
			    public int compare( ScanResult lhs, ScanResult rhs ) {
			    	if( lhs.level > rhs.level )
			    		return -1;
			    	
			    	else if( rhs.level > lhs.level )
			    		return 1;
			    	
			    	else 
			    		return 0;
			    } 
			});
		}
		
		public void reset() {
			mResults.clear();
			notifyDataSetChanged();
		}

		@Override
		public int getCount(){
			return mResults.size();
		}
		
		@Override
		public ScanResult getItem( int position ) {
			return mResults.get( position );
		}
		
		public int getImageFromSignal( int level ) {
			level = Math.abs( level );
			
			if( level <= 76 )
				return R.drawable.wifi_high;
			
			else if( level <= 87 )
				return R.drawable.wifi_good;
			
			else if( level <= 98 )
				return R.drawable.wifi_low;
			
			else
				return R.drawable.wifi_bad;
		}
		
		public Bitmap addLogo( Bitmap mainImage, Bitmap logoImage ) { 
		    Bitmap finalImage = null; 
		    int width, height = 0; 
		        
		    width = mainImage.getWidth(); 
		    height = mainImage.getHeight(); 
		    
		    finalImage = Bitmap.createBitmap(width, height, mainImage.getConfig()); 
		    
		    Canvas canvas = new Canvas(finalImage); 
		    
		    canvas.drawBitmap(mainImage, 0,0,null);
		    canvas.drawBitmap(logoImage, canvas.getWidth()-logoImage.getWidth() ,canvas.getHeight()-logoImage.getHeight() ,null);

		    return finalImage; 
		}
		
		@Override
	    public View getView( int position, View convertView, ViewGroup parent ) {		
	        View 		 row    = convertView;
	        ResultHolder holder = null;
	        
	        if( row == null )
	        {
	            LayoutInflater inflater = ( LayoutInflater )WifiScannerActivity.this.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
	            row = inflater.inflate( R.layout.wifi_scanner_list_item, parent, false );
	            
	            holder = new ResultHolder();
	            
	            holder.powerIcon = ( ImageView )row.findViewById( R.id.powerIcon );
	            holder.ssid  	 = ( TextView )row.findViewById( R.id.ssid );
	            holder.bssid     = ( TextView )row.findViewById( R.id.bssid );

	            row.setTag(holder);
	        }
	        else
	        {
	            holder = ( ResultHolder )row.getTag();
	        }
	        
	        ScanResult result = mResults.get( position );

	        Bitmap picture = BitmapFactory.decodeResource( getResources(), getImageFromSignal( result.level ) );
	        
	        if( result.capabilities.contains("WEP") || result.capabilities.contains("WPA") )
	        	picture = addLogo( picture, BitmapFactory.decodeResource( getResources(), R.drawable.wifi_locked ) );
	        	        	        	      
	        holder.powerIcon.setImageBitmap( picture );
	        holder.ssid.setTypeface( null, Typeface.BOLD );
	        holder.ssid.setText( result.SSID );
	        
	        String protection = "<b>Open</b>";
	        
	        List<String> capabilities = Arrays.asList( result.capabilities.split( "[\\-\\[\\]]" ) );

	        if( capabilities.contains("WEP") )
	        	protection = "<b>WEP</b>";
	        
	        else if( capabilities.contains("WPA2") )
	        	protection = "<b>WPA2</b>";
	        
	        else if( capabilities.contains("WPA") )
	        	protection = "<b>WPA</b>";
	        
	        if( capabilities.contains( "PSK" ) )
	        	protection += " PSK";
	        
	        if( capabilities.contains("WPS") )
	        	protection += " ( WPS )";
	        	        
	        holder.bssid.setText
	        ( 
	          Html.fromHtml
	          (
	            result.BSSID + " " + protection + " <small>( " + ( Math.round( ( result.frequency / 1000.0 ) * 10.0 ) / 10.0 ) + " Ghz )</small>"
	          ) 
	        );
	        
	        return row;
	    }
	}

	private class ScanReceiver extends BroadcastReceiver
	{
		public ScanReceiver() {
			super();
		}
		
		@Override
		public void onReceive( Context context, Intent intent ) {
			
			if( intent.getAction().equals( WifiManager.SUPPLICANT_STATE_CHANGED_ACTION ) )
			{
				SupplicantState state    = intent.getParcelableExtra( WifiManager.EXTRA_NEW_STATE );
				boolean			hasError = intent.hasExtra( WifiManager.EXTRA_SUPPLICANT_ERROR );
				int				error    = intent.getIntExtra( WifiManager.EXTRA_SUPPLICANT_ERROR, 0 );
				
				if( SupplicantState.FOUR_WAY_HANDSHAKE.equals(state) )
				{
					mStatusText.setText( "Authenticating ..." );
					mStatus.setVisibility( View.VISIBLE ); 
				}				
				else if( hasError && error == WifiManager.ERROR_AUTHENTICATING )
				{
					mStatusText.setText( Html.fromHtml( "<font color=\"red\">Authentication Error.</font>" ) );
					mStatus.setVisibility( View.INVISIBLE ); 
				}
			}
			else if( intent.getAction().equals( WifiManager.NETWORK_STATE_CHANGED_ACTION ) )
			{
				NetworkInfo info = intent.getParcelableExtra( WifiManager.EXTRA_NETWORK_INFO );
				
				if( NetworkInfo.State.CONNECTING.equals( info.getState() ) )
				{
					mStatusText.setText( "Connecting ..." );
					mStatus.setVisibility( View.VISIBLE ); 
				}
				else if( NetworkInfo.State.CONNECTED.equals( info.getState() ) )
				{					
					Toast.makeText( WifiScannerActivity.this, "Connected to " + mWifiManager.getConnectionInfo().getSSID(), Toast.LENGTH_SHORT ).show();
					mStatusText.setText( Html.fromHtml( "Connected to <b>" + mWifiManager.getConnectionInfo().getSSID() + "</b>" ) );
					mStatus.setVisibility( View.INVISIBLE ); 
					mConnected = true;
				}
				else if( NetworkInfo.State.DISCONNECTED.equals( info.getState() ) )
				{
					mStatusText.setText( "Disconnected." );
					mStatus.setVisibility( View.INVISIBLE ); 
					mConnected = false;
				}
			}
			else if( intent.getAction().equals( WifiManager.SCAN_RESULTS_AVAILABLE_ACTION ) )
			{
				List<ScanResult> results = mWifiManager.getScanResults();
				
				for( ScanResult result : results )
				{
					mAdapter.addResult( result );
				}
				
				mStatusText.setText( "Scanning finished." );
		        mStatus.setVisibility( View.INVISIBLE ); 	
				
		        mAdapter.notifyDataSetChanged();
			}
		}		
	}
	
	@Override
    public void onCreate( Bundle savedInstanceState ) {		
        super.onCreate(savedInstanceState);           
        setContentView( R.layout.wifi_scanner ); 
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        
        GoogleAnalyticsTracker.getInstance().trackPageView("/wifi-scanner");
        
        mWifiManager = ( WifiManager )this.getSystemService( Context.WIFI_SERVICE );
        mReceiver	 = new ScanReceiver();
        mStatusText	 = ( TextView )findViewById( R.id.statusText );
        mStatus		 = ( ProgressBar )findViewById( R.id.status );
        mAdapter	 = new ScanAdapter();
        
        getListView().setAdapter( mAdapter );
        
        mStatusText.setText( "Initializing ..." );
        
        if( mWifiManager.isWifiEnabled() == false )
        {
        	mStatusText.setText( "Activating WiFi interface ..." );
			mWifiManager.setWifiEnabled( true );
			mStatusText.setText( "WiFi activated." );						
        }        
                
        mIntentFilter = new IntentFilter();
        
	    mIntentFilter.addAction( WifiManager.SCAN_RESULTS_AVAILABLE_ACTION );
	    mIntentFilter.addAction( WifiManager.NETWORK_STATE_CHANGED_ACTION );
	    mIntentFilter.addAction( WifiManager.SUPPLICANT_STATE_CHANGED_ACTION );
	    
	    registerReceiver( mReceiver, mIntentFilter );   
	            
	    mStatusText.setText( "Scanning ..." );
		mStatus.setVisibility( View.VISIBLE ); 

	    mWifiManager.startScan();	
	}
	
	@Override
	protected void onListItemClick( ListView l, View v, int position, long id ){
		super.onListItemClick( l, v, position, id);
						
		ScanResult result = mAdapter.getItem( position );
		if( result != null )
		{			
			final WifiConfiguration config = new WifiConfiguration();
			
			config.SSID = "\"" + result.SSID + "\"";
			
			if( result.capabilities.contains("WEP") )
			{
				new InputDialog( "Connect", "WEP Key:", null, true, true, this, new InputDialogListener() {					
					@Override
					public void onInputEntered( String input ) {
						config.wepKeys[0]    = "\"" + input + "\""; 
						config.wepTxKeyIndex = 0;
						config.status        = WifiConfiguration.Status.ENABLED; 
						config.allowedKeyManagement.set( WifiConfiguration.KeyMgmt.NONE );
						config.allowedGroupCiphers.set( WifiConfiguration.GroupCipher.WEP40 ); 
						
						int idx = mWifiManager.addNetwork( config );
						
						mWifiManager.disconnect();
						mWifiManager.enableNetwork( idx, true );
						mWifiManager.reconnect();                						   
						mWifiManager.setWifiEnabled(true);												
					}
				}).show();
			}
			else if( result.capabilities.contains("WPA") )
			{
				new InputDialog( "Connect", "WPA Key:", null, true, true, this, new InputDialogListener() {					
					@Override
					public void onInputEntered( String input ) {
						config.preSharedKey = "\""+ input +"\"";
						
						int idx = mWifiManager.addNetwork( config );
						
						mWifiManager.disconnect();
						mWifiManager.enableNetwork( idx, true );
						mWifiManager.reconnect();                						   
						mWifiManager.setWifiEnabled(true);												
					}
				}).show();
			}
			else
			{
				config.allowedKeyManagement.set( WifiConfiguration.KeyMgmt.NONE );
				
				int idx = mWifiManager.addNetwork( config );
				
				mWifiManager.disconnect();
				mWifiManager.enableNetwork( idx, true );
				mWifiManager.reconnect();                						   
				mWifiManager.setWifiEnabled(true);	
			}
		}
	}
		
	@Override
	public boolean onCreateOptionsMenu( Menu menu ) {
		MenuInflater inflater = getSupportMenuInflater();		
		inflater.inflate( R.menu.wifi_scanner, menu );		
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected( MenuItem item ) {
		if( item.getItemId() == R.id.scan )
		{
			mAdapter.reset();
			
			mWifiManager.startScan();
	        
	        mStatusText.setText( "Scanning ..." );
	        mStatus.setVisibility( View.VISIBLE ); 	

			return true;
		}
		if( item.getItemId() == android.R.id.home )
		{
			onBackPressed();
			
			return true;
		}
		else
			return super.onOptionsItemSelected(item);
	}
	
	@Override
	public void onBackPressed() {
		unregisterReceiver( mReceiver );	
		
		Bundle bundle = new Bundle();
	    bundle.putBoolean( CONNECTED, mConnected );

	    Intent intent = new Intent();
	    intent.putExtras(bundle);
	    
	    setResult( RESULT_OK, intent );
	    	    	    
	    super.onBackPressed();
	    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
	}
}
