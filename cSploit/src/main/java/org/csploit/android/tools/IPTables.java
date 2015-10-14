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

import org.csploit.android.core.System;
import org.csploit.android.core.Logger;

public class IPTables extends Tool
{
  public IPTables(){
    mHandler = "raw";
    mCmdPrefix = "iptables";
  }

  public void trafficRedirect(String to){
    Logger.debug("Redirecting traffic to " + to);

    try{
      super.run("-t nat -A PREROUTING -j DNAT -p tcp --to " + to);
    }
    catch(Exception e){
      System.errorLogging(e);
    }
  }

  public void undoTrafficRedirect(String to){
    Logger.debug("Undoing traffic redirection");

    try{
      super.run("-t nat -D PREROUTING -j DNAT -p tcp --to " + to);
    }
    catch(Exception e){
      System.errorLogging(e);
    }
  }

  public void portRedirect(int from, int to, boolean cleanRules){
    Logger.debug("Redirecting traffic from port " + from + " to port " + to);

    try{
      if (cleanRules) {
        // clear nat
        super.run("-t nat -F");
        // clear
        super.run("-F");
        // post route
        super.run("-t nat -I POSTROUTING -s 0/0 -j MASQUERADE");
        // accept all
        super.run("-P FORWARD ACCEPT");
      }
      // add rule
      super.run("-t nat -A PREROUTING -j DNAT -p tcp --dport " + from + " --to " + System.getNetwork().getLocalAddressAsString() + ":" + to);
    }
    catch(Exception e){
      System.errorLogging(e);
    }
  }

  public void undoPortRedirect(int from, int to){
    Logger.debug("Undoing port redirection");

    try{
      // clear nat
      super.run("-t nat -F");
      // clear
      super.run("-F");
      // remove post route
      super.run("-t nat -D POSTROUTING -s 0/0 -j MASQUERADE");
      // remove rule
      super.run("-t nat -D PREROUTING -j DNAT -p tcp --dport " + from + " --to " + System.getNetwork().getLocalAddressAsString() + ":" + to);
    }
    catch(Exception e){
      System.errorLogging(e);
    }
  }
}
