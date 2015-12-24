package org.csploit.msf.api.events;

import org.csploit.msf.api.Session;

/**
 * A session has been closed
 */
public class SessionClosedEvent extends SessionEvent {
  public SessionClosedEvent(Object source, Session session) {
    super(source, session);
  }
}
