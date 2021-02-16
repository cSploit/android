package org.csploit.msf.api.listeners;

import org.csploit.msf.api.events.ConsoleChangedEvent;
import org.csploit.msf.api.events.ConsoleClosedEvent;
import org.csploit.msf.api.events.ConsoleOpenedEvent;
import org.csploit.msf.api.events.ConsoleOutputEvent;

import java.util.EventListener;

/**
 * Something that is listening on Console events
 */
public interface ConsoleListener extends EventListener {
  /**
   * A {@link org.csploit.msf.api.Console} has been opened
   * @param event event data
   */
  void onConsoleOpened(ConsoleOpenedEvent event);

  /**
   * A {@link org.csploit.msf.api.Console} has changed it's state
   * @param event event data
   */
  void onConsoleChanged(ConsoleChangedEvent event);

  /**
   * A {@link org.csploit.msf.api.Console} has printed something
   * @param event event data
   */
  void onConsoleOutput(ConsoleOutputEvent event);

  /**
   * A {@link org.csploit.msf.api.Console} has been closed
   * @param event event data
   */
  void onConsoleClosed(ConsoleClosedEvent event);
}
