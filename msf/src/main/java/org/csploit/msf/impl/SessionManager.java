package org.csploit.msf.impl;

import java.util.HashMap;

/**
 * The purpose of the session manager is to keep track of sessions that are
 * created during the course of a framework instance's lifetime.  When
 * exploits succeed, the payloads they use will create a session object,
 * where applicable, there will implement zero or more of the core
 * supplied interfaces for interacting with that session.  For instance,
 * if the payload supports reading and writing from an executed process,
 * the session would implement SimpleCommandShell in a method that is
 * applicable to the way that the command interpreter is communicated
 * with.
 */
class SessionManager extends HashMap<Integer, Session> implements Offspring {
  private InternalFramework framework;

  public SessionManager(InternalFramework framework) {
    this.framework = framework;
  }

  @Override
  public InternalFramework getFramework() {
    return framework;
  }

  @Override
  public boolean haveFramework() {
    return framework != null;
  }

  @Override
  public void setFramework(InternalFramework framework) {
    this.framework = framework;
  }
}
