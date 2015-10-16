package org.csploit.msf.api;

/**
 * The session class represents a post-exploitation, uh, session.
 * Sessions can be written to, read from, and interacted with.  The
 * underlying medium on which they are backed is arbitrary.  For
 * instance, when an exploit is provided with a command shell,
 * either through a network connection or locally, the session's
 * read and write operations end up reading from and writing to
 * the shell that was spawned.  The session object can be seen
 * as a general means of interacting with various post-exploitation
 * payloads through a common interface that is not necessarily
 * tied to a network connection.
 */
public interface Session {
     int getId();
     String getLocalTunnel();
     String getPeerTunnel();
     Exploit getExploit();
     Payload getPayload();
     String getDescription();
     String getInfo();
     String getWorkspace();
     String getSessionHost();
     int getSessionPort();
     String getTargetHost();
     String getUsername();
     String getUuid();
}
