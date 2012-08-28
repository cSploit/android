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

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.system.Environment;
import it.evilsocket.dsploit.tools.Ettercap;
import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ToggleButton;

public class Sniffer extends Activity 
{
	private ToggleButton mSniffToggleButton = null;
	private ProgressBar	 mSniffProgress     = null;
	private TextView	 mSniffText			= null;		
	private Ettercap	 mEttercap			= null;
	private boolean	     mRunning			= false;
	private Handler		 mHandler			= null;
	
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);        
        setTitle( "MITM - Sniffer" );
        setContentView( R.layout.plugin_mitm_sniffer );
        
        mSniffToggleButton = ( ToggleButton )findViewById( R.id.sniffToggleButton );
        mSniffProgress	   = ( ProgressBar )findViewById( R.id.sniffActivity );
        mSniffText		   = ( TextView )findViewById( R.id.sniffData );
        mEttercap		   = new Ettercap( this );
        
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
        
        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
            	String text = msg.obj.toString();

            	if( mSniffText.getText().length() > 50 )
            		mSniffText.setText("");
            	
            	mSniffText.append( text + "\n" );
            }
        };
	}

	private void setStoppedState( ) {
		mEttercap.kill();
		mSniffProgress.setVisibility( View.INVISIBLE );
		mRunning = false;
		mSniffToggleButton.setChecked( false );                	
	}

	private void setStartedState( ) {		
		final Ettercap ettercap = mEttercap;

		mRunning = true;
	}
	
	@Override
	public void onBackPressed() {
	    setStoppedState();	
	    super.onBackPressed();
	    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);	    	    
	}
}