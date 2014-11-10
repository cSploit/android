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

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.System;

public class AboutDialog extends AlertDialog{
  public AboutDialog(Activity activity){
    super(activity);

    final View view = LayoutInflater.from(activity).inflate(R.layout.about_dialog, null);

    this.setTitle(activity.getString(R.string.about_dsploit_v_) + System.getAppVersionName());
    this.setView(view);

    this.setButton("Ok", new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        dialog.dismiss();
      }
    });
  }
}
