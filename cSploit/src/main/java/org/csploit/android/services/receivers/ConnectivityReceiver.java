package org.csploit.android.services.receivers;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.widget.Toast;

import org.csploit.android.R;
import org.csploit.android.core.*;
import org.csploit.android.core.System;
import org.csploit.android.gui.activities.AbstractSidebarActivity;
import org.csploit.android.gui.dialogs.ConfirmDialog;
import org.csploit.android.net.Network;
import org.csploit.android.services.Services;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

/**
 * Receive connectivity changes
 */
public class ConnectivityReceiver extends ManagedReceiver {
  private static final int CHECK_DELAY = 2000;

  private final IntentFilter filter;
  private final AbstractSidebarActivity activity;

  private List<String> ifaces;
  private TimerTask mTask = null;
  private boolean offline;

  public ConnectivityReceiver(AbstractSidebarActivity activity) {
    this.activity = activity;

    filter = new IntentFilter();
    filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);

    loadInterfaces();
  }

  @Override
  public IntentFilter getFilter() {
    return filter;
  }

  private String ifacesToString() {
    StringBuilder sb = new StringBuilder();

    for (String iface : ifaces) {
      if (sb.length() > 0) {
        sb.append(", ");
      }
      sb.append(iface);
    }

    return sb.toString();
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    synchronized (this) {
      if (mTask != null) {
        mTask.cancel();
      }
      mTask = new TimerTask() {
        @Override
        public void run() {
          check();
        }
      };
      new Timer().schedule(mTask, CHECK_DELAY);
    }
  }

  @Override
  public void unregister() {
    synchronized (this) {
      if (mTask != null) {
        mTask.cancel();
      }
    }
    super.unregister();
  }

  private void check() {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        loadInterfaces();

        String current = System.getIfname();

        Logger.debug(String.format("current='%s', ifaces=[%s], haveInterface=%s, isAnyNetInterfaceAvailable=%s",
                current != null ? current : "(null)",
                ifacesToString(), haveInterface(current), isAnyNetInterfaceAvailable()));

        if (haveInterface(current)) {
          onConnectionResumed();
        } else if (current != null) {
          onConnectionLost();
        } else if (isAnyNetInterfaceAvailable()) {
          onNetworkInterfaceChanged();
        }

        updateSidebar();
      }
    });

    synchronized (this) {
      mTask = null;
    }
  }

  private synchronized List<NetworkMenuItem> buildInterfacesMenu() {
    List<NetworkMenuItem> result = new ArrayList<>(ifaces.size());

    for(String iface : ifaces) {
      //TODO: get network icon
      result.add(new NetworkMenuItem(iface, R.drawable.ic_wifi_signal_4));
    }

    return result;
  }

  private synchronized boolean isAnyNetInterfaceAvailable() {
    return !ifaces.isEmpty();
  }

  private synchronized boolean haveInterface(String ifname) {
    return ifaces.contains(ifname);
  }

  private synchronized void loadInterfaces() {
    ifaces = Network.getAvailableInterfaces();
  }

  private void onConnectionResumed() {
    if (!offline)
      return;
    offline = false;
    System.markInitialNetworkTargetsAsConnected();
    Services.getNetworkRadar().startAsync();
  }

  private void loadDefaultInterface() {
    offline = false;
    System.setIfname(null);
    onNetworkInterfaceChanged();
  }

  private void promptForLoadingDefaultInterface() {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        new ConfirmDialog(activity.getString(R.string.connection_lost),
                activity.getString(R.string.connection_lost_prompt), activity,
                new ConfirmDialog.ConfirmDialogListener() {
                  @Override
                  public void onConfirm() {
                    loadDefaultInterface();
                  }

                  @Override
                  public void onCancel() {
                  }
                }).show();
      }
    });
  }

  private void onConnectionLost() {
    if (offline)
      return;

    offline = true;

    Services.getNetworkRadar().stopAsync();
    System.markNetworkAsDisconnected();

    promptForLoadingDefaultInterface();
  }

  private void onNetworkInterfaceChanged() {
    String toastMessage = null;

    Services.getNetworkRadar().stopAsync();

    if (!System.reloadNetworkMapping()) {
      String ifname = System.getIfname();

      ifname = ifname == null ? activity.getString(R.string.any_interface) : ifname;

      toastMessage = String.format(activity.getString(R.string.error_initializing_interface), ifname);
    } else {
      Services.getNetworkRadar().startAsync();
    }

    final String msg = toastMessage;

    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        if (msg != null) {
          Toast.makeText(activity, msg, Toast.LENGTH_LONG).show();
        }
      }
    });
  }

  public void tryLoadDefaultInterface() {
    String current = System.getIfname();

    if(current != null) {
      String def = System.getDefaultIfname();

      if(def != null && !current.equals(def)) {
        promptForLoadingDefaultInterface();
      }
    } else {
      loadDefaultInterface();
    }
  }

  public void updateSidebar() {
    List<NetworkMenuItem> items = buildInterfacesMenu();

    activity.updateNetworks(items);
  }

  private class NetworkMenuItem implements AbstractSidebarActivity.NavNetworkItem {
    private final String name;
    private final int drawableId;

    public NetworkMenuItem(String name, int drawableId) {
      this.name = name;
      this.drawableId = drawableId;
    }

    @Override
    public String getName() {
      return name;
    }

    @Override
    public int getDrawableId() {
      return drawableId;
    }
  }
}
