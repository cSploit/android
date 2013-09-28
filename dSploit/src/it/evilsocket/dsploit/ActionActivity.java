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
package it.evilsocket.dsploit;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.actionbarsherlock.app.SherlockListActivity;
import com.actionbarsherlock.view.MenuItem;

import java.util.ArrayList;

import it.evilsocket.dsploit.core.Plugin;
import it.evilsocket.dsploit.core.Profiler;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.gui.dialogs.FinishDialog;

public class ActionActivity extends SherlockListActivity{
  private ArrayList<Plugin> mAvailable = null;

  @Override
  public void onCreate(Bundle savedInstanceState){
    super.onCreate(savedInstanceState);

    if(System.getTargets() != null && System.getTargets().size() > 0 && System.getCurrentTarget() != null){
      setTitle("dSploit > " + System.getCurrentTarget());
      setContentView(R.layout.actions_layout);
      getSupportActionBar().setDisplayHomeAsUpEnabled(true);

      mAvailable = System.getPluginsForTarget();
      ActionsAdapter mActionsAdapter = new ActionsAdapter();

      setListAdapter(mActionsAdapter);
    } else
      new FinishDialog(getString(R.string.warning), getString(R.string.something_went_wrong), this).show();
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item){
    switch(item.getItemId()){
      case android.R.id.home:

        onBackPressed();

        return true;

      default:
        return super.onOptionsItemSelected(item);
    }
  }

  @Override
  protected void onListItemClick(ListView l, View v, int position, long id){
    super.onListItemClick(l, v, position, id);

    if(System.checkNetworking(this)){
      Plugin plugin = mAvailable.get(position);
      System.setCurrentPlugin(plugin);

      if(plugin.hasLayoutToShow()){
        Toast.makeText(ActionActivity.this, getString(R.string.selected) + getString(plugin.getName()), Toast.LENGTH_SHORT).show();

        startActivity(new Intent(
          ActionActivity.this,
          plugin.getClass()
        ));
        overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
      } else
        plugin.onActionClick(this);
    }
  }

  @Override
  public void onBackPressed(){
    super.onBackPressed();
    overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_left);
  }

  public class ActionsAdapter extends ArrayAdapter<Plugin>{
    public ActionsAdapter(){
      super(ActionActivity.this, R.layout.actions_list_item, mAvailable);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent){
      View row = convertView;
      ActionHolder holder;

      if(row == null){
        LayoutInflater inflater = (LayoutInflater) ActionActivity.this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        row = inflater.inflate(R.layout.actions_list_item, parent, false);

        holder = new ActionHolder();

        holder.icon = (ImageView) (row != null ? row.findViewById(R.id.actionIcon) : null);
        holder.name = (TextView) (row != null ? row.findViewById(R.id.actionName) : null);
        holder.description = (TextView) (row != null ? row.findViewById(R.id.actionDescription) : null);
        if(row != null) row.setTag(holder);

      } else holder = (ActionHolder) row.getTag();

      Plugin action = mAvailable.get(position);

      holder.icon.setImageResource(action.getIconResourceId());
      holder.name.setText(getString(action.getName()));
      holder.description.setText(getString(action.getDescription()));

      return row;
    }

    public class ActionHolder{
      ImageView icon;
      TextView name;
      TextView description;
    }
  }
}
