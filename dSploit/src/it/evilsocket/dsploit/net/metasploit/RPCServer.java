/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 * 			   Massimo Dragano	aka tux_mind <massimo.dragano@gmail.com>
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
package it.evilsocket.dsploit.net.metasploit;

import java.io.IOException;
import java.net.MalformedURLException;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Logger;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import java.util.concurrent.TimeoutException;

public class RPCServer extends Thread
{
  public static final String 	TOAST 			= "RPCServer.action.TOAST";
  public static final String 	ERROR       = "RPCServer.action.ERROR";
  public static final String  STRINGID    = "RPCServer.data.STRINGID";
  private final static long 	TIMEOUT			= 540000; // 4 minutes
  private final static int    DELAY 			= 5000; // poll every 5 secs

  private Context         mContext	 		  = null;
  private boolean         mRunning	 		  = false;
  private boolean         mRemote 	 		  = false;
  private boolean         msfSsl    	 		= false;
  private String          msfHost,
                          msfUser,
                          msfPassword,
                          msfDir;
  private int             msfPort;
  private long            mTimeout = 0;

  public RPCServer(Context context) {
    super("RPCServer");
    mContext   = context;
  }

  public static boolean exists() {
    return (new java.io.File(System.getMsfPath() + "/msfrpcd")).exists();
  }

  public static boolean isInternal() {
    return System.getNetwork().isInternal(System.getSettings().getString("MSF_RPC_HOST", "127.0.0.1"));
  }

  public static boolean isLocal() {
    return System.getSettings().getString("MSF_RPC_HOST", "127.0.0.1").equals("127.0.0.1");
  }

  private void sendDaemonNotification(String action, int message)
  {
    Intent i = new Intent(action);
    i.putExtra(STRINGID, message);
    mContext.sendBroadcast(i);
  }

  public boolean isRunning() {
    return mRunning;
  }

  private boolean connect_to_running_server() throws RuntimeException, IOException, InterruptedException, TimeoutException {
    boolean ret = false;

    if(mRemote || Shell.exec("pidof msfrpcd")==0)
    {
      try
      {
        System.setMsfRpc(new RPCClient(msfHost,msfUser,msfPassword,msfPort,msfSsl));
        ret = true;
      }
      catch ( MalformedURLException mue)
      {
        System.errorLogging(mue);
        throw new RuntimeException();
      }
      catch ( IOException ioe)
      {
        Logger.debug(ioe.getMessage());
        if(mRemote)
          throw new RuntimeException();
      }
      catch ( RPCClient.MSFException me)
      {
        System.errorLogging(me);
        throw new RuntimeException();
      }
      finally {
        if(!ret && !mRemote)
          Shell.exec("killall msfrpcd");
      }
    }
    if(mRemote && !ret)
      throw new TimeoutException("remote rcpd does not respond");
    return ret;
  }

  /* WARNING: this method will hang forever if msfrpcd start successfully,
   * use it only for report server crashes.
   * NOTE: it can be useful if we decide to own the msfrpcd process
  */
  private void start_daemon_fg() {
    Shell.OutputReceiver debug_receiver = new Shell.OutputReceiver() {
      @Override
      public void onStart(String command) {
        Logger.debug("running \""+command+"\"");
      }

      @Override
      public void onNewLine(String line) {
        Logger.debug(line);
      }

      @Override
      public void onEnd(int exitCode) {
        Logger.debug("exitValue="+exitCode);
      }
    };

    try
    {
      Shell.setupRubyEnviron();
      String command = String.format("%s/msfrpcd -P '%s' -U '%s' -p '%d' -a 127.0.0.1 -n %s -t Msg -f",
              msfDir,msfPassword,msfUser,msfPort,(msfSsl ? "" : "-S"));
      if(Shell.exec( command, debug_receiver) != 0) {
        Logger.error("msfrpcd failed");
      }
    }
    catch ( Exception e)
    {
      System.errorLogging(e);
    }
  }

  private void start_daemon() throws RuntimeException, IOException, InterruptedException, TimeoutException {
    Thread res;
    Shell.StreamGobbler msfrpcd;
    long time;
    boolean daemonJoined = false;

    Shell.setupRubyEnviron();

    if(!Shell.canExecuteInDir(msfDir) && !Shell.canRootExecuteInDir((msfDir = Shell.getRealPath(msfDir))))
      throw new IOException("cannot execute msf binaries");

    res = Shell.async(String.format(
            "%s/msfrpcd -P '%s' -U '%s' -p '%d' -a 127.0.0.1 -n %s -t Msg",
            msfDir,msfPassword,msfUser,msfPort,(msfSsl ? "" : "-S")
    ));
    if(!(res instanceof Shell.StreamGobbler))
      throw new IOException("cannot run shell commands");

    msfrpcd = (Shell.StreamGobbler) res;
    msfrpcd.setName("msfrpcd");
    msfrpcd.start();

    try {
      while ((time = java.lang.System.currentTimeMillis()) < mTimeout &&
              msfrpcd.getState() != State.TERMINATED)
        Thread.sleep(100);
      if (time >= mTimeout)
        throw new TimeoutException("msfrpcd timed out");
      msfrpcd.join();
      daemonJoined = true;
      if (msfrpcd.exitValue != 0) {
        Logger.error("msfrpcd returned " + msfrpcd.exitValue);
        throw new RuntimeException("msfrpcd failed");
      }
    } finally {
      if(!daemonJoined) {
        msfrpcd.interrupt();
      }
    }
  }

  private void wait_for_connection() throws RuntimeException, IOException, InterruptedException, TimeoutException {

    do {
      if(Shell.exec("pidof msfrpcd")!=0)
      {
        // OMG, server crashed!
        // start server in foreground and log errors.
        sendDaemonNotification(ERROR,R.string.error_rcpd_fatal);
        start_daemon_fg();
        throw new InterruptedException("exiting due to server crash");
      }
      try
      {
        Thread.sleep(DELAY);
        System.setMsfRpc(new RPCClient(msfHost,msfUser,msfPassword,msfPort,msfSsl));
        return;
      }
      catch ( MalformedURLException mue)
      {
        System.errorLogging(mue);
        throw new RuntimeException();
      }
      catch ( IOException ioe)
      {
        // cannot connect now...
      }
      catch ( RPCClient.MSFException me )
      {
        System.errorLogging(me);
        throw new RuntimeException();
      }
    } while(java.lang.System.currentTimeMillis() < mTimeout);
    Logger.debug("MSF RPC Server timed out");
    throw new TimeoutException();
  }

  @Override
  public void run( ) {
    mTimeout = java.lang.System.currentTimeMillis() + TIMEOUT;
    Logger.debug("RPCD Service started");

    mRunning = true;

    SharedPreferences prefs = System.getSettings();

    msfHost     = prefs.getString("MSF_RPC_HOST", "127.0.0.1");
    msfUser     = prefs.getString("MSF_RPC_USER", "msf");
    msfPassword = prefs.getString("MSF_RPC_PSWD", "msf");
    msfDir      = prefs.getString("MSF_DIR", System.getDefaultMsfPath());
    msfPort     = System.MSF_RPC_PORT;
    msfSsl      = prefs.getBoolean("MSF_RPC_SSL", false);

    mRemote     = !msfHost.equals("127.0.0.1");

    try
    {
      if(!connect_to_running_server()) {
        sendDaemonNotification(TOAST, R.string.rpcd_starting);
        start_daemon();
        wait_for_connection();
        sendDaemonNotification(TOAST, R.string.rpcd_started);
        Logger.debug("connected to new MSF RPC Server");
      } else {
        sendDaemonNotification(TOAST, R.string.connected_msf);
        Logger.debug("connected to running MSF RPC Server");
      }
    } catch ( IOException ioe ) {
      Logger.error(ioe.getMessage());
      sendDaemonNotification(ERROR, R.string.error_rpcd_shell);
    } catch ( InterruptedException e ) {
      if(e.getMessage()!=null)
        Logger.debug(e.getMessage());
      else
        System.errorLogging(e);
    } catch ( RuntimeException e ) {
      sendDaemonNotification(ERROR, R.string.error_rpcd_inval);
    } catch (TimeoutException e) {
      sendDaemonNotification(TOAST, R.string.rpcd_timedout);
    }

    Logger.debug("RPCD Service stopped");
    mRunning = false;
  }

  public void exit() {
    if(this.isAlive())
      this.interrupt();
  }
}