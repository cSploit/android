/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *             Massimo Dragano aka tux_mind <massimo.dragano@gmail.com>
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

import it.evilsocket.dsploit.net.msfrpc;
import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import java.util.*;

import java.math.*;
import java.security.*;

import android.util.Log;

/* Implements a class for writing commands to a shell and firing an
   event when the command is successfully executed (with its output) */
public class ShellSession extends Thread {
  protected msfrpc 						connection;
  protected OutputReceiver				receiver;
  protected LinkedList<String>   			commands  = new LinkedList<String>();
  protected String        				session;
  private   SecureRandom 					random	  = new SecureRandom();
  private	  ArrayList<String>				array_buffer = new ArrayList<String>();

  public ShellSession(msfrpc connection, String session, OutputReceiver receiver) {
    this.connection = connection;
    this.session = session;
    this.receiver = receiver;
    new Thread(this).start();
  }

  protected void processCommand(String command) {
    try {
      String marker = new BigInteger(130, random).toString(32) + "\n";
      StringBuffer buffer = new StringBuffer();
      long start;
      int newline;

      buffer.append(command);
      buffer.append("\n");
      buffer.append("echo\n");
      buffer.append("echo " + marker);
      array_buffer.clear();
      array_buffer.add(session);
      array_buffer.add(buffer.toString());
			/* write our command to whateverz */
      connection.execute("session.shell_write", array_buffer);

			/* read until we encounter AAAAAAAAAA */
      buffer.delete(0,buffer.length());

			/* loop forever waiting for response to come back. If session is dead
			   then this loop will break with an exception */
      start = System.currentTimeMillis();
      while ((System.currentTimeMillis() - start) < 60000) {
        String data = (readResponse().get("data") + "");

        if (data.length() > 0) {
          if((newline = data.indexOf('\n')) > 0)
          {
            buffer.append(data.substring(0, newline));
            receiver.onNewLine(buffer.toString());
            buffer.delete(0,buffer.length());
            data = data.substring(newline);
          }
          else
            buffer.append(data);
          if(data.endsWith(marker))
          {
            receiver.onEnd(0);
            return;
          }
        }

        Thread.sleep(100);
      }
      Log.e("SHELLSESSION", "session " + session + " -> \"" + command + "\" TIMEDOUT");
    }
    catch (Exception ex) {
      Log.e("SHELLSESSION", "session " + session + " -> \"" + command + "\" Throw \"" + ex.getMessage() + "\"");
      ex.printStackTrace();
    }
  }

  public void addCommand(String command) {
    synchronized (this) {
      if (command.trim().equals("")) {
        return;
      }
      commands.add(command);
    }
  }

  protected String grabCommand() {
    synchronized (this) {
      return (String)commands.pollFirst();
    }
  }

  /* keep grabbing commands, acquiring locks, until everything is executed */
  public void run() {
    while (true) {
      try {
        String next = grabCommand();
        if (next != null) {
          processCommand(next);
          Thread.sleep(50);
        }
        else Thread.sleep(500);
      }
      catch (Exception ex) {
        Log.e("SHELLSESSION", "This session appears to be dead! " + session + ", " + ex);
        return;
      }
    }
  }

  private Map readResponse() throws Exception {
    return (Map)(connection.execute("session.shell_read", new ArrayList<String>() {{ add(session);}}));
  }
}