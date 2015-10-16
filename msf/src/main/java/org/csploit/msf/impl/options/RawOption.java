package org.csploit.msf.impl.options;

import org.apache.commons.io.IOUtils;
import org.csploit.msf.impl.Option;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Arrays;

/**
 * Raw, arbitrary data option.
 */
public class RawOption extends Option<byte[]> {

  public RawOption(String name, boolean required, String description, byte[] defaultValue) {
    super(name, required, description, defaultValue);
  }

  public RawOption(String name, String description, byte[] defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  protected byte[] normalize(String input) {
    if(input.startsWith("file:")) {
      try {
        IOUtils.toByteArray(new URI(input));
      } catch (IOException | URISyntaxException e) {
        //ignore
      }
      return null;
    }
    return input.getBytes();
  }

  @Override
  protected String display(byte[] value) {
    return Arrays.toString(value);
  }
}
