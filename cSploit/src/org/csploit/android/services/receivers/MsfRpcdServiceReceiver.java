package org.csploit.android.services.receivers;

import android.app.Activity;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.support.v4.app.NotificationCompat;
import android.support.v4.content.ContextCompat;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.core.ManagedReceiver;
import org.csploit.android.core.System;
import org.csploit.android.services.MsfRpcdService;

/**
 * Receive and manage intents from the MsfRpcd service
 */
public class MsfRpcdServiceReceiver extends ManagedReceiver {

  final int MSF_NOTIFICATION = 1337;
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

    SharedPreferences myPrefs = System.getSettings();
    if (myPrefs.getBoolean("MSF_NOTIFICATIONS", true)) {
      updateNotificationForStatus(context, status);
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

  private void updateNotificationForStatus(Context context, MsfRpcdService.Status status) {
    NotificationCompat.Builder mBuilder =
            new NotificationCompat.Builder(context)
                    .setSmallIcon(R.drawable.exploit_msf)
                    .setContentTitle("MetaSploit RPCD Status");
    mBuilder.setProgress(0, 0, false);
    switch (status) {
      case STARTING:
        mBuilder.setContentText(context.getResources().getText(R.string.rpcd_starting));
        mBuilder.setProgress(0, 0, true);
        mBuilder.setColor(ContextCompat.getColor(context, R.color.selectable_blue));
        break;
      case CONNECTED:
        mBuilder.setContentText(context.getResources().getText(R.string.connected_msf));
        mBuilder.setColor(ContextCompat.getColor(context, R.color.green));
        break;
      case DISCONNECTED:
        mBuilder.setContentText(context.getResources().getText(R.string.msfrpc_disconnected));
        mBuilder.setColor(ContextCompat.getColor(context, R.color.purple));
        break;
      case STOPPED:
        mBuilder.setContentText(context.getResources().getText(R.string.rpcd_stopped));
        mBuilder.setColor(ContextCompat.getColor(context, R.color.purple));
        break;
      case KILLED:
        mBuilder.setContentText(context.getResources().getText(R.string.msfrpcd_killed));
        mBuilder.setColor(ContextCompat.getColor(context, R.color.purple));
        break;
      case START_FAILED:
        mBuilder.setContentText(context.getResources().getText(R.string.msfrcd_start_failed));
        mBuilder.setColor(ContextCompat.getColor(context, R.color.red));
        break;
      case CONNECTION_FAILED:
        mBuilder.setContentText(context.getResources().getText(R.string.msf_connection_failed));
        mBuilder.setColor(ContextCompat.getColor(context, R.color.red));
        break;
    }

    NotificationManager mNotificationManager =
            (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    mNotificationManager.notify(MSF_NOTIFICATION, mBuilder.build());
  }
}
