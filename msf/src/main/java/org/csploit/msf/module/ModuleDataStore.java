package org.csploit.msf.module;

import org.csploit.msf.Module;

/**
 * DataStore wrapper for modules that will attempt to back values against the
 * framework's datastore if they aren't found in the module's datastore.  This
 * is done to simulate global data store values.
 */
public class ModuleDataStore extends org.csploit.msf.DataStore {
  private Module module;

  public ModuleDataStore(Module module) {
    this.module = module;
  }

  public boolean isDefault(String key) {
    return defaults.contains(key);
  }

  @Override
  public String get(Object key) {
    String k, res;

    k = key != null ? key.toString() : null;
    k = findKeyCase(k);
    res = null;
    if(!isDefault(k))
      res = super.get(key);

    if(res == null && module != null && module.haveFramework()) {
      res = module.getFramework().getDataStore().get(k);
    }

    return res != null ? res : super.get(k);
  }
}
