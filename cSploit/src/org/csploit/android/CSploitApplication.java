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
package org.csploit.android;

import android.app.Application;
import android.content.SharedPreferences;

import org.acra.ACRA;
import org.acra.ReportingInteractionMode;
import org.acra.annotation.ReportsCrashes;
import org.acra.sender.HttpSender;
import org.csploit.android.core.System;
import org.csploit.android.plugins.ExploitFinder;
import org.csploit.android.plugins.Inspector;
import org.csploit.android.plugins.LoginCracker;
import org.csploit.android.plugins.PacketForger;
import org.csploit.android.plugins.PortScanner;
import org.csploit.android.plugins.RouterPwn;
import org.csploit.android.plugins.Sessions;
import org.csploit.android.plugins.Traceroute;
import org.csploit.android.plugins.mitm.MITM;
import org.csploit.android.services.Services;

import java.net.NoRouteToHostException;

@ReportsCrashes(
  httpMethod = HttpSender.Method.PUT,
  reportType = HttpSender.Type.JSON,
  formUri = "http://csploit.iriscouch.com/acra-csploit/_design/acra-storage/_update/report",
  formUriBasicAuthLogin = "android",
  formUriBasicAuthPassword = "DEADBEEF",
  mode = ReportingInteractionMode.DIALOG,
  resDialogText = R.string.crash_dialog_text,
  resDialogIcon = R.drawable.dsploit_icon,
  resDialogTitle = R.string.crash_dialog_title,
  resDialogCommentPrompt = R.string.crash_dialog_comment
)
public class CSploitApplication extends Application {
  @Override
  public void onCreate() {
    SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
    Boolean isDark = themePrefs.getBoolean("isDark", false);
    if (isDark)
      setTheme(R.style.DarkTheme);
    else
      setTheme(R.style.AppTheme);

    super.onCreate();

    ACRA.init(this);
    Services.init(this);

    // initialize the system
    try {
      System.init(this);
    } catch (Exception e) {
      // ignore exception when the user has wifi off
      if (!(e instanceof NoRouteToHostException))
        System.errorLogging(e);
    }

    ACRA.setConfig(ACRA.getConfig().setApplicationLogFile(System.getCorePath() + "/cSploitd.log"));

    // load system modules even if the initialization failed
    System.registerPlugin(new RouterPwn());
    System.registerPlugin(new Traceroute());
    System.registerPlugin(new PortScanner());
    System.registerPlugin(new Inspector());
    System.registerPlugin(new ExploitFinder());
    System.registerPlugin(new LoginCracker());
    System.registerPlugin(new Sessions());
    System.registerPlugin(new MITM());
    System.registerPlugin(new PacketForger());
  }
}
