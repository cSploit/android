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
package org.csploit.android.gui.dialogs;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

import org.csploit.android.R;

public class RedirectionDialog extends AlertDialog{
  public RedirectionDialog(String title, Activity activity, final RedirectionDialogListener listener){
    super(activity);

    final View view = LayoutInflater.from(activity).inflate(R.layout.plugin_mitm_redirect_dialog, null);

    this.setTitle(title);
    this.setView(view);

    this.setButton(BUTTON_POSITIVE, "Ok", new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        assert view != null;
        String address = ((EditText) view.findViewById(R.id.redirAddress)).getText() + "".trim(),
          port = ((EditText) view.findViewById(R.id.redirPort)).getText() + "".trim();

        listener.onInputEntered(address, port);
      }
    });

    this.setButton(BUTTON_NEGATIVE, activity.getString(R.string.cancel_dialog), new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface dialog, int id) {
        dialog.dismiss();
      }
    });
  }

  public interface RedirectionDialogListener{
    void onInputEntered(String address, String port);
  }
}
