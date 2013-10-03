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

public class RPCServer extends Thread
{
  public static final String 	TOAST 			= "RPCServer.action.TOAST";
  public static final String 	ERROR       = "RPCServer.action.ERROR";
  public static final String  STRINGID    = "RPCServer.data.STRINGID";
  private final static long 	TIMEOUT			= 540000; // 4 minutes
  private final static int    DELAY 			= 5000; // poll every 5 secs

  private Context   			mContext	 		= null;

  private boolean   			mRunning	 		= false;

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

  @Override
  public void run( ) {
    final String msfChrootPath,msfUser,msfPassword;
    final int msfPort;
    long start;

    Logger.debug("RPCServer started");

    mRunning = true;

    msfChrootPath = System.getSettings().getString("MSF_CHROOT_PATH", "/data/gentoo_msf");
    msfUser = System.getSettings().getString("MSF_RPC_USER", "msf");
    msfPassword = System.getSettings().getString("MSF_RPC_PSWD", "pswd");
    msfPort = System.getSettings().getInt("MSF_RPC_PORT", 55553);

    try
    {
      if(Shell.exec("pidof msfrpcd") == 0)
      {
        try
        {
          System.setMsfRpc(new RPCClient("127.0.0.1",msfUser,msfPassword,msfPort));
          sendDaemonNotification(TOAST, R.string.rpcd_running);
          mRunning = false;
          return;
        }
        catch ( MalformedURLException mue)
        {
          System.errorLogging(mue);
          sendDaemonNotification(ERROR,R.string.error_rpcd_inval);
          mRunning = false;
          return;
        }
        catch ( IOException ioe)
        {
          System.errorLogging(ioe);
          Shell.exec("killall msfrpcd");
        }
        catch ( RPCClient.MSFException me)
        {
          System.errorLogging(me);
          sendDaemonNotification(ERROR,R.string.error_rpcd_inval);
          mRunning = false;
          return;
        }
      }
      sendDaemonNotification(TOAST,R.string.rpcd_starting);

      Shell.exec( "chroot \"" + msfChrootPath + "\" /start_msfrpcd.sh -P \"" + msfPassword + "\" -U \"" + msfUser + "\" -p " + msfPort + " -a 127.0.0.1 -n -S -t Msg\n" );
    }
    catch ( IOException ioe )
    {
      System.errorLogging(ioe);
      sendDaemonNotification(ERROR,R.string.error_rpcd_shell);
      mRunning = false;
      return;
    }
    catch ( InterruptedException ie)
    {
      mRunning = false;
      return;
    }

    start = new Date().getTime();

    do
    {
      try
      {
        Thread.sleep(DELAY);
        System.setMsfRpc(new RPCClient("127.0.0.1",msfUser,msfPassword,msfPort));
        sendDaemonNotification(TOAST,R.string.rpcd_started);
        break;
      }
      catch ( InterruptedException iex)
      {
        break;
      }
      catch ( MalformedURLException mue)
      {
        System.errorLogging(mue);
        sendDaemonNotification(ERROR,R.string.error_rpcd_inval);
        break;
      }
      catch ( IOException ioe)
      {
        // not running...
        if(((new Date().getTime()) - start) > TIMEOUT)
        {
          Logger.warning("ETIMEDOUT");
          sendDaemonNotification(TOAST,R.string.rpcd_timedout);
          break;
        }
      }
      catch ( RPCClient.MSFException me )
      {
        System.errorLogging(me);
        sendDaemonNotification(ERROR,R.string.error_rpcd_inval);
        break;
      }
    }while(mRunning);
    mRunning = false;
  }

  public void exit() {
    this.interrupt();
  }
}