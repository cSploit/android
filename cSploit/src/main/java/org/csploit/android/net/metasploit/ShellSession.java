package org.csploit.android.net.metasploit;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.Map;
import java.util.Stack;
import java.util.UUID;
import java.util.concurrent.TimeoutException;

import org.csploit.android.R;
import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.tools.Raw;

/**
 * Extends Session with some method to read and write to shell
 * part of this code is taken from armitage
 */
public class ShellSession extends Session {

  // max time to wait for command execution
  private final static int TIMEOUT = 60000;

  public static abstract class RpcShellReceiver extends Raw.RawReceiver {

    public abstract void onRpcClosed();
    public abstract void onTimedOut();
  }

  private static class CmdAndReceiver {
    public final String command;
    public final RpcShellReceiver receiver;

    public CmdAndReceiver(String command, RpcShellReceiver receiver) {
      this.command = command;
      this.receiver = receiver;
    }
  }

  protected final Stack<CmdAndReceiver> mCommands  = new Stack<CmdAndReceiver>();

  public ShellSession(Integer id, Map<String,Object> map) throws UnknownHostException {
    super(id,map);
    start();
  }

  @SuppressWarnings("unchecked")
  protected void processCommand(CmdAndReceiver cmd) throws TimeoutException, IOException, RPCClient.MSFException, InterruptedException {
    String token = UUID.randomUUID().toString();
    StringBuilder buffer = new StringBuilder();
    Boolean tokenFound = false;
    int newline;
    long timeout;
    RPCClient client = System.getMsfRpc();

    if(client==null) {
      if(cmd.receiver != null)
        cmd.receiver.onRpcClosed();
      throw new IOException("RPC Client unavailable");
    }

    // send command
    client.call("session.shell_write", mJobId,cmd.command+"\necho\necho \"$?"+token+"\"\n");

    // read until token is found
    timeout = java.lang.System.currentTimeMillis() + TIMEOUT;
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
        if(cmd.receiver!=null)
          cmd.receiver.onNewLine(buffer.toString());
        buffer.delete(0,buffer.length());
        // substring(start) => start is inclusive , we want remove the '\n'
        data=data.substring(newline+1);
        newline = data.indexOf('\n');
      }
      if(tokenFound) {
        if(cmd.receiver!=null)
          cmd.receiver.onEnd(newline);
        return;
      }
      buffer.append(data);

      Thread.sleep(100);
    }while (java.lang.System.currentTimeMillis() < timeout);

    // no token found, TIMEOUT!
    if(cmd.receiver!=null)
      cmd.receiver.onTimedOut();
    throw new TimeoutException("token not found");
  }

  public void addCommand(String command, RpcShellReceiver receiver) {
    synchronized (mCommands) {
      if (command.trim().equals(""))
        return;
      mCommands.push(new CmdAndReceiver(command, receiver));
      mCommands.notify();
    }
  }

  protected CmdAndReceiver grabCommand() throws InterruptedException {
    synchronized (mCommands) {
      while(mCommands.size() == 0) {
        mCommands.wait();
      }
      return mCommands.pop();
    }
  }

  /* keep grabbing commands, acquiring locks, until everything is executed */
  public void run() {
    try {
      //noinspection InfiniteLoopStatement
      while(true) {
        processCommand(grabCommand());
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

  @Override
  public boolean haveShell() {
    return true;
  }

}
