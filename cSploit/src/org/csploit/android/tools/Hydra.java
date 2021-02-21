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
import org.csploit.android.events.Status;
import org.csploit.android.events.Event;
import org.csploit.android.events.Login;
import org.csploit.android.net.Target;

public class Hydra extends Tool
{
  public Hydra() {
    mHandler = "hydra";
    mCmdPrefix = null;
  }

  public static abstract class AttemptsReceiver extends Child.EventReceiver
  {
    public void onEnd( int exitCode ) {
      if( exitCode != 0 )
        Logger.error( "hydra exited with code " + exitCode );
    }
    public void onDeath(int signal) {
      Logger.error("hydra killed by signal " + signal);
    }


    public abstract void onAttemptStatus(int rate, int progress, int left);

    public abstract void onWarning(String message);

    public abstract void onError(String message);

    public abstract void onAccountFound(String login, String password);
        public void onEvent(Event e) {
            if(e instanceof Status) {
                Status a = (Status) e;
                onAttemptStatus(a.rate, a.sent, a.left);
            }
        /*else if(e instanceof Message) {
          Message m = (Message)e;

          if(m.severity == Message.Severity.ERROR) {
            onError(m.message);
          } else if(m.severity == Message.Severity.WARNING) {
            onWarning(m.message);
          } else {
            Logger.error("Unknown event: " + e);
          }
        } */
            else if(e instanceof Login) {
                Login a = (Login) e;
                onAccountFound(a.login, a.password);
            } else if(e instanceof Status) {
                Status s = (Status) e;
                onAttemptStatus(s.rate, s.sent, s.left);
            }
            else {
                Logger.error("Unknown event: " + e);
            }
        }
    }

    public Child crack(Target target, int port, String service, String charset, int minlength, int maxlength, String username, String userWordlist,
                       String passWordlist, boolean stop, boolean loop, Integer threads, String s, AttemptsReceiver receiver) throws ChildManager.ChildNotStartedException {
        String command = "";
        if(loop)
          command += "-u ";
        if(stop)
          command += "-f ";
        command += String.format("-t %d ", threads);

        if(userWordlist != null)
          command += "-L " + userWordlist;
        else
          command += "-l " + username;

        if(passWordlist != null)
          command += " -P " + passWordlist;

        else
          command += " -x " + "\"" + minlength + ":" + maxlength + ":" + charset + "\"";

        command += " -t 1 " + service + "://" + target.getCommandLineRepresentation() + ":" + port;

        if(service.equalsIgnoreCase("http-get") || service.equalsIgnoreCase("https-get") || service.equalsIgnoreCase("http-get-form") ||
                service.equalsIgnoreCase("https-get-form") || service.equalsIgnoreCase("http-post-form") || service.equalsIgnoreCase("https-post-form"))
            command += s;

        return super.async(command, receiver);
    }
}