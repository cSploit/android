package org.csploit.android.services.receivers;

import android.app.Activity;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import com.google.android.material.snackbar.Snackbar;
import androidx.core.app.NotificationCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.AppCompatActivity;

import org.csploit.android.R;
import org.csploit.android.core.ManagedReceiver;
import org.csploit.android.core.System;
import org.csploit.android.services.MsfRpcdService;

/**
 * Receive and manage intents from the MsfRpcd service
 */
public class MsfRpcdServiceReceiver extends ManagedReceiver {

  final static int MSF_NOTIFICATION = 1337;
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
    Snackbar
            .make(((AppCompatActivity) context).findViewById(android.R.id.content), status.getText(), status.isError() ? Snackbar.LENGTH_LONG : Snackbar.LENGTH_SHORT)
    .show();
  }

  private void updateNotificationForStatus(Context context, MsfRpcdService.Status status) {
    NotificationCompat.Builder mBuilder =
            new NotificationCompat.Builder(context, context.getString(R.string.csploitChannelId))
            .setSmallIcon(R.drawable.exploit_msf)
            .setContentTitle(context.getString(R.string.msf_status))
            .setProgress(0, 0, status.inProgress())
            .setContentText(context.getString(status.getText()))
            .setColor(ContextCompat.getColor(context, status.getColor()))
            .setChannelId(context.getString(R.string.csploitChannelId));

    NotificationManager mNotificationManager =
            (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    mNotificationManager.notify(MSF_NOTIFICATION, mBuilder.build());
  }
}
