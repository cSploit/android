package org.csploit.msf.api;

import java.io.IOException;
import java.util.Collection;

/**
 * An object that can be tuned up
 * by editing it's options.
 */
public interface Customizable {
  void setOption(String key, String value);
  Collection<Option> getOptions() throws IOException, MsfException;
  Collection<String> getInvalidOptions();
  <T> T getOptionValue(Option<T> option);
}
