/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
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

import android.content.Context;

import java.io.File;
import java.io.IOException;

import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.core.System;

public class Tool
{
  protected String mPath = null;
  protected String mBasename = null;

  public Tool(String name, Context context){
    File stat;

    mPath = context.getFilesDir().getAbsolutePath() + "/tools/" + name;
    stat = new File(mPath);
    if(!stat.exists()) {
      Logger.error("cannot find tool: '"+name+"'");
      Logger.error(mPath +": No such file or directory");
      Logger.error("this tool will be disabled.");
      mPath = "date";
    } else {
      mBasename = stat.getName();
    }
  }

  public Tool(String name){
    mPath = mBasename = name;
  }

  public void run(String args, OutputReceiver receiver) throws IOException, InterruptedException{
    Shell.exec(mPath + " " + args, receiver);
  }

  public void run(String args) throws IOException, InterruptedException{
    run(args, null);
  }

  public Thread async(String args, OutputReceiver receiver){
    return Shell.async(mPath + " " + args, receiver);
  }

  public boolean kill(){
    return kill("9");
  }

  public boolean kill(String signal){
    try{
      Shell.exec("killall -" + signal + " " + mBasename);

      return true;
    }
    catch(Exception e){
      System.errorLogging(e);
    }

    return false;
  }
}
