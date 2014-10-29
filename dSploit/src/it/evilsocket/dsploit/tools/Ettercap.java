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
package it.evilsocket.dsploit.tools;

import it.evilsocket.dsploit.core.Child;
import it.evilsocket.dsploit.core.ChildManager;
import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.events.Account;
import it.evilsocket.dsploit.events.Event;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.events.Ready;
import it.evilsocket.dsploit.net.Target;

public class Ettercap extends Tool
{
  public Ettercap() {
    mHandler = "ettercap";
    mCmdPrefix = null;
  }

  public static abstract class OnAccountListener extends Child.EventReceiver
  {
    @Override
    public void onEvent(Event e) {
      if(e instanceof Ready) {
        onReady();
      } else if(e instanceof Account) {
        Account a = (Account)e;
        onAccount(a.protocol, a.address.getHostAddress(), a.username, a.password);
      } else {
        Logger.warning("unknown event: " + e);
      }
    }

    public abstract void onAccount(String protocol, String address, String username, String password);
    public abstract void onReady();
  }

  public Child dissect(Target target, OnAccountListener listener) throws ChildManager.ChildNotStartedException {
    StringBuilder sb = new StringBuilder();

    sb.append("-Tpq -i ");
    try {
      sb.append(System.getNetwork().getInterface().getDisplayName());
    } catch (Exception e) {
      System.errorLogging(e);
      throw new ChildManager.ChildNotStartedException();
    }


    // poison the entire network
    if(target.getType() == Target.Type.NETWORK)
      sb.append(" /// ///");
      // router -> target poison
    else {
      sb.append("/");
      sb.append(target.getCommandLineRepresentation());
      sb.append("// ///");
    }

    return super.async(sb.toString(), listener);
  }
}