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

  public Collection<Session> getSessions() {
    return sessions.values();
  }

  public Session getSession(int id) {
    return sessions.get(id);
  }

  public void registerSession(Session session) {
    sessions.put(session.getId(), session);
  }

  public Collection<Job> getJobs() {
    return jobs.values();
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
