package org.csploit.msf.session;

/**
 * Represent a session that can run scripts
 */
public interface Scriptable {
  boolean executeFile(String path, String[] args);
  int executeScript(String scriptName, String[] args);
}
