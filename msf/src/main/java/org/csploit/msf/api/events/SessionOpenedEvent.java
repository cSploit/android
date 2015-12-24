package org.csploit.msf.api.events;

import org.csploit.msf.api.Session;

/**
 * A session has been opened
 */
public class SessionOpenedEvent extends SessionEvent {
  public SessionOpenedEvent(Object source, Session session) {
    super(source, session);
  }
}
