/*
 * This file is part of the cSploit.
 *
 * Copyleft of Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 *
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package it.evilsocket.dsploit.net.metasploit;

import it.evilsocket.dsploit.core.Child;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.ChildManager;
import it.evilsocket.dsploit.events.Event;
import it.evilsocket.dsploit.events.Ready;

/**
 * create instance of the MSF RPC daemon
 */
public class MsfRpcd {

  private Child mProcess = null;

  public static abstract class MsfRpcdReceiver extends Child.EventReceiver {
    @Override
    public void onEvent(Event e) {
      if(e instanceof Ready)
        onReady();
    }

    public abstract void onReady();
  }

  /**
   * start an MsfRpcd
   * @param receiver  will be notified when the daemon it's ready to accept connections
   */
  public void start(String user, String pswd, int port, boolean ssl, MsfRpcdReceiver receiver) throws ChildManager.ChildNotStartedException {
    String args;

    try {
      stop();
    } catch (InterruptedException e) {
      mProcess.kill();
    }

    args = String.format("msfrpcd -P '%s' -U '%s' -p '%d' -a 127.0.0.1 -n %s -t Msg -f",
            pswd, user, port, (ssl ? "" : "-S"));

    mProcess = System.getTools().msf.async(args, receiver);
  }

  public boolean isRunning() {
    return mProcess != null && mProcess.running;
  }

  public static boolean isLocal() {
    return System.getSettings().getString("MSF_RPC_HOST", "127.0.0.1").equals("127.0.0.1");
  }

  public void stop() throws InterruptedException {
    if(mProcess != null && mProcess.running) {
      mProcess.kill(2);
      mProcess.join();
    }
    mProcess = null;
  }

  @Override
  protected void finalize() throws Throwable {
    super.finalize();
    try {
      stop();
    } catch (InterruptedException ignored) { }
  }
}
