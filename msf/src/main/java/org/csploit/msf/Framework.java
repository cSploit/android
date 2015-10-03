package org.csploit.msf;

import java.util.Collection;

/**
 * Represent a MetaSploit Framework instance
 */
public class Framework implements DataHolder {

  private DataStore dataStore;
  private ModuleManager moduleManager;
  private JobContainer jobs;
  private SessionManager sessions;

  public Framework() {
    dataStore = new DataStore();
    moduleManager = new ModuleManager(this, "all");
    jobs = new JobContainer();
    sessions = new SessionManager(this);
  }

  @Override
  public DataStore getDataStore() {
    return dataStore;
  }

  public ModuleManager getModuleManager() {
    return moduleManager;
  }

  public void setGlobalOption(String key, String value) {
    getDataStore().put(key, value);
  }
}
