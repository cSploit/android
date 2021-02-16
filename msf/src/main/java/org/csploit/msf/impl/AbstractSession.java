package org.csploit.msf.impl;

import org.csploit.msf.api.Exploit;
import org.csploit.msf.api.Payload;

/**
 * The session class represents a post-exploitation, uh, session.
 * Sessions can be written to, read from, and interacted with.  The
 * underlying medium on which they are backed is arbitrary.  For
 * instance, when an exploit is provided with a command shell,
 * either through a network connection or locally, the session's
 * read and write operations end up reading from and writing to
 * the shell that was spawned.  The session object can be seen
 * as a general means of interacting with various post-exploitation
 * payloads through a common interface that is not necessarily
 * tied to a network connection.
 */
abstract class AbstractSession implements Offspring, org.csploit.msf.api.Session {
  private InternalFramework framework;
  private int id;
  private String localTunnel;
  private String peerTunnel;
  private Exploit exploit;
  private Payload payload;
  private String description;
  private String info;
  private String workspace;
  private String sessionHost;
  private int sessionPort;
  private String targetHost;
  private String username;
  private String uuid;
  private String[] routes;

  public AbstractSession(int id) {
    this.id = id;
  }

  public int getId() {
    return id;
  }

  public String getLocalTunnel() {
    return localTunnel;
  }

  public void setLocalTunnel(String localTunnel) {
    this.localTunnel = localTunnel;
  }

  public String getPeerTunnel() {
    return peerTunnel;
  }

  public void setPeerTunnel(String peerTunnel) {
    this.peerTunnel = peerTunnel;
  }

  public Exploit getExploit() {
    return exploit;
  }

  public void setExploit(Exploit exploit) {
    this.exploit = exploit;
  }

  public Payload getPayload() {
    return payload;
  }

  public void setPayload(Payload payload) {
    this.payload = payload;
  }

  public String getDescription() {
    return description;
  }

  public void setDescription(String description) {
    this.description = description;
  }

  public String getInfo() {
    return info;
  }

  public void setInfo(String info) {
    this.info = info;
  }

  public String getWorkspace() {
    return workspace;
  }

  public void setWorkspace(String workspace) {
    this.workspace = workspace;
  }

  public String getSessionHost() {
    return sessionHost;
  }

  public void setSessionHost(String sessionHost) {
    this.sessionHost = sessionHost;
  }

  public int getSessionPort() {
    return sessionPort;
  }

  public void setSessionPort(int sessionPort) {
    this.sessionPort = sessionPort;
  }

  public String getTargetHost() {
    return targetHost;
  }

  public void setTargetHost(String targetHost) {
    this.targetHost = targetHost;
  }

  public String getUsername() {
    return username;
  }

  public void setUsername(String username) {
    this.username = username;
  }

  public String getUuid() {
    return uuid;
  }

  public void setUuid(String uuid) {
    this.uuid = uuid;
  }

  public String[] getRoutes() {
    return routes;
  }

  public void setRoutes(String[] routes) {
    this.routes = routes;
  }

  @Override
  public boolean haveFramework() {
    return framework != null;
  }

  @Override
  public InternalFramework getFramework() {
    return framework;
  }

  @Override
  public void setFramework(InternalFramework framework) {
    this.framework = framework;
  }
}
