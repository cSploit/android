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

import com.actionbarsherlock.app.SherlockPreferenceActivity;
import com.actionbarsherlock.view.MenuItem;

import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.EditTextPreference;
import android.widget.Toast;

public class SettingsActivity extends SherlockPreferenceActivity implements OnSharedPreferenceChangeListener
{
	private EditTextPreference mSnifferSampleTime	 = null;
	private EditTextPreference mHttpBufferSize		 = null;
	private EditTextPreference mPasswordFilename	 = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getSupportActionBar().setDisplayHomeAsUpEnabled(true);
		addPreferencesFromResource( R.layout.preferences );		
				
		mSnifferSampleTime	  = ( EditTextPreference )getPreferenceScreen().findPreference( "PREF_SNIFFER_SAMPLE_TIME" );
		mHttpBufferSize		  = ( EditTextPreference )getPreferenceScreen().findPreference( "PREF_HTTP_MAX_BUFFER_SIZE" );
		mPasswordFilename	  = ( EditTextPreference )getPreferenceScreen().findPreference( "PREF_PASSWORD_FILENAME" );
	}
	
	@Override
    protected void onResume() {
        super.onResume();
        getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
    }
	
	public void onSharedPreferenceChanged( SharedPreferences sharedPreferences, String key ) {
		String message = null;
		
		if( key.equals("PREF_SNIFFER_SAMPLE_TIME") )
		{
			double sampleTime = 1.0;
			
			try
			{				
				sampleTime = Double.parseDouble( mSnifferSampleTime.getText() );
				if( sampleTime < 0.4 )
				{
					message    = "Sample time can't be less than 0.4.";
					sampleTime = 1.0;
				}
				else if( sampleTime > 1.0 )
				{
					message    = "Sample time can't be greater than 1.0.";
					sampleTime = 1.0;
				}
			}
			catch( Throwable t )
			{
				message    = "Invalid number.";
				sampleTime = 1.0;
			}
			
			if( message != null )
				Toast.makeText( SettingsActivity.this, message, Toast.LENGTH_SHORT ).show();
				
			mSnifferSampleTime.setText( Double.toString( sampleTime ) );
		}
		else if( key.equals("PREF_HTTP_MAX_BUFFER_SIZE") )
		{
			int maxBufferSize = 10485760;
			
			try
			{				
				maxBufferSize = Integer.parseInt( mHttpBufferSize.getText() );
				if( maxBufferSize < 1024 )
				{
					message    	  = "Buffer size can't be less than 1024.";
					maxBufferSize = 10485760;
				}
				else if( maxBufferSize > 104857600 )
				{
					message       = "Buffer size can't be greater than 10485760.";
					maxBufferSize = 10485760;
				}
			}
			catch( Throwable t )
			{
				message    	  = "Invalid number.";
				maxBufferSize = 10485760;
			}
			
			if( message != null )
				Toast.makeText( SettingsActivity.this, message, Toast.LENGTH_SHORT ).show();
				
			mHttpBufferSize.setText( Integer.toString( maxBufferSize ) );
		}
		else if( key.equals("PREF_PASSWORD_FILENAME") )
		{
			String passFileName = "dsploit-password-sniff.log";
			
			try
			{				
				passFileName = mPasswordFilename.getText();
				if( passFileName.matches("[^/?*:;{}\\]+") == false )
				{
					message		 = "Invalid file name.";
					passFileName = "dsploit-password-sniff.log";
				}
			}
			catch( Throwable t )
			{
				message		 = "Invalid file name.";
				passFileName = "dsploit-password-sniff.log";
			}
			
			if( message != null )
				Toast.makeText( SettingsActivity.this, message, Toast.LENGTH_SHORT ).show();
				
			mPasswordFilename.setText( passFileName );
		}
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
}
