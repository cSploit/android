package org.csploit.msf.impl;

import org.csploit.msf.api.*;
import org.csploit.msf.api.Module;
import org.csploit.msf.api.Session;
import org.csploit.msf.api.events.SessionEvent;
import org.csploit.msf.api.events.SessionOutputEvent;
import org.csploit.msf.api.listeners.SessionListener;
import org.junit.Test;

import java.io.IOException;
import java.util.List;

import static org.junit.Assert.*;
import static org.hamcrest.CoreMatchers.*;

/**
 * Test the event manager
 */
public class EventManagerTest {

  private class DummySessionListener implements SessionListener {
    private int opened = -1;
    private int output = -1;
    private int closed = -1;
    private int counter = 0;

    @Override
    public synchronized void onSessionOpened(SessionEvent e) {
      opened = counter++;
      notifyAll();
    }

    @Override
    public synchronized void onSessionOutput(SessionOutputEvent e) {
      output = counter++;
      notifyAll();
    }

    @Override
    public synchronized void onSessionClosed(SessionEvent e) {
      closed = counter++;
      notifyAll();
    }

    public synchronized boolean isEverythingReceived() {
      return opened >= 0 && output >= 0 && closed >= 0;
    }

    public synchronized int getOpenedReceiveOrder() {
      return opened;
    }

    public synchronized int getOutputReceiveOrder() {
      return output;
    }

    public synchronized int getClosedReceiveOrder() {
      return closed;
    }

    public synchronized void reset() {
      opened = output = closed = -1;
    }
  }

  private class DummySession extends AbstractSession {
    public DummySession(int id) {
      super(id);
    }

    @Override
    public List<? extends Module> getCompatibleModules() throws IOException, MsfException {
      return null;
    }

    @Override
    public void close() throws IOException, MsfException {

    }
  }

  private EventManager buildLocalEventManager() {
    return new Framework().getEventManager();
  }

  @Test(timeout = 1000)
  public void testNotify() throws Exception {
    EventManager manager = buildLocalEventManager();
    DummySessionListener listener = new DummySessionListener();
    Session session = new DummySession(1);

    assertThat(listener.isEverythingReceived(), is(false));

    manager.addSubscriber(listener);

    manager.onSessionOpened(session);
    manager.onSessionOutput(session, "hello world");
    manager.onSessionClosed(session);

    synchronized (listener) {
      while (!listener.isEverythingReceived()) {
        listener.wait();
      }
    }

    assertThat(listener.getOpenedReceiveOrder(), is(0));
    assertThat(listener.getOutputReceiveOrder(), is(1));
    assertThat(listener.getClosedReceiveOrder(), is(2));

    manager.removeSubscriber(listener);

    manager.onSessionClosed(session);

    assertThat(listener.getClosedReceiveOrder(), is(2));
  }
}