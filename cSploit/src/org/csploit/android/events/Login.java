package org.csploit.android.events;

import java.net.InetAddress;

/**
 * a login has been found
 */
public class Login implements Event {
  public final short port;
  public final String address;
  public final String login;
  public final String password;

  public Login(short port, InetAddress address, String login, String password) {
    this.port = port;
    this.address = address.getHostAddress();
    this.login = login;
    this.password = password;
  }
  @Override
  public String toString() {
    return String.format("Login: { port=%d, address='%s', login='%s', password='%s' }",
            port, address, login, password);
  }
}
