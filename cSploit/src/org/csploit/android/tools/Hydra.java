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
import org.csploit.android.events.Attempts;
import org.csploit.android.events.Event;
import org.csploit.android.events.Login;
import org.csploit.android.events.Message;
import org.csploit.android.net.Target;

public class Hydra extends Tool
{
  public Hydra() {
    mHandler = "hydra";
    mCmdPrefix = null;
  }

  public static abstract class AttemptReceiver extends Child.EventReceiver
  {
    public abstract void onAttemptStatus(long progress, long total);

    public abstract void onWarning(String message);

    public abstract void onError(String message);

    public abstract void onAccountFound(String login, String password);

    public void onEvent(Event e) {
      if(e instanceof Attempts) {
        Attempts a = (Attempts)e;
        onAttemptStatus(a.sent, a.sent + a.left);
      } else if(e instanceof Message) {
        Message m = (Message)e;

        if(m.severity == Message.Severity.ERROR) {
          onError(m.message);
        } else if(m.severity == Message.Severity.WARNING) {
          onWarning(m.message);
        } else {
          Logger.error("Unknown event: " + e);
        }
      } else if(e instanceof Login) {
        Login login = (Login) e;

        onAccountFound(login.login, login.password);
      } else {
        Logger.error("Unknown event: " + e);
      }
    }
  }

  public Child crack(Target target, int port, String service, String charset, int minlength, int maxlength, String username, String userWordlist, String passWordlist, AttemptReceiver receiver) throws ChildManager.ChildNotStartedException {
    String command = "-F ";

    if(userWordlist != null)
      command += "-L " + userWordlist;

    else
      command += "-l " + username;

    if(passWordlist != null)
      command += " -P " + passWordlist;

    else
      command += " -x \"" + minlength + ":" + maxlength + ":" + charset + "\" ";

    command += " -s " + port + " -V -t 10 " + target.getCommandLineRepresentation() + " " + service;

    if(service.equalsIgnoreCase("http-head"))
      command += " /";

    return super.async(command, receiver);
  }
}