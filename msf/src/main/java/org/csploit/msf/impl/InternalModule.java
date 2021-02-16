package org.csploit.msf.impl;

import org.csploit.msf.api.License;
import org.csploit.msf.api.Module;
import org.csploit.msf.api.Option;
import org.csploit.msf.api.module.Rank;
import org.csploit.msf.impl.module.ArchSet;
import org.csploit.msf.impl.module.PlatformList;
import org.csploit.msf.impl.module.Reference;

/**
 * Give access to Module modifiers
 */
interface InternalModule extends Module, DataHolder, Offspring {
  String getType();
  void setAuthors(Author[] authors);
  void setArch(ArchSet arch);
  void setPlatform(PlatformList platform);
  void setReferences(Reference[] references);
  void setOptions(OptionContainer options);
  void setDatastore(ModuleDataStore datastore);
  void setPrivileged(boolean privileged);
  void setLicense(License license);
  void setRank(Rank rank);
  void setName(String name);
  void setVersion(String version);
  void setDescription(String description);
  void registerOption(Option option);
}