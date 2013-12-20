package it.evilsocket.dsploit.net.metasploit;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.Map;
import java.util.Stack;
import java.util.UUID;
import java.util.concurrent.TimeoutException;

import it.evilsocket.dsploit.R;
import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.core.Shell;
import it.evilsocket.dsploit.core.System;

/**
 * Extends Session with some method to read and write to shell
 * part of this code is taken from armitage
 */
public class ShellSession extends Session {

  // max time to wait for command execution
  private final static int TIMEOUT = 60000;

  protected Shell.OutputReceiver mReceiver;
  protected final Stack<String> mCommands  = new Stack<String>();
  protected boolean mSleep = true;
  protected int exitCode = -1;

  public ShellSession(Integer id, Map<String,Object> map) throws UnknownHostException {
    super(id,map);
    mReceiver = null;
    start();
  }

  @SuppressWarnings("unchecked")
  protected void processCommand(String command) throws TimeoutException, IOException, RPCClient.MSFException, InterruptedException {
    String token = UUID.randomUUID().toString();
    StringBuilder buffer = new StringBuilder();
    Boolean tokenFound = false;
    long start;
    int newline;
    RPCClient client = System.getMsfRpc();

    if(client==null) {
      mReceiver.onEnd(126); // @see http://stackoverflow.com/questions/1101957/are-there-any-standard-exit-status-codes-in-linux
      return;
    }

    // send command
    client.call("session.shell_write", mJobId,command+"\necho\necho \"$?"+token+"\"\n");

    // read until token is found
    start = java.lang.System.currentTimeMillis();
    do
    {
      String data = (((Map<String, String>)client.call("session.shell_read",mJobId )).get("data") + "");

      newline = data.indexOf('\n');
      while(newline>=0) {
        buffer.append(data.substring(0,newline));
        if((tokenFound = (buffer.toString().endsWith(token)))) {
          newline = Integer.parseInt(buffer.toString().substring(0,buffer.length() - token.length()));
          break;
        }
        mReceiver.onNewLine(buffer.toString());
        buffer.delete(0,buffer.length());
        // substring(start) => start is inclusive , we want remove the '\n'
        data=data.substring(newline+1);
        newline = data.indexOf('\n');
      }
      if(tokenFound) {
        mReceiver.onEnd(newline);
        return;
      }
      buffer.append(data);

      Thread.sleep(100);
    }while ((java.lang.System.currentTimeMillis() - start) < TIMEOUT);

    // no token found, TIMEOUT!
    throw new TimeoutException("token not found");
  }

  public void addCommand(String command) {
    synchronized (this) {
      if (mReceiver==null || command.trim().equals(""))
        return;
      mCommands.push(command);
    }
  }

  public int runCommand(String cmd) throws InterruptedException, TimeoutException, RPCClient.MSFException, IOException {
    synchronized (this) {
      Shell.OutputReceiver oldReceiver = mReceiver;
      exitCode = -1;

      mReceiver = new Shell.OutputReceiver() {
        @Override
        public void onStart(String command) {

        }

        @Override
        public void onNewLine(String line) {

        }

        @Override
        public void onEnd(int ret) {
          exitCode = ret;
        }
      };
      processCommand(cmd);
      mReceiver = oldReceiver;
      return exitCode;
    }
  }

  protected String grabCommand() {
    synchronized (this) {
      if(mCommands.size()>0)
        return mCommands.pop();
      else
        return null;
    }
  }

  public void setReceiver(Shell.OutputReceiver receiver) {
    synchronized (this) {
      mReceiver = receiver;
    }
  }

  /* keep grabbing commands, acquiring locks, until everything is executed */
  public void run() {
    try {
      //noinspection InfiniteLoopStatement
      while(true) {
        while(mSleep)
          Thread.sleep(200);
        String next = grabCommand();
        if (next != null) {
          processCommand(next);
          Thread.sleep(50);
        }
        else Thread.sleep(200);
      }
    } catch (InterruptedException e) {
      Logger.warning("interrupted");
    } catch (TimeoutException e) {
      Logger.error("Session timed out: " + e.getMessage());
    } catch (RPCClient.MSFException e) {
      System.errorLogging(e);
    } catch (IOException e) {
      System.errorLogging(e);
    } finally {
      stopSession();
    }
  }

  @Override
  public int getResourceId() {
    //TODO
    return R.drawable.exploit_msf;
  }

  public void sleep() {
    mSleep=true;
  }

  public void wakeUp() {
    mSleep=false;
  }

  @Override
  public boolean haveShell() {
    return true;
  }

}
