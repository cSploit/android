package org.csploit.msf.api.listeners;

import org.csploit.msf.api.events.SessionClosedEvent;
import org.csploit.msf.api.events.SessionOpenedEvent;
import org.csploit.msf.api.events.SessionOutputEvent;

/**
 * Listen for session events
 */
public interface SessionListener extends java.util.EventListener {
  void onSessionOpened(SessionOpenedEvent e);
  void onSessionOutput(SessionOutputEvent e);
  void onSessionClosed(SessionClosedEvent e);
}
