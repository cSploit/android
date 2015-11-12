package org.csploit.msf.impl;

import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.Session;
import org.csploit.msf.api.sessions.Interactive;
import org.csploit.msf.api.sessions.SingleCommandShell;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.TreeMap;

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
class SessionManager implements Offspring, Runnable {
  private static final int SLEEP_INTERVAL = 3000;
  private static final int WORK_INTERVAL = 300;

  private InternalFramework framework;
  private boolean running = false;
  private boolean watchList = false;
  private Thread thread = null;
  private final TreeMap<Integer, Session> map;

  public SessionManager() {
    map = new TreeMap<>();
  }

  @Override
  public void run() {
    Collection<Session> copy;
    String output;

    running = true;

    try {
      while (running) {
        synchronized (map) {
          copy = new ArrayList<>(map.values());
        }

        boolean interactingSessionsFound = false;

        for (Session s : copy) {
          if(!interactingSessionsFound && s instanceof Interactive) {
            try {
              interactingSessionsFound = ((Interactive) s).isInteracting();
            } catch (IOException | MsfException e) {
              // ingored
            }
          }

          if (!(s instanceof SingleCommandShell)) {
            continue;
          }

          try {
            output = ((SingleCommandShell) s).read();
            if (output != null && output.length() > 0 && haveFramework()) {
              getFramework().getEventManager().onSessionOutput(s, output);
            }
          } catch (IOException | MsfException e) {
            unregister(s);
          }
        }

        long interval = interactingSessionsFound ? WORK_INTERVAL : SLEEP_INTERVAL;

        if(watchList && haveFramework()) {
          long start = java.lang.System.currentTimeMillis();
          try {
            getFramework().getSessions();
          } catch (IOException | MsfException e) {
            // ignored
          }
          interval -= (java.lang.System.currentTimeMillis() - start);
        }

        if(interval > 0) {
          Thread.sleep(interval);
        }
      }
    } catch (InterruptedException e) {
      // break
    } finally {
      running = false;
    }
  }

  public void start() {
    stop();
    thread = new Thread(this, this.getClass().getSimpleName());
    thread.start();
  }

  public void stop() {
    running = false;
    try {
      if(thread != null && thread.isAlive())
        thread.join();
    } catch (InterruptedException e) {
      // ignore
    }
  }

  /**
   * set to watch session list or not
   */
  public void setWatchList(boolean watchList) {
    this.watchList = watchList;
  }

  public void register(Session s) {
    synchronized (map) {
      if (!map.containsKey(s.getId())) {
        map.put(s.getId(), s);
        map.notify();
        if(haveFramework()) {
          getFramework().getEventManager().onSessionOpened(s);
        }
      }
    }
  }

  public void unregister(int id) {
    synchronized (map) {
      Session s = map.remove(id);
      if (s != null && haveFramework()) {
        getFramework().getEventManager().onSessionClosed(s);
      }
    }
  }

  public void unregister(Session s) {
    unregister(s.getId());
  }

  public List<Session> getSessions() {
    synchronized (map) {
      return new ArrayList<>(map.values());
    }
  }

  public boolean containsSession(int id) {
    synchronized (map) {
      return map.containsKey(id);
    }
  }

  public boolean containsSession(Session s) {
    return containsSession(s.getId());
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
