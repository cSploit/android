package org.csploit.msf.api;

import org.csploit.msf.api.listeners.Listener;

import java.io.IOException;
import java.util.List;

/**
 * A Metasploit Framework interface
 */
public interface Framework {
  List<? extends Session> getSessions() throws IOException, MsfException;
  List<? extends Exploit> getExploits() throws IOException, MsfException;
  List<? extends Payload> getPayloads() throws IOException, MsfException;
  List<? extends Post> getPosts() throws IOException, MsfException;
  Module getModule(String refname) throws IOException, MsfException;
  List<? extends Job> getJobs() throws IOException, MsfException;
  Job getJob(int id) throws IOException, MsfException;
  void setGlobalOption(String key, String value);
  void unsetGlobalOption(String key);
  void addSubscriber(Listener listener);
  void removeSubscriber(Listener listener);
}
