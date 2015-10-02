package org.csploit.msf;

/**
 * Represent a MetaSploit Framework instance
 */
public class Framework implements DataHolder {

  private DataStore dataStore;
  private ModuleManager moduleManager;

  public Framework() {
    dataStore = new DataStore();
    moduleManager = new ModuleManager(this, "all");
  }

  @Override
  public DataStore getDataStore() {
    return dataStore;
  }

  public ModuleManager getModuleManager() {
    return moduleManager;
  }
}
