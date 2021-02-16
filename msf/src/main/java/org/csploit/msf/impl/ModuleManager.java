package org.csploit.msf.impl;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Manage all framework modules
 */
class ModuleManager implements Offspring {

  final static String[] validModuleTypes = {
          "encoder",
          "exploit",
          "nop",
          "auxiliary",
          "payload",
          "post"
  };

  private Map<String, ModuleSet> sets = new HashMap<>();
  private InternalFramework framework;

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

  public InternalModule get(String name) {
    try {
      String[] parts = NameHelper.getTypeAndRefnameFromFullname(name);

      return get(parts[0], parts[1]);
    } catch (NameHelper.BadModuleNameException | NameHelper.BadModuleTypeException e) {
      return worldSearch(name);
    }
  }

  public InternalModule get(String type, String refname) {
    if(!sets.containsKey(type)) {
      if(isValidType(type)) {
        return null;
      }
      return worldSearch(type + "/" + refname);
    }
    return sets.get(type).get(refname);
  }

  @SuppressWarnings("unchecked")
  public <T extends InternalModule> List<T> getOfType(String type) {
    if(!sets.containsKey(type)) {
      return Collections.emptyList();
    }

    ArrayList<T> result = new ArrayList<>();
    result.addAll((Collection<? extends T>) sets.get(type).values());
    Collections.sort(result);
    return result;
  }

  private InternalModule worldSearch(String name) {
    for(ModuleSet set : sets.values()) {
      InternalModule m = set.get(name);
      if(m != null)
        return m;
    }
    return null;
  }

  public void put(InternalModule module) {
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
  public InternalFramework getFramework() {
    return framework;
  }

  @Override
  public void setFramework(InternalFramework framework) {
    this.framework = framework;
  }
}
