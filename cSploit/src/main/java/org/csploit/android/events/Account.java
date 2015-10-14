package org.csploit.android.events;

import java.net.InetAddress;

/**
 * an account has been found
 */
public class Account implements Event {
  public final InetAddress address;
  public final String protocol;
  public final String username;
  public final String password;

  public Account(InetAddress address, String protocol, String username, String password) {
    this.address = address;
    this.protocol = protocol;
    this.username = username;
    this.password = password;
  }

  @Override
  public String toString() {
    return String.format("Account: { address='%s', protocol='%s', username='%s', password='%s' }",
            address.getHostAddress(), protocol, username, password);
  }
}
