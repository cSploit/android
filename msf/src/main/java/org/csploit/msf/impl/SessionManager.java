package org.csploit.msf.impl;

import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.Publisher;
import org.csploit.msf.api.Session;
import org.csploit.msf.api.events.SessionClosedEvent;
import org.csploit.msf.api.events.SessionOpenedEvent;
import org.csploit.msf.api.events.SessionOutputEvent;
import org.csploit.msf.api.listeners.SessionListener;
import org.csploit.msf.api.sessions.Interactive;
import org.csploit.msf.api.sessions.SingleCommandShell;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.TreeMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

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
class SessionManager implements Runnable, Publisher<SessionListener> {
  private static final int SLEEP_INTERVAL = 3000;
  private static final int WORK_INTERVAL = 300;

  private boolean running = false;
  private boolean watchList = false;
  private Thread thread = null;
  private final TreeMap<Integer, Session> map = new TreeMap<>();
  private final Set<SessionListener> listeners = new LinkedHashSet<>();
  private final ExecutorService executorService = Executors.newSingleThreadExecutor();

  public SessionManager() {

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
              // ignored
            }
          }

          if (!(s instanceof SingleCommandShell)) {
            continue;
          }

          try {
            output = ((SingleCommandShell) s).read();
            if (output != null && output.length() > 0) {
              notifySessionOutput(s, output);
            }
          } catch (IOException | MsfException e) {
            unregister(s);
          }
        }

        long interval = interactingSessionsFound ? WORK_INTERVAL : SLEEP_INTERVAL;

        if(watchList) {
          long start = java.lang.System.currentTimeMillis();
          try {
            getSessions();
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
      if(map.containsKey(s.getId())) {
        return;
      }
      map.put(s.getId(), s);
      map.notify();
    }
    notifySessionOpened(s);
  }

  public void unregister(Session s) {
    synchronized (map) {
      s = map.remove(s.getId());
      if(s == null)
        return;
    }
    notifySessionClosed(s);
  }

  @Override
  public void addSubscriber(SessionListener listener) {
    synchronized (listeners) {
      listeners.add(listener);
    }
  }

  @Override
  public void removeSubscriber(SessionListener listener) {
    synchronized (listeners) {
      listeners.add(listener);
    }
  }

  private void notifySessionOpened(Session session) {
    final SessionOpenedEvent event = new SessionOpenedEvent(this, session);

    executorService.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for(SessionListener listener : listeners) {
            listener.onSessionOpened(event);
          }
        }
      }
    });
  }

  private void notifySessionOutput(Session session, String output) {
    final SessionOutputEvent event = new SessionOutputEvent(this, session, output);

    executorService.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (listeners) {
          for(SessionListener listener : listeners) {
            listener.onSessionOutput(event);
          }
        }
      }
    });
  }

  private void notifySessionClosed(Session session) {
    final SessionClosedEvent event = new SessionClosedEvent(this, session);

    executorService.execute(new Runnable() {
      @Override
      public void run() {
        for(SessionListener listener : listeners) {
          listener.onSessionClosed(event);
        }
      }
    });
  }

  public List<Session> getSessions() throws IOException, MsfException {
    synchronized (map) {
      return new ArrayList<>(map.values());
    }
  }

  boolean containsSession(int id) {
    synchronized (map) {
      return map.containsKey(id);
    }
  }

  boolean containsSession(Session s) {
    return containsSession(s.getId());
  }
}
