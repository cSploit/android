package org.csploit.msf.impl;

import org.csploit.msf.impl.module.ArchSet;
import org.csploit.msf.impl.module.PlatformList;

import java.util.HashMap;

/**
 * A module set contains zero or more named module classes of an arbitrary
 * type.
 */
class ModuleSet extends HashMap<String, InternalModule> implements Offspring {
  protected String type;
  private InternalFramework framework;

  public ModuleSet(String type) {
    this.type = type;
  }

  public String getType() {
    return type;
  }

  @Override
  public boolean haveFramework() {
    return framework != null;
  }

  @Override
  public InternalFramework getFramework() {
    return framework;
  }

  @Override
  public void setFramework(InternalFramework framework) {
    this.framework = framework;
  }

  public void add(InternalModule module) {
    module.setFramework(framework);
    put(module.getRefname(), module);
  }

  public ModuleSet filter(ArchSet architectures, PlatformList platforms) {
    ModuleSet res = new ModuleSet(getType());
    res.setFramework(framework);
    for(InternalModule m : values()) {

      if(architectures != null && architectures.intersect(m.getArches()).isEmpty()) {
        continue;
      }

      if(platforms != null && platforms.intersect(m.getPlatforms()).isEmpty()) {
        continue;
      }

      //TODO: custom filter

      res.add(m);
    }
    return res;
  }

  public static ModuleSet forType(String type) {
    //TODO: PayloadSet
    return new ModuleSet(type);
  }
}
