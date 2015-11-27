package org.csploit.android.gui.adapters;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import org.csploit.android.R;
import org.csploit.android.core.Plugin;
import org.csploit.android.helpers.GUIHelper;

import java.util.List;

/**
 * Adapt a list of plugins to the UI
 */
public class PluginListAdapter extends ArrayAdapter<Plugin> {

  public PluginListAdapter(Context context, Plugin[] objects) {
    super(context, R.layout.actions_list_item, objects);
  }

  public PluginListAdapter(Context context, List<Plugin> objects) {
    super(context, R.layout.actions_list_item, objects);
  }

  @SuppressLint("NewApi")
  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    View row = convertView;
    Object tag = row != null ? row.getTag() : null;
    PluginHolder holder;

    if (tag == null || !(tag instanceof PluginHolder)) {
      LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
      row = inflater.inflate(R.layout.actions_list_item, parent, false);
      row.setBackgroundResource(
              GUIHelper.isDarkThemeEnabled() ?
              R.drawable.card_background_dark :
              R.drawable.card_background
      );
      holder = new PluginHolder();

      holder.icon = (ImageView) row.findViewById(R.id.actionIcon);
      holder.name = (TextView) row.findViewById(R.id.actionName);
      holder.description = (TextView) row.findViewById(R.id.actionDescription);
      row.setTag(holder);

    } else holder = (PluginHolder) tag;

    Plugin plugin = getItem(position);

    holder.icon.setImageResource(plugin.getIconResourceId());
    holder.name.setText(getContext().getString(plugin.getName()));
    holder.description.setText(getContext().getString(plugin.getDescription()));

    return row;
  }

  public class PluginHolder {
    ImageView icon;
    TextView name;
    TextView description;
  }
}
