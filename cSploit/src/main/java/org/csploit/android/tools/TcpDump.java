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

import java.net.InetAddress;

import org.csploit.android.core.Child;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.events.Event;
import org.csploit.android.events.Packet;

public class TcpDump extends Tool{

  public TcpDump() {
    mHandler = "tcpdump";
    mCmdPrefix = null;
  }

  public static abstract class TcpDumpReceiver extends Child.EventReceiver {

    @Override
    public void onEvent(Event e) {
      if(e instanceof Packet) {
        Packet p = (Packet)e;
        onPacket(p.src, p.dst, p.len);
      } else {
        Logger.warning("Unknown event: " + e);
      }
    }

    public abstract void onPacket(InetAddress src, InetAddress dst, int len);
  }

  public Child sniff(String filter, String pcap, TcpDumpReceiver receiver) throws ChildManager.ChildNotStartedException {

    StringBuilder sb = new StringBuilder("-nvs 0 ");

    if(pcap != null) {
      // TODO: find a way to receive tcpdump output when saving to a file
      // NOTE: tcpdump -w - | tee file.pcap | tcpdump -r -
      sb.append("-Uw '");
      sb.append(pcap);
      sb.append("' ");
    } else {
      sb.append("-l ");
    }

    if(filter != null) {
      sb.append(filter);
    }

    return super.async(sb.toString(), receiver);
  }

  public void sniff(TcpDumpReceiver receiver) throws ChildManager.ChildNotStartedException {
    sniff(null, null, receiver);
  }
}
