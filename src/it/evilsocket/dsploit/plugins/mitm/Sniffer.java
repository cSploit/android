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
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.tools.Ettercap.OnReadyListener;
import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ToggleButton;

public class Sniffer extends Activity 
{
	private static final String  PCAP_FILTER = "not '(src host localhost or dst host localhost or arp)'";
	private static final Pattern PARSER 	 = Pattern.compile( "^.+length\\s+(\\d+)\\)\\s+([^\\s]+)\\s+>\\s+([^\\:]+):.+", Pattern.CASE_INSENSITIVE );

	private ToggleButton mSniffToggleButton = null;
	private ProgressBar	 mSniffProgress     = null;
	private TextView	 mSniffText			= null;		
	private boolean	     mRunning			= false;	
	
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);        
        setTitle( "MITM - Sniffer" );
        setContentView( R.layout.plugin_mitm_sniffer );
        
        mSniffToggleButton = ( ToggleButton )findViewById( R.id.sniffToggleButton );
        mSniffProgress	   = ( ProgressBar )findViewById( R.id.sniffActivity );
        mSniffText		   = ( TextView )findViewById( R.id.sniffData );
        
        mSniffText.setEnabled(false);
                
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
					public void onNewLine(String line) {
						Matcher matcher = PARSER.matcher( line.trim() );
						
						if( matcher != null && matcher.find() )
						{
							String length = matcher.group( 1 ),
								   source = matcher.group( 2 ),
								   dest   = matcher.group( 3 );
																					
							final String entry = "[ " + length + " bytes ] " + source + " -> " + dest;
							
							Sniffer.this.runOnUiThread( new Runnable() {
								@Override
								public void run(){
									
									if( mSniffText.getText().length() > 4096 )
										mSniffText.setText("");
									
					            	mSniffText.append( entry + "\n" );
								}							
							});				
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