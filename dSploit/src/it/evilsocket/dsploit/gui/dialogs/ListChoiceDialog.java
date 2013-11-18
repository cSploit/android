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
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Spinner;

import it.evilsocket.dsploit.R;

public class ListChoiceDialog extends AlertDialog{

  public ListChoiceDialog(Integer title, Integer[] items, Activity activity, final ChoiceDialog.ChoiceDialogListener listener){
    super(activity);

    ListView mList = new ListView(activity);
    String[] _items;

    _items = new String[items.length];

    for(int i = 0; i <items.length;i++)
      _items[i] = activity.getString(items[i]);

    mList.setAdapter(new ArrayAdapter<String>(activity, android.R.layout.simple_list_item_1,_items));

    mList.setOnItemClickListener( new AdapterView.OnItemClickListener() {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        listener.onChoice(position);
        ListChoiceDialog.this.dismiss();
      }
    });

    this.setTitle(activity.getString(title));
    this.setView(mList);

    this.setButton(activity.getString(R.string.cancel_dialog), new OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        dialog.dismiss();
      }
    });
  }

  public ListChoiceDialog(String title, String[] items, Activity activity, final ChoiceDialog.ChoiceDialogListener listener){
    super(activity);

    ListView mList = new ListView(activity);

    mList.setAdapter(new ArrayAdapter<String>(activity, android.R.layout.simple_list_item_1,items));

    mList.setOnItemClickListener( new AdapterView.OnItemClickListener() {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        listener.onChoice(position);
        ListChoiceDialog.this.dismiss();
      }
    });

    this.setTitle(title);
    this.setView(mList);

    this.setButton(activity.getString(R.string.cancel_dialog), new OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        dialog.dismiss();
      }
    });
  }
}
