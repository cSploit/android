package org.csploit.android.gui.adapters;

import android.app.Activity;
import android.graphics.Typeface;
import android.support.v4.content.ContextCompat;
import android.text.Html;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.csploit.android.R;
import org.csploit.android.core.System;
import org.csploit.android.net.Target;

import java.util.List;
import java.util.Observable;
import java.util.Observer;

public class TargetListAdapter extends BaseLiveListAdapter<Target, TargetListAdapter.TargetHolder> implements Observer {

  public TargetListAdapter(Activity activity) {
    super(activity, TargetListAdapter.TargetHolder.class);
  }

  @Override
  public void update(Observable observable, Object data) {
    if(data == null || !(data instanceof Target)) {
      updateWholeList();
    } else {
      updateOneItem((Target) data);
    }
  }

  public int indexOf(String id) {
    List<Target> list = getList();
    for(int i = 0; i < list.size(); i++) {
      if(list.get(i).getIdentifier().equals(id)) {
        return i;
      }
    }

    return -1;
  }

  @Override
  protected TargetHolder createHolder(View row) {
    TargetHolder holder = new TargetHolder();

    holder.itemImage = (ImageView) row.findViewById(R.id.itemIcon);
    holder.itemTitle = (TextView) row.findViewById(R.id.itemTitle);
    holder.itemDescription = (TextView) row.findViewById(R.id.itemDescription);
    holder.portCount = (TextView) row.findViewById(R.id.portCount);
    holder.portCountLayout = (LinearLayout) row.findViewById(R.id.portCountLayout);

    if (isDark())
      holder.portCountLayout.setBackgroundResource(R.drawable.rounded_square_grey);

    return holder;
  }

  @Override
  protected void updateView(Target target, TargetHolder holder) {

    if (target.hasAlias()) {
      holder.itemTitle.setText(Html.fromHtml("<b>"
              + target.getAlias() + "</b> <small>( "
              + target.getDisplayAddress() + " )</small>"));
    } else {
      holder.itemTitle.setText(target.toString());
    }

    holder.itemTitle.setTextColor(ContextCompat.getColor(
            getActivity().getApplicationContext(),
            (target.isConnected() ? R.color.app_color : R.color.gray_text)));

    holder.itemTitle.setTypeface(null, Typeface.NORMAL);
    holder.itemImage.setImageResource(target.getDrawableResourceId());
    holder.itemDescription.setText(target.getDescription());

    int openedPorts = target.getOpenPorts().size();

    holder.portCount.setText(String.format("%d", openedPorts));
    holder.portCountLayout.setVisibility(openedPorts < 1 ? View.GONE : View.VISIBLE);
  }

  @Override
  protected List<Target> getListCopy() {
    return System.getTargets();
  }

  @Override
  protected int getItemLayout() {
    return R.layout.target_list_item;
  }

  class TargetHolder {
    ImageView itemImage;
    TextView itemTitle;
    TextView itemDescription;
    TextView portCount;
    LinearLayout portCountLayout;
  }
}

