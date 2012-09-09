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

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Socket;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.ToggleButton;
import it.evilsocket.dsploit.MainActivity;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.FatalDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.net.Target;

public class PacketForger extends Plugin implements OnClickListener
{
	private static final int	  TCP_PROTOCOL = 0;
	private static final int	  UDP_PROTOCOL = 1;
	private static final String[] PROTOCOLS    = new String[]{ "TCP", "UDP" };
	
	private Spinner        mProtocol     = null;
	private EditText       mPort 		 = null;
	private CheckBox       mWaitResponse = null;
	private EditText       mData 		 = null;
	private EditText       mResponse     = null;
	private ToggleButton   mSendButton   = null;
	private boolean        mRunning	     = false;
	private Thread         mThread	     = null;
	private Socket  	   mSocket	     = null;
	private DatagramSocket mUdpSocket    = null;
	
	public PacketForger( ) {
		super
		( 
		    "Packet Forger", 
		    "Craft and send a custom TCP or UDP packet to the target.", 
		    new Target.Type[]{ Target.Type.ENDPOINT, Target.Type.REMOTE }, 
		    R.layout.plugin_packet_forger,
		    R.drawable.action_forge_48
	    );		
	}
	
	@Override
    public void onCreate( Bundle savedInstanceState ) {
        super.onCreate(savedInstanceState);   
        
        mProtocol     = ( Spinner )findViewById( R.id.protocolSpinner );
        mPort	      = ( EditText )findViewById( R.id.portText );
        mWaitResponse = ( CheckBox )findViewById( R.id.responseCheckBox );
        mData	      = ( EditText )findViewById( R.id.dataText );
        mResponse     = ( EditText )findViewById( R.id.responseText );
        mSendButton   = ( ToggleButton )findViewById( R.id.sendButton );
        
        mProtocol.setAdapter( new ArrayAdapter<String>( this, android.R.layout.simple_spinner_item, PROTOCOLS ) );
        
        mSendButton.setOnClickListener( this );
	}

	@Override
	public void onClick(View v) {
		if( mRunning == false )
		{
			mResponse.setText("");
			
			mRunning = true;

			mThread = new Thread( new Runnable(){
				@Override
				public void run() {
					int     protocol     = mProtocol.getSelectedItemPosition(),
						    port	     = -1;
					String  data         = mData.getText().toString(),
							error		 = null;
					boolean waitResponse = mWaitResponse.isChecked();
					
					try
					{
						port = Integer.parseInt( mPort.getText().toString().trim() );
						if( port <= 0 || port > 65535 )
							port = -1;
					}
					catch( Exception e )
					{
						port = -1;
					}
					
					if( port == -1 )
						error = "Invalid port specified.";
					
					else if( data.isEmpty() )
						error = "The request can not be empty.";
					
					else
					{
						try
						{								
							if( protocol == TCP_PROTOCOL )
							{
								mSocket			    = new Socket( System.getCurrentTarget().getCommandLineRepresentation(), port );
								OutputStream writer = mSocket.getOutputStream();
								
								writer.write( data.getBytes() );
								writer.flush();
								
								if( waitResponse )
								{
									BufferedReader reader   = new BufferedReader( new InputStreamReader( mSocket.getInputStream() ) );
									String 		   response = "",
												   line	    = null;
									
									while( ( line = reader.readLine() ) != null )
									{
										response += line + "\n";
									}
									
									final String text = response;								
									PacketForger.this.runOnUiThread( new Runnable() {
								       public void run() {
								    	   mResponse.setText( text );
								       }
								    });
																														
									reader.close();
								}
								
								writer.close();
								mSocket.close();
							}
							else
							{
								mUdpSocket 			  = new DatagramSocket();
							    DatagramPacket packet = new DatagramPacket( data.getBytes(), data.length(), System.getCurrentTarget().getAddress(), port );
							    
							    mUdpSocket.send( packet );
	
							    if( waitResponse )
								{
							    	byte[] buffer = new byte[1024];
							    	
							    	DatagramPacket response = new DatagramPacket( buffer, buffer.length );
							        
							    	mUdpSocket.receive( response );
							    	
							    	final String text = new String( buffer );								
									PacketForger.this.runOnUiThread( new Runnable() {
								       public void run() {
								    	   mResponse.setText( text );
								       }
								    });
								}
							    
							    mUdpSocket.close();
							}
						}
						catch( Exception e )
						{
							error = e.getMessage();							
						}				
					}
					
					
					final String errorMessage = error;
					PacketForger.this.runOnUiThread( new Runnable() {
				       public void run() {
				    	   setStoppedState( errorMessage );
				       }
				    });
				}
			});
			
			mThread.start();			
		}
		else
		{
			setStoppedState( null );
		}
	}
	
	private void setStoppedState( String errorMessage ) {
		mSendButton.setChecked( false );
		mRunning = false;
		try
		{
			if( mThread != null && mThread.isAlive() )
			{
				if( mSocket != null )
					mSocket.close();
				
				if( mUdpSocket != null )
					mUdpSocket.close();
				
				mThread.stop();
				mThread = null;
				mRunning = false;
			}
		}
		catch( Exception e )
		{
			
		}
		
		if( errorMessage != null )
			new ErrorDialog( "Error", errorMessage, this ).show();
	}
	
	@Override
	public void onBackPressed() {
		setStoppedState( null );
		
	    super.onBackPressed();
	}
}
