package org.csploit.msf.api.events;

import org.csploit.msf.api.Console;

import java.util.EventObject;

/**
 * An event related to a {@link org.csploit.msf.api.Console}
 */
public abstract class ConsoleEvent extends EventObject {
  private int id;

  public ConsoleEvent(Object source, Console console) {
    super(source);
    this.id = console.getId();
  }

  public int getId() {
    return id;
  }
}
