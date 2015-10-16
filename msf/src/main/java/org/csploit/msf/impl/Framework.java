package org.csploit.msf.impl;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Represent a MetaSploit Framework instance
 */
class Framework implements DataHolder, org.csploit.msf.api.Framework {

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

  public List<Session> getSessions() {
    List<Integer> keys = new ArrayList<>(sessions.keySet());
    Collections.sort(keys);
    List<Session> result = new ArrayList<>(keys.size());
    for(int k : keys) {
      result.add(sessions.get(k));
    }
    return result;
  }

  public Session getSession(int id) {
    return sessions.get(id);
  }

  public void registerSession(Session session) {
    sessions.put(session.getId(), session);
  }

  public List<Job> getJobs() {
    List<Integer> keys = new ArrayList<>(jobs.keySet());
    Collections.sort(keys);
    List<Job> result = new ArrayList<>(keys.size());
    for(int k : keys) {
      result.add(jobs.get(k));
    }
    return result;
  }

  public Job getJob(int id) {
    return jobs.get(id);
  }

  public void registerJob(Job job) {
    jobs.put(job.getId(), job);
  }

  public void deleteJob(int id) {
    jobs.remove(id);
  }
}
