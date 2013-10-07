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

import java.util.Date;
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
  private String          msfUser,
                          msfPassword,
                          msfChrootPath;
  private int             msfPort;
  private long            mTimeout = 0;

  public RPCServer(Context context) {
    super("RPCServer");
    mContext   = context;
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

  private boolean connect_to_running_server() throws RuntimeException, IOException, InterruptedException {
    boolean ret = false;

    if(Shell.exec("pidof msfrpcd")==0)
    {
      try
      {
        System.setMsfRpc(new RPCClient("127.0.0.1",msfUser,msfPassword,msfPort));
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
      }
      catch ( RPCClient.MSFException me)
      {
        System.errorLogging(me);
        throw new RuntimeException();
      }
      finally {
        if(!ret)
          Shell.exec("killall msfrpcd");
      }
    }
    return ret;
  }

  /* WARNING: this method will hang forever if msfrpcd start successfully,
   * use it only for report server crashes.
   * NOTE: it can be useful if we decide to own the msfrpcd process
  */
  private void start_daemon_fg() {
    class debug_receiver implements Shell.OutputReceiver {
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
        Logger.debug("exit_code="+exitCode);
      }
    }

    try
    {
      if(Shell.exec( "chroot \"" + msfChrootPath + "\" /start_msfrpcd.sh -P \"" + msfPassword + "\" -U \"" + msfUser + "\" -p " + msfPort + " -a 127.0.0.1 -n -S -t Msg -f", new debug_receiver()) != 0) {
        Logger.error("chroot failed");
      }
    }
    catch ( Exception e)
    {
      System.errorLogging(e);
    }
  }

  private void start_daemon() throws RuntimeException, IOException, InterruptedException {
    if(Shell.exec( "chroot \"" + msfChrootPath + "\" /start_msfrpcd.sh -P \"" + msfPassword + "\" -U \"" + msfUser + "\" -p " + msfPort + " -a 127.0.0.1 -n -S -t Msg\n")!=0) {
      throw new RuntimeException("chroot failed");
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
        System.setMsfRpc(new RPCClient("127.0.0.1",msfUser,msfPassword,msfPort));
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
    } while(new Date().getTime() < mTimeout);
    throw new TimeoutException();
  }

  @Override
  public void run( ) {
    mTimeout = new Date().getTime() + TIMEOUT;
    Logger.debug("RPCServer started");

    mRunning = true;

    msfChrootPath = System.getSettings().getString("MSF_CHROOT_PATH", "/data/gentoo_msf");
    msfUser = System.getSettings().getString("MSF_RPC_USER", "msf");
    msfPassword = System.getSettings().getString("MSF_RPC_PSWD", "pswd");
    msfPort = System.getSettings().getInt("MSF_RPC_PORT", 55553);

    try
    {
      if(!connect_to_running_server()) {
        sendDaemonNotification(TOAST,R.string.rpcd_starting);
        start_daemon();
        wait_for_connection();
        sendDaemonNotification(TOAST,R.string.rpcd_started);
      }
      else
        sendDaemonNotification(TOAST, R.string.rpcd_running);
    } catch ( IOException ioe ) {
      Logger.error(ioe.getMessage());
      sendDaemonNotification(ERROR,R.string.error_rpcd_shell);
    } catch ( InterruptedException e ) {
      if(e.getMessage()!=null)
        Logger.debug(e.getMessage());
      else
        System.errorLogging(e);
    } catch ( RuntimeException e ) {
      sendDaemonNotification(ERROR,R.string.error_rpcd_inval);
    } catch (TimeoutException e) {
      sendDaemonNotification(TOAST,R.string.rpcd_timedout);
    }
    mRunning = false;
  }

  public void exit() {
    if(this.isAlive())
      this.interrupt();
  }
}