package org.csploit.msf.impl.module;

/**
 * Represent a target
 */
public class Target implements org.csploit.msf.api.module.Target {
  private String name;
  private PlatformList platform;
  private ArchSet arch;

  public Target(String name, PlatformList platform, ArchSet arch) {
    this.name = name;
    this.platform = platform;
    this.arch = arch;
  }

  public Target(String name) {
    this(name, new PlatformList(), new ArchSet());
  }

  public String getName() {
    return name;
  }

  public PlatformList getPlatform() {
    return platform;
  }

  public ArchSet getArch() {
    return arch;
  }
}
