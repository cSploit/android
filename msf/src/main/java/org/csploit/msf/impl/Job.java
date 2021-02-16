package org.csploit.msf.impl;

import org.csploit.msf.api.Payload;
import org.csploit.msf.api.Exploit;

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

  public Job(int id, String name, Exploit exploit, Payload payload) {
    this(null, id, name, exploit, payload);
  }

  public Job(int id, String name) {
    this(null, id, name, null, null);
  }

  void setStartTime(Date startTime) {
    this.startTime = startTime;
  }

  void setName(String name) {
    this.name = name;
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

  void setContainer(JobContainer container) {
    this.container = container;
  }

  public void setExploit(Exploit exploit) {
    this.exploit = exploit;
  }

  public void setPayload(Payload payload) {
    this.payload = payload;
  }
}
