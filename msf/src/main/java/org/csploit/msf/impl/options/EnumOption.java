package org.csploit.msf.impl.options;

import org.csploit.msf.impl.Option;

/**
 * Enum option.
 */
public class EnumOption extends Option<String> {

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
  protected String normalize(String input) {
    for(String s : enums) {
      if(s.equals(input)) {
        return s;
      }
    }
    return null;
  }

  @Override
  protected boolean isValid(String input) {
    return super.isValid(input) && (input == null || normalize(input) != null);
  }
}
