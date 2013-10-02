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
package it.evilsocket.dsploit.net;

import java.io.IOException;
import java.net.MalformedURLException;

import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.System;
import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.net.msfrpc.MSFException;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import java.util.Date;

public class MsfRpcd extends Thread
{
  public static final String TAG				 = "MsfRpcd";

  public static final String 	STARTED			= "MsfRpcd.action.STARTED";
  public static final String 	RUNNING			= "MsfRpcd.action.RUNNING";
  public static final String 	STOPPED			= "MsfRpcd.action.STOPPED";
  public static final String 	STARTING		= "MsfRpcd.action.STARTING";
  public static final String 	FAILED			= "MsfRpcd.action.FAILED";
  public static final String 	ERROR			= "MsfRpcd.data.ERROR";
  private final static long 	TIMEOUT			= 540000; // 4 minutes
  private final static int 	DELAY 			= 5000; // poll every 5 secs

  private Context   			mContext	 		= null;
  private int 				mPort				= 55553;

  private boolean   			mRunning	 		= false;

  public MsfRpcd( Context context, int port ) {
    super("MsfRpcd");

    mContext   = context;
    mPort = port;
  }

  private void sendDaemonNotification(String action)
  {
    mContext.sendBroadcast(new Intent(action));
  }

  private void sendErrorMessageNotification(String message)
  {
    Intent intent = new Intent(FAILED);

    intent.putExtra(ERROR, message);
    mContext.sendBroadcast(intent);
  }

  public boolean isRunning() {
    return mRunning;
  }

  @Override
  public void run( ) {
    Logger.debug("MsfRpcd started");

    mRunning = true;

    try
    {
      if(Shell.exec("pidof msfrpcd") == 0)
      {
        try
        {
          System.setMsfRpc(new msfrpc("127.0.0.1",System.getSettings().getString("MSF_RPC_USER", "msf"),System.getSettings().getString("MSF_RPC_PSWD", "pswd"),mPort));
          sendDaemonNotification(RUNNING);
          mRunning = false;
          return;
        }
        catch ( MalformedURLException mue)
        {
          System.errorLogging(mue);
          sendErrorMessageNotification(mue.getMessage());
          mRunning = false;
          return;
        }
        catch ( IOException ioe)
        {
          System.errorLogging(ioe);
          Shell.exec("killall msfrpcd");
        }
        catch ( MSFException me)
        {
          System.errorLogging(me);
          sendErrorMessageNotification(me.getMessage());
          mRunning = false;
          return;
        }
      }
      sendDaemonNotification(STARTING);
      Shell.exec("chroot \"" + System.getSettings().getString("MSF_CHROOT_PATH", "/data/gentoo") + "\" /start_msfrpcd.sh -P \"" +
                   System.getSettings().getString("MSF_RPC_PSWD", "pswd") +
                   "\" -U \"" + System.getSettings().getString("MSF_RPC_USER", "msf") +
                   "\" -p " + mPort +
                   " -a 127.0.0.1 -n -S -t Msg\n");
    }
    catch ( IOException ioe)
    {
      Log.e(TAG, "Shell error", ioe);
      sendErrorMessageNotification(ioe.getMessage());
      mRunning = false;
      return;
    }
    catch ( InterruptedException ie)
    {
      mRunning = false;
      return;
    }

    long start = new Date().getTime();

    do
    {
      try
      {
        Thread.sleep(DELAY);
        System.setMsfRpc(new msfrpc("127.0.0.1",System.getSettings().getString("MSF_RPC_USER", "msf"),System.getSettings().getString("MSF_RPC_PSWD", "pswd"),mPort));
        sendDaemonNotification(STARTED);
        break;
      }
      catch ( InterruptedException iex)
      {
        break;
      }
      catch ( MalformedURLException mue)
      {
        System.errorLogging(mue);
        sendErrorMessageNotification(mue.getMessage());
        break;
      }
      catch ( IOException ioe)
      {
        // not running...
        if(((new Date().getTime()) - start) > TIMEOUT)
        {
          Logger.warning("timeout");
          sendErrorMessageNotification("timeout");
          break;
        }
      }
      catch ( MSFException me )
      {
        System.errorLogging(me);
        sendErrorMessageNotification(me.getMessage());
        break;
      }
    }while(mRunning);
    mRunning = false;
  }

  public void exit() {
    this.interrupt();
  }
}