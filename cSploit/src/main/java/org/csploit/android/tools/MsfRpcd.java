package org.csploit.android.tools;

import org.csploit.android.core.*;
import org.csploit.android.core.System;
import org.csploit.android.events.Event;
import org.csploit.android.events.Ready;

/**
 * MetaSploit RPC Daemon
 */
public class MsfRpcd extends Msf {

  public static abstract class MsfRpcdReceiver extends Child.EventReceiver {
    @Override
    public void onEvent(Event e) {
      if(e instanceof Ready)
        onReady();
    }

    public abstract void onReady();
  }

  public MsfRpcd() {
    mHandler = "msfrpcd";
  }

  /**
   * start an MsfRpcd
   * @param receiver  will be notified when the daemon it's ready to accept connections
   */
  public Child async(String user, String pswd, int port, boolean ssl, MsfRpcdReceiver receiver) throws ChildManager.ChildNotStartedException {
    return async(
            String.format("-P '%s' -U '%s' -p '%d' -a 127.0.0.1 -n %s -t Msg -f",
            pswd, user, port, (ssl ? "" : "-S")),
            receiver);
  }

  /*public static boolean isLocal() {
    return System.getSettings().getString("MSF_RPC_HOST", "127.0.0.1").equals("127.0.0.1");
  }*/

  public static boolean isInstalled() {
    return System.getLocalMsfVersion() != null;
  }
}
