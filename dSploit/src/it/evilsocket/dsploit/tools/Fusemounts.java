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

import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.core.Shell.OutputReceiver;

public class Fusemounts extends Tool
{

  public static abstract class fuseReceiver implements OutputReceiver {

    @Override
    public void onStart(String command) {

    }

    @Override
    public void onNewLine(String line) {
      int space;

      if(line.length()==0)
        return;

      space = line.indexOf(' ');

      if(space < 0) {
        Logger.warning(String.format("no space in line: '%s'", line));
        return;
      }

      String source = line.substring(0, space).trim();
      String mountpoint = line.substring(space +1).trim();

      if(!source.endsWith("/"))
        source+="/";
      if(!mountpoint.endsWith("/"))
        mountpoint+="/";

      onNewMountpoint(source, mountpoint);
    }

    public abstract void onNewMountpoint(String source, String mountpoint);

    @Override
    public void onEnd(int exitCode) {
      if(exitCode!=0)
        Logger.error("fusemounts exited with code: "+exitCode);
    }
  }

  public Fusemounts(Context context){
    super("fusemounts/fusemounts", context);
  }

  public Thread getSources(fuseReceiver receiver){
    return super.async("", receiver);
  }

  public boolean kill(){
    // arpspoof needs SIGINT ( ctrl+c ) to restore arp table.
    return super.kill("SIGINT");
  }
}