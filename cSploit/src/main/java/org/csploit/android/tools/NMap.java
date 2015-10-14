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

import android.text.TextUtils;

import org.csploit.android.core.Child;
import org.csploit.android.core.ChildManager;
import org.csploit.android.core.Logger;
import org.csploit.android.events.Event;
import org.csploit.android.events.Hop;
import org.csploit.android.events.Os;
import org.csploit.android.events.Port;
import org.csploit.android.net.Network;
import org.csploit.android.net.Target;

import java.util.LinkedList;

public class NMap extends Tool {

  public static abstract class TraceReceiver extends Child.EventReceiver
  {
    public void onEnd( int exitCode ) {
      if( exitCode != 0 )
        Logger.error("nmap exited with code " + exitCode );
    }

    public void onDeath( int signal ) {
      Logger.error("nmap killed by signal " + signal);
    }

    public void onEvent(Event e) {
      if(e instanceof Hop) {
        Hop hop = (Hop)e;
        onHop(hop.hop, hop.usec, hop.node.getHostAddress(), hop.name);
      } else {
        Logger.error("unknown event: " + e);
      }
    }

    public abstract void onHop( int hop, long usec, String address, String name );
  }

  public static abstract class SynScanReceiver extends Child.EventReceiver
  {

    public void onEnd( int exitCode ) {
      if( exitCode != 0 )
        Logger.error( "nmap exited with code " + exitCode );
    }

    public void onDeath(int signal) {
      Logger.error("nmap killed by signal " + signal);
    }

    public void onEvent( Event e) {
      if(e instanceof Port) {
        Port p = (Port)e;
        onPortFound(p.port, p.protocol);
      } else {
        Logger.error("unkown event: " + e);
      }
    }

    public abstract void onPortFound( int port, String protocol );
  }

  public static abstract class InspectionReceiver extends Child.EventReceiver
  {

    public void onEnd( int exitCode ) {
      if( exitCode != 0 )
        Logger.error( "nmap exited with code " + exitCode );
    }

    public void onDeath(int signal) {
      Logger.error( "nmap killed by signal " + signal);
    }

    public void onEvent(Event e) {
      if(e instanceof Port) {
        Port p = (Port)e;
        if(p.service == null) {
          onOpenPortFound(p.port, p.protocol);
        } else {
          onServiceFound(p.port, p.protocol, p.service, p.version);
        }
      } else if(e instanceof Os) {
        Os os = (Os) e;
        onOsFound(os.os);
        onDeviceFound(os.type);
      } else {
        Logger.error("unknown event: " + e);
      }
    }

    public abstract void onOpenPortFound( int port, String protocol );
    public abstract void onServiceFound( int port, String protocol, String service, String version );
    public abstract void onOsFound( String os );
    public abstract void onDeviceFound( String device );
  }

  public NMap() {
    mHandler = "nmap";
    mCmdPrefix = null;
  }

  public Child trace( Target target, boolean resolve, TraceReceiver receiver ) throws ChildManager.ChildNotStartedException {

    String cmd = String.format("-sn --traceroute --privileged --send-ip --system-dns -%c %s",
            (resolve ? 'R' : 'n'), target.getCommandLineRepresentation());

    return super.async(cmd, receiver );
  }

  public Child synScan( Target target, SynScanReceiver receiver, String custom ) throws ChildManager.ChildNotStartedException {
    String command = "-sS -P0 --privileged --send-ip --system-dns -vvv ";

    if( custom != null )
      command += "-p " + custom + " ";

    command += target.getCommandLineRepresentation();

    Logger.debug( "synScan - " + command );

    return super.async( command, receiver );
  }

  public Child synScan( Target target, SynScanReceiver receiver) throws ChildManager.ChildNotStartedException {
    return synScan(target, receiver, null);
  }

  public Child customScan( Target target, SynScanReceiver receiver, String custom ) throws ChildManager.ChildNotStartedException {
    String command = "-vvv ";

    if( custom != null )
      command += custom + " ";

    command += target.getCommandLineRepresentation();

    Logger.debug( "customScan - " + command );

    return super.async( command, receiver );
  }

  public Child inpsect( Target target, InspectionReceiver receiver, boolean focusedScan ) throws ChildManager.ChildNotStartedException {
    String cmd;
    LinkedList<Integer> tcp,udp;
    Network.Protocol protocol;
    int pNumber;

    if(focusedScan)
    {
      tcp = new LinkedList<Integer>();
      udp = new LinkedList<Integer>();
      for( Target.Port p : target.getOpenPorts()) {
        protocol = p.getProtocol();
        pNumber = p.getNumber();

        if(protocol.equals(Network.Protocol.TCP)) {
          if(!tcp.contains(pNumber))
            tcp.add(pNumber);
        } else if(protocol.equals(Network.Protocol.UDP)) {
          if(!udp.contains(pNumber))
            udp.add(pNumber);
        }
      }
      cmd = "-T4 -sV -O --privileged --send-ip --system-dns -Pn -oX - ";
      if(tcp.size() + udp.size() > 0) {
        cmd+= "-p ";
        if(tcp.size()>0)
          cmd+= "T:" + TextUtils.join(",",tcp);
        if(udp.size()>0)
          cmd+= "U:" + TextUtils.join(",", udp);
        cmd+= " ";
      }
      cmd+= target.getCommandLineRepresentation();
    }
    else
      cmd = "-T4 -F -O -sV --privileged --send-ip --system-dns -oX - " + target.getCommandLineRepresentation();

    Logger.debug( "Inspect - " + cmd );

    return super.async( cmd, receiver);
  }
}