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
package org.csploit.android.core;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

public class UpdateChecker extends Thread
{
  public static final String UPDATE_CHECKING = "UpdateChecker.action.CHECKING";
  public static final String UPDATE_AVAILABLE = "UpdateChecker.action.UPDATE_AVAILABLE";
  public static final String CORE_AVAILABLE = "UpdateChecker.action.CORE_AVAILABLE";
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

  private void send(String msg) {
    send(msg, null, null);
  }

  public void run(){

    send(UPDATE_CHECKING);

    Logger.debug("Service started.");

    SharedPreferences prefs = System.getSettings();

    boolean checkApp = prefs.getBoolean("PREF_UPDATES_APP", true);

    boolean checkCore = prefs.getBoolean("PREF_UPDATES_CORE", true);

    boolean canCheckMsf = System.isCoreInitialized() && prefs.getBoolean("MSF_ENABLED", true);

    boolean checkRuby = canCheckMsf && prefs.getBoolean("PREF_UPDATES_RUBY", true);

    boolean checkGems = canCheckMsf && prefs.getBoolean("PREF_UPDATES_GEMS", true) &&
            System.getLocalRubyVersion() != null;

    boolean checkMsf = canCheckMsf && prefs.getBoolean("PREF_UPDATES_MSF", true) &&
            System.getLocalRubyVersion() != null;

    if(checkApp && UpdateService.isUpdateAvailable())
      send(UPDATE_AVAILABLE, AVAILABLE_VERSION, UpdateService.getRemoteVersion());
    else if(checkCore && UpdateService.isCoreUpdateAvailable())
      send(CORE_AVAILABLE, AVAILABLE_VERSION, UpdateService.getRemoteCoreVersion());
    else if(checkRuby && UpdateService.isRubyUpdateAvailable())
      send(RUBY_AVAILABLE);
    else if(checkMsf && UpdateService.isMsfUpdateAvailable()) {
      send(MSF_AVAILABLE);
    } else if(checkGems && UpdateService.isGemUpdateAvailable()){
      send(GEMS_AVAILABLE);
    } else
      send(UPDATE_NOT_AVAILABLE);

    Logger.debug("Service stopped.");
  }
}
