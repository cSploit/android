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
import android.widget.ArrayAdapter;
import android.widget.CheckedTextView;
import android.widget.ListView;

import org.csploit.android.R;

public class MultipleChoiceDialog extends AlertDialog{

  public interface MultipleChoiceDialogListener{
    void onChoice(int[] choices);
  }

  private boolean[] checkList = null;

  /** create a list choice dialog from android resource ids
   * @param items String ids
   */
  public MultipleChoiceDialog(int title, int[] items, Activity activity, final MultipleChoiceDialogListener listener){
    super(activity);

    String[] _items = new String[items.length];

    for(int i = 0; i <items.length;i++)
      _items[i] = activity.getString(items[i]);

    commonCtor(activity.getString(title),_items, activity, listener);
  }

  /** create a list choice dialog from a String array
   * @param items Strings to choose from
   */

  public MultipleChoiceDialog(String title, String[] items, Activity activity, final MultipleChoiceDialogListener listener){
    super(activity);

    commonCtor(title, items, activity, listener);
  }

  /** create a list choice dialog from generic objects array ( call toString on every object )
   * @param items
   */
  public MultipleChoiceDialog(String title, Object[] items, Activity activity, final MultipleChoiceDialogListener listener) {
    super(activity);

    String[] _items = new String[items.length];

    for(int i = 0; i < _items.length;i++)
      _items[i] = items[i].toString();

    commonCtor(title, _items, activity, listener);
  }

  private void commonCtor(String title, String[] items, Activity activity, final MultipleChoiceDialogListener listener) {

    ListView mList = new ListView(activity);

    mList.setAdapter(new ArrayAdapter<String>(activity, android.R.layout.simple_list_item_multiple_choice,items));

    checkList = new boolean[items.length];

    mList.setOnItemClickListener( new AdapterView.OnItemClickListener() {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        CheckedTextView checkedTextView = (CheckedTextView)view;

        checkedTextView.setChecked((checkList[position] = !checkedTextView.isChecked()));
      }
    });

    this.setTitle(title);
    this.setView(mList);

    setButton(BUTTON_NEGATIVE, activity.getString(R.string.cancel_dialog), new OnClickListener(){
      public void onClick(DialogInterface dialog, int id){
        dialog.dismiss();
      }
    });
    setButton(BUTTON_POSITIVE, activity.getString(R.string.choose), new OnClickListener() {
      @Override
      public void onClick(DialogInterface dialog, int which) {
        int count = 0;
        int[] res;

        for(boolean b : checkList)
          if(b)
            count++;
        res = new int[count];
        count=0;
        for(int i=0; i<checkList.length; i++)
          if(checkList[i])
            res[count++] = i;
        listener.onChoice(res);
        dialog.dismiss();
      }
    });
  }
}
