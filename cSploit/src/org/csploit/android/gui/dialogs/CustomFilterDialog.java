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

import java.util.ArrayList;

public class CustomFilterDialog extends AlertDialog{
  public interface CustomFilterDialogListener{
    void onInputEntered(ArrayList<String> from, ArrayList<String> to);
  }

  public CustomFilterDialog(String title, Activity activity, final CustomFilterDialogListener listener){
    super(activity);

    final View view = LayoutInflater.from(activity).inflate(R.layout.plugin_mitm_filter_dialog, null);

    this.setTitle(title);
    this.setView(view);

    this.setButton(BUTTON_POSITIVE, "Ok", new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        assert view != null;
        String f0 = ((EditText) view.findViewById(R.id.fromText0)).getText() + "".trim(),
          f1 = ((EditText) view.findViewById(R.id.fromText1)).getText() + "".trim(),
          f2 = ((EditText) view.findViewById(R.id.fromText2)).getText() + "".trim(),
          f3 = ((EditText) view.findViewById(R.id.fromText3)).getText() + "".trim(),
          f4 = ((EditText) view.findViewById(R.id.fromText4)).getText() + "".trim(),
          t0 = ((EditText) view.findViewById(R.id.toText0)).getText() + "".trim(),
          t1 = ((EditText) view.findViewById(R.id.toText1)).getText() + "".trim(),
          t2 = ((EditText) view.findViewById(R.id.toText2)).getText() + "".trim(),
          t3 = ((EditText) view.findViewById(R.id.toText3)).getText() + "".trim(),
          t4 = ((EditText) view.findViewById(R.id.toText4)).getText() + "".trim();

        ArrayList<String> from = new ArrayList<String>(),
          to = new ArrayList<String>();

        if(!f0.isEmpty() && !t0.isEmpty()){
          from.add(f0);
          to.add(t0);
        }

        if(!f1.isEmpty() && !t1.isEmpty()){
          from.add(f1);
          to.add(t1);
        }

        if(!f2.isEmpty() && !t2.isEmpty()){
          from.add(f2);
          to.add(t2);
        }

        if(!f3.isEmpty() && !t3.isEmpty()){
          from.add(f3);
          to.add(t3);
        }

        if(!f4.isEmpty() && !t4.isEmpty()){
          from.add(f4);
          to.add(t4);
        }

        listener.onInputEntered(from, to);
      }
    });

    this.setButton(BUTTON_NEGATIVE, activity.getString(R.string.cancel_dialog), new DialogInterface.OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        dialog.dismiss();
      }
    });
  }
}
