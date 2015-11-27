package org.csploit.android.gui.adapters;

import android.app.Activity;
import android.content.Context;
import android.graphics.Typeface;
import android.support.v4.content.ContextCompat;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import org.csploit.android.R;
import org.csploit.android.core.System;
import org.csploit.android.net.Target;

import java.util.List;
import java.util.Observable;
import java.util.Observer;

public class TargetListAdapter extends BaseAdapter implements Runnable, Observer {

  private List<Target> list = org.csploit.android.core.System.getTargets();
  private boolean isDark;
  private final Activity activity;
  private ListView lv;

  public TargetListAdapter(Activity activity) {
    this.activity = activity;
    isDark = System.getSettings().getBoolean("PREF_DARK_THEME", false);
  }

  public void setListView(ListView lv) {
    this.lv = lv;
  }

  @Override
  public int getCount() {
    return list.size();
  }

  @Override
  public Object getItem(int position) {
    return list.get(position);
  }

  @Override
  public long getItemId(int position) {
    return R.layout.target_list_item;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    View row = convertView;
    TargetHolder holder;
    Object tag = row != null ? row.getTag() : null;

    if (row == null || tag == null || !(tag instanceof TargetHolder)) {
      LayoutInflater inflater = (LayoutInflater) activity
              .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
      row = inflater.inflate(R.layout.target_list_item, parent, false);

      if (isDark)
        row.setBackgroundResource(R.drawable.card_background_dark);

      holder = new TargetHolder();
      holder.itemImage = (ImageView) row.findViewById(R.id.itemIcon);
      holder.itemTitle = (TextView) row.findViewById(R.id.itemTitle);
      holder.itemDescription = (TextView) row.findViewById(R.id.itemDescription);
      holder.portCount = (TextView) row.findViewById(R.id.portCount);
      holder.portCountLayout = (LinearLayout) row.findViewById(R.id.portCountLayout);
      if (isDark)
        holder.portCountLayout.setBackgroundResource(R.drawable.rounded_square_grey);
      row.setTag(holder);
    } else {
      holder = (TargetHolder) tag;
    }

    final Target target = list.get(position);

    if (target.hasAlias()) {
      holder.itemTitle.setText(Html.fromHtml("<b>"
              + target.getAlias() + "</b> <small>( "
              + target.getDisplayAddress() + " )</small>"));
    } else {
      holder.itemTitle.setText(target.toString());
    }
    holder.itemTitle.setTextColor(ContextCompat.getColor(activity.getApplicationContext(), (target.isConnected() ? R.color.app_color : R.color.gray_text)));

    holder.itemTitle.setTypeface(null, Typeface.NORMAL);
    holder.itemImage.setImageResource(target.getDrawableResourceId());
    holder.itemDescription.setText(target.getDescription());

    int openedPorts = target.getOpenPorts().size();

    holder.portCount.setText(String.format("%d", openedPorts));
    holder.portCountLayout.setVisibility(openedPorts < 1 ? View.GONE : View.VISIBLE);
    return row;
  }

  public int indexOf(String id) {
    synchronized (this) {
      for(int i = 0; i < list.size(); i++) {
        if(list.get(i).getIdentifier().equals(id)) {
          return i;
        }
      }
    }
    return -1;
  }

  @Override
  public void update(Observable observable, Object data) {
    final Target target = (Target) data;

    if (target == null) {
      // update the whole list
      activity.runOnUiThread(this);
      return;
    }

    // update only a row, if it's displayed
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        if (lv == null)
          return;
        synchronized (this) {
          int start = lv.getFirstVisiblePosition();
          int end = Math.min(lv.getLastVisiblePosition(), list.size());
          for (int i = start; i <= end; i++)
            if (target == list.get(i)) {
              View view = lv.getChildAt(i - start);
              getView(i, view, lv);
              break;
            }
        }
      }
    });
  }

  @Override
  public void run() {
    synchronized (this) {
      list = System.getTargets();
    }
    notifyDataSetChanged();
  }

  class TargetHolder {
    ImageView itemImage;
    TextView itemTitle;
    TextView itemDescription;
    TextView portCount;
    LinearLayout portCountLayout;
  }
}

