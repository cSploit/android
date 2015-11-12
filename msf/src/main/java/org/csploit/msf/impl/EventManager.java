package org.csploit.msf.impl;

import org.csploit.msf.api.Session;
import org.csploit.msf.api.events.Event;
import org.csploit.msf.api.events.SessionEvent;
import org.csploit.msf.api.events.SessionOutputEvent;
import org.csploit.msf.api.listeners.Listener;
import org.csploit.msf.api.listeners.SessionListener;

import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Manage listeners and events
 */
class EventManager {
  private ExecutorService notifier = Executors.newSingleThreadExecutor();
  private final Set<SessionListener> sessionSubscribers = new HashSet<>();

  public void addSubscriber(Listener listener) {
    if(listener instanceof SessionListener) {
      synchronized (sessionSubscribers) {
        sessionSubscribers.add((SessionListener)  listener);
      }
    }
  }

  public void removeSubscriber(Listener listener) {
    if(listener instanceof SessionListener) {
      synchronized (sessionSubscribers) {
        sessionSubscribers.remove(listener);
      }
    }
  }

  public void onSessionOpened(Session session) {
    final SessionEvent event = new SessionEventImpl(session.getId());

    notifier.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (sessionSubscribers) {
          for (SessionListener listener : sessionSubscribers) {
            listener.onSessionOpened(event);
          }
        }
      }
    });
  }

  public void onSessionOutput(Session session, String output) {
    final SessionOutputEvent event = new SessionOutputEventImpl(session.getId(), output);

    notifier.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (sessionSubscribers) {
          for(SessionListener listener : sessionSubscribers) {
            listener.onSessionOutput(event);
          }
        }
      }
    });
  }

  public void onSessionClosed(Session session) {
    final SessionEvent event = new SessionEventImpl(session.getId());

    notifier.execute(new Runnable() {
      @Override
      public void run() {
        synchronized (sessionSubscribers) {
          for (SessionListener listener : sessionSubscribers) {
            listener.onSessionClosed(event);
          }
        }
      }
    });
  }

  private static class SessionEventImpl implements SessionEvent {
    private final int id;

    public SessionEventImpl(int id) {
      this.id = id;
    }

    @Override
    public int getId() {
      return id;
    }
  }

  private static class SessionOutputEventImpl extends SessionEventImpl implements SessionOutputEvent {
    private final String output;
    public SessionOutputEventImpl(int id, String output) {
      super(id);
      this.output = output;
    }

    @Override
    public String getOutput() {
      return output;
    }
  }
}
