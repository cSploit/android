package org.csploit.android.services;

import android.app.Activity;
import android.content.Context;
import android.view.MenuItem;

import org.csploit.android.R;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.core.ChildManager;
import org.csploit.android.net.Target;

import java.net.InetAddress;

/**
 * network-radar process manager
 */
public class NetworkRadar extends NativeService implements MenuControllableService {
  public static final String NRDR_CHANGED = "NetworkRadar.action.TARGET_CHANGED";
  public static final String NRDR_STOPPED = "NetworkRadar.action.STOPPED";
  public static final String NRDR_STARTED = "NetworkRadar.action.STARTED";
  public static final String NRDR_START_FAILED = "NetworkRadar.action.START_FAILED";

  public NetworkRadar(Context context) {
    this.context = context;
  }

  public boolean start() {
    stop();
    try {
      nativeProcess = System.getTools().networkRadar
        .start(new Receiver());
      return true;
    } catch (ChildManager.ChildNotStartedException e) {
      Logger.error(e.getMessage());
      sendIntent(NRDR_START_FAILED);
    }
    return false;
  }

  @Override
  protected int getStopSignal() {
    return 2;
  }

  @Override
  public void onMenuClick(Activity activity, final MenuItem item) {
    if(isRunning()) {
      stop();
    } else {
      start();
    }
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        buildMenuItem(item);
      }
    });
  }

  @Override
  public void buildMenuItem(MenuItem item) {
    item.setTitle(isRunning() ? R.string.stop_monitor : R.string.start_monitor);
    item.setEnabled(System.getTools().networkRadar.isEnabled());
  }

  private class Receiver extends org.csploit.android.tools.NetworkRadar.HostReceiver {

    @Override
    public void onHostFound(byte[] macAddress, InetAddress ipAddress, String name) {
      Target t;
      boolean notify = false;

      t = System.getTargetByAddress(ipAddress);

      if(t==null) {
        t = new Target(ipAddress, macAddress);

        System.addOrderedTarget(t);

        notify = true;
      }

      if( !t.isConnected() ) {
        t.setConneced(true);
        notify = true;
      }

      if (name != null && !name.equals(t.getAlias())) {
        t.setAlias(name);
        notify = true;
      }

      if(notify) {
        sendIntent(NRDR_CHANGED);
      }
    }

    @Override
    public void onHostLost(InetAddress ipAddress) {
      Target t = System.getTargetByAddress(ipAddress);

      if(t != null) {
        t.setConneced(false);
        sendIntent(NRDR_CHANGED);
      }
    }

    @Override
    public void onStart(String cmd) {
      sendIntent(NRDR_STARTED);
    }

    public void onEnd(int exitValue) {
      sendIntent(NRDR_STOPPED);
    }

    public void onDeath(int signal) {
      Logger.error("network-radar has been killed by signal #" + signal);
      sendIntent(NRDR_STOPPED);
    }
  }
}
