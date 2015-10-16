package org.csploit.msf.api.options;

import org.csploit.msf.api.Option;

/**
 * Enum option.
 */
public interface EnumOption extends Option<String> {
  String[] getValues();
}
