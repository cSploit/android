package org.csploit.msf.impl;

import org.csploit.msf.api.Arch;
import org.csploit.msf.api.Exploit;
import org.csploit.msf.api.License;
import org.csploit.msf.api.Module;
import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.Option;
import org.csploit.msf.api.Payload;
import org.csploit.msf.api.Post;
import org.csploit.msf.api.Session;
import org.csploit.msf.api.module.AuxiliaryAction;
import org.csploit.msf.api.module.Platform;
import org.csploit.msf.api.module.Rank;
import org.csploit.msf.api.module.Target;
import org.csploit.msf.api.sessions.CommandShell;
import org.csploit.msf.api.sessions.Interactive;
import org.csploit.msf.api.sessions.UpgradableShell;
import org.csploit.msf.impl.module.ArchSet;
import org.csploit.msf.impl.module.PlatformList;
import org.csploit.msf.impl.module.Reference;

import java.io.IOException;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Collection;
import java.util.EventListener;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * A proxy that use Msgpack to talk with the framework instance
 *
 * used also as container for other Msgpack proxies classes
 */
@SuppressWarnings("unchecked, rawtypes")
class MsgpackProxy implements InternalFramework {
  private final Framework framework = new Framework();
  private final MsgpackClient client;
  private final MsgpackLoader loader;

  public MsgpackProxy(MsgpackClient client) {
    this.client = client;
    this.loader = new MsgpackLoader(client, this);
    framework.getSessionManager().setWatchList(true);
  }

  public void setGlobalOption(String key, String value) {
    framework.setGlobalOption(key, value);
  }

  @Override
  public void unsetGlobalOption(String key) {
    framework.unsetGlobalOption(key);
  }

  public ModuleManager getModuleManager() {
    return framework.getModuleManager();
  }

  @Override
  public DataStore getDataStore() {
    return framework.getDataStore();
  }

  @Override
  public Module getModule(String refname) throws IOException, MsfException {
    return loader.loadModule(refname);
  }

  @Override
  public JobContainer getJobContainer() {
    return framework.getJobContainer();
  }

  @Override
  public void registerJob(Job job) {
    framework.registerJob(job);
  }

  @Override
  public void unregisterJob(int id) {
    framework.unregisterJob(id);
  }

  @Override
  public Job getJob(int id) throws IOException, MsfException {
    return loader.loadJob(id);
  }

  @Override
  public List<Job> getJobs() throws IOException, MsfException {
    Map<Integer, String> result = client.getJobs();

    for(Job j : framework.getJobs()) {
      if(!result.containsKey(j.getId())) {
        framework.unregisterJob(j.getId());
      }
    }

    for(int id : result.keySet()) {
      loader.lazyLoadJob(id, result.get(id));
    }

    return framework.getJobs();
  }

  @Override
  public List<Session> getSessions() throws IOException, MsfException {
    Map<Integer, MsgpackClient.SessionInfo> result = client.getSessions();
    SessionManager manager = framework.getSessionManager();

    for(Session session : manager.getSessions()) {
      if(result.containsKey(session.getId())) {
        // remove a session from the results
        // when we already have it locally.
        result.remove(session.getId());
      } else {
        // remove a session from local store
        // when the remote lost it.
        manager.unregister(session);
      }
    }

    // parse the remaining results

    for(int id : result.keySet()) {
      try {
        Session session = loader.buildSession(id, result.get(id));
        manager.register(session);
      } catch (NameHelper.BadSessionTypeException e) {
        // not supported yet
      }
    }

    return manager.getSessions();
  }

  @Override
  public SessionManager getSessionManager() {
    return framework.getSessionManager();
  }

  @Override
  public void addSubscriber(EventListener listener) throws IOException, MsfException {
    framework.addSubscriber(listener);
  }

  @Override
  public void removeSubscriber(EventListener listener) throws IOException, MsfException {
    framework.removeSubscriber(listener);
  }

  @Override
  public List<? extends Exploit> getExploits() throws IOException, MsfException {
    if(framework.getModuleManager().getOfType("exploit").isEmpty()) {
      for(String name : client.getExploits()) {
        loader.lazyLoadModule("exploit", name);
      }
    }
    return framework.getExploits();
  }

  @Override
  public List<? extends Payload> getPayloads() throws IOException, MsfException {
    if(framework.getModuleManager().getOfType("payload").isEmpty()) {
      for(String name : client.getPayloads()) {
        loader.lazyLoadModule("payload", name);
      }
    }
    return framework.getPayloads();
  }

  @Override
  public List<? extends Post> getPosts() throws IOException, MsfException {
    if(framework.getModuleManager().getOfType("post").isEmpty()) {
      for(String name : client.getPosts()) {
        loader.lazyLoadModule("post", name);
      }
    }
    return framework.getPosts();
  }

  interface MsgpackCommunicator {
    MsgpackClient getClient();
    void setClient(MsgpackClient client);
  }

  private static abstract class SessionProxy extends AbstractSession implements MsgpackCommunicator, Interactive {
    private MsgpackClient client;
    private boolean interacting = false;

    public SessionProxy(int id) {
      super(id);
    }

    @Override
    public List<Module> getCompatibleModules() throws IOException, MsfException {
      String[] values = getClient().getSessionCompatibleModules(getId());
      MsgpackProxy framework = (MsgpackProxy) getFramework();
      MsgpackLoader loader = framework.loader;
      ModuleManager moduleManager = framework.getModuleManager();

      List<Module> result = new ArrayList<>(values.length);

      for(String fullname : values) {
        Module m = moduleManager.get(fullname);
        if(m != null) {
          result.add(m);
          continue;
        }

        try {
          String[] names = NameHelper.getTypeAndRefnameFromFullname(fullname);
          result.add(loader.lazyLoadModule(names[0], names[1]));
        } catch (NameHelper.BadModuleNameException | NameHelper.BadModuleTypeException e) {
          e.printStackTrace();
        }
      }

      return result;
    }

    @Override
    public void close() throws IOException, MsfException {
      getClient().stopSession(getId());
    }

    @Override
    public void interact() throws IOException, MsfException {
      interacting = true;
    }

    @Override
    public void suspend() throws IOException, MsfException {
      interacting = false;
    }

    @Override
    public void complete() throws IOException, MsfException {
      interacting = false;
      close();
    }

    @Override
    public boolean isInteracting() throws IOException, MsfException {
      return interacting;
    }

    @Override
    public MsgpackClient getClient() {
      return client;
    }

    @Override
    public void setClient(MsgpackClient client) {
      this.client = client;
    }
  }

  static class ShellProxy extends SessionProxy implements CommandShell, UpgradableShell {

    public ShellProxy(int id) {
      super(id);
    }

    @Override
    public void init() {
      // nothing to do there
    }

    @Override
    public String read() throws IOException, MsfException {
      return getClient().readFromShellSession(getId());
    }

    @Override
    public int write(String data) throws IOException, MsfException {
      return getClient().writeToShellSession(getId(), data);
    }

    @Override
    public void upgrade(InetAddress lhost, int lport) throws IOException, MsfException {
      getClient().upgradeShellSession(getId(), lhost.getHostAddress(), lport);
    }
  }

  static class MeterpreterProxy extends SessionProxy implements InternalMeterpreter {
    private String platform = null;

    public MeterpreterProxy(int id) {
      super(id);
    }

    @Override
    public void init() {
      // nothing to do there
    }

    @Override
    public String read() throws IOException, MsfException {
      return getClient().readFromMeterpreterSession(getId());
    }

    @Override
    public int write(String data) throws IOException, MsfException {
      getClient().writeToMeterpreterSession(getId(), data);
      return data.length();
    }

    @Override
    public boolean detach() throws IOException, MsfException {
      return getClient().detachMeterpreterSession(getId());
    }

    @Override
    public boolean interrupt() throws IOException, MsfException {
      return getClient().killMeterpreterSession(getId());
    }

    @Override
    public String[] tabs(String base) throws IOException, MsfException {
      return getClient().getMeterpreterTabs(getId(), base);
    }

    @Override
    public void setPlatform(String platform) {
      this.platform = platform;
    }

    @Override
    public String getPlatform() {
      return platform;
    }
  }

  static abstract class ModuleProxy<T extends InternalModule> implements InternalModule, MsgpackCommunicator {
    private MsgpackClient client;
    private MsgpackLoader loader;
    protected T instance;

    public ModuleProxy(T instance) {
      this.instance = instance;
    }

    @Override
    public MsgpackClient getClient() {
      return client;
    }

    @Override
    public void setClient(MsgpackClient client) {
      this.client = client;
    }

    protected MsgpackLoader getLoader() {
      if(loader == null) {
        loader = new MsgpackLoader(client, getFramework());
      }
      return loader;
    }

    @Override
    public Collection<Option> getOptions() throws IOException, MsfException {

      if(getClient() != null && instance.getOptions().isEmpty()) {
        MsgpackLoader.fillModuleOptions(instance,
                getClient().getModuleOptions(getType(), getRefname()));
      }

      return instance.getOptions();
    }

    @Override
    public Collection<String> getInvalidOptions() {
      return instance.getInvalidOptions();
    }

    @Override
    public <K> K getOptionValue(Option<K> option) {
      return instance.getOptionValue(option);
    }

    @Override
    public boolean haveFramework() {
      return instance.haveFramework();
    }

    public InternalFramework getFramework() {
      return instance.getFramework();
    }

    public void setFramework(InternalFramework framework) {
      instance.setFramework(framework);
    }

    protected Map options4client() throws IOException, MsfException {
      Map<String, Object> options = new HashMap<>();

      for(Option option: getOptions()) {
        options.put(option.getName(), getOptionValue(option));
      }
      return options;
    }

    protected Job executeWithJob() throws IOException, MsfException {
      int jobId = getClient().execute(getType(), getRefname(), options4client());
      return ((MsgpackProxy) instance.getFramework()).loader.
              lazyLoadJob(jobId, getName() + " job");
    }

    public void setPlatform(PlatformList platform) {
      instance.setPlatform(platform);
    }

    public void setAuthors(Author[] authors) {
      instance.setAuthors(authors);
    }

    public void setArch(ArchSet arch) {
      instance.setArch(arch);
    }

    public void setReferences(Reference[] references) {
      instance.setReferences(references);
    }

    public void setOptions(OptionContainer options) {
      instance.setOptions(options);
    }

    public void setDatastore(ModuleDataStore datastore) {
      instance.setDatastore(datastore);
    }

    public void setPrivileged(boolean privileged) {
      instance.setPrivileged(privileged);
    }

    public void setLicense(License license) {
      instance.setLicense(license);
    }

    public void setRank(Rank rank) {
      instance.setRank(rank);
    }

    public void setName(String name) {
      instance.setName(name);
    }

    public void setVersion(String version) {
      instance.setVersion(version);
    }

    public void setDescription(String description) {
      instance.setDescription(description);
    }

    public void registerOption(Option option) {
      instance.registerOption(option);
    }

    @Override
    public String getRefname() {
      return instance.getRefname();
    }

    @Override
    public String getFullName() {
      return instance.getFullName();
    }

    @Override
    public String getShortName() {
      return instance.getShortName();
    }

    @Override
    public String getDescription() {
      return instance.getDescription();
    }

    @Override
    public String getName() {
      return instance.getName();
    }

    @Override
    public License getLicense() {
      return instance.getLicense();
    }

    @Override
    public Rank getRank() {
      return instance.getRank();
    }

    @Override
    public org.csploit.msf.api.module.Reference[] getReferences() {
      return instance.getReferences();
    }

    @Override
    public org.csploit.msf.api.Author[] getAuthors() {
      return instance.getAuthors();
    }

    @Override
    public void setOption(String key, String value) {
      instance.setOption(key, value);
    }

    @Override
    public DataStore getDataStore() {
      return instance.getDataStore();
    }

    @Override
    public Set<Arch> getArches() {
      return instance.getArches();
    }

    @Override
    public int compareTo(Module module) {
      return instance.compareTo(module);
    }

    @Override
    public Set<Platform> getPlatforms() {
      return instance.getPlatforms();
    }

    @Override
    public String getType() {
      return instance.getType();
    }
  }

  static class ExploitProxy extends ModuleProxy<InternalExploit> implements InternalExploit {

    public ExploitProxy(InternalExploit instance) {
      super(instance);
    }

    public void setTargets(Target[] targets) {
      instance.setTargets(targets);
    }

    public void setDefaultTarget(int defaultTarget) {
      instance.setDefaultTarget(defaultTarget);
    }

    public Target[] getTargets() {
      return instance.getTargets();
    }

    public Target getTarget() {
      return instance.getTarget();
    }

    @Override
    public void setTarget(int i) {
      instance.setTarget(i);
    }

    @Override
    public int getTargetIndex() {
      return instance.getTargetIndex();
    }

    @Override
    public List<? extends Payload> getCompatiblePayloads() throws IOException, MsfException {
      String [] response = getClient()
              .getExploitCompatiblePayloads(getRefname(), getTargetIndex());
      List<Payload> result = new ArrayList<>();

      for(String refname : response) {
        result.add((Payload) getLoader().lazyLoadModule("payload", refname));
      }

      return result;
    }

    @Override
    public void execute() throws IOException, MsfException {
      Job result = executeWithJob();
      result.setExploit(this);
      //TODO: result.setPayload(getPayload());
    }
  }

  static class PayloadProxy extends ModuleProxy<InternalModule> implements Payload {

    public PayloadProxy(InternalModule payload) {
      super(payload);
    }

    @Override
    public byte[] generate() throws IOException, MsfException {
      return getClient().generatePayload("payload", getRefname(), options4client());
    }
  }

  static class PostProxy extends ModuleProxy<InternalModule> implements Post {

    public PostProxy(InternalModule instance) {
      super(instance);
    }

    @Override
    public void execute() throws IOException, MsfException {
      executeWithJob();
    }
  }

  static class AuxiliaryProxy extends ModuleProxy<InternalAuxiliary> implements InternalAuxiliary {

    public AuxiliaryProxy(InternalAuxiliary instance) {
      super(instance);
    }

    @Override
    public void setDefaultAction(String defaultAction) {
      instance.setDefaultAction(defaultAction);
    }

    @Override
    public void setActions(AuxiliaryAction[] actions) {
      instance.setActions(actions);
    }

    @Override
    public void setDefaultAction(int defaultAction) {
      instance.setDefaultAction(defaultAction);
    }

    @Override
    public AuxiliaryAction[] getActions() {
      return instance.getActions();
    }

    @Override
    public AuxiliaryAction getDefaultAction() {
      return instance.getDefaultAction();
    }

    @Override
    public void execute() throws IOException, MsfException {
      executeWithJob();
    }
  }

  public static class FrameworkFactory {
    private final MsgpackClient client;

    public FrameworkFactory(MsgpackClient client) {
      this.client = client;
    }

    public org.csploit.msf.api.Framework createFramework() {
      return new MsgpackProxy(client);
    }
  }

  public static class SessionFactory {
    private final MsgpackClient client;
    private final InternalFramework framework;

    public SessionFactory(MsgpackClient client, InternalFramework framework) {
      this.client = client;
      this.framework = framework;
    }

    public AbstractSession createSession(String type, int id)
      throws NameHelper.BadSessionTypeException {
      SessionProxy result;

      switch (type) {
        case "shell":
          result = new MsgpackProxy.ShellProxy(id);
          break;
        case "meterpreter":
          result = new MsgpackProxy.MeterpreterProxy(id);
          break;
        default:
          throw new NameHelper.BadSessionTypeException(type);
      }

      result.setFramework(framework);
      result.setClient(client);

      return result;
    }
  }

  public static class ModuleFactory {
    private final MsgpackClient client;
    private final InternalFramework framework;

    public ModuleFactory(MsgpackClient client, InternalFramework framework) {
      this.client = client;
      this.framework = framework;
    }

    public InternalModule createModule(String type, String refname)
      throws NameHelper.BadModuleTypeException {
      InternalModule module = org.csploit.msf.impl.Module.fromType(type, refname);

      module.setFramework(framework);

      if(module instanceof InternalExploit) {
        module = new ExploitProxy((InternalExploit) module);
      } else if(module instanceof InternalAuxiliary) {
        module = new AuxiliaryProxy((InternalAuxiliary) module);
      } else if(module instanceof Post) {
        module = new PostProxy(module);
      } else if(module instanceof Payload) {
        module = new PayloadProxy(module);
      }

      module.setFramework(framework);

      if(module instanceof MsgpackCommunicator) {
        ((MsgpackCommunicator) module).setClient(client);
      }

      return module;
    }
  }
}
