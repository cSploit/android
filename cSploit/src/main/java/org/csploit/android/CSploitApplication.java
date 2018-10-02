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
import android.content.Context;
import android.content.SharedPreferences;

import org.acra.ACRA;
import org.acra.annotation.AcraCore;
import org.acra.annotation.AcraHttpSender;
import org.acra.annotation.AcraNotification;
import org.acra.config.CoreConfigurationBuilder;
import org.acra.config.HttpSenderConfigurationBuilder;
import org.acra.config.NotificationConfigurationBuilder;
import org.acra.data.StringFormat;
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

import androidx.multidex.MultiDex;

@AcraHttpSender(
        httpMethod = HttpSender.Method.PUT,
        uri = "http://csploit.iriscouch.com/acra-csploit/_design/acra-storage/_update/report",
        basicAuthLogin = "android",
        basicAuthPassword = "DEADBEEF"
)
@AcraNotification(
        resChannelName = R.string.csploitChannelId,
        resText = R.string.crash_dialog_text,
        resIcon = R.drawable.dsploit_icon,
        resTitle = R.string.crash_dialog_title,
        resCommentPrompt = R.string.crash_dialog_comment
)

@AcraCore(applicationLogFile = "/cSploitd.log")

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

        CoreConfigurationBuilder builder = new CoreConfigurationBuilder(this);
        builder.setBuildConfigClass(BuildConfig.class).setReportFormat(StringFormat.JSON);
        builder.getPluginConfigurationBuilder(HttpSenderConfigurationBuilder.class);
        builder.getPluginConfigurationBuilder(NotificationConfigurationBuilder.class);
        ACRA.init(this, builder);

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

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        MultiDex.install(this);
        ACRA.init(this);
    }
}
