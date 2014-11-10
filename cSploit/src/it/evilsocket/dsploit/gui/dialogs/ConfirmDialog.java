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

import it.evilsocket.dsploit.R;

public class ConfirmDialog extends AlertDialog{
  public interface ConfirmDialogListener{
    public void onConfirm();

    public void onCancel();
  }

  public ConfirmDialog(String title, CharSequence message, Activity activity, ConfirmDialogListener confirmDialogListener){
    super(activity);

    this.setTitle(title);
    this.setMessage(message);

    final ConfirmDialogListener listener = confirmDialogListener;

    this.setButton(activity.getString(R.string.yes), new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        listener.onConfirm();
      }
    });

    this.setButton2(activity.getString(R.string.no), new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        dialog.dismiss();
        listener.onCancel();
      }
    });
  }
}
