package it.evilsocket.dsploit.events;

import java.net.InetAddress;

import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.net.Network.Protocol;
import it.evilsocket.dsploit.net.Target;

/**
 * an open port has been found
 */
public class Port implements Event {
  public final String protocol;
  public final int port;
  public final String service;
  public final String version;

  public Port(String proto, int port) {
    this.protocol = proto;
    this.port = port;
    this.service = null;
    this.version = null;
  }

  public Port(String proto, int port, String service, String version) {
    this.protocol = proto;
    this.port = port;
    this.service = service;
    this.version = version;
  }

  public String toString() {
    return String.format("Port: { protocol='%s', port=%d, service='%s', version='%s' }",
            protocol, port, service, version);
  }
}
