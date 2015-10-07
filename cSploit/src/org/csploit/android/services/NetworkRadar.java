package org.csploit.android.services;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.view.MenuItem;

import org.csploit.android.R;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.helpers.ThreadHelper;
import org.csploit.android.net.Endpoint;
import org.csploit.android.net.Network;
import org.csploit.android.net.Target;
import org.csploit.android.tools.NMap;
import org.csploit.android.tools.NetworkRadar.HostReceiver;

import java.net.InetAddress;

/**
 * network-radar process manager
 */
public class NetworkRadar extends NativeService implements MenuControllableService {
  public static final String NRDR_STOPPED = "NetworkRadar.action.STOPPED";
  public static final String NRDR_STARTED = "NetworkRadar.action.STARTED";
  public static final String NRDR_START_FAILED = "NetworkRadar.action.START_FAILED";

  private boolean autoScan = true;

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
    item.setEnabled(System.getTools().networkRadar.isEnabled() && System.getNetwork() != null);
  }

  public void onAutoScanChanged() {
    autoScan = System.isCoreInitialized() && System.getSettings().getBoolean("PREF_AUTO_PORTSCAN", true);
    Logger.info("autoScan has been set to " + autoScan);
  }

  public boolean isAutoScanEnabled() {
    return autoScan;
  }

  public void onNewTargetFound(final Target target) {
    if(!autoScan || target.getType() == Target.Type.NETWORK)
      return;
    ThreadHelper.getSharedExecutor().execute(new Runnable() {
      @Override
      public void run() {
        try {
          System.getTools().nmap.synScan(target, new ScanReceiver(target));
        } catch (ChildManager.ChildNotStartedException e) {
          System.errorLogging(e);
        }
      }
    });
  }

  private class Receiver extends HostReceiver {

    @Override
    public void onHostFound(byte[] macAddress, InetAddress ipAddress, String name) {
      Target t;
      boolean notify = false;
      boolean justFound;

      t = System.getTargetByAddress(ipAddress);
      justFound = t == null;

      if(justFound) {
        t = new Target(ipAddress, macAddress);
        t.setAlias(name);
        System.addOrderedTarget(t);
        notify = true;
      } else {
        if (!t.isConnected()) {
          t.setConneced(true);
          notify = true;
        }

        if (name != null && !name.equals(t.getAlias())) {
          t.setAlias(name);
          notify = true;
        }

        //TODO: remove me ( and imports )
        Endpoint e = new Endpoint(ipAddress, macAddress);
        if(!e.equals(t.getEndpoint())) {
          Logger.warning(
                  String.format("target '%s' changed it's mac address from '%s' to '%s'",
                          t.toString(), t.getEndpoint().getHardwareAsString(), e.getHardwareAsString()));
        }
      }

      if(!notify)
        return;

      if(justFound) {
        System.notifyTargetListChanged();
      } else {
        System.notifyTargetListChanged(t);
      }
    }

    @Override
    public void onHostLost(InetAddress ipAddress) {
      Target t = System.getTargetByAddress(ipAddress);

      if(t != null && t.isConnected()) {
        t.setConneced(false);
        System.notifyTargetListChanged(t);
      }
    }

    @Override
    public void onStart(String cmd) {
      sendIntent(NRDR_STARTED);
      System.scanThemAll();
    }

    public void onEnd(int exitValue) {
      sendIntent(NRDR_STOPPED);
    }

    public void onDeath(int signal) {
      Logger.error("network-radar has been killed by signal #" + signal);
      sendIntent(NRDR_STOPPED);
    }
  }

  private class ScanReceiver extends NMap.SynScanReceiver {

    private final Target target;

    public ScanReceiver(Target target) {
      this.target = target;
    }

    @Override
    public void onPortFound(int port, String protocol) {
      target.addOpenPort(port, Network.Protocol.fromString(protocol));
    }
  }
}
