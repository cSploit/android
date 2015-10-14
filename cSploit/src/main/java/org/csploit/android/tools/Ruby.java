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

package org.csploit.android.tools;

import org.csploit.android.core.ExecChecker;
import org.csploit.android.core.SettingReceiver;
import org.csploit.android.core.System;

/**
 * run commands with custom environment variables to provide access to ruby binaries
 */
public class Ruby extends Tool {

  private final static String rubyLib = "%1$s/site_ruby/1.9.1:%1$s/site_ruby/1.9.1/arm-linux-androideabi:%1$s/site_ruby:%1$s/vendor_ruby/1.9.1:%1$s/vendor_ruby/1.9.1/arm-linux-androideabi:%1$s/vendor_ruby:%1$s/1.9.1:%1$s/1.9.1/arm-linux-androideabi";

  public Ruby() {
    mHandler = "raw";
  }

  public void init() {
    setEnabled();

    if(mEnabled)
      setupEnvironment();

    registerSettingReceiver();
  }

  @Override
  public void setEnabled() {
    super.setEnabled();
    mEnabled = mEnabled &&
            System.getSettings().getBoolean("MSF_ENABLED", true) &&
                    (ExecChecker.ruby().getRoot() != null ||
                            ExecChecker.ruby().canExecuteInDir(System.getRubyPath()));
  }

  protected void registerSettingReceiver() {
    onSettingsChanged.addFilter("MSF_ENABLED");
    onSettingsChanged.addFilter("RUBY_DIR");
    System.registerSettingListener(onSettingsChanged);
  }

  protected void unregisterSettingReceiver() {
    System.unregisterSettingListener(onSettingsChanged);
  }

  protected SettingReceiver onSettingsChanged = new SettingReceiver() {

    public void onSettingChanged(String key) {
      setEnabled();
    }
  };

  public void setupEnvironment() {
    String rubyRoot;
    String path;
    mEnv = new String[3];

    rubyRoot = ExecChecker.ruby().getRoot();
    path = java.lang.System.getenv("PATH");

    mEnv[0] = String.format("RUBYLIB=" + rubyLib, rubyRoot + "/lib/ruby");
    mEnv[1] = String.format("PATH=%s:%s", path, rubyRoot + "/bin");
    mEnv[2] = String.format("HOME=%s", rubyRoot + "/home/ruby");
  }
}