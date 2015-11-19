package org.csploit.msf.impl;

import org.csploit.msf.api.MsfException;

import java.io.IOException;

/**
 * A Console implementor that uses Msgpack RPC
 */
class MsgpackConsole implements InternalConsole {
  private final MsgpackClient client;
  private final int id;
  private String prompt;
  private boolean busy;
  private boolean interacting = false;
  private boolean changed = false;

  public MsgpackConsole(MsgpackClient client, int id) {
    this.client = client;
    this.id = id;
  }

  @Override
  public int getId() {
    return id;
  }

  @Override
  public String getPrompt() {
    return prompt;
  }

  public void setPrompt(String prompt) {
    changed |= this.prompt == null ? prompt != null : !this.prompt.equals(prompt);
    this.prompt = prompt;
  }

  @Override
  public boolean isBusy() {
    return busy;
  }

  public void setBusy(boolean busy) {
    changed |= this.busy != busy;
    this.busy = busy;
  }

  @Override
  public void close() throws IOException, MsfException {
    client.closeConsole(id);
  }

  @Override
  public String read() throws IOException, MsfException {
    MsgpackClient.ConsoleReadInfo info = client.readFromConsole(id);

    setPrompt(info.prompt);
    setBusy(info.busy);

    return info.data;
  }

  @Override
  public void write(String data) throws IOException, MsfException {
    int sent = client.writeToConsole(id, data);
    while(sent < data.length()) {
      client.writeToConsole(id, data.substring(sent));
    }
  }

  @Override
  public String[] tab(String part) throws IOException, MsfException {
    return client.getConsoleTabs(id, part);
  }

  @Override
  public void killSession() throws IOException, MsfException {
    client.killSessionInConsole(id);
  }

  @Override
  public void detachSession() throws IOException, MsfException {
    client.detachSessionFromConsole(id);
  }

  @Override
  public void interact() throws IOException, MsfException {
    interacting = true;
  }

  @Override
  public void suspend() throws IOException, MsfException {
    interacting = false;
  }

  @Override
  public void complete() throws IOException, MsfException {
    suspend();
    close();
  }

  @Override
  public boolean isInteracting() throws IOException, MsfException {
    return interacting;
  }

  @Override
  public void clearChanged() {
    changed = false;
  }

  @Override
  public boolean hasChanged() {
    return changed;
  }

  @Override
  public String toString() {
    return String.format("MsgpackConsole: {id=%d, busy=%s}", id, busy);
  }
}
