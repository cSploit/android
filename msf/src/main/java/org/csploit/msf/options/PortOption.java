package org.csploit.msf.options;

import org.csploit.msf.Option;

/**
 * Network port option.
 */
public class PortOption extends Option<Integer> {

  public PortOption(String name, boolean required, String description, Integer defaultValue) {
    super(name, required, description, defaultValue);
  }

  public PortOption(String name, String description, Integer defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  protected Integer normalize(String input) {
    try {
      int val = Integer.parseInt(input);
      return isValid(val) ? val : null;
    } catch (NumberFormatException e) {
      // ignore
    }
    return null;
  }

  @Override
  protected boolean isValid(Integer input) {
    return super.isValid(input) && (input == null || input >= 0 && input <= 65535);
  }
}
