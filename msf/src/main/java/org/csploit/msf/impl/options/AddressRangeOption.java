package org.csploit.msf.impl.options;

import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.StringUtils;
import org.csploit.msf.impl.AbstractOption;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

/**
 * Network address range option.
 */
public class AddressRangeOption extends AbstractOption<InetAddress[]> {

  public AddressRangeOption(String name, boolean required, String description, InetAddress[] defaultValue) {
    super(name, required, description, defaultValue);
  }

  public AddressRangeOption(String name, String description, InetAddress[] defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  public InetAddress[] normalize(String input) {
    if(input == null)
      return null;

    if(input.startsWith("file:")) {
      InputStream is = null;
      try {
        input = input.substring(5).trim();
        is = new FileInputStream(new File(input));
        List<String> lines = IOUtils.readLines(is);
        ArrayList<InetAddress> result = new ArrayList<>();

        for(String line : lines) {
          try {
            result.add(InetAddress.getByName(line.trim()));
          } catch (UnknownHostException e) {
            // ignore
          }
        }

        return result.toArray(new InetAddress[result.size()]);
      } catch (IOException e) {
        // ignore
      } finally {
        IOUtils.closeQuietly(is);
      }
    } else if ( input.startsWith("rand:")) {
      throw new UnsupportedOperationException("random address ranges not supported");
    } else {
      String[] parts = input.split(" ");
      ArrayList<InetAddress> result = new ArrayList<>();

      for(String part : parts) {
        try {
          result.add(InetAddress.getByName(part));
        } catch (UnknownHostException e) {
          // ignore
        }
      }

      return result.toArray(new InetAddress[result.size()]);
    }
    return null;
  }

  @Override
  public String display(InetAddress[] value) {
    return StringUtils.join(value, " ");
  }
}
