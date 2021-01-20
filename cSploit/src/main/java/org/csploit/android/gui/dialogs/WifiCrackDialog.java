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

import android.content.DialogInterface;
import androidx.fragment.app.FragmentActivity;
import androidx.appcompat.app.AlertDialog;
import android.text.InputType;
import android.widget.EditText;

import org.csploit.android.R;

public class WifiCrackDialog extends AlertDialog{
  private EditText mEditText = null;

  public WifiCrackDialog(String title, String message, FragmentActivity activity, WifiCrackDialogListener wifiCrackDialogListener){
    super(activity);

    mEditText = new EditText(activity);
    mEditText.setEnabled(true);
    mEditText.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);

    this.setTitle(title);
    this.setMessage(message);
    this.setView(mEditText);

    final WifiCrackDialogListener listener = wifiCrackDialogListener;

    this.setButton(BUTTON_POSITIVE, activity.getString(R.string.connect), new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        if(listener != null)
          listener.onManualConnect(mEditText.getText() + "");
      }
    });

    this.setButton(BUTTON_NEUTRAL, activity.getString(R.string.crack), new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface dialog, int id) {
        if (listener != null)
          listener.onCrack();
      }
    });

    this.setButton(BUTTON_NEGATIVE, activity.getString(R.string.cancel_dialog), new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface dialog, int id) {
        dialog.dismiss();
      }
    });
  }

  public interface WifiCrackDialogListener{
    void onManualConnect(String key);

    void onCrack();
  }
}