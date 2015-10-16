package org.csploit.msf.impl;

import org.csploit.msf.impl.module.ArchSet;
import org.csploit.msf.impl.module.PlatformList;

import java.util.HashMap;

/**
 * A module set contains zero or more named module classes of an arbitrary
 * type.
 */
class ModuleSet extends HashMap<String, Module> implements Offspring {
  protected String type;
  private Framework framework;

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
  public Framework getFramework() {
    return framework;
  }

  @Override
  public void setFramework(Framework framework) {
    this.framework = framework;
  }

  public void add(Module module) {
    module.setFramework(framework);
    put(module.getRefname(), module);
  }

  public ModuleSet filter(ArchSet architectures, PlatformList platforms) {
    ModuleSet res = new ModuleSet(getType());
    res.setFramework(framework);
    for(Module m : values()) {

      if(architectures != null && m.arch.intersect(architectures).isEmpty()) {
        continue;
      }

      if(platforms != null && m.platform.intersect(platforms).isEmpty()) {
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
