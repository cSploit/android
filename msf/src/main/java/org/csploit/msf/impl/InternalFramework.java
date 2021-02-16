package org.csploit.msf.impl;

import org.csploit.msf.api.Framework;

/**
 * Give access to some hidden parts of the framework
 */
interface InternalFramework extends Framework, DataHolder {
  ModuleManager getModuleManager();
  SessionManager getSessionManager();
  JobContainer getJobContainer();
  void registerJob(Job job);
  void unregisterJob(int id);
}
