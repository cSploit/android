package org.csploit.android.net;

import android.content.Context;
import android.content.Intent;

import org.csploit.android.core.Child;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.core.ChildManager;

import java.net.InetAddress;

/**
 * network-radar process manager
 */
public class NetworkRadar {
  public static final String TARGET_UPDATE = "NetworkRadar.action.TARGET_UPDATE";
  public static final String NRDR_STOPPED = "NetworkRadar.action.STOPPED";

  private Child process;
  private Context mContext;
  private boolean mNotifyStopped;

  public NetworkRadar(Context context) {
    mContext = context;
    mNotifyStopped = true;
  }

  public void start() throws ChildManager.ChildNotStartedException {
    process = System.getTools().networkRadar.start( new org.csploit.android.tools.NetworkRadar.HostReceiver() {
      @Override
      public void onHostFound(byte[] macAddress, InetAddress ipAddress, String name) {
        Target t;
        boolean notify;

        notify = false;

        t = System.getTargetByAddress(ipAddress);

        if(t==null) {
          t = new Target(ipAddress, macAddress);

          if(name != null)
            t.setAlias(name);

          System.addOrderedTarget(t);
          notify = true;
        } else {
          if( !t.isConnected() ) {
            t.setConneced(true);
            notify = true;
          }

          if (name != null && !name.equals(t.getAlias())) {
            t.setAlias(name);
            notify = true;
          }
        }

        if(notify) {
          sendTargetUpdateNotification();
        }
      }

      @Override
      public void onHostLost(InetAddress ipAddress) {
        Target t = System.getTargetByAddress(ipAddress);

        if(t != null) {
          t.setConneced(false);
          sendTargetUpdateNotification();
        }
      }

      public void onEnd(int exitValue) {
        sendStoppedNotification();
      }

      public void onDeath(int signal) {
        Logger.error("network-radar has been killed by signal #" + signal);
        sendStoppedNotification();
      }
    });
  }

  public void stop(boolean notify) {
    mNotifyStopped = notify;
    if(process == null || !process.running)
      return;
    process.kill(2);
  }

  public void kill() {
    if(process == null || !process.running)
      return;
    process.kill(9);
  }

  public void finalize() throws Throwable {
    kill();
    super.finalize();
  }

  private void sendTargetUpdateNotification(){
    mContext.sendBroadcast(new Intent(TARGET_UPDATE));
  }

  private void sendStoppedNotification() {
    if(mNotifyStopped) {
      mContext.sendBroadcast(new Intent(NRDR_STOPPED));
    }
  }

  public boolean isRunning() {
    return process != null && process.running;
  }
}
