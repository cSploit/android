package org.csploit.android.gui.adapters;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.view.View;
import android.widget.TextView;

import org.csploit.android.R;
import org.csploit.android.net.Target;

import java.util.List;
import java.util.Observable;
import java.util.Observer;

/**
 * Adapt a target opened ports to the UI
 */
public class PortListAdapter extends BaseLiveListAdapter<Target.Port, PortListAdapter.PortHolder>
  implements Observer {

  private final Target target;

  public PortListAdapter(@NonNull Activity activity, @NonNull Target target) {
    super(activity, PortListAdapter.PortHolder.class);
    this.target = target;
  }

  @Override
  protected List<Target.Port> getListCopy() {
    return target.getOpenPorts();
  }

  @Override
  protected int getItemLayout() {
    return R.layout.port_list_item;
  }

  @Override
  protected PortHolder createHolder(View row) {
    PortHolder holder = new PortHolder();

    holder.title = (TextView) row.findViewById(R.id.itemTitle);
    holder.description = (TextView) row.findViewById(R.id.itemDescription);

    return holder;
  }

  @Override
  protected void updateView(Target.Port port, PortHolder holder) {
    holder.title.setText(port.getProtocol() + ": " + port.getNumber());
    holder.description.setText(port.getService() + " - " + port.getVersion());
  }

  @Override
  public void update(Observable observable, Object data) {

  }

  class PortHolder {
    public TextView title;
    public TextView description;
  }
}
