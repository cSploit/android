package org.csploit.msf.api.events;

import org.csploit.msf.api.Console;

/**
 * A console has changed
 */
public class ConsoleChangedEvent extends ConsoleEvent {
  public ConsoleChangedEvent(Object source, Console console) {
    super(source, console);
  }
}
