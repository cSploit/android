package org.csploit.msf.api.sessions;

import org.csploit.msf.api.Session;

/**
 * Represent a session that can run scripts
 */
public interface Scriptable extends Session {
  boolean executeFile(String path, String[] args);
  int executeScript(String scriptName, String[] args);
}
