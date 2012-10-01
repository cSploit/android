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

import java.util.ArrayList;
import java.util.Arrays;

import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ToggleButton;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.gui.dialogs.FinishDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Target.Port;
import it.evilsocket.dsploit.tools.Hydra;

public class LoginCracker extends Plugin
{
	private static final String[] PROTOCOLS = new String[]
	{
		"ftp",
		"http-head",
		"icq",
		"imap",
		"imap-ntlm",
		"ldap",
		"oracle-listener",
		"mssql",
		"mysql",
		"postgres",
		"pcanywhere",
		"nntp",
		"pcnfs",
		"pop3",
		"pop3-ntlm",
		"rexec",
		"rlogin",
		"rsh",
		"sapr3",
		"smb",
		"smbnt",
		"socks5",
		"ssh",
		"telnet",
		"cisco",
		"cisco-enable",
		"vnc",
		"snmp",
		"cvs",
		"svn",
		"firebird",
		"afp",
		"ncp",
		"smtp-auth",
		"smtp-auth-ntlm",
		"teamspeak",
		"sip",
		"vmauthd"
	};
	
	private static final String[] CHARSETS = new String[]
	{
		"a-z",
		"A-Z",
		"a-z0-9",
		"A-Z0-9",
		"a-zA-Z0-9"
	};
	
	private static final String[] CHARSETS_MAPPING = new String[]
	{
		"a",
		"A",
		"a1",
		"A1",
		"aA1"
	};
	
	private static String[] USERNAMES = new String[]
	{
		"admin",
		"saroot",
		"root",
		"administrator",
		"Administrator",
		"Admin",
		"admin",
		"system",
		"webadmin",
		"daemon ",
		"bin ",
		"sys ",
		"adm ",
		"lp ",
		"uucp ",
		"nuucp ",
		"smmsp ",
		"listen ",
		"gdm ",
		"sshd ",
		"webservd ",
		"oracle",
		"httpd",
		"nginx",
		"-- ADD --"
	};
	
	private static final String[] LENGTHS = new String[]
	{
		"1",
		"2",
		"3",
		"4",
		"5",
		"6"
	};
	
	private Spinner 	 	mPortSpinner	 = null;
	private Spinner 	 	mProtocolSpinner = null;
	private Spinner 	 	mCharsetSpinner  = null;
	private Spinner 	 	mUserSpinner	 = null;
	private Spinner 	 	mMinSpinner	  	 = null;
	private Spinner 	 	mMaxSpinner	  	 = null;
	private ToggleButton 	mStartButton 	 = null;
	private TextView 	 	mStatusText 	 = null;
	private ProgressBar	    mActivity		 = null;
	private ProgressBar  	mProgressBar 	 = null;
	private boolean	     	mRunning		 = false;
	private boolean			mAccountFound	 = false;
	private AttemptReceiver mReceiver     	 = null;
	
	public LoginCracker( ) {
		super
		( 
		    "Login Cracker", 
		    "A very fast network logon cracker which support many different services.", 
		    new Target.Type[]{ Target.Type.ENDPOINT, Target.Type.REMOTE }, 
		    R.layout.plugin_login_cracker,
		    R.drawable.action_login_48
	    );		
	}
	
	private class AttemptReceiver extends Hydra.AttemptReceiver
	{
		@Override
		public void onNewAttempt( String login, String password, int progress, int total ) {
			final String user = login,
						 pass = password;
			
			final int    step = progress,
						 tot  = total;
			
			LoginCracker.this.runOnUiThread(new Runnable() {
	            @Override
	            public void run() {
	            	mStatusText.setTextColor( Color.DKGRAY );
	            	mStatusText.setText( "Trying " + user + ":" + pass );
	            	mProgressBar.setMax( tot );
	            	mProgressBar.setProgress( step );
	            }
	        });
		}		
		
		@Override
		public void onAccountFound( final String login, final String password ) {
			LoginCracker.this.runOnUiThread(new Runnable() {
	            @Override
	            public void run() {
	            	setStoppedState( login, password );
	            }
	        });			
		}
		
		@Override
		public void onError( String message ) {
			final String error = message;
			
			LoginCracker.this.runOnUiThread(new Runnable() {
	            @Override
	            public void run() {
	            	mStatusText.setTextColor( Color.YELLOW );
	            	mStatusText.setText( error );
	            }
	        });
		}

		@Override
		public void onFatal( String message ) {			
			final String error = message;
			
			LoginCracker.this.runOnUiThread(new Runnable() {
	            @Override
	            public void run() {
	    			setStoppedState();
	            	mStatusText.setTextColor( Color.RED );
	            	mStatusText.setText( error );
	            }
	        });
		}				
		
		@Override
		public void onEnd( int code ) {
			if( mRunning )
			{
				LoginCracker.this.runOnUiThread(new Runnable() {
		            @Override
		            public void run() {
		    			setStoppedState();
		            }
		        });
			}
		}
	}
	
	private void setStoppedState( final String user, final String pass ) {
		System.getHydra().kill();
		mRunning 	  = false;
		mAccountFound = true;
		
		LoginCracker.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
            	mActivity.setVisibility( View.INVISIBLE );
        		mProgressBar.setProgress( 0 );
        		mStartButton.setChecked( false );
        		mStatusText.setTextColor( Color.GREEN );
            	mStatusText.setText( "USERNAME = " + user + " - PASSWORD = " + pass );
            }
        });
	}
	
	private void setStoppedState( ) {
		System.getHydra().kill();
		mRunning = false;
		
		LoginCracker.this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
            	mActivity.setVisibility( View.INVISIBLE );
        		mProgressBar.setProgress( 0 );
        		mStartButton.setChecked( false );
        		if( mAccountFound == false )
        		{
        			mStatusText.setTextColor( Color.DKGRAY );
        			mStatusText.setText( "Stopped ..." );
        		}
            }
        });
	}
	
	private void setStartedState( ) {
		int min = Integer.parseInt( ( String )mMinSpinner.getSelectedItem() ), 
			max	= Integer.parseInt( ( String )mMaxSpinner.getSelectedItem() );
		
		if( min > max ) max = min + 1;
		
		mAccountFound = false;
		
		mActivity.setVisibility( View.VISIBLE );
    	mStatusText.setTextColor( Color.DKGRAY );
    	mStatusText.setText( "Starting ..." );
    	
    	System.getHydra().crack
		(
		  System.getCurrentTarget(), 
		  Integer.parseInt( ( String )mPortSpinner.getSelectedItem() ), 
		  ( String )mProtocolSpinner.getSelectedItem(), 
		  CHARSETS_MAPPING[ mCharsetSpinner.getSelectedItemPosition() ], 
		  min,
		  max,
		  ( String )mUserSpinner.getSelectedItem(), 
		  mReceiver
		).start();
		
		mRunning = true;
	}
	
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);   
       
        if( System.getCurrentTarget().hasOpenPorts() == false )
        	new FinishDialog( "Warning", "No open ports detected on current target, run the port scanner first.", this ).show();
                        
        ArrayList<String> ports = new ArrayList<String>();
        
        for( Port port : System.getCurrentTarget().getOpenPorts() )
        	ports.add( Integer.toString( port.number ) );
        
        mPortSpinner = ( Spinner )findViewById( R.id.portSpinner );
        mPortSpinner.setAdapter( new ArrayAdapter<String>( this, android.R.layout.simple_spinner_item, ports ) );
        mPortSpinner.setOnItemSelectedListener( new OnItemSelectedListener() {
        	public void onItemSelected( AdapterView<?> adapter, View view, int position, long id ) 
        	{
        		String port 	= ( String )adapter.getItemAtPosition( position ),
        			   protocol = System.getProtocolByPort( port );
        		
        		if( protocol != null )
        		{
        			for( int i = 0; i < PROTOCOLS.length; i++ )
        			{
        				if( PROTOCOLS[i].equals( protocol ) )
        				{
        					mProtocolSpinner.setSelection( i );
        					break;
        				}
        			}
        		}
        	}
        	
        	public void onNothingSelected(AdapterView<?> arg0) {}
		});
        
        mProtocolSpinner = ( Spinner )findViewById( R.id.protocolSpinner );
        mProtocolSpinner.setAdapter( new ArrayAdapter<String>( this, android.R.layout.simple_spinner_item, PROTOCOLS ) );
        
        mCharsetSpinner = ( Spinner )findViewById( R.id.charsetSpinner );
        mCharsetSpinner.setAdapter( new ArrayAdapter<String>( this, android.R.layout.simple_spinner_item, CHARSETS ) );

        mUserSpinner = ( Spinner )findViewById( R.id.userSpinner );
        mUserSpinner.setAdapter( new ArrayAdapter<String>( this, android.R.layout.simple_spinner_item, USERNAMES ) );
        mUserSpinner.setOnItemSelectedListener( new OnItemSelectedListener() {
        	public void onItemSelected( AdapterView<?> adapter, View view, int position, long id ) 
        	{
        		String user = ( String )adapter.getItemAtPosition( position );
        		if( user.equals("-- ADD --") )
        		{
        			new InputDialog( "Add username", "Enter the username you want to use:", LoginCracker.this, new InputDialogListener(){
						@Override
						public void onInputEntered( String input ) {
							USERNAMES = Arrays.copyOf( USERNAMES, USERNAMES.length + 1 );
							USERNAMES[ USERNAMES.length - 1 ] = "-- ADD --";
							USERNAMES[ USERNAMES.length - 2 ] = input;
							
							mUserSpinner.setAdapter( new ArrayAdapter<String>( LoginCracker.this, android.R.layout.simple_spinner_item, USERNAMES ) );
							mUserSpinner.setSelection( USERNAMES.length - 2 );
						}        				
        			}).show();
        		}
        	}
        	
        	public void onNothingSelected(AdapterView<?> arg0) {}
		});

        mMaxSpinner = ( Spinner )findViewById( R.id.maxSpinner );
        mMaxSpinner.setAdapter( new ArrayAdapter<String>( this, android.R.layout.simple_spinner_item, LENGTHS ) );
        mMinSpinner = ( Spinner )findViewById( R.id.minSpinner );
        mMinSpinner.setAdapter( new ArrayAdapter<String>( this, android.R.layout.simple_spinner_item, LENGTHS ) );
        
        mStartButton = ( ToggleButton )findViewById( R.id.startButton );
        mStatusText  = ( TextView )findViewById( R.id.statusText );
        mProgressBar = ( ProgressBar )findViewById( R.id.progressBar );
        mActivity	 = ( ProgressBar )findViewById( R.id.activity );
        
        mReceiver = new AttemptReceiver();
        
        mStartButton.setOnClickListener( new OnClickListener(){
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
	public void onBackPressed() {
		setStoppedState();	
	    super.onBackPressed();
	}
}
