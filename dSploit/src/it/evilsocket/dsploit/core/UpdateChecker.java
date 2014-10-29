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
package it.evilsocket.dsploit.core;

import android.content.Context;
import android.content.Intent;

public class UpdateChecker extends Thread
{
  public static final String UPDATE_CHECKING = "UpdateChecker.action.CHECKING";
  public static final String UPDATE_AVAILABLE = "UpdateChecker.action.UPDATE_AVAILABLE";
  public static final String RUBY_AVAILABLE = "UpdateChecker.action.RUBY_AVAILABLE";
  public static final String GEMS_AVAILABLE = "UpdateChecker.action.GEMS_AVAILABLE";
  public static final String MSF_AVAILABLE = "UpdateChecker.action.MSF_AVAILABLE";
  public static final String UPDATE_NOT_AVAILABLE = "UpdateChecker.action.UPDATE_NOT_AVAILABLE";
  public static final String AVAILABLE_VERSION = "UpdateChecker.data.AVAILABLE_VERSION";

  private Context mContext = null;

  public UpdateChecker(Context context){
    super("UpdateChecker");

    mContext = context;
  }

  private void send(String msg, String extra, String value){
    Intent intent = new Intent(msg);

    if(extra != null && value != null)
      intent.putExtra(extra, value);

    mContext.sendBroadcast(intent);
  }

  public void run(){

    send(UPDATE_CHECKING, null, null);

    Logger.debug("Service started.");

    boolean checkMsfUpdates = System.getSettings().getBoolean("MSF_ENABLED", true) &&
            System.getSettings().getBoolean("MSF_CHECK_UPDATES", true);

    if(UpdateService.isUpdateAvailable())
      send(UPDATE_AVAILABLE, AVAILABLE_VERSION, UpdateService.getRemoteVersion());
    else if(checkMsfUpdates && UpdateService.isRubyUpdateAvailable())
      send(RUBY_AVAILABLE, null, null);
    else if(checkMsfUpdates && UpdateService.isMsfUpdateAvailable()) {
      send(MSF_AVAILABLE, null, null);
    } else if(checkMsfUpdates && UpdateService.isGemUpdateAvailable()){
      send(GEMS_AVAILABLE, null, null);
    } else
      send(UPDATE_NOT_AVAILABLE, null, null);

    Logger.debug("Service stopped.");
  }
}
