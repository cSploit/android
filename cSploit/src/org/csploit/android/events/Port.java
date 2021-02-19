package org.csploit.android.events;

/**
 * an open port has been found
 */
public class Port implements Event {
  public final short port;
  public final String protocol;
  public final String service;
  public final String version;

  public Port(short port, String protocol) {
    this.port = port;
    this.protocol = protocol;
    this.service = null;
    this.version = null;
  }

  public Port(short port, String protocol, String service, String version) {
    this.protocol = protocol;
    this.port = port;
    this.service = service;
    this.version = version;
  }
  @Override
  public String toString() {
    return String.format("Port: { protocol='%s', port=%d, service='%s', version='%s' }",
            protocol, port, service, version);
  }
}
