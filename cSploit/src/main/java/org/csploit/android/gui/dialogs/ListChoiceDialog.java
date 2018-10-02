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

import android.widget.ArrayAdapter;
import android.widget.ListView;

import org.csploit.android.R;

import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.FragmentActivity;

public class ListChoiceDialog extends AlertDialog {

    /**
     * create a list choice dialog from android resource ids
     *
     * @param items String ids
     */
    public ListChoiceDialog(Integer title, Integer[] items, FragmentActivity activity,
                            final ChoiceDialog.ChoiceDialogListener listener) {
        super(activity);

        ListView mList = new ListView(activity);
        String[] _items;

        _items = new String[items.length];

        for (int i = 0; i < items.length; i++)
            _items[i] = activity.getString(items[i]);

        mList.setAdapter(new ArrayAdapter<>(activity, android.R.layout.simple_list_item_1, _items));

        mList.setOnItemClickListener((parent, view, position, id) -> {
            listener.onChoice(position);
            ListChoiceDialog.this.dismiss();
        });

        setTitle(activity.getString(title));
        setView(mList);

        setButton(BUTTON_NEGATIVE, activity.getString(R.string.cancel_dialog), (dialog, id) -> dialog.dismiss());
    }

    /**
     * create a list choice dialog from a String array
     *
     * @param items Strings to choose from
     */
    public ListChoiceDialog(String title, String[] items, FragmentActivity activity,
                            final ChoiceDialog.ChoiceDialogListener listener) {
        super(activity);

        ListView mList = new ListView(activity);

        mList.setAdapter(new ArrayAdapter<>(activity, android.R.layout.simple_list_item_1, items));

        mList.setOnItemClickListener((parent, view, position, id) -> {
            listener.onChoice(position);
            ListChoiceDialog.this.dismiss();
        });

        this.setTitle(title);
        this.setView(mList);

        this.setButton(BUTTON_NEGATIVE, activity.getString(R.string.cancel_dialog), (dialog, id) -> dialog.dismiss());
    }

    /**
     * create a list choice dialog from generic objects array ( call toString on every object )
     *
     * @param items array containing objects to choices from
     */
    public ListChoiceDialog(String title, Object[] items, FragmentActivity activity,
                            final ChoiceDialog.ChoiceDialogListener listener) {
        super(activity);

        ListView mList = new ListView(activity);

        String[] _items = new String[items.length];

        for (int i = 0; i < _items.length; i++)
            _items[i] = items[i].toString();

        mList.setAdapter(new ArrayAdapter<>(activity, android.R.layout.simple_list_item_1, _items));

        mList.setOnItemClickListener((parent, view, position, id) -> {
            listener.onChoice(position);
            ListChoiceDialog.this.dismiss();
        });

        this.setTitle(title);
        this.setView(mList);

        this.setButton(BUTTON_NEGATIVE, activity.getString(R.string.cancel_dialog), (dialog, id) -> dialog.dismiss());
    }
}
