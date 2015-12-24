package org.csploit.msf.api.events;

import org.csploit.msf.api.Session;

import java.util.EventObject;

/**
 * A session event
 */
public abstract class SessionEvent extends EventObject {
  private int id;

  public SessionEvent(Object source, Session session) {
    super(source);

    id = session.getId();
  }

  public int getId() {
    return id;
  }
}
