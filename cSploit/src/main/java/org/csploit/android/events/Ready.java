package org.csploit.android.events;

/**
 * a child is ready to perform some specific operation.
 */
public class Ready implements Event {
  @Override
  public String toString() {
    return "Ready: {}";
  }
}
