package org.csploit.msf.session;

import org.csploit.msf.Session;

/**
 * This class represents a session compatible interface to a meterpreter server
 * instance running on a remote machine.  It provides the means of interacting
 * with the server instance both at an API level as well as at a console level.
 */
public abstract class Meterpreter extends Session implements SingleCommandShell, Scriptable {
  public Meterpreter(int id) {
    super(id);
  }

  @Override
  public String getType() {
    return "meterpreter";
  }
}
