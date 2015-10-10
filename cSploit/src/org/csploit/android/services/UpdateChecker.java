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
package org.csploit.android.services;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import com.github.zafarkhaja.semver.Version;

import org.csploit.android.core.*;
import org.csploit.android.core.System;
import org.csploit.android.net.GitHubParser;
import org.csploit.android.services.UpdateService;
import org.csploit.android.update.ApkUpdate;
import org.csploit.android.update.CoreUpdate;
import org.csploit.android.update.MsfUpdate;
import org.csploit.android.update.RubyUpdate;
import org.csploit.android.update.Update;

import java.io.File;

public class UpdateChecker extends Thread
{
  public static final String UPDATE_CHECKING = "UpdateChecker.action.CHECKING";
  public static final String UPDATE_AVAILABLE = "UpdateChecker.action.UPDATE_AVAILABLE";
  public static final String UPDATE_NOT_AVAILABLE = "UpdateChecker.action.UPDATE_NOT_AVAILABLE";

  private Context mContext = null;

  public UpdateChecker(Context context){
    super("UpdateChecker");

    mContext = context;
  }

  private void send(String msg, Update update) {
    Intent intent = new Intent(msg);

    if(update != null)
      intent.putExtra(UpdateService.UPDATE, update);

    mContext.sendBroadcast(intent);
  }

  private void send(String msg) {
    send(msg, null);
  }

  /**
   * is version {@code a} newer than {@code b} ?
   */
  private boolean isNewerThan(String a, String b) {
    return Version.valueOf(a).compareTo(Version.valueOf(b)) > 0;
  }

  private Update getApkUpdate() {
    String localVersion = org.csploit.android.core.System.getAppVersionName();
    String remoteVersion, remoteURL;
    Update update;

    // cannot retrieve installed apk version
    if(localVersion==null)
      return null;

    try {
      remoteVersion = GitHubParser.getcSploitRepo().getLastReleaseVersion();

      if(remoteVersion == null)
        return null;

      remoteURL = GitHubParser.getcSploitRepo().getLastReleaseAssetUrl();

      Logger.debug(String.format("localVersion   = %s", localVersion));
      Logger.debug(String.format("remoteVersion  = %s", remoteVersion));

      update = new ApkUpdate(mContext, remoteURL, remoteVersion);

      if(isNewerThan(remoteVersion, localVersion ))
        return update;
    } catch(Exception e){
      System.errorLogging(e);
    }
    return null;
  }

  private Update getCoreUpdate() {
    String localVersion = System.getCoreVersion();
    String platform = System.getPlatform();
    String remoteVersion, remoteURL;
    Update update;

    try {

      remoteVersion = GitHubParser.getCoreRepo().getLastReleaseVersion();

      if(remoteVersion == null)
        return null;

      remoteURL = GitHubParser.getCoreRepo().getLastReleaseAssetUrl(platform + ".");

      if(remoteURL == null) {
        Logger.warning(String.format("unsupported platform ( %s )", platform));
        platform = System.getCompatiblePlatform();
        Logger.debug(String.format("trying with '%s'", platform));

        remoteURL = GitHubParser.getCoreRepo().getLastReleaseAssetUrl(platform + ".");
      }

      Logger.debug(String.format("localVersion   = %s", localVersion));
      Logger.debug(String.format("remoteVersion  = %s", remoteVersion));

      if(remoteURL == null) {
        Logger.warning(String.format("unsupported platform ( %s )", platform));
        return null;
      }

      update = new CoreUpdate(mContext, remoteURL, remoteVersion);

      if(localVersion == null)
        return update;

      if(isNewerThan(remoteVersion, localVersion ))
        return update;
    } catch(Exception e){
      System.errorLogging(e);
    }

    return null;
  }

  private Update getRubyUpdate() {
    String localVersion = System.getLocalRubyVersion();
    String platform = System.getPlatform();
    String remoteVersion, remoteURL;
    Update update;

    try {
      remoteVersion = GitHubParser.getRubyRepo().getLastReleaseVersion();

      if(remoteVersion == null)
        return null;

      remoteURL = GitHubParser.getRubyRepo().getLastReleaseAssetUrl(platform + ".");

      if(remoteURL == null) {
        Logger.warning(String.format("unsupported platform ( %s )", platform));

        platform = System.getCompatiblePlatform();
        Logger.debug(String.format("trying with '%s'", platform));

        remoteURL = GitHubParser.getRubyRepo().getLastReleaseAssetUrl(platform + ".");
      }

      Logger.debug(String.format("localVersion   = %s", localVersion));
      Logger.debug(String.format("remoteVersion  = %s", remoteVersion));

      if(remoteURL == null) {
        Logger.warning(String.format("unsupported platform ( %s )", platform));
        return null;
      }

      update = new RubyUpdate(mContext, remoteURL, remoteVersion);

      if(localVersion == null)
        return update;

      if(isNewerThan(remoteVersion, localVersion ))
        return update;
    } catch(Exception e){
      System.errorLogging(e);
    }
    return null;
  }

  private Update getMsfUpdate() {
    String localVersion = System.getLocalMsfVersion();
    GitHubParser msfRepo = GitHubParser.getMsfRepo();
    Update update;

    try {

      String remoteUrl = msfRepo.getLastReleaseAssetUrl();
      String remoteVerison = msfRepo.getLastReleaseVersion();

      if(remoteUrl == null || remoteVerison == null)
        return null;

      update = new MsfUpdate(mContext, remoteUrl, remoteVerison);

      File local = new File(update.path);

      if (local.exists() && local.isFile() && local.canRead()) {
        update.url = null;
        update.version = "LOCAL_UPDATE";
      }

      return update.version.equals(localVersion) ? null : update;
    } catch (Exception e) {
      System.errorLogging(e);
    }
    return null;
  }

  public void run(){

    send(UPDATE_CHECKING);

    Logger.debug("Service started.");

    SharedPreferences prefs = System.getSettings();

    boolean checkApp = prefs.getBoolean("PREF_UPDATES_APP", true);

    boolean checkCore = prefs.getBoolean("PREF_UPDATES_CORE", true);

    boolean canCheckMsf = System.isCoreInitialized() && prefs.getBoolean("MSF_ENABLED", true);

    boolean checkRuby = canCheckMsf && prefs.getBoolean("PREF_UPDATES_RUBY", true);

    boolean checkMsf = canCheckMsf && prefs.getBoolean("PREF_UPDATES_MSF", true) &&
            System.getLocalRubyVersion() != null;

    Update update = null;

    if(checkApp)
      update = getApkUpdate();

    if(update == null && checkCore)
      update = getCoreUpdate();

    if(update == null && checkRuby)
      update = getRubyUpdate();

    if(update == null && checkMsf)
      update = getMsfUpdate();

    if(update != null) {
      send(UPDATE_AVAILABLE, update);
    } else {
      send(UPDATE_NOT_AVAILABLE);
    }

    Logger.debug("Service stopped.");
  }
}
