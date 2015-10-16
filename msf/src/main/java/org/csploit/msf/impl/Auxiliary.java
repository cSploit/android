package org.csploit.msf.impl;

import org.csploit.msf.api.License;
import org.csploit.msf.impl.module.ArchSet;
import org.csploit.msf.impl.module.PlatformList;
import org.csploit.msf.impl.module.Reference;

/**
 * The auxiliary class acts as a base class for all modules that perform
 * reconnaissance, retrieve data, brute force logins, or any other action
 * that doesn't fit our concept of an 'exploit' (involving payloads and
 * targets and whatnot).
 */
class Auxiliary extends Module implements org.csploit.msf.api.Auxiliary {

  public Auxiliary(String name, String description, String version, Author[] authors, ArchSet arch, PlatformList platform, Reference[] references, boolean privileged, License license) {
    super(name, description, version, authors, arch, platform, references, privileged, license);
  }

  public Auxiliary() {
    super();
  }

  @Override
  public String getType() {
    return "auxiliary";
  }
}
