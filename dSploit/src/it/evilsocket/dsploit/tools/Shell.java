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

import java.io.IOException;

import it.evilsocket.dsploit.core.Child;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.ChildManager;

public class Shell extends Raw {

  protected String mPreCmd;

  public Shell() {
    mHandler = "raw";
    mCmdPrefix = "sh";
    mPreCmd = null;
  }

  /**
   * run a shell command and return it's exit value
   * @param cmd the shell command to execute
   * @return the command exit status
   */
  @Override
  public int run(String cmd, RawReceiver receiver) throws InterruptedException, ChildManager.ChildDiedException, ChildManager.ChildNotStartedException {
    if(mPreCmd == null)
      return super.run("-c '" + cmd.replaceAll("'", "\\'") + "'", receiver);

    Child c = super.async(null, receiver);

    try {
      c.send(mPreCmd + cmd + "; exit $?\n");
    } catch (IOException e) {
      System.errorLogging(e);
      throw new ChildManager.ChildNotStartedException();
    }

    c.join();

    if(c.signal>=0)
      throw new ChildManager.ChildDiedException(c.signal);

    return c.exitValue;
  }

  public int run(String cmd) throws InterruptedException, ChildManager.ChildDiedException, ChildManager.ChildNotStartedException {
    return this.run(cmd, null);
  }

  /**
   * run a Shell command asynchronously
   * @param cmd the shell command to execute
   */
  public Child async(String cmd, RawReceiver receiver) throws ChildManager.ChildNotStartedException {
    Child c;

    if(mPreCmd==null) {
      if(cmd != null) {
        c = super.async("-c '" + cmd.replace("'", "\\'") + "'", receiver);
      } else {
        c = super.async(receiver);
      }
    } else {
      c = super.async(null, receiver);
      try {
        if(cmd!=null) {
          c.send(mPreCmd + cmd + "; exit $?\n");
        } else {
          c.send(mPreCmd);
        }
      } catch (IOException e) {
        System.errorLogging(e);
        throw new ChildManager.ChildNotStartedException();
      }
    }

    return c;
  }
}