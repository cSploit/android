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

/**
 * a container of tools.* instances
 */
public class ToolBox {
  public final Raw raw;
  public final Shell shell;
  public final Ruby ruby;
  public final NMap nmap;
  public final Hydra hydra;
  public final ArpSpoof arpSpoof;
  public final Ettercap ettercap;
  public final Fusemounts fusemounts;
  public final IPTables ipTables;
  public final TcpDump tcpDump;
  public final Msf msf;
  public final NetworkRadar networkRadar;
  public final MsfRpcd msfrpcd;
  public final Logcat logcat;

  public ToolBox() {
    raw = new Raw();
    shell = new Shell();
    ruby = new Ruby();
    nmap = new NMap();
    hydra = new Hydra();
    arpSpoof = new ArpSpoof();
    ettercap = new Ettercap();
    fusemounts = new Fusemounts();
    ipTables = new IPTables();
    tcpDump = new TcpDump();
    msf = new Msf();
    networkRadar = new NetworkRadar();
    msfrpcd = new MsfRpcd();
    logcat = new Logcat();
  }

  public void reload() {
    raw.setEnabled();
    shell.setEnabled();
    nmap.setEnabled();
    hydra.setEnabled();
    arpSpoof.setEnabled();
    ettercap.setEnabled();
    fusemounts.setEnabled();
    ipTables.setEnabled();
    tcpDump.setEnabled();
    networkRadar.setEnabled();
    logcat.setEnabled();

    ruby.init();
    msf.init();
    msfrpcd.init();
  }
}
