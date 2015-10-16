package org.csploit.msf.impl.options;

import org.csploit.msf.impl.Option;

import java.util.regex.Pattern;

/**
 * A regex option
 */
public class RegexpOption extends Option<Pattern> {

  public RegexpOption(String name, boolean required, String description, Pattern defaultValue) {
    super(name, required, description, defaultValue);
  }

  public RegexpOption(String name, String description, Pattern defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  protected Pattern normalize(String input) {
    return Pattern.compile(input);
  }

  @Override
  protected String display(Pattern value) {
    return value.pattern();
  }
}
