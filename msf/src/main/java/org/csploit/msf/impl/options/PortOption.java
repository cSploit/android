package org.csploit.msf.impl.options;

import org.csploit.msf.impl.AbstractOption;

/**
 * Network port option.
 */
public class PortOption extends AbstractOption<Integer> {

  public PortOption(String name, boolean required, String description, Integer defaultValue) {
    super(name, required, description, defaultValue);
  }

  public PortOption(String name, String description, Integer defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  public Integer normalize(String input) {
    try {
      int val = Integer.parseInt(input);
      return isValid(val) ? val : null;
    } catch (NumberFormatException e) {
      // ignore
    }
    return null;
  }

  @Override
  public boolean isValid(Integer input) {
    return super.isValid(input) && (input == null || input >= 0 && input <= 65535);
  }
}
