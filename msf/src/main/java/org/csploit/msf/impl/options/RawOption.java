package org.csploit.msf.impl.options;

import org.apache.commons.codec.DecoderException;
import org.apache.commons.io.IOUtils;
import org.csploit.msf.impl.AbstractOption;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Arrays;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Raw, arbitrary data option.
 */
public class RawOption extends AbstractOption<byte[]> {

  private static final Pattern hexStringPattern = Pattern.compile("([a-fA-F0-9]{2})+");
  private static final Pattern b64Pattern = Pattern.compile("([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{4}|[A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)");

  public RawOption(String name, boolean required, String description, byte[] defaultValue) {
    super(name, required, description, defaultValue);
  }

  public RawOption(String name, String description, byte[] defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  public byte[] normalize(String input) {
    if(input.startsWith("file:")) {
      try {
        return IOUtils.toByteArray(new URI(input));
      } catch (IOException | URISyntaxException e) {
        //ignore
      }
      return null;
    }
    Matcher matcher = b64Pattern.matcher(input);

    if(matcher.matches()) {
      return org.apache.commons.codec.binary.Base64.decodeBase64(input);
    }

    matcher = hexStringPattern.matcher(input);

    if(matcher.matches()) {
      try {
        return org.apache.commons.codec.binary.Hex.decodeHex(input.toCharArray());
      } catch (DecoderException e) {
        // ignored
      }
    }

    return null;
  }

  @Override
  public String display(byte[] value) {
    return new String(value);
  }
}
