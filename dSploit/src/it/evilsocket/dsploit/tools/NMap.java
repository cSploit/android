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
package it.evilsocket.dsploit.tools;

import java.util.LinkedList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Target.Port;
import android.content.Context;
//import android.net.wifi.WifiConfiguration.Protocol;
import android.text.TextUtils;
import android.util.Log;
import it.evilsocket.dsploit.net.Network.Protocol;


public class NMap extends Tool
{
  private static final String TAG = "NMAP";

  public NMap( Context context ){
    super( "nmap/nmap", context );
  }

  public static abstract class TraceOutputReceiver implements OutputReceiver
  {
    private static final Pattern HOP_PATTERN = Pattern.compile( "^(\\d+)\\s+(\\.\\.\\.|[0-9\\.]+\\s[ms]+)\\s+([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}|[\\d]+).*", Pattern.CASE_INSENSITIVE );

    public void onStart( String commandLine ) { }

    public void onNewLine( String line ) {
      Matcher matcher;

      if( ( matcher = HOP_PATTERN.matcher( line ) ) != null && matcher.find() )
      {
        onHop( matcher.group( 1 ), matcher.group( 2 ), matcher.group( 3 ) );
      }
    }

    public void onEnd( int exitCode ) {
      if( exitCode != 0 )
        Log.e( TAG, "nmap exited with code " + exitCode );
    }

    public abstract void onHop( String hop, String time, String address );
  }

  public Thread trace( Target target, TraceOutputReceiver receiver ) {
    return super.async( "-sn --traceroute --privileged --send-ip --system-dns " + target.getCommandLineRepresentation(), receiver );
  }

  public static abstract class SynScanOutputReceiver implements OutputReceiver
  {
    private static final Pattern PORT_PATTERN = Pattern.compile( "^discovered open port (\\d+)/([^\\s]+).+", Pattern.CASE_INSENSITIVE );

    public void onStart( String commandLine ) {

    }

    public void onNewLine( String line ) {
      Matcher matcher;

      if( ( matcher = PORT_PATTERN.matcher( line ) ) != null && matcher.find() )
      {
        onPortFound( matcher.group( 1 ), matcher.group( 2 ) );
      }
    }

    public void onEnd( int exitCode ) {
      if( exitCode != 0 )
        Logger.error( "nmap exited with code " + exitCode );
    }

    public abstract void onPortFound( String port, String protocol );
  }

  public Thread synScan( Target target, SynScanOutputReceiver receiver, String custom ) {
    String command = "-sS -P0 --privileged --send-ip --system-dns -vvv ";

    if( custom != null )
      command += "-p " + custom + " ";

    command += target.getCommandLineRepresentation();

    Logger.debug( "synScan - " + command );

    return super.async( command, receiver );
  }

  public static abstract class InspectionReceiver implements OutputReceiver
  {
    private static final Pattern PORT_PATTERN 		  = Pattern.compile( "<port protocol=\"([^\"]+)\" portid=\"([^\"]+)\"><state state=\"open\"[^>]+><service(.+product=\"([^\"]+)\")?(.+version=\"([^\"]+)\")?", Pattern.CASE_INSENSITIVE);
    private static final Pattern OS_PATTERN 		  = Pattern.compile( "<osclass type=\"([^\"]+)\".+osfamily=\"([^\"]+)\".+accuracy=\"([^\"]+)\"", Pattern.CASE_INSENSITIVE);
    // remove "dev" "rc" and other extra version info
    private final static Pattern VERSION_PATTERN = Pattern.compile( "(([0-9]+\\.)+[0-9]+)[a-zA-Z]+");

    private static final int PROTO = 1,
      NUMBER = 2,
      NAME = 4,
      VERSION = 6,
      DEVICE_TYPE = 1,
      OSFAMILY = 2,
      ACCURACY = 3;
    private static int last_accuracy;

    public void onStart( String commandLine ) {
      last_accuracy=0;
    }

    public void onNewLine( String line ) {
      Matcher matcher = null;

      if((matcher = PORT_PATTERN.matcher(line)) != null && matcher.find())
      {
        int port_number = Integer.parseInt(matcher.group(NUMBER));
        String protocol = matcher.group(PROTO);
        String name     = matcher.group(NAME);
        String version  = matcher.group(VERSION);

        if(version == null || version.trim().isEmpty() || name == null || name.trim().isEmpty())
          onOpenPortFound(port_number, protocol);
        else
          for( String _version : version.split("-")) {
            matcher = VERSION_PATTERN.matcher(_version);
            if(matcher!=null && matcher.find())
              onServiceFound(port_number,protocol,name,matcher.group(1));
            else
              onServiceFound(port_number, protocol, name, _version);
          }

      }
      else if( ( matcher = OS_PATTERN.matcher(line) ) != null && matcher.find() )
      {
        int found_accuracy;
        if((found_accuracy = Integer.parseInt(matcher.group(ACCURACY))) > last_accuracy)
        {
          last_accuracy = found_accuracy;
          onOsFound(matcher.group(OSFAMILY));
          onDeviceFound(matcher.group(DEVICE_TYPE));
        }
      }
    }

    public abstract void onOpenPortFound( int port, String protocol );
    public abstract void onServiceFound( int port, String protocol, String service, String version );
    public abstract void onOsFound( String os );
    public abstract void onGuessOsFound( String os );
    public abstract void onDeviceFound( String device );
    public abstract void onServiceInfoFound( String info );

    public void onEnd( int exitCode ) {
      if( exitCode != 0 )
        Logger.error( "nmap exited with code " + exitCode );
    }
  }

  public Thread inpsect( Target target, InspectionReceiver receiver, boolean advanced_scan ) {
    String cmd;
    LinkedList<Integer> tcp,udp;

    if(advanced_scan)
    {
      tcp = new LinkedList<Integer>();
      udp = new LinkedList<Integer>();
      for( Port p : target.getOpenPorts()) {
        if(p.protocol.equals(Protocol.TCP)) {
          if(!tcp.contains(p.number))
            tcp.add(p.number);
        } else if(p.protocol.equals(Protocol.UDP)) {
          if(!udp.contains(p.number))
            udp.add(p.number);
        }
      }
      cmd = "-T4 -sV -O --privileged --send-ip --system-dns -Pn -oX - ";
      if(tcp.size() + udp.size() > 0) {
        cmd+= "-p ";
        if(tcp.size()>0)
          cmd+= "T:"+TextUtils.join(",",tcp);
        if(udp.size()>0)
          cmd+= "U:"+TextUtils.join(",",udp);
        cmd+= " ";
      }
      cmd+= target.getCommandLineRepresentation();
    }
    else
      cmd = "-T4 -F -O -sV --privileged --send-ip --system-dns -oX - " + target.getCommandLineRepresentation();

    Logger.debug( "Inspect - " + cmd );

    return super.async( cmd, receiver );
  }
}