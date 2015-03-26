package org.csploit.android.events;

/**
 * an open port has been found
 */
public class Port implements Event {
  public final String protocol;
  public final int port;
  public final String service;
  public final String version;

  public Port(String protocol, int port) {
    this.protocol = protocol;
    this.port = port;
    this.service = null;
    this.version = null;
  }

  public Port(String protocol, int port, String service, String version) {
    this.protocol = protocol;
    this.port = port;
    this.service = service;
    this.version = version;
  }

  public String toString() {
    return String.format("Port: { protocol='%s', port=%d, service='%s', version='%s' }",
            protocol, port, service, version);
  }
}
