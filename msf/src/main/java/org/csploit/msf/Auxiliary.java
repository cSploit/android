package org.csploit.msf;

import org.csploit.msf.module.ArchSet;
import org.csploit.msf.module.PlatformList;
import org.csploit.msf.module.Reference;

/**
 * The auxiliary class acts as a base class for all modules that perform
 * reconnaissance, retrieve data, brute force logins, or any other action
 * that doesn't fit our concept of an 'exploit' (involving payloads and
 * targets and whatnot).
 */
public class Auxiliary extends Module {

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
