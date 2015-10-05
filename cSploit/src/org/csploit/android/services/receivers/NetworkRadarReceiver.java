package org.csploit.android.services.receivers;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.widget.ArrayAdapter;
import android.widget.Toast;

import org.csploit.android.MainActivity;
import org.csploit.android.R;
import org.csploit.android.core.*;
import org.csploit.android.services.NetworkRadar;

import java.util.Observer;

/**
 * receive notifications from NetworkRadar
 */
public class NetworkRadarReceiver extends ManagedReceiver {

  private final IntentFilter filter;
  private Observer changeObserver;

  public NetworkRadarReceiver() {
    filter = new IntentFilter();

    filter.addAction(NetworkRadar.NRDR_STARTED);
    filter.addAction(NetworkRadar.NRDR_START_FAILED);
    filter.addAction(NetworkRadar.NRDR_STOPPED);
    filter.addAction(NetworkRadar.NRDR_CHANGED);
  }

  public void setObserver(Observer changeObserver) {
    this.changeObserver = changeObserver;
  }

  @Override
  public IntentFilter getFilter() {
    return filter;
  }

  @Override
  public void onReceive(final Context context, final Intent intent) {
    if(context instanceof Activity) {
      ((Activity) context).runOnUiThread(new Runnable() {
        @Override
        public void run() {
          notifyIntent(context, intent);
        }
      });
    } else {
      notifyIntent(context, intent);
    }
  }

  private void notifyIntent(Context context, Intent intent) {
    String action = intent.getAction();

    switch (action) {
      case NetworkRadar.NRDR_STARTED:
        Toast.makeText(context, R.string.net_discovery_started, Toast.LENGTH_SHORT).show();
        break;
      case NetworkRadar.NRDR_STOPPED:
        Toast.makeText(context, R.string.net_discovery_stopped, Toast.LENGTH_SHORT).show();
        break;
      case NetworkRadar.NRDR_START_FAILED:
        Toast.makeText(context, R.string.net_discovery_start_failed, Toast.LENGTH_LONG).show();
        break;
      case NetworkRadar.NRDR_CHANGED:
        if(changeObserver != null) {
          changeObserver.update(null, null);
        }
        break;
    }
  }
}
