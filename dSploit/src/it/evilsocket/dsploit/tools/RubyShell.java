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

import it.evilsocket.dsploit.core.ExecChecker;
import it.evilsocket.dsploit.core.SettingReceiver;
import it.evilsocket.dsploit.core.System;

/**
 * a shell with a modified environment to execute ruby binaries
 */
public class RubyShell extends Shell {

  private final static String rubyLib = "%1$s/site_ruby/1.9.1:%1$s/site_ruby/1.9.1/arm-linux-androideabi:%1$s/site_ruby:%1$s/vendor_ruby/1.9.1:%1$s/vendor_ruby/1.9.1/arm-linux-androideabi:%1$s/vendor_ruby:%1$s/1.9.1:%1$s/1.9.1/arm-linux-androideabi";


  public RubyShell() {
    super();

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

      if(mEnabled)
        setupEnvironment();
    }
  };


  protected void setupEnvironment() {
    String rubyRoot;
    StringBuilder sb = new StringBuilder();

    rubyRoot = ExecChecker.ruby().getRoot();

    sb.append(String.format("export RUBYLIB=\"" + rubyLib + "\"\n", rubyRoot + "/lib/ruby"));
    sb.append(String.format("export PATH=\"$PATH:%s\"\n", rubyRoot + "/bin"));
    sb.append(String.format("export HOME=\"%s\"\n", rubyRoot + "/home/ruby"));

    mPreCmd = sb.toString();
  }
}