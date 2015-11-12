package org.csploit.msf.api.listeners;

import org.csploit.msf.api.events.SessionEvent;
import org.csploit.msf.api.events.SessionOutputEvent;

/**
 * Listen for session events
 */
public interface SessionListener extends Listener {
  void onSessionOpened(SessionEvent e);
  void onSessionOutput(SessionOutputEvent e);
  void onSessionClosed(SessionEvent e);
}
