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
package org.csploit.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import org.csploit.android.core.Plugin;
import org.csploit.android.core.System;
import org.csploit.android.gui.dialogs.FinishDialog;
import org.csploit.android.net.Target;

import java.util.ArrayList;

public class ActionActivity extends AppCompatActivity {
  private ArrayList<Plugin> mAvailable = null;
  private ListView theList;
  private Target mTarget;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
    Boolean isDark = themePrefs.getBoolean("isDark", false);

    if (isDark)
      setTheme(R.style.DarkTheme);
    else
      setTheme(R.style.AppTheme);
    super.onCreate(savedInstanceState);

    mTarget = System.getCurrentTarget();

    if (mTarget != null) {
      setTitle("cSploit > " + mTarget);
      setContentView(R.layout.actions_layout);
      getSupportActionBar().setDisplayHomeAsUpEnabled(true);
      theList = (ListView) findViewById(R.id.android_list);
      mAvailable = System.getPluginsForTarget();
      ActionsAdapter mActionsAdapter = new ActionsAdapter();
      theList.setAdapter(mActionsAdapter);
      theList.setOnItemClickListener(new ListView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {

          if (System.checkNetworking(ActionActivity.this)) {
            Plugin plugin = mAvailable.get(position);
            System.setCurrentPlugin(plugin);

            if (plugin.hasLayoutToShow()) {
              Toast.makeText(ActionActivity.this, getString(R.string.selected) + getString(plugin.getName()), Toast.LENGTH_SHORT).show();

              startActivity(new Intent(
                      ActionActivity.this,
                      plugin.getClass()
              ));
              overridePendingTransition(R.anim.fadeout, R.anim.fadein);
            } else
              plugin.onActionClick(getApplicationContext());
          }
        }
      });
    } else {
      new FinishDialog(getString(R.string.warning), getString(R.string.something_went_wrong), this).show();
    }
  }


  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
      case android.R.id.home:

        onBackPressed();

        return true;

      default:
        return super.onOptionsItemSelected(item);
    }
  }


  @Override
  public void onBackPressed() {
    super.onBackPressed();
    overridePendingTransition(R.anim.fadeout, R.anim.fadein);
  }

  public class ActionsAdapter extends ArrayAdapter<Plugin> {
    public ActionsAdapter() {
      super(ActionActivity.this, R.layout.actions_list_item, mAvailable);
    }

    @SuppressLint("NewApi")
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
      View row = convertView;
      ActionHolder holder;

      if (row == null) {
        LayoutInflater inflater = (LayoutInflater) ActionActivity.this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        row = inflater.inflate(R.layout.actions_list_item, parent, false);
        if (getSharedPreferences("THEME", 0).getBoolean("isDark", false))
          row.setBackgroundResource(R.drawable.card_background_dark);
        holder = new ActionHolder();

        holder.icon = (ImageView) (row != null ? row.findViewById(R.id.actionIcon) : null);
        holder.name = (TextView) (row != null ? row.findViewById(R.id.actionName) : null);
        holder.description = (TextView) (row != null ? row.findViewById(R.id.actionDescription) : null);
        if (row != null) row.setTag(holder);

      } else holder = (ActionHolder) row.getTag();

      Plugin action = mAvailable.get(position);

      holder.icon.setImageResource(action.getIconResourceId());
      holder.name.setText(getString(action.getName()));
      holder.description.setText(getString(action.getDescription()));

      return row;
    }

    public class ActionHolder {
      ImageView icon;
      TextView name;
      TextView description;
    }
  }
}
