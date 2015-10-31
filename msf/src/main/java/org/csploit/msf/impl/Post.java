package org.csploit.msf.impl;

import org.csploit.msf.api.MsfException;

import java.io.IOException;

/**
 * A Post-exploitation module.
 */
class Post extends Module implements org.csploit.msf.api.Post {

  public Post(String refname) {
    super(refname);
  }

  @Override
  public String getType() {
    return "post";
  }

  @Override
  public void execute() throws IOException, MsfException {
    throw new UnsupportedOperationException("not implemented yet");
  }
}
