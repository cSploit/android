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

import java.util.List;

import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.actionbarsherlock.app.SherlockListActivity;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;

public class WifiScannerActivity extends SherlockListActivity
{
	private static final String TAG = "WIFISCANNER";
	
	private WifiManager  mWifiManager = null;
	private TextView     mStatusText  = null;
	private ScanReceiver mReceiver	  = null;     
	
	private class ScanReceiver extends BroadcastReceiver
	{
		public ScanReceiver() {
			super();
		}
		
		@Override
		public void onReceive( Context context, Intent intent ) {
			List<ScanResult> results = mWifiManager.getScanResults();
			
			for( ScanResult result : results )
			{
				Log.d( TAG, result.SSID );
			}
		}		
	}
	
	@Override
    public void onCreate( Bundle savedInstanceState ) {		
        super.onCreate(savedInstanceState);           
        setContentView( R.layout.wifi_scanner ); 
                
        mWifiManager = ( WifiManager )this.getSystemService( Context.WIFI_SERVICE );
        mReceiver	 = new ScanReceiver();
        mStatusText	 = ( TextView )findViewById( R.id.statusText );

        mStatusText.setText( "Initializing ..." );
        
        if( mWifiManager.isWifiEnabled() == false )
        {
        	new ConfirmDialog( "Enable WiFi", "Enable WiFi ?", this, new ConfirmDialogListener() {				
				@Override
				public void onConfirm() 
				{
					mStatusText.setText( "Activating WiFi interface ..." );
					mWifiManager.setWifiEnabled( true );
					mStatusText.setText( "WiFi activated." );
					
					mWifiManager.startScan();
			        
			        mStatusText.setText( "Scanning ..." );
				}
				
				@Override
				public void onCancel() { 
					WifiScannerActivity.this.finish();
				}
			}).show();        	
        }        
        
        registerReceiver( mReceiver, new IntentFilter( WifiManager.SCAN_RESULTS_AVAILABLE_ACTION ) );                
	}
	
	@Override
	public void onStop() {
		super.onStop();
		unregisterReceiver( mReceiver );
	}
	
	@Override
	public boolean onCreateOptionsMenu( Menu menu ) {
		MenuInflater inflater = getSupportMenuInflater();		
		inflater.inflate( R.menu.wifi_scanner, menu );		
		return super.onCreateOptionsMenu(menu);
	}
}
