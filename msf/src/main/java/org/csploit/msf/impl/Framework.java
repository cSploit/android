package org.csploit.msf.impl;

import org.csploit.msf.api.Exploit;
import org.csploit.msf.api.Module;
import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.Payload;
import org.csploit.msf.api.Post;
import org.csploit.msf.api.Session;
import org.csploit.msf.api.listeners.SessionListener;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.EventListener;
import java.util.List;

/**
 * Represent a Metasploit Framework instance
 */
class Framework implements DataHolder, InternalFramework {

  private DataStore dataStore;
  private ModuleManager moduleManager;
  private JobContainer jobs;
  private SessionManager sessionManager;

  public Framework() {
    dataStore = new DataStore();
    moduleManager = new ModuleManager(this, "all");
    jobs = new JobContainer();
    sessionManager = new SessionManager();

    sessionManager.start();
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

  @Override
  public void unsetGlobalOption(String key) {
    getDataStore().remove(key);
  }

  @Override
  public void addSubscriber(EventListener listener) throws IOException, MsfException {
    if(listener instanceof SessionListener) {
      sessionManager.addSubscriber((SessionListener) listener);
    }
  }

  @Override
  public void removeSubscriber(EventListener listener) throws IOException, MsfException {
    if(listener instanceof SessionListener) {
      sessionManager.removeSubscriber((SessionListener) listener);
    }
  }

  @Override
  public List<Session> getSessions() throws IOException, MsfException {
    return sessionManager.getSessions();
  }

  @Override
  public Module getModule(String refname) throws IOException, MsfException {
    return moduleManager.get(refname);
  }

  @Override
  public List<? extends Exploit> getExploits() throws IOException, MsfException {
    return moduleManager.<InternalExploit>getOfType("exploit");
  }

  @Override
  public List<? extends Payload> getPayloads() throws IOException, MsfException {
    return moduleManager.<org.csploit.msf.impl.Payload>getOfType("payload");
  }

  @Override
  public List<? extends Post> getPosts() throws IOException, MsfException {
    return moduleManager.<org.csploit.msf.impl.Post>getOfType("post");
  }

  @Override
  public SessionManager getSessionManager() {
    return sessionManager;
  }

  @Override
  public org.csploit.msf.api.Job getJob(int id) throws IOException, MsfException {
    return jobs.get(id);
  }

  public List<Job> getJobs() throws IOException, MsfException {
    List<Integer> keys = new ArrayList<>(jobs.keySet());
    Collections.sort(keys);
    List<Job> result = new ArrayList<>(keys.size());
    for(int k : keys) {
      result.add(jobs.get(k));
    }
    return result;
  }

  @Override
  public JobContainer getJobContainer() {
    return jobs;
  }

  public void registerJob(Job job) {
    jobs.put(job.getId(), job);
  }

  public void unregisterJob(int id) {
    jobs.remove(id);
  }

  @Override
  protected void finalize() throws Throwable {
    sessionManager.stop();
    super.finalize();
  }
}
