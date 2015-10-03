package org.csploit.msf;

import org.csploit.msf.module.ArchSet;
import org.csploit.msf.module.ModuleDataStore;
import org.csploit.msf.module.PlatformList;
import org.csploit.msf.module.Reference;

import java.util.Collection;

/**
 * Represent a module of the MSF
 */
public abstract class Module implements Offspring, DataHolder, Customizable {
  protected Author[] authors;
  protected ArchSet arch;
  protected PlatformList platform;
  protected Reference[] references;
  protected OptionContainer options;
  protected Framework framework;
  protected ModuleDataStore datastore;
  protected boolean privileged;
  protected License license;

  protected String name;
  protected String refname;
  protected String description;
  protected String version;

  public Module(String name, String description, String version, Author[] authors, ArchSet arch, PlatformList platform, Reference[] references, boolean privileged, License license ) {
    this.name = name;
    this.description = description;
    this.version = version;
    this.authors = authors;
    this.arch = arch;
    this.platform = platform;
    this.references = references;
    this.privileged = privileged;
    this.license = license;
    this.options = new OptionContainer();
    this.datastore = new ModuleDataStore(this);
  }

  public Module() {
    this("No module name", "No module description", "0", null, new ArchSet(), new PlatformList(), null, false, License.MSF);
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

  @Override
  public DataStore getDataStore() {
    return datastore;
  }

  public String getRefname() {
    return refname;
  }

  public void setRefname(String refname) {
    this.refname = refname;
  }

  public abstract String getType();

  public String getFullName() {
    return getType() + "/" + getRefname();
  }

  public String getShortName() {
    String[] parts = getRefname().split("/");
    return parts[parts.length - 1];
  }

  public void registerOption(String name, Option option, boolean advanced, boolean evasion) {
    options.addOption(name, option, advanced, evasion);
  }

  @Override
  public void setOption(String key, String value) {
    getDataStore().put(key, value);
  }

  @Override
  public Collection<Option> getOptions() {
    return options.values();
  }

  @Override
  public Collection<String> getInvalidOptions() {
    return options.getInvalidOptions(getDataStore());
  }

  @Override
  public <T> T getOptionValue(Option<T> option) {
    return option.normalize(getDataStore().get(option.getName()));
  }

  @Override
  public int hashCode() {
    return refname != null ? refname.hashCode() : 0;
  }
}
