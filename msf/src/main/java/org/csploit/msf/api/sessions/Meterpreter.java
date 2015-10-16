package org.csploit.msf.api.sessions;

/**
 * This class represents a session compatible interface to a meterpreter server
 * instance running on a remote machine.  It provides the means of interacting
 * with the server instance both at an API level as well as at a console level.
 */
public interface Meterpreter extends SingleCommandShell, Scriptable {
  //TODO
}
