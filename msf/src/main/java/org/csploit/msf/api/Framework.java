package org.csploit.msf.api;

import java.util.List;

/**
 * A Metasploit Framework interface
 */
public interface Framework {
  List<? extends Session> getSessions();
  List<? extends Job> getJobs();
}
