package org.csploit.msf;

import org.csploit.msf.module.ArchSet;
import org.csploit.msf.module.PlatformList;
import org.csploit.msf.module.Reference;

/**
 * A Post-exploitation module.
 */
public class Post extends Module {

  public Post(String name, String description, String version, Author[] authors, ArchSet arch, PlatformList platform, Reference[] references, boolean privileged, License license) {
    super(name, description, version, authors, arch, platform, references, privileged, license);
  }

  public Post() {
    super();
  }

  @Override
  public String getType() {
    return "post";
  }
}
