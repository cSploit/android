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
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;

import org.csploit.android.R;

public class SpinnerDialog extends AlertDialog{
  private int mSelected = 0;

  public SpinnerDialog(String title, String message, String[] items, int default_index, Activity activity, final SpinnerDialogListener listener){
    super(activity);

    Spinner mSpinner = new Spinner(activity);
    mSpinner.setAdapter(new ArrayAdapter<String>(activity, android.R.layout.simple_spinner_item, items));

    mSpinner.setOnItemSelectedListener(new OnItemSelectedListener(){
      public void onItemSelected(AdapterView<?> adapter, View view, int position, long id){
        mSelected = position;
      }

      public void onNothingSelected(AdapterView<?> arg0){
      }
    });

    mSpinner.setSelection(default_index);

    this.setTitle(title);
    this.setMessage(message);
    this.setView(mSpinner);

    this.setButton(BUTTON_POSITIVE, "Ok", new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        listener.onItemSelected(mSelected);
      }
    });

    this.setButton(BUTTON_NEGATIVE, activity.getString(R.string.cancel_dialog), new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface dialog, int id) {
        dialog.dismiss();
      }
    });
  }

  public SpinnerDialog(String title, String message, String[] items, Activity activity, final SpinnerDialogListener listener) {
    this(title,message,items,0,activity,listener);
  }

  public interface SpinnerDialogListener{
    void onItemSelected(int index);
  }
}
