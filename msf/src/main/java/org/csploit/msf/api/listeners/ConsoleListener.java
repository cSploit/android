package org.csploit.msf.api.listeners;

import org.csploit.msf.api.events.ConsoleEvent;
import org.csploit.msf.api.events.ConsoleOutputEvent;

/**
 * Something that is listening on Console events
 */
public interface ConsoleListener {
  /**
   * A {@link org.csploit.msf.api.Console} has been opened
   * @param event event data
   */
  void onConsoleOpened(ConsoleEvent event);

  /**
   * A {@link org.csploit.msf.api.Console} has changed it's state
   * @param event event data
   */
  void onConsoleChanged(ConsoleEvent event);

  /**
   * A {@link org.csploit.msf.api.Console} has printed something
   * @param event event data
   */
  void onConsoleOutput(ConsoleOutputEvent event);

  /**
   * A {@link org.csploit.msf.api.Console} has been closed
   * @param event event data
   */
  void onConsoleClosed(ConsoleEvent event);
}
