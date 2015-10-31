package org.csploit.msf.impl;

import org.csploit.msf.api.License;
import org.csploit.msf.api.Exploit;
import org.csploit.msf.api.Payload;
import org.csploit.msf.api.Module;
import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.module.Rank;
import org.csploit.msf.impl.module.AuxiliaryAction;
import org.csploit.msf.impl.module.Reference;
import org.csploit.msf.impl.module.SiteReference;
import org.csploit.msf.impl.module.Target;
import org.csploit.msf.impl.options.*;

import java.io.File;
import java.io.IOException;
import java.util.Date;
import java.util.Map;
import java.util.regex.Pattern;

/**
 * Load MSF content from Msgpack responses
 */
@SuppressWarnings("unchecked, rawtypes")
class MsgpackLoader {

  private final MsgpackClient client;
  private final InternalFramework framework;

  public MsgpackLoader(MsgpackClient client, InternalFramework framework) {
    this.client = client;
    this.framework = framework;
  }

  public Module loadModule(String fullname) throws IOException, MsfException {
    String[] parts = NameHelper.getTypeAndRefnameFromFullname(fullname);
    return loadModule(parts[0], parts[1]);
  }

  public Module loadModule(String type, String name) throws IOException, MsfException {
    NameHelper.assertValidModuleType(type);
    InternalModule result = framework.getModuleManager().get(type, name);

    if(result != null && !isModuleAlreadyFetched(result)) {
      fillModule(result, client.getModuleInfo(type, name));
    } else if(result == null) {
      result = fetchModule(type, name);
      if(result instanceof MsgpackProxy.MsgpackCommunicator) {
        ((MsgpackProxy.MsgpackCommunicator) result).setClient(client);
      }
      framework.getModuleManager().put(result);
    }

    return result;
  }

  public Module lazyLoadModule(String type, String name) throws NameHelper.BadModuleTypeException {
    InternalModule result = framework.getModuleManager().get(type, name);

    if(result != null) {
      return result;
    }

    result = MsgpackProxy.Factory.newModule(type, name);

    if(result instanceof MsgpackProxy.MsgpackCommunicator) {
      ((MsgpackProxy.MsgpackCommunicator) result).setClient(client);
    }

    framework.getModuleManager().put(result);

    return result;
  }

  private InternalModule fetchModule(String type, String refname) throws IOException, MsfException, IllegalArgumentException {
    InternalModule module = MsgpackProxy.Factory.newModule(type, refname);
    MsgpackClient.ModuleInfo response = client.getModuleInfo(type, refname);

    fillModule(module, response);

    return module;
  }

  static boolean isModuleAlreadyFetched(Module module) {
    String desc = module.getDescription();
    return desc != null && desc.length() > 0;
  }

  /**
   * fill a module with data from the "module.info" response
   * @param module the module to fill info in
   * @param info ModuleInfo from Msgpack API
   */
  static void fillModule(InternalModule module, MsgpackClient.ModuleInfo info) {
    module.setName(info.name);
    module.setDescription(info.description);
    module.setLicense(License.fromString(info.license));
    module.setRank(Rank.fromValue(info.rank));

    int maxTargetId, maxActionId;

    maxTargetId = maxActionId = -1;

    if(info.targets != null) {
      for(Integer key : info.targets.keySet()) {
        if(key >= maxTargetId) {
          maxTargetId = key;
        }
      }
    }

    if(info.actions != null) {
      for(Integer key : info.actions.keySet()) {
        if(key >= maxActionId) {
          maxActionId = key;
        }
      }
    }

    Reference[] references = new Reference[info.references.length];
    Author[] authors = new Author[info.authors.length];
    Target[] targets = maxTargetId < 0 ? null : new Target[maxTargetId + 1];
    AuxiliaryAction[] actions = maxActionId < 0 ? null : new AuxiliaryAction[maxActionId + 1];

    for(int i = 0; i < references.length; i++) {
      String[] ctx = info.references[i];
      String id = ctx[0];
      String val = ctx[1];
      references[i] = new SiteReference(id, val);
    }

    module.setReferences(references);

    for(int i = 0; i < authors.length; i++) {
      String author = info.authors[i];
      authors[i] = Author.fromString(author);
    }

    module.setAuthors(authors);

    if(targets != null && module instanceof InternalExploit) {
      for(int i = 0; i < targets.length; i++) {
        String name = info.targets.get(i);
        targets[i] = new Target(name);
      }
      ((InternalExploit)module).setTargets(targets);

      if(info.defaultTarget >= 0) {
        ((InternalExploit)module).setDefaultTarget(info.defaultTarget);
      }
    }

    if(actions != null && module instanceof Auxiliary) {
      for( int i = 0 ; i < actions.length; i++) {
        String name = info.actions.get(i);
        actions[i] = new AuxiliaryAction(name);
      }
      ((Auxiliary) module).setActions(actions);

      if(info.defaultAction >= 0) {
        ((Auxiliary) module).setDefaultAction(info.defaultAction);
      }
    }
  }

  static void fillModuleOptions(InternalModule module, Map<String, MsgpackClient.OptionInfo> list) {

    for(String name : list.keySet()) {
      MsgpackClient.OptionInfo info = list.get(name);
      Option result;

      switch (info.type) {
        case "address":
          result = new AddressOption(name, info.required, info.description, (String) info.defaultValue);
          break;
        case "addressrange":
          result = new AddressRangeOption(name, info.required, info.description, null);
           break;
        case "bool":
          Boolean defValue = getBooleanDefaultValue(info.defaultValue);
          result = new BooleanOption(name, info.required, info.description, defValue);
          break;
        case "enum":
          String[] values = info.enums == null ? new String[] {(String) info.defaultValue} : info.enums;
          result = new EnumOption(name, info.required, info.description, (String) info.defaultValue, values);
          break;
        case "port":
          result = new PortOption(name, info.required, info.description, (Integer) info.defaultValue);
          break;
        case "path":
          result = new PathOption(name, info.required, info.description,
                  info.defaultValue == null ? null : new File((String) info.defaultValue));
          break;
        case "integer":
          Long longDefValue = getLongDefaultValue(info.defaultValue);
          result = new IntegerOption(name, info.required, info.description, longDefValue);
          break;
        case "regexp":
          result = new RegexpOption(name, info.required, info.description,
                  info.defaultValue == null ? null : Pattern.compile((String) info.defaultValue));
          break;
        case "raw":
          result = new RawOption(name, info.required, info.description, (byte[]) info.defaultValue);
          break;

        default: // string
          String s = info.defaultValue == null ? null : info.defaultValue.toString();
          result = new StringOption(name, info.required, info.description, s);
      }

      module.registerOption(name, result, info.advanced, info.evasion);
    }
  }

  private static Boolean getBooleanDefaultValue(Object value) {
    if(value instanceof String) {
      String str = ((String) value).toLowerCase();
      return str.startsWith("y") || str.equals("true") || str.equals("1");
    }
    return (Boolean) value;
  }

  private static Long getLongDefaultValue(Object value) {
    if(value instanceof Integer) {
      return ((Integer) value).longValue();
    } else if(value instanceof String) {
      return Long.parseLong((String) value);
    }
    return (Long) value;
  }

  public Session buildSession(int id, MsgpackClient.SessionInfo info)
          throws NameHelper.BadSessionTypeException {
    Session session;

    session = MsgpackProxy.Factory.newSessionFromType(info.type, id);

    session.setFramework(framework);
    ((MsgpackProxy.MsgpackCommunicator) session).setClient(client);

    session.setLocalTunnel(info.tunnelLocal);
    session.setPeerTunnel(info.tunnelPeer);
    session.setDescription(info.description);
    session.setInfo(info.info);
    session.setWorkspace(info.workspace);
    session.setSessionHost(info.sessionHost);
    session.setSessionPort(info.sessionPort);
    session.setTargetHost(info.targetHost);
    session.setUsername(info.username);
    session.setUuid(info.uuid);
    session.setRoutes(info.routes);

    if(info.viaExploit != null && info.viaExploit.length() > 0) {
      try {
        session.setExploit((Exploit) lazyLoadModule("exploit", info.viaExploit));
      } catch (NameHelper.BadModuleTypeException e) {
        // ignored
      }
    }

    if(info.viaPayload != null && info.viaPayload.length() > 0) {
      try {
        session.setPayload((Payload) lazyLoadModule("payload", info.viaPayload));
      } catch (NameHelper.BadModuleTypeException e) {
        // ignored
      }
    }

    if(session instanceof InternalMeterpreter &&
            info.platform != null && info.platform.length() > 0) {
      ((InternalMeterpreter) session).setPlatform(info.platform);
    }

    return session;
  }

  public Job loadJob(int id) throws IOException, MsfException {
    Job result = framework.getJobContainer().get(id);
    MsgpackClient.JobInfo info;

    if(result != null && !isJobAlreadyFetched(result)) {
      info = client.getJobInfo(id);
      fillJob(result, info);
    } else if(result == null) {
      result = new Job(id, null);
      fillJob(result, client.getJobInfo(id));
      framework.registerJob(result);
    }

    return result;
  }

  public Job lazyLoadJob(int id, String name) {
    Job result = framework.getJobContainer().get(id);

    if(result != null) {
      return result;
    }

    result = new Job(id, name);

    framework.registerJob(result);

    return result;
  }

  private boolean isJobAlreadyFetched(Job job) {
    return job.getStartTime() != null;
  }

  Job fillJob(Job job, MsgpackClient.JobInfo info) {
    job.setName(info.name);
    job.setStartTime(new Date(info.startTime));

    return job;
  }
}
