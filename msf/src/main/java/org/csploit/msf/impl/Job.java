package org.csploit.msf.impl;

import java.util.Date;

/**
 * This class is the concrete representation of an abstract job.
 */
class Job implements org.csploit.msf.api.Job {

  private JobContainer container;
  private int id;
  private String name;
  protected Exploit exploit;
  protected Payload payload;
  private Date startTime;

  public Job(JobContainer container, int id, String name, Exploit exploit, Payload payload) {
    this.container = container;
    this.id = id;
    this.name = name;
    this.exploit = exploit;
    this.payload = payload;
  }

  public void setStartTime(Date startTime) {
    this.startTime = startTime;
  }

  public int getId() {
    return id;
  }

  public String getName() {
    return name;
  }

  public Date getStartTime() {
    return startTime;
  }

  public void stop() {
    container.remove(id);
  }
}
