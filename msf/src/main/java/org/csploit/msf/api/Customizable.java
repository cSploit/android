package org.csploit.msf.api;

import org.csploit.msf.impl.Option;

import java.util.Collection;

/**
 * An object that can be tuned up
 * by editing it's options.
 */
public interface Customizable {
  void setOption(String key, String value);
  Collection<Option> getOptions();
  Collection<String> getInvalidOptions();
  <T> T getOptionValue(Option<T> option);
}
