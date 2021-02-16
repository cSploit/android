package org.csploit.msf.impl.options;

import org.csploit.msf.impl.AbstractOption;

import java.util.regex.Pattern;

/**
 * Boolean option.
 */
public class BooleanOption extends AbstractOption<Boolean> {
  private static final Pattern TRUE = Pattern.compile("^(y|yes|t|1|true)$", Pattern.CASE_INSENSITIVE);
  private static final Pattern BOOLEAN = Pattern.compile("^(y|yes|n|no|t|f|0|1|true|false)$", Pattern.CASE_INSENSITIVE);

  public BooleanOption(String name, boolean required, String description, Boolean defaultValue) {
    super(name, required, description, defaultValue);
  }

  public BooleanOption(String name, String description, Boolean defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  public Boolean normalize(String input) {
    return input != null && TRUE.matcher(input).matches();
  }
}
