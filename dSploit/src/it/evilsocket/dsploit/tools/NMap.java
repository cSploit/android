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

import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.core.Shell.OutputReceiver;
import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.net.Target;
import it.evilsocket.dsploit.net.Target.Port;
import android.content.Context;
//import android.net.wifi.WifiConfiguration.Protocol;
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
    private static final Pattern PORT_PATTERN 		  = Pattern.compile( "<port protocol=\"([^\"]+)\" portid=\"([^\"]+)\"><state state=\"open\"[^>]+><service.+product=\"([^\"]+)\" version=\"([^\"]+)\"", Pattern.CASE_INSENSITIVE);
    private static final Pattern OS_PATTERN 		  = Pattern.compile( "<osclass type=\"([^\"]+)\".+osfamily=\"([^\"]+)\".+accuracy=\"([^\"]+)\"", Pattern.CASE_INSENSITIVE);

    private static final int 	proto = 1,
      number = 2,
      name = 3,
      version = 4,
      device_type = 1,
      osfamily = 2,
      accuracy = 3;
    private static int last_accuracy;

    public void onStart( String commandLine ) {
      last_accuracy=0;
    }

    public void onNewLine( String line ) {
      Logger.debug( "NMAP: " + line );

      Matcher matcher = null;

      if((matcher = PORT_PATTERN.matcher(line)) != null && matcher.find())
      {
        int port_number = Integer.parseInt(matcher.group(number));
        String protocol = matcher.group(proto);

        onOpenPortFound(port_number, protocol);
        for( String _version : matcher.group(version).split("-"))
        {
          onServiceFound(port_number, protocol, matcher.group(name), _version);
        }

      }
      else if( ( matcher = OS_PATTERN.matcher(line) ) != null && matcher.find() )
      {
        int found_accuracy;
        if((found_accuracy = Integer.parseInt(matcher.group(accuracy))) > last_accuracy)
        {
          last_accuracy = found_accuracy;
          onOsFound(matcher.group(osfamily));
          onDeviceFound(matcher.group(device_type));
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
    String tcp_ports,udp_ports, cmd;

    if(advanced_scan)
    {
      tcp_ports = "";
      udp_ports = "";
      for( Port p : target.getOpenPorts())
        if(p.protocol.equals(Protocol.TCP))
          tcp_ports += Integer.toString(p.number) + ",";
        else if(p.protocol.equals(Protocol.UDP))
          udp_ports += Integer.toString(p.number) + ",";
      if(udp_ports.length()>0)
        udp_ports = "U:" + udp_ports;
      if(tcp_ports.length()>0)
        tcp_ports = "T:" + tcp_ports;

      cmd = "-T4 -sV -O --privileged --send-ip --system-dns -Pn -oX - -p " + tcp_ports + udp_ports + " " + target.getCommandLineRepresentation();
    }
    else
      cmd = "-T4 -F -O -sV --privileged --send-ip --system-dns -oX - " + target.getCommandLineRepresentation();

    Logger.debug( "Inspect - " + cmd );

    return super.async( cmd, receiver );
  }
}