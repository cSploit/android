package org.csploit.msf.session;

import org.csploit.msf.Session;

/**
 * This class provides basic interaction with a command shell on the remote
 * endpoint.  This session is initialized with a stream that will be used
 * as the pipe for reading and writing the command shell.
 */
public abstract class CommandShell extends Session implements SingleCommandShell, Scriptable {
  public CommandShell(int id) {
    super(id);
  }

  @Override
  public String getType() {
    return "shell";
  }
}
