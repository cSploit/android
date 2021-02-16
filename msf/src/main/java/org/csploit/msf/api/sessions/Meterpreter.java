package org.csploit.msf.api.sessions;

import org.csploit.msf.api.MsfException;

import java.io.IOException;

/**
 * This class represents a session compatible interface to a meterpreter server
 * instance running on a remote machine.  It provides the means of interacting
 * with the server instance both at an API level as well as at a console level.
 */
public interface Meterpreter extends SingleCommandShell {
  boolean detach() throws IOException, MsfException;
  boolean interrupt() throws IOException, MsfException;
  String[] tabs(String base) throws IOException, MsfException;
  String getPlatform();
  // TODO: transport_change
}
