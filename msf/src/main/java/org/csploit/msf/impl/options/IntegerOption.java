package org.csploit.msf.impl.options;

import org.csploit.msf.impl.Option;

/**
 * Integer option.
 */
public class IntegerOption extends Option<Integer> {
  public IntegerOption(String name, boolean required, String description, Integer defaultValue) {
    super(name, required, description, defaultValue);
  }

  public IntegerOption(String name, String description, Integer defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  protected Integer normalize(String input) {
    try {
      if (input.startsWith("0x")) {
        return Integer.parseInt(input.substring(2), 16);
      } else {
        return Integer.parseInt(input);
      }
    } catch (NumberFormatException e) {
      return null;
    }
  }
}
