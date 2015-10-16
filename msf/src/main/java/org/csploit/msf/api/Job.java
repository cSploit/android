package org.csploit.msf.api;

import java.util.Date;

/**
 * This class is the representation of an abstract job.
 */
public interface Job {
  int getId();
  String getName();
  Date getStartTime();
  void stop();
}
