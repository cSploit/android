package org.csploit.msf.api.events;

import org.csploit.msf.api.Console;

/**
 * A console has been opened
 */
public class ConsoleOpenedEvent extends ConsoleEvent {
  public ConsoleOpenedEvent(Object source, Console console) {
    super(source, console);
  }
}
