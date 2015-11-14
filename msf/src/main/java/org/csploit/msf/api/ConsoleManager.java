package org.csploit.msf.api;

import org.csploit.msf.api.listeners.ConsoleListener;

import java.io.IOException;
import java.util.List;

/**
 * Manage consoles
 */
public interface ConsoleManager {
  /**
   * get a list of consoles managed by this {@link ConsoleManager}
   * @return a list of consoles managed by this {@link ConsoleManager}
   */
  List<? extends Console> getConsoles() throws IOException, MsfException;

  /**
   * Create a new console
   *
   * @param allowCommandPassthru Whether to allow
   *                             unrecognized commands to be executed by the system shell
   * @param realReadline Whether to use the system's
   *                     readline library instead of RBReadline
   * @param histFile Path to a file
   *                 where we can store command history
   * @param resources A list of resource files to load.
   *                  If no resources are given, will load the default resource script,
   *                  'msfconsole.rc' in the user's config directory
   * @param skipDatabaseInit Whether to skip connecting to the database
   *                         and running migrations
   * @return a new {@link Console}
   */
  Console create(boolean allowCommandPassthru, boolean realReadline,
                    String histFile, String[] resources, boolean skipDatabaseInit)
  throws IOException, MsfException;

  /**
   * Create a new Console using framework defaults settings
   *
   * @return a new {@link Console}
   */
  Console create() throws IOException, MsfException;

  /**
   * add an event subscriber
   * @param listener to add
   */
  void addSubscriber(ConsoleListener listener);

  /**
   * remove an event subscriber
   * @param listener to remove
   */
  void removeSubscriber(ConsoleListener listener);

  /**
   * start watching for consoles output
   */
  void start();

  /**
   * stop watching for consoles output
   */
  void stop();
}
