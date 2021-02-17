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

public class Shell extends Raw {

  public Shell() {
    mHandler = "raw";
    mCmdPrefix = "sh";
    mEnv = null;
  }

  /**
   * run a shell command and return it's exit value
   * @param cmd the shell command to execute
   * @return the command exit status
   */
  @Override
  public int run(String cmd, RawReceiver receiver) throws InterruptedException, ChildManager.ChildDiedException, ChildManager.ChildNotStartedException {
    return super.run("-c '" + cmd.replace("'", "\\'") + "'", receiver);
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

    if(cmd != null) {
      c = super.async("-c '" + cmd.replace("'", "\\'") + "'", receiver);
    } else {
      c = super.async(receiver);
    }

    return c;
  }
}