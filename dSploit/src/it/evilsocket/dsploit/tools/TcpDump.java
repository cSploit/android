/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *
 * dSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package it.evilsocket.dsploit.tools;

import android.content.Context;

import java.io.IOException;

import it.evilsocket.dsploit.core.Shell.OutputReceiver;

public class TcpDump extends Tool{
  public TcpDump(Context context){
    super("tcpdump/tcpdump", context);
  }

  public Thread sniff(String filter, String pcap, OutputReceiver receiver){
    if(pcap == null)
      return super.async("-l -vv -s 0 " + (filter == null ? "" : filter), receiver);

    else
      return super.async("-l -vv -s 0 -w '" + pcap + "' " + (filter == null ? "" : filter), receiver);
  }

  public void sniff(OutputReceiver receiver){
    sniff(null, null, receiver);
  }
}
