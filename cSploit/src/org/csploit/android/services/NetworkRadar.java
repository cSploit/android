package org.csploit.android.services;

import android.app.Activity;
import android.view.MenuItem;
import android.widget.BaseAdapter;

import org.csploit.android.R;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.core.ChildManager;
import org.csploit.android.net.Target;

import java.net.InetAddress;
import java.util.LinkedList;
import java.util.List;

/**
 * network-radar process manager
 */
public class NetworkRadar extends NativeService implements MenuControllableService {
  public static final String NRDR_STOPPED = "NetworkRadar.action.STOPPED";
  public static final String NRDR_STARTED = "NetworkRadar.action.STARTED";
  public static final String NRDR_START_FAILED = "NetworkRadar.action.START_FAILED";

  private final List<TargetTask> taskList = new LinkedList<>();
  private final TargetSubmitter submitter = new TargetSubmitter();
  private BaseAdapter adapter;

  public NetworkRadar(Activity context) {
    this.context = context;
  }

  public void setAdapter(BaseAdapter adapter) {
    this.adapter = adapter;
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
      TargetTask task = null;

      t = System.getTargetByAddress(ipAddress);

      if(t==null) {
        t = new Target(ipAddress, macAddress);
        t.setAlias(name);

        task = new TargetTask(t, true, null, true);
      } else {
        boolean aliasChanged = name != null && !name.equals(t.getAlias());

        if(aliasChanged || !t.isConnected()) {
          task = new TargetTask(t, false, (aliasChanged ? name : t.getAlias()), true);
        }
      }

      if(task == null)
        return;

      synchronized (taskList) {
        taskList.add(task);
      }

      ((Activity)context).runOnUiThread(submitter);
    }

    @Override
    public void onHostLost(InetAddress ipAddress) {
      Target t = System.getTargetByAddress(ipAddress);

      if(t == null) {
        return;
      }

      synchronized (taskList) {
        taskList.add(
                new TargetTask(t, false, t.getAlias(), false)
        );
      }

      ((Activity)context).runOnUiThread(submitter);
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

  private class TargetTask {
    final Target target;
    final boolean add;
    final String name;
    final boolean connected;

    public TargetTask(Target target, boolean add, String name, boolean connected) {
      this.target = target;
      this.add = add;
      this.name = name;
      this.connected = connected;
    }
  }

  private class TargetSubmitter implements Runnable {
    @Override
    public void run() {
      synchronized (taskList) {
        if(!taskList.isEmpty()) {
          for (TargetTask task : taskList) {
            processTask(task);
          }
          if(adapter != null)
            adapter.notifyDataSetChanged();
        }
      }
    }

    private void processTask(final TargetTask task) {
      if(task.add) {
        System.addOrderedTarget(task.target);
      } else {
        task.target.setConneced(task.connected);
        task.target.setAlias(task.name);
      }
    }

  }


}
