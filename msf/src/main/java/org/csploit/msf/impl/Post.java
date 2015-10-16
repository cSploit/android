package org.csploit.msf.impl;

import org.csploit.msf.api.License;
import org.csploit.msf.impl.module.ArchSet;
import org.csploit.msf.impl.module.PlatformList;
import org.csploit.msf.impl.module.Reference;

/**
 * A Post-exploitation module.
 */
class Post extends Module implements org.csploit.msf.api.Post {

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
