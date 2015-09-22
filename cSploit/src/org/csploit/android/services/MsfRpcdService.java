package org.csploit.android.services;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.view.MenuItem;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.net.metasploit.RPCClient;
import org.csploit.android.tools.MsfRpcd;

/**
 * The MSFRPC daemon manager
 */
public class MsfRpcdService extends NativeService implements MenuControllableService {

  public static final String STATUS_ACTION = "MsfRpcdService.action.STATUS";
  public static final String STATUS = "MsfRpcdService.data.STATUS";

  final String host, user, password;
  final int port;
  final boolean ssl;

  @Override
  public void onMenuClick(Activity activity, final MenuItem item) {
    if(isLocal()) {
      if(isRunning()) {
        stop();
      } else {
        start();
      }
    } else {
      if(isConnected()) {
        disconnect();
      } else {
        connect();
      }
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
    item.setTitle(
            isLocal() ?
                    (isRunning() ? R.string.stop_msfrpcd : R.string.start_msfrpcd) :
                    (isConnected() ? R.string.connect_msf : R.string.disconnect_msf));
    item.setEnabled(isAvailable());
  }

  public enum Status {
    STARTING,
    CONNECTED,
    DISCONNECTED,
    STOPPED,
    KILLED,
    START_FAILED,
    CONNECTION_FAILED,
  }

  public MsfRpcdService(Context context) {
    SharedPreferences prefs = System.getSettings();

    host = prefs.getString("MSF_RPC_HOST", "127.0.0.1");
    user = prefs.getString("MSF_RPC_USER", "msf");
    password = prefs.getString("MSF_RPC_PSWD", "msf");
    port = System.MSF_RPC_PORT;
    ssl = prefs.getBoolean("MSF_RPC_SSL", false);

    this.context = context;
  }

  private boolean isLocal() {
    return "127.0.0.1".equals(host);
  }

  public boolean isAvailable() {
    return !isLocal() || (
              System.getLocalMsfVersion() != null &&
              System.getTools().msfrpcd.isEnabled() &&
              !System.isServiceRunning("org.csploit.android.core.UpdateService"));
  }

  @Override
  protected int getStopSignal() {
    return 2;
  }

  @Override
  public boolean start() {
    stop();
    if(!isLocal()) {
      return connect();
    }
    try {
      nativeProcess = System.getTools().msfrpcd.async(user, password, port, ssl, new Receiver());
      return true;
    } catch (ChildManager.ChildNotStartedException e) {
      Logger.error(e.getMessage());
      sendIntent(STATUS_ACTION, STATUS, Status.START_FAILED);
    }
    return false;
  }

  /**
   * connect to this msfrpcd instance
   * @return true if connection succeeded, false otherwise
   */
  public boolean connect() {
    do {
      try {
        System.setMsfRpc(new RPCClient(host, user, password, port, ssl));
        Logger.info("successfully connected to MSF RPC Daemon ");
        sendIntent(STATUS_ACTION, STATUS, Status.CONNECTED);
        return true;
      } catch (Exception e) {
        Logger.error(e.getClass().getName() + ": " + e.getMessage());
      }

      if(isRunning()) {
        try {
          Thread.sleep(1000);
        } catch (InterruptedException e) {
          stop();
        }
      }
    } while(isRunning() && !isConnected());


    sendIntent(STATUS_ACTION, STATUS, Status.CONNECTION_FAILED);
    return false;
  }

  public void disconnect() {
    System.setMsfRpc(null);
    if(!isLocal()) {
      sendIntent(STATUS_ACTION, STATUS, Status.DISCONNECTED);
    }
  }

  private boolean isConnected() {
    return System.getMsfRpc() != null;
  }

  @Override
  public boolean stop() {
    disconnect();
    return super.stop();
  }

  private class Receiver extends MsfRpcd.MsfRpcdReceiver {
    @Override
    public void onReady() {
      connect();
    }

    @Override
    public void onStart(String cmd) {
      sendIntent(STATUS_ACTION, STATUS, Status.STARTING);
    }

    @Override
    public void onDeath(int signal) {
      disconnect();
      sendIntent(STATUS_ACTION, STATUS, Status.KILLED);
    }

    @Override
    public void onEnd(int exitValue) {
      disconnect();
      sendIntent(STATUS_ACTION, STATUS, Status.STOPPED);
    }
  }
}
