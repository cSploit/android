package org.csploit.msf.impl.options;

import org.csploit.msf.impl.AbstractOption;

/**
 * Enum option.
 */
public class EnumOption extends AbstractOption<String> {

  private String[] enums;

  public EnumOption(String name, boolean required, String description, String defaultValue, String[] enums) {
    super(name, required, description, defaultValue);
    this.enums = enums;
  }

  public EnumOption(String name, String description, String defaultValue, String[] enums) {
    super(name, description, defaultValue);
    this.enums = enums;
  }

  @Override
  public String normalize(String input) {
    for(String s : enums) {
      if(s.equals(input)) {
        return s;
      }
    }
    return null;
  }

  @Override
  public boolean isValid(String input) {
    return super.isValid(input) && (input == null || normalize(input) != null);
  }
}
