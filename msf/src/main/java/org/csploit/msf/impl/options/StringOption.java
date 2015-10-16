package org.csploit.msf.impl.options;

import org.apache.commons.io.IOUtils;
import org.csploit.msf.impl.Option;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;

/**
 * Option string
 */
public class StringOption extends Option<String> {

  public StringOption(String name, boolean required, String description, String defaultValue) {
    super(name, required, description, defaultValue);
  }

  public StringOption(String name, String description, String defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  protected String normalize(String input) {
    if(input.startsWith("file:")) {
      try {
        return IOUtils.toString(new URI(input));
      } catch (IOException | URISyntaxException e) {
        //ignore
      }
      return null;
    }
    return input;
  }
}
