package org.csploit.msf.api.events;

import org.csploit.msf.api.Console;

/**
 * A console printed something
 */
public class ConsoleOutputEvent extends ConsoleEvent implements OutputEvent {
  private String output;

  public ConsoleOutputEvent(Object source, Console console, String output) {
    super(source, console);
    this.output = output;
  }

  @Override
  public String getOutput() {
    return output;
  }
}
