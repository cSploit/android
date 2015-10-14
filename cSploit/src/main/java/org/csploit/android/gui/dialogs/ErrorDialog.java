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

public class ErrorDialog extends AlertDialog{
  public ErrorDialog(String title, String message, final Activity activity){
    super(activity);

    this.setTitle(title);
    this.setMessage(message);
    this.setCancelable(false);
    this.setButton(BUTTON_POSITIVE, "Ok", new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        dialog.dismiss();
      }
    });
  }
}
