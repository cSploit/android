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

public abstract class Tool
{
  protected boolean mEnabled = true;
  protected String mHandler = null;
  protected String mCmdPrefix = null;

  public int run(String args, Child.EventReceiver receiver) throws InterruptedException, ChildManager.ChildDiedException, ChildManager.ChildNotStartedException {

    // debugging inheritance
    String stack = "";
    for(StackTraceElement element : Thread.currentThread().getStackTrace()) {
      if(element.getClassName().startsWith("it.evilsocket.dsploit"))
        stack += String.format("at %s.%s(%s)\n",
                element.getClassName(), element.getMethodName(),
                (element.getLineNumber() >= 0 ? element.getFileName() + ":" + element.getLineNumber() : "NativeMethod"));
    }
    Logger.debug(stack);

    if(!mEnabled) {
      Logger.warning(mHandler + ":" + mCmdPrefix + ": disabled");
      throw new ChildManager.ChildNotStartedException();
    }

    if(mCmdPrefix!=null) {
      args = mCmdPrefix + " " + args;
    }

    return ChildManager.exec(mHandler, args, receiver);
  }

  public int run(String args) throws InterruptedException, ChildManager.ChildDiedException, ChildManager.ChildNotStartedException {
    return run(args, null);
  }

  public Child async(String args, Child.EventReceiver receiver) throws ChildManager.ChildNotStartedException {

    // debugging inheritance
    String stack = "";
    for(StackTraceElement element : Thread.currentThread().getStackTrace()) {
      if(element.getClassName().startsWith("it.evilsocket.dsploit"))
        stack += String.format("at %s.%s(%s)\n",
                element.getClassName(), element.getMethodName(),
                (element.getLineNumber() >= 0 ? element.getFileName() + ":" + element.getLineNumber() : "NativeMethod"));
    }
    Logger.debug(stack);

    if(!mEnabled) {
      Logger.warning(mHandler + (mCmdPrefix != null ? ":" + mCmdPrefix : "" ) + ": disabled");
      throw new ChildManager.ChildNotStartedException();
    }

    if(mCmdPrefix!=null) {
      if(args != null) {
        args = mCmdPrefix + " " + args;
      } else {
        args = mCmdPrefix;
      }
    }

    return ChildManager.async(mHandler, args, receiver);
  }

  public Child async(String args) throws ChildManager.ChildNotStartedException {
    return this.async(args, null);
  }

  public Child async(Child.EventReceiver receiver) throws ChildManager.ChildNotStartedException {
    return async(null, receiver);
  }

  public boolean isEnabled() {
    return mEnabled;
  }

  public void setEnabled() {
    if(ChildManager.handlers != null) {
      mEnabled = ChildManager.handlers.contains(mHandler);
    }
  }
}
