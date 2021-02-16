package org.csploit.msf.impl.options;

import org.csploit.msf.impl.AbstractOption;

import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * Network address option.
 */
public class AddressOption extends AbstractOption<InetAddress> {

  public AddressOption(String name, boolean required, String description, InetAddress defaultValue) {
    super(name, required, description, defaultValue);
  }

  public AddressOption(String name, boolean required, String description, String defaultValue) {
    super(name, required, description, sNormalize(defaultValue));
  }

  public AddressOption(String name, String description, InetAddress defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  public InetAddress normalize(String input) {
    return sNormalize(input);
  }

  private static InetAddress sNormalize(String input) {
    try {
      return input != null ? InetAddress.getByName(input) : null;
    } catch (UnknownHostException e) {
      return null;
    }
  }
}
