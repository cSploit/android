package org.csploit.msf.api.events;

import org.csploit.msf.api.Console;

/**
 * A console has been closed
 */
public class ConsoleClosedEvent extends ConsoleEvent {
  public ConsoleClosedEvent(Object source, Console console) {
    super(source, console);
  }
}
