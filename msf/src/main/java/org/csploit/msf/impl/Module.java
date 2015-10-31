package org.csploit.msf.impl;

import org.csploit.msf.api.Arch;
import org.csploit.msf.api.License;
import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.module.Platform;
import org.csploit.msf.api.module.Rank;
import org.csploit.msf.impl.module.ArchSet;
import org.csploit.msf.impl.module.PlatformList;
import org.csploit.msf.impl.module.Reference;

import java.io.IOException;
import java.util.Collection;
import java.util.Set;

/**
 * Represent a module of the MSF
 */
abstract class Module implements InternalModule {
  protected Author[] authors;
  protected ArchSet arch;
  protected PlatformList platform;
  protected Reference[] references;
  protected OptionContainer options;
  protected InternalFramework framework;
  protected ModuleDataStore datastore;
  protected boolean privileged;
  protected License license;
  protected Rank rank;

  protected final String refname;
  protected String name;
  protected String description;
  protected String version;

  public Module(String refname) {
    this.refname = refname;
    this.options = new OptionContainer();
    this.datastore = new ModuleDataStore(this);
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

  @Override
  public DataStore getDataStore() {
    return datastore;
  }

  public String getRefname() {
    return refname;
  }

  public String getFullName() {
    return getType() + "/" + getRefname();
  }

  public String getShortName() {
    String[] parts = getRefname().split("/");
    return parts[parts.length - 1];
  }

  public void setAuthors(Author[] authors) {
    this.authors = authors;
  }

  public void setArch(ArchSet arch) {
    this.arch = arch;
  }

  public void setPlatform(PlatformList platform) {
    this.platform = platform;
  }

  public void setReferences(Reference[] references) {
    this.references = references;
  }

  public void setOptions(OptionContainer options) {
    this.options = options;
  }

  public void setDatastore(ModuleDataStore datastore) {
    this.datastore = datastore;
  }

  public void setPrivileged(boolean privileged) {
    this.privileged = privileged;
  }

  public void setLicense(License license) {
    this.license = license;
  }

  public void setRank(Rank rank) {
    this.rank = rank;
  }

  public void setName(String name) {
    this.name = name;
  }

  public void setVersion(String version) {
    this.version = version;
  }

  public void setDescription(String description) {
    this.description = description;
  }

  @Override
  public String getName() {
    return name;
  }

  @Override
  public License getLicense() {
    return license;
  }

  @Override
  public Rank getRank() {
    return rank;
  }

  @Override
  public Reference[] getReferences() {
    return references;
  }

  @Override
  public Author[] getAuthors() {
    return authors;
  }

  @Override
  public Set<Arch> getArches() {
    return arch;
  }

  @Override
  public Set<Platform> getPlatforms() {
    return platform;
  }

  public int compareTo(org.csploit.msf.api.Module module) {
    return getFullName().compareToIgnoreCase(module.getFullName());
  }

  @Override
  public String getDescription() {
    return description;
  }

  public void registerOption(String name, Option option, boolean advanced, boolean evasion) {
    options.addOption(name, option, advanced, evasion);
  }

  @Override
  public void setOption(String key, String value) {
    getDataStore().put(key, value);
  }

  @Override
  public Collection<Option> getOptions() throws IOException, MsfException {
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

  public static InternalModule fromType(String type, String refname) throws NameHelper.BadModuleTypeException {
    switch (type) {
      case "exploit":
        return new Exploit(refname);
      case "payload":
        return new Payload(refname);
      case "post":
        return new Post(refname);
      case "nop":
      case "encoder":
        //TODO
      default:
        throw new NameHelper.BadModuleTypeException(type);
    }
  }
}
