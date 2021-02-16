package org.csploit.msf.api.module;

import org.csploit.msf.api.Arch;

import java.util.Set;

/**
 * Represent a target
 */
public interface Target {
  String getName();
  Set<Platform> getPlatform();
  Set<Arch> getArch();
}
