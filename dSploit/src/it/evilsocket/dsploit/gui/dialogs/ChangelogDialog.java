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
package it.evilsocket.dsploit.gui.dialogs;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import it.evilsocket.dsploit.R;

public class ChangelogDialog extends AlertDialog
{
  private final String ERROR_HTML = getContext().getString(R.string.something_went_wrong_changelog);

  private ProgressDialog mLoader = null;

  @SuppressLint("SetJavaScriptEnabled")
  public ChangelogDialog(final Activity activity){
    super(activity);

    this.setTitle("Changelog");


    WebView view = new WebView(activity);
    WebSettings settings = view.getSettings();

    settings.setJavaScriptEnabled(true);
    settings.setAppCacheEnabled(false);

    view.setWebViewClient(new WebViewClient(){
      @Override
      public void onPageStarted(WebView view, String url, Bitmap favicon){
        if(mLoader == null)
          mLoader = ProgressDialog.show(activity, "", getContext().getString(R.string.loading_changelog));

        super.onPageStarted(view, url, favicon);
      }

      @Override
      public void onPageFinished(WebView view, String url){
        super.onPageFinished(view, url);
        mLoader.dismiss();
      }

      @Override
      public void onReceivedError(WebView view, int errorCode, String description, String failingUrl){
        try{
          mLoader.dismiss();
        } catch(Exception e){
        }

        try{
          view.stopLoading();
        } catch(Exception e){
        }

        try{
          view.clearView();
        } catch(Exception e){
        }

        view.loadData(ERROR_HTML.replace("{DESCRIPTION}", description), "text/html", "UTF-8");

        super.onReceivedError(view, errorCode, description, failingUrl);
      }
    });

    this.setView(view);

    view.loadUrl("http://update.dsploit.net/changelog");

    this.setCancelable(false);
    this.setButton("Ok", new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        dialog.dismiss();
      }
    });
  }
}
