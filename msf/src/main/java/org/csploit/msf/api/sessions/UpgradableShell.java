package org.csploit.msf.api.sessions;

import org.csploit.msf.api.MsfException;

import java.io.IOException;
import java.net.InetAddress;

/**
 * A Shell that can be upgrade to a Metrpreter one
 */
public interface UpgradableShell {
  /**
   * open a new Meterpreter session on the target
   * @param lhost local host address
   * @param lport local bound port
   */
  void upgrade(InetAddress lhost, int lport) throws IOException, MsfException;
}
