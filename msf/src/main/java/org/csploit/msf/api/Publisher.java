package org.csploit.msf.api;

import java.io.IOException;
import java.util.EventListener;

/**
 * An event publisher
 */
public interface Publisher<T extends EventListener> {
  /**
   * add an event subscriber
   * @param listener to add
   */
  void addSubscriber(T listener) throws IOException, MsfException;
  /**
   * remove an event subscriber
   * @param listener to remove
   */
  void removeSubscriber(T listener) throws IOException, MsfException;
}
