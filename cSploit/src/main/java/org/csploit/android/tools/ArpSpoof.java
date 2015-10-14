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

import org.csploit.android.core.Child;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.events.Event;
import org.csploit.android.events.Message;
import org.csploit.android.net.Target;
import org.csploit.android.net.Target.Type;

public class ArpSpoof extends Tool
{
  public ArpSpoof() {
    mHandler = "arpspoof";
  }

  public abstract static class ArpSpoofReceiver extends Child.EventReceiver {

    @Override
    public void onEvent(Event e) {
      if(e instanceof Message) {
        Message m = (Message)e;
        if(m.severity == Message.Severity.ERROR)
          onError(m.message);
        else
          Logger.warning("unexpected message from arpspoof: " + m);
      } else {
        Logger.warning("unknown event " + e);
      }
    }

    public abstract void onError(String line);
  }

  public Child spoof(Target target, ArpSpoofReceiver receiver) throws ChildManager.ChildNotStartedException {
    String commandLine = "";

    try{
      if(target.getType() == Type.NETWORK)
        commandLine = "-i " + System.getNetwork().getInterface().getDisplayName() + " " + System.getGatewayAddress();

      else
        commandLine = "-i " + System.getNetwork().getInterface().getDisplayName() + " -t " + target.getCommandLineRepresentation() + " " + System.getGatewayAddress();
    }
    catch(Exception e){
      System.errorLogging(e);
    }

    return super.async(commandLine, receiver);
  }
}