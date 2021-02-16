package org.csploit.msf.impl;

import org.csploit.msf.api.Option;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * The data store is just a bitbucket that holds keyed values.  It is used
 * by various classes to hold option values and other state information.
 */
class DataStore extends HashMap<String, String> {
  protected Set<String> imported = new HashSet<>();
  protected Set<String> defaults = new HashSet<>();

  @Override
  public String put(String key, String value) {
    key = findKeyCase(key);

    defaults.remove(key);
    imported.remove(key);

    return super.put(key, value);
  }

  @Override
  public String get(Object key) {
    String k = (key instanceof String) ? (String) key : key.toString();
    return super.get(findKeyCase(k));
  }

  public void updateValue(String key, String value) {
    super.put(key, value);
  }

  /**
   * This method is a helper method that imports the default value for
   * all of the supplied options
   */
  public void importOptions(OptionContainer options, boolean isDefault, boolean overwrite) {
    for(Option opt : options.values()) {
      String name = opt.getName();
      if(containsKey(name) && !overwrite) {
        continue;
      }
      if(opt.haveDefaultValue() && (overwrite || get(name) == null)) {
        String val = opt.display(opt.getDefaultValue());
        importOption(name, val, true, isDefault);
      }
    }
  }

  public void importOptions(OptionContainer options, boolean isDefault) {
    importOptions(options, isDefault, false);
  }

  public void importOptions(OptionContainer options) {
    importOptions(options, false, false);
  }

  public void importOption(String name, String val, boolean imported, boolean isDefault) {
    if(imported) {
      this.imported.add(name);
    }

    if(isDefault) {
      defaults.add(name);
    }

    super.put(name, val);
  }

  public void importOption(String name, String val, boolean imported) {
    importOption(name, val, imported, false);
  }

  public void importOption(String name, String val) {
    importOption(name, val, true, false);
  }

  public synchronized Map<String, String> userDefined() {
    Map<String, String> res = new HashMap<>();

    for (String key : keySet()) {
      if (!imported.contains(key)) {
        res.put(key, super.get(key));
      }
    }

    return res;
  }

  public void clearNotUserDefined() {
    for(String key : imported) {
      if(defaults.remove(key)) {
        remove(key);
      }
    }
  }
  
  protected String findKeyCase(String k) {

    if(k == null)
      return null;

    for(String key : keySet()) {
      if(key.toLowerCase().equals(k.toLowerCase()))
        return key;
    }
    
    return k;
  }
}
