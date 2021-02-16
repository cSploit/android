package org.csploit.msf.api;

import org.csploit.msf.api.module.Platform;
import org.csploit.msf.api.module.Rank;
import org.csploit.msf.api.module.Reference;

import java.util.Set;

/**
 * Represent a module of the MSF
 */
public interface Module extends Customizable, Comparable<Module> {
  String getRefname();
  String getFullName();
  String getShortName();
  String getDescription();
  String getName();
  License getLicense();
  Rank getRank();
  Reference[] getReferences();
  Author[] getAuthors();
  Set<Arch> getArches();
  Set<Platform> getPlatforms();
}
