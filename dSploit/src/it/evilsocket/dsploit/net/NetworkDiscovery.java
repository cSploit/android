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
package it.evilsocket.dsploit.net;

import android.content.Context;
import android.content.Intent;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Logger;

public class NetworkDiscovery extends Thread
{
  public static final String NEW_ENDPOINT = "NetworkDiscovery.action.NEW_ENDPOINT";
  public static final String ENDPOINT_UPDATE = "NetworkDiscovery.action.ENDPOINT_UPDATE";
  public static final String ENDPOINT_ADDRESS = "NetworkDiscovery.data.ENDPOINT_ADDRESS";
  public static final String ENDPOINT_HARDWARE = "NetworkDiscovery.data.ENDPOINT_HARDWARE";
  public static final String ENDPOINT_NAME = "NetworkDiscovery.data.ENDPOINT_NAME";

  private static final String ARP_TABLE_FILE = "/proc/net/arp";
  private static final Pattern ARP_TABLE_PARSER = Pattern.compile("^([\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3}\\.[\\d]{1,3})\\s+([0-9-a-fx]+)\\s+([0-9-a-fx]+)\\s+([a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2}:[a-f0-9]{2})\\s+([^\\s]+)\\s+(.+)$", Pattern.CASE_INSENSITIVE);
  private static final short NETBIOS_UDP_PORT = 137;
  // NBT UDP PACKET: QUERY; REQUEST; UNICAST
  private static final byte[] NETBIOS_REQUEST =
  {
    (byte) 0x82, (byte) 0x28, (byte) 0x0, (byte) 0x0, (byte) 0x0,
    (byte) 0x1, (byte) 0x0, (byte) 0x0, (byte) 0x0, (byte) 0x0,
    (byte) 0x0, (byte) 0x0, (byte) 0x20, (byte) 0x43, (byte) 0x4B,
    (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41,
    (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41,
    (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41,
    (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41,
    (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41,
    (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41, (byte) 0x41,
    (byte) 0x0, (byte) 0x0, (byte) 0x21, (byte) 0x0, (byte) 0x1
  };

  private Context mContext = null;
  private UdpProber mProber = null;
  private ArpReader mArpReader = null;
  private boolean mRunning = false;

  private class ArpReader extends Thread
  {
    private static final int RESOLVER_THREAD_POOL_SIZE = 25;

    private ThreadPoolExecutor mExecutor = null;
    private boolean mStopped = true;
    private final HashMap<String, String> mNetBiosMap = new HashMap<String, String>();

    public ArpReader(){
      super("ArpReader");
      mExecutor = (ThreadPoolExecutor) Executors.newFixedThreadPool(RESOLVER_THREAD_POOL_SIZE);
    }

    public synchronized void addNetBiosName(String address, String name){
      synchronized(mNetBiosMap){
        mNetBiosMap.put(address, name);
      }
    }

    @Override
    public void run(){
      Logger.debug("ArpReader started ...");

      mNetBiosMap.clear();
      mStopped = false;
      String iface = "";

      try{
        iface = System.getNetwork().getInterface().getDisplayName();
      }
      catch(Exception e){
        System.errorLogging(e);
      }

      while(!mStopped){
        try{
          BufferedReader reader = new BufferedReader(new FileReader(ARP_TABLE_FILE));
          String line = null,
            name = null;
          Matcher matcher = null;
          Endpoint endpoint = null;
          Target target = null;
          Network network = System.getNetwork();

          while((line = reader.readLine()) != null){
            if((matcher = ARP_TABLE_PARSER.matcher(line)) != null && matcher.find()){
              String address = matcher.group(1),
                // hwtype  = matcher.group( 2 ),
                flags = matcher.group(3),
                hwaddr = matcher.group(4),
                // mask	   = matcher.group( 5 ),
                device = matcher.group(6);

              if(device.equals(iface) && !hwaddr.equals("00:00:00:00:00:00") && flags.contains("2")){
                endpoint = new Endpoint(address, hwaddr);
                target = new Target(endpoint);
                // rescanning the gateway could cause an issue when the gateway itself has multiple interfaces ( LAN, WAN ... )
                if(!endpoint.getAddress().equals(network.getGatewayAddress()) && !endpoint.getAddress().equals(network.getLocalAddress())){
                  synchronized(mNetBiosMap){
                    name = mNetBiosMap.get(address);
                  }

                  if(name == null){
                    try{
                      mExecutor.execute(new NBResolver(address));
                    } catch(RejectedExecutionException e){
                      // ignore since this is happening because the executor was shut down.
                    } catch(OutOfMemoryError m){
                      // wait until the thread queue gets freed
                      break;
                    }

                    if(!target.isRouter()){
                      // attempt DNS resolution
                      name = endpoint.getAddress().getHostName();

                      if(!name.equals(address)){
                        Logger.debug(address + " was DNS resolved to " + name);

                        synchronized(mNetBiosMap){
                          mNetBiosMap.put(address, name);
                        }
                      } else
                        name = null;
                    }
                  }

                  if(!System.hasTarget(target))
                    sendNewEndpointNotification(endpoint, name);

                  else if(name != null){
                    target = System.getTargetByAddress(address);
                    if(target != null && !target.hasAlias()){
                      target.setAlias(name);
                      sendEndpointUpdateNotification();
                    }
                  }
                }
              }
            }
          }

          reader.close();

          Thread.sleep(500);
        }
        catch(Exception e){
          System.errorLogging(e);
        }
      }
    }

    public synchronized void exit(){
      mStopped = true;
      try{
        mExecutor.shutdown();
        mExecutor.awaitTermination(30, TimeUnit.SECONDS);
        mExecutor.shutdownNow();
      } catch(Exception ignored){
      }
    }
  }

  private class NBResolver extends Thread
  {
    private static final int MAX_RETRIES = 3;

    private InetAddress mAddress = null;
    private DatagramSocket mSocket = null;

    public NBResolver(String address) throws SocketException, UnknownHostException{
      super("NBResolver");

      mAddress = InetAddress.getByName(address);
      mSocket = new DatagramSocket();

      mSocket.setSoTimeout(200);
    }

    @Override
    public void run(){
      byte[] buffer = new byte[128];
      DatagramPacket packet = new DatagramPacket(buffer, buffer.length, mAddress, NETBIOS_UDP_PORT),
        query = new DatagramPacket(NETBIOS_REQUEST, NETBIOS_REQUEST.length, mAddress, NETBIOS_UDP_PORT);
      String name, address = mAddress.getHostAddress();
      Target target;

      for(int i = 0; i < MAX_RETRIES; i++){
        try{
          mSocket.send(query);
          mSocket.receive(packet);

          byte[] data = packet.getData();

          if(data != null && data.length >= 74){
            String response = new String(data, "ASCII");

            // i know this is horrible, but i really need only the netbios name
            name = response.substring(57, 73).trim();

            Logger.debug(address + " was resolved to " + name);

            // update netbios cache
            mArpReader.addNetBiosName(address, name);

            // existing target
            target = System.getTargetByAddress(address);
            if(target != null){
              target.setAlias(name);
              sendEndpointUpdateNotification();
            }

            break;
          }
        }
        catch(SocketTimeoutException ste){
          // swallow timeout error
        }
        catch(IOException e){
          System.errorLogging(e);
        }
        finally{
          try{
            // send again a query
            mSocket.send(query);
          }
          catch(Exception e){
            // swallow error
          }
        }
      }

      mSocket.close();
    }
  }

  private class UdpProber extends Thread
  {
    private static final int PROBER_THREAD_POOL_SIZE = 25;

    private class SingleProber extends Thread{
      private InetAddress mAddress = null;

      public SingleProber(InetAddress address){
        mAddress = address;
      }

      @Override
      public void run(){
        try{
          DatagramSocket socket = new DatagramSocket();
          DatagramPacket packet = new DatagramPacket(NETBIOS_REQUEST, NETBIOS_REQUEST.length, mAddress, NETBIOS_UDP_PORT);

          socket.setSoTimeout(200);
          socket.send(packet);

          socket.close();
        }
        catch(Exception ignored){ }
      }
    }

    private ThreadPoolExecutor mExecutor = null;
    private boolean mStopped = true;
    private Network mNetwork = null;

    public UdpProber(){
      mExecutor = (ThreadPoolExecutor) Executors.newFixedThreadPool(PROBER_THREAD_POOL_SIZE);
    }

    @Override
    public void run(){
      Logger.debug("UdpProber started ...");

      mStopped = false;

      int i, nhosts = 0;
      IP4Address current = null;

      try{
        mNetwork = System.getNetwork();
        nhosts = mNetwork.getNumberOfAddresses();
      } catch(Exception e){
        System.errorLogging(e);
      }

      while(!mStopped && mNetwork != null && nhosts > 0){
        try{
          for(i = 1, current = IP4Address.next(mNetwork.getStartAddress()); current != null && i <= nhosts; current = IP4Address.next(current), i++){
            // rescanning the gateway could cause an issue when the gateway itself has multiple interfaces ( LAN, WAN ... )
            if(!current.equals(mNetwork.getGatewayAddress()) && !current.equals(mNetwork.getLocalAddress())){
              InetAddress address = current.toInetAddress();

              try{
                mExecutor.execute(new SingleProber(address));
              } catch(RejectedExecutionException e){
                // ignore since this is happening because the executor was shut down.
                break;
              } catch(OutOfMemoryError m){
                // wait until the thread queue gets freed
                break;
              }
            }
          }

          Thread.sleep(1000);
        }
        catch(Exception e){ }
      }
    }

    public synchronized void exit(){
      mStopped = true;
      try{
        mExecutor.shutdown();
        mExecutor.awaitTermination(30, TimeUnit.SECONDS);
        mExecutor.shutdownNow();
      }
      catch(Exception ignored){ }
    }
  }

  public NetworkDiscovery(Context context){
    super("NetworkDiscovery");

    mContext = context;
    mArpReader = new ArpReader();
    mProber = new UdpProber();
    mRunning = false;
  }

  private void sendNewEndpointNotification(Endpoint endpoint, String name){
    Intent intent = new Intent(NEW_ENDPOINT);

    intent.putExtra(ENDPOINT_ADDRESS, endpoint.toString());
    intent.putExtra(ENDPOINT_HARDWARE, endpoint.getHardwareAsString());
    intent.putExtra(ENDPOINT_NAME, name == null ? "" : name);

    mContext.sendBroadcast(intent);
  }

  private void sendEndpointUpdateNotification(){
    mContext.sendBroadcast(new Intent(ENDPOINT_UPDATE));
  }

  public boolean isRunning(){
    return mRunning;
  }

  @Override
  public void run(){
    Logger.debug("Network monitor started ...");

    mRunning = true;

    try{
      mProber.start();
      mArpReader.start();

      mProber.join();
      mArpReader.join();

      Logger.debug("Network monitor stopped.");

      mRunning = false;
    } catch(Exception e){
      System.errorLogging(e);
    }
  }

  public void exit(){
    try{
      mProber.exit();
      mArpReader.exit();
    } catch(Exception e){
      System.errorLogging(e);
    }
  }
}
