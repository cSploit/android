package org.csploit.msf.impl.options;

import org.csploit.msf.impl.AbstractOption;

import java.util.regex.Pattern;

/**
 * A regex option
 */
public class RegexpOption extends AbstractOption<Pattern> {

  public RegexpOption(String name, boolean required, String description, Pattern defaultValue) {
    super(name, required, description, defaultValue);
  }

  public RegexpOption(String name, String description, Pattern defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  public Pattern normalize(String input) {
    return Pattern.compile(input);
  }

  @Override
  public String display(Pattern value) {
    return value.pattern();
  }
}
