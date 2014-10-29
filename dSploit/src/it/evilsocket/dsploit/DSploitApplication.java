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
package it.evilsocket.dsploit;

import android.app.Application;
import android.content.SharedPreferences;

import java.net.NoRouteToHostException;

import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.plugins.ExploitFinder;
import it.evilsocket.dsploit.plugins.Inspector;
import it.evilsocket.dsploit.plugins.LoginCracker;
import it.evilsocket.dsploit.plugins.PacketForger;
import it.evilsocket.dsploit.plugins.PortScanner;
import it.evilsocket.dsploit.plugins.RouterPwn;
import it.evilsocket.dsploit.plugins.Traceroute;
import it.evilsocket.dsploit.plugins.mitm.MITM;
import it.evilsocket.dsploit.plugins.Sessions;
import it.evilsocket.dsploit.plugins.VulnerabilityFinder;

public class DSploitApplication extends Application{
  @Override
  public void onCreate(){
	  SharedPreferences themePrefs = getSharedPreferences("THEME", 0);
		Boolean isDark = themePrefs.getBoolean("isDark", false);
		if (isDark)
			setTheme(R.style.Sherlock___Theme);
		else
			setTheme(R.style.AppTheme);
		
    super.onCreate();

    // initialize the system
    try{
      System.init(this);
    } catch(Exception e){
      // ignore exception when the user has wifi off
      if(!(e instanceof NoRouteToHostException))
        System.errorLogging(e);
    }

    // load system modules even if the initialization failed
    System.registerPlugin(new RouterPwn());
    System.registerPlugin(new Traceroute());
    System.registerPlugin(new PortScanner());
    System.registerPlugin(new Inspector());
    System.registerPlugin(new VulnerabilityFinder());
    System.registerPlugin(new ExploitFinder());
    System.registerPlugin(new LoginCracker());
    System.registerPlugin(new Sessions());
    System.registerPlugin(new MITM());
    System.registerPlugin(new PacketForger());

  }
}
