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

package org.csploit.android.tools;

import org.csploit.android.core.ChildManager;
import org.csploit.android.core.System;
import org.csploit.android.core.Child;
import org.csploit.android.core.Logger;
import org.csploit.android.events.Event;
import org.csploit.android.events.Host;
import org.csploit.android.events.HostLost;

import java.net.InetAddress;

public class NetworkRadar extends Tool {

  public NetworkRadar() {
    mHandler = "network-radar";
    mCmdPrefix = null;
  }

  public static abstract class HostReceiver extends Child.EventReceiver {

    public abstract void onHostFound(byte[] macAddress, InetAddress ipAddress, String name);
    public abstract void onHostLost(InetAddress ipAddress);

    public void onEvent(Event e) {
      if ( e instanceof Host ) {
        Host h = (Host)e;
        onHostFound(h.ethAddress, h.ipAddress, h.name);
      } else if ( e instanceof HostLost ) {
        onHostLost(((HostLost)e).ipAddress);
      } else {
        Logger.error("Unknown event: " + e);
      }
    }
  }

  public Child start(HostReceiver receiver) throws ChildManager.ChildNotStartedException {
    String ifName;

    if(System.getNetwork() == null) {
      throw new ChildManager.ChildNotStartedException();
    }

    ifName = System.getNetwork().getInterface().getDisplayName();

    return async(ifName, receiver);
  }
}
