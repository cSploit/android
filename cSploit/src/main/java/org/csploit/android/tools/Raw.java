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
import org.csploit.android.events.Event;
import org.csploit.android.events.Newline;

/**
 * the "Raw" tool, it simply read stdout line by line.
 */
public class Raw extends Tool {

  public static abstract class RawReceiver extends Child.EventReceiver {

    @Override
    public void onEvent(Event e) {
      if(e instanceof Newline)
        onNewLine(((Newline)e).line);
      else
        Logger.warning("unknown event: " + e);
    }

    public abstract void onNewLine(String line);
  }

  public Raw() {
    mHandler = "raw";
    mCmdPrefix = null;
  }

  public int run(String cmd, RawReceiver receiver) throws InterruptedException, ChildManager.ChildDiedException, ChildManager.ChildNotStartedException {
    return super.run(cmd, receiver);
  }

  @Override
  public int run(String cmd) throws InterruptedException, ChildManager.ChildDiedException, ChildManager.ChildNotStartedException {
    return this.run(cmd, null);
  }

  public Child async(String cmd, RawReceiver receiver) throws ChildManager.ChildNotStartedException {
    return super.async(cmd, receiver);
  }
}
