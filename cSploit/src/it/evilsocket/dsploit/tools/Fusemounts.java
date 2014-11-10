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
import it.evilsocket.dsploit.events.Event;

public class Fusemounts extends Tool
{
  public Fusemounts() {
    mHandler = "fusemounts";
    mCmdPrefix = null;
  }

  public static abstract class fuseReceiver extends Child.EventReceiver {

    @Override
    public void onEvent(Event e) {
      //TODO
    }

    public abstract void onNewMountpoint(String source, String mountpoint);
  }

  public Child getSources(fuseReceiver receiver) throws ChildManager.ChildNotStartedException {
    return super.async(receiver);
  }
}