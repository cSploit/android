package org.csploit.msf;

import org.csploit.msf.module.ArchSet;
import org.csploit.msf.module.PlatformList;
import org.csploit.msf.module.Reference;

/**
 * This class represents the base class for a logical payload.  The framework
 * automatically generates payload combinations at runtime which are all
 * extended for this Payload as a base class.
 */
public class Payload extends Module {
  public Payload(String name, String description, String version, Author[] authors, ArchSet arch, PlatformList platform, Reference[] references, boolean privileged, License license) {
    super(name, description, version, authors, arch, platform, references, privileged, license);
  }

  public Payload() {
    super();
  }

  @Override
  public String getType() {
    return "payload";
  }
}
