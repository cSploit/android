package org.csploit.msf.impl;

import org.csploit.msf.api.License;
import org.csploit.msf.impl.module.ArchSet;
import org.csploit.msf.impl.module.PlatformList;
import org.csploit.msf.impl.module.Reference;

/**
 * This class represents the base class for a logical payload.  The framework
 * automatically generates payload combinations at runtime which are all
 * extended for this Payload as a base class.
 */
class Payload extends Module implements org.csploit.msf.api.Payload {
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
