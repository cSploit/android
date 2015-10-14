package org.csploit.msf;

import java.util.HashMap;
import java.util.Map;

/**
 * Manage all framework modules
 */
public class ModuleManager implements Offspring {

  private final static String[] validModuleTypes = {
          "encoder",
          "exploit",
          "nop",
          "auxiliary",
          "payload",
          "post"
  };

  private Map<String, ModuleSet> sets = new HashMap<>();
  private Framework framework;

  public ModuleManager(Framework framework, String types) {
    this(framework, types.equals("all") ? validModuleTypes : types.split(","));
  }

  public ModuleManager(Framework framework, String[] types) {
    setFramework(framework);
    for(String type : types) {
      type = type.trim();
      if(isValidType(type))
        initSet(type);
    }
  }

  private void initSet(String type) {
    if(sets.containsKey(type))
      return;

    ModuleSet set = ModuleSet.forType(type);
    set.setFramework(getFramework());
    sets.put(type, set);
  }

  private static boolean isValidType(String type) {
    for(String t : validModuleTypes) {
      if(t.equals(type))
        return true;
    }
    return false;
  }

  public Module get(String name) {
    String[] parts = name.split("/");
    String refname;

    if(!sets.containsKey(parts[0])) {
      if(!isValidType(parts[0])) {
        return worldSearch(name);
      } else {
        return null;
      }
    }

    refname = name.substring(parts[0].length() + 1);

    return sets.get(parts[0]).get(refname);
  }

  private Module worldSearch(String name) {
    for(ModuleSet set : sets.values()) {
      Module m = set.get(name);
      if(m != null)
        return m;
    }
    return null;
  }

  public void put(Module module) {
    String type = module.getType();
    if(!sets.containsKey(type)) {
      return;
    }
    sets.get(type).add(module);
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
}
