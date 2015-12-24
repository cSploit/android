package org.csploit.msf.impl;

import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.Session;
import org.csploit.msf.api.events.SessionClosedEvent;
import org.csploit.msf.api.events.SessionOpenedEvent;
import org.csploit.msf.api.events.SessionOutputEvent;
import org.csploit.msf.api.listeners.SessionListener;
import org.junit.Test;

import java.io.IOException;
import java.util.List;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;

/**
 * test the session manager
 */
public class SessionManagerTest {

  private class DummySession extends AbstractSession {

    public DummySession(int id) {
      super(id);
    }

    @Override
    public List<? extends Module> getCompatibleModules() throws IOException, MsfException {
      return null;
    }

    @Override
    public void close() throws IOException, MsfException { }
  }

  private class DummySessionListener implements SessionListener {
    private boolean opened = false;

    @Override
    public synchronized void onSessionOpened(SessionOpenedEvent e) {
      opened = true;
      notifyAll();
    }

    @Override
    public void onSessionOutput(SessionOutputEvent e) { }

    @Override
    public synchronized void onSessionClosed(SessionClosedEvent e) {
      opened = false;
      notifyAll();
    }

    public synchronized boolean isOpened() {
      return opened;
    }
  }

  private SessionManager buildLocalSessionManager() {
    return new Framework().getSessionManager();
  }

  @Test
  public void testCreation() throws Exception {
    SessionManager manager = buildLocalSessionManager();

    assertThat(manager.getSessions().isEmpty(), is(true));
  }

  @Test
  public void testInsertGetRemoveContains() throws Exception {
    SessionManager manager = buildLocalSessionManager();
    Session session = new DummySession(1);
    manager.register(session);

    assertThat(manager.containsSession(session), is(true));

    assertThat(manager.getSessions().size(), is(1));
    assertThat(manager.getSessions().get(0), is(session));

    manager.unregister(session);

    assertThat(manager.getSessions().isEmpty(), is(true));
  }

  @Test(timeout = 1000)
  public void testEvents() throws Exception{
    InternalFramework framework = new Framework();
    SessionManager manager = framework.getSessionManager();
    DummySessionListener listener = new DummySessionListener();
    Session session = new DummySession(1);

    assertThat(listener.isOpened(), is(false));

    framework.addSubscriber(listener);
    manager.register(session);

    synchronized (listener) {
      while(!listener.isOpened()) {
        listener.wait();
      }
    }

    manager.unregister(session);

    synchronized (listener) {
      while(listener.isOpened()) {
        listener.wait();
      }
    }
  }
}