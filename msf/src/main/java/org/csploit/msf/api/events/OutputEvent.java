package org.csploit.msf.api.events;

/**
 * someone has printed something
 */
public interface OutputEvent extends Event {
  String getOutput();
}
