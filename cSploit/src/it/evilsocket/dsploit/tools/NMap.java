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

import android.text.TextUtils;

import java.util.LinkedList;

import it.evilsocket.dsploit.core.Child;
import it.evilsocket.dsploit.core.ChildManager;
import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.events.Event;
import it.evilsocket.dsploit.events.Hop;
import it.evilsocket.dsploit.events.Os;
import it.evilsocket.dsploit.events.Port;
import it.evilsocket.dsploit.net.Network;
import it.evilsocket.dsploit.net.Target;

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
        onHop(hop.hop, hop.usec, hop.node.getHostAddress());
      } else {
        Logger.error("unknown event: " + e);
      }
    }

    public abstract void onHop( int hop, long usec, String address );
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

  public Child trace( Target target, TraceReceiver receiver ) throws ChildManager.ChildNotStartedException {
    return super.async("-sn --traceroute --privileged --send-ip --system-dns " + target.getCommandLineRepresentation(), receiver );
  }

  public Child synScan( Target target, SynScanReceiver receiver, String custom ) throws ChildManager.ChildNotStartedException {
    String command = "-sS -P0 --privileged --send-ip --system-dns -vvv ";

    if( custom != null )
      command += "-p " + custom + " ";

    command += target.getCommandLineRepresentation();

    Logger.debug( "synScan - " + command );

    return super.async( command, receiver );
  }

  public Child inpsect( Target target, InspectionReceiver receiver, boolean focusedScan ) throws ChildManager.ChildNotStartedException {
    String cmd;
    LinkedList<Integer> tcp,udp;

    if(focusedScan)
    {
      tcp = new LinkedList<Integer>();
      udp = new LinkedList<Integer>();
      for( Target.Port p : target.getOpenPorts()) {
        if(p.protocol.equals(Network.Protocol.TCP)) {
          if(!tcp.contains(p.number))
            tcp.add(p.number);
        } else if(p.protocol.equals(Network.Protocol.UDP)) {
          if(!udp.contains(p.number))
            udp.add(p.number);
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