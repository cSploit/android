package org.csploit.msf.impl.options;

import org.csploit.msf.impl.Option;

import java.io.File;

/**
 * File system path option.
 */
public class PathOption extends Option<File> {

  public PathOption(String name, boolean required, String description, File defaultValue) {
    super(name, required, description, defaultValue);
  }

  public PathOption(String name, String description, File defaultValue) {
    super(name, description, defaultValue);
  }

  @Override
  protected File normalize(String input) {
    File res = new File(input);
    return res.exists() ? res : null;
  }

  @Override
  protected boolean isValid(File input) {
    return super.isValid(input) && (input == null || input.exists());
  }

  @Override
  protected String display(File value) {
    return value.getAbsolutePath();
  }
}
