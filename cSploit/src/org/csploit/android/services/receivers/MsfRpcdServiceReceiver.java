package org.csploit.android.services.receivers;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.core.ManagedReceiver;
import org.csploit.android.services.MsfRpcdService;

/**
 * Receive and manage intents from the MsfRpcd service
 */
public class MsfRpcdServiceReceiver extends ManagedReceiver {

  private final IntentFilter filter;

  public MsfRpcdServiceReceiver() {
    filter = new IntentFilter();

    filter.addAction(MsfRpcdService.STATUS_ACTION);
  }

  @Override
  public IntentFilter getFilter() {
    return filter;
  }

  @Override
  public void onReceive(final Context context, Intent intent) {
    if(!MsfRpcdService.STATUS_ACTION.equals(intent.getAction())) {
      return;
    }

    final MsfRpcdService.Status status = (MsfRpcdService.Status)
            intent.getSerializableExtra(MsfRpcdService.STATUS);

    if(context instanceof Activity) {
      ((Activity) context).runOnUiThread(new Runnable() {
        @Override
        public void run() {
          showToastForStatus(context, status);
        }
      });
    } else {
      showToastForStatus(context, status);
    }
  }

  private void showToastForStatus(Context context, MsfRpcdService.Status status) {
    switch (status) {
      case STARTING:
        Toast.makeText(context, R.string.rpcd_starting, Toast.LENGTH_SHORT).show();
        break;
      case CONNECTED:
        Toast.makeText(context, R.string.connected_msf, Toast.LENGTH_SHORT).show();
        break;
      case DISCONNECTED:
        Toast.makeText(context, R.string.msfrpc_disconnected, Toast.LENGTH_SHORT).show();
        break;
      case STOPPED:
        Toast.makeText(context, R.string.rpcd_stopped, Toast.LENGTH_SHORT).show();
        break;
      case KILLED:
        Toast.makeText(context, R.string.msfrpcd_killed, Toast.LENGTH_SHORT).show();
        break;
      case START_FAILED:
        Toast.makeText(context, R.string.msfrcd_start_failed, Toast.LENGTH_LONG).show();
        break;
      case CONNECTION_FAILED:
        Toast.makeText(context, R.string.msf_connection_failed, Toast.LENGTH_LONG).show();
        break;
    }
  }
}
