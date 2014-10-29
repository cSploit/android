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

/**
 * a container of tools.* instances
 */
public class ToolBox {
  public final Raw raw;
  public final Shell shell;
  public final RubyShell rubyShell;
  public final NMap nmap;
  public final Hydra hydra;
  public final ArpSpoof arpSpoof;
  public final Ettercap ettercap;
  public final Fusemounts fusemounts;
  public final IPTables ipTables;
  public final TcpDump tcpDump;
  public final MsfShell msfShell;
  public final MsfRpcd msfRpcd;

  public ToolBox() {
    raw = new Raw();
    shell = new Shell();
    rubyShell = new RubyShell();
    nmap = new NMap();
    hydra = new Hydra();
    arpSpoof = new ArpSpoof();
    ettercap = new Ettercap();
    fusemounts = new Fusemounts();
    ipTables = new IPTables();
    tcpDump = new TcpDump();
    msfShell = new MsfShell();
    msfRpcd = new MsfRpcd();
  }

  public void reload() {
    raw.setEnabled();
    shell.setEnabled();
    rubyShell.setEnabled();
    nmap.setEnabled();
    hydra.setEnabled();
    arpSpoof.setEnabled();
    ettercap.setEnabled();
    fusemounts.setEnabled();
    ipTables.setEnabled();
    tcpDump.setEnabled();
    msfShell.setEnabled();
    msfRpcd.setEnabled();
  }
}
