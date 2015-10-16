package org.csploit.msf.api;

/**
 * Represent a module of the MSF
 */
public interface Module {

  String getRefname();
  void setRefname(String refname);

  String getFullName();
  String getShortName();
}
