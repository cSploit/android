package org.csploit.msf;

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
  private Framework framework;

  public SessionManager(Framework framework) {
    this.framework = framework;
  }

  @Override
  public Framework getFramework() {
    return framework;
  }

  @Override
  public boolean haveFramework() {
    return framework != null;
  }

  @Override
  public void setFramework(Framework framework) {
    this.framework = framework;
  }
}
