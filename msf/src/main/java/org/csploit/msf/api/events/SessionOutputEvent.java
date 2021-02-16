package org.csploit.msf.api.events;

import org.csploit.msf.api.Session;

/**
 * A session printed something
 */
public class SessionOutputEvent extends SessionEvent implements OutputEvent {
  private String output;

  public SessionOutputEvent(Object source, Session session, String output) {
    super(source, session);
    this.output = output;
  }

  @Override
  public String getOutput() {
    return output;
  }
}
